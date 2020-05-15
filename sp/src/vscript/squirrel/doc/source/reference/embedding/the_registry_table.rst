.. _embedding_the_registry_table:

==================
The registry table
==================

The registry table is an hidden table shared between vm and all his thread(friend vms).
This table is accessible only through the C API and is meant to be an utility structure
for native C library implementation.
For instance the sqstdlib(squirrel standard library)uses it to store configuration and shared objects
delegates.
The registry is accessible through the API call *sq_pushregistrytable()*.::

    void sq_pushregistrytable(HSQUIRRELVM v);
