.. _embedding_creating_a_c_function:

===================
Create a C function
===================

A native C function must have the following prototype: ::

    typedef SQInteger (*SQFUNCTION)(HSQUIRRELVM);

The parameters is an handle to the calling VM and the return value is an integer
respecting the following rules:

* 1 if the function returns a value
* 0 if the function does not return a value
* SQ_ERROR runtime error is thrown

In order to obtain a new callable squirrel function from a C function pointer, is necessary
to call sq_newclosure() passing the C function to it; the new Squirrel function will be
pushed in the stack.

When the function is called, the stackbase is the first parameter of the function and the
top is the last. In order to return a value the function has to push it in the stack and
return 1.

Function parameters are in the stack from position 1 ('this') to *n*.
*sq_gettop()* can be used to determinate the number of parameters.

If the function has free variables, those will be in the stack after the explicit parameters
an can be handled as normal parameters. Note also that the value returned by *sq_gettop()* will be
affected by free variables. *sq_gettop()* will return the number of parameters plus
number of free variables.

Here an example, the following function print the value of each argument and return the
number of arguments. ::

    SQInteger print_args(HSQUIRRELVM v)
    {
        SQInteger nargs = sq_gettop(v); //number of arguments
        for(SQInteger n=1;n<=nargs;n++)
        {
            printf("arg %d is ",n);
            switch(sq_gettype(v,n))
            {
                case OT_NULL:
                    printf("null");
                    break;
                case OT_INTEGER:
                    printf("integer");
                    break;
                case OT_FLOAT:
                    printf("float");
                    break;
                case OT_STRING:
                    printf("string");
                    break;
                case OT_TABLE:
                    printf("table");
                    break;
                case OT_ARRAY:
                    printf("array");
                    break;
                case OT_USERDATA:
                    printf("userdata");
                    break;
                case OT_CLOSURE:
                    printf("closure(function)");
                    break;
                case OT_NATIVECLOSURE:
                    printf("native closure(C function)");
                    break;
                case OT_GENERATOR:
                    printf("generator");
                    break;
                case OT_USERPOINTER:
                    printf("userpointer");
                    break;
                case OT_CLASS:
                    printf("class");
                    break;
                case OT_INSTANCE:
                    printf("instance");
                    break;
                case OT_WEAKREF:
                    printf("weak reference");
                    break;
                default:
                    return sq_throwerror(v,"invalid param"); //throws an exception
            }
        }
        printf("\n");
        sq_pushinteger(v,nargs); //push the number of arguments as return value
        return 1; //1 because 1 value is returned
    }

Here an example of how to register a function::

    SQInteger register_global_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname)
    {
        sq_pushroottable(v);
        sq_pushstring(v,fname,-1);
        sq_newclosure(v,f,0); //create a new function
        sq_newslot(v,-3,SQFalse);
        sq_pop(v,1); //pops the root table
        return 0;
    }
