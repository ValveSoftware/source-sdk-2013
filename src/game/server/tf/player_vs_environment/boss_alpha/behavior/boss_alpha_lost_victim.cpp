//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_lost_victim.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_lost_victim.h"

//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLostVictim::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	m_headTurn = 0.0f;
	m_headYawPoseParameter = me->LookupPoseParameter( "body_yaw" );

	m_timer.Start( RandomFloat( 3.0f, 5.0f ) );

	me->EmitSound( "RobotBoss.Scanning" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaLostVictim::Update( CBossAlpha *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		return Done( "Giving up" );
	}

	CBaseCombatCharacter *target = me->GetAttackTarget();
	if ( target )
	{
		if ( me->IsLineOfSightClear( target ) || me->IsPrisonerOfMinion( target ) )
		{
			me->EmitSound( "RobotBoss.Acquire" );
			me->AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
			return Done( "Ah hah!" );
		}
	}

	const float rate = M_PI / 3.0f;
	m_headTurn += rate * interval;

	float s, c;
	SinCos( m_headTurn, &s, &c );

	me->SetPoseParameter( m_headYawPoseParameter, 40.0f * s );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaLostVictim::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->SetPoseParameter( m_headYawPoseParameter, 0 );
}


#endif // TF_RAID_MODE
