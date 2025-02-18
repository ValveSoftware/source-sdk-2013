/* 7zCrc.c */

#include "7zCrc.h"

#define kCrcPoly 0xEDB88320

UInt32 g_CrcTable[256];

void InitCrcTable()
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
    UInt32 r = i;
    int j;
    for (j = 0; j < 8; j++)
      if (r & 1) 
        r = (r >> 1) ^ kCrcPoly;
      else     
        r >>= 1;
    g_CrcTable[i] = r;
  }
}

void CrcInit(UInt32 *crc) { *crc = 0xFFFFFFFF; }
UInt32 CrcGetDigest(UInt32 *crc) { return *crc ^ 0xFFFFFFFF; } 

void CrcUpdateByte(UInt32 *crc, Byte b)
{
  *crc = g_CrcTable[((Byte)(*crc)) ^ b] ^ (*crc >> 8);
}

void CrcUpdateUInt16(UInt32 *crc, UInt16 v)
{
  CrcUpdateByte(crc, (Byte)v);
  CrcUpdateByte(crc, (Byte)(v >> 8));
}

void CrcUpdateUInt32(UInt32 *crc, UInt32 v)
{
  int i;
  for (i = 0; i < 4; i++)
    CrcUpdateByte(crc, (Byte)(v >> (8 * i)));
}

void CrcUpdateUInt64(UInt32 *crc, UInt64 v)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    CrcUpdateByte(crc, (Byte)(v));
    v >>= 8;
  }
}

void CrcUpdate(UInt32 *crc, const void *data, size_t size)
{
  UInt32 v = *crc;
  const Byte *p = (const Byte *)data;
  for (; size > 0 ; size--, p++)
    v = g_CrcTable[((Byte)(v)) ^ *p] ^ (v >> 8);
  *crc = v;
}

UInt32 CrcCalculateDigest(const void *data, size_t size)
{
  UInt32 crc;
  CrcInit(&crc);
  CrcUpdate(&crc, data, size);
  return CrcGetDigest(&crc);
}

int CrcVerifyDigest(UInt32 digest, const void *data, size_t size)
{
  return (CrcCalculateDigest(data, size) == digest);
}
