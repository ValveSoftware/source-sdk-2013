//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_notice.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_notice.h"
#include "eyeball_boss_approach_target.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossNotice::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	m_timer.Start( 0.25f );

	me->EmitSound( "Halloween.EyeballBossBecomeAlert" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossNotice::Update( CEyeballBoss *me, float interval )
{
	CBaseEntity *victim = me->GetVictim();

	if ( !victim )
	{
		return Done( "Victim gone" );
	}

	me->GetBodyInterface()->AimHeadTowards( victim );

	if ( m_timer.IsElapsed() )
	{
		return ChangeTo( new CEyeballBossApproachTarget, "Chasing victim" );
	}

	return Continue();
}

