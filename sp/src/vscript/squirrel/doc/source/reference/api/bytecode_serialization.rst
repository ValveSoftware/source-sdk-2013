.. _api_ref_bytecode_serialization:

======================
Bytecode serialization
======================

.. _sq_readclosure:

.. c:function:: SQRESULT sq_readclosure(HSQUIRRELVM v, SQREADFUNC readf, SQUserPointer up)

    :param HSQUIRRELVM v: the target VM
    :param SQREADFUNC readf: pointer to a read function that will be invoked by the vm during the serialization.
    :param SQUserPointer up: pointer that will be passed to each call to the read function
    :returns: a SQRESULT

serialize (read) a closure and pushes it on top of the stack, the source is user defined through a read callback.





.. _sq_writeclosure:

.. c:function:: SQRESULT sq_writeclosure(HSQUIRRELVM v, SQWRITEFUNC writef, SQUserPointer up)

    :param HSQUIRRELVM v: the target VM
    :param SQWRITEFUNC writef: pointer to a write function that will be invoked by the vm during the serialization.
    :param SQUserPointer up: pointer that will be passed to each call to the write function
    :returns: a SQRESULT
    :remarks: closures with free variables cannot be serialized

serializes(writes) the closure on top of the stack, the destination is user defined through a write callback.
