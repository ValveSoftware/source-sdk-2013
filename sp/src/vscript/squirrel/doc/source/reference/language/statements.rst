.. _statements:


=================
Statements
=================

.. index::
    single: statements

A squirrel program is a simple sequence of statements.::

    stats := stat [';'|'\n'] stats

Statements in squirrel are comparable to the C-Family languages (C/C++, Java, C#
etc...): assignment, function calls, program flow control structures etc.. plus some
custom statement like yield, table and array constructors (All those will be covered in detail
later in this document).
Statements can be separated with a new line or ';' (or with the keywords case or default if
inside a switch/case statement), both symbols are not required if the statement is
followed by '}'.

------
Block
------

.. index::
    pair: block; statement

::

    stat := '{' stats '}'

A sequence of statements delimited by curly brackets ({ }) is called block;
a block is a statement itself.

-----------------------
Control Flow Statements
-----------------------

.. index::
    single: control flow statements

squirrel implements the most common control flow statements: ``if, while, do-while, switch-case, for, foreach``

^^^^^^^^^^^^^^
true and false
^^^^^^^^^^^^^^

.. index::
    single: true and false
    single: true
    single: false

Squirrel has a boolean type (bool) however like C++ it considers null, 0(integer) and 0.0(float)
as *false*, any other value is considered *true*.

^^^^^^^^^^^^^^^^^
if/else statement
^^^^^^^^^^^^^^^^^

.. index::
    pair: if/else; statement
    pair: if; statement
    pair: else; statement

::

    stat:= 'if' '(' exp ')' stat ['else' stat]

Conditionally execute a statement depending on the result of an expression.::

    if(a>b)
        a=b;
    else
        b=a;
    ////
    if(a==10)
    {
        b=a+b;
        return a;
    }

^^^^^^^^^^^^^^^^^
while statement
^^^^^^^^^^^^^^^^^

.. index::
    pair: while; statement

::

    stat:= 'while' '(' exp ')' stat

Executes a statement while the condition is true.::

    function testy(n)
    {
        local a=0;
        while(a<n) a+=1;

        while(1)
        {
            if(a<0) break;
            a-=1;
        }
    }

^^^^^^^^^^^^^^^^^^
do/while statement
^^^^^^^^^^^^^^^^^^

.. index::
    pair: do/while; statement

::

    stat:= 'do' stat 'while' '(' expression ')'

Executes a statement once, and then repeats execution of the statement until a condition
expression evaluates to false.::

    local a=0;
    do
    {
        print(a+"\n");
        a+=1;
    } while(a>100)

^^^^^^^^^^^^^^^^^
switch statement
^^^^^^^^^^^^^^^^^

.. index::
    pair: switch; statement

::

    stat := 'switch' ''( exp ')' '{'
    'case' case_exp ':'
        stats
    ['default' ':'
        stats]
    '}'

Switch is a control statement allows multiple selections of code by passing control to one of the
case statements within its body.
The control is transferred to the case label whose case_exp matches with exp if none of
the case match will jump to the default label (if present).
A switch statement can contain any number if case instances, if 2 case have the same
expression result the first one will be taken in account first. The default label is only
allowed once and must be the last one.
A break statement will jump outside the switch block.

-----
Loops
-----

.. index::
    single: Loops

^^^^^^^^
for
^^^^^^^^

.. index::
    pair: for; statement

::

    stat:= 'for' '(' [initexp] ';' [condexp] ';' [incexp] ')' statement

Executes a statement as long as a condition is different than false.::

    for(local a=0;a<10;a+=1)
        print(a+"\n");
    //or
    glob <- null
    for(glob=0;glob<10;glob+=1){
        print(glob+"\n");
    }
    //or
    for(;;){
        print(loops forever+"\n");
    }

^^^^^^^^
foreach
^^^^^^^^

.. index::
    pair: foreach; statement

::

    'foreach' '(' [index_id','] value_id 'in' exp ')' stat

Executes a statement for every element contained in an array, table, class, string or generator.
If exp is a generator it will be resumed every iteration as long as it is alive; the value will
be the result of 'resume' and the index the sequence number of the iteration starting
from 0.::

    local a=[10,23,33,41,589,56]
    foreach(idx,val in a)
        print("index="+idx+" value="+val+"\n");
    //or
    foreach(val in a)
        print("value="+val+"\n");

-------
break
-------

.. index::
    pair: break; statement

::

    stat := 'break'

The break statement terminates the execution of a loop (for, foreach, while or do/while)
or jumps out of switch statement;

---------
continue
---------

.. index::
    pair: continue; statement

::

    stat := 'continue'

The continue operator jumps to the next iteration of the loop skipping the execution of
the following statements.

---------
return
---------

.. index::
    pair: return; statement

::

    stat:= return [exp]

The return statement terminates the execution of the current function/generator and
optionally returns the result of an expression. If the expression is omitted the function
will return null. If the return statement is used inside a generator, the generator will not
be resumable anymore.

---------
yield
---------

.. index::
    pair: yield; statement

::

    stat := yield [exp]

(see :ref:`Generators <generators>`).


---------------------------
Local variables declaration
---------------------------

.. index::
    pair: Local variables declaration; statement

::

    initz := id [= exp][',' initz]
    stat := 'local' initz

Local variables can be declared at any point in the program; they exist between their
declaration to the end of the block where they have been declared.
*EXCEPTION:* a local declaration statement is allowed as first expression in a for loop.::

    for(local a=0;a<10;a+=1)
        print(a);

--------------------
Function declaration
--------------------

.. index::
    pair: Function declaration; statement

::

    funcname := id ['::' id]
    stat:= 'function' id ['::' id]+ '(' args ')' stat

creates a new function.

-----------------
Class declaration
-----------------

.. index::
    pair: Class declaration; statement

::

    memberdecl := id '=' exp [';'] |    '[' exp ']' '=' exp [';'] | functionstat | 'constructor' functionexp
    stat:= 'class' derefexp ['extends' derefexp] '{'
            [memberdecl]
        '}'

creates a new class.

-----------
try/catch
-----------

.. index::
    pair: try/catch; statement

::

    stat:= 'try' stat 'catch' '(' id ')' stat

The try statement encloses a block of code in which an exceptional condition can occur,
such as a runtime error or a throw statement. The catch clause provides the exception-handling
code. When a catch clause catches an exception, its id is bound to that
exception.

-----------
throw
-----------

.. index::
    pair: throw; statement

::

    stat:= 'throw' exp

Throws an exception. Any value can be thrown.

--------------
const
--------------

.. index::
    pair: const; statement

::

    stat:= 'const' id '=' 'Integer | Float | StringLiteral

Declares a constant (see :ref:`Constants & Enumerations <constants_and_enumerations>`).

--------------
enum
--------------

.. index::
    pair: enum; statement

::

    enumerations := ( 'id' '=' Integer | Float | StringLiteral ) [',']
    stat:= 'enum' id '{' enumerations '}'

Declares an enumeration (see :ref:`Constants & Enumerations <constants_and_enumerations>`).

--------------------
Expression statement
--------------------

.. index::
    pair: Expression statement; statement

::

    stat := exp

In Squirrel every expression is also allowed as statement, if so, the result of the
expression is thrown away.

