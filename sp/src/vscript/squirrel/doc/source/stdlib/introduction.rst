.. _stdlib_introduction:

============
Introduction
============

The squirrel standard libraries consist in a set of modules implemented in C++.
While are not essential for the language, they provide a set of useful services that are
commonly used by a wide range of applications(file I/O, regular expressions, etc...),
plus they offer a foundation for developing additional libraries.

All libraries are implemented through the squirrel API and the ANSI C runtime library.
The modules are organized in the following way:

* I/O : input and output
* blob : binary buffers manipilation
* math : basic mathematical routines
* system : system access function
* string : string formatting and manipulation
* aux : auxiliary functions

The libraries can be registered independently,except for the IO library that depends from the bloblib.
