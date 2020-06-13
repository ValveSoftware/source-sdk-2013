#include <stdarg.h>
#include <stdio.h>

#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>

#ifdef _MSC_VER
#pragma comment (lib ,"squirrel.lib")
#pragma comment (lib ,"sqstdlib.lib")
#endif

#ifdef SQUNICODE

#define scvprintf vfwprintf
#else

#define scvprintf vfprintf
#endif

void printfunc(HSQUIRRELVM v,const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stdout, s, vl);
    va_end(vl);
}

void errorfunc(HSQUIRRELVM v,const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stderr, s, vl);
    va_end(vl);
}

void call_foo(HSQUIRRELVM v, int n,float f,const SQChar *s)
{
    SQInteger top = sq_gettop(v); //saves the stack size before the call
    sq_pushroottable(v); //pushes the global table
    sq_pushstring(v,_SC("foo"),-1);
    if(SQ_SUCCEEDED(sq_get(v,-2))) { //gets the field 'foo' from the global table
        sq_pushroottable(v); //push the 'this' (in this case is the global table)
        sq_pushinteger(v,n);
        sq_pushfloat(v,f);
        sq_pushstring(v,s,-1);
        sq_call(v,4,SQFalse,SQTrue); //calls the function
    }
    sq_settop(v,top); //restores the original stack size
}

int main(int argc, char* argv[])
{
    HSQUIRRELVM v;
    v = sq_open(1024); // creates a VM with initial stack size 1024

    //REGISTRATION OF STDLIB
    //sq_pushroottable(v); //push the root table where the std function will be registered
    //sqstd_register_iolib(v);  //registers a library
    // ... call here other stdlibs string,math etc...
    //sq_pop(v,1); //pops the root table
    //END REGISTRATION OF STDLIB

    sqstd_seterrorhandlers(v); //registers the default error handlers

    sq_setprintfunc(v, printfunc,errorfunc); //sets the print function

    sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
    if(SQ_SUCCEEDED(sqstd_dofile(v, _SC("test.nut"), SQFalse, SQTrue))) // also prints syntax errors if any
    {
        call_foo(v,1,2.5,_SC("teststring"));
    }

    sq_pop(v,1); //pops the root table
    sq_close(v);

    return 0;
}
