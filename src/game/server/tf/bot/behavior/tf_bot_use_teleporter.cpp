//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_use_teleporter.cpp
// Ride a friendly teleporter
// Michael Booth, May 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_use_teleporter.h"

extern ConVar tf_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CTFBotUseTeleporter::CTFBotUseTeleporter( CObjectTeleporter *teleporter, UseHowType how )
{
	m_teleporter = teleporter;
	m_how = how;
	m_isInTransit = false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotUseTeleporter::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
// We could compute the time it would take to walk from the tele entrance to exit
// and compare that to the time needed to wait for the tele to be ready to send us,
// but players tend to use the tele only if it is ready NOW (or very soon).
bool CTFBotUseTeleporter::IsTeleporterAvailable( void ) const
{
	if ( m_teleporter != NULL )
	{
		if ( !m_teleporter->IsReady() )
			return false;

		if ( m_teleporter->GetState() == TELEPORTER_STATE_READY )
			return true;

/* causes massive bot pileups
		if ( m_teleporter->GetState() == TELEPORTER_STATE_SENDING ||
			 m_teleporter->GetState() == TELEPORTER_STATE_RECHARGING )
		{
			if ( m_teleporter->GetUpgradeLevel() == 3 )
			{
				// we'll wait for level 3 teleporters - they're really fast
				return true;
			}
		}
*/
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotUseTeleporter::Update( CTFBot *me, float interval )
{
	if ( m_teleporter == NULL )
	{
		return Done( "Teleporter is gone" );
	}

	CObjectTeleporter *teleporterExit = m_teleporter->GetMatchingTeleporter();
	if ( !teleporterExit )
	{
		return Done( "Missing teleporter exit" );
	}

	if ( m_teleporter->IsSendingPlayer( me ) )
	{
		// note that we have been teleported, because it takes a few frames
		// to actually relocate us, even after our teleporter leaves the "sending" state
		m_isInTransit = true;
	}

	if ( m_isInTransit )
	{
		if ( me->IsRangeLessThan( teleporterExit, 25.0f ) )
		{
			return Done( "Successful teleport" );
		}
	}
	else if ( !IsTeleporterAvailable() && m_how == USE_IF_READY )
	{
		return Done( "Teleporter is not available" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		if ( m_path.Compute( me, m_teleporter->GetAbsOrigin(), cost ) == false )
		{
			// no path to teleporter
			return Done( "Can't reach teleporter!" );
		}
	}

	// move toward the teleporter until we're standing on it
	if ( me->GetLocomotionInterface()->GetGround() != m_teleporter )
	{
		m_path.Update( me );
	}

	return Continue();
}

