//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_approach_target.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_approach_target.h"
#include "eyeball_boss_launch_rockets.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossApproachTarget::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	m_giveUpTimer.Start( 5.0f );
	m_minChaseTimer.Start( 0.5f );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossApproachTarget::Update( CEyeballBoss *me, float interval )
{
	CBaseCombatCharacter *victim = me->GetVictim();
	CBaseCombatCharacter *closestVictim = me->FindClosestVisibleVictim();

	if ( victim != closestVictim && m_minChaseTimer.IsElapsed() )
	{
		return Done( "Noticed better victim" );
	}

	if ( !victim || !victim->IsAlive() )
	{
		return Done( "Victim gone" );
	}

	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done( "Giving up" );
	}

	bool isVictimVisible = me->IsLineOfSightClear( victim, CBaseCombatCharacter::IGNORE_ACTORS );

	if ( !isVictimVisible )
	{
		if ( m_lingerTimer.IsElapsed() )
		{
			return Done( "Lost victim" );
		}

		// wait a bit to see if we catch a glimpse of our victim again
		return Continue();
	}

	m_lingerTimer.Start( 1.0f );

	float attackRange = tf_eyeball_boss_attack_range.GetFloat();
	if ( me->IsEnraged() )
	{
		attackRange *= 2.0f;
	}

	if ( me->IsRangeLessThan( victim, attackRange ) )
	{
		return ChangeTo( new CEyeballBossLaunchRockets, "Rocket attack!" );
	}

	// approach victim
	me->GetLocomotionInterface()->SetDesiredSpeed( tf_eyeball_boss_speed.GetFloat() );
	me->GetLocomotionInterface()->Approach( victim->WorldSpaceCenter() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CEyeballBossApproachTarget::OnEnd( CEyeballBoss *me, Action< CEyeballBoss > *nextAction )
{
}
