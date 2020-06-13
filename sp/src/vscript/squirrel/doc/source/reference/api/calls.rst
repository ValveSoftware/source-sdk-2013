.. _api_ref_calls:

=====
Calls
=====

.. _sq_call:

.. c:function:: SQRESULT sq_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger params: number of parameters of the function
    :param SQBool retval: if true the function will push the return value in the stack
    :param SQBool raiseerror: if true, if a runtime error occurs during the execution of the call, the vm will invoke the error handler.
    :returns: a SQRESULT

calls a closure or a native closure. The function pops all the parameters and leave the closure in the stack; if retval is true the return value of the closure is pushed. If the execution of the function is suspended through sq_suspendvm(), the closure and the arguments will not be automatically popped from the stack.

When using to create an instance, push a dummy parameter to be filled with the newly-created instance for the constructor's 'this' parameter.



.. _sq_getcallee:

.. c:function:: SQRESULT sq_getcallee(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: a SQRESULT

push in the stack the currently running closure.





.. _sq_getlasterror:

.. c:function:: SQRESULT sq_getlasterror(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: a SQRESULT
    :remarks: the pushed error descriptor can be any valid squirrel type.

pushes the last error in the stack.





.. _sq_getlocal:

.. c:function:: const SQChar * sq_getlocal(HSQUIRRELVM v, SQUnsignedInteger level, SQUnsignedInteger nseq)

    :param HSQUIRRELVM v: the target VM
    :param SQUnsignedInteger level: the function index in the calls stack, 0 is the current function
    :param SQUnsignedInteger nseq: the index of the local variable in the stack frame (0 is 'this')
    :returns: the name of the local variable if a variable exists at the given level/seq otherwise NULL.

Returns the name of a local variable given stackframe and sequence in the stack and pushes is current value. Free variables are treated as local variables, by sq_getlocal(), and will be returned as they would be at the base of the stack, just before the real local variables.





.. _sq_reseterror:

.. c:function:: void sq_reseterror(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

reset the last error in the virtual machine to null





.. _sq_resume:

.. c:function:: SQRESULT sq_resume(HSQUIRRELVM v, SQBool retval, SQBool raiseerror)

    :param HSQUIRRELVM v: the target VM
    :param SQBool retval: if true the function will push the return value in the stack
    :param SQBool raiseerror: if true, if a runtime error occurs during the execution of the call, the vm will invoke the error handler.
    :returns: a SQRESULT
    :remarks: if retval != 0 the return value of the generator is pushed.

resumes the generator at the top position of the stack.


.. _sq_tailcall:

.. c:function:: SQRESULT sq_tailcall(HSQUIRRELVM v, SQInteger nparams)

	:param HSQUIRRELVM v: the target VM
    :param SQInteger params: number of parameters of the function

	Calls a closure and removes the caller function from the call stack.
	This function must be invoke from a native closure and 
	he return value of sq_tailcall must be returned by the caller function(see example).
	
*.eg*

::

    SQInteger tailcall_something_example(HSQUIRRELVM v)
    {
		//push closure and parameters here
		... 
        return sq_tailcall(v,2);
    }
	
.. _sq_throwerror:

.. c:function:: SQRESULT sq_throwerror(HSQUIRRELVM v, const SQChar * err)

    :param HSQUIRRELVM v: the target VM
    :param const SQChar * err: the description of the error that has to be thrown
    :returns: the value that has to be returned by a native closure in order to throw an exception in the virtual machine.

sets the last error in the virtual machine and returns the value that has to be returned by a native closure in order to trigger an exception in the virtual machine.


.. _sq_throwobject:

.. c:function:: SQRESULT sq_throwobject(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the value that has to be returned by a native closure in order to throw an exception in the virtual machine.

pops a value from the stack sets it as the last error in the virtual machine. Returns the value that has to be returned by a native closure in order to trigger an exception in the virtual machine (aka SQ_ERROR).
