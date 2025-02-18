//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "tf_obj_sentrygun.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_teleport_spawn.h"
#include "bot/map_entities/tf_bot_hint_entity.h"
#include "string_t.h"
#include "tf_fx.h"
#include "player_vs_environment/tf_population_manager.h"

//---------------------------------------------------------------------------------------------
CTFBotMvMEngineerTeleportSpawn::CTFBotMvMEngineerTeleportSpawn( CBaseTFBotHintEntity* pHint, bool bFirstTeleportSpawn )
{
	m_hintEntity = pHint;
	m_bFirstTeleportSpawn = bFirstTeleportSpawn;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerTeleportSpawn::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	if ( !me->HasAttribute( CTFBot::TELEPORT_TO_HINT ) )
	{
		return Done( "Cannot teleport to hint with out Attributes TeleportToHint" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerTeleportSpawn::Update( CTFBot *me, float interval )
{
	if ( !m_teleportDelay.HasStarted() )
	{
		m_teleportDelay.Start( 0.1f );
		if ( m_hintEntity )
			TFGameRules()->PushAllPlayersAway( m_hintEntity->GetAbsOrigin(), 400, 500, TF_TEAM_RED );
	}
	else if ( m_teleportDelay.IsElapsed() )
	{
		if ( !m_hintEntity )
			return Done( "Cannot teleport to hint as m_hintEntity is NULL" );

		// teleport the engineer to the sentry spawn point
		QAngle angles = m_hintEntity->GetAbsAngles();
		Vector origin = m_hintEntity->GetAbsOrigin();
		origin.z += 10.f; // move up off the around a little bit to prevent the engineer from getting stuck in the ground

		me->Teleport( &origin, &angles, NULL );

		CPVSFilter filter( origin );
		TE_TFParticleEffect( filter, 0.0, "teleported_blue", origin, vec3_angle );
		TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", origin, vec3_angle );

		if ( m_bFirstTeleportSpawn )
		{
			// notify players that engineer's teleported into the map
			TE_TFParticleEffect( filter, 0.0, "teleported_mvm_bot", origin, vec3_angle );
			me->EmitSound( "Engineer.MVM_BattleCry07" );
			m_hintEntity->EmitSound( "MVM.Robot_Engineer_Spawn" );

			if ( g_pPopulationManager )
			{
				CWave *pWave = g_pPopulationManager->GetCurrentWave();
				if ( pWave )
				{
					if ( pWave->NumEngineersTeleportSpawned() == 0 )
					{
						TFGameRules()->BroadcastSound( 255, "Announcer.MVM_First_Engineer_Teleport_Spawned" );
					}
					else
					{
						TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Another_Engineer_Teleport_Spawned" );
					}

					pWave->IncrementEngineerTeleportSpawned();
				}
			}
		}

		return Done( "Teleported" );
	}

	return Continue();
}

