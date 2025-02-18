// ARMThumb.cpp

#include "StdAfx.h"
#include "ARMThumb.h"

#include "BranchARMThumb.c"

UInt32 CBC_ARMThumb_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARMThumb_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_ARMThumb_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARMThumb_Convert(data, size, _bufferPos, 0);
}
