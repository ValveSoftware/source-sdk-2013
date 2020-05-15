.. _lexical_structure:


=================
Lexical Structure
=================

.. index:: single: lexical structure

-----------
Identifiers
-----------

.. index:: single: identifiers

Identifiers start with an alphabetic character or the symbol '_' followed by any number
of alphabetic characters, '_' or digits ([0-9]). Squirrel is a case sensitive language
meaning that the lowercase and uppercase representation of the same alphabetic
character are considered different characters. For instance, "foo", "Foo" and "fOo" are
treated as 3 distinct identifiers.

-----------
Keywords
-----------

.. index:: single: keywords

The following words are reserved and cannot be used as identifiers:

+------------+------------+-----------+------------+------------+-------------+
| base       | break      | case      | catch      | class      | clone       |
+------------+------------+-----------+------------+------------+-------------+
| continue   | const      | default   | delete     | else       | enum        |
+------------+------------+-----------+------------+------------+-------------+
| extends    | for        | foreach   | function   | if         | in          |
+------------+------------+-----------+------------+------------+-------------+
| local      | null       | resume    | return     | switch     | this        |
+------------+------------+-----------+------------+------------+-------------+
| throw      | try        | typeof    | while      | yield      | constructor |
+------------+------------+-----------+------------+------------+-------------+
| instanceof | true       | false     | static     | __LINE__   | __FILE__    |
+------------+------------+-----------+------------+------------+-------------+

Keywords are covered in detail later in this document.

-----------
Operators
-----------

.. index:: single: operators

Squirrel recognizes the following operators:

+----------+----------+----------+----------+----------+----------+----------+----------+
| ``!``    | ``!=``   | ``||``   | ``==``   | ``&&``   | ``>=``   | ``<=``   | ``>``    |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``<=>``  | ``+``    | ``+=``   | ``-``    | ``-=``   | ``/``    | ``/=``   | ``*``    |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``*=``   | ``%``    | ``%=``   | ``++``   | ``--``   | ``<-``   | ``=``    | ``&``    |
+----------+----------+----------+----------+----------+----------+----------+----------+
| ``^``    | ``|``    | ``~``    | ``>>``   | ``<<``   | ``>>>``  |          |          |
+----------+----------+----------+----------+----------+----------+----------+----------+

------------
Other tokens
------------

.. index::
    single: delimiters
    single: other tokens

Other significant tokens are:

+----------+----------+----------+----------+----------+----------+
| ``{``    | ``}``    | ``[``    | ``]``    | ``.``    | ``:``    |
+----------+----------+----------+----------+----------+----------+
| ``::``   | ``'``    | ``;``    | ``"``    | ``@"``   |          |
+----------+----------+----------+----------+----------+----------+

-----------
Literals
-----------

.. index::
    single: literals
    single: string literals
    single: numeric literals

Squirrel accepts integer numbers, floating point numbers and string literals.

+-------------------------------+------------------------------------------+
| ``34``                        | Integer number(base 10)                  |
+-------------------------------+------------------------------------------+
| ``0xFF00A120``                | Integer number(base 16)                  |
+-------------------------------+------------------------------------------+
| ``0753``                      | Integer number(base 8)                   |
+-------------------------------+------------------------------------------+
| ``'a'``                       | Integer number                           |
+-------------------------------+------------------------------------------+
| ``1.52``                      | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``1.e2``                      | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``1.e-2``                     | Floating point number                    |
+-------------------------------+------------------------------------------+
| ``"I'm a string"``            | String                                   |
+-------------------------------+------------------------------------------+
| ``@"I'm a verbatim string"``  | String                                   |
+-------------------------------+------------------------------------------+
| ``@" I'm a``                  |                                          |
| ``multiline verbatim string`` |                                          |
| ``"``                         | String                                   |
+-------------------------------+------------------------------------------+

Pesudo BNF

.. productionlist::
    IntegerLiteral : [1-9][0-9]* | '0x' [0-9A-Fa-f]+ | ''' [.]+ ''' | 0[0-7]+
    FloatLiteral : [0-9]+ '.' [0-9]+
    FloatLiteral : [0-9]+ '.' 'e'|'E' '+'|'-' [0-9]+
    StringLiteral: '"'[.]* '"'
    VerbatimStringLiteral: '@''"'[.]* '"'

-----------
Comments
-----------

.. index:: single: comments

A comment is text that the compiler ignores but that is useful for programmers.
Comments are normally used to embed annotations in the code. The compiler
treats them as white space.

A comment can be ``/*`` (slash, asterisk) characters, followed by any
sequence of characters (including new lines),
followed by the ``*/`` characters. This syntax is the same as ANSI C.::

    /*
    this is
    a multiline comment.
    this lines will be ignored by the compiler
    */

A comment can also be ``//`` (two slashes) characters, followed by any sequence of
characters.  A new line not immediately preceded by a backslash terminates this form of
comment.  It is commonly called a *"single-line comment."*::

    //this is a single line comment. this line will be ignored by the compiler

The character ``#`` is an alternative syntax for single line comment.::

    # this is also a single line comment.

This to facilitate the use of squirrel in UNIX-style shell scripts.
