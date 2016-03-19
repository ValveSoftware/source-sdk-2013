/* BranchARM.c */

#include "BranchARM.h"

UInt32 ARM_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 4)
  {
    if (data[i + 3] == 0xEB)
    {
      UInt32 src = (data[i + 2] << 16) | (data[i + 1] << 8) | (data[i + 0]);
      src <<= 2;
      UInt32 dest;
      if (encoding)
        dest = nowPos + i + 8 + src;
      else
        dest = src - (nowPos + i + 8);
      dest >>= 2;
      data[i + 2] = (dest >> 16);
      data[i + 1] = (dest >> 8);
      data[i + 0] = dest;
    }
  }
  return i;
}
