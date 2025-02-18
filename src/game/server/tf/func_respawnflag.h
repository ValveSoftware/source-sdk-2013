//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_RESPAWNFLAG_H
#define FUNC_RESPAWNFLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//-----------------------------------------------------------------------------
// Purpose: Designates an area that triggers the flag to respawn when it touches the area
//-----------------------------------------------------------------------------
class CFuncRespawnFlagZone : public CBaseTrigger
{
	DECLARE_CLASS( CFuncRespawnFlagZone, CBaseTrigger );

public:
	DECLARE_DATADESC();

	CFuncRespawnFlagZone();

	void	Spawn( void );
	void	Touch( CBaseEntity *pOther );
};

// Return true if the specified entity is in a NoGrenades zone
bool PointInRespawnFlagZone( const Vector &vecPoint );

#endif // FUNC_RESPAWNFLAG_H
