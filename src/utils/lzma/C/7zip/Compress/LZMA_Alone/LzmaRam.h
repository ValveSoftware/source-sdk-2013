// LzmaRam.h

#ifndef __LzmaRam_h
#define __LzmaRam_h

#include <stdlib.h>
#include "../../../Common/Types.h"

/*
LzmaRamEncode: BCJ + LZMA RAM->RAM compressing.
It uses .lzma format, but it writes one additional byte to .lzma file:
  0: - no filter
  1: - x86(BCJ) filter.

To provide best compression ratio dictionarySize mustbe >= inSize

LzmaRamEncode allocates Data with MyAlloc/BigAlloc functions.
RAM Requirements:
  RamSize = dictionarySize * 9.5 + 6MB + FilterBlockSize 
    FilterBlockSize = 0, if useFilter == false
    FilterBlockSize = inSize, if useFilter == true

  Return code:
    0 - OK
    1 - Unspecified Error
    2 - Memory allocating error
    3 - Output buffer OVERFLOW

If you use SZ_FILTER_AUTO mode, then encoder will use 2 or 3 passes:
  2 passes when FILTER_NO provides better compression.
  3 passes when FILTER_YES provides better compression.
*/

enum ESzFilterMode 
{
  SZ_FILTER_NO,
  SZ_FILTER_YES,
  SZ_FILTER_AUTO
};

int LzmaRamEncode(
    const Byte *inBuffer, size_t inSize, 
    Byte *outBuffer, size_t outSize, size_t *outSizeProcessed, 
    UInt32 dictionarySize, ESzFilterMode filterMode);

#endif
