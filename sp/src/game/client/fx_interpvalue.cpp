//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "fx_interpvalue.h"

CInterpolatedValue::CInterpolatedValue( void ) :  m_flStartTime( 0.0f ), m_flEndTime( 0.0f ), m_flStartValue( 0.0f ), m_flEndValue( 0.0f ), m_nInterpType( INTERP_LINEAR ) 
{
}

CInterpolatedValue::CInterpolatedValue( float startTime, float endTime, float startValue, float endValue, InterpType_t type ) : 
	  m_flStartTime( startTime ), m_flEndTime( endTime ), m_flStartValue( startValue ), m_flEndValue( endValue ), m_nInterpType( type ) 
{
}

void CInterpolatedValue::SetTime( float start, float end ) 
{ 
	m_flStartTime = start; m_flEndTime = end; 
}

void CInterpolatedValue::SetRange( float start, float end ) 
{ 
	m_flStartValue = start; m_flEndValue = end; 
}

void CInterpolatedValue::SetType( InterpType_t type )
{ 
	m_nInterpType = type; 
}
	
// Set the value with no range
void CInterpolatedValue::SetAbsolute( float value )
{
	m_flStartValue = m_flEndValue = value;
	m_flStartTime = m_flEndTime = gpGlobals->curtime;
	m_nInterpType = INTERP_LINEAR;
}

// Set the value with range and time supplied
void CInterpolatedValue::Init( float startValue, float endValue, float dt, InterpType_t type /*= INTERP_LINEAR*/ )
{
	if ( dt <= 0.0f )
	{
		SetAbsolute( endValue );
		return;
	}

	SetTime( gpGlobals->curtime, gpGlobals->curtime + dt );
	SetRange( startValue, endValue );
	SetType( type );
}

// Start from the current value and move towards the end value
void CInterpolatedValue::InitFromCurrent( float endValue, float dt, InterpType_t type /*= INTERP_LINEAR*/ )
{
	Init( Interp( gpGlobals->curtime ), endValue, dt, type );
}

// Find our interpolated value at the given point in time
float CInterpolatedValue::Interp( float curTime )
{
	switch( m_nInterpType )
	{
	case INTERP_LINEAR:
		{
			if ( curTime >= m_flEndTime )
				return m_flEndValue;

			if ( curTime <= m_flStartTime )
				return m_flStartValue;

			return RemapVal( curTime, m_flStartTime, m_flEndTime, m_flStartValue, m_flEndValue );
		}

	case INTERP_SPLINE:
		{
			if ( curTime >= m_flEndTime )
				return m_flEndValue;

			if ( curTime <= m_flStartTime )
				return m_flStartValue;

			return SimpleSplineRemapVal( curTime, m_flStartTime, m_flEndTime, m_flStartValue, m_flEndValue );
		}
	}

	// NOTENOTE: You managed to pass in a bogus interpolation type!
	Assert(0);
	return -1.0f;
}
