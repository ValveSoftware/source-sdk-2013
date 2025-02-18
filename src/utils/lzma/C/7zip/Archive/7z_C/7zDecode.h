/* 7zDecode.h */

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "7zItem.h"
#include "7zAlloc.h"
#ifdef _LZMA_IN_CB
#include "7zIn.h"
#endif

SZ_RESULT SzDecode(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *stream,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, 
    size_t *outSizeProcessed, ISzAlloc *allocMain);

#endif
