.. _api_ref_virtual_machine:

===============
Virtual Machine
===============


.. _sq_close:

.. c:function:: void sq_close(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

releases a squirrel VM and all related friend VMs





.. _sq_geterrorfunc:

.. c:function:: SQPRINTFUNCTION sq_geterrorfunc(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: a pointer to a SQPRINTFUNCTION, or NULL if no function has been set.

returns the current error function of the given Virtual machine. (see sq_setprintfunc())





.. _sq_getforeignptr:

.. c:function:: SQUserPointer sq_getforeignptr(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the current VMs foreign pointer.

Returns the foreign pointer of a VM instance.





.. _sq_getprintfunc:

.. c:function:: SQPRINTFUNCTION sq_getprintfunc(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: a pointer to a SQPRINTFUNCTION, or NULL if no function has been set.

returns the current print function of the given Virtual machine. (see sq_setprintfunc())





.. _sq_getsharedforeignptr:

.. c:function:: SQUserPointer sq_getsharedforeignptr(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the current VMs shared foreign pointer

Returns the shared foreign pointer of a group of friend VMs





.. _sq_getsharedreleasehook:

.. c:function:: SQUserPointer sq_getsharedreleasehook(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the current VMs release hook.

Returns the shared release hook of a group of friend VMs





.. _sq_getversion:

.. c:function:: SQInteger sq_getversion()

    :returns: version number of the vm(as in SQUIRREL_VERSION_NUMBER).

returns the version number of the vm





.. _sq_getvmreleasehook:

.. c:function:: SQUserPointer sq_getvmreleasehook(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the current VMs release hook.

Returns the release hook of a VM instance





.. _sq_getvmstate:

.. c:function:: SQInteger sq_getvmstate(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: the state of the vm encoded as integer value. The following constants are defined: SQ_VMSTATE_IDLE, SQ_VMSTATE_RUNNING, SQ_VMSTATE_SUSPENDED.

returns the execution state of a virtual machine





.. _sq_move:

.. c:function:: void sq_move(HSQUIRRELVM dest, HSQUIRRELVM src, SQInteger idx)

    :param HSQUIRRELVM dest: the destination VM
    :param HSQUIRRELVM src: the source VM
    :param SQInteger idx: the index in the source stack of the value that has to be moved

pushes the object at the position 'idx' of the source vm stack in the destination vm stack





.. _sq_newthread:

.. c:function:: HSQUIRRELVM sq_newthread(HSQUIRRELVM friendvm, SQInteger initialstacksize)

    :param HSQUIRRELVM friendvm: a friend VM
    :param SQInteger initialstacksize: the size of the stack in slots(number of objects)
    :returns: a pointer to the new VM.
    :remarks: By default the roottable is shared with the VM passed as first parameter. The new VM lifetime is bound to the "thread" object pushed in the stack and behave like a normal squirrel object.

creates a new vm friendvm of the one passed as first parmeter and pushes it in its stack as "thread" object.





.. _sq_open:

.. c:function:: HSQUIRRELVM sq_open(SQInteger initialstacksize)

    :param SQInteger initialstacksize: the size of the stack in slots(number of objects)
    :returns: an handle to a squirrel vm
    :remarks: the returned VM has to be released with sq_releasevm

creates a new instance of a squirrel VM that consists in a new execution stack.





.. _sq_pushconsttable:

.. c:function:: void sq_pushconsttable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pushes the current const table in the stack





.. _sq_pushregistrytable:

.. c:function:: void sq_pushregistrytable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pushes the registry table in the stack





.. _sq_pushroottable:

.. c:function:: void sq_pushroottable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pushes the current root table in the stack





.. _sq_setconsttable:

.. c:function:: void sq_setconsttable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pops a table from the stack and set it as const table





.. _sq_seterrorhandler:

.. c:function:: void sq_seterrorhandler(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :remarks: the error handler is shared by friend VMs

pops from the stack a closure or native closure an sets it as runtime-error handler.





.. _sq_setforeignptr:

.. c:function:: void sq_setforeignptr(HSQUIRRELVM v, SQUserPointer p)

    :param HSQUIRRELVM v: the target VM
    :param SQUserPointer p: The pointer that has to be set

Sets the foreign pointer of a certain VM instance. The foreign pointer is an arbitrary user defined pointer associated to a VM (by default is value id 0). This pointer is ignored by the VM.





.. _sq_setprintfunc:

.. c:function:: void sq_setprintfunc(HSQUIRRELVM v, SQPRINTFUNCTION printfunc, SQPRINTFUNCTION errorfunc)

    :param HSQUIRRELVM v: the target VM
    :param SQPRINTFUNCTION printfunc: a pointer to the print func or NULL to disable the output.
    :param SQPRINTFUNCTION errorfunc: a pointer to the error func or NULL to disable the output.
    :remarks: the print func has the following prototype: void printfunc(HSQUIRRELVM v,const SQChar \*s,...)

sets the print function of the virtual machine. This function is used by the built-in function '::print()' to output text.





.. _sq_setroottable:

.. c:function:: void sq_setroottable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pops a table from the stack and set it as root table





.. _sq_setsharedforeignptr:

.. c:function:: void sq_setsharedforeignptr(HSQUIRRELVM v, SQUserPointer p)

    :param HSQUIRRELVM v: the target VM
    :param SQUserPointer p: The pointer that has to be set

Sets the shared foreign pointer. The foreign pointer is an arbitrary user defined pointer associated to a group of friend VMs (by default is value id 0). After a "main" VM is created using sq_open() all friend VMs created with sq_newthread share the same shared pointer.





.. _sq_setsharedreleasehook:

.. c:function:: void sq_setsharedreleasehook(HSQUIRRELVM v, SQRELESEHOOK hook)

    :param HSQUIRRELVM v: the target VM
    :param SQRELESEHOOK hook: The hook that has to be set

Sets the release hook of a certain VM group. The release hook is invoked when the last vm of the group vm is destroyed (usually when sq_close() is invoked). The userpointer passed to the function is the shared foreignpointer(see sq_getsharedforeignptr()). After a "main" VM is created using sq_open() all friend VMs created with sq_newthread() share the same shared release hook.





.. _sq_setvmreleasehook:

.. c:function:: void sq_setvmreleasehook(HSQUIRRELVM v, SQRELESEHOOK hook)

    :param HSQUIRRELVM v: the target VM
    :param SQRELESEHOOK hook: The hook that has to be set

Sets the release hook of a certain VM instance. The release hook is invoked when the vm is destroyed. The userpointer passed to the function is the vm foreignpointer (see sq_setforeignpointer())





.. _sq_suspendvm:

.. c:function:: HRESULT sq_suspendvm(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT(that has to be returned by a C function)
    :remarks: sq_result can only be called as return expression of a C function. The function will fail is the suspension is done through more C calls or in a metamethod.

Suspends the execution of the specified vm.

*.eg*

::

    SQInteger suspend_vm_example(HSQUIRRELVM v)
    {
        return sq_suspendvm(v);
    }






.. _sq_wakeupvm:

.. c:function:: HRESULT sq_wakeupvm(HSQUIRRELVM v, SQBool resumedret, SQBool retval, SQBool raiseerror, SQBool throwerror)

    :param HSQUIRRELVM v: the target VM
    :param SQBool resumedret: if true the function will pop a value from the stack and use it as return value for the function that has previously suspended the virtual machine.
    :param SQBool retval: if true the function will push the return value of the function that suspend the excution or the main function one.
    :param SQBool raiseerror: if true, if a runtime error occurs during the execution of the call, the vm will invoke the error handler.
    :param SQBool throwerror: if true, the vm will thow an exception as soon as is resumed. the exception payload must be set beforehand invoking sq_thowerror().
    :returns: an HRESULT.

wake up the execution a previously suspended virtual machine
