//========= Copyright Valve Corporation, All rights reserved. ============//
// Michael Booth, September 2012

#include "cbase.h"
#include "string_t.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_teleporter.h"
#include "bot/map_entities/tf_bot_hint_teleporter_exit.h"

ConVar tf_bot_engineer_mvm_building_health_multiplier( "tf_bot_engineer_building_health_multiplier", "2", FCVAR_CHEAT );

extern ConVar tf_bot_path_lookahead_range;

				   
//---------------------------------------------------------------------------------------------
CTFBotMvMEngineerBuildTeleportExit::CTFBotMvMEngineerBuildTeleportExit( CTFBotHintTeleporterExit *hint )
{
	m_teleporterBuildHint = hint;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerBuildTeleportExit::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerBuildTeleportExit::Update( CTFBot *me, float interval )
{
	if ( m_teleporterBuildHint == NULL )
		return Done( "No hint entity" );

	// various interruptions could mean we're away from our build location - move to it
	if ( me->IsRangeGreaterThan( m_teleporterBuildHint->GetAbsOrigin(), 25.0f ) )
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, m_teleporterBuildHint->GetAbsOrigin(), cost );
		}

		m_path.Update( me );

		if ( !m_path.IsValid() )
		{
			return Done( "Path failed" );
		}

		return Continue();
	}

	if ( !m_delayBuildTime.HasStarted() )
	{
		m_delayBuildTime.Start( 0.1f );
		TFGameRules()->PushAllPlayersAway( m_teleporterBuildHint->GetAbsOrigin(), 400, 500, TF_TEAM_RED );
	}
	else if ( m_delayBuildTime.IsElapsed() )
	{
		// destroy previous object
		me->DetonateObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT, true );

		// directly create at the precise position and orientation desired
		CObjectTeleporter* myTeleporter = (CObjectTeleporter *)CreateEntityByName( "obj_teleporter" );
		if ( myTeleporter )
		{
			myTeleporter->SetAbsOrigin( m_teleporterBuildHint->GetAbsOrigin() );
			myTeleporter->SetAbsAngles( QAngle( 0, m_teleporterBuildHint->GetAbsAngles().y, 0 ) );
			myTeleporter->SetObjectMode( MODE_TELEPORTER_EXIT );
			myTeleporter->Spawn();

			myTeleporter->SetTeleportWhere( me->GetTeleportWhere() );

			if ( me->ShouldQuickBuild() )
			{
				myTeleporter->ForceQuickBuild();
			}

			myTeleporter->StartPlacement( me );
			myTeleporter->StartBuilding( me );

			int iHealth = myTeleporter->GetMaxHealthForCurrentLevel() * tf_bot_engineer_mvm_building_health_multiplier.GetFloat();
			myTeleporter->SetMaxHealth( iHealth );
			myTeleporter->SetHealth( iHealth );

			m_teleporterBuildHint->SetOwnerEntity( myTeleporter );

			me->EmitSound( "Engineer.MVM_AutoBuildingTeleporter02" );

			return Done( "Teleport exit built" );
		}
	}

	return Continue();
}
