//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions for spherical geometry.
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef SPHERICAL_GEOMETRY_H
#define SPHERICAL_GEOMETRY_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <float.h>

// see http://mathworld.wolfram.com/SphericalTrigonometry.html

// return the spherical distance, in radians, between 2 points on the unit sphere.
FORCEINLINE float UnitSphereLineSegmentLength( Vector const &a, Vector const &b )
{
	// check unit length
	Assert( fabs( VectorLength( a ) - 1.0 ) < 1.0e-3 );
	Assert( fabs( VectorLength( b ) - 1.0 ) < 1.0e-3 );
	return acos( DotProduct( a, b ) );
}


// given 3 points on the unit sphere, return the spherical area (in radians) of the triangle they form.
// valid for "small" triangles.
FORCEINLINE float UnitSphereTriangleArea( Vector const &a, Vector const &b , Vector const &c )
{
	float flLengthA = UnitSphereLineSegmentLength( b, c );
	float flLengthB = UnitSphereLineSegmentLength( c, a );
	float flLengthC = UnitSphereLineSegmentLength( a, b );
	
	if ( ( flLengthA == 0. ) || ( flLengthB == 0. ) || ( flLengthC == 0. ) )
		return 0.;											// zero area triangle
			
	// now, find the 3 incribed angles for the triangle
	float flHalfSumLens = 0.5 * ( flLengthA + flLengthB + flLengthC );
	float flSinSums = sin( flHalfSumLens );
	float flSinSMinusA= sin( flHalfSumLens - flLengthA );
	float flSinSMinusB= sin( flHalfSumLens - flLengthB );
	float flSinSMinusC= sin( flHalfSumLens - flLengthC );
	
	float flTanAOver2 = sqrt ( ( flSinSMinusB * flSinSMinusC ) / ( flSinSums * flSinSMinusA ) );
	float flTanBOver2 = sqrt ( ( flSinSMinusA * flSinSMinusC ) / ( flSinSums * flSinSMinusB ) );
	float flTanCOver2 = sqrt ( ( flSinSMinusA * flSinSMinusB ) / ( flSinSums * flSinSMinusC ) );

	// Girards formula : area = sum of angles - pi.
	return 2.0 * ( atan( flTanAOver2 ) + atan( flTanBOver2 ) + atan( flTanCOver2 ) ) - M_PI;
}

// spherical harmonics-related functions. Best explanation at http://www.research.scea.com/gdc2003/spherical-harmonic-lighting.pdf

// Evaluate associated legendre polynomial P( l, m ) at flX, using recurrence relation
float AssociatedLegendrePolynomial( int nL, int nM, float flX );

// Evaluate order N spherical harmonic with spherical coordinates
// nL = band, 0..N
// nM = -nL .. nL
// theta = 0..M_PI
// phi = 0.. 2 * M_PHI
float SphericalHarmonic( int nL, int nM, float flTheta, float flPhi );

// evaluate spherical harmonic with normalized vector direction
float SphericalHarmonic( int nL, int nM, Vector const &vecDirection );


#endif // SPHERICAL_GEOMETRY_H
