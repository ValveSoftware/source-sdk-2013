//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Provide a class (SSE/SIMD only) holding a 2d matrix of class FourVectors,
// for high speed processing in tools.
//
// $NoKeywords: $
//
//=============================================================================//



#include "basetypes.h"
#include "mathlib/mathlib.h"
#include "mathlib/simdvectormatrix.h"
#include "mathlib/ssemath.h"
#include "tier0/dbg.h"

void CSIMDVectorMatrix::CreateFromRGBA_FloatImageData(int srcwidth, int srcheight,
													  float const *srcdata )
{
	Assert( srcwidth && srcheight && srcdata );
	SetSize( srcwidth, srcheight );

	FourVectors *p_write_ptr=m_pData;
	int n_vectors_per_source_line=(srcwidth >> 2);
	int ntrailing_pixels_per_source_line=(srcwidth & 3);
	for(int y=0;y<srcheight;y++)
	{
		float const *data_in=srcdata;
		float *data_out=reinterpret_cast<float *>( p_write_ptr );
		// copy full input blocks
		for(int x=0;x<n_vectors_per_source_line;x++)
		{
			for(int c=0;c<3;c++)
			{
				data_out[0]=data_in[c];					// x0
				data_out[1]=data_in[4+c];				// x1
				data_out[2]=data_in[8+c];				// x2
				data_out[3]=data_in[12+c];				// x3
				data_out+=4;
			}
			data_in += 16;
		}
		// now, copy trailing data and pad with copies
		if (ntrailing_pixels_per_source_line )
		{
			for(int c=0;c<3;c++)
			{
				for(int cp=0;cp<4; cp++)
				{
					int real_cp=min( cp, ntrailing_pixels_per_source_line-1 );
					data_out[4*c+cp]= data_in[c+4*real_cp];
				}
			}
		}
		// advance ptrs to next line
		p_write_ptr += m_nPaddedWidth;
		srcdata += 4 * srcwidth;
	}
}

void CSIMDVectorMatrix::RaiseToPower( float power )
{
	int nv=NVectors();
	if ( nv )
	{
		int fixed_point_exp=(int) ( 4.0*power );
		FourVectors *src=m_pData;
		do
		{
			src->x=Pow_FixedPoint_Exponent_SIMD( src->x, fixed_point_exp );
			src->y=Pow_FixedPoint_Exponent_SIMD( src->y, fixed_point_exp );
			src->z=Pow_FixedPoint_Exponent_SIMD( src->z, fixed_point_exp );
			src++;
		} while (--nv);
	}
}

CSIMDVectorMatrix & CSIMDVectorMatrix::operator+=( CSIMDVectorMatrix const &src )
{
	Assert( m_nWidth == src.m_nWidth );
	Assert( m_nHeight == src.m_nHeight );
	int nv=NVectors();
	if ( nv )
	{
		FourVectors *srcv=src.m_pData;
		FourVectors *destv=m_pData;
		do													// !! speed !! inline more iters
		{
			*( destv++ ) += *( srcv++ );
		} while ( --nv );
	}
	return *this;
}

CSIMDVectorMatrix & CSIMDVectorMatrix::operator*=( Vector const &src )
{
	int nv=NVectors();
	if ( nv )
	{
		FourVectors scalevalue;
		scalevalue.DuplicateVector( src );
		FourVectors *destv=m_pData;
		do													// !! speed !! inline more iters
		{
			destv->VProduct( scalevalue );
			destv++;
		} while ( --nv );
	}
	return *this;
}

