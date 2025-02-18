/* 7zTypes.h */

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#ifndef _7ZIP_BYTE_DEFINED
#define _7ZIP_BYTE_DEFINED
typedef unsigned char Byte;
#endif 

#ifndef _7ZIP_UINT16_DEFINED
#define _7ZIP_UINT16_DEFINED
typedef unsigned short UInt16;
#endif 

#ifndef _7ZIP_UINT32_DEFINED
#define _7ZIP_UINT32_DEFINED
#ifdef _LZMA_UINT32_IS_ULONG
typedef unsigned long UInt32;
#else
typedef unsigned int UInt32;
#endif
#endif 

/* #define _SZ_NO_INT_64 */
/* define it your compiler doesn't support long long int */

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
#ifdef _SZ_NO_INT_64
typedef unsigned long UInt64;
#else
#ifdef _MSC_VER
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long int UInt64;
#endif
#endif
#endif


/* #define _SZ_FILE_SIZE_64 */
/* Use _SZ_FILE_SIZE_64 if you need support for files larger than 4 GB*/

#ifndef CFileSize
#ifdef _SZ_FILE_SIZE_64
typedef UInt64 CFileSize; 
#else
typedef UInt32 CFileSize; 
#endif
#endif

#define SZ_RESULT int

#define SZ_OK (0)
#define SZE_DATA_ERROR (1)
#define SZE_OUTOFMEMORY (2)
#define SZE_CRC_ERROR (3)

#define SZE_NOTIMPL (4)
#define SZE_FAIL (5)

#define SZE_ARCHIVE_ERROR (6)

#define RINOK(x) { int __result_ = (x); if(__result_ != 0) return __result_; }

#endif
