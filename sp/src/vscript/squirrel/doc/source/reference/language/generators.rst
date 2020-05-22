.. _generators:


=================
Generators
=================

.. index::
    single: Generators

A function that contains a ``yield`` statement is called *'generator function'* .
When a generator function is called, it does not execute the function body, instead it
returns a new suspended generator.
The returned generator can be resumed through the resume statement while it is alive.
The yield keyword, suspends the execution of a generator and optionally returns the
result of an expression to the function that resumed the generator.
The generator dies when it returns, this can happen through an explicit return
statement or by exiting the function body; If an unhandled exception (or runtime error)
occurs while a generator is running, the generator will automatically die. A dead
generator cannot be resumed anymore.::

    function geny(n)
    {
        for(local i=1;i<=n;i+=1)
            yield i;
        return null;
    }

    local gtor=geny(10);
    local x;
    while(x=resume gtor) print(x+"\n");

the output of this program will be::

    1
    2
    3
    4
    5
    6
    7
    8
    9
    10

generators can also be iterated using the foreach statement. When a generator is evaluated
by ``foreach``, the generator will be resumed for each iteration until it returns. The value
returned by the ``return`` statement will be ignored.

.. note:: A suspended generator will hold a strong reference to all the values stored in it's local variables except the ``this``
        object that is only a weak reference. A running generator hold a strong reference also to the ``this`` object.
