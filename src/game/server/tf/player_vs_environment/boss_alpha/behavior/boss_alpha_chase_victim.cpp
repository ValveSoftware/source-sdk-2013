//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_chase_victim.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_chase_victim.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_lost_victim.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_nuke_attack.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_launch_grenades.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_launch_rockets.h"

extern ConVar tf_boss_alpha_grenade_launch_range;
extern ConVar tf_boss_alpha_chase_range;


//---------------------------------------------------------------------------------------------
CBossAlphaChaseVictim::CBossAlphaChaseVictim( CBaseCombatCharacter *chaseTarget )
{
	m_chaseTarget = chaseTarget;
	m_lastKnownTargetSpot = chaseTarget->GetAbsOrigin();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaChaseVictim::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	if ( m_chaseTarget == NULL )
	{
		return Done( "Target is NULL" );
	}

	m_lastKnownTargetSpot = m_chaseTarget->GetAbsOrigin();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaChaseVictim::Update( CBossAlpha *me, float interval )
{
	if ( m_chaseTarget == NULL || !m_chaseTarget->IsAlive() )
	{
		return ChangeTo( new CBossAlphaLostVictim, "No victim" );
	}

	if ( m_chaseTarget != me->GetAttackTarget() )
	{
		return Done( "Changing targets" );
	}

	Vector moveGoal = m_chaseTarget->GetAbsOrigin();

	if ( me->IsLineOfSightClear( m_chaseTarget ) )
	{
		if ( !m_visibleTimer.HasStarted() )
		{
			m_visibleTimer.Start();
		}

		if ( me->HasAbility( CBossAlpha::CAN_NUKE ) && me->GetNukeTimer()->IsElapsed() )
		{
			return SuspendFor( new CBossAlphaNukeAttack, "Nuking!" );
		}

		m_lastKnownTargetSpot = m_chaseTarget->GetAbsOrigin();

		if ( me->HasAbility( CBossAlpha::CAN_LAUNCH_STICKIES ) )
		{
			if ( ( me->GetGrenadeTimer()->IsElapsed() && me->IsRangeLessThan( m_chaseTarget, tf_boss_alpha_grenade_launch_range.GetFloat() ) ) ||
				   me->IsInCondition( CBossAlpha::ENRAGED ) )
			{
				return SuspendFor( new CBossAlphaLaunchGrenades, "Target is close (or I am enraged) - grenades!" );
			}
		}

		// chase into line of sight a bit so they can't immediately get behind cover again
		if ( me->HasAbility( CBossAlpha::CAN_FIRE_ROCKETS ) )
		{
			if ( m_visibleTimer.IsGreaterThen( 1.0f ) ||
				 me->IsRangeLessThan( m_chaseTarget, tf_boss_alpha_chase_range.GetFloat() ) )
			{
				return SuspendFor( new CBossAlphaLaunchRockets, "Fire!" );
			}
		}

		if ( me->IsRangeLessThan( m_chaseTarget, 150.0f ) )
		{
			// too close - stand still
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_MELEE ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_STAND_MELEE );
			}

			return Continue();
		}
	}
	else
	{
		m_visibleTimer.Invalidate();

		// move to where we last saw our target
		moveGoal = m_lastKnownTargetSpot;

		if ( me->IsRangeLessThan( m_lastKnownTargetSpot, 20.0f ) )
		{
			// reached spot where we last saw our victim - give up
			me->SetAttackTarget( NULL );

			return ChangeTo( new CBossAlphaLostVictim, "I lost my chase victim" );
		}
	}


	// move into sight of target
	if ( m_path.GetAge() > 1.0f )
	{
		CBossAlphaPathCost cost( me );
		m_path.Compute( me, moveGoal, cost );
	}

	me->GetLocomotionInterface()->Run();
	m_path.Update( me );

	// play running animation
	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaChaseVictim::OnMoveToSuccess( CBossAlpha *me, const Path *path )
{
	return TryDone( RESULT_CRITICAL, "Reached move goal" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaChaseVictim::OnMoveToFailure( CBossAlpha *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Path follow failed" );
}


//---------------------------------------------------------------------------------------------
void CBossAlphaChaseVictim::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaChaseVictim::OnStuck( CBossAlpha *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue();
}

#endif // TF_RAID_MODE
