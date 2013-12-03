//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef INTERPOLATORTYPES_H
#define INTERPOLATORTYPES_H
#ifdef _WIN32
#pragma once
#endif

class Quaternion;

enum
{
	INTERPOLATE_DEFAULT = 0,
	INTERPOLATE_CATMULL_ROM_NORMALIZEX,
	INTERPOLATE_EASE_IN,								
	INTERPOLATE_EASE_OUT,								
	INTERPOLATE_EASE_INOUT,			
	INTERPOLATE_BSPLINE,							
	INTERPOLATE_LINEAR_INTERP,				
	INTERPOLATE_KOCHANEK_BARTELS,			
	INTERPOLATE_KOCHANEK_BARTELS_EARLY,	
	INTERPOLATE_KOCHANEK_BARTELS_LATE,
	INTERPOLATE_SIMPLE_CUBIC,

	INTERPOLATE_CATMULL_ROM,
	INTERPOLATE_CATMULL_ROM_NORMALIZE,	
	INTERPOLATE_CATMULL_ROM_TANGENT,

	INTERPOLATE_EXPONENTIAL_DECAY,

	INTERPOLATE_HOLD,

	NUM_INTERPOLATE_TYPES,
};

#define MAKE_CURVE_TYPE( left, right )      ( ( right ) & 0xff ) | ( ( ( left ) & 0xff ) << 8 )

#define GET_RIGHT_CURVE(w)           ( ( w ) & 0xff )
#define GET_LEFT_CURVE(w)           ( ( ( w ) >> 8) & 0xff )

// Presets used by faceposer
enum
{
	CURVE_DEFAULT											= MAKE_CURVE_TYPE( INTERPOLATE_DEFAULT, INTERPOLATE_DEFAULT ),
	CURVE_CATMULL_ROM_TO_CATMULL_ROM						= MAKE_CURVE_TYPE( INTERPOLATE_CATMULL_ROM_NORMALIZEX , INTERPOLATE_CATMULL_ROM_NORMALIZEX ), // hotkey 1
	CURVE_EASE_IN_TO_EASE_OUT								= MAKE_CURVE_TYPE( INTERPOLATE_EASE_IN, INTERPOLATE_EASE_OUT ),							// hotkey 2
	CURVE_EASE_IN_TO_EASE_IN								= MAKE_CURVE_TYPE( INTERPOLATE_EASE_IN, INTERPOLATE_EASE_IN ),
	CURVE_EASE_OUT_TO_EASE_OUT								= MAKE_CURVE_TYPE( INTERPOLATE_EASE_OUT, INTERPOLATE_EASE_OUT ),
	CURVE_BSPLINE_TO_BSPLINE								= MAKE_CURVE_TYPE( INTERPOLATE_BSPLINE, INTERPOLATE_BSPLINE ),
	CURVE_LINEAR_INTERP_TO_LINEAR_INTERP					= MAKE_CURVE_TYPE( INTERPOLATE_LINEAR_INTERP, INTERPOLATE_LINEAR_INTERP ),
	CURVE_KOCHANEK_BARTELS_TO_KOCHANEK_BARTELS				= MAKE_CURVE_TYPE( INTERPOLATE_KOCHANEK_BARTELS, INTERPOLATE_KOCHANEK_BARTELS ),
	CURVE_KOCHANEK_BARTELS_EARLY_TO_KOCHANEK_BARTELS_EARLY	= MAKE_CURVE_TYPE( INTERPOLATE_KOCHANEK_BARTELS_EARLY, INTERPOLATE_KOCHANEK_BARTELS_EARLY ),
	CURVE_KOCHANEK_BARTELS_LATE_TO_KOCHANEK_BARTELS_LATE	= MAKE_CURVE_TYPE( INTERPOLATE_KOCHANEK_BARTELS_LATE, INTERPOLATE_KOCHANEK_BARTELS_LATE ),
	CURVE_SIMPLE_CUBIC_TO_SIMPLE_CUBIC						= MAKE_CURVE_TYPE( INTERPOLATE_SIMPLE_CUBIC, INTERPOLATE_SIMPLE_CUBIC ),
	CURVE_LINEAR_TO_HOLD									= MAKE_CURVE_TYPE( INTERPOLATE_LINEAR_INTERP, INTERPOLATE_HOLD ),
	CURVE_HOLD_TO_LINEAR									= MAKE_CURVE_TYPE( INTERPOLATE_HOLD, INTERPOLATE_LINEAR_INTERP ),
};

// Turn enum into string and vice versa
int			Interpolator_CurveTypeForName( const char *name );
const char	*Interpolator_NameForCurveType( int type, bool printname );
void		Interpolator_CurveInterpolatorsForType( int type, int& inbound, int& outbound );
int			Interpolator_CurveTypeForHotkey( int key );

int			Interpolator_InterpolatorForName( char const *name );
char const	*Interpolator_NameForInterpolator( int type, bool printname );

void		Interpolator_GetKochanekBartelsParams( int interpolatorType, float& tension, float& bias, float& continuity );

class Vector;
// Main spline interpolation function, assumes .x holds time and .y holds one dimensional value
void Interpolator_CurveInterpolate( int interpolationType,
	const Vector &vPre,
	const Vector &vStart,
	const Vector &vEnd,
	const Vector &vNext,
	float f,
	Vector &vOut );

// Main spline interpolation function for Vectors, doesn't assume time is in .x and doesn't do normalization
void Interpolator_CurveInterpolate_NonNormalized( int interpolationType,
	const Vector &vPre,
	const Vector &vStart,
	const Vector &vEnd,
	const Vector &vNext,
	float f,
	Vector &vOut );

// Main spline interpolation function for Vectors, doesn't assume time is in .x and doesn't do normalization
void Interpolator_CurveInterpolate_NonNormalized( int interpolationType,
												 const Quaternion &vPre,
												 const Quaternion &vStart,
												 const Quaternion &vEnd,
												 const Quaternion &vNext,
												 float f,
												 Quaternion &vOut );

#endif // INTERPOLATORTYPES_H
