.. _threads:


========================
Threads
========================

.. index::
    single: Threads

Squirrel supports cooperative threads(also known as coroutines).
A cooperative thread is a subroutine that can suspended in mid-execution and provide a value to the
caller without returning program flow, then its execution can be resumed later from the same
point where it was suspended.
At first look a Squirrel thread can be confused with a generator, in fact their behaviour is quite similar.
However while a generator runs in the caller stack and can suspend only the local routine stack a thread
has its own execution stack, global table and error handler; This allows a thread to suspend nested calls and
have it's own error policies.

------------------
Using threads
------------------

.. index::
    single: Using Threads

Threads are created through the built-in function 'newthread(func)'; this function
gets as parameter a squirrel function and bind it to the new thread objects (will be the thread body).
The returned thread object is initially in 'idle' state. the thread can be started with the function
'threadobj.call()'; the parameters passed to 'call' are passed to the thread function.

A thread can be be suspended calling the function suspend(), when this happens the function
that wokeup(or started) the thread returns (If a parameter is passed to suspend() it will
be the return value of the wakeup function , if no parameter is passed the return value will be null).
A suspended thread can be resumed calling the function 'threadobj.wakeup', when this happens
the function that suspended the thread will return(if a parameter is passed to wakeup it will
be the return value of the suspend function, if no parameter is passed the return value will be null).

A thread terminates when its main function returns or when an unhandled exception occurs during its execution.::

    function coroutine_test(a,b)
    {
        ::print(a+" "+b+"\n");
        local ret = ::suspend("suspend 1");
        ::print("the coroutine says "+ret+"\n");
        ret = ::suspend("suspend 2");
        ::print("the coroutine says "+ret+"\n");
        ret = ::suspend("suspend 3");
        ::print("the coroutine says "+ret+"\n");
        return "I'm done"
    }

    local coro = ::newthread(coroutine_test);

    local susparam = coro.call("test","coroutine"); //starts the coroutine

    local i = 1;
    do
    {
        ::print("suspend passed ("+susparam+")\n")
        susparam = coro.wakeup("ciao "+i);
        ++i;
    }while(coro.getstatus()=="suspended")

    ::print("return passed ("+susparam+")\n")

the result of this program will be::

    test coroutine
    suspend passed (suspend 1)
    the coroutine says ciao 1
    suspend passed (suspend 2)
    the coroutine says ciao 2
    suspend passed (suspend 3)
    the coroutine says ciao 3
    return passed (I'm done).


the following is an interesting example of how threads and tail recursion
can be combined.::

    function state1()
    {
        ::suspend("state1");
        return state2(); //tail call
    }

    function state2()
    {
        ::suspend("state2");
        return state3(); //tail call
    }

    function state3()
    {
        ::suspend("state3");
        return state1(); //tail call
    }

    local statethread = ::newthread(state1)

    ::print(statethread.call()+"\n");

    for(local i = 0; i < 10000; i++)
        ::print(statethread.wakeup()+"\n");

