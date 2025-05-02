// checks to see if player is in winstrat zone so we can count it

#ifndef FUNC_PASSTIME_WINSTRAT_ZONE_H
#define FUNC_PASSTIME_WINSTRAT_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "util.h"

extern bool EntityIsInWinstratZone( CBaseEntity *pTarget );

DECLARE_AUTO_LIST( IFuncPasstimeWinstratZoneAutoList );

class CFuncPasstimeWinstratZone : public CBaseTrigger,
								public IFuncPasstimeWinstratZoneAutoList
{
  public:
	DECLARE_CLASS( CFuncPasstimeWinstratZone, CBaseTrigger );
	DECLARE_DATADESC();
	virtual void Spawn() OVERRIDE;
};

#endif // FUNC_PASSTIME_WINSTRAT_ZONE_H
