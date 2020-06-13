.. _embedding_calling_a_function:

==================
Calling a function
==================

To call a squirrel function it is necessary to push the function in the stack followed by the
parameters and then call the function sq_call.
The function will pop the parameters and push the return value if the last sq_call
parameter is > 0. ::

    sq_pushroottable(v);
    sq_pushstring(v,"foo",-1);
    sq_get(v,-2); //get the function from the root table
    sq_pushroottable(v); //'this' (function environment object)
    sq_pushinteger(v,1);
    sq_pushfloat(v,2.0);
    sq_pushstring(v,"three",-1);
    sq_call(v,4,SQFalse,SQFalse);
    sq_pop(v,2); //pops the roottable and the function

this is equivalent to the following Squirrel code::

    foo(1,2.0,"three");

If a runtime error occurs (or a exception is thrown) during the squirrel code execution
the sq_call will fail.
