.. _api_ref_raw_object_handling:

===================
Raw object handling
===================

.. _sq_addref:

.. c:function:: void sq_addref(HSQUIRRELVM v, HSQOBJECT* po)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT* po: pointer to an object handler

adds a reference to an object handler.





.. _sq_getobjtypetag:

.. c:function:: SQRESULT sq_getobjtypetag(HSQOBJECT* o, SQUserPointer* typetag)

    :param HSQOBJECT* o: pointer to an object handler
    :param SQUserPointer* typetag: a pointer to the variable that will store the tag
    :returns: a SQRESULT
    :remarks: the function works also with instances. if the target object is an instance, the typetag of it's base class is fetched.

gets the typetag of a raw object reference(userdata or class).





.. _sq_getrefcount:

.. c:function:: SQUnsignedInteger sq_getrefcount(HSQUIRRELVM v, HSQOBJECT* po)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT* po: object handler

returns the number of references of a given object.





.. _sq_getstackobj:

.. c:function:: SQRESULT sq_getstackobj(HSQUIRRELVM v, SQInteger idx, HSQOBJECT* po)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target object in the stack
    :param HSQOBJECT* po: pointer to an object handler
    :returns: a SQRESULT

gets an object from the stack and stores it in a object handler.





.. _sq_objtobool:

.. c:function:: SQBool sq_objtobool(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: If the object is not a bool will always return false.

return the bool value of a raw object reference.





.. _sq_objtofloat:

.. c:function:: SQFloat sq_objtofloat(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: If the object is an integer will convert it to float. If the object is not a number will always return 0.

return the float value of a raw object reference.





.. _sq_objtointeger:

.. c:function:: SQInteger sq_objtointeger(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: If the object is a float will convert it to integer. If the object is not a number will always return 0.

return the integer value of a raw object reference.





.. _sq_objtostring:

.. c:function:: const SQChar* sq_objtostring(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: If the object doesn't reference a string it returns NULL.

return the string value of a raw object reference.





.. _sq_objtouserpointer:

.. c:function:: SQUserPointer sq_objtouserpointer(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: If the object doesn't reference a userpointer it returns NULL.

return the userpointer value of a raw object reference.





.. _sq_pushobject:

.. c:function:: void sq_pushobject(HSQUIRRELVM v, HSQOBJECT obj)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT obj: object handler

push an object referenced by an object handler into the stack.





.. _sq_release:

.. c:function:: SQBool sq_release(HSQUIRRELVM v, HSQOBJECT* po)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT* po: pointer to an object handler
    :returns: SQTrue if the object handler released has lost all is references(the ones added with sq_addref). SQFalse otherwise.
    :remarks: the function will reset the object handler to null when it loses all references.

remove a reference from an object handler.





.. _sq_resetobject:

.. c:function:: void sq_resetobject(HSQOBJECT* po)

    :param HSQOBJECT* po: pointer to an object handler
    :remarks: Every object handler has to be initialized with this function.

resets(initialize) an object handler.
