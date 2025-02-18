//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_stunned.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_stunned.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossStunned::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	m_stunTimer.Start( 5.0f );

	int animSequence = me->LookupSequence( "stunned" );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossStunned" );

	// limit the total amount of damage we can take while stunned
	me->SetDamageLimit( me->GetMaxHealth() / 3 );

	// sink
	me->GetLocomotionInterface()->SetDesiredAltitude( 0.0f );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossStunned::Update( CEyeballBoss *me, float interval )
{
	if ( m_stunTimer.IsElapsed() )
	{
		// get mad and retaliate
		me->BecomeEnraged( 20.0f );

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CEyeballBossStunned::OnEnd( CEyeballBoss *me, Action< CEyeballBoss > *nextAction )
{
	me->RemoveDamageLimit();

	// resume hovering
	me->GetLocomotionInterface()->SetDesiredAltitude( tf_eyeball_boss_hover_height.GetFloat() );
}
