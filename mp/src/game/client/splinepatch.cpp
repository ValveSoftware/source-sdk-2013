//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A little helper class that computes a spline patch
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "splinepatch.h"

#include "mathlib/vmatrix.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Catmull rom blend weights
//-----------------------------------------------------------------------------

static VMatrix s_CatmullRom( -0.5,  1.5, -1.5,  0.5,
							    1, -2.5,    2, -0.5,
							 -0.5,    0,  0.5,    0,
							    0,    1,    0,    0 );

//-----------------------------------------------------------------------------
// The last argument represents the number of float channels in addition to position
//-----------------------------------------------------------------------------

CSplinePatch::CSplinePatch( ) : m_ChannelCount(0),
	m_Width(0), m_Height(0), m_ppPositions(0), m_LinearFactor(1.0f)
{
}

CSplinePatch::~CSplinePatch()
{
}

//-----------------------------------------------------------------------------
// Initialize the spline patch
//-----------------------------------------------------------------------------

void CSplinePatch::Init( int w, int h, int extraChannels )
{
	assert( extraChannels < MAX_CHANNELS );
	m_ChannelCount = extraChannels;
	m_Width = w;
	m_Height = h;
	m_LinearFactor = 1.0f;
}


//-----------------------------------------------------------------------------
// 0 = linear, 1 = spliney!
//-----------------------------------------------------------------------------

void CSplinePatch::SetLinearBlend( float factor )
{
	m_LinearFactor = factor;
}


//-----------------------------------------------------------------------------
// Hooks the patch up to externally controlled data...
//-----------------------------------------------------------------------------

void CSplinePatch::SetControlPositions( Vector const** pPositions )
{
	m_ppPositions = pPositions;
}

void CSplinePatch::SetChannelData( int channel, float* pChannel )
{
	m_pChannel[channel] = pChannel;
}

static inline void ComputeIndex( int i, int maxval, int* idx )
{
	if (i == 0)
	{
		idx[0] = 0; idx[1] = 0; idx[2] = 1; 
		idx[3] = (maxval > 2) ? 2 : 1;
	}
	else
	{
		idx[0] = i-1; idx[1] = i;
		if (i >= maxval - 1)
		{
			idx[2] = i; idx[3] = i;
		}
		else
		{
			idx[2] = i+1;
			if (i >= maxval - 2)
				idx[3] = i+1;
			else
				idx[3]=  i+2;
		}
	}
}

//-----------------------------------------------------------------------------
// Computes indices of the samples to read for this interpolation
//-----------------------------------------------------------------------------

void CSplinePatch::ComputeIndices( )
{
	int s[4];
	int t[4];

	ComputeIndex( m_is, m_Width, s );
	ComputeIndex( m_it, m_Height, t );

	int base = t[0] * m_Width;
	m_SampleIndices[0][0] = base + s[0];
	m_SampleIndices[1][0] = base + s[1];
	m_SampleIndices[2][0] = base + s[2];
	m_SampleIndices[3][0] = base + s[3];

	base = t[1] * m_Width;
	m_SampleIndices[0][1] = base + s[0];
	m_SampleIndices[1][1] = base + s[1];
	m_SampleIndices[2][1] = base + s[2];
	m_SampleIndices[3][1] = base + s[3];

	base = t[2] * m_Width;
	m_SampleIndices[0][2] = base + s[0];
	m_SampleIndices[1][2] = base + s[1];
	m_SampleIndices[2][2] = base + s[2];
	m_SampleIndices[3][2] = base + s[3];

	base = t[3] * m_Width;
	m_SampleIndices[0][3] = base + s[0];
	m_SampleIndices[1][3] = base + s[1];
	m_SampleIndices[2][3] = base + s[2];
	m_SampleIndices[3][3] = base + s[3];
}

//-----------------------------------------------------------------------------
// Call this before querying the patch for data at (s,t)
//-----------------------------------------------------------------------------

void CSplinePatch::SetupPatchQuery( float s, float t )
{
	m_is = (int)s;
	m_it = (int)t;

	if( m_is >= m_Width )
	{
		m_is = m_Width - 1;
		m_fs = 1.0f;
	}
	else
	{
		m_fs = s - m_is;
	}

	if( m_it >= m_Height )
	{
		m_it = m_Height - 1;
		m_ft = 1.0f;
	}
	else
	{
		m_ft = t - m_it;
	}

	ComputeIndices( );

	// The patch equation is:
	// px = S * M * Gx * M^T * T^T 
	// py = S * M * Gy * M^T * T^T 
	// pz = S * M * Gz * M^T * T^T 
	// where S = [s^3 s^2 s 1], T = [t^3 t^2 t 1]
	// M is the patch type matrix, in my case I'm using a catmull-rom
	// G is the array of control points. rows have constant t

	// We're gonna cache off S * M and M^T * T^T...
	Vector4D svec, tvec;
 	float fs2 = m_fs * m_fs;
	svec[0] = fs2 * m_fs; svec[1] = fs2; svec[2] = m_fs; svec[3] = 1.0f;
	float ft2 = m_ft * m_ft;
	tvec[0] = ft2 * m_ft; tvec[1] = ft2; tvec[2] = m_ft; tvec[3] = 1.0f;

	// This sets up the catmull rom matrix based on the blend factor!!
	// we can go from linear to curvy!
	s_CatmullRom.Init(  -0.5 * m_LinearFactor,  1.5 * m_LinearFactor, -1.5 * m_LinearFactor,  0.5 * m_LinearFactor,
						       m_LinearFactor, -2.5 * m_LinearFactor,    2 * m_LinearFactor, -0.5 * m_LinearFactor,
						-0.5 * m_LinearFactor,   -1 + m_LinearFactor,    1 - 0.5 * m_LinearFactor,    0,
											0,					   1,					  0,    0 );
	Vector4DMultiplyTranspose( s_CatmullRom, svec, m_SVec );
	Vector4DMultiplyTranspose( s_CatmullRom, tvec, m_TVec );
}

//-----------------------------------------------------------------------------
// Gets the point and normal at (i,j) specified above
//-----------------------------------------------------------------------------

void CSplinePatch::GetPointAndNormal( Vector& position, Vector& normal ) const
{
	// The patch equation is:
	// px = S * M * Gx * M^T * T^T 
	// py = S * M * Gy * M^T * T^T 
	// pz = S * M * Gz * M^T * T^T 
	// where S = [s^3 s^2 s 1], T = [t^3 t^2 t 1]
	// M is the patch type matrix, in my case I'm using a catmull-rom
	// G is the array of control points. rows have constant t

	VMatrix controlPointsX, controlPointsY, controlPointsZ;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int idx = m_SampleIndices[i][j];
			controlPointsX[i][j] = m_ppPositions[ idx ]->x;
			controlPointsY[i][j] = m_ppPositions[ idx ]->y;
			controlPointsZ[i][j] = m_ppPositions[ idx ]->z;
		}
	}

	Vector4D tmp;

	Vector4DMultiply( controlPointsX, m_TVec, tmp );
	position[0] = DotProduct4D( tmp, m_SVec );
	Vector4DMultiply( controlPointsY, m_TVec, tmp );
	position[1] = DotProduct4D( tmp, m_SVec );
	Vector4DMultiply( controlPointsZ, m_TVec, tmp );
	position[2] = DotProduct4D( tmp, m_SVec );

	// Normal computation

	float fs2 = m_fs * m_fs;
	float ft2 = m_ft * m_ft;
	Vector4D dsvec( 3.0f * fs2, 2.0f * m_fs, 1.0f, 0.0f );
	Vector4D dtvec( 3.0f * ft2, 2.0f * m_ft, 1.0f, 0.0f );

	Vector4DMultiplyTranspose( s_CatmullRom, dsvec, dsvec );
	Vector4DMultiplyTranspose( s_CatmullRom, dtvec, dtvec );

	Vector ds, dt;

	Vector4DMultiply( controlPointsX, m_TVec, tmp );
	ds[0] = DotProduct4D( tmp, dsvec );
	Vector4DMultiply( controlPointsY, m_TVec, tmp );
	ds[1] = DotProduct4D( tmp, dsvec );
	Vector4DMultiply( controlPointsZ, m_TVec, tmp );
	ds[2] = DotProduct4D( tmp, dsvec );

	Vector4DMultiply( controlPointsX, dtvec, tmp );
	dt[0] = DotProduct4D( tmp, m_SVec );
	Vector4DMultiply( controlPointsY, dtvec, tmp );
	dt[1] = DotProduct4D( tmp, m_SVec );
	Vector4DMultiply( controlPointsZ, dtvec, tmp );
	dt[2] = DotProduct4D( tmp, m_SVec );

	CrossProduct( ds, dt, normal );
	VectorNormalize( normal );
}

//-----------------------------------------------------------------------------
// Gets at other channels
//-----------------------------------------------------------------------------

float CSplinePatch::GetChannel( int channel ) const
{
	// The patch equation is:
	// px = S * M * Gx * M^T * T^T 
	// py = S * M * Gy * M^T * T^T 
	// pz = S * M * Gz * M^T * T^T 
	// where S = [s^3 s^2 s 1], T = [t^3 t^2 t 1]
	// M is the patch type matrix, in my case I'm using a catmull-rom
	// G is the array of control points. rows have constant t

	assert( m_pChannel[channel] );

	VMatrix controlPoints;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			controlPoints[i][j] = m_pChannel[channel][ m_SampleIndices[i][j] ];
		}
	}

	Vector4D tmp;
	Vector4DMultiply( controlPoints, m_TVec, tmp );
	return DotProduct4D( tmp, m_SVec );
}
