//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_teleport_exit.cpp
// Engineer building a teleport exit
// Michael Booth, May 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_builder.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.h"
#include "bot/behavior/tf_bot_get_ammo.h"

extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit( void )
{
	m_hasPreciseBuildLocation = false;
}


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit( const Vector &buildLocation, float buildAngle )
{
	m_hasPreciseBuildLocation = true;
	m_buildLocation = buildLocation;
	m_buildAngle = buildAngle;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildTeleportExit::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	if ( !m_hasPreciseBuildLocation )
	{
		// if no specific build location given, just build right where we are
		m_buildLocation = me->GetAbsOrigin();
	}

	m_giveUpTimer.Start( 3.1f );
	m_path.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildTeleportExit::Update( CTFBot *me, float interval )
{
	if ( me->GetTimeSinceLastInjury() < 1.0f )
	{
		return Done( "Ouch! I'm under attack" );
	}

	CBaseObject	*myTeleportEntrance = me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
	if ( myTeleportEntrance )
	{
		// successfully built
		return Done( "Teleport exit built" );
	}


	// collect metal as we move to our build location
	if ( me->CanBuild( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT ) == CB_NEED_RESOURCES )
	{
		if ( m_getAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			// need more metal - get some
			m_getAmmoTimer.Start( 1.0f );
			return SuspendFor( new CTFBotGetAmmo, "Need more metal to build my Teleporter Exit" );
		}
	}

	// move near our build position
	const float buildRange = 50.0f;
	if ( me->IsRangeGreaterThan( m_buildLocation, buildRange ) )
	{
		// move into position
		if ( !m_path.IsValid() || m_repathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, m_buildLocation, cost );

			m_repathTimer.Start( RandomFloat( 2.0f, 3.0f ) );
		}

		m_path.Update( me );

		// don't give up until we've reached our build location
		m_giveUpTimer.Reset();

		return Continue();
	}

	// in position to build
	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done( "Taking too long - giving up" );
	}

	if ( m_hasPreciseBuildLocation )
	{
		me->GetBodyInterface()->AimHeadTowards( m_buildLocation, IBody::CRITICAL, 1.0f, NULL, "Looking toward my precise build location" );

		// directly create a teleporter exit at the precise position and orientation desired
		CObjectTeleporter *myTeleporterExit = (CObjectTeleporter *)CreateEntityByName( "obj_teleporter" );
		if ( myTeleporterExit )
		{
			myTeleporterExit->SetObjectMode( MODE_TELEPORTER_EXIT );
			myTeleporterExit->SetAbsOrigin( m_buildLocation );
			myTeleporterExit->SetAbsAngles( QAngle( 0, m_buildAngle, 0 ) );
			myTeleporterExit->Spawn();

			myTeleporterExit->StartPlacement( me );
			myTeleporterExit->StartBuilding( me );
			myTeleporterExit->SetBuilder( me );

			// teleporter exits are solid blockers - put engineer on top of exit or he'll be stuck
			Vector myNewOrigin = me->GetAbsOrigin();
			myNewOrigin.z += me->GetLocomotionInterface()->GetStepHeight();

			me->SetAbsOrigin( myNewOrigin );

			return Done( "Teleport exit built at precise location" );
		}

		return Continue();
	}


	// build exit roughly at this spot
	CTFWeaponBase *myGun = me->GetActiveTFWeapon();
	if ( myGun )
	{
		CTFWeaponBuilder *builder = dynamic_cast< CTFWeaponBuilder * >( myGun );
		if ( builder )
		{
			if ( builder->IsValidPlacement() )
			{
				// place it down
				me->PressFireButton();
			}
			else if ( m_searchTimer.IsElapsed() )
			{
				// rotate to find valid spot
				Vector forward;
				float angle = RandomFloat( -M_PI, M_PI );
				FastSinCos( angle, &forward.y, &forward.x );

				forward.z = 0.0f;

				me->GetBodyInterface()->AimHeadTowards( me->EyePosition() - 100.0f * forward, IBody::CRITICAL, 1.0f, NULL, "Trying to place my teleport exit" );

				m_searchTimer.Start( 1.0f );
			}
		}
		else
		{
			// switch to teleporter builder
			me->StartBuildingObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildTeleportExit::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_giveUpTimer.Reset();
	m_path.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuildTeleportExit::OnStuck( CTFBot *me )
{
	m_path.Invalidate();

	return TryContinue();
}


