/* 7zCrc.h */

#ifndef __7Z_CRC_H
#define __7Z_CRC_H

#include <stddef.h>

#include "7zTypes.h"

extern UInt32 g_CrcTable[256];
void InitCrcTable();

void CrcInit(UInt32 *crc);
UInt32 CrcGetDigest(UInt32 *crc);
void CrcUpdateByte(UInt32 *crc, Byte v);
void CrcUpdateUInt16(UInt32 *crc, UInt16 v);
void CrcUpdateUInt32(UInt32 *crc, UInt32 v);
void CrcUpdateUInt64(UInt32 *crc, UInt64 v);
void CrcUpdate(UInt32 *crc, const void *data, size_t size);
 
UInt32 CrcCalculateDigest(const void *data, size_t size);
int CrcVerifyDigest(UInt32 digest, const void *data, size_t size);

#endif
