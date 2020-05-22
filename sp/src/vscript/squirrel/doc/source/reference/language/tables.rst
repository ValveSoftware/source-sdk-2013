.. _tables:


=================
Tables
=================

.. index::
    single: Tables

Tables are associative containers implemented as pairs of key/value (called slot); values
can be any possible type and keys any type except 'null'.
Tables are squirrel's skeleton, delegation and many other features are all implemented
through this type; even the environment, where "global" variables are stored, is a table
(known as root table).

------------------
Construction
------------------

Tables are created through the table constructor (see :ref:`Table constructor <table_constructor>`)

------------------
Slot creation
------------------

.. index::
    single: Slot Creation(table)

Adding a new slot in a existing table is done through the "new slot" operator ``<-``; this
operator behaves like a normal assignment except that if the slot does not exists it will
be created.::

    local a = {}

The following line will cause an exception because the slot named 'newslot' does not exist
in the table 'a'::

    a.newslot = 1234

this will succeed: ::

    a.newslot <- 1234;

or::

    a[1] <- "I'm the value of the new slot";

-----------------
Slot deletion
-----------------

.. index::
    single: Slot Deletion(table)


::

    exp:= delete derefexp

Deletion of a slot is done through the keyword delete; the result of this expression will be
the value of the deleted slot.::

    a <- {
        test1=1234
        deleteme="now"
    }

    delete a.test1
    print(delete a.deleteme); //this will print the string "now"

