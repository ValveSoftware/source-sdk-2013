//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Spawn Point.
//
//=============================================================================//
#ifndef ENTITY_TFSTART_H
#define ENTITY_TFSTART_H

#ifdef _WIN32
#pragma once
#endif

class CTeamControlPoint;
class CTeamControlPointRound;

//=============================================================================
//
// TF team spawning entity.
//

enum PlayerTeamSpawnMode_t
{
	PlayerTeamSpawnMode_Normal = 0,
	PlayerTeamSpawnMode_Triggered = 1,
};

enum PlayerTeamSpawn_MatchSummary_t
{
	PlayerTeamSpawn_MatchSummary_None = 0,
	PlayerTeamSpawn_MatchSummary_Loser = 1,
	PlayerTeamSpawn_MatchSummary_Winner = 2,
};

DECLARE_AUTO_LIST( ITFTeamSpawnAutoList );

class CTFTeamSpawn : public CServerOnlyPointEntity, public ITFTeamSpawnAutoList
{
public:
	DECLARE_CLASS( CTFTeamSpawn, CServerOnlyPointEntity );

	CTFTeamSpawn();

	void Activate( void );

	bool IsDisabled( void ) { return m_bDisabled; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	PlayerTeamSpawnMode_t GetTeamSpawnMode( void ) { return m_nSpawnMode; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );

	int DrawDebugTextOverlays(void);

	CHandle<CTeamControlPoint> GetControlPoint( void ) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundBlueSpawn( void ) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRedSpawn( void ) { return m_hRoundRedSpawn; }

	PlayerTeamSpawn_MatchSummary_t GetMatchSummaryType( void ){ return m_nMatchSummaryType; }
	bool AlreadyUsedForMatchSummary( void ){ return m_bAlreadyUsedForMatchSummary; }
	void SetAlreadyUsedForMatchSummary( void ){ m_bAlreadyUsedForMatchSummary = true; }

private:
	bool							m_bDisabled;		// Enabled/Disabled?
	PlayerTeamSpawnMode_t			m_nSpawnMode;		// How are players allowed to spawn here?

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;

	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;

	PlayerTeamSpawn_MatchSummary_t	m_nMatchSummaryType;		// is this a spawn location for a match summary?
	bool m_bAlreadyUsedForMatchSummary;

	DECLARE_DATADESC();
};

#endif // ENTITY_TFSTART_H


