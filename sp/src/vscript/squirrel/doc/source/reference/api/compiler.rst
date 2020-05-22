.. _api_ref_compiler:

========
Compiler
========

.. _sq_compile:

.. c:function:: SQRESULT sq_compile(HSQUIRRELVM v, HSQLEXREADFUNC read, SQUserPointer p, const SQChar * sourcename, SQBool raiseerror)

    :param HSQUIRRELVM v: the target VM
    :param HSQLEXREADFUNC read: a pointer to a read function that will feed the compiler with the program.
    :param SQUserPointer p: a user defined pointer that will be passed by the compiler to the read function at each invocation.
    :param const SQChar * sourcename: the symbolic name of the program (used only for more meaningful runtime errors)
    :param SQBool raiseerror: if this value is true the compiler error handler will be called in case of an error
    :returns: a SQRESULT. If the sq_compile fails nothing is pushed in the stack.
    :remarks: in case of an error the function will call the function set by sq_setcompilererrorhandler().

compiles a squirrel program; if it succeeds, push the compiled script as function in the stack.





.. _sq_compilebuffer:

.. c:function:: SQRESULT sq_compilebuffer(HSQUIRRELVM v, const SQChar* s, SQInteger size, const SQChar * sourcename, SQBool raiseerror)

    :param HSQUIRRELVM v: the target VM
    :param const SQChar* s: a pointer to the buffer that has to be compiled.
    :param SQInteger size: size in characters of the buffer passed in the parameter 's'.
    :param const SQChar * sourcename: the symbolic name of the program (used only for more meaningful runtime errors)
    :param SQBool raiseerror: if this value true the compiler error handler will be called in case of an error
    :returns: a SQRESULT. If the sq_compilebuffer fails nothing is pushed in the stack.
    :remarks: in case of an error the function will call the function set by sq_setcompilererrorhandler().

compiles a squirrel program from a memory buffer; if it succeeds, push the compiled script as function in the stack.





.. _sq_enabledebuginfo:

.. c:function:: void sq_enabledebuginfo(HSQUIRRELVM v, SQBool enable)

    :param HSQUIRRELVM v: the target VM
    :param SQBool enable: if true enables the debug info generation, if == 0 disables it.
    :remarks: The function affects all threads as well.

enable/disable the debug line information generation at compile time.





.. _sq_notifyallexceptions:

.. c:function:: void sq_notifyallexceptions(HSQUIRRELVM v, SQBool enable)

    :param HSQUIRRELVM v: the target VM
    :param SQBool enable: if true enables the error callback notification of handled exceptions.
    :remarks: By default the VM will invoke the error callback only if an exception is not handled (no try/catch traps are present in the call stack). If notifyallexceptions is enabled, the VM will call the error callback for any exception even if between try/catch blocks. This feature is useful for implementing debuggers.

enable/disable the error callback notification of handled exceptions.





.. _sq_setcompilererrorhandler:

.. c:function:: void sq_setcompilererrorhandler(HSQUIRRELVM v, SQCOMPILERERROR f)

    :param HSQUIRRELVM v: the target VM
    :param SQCOMPILERERROR f: A pointer to the error handler function
    :remarks: if the parameter f is NULL no function will be called when a compiler error occurs. The compiler error handler is shared between friend VMs.

sets the compiler error handler function
