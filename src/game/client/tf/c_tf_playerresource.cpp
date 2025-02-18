//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_playerresource.h"
#include <shareddefs.h>
#include <tf_shareddefs.h>
#include "hud.h"
#include "tf_gamerules.h"
#include "tf_gc_client.h"
#include "tf_lobby_server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_mvm_respec_limit;
extern ConVar tf_mvm_buybacks_method;

C_TF_PlayerResource *g_TF_PR;

IMPLEMENT_CLIENTCLASS_DT( C_TF_PlayerResource, DT_TFPlayerResource, CTFPlayerResource )
	RecvPropArray3( RECVINFO_ARRAY( m_iTotalScore ), RecvPropInt( RECVINFO( m_iTotalScore[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxHealth ), RecvPropInt( RECVINFO( m_iMaxHealth[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxBuffedHealth ), RecvPropInt( RECVINFO( m_iMaxBuffedHealth[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerClass ), RecvPropInt( RECVINFO( m_iPlayerClass[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bArenaSpectator ), RecvPropBool( RECVINFO( m_bArenaSpectator[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iActiveDominations ), RecvPropInt( RECVINFO( m_iActiveDominations[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flNextRespawnTime ), RecvPropTime( RECVINFO( m_flNextRespawnTime[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iChargeLevel ), RecvPropInt( RECVINFO( m_iChargeLevel[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamage ), RecvPropInt( RECVINFO( m_iDamage[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamageAssist ), RecvPropInt( RECVINFO( m_iDamageAssist[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamageBoss ), RecvPropInt( RECVINFO( m_iDamageBoss[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iHealing ), RecvPropInt( RECVINFO( m_iHealing[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iHealingAssist ), RecvPropInt( RECVINFO( m_iHealingAssist[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iDamageBlocked ), RecvPropInt( RECVINFO( m_iDamageBlocked[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iCurrencyCollected ), RecvPropInt( RECVINFO( m_iCurrencyCollected[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iBonusPoints ), RecvPropInt( RECVINFO( m_iBonusPoints[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerLevel ), RecvPropInt( RECVINFO( m_iPlayerLevel[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iStreaks ), RecvPropInt( RECVINFO_ARRAY( m_iStreaks ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iUpgradeRefundCredits ), RecvPropInt( RECVINFO( m_iUpgradeRefundCredits[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iBuybackCredits ), RecvPropInt( RECVINFO( m_iBuybackCredits[0] ) ) ),
	RecvPropInt( RECVINFO( m_iPartyLeaderRedTeamIndex ) ),
	RecvPropInt( RECVINFO( m_iPartyLeaderBlueTeamIndex ) ),
	RecvPropInt( RECVINFO( m_iEventTeamStatus ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iPlayerClassWhenKilled ), RecvPropInt( RECVINFO( m_iPlayerClassWhenKilled[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iConnectionState ), RecvPropInt( RECVINFO( m_iConnectionState[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flConnectTime ), RecvPropTime( RECVINFO( m_flConnectTime[0] ) ) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TF_PlayerResource::C_TF_PlayerResource()
{
	m_Colors[TEAM_UNASSIGNED] = COLOR_TF_SPECTATOR;
	m_Colors[TEAM_SPECTATOR] = COLOR_TF_SPECTATOR;
	m_Colors[TF_TEAM_RED] = COLOR_RED;
	m_Colors[TF_TEAM_BLUE] = COLOR_BLUE;

	m_iPartyLeaderRedTeamIndex = 0;
	m_iPartyLeaderBlueTeamIndex = 0;
	m_iEventTeamStatus = 0;

	ResetPlayerScoreStats();

	g_TF_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TF_PlayerResource::~C_TF_PlayerResource()
{
	g_TF_PR = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetTeam( int iIndex )
{
	bool bValid = ( iIndex >= 1 && iIndex <= MAX_PLAYERS );
	if ( !bValid )
	{
		Assert( bValid );
		return TEAM_UNASSIGNED;
	}

	int iTeam = BaseClass::GetTeam( iIndex );

	if ( iTeam == TEAM_UNASSIGNED )
	{
		// In MvM, force everybody to show as being on the defending team,
		// even if they have not picked a team yet
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			iTeam = TF_TEAM_PVE_DEFENDERS;
		}
	}

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
MM_PlayerConnectionState_t C_TF_PlayerResource::GetPlayerConnectionState( int iIndex ) const 
{
	if ( !iIndex || ( iIndex > MAX_PLAYERS ) )
		return MM_DISCONNECTED;

	return m_iConnectionState[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetArrayValue( int iIndex, int *pArray, int iDefaultVal )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return iDefaultVal;

	return pArray[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Gets a streak value
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetStreak( unsigned int iIndex, CTFPlayerShared::ETFStreak streak_type )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iStreaks[ iIndex * CTFPlayerShared::kTFStreak_COUNT + streak_type ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetNumRespecCredits( uint32 unIndex )
{
	if ( !unIndex || unIndex > MAX_PLAYERS )
		return 0;

	if ( !tf_mvm_respec_limit.GetBool() )
		return 1;

	return GetArrayValue( unIndex, m_iUpgradeRefundCredits, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetNumBuybackCredits( uint32 unIndex )
{
	if ( !unIndex || unIndex > MAX_PLAYERS )
		return 0;

	if ( !tf_mvm_buybacks_method.GetBool() )
		return 0;

	return GetArrayValue( unIndex, m_iBuybackCredits, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer /*=false*/ )
{
	int count = 0;
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	for ( int i = 1 ; i <= MAX_PLAYERS ; i++ )
	{
		if ( bExcludeLocalPlayer && ( i == iLocalPlayerIndex ) )
		{
			continue;
		}

		if ( ( GetTeam( i ) == iTeam ) && ( GetPlayerClass( i ) == iClass ) )
		{
			count++;
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetNumPlayersForTeam( int iTeam, bool bAliveOnly )
{
	int count = 0;

	for ( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( IsConnected( playerIndex ) )
		{
			if ( GetTeam( playerIndex ) == iTeam )
			{
				if ( bAliveOnly && !IsAlive( playerIndex ) )
					continue;

				count++;
			}
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetDamage( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iDamage, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetDamageAssist( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iDamageAssist, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevDamageAssist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetDamageBoss( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iDamageBoss, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevDamageBoss;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetHealing( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iHealing, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevHealing;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetHealingAssist( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iHealingAssist, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevHealingAssist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetDamageBlocked( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iDamageBlocked, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevDamageBlocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetCurrencyCollected( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iCurrencyCollected, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevCurrencyCollected;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TF_PlayerResource::GetBonusPoints( unsigned int nIndex )
{
	Assert( nIndex < ARRAYSIZE( m_aPlayerScoreStats ) );

	return GetArrayValue( nIndex, m_iBonusPoints, 0 ) + m_aPlayerScoreStats[nIndex].m_iPrevBonusPoints;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TF_PlayerResource::UpdatePlayerScoreStats( void )
{
	for ( int playerIndex = 0; playerIndex < ARRAYSIZE( m_aPlayerScoreStats ); playerIndex++ )
	{
		// Add current round stats to the accumulator
		m_aPlayerScoreStats[playerIndex].m_iPrevDamage += GetArrayValue( playerIndex, m_iDamage, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageAssist += GetArrayValue( playerIndex, m_iDamageAssist, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageBoss += GetArrayValue( playerIndex, m_iDamageBoss, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevHealing += GetArrayValue( playerIndex, m_iHealing, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevHealingAssist += GetArrayValue( playerIndex, m_iHealingAssist, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageBlocked += GetArrayValue( playerIndex, m_iDamageBlocked, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevCurrencyCollected += GetArrayValue( playerIndex, m_iCurrencyCollected, 0 );
		m_aPlayerScoreStats[playerIndex].m_iPrevBonusPoints += GetArrayValue( playerIndex, m_iBonusPoints, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TF_PlayerResource::ResetPlayerScoreStats( int playerIndex /*= -1*/ )
{
	if ( playerIndex == -1 )
	{
		Q_memset( m_aPlayerScoreStats, 0, sizeof( m_aPlayerScoreStats ) );
	}
	else
	{
		// valid playerIndex should be 1-33 (32 players)
		Assert( playerIndex > 0 && playerIndex < ARRAYSIZE( m_aPlayerScoreStats ) );
		m_aPlayerScoreStats[playerIndex].m_iPrevDamage = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageAssist = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageBoss = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevHealing = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevHealingAssist = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevDamageBlocked = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevCurrencyCollected = 0;
		m_aPlayerScoreStats[playerIndex].m_iPrevBonusPoints = 0;
	}
}
