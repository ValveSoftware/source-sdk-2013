.. _arrays:


=================
Arrays
=================

.. index::
    single: Arrays

An array is a sequence of values indexed by a integer number from 0 to the size of the
array minus 1. Arrays elements can be obtained through their index.::

    local a=["I'm a string", 123]
    print(typeof a[0]) //prints "string"
    print(typeof a[1]) //prints "integer"

Resizing, insertion, deletion of arrays and arrays elements is done through a set of
standard functions (see :ref:`built-in functions <builtin_functions>`).
