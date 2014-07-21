//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CS's custom C_VoteController
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_vote_controller.h"
#include "shareddefs.h"
#include "hud.h"
#include "cdll_client_int.h"
#include "igameevents.h"
#include "hud_vote.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_VoteController, DT_VoteController, CVoteController )
	RecvPropInt( RECVINFO( m_iActiveIssueIndex ), 0, C_VoteController::RecvProxy_VoteType ),
	RecvPropInt( RECVINFO( m_iOnlyTeamToVote ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nVoteOptionCount ), RecvPropInt( RECVINFO( m_nVoteOptionCount[0] ), 0, C_VoteController::RecvProxy_VoteOption ) ),
	RecvPropInt( RECVINFO( m_nPotentialVotes ) ),
	RecvPropBool( RECVINFO( m_bIsYesNoVote ) )
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::RecvProxy_VoteType( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_VoteController *pMe = (C_VoteController *)pStruct;
	if( pMe->m_iActiveIssueIndex == pData->m_Value.m_Int )
		return;

	pMe->m_iActiveIssueIndex = pData->m_Value.m_Int;
	pMe->m_bTypeDirty = true;

	// Since the contents of a new vote are in three parts, we can't directly send an event to the Hud
	// because we don't really know if we have all three parts yet.  So we'll mark dirty, and our think
	// can notice that and send the event.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::RecvProxy_VoteOption( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int index = pData->m_pRecvProp->GetOffset() / sizeof(int);
	
	size_t offset = offsetof( C_VoteController, m_nVoteOptionCount );
	C_VoteController *pMe = (C_VoteController *)((byte *)pStruct - offset );
	if( pMe->m_nVoteOptionCount[index] == pData->m_Value.m_Int )
		return;
	
	pMe->m_nVoteOptionCount[index] = pData->m_Value.m_Int;
	pMe->m_bVotesDirty = true;
	pMe->SetNextClientThink( gpGlobals->curtime + 0.001 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
C_VoteController::C_VoteController()
{
	ResetData();

	ListenForGameEvent( "vote_cast" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
C_VoteController::~C_VoteController()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::ResetData()
{
	m_iActiveIssueIndex = INVALID_ISSUE;
	m_iOnlyTeamToVote = TEAM_INVALID;
	for( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		m_nVoteOptionCount[index] = 0;
	}
	m_nPotentialVotes = 0;
	m_bVotesDirty = false;
	m_bTypeDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::Spawn( void )
{
	ResetData();
	BaseClass::Spawn();
	SetNextClientThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::ClientThink()
{
	BaseClass::ClientThink();

	if( m_bTypeDirty )
	{
		m_bTypeDirty = false;
		m_bVotesDirty = true;
	}
	
	if( m_bVotesDirty )
	{
		if ( m_nPotentialVotes > 0 )
		{
			// Currently hard-coded to MAX_VOTE_COUNT options per issue
			DevMsg( "Votes: Option1 - %d, Option2 - %d, Option3 - %d, Option4 - %d, Option5 - %d\n",
				m_nVoteOptionCount[0], m_nVoteOptionCount[1], m_nVoteOptionCount[2], m_nVoteOptionCount[3], m_nVoteOptionCount[4] );

			IGameEvent *event = gameeventmanager->CreateEvent( "vote_changed" );
			if ( event )
			{
				for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
				{	
					char szOption[2];
					Q_snprintf( szOption, sizeof( szOption ), "%i", index + 1 );

					char szVoteOption[13] = "vote_option";
					Q_strncat( szVoteOption, szOption, sizeof( szVoteOption ), COPY_ALL_CHARACTERS );

					event->SetInt( szVoteOption, m_nVoteOptionCount[index] );
				}
				event->SetInt( "potentialVotes", m_nPotentialVotes );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		m_bVotesDirty = false;
	}

	SetNextClientThink( gpGlobals->curtime + 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_VoteController::FireGameEvent( IGameEvent *event )
{
	CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
	if ( pHudVote && pHudVote->IsVisible() )
	{
		const char *eventName = event->GetName();
		if ( !eventName )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		int team = event->GetInt( "team", TEAM_UNASSIGNED );
		if ( team > TEAM_UNASSIGNED && team != pLocalPlayer->GetTeamNumber() )
			return;

		if ( FStrEq( eventName, "vote_cast" ) )
		{
			if ( m_bIsYesNoVote )
			{
				int vote_option = event->GetInt( "vote_option", TEAM_UNASSIGNED );
				if( vote_option == VOTE_OPTION2 )
				{
					pLocalPlayer->EmitSound( "Vote.Cast.No" );
				}
				else if( vote_option == VOTE_OPTION1 )
				{
					pLocalPlayer->EmitSound( "Vote.Cast.Yes" );
				}
			}
			else
			{
				pLocalPlayer->EmitSound( "Vote.Cast.Yes" );
			}
		}
	}
}