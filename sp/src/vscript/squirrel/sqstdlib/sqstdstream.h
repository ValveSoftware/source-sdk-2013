/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

SQInteger _stream_readblob(HSQUIRRELVM v);
SQInteger _stream_readline(HSQUIRRELVM v);
SQInteger _stream_readn(HSQUIRRELVM v);
SQInteger _stream_writeblob(HSQUIRRELVM v);
SQInteger _stream_writen(HSQUIRRELVM v);
SQInteger _stream_seek(HSQUIRRELVM v);
SQInteger _stream_tell(HSQUIRRELVM v);
SQInteger _stream_len(HSQUIRRELVM v);
SQInteger _stream_eos(HSQUIRRELVM v);
SQInteger _stream_flush(HSQUIRRELVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(HSQUIRRELVM v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals);
#endif /*_SQSTD_STREAM_H_*/
