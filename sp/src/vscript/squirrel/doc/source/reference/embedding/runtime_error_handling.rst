.. _embedding_runtime_error_handling:

======================
Runtime error handling
======================

When an exception is not handled by Squirrel code with a try/catch statement, a runtime
error is raised and the execution of the current program is interrupted. It is possible to
set a call back function to intercept the runtime error from the host program; this is
useful to show meaningful errors to the script writer and for implementing visual
debuggers.
The following API call pops a Squirrel function from the stack and sets it as error handler.::

    SQUIRREL_API void sq_seterrorhandler(HSQUIRRELVM v);

The error handler is called with 2 parameters, an environment object (this) and a object.
The object can be any squirrel type.
