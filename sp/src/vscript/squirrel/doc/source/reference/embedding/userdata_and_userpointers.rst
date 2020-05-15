.. _embedding_userdata_and_userpointers:

=========================
Userdata and UserPointers
=========================

Squirrel allows the host application put arbitrary data chunks into a Squirrel value, this is
possible through the data type userdata.::

    SQUserPointer sq_newuserdata(HSQUIRRELVM v,SQUnsignedInteger size);

When the function *sq_newuserdata* is called, Squirrel allocates a new userdata with the
specified size, returns a pointer to his payload buffer and push the object in the stack; at
this point the application can do whatever it want with this memory chunk, the VM will
automatically take cake of the memory deallocation like for every other built-in type.
A userdata can be passed to a function or stored in a table slot. By default Squirrel
cannot manipulate directly userdata; however is possible to assign a delegate to it and
define a behavior like it would be a table.
Because the application would want to do something with the data stored in a userdata
object when it get deleted, is possible to assign a callback that will be called by the VM
just before deleting a certain userdata.
This is done through the API call *sq_setreleasehook*.::

    typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size);

    void sq_setreleasehook(HSQUIRRELVM v,SQInteger idx,SQRELEASEHOOK hook);

Another kind of userdata is the userpointer; this type is not a memory chunk like the
normal userdata, but just a 'void*' pointer. It cannot have a delegate and is passed by
value, so pushing a userpointer doesn't cause any memory allocation.::

    void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p);

