/* see copyright notice in squirrel.h */
#include <new>
#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

#define SQSTD_BLOB_TYPE_TAG ((SQUnsignedInteger)(SQSTD_STREAM_TYPE_TAG | 0x00000002))

//Blob


#define SETUP_BLOB(v) \
    SQBlob *self = NULL; \
    { if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));  } \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,_SC("the blob is invalid"));


static SQInteger _blob_resize(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger size;
    sq_getinteger(v,2,&size);
    if(!self->Resize(size))
        return sq_throwerror(v,_SC("resize failed"));
    return 0;
}

static void __swap_dword(unsigned int *n)
{
    *n=(unsigned int)(((*n&0xFF000000)>>24)  |
            ((*n&0x00FF0000)>>8)  |
            ((*n&0x0000FF00)<<8)  |
            ((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
    *n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static SQInteger _blob_swap4(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger num=(self->Len()-(self->Len()%4))>>2;
    unsigned int *t=(unsigned int *)self->GetBuf();
    for(SQInteger i = 0; i < num; i++) {
        __swap_dword(&t[i]);
    }
    return 0;
}

static SQInteger _blob_swap2(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger num=(self->Len()-(self->Len()%2))>>1;
    unsigned short *t = (unsigned short *)self->GetBuf();
    for(SQInteger i = 0; i < num; i++) {
        __swap_word(&t[i]);
    }
    return 0;
}

static SQInteger _blob__set(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger idx,val;
    sq_getinteger(v,2,&idx);
    sq_getinteger(v,3,&val);
    if(idx < 0 || idx >= self->Len())
        return sq_throwerror(v,_SC("index out of range"));
    ((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
    sq_push(v,3);
    return 1;
}

static SQInteger _blob__get(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    SQInteger idx;
	
	if ((sq_gettype(v, 2) & SQOBJECT_NUMERIC) == 0)
	{
		sq_pushnull(v);
		return sq_throwobject(v);
	}
    sq_getinteger(v,2,&idx);
    if(idx < 0 || idx >= self->Len())
        return sq_throwerror(v,_SC("index out of range"));
    sq_pushinteger(v,((unsigned char *)self->GetBuf())[idx]);
    return 1;
}

static SQInteger _blob__nexti(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    if(sq_gettype(v,2) == OT_NULL) {
        sq_pushinteger(v, 0);
        return 1;
    }
    SQInteger idx;
    if(SQ_SUCCEEDED(sq_getinteger(v, 2, &idx))) {
        if(idx+1 < self->Len()) {
            sq_pushinteger(v, idx+1);
            return 1;
        }
        sq_pushnull(v);
        return 1;
    }
    return sq_throwerror(v,_SC("internal error (_nexti) wrong argument type"));
}

static SQInteger _blob__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("blob"),-1);
    return 1;
}

static SQInteger _blob_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQBlob *self = (SQBlob*)p;
    self->~SQBlob();
    sq_free(self,sizeof(SQBlob));
    return 1;
}

static SQInteger _blob_constructor(HSQUIRRELVM v)
{
    SQInteger nparam = sq_gettop(v);
    SQInteger size = 0;
    if(nparam == 2) {
        sq_getinteger(v, 2, &size);
    }
    if(size < 0) return sq_throwerror(v, _SC("cannot create blob with negative size"));
    //SQBlob *b = new SQBlob(size);

    SQBlob *b = new (sq_malloc(sizeof(SQBlob)))SQBlob(size);
    if(SQ_FAILED(sq_setinstanceup(v,1,b))) {
        b->~SQBlob();
        sq_free(b,sizeof(SQBlob));
        return sq_throwerror(v, _SC("cannot create blob"));
    }
    sq_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

static SQInteger _blob__cloned(HSQUIRRELVM v)
{
    SQBlob *other = NULL;
    {
        if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&other,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
            return SQ_ERROR;
    }
    //SQBlob *thisone = new SQBlob(other->Len());
    SQBlob *thisone = new (sq_malloc(sizeof(SQBlob)))SQBlob(other->Len());
    memcpy(thisone->GetBuf(),other->GetBuf(),thisone->Len());
    if(SQ_FAILED(sq_setinstanceup(v,1,thisone))) {
        thisone->~SQBlob();
        sq_free(thisone,sizeof(SQBlob));
        return sq_throwerror(v, _SC("cannot clone blob"));
    }
    sq_setreleasehook(v,1,_blob_releasehook);
    return 0;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_SC(#name),_blob_##name,nparams,typecheck}
static const SQRegFunction _blob_methods[] = {
    _DECL_BLOB_FUNC(constructor,-1,_SC("xn")),
    _DECL_BLOB_FUNC(resize,2,_SC("xn")),
    _DECL_BLOB_FUNC(swap2,1,_SC("x")),
    _DECL_BLOB_FUNC(swap4,1,_SC("x")),
    _DECL_BLOB_FUNC(_set,3,_SC("xnn")),
    _DECL_BLOB_FUNC(_get,2,_SC("x.")),
    _DECL_BLOB_FUNC(_typeof,1,_SC("x")),
    _DECL_BLOB_FUNC(_nexti,2,_SC("x")),
    _DECL_BLOB_FUNC(_cloned,2,_SC("xx")),
    {NULL,(SQFUNCTION)0,0,NULL}
};



//GLOBAL FUNCTIONS

static SQInteger _g_blob_casti2f(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    sq_pushfloat(v,*((const SQFloat *)&i));
    return 1;
}

static SQInteger _g_blob_castf2i(HSQUIRRELVM v)
{
    SQFloat f;
    sq_getfloat(v,2,&f);
    sq_pushinteger(v,*((const SQInteger *)&f));
    return 1;
}

static SQInteger _g_blob_swap2(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    short s=(short)i;
    sq_pushinteger(v,(s<<8)|((s>>8)&0x00FF));
    return 1;
}

static SQInteger _g_blob_swap4(HSQUIRRELVM v)
{
    SQInteger i;
    sq_getinteger(v,2,&i);
    unsigned int t4 = (unsigned int)i;
    __swap_dword(&t4);
    sq_pushinteger(v,(SQInteger)t4);
    return 1;
}

static SQInteger _g_blob_swapfloat(HSQUIRRELVM v)
{
    SQFloat f;
    sq_getfloat(v,2,&f);
    __swap_dword((unsigned int *)&f);
    sq_pushfloat(v,f);
    return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_SC(#name),_g_blob_##name,nparams,typecheck}
static const SQRegFunction bloblib_funcs[]={
    _DECL_GLOBALBLOB_FUNC(casti2f,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(castf2i,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swap2,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swap4,2,_SC(".n")),
    _DECL_GLOBALBLOB_FUNC(swapfloat,2,_SC(".n")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_getblob(HSQUIRRELVM v,SQInteger idx,SQUserPointer *ptr)
{
    SQBlob *blob;
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
        return -1;
    *ptr = blob->GetBuf();
    return SQ_OK;
}

SQInteger sqstd_getblobsize(HSQUIRRELVM v,SQInteger idx)
{
    SQBlob *blob;
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
        return -1;
    return blob->Len();
}

SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size)
{
    SQInteger top = sq_gettop(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,_SC("std_blob"),-1);
    if(SQ_SUCCEEDED(sq_get(v,-2))) {
        sq_remove(v,-2); //removes the registry
        sq_push(v,1); // push the this
        sq_pushinteger(v,size); //size
        SQBlob *blob = NULL;
        if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQFalse))
            && SQ_SUCCEEDED(sq_getinstanceup(v,-1,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) {
            sq_remove(v,-2);
            return blob->GetBuf();
        }
    }
    sq_settop(v,top);
    return NULL;
}

SQRESULT sqstd_register_bloblib(HSQUIRRELVM v)
{
    return declare_stream(v,_SC("blob"),(SQUserPointer)SQSTD_BLOB_TYPE_TAG,_SC("std_blob"),_blob_methods,bloblib_funcs);
}

