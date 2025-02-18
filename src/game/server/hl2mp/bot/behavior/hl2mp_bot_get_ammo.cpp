//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/hl2mp_bot_get_ammo.h"
#include "items.h"

extern ConVar hl2mp_bot_path_lookahead_range;

ConVar hl2mp_bot_ammo_search_range( "hl2mp_bot_ammo_search_range", "5000", FCVAR_CHEAT, "How far bots will search to find ammo around them" );
ConVar hl2mp_bot_debug_ammo_scavenging( "hl2mp_bot_debug_ammo_scavenging", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CHL2MPBotGetAmmo::CHL2MPBotGetAmmo( void )
{
	m_path.Invalidate();
	m_ammo = NULL;
}


//---------------------------------------------------------------------------------------------
class CAmmoFilter : public INextBotFilter
{
public:
	CAmmoFilter( CHL2MPBot *me )
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

		CItem* pItem = dynamic_cast< CItem* >( candidate );
		if ( !pItem )
			return false;

		const char* pszWeaponClass = pItem->GetWeaponClassForAmmo();
		if ( !pszWeaponClass || !*pszWeaponClass )
			return false;

		//CBaseCombatWeapon* pWeapon = m_me->Weapon_OwnsThisType( pszWeaponClass );
		CBaseCombatWeapon* pWeapon = m_me->GetActiveWeapon();
		if ( !pWeapon )
			return false;

		if ( m_me->IsBludgeon( pWeapon ) || pWeapon->GetPrimaryAmmoType() == -1 )
			return false;

		// Only hunt for ammo for our active weapon.
		if ( !pWeapon->ClassMatches( pszWeaponClass ) )
			return false;

		return m_me->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) < m_me->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() );
	}

	CHL2MPBot *m_me;
};


//---------------------------------------------------------------------------------------------
static CHL2MPBot *s_possibleBot = NULL;
static CHandle< CBaseEntity > s_possibleAmmo = NULL;
static int s_possibleFrame = 0;


//---------------------------------------------------------------------------------------------
/**
 * Return true if this Action has what it needs to perform right now
 */
bool CHL2MPBotGetAmmo::IsPossible( CHL2MPBot *me )
{
	VPROF_BUDGET( "CHL2MPBotGetAmmo::IsPossible", "NextBot" );

	float searchRange = hl2mp_bot_ammo_search_range.GetFloat();

	CBaseEntity* ammo = NULL;
	CUtlVector< CHandle< CBaseEntity > > hAmmos;
	while ( ( ammo = gEntList.FindEntityByClassname( ammo, "*_health*" ) ) != NULL )
	{
		hAmmos.AddToTail( ammo );
	}

	CAmmoFilter healthFilter( me );
	CUtlVector< CHandle< CBaseEntity > > hReachableAmmo;
	me->SelectReachableObjects( hAmmos, &hReachableAmmo, healthFilter, me->GetLastKnownArea(), searchRange );

	CBaseEntity* closestAmmo = hReachableAmmo.Size() > 0 ? hReachableAmmo[0] : NULL;

	if ( !closestAmmo )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No ammo nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	if ( hl2mp_bot_debug_ammo_scavenging.GetBool() )
	{
		NDebugOverlay::Cross3D( closestAmmo->WorldSpaceCenter(), 5.0f, 255, 255, 0, true, 999.9 );
	}

	CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
	PathFollower path;
	if ( !path.Compute( me, closestAmmo->WorldSpaceCenter(), cost ) || !path.IsValid() || path.GetResult() != Path::COMPLETE_PATH )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No path to ammo!\n", gpGlobals->curtime );
		}
		return false;
	}

	s_possibleBot = me;
	s_possibleAmmo = closestAmmo;
	s_possibleFrame = gpGlobals->framecount;

	return true;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotGetAmmo::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	VPROF_BUDGET( "CHL2MPBotGetAmmo::OnStart", "NextBot" );

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// if IsPossible() has already been called, use its cached data
	if ( s_possibleFrame != gpGlobals->framecount || s_possibleBot != me )
	{
		if ( !IsPossible( me ) || s_possibleAmmo == NULL )
		{
			return Done( "Can't get ammo" );
		}
	}

	m_ammo = s_possibleAmmo;

	CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_path.Compute( me, m_ammo->WorldSpaceCenter(), cost ) || !m_path.IsValid() || m_path.GetResult() != Path::COMPLETE_PATH )
	{
		return Done( "No path to ammo!" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotGetAmmo::Update( CHL2MPBot *me, float interval )
{
	if ( me->IsAmmoFull() )
	{
		return Done( "My ammo is full" );
	}

	if ( m_ammo == NULL )
	{
		return Done( "Ammo I was going for has been taken" );
	}

	if ( !m_path.IsValid() )
	{
		return Done( "My path became invalid" );
	}

	// may need to switch weapons due to out of ammo
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetAmmo::OnContact( CHL2MPBot *me, CBaseEntity *other, CGameTrace *result )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetAmmo::OnStuck( CHL2MPBot *me )
{
	return TryDone( RESULT_CRITICAL, "Stuck trying to reach ammo" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetAmmo::OnMoveToSuccess( CHL2MPBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotGetAmmo::OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Failed to reach ammo" );
}


//---------------------------------------------------------------------------------------------
QueryResultType CHL2MPBotGetAmmo::ShouldHurry( const INextBot *me ) const
{
	// if we need ammo, we best hustle
	return ANSWER_YES;
}
