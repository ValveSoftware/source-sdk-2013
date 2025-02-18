//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENTITY_PASSTIME_BALL_SPAWN_H
#define ENTITY_PASSTIME_BALL_SPAWN_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "util_shared.h"

//-----------------------------------------------------------------------------
class CTFPasstimeLogic;

DECLARE_AUTO_LIST( IPasstimeBallSpawnAutoList );

//-----------------------------------------------------------------------------
class CPasstimeBallSpawn : public CPointEntity, public IPasstimeBallSpawnAutoList
{
public:
	DECLARE_CLASS( CPasstimeBallSpawn, CPointEntity );
	DECLARE_DATADESC();
	CPasstimeBallSpawn();
	bool IsEnabled() const;

private:
	friend class CTFPasstimeLogic;
	void InputEnable( inputdata_t &input );
	void InputDisable( inputdata_t &input );

	COutputEvent m_onSpawnBall;
	bool m_bDisabled;
};

#endif // ENTITY_PASSTIME_BALL_SPAWN_H  
