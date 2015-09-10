//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	Purpose: LZMA Glue. Designed for Tool time Encoding/Decoding.
//
//  LZMA Codec interface for engine. Based largely on LzmaUtil.c in SDK
//
//  LZMA SDK 9.38 beta
//  2015-01-03 : Igor Pavlov : Public domain
//  http://www.7-zip.org/
//
//====================================================================================//

#ifndef LZMA_H
#define LZMA_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
//	These routines are designed for TOOL TIME encoding/decoding on the PC!
//	They have not been made to encode/decode on the PPC and lack big endian awarnesss.
//	Lightweight GAME TIME Decoding is part of tier1.lib, via CLZMA.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Encoding glue. Returns non-null Compressed buffer if successful.
// Caller must free.
//-----------------------------------------------------------------------------
unsigned char *LZMA_Compress(
unsigned char	*pInput,
unsigned int	inputSize,
unsigned int	*pOutputSize );

//-----------------------------------------------------------------------------
// Decoding glue. Returns TRUE if succesful.
//-----------------------------------------------------------------------------
bool LZMA_Uncompress(
unsigned char	*pInput,
unsigned char	**ppOutput,
unsigned int	*pOutputSize );

//-----------------------------------------------------------------------------
// Decoding helper, returns TRUE if buffer is LZMA compressed.
//-----------------------------------------------------------------------------
bool LZMA_IsCompressed( unsigned char *pInput );

//-----------------------------------------------------------------------------
// Decoding helper, returns non-zero size of data when uncompressed, otherwise 0.
//-----------------------------------------------------------------------------
unsigned int LZMA_GetActualSize( unsigned char *pInput );

#endif
