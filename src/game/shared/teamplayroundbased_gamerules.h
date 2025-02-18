//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teamplay game rules that manage a round based structure for you
//
//=============================================================================

#ifndef TEAMPLAYROUNDBASED_GAMERULES_H
#define TEAMPLAYROUNDBASED_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "teamplay_gamerules.h"
#include "teamplay_round_timer.h"
#include "GameEventListener.h"

#ifdef GAME_DLL
#include "team_control_point.h"
#include "viewport_panel_names.h"
	extern ConVar mp_respawnwavetime;
	extern ConVar mp_showroundtransitions;
	extern ConVar mp_enableroundwaittime;
	extern ConVar mp_showcleanedupents;
	extern ConVar mp_bonusroundtime;
	extern ConVar mp_restartround;
	extern ConVar mp_winlimit;
	extern ConVar mp_maxrounds;
	extern ConVar mp_stalemate_timelimit;
	extern ConVar mp_stalemate_enable;
#else
	#define CTeamplayRoundBasedRules C_TeamplayRoundBasedRules
	#define CTeamplayRoundBasedRulesProxy C_TeamplayRoundBasedRulesProxy
#endif

extern ConVar	tf_arena_use_queue;
extern ConVar	mp_stalemate_meleeonly;
extern ConVar	mp_forceautoteam;

class CTeamplayRoundBasedRules;

//-----------------------------------------------------------------------------
// Round states
//-----------------------------------------------------------------------------
enum gamerules_roundstate_t
{
	// initialize the game, create teams
	GR_STATE_INIT = 0,

	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	GR_STATE_PREGAME,

	//The game is about to start, wait a bit and spawn everyone
	GR_STATE_STARTGAME,

	//All players are respawned, frozen in place
	GR_STATE_PREROUND,

	//Round is on, playing normally
	GR_STATE_RND_RUNNING,

	//Someone has won the round
	GR_STATE_TEAM_WIN,

	//Noone has won, manually restart the game, reset scores
	GR_STATE_RESTART,

	//Noone has won, restart the game
	GR_STATE_STALEMATE,

	//Game is over, showing the scoreboard etc
	GR_STATE_GAME_OVER,

	//Game is in a bonus state, transitioned to after a round ends
	GR_STATE_BONUS,

	//Game is awaiting the next wave/round of a multi round experience
	GR_STATE_BETWEEN_RNDS,

	GR_NUM_ROUND_STATES
};

enum {
	WINREASON_NONE =0,
	WINREASON_ALL_POINTS_CAPTURED,
	WINREASON_OPPONENTS_DEAD,
	WINREASON_FLAG_CAPTURE_LIMIT,
	WINREASON_DEFEND_UNTIL_TIME_LIMIT,
	WINREASON_STALEMATE,
	WINREASON_TIMELIMIT,
	WINREASON_WINLIMIT,
	WINREASON_WINDIFFLIMIT,
#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	WINREASON_RD_REACTOR_CAPTURED,
	WINREASON_RD_CORES_COLLECTED,
	WINREASON_RD_REACTOR_RETURNED,
	WINREASON_PD_POINTS,
	WINREASON_SCORED,
	WINREASON_STOPWATCH_WATCHING_ROUNDS,
	WINREASON_STOPWATCH_WATCHING_FINAL_ROUND,
	WINREASON_STOPWATCH_PLAYING_ROUNDS,
#endif
};

enum stalemate_reasons_t
{
	STALEMATE_JOIN_MID,
	STALEMATE_TIMER,
	STALEMATE_SERVER_TIMELIMIT,

	NUM_STALEMATE_REASONS,
};


#if defined(TF_CLIENT_DLL) || defined(TF_DLL)


#endif

//-----------------------------------------------------------------------------
// Purpose: Per-state data
//-----------------------------------------------------------------------------
class CGameRulesRoundStateInfo
{
public:
	gamerules_roundstate_t	m_iRoundState;
	const char				*m_pStateName;

	void (CTeamplayRoundBasedRules::*pfnEnterState)();	// Init and deinit the state.
	void (CTeamplayRoundBasedRules::*pfnLeaveState)();
	void (CTeamplayRoundBasedRules::*pfnThink)();	// Do a PreThink() in this state.
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTeamplayRoundBasedRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CTeamplayRoundBasedRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
	void	InputSetStalemateOnTimelimit( inputdata_t &inputdata );
#endif

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
	void			OnPreDataChanged( DataUpdateType_t updateType );
	void			OnDataChanged( DataUpdateType_t updateType );
#endif // CLIENT_DLL
};

//-----------------------------------------------------------------------------
// Purpose: Teamplay game rules that manage a round based structure for you
//-----------------------------------------------------------------------------
class CTeamplayRoundBasedRules : public CTeamplayRules, public CGameEventListener
{
	DECLARE_CLASS( CTeamplayRoundBasedRules, CTeamplayRules );
public:
	CTeamplayRoundBasedRules();

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	void SetRoundState( int iRoundState );
#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

	float GetLastRoundStateChangeTime( void ) const { return m_flLastRoundStateChangeTime; }
	float m_flLastRoundStateChangeTime;

	// Data accessors
	inline gamerules_roundstate_t State_Get( void ) { return m_iRoundState; }
	bool	IsInWaitingForPlayers( void ) { return m_bInWaitingForPlayers; }
	virtual bool InRoundRestart( void ) { return State_Get() == GR_STATE_PREROUND; }
	bool	InStalemate( void ) { return State_Get() == GR_STATE_STALEMATE; }
	bool	RoundHasBeenWon( void ) { return State_Get() == GR_STATE_TEAM_WIN; }

	virtual float GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer );
	virtual bool HasPassedMinRespawnTime( CBasePlayer *pPlayer );
	virtual void	LevelInitPostEntity( void );
	virtual float	GetRespawnTimeScalar( int iTeam );
	virtual float	GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers = true );
	virtual bool	ShouldRespawnQuickly( CBasePlayer *pPlayer ) { return false; }
	float	GetMinTimeWhenPlayerMaySpawn( CBasePlayer *pPlayer );

	// Return false if players aren't allowed to cap points at this time (i.e. in WaitingForPlayers)
	virtual bool PointsMayBeCaptured( void ) { return ((State_Get() == GR_STATE_RND_RUNNING || State_Get() == GR_STATE_STALEMATE) && !IsInWaitingForPlayers()); }
	virtual void SetLastCapPointChanged( int iIndex ) { m_iLastCapPointChanged = iIndex; }
	int			 GetLastCapPointChanged( void ) { return m_iLastCapPointChanged; }

	virtual int GetWinningTeam( void )
	{
		return m_iWinningTeam; 
	}
	int GetWinReason() { return m_iWinReason; }

	bool InOvertime( void ){ return m_bInOvertime; }
	void SetOvertime( bool bOvertime );

	bool InSetup( void ){ return m_bInSetup; }

#ifdef GAME_DLL
	virtual void BalanceTeams( bool bRequireSwitcheesToBeDead );
#endif // GAME_DLL

	bool		SwitchedTeamsThisRound( void ) { return m_bSwitchedTeamsThisRound; }

	virtual bool ShouldBalanceTeams( void );
	bool		IsInTournamentMode( void );
	bool		IsInHighlanderMode( void );
	bool		IsInPreMatch( void ) { return (IsInTournamentMode() && IsInWaitingForPlayers()); }
	bool		IsWaitingForTeams( void ) { return m_bAwaitingReadyRestart; }
	bool		IsInStopWatch( void ) { return m_bStopWatch; }
	void		SetInStopWatch( bool bState ) { m_bStopWatch = bState; }
	virtual void	StopWatchModeThink( void ) { };

	bool IsTeamReady( int iTeamNumber )
	{
		if ( iTeamNumber < 0 || iTeamNumber >= MAX_TEAMS_ARRAY_SAFE )
			return false;

		return m_bTeamReady[iTeamNumber];
	}

	bool IsPlayerReady( int iIndex )
	{
		if ( !IsIndexIntoPlayerArrayValid(iIndex) )
			return false;

		return m_bPlayerReady[iIndex];
	}
	
	float GetNextRespawnWave( int iTeamNumber )
	{
		if ( iTeamNumber < 0 || iTeamNumber >= MAX_TEAMS_ARRAY_SAFE )
			return 0.0f;

		return m_flNextRespawnWave[iTeamNumber];
	}

	void SetNextRespawnWave( int iTeamNumber, float flValue )
	{
		if ( iTeamNumber < 0 || iTeamNumber >= MAX_TEAMS_ARRAY_SAFE )
			return;
			
		m_flNextRespawnWave.Set( iTeamNumber, flValue );
	}

	virtual void HandleTeamScoreModify( int iTeam, int iScore) {  };

	float GetRoundRestartTime( void ) const { return m_flRestartRoundTime; }

	//Arena Mode
	virtual bool	IsInArenaMode( void ) const { return false; }

	//Koth Mode
	virtual bool	IsInKothMode( void ) const { return false; }

	//Training Mode
	virtual bool	IsInTraining( void ) { return false; }
	virtual bool	IsInItemTestingMode( void ) { return false; }

	void SetMultipleTrains( bool bMultipleTrains ){ m_bMultipleTrains = bMultipleTrains; }
	bool HasMultipleTrains( void ){ return m_bMultipleTrains; }

	virtual int		GetBonusRoundTime( bool bGameOver = false );
	virtual int		GetPostMatchPeriod( void );
	int				GetRoundsPlayed( void ) { return m_nRoundsPlayed; }

	float GetStateTransitionTime( void ){ return m_flStateTransitionTime; }

#ifdef CLIENT_DLL
	virtual void Update( float frametime ) OVERRIDE;
#endif

	void SetAllowBetweenRounds( bool bValue ) { m_bAllowBetweenRounds = bValue; }

	CTeamRoundTimer *GetActiveRoundTimer( void );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

	//----------------------------------------------------------------------------------
	// Server specific
#ifdef GAME_DLL
	// Derived game rules class should override these
public:
	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool	ShouldCreateEntity( const char *pszClassName );

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart( void ) { return; }

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning( void ) { return; }

	// Called before a new round is started (so the previous round can end)
	virtual void	PreviousRoundEnd( void ) { return; }

	// Send the team scores down to the client
	virtual void	SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	virtual void	SendWinPanelInfo( bool bGameOver ) { return; }

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound( void ) { return; }

	// Called when a round has entered stalemate mode (timer has run out)
	virtual void	SetupOnStalemateStart( void ) { return; }
	virtual void	SetupOnStalemateEnd( void ) { return; }
	virtual void SetSetup( bool bSetup );

	virtual bool	ShouldGoToBonusRound( void ) { return false; }
	virtual void	SetupOnBonusStart( void ) { return; }
	virtual void	SetupOnBonusEnd( void ) { return; }
	virtual void	BonusStateThink( void ) { return; }

	virtual void	BetweenRounds_Start( void ) { return; }
	virtual void	BetweenRounds_End( void ) { return; }
	virtual void	BetweenRounds_Think( void ) { return; }

	virtual void	PreRound_Start( void ) { return; }
	virtual void	PreRound_End( void ) { return; }

	bool PrevRoundWasWaitingForPlayers() { return m_bPrevRoundWasWaitingForPlayers; }

	virtual bool ShouldScorePerRound( void ){ return true; }

	bool CheckNextLevelCvar( bool bAllowEnd = true );

	virtual bool TimerMayExpire( void );

	virtual bool IsValveMap( void ){ return false; }

	virtual		void RestartTournament( void );

	virtual		bool TournamentModeCanEndWithTimelimit( void ){ return true; }

public:
	void State_Transition( gamerules_roundstate_t newState );

	virtual void RespawnPlayers( bool bForceRespawn, bool bTeam = false, int iTeam = TEAM_UNASSIGNED );

	void SetForceMapReset( bool reset );

	void SetRoundToPlayNext( string_t strName ){ m_iszRoundToPlayNext = strName; }
	string_t GetRoundToPlayNext( void ){ return m_iszRoundToPlayNext; }
	void AddPlayedRound( string_t strName );
	bool IsPreviouslyPlayedRound ( string_t strName );
	string_t GetLastPlayedRound( void );

	virtual void SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false ) OVERRIDE;
	virtual void SetStalemate( int iReason, bool bForceMapReset = true, bool bSwitchTeams = false );

	virtual void SetRoundOverlayDetails( void ){ return; }

	void ShouldResetScores( bool bResetTeam, bool bResetPlayer ){ m_bResetTeamScores = bResetTeam; m_bResetPlayerScores = bResetPlayer; }
	void ShouldResetRoundsPlayed( bool bResetRoundsPlayed ){ m_bResetRoundsPlayed = bResetRoundsPlayed; }

	void SetFirstRoundPlayed( string_t strName ){ m_iszFirstRoundPlayed = strName ; }
	string_t GetFirstRoundPlayed(){ return m_iszFirstRoundPlayed; }

	void SetTeamRespawnWaveTime( int iTeam, float flValue );
	void AddTeamRespawnWaveTime( int iTeam, float flValue );
	virtual void FillOutTeamplayRoundWinEvent( IGameEvent *event ) {}	// derived classes may implement to add fields to this event

	void SetStalemateOnTimelimit( bool bStalemate ) { m_bAllowStalemateAtTimelimit = bStalemate; }

	bool IsGameUnderTimeLimit( void );

	void HandleTimeLimitChange( void );

	void SetTeamReadyState( bool bState, int iTeam )
	{
		m_bTeamReady.Set( iTeam, bState );
	}

	void SetPlayerReadyState( int iIndex, bool bState )
	{
		m_bPlayerReady.Set( iIndex, bState );
	}
	void ResetPlayerAndTeamReadyState( void );

	virtual void PlayTrainCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap ){ return; }

	virtual void PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint ){ return; }

	bool PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext );

	virtual void BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags = 0, CBasePlayer *pPlayer = NULL );

	virtual void RecalculateControlPointState( void ){ return; }

	virtual bool ShouldSkipAutoScramble( void ){ return false; }

	virtual bool ShouldWaitToStartRecording( void ){ return IsInWaitingForPlayers(); }

	bool IsGameOver( void ){ return ( CheckTimeLimit( false ) || CheckWinLimit( false ) || CheckMaxRounds( false ) || CheckNextLevelCvar( false ) ); }

	virtual bool	StopWatchShouldBeTimedWin( void ) { return m_bStopWatchShouldBeTimedWin; }

protected:
	virtual void Think( void );

	virtual void CheckChatText( CBasePlayer *pPlayer, char *pText );
	void		 CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg );

	// Game beginning / end handling
	virtual void GoToIntermission( void );
	void		 SetInWaitingForPlayers( bool bWaitingForPlayers );
	void		 CheckWaitingForPlayers( void );
	virtual bool AllowWaitingForPlayers( void ) { return true; }
	void		 CheckRestartRound( void );
	bool		 CheckTimeLimit( bool bAllowEnd = true );
	int			 GetTimeLeft( void );
	virtual	bool CheckWinLimit( bool bAllowEnd = true, int nAddValueWhenChecking = 0 );
	bool		 CheckMaxRounds( bool bAllowEnd = true, int nAddValueWhenChecking = 0 );

	void		 CheckReadyRestart( void );

	virtual bool CanChangelevelBecauseOfTimeLimit( void ) { return true; }
	virtual bool CanGoToStalemate( void ) { return true; }

	// State machine handling
	void State_Enter( gamerules_roundstate_t newState );	// Initialize the new state.
	void State_Leave();										// Cleanup the previous state.
	void State_Think();										// Update the current state.
	static CGameRulesRoundStateInfo* State_LookupInfo( gamerules_roundstate_t state );	// Find the state info for the specified state.

	// State Functions
	void State_Enter_INIT( void );
	void State_Think_INIT( void );

	void State_Enter_PREGAME( void );
	void State_Think_PREGAME( void );

	void State_Enter_STARTGAME( void );
	void State_Think_STARTGAME( void );

	void State_Enter_PREROUND( void );
	void State_Leave_PREROUND( void );
	void State_Think_PREROUND( void );

	void State_Enter_RND_RUNNING( void );
	void State_Think_RND_RUNNING( void );

	void State_Enter_TEAM_WIN( void );
	void State_Think_TEAM_WIN( void );

	void State_Enter_RESTART( void );
	void State_Think_RESTART( void );

	void State_Enter_STALEMATE( void );
	void State_Think_STALEMATE( void );
	void State_Leave_STALEMATE( void );

	void State_Enter_BONUS( void );
	void State_Think_BONUS( void );
	void State_Leave_BONUS( void );

	void State_Enter_BETWEEN_RNDS( void );
	void State_Leave_BETWEEN_RNDS( void );
	void State_Think_BETWEEN_RNDS( void );

	// mp_scrambleteams_auto
	void ResetTeamsRoundWinTracking( void );

protected:
	virtual void InitTeams( void );
	virtual bool BHavePlayers( void );

	virtual void RoundRespawn( void );
	virtual void CleanUpMap( void );
	virtual void CheckRespawnWaves( void );
	void ResetScores( void );
	void ResetMapTime( void );

	void PlayStartRoundVoice( void );
	virtual void PlayWinSong( int team );
	void PlayStalemateSong( void );
	void PlaySuddenDeathSong( void );

	virtual const char* GetStalemateSong( int nTeam ) { return "Game.Stalemate"; }
	virtual const char* WinSongName( int nTeam ) { return "Game.YourTeamWon"; }
	virtual const char* LoseSongName( int nTeam ) { return "Game.YourTeamLost"; }
	
	virtual void RespawnTeam( int iTeam ) { RespawnPlayers( false, true, iTeam ); }

	void HideActiveTimer( void );
	virtual void RestoreActiveTimer( void );

	virtual void InternalHandleTeamWin( int iWinningTeam ){ return; }

	bool MapHasActiveTimer( void );
	void CreateTimeLimitTimer( void );

	virtual float GetLastMajorEventTime( void ) OVERRIDE { return m_flLastTeamWin; }

protected:
	CGameRulesRoundStateInfo	*m_pCurStateInfo;			// Per-state data 

	float						m_flWaitingForPlayersTimeEnds;
	CHandle<CTeamRoundTimer>	m_hWaitingForPlayersTimer;

	float						m_flNextPeriodicThink;
	bool						m_bChangeLevelOnRoundEnd;

	bool						m_bResetTeamScores;
	bool						m_bResetPlayerScores;
	bool						m_bResetRoundsPlayed;

	// Stalemate
	EHANDLE						m_hPreviousActiveTimer;
	CHandle<CTeamRoundTimer>	m_hStalemateTimer;
	float						m_flStalemateStartTime;

	CHandle<CTeamRoundTimer>	m_hTimeLimitTimer;

	bool						m_bForceMapReset; // should the map be reset when a team wins and the round is restarted?
	bool						m_bPrevRoundWasWaitingForPlayers;	// was the previous map reset after a waiting for players period
	bool						m_bInitialSpawn;

	string_t					m_iszRoundToPlayNext;
	CUtlVector<string_t>		m_iszPreviousRounds; // we'll store the two previous rounds so we won't play them again right away if there are other rounds that can be played first
	string_t					m_iszFirstRoundPlayed; // store the first round played after a full restart so we can pick a different one next time if we have other options

	float						m_flOriginalTeamRespawnWaveTime[ MAX_TEAMS_ARRAY_SAFE ];

	bool						m_bAllowStalemateAtTimelimit;
	bool						m_bChangelevelAfterStalemate;

	float						m_flRoundStartTime;		// time the current round started
	float						m_flNewThrottledAlertTime;		// time that we can play another throttled alert

	bool						m_bUseAddScoreAnim;

	gamerules_roundstate_t		m_prevState;

	bool						m_bPlayerReadyBefore[MAX_PLAYERS_ARRAY_SAFE];	// Test to see if a player has hit ready before

	float						m_flLastTeamWin;

	bool						m_bStopWatchShouldBeTimedWin;

private:

	CUtlMap < int, int >	m_GameTeams;  // Team index, Score
#endif
	// End server specific
	//----------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
public:
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	HandleOvertimeBegin(){}
	virtual void	GetTeamGlowColor( int nTeam, float &r, float &g, float &b ){ r = 0.76f; g = 0.76f; b = 0.76f; }

private:
	bool			m_bOldInWaitingForPlayers;
	bool			m_bOldInOvertime;
	bool			m_bOldInSetup;
#endif // CLIENT_DLL

public:
	bool WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam  );
	bool AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam );
	virtual bool HaveCheatsBeenEnabledDuringLevel( void ) { return m_bCheatsEnabledDuringLevel; }

	float GetPreroundCountdownTime( void ){ return m_flCountdownTime; }

protected:
	CNetworkVar( gamerules_roundstate_t, m_iRoundState );
	CNetworkVar( bool, m_bInOvertime ); // Are we currently in overtime?
	CNetworkVar( bool, m_bInSetup ); // Are we currently in setup?
	CNetworkVar( bool, m_bSwitchedTeamsThisRound );

protected:
	CNetworkVar( int,			m_iWinningTeam );				// Set before entering GR_STATE_TEAM_WIN
	CNetworkVar( int,			m_iWinReason );
	CNetworkVar( bool,			m_bInWaitingForPlayers );
	CNetworkVar( bool,			m_bAwaitingReadyRestart );
	CNetworkVar( float,			m_flRestartRoundTime );
	CNetworkVar( float,			m_flMapResetTime );						// Time that the map was reset
	CNetworkArray( float,		m_flNextRespawnWave, MAX_TEAMS_ARRAY_SAFE );		// Minor waste, but cleaner code
	CNetworkArray( bool,		m_bTeamReady, MAX_TEAMS_ARRAY_SAFE );
	CNetworkVar( bool,			m_bStopWatch );
	CNetworkVar( bool,			m_bMultipleTrains ); // two trains in this map?
	CNetworkArray( bool,		m_bPlayerReady, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkVar( bool,			m_bCheatsEnabledDuringLevel );
	CNetworkVar( int, 			m_nRoundsPlayed );
	CNetworkVar( float,			m_flCountdownTime );
	CNetworkVar( float,			m_flStateTransitionTime );	// Timer for round states
public:
	CNetworkArray( float,		m_TeamRespawnWaveTimes, MAX_TEAMS_ARRAY_SAFE );	// Time between each team's respawn wave

private:
	float m_flStartBalancingTeamsAt;
	float m_flNextBalanceTeamsTime;
	bool m_bPrintedUnbalanceWarning;
	float m_flFoundUnbalancedTeamsTime;

	float	m_flAutoBalanceQueueTimeEnd;
	int		m_nAutoBalanceQueuePlayerIndex;
	int		m_nAutoBalanceQueuePlayerScore;

	int		m_nLastEventFiredTime;
protected:
	bool	m_bAllowBetweenRounds;

public:

	float	m_flStopWatchTotalTime;
	int		m_iLastCapPointChanged;
};

// Utility function
bool FindInList( const char **pStrings, const char *pToFind );

inline CTeamplayRoundBasedRules* TeamplayRoundBasedRules()
{
	return static_cast<CTeamplayRoundBasedRules*>(g_pGameRules);
}

#endif // TEAMPLAYROUNDBASED_GAMERULES_H
