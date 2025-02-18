//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  LZMA Codec interface for engine. Based largely on LzmaUtil.c in SDK
//
//  LZMA SDK 9.38 beta
//  2015-01-03 : Igor Pavlov : Public domain
//  http://www.7-zip.org/
//
//========================================================================//

#ifdef POSIX
#include <stdlib.h>
#endif
#include "tier0/memdbgon.h"
#include "../../public/tier1/lzmaDecoder.h"
#include "C/7zTypes.h"
#include "C/LzmaEnc.h"
#include "C/LzmaDec.h"
#include "tier0/dbg.h"

// Allocator to pass to LZMA functions
static void *SzAlloc(void *p, size_t size) { return malloc(size); }
static void SzFree(void *p, void *address) { free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

// lzma buffers will have a 13 byte trivial header
// [0]		reserved
// [1..4]	dictionary size, little endian
// [5..8]	uncompressed size, little endian low word
// [9..12]	uncompressed size, little endian high word
// [13..]	lzma compressed data
#define LZMA_ORIGINAL_HEADER_SIZE	13

SRes CInStreamRam_StaticRead(void *p, void *buf, size_t *size );
size_t COutStreamRam_StaticWrite(void *p, const void *buf, size_t size);

class CInStreamRam : public ISeqInStream
{
	const Byte *Data;
	size_t Size;
	size_t Pos;

public:
	void Init(const Byte *data, size_t size)
	{
		Data = data;
		Size = size;
		Pos = 0;
		Read = CInStreamRam_StaticRead;
	}

	SRes DoRead( void *buf, size_t *size )
	{
		size_t inSize = *size;
		size_t remain = Size - Pos;
		if (inSize > remain)
			inSize = remain;

		for (UInt32 i = 0; i < inSize; i++)
			((Byte *)buf)[i] = Data[Pos + i];

		Pos += inSize;

		*size = inSize;

		return SZ_OK;
	}
};

class COutStreamRam: public ISeqOutStream
{
	size_t Size;

public:
	Byte *Data;
	size_t Pos;
	bool Overflow;

	void Init(Byte *data, size_t size)
	{
		Data = data;
		Size = size;
		Pos = 0;
		Overflow = false;
		Write = COutStreamRam_StaticWrite;
	}

	size_t DoWrite( const void *buf, size_t size )
	{
		UInt32 i;
		for (i = 0; i < size && Pos < Size; i++)
			Data[Pos++] = ((const Byte *)buf)[i];
		if (i != size)
		{
			Overflow = true;
		}
		return i;
	}
};

SRes CInStreamRam_StaticRead(void *p, void *buf, size_t *size )
{
	return reinterpret_cast<CInStreamRam *>(p)->DoRead( buf, size );
}

size_t COutStreamRam_StaticWrite(void *p, const void *buf, size_t size)
{
	return reinterpret_cast<COutStreamRam *>(p)->DoWrite( buf, size );
}

SRes
LzmaEncode( const Byte *inBuffer,
            size_t     inSize,
            Byte       *outBuffer,
            size_t     outSize,
            size_t     *outSizeProcessed )
{
	// Based on Encode helper in SDK/LzmaUtil
	*outSizeProcessed = 0;

	const size_t kMinDestSize = LZMA_ORIGINAL_HEADER_SIZE;
	if ( outSize < kMinDestSize )
	{
		return SZ_ERROR_FAIL;
	}

	CLzmaEncHandle enc;
	SRes res;
	CLzmaEncProps props;

	enc = LzmaEnc_Create( &g_Alloc );
	if ( !enc )
	{
		return SZ_ERROR_FAIL;
	}

	LzmaEncProps_Init( &props );
	res = LzmaEnc_SetProps( enc, &props );

	if ( res != SZ_OK )
	{
		return res;
	}

	COutStreamRam outStream;

	outStream.Init( outBuffer, outSize );

	Byte header[LZMA_PROPS_SIZE + 8];
	size_t headerSize = LZMA_PROPS_SIZE;
	int i;

	res = LzmaEnc_WriteProperties( enc, header, &headerSize );
	if ( res != SZ_OK )
	{
		return res;
	}

	// Uncompressed size after properties in header
    for (i = 0; i < 8; i++)
	{
		header[headerSize++] = (Byte)(inSize >> (8 * i));
	}

    if ( outStream.DoWrite( header, headerSize ) != headerSize )
	{
		res = SZ_ERROR_WRITE;
	}
	else if ( res == SZ_OK )
	{
		CInStreamRam inStream;
		inStream.Init( inBuffer, inSize );
		res = LzmaEnc_Encode( enc, &outStream, &inStream, NULL, &g_Alloc, &g_Alloc );

		if ( outStream.Overflow )
		{
			res = SZ_ERROR_FAIL;
		}
		else
		{
			*outSizeProcessed = outStream.Pos;
		}
	}

	LzmaEnc_Destroy( enc, &g_Alloc, &g_Alloc );

	return res;
}

//-----------------------------------------------------------------------------
// Encoding glue. Returns non-null Compressed buffer if successful.
// Caller must free.
//-----------------------------------------------------------------------------
unsigned char *LZMA_Compress( unsigned char *pInput,
                              unsigned int  inputSize,
                              unsigned int  *pOutputSize )
{
	*pOutputSize = 0;

	// using same work buffer calcs as the SDK 105% + 64K
	unsigned outSize = inputSize/20 * 21 + (1<<16);
	unsigned char *pOutputBuffer = (unsigned char*)malloc( outSize );
	if ( !pOutputBuffer )
	{
		return NULL;
	}

	// compress, skipping past our header
	size_t compressedSize;
	int result = LzmaEncode( pInput, inputSize, pOutputBuffer + sizeof( lzma_header_t ), outSize - sizeof( lzma_header_t ), &compressedSize );
	if ( result != SZ_OK )
	{
		Warning( "LZMA encode failed (%i)\n", result );
		Assert( result == SZ_OK );
		free( pOutputBuffer );
		return NULL;
	}

	// construct our header, strip theirs
	lzma_header_t *pHeader = (lzma_header_t *)pOutputBuffer;
	pHeader->id = LZMA_ID;
	pHeader->actualSize = inputSize;
	pHeader->lzmaSize = (unsigned int)( compressedSize - LZMA_ORIGINAL_HEADER_SIZE );
	memcpy( pHeader->properties, pOutputBuffer + sizeof( lzma_header_t ), LZMA_PROPS_SIZE );

	// shift the compressed data into place
	memmove( pOutputBuffer + sizeof( lzma_header_t ),
	         pOutputBuffer + sizeof( lzma_header_t ) + LZMA_ORIGINAL_HEADER_SIZE,
	         compressedSize - LZMA_ORIGINAL_HEADER_SIZE );

	// final output size is our header plus compressed bits
	*pOutputSize = (unsigned int)( sizeof( lzma_header_t ) + compressedSize - LZMA_ORIGINAL_HEADER_SIZE );

	return pOutputBuffer;
}

//-----------------------------------------------------------------------------
// Above, but returns null if compression would not yield a size improvement
//-----------------------------------------------------------------------------
unsigned char *LZMA_OpportunisticCompress( unsigned char *pInput,
                              unsigned int  inputSize,
                              unsigned int  *pOutputSize )
{
	unsigned char *pRet = LZMA_Compress( pInput, inputSize, pOutputSize );
	if ( *pOutputSize <= inputSize )
	{
		// compression got worse or stayed the same
		free( pRet );
		return NULL;
	}

	return pRet;
}

//-----------------------------------------------------------------------------
//	Decoding glue. Returns TRUE if succesful.
//-----------------------------------------------------------------------------
bool LZMA_Uncompress( unsigned char *pInBuffer,
                      unsigned char **ppOutBuffer,
                      unsigned int  *pOutSize )
{
	*ppOutBuffer = NULL;
	*pOutSize = 0;

	lzma_header_t *pHeader = (lzma_header_t *)pInBuffer;
	if ( pHeader->id != LZMA_ID )
	{
		// not ours
		return false;
	}

	CLzmaDec state;

	LzmaDec_Construct(&state);

	if ( LzmaDec_Allocate(&state, pHeader->properties, LZMA_PROPS_SIZE, &g_Alloc) != SZ_OK )
	{
		return false;
	}

	unsigned char *pOutBuffer = (unsigned char *)malloc( pHeader->actualSize );
	if ( !pOutBuffer )
	{
		LzmaDec_Free(&state, &g_Alloc);
		return false;
	}

	// These are in/out variables
	SizeT outProcessed = pHeader->actualSize;
	SizeT inProcessed = pHeader->lzmaSize;
	ELzmaStatus status;
	SRes result = LzmaDecode( (Byte *)pOutBuffer, &outProcessed, (Byte *)(pInBuffer + sizeof( lzma_header_t ) ),
	                          &inProcessed, (Byte *)pHeader->properties, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &g_Alloc );


	LzmaDec_Free(&state, &g_Alloc);

	if ( result != SZ_OK || pHeader->actualSize != outProcessed )
	{
		free( pOutBuffer );
		return false;
	}

	*ppOutBuffer = pOutBuffer;
	*pOutSize = pHeader->actualSize;

	return true;
}

bool LZMA_IsCompressed( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return true;
	}

	// unrecognized
	return false;
}

unsigned int LZMA_GetActualSize( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return pHeader->actualSize;
	}

	// unrecognized
	return 0;
}
