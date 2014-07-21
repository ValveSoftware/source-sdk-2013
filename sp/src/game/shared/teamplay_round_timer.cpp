//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team gamerules round timer 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "teamplay_round_timer.h"
#include "teamplayroundbased_gamerules.h"

#ifdef CLIENT_DLL
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "c_playerresource.h"
#include "c_team_objectiveresource.h"
#if defined( TF_CLIENT_DLL )
#include "tf_gamerules.h"
#include "c_tf_player.h"
#endif // TF_CLIENT_DLL
#else
#include "team.h"
#include "team_objectiveresource.h"
#if defined( TF_DLL )
#include "tf_player.h"
#endif // TF_DLL
#endif

#define ROUND_TIMER_60SECS	"Announcer.RoundEnds60seconds"
#define ROUND_TIMER_30SECS	"Announcer.RoundEnds30seconds"
#define ROUND_TIMER_10SECS	"Announcer.RoundEnds10seconds"
#define ROUND_TIMER_5SECS	"Announcer.RoundEnds5seconds"
#define ROUND_TIMER_4SECS	"Announcer.RoundEnds4seconds"
#define ROUND_TIMER_3SECS	"Announcer.RoundEnds3seconds"
#define ROUND_TIMER_2SECS	"Announcer.RoundEnds2seconds"
#define ROUND_TIMER_1SECS	"Announcer.RoundEnds1seconds"

#define ROUND_SETUP_60SECS	"Announcer.RoundBegins60Seconds"
#define ROUND_SETUP_30SECS	"Announcer.RoundBegins30Seconds"
#define ROUND_SETUP_10SECS	"Announcer.RoundBegins10Seconds"
#define ROUND_SETUP_5SECS	"Announcer.RoundBegins5Seconds"
#define ROUND_SETUP_4SECS	"Announcer.RoundBegins4Seconds"
#define ROUND_SETUP_3SECS	"Announcer.RoundBegins3Seconds"
#define ROUND_SETUP_2SECS	"Announcer.RoundBegins2Seconds"
#define ROUND_SETUP_1SECS	"Announcer.RoundBegins1Seconds"

#define ROUND_START_BELL	"Ambient.Siren"

#define ROUND_TIMER_TIME_ADDED			"Announcer.TimeAdded"
#define ROUND_TIMER_TIME_ADDED_LOSER	"Announcer.TimeAddedForEnemy"
#define ROUND_TIMER_TIME_ADDED_WINNER	"Announcer.TimeAwardedForTeam"

enum
{
	RT_THINK_SETUP,
	RT_THINK_NORMAL,	
};

enum
{
	RT_WARNING_60SECS,
	RT_WARNING_30SECS,
	RT_WARNING_10SECS,
	RT_WARNING_5SECS,
	RT_WARNING_4SECS,
	RT_WARNING_3SECS,
	RT_WARNING_2SECS,
	RT_WARNING_1SECS,
	RT_WARNING_TIME_START,
};

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool IsInCommentaryMode();

#if defined( GAME_DLL ) && defined( TF_DLL )
ConVar tf_overtime_nag( "tf_overtime_nag", "0", FCVAR_NOTIFY, "Announcer overtime nag." );
#endif

#ifdef CLIENT_DLL

// Use this proxy to flash the round timer whenever the timer is restarted
// because trapping the round start event doesn't work ( the event also flushes
// all hud events and obliterates our TimerFlash event )
static void RecvProxy_TimerPaused( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTeamRoundTimer *pTimer = (CTeamRoundTimer *) pStruct;

	bool bTimerPaused = ( pData->m_Value.m_Int > 0 );

	if ( bTimerPaused == false )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "TimerFlash" ); 
	}

	if ( pTimer )
	{
		pTimer->InternalSetPaused( bTimerPaused );
	}
}

#endif

LINK_ENTITY_TO_CLASS( team_round_timer, CTeamRoundTimer );
PRECACHE_REGISTER( team_round_timer );

IMPLEMENT_NETWORKCLASS_ALIASED( TeamRoundTimer, DT_TeamRoundTimer )

BEGIN_NETWORK_TABLE_NOBASE( CTeamRoundTimer, DT_TeamRoundTimer )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_bTimerPaused ), 0, RecvProxy_TimerPaused ),
	RecvPropTime( RECVINFO( m_flTimeRemaining ) ),
	RecvPropTime( RECVINFO( m_flTimerEndTime ) ),
	RecvPropInt( RECVINFO( m_nTimerMaxLength ) ),
	RecvPropBool( RECVINFO( m_bIsDisabled ) ),
	RecvPropBool( RECVINFO( m_bShowInHUD ) ),
	RecvPropInt( RECVINFO( m_nTimerLength ) ),
	RecvPropInt( RECVINFO( m_nTimerInitialLength ) ),
	RecvPropBool( RECVINFO( m_bAutoCountdown ) ),
	RecvPropInt( RECVINFO( m_nSetupTimeLength ) ),
	RecvPropInt( RECVINFO( m_nState ) ),
	RecvPropBool( RECVINFO( m_bStartPaused ) ),
	RecvPropBool( RECVINFO( m_bShowTimeRemaining ) ),
	RecvPropBool( RECVINFO( m_bInCaptureWatchState ) ),
	RecvPropBool( RECVINFO( m_bStopWatchTimer ) ),
	RecvPropTime( RECVINFO( m_flTotalTime ) ),

#else

	SendPropBool( SENDINFO( m_bTimerPaused ) ),
	SendPropTime( SENDINFO( m_flTimeRemaining ) ),
	SendPropTime( SENDINFO( m_flTimerEndTime ) ),
	SendPropInt( SENDINFO( m_nTimerMaxLength ) ),
	SendPropBool( SENDINFO( m_bIsDisabled ) ),
	SendPropBool( SENDINFO( m_bShowInHUD ) ),
	SendPropInt( SENDINFO( m_nTimerLength ) ),
	SendPropInt( SENDINFO( m_nTimerInitialLength ) ),
	SendPropBool( SENDINFO( m_bAutoCountdown ) ),
	SendPropInt( SENDINFO( m_nSetupTimeLength ) ),
	SendPropInt( SENDINFO( m_nState ) ),
	SendPropBool( SENDINFO( m_bStartPaused ) ),
	SendPropBool( SENDINFO( m_bShowTimeRemaining ) ),
	SendPropBool( SENDINFO( m_bStopWatchTimer ) ),
	SendPropBool( SENDINFO( m_bInCaptureWatchState ) ),
	SendPropTime( SENDINFO( m_flTotalTime ) ),

#endif
END_NETWORK_TABLE()

#ifndef CLIENT_DLL
BEGIN_DATADESC(CTeamRoundTimer)
	DEFINE_KEYFIELD( m_nTimerInitialLength,		FIELD_INTEGER,	"timer_length" ),
	DEFINE_KEYFIELD( m_nTimerMaxLength,			FIELD_INTEGER,	"max_length" ),
	DEFINE_KEYFIELD( m_bShowInHUD,				FIELD_BOOLEAN,	"show_in_hud" ),
	DEFINE_KEYFIELD( m_bIsDisabled,				FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_bAutoCountdown,			FIELD_BOOLEAN,	"auto_countdown" ),
	DEFINE_KEYFIELD( m_nSetupTimeLength,		FIELD_INTEGER,	"setup_length" ),
	DEFINE_KEYFIELD( m_bResetTimeOnRoundStart,	FIELD_BOOLEAN,	"reset_time" ),
	DEFINE_KEYFIELD( m_bStartPaused,			FIELD_BOOLEAN,	"start_paused" ),
	DEFINE_KEYFIELD( m_bShowTimeRemaining,			FIELD_BOOLEAN,	"show_time_remaining" ),

	DEFINE_FUNCTION( RoundTimerSetupThink ),
	DEFINE_FUNCTION( RoundTimerThink ),

	DEFINE_INPUTFUNC( FIELD_VOID,		"Enable",			InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Disable",			InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Pause",			InputPause ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Resume",			InputResume ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetTime",			InputSetTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AddTime",			InputAddTime ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Restart",			InputRestart ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"ShowInHUD",		InputShowInHUD ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"RoundSpawn",		InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetMaxTime",		InputSetMaxTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AutoCountdown",	InputAutoCountdown ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"AddTeamTime",		InputAddTeamTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetSetupTime",		InputSetSetupTime ),

	DEFINE_OUTPUT(	m_OnRoundStart,		"OnRoundStart" ),
	DEFINE_OUTPUT(	m_OnFinished,		"OnFinished" ),
	DEFINE_OUTPUT(	m_On5MinRemain,		"On5MinRemain" ),
	DEFINE_OUTPUT(	m_On4MinRemain,		"On4MinRemain" ),
	DEFINE_OUTPUT(	m_On3MinRemain,		"On3MinRemain" ),
	DEFINE_OUTPUT(	m_On2MinRemain,		"On2MinRemain" ),
	DEFINE_OUTPUT(	m_On1MinRemain,		"On1MinRemain" ),
	DEFINE_OUTPUT(	m_On30SecRemain,	"On30SecRemain" ),
	DEFINE_OUTPUT(	m_On10SecRemain,	"On10SecRemain" ),
	DEFINE_OUTPUT(	m_On5SecRemain,		"On5SecRemain" ),
	DEFINE_OUTPUT(	m_On4SecRemain,		"On4SecRemain" ),
	DEFINE_OUTPUT(	m_On3SecRemain,		"On3SecRemain" ),
	DEFINE_OUTPUT(	m_On2SecRemain,		"On2SecRemain" ),
	DEFINE_OUTPUT(	m_On1SecRemain,		"On1SecRemain" ),
	DEFINE_OUTPUT(	m_OnSetupStart,		"OnSetupStart" ),
	DEFINE_OUTPUT(	m_OnSetupFinished,	"OnSetupFinished" ),

END_DATADESC();
#endif

#ifndef CLIENT_DLL
#define ROUND_TIMER_THINK			"CTeamplayRoundTimerThink"
#define ROUND_TIMER_SETUP_THINK		"CTeamplayRoundTimerSetupThink"
#endif

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CTeamRoundTimer::CTeamRoundTimer( void )
{
	m_bTimerPaused = false;
	m_flTimeRemaining = 0;
	m_nTimerLength = 0;
	m_nTimerInitialLength = 0;
	m_nTimerMaxLength = 0;
	m_flTimerEndTime = 0;
	m_bIsDisabled = false;
	m_bAutoCountdown = true;
	m_nState.Set( RT_STATE_NORMAL );        // we'll assume no setup time for now
	m_bStartPaused = true;
	m_bShowTimeRemaining = true;

	m_bFireFinished = true;
	m_bFire5MinRemain = true;
	m_bFire4MinRemain = true;
	m_bFire3MinRemain = true;
	m_bFire2MinRemain = true;
	m_bFire1MinRemain = true;
	m_bFire30SecRemain = true;
	m_bFire10SecRemain = true;
	m_bFire5SecRemain = true;
	m_bFire4SecRemain = true;
	m_bFire3SecRemain = true;
	m_bFire2SecRemain = true;
	m_bFire1SecRemain = true;

	m_bStopWatchTimer = false;

	m_flTotalTime = 0.0f;

	m_nSetupTimeLength = 0;

#ifndef CLIENT_DLL
	m_bPauseDueToWin = false;
	m_bResetTimeOnRoundStart = false;
	m_nTimeToUseAfterSetupFinished = 0;
	m_flNextOvertimeNag = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CTeamRoundTimer::~CTeamRoundTimer( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
void CTeamRoundTimer::Precache( void )
{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL ) 
	PrecacheScriptSound( ROUND_TIMER_60SECS );
	PrecacheScriptSound( ROUND_TIMER_30SECS );
	PrecacheScriptSound( ROUND_TIMER_10SECS );
	PrecacheScriptSound( ROUND_TIMER_5SECS );
	PrecacheScriptSound( ROUND_TIMER_4SECS );
	PrecacheScriptSound( ROUND_TIMER_3SECS );
	PrecacheScriptSound( ROUND_TIMER_2SECS );
	PrecacheScriptSound( ROUND_TIMER_1SECS );
	PrecacheScriptSound( ROUND_SETUP_60SECS );
	PrecacheScriptSound( ROUND_SETUP_30SECS );
	PrecacheScriptSound( ROUND_SETUP_10SECS );
	PrecacheScriptSound( ROUND_SETUP_5SECS );
	PrecacheScriptSound( ROUND_SETUP_4SECS );
	PrecacheScriptSound( ROUND_SETUP_3SECS );
	PrecacheScriptSound( ROUND_SETUP_2SECS );
	PrecacheScriptSound( ROUND_SETUP_1SECS );
	PrecacheScriptSound( ROUND_TIMER_TIME_ADDED );
	PrecacheScriptSound( ROUND_TIMER_TIME_ADDED_LOSER );
	PrecacheScriptSound( ROUND_TIMER_TIME_ADDED_WINNER );
	PrecacheScriptSound( ROUND_START_BELL );
#endif // TF_DLL || TF_CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::Activate( void )
{
	BaseClass::Activate();

#ifndef CLIENT_DLL
	if ( m_bShowInHUD )
	{
		SetActiveTimer( this );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::Spawn( void )
{
	Precache();

#ifdef CLIENT_DLL
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#else

	int nTimerTime = 0;

	// do we have a setup time?
	if ( m_nSetupTimeLength > 0 )
	{
		nTimerTime = m_nSetupTimeLength;
		SetState( RT_STATE_SETUP );
	}
	else
	{
		nTimerTime = m_nTimerInitialLength;
		SetState( RT_STATE_NORMAL );
	}

	m_nTimeToUseAfterSetupFinished = m_nTimerInitialLength;

	if ( IsDisabled() )  // we need to get the data initialized before actually become disabled
	{
		m_bIsDisabled = false;
		PauseTimer(); // start paused
		SetTimeRemaining( nTimerTime );
		m_bIsDisabled = true;
	}
	else
	{
		PauseTimer(); // start paused
		SetTimeRemaining( nTimerTime );
	}

	m_nTimerLength = nTimerTime;

	BaseClass::Spawn();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamRoundTimer::ShowInHud( void )
{
	return m_bShowInHUD;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the seconds left on the timer, paused or not.
//-----------------------------------------------------------------------------
float CTeamRoundTimer::GetTimeRemaining( void )
{
	float flSecondsRemaining;

	if ( IsStopWatchTimer() == true && m_bInCaptureWatchState == true )
	{
		flSecondsRemaining = m_flTotalTime;
	}
	else
	{
		if ( m_bTimerPaused )
		{
			flSecondsRemaining = m_flTimeRemaining;
		}
		else
		{
			flSecondsRemaining = m_flTimerEndTime - gpGlobals->curtime;
		}
	}

	if ( flSecondsRemaining < 0 )
	{
		flSecondsRemaining = 0.0f;
	}

	return flSecondsRemaining;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetCaptureWatchState( bool bCaptureWatch )
{
	m_bInCaptureWatchState = bCaptureWatch;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamRoundTimer::GetTimerMaxLength( void )
{
	if ( m_nState == RT_STATE_SETUP )
	{
		return m_nSetupTimeLength;
	}
	else
	{
		if ( m_nTimerMaxLength )
			return m_nTimerMaxLength;

		return m_nTimerLength;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::CalculateOutputMessages( void )
{
	float flTime = GetTimeRemaining();

#ifndef GAME_DLL
	// We need to add a couple seconds to the time remaining because we've probably lost ~0.5 seconds from the timer while 
	// waiting for the update to arrive from the server and we don't want to miss any critical countdown messages.  If the time
	// remaining is over 10 seconds...adding 2 seconds to the total when calculating our output messages won't affect anything
	if ( flTime > 10.0f )
	{
		flTime += 2.0f;
	}
#endif

	m_bFireFinished = ( flTime > 0.0f );
	m_bFire5MinRemain = ( flTime >= 300.0f );
	m_bFire4MinRemain = ( flTime >= 240.0f );
	m_bFire3MinRemain = ( flTime >= 180.0f );
	m_bFire2MinRemain = ( flTime >= 120.0f );
	m_bFire1MinRemain = ( flTime >= 60.0f );
	m_bFire30SecRemain = ( flTime >= 30.0f );
	m_bFire10SecRemain = ( flTime >= 10.0f );
	m_bFire5SecRemain = ( flTime >= 5.0f );
	m_bFire4SecRemain = ( flTime >= 4.0f );
	m_bFire3SecRemain = ( flTime >= 3.0f );
	m_bFire2SecRemain = ( flTime >= 2.0f );
	m_bFire1SecRemain = ( flTime >= 1.0f );
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::ClientThink()
{
	if ( IsDisabled() || m_bTimerPaused || IsInCommentaryMode() )
		return;

	if ( IsStopWatchTimer() == true && IsWatchingTimeStamps() == true )
		return;

	float flTime = GetTimeRemaining();

	if ( flTime <= 61.0 && m_bFire1MinRemain )
	{
		m_bFire1MinRemain = false;
		SendTimeWarning( RT_WARNING_60SECS );
	}
	else if ( flTime <= 31.0 && m_bFire30SecRemain )
	{
		m_bFire30SecRemain = false;
		SendTimeWarning( RT_WARNING_30SECS );
	}
	else if ( flTime <= 11.0 && m_bFire10SecRemain )
	{
		m_bFire10SecRemain = false;
		SendTimeWarning( RT_WARNING_10SECS );
	}
	else if ( flTime <= 6.0 && m_bFire5SecRemain )
	{
		m_bFire5SecRemain = false;
		SendTimeWarning( RT_WARNING_5SECS );
	}
	else if ( flTime <= 5.0 && m_bFire4SecRemain )
	{
		m_bFire4SecRemain = false;
		SendTimeWarning( RT_WARNING_4SECS );
	}
	else if ( flTime <= 4.0 && m_bFire3SecRemain )
	{
		m_bFire3SecRemain = false;
		SendTimeWarning( RT_WARNING_3SECS );
	}
	else if ( flTime <= 3.0 && m_bFire2SecRemain )
	{
		m_bFire2SecRemain = false;
		SendTimeWarning( RT_WARNING_2SECS );
	}
	else if ( flTime <= 2.0 && m_bFire1SecRemain )
	{
		m_bFire1SecRemain = false;
		SendTimeWarning( RT_WARNING_1SECS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_nOldTimerLength = m_nTimerLength;
	m_nOldTimerState = m_nState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_nOldTimerLength != m_nTimerLength )
	{
		// recalculate our output messages because the timer length has changed
		CalculateOutputMessages();
	}

	// if we were in state_setup and now we're in state_normal, play the bell sound
	if ( ( m_nOldTimerState == RT_STATE_SETUP ) && ( m_nState == RT_STATE_NORMAL ) )
	{
		SendTimeWarning( RT_WARNING_TIME_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTeamRoundTimer::GetTimeWarningSound( int nWarning )
{
	const char *pszRetVal;

	switch( nWarning )
	{
	case RT_WARNING_60SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_60SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_60SECS;
		}
		break;
	case RT_WARNING_30SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_30SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_30SECS;
		}
		break;
	case RT_WARNING_10SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_10SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_10SECS;
		}
		break;
	case RT_WARNING_5SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_5SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_5SECS;
		}
		break;
	case RT_WARNING_4SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_4SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_4SECS;
		}
		break;
	case RT_WARNING_3SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_3SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_3SECS;
		}
		break;
	case RT_WARNING_2SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_2SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_2SECS;
		}
		break;
	case RT_WARNING_1SECS:
		if ( m_nState == RT_STATE_SETUP )
		{
			pszRetVal = ROUND_SETUP_1SECS;
		}
		else
		{
			pszRetVal = ROUND_TIMER_1SECS;
		}
		break;
	case RT_WARNING_TIME_START:
		pszRetVal = ROUND_START_BELL;
		break;
	default:
		pszRetVal = "";
	}

	return pszRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SendTimeWarning( int nWarning )
{
#if defined( TF_CLIENT_DLL )
	// don't play any time warnings for Helltower
	if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
		return;
#endif

	// don't play sounds if the level designer has turned them off or if it's during the WaitingForPlayers time
	if ( !m_bTimerPaused && m_bAutoCountdown && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			if ( ObjectiveResource() )
			{
				bool bShouldPlaySound = false;

				if ( TeamplayRoundBasedRules()->IsInTournamentMode() == true && TeamplayRoundBasedRules()->IsInStopWatch() == true )
				{
					int iActiveTimer = ObjectiveResource()->GetTimerToShowInHUD();
					int iStopWatchTimer = ObjectiveResource()->GetStopWatchTimer();

					if ( IsStopWatchTimer() == true && IsWatchingTimeStamps() == false )
					{
						CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iActiveTimer ) );

						if ( pTimer && pTimer->IsTimerPaused() == false && pTimer->GetTimeRemaining() > GetTimeRemaining() )
						{
							bShouldPlaySound = true;
						}
					}
					else
					{
						CTeamRoundTimer *pStopWatch = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iStopWatchTimer ) );

						if ( ObjectiveResource()->GetTimerToShowInHUD() == entindex()  )
						{
							if ( pStopWatch )
							{
								if ( pStopWatch->IsTimerPaused() == true )
								{
									bShouldPlaySound = true;
								}

								if ( pStopWatch->GetTimeRemaining() > GetTimeRemaining() && pStopWatch->IsWatchingTimeStamps() == false )
								{
									bShouldPlaySound = true;
								}

								if ( pStopWatch->IsWatchingTimeStamps() == true )
								{
									bShouldPlaySound = true;
								}
							}
							else
							{
								bShouldPlaySound = true;
							}
						}
					}
				}
				else 
				{
					if( ObjectiveResource()->GetTimerToShowInHUD() == entindex() )
					{
						bShouldPlaySound = true;
					}

					if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInKothMode() )
					{
						bShouldPlaySound = true;
					}
				}

#ifdef TF_CLIENT_DLL
				if ( bShouldPlaySound == true )
				{
					pPlayer->EmitSound( GetTimeWarningSound( nWarning ) );
				}
#endif // TF_CLIENT_DLL
			}
		}
	}
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetState( int nState, bool bFireOutput )
{
	m_nState = nState;

	if ( nState == RT_STATE_SETUP )
	{
		if ( IsStopWatchTimer() == false )
		{
			TeamplayRoundBasedRules()->SetSetup( true );
		}

		SetTimerThink( RT_THINK_SETUP );

		if ( bFireOutput )
		{
			m_OnSetupStart.FireOutput( this, this );
		}
	}
	else
	{
		if ( IsStopWatchTimer() == false )
		{
			TeamplayRoundBasedRules()->SetSetup( false );
		}

		SetTimerThink( RT_THINK_NORMAL );

		if ( bFireOutput )
		{
			m_OnRoundStart.FireOutput( this, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetTimerThink( int nType )
{
	if ( nType == RT_THINK_SETUP )
	{
		SetContextThink( &CTeamRoundTimer::RoundTimerSetupThink, gpGlobals->curtime + 0.05, ROUND_TIMER_SETUP_THINK );
		SetContextThink( NULL, 0, ROUND_TIMER_THINK );
	}
	else
	{
		SetContextThink( &CTeamRoundTimer::RoundTimerThink, gpGlobals->curtime + 0.05, ROUND_TIMER_THINK );
		SetContextThink( NULL, 0, ROUND_TIMER_SETUP_THINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::RoundTimerSetupThink( void )
{
	if ( TeamplayRoundBasedRules()->IsInPreMatch() == true && IsDisabled() == false )
	{
		inputdata_t data;
		InputDisable( data );
		m_OnSetupFinished.FireOutput( this, this );
	}

	if ( IsDisabled() || m_bTimerPaused )
	{
		SetContextThink( &CTeamRoundTimer::RoundTimerSetupThink, gpGlobals->curtime + 0.05, ROUND_TIMER_SETUP_THINK );
		return;
	}

	float flTime = GetTimeRemaining();
	TeamplayRoundBasedRules()->SetOvertime( false );

	if ( flTime <= 0.0f && m_bFireFinished )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_setup_finished" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		m_OnSetupFinished.FireOutput( this, this );
		m_bFireFinished = false;

		SetTimeRemaining( m_nTimeToUseAfterSetupFinished );
		SetState( RT_STATE_NORMAL );

		if ( ShowInHud() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			UTIL_LogPrintf( "World triggered \"Round_Setup_End\"\n" );
		}
		return;
	}
	else if ( flTime <= 60.0 && m_bFire1MinRemain )
	{
		m_On1MinRemain.FireOutput( this, this );
		m_bFire1MinRemain = false;
	}
	else if ( flTime <= 30.0 && m_bFire30SecRemain )
	{
		m_On30SecRemain.FireOutput( this, this );
		m_bFire30SecRemain = false;
	}
	else if ( flTime <= 10.0 && m_bFire10SecRemain )
	{
		m_On10SecRemain.FireOutput( this, this );
		m_bFire10SecRemain = false;
	}
	else if ( flTime <= 5.0 && m_bFire5SecRemain )
	{
		m_On5SecRemain.FireOutput( this, this );
		m_bFire5SecRemain = false;
	}
	else if ( flTime <= 4.0 && m_bFire4SecRemain )
	{
		m_On4SecRemain.FireOutput( this, this );
		m_bFire4SecRemain = false;
	}
	else if ( flTime <= 3.0 && m_bFire3SecRemain )
	{
		m_On3SecRemain.FireOutput( this, this );
		m_bFire3SecRemain = false;
	}
	else if ( flTime <= 2.0 && m_bFire2SecRemain )
	{
		m_On2SecRemain.FireOutput( this, this );
		m_bFire2SecRemain = false;
	}
	else if ( flTime <= 1.0 && m_bFire1SecRemain )
	{
		m_On1SecRemain.FireOutput( this, this );
		m_bFire1SecRemain = false;
	}

	SetContextThink( &CTeamRoundTimer::RoundTimerSetupThink, gpGlobals->curtime + 0.05, ROUND_TIMER_SETUP_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::RoundTimerThink( void )
{
	if ( TeamplayRoundBasedRules()->IsInPreMatch() == true && IsDisabled() == false )
	{
		inputdata_t data;
		InputDisable( data );
	}

	if ( IsDisabled() || m_bTimerPaused || IsInCommentaryMode() || gpGlobals->eLoadType == MapLoad_Background )
	{
		SetContextThink( &CTeamRoundTimer::RoundTimerThink, gpGlobals->curtime + 0.05, ROUND_TIMER_THINK );
		return;
	}

	// Don't do anything when the game has been won or if we're loading a bugbait report
	if ( TeamplayRoundBasedRules()->RoundHasBeenWon() ||
		 TeamplayRoundBasedRules()->IsLoadingBugBaitReport() )
	{
		// We want to stop timers when the round has been won, but we don't want to 
		// force mapmakers to deal with having to unpause it. This little hack works around that.
		if ( !m_bTimerPaused )
		{
			PauseTimer();
			m_bPauseDueToWin = true;
		}

		SetContextThink( &CTeamRoundTimer::RoundTimerThink, gpGlobals->curtime + 0.05, ROUND_TIMER_THINK );
		return;
	}
	else if ( m_bPauseDueToWin )
	{
		ResumeTimer();
		m_bPauseDueToWin = false;
	}

	float flTime = GetTimeRemaining();

	if ( flTime > 0 && ShowInHud() ) // is this the timer we're showing in the HUD?
	{
		TeamplayRoundBasedRules()->SetOvertime( false );
	}

	if ( flTime <= 0.0f && m_bFireFinished )
	{
		// Allow the gamerules to prevent timer expiration (i.e. while a control point is contested)
		if ( !TeamplayGameRules()->TimerMayExpire() )
		{
			// we don't want the timer to keep going (negative time)
			m_flTimerEndTime = gpGlobals->curtime;

			// is this the timer we're showing in the HUD?
			if ( ShowInHud() )
			{
				if ( !TeamplayRoundBasedRules()->InOvertime() )
				{
					TeamplayRoundBasedRules()->SetOvertime( true );
				}
#if defined( TF_DLL )
				else
				{
					if ( tf_overtime_nag.GetBool() && ( gpGlobals->curtime > m_flNextOvertimeNag ) )
					{
						m_flNextOvertimeNag = gpGlobals->curtime + 1.0f;

						if ( RandomInt( 0, 1 ) > 0 )
						{
							IGameEvent *event = gameeventmanager->CreateEvent( "overtime_nag" );
							if ( event )
							{
								gameeventmanager->FireEvent( event );
							}
						}
					}
				}
#endif
			}

			SetContextThink( &CTeamRoundTimer::RoundTimerThink, gpGlobals->curtime + 0.05, ROUND_TIMER_THINK );
			return;
		}

		m_OnFinished.FireOutput( this, this );
		m_bFireFinished = false;
	}
	else if ( flTime <= 300.0 && m_bFire5MinRemain )
	{
		m_On5MinRemain.FireOutput( this, this );
		m_bFire5MinRemain = false;
	}
	else if ( flTime <= 240.0 && m_bFire4MinRemain )
	{
		m_On4MinRemain.FireOutput( this, this );
		m_bFire4MinRemain = false;
	}
	else if ( flTime <= 180.0 && m_bFire3MinRemain )
	{
		m_On3MinRemain.FireOutput( this, this );
		m_bFire3MinRemain = false;
	}
	else if ( flTime <= 120.0 && m_bFire2MinRemain )
	{
		m_On2MinRemain.FireOutput( this, this );
		m_bFire2MinRemain = false;
	}
	else if ( flTime <= 60.0 && m_bFire1MinRemain )
	{
		m_On1MinRemain.FireOutput( this, this );
		m_bFire1MinRemain = false;
	}
	else if ( flTime <= 30.0 && m_bFire30SecRemain )
	{
		m_On30SecRemain.FireOutput( this, this );
		m_bFire30SecRemain = false;
	}
	else if ( flTime <= 10.0 && m_bFire10SecRemain )
	{
		m_On10SecRemain.FireOutput( this, this );
		m_bFire10SecRemain = false;
	}
	else if ( flTime <= 5.0 && m_bFire5SecRemain )
	{
		m_On5SecRemain.FireOutput( this, this );
		m_bFire5SecRemain = false;
	}
	else if ( flTime <= 4.0 && m_bFire4SecRemain )
	{
		m_On4SecRemain.FireOutput( this, this );
		m_bFire4SecRemain = false;
	}
	else if ( flTime <= 3.0 && m_bFire3SecRemain )
	{
		m_On3SecRemain.FireOutput( this, this );
		m_bFire3SecRemain = false;
	}
	else if ( flTime <= 2.0 && m_bFire2SecRemain )
	{
		m_On2SecRemain.FireOutput( this, this );
		m_bFire2SecRemain = false;
	}
	else if ( flTime <= 1.0 && m_bFire1SecRemain )
	{
		m_On1SecRemain.FireOutput( this, this );
		m_bFire1SecRemain = false;
	}

	SetContextThink( &CTeamRoundTimer::RoundTimerThink, gpGlobals->curtime + 0.05, ROUND_TIMER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputRoundSpawn( inputdata_t &input )
{
	if ( !m_bResetTimeOnRoundStart && ( m_nState == RT_STATE_NORMAL ) )
	{
		m_nTimeToUseAfterSetupFinished = GetTimeRemaining();
	}
	else
	{
		m_nTimeToUseAfterSetupFinished = m_nTimerInitialLength;
	}

	if ( m_nSetupTimeLength > 0 )
	{
		SetTimeRemaining( m_nSetupTimeLength );
		SetState( RT_STATE_SETUP );

		if ( ShowInHud() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			UTIL_LogPrintf( "World triggered \"Round_Setup_Begin\"\n" );
		}
	}
	else
	{
		SetTimeRemaining( m_nTimeToUseAfterSetupFinished );
		SetState( RT_STATE_NORMAL );
	}

	if ( !m_bStartPaused && !TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
	{
		ResumeTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: To set the initial timer duration
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetTimeRemaining( int iTimerSeconds )
{
	if ( IsDisabled() )
		return;

	// make sure we don't go over our max length
	if ( m_nTimerMaxLength > 0 )
	{
		if ( iTimerSeconds > m_nTimerMaxLength )
		{
			iTimerSeconds = m_nTimerMaxLength;
		}
	}

	m_flTimeRemaining = (float)iTimerSeconds;
	m_flTimerEndTime = gpGlobals->curtime + m_flTimeRemaining;
	m_nTimerLength = iTimerSeconds;
	
	CalculateOutputMessages();
}

//-----------------------------------------------------------------------------
// Purpose: To set the initial timer duration
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetStopWatchTimeStamp( void )
{
	if ( IsDisabled() )
		return;

	if ( IsWatchingTimeStamps() == false )
		return;

	m_flTotalTime = m_flTotalTime + (gpGlobals->curtime - m_flTimerEndTime);
	m_flTimerEndTime = gpGlobals->curtime;

	CalculateOutputMessages();
}

//-----------------------------------------------------------------------------
// Purpose: Timer is paused at round end, stops the countdown
//-----------------------------------------------------------------------------
void CTeamRoundTimer::PauseTimer( void )
{
	if ( IsDisabled() )
		return;

	if ( m_bTimerPaused == false )
	{
		m_bTimerPaused = true;

		m_flTimeRemaining = m_flTimerEndTime - gpGlobals->curtime;
	}

	// Clear pause on win flag, because we've been set by the mapmaker
	m_bPauseDueToWin = false;
}

//-----------------------------------------------------------------------------
// Purpose: To start or re-start the timer after a pause
//-----------------------------------------------------------------------------
void CTeamRoundTimer::ResumeTimer( void )
{
	if ( IsDisabled() )
		return;

	if ( m_bTimerPaused == true )
	{
		m_bTimerPaused = false;

		if ( IsStopWatchTimer() == true && m_bInCaptureWatchState == true )
		{
			m_flTimerEndTime = gpGlobals->curtime;
		}
		else
		{
			m_flTimerEndTime = gpGlobals->curtime + m_flTimeRemaining;			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add seconds to the timer while it is running or paused
//-----------------------------------------------------------------------------
void CTeamRoundTimer::AddTimerSeconds( int iSecondsToAdd, int iTeamResponsible /* = TEAM_UNASSIGNED*/ )
{
	if ( IsDisabled() )
		return;

	if ( TeamplayRoundBasedRules()->InStalemate() )
		return;

	// we only want to add time if we're round_running or team_win so the control points 
	// don't add time when they try to set their default owner when the map is first loading
	if ( TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING && TeamplayRoundBasedRules()->State_Get() != GR_STATE_TEAM_WIN )
		return;

	if ( m_nTimerMaxLength > 0 )
	{
		// will adding this many seconds push us over our max length?
		if ( GetTimeRemaining() + iSecondsToAdd > m_nTimerMaxLength )
		{
			// adjust to only add up to our max length
			iSecondsToAdd = m_nTimerMaxLength - GetTimeRemaining();
		}
	}

	if ( m_bTimerPaused )
	{
		m_flTimeRemaining += (float)iSecondsToAdd;
	}
	else
	{
		m_flTimerEndTime += (float)iSecondsToAdd;
	}

	m_nTimerLength += iSecondsToAdd;
	CalculateOutputMessages();

	if ( ( ObjectiveResource() && ObjectiveResource()->GetTimerInHUD() == entindex() ) || ( TeamplayRoundBasedRules()->IsInKothMode() ) )
	{
		if ( !TeamplayRoundBasedRules()->InStalemate() && !TeamplayRoundBasedRules()->RoundHasBeenWon() && !TeamplayRoundBasedRules()->IsInKothMode() )
		{
			if ( iTeamResponsible >= LAST_SHARED_TEAM+1 )
			{
				for ( int iTeam = LAST_SHARED_TEAM+1 ; iTeam < GetNumberOfTeams(); iTeam++ )
				{
					if ( iTeam == iTeamResponsible )
					{
						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), ROUND_TIMER_TIME_ADDED_WINNER );
						
					}
					else
					{
						CTeamRecipientFilter filter( iTeam, true );
						EmitSound( filter, entindex(), ROUND_TIMER_TIME_ADDED_LOSER );
					}
				}
			}
			else
			{
				CReliableBroadcastRecipientFilter filter;
				EmitSound( filter, entindex(), ROUND_TIMER_TIME_ADDED );
			}
		}

		// is this the timer we're showing in the HUD?
		if ( m_bShowInHUD )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_timer_time_added" );
			if ( event )
			{
				event->SetInt( "timer", entindex() );
				event->SetInt( "seconds_added", iSecondsToAdd );
				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CTeamRoundTimer::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputPause( inputdata_t &input )
{
	PauseTimer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputResume( inputdata_t &input )
{
	ResumeTimer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputSetTime( inputdata_t &input )
{
	if ( IsStopWatchTimer() == true && IsWatchingTimeStamps() == true )
	{
		SetStopWatchTimeStamp();
	}
	else
	{
		int nSeconds = input.value.Int();
		SetTimeRemaining( nSeconds );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputSetMaxTime( inputdata_t &input )
{
	int nSeconds = input.value.Int();
	m_nTimerMaxLength = nSeconds;

	if ( m_nTimerMaxLength > 0 )
	{
		// make sure our current time is not above the max length
		if ( GetTimeRemaining() > m_nTimerMaxLength )
		{
			SetTimeRemaining( m_nTimerMaxLength );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputAddTime( inputdata_t &input )
{
	int nSeconds = input.value.Int();
	AddTimerSeconds( nSeconds );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputAddTeamTime( inputdata_t &input )
{
	char		token[128];
	const char	*p = STRING( input.value.StringID() );
	int			nTeam = TEAM_UNASSIGNED;
	int			nSeconds = 0;

	// get the team
	p = nexttoken( token, p, ' ' );
	if ( token )
	{
		nTeam = Q_atoi( token );
	}

	// get the time
	p = nexttoken( token, p, ' ' );
	if ( token )
	{
		nSeconds = Q_atoi( token );
	}

	if ( nSeconds != 0 )
	{
		AddTimerSeconds( nSeconds, nTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputRestart( inputdata_t &input )
{
	SetTimeRemaining( m_nTimerInitialLength );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputEnable( inputdata_t &input )
{ 
	m_bIsDisabled = false;
	ResumeTimer();

	if ( m_bShowInHUD )
	{
		SetActiveTimer( this );
	}

	if ( IsStopWatchTimer() == true && IsWatchingTimeStamps() == true )
	{
		m_flTimerEndTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputDisable( inputdata_t &input )
{ 
	PauseTimer();
	m_bIsDisabled = true;

	if ( m_bShowInHUD )
	{
		SetActiveTimer( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputShowInHUD( inputdata_t &input )
{ 
	int nShow = input.value.Int();

	if ( m_bShowInHUD && !nShow )
	{
		SetActiveTimer( NULL );
	}
	else if ( nShow == 1 )
	{
		SetActiveTimer( this );
		SetState( m_nState, false ); // set our current state again so the gamerules are updated with our setup state
	}

	m_bShowInHUD = ( nShow == 1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputAutoCountdown( inputdata_t &input )
{ 
	int nAuto = input.value.Int();
	SetAutoCountdown( nAuto == 1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::InputSetSetupTime( inputdata_t &input )
{ 
	int nSetupTime = input.value.Int();
	if ( nSetupTime >= 0 )
	{
		m_nSetupTimeLength = nSetupTime;
	}

	if ( !IsDisabled() )
	{
		if ( m_nState == RT_STATE_SETUP )
		{
			SetTimeRemaining( m_nSetupTimeLength );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamRoundTimer::SetActiveTimer( CTeamRoundTimer *pNewlyActive )
{
	CBaseEntity *pChosenTimer = pNewlyActive;	

	// Ensure all other timers are off.
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" )) != NULL)
	{
		if ( pEntity == pNewlyActive )
			continue;

		CTeamRoundTimer *pTimer = assert_cast< CTeamRoundTimer* >( pEntity );
		if ( !pTimer->IsDisabled() && pTimer->ShowInHud() )
		{
			if ( pChosenTimer )
			{
				// Turn off all other hud timers
				pTimer->SetShowInHud( false );
			}
			else
			{
				// Found a timer. Use it.
				pChosenTimer = pTimer;
			}
		}
	}

	ObjectiveResource()->SetTimerInHUD( pChosenTimer );
}

#endif
