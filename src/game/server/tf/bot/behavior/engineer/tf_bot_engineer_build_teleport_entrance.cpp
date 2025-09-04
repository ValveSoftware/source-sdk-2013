//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_teleport_entrance.cpp
// Engineer building a teleport entrance right outside of the spawn room
// Michael Booth, May 2009

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_weapon_builder.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_entrance.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/tf_bot_get_ammo.h"
// Guarded Engineer behavior seams (header-only; default off)
#include "bot/behavior/engineer/tf_bot_engineer_seams.h"

#if TF_BOT_ENGINEER_SEAMS
extern ConVar tf_bot_engineer_seams_debug;
#endif

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_max_teleport_entrance_travel( "tf_bot_max_teleport_entrance_travel", "1500", FCVAR_CHEAT, "Don't plant teleport entrances farther than this travel distance from our spawn room" );
ConVar tf_bot_teleport_build_surface_normal_limit( "tf_bot_teleport_build_surface_normal_limit", "0.99", FCVAR_CHEAT, "If the ground normal Z component is less that this value, Engineer bots won't place their entrance teleporter" );

//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildTeleportEntrance::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildTeleportEntrance::Update( CTFBot *me, float interval )
{
	#if TF_BOT_ENGINEER_SEAMS
	// If both entrance and exit exist, validate link and estimate travel delta
	CObjectTeleporter *entrance = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
	CObjectTeleporter *exit = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
	if ( entrance && exit )
	{
		entrance->UpdateLastKnownArea();
		exit->UpdateLastKnownArea();
		CTFNavArea *entrArea = (CTFNavArea *)entrance->GetLastKnownArea();
		CTFNavArea *exitArea = (CTFNavArea *)exit->GetLastKnownArea();
		float travelDelta = 0.0f;
		if ( entrArea && exitArea )
		{
			float incA = entrArea->GetIncursionDistance( me->GetTeamNumber() );
			float incB = exitArea->GetIncursionDistance( me->GetTeamNumber() );
			travelDelta = fabsf( incA - incB );
		}
		else
		{
			travelDelta = ( entrance->GetAbsOrigin() - exit->GetAbsOrigin() ).Length();
		}

		bool isValid = Seam_TeleporterIsValid( entrance, exit );
		bool shouldRedeploy = Seam_TeleporterShouldRedeploy( entrance, exit, travelDelta );

		if ( developer.GetBool() )
		{
			if ( tf_bot_engineer_seams_debug.GetBool() )
			{
				DevMsg( "[TF-ENG seam] Teleporter link check (Entrance): valid=%d redeploy=%d travelDelta=%.1f\n", (int)isValid, (int)shouldRedeploy, travelDelta );
			}
		}
	}
	#endif

	CTeamControlPoint *point = me->GetMyControlPoint();
	CCaptureZone *zone = me->GetFlagCaptureZone();
	if ( !point && !zone )
	{
		// wait until a control point becomes available
		return Continue();
	}

	CTFNavArea *myArea = me->GetLastKnownArea();

	if ( !myArea )
	{
		return Done( "No nav mesh!" );
	}

	if ( myArea->GetIncursionDistance( me->GetTeamNumber() ) > tf_bot_max_teleport_entrance_travel.GetFloat() )
	{
		return ChangeTo( new CTFBotEngineerMoveToBuild, "Too far from our spawn room to build teleporter entrance" );
	}

	// make sure we go back to our resupply cabinet after planting the teleporter entrance before we move on
	if ( !me->IsAmmoFull() && CTFBotGetAmmo::IsPossible( me ) )
	{
		return SuspendFor( new CTFBotGetAmmo, "Refilling ammo" );
	}

	CBaseObject	*myTeleportEntrance = me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
	if ( myTeleportEntrance )
	{
		// successfully built
		return ChangeTo( new CTFBotEngineerMoveToBuild, "Teleport entrance built" );
	}

	// head towards the control point and build as soon as we can
	if ( !m_path.IsValid() )
	{
		CTFBotPathCost cost( me, FASTEST_ROUTE );
		if ( point )
		{
			m_path.Compute( me, point->GetAbsOrigin(), cost );
		}
		else if ( zone )
		{
			m_path.Compute( me, zone->WorldSpaceCenter(), cost );
		}
	}

	m_path.Update( me );

	// build
	CTFWeaponBase *myGun = me->GetActiveTFWeapon();
	if ( myGun )
	{
		CTFWeaponBuilder *builder = dynamic_cast< CTFWeaponBuilder * >( myGun );
		if ( builder )
		{
			// don't build on slopes - causes player blockages
			Vector forward;
			me->EyeVectors( &forward );

			const float placementRange = 30.0f;
			forward *= placementRange;

			trace_t result;
			UTIL_TraceLine( me->WorldSpaceCenter() + Vector( forward.x, forward.y, 0.0f ), me->WorldSpaceCenter() + Vector( forward.x, forward.y, -200.0f ), MASK_PLAYERSOLID, me, COLLISION_GROUP_NONE, &result );

			if ( builder->IsValidPlacement() && result.DidHit() && result.plane.normal.z > tf_bot_teleport_build_surface_normal_limit.GetFloat() )
			{
				// place it down
				me->PressFireButton();
			}
		}
		else
		{
			// switch to teleporter builder
			me->StartBuildingObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuildTeleportEntrance::OnStuck( CTFBot *me )
{
	m_path.Invalidate();

	return TryContinue();
}
