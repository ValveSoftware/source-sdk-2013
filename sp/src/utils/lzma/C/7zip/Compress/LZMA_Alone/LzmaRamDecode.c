/* LzmaRamDecode.c */

#include "LzmaRamDecode.h"
#ifdef _SZ_ONE_DIRECTORY
#include "LzmaDecode.h"
#include "BranchX86.h"
#else
#include "../LZMA_C/LzmaDecode.h"
#include "../Branch/BranchX86.h"
#endif

#define LZMA_PROPS_SIZE 14
#define LZMA_SIZE_OFFSET 6

int LzmaRamGetUncompressedSize(
    const unsigned char *inBuffer, 
    size_t inSize, 
    size_t *outSize)
{
  unsigned int i;
  if (inSize < LZMA_PROPS_SIZE)
    return 1;
  *outSize = 0;
  for(i = 0; i < sizeof(size_t); i++)
    *outSize += ((size_t)inBuffer[LZMA_SIZE_OFFSET + i]) << (8 * i);
  for(; i < 8; i++)
    if (inBuffer[LZMA_SIZE_OFFSET + i] != 0)
      return 1;
  return 0;
}

#define SZE_DATA_ERROR (1)
#define SZE_OUTOFMEMORY (2)

int LzmaRamDecompress(
    const unsigned char *inBuffer, 
    size_t inSize,
    unsigned char *outBuffer,
    size_t outSize,
    size_t *outSizeProcessed,
    void * (*allocFunc)(size_t size), 
    void (*freeFunc)(void *))
{
  CLzmaDecoderState state;  /* it's about 24 bytes structure, if int is 32-bit */
  int result;
  SizeT outSizeProcessedLoc;
  SizeT inProcessed;
  int useFilter;
  
  if (inSize < LZMA_PROPS_SIZE)
    return 1;
  useFilter = inBuffer[0];

  *outSizeProcessed = 0;
  if (useFilter > 1)
    return 1;

  if (LzmaDecodeProperties(&state.Properties, inBuffer + 1, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK)
    return 1;
  state.Probs = (CProb *)allocFunc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
  if (state.Probs == 0)
    return SZE_OUTOFMEMORY;
  
  result = LzmaDecode(&state,
    inBuffer + LZMA_PROPS_SIZE, (SizeT)inSize - LZMA_PROPS_SIZE, &inProcessed,
    outBuffer, (SizeT)outSize, &outSizeProcessedLoc);
  freeFunc(state.Probs);
  if (result != LZMA_RESULT_OK)
    return 1;
  *outSizeProcessed = (size_t)outSizeProcessedLoc;
  if (useFilter == 1)
  {
    UInt32 _prevMask;
    UInt32 _prevPos;
    x86_Convert_Init(_prevMask, _prevPos);
    x86_Convert(outBuffer, (UInt32)outSizeProcessedLoc, 0, &_prevMask, &_prevPos, 0);
  }
  return 0;
}