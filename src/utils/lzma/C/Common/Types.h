// Common/Types.h

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#ifdef _MSC_VER
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4706)	// warning C4706: assignment within conditional expression
#pragma warning(disable : 4100)	// warning C4100: unreferenced formal parameter
//#pragma warning(disable : 4312)	// warning C4312: conversion from 'unsigned int' to 'LPSTR' of greater size
#pragma warning(disable : 4244)	// warning C4244: 'initializing' : conversion from '__w64 int' to 'UInt32', possible loss of data
#pragma warning(disable : 4267)	// warning C4267: 'conversion from 'size_t' to 'UInt32', possible loss of data
#endif

#ifndef _7ZIP_BYTE_DEFINED
#define _7ZIP_BYTE_DEFINED
typedef unsigned char Byte;
#endif 

#ifndef _7ZIP_INT16_DEFINED
#define _7ZIP_INT16_DEFINED
typedef short Int16;
#endif 

#ifndef _7ZIP_UINT16_DEFINED
#define _7ZIP_UINT16_DEFINED
typedef unsigned short UInt16;
#endif 

#ifndef _7ZIP_INT32_DEFINED
#define _7ZIP_INT32_DEFINED
typedef int Int32;
#endif 

#ifndef _7ZIP_UINT32_DEFINED
#define _7ZIP_UINT32_DEFINED
typedef unsigned int UInt32;
#endif 

#ifdef _MSC_VER

#ifndef _7ZIP_INT64_DEFINED
#define _7ZIP_INT64_DEFINED
typedef __int64 Int64;
#endif 

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
typedef unsigned __int64 UInt64;
#endif 

#else

#ifndef _7ZIP_INT64_DEFINED
#define _7ZIP_INT64_DEFINED
typedef long long int Int64;
#endif 

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
typedef unsigned long long int UInt64;
#endif 

#endif

#endif
