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

#ifdef TF_DLL
#include "tf/tf_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_VOTER_HISTORY 64

// Datatable
IMPLEMENT_SERVERCLASS_ST( CVoteController, DT_VoteController )
	SendPropInt( SENDINFO( m_iActiveIssueIndex ) ),
	SendPropInt( SENDINFO( m_iOnlyTeamToVote ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nVoteOptionCount ), SendPropInt( SENDINFO_ARRAY( m_nVoteOptionCount ), 8, SPROP_UNSIGNED ) ),
	SendPropInt( SENDINFO( m_nPotentialVotes ) ),
	SendPropBool( SENDINFO( m_bIsYesNoVote ) )
END_SEND_TABLE()

BEGIN_DATADESC( CVoteController )
	DEFINE_THINKFUNC( VoteControllerThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( vote_controller, CVoteController );

CVoteController *g_voteController = NULL;

ConVar sv_vote_timer_duration("sv_vote_timer_duration", "15", FCVAR_DEVELOPMENTONLY, "How long to allow voting on an issue");
ConVar sv_vote_command_delay("sv_vote_command_delay", "2", FCVAR_DEVELOPMENTONLY, "How long after a vote passes until the action happens", false, 0, true, 4.5);
ConVar sv_allow_votes("sv_allow_votes", "1", 0, "Allow voting?");
ConVar sv_vote_failure_timer("sv_vote_failure_timer", "300", 0, "A vote that fails cannot be re-submitted for this long");
#ifdef TF_DLL
ConVar sv_vote_failure_timer_mvm( "sv_vote_failure_timer_mvm", "120", 0, "A vote that fails in MvM cannot be re-submitted for this long" );
#endif // TF_DLL
ConVar sv_vote_creation_timer("sv_vote_creation_timer", "120", FCVAR_DEVELOPMENTONLY, "How often someone can individually call a vote.");
ConVar sv_vote_quorum_ratio( "sv_vote_quorum_ratio", "0.6", 1, "The minimum ratio of players needed to vote on an issue to resolve it.", true, 0.1, true, 1.0 );
ConVar sv_vote_allow_spectators( "sv_vote_allow_spectators", "0", 0, "Allow spectators to vote?" );
ConVar sv_vote_ui_hide_disabled_issues( "sv_vote_ui_hide_disabled_issues", "1", 0, "Suppress listing of disabled issues in the vote setup screen." );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CommandListIssues( void )
{
	CBasePlayer *commandIssuer = UTIL_GetCommandClient();

	if ( g_voteController && commandIssuer )
	{
		g_voteController->ListIssues(commandIssuer);
	}
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
	if ( !g_voteController )
	{
		DevMsg( "Vote Controller Not Found!\n" );
			return;
	}

	CBasePlayer *pVoteCaller = UTIL_GetCommandClient();
	if( !pVoteCaller )
		return;

	if ( !sv_vote_allow_spectators.GetBool() )
	{
		if ( pVoteCaller->GetTeamNumber() == TEAM_SPECTATOR )
		{
			g_voteController->SendVoteFailedMessage( VOTE_FAILED_SPECTATOR, pVoteCaller );
			return;
		}
	}

	// Prevent spamming commands
#ifndef _DEBUG
	int nCooldown = 0;
	if ( !g_voteController->CanEntityCallVote( pVoteCaller, nCooldown ) )
	{
		g_voteController->SendVoteFailedMessage( VOTE_FAILED_RATE_EXCEEDED, pVoteCaller, nCooldown );
		return;
	}
#endif

	// Parameters
	char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
	szEmptyDetails[0] = '\0';
	const char *arg2 = args[1];
	const char *arg3 = args.ArgC() >= 3 ? args[2] : szEmptyDetails;

	// If we don't have any arguments, invoke VoteSetup UI
	if( args.ArgC() < 2 )
	{
		g_voteController->SetupVote( pVoteCaller->entindex() );
		return;
	}

	g_voteController->CreateVote( pVoteCaller->entindex(), arg2, arg3 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CVoteController::~CVoteController()
{
	g_voteController = NULL;

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

	m_nPotentialVotes = 0;
	m_acceptingVotesTimer.Invalidate();
	m_executeCommandTimer.Invalidate();
	m_iEntityHoldingVote = -1;
	m_iOnlyTeamToVote = TEAM_INVALID;
	m_bIsYesNoVote = true;

	for( int voteIndex = 0; voteIndex < ARRAYSIZE( m_nVotesCast ); ++voteIndex )
	{
		m_nVotesCast[voteIndex] = VOTE_UNCAST;
	}
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

	g_voteController = this;
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
bool CVoteController::CanTeamCastVote( int iTeam ) const
{
	if ( m_iOnlyTeamToVote == TEAM_INVALID )
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

	// Passing an nIssueCount of 0 triggers a "Voting disabled on server" message in the setup UI
	if ( sv_allow_votes.GetBool() )
	{
		for( int iIndex = 0; iIndex < m_potentialIssues.Count(); ++iIndex )
		{
			// Hide disabled issues?
			CBaseIssue *pCurrentIssue = m_potentialIssues[iIndex];
			if ( pCurrentIssue )
			{
				if ( !pCurrentIssue->IsEnabled() && sv_vote_ui_hide_disabled_issues.GetBool() )
					continue;

				nIssueCount++;
			}
		}
	}

	CSingleUserRecipientFilter filter( pVoteCaller );
	filter.MakeReliable();
	UserMessageBegin( filter, "VoteSetup" );
	WRITE_BYTE( nIssueCount );

	for( int iIndex = 0; iIndex < m_potentialIssues.Count(); ++iIndex )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[iIndex];
		if ( pCurrentIssue )
		{
			if ( pCurrentIssue->IsEnabled() )
			{
				WRITE_STRING( pCurrentIssue->GetTypeString() );
			}
			else
			{
				// Don't send/display disabled issues when set
				if ( sv_vote_ui_hide_disabled_issues.GetBool() )
					continue;

				char szDisabledIssueStr[MAX_COMMAND_LENGTH + 12];
				V_strcpy( szDisabledIssueStr, pCurrentIssue->GetTypeString() );
				V_strcat( szDisabledIssueStr, " (Disabled on Server)", sizeof(szDisabledIssueStr) );

				WRITE_STRING( szDisabledIssueStr );
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

	if( !sv_allow_votes.GetBool() )
		return false;

	// Already running a vote?
	if( IsVoteActive() )
		return false;

	CBasePlayer *pVoteCaller = UTIL_PlayerByIndex( iEntIndex );
	if( !pVoteCaller && !bDedicatedServer )
		return false;

	// Find the issue the user is asking for
	for( int issueIndex = 0; issueIndex < m_potentialIssues.Count(); ++issueIndex )
	{
		CBaseIssue *pCurrentIssue = m_potentialIssues[issueIndex];
		if ( !pCurrentIssue )
			return false;
		
		if( FStrEq( pszTypeString, pCurrentIssue->GetTypeString() ) )
		{
			vote_create_failed_t nErrorCode = VOTE_FAILED_GENERIC;
			int nTime = 0;
			if( pCurrentIssue->CanCallVote( iEntIndex, pszDetailString, nErrorCode, nTime ) )
			{
				// Establish a bunch of data on this particular issue
				pCurrentIssue->SetIssueDetails( pszDetailString );
				m_bIsYesNoVote = pCurrentIssue->IsYesNoVote();
				m_iActiveIssueIndex = issueIndex;
				m_iEntityHoldingVote = iEntIndex;
				if ( !bDedicatedServer )
				{
					if( pCurrentIssue->IsAllyRestrictedVote() )
					{
						m_iOnlyTeamToVote = GetVoterTeam( pVoteCaller );
					}
					else
					{
						m_iOnlyTeamToVote = TEAM_INVALID;
					}
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
				m_acceptingVotesTimer.Start( sv_vote_timer_duration.GetFloat() );

				// Force the vote holder to agree with a Yes/No vote
				if ( m_bIsYesNoVote && !bDedicatedServer )
				{
					TryCastVote( iEntIndex, "Option1" );
				}

				// Get the data out to the client
				CBroadcastRecipientFilter filter;
				filter.MakeReliable();
				UserMessageBegin( filter, "VoteStart" );
					WRITE_BYTE( m_iOnlyTeamToVote );			// move into the filter
					WRITE_BYTE( m_iEntityHoldingVote );
					WRITE_STRING( pCurrentIssue->GetDisplayString() );
					WRITE_STRING( pCurrentIssue->GetDetailsString() );
					WRITE_BOOL( m_bIsYesNoVote );
				MessageEnd();

				if ( !bDedicatedServer )
				{
					TrackVoteCaller( pVoteCaller );
				}

				return true;
			}
			else
			{
				if ( !bDedicatedServer )
				{
					SendVoteFailedMessage( nErrorCode, pVoteCaller, nTime );
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sent to everyone, unless we pass a player pointer
//-----------------------------------------------------------------------------
void CVoteController::SendVoteFailedMessage( vote_create_failed_t nReason, CBasePlayer *pVoteCaller, int nTime )
{
	// driller: need to merge all failure case stuff into a single path
	if ( pVoteCaller ) 
	{
		CSingleUserRecipientFilter user( pVoteCaller );
		user.MakeReliable();

		UserMessageBegin( user, "CallVoteFailed" );
			WRITE_BYTE( nReason );
			WRITE_SHORT( nTime );
		MessageEnd();
	}
	else
	{
		UTIL_LogPrintf("Vote failed \"%s %s\" \n",
			m_potentialIssues[m_iActiveIssueIndex]->GetTypeString(),
			m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString() );

		CBroadcastRecipientFilter filter;
		filter.MakeReliable();

		UserMessageBegin( filter, "VoteFailed" );
			WRITE_BYTE( m_iOnlyTeamToVote );
			WRITE_BYTE( nReason );
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Player generated a vote command.  i.e. /vote option1
//-----------------------------------------------------------------------------
CVoteController::TryCastVoteResult CVoteController::TryCastVote( int iEntIndex, const char *pszVoteString )
{
	if( !sv_allow_votes.GetBool() )
		return CAST_FAIL_SERVER_DISABLE;

	if( iEntIndex >= ARRAYSIZE( m_nVotesCast ) )
		return CAST_FAIL_SYSTEM_ERROR;

	if( !IsVoteActive() )
		return CAST_FAIL_NO_ACTIVE_ISSUE;

	if( m_executeCommandTimer.HasStarted() )
		return CAST_FAIL_VOTE_CLOSED;

	if( m_potentialIssues[m_iActiveIssueIndex] && m_potentialIssues[m_iActiveIssueIndex]->IsAllyRestrictedVote() )
	{
		CBaseEntity *pVoteHolder = UTIL_EntityByIndex( m_iEntityHoldingVote );
		CBaseEntity *pVoter = UTIL_EntityByIndex( iEntIndex );

		if( ( pVoteHolder == NULL ) || ( pVoter == NULL ) || ( GetVoterTeam( pVoteHolder ) != GetVoterTeam( pVoter ) ) )
		{
			return CAST_FAIL_TEAM_RESTRICTED;
		}
	}

	// Look for a previous vote
	int nOldVote = m_nVotesCast[iEntIndex];
#ifndef DEBUG
	if( nOldVote != VOTE_UNCAST )
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
	m_nVotesCast[iEntIndex] = nCurrentVote;

	// Tell the client-side UI
	IGameEvent *event = gameeventmanager->CreateEvent( "vote_cast" );
	if ( event )
	{
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
	if( m_acceptingVotesTimer.HasStarted() && m_acceptingVotesTimer.IsElapsed() )
	{
		m_acceptingVotesTimer.Invalidate();
		
		int nVoteTally = 0;
		for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
		{
			nVoteTally += m_nVoteOptionCount.Get( index );
		}

		bool bVotePassed = true;

		// for record-keeping
		if ( m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() )
		{
			m_potentialIssues[m_iActiveIssueIndex]->SetYesNoVoteCount( m_nVoteOptionCount[VOTE_OPTION1],  m_nVoteOptionCount[VOTE_OPTION2], m_nPotentialVotes );
		}

		// Have we exceeded the required ratio of Voted-vs-Abstained?
		if ( nVoteTally >= m_nPotentialVotes * sv_vote_quorum_ratio.GetFloat() )
		{
			int nWinningVoteOption = GetWinningVoteOption();
			Assert( nWinningVoteOption >= 0 && nWinningVoteOption < m_VoteOptions.Count() );

			if ( nWinningVoteOption >= 0 && nWinningVoteOption < MAX_VOTE_OPTIONS )
			{
				// YES/NO VOTES
				if ( m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() )
				{
					// Option1 is Yes
					if ( nWinningVoteOption != VOTE_OPTION1 )
					{
						SendVoteFailedMessage( VOTE_FAILED_YES_MUST_EXCEED_NO );
						bVotePassed = false;
					}					
				}
				// GENERAL VOTES:
				// We set the details string after the vote, since that's when
				// we finally have a parameter to pass along and execute
				else if ( nWinningVoteOption < m_VoteOptions.Count() )
				{
					m_potentialIssues[m_iActiveIssueIndex]->SetIssueDetails( m_VoteOptions[nWinningVoteOption] );
				}
			}
		}
		else
		{
			SendVoteFailedMessage( VOTE_FAILED_QUORUM_FAILURE );
			bVotePassed = false;
		}

 		if ( bVotePassed )
		{
			m_executeCommandTimer.Start( sv_vote_command_delay.GetFloat() );
			m_resetVoteTimer.Start( 5.0 );

			UTIL_LogPrintf("Vote succeeded \"%s %s\"\n",
				m_potentialIssues[m_iActiveIssueIndex]->GetTypeString(),
				m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString() );

			CBroadcastRecipientFilter filter;
			filter.MakeReliable();

			UserMessageBegin( filter, "VotePass" );
				WRITE_BYTE( m_iOnlyTeamToVote );
				WRITE_STRING( m_potentialIssues[m_iActiveIssueIndex]->GetVotePassedString() );
				WRITE_STRING( m_potentialIssues[m_iActiveIssueIndex]->GetDetailsString() );
			MessageEnd();
		}
		else
		{
			m_potentialIssues[m_iActiveIssueIndex]->OnVoteFailed( m_iEntityHoldingVote );
			m_resetVoteTimer.Start( 5.0 );
		}
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
	int nVoteTally = 0;
	for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		nVoteTally += m_nVoteOptionCount.Get( index );
	}

	if( nVoteTally >= m_nPotentialVotes )
	{
		m_acceptingVotesTimer.Start( 0 );	// Run the timer out right now
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CVoteController::IsValidVoter( CBasePlayer *pWhom )
{
	if ( pWhom == NULL )
		return false;

	if ( !pWhom->IsConnected() )
		return false;

	if ( !sv_vote_allow_spectators.GetBool() )
	{
		if ( pWhom->GetTeamNumber() == TEAM_SPECTATOR )
			return false;
	}

#ifndef DEBUG  // Don't want to do this check for debug builds (so we can test with bots)
	if ( pWhom->IsBot() )
		return false;

	if ( pWhom->IsFakeClient() )
		return false;
#endif // DEBUG

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
	if( !sv_allow_votes.GetBool() )
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
// Purpose:
//-----------------------------------------------------------------------------
int CVoteController::GetWinningVoteOption( void )
{
	if ( m_potentialIssues[m_iActiveIssueIndex]->IsYesNoVote() )
	{
		return ( m_nVoteOptionCount[VOTE_OPTION1] > m_nVoteOptionCount[VOTE_OPTION2] ) ? VOTE_OPTION1 : VOTE_OPTION2;
	}
	else
	{
		CUtlVector <int> pVoteCounts;

		// Which option had the most votes?
		// driller:  Need to handle ties
		int nHighest = m_nVoteOptionCount[0];
		for ( int iIndex = 0; iIndex < m_nVoteOptionCount.Count(); iIndex ++ )
		{
			nHighest = ( ( nHighest < m_nVoteOptionCount[iIndex] ) ? m_nVoteOptionCount[iIndex] : nHighest );
			pVoteCounts.AddToTail( m_nVoteOptionCount[iIndex] );
		}
		
		m_nHighestCountIndex = -1;
		for ( int iIndex = 0; iIndex < m_nVoteOptionCount.Count(); iIndex++ )
		{
			if ( m_nVoteOptionCount[iIndex] == nHighest )
			{
				m_nHighestCountIndex = iIndex;
				// henryg: break on first match, not last. this avoids a crash
				// if we are all tied at zero and we pick something beyond the
				// last vote option. this code really ought to ignore attempts
				// to tally votes for options beyond the last valid one!
				break;
			}
		}

		return m_nHighestCountIndex;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Store steamIDs for every player that calls a vote
//-----------------------------------------------------------------------------
void CVoteController::TrackVoteCaller( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	CSteamID steamID;
	pPlayer->GetSteamID( &steamID );

	int iIdx = m_VoteCallers.Find( steamID.ConvertToUint64() );
	if ( iIdx != m_VoteCallers.InvalidIndex() )
	{
		// Already being tracked - update timer
		m_VoteCallers[ iIdx ] = gpGlobals->curtime + sv_vote_creation_timer.GetInt();
		return;
	}

	m_VoteCallers.Insert( steamID.ConvertToUint64(), gpGlobals->curtime + sv_vote_creation_timer.GetInt() );
};

//-----------------------------------------------------------------------------
// Purpose: Check the history of steamIDs that called votes and test against a timer
//-----------------------------------------------------------------------------
bool CVoteController::CanEntityCallVote( CBasePlayer *pPlayer, int &nCooldown )
{
	if ( !pPlayer )
		return false;
	
	CSteamID steamID;
	pPlayer->GetSteamID( &steamID );

	// Has this SteamID tried to call a vote recently?
	int iIdx = m_VoteCallers.Find( steamID.ConvertToUint64() );
	if ( iIdx != m_VoteCallers.InvalidIndex() )
	{
		// Timer elapsed?
		nCooldown = (int)( m_VoteCallers[ iIdx ] - gpGlobals->curtime );
		if ( nCooldown > 0 )
			return false;

		// Expired
		m_VoteCallers.Remove( iIdx );
	}

	return true;
};

//-----------------------------------------------------------------------------
// Purpose: BaseIssue
//-----------------------------------------------------------------------------
CBaseIssue::CBaseIssue( const char *pszTypeString )
{
	Q_strcpy( m_szTypeString, pszTypeString );

	m_iNumYesVotes = 0;
	m_iNumNoVotes = 0;
	m_iNumPotentialVotes = 0;

	ASSERT( g_voteController );
	g_voteController->RegisterIssue( this );
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
	Q_strcpy( m_szDetailsString, pszDetails );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseIssue::IsAllyRestrictedVote( void )
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
		Q_strcpy( m_FailedVotes[iIndex]->szFailedVoteParameter, GetDetailsString() );
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
bool CBaseIssue::CanCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	// Automated server vote - don't bother testing against it
	if ( !BRecordVoteFailureEventForEntity( iEntIndex ) )
		return true;

	// Bogus player
	if( iEntIndex == -1 )
		return false;

#ifdef TF_DLL
	if ( TFGameRules() && TFGameRules()->IsInWaitingForPlayers() && !TFGameRules()->IsInTournamentMode() )
	{
		nFailCode = VOTE_FAILED_WAITINGFORPLAYERS;
		return false;
	}
#endif // TF_DLL

	CBaseEntity *pVoteCaller = UTIL_EntityByIndex( iEntIndex );
	if( pVoteCaller && !CanTeamCallVote( GetVoterTeam( pVoteCaller ) ) )
	{
		nFailCode = VOTE_FAILED_TEAM_CANT_CALL;
		return false;
	}

	// Did this fail recently?
	for( int iIndex = 0; iIndex < m_FailedVotes.Count(); iIndex++ )
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
			nFailCode = VOTE_FAILED_FAILED_RECENTLY;	
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

	for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; ++playerIndex )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );
		if( g_voteController->IsValidVoter( pPlayer ) )
		{
			if ( g_voteController->CanTeamCastVote( GetVoterTeam( pPlayer ) ) )
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
void CBaseIssue::SetYesNoVoteCount( int iNumYesVotes, int iNumNoVotes, int iNumPotentialVotes )
{
	m_iNumYesVotes = iNumYesVotes;
	m_iNumNoVotes = iNumNoVotes;
	m_iNumPotentialVotes = iNumPotentialVotes;
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
// Purpose: Game system to detect maps without cameras in them, and move on
//-----------------------------------------------------------------------------
class CVoteControllerSystem : public CAutoGameSystemPerFrame
{
public:
	CVoteControllerSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	virtual void LevelInitPreEntity()
	{
	}

	virtual void FrameUpdatePostEntityThink( void )
	{
		// Executing the vote controller command needs to happen in the PostEntityThink as it can restart levels and
		//	blast entities, etc. If you're doing this during a regular think, this can cause entities thinking after
		//	you in Physics_RunThinkFunctions() to get grumpy and crash.
		if( g_voteController )
		{
			// Vote passed - execute the command
			if( g_voteController->m_executeCommandTimer.HasStarted() && g_voteController->m_executeCommandTimer.IsElapsed() )
			{
				g_voteController->m_executeCommandTimer.Invalidate();
				g_voteController->m_potentialIssues[ g_voteController->m_iActiveIssueIndex ]->ExecuteCommand();
			}
		}
	}
};

CVoteControllerSystem VoteControllerSystem( "CVoteControllerSystem" );

