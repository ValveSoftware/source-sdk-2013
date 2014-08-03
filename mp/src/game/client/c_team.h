//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TEAM_H
#define C_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "utlvector.h"
#include "client_thinklist.h"


class C_BasePlayer;

class C_Team : public C_BaseEntity
{
	DECLARE_CLASS( C_Team, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_Team();
	virtual			~C_Team();

	virtual void	PreDataUpdate( DataUpdateType_t updateType );

	// Data Handling
	virtual char	*Get_Name( void );
	virtual int		Get_Score( void );
	virtual int		Get_Deaths( void );
	virtual int		Get_Ping( void );

	// Player Handling
	virtual int		Get_Number_Players( void );
	virtual bool	ContainsPlayer( int iPlayerIndex );
	C_BasePlayer*	GetPlayer( int idx );

	// for shared code, use the same function name
	virtual int		GetNumPlayers( void ) { return Get_Number_Players(); }

	int		GetTeamNumber() const;

	int		GetRoundsWon(void) { return m_iRoundsWon; }

	void	RemoveAllPlayers();


// IClientThinkable overrides.
public:

	virtual	void				ClientThink();


public:

	// Data received from the server
	CUtlVector< int > m_aPlayers;
	char	m_szTeamname[ MAX_TEAM_NAME_LENGTH ];
	int		m_iScore;
	int		m_iRoundsWon;

	// Data for the scoreboard
	int		m_iDeaths;
	int		m_iPing;
	int		m_iPacketloss;
	int		m_iTeamNum;
};


// Global list of client side team entities
extern CUtlVector< C_Team * > g_Teams;

// Global team handling functions
C_Team *GetLocalTeam( void );
C_Team *GetGlobalTeam( int iTeamNumber );
C_Team *GetPlayersTeam( int iPlayerIndex );
C_Team *GetPlayersTeam( C_BasePlayer *pPlayer );
bool ArePlayersOnSameTeam( int iPlayerIndex1, int iPlayerIndex2 );
extern int GetNumberOfTeams( void );

#endif // C_TEAM_H
