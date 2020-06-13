.. _stdlib_stdiolib:

========================
The Input/Output library
========================

the i/o library implements basic input/output routines.

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++


.. js:function:: dofile(path, [raiseerror])

    compiles a squirrel script or loads a precompiled one and executes it.
    returns the value returned by the script or null if no value is returned.
    if the optional parameter 'raiseerror' is true, the compiler error handler is invoked
    in case of a syntax error. If raiseerror is omitted or set to false, the compiler
    error handler is not invoked.
    When squirrel is compiled in Unicode mode the function can handle different character encodings,
    UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. js:function:: loadfile(path, [raiseerror])

    compiles a squirrel script or loads a precompiled one an returns it as as function.
    if the optional parameter 'raiseerror' is true, the compiler error handler is invoked
    in case of a syntax error. If raiseerror is omitted or set to false, the compiler
    error handler is not invoked.
    When squirrel is compiled in Unicode mode the function can handle different character encodings,
    UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. js:function:: writeclosuretofile(destpath, closure)

    serializes a closure to a bytecode file (destpath). The serialized file can be loaded
    using loadfile() and dofile().


.. js:data:: stderr

    File object bound on the os *standard error* stream

.. js:data:: stdin

    File object bound on the os *standard input* stream

.. js:data:: stdout

    File object bound on the os *standard output* stream


++++++++++++++
The file class
++++++++++++++

    The file object implements a stream on a operating system file.

.. js:class:: file(path, patten)

    It's constructor imitates the behaviour of the C runtime function fopen for eg. ::

        local myfile = file("test.xxx","wb+");

    creates a file with read/write access in the current directory.

.. js:function:: file.close()

    closes the file.

.. js:function:: file.eos()

    returns a non null value if the read/write pointer is at the end of the stream.

.. js:function:: file.flush()

    flushes the stream.return a value != null if succeeded, otherwise returns null

.. js:function:: file.len()

    returns the length of the stream

.. js:function:: file.readblob(size)

    :param int size: number of bytes to read

    read n bytes from the stream and returns them as blob

.. js:function:: file.readn(type)

    :param int type: type of the number to read

    reads a number from the stream according to the type parameter.

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+----------------------+
| parameter    | return description                                                             |  return type         |
+==============+================================================================================+======================+
| 'l'          | processor dependent, 32bits on 32bits processors, 64bits on 64bits processors  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'i'          | 32bits number                                                                  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 's'          | 16bits signed integer                                                          |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'w'          | 16bits unsigned integer                                                        |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'c'          | 8bits signed integer                                                           |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'b'          | 8bits unsigned integer                                                         |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'f'          | 32bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'd'          | 64bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+

.. js:function:: file.resize(size)

    :param int size: the new size of the blob in bytes

    resizes the blob to the specified `size`

.. js:function:: file.seek(offset [,origin])

    :param int offset: indicates the number of bytes from `origin`.
    :param int origin: origin of the seek

                        +--------------+-------------------------------------------+
                        |  'b'         |  beginning of the stream                  |
                        +--------------+-------------------------------------------+
                        |  'c'         |  current location                         |
                        +--------------+-------------------------------------------+
                        |  'e'         |  end of the stream                        |
                        +--------------+-------------------------------------------+

    Moves the read/write pointer to a specified location.

.. note:: If origin is omitted the parameter is defaulted as 'b'(beginning of the stream).

.. js:function:: file.tell()

    returns the read/write pointer absolute position

.. js:function:: file.writeblob(src)

    :param blob src: the source blob containing the data to be written

    writes a blob in the stream

.. js:function:: file.writen(n, type)

    :param number n: the value to be written
    :param int type: type of the number to write

    writes a number in the stream formatted according to the `type` pamraeter

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+
| parameter    | return description                                                             |
+==============+================================================================================+
| 'i'          | 32bits number                                                                  |
+--------------+--------------------------------------------------------------------------------+
| 's'          | 16bits signed integer                                                          |
+--------------+--------------------------------------------------------------------------------+
| 'w'          | 16bits unsigned integer                                                        |
+--------------+--------------------------------------------------------------------------------+
| 'c'          | 8bits signed integer                                                           |
+--------------+--------------------------------------------------------------------------------+
| 'b'          | 8bits unsigned integer                                                         |
+--------------+--------------------------------------------------------------------------------+
| 'f'          | 32bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+
| 'd'          | 64bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+


--------------
C API
--------------

.. _sqstd_register_iolib:

.. c:function:: SQRESULT sqstd_register_iolib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the io library in the given VM.

++++++++++++++
File Object
++++++++++++++

.. c:function:: SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file, SQBool owns)

    :param HSQUIRRELVM v: the target VM
    :param SQFILE file: the stream that will be rapresented by the file object
    :param SQBool owns: if different true the stream will be automatically closed when the newly create file object is destroyed.
    :returns: an SQRESULT

    creates a file object bound to the SQFILE passed as parameter
    and pushes it in the stack

.. c:function:: SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE* file)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: and index in the stack
    :param SQFILE* file: A pointer to a SQFILE handle that will store the result
    :returns: an SQRESULT

    retrieve the pointer of a stream handle from an arbitrary
    position in the stack.

++++++++++++++++++++++++++++++++
Script loading and serialization
++++++++++++++++++++++++++++++++

.. c:function:: SQRESULT sqstd_loadfile(HSQUIRRELVM v, const SQChar* filename, SQBool printerror)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* filename: path of the script that has to be loaded
    :param SQBool printerror: if true the compiler error handler will be called if a error occurs
    :returns: an SQRESULT

    Compiles a squirrel script or loads a precompiled one an pushes it as closure in the stack.
    When squirrel is compiled in Unicode mode the function can handle different character encodings,
    UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. c:function:: SQRESULT sqstd_dofile(HSQUIRRELVM v, const SQChar* filename, SQBool retval, SQBool printerror)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* filename: path of the script that has to be loaded
    :param SQBool retval: if true the function will push the return value of the executed script in the stack.
    :param SQBool printerror: if true the compiler error handler will be called if a error occurs
    :returns: an SQRESULT
    :remarks: the function expects a table on top of the stack that will be used as 'this' for the execution of the script. The 'this' parameter is left untouched in the stack.

    Compiles a squirrel script or loads a precompiled one and executes it.
    Optionally pushes the return value of the executed script in the stack.
    When squirrel is compiled in unicode mode the function can handle different character encodings,
    UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed, UTF8 encoding is used as default. ::

        sq_pushroottable(v); //push the root table(were the globals of the script will are stored)
        sqstd_dofile(v, _SC("test.nut"), SQFalse, SQTrue);// also prints syntax errors if any

.. c:function:: SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v, const SQChar* filename)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* filename: destination path of serialized closure
    :returns: an SQRESULT

    serializes the closure at the top position in the stack as bytecode in
    the file specified by the parameter filename. If a file with the
    same name already exists, it will be overwritten.

