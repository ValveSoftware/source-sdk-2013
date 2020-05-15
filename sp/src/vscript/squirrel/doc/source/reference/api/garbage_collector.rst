.. _api_ref_garbage_collector:

=================
Garbage Collector
=================

.. _sq_collectgarbage:

.. c:function:: SQInteger sq_collectgarbage(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :remarks: this api only works with garbage collector builds (NO_GARBAGE_COLLECTOR is not defined)

runs the garbage collector and returns the number of reference cycles found (and deleted)





.. _sq_resurrectunreachable:

.. c:function:: SQRESULT sq_resurrectunreachable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :remarks: this api only works with garbage collector builds (NO_GARBAGE_COLLECTOR is not defined)

runs the garbage collector and pushes an array in the stack containing all unreachable object found. If no unreachable object is found, null is pushed instead. This function is meant to help debug reference cycles.
