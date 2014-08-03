//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_PLAYER_SURFACE_TRIGGER_H
#define ENV_PLAYER_SURFACE_TRIGGER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"

//-----------------------------------------------------------------------------
// Purpose: Entity that fires outputs whenever the player stands on a different surface
//-----------------------------------------------------------------------------
class CEnvPlayerSurfaceTrigger : public CPointEntity
{
	DECLARE_CLASS( CEnvPlayerSurfaceTrigger, CPointEntity );
public:
	DECLARE_DATADESC();

	~CEnvPlayerSurfaceTrigger( void );
	void	Spawn( void );
	void	OnRestore( void );

	// Main interface to all surface triggers
	static void	SetPlayerSurface( CBasePlayer *pPlayer, char gameMaterial );

	void	UpdateMaterialThink( void );

private:
	void	PlayerSurfaceChanged( CBasePlayer *pPlayer, char gameMaterial );
	void	InputDisable( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );

private:
	int		m_iTargetGameMaterial;
	int		m_iCurrentGameMaterial;
	bool	m_bDisabled;

	// Outputs
	COutputEvent m_OnSurfaceChangedToTarget;
	COutputEvent m_OnSurfaceChangedFromTarget;
};

#endif // ENV_PLAYER_SURFACE_TRIGGER_H
