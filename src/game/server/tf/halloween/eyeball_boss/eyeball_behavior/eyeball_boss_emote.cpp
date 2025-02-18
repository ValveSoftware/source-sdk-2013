//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emote.cpp
// The 2011 Halloween Boss - play an animation
// Michael Booth, October 2011

#include "cbase.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_emote.h"


//---------------------------------------------------------------------------------------------
CEyeballBossEmote::CEyeballBossEmote( int animationSequence, const char *soundName, Action< CEyeballBoss > *nextAction )
{
	m_animationSequence = animationSequence;
	m_soundName = soundName;
	m_nextAction = nextAction;
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEmote::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	if ( m_animationSequence )
	{
		me->SetSequence( m_animationSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	if ( m_soundName )
	{
		me->EmitSound( m_soundName );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEmote::Update( CEyeballBoss *me, float interval )
{
	if ( me->IsSequenceFinished() )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction );
		}

		return Done();
	}

	return Continue();
}

