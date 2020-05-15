.. _classes:


=================
Classes
=================

.. index::
    single: Classes

Squirrel implements a class mechanism similar to languages like Java/C++/etc...
however because of its dynamic nature it differs in several aspects.
Classes are first class objects like integer or strings and can be stored in
table slots local variables, arrays and passed as function parameters.

-----------------
Class Declaration
-----------------

.. index::
    pair: declaration; Class
    single: Class Declaration

A class object is created through the keyword 'class' . The class object follows
the same declaration syntax of a table(see :ref:`Tables <tables>`) with the only difference
of using ';' as optional separator rather than ','.

For instance: ::

    class Foo {
        //constructor
        constructor(a)
        {
            testy = ["stuff",1,2,3,a];
        }
        //member function
        function PrintTesty()
        {
            foreach(i,val in testy)
            {
                ::print("idx = "+i+" = "+val+" \n");
            }
        }
        //property
        testy = null;

    }

the previous code example is a syntactic sugar for: ::

    Foo <- class {
        //constructor
        constructor(a)
        {
            testy = ["stuff",1,2,3,a];
        }
        //member function
        function PrintTesty()
        {
            foreach(i,val in testy)
            {
                ::print("idx = "+i+" = "+val+" \n");
            }
        }
        //property
        testy = null;

    }

in order to emulate namespaces, it is also possible to declare something like this::

    //just 2 regular nested tables
    FakeNamespace <- {
        Utils = {}
    }

    class FakeNamespace.Utils.SuperClass {
        constructor()
        {
            ::print("FakeNamespace.Utils.SuperClass")
        }
        function DoSomething()
        {
            ::print("DoSomething()")
        }
    }

    function FakeNamespace::Utils::SuperClass::DoSomethingElse()
    {
        ::print("FakeNamespace::Utils::SuperClass::DoSomethingElse()")
    }

    local testy = FakeNamespace.Utils.SuperClass();
    testy.DoSomething();
    testy.DoSomethingElse();

After its declaration, methods or properties can be added or modified by following
the same rules that apply to a table(operator ``<-``).::

    //adds a new property
    Foo.stuff <- 10;

    //modifies the default value of an existing property
    Foo.testy <- "I'm a string";

    //adds a new method
    function Foo::DoSomething(a,b)
    {
        return a+b;
    }

After a class is instantiated is no longer possible to add new properties however is possible to add or replace methods.

^^^^^^^^^^^^^^^^
Static variables
^^^^^^^^^^^^^^^^

.. index::
    pair: static variables; Class
    single: Static variables

Squirrel's classes support static member variables. A static variable shares its value
between all instances of the class. Statics are declared by prefixing the variable declaration
with the keyword ``static``; the declaration must be in the class body.

.. note:: Statics are read-only.

::

    class Foo {
        constructor()
        {
            //..stuff
        }
        name = "normal variable";
        //static variable
        static classname = "The class name is foo";
    };

^^^^^^^^^^^^^^^^
Class Attributes
^^^^^^^^^^^^^^^^

.. index::
    pair: attributes; Class
    single: Class Attributes

Classes allow to associate attributes to it's members. Attributes are a form of metadata
that can be used to store application specific informations, like documentations
strings, properties for IDEs, code generators etc...
Class attributes are declared in the class body by preceding the member declaration and
are delimited by the symbol ``</`` and ``/>``.
Here an example: ::

    class Foo </ test = "I'm a class level attribute" />{
        </ test = "freakin attribute" /> //attributes of PrintTesty
        function PrintTesty()
        {
            foreach(i,val in testy)
            {
                ::print("idx = "+i+" = "+val+" \n");
            }
        }
        </ flippy = 10 , second = [1,2,3] /> //attributes of testy
        testy = null;
    }

Attributes are, matter of fact, a table. Squirrel uses ``</ />`` syntax
instead of curly brackets ``{}`` for the attribute declaration to increase readability.

This means that all rules that apply to tables apply to attributes.

Attributes can be retrieved through the built-in function ``classobj.getattributes(membername)`` (see <link linkend="builtin">built-in functions</link>).
and can be modified/added through the built-in function ``classobj.setattributes(membername,val)``.

the following code iterates through the attributes of all Foo members.::

    foreach(member,val in Foo)
    {
        ::print(member+"\n");
        local attr;
        if((attr = Foo.getattributes(member)) != null) {
            foreach(i,v in attr)
            {
                ::print("\t"+i+" = "+(typeof v)+"\n");
            }
        }
        else {
            ::print("\t<no attributes>\n")
        }
    }

-----------------
Class Instances
-----------------

.. index::
    pair: instances; Class
    single: Class Instances

The class objects inherits several of the table's feature with the difference that multiple instances of the
same class can be created.
A class instance is an object that share the same structure of the table that created it but
holds is own values.
Class *instantiation* uses function notation.
A class instance is created by calling a class object. Can be useful to imagine a class like a function
that returns a class instance.::

    //creates a new instance of Foo
    local inst = Foo();

When a class instance is created its member are initialized *with the same value* specified in the
class declaration. The values are copied verbatim, *no cloning is performed* even if the value is a container or a class instances.

.. note:: FOR C# and Java programmers:

            Squirrel doesn't clone member's default values nor executes the member declaration for each instance(as C# or java).

            So consider this example: ::

                class Foo {
                  myarray = [1,2,3]
                  mytable = {}
                }

                local a = Foo();
                local b = Foo();

            In the snippet above both instances will refer to the same array and same table.To achieve what a C# or Java programmer would
            expect, the following approach should be taken. ::

                class Foo {
                  myarray = null
                  mytable = null
                  constructor()
                  {
                    myarray = [1,2,3]
                    mytable = {}
                  }
                }

                local a = Foo();
                local b = Foo();

When a class defines a method called 'constructor', the class instantiation operation will
automatically invoke it for the newly created instance.
The constructor method can have parameters, this will impact on the number of parameters
that the *instantiation operation* will require.
Constructors, as normal functions, can have variable number of parameters (using the parameter ``...``).::

    class Rect {
        constructor(w,h)
        {
            width = w;
            height = h;
        }
        x = 0;
        y = 0;
        width = null;
        height = null;
    }

    //Rect's constructor has 2 parameters so the class has to be 'called'
    //with 2 parameters
    local rc = Rect(100,100);

After an instance is created, its properties can be set or fetched following the
same rules that apply to tables. Methods cannot be set.

Instance members cannot be removed.

The class object that created a certain instance can be retrieved through the built-in function
``instance.getclass()`` (see :ref:`built-in functions <builtin_functions>`)

The operator ``instanceof`` tests if a class instance is an instance of a certain class.::

    local rc = Rect(100,100);
    if(rc instanceof ::Rect) {
        ::print("It's a rect");
    }
    else {
        ::print("It isn't a rect");
    }

.. note:: Since Squirrel 3.x instanceof doesn't throw an exception if the left expression is not a class, it simply fails

--------------
Inheritance
--------------

.. index::
    pair: inheritance; Class
    single: Inheritance

Squirrel's classes support single inheritance by adding the keyword ``extends``, followed
by an expression, in the class declaration.
The syntax for a derived class is the following: ::

    class SuperFoo extends Foo {
        function DoSomething() {
            ::print("I'm doing something");
        }
    }

When a derived class is declared, Squirrel first copies all base's members in the
new class then proceeds with evaluating the rest of the declaration.

A derived class inherit all members and properties of it's base, if the derived class
overrides a base function the base implementation is shadowed.
It's possible to access a overridden method of the base class by fetching the method from
through the 'base' keyword.

Here an example: ::

    class Foo {
        function DoSomething() {
            ::print("I'm the base");
        }
    };

    class SuperFoo extends Foo {
        //overridden method
        function DoSomething() {
            //calls the base method
            base.DoSomething();
            ::print("I'm doing something");
        }
    }

Same rule apply to the constructor. The constructor is a regular function (apart from being automatically invoked on construction).::

    class BaseClass {
        constructor()
        {
            ::print("Base constructor\n");
        }
    }

    class ChildClass extends BaseClass {
        constructor()
        {
            base.constructor();
            ::print("Child constructor\n");
        }
    }

    local test = ChildClass();

The base class of a derived class can be retrieved through the built-in method ``getbase()``.::

    local thebaseclass = SuperFoo.getbase();

Note that because methods do not have special protection policies when calling methods of the same
objects, a method of a base class that calls a method of the same class can end up calling a overridden method of the derived class.

A method of a base class can be explicitly invoked by a method of a derived class though the keyword ``base`` (as in base.MyMethod() ).::

    class Foo {
        function DoSomething() {
            ::print("I'm the base");
        }
        function DoIt()
        {
            DoSomething();
        }
    };

    class SuperFoo extends Foo {
        //overridden method
        function DoSomething() {
            ::print("I'm the derived");

        }
        function DoIt() {
            base.DoIt();
        }
    }

    //creates a new instance of SuperFoo
    local inst = SuperFoo();

    //prints "I'm the derived"
    inst.DoIt();

----------------------
Metamethods
----------------------

.. index::
    pair: metamethods; Class
    single: Class metamethods

Class instances allow the customization of certain aspects of the
their semantics through metamethods(see see :ref:`Metamethods <metamethods>`).
For C++ programmers: "metamethods behave roughly like overloaded operators".
The metamethods supported by classes are ``_add, _sub, _mul, _div, _unm, _modulo,
_set, _get, _typeof, _nexti, _cmp, _call, _delslot, _tostring``

Class objects instead support only 2 metamethods : ``_newmember`` and ``_inherited``

the following example show how to create a class that implements the metamethod ``_add``.::

    class Vector3 {
        constructor(...)
        {
            if(vargv.len() >= 3) {
                x = vargv[0];
                y = vargv[1];
                z = vargv[2];
            }
        }
        function _add(other)
        {
            return ::Vector3(x+other.x,y+other.y,z+other.z);
        }

        x = 0;
        y = 0;
        z = 0;
    }

    local v0 = Vector3(1,2,3)
    local v1 = Vector3(11,12,13)
    local v2 = v0 + v1;
    ::print(v2.x+","+v2.y+","+v2.z+"\n");

Since version 2.1, classes support 2 metamethods ``_inherited`` and ``_newmember``.
``_inherited`` is invoked when a class inherits from the one that implements ``_inherited``.
``_newmember`` is invoked for each member that is added to the class(at declaration time).
