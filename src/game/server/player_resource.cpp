//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "player.h"
#include "player_resource.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CPlayerResource, DT_PlayerResource)
//	SendPropArray( SendPropString( SENDINFO(m_szName[0]) ), SENDARRAYINFO(m_szName) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iPing), SendPropInt( SENDINFO_ARRAY(m_iPing), 10, SPROP_UNSIGNED ) ),
//	SendPropArray( SendPropInt( SENDINFO_ARRAY(m_iPacketloss), 7, SPROP_UNSIGNED ), m_iPacketloss ),
	SendPropArray3( SENDINFO_ARRAY3(m_iScore), SendPropInt( SENDINFO_ARRAY(m_iScore), 12 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iDeaths), SendPropInt( SENDINFO_ARRAY(m_iDeaths), 12 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bConnected), SendPropInt( SENDINFO_ARRAY(m_bConnected), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeam), SendPropInt( SENDINFO_ARRAY(m_iTeam), 4 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bAlive), SendPropInt( SENDINFO_ARRAY(m_bAlive), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iHealth), SendPropInt( SENDINFO_ARRAY(m_iHealth), -1, SPROP_VARINT | SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iAccountID), SendPropInt( SENDINFO_ARRAY(m_iAccountID), 32, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bValid), SendPropInt( SENDINFO_ARRAY(m_bValid), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iUserID ), SendPropInt( SENDINFO_ARRAY( m_iUserID ) ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CPlayerResource )

	// DEFINE_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_iPacketloss, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_iScore, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_iDeaths, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_bConnected, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_FIELD( m_flNextPingUpdate, FIELD_FLOAT ),
	// DEFINE_ARRAY( m_iTeam, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_bAlive, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_ARRAY( m_iHealth, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE ),
	// DEFINE_FIELD( m_nUpdateCounter, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( ResourceThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CPlayerResource );

CPlayerResource *g_pPlayerResource;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::Spawn( void )
{
	for ( int i=0; i < MAX_PLAYERS_ARRAY_SAFE; i++ )
	{
		Init( i );
	}

	SetThink( &CPlayerResource::ResourceThink );
	SetNextThink( gpGlobals->curtime );
	m_nUpdateCounter = 0;
}

void CPlayerResource::Init( int iIndex )
{
	m_iPing.Set( iIndex, 0 );
	m_iScore.Set( iIndex, 0 );
	m_iDeaths.Set( iIndex, 0 );
	m_bConnected.Set( iIndex, 0 );
	m_iTeam.Set( iIndex, 0 );
	m_bAlive.Set( iIndex, 0 );
	m_iHealth.Set( iIndex, 0 );
	m_iAccountID.Set( iIndex, 0 );
	m_bValid.Set( iIndex, 0 );
	m_iUserID.Set( iIndex, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: The Player resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CPlayerResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper for the virtual GrabPlayerData Think function
//-----------------------------------------------------------------------------
void CPlayerResource::ResourceThink( void )
{
	m_nUpdateCounter++;

	UpdatePlayerData();

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::UpdatePlayerData( void )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{
			UpdateConnectedPlayer( i, pPlayer );
		}
		else
		{
			UpdateDisconnectedPlayer( i );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::UpdateConnectedPlayer( int iIndex, CBasePlayer *pPlayer )
{
	m_iScore.Set( iIndex, pPlayer->FragCount() );
	m_iDeaths.Set( iIndex, pPlayer->DeathCount() );
	m_bConnected.Set( iIndex, 1 );
	m_iTeam.Set( iIndex, pPlayer->GetTeamNumber() );
	m_bAlive.Set( iIndex, pPlayer->IsAlive()?1:0 );
	m_iHealth.Set( iIndex, MAX( 0, pPlayer->GetHealth() ) );
	m_bValid.Set( iIndex, 1 );

	// Don't update ping / packetloss every time

	if ( !(m_nUpdateCounter%20) )
	{
		// update ping all 20 think ticks = (20*0.1=2seconds)
		int ping, packetloss;
		UTIL_GetPlayerConnectionInfo( iIndex, ping, packetloss );
				
		// calc avg for scoreboard so it's not so jittery
		ping = 0.8f * m_iPing.Get( iIndex ) + 0.2f * ping;
				
		m_iPing.Set( iIndex, ping );
		// m_iPacketloss.Set( iSlot, packetloss );
	}

	CSteamID steamID;
	pPlayer->GetSteamID( &steamID );
	m_iAccountID.Set( iIndex, steamID.GetAccountID() );
	m_iUserID.Set( iIndex, pPlayer->GetUserID() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::UpdateDisconnectedPlayer( int iIndex )
{
	m_bConnected.Set( iIndex, 0 );
	m_iAccountID.Set( iIndex, 0 );
	m_bValid.Set( iIndex, 0 );
	m_iUserID.Set( iIndex, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPlayerResource::GetTeam( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}