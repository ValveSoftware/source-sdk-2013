//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_passtime_goalie_zone.h"
#include "tf_player.h"
#include "passtime_convars.h"

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncPasstimeGoalieZone )
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_passtime_goalie_zone, CFuncPasstimeGoalieZone )

IMPLEMENT_AUTO_LIST( IFuncPasstimeGoalieZoneAutoList );

//-----------------------------------------------------------------------------
void CFuncPasstimeGoalieZone::Spawn() 
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
bool CFuncPasstimeGoalieZone::BPlayerInAny( CTFPlayer *pPlayer )
{
	auto &all = AutoList();
	for ( int i = 0; i < all.Count(); ++i )
	{
		auto *pZone = (CFuncPasstimeGoalieZone *)all[i];
		if ( pZone->IsTouching( pPlayer ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CFuncPasstimeGoalieZone::BPlayerInFriendly( CTFPlayer *pPlayer )
{
	auto &all = AutoList();
	int iPlayerTeam = pPlayer->GetTeamNumber();
	for ( int i = 0; i < all.Count(); ++i )
	{
		auto *pZone = (CFuncPasstimeGoalieZone *)all[i];
		if ( (pZone->GetTeamNumber() == iPlayerTeam) && pZone->IsTouching( pPlayer ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CFuncPasstimeGoalieZone::BPlayerInEnemy( CTFPlayer *pPlayer )
{
	auto &all = AutoList();
	int iPlayerTeam = pPlayer->GetTeamNumber();
	for ( int i = 0; i < all.Count(); ++i )
	{
		auto *pZone = (CFuncPasstimeGoalieZone *)all[i];
		if ( (pZone->GetTeamNumber() != iPlayerTeam) && pZone->IsTouching( pPlayer ) )
			return true;
	}
	return false;
}
