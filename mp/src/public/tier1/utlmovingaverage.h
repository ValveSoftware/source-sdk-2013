//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple moving average class
//
// $NoKeywords: $
//
// 
//=============================================================================//
#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/basetypes.h"

template<uint32 TBufferSize> class CUtlMovingAverage
{
public:
	CUtlMovingAverage() :
		m_nValuesPushed( 0 ),
		m_flTotal( 0.0f )
	{
	}

	void Reset()
	{
		m_nValuesPushed = 0;
		m_flTotal = 0.0f;
	}

	uint32 GetTotalValuesPushed() const 
	{ 
		return m_nValuesPushed; 
	}

	float GetAverage( )
	{
		uint n = MIN( TBufferSize, m_nValuesPushed );
		return n ? ( m_flTotal / static_cast<double>( n ) ) : 0.0f;
	}

	void GetAverageAndAbsRange( float *pflOutAverage, float *pflOutAbsRange, float *pflMinTime, float *pflMaxTime )
	{
		if ( m_nValuesPushed == 0 )
		{
			*pflOutAverage = 0;
			*pflOutAbsRange = 0;
			*pflMinTime = 0;
			*pflMaxTime = 0;
			return;
		}

		*pflOutAverage = GetAverage();

		const int nNumValues = MIN( m_nValuesPushed, TBufferSize );

		float flAbsRange = 0;
		float flMinTime = 9e+9;
		float flMaxTime = 0;

		for ( int i = 0; i < nNumValues; ++i )
		{
			float flDif = ( m_Buffer[i] - *pflOutAverage );
			flAbsRange = MAX( flAbsRange, abs( flDif ) );
			flMinTime = MIN( flMinTime, m_Buffer[i] );
			flMaxTime = MAX( flMaxTime, m_Buffer[i] );
		}

		*pflOutAbsRange = flAbsRange;
		*pflMinTime = flMinTime;
		*pflMaxTime = flMaxTime;
	}

	void PushValue( float v )
	{
		uint nIndex = m_nValuesPushed % TBufferSize;
		
		if ( m_nValuesPushed >= TBufferSize )
		{
			m_flTotal = MAX( m_flTotal - m_Buffer[nIndex], 0.0f );
		}
		m_flTotal += v;

		m_Buffer[nIndex] = v;
		m_nValuesPushed++;

		if ( UINT_MAX == m_nValuesPushed )
		{
			Reset();
		}
	}
		
private:
	float m_Buffer[TBufferSize];
	uint32 m_nValuesPushed;

	double m_flTotal;
};

#endif // MOVING_AVERAGE_H
