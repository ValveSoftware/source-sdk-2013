.. _datatypes_and_values:

=====================
Values and Data types
=====================

While Squirrel is a dynamically typed language and variables do not
have a type, different operations may interpret the variable as
containing a type.  Squirrel's basic types are integer, float, string,
null, table, array, function, generator, class, instance, bool, thread
and userdata.

.. _userdata-index:

--------
Integer
--------

An Integer represents a 32 bit (or better) signed number.::

    local a = 123 //decimal
    local b = 0x0012 //hexadecimal
    local c = 075 //octal
    local d = 'w' //char code

--------
Float
--------

A float represents a 32 bit (or better) floating point number.::

    local a=1.0
    local b=0.234

--------
String
--------

Strings are an immutable sequence of characters. In order to modify a
string is it necessary create a new one.

Squirrel's strings are similar to strings in C or C++.  They are
delimited by quotation marks(``"``) and can contain escape
sequences (``\t``, ``\a``, ``\b``, ``\n``, ``\r``, ``\v``, ``\f``,
``\\``, ``\"``, ``\'``, ``\0``, ``\x<hh>``, ``\u<hhhh>`` and
``\U<hhhhhhhh>``).

Verbatim string literals do not interpret escape sequences. They begin
with ``@"`` and end with the matching quote.  Verbatim string literals
also can extend over a line break. If they do, they include any white
space characters between the quotes: ::

    local a = "I'm a wonderful string\n"
    // has a newline at the end of the string
    local x = @"I'm a verbatim string\n"
    // the \n is literal, similar to "\\n" in a regular string.

However, a doubled quotation mark within a verbatim string is replaced
by a single quotation mark: ::

    local multiline = @"
        this is a multiline string
        it will ""embed"" all the new line
        characters
    "

--------
Null
--------

The null value is a primitive value that represents the null, empty, or non-existent
reference. The type Null has exactly one value, called null.::

    local a = null

--------
Bool
--------

Bool is a double-valued (Boolean) data type. Its literals are ``true``
and ``false``. A bool value expresses the validity of a condition
(tells whether the condition is true or false).::

    local a = true;

--------
Table
--------

Tables are associative containers implemented as a set of key/value pairs
called slots.::

    local t={}
    local test=
    {
        a=10
        b=function(a) { return a+1; }
    }

--------
Array
--------

Arrays are simple sequence of objects. Their size is dynamic and their index always starts from 0.::

    local a  = ["I'm","an","array"]
    local b = [null]
    b[0] = a[2];

--------
Function
--------

Functions are similar to those in other C-like languages with a few key differences (see below).

--------
Class
--------

Classes are associative containers implemented as sets of key/value
pairs. Classes are created through a 'class expression' or a 'class
statement'. class members can be inherited from another class object
at creation time. After creation, members can be added until an
instance of the class is created.

--------------
Class Instance
--------------

Class instances are created by calling a *class object*. Instances, as
tables, are implemented as sets of key/value pairs. Instance members
cannot be dynamically added or removed; however the value of the
members can be changed.

---------
Generator
---------

Generators are functions that can be suspended with the statement
'yield' and resumed later (see :ref:`Generators <generators>`).

---------
Userdata
---------

Userdata objects are blobs of memory or pointers defined by the host
application but stored within Squirrel variables (See :ref:`Userdata
and UserPointers <embedding_userdata_and_userpointers>`).

---------
Thread
---------

Threads are objects representing a cooperative thread of execution,
also known as coroutines.

--------------
Weak Reference
--------------

Weak References are objects that point to another (non-scalar) object but do not own a strong reference to it.
(See :ref:`Weak References <weak_references>`).
