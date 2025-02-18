//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef LERP_FUNCTIONS_H
#define LERP_FUNCTIONS_H
#ifdef _WIN32
#pragma once
#endif


template <class T>
inline T LoopingLerp( float flPercent, T flFrom, T flTo )
{
	T s = flTo * flPercent + flFrom * (1.0f - flPercent);
	return s;
}

template <>
inline float LoopingLerp( float flPercent, float flFrom, float flTo )
{
	if ( fabs( flTo - flFrom ) >= 0.5f )
	{
		if (flFrom < flTo)
			flFrom += 1.0f;
		else
			flTo += 1.0f;
	}

	float s = flTo * flPercent + flFrom * (1.0f - flPercent);

	s = s - (int)(s);
	if (s < 0.0f)
		s = s + 1.0f;

	return s;
}

template <class T>
inline T Lerp_Hermite( float t, const T& p0, const T& p1, const T& p2 )
{
	T d1 = p1 - p0;
	T d2 = p2 - p1;

	T output;
	float tSqr = t*t;
	float tCube = t*tSqr;

	output = p1 * (2*tCube-3*tSqr+1);
	output += p2 * (-2*tCube+3*tSqr);
	output += d1 * (tCube-2*tSqr+t);
	output += d2 * (tCube-tSqr);

	return output;
}


template <class T>
inline T Derivative_Hermite( float t, const T& p0, const T& p1, const T& p2 )
{
	T d1 = p1 - p0;
	T d2 = p2 - p1;

	T output;
	float tSqr = t*t;

	output = p1 * (6*tSqr - 6*t);
	output += p2 * (-6*tSqr + 6*t);
	output += d1 * (3*tSqr - 4*t + 1);
	output += d2 * (3*tSqr - 2*t);

	return output;
}


inline void Lerp_Clamp( int val )
{
}

inline void Lerp_Clamp( float val )
{
}

inline void Lerp_Clamp( const Vector &val )
{
}

inline void Lerp_Clamp( const QAngle &val )
{
}


// If we have a range checked var, then we can clamp to its limits.
template< class T, int minValue, int maxValue, int startValue >
inline void Lerp_Clamp( CRangeCheckedVar<T,minValue,maxValue,startValue> &val )
{
	val.Clamp();
}


template<> 
inline QAngle Lerp_Hermite<QAngle>( float t, const QAngle& p0, const QAngle& p1, const QAngle& p2 )
{
	// Can't do hermite with QAngles, get discontinuities, just do a regular interpolation
	return Lerp( t, p1, p2 );
}

template <class T>
inline T LoopingLerp_Hermite( float t, T p0, T p1, T p2  )
{
	return Lerp_Hermite( t, p0, p1, p2 );
}

template <>
inline float LoopingLerp_Hermite( float t, float p0, float p1, float p2 )
{
	if ( fabs( p1 - p0 ) > 0.5f )
	{
		if ( p0 < p1 )
			p0 += 1.0f;
		else
			p1 += 1.0f;
	}

	if ( fabs( p2 - p1 ) > 0.5f )
	{
		if ( p1 < p2 )
		{
			p1 += 1.0f;

			// see if we need to fix up p0
			// important for vars that are decreasing from p0->p1->p2 where
			// p1 is fixed up relative to p2, eg p0 = 0.2, p1 = 0.1, p2 = 0.9
			if ( abs( p1 - p0 ) > 0.5 )
			{
				if ( p0 < p1 )
					p0 += 1.0f;
				else
					p1 += 1.0f;
			}
		}
		else
		{
			p2 += 1.0f;
		}
	}
		
	float s = Lerp_Hermite( t, p0, p1, p2 );

	s = s - (int)(s);
	if (s < 0.0f)
	{
		s = s + 1.0f;
	}

	return s;
}


// NOTE: C_AnimationLayer has its own versions of these functions in animationlayer.h.


#endif // LERP_FUNCTIONS_H
