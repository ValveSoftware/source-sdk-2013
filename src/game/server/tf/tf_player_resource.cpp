//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>
#include "tf_matchmaking_shared.h"

#include "tf_mann_vs_machine_stats.h"
#include "player_vs_environment/tf_population_manager.h"
#include "tf_gc_server.h"

#define STATS_SEND_FREQUENCY 1.f

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxBuffedHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxBuffedHealth ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bArenaSpectator ), SendPropBool( SENDINFO_ARRAY( m_bArenaSpectator ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iActiveDominations ), SendPropInt( SENDINFO_ARRAY( m_iActiveDominations ), 6, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flNextRespawnTime ), SendPropTime( SENDINFO_ARRAY( m_flNextRespawnTime ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iChargeLevel ), SendPropInt( SENDINFO_ARRAY( m_iChargeLevel ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamage ), SendPropInt( SENDINFO_ARRAY( m_iDamage ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageAssist ), SendPropInt( SENDINFO_ARRAY( m_iDamageAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBoss ), SendPropInt( SENDINFO_ARRAY( m_iDamageBoss ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealing ), SendPropInt( SENDINFO_ARRAY( m_iHealing ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealingAssist ), SendPropInt( SENDINFO_ARRAY( m_iHealingAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBlocked ), SendPropInt( SENDINFO_ARRAY( m_iDamageBlocked ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCurrencyCollected ), SendPropInt( SENDINFO_ARRAY( m_iCurrencyCollected ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBonusPoints ), SendPropInt( SENDINFO_ARRAY( m_iBonusPoints ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerLevel ), SendPropInt( SENDINFO_ARRAY( m_iPlayerLevel ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iStreaks ), SendPropInt( SENDINFO_ARRAY( m_iStreaks ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iUpgradeRefundCredits ), SendPropInt( SENDINFO_ARRAY( m_iUpgradeRefundCredits ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBuybackCredits ), SendPropInt( SENDINFO_ARRAY( m_iBuybackCredits ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropInt( SENDINFO( m_iPartyLeaderRedTeamIndex ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropInt( SENDINFO( m_iPartyLeaderBlueTeamIndex ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropInt( SENDINFO( m_iEventTeamStatus ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClassWhenKilled ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClassWhenKilled ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iConnectionState ), SendPropInt( SENDINFO_ARRAY( m_iConnectionState ), 3, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flConnectTime ), SendPropTime( SENDINFO_ARRAY( m_flConnectTime ) ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerResource::CTFPlayerResource( void )
{
	ListenForGameEvent( "mvm_wave_complete" );

	m_flNextDamageAndHealingSend = 0.f;

	m_iPartyLeaderRedTeamIndex = 0;
	m_iPartyLeaderBlueTeamIndex = 0;
	m_iEventTeamStatus = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::FireGameEvent( IGameEvent * event )
{
	const char *pszEvent = event->GetName();

	if ( !Q_strcmp( pszEvent, "mvm_wave_complete" ) )
	{
		// Force a re-send on wave complete
		m_flNextDamageAndHealingSend = 0.f;
		UpdatePlayerData();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::SetPartyLeaderIndex( int iTeam, int iIndex )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	switch( iTeam )
	{
	case TF_TEAM_RED:
		m_iPartyLeaderRedTeamIndex = iIndex;
		break;
	case TF_TEAM_BLUE:
		m_iPartyLeaderBlueTeamIndex = iIndex;
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetPartyLeaderIndex( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_iPartyLeaderRedTeamIndex;
	else if ( iTeam == TF_TEAM_BLUE )
		return m_iPartyLeaderBlueTeamIndex;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	m_vecRedPlayers.RemoveAll();
	m_vecBluePlayers.RemoveAll();
	m_vecFreeSlots.RemoveAll();

	BaseClass::UpdatePlayerData();

	// check if player is still part of the match
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch && TFGameRules() )
	{
		for ( int i=0; i<pMatch->GetNumTotalMatchPlayers(); ++i )
		{
			AssertMsg( m_vecFreeSlots.Count() > 0, "There should always be free slots for player to join" );

			CMatchInfo::PlayerMatchData_t *pData = pMatch->GetMatchDataForPlayer( i );
			uint32 unAccountID = pData->steamID.GetAccountID();
			int iTeam = TFGameRules()->GetGameTeamForGCTeam( pData->eGCTeam );
			CUtlVector< uint32 >* pVecPlayers = iTeam == TF_TEAM_RED ? &m_vecRedPlayers : &m_vecBluePlayers;

			// add players that are not yet connected to the server
			if ( !pData->bDropped && pVecPlayers->Find( unAccountID ) == pVecPlayers->InvalidIndex() && m_vecFreeSlots.Count() > 0 )
			{
				int iIndex = m_vecFreeSlots[0];
				m_vecFreeSlots.Remove( 0 );

				AssertMsg( m_iAccountID[iIndex] == 0, "No account should be assigned to this slot" );

				Init( iIndex );
				m_iAccountID.Set( iIndex, unAccountID );
				m_bValid.Set( iIndex, 1 );
				m_iTeam.Set( iIndex, iTeam );
				m_iConnectionState.Set( iIndex, pData->GetConnectionState() );
			}
		}

		// do we need to set m_bValid on these?
		if ( GTFGCClientSystem()->BLateJoinEligible() )
		{
			int iTeamSize = pMatch->GetCanonicalMatchSize() / 2;

			int iRedWaiting = iTeamSize-m_vecRedPlayers.Count();
			for ( int i=0; i<iRedWaiting && m_vecFreeSlots.Count() > 0; ++i )
			{
				int iIndex = m_vecFreeSlots[0];
				m_vecFreeSlots.Remove( 0 );

				AssertMsg( m_iAccountID[iIndex] == 0, "No account should be assigned to this slot" );

				m_iTeam.Set( iIndex, TF_TEAM_RED );
				m_iConnectionState.Set( iIndex, MM_WAITING_FOR_PLAYER );
			}

			int iBlueWaiting = iTeamSize-m_vecBluePlayers.Count();
			for ( int i=0; i<iBlueWaiting && m_vecFreeSlots.Count() > 0; ++i )
			{
				int iIndex = m_vecFreeSlots[0];
				m_vecFreeSlots.Remove( 0 );

				AssertMsg( m_iAccountID[iIndex] == 0, "No account should be assigned to this slot" );

				m_iTeam.Set( iIndex, TF_TEAM_BLUE );
				m_iConnectionState.Set( iIndex, MM_WAITING_FOR_PLAYER );
			}
		}
	}

	if ( gpGlobals->curtime > m_flNextDamageAndHealingSend )
	{
		m_flNextDamageAndHealingSend = gpGlobals->curtime + STATS_SEND_FREQUENCY;
	}

	if ( pMatch && m_iEventTeamStatus != (int)pMatch->m_unEventTeamStatus )
	{
		m_iEventTeamStatus = pMatch->m_unEventTeamStatus;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdateConnectedPlayer( int iIndex, CBasePlayer *pPlayer )
{
	BaseClass::UpdateConnectedPlayer( iIndex, pPlayer );

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	PlayerStats_t *pTFPlayerStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
	if ( pTFPlayerStats ) 
	{
		// Update accumulated and per-round stats
		localplayerscoring_t *pData = pTFPlayer->m_Shared.GetScoringData();
		pData->UpdateStats( pTFPlayerStats->statsAccumulated, pTFPlayer, false );

		pData = pTFPlayer->m_Shared.GetRoundScoringData();
		pData->UpdateStats( pTFPlayerStats->statsCurrentRound, pTFPlayer, true );

		// Send every STATS_SEND_FREQUENCY (1.f)
		if ( gpGlobals->curtime > m_flNextDamageAndHealingSend )
		{
			m_iDamage.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE] );
			m_iDamageAssist.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_ASSIST] );
			m_iDamageBoss.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BOSS] );
			m_iHealing.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING] );
			m_iHealingAssist.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING_ASSIST] );
			m_iDamageBlocked.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BLOCKED] );
			m_iCurrencyCollected.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_CURRENCY_COLLECTED] );
			m_iBonusPoints.Set( iIndex, pTFPlayerStats->statsCurrentRound.m_iStat[TFSTAT_BONUS_POINTS] );
			m_iPlayerLevel.Set( iIndex, pTFPlayer->GetExperienceLevel() );
		}
	}

	m_iMaxHealth.Set( iIndex, pTFPlayer->GetMaxHealth() );

	// m_iMaxBuffedHealth is misnamed -- it should be m_iMaxHealthForBuffing, but we don't want to change it now due to demos.
	m_iMaxBuffedHealth.Set( iIndex, pTFPlayer->GetMaxHealthForBuffing() );
	m_iPlayerClass.Set( iIndex, pTFPlayer->GetPlayerClass()->GetClassIndex() );

	m_iActiveDominations.Set( iIndex, pTFPlayer->GetNumberofDominations() );

	int iTotalScore = CTFGameRules::CalcPlayerScore( &pTFPlayerStats->statsAccumulated, pTFPlayer );

	if ( m_iTotalScore.Get( iIndex ) != iTotalScore )
	{
		int nDelta = iTotalScore -  m_iTotalScore.Get( iIndex );
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			MannVsMachineStats_PlayerEvent_PointsChanged( pTFPlayer, nDelta );
		}
		else
		{
			// Kill eater points-scored tracking.  Increment all equipped items with this kill eater type.  
			// We only do this when we're NOT in MvM
			HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( pTFPlayer, kKillEaterEvent_PointsScored, nDelta );
		}
	}
		
	m_iTotalScore.Set( iIndex, iTotalScore );
	m_bArenaSpectator.Set( iIndex, pTFPlayer->IsArenaSpectator() );

	if ( TFGameRules()->IsInTournamentMode() )
	{
		float flCharge = pTFPlayer->MedicGetChargeLevel();
		m_iChargeLevel.Set( iIndex, (int)(flCharge * 100) );
	}
	else
	{
		m_iChargeLevel.Set( iIndex, 0 );
	}

	float flRespawnTime = pTFPlayer->IsAlive() ? 0 : TFGameRules()->GetNextRespawnWave( pTFPlayer->GetTeamNumber(), pTFPlayer );
	if ( pTFPlayer->GetRespawnTimeOverride() != -1.f )
	{
		flRespawnTime = pTFPlayer->GetDeathTime() + pTFPlayer->GetRespawnTimeOverride();
	}
	m_flNextRespawnTime.Set( iIndex, flRespawnTime );

	m_flConnectTime.Set( iIndex, pTFPlayer->GetConnectionTime() );

	for ( int streak_type = 0; streak_type < CTFPlayerShared::kTFStreak_COUNT; streak_type++ )
	{
		m_iStreaks.Set( iIndex * CTFPlayerShared::kTFStreak_COUNT + streak_type, pTFPlayer->m_Shared.GetStreak( (CTFPlayerShared::ETFStreak)streak_type ) );
	}

	if ( g_pPopulationManager )
	{
		// Only update when we have new data
		int nRespecs = g_pPopulationManager->GetNumRespecsAvailableForPlayer( pTFPlayer );
		m_iUpgradeRefundCredits.Set( iIndex, nRespecs );

		int nBuybacks = g_pPopulationManager->GetNumBuybackCreditsForPlayer( pTFPlayer );
		m_iBuybackCredits.Set( iIndex, nBuybacks );
	}

	CSteamID steamID;
	pTFPlayer->GetSteamID( &steamID );

	int iTeam = pPlayer->GetTeamNumber();

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		CMatchInfo::PlayerMatchData_t *pData = pMatch->GetMatchDataForPlayer( steamID );
		if ( pData )
		{
			int iGCTeam = TFGameRules()->GetGameTeamForGCTeam( pData->eGCTeam );

			// if the team hasn't been set yet in-game, we want to show them on the
			// team the GC has assigned them to instead of spectator or unassigned
			if ( ( iTeam == TEAM_UNASSIGNED ) || ( iTeam == TEAM_SPECTATOR ) )
			{
				m_iTeam.Set( iIndex, iGCTeam );
			}

			iTeam = iGCTeam;
		}
	}

	CUtlVector< uint32 >* pVecPlayers = ( iTeam == TF_TEAM_RED ) ? &m_vecRedPlayers : ( ( iTeam == TF_TEAM_BLUE ) ? &m_vecBluePlayers : NULL );
	if ( pVecPlayers )
	{
		if ( pVecPlayers->Find( steamID.GetAccountID() ) == pVecPlayers->InvalidIndex()	)
		{
			pVecPlayers->AddToTail( steamID.GetAccountID() );
		}
	}

	m_iConnectionState.Set( iIndex, MM_CONNECTED );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdateDisconnectedPlayer( int iIndex )
{
	// cache accountID to see if we should preserve this account
	uint32 unAccountID = m_iAccountID[iIndex];

	BaseClass::UpdateDisconnectedPlayer( iIndex );

	// preserve if player is still in part of the match, and still gone
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		CSteamID steamID( unAccountID, GetUniverse(), k_EAccountTypeIndividual );
		if ( steamID.IsValid() )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerBySteamID( steamID );
			CMatchInfo::PlayerMatchData_t *pData = pMatch->GetMatchDataForPlayer( steamID );
			// Skip if they're connected
			if ( pData && ( !pPlayer || !pPlayer->IsConnected() ) )
			{
				if ( !pData->bDropped )
				{
					int iTeam = TFGameRules()->GetGameTeamForGCTeam( pData->eGCTeam );
					m_iConnectionState.Set( iIndex, pData->GetConnectionState() );
					// re-apply the accountID to keep the data
					m_iAccountID.Set( iIndex, unAccountID );
					m_iTeam.Set( iIndex, iTeam );
					m_bValid.Set( iIndex, 1 );

					CUtlVector< uint32 >* pVecPlayers = iTeam == TF_TEAM_RED ? &m_vecRedPlayers : &m_vecBluePlayers;
					pVecPlayers->AddToTail( unAccountID );
					return;
				}
			}
		}
	}
	
	// free up the slot if we're not preserving it
	m_iConnectionState.Set( iIndex, MM_DISCONNECTED );
	m_vecFreeSlots.AddToTail( iIndex );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::Init( int iIndex )
{
	BaseClass::Init( iIndex );

	m_iTotalScore.Set( iIndex, 0 );
	m_iMaxHealth.Set( iIndex, TF_HEALTH_UNDEFINED );
	m_iMaxBuffedHealth.Set( iIndex, TF_HEALTH_UNDEFINED );
	m_iPlayerClass.Set( iIndex, TF_CLASS_UNDEFINED );
	m_iActiveDominations.Set( iIndex, 0 );
	m_iPlayerClassWhenKilled.Set( iIndex, TF_CLASS_UNDEFINED );
	m_iConnectionState.Set( iIndex, MM_DISCONNECTED );
	m_bValid.Set( iIndex, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerResource::SetPlayerClassWhenKilled( int iIndex, int iClass )
{
	Assert( iIndex >= 0 && iIndex <= MAX_PLAYERS );

	m_iPlayerClassWhenKilled.Set( iIndex, iClass );
}


