/* BranchARMThumb.c */

#include "BranchARMThumb.h"

UInt32 ARMThumb_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 2)
  {
    if ((data[i + 1] & 0xF8) == 0xF0 && 
        (data[i + 3] & 0xF8) == 0xF8)
    {
      UInt32 src = 
        ((data[i + 1] & 0x7) << 19) |
        (data[i + 0] << 11) |
        ((data[i + 3] & 0x7) << 8) |
        (data[i + 2]);
      
      src <<= 1;
      UInt32 dest;
      if (encoding)
        dest = nowPos + i + 4 + src;
      else
        dest = src - (nowPos + i + 4);
      dest >>= 1;
      
      data[i + 1] = 0xF0 | ((dest >> 19) & 0x7);
      data[i + 0] = (dest >> 11);
      data[i + 3] = 0xF8 | ((dest >> 8) & 0x7);
      data[i + 2] = (dest);
      i += 2;
    }
  }
  return i;
}
