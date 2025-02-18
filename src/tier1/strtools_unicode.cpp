//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <limits.h>
#include "tier0/dbg.h"
#include "tier1/strtools.h"

// This code was copied from steam
#define DbgAssert Assert

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32( uchar32 uVal )
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return ( uVal < 0x110000u ) && ( (uVal - 0x00D800u) > 0x7FFu ) && ( (uVal & 0xFFFFu) < 0xFFFEu ) && ( ( uVal - 0x00FDD0u ) > 0x1Fu );
}

//-----------------------------------------------------------------------------
// Purpose: return number of UTF-8 bytes required to encode a Unicode code point
//-----------------------------------------------------------------------------
int Q_UChar32ToUTF8Len( uchar32 uVal )
{
	DbgAssert( Q_IsValidUChar32( uVal ) );
	if ( uVal <= 0x7F )
		return 1;
	if ( uVal <= 0x7FF )
		return 2;
	if ( uVal <= 0xFFFF )
		return 3;
	return 4;
}


//-----------------------------------------------------------------------------
// Purpose: return number of UTF-16 elements required to encode a Unicode code point
//-----------------------------------------------------------------------------
int Q_UChar32ToUTF16Len( uchar32 uVal )
{
	DbgAssert( Q_IsValidUChar32( uVal ) );
	if ( uVal <= 0xFFFF )
		return 1;
	return 2;
}


//-----------------------------------------------------------------------------
// Purpose: encode Unicode code point as UTF-8, returns number of bytes written
//-----------------------------------------------------------------------------
int Q_UChar32ToUTF8( uchar32 uVal, char *pUTF8Out )
{
	DbgAssert( Q_IsValidUChar32( uVal ) );
	if ( uVal <= 0x7F )
	{
		pUTF8Out[0] = (unsigned char) uVal;
		return 1;
	}
	if ( uVal <= 0x7FF )
	{
		pUTF8Out[0] = (unsigned char)(uVal >> 6) | 0xC0;
		pUTF8Out[1] = (unsigned char)(uVal & 0x3F) | 0x80;
		return 2;
	}
	if ( uVal <= 0xFFFF )
	{
		pUTF8Out[0] = (unsigned char)(uVal >> 12) | 0xE0;
		pUTF8Out[1] = (unsigned char)((uVal >> 6) & 0x3F) | 0x80;
		pUTF8Out[2] = (unsigned char)(uVal & 0x3F) | 0x80;
		return 3;
	}
	pUTF8Out[0] = (unsigned char)((uVal >> 18) & 0x07) | 0xF0;
	pUTF8Out[1] = (unsigned char)((uVal >> 12) & 0x3F) | 0x80;
	pUTF8Out[2] = (unsigned char)((uVal >> 6) & 0x3F) | 0x80;
	pUTF8Out[3] = (unsigned char)(uVal & 0x3F) | 0x80;
	return 4;
}

//-----------------------------------------------------------------------------
// Purpose: encode Unicode code point as UTF-16, returns number of elements written
//-----------------------------------------------------------------------------
int Q_UChar32ToUTF16( uchar32 uVal, uchar16 *pUTF16Out )
{
	DbgAssert( Q_IsValidUChar32( uVal ) );
	if ( uVal <= 0xFFFF )
	{
		pUTF16Out[0] = (uchar16) uVal;
		return 1;
	}
	uVal -= 0x010000;
	pUTF16Out[0] = (uchar16)(uVal >> 10) | 0xD800;
	pUTF16Out[1] = (uchar16)(uVal & 0x3FF) | 0xDC00;
	return 2;
}
	

// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32( const char *pUTF8_, uchar32 &uValueOut, bool &bErrorOut )
{
	const uint8 *pUTF8 = (const uint8 *)pUTF8_;

	int nBytes = 1;
	uint32 uValue = pUTF8[0];
	uint32 uMinValue = 0;

	// 0....... single byte
	if ( uValue < 0x80 )
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if ( (uValue - 0xC0u) > 0x37u || ( pUTF8[1] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = (uValue << 6) - (0xC0 << 6) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if ( !( uValue & (0x20 << 6) ) )
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if ( ( pUTF8[2] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = (uValue << 6) - (0x20 << 12) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
	if ( !( uValue & (0x10 << 12) ) )
		goto decodeFinishedMaybeCESU8;

	// Expecting a four-byte sequence, longest permissible in UTF-8
	if ( ( pUTF8[3] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = (uValue << 6) - (0x10 << 18) + pUTF8[3] - 0x80;
	nBytes = 4;
	uMinValue = 0x10000;

	// 11110... four-byte lead byte. fall through to finished.

decodeFinished:
	if ( uValue >= uMinValue && Q_IsValidUChar32( uValue ) )
	{
decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if ( ( uValue - 0xD800u ) < 0x400u && pUTF8[3] == 0xED && (uint8)( pUTF8[4] - 0xB0 ) < 0x10 && ( pUTF8[5] & 0xC0 ) == 0x80 )
	{
		uValue = 0x10000 + ( ( uValue - 0xD800u ) << 10 ) + ( (uint8)( pUTF8[4] - 0xB0 ) << 6 ) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}

// Decode one character from a UTF-16 encoded string.
int Q_UTF16ToUChar32( const uchar16 *pUTF16, uchar32 &uValueOut, bool &bErrorOut )
{
	if ( Q_IsValidUChar32( pUTF16[0] ) )
	{
		uValueOut = pUTF16[0];
		bErrorOut = false;
		return 1;
	}
	else if ( (pUTF16[0] - 0xD800u) < 0x400u && (pUTF16[1] - 0xDC00u) < 0x400u )
	{
		// Valid surrogate pair, but maybe not encoding a valid Unicode code point...
		uchar32 uVal = 0x010000 + ((pUTF16[0] - 0xD800u) << 10) + (pUTF16[1] - 0xDC00);
		if ( Q_IsValidUChar32( uVal ) )
		{
			uValueOut = uVal;
			bErrorOut = false;
			return 2;
		}
		else
		{
			uValueOut = '?';
			bErrorOut = true;
			return 2;
		}
	}
	else
	{
		uValueOut = '?';
		bErrorOut = true;
		return 1;
	}
}

namespace // internal use only
{
	// Identity transformations and validity tests for use with Q_UnicodeConvertT
	int Q_UTF32ToUChar32( const uchar32 *pUTF32, uchar32 &uVal, bool &bErr )
	{
		bErr = !Q_IsValidUChar32( *pUTF32 );
		uVal = bErr ? '?' : *pUTF32;
		return 1;
	}

	int Q_UChar32ToUTF32Len( uchar32 uVal )
	{
		return 1;
	}

	int Q_UChar32ToUTF32( uchar32 uVal, uchar32 *pUTF32 )
	{
		*pUTF32 = uVal;
		return 1;
	}

	// A generic Unicode processing loop: decode one character from input to uchar32, handle errors, encode uchar32 to output
	template < typename SrcType, typename DstType, bool bStopAtNull, int (&DecodeSrc)( const SrcType*, uchar32&, bool& ), int (&EncodeDstLen)( uchar32 ), int (&EncodeDst)( uchar32, DstType* ) >
	int Q_UnicodeConvertT( const SrcType *pIn, int nInChars, DstType *pOut, int nOutBytes, EStringConvertErrorPolicy ePolicy )
	{
		if ( !pIn )
		{
			// For now, assert and return 0. Once these are cleaned out a bit
			//  we should remove this return and just leave in the assert...
			AssertMsg( pIn, "We shouldn't be passing in NULL!" );
			return 0;
		}

		int nOut = 0;

		if ( !pOut )
		{
			while ( bStopAtNull ? ( *pIn ) : ( nInChars-- > 0 ) )
			{
				uchar32 uVal;
				// Initialize in order to avoid /analyze warnings.
				bool bErr = false;
				pIn += DecodeSrc( pIn, uVal, bErr );
				nOut += EncodeDstLen( uVal );
				if ( bErr )
				{
#ifdef _DEBUG
					AssertMsg( !(ePolicy & _STRINGCONVERTFLAG_ASSERT), "invalid Unicode byte sequence" );
#endif
					if ( ePolicy & _STRINGCONVERTFLAG_SKIP )
					{
						nOut -= EncodeDstLen( uVal );
					}
					else if ( ePolicy & _STRINGCONVERTFLAG_FAIL )
					{
						pOut[0] = 0;
						return 0;
					}
				}
			}
		}
		else
		{
			int nOutElems = nOutBytes / sizeof( DstType );
			if ( nOutElems <= 0 )
				return 0;

			int nMaxOut = nOutElems - 1;
			while ( bStopAtNull ? ( *pIn ) : ( nInChars-- > 0 ) )
			{
				uchar32 uVal;
				// Initialize in order to avoid /analyze warnings.
				bool bErr = false;
				pIn += DecodeSrc( pIn, uVal, bErr );
				if ( nOut + EncodeDstLen( uVal ) > nMaxOut )
					break;
				nOut += EncodeDst( uVal, pOut + nOut );
				if ( bErr )
				{
#ifdef _DEBUG
					AssertMsg( !(ePolicy & _STRINGCONVERTFLAG_ASSERT), "invalid Unicode byte sequence" );
#endif
					if ( ePolicy & _STRINGCONVERTFLAG_SKIP )
					{
						nOut -= EncodeDstLen( uVal );
					}
					else if ( ePolicy & _STRINGCONVERTFLAG_FAIL )
					{
						pOut[0] = 0;
						return 0;
					}
				}
			}
			pOut[nOut] = 0;
		}

		return (nOut + 1) * sizeof( DstType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate( const char *pUTF8 )
{
	bool bError = false;
	while ( *pUTF8 )
	{
		uchar32 uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		int nCharSize = Q_UTF8ToUChar32( pUTF8, uVal, bError );
		if ( bError || nCharSize == 6 )
			return false;
		pUTF8 += nCharSize;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-16 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate( const uchar16 *pUTF16 )
{
	bool bError = false;
	while ( *pUTF16 )
	{
		uchar32 uVal;
		pUTF16 += Q_UTF16ToUChar32( pUTF16, uVal, bError );
		if ( bError )
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-32 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate( const uchar32 *pUTF32 )
{
	while ( *pUTF32 )
	{
		if ( !Q_IsValidUChar32( *pUTF32++ ) )
			return false;
		++pUTF32;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of Unicode code points (aka glyphs / characters) encoded in the UTF-8 string
//-----------------------------------------------------------------------------
int Q_UnicodeLength( const char *pUTF8 )
{
	int nChars = 0;
	while ( *pUTF8 )
	{
		bool bError;
		uchar32 uVal;
		pUTF8 += Q_UTF8ToUChar32( pUTF8, uVal, bError );
		++nChars;
	}
	return nChars;
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of Unicode code points (aka glyphs / characters) encoded in the UTF-16 string
//-----------------------------------------------------------------------------
int Q_UnicodeLength( const uchar16 *pUTF16 )
{
	int nChars = 0;
	while ( *pUTF16 )
	{
		bool bError;
		uchar32 uVal;
		pUTF16 += Q_UTF16ToUChar32( pUTF16, uVal, bError );
		++nChars;
	}
	return nChars;
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of Unicode code points (aka glyphs / characters) encoded in the UTF-32 string
//-----------------------------------------------------------------------------
int Q_UnicodeLength( const uchar32 *pUTF32 )
{
	int nChars = 0;
	while ( *pUTF32++ )
		++nChars;
	return nChars;
}

//-----------------------------------------------------------------------------
// Purpose: Advance a UTF-8 string pointer by a certain number of Unicode code points, stopping at end of string
//-----------------------------------------------------------------------------
char *Q_UnicodeAdvance( char *pUTF8, int nChars )
{
	while ( nChars > 0 && *pUTF8 )
	{
		uchar32 uVal;
		bool bError;
		pUTF8 += Q_UTF8ToUChar32( pUTF8, uVal, bError );
		--nChars;
	}
	return pUTF8;
}

//-----------------------------------------------------------------------------
// Purpose: Advance a UTF-16 string pointer by a certain number of Unicode code points, stopping at end of string
//-----------------------------------------------------------------------------
uchar16 *Q_UnicodeAdvance( uchar16 *pUTF16, int nChars )
{
	while ( nChars > 0 && *pUTF16 )
	{
		uchar32 uVal;
		bool bError;
		pUTF16 += Q_UTF16ToUChar32( pUTF16, uVal, bError );
		--nChars;
	}
	return pUTF16;
}
	
//-----------------------------------------------------------------------------
// Purpose: Advance a UTF-32 string pointer by a certain number of Unicode code points, stopping at end of string
//-----------------------------------------------------------------------------
uchar32 *Q_UnicodeAdvance( uchar32 *pUTF32, int nChars )
{
	while ( nChars > 0 && *pUTF32 )
	{
		++pUTF32;
		--nChars;
	}
	return pUTF32;
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF8ToUTF16( const char *pUTF8, uchar16 *pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< char, uchar16, true, Q_UTF8ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16 >( pUTF8, 0, pUTF16, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF8ToUTF32( const char *pUTF8, uchar32 *pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< char, uchar32, true, Q_UTF8ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF8, 0, pUTF32, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF16ToUTF8( const uchar16 *pUTF16, char *pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar16, char, true, Q_UTF16ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8 >( pUTF16, 0, pUTF8, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF16ToUTF32( const uchar16 *pUTF16, uchar32 *pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar16, uchar32, true, Q_UTF16ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF16, 0, pUTF32, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF32ToUTF8( const uchar32 *pUTF32, char *pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, char, true, Q_UTF32ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8 >( pUTF32, 0, pUTF8, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF32ToUTF16( const uchar32 *pUTF32, uchar16 *pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, uchar16, true, Q_UTF32ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16 >( pUTF32, 0, pUTF16, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF32ToUTF32( const uchar32 *pUTF32Source, uchar32 *pUTF32Dest, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, uchar32, true, Q_UTF32ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF32Source, 0, pUTF32Dest, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF8CharsToUTF16( const char *pUTF8, int nElements, uchar16 *pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< char, uchar16, false, Q_UTF8ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16 >( pUTF8, nElements, pUTF16, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF8CharsToUTF32( const char *pUTF8, int nElements, uchar32 *pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< char, uchar32, false, Q_UTF8ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF8, nElements, pUTF32, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF16CharsToUTF8( const uchar16 *pUTF16, int nElements, char *pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar16, char, false, Q_UTF16ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8 >( pUTF16, nElements, pUTF8, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF16CharsToUTF32( const uchar16 *pUTF16, int nElements, uchar32 *pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar16, uchar32, false, Q_UTF16ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF16, nElements, pUTF32, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF32CharsToUTF8( const uchar32 *pUTF32, int nElements, char *pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, char, false, Q_UTF32ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8 >( pUTF32, nElements, pUTF8, cubDestSizeInBytes, ePolicy );
}
	
//-----------------------------------------------------------------------------
// Purpose: Perform conversion. Returns number of *bytes* required if output pointer is NULL.
//-----------------------------------------------------------------------------
int Q_UTF32CharsToUTF16( const uchar32 *pUTF32, int nElements, uchar16 *pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, uchar16, false, Q_UTF32ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16 >( pUTF32, nElements, pUTF16, cubDestSizeInBytes, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Repair a UTF-8 string by removing or replacing invalid seqeuences. Returns non-zero on success.
//-----------------------------------------------------------------------------
int Q_UnicodeRepair( char *pUTF8, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< char, char, true, Q_UTF8ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8 >( pUTF8, 0, pUTF8, INT_MAX, ePolicy );
}

//-----------------------------------------------------------------------------
// Purpose: Repair a UTF-16 string by removing or replacing invalid seqeuences. Returns non-zero on success.
//-----------------------------------------------------------------------------
int Q_UnicodeRepair( uchar16 *pUTF16, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar16, uchar16, true, Q_UTF16ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16 >( pUTF16, 0, pUTF16, INT_MAX/sizeof(uchar16), ePolicy );
}
	
//-----------------------------------------------------------------------------
// Purpose: Repair a UTF-32 string by removing or replacing invalid seqeuences. Returns non-zero on success.
//-----------------------------------------------------------------------------
int Q_UnicodeRepair( uchar32 *pUTF32, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT< uchar32, uchar32, true, Q_UTF32ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32 >( pUTF32, 0, pUTF32, INT_MAX/sizeof(uchar32), ePolicy );
}

