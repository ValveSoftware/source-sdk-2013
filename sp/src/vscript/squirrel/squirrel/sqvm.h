/*  see copyright notice in squirrel.h */
#ifndef _SQVM_H_
#define _SQVM_H_

#include "sqopcodes.h"
#include "sqobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW                0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002
//base lib
void sq_base_register(HSQUIRRELVM v);

struct SQExceptionTrap{
    SQExceptionTrap() {}
    SQExceptionTrap(SQInteger ss, SQInteger stackbase,SQInstruction *ip, SQInteger ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
    SQExceptionTrap(const SQExceptionTrap &et) { (*this) = et;  }
    SQInteger _stackbase;
    SQInteger _stacksize;
    SQInstruction *_ip;
    SQInteger _extarget;
};

#define _INLINE

typedef sqvector<SQExceptionTrap> ExceptionsTraps;

struct SQVM : public CHAINABLE_OBJ
{
    struct CallInfo{
        //CallInfo() { _generator = NULL;}
        SQInstruction *_ip;
        SQObjectPtr *_literals;
        SQObjectPtr _closure;
        SQGenerator *_generator;
        SQInt32 _etraps;
        SQInt32 _prevstkbase;
        SQInt32 _prevtop;
        SQInt32 _target;
        SQInt32 _ncalls;
        SQBool _root;
    };

typedef sqvector<CallInfo> CallInfoVec;
public:
    void DebugHookProxy(SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname);
    static void _DebugHookProxy(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname);
    enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM,ET_RESUME_THROW_VM };
    SQVM(SQSharedState *ss);
    ~SQVM();
    bool Init(SQVM *friendvm, SQInteger stacksize);
    bool Execute(SQObjectPtr &func, SQInteger nargs, SQInteger stackbase, SQObjectPtr &outres, SQBool raiseerror, ExecutionType et = ET_CALL);
    //starts a native call return when the NATIVE closure returns
    bool CallNative(SQNativeClosure *nclosure, SQInteger nargs, SQInteger newbase, SQObjectPtr &retval, SQInt32 target, bool &suspend,bool &tailcall);
	bool TailCall(SQClosure *closure, SQInteger firstparam, SQInteger nparams);
    //starts a SQUIRREL call in the same "Execution loop"
    bool StartCall(SQClosure *closure, SQInteger target, SQInteger nargs, SQInteger stackbase, bool tailcall);
    bool CreateClassInstance(SQClass *theclass, SQObjectPtr &inst, SQObjectPtr &constructor);
    //call a generic closure pure SQUIRREL or NATIVE
    bool Call(SQObjectPtr &closure, SQInteger nparams, SQInteger stackbase, SQObjectPtr &outres,SQBool raiseerror);
    SQRESULT Suspend();

    void CallDebugHook(SQInteger type,SQInteger forcedline=0);
    void CallErrorHandler(SQObjectPtr &e);
    bool Get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, SQUnsignedInteger getflags, SQInteger selfidx);
    SQInteger FallBackGet(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
    bool InvokeDefaultDelegate(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
    bool Set(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val, SQInteger selfidx);
    SQInteger FallBackSet(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
    bool NewSlot(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val,bool bstatic);
    bool NewSlotA(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,const SQObjectPtr &attrs,bool bstatic,bool raw);
    bool DeleteSlot(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &res);
    bool Clone(const SQObjectPtr &self, SQObjectPtr &target);
    bool ObjCmp(const SQObjectPtr &o1, const SQObjectPtr &o2,SQInteger &res);
    bool StringCat(const SQObjectPtr &str, const SQObjectPtr &obj, SQObjectPtr &dest);
    static bool IsEqual(const SQObjectPtr &o1,const SQObjectPtr &o2,bool &res);
    bool ToString(const SQObjectPtr &o,SQObjectPtr &res);
    SQString *PrintObjVal(const SQObjectPtr &o);


    void Raise_Error(const SQChar *s, ...);
    void Raise_Error(const SQObjectPtr &desc);
    void Raise_IdxError(const SQObjectPtr &o);
    void Raise_CompareError(const SQObject &o1, const SQObject &o2);
    void Raise_ParamTypeError(SQInteger nparam,SQInteger typemask,SQInteger type);

    void FindOuter(SQObjectPtr &target, SQObjectPtr *stackindex);
    void RelocateOuters();
    void CloseOuters(SQObjectPtr *stackindex);

    bool TypeOf(const SQObjectPtr &obj1, SQObjectPtr &dest);
    bool CallMetaMethod(SQObjectPtr &closure, SQMetaMethod mm, SQInteger nparams, SQObjectPtr &outres);
    bool ArithMetaMethod(SQInteger op, const SQObjectPtr &o1, const SQObjectPtr &o2, SQObjectPtr &dest);
    bool Return(SQInteger _arg0, SQInteger _arg1, SQObjectPtr &retval);
    //new stuff
    _INLINE bool ARITH_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
    _INLINE bool BW_OP(SQUnsignedInteger op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
    _INLINE bool NEG_OP(SQObjectPtr &trg,const SQObjectPtr &o1);
    _INLINE bool CMP_OP(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &res);
    bool CLOSURE_OP(SQObjectPtr &target, SQFunctionProto *func);
    bool CLASS_OP(SQObjectPtr &target,SQInteger base,SQInteger attrs);
    //return true if the loop is finished
    bool FOREACH_OP(SQObjectPtr &o1,SQObjectPtr &o2,SQObjectPtr &o3,SQObjectPtr &o4,SQInteger arg_2,int exitpos,int &jump);
    //_INLINE bool LOCAL_INC(SQInteger op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
    _INLINE bool PLOCAL_INC(SQInteger op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
    _INLINE bool DerefInc(SQInteger op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix,SQInteger arg0);
#ifdef _DEBUG_DUMP
    void dumpstack(SQInteger stackbase=-1, bool dumpall = false);
#endif

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    SQObjectType GetType() {return OT_THREAD;}
#endif
    void Finalize();
    void GrowCallStack() {
        SQInteger newsize = _alloccallsstacksize*2;
        _callstackdata.resize(newsize);
        _callsstack = &_callstackdata[0];
        _alloccallsstacksize = newsize;
    }
    bool EnterFrame(SQInteger newbase, SQInteger newtop, bool tailcall);
    void LeaveFrame();
    void Release(){ sq_delete(this,SQVM); }
////////////////////////////////////////////////////////////////////////////
    //stack functions for the api
    void Remove(SQInteger n);

    static bool IsFalse(SQObjectPtr &o);

    void Pop();
    void Pop(SQInteger n);
    void Push(const SQObjectPtr &o);
    void PushNull();
    SQObjectPtr &Top();
    SQObjectPtr &PopGet();
    SQObjectPtr &GetUp(SQInteger n);
    SQObjectPtr &GetAt(SQInteger n);

    SQObjectPtrVec _stack;

    SQInteger _top;
    SQInteger _stackbase;
    SQOuter *_openouters;
    SQObjectPtr _roottable;
    SQObjectPtr _lasterror;
    SQObjectPtr _errorhandler;

    bool _debughook;
    SQDEBUGHOOK _debughook_native;
    SQObjectPtr _debughook_closure;

    SQObjectPtr temp_reg;


    CallInfo* _callsstack;
    SQInteger _callsstacksize;
    SQInteger _alloccallsstacksize;
    sqvector<CallInfo>  _callstackdata;

    ExceptionsTraps _etraps;
    CallInfo *ci;
    SQUserPointer _foreignptr;
    //VMs sharing the same state
    SQSharedState *_sharedstate;
    SQInteger _nnativecalls;
    SQInteger _nmetamethodscall;
    SQRELEASEHOOK _releasehook;
    //suspend infos
    SQBool _suspended;
    SQBool _suspended_root;
    SQInteger _suspended_target;
    SQInteger _suspended_traps;
};

struct AutoDec{
    AutoDec(SQInteger *n) { _n = n; }
    ~AutoDec() { (*_n)--; }
    SQInteger *_n;
};

inline SQObjectPtr &stack_get(HSQUIRRELVM v,SQInteger idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

#define _ss(_vm_) (_vm_)->_sharedstate

#ifndef NO_GARBAGE_COLLECTOR
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
#define _opt_ss(_vm_) NULL
#endif

#define PUSH_CALLINFO(v,nci){ \
    SQInteger css = v->_callsstacksize; \
    if(css == v->_alloccallsstacksize) { \
        v->GrowCallStack(); \
    } \
    v->ci = &v->_callsstack[css]; \
    *(v->ci) = nci; \
    v->_callsstacksize++; \
}

#define POP_CALLINFO(v){ \
    SQInteger css = --v->_callsstacksize; \
    v->ci->_closure.Null(); \
    v->ci = css?&v->_callsstack[css-1]:NULL;    \
}
#endif //_SQVM_H_
