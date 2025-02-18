//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COUNTDOWN_ANNOUNCER_H
#define COUNTDOWN_ANNOUNCER_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "tf/tf_gamerules.h"

//=============================================================================

class CCountdownAnnouncer 
{
public:
	struct TimeSounds {
		const char *at60;
		const char *at30;
		const char *at10;
		const char *at5;
		const char *at4;
		const char *at3;
		const char *at2;
		const char *at1;
	};

	CCountdownAnnouncer(const TimeSounds *pTimeSounds) 
		: m_state( Disabled )
		, m_pTimeSounds( pTimeSounds )
		, m_fTimeRemain( 0 )
		, m_iPrevSecondsRemain( 0 )
	{}

	void Disable() { m_state = Disabled; }

	void Start( int durationSec ) 
	{
		if ( m_state == Running )
			return;
		m_state = Running;
		m_fTimeRemain = (float) durationSec;
		m_iPrevSecondsRemain = 1 + (int) m_fTimeRemain;
		Tick( 0 );
	}

	// returns true once when time expires
	bool Tick( float delta ) 
	{
		assert( delta >= 0 );
		switch ( m_state )
		{
		case Disabled:
		case Expired:
			break;
			
		case Running: 
			{
				m_fTimeRemain = MAX( 0, m_fTimeRemain - delta );
				const int iSecondsRemain = Ceil2Int( m_fTimeRemain );
				if ( iSecondsRemain == m_iPrevSecondsRemain )
					break;
				m_iPrevSecondsRemain = iSecondsRemain;
				switch( iSecondsRemain ) 
				{
				case 60: BroadcastSound( m_pTimeSounds->at60 ); break;
				case 30: BroadcastSound( m_pTimeSounds->at30 ); break;
				case 10: BroadcastSound( m_pTimeSounds->at10 ); break;
				case 5: BroadcastSound( m_pTimeSounds->at5 ); break;
				case 4: BroadcastSound( m_pTimeSounds->at4 ); break;
				case 3: BroadcastSound( m_pTimeSounds->at3 ); break;
				case 2: BroadcastSound( m_pTimeSounds->at2 ); break;
				case 1: BroadcastSound( m_pTimeSounds->at1 ); break;
				default: break;
				}

				if ( iSecondsRemain == 0 )
				{
					Disable();
					return true;
				}
				break;
			}
		}
		return false;
	}

	bool IsDisabled() const { return m_state == Disabled; }

private:
	void BroadcastSound( const char* name ) 
	{ 
		if ( name && *name )
			TFGameRules()->BroadcastSound( 255, name );
	}

	enum State { Disabled, Running, Expired };
	State m_state;
	const TimeSounds *m_pTimeSounds;
	float m_fTimeRemain;
	int m_iPrevSecondsRemain;
};

#endif // COUNTDOWN_ANNOUNCER_H  
