.. _embedding_the_stack:


==========
The Stack
==========

Squirrel exchanges values with the virtual machine through a stack. This mechanism has
been inherited from the language Lua.
For instance to call a Squirrel function from C it is necessary to push the function and the
arguments in the stack and then invoke the function; also when Squirrel calls a C
function the parameters will be in the stack as well.

-------------
Stack indexes
-------------

Many API functions can arbitrarily refer to any element in the stack through an index.
The stack indexes follow those conventions:

* 1 is the stack base
* Negative indexes are considered an offset from top of the stack. For instance -1 isthe top of the stack.
* 0 is an invalid index

Here an example (let's pretend that this table is the VM stack)

+------------+--------------------+--------------------+
| **STACK**  | **positive index** | **negative index** |
+============+====================+====================+
| "test"     | 4                  | -1(top)            |
+------------+--------------------+--------------------+
| 1          | 3                  | -2                 |
+------------+--------------------+--------------------+
| 0.5        | 2                  | -3                 |
+------------+--------------------+--------------------+
| "foo"      | 1(base)            | -4                 |
+------------+--------------------+--------------------+

In this case, the function *sq_gettop* would return 4;

------------------
Stack manipulation
------------------

The API offers several functions to push and retrieve data from the Squirrel stack.

To push a value that is already present in the stack in the top position::

    void sq_push(HSQUIRRELVM v,SQInteger idx);

To pop an arbitrary number of elements::

    void sq_pop(HSQUIRRELVM v,SQInteger nelemstopop);

To remove an element from the stack::

    void sq_remove(HSQUIRRELVM v,SQInteger idx);

To retrieve the top index (and size) of the current
virtual stack you must call *sq_gettop* ::

    SQInteger sq_gettop(HSQUIRRELVM v);

To force the stack to a certain size you can call *sq_settop* ::

    void sq_settop(HSQUIRRELVM v,SQInteger newtop);

If the newtop is bigger than the previous one, the new positions in the stack will be
filled with null values.

The following function pushes a C value into the stack::

    void sq_pushstring(HSQUIRRELVM v,const SQChar *s,SQInteger len);
    void sq_pushfloat(HSQUIRRELVM v,SQFloat f);
    void sq_pushinteger(HSQUIRRELVM v,SQInteger n);
    void sq_pushuserpointer(HSQUIRRELVM v,SQUserPointer p);
    void sq_pushbool(HSQUIRRELVM v,SQBool b);

this function pushes a null into the stack::

    void sq_pushnull(HSQUIRRELVM v);

returns the type of the value in a arbitrary position in the stack::

    SQObjectType sq_gettype(HSQUIRRELVM v,SQInteger idx);

the result can be one of the following values: ::

    OT_NULL,OT_INTEGER,OT_FLOAT,OT_STRING,OT_TABLE,OT_ARRAY,OT_USERDATA,
    OT_CLOSURE,OT_NATIVECLOSURE,OT_GENERATOR,OT_USERPOINTER,OT_BOOL,OT_INSTANCE,OT_CLASS,OT_WEAKREF

The following functions convert a squirrel value in the stack to a C value::

    SQRESULT sq_getstring(HSQUIRRELVM v,SQInteger idx,const SQChar **c);
    SQRESULT sq_getstringandsize(HSQUIRRELVM v,SQInteger idx,const SQChar **c,SQInteger size);
    SQRESULT sq_getinteger(HSQUIRRELVM v,SQInteger idx,SQInteger *i);
    SQRESULT sq_getfloat(HSQUIRRELVM v,SQInteger idx,SQFloat *f);
    SQRESULT sq_getuserpointer(HSQUIRRELVM v,SQInteger idx,SQUserPointer *p);
    SQRESULT sq_getuserdata(HSQUIRRELVM v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag);
    SQRESULT sq_getbool(HSQUIRRELVM v,SQInteger idx,SQBool *p);

The function sq_cmp compares 2 values from the stack and returns their relation (like strcmp() in ANSI C).::

    SQInteger sq_cmp(HSQUIRRELVM v);
