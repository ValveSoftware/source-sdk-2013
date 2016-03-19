//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  LZMA Codec interface for engine.
//
//  LZMA SDK 9.38 beta
//  2015-01-03 : Igor Pavlov : Public domain
//  http://www.7-zip.org/
//
//========================================================================//

#define _LZMADECODER_CPP

#include "tier0/platform.h"
#include "tier0/basetypes.h"
#include "tier0/dbg.h"

#include "../utils/lzma/C/7zTypes.h"
#include "../utils/lzma/C/LzmaEnc.h"
#include "../utils/lzma/C/LzmaDec.h"

// Ugly define to let us forward declare the anonymous-struct-typedef that is CLzmaDec in the header.
#define CLzmaDec_t CLzmaDec
#include "tier1/lzmaDecoder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Allocator to pass to LZMA functions
static void *SzAlloc(void *p, size_t size) { return malloc(size); }
static void SzFree(void *p, void *address) { free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

//-----------------------------------------------------------------------------
// Returns true if buffer is compressed.
//-----------------------------------------------------------------------------
/* static */
bool CLZMA::IsCompressed( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return true;
	}

	// unrecognized
	return false;
}

//-----------------------------------------------------------------------------
// Returns uncompressed size of compressed input buffer. Used for allocating output
// buffer for decompression. Returns 0 if input buffer is not compressed.
//-----------------------------------------------------------------------------
/* static */
unsigned int CLZMA::GetActualSize( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return LittleLong( pHeader->actualSize );
	}

	// unrecognized
	return 0;
}

//-----------------------------------------------------------------------------
// Uncompress a buffer, Returns the uncompressed size. Caller must provide an
// adequate sized output buffer or memory corruption will occur.
//-----------------------------------------------------------------------------
/* static */
unsigned int CLZMA::Uncompress( unsigned char *pInput, unsigned char *pOutput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader->id != LZMA_ID )
	{
		// not ours
		return false;
	}

	CLzmaDec state;

	LzmaDec_Construct(&state);

	if ( LzmaDec_Allocate(&state, pHeader->properties, LZMA_PROPS_SIZE, &g_Alloc) != SZ_OK )
	{
		Assert( false );
		return 0;
	}

	// These are in/out variables
	SizeT outProcessed = pHeader->actualSize;
	SizeT inProcessed = pHeader->lzmaSize;
	ELzmaStatus status;
	SRes result = LzmaDecode( (Byte *)pOutput, &outProcessed, (Byte *)(pInput + sizeof( lzma_header_t ) ),
	                          &inProcessed, (Byte *)pHeader->properties, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &g_Alloc );


	LzmaDec_Free(&state, &g_Alloc);

	if ( result != SZ_OK || pHeader->actualSize != outProcessed )
	{
		Warning( "LZMA Decompression failed (%i)\n", result );
		return 0;
	}

	return outProcessed;
}

CLZMAStream::CLZMAStream()
	: m_pDecoderState( NULL ),
	  m_nActualSize( 0 ),
	  m_nActualBytesRead ( 0 ),
	  m_nCompressedSize( 0 ),
	  m_nCompressedBytesRead ( 0 ),
	  m_bParsedHeader( false ),
	  m_bZIPStyleHeader( false )
{}

CLZMAStream::~CLZMAStream()
{
	FreeDecoderState();
}

void CLZMAStream::FreeDecoderState()
{
	if ( m_pDecoderState )
	{
		LzmaDec_Free( m_pDecoderState, &g_Alloc );
		m_pDecoderState = NULL;
	}
}

bool CLZMAStream::CreateDecoderState( const unsigned char *pProperties )
{
	if ( m_pDecoderState )
	{
		Assert( !m_pDecoderState );
		FreeDecoderState();
	}

	m_pDecoderState = new CLzmaDec();

	LzmaDec_Construct( m_pDecoderState );
	if ( LzmaDec_Allocate( m_pDecoderState, pProperties, LZMA_PROPS_SIZE, &g_Alloc) != SZ_OK )
	{
		AssertMsg( false, "Failed to allocate lzma decoder state" );
		m_pDecoderState = NULL;
		return false;
	}

	LzmaDec_Init( m_pDecoderState );

	return true;
}

// Attempt to read up to nMaxInputBytes from the compressed stream, writing up to nMaxOutputBytes to pOutput.
// Returns false if read stops due to an error.
bool CLZMAStream::Read( unsigned char *pInput, unsigned int nMaxInputBytes,
                        unsigned char *pOutput, unsigned int nMaxOutputBytes,
                        /* out */ unsigned int &nCompressedBytesRead,
                        /* out */ unsigned int &nOutputBytesWritten )
{
	nCompressedBytesRead = 0;
	nOutputBytesWritten = 0;
	bool bStartedWithHeader = m_bParsedHeader;

	// Check for initial chunk of data
	if ( !m_bParsedHeader )
	{
		unsigned int nBytesConsumed = 0;
		eHeaderParse parseResult = TryParseHeader( pInput, nMaxInputBytes, nBytesConsumed );

		if ( parseResult == eHeaderParse_NeedMoreBytes )
		{
			// Not an error, just need more data to continue
			return true;
		}
		else if ( parseResult != eHeaderParse_OK )
		{
			Assert( parseResult == eHeaderParse_Fail );
			// Invalid header
			return false;
		}

		// Header consumed, fall through to continue read after it
		nCompressedBytesRead += nBytesConsumed;
		pInput += nBytesConsumed;
		nMaxInputBytes -= nBytesConsumed;
	}

	// These are input ( available size ) *and* output ( size processed ) vars for lzma
	SizeT expectedInputRemaining = m_nCompressedSize - Min( m_nCompressedBytesRead + nCompressedBytesRead, m_nCompressedSize );
	SizeT expectedOutputRemaining = m_nActualSize - m_nActualBytesRead;
	SizeT inSize = Min( (SizeT)nMaxInputBytes, expectedInputRemaining );
	SizeT outSize = Min( (SizeT)nMaxOutputBytes, expectedOutputRemaining );
	ELzmaStatus status;
	ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
	if ( inSize == expectedInputRemaining && outSize == expectedOutputRemaining )
	{
		// Expect to finish decoding this call.
		finishMode = LZMA_FINISH_END;
	}
	SRes result = LzmaDec_DecodeToBuf( m_pDecoderState, pOutput, &outSize,
									   pInput, &inSize, finishMode, &status );

	// DevMsg("[%p] Running lzmaDecode:\n"
	//        "    pInput:             %p\n"
	//        "    nMaxInputBytes:     %i\n"
	//        "    pOutput:            %p\n"
	//        "    nMaxOutputBytes:    %u\n"
	//        "    inSize:             %u\n"
	//        "    outSize:            %u\n"
	//        "    result:             %u\n"
	//        "    status:             %i\n"
	//        "    m_nActualSize:      %u\n"
	//        "    m_nActualBytesRead: %u\n",
	//        this, pInput, nMaxInputBytes, pOutput, nMaxOutputBytes,
	//        inSize, outSize, result, status, m_nActualSize, m_nActualBytesRead);

	if ( result != SZ_OK )
	{
		if ( !bStartedWithHeader )
		{
			// If we're returning false, we need to pretend we didn't consume anything.
			FreeDecoderState();
			m_bParsedHeader = false;
		}
		return false;
	}

	nCompressedBytesRead += inSize;
	nOutputBytesWritten += outSize;

	m_nCompressedBytesRead += nCompressedBytesRead;
	m_nActualBytesRead += nOutputBytesWritten;

	Assert( m_nCompressedBytesRead <= m_nCompressedSize );
	return true;
}

bool CLZMAStream::GetExpectedBytesRemaining( /* out */ unsigned int &nBytesRemaining )
{
	if ( !m_bParsedHeader && !m_bZIPStyleHeader ) {
		return false;
	}

	nBytesRemaining = m_nActualSize - m_nActualBytesRead;

	return true;
}

void CLZMAStream::InitZIPHeader( unsigned int nCompressedSize, unsigned int nOriginalSize )
{
	if ( m_bParsedHeader || m_bZIPStyleHeader )
	{
		AssertMsg( !m_bParsedHeader && !m_bZIPStyleHeader,
		           "LZMA Stream: InitZIPHeader() called on stream past header" );
		return;
	}

	m_nCompressedSize = nCompressedSize;
	m_nActualSize = nOriginalSize;
	// Signal to TryParseHeader to expect a zip-style header (which wont have the size values)
	m_bZIPStyleHeader = true;
}

CLZMAStream::eHeaderParse CLZMAStream::TryParseHeader( unsigned char *pInput, unsigned int nBytesAvailable, /* out */ unsigned int &nBytesConsumed )
{
	nBytesConsumed = 0;

	if ( m_bParsedHeader  )
	{
		AssertMsg( !m_bParsedHeader, "CLZMAStream::ReadSourceHeader called on already initialized stream" );
		return eHeaderParse_Fail;
	}

	if ( m_bZIPStyleHeader )
	{
		// ZIP Spec, 5.8.8
		//   LZMA Version Information 2 bytes
		//   LZMA Properties Size 2 bytes
		//   LZMA Properties Data variable, defined by "LZMA Properties Size"

		if ( nBytesAvailable < 4 )
		{
			// No error, but need more input to continue
			return eHeaderParse_NeedMoreBytes;
		}

		// Should probably check this
		// unsigned char nLZMAVer[2] = { pInput[0], pInput[1] };

		uint16 nLZMAPropertiesSize = LittleWord( *(uint16 *)(pInput + 2) );

		nBytesConsumed += 4;

		if ( nLZMAPropertiesSize != LZMA_PROPS_SIZE )
		{
			Warning( "LZMA stream: Unexpected LZMA properties size: %hu, expecting %u. Version mismatch?\n",
			         nLZMAPropertiesSize, LZMA_PROPS_SIZE );
			return eHeaderParse_Fail;
		}

		if ( nBytesAvailable < static_cast<unsigned int>(nLZMAPropertiesSize) + 4 )
		{
			return eHeaderParse_NeedMoreBytes;
		}

		// Looks reasonable, try to parse
		if ( !CreateDecoderState( (Byte *)pInput + 4 ) )
		{
			AssertMsg( false, "Failed decoding Lzma properties" );
			return eHeaderParse_Fail;
		}

		nBytesConsumed += nLZMAPropertiesSize;
	}
	else
	{
		// Else native source engine style header
		if ( nBytesAvailable < sizeof( lzma_header_t ) )
		{
			// need more input to continue
			return eHeaderParse_NeedMoreBytes;
		}

		m_nActualSize = CLZMA::GetActualSize( pInput );

		if ( !m_nActualSize )
		{
			// unrecognized
			Warning( "Unrecognized LZMA data\n" );
			return eHeaderParse_Fail;
		}

		if ( !CreateDecoderState( ((lzma_header_t *)pInput)->properties ) )
		{
			AssertMsg( false, "Failed decoding Lzma properties" );
			return eHeaderParse_Fail;
		}

		m_nCompressedSize = LittleLong( ((lzma_header_t *)pInput)->lzmaSize ) + sizeof( lzma_header_t );
		nBytesConsumed += sizeof( lzma_header_t );
	}

	m_bParsedHeader = true;
	return eHeaderParse_OK;
}
