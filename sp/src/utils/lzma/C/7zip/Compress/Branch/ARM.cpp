// ARM.cpp

#include "StdAfx.h"
#include "ARM.h"

#include "BranchARM.c"

UInt32 CBC_ARM_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARM_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_ARM_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARM_Convert(data, size, _bufferPos, 0);
}
