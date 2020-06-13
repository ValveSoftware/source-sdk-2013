.. _embedding_error_convetions:


========================
Error Conventions
========================

.. index::
    single: Error Conventions

Most of the functions in the API return a SQRESULT value; SQRESULT indicates if a
function completed successfully or not.
The macros SQ_SUCCEEDED() and SQ_FAILED() are used to test the result of a function.::

    if(SQ_FAILED(sq_getstring(v,-1,&s)))
        printf("getstring failed");
