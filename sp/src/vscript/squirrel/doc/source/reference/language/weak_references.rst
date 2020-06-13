.. _weak_references:


========================
Weak References
========================

.. index::
    single: Weak References


The weak references allows the programmers to create references to objects without
influencing the lifetime of the object itself.
In squirrel Weak references are first-class objects created through the built-in method obj.weakref().
All types except null implement the weakref() method; however in bools, integers, and floats the method
simply returns the object itself(this because this types are always passed by value).
When a weak references is assigned to a container (table slot,array,class or
instance) is treated differently than other objects; When a container slot that hold a weak
reference is fetched, it always returns the value pointed by the weak reference instead of the weak
reference object. This allow the programmer to ignore the fact that the value handled is weak.
When the object pointed by weak reference is destroyed, the weak reference is automatically set to null.::

    local t = {}
    local a = ["first","second","third"]
    //creates a weakref to the array and assigns it to a table slot
    t.thearray <- a.weakref();

The table slot 'thearray' contains a weak reference to an array.
The following line prints "first", because tables(and all other containers) always return
the object pointed by a weak ref::

    print(t.thearray[0]);

the only strong reference to the array is owned by the local variable 'a', so
because the following line assigns a integer to 'a' the array is destroyed.::

    a = 123;

When an object pointed by a weak ref is destroyed the weak ref is automatically set to null,
so the following line will print "null".::

    ::print(typeof(t.thearray))

-----------------------------------
Handling weak references explicitly
-----------------------------------

If a weak reference is assigned to a local variable, then is treated as any other value.::

    local t = {}
    local weakobj = t.weakref();

the following line prints "weakref".::

    ::print(typeof(weakobj))

the object pointed by the weakref can be obtained through the built-in method weakref.ref().

The following line prints "table".::

    ::print(typeof(weakobj.ref()))
