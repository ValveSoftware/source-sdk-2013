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

#ifndef SPLINEPATCH_H
#define SPLINEPATCH_H
#pragma once

#include "mathlib/vector4d.h"

//-----------------------------------------------------------------------------
// Spline patch: 
//-----------------------------------------------------------------------------

class CSplinePatch
{
public:
	// The last argument represents the number of float channels in addition to position
	CSplinePatch( );
	~CSplinePatch();

	// Call this to initialize the patch
	void Init( int w, int h, int extraChannels );

	// 0 = linear, 1 = spliney!
	void SetLinearBlend( float factor );

	// Hooks the patch up to externally controlled data...
	void SetControlPositions( Vector const** pPositions );
	void SetChannelData( int channel, float* pChannel );

	// This interface isn't wonderful; it's limited by optimization issues...

	// Call this before querying the patch for data at (i,j)
	void SetupPatchQuery( float i, float j );

	// Gets the point and normal at (i,j) specified above
	void GetPointAndNormal( Vector& position, Vector& normal ) const;

	// Gets at other channels
	float GetChannel( int channel ) const;

	// Gets at the dimensions
	int	Width() const { return m_Width; }
	int Height() const { return m_Height; }

public:
	// The integer + float values for the patch query
	int		m_is, m_it;
	float	m_fs, m_ft;

private:
	enum
	{
		MAX_CHANNELS = 4
	};

	// no copy constructor
	CSplinePatch( const CSplinePatch& );

	// Computes indices of the samples to read for this interpolation
	void ComputeIndices( );

	// input data
	int m_Width;
	int m_Height;
	int m_ChannelCount;
	Vector const** m_ppPositions;
	float const* m_pChannel[MAX_CHANNELS];

	// temporary data used for a single patch query
	int		m_SampleIndices[4][4];
	Vector4D m_SVec;
	Vector4D m_TVec;

	float m_LinearFactor;
};

#endif // SPLINEPATCH_H
