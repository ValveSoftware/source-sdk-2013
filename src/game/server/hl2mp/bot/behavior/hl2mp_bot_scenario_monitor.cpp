//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "fmtstr.h"

#include "hl2mp_gamerules.h"
#include "hl2mp/weapon_slam.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/hl2mp_bot.h"
#include "bot/hl2mp_bot_manager.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_wait.h"
#include "bot/behavior/hl2mp_bot_tactical_monitor.h"
#include "bot/behavior/hl2mp_bot_retreat_to_cover.h"
#include "bot/behavior/hl2mp_bot_get_health.h"
#include "bot/behavior/hl2mp_bot_get_ammo.h"

#include "bot/behavior/hl2mp_bot_attack.h"
#include "bot/behavior/hl2mp_bot_seek_and_destroy.h"

#include "bot/behavior/hl2mp_bot_scenario_monitor.h"


extern ConVar hl2mp_bot_health_ok_ratio;
extern ConVar hl2mp_bot_health_critical_ratio;


//-----------------------------------------------------------------------------------------
// Returns the initial Action we will run concurrently as a child to us
Action< CHL2MPBot > *CHL2MPBotScenarioMonitor::InitialContainedAction( CHL2MPBot *me )
{
	if ( me->IsInASquad() )
	{
		if ( me->GetSquad()->IsLeader( me ) )
		{
			// I'm the leader of this Squad, so I can do what I want and the other Squaddies will support me
			return DesiredScenarioAndClassAction( me );
		}

		// I'm in a Squad but not the leader, do "escort and support" Squad behavior
		// until the Squad disbands, and then do my normal thing
		//
		// TODO: Implement this if we ever want squads in HL2MP.
		// It's like, an MVM thing, not really useful for us.
		//return new CHL2MPBotEscortSquadLeader( DesiredScenarioAndClassAction( me ) );
	}

	return DesiredScenarioAndClassAction( me );
}


//-----------------------------------------------------------------------------------------
// Returns Action specific to the scenario and my class
Action< CHL2MPBot > *CHL2MPBotScenarioMonitor::DesiredScenarioAndClassAction( CHL2MPBot *me )
{
	return new CHL2MPBotSeekAndDestroy;
}


//-----------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotScenarioMonitor::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	m_ignoreLostFlagTimer.Start( 20.0f );
	m_lostFlagTimer.Invalidate();
	return Continue();
}


ConVar hl2mp_bot_fetch_lost_flag_time( "hl2mp_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT, "How long busy HL2MPBots will ignore the dropped flag before they give up what they are doing and go after it" );
ConVar hl2mp_bot_flag_kill_on_touch( "hl2mp_bot_flag_kill_on_touch", "0", FCVAR_CHEAT, "If nonzero, any bot that picks up the flag dies. For testing." );


//-----------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotScenarioMonitor::Update( CHL2MPBot *me, float interval )
{
	return Continue();
}

