.. _functions:


=================
Functions
=================

.. index::
    single: Functions

Functions are first class values like integer or strings and can be stored in table slots,
local variables, arrays and passed as function parameters.
Functions can be implemented in Squirrel or in a native language with calling conventions
compatible with ANSI C.

--------------------
Function declaration
--------------------

.. index::
    single: Function Declaration

Functions are declared through the function expression::

    local a = function(a, b, c) { return a + b - c; }

or with the syntactic sugar::

    function ciao(a,b,c)
    {
        return a+b-c;
    }

that is equivalent to::

    this.ciao <- function(a,b,c)
    {
        return a+b-c;
    }

a local function can be declared with this syntactic sugar::

    local function tuna(a,b,c)
    {
        return a+b-c;
    }

that is equivalent to::

    local tuna = function(a,b,c)
    {
        return a+b-c;
    }

is also possible to declare something like::

    T <- {}
    function T::ciao(a,b,c)
    {
        return a+b-c;
    }

    //that is equivalent to write

    T.ciao <- function(a,b,c)
    {
        return a+b-c;
    }

    //or

    T <- {
        function ciao(a,b,c)
        {
            return a+b-c;
        }
    }

^^^^^^^^^^^^^^^^^^
Default Paramaters
^^^^^^^^^^^^^^^^^^

.. index::
    single: Function Default Paramaters

Squirrel's functions can have default parameters.

A function with default parameters is declared as follows: ::

    function test(a,b,c = 10, d = 20)
    {
        ....
    }

when the function *test* is invoked and the parameter c or d are not specified,
the VM autometically assigns the default value to the unspecified parameter. A default parameter can be
any valid squirrel expression. The expression is evaluated at runtime.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function with variable number of paramaters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    single: Function with variable number of paramaters

Squirrel's functions can have variable number of parameters(varargs functions).

A vararg function is declared by adding three dots (`...`) at the end of its parameter list.

When the function is called all the extra parameters will be accessible through the *array*
called ``vargv``, that is passed as implicit parameter.

``vargv`` is a regular squirrel array and can be used accordingly.::

    function test(a,b,...)
    {
        for(local i = 0; i< vargv.len(); i++)
        {
            ::print("varparam "+i+" = "+vargv[i]+"\n");
        }
      foreach(i,val in vargv)
        {
            ::print("varparam "+i+" = "+val+"\n");
        }
    }

    test("goes in a","goes in b",0,1,2,3,4,5,6,7,8);

---------------
Function calls
---------------

.. index::
    single: Function calls

::

    exp:= derefexp '(' explist ')'

The expression is evaluated in this order: derefexp after the explist (arguments) and at
the end the call.

A function call in Squirrel passes the current environment object *this* as a hidden parameter.
But when the function was immediately indexed from an object, *this* shall be the object
which was indexed, instead.

If we call a function with the syntax::

    mytable.foo(x,y)

the environment object passed to 'foo' as *this* will be 'mytable' (since 'foo' was immediately indexed from 'mytable')

Whereas with the syntax::

    foo(x,y) // implicitly equivalent to this.foo(x,y)

the environment object will be the current *this* (that is, propagated from the caller's *this*).

It may help to remember the rules in the following way:

    foo(x,y) ---> this.foo(x,y)
    table.foo(x,y) ---> call foo with (table,x,y)

It may also help to consider why it works this way: it's designed to assist with object-oriented style.
When calling 'foo(x,y)' it's assumed you're calling another member of the object (or of the file) and
so should operate on the same object.
When calling 'mytable.foo(x,y)' it's written plainly that you're calling a member of a different object.

---------------------------------------------
Binding an environment to a function
---------------------------------------------

.. index::
    single: Binding an environment to a function

while by default a squirrel function call passes as environment object 'this', the object
where the function was indexed from. However, is also possible to statically bind an evironment to a
closure using the built-in method ``closure.bindenv(env_obj)``.
The method bindenv() returns a new instance of a closure with the environment bound to it.
When an environment object is bound to a function, every time the function is invoked, its
'this' parameter will always be the previously bound environent.
This mechanism is useful to implement callbacks systems similar to C# delegates.

.. note:: The closure keeps a weak reference to the bound environmet object, because of this if
          the object is deleted, the next call to the closure will result in a ``null``
          environment object.

---------------------------------------------
Lambda Expressions
---------------------------------------------

.. index::
    single: Lambda Expressions

::

    exp := '@' '(' paramlist ')' exp

Lambda expressions are a syntactic sugar to quickly define a function that consists of a single expression.
This feature comes handy when functional programming patterns are applied, like map/reduce or passing a compare method to
array.sort().

here a lambda expression::

    local myexp = @(a,b) a + b

that is equivalent to::

    local myexp = function(a,b) { return a + b; }

a more useful usage could be::

    local arr = [2,3,5,8,3,5,1,2,6];
    arr.sort(@(a,b) a <=> b);
    arr.sort(@(a,b) -(a <=> b));

that could have been written as::

    local arr = [2,3,5,8,3,5,1,2,6];
    arr.sort(function(a,b) { return a <=> b; } );
    arr.sort(function(a,b) { return -(a <=> b); } );

other than being limited to a single expression lambdas support all features of regular functions.
in fact are implemented as a compile time feature.

---------------------------------------------
Free Variables
---------------------------------------------

.. index::
    single: Free Variables

A free variable is a variable external from the function scope as is not a local variable
or parameter of the function.
Free variables reference a local variable from a outer scope.
In the following example the variables 'testy', 'x' and 'y' are bound to the function 'foo'.::

    local x=10,y=20
    local testy="I'm testy"

    function foo(a,b)
    {
        ::print(testy);
        return a+b+x+y;
    }

A program can read or write a free variable.

---------------------------------------------
Tail Recursion
---------------------------------------------

.. index::
    single: Tail Recursion

Tail recursion is a method for partially transforming a recursion in a program into an
iteration: it applies when the recursive calls in a function are the last executed
statements in that function (just before the return).
If this happenes the squirrel interpreter collapses the caller stack frame before the
recursive call; because of that very deep recursions are possible without risk of a stack
overflow.::

    function loopy(n)
    {
        if(n>0){
            ::print("n="+n+"\n");
            return loopy(n-1);
        }
    }

    loopy(1000);

