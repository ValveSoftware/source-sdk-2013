//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_raid_companion.cpp
// Teammate bots for Raid mode
// Michael Booth, October 2009

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "team.h"
#include "bot/tf_bot.h"
#include "team_control_point_master.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/scenario/raid/tf_bot_companion.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_move_to_vantage_point.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"
#include "bot/behavior/sniper/tf_bot_sniper_lurk.h"

#include "bot/map_entities/tf_bot_generator.h"		// action point

ConVar tf_raid_companion_follow_range( "tf_raid_companion_follow_range", "150", FCVAR_CHEAT );
ConVar tf_raid_companion_allow_bot_leader( "tf_raid_companion_allow_bot_leader", "0", FCVAR_CHEAT );

extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCompanion::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
CTFPlayer *CTFBotCompanion::GetLeader( void )
{
	CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );
	CTFPlayer *leader = NULL;
	float leaderSpeed = FLT_MAX;

	for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
	{
		CTFPlayer *player = (CTFPlayer *)raidingTeam->GetPlayer(i);

		if ( player->IsBot() && !tf_raid_companion_allow_bot_leader.GetBool() )
			continue;

/*
		if ( player->IsPlayerClass( TF_CLASS_ENGINEER ) ||
			 player->IsPlayerClass( TF_CLASS_SNIPER ) ||
			 player->IsPlayerClass( TF_CLASS_MEDIC ) )
			continue;
*/

		if ( player->IsAlive() )
		{
			float speed = player->GetPlayerClass()->GetMaxSpeed();

			if ( speed < leaderSpeed )
			{
				leader = player;
				leaderSpeed = speed;
			}
		}
	}

	return leader;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCompanion::Update( CTFBot *me, float interval )
{
	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		const CKnownEntity *patient = me->GetVisionInterface()->GetClosestKnown( me->GetTeamNumber() );
		if ( patient )
		{
			return SuspendFor( new CTFBotMedicHeal );
		}
	}

	CTFPlayer *leader = GetLeader();
	if ( !leader )
		return Continue();

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( me->IsSelf( leader ) )
	{
		const float engageRange = 500.0f;
		if ( threat && threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetEntity(), engageRange ) )
		{
			// stop pushing ahead and kill nearby threats
			return SuspendFor( new CTFBotAttack, "Attacking nearby threats" );
		}

		// head toward next capture point
		CTeamControlPoint *point = me->GetMyControlPoint();
		if ( point )
		{
			m_path.Update( me, point, cost );
		}
	}
	else
	{
		if ( ( !threat || threat->GetTimeSinceLastSeen() > 3.0f ) && leader->GetTimeSinceLastInjury() < 1.0f )
		{
			// we don't see anything, but the leader is under attack - find a better vantage point
			const float nearRange = 1000.0f;
			return SuspendFor( new CTFBotMoveToVantagePoint( nearRange ), "Moving to where I can see the enemy" );
		}

		if ( leader && me->IsDistanceBetweenGreaterThan( leader, tf_raid_companion_follow_range.GetFloat() ) )
		{
			m_path.Update( me, leader, cost );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCompanion::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	return Continue();
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotGuardian::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotGuardian::Update( CTFBot *me, float interval )
{
	if ( me->GetActionPoint() )
	{
		const float atHomeRange = 35.0f; // 25.0f;
		const Vector &home = me->GetActionPoint()->GetAbsOrigin();

		if ( me->IsRangeGreaterThan( home, atHomeRange ) )
		{
			if ( m_repathTimer.IsElapsed() && !m_path.IsValid() )
			{
				m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );

				CTFBotPathCost cost( me, FASTEST_ROUTE );
				m_path.Compute( me, home, cost );
			}

			// move home
			m_path.Update( me );

			return Continue();
		}
	}

	// at home
	m_path.Invalidate();
	me->SetHomeArea( me->GetLastKnownArea() );

	if ( me->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		return SuspendFor( new CTFBotEngineerBuild );
	}

	if ( me->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		return SuspendFor( new CTFBotSniperLurk );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotGuardian::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGuardian::OnStuck( CTFBot *me )
{
	m_path.Invalidate();
	return TryContinue( RESULT_IMPORTANT );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGuardian::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	m_path.Invalidate();
	return TryContinue( RESULT_IMPORTANT );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGuardian::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue( RESULT_IMPORTANT );
}

#endif // TF_RAID_MODE
