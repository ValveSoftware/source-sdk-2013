//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Reset Entity (resets the teams).
//
//=============================================================================//
#ifndef ENTITY_FORCERESPAWN_H
#define ENTITY_FORCERESPAWN_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTF Force Respawn Entity.
//

class CTFForceRespawn : public CPointEntity 
{
public:
	DECLARE_CLASS( CTFForceRespawn, CPointEntity );

	CTFForceRespawn();
	void Reset( void );

	void ForceRespawn( bool bSwitchTeams, int nTeam = TEAM_UNASSIGNED, bool bRemoveEverything = true );

	// Input.
	void InputForceRespawn( inputdata_t &inputdata );
	void InputForceRespawnSwitchTeams( inputdata_t &inputdata );
	void InputForceTeamRespawn( inputdata_t &inputdata );

private:

	COutputEvent	m_outputOnForceRespawn;	// Fired when the entity is done respawning the players.

	DECLARE_DATADESC();
};

#endif // ENTITY_FORCERESPAWN_H


