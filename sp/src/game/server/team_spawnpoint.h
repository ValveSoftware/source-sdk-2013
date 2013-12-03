//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team spawnpoint entity
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_TEAMSPAWNPOINT_H
#define TF_TEAMSPAWNPOINT_H
#pragma once

#include "baseentity.h"
#include "entityoutput.h"

class CTeam;

//-----------------------------------------------------------------------------
// Purpose: points at which the player can spawn, restricted by team
//-----------------------------------------------------------------------------
class CTeamSpawnPoint : public CPointEntity
{
public:
	DECLARE_CLASS( CTeamSpawnPoint, CPointEntity );

	void	Activate( void );
	virtual bool	IsValid( CBasePlayer *pPlayer );

	COutputEvent m_OnPlayerSpawn;

protected:	
	int		m_iDisabled;

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: points at which vehicles can spawn, restricted by team
//-----------------------------------------------------------------------------
class CTeamVehicleSpawnPoint : public CTeamSpawnPoint
{
	DECLARE_CLASS( CTeamVehicleSpawnPoint, CTeamSpawnPoint );
public:
	void	Activate( void );
	bool	IsValid( void );

	COutputEvent m_OnVehicleSpawn;

	DECLARE_DATADESC();
};


#endif // TF_TEAMSPAWNPOINT_H
