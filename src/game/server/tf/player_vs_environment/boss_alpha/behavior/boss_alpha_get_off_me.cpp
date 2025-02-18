//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_get_off_me.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_get_off_me.h"

ConVar tf_boss_alpha_charge_pushaway_force( "tf_boss_alpha_charge_pushaway_force", "500"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
void PushawayPlayer( CTFPlayer *victim, const Vector &pushOrigin, float pushForce )
{
	if ( !victim )
		return;

	if ( victim->GetFlags() & FL_ONGROUND )
	{
		// launching into the air
		victim->SetAbsVelocity( vec3_origin );

		const float stunTime = 0.5f;
		victim->m_Shared.StunPlayer( stunTime, 1.0, TF_STUN_MOVEMENT );

		victim->ApplyPunchImpulseX( RandomInt( 10, 15 ) );
		victim->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:0,victim:1" );
	}

	victim->RemoveFlag( FL_ONGROUND );

	Vector toVictim = victim->WorldSpaceCenter() - pushOrigin;
	toVictim.z = 0.0f;
	toVictim.NormalizeInPlace();
	toVictim.z = 1.0f;

	victim->ApplyAbsVelocityImpulse( pushForce * toVictim );
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaGetOffMe::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	me->AddGestureSequence( me->LookupSequence( "gesture_melee_help" ) );
	m_timer.Start( 0.5f );

	me->AddCondition( CBossAlpha::BUSY );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaGetOffMe::Update( CBossAlpha *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		// blast players off of my head
		CUtlVector< CTFPlayer * > onMeVector;
		me->CollectPlayersStandingOnMe( &onMeVector );

		Vector headPos;
		QAngle headAngles;
		if ( me->GetAttachment( "head", headPos, headAngles ) )
		{
			for( int i=0; i<onMeVector.Count(); ++i )
			{
				// push 'em off
				PushawayPlayer( onMeVector[i], headPos, tf_boss_alpha_charge_pushaway_force.GetFloat() );
			}
		}

		me->EmitSound( "Weapon_FlameThrower.AirBurstAttack" );

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaGetOffMe::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->RemoveCondition( CBossAlpha::BUSY );
}


#endif // TF_RAID_MODE
