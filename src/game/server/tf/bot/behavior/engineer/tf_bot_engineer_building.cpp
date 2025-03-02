//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_building.cpp
// At building location, constructing buildings
// Michael Booth, May 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "team_train_watcher.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_building.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_sentrygun.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_dispenser.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/map_entities/tf_bot_hint_teleporter_exit.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "NextBotUtil.h"


ConVar tf_bot_engineer_retaliate_range( "tf_bot_engineer_retaliate_range", "750", FCVAR_CHEAT, "If attacker who destroyed sentry is closer than this, attack. Otherwise, retreat" );
ConVar tf_bot_engineer_exit_near_sentry_range( "tf_bot_engineer_exit_near_sentry_range", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and its Teleporter Exit" );
ConVar tf_bot_engineer_max_sentry_travel_distance_to_point( "tf_bot_engineer_max_sentry_travel_distance_to_point", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and the currently contested point" );

extern ConVar tf_bot_path_lookahead_range;

const int MaxPlacementAttempts = 5;


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuilding::CTFBotEngineerBuilding( void )
{
	m_sentryBuildHint = NULL;
}


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuilding::CTFBotEngineerBuilding( CTFBotHintSentrygun *sentryBuildHint )
{
	m_sentryBuildHint = sentryBuildHint;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuilding::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_sentryTriesLeft = MaxPlacementAttempts;

	m_territoryRangeTimer.Invalidate();

	m_hasBuiltSentry = false;
	m_isSentryOutOfPosition = false;
	m_nearbyMetalStatus = NEARBY_METAL_UNKNOWN;

	return Continue();
}

//---------------------------------------------------------------------------------------------
// Return whichever teleporter is the closest if both are built. 
// Otherwise returns whichever is built or NULL if none
CObjectTeleporter* CTFBotEngineerBuilding::PickClosestValidTeleporter( CTFBot *me ) const
{
	CObjectTeleporter *myTeleporterEntrance =
		static_cast< CObjectTeleporter* >( me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE ) );
	CObjectTeleporter *myTeleporterExit =
		static_cast< CObjectTeleporter* >( me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT ) );

	if ( !myTeleporterEntrance && !myTeleporterExit ) return NULL;
	if ( myTeleporterEntrance && !myTeleporterExit ) return myTeleporterEntrance;
	if ( !myTeleporterEntrance && myTeleporterExit ) return myTeleporterExit;

	return 
		me->GetDistanceBetween( myTeleporterEntrance ) < me->GetDistanceBetween( myTeleporterExit )
		? myTeleporterEntrance : myTeleporterExit;
}

//---------------------------------------------------------------------------------------------
// Return the building currently needing upmost attention
CBaseObject* CTFBotEngineerBuilding::PickCurrentWorkTarget( CTFBot *me ) const
{
	CObjectSentrygun *mySentry = 
		static_cast< CObjectSentrygun* >( me->GetObjectOfType( OBJ_SENTRYGUN ) );
	CObjectDispenser *myDispencer =
		static_cast< CObjectDispenser* >( me->GetObjectOfType( OBJ_DISPENSER ) );
	CObjectTeleporter *myClosestTeleporter = PickClosestValidTeleporter( me );
	CObjectTeleporter *myOtherTeleporter = ( myClosestTeleporter ) ? myClosestTeleporter->GetMatchingTeleporter() : NULL;

	// Don't do anything if we do not have a sentry
	if ( !mySentry ) return NULL;

	// Only consider teleporters close enough for maintenance
	bool hasValidTeleporterCloseEnough = 
		myClosestTeleporter && 
		( mySentry->GetAbsOrigin() - myClosestTeleporter->GetAbsOrigin() )
			.IsLengthLessThan( tf_bot_engineer_exit_near_sentry_range.GetFloat() );

	// Prioritize building that has sapper on it above else
	if ( mySentry->HasSapper() || mySentry->IsPlasmaDisabled() ) 
		return mySentry;
	if ( myDispencer && ( myDispencer->HasSapper() || myDispencer->IsPlasmaDisabled() ) ) 
		return myDispencer;
	if ( hasValidTeleporterCloseEnough && ( myClosestTeleporter->HasSapper() || myClosestTeleporter->IsPlasmaDisabled() ) ) 
		return myClosestTeleporter;
	// Prioritize sentry if it's damaged
	if ( mySentry->GetTimeSinceLastInjury() < 1.0f || mySentry->GetHealth() < mySentry->GetMaxHealth() )
		return mySentry;
	// ... sentry or dispencer that are currently being built
	if ( mySentry->IsBuilding() )
		return mySentry;
	if ( myDispencer && myDispencer->IsBuilding() )
		return myDispencer;
	// ... sentry that is not upgraded
	if ( mySentry->GetUpgradeLevel() < 3 )
		return mySentry;
	// ... damaged dispencer or either of the teleporters
	if ( myDispencer && myDispencer->GetHealth() < myDispencer->GetMaxHealth() )
		return myDispencer;
	if ( hasValidTeleporterCloseEnough && ( 
		( myClosestTeleporter->GetHealth() < myClosestTeleporter->GetMaxHealth() ) || 
		( myOtherTeleporter && ( ( myOtherTeleporter->GetMaxHealth() - myOtherTeleporter->GetHealth() ) > 1.0f ) )
		// I can't beleive that of all building teleporter consistently gets considered injured due float imprecisions on health values,
		// which is why it is needed to check if the difference is bigger than 1 rathen than just comparing them directly
	) )
		return myClosestTeleporter;
	// ... dispencer that is not upgraded
	if ( myDispencer && myDispencer->GetUpgradeLevel() < mySentry->GetUpgradeLevel() )
		return myDispencer;
	if ( hasValidTeleporterCloseEnough && myClosestTeleporter->GetUpgradeLevel() < mySentry->GetUpgradeLevel() )
		return myClosestTeleporter;

	// Just keep whacking the sentry if nothing specifically wrong with any other building
	return mySentry;


	// Old code for reference
	/*
	CBaseObject* workTarget = mySentry;

	if (mySentry->HasSapper() || mySentry->IsPlasmaDisabled())
		workTarget = mySentry;
	else if (myDispenser->HasSapper() || myDispenser->IsPlasmaDisabled())
		workTarget = myDispenser;
	else if (mySentry->GetTimeSinceLastInjury() < 1.0f || mySentry->GetHealth() < mySentry->GetMaxHealth())
		workTarget = mySentry;
	else if (mySentry->IsBuilding())
		workTarget = mySentry;
	else if (myDispenser->IsBuilding())
		workTarget = myDispenser;
	else if (mySentry->GetUpgradeLevel() < 3)
		workTarget = mySentry;
	else if (myDispenser->GetHealth() < myDispenser->GetMaxHealth())
		workTarget = myDispenser;
	else if (myDispenser->GetUpgradeLevel() < mySentry->GetUpgradeLevel())
		workTarget = myDispenser;
	*/
}

//---------------------------------------------------------------------------------------------
// Pick a spot where engineer should be safe enough to work on a building
Vector CTFBotEngineerBuilding::PickIdealWorkSpot( CTFBot *me, CBaseObject *workTarget ) const
{
	if ( workTarget->GetType() == OBJ_SENTRYGUN || workTarget->GetType() == OBJ_DISPENSER )
	{
		// When maintaining a sentry or a dispencer, engineer should sit inbetween them
		CBaseObject *sentry = NULL, *dispencer = NULL;
		if ( workTarget->GetType() == OBJ_SENTRYGUN )
		{
			sentry = workTarget;
			dispencer = me->GetObjectOfType( OBJ_DISPENSER );
		}
		else
		{
			dispencer = workTarget;
			sentry = me->GetObjectOfType( OBJ_SENTRYGUN );
		}

		// Only do this if both buildings actually exist
		if ( sentry && dispencer )
			return ( sentry->GetAbsOrigin() + dispencer->GetAbsOrigin() ) / 2.0f;
	}

	return workTarget->GetAbsOrigin();
}

//---------------------------------------------------------------------------------------------
// Whether or not engineer is too far from it's work target
// true if the engineer is close enought to perform maintenance
// false if the engineer still need to navigate to it's maintenance target
// Additionally has 'shouldBeDucking' that should be set to true if engineer needs to duck
bool CTFBotEngineerBuilding::IsInPositionToWork( 
	CTFBot *me, 
	CBaseObject *workTarget, 
	bool& shouldBeDucking, 
	const Vector* idealPosition
) const {
	const float tooFarRange = 75.0f;
	
	float curDistanceToPosition = 
		me->GetDistanceBetween( ( idealPosition != NULL ) ? *idealPosition : PickIdealWorkSpot( me, workTarget ) );

	// Should duck not only to cover but also to approach the target more precisely
	shouldBeDucking = curDistanceToPosition < 1.2f * tooFarRange;

	if ( curDistanceToPosition > tooFarRange ) return false;
	
	if ( workTarget->GetType() == OBJ_SENTRYGUN || workTarget->GetType() == OBJ_DISPENSER )
	{
		// When maintaining a sentry or a dispencer, engineer should sit inbetween them
		CBaseObject *sentry = NULL, *dispencer = NULL;
		if ( workTarget->GetType() == OBJ_SENTRYGUN )
		{
			sentry = workTarget;
			dispencer = me->GetObjectOfType( OBJ_DISPENSER );
		}
		else
		{
			dispencer = workTarget;
			sentry = me->GetObjectOfType( OBJ_SENTRYGUN );
		}

		// Only do this if both buildings actually exist
		if (sentry && dispencer)
		{
			// ... and 'inbetween' means the difference between distances to 
			// engineer and both his buildings should be about equal
			const float equalityTolerance = 25.0f;

			const float sentryDistance = me->GetDistanceBetween( sentry );
			const float dispDistance = me->GetDistanceBetween( dispencer );

			if ( fabs( sentryDistance - dispDistance ) > equalityTolerance ) return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------
// Proccesses navigation towards ideal working spot
void CTFBotEngineerBuilding::NavigateToWorkingSpot( CTFBot* me, const Vector& spot )
{
	if ( m_repathTimer.IsElapsed() )
	{
		// Periodically recalculate the path in case if our building... teleports... I guess...
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, spot, cost );
	}

	// Go to building
	m_path.Update( me );
}

//---------------------------------------------------------------------------------------------
// Proccesses maintaining buildings
void CTFBotEngineerBuilding::MaintainBuilding( CTFBot *me, CBaseObject *workTarget )
{
	m_searchTimer.Invalidate();

	// we are in position - work on our buildings
	me->StopLookingAroundForEnemies();
	me->GetBodyInterface()->AimHeadTowards( LookAtPointOnWorkTarget( me, workTarget ), IBody::CRITICAL, 1.0f, NULL, "Work on my buildings" );
	me->PressFireButton();
}

//---------------------------------------------------------------------------------------------
// Returns a point on the work target which engineers should be looking at 
// when they maintain their buildings 
const Vector& CTFBotEngineerBuilding::LookAtPointOnWorkTarget( CTFBot *me, CBaseObject *workTarget ) const
{
	return workTarget->WorldSpaceCenter();
}

//---------------------------------------------------------------------------------------------
// Everything is built, upgrade/maintain it
// TODO: Upgrade/maintain nearby friendly buildings, too.
void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings( CTFBot *me )
{
	CBaseObject *workTarget = PickCurrentWorkTarget( me );
	if ( !workTarget ) return;

	CBaseCombatWeapon *wrench = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( wrench )
	{
		me->Weapon_Switch( wrench );
	}

	Vector spot = PickIdealWorkSpot( me, workTarget );
	bool shouldBeDucking;
	if ( IsInPositionToWork( me, workTarget, shouldBeDucking, &spot ) )
		MaintainBuilding( me, workTarget );
	else
		NavigateToWorkingSpot( me, spot );

	if ( shouldBeDucking )
		me->PressCrouchButton();
}


//---------------------------------------------------------------------------------------------
bool CTFBotEngineerBuilding::IsMetalSourceNearby( CTFBot *me ) const
{
	CUtlVector< CNavArea * > nearbyVector;
	CollectSurroundingAreas( &nearbyVector, me->GetLastKnownArea(), 2000.0f, me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetStepHeight() );

	for( int i=0; i<nearbyVector.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)nearbyVector[i];
		if ( area->HasAttributeTF( TF_NAV_HAS_AMMO ) )
		{
			return true;
		}

		// this assumes all spawn rooms have resupply cabinets
		if ( me->GetTeamNumber() == TF_TEAM_RED && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) )
		{
			return true;
		}

		if ( me->GetTeamNumber() == TF_TEAM_BLUE && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition( CTFBot *me ) const
{
	// Re-evaluate if MvM ever needs something more dynamic
	if ( TFGameRules()->IsPVEModeActive() )
		return false;

	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );

	if ( !mySentry )
	{
		return false;
	}

	// payload
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *trainWatcher;

		if ( me->GetTeamNumber() == TF_TEAM_BLUE )
		{
			trainWatcher = TFGameRules()->GetPayloadToPush( me->GetTeamNumber() );
		}
		else
		{
			trainWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		}

		if ( trainWatcher )
		{
			float sentryDistanceAlongPath;
			trainWatcher->ProjectPointOntoPath( mySentry->GetAbsOrigin(), NULL, &sentryDistanceAlongPath );

			const float behindTrainTolerance = SENTRY_MAX_RANGE;
			return ( trainWatcher->GetTrainDistanceAlongTrack() > sentryDistanceAlongPath + behindTrainTolerance );
		}
	}

	// control points
	mySentry->UpdateLastKnownArea();
	CNavArea *sentryArea = mySentry->GetLastKnownArea();

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point )
	{
		CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );

		if ( sentryArea && pointArea )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			if ( NavAreaTravelDistance( sentryArea, pointArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0 &&
				 NavAreaTravelDistance( pointArea, sentryArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0 )
			{
				return true;
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuilding::Update( CTFBot *me, float interval )
{
	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	CObjectDispenser *myDispenser = (CObjectDispenser *)me->GetObjectOfType( OBJ_DISPENSER );
	CObjectTeleporter *myTeleportEntrance = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
	CObjectTeleporter *myTeleportExit = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );

	bool isUnderAttack = ( me->GetTimeSinceLastInjury() < 1.0f );
	isUnderAttack |= ( mySentry && ( mySentry->HasSapper() || mySentry->IsPlasmaDisabled() ) );
	isUnderAttack |= ( myDispenser && ( myDispenser->HasSapper() || myDispenser->IsPlasmaDisabled() ) );

	me->StartLookingAroundForEnemies();

	// try to build a Sentry
	if ( !mySentry )
	{
		m_nearbyMetalStatus = NEARBY_METAL_UNKNOWN;

		// react to nearby threats if our sentry is down
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && threat->IsVisibleRecently() )
		{
			me->EquipBestWeaponForThreat( threat );
		}

		if ( !m_hasBuiltSentry && m_sentryTriesLeft > 0 )
		{
			--m_sentryTriesLeft;

			if ( m_sentryBuildHint )
			{
				return SuspendFor( new CTFBotEngineerBuildSentryGun( m_sentryBuildHint ), "Building a Sentry at a hint location" );
			}

			return SuspendFor( new CTFBotEngineerBuildSentryGun, "Building a Sentry" );
		}
		else
		{
			// can't build a Sentry here - pick a new place
			return ChangeTo( new CTFBotEngineerMoveToBuild, "Couldn't find a place to build" );
		}
	}

	// I have a Sentry
	m_hasBuiltSentry = true;

	if ( m_sentryBuildHint != NULL && !m_sentryBuildHint->IsEnabled() )
	{
		// our hint has been disabled and no longer has influence on our behavior
		m_sentryBuildHint = NULL;
	}

	// periodically check that our Sentry is still near the contested point
	if ( m_sentryBuildHint == NULL || !m_sentryBuildHint->IsSticky() )
	{
		if ( !m_isSentryOutOfPosition && m_territoryRangeTimer.IsElapsed() )
		{
			m_territoryRangeTimer.Start( RandomFloat( 3.0f, 5.0f ) );

			m_isSentryOutOfPosition = CheckIfSentryIsOutOfPosition( me );
		}

		if ( m_isSentryOutOfPosition )
		{
			// the point has moved, only keep sentry as long as it keeps attacking
			if ( mySentry->GetTimeSinceLastFired() > 10.0f )
			{
				mySentry->DetonateObject();

				// if we built here because of a hint, disable that hint so we don't use it and rebuild here again
				if ( m_sentryBuildHint != NULL )
				{
					inputdata_t dummy;
					m_sentryBuildHint->InputDisable( dummy );

					m_sentryBuildHint = NULL;
				}

				if ( myDispenser )
				{
					myDispenser->DetonateObject();
				}

				if ( myTeleportExit )
				{
					myTeleportExit->DetonateObject();
				}

				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_MOVEUP );

				return ChangeTo( new CTFBotEngineerMoveToBuild, "Need to move my gear closer to the point!" );
			}
		}
	}

	// if my dispenser is too far away from my sentry, destroy and rebuild it next update
	// @TODO: Flag hint-built entities for a larger range
	const float maxSeparation = 500.0f;
	if ( myDispenser )
	{
		if ( ( mySentry->GetAbsOrigin() - myDispenser->GetAbsOrigin() ).IsLengthGreaterThan( maxSeparation ) )
		{
			myDispenser->DestroyObject();
			myDispenser = NULL;
		}
	}

	// build up the sentry all the way if there is a metal source nearby
	if ( mySentry->GetUpgradeLevel() < 3 )
	{
		if ( m_nearbyMetalStatus == NEARBY_METAL_UNKNOWN )
		{
			m_nearbyMetalStatus = IsMetalSourceNearby( me ) ? NEARBY_METAL_EXISTS : NEARBY_METAL_NONE;
		}

		if ( m_nearbyMetalStatus == NEARBY_METAL_EXISTS )
		{
			UpgradeAndMaintainBuildings( me );
			return Continue();
		}
	}

/*
	if ( myTeleportExit )
	{
		// if my teleporter exit is too far away from my sentry, destroy and rebuild it next update
		if ( ( mySentry->GetAbsOrigin() - myTeleportExit->GetAbsOrigin() ).IsLengthGreaterThan( maxSeparation ) )
		{
			myTeleportExit->DestroyObject();
			myTeleportExit = NULL;
		}
	}
*/

	// try to build a Dispenser (build after tele exit in training)
	if ( !TFGameRules()->IsInTraining() || myTeleportExit )
	{
		const float dispenserRebuildInterval = 10.0f;
		if ( myDispenser )
		{
			// don't rebuild immediately after building is destroyed
			m_dispenserRetryTimer.Start( dispenserRebuildInterval );
		}
		else if ( m_dispenserRetryTimer.IsElapsed() && !isUnderAttack )
		{
			m_dispenserRetryTimer.Start( dispenserRebuildInterval );

			return SuspendFor( new CTFBotEngineerBuildDispenser, "Building a Dispenser" );
		}
	}

	// try to build a Teleporter Exit
	const float exitRebuildInterval = TFGameRules()->IsInTraining() ? 5.0f : 30.0f;
	if ( myTeleportExit )
	{
		// don't rebuild immediately after building is destroyed
		m_teleportExitRetryTimer.Start( exitRebuildInterval );
	}
	else if ( m_teleportExitRetryTimer.IsElapsed() && myTeleportEntrance && !isUnderAttack )
	{
		m_teleportExitRetryTimer.Start( exitRebuildInterval );

		// we need to build a teleporter exit yet
		if ( m_sentryBuildHint != NULL )
		{
			// if there are teleporter exit hints, find the closest one to our sentry and use it
			CUtlVector< CBaseEntity * > hintVector;
			CTFBotHintTeleporterExit *hint = NULL;
			while( ( hint = (CTFBotHintTeleporterExit *)gEntList.FindEntityByClassname( hint, "bot_hint_teleporter_exit" ) ) != NULL )
			{
				if ( hint->IsEnabled() && hint->InSameTeam( me ) )
				{
					hintVector.AddToTail( hint );
				}
			}

			if ( hintVector.Count() > 0 )
			{
				mySentry->UpdateLastKnownArea();
				CBaseEntity *closeHint = SelectClosestEntityByTravelDistance( me, hintVector, mySentry->GetLastKnownArea(), tf_bot_engineer_exit_near_sentry_range.GetFloat() );

				if ( closeHint )
				{
					return SuspendFor( new CTFBotEngineerBuildTeleportExit( closeHint->GetAbsOrigin(), closeHint->GetAbsAngles().y ), "Building teleporter exit at nearby hint" );
				}
			}
		}
		else if ( me->IsRangeLessThan( mySentry, 300.0f ) )
		{
			// drop a teleporter exit near our sentry
			return SuspendFor( new CTFBotEngineerBuildTeleportExit(), "Building teleporter exit" );
		}
	}

	// everything is built - maintain them
	UpgradeAndMaintainBuildings( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotEngineerBuilding::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->StartLookingAroundForEnemies();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEngineerBuilding::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuilding::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuilding::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryContinue();
}
