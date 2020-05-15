/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS
void *sq_vm_malloc(SQUnsignedInteger size){ return malloc(size); }

void *sq_vm_realloc(void *p, SQUnsignedInteger SQ_UNUSED_ARG(oldsize), SQUnsignedInteger size){ return realloc(p, size); }

void sq_vm_free(void *p, SQUnsignedInteger SQ_UNUSED_ARG(size)){ free(p); }
#endif
