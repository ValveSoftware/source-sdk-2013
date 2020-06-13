.. embedding_references_from_c:

========================================================
Mantaining references to Squirrel values from the C API
========================================================

Squirrel allows to reference values through the C API; the function sq_getstackobj() gets
a handle to a squirrel object(any type). The object handle can be used to control the lifetime
of an object by adding or removing references to it( see sq_addref() and sq_release()).
The object can be also re-pushed in the VM stack using sq_pushobject().::

    HSQOBJECT obj;

    sq_resetobject(&obj); //initialize the handle
    sq_getstackobj(v,-2,&obj); //retrieve an object handle from the pos -2
    sq_addref(v,&obj); //adds a reference to the object

    ... //do stuff

    sq_pushobject(v,obj); //push the object in the stack
    sq_release(v,&obj); //relese the object
