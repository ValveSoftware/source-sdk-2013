//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_stunned.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_shareddefs.h"
#include "tf_ammo_pack.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_stunned.h"


extern ConVar tf_boss_alpha_min_nuke_after_stun_time;


//---------------------------------------------------------------------------------------------
CBossAlphaStunned::CBossAlphaStunned( float duration, Action< CBossAlpha > *nextAction )
{
	m_timer.Start( duration );
	m_nextAction = nextAction;
}


//---------------------------------------------------------------------------------------------
ConVar tf_boss_alpha_stun_ammo_count( "tf_boss_alpha_stun_ammo_count", "3"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_stun_ammo_amount( "tf_boss_alpha_stun_ammo_amount", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_stun_ammo_velocity( "tf_boss_alpha_stun_ammo_velocity", "100"/*, FCVAR_CHEAT*/ );

void TossAmmoPack( CBossAlpha *me )
{
	int iPrimary = tf_boss_alpha_stun_ammo_amount.GetInt();
	int iSecondary = tf_boss_alpha_stun_ammo_amount.GetInt();
	int iMetal = tf_boss_alpha_stun_ammo_amount.GetInt();

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( me->GetAbsOrigin(), me->GetAbsAngles(), NULL, "models/items/ammopack_medium.mdl" );
	if ( pAmmoPack )
	{
/*
		Vector vel;
		
		vel.x = RandomFloat( -1.0f, 1.0f ) * tf_boss_alpha_stun_ammo_velocity.GetFloat();
		vel.y = RandomFloat( -1.0f, 1.0f ) * tf_boss_alpha_stun_ammo_velocity.GetFloat();
		vel.z = tf_boss_alpha_stun_ammo_velocity.GetFloat();

		pAmmoPack->SetInitialVelocity( vel );
*/
		pAmmoPack->m_nSkin = 0;

		// Give the ammo pack some health, so that trains can destroy it.
		pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pAmmoPack->m_takedamage = DAMAGE_YES;		
		pAmmoPack->SetHealth( 900 );

		pAmmoPack->SetBodygroup( 1, 1 );

		pAmmoPack->ApplyLocalAngularVelocityImpulse( AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ) );

		DispatchSpawn( pAmmoPack );

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( iPrimary, TF_AMMO_PRIMARY );
		pAmmoPack->GiveAmmo( iSecondary, TF_AMMO_SECONDARY );
		pAmmoPack->GiveAmmo( iMetal, TF_AMMO_METAL );
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaStunned::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_MELEE );
	m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_Stun_begin" ), 0 );
	m_state = BECOMING_STUNNED;

	m_timer.Reset();

	me->AddCondition( CBossAlpha::STUNNED );
	me->EmitSound( "RobotBoss.StunStart" );

	// throw out some ammo
	for( int i=0; i<tf_boss_alpha_stun_ammo_count.GetInt(); ++i )
	{
		TossAmmoPack( me );
	}

	// relay the event to the map logic
	me->m_outputOnStunned.FireOutput( me, me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaStunned::Update( CBossAlpha *me, float interval )
{
	switch( m_state )
	{
	case BECOMING_STUNNED:
		if ( me->IsSequenceFinished() )
		{
			me->FastRemoveLayer( m_layerUsed );

			m_state = STUNNED;
			m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_stun_middle" ), 0 );
			me->SetLayerLooping( m_layerUsed, true );
			me->EmitSound( "RobotBoss.Stunned" );
		}
		break;

	case STUNNED:
		if ( m_timer.IsElapsed() )
		{
			me->FastRemoveLayer( m_layerUsed );

			m_state = RECOVERING;
			m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_stun_end" ), 0 );
			me->StopSound( "RobotBoss.Stunned" );
			me->EmitSound( "RobotBoss.StunRecover" );
		}
		break;

	case RECOVERING:
		if ( me->IsSequenceFinished() )
		{
			me->FastRemoveLayer( m_layerUsed );

			if ( m_nextAction )
			{
				return ChangeTo( m_nextAction, "Stun finished" );
			}

			return Done( "Stun finished" );
		}
		break;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaStunned::OnInjured( CBossAlpha *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL );
}


//---------------------------------------------------------------------------------------------
void CBossAlphaStunned::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->RemoveCondition( CBossAlpha::STUNNED );

	if ( me->HasAbility( CBossAlpha::CAN_ENRAGE ) )
	{
		// being stunned makes the boss ANGRY!
		me->AddCondition( CBossAlpha::ENRAGED );
	}

	// make sure the boss attacks at least once before he starts a nuke
	if ( me->GetNukeTimer()->GetRemainingTime() < tf_boss_alpha_min_nuke_after_stun_time.GetFloat() )
	{
		me->GetNukeTimer()->Start( tf_boss_alpha_min_nuke_after_stun_time.GetFloat() );
	}
}


#endif // TF_RAID_MODE
