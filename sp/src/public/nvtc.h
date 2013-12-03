//========= Copyright Valve Corporation, All rights reserved. ============//
/*
 *   Copyright (c) 1997-8  S3 Inc.  All Rights Reserved.
 *
 *   Module Name:  s3_intrf.h
 *
 *   Purpose:  Constant, structure, and prototype definitions for S3TC
 *			   interface to DX surface
 *
 *   Author:  Dan Hung, Martin Hoffesommer
 *
 *   Revision History:
 *	version Beta 1.00.00-98-03-26
 */

// Highlevel interface

#ifndef NVTC_H
#define NVTC_H

#if defined( _WIN32 ) && !defined( _X360 )
#include <ddraw.h>
#endif

// RGB encoding types
#define S3TC_ENCODE_RGB_FULL    		0x0
#define S3TC_ENCODE_RGB_COLOR_KEY		0x1
#define S3TC_ENCODE_RGB_ALPHA_COMPARE	0x2
#define _S3TC_ENCODE_RGB_MASK			0xff

// alpha encoding types
#define S3TC_ENCODE_ALPHA_NONE			0x000
#define S3TC_ENCODE_ALPHA_EXPLICIT		0x100
#define S3TC_ENCODE_ALPHA_INTERPOLATED	0x200
#define _S3TC_ENCODE_ALPHA_MASK			0xff00


#if defined( _WIN32 ) && !defined( _X360 )
// common encoding types
//@@@TBD

// error codes
#define ERROR_ABORTED -1

// Progress Callback for S3TCencode
typedef BOOL (* LP_S3TC_PROGRESS_CALLBACK)(float fProgress, LPVOID lpUser1, LPVOID lpUser2);

// set alpha reference value for alpha compare encoding
void S3TCsetAlphaReference(int nRef);

// determine number of bytes needed to compress given source image
unsigned int S3TCgetEncodeSize(DDSURFACEDESC *lpDesc,	// [in]
						   unsigned int dwEncodeType 	// [in]
						   );

// encode (compress) given source image to given destination surface
void S3TCencode(DDSURFACEDESC *lpSrc,		// [in]
				PALETTEENTRY *lpPal,		// [in], may be NULL
				DDSURFACEDESC *lpDest,		// [out]
				void *lpDestBuf,			// [in]
				unsigned int dwEncodeType,  // [in]
				float *weight				// [in]
				);

int S3TCencodeEx(DDSURFACEDESC *lpSrc,		// [in]
				PALETTEENTRY *lpPal,		// [in], may be NULL
				DDSURFACEDESC *lpDest,		// [out]
				void *lpDestBuf,			// [in]
				unsigned int dwEncodeType,  // [in]
				float *weight,				// [in]
				LP_S3TC_PROGRESS_CALLBACK lpS3TCProgressProc, // [in], may be NULL
				LPVOID lpArg1,				// in 
				LPVOID lpArg2				// in 
				);

// determine number of bytes needed do decompress given compressed image
unsigned int S3TCgetDecodeSize(DDSURFACEDESC *lpDesc);

// decode (decompress) to ARGB8888
void S3TCdecode(DDSURFACEDESC *lpSrc,		// [in]
				DDSURFACEDESC *lpDest,		// [out]
				void *lpDestBuf				// [in]
				);

#endif // _WIN32

#endif // NVTC_H
