//========= Copyright Valve Corporation, All rights reserved. ============//
//
// $Header: $
// $NoKeywords: $
//
// Serialization buffer
//===========================================================================//

#pragma warning (disable : 4514)

#include "tier1/utlbufferutil.h"
#include "tier1/utlbuffer.h"
#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "mathlib/vector4d.h"
#include "mathlib/vmatrix.h"
#include "Color.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include "tier1/utlbinaryblock.h"
#include "tier1/utlstring.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
			    

//-----------------------------------------------------------------------------
// For serialization, set the delimiter rules
//-----------------------------------------------------------------------------
CUtlCharConversion *s_pConv = NULL;
const char *s_pUtlBufferUtilArrayDelim = NULL;
void SetSerializationDelimiter( CUtlCharConversion *pConv )
{
	s_pConv = pConv;
}

void SetSerializationArrayDelimiter( const char *pDelimiter )
{
	s_pUtlBufferUtilArrayDelim = pDelimiter;
}


//-----------------------------------------------------------------------------
// Serialize a floating point number in text mode in a readably friendly fashion
//-----------------------------------------------------------------------------
static void SerializeFloat( CUtlBuffer &buf, float f )
{
	Assert( buf.IsText() );

	// FIXME: Print this in a way that we never lose precision
	char pTemp[256];
	int nLen = Q_snprintf( pTemp, sizeof(pTemp), "%.10f", f );
	while ( nLen > 0 && pTemp[nLen-1] == '0' )
	{
		--nLen;
		pTemp[nLen] = 0;
	}
	if ( nLen > 0 && pTemp[nLen-1] == '.' )
	{
		--nLen;
		pTemp[nLen] = 0;
	}
	buf.PutString( pTemp );
}

static void SerializeFloats( CUtlBuffer &buf, int nCount, const float *pFloats )
{
	for ( int i = 0; i < nCount; ++i )
	{
		SerializeFloat( buf, pFloats[i] );
		if ( i != nCount-1 )
		{
			buf.PutChar( ' ' );
		}
	}
}


//-----------------------------------------------------------------------------
// Serialization methods for basic types
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const bool &src )
{
	if ( buf.IsText() )
	{
		buf.Printf( "%d", src );
	}
	else
	{
		buf.PutChar( src );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, bool &dest )
{
	if ( buf.IsText() )
	{
		int nValue = 0;
		int nRetVal = buf.Scanf( "%d", &nValue );
		dest = ( nValue != 0 );
		return (nRetVal == 1) && buf.IsValid();
	}

	dest = ( buf.GetChar( ) != 0 );
	return buf.IsValid();
}


bool Serialize( CUtlBuffer &buf, const int &src )
{
	if ( buf.IsText() )
	{
		buf.Printf( "%d", src );
	}
	else
	{
		buf.PutInt( src );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, int &dest )
{
	if ( buf.IsText() )
	{
		int nRetVal = buf.Scanf( "%d", &dest );
		return (nRetVal == 1) && buf.IsValid();
	}

	dest = buf.GetInt( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const float &src )
{
	if ( buf.IsText() )
	{
		SerializeFloat( buf, src );
	}
	else
	{
		buf.PutFloat( src );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, float &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f", &dest );
		return (nRetVal == 1) && buf.IsValid();
	}

	dest = buf.GetFloat( );
	return buf.IsValid();
}


//-----------------------------------------------------------------------------
// Attribute types related to vector math
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const Vector2D &src )
{
	if ( buf.IsText() )
	{
		SerializeFloats( buf, 2, src.Base() );
	}
	else
	{
		buf.PutFloat( src.x );
		buf.PutFloat( src.y );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, Vector2D &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f %f", &dest.x, &dest.y );
		return (nRetVal == 2) && buf.IsValid();
	}

	dest.x = buf.GetFloat( );
	dest.y = buf.GetFloat( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const Vector &src )
{
	if ( buf.IsText() )
	{
		SerializeFloats( buf, 3, src.Base() );
	}
	else
	{
		buf.PutFloat( src.x );
		buf.PutFloat( src.y );
		buf.PutFloat( src.z );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, Vector &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f %f %f", &dest.x, &dest.y, &dest.z );
		return (nRetVal == 3) && buf.IsValid();
	}

	dest.x = buf.GetFloat( );
	dest.y = buf.GetFloat( );
	dest.z = buf.GetFloat( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const Vector4D &src )
{
	if ( buf.IsText() )
	{
		SerializeFloats( buf, 4, src.Base() );
	}
	else
	{
		buf.PutFloat( src.x );
		buf.PutFloat( src.y );
		buf.PutFloat( src.z );
		buf.PutFloat( src.w );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, Vector4D &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f %f %f %f", &dest.x, &dest.y, &dest.z, &dest.w );
		return (nRetVal == 4) && buf.IsValid();
	}

	dest.x = buf.GetFloat( );
	dest.y = buf.GetFloat( );
	dest.z = buf.GetFloat( );
	dest.w = buf.GetFloat( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const QAngle &src )
{
	if ( buf.IsText() )
	{
		SerializeFloats( buf, 3, src.Base() );
	}
	else
	{
		buf.PutFloat( src.x );
		buf.PutFloat( src.y );
		buf.PutFloat( src.z );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, QAngle &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f %f %f", &dest.x, &dest.y, &dest.z );
		return (nRetVal == 3) && buf.IsValid();
	}

	dest.x = buf.GetFloat( );
	dest.y = buf.GetFloat( );
	dest.z = buf.GetFloat( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const Quaternion &src )
{
	if ( buf.IsText() )
	{
		SerializeFloats( buf, 4, &src.x );
	}
	else
	{
		buf.PutFloat( src.x );
		buf.PutFloat( src.y );
		buf.PutFloat( src.z );
		buf.PutFloat( src.w );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, Quaternion &dest )
{
	if ( buf.IsText() )
	{
		// FIXME: Print this in a way that we never lose precision
		int nRetVal = buf.Scanf( "%f %f %f %f", &dest.x, &dest.y, &dest.z, &dest.w );
		return (nRetVal == 4) && buf.IsValid();
	}

	dest.x = buf.GetFloat( );
	dest.y = buf.GetFloat( );
	dest.z = buf.GetFloat( );
	dest.w = buf.GetFloat( );
	return buf.IsValid();
}

bool Serialize( CUtlBuffer &buf, const VMatrix &src )
{
	if ( buf.IsText() )
	{
		buf.Printf( "\n" );
		SerializeFloats( buf, 4, src[0] );
		buf.Printf( "\n" );
		SerializeFloats( buf, 4, src[1] );
		buf.Printf( "\n" );
		SerializeFloats( buf, 4, src[2] );
		buf.Printf( "\n" );
		SerializeFloats( buf, 4, src[3] );
		buf.Printf( "\n" );
	}
	else
	{
		buf.Put( &src, sizeof(VMatrix) );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, VMatrix &dest )
{
	if ( !buf.IsValid() )
		return false;

	if ( buf.IsText() )
	{
		int nRetVal = buf.Scanf( "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
			&dest[ 0 ][ 0 ], &dest[ 0 ][ 1 ], &dest[ 0 ][ 2 ], &dest[ 0 ][ 3 ],
			&dest[ 1 ][ 0 ], &dest[ 1 ][ 1 ], &dest[ 1 ][ 2 ], &dest[ 1 ][ 3 ],
			&dest[ 2 ][ 0 ], &dest[ 2 ][ 1 ], &dest[ 2 ][ 2 ], &dest[ 2 ][ 3 ],
			&dest[ 3 ][ 0 ], &dest[ 3 ][ 1 ], &dest[ 3 ][ 2 ], &dest[ 3 ][ 3 ] );
		return (nRetVal == 16);
	}

	buf.Get( &dest, sizeof(VMatrix) );
	return true;
}


//-----------------------------------------------------------------------------
// Color attribute
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const Color &src )
{
	if ( buf.IsText() )
	{
		buf.Printf( "%d %d %d %d", src[0], src[1], src[2], src[3] );
	}
	else
	{
		buf.PutUnsignedChar( src[0] );
		buf.PutUnsignedChar( src[1] );
		buf.PutUnsignedChar( src[2] );
		buf.PutUnsignedChar( src[3] );
	}
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, Color &dest )
{
	if ( buf.IsText() )
	{
		int r = 0, g = 0, b = 0, a = 255;
		int nRetVal = buf.Scanf( "%d %d %d %d", &r, &g, &b, &a );
		dest.SetColor( r, g, b, a );
		return (nRetVal == 4) && buf.IsValid();
	}

	dest[0] = buf.GetUnsignedChar( );
	dest[1] = buf.GetUnsignedChar( );
	dest[2] = buf.GetUnsignedChar( );
	dest[3] = buf.GetUnsignedChar( );
	return buf.IsValid();
}

/*
//-----------------------------------------------------------------------------
// Object ID attribute
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const DmObjectId_t &src )
{
	return g_pDataModel->Serialize( buf, src );
}

bool Unserialize( CUtlBuffer &buf, DmObjectId_t &dest )
{
	return g_pDataModel->Unserialize( buf, &dest );
}
*/

//-----------------------------------------------------------------------------
// Binary buffer attribute
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const CUtlBinaryBlock &src )
{
	int nLength = src.Length();
	if ( !buf.IsText() )
	{
		buf.PutInt( nLength );
		if ( nLength != 0 )
		{
			buf.Put( src.Get(), nLength );
		}
		return buf.IsValid();
	}

	// Writes out uuencoded binaries
	for ( int i = 0; i < nLength; ++i )
	{
		if ( (i % 40) == 0 )
		{
			buf.PutChar( '\n' );
		}

		char b1 = src[i] & 0xF;
		char b2 = src[i] >> 4;

		char c1 = ( b1 <= 9 ) ? b1 + '0' : b1 - 10 + 'A';
		char c2 = ( b2 <= 9 ) ? b2 + '0' : b2 - 10 + 'A';

		buf.PutChar( c2 );
		buf.PutChar( c1 );
	}

	buf.PutChar( '\n' );
	return buf.IsValid();
}

static int CountBinaryBytes( CUtlBuffer &buf, int *pEndGet )
{
	// This counts the number of bytes in the uuencoded text
	int nStartGet = buf.TellGet();
	buf.EatWhiteSpace();
	*pEndGet = buf.TellGet();
	int nByteCount = 0;
	while ( buf.IsValid() )
	{
		char c1 = buf.GetChar();
		char c2 = buf.GetChar();

		bool bIsNum1 = ( c1 >= '0' ) && ( c1 <= '9' );
		bool bIsNum2 = ( c2 >= '0' ) && ( c2 <= '9' );

		bool bIsAlpha1 = (( c1 >= 'A' ) && ( c1 <= 'F' )) || (( c1 >= 'a' ) && ( c1 <= 'f' ));
		bool bIsAlpha2 = (( c2 >= 'A' ) && ( c2 <= 'F' )) || (( c2 >= 'a' ) && ( c2 <= 'f' ));

		if ( !(bIsNum1 || bIsAlpha1) || !(bIsNum2 || bIsAlpha2) )
			break;

		buf.EatWhiteSpace();
		*pEndGet = buf.TellGet();
		++nByteCount;
	}
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, nStartGet );
	return nByteCount;
}

inline static unsigned char HexCharToInt( int c1 )
{
	if (( c1 >= '0' ) && ( c1 <= '9' ))
		return c1 - '0';

	if (( c1 >= 'A' ) && ( c1 <= 'F' ))
		return 10 + c1 - 'A';

	if (( c1 >= 'a' ) && ( c1 <= 'f' ))
		return 10 + c1 - 'a';

	return 0xFF;
}

bool Unserialize( CUtlBuffer &buf, CUtlBinaryBlock &dest )
{
	if ( !buf.IsText() )
	{
		int nLen = buf.GetInt( );
		dest.SetLength( nLen );
		if ( dest.Length() != 0 )
		{
			buf.Get( dest.Get(), dest.Length() );
		}

		if ( nLen != dest.Length() )
		{
			buf.SeekGet( CUtlBuffer::SEEK_CURRENT, nLen - dest.Length() );
			return false;
		}

		return buf.IsValid();
	}

	int nEndGet;
	int nByteCount = CountBinaryBytes( buf, &nEndGet );
	if ( nByteCount < 0 )
		return false;

	buf.EatWhiteSpace();
	int nDest = 0;
	dest.SetLength( nByteCount );
	while( buf.TellGet() < nEndGet )
	{
		char c1 = buf.GetChar();
		char c2 = buf.GetChar();

		unsigned char b1 = HexCharToInt( c1 );
		unsigned char b2 = HexCharToInt( c2 );
		if ( b1 == 0xFF || b2 == 0xFF )
			return false;

		dest[ nDest++ ] = b2 | ( b1 << 4 );
		buf.EatWhiteSpace();
	}

	return true;
}


//-----------------------------------------------------------------------------
// String attribute
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const CUtlString &src )
{
	buf.PutDelimitedString( s_pConv, src.Get() );
	return buf.IsValid();
}

bool Unserialize( CUtlBuffer &buf, CUtlString &dest )
{
	int nLen = buf.PeekDelimitedStringLength( s_pConv );
	dest.SetLength( nLen - 1 );	// -1 because the length returned includes space for \0
	buf.GetDelimitedString( s_pConv, dest.GetForModify(), nLen );
	return buf.IsValid();
}




