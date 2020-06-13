.. _stdlib_stdstringlib:

==================
The String library
==================

the string lib implements string formatting and regular expression matching routines.

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++

.. js:function:: endswith(str, cmp)

    returns `true` if the end of the string `str`  matches a the string `cmp` otherwise returns `false`

.. js:function:: escape(str)

    Returns a string with backslashes before characters that need to be escaped(`\",\a,\b,\t,\n,\v,\f,\r,\\,\",\',\0,\xnn`).

.. js:function:: format(formatstr, ...)

    Returns a string formatted according `formatstr` and the optional parameters following it.
    The format string follows the same rules as the `printf` family of
    standard C functions( the "*" is not supported). ::

        e.g.
        sq> print(format("%s %d 0x%02X\n","this is a test :",123,10));
        this is a test : 123 0x0A

.. js:function:: printf(formatstr, ...)

    Just like calling `print(format(formatstr` as in the example above, but is more convenient AND more efficient. ::

        e.g.
        sq> printf("%s %d 0x%02X\n","this is a test :",123,10);
        this is a test : 123 0x0A

.. js:function:: lstrip(str)

    Strips white-space-only characters that might appear at the beginning of the given string
    and returns the new stripped string.

.. js:function:: rstrip(str)

    Strips white-space-only characters that might appear at the end of the given string
    and returns the new stripped string.

.. js:function:: split(str, separators)

    returns an array of strings split at each point where a separator character occurs in `str`.
    The separator is not returned as part of any array element.
    The parameter `separators` is a string that specifies the characters as to be used for the splitting.

    ::

        eg.
        local a = split("1.2-3;4/5",".-/;");
        // the result will be  [1,2,3,4,5]


.. js:function:: startswith(str, cmp)

    returns `true` if the beginning of the string `str` matches the string `cmp`; otherwise returns `false`

.. js:function:: strip(str)

    Strips white-space-only characters that might appear at the beginning or end of the given string and returns the new stripped string.

++++++++++++++++++
The regexp class
++++++++++++++++++

.. js:class:: regexp(pattern)

    The regexp object represents a precompiled regular expression pattern. The object is created
    through `regexp(pattern)`.


+---------------------+--------------------------------------+
|      `\\`           |  Quote the next metacharacter        |
+---------------------+--------------------------------------+
|      `^`            |  Match the beginning of the string   |
+---------------------+--------------------------------------+
|      `.`            |  Match any character                 |
+---------------------+--------------------------------------+
|      `$`            |  Match the end of the string         |
+---------------------+--------------------------------------+
|      `|`            |  Alternation                         |
+---------------------+--------------------------------------+
|      `(subexp)`     |  Grouping (creates a capture)        |
+---------------------+--------------------------------------+
|      `(?:subexp)`   |  No Capture Grouping (no capture)    |
+---------------------+--------------------------------------+
|      `[]`           |  Character class                     |
+---------------------+--------------------------------------+

**GREEDY CLOSURES**

+---------------------+---------------------------------------------+
|      `*`            |  Match 0 or more times                      |
+---------------------+---------------------------------------------+
|      `+`            |  Match 1 or more times                      |
+---------------------+---------------------------------------------+
|      `?`            |  Match 1 or 0 times                         |
+---------------------+---------------------------------------------+
|      `{n}`          |  Match exactly n times                      |
+---------------------+---------------------------------------------+
|      `{n,}`         |  Match at least n times                     |
+---------------------+---------------------------------------------+
|      `{n,m}`        |  Match at least n but not more than m times |
+---------------------+---------------------------------------------+

**ESCAPE CHARACTERS**

+---------------------+--------------------------------------+
|      `\\t`          |  tab (HT, TAB)                       |
+---------------------+--------------------------------------+
|      `\\n`          |  newline (LF, NL)                    |
+---------------------+--------------------------------------+
|      `\\r`          | return (CR)                          |
+---------------------+--------------------------------------+
|      `\\f`          |  form feed (FF)                      |
+---------------------+--------------------------------------+

**PREDEFINED CLASSES**

+---------------------+--------------------------------------+
|      `\\l`          |  lowercase next char                 |
+---------------------+--------------------------------------+
|      `\\u`          |  uppercase next char                 |
+---------------------+--------------------------------------+
|      `\\a`          |  letters                             |
+---------------------+--------------------------------------+
|      `\\A`          |  non letters                         |
+---------------------+--------------------------------------+
|      `\\w`          |  alphanumeric `[_0-9a-zA-Z]`         |
+---------------------+--------------------------------------+
|      `\\W`          |  non alphanumeric `[^_0-9a-zA-Z]`    |
+---------------------+--------------------------------------+
|      `\\s`          |  space                               |
+---------------------+--------------------------------------+
|      `\\S`          |  non space                           |
+---------------------+--------------------------------------+
|      `\\d`          |  digits                              |
+---------------------+--------------------------------------+
|      `\\D`          |  non digits                          |
+---------------------+--------------------------------------+
|      `\\x`          |  hexadecimal digits                  |
+---------------------+--------------------------------------+
|      `\\X`          |  non hexadecimal digits              |
+---------------------+--------------------------------------+
|      `\\c`          |  control characters                  |
+---------------------+--------------------------------------+
|      `\\C`          |  non control characters              |
+---------------------+--------------------------------------+
|      `\\p`          |  punctuation                         |
+---------------------+--------------------------------------+
|      `\\P`          |  non punctuation                     |
+---------------------+--------------------------------------+
|      `\\b`          |  word boundary                       |
+---------------------+--------------------------------------+
|      `\\B`          |  non word boundary                   |
+---------------------+--------------------------------------+


.. js:function:: regexp.capture(str [, start])

    returns an array of tables containing two indexes ("begin" and "end") of
    the first match of the regular expression in the string `str`.
    An array entry is created for each captured sub expressions. If no match occurs returns null.
    The search starts from the index `start`
    of the string; if `start` is omitted the search starts from the beginning of the string.

    The first element of the returned array(index 0) always contains the complete match.

    ::

        local ex = regexp(@"(\d+) ([a-zA-Z]+)(\p)");
        local string = "stuff 123 Test;";
        local res = ex.capture(string);
        foreach(i,val in res)
        {
            print(format("match number[%02d] %s\n",
                    i,string.slice(val.begin,val.end))); //prints "Test"
        }

        ...
        will print
        match number[00] 123 Test;
        match number[01] 123
        match number[02] Test
        match number[03] ;

.. js:function:: regexp.match(str)

    returns a true if the regular expression matches the string
    `str`, otherwise returns false.

.. js:function:: regexp.search(str [, start])

    returns a table containing two indexes ("begin" and "end") of the first match of the regular expression in
    the string `str`, otherwise if no match occurs returns null. The search starts from the index `start`
    of the string; if `start` is omitted the search starts from the beginning of the string.

    ::

        local ex = regexp("[a-zA-Z]+");
        local string = "123 Test;";
        local res = ex.search(string);
        print(string.slice(res.begin,res.end)); //prints "Test"

-------------
C API
-------------

.. _sqstd_register_stringlib:

.. c:function:: SQRESULT sqstd_register_stringlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the string library in the given VM.

+++++++++++++
Formatting
+++++++++++++

.. c:function:: SQRESULT sqstd_format(HSQUIRRELVM v, SQInteger nformatstringidx, SQInteger* outlen, SQChar** output)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger nformatstringidx: index in the stack of the format string
    :param SQInteger* outlen: a pointer to an integer that will be filled with the length of the newly created string
    :param SQChar** output: a pointer to a string pointer that will receive the newly created string
    :returns: an SQRESULT
    :remarks: the newly created string is allocated in the scratchpad memory.


    creates a new string formatted according to the object at position `nformatstringidx` and the optional parameters following it.
    The format string follows the same rules as the `printf` family of
    standard C functions( the "*" is not supported).

++++++++++++++++++
Regular Expessions
++++++++++++++++++

.. c:function:: SQRex* sqstd_rex_compile(const SQChar *pattern, const SQChar ** error)

    :param SQChar* pattern: a pointer to a zero terminated string containing the pattern that has to be compiled.
    :param SQChar** error: a pointer to a string pointer that will be set with an error string in case of failure.
    :returns: a pointer to the compiled pattern

    compiles an expression and returns a pointer to the compiled version.
    in case of failure returns NULL.The returned object has to be deleted
    through the function sqstd_rex_free().

.. c:function:: void sqstd_rex_free(SQRex * exp)

    :param SQRex* exp: the expression structure that has to be deleted.

    deletes a expression structure created with sqstd_rex_compile()

.. c:function:: SQBool sqstd_rex_match(SQRex * exp,const SQChar * text)

    :param SQRex* exp: a compiled expression
    :param SQChar* text: the string that has to be tested
    :returns: SQTrue if successful otherwise SQFalse

    returns SQTrue if the string specified in the parameter text is an
    exact match of the expression, otherwise returns SQFalse.

.. c:function:: SQBool sqstd_rex_search(SQRex * exp, const SQChar * text, const SQChar ** out_begin, const SQChar ** out_end)

    :param SQRex* exp: a compiled expression
    :param SQChar* text: the string that has to be tested
    :param SQChar** out_begin: a pointer to a string pointer that will be set with the beginning of the match
    :param SQChar** out_end: a pointer to a string pointer that will be set with the end of the match
    :returns: SQTrue if successful otherwise SQFalse

    searches the first match of the expression in the string specified in the parameter text.
    if the match is found returns SQTrue and the sets out_begin to the beginning of the
    match and out_end at the end of the match; otherwise returns SQFalse.

.. c:function:: SQBool sqstd_rex_searchrange(SQRex * exp, const SQChar * text_begin, const SQChar * text_end, const SQChar ** out_begin, const SQChar ** out_end)

    :param SQRex* exp: a compiled expression
    :param SQChar* text_begin:  a pointer to the beginnning of the string that has to be tested
    :param SQChar* text_end: a pointer to the end of the string that has to be tested
    :param SQChar** out_begin: a pointer to a string pointer that will be set with the beginning of the match
    :param SQChar** out_end: a pointer to a string pointer that will be set with the end of the match
    :returns: SQTrue if successful otherwise SQFalse

    searches the first match of the expression in the string delimited
    by the parameter text_begin and text_end.
    if the match is found returns SQTrue and sets out_begin to the beginning of the
    match and out_end at the end of the match; otherwise returns SQFalse.

.. c:function:: SQInteger sqstd_rex_getsubexpcount(SQRex * exp)

    :param SQRex* exp: a compiled expression
    :returns: the number of sub expressions matched by the expression

    returns the number of sub expressions matched by the expression

.. c:function:: SQBool sqstd_rex_getsubexp(SQRex * exp, SQInteger n, SQRexMatch *subexp)

    :param SQRex* exp: a compiled expression
    :param SQInteger n: the index of the submatch(0 is the complete match)
    :param SQRexMatch* a: pointer to structure that will store the result
    :returns: the function returns SQTrue if n is a valid index; otherwise SQFalse.

    retrieve the begin and and pointer to the length of the sub expression indexed
    by n. The result is passed through the struct SQRexMatch.
