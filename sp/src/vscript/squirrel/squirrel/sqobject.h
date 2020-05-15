/*  see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include "squtils.h"

#ifdef _SQ64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct SQSharedState;

enum SQMetaMethod{
    MT_ADD=0,
    MT_SUB=1,
    MT_MUL=2,
    MT_DIV=3,
    MT_UNM=4,
    MT_MODULO=5,
    MT_SET=6,
    MT_GET=7,
    MT_TYPEOF=8,
    MT_NEXTI=9,
    MT_CMP=10,
    MT_CALL=11,
    MT_CLONED=12,
    MT_NEWSLOT=13,
    MT_DELSLOT=14,
    MT_TOSTRING=15,
    MT_NEWMEMBER=16,
    MT_INHERITED=17,
    MT_LAST = 18
};

#define MM_ADD      _SC("_add")
#define MM_SUB      _SC("_sub")
#define MM_MUL      _SC("_mul")
#define MM_DIV      _SC("_div")
#define MM_UNM      _SC("_unm")
#define MM_MODULO   _SC("_modulo")
#define MM_SET      _SC("_set")
#define MM_GET      _SC("_get")
#define MM_TYPEOF   _SC("_typeof")
#define MM_NEXTI    _SC("_nexti")
#define MM_CMP      _SC("_cmp")
#define MM_CALL     _SC("_call")
#define MM_CLONED   _SC("_cloned")
#define MM_NEWSLOT  _SC("_newslot")
#define MM_DELSLOT  _SC("_delslot")
#define MM_TOSTRING _SC("_tostring")
#define MM_NEWMEMBER _SC("_newmember")
#define MM_INHERITED _SC("_inherited")


#define _CONSTRUCT_VECTOR(type,size,ptr) { \
    for(SQInteger n = 0; n < ((SQInteger)size); n++) { \
            new (&ptr[n]) type(); \
        } \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
    for(SQInteger nl = 0; nl < ((SQInteger)size); nl++) { \
            ptr[nl].~type(); \
    } \
}

#define _COPY_VECTOR(dest,src,size) { \
    for(SQInteger _n_ = 0; _n_ < ((SQInteger)size); _n_++) { \
        dest[_n_] = src[_n_]; \
    } \
}

#define _NULL_SQOBJECT_VECTOR(vec,size) { \
    for(SQInteger _n_ = 0; _n_ < ((SQInteger)size); _n_++) { \
        vec[_n_].Null(); \
    } \
}

#define MINPOWER2 4

struct SQRefCounted
{
    SQUnsignedInteger _uiRef;
    struct SQWeakRef *_weakref;
    SQRefCounted() { _uiRef = 0; _weakref = NULL; }
    virtual ~SQRefCounted();
    SQWeakRef *GetWeakRef(SQObjectType type);
    virtual void Release()=0;

};

struct SQWeakRef : SQRefCounted
{
    void Release();
    SQObject _obj;
};

#define _realval(o) (sq_type((o)) != OT_WEAKREF?(SQObject)o:_weakref(o)->_obj)

struct SQObjectPtr;

#define __AddRef(type,unval) if(ISREFCOUNTED(type)) \
        { \
            unval.pRefCounted->_uiRef++; \
        }

#define __Release(type,unval) if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)==0))  \
        {   \
            unval.pRefCounted->Release();   \
        }

#define __ObjRelease(obj) { \
    if((obj)) { \
        (obj)->_uiRef--; \
        if((obj)->_uiRef == 0) \
            (obj)->Release(); \
        (obj) = NULL;   \
    } \
}

#define __ObjAddRef(obj) { \
    (obj)->_uiRef++; \
}

#define is_delegable(t) (sq_type(t)&SQOBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((SQDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _outer(obj) ((obj)._unVal.pOuter)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) ((SQUserPointer)sq_aligning((obj)._unVal.pUserData + 1))

#define tofloat(num) ((sq_type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
#define tointeger(num) ((sq_type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
#define SQ_REFOBJECT_INIT() SQ_OBJECT_RAWINIT()
#else
#define SQ_REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
    SQObjectPtr(_class * x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
        assert(_unVal.pTable); \
        _unVal.pRefCounted->_uiRef++; \
    } \
    inline SQObjectPtr& operator=(_class *x) \
    {  \
        SQObjectType tOldType; \
        SQObjectValue unOldVal; \
        tOldType=_type; \
        unOldVal=_unVal; \
        _type = type; \
        SQ_REFOBJECT_INIT() \
        _unVal.sym = x; \
        _unVal.pRefCounted->_uiRef++; \
        __Release(tOldType,unOldVal); \
        return *this; \
    }

#define _SCALAR_TYPE_DECL(type,_class,sym) \
    SQObjectPtr(_class x) \
    { \
        SQ_OBJECT_RAWINIT() \
        _type=type; \
        _unVal.sym = x; \
    } \
    inline SQObjectPtr& operator=(_class x) \
    {  \
        __Release(_type,_unVal); \
        _type = type; \
        SQ_OBJECT_RAWINIT() \
        _unVal.sym = x; \
        return *this; \
    }
struct SQObjectPtr : public SQObject
{
    SQObjectPtr()
    {
        SQ_OBJECT_RAWINIT()
        _type=OT_NULL;
        _unVal.pUserPointer=NULL;
    }
    SQObjectPtr(const SQObjectPtr &o)
    {
        _type = o._type;
        _unVal = o._unVal;
        __AddRef(_type,_unVal);
    }
    SQObjectPtr(const SQObject &o)
    {
        _type = o._type;
        _unVal = o._unVal;
        __AddRef(_type,_unVal);
    }
    _REF_TYPE_DECL(OT_TABLE,SQTable,pTable)
    _REF_TYPE_DECL(OT_CLASS,SQClass,pClass)
    _REF_TYPE_DECL(OT_INSTANCE,SQInstance,pInstance)
    _REF_TYPE_DECL(OT_ARRAY,SQArray,pArray)
    _REF_TYPE_DECL(OT_CLOSURE,SQClosure,pClosure)
    _REF_TYPE_DECL(OT_NATIVECLOSURE,SQNativeClosure,pNativeClosure)
    _REF_TYPE_DECL(OT_OUTER,SQOuter,pOuter)
    _REF_TYPE_DECL(OT_GENERATOR,SQGenerator,pGenerator)
    _REF_TYPE_DECL(OT_STRING,SQString,pString)
    _REF_TYPE_DECL(OT_USERDATA,SQUserData,pUserData)
    _REF_TYPE_DECL(OT_WEAKREF,SQWeakRef,pWeakRef)
    _REF_TYPE_DECL(OT_THREAD,SQVM,pThread)
    _REF_TYPE_DECL(OT_FUNCPROTO,SQFunctionProto,pFunctionProto)

    _SCALAR_TYPE_DECL(OT_INTEGER,SQInteger,nInteger)
    _SCALAR_TYPE_DECL(OT_FLOAT,SQFloat,fFloat)
    _SCALAR_TYPE_DECL(OT_USERPOINTER,SQUserPointer,pUserPointer)

    SQObjectPtr(bool bBool)
    {
        SQ_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _unVal.nInteger = bBool?1:0;
    }
    inline SQObjectPtr& operator=(bool b)
    {
        __Release(_type,_unVal);
        SQ_OBJECT_RAWINIT()
        _type = OT_BOOL;
        _unVal.nInteger = b?1:0;
        return *this;
    }

    ~SQObjectPtr()
    {
        __Release(_type,_unVal);
    }

    inline SQObjectPtr& operator=(const SQObjectPtr& obj)
    {
        SQObjectType tOldType;
        SQObjectValue unOldVal;
        tOldType=_type;
        unOldVal=_unVal;
        _unVal = obj._unVal;
        _type = obj._type;
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline SQObjectPtr& operator=(const SQObject& obj)
    {
        SQObjectType tOldType;
        SQObjectValue unOldVal;
        tOldType=_type;
        unOldVal=_unVal;
        _unVal = obj._unVal;
        _type = obj._type;
        __AddRef(_type,_unVal);
        __Release(tOldType,unOldVal);
        return *this;
    }
    inline void Null()
    {
        SQObjectType tOldType = _type;
        SQObjectValue unOldVal = _unVal;
        _type = OT_NULL;
        _unVal.raw = (SQRawObjectVal)NULL;
        __Release(tOldType ,unOldVal);
    }
    private:
        SQObjectPtr(const SQChar *){} //safety
};


inline void _Swap(SQObject &a,SQObject &b)
{
    SQObjectType tOldType = a._type;
    SQObjectValue unOldVal = a._unVal;
    a._type = b._type;
    a._unVal = b._unVal;
    b._type = tOldType;
    b._unVal = unOldVal;
}

/////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_GARBAGE_COLLECTOR
#define MARK_FLAG 0x80000000
struct SQCollectable : public SQRefCounted {
    SQCollectable *_next;
    SQCollectable *_prev;
    SQSharedState *_sharedstate;
    virtual SQObjectType GetType()=0;
    virtual void Release()=0;
    virtual void Mark(SQCollectable **chain)=0;
    void UnMark();
    virtual void Finalize()=0;
    static void AddToChain(SQCollectable **chain,SQCollectable *c);
    static void RemoveFromChain(SQCollectable **chain,SQCollectable *c);
};


#define ADD_TO_CHAIN(chain,obj) AddToChain(chain,obj)
#define REMOVE_FROM_CHAIN(chain,obj) {if(!(_uiRef&MARK_FLAG))RemoveFromChain(chain,obj);}
#define CHAINABLE_OBJ SQCollectable
#define INIT_CHAIN() {_next=NULL;_prev=NULL;_sharedstate=ss;}
#else

#define ADD_TO_CHAIN(chain,obj) ((void)0)
#define REMOVE_FROM_CHAIN(chain,obj) ((void)0)
#define CHAINABLE_OBJ SQRefCounted
#define INIT_CHAIN() ((void)0)
#endif

struct SQDelegable : public CHAINABLE_OBJ {
    bool SetDelegate(SQTable *m);
    virtual bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);
    SQTable *_delegate;
};

SQUnsignedInteger TranslateIndex(const SQObjectPtr &idx);
typedef sqvector<SQObjectPtr> SQObjectPtrVec;
typedef sqvector<SQInteger> SQIntVec;
const SQChar *GetTypeName(const SQObjectPtr &obj1);
const SQChar *IdType2Name(SQObjectType type);



#endif //_SQOBJECT_H_
