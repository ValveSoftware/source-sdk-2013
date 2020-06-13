.. _stdlib_stdbloblib:

==================
The Blob library
==================
The blob library implements binary data manipulations routines. The library is
based on `blob objects` that represent a buffer of arbitrary
binary data.

---------------
Squirrel API
---------------

+++++++++++++++
Global symbols
+++++++++++++++

.. js:function:: castf2i(f)

    casts a float to a int

.. js:function:: casti2f(n)

    casts a int to a float

.. js:function:: swap2(n)

    swap the byte order of a number (like it would be a 16bits integer)

.. js:function:: swap4(n)

    swap the byte order of an integer

.. js:function:: swapfloat(n)

    swaps the byteorder of a float

++++++++++++++++++
The blob class
++++++++++++++++++

The blob object is a buffer of arbitrary binary data. The object behaves like
a file stream, it has a read/write pointer and it automatically grows if data
is written out of his boundary.
A blob can also be accessed byte by byte through the `[]` operator.

.. js:class:: blob(size)

    :param int size: initial size of the blob

    returns a new instance of a blob class of the specified size in bytes

.. js:function:: blob.eos()

    returns a non null value if the read/write pointer is at the end of the stream.

.. js:function:: blob.flush()

    flushes the stream.return a value != null if succeded, otherwise returns null

.. js:function:: blob.len()

    returns the length of the stream

.. js:function:: blob.readblob(size)

    :param int size: number of bytes to read

    read n bytes from the stream and returns them as blob

.. js:function:: blob.readn(type)

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

.. js:function:: blob.resize(size)

    :param int size: the new size of the blob in bytes

    resizes the blob to the specified `size`

.. js:function:: blob.seek(offset [,origin])

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

.. js:function:: blob.swap2()

    swaps the byte order of the blob content as it would be an array of `16bits integers`

.. js:function:: blob.swap4()

    swaps the byte order of the blob content as it would be an array of `32bits integers`

.. js:function:: blob.tell()

    returns the read/write pointer absolute position

.. js:function:: blob.writeblob(src)

    :param blob src: the source blob containing the data to be written

    writes a blob in the stream

.. js:function:: blob.writen(n, type)

    :param number n: the value to be written
    :param int type: type of the number to write

    writes a number in the stream formatted according to the `type` parameter

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


------
C API
------

.. _sqstd_register_bloblib:

.. c:function:: SQRESULT sqstd_register_bloblib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initializes and registers the blob library in the given VM.

.. _sqstd_getblob:

.. c:function:: SQRESULT sqstd_getblob(HSQUIRRELVM v, SQInteger idx, SQUserPointer* ptr)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: and index in the stack
    :param SQUserPointer* ptr: A pointer to the userpointer that will point to the blob's payload
    :returns: an SQRESULT

    retrieve the pointer of a blob's payload from an arbitrary
    position in the stack.

.. _sqstd_getblobsize:

.. c:function:: SQInteger sqstd_getblobsize(HSQUIRRELVM v, SQInteger idx)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: and index in the stack
    :returns: the size of the blob at `idx` position

    retrieves the size of a blob's payload from an arbitrary
    position in the stack.

.. _sqstd_createblob:

.. c:function:: SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger size:  the size of the blob payload that has to be created
    :returns: a pointer to the newly created blob payload

    creates a blob with the given payload size and pushes it in the stack.
