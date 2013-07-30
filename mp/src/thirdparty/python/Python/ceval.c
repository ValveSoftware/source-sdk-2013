
/* Execute compiled code */

/* XXX TO DO:
   XXX speed up searching for keywords by using a dictionary
   XXX document it!
   */

/* enable more aggressive intra-module optimizations, where available */
#define PY_LOCAL_AGGRESSIVE

#include "Python.h"

#include "code.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#include <ctype.h>

#ifndef WITH_TSC

#define READ_TIMESTAMP(var)

#else

typedef unsigned long long uint64;

/* PowerPC support.
   "__ppc__" appears to be the preprocessor definition to detect on OS X, whereas
   "__powerpc__" appears to be the correct one for Linux with GCC
*/
#if defined(__ppc__) || defined (__powerpc__)

#define READ_TIMESTAMP(var) ppc_getcounter(&var)

static void
ppc_getcounter(uint64 *v)
{
    register unsigned long tbu, tb, tbu2;

  loop:
    asm volatile ("mftbu %0" : "=r" (tbu) );
    asm volatile ("mftb  %0" : "=r" (tb)  );
    asm volatile ("mftbu %0" : "=r" (tbu2));
    if (__builtin_expect(tbu != tbu2, 0)) goto loop;

    /* The slightly peculiar way of writing the next lines is
       compiled better by GCC than any other way I tried. */
    ((long*)(v))[0] = tbu;
    ((long*)(v))[1] = tb;
}

#elif defined(__i386__)

/* this is for linux/x86 (and probably any other GCC/x86 combo) */

#define READ_TIMESTAMP(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

#elif defined(__x86_64__)

/* for gcc/x86_64, the "A" constraint in DI mode means *either* rax *or* rdx;
   not edx:eax as it does for i386.  Since rdtsc puts its result in edx:eax
   even in 64-bit mode, we need to use "a" and "d" for the lower and upper
   32-bit pieces of the result. */

#define READ_TIMESTAMP(val) \
    __asm__ __volatile__("rdtsc" : \
                         "=a" (((int*)&(val))[0]), "=d" (((int*)&(val))[1]));


#else

#error "Don't know how to implement timestamp counter for this architecture"

#endif

void dump_tsc(int opcode, int ticked, uint64 inst0, uint64 inst1,
              uint64 loop0, uint64 loop1, uint64 intr0, uint64 intr1)
{
    uint64 intr, inst, loop;
    PyThreadState *tstate = PyThreadState_Get();
    if (!tstate->interp->tscdump)
        return;
    intr = intr1 - intr0;
    inst = inst1 - inst0 - intr;
    loop = loop1 - loop0 - intr;
    fprintf(stderr, "opcode=%03d t=%d inst=%06lld loop=%06lld\n",
            opcode, ticked, inst, loop);
}

#endif

/* Turn this on if your compiler chokes on the big switch: */
/* #define CASE_TOO_BIG 1 */

#ifdef Py_DEBUG
/* For debugging the interpreter: */
#define LLTRACE  1      /* Low-level trace feature */
#define CHECKEXC 1      /* Double-check exception checking */
#endif

typedef PyObject *(*callproc)(PyObject *, PyObject *, PyObject *);

/* Forward declarations */
#ifdef WITH_TSC
static PyObject * call_function(PyObject ***, int, uint64*, uint64*);
#else
static PyObject * call_function(PyObject ***, int);
#endif
static PyObject * fast_function(PyObject *, PyObject ***, int, int, int);
static PyObject * do_call(PyObject *, PyObject ***, int, int);
static PyObject * ext_do_call(PyObject *, PyObject ***, int, int, int);
static PyObject * update_keyword_args(PyObject *, int, PyObject ***,
                                      PyObject *);
static PyObject * update_star_args(int, int, PyObject *, PyObject ***);
static PyObject * load_args(PyObject ***, int);
#define CALL_FLAG_VAR 1
#define CALL_FLAG_KW 2

#ifdef LLTRACE
static int lltrace;
static int prtrace(PyObject *, char *);
#endif
static int call_trace(Py_tracefunc, PyObject *, PyFrameObject *,
                      int, PyObject *);
static int call_trace_protected(Py_tracefunc, PyObject *,
                                PyFrameObject *, int, PyObject *);
static void call_exc_trace(Py_tracefunc, PyObject *, PyFrameObject *);
static int maybe_call_line_trace(Py_tracefunc, PyObject *,
                                 PyFrameObject *, int *, int *, int *);

static PyObject * cmp_outcome(int, PyObject *, PyObject *);
static PyObject * import_from(PyObject *, PyObject *);
static int import_all_from(PyObject *, PyObject *);
static void format_exc_check_arg(PyObject *, const char *, PyObject *);
static void format_exc_unbound(PyCodeObject *co, int oparg);
static PyObject * unicode_concatenate(PyObject *, PyObject *,
                                      PyFrameObject *, unsigned char *);
static PyObject * special_lookup(PyObject *, _Py_Identifier *);

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"
#define GLOBAL_NAME_ERROR_MSG \
    "global name '%.200s' is not defined"
#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

/* Dynamic execution profile */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
static long dxpairs[257][256];
#define dxp dxpairs[256]
#else
static long dxp[256];
#endif
#endif

/* Function call profile */
#ifdef CALL_PROFILE
#define PCALL_NUM 11
static int pcall[PCALL_NUM];

#define PCALL_ALL 0
#define PCALL_FUNCTION 1
#define PCALL_FAST_FUNCTION 2
#define PCALL_FASTER_FUNCTION 3
#define PCALL_METHOD 4
#define PCALL_BOUND_METHOD 5
#define PCALL_CFUNCTION 6
#define PCALL_TYPE 7
#define PCALL_GENERATOR 8
#define PCALL_OTHER 9
#define PCALL_POP 10

/* Notes about the statistics

   PCALL_FAST stats

   FAST_FUNCTION means no argument tuple needs to be created.
   FASTER_FUNCTION means that the fast-path frame setup code is used.

   If there is a method call where the call can be optimized by changing
   the argument tuple and calling the function directly, it gets recorded
   twice.

   As a result, the relationship among the statistics appears to be
   PCALL_ALL == PCALL_FUNCTION + PCALL_METHOD - PCALL_BOUND_METHOD +
                PCALL_CFUNCTION + PCALL_TYPE + PCALL_GENERATOR + PCALL_OTHER
   PCALL_FUNCTION > PCALL_FAST_FUNCTION > PCALL_FASTER_FUNCTION
   PCALL_METHOD > PCALL_BOUND_METHOD
*/

#define PCALL(POS) pcall[POS]++

PyObject *
PyEval_GetCallStats(PyObject *self)
{
    return Py_BuildValue("iiiiiiiiiii",
                         pcall[0], pcall[1], pcall[2], pcall[3],
                         pcall[4], pcall[5], pcall[6], pcall[7],
                         pcall[8], pcall[9], pcall[10]);
}
#else
#define PCALL(O)

PyObject *
PyEval_GetCallStats(PyObject *self)
{
    Py_INCREF(Py_None);
    return Py_None;
}
#endif


#ifdef WITH_THREAD
#define GIL_REQUEST _Py_atomic_load_relaxed(&gil_drop_request)
#else
#define GIL_REQUEST 0
#endif

/* This can set eval_breaker to 0 even though gil_drop_request became
   1.  We believe this is all right because the eval loop will release
   the GIL eventually anyway. */
#define COMPUTE_EVAL_BREAKER() \
    _Py_atomic_store_relaxed( \
        &eval_breaker, \
        GIL_REQUEST | \
        _Py_atomic_load_relaxed(&pendingcalls_to_do) | \
        pending_async_exc)

#ifdef WITH_THREAD

#define SET_GIL_DROP_REQUEST() \
    do { \
        _Py_atomic_store_relaxed(&gil_drop_request, 1); \
        _Py_atomic_store_relaxed(&eval_breaker, 1); \
    } while (0)

#define RESET_GIL_DROP_REQUEST() \
    do { \
        _Py_atomic_store_relaxed(&gil_drop_request, 0); \
        COMPUTE_EVAL_BREAKER(); \
    } while (0)

#endif

/* Pending calls are only modified under pending_lock */
#define SIGNAL_PENDING_CALLS() \
    do { \
        _Py_atomic_store_relaxed(&pendingcalls_to_do, 1); \
        _Py_atomic_store_relaxed(&eval_breaker, 1); \
    } while (0)

#define UNSIGNAL_PENDING_CALLS() \
    do { \
        _Py_atomic_store_relaxed(&pendingcalls_to_do, 0); \
        COMPUTE_EVAL_BREAKER(); \
    } while (0)

#define SIGNAL_ASYNC_EXC() \
    do { \
        pending_async_exc = 1; \
        _Py_atomic_store_relaxed(&eval_breaker, 1); \
    } while (0)

#define UNSIGNAL_ASYNC_EXC() \
    do { pending_async_exc = 0; COMPUTE_EVAL_BREAKER(); } while (0)


#ifdef WITH_THREAD

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "pythread.h"

static PyThread_type_lock pending_lock = 0; /* for pending calls */
static long main_thread = 0;
/* This single variable consolidates all requests to break out of the fast path
   in the eval loop. */
static _Py_atomic_int eval_breaker = {0};
/* Request for dropping the GIL */
static _Py_atomic_int gil_drop_request = {0};
/* Request for running pending calls. */
static _Py_atomic_int pendingcalls_to_do = {0};
/* Request for looking at the `async_exc` field of the current thread state.
   Guarded by the GIL. */
static int pending_async_exc = 0;

#include "ceval_gil.h"

int
PyEval_ThreadsInitialized(void)
{
    return gil_created();
}

void
PyEval_InitThreads(void)
{
    if (gil_created())
        return;
    create_gil();
    take_gil(PyThreadState_GET());
    main_thread = PyThread_get_thread_ident();
    if (!pending_lock)
        pending_lock = PyThread_allocate_lock();
}

void
_PyEval_FiniThreads(void)
{
    if (!gil_created())
        return;
    destroy_gil();
    assert(!gil_created());
}

void
PyEval_AcquireLock(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate == NULL)
        Py_FatalError("PyEval_AcquireLock: current thread state is NULL");
    take_gil(tstate);
}

void
PyEval_ReleaseLock(void)
{
    /* This function must succeed when the current thread state is NULL.
       We therefore avoid PyThreadState_GET() which dumps a fatal error
       in debug mode.
    */
    drop_gil((PyThreadState*)_Py_atomic_load_relaxed(
        &_PyThreadState_Current));
}

void
PyEval_AcquireThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_AcquireThread: NULL new thread state");
    /* Check someone has called PyEval_InitThreads() to create the lock */
    assert(gil_created());
    take_gil(tstate);
    if (PyThreadState_Swap(tstate) != NULL)
        Py_FatalError(
            "PyEval_AcquireThread: non-NULL old thread state");
}

void
PyEval_ReleaseThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_ReleaseThread: NULL thread state");
    if (PyThreadState_Swap(NULL) != tstate)
        Py_FatalError("PyEval_ReleaseThread: wrong thread state");
    drop_gil(tstate);
}

/* This function is called from PyOS_AfterFork to ensure that newly
   created child processes don't hold locks referring to threads which
   are not running in the child process.  (This could also be done using
   pthread_atfork mechanism, at least for the pthreads implementation.) */

void
PyEval_ReInitThreads(void)
{
    _Py_IDENTIFIER(_after_fork);
    PyObject *threading, *result;
    PyThreadState *tstate = PyThreadState_GET();

    if (!gil_created())
        return;
    recreate_gil();
    pending_lock = PyThread_allocate_lock();
    take_gil(tstate);
    main_thread = PyThread_get_thread_ident();

    /* Update the threading module with the new state.
     */
    tstate = PyThreadState_GET();
    threading = PyMapping_GetItemString(tstate->interp->modules,
                                        "threading");
    if (threading == NULL) {
        /* threading not imported */
        PyErr_Clear();
        return;
    }
    result = _PyObject_CallMethodId(threading, &PyId__after_fork, NULL);
    if (result == NULL)
        PyErr_WriteUnraisable(threading);
    else
        Py_DECREF(result);
    Py_DECREF(threading);
}

#else
static _Py_atomic_int eval_breaker = {0};
static int pending_async_exc = 0;
#endif /* WITH_THREAD */

/* This function is used to signal that async exceptions are waiting to be
   raised, therefore it is also useful in non-threaded builds. */

void
_PyEval_SignalAsyncExc(void)
{
    SIGNAL_ASYNC_EXC();
}

/* Functions save_thread and restore_thread are always defined so
   dynamically loaded modules needn't be compiled separately for use
   with and without threads: */

PyThreadState *
PyEval_SaveThread(void)
{
    PyThreadState *tstate = PyThreadState_Swap(NULL);
    if (tstate == NULL)
        Py_FatalError("PyEval_SaveThread: NULL tstate");
#ifdef WITH_THREAD
    if (gil_created())
        drop_gil(tstate);
#endif
    return tstate;
}

void
PyEval_RestoreThread(PyThreadState *tstate)
{
    if (tstate == NULL)
        Py_FatalError("PyEval_RestoreThread: NULL tstate");
#ifdef WITH_THREAD
    if (gil_created()) {
        int err = errno;
        take_gil(tstate);
        /* _Py_Finalizing is protected by the GIL */
        if (_Py_Finalizing && tstate != _Py_Finalizing) {
            drop_gil(tstate);
            PyThread_exit_thread();
            assert(0);  /* unreachable */
        }
        errno = err;
    }
#endif
    PyThreadState_Swap(tstate);
}


/* Mechanism whereby asynchronously executing callbacks (e.g. UNIX
   signal handlers or Mac I/O completion routines) can schedule calls
   to a function to be called synchronously.
   The synchronous function is called with one void* argument.
   It should return 0 for success or -1 for failure -- failure should
   be accompanied by an exception.

   If registry succeeds, the registry function returns 0; if it fails
   (e.g. due to too many pending calls) it returns -1 (without setting
   an exception condition).

   Note that because registry may occur from within signal handlers,
   or other asynchronous events, calling malloc() is unsafe!

#ifdef WITH_THREAD
   Any thread can schedule pending calls, but only the main thread
   will execute them.
   There is no facility to schedule calls to a particular thread, but
   that should be easy to change, should that ever be required.  In
   that case, the static variables here should go into the python
   threadstate.
#endif
*/

#ifdef WITH_THREAD

/* The WITH_THREAD implementation is thread-safe.  It allows
   scheduling to be made from any thread, and even from an executing
   callback.
 */

#define NPENDINGCALLS 32
static struct {
    int (*func)(void *);
    void *arg;
} pendingcalls[NPENDINGCALLS];
static int pendingfirst = 0;
static int pendinglast = 0;

int
Py_AddPendingCall(int (*func)(void *), void *arg)
{
    int i, j, result=0;
    PyThread_type_lock lock = pending_lock;

    /* try a few times for the lock.  Since this mechanism is used
     * for signal handling (on the main thread), there is a (slim)
     * chance that a signal is delivered on the same thread while we
     * hold the lock during the Py_MakePendingCalls() function.
     * This avoids a deadlock in that case.
     * Note that signals can be delivered on any thread.  In particular,
     * on Windows, a SIGINT is delivered on a system-created worker
     * thread.
     * We also check for lock being NULL, in the unlikely case that
     * this function is called before any bytecode evaluation takes place.
     */
    if (lock != NULL) {
        for (i = 0; i<100; i++) {
            if (PyThread_acquire_lock(lock, NOWAIT_LOCK))
                break;
        }
        if (i == 100)
            return -1;
    }

    i = pendinglast;
    j = (i + 1) % NPENDINGCALLS;
    if (j == pendingfirst) {
        result = -1; /* Queue full */
    } else {
        pendingcalls[i].func = func;
        pendingcalls[i].arg = arg;
        pendinglast = j;
    }
    /* signal main loop */
    SIGNAL_PENDING_CALLS();
    if (lock != NULL)
        PyThread_release_lock(lock);
    return result;
}

int
Py_MakePendingCalls(void)
{
    static int busy = 0;
    int i;
    int r = 0;

    if (!pending_lock) {
        /* initial allocation of the lock */
        pending_lock = PyThread_allocate_lock();
        if (pending_lock == NULL)
            return -1;
    }

    /* only service pending calls on main thread */
    if (main_thread && PyThread_get_thread_ident() != main_thread)
        return 0;
    /* don't perform recursive pending calls */
    if (busy)
        return 0;
    busy = 1;
    /* perform a bounded number of calls, in case of recursion */
    for (i=0; i<NPENDINGCALLS; i++) {
        int j;
        int (*func)(void *);
        void *arg = NULL;

        /* pop one item off the queue while holding the lock */
        PyThread_acquire_lock(pending_lock, WAIT_LOCK);
        j = pendingfirst;
        if (j == pendinglast) {
            func = NULL; /* Queue empty */
        } else {
            func = pendingcalls[j].func;
            arg = pendingcalls[j].arg;
            pendingfirst = (j + 1) % NPENDINGCALLS;
        }
        if (pendingfirst != pendinglast)
            SIGNAL_PENDING_CALLS();
        else
            UNSIGNAL_PENDING_CALLS();
        PyThread_release_lock(pending_lock);
        /* having released the lock, perform the callback */
        if (func == NULL)
            break;
        r = func(arg);
        if (r)
            break;
    }
    busy = 0;
    return r;
}

#else /* if ! defined WITH_THREAD */

/*
   WARNING!  ASYNCHRONOUSLY EXECUTING CODE!
   This code is used for signal handling in python that isn't built
   with WITH_THREAD.
   Don't use this implementation when Py_AddPendingCalls() can happen
   on a different thread!

   There are two possible race conditions:
   (1) nested asynchronous calls to Py_AddPendingCall()
   (2) AddPendingCall() calls made while pending calls are being processed.

   (1) is very unlikely because typically signal delivery
   is blocked during signal handling.  So it should be impossible.
   (2) is a real possibility.
   The current code is safe against (2), but not against (1).
   The safety against (2) is derived from the fact that only one
   thread is present, interrupted by signals, and that the critical
   section is protected with the "busy" variable.  On Windows, which
   delivers SIGINT on a system thread, this does not hold and therefore
   Windows really shouldn't use this version.
   The two threads could theoretically wiggle around the "busy" variable.
*/

#define NPENDINGCALLS 32
static struct {
    int (*func)(void *);
    void *arg;
} pendingcalls[NPENDINGCALLS];
static volatile int pendingfirst = 0;
static volatile int pendinglast = 0;
static _Py_atomic_int pendingcalls_to_do = {0};

int
Py_AddPendingCall(int (*func)(void *), void *arg)
{
    static volatile int busy = 0;
    int i, j;
    /* XXX Begin critical section */
    if (busy)
        return -1;
    busy = 1;
    i = pendinglast;
    j = (i + 1) % NPENDINGCALLS;
    if (j == pendingfirst) {
        busy = 0;
        return -1; /* Queue full */
    }
    pendingcalls[i].func = func;
    pendingcalls[i].arg = arg;
    pendinglast = j;

    SIGNAL_PENDING_CALLS();
    busy = 0;
    /* XXX End critical section */
    return 0;
}

int
Py_MakePendingCalls(void)
{
    static int busy = 0;
    if (busy)
        return 0;
    busy = 1;
    UNSIGNAL_PENDING_CALLS();
    for (;;) {
        int i;
        int (*func)(void *);
        void *arg;
        i = pendingfirst;
        if (i == pendinglast)
            break; /* Queue empty */
        func = pendingcalls[i].func;
        arg = pendingcalls[i].arg;
        pendingfirst = (i + 1) % NPENDINGCALLS;
        if (func(arg) < 0) {
            busy = 0;
            SIGNAL_PENDING_CALLS(); /* We're not done yet */
            return -1;
        }
    }
    busy = 0;
    return 0;
}

#endif /* WITH_THREAD */


/* The interpreter's recursion limit */

#ifndef Py_DEFAULT_RECURSION_LIMIT
#define Py_DEFAULT_RECURSION_LIMIT 1000
#endif
static int recursion_limit = Py_DEFAULT_RECURSION_LIMIT;
int _Py_CheckRecursionLimit = Py_DEFAULT_RECURSION_LIMIT;

int
Py_GetRecursionLimit(void)
{
    return recursion_limit;
}

void
Py_SetRecursionLimit(int new_limit)
{
    recursion_limit = new_limit;
    _Py_CheckRecursionLimit = recursion_limit;
}

/* the macro Py_EnterRecursiveCall() only calls _Py_CheckRecursiveCall()
   if the recursion_depth reaches _Py_CheckRecursionLimit.
   If USE_STACKCHECK, the macro decrements _Py_CheckRecursionLimit
   to guarantee that _Py_CheckRecursiveCall() is regularly called.
   Without USE_STACKCHECK, there is no need for this. */
int
_Py_CheckRecursiveCall(char *where)
{
    PyThreadState *tstate = PyThreadState_GET();

#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        --tstate->recursion_depth;
        PyErr_SetString(PyExc_MemoryError, "Stack overflow");
        return -1;
    }
#endif
    _Py_CheckRecursionLimit = recursion_limit;
    if (tstate->recursion_critical)
        /* Somebody asked that we don't check for recursion. */
        return 0;
    if (tstate->overflowed) {
        if (tstate->recursion_depth > recursion_limit + 50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from stack overflow.");
        }
        return 0;
    }
    if (tstate->recursion_depth > recursion_limit) {
        --tstate->recursion_depth;
        tstate->overflowed = 1;
        PyErr_Format(PyExc_RuntimeError,
                     "maximum recursion depth exceeded%s",
                     where);
        return -1;
    }
    return 0;
}

/* Status code for main loop (reason for stack unwind) */
enum why_code {
        WHY_NOT =       0x0001, /* No error */
        WHY_EXCEPTION = 0x0002, /* Exception occurred */
        WHY_RERAISE =   0x0004, /* Exception re-raised by 'finally' */
        WHY_RETURN =    0x0008, /* 'return' statement */
        WHY_BREAK =     0x0010, /* 'break' statement */
        WHY_CONTINUE =  0x0020, /* 'continue' statement */
        WHY_YIELD =     0x0040, /* 'yield' operator */
        WHY_SILENCED =  0x0080  /* Exception silenced by 'with' */
};

static void save_exc_state(PyThreadState *, PyFrameObject *);
static void swap_exc_state(PyThreadState *, PyFrameObject *);
static void restore_and_clear_exc_state(PyThreadState *, PyFrameObject *);
static enum why_code do_raise(PyObject *, PyObject *);
static int unpack_iterable(PyObject *, int, int, PyObject **);

/* Records whether tracing is on for any thread.  Counts the number of
   threads for which tstate->c_tracefunc is non-NULL, so if the value
   is 0, we know we don't have to check this thread's c_tracefunc.
   This speeds up the if statement in PyEval_EvalFrameEx() after
   fast_next_opcode*/
static int _Py_TracingPossible = 0;



PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    return PyEval_EvalCodeEx(co,
                      globals, locals,
                      (PyObject **)NULL, 0,
                      (PyObject **)NULL, 0,
                      (PyObject **)NULL, 0,
                      NULL, NULL);
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f) {
    /* This is for backward compatibility with extension modules that
       used this API; core interpreter code should call
       PyEval_EvalFrameEx() */
    return PyEval_EvalFrameEx(f, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
#ifdef DXPAIRS
    int lastopcode = 0;
#endif
    register PyObject **stack_pointer;  /* Next free slot in value stack */
    register unsigned char *next_instr;
    register int opcode;        /* Current opcode */
    register int oparg;         /* Current opcode argument, if any */
    register enum why_code why; /* Reason for block stack unwind */
    register int err;           /* Error status -- nonzero if error */
    register PyObject *x;       /* Result object -- NULL if error */
    register PyObject *v;       /* Temporary objects popped off stack */
    register PyObject *w;
    register PyObject *u;
    register PyObject *t;
    register PyObject **fastlocals, **freevars;
    PyObject *retval = NULL;            /* Return value */
    PyThreadState *tstate = PyThreadState_GET();
    PyCodeObject *co;

    /* when tracing we set things up so that

           not (instr_lb <= current_bytecode_offset < instr_ub)

       is true when the line being executed has changed.  The
       initial values are such as to make this false the first
       time it is tested. */
    int instr_ub = -1, instr_lb = 0, instr_prev = -1;

    unsigned char *first_instr;
    PyObject *names;
    PyObject *consts;

#ifdef LLTRACE
    _Py_IDENTIFIER(__ltrace__);
#endif

/* Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   We disable the optimization if DYNAMIC_EXECUTION_PROFILE is defined,
   because it would render the measurements invalid.


   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/

#ifdef DYNAMIC_EXECUTION_PROFILE
#undef USE_COMPUTED_GOTOS
#define USE_COMPUTED_GOTOS 0
#endif

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#else
    #if defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
    #endif
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#if USE_COMPUTED_GOTOS
/* Import the static jump table */
#include "opcode_targets.h"

/* This macro is used when several opcodes defer to the same implementation
   (e.g. SETUP_LOOP, SETUP_FINALLY) */
#define TARGET_WITH_IMPL(op, impl) \
    TARGET_##op: \
        opcode = op; \
        if (HAS_ARG(op)) \
            oparg = NEXTARG(); \
    case op: \
        goto impl; \

#define TARGET(op) \
    TARGET_##op: \
        opcode = op; \
        if (HAS_ARG(op)) \
            oparg = NEXTARG(); \
    case op:


#define DISPATCH() \
    { \
        if (!_Py_atomic_load_relaxed(&eval_breaker)) {      \
                    FAST_DISPATCH(); \
        } \
        continue; \
    }

#ifdef LLTRACE
#define FAST_DISPATCH() \
    { \
        if (!lltrace && !_Py_TracingPossible) { \
            f->f_lasti = INSTR_OFFSET(); \
            goto *opcode_targets[*next_instr++]; \
        } \
        goto fast_next_opcode; \
    }
#else
#define FAST_DISPATCH() \
    { \
        if (!_Py_TracingPossible) { \
            f->f_lasti = INSTR_OFFSET(); \
            goto *opcode_targets[*next_instr++]; \
        } \
        goto fast_next_opcode; \
    }
#endif

#else
#define TARGET(op) \
    case op:
#define TARGET_WITH_IMPL(op, impl) \
    /* silence compiler warnings about `impl` unused */ \
    if (0) goto impl; \
    case op:
#define DISPATCH() continue
#define FAST_DISPATCH() goto fast_next_opcode
#endif


/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((PyTupleObject *)(v), (i))
#else
#define GETITEM(v, i) PyTuple_GetItem((v), (i))
#endif

#ifdef WITH_TSC
/* Use Pentium timestamp counter to mark certain events:
   inst0 -- beginning of switch statement for opcode dispatch
   inst1 -- end of switch statement (may be skipped)
   loop0 -- the top of the mainloop
   loop1 -- place where control returns again to top of mainloop
            (may be skipped)
   intr1 -- beginning of long interruption
   intr2 -- end of long interruption

   Many opcodes call out to helper C functions.  In some cases, the
   time in those functions should be counted towards the time for the
   opcode, but not in all cases.  For example, a CALL_FUNCTION opcode
   calls another Python function; there's no point in charge all the
   bytecode executed by the called function to the caller.

   It's hard to make a useful judgement statically.  In the presence
   of operator overloading, it's impossible to tell if a call will
   execute new Python code or not.

   It's a case-by-case judgement.  I'll use intr1 for the following
   cases:

   IMPORT_STAR
   IMPORT_FROM
   CALL_FUNCTION (and friends)

 */
    uint64 inst0, inst1, loop0, loop1, intr0 = 0, intr1 = 0;
    int ticked = 0;

    READ_TIMESTAMP(inst0);
    READ_TIMESTAMP(inst1);
    READ_TIMESTAMP(loop0);
    READ_TIMESTAMP(loop1);

    /* shut up the compiler */
    opcode = 0;
#endif

/* Code access macros */

#define INSTR_OFFSET()  ((int)(next_instr - first_instr))
#define NEXTOP()        (*next_instr++)
#define NEXTARG()       (next_instr += 2, (next_instr[-1]<<8) + next_instr[-2])
#define PEEKARG()       ((next_instr[2]<<8) + next_instr[1])
#define JUMPTO(x)       (next_instr = first_instr + (x))
#define JUMPBY(x)       (next_instr += (x))

/* OpCode prediction macros
    Some opcodes tend to come in pairs thus making it possible to
    predict the second code when the first is run.  For example,
    COMPARE_OP is often followed by JUMP_IF_FALSE or JUMP_IF_TRUE.  And,
    those opcodes are often followed by a POP_TOP.

    Verifying the prediction costs a single high-speed test of a register
    variable against a constant.  If the pairing was good, then the
    processor's own internal branch predication has a high likelihood of
    success, resulting in a nearly zero-overhead transition to the
    next opcode.  A successful prediction saves a trip through the eval-loop
    including its two unpredictable branches, the HAS_ARG test and the
    switch-case.  Combined with the processor's internal branch prediction,
    a successful PREDICT has the effect of making the two opcodes run as if
    they were a single new opcode with the bodies combined.

    If collecting opcode statistics, your choices are to either keep the
    predictions turned-on and interpret the results as if some opcodes
    had been combined or turn-off predictions so that the opcode frequency
    counter updates for both opcodes.

    Opcode prediction is disabled with threaded code, since the latter allows
    the CPU to record separate branch prediction information for each
    opcode.

*/

#if defined(DYNAMIC_EXECUTION_PROFILE) || USE_COMPUTED_GOTOS
#define PREDICT(op)             if (0) goto PRED_##op
#define PREDICTED(op)           PRED_##op:
#define PREDICTED_WITH_ARG(op)  PRED_##op:
#else
#define PREDICT(op)             if (*next_instr == op) goto PRED_##op
#define PREDICTED(op)           PRED_##op: next_instr++
#define PREDICTED_WITH_ARG(op)  PRED_##op: oparg = PEEKARG(); next_instr += 3
#endif


/* Stack manipulation macros */

/* The stack can grow at most MAXINT deep, as co_nlocals and
   co_stacksize are ints. */
#define STACK_LEVEL()     ((int)(stack_pointer - f->f_valuestack))
#define EMPTY()           (STACK_LEVEL() == 0)
#define TOP()             (stack_pointer[-1])
#define SECOND()          (stack_pointer[-2])
#define THIRD()           (stack_pointer[-3])
#define FOURTH()          (stack_pointer[-4])
#define PEEK(n)           (stack_pointer[-(n)])
#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define SET_SECOND(v)     (stack_pointer[-2] = (v))
#define SET_THIRD(v)      (stack_pointer[-3] = (v))
#define SET_FOURTH(v)     (stack_pointer[-4] = (v))
#define SET_VALUE(n, v)   (stack_pointer[-(n)] = (v))
#define BASIC_STACKADJ(n) (stack_pointer += n)
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)

#ifdef LLTRACE
#define PUSH(v)         { (void)(BASIC_PUSH(v), \
                          lltrace && prtrace(TOP(), "push")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); }
#define POP()           ((void)(lltrace && prtrace(TOP(), "pop")), \
                         BASIC_POP())
#define STACKADJ(n)     { (void)(BASIC_STACKADJ(n), \
                          lltrace && prtrace(TOP(), "stackadj")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); }
#define EXT_POP(STACK_POINTER) ((void)(lltrace && \
                                prtrace((STACK_POINTER)[-1], "ext_pop")), \
                                *--(STACK_POINTER))
#else
#define PUSH(v)                BASIC_PUSH(v)
#define POP()                  BASIC_POP()
#define STACKADJ(n)            BASIC_STACKADJ(n)
#define EXT_POP(STACK_POINTER) (*--(STACK_POINTER))
#endif

/* Local variable macros */

#define GETLOCAL(i)     (fastlocals[i])

/* The SETLOCAL() macro must not DECREF the local variable in-place and
   then store the new value; it must copy the old value to a temporary
   value, then store the new value, and then DECREF the temporary value.
   This is because it is possible that during the DECREF the frame is
   accessed by other code (e.g. a __del__ method or gc.collect()) and the
   variable would be pointing to already-freed memory. */
#define SETLOCAL(i, value)      do { PyObject *tmp = GETLOCAL(i); \
                                     GETLOCAL(i) = value; \
                                     Py_XDECREF(tmp); } while (0)


#define UNWIND_BLOCK(b) \
    while (STACK_LEVEL() > (b)->b_level) { \
        PyObject *v = POP(); \
        Py_XDECREF(v); \
    }

#define UNWIND_EXCEPT_HANDLER(b) \
    { \
        PyObject *type, *value, *traceback; \
        assert(STACK_LEVEL() >= (b)->b_level + 3); \
        while (STACK_LEVEL() > (b)->b_level + 3) { \
            value = POP(); \
            Py_XDECREF(value); \
        } \
        type = tstate->exc_type; \
        value = tstate->exc_value; \
        traceback = tstate->exc_traceback; \
        tstate->exc_type = POP(); \
        tstate->exc_value = POP(); \
        tstate->exc_traceback = POP(); \
        Py_XDECREF(type); \
        Py_XDECREF(value); \
        Py_XDECREF(traceback); \
    }

/* Start of code */

    /* push frame */
    if (Py_EnterRecursiveCall(""))
        return NULL;

    tstate->frame = f;

    if (tstate->use_tracing) {
        if (tstate->c_tracefunc != NULL) {
            /* tstate->c_tracefunc, if defined, is a
               function that will be called on *every* entry
               to a code block.  Its return value, if not
               None, is a function that will be called at
               the start of each executed line of code.
               (Actually, the function must return itself
               in order to continue tracing.)  The trace
               functions are called with three arguments:
               a pointer to the current frame, a string
               indicating why the function is called, and
               an argument which depends on the situation.
               The global trace function is also called
               whenever an exception is detected. */
            if (call_trace_protected(tstate->c_tracefunc,
                                     tstate->c_traceobj,
                                     f, PyTrace_CALL, Py_None)) {
                /* Trace function raised an error */
                goto exit_eval_frame;
            }
        }
        if (tstate->c_profilefunc != NULL) {
            /* Similar for c_profilefunc, except it needn't
               return itself and isn't called for "line" events */
            if (call_trace_protected(tstate->c_profilefunc,
                                     tstate->c_profileobj,
                                     f, PyTrace_CALL, Py_None)) {
                /* Profile function raised an error */
                goto exit_eval_frame;
            }
        }
    }

    co = f->f_code;
    names = co->co_names;
    consts = co->co_consts;
    fastlocals = f->f_localsplus;
    freevars = f->f_localsplus + co->co_nlocals;
    first_instr = (unsigned char*) PyBytes_AS_STRING(co->co_code);
    /* An explanation is in order for the next line.

       f->f_lasti now refers to the index of the last instruction
       executed.  You might think this was obvious from the name, but
       this wasn't always true before 2.3!  PyFrame_New now sets
       f->f_lasti to -1 (i.e. the index *before* the first instruction)
       and YIELD_VALUE doesn't fiddle with f_lasti any more.  So this
       does work.  Promise.
       YIELD_FROM sets f_lasti to itself, in order to repeated yield
       multiple values.

       When the PREDICT() macros are enabled, some opcode pairs follow in
       direct succession without updating f->f_lasti.  A successful
       prediction effectively links the two codes together as if they
       were a single new opcode; accordingly,f->f_lasti will point to
       the first code in the pair (for instance, GET_ITER followed by
       FOR_ITER is effectively a single opcode and f->f_lasti will point
       at to the beginning of the combined pair.)
    */
    next_instr = first_instr + f->f_lasti + 1;
    stack_pointer = f->f_stacktop;
    assert(stack_pointer != NULL);
    f->f_stacktop = NULL;       /* remains NULL unless yield suspends frame */

    if (co->co_flags & CO_GENERATOR && !throwflag) {
        if (f->f_exc_type != NULL && f->f_exc_type != Py_None) {
            /* We were in an except handler when we left,
               restore the exception state which was put aside
               (see YIELD_VALUE). */
            swap_exc_state(tstate, f);
        }
        else
            save_exc_state(tstate, f);
    }

#ifdef LLTRACE
    lltrace = _PyDict_GetItemId(f->f_globals, &PyId___ltrace__) != NULL;
#endif

    why = WHY_NOT;
    err = 0;
    x = Py_None;        /* Not a reference, just anything non-NULL */
    w = NULL;

    if (throwflag) { /* support for generator.throw() */
        why = WHY_EXCEPTION;
        goto on_error;
    }

    for (;;) {
#ifdef WITH_TSC
        if (inst1 == 0) {
            /* Almost surely, the opcode executed a break
               or a continue, preventing inst1 from being set
               on the way out of the loop.
            */
            READ_TIMESTAMP(inst1);
            loop1 = inst1;
        }
        dump_tsc(opcode, ticked, inst0, inst1, loop0, loop1,
                 intr0, intr1);
        ticked = 0;
        inst1 = 0;
        intr0 = 0;
        intr1 = 0;
        READ_TIMESTAMP(loop0);
#endif
        assert(stack_pointer >= f->f_valuestack); /* else underflow */
        assert(STACK_LEVEL() <= co->co_stacksize);  /* else overflow */

        /* Do periodic things.  Doing this every time through
           the loop would add too much overhead, so we do it
           only every Nth instruction.  We also do it if
           ``pendingcalls_to_do'' is set, i.e. when an asynchronous
           event needs attention (e.g. a signal handler or
           async I/O handler); see Py_AddPendingCall() and
           Py_MakePendingCalls() above. */

        if (_Py_atomic_load_relaxed(&eval_breaker)) {
            if (*next_instr == SETUP_FINALLY) {
                /* Make the last opcode before
                   a try: finally: block uninterruptible. */
                goto fast_next_opcode;
            }
            tstate->tick_counter++;
#ifdef WITH_TSC
            ticked = 1;
#endif
            if (_Py_atomic_load_relaxed(&pendingcalls_to_do)) {
                if (Py_MakePendingCalls() < 0) {
                    why = WHY_EXCEPTION;
                    goto on_error;
                }
            }
#ifdef WITH_THREAD
            if (_Py_atomic_load_relaxed(&gil_drop_request)) {
                /* Give another thread a chance */
                if (PyThreadState_Swap(NULL) != tstate)
                    Py_FatalError("ceval: tstate mix-up");
                drop_gil(tstate);

                /* Other threads may run now */

                take_gil(tstate);
                if (PyThreadState_Swap(tstate) != NULL)
                    Py_FatalError("ceval: orphan tstate");
            }
#endif
            /* Check for asynchronous exceptions. */
            if (tstate->async_exc != NULL) {
                x = tstate->async_exc;
                tstate->async_exc = NULL;
                UNSIGNAL_ASYNC_EXC();
                PyErr_SetNone(x);
                Py_DECREF(x);
                why = WHY_EXCEPTION;
                goto on_error;
            }
        }

    fast_next_opcode:
        f->f_lasti = INSTR_OFFSET();

        /* line-by-line tracing support */

        if (_Py_TracingPossible &&
            tstate->c_tracefunc != NULL && !tstate->tracing) {
            /* see maybe_call_line_trace
               for expository comments */
            f->f_stacktop = stack_pointer;

            err = maybe_call_line_trace(tstate->c_tracefunc,
                                        tstate->c_traceobj,
                                        f, &instr_lb, &instr_ub,
                                        &instr_prev);
            /* Reload possibly changed frame fields */
            JUMPTO(f->f_lasti);
            if (f->f_stacktop != NULL) {
                stack_pointer = f->f_stacktop;
                f->f_stacktop = NULL;
            }
            if (err) {
                /* trace function raised an exception */
                goto on_error;
            }
        }

        /* Extract opcode and argument */

        opcode = NEXTOP();
        oparg = 0;   /* allows oparg to be stored in a register because
            it doesn't have to be remembered across a full loop */
        if (HAS_ARG(opcode))
            oparg = NEXTARG();
    dispatch_opcode:
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
        dxpairs[lastopcode][opcode]++;
        lastopcode = opcode;
#endif
        dxp[opcode]++;
#endif

#ifdef LLTRACE
        /* Instruction tracing */

        if (lltrace) {
            if (HAS_ARG(opcode)) {
                printf("%d: %d, %d\n",
                       f->f_lasti, opcode, oparg);
            }
            else {
                printf("%d: %d\n",
                       f->f_lasti, opcode);
            }
        }
#endif

        /* Main switch on opcode */
        READ_TIMESTAMP(inst0);

        switch (opcode) {

        /* BEWARE!
           It is essential that any operation that fails sets either
           x to NULL, err to nonzero, or why to anything but WHY_NOT,
           and that no operation that succeeds does this! */

        TARGET(NOP)
            FAST_DISPATCH();

        TARGET(LOAD_FAST)
            x = GETLOCAL(oparg);
            if (x != NULL) {
                Py_INCREF(x);
                PUSH(x);
                FAST_DISPATCH();
            }
            format_exc_check_arg(PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(co->co_varnames, oparg));
            break;

        TARGET(LOAD_CONST)
            x = GETITEM(consts, oparg);
            Py_INCREF(x);
            PUSH(x);
            FAST_DISPATCH();

        PREDICTED_WITH_ARG(STORE_FAST);
        TARGET(STORE_FAST)
            v = POP();
            SETLOCAL(oparg, v);
            FAST_DISPATCH();

        TARGET(POP_TOP)
            v = POP();
            Py_DECREF(v);
            FAST_DISPATCH();

        TARGET(ROT_TWO)
            v = TOP();
            w = SECOND();
            SET_TOP(w);
            SET_SECOND(v);
            FAST_DISPATCH();

        TARGET(ROT_THREE)
            v = TOP();
            w = SECOND();
            x = THIRD();
            SET_TOP(w);
            SET_SECOND(x);
            SET_THIRD(v);
            FAST_DISPATCH();

        TARGET(DUP_TOP)
            v = TOP();
            Py_INCREF(v);
            PUSH(v);
            FAST_DISPATCH();

        TARGET(DUP_TOP_TWO)
            x = TOP();
            Py_INCREF(x);
            w = SECOND();
            Py_INCREF(w);
            STACKADJ(2);
            SET_TOP(x);
            SET_SECOND(w);
            FAST_DISPATCH();

        TARGET(UNARY_POSITIVE)
            v = TOP();
            x = PyNumber_Positive(v);
            Py_DECREF(v);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(UNARY_NEGATIVE)
            v = TOP();
            x = PyNumber_Negative(v);
            Py_DECREF(v);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(UNARY_NOT)
            v = TOP();
            err = PyObject_IsTrue(v);
            Py_DECREF(v);
            if (err == 0) {
                Py_INCREF(Py_True);
                SET_TOP(Py_True);
                DISPATCH();
            }
            else if (err > 0) {
                Py_INCREF(Py_False);
                SET_TOP(Py_False);
                err = 0;
                DISPATCH();
            }
            STACKADJ(-1);
            break;

        TARGET(UNARY_INVERT)
            v = TOP();
            x = PyNumber_Invert(v);
            Py_DECREF(v);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_POWER)
            w = POP();
            v = TOP();
            x = PyNumber_Power(v, w, Py_None);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_MULTIPLY)
            w = POP();
            v = TOP();
            x = PyNumber_Multiply(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_TRUE_DIVIDE)
            w = POP();
            v = TOP();
            x = PyNumber_TrueDivide(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_FLOOR_DIVIDE)
            w = POP();
            v = TOP();
            x = PyNumber_FloorDivide(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_MODULO)
            w = POP();
            v = TOP();
            if (PyUnicode_CheckExact(v))
                x = PyUnicode_Format(v, w);
            else
                x = PyNumber_Remainder(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_ADD)
            w = POP();
            v = TOP();
            if (PyUnicode_CheckExact(v) &&
                     PyUnicode_CheckExact(w)) {
                x = unicode_concatenate(v, w, f, next_instr);
                /* unicode_concatenate consumed the ref to v */
                goto skip_decref_vx;
            }
            else {
                x = PyNumber_Add(v, w);
            }
            Py_DECREF(v);
          skip_decref_vx:
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_SUBTRACT)
            w = POP();
            v = TOP();
            x = PyNumber_Subtract(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_SUBSCR)
            w = POP();
            v = TOP();
            x = PyObject_GetItem(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_LSHIFT)
            w = POP();
            v = TOP();
            x = PyNumber_Lshift(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_RSHIFT)
            w = POP();
            v = TOP();
            x = PyNumber_Rshift(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_AND)
            w = POP();
            v = TOP();
            x = PyNumber_And(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_XOR)
            w = POP();
            v = TOP();
            x = PyNumber_Xor(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(BINARY_OR)
            w = POP();
            v = TOP();
            x = PyNumber_Or(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(LIST_APPEND)
            w = POP();
            v = PEEK(oparg);
            err = PyList_Append(v, w);
            Py_DECREF(w);
            if (err == 0) {
                PREDICT(JUMP_ABSOLUTE);
                DISPATCH();
            }
            break;

        TARGET(SET_ADD)
            w = POP();
            v = stack_pointer[-oparg];
            err = PySet_Add(v, w);
            Py_DECREF(w);
            if (err == 0) {
                PREDICT(JUMP_ABSOLUTE);
                DISPATCH();
            }
            break;

        TARGET(INPLACE_POWER)
            w = POP();
            v = TOP();
            x = PyNumber_InPlacePower(v, w, Py_None);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_MULTIPLY)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceMultiply(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_TRUE_DIVIDE)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceTrueDivide(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_FLOOR_DIVIDE)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceFloorDivide(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_MODULO)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceRemainder(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_ADD)
            w = POP();
            v = TOP();
            if (PyUnicode_CheckExact(v) &&
                     PyUnicode_CheckExact(w)) {
                x = unicode_concatenate(v, w, f, next_instr);
                /* unicode_concatenate consumed the ref to v */
                goto skip_decref_v;
            }
            else {
                x = PyNumber_InPlaceAdd(v, w);
            }
            Py_DECREF(v);
          skip_decref_v:
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_SUBTRACT)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceSubtract(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_LSHIFT)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceLshift(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_RSHIFT)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceRshift(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_AND)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceAnd(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_XOR)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceXor(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(INPLACE_OR)
            w = POP();
            v = TOP();
            x = PyNumber_InPlaceOr(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(STORE_SUBSCR)
            w = TOP();
            v = SECOND();
            u = THIRD();
            STACKADJ(-3);
            /* v[w] = u */
            err = PyObject_SetItem(v, w, u);
            Py_DECREF(u);
            Py_DECREF(v);
            Py_DECREF(w);
            if (err == 0) DISPATCH();
            break;

        TARGET(DELETE_SUBSCR)
            w = TOP();
            v = SECOND();
            STACKADJ(-2);
            /* del v[w] */
            err = PyObject_DelItem(v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            if (err == 0) DISPATCH();
            break;

        TARGET(PRINT_EXPR)
            v = POP();
            w = PySys_GetObject("displayhook");
            if (w == NULL) {
                PyErr_SetString(PyExc_RuntimeError,
                                "lost sys.displayhook");
                err = -1;
                x = NULL;
            }
            if (err == 0) {
                x = PyTuple_Pack(1, v);
                if (x == NULL)
                    err = -1;
            }
            if (err == 0) {
                w = PyEval_CallObject(w, x);
                Py_XDECREF(w);
                if (w == NULL)
                    err = -1;
            }
            Py_DECREF(v);
            Py_XDECREF(x);
            break;

#ifdef CASE_TOO_BIG
        default: switch (opcode) {
#endif
        TARGET(RAISE_VARARGS)
            v = w = NULL;
            switch (oparg) {
            case 2:
                v = POP(); /* cause */
            case 1:
                w = POP(); /* exc */
            case 0: /* Fallthrough */
                why = do_raise(w, v);
                break;
            default:
                PyErr_SetString(PyExc_SystemError,
                           "bad RAISE_VARARGS oparg");
                why = WHY_EXCEPTION;
                break;
            }
            break;

        TARGET(STORE_LOCALS)
            x = POP();
            v = f->f_locals;
            Py_XDECREF(v);
            f->f_locals = x;
            DISPATCH();

        TARGET(RETURN_VALUE)
            retval = POP();
            why = WHY_RETURN;
            goto fast_block_end;

        TARGET(YIELD_FROM)
            u = POP();
            x = TOP();
            /* send u to x */
            if (PyGen_CheckExact(x)) {
                retval = _PyGen_Send((PyGenObject *)x, u);
            } else {
                _Py_IDENTIFIER(send);
                if (u == Py_None)
                    retval = Py_TYPE(x)->tp_iternext(x);
                else
                    retval = _PyObject_CallMethodId(x, &PyId_send, "O", u);
            }
            Py_DECREF(u);
            if (!retval) {
                PyObject *val;
                x = POP(); /* Remove iter from stack */
                Py_DECREF(x);
                err = _PyGen_FetchStopIterationValue(&val);
                if (err < 0) {
                    x = NULL;
                    break;
                }
                x = val;
                PUSH(x);
                continue;
            }
            /* x remains on stack, retval is value to be yielded */
            f->f_stacktop = stack_pointer;
            why = WHY_YIELD;
            /* and repeat... */
            f->f_lasti--;
            goto fast_yield;

        TARGET(YIELD_VALUE)
            retval = POP();
            f->f_stacktop = stack_pointer;
            why = WHY_YIELD;
            goto fast_yield;

        TARGET(POP_EXCEPT)
            {
                PyTryBlock *b = PyFrame_BlockPop(f);
                if (b->b_type != EXCEPT_HANDLER) {
                    PyErr_SetString(PyExc_SystemError,
                        "popped block is not an except handler");
                    why = WHY_EXCEPTION;
                    break;
                }
                UNWIND_EXCEPT_HANDLER(b);
            }
            DISPATCH();

        TARGET(POP_BLOCK)
            {
                PyTryBlock *b = PyFrame_BlockPop(f);
                UNWIND_BLOCK(b);
            }
            DISPATCH();

        PREDICTED(END_FINALLY);
        TARGET(END_FINALLY)
            v = POP();
            if (PyLong_Check(v)) {
                why = (enum why_code) PyLong_AS_LONG(v);
                assert(why != WHY_YIELD);
                if (why == WHY_RETURN ||
                    why == WHY_CONTINUE)
                    retval = POP();
                if (why == WHY_SILENCED) {
                    /* An exception was silenced by 'with', we must
                    manually unwind the EXCEPT_HANDLER block which was
                    created when the exception was caught, otherwise
                    the stack will be in an inconsistent state. */
                    PyTryBlock *b = PyFrame_BlockPop(f);
                    assert(b->b_type == EXCEPT_HANDLER);
                    UNWIND_EXCEPT_HANDLER(b);
                    why = WHY_NOT;
                }
            }
            else if (PyExceptionClass_Check(v)) {
                w = POP();
                u = POP();
                PyErr_Restore(v, w, u);
                why = WHY_RERAISE;
                break;
            }
            else if (v != Py_None) {
                PyErr_SetString(PyExc_SystemError,
                    "'finally' pops bad exception");
                why = WHY_EXCEPTION;
            }
            Py_DECREF(v);
            break;

        TARGET(LOAD_BUILD_CLASS)
        {
            _Py_IDENTIFIER(__build_class__);

            if (PyDict_CheckExact(f->f_builtins)) {
                x = _PyDict_GetItemId(f->f_builtins, &PyId___build_class__);
                if (x == NULL) {
                    PyErr_SetString(PyExc_NameError,
                                    "__build_class__ not found");
                    break;
                }
                Py_INCREF(x);
            }
            else {
                PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
                if (build_class_str == NULL)
                    break;
                x = PyObject_GetItem(f->f_builtins, build_class_str);
                if (x == NULL) {
                    if (PyErr_ExceptionMatches(PyExc_KeyError))
                        PyErr_SetString(PyExc_NameError,
                                        "__build_class__ not found");
                    break;
                }
            }
            PUSH(x);
            break;
        }

        TARGET(STORE_NAME)
            w = GETITEM(names, oparg);
            v = POP();
            if ((x = f->f_locals) != NULL) {
                if (PyDict_CheckExact(x))
                    err = PyDict_SetItem(x, w, v);
                else
                    err = PyObject_SetItem(x, w, v);
                Py_DECREF(v);
                if (err == 0) DISPATCH();
                break;
            }
            PyErr_Format(PyExc_SystemError,
                         "no locals found when storing %R", w);
            break;

        TARGET(DELETE_NAME)
            w = GETITEM(names, oparg);
            if ((x = f->f_locals) != NULL) {
                if ((err = PyObject_DelItem(x, w)) != 0)
                    format_exc_check_arg(PyExc_NameError,
                                         NAME_ERROR_MSG,
                                         w);
                break;
            }
            PyErr_Format(PyExc_SystemError,
                         "no locals when deleting %R", w);
            break;

        PREDICTED_WITH_ARG(UNPACK_SEQUENCE);
        TARGET(UNPACK_SEQUENCE)
            v = POP();
            if (PyTuple_CheckExact(v) &&
                PyTuple_GET_SIZE(v) == oparg) {
                PyObject **items = \
                    ((PyTupleObject *)v)->ob_item;
                while (oparg--) {
                    w = items[oparg];
                    Py_INCREF(w);
                    PUSH(w);
                }
                Py_DECREF(v);
                DISPATCH();
            } else if (PyList_CheckExact(v) &&
                       PyList_GET_SIZE(v) == oparg) {
                PyObject **items = \
                    ((PyListObject *)v)->ob_item;
                while (oparg--) {
                    w = items[oparg];
                    Py_INCREF(w);
                    PUSH(w);
                }
            } else if (unpack_iterable(v, oparg, -1,
                                       stack_pointer + oparg)) {
                STACKADJ(oparg);
            } else {
                /* unpack_iterable() raised an exception */
                why = WHY_EXCEPTION;
            }
            Py_DECREF(v);
            break;

        TARGET(UNPACK_EX)
        {
            int totalargs = 1 + (oparg & 0xFF) + (oparg >> 8);
            v = POP();

            if (unpack_iterable(v, oparg & 0xFF, oparg >> 8,
                                stack_pointer + totalargs)) {
                stack_pointer += totalargs;
            } else {
                why = WHY_EXCEPTION;
            }
            Py_DECREF(v);
            break;
        }

        TARGET(STORE_ATTR)
            w = GETITEM(names, oparg);
            v = TOP();
            u = SECOND();
            STACKADJ(-2);
            err = PyObject_SetAttr(v, w, u); /* v.w = u */
            Py_DECREF(v);
            Py_DECREF(u);
            if (err == 0) DISPATCH();
            break;

        TARGET(DELETE_ATTR)
            w = GETITEM(names, oparg);
            v = POP();
            err = PyObject_SetAttr(v, w, (PyObject *)NULL);
                                            /* del v.w */
            Py_DECREF(v);
            break;

        TARGET(STORE_GLOBAL)
            w = GETITEM(names, oparg);
            v = POP();
            err = PyDict_SetItem(f->f_globals, w, v);
            Py_DECREF(v);
            if (err == 0) DISPATCH();
            break;

        TARGET(DELETE_GLOBAL)
            w = GETITEM(names, oparg);
            if ((err = PyDict_DelItem(f->f_globals, w)) != 0)
                format_exc_check_arg(
                    PyExc_NameError, GLOBAL_NAME_ERROR_MSG, w);
            break;

        TARGET(LOAD_NAME)
            w = GETITEM(names, oparg);
            if ((v = f->f_locals) == NULL) {
                PyErr_Format(PyExc_SystemError,
                             "no locals when loading %R", w);
                why = WHY_EXCEPTION;
                break;
            }
            if (PyDict_CheckExact(v)) {
                x = PyDict_GetItem(v, w);
                Py_XINCREF(x);
            }
            else {
                x = PyObject_GetItem(v, w);
                if (x == NULL && PyErr_Occurred()) {
                    if (!PyErr_ExceptionMatches(
                                    PyExc_KeyError))
                        break;
                    PyErr_Clear();
                }
            }
            if (x == NULL) {
                x = PyDict_GetItem(f->f_globals, w);
                Py_XINCREF(x);
                if (x == NULL) {
                    if (PyDict_CheckExact(f->f_builtins)) {
                        x = PyDict_GetItem(f->f_builtins, w);
                        if (x == NULL) {
                            format_exc_check_arg(
                                        PyExc_NameError,
                                        NAME_ERROR_MSG, w);
                            break;
                        }
                        Py_INCREF(x);
                    }
                    else {
                        x = PyObject_GetItem(f->f_builtins, w);
                        if (x == NULL) {
                            if (PyErr_ExceptionMatches(PyExc_KeyError))
                                format_exc_check_arg(
                                            PyExc_NameError,
                                            NAME_ERROR_MSG, w);
                            break;
                        }
                    }
                }
            }
            PUSH(x);
            DISPATCH();

        TARGET(LOAD_GLOBAL)
            w = GETITEM(names, oparg);
            if (PyDict_CheckExact(f->f_globals)
                && PyDict_CheckExact(f->f_builtins)) {
                x = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
                                       (PyDictObject *)f->f_builtins,
                                       w);
                if (x == NULL) {
                    if (!PyErr_Occurred())
                        format_exc_check_arg(PyExc_NameError,
                                             GLOBAL_NAME_ERROR_MSG, w);
                    break;
                }
                Py_INCREF(x);
            }
            else {
                /* Slow-path if globals or builtins is not a dict */
                x = PyObject_GetItem(f->f_globals, w);
                if (x == NULL) {
                    x = PyObject_GetItem(f->f_builtins, w);
                    if (x == NULL) {
                        if (PyErr_ExceptionMatches(PyExc_KeyError))
                            format_exc_check_arg(
                                        PyExc_NameError,
                                        GLOBAL_NAME_ERROR_MSG, w);
                        break;
                    }
                }
            }
            PUSH(x);
            DISPATCH();

        TARGET(DELETE_FAST)
            x = GETLOCAL(oparg);
            if (x != NULL) {
                SETLOCAL(oparg, NULL);
                DISPATCH();
            }
            format_exc_check_arg(
                PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(co->co_varnames, oparg)
                );
            break;

        TARGET(DELETE_DEREF)
            x = freevars[oparg];
            if (PyCell_GET(x) != NULL) {
                PyCell_Set(x, NULL);
                DISPATCH();
            }
            err = -1;
            format_exc_unbound(co, oparg);
            break;

        TARGET(LOAD_CLOSURE)
            x = freevars[oparg];
            Py_INCREF(x);
            PUSH(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(LOAD_DEREF)
            x = freevars[oparg];
            w = PyCell_Get(x);
            if (w != NULL) {
                PUSH(w);
                DISPATCH();
            }
            err = -1;
            format_exc_unbound(co, oparg);
            break;

        TARGET(STORE_DEREF)
            w = POP();
            x = freevars[oparg];
            PyCell_Set(x, w);
            Py_DECREF(w);
            DISPATCH();

        TARGET(BUILD_TUPLE)
            x = PyTuple_New(oparg);
            if (x != NULL) {
                for (; --oparg >= 0;) {
                    w = POP();
                    PyTuple_SET_ITEM(x, oparg, w);
                }
                PUSH(x);
                DISPATCH();
            }
            break;

        TARGET(BUILD_LIST)
            x =  PyList_New(oparg);
            if (x != NULL) {
                for (; --oparg >= 0;) {
                    w = POP();
                    PyList_SET_ITEM(x, oparg, w);
                }
                PUSH(x);
                DISPATCH();
            }
            break;

        TARGET(BUILD_SET)
            x = PySet_New(NULL);
            if (x != NULL) {
                for (; --oparg >= 0;) {
                    w = POP();
                    if (err == 0)
                        err = PySet_Add(x, w);
                    Py_DECREF(w);
                }
                if (err != 0) {
                    Py_DECREF(x);
                    break;
                }
                PUSH(x);
                DISPATCH();
            }
            break;

        TARGET(BUILD_MAP)
            x = _PyDict_NewPresized((Py_ssize_t)oparg);
            PUSH(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(STORE_MAP)
            w = TOP();     /* key */
            u = SECOND();  /* value */
            v = THIRD();   /* dict */
            STACKADJ(-2);
            assert (PyDict_CheckExact(v));
            err = PyDict_SetItem(v, w, u);  /* v[w] = u */
            Py_DECREF(u);
            Py_DECREF(w);
            if (err == 0) DISPATCH();
            break;

        TARGET(MAP_ADD)
            w = TOP();     /* key */
            u = SECOND();  /* value */
            STACKADJ(-2);
            v = stack_pointer[-oparg];  /* dict */
            assert (PyDict_CheckExact(v));
            err = PyDict_SetItem(v, w, u);  /* v[w] = u */
            Py_DECREF(u);
            Py_DECREF(w);
            if (err == 0) {
                PREDICT(JUMP_ABSOLUTE);
                DISPATCH();
            }
            break;

        TARGET(LOAD_ATTR)
            w = GETITEM(names, oparg);
            v = TOP();
            x = PyObject_GetAttr(v, w);
            Py_DECREF(v);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(COMPARE_OP)
            w = POP();
            v = TOP();
            x = cmp_outcome(oparg, v, w);
            Py_DECREF(v);
            Py_DECREF(w);
            SET_TOP(x);
            if (x == NULL) break;
            PREDICT(POP_JUMP_IF_FALSE);
            PREDICT(POP_JUMP_IF_TRUE);
            DISPATCH();

        TARGET(IMPORT_NAME)
        {
            _Py_IDENTIFIER(__import__);
            w = GETITEM(names, oparg);
            x = _PyDict_GetItemId(f->f_builtins, &PyId___import__);
            if (x == NULL) {
                PyErr_SetString(PyExc_ImportError,
                                "__import__ not found");
                break;
            }
            Py_INCREF(x);
            v = POP();
            u = TOP();
            if (PyLong_AsLong(u) != -1 || PyErr_Occurred())
                w = PyTuple_Pack(5,
                            w,
                            f->f_globals,
                            f->f_locals == NULL ?
                                  Py_None : f->f_locals,
                            v,
                            u);
            else
                w = PyTuple_Pack(4,
                            w,
                            f->f_globals,
                            f->f_locals == NULL ?
                                  Py_None : f->f_locals,
                            v);
            Py_DECREF(v);
            Py_DECREF(u);
            if (w == NULL) {
                u = POP();
                Py_DECREF(x);
                x = NULL;
                break;
            }
            READ_TIMESTAMP(intr0);
            v = x;
            x = PyEval_CallObject(v, w);
            Py_DECREF(v);
            READ_TIMESTAMP(intr1);
            Py_DECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;
        }

        TARGET(IMPORT_STAR)
            v = POP();
            PyFrame_FastToLocals(f);
            if ((x = f->f_locals) == NULL) {
                PyErr_SetString(PyExc_SystemError,
                    "no locals found during 'import *'");
                break;
            }
            READ_TIMESTAMP(intr0);
            err = import_all_from(x, v);
            READ_TIMESTAMP(intr1);
            PyFrame_LocalsToFast(f, 0);
            Py_DECREF(v);
            if (err == 0) DISPATCH();
            break;

        TARGET(IMPORT_FROM)
            w = GETITEM(names, oparg);
            v = TOP();
            READ_TIMESTAMP(intr0);
            x = import_from(v, w);
            READ_TIMESTAMP(intr1);
            PUSH(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(JUMP_FORWARD)
            JUMPBY(oparg);
            FAST_DISPATCH();

        PREDICTED_WITH_ARG(POP_JUMP_IF_FALSE);
        TARGET(POP_JUMP_IF_FALSE)
            w = POP();
            if (w == Py_True) {
                Py_DECREF(w);
                FAST_DISPATCH();
            }
            if (w == Py_False) {
                Py_DECREF(w);
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(w);
            Py_DECREF(w);
            if (err > 0)
                err = 0;
            else if (err == 0)
                JUMPTO(oparg);
            else
                break;
            DISPATCH();

        PREDICTED_WITH_ARG(POP_JUMP_IF_TRUE);
        TARGET(POP_JUMP_IF_TRUE)
            w = POP();
            if (w == Py_False) {
                Py_DECREF(w);
                FAST_DISPATCH();
            }
            if (w == Py_True) {
                Py_DECREF(w);
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(w);
            Py_DECREF(w);
            if (err > 0) {
                err = 0;
                JUMPTO(oparg);
            }
            else if (err == 0)
                ;
            else
                break;
            DISPATCH();

        TARGET(JUMP_IF_FALSE_OR_POP)
            w = TOP();
            if (w == Py_True) {
                STACKADJ(-1);
                Py_DECREF(w);
                FAST_DISPATCH();
            }
            if (w == Py_False) {
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(w);
            if (err > 0) {
                STACKADJ(-1);
                Py_DECREF(w);
                err = 0;
            }
            else if (err == 0)
                JUMPTO(oparg);
            else
                break;
            DISPATCH();

        TARGET(JUMP_IF_TRUE_OR_POP)
            w = TOP();
            if (w == Py_False) {
                STACKADJ(-1);
                Py_DECREF(w);
                FAST_DISPATCH();
            }
            if (w == Py_True) {
                JUMPTO(oparg);
                FAST_DISPATCH();
            }
            err = PyObject_IsTrue(w);
            if (err > 0) {
                err = 0;
                JUMPTO(oparg);
            }
            else if (err == 0) {
                STACKADJ(-1);
                Py_DECREF(w);
            }
            else
                break;
            DISPATCH();

        PREDICTED_WITH_ARG(JUMP_ABSOLUTE);
        TARGET(JUMP_ABSOLUTE)
            JUMPTO(oparg);
#if FAST_LOOPS
            /* Enabling this path speeds-up all while and for-loops by bypassing
               the per-loop checks for signals.  By default, this should be turned-off
               because it prevents detection of a control-break in tight loops like
               "while 1: pass".  Compile with this option turned-on when you need
               the speed-up and do not need break checking inside tight loops (ones
               that contain only instructions ending with FAST_DISPATCH).
            */
            FAST_DISPATCH();
#else
            DISPATCH();
#endif

        TARGET(GET_ITER)
            /* before: [obj]; after [getiter(obj)] */
            v = TOP();
            x = PyObject_GetIter(v);
            Py_DECREF(v);
            if (x != NULL) {
                SET_TOP(x);
                PREDICT(FOR_ITER);
                DISPATCH();
            }
            STACKADJ(-1);
            break;

        PREDICTED_WITH_ARG(FOR_ITER);
        TARGET(FOR_ITER)
            /* before: [iter]; after: [iter, iter()] *or* [] */
            v = TOP();
            x = (*v->ob_type->tp_iternext)(v);
            if (x != NULL) {
                PUSH(x);
                PREDICT(STORE_FAST);
                PREDICT(UNPACK_SEQUENCE);
                DISPATCH();
            }
            if (PyErr_Occurred()) {
                if (!PyErr_ExceptionMatches(
                                PyExc_StopIteration))
                    break;
                PyErr_Clear();
            }
            /* iterator ended normally */
            x = v = POP();
            Py_DECREF(v);
            JUMPBY(oparg);
            DISPATCH();

        TARGET(BREAK_LOOP)
            why = WHY_BREAK;
            goto fast_block_end;

        TARGET(CONTINUE_LOOP)
            retval = PyLong_FromLong(oparg);
            if (!retval) {
                x = NULL;
                break;
            }
            why = WHY_CONTINUE;
            goto fast_block_end;

        TARGET_WITH_IMPL(SETUP_LOOP, _setup_finally)
        TARGET_WITH_IMPL(SETUP_EXCEPT, _setup_finally)
        TARGET(SETUP_FINALLY)
        _setup_finally:
            /* NOTE: If you add any new block-setup opcodes that
               are not try/except/finally handlers, you may need
               to update the PyGen_NeedsFinalizing() function.
               */

            PyFrame_BlockSetup(f, opcode, INSTR_OFFSET() + oparg,
                               STACK_LEVEL());
            DISPATCH();

        TARGET(SETUP_WITH)
        {
            _Py_IDENTIFIER(__exit__);
            _Py_IDENTIFIER(__enter__);
            w = TOP();
            x = special_lookup(w, &PyId___exit__);
            if (!x)
                break;
            SET_TOP(x);
            u = special_lookup(w, &PyId___enter__);
            Py_DECREF(w);
            if (!u) {
                x = NULL;
                break;
            }
            x = PyObject_CallFunctionObjArgs(u, NULL);
            Py_DECREF(u);
            if (!x)
                break;
            /* Setup the finally block before pushing the result
               of __enter__ on the stack. */
            PyFrame_BlockSetup(f, SETUP_FINALLY, INSTR_OFFSET() + oparg,
                               STACK_LEVEL());

            PUSH(x);
            DISPATCH();
        }

        TARGET(WITH_CLEANUP)
        {
            /* At the top of the stack are 1-3 values indicating
               how/why we entered the finally clause:
               - TOP = None
               - (TOP, SECOND) = (WHY_{RETURN,CONTINUE}), retval
               - TOP = WHY_*; no retval below it
               - (TOP, SECOND, THIRD) = exc_info()
                 (FOURTH, FITH, SIXTH) = previous exception for EXCEPT_HANDLER
               Below them is EXIT, the context.__exit__ bound method.
               In the last case, we must call
                 EXIT(TOP, SECOND, THIRD)
               otherwise we must call
                 EXIT(None, None, None)

               In the first two cases, we remove EXIT from the
               stack, leaving the rest in the same order.  In the
               third case, we shift the bottom 3 values of the
               stack down, and replace the empty spot with NULL.

               In addition, if the stack represents an exception,
               *and* the function call returns a 'true' value, we
               push WHY_SILENCED onto the stack.  END_FINALLY will
               then not re-raise the exception.  (But non-local
               gotos should still be resumed.)
            */

            PyObject *exit_func;
            u = TOP();
            if (u == Py_None) {
                (void)POP();
                exit_func = TOP();
                SET_TOP(u);
                v = w = Py_None;
            }
            else if (PyLong_Check(u)) {
                (void)POP();
                switch(PyLong_AsLong(u)) {
                case WHY_RETURN:
                case WHY_CONTINUE:
                    /* Retval in TOP. */
                    exit_func = SECOND();
                    SET_SECOND(TOP());
                    SET_TOP(u);
                    break;
                default:
                    exit_func = TOP();
                    SET_TOP(u);
                    break;
                }
                u = v = w = Py_None;
            }
            else {
                PyObject *tp, *exc, *tb;
                PyTryBlock *block;
                v = SECOND();
                w = THIRD();
                tp = FOURTH();
                exc = PEEK(5);
                tb = PEEK(6);
                exit_func = PEEK(7);
                SET_VALUE(7, tb);
                SET_VALUE(6, exc);
                SET_VALUE(5, tp);
                /* UNWIND_EXCEPT_HANDLER will pop this off. */
                SET_FOURTH(NULL);
                /* We just shifted the stack down, so we have
                   to tell the except handler block that the
                   values are lower than it expects. */
                block = &f->f_blockstack[f->f_iblock - 1];
                assert(block->b_type == EXCEPT_HANDLER);
                block->b_level--;
            }
            /* XXX Not the fastest way to call it... */
            x = PyObject_CallFunctionObjArgs(exit_func, u, v, w,
                                             NULL);
            Py_DECREF(exit_func);
            if (x == NULL)
                break; /* Go to error exit */

            if (u != Py_None)
                err = PyObject_IsTrue(x);
            else
                err = 0;
            Py_DECREF(x);

            if (err < 0)
                break; /* Go to error exit */
            else if (err > 0) {
                err = 0;
                /* There was an exception and a True return */
                PUSH(PyLong_FromLong((long) WHY_SILENCED));
            }
            PREDICT(END_FINALLY);
            break;
        }

        TARGET(CALL_FUNCTION)
        {
            PyObject **sp;
            PCALL(PCALL_ALL);
            sp = stack_pointer;
#ifdef WITH_TSC
            x = call_function(&sp, oparg, &intr0, &intr1);
#else
            x = call_function(&sp, oparg);
#endif
            stack_pointer = sp;
            PUSH(x);
            if (x != NULL)
                DISPATCH();
            break;
        }

        TARGET_WITH_IMPL(CALL_FUNCTION_VAR, _call_function_var_kw)
        TARGET_WITH_IMPL(CALL_FUNCTION_KW, _call_function_var_kw)
        TARGET(CALL_FUNCTION_VAR_KW)
        _call_function_var_kw:
        {
            int na = oparg & 0xff;
            int nk = (oparg>>8) & 0xff;
            int flags = (opcode - CALL_FUNCTION) & 3;
            int n = na + 2 * nk;
            PyObject **pfunc, *func, **sp;
            PCALL(PCALL_ALL);
            if (flags & CALL_FLAG_VAR)
                n++;
            if (flags & CALL_FLAG_KW)
                n++;
            pfunc = stack_pointer - n - 1;
            func = *pfunc;

            if (PyMethod_Check(func)
                && PyMethod_GET_SELF(func) != NULL) {
                PyObject *self = PyMethod_GET_SELF(func);
                Py_INCREF(self);
                func = PyMethod_GET_FUNCTION(func);
                Py_INCREF(func);
                Py_DECREF(*pfunc);
                *pfunc = self;
                na++;
                /* n++; */
            } else
                Py_INCREF(func);
            sp = stack_pointer;
            READ_TIMESTAMP(intr0);
            x = ext_do_call(func, &sp, flags, na, nk);
            READ_TIMESTAMP(intr1);
            stack_pointer = sp;
            Py_DECREF(func);

            while (stack_pointer > pfunc) {
                w = POP();
                Py_DECREF(w);
            }
            PUSH(x);
            if (x != NULL)
                DISPATCH();
            break;
        }

        TARGET_WITH_IMPL(MAKE_CLOSURE, _make_function)
        TARGET(MAKE_FUNCTION)
        _make_function:
        {
            int posdefaults = oparg & 0xff;
            int kwdefaults = (oparg>>8) & 0xff;
            int num_annotations = (oparg >> 16) & 0x7fff;

            w = POP(); /* qualname */
            v = POP(); /* code object */
            x = PyFunction_NewWithQualName(v, f->f_globals, w);
            Py_DECREF(v);
            Py_DECREF(w);

            if (x != NULL && opcode == MAKE_CLOSURE) {
                v = POP();
                if (PyFunction_SetClosure(x, v) != 0) {
                    /* Can't happen unless bytecode is corrupt. */
                    why = WHY_EXCEPTION;
                }
                Py_DECREF(v);
            }

            if (x != NULL && num_annotations > 0) {
                Py_ssize_t name_ix;
                u = POP(); /* names of args with annotations */
                v = PyDict_New();
                if (v == NULL) {
                    Py_DECREF(x);
                    x = NULL;
                    break;
                }
                name_ix = PyTuple_Size(u);
                assert(num_annotations == name_ix+1);
                while (name_ix > 0) {
                    --name_ix;
                    t = PyTuple_GET_ITEM(u, name_ix);
                    w = POP();
                    /* XXX(nnorwitz): check for errors */
                    PyDict_SetItem(v, t, w);
                    Py_DECREF(w);
                }

                if (PyFunction_SetAnnotations(x, v) != 0) {
                    /* Can't happen unless
                       PyFunction_SetAnnotations changes. */
                    why = WHY_EXCEPTION;
                }
                Py_DECREF(v);
                Py_DECREF(u);
            }

            /* XXX Maybe this should be a separate opcode? */
            if (x != NULL && posdefaults > 0) {
                v = PyTuple_New(posdefaults);
                if (v == NULL) {
                    Py_DECREF(x);
                    x = NULL;
                    break;
                }
                while (--posdefaults >= 0) {
                    w = POP();
                    PyTuple_SET_ITEM(v, posdefaults, w);
                }
                if (PyFunction_SetDefaults(x, v) != 0) {
                    /* Can't happen unless
                       PyFunction_SetDefaults changes. */
                    why = WHY_EXCEPTION;
                }
                Py_DECREF(v);
            }
            if (x != NULL && kwdefaults > 0) {
                v = PyDict_New();
                if (v == NULL) {
                    Py_DECREF(x);
                    x = NULL;
                    break;
                }
                while (--kwdefaults >= 0) {
                    w = POP(); /* default value */
                    u = POP(); /* kw only arg name */
                    /* XXX(nnorwitz): check for errors */
                    PyDict_SetItem(v, u, w);
                    Py_DECREF(w);
                    Py_DECREF(u);
                }
                if (PyFunction_SetKwDefaults(x, v) != 0) {
                    /* Can't happen unless
                       PyFunction_SetKwDefaults changes. */
                    why = WHY_EXCEPTION;
                }
                Py_DECREF(v);
            }
            PUSH(x);
            break;
        }

        TARGET(BUILD_SLICE)
            if (oparg == 3)
                w = POP();
            else
                w = NULL;
            v = POP();
            u = TOP();
            x = PySlice_New(u, v, w);
            Py_DECREF(u);
            Py_DECREF(v);
            Py_XDECREF(w);
            SET_TOP(x);
            if (x != NULL) DISPATCH();
            break;

        TARGET(EXTENDED_ARG)
            opcode = NEXTOP();
            oparg = oparg<<16 | NEXTARG();
            goto dispatch_opcode;

#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#endif
        default:
            fprintf(stderr,
                "XXX lineno: %d, opcode: %d\n",
                PyFrame_GetLineNumber(f),
                opcode);
            PyErr_SetString(PyExc_SystemError, "unknown opcode");
            why = WHY_EXCEPTION;
            break;

#ifdef CASE_TOO_BIG
        }
#endif

        } /* switch */

        on_error:

        READ_TIMESTAMP(inst1);

        /* Quickly continue if no error occurred */

        if (why == WHY_NOT) {
            if (err == 0 && x != NULL) {
#ifdef CHECKEXC
                /* This check is expensive! */
                if (PyErr_Occurred())
                    fprintf(stderr,
                        "XXX undetected error\n");
                else {
#endif
                    READ_TIMESTAMP(loop1);
                    continue; /* Normal, fast path */
#ifdef CHECKEXC
                }
#endif
            }
            why = WHY_EXCEPTION;
            x = Py_None;
            err = 0;
        }

        /* Double-check exception status */

        if (why == WHY_EXCEPTION || why == WHY_RERAISE) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_SystemError,
                    "error return without exception set");
                why = WHY_EXCEPTION;
            }
        }
#ifdef CHECKEXC
        else {
            /* This check is expensive! */
            if (PyErr_Occurred()) {
                char buf[128];
                sprintf(buf, "Stack unwind with exception "
                    "set and why=%d", why);
                Py_FatalError(buf);
            }
        }
#endif

        /* Log traceback info if this is a real exception */

        if (why == WHY_EXCEPTION) {
            PyTraceBack_Here(f);

            if (tstate->c_tracefunc != NULL)
                call_exc_trace(tstate->c_tracefunc,
                               tstate->c_traceobj, f);
        }

        /* For the rest, treat WHY_RERAISE as WHY_EXCEPTION */

        if (why == WHY_RERAISE)
            why = WHY_EXCEPTION;

        /* Unwind stacks if a (pseudo) exception occurred */

fast_block_end:
        while (why != WHY_NOT && f->f_iblock > 0) {
            /* Peek at the current block. */
            PyTryBlock *b = &f->f_blockstack[f->f_iblock - 1];

            assert(why != WHY_YIELD);
            if (b->b_type == SETUP_LOOP && why == WHY_CONTINUE) {
                why = WHY_NOT;
                JUMPTO(PyLong_AS_LONG(retval));
                Py_DECREF(retval);
                break;
            }
            /* Now we have to pop the block. */
            f->f_iblock--;

            if (b->b_type == EXCEPT_HANDLER) {
                UNWIND_EXCEPT_HANDLER(b);
                continue;
            }
            UNWIND_BLOCK(b);
            if (b->b_type == SETUP_LOOP && why == WHY_BREAK) {
                why = WHY_NOT;
                JUMPTO(b->b_handler);
                break;
            }
            if (why == WHY_EXCEPTION && (b->b_type == SETUP_EXCEPT
                || b->b_type == SETUP_FINALLY)) {
                PyObject *exc, *val, *tb;
                int handler = b->b_handler;
                /* Beware, this invalidates all b->b_* fields */
                PyFrame_BlockSetup(f, EXCEPT_HANDLER, -1, STACK_LEVEL());
                PUSH(tstate->exc_traceback);
                PUSH(tstate->exc_value);
                if (tstate->exc_type != NULL) {
                    PUSH(tstate->exc_type);
                }
                else {
                    Py_INCREF(Py_None);
                    PUSH(Py_None);
                }
                PyErr_Fetch(&exc, &val, &tb);
                /* Make the raw exception data
                   available to the handler,
                   so a program can emulate the
                   Python main loop. */
                PyErr_NormalizeException(
                    &exc, &val, &tb);
                PyException_SetTraceback(val, tb);
                Py_INCREF(exc);
                tstate->exc_type = exc;
                Py_INCREF(val);
                tstate->exc_value = val;
                tstate->exc_traceback = tb;
                if (tb == NULL)
                    tb = Py_None;
                Py_INCREF(tb);
                PUSH(tb);
                PUSH(val);
                PUSH(exc);
                why = WHY_NOT;
                JUMPTO(handler);
                break;
            }
            if (b->b_type == SETUP_FINALLY) {
                if (why & (WHY_RETURN | WHY_CONTINUE))
                    PUSH(retval);
                PUSH(PyLong_FromLong((long)why));
                why = WHY_NOT;
                JUMPTO(b->b_handler);
                break;
            }
        } /* unwind stack */

        /* End the loop if we still have an error (or return) */

        if (why != WHY_NOT)
            break;
        READ_TIMESTAMP(loop1);

    } /* main loop */

    assert(why != WHY_YIELD);
    /* Pop remaining stack entries. */
    while (!EMPTY()) {
        v = POP();
        Py_XDECREF(v);
    }

    if (why != WHY_RETURN)
        retval = NULL;

fast_yield:
    if (co->co_flags & CO_GENERATOR && (why == WHY_YIELD || why == WHY_RETURN)) {
        /* The purpose of this block is to put aside the generator's exception
           state and restore that of the calling frame. If the current
           exception state is from the caller, we clear the exception values
           on the generator frame, so they are not swapped back in latter. The
           origin of the current exception state is determined by checking for
           except handler blocks, which we must be in iff a new exception
           state came into existence in this frame. (An uncaught exception
           would have why == WHY_EXCEPTION, and we wouldn't be here). */
        int i;
        for (i = 0; i < f->f_iblock; i++)
            if (f->f_blockstack[i].b_type == EXCEPT_HANDLER)
                break;
        if (i == f->f_iblock)
            /* We did not create this exception. */
            restore_and_clear_exc_state(tstate, f);
        else
            swap_exc_state(tstate, f);
    }

    if (tstate->use_tracing) {
        if (tstate->c_tracefunc) {
            if (why == WHY_RETURN || why == WHY_YIELD) {
                if (call_trace(tstate->c_tracefunc,
                               tstate->c_traceobj, f,
                               PyTrace_RETURN, retval)) {
                    Py_XDECREF(retval);
                    retval = NULL;
                    why = WHY_EXCEPTION;
                }
            }
            else if (why == WHY_EXCEPTION) {
                call_trace_protected(tstate->c_tracefunc,
                                     tstate->c_traceobj, f,
                                     PyTrace_RETURN, NULL);
            }
        }
        if (tstate->c_profilefunc) {
            if (why == WHY_EXCEPTION)
                call_trace_protected(tstate->c_profilefunc,
                                     tstate->c_profileobj, f,
                                     PyTrace_RETURN, NULL);
            else if (call_trace(tstate->c_profilefunc,
                                tstate->c_profileobj, f,
                                PyTrace_RETURN, retval)) {
                Py_XDECREF(retval);
                retval = NULL;
                /* why = WHY_EXCEPTION; */
            }
        }
    }

    /* pop frame */
exit_eval_frame:
    Py_LeaveRecursiveCall();
    tstate->frame = f->f_back;

    return retval;
}

static void
format_missing(const char *kind, PyCodeObject *co, PyObject *names)
{
    int err;
    Py_ssize_t len = PyList_GET_SIZE(names);
    PyObject *name_str, *comma, *tail, *tmp;

    assert(PyList_CheckExact(names));
    assert(len >= 1);
    /* Deal with the joys of natural language. */
    switch (len) {
    case 1:
        name_str = PyList_GET_ITEM(names, 0);
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyList_GET_ITEM(names, len - 2),
                                        PyList_GET_ITEM(names, len - 1));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyList_GET_ITEM(names, len - 2),
                                    PyList_GET_ITEM(names, len - 1));
        if (tail == NULL)
            return;
        /* Chop off the last two objects in the list. This shouldn't actually
           fail, but we can't be too careful. */
        err = PyList_SetSlice(names, len - 2, len, NULL);
        if (err == -1) {
            Py_DECREF(tail);
            return;
        }
        /* Stitch everything up into a nice comma-separated list. */
        comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            Py_DECREF(tail);
            return;
        }
        tmp = PyUnicode_Join(comma, names);
        Py_DECREF(comma);
        if (tmp == NULL) {
            Py_DECREF(tail);
            return;
        }
        name_str = PyUnicode_Concat(tmp, tail);
        Py_DECREF(tmp);
        Py_DECREF(tail);
        break;
    }
    if (name_str == NULL)
        return;
    PyErr_Format(PyExc_TypeError,
                 "%U() missing %i required %s argument%s: %U",
                 co->co_name,
                 len,
                 kind,
                 len == 1 ? "" : "s",
                 name_str);
    Py_DECREF(name_str);
}

static void
missing_arguments(PyCodeObject *co, int missing, int defcount,
                  PyObject **fastlocals)
{
    int i, j = 0;
    int start, end;
    int positional = defcount != -1;
    const char *kind = positional ? "positional" : "keyword-only";
    PyObject *missing_names;

    /* Compute the names of the arguments that are missing. */
    missing_names = PyList_New(missing);
    if (missing_names == NULL)
        return;
    if (positional) {
        start = 0;
        end = co->co_argcount - defcount;
    }
    else {
        start = co->co_argcount;
        end = start + co->co_kwonlyargcount;
    }
    for (i = start; i < end; i++) {
        if (GETLOCAL(i) == NULL) {
            PyObject *raw = PyTuple_GET_ITEM(co->co_varnames, i);
            PyObject *name = PyObject_Repr(raw);
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_ITEM(missing_names, j++, name);
        }
    }
    assert(j == missing);
    format_missing(kind, co, missing_names);
    Py_DECREF(missing_names);
}

static void
too_many_positional(PyCodeObject *co, int given, int defcount, PyObject **fastlocals)
{
    int plural;
    int kwonly_given = 0;
    int i;
    PyObject *sig, *kwonly_sig;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co->co_argcount; i < co->co_argcount + co->co_kwonlyargcount; i++)
        if (GETLOCAL(i) != NULL)
            kwonly_given++;
    if (defcount) {
        int atleast = co->co_argcount - defcount;
        plural = 1;
        sig = PyUnicode_FromFormat("from %d to %d", atleast, co->co_argcount);
    }
    else {
        plural = co->co_argcount != 1;
        sig = PyUnicode_FromFormat("%d", co->co_argcount);
    }
    if (sig == NULL)
        return;
    if (kwonly_given) {
        const char *format = " positional argument%s (and %d keyword-only argument%s)";
        kwonly_sig = PyUnicode_FromFormat(format, given != 1 ? "s" : "", kwonly_given,
                                              kwonly_given != 1 ? "s" : "");
        if (kwonly_sig == NULL) {
            Py_DECREF(sig);
            return;
        }
    }
    else {
        /* This will not fail. */
        kwonly_sig = PyUnicode_FromString("");
        assert(kwonly_sig != NULL);
    }
    PyErr_Format(PyExc_TypeError,
                 "%U() takes %U positional argument%s but %d%U %s given",
                 co->co_name,
                 sig,
                 plural ? "s" : "",
                 given,
                 kwonly_sig,
                 given == 1 && !kwonly_given ? "was" : "were");
    Py_DECREF(sig);
    Py_DECREF(kwonly_sig);
}

/* This is gonna seem *real weird*, but if you put some other code between
   PyEval_EvalFrame() and PyEval_EvalCodeEx() you will need to adjust
   the test in the if statements in Misc/gdbinit (pystack and pystackv). */

PyObject *
PyEval_EvalCodeEx(PyObject *_co, PyObject *globals, PyObject *locals,
           PyObject **args, int argcount, PyObject **kws, int kwcount,
           PyObject **defs, int defcount, PyObject *kwdefs, PyObject *closure)
{
    PyCodeObject* co = (PyCodeObject*)_co;
    register PyFrameObject *f;
    register PyObject *retval = NULL;
    register PyObject **fastlocals, **freevars;
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *x, *u;
    int total_args = co->co_argcount + co->co_kwonlyargcount;
    int i;
    int n = argcount;
    PyObject *kwdict = NULL;

    if (globals == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "PyEval_EvalCodeEx: NULL globals");
        return NULL;
    }

    assert(tstate != NULL);
    assert(globals != NULL);
    f = PyFrame_New(tstate, co, globals, locals);
    if (f == NULL)
        return NULL;

    fastlocals = f->f_localsplus;
    freevars = f->f_localsplus + co->co_nlocals;

    /* Parse arguments. */
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL)
            goto fail;
        i = total_args;
        if (co->co_flags & CO_VARARGS)
            i++;
        SETLOCAL(i, kwdict);
    }
    if (argcount > co->co_argcount)
        n = co->co_argcount;
    for (i = 0; i < n; i++) {
        x = args[i];
        Py_INCREF(x);
        SETLOCAL(i, x);
    }
    if (co->co_flags & CO_VARARGS) {
        u = PyTuple_New(argcount - n);
        if (u == NULL)
            goto fail;
        SETLOCAL(total_args, u);
        for (i = n; i < argcount; i++) {
            x = args[i];
            Py_INCREF(x);
            PyTuple_SET_ITEM(u, i-n, x);
        }
    }
    for (i = 0; i < kwcount; i++) {
        PyObject **co_varnames;
        PyObject *keyword = kws[2*i];
        PyObject *value = kws[2*i + 1];
        int j;
        if (keyword == NULL || !PyUnicode_Check(keyword)) {
            PyErr_Format(PyExc_TypeError,
                         "%U() keywords must be strings",
                         co->co_name);
            goto fail;
        }
        /* Speed hack: do raw pointer compares. As names are
           normally interned this should almost always hit. */
        co_varnames = ((PyTupleObject *)(co->co_varnames))->ob_item;
        for (j = 0; j < total_args; j++) {
            PyObject *nm = co_varnames[j];
            if (nm == keyword)
                goto kw_found;
        }
        /* Slow fallback, just in case */
        for (j = 0; j < total_args; j++) {
            PyObject *nm = co_varnames[j];
            int cmp = PyObject_RichCompareBool(
                keyword, nm, Py_EQ);
            if (cmp > 0)
                goto kw_found;
            else if (cmp < 0)
                goto fail;
        }
        if (j >= total_args && kwdict == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U() got an unexpected "
                         "keyword argument '%S'",
                         co->co_name,
                         keyword);
            goto fail;
        }
        PyDict_SetItem(kwdict, keyword, value);
        continue;
      kw_found:
        if (GETLOCAL(j) != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U() got multiple "
                         "values for argument '%S'",
                         co->co_name,
                         keyword);
            goto fail;
        }
        Py_INCREF(value);
        SETLOCAL(j, value);
    }
    if (argcount > co->co_argcount && !(co->co_flags & CO_VARARGS)) {
        too_many_positional(co, argcount, defcount, fastlocals);
        goto fail;
    }
    if (argcount < co->co_argcount) {
        int m = co->co_argcount - defcount;
        int missing = 0;
        for (i = argcount; i < m; i++)
            if (GETLOCAL(i) == NULL)
                missing++;
        if (missing) {
            missing_arguments(co, missing, defcount, fastlocals);
            goto fail;
        }
        if (n > m)
            i = n - m;
        else
            i = 0;
        for (; i < defcount; i++) {
            if (GETLOCAL(m+i) == NULL) {
                PyObject *def = defs[i];
                Py_INCREF(def);
                SETLOCAL(m+i, def);
            }
        }
    }
    if (co->co_kwonlyargcount > 0) {
        int missing = 0;
        for (i = co->co_argcount; i < total_args; i++) {
            PyObject *name;
            if (GETLOCAL(i) != NULL)
                continue;
            name = PyTuple_GET_ITEM(co->co_varnames, i);
            if (kwdefs != NULL) {
                PyObject *def = PyDict_GetItem(kwdefs, name);
                if (def) {
                    Py_INCREF(def);
                    SETLOCAL(i, def);
                    continue;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(co, missing, -1, fastlocals);
            goto fail;
        }
    }

    /* Allocate and initialize storage for cell vars, and copy free
       vars into frame. */
    for (i = 0; i < PyTuple_GET_SIZE(co->co_cellvars); ++i) {
        PyObject *c;
        int arg;
        /* Possibly account for the cell variable being an argument. */
        if (co->co_cell2arg != NULL &&
            (arg = co->co_cell2arg[i]) != CO_CELL_NOT_AN_ARG)
            c = PyCell_New(GETLOCAL(arg));
        else
            c = PyCell_New(NULL);
        if (c == NULL)
            goto fail;
        SETLOCAL(co->co_nlocals + i, c);
    }
    for (i = 0; i < PyTuple_GET_SIZE(co->co_freevars); ++i) {
        PyObject *o = PyTuple_GET_ITEM(closure, i);
        Py_INCREF(o);
        freevars[PyTuple_GET_SIZE(co->co_cellvars) + i] = o;
    }

    if (co->co_flags & CO_GENERATOR) {
        /* Don't need to keep the reference to f_back, it will be set
         * when the generator is resumed. */
        Py_XDECREF(f->f_back);
        f->f_back = NULL;

        PCALL(PCALL_GENERATOR);

        /* Create a new generator that owns the ready to run frame
         * and return that as the value. */
        return PyGen_New(f);
    }

    retval = PyEval_EvalFrameEx(f,0);

fail: /* Jump here from prelude on failure */

    /* decref'ing the frame can cause __del__ methods to get invoked,
       which can call back into Python.  While we're done with the
       current Python frame (f), the associated C stack is still in use,
       so recursion_depth must be boosted for the duration.
    */
    assert(tstate != NULL);
    ++tstate->recursion_depth;
    Py_DECREF(f);
    --tstate->recursion_depth;
    return retval;
}


static PyObject *
special_lookup(PyObject *o, _Py_Identifier *id)
{
    PyObject *res;
    res = _PyObject_LookupSpecial(o, id);
    if (res == NULL && !PyErr_Occurred()) {
        PyErr_SetObject(PyExc_AttributeError, id->object);
        return NULL;
    }
    return res;
}


/* These 3 functions deal with the exception state of generators. */

static void
save_exc_state(PyThreadState *tstate, PyFrameObject *f)
{
    PyObject *type, *value, *traceback;
    Py_XINCREF(tstate->exc_type);
    Py_XINCREF(tstate->exc_value);
    Py_XINCREF(tstate->exc_traceback);
    type = f->f_exc_type;
    value = f->f_exc_value;
    traceback = f->f_exc_traceback;
    f->f_exc_type = tstate->exc_type;
    f->f_exc_value = tstate->exc_value;
    f->f_exc_traceback = tstate->exc_traceback;
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
}

static void
swap_exc_state(PyThreadState *tstate, PyFrameObject *f)
{
    PyObject *tmp;
    tmp = tstate->exc_type;
    tstate->exc_type = f->f_exc_type;
    f->f_exc_type = tmp;
    tmp = tstate->exc_value;
    tstate->exc_value = f->f_exc_value;
    f->f_exc_value = tmp;
    tmp = tstate->exc_traceback;
    tstate->exc_traceback = f->f_exc_traceback;
    f->f_exc_traceback = tmp;
}

static void
restore_and_clear_exc_state(PyThreadState *tstate, PyFrameObject *f)
{
    PyObject *type, *value, *tb;
    type = tstate->exc_type;
    value = tstate->exc_value;
    tb = tstate->exc_traceback;
    tstate->exc_type = f->f_exc_type;
    tstate->exc_value = f->f_exc_value;
    tstate->exc_traceback = f->f_exc_traceback;
    f->f_exc_type = NULL;
    f->f_exc_value = NULL;
    f->f_exc_traceback = NULL;
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
}


/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static enum why_code
do_raise(PyObject *exc, PyObject *cause)
{
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        PyThreadState *tstate = PyThreadState_GET();
        PyObject *tb;
        type = tstate->exc_type;
        value = tstate->exc_value;
        tb = tstate->exc_traceback;
        if (type == Py_None) {
            PyErr_SetString(PyExc_RuntimeError,
                            "No active exception to reraise");
            return WHY_EXCEPTION;
            }
        Py_XINCREF(type);
        Py_XINCREF(value);
        Py_XINCREF(tb);
        PyErr_Restore(type, value, tb);
        return WHY_RERAISE;
    }

    /* We support the following forms of raise:
       raise
       raise <instance>
       raise <type> */

    if (PyExceptionClass_Check(exc)) {
        type = exc;
        value = PyObject_CallObject(exc, NULL);
        if (value == NULL)
            goto raise_error;
        if (!PyExceptionInstance_Check(value)) {
            PyErr_Format(PyExc_TypeError,
                         "calling %R should have returned an instance of "
                         "BaseException, not %R",
                         type, Py_TYPE(value));
            goto raise_error;
        }
    }
    else if (PyExceptionInstance_Check(exc)) {
        value = exc;
        type = PyExceptionInstance_Class(exc);
        Py_INCREF(type);
    }
    else {
        /* Not something you can raise.  You get an exception
           anyway, just not what you specified :-) */
        Py_DECREF(exc);
        PyErr_SetString(PyExc_TypeError,
                        "exceptions must derive from BaseException");
        goto raise_error;
    }

    if (cause) {
        PyObject *fixed_cause;
        if (PyExceptionClass_Check(cause)) {
            fixed_cause = PyObject_CallObject(cause, NULL);
            if (fixed_cause == NULL)
                goto raise_error;
            Py_DECREF(cause);
        }
        else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
        }
        else if (cause == Py_None) {
            Py_DECREF(cause);
            fixed_cause = NULL;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "exception causes must derive from "
                            "BaseException");
            goto raise_error;
        }
        PyException_SetCause(value, fixed_cause);
    }

    PyErr_SetObject(type, value);
    /* PyErr_SetObject incref's its arguments */
    Py_XDECREF(value);
    Py_XDECREF(type);
    return WHY_EXCEPTION;

raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(cause);
    return WHY_EXCEPTION;
}

/* Iterate v argcnt times and store the results on the stack (via decreasing
   sp).  Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

static int
unpack_iterable(PyObject *v, int argcnt, int argcntafter, PyObject **sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
    PyObject *w;
    PyObject *l = NULL; /* variable list */

    assert(v != NULL);

    it = PyObject_GetIter(v);
    if (it == NULL)
        goto Error;

    for (; i < argcnt; i++) {
        w = PyIter_Next(it);
        if (w == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError,
                    "need more than %d value%s to unpack",
                    i, i == 1 ? "" : "s");
            }
            goto Error;
        }
        *--sp = w;
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        w = PyIter_Next(it);
        if (w == NULL) {
            if (PyErr_Occurred())
                goto Error;
            Py_DECREF(it);
            return 1;
        }
        Py_DECREF(w);
        PyErr_Format(PyExc_ValueError, "too many values to unpack "
                     "(expected %d)", argcnt);
        goto Error;
    }

    l = PySequence_List(it);
    if (l == NULL)
        goto Error;
    *--sp = l;
    i++;

    ll = PyList_GET_SIZE(l);
    if (ll < argcntafter) {
        PyErr_Format(PyExc_ValueError, "need more than %zd values to unpack",
                     argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        *--sp = PyList_GET_ITEM(l, ll - j);
    }
    /* Resize the list. */
    Py_SIZE(l) = ll - argcntafter;
    Py_DECREF(it);
    return 1;

Error:
    for (; i > 0; i--, sp++)
        Py_DECREF(*sp);
    Py_XDECREF(it);
    return 0;
}


#ifdef LLTRACE
static int
prtrace(PyObject *v, char *str)
{
    printf("%s ", str);
    if (PyObject_Print(v, stdout, 0) != 0)
        PyErr_Clear(); /* Don't know what else to do */
    printf("\n");
    return 1;
}
#endif

static void
call_exc_trace(Py_tracefunc func, PyObject *self, PyFrameObject *f)
{
    PyObject *type, *value, *traceback, *arg;
    int err;
    PyErr_Fetch(&type, &value, &traceback);
    if (value == NULL) {
        value = Py_None;
        Py_INCREF(value);
    }
    PyErr_NormalizeException(&type, &value, &traceback);
    arg = PyTuple_Pack(3, type, value, traceback);
    if (arg == NULL) {
        PyErr_Restore(type, value, traceback);
        return;
    }
    err = call_trace(func, self, f, PyTrace_EXCEPTION, arg);
    Py_DECREF(arg);
    if (err == 0)
        PyErr_Restore(type, value, traceback);
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
}

static int
call_trace_protected(Py_tracefunc func, PyObject *obj, PyFrameObject *frame,
                     int what, PyObject *arg)
{
    PyObject *type, *value, *traceback;
    int err;
    PyErr_Fetch(&type, &value, &traceback);
    err = call_trace(func, obj, frame, what, arg);
    if (err == 0)
    {
        PyErr_Restore(type, value, traceback);
        return 0;
    }
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        return -1;
    }
}

static int
call_trace(Py_tracefunc func, PyObject *obj, PyFrameObject *frame,
           int what, PyObject *arg)
{
    register PyThreadState *tstate = frame->f_tstate;
    int result;
    if (tstate->tracing)
        return 0;
    tstate->tracing++;
    tstate->use_tracing = 0;
    result = func(obj, frame, what, arg);
    tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
    tstate->tracing--;
    return result;
}

PyObject *
_PyEval_CallTracing(PyObject *func, PyObject *args)
{
    PyFrameObject *frame = PyEval_GetFrame();
    PyThreadState *tstate = frame->f_tstate;
    int save_tracing = tstate->tracing;
    int save_use_tracing = tstate->use_tracing;
    PyObject *result;

    tstate->tracing = 0;
    tstate->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
    result = PyObject_Call(func, args, NULL);
    tstate->tracing = save_tracing;
    tstate->use_tracing = save_use_tracing;
    return result;
}

/* See Objects/lnotab_notes.txt for a description of how tracing works. */
static int
maybe_call_line_trace(Py_tracefunc func, PyObject *obj,
                      PyFrameObject *frame, int *instr_lb, int *instr_ub,
                      int *instr_prev)
{
    int result = 0;
    int line = frame->f_lineno;

    /* If the last instruction executed isn't in the current
       instruction window, reset the window.
    */
    if (frame->f_lasti < *instr_lb || frame->f_lasti >= *instr_ub) {
        PyAddrPair bounds;
        line = _PyCode_CheckLineNumber(frame->f_code, frame->f_lasti,
                                       &bounds);
        *instr_lb = bounds.ap_lower;
        *instr_ub = bounds.ap_upper;
    }
    /* If the last instruction falls at the start of a line or if
       it represents a jump backwards, update the frame's line
       number and call the trace function. */
    if (frame->f_lasti == *instr_lb || frame->f_lasti < *instr_prev) {
        frame->f_lineno = line;
        result = call_trace(func, obj, frame, PyTrace_LINE, Py_None);
    }
    *instr_prev = frame->f_lasti;
    return result;
}

void
PyEval_SetProfile(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *temp = tstate->c_profileobj;
    Py_XINCREF(arg);
    tstate->c_profilefunc = NULL;
    tstate->c_profileobj = NULL;
    /* Must make sure that tracing is not ignored if 'temp' is freed */
    tstate->use_tracing = tstate->c_tracefunc != NULL;
    Py_XDECREF(temp);
    tstate->c_profilefunc = func;
    tstate->c_profileobj = arg;
    /* Flag that tracing or profiling is turned on */
    tstate->use_tracing = (func != NULL) || (tstate->c_tracefunc != NULL);
}

void
PyEval_SetTrace(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *temp = tstate->c_traceobj;
    _Py_TracingPossible += (func != NULL) - (tstate->c_tracefunc != NULL);
    Py_XINCREF(arg);
    tstate->c_tracefunc = NULL;
    tstate->c_traceobj = NULL;
    /* Must make sure that profiling is not ignored if 'temp' is freed */
    tstate->use_tracing = tstate->c_profilefunc != NULL;
    Py_XDECREF(temp);
    tstate->c_tracefunc = func;
    tstate->c_traceobj = arg;
    /* Flag that tracing or profiling is turned on */
    tstate->use_tracing = ((func != NULL)
                           || (tstate->c_profilefunc != NULL));
}

PyObject *
PyEval_GetBuiltins(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL)
        return PyThreadState_GET()->interp->builtins;
    else
        return current_frame->f_builtins;
}

PyObject *
PyEval_GetLocals(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL)
        return NULL;
    PyFrame_FastToLocals(current_frame);
    return current_frame->f_locals;
}

PyObject *
PyEval_GetGlobals(void)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    if (current_frame == NULL)
        return NULL;
    else
        return current_frame->f_globals;
}

PyFrameObject *
PyEval_GetFrame(void)
{
    PyThreadState *tstate = PyThreadState_GET();
    return _PyThreadState_GetFrame(tstate);
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyFrameObject *current_frame = PyEval_GetFrame();
    int result = cf->cf_flags != 0;

    if (current_frame != NULL) {
        const int codeflags = current_frame->f_code->co_flags;
        const int compilerflags = codeflags & PyCF_MASK;
        if (compilerflags) {
            result = 1;
            cf->cf_flags |= compilerflags;
        }
#if 0 /* future keyword */
        if (codeflags & CO_GENERATOR_ALLOWED) {
            result = 1;
            cf->cf_flags |= CO_GENERATOR_ALLOWED;
        }
#endif
    }
    return result;
}


/* External interface to call any callable object.
   The arg must be a tuple or NULL.  The kw must be a dict or NULL. */

PyObject *
PyEval_CallObjectWithKeywords(PyObject *func, PyObject *arg, PyObject *kw)
{
    PyObject *result;

    if (arg == NULL) {
        arg = PyTuple_New(0);
        if (arg == NULL)
            return NULL;
    }
    else if (!PyTuple_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument list must be a tuple");
        return NULL;
    }
    else
        Py_INCREF(arg);

    if (kw != NULL && !PyDict_Check(kw)) {
        PyErr_SetString(PyExc_TypeError,
                        "keyword list must be a dictionary");
        Py_DECREF(arg);
        return NULL;
    }

    result = PyObject_Call(func, arg, kw);
    Py_DECREF(arg);
    return result;
}

const char *
PyEval_GetFuncName(PyObject *func)
{
    if (PyMethod_Check(func))
        return PyEval_GetFuncName(PyMethod_GET_FUNCTION(func));
    else if (PyFunction_Check(func))
        return _PyUnicode_AsString(((PyFunctionObject*)func)->func_name);
    else if (PyCFunction_Check(func))
        return ((PyCFunctionObject*)func)->m_ml->ml_name;
    else
        return func->ob_type->tp_name;
}

const char *
PyEval_GetFuncDesc(PyObject *func)
{
    if (PyMethod_Check(func))
        return "()";
    else if (PyFunction_Check(func))
        return "()";
    else if (PyCFunction_Check(func))
        return "()";
    else
        return " object";
}

static void
err_args(PyObject *func, int flags, int nargs)
{
    if (flags & METH_NOARGS)
        PyErr_Format(PyExc_TypeError,
                     "%.200s() takes no arguments (%d given)",
                     ((PyCFunctionObject *)func)->m_ml->ml_name,
                     nargs);
    else
        PyErr_Format(PyExc_TypeError,
                     "%.200s() takes exactly one argument (%d given)",
                     ((PyCFunctionObject *)func)->m_ml->ml_name,
                     nargs);
}

#define C_TRACE(x, call) \
if (tstate->use_tracing && tstate->c_profilefunc) { \
    if (call_trace(tstate->c_profilefunc, \
        tstate->c_profileobj, \
        tstate->frame, PyTrace_C_CALL, \
        func)) { \
        x = NULL; \
    } \
    else { \
        x = call; \
        if (tstate->c_profilefunc != NULL) { \
            if (x == NULL) { \
                call_trace_protected(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate->frame, PyTrace_C_EXCEPTION, \
                    func); \
                /* XXX should pass (type, value, tb) */ \
            } else { \
                if (call_trace(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate->frame, PyTrace_C_RETURN, \
                    func)) { \
                    Py_DECREF(x); \
                    x = NULL; \
                } \
            } \
        } \
    } \
} else { \
    x = call; \
    }

static PyObject *
call_function(PyObject ***pp_stack, int oparg
#ifdef WITH_TSC
                , uint64* pintr0, uint64* pintr1
#endif
                )
{
    int na = oparg & 0xff;
    int nk = (oparg>>8) & 0xff;
    int n = na + 2 * nk;
    PyObject **pfunc = (*pp_stack) - n - 1;
    PyObject *func = *pfunc;
    PyObject *x, *w;

    /* Always dispatch PyCFunction first, because these are
       presumed to be the most frequent callable object.
    */
    if (PyCFunction_Check(func) && nk == 0) {
        int flags = PyCFunction_GET_FLAGS(func);
        PyThreadState *tstate = PyThreadState_GET();

        PCALL(PCALL_CFUNCTION);
        if (flags & (METH_NOARGS | METH_O)) {
            PyCFunction meth = PyCFunction_GET_FUNCTION(func);
            PyObject *self = PyCFunction_GET_SELF(func);
            if (flags & METH_NOARGS && na == 0) {
                C_TRACE(x, (*meth)(self,NULL));
            }
            else if (flags & METH_O && na == 1) {
                PyObject *arg = EXT_POP(*pp_stack);
                C_TRACE(x, (*meth)(self,arg));
                Py_DECREF(arg);
            }
            else {
                err_args(func, flags, na);
                x = NULL;
            }
        }
        else {
            PyObject *callargs;
            callargs = load_args(pp_stack, na);
            READ_TIMESTAMP(*pintr0);
            C_TRACE(x, PyCFunction_Call(func,callargs,NULL));
            READ_TIMESTAMP(*pintr1);
            Py_XDECREF(callargs);
        }
    } else {
        if (PyMethod_Check(func) && PyMethod_GET_SELF(func) != NULL) {
            /* optimize access to bound methods */
            PyObject *self = PyMethod_GET_SELF(func);
            PCALL(PCALL_METHOD);
            PCALL(PCALL_BOUND_METHOD);
            Py_INCREF(self);
            func = PyMethod_GET_FUNCTION(func);
            Py_INCREF(func);
            Py_DECREF(*pfunc);
            *pfunc = self;
            na++;
            n++;
        } else
            Py_INCREF(func);
        READ_TIMESTAMP(*pintr0);
        if (PyFunction_Check(func))
            x = fast_function(func, pp_stack, n, na, nk);
        else
            x = do_call(func, pp_stack, na, nk);
        READ_TIMESTAMP(*pintr1);
        Py_DECREF(func);
    }

    /* Clear the stack of the function object.  Also removes
       the arguments in case they weren't consumed already
       (fast_function() and err_args() leave them on the stack).
     */
    while ((*pp_stack) > pfunc) {
        w = EXT_POP(*pp_stack);
        Py_DECREF(w);
        PCALL(PCALL_POP);
    }
    return x;
}

/* The fast_function() function optimize calls for which no argument
   tuple is necessary; the objects are passed directly from the stack.
   For the simplest case -- a function that takes only positional
   arguments and is called with only positional arguments -- it
   inlines the most primitive frame setup code from
   PyEval_EvalCodeEx(), which vastly reduces the checks that must be
   done before evaluating the frame.
*/

static PyObject *
fast_function(PyObject *func, PyObject ***pp_stack, int n, int na, int nk)
{
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);
    PyObject *kwdefs = PyFunction_GET_KW_DEFAULTS(func);
    PyObject **d = NULL;
    int nd = 0;

    PCALL(PCALL_FUNCTION);
    PCALL(PCALL_FAST_FUNCTION);
    if (argdefs == NULL && co->co_argcount == n &&
        co->co_kwonlyargcount == 0 && nk==0 &&
        co->co_flags == (CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE)) {
        PyFrameObject *f;
        PyObject *retval = NULL;
        PyThreadState *tstate = PyThreadState_GET();
        PyObject **fastlocals, **stack;
        int i;

        PCALL(PCALL_FASTER_FUNCTION);
        assert(globals != NULL);
        /* XXX Perhaps we should create a specialized
           PyFrame_New() that doesn't take locals, but does
           take builtins without sanity checking them.
        */
        assert(tstate != NULL);
        f = PyFrame_New(tstate, co, globals, NULL);
        if (f == NULL)
            return NULL;

        fastlocals = f->f_localsplus;
        stack = (*pp_stack) - n;

        for (i = 0; i < n; i++) {
            Py_INCREF(*stack);
            fastlocals[i] = *stack++;
        }
        retval = PyEval_EvalFrameEx(f,0);
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
        return retval;
    }
    if (argdefs != NULL) {
        d = &PyTuple_GET_ITEM(argdefs, 0);
        nd = Py_SIZE(argdefs);
    }
    return PyEval_EvalCodeEx((PyObject*)co, globals,
                             (PyObject *)NULL, (*pp_stack)-n, na,
                             (*pp_stack)-2*nk, nk, d, nd, kwdefs,
                             PyFunction_GET_CLOSURE(func));
}

static PyObject *
update_keyword_args(PyObject *orig_kwdict, int nk, PyObject ***pp_stack,
                    PyObject *func)
{
    PyObject *kwdict = NULL;
    if (orig_kwdict == NULL)
        kwdict = PyDict_New();
    else {
        kwdict = PyDict_Copy(orig_kwdict);
        Py_DECREF(orig_kwdict);
    }
    if (kwdict == NULL)
        return NULL;
    while (--nk >= 0) {
        int err;
        PyObject *value = EXT_POP(*pp_stack);
        PyObject *key = EXT_POP(*pp_stack);
        if (PyDict_GetItem(kwdict, key) != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s%s got multiple values "
                         "for keyword argument '%U'",
                         PyEval_GetFuncName(func),
                         PyEval_GetFuncDesc(func),
                         key);
            Py_DECREF(key);
            Py_DECREF(value);
            Py_DECREF(kwdict);
            return NULL;
        }
        err = PyDict_SetItem(kwdict, key, value);
        Py_DECREF(key);
        Py_DECREF(value);
        if (err) {
            Py_DECREF(kwdict);
            return NULL;
        }
    }
    return kwdict;
}

static PyObject *
update_star_args(int nstack, int nstar, PyObject *stararg,
                 PyObject ***pp_stack)
{
    PyObject *callargs, *w;

    callargs = PyTuple_New(nstack + nstar);
    if (callargs == NULL) {
        return NULL;
    }
    if (nstar) {
        int i;
        for (i = 0; i < nstar; i++) {
            PyObject *a = PyTuple_GET_ITEM(stararg, i);
            Py_INCREF(a);
            PyTuple_SET_ITEM(callargs, nstack + i, a);
        }
    }
    while (--nstack >= 0) {
        w = EXT_POP(*pp_stack);
        PyTuple_SET_ITEM(callargs, nstack, w);
    }
    return callargs;
}

static PyObject *
load_args(PyObject ***pp_stack, int na)
{
    PyObject *args = PyTuple_New(na);
    PyObject *w;

    if (args == NULL)
        return NULL;
    while (--na >= 0) {
        w = EXT_POP(*pp_stack);
        PyTuple_SET_ITEM(args, na, w);
    }
    return args;
}

static PyObject *
do_call(PyObject *func, PyObject ***pp_stack, int na, int nk)
{
    PyObject *callargs = NULL;
    PyObject *kwdict = NULL;
    PyObject *result = NULL;

    if (nk > 0) {
        kwdict = update_keyword_args(NULL, nk, pp_stack, func);
        if (kwdict == NULL)
            goto call_fail;
    }
    callargs = load_args(pp_stack, na);
    if (callargs == NULL)
        goto call_fail;
#ifdef CALL_PROFILE
    /* At this point, we have to look at the type of func to
       update the call stats properly.  Do it here so as to avoid
       exposing the call stats machinery outside ceval.c
    */
    if (PyFunction_Check(func))
        PCALL(PCALL_FUNCTION);
    else if (PyMethod_Check(func))
        PCALL(PCALL_METHOD);
    else if (PyType_Check(func))
        PCALL(PCALL_TYPE);
    else if (PyCFunction_Check(func))
        PCALL(PCALL_CFUNCTION);
    else
        PCALL(PCALL_OTHER);
#endif
    if (PyCFunction_Check(func)) {
        PyThreadState *tstate = PyThreadState_GET();
        C_TRACE(result, PyCFunction_Call(func, callargs, kwdict));
    }
    else
        result = PyObject_Call(func, callargs, kwdict);
call_fail:
    Py_XDECREF(callargs);
    Py_XDECREF(kwdict);
    return result;
}

static PyObject *
ext_do_call(PyObject *func, PyObject ***pp_stack, int flags, int na, int nk)
{
    int nstar = 0;
    PyObject *callargs = NULL;
    PyObject *stararg = NULL;
    PyObject *kwdict = NULL;
    PyObject *result = NULL;

    if (flags & CALL_FLAG_KW) {
        kwdict = EXT_POP(*pp_stack);
        if (!PyDict_Check(kwdict)) {
            PyObject *d;
            d = PyDict_New();
            if (d == NULL)
                goto ext_call_fail;
            if (PyDict_Update(d, kwdict) != 0) {
                Py_DECREF(d);
                /* PyDict_Update raises attribute
                 * error (percolated from an attempt
                 * to get 'keys' attribute) instead of
                 * a type error if its second argument
                 * is not a mapping.
                 */
                if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    PyErr_Format(PyExc_TypeError,
                                 "%.200s%.200s argument after ** "
                                 "must be a mapping, not %.200s",
                                 PyEval_GetFuncName(func),
                                 PyEval_GetFuncDesc(func),
                                 kwdict->ob_type->tp_name);
                }
                goto ext_call_fail;
            }
            Py_DECREF(kwdict);
            kwdict = d;
        }
    }
    if (flags & CALL_FLAG_VAR) {
        stararg = EXT_POP(*pp_stack);
        if (!PyTuple_Check(stararg)) {
            PyObject *t = NULL;
            t = PySequence_Tuple(stararg);
            if (t == NULL) {
                if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                    PyErr_Format(PyExc_TypeError,
                                 "%.200s%.200s argument after * "
                                 "must be a sequence, not %.200s",
                                 PyEval_GetFuncName(func),
                                 PyEval_GetFuncDesc(func),
                                 stararg->ob_type->tp_name);
                }
                goto ext_call_fail;
            }
            Py_DECREF(stararg);
            stararg = t;
        }
        nstar = PyTuple_GET_SIZE(stararg);
    }
    if (nk > 0) {
        kwdict = update_keyword_args(kwdict, nk, pp_stack, func);
        if (kwdict == NULL)
            goto ext_call_fail;
    }
    callargs = update_star_args(na, nstar, stararg, pp_stack);
    if (callargs == NULL)
        goto ext_call_fail;
#ifdef CALL_PROFILE
    /* At this point, we have to look at the type of func to
       update the call stats properly.  Do it here so as to avoid
       exposing the call stats machinery outside ceval.c
    */
    if (PyFunction_Check(func))
        PCALL(PCALL_FUNCTION);
    else if (PyMethod_Check(func))
        PCALL(PCALL_METHOD);
    else if (PyType_Check(func))
        PCALL(PCALL_TYPE);
    else if (PyCFunction_Check(func))
        PCALL(PCALL_CFUNCTION);
    else
        PCALL(PCALL_OTHER);
#endif
    if (PyCFunction_Check(func)) {
        PyThreadState *tstate = PyThreadState_GET();
        C_TRACE(result, PyCFunction_Call(func, callargs, kwdict));
    }
    else
        result = PyObject_Call(func, callargs, kwdict);
ext_call_fail:
    Py_XDECREF(callargs);
    Py_XDECREF(kwdict);
    Py_XDECREF(stararg);
    return result;
}

/* Extract a slice index from a PyInt or PyLong or an object with the
   nb_index slot defined, and store in *pi.
   Silently reduce values larger than PY_SSIZE_T_MAX to PY_SSIZE_T_MAX,
   and silently boost values less than -PY_SSIZE_T_MAX-1 to -PY_SSIZE_T_MAX-1.
   Return 0 on error, 1 on success.
*/
/* Note:  If v is NULL, return success without storing into *pi.  This
   is because_PyEval_SliceIndex() is called by apply_slice(), which can be
   called by the SLICE opcode with v and/or w equal to NULL.
*/
int
_PyEval_SliceIndex(PyObject *v, Py_ssize_t *pi)
{
    if (v != NULL) {
        Py_ssize_t x;
        if (PyIndex_Check(v)) {
            x = PyNumber_AsSsize_t(v, NULL);
            if (x == -1 && PyErr_Occurred())
                return 0;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "slice indices must be integers or "
                            "None or have an __index__ method");
            return 0;
        }
        *pi = x;
    }
    return 1;
}

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

static PyObject *
cmp_outcome(int op, register PyObject *v, register PyObject *w)
{
    int res = 0;
    switch (op) {
    case PyCmp_IS:
        res = (v == w);
        break;
    case PyCmp_IS_NOT:
        res = (v != w);
        break;
    case PyCmp_IN:
        res = PySequence_Contains(w, v);
        if (res < 0)
            return NULL;
        break;
    case PyCmp_NOT_IN:
        res = PySequence_Contains(w, v);
        if (res < 0)
            return NULL;
        res = !res;
        break;
    case PyCmp_EXC_MATCH:
        if (PyTuple_Check(w)) {
            Py_ssize_t i, length;
            length = PyTuple_Size(w);
            for (i = 0; i < length; i += 1) {
                PyObject *exc = PyTuple_GET_ITEM(w, i);
                if (!PyExceptionClass_Check(exc)) {
                    PyErr_SetString(PyExc_TypeError,
                                    CANNOT_CATCH_MSG);
                    return NULL;
                }
            }
        }
        else {
            if (!PyExceptionClass_Check(w)) {
                PyErr_SetString(PyExc_TypeError,
                                CANNOT_CATCH_MSG);
                return NULL;
            }
        }
        res = PyErr_GivenExceptionMatches(v, w);
        break;
    default:
        return PyObject_RichCompare(v, w, op);
    }
    v = res ? Py_True : Py_False;
    Py_INCREF(v);
    return v;
}

static PyObject *
import_from(PyObject *v, PyObject *name)
{
    PyObject *x;

    x = PyObject_GetAttr(v, name);
    if (x == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
        PyErr_Format(PyExc_ImportError, "cannot import name %S", name);
    }
    return x;
}

static int
import_all_from(PyObject *locals, PyObject *v)
{
    _Py_IDENTIFIER(__all__);
    _Py_IDENTIFIER(__dict__);
    PyObject *all = _PyObject_GetAttrId(v, &PyId___all__);
    PyObject *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (all == NULL) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
            return -1; /* Unexpected error */
        PyErr_Clear();
        dict = _PyObject_GetAttrId(v, &PyId___dict__);
        if (dict == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError))
                return -1;
            PyErr_SetString(PyExc_ImportError,
            "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0; ; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_IndexError))
                err = -1;
            else
                PyErr_Clear();
            break;
        }
        if (skip_leading_underscores &&
            PyUnicode_Check(name) &&
            PyUnicode_READY(name) != -1 &&
            PyUnicode_READ_CHAR(name, 0) == '_')
        {
            Py_DECREF(name);
            continue;
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetItem(locals, name, value);
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err != 0)
            break;
    }
    Py_DECREF(all);
    return err;
}

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj)
{
    const char *obj_str;

    if (!obj)
        return;

    obj_str = _PyUnicode_AsString(obj);
    if (!obj_str)
        return;

    PyErr_Format(exc, format_str, obj_str);
}

static void
format_exc_unbound(PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (PyErr_Occurred())
        return;
    if (oparg < PyTuple_GET_SIZE(co->co_cellvars)) {
        name = PyTuple_GET_ITEM(co->co_cellvars,
                                oparg);
        format_exc_check_arg(
            PyExc_UnboundLocalError,
            UNBOUNDLOCAL_ERROR_MSG,
            name);
    } else {
        name = PyTuple_GET_ITEM(co->co_freevars, oparg -
                                PyTuple_GET_SIZE(co->co_cellvars));
        format_exc_check_arg(PyExc_NameError,
                             UNBOUNDFREE_ERROR_MSG, name);
    }
}

static PyObject *
unicode_concatenate(PyObject *v, PyObject *w,
                    PyFrameObject *f, unsigned char *next_instr)
{
    PyObject *res;
    if (Py_REFCNT(v) == 2) {
        /* In the common case, there are 2 references to the value
         * stored in 'variable' when the += is performed: one on the
         * value stack (in 'v') and one still stored in the
         * 'variable'.  We try to delete the variable now to reduce
         * the refcnt to 1.
         */
        switch (*next_instr) {
        case STORE_FAST:
        {
            int oparg = PEEKARG();
            PyObject **fastlocals = f->f_localsplus;
            if (GETLOCAL(oparg) == v)
                SETLOCAL(oparg, NULL);
            break;
        }
        case STORE_DEREF:
        {
            PyObject **freevars = (f->f_localsplus +
                                   f->f_code->co_nlocals);
            PyObject *c = freevars[PEEKARG()];
            if (PyCell_GET(c) == v)
                PyCell_Set(c, NULL);
            break;
        }
        case STORE_NAME:
        {
            PyObject *names = f->f_code->co_names;
            PyObject *name = GETITEM(names, PEEKARG());
            PyObject *locals = f->f_locals;
            if (PyDict_CheckExact(locals) &&
                PyDict_GetItem(locals, name) == v) {
                if (PyDict_DelItem(locals, name) != 0) {
                    PyErr_Clear();
                }
            }
            break;
        }
        }
    }
    res = v;
    PyUnicode_Append(&res, w);
    return res;
}

#ifdef DYNAMIC_EXECUTION_PROFILE

static PyObject *
getarray(long a[256])
{
    int i;
    PyObject *l = PyList_New(256);
    if (l == NULL) return NULL;
    for (i = 0; i < 256; i++) {
        PyObject *x = PyLong_FromLong(a[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SetItem(l, i, x);
    }
    for (i = 0; i < 256; i++)
        a[i] = 0;
    return l;
}

PyObject *
_Py_GetDXProfile(PyObject *self, PyObject *args)
{
#ifndef DXPAIRS
    return getarray(dxp);
#else
    int i;
    PyObject *l = PyList_New(257);
    if (l == NULL) return NULL;
    for (i = 0; i < 257; i++) {
        PyObject *x = getarray(dxpairs[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SetItem(l, i, x);
    }
    return l;
#endif
}

#endif
