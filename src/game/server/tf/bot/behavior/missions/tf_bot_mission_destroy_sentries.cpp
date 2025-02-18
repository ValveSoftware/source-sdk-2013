//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mission_destroy_sentries.cpp
// Seek and destroy enemy sentries and ignore everything else
// Michael Booth, June 2011

#include "cbase.h"
#include "team.h"
#include "bot/tf_bot.h"
#include "bot/behavior/missions/tf_bot_mission_destroy_sentries.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"
#include "bot/behavior/tf_bot_destroy_enemy_sentry.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/missions/tf_bot_mission_suicide_bomber.h"
#include "tf_obj_sentrygun.h"

//
// NOTE: This behavior is deprecated and unused for now.
// The only sentry destroying mission is the Sentry Buster right now (suicide bomber).
//

//---------------------------------------------------------------------------------------------
CTFBotMissionDestroySentries::CTFBotMissionDestroySentries( CObjectSentrygun *goalSentry )
{
	m_goalSentry = goalSentry;
}


//---------------------------------------------------------------------------------------------
CObjectSentrygun *CTFBotMissionDestroySentries::SelectSentryTarget( CTFBot *me )
{
	
	return NULL;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMissionDestroySentries::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		return ChangeTo( new CTFBotMedicHeal, "My job is to heal/uber the others in the mission" );
	}

	// focus only on the mission
	me->SetAttribute( CTFBot::IGNORE_ENEMIES );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotMissionDestroySentries::Update( CTFBot *me, float interval )
{
	if ( m_goalSentry == NULL )
	{
		// first destroy the sentry we were assigned to, or any sentry we discovered or that is attacking us
		m_goalSentry = me->GetEnemySentry();

		if ( m_goalSentry == NULL )
		{
			// next destroy the most dangerous sentry
 			int iTeam = ( me->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;

			if ( TFGameRules() && TFGameRules()->IsPVEModeActive() )
			{
				iTeam = TF_TEAM_PVE_DEFENDERS;
			}

			m_goalSentry = TFGameRules()->FindSentryGunWithMostKills( iTeam );
		}
	}

	// for suicide bombers, we never want them to revert to normal behavior even if there is no sentry to kill
	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		return SuspendFor( new CTFBotMissionSuicideBomber, "On a suicide mission to blow up a sentry" );
	}

	if ( m_goalSentry == NULL )
	{
		// no sentries left to destroy - our mission is complete
		me->SetMission( CTFBot::NO_MISSION, MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM );
		return ChangeTo( GetParentAction()->InitialContainedAction( me ), "Mission complete - reverting to normal behavior" );
	}

	if ( m_goalSentry != me->GetEnemySentry() )
	{
		me->RememberEnemySentry( m_goalSentry, m_goalSentry->WorldSpaceCenter() );
	}

	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return SuspendFor( new CTFBotSpySap( m_goalSentry ), "On a mission to sap a sentry" );
	}

	return SuspendFor( new CTFBotDestroyEnemySentry, "On a mission to destroy a sentry" );
}


//---------------------------------------------------------------------------------------------
void CTFBotMissionDestroySentries::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->ClearAttribute( CTFBot::IGNORE_ENEMIES );
}
