//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined ( DLIGHTH )
#define DLIGHTH
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

//-----------------------------------------------------------------------------
// Dynamic light structure
//-----------------------------------------------------------------------------

enum
{
	DLIGHT_NO_WORLD_ILLUMINATION = 0x1,
	DLIGHT_NO_MODEL_ILLUMINATION = 0x2,

	// NOTE: These two features are used to dynamically tweak the alpha on displacements
	// which is a special effect for selecting which texture to use. If 
	// we ever change how alpha is stored for displacements, we'll have to kill this feature
	DLIGHT_ADD_DISPLACEMENT_ALPHA = 0x4,
	DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA = 0x8,
	DLIGHT_DISPLACEMENT_MASK = (DLIGHT_ADD_DISPLACEMENT_ALPHA | DLIGHT_SUBTRACT_DISPLACEMENT_ALPHA),
};

// This is the lighting value that is used to determine when something can be 
// culle from lighting because it is close enough to black to be virtually black.
//#define MIN_LIGHTING_VALUE	(1.0f/256.0f)

// This is the broken value of MIN_LIGHTING_VALUE that we have to take into consideration
// to make sure that the lighting for dlights look the same as they did in HL2.
// We'll use the real MIN_LIGHTING_VALUE above to calculate larger radii for dynamic
// light sources.
//#define HL2_BROKEN_MIN_LIGHTING_VALUE (20.0f/256.0f)

struct dlight_t
{
	int		flags;
	Vector	origin;
	float	radius;
	ColorRGBExp32	color;		// Light color with exponent
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
	int		style;				// lightstyle

	// For spotlights. Use m_OuterAngle == 0 for point lights
	Vector	m_Direction;		// center of the light cone
	float	m_InnerAngle;
	float	m_OuterAngle;

	// see comments above about HL2_BROKEN_MIN_LIGHTING_VALUE and MIN_LIGHTING_VALUE
	// THIS SHOULD ONLY GET CALLED FROM THE ENGINE
	float GetRadius() const
	{
//		return FastSqrt( radius * radius * ( HL2_BROKEN_MIN_LIGHTING_VALUE / MIN_LIGHTING_VALUE ) );
		return radius;
	}

	// see comments above about HL2_BROKEN_MIN_LIGHTING_VALUE and MIN_LIGHTING_VALUE
	// THIS SHOULD ONLY GET CALLED FROM THE ENGINE
	float GetRadiusSquared() const
	{
//		return radius * radius * ( HL2_BROKEN_MIN_LIGHTING_VALUE / MIN_LIGHTING_VALUE );
		return radius * radius;
	}

	// THIS SHOULD ONLY GET CALLED FROM THE ENGINE
	float IsRadiusGreaterThanZero() const
	{
		// don't bother calculating the new radius if you just want to know if it is greater than zero.
		return radius > 0.0f;
	}
};

#endif
