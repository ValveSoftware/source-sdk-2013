/* LzmaRamDecode.h */

#ifndef __LzmaRamDecode_h
#define __LzmaRamDecode_h

#include <stdlib.h>

/*
LzmaRamGetUncompressedSize:
  In: 
    inBuffer - input data
    inSize   - input data size
  Out: 
    outSize  - uncompressed size
  Return code:
    0 - OK
    1 - Error in headers
*/

int LzmaRamGetUncompressedSize(
    const unsigned char *inBuffer, 
    size_t inSize,
    size_t *outSize);


/*
LzmaRamDecompress:
  In: 
    inBuffer  - input data
    inSize    - input data size
    outBuffer - output data
    outSize   - output size
    allocFunc - alloc function (can be malloc)
    freeFunc  - free function (can be free)
  Out: 
    outSizeProcessed - processed size
  Return code:
    0 - OK
    1 - Error in headers / data stream
    2 - Memory allocating error

Memory requirements depend from properties of LZMA stream.
With default lzma settings it's about 16 KB.
*/

int LzmaRamDecompress(
    const unsigned char *inBuffer, 
    size_t inSize,
    unsigned char *outBuffer,
    size_t outSize,
    size_t *outSizeProcessed,
    void * (*allocFunc)(size_t size), 
    void (*freeFunc)(void *));

#endif
