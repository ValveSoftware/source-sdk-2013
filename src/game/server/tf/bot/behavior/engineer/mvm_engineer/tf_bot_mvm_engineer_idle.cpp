//========= Copyright Valve Corporation, All rights reserved. ============//
// Michael Booth, September 2012

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_teleporter.h"
#include "bot/tf_bot.h"
#include "bot/map_entities/tf_bot_hint_engineer_nest.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "bot/map_entities/tf_bot_hint_teleporter_exit.h"

#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_sentry.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_teleporter.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_teleport_spawn.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"


ConVar tf_bot_engineer_mvm_sentry_hint_bomb_forward_range( "tf_bot_engineer_mvm_sentry_hint_bomb_forward_range", "0", FCVAR_CHEAT );
ConVar tf_bot_engineer_mvm_sentry_hint_bomb_backward_range( "tf_bot_engineer_mvm_sentry_hint_bomb_backward_range", "3000", FCVAR_CHEAT );
ConVar tf_bot_engineer_mvm_hint_min_distance_from_bomb( "tf_bot_engineer_mvm_hint_min_distance_from_bomb", "1300", FCVAR_CHEAT );

struct BombInfo_t
{
	Vector m_vPosition;
	float m_flMinBattleFront;
	float m_flMaxBattleFront;
};


bool GetBombInfo( BombInfo_t* pBombInfo = NULL )
{
	// find the incursion distance of the current "front" (the location of the bomb)

	// first find farthest bomb delivery distance of invading team since maps
	// have different spawn room sizes and geometries
	float battlefront = 0.0f;

	for( int n=0; n<TheNavAreas.Count(); ++n )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[n];

		if ( area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
		{
			continue;
		}

		float areaDistanceToTarget = area->GetTravelDistanceToBombTarget();
		if ( areaDistanceToTarget > battlefront && areaDistanceToTarget > 0.0f )
		{
			battlefront = areaDistanceToTarget;
		}
	}


	// find the travel distance from the bomb to the delivery target and use it as the front
	CCaptureFlag *flag = NULL;
	Vector vBombSpot(0, 0, 0);
	for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
	{
		CCaptureFlag *pTempFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
		Vector vTempBombSpot;
		CTFPlayer *carrier = ToTFPlayer( pTempFlag->GetOwnerEntity() );
		if ( carrier )
		{
			vTempBombSpot = carrier->GetAbsOrigin();
		}
		else
		{
			vTempBombSpot = pTempFlag->WorldSpaceCenter();
		}

		CTFNavArea *flagArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( vTempBombSpot, false, 1000.0f );
		if ( flagArea )
		{
			float flagDistanceToTarget = flagArea->GetTravelDistanceToBombTarget();

			if ( flagDistanceToTarget < battlefront && flagDistanceToTarget >= 0.0f )
			{
				battlefront = flagDistanceToTarget;
				flag = pTempFlag;
				vBombSpot = vTempBombSpot;
			}
		}
	}

	float flMaxBattlefront = battlefront + tf_bot_engineer_mvm_sentry_hint_bomb_backward_range.GetFloat();
	float flMinBattlefront = battlefront - tf_bot_engineer_mvm_sentry_hint_bomb_forward_range.GetFloat();

	if ( pBombInfo )
	{
		pBombInfo->m_vPosition = vBombSpot;
		pBombInfo->m_flMinBattleFront = flMinBattlefront;
		pBombInfo->m_flMaxBattleFront = flMaxBattlefront;
	}

	return flag ? true : false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerIdle::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->StopLookingAroundForEnemies();

	m_sentryHint = NULL;
	m_teleporterHint = NULL;
	m_nestHint = NULL;
	m_nTeleportedCount = 0;
	m_bTeleportedToHint = false;
	m_bTriedToDetonateStaleNest = false;

	return Continue();
}


void CTFBotMvMEngineerIdle::TakeOverStaleNest( CBaseTFBotHintEntity* pHint, CTFBot *me )
{
	if ( pHint != NULL && pHint->OwnerObjectHasNoOwner() )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( pHint->GetOwnerEntity() );
		pObj->SetOwnerEntity( me );
		pObj->SetBuilder( me );
		me->AddObject( pObj );
	}
}


bool CTFBotMvMEngineerIdle::ShouldAdvanceNestSpot( CTFBot *me )
{
	if ( !m_nestHint )
	{
		return false;
	}

	if ( !m_reevaluateNestTimer.HasStarted() )
	{
		m_reevaluateNestTimer.Start( 5.f );
		return false;
	}

	for ( int i=0; i<me->GetObjectCount(); ++i )
	{
		CBaseObject *pObj = me->GetObject( i );
		if ( pObj && pObj->GetHealth() < pObj->GetMaxHealth() )
		{
			// if the nest is under attack, don't advance the nest
			m_reevaluateNestTimer.Start( 5.f );
			return false;
		}
	}

	if ( m_reevaluateNestTimer.IsElapsed() )
	{
		m_reevaluateNestTimer.Invalidate();
	}

	BombInfo_t bombInfo;
	if ( GetBombInfo( &bombInfo ) )
	{
		if ( m_nestHint )
		{
			CTFNavArea *hintArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( m_nestHint->GetAbsOrigin(), false, 1000.0f );
			if ( hintArea )
			{
				float hintDistanceToTarget = hintArea->GetTravelDistanceToBombTarget();

				bool bShouldAdvance = ( hintDistanceToTarget > bombInfo.m_flMaxBattleFront );

				return bShouldAdvance;
			}
		}
	}

	return false;
}


void CTFBotMvMEngineerIdle::TryToDetonateStaleNest()
{
	if ( m_bTriedToDetonateStaleNest )
		return;

	// wait until the engy finish building his nest
	if ( ( m_sentryHint && !m_sentryHint->OwnerObjectFinishBuilding() ) ||
		 ( m_teleporterHint && !m_teleporterHint->OwnerObjectFinishBuilding() ) )
		return;

	// collect all existing and active teleporter hints
	CUtlVector< CTFBotHintEngineerNest* > activeEngineerNest;
	for ( int i=0; i<ITFBotHintEntityAutoList::AutoList().Count(); ++i )
	{
		CBaseTFBotHintEntity *pHint = static_cast<CBaseTFBotHintEntity*>( ITFBotHintEntityAutoList::AutoList()[i] );
		if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_ENGINEER_NEST ) && pHint->IsEnabled() && pHint->GetOwnerEntity() == NULL )
		{
			activeEngineerNest.AddToTail( static_cast< CTFBotHintEngineerNest* >( pHint ) );
		}
	}

	// try to detonate stale nest that's out of range, when engineer finished building his nest
	for ( int i=0; i<activeEngineerNest.Count(); ++i )
	{
		CTFBotHintEngineerNest *pNest = activeEngineerNest[i];
		if ( pNest->IsStaleNest() )
		{
			pNest->DetonateStaleNest();
		}
	}

	m_bTriedToDetonateStaleNest = true;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMEngineerIdle::Update( CTFBot *me, float interval )
{
	if ( !me->IsAlive() )
	{
		// don't do anything when I'm dead
		return Done();
	}

	// Always equip my wrench
	CBaseCombatWeapon *wrench = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( wrench )
	{
		me->Weapon_Switch( wrench );
	}

	if ( m_nestHint == NULL || ShouldAdvanceNestSpot( me ) )
	{
		if ( m_findHintTimer.HasStarted() && !m_findHintTimer.IsElapsed() )
		{
			// too soon
			return Continue();
		}

		m_findHintTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		// figure out where to teleport into the map
		bool bShouldTeleportToHint = me->HasAttribute( CTFBot::TELEPORT_TO_HINT );
		bool bShouldCheckForBlockingObject = !m_bTeleportedToHint && bShouldTeleportToHint;
		CHandle< CTFBotHintEngineerNest > newNest = NULL;
		if ( !CTFBotMvMEngineerHintFinder::FindHint( bShouldCheckForBlockingObject, !bShouldTeleportToHint, &newNest ) )
		{
			// try again next time
			return Continue();
		}

		// unown the old nest
		if ( m_nestHint )
		{
			m_nestHint->SetOwnerEntity( NULL );
		}

		m_nestHint = newNest;
		m_nestHint->SetOwnerEntity( me );
		m_sentryHint = m_nestHint->GetSentryHint();
		TakeOverStaleNest( m_sentryHint, me );

		if ( me->GetTeleportWhere().Count() > 0 )
		{
			m_teleporterHint = m_nestHint->GetTeleporterHint();
			TakeOverStaleNest( m_teleporterHint, me );
		}
	}

	if ( !m_bTeleportedToHint && me->HasAttribute( CTFBot::TELEPORT_TO_HINT ) )
	{
		m_nTeleportedCount++;
		bool bFirstTeleportSpawn = m_nTeleportedCount == 1;
		m_bTeleportedToHint = true;
		return SuspendFor( new CTFBotMvMEngineerTeleportSpawn( m_nestHint, bFirstTeleportSpawn ), "In spawn area - teleport to the teleporter hint" );
	}

	const float rebuildInterval = 3.0f;
	CObjectSentrygun *mySentry = NULL;
	if ( m_sentryHint )
	{
		if ( m_sentryHint->GetOwnerEntity() && m_sentryHint->GetOwnerEntity()->IsBaseObject() )
		{
			mySentry = assert_cast< CObjectSentrygun* >( m_sentryHint->GetOwnerEntity() );
		}

		if ( mySentry )
		{
			// force an interval between sentry being destroyed and me trying to rebuild it
			m_sentryRebuildTimer.Start( rebuildInterval );
		}
		else
		{
			// check if there's a stale object on the hint
			if ( m_sentryHint->GetOwnerEntity() && m_sentryHint->GetOwnerEntity()->IsBaseObject() )
			{
				mySentry = assert_cast< CObjectSentrygun* >( m_sentryHint->GetOwnerEntity() );		
				me->AddObject( mySentry );
				mySentry->SetOwnerEntity( me );
			}
			else
			{
				if ( m_sentryRebuildTimer.IsElapsed() )
				{
					return SuspendFor( new CTFBotMvMEngineerBuildSentryGun( m_sentryHint ), "No sentry - building a new one" );
				}
				else
				{
					// run away!
					return SuspendFor( new CTFBotRetreatToCover( 1.0f ), "Lost my sentry - retreat!" );
				}
			}
		}
	}

	if ( mySentry && mySentry->GetHealth() < mySentry->GetMaxHealth() && !mySentry->IsBuilding() )
	{
		// track when sentry was last hurt
		m_sentryInjuredTimer.Start( 3.0f );
	}

	
	CObjectTeleporter *myTeleporter = NULL;
	if ( m_teleporterHint && m_sentryInjuredTimer.IsElapsed() )
	{
		if ( m_teleporterHint->GetOwnerEntity() && m_teleporterHint->GetOwnerEntity()->IsBaseObject() )
		{
			// force an interval between teleporter being destroyed and me trying to rebuild it
			myTeleporter = assert_cast< CObjectTeleporter* >( m_teleporterHint->GetOwnerEntity() );
			m_teleporterRebuildTimer.Start( rebuildInterval );
		}
		else if ( m_teleporterRebuildTimer.IsElapsed() )
		{
			return SuspendFor( new CTFBotMvMEngineerBuildTeleportExit( m_teleporterHint ), "Sentry is safe - building a teleport exit" );
		}
	}

	// fix teleporter if sentry is not hurt
	if ( myTeleporter && m_sentryInjuredTimer.IsElapsed() && myTeleporter->GetHealth() < myTeleporter->GetMaxHealth() && !myTeleporter->IsBuilding() )
	{
		float rangeToTeleporter = me->GetDistanceBetween( myTeleporter );

		const float nearTeleporterRange = 75.0f;

		if ( rangeToTeleporter < 1.2f * nearTeleporterRange )
		{
			// crouch as I get close
			me->PressCrouchButton();
		}

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			Vector toTeleporter = myTeleporter->GetAbsOrigin() - me->GetAbsOrigin();
			Vector hittingTeleporterSpot = myTeleporter->GetAbsOrigin() - 50.0f * toTeleporter.Normalized();

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_path.Compute( me, hittingTeleporterSpot, cost );
		}

		m_path.Update( me );

		if ( rangeToTeleporter < nearTeleporterRange )
		{
			// we are in position - hit sentry with wrench
			me->GetBodyInterface()->AimHeadTowards( myTeleporter->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Work on my Teleporter" );
			me->PressFireButton();
		}
	}
	else if ( mySentry )
	{
		float rangeToSentry = me->GetDistanceBetween( mySentry );

		const float nearSentryRange = 75.0f;

		if ( rangeToSentry < 1.2f * nearSentryRange )
		{
			// crouch as I get close
			me->PressCrouchButton();
		}

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			Vector mySentryForward;
			AngleVectors( mySentry->GetTurretAngles(), &mySentryForward );

			Vector behindSentrySpot = mySentry->GetAbsOrigin() - 50.0f * mySentryForward;

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_path.Compute( me, behindSentrySpot, cost );
		}

		m_path.Update( me );

		if ( rangeToSentry < nearSentryRange )
		{
			// we are in position - hit sentry with wrench
			me->GetBodyInterface()->AimHeadTowards( mySentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Work on my Sentry" );
			me->PressFireButton();
		}
	}

	TryToDetonateStaleNest();

	return Continue();
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotMvMEngineerIdle::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotMvMEngineerIdle::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotMvMEngineerIdle::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


CTFBotHintEngineerNest* SelectOutOfRangeNest( const CUtlVector< CTFBotHintEngineerNest* >& nestVector )
{
	if ( nestVector.Count() )
	{
		for ( int i=0; i<nestVector.Count(); ++i )
		{
			if ( nestVector[i]->IsStaleNest() )
			{
				return nestVector[i];
			}
		}

		int which = RandomInt( 0, nestVector.Count() - 1 );
		return nestVector[which];
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
bool CTFBotMvMEngineerHintFinder::FindHint( bool bShouldCheckForBlockingObjects, bool bAllowOutOfRangeNest, CHandle< CTFBotHintEngineerNest >* pFoundNest /*= NULL*/ )
{
	// collect all existing and active teleporter hints
	CUtlVector< CTFBotHintEngineerNest* > activeEngineerNest;
	for ( int i=0; i<ITFBotHintEntityAutoList::AutoList().Count(); ++i )
	{
		CBaseTFBotHintEntity *pHint = static_cast<CBaseTFBotHintEntity*>( ITFBotHintEntityAutoList::AutoList()[i] );
		if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_ENGINEER_NEST ) && pHint->IsEnabled() && pHint->GetOwnerEntity() == NULL )
		{
			activeEngineerNest.AddToTail( static_cast< CTFBotHintEngineerNest* >( pHint ) );
		}
	}

	if ( activeEngineerNest.Count() == 0 )
	{
		if ( pFoundNest )
		{
			*pFoundNest = NULL;
		}

		return false;
	}

	BombInfo_t bombInfo;
	GetBombInfo( &bombInfo );

	CUtlVector< CTFBotHintEngineerNest* > forwardOutOfRangeHintVector;
	CUtlVector< CTFBotHintEngineerNest* > backwardOutOfRangeHintVector;

	CUtlVector< CTFBotHintEngineerNest* > freeAtFrontHintVector;
	CUtlVector< CTFBotHintEngineerNest* > staleAtFrontHintVector;
	for( int i=0; i<activeEngineerNest.Count(); ++i )
	{
		CTFBotHintEngineerNest* pCurrentNest = activeEngineerNest[i];
		const Vector& vNestPosition = pCurrentNest->GetAbsOrigin();
		CTFNavArea *hintArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( vNestPosition, false, 1000.0f );
		if ( !hintArea )
		{
			Warning( "Sentry hint has NULL nav area!\n" );
			continue;
		}


		float hintDistanceToTarget = hintArea->GetTravelDistanceToBombTarget();
		if ( hintDistanceToTarget > bombInfo.m_flMinBattleFront && hintDistanceToTarget < bombInfo.m_flMaxBattleFront )
		{
			if ( bShouldCheckForBlockingObjects )
			{
				// check for blocking players and objects
				CBaseEntity *pList[256];
				int count = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), vNestPosition + VEC_HULL_MIN, vNestPosition + VEC_HULL_MAX, FL_CLIENT|FL_OBJECT );
				if ( count > 0 )
				{
					continue;
				}
			}

			// this hint is in range of the front
			if ( pCurrentNest->IsStaleNest() )
			{
				// some dead engineer was here and left his object(s) behind. I should take over
				staleAtFrontHintVector.AddToTail( pCurrentNest );
			}
			else
			{
				if ( VectorLength( bombInfo.m_vPosition - vNestPosition ) < tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat() )
				{
					// the hint is too close to the bomb, don't go there
					continue;
				}
				// this hint is also unowned
				freeAtFrontHintVector.AddToTail( pCurrentNest );
			}
		}
		else if ( hintDistanceToTarget > bombInfo.m_flMaxBattleFront )
		{
			forwardOutOfRangeHintVector.AddToTail( pCurrentNest );
		}
		else
		{
			backwardOutOfRangeHintVector.AddToTail( pCurrentNest );
		}
	}

	CTFBotHintEngineerNest *hint = NULL;
	if ( freeAtFrontHintVector.Count() == 0 && staleAtFrontHintVector.Count() == 0 )
	{
		if ( bAllowOutOfRangeNest )
		{
			// try to advance forward before falling backward
			hint = SelectOutOfRangeNest( forwardOutOfRangeHintVector );
			if ( !hint )
			{
				hint = SelectOutOfRangeNest( backwardOutOfRangeHintVector );
			}
		}

		// no hints are in range, or they are all in use
		if ( pFoundNest )
		{
			*pFoundNest = hint;
		}
	}
	else
	{
		// try to pick stale nest in range first
		if ( staleAtFrontHintVector.Count() )
		{
			int whichHint = RandomInt( 0, staleAtFrontHintVector.Count()-1 );
			hint = staleAtFrontHintVector[ whichHint ];
		}
		// if I didn't find any stale nest, try to find a free one
		else if ( freeAtFrontHintVector.Count() )
		{
			int whichHint = RandomInt( 0, freeAtFrontHintVector.Count()-1 );
			hint = freeAtFrontHintVector[ whichHint ];
		}

		if ( pFoundNest )
		{
			*pFoundNest = hint;
		}
	}

	return hint != NULL;
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_mvm_show_engineer_hint_region, "Show the nav areas MvM engineer bots will consider when selecting sentry and teleporter hints", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return; 

	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	trace_t result;
	Vector forward;
	pPlayer->EyeVectors( &forward );

	UTIL_TraceLine( pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * 10000.0f, MASK_SOLID,
		pPlayer, COLLISION_GROUP_NONE, &result );

	float flDrawTime = 5.0f;

	if ( result.DidHit() )
	{
		CTFNavArea *area = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( result.endpos );

		if ( area )
		{
			float battlefront = area->GetTravelDistanceToBombTarget();

			float maxBattlefront = battlefront + tf_bot_engineer_mvm_sentry_hint_bomb_backward_range.GetFloat();
			float minBattlefront = battlefront - tf_bot_engineer_mvm_sentry_hint_bomb_forward_range.GetFloat();

			CUtlVector< CTFNavArea * > battlefrontAreaVector;
			TheTFNavMesh()->CollectAreaWithinBombTravelRange( &battlefrontAreaVector, minBattlefront, maxBattlefront );

			CUtlVector< CTFNavArea * > hintAreaVector;
			for ( int i=0; i<ITFBotHintEntityAutoList::AutoList().Count(); ++i )
			{
				CBaseTFBotHintEntity *pHint = static_cast< CBaseTFBotHintEntity* >( ITFBotHintEntityAutoList::AutoList()[i] );
				hintAreaVector.AddToTail( (CTFNavArea*)TheNavMesh->GetNearestNavArea( pHint ) );
			}

			for( int i=0; i<battlefrontAreaVector.Count(); ++i )
			{
				CTFNavArea *fillArea = battlefrontAreaVector[i];

				if ( fillArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) || fillArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) )
				{
					continue;
				}

				fillArea->DrawFilled( 255, 100, 0, 0, flDrawTime );

				for ( int j=0; j<hintAreaVector.Count(); ++j )
				{
					if ( fillArea == hintAreaVector[j] )
					{
						CBaseTFBotHintEntity *pHint = static_cast< CBaseTFBotHintEntity* >( ITFBotHintEntityAutoList::AutoList()[j] );
						Color color;
						if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_SENTRYGUN ) )
						{
							color = Color( 0, 255, 0 );
						}
						else if ( pHint->IsHintType( CBaseTFBotHintEntity::HINT_TELEPORTER_EXIT ) )
						{
							color = Color( 0, 0, 255 );
						}
						else
						{
							bool bTooCloseToBomb = VectorLength( result.endpos - pHint->GetAbsOrigin() ) < tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat();
							color = bTooCloseToBomb ? Color( 255, 0, 0 ) : Color( 255, 255, 0 );
						}
						NDebugOverlay::Sphere( pHint->GetAbsOrigin(), 50, color.r(), color.g(), color.b(), true, flDrawTime );
					}
				}
			}

			NDebugOverlay::Sphere( result.endpos, tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat(), 255, 255, 0, false, flDrawTime );
		}
	}
}
