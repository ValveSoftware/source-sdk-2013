//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_mann_vs_machine_stats.h"
#include "filesystem.h"
#include "steamworks_gamestats.h"
#include "econ_item_description.h"
#include "econ_item_system.h"
#include "econ_item_schema.h"
#include "econ_item_constants.h"

#ifdef GAME_DLL
	#include "dt_utlvector_send.h"
	#include "tf_gamerules.h"
	#include "tf_player.h"
	#include "tf_gc_server.h"
#else
	#include "dt_utlvector_recv.h"
	#include "c_tf_player.h"
	#include "hud_macros.h"
	#include "tf_hud_mann_vs_machine_status.h"
	#include "c_tf_objective_resource.h"	
	#include "player_vs_environment/c_tf_upgrades.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_mm_trusted;

extern ConVar tf_mvm_respec_limit;
extern ConVar tf_mvm_respec_credit_goal;

static CMannVsMachineStats *g_pMVMStats = NULL;

//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
	// Avoid redef warnings
	#undef CTFPlayer
	#define CTFPlayer C_TFPlayer
	#define CMannVsMachineStats C_MannVsMachineStats
#endif

BEGIN_NETWORK_TABLE_NOBASE( CMannVsMachineWaveStats, DT_CMannVsMachineWaveStats )
#if defined( GAME_DLL )
	SendPropInt( SENDINFO( nCreditsDropped ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nCreditsAcquired ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nCreditsBonus ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nPlayerDeaths ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nBuyBacks ), 8, SPROP_UNSIGNED ),
#else
	
	RecvPropInt( RECVINFO( nCreditsDropped ) ),
	RecvPropInt( RECVINFO( nCreditsAcquired ) ),
	RecvPropInt( RECVINFO( nCreditsBonus ) ),
	RecvPropInt( RECVINFO( nPlayerDeaths ) ),
	RecvPropInt( RECVINFO( nBuyBacks ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_mann_vs_machine_stats, CMannVsMachineStats );
IMPLEMENT_NETWORKCLASS_ALIASED( MannVsMachineStats, DT_MannVsMachineStats )

BEGIN_NETWORK_TABLE( CMannVsMachineStats, DT_MannVsMachineStats )
#if defined ( GAME_DLL )
    SendPropInt( SENDINFO( m_iCurrentWaveIdx ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iServerWaveID ), 8, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT(m_runningTotalWaveStats), &REFERENCE_SEND_TABLE(DT_CMannVsMachineWaveStats) ),
	SendPropDataTable( SENDINFO_DT(m_previousWaveStats), &REFERENCE_SEND_TABLE(DT_CMannVsMachineWaveStats) ),
	SendPropDataTable( SENDINFO_DT(m_currentWaveStats), &REFERENCE_SEND_TABLE(DT_CMannVsMachineWaveStats) ),
	SendPropInt( SENDINFO( m_iCurrencyCollectedForRespec ), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO( m_nRespecsAwardedInWave ), 8, SPROP_UNSIGNED ),
#else
	RecvPropInt( RECVINFO( m_iCurrentWaveIdx ) ),
	RecvPropInt( RECVINFO( m_iServerWaveID ) ),
	RecvPropDataTable(RECVINFO_DT(m_runningTotalWaveStats), 0, &REFERENCE_RECV_TABLE(DT_CMannVsMachineWaveStats)),
	RecvPropDataTable(RECVINFO_DT(m_previousWaveStats), 0, &REFERENCE_RECV_TABLE(DT_CMannVsMachineWaveStats)),
	RecvPropDataTable(RECVINFO_DT(m_currentWaveStats), 0, &REFERENCE_RECV_TABLE(DT_CMannVsMachineWaveStats)),
	RecvPropInt( RECVINFO( m_iCurrencyCollectedForRespec ) ),
	RecvPropInt( RECVINFO( m_nRespecsAwardedInWave ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Server message that tells that a mann vs machine event occurred
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMStatsReset )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->ResetStats( );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server message that tells that a mann vs machine event occurred
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMPlayerEvent )
{
	// Deprecated.
	// Dont delete this or its HOOK_MESSAGE below or else demos will break
}

//-----------------------------------------------------------------------------
// Purpose: Reports Player Spending
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMLocalPlayerWaveSpendingValue )
{
	if ( !g_pMVMStats )
		return;

	// PlayerIdx(8), Wave(1), Type(1), Cost(2)
	uint64 playerID;
	if ( !msg.ReadBytes( &playerID, sizeof(playerID) ) )
	{
		DevMsg( " Unable to Get Player SteamID from __MsgFunc_MVMLocalPlayerWaveSpendingValue() " );
		return;
	}

	uint8 idxWave = msg.ReadByte();
	eMannVsMachineEvent eType = (eMannVsMachineEvent)msg.ReadByte();
	int16 nCost = msg.ReadWord();

	CPlayerWaveSpendingStats *pPlayerStats = g_pMVMStats->GetSpending( idxWave, playerID );
	if ( !pPlayerStats )
		return;

	CSteamID steamId( playerID );
	C_TFPlayer *pTFPlayer = ToTFPlayer( GetPlayerBySteamID( steamId ) );
	C_TFPlayer *pTFLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	switch ( eType )
	{
	case kMVMEvent_Player_BoughtInstantRespawn:
		pPlayerStats->nCreditsSpentOnBuyBacks += nCost;
		// Read the event msg and pass it on to MVMStats
		if ( pTFPlayer == pTFLocalPlayer )
		{
			g_pMVMStats->SW_ReportClientBuyBackPurchase( idxWave, nCost );
		}
		break;
	case kMVMEvent_Player_BoughtBottle:
		pPlayerStats->nCreditsSpentOnBottles += nCost;
		break;
	case kMVMEvent_Player_BoughtUpgrade:
		pPlayerStats->nCreditsSpentOnUpgrades += nCost;
		break;
	case kMVMEvent_Player_ActiveUpgrades:
		g_pMVMStats->SetPlayerActiveUpgradeCosts( playerID, nCost );
		break;
	} // switch

}
//-----------------------------------------------------------------------------
// Purpose: Server message that tells that a mann vs machine event occurred
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMResetPlayerStats )
{
	if ( !g_pMVMStats )
		return;

	uint8 playerIndex = msg.ReadByte();

	C_TFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
	if ( !pTFPlayer )
		return;

	g_pMVMStats->ResetPlayerEvents( pTFPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Handling a Server message that notifies that a player has upgraded in MvM
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMPlayerUpgradedEvent )
{
	//if ( !g_pMVMStats )
	//	return;
	//
	//// Read the data
	//// PlayerIdx(1), WaveIdx(1), ItemDef(2), AttributeDef(2), Quality(1), cost(2)
	//uint8 playerIndex		= msg.ReadByte();
	//uint8 idxWave			= msg.ReadByte();
	//uint16 nItemDef			= msg.ReadWord();
	//uint16 nAttributeDef	= msg.ReadWord();
	//uint8 nQuality			= msg.ReadByte();
	//int16 nCost				= msg.ReadWord();

	//CMannVsMachineUpgradeEvent upgrade;
	//upgrade.nItemDef = nItemDef;
	//upgrade.nAttributeDef = nAttributeDef;
	//upgrade.nQuality = nQuality;

	//C_TFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
	//C_TFPlayer *pTFLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	//// Report Stats if Local Player
	//if ( pTFPlayer && pTFLocalPlayer && ( pTFPlayer == pTFLocalPlayer ) )
	//{
	//	// Read the event msg and pass it on to MVMStats
	//	g_pMVMStats->SW_ReportClientUpgradePurchase( idxWave, nItemDef, nAttributeDef, nQuality, nCost );
	//}
}

//-----------------------------------------------------------------------------
// Message for the Local Player that it should clear its upgrade vector
USER_MESSAGE( MVMLocalPlayerUpgradesClear )
{
	if ( !g_pMVMStats )
		return;

	// Read the data
	//uint8 count = msg.ReadByte();

	g_pMVMStats->ClearLocalPlayerUpgrades();
}

//-----------------------------------------------------------------------------
// Message for the local player about an upgrade that it owns
USER_MESSAGE( MVMLocalPlayerUpgradesValue )
{
	if ( !g_pMVMStats )
		return;

	// Read the data
	// Class(1), ItemDef(2), Upgrade(1), cost(1)
	uint8 playerClass		= msg.ReadByte();
	uint16 nItemDef			= msg.ReadWord();
	//uint8 upgrade			= msg.ReadByte();
	//uint16 cost			= msg.ReadByte();

	g_pMVMStats->AddLocalPlayerUpgrade( playerClass, (item_definition_index_t)nItemDef );
}

//-----------------------------------------------------------------------------
// Message for the player that it should clear spending stats history for target wave
USER_MESSAGE( MVMResetPlayerWaveSpendingStats )
{
	if ( !g_pMVMStats )
		return;

	// Read the data
	// Wave(1)
	uint8 idxWave			= msg.ReadByte();
	
	g_pMVMStats->ClearCurrentPlayerWaveSpendingStats( idxWave );
}

//-----------------------------------------------------------------------------
// Purpose: Server message that tells that the current wave has ended and a new one has begun (including resets)
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMWaveChange )
{
	if ( !g_pMVMStats )
		return;

	// Read the data
	uint16 waveID			= msg.ReadWord();
	
	CMannVsMachinePlayerStats stats;

	stats.nDeaths			= msg.ReadByte();
	stats.nBotDamage		= msg.ReadLong();
	stats.nGiantDamage		= msg.ReadLong();
	stats.nTankDamage		= msg.ReadLong();

	// Send the Stats
	g_pMVMStats->SW_ReportClientWaveSummary( waveID, stats );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMResetPlayerUpgradeSpending )
{
	if ( !g_pMVMStats )
		return;

	uint8 playerIndex = msg.ReadByte();

	C_TFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
	if ( !pTFPlayer )
		return;

	g_pMVMStats->ResetUpgradeSpending( pTFPlayer );
}

#endif // CLIENT_DLL

//------------------------------------------------------------------------------
// CMannVsMachineStats
//------------------------------------------------------------------------------
CMannVsMachineStats::CMannVsMachineStats()
{
	g_pMVMStats = this;

#ifdef GAME_DLL
	SetCurrentWave( 0 );
	Q_memset( m_playerStats, 0, sizeof( m_playerStats ) );
#else
	m_iCurrentWaveIdx = 0;
	
	m_teamActiveUpgrades.Purge();
	m_teamActiveUpgrades.SetLessFunc( DefLessFunc (uint64) );
#endif
	m_currWaveSpendingStats.SetLessFunc( DefLessFunc (uint64) );
	m_prevWaveSpendingStats.SetLessFunc( DefLessFunc (uint64) );
	m_allPrevWaveSpendingStats.SetLessFunc( DefLessFunc (uint64) );
	
	m_iCurrencyCollectedForRespec = 0;
	m_nRespecsAwardedInWave = 0;
	//m_nEventID = 0;
}

//-----------------------------------------------------------------------------
// Purpose: DTOR
//-----------------------------------------------------------------------------
CMannVsMachineStats::~CMannVsMachineStats()
{
	g_pMVMStats = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetStats( )
{
#ifdef GAME_DLL
	// notify everyone to reset everything
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMStatsReset" );
	MessageEnd();

	SetCurrentWave( 0 );

	m_runningTotalWaveStats.ClearStats();
	m_previousWaveStats.ClearStats();
	m_currentWaveStats.ClearStats();

#else
	m_vecLocalPlayerUpgrades.Purge();
	m_teamActiveUpgrades.Purge();
#endif

	m_currWaveSpendingStats.RemoveAll();
	m_prevWaveSpendingStats.RemoveAll();
	m_allPrevWaveSpendingStats.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetPlayerEvents( CTFPlayer *pTFPlayer )
{
	if ( pTFPlayer->IsBot() )
		return;

#ifdef GAME_DLL
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMResetPlayerStats" );
	WRITE_BYTE( (uint8)pTFPlayer->entindex() );
	MessageEnd();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetUpgradeSpending( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	if ( pTFPlayer->IsBot() )
		return;

	CSteamID steamID;
	if ( pTFPlayer && pTFPlayer->GetSteamID( &steamID ) )
	{
		int iWave = m_currWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_currWaveSpendingStats.InvalidIndex() )
		{
			m_currWaveSpendingStats[iWave].nCreditsSpentOnUpgrades = 0;
		}

		iWave = m_allPrevWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_allPrevWaveSpendingStats.InvalidIndex() )
		{
			m_allPrevWaveSpendingStats[iWave].nCreditsSpentOnUpgrades = 0;
		}

		iWave = m_prevWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_prevWaveSpendingStats.InvalidIndex() )
		{
			m_prevWaveSpendingStats[iWave].nCreditsSpentOnUpgrades = 0;
		}
	}

#ifdef GAME_DLL
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMResetPlayerUpgradeSpending" );
	WRITE_BYTE( (uint8)pTFPlayer->entindex() );
	MessageEnd();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns acquired credits - default is all waves
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetAcquiredCredits( int iWaveIdx, bool bWithBonus /*= true*/  )
{
	CMannVsMachineLocalWaveStats waveStats = GetWaveStats( iWaveIdx );

	if ( bWithBonus )
		return waveStats.nCreditsAcquired + waveStats.nCreditsBonus;
	else
		return waveStats.nCreditsAcquired;
}

//-----------------------------------------------------------------------------
// Purpose: Returns dropped credits for a target wave.  If target wave is -1, returns sum of all waves
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetDroppedCredits( int iWaveIdx )
{
	CMannVsMachineLocalWaveStats waveStats = GetWaveStats( iWaveIdx );
	return waveStats.nCreditsDropped;
}

//-----------------------------------------------------------------------------
// Number of Credits missed (dropped and not collected)
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetMissedCredits( int iWaveIdx )
{
	CMannVsMachineLocalWaveStats waveStats = GetWaveStats( iWaveIdx );
	return waveStats.nCreditsDropped - waveStats.nCreditsAcquired;
}

//-----------------------------------------------------------------------------
// Purpose: Returns credit bonus for a target wave.  If target wave is -1, returns sum of all waves
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetBonusCredits( int iWaveIdx )
{
	CMannVsMachineLocalWaveStats waveStats = GetWaveStats( iWaveIdx );
	return waveStats.nCreditsBonus;
}
//=============================================================================//
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundOver( bool bHumansWon )
{
	// Send Stats to OGS
	// Increment server wave id since victory uses the previous "wave" and this one has yet to be incremented
	m_iServerWaveID++;
	SW_ReportWaveSummary( m_iCurrentWaveIdx, bHumansWon );
	m_iServerWaveID--;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SetCurrentWave( uint32 idxWave )
{
	// Advancing Wave, compact in the stats
	if ( idxWave > m_iCurrentWaveIdx )
	{
		m_runningTotalWaveStats += m_previousWaveStats;
		m_previousWaveStats = m_currentWaveStats;
	}

	if ( idxWave == 0 )
	{
		// Purge it all
		m_runningTotalWaveStats.ClearStats();
		m_previousWaveStats.ClearStats();
		m_currentWaveStats.ClearStats();
	}
	
	m_iCurrentWaveIdx = idxWave;
	
	// Notify Players of Reset of current Wave for PlayerStats
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();

	UserMessageBegin( filter, "MVMResetPlayerWaveSpendingStats" );
	WRITE_BYTE( (uint8)m_iCurrentWaveIdx );
	MessageEnd();

	ResetWaveStats();
	OnStatsChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_WaveStart()
{
	// Starting a new wave
	// Report the previous successful wave.
	if ( m_iCurrentWaveIdx > 0 && m_currentWaveStats.nAttempts == 0)
	{
		SW_ReportWaveSummary( m_iCurrentWaveIdx - 1, true );
	}

	// reset stats
	ResetWaveStats();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_WaveEnd( bool bSuccess )
{
	// if failed, report immediately, 
	// report successes on starting the next wave because of currency picked up late
	ConVar *sv_cheats = g_pCVar->FindVar( "sv_cheats" );
	if ( !bSuccess && ( !sv_cheats || !sv_cheats->GetBool() ) )
	{
		m_currentWaveStats.nAttempts++;
		SW_ReportWaveSummary( m_iCurrentWaveIdx, false );
		ResetWaveStats();
		OnStatsChanged();

		CBroadcastRecipientFilter filter;
		filter.MakeReliable();
		UserMessageBegin( filter, "MVMWaveFailed" );
		MessageEnd();
	}	

	// Send a User message to Clients to send there data
	// Tell each client their damage
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];
		
		int nPlayerEntIdx = player->entindex();
		if ( !IsIndexIntoPlayerArrayValid(nPlayerEntIdx) )
			continue;
		
		CMannVsMachinePlayerStats stats = m_playerStats[ nPlayerEntIdx ];

		CSingleUserRecipientFilter filter( player );
		filter.MakeReliable();

		// ServerWaveID(2), deaths(1), damageBot(4), damageGiant(4), damageTank(4)
		UserMessageBegin( filter, "MVMWaveChange" );
		WRITE_WORD( (uint16)m_iServerWaveID );
		WRITE_BYTE( (uint8)stats.nDeaths );
		WRITE_LONG( stats.nBotDamage );
		WRITE_LONG( stats.nGiantDamage );
		WRITE_LONG( stats.nTankDamage );
		MessageEnd();
	}

	// Increment the ServerId
	m_iServerWaveID++;
	Q_memset( m_playerStats, 0, sizeof( m_playerStats ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_AcquiredCredits( uint32 idxWave, int nAmount, bool bIsBonus )
{
	if ( idxWave == (int)m_iCurrentWaveIdx )
	{
		if ( bIsBonus )
		{
			m_currentWaveStats.nCreditsBonus += nAmount;
		}
		else 
		{
			m_currentWaveStats.nCreditsAcquired += nAmount;
		}
	}
	else if ( idxWave == m_iCurrentWaveIdx - 1 )
	{
		if ( bIsBonus )
		{
			m_previousWaveStats.nCreditsBonus += nAmount;
		}
		else 
		{
			m_previousWaveStats.nCreditsAcquired += nAmount;
		}
	}

	OnStatsChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_CreditsDropped( uint32 idxWave, int nAmount )
{
	if ( idxWave == m_iCurrentWaveIdx )
	{
		m_currentWaveStats.nCreditsDropped += nAmount;
	}
	else if ( idxWave == m_iCurrentWaveIdx - 1 )
	{
		m_previousWaveStats.nCreditsDropped += nAmount;
	}
	OnStatsChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_PointsChanged( CTFPlayer *pTFPlayer, int nPoints )
{
	//
	// Deprecated
	//

	//if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
	//	return;

	//OnStatsChanged();
	//NotifyPlayerEvent( pTFPlayer, m_iCurrentWaveIdx, kMVMEvent_Player_Points, nPoints );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_Died( CTFPlayer *pTFPlayer )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
		return;
		
	int nPlayerEntIdx = pTFPlayer->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nPlayerEntIdx) )
		return;

	m_playerStats[nPlayerEntIdx].nDeaths += 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_Upgraded( CTFPlayer *pTFPlayer, uint16 nItemDef, uint16 nAttributeDef, uint8 nQuality, int16 nCost, bool bIsBottle )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
		return;

	OnStatsChanged();

	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	
	// PlayerIdx(1), WaveIdx(1), ItemDef(2), AttributeDef(2), Quality(1), cost(2)
	// send user message
	UserMessageBegin( filter, "MVMPlayerUpgradedEvent" );
	WRITE_BYTE( (uint8)pTFPlayer->entindex() );
	WRITE_BYTE( (uint8)m_iCurrentWaveIdx );
	WRITE_WORD( nItemDef );
	WRITE_WORD( nAttributeDef );
	WRITE_BYTE( nQuality);
	WRITE_WORD( nCost );
	MessageEnd();

	if ( bIsBottle )
	{
		NotifyTargetPlayerEvent( pTFPlayer, m_iCurrentWaveIdx, kMVMEvent_Player_BoughtBottle, nCost );
	}
	else
	{
		NotifyTargetPlayerEvent( pTFPlayer, m_iCurrentWaveIdx, kMVMEvent_Player_BoughtUpgrade, nCost );
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_PickedUpCredits( CTFPlayer *pTFPlayer, uint32 idxWave, int nCreditsAmount )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive()  )
		return;

	OnStatsChanged();
	NotifyPlayerEvent( pTFPlayer, idxWave, kMVMEvent_Player_PickedUpCredits, nCreditsAmount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_BoughtInstantRespawn( CTFPlayer *pTFPlayer, int nCost )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive()  )
		return;

	m_currentWaveStats.nBuyBacks++;
	OnStatsChanged();

	NotifyTargetPlayerEvent( pTFPlayer, m_iCurrentWaveIdx, kMVMEvent_Player_BoughtInstantRespawn, nCost );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealtDamageToBots( CTFPlayer *pTFPlayer, int damage )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
		return;
		
	int nPlayerEntIdx = pTFPlayer->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nPlayerEntIdx) )
		return;

	m_playerStats[nPlayerEntIdx].nBotDamage += damage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealtDamageToGiants( CTFPlayer *pTFPlayer, int damage )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
		return;

	int nPlayerEntIdx = pTFPlayer->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nPlayerEntIdx) )
		return;

	m_playerStats[nPlayerEntIdx].nGiantDamage += damage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealtDamageToTanks( CTFPlayer *pTFPlayer, int damage )
{
	if ( pTFPlayer->IsBot() || !TFGameRules() || !TFGameRules()->IsPVEModeActive() )
		return;

	m_playerStats[pTFPlayer->entindex()].nTankDamage += damage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::NotifyPlayerEvent( CTFPlayer *pTFPlayer, uint32 idxWave, eMannVsMachineEvent eType, int nValue, int nParam )
{
	// Deprecated 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::NotifyTargetPlayerEvent( CTFPlayer *pTFPlayer, uint32 idxWave, eMannVsMachineEvent eType, int nCost )
{
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();

	// Client
	UserMessageBegin( filter, "MVMLocalPlayerWaveSpendingValue" );
	uint64 id = pTFPlayer->GetSteamIDAsUInt64();

	WRITE_BITS( &id, 64 );
	WRITE_BYTE( (uint8)idxWave );
	WRITE_BYTE( (uint8)eType );
	WRITE_WORD( (uint16)nCost );
	MessageEnd();

	// Server
	uint64 steamID = pTFPlayer->GetSteamIDAsUInt64();
	CPlayerWaveSpendingStats *pPlayerStats = g_pMVMStats->GetSpending( idxWave, steamID );
	if ( !pPlayerStats )
		return;

	switch ( eType )
	{
	case kMVMEvent_Player_BoughtInstantRespawn:
		// Don't track refunds (permanent cost)
		pPlayerStats->nCreditsSpentOnBuyBacks += nCost;
		break;
	case kMVMEvent_Player_BoughtBottle:
		// Don't track refunds (permanent cost)
		pPlayerStats->nCreditsSpentOnBottles += nCost;
		break;
	case kMVMEvent_Player_BoughtUpgrade:
		pPlayerStats->nCreditsSpentOnUpgrades += nCost;
		break;
	}
}

//-----------------------------------------------------------------------------
// Tell the Player What Upgrades they have
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SendUpgradesToPlayer( CTFPlayer *pTFPlayer, CUtlVector< CUpgradeInfo > *upgrades )
{
	CRecipientFilter filter;
	filter.AddRecipient( pTFPlayer );

	if ( upgrades == NULL )
	{
		UserMessageBegin( filter, "MVMLocalPlayerUpgradesClear" );
		WRITE_BYTE( 0 );
		MessageEnd();
		return;
	}

	// Send the reset command
	UserMessageBegin( filter, "MVMLocalPlayerUpgradesClear" );
	WRITE_BYTE( 0 );
	MessageEnd();

	// send all the upgrades
	// Class(1), ItemDef(2), Upgrade(1), cost(2)
	for ( int i = 0; i < upgrades->Count(); ++i )
	{
		UserMessageBegin( filter, "MVMLocalPlayerUpgradesValue" );
		WRITE_BYTE( (uint8)upgrades->Element(i).m_iPlayerClass );
		WRITE_WORD( (uint16)upgrades->Element(i).m_itemDefIndex );
		WRITE_BYTE( (uint8)upgrades->Element(i).m_upgrade );
		WRITE_WORD( (uint16)upgrades->Element(i).m_nCost );
		MessageEnd();
	}
}

//=============================================================================//
void CMannVsMachineStats::NotifyPlayerActiveUpgradeCosts( CTFPlayer *pTFPlayer, int nSpending )
{
	NotifyTargetPlayerEvent( pTFPlayer, 0, kMVMEvent_Player_ActiveUpgrades, nSpending );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Clear The Player spending stats for this wave
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ClearCurrentPlayerWaveSpendingStats( int idxWave )
{
	// If the wave is different then the current, we're changing waves and need to save this stuff off if we're advancing

	if ( idxWave != (int)m_iCurrentWaveIdx )
	{
		// if wave incremented, we changed waves to save off the previous wave data
		m_prevWaveSpendingStats.RemoveAll();

		FOR_EACH_MAP_FAST( m_currWaveSpendingStats, i )
		{
			// Save off current wave in to previous
			int iWave = m_prevWaveSpendingStats.Insert(m_currWaveSpendingStats.Key(i), CPlayerWaveSpendingStats() );
			m_prevWaveSpendingStats[iWave] = m_currWaveSpendingStats[i];

			// Append current wave in to all prev
			int allIndex = m_allPrevWaveSpendingStats.Find( m_currWaveSpendingStats.Key(i) );
			if ( allIndex != m_allPrevWaveSpendingStats.InvalidIndex() )
			{
				m_allPrevWaveSpendingStats[allIndex] += m_currWaveSpendingStats[i];
			}
			else
			{
				allIndex = m_allPrevWaveSpendingStats.Insert( m_currWaveSpendingStats.Key(i), CPlayerWaveSpendingStats() );
				m_allPrevWaveSpendingStats[allIndex] = m_currWaveSpendingStats[i];
			}
		}
	}
	// Purge away current stats
	m_currWaveSpendingStats.RemoveAll();
}

//-----------------------------------------------------------------------------
CPlayerWaveSpendingStats *CMannVsMachineStats::GetSpending( int iWaveIndex, uint64 steamId )
{
	if ( iWaveIndex == (int)m_iCurrentWaveIdx )
	{
		int iWave = m_currWaveSpendingStats.Find( steamId );
		if ( iWave != m_currWaveSpendingStats.InvalidIndex() )
		{
			return &m_currWaveSpendingStats[iWave];
		}
		else
		{
			iWave = m_currWaveSpendingStats.Insert( steamId, CPlayerWaveSpendingStats() );
			return &m_currWaveSpendingStats[iWave];
		}
	}
	else if ( iWaveIndex == -1 )
	{
		int iWave = m_allPrevWaveSpendingStats.Find( steamId );
		if ( iWave != m_allPrevWaveSpendingStats.InvalidIndex() )
		{
			return &m_allPrevWaveSpendingStats[iWave];
		}
		else
		{
			iWave = m_allPrevWaveSpendingStats.Insert( steamId, CPlayerWaveSpendingStats() );
			return &m_allPrevWaveSpendingStats[iWave];
		}
	}
	else if ( iWaveIndex == (int)(m_iCurrentWaveIdx - 1) )
	{
		int iWave = m_prevWaveSpendingStats.Find( steamId );
		if ( iWave != m_prevWaveSpendingStats.InvalidIndex() )
		{
			return &m_prevWaveSpendingStats[iWave];
		}
		else
		{
			iWave = m_prevWaveSpendingStats.Insert( steamId, CPlayerWaveSpendingStats() );
			return &m_prevWaveSpendingStats[iWave];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetUpgradeSpending( CTFPlayer *pTFPlayer /*= NULL*/ )
{
	int spending = 0;

	CSteamID steamID;
	if ( pTFPlayer && pTFPlayer->GetSteamID( &steamID ) )
	{
		int iWave = m_currWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_currWaveSpendingStats.InvalidIndex() )
		{
			spending += m_currWaveSpendingStats[iWave].nCreditsSpentOnUpgrades;
		}

		iWave = m_allPrevWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_allPrevWaveSpendingStats.InvalidIndex() )
		{
			spending += m_allPrevWaveSpendingStats[iWave].nCreditsSpentOnUpgrades;
		}
	}
	else
	{
		FOR_EACH_MAP_FAST ( m_currWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_currWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_currWaveSpendingStats[iPlayer].nCreditsSpentOnUpgrades;
			}
		}

		FOR_EACH_MAP_FAST ( m_allPrevWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_allPrevWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_allPrevWaveSpendingStats[iPlayer].nCreditsSpentOnUpgrades;
			}
		}
	}

	return spending;
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetBottleSpending( CTFPlayer *pTFPlayer /*= NULL*/ )
{
	int spending = 0;
	
	CSteamID steamID;
	if ( pTFPlayer && pTFPlayer->GetSteamID( &steamID ) )
	{
		int iWave = m_currWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_currWaveSpendingStats.InvalidIndex() )
		{
			spending += m_currWaveSpendingStats[iWave].nCreditsSpentOnBottles;
		}

		iWave = m_allPrevWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_allPrevWaveSpendingStats.InvalidIndex() )
		{
			spending += m_allPrevWaveSpendingStats[iWave].nCreditsSpentOnBottles;
		}
	}
	else
	{
		FOR_EACH_MAP_FAST ( m_currWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_currWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_currWaveSpendingStats[iPlayer].nCreditsSpentOnBottles;
			}
		}

		FOR_EACH_MAP_FAST ( m_allPrevWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_allPrevWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_allPrevWaveSpendingStats[iPlayer].nCreditsSpentOnBottles;
			}
		}
	}

	return spending;
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetBuyBackSpending( CTFPlayer *pTFPlayer /*= NULL*/ )
{
	int spending = 0;

	CSteamID steamID;
	if ( pTFPlayer && pTFPlayer->GetSteamID( &steamID ) )
	{
		int iWave = m_currWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_currWaveSpendingStats.InvalidIndex() )
		{
			spending += m_currWaveSpendingStats[iWave].nCreditsSpentOnBuyBacks;
		}

		iWave = m_allPrevWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( iWave != m_allPrevWaveSpendingStats.InvalidIndex() )
		{
			spending += m_allPrevWaveSpendingStats[iWave].nCreditsSpentOnBuyBacks;
		}
	}
	else
	{
		FOR_EACH_MAP_FAST ( m_currWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_currWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_currWaveSpendingStats[iPlayer].nCreditsSpentOnBuyBacks;
			}
		}

		FOR_EACH_MAP_FAST ( m_allPrevWaveSpendingStats, iPlayer )
		{
			CBasePlayer * pPlayer = GetPlayerBySteamID( CSteamID( m_allPrevWaveSpendingStats.Key( iPlayer ) ) );
			if ( pPlayer )
			{
				spending += m_allPrevWaveSpendingStats[iPlayer].nCreditsSpentOnBuyBacks;
			}
		}
	}

	return spending;
}

//=============================================================================//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::OnDataChanged( DataUpdateType_t updateType )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: Clear Local Player Upgrades
//-----------------------------------------------------------------------------
CPlayerWaveSpendingStats *CMannVsMachineStats::GetLocalSpending ( int iWaveIdx )
{
	CSteamID steamId;
	if ( C_TFPlayer::GetLocalTFPlayer()->GetSteamID( &steamId ) )
	{
		return GetSpending( iWaveIdx, steamId.ConvertToUint64() );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Set the active upgrade costs for this player
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SetPlayerActiveUpgradeCosts( uint64 steamId, int nSpending )
{
	int iUpgrade = m_teamActiveUpgrades.Find(steamId);
	if ( iUpgrade != m_teamActiveUpgrades.InvalidIndex() )
	{
		m_teamActiveUpgrades[iUpgrade] = nSpending;
	}
	else
	{
		m_teamActiveUpgrades.Insert( steamId, nSpending );
	}
}

//-----------------------------------------------------------------------------
// Get Player Active Upgrade Costs
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetPlayerActiveUpgradeCosts( uint64 steamId64 )
{
	// 0, give team total
	if ( steamId64 == 0 )
	{
		int nSpending = 0;
		FOR_EACH_MAP_FAST ( m_teamActiveUpgrades, i )
		{
			CSteamID steamId( m_teamActiveUpgrades.Key( i ) );
			CBasePlayer * pPlayer = GetPlayerBySteamID( steamId );
			if ( pPlayer )
			{
				nSpending += m_teamActiveUpgrades[ i ];
			}
		}

		return nSpending;
	}
	else 
	{
		CSteamID steamId( steamId64 );
		CBasePlayer * pPlayer = GetPlayerBySteamID( steamId );
		if ( pPlayer )
		{
			int iUpgrade = m_teamActiveUpgrades.Find( steamId64 );
			if ( iUpgrade != m_teamActiveUpgrades.InvalidIndex() )
				return m_teamActiveUpgrades[iUpgrade];
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Clear Local Player Upgrades
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ClearLocalPlayerUpgrades ()
{
	m_vecLocalPlayerUpgrades.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Add Local Player Upgrade
//-----------------------------------------------------------------------------
void CMannVsMachineStats::AddLocalPlayerUpgrade ( int iPlayerClass, item_definition_index_t iItemDef )
{
	CUpgradeInfo upgrade;
	upgrade.m_iPlayerClass = iPlayerClass;
	upgrade.m_itemDefIndex = iItemDef;
	upgrade.m_upgrade = 0;
	upgrade.m_nCost = 0;
	
	m_vecLocalPlayerUpgrades.AddToTail( upgrade );
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerUpgradeSpending( int idxWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( idxWave );
	return pStats ? pStats->nCreditsSpentOnUpgrades : 0;
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerBottleSpending( int idxWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( idxWave );
	return pStats ? pStats->nCreditsSpentOnBottles : 0;
}

//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerBuyBackSpending ( int idxWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( idxWave );
	return pStats ? pStats->nCreditsSpentOnBuyBacks : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Submits the use of MvM Credits
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientUpgradePurchase( uint8 waveIdx, uint16 nItemDef, uint16 nAttributeDef, uint8 nQuality, int16 nCost )
{
	// Disabled due to bad key indexing.
	// Make a V2 if this data is wanted

// Old Table
	//-----------------------------------------------------------------------------
	// OGS TF2ClientMvMWaveCreditSpend: MvM mode - credit spend
	//-----------------------------------------------------------------------------
	//START_TABLE( k_ESchemaCatalogOGS, TF2ClientMvMWaveCreditSpend, TABLE_PROP_NORMAL )
	//INT_FIELD( nID, ID, int32 )										// Auto-increment fake key
	//INT_FIELD( llSessionID, SessionID, uint64 )
	//INT_FIELD( llServerSessionID, ServerSessionID, uint64 )			// Duplicated data from Session table due to game client anomalies
	//INT_FIELD( nServerWaveID, ServerWaveID, int32 )					// Incrementing WaveId PerAttempt
	//INT_FIELD( ulAccountID, AccountID, uint64 )						// Duplicated data from Session table due to game client anomalies
	//INT_FIELD( nWaveNumber, WaveNumber, uint8 )
	//INT_FIELD( nItemCategoryID, MvMItemCategoryID, int16 )			// References TF2MvMItemCategoryReference
	//INT_FIELD( nItemNameID, MvMItemNameID, int32 )					// References TF2MvMItemNameReference
	//INT_FIELD( nItemCost, ItemCost, int32 )
	//INT_FIELD( RTime32EventTime, EventTime, RTime32 ) 
	//PRIMARY_KEY_CLUSTERED( 100, nID )
	//WIPE_TABLE_BETWEEN_TESTS( k_EWipePolicyWipeForAllTests )
	//AUTOINCREMENT_FIELD( nID )
	//PARTITION_INTERVAL( k_EPartitionIntervalDaily ) 
	//OWNING_APPLICATION( 440 )
	//END_TABLE



//#if !defined(NO_STEAM)
//	ConVar *sv_cheats = g_pCVar->FindVar( "sv_cheats" );
//	if ( sv_cheats && sv_cheats->GetBool() )
//		return;
//
//	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
//	if ( !pTFPlayer )
//		return;
//
//	// Get Data needed for this report
//	const CEconItemAttributeDefinition *pAttributeDef = ItemSystem()->GetItemSchema()->GetAttributeDefinition( nAttributeDef );
//	if ( !pAttributeDef )
//		return;
//
//	float flValue = 1.0f;
//	if ( !MannVsMachine_GetUpgradeInfo( pAttributeDef->GetDefinitionIndex(), nQuality, flValue ) )
//		return;
//
//	CSteamID steamIDForPlayer;
//	if ( !pTFPlayer->GetSteamID( &steamIDForPlayer ) )
//		return;
//
//	KeyValuesAD pKeyValues( "data" );
//
//	// Create and Send the report
//	//SessionId, ServerSessionId, AccountID, ServerWaveID, WaveNumber, MvMItemCategoryID, MvMItemNameID, ItemCost, EventTime
//	KeyValues* pKVData = new KeyValues( "TF2ClientMvMWaveCreditSpend" );
//	pKVData->SetInt( "ID", ++m_nEventID );
//
//	// ServerSessionID - Auto
//	// SessionID - Auto
//
//	// AccountID
//	pKVData->SetUint64( "AccountID", steamIDForPlayer.ConvertToUint64() );
//
//	//ServerWaveID - Use provided ID not m_iServerWaveID as that may have changed
//	pKVData->SetInt( "ServerWaveID", m_iServerWaveID );
//
//	// WaveNumber
//	pKVData->SetInt( "WaveNumber", waveIdx + 1 );
//
//	// MvMItemNameID
//	if ( nItemDef )
//	{
//		const CEconItemDefinition *pItemDef = ItemSystem()->GetItemSchema()->GetItemDefinition( nItemDef );
//		pKVData->SetString( "MvMItemNameID", pItemDef->GetDefinitionName() );
//	}
//	else 
//	{
//		pKVData->SetString( "MvMItemNameID", g_aPlayerClassNames_NonLocalized[ pTFPlayer->GetPlayerClass()->GetClassIndex() ] );
//	}
//
//	// MvMItemCategoryID
//	pKVData->SetString( "MvMItemCategoryID", pAttributeDef->GetDefinitionName() );
//	
//	// ItemCost
//	pKVData->SetInt( "ItemCost", nCost );
//
//	// EventTime
//	pKVData->SetInt( "EventTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
//
//	// Send to DB
//	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
//
//#endif // !defined(NO_STEAM)
}

//-----------------------------------------------------------------------------
// Purpose: Submits the use of MvM Credits
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientBuyBackPurchase ( uint8 waveIdx, uint16 nCost )
{
//#if !defined(NO_STEAM)
//	ConVar *sv_cheats = g_pCVar->FindVar( "sv_cheats" );
//	if ( sv_cheats && sv_cheats->GetBool() )
//		return;
//
//	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
//	if ( !pTFPlayer )
//		return;
//
//	CSteamID steamIDForPlayer;
//	if ( !pTFPlayer->GetSteamID( &steamIDForPlayer ) )
//		return;
//
//	// Create and Send the report
//	//SessionId, ServerSessionId, AccountID, ServerWaveID, WaveNumber, MvMItemCategoryID, MvMItemNameID, ItemCost, EventTime
//	KeyValues* pKVData = new KeyValues( "TF2ClientMvMWaveCreditSpend" );
//
//	// ID - Auto
//	// ServerSessionID - Auto
//	// SessionID - Auto
//
//	// AccountID
//	pKVData->SetUint64( "AccountID", steamIDForPlayer.ConvertToUint64() );
//
//	//ServerWaveID - Use provided ID not m_iServerWaveID as that may have changed
//	pKVData->SetInt( "ServerWaveID", m_iServerWaveID );
//
//	// WaveNumber
//	pKVData->SetInt( "WaveNumber", waveIdx + 1 );
//
//	// MvMItemNameID
//	static char szAnsi[64];
//	g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( g_aPlayerClassNames[ pTFPlayer->GetPlayerClass()->GetClassIndex() ] ), szAnsi, sizeof(szAnsi) );
//	pKVData->SetString( "MvMItemNameID", szAnsi );
//
//	// MvMItemCategoryID
//	pKVData->SetString( "MvMItemCategoryID", "BuyBack");
//
//	// ItemCost
//	pKVData->SetInt( "ItemCost", nCost );
//
//	// EventTime
//	pKVData->SetInt( "EventTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
//
//	// Send to DB
//	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
//
//#endif // !defined(NO_STEAM)
}

//-----------------------------------------------------------------------------
// Purpose: Submits the use of MvM Credits
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientWaveSummary( uint16 waveID, CMannVsMachinePlayerStats stats )
{
	// Disabled due to bad key indexing.
	// Make a V2 if this data is wanted
	// Old Table
	//-----------------------------------------------------------------------------   
	// OGS TF2ClientMvMWaveSummary: MvM mode � Player wave summary information   
	//-----------------------------------------------------------------------------   
	//START_TABLE( k_ESchemaCatalogOGS, TF2ClientMvMWaveSummary, TABLE_PROP_NORMAL )   
	//INT_FIELD( nID, ID, int32 )                                           // Auto-increment fake key
	//INT_FIELD( nServerWaveID, ServerWaveID, int32 )                       // ID of ServerMvMWaveSummary Entry this is linked with, How do we get this?
	//INT_FIELD( llSessionID, SessionID, uint64 )
	//INT_FIELD( llServerSessionID, ServerSessionID, uint64 )               // Duplicated data from Session table due to game client anomalies
	//INT_FIELD( ulAccountID, AccountID, uint64 )                           // Duplicated data from Session table due to game client anomalies                                             
	//INT_FIELD( nDamageToBots, DamageToBots, int32 )
	//INT_FIELD( nDamageToTanks, DamageToTanks, int32 )
	//INT_FIELD( nDamageToGiants, DamageToGiants, int32 )
	//INT_FIELD( nDeaths, Deaths, uint8 )
	//INT_FIELD( nClass, Class, int64 )                                     // References TF2ClassReference
	//INT_FIELD( nPrimaryWeapon, PrimaryWeapon, int32 )                     // References TF2MvMItemNameReference   
	//INT_FIELD( nSecondaryWeapon, SecondaryWeapon, int32 )                 // References TF2MvMItemNameReference   
	//INT_FIELD( nMeleeWeapon, MeleeWeapon, int32 )                         // References TF2MvMItemNameReference   
	//INT_FIELD( bHasBottle, HasBottle, int8 )
	//PRIMARY_KEY_CLUSTERED( 100, nID )
	//WIPE_TABLE_BETWEEN_TESTS( k_EWipePolicyWipeForAllTests )
	//AUTOINCREMENT_FIELD( nID )
	//PARTITION_INTERVAL( k_EPartitionIntervalDaily )
	//OWNING_APPLICATION( 440 )
	//END_TABLE


//#if !defined(NO_STEAM)
//	ConVar *sv_cheats = g_pCVar->FindVar( "sv_cheats" );
//	if ( sv_cheats && sv_cheats->GetBool() )
//		return;
//
//	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
//	if ( !pTFPlayer )
//		return;
//
//	CSteamID steamIDForPlayer;
//	if ( !pTFPlayer->GetSteamID( &steamIDForPlayer ) )
//		return;
//
//	// Class
//	int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
//
//	// Invalid Class, Spectators?
//	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
//		return;
//
//	// Create and Send the report
//	//[ID]	,[ServerWaveID]	,[SessionID]	,[AccountID]	,[DamageToBots]	,[DamageToTanks]	,[DamageToGiants]	
//	//,[Deaths]	,[Class]	,[PrimaryWeapon]	,[SecondaryWeapon]	,[MeleeWeapon]	,[HasBottle]
//	KeyValues* pKVData = new KeyValues( "TF2ClientMvMWaveSummary" );
//	pKVData->SetInt( "ID", ++m_nEventID );
//
//	// ServerSessionID - Auto
//	// SessionID - Auto
//
//	// AccountID
//	pKVData->SetUint64( "AccountID", steamIDForPlayer.ConvertToUint64() );
//
//	//Damage
//	pKVData->SetInt( "DamageToBots", stats.nBotDamage );
//	pKVData->SetInt( "DamageToGiants", stats.nGiantDamage );
//	pKVData->SetInt( "DamageToTanks", stats.nTankDamage );
//
//	// Death
//	pKVData->SetInt( "Deaths", stats.nDeaths );
//	
//	//ServerWaveID - Use provided ID not m_iServerWaveID as that may have changed
//	pKVData->SetInt( "ServerWaveID", waveID );
//
//	static char szAnsi[512];
//	pKVData->SetString( "Class", g_aPlayerClassNames_NonLocalized[ iClass ] );
//
//	// Primary
//	CEconItemView *pPrimaryView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, LOADOUT_POSITION_PRIMARY );
//	const CEconItemDefinition *pPrimaryDef = pPrimaryView->GetItemDefinition();
//
//	if ( pPrimaryDef )
//	{
//		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find ( pPrimaryDef->GetItemBaseName() ) , szAnsi, sizeof(szAnsi) );
//		pKVData->SetString( "PrimaryWeapon", szAnsi );
//	}
//
//	// Secondary
//	CEconItemView *pSecondaryView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, LOADOUT_POSITION_SECONDARY );
//	const CEconItemDefinition *pSecondaryDef = pSecondaryView->GetItemDefinition();
//
//	if ( pSecondaryDef )
//	{
//		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find ( pSecondaryDef->GetItemBaseName() ) , szAnsi, sizeof(szAnsi) );
//		pKVData->SetString( "SecondaryWeapon", szAnsi );
//	}
//
//	// Melee
//	CEconItemView *pMeleeView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, LOADOUT_POSITION_MELEE );
//	const CEconItemDefinition *pMeleeDef = pMeleeView->GetItemDefinition();
//
//	if ( pMeleeDef )
//	{
//		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find ( pMeleeDef->GetItemBaseName() ) , szAnsi, sizeof(szAnsi) );
//		pKVData->SetString( "MeleeWeapon", szAnsi );
//	}
//
//	// Check for power up bottle
//	CEconItemView *pActionView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, LOADOUT_POSITION_ACTION );
//	//489 is bottle - Power Up Canteen (MvM)
//	pKVData->SetBool( "HasBottle", pActionView->GetItemDefIndex() == 489 );
//
//	// Send to DB
//	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
//
//#endif // !defined(NO_STEAM)
}
#endif // CLIENT_DLL
//=============================================================================//
//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
// Purpose: Returns a copy of the WaveStats if legit
//-----------------------------------------------------------------------------
CMannVsMachineLocalWaveStats CMannVsMachineStats::GetWaveStats( int iWaveIdx )
{
	CMannVsMachineLocalWaveStats waveStats;

	if ( iWaveIdx == (int)m_iCurrentWaveIdx )
	{
		waveStats = m_currentWaveStats;
	}
	else if ( iWaveIdx >= 0 && iWaveIdx == (int)m_iCurrentWaveIdx - 1 )
	{
		waveStats = m_previousWaveStats;
	}
	else if ( iWaveIdx == -1 )
	{
		waveStats = m_runningTotalWaveStats;
		waveStats += m_previousWaveStats;
		waveStats += m_currentWaveStats;
	}
	return waveStats;
}

//-----------------------------------------------------------------------------
// Purpose: Advances Event time for the next set of events
//-----------------------------------------------------------------------------
void CMannVsMachineStats::OnStatsChanged()
{
	NetworkStateChanged();
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Reset the tracked wave stats
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetWaveStats()
{
	m_currentWaveStats.ClearStats();
	m_nRespecsAwardedInWave = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Submits the WaveSummary Data to OGS
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportWaveSummary ( int waveIdx, bool bIsSuccess )
{
// Disabled due to bad key indexing.
// Make a V2 if this data is wanted
//#if !defined(NO_STEAM)
//	ConVar *sv_cheats = g_pCVar->FindVar( "sv_cheats" );
//	if ( sv_cheats && sv_cheats->GetBool() )
//		return;
//
//	CMannVsMachineWaveStats stats;
//	if ( waveIdx == (int)m_iCurrentWaveIdx )
//	{
//		stats = m_currentWaveStats;
//	}
//	else if ( waveIdx == (int)m_iCurrentWaveIdx - 1)
//	{
//		stats = m_previousWaveStats;
//	}
//	else
//	{
//		return;
//	}
//
//	KeyValues* pKVData = new KeyValues( "TF2ServerMvMWaveSummary" );
//
//	// ServerSessionID - Auto
//	pKVData->SetUint64( "ServerSessionID", GetSteamWorksSGameStatsUploader().GetSessionID());
//	pKVData->SetInt( "ID", ++m_nEventID );
//	//ServerWaveID, if we won this wave, use the previous WaveID
//	int iServerWaveID = m_iServerWaveID;
//	if ( bIsSuccess )
//	{
//		iServerWaveID--;
//	}
//	pKVData->SetInt( "ServerWaveID", iServerWaveID );
//
//	// WaveNumber
//	pKVData->SetInt( "WaveNumber", waveIdx + 1 );
//	
//	// PopulationFileID
//	pKVData->SetString( "PopulationFileID", m_pPopFileName );
//
//	//// MoneyCollected
//	pKVData->SetInt( "MoneyCollected", stats.nCreditsAcquired );
//	//
//	//// MoneyMissed
//	pKVData->SetInt( "MoneyMissed", stats.nCreditsDropped - stats.nCreditsAcquired );
//	//
//	//// Attempts
//	pKVData->SetInt( "Attempts", stats.nAttempts );
//	
//	// IsSuccess
//	pKVData->SetBool( "IsSuccess", bIsSuccess );
//
//	// IsSuccess
//	bool bMannUp = false;
//	bool bInLobby = false;
//	CTFLobby *pLobby = GTFGCClientSystem()->GetLobby();
//	if ( pLobby )
//	{
//		bInLobby = true;
//		bMannUp = IsMannUpGroup( pLobby->GetMatchGroup() );
//	}
//
//	pKVData->SetBool( "IsTrustedServer", bInLobby );
//
//	pKVData->SetBool( "IsMannUp", bMannUp );
//
//	// EventTime
//	pKVData->SetInt( "EventTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
//
//	// Send to DB
//	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
//
//#endif // !defined(NO_STEAM)
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// End CMannVsMachineStats
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------
uint32 MannVsMachineStats_GetCurrentWave()
{
	return g_pMVMStats ? g_pMVMStats->GetCurrentWave() : 0;
}

uint32 MannVsMachineStats_GetAcquiredCredits( int idxWave /*= - 1 */, bool bIncludeBonus /*= true*/ )
{
	return g_pMVMStats ? g_pMVMStats->GetAcquiredCredits( idxWave, bIncludeBonus ) : 0;
}

uint32 MannVsMachineStats_GetDroppedCredits( int idxWave )
{
	return g_pMVMStats ? g_pMVMStats->GetDroppedCredits( idxWave ) : 0;
}

uint32 MannVsMachineStats_GetMissedCredits( int idxWave )
{
	return g_pMVMStats ? g_pMVMStats->GetMissedCredits( idxWave ) : 0;
}

CMannVsMachineStats *MannVsMachineStats_GetInstance()
{
	return g_pMVMStats;
}


#ifdef GAME_DLL

void MannVsMachineStats_Init()
{
	CBaseEntity::Create( "tf_mann_vs_machine_stats", vec3_origin, vec3_angle );
}

void MannVsMachineStats_ResetPlayerEvents( CTFPlayer *pTFPlayer )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->ResetPlayerEvents( pTFPlayer );
	}
}

void MannVsMachineStats_RoundEvent_CreditsDropped( uint32 waveIdx, int nAmount )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->RoundEvent_CreditsDropped( waveIdx, nAmount );
	}
}

void MannVsMachineStats_PlayerEvent_PointsChanged( CTFPlayer *pTFPlayer, int nPoints )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_PointsChanged( pTFPlayer, nPoints );
	}
}

void MannVsMachineStats_PlayerEvent_Died( CTFPlayer *pTFPlayer )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_Died( pTFPlayer );
	}
}

void MannVsMachineStats_PlayerEvent_Upgraded( CTFPlayer *pTFPlayer, uint16 nItemDef, uint16 nAttributeDef, uint16 nQuality, int16 nCost, bool bIsBottle )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_Upgraded( pTFPlayer, nItemDef, nAttributeDef, nQuality, nCost, bIsBottle );
	}
}

void MannVsMachineStats_PlayerEvent_PickedUpCredits( CTFPlayer *pTFPlayer, uint32 idxWave, int nCreditsAmount )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_PickedUpCredits( pTFPlayer, idxWave, nCreditsAmount );
	}
}

void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn( CTFPlayer *pTFPlayer, int nCost )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_BoughtInstantRespawn( pTFPlayer, nCost );
	}
}

void MannVsMachineStats_SetPopulationFile( const char * pPopulationFile)
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->SetPopFile( pPopulationFile );
	}
}

#endif // GAME_DLL
