// PPC.cpp

#include "StdAfx.h"
#include "PPC.h"

#include "Windows/Defs.h"
#include "BranchPPC.c"

UInt32 CBC_PPC_B_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::PPC_B_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_PPC_B_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::PPC_B_Convert(data, size, _bufferPos, 0);
}
