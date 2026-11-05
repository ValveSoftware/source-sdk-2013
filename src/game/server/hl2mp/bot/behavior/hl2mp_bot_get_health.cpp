//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "bot/hl2mp_bot.h"
#include "item_healthkit.h"
#include "bot/behavior/hl2mp_bot_get_health.h"

extern ConVar hl2mp_bot_path_lookahead_range;

ConVar hl2mp_bot_health_critical_ratio( "hl2mp_bot_health_critical_ratio", "0.3", FCVAR_CHEAT );
ConVar hl2mp_bot_health_ok_ratio( "hl2mp_bot_health_ok_ratio", "0.65", FCVAR_CHEAT );
ConVar hl2mp_bot_health_search_near_range( "hl2mp_bot_health_search_near_range", "1000", FCVAR_CHEAT );
ConVar hl2mp_bot_health_search_far_range( "hl2mp_bot_health_search_far_range", "2000", FCVAR_CHEAT );
ConVar hl2mp_bot_health_charger_weave( "hl2mp_bot_health_charger_weave", "1", FCVAR_CHEAT );

ConVar hl2mp_bot_debug_health_scavenging( "hl2mp_bot_debug_health_scavenging", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter( CHL2MPBot *me, bool bFindChargers = true )
	{
		m_me = me;
		m_bFindChargers = bFindChargers;
	}

	bool IsSelected( const CBaseEntity *constCandidate ) const
	{
		if ( !constCandidate )
			return false;

		CBaseEntity *candidate = const_cast< CBaseEntity * >( constCandidate );

		CClosestHL2MPPlayer close( candidate );
		ForEachPlayer( close );

		// if the closest player to this candidate object is an enemy, don't use it
		if ( close.m_closePlayer && m_me->IsEnemy( close.m_closePlayer ) )
			return false;

		// ignore non-existent ammo to ensure we collect nearby existing ammo
		if ( candidate->IsEffectActive( EF_NODRAW ) )
			return false;

		if ( candidate->ClassMatches( "item_healthkit" ) )
			return true;

		if ( candidate->ClassMatches( "item_healthvial" ) )
			return true;

		if ( m_bFindChargers )
		{
			if ( candidate->ClassMatches( "item_healthcharger" ) )
			{
				// needs to have juice
				CNewWallHealth* pNewWallHealth = dynamic_cast< CNewWallHealth* >( candidate );
				if ( pNewWallHealth && pNewWallHealth->GetJuice() == 0 )
					return false;

				return true;
			}

			if ( candidate->ClassMatches( "func_healthcharger" ) )
			{
				// needs to have juice
				CWallHealth* pWallHealth = dynamic_cast< CWallHealth* >( candidate );
				if ( pWallHealth && pWallHealth->GetJuice() == 0 )
					return false;

				return true;
			}
		}

		return false;
	}

	CHL2MPBot *m_me;
	bool m_bFindChargers;
};


//---------------------------------------------------------------------------------------------
Vector CHL2MPBotGetHealth::GetHealthKitPathOrigin( CBaseEntity *pHealthKit, bool bIsCharger )
{
	Vector target = pHealthKit->WorldSpaceCenter();
	if ( bIsCharger )
	{
		// chargers are usually on walls, so move in front of it instead
		if ( pHealthKit->ClassMatches( "func*" ) )
		{
			// brushes typically don't have angles, so we have to compromise by finding the nearest area
			CNavArea *pNearestArea = TheNavMesh->GetNearestNavArea( pHealthKit );

			Vector dir = (pNearestArea->GetCenter() - pHealthKit->WorldSpaceCenter());
			dir.z = 0;
			VectorNormalize( dir );

			target += (dir * pHealthKit->BoundingRadius()) + (dir * HalfHumanWidth);
		}
		else
		{
			Vector dir;
			pHealthKit->GetVectors( &dir, NULL, NULL );

			target += (dir * pHealthKit->BoundingRadius()) + (dir * HalfHumanWidth);
		}

		if ( hl2mp_bot_debug_health_scavenging.GetBool() )
		{
			NDebugOverlay::Cross3D( target, 5.0f, 0, 255, 0, true, 10.0f );
		}
	}

	return target;
}

//---------------------------------------------------------------------------------------------
// Version of CCollectReachableObjects which uses GetHealthKitPathOrigin()
//---------------------------------------------------------------------------------------------
class CCollectReachableHealthKits : public ISearchSurroundingAreasFunctor
{
public:
	CCollectReachableHealthKits( const CHL2MPBot *me, float maxRange, const CUtlVector< CHandle< CBaseEntity > > &potentialVector, CUtlVector< CHandle< CBaseEntity > > *collectionVector ) : m_potentialVector( potentialVector )
	{
		m_me = me;
		m_maxRange = maxRange;
		m_collectionVector = collectionVector;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
	{
		// do any of the potential objects overlap this area?
		FOR_EACH_VEC( m_potentialVector, it )
		{
			CBaseEntity *obj = m_potentialVector[ it ];

			if ( obj && area->Contains( CHL2MPBotGetHealth::GetHealthKitPathOrigin( obj, V_strstr( obj->GetClassname(), "charger" ) ) ) )
			{
				// reachable - keep it
				if ( !m_collectionVector->HasElement( obj ) )
				{
					m_collectionVector->AddToTail( obj );
				}
			}
		}
		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( m_me->GetTeamNumber() ) )
		{
			return false;
		}

		if ( travelDistanceSoFar > m_maxRange )
		{
			// too far away
			return false;
		}

		return currentArea->IsContiguous( adjArea );
	}

	const CHL2MPBot *m_me;
	float m_maxRange;
	const CUtlVector< CHandle< CBaseEntity > > &m_potentialVector;
	CUtlVector< CHandle< CBaseEntity > > *m_collectionVector;
};


//---------------------------------------------------------------------------------------------
static CHL2MPBot *s_possibleBot = NULL;
static CHandle< CBaseEntity > s_possibleHealth = NULL;
static bool s_possibleIsCharger = NULL;
static int s_possibleFrame = 0;


//---------------------------------------------------------------------------------------------
/** 
 * Return true if this Action has what it needs to perform right now
 */
bool CHL2MPBotGetHealth::IsPossible( CHL2MPBot *me )
{
	VPROF_BUDGET( "CHL2MPBotGetHealth::IsPossible", "NextBot" );

	float healthRatio = (float)me->GetHealth() / (float)me->GetMaxHealth();

	float t = ( healthRatio - hl2mp_bot_health_critical_ratio.GetFloat() ) / ( hl2mp_bot_health_ok_ratio.GetFloat() - hl2mp_bot_health_critical_ratio.GetFloat() );
	t = clamp( t, 0.0f, 1.0f );

	if ( me->GetFlags() & FL_ONFIRE )
	{
		// on fire - get health now
		t = 0.0f;
	}

	// the more we are hurt, the farther we'll travel to get health
	float searchRange = hl2mp_bot_health_search_far_range.GetFloat() + t * ( hl2mp_bot_health_search_near_range.GetFloat() - hl2mp_bot_health_search_far_range.GetFloat() );

	CBaseEntity* healthkit = NULL;
	CUtlVector< CHandle< CBaseEntity > > hHealthKits;
	while ( ( healthkit = gEntList.FindEntityByClassname( healthkit, "item_health*" ) ) != NULL )
	{
		hHealthKits.AddToTail( healthkit );
	}
	
	healthkit = NULL;
	while ( ( healthkit = gEntList.FindEntityByClassname( healthkit, "func_health*" ) ) != NULL )
	{
		hHealthKits.AddToTail( healthkit );
	}

	bool bFindChargers = true;
	bool bLowHealth = healthRatio < hl2mp_bot_health_critical_ratio.GetFloat();

	// don't use chargers if I'm in combat
	if ( !bLowHealth && me->GetVisionInterface()->GetPrimaryKnownThreat( true ) )
		bFindChargers = false;

	CHealthFilter healthFilter( me, bFindChargers );
	CUtlVector< CHandle< CBaseEntity > > hReachableHealthKits;

	// can't call CHL2MPBot::SelectReachableObjects() because chargers don't use their worldspace center in pathfinding
	if ( me->GetLastKnownArea() )
	{		
		// filter candidate objects
		CUtlVector< CHandle< CBaseEntity > > filteredObjectVector;
		for( int i=0; i<hHealthKits.Count(); ++i )
		{
			if ( healthFilter.IsSelected( hHealthKits[i] ) )
			{
				filteredObjectVector.AddToTail( hHealthKits[i] );
			}
		}

		// only keep those that are reachable by us
		CCollectReachableHealthKits collector( me, searchRange, filteredObjectVector, &hReachableHealthKits );
		SearchSurroundingAreas( me->GetLastKnownArea(), collector );
	}

	CBaseEntity* closestHealth = hReachableHealthKits.Size() > 0 ? hReachableHealthKits[0] : NULL;

	if ( !closestHealth )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	s_possibleIsCharger = V_strstr( closestHealth->GetClassname(), "charger" );

	CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
	PathFollower path;
	if ( !path.Compute( me, GetHealthKitPathOrigin( closestHealth, s_possibleIsCharger ), cost ) || !path.IsValid() || path.GetResult() != Path::COMPLETE_PATH )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No path to health!\n", gpGlobals->curtime );
		}
		return false;
	}

	s_possibleBot = me;
	s_possibleHealth = closestHealth;
	s_possibleFrame = gpGlobals->framecount;

	return true;
}

//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotGetHealth::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	VPROF_BUDGET( "CHL2MPBotGetHealth::OnStart", "NextBot" );

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// if IsPossible() has already been called, use its cached data
	if ( s_possibleFrame != gpGlobals->framecount || s_possibleBot != me )
	{
		if ( !IsPossible( me ) || s_possibleHealth == NULL )
		{
			return Done( "Can't get health" );
		}
	}

	m_healthKit = s_possibleHealth;
	m_isGoalCharger = s_possibleIsCharger;

	CHL2MPBotPathCost cost( me, SAFEST_ROUTE );
	if ( !m_path.Compute( me, GetHealthKitPathOrigin( m_healthKit, m_isGoalCharger ), cost ) )
	{
		return Done( "No path to health!" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotGetHealth::Update( CHL2MPBot *me, float interval )
{
	if ( m_healthKit == NULL || ( m_healthKit->IsEffectActive( EF_NODRAW ) ) )
	{
		return Done( "Health kit I was going for has been taken" );
	}

	if ( me->GetHealth() >= me->GetMaxHealth() )
	{
		return Done( "I've been healed" );
	}

	if ( HL2MPRules()->IsTeamplay() )
	{
		// if the closest player to the item we're after is an enemy, give up

		CClosestHL2MPPlayer close( m_healthKit );
		ForEachPlayer( close );
		if ( close.m_closePlayer && me->IsEnemy( close.m_closePlayer ) )
			return Done( "An enemy is closer to it" );
	}

	if ( m_isGoalCharger )
	{
		// we need to get near and wait, not try to run over
		const float nearRange = PLAYER_USE_RADIUS;
		Vector vecDir = (me->EyePosition() - m_healthKit->WorldSpaceCenter());
		if ( vecDir.IsLengthLessThan( nearRange ) )
		{
			// if we're carrying a prop, drop it
			if ( me->Physcannon_GetHeldProp() )
			{
				me->PressAltFireButton( 0.1f );
			}

			if ( me->GetVisionInterface()->IsLineOfSightClearToEntity( m_healthKit ) )
			{
				if ( me->GetHealth() == me->GetMaxHealth() )
				{
					return Done( "Health refilled by the Charger" );
				}

				CNewWallHealth* pNewWallHealth = dynamic_cast< CNewWallHealth* >( m_healthKit.Get() );
				if ( pNewWallHealth )
				{
					if ( pNewWallHealth->GetJuice() == 0 )
						return Done( "Charger is out of juice!" );
				}

				CWallHealth* pWallHealth = dynamic_cast< CWallHealth* >( m_healthKit.Get() );
				if ( pWallHealth )
				{
					if ( pWallHealth->GetJuice() == 0 )
						return Done( "Charger is out of juice!" );
				}

				float healthRatio = ( float )me->GetHealth() / ( float )me->GetMaxHealth();
				bool bLowHealth = healthRatio < hl2mp_bot_health_critical_ratio.GetFloat();

				// don't wait if I'm in combat
				const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
				if ( !bLowHealth && threat && me->IsLineOfSightClear( threat->GetEntity(), CHL2MPBot::IGNORE_ACTORS ) )
				{
					return Done( "No time to wait for more health, I must fight" );
				}
				
				if ( !m_usingCharger )
				{
					// make sure we're not already using anything
					m_usingCharger = true;
					me->ReleaseUseButton();
					return Continue();
				}

				if ( me->GetDifficulty() >= CHL2MPBot::EXPERT && hl2mp_bot_health_charger_weave.GetBool() )
				{
					// expert bots go back and forth to avoid getting hit
					Vector right;
					m_healthKit->GetVectors( NULL, &right, NULL );

					Vector2D toMe = (me->GetAbsOrigin() - m_healthKit->GetAbsOrigin()).AsVector2D();
					Vector2DNormalize( toMe );

					// go towards the center
					float dot = right.AsVector2D().Dot( toMe );
					if ( dot < 0 )
					{
						me->PressLeftButton( 0.1f );
					}
					else
					{
						me->PressRightButton( 0.1f );
					}
				}

				// begin looking at the charger, then +USE it
				me->GetBodyInterface()->AimHeadTowards( m_healthKit, IBody::CRITICAL, 0.5f, NULL, "Using health charger" );
				if ( me->GetBodyInterface()->IsHeadAimingOnTarget() )
				{
					me->PressUseButton( 0.5f );
				}
				else
				{
					me->ReleaseUseButton();
				}

				// wait until the charger refills us
				return Continue();
			}
		}

		if ( m_usingCharger )
		{
			// something may have obstructed or moved us away
			m_usingCharger = false;
			me->ReleaseUseButton();
		}

		float healthRatio = ( float )me->GetHealth() / ( float )me->GetMaxHealth();
		bool bLowHealth = healthRatio < hl2mp_bot_health_critical_ratio.GetFloat();

		// don't keep going if I'm in combat
		if ( !bLowHealth && me->GetVisionInterface()->GetPrimaryKnownThreat( true ) )
		{
			return Done( "No time to reach charger, I must fight" );
		}
	}

	if ( !m_path.IsValid() )
	{
		// this can occur if we overshoot the health kit's location
		// because it is momentarily gone
		CHL2MPBotPathCost cost( me, SAFEST_ROUTE );
		if ( !m_path.Compute( me, GetHealthKitPathOrigin( m_healthKit, m_isGoalCharger ), cost ) )
		{
			return Done( "No path to health!" );
		}
	}

	m_path.Update( me );

	// may need to switch weapons (ie: engineer holding toolbox now needs to heal and defend himself)
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotGetHealth::OnSuspend( CHL2MPBot* me, Action< CHL2MPBot >* interruptingAction )
{
	if ( m_usingCharger )
	{
		m_usingCharger = false;
		me->ReleaseUseButton();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CHL2MPBotGetHealth::OnEnd( CHL2MPBot* me, Action< CHL2MPBot >* nextAction )
{
	if ( m_usingCharger )
	{
		m_usingCharger = false;
		me->ReleaseUseButton();
	}
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetHealth::OnStuck( CHL2MPBot *me )
{
	return TryDone( RESULT_CRITICAL, "Stuck trying to reach health kit" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetHealth::OnMoveToSuccess( CHL2MPBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetHealth::OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Failed to reach health kit" );
}


//---------------------------------------------------------------------------------------------
// We are always hurrying if we need to collect health
QueryResultType CHL2MPBotGetHealth::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}
