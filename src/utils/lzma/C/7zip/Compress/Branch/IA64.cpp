// IA64.cpp

#include "StdAfx.h"
#include "IA64.h"

#include "BranchIA64.c"

UInt32 CBC_IA64_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::IA64_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_IA64_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::IA64_Convert(data, size, _bufferPos, 0);
}
