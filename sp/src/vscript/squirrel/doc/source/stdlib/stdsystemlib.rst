.. _stdlib_stdsystemlib:

==================
The System library
==================

The system library exposes operating system facilities like environment variables,
date time manipulation etc..

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++

.. js:function:: clock()

    returns a float representing the number of seconds elapsed since the start of the process

.. js:function:: date([time [, format]])

    returns a table containing a date/time split into the slots:

+-------------+----------------------------------------+
| sec         | Seconds after minute (0 - 59).         |
+-------------+----------------------------------------+
| min         | Minutes after hour (0 - 59).           |
+-------------+----------------------------------------+
| hour        | Hours since midnight (0 - 23).         |
+-------------+----------------------------------------+
| day         | Day of month (1 - 31).                 |
+-------------+----------------------------------------+
| month       | Month (0 - 11; January = 0).           |
+-------------+----------------------------------------+
| year        | Year (current year).                   |
+-------------+----------------------------------------+
| wday        | Day of week (0 - 6; Sunday = 0).       |
+-------------+----------------------------------------+
| yday        | Day of year (0 - 365; January 1 = 0).  |
+-------------+----------------------------------------+

if `time` is omitted the current time is used.

if `format` can be 'l' local time or 'u' UTC time, if omitted is defaulted as 'l'(local time).

.. js:function:: getenv(varaname)

    Returns a string containing the value of the environment variable `varname`

.. js:function:: remove(path)

    deletes the file specified by `path`

.. js:function:: rename(oldname, newname)

    renames the file or directory specified by `oldname` to the name given by `newname`

.. js:function:: system(cmd)

    xecutes the string `cmd` through the os command interpreter.

.. js:function:: time()

    returns the number of seconds elapsed since midnight 00:00:00, January 1, 1970.

    the result of this function can be formatted through the function `date()`

--------------
C API
--------------

.. _sqstd_register_systemlib:

.. c:function:: SQRESULT sqstd_register_systemlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the system library in the given VM.
