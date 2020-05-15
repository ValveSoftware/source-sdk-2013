.. _api_ref_object_creation_and_handling:

============================
Object creation and handling
============================

.. _sq_bindenv:

.. c:function:: SQRESULT sq_bindenv(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target closure
    :returns: a SQRESULT
    :remarks: the cloned closure holds the environment object as weak reference

pops an object from the stack (must be a table, instance, or class); clones the closure at position idx in the stack and sets the popped object as environment of the cloned closure. Then pushes the new cloned closure on top of the stack.





.. _sq_createinstance:

.. c:function:: SQRESULT sq_createinstance(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target class
    :returns: a SQRESULT
    :remarks: the function doesn't invoke the instance contructor. To create an instance and automatically invoke its contructor, sq_call must be used instead.

creates an instance of the class at 'idx' position in the stack. The new class instance is pushed on top of the stack.





.. _sq_getbool:

.. c:function:: SQRESULT sq_getbool(HSQUIRRELVM v, SQInteger idx, SQBool * b)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQBool * b: A pointer to the bool that will store the value
    :returns: a SQRESULT

gets the value of the bool at the idx position in the stack.





.. _sq_getbyhandle:

.. c:function:: SQRESULT sq_getbyhandle(HSQUIRRELVM v, SQInteger idx, HSQMEMBERHANDLE* handle)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack pointing to the class or instance
    :param HSQMEMBERHANDLE* handle: a pointer to the member handle
    :returns: a SQRESULT

pushes the value of a class or instance member using a member handle (see sq_getmemberhandle)





.. _sq_getclosureinfo:

.. c:function:: SQRESULT sq_getclosureinfo(HSQUIRRELVM v, SQInteger idx, SQInteger * nparams, SQInteger * nfreevars)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target closure
    :param SQInteger * nparams: a pointer to an integer that will store the number of parameters
    :param SQInteger * nfreevars: a pointer to an integer that will store the number of free variables
    :returns: an SQRESULT

retrieves number of parameters and number of freevariables from a squirrel closure.





.. _sq_getclosurename:

.. c:function:: SQRESULT sq_getclosurename(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target closure
    :returns: an SQRESULT

pushes the name of the closure at position idx in the stack. Note that the name can be a string or null if the closure is anonymous or a native closure with no name assigned to it.





.. _sq_getclosureroot:

.. c:function:: SQRESULT sq_getclosureroot(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target closure
    :returns: an SQRESULT

pushes the root table of the closure at position idx in the stack





.. _sq_getfloat:

.. c:function:: SQRESULT sq_getfloat(HSQUIRRELVM v, SQInteger idx, SQFloat * f)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQFloat * f: A pointer to the float that will store the value
    :returns: a SQRESULT

gets the value of the float at the idx position in the stack.





.. _sq_gethash:

.. c:function:: SQHash sq_gethash(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :returns: the hash key of the value at the position idx in the stack
    :remarks: the hash value function is the same used by the VM.

returns the hash key of a value at the idx position in the stack.





.. _sq_getinstanceup:

.. c:function:: SQRESULT sq_getinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer * up, SQUSerPointer typetag)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer * up: a pointer to the userpointer that will store the result
    :param SQUSerPointer typetag: the typetag that has to be checked, if this value is set to 0 the typetag is ignored.
    :returns: a SQRESULT

gets the userpointer of the class instance at position idx in the stack. if the parameter 'typetag' is different than 0, the function checks that the class or a base class of the instance is tagged with the specified tag; if not the function fails. If 'typetag' is 0 the function will ignore the tag check.





.. _sq_getinteger:

.. c:function:: SQRESULT sq_getinteger(HSQUIRRELVM v, SQInteger idx, SQInteger * i)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQInteger * i: A pointer to the integer that will store the value
    :returns: a SQRESULT

gets the value of the integer at the idx position in the stack.





.. _sq_getmemberhandle:

.. c:function:: SQRESULT sq_getmemberhandle(HSQUIRRELVM v, SQInteger idx, HSQMEMBERHANDLE* handle)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack pointing to the class
    :param HSQMEMBERHANDLE* handle: a pointer to the variable that will store the handle
    :returns: a SQRESULT
    :remarks: This method works only with classes. A handle retrieved through a class can be later used to set or get values from one of the class instances. Handles retrieved from base classes are still valid in derived classes and respect inheritance rules.

pops a value from the stack and uses it as index to fetch the handle of a class member. The handle can be later used to set or get the member value using sq_getbyhandle(), sq_setbyhandle().





.. _sq_getreleasehook:

.. c:function:: SQRELEASEHOOK sq_getreleasehook(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :remarks: if the object that position idx is not an userdata, class instance or class the function returns NULL.

gets the release hook of the userdata, class instance or class at position idx in the stack.





.. _sq_getscratchpad:

.. c:function:: SQChar * sq_getscratchpad(HSQUIRRELVM v, SQInteger minsize)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger minsize: the requested size for the scratchpad buffer
    :remarks: the buffer is valid until the next call to sq_getscratchpad

returns a pointer to a memory buffer that is at least as big as minsize.





.. _sq_getsize:

.. c:function:: SQObjectType sq_getsize(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :returns: the size of the value at the position idx in the stack
    :remarks: this function only works with strings, arrays, tables, classes, instances, and userdata if the value is not a valid type, the function will return -1.

returns the size of a value at the idx position in the stack. If the value is a class or a class instance the size returned is the size of the userdata buffer (see sq_setclassudsize).





.. _sq_getstring:

.. c:function:: SQRESULT sq_getstring(HSQUIRRELVM v, SQInteger idx, const SQChar ** c)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param const SQChar ** c: a pointer to the pointer that will point to the string
    :returns: a SQRESULT

gets a pointer to the string at the idx position in the stack.





.. _sq_getstringandsize:

.. c:function:: SQRESULT sq_getstringandsize(HSQUIRRELVM v, SQInteger idx, const SQChar ** c, SQInteger* size)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param const SQChar ** c: a pointer to the pointer that will point to the string
    :param SQInteger * size: a pointer to a SQInteger which will receive the size of the string
    :returns: a SQRESULT

gets a pointer to the string at the idx position in the stack; additionally retrieves its size.




.. _sq_getthread:

.. c:function:: SQRESULT sq_getthread(HSQUIRRELVM v, SQInteger idx, HSQUIRRELVM* v)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param HSQUIRRELVM* v: A pointer to the variable that will store the thread pointer
    :returns: a SQRESULT

gets a pointer to the thread the idx position in the stack.





.. _sq_gettype:

.. c:function:: SQObjectType sq_gettype(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :returns: the type of the value at the position idx in the stack

returns the type of the value at the position idx in the stack





.. _sq_gettypetag:

.. c:function:: SQRESULT sq_gettypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer * typetag)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer * typetag: a pointer to the variable that will store the tag
    :returns: a SQRESULT
    :remarks: the function works also with instances. if the taget object is an instance, the typetag of it's base class is fetched.

gets the typetag of the object (userdata or class) at position idx in the stack.





.. _sq_getuserdata:

.. c:function:: SQRESULT sq_getuserdata(HSQUIRRELVM v, SQInteger idx, SQUserPointer * p, SQUserPointer * typetag)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer * p: A pointer to the userpointer that will point to the userdata's payload
    :param SQUserPointer * typetag: A pointer to a SQUserPointer that will store the userdata tag(see sq_settypetag). The parameter can be NULL.
    :returns: a SQRESULT

gets a pointer to the value of the userdata at the idx position in the stack.





.. _sq_getuserpointer:

.. c:function:: SQRESULT sq_getuserpointer(HSQUIRRELVM v, SQInteger idx, SQUserPointer * p)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer * p: A pointer to the userpointer that will store the value
    :returns: a SQRESULT

gets the value of the userpointer at the idx position in the stack.





.. _sq_newarray:

.. c:function:: void sq_newarray(HSQUIRRELVM v, SQInteger size)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger size: the size of the array that as to be created

creates a new array and pushes it in the stack





.. _sq_newclass:

.. c:function:: SQRESULT sq_newclass(HSQUIRRELVM v, SQBool hasbase)

    :param HSQUIRRELVM v: the target VM
    :param SQBool hasbase: if the parameter is true the function expects a base class on top of the stack.
    :returns: a SQRESULT

creates a new class object. If the parameter 'hasbase' is different than 0, the function pops a class from the stack and inherits the new created class from it. The new class is pushed in the stack.





.. _sq_newclosure:

.. c:function:: void sq_newclosure(HSQUIRRELVM v, HSQFUNCTION func, SQInteger nfreevars)

    :param HSQUIRRELVM v: the target VM
    :param HSQFUNCTION func: a pointer to a native-function
    :param SQInteger nfreevars: number of free variables(can be 0)

create a new native closure, pops n values set those as free variables of the new closure, and push the new closure in the stack.





.. _sq_newtable:

.. c:function:: void sq_newtable(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

creates a new table and pushes it in the stack





.. _sq_newtableex:

.. c:function:: void sq_newtableex(HSQUIRRELVM v, SQInteger initialcapacity)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger initialcapacity: number of key/value pairs to preallocate

creates a new table and pushes it in the stack. This function allows you to specify the initial capacity of the table to prevent unnecessary rehashing when the number of slots required is known at creation-time.





.. _sq_newuserdata:

.. c:function:: SQUserPointer sq_newuserdata(HSQUIRRELVM v, SQUnsignedInteger size)

    :param HSQUIRRELVM v: the target VM
    :param SQUnsignedInteger size: the size of the userdata that as to be created in bytes

creates a new userdata and pushes it in the stack





.. _sq_pushbool:

.. c:function:: void sq_pushbool(HSQUIRRELVM v, SQBool b)

    :param HSQUIRRELVM v: the target VM
    :param SQBool b: the bool that has to be pushed(SQTrue or SQFalse)

pushes a bool into the stack





.. _sq_pushfloat:

.. c:function:: void sq_pushfloat(HSQUIRRELVM v, SQFloat f)

    :param HSQUIRRELVM v: the target VM
    :param SQFloat f: the float that has to be pushed

pushes a float into the stack





.. _sq_pushinteger:

.. c:function:: void sq_pushinteger(HSQUIRRELVM v, SQInteger n)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger n: the integer that has to be pushed

pushes an integer into the stack





.. _sq_pushnull:

.. c:function:: void sq_pushnull(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

pushes a null value into the stack





.. _sq_pushstring:

.. c:function:: void sq_pushstring(HSQUIRRELVM v, const SQChar * s, SQInteger len)

    :param HSQUIRRELVM v: the target VM
    :param const SQChar * s: pointer to the string that has to be pushed
    :param SQInteger len: length of the string pointed by s
    :remarks: if the parameter len is less than 0 the VM will calculate the length using strlen(s)

pushes a string in the stack





.. _sq_pushuserpointer:

.. c:function:: void sq_pushuserpointer(HSQUIRRELVM v, SQUserPointer p)

    :param HSQUIRRELVM v: the target VM
    :param SQUserPointer p: the pointer that as to be pushed

pushes a userpointer into the stack





.. _sq_setbyhandle:

.. c:function:: SQRESULT sq_setbyhandle(HSQUIRRELVM v, SQInteger idx, HSQMEMBERHANDLE* handle)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack pointing to the class
    :param HSQMEMBERHANDLE* handle: a pointer the member handle
    :returns: a SQRESULT

pops a value from the stack and sets it to a class or instance member using a member handle (see sq_getmemberhandle)





.. _sq_setclassudsize:

.. c:function:: SQRESULT sq_setclassudsize(HSQUIRRELVM v, SQInteger idx, SQInteger udsize)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack pointing to the class
    :param SQInteger udsize: size in bytes reserved for user data
    :returns: a SQRESULT

Sets the user data size of a class. If a class 'user data size' is greater than 0. When an instance of the class is created additional space will be reserved at the end of the memory chunk where the instance is stored. The userpointer of the instance will also be automatically set to this memory area. This allows you to minimize allocations in applications that have to carry data along with the class instance.





.. _sq_setclosureroot:

.. c:function:: SQRESULT sq_setclosureroot(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target closure
    :returns: an SQRESULT

pops a table from the stack and sets it as root of the closure at position idx in the stack





.. _sq_setinstanceup:

.. c:function:: SQRESULT sq_setinstanceup(HSQUIRRELVM v, SQInteger idx, SQUserPointer up)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer up: an arbitrary user pointer
    :returns: a SQRESULT

sets the userpointer of the class instance at position idx in the stack.





.. _sq_setnativeclosurename:

.. c:function:: SQRESULT sq_setnativeclosurename(HSQUIRRELVM v, SQInteger idx, const SQChar * name)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: index of the target native closure
    :param const SQChar * name: the name that has to be set
    :returns: an SQRESULT

sets the name of the native closure at the position idx in the stack. The name of a native closure is purely for debug purposes. The name is retrieved through the function sq_stackinfos() while the closure is in the call stack.





.. _sq_setparamscheck:

.. c:function:: SQRESULT sq_setparamscheck(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar * typemask)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger nparamscheck: defines the parameters number check policy (0 disables the param checking). If nparamscheck is greater than 0, the VM ensures that the number of parameters is exactly the number specified in nparamscheck (eg. if nparamscheck == 3 the function can only be called with 3 parameters). If nparamscheck is less than 0 the VM ensures that the closure is called with at least the absolute value of the number specified in nparamcheck (eg. nparamscheck == -3 will check that the function is called with at least 3 parameters). The hidden parameter 'this' is included in this number; free variables aren't. If SQ_MATCHTYPEMASKSTRING is passed instead of the number of parameters, the function will automatically infer the number of parameters to check from the typemask (eg. if the typemask is ".sn", it is like passing 3).
    :param const SQChar * typemask: defines a mask to validate the parametes types passed to the function. If the parameter is NULL, no typechecking is applied (default).
    :remarks: The typemask consists in a zero terminated string that represent the expected parameter type. The types are expressed as follows: 'o' null, 'i' integer, 'f' float, 'n' integer or float, 's' string, 't' table, 'a' array, 'u' userdata, 'c' closure and nativeclosure, 'g' generator, 'p' userpointer, 'v' thread, 'x' instance(class instance), 'y' class, 'b' bool. and '.' any type. The symbol '|' can be used as 'or' to accept multiple types on the same parameter. There isn't any limit on the number of 'or' that can be used. Spaces are ignored so can be inserted between types to increase readability. For instance to check a function that expect a table as 'this' a string as first parameter and a number or a userpointer as second parameter, the string would be "tsn|p" (table,string,number or userpointer). If the parameters mask is contains fewer parameters than 'nparamscheck', the remaining parameters will not be typechecked.

Sets the parameter validation scheme for the native closure at the top position in the stack. Allows you to validate the number of parameters accepted by the function and optionally their types. If the function call does not comply with the parameter schema set by sq_setparamscheck, an exception is thrown.

*.eg*

::

    //example
    SQInteger testy(HSQUIRRELVM v)
    {
        SQUserPointer p;
        const SQChar *s;
        SQInteger i;
        //no type checking, if the call complies with the mask
        //surely the functions will succeed.
        sq_getuserdata(v,1,&p,NULL);
        sq_getstring(v,2,&s);
        sq_getinteger(v,3,&i);
        //... do something
        return 0;
    }

    //the reg code

    //....stuff
    sq_newclosure(v,testy,0);
    //expects exactly 3 parameters(userdata,string,number)
    sq_setparamscheck(v,3,_SC("usn"));
    //....stuff






.. _sq_setreleasehook:

.. c:function:: void sq_setreleasehook(HSQUIRRELVM v, SQInteger idx, SQRELEASEHOOK hook)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQRELEASEHOOK hook: a function pointer to the hook(see sample below)
    :remarks: the function hook is called by the VM before the userdata memory is deleted.

sets the release hook of the userdata, class instance, or class at position idx in the stack.

*.eg*

::


    /* tyedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size); */

    SQInteger my_release_hook(SQUserPointer p,SQInteger size)
    {
        /* do something here */
        return 1;
    }






.. _sq_settypetag:

.. c:function:: SQRESULT sq_settypetag(HSQUIRRELVM v, SQInteger idx, SQUserPointer typetag)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQUserPointer typetag: an arbitrary SQUserPointer
    :returns: a SQRESULT

sets the typetag of the object (userdata or class) at position idx in the stack.





.. _sq_tobool:

.. c:function:: void sq_tobool(HSQUIRRELVM v, SQInteger idx, SQBool * b)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :param SQBool * b: A pointer to the bool that will store the value
    :remarks: if the object is not a bool the function converts the value to bool according to squirrel's rules. For instance the number 1 will result in true, and the number 0 in false.

gets the value at position idx in the stack as bool.





.. _sq_tostring:

.. c:function:: void sq_tostring(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack

converts the object at position idx in the stack to string and pushes the resulting string in the stack.





.. _sq_typeof:

.. c:function:: SQObjectType sq_typeof(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: an index in the stack
    :returns: a SQRESULT

pushes the type name of the value at the position idx in the stack. It also invokes the _typeof metamethod for tables and class instances that implement it; in that case the pushed object could be something other than a string (is up to the _typeof implementation).



