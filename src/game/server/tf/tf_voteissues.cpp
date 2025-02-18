//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  TF-specific things to vote on
//
//=============================================================================

#include "cbase.h"
#include "tf_voteissues.h"
#include "tf_player.h"

#include "vote_controller.h"
#include "fmtstr.h"
#include "eiface.h"
#include "tf_gamerules.h"
#include "inetchannelinfo.h"
#include "tf_gamestats.h"

#include "tf_gcmessages.h"
#include "player_vs_environment/tf_population_manager.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_objective_resource.h"
#include "gc_clientsystem.h"
#include "tf_gc_server.h"
#include "player_resource.h"
#include "tf_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_mm_trusted;
extern ConVar mp_autoteambalance;
extern ConVar tf_classlimit;
extern ConVar sv_vote_quorum_ratio;
extern ConVar tf_mm_strict;

static bool VotableMap( const char *pszMapName )
{
	char szCanonName[64] = { 0 };
	V_strncpy( szCanonName, pszMapName, sizeof( szCanonName ) );
	IVEngineServer::eFindMapResult eResult = engine->FindMap( szCanonName, sizeof( szCanonName ) );

	switch ( eResult )
	{
	case IVEngineServer::eFindMap_Found:
	case IVEngineServer::eFindMap_NonCanonical:
	case IVEngineServer::eFindMap_PossiblyAvailable:
	case IVEngineServer::eFindMap_FuzzyMatch:
		return true;
	case IVEngineServer::eFindMap_NotFound:
		return false;
	}

	AssertMsg( false, "Unhandled engine->FindMap return value\n" );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Base TF Issue
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Restart Round Issue
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_restart_game_allowed( "sv_vote_issue_restart_game_allowed", "0", FCVAR_NONE, "Can players call votes to restart the game?" );
ConVar sv_vote_issue_restart_game_allowed_mvm( "sv_vote_issue_restart_game_allowed_mvm", "1", FCVAR_NONE, "Can players call votes to restart the game in Mann-Vs-Machine?" );
ConVar sv_vote_issue_restart_game_cooldown( "sv_vote_issue_restart_game_cooldown", "300", FCVAR_NONE, "Minimum time before another restart vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestartGameIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_restart_game_cooldown.GetInt() )
	{
		SetIssueCooldownDuration( sv_vote_issue_restart_game_cooldown.GetFloat() );
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		g_pPopulationManager->ResetMap();
		return;
	}

	engine->ServerCommand( "mp_restartgame 1;" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRestartGameIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return sv_vote_issue_restart_game_allowed_mvm.GetBool();
	}

	return sv_vote_issue_restart_game_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRestartGameIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRestartGameIssue::GetDisplayString( void )
{
	return "#TF_vote_restart_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRestartGameIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_restart_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestartGameIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !sv_vote_issue_restart_game_allowed.GetBool() )
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}


//-----------------------------------------------------------------------------
// Purpose: Kick Player Issue
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_kick_allowed( "sv_vote_issue_kick_allowed", "0", FCVAR_REPLICATED, "Can players call votes to kick players from the server?" );
ConVar sv_vote_issue_kick_allowed_mvm( "sv_vote_issue_kick_allowed_mvm", "1", FCVAR_NONE, "Can players call votes to kick players from the server in MvM?" );
ConVar sv_vote_kick_ban_duration( "sv_vote_kick_ban_duration", "20", FCVAR_NONE, "The number of minutes a vote ban should last. (0 = Disabled)" );
ConVar sv_vote_issue_kick_min_connect_time_mvm( "sv_vote_issue_kick_min_connect_time_mvm", "300", FCVAR_NONE, "How long a player must be connected before they can be kicked (in seconds)." );
ConVar sv_vote_issue_kick_spectators_mvm( "sv_vote_issue_kick_spectators_mvm", "1", FCVAR_NONE, "Allow players to kick spectators in MvM." );
ConVar sv_vote_issue_kick_namelock_duration( "sv_vote_issue_kick_namelock_duration", "120", FCVAR_NONE, "How long to prevent kick targets from changing their name (in seconds)." );
ConVar sv_vote_issue_kick_limit_mvm( "sv_vote_issue_kick_limit_mvm", "0", FCVAR_HIDDEN, "The maximum number of kick votes a player can call during an MvM mission started by matchmaking. (0 = disabled)" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::Init( void )
{
	m_szTargetPlayerName[0] = 0;
	m_hPlayerTarget = NULL;
	m_eKickReason = TFVoteKickReason_Other;
	m_steamIDVoteCaller.Clear();
	m_steamIDVoteTarget.Clear();
	m_bSubmittedToMatchSystem = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::ExecuteCommand( void )
{
	PrintLogData();

	// If we submitted this kick to the match system in ProcessResults, it may have already gone ahead and removed them,
	// but if we're a mixed mode with ad-hoc players present we want to catch weird cases like the match player left and
	// rejoined as ad-hoc while it was going on.
	engine->ServerCommand( CFmtStr( "kickid \"%s\" %s\n", m_steamIDVoteTarget.Render(), g_pszVoteKickString ) );

	if ( tf_mm_strict.GetInt() == 1 )
	{
		// If we're in strict match mode, we're done.
		//
		// If the GC does send them back here, banning them would put them in a broken rejoin state. The matchmaker
		// should just not re-match you to somewhere you got kicked from.
		return;
	}

	// If we're not in strict mode they might be here as or be able to rejoin as an adhoc player -- ban.
	// Technically the GC might send them back here for *new* match if they get kicked as ad-hoc, the current match ends, etc.
	engine->ServerCommand( CFmtStr( "banid %d \"%s\"\n", sv_vote_kick_ban_duration.GetInt(), m_steamIDVoteTarget.Render() ) );

	// Band-aid: Hacks are able to avoid kick+ban, and we're not yet sure how they're doing it.  This code checks to see
	//           if they come back.
	//
	// XXX(JohnS): We think the original cause behind this was fixed (connecting-but-not-active race condition)
	CVoteController::AddPlayerToKickWatchList( m_steamIDVoteTarget, ( sv_vote_kick_ban_duration.GetFloat() * 60.f ) );

	NotifyGCAdHocKick( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return sv_vote_issue_kick_allowed_mvm.GetBool();
	}

	return sv_vote_issue_kick_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: This gets calle first.  If true, moves on to OnVoteStarted()
//-----------------------------------------------------------------------------
bool CKickIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	Init();

	// Lookup caller steamID
	CTFPlayer *pTFVoteCaller = ToTFPlayer( UTIL_EntityByIndex( iEntIndex ) );
	if ( !pTFVoteCaller )
		{ return false; }

	pTFVoteCaller->GetSteamID( &m_steamIDVoteCaller );
	if ( !m_steamIDVoteCaller.IsValid() || !m_steamIDVoteCaller.BIndividualAccount() )
		{ return false; }

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !CreateVoteDataFromDetails( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		return false;
	}

	// Don't kick proxies
	if ( m_hPlayerTarget->IsReplay() || m_hPlayerTarget->IsHLTV() )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		return false;
	}

	// Don't kick the host or an admin
	if ( ( !engine->IsDedicatedServer() && m_hPlayerTarget->entindex() == 1 ) ||
		 m_hPlayerTarget->IsAutoKickDisabled() )
	{
		nFailCode = VOTE_FAILED_CANNOT_KICK_ADMIN;
		return false;
	}

	// Store target steamID - if they're not a bot
	bool bFakeClient = m_hPlayerTarget->IsFakeClient() || m_hPlayerTarget->IsBot();
	if ( !bFakeClient )
	{
		m_hPlayerTarget->GetSteamID( &m_steamIDVoteTarget );
		if ( !m_steamIDVoteTarget.IsValid() || !m_steamIDVoteTarget.BIndividualAccount() )
			return false;
	}

	// If we have a match, the match system does special handling of votekicks.
	if ( GTFGCClientSystem()->GetLiveMatch() )
	{
		CTFGCServerSystem::EVoteKickRequest eKickRequest;
		eKickRequest = GTFGCClientSystem()->PlayerRequestVoteKick( m_steamIDVoteCaller, m_steamIDVoteTarget, m_eKickReason );
		switch ( eKickRequest )
		{
			case CTFGCServerSystem::eVoteKick_Allow:
			{
				break; // Continue
			}
			case CTFGCServerSystem::eVoteKick_Deny:
			{
				nFailCode = VOTE_FAILED_KICK_DENIED_BY_GC;
				return false;
			}
			case CTFGCServerSystem::eVoteKick_Handled:
			{
				// Special fail code that tells the vote system we'll handle this -- the match system will start a vote
				// if allowed later.
				nFailCode = VOTE_FAILED_REQUEST_HANDLED_BY_ISSUE;
				return false;
			}
		}
	}

	// MvM
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// Don't allow kicking unless we're between rounds
		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_BETWEEN_RNDS )
		{
			nFailCode = VOTE_FAILED_CANNOT_KICK_DURING_ROUND;
			return false;
		}

		// Allow kicking team unassigned
		if ( m_hPlayerTarget->IsConnected() && m_hPlayerTarget->GetTeamNumber() == TEAM_UNASSIGNED )
			return true;

		// Don't allow kicking of players connected less than sv_vote_kick_min_connect_time_mvm
		CTFPlayer *pTFVoteTarget = ToTFPlayer( m_hPlayerTarget );
		if ( pTFVoteTarget )
		{
			float flTimeConnected = gpGlobals->curtime - pTFVoteTarget->GetConnectionTime();

			// See if we have a lobby...
			if ( !bFakeClient )
			{
				CMatchInfo::PlayerMatchData_t *pMatchPlayerTarget = GTFGCClientSystem()->GetLiveMatchPlayer( m_steamIDVoteTarget );
				CMatchInfo::PlayerMatchData_t *pMatchPlayerCaller = GTFGCClientSystem()->GetLiveMatchPlayer( m_steamIDVoteCaller );
				if ( pMatchPlayerTarget )
				{
					// Use this time instead (prevents disconnect avoidance)
					flTimeConnected = CRTime::RTime32TimeCur() - pMatchPlayerTarget->rtJoinedMatch;

					// TODO Now that we have this piped through the GC maybe it should just be doing this
					if ( sv_vote_issue_kick_limit_mvm.GetInt() )
					{
						if ( pMatchPlayerCaller && pMatchPlayerCaller->nVoteKickAttempts > (uint32)sv_vote_issue_kick_limit_mvm.GetInt() )
						{
							nFailCode = VOTE_FAILED_KICK_LIMIT_REACHED;
							return false;
						}
					}
				}
			}

			if ( flTimeConnected < sv_vote_issue_kick_min_connect_time_mvm.GetFloat() )
			{
				nFailCode = VOTE_FAILED_CANNOT_KICK_FOR_TIME;
				nTime = sv_vote_issue_kick_min_connect_time_mvm.GetFloat() - flTimeConnected;
				return false;
			}
		}

		// Allow kicking of spectators when this is set, except when it's a bot (invader bots are spectators between rounds)
		if ( sv_vote_issue_kick_spectators_mvm.GetBool() && !m_hPlayerTarget->IsBot() && m_hPlayerTarget->GetTeamNumber() == TEAM_SPECTATOR )
				return true;
	}
	
#ifndef _DEBUG
	// Don't kick players on other teams
	if ( g_pPlayerResource->GetTeam( pTFVoteCaller->entindex() ) != g_pPlayerResource->GetTeam( m_hPlayerTarget->entindex() ) )
	{
		nFailCode = VOTE_FAILED_TEAM_CANT_CALL;
		return false;
	}
#endif // !_DEBUG

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::OnVoteFailed( int iEntityHoldingVote )
{
	CBaseTFIssue::OnVoteFailed( iEntityHoldingVote );

	PrintLogData();
	NotifyGCAdHocKick( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
 void CKickIssue::OnVoteStarted( void )
 {
	// CanCallVote() should have initialized this
	if ( !m_hPlayerTarget )
	{
		m_hPlayerTarget = UTIL_PlayerBySteamID( m_steamIDVoteTarget );
	}

	if ( !m_hPlayerTarget )
		return;

	// Capture some data about the kick target now, so they can't avoid the 
	// result by doing things like drop, retry, stop sending commands, etc.
	if ( m_steamIDVoteTarget.IsValid() && m_steamIDVoteTarget.BIndividualAccount() )
	{
		Q_strncpy( m_szTargetPlayerName, m_hPlayerTarget->GetPlayerName(), sizeof( m_szTargetPlayerName ) );

		// Configured to block name changing when targeted for a kick?
		if ( sv_vote_issue_kick_namelock_duration.GetFloat() > 0 )
		{
			CVoteController::AddPlayerToNameLockedList( m_steamIDVoteTarget, sv_vote_issue_kick_namelock_duration.GetFloat(), m_hPlayerTarget->GetUserID() );
		}

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CMatchInfo::PlayerMatchData_t *pMatchPlayerCaller = GTFGCClientSystem()->GetLiveMatchPlayer( m_steamIDVoteCaller );
			if ( pMatchPlayerCaller )
			{
				pMatchPlayerCaller->nVoteKickAttempts++;
			}
		}
	}

	// Auto vote 'No' for the person being kicked unless they are idle
	CTFPlayer *pTFVoteTarget = ToTFPlayer( m_hPlayerTarget );
	if ( pTFVoteTarget && !pTFVoteTarget->IsAwayFromKeyboard() && ( pTFVoteTarget->GetTeamNumber() != TEAM_SPECTATOR ) )
	{
		CVoteController *pVoteController = pTFVoteTarget->GetTeamVoteController();
		if ( pVoteController )
			pVoteController->TryCastVote( pTFVoteTarget->entindex(), "Option2" );
	}
 }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDisplayString( void )
{
	switch ( m_eKickReason )
	{
	case TFVoteKickReason_Other:	return "#TF_vote_kick_player_other";
	case TFVoteKickReason_Cheating:	return "#TF_vote_kick_player_cheating";
	case TFVoteKickReason_Idle:		return "#TF_vote_kick_player_idle";
	case TFVoteKickReason_Scamming:	return "#TF_vote_kick_player_scamming";
	}
	return "#TF_vote_kick_player_other";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetVotePassedString( void )
{
	// Player left before we could finish, but still got banned
	if ( !m_hPlayerTarget && sv_vote_kick_ban_duration.GetInt() > 0 )
		return "#TF_vote_passed_ban_player";

	// Player is still here
	return "#TF_vote_passed_kick_player";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDetailsString( void )
{
	if ( m_hPlayerTarget )
		return m_hPlayerTarget->GetPlayerName();

	// If they left, use name stored at creation
	if ( V_strlen( m_szTargetPlayerName ) )
		return m_szTargetPlayerName;
	
	return "Unnamed";
}

//-----------------------------------------------------------------------------
// Purpose: Custom process results -- in match mode, let match system handle
//          vote kick results
//-----------------------------------------------------------------------------
CBaseIssue::EVoteAction CKickIssue::ProcessResults( const CUtlVector <const char*> &vecOptions,
                                                    const int arVoteCountByOption[],
                                                    const CUtlMap<CSteamID, int> &mapVotesBySteamID,
                                                    int nHighestCountOption,
                                                    int nTotalVotes, int nPotentialVoters )
{
	//
	// In match mode, we submit the results to the match system which then records it has a votekick pending.  We then
	// return eVoteAction_Wait until it has done something with it, and just return pass/fail on this vote based on if
	// the match system decided to kick them.  In that mode, we just don't do anything in Execute() (it owns the vote
	// from thereon)
	//
	CMatchInfo *pLiveMatch = GTFGCClientSystem()->GetLiveMatch();

	// If they were ever present in the match we want to submit this to the match system, even if they are now dropped,
	// so it can determine if they should be penalized anyway.  For ad-hoc players, even in match mode, we'll just kick
	// them ourselves, the match system doesn't need to care.
	bool bTargetInMatch = pLiveMatch && pLiveMatch->GetMatchDataForPlayer( m_steamIDVoteTarget );

	if ( m_bSubmittedToMatchSystem )
	{
		// Submitted to match system and returned wait, see if match system finished handling it
		if ( GTFGCClientSystem()->BVoteKickPending( m_steamIDVoteTarget ) )
		{
			return eVoteAction_Wait;
		}
		else
		{
			// No longer a pending vote kick at the match level, see if this guy was kicked or not and pass/fail the
			// vote based on that.
			bool bKicked = GTFGCClientSystem()->BPlayerWasVoteKicked( m_steamIDVoteTarget );
			return bKicked ? eVoteAction_Pass : eVoteAction_Fail;
		}
	}
	else if ( bTargetInMatch )
	{
		// See what the default result would be
		EVoteAction eDefault = CBaseIssue::ProcessResults( vecOptions, arVoteCountByOption, mapVotesBySteamID,
		                                                   nHighestCountOption, nTotalVotes, nPotentialVoters );
		bool bWouldPass = ( eDefault == eVoteAction_Pass );

		// Match, but haven't submitted this (first time called, since no other options stall)
		GTFGCClientSystem()->SubmitVoteKickResults( m_steamIDVoteCaller, m_steamIDVoteTarget,
		                                            m_eKickReason, mapVotesBySteamID, bWouldPass );
		m_bSubmittedToMatchSystem = true;
		// Wait and check back
		return eVoteAction_Wait;
	}
	else
	{
		// Non-match player, including possibly an ad-hoc player in a mixed match mode like bootcamp, do the normal
		// thing
		return CBaseIssue::ProcessResults( vecOptions, arVoteCountByOption, mapVotesBySteamID, nHighestCountOption,
		                                   nTotalVotes, nPotentialVoters );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::NotifyGCAdHocKick( bool bKickedSuccessfully )
{
#if 0 // No longer being collected, see GC job comment
	if ( m_steamIDVoteCaller.IsValid() && m_steamIDVoteTarget.IsValid() && m_steamIDVoteTarget.BIndividualAccount() )
	{
		GCSDK::CProtoBufMsg<CMsgTFVoteKickBanPlayerResult> msg( k_EMsgGCVoteKickBanPlayerResult );
		msg.Body().set_account_id_initiator( m_steamIDVoteCaller.GetAccountID() );
		msg.Body().set_account_id_subject( m_steamIDVoteTarget.GetAccountID() );
		msg.Body().set_kick_successful( bKickedSuccessfully );
		msg.Body().set_kick_reason( m_unKickReason );
		msg.Body().set_num_yes_votes( m_iNumYesVotes );
		msg.Body().set_num_no_votes( m_iNumNoVotes );
		msg.Body().set_num_possible_votes( m_iNumPotentialVotes );
		GCClientSystem()->BSendMessage( msg );
	}
#endif // 0 // Disabled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::PrintLogData( void )
{
	bool bFakeClient = m_hPlayerTarget && ( m_hPlayerTarget->IsFakeClient() || m_hPlayerTarget->IsHLTV() || m_hPlayerTarget->IsReplay() );

	UTIL_LogPrintf( "Kick Vote details:  VoteInitiatorSteamID: %s  VoteTargetSteamID: %s  Valid: %i  BIndividual: %i  Name: %s  Proxy: %i\n",
					m_steamIDVoteCaller.IsValid() ? m_steamIDVoteCaller.Render() : "[unknown]",
					m_steamIDVoteTarget.Render(),
					m_steamIDVoteTarget.IsValid(),
					m_steamIDVoteTarget.BIndividualAccount(),
					m_hPlayerTarget ? m_szTargetPlayerName : "Disconnected",
					bFakeClient );
}

//-----------------------------------------------------------------------------
TFVoteKickReason CKickIssue::ParseKickReason( const char *pszReason )
{
	// Look for TFVoteKickReason_foo
	CFmtStr strEnum( "%s_%s", TFVoteKickReason_descriptor()->name().c_str(), pszReason );

	// TFVoteKickReason_Parse is case sensitive, so search ourselves
	for ( int idxReason = 0; idxReason < TFVoteKickReason_descriptor()->value_count(); idxReason++ )
	{
		const google::protobuf::EnumValueDescriptor *reasonDesc;
		reasonDesc = TFVoteKickReason_descriptor()->value( idxReason );

		if ( V_stricmp( strEnum.Get(), reasonDesc->name().c_str() ) == 0 )
		{
			return (TFVoteKickReason)(reasonDesc->number());
		}
	}

	return TFVoteKickReason_Other;
}

//-----------------------------------------------------------------------------
const char* CKickIssue::KickReasonString( TFVoteKickReason eReason )
{
	// Strip enum_ prefix if relevant
	CFmtStr strPrefix( "%s_", TFVoteKickReason_descriptor()->name().c_str() );
	const char *pszName = TFVoteKickReason_Name( eReason ).c_str();

	if ( V_strncmp( pszName, strPrefix.Get(), strPrefix.Length() ) == 0 )
	{
		return pszName + strPrefix.Length();
	}

	return pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::CreateVoteDataFromDetails( const char *pszDetails )
{
	int iUserID = 0;
	const char *pReasonString = strstr( pszDetails, " " );
	if ( pReasonString != NULL )
	{
		pReasonString += 1;
		CUtlString userID;
		userID.SetDirect( pszDetails, pReasonString - pszDetails );
		iUserID = atoi( userID );
		m_eKickReason = ParseKickReason( pReasonString );
	}
	else
	{
		iUserID = atoi( pszDetails );
	}

	// Try to use the steamID we stored in OnVoteStarted() (will fail for bots)
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		CSteamID steamID;
		if ( pPlayer->GetSteamID( &steamID ) && steamID == m_steamIDVoteTarget )
		{
			m_hPlayerTarget = pPlayer;
			return true;
		}
	}

	// Otherwise rely on userID
	if ( iUserID )
	{
		m_hPlayerTarget = ToBasePlayer( UTIL_PlayerByUserId( iUserID ) );
	}

	return ( m_hPlayerTarget ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !IsEnabled() )
		return;

	char szBuffer[MAX_COMMAND_LENGTH];
	Q_snprintf( szBuffer, MAX_COMMAND_LENGTH, "callvote %s <userID>\n", GetTypeString() );
	ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::OnPlayerDisconnected( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( !m_pVoteController )
		return;

	if ( !IsEnabled() )
		return;

	if ( !m_pVoteController->IsVoteActive() )
		return;

	CBaseIssue *pIssue = m_pVoteController->GetCurrentVote();
	if ( !pIssue || ( pIssue != this ) )
		return;

	if ( pPlayer != m_hPlayerTarget.Get() )
	{
		// remove their vote if they're leaving the server
		CSteamID steamid;
		pPlayer->GetSteamID( &steamid );
		if ( steamid.IsValid() && m_pVoteController->HasPlayerVotedOnCurrentIssue( steamid ) )
		{
			m_pVoteController->RemovePlayerVote( steamid );
		}
	}
	else
	{
		for ( int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++ )
		{
			CBasePlayer *pVoter = UTIL_PlayerByIndex( iIndex );
			if ( pVoter && m_pVoteController->CanTeamCastVote( pVoter->GetTeamNumber() ) && ( pVoter != m_hPlayerTarget.Get() ) && !pVoter->IsFakeClient() )
			{
				CSteamID steamidVoter;
				pVoter->GetSteamID( &steamidVoter );
				if ( steamidVoter.IsValid() && !m_pVoteController->HasPlayerVotedOnCurrentIssue( steamidVoter ) )
				{
					m_pVoteController->TryCastVote( pVoter->entindex(), "Option1" );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Changelevel
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_changelevel_allowed( "sv_vote_issue_changelevel_allowed", "0", FCVAR_NONE, "Can players call votes to change levels?" );
ConVar sv_vote_issue_changelevel_allowed_mvm( "sv_vote_issue_changelevel_allowed_mvm", "0", FCVAR_NONE, "Can players call votes to change levels in MvM?" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeLevelIssue::ExecuteCommand( void )
{
	engine->ChangeLevel( m_szDetailsString, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::CanTeamCallVote( int iTeam ) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return sv_vote_issue_changelevel_allowed_mvm.GetBool();
	}

	return sv_vote_issue_changelevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !Q_strcmp( pszDetails, "" ) )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}
	else
	{
		if ( !VotableMap( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
			return false;
		}

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			// We can't test if it's valid - deny
			if ( !g_pPopulationManager )
			{
				nFailCode = VOTE_FAILED_GENERIC;
				return false;
			}

			if ( !g_pPopulationManager->IsValidMvMMap( pszDetails ) )
			{
				nFailCode = VOTE_FAILED_MAP_NOT_VALID;
				return false;
			}
		}
		else
		{
			if ( MultiplayRules() && !MultiplayRules()->IsMapInMapCycle( pszDetails ) )
			{
				nFailCode = VOTE_FAILED_MAP_NOT_VALID;
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDisplayString( void )
{
	return "#TF_vote_changelevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_changelevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDetailsString( void )
{
	return m_szDetailsString;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !sv_vote_issue_changelevel_allowed.GetBool() )
		return;

	char szBuffer[MAX_COMMAND_LENGTH];
	Q_snprintf( szBuffer, MAX_COMMAND_LENGTH, "callvote %s <mapname>\n", GetTypeString() );
	ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::IsYesNoVote( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Nextlevel
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
ConVar sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode", "0", FCVAR_NONE, "Present players with a list of lowest playtime maps to choose from?" );
ConVar sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
ConVar sv_vote_issue_nextlevel_prevent_change( "sv_vote_issue_nextlevel_prevent_change", "1", FCVAR_NONE, "Not allowed to vote for a nextlevel if one has already been set." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{
	m_IssueOptions.RemoveAll();

	// Reserve the last option for "Extend current Map?"
	int nNumOptions = sv_vote_issue_nextlevel_allowextend.GetBool() ? GetNumberVoteOptions() - 1 : GetNumberVoteOptions();

	// Ask the stats system for playtime data
	if ( CTF_GameStats.GetVoteData( "NextLevel", nNumOptions, m_IssueOptions ) )
	{
		FOR_EACH_VEC( m_IssueOptions, iIndex )
		{
			vecNames.AddToTail( m_IssueOptions[iIndex] );
		}

		if ( sv_vote_issue_nextlevel_allowextend.GetBool() || m_IssueOptions.Count() == 1 )
		{
			vecNames.AddToTail( "Extend current Map" );
		}
	
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNextLevelIssue::ExecuteCommand( void )
{
	if ( Q_strcmp( m_szDetailsString, "Extend current Map" ) == 0 )
	{
		// Players want to extend the current map, so extend any existing limits
		if ( mp_timelimit.GetInt() > 0 )
		{
			engine->ServerCommand( CFmtStr( "mp_timelimit %d;", mp_timelimit.GetInt() + 20 ) );
		}

		if ( mp_maxrounds.GetInt() > 0 )
		{
			engine->ServerCommand( CFmtStr( "mp_maxrounds %d;", mp_maxrounds.GetInt() + 2 ) );
		}

		if ( mp_winlimit.GetInt() > 0 )
		{
			engine->ServerCommand( CFmtStr( "mp_winlimit %d;", mp_winlimit.GetInt() + 2 ) );
		}
	}
	else
	{
		nextlevel.SetValue( m_szDetailsString );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::CanTeamCallVote( int iTeam ) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return false;
	}

	return sv_vote_issue_nextlevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	// TFGameRules created vote
	if ( sv_vote_issue_nextlevel_choicesmode.GetBool() && iEntIndex == 99 )
	{
		// Invokes a UI down stream
		if ( Q_strcmp( pszDetails, "" ) == 0 )
		{
			return true;
		}

		return false;
	}
	
	if( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( Q_strcmp( pszDetails, "" ) == 0 )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}
	else
	{
		if ( !VotableMap( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
			return false;
		}

		if ( MultiplayRules() && !MultiplayRules()->IsMapInMapCycle( pszDetails ) )
		{
			nFailCode = VOTE_FAILED_MAP_NOT_VALID;
			return false;
		}
	}

	if ( sv_vote_issue_nextlevel_prevent_change.GetBool() )
	{
		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			nFailCode = VOTE_FAILED_NEXTLEVEL_SET;
			return false;
		}
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDisplayString( void )
{
	// If we don't have a map passed in already...
	if ( Q_strcmp( m_szDetailsString, "" ) == 0 )
	{
		if ( sv_vote_issue_nextlevel_choicesmode.GetBool() )
		{
			return "#TF_vote_nextlevel_choices";
		}
	}

	return "#TF_vote_nextlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetVotePassedString( void )
{
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() )
	{
		if ( Q_strcmp( m_szDetailsString, "Extend current Map" ) == 0 )
		{
			return "#TF_vote_passed_nextlevel_extend";
		}
	}
	
	return "#TF_vote_passed_nextlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDetailsString( void )
{
	return m_szDetailsString;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNextLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !sv_vote_issue_nextlevel_allowed.GetBool() )
		return;

	if ( !sv_vote_issue_nextlevel_choicesmode.GetBool() )
	{
		char szBuffer[MAX_COMMAND_LENGTH];
		Q_snprintf( szBuffer, MAX_COMMAND_LENGTH, "callvote %s <mapname>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuffer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::IsYesNoVote( void )
{
	// If we don't have a map name already, this will trigger a list of choices
	if ( Q_strcmp( m_szDetailsString, "" ) == 0 )
	{
		if ( sv_vote_issue_nextlevel_choicesmode.GetBool() )
			return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNextLevelIssue::GetNumberVoteOptions( void )
{
	// If we don't have a map name already, this will trigger a list of choices
	if ( Q_strcmp( m_szDetailsString, "" ) == 0 )
	{
		if ( sv_vote_issue_nextlevel_choicesmode.GetBool() )
			return MAX_VOTE_OPTIONS;
	}

	// Vote on a specific map - Yes, No
	return 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNextLevelIssue::GetQuorumRatio( void )
{
	// We don't really care about a quorum in this case.  If a few
	// people have a preference on the next level, and no one else
	// bothers to vote, just let their choice pass.
	if ( sv_vote_issue_nextlevel_choicesmode.GetBool() )
		return 0.1f;

	// Default
	return sv_vote_quorum_ratio.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Extend the current level
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_extendlevel_allowed( "sv_vote_issue_extendlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
ConVar sv_vote_issue_extendlevel_quorum( "sv_vote_issue_extendlevel_quorum", "0.6", FCVAR_NONE, "What is the ratio of voters needed to reach quorum?" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtendLevelIssue::ExecuteCommand( void )
{
	// Players want to extend the current map, so extend any existing limits
	if ( mp_timelimit.GetInt() > 0 )
	{
		engine->ServerCommand( CFmtStr( "mp_timelimit %d;", mp_timelimit.GetInt() + 20 ) );
	}

	if ( mp_maxrounds.GetInt() > 0 )
	{
		engine->ServerCommand( CFmtStr( "mp_maxrounds %d;", mp_maxrounds.GetInt() + 2 ) );
	}

	if ( mp_winlimit.GetInt() > 0 )
	{
		engine->ServerCommand( CFmtStr( "mp_winlimit %d;", mp_winlimit.GetInt() + 2 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CExtendLevelIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return false;
	}

	return sv_vote_issue_extendlevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CExtendLevelIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CExtendLevelIssue::GetDisplayString( void )
{
	return "#TF_vote_extendlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtendLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( !sv_vote_issue_extendlevel_allowed.GetBool() ) 
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CExtendLevelIssue::GetVotePassedString( void )
{
	// We already had a localized string for this, even though we never use it.
	return "#TF_vote_passed_nextlevel_extend";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CExtendLevelIssue::GetQuorumRatio( void )
{
	// Our own quorom 
	return sv_vote_issue_extendlevel_quorum.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Scramble Teams Issue
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_scramble_teams_allowed( "sv_vote_issue_scramble_teams_allowed", "1", FCVAR_NONE, "Can players call votes to scramble the teams?" );
ConVar sv_vote_issue_scramble_teams_cooldown( "sv_vote_issue_scramble_teams_cooldown", "1200", FCVAR_NONE, "Minimum time before another scramble vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrambleTeams::ExecuteCommand( void )
{
	if ( sv_vote_issue_scramble_teams_cooldown.GetInt() )
	{
		SetIssueCooldownDuration( sv_vote_issue_scramble_teams_cooldown.GetFloat() );
	}

	engine->ServerCommand( "mp_scrambleteams 2;" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CScrambleTeams::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
			return false;
	}

	return sv_vote_issue_scramble_teams_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CScrambleTeams::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( TFGameRules() && TFGameRules()->ShouldScrambleTeams() )
	{
		nFailCode = VOTE_FAILED_SCRAMBLE_IN_PROGRESS;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CScrambleTeams::GetDisplayString( void )
{
	return "#TF_vote_scramble_teams";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CScrambleTeams::GetVotePassedString( void )
{
	return "#TF_vote_passed_scramble_teams";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrambleTeams::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !sv_vote_issue_scramble_teams_allowed.GetBool() )
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: MvM Challenge Issue
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_mvm_challenge_allowed( "sv_vote_issue_mvm_challenge_allowed", "1", FCVAR_NONE, "Can players call votes to set the challenge level?" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineChangeChallengeIssue::ExecuteCommand( void )
{
	if ( Q_stricmp( m_szDetailsString, "normal" ) == 0 )
	{
		engine->ServerCommand( CFmtStr( "tf_mvm_popfile \"%s\";", STRING(gpGlobals->mapname) ) );
	}
	else
	{
		engine->ServerCommand( CFmtStr( "tf_mvm_popfile \"%s\";", m_szDetailsString ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMannVsMachineChangeChallengeIssue::CanTeamCallVote( int iTeam ) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: MvM-specific issue
//-----------------------------------------------------------------------------
bool CMannVsMachineChangeChallengeIssue::IsEnabled( void )
{
	// Only allow in MvM
	if ( TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
		return false;

	// But prevent on MannUp (Valve) servers
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch && pMatch->m_eMatchGroup == k_eTFMatchGroup_MvM_MannUp )
	{
		return false;
	}

	return sv_vote_issue_mvm_challenge_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMannVsMachineChangeChallengeIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( Q_strcmp( pszDetails, "" ) == 0 )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}
	else
	{
		CUtlString fullPath;
		if ( !g_pPopulationManager->FindPopulationFileByShortName( pszDetails, fullPath ) ||
			 // did we fall back to something other than what we asked for?
			 ( !FStrEq( pszDetails, "normal" ) && !Q_stristr( fullPath, pszDetails ) ) ||
			 !g_pPopulationManager->IsValidPopfile( fullPath ) )
		{
			nFailCode = VOTE_FAILED_INVALID_ARGUMENT;
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CMannVsMachineChangeChallengeIssue::GetDisplayString( void )
{
	return "#TF_vote_changechallenge";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CMannVsMachineChangeChallengeIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_changechallenge";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CMannVsMachineChangeChallengeIssue::GetDetailsString( void )
{
	return m_szDetailsString;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineChangeChallengeIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if( !sv_vote_issue_mvm_challenge_allowed.GetBool() )
		return;

	char szBuffer[MAX_COMMAND_LENGTH];
	Q_snprintf( szBuffer, MAX_COMMAND_LENGTH, "callvote %s <popfile>\n", GetTypeString() );
	ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMannVsMachineChangeChallengeIssue::IsYesNoVote( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineChangeChallengeIssue::GetNumberVoteOptions( void )
{
	// Vote on a specific map - Yes, No
	return 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEnableTemporaryHalloweenIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	// Prevent concommand calling of this vote
	if ( iEntIndex != DEDICATED_SERVER )
		return false;

	if( TFGameRules()->IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
	{
		nFailCode = VOTE_FAILED_MODIFICATION_ALREADY_ACTIVE;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnableTemporaryHalloweenIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void SendVoteResponseToGC( bool bVoteResponse )
{
	if ( GTFGCClientSystem() )
	{
		// Tell the GC.
		GCSDK::CProtoBufMsg<CMsgGC_GameServer_UseServerModificationItem_Response> msgResponse( k_EMsgGC_GameServer_UseServerModificationItem_Response );
		msgResponse.Body().set_server_response_code( bVoteResponse ? CMsgGC_GameServer_UseServerModificationItem_Response::kServerModificationItemServerResponse_Accepted : CMsgGC_GameServer_UseServerModificationItem_Response::kServerModificationItemServerResponse_VoteFailed );
		GTFGCClientSystem()->BSendMessage( msgResponse );

		// Tell the players on the server.
		if ( bVoteResponse )
		{
			TFGameRules()->BroadcastSound( 255, RandomInt( 0, 100 ) <= 10 ? "Halloween.MerasmusHalloweenModeRare" : "Halloween.MerasmusHalloweenModeCommon" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnableTemporaryHalloweenIssue::ExecuteCommand( void )
{
	SendVoteResponseToGC( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnableTemporaryHalloweenIssue::OnVoteFailed( int iEntityHoldingVote )
{
	CBaseTFIssue::OnVoteFailed( iEntityHoldingVote );

	SendVoteResponseToGC( false );
}

//-----------------------------------------------------------------------------
// Purpose: Enable/Disable mp_autoteambalance
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_autobalance_allowed( "sv_vote_issue_autobalance_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable auto team balance?" );
ConVar sv_vote_issue_autobalance_cooldown( "sv_vote_issue_autobalance_cooldown", "300", FCVAR_NONE, "Minimum time before another auto team balance vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTeamAutoBalanceIssue::GetTypeStringLocalized( void )
{
	// Disabled
	if ( !mp_autoteambalance.GetInt() )
	{
		return "#Vote_TeamAutoBalance_Enable";
	}

	return "#Vote_TeamAutoBalance_Disable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamAutoBalanceIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_autobalance_cooldown.GetInt() )
	{
		SetIssueCooldownDuration( sv_vote_issue_autobalance_cooldown.GetFloat() );
	}

	// Disable
	if ( mp_autoteambalance.GetInt() )
	{
		engine->ServerCommand( "mp_autoteambalance 0;" );
	}
	// Enable
	else
	{
		engine->ServerCommand( "mp_autoteambalance 1;" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamAutoBalanceIssue::IsEnabled( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsDefaultGameMode() )
		return false;

	return sv_vote_issue_autobalance_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamAutoBalanceIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTeamAutoBalanceIssue::GetDisplayString( void )
{
	// Disable
	if ( mp_autoteambalance.GetInt() )
		return "#TF_vote_autobalance_disable";

	// Enable
	return "#TF_vote_autobalance_enable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTeamAutoBalanceIssue::GetVotePassedString( void )
{
	// Disable
	if ( mp_autoteambalance.GetInt() )
		return "#TF_vote_passed_autobalance_disable";

	// Enable
	return "#TF_vote_passed_autobalance_enable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamAutoBalanceIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( !sv_vote_issue_autobalance_allowed.GetBool() )
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamAutoBalanceIssue::GetQuorumRatio( void )
{
	float flRatio = sv_vote_quorum_ratio.GetFloat();

	// Disable
	if ( mp_autoteambalance.GetInt() )
		return flRatio;

	// Enable
	return Max( 0.1f, flRatio * 0.7f );
}

//-----------------------------------------------------------------------------
// Purpose: Enable/Disable tf_classlimit
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_classlimits_allowed( "sv_vote_issue_classlimits_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable per-class limits?" );
ConVar sv_vote_issue_classlimits_allowed_mvm( "sv_vote_issue_classlimits_allowed_mvm", "0", FCVAR_NONE, "Can players call votes in Mann-Vs-Machine to enable or disable per-class limits?" );
ConVar sv_vote_issue_classlimits_max( "sv_vote_issue_classlimits_max", "4", FCVAR_NONE, "Maximum number of players (per-team) that can be any one class.", true, 1.f, false, 16.f );
ConVar sv_vote_issue_classlimits_max_mvm( "sv_vote_issue_classlimits_max_mvm", "2", FCVAR_NONE, "Maximum number of players (per-team) that can be any one class.", true, 1.f, false, 16.f );
ConVar sv_vote_issue_classlimits_cooldown( "sv_vote_issue_classlimits_cooldown", "300", FCVAR_NONE, "Minimum time before another classlimits vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CClassLimitsIssue::GetTypeStringLocalized( void )
{
	// Disabled
	if ( !tf_classlimit.GetInt() )
	{
		return "#Vote_ClassLimit_Enable";
	}

	return "#Vote_ClassLimit_Disable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLimitsIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_classlimits_cooldown.GetInt() )
	{
		SetIssueCooldownDuration( sv_vote_issue_classlimits_cooldown.GetFloat() );
	}

	// Disable
	if ( tf_classlimit.GetInt() )
	{
		engine->ServerCommand( "tf_classlimit 0;" );
	}
	// Enable
	else
	{
		int nLimit = ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() ) ? sv_vote_issue_classlimits_max_mvm.GetInt() : sv_vote_issue_classlimits_max.GetInt();
		engine->ServerCommand( CFmtStr( "tf_classlimit %i;", nLimit ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CClassLimitsIssue::IsEnabled( void )
{
	if ( TFGameRules() )
	{
		// Manages class limits already
		if ( TFGameRules()->IsInTournamentMode() )
			return false;

		// Manages class limits already
		if ( TFGameRules()->IsInHighlanderMode() )
			return false;

		if ( TFGameRules()->IsMannVsMachineMode() )
			return sv_vote_issue_classlimits_allowed_mvm.GetBool();
	}

	return sv_vote_issue_classlimits_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CClassLimitsIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CClassLimitsIssue::GetDisplayString( void )
{
	// Disable
	if ( tf_classlimit.GetInt() )
		return "#TF_vote_classlimits_disable";

	// Enable
	return "#TF_vote_classlimits_enable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CClassLimitsIssue::GetVotePassedString( void )
{
	// Disable
	if ( tf_classlimit.GetInt() )
		return "#TF_vote_passed_classlimits_disable";

	// Enable
	return "#TF_vote_passed_classlimits_enable";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLimitsIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && !sv_vote_issue_classlimits_allowed_mvm.GetBool() )
		return;

	if ( !sv_vote_issue_classlimits_allowed.GetBool() )
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CClassLimitsIssue::GetDetailsString( void )
{
	int nLimit = ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() ) ? sv_vote_issue_classlimits_max_mvm.GetInt() : sv_vote_issue_classlimits_max.GetInt();
	m_sRetString = CFmtStr( "%i", nLimit );
	return m_sRetString.String();
}


//-----------------------------------------------------------------------------
// Purpose: Pause Game
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_pause_game_allowed( "sv_vote_issue_pause_game_allowed", "0", FCVAR_HIDDEN, "Can players call votes to pause the game?" );
ConVar sv_vote_issue_pause_game_timer( "sv_vote_issue_pause_game_timer", "120", FCVAR_HIDDEN, "How long to pause the game for when this vote passes (in seconds)." );
ConVar sv_vote_issue_pause_game_cooldown( "sv_vote_issue_pause_game_cooldown", "1200", FCVAR_HIDDEN, "Minimum time before another pause vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPauseGameIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_pause_game_cooldown.GetInt() )
	{
		SetIssueCooldownDuration( sv_vote_issue_pause_game_cooldown.GetFloat() );
	}

	engine->SetPausedForced( true, sv_vote_issue_pause_game_timer.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPauseGameIssue::IsEnabled( void )
{
	if ( engine->IsPaused() )
		return false;

	return sv_vote_issue_pause_game_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPauseGameIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( !CBaseTFIssue::RequestCallVote( iEntIndex, pszDetails, nFailCode, nTime ) )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPauseGameIssue::GetDisplayString( void )
{
	return "#TF_vote_pause_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPauseGameIssue::GetVotePassedString( void )
{
	return "#TF_vote_passed_pause_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPauseGameIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( !sv_vote_issue_pause_game_allowed.GetBool() )
		return;

	ListStandardNoArgCommand( pForWhom, GetTypeString() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPauseGameIssue::GetDetailsString( void )
{
	m_sRetString = CFmtStr( "%i", sv_vote_issue_pause_game_timer.GetInt() );
	return (m_sRetString.String());
}

