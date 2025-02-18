//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_gcmessages.h"
#include "tf_item_inventory.h"
#include "tf_player.h"
#include "tf_duel_summary.h"
#include "tf_gamerules.h"

#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
																			   
//-----------------------------------------------------------------------------

struct duel_minigame_data_t
{
	CSteamID m_steamIDInitiator;
	CSteamID m_steamIDTarget;
	uint16 m_usScoreInitiator;
	uint16 m_usScoreTarget;
	int m_iPlayerClass;
	bool operator==( const duel_minigame_data_t &rhs ) const
	{
		return m_steamIDInitiator == rhs.m_steamIDInitiator && m_steamIDTarget == rhs.m_steamIDTarget;
	}
};

CUtlVector< duel_minigame_data_t > g_duels;

static duel_minigame_data_t *FindDuelBySteamID( const CSteamID &steamID )
{
	FOR_EACH_VEC( g_duels, i )
	{
		duel_minigame_data_t &duel = g_duels[i];
		if ( duel.m_steamIDInitiator == steamID || duel.m_steamIDTarget == steamID )
		{
			 return &duel;
		}
	}
	return NULL;
}

static void SpeakConceptBySteamID( const CSteamID &steamID, int iConcept, const CSteamID &steamIDInitiator, const CSteamID &steamIDTarget )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( steamID ) );
	if ( pPlayer != NULL)
	{
		CTFPlayer *pPlayerInitiator = ToTFPlayer( GetPlayerBySteamID( steamIDInitiator ) );
		CTFPlayer *pPlayerTarget = ToTFPlayer( GetPlayerBySteamID( steamIDTarget ) );
		char pModifiers[256] = "";
		if ( pPlayerInitiator && pPlayerTarget )
		{
			Q_snprintf( pModifiers, sizeof(pModifiers), "duelinitiatorclass:%s,dueltargetclass:%s", 
					    g_aPlayerClassNames_NonLocalized[ pPlayerInitiator->m_Shared.InCond( TF_COND_DISGUISED ) ? pPlayerInitiator->m_Shared.GetDisguiseClass() : pPlayerInitiator->GetPlayerClass()->GetClassIndex() ],
						g_aPlayerClassNames_NonLocalized[ pPlayerTarget->m_Shared.InCond( TF_COND_DISGUISED ) ? pPlayerTarget->m_Shared.GetDisguiseClass() : pPlayerTarget->GetPlayerClass()->GetClassIndex() ]
						);
		}
		pPlayer->SpeakConceptIfAllowed( iConcept, pModifiers );
	}
}

static void RemoveDuel( duel_minigame_data_t *pDuel )
{
	g_duels.FindAndFastRemove( *pDuel );
}

static void SendDuelResults( duel_minigame_data_t &duel, const CSteamID &steamIDWinner, eDuelEndReason eReason )
{
	GCSDK::CGCMsg<MsgGC_Duel_Results_t> msg( k_EMsgGC_Duel_Results );
	msg.Body().m_ulInitiatorSteamID = duel.m_steamIDInitiator.ConvertToUint64();
	msg.Body().m_ulTargetSteamID = duel.m_steamIDTarget.ConvertToUint64();
	msg.Body().m_ulWinnerSteamID = steamIDWinner.ConvertToUint64();
	msg.Body().m_usScoreInitiator = duel.m_usScoreInitiator;
	msg.Body().m_usScoreTarget = duel.m_usScoreTarget;	
	msg.Body().m_usEndReason = eReason;
	GCClientSystem()->BSendMessage( msg );
}

typedef enum
{
	kDuelScoreType_Kill,
	kDuelScoreType_Assist,
	kMaxDuelScoreTypes,
} eDuelScoreType;

static int kDuelScoreTypes[kMaxDuelScoreTypes] = { 1, 1 };

static void UpdateDuelScore( CTFPlayer *pKiller, CTFPlayer *pVictim, eDuelScoreType scoreType )
{
	CSteamID steamIDKiller;
	CSteamID steamIDVictim;
	if ( pKiller->GetSteamID( &steamIDKiller ) == false || pVictim->GetSteamID( &steamIDVictim ) == false )
		return;
		
	int iScoreIncrement = kDuelScoreTypes[ scoreType ];

	FOR_EACH_VEC( g_duels, i )
	{
		duel_minigame_data_t &duel = g_duels[i];
		if ( ( duel.m_steamIDInitiator == steamIDKiller && duel.m_steamIDTarget == steamIDVictim ) ||
			 ( duel.m_steamIDInitiator == steamIDVictim && duel.m_steamIDTarget == steamIDKiller ) )
		{
			// if we have a class restriction...
			bool bCountScore = true;
			if ( duel.m_iPlayerClass >= TF_FIRST_NORMAL_CLASS && duel.m_iPlayerClass < TF_LAST_NORMAL_CLASS )
			{
				bCountScore = ( pKiller->GetPlayerClass() != NULL && duel.m_iPlayerClass == pKiller->GetPlayerClass()->GetClassIndex() &&
								pVictim->GetPlayerClass() != NULL && duel.m_iPlayerClass == pVictim->GetPlayerClass()->GetClassIndex() );
			}

			if ( bCountScore )
			{
				// send appropriate event to all clients
				IGameEvent * event = gameeventmanager->CreateEvent( "duel_status" );
				if ( event )
				{
					event->SetInt( "killer", pKiller->GetUserID() );
					event->SetInt( "score_type", scoreType );
					if ( steamIDKiller == duel.m_steamIDInitiator )
					{
						event->SetInt( "initiator", pKiller->GetUserID() );
						event->SetInt( "target", pVictim->GetUserID() );
						duel.m_usScoreInitiator += iScoreIncrement;
					}
					else
					{
						event->SetInt( "initiator", pVictim->GetUserID() );
						event->SetInt( "target", pKiller->GetUserID() );
						duel.m_usScoreTarget += iScoreIncrement;
					}
					event->SetInt( "initiator_score", duel.m_usScoreInitiator );
					event->SetInt( "target_score", duel.m_usScoreTarget );
					gameeventmanager->FireEvent( event );
				}
			}

			break;
		}
	}
}

/**
 * Duel request
 */
class CGC_GameServer_Duel_Request : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_Duel_Request( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Request_t> msg( pNetPacket );
		SpeakConceptBySteamID( msg.Body().m_ulInitiatorSteamID, MP_CONCEPT_DUEL_REQUEST, msg.Body().m_ulInitiatorSteamID, msg.Body().m_ulTargetSteamID );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_Duel_Request, "CGC_GameServer_Duel_Request", k_EMsgGC_Duel_Request, GCSDK::k_EServerTypeGCClient );

/**
 * Duel response
 */
class CGC_GameServer_Duel_Response : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_Duel_Response( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Response_t> msg( pNetPacket );

		// make sure we're still allowed to start a duel because the
		// game state may have changed since the duel request was sent
		if ( TFGameRules() && !TFGameRules()->CanInitiateDuels() )
		{
			// if they accepted the duel somehow, we need to cancel it with the GC
			if ( msg.Body().m_bAccepted )
			{
				GCSDK::CGCMsg<MsgGC_Duel_Results_t> msgDuelResults( k_EMsgGC_Duel_Results );
				msgDuelResults.Body().m_ulInitiatorSteamID = msg.Body().m_ulInitiatorSteamID;
				msgDuelResults.Body().m_ulTargetSteamID = msg.Body().m_ulTargetSteamID;
				msgDuelResults.Body().m_ulWinnerSteamID = 0;
				msgDuelResults.Body().m_usScoreInitiator = 0;
				msgDuelResults.Body().m_usScoreTarget = 0;
				msgDuelResults.Body().m_usEndReason = kDuelEndReason_Cancelled;
				GCClientSystem()->BSendMessage( msgDuelResults );
			}

			return true;
		}

		// duel was rejected
		if ( msg.Body().m_bAccepted == false )
		{
			SpeakConceptBySteamID( msg.Body().m_ulInitiatorSteamID, MP_CONCEPT_DUEL_REJECTED, msg.Body().m_ulInitiatorSteamID, msg.Body().m_ulTargetSteamID  );
			SpeakConceptBySteamID( msg.Body().m_ulTargetSteamID, MP_CONCEPT_DUEL_TARGET_REJECT, msg.Body().m_ulInitiatorSteamID, msg.Body().m_ulTargetSteamID  );
			return true;
		}

		// duel was accepted
		SpeakConceptBySteamID( msg.Body().m_ulInitiatorSteamID, MP_CONCEPT_DUEL_ACCEPTED, msg.Body().m_ulInitiatorSteamID, msg.Body().m_ulTargetSteamID  );
		SpeakConceptBySteamID( msg.Body().m_ulTargetSteamID, MP_CONCEPT_DUEL_TARGET_ACCEPT, msg.Body().m_ulInitiatorSteamID, msg.Body().m_ulTargetSteamID  );
		int idx = g_duels.AddToTail();
		duel_minigame_data_t &duel = g_duels[idx];
		memset( &duel, 0, sizeof(duel) );
		duel.m_steamIDInitiator = msg.Body().m_ulInitiatorSteamID;
		duel.m_steamIDTarget = msg.Body().m_ulTargetSteamID;
		duel.m_iPlayerClass = msg.Body().m_usAsPlayerClass;

		if ( duel.m_iPlayerClass >= TF_FIRST_NORMAL_CLASS && duel.m_iPlayerClass < TF_LAST_NORMAL_CLASS )
		{
			CTFPlayer *pPlayer_Initiator = ToTFPlayer( GetPlayerBySteamID( msg.Body().m_ulInitiatorSteamID ) );
			CTFPlayer *pPlayer_Target = ToTFPlayer( GetPlayerBySteamID( msg.Body().m_ulTargetSteamID ) );
			if ( pPlayer_Initiator && ( pPlayer_Initiator->GetPlayerClass() == NULL || pPlayer_Initiator->GetPlayerClass()->GetClassIndex() != duel.m_iPlayerClass ) )
			{
				pPlayer_Initiator->SetDesiredPlayerClassIndex( duel.m_iPlayerClass );
				pPlayer_Initiator->ForceRespawn();
			}
			if ( pPlayer_Target && ( pPlayer_Target->GetPlayerClass() == NULL || pPlayer_Target->GetPlayerClass()->GetClassIndex() != duel.m_iPlayerClass ) )
			{
				pPlayer_Target->SetDesiredPlayerClassIndex( duel.m_iPlayerClass );
				pPlayer_Target->ForceRespawn();
			}
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_Duel_Response, "CGC_GameServer_Duel_Response", k_EMsgGC_Duel_Response, GCSDK::k_EServerTypeGCClient );

bool DuelMiniGame_IsInDuel( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) == false )
	{
		return false;
	}
	duel_minigame_data_t *pDuel = FindDuelBySteamID( steamID );
	return pDuel != NULL;
}

int DuelMiniGame_GetRequiredPlayerClass( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) == false )
	{
		return false;
	}
	duel_minigame_data_t *pDuel = FindDuelBySteamID( steamID );
	return pDuel != NULL ? pDuel->m_iPlayerClass : TF_CLASS_UNDEFINED;
}

void DuelMiniGame_NotifyKill( CTFPlayer *pKiller, CTFPlayer *pVictim )
{
	UpdateDuelScore( pKiller, pVictim, kDuelScoreType_Kill );
}

void DuelMiniGame_NotifyAssist( CTFPlayer *pAssister, CTFPlayer *pVictim )
{
	UpdateDuelScore( pAssister, pVictim, kDuelScoreType_Assist );
}

void DuelMiniGame_NotifyPlayerChangedTeam( CTFPlayer *pPlayer, int iNewTeam, bool bInitiatedByPlayer )
{
	CSteamID steamIDPlayerWhoChangedTeams;
	if ( pPlayer->GetSteamID( &steamIDPlayerWhoChangedTeams ) == false )
	{
		return;
	}
	duel_minigame_data_t *pDuel = FindDuelBySteamID( steamIDPlayerWhoChangedTeams );
	if ( pDuel == NULL )
	{
		return;
	}
	CSteamID steamIDOpponent;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pOpponent = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pOpponent == NULL || pOpponent == pPlayer )
			continue;
		if ( pOpponent->GetSteamID( &steamIDOpponent ) == false )
			continue;
		if ( steamIDOpponent == pDuel->m_steamIDInitiator || steamIDOpponent == pDuel->m_steamIDTarget )
		{
			// player is disconnecting?
			if ( iNewTeam == TEAM_UNASSIGNED )
			{
				SendDuelResults( *pDuel, steamIDOpponent, kDuelEndReason_PlayerDisconnected );
				RemoveDuel( pDuel );
			}
			// found the opponent, if they are on the same team or the player is on spectator...
			else if ( iNewTeam == TEAM_SPECTATOR ||
				      iNewTeam == pOpponent->GetTeamNumber() )
			{
				SendDuelResults( *pDuel, steamIDOpponent, bInitiatedByPlayer ? kDuelEndReason_PlayerSwappedTeams : kDuelEndReason_PlayerForceSwappedTeams );
				RemoveDuel( pDuel );
			}			
			return;
		}
	}
}

void DuelMiniGame_NotifyPlayerDisconnect( CTFPlayer *pPlayer, bool bKicked )
{
	CSteamID steamIDPlayerWhoChangedTeams;
	if ( pPlayer->GetSteamID( &steamIDPlayerWhoChangedTeams ) == false )
	{
		return;
	}
	duel_minigame_data_t *pDuel = FindDuelBySteamID( steamIDPlayerWhoChangedTeams );
	if ( pDuel == NULL )
	{
		return;
	}
	CSteamID steamIDOpponent;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pOpponent = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pOpponent == NULL || pOpponent == pPlayer )
			continue;
		if ( pOpponent->GetSteamID( &steamIDOpponent ) == false )
			continue;
		if ( steamIDOpponent == pDuel->m_steamIDInitiator || steamIDOpponent == pDuel->m_steamIDTarget )
		{
			SendDuelResults( *pDuel, steamIDOpponent, bKicked ? kDuelEndReason_PlayerKicked : kDuelEndReason_PlayerDisconnected );
			RemoveDuel( pDuel );
			return;
		}
	}
}

void DuelMiniGame_AssignWinners()
{
	// send a message to the GC for each duel and remove the duels afterwards
	FOR_EACH_VEC( g_duels, i )
	{
		duel_minigame_data_t &duel = g_duels[i];
		if ( duel.m_usScoreInitiator == 0 && duel.m_usScoreTarget == 0 )
		{
			SendDuelResults( duel, duel.m_usScoreInitiator > duel.m_usScoreTarget ? duel.m_steamIDInitiator : duel.m_steamIDTarget, kDuelEndReason_ScoreTiedAtZero );
		}
		else if ( duel.m_usScoreInitiator == duel.m_usScoreTarget )
		{
			SendDuelResults( duel, duel.m_usScoreInitiator > duel.m_usScoreTarget ? duel.m_steamIDInitiator : duel.m_steamIDTarget, kDuelEndReason_ScoreTied );
		}
		else
		{
			SendDuelResults( duel, duel.m_usScoreInitiator > duel.m_usScoreTarget ? duel.m_steamIDInitiator : duel.m_steamIDTarget, kDuelEndReason_DuelOver );
		}
	}
	g_duels.RemoveAll();
}

void DuelMiniGame_Stop()
{
	DuelMiniGame_AssignWinners();
}

void DuelMiniGame_LevelShutdown()
{
	// send a message to the GC for each duel and remove the duels afterwards
	FOR_EACH_VEC( g_duels, i )
	{
		duel_minigame_data_t &duel = g_duels[i];
		SendDuelResults( duel, duel.m_usScoreInitiator > duel.m_usScoreTarget ? duel.m_steamIDInitiator : duel.m_steamIDTarget, kDuelEndReason_LevelShutdown );
	}
	g_duels.RemoveAll();
}
