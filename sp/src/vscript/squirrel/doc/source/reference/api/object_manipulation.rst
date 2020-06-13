.. _api_ref_object_manipulation:

====================
Object manipulation
====================

.. _sq_arrayappend:

.. c:function:: SQRESULT sq_arrayappend(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :returns: a SQRESULT
    :remarks: Only works on arrays.

pops a value from the stack and pushes it in the back of the array at the position idx in the stack.





.. _sq_arrayinsert:

.. c:function:: SQRESULT sq_arrayinsert(HSQUIRRELVM v, SQInteger idx, SQInteger destpos)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :param SQInteger destpos: the position in the array where the item has to be inserted
    :returns: a SQRESULT
    :remarks: Only works on arrays.

pops a value from the stack and inserts it in an array at the specified position





.. _sq_arraypop:

.. c:function:: SQRESULT sq_arraypop(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :returns: a SQRESULT
    :remarks: Only works on arrays.

pops a value from the back of the array at the position idx in the stack.





.. _sq_arrayremove:

.. c:function:: SQRESULT sq_arrayremove(HSQUIRRELVM v, SQInteger idx, SQInteger itemidx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :param SQInteger itemidx: the index of the item in the array that has to be removed
    :returns: a SQRESULT
    :remarks: Only works on arrays.

removes an item from an array





.. _sq_arrayresize:

.. c:function:: SQRESULT sq_arrayresize(HSQUIRRELVM v, SQInteger idx, SQInteger newsize)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :param SQInteger newsize: requested size of the array
    :returns: a SQRESULT
    :remarks: Only works on arrays. If newsize if greater than the current size the new array slots will be filled with nulls.

resizes the array at the position idx in the stack.





.. _sq_arrayreverse:

.. c:function:: SQRESULT sq_arrayreverse(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target array in the stack
    :returns: a SQRESULT
    :remarks: Only works on arrays.

reverses an array in place.





.. _sq_clear:

.. c:function:: SQRESULT sq_clear(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: Only works on tables and arrays.

clears all the elements of the table/array at position idx in the stack.





.. _sq_clone:

.. c:function:: SQRESULT sq_clone(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT

pushes a clone of the table, array, or class instance at the position idx.





.. _sq_createslot:

.. c:function:: SQRESULT sq_createslot(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :returns: a SQRESULT
    :remarks: invoke the _newslot metamethod in the table delegate. it only works on tables. [this function is deperecated since version 2.0.5 use sq_newslot() instead]

pops a key and a value from the stack and performs a set operation on the table or class that is at position idx in the stack; if the slot does not exist, it will be created.





.. _sq_deleteslot:

.. c:function:: SQRESULT sq_deleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :param SQBool pushval: if this param is true the function will push the value of the deleted slot.
    :returns: a SQRESULT
    :remarks: invoke the _delslot metamethod in the table delegate. it only works on tables.

pops a key from the stack and delete the slot indexed by it from the table at position idx in the stack; if the slot does not exist, nothing happens.





.. _sq_get:

.. c:function:: SQRESULT sq_get(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: this call will invokes the delegation system like a normal dereference it only works on tables, arrays, classes, instances and userdata; if the function fails, nothing will be pushed in the stack.

pops a key from the stack and performs a get operation on the object at the position idx in the stack; and pushes the result in the stack.





.. _sq_getattributes:

.. c:function:: SQRESULT sq_getattributes(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target class in the stack
    :returns: a SQRESULT

Gets the attribute of a class member. The function pops a key from the stack and pushes the attribute of the class member indexed by they key from a class at position idx in the stack. If key is null the function gets the class level attribute.





.. _sq_getbase:

.. c:function:: SQRESULT sq_getbase(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target class in the stack
    :returns: a SQRESULT

pushes the base class of the 'class' at stored position idx in the stack.





.. _sq_getclass:

.. c:function:: SQRESULT sq_getclass(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target class instance in the stack
    :returns: a SQRESULT

pushes the class of the 'class instance' at stored position idx in the stack.





.. _sq_getdelegate:

.. c:function:: SQRESULT sq_getdelegate(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT

pushes the current delegate of the object at the position idx in the stack.





.. _sq_getfreevariable:

.. c:function:: const SQChar * sq_getfreevariable(HSQUIRRELVM v, SQInteger idx, SQInteger nval)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack(closure)
    :param SQInteger nval: 0 based index of the free variable(relative to the closure).
    :returns: the name of the free variable for pure squirrel closures. NULL in case of error or if the index of the variable is out of range. In case the target closure is a native closure, the return name is always "@NATIVE".
    :remarks: The function works for both squirrel closure and native closure.

gets the value of the free variable of the closure at the position idx in the stack.





.. _sq_getweakrefval:

.. c:function:: SQRESULT sq_getweakrefval(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target weak reference
    :returns: a SQRESULT
    :remarks: if the function fails, nothing is pushed in the stack.

pushes the object pointed by the weak reference at position idx in the stack.





.. _sq_instanceof:

.. c:function:: SQBool sq_instanceof(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: SQTrue if the instance at position -2 in the stack is an instance of the class object at position -1 in the stack.
    :remarks: The function doesn't pop any object from the stack.

Determines if an object is an instance of a certain class. Expects an instance and a class in the stack.





.. _sq_newmember:

.. c:function:: SQRESULT sq_newmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :param SQBool bstatic: if SQTrue creates a static member.
    :returns: a SQRESULT
    :remarks: Invokes the _newmember metamethod in the class. it only works on classes.

pops a key, a value and an object (which will be set as attribute of the member) from the stack and performs a new slot operation on the class that is at position idx in the stack; if the slot does not exist, it will be created.





.. _sq_newslot:

.. c:function:: SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx, SQBool bstatic)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :param SQBool bstatic: if SQTrue creates a static member. This parameter is only used if the target object is a class.
    :returns: a SQRESULT
    :remarks: Invokes the _newslot metamethod in the table delegate. it only works on tables and classes.

pops a key and a value from the stack and performs a set operation on the table or class that is at position idx in the stack, if the slot does not exist it will be created.





.. _sq_next:

.. c:function:: SQRESULT sq_next(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT

Pushes in the stack the next key and value of an array, table, or class slot. To start the iteration this function expects a null value on top of the stack; at every call the function will substitute the null value with an iterator and push key and value of the container slot. Every iteration the application has to pop the previous key and value but leave the iterator(that is used as reference point for the next iteration). The function will fail when all slots have been iterated(see Tables and arrays manipulation).





.. _sq_rawdeleteslot:

.. c:function:: SQRESULT sq_rawdeleteslot(HSQUIRRELVM v, SQInteger idx, SQBool pushval)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :param SQBool pushval: if this param is true the function will push the value of the deleted slot.
    :returns: a SQRESULT

Deletes a slot from a table without employing the _delslot metamethod. Pops a key from the stack and delete the slot indexed by it from the table at position idx in the stack; if the slot does not exist nothing happens.





.. _sq_rawget:

.. c:function:: SQRESULT sq_rawget(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: Only works on tables and arrays.

pops a key from the stack and performs a get operation on the object at position idx in the stack, without employing delegation or metamethods.





.. _sq_rawnewmember:

.. c:function:: SQRESULT sq_rawnewmember(HSQUIRRELVM v, SQInteger idx, SQBool bstatic)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target table in the stack
    :param SQBool bstatic: if SQTrue creates a static member.
    :returns: a SQRESULT
    :remarks: it only works on classes.

pops a key, a value and an object(that will be set as attribute of the member) from the stack and performs a new slot operation on the class that is at position idx in the stack; if the slot does not exist it will be created.





.. _sq_rawset:

.. c:function:: SQRESULT sq_rawset(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: it only works on tables and arrays. if the function fails nothing will be pushed in the stack.

pops a key and a value from the stack and performs a set operation on the object at position idx in the stack, without employing delegation or metamethods.





.. _sq_set:

.. c:function:: SQRESULT sq_set(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: this call will invoke the delegation system like a normal assignment, it only works on tables, arrays and userdata.

pops a key and a value from the stack and performs a set operation on the object at position idx in the stack.





.. _sq_setattributes:

.. c:function:: SQRESULT sq_setattributes(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target class in the stack.
    :returns: a SQRESULT

Sets the attribute of a class member. The function pops a key and a value from the stack and sets the attribute (indexed by the key) on the class at position idx in the stack. If key is null the function sets the class level attribute. If the function succeed, the old attribute value is pushed in the stack.





.. _sq_setdelegate:

.. c:function:: SQRESULT sq_setdelegate(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :returns: a SQRESULT
    :remarks: to remove the delegate from an object, set a null value.

pops a table from the stack and sets it as the delegate of the object at the position idx in the stack.





.. _sq_setfreevariable:

.. c:function:: SQRESULT sq_setfreevariable(HSQUIRRELVM v, SQInteger idx, SQInteger nval)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :param SQInteger nval: 0 based index of the free variable(relative to the closure).
    :returns: a SQRESULT

pops a value from the stack and sets it as a free variable of the closure at the position idx in the stack.





.. _sq_weakref:

.. c:function:: void sq_weakref(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index to the target object in the stack
    :returns: a SQRESULT
    :remarks: if the object at idx position is one of (integer, float, bool, null), the object itself is pushed instead of a weak ref.

pushes a weak reference to the object at position idx in the stack.
