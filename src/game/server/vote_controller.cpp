//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base VoteController.  Handles holding and voting on issues.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vote_controller.h"
#include "shareddefs.h"
#include "eiface.h"
#include "team.h"
#include "gameinterface.h"
#include "fmtstr.h"

#ifdef TF_DLL
#include "tf/tf_gamerules.h"
#include "tf/tf_voteissues.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_VOTER_HISTORY 64

// Datatable
IMPLEMENT_SERVERCLASS_ST( CVoteController, DT_VoteController )
	SendPropInt( SENDINFO( m_iActiveIssueIndex ) ),
	SendPropInt( SENDINFO( m_nVoteIdx ) ),
	SendPropInt( SENDINFO( m_iOnlyTeamToVote ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nVoteOptionCount ), SendPropInt( SENDINFO_ARRAY( m_nVoteOptionCount ), 8, SPROP_UNSIGNED ) ),
	SendPropInt( SENDINFO( m_nPotentialVotes ) ),
	SendPropBool( SENDINFO( m_bIsYesNoVote ) )
END_SEND_TABLE()

BEGIN_DATADESC( CVoteController )
	DEFINE_THINKFUNC( VoteControllerThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( vote_controller, CVoteController );

CVoteController *g_voteControllerGlobal = NULL;
CVoteController *g_voteControllerRed = NULL;
CVoteController *g_voteControllerBlu = NULL;

ConVar sv_vote_timer_duration( "sv_vote_timer_duration", "15", FCVAR_NONE, "How long to allow voting on an issue" );
ConVar sv_vote_timer_allow_early_finish( "sv_vote_timer_allow_early_finish", "1", FCVAR_NONE, "If all votes are in, whether to end the vote (for debugging)" );
ConVar sv_vote_command_delay( "sv_vote_command_delay", "2", FCVAR_DEVELOPMENTONLY, "How long after a vote passes until the action happens", false, 0.f, true, 4.5f );

ConVar sv_allow_votes( "sv_allow_votes", "1", FCVAR_NONE, "Allow voting?" );
ConVar sv_vote_failure_timer( "sv_vote_failure_timer", "300", FCVAR_NONE, "A vote that fails cannot be re-submitted for this long" );
#ifdef TF_DLL
ConVar sv_vote_failure_timer_mvm( "sv_vote_failure_timer_mvm", "120", FCVAR_NONE, "A vote that fails in MvM cannot be re-submitted for this long" );
#endif // TF_DLL
ConVar sv_vote_creation_timer( "sv_vote_creation_timer", "150", FCVAR_NONE, "How long before a player can attempt to call another vote (in seconds)." );
ConVar sv_vote_quorum_ratio( "sv_vote_quorum_ratio", "0.6", FCVAR_NOTIFY, "The minimum ratio of eligible players needed to pass a vote.  Min 0.1, Max 1.0.", true, 0.1f, true, 1.0f );
ConVar sv_vote_allow_spectators( "sv_vote_allow_spectators", "0", FCVAR_NONE, "Allow spectators to vote?" );
ConVar sv_vote_ui_hide_disabled_issues( "sv_vote_ui_hide_disabled_issues", "1", FCVAR_NONE, "Suppress listing of disabled issues in the vote setup screen." );

ConVar sv_vote_holder_may_vote_no( "sv_vote_holder_may_vote_no", "0", FCVAR_REPLICATED, "1 = Vote caller is not forced to vote yes on yes/no votes." );
ConVar sv_vote_bots_allowed( "sv_vote_bots_allowed", "0", FCVAR_NONE, "Allow bots to vote or not." );

static const int k_nKickWatchListMaxDuration = 300;

static int s_nVoteIdx = 0;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CVoteControllerSystem : public CAutoGameSystemPerFrame
{
public:
	CVoteControllerSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
		SetDefLessFunc( m_mapKickWatchList );
		SetDefLessFunc( m_mapNameLockedList );
		m_flNextKickCheckTime = 0.f;
		m_flNextNameLockCheckTime = 0.f;
	}

	virtual void LevelInitPreEntity()
	{
		m_flNextNameLockCheckTime = 0.f;
		m_flNextKickCheckTime = 0.f;
	}

	void UpdateVoteController( CVoteController *pVoteController )
	{
		// Vote passed - execute the command
		if ( pVoteController->m_executeCommandTimer.HasStarted() && pVoteController->m_executeCommandTimer.IsElapsed() )
		{
			pVoteController->m_executeCommandTimer.Invalidate();
			pVoteController->m_potentialIssues[pVoteController->m_iActiveIssueIndex]->ExecuteCommand();
		}
	}

	virtual void FrameUpdatePostEntityThink( void )
	{
		// Executing the vote controller command needs to happen in the PostEntityThink as it can restart levels and
		//	blast entities, etc. If you're doing this during a regular think, this can cause entities thinking after
		//	you in Physics_RunThinkFunctions() to get grumpy and crash.

		if ( g_voteControllerGlobal )
			UpdateVoteController( g_voteControllerGlobal );

		if ( g_voteControllerRed )
			UpdateVoteController( g_voteControllerRed );

		if ( g_voteControllerBlu )
			UpdateVoteController( g_voteControllerBlu );

		// Kick watch
		if ( m_flNextKickCheckTime < gpGlobals->curtime )
		{
			FOR_EACH_MAP( m_mapKickWatchList, i )
			{
				if ( gpGlobals->curtime > m_mapKickWatchList[i] )
				{
					m_mapKickWatchList.RemoveAt( i );
					break;	// Constantly called code - resume on next pass
				}

				CBasePlayer *pTarget = UTIL_PlayerBySteamID( m_mapKickWatchList.Key( i ) );
				if ( pTarget )
				{
					// Welcome back
					engine->ServerCommand( CFmtStr( "kickid %d %s;", pTarget->GetUserID(), "Kicked by server." ) );
				}
			}

			m_flNextKickCheckTime = gpGlobals->curtime + 0.2f;
		}

		// Name lock management
		if ( m_flNextNameLockCheckTime < gpGlobals->curtime )
		{
			FOR_EACH_MAP( m_mapNameLockedList, i )
			{
				CBasePlayer *pPlayer = UTIL_PlayerBySteamID( m_mapNameLockedList.Key( i ) );

				// Time up?
				if ( gpGlobals->curtime > m_mapNameLockedList[i] )
				{
					// Disable the lock if they're still here
					if ( pPlayer )
					{
						engine->ServerCommand( UTIL_VarArgs( "namelockid %d %d\n", pPlayer->GetUserID(), 0 ) );
					}

					// Remove and break - this will re-run in 1 second
					m_mapNameLockedList.RemoveAt( i );
					break;
				}
				// See if they reconnected
				else if ( pPlayer && !engine->IsPlayerNameLocked( pPlayer->edict() ) )
				{
					engine->ServerCommand( UTIL_VarArgs( "namelockid %d %d\n", pPlayer->GetUserID(), 1 ) );
				}
			}

			m_flNextNameLockCheckTime = gpGlobals->curtime + 1.f;
		}
	}

	void AddPlayerToKickWatchList( CSteamID steamID, float flDuration )
	{
		if ( !steamID.IsValid() || !steamID.BIndividualAccount() )
			return;

		flDuration = clamp( flDuration, 1.f, (float)k_nKickWatchListMaxDuration );
		if ( m_mapKickWatchList.Find( steamID ) == m_mapKickWatchList.InvalidIndex() )
		{
			m_mapKickWatchList.Insert( steamID, ( gpGlobals->curtime + flDuration ) );
		}
	}

	void AddPlayerToNameLockedList( CSteamID steamID, float flDuration )
	{
		if ( !steamID.IsValid() || !steamID.BIndividualAccount() )
			return;

		flDuration = clamp( flDuration, 1.f, (float)k_nKickWatchListMaxDuration );
		if ( m_mapNameLockedList.Find( steamID ) == m_mapNameLockedList.InvalidIndex() )
		{
			m_mapNameLockedList.Insert( steamID, ( gpGlobals->curtime + flDuration ) );
		}
	}

private:

	CUtlMap< CSteamID, float > m_mapKickWatchList;
	CUtlMap< CSteamID, float > m_mapNameLockedList;
	float m_flNextKickCheckTime;
	float m_flNextNameLockCheckTime;
};

CVoteControllerSystem VoteControllerSystem( "CVoteControllerSystem" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CommandListIssues( void )
{
	CBasePlayer *pCommandIssuer = UTIL_GetCommandClient();

	if ( !pCommandIssuer )
		return;

	CVoteController *pTeamVoteController = pCommandIssuer->GetTeamVoteController();

	// Team issues, eg. vote kick
	if ( pTeamVoteController )
		pTeamVoteController->ListIssues( pCommandIssuer );

	// Always list global issues (map change, etc)
	if ( g_voteControllerGlobal )
		g_voteControllerGlobal->ListIssues( pCommandIssuer );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ConCommand ListIssues("listissues", CommandListIssues, "List all the issues that can be voted on.", 0);

//-----------------------------------------------------------------------------
// Purpose: This should eventually ask the player what team they are voting on
// to take into account different idle / spectator rules.
//-----------------------------------------------------------------------------

int GetVoterTeam( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return TEAM_UNASSIGNED;

	int iTeam = pEntity->GetTeamNumber();

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CON_COMMAND( callvote, "Start a vote on an issue." )
{
	if ( !g_voteControllerGlobal || !g_voteControllerRed || !g_voteControllerBlu )
	{
		DevMsg( "Vote Controller Not Found!\n" );
		return;
	}

	CBasePlayer *pVoteCaller = UTIL_GetCommandClient();
	if ( !pVoteCaller )
		return;

	if ( !sv_vote_allow_spectators.GetBool() )
	{
		if ( pVoteCaller->GetTeamNumber() == TEAM_SPECTATOR )
		{
			g_voteControllerGlobal->SendVoteCreationFailedMessage( VOTE_FAILED_SPECTATOR, pVoteCaller );
			return;
		}
	}

	// If we don't have any arguments, invoke VoteSetup UI
	if ( args.ArgC() < 2 )
	{
		CVoteController::SetupVote( pVoteCaller->entindex() );
		return;
	}

	const char *arg2 = args[1];
	char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
	szEmptyDetails[0] = '\0';
	const char *arg3 = args.ArgC() >= 3 ? args[2] : szEmptyDetails;

	CVoteController *pTeamVoteController = pVoteCaller->GetTeamVoteController();

	CVoteController *pVoteController = NULL;
	if ( g_voteControllerGlobal->HasIssue( arg2 ) )
		pVoteController = g_voteControllerGlobal;
	else if ( pTeamVoteController && pTeamVoteController->HasIssue( arg2 ) )
		pVoteController = pTeamVoteController;
	else
	{
		DevMsg( "Vote Issue Not Found!\n" );
		return;
	}

	if ( !pVoteController )
		return;

	bool bActiveVote = pVoteController->IsVoteActive();
#if 1
	if ( bActiveVote )
	{
		ClientPrint( pVoteCaller, HUD_PRINTCENTER, "#GameUI_vote_failed_vote_in_progress" );
		return;
	}
#else
	// Josh:
	// Right now TF2's UI only supports showing 1 vote at once.
	//
	// If there's a team vote active, don't allow calling whatever
	// halloween enablement vote people are trying, as the kick
	// is way more important.
	bool bIsTeamVote = pVoteController == g_voteControllerRed || pVoteController == g_voteControllerBlu;
	bool bActiveTeamVote = g_voteControllerRed->IsVoteActive() || g_voteControllerBlu->IsVoteActive();
	bool bActiveGlobalVote = g_voteControllerGlobal->IsVoteActive();

	if ( bActiveVote || ( bActiveTeamVote && !bIsTeamVote ) )
	{
		ClientPrint( pVoteCaller, HUD_PRINTCENTER, "#GameUI_vote_failed_vote_in_progress" );
		return;
	}

	// Josh:
	// Additionally, I can already pre-empt what bots will do,
	// they will just start spam-voting random global events like Halloween
	// or Changelevel, etc to avoid getting kicked.
	// Always let a team-vote cancel an on-going global vote.
	if ( bActiveGlobalVote && bIsTeamVote )
	{
		SendVoteFailedToPassMessage( VOTE_FAILED_VOTE_IN_PROGRESS );
		g_voteControllerGlobal->m_potentialIssues[m_iActiveIssueIndex]->OnVoteFailed( m_iEntityHoldingVote );
		g_voteControllerGlobal->m_potentialIssues[m_iActiveIssueIndex]->OnVoteEnded();
		g_voteControllerGlobal->ResetData();
	}
#endif

	// Ask the controller if this is allowed
	int nCooldown = 0;
	vote_create_failed_t nError = VOTE_FAILED_GENERIC;

	if ( !pVoteController->CanEntityCallVote( pVoteCaller, nCooldown, nError ) )
	{
		pVoteController->SendVoteCreationFailedMessage( nError, pVoteCaller, nCooldown );
		return;
	}

	pVoteController->CreateVote( pVoteCaller->entindex(), arg2, arg3 );
}

//-----------------------------------------------------------------------------
CVoteController::CVoteController() : m_mapVotesBySteamID( DefLessFunc( CSteamID ) )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CVoteController::~CVoteController()
{
	if ( g_voteControllerGlobal == this )	{ g_voteControllerGlobal = NULL; }
	else if ( g_voteControllerRed == this )	{ g_voteControllerRed = NULL; }
	else if ( g_voteControllerBlu == this ) { g_voteControllerBlu = NULL; }


	for( int issueIndex = 0; issueIndex < m_potentialIssues.Count(); ++issueIndex )
	{
		delete m_potentialIssues[issueIndex];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::ResetData( void )
{
	m_iActiveIssueIndex = INVALID_ISSUE;

	for ( int index = 0; index < m_nVoteOptionCount.Count(); index++ )
	{
		m_nVoteOptionCount.Set( index, 0 );
	}

	m_nVoteIdx = -1;
	m_nPotentialVotes = 0;
	m_acceptingVotesTimer.Invalidate();
	m_executeCommandTimer.Invalidate();
	m_iEntityHoldingVote = -1;
	m_iOnlyTeamToVote = TEAM_UNASSIGNED;
	m_bIsYesNoVote = true;

	m_mapVotesBySteamID.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::Spawn( void )
{
	ResetData();

	BaseClass::Spawn();

	SetThink( &CVoteController::VoteControllerThink );
	SetNextThink( gpGlobals->curtime );

	SetDefLessFunc( m_VoteCallers );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CVoteController::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CVoteController::IsVoteSystemEnabled( void )
{
	return sv_allow_votes.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CVoteController::CanTeamCastVote( int iTeam ) const
{
	if ( m_iOnlyTeamToVote == TEAM_UNASSIGNED )
		return true;

	return iTeam == m_iOnlyTeamToVote;
}

//-----------------------------------------------------------------------------
// Purpose: Handles menu-driven setup of Voting
//-----------------------------------------------------------------------------
bool CVoteController::SetupVote( int iEntIndex )
{
	CBasePlayer *pVoteCaller = UTIL_PlayerByIndex( iEntIndex );
	if( !pVoteCaller )
		return false;

	int nIssueCount = 0;

	CVoteController *pVoteControllers[] =
	{
		g_voteControllerGlobal,
		pVoteCaller->GetTeamVoteController(),
	};

	// Passing an nIssueCount of 0 triggers a "Voting disabled on server" message in the setup UI
	if ( IsVoteSystemEnabled() )
	{
		for ( int j = 0; j < ARRAYSIZE( pVoteControllers ); j++ ) 
		{
			if ( !pVoteControllers[ j ] )
				continue;

			auto &potentialIssues = pVoteControllers[ j ]->m_potentialIssues;
			for( int iIndex = 0; iIndex < potentialIssues.Count(); ++iIndex )
			{
				// Hide disabled issues?
				CBaseIssue *pCurrentIssue = potentialIssues[iIndex];
				if ( pCurrentIssue )
				{
					if ( !pCurrentIssue->IsEnabled() && sv_vote_ui_hide_disabled_issues.GetBool() )
						continue;

					nIssueCount++;
				}
			}
		}
	}

	CSingleUserRecipientFilter filter( pVoteCaller );
	filter.MakeReliable();
	UserMessageBegin( filter, "VoteSetup" );
	WRITE_BYTE( nIssueCount );
	int nMsgSize = 0;

	for ( int j = 0; j < ARRAYSIZE( pVoteControllers ); j++ ) 
	{
		if ( !pVoteControllers[ j ] )
			continue;

		auto &potentialIssues = pVoteControllers[ j ]->m_potentialIssues;

		for( int iIndex = 0; iIndex < potentialIssues.Count(); ++iIndex )
		{
			CBaseIssue *pCurrentIssue = potentialIssues[iIndex];
			if ( pCurrentIssue )
			{
				// Don't send/display disabled issues when set
				if ( !pCurrentIssue->IsEnabled() && sv_vote_ui_hide_disabled_issues.GetBool() )
					continue;

				// Don't exceed MAX_USER_MSG_DATA (hack)
				nMsgSize += ( V_strlen( pCurrentIssue->GetTypeString() ) + 1 );
				nMsgSize += ( V_strlen( pCurrentIssue->GetTypeStringLocalized() ) + 1 );
				++nMsgSize;
				Assert( nMsgSize <= MAX_USER_MSG_DATA );
				if ( nMsgSize > MAX_USER_MSG_DATA )
					continue;

				WRITE_STRING( pCurrentIssue->GetTypeString() );
				WRITE_STRING( pCurrentIssue->GetTypeStringLocalized() );
				WRITE_BYTE( pCurrentIssue->IsEnabled() );
			}
		}
	}

	MessageEnd();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Handles console-driven setup of Voting
//-----------------------------------------------------------------------------
bool CVoteController::CreateVote( int iEntIndex, const char *pszTypeString, const char *pszDetailString )
{
	// Terrible Hack:  Dedicated servers pass 99 as the EntIndex
	bool bDedicatedServer = ( iEntIndex == DEDICATED_SERVER ) ? true : false;

	if ( !IsVoteSystemEnabled() )
		return false;

	// Already running a vote?
	if ( IsVoteActive() )
		return false;

	CBasePlayer *pVoteCaller = UTIL_PlayerByIndex( iEntIndex );
	if ( !pVoteCaller && !bDedicatedServer )
		return false;

	// Find the issue the user is asking for
	for ( int issueIndex = 0; issueIndex < m_potentialIssues.Count(); ++issueIndex )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[issueIndex];
		if ( !pCurrentIssue )
			{ return false; }

		if ( FStrEq( pszTypeString, pCurrentIssue->GetTypeString() ) )
		{
			vote_create_failed_t nErrorCode = VOTE_FAILED_GENERIC;
			int nTime = 0;
			if ( pCurrentIssue->RequestCallVote( iEntIndex, pszDetailString, nErrorCode, nTime ) )
			{
				// Establish a bunch of data on this particular issue
				pCurrentIssue->SetIssueDetails( pszDetailString );
				m_bIsYesNoVote = pCurrentIssue->IsYesNoVote();
				m_iActiveIssueIndex = issueIndex;
				m_iEntityHoldingVote = iEntIndex;
				m_nVoteIdx = s_nVoteIdx++;
				if ( !bDedicatedServer )
				{
					m_iOnlyTeamToVote = ( pCurrentIssue->IsTeamRestrictedVote() ) ? GetVoterTeam( pVoteCaller ) : TEAM_UNASSIGNED;
				}

				// Now get our choices
				m_VoteOptions.RemoveAll();
				pCurrentIssue->GetVoteOptions( m_VoteOptions );
				int nNumVoteOptions = m_VoteOptions.Count();
				if ( nNumVoteOptions >= 2 )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "vote_options" );
					if ( event )
					{
						event->SetInt( "voteidx", m_nVoteIdx );
						event->SetInt( "count", nNumVoteOptions );
						for ( int iIndex = 0; iIndex < nNumVoteOptions; iIndex++ )
						{
							char szNumber[2];
							Q_snprintf( szNumber, sizeof( szNumber ), "%i", iIndex + 1 );

							char szOptionName[8] = "option";
							Q_strncat( szOptionName, szNumber, sizeof( szOptionName ), COPY_ALL_CHARACTERS );

							event->SetString( szOptionName, m_VoteOptions[iIndex] );
						}
						gameeventmanager->FireEvent( event );
					}
				}
				else
				{
					Assert( nNumVoteOptions >= 2 );
				}

				// Have the issue start working on it
				pCurrentIssue->OnVoteStarted();

				// Now the vote handling and UI
				m_nPotentialVotes = pCurrentIssue->CountPotentialVoters();
				m_acceptingVotesTimer.Start( sv_vote_timer_duration.GetFloat() + random->RandomFloat( -1.f, 1.f ) );

#ifndef _DEBUG
				// Force the vote holder to agree with a Yes/No vote
				if ( pCurrentIssue->IsYesNoVote() && !bDedicatedServer && !sv_vote_holder_may_vote_no.GetBool() )
				{
					TryCastVote( iEntIndex, "Option1" );
				}
#endif

				// Get the data out to the client
				CBroadcastRecipientFilter filter;
				filter.MakeReliable();
				UserMessageBegin( filter, "VoteStart" );
					WRITE_BYTE( m_iOnlyTeamToVote );			// move into the filter
					WRITE_LONG( m_nVoteIdx );
					WRITE_BYTE( m_iEntityHoldingVote );
					WRITE_STRING( pCurrentIssue->GetDisplayString() );
					WRITE_STRING( pCurrentIssue->GetDetailsString() );
					WRITE_BOOL( pCurrentIssue->IsYesNoVote() );
					WRITE_BYTE( ( pCurrentIssue->m_hPlayerTarget ) ? pCurrentIssue->m_hPlayerTarget->entindex() : 0 );
				MessageEnd();

				if ( !bDedicatedServer )
				{
					TrackVoteCaller( pVoteCaller, sv_vote_creation_timer.GetFloat() );
				}

				return true;
			}
			else
			{
				if ( !bDedicatedServer )
				{
					SendVoteCreationFailedMessage( nErrorCode, pVoteCaller, nTime );
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: The vote failed to start - let the caller know why
//-----------------------------------------------------------------------------
void CVoteController::SendVoteCreationFailedMessage( vote_create_failed_t nReason, CBasePlayer *pVoteCaller, int nTime /*= -1*/ )
{
	Assert( pVoteCaller );
	if ( !pVoteCaller )
		return;

	// No error message if the vote issue handled the request itself.
	if ( nReason == VOTE_FAILED_REQUEST_HANDLED_BY_ISSUE )
		{ return; }

	CSingleUserRecipientFilter user( pVoteCaller );
	user.MakeReliable();

	UserMessageBegin( user, "CallVoteFailed" );
	WRITE_BYTE( nReason );
	WRITE_SHORT( nTime );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: The vote was called, but failed to pass - let everyone know why
//-----------------------------------------------------------------------------
void CVoteController::SendVoteFailedToPassMessage( vote_create_failed_t nReason )
{
	Assert( m_potentialIssues[m_iActiveIssueIndex] );

	UTIL_LogPrintf( "Vote failed \"%s %s\" with code %i\n", m_potentialIssues[m_iActiveIssueIndex]->GetTypeString(), m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString(), (int)nReason );

	CBroadcastRecipientFilter filter;
	filter.MakeReliable();

	UserMessageBegin( filter, "VoteFailed" );
	WRITE_BYTE( m_iOnlyTeamToVote );
	WRITE_LONG( m_nVoteIdx );
	WRITE_BYTE( nReason );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose:  Player generated a vote command.  i.e. /vote option1
//-----------------------------------------------------------------------------
CVoteController::TryCastVoteResult CVoteController::TryCastVote( int iEntIndex, const char *pszVoteString )
{
	if ( !IsVoteSystemEnabled() )
		return CAST_FAIL_SERVER_DISABLE;

	if ( !IsVoteActive() )
		return CAST_FAIL_NO_ACTIVE_ISSUE;

	if ( m_executeCommandTimer.HasStarted() )
		return CAST_FAIL_VOTE_CLOSED;

	// Lookup voter
	CBasePlayer *pVoter = UTIL_PlayerByIndex( iEntIndex );
	if ( !pVoter )
		{ return CAST_FAIL_SYSTEM_ERROR; }
	CSteamID steamidVoter;
	pVoter->GetSteamID( &steamidVoter );
	if ( !steamidVoter.IsValid() )
		{ return CAST_FAIL_SYSTEM_ERROR; }

	if ( m_potentialIssues[m_iActiveIssueIndex] && m_potentialIssues[m_iActiveIssueIndex]->IsTeamRestrictedVote() )
	{
		CBaseEntity *pVoteHolder = UTIL_EntityByIndex( m_iEntityHoldingVote );

		if ( ( pVoteHolder == NULL ) || ( pVoter == NULL ) || ( GetVoterTeam( pVoteHolder ) != GetVoterTeam( pVoter ) ) )
		{
			return CAST_FAIL_TEAM_RESTRICTED;
		}
	}

	// Look for a previous vote
	int idxVoter = m_mapVotesBySteamID.Find( steamidVoter );
	int nOldVote = ( idxVoter == m_mapVotesBySteamID.InvalidIndex() ? VOTE_UNCAST : m_mapVotesBySteamID[idxVoter] );
#ifndef DEBUG
	if ( nOldVote != VOTE_UNCAST )
	{
		return CAST_FAIL_NO_CHANGES;
	}
#endif // !DEBUG

	// Which option are they voting for?
	int nCurrentVote = VOTE_UNCAST;
	if ( Q_strnicmp( pszVoteString, "Option", 6 ) != 0 )
		return CAST_FAIL_SYSTEM_ERROR;

	nCurrentVote = (CastVote)( atoi( pszVoteString + 6 ) - 1 );

	if ( nCurrentVote < VOTE_OPTION1 || nCurrentVote > VOTE_OPTION5 )
		return CAST_FAIL_SYSTEM_ERROR;

	// They're changing their vote
#ifdef DEBUG
	if ( nOldVote != VOTE_UNCAST )
	{
		if( nOldVote == nCurrentVote )
		{
			return CAST_FAIL_DUPLICATE;
		}
		VoteChoice_Decrement( nOldVote );
	}
#endif // DEBUG

	// With a Yes/No vote, slam anything past "No" to No
	if ( m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() )
	{
		if ( nCurrentVote > VOTE_OPTION2 )
			nCurrentVote = VOTE_OPTION2;
	}

	// Register and track this vote
	VoteChoice_Increment( nCurrentVote );
	m_mapVotesBySteamID.InsertOrReplace( steamidVoter, nCurrentVote );

	// Tell the client-side UI
	IGameEvent *event = gameeventmanager->CreateEvent( "vote_cast" );
	if ( event )
	{
		event->SetInt( "voteidx", m_nVoteIdx );
		event->SetInt( "vote_option", nCurrentVote );
		event->SetInt( "team", m_iOnlyTeamToVote );
		event->SetInt( "entityid", iEntIndex );
		gameeventmanager->FireEvent( event );
	}

	CheckForEarlyVoteClose();

	return CAST_OK;
}

//-----------------------------------------------------------------------------
// Purpose:  Increments the vote count for a particular vote option 
//			 i.e. nVoteChoice = 0 might mean a Yes vote
//-----------------------------------------------------------------------------
void CVoteController::VoteChoice_Increment( int nVoteChoice )
{
	if ( nVoteChoice < VOTE_OPTION1 || nVoteChoice > VOTE_OPTION5 )
		return;

	int nValue = m_nVoteOptionCount.Get( nVoteChoice );
	m_nVoteOptionCount.Set( nVoteChoice, ++nValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::VoteChoice_Decrement( int nVoteChoice )
{
	if ( nVoteChoice < VOTE_OPTION1 || nVoteChoice > VOTE_OPTION5 )
		return;

	int nValue = m_nVoteOptionCount.Get( nVoteChoice );
	m_nVoteOptionCount.Set( nVoteChoice, --nValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::VoteControllerThink( void )
{
	if ( !m_potentialIssues.IsValidIndex( m_iActiveIssueIndex ) )
	{
		SetNextThink( gpGlobals->curtime + 0.5f );

		return;
	}

	// Vote time is up - process the result
	if ( m_acceptingVotesTimer.HasStarted() && m_acceptingVotesTimer.IsElapsed() )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[m_iActiveIssueIndex];
		CBaseIssue::EVoteAction eVoteAction = pCurrentIssue->ProcessResults( m_VoteOptions,
		                                                                     m_nVoteOptionCount.Base(),
		                                                                     m_mapVotesBySteamID,
		                                                                     GetVoteIssueIndexWithHighestCount(),
		                                                                     GetNumVotesCast(),
		                                                                     m_nPotentialVotes );

		bool bVotePassed = false;
		switch ( eVoteAction )
		{
			case CBaseIssue::eVoteAction_Wait:
			{
				// Vote needs more time to think about the results
				SetNextThink( gpGlobals->curtime + 0.1f );
				return;
			}
			case CBaseIssue::eVoteAction_Pass:
			{
				bVotePassed = true;
				break;
			}
			case CBaseIssue::eVoteAction_Fail:
			{
				bVotePassed = false;
				break;
			}
			default: Assert( false );
		}

		// Got a result (didn't decide to wait)
		m_acceptingVotesTimer.Invalidate();

		if ( bVotePassed )
		{
			float flDelay = sv_vote_command_delay.GetFloat();
#ifdef TF_DLL
			if ( dynamic_cast< CKickIssue* >( m_potentialIssues[m_iActiveIssueIndex] ) )
			{
				// Don't delay successful kick votes
				flDelay = 0.f;
			}
#endif
			m_executeCommandTimer.Start( flDelay );
			m_resetVoteTimer.Start( 5.f );

			UTIL_LogPrintf( "Vote succeeded \"%s %s\"\n", m_potentialIssues[m_iActiveIssueIndex]->GetTypeString(), m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString() );

			CBroadcastRecipientFilter filter;
			filter.MakeReliable();

			UserMessageBegin( filter, "VotePass" );
				WRITE_BYTE( m_iOnlyTeamToVote );
				WRITE_LONG( m_nVoteIdx );
				WRITE_STRING( m_potentialIssues[m_iActiveIssueIndex]->GetVotePassedString() );
				WRITE_STRING( m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString() );
			MessageEnd();
		}
		else
		{
			vote_create_failed_t nReason = m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() ? VOTE_FAILED_YES_MUST_EXCEED_NO : VOTE_FAILED_QUORUM_FAILURE;
			SendVoteFailedToPassMessage( nReason );
			m_potentialIssues[m_iActiveIssueIndex]->OnVoteFailed( m_iEntityHoldingVote );
			m_resetVoteTimer.Start( 5.f );
		}

		m_potentialIssues[m_iActiveIssueIndex]->OnVoteEnded();
	}

	// Vote passed check moved down to FrameUpdatePostEntityThink at bottom of this file...

	if ( m_resetVoteTimer.HasStarted() && m_resetVoteTimer.IsElapsed() )
	{
		ResetData();
		m_resetVoteTimer.Invalidate();
	}

	// Size maintenance on m_VoteCallers
	if ( m_VoteCallers.Count() >= MAX_VOTER_HISTORY )
	{
		// Remove older entries
		for ( int iIdx = m_VoteCallers.FirstInorder(); iIdx != m_VoteCallers.InvalidIndex(); iIdx = m_VoteCallers.NextInorder( iIdx ) )
		{
			if ( m_VoteCallers[ iIdx ] - gpGlobals->curtime <= 0 )
			{
				m_VoteCallers.Remove( iIdx );
			}
		}
	}

	SetNextThink( gpGlobals->curtime + 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: End the vote early if everyone's voted
//-----------------------------------------------------------------------------
void CVoteController::CheckForEarlyVoteClose( void )
{
	if ( !sv_vote_timer_allow_early_finish.GetBool() )
		return;

	int nVoteTally = 0;
	for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		nVoteTally += m_nVoteOptionCount.Get( index );
	}

	if( nVoteTally >= m_nPotentialVotes )
	{
		m_acceptingVotesTimer.Start( 1.f );	// Run the timer out
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CVoteController::IsValidVoter( CBasePlayer *pWhom )
{
	if ( !pWhom  )
		return false;

	if ( !pWhom->IsConnected() )
		return false;

	if ( pWhom->GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( !sv_vote_allow_spectators.GetBool() )
	{
		if ( pWhom->GetTeamNumber() == TEAM_SPECTATOR )
			return false;
	}

	if ( !sv_vote_bots_allowed.GetBool() )
	{
		if ( pWhom->IsBot() )
			return false;

		if ( pWhom->IsFakeClient() )
			return false;
	}

	if ( pWhom->IsHLTV() )
		return false;

	if ( pWhom->IsReplay() )
		return false;

#ifdef TF_DLL
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( pWhom->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS )
			return false;
	}
#endif // TF_DLL

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::RegisterIssue( CBaseIssue *pszNewIssue )
{
	m_potentialIssues.AddToTail( pszNewIssue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteController::ListIssues( CBasePlayer *pForWhom )
{
	if ( !IsVoteSystemEnabled() )
		return;

	ClientPrint( pForWhom, HUD_PRINTCONSOLE, "---Vote commands---\n" );

	for( int issueIndex = 0; issueIndex < m_potentialIssues.Count(); ++issueIndex )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[issueIndex];
		pCurrentIssue->ListIssueDetails( pForWhom );
	}
	ClientPrint( pForWhom, HUD_PRINTCONSOLE, "--- End Vote commands---\n" );
}

//-----------------------------------------------------------------------------
// Purpose: -1 when invalid
//-----------------------------------------------------------------------------
int CVoteController::GetVoteIssueIndexWithHighestCount( void )
{
	int nMaxIndex = -1;
	
	// Legacy Yes/No system
	if ( m_iActiveIssueIndex != INVALID_ISSUE && m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() )
	{
		return ( m_nVoteOptionCount[VOTE_OPTION1] > m_nVoteOptionCount[VOTE_OPTION2] ) ? VOTE_OPTION1 : VOTE_OPTION2;
	}
	// Which option had the most votes?
	else
	{
		int nMaxCount = 0;

		// TODO: Handle ties
		for ( int iIndex = 0; iIndex < m_nVoteOptionCount.Count(); iIndex ++ )
		{
			if ( m_nVoteOptionCount[iIndex] && m_nVoteOptionCount[iIndex] > nMaxCount )
			{
				nMaxCount = m_nVoteOptionCount[iIndex];
				nMaxIndex = iIndex;
			}
		}
	}

	return nMaxIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Store steamIDs for every player that calls a vote
//-----------------------------------------------------------------------------
void CVoteController::TrackVoteCaller( CBasePlayer *pPlayer, float flTime )
{
	if ( !pPlayer )
		return;

	CSteamID steamID;
	pPlayer->GetSteamID( &steamID );

	int iIdx = m_VoteCallers.Find( steamID.ConvertToUint64() );
	if ( iIdx != m_VoteCallers.InvalidIndex() )
	{
		// Already being tracked - update timer
		m_VoteCallers[iIdx] = gpGlobals->curtime + flTime;
		return;
	}

	m_VoteCallers.Insert( steamID.ConvertToUint64(), gpGlobals->curtime + sv_vote_creation_timer.GetInt() );
};

//-----------------------------------------------------------------------------
// Purpose: Check the history of steamIDs that called votes and test against a timer
//-----------------------------------------------------------------------------
bool CVoteController::CanEntityCallVote( CBasePlayer *pPlayer, int &nCooldown, vote_create_failed_t &nErrorCode )
{
	if ( !pPlayer )
		return false;

	if ( !pPlayer->BCanCallVote() )
	{
		nErrorCode = VOTE_FAILED_PLAYER_TRANSITIONING;
		return false;
	}

	CSteamID steamID;
	pPlayer->GetSteamID( &steamID );

	// Has this SteamID tried to call a vote recently?
	int iIdx = m_VoteCallers.Find( steamID.ConvertToUint64() );
	if ( iIdx != m_VoteCallers.InvalidIndex() )
	{
		// Timer elapsed?
		nCooldown = (int)( m_VoteCallers[ iIdx ] - gpGlobals->curtime );
		if ( nCooldown > 0 )
		{
			nErrorCode = VOTE_FAILED_RATE_EXCEEDED;
			return false;
		}

		// Expired
		m_VoteCallers.Remove( iIdx );
	}

	return true;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CVoteController::GetNumVotesCast( void )
{
	int nVoteTally = 0;

	for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		nVoteTally += m_nVoteOptionCount.Get( index );
	}

	return nVoteTally;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteController::AddPlayerToKickWatchList( CSteamID steamID, float flDuration )
{
	VoteControllerSystem.AddPlayerToKickWatchList( steamID, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteController::AddPlayerToNameLockedList( CSteamID steamID, float flDuration, int nUserID )
{
	engine->ServerCommand( UTIL_VarArgs( "namelockid %d %d\n", nUserID, 1 ) );

	VoteControllerSystem.AddPlayerToNameLockedList( steamID, flDuration );
}

bool CVoteController::HasIssue( const char *pszIssue )
{
	for ( int issueIndex = 0; issueIndex < m_potentialIssues.Count(); ++issueIndex )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[ issueIndex ];
		if ( !pCurrentIssue )
			return false;

		if ( FStrEq( pszIssue, pCurrentIssue->GetTypeString() ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseIssue *CVoteController::GetCurrentVote()
{
	if ( IsVoteActive() )
	{
		return m_potentialIssues[m_iActiveIssueIndex];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CVoteController::HasPlayerVotedOnCurrentIssue( CSteamID steamID )
{
	int idx = m_mapVotesBySteamID.Find( steamID );
	return ( idx != m_mapVotesBySteamID.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteController::RemovePlayerVote( CSteamID steamID )
{
	int idx = m_mapVotesBySteamID.Find( steamID );
	if ( idx != m_mapVotesBySteamID.InvalidIndex() )
	{
		m_nPotentialVotes--;
		VoteChoice_Decrement( m_mapVotesBySteamID[idx] );
		m_mapVotesBySteamID.RemoveAt( idx );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteController::OnPlayerDisconnected( CBasePlayer *pPlayer )
{
	CBaseIssue *pCurrentVote = GetCurrentVote();
	if ( pCurrentVote )
	{
		pCurrentVote->OnPlayerDisconnected( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: BaseIssue
//-----------------------------------------------------------------------------
CBaseIssue::CBaseIssue( const char *pszTypeString, CVoteController *pVoteController )
	: m_pVoteController( pVoteController )
{
	V_strcpy_safe( m_szTypeString, pszTypeString );

	m_iNumYesVotes = 0;
	m_iNumNoVotes = 0;
	m_iNumPotentialVotes = 0;
	m_flNextCallTime = -1.f;

	ASSERT( m_pVoteController );
	if ( m_pVoteController )
		m_pVoteController->RegisterIssue( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseIssue::~CBaseIssue()
{
	for ( int index = 0; index < m_FailedVotes.Count(); index++ )
	{
		FailedVote *pFailedVote = m_FailedVotes[index];
		delete pFailedVote;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CBaseIssue::GetTypeString( void )
{
	return m_szTypeString;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CBaseIssue::GetDetailsString( void )
{
	return m_szDetailsString;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseIssue::SetIssueDetails( const char *pszDetails )
{
	V_strcpy_safe( m_szDetailsString, pszDetails );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseIssue::IsTeamRestrictedVote( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CBaseIssue::GetVotePassedString( void )
{
	return "Unknown vote passed.";
}

//-----------------------------------------------------------------------------
// Purpose:  Store failures to prevent vote spam
//-----------------------------------------------------------------------------
void CBaseIssue::OnVoteFailed( int iEntityHoldingVote )
{
	// Don't track failed dedicated server votes
	if ( BRecordVoteFailureEventForEntity( iEntityHoldingVote ) )
	{
		// Check for an existing match
		for ( int index = 0; index < m_FailedVotes.Count(); index++ )
		{
			FailedVote *pFailedVote = m_FailedVotes[index];
			if ( Q_strcmp( pFailedVote->szFailedVoteParameter, GetDetailsString() ) == 0 )
			{
				int nTime = sv_vote_failure_timer.GetInt();

#ifdef TF_DLL
				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
				{
					nTime = sv_vote_failure_timer_mvm.GetInt();
				}
#endif // TF_DLL

				pFailedVote->flLockoutTime = gpGlobals->curtime + nTime;

				return;
			}
		}

		// Need to create a new one
		FailedVote *pNewFailedVote = new FailedVote;
		int iIndex = m_FailedVotes.AddToTail( pNewFailedVote );
		V_strcpy_safe( m_FailedVotes[iIndex]->szFailedVoteParameter, GetDetailsString() );
		m_FailedVotes[iIndex]->flLockoutTime = gpGlobals->curtime + sv_vote_failure_timer.GetFloat();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseIssue::CanTeamCallVote( int iTeam ) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseIssue::RequestCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	// Automated server vote - don't bother testing against it
	if ( !BRecordVoteFailureEventForEntity( iEntIndex ) )
		return true;

	// Bogus player
	if ( iEntIndex == -1 )
		return false;

	// Note: Issue timers reset on level change because the class is created/destroyed during transitions.
	// It'd be nice to refactor the basic framework of the system to get rid of side-effects like this.
	if ( m_flNextCallTime != -1.f && gpGlobals->curtime < m_flNextCallTime )
	{
		nFailCode = VOTE_FAILED_ON_COOLDOWN;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

#ifdef TF_DLL
	if ( TFGameRules() && TFGameRules()->IsInWaitingForPlayers() && !TFGameRules()->IsInTournamentMode() )
	{
		nFailCode = VOTE_FAILED_WAITINGFORPLAYERS;
		return false;
	}
#endif // TF_DLL

	CBaseEntity *pVoteCaller = UTIL_EntityByIndex( iEntIndex );
	if ( pVoteCaller )
	{
		if ( !CanTeamCallVote( GetVoterTeam( pVoteCaller ) ) )
		{
			nFailCode = VOTE_FAILED_TEAM_CANT_CALL;
			return false;
		}
		else if ( !pVoteCaller->BCanCallVote() )
		{
			nFailCode = VOTE_FAILED_PLAYER_TRANSITIONING;
			return false;
		}
	}

	// Did this fail recently?
	for ( int iIndex = 0; iIndex < m_FailedVotes.Count(); iIndex++ )
	{
		FailedVote *pCurrentFailure = m_FailedVotes[iIndex];
		int nTimeRemaining = pCurrentFailure->flLockoutTime - gpGlobals->curtime;
		bool bFailed = false;

		// If this issue requires a parameter, see if we're voting for the same one again (i.e. changelevel ctf_2fort)
		if ( Q_strlen( pCurrentFailure->szFailedVoteParameter ) > 0 )
		{
			if( nTimeRemaining > 1 && FStrEq( pCurrentFailure->szFailedVoteParameter, pszDetails ) )
			{
				bFailed = true;
			}
		}
		// Otherwise we have a parameter-less vote, so just check the lockout timer (i.e. restartgame)
		else
		{
			if( nTimeRemaining > 1 )
			{
				bFailed = true;

			}
		}

		if ( bFailed )
		{
			nFailCode = VOTE_FAILED_ON_COOLDOWN;
			nTime = nTimeRemaining;
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBaseIssue::CountPotentialVoters( void )
{
	int nTotalPlayers = 0;

	if ( !m_pVoteController )
		return 0;

	for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; ++playerIndex )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );
		if( m_pVoteController->IsValidVoter( pPlayer ) )
		{
			if ( m_pVoteController->CanTeamCastVote( GetVoterTeam( pPlayer ) ) )
			{
				nTotalPlayers++;
			}
		}
	}

	return nTotalPlayers;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBaseIssue::GetNumberVoteOptions( void )
{
	return 2;  // The default issue is Yes/No (so 2), but it can be anywhere between 1 and MAX_VOTE_COUNT
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseIssue::IsYesNoVote( void )
{
	return true;  // Default
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseIssue::ListStandardNoArgCommand( CBasePlayer *forWhom, const char *issueString )
{
	ClientPrint( forWhom, HUD_PRINTCONSOLE, "callvote %s1\n", issueString );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{
	// The default vote issue is a Yes/No vote
	vecNames.AddToHead( "Yes" );
	vecNames.AddToTail( "No" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseIssue::GetQuorumRatio( void )
{
	return sv_vote_quorum_ratio.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Default processing of results - pick highest option if voters are
//          above GetQuorumRatio
//-----------------------------------------------------------------------------
CBaseIssue::EVoteAction CBaseIssue::ProcessResults( const CUtlVector <const char*> &vecOptions,
                                                    const int arVoteCountByOption[],
                                                    const CUtlMap<CSteamID, int> &mapVotesBySteamID,
                                                    int nHighestCountOption,
                                                    int nTotalVotes, int nPotentialVoters )
{
	NoteUnused( arVoteCountByOption );
	NoteUnused( mapVotesBySteamID );

	bool bVotePassed = false;
	if ( nTotalVotes >= (int)( nPotentialVoters * GetQuorumRatio() ) )
	{
		int nPassingVoteOptionIndex = nHighestCountOption;
		if ( nPassingVoteOptionIndex >= 0 && nPassingVoteOptionIndex < MAX_VOTE_OPTIONS )
		{
			// YES/NO VOTES - hard-wired to VOTE_OPTION1 (Yes)
			if ( IsYesNoVote() )
			{
				if ( nPassingVoteOptionIndex == VOTE_OPTION1 )
				{
					bVotePassed = true;
				}
			}
			// GENERAL VOTES - as long as there's a quorum, go with the most popular choice
			else
			{
				bVotePassed = true;

				// By default, update our own details to what the winning vote is
				SetIssueDetails( vecOptions[nPassingVoteOptionIndex] );
			}
		}
	}

	return bVotePassed ? eVoteAction_Pass : eVoteAction_Fail;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseIssue::OnVoteEnded( void )
{
}

