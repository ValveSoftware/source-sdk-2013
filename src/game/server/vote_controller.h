//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Player-driven Voting System for Multiplayer Source games (currently implemented for TF2)
//
// $NoKeywords: $
//=============================================================================//

#ifndef VOTE_CONTROLLER_H
#define VOTE_CONTROLLER_H

#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

#define MAX_COMMAND_LENGTH 64
#define MAX_CREATE_ERROR_STRING 96

struct VoteParams_t
{
	VoteParams_t()
	{
		Reset();
	}

	int m_iIssueIndex;
	int m_iEntIndex;
	char m_szTypeString[MAX_COMMAND_LENGTH];
	char m_szDetailString[MAX_VOTE_DETAILS_LENGTH];

	void Reset( void )
	{
		m_iIssueIndex = INVALID_ISSUE;
		m_iEntIndex = -1;
		m_szTypeString[0] = 0;
		m_szDetailString[0] = 0;
	}
};

class CVoteController;

class CBaseIssue	// Base class concept for vote issues (i.e. Kick Player).  Created per level-load and destroyed by CVoteController's dtor.
{
public:
	CBaseIssue( const char *typeString, CVoteController *pVoteController );
	virtual				 ~CBaseIssue();
	const char			*GetTypeString( void );						// Connection between console command and specific type of issue
	virtual const char	*GetTypeStringLocalized( void ) { return ""; }	// When empty, the client uses the classname string and prepends "#Vote_"
	virtual const char	*GetDetailsString( void );
	virtual void		SetIssueDetails( const char *pszDetails );	// We need to know the details part of the con command for later
	virtual void		OnVoteFailed( int iEntityHoldingVote );		// The moment the vote fails, also has some time for feedback before the window goes away
	virtual void		OnVoteStarted( void ) {}					// Called as soon as the vote starts
	virtual bool		IsEnabled( void ) { return false; }			// Query the issue to see if it's enabled
	virtual bool		CanTeamCallVote( int iTeam ) const;			// Can someone on the given team call this vote?
	virtual bool		RequestCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime ); // Can this guy hold a vote on this issue?
	virtual bool		IsTeamRestrictedVote( void );				// Restrict access and visibility of this vote to a specific team?
	virtual const char *GetDisplayString( void ) = 0;				// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void ) = 0;					// Where the magic happens.  Do your thing.
	virtual void		ListIssueDetails( CBasePlayer *pForWhom ) = 0;	// Someone would like to know all your valid details
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display when it passes.
	virtual int			CountPotentialVoters( void );
	virtual int			GetNumberVoteOptions( void );				// How many choices this vote will have.  i.e. Returns 2 on a Yes/No issue (the default).
	virtual bool		IsYesNoVote( void );
	virtual bool		GetVoteOptions( CUtlVector <const char*> &vecNames );	// We use this to generate options for voting
	virtual bool		BRecordVoteFailureEventForEntity( int iVoteCallingEntityIndex ) const { return iVoteCallingEntityIndex != DEDICATED_SERVER; }
	void				SetIssueCooldownDuration( float flDuration ) { m_flNextCallTime = gpGlobals->curtime + flDuration; }	// The issue can not be raised again for this period of time (in seconds)
	virtual float		GetQuorumRatio( void );						// Each issue can decide the required ratio of voted-vs-abstained
	// After a vote finishes, this is called to determine the next step.  The default issue uses GetQuorumRatio to
	// return a pass/fail, but issues may do more complex actions here.  If Wait is returned, ProcessResults will be
	// called again shortly, to allow issues to send GC messages or take other actions prior to deciding.
	enum EVoteAction {
		eVoteAction_Wait, // Call again, for asynchronous processing
		eVoteAction_Pass, // Consider vote passed
		eVoteAction_Fail  // Consdier vote failed
	};
	virtual EVoteAction ProcessResults( const CUtlVector <const char*> &vecOptions, const int arVoteCountByOption[],
	                                    const CUtlMap<CSteamID, int> &mapVotesBySteamID, int nHighestCountOption,
	                                    int nTotalVotes, int nPotentialVoters );
	virtual void		OnVoteEnded( void );

	virtual void		OnPlayerDisconnected( CBasePlayer *pPlayer ) {}

	CHandle< CBasePlayer > m_hPlayerTarget;							// If the target of the issue is a player, we should store them here

protected:
	static void			ListStandardNoArgCommand( CBasePlayer *forWhom, const char *issueString );		// List a Yes vote command

	struct FailedVote
	{
		char	szFailedVoteParameter[MAX_VOTE_DETAILS_LENGTH];
		float	flLockoutTime;
	};

	CUtlVector< FailedVote* > m_FailedVotes;
	char m_szTypeString[MAX_COMMAND_LENGTH];
	char m_szDetailsString[MAX_VOTE_DETAILS_LENGTH];
	int m_iNumYesVotes;
	int m_iNumNoVotes;
	int m_iNumPotentialVotes;
	float m_flNextCallTime;

	CVoteController *m_pVoteController;
};

class CVoteController : public CBaseEntity
{
	DECLARE_CLASS( CVoteController, CBaseEntity );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CVoteController();
	virtual ~CVoteController();

	enum TryCastVoteResult
	{
		CAST_OK,
		CAST_FAIL_SERVER_DISABLE,
		CAST_FAIL_NO_ACTIVE_ISSUE,
		CAST_FAIL_TEAM_RESTRICTED,
		CAST_FAIL_NO_CHANGES,
		CAST_FAIL_DUPLICATE,
		CAST_FAIL_VOTE_CLOSED,
		CAST_FAIL_SYSTEM_ERROR
	};

	virtual void	Spawn( void );
	virtual int		UpdateTransmitState( void );

	static bool		IsVoteSystemEnabled( void );

	static bool		SetupVote( int iEntIndex );					// This creates a list of issues for the UI
	bool			CreateVote( int iEntIndex, const char *pszTypeString, const char *pszDetailString );	// This is what the UI passes in
	TryCastVoteResult TryCastVote( int iEntIndex, const char *pszVoteString );
	void			RegisterIssue( CBaseIssue *pNewIssue );
	void			ListIssues( CBasePlayer *pForWhom );
	bool			IsValidVoter( CBasePlayer *pWhom );
	bool			CanTeamCastVote( int iTeam ) const;
	void			SendVoteCreationFailedMessage( vote_create_failed_t nReason, CBasePlayer *pVoteCaller, int nTime = -1 );
	void			SendVoteFailedToPassMessage( vote_create_failed_t nReason );
	void			VoteChoice_Increment( int nVoteChoice );
	void			VoteChoice_Decrement( int nVoteChoice );
	int				GetVoteIssueIndexWithHighestCount( void );
	void			TrackVoteCaller( CBasePlayer *pPlayer, float flTime );
	bool			CanEntityCallVote( CBasePlayer *pPlayer, int &nCooldown, vote_create_failed_t &nErrorCode );
	bool			IsVoteActive( void ) { return ( m_iActiveIssueIndex != INVALID_ISSUE ); }
	int				GetNumVotesCast( void );

	static void			AddPlayerToKickWatchList( CSteamID steamID, float flDuration );		// Band-aid until we figure out how player's avoid kick votes
	static void			AddPlayerToNameLockedList( CSteamID steamID, float flDuration, int nUserID );

	CBaseIssue		*GetCurrentVote();
	bool			HasPlayerVotedOnCurrentIssue( CSteamID steamID );
	void			RemovePlayerVote( CSteamID steamID );

	void			OnPlayerDisconnected( CBasePlayer *pPlayer );

	bool			HasIssue( const char *pszIssue );
	bool			IsAVoteInProgress( void ) { return ( m_iActiveIssueIndex != INVALID_ISSUE ); }
	int				GetVoteID() const { return m_nVoteIdx; }

protected:
	void			ResetData( void );
	void			VoteControllerThink( void );
	void			CheckForEarlyVoteClose( void );				// If everyone has voted (and changing votes is not allowed) then end early

	CNetworkVar( int, m_iActiveIssueIndex );					// Type of thing being voted on
	CNetworkVar( int, m_iOnlyTeamToVote );						// If an Ally restricted vote, the team number that is allowed to vote
	CNetworkArray( int, m_nVoteOptionCount, MAX_VOTE_OPTIONS );	// Vote options counter
	CNetworkVar( int, m_nPotentialVotes );						// How many votes could come in, so we can close ballot early
	CNetworkVar( bool, m_bIsYesNoVote );						// Is the current issue Yes/No?
	CountdownTimer	m_acceptingVotesTimer;						// How long from vote start until we count the ballots
	CountdownTimer	m_executeCommandTimer;						// How long after end of vote time until we execute a passed vote
	CountdownTimer	m_resetVoteTimer;							// when the current vote will end
	int				m_iEntityHoldingVote;
	CNetworkVar( int, m_nVoteIdx );

	CUtlMap<CSteamID, int>		m_mapVotesBySteamID;			// Votes cast by steamid
	CUtlVector <CBaseIssue *>	m_potentialIssues;
	CUtlVector <const char *>	m_VoteOptions;
	CUtlMap <uint64, float>		m_VoteCallers;					// History of SteamIDs that have tried to call votes.

	friend class CVoteControllerSystem;
};

extern CVoteController *g_voteControllerGlobal;
extern CVoteController *g_voteControllerRed;
extern CVoteController *g_voteControllerBlu;

#endif // VOTE_CONTROLLER_H
