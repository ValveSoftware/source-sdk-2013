//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_PASSTIME_NO_BALL_ZONE_H
#define FUNC_PASSTIME_NO_BALL_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "util.h"

extern bool EntityIsInNoBallZone( CBaseEntity *pTarget );

DECLARE_AUTO_LIST( IFuncPasstimeNoBallZoneAutoList );

class CFuncPasstimeNoBallZone : public CBaseTrigger, public IFuncPasstimeNoBallZoneAutoList
{
public:
	DECLARE_CLASS( CFuncPasstimeNoBallZone, CBaseTrigger );
	DECLARE_DATADESC();
	virtual void Spawn() OVERRIDE;
};

#endif // FUNC_PASSTIME_NO_BALL_ZONE_H  
