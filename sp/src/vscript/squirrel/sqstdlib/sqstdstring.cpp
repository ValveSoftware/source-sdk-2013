/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <sqstdstring.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(SQChar))

static SQUserPointer rex_typetag = NULL;

static SQBool isfmtchr(SQChar ch)
{
    switch(ch) {
    case '-': case '+': case ' ': case '#': case '0': return SQTrue;
    }
    return SQFalse;
}

static SQInteger validate_format(HSQUIRRELVM v, SQChar *fmt, const SQChar *src, SQInteger n,SQInteger &width)
{
    SQChar *dummy;
    SQChar swidth[MAX_WFORMAT_LEN];
    SQInteger wc = 0;
    SQInteger start = n;
    fmt[0] = '%';
    while (isfmtchr(src[n])) n++;
    while (scisdigit(src[n])) {
        swidth[wc] = src[n];
        n++;
        wc++;
        if(wc>=MAX_WFORMAT_LEN)
            return sq_throwerror(v,_SC("width format too long"));
    }
    swidth[wc] = '\0';
    if(wc > 0) {
        width = scstrtol(swidth,&dummy,10);
    }
    else
        width = 0;
    if (src[n] == '.') {
        n++;

        wc = 0;
        while (scisdigit(src[n])) {
            swidth[wc] = src[n];
            n++;
            wc++;
            if(wc>=MAX_WFORMAT_LEN)
                return sq_throwerror(v,_SC("precision format too long"));
        }
        swidth[wc] = '\0';
        if(wc > 0) {
            width += scstrtol(swidth,&dummy,10);

        }
    }
    if (n-start > MAX_FORMAT_LEN )
        return sq_throwerror(v,_SC("format too long"));
    memcpy(&fmt[1],&src[start],((n-start)+1)*sizeof(SQChar));
    fmt[(n-start)+2] = '\0';
    return n;
}

SQRESULT sqstd_format(HSQUIRRELVM v,SQInteger nformatstringidx,SQInteger *outlen,SQChar **output)
{
    const SQChar *format;
    SQChar *dest;
    SQChar fmt[MAX_FORMAT_LEN];
    const SQRESULT res = sq_getstring(v,nformatstringidx,&format);
    if (SQ_FAILED(res)) {
        return res; // propagate the error
    }
    SQInteger format_size = sq_getsize(v,nformatstringidx);
    SQInteger allocated = (format_size+2)*sizeof(SQChar);
    dest = sq_getscratchpad(v,allocated);
    SQInteger n = 0,i = 0, nparam = nformatstringidx+1, w = 0;
    //while(format[n] != '\0')
    while(n < format_size)
    {
        if(format[n] != '%') {
            assert(i < allocated);
            dest[i++] = format[n];
            n++;
        }
        else if(format[n+1] == '%') { //handles %%
                dest[i++] = '%';
                n += 2;
        }
        else {
            n++;
            if( nparam > sq_gettop(v) )
                return sq_throwerror(v,_SC("not enough parameters for the given format string"));
            n = validate_format(v,fmt,format,n,w);
            if(n < 0) return -1;
            SQInteger addlen = 0;
            SQInteger valtype = 0;
            const SQChar *ts = NULL;
            SQInteger ti = 0;
            SQFloat tf = 0;
            switch(format[n]) {
            case 's':
                if(SQ_FAILED(sq_getstring(v,nparam,&ts)))
                    return sq_throwerror(v,_SC("string expected for the specified format"));
                addlen = (sq_getsize(v,nparam)*sizeof(SQChar))+((w+1)*sizeof(SQChar));
                valtype = 's';
                break;
            case 'i': case 'd': case 'o': case 'u':  case 'x':  case 'X':
#ifdef _SQ64
                {
                size_t flen = scstrlen(fmt);
                SQInteger fpos = flen - 1;
                SQChar f = fmt[fpos];
                const SQChar *prec = (const SQChar *)_PRINT_INT_PREC;
                while(*prec != _SC('\0')) {
                    fmt[fpos++] = *prec++;
                }
                fmt[fpos++] = f;
                fmt[fpos++] = _SC('\0');
                }
#endif
            case 'c':
                if(SQ_FAILED(sq_getinteger(v,nparam,&ti)))
                    return sq_throwerror(v,_SC("integer expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(SQChar));
                valtype = 'i';
                break;
            case 'f': case 'g': case 'G': case 'e':  case 'E':
                if(SQ_FAILED(sq_getfloat(v,nparam,&tf)))
                    return sq_throwerror(v,_SC("float expected for the specified format"));
                addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(SQChar));
                valtype = 'f';
                break;
            default:
                return sq_throwerror(v,_SC("invalid format"));
            }
            n++;
            allocated += addlen + sizeof(SQChar);
            dest = sq_getscratchpad(v,allocated);
            switch(valtype) {
            case 's': i += scsprintf(&dest[i],allocated,fmt,ts); break;
            case 'i': i += scsprintf(&dest[i],allocated,fmt,ti); break;
            case 'f': i += scsprintf(&dest[i],allocated,fmt,tf); break;
            };
            nparam ++;
        }
    }
    *outlen = i;
    dest[i] = '\0';
    *output = dest;
    return SQ_OK;
}

void sqstd_pushstringf(HSQUIRRELVM v,const SQChar *s,...)
{
    SQInteger n=256;
    va_list args;
begin:
    va_start(args,s);
    SQChar *b=sq_getscratchpad(v,n);
    SQInteger r=scvsprintf(b,n,s,args);
    va_end(args);
    if (r>=n) {
        n=r+1;//required+null
        goto begin;
    } else if (r<0) {
        sq_pushnull(v);
    } else {
        sq_pushstring(v,b,r);
    }
}

static SQInteger _string_printf(HSQUIRRELVM v)
{
    SQChar *dest = NULL;
    SQInteger length = 0;
    if(SQ_FAILED(sqstd_format(v,2,&length,&dest)))
        return -1;

    SQPRINTFUNCTION printfunc = sq_getprintfunc(v);
    if(printfunc) printfunc(v,_SC("%s"),dest);

    return 0;
}

static SQInteger _string_format(HSQUIRRELVM v)
{
    SQChar *dest = NULL;
    SQInteger length = 0;
    if(SQ_FAILED(sqstd_format(v,2,&length,&dest)))
        return -1;
    sq_pushstring(v,dest,length);
    return 1;
}

static void __strip_l(const SQChar *str,const SQChar **start)
{
    const SQChar *t = str;
    while(((*t) != '\0') && scisspace(*t)){ t++; }
    *start = t;
}

static void __strip_r(const SQChar *str,SQInteger len,const SQChar **end)
{
    if(len == 0) {
        *end = str;
        return;
    }
    const SQChar *t = &str[len-1];
    while(t >= str && scisspace(*t)) { t--; }
    *end = t + 1;
}

static SQInteger _string_strip(HSQUIRRELVM v)
{
    const SQChar *str,*start,*end;
    sq_getstring(v,2,&str);
    SQInteger len = sq_getsize(v,2);
    __strip_l(str,&start);
    __strip_r(str,len,&end);
    sq_pushstring(v,start,end - start);
    return 1;
}

static SQInteger _string_lstrip(HSQUIRRELVM v)
{
    const SQChar *str,*start;
    sq_getstring(v,2,&str);
    __strip_l(str,&start);
    sq_pushstring(v,start,-1);
    return 1;
}

static SQInteger _string_rstrip(HSQUIRRELVM v)
{
    const SQChar *str,*end;
    sq_getstring(v,2,&str);
    SQInteger len = sq_getsize(v,2);
    __strip_r(str,len,&end);
    sq_pushstring(v,str,end - str);
    return 1;
}

static SQInteger _string_split(HSQUIRRELVM v)
{
    const SQChar *str,*seps;
    SQInteger sepsize;
    SQBool skipempty = SQFalse;
    sq_getstring(v,2,&str);
    sq_getstringandsize(v,3,&seps,&sepsize);
    if(sepsize == 0) return sq_throwerror(v,_SC("empty separators string"));
    if(sq_gettop(v)>3) {
        sq_getbool(v,4,&skipempty);
    }
    const SQChar *start = str;
    const SQChar *end = str;
    sq_newarray(v,0);
    while(*end != '\0')
    {
        SQChar cur = *end;
        for(SQInteger i = 0; i < sepsize; i++)
        {
            if(cur == seps[i])
            {
                if(!skipempty || (end != start)) {
                    sq_pushstring(v,start,end-start);
                    sq_arrayappend(v,-2);
                }
                start = end + 1;
                break;
            }
        }
        end++;
    }
    if(end != start)
    {
        sq_pushstring(v,start,end-start);
        sq_arrayappend(v,-2);
    }
    return 1;
}

static SQInteger _string_escape(HSQUIRRELVM v)
{
    const SQChar *str;
    SQChar *dest,*resstr;
    SQInteger size;
    sq_getstring(v,2,&str);
    size = sq_getsize(v,2);
    if(size == 0) {
        sq_push(v,2);
        return 1;
    }
#ifdef SQUNICODE
#if WCHAR_SIZE == 2
    const SQChar *escpat = _SC("\\x%04x");
    const SQInteger maxescsize = 6;
#else //WCHAR_SIZE == 4
    const SQChar *escpat = _SC("\\x%08x");
    const SQInteger maxescsize = 10;
#endif
#else
    const SQChar *escpat = _SC("\\x%02x");
    const SQInteger maxescsize = 4;
#endif
    SQInteger destcharsize = (size * maxescsize); //assumes every char could be escaped
    resstr = dest = (SQChar *)sq_getscratchpad(v,destcharsize * sizeof(SQChar));
    SQChar c;
    SQChar escch;
    SQInteger escaped = 0;
    for(int n = 0; n < size; n++){
        c = *str++;
        escch = 0;
        if(scisprint(c) || c == 0) {
            switch(c) {
            case '\a': escch = 'a'; break;
            case '\b': escch = 'b'; break;
            case '\t': escch = 't'; break;
            case '\n': escch = 'n'; break;
            case '\v': escch = 'v'; break;
            case '\f': escch = 'f'; break;
            case '\r': escch = 'r'; break;
            case '\\': escch = '\\'; break;
            case '\"': escch = '\"'; break;
            case '\'': escch = '\''; break;
            case 0: escch = '0'; break;
            }
            if(escch) {
                *dest++ = '\\';
                *dest++ = escch;
                escaped++;
            }
            else {
                *dest++ = c;
            }
        }
        else {

            dest += scsprintf(dest, destcharsize, escpat, c);
            escaped++;
        }
    }

    if(escaped) {
        sq_pushstring(v,resstr,dest - resstr);
    }
    else {
        sq_push(v,2); //nothing escaped
    }
    return 1;
}

static SQInteger _string_startswith(HSQUIRRELVM v)
{
    const SQChar *str,*cmp;
    sq_getstring(v,2,&str);
    sq_getstring(v,3,&cmp);
    SQInteger len = sq_getsize(v,2);
    SQInteger cmplen = sq_getsize(v,3);
    SQBool ret = SQFalse;
    if(cmplen <= len) {
        ret = memcmp(str,cmp,sq_rsl(cmplen)) == 0 ? SQTrue : SQFalse;
    }
    sq_pushbool(v,ret);
    return 1;
}

static SQInteger _string_endswith(HSQUIRRELVM v)
{
    const SQChar *str,*cmp;
    sq_getstring(v,2,&str);
    sq_getstring(v,3,&cmp);
    SQInteger len = sq_getsize(v,2);
    SQInteger cmplen = sq_getsize(v,3);
    SQBool ret = SQFalse;
    if(cmplen <= len) {
        ret = memcmp(&str[len - cmplen],cmp,sq_rsl(cmplen)) == 0 ? SQTrue : SQFalse;
    }
    sq_pushbool(v,ret);
    return 1;
}

#define SETUP_REX(v) \
    SQRex *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer *)&self,rex_typetag))) { \
		return sq_throwerror(v,_SC("invalid type tag")); \
	}

static SQInteger _rexobj_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQRex *self = ((SQRex *)p);
    sqstd_rex_free(self);
    return 1;
}

static SQInteger _regexp_match(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str;
    sq_getstring(v,2,&str);
    if(sqstd_rex_match(self,str) == SQTrue)
    {
        sq_pushbool(v,SQTrue);
        return 1;
    }
    sq_pushbool(v,SQFalse);
    return 1;
}

static void _addrexmatch(HSQUIRRELVM v,const SQChar *str,const SQChar *begin,const SQChar *end)
{
    sq_newtable(v);
    sq_pushstring(v,_SC("begin"),-1);
    sq_pushinteger(v,begin - str);
    sq_rawset(v,-3);
    sq_pushstring(v,_SC("end"),-1);
    sq_pushinteger(v,end - str);
    sq_rawset(v,-3);
}

static SQInteger _regexp_search(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str,*begin,*end;
    SQInteger start = 0;
    sq_getstring(v,2,&str);
    if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
    if(sqstd_rex_search(self,str+start,&begin,&end) == SQTrue) {
        _addrexmatch(v,str,begin,end);
        return 1;
    }
    return 0;
}

static SQInteger _regexp_capture(HSQUIRRELVM v)
{
    SETUP_REX(v);
    const SQChar *str,*begin,*end;
    SQInteger start = 0;
    sq_getstring(v,2,&str);
    if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
    if(sqstd_rex_search(self,str+start,&begin,&end) == SQTrue) {
        SQInteger n = sqstd_rex_getsubexpcount(self);
        SQRexMatch match;
        sq_newarray(v,0);
        for(SQInteger i = 0;i < n; i++) {
            sqstd_rex_getsubexp(self,i,&match);
            if(match.len > 0)
                _addrexmatch(v,str,match.begin,match.begin+match.len);
            else
                _addrexmatch(v,str,str,str); //empty match
            sq_arrayappend(v,-2);
        }
        return 1;
    }
    return 0;
}

static SQInteger _regexp_subexpcount(HSQUIRRELVM v)
{
    SETUP_REX(v);
    sq_pushinteger(v,sqstd_rex_getsubexpcount(self));
    return 1;
}

static SQInteger _regexp_constructor(HSQUIRRELVM v)
{
	SQRex *self = NULL;
	if (SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer *)&self, rex_typetag))) {
		return sq_throwerror(v, _SC("invalid type tag"));
	}
	if (self != NULL) {
		return sq_throwerror(v, _SC("invalid regexp object"));
	}
    const SQChar *error,*pattern;
    sq_getstring(v,2,&pattern);
    SQRex *rex = sqstd_rex_compile(pattern,&error);
    if(!rex) return sq_throwerror(v,error);
    sq_setinstanceup(v,1,rex);
    sq_setreleasehook(v,1,_rexobj_releasehook);
    return 0;
}

static SQInteger _regexp__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("regexp"),-1);
    return 1;
}

#define _DECL_REX_FUNC(name,nparams,pmask) {_SC(#name),_regexp_##name,nparams,pmask}
static const SQRegFunction rexobj_funcs[]={
    _DECL_REX_FUNC(constructor,2,_SC(".s")),
    _DECL_REX_FUNC(search,-2,_SC("xsn")),
    _DECL_REX_FUNC(match,2,_SC("xs")),
    _DECL_REX_FUNC(capture,-2,_SC("xsn")),
    _DECL_REX_FUNC(subexpcount,1,_SC("x")),
    _DECL_REX_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_REX_FUNC

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_string_##name,nparams,pmask}
static const SQRegFunction stringlib_funcs[]={
    _DECL_FUNC(format,-2,_SC(".s")),
    _DECL_FUNC(printf,-2,_SC(".s")),
    _DECL_FUNC(strip,2,_SC(".s")),
    _DECL_FUNC(lstrip,2,_SC(".s")),
    _DECL_FUNC(rstrip,2,_SC(".s")),
    _DECL_FUNC(split,-3,_SC(".ssb")),
    _DECL_FUNC(escape,2,_SC(".s")),
    _DECL_FUNC(startswith,3,_SC(".ss")),
    _DECL_FUNC(endswith,3,_SC(".ss")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC


SQInteger sqstd_register_stringlib(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("regexp"),-1);
    sq_newclass(v,SQFalse);
	rex_typetag = (SQUserPointer)rexobj_funcs;
	sq_settypetag(v, -1, rex_typetag);
    SQInteger i = 0;
    while(rexobj_funcs[i].name != 0) {
        const SQRegFunction &f = rexobj_funcs[i];
        sq_pushstring(v,f.name,-1);
        sq_newclosure(v,f.f,0);
        sq_setparamscheck(v,f.nparamscheck,f.typemask);
        sq_setnativeclosurename(v,-1,f.name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    sq_newslot(v,-3,SQFalse);

    i = 0;
    while(stringlib_funcs[i].name!=0)
    {
        sq_pushstring(v,stringlib_funcs[i].name,-1);
        sq_newclosure(v,stringlib_funcs[i].f,0);
        sq_setparamscheck(v,stringlib_funcs[i].nparamscheck,stringlib_funcs[i].typemask);
        sq_setnativeclosurename(v,-1,stringlib_funcs[i].name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    return 1;
}
