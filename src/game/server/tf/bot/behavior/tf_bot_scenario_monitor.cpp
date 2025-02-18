//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_scenario_monitor.h
// Behavior layer that interrupts for scenario rules (picked up flag, drop what you're doing and capture, etc)
// Michael Booth, May 2011

#include "cbase.h"
#include "fmtstr.h"

#include "tf_gamerules.h"
#include "tf_weapon_pipebomblauncher.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/tf_bot.h"
#include "bot/tf_bot_manager.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_wait.h"
#include "bot/behavior/tf_bot_tactical_monitor.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/tf_bot_get_health.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/behavior/sniper/tf_bot_sniper_lurk.h"
#include "bot/behavior/scenario/capture_point/tf_bot_capture_point.h"
#include "bot/behavior/scenario/capture_point/tf_bot_defend_point.h"
#include "bot/behavior/scenario/payload/tf_bot_payload_guard.h"
#include "bot/behavior/scenario/payload/tf_bot_payload_push.h"
#include "bot/behavior/tf_bot_use_teleporter.h"
#include "bot/behavior/training/tf_bot_training.h"
#include "bot/behavior/tf_bot_destroy_enemy_sentry.h"
#include "bot/behavior/engineer/tf_bot_engineer_building.h"
#include "bot/behavior/spy/tf_bot_spy_infiltrate.h"
#include "bot/behavior/spy/tf_bot_spy_leave_spawn_room.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"

#ifdef TF_RAID_MODE
#include "bot/behavior/scenario/raid/tf_bot_wander.h"
#include "bot/behavior/scenario/raid/tf_bot_companion.h"
#include "bot/behavior/scenario/raid/tf_bot_squad_attack.h"
#include "bot/behavior/scenario/raid/tf_bot_guard_area.h"
#endif // TF_RAID_MODE

#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_seek_and_destroy.h"
#include "bot/behavior/tf_bot_taunt.h"
#include "bot/behavior/tf_bot_escort.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h"

#include "bot/behavior/missions/tf_bot_mission_suicide_bomber.h"
#include "bot/behavior/squad/tf_bot_escort_squad_leader.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.h"
#include "bot/behavior/missions/tf_bot_mission_reprogrammed.h"

#include "bot/behavior/tf_bot_scenario_monitor.h"


extern ConVar tf_bot_health_ok_ratio;
extern ConVar tf_bot_health_critical_ratio;


//-----------------------------------------------------------------------------------------
// Returns the initial Action we will run concurrently as a child to us
Action< CTFBot > *CTFBotScenarioMonitor::InitialContainedAction( CTFBot *me )
{
	if ( me->IsInASquad() )
	{
		if ( me->GetSquad()->IsLeader( me ) )
		{
			// I'm the leader of this Squad, so I can do what I want and the other Squaddies will support me
			return DesiredScenarioAndClassAction( me );
		}

		// Medics are the exception - they always heal, and have special squad logic in their heal logic
		if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			return new CTFBotMedicHeal;
		}

		// I'm in a Squad but not the leader, do "escort and support" Squad behavior
		// until the Squad disbands, and then do my normal thing
		return new CTFBotEscortSquadLeader( DesiredScenarioAndClassAction( me ) );
	}

	return DesiredScenarioAndClassAction( me );
}


//-----------------------------------------------------------------------------------------
// Returns Action specific to the scenario and my class
Action< CTFBot > *CTFBotScenarioMonitor::DesiredScenarioAndClassAction( CTFBot *me )
{
	switch( me->GetMission() )
	{
	case CTFBot::MISSION_SEEK_AND_DESTROY:
		break;

	case CTFBot::MISSION_DESTROY_SENTRIES:
		return new CTFBotMissionSuicideBomber;

	case CTFBot::MISSION_SNIPER:
		return new CTFBotSniperLurk;

	}

#ifdef TF_RAID_MODE
	if ( me->HasAttribute( CTFBot::IS_NPC ) )
	{
		// map-spawned guardians
		return new CTFBotGuardian;
	}
#endif // TF_RAID_MODE

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsBossBattleMode() )
	{
		if ( me->GetTeamNumber() == TF_TEAM_BLUE )
		{
			// bot teammates
			return new CTFBotCompanion;
		}
		
		if ( me->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			return new CTFBotSniperLurk;
		}

		if ( me->IsPlayerClass( TF_CLASS_SPY ) )
		{
			return new CTFBotSpyInfiltrate;
		}

		if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			return new CTFBotMedicHeal;
		}

		if ( me->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			return new CTFBotEngineerBuild;
		}

		return new CTFBotEscort( TFGameRules()->GetActiveBoss() );
	}
	else if ( TFGameRules()->IsRaidMode() )
	{
		if ( me->GetTeamNumber() == TF_TEAM_BLUE )
		{
			// bot teammates
			return new CTFBotCompanion;
		}

		if ( me->IsInASquad() )
		{
			// squad behavior
			return new CTFBotSquadAttack;
		}

		if ( me->IsPlayerClass( TF_CLASS_SCOUT ) || me->HasAttribute( CTFBot::AGGRESSIVE ) )
		{
			return new CTFBotWander;
		}

		if ( me->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			return new CTFBotSniperLurk;
		}

		if ( me->IsPlayerClass( TF_CLASS_SPY ) )
		{
			return new CTFBotSpyInfiltrate;
		}

		return new CTFBotGuardArea;
	}
#endif // TF_RAID_MODE	

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( me->IsPlayerClass( TF_CLASS_SPY ) )
		{
			return new CTFBotSpyLeaveSpawnRoom;
		}

		if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			// if I'm being healed by another medic, I should do something else other than healing
			bool bIsBeingHealedByAMedic = false;
			int nNumHealers = me->m_Shared.GetNumHealers();
			for ( int i=0; i<nNumHealers; ++i )
			{
				CBaseEntity *pHealer = me->m_Shared.GetHealerByIndex(i);
				if ( pHealer && pHealer->IsPlayer() )
				{
					bIsBeingHealedByAMedic = true;
					break;
				}
			}

			if ( !bIsBeingHealedByAMedic )
			{
				return new CTFBotMedicHeal;
			}
		}

		if ( me->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			return new CTFBotMvMEngineerIdle;
		}

		// NOTE: Snipers are intentionally left out so they go after the flag. Actual sniping behavior is done as a mission.

		if ( me->HasAttribute( CTFBot::AGGRESSIVE ) )
		{
			// push for the point first, then attack
			return new CTFBotPushToCapturePoint( new CTFBotFetchFlag );
		}

		// capture the flag
		return new CTFBotFetchFlag;
	}

	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return new CTFBotSpyInfiltrate;
	}

	if ( !TheTFBots().IsMeleeOnly() )
	{
		if ( me->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			return new CTFBotSniperLurk;
		}

		if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			return new CTFBotMedicHeal;
		}

		if ( me->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			return new CTFBotEngineerBuild;
		}
	}

	if ( me->GetFlagToFetch() )
	{
		// capture the flag
		return new CTFBotFetchFlag;
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		// push the cart
		if ( me->GetTeamNumber() == TF_TEAM_BLUE )
		{
			// blu is pushing
			return new CTFBotPayloadPush;
		}
		else if ( me->GetTeamNumber() == TF_TEAM_RED )
		{
			// red is blocking
			return new CTFBotPayloadGuard;
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		// if we have a point we can capture - do it
		CUtlVector< CTeamControlPoint * > captureVector;
		TFGameRules()->CollectCapturePoints( me, &captureVector );

		if ( captureVector.Count() > 0 )
		{
			return new CTFBotCapturePoint;
		}

		// otherwise, defend our point(s) from capture
		CUtlVector< CTeamControlPoint * > defendVector;
		TFGameRules()->CollectDefendPoints( me, &defendVector );

		if ( defendVector.Count() > 0 )
		{
			return new CTFBotDefendPoint;
		}

		// likely KotH mode and/or all points are locked - assume capture
		DevMsg( "%3.2f: %s: Gametype is CP, but I can't find a point to capture or defend!\n", gpGlobals->curtime, me->GetDebugIdentifier() );
		return new CTFBotCapturePoint;
	}
	else
	{
		// scenario not implemented yet - just fight
		return new CTFBotSeekAndDestroy;
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotScenarioMonitor::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_ignoreLostFlagTimer.Start( 20.0f );
	m_lostFlagTimer.Invalidate();
	return Continue();
}


ConVar tf_bot_fetch_lost_flag_time( "tf_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT, "How long busy TFBots will ignore the dropped flag before they give up what they are doing and go after it" );
ConVar tf_bot_flag_kill_on_touch( "tf_bot_flag_kill_on_touch", "0", FCVAR_CHEAT, "If nonzero, any bot that picks up the flag dies. For testing." );


//-----------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotScenarioMonitor::Update( CTFBot *me, float interval )
{
	// CTF Scenario
	if ( me->HasTheFlag() )
	{
		if ( tf_bot_flag_kill_on_touch.GetBool() )
		{
			me->CommitSuicide( false, true );
			return Done( "Flag kill" );
		}

		// we just picked up the flag - drop what we're doing and take it in
		return SuspendFor( new CTFBotDeliverFlag, "I've picked up the flag! Running it in..." );
	}

	if ( me->HasMission( CTFBot::NO_MISSION ) && m_ignoreLostFlagTimer.IsElapsed() && me->IsAllowedToPickUpFlag() )
	{
		CCaptureFlag *flag = me->GetFlagToFetch();

		if ( flag )
		{
			CTFPlayer *carrier = ToTFPlayer( flag->GetOwnerEntity() );
			if ( carrier )
			{
				m_lostFlagTimer.Invalidate();
			}
			else
			{
				// flag is loose
				if ( !m_lostFlagTimer.HasStarted() )
				{
					m_lostFlagTimer.Start( tf_bot_fetch_lost_flag_time.GetFloat() );
				}
				else if ( m_lostFlagTimer.IsElapsed() )
				{
					m_lostFlagTimer.Invalidate();

					// if we're a Medic an actively healing someone, don't interrupt
					if ( !me->MedicGetHealTarget() )
					{
						// we better go get the flag
						return SuspendFor( new CTFBotFetchFlag( TEMPORARY_FLAG_FETCH ), "Fetching lost flag..." );
					}
				}
			}
		}
	}

	return Continue();
}

