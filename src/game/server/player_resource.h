//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYER_RESOURCE_H
#define PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class CPlayerResource : public CBaseEntity
{
	DECLARE_CLASS( CPlayerResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Init( int iIndex );
	virtual	int	 ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }
	virtual void ResourceThink( void );
	virtual void UpdatePlayerData( void );
	virtual int  UpdateTransmitState( void );
	virtual int  GetTeam( int iIndex );

protected:
	virtual void UpdateConnectedPlayer( int iIndex, CBasePlayer *pPlayer );
	virtual void UpdateDisconnectedPlayer( int iIndex );

	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	CNetworkArray( int, m_iPing, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_iScore, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_iDeaths, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_bConnected, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_iTeam, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_bAlive, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_iHealth, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( uint32, m_iAccountID, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_bValid, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkArray( int, m_iUserID, MAX_PLAYERS_ARRAY_SAFE );
		
	int	m_nUpdateCounter;
};

extern CPlayerResource *g_pPlayerResource;

#endif // PLAYER_RESOURCE_H
