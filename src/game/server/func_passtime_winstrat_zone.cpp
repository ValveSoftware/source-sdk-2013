//========= Contributed by 4v4 PASS Time developers. ==========================//
//
// Purpose: Trigger brush for map developers to specify that a first grab goal
// (usually a Panacea) should be considered a "win strat."
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_passtime_winstrat_zone.h"
#include "passtime_convars.h"
#include "tf_player.h"

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncPasstimeWinstratZone )
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_passtime_winstrat_zone, CFuncPasstimeWinstratZone )

//-----------------------------------------------------------------------------
IMPLEMENT_AUTO_LIST( IFuncPasstimeWinstratZoneAutoList );

//-----------------------------------------------------------------------------
void CFuncPasstimeWinstratZone::Spawn()
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS | SF_TRIGGER_ALLOW_PHYSICS );
	BaseClass::Spawn();
	InitTrigger();
	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Is a given point contained within a winstrat zone
//-----------------------------------------------------------------------------
bool EntityIsInWinstratZone( CBaseEntity *pTarget )
{
	const auto &allWinstratZones = IFuncPasstimeWinstratZoneAutoList::AutoList();
	for ( int i = 0; i < allWinstratZones.Count(); ++i )
	{
		CFuncPasstimeWinstratZone *pWinstratzone =
		static_cast<CFuncPasstimeWinstratZone *>( allWinstratZones[i] );
		if ( pTarget && pWinstratzone->IsTouching( pTarget ) )
		{
			return true;
		}
	}
	return false;
}
