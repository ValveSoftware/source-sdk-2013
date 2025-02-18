//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_passtime_no_ball_zone.h"
#include "passtime_convars.h"
#include "tf_player.h"

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncPasstimeNoBallZone )
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_passtime_no_ball_zone, CFuncPasstimeNoBallZone )

//-----------------------------------------------------------------------------
IMPLEMENT_AUTO_LIST( IFuncPasstimeNoBallZoneAutoList );

//-----------------------------------------------------------------------------
void CFuncPasstimeNoBallZone::Spawn()
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS | SF_TRIGGER_ALLOW_PHYSICS );
	BaseClass::Spawn();
	InitTrigger();
	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Is a given point contained within a no ball zone?
//-----------------------------------------------------------------------------
bool EntityIsInNoBallZone( CBaseEntity *pTarget )
{
	const auto &allNoBallZones = IFuncPasstimeNoBallZoneAutoList::AutoList();
	for ( int i = 0; i < allNoBallZones.Count(); ++i )
	{
		CFuncPasstimeNoBallZone *pNoBallZone = static_cast< CFuncPasstimeNoBallZone* >( allNoBallZones[i] );
		if ( pTarget && pNoBallZone->IsTouching(pTarget) )
		{
			return true;
		}
	}
	return false;
}
