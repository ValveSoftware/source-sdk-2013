.. _api_ref_stack_operations:

================
Stack Operations
================

.. _sq_cmp:

.. c:function:: SQInteger sq_cmp(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: > 0 if obj1>obj2
    :returns: == 0 if obj1==obj2
    :returns: < 0 if obj1<obj2

compares 2 object from the top of the stack. obj2 should be pushed before obj1.





.. _sq_gettop:

.. c:function:: SQInteger sq_gettop(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an integer representing the index of the top of the stack

returns the index of the top of the stack





.. _sq_pop:

.. c:function:: void sq_pop(HSQUIRRELVM v, SQInteger nelementstopop)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger nelementstopop: the number of elements to pop

pops n elements from the stack





.. _sq_poptop:

.. c:function:: void sq_poptop(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pops 1 object from the stack





.. _sq_push:

.. c:function:: void sq_push(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: the index in the stack of the value that has to be pushed

pushes in the stack the value at the index idx





.. _sq_remove:

.. c:function:: void sq_remove(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the element that has to be removed

removes an element from an arbitrary position in the stack





.. _sq_reservestack:

.. c:function:: SQRESULT sq_reservestack(HSQUIRRELVM v, SQInteger nsize)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger nsize: required stack size
    :returns: a SQRESULT

ensure that the stack space left is at least of a specified size.If the stack is smaller it will automatically grow. If there's a metamethod currently running the function will fail and the stack will not be resized, this situation has to be considered a "stack overflow".





.. _sq_settop:

.. c:function:: void sq_settop(HSQUIRRELVM v, SQInteger v)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger v: the new top index

resize the stack. If new top is bigger then the current top the function will push nulls.
