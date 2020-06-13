.. _embedding_squirrel:

***************************
  Embedding Squirrel
***************************

*This section describes how to embed Squirrel in a host application,
C language knowledge is required to understand this part of the manual.*

Because of his nature of extension language, Squirrel's compiler and virtual machine
are implemented as C library. The library exposes a set of functions to compile scripts,
call functions, manipulate data and extend the virtual machine.
All declarations needed for embedding the language in an application are in the header file 'squirrel.h'.

.. toctree::
    embedding/memory_management.rst
    embedding/build_configuration.rst
    embedding/error_conventions.rst
    embedding/vm_initialization.rst
    embedding/the_stack.rst
    embedding/runtime_error_handling.rst
    embedding/compiling_a_script.rst
    embedding/calling_a_function.rst
    embedding/creating_a_c_function.rst
    embedding/tables_and_arrays_manipulation.rst
    embedding/userdata_and_userpointers.rst
    embedding/the_registry_table.rst
    embedding/references_from_c.rst
    embedding/debug_interface.rst
