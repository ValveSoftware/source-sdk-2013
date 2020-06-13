.. _api_ref_debug_interface:

===============
Debug interface
===============

.. _sq_getfunctioninfo:

.. c:function:: SQRESULT sq_getfunctioninfo(HSQUIRRELVM v, SQInteger level, SQFunctionInfo * fi)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger level: calls stack level
    :param SQFunctionInfo * fi: pointer to the SQFunctionInfo structure that will store the closure informations
    :returns: a SQRESULT.
    :remarks: the member 'funcid' of the returned SQFunctionInfo structure is a unique identifier of the function; this can be useful to identify a specific piece of squirrel code in an application like for instance a profiler. this method will fail if the closure in the stack is a native C closure.



*.eg*

::


    typedef struct tagSQFunctionInfo {
        SQUserPointer funcid; //unique idetifier for a function (all it's closures will share the same funcid)
        const SQChar *name; //function name
        const SQChar *source; //function source file name
    }SQFunctionInfo;







.. _sq_setdebughook:

.. c:function:: void sq_setdebughook(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :remarks: In order to receive a 'per line' callback, is necessary to compile the scripts with the line informations. Without line informations activated, only the 'call/return' callbacks will be invoked.

pops a closure from the stack an sets it as debug hook. When a debug hook is set it overrides any previously set native or non native hooks. if the hook is null the debug hook will be disabled.





.. _sq_setnativedebughook:

.. c:function:: void sq_setnativedebughook(HSQUIRRELVM v, SQDEBUGHOOK hook)

    :param HSQUIRRELVM v: the target VM
    :param SQDEBUGHOOK hook: the native hook function
    :remarks: In order to receive a 'per line' callback, is necessary to compile the scripts with the line informations. Without line informations activated, only the 'call/return' callbacks will be invoked.

sets the native debug hook. When a native hook is set it overrides any previously set native or non native hooks. if the hook is NULL the debug hook will be disabled.





.. _sq_stackinfos:

.. c:function:: SQRESULT sq_stackinfos(HSQUIRRELVM v, SQInteger level, SQStackInfos * si)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger level: calls stack level
    :param SQStackInfos * si: pointer to the SQStackInfos structure that will store the stack informations
    :returns: a SQRESULT.

retrieve the calls stack informations of a ceratain level in the calls stack.
