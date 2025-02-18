//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FX_INTERPVALUE_H
#define FX_INTERPVALUE_H
#ifdef _WIN32
#pragma once
#endif

// Types of supported interpolation
enum InterpType_t
{
	INTERP_LINEAR = 0,
	INTERP_SPLINE,
};

class CInterpolatedValue 
{
public:
			CInterpolatedValue( void );
			CInterpolatedValue( float startTime, float endTime, float startValue, float endValue, InterpType_t type );

	void	SetTime( float start, float end );
	void	SetRange( float start, float end );
	void	SetType( InterpType_t type );
	
	// Set the value with no range
	void SetAbsolute( float value );

	// Set the value with range and time supplied
	void Init( float startValue, float endValue, float dt, InterpType_t type = INTERP_LINEAR );

	// Start from the current value and move towards the end value
	void InitFromCurrent( float endValue, float dt, InterpType_t type = INTERP_LINEAR );

	// Find our interpolated value at the given point in time
	float Interp( float curTime );

private:

	float	m_flStartTime;
	float	m_flEndTime;
	float	m_flStartValue;
	float	m_flEndValue;

	int		m_nInterpType;
};

#endif // FX_INTERPVALUE_H
