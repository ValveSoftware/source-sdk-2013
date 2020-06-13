.. _embedding_debug_interface:

===============
Debug Interface
===============

The squirrel VM exposes a very simple debug interface that allows to easily built a full
featured debugger.
Through the functions sq_setdebughook and sq_setnativedebughook is possible in fact to set a callback function that
will be called every time the VM executes an new line of a script or if a function get
called/returns. The callback will pass as argument the current line the current source and the
current function name (if any).::

    SQUIRREL_API void sq_setdebughook(HSQUIRRELVM v);

or ::

    SQUIRREL_API void sq_setnativedebughook(HSQUIRRELVM v,SQDEBUGHOOK hook);

The following code shows how a debug hook could look like(obviously is possible to
implement this function in C as well). ::

    function debughook(event_type,sourcefile,line,funcname)
    {
        local fname=funcname?funcname:"unknown";
        local srcfile=sourcefile?sourcefile:"unknown"
        switch (event_type) {
        case 'l': //called every line(that contains some code)
            ::print("LINE line [" + line + "] func [" + fname + "]");
            ::print("file [" + srcfile + "]\n");
            break;
        case 'c': //called when a function has been called
            ::print("LINE line [" + line + "] func [" + fname + "]");
            ::print("file [" + srcfile + "]\n");
            break;
        case 'r': //called when a function returns
            ::print("LINE line [" + line + "] func [" + fname + "]");
            ::print("file [" + srcfile + "]\n");
            break;
        }
    }

The parameter *event_type* can be 'l' ,'c' or 'r' ; a hook with a 'l' event is called for each line that
gets executed, 'c' every time a function gets called and 'r' every time a function returns.

A full-featured debugger always allows displaying local variables and calls stack.
The call stack information are retrieved through sq_getstackinfos()::

    SQInteger sq_stackinfos(HSQUIRRELVM v,SQInteger level,SQStackInfos *si);

While the local variables info through sq_getlocal()::

    SQInteger sq_getlocal(HSQUIRRELVM v,SQUnsignedInteger level,SQUnsignedInteger nseq);

In order to receive line callbacks the scripts have to be compiled with debug infos enabled
this is done through sq_enabledebuginfo(); ::

    void sq_enabledebuginfo(HSQUIRRELVM v, SQInteger debuginfo);
