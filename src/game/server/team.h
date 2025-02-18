//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAM_H
#define TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"

class CBasePlayer;
class CTeamSpawnPoint;

class CTeam : public CBaseEntity
{
	DECLARE_CLASS( CTeam, CBaseEntity );
public:
	CTeam( void );
	virtual ~CTeam( void );

	DECLARE_SERVERCLASS();

	virtual void Precache( void ) { return; };

	virtual void Think( void );
	virtual int  UpdateTransmitState( void );

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	virtual void		Init( const char *pName, int iNumber );

	//-----------------------------------------------------------------------------
	// Data Handling
	//-----------------------------------------------------------------------------
	virtual int			GetTeamNumber( void ) const;
	virtual const char *GetName( void ) const;
	virtual void		UpdateClientData( CBasePlayer *pPlayer );
	virtual bool		ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity ) const;

	//-----------------------------------------------------------------------------
	// Spawnpoints
	//-----------------------------------------------------------------------------
	virtual void InitializeSpawnpoints( void );
	virtual void AddSpawnpoint( CTeamSpawnPoint *pSpawnpoint );
	virtual void RemoveSpawnpoint( CTeamSpawnPoint *pSpawnpoint );
	virtual CBaseEntity *SpawnPlayer( CBasePlayer *pPlayer );

	//-----------------------------------------------------------------------------
	// Players
	//-----------------------------------------------------------------------------
	virtual void InitializePlayers( void );
	virtual void AddPlayer( CBasePlayer *pPlayer );
	virtual void RemovePlayer( CBasePlayer *pPlayer );
	virtual int  GetNumPlayers( void ) const;
	virtual CBasePlayer *GetPlayer( int iIndex ) const ;

	//-----------------------------------------------------------------------------
	// Scoring
	//-----------------------------------------------------------------------------
	virtual void AddScore( int iScore );
	virtual void SetScore( int iScore );
	virtual int  GetScore( void ) const;
	virtual void ResetScores( void );

	// Round scoring
	virtual int GetRoundsWon( void ) const { return m_iRoundsWon; }
	virtual void SetRoundsWon( int iRounds ) { m_iRoundsWon = iRounds; }
	virtual void IncrementRoundsWon( void ) { m_iRoundsWon++; }

	void AwardAchievement( int iAchievement );

	virtual int GetAliveMembers( void ) const;

public:
	CUtlVector< CTeamSpawnPoint * > m_aSpawnPoints;
	CUtlVector< CBasePlayer * >		m_aPlayers;

	// Data
	CNetworkString( m_szTeamname, MAX_TEAM_NAME_LENGTH );
	CNetworkVar( int, m_iScore );
	CNetworkVar( int, m_iRoundsWon );
	int		m_iDeaths;

	// Spawnpoints
	int		m_iLastSpawn;		// Index of the last spawnpoint used

	CNetworkVar( int, m_iTeamNum );			// Which team is this?
};

extern CUtlVector< CTeam * > g_Teams;
extern CTeam *GetGlobalTeam( int iIndex );
extern int GetNumberOfTeams( void );

#endif // TEAM_H
