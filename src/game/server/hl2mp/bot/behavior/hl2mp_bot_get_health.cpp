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

ConVar hl2mp_bot_debug_health_scavenging( "hl2mp_bot_debug_ammo_scavenging", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter( CHL2MPBot *me )
	{
		m_me = me;
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

		if ( candidate->ClassMatches( "item_healthcharger" ) )
			return true;

		if ( candidate->ClassMatches( "func_healthcharger" ) )
			return true;

		return false;
	}

	CHL2MPBot *m_me;
};


//---------------------------------------------------------------------------------------------
static CHL2MPBot *s_possibleBot = NULL;
static CHandle< CBaseEntity > s_possibleHealth = NULL;
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
	while ( ( healthkit = gEntList.FindEntityByClassname( healthkit, "*_health*" ) ) != NULL )
	{
		hHealthKits.AddToTail( healthkit );
	}

	CHealthFilter healthFilter( me );
	CUtlVector< CHandle< CBaseEntity > > hReachableHealthKits;
	me->SelectReachableObjects( hHealthKits, &hReachableHealthKits, healthFilter, me->GetLastKnownArea(), searchRange );

	CBaseEntity* closestHealth = hReachableHealthKits.Size() > 0 ? hReachableHealthKits[0] : NULL;

	if ( !closestHealth )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
	PathFollower path;
	if ( !path.Compute( me, closestHealth->WorldSpaceCenter(), cost ) || !path.IsValid() || path.GetResult() != Path::COMPLETE_PATH )
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
	m_isGoalCharger = m_healthKit->ClassMatches( "*charger*" );

	CHL2MPBotPathCost cost( me, SAFEST_ROUTE );
	if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
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
		const float nearRange = 50.0f;
		if ( ( me->GetAbsOrigin() - m_healthKit->GetAbsOrigin() ).IsLengthLessThan( nearRange ) )
		{
			if ( me->GetVisionInterface()->IsLineOfSightClearToEntity( m_healthKit ) )
			{
				if ( me->GetHealth() == me->GetMaxHealth() )
				{
					return Done( "Health refilled by the Charger" );
				}

				CNewWallHealth* pNewWallHealth = dynamic_cast< CNewWallHealth* >( m_healthKit.Get() );
				if ( pNewWallHealth )
				{
					pNewWallHealth->Use( me, me, USE_ON, 0.0f );

					if ( pNewWallHealth->GetJuice() == 0 )
						return Done( "Charger is out of juice!" );
				}

				CWallHealth* pWallHealth = dynamic_cast< CWallHealth* >( m_healthKit.Get() );
				if ( pWallHealth )
				{
					pWallHealth->Use( me, me, USE_ON, 0.0f );

					if ( pWallHealth->GetJuice() == 0 )
						return Done( "Charger is out of juice!" );
				}

				float healthRatio = ( float )me->GetHealth() / ( float )me->GetMaxHealth();
				bool bLowHealth = healthRatio > hl2mp_bot_health_critical_ratio.GetFloat();

				// don't wait if I'm in combat
				if ( !bLowHealth && me->GetVisionInterface()->GetPrimaryKnownThreat() )
				{
					return Done( "No time to wait for more health, I must fight" );
				}

				// wait until the charger refills us
				return Continue();
			}
		}
	}

	if ( !m_path.IsValid() )
	{
		// this can occur if we overshoot the health kit's location
		// because it is momentarily gone
		CHL2MPBotPathCost cost( me, SAFEST_ROUTE );
		if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
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
