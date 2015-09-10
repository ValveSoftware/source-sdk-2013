//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Round timer for team gamerules
//
//=============================================================================//

#ifndef TEAM_ROUND_TIMER_H
#define TEAM_ROUND_TIMER_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTeamRoundTimer C_TeamRoundTimer
#endif

class CTeamRoundTimer : public CBaseEntity
{
public:
	DECLARE_CLASS( CTeamRoundTimer, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CTeamRoundTimer();
	virtual ~CTeamRoundTimer();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );

	// Returns seconds to display.
	// When paused shows amount of time left once the timer is resumed
	virtual float GetTimeRemaining( void );
	virtual int GetTimerMaxLength( void );
	virtual bool ShowInHud( void );
	virtual bool StartPaused( void ){ return m_bStartPaused; }
	bool ShowTimeRemaining( void ) { return m_bShowTimeRemaining; }

	bool IsDisabled( void ) { return m_bIsDisabled; }
	int GetTimerState( void ){ return m_nState; }

	bool IsTimerPaused( void ) { return m_bTimerPaused; }
	
#ifdef CLIENT_DLL

	void InternalSetPaused( bool bPaused ) { m_bTimerPaused = bPaused; }

#else

	void	SetStopWatchTimeStamp( void );
	virtual void SetTimeRemaining( int iTimerSeconds ); // Set the initial length of the timer
	virtual void AddTimerSeconds( int iSecondsToAdd, int iTeamResponsible = TEAM_UNASSIGNED ); // Add time to an already running ( or paused ) timer
	virtual void PauseTimer( void );
	virtual void ResumeTimer( void );
	virtual void SetAutoCountdown( bool bAuto ){ m_bAutoCountdown = bAuto; }

	void		 SetShowInHud( bool bShowInHUD ) { m_bShowInHUD = bShowInHUD; }

	int UpdateTransmitState();

	void InputEnable( inputdata_t &input );
	void InputDisable( inputdata_t &input );
	void InputPause( inputdata_t &input );
	void InputResume( inputdata_t &input );
	void InputSetTime( inputdata_t &input );
	void InputAddTime( inputdata_t &input );
	void InputRestart( inputdata_t &input );
	void InputShowInHUD( inputdata_t &input );
	void InputRoundSpawn( inputdata_t &inputdata );
	void InputSetMaxTime( inputdata_t &input );
	void InputAutoCountdown( inputdata_t &input );
	void InputAddTeamTime( inputdata_t &input );
	void InputSetSetupTime( inputdata_t &input );

#endif

	void SetCaptureWatchState( bool bCaptureWatch );
	bool IsWatchingTimeStamps( void ) { return m_bInCaptureWatchState; }
	void SetStopWatch( bool bState ) { m_bStopWatchTimer = bState; }
	bool IsStopWatchTimer( void ) { return m_bStopWatchTimer; }
	float GetStopWatchTotalTime( void ) { return m_flTotalTime; }
	bool IsRoundMaxTimerSet( void ) { return m_nTimerMaxLength > 0; }
	int GetTimerInitialLength( void ) { return m_nTimerInitialLength; }

private:
	void CalculateOutputMessages( void );

#ifdef CLIENT_DLL
	virtual void ClientThink();
	void OnPreDataChanged( DataUpdateType_t updateType );
	void OnDataChanged( DataUpdateType_t updateType );
	void SendTimeWarning( int nWarning );
	const char *GetTimeWarningSound( int nWarning );

#else
	void SetState( int nState, bool bFireOutput = true );
	void SetTimerThink( int nType );
	void EXPORT RoundTimerThink( void );
	void EXPORT RoundTimerSetupThink( void );

	static void SetActiveTimer( CTeamRoundTimer *pNewlyActive );
#endif

private:
	CNetworkVar( bool, m_bTimerPaused );
	CNetworkVar( float, m_flTimeRemaining );
	CNetworkVar( float, m_flTimerEndTime );	
	CNetworkVar( bool, m_bIsDisabled );
	CNetworkVar( bool, m_bShowInHUD );
	CNetworkVar( int, m_nTimerLength );			// current timer's length (used in the timer panel if no max length is set)
	CNetworkVar( int, m_nTimerInitialLength );	// initial length of the timer
	CNetworkVar( int, m_nTimerMaxLength );		// max time the timer can have (0 is no max)
	CNetworkVar( bool, m_bAutoCountdown );		// automatically count down the end of a round
	CNetworkVar( int, m_nSetupTimeLength );		// current timer's setup time length (setup time is the time before the round begins)
	CNetworkVar( int, m_nState );				// RT_STATE_SETUP or RT_STATE_NORMAL
	CNetworkVar( bool, m_bStartPaused );		// start the timer paused when it spawns
	CNetworkVar( bool, m_bShowTimeRemaining );  //show how much time is left (default) instead of how much time has passed.
	CNetworkVar( bool, m_bInCaptureWatchState );
	CNetworkVar( float, m_flTotalTime );
	CNetworkVar( bool, m_bStopWatchTimer );

	bool			m_bFireFinished;
	bool			m_bFire5MinRemain;
	bool			m_bFire4MinRemain;
	bool			m_bFire3MinRemain;
	bool			m_bFire2MinRemain;
	bool			m_bFire1MinRemain;
	bool			m_bFire30SecRemain;
	bool			m_bFire10SecRemain;
	bool			m_bFire5SecRemain;
	bool			m_bFire4SecRemain;
	bool			m_bFire3SecRemain;
	bool			m_bFire2SecRemain;
	bool			m_bFire1SecRemain;

#ifdef CLIENT_DLL 

	int				m_nOldTimerLength;
	int				m_nOldTimerState;

#else
	COutputEvent	m_OnRoundStart;
	COutputEvent	m_OnFinished;
	COutputEvent	m_On5MinRemain;
	COutputEvent	m_On4MinRemain;
	COutputEvent	m_On3MinRemain;
	COutputEvent	m_On2MinRemain;
	COutputEvent	m_On1MinRemain;
	COutputEvent	m_On30SecRemain;
	COutputEvent	m_On10SecRemain;
	COutputEvent	m_On5SecRemain;
	COutputEvent	m_On4SecRemain;
	COutputEvent	m_On3SecRemain;
	COutputEvent	m_On2SecRemain;
	COutputEvent	m_On1SecRemain;

	COutputEvent	m_OnSetupStart;
	COutputEvent	m_OnSetupFinished;

	float			m_flNextOvertimeNag;
	float			m_flLastTime;

	DECLARE_DATADESC();

	bool			m_bPauseDueToWin;
	bool			m_bResetTimeOnRoundStart;
	int				m_nTimeToUseAfterSetupFinished;
#endif 
};

#ifdef CLIENT_DLL
extern CTeamRoundTimer *g_TeamRoundTimer;
#endif

#endif	//TEAM_ROUND_TIMER_H