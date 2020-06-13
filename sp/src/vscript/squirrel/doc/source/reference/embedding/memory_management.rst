.. _embedding_memory_management:

========================
Memory Management
========================

.. index:: single: Memory Management

Squirrel uses reference counting (RC) as primary system for memory management;
however, the virtual machine (VM) has an auxiliary
mark and sweep garbage collector that can be invoked on demand.

There are 2 possible compile time options:

    * The default configuration consists in RC plus a mark and sweep garbage collector.
      The host program can call the function sq_collectgarbage() and perform a garbage collection cycle
      during the program execution. The garbage collector isn't invoked by the VM and has to
      be explicitly called by the host program.

    * The second a situation consists in RC only(define NO_GARBAGE_COLLECTOR); in this case is impossible for
      the VM to detect reference cycles, so is the programmer that has to solve them explicitly in order to
      avoid memory leaks.

The only advantage introduced by the second option is that saves 2 additional
pointers that have to be stored for each object in the default configuration with
garbage collector(8 bytes for 32 bits systems).
The types involved are: tables, arrays, functions, threads, userdata and generators; all other
types are untouched. These options do not affect execution speed.
