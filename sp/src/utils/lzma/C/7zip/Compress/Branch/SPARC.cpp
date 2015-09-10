// SPARC.cpp

#include "StdAfx.h"
#include "SPARC.h"

#include "Windows/Defs.h"
#include "BranchSPARC.c"

UInt32 CBC_SPARC_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::SPARC_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_SPARC_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::SPARC_Convert(data, size, _bufferPos, 0);
}
