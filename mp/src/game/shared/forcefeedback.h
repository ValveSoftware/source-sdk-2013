//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FORCEFEEDBACK_H
#define FORCEFEEDBACK_H
#ifdef _WIN32
#pragma once
#endif

enum
{
	FFMSG_START = 0,
	FFMSG_STOP,
	FFMSG_STOPALL,
	FFMSG_PAUSE,
	FFMSG_RESUME
};

typedef enum
{
	FORCE_FEEDBACK_SHOT_SINGLE,
	FORCE_FEEDBACK_SHOT_DOUBLE,

	FORCE_FEEDBACK_TAKEDAMAGE,

	FORCE_FEEDBACK_SCREENSHAKE,

	FORCE_FEEDBACK_SKIDDING,
	FORCE_FEEDBACK_BREAKING,

	NUM_FORCE_FEEDBACK_PRESETS
} FORCEFEEDBACK_t;

class CBasePlayer;

struct FFBaseParams_t
{
	FFBaseParams_t() :
		m_flDirection( 0.0f ),
		m_flDuration( 0.0f ),
		m_flGain( 1.0f ),
		m_nPriority( 0 ),
		m_bSolo( false )
	{
	}

	float			m_flDirection;	// yaw
	float			m_flDuration;	// seconds (-1 == INFINITE, 0.0 == use duration from .ffe file)
	float			m_flGain;		// 0 -> 1 global scale
	int				m_nPriority;	// Higher is more important
	bool			m_bSolo;		// Temporarily suppress all other FF effects while playing
};

abstract_class IForceFeedback
{
public:
	// API
	virtual void			StopAllEffects( CBasePlayer *player ) = 0;
	virtual void			StopEffect( CBasePlayer *player, FORCEFEEDBACK_t effect ) = 0;
	virtual void			StartEffect( CBasePlayer *player, FORCEFEEDBACK_t effect, const FFBaseParams_t& params ) = 0;

	virtual void			PauseAll( CBasePlayer *player ) = 0;
	virtual void			ResumeAll( CBasePlayer *player ) = 0;
};

extern IForceFeedback *forcefeedback;

#endif // FORCEFEEDBACK_H
