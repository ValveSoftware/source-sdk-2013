//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "bitbuf.h"
#include "coordsize.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include "tier1/strtools.h"
#include "bitvec.h"

// FIXME: Can't use this until we get multithreaded allocations in tier0 working for tools
// This is used by VVIS and fails to link
// NOTE: This must be the last file included!!!
//#include "tier0/memdbgon.h"

#ifdef _X360
// mandatory ... wary of above comment and isolating, tier0 is built as MT though
#include "tier0/memdbgon.h"
#endif

#include "stdio.h"

#if 0

void CBitWrite::StartWriting( void *pData, int nBytes, int iStartBit, int nBits )
{
	// Make sure it's dword aligned and padded.
	Assert( (nBytes % 4) == 0 );
	Assert(((unsigned long)pData & 3) == 0);
	Assert( iStartBit == 0 );
	m_pData = (uint32 *) pData;
	m_pDataOut = m_pData;
	m_nDataBytes = nBytes;

	if ( nBits == -1 )
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		Assert( nBits <= nBytes*8 );
		m_nDataBits = nBits;
	}
	m_bOverflow = false;
	m_nOutBufWord = 0;
	m_nOutBitsAvail = 32;
	m_pBufferEnd = m_pDataOut + ( nBytes >> 2 );
}

const uint32 CBitBuffer::s_nMaskTable[33] = {
	0,
	( 1 << 1 ) - 1,
	( 1 << 2 ) - 1,
	( 1 << 3 ) - 1,
	( 1 << 4 ) - 1,
	( 1 << 5 ) - 1,
	( 1 << 6 ) - 1,
	( 1 << 7 ) - 1,
	( 1 << 8 ) - 1,
	( 1 << 9 ) - 1,
	( 1 << 10 ) - 1,
	( 1 << 11 ) - 1,
	( 1 << 12 ) - 1,
	( 1 << 13 ) - 1,
	( 1 << 14 ) - 1,
	( 1 << 15 ) - 1,
	( 1 << 16 ) - 1,
	( 1 << 17 ) - 1,
	( 1 << 18 ) - 1,
	( 1 << 19 ) - 1,
   	( 1 << 20 ) - 1,
	( 1 << 21 ) - 1,
	( 1 << 22 ) - 1,
	( 1 << 23 ) - 1,
	( 1 << 24 ) - 1,
	( 1 << 25 ) - 1,
	( 1 << 26 ) - 1,
   	( 1 << 27 ) - 1,
	( 1 << 28 ) - 1,
	( 1 << 29 ) - 1,
	( 1 << 30 ) - 1,
	0x7fffffff,
	0xffffffff,
};

bool CBitWrite::WriteString( const char *pStr )
{
	if(pStr)
	{
		while( *pStr )
		{
			WriteChar( * ( pStr++ ) );
		}
	}
	WriteChar( 0 );
	return !IsOverflowed();
}

			 
void CBitWrite::WriteLongLong(int64 val)
{
	uint *pLongs = (uint*)&val;

	// Insert the two DWORDS according to network endian
	const short endianIndex = 0x0100;
	byte *idx = (byte*)&endianIndex;
	WriteUBitLong(pLongs[*idx++], sizeof(long) << 3);
	WriteUBitLong(pLongs[*idx], sizeof(long) << 3);
}

bool CBitWrite::WriteBits(const void *pInData, int nBits)
{
	unsigned char *pOut = (unsigned char*)pInData;
	int nBitsLeft = nBits;

	// Bounds checking..
	if ( ( GetNumBitsWritten() + nBits) > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, m_pDebugName );
		return false;
	}

	// !! speed!! need fast paths
	// write remaining bytes
	while ( nBitsLeft >= 8 )
	{
		WriteUBitLong( *pOut, 8, false );
		++pOut;
		nBitsLeft -= 8;
	}
	
	// write remaining bits
	if ( nBitsLeft )
	{
		WriteUBitLong( *pOut, nBitsLeft, false );
	}

	return !IsOverflowed();
}

void CBitWrite::WriteBytes( const void *pBuf, int nBytes )
{
	WriteBits(pBuf, nBytes << 3);
}

void CBitWrite::WriteBitCoord (const float f)
{
	int		signbit = (f <= -COORD_RESOLUTION);
	int		intval = (int)abs(f);
	int		fractval = abs((int)(f*COORD_DENOMINATOR)) & (COORD_DENOMINATOR-1);


	// Send the bit flags that indicate whether we have an integer part and/or a fraction part.
	WriteOneBit( intval );
	WriteOneBit( fractval );

	if ( intval || fractval )
	{
		// Send the sign bit
		WriteOneBit( signbit );

		// Send the integer if we have one.
		if ( intval )
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			WriteUBitLong( (unsigned int)intval, COORD_INTEGER_BITS );
		}
		
		// Send the fraction if we have one
		if ( fractval )
		{
			WriteUBitLong( (unsigned int)fractval, COORD_FRACTIONAL_BITS );
		}
	}
}

void CBitWrite::WriteBitCoordMP (const float f, bool bIntegral, bool bLowPrecision )
{
	int		signbit = (f <= -( bLowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION ));
	int		intval = (int)abs(f);
	int		fractval = bLowPrecision ? 
		( abs((int)(f*COORD_DENOMINATOR_LOWPRECISION)) & (COORD_DENOMINATOR_LOWPRECISION-1) ) :
		( abs((int)(f*COORD_DENOMINATOR)) & (COORD_DENOMINATOR-1) );

	bool    bInBounds = intval < (1 << COORD_INTEGER_BITS_MP );

	WriteOneBit( bInBounds );

	if ( bIntegral )
	{
		// Send the sign bit
		WriteOneBit( intval );
		if ( intval )
		{
			WriteOneBit( signbit );
			// Send the integer if we have one.
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			if ( bInBounds )
			{
				WriteUBitLong( (unsigned int)intval, COORD_INTEGER_BITS_MP );
			}
			else
			{
				WriteUBitLong( (unsigned int)intval, COORD_INTEGER_BITS );
			}
		}
	}
	else
	{
		// Send the bit flags that indicate whether we have an integer part and/or a fraction part.
		WriteOneBit( intval );
		// Send the sign bit
		WriteOneBit( signbit );

		// Send the integer if we have one.
		if ( intval )
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			if ( bInBounds )
			{
				WriteUBitLong( (unsigned int)intval, COORD_INTEGER_BITS_MP );
			}
			else
			{
				WriteUBitLong( (unsigned int)intval, COORD_INTEGER_BITS );
			}
		}
		WriteUBitLong( (unsigned int)fractval, bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS );
	}
}

void CBitWrite::SeekToBit( int nBit )
{
	TempFlush();
	m_pDataOut = m_pData + ( nBit / 32 );
	m_nOutBufWord = *( m_pDataOut );
	m_nOutBitsAvail = 32 - ( nBit & 31 );
}



void CBitWrite::WriteBitVec3Coord( const Vector& fa )
{
	int		xflag, yflag, zflag;

	xflag = (fa[0] >= COORD_RESOLUTION) || (fa[0] <= -COORD_RESOLUTION);
	yflag = (fa[1] >= COORD_RESOLUTION) || (fa[1] <= -COORD_RESOLUTION);
	zflag = (fa[2] >= COORD_RESOLUTION) || (fa[2] <= -COORD_RESOLUTION);

	WriteOneBit( xflag );
	WriteOneBit( yflag );
	WriteOneBit( zflag );

	if ( xflag )
		WriteBitCoord( fa[0] );
	if ( yflag )
		WriteBitCoord( fa[1] );
	if ( zflag )
		WriteBitCoord( fa[2] );
}

void CBitWrite::WriteBitNormal( float f )
{
	int	signbit = (f <= -NORMAL_RESOLUTION);

	// NOTE: Since +/-1 are valid values for a normal, I'm going to encode that as all ones
	unsigned int fractval = abs( (int)(f*NORMAL_DENOMINATOR) );

	// clamp..
	if (fractval > NORMAL_DENOMINATOR)
		fractval = NORMAL_DENOMINATOR;

	// Send the sign bit
	WriteOneBit( signbit );

	// Send the fractional component
	WriteUBitLong( fractval, NORMAL_FRACTIONAL_BITS );
}

void CBitWrite::WriteBitVec3Normal( const Vector& fa )
{
	int		xflag, yflag;

	xflag = (fa[0] >= NORMAL_RESOLUTION) || (fa[0] <= -NORMAL_RESOLUTION);
	yflag = (fa[1] >= NORMAL_RESOLUTION) || (fa[1] <= -NORMAL_RESOLUTION);

	WriteOneBit( xflag );
	WriteOneBit( yflag );

	if ( xflag )
		WriteBitNormal( fa[0] );
	if ( yflag )
		WriteBitNormal( fa[1] );
	
	// Write z sign bit
	int	signbit = (fa[2] <= -NORMAL_RESOLUTION);
	WriteOneBit( signbit );
}

void CBitWrite::WriteBitAngle( float fAngle, int numbits )
{

	unsigned int shift = GetBitForBitnum(numbits);
	unsigned int mask = shift - 1;

	int d = (int)( (fAngle / 360.0) * shift );
	d &= mask;

	WriteUBitLong((unsigned int)d, numbits);
}

bool CBitWrite::WriteBitsFromBuffer( bf_read *pIn, int nBits )
{
// This could be optimized a little by
	while ( nBits > 32 )
	{
		WriteUBitLong( pIn->ReadUBitLong( 32 ), 32 );
		nBits -= 32;
	}

	WriteUBitLong( pIn->ReadUBitLong( nBits ), nBits );
	return !IsOverflowed() && !pIn->IsOverflowed();
}

void CBitWrite::WriteBitAngles( const QAngle& fa )
{
	// FIXME:
	Vector tmp( fa.x, fa.y, fa.z );
	WriteBitVec3Coord( tmp );
}

bool CBitRead::Seek( int nPosition )
{
	bool bSucc = true;
	if ( nPosition < 0 || nPosition > m_nDataBits)
	{
		SetOverflowFlag();
		bSucc = false;
		nPosition = m_nDataBits;
	}
	int nHead = m_nDataBytes & 3;							// non-multiple-of-4 bytes at head of buffer. We put the "round off"
															// at the head to make reading and detecting the end efficient.
	
	int nByteOfs = nPosition / 8;
	if ( ( m_nDataBytes < 4 ) || ( nHead && ( nByteOfs < nHead ) ) )
	{
		// partial first dword
		uint8 const *pPartial = ( uint8 const *) m_pData;
		if ( m_pData )
		{
			m_nInBufWord = *( pPartial++ );
			if ( nHead > 1 )
				m_nInBufWord |= ( *pPartial++ )  << 8;
			if ( nHead > 2 )
				m_nInBufWord |= ( *pPartial++ ) << 16;
		}
		m_pDataIn = ( uint32 const * ) pPartial;
		m_nInBufWord >>= ( nPosition & 31 );
		m_nBitsAvail = ( nHead << 3 ) - ( nPosition & 31 );
	}
	else
	{
		int nAdjPosition = nPosition - ( nHead << 3 );
		m_pDataIn = reinterpret_cast<uint32 const *> (
			reinterpret_cast<uint8 const *>( m_pData ) + ( ( nAdjPosition / 32 ) << 2 ) + nHead );
		if ( m_pData )
		{
			m_nBitsAvail = 32;
			GrabNextDWord();
		}
		else
		{
			m_nInBufWord = 0;
			m_nBitsAvail = 1;
		}
		m_nInBufWord >>= ( nAdjPosition & 31 );
		m_nBitsAvail = min( m_nBitsAvail, 32 - ( nAdjPosition & 31 ) );	// in case grabnextdword overflowed
	}
	return bSucc;
}


void CBitRead::StartReading( const void *pData, int nBytes, int iStartBit, int nBits )
{
// Make sure it's dword aligned and padded.
	Assert(((unsigned long)pData & 3) == 0);
	m_pData = (uint32 *) pData;
	m_pDataIn = m_pData;
	m_nDataBytes = nBytes;

	if ( nBits == -1 )
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		Assert( nBits <= nBytes*8 );
		m_nDataBits = nBits;
	}
	m_bOverflow = false;
	m_pBufferEnd = reinterpret_cast<uint32 const *> ( reinterpret_cast< uint8 const *> (m_pData) + nBytes );
	if ( m_pData )
		Seek( iStartBit ); 
	
}

bool CBitRead::ReadString( char *pStr, int maxLen, bool bLine, int *pOutNumChars )
{
	Assert( maxLen != 0 );

	bool bTooSmall = false;
	int iChar = 0;
	while(1)
	{
		char val = ReadChar();
		if ( val == 0 )
			break;
		else if ( bLine && val == '\n' )
			break;

		if ( iChar < (maxLen-1) )
		{
			pStr[iChar] = val;
			++iChar;
		}
		else
		{
			bTooSmall = true;
		}
	}

	// Make sure it's null-terminated.
	Assert( iChar < maxLen );
	pStr[iChar] = 0;

	if ( pOutNumChars )
		*pOutNumChars = iChar;

	return !IsOverflowed() && !bTooSmall;
}

char* CBitRead::ReadAndAllocateString( bool *pOverflow )
{
	char str[2048];
	
	int nChars;
	bool bOverflow = !ReadString( str, sizeof( str ), false, &nChars );
	if ( pOverflow )
		*pOverflow = bOverflow;

	// Now copy into the output and return it;
	char *pRet = new char[ nChars + 1 ];
	for ( int i=0; i <= nChars; i++ )
		pRet[i] = str[i];

	return pRet;
}

int64 CBitRead::ReadLongLong( void )
{
	int64 retval;
	uint *pLongs = (uint*)&retval;

	// Read the two DWORDs according to network endian
	const short endianIndex = 0x0100;
	byte *idx = (byte*)&endianIndex;
	pLongs[*idx++] = ReadUBitLong(sizeof(long) << 3);
	pLongs[*idx] = ReadUBitLong(sizeof(long) << 3);
	return retval;
}

void CBitRead::ReadBits(void *pOutData, int nBits)
{
	unsigned char *pOut = (unsigned char*)pOutData;
	int nBitsLeft = nBits;

	
	// align output to dword boundary
	while( ((unsigned long)pOut & 3) != 0 && nBitsLeft >= 8 )
	{
		*pOut = (unsigned char)ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}

	// X360TBD: Can't read dwords in ReadBits because they'll get swapped
	if ( IsPC() )
	{
		// read dwords
		while ( nBitsLeft >= 32 )
		{
			*((unsigned long*)pOut) = ReadUBitLong(32);
			pOut += sizeof(unsigned long);
			nBitsLeft -= 32;
		}
	}

	// read remaining bytes
	while ( nBitsLeft >= 8 )
	{
		*pOut = ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}
	
	// read remaining bits
	if ( nBitsLeft )
	{
		*pOut = ReadUBitLong(nBitsLeft);
	}

}

bool CBitRead::ReadBytes(void *pOut, int nBytes)
{
	ReadBits(pOut, nBytes << 3);
	return !IsOverflowed();
}

float CBitRead::ReadBitAngle( int numbits )
{
	float shift = (float)( GetBitForBitnum(numbits) );

	int i = ReadUBitLong( numbits );
	float fReturn = (float)i * (360.0 / shift);

	return fReturn;
}

// Basic Coordinate Routines (these contain bit-field size AND fixed point scaling constants)
float CBitRead::ReadBitCoord (void)
{
	int		intval=0,fractval=0,signbit=0;
	float	value = 0.0;


	// Read the required integer and fraction flags
	intval = ReadOneBit();
	fractval = ReadOneBit();

	// If we got either parse them, otherwise it's a zero.
	if ( intval || fractval )
	{
		// Read the sign bit
		signbit = ReadOneBit();

		// If there's an integer, read it in
		if ( intval )
		{
			// Adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			intval = ReadUBitLong( COORD_INTEGER_BITS ) + 1;
		}

		// If there's a fraction, read it in
		if ( fractval )
		{
			fractval = ReadUBitLong( COORD_FRACTIONAL_BITS );
		}

		// Calculate the correct floating point value
		value = intval + ((float)fractval * COORD_RESOLUTION);

		// Fixup the sign if negative.
		if ( signbit )
			value = -value;
	}

	return value;
}

float CBitRead::ReadBitCoordMP( bool bIntegral, bool bLowPrecision )
{
	int		intval=0,fractval=0,signbit=0;
	float	value = 0.0;

	bool bInBounds = ReadOneBit() ? true : false;

	if ( bIntegral )
	{
		// Read the required integer and fraction flags
		intval = ReadOneBit();
		// If we got either parse them, otherwise it's a zero.
		if ( intval )
		{
			// Read the sign bit
			signbit = ReadOneBit();

			// If there's an integer, read it in
			// Adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			if ( bInBounds )
			{
				value = ReadUBitLong( COORD_INTEGER_BITS_MP ) + 1;
			}
			else
			{
				value = ReadUBitLong( COORD_INTEGER_BITS ) + 1;
			}
		}
	}
	else
	{
		// Read the required integer and fraction flags
		intval = ReadOneBit();

		// Read the sign bit
		signbit = ReadOneBit();

		// If we got either parse them, otherwise it's a zero.
		if ( intval )
		{
			if ( bInBounds )
			{
				intval = ReadUBitLong( COORD_INTEGER_BITS_MP ) + 1;
			}
			else
			{
				intval = ReadUBitLong( COORD_INTEGER_BITS ) + 1;
			}
		}

		// If there's a fraction, read it in
		fractval = ReadUBitLong( bLowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS );

		// Calculate the correct floating point value
		value = intval + ((float)fractval * ( bLowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION ) );
	}

	// Fixup the sign if negative.
	if ( signbit )
		value = -value;

	return value;
}

void CBitRead::ReadBitVec3Coord( Vector& fa )
{
	int		xflag, yflag, zflag;

	// This vector must be initialized! Otherwise, If any of the flags aren't set, 
	// the corresponding component will not be read and will be stack garbage.
	fa.Init( 0, 0, 0 );

	xflag = ReadOneBit();
	yflag = ReadOneBit(); 
	zflag = ReadOneBit();

	if ( xflag )
		fa[0] = ReadBitCoord();
	if ( yflag )
		fa[1] = ReadBitCoord();
	if ( zflag )
		fa[2] = ReadBitCoord();
}

float CBitRead::ReadBitNormal (void)
{
	// Read the sign bit
	int	signbit = ReadOneBit();

	// Read the fractional part
	unsigned int fractval = ReadUBitLong( NORMAL_FRACTIONAL_BITS );

	// Calculate the correct floating point value
	float value = (float)fractval * NORMAL_RESOLUTION;

	// Fixup the sign if negative.
	if ( signbit )
		value = -value;

	return value;
}

void CBitRead::ReadBitVec3Normal( Vector& fa )
{
	int xflag = ReadOneBit();
	int yflag = ReadOneBit(); 

	if (xflag)
		fa[0] = ReadBitNormal();
	else
		fa[0] = 0.0f;

	if (yflag)
		fa[1] = ReadBitNormal();
	else
		fa[1] = 0.0f;

	// The first two imply the third (but not its sign)
	int znegative = ReadOneBit();

	float fafafbfb = fa[0] * fa[0] + fa[1] * fa[1];
	if (fafafbfb < 1.0f)
		fa[2] = sqrt( 1.0f - fafafbfb );
	else
		fa[2] = 0.0f;

	if (znegative)
		fa[2] = -fa[2];
}

void CBitRead::ReadBitAngles( QAngle& fa )
{
	Vector tmp;
	ReadBitVec3Coord( tmp );
	fa.Init( tmp.x, tmp.y, tmp.z );
}

#endif