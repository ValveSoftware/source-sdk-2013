//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Provide a class (SSE/SIMD only) holding a 2d matrix of class FourVectors,
// for high speed processing in tools.
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef SIMDVECTORMATRIX_H
#define SIMDVECTORMATRIX_H

#ifdef _WIN32
#pragma once
#endif


#include <string.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/utlsoacontainer.h"
#include "mathlib/ssemath.h"

class CSIMDVectorMatrix
{
public:
	int m_nWidth;											// in actual vectors
	int m_nHeight;

	int m_nPaddedWidth;										// # of 4x wide elements

	FourVectors *m_pData;

protected:
	void Init( void )
	{
		m_pData = NULL;
		m_nWidth = 0;
		m_nHeight = 0;
		m_nPaddedWidth = 0;
	}

	int NVectors( void ) const
	{
		return m_nHeight * m_nPaddedWidth;
	}

public:
	// constructors and destructors
	CSIMDVectorMatrix( void )
	{
		Init();
	}

	~CSIMDVectorMatrix( void )
	{
		if ( m_pData )
			delete[] m_pData;
	}

	// set up storage and fields for m x n matrix. destroys old data
	void SetSize( int width, int height )
	{
		if ( ( ! m_pData ) || ( width != m_nWidth ) || ( height != m_nHeight ) )
		{
			if ( m_pData )
				delete[] m_pData;
			
			m_nWidth = width;
			m_nHeight = height;
			
			m_nPaddedWidth = ( m_nWidth + 3) >> 2;
			m_pData = NULL;
			if ( width && height )
				m_pData = new FourVectors[ m_nPaddedWidth * m_nHeight ];
		}
	}

	CSIMDVectorMatrix( int width, int height )
	{
		Init();
		SetSize( width, height );
	}

	CSIMDVectorMatrix &operator=( CSIMDVectorMatrix const &src )
	{
		SetSize( src.m_nWidth, src.m_nHeight );
		if ( m_pData )
			memcpy( m_pData, src.m_pData, m_nHeight*m_nPaddedWidth*sizeof(m_pData[0]) ); 
		return *this;
	}

	CSIMDVectorMatrix &operator+=( CSIMDVectorMatrix const &src );

	CSIMDVectorMatrix &operator*=( Vector const &src );

	// create from an RGBA float bitmap. alpha ignored.
	void CreateFromRGBA_FloatImageData(int srcwidth, int srcheight, float const *srcdata );

	// create from 3 fields in a csoa
	void CreateFromCSOAAttributes( CSOAContainer const *pSrc,
								   int nAttrIdx0, int nAttrIdx1, int nAttrIdx2 );

	// Element access. If you are calling this a lot, you don't want to use this class, because
	// you're not getting the sse advantage
	Vector Element(int x, int y) const
	{
		Assert( m_pData );
		Assert( x < m_nWidth );
		Assert( y < m_nHeight );
		Vector ret;
		FourVectors const *pData=m_pData+y*m_nPaddedWidth+(x >> 2);

		int xo=(x & 3);
		ret.x=pData->X( xo );
		ret.y=pData->Y( xo );
		ret.z=pData->Z( xo );
		return ret;
	}

	//addressing the individual fourvectors elements
	FourVectors &CompoundElement(int x, int y)
	{
		Assert( m_pData );
		Assert( y < m_nHeight );
		Assert( x < m_nPaddedWidth );
		return m_pData[x + m_nPaddedWidth*y ];
	}

	// math operations on the whole image
	void Clear( void )
	{
		Assert( m_pData );
		memset( m_pData, 0, m_nHeight*m_nPaddedWidth*sizeof(m_pData[0]) );
	}

	void RaiseToPower( float power );
};



#endif
