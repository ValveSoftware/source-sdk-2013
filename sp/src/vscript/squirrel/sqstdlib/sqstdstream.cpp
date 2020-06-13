/* see copyright notice in squirrel.h */
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

#define SETUP_STREAM(v) \
    SQStream *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)((SQUnsignedInteger)SQSTD_STREAM_TYPE_TAG)))) \
        return sq_throwerror(v,_SC("invalid type tag")); \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,_SC("the stream is invalid"));

SQInteger _stream_readblob(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQUserPointer data,blobp;
    SQInteger size,res;
    sq_getinteger(v,2,&size);
    if(size > self->Len()) {
        size = self->Len();
    }
    data = sq_getscratchpad(v,size);
    res = self->Read(data,size);
    if(res <= 0)
        return sq_throwerror(v,_SC("no data left to read"));
    blobp = sqstd_createblob(v,res);
    memcpy(blobp,data,res);
    return 1;
}

#define SAFE_READN(ptr,len) { \
    if(self->Read(ptr,len) != len) return sq_throwerror(v,_SC("io error")); \
    }
SQInteger _stream_readn(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger format;
    sq_getinteger(v, 2, &format);
    switch(format) {
    case 'l': {
        SQInteger i;
        SAFE_READN(&i, sizeof(i));
        sq_pushinteger(v, i);
              }
        break;
    case 'i': {
        SQInt32 i;
        SAFE_READN(&i, sizeof(i));
        sq_pushinteger(v, i);
              }
        break;
    case 's': {
        short s;
        SAFE_READN(&s, sizeof(short));
        sq_pushinteger(v, s);
              }
        break;
    case 'w': {
        unsigned short w;
        SAFE_READN(&w, sizeof(unsigned short));
        sq_pushinteger(v, w);
              }
        break;
    case 'c': {
        char c;
        SAFE_READN(&c, sizeof(char));
        sq_pushinteger(v, c);
              }
        break;
    case 'b': {
        unsigned char c;
        SAFE_READN(&c, sizeof(unsigned char));
        sq_pushinteger(v, c);
              }
        break;
    case 'f': {
        float f;
        SAFE_READN(&f, sizeof(float));
        sq_pushfloat(v, f);
              }
        break;
    case 'd': {
        double d;
        SAFE_READN(&d, sizeof(double));
        sq_pushfloat(v, (SQFloat)d);
              }
        break;
    default:
        return sq_throwerror(v, _SC("invalid format"));
    }
    return 1;
}

SQInteger _stream_writeblob(HSQUIRRELVM v)
{
    SQUserPointer data;
    SQInteger size;
    SETUP_STREAM(v);
    if(SQ_FAILED(sqstd_getblob(v,2,&data)))
        return sq_throwerror(v,_SC("invalid parameter"));
    size = sqstd_getblobsize(v,2);
    if(self->Write(data,size) != size)
        return sq_throwerror(v,_SC("io error"));
    sq_pushinteger(v,size);
    return 1;
}

SQInteger _stream_writen(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger format, ti;
    SQFloat tf;
    sq_getinteger(v, 3, &format);
    switch(format) {
    case 'l': {
        SQInteger i;
        sq_getinteger(v, 2, &ti);
        i = ti;
        self->Write(&i, sizeof(SQInteger));
              }
        break;
    case 'i': {
        SQInt32 i;
        sq_getinteger(v, 2, &ti);
        i = (SQInt32)ti;
        self->Write(&i, sizeof(SQInt32));
              }
        break;
    case 's': {
        short s;
        sq_getinteger(v, 2, &ti);
        s = (short)ti;
        self->Write(&s, sizeof(short));
              }
        break;
    case 'w': {
        unsigned short w;
        sq_getinteger(v, 2, &ti);
        w = (unsigned short)ti;
        self->Write(&w, sizeof(unsigned short));
              }
        break;
    case 'c': {
        char c;
        sq_getinteger(v, 2, &ti);
        c = (char)ti;
        self->Write(&c, sizeof(char));
                  }
        break;
    case 'b': {
        unsigned char b;
        sq_getinteger(v, 2, &ti);
        b = (unsigned char)ti;
        self->Write(&b, sizeof(unsigned char));
              }
        break;
    case 'f': {
        float f;
        sq_getfloat(v, 2, &tf);
        f = (float)tf;
        self->Write(&f, sizeof(float));
              }
        break;
    case 'd': {
        double d;
        sq_getfloat(v, 2, &tf);
        d = tf;
        self->Write(&d, sizeof(double));
              }
        break;
    default:
        return sq_throwerror(v, _SC("invalid format"));
    }
    return 0;
}

SQInteger _stream_seek(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    SQInteger offset, origin = SQ_SEEK_SET;
    sq_getinteger(v, 2, &offset);
    if(sq_gettop(v) > 2) {
        SQInteger t;
        sq_getinteger(v, 3, &t);
        switch(t) {
            case 'b': origin = SQ_SEEK_SET; break;
            case 'c': origin = SQ_SEEK_CUR; break;
            case 'e': origin = SQ_SEEK_END; break;
            default: return sq_throwerror(v,_SC("invalid origin"));
        }
    }
    sq_pushinteger(v, self->Seek(offset, origin));
    return 1;
}

SQInteger _stream_tell(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    sq_pushinteger(v, self->Tell());
    return 1;
}

SQInteger _stream_len(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    sq_pushinteger(v, self->Len());
    return 1;
}

SQInteger _stream_flush(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    if(!self->Flush())
        sq_pushinteger(v, 1);
    else
        sq_pushnull(v);
    return 1;
}

SQInteger _stream_eos(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
    if(self->EOS())
        sq_pushinteger(v, 1);
    else
        sq_pushnull(v);
    return 1;
}

 SQInteger _stream__cloned(HSQUIRRELVM v)
 {
     return sq_throwerror(v,_SC("this object cannot be cloned"));
 }

static const SQRegFunction _stream_methods[] = {
    _DECL_STREAM_FUNC(readblob,2,_SC("xn")),
    _DECL_STREAM_FUNC(readn,2,_SC("xn")),
    _DECL_STREAM_FUNC(writeblob,-2,_SC("xx")),
    _DECL_STREAM_FUNC(writen,3,_SC("xnn")),
    _DECL_STREAM_FUNC(seek,-2,_SC("xnn")),
    _DECL_STREAM_FUNC(tell,1,_SC("x")),
    _DECL_STREAM_FUNC(len,1,_SC("x")),
    _DECL_STREAM_FUNC(eos,1,_SC("x")),
    _DECL_STREAM_FUNC(flush,1,_SC("x")),
    _DECL_STREAM_FUNC(_cloned,0,NULL),
    {NULL,(SQFUNCTION)0,0,NULL}
};

void init_streamclass(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);
    sq_pushstring(v,_SC("std_stream"),-1);
    if(SQ_FAILED(sq_get(v,-2))) {
        sq_pushstring(v,_SC("std_stream"),-1);
        sq_newclass(v,SQFalse);
        sq_settypetag(v,-1,(SQUserPointer)((SQUnsignedInteger)SQSTD_STREAM_TYPE_TAG));
        SQInteger i = 0;
        while(_stream_methods[i].name != 0) {
            const SQRegFunction &f = _stream_methods[i];
            sq_pushstring(v,f.name,-1);
            sq_newclosure(v,f.f,0);
            sq_setparamscheck(v,f.nparamscheck,f.typemask);
            sq_newslot(v,-3,SQFalse);
            i++;
        }
        sq_newslot(v,-3,SQFalse);
        sq_pushroottable(v);
        sq_pushstring(v,_SC("stream"),-1);
        sq_pushstring(v,_SC("std_stream"),-1);
        sq_get(v,-4);
        sq_newslot(v,-3,SQFalse);
        sq_pop(v,1);
    }
    else {
        sq_pop(v,1); //result
    }
    sq_pop(v,1);
}

SQRESULT declare_stream(HSQUIRRELVM v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals)
{
    if(sq_gettype(v,-1) != OT_TABLE)
        return sq_throwerror(v,_SC("table expected"));
    SQInteger top = sq_gettop(v);
    //create delegate
    init_streamclass(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,reg_name,-1);
    sq_pushstring(v,_SC("std_stream"),-1);
    if(SQ_SUCCEEDED(sq_get(v,-3))) {
        sq_newclass(v,SQTrue);
        sq_settypetag(v,-1,typetag);
        SQInteger i = 0;
        while(methods[i].name != 0) {
            const SQRegFunction &f = methods[i];
            sq_pushstring(v,f.name,-1);
            sq_newclosure(v,f.f,0);
            sq_setparamscheck(v,f.nparamscheck,f.typemask);
            sq_setnativeclosurename(v,-1,f.name);
            sq_newslot(v,-3,SQFalse);
            i++;
        }
        sq_newslot(v,-3,SQFalse);
        sq_pop(v,1);

        i = 0;
        while(globals[i].name!=0)
        {
            const SQRegFunction &f = globals[i];
            sq_pushstring(v,f.name,-1);
            sq_newclosure(v,f.f,0);
            sq_setparamscheck(v,f.nparamscheck,f.typemask);
            sq_setnativeclosurename(v,-1,f.name);
            sq_newslot(v,-3,SQFalse);
            i++;
        }
        //register the class in the target table
        sq_pushstring(v,name,-1);
        sq_pushregistrytable(v);
        sq_pushstring(v,reg_name,-1);
        sq_get(v,-2);
        sq_remove(v,-2);
        sq_newslot(v,-3,SQFalse);

        sq_settop(v,top);
        return SQ_OK;
    }
    sq_settop(v,top);
    return SQ_ERROR;
}
