.. _embedding_tables_and_arrays_manipulation:

==============================
Tables and arrays manipulation
==============================

A new table is created calling sq_newtable, this function pushes a new table in the stack.::

    void sq_newtable(HSQUIRRELVM v);

To create a new slot::

    SQRESULT sq_newslot(HSQUIRRELVM v,SQInteger idx,SQBool bstatic);

To set or get the table delegate::

    SQRESULT sq_setdelegate(HSQUIRRELVM v,SQInteger idx);
    SQRESULT sq_getdelegate(HSQUIRRELVM v,SQInteger idx);


A new array is created calling sq_newarray, the function pushes a new array in the
stack; if the parameters size is bigger than 0 the elements are initialized to null.::

    void sq_newarray (HSQUIRRELVM v,SQInteger size);

To append a value to the back of the array::

    SQRESULT sq_arrayappend(HSQUIRRELVM v,SQInteger idx);

To remove a value from the back of the array::

    SQRESULT sq_arraypop(HSQUIRRELVM v,SQInteger idx,SQInteger pushval);

To resize the array::

    SQRESULT sq_arrayresize(HSQUIRRELVM v,SQInteger idx,SQInteger newsize);

To retrieve the size of a table or an array you must use sq_getsize()::

    SQInteger sq_getsize(HSQUIRRELVM v,SQInteger idx);

To set a value in an array or table::

    SQRESULT sq_set(HSQUIRRELVM v,SQInteger idx);

To get a value from an array or table::

    SQRESULT sq_get(HSQUIRRELVM v,SQInteger idx);

To get or set a value from a table without employing delegation::

    SQRESULT sq_rawget(HSQUIRRELVM v,SQInteger idx);
    SQRESULT sq_rawset(HSQUIRRELVM v,SQInteger idx);

To iterate a table or an array::

    SQRESULT sq_next(HSQUIRRELVM v,SQInteger idx);

Here an example of how to perform an iteration: ::

    //push your table/array here
    sq_pushnull(v)  //null iterator
    while(SQ_SUCCEEDED(sq_next(v,-2)))
    {
        //here -1 is the value and -2 is the key

        sq_pop(v,2); //pops key and val before the nex iteration
    }

    sq_pop(v,1); //pops the null iterator
