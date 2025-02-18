//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Utilities for serialization/unserialization buffer
//=============================================================================//

#ifndef UTLBUFFERUTIL_H
#define UTLBUFFERUTIL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include "tier1/utlbuffer.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Vector2D;
class Vector;
class Vector4D;
class QAngle;
class Quaternion;
class VMatrix;
class Color;
class CUtlBinaryBlock;
class CUtlString;
class CUtlCharConversion;

	
//-----------------------------------------------------------------------------
// For string serialization, set the delimiter rules
//-----------------------------------------------------------------------------
void SetSerializationDelimiter( CUtlCharConversion *pConv );
void SetSerializationArrayDelimiter( const char *pDelimiter );


//-----------------------------------------------------------------------------
// Standard serialization methods for basic types
//-----------------------------------------------------------------------------
bool Serialize( CUtlBuffer &buf, const bool &src );
bool Unserialize( CUtlBuffer &buf, bool &dest );

bool Serialize( CUtlBuffer &buf, const int &src );
bool Unserialize( CUtlBuffer &buf, int &dest );

bool Serialize( CUtlBuffer &buf, const float &src );
bool Unserialize( CUtlBuffer &buf, float &dest );

bool Serialize( CUtlBuffer &buf, const Vector2D &src );
bool Unserialize( CUtlBuffer &buf, Vector2D &dest );

bool Serialize( CUtlBuffer &buf, const Vector &src );
bool Unserialize( CUtlBuffer &buf, Vector &dest );

bool Serialize( CUtlBuffer &buf, const Vector4D &src );
bool Unserialize( CUtlBuffer &buf, Vector4D &dest );

bool Serialize( CUtlBuffer &buf, const QAngle &src );
bool Unserialize( CUtlBuffer &buf, QAngle &dest );

bool Serialize( CUtlBuffer &buf, const Quaternion &src );
bool Unserialize( CUtlBuffer &buf, Quaternion &dest );

bool Serialize( CUtlBuffer &buf, const VMatrix &src );
bool Unserialize( CUtlBuffer &buf, VMatrix &dest );

bool Serialize( CUtlBuffer &buf, const Color &src );
bool Unserialize( CUtlBuffer &buf, Color &dest );

bool Serialize( CUtlBuffer &buf, const CUtlBinaryBlock &src );
bool Unserialize( CUtlBuffer &buf, CUtlBinaryBlock &dest );

bool Serialize( CUtlBuffer &buf, const CUtlString &src );
bool Unserialize( CUtlBuffer &buf, CUtlString &dest );


//-----------------------------------------------------------------------------
// You can use this to check if a type serializes on multiple lines
//-----------------------------------------------------------------------------
template< class T >
inline bool SerializesOnMultipleLines()
{
	return false;
}

template< >
inline bool SerializesOnMultipleLines<VMatrix>()
{
	return true;
}

template< >
inline bool SerializesOnMultipleLines<CUtlBinaryBlock>()
{
	return true;
}


//-----------------------------------------------------------------------------
// Vector serialization
//-----------------------------------------------------------------------------
template< class T >
bool Serialize( CUtlBuffer &buf, const CUtlVector<T> &src )
{
	extern const char *s_pUtlBufferUtilArrayDelim;

	int nCount = src.Count();

	if ( !buf.IsText() )
	{
		buf.PutInt( nCount );
		for ( int i = 0; i < nCount; ++i )
		{
			::Serialize( buf, src[i] );
		}
		return buf.IsValid();
	}

	if ( !SerializesOnMultipleLines<T>() )
	{
		buf.PutChar('\n');
		for ( int i = 0; i < nCount; ++i )
		{
			::Serialize( buf, src[i] );
			if ( s_pUtlBufferUtilArrayDelim && (i != nCount-1) )
			{
				buf.PutString( s_pUtlBufferUtilArrayDelim );
			}
			buf.PutChar('\n');
		}
	}
	else
	{
		for ( int i = 0; i < nCount; ++i )
		{
			::Serialize( buf, src[i] );
			if ( s_pUtlBufferUtilArrayDelim && (i != nCount-1) )
			{
				buf.PutString( s_pUtlBufferUtilArrayDelim );
			}
			buf.PutChar(' ');
		}
	}

	return buf.IsValid();
}

template< class T >
bool Unserialize( CUtlBuffer &buf, CUtlVector<T> &dest )
{
	dest.RemoveAll();

	MEM_ALLOC_CREDIT_FUNCTION();

	if ( !buf.IsText() )
	{
		int nCount = buf.GetInt();
		if ( nCount )
		{
			dest.EnsureCapacity( nCount );
			for ( int i = 0; i < nCount; ++i )
			{
				VerifyEquals( dest.AddToTail(), i );
				if ( !::Unserialize( buf, dest[i] ) )
					return false;
			}
		}
		return buf.IsValid();
	}

	while ( true )
	{
		buf.EatWhiteSpace();
		if ( !buf.IsValid() )
			break;

		int i = dest.AddToTail( );
		if ( ! ::Unserialize( buf, dest[i] ) )
			return false;
	}
	return true;
}


#endif // UTLBUFFERUTIL_H

