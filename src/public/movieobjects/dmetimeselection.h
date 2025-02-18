//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMETIMESELECTION_H
#define DMETIMESELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "movieobjects/timeutils.h"
#include "movieobjects/dmetimeselectiontimes.h"

enum RecordingState_t;

class CDmeTimeSelection : public CDmElement
{
	DEFINE_ELEMENT( CDmeTimeSelection, CDmElement );

public:
	bool				IsEnabled() const;
	void				SetEnabled( bool state );

	bool				IsRelative() const;
	void				SetRelative( DmeTime_t time, bool state );

	DmeTime_t			GetAbsFalloff( DmeTime_t time, int side );
	DmeTime_t			GetAbsHold( DmeTime_t time, int side );

	DmeTime_t			GetRelativeFalloff( DmeTime_t time, int side );
	DmeTime_t			GetRelativeHold( DmeTime_t time, int side );

	void				SetAbsFalloff( DmeTime_t time, int side, DmeTime_t absfallofftime );
	void				SetAbsHold   ( DmeTime_t time, int side, DmeTime_t absholdtime );

	int					GetFalloffInterpolatorType( int side );
	void				SetFalloffInterpolatorType( int side, int interpolatorType );

	void				GetAlphaForTime( DmeTime_t t, DmeTime_t curtime, byte &alpha );
	float				GetAmountForTime( DmeTime_t t, DmeTime_t curtime );
	float				AdjustFactorForInterpolatorType( float factor, int side );

	void				CopyFrom( const CDmeTimeSelection &src );
	
	void				GetCurrent( DmeTime_t pTimes[TS_TIME_COUNT] );
	void				SetCurrent( DmeTime_t* pTimes );

	float				GetThreshold();

	void				SetRecordingState( RecordingState_t state );
	RecordingState_t	GetRecordingState() const;

private:
	CDmeTimeSelection & operator =( const CDmeTimeSelection& src );

	void				ConvertToRelative( DmeTime_t time );
	void				ConvertToAbsolute( DmeTime_t time );

	CDmaVar< bool > m_bEnabled;
	CDmaVar< bool > m_bRelative;
	// These are all offsets from the "current" head position in seconds, or they are absolute times if not using relative mode
	CDmaVar< int > m_falloff[ 2 ];
	CDmaVar< int > m_hold[ 2 ];
	CDmaVar< int > m_nFalloffInterpolatorType[ 2 ];
	CDmaVar< float > m_threshold;
	CDmaVar< int > m_nRecordingState;
};

#endif // DMETIMESELECTION_H
