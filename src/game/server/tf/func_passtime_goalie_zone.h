//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_PASSTIME_GOALIE_ZONE_H
#define FUNC_PASSTIME_GOALIE_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

DECLARE_AUTO_LIST( IFuncPasstimeGoalieZoneAutoList );
class CTFPlayer;

class CFuncPasstimeGoalieZone : public CBaseTrigger, public IFuncPasstimeGoalieZoneAutoList
{
public:
	DECLARE_CLASS( CFuncPasstimeGoalieZone, CBaseTrigger );
	DECLARE_DATADESC();
	virtual void Spawn() OVERRIDE;

	static bool BPlayerInAny( CTFPlayer *pPlayer );
	static bool BPlayerInFriendly( CTFPlayer *pPlayer );
	static bool BPlayerInEnemy( CTFPlayer *pPlayer );
};

#endif // FUNC_PASSTIME_GOALIE_ZONE_H  
