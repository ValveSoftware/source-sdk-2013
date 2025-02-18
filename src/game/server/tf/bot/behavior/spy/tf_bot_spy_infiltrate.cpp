//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_infiltrate.cpp
// Move into position behind enemy lines and wait for victims
// Michael Booth, June 2010

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "bot/tf_bot.h"
#include "bot/behavior/spy/tf_bot_spy_infiltrate.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"
#include "bot/behavior/spy/tf_bot_spy_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_debug_spy( "tf_bot_debug_spy", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyInfiltrate::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_hideArea = NULL;

	m_hasEnteredCombatZone = false;

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyInfiltrate::Update( CTFBot *me, float interval )
{
	// switch to our pistol
	CBaseCombatWeapon *myGun = me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
	if ( myGun )
	{
		me->Weapon_Switch( myGun );
	}

	CTFNavArea *myArea = me->GetLastKnownArea();

	if ( !myArea )
	{
		return Continue();
	}

	bool isInMySpawn = myArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED );
	if ( myArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_EXIT ) )
	{
		// don't count exits so we cloak as we leave
		isInMySpawn = false;
	}

	// cloak when we first enter an area of active combat
	if ( !me->m_Shared.IsStealthed() && 
		 !isInMySpawn && 
		 myArea->IsInCombat() && 
		 !m_hasEnteredCombatZone )
	{
		m_hasEnteredCombatZone = true;
		me->PressAltFireButton();
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() && threat->GetEntity()->IsBaseObject() )
	{
		CBaseObject *enemyObject = (CBaseObject *)threat->GetEntity();
		if ( !enemyObject->HasSapper() && me->IsEnemy( enemyObject ) )
		{
			return SuspendFor( new CTFBotSpySap( enemyObject ), "Sapping an enemy object" );
		}
	}

	if ( me->GetEnemySentry() && !me->GetEnemySentry()->HasSapper() )
	{
		return SuspendFor( new CTFBotSpySap( me->GetEnemySentry() ), "Sapping a Sentry" );
	}

	if ( !m_hideArea && m_findHidingSpotTimer.IsElapsed() )
	{
		FindHidingSpot( me );
		m_findHidingSpotTimer.Start( 3.0f );
	}

	if ( !TFGameRules()->InSetup() )
	{
		// go after victims we've gotten behind
		if ( threat && threat->GetTimeSinceLastKnown() < 3.0f )
		{
			CTFPlayer *victim = ToTFPlayer( threat->GetEntity() );
			if ( victim )
			{
				CTFNavArea *victimArea = (CTFNavArea *)victim->GetLastKnownArea();
				if ( victimArea )
				{
					int victimTeam = victim->GetTeamNumber();

					if ( victimArea->GetIncursionDistance( victimTeam ) > myArea->GetIncursionDistance( victimTeam ) )
					{
						if ( me->m_Shared.IsStealthed() )
						{
							return SuspendFor( new CTFBotRetreatToCover( new CTFBotSpyAttack( victim ) ), "Hiding to decloak before going after a backstab victim" );
						}
						else
						{
							return SuspendFor( new CTFBotSpyAttack( victim ), "Going after a backstab victim" );
						}
					}
				}					
			}
		}
	}

	if ( m_hideArea )
	{
		if ( tf_bot_debug_spy.GetBool() )
		{
			m_hideArea->DrawFilled( 255, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}

		if ( myArea == m_hideArea )
		{
			// stay hidden during setup time
			if ( TFGameRules()->InSetup() )
			{
				m_waitTimer.Start( RandomFloat( 0.0f, 5.0f ) );
			}
			else
			{
				// wait in our hiding spot for a bit, then try another
				if ( !m_waitTimer.HasStarted() )
				{
					m_waitTimer.Start( RandomFloat( 5.0f, 10.0f ) );
				}
				else if ( m_waitTimer.IsElapsed() )
				{
					// time to find a new hiding spot
					m_hideArea = NULL;
				}
			}
		}
		else
		{
			// move to our ambush position
			if ( m_repathTimer.IsElapsed() )
			{
				m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

				// we may not be able to path to our hiding spot, but get as close as we can
				// (dropdown mid spawn in cp_gorge)
				CTFBotPathCost cost( me, SAFEST_ROUTE );
				m_path.Compute( me, m_hideArea->GetCenter(), cost );
			}

			m_path.Update( me );

			m_waitTimer.Invalidate();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotSpyInfiltrate::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyInfiltrate::OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyInfiltrate::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_repathTimer.Invalidate();
	m_hideArea = NULL;

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CTFBotSpyInfiltrate::FindHidingSpot( CTFBot *me )
{
	m_hideArea = NULL;

	if ( me->GetAliveDuration() < 5.0f && TFGameRules()->InSetup() )
	{
		// wait a bit until the nav mesh has updated itself
		return false;
	}

	int myTeam = me->GetTeamNumber();
	const CUtlVector< CTFNavArea * > *enemySpawnExitVector = TheTFNavMesh()->GetSpawnRoomExitAreas( GetEnemyTeam( myTeam ) );

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		// for now, just lurk where we are
		return false;
	}
#endif

	if ( !enemySpawnExitVector || enemySpawnExitVector->Count() == 0 )
	{
		if ( tf_bot_debug_spy.GetBool() )
		{
			DevMsg( "%3.2f: No enemy spawn room exit areas found\n", gpGlobals->curtime );
		}
		return false;
	}

	// find nearby place to hide hear enemy spawn exit(s)
	CUtlVector< CNavArea * > nearbyAreaVector;
	const float nearbyHideRange = 2500.0f;
	for( int x=0; x<enemySpawnExitVector->Count(); ++x )
	{
		CTFNavArea *enemySpawnExitArea = enemySpawnExitVector->Element( x );

		CUtlVector< CNavArea * > nearbyThisExitAreaVector;
		CollectSurroundingAreas( &nearbyThisExitAreaVector, enemySpawnExitArea, nearbyHideRange, me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetStepHeight() );

		// concat vectors (assuming N^2 unique search would cost more than ripping through some duplicates)
		nearbyAreaVector.AddVectorToTail( nearbyThisExitAreaVector );
	}

	// find area not visible to any enemy spawn exits
	CUtlVector< CTFNavArea * > hideAreaVector;
	int i;

	for( i=0; i<nearbyAreaVector.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)nearbyAreaVector[i];

		if ( !me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			continue;

		bool isHidden = true;
		for( int j=0; j<enemySpawnExitVector->Count(); ++j )
		{
			if ( area->IsPotentiallyVisible( enemySpawnExitVector->Element(j) ) )
			{
				isHidden = false;
				break;
			}
		}

		if ( isHidden )
		{
			hideAreaVector.AddToTail( area );
		}
	}

	if ( hideAreaVector.Count() == 0 )
	{
		if ( tf_bot_debug_spy.GetBool() )
		{
			DevMsg( "%3.2f: Can't find any non-visible hiding areas, trying for anything near the spawn exit...\n", gpGlobals->curtime );
		}

		for( i=0; i<nearbyAreaVector.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)nearbyAreaVector[i];

			if ( !me->GetLocomotionInterface()->IsAreaTraversable( area ) )
				continue;

			hideAreaVector.AddToTail( area );
		}
	}

	if ( hideAreaVector.Count() == 0 )
	{
		if ( tf_bot_debug_spy.GetBool() )
		{
			DevMsg( "%3.2f: Can't find any areas near the enemy spawn exit - just heading to the enemy spawn and hoping...\n", gpGlobals->curtime );
		}

		m_hideArea = enemySpawnExitVector->Element( RandomInt( 0, enemySpawnExitVector->Count()-1 ) );

		return false;
	}

	// pick a specific hiding spot
	m_hideArea = hideAreaVector[ RandomInt( 0, hideAreaVector.Count()-1 ) ];

	return true;
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyInfiltrate::OnStuck( CTFBot *me )
{
	m_hideArea = NULL;
	m_findHidingSpotTimer.Invalidate();

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyInfiltrate::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	// enemy spawn likely changed - find new hiding spot after internal data has updated
	m_hideArea = NULL;
	m_findHidingSpotTimer.Start( 5.0f );

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyInfiltrate::OnTerritoryLost( CTFBot *me, int territoryID )
{
	// enemy spawn likely changed - find new hiding spot after internal data has updated
	m_hideArea = NULL;
	m_findHidingSpotTimer.Start( 5.0f );

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSpyInfiltrate::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_NO;
}
