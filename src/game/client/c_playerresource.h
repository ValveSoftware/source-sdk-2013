//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PLAYERRESOURCE_H
#define C_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>

#define PLAYER_UNCONNECTED_NAME	"unconnected"
#define PLAYER_ERROR_NAME		"ERRORNAME"

class C_PlayerResource : public C_BaseEntity, public IGameResources
{
	DECLARE_CLASS( C_PlayerResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_PlayerResource();
	virtual			~C_PlayerResource();

public : // IGameResources interface

	// Team data access 
	virtual int		GetTeamScore( int index );
	virtual const char *GetTeamName( int index );
	virtual const Color&GetTeamColor( int index );

	// Player data access
	virtual bool	IsConnected( int index );
	virtual bool	IsAlive( int index );
	virtual bool	IsFakePlayer( int index );
	virtual bool	IsLocalPlayer( int index  );
	virtual bool	IsHLTV(int index);
	virtual bool	IsReplay(int index);

	virtual const char *GetPlayerName( int index );
	virtual int		GetPing( int index );
//	virtual int		GetPacketloss( int index );
	virtual int		GetPlayerScore( int index );
	virtual int		GetDeaths( int index );
	virtual int		GetTeam( int index );
	virtual int		GetFrags( int index );
	virtual int		GetHealth( int index );

	virtual void ClientThink();
	virtual	void	OnDataChanged(DataUpdateType_t updateType);

	virtual int		GetUserID( int index );

	uint32 GetAccountID( int iIndex );
	bool IsValid( int iIndex );

protected:
	void	UpdatePlayerName( int slot );

	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	string_t	m_szName[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iPing[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iScore[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iDeaths[MAX_PLAYERS_ARRAY_SAFE];
	bool	m_bConnected[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iTeam[MAX_PLAYERS_ARRAY_SAFE];
	bool	m_bAlive[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iHealth[MAX_PLAYERS_ARRAY_SAFE];
	Color	m_Colors[MAX_TEAMS];
	uint32	m_iAccountID[MAX_PLAYERS_ARRAY_SAFE];
	bool	m_bValid[MAX_PLAYERS_ARRAY_SAFE];
	int		m_iUserID[MAX_PLAYERS_ARRAY_SAFE];
	string_t m_szUnconnectedName;
};

extern C_PlayerResource *g_PR;

#endif // C_PLAYERRESOURCE_H
