//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"
#include "teamplayroundbased_gamerules.h"

#ifdef CLIENT_DLL
	#include "iclientmode.h"
	#include <vgui_controls/AnimationController.h>
	#include <igameevents.h>
	#include "c_team.h"
	#include "c_playerresource.h"
	#define CTeam C_Team

#else
	#include "viewport_panel_names.h"
	#include "team.h"
	#include "mapentities.h"
	#include "gameinterface.h"
	#include "eventqueue.h"
	#include "team_control_point_master.h"
	#include "team_train_watcher.h"
	#include "serverbenchmark_base.h"

#if defined( REPLAY_ENABLED )	
	#include "replay/ireplaysystem.h"
	#include "replay/iserverreplaycontext.h"
	#include "replay/ireplaysessionrecorder.h"
#endif // REPLAY_ENABLED
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	#include "tf_gamerules.h"
	#include "tf_lobby.h"
	#ifdef GAME_DLL
		#include "player_vs_environment/tf_population_manager.h"
		#include "../server/tf/tf_gc_server.h"
		#include "../server/tf/tf_objective_resource.h"
	#else
		#include "../client/tf/tf_gc_client.h"
		#include "../client/tf/c_tf_objective_resource.h"
	#endif // GAME_DLL
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
CUtlVector< CHandle<CTeamControlPointMaster> >		g_hControlPointMasters;

extern bool IsInCommentaryMode( void );

#if defined( REPLAY_ENABLED )
extern IReplaySystem *g_pReplay;
#endif // REPLAY_ENABLED
#endif

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundState( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTeamplayRoundBasedRules *pGamerules = ( CTeamplayRoundBasedRules *)pStruct;
	int iRoundState = pData->m_Value.m_Int;
	pGamerules->SetRoundState( iRoundState );
}
#endif 

BEGIN_NETWORK_TABLE_NOBASE( CTeamplayRoundBasedRules, DT_TeamplayRoundBasedRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iRoundState ), 0, RecvProxy_TeamplayRoundState ),
	RecvPropBool( RECVINFO( m_bInWaitingForPlayers ) ),
	RecvPropInt( RECVINFO( m_iWinningTeam ) ),
	RecvPropInt( RECVINFO( m_bInOvertime ) ),
	RecvPropInt( RECVINFO( m_bInSetup ) ),
	RecvPropInt( RECVINFO( m_bSwitchedTeamsThisRound ) ),
	RecvPropBool( RECVINFO( m_bAwaitingReadyRestart ) ),
	RecvPropTime( RECVINFO( m_flRestartRoundTime ) ),
	RecvPropTime( RECVINFO( m_flMapResetTime ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flNextRespawnWave), RecvPropTime( RECVINFO(m_flNextRespawnWave[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamRespawnWaveTimes), RecvPropFloat( RECVINFO(m_TeamRespawnWaveTimes[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTeamReady), RecvPropBool( RECVINFO(m_bTeamReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bStopWatch ) ),
	RecvPropBool( RECVINFO( m_bMultipleTrains ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bPlayerReady), RecvPropBool( RECVINFO(m_bPlayerReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bCheatsEnabledDuringLevel ) ),

#else
	SendPropInt( SENDINFO( m_iRoundState ), 5 ),
	SendPropBool( SENDINFO( m_bInWaitingForPlayers ) ),
	SendPropInt( SENDINFO( m_iWinningTeam ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bInOvertime ) ),
	SendPropBool( SENDINFO( m_bInSetup ) ),
	SendPropBool( SENDINFO( m_bSwitchedTeamsThisRound ) ),
	SendPropBool( SENDINFO( m_bAwaitingReadyRestart ) ),
	SendPropTime( SENDINFO( m_flRestartRoundTime ) ),
	SendPropTime( SENDINFO( m_flMapResetTime ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flNextRespawnWave), SendPropTime( SENDINFO_ARRAY(m_flNextRespawnWave) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamRespawnWaveTimes), SendPropFloat( SENDINFO_ARRAY(m_TeamRespawnWaveTimes) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bTeamReady), SendPropBool( SENDINFO_ARRAY(m_bTeamReady) ) ),
	SendPropBool( SENDINFO( m_bStopWatch ) ),
	SendPropBool( SENDINFO( m_bMultipleTrains ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bPlayerReady), SendPropBool( SENDINFO_ARRAY(m_bPlayerReady) ) ),
	SendPropBool( SENDINFO( m_bCheatsEnabledDuringLevel ) ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( TeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundBasedRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CTeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )
	RecvPropDataTable( "teamplayroundbased_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TeamplayRoundBasedRules ), RecvProxy_TeamplayRoundBasedRules )
END_RECV_TABLE()

void CTeamplayRoundBasedRulesProxy::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRules->OnPreDataChanged(updateType);
}
void CTeamplayRoundBasedRulesProxy::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRules->OnDataChanged(updateType);
}

#else
void* SendProxy_TeamplayRoundBasedRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE( CTeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )
	SendPropDataTable( "teamplayroundbased_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TeamplayRoundBasedRules ), SendProxy_TeamplayRoundBasedRules )
END_SEND_TABLE()

BEGIN_DATADESC( CTeamplayRoundBasedRulesProxy )
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetStalemateOnTimelimit", InputSetStalemateOnTimelimit ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRulesProxy::InputSetStalemateOnTimelimit( inputdata_t &inputdata )
{
	TeamplayRoundBasedRules()->SetStalemateOnTimelimit( inputdata.value.Bool() );
}
#endif

ConVar mp_capstyle( "mp_capstyle", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Sets the style of capture points used. 0 = Fixed players required to cap. 1 = More players cap faster, but longer cap times." );
ConVar mp_blockstyle( "mp_blockstyle", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Sets the style of capture point blocking used. 0 = Blocks break captures completely. 1 = Blocks only pause captures." );
ConVar mp_respawnwavetime( "mp_respawnwavetime", "10.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Time between respawn waves." );
ConVar mp_capdeteriorate_time( "mp_capdeteriorate_time", "90.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Time it takes for a full capture point to deteriorate." );
ConVar mp_tournament( "mp_tournament", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );

#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
ConVar mp_highlander( "mp_highlander", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow only 1 of each player class type." );
#endif

//Arena Mode
ConVar tf_arena_preround_time( "tf_arena_preround_time", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of the Pre-Round time", true, 5.0, true, 15.0 );
ConVar tf_arena_round_time( "tf_arena_round_time", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_arena_max_streak( "tf_arena_max_streak", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Teams will be scrambled if one team reaches this streak" );
ConVar tf_arena_use_queue( "tf_arena_use_queue", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enables the spectator queue system for Arena." );

ConVar mp_teams_unbalance_limit( "mp_teams_unbalance_limit", "1", FCVAR_REPLICATED,
					 "Teams are unbalanced when one team has this many more players than the other team. (0 disables check)",
					 true, 0,	// min value
					 true, 30	// max value
					 );

ConVar mp_maxrounds( "mp_maxrounds", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "max number of rounds to play before server changes maps", true, 0, false, 0 );
ConVar mp_winlimit( "mp_winlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Max score one team can reach before server changes maps", true, 0, false, 0 );
ConVar mp_disable_respawn_times( "mp_disable_respawn_times", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar mp_bonusroundtime( "mp_bonusroundtime", "15", FCVAR_REPLICATED, "Time after round win until round restarts", true, 5, true, 15 );
ConVar mp_bonusroundtime_final( "mp_bonusroundtime_final", "15", FCVAR_REPLICATED, "Time after final round ends until round restarts", true, 5, true, 300 );
ConVar mp_stalemate_meleeonly( "mp_stalemate_meleeonly", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Restrict everyone to melee weapons only while in Sudden Death." );
ConVar mp_forceautoteam( "mp_forceautoteam", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Automatically assign players to teams when joining." );

#if defined( _DEBUG ) || defined( STAGING_ONLY )
ConVar mp_developer( "mp_developer", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_NOTIFY, "1: basic conveniences (instant respawn and class change, etc).  2: add combat conveniences (infinite ammo, buddha, etc)" );
#endif // _DEBUG || STAGING_ONLY

#ifdef GAME_DLL
ConVar mp_showroundtransitions( "mp_showroundtransitions", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show gamestate round transitions." );
ConVar mp_enableroundwaittime( "mp_enableroundwaittime", "1", FCVAR_REPLICATED, "Enable timers to wait between rounds." );
ConVar mp_showcleanedupents( "mp_showcleanedupents", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show entities that are removed on round respawn." );
ConVar mp_restartround( "mp_restartround", "0", FCVAR_GAMEDLL, "If non-zero, the current round will restart in the specified number of seconds" );	

ConVar mp_stalemate_timelimit( "mp_stalemate_timelimit", "240", FCVAR_REPLICATED, "Timelimit (in seconds) of the stalemate round." );
ConVar mp_autoteambalance( "mp_autoteambalance", "1", FCVAR_NOTIFY );

ConVar mp_stalemate_enable( "mp_stalemate_enable", "0", FCVAR_NOTIFY, "Enable/Disable stalemate mode." );
ConVar mp_match_end_at_timelimit( "mp_match_end_at_timelimit", "0", FCVAR_NOTIFY, "Allow the match to end when mp_timelimit hits instead of waiting for the end of the current round." );

ConVar mp_holiday_nogifts( "mp_holiday_nogifts", "0", FCVAR_NOTIFY, "Set to 1 to prevent holiday gifts from spawning when players are killed." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_SwitchTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );

		if ( pRules )
		{
			pRules->SetSwitchTeams( true );
			mp_restartgame.SetValue( 5 );
			pRules->ShouldResetScores( false, false );
			pRules->ShouldResetRoundsPlayed( false );
		}
	}
}

static ConCommand mp_switchteams( "mp_switchteams", cc_SwitchTeams, "Switch teams and restart the game" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_ScrambleTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );

		if ( pRules )
		{
			pRules->SetScrambleTeams( true );
			mp_restartgame.SetValue( 5 );
			pRules->ShouldResetScores( true, false );

			if ( args.ArgC() == 2 )
			{
				// Don't reset the roundsplayed when mp_scrambleteams 2 is passed
				if ( atoi( args[1] ) == 2 )
				{
					pRules->ShouldResetRoundsPlayed( false );
				}
			}
		}
	}
}

static ConCommand mp_scrambleteams( "mp_scrambleteams", cc_ScrambleTeams, "Scramble the teams and restart the game" );
ConVar mp_scrambleteams_auto( "mp_scrambleteams_auto", "1", FCVAR_NOTIFY, "Server will automatically scramble the teams if criteria met.  Only works on dedicated servers." );
ConVar mp_scrambleteams_auto_windifference( "mp_scrambleteams_auto_windifference", "2", FCVAR_NOTIFY, "Number of round wins a team must lead by in order to trigger an auto scramble." );

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"point_commentary_node",
	"point_viewcontrol",
	"func_precipitation",
	"func_team_wall",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"commentary_auto",
	"point_commentary_node",
	"point_commentary_viewpoint",
	"bot_roster",
	"info_populator",
	"", // END Marker
};

CON_COMMAND_F( mp_forcewin, "Forces team to win", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		int iTeam = TEAM_UNASSIGNED;		
		if ( args.ArgC() == 1 )
		{
			// if no team specified, use player 1's team
			iTeam = UTIL_PlayerByIndex( 1 )->GetTeamNumber();	
		}
		else if ( args.ArgC() == 2 )
		{
			// if team # specified, use that
			iTeam = atoi( args[1] );
		}
		else
		{
			Msg( "Usage: mp_forcewin <opt: team#>" );
			return;
		}

		int iWinReason = ( TEAM_UNASSIGNED == iTeam ? WINREASON_STALEMATE : WINREASON_ALL_POINTS_CAPTURED );
		pRules->SetWinningTeam( iTeam, iWinReason );
	}
}

#endif // GAME_DLL

// Utility function
bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamplayRoundBasedRules::CTeamplayRoundBasedRules( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_flNextRespawnWave.Set( i, 0 );
		m_TeamRespawnWaveTimes.Set( i, -1.0f );
			
#ifdef GAME_DLL
		m_flOriginalTeamRespawnWaveTime[i] = -1.0f;
#endif
	}

	m_bInOvertime = false;
	m_bInSetup = false;
	m_bSwitchedTeamsThisRound = false;
	m_flStopWatchTotalTime = -1.0f;
	m_bMultipleTrains = false;
	m_bAllowBetweenRounds = true;

#ifdef GAME_DLL
	ListenForGameEvent( "server_changelevel_failed" );

	m_pCurStateInfo = NULL;
	State_Transition( GR_STATE_PREGAME );

	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	InitTeams();
	ResetMapTime();
	ResetScores();
	SetForceMapReset( true );
	SetRoundToPlayNext( NULL_STRING );
	m_bInWaitingForPlayers  = false;
	m_bAwaitingReadyRestart = false;
	m_flRestartRoundTime = -1.0f;
	m_flMapResetTime = 0.0f;
	m_bPrevRoundWasWaitingForPlayers = false;
	m_iWinningTeam = TEAM_UNASSIGNED;

	m_iszPreviousRounds.RemoveAll();
	SetFirstRoundPlayed( NULL_STRING );

	m_bAllowStalemateAtTimelimit = false;
	m_bChangelevelAfterStalemate = false;
	m_flRoundStartTime = 0.0f;
	m_flNewThrottledAlertTime = 0.0f;
	m_flStartBalancingTeamsAt = 0.0f;
	m_bPrintedUnbalanceWarning = false;
	m_flFoundUnbalancedTeamsTime = -1.0f;
	m_flWaitingForPlayersTimeEnds = 0.0f;
	m_flLastTeamWin = -1.0f;

	m_nRoundsPlayed = 0;
	m_bUseAddScoreAnim = false;

	m_bStopWatch = false;
	m_bAwaitingReadyRestart = false;

	if ( IsInTournamentMode() )
	{
		m_bAwaitingReadyRestart = true;
	}

	m_flAutoBalanceQueueTimeEnd = -1.0f;
	m_nAutoBalanceQueuePlayerIndex = -1;
	m_nAutoBalanceQueuePlayerScore = -1;

	SetDefLessFunc( m_GameTeams );
	m_bCheatsEnabledDuringLevel = false;

	ResetPlayerAndTeamReadyState();

#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetTeamRespawnWaveTime( int iTeam, float flValue ) 
{ 
	if ( flValue < 0 )
	{
		flValue = 0;
	}

	// initialized to -1 so we can try to determine if this is the first spawn time we have received for this team
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flValue;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::AddTeamRespawnWaveTime( int iTeam, float flValue ) 
{ 
	float flAddAmount = flValue;
	float flCurrentSetting = m_TeamRespawnWaveTimes[iTeam];
	float flNewValue;

	if ( flCurrentSetting < 0 )
	{
		flCurrentSetting = mp_respawnwavetime.GetFloat();
	}

	// initialized to -1 so we can try to determine if this is the first spawn time we have received for this team
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flCurrentSetting;
	}

	flNewValue = flCurrentSetting + flAddAmount;

	if ( flNewValue < 0 )
	{
		flNewValue = 0;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flNewValue );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: don't let us spawn before our freezepanel time would have ended, even if we skip it
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer ) 
{ 
	if ( State_Get() == GR_STATE_STALEMATE )
		return 0;

	// If we are purely checking when the next respawn wave is for this team
	if ( pPlayer == NULL )
	{
		return m_flNextRespawnWave[iTeam];
	}

	// The soonest this player may spawn
	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer );
	if ( ShouldRespawnQuickly( pPlayer ) )
	{
		return flMinSpawnTime;
	}

	// the next scheduled respawn wave time
	float flNextRespawnTime = m_flNextRespawnWave[iTeam];

	// the length of one respawn wave. We'll check in increments of this
	float flRespawnWaveMaxLen = GetRespawnWaveMaxLength( iTeam );

	if ( flRespawnWaveMaxLen <= 0 )
	{
		return flNextRespawnTime;
	}

	// Keep adding the length of one respawn until we find a wave that
	// this player will be eligible to spawn in.
	while ( flNextRespawnTime < flMinSpawnTime )
	{
		flNextRespawnTime += flRespawnWaveMaxLen; 
	}

	return flNextRespawnTime; 
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetMinTimeWhenPlayerMaySpawn( CBasePlayer *pPlayer )
{
	// Min respawn time is the sum of
	//
	// a) the length of one full *unscaled* respawn wave for their team
	//		and
	// b) death anim length + freeze panel length

	float flDeathAnimLength = 2.0 + spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();

	float fMinDelay = flDeathAnimLength;

	if ( !ShouldRespawnQuickly( pPlayer ) )
	{
		fMinDelay += GetRespawnWaveMaxLength( pPlayer->GetTeamNumber(), false );
	}

	return pPlayer->GetDeathTime() + fMinDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	m_bCheatsEnabledDuringLevel = sv_cheats && sv_cheats->GetBool();
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetRespawnTimeScalar( int iTeam )
{
	// For long respawn times, scale the time as the number of players drops
	int iOptimalPlayers = 8;	// 16 players total, 8 per team

	int iNumPlayers = GetGlobalTeam(iTeam)->GetNumPlayers();

	float flScale = RemapValClamped( iNumPlayers, 1, iOptimalPlayers, 0.25, 1.0 );
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::FireGameEvent( IGameEvent * event )
{
#ifdef GAME_DLL
	const char *eventName = event->GetName();
	if ( g_fGameOver && !Q_strcmp( eventName, "server_changelevel_failed" ) )
	{
		Warning( "In gameover, but failed to load the next map. Trying next map in cycle.\n" );
		nextlevel.SetValue( "" );
		ChangeLevel();
	}
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetForceMapReset( bool reset )
{
	m_bForceMapReset = reset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::Think( void )
{
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime && ( m_flIntermissionEndTime < gpGlobals->curtime ) )
		{
			if ( !IsX360() )
			{
				ChangeLevel(); // intermission is over
			}
			else
			{
				IGameEvent * event = gameeventmanager->CreateEvent( "player_stats_updated" );
				if ( event )
				{
					event->SetBool( "forceupload", true );
					gameeventmanager->FireEvent( event );
				}
				engine->MultiplayerEndGame();
			}

			// Don't run this code again
			m_flIntermissionEndTime = 0.f;
		}

		return;
	}

	State_Think();

	if ( m_hWaitingForPlayersTimer )
	{
		Assert( m_bInWaitingForPlayers );
	}

	if ( gpGlobals->curtime > m_flNextPeriodicThink )
	{
		// Don't end the game during win or stalemate states
		if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_STALEMATE && State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( CheckWinLimit() )
				return;

			if ( CheckMaxRounds() )
				return;
		}

		CheckRestartRound();
		CheckWaitingForPlayers();

		m_flNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	// Watch dog for cheats ever being enabled during a level
	if ( !m_bCheatsEnabledDuringLevel && sv_cheats && sv_cheats->GetBool() )
	{
		m_bCheatsEnabledDuringLevel = true;
	}

	// Bypass teamplay think.
	CGameRules::Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::TimerMayExpire( void )
{
#ifndef CSTRIKE_DLL
	// team_train_watchers can also prevent timer expiring ( overtime )
	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->TimerMayExpire() )
		{
			return false;
		}

		pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}
#endif

	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckChatText( CBasePlayer *pPlayer, char *pText )
{
	CheckChatForReadySignal( pPlayer, pText );

	BaseClass::CheckChatText( pPlayer, pText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg )
{
	if ( IsInTournamentMode() == false )
	{	
		if( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_clan_ready_signal.GetString() ) )
		{
			int iTeam = pPlayer->GetTeamNumber();
			if ( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() )
			{
				m_bTeamReady.Set( iTeam, true );

				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_team_ready" );
				if ( event )
				{
					event->SetInt( "team", iTeam );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::GoToIntermission( void )
{
	if ( IsInTournamentMode() == true )
		return;

	BaseClass::GoToIntermission();

	// set all players to FL_FROZEN
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );
		}
	}

	// Print out map stats to a text file
	//WriteStatsFile( "stats.xml" );

	State_Enter( GR_STATE_GAME_OVER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetInWaitingForPlayers( bool bWaitingForPlayers  )
{
	// never waiting for players when loading a bug report
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	if( m_bInWaitingForPlayers == bWaitingForPlayers  )
		return;

	if ( IsInArenaMode() == true && m_flWaitingForPlayersTimeEnds == -1 && IsInTournamentMode() == false )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	m_bInWaitingForPlayers = bWaitingForPlayers;

	if( m_bInWaitingForPlayers )
	{
		m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();
	}
	else
	{
		m_flWaitingForPlayersTimeEnds = -1;

		if ( m_hWaitingForPlayersTimer )
		{
			UTIL_Remove( m_hWaitingForPlayersTimer );
		}

		RestoreActiveTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetOvertime( bool bOvertime ) 
{ 
	if ( m_bInOvertime == bOvertime )
		return;

	if ( bOvertime )
	{
		UTIL_LogPrintf( "World triggered \"Round_Overtime\"\n" );
	}

	m_bInOvertime = bOvertime;

	if ( m_bInOvertime )
	{
		// tell train watchers that we've transitioned to overtime

#ifndef CSTRIKE_DLL
		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			variant_t emptyVariant;
			pWatcher->AcceptInput( "OnStartOvertime", NULL, NULL, emptyVariant, 0 );

			pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetSetup( bool bSetup ) 
{ 
	if ( m_bInSetup == bSetup )
		return;

	m_bInSetup = bSetup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckWaitingForPlayers( void )
{
	// never waiting for players when loading a bug report, or training
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background || !AllowWaitingForPlayers() )
		return;

	if( mp_waitingforplayers_restart.GetBool() )
	{
		if( m_bInWaitingForPlayers )
		{
			m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();

			if ( m_hWaitingForPlayersTimer )
			{
				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			}
		}
		else
		{
			SetInWaitingForPlayers( true );
		}

		mp_waitingforplayers_restart.SetValue( 0 );
	}

	bool bCancelWait = ( mp_waitingforplayers_cancel.GetBool() || IsInItemTestingMode() ) && !IsInTournamentMode();

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		bCancelWait = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bCancelWait )
	{
		// Cancel the wait period and manually Resume() the timer if 
		// it's not supposed to start paused at the beginning of a round.
		// We must do this before SetInWaitingForPlayers() is called because it will
		// restore the timer in the HUD and set the handle to NULL
#ifndef CSTRIKE_DLL
		if ( m_hPreviousActiveTimer.Get() )
		{
			CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( m_hPreviousActiveTimer.Get() );
			if ( pTimer && !pTimer->StartPaused() )
			{
				pTimer->ResumeTimer();
			}
		}
#endif
		SetInWaitingForPlayers( false );
		mp_waitingforplayers_cancel.SetValue( 0 );
	}

	if( m_bInWaitingForPlayers )
	{
		if ( IsInTournamentMode() == true )
			return;

		// only exit the waitingforplayers if the time is up, and we are not in a round
		// restart countdown already, and we are not waiting for a ready restart
		if( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds && m_flRestartRoundTime < 0 && !m_bAwaitingReadyRestart )
		{
			m_flRestartRoundTime = gpGlobals->curtime;	// reset asap

			if ( IsInArenaMode() == true )
			{
				if ( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds )
				{
					SetInWaitingForPlayers( false );
					State_Transition( GR_STATE_PREROUND );
				}

				return;
			}

			// if "waiting for players" is ending and we're restarting...
			// keep the current round that we're already running around in as the first round after the restart
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster && pMaster->PlayingMiniRounds() && pMaster->GetCurrentRound() )
			{
				SetRoundToPlayNext( pMaster->GetRoundToUseAfterRestart() );
			}
		}
		else
		{
			if ( !m_hWaitingForPlayersTimer )
			{
				// Stop any timers, and bring up a new one
				HideActiveTimer();

#ifndef CSTRIKE_DLL
				variant_t sVariant;
				m_hWaitingForPlayersTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
				m_hWaitingForPlayersTimer->SetName( MAKE_STRING("zz_teamplay_waiting_timer") );
				m_hWaitingForPlayersTimer->KeyValue( "show_in_hud", "1" );
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckRestartRound( void )
{
	if( mp_clan_readyrestart.GetBool() && IsInTournamentMode() == false )
	{
		m_bAwaitingReadyRestart = true;

		for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
		{
			m_bTeamReady.Set( i, false );
		}

		const char *pszReadyString = mp_clan_ready_signal.GetString();

		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#clan_ready_rules", pszReadyString );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#clan_ready_rules", pszReadyString );

		// Don't let them put anything malicious in there
		if( pszReadyString == NULL || Q_strlen(pszReadyString) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_ready_restart" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		mp_clan_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartRoundTime = -1;
	}

	// Restart the game if specified by the server
	int iRestartDelay = mp_restartround.GetInt();
	bool bRestartGameNow = mp_restartgame_immediate.GetBool();
	if ( iRestartDelay == 0 && !bRestartGameNow )
	{
		iRestartDelay = mp_restartgame.GetInt();
	}

	if ( iRestartDelay > 0 || bRestartGameNow )
	{
		int iDelayMax = 60;

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
		if ( TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() ) )
		{
			iDelayMax = 180;
		}
#endif // #if defined(TF_CLIENT_DLL) || defined(TF_DLL)

		if ( iRestartDelay > iDelayMax )
		{
			iRestartDelay = iDelayMax;
		}

		if ( mp_restartgame.GetInt() > 0 || bRestartGameNow )
		{
			SetForceMapReset( true );
		}
		else
		{
			SetForceMapReset( false );
		}

		SetInStopWatch( false );

		if ( bRestartGameNow )
		{
			iRestartDelay = 0;
		}

		m_flRestartRoundTime = gpGlobals->curtime + iRestartDelay;

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_restart_seconds" );
		if ( event )
		{
			event->SetInt( "seconds", iRestartDelay );
			gameeventmanager->FireEvent( event );
		}

		if ( IsInTournamentMode() == false )
		{
			// let the players know
			const char *pFormat = NULL;

			if ( mp_restartgame.GetInt() > 0 )
			{
				if ( ShouldSwitchTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_switch_in_secs" : "#game_switch_in_sec";
				}
				else if ( ShouldScrambleTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_scramble_in_secs" : "#game_scramble_in_sec";

#ifdef TF_DLL
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
					if ( event )
					{
						event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
						gameeventmanager->FireEvent( event );
					}

					pFormat = NULL;
#endif
				}
			}
			else if ( mp_restartround.GetInt() > 0 )
			{
				pFormat = ( iRestartDelay > 1 ) ? "#round_restart_in_secs" : "#round_restart_in_sec";
			}

			if ( pFormat )
			{
				char strRestartDelay[64];
				Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pFormat, strRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pFormat, strRestartDelay );
			}
		}

		mp_restartround.SetValue( 0 );
		mp_restartgame.SetValue( 0 );
		mp_restartgame_immediate.SetValue( 0 );

		// cancel any ready restart in progress
		m_bAwaitingReadyRestart = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckTimeLimit( bool bAllowEnd /*= true*/ )
{
	if ( IsInPreMatch() == true )
		return false;

	if ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate )
	{
		// If there's less than 5 minutes to go, just switch now. This avoids the problem
		// of sudden death modes starting shortly after a new round starts.
		const int iMinTime = 5;
		bool bSwitchDueToTime = ( mp_timelimit.GetInt() > iMinTime && GetTimeLeft() < (iMinTime * 60) );

		if ( IsInTournamentMode() == true  )
		{
			if ( TournamentModeCanEndWithTimelimit() == false )
			{
				return false;
			}

			bSwitchDueToTime = false;
		}

		if ( IsInArenaMode() == true )
		{
			bSwitchDueToTime = false;
		}

		if ( GetTimeLeft() <= 0 || m_bChangelevelAfterStalemate || bSwitchDueToTime )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Time Limit" );
					gameeventmanager->FireEvent( event );
				}

				SendTeamScoresEvent();

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsGameUnderTimeLimit( void )
{
	return ( mp_timelimit.GetInt() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;
	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	// If the round timer is longer, let the round complete
	// TFTODO: Do we need to worry about the timelimit running our during a round?

	int iTime = (int)(flMapChangeTime - gpGlobals->curtime);
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return ( iTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckNextLevelCvar( bool bAllowEnd /*= true*/ )
{
	if ( m_bForceMapReset )
	{
		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "NextLevel CVAR" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckWinLimit( bool bAllowEnd /*= true*/ )
{
	// has one team won the specified number of rounds?
	int iWinLimit = mp_winlimit.GetInt();

	if ( iWinLimit > 0 )
	{
		for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
		{
			CTeam *pTeam = GetGlobalTeam(i);
			Assert( pTeam );

			if ( pTeam->GetScore() >= iWinLimit )
			{
				if ( bAllowEnd )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
					if ( event )
					{
						event->SetString( "reason", "Reached Win Limit" );
						gameeventmanager->FireEvent( event );
					}

					GoToIntermission();
				}
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckMaxRounds( bool bAllowEnd /*= true*/ )
{
	if ( mp_maxrounds.GetInt() > 0 && IsInPreMatch() == false )
	{
		if ( m_nRoundsPlayed >= mp_maxrounds.GetInt() )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Round Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Transition( gamerules_roundstate_t newState )
{
	m_prevState = State_Get();

	State_Leave();
	State_Enter( newState );
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter( gamerules_roundstate_t newState )
{
	m_iRoundState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flLastRoundStateChangeTime = gpGlobals->curtime;

	if ( mp_showroundtransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			Msg( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "Gamerules: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGameRulesRoundStateInfo* CTeamplayRoundBasedRules::State_LookupInfo( gamerules_roundstate_t state )
{
	static CGameRulesRoundStateInfo playerStateInfos[] =
	{
		{ GR_STATE_INIT,		"GR_STATE_INIT",		&CTeamplayRoundBasedRules::State_Enter_INIT, NULL, &CTeamplayRoundBasedRules::State_Think_INIT },
		{ GR_STATE_PREGAME,		"GR_STATE_PREGAME",		&CTeamplayRoundBasedRules::State_Enter_PREGAME, NULL, &CTeamplayRoundBasedRules::State_Think_PREGAME },
		{ GR_STATE_STARTGAME,	"GR_STATE_STARTGAME",	&CTeamplayRoundBasedRules::State_Enter_STARTGAME, NULL, &CTeamplayRoundBasedRules::State_Think_STARTGAME },
		{ GR_STATE_PREROUND,	"GR_STATE_PREROUND",	&CTeamplayRoundBasedRules::State_Enter_PREROUND, &CTeamplayRoundBasedRules::State_Leave_PREROUND, &CTeamplayRoundBasedRules::State_Think_PREROUND },
		{ GR_STATE_RND_RUNNING,	"GR_STATE_RND_RUNNING",	&CTeamplayRoundBasedRules::State_Enter_RND_RUNNING, NULL,	&CTeamplayRoundBasedRules::State_Think_RND_RUNNING },
		{ GR_STATE_TEAM_WIN,	"GR_STATE_TEAM_WIN",	&CTeamplayRoundBasedRules::State_Enter_TEAM_WIN, NULL,	&CTeamplayRoundBasedRules::State_Think_TEAM_WIN },
		{ GR_STATE_RESTART,		"GR_STATE_RESTART",		&CTeamplayRoundBasedRules::State_Enter_RESTART,	NULL, &CTeamplayRoundBasedRules::State_Think_RESTART },
		{ GR_STATE_STALEMATE,	"GR_STATE_STALEMATE",	&CTeamplayRoundBasedRules::State_Enter_STALEMATE,	&CTeamplayRoundBasedRules::State_Leave_STALEMATE, &CTeamplayRoundBasedRules::State_Think_STALEMATE },
		{ GR_STATE_GAME_OVER,	"GR_STATE_GAME_OVER",	NULL, NULL, NULL },
		{ GR_STATE_BONUS,		"GR_STATE_BONUS",		&CTeamplayRoundBasedRules::State_Enter_BONUS, &CTeamplayRoundBasedRules::State_Leave_BONUS,	&CTeamplayRoundBasedRules::State_Think_BONUS },
		{ GR_STATE_BETWEEN_RNDS, "GR_STATE_BETWEEN_RNDS", &CTeamplayRoundBasedRules::State_Enter_BETWEEN_RNDS, &CTeamplayRoundBasedRules::State_Leave_BETWEEN_RNDS,	&CTeamplayRoundBasedRules::State_Think_BETWEEN_RNDS },
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iRoundState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_INIT( void )
{
	InitTeams();
	ResetMapTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_INIT( void )
{
	State_Transition( GR_STATE_PREGAME );
}

//-----------------------------------------------------------------------------
// Purpose: The server is idle and waiting for enough players to start up again. 
//			When we find an active player go to GR_STATE_STARTGAME.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_PREGAME( void )
{
	m_flNextPeriodicThink = gpGlobals->curtime + 0.1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_PREGAME( void )
{
	CheckRespawnWaves();

	// we'll just stay in pregame for the bugbait reports
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
		return;

	// Commentary stays in this mode too
	if ( IsInCommentaryMode() )
		return;
	
	if( CountActivePlayers() > 0 || (IsInArenaMode() == true && m_flWaitingForPlayersTimeEnds == 0.0f) )
	{
		State_Transition( GR_STATE_STARTGAME );			
	}
}

//-----------------------------------------------------------------------------
// Purpose: Wait a bit and then spawn everyone into the preround
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_STARTGAME( void )
{
	m_flStateTransitionTime = gpGlobals->curtime;

	m_bInitialSpawn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_STARTGAME()
{
	if( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( !IsInTraining() && !IsInItemTestingMode() )
		{
			ConVarRef tf_bot_offline_practice( "tf_bot_offline_practice" );
			if ( mp_waitingforplayers_time.GetFloat() > 0 && tf_bot_offline_practice.GetInt() == 0 )
			{
				// go into waitingforplayers, reset at end of it
				SetInWaitingForPlayers( true );
			}
		}

		State_Transition( GR_STATE_PREROUND );
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_PREROUND( void )
{
	BalanceTeams( false );

	m_flStartBalancingTeamsAt = gpGlobals->curtime + 60.0;

	RoundRespawn();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_start" );
	if ( event )
	{
		event->SetBool( "full_reset", m_bForceMapReset );
		gameeventmanager->FireEvent( event );
	}

	if ( IsInArenaMode() == true )
	{
		if ( CountActivePlayers() > 0 )
		{
#ifndef CSTRIKE_DLL
			variant_t sVariant;
			if ( !m_hStalemateTimer )
			{
				m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
			}
			m_hStalemateTimer->KeyValue( "show_in_hud", "1" );

			sVariant.SetInt( tf_arena_preround_time.GetInt() );

			m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			m_hStalemateTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}
		}

		m_flStateTransitionTime = gpGlobals->curtime + tf_arena_preround_time.GetInt();
	}
#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	// Only allow at the very beginning of the game, or between waves in mvm
	else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && m_bAllowBetweenRounds )
	{
		State_Transition( GR_STATE_BETWEEN_RNDS );
		m_bAllowBetweenRounds = false;

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
		}
	}
#endif // #if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	else
	{
		m_flStateTransitionTime = gpGlobals->curtime + 5 * mp_enableroundwaittime.GetFloat();
	}

	StopWatchModeThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_PREROUND( void )
{
	PreRound_End();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_PREROUND( void )
{
	if( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( IsInArenaMode() == true )
		{
			if ( IsInWaitingForPlayers() == true )
			{
				if ( IsInTournamentMode() == true )
				{
					// check round restart
					CheckReadyRestart();
					State_Transition( GR_STATE_STALEMATE );
				}

				return;
			}

			State_Transition( GR_STATE_STALEMATE );	

			// hide the class composition panel
		}
		else
		{
			State_Transition( GR_STATE_RND_RUNNING );
		}
	}

	CheckRespawnWaves();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_RND_RUNNING( void )
{
	SetupOnRoundRunning();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_active" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if( !IsInWaitingForPlayers() )
	{
		PlayStartRoundVoice();
	}

	m_bChangeLevelOnRoundEnd = false;
	m_bPrevRoundWasWaitingForPlayers = false;

	m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckReadyRestart( void )
{
	// check round restart
	if ( m_flRestartRoundTime > 0 && m_flRestartRoundTime <= gpGlobals->curtime && !g_pServerBenchmark->IsBenchmarkRunning() )
	{
		m_flRestartRoundTime = -1;

#ifdef TF_DLL
		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				if ( g_pPopulationManager && TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
				{
					g_pPopulationManager->StartCurrentWave();
					m_bAllowBetweenRounds = true;
					return;
				}
			}
			else if ( TFGameRules()->IsCompetitiveMode() )
			{
				TFGameRules()->StartCompetitiveMatch();
				return;
			}
			else if ( mp_tournament.GetBool() )
			{
				// Temp
				TFGameRules()->StartCompetitiveMatch();
				return;
			}
		}
#endif // TF_DLL

		// time to restart!
		State_Transition( GR_STATE_RESTART );
	}

	bool bProcessReadyRestart = m_bAwaitingReadyRestart;

#ifdef TF_DLL
	bProcessReadyRestart &= TFGameRules() && !TFGameRules()->UsePlayerReadyStatusMode();
#endif // TF_DLL

	// check ready restart
	if ( bProcessReadyRestart )
	{
		bool bTeamNotReady = false;
		for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
		{
			if ( !m_bTeamReady[i] )
			{
				bTeamNotReady = true;
				break;
			}
		}

		if ( !bTeamNotReady )
		{
			mp_restartgame.SetValue( 5 );
			m_bAwaitingReadyRestart = false;

			ShouldResetScores( true, true );
			ShouldResetRoundsPlayed( true );
		}
	}
}

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::AreLobbyPlayersOnTeamReady( int iTeam )
{
	if ( !TFGameRules() )
		return false;

	if ( TFGameRules()->IsMannVsMachineMode() && iTeam == TF_TEAM_PVE_INVADERS )
		return true;

	bool bAtLeastOnePersonReady = false;
	
	CUtlVector<LobbyPlayerInfo_t> vecLobbyPlayers;
	GetPotentialPlayersLobbyPlayerInfo( vecLobbyPlayers );

	for ( int i = 0; i < vecLobbyPlayers.Count(); i++ )
	{
		const LobbyPlayerInfo_t &p = vecLobbyPlayers[i];
		
		// Make sure all lobby players are connected
		if ( !AreLobbyPlayersConnected() )
		{
			return false;
		}
		// All are connected, make sure their team is ready
		else if ( p.m_iTeam == iTeam )
		{
			if ( !m_bPlayerReady[ p.m_nEntNum ] )
				return false;

			// He's totally ready
			bAtLeastOnePersonReady = true;
		}
		else
		{
			// In MvM, only the red team should pass through here
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				// And you may ask yourself, "How did I get here?"
				Assert( p.m_iTeam == iTeam );
			}
		}
	}

	// We didn't find anybody who we should wait for, so
	// if at least one person is ready, then we're ready
	return bAtLeastOnePersonReady;
}

//-----------------------------------------------------------------------------
// Purpose: Is everyone in the lobby connected to the server?
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::AreLobbyPlayersConnected( void )
{
	CUtlVector<LobbyPlayerInfo_t> vecLobbyPlayers;
	GetPotentialPlayersLobbyPlayerInfo( vecLobbyPlayers );

	// If you're calling this, you should have lobby members
	Assert( vecLobbyPlayers.Count() );

	for ( int i = 0; i < vecLobbyPlayers.Count(); i++ )
	{
		const LobbyPlayerInfo_t &pLobbyPlayer = vecLobbyPlayers[i];
		if ( !pLobbyPlayer.m_bConnected || 
			 pLobbyPlayer.m_nEntNum <= 0 || 
			 pLobbyPlayer.m_nEntNum >= MAX_PLAYERS ||
			 ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pLobbyPlayer.m_iTeam == TEAM_UNASSIGNED ) )
		{
			if ( pLobbyPlayer.m_bInLobby )
				return false;
		}
	}

	return true;
}
#endif // #if defined(TF_CLIENT_DLL) || defined(TF_DLL)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_RND_RUNNING( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if( CountActivePlayers() <= 0 )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

#ifdef TF_DLL
		// Mass time-out?  Clean everything up.
		if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		{
			TFGameRules()->EndCompetitiveMatch();
			return;
		}
#endif // TF_DLL

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( m_flNextBalanceTeamsTime < gpGlobals->curtime )
	{
		BalanceTeams( true );
		m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
	}

	CheckRespawnWaves();

	// check round restart
	CheckReadyRestart();

	// See if we're coming up to the server timelimit, in which case force a stalemate immediately.
	if ( mp_timelimit.GetInt() > 0 && IsInPreMatch() == false && GetTimeLeft() <= 0 )
	{
		if ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) )
		{
			int iDrawScoreCheck = -1;
			int iWinningTeam = 0;
			bool bTeamsAreDrawn = true;
			for ( int i = FIRST_GAME_TEAM; (i < GetNumberOfTeams()) && bTeamsAreDrawn; i++ )
			{
				int iTeamScore = GetGlobalTeam(i)->GetScore();

				if ( iTeamScore > iDrawScoreCheck )
				{
					iWinningTeam = i;
				}

				if ( iTeamScore != iDrawScoreCheck )
				{
					if ( iDrawScoreCheck == -1 )
					{
						iDrawScoreCheck = iTeamScore;
					}
					else
					{
						bTeamsAreDrawn = false;
					}
				}
			}

			if ( bTeamsAreDrawn )
			{
				if ( CanGoToStalemate() )
				{
					m_bChangelevelAfterStalemate = true;
					SetStalemate( STALEMATE_SERVER_TIMELIMIT, m_bForceMapReset );
				}
				else
				{
					SetOvertime( true );
				}
			}
			else
			{
				SetWinningTeam( iWinningTeam, WINREASON_TIMELIMIT, true, false, true );
			}
		}
	}

	StopWatchModeThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_TEAM_WIN( void )
{
	m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime();

	// if we're forcing the map to reset it must be the end of a "full" round not a mini-round
	if ( m_bForceMapReset )
	{
		m_nRoundsPlayed++;
	}

	InternalHandleTeamWin( m_iWinningTeam );

	SendWinPanelInfo();

#ifdef TF_DLL
	// Do this now, so players don't leave before the usual CheckWinLimit() call happens
	bool bDone = ( CheckTimeLimit( false ) || CheckWinLimit( false ) || CheckMaxRounds( false ) || CheckNextLevelCvar( false ) );
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() && bDone )
	{
		TFGameRules()->StopCompetitiveMatch( CMsgGC_Match_Result_Status_MATCH_SUCCEEDED );
	}
#endif // TF_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_TEAM_WIN( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
#ifdef TF_DLL
		IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
#endif // TF_DLL

		bool bDone = ( CheckTimeLimit() || CheckWinLimit() || CheckMaxRounds() || CheckNextLevelCvar() );

		// check the win limit, max rounds, time limit and nextlevel cvar before starting the next round
		if ( !bDone )
		{
			PreviousRoundEnd();

			if ( ShouldGoToBonusRound() )
			{
				State_Transition( GR_STATE_BONUS );
			}
			else
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
		}
		else if ( IsInTournamentMode() )
		{
			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( !pPlayer )
					continue;

				pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
			}

			RestartTournament();

			if ( IsInArenaMode() )
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
#ifdef TF_DLL
			else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_pPopulationManager )
			{
				// one of the convars mp_timelimit, mp_winlimit, mp_maxrounds, or nextlevel has been triggered
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				g_pPopulationManager->SetMapRestartTime( gpGlobals->curtime + 10.0f );
				State_Enter( GR_STATE_GAME_OVER );
				return;
			}
			else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() )
			{
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				State_Enter( GR_STATE_GAME_OVER );
				m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime( true );
				return;
			}
#endif // TF_DLL
			else
			{
				State_Transition( GR_STATE_RND_RUNNING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_STALEMATE( void )
{
	m_flStalemateStartTime = gpGlobals->curtime;
	SetupOnStalemateStart();

	// Stop any timers, and bring up a new one
	HideActiveTimer();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
		m_hStalemateTimer = NULL;
	}

	int iTimeLimit = mp_stalemate_timelimit.GetInt();

	if ( IsInArenaMode() == true )
	{
		iTimeLimit = tf_arena_round_time.GetInt();
	}

	if ( iTimeLimit > 0 )
	{
#ifndef CSTRIKE_DLL
		variant_t sVariant;
		if ( !m_hStalemateTimer )
		{
			m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		}
		m_hStalemateTimer->KeyValue( "show_in_hud", "1" );
		sVariant.SetInt( iTimeLimit );
	
		m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_STALEMATE( void )
{
	SetupOnStalemateEnd();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
	}

	if ( IsInArenaMode() == false )
	{
		RestoreActiveTimer();

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_BONUS( void )
{
	SetupOnBonusStart();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_BONUS( void )
{
	SetupOnBonusEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_BONUS( void )
{
	BonusStateThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_BETWEEN_RNDS( void )
{
	BetweenRounds_Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_BETWEEN_RNDS( void )
{
	BetweenRounds_End();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_BETWEEN_RNDS( void )
{
	BetweenRounds_Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::HideActiveTimer( void )
{
	// We can't handle this, because we won't be able to restore multiple timers
	Assert( m_hPreviousActiveTimer.Get() == NULL );

	m_hPreviousActiveTimer = NULL;

#ifndef CSTRIKE_DLL
	CBaseEntity *pEntity = NULL;
	variant_t sVariant;
	sVariant.SetInt( false );

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" )) != NULL)
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer*>(pEntity);
		if ( pTimer && pTimer->ShowInHud() )
		{
			Assert( !m_hPreviousActiveTimer );
			m_hPreviousActiveTimer = pTimer;
			pEntity->AcceptInput( "ShowInHUD", NULL, NULL, sVariant, 0 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RestoreActiveTimer( void )
{
	if ( m_hPreviousActiveTimer )
	{
		variant_t sVariant;
		sVariant.SetInt( true );
		m_hPreviousActiveTimer->AcceptInput( "ShowInHUD", NULL, NULL, sVariant, 0 );
		m_hPreviousActiveTimer = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_STALEMATE( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if( CountActivePlayers() <= 0 && IsInArenaMode() == false )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( IsInTournamentMode() == true && IsInWaitingForPlayers() == true )
	{
		CheckReadyRestart();
		CheckRespawnWaves();
		return;
	}

	int iDeadTeam = TEAM_UNASSIGNED;
	int iAliveTeam = TEAM_UNASSIGNED;

	// If a team is fully killed, the other team has won
	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam(i);
		Assert( pTeam );

		int iPlayers = pTeam->GetNumPlayers();
		if ( iPlayers )
		{
			bool bFoundLiveOne = false;
			for ( int player = 0; player < iPlayers; player++ )
			{
				if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsAlive() )
				{
					bFoundLiveOne = true;
					break;
				}
			}

			if ( bFoundLiveOne )
			{
				iAliveTeam = i;
			}
			else
			{
				iDeadTeam = i;
			}
		}
		else
		{
			iDeadTeam = i;
		}
	}

	if ( iDeadTeam && iAliveTeam )
	{
		// The live team has won. 
		bool bMasterHandled = false;
		if ( !m_bForceMapReset )
		{
			// We're not resetting the map, so give the winners control
			// of all the points that were in play this round.
			// Find the control point master.
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster )
			{
				variant_t sVariant;
				sVariant.SetInt( iAliveTeam );
				pMaster->AcceptInput( "SetWinnerAndForceCaps", NULL, NULL, sVariant, 0 );
				bMasterHandled = true;
			}
		}

		if ( !bMasterHandled )
		{
			SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD, m_bForceMapReset );
		}
	}
	else if ( ( iDeadTeam && iAliveTeam == TEAM_UNASSIGNED ) || 
			  ( m_hStalemateTimer && TimerMayExpire() && m_hStalemateTimer->GetTimeRemaining() <= 0 ) )
	{
		bool bFullReset = true;

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

		if ( pMaster && pMaster->PlayingMiniRounds() )
		{
			// we don't need to do a full map reset for maps with mini-rounds
			bFullReset = false;
		}

		// Both teams are dead. Pure stalemate.
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bFullReset, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: manual restart
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_RESTART( void )
{
	// send scores
	SendTeamScoresEvent();

	// send restart event
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_restart_round" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	m_bPrevRoundWasWaitingForPlayers = m_bInWaitingForPlayers;
	SetInWaitingForPlayers( false );

	ResetScores();

	// reset the round time
	ResetMapTime();

	State_Transition( GR_STATE_PREROUND );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_RESTART( void )
{
	// should never get here, State_Enter_RESTART sets us into a different state
	Assert( 0 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Sorts teams by score
//-----------------------------------------------------------------------------
int TeamScoreSort( CTeam* const *pTeam1, CTeam* const *pTeam2 )
{
	if ( !*pTeam1 )
		return -1;

	if ( !*pTeam2 )
		return -1;

	if ( (*pTeam1)->GetScore() > (*pTeam2)->GetScore() )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a round winner.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	// Commentary doesn't let anyone win
	if ( IsInCommentaryMode() )
		return;

	if ( ( team != TEAM_UNASSIGNED ) && ( team <= LAST_SHARED_TEAM || team >= GetNumberOfTeams() ) )
	{
		Assert( !"SetWinningTeam() called with invalid team." );
		return;
	}

	// are we already in this state?
	if ( State_Get() == GR_STATE_TEAM_WIN )
		return;

	SetForceMapReset( bForceMapReset );
	SetSwitchTeams( bSwitchTeams );

	m_iWinningTeam = team;
	m_iWinReason = iWinReason;

	PlayWinSong( team );

	// only reward the team if they have won the map and we're going to do a full reset or the time has run out and we're changing maps
	bool bRewardTeam = bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

	if ( bDontAddScore == true )
	{
		bRewardTeam = false;
	}

	m_bUseAddScoreAnim = false;
	if ( bRewardTeam && ( team != TEAM_UNASSIGNED ) && ShouldScorePerRound() )
	{
		GetGlobalTeam( team )->AddScore( TEAMPLAY_ROUND_WIN_SCORE );
		m_bUseAddScoreAnim = true;
	}

	// this was a sudden death win if we were in stalemate then a team won it
	bool bWasSuddenDeath = ( InStalemate() && m_iWinningTeam >= FIRST_GAME_TEAM );

	State_Transition( GR_STATE_TEAM_WIN );

	m_flLastTeamWin = gpGlobals->curtime;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_win" );
	if ( event )
	{
		event->SetInt( "team", team );
		event->SetInt( "winreason", iWinReason );
		event->SetBool( "full_round", bForceMapReset );
		event->SetFloat( "round_time", gpGlobals->curtime - m_flRoundStartTime );
		event->SetBool( "was_sudden_death", bWasSuddenDeath );
		// let derived classes add more fields to the event
		FillOutTeamplayRoundWinEvent( event );
		gameeventmanager->FireEvent( event );
	}

	// send team scores
	SendTeamScoresEvent();

	if ( team == TEAM_UNASSIGNED )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_STALEMATE );
		}
	}

	// Auto scramble teams?
	if ( bForceMapReset && mp_scrambleteams_auto.GetBool() )
	{
		if ( IsInArenaMode() || IsInTournamentMode() || ShouldSkipAutoScramble() )
			return;

#ifndef DEBUG
		// Don't bother on a listen server - usually not desirable
		if ( !engine->IsDedicatedServer() )
			return;
#endif // DEBUG

		// Skip if we have a nextlevel set
		if ( !FStrEq( nextlevel.GetString(), "" ) )
			return;

		// Track the team scores
		if ( m_iWinningTeam != TEAM_UNASSIGNED )
		{
			// m_GameTeams differs from g_Teams by storing only "Real" teams
			if ( m_GameTeams.Count() == 0 )
			{
				int iTeamIndex = FIRST_GAME_TEAM;
				CTeam *pTeam;
				for ( pTeam = GetGlobalTeam(iTeamIndex); pTeam != NULL; pTeam = GetGlobalTeam(++iTeamIndex) )
				{
					m_GameTeams.Insert( iTeamIndex, 0 );
				}
			}

			// Safety net hack - we assume there are only two "Real" teams
			// driller:  need to make this work in all cases
			if ( m_GameTeams.Count() != 2 )
				return;
		}

		// Look for impending level change
		if ( ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate ) && GetTimeLeft() <= 300 )
			return;

		if ( mp_winlimit.GetInt() || mp_maxrounds.GetInt() )
		{
			int nRoundsPlayed = GetRoundsPlayed();
			if ( ( mp_maxrounds.GetInt() - nRoundsPlayed ) == 1 )
			{
				return;
			}

			int nWinLimit = mp_winlimit.GetInt();
			for ( int iIndex = m_GameTeams.FirstInorder(); iIndex != m_GameTeams.InvalidIndex(); iIndex = m_GameTeams.NextInorder( iIndex ) )
			{
				int nTeamScore = GetGlobalTeam( m_GameTeams.Key( iIndex ) )->GetScore();
				if ( nWinLimit - nTeamScore == 1 )
				{
					return;
				}
			}
		}

		// Increment win counters
		int iWinningTeamIndex = m_GameTeams.Find( m_iWinningTeam );
		if ( iWinningTeamIndex != m_GameTeams.InvalidIndex() )
		{
			m_GameTeams[iWinningTeamIndex]++;
		}
		else
		{
			Assert( iWinningTeamIndex == m_GameTeams.InvalidIndex() );
			return;
		}

		// Did we hit our win delta?
		int nWinDelta = abs( m_GameTeams[1] - m_GameTeams[0] );
		if ( nWinDelta >= mp_scrambleteams_auto_windifference.GetInt() )
		{
			// Let the server know we're going to scramble on round restart
#ifdef TF_DLL
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
			if ( event )
			{
				event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
				gameeventmanager->FireEvent( event );
			}
#else
			const char *pszMessage = "#game_scramble_onrestart";
			if ( pszMessage )
			{
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pszMessage );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pszMessage );
			}
#endif
			UTIL_LogPrintf( "World triggered \"ScrambleTeams_Auto\"\n" );

			SetScrambleTeams( true );
			ShouldResetScores( true, false );
			ShouldResetRoundsPlayed( false );
		}

		// If we switch teams after this win, swap scores
		if ( ShouldSwitchTeams() )
		{
			int nTempScore = m_GameTeams[0];
			m_GameTeams[0] = m_GameTeams[1];
			m_GameTeams[1] = nTempScore;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a stalemate
//			Most often a team_control_point_master saying that the
//			round timer expired 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetStalemate( int iReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false */ )
{
	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
		return;

	if ( !mp_stalemate_enable.GetBool() )
	{
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bForceMapReset, bSwitchTeams );
		return;
	}

	if ( InStalemate() )
		return;

	SetForceMapReset( bForceMapReset );

	m_iWinningTeam = TEAM_UNASSIGNED;

	PlaySuddenDeathSong();

	State_Transition( GR_STATE_STALEMATE );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
	if ( event )
	{
		event->SetInt( "reason", iReason );
		gameeventmanager->FireEvent( event );
	}
}

#ifdef GAME_DLL
void CC_CH_ForceRespawn( void )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		pRules->RespawnPlayers( true );
	}
}
static ConCommand mp_forcerespawnplayers("mp_forcerespawnplayers", CC_CH_ForceRespawn, "Force all players to respawn.", FCVAR_CHEAT );

static ConVar mp_tournament_allow_non_admin_restart( "mp_tournament_allow_non_admin_restart", "1", FCVAR_NONE, "Allow mp_tournament_restart command to be issued by players other than admin.");
void CC_CH_TournamentRestart( void )
{
	if ( mp_tournament_allow_non_admin_restart.GetBool() == false )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;
	}

#ifdef TF_DLL
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		return;
#endif // TF_DLL

	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		pRules->RestartTournament();
	}
}
static ConCommand mp_tournament_restart("mp_tournament_restart", CC_CH_TournamentRestart, "Restart Tournament Mode on the current level."  );

void CTeamplayRoundBasedRules::RestartTournament( void )
{
	if ( IsInTournamentMode() == false )
		return;

	SetInWaitingForPlayers( true );
	m_bAwaitingReadyRestart = true;
	m_flStopWatchTotalTime = -1.0f;
	m_bStopWatch = false;

	// we might have had a stalemate during the last round
	// so reset this bool each time we restart the tournament
	m_bChangelevelAfterStalemate = false;

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}
}

#endif


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bForceRespawn - respawn player even if dead or dying
//			bTeam - if true, only respawn the passed team
//			iTeam  - team to respawn
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RespawnPlayers( bool bForceRespawn, bool bTeam /* = false */, int iTeam/* = TEAM_UNASSIGNED */ )
{
	if ( bTeam )
	{
		Assert( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() );
	}	

	int iPlayersSpawned = 0;

	CBasePlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		// Check for team specific spawn
		if ( bTeam && pPlayer->GetTeamNumber() != iTeam )
			continue;

		// players that haven't chosen a team/class can never spawn
		if ( !pPlayer->IsReadyToPlay() )
		{
			// Let the player spawn immediately when they do pick a class
			if ( pPlayer->ShouldGainInstantSpawn() )
			{
				pPlayer->AllowInstantSpawn();
			}

			continue;
		}

		// If we aren't force respawning, don't respawn players that:
		// - are alive
		// - are still in the death anim stage of dying
		if ( !bForceRespawn )
		{
			if ( pPlayer->IsAlive() )
				continue; 

			if ( m_iRoundState != GR_STATE_PREROUND )
			{
					// If the player hasn't been dead the minimum respawn time, he
				// waits until the next wave.
				if ( bTeam && !HasPassedMinRespawnTime( pPlayer ) )
					continue;
				
				if ( !pPlayer->IsReadyToSpawn() )
				{
					// Let the player spawn immediately when they do pick a class
					if ( pPlayer->ShouldGainInstantSpawn() )
					{
						pPlayer->AllowInstantSpawn();
					}

					continue;
				}

			
			}
		}

		// Respawn this player
		pPlayer->ForceRespawn();
		iPlayersSpawned++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::InitTeams( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::CountActivePlayers( void )
{
	int i;
	int count = 0;
	CBasePlayer *pPlayer;

	for (i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if( pPlayer->IsReadyToPlay() )
			{
				count++;
			}
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::HandleTimeLimitChange( void )
{
	// check that we have an active timer in the HUD and use mp_timelimit if we don't
	if ( !MapHasActiveTimer() && ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0  ) )
	{
		CreateTimeLimitTimer();
	}
	else
	{
		if ( m_hTimeLimitTimer )
		{
			UTIL_Remove( m_hTimeLimitTimer );
			m_hTimeLimitTimer = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetPlayerAndTeamReadyState( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}

#ifdef GAME_DLL
	// Note <= MAX_PLAYERS vs < MAX_PLAYERS above
	for ( int i = 0; i <= MAX_PLAYERS; i++ )
	{
		m_bPlayerReadyBefore[i] = false;
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::MapHasActiveTimer( void )
{
#ifndef CSTRIKE_DLL
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" ) ) != NULL )
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer*>( pEntity );
		if ( pTimer && pTimer->ShowInHud() && ( Q_stricmp( STRING( pTimer->GetEntityName() ), "zz_teamplay_timelimit_timer" ) != 0 ) )
		{
			return true;
		}
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CreateTimeLimitTimer( void )
{
	if ( IsInArenaMode () == true || IsInKothMode() == true )
		return;

	// this is the same check we use in State_Think_RND_RUNNING()
	// don't show the timelimit timer if we're not going to end the map when it runs out
	bool bAllowStalemate = ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) );
	if ( !bAllowStalemate )
		return;

#ifndef CSTRIKE_DLL
	if ( !m_hTimeLimitTimer )
	{
		m_hTimeLimitTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		m_hTimeLimitTimer->SetName( MAKE_STRING( "zz_teamplay_timelimit_timer" ) );
	}

	variant_t sVariant;
	m_hTimeLimitTimer->KeyValue( "show_in_hud", "1" );
	sVariant.SetInt( GetTimeLeft() );
	m_hTimeLimitTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RoundRespawn( void )
{
	m_flRoundStartTime = gpGlobals->curtime;

	if ( m_bForceMapReset || m_bPrevRoundWasWaitingForPlayers )
	{
		CleanUpMap();

		// clear out the previously played rounds
		m_iszPreviousRounds.RemoveAll();

		if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0  )
		{
			// check that we have an active timer in the HUD and use mp_timelimit if we don't
			if ( !MapHasActiveTimer() )
			{
				CreateTimeLimitTimer();
			}
		}

		m_iLastCapPointChanged = 0;
	}

	// reset our spawn times to the original values
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		if ( m_flOriginalTeamRespawnWaveTime[i] >= 0 )
		{
			m_TeamRespawnWaveTimes.Set( i, m_flOriginalTeamRespawnWaveTime[i] );
		}
	}

	if ( !IsInWaitingForPlayers() )
	{
		if ( m_bForceMapReset )
		{
			UTIL_LogPrintf( "World triggered \"Round_Start\"\n" );
		}
	}
	
	// Setup before respawning players, so we can mess with spawnpoints
	SetupOnRoundStart();

	// Do we need to switch the teams?
	m_bSwitchedTeamsThisRound = false;
	if ( ShouldSwitchTeams() )
	{
		m_bSwitchedTeamsThisRound = true;
		HandleSwitchTeams();
		SetSwitchTeams( false );
	}

	// Do we need to switch the teams?
	if ( ShouldScrambleTeams() )
	{
		HandleScrambleTeams();
		SetScrambleTeams( false );
	}

#if defined( REPLAY_ENABLED )
	bool bShouldWaitToStartRecording = ShouldWaitToStartRecording();
	if ( g_pReplay && g_pReplay->SV_ShouldBeginRecording( bShouldWaitToStartRecording ) )
	{
		// Tell the replay manager that it should begin recording the new round as soon as possible
		g_pReplay->SV_GetContext()->GetSessionRecorder()->StartRecording();
	}
#endif

    // Free any edicts that were marked deleted. This should hopefully clear some out
    //  so the below function can use the now freed ones.
	engine->AllowImmediateEdictReuse();

	RespawnPlayers( true );

	// reset per-round scores for each player
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->ResetPerRoundStats();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recreate all the map entities from the map data (preserving their indices),
//			then remove everything else except the players.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CleanUpMap()
{
	if( mp_showcleanedupents.GetInt() )
	{
		Msg( "CleanUpMap\n===============\n" );
		Msg( "  Entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );
	}

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		if ( !RoundCleanupShouldIgnore( pCur ) )
		{
			if( mp_showcleanedupents.GetInt() & 1 )
			{
				Msg( "Removed Entity: %s\n", pCur->GetClassname() );
			}
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Clear out the event queue
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	engine->AllowImmediateEdictReuse();

	if ( mp_showcleanedupents.GetInt() & 2 )
	{
		Msg( "  Entities Left:\n" );
		pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			Msg( "  %s (%d)\n", pCur->GetClassname(), pCur->entindex() );
			pCur = gEntList.NextEnt( pCur );
		}
	}

	// Now reload the map entities.
	class CTeamplayMapEntityFilter : public IMapEntityFilter
	{
	public:
		CTeamplayMapEntityFilter()
		{
			m_pRules = assert_cast<CTeamplayRoundBasedRules*>( GameRules() );
		}

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( m_pRules->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
		CTeamplayRoundBasedRules *m_pRules;
	};
	CTeamplayMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::ShouldCreateEntity( const char *pszClassName )
{
	return !FindInList( s_PreserveEnts, pszClassName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	return FindInList( s_PreserveEnts, pEnt->GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected ( user ID )
//-----------------------------------------------------------------------------
static int SwitchPlayersSort(  CBaseMultiplayerPlayer * const *p1, CBaseMultiplayerPlayer * const *p2 )
{
	// sort by score
	return ( (*p2)->GetTeamBalanceScore() - (*p1)->GetTeamBalanceScore() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckRespawnWaves( void )
{
	for ( int team = LAST_SHARED_TEAM+1; team < GetNumberOfTeams(); team++ )
	{
		if ( m_flNextRespawnWave[team] && m_flNextRespawnWave[team] > gpGlobals->curtime )
			continue;

		RespawnTeam( team );

		// Set m_flNextRespawnWave to 0 when we don't have a respawn time to reduce networking
		float flNextRespawnLength = GetRespawnWaveMaxLength( team );
		if ( flNextRespawnLength )
		{
			m_flNextRespawnWave.Set( team, gpGlobals->curtime + flNextRespawnLength );
		}
		else
		{
			m_flNextRespawnWave.Set( team, 0.0f );
		}

	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the teams are balanced after this function
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( mp_autoteambalance.GetBool() == false || ( IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true ) )
	{
		return;
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return;
#endif // _DEBUG || STAGING_ONLY

	if ( IsInTraining() || IsInItemTestingMode() )
	{
		return;
	}

	// we don't balance for a period of time at the start of the game
	if ( gpGlobals->curtime < m_flStartBalancingTeamsAt )
	{
		return;
	}

	// wrap with this bool, indicates it's a round running switch and not a between rounds insta-switch
	if ( bRequireSwitcheesToBeDead )
	{
#ifndef CSTRIKE_DLL
		// we don't balance if there is less than 60 seconds on the active timer
		CTeamRoundTimer *pActiveTimer = GetActiveRoundTimer();
		if ( pActiveTimer && pActiveTimer->GetTimeRemaining() < 60 )
		{
			return;
		}
#endif
	}

	int iHeaviestTeam = TEAM_UNASSIGNED, iLightestTeam = TEAM_UNASSIGNED;

	// Figure out if we're unbalanced
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
	{
		m_flFoundUnbalancedTeamsTime = -1;
		m_bPrintedUnbalanceWarning = false;
		return;
	}

	if ( m_flFoundUnbalancedTeamsTime < 0 )
	{
		m_flFoundUnbalancedTeamsTime = gpGlobals->curtime;
	}

	// if teams have been unbalanced for X seconds, play a warning 
	if ( !m_bPrintedUnbalanceWarning && ( ( gpGlobals->curtime - m_flFoundUnbalancedTeamsTime ) > 1.0 ) )
	{
		// print unbalance warning
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_auto_team_balance_in", "5" );
		m_bPrintedUnbalanceWarning = true;
	}

	// teams are unblanced, figure out some players that need to be switched

	CTeam *pHeavyTeam = GetGlobalTeam( iHeaviestTeam );
	CTeam *pLightTeam = GetGlobalTeam( iLightestTeam );

	Assert( pHeavyTeam && pLightTeam );

	int iNumSwitchesRequired = ( pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers() ) / 2;

	// sort the eligible players and switch the n best candidates
	CUtlVector<CBaseMultiplayerPlayer *> vecPlayers;

	CBaseMultiplayerPlayer *pPlayer;

	int iScore;

	int i;
	for ( i = 0; i < pHeavyTeam->GetNumPlayers(); i++ )
	{
		pPlayer = ToBaseMultiplayerPlayer( pHeavyTeam->GetPlayer(i) );

		if ( !pPlayer )
			continue;

		if ( !pPlayer->CanBeAutobalanced() )
			continue;

		// calculate a score for this player. higher is more likely to be switched
		iScore = pPlayer->CalculateTeamBalanceScore();

		pPlayer->SetTeamBalanceScore( iScore );

		vecPlayers.AddToTail( pPlayer );
	}

	// sort the vector
	vecPlayers.Sort( SwitchPlayersSort );

	int iNumEligibleSwitchees = iNumSwitchesRequired + 2;

	for ( int i=0; i<vecPlayers.Count() && iNumSwitchesRequired > 0 && i < iNumEligibleSwitchees; i++ )
	{
		pPlayer = vecPlayers.Element(i);

		Assert( pPlayer );

		if ( !pPlayer )
			continue;

		if ( bRequireSwitcheesToBeDead == false || !pPlayer->IsAlive() )
		{
			// We're trying to avoid picking a player that's recently
			// been auto-balanced by delaying their selection in the hope
			// that a better candidate comes along.
			if ( bRequireSwitcheesToBeDead )
			{
				int nPlayerTeamBalanceScore = pPlayer->CalculateTeamBalanceScore();

				// Do we already have someone in the queue?
				if ( m_nAutoBalanceQueuePlayerIndex > 0 )
				{
					// Is this player's score worse?
					if ( nPlayerTeamBalanceScore < m_nAutoBalanceQueuePlayerScore )
					{
						m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
						m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					}
				}
				// Has this person been switched recently?
				else if ( nPlayerTeamBalanceScore < -10000 ) 
				{
					// Put them in the queue
					m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
					m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					m_flAutoBalanceQueueTimeEnd = gpGlobals->curtime + 3.0f;

					continue;
				}

				// If this is the player in the queue...
				if ( m_nAutoBalanceQueuePlayerIndex == pPlayer->entindex() )
				{
					// Pass until their timer is up
					if ( m_flAutoBalanceQueueTimeEnd > gpGlobals->curtime )
						continue;
				}
			}
				
			pPlayer->ChangeTeam( iLightestTeam );
			pPlayer->SetLastForcedChangeTeamTimeToNow();

			m_nAutoBalanceQueuePlayerScore = -1;
			m_nAutoBalanceQueuePlayerIndex = -1;

			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if ( event )
			{
				event->SetInt( "player", pPlayer->entindex() );
				event->SetInt( "team", iLightestTeam );
				gameeventmanager->FireEvent( event );
			}

			// tell people that we've switched this player
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pPlayer->GetPlayerName() );

			iNumSwitchesRequired--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetScores( void )
{
	if ( m_bResetTeamScores )
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			GetGlobalTeam( i )->ResetScores();
		}
	}

	if ( m_bResetPlayerScores )
	{
		CBasePlayer *pPlayer;

		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

			if (pPlayer == NULL)
				continue;

			if (FNullEnt( pPlayer->edict() ))
				continue;

			pPlayer->ResetScores();
		}
	}

	if ( m_bResetRoundsPlayed )
	{
		m_nRoundsPlayed = 0;
	}

	// assume we always want to reset the scores 
	// unless someone tells us not to for the next reset 
	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	//m_flStopWatchTime = -1.0f;

#ifdef TF_DLL
	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_reset" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
#endif // TF_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetMapTime( void )
{
	m_flMapResetTime = gpGlobals->curtime;

	// send an event with the time remaining until map change
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_map_time_remaining" );
	if ( event )
	{
		event->SetInt( "seconds", GetTimeLeft() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayStartRoundVoice( void )
{
	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, UTIL_VarArgs("Game.TeamRoundStart%d", i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayWinSong( int team )
{
	if ( team == TEAM_UNASSIGNED )
	{
		PlayStalemateSong();
	}
	else
	{
#if defined (TF_DLL) || defined (TF_CLIENT_DLL)
		if ( TFGameRules() && TFGameRules()->IsPlayingSpecialDeliveryMode() )
			return;
#endif // TF_DLL

		BroadcastSound( TEAM_UNASSIGNED, UTIL_VarArgs("Game.TeamWin%d", team ) );

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			if ( i == team )
			{
				BroadcastSound( i, WinSongName( i ) );
			}
			else
			{
				const char *pchLoseSong = LoseSongName( i );
				if ( pchLoseSong )
				{
					BroadcastSound( i, pchLoseSong );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlaySuddenDeathSong( void )
{
	BroadcastSound( TEAM_UNASSIGNED, "Game.SuddenDeath" );

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, "Game.SuddenDeath" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayStalemateSong( void )
{
	BroadcastSound( TEAM_UNASSIGNED, GetStalemateSong( TEAM_UNASSIGNED ) );

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, GetStalemateSong( i ) );
	}
}

bool CTeamplayRoundBasedRules::PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext )
{
	if ( m_flNewThrottledAlertTime <= gpGlobals->curtime )
	{
		BroadcastSound( iTeam, sound );
		m_flNewThrottledAlertTime = gpGlobals->curtime + fDelayBeforeNext;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags )
{
	//send it to everyone
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::AddPlayedRound( string_t strName )
{
	if ( strName != NULL_STRING )
	{
		m_iszPreviousRounds.AddToHead( strName );

		// we only need to store the last two rounds that we've played
		if ( m_iszPreviousRounds.Count() > 2 )
		{
			// remove all but two of the entries (should only ever have to remove 1 when we're at 3)
			for ( int i = m_iszPreviousRounds.Count() - 1 ; i > 1 ; i-- )
			{
				m_iszPreviousRounds.Remove( i );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsPreviouslyPlayedRound( string_t strName )
{
	return ( m_iszPreviousRounds.Find( strName ) != m_iszPreviousRounds.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CTeamplayRoundBasedRules::GetLastPlayedRound( void )
{
	return ( m_iszPreviousRounds.Count() ? m_iszPreviousRounds[0] : NULL_STRING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamRoundTimer *CTeamplayRoundBasedRules::GetActiveRoundTimer( void )
{
#ifdef TF_DLL
	int iTimerEntIndex = ObjectiveResource()->GetTimerInHUD();
	return ( dynamic_cast<CTeamRoundTimer *>( UTIL_EntityByIndex( iTimerEntIndex ) ) );
#else
	return NULL;
#endif
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: How long are the respawn waves for this team currently?
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers /* = true */ )
{
	if ( State_Get() != GR_STATE_RND_RUNNING )
		return 0;

	if ( mp_disable_respawn_times.GetBool() == true )
		return 0.0f;

	//Let's just turn off respawn times while players are messing around waiting for the tournament to start
	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
		return 0.0f;

	float flTime = ( ( m_TeamRespawnWaveTimes[iTeam] >= 0 ) ? m_TeamRespawnWaveTimes[iTeam] : mp_respawnwavetime.GetFloat() );

	// For long respawn times, scale the time as the number of players drops
	if ( bScaleWithNumPlayers && flTime > 5 )
	{
		flTime = MAX( 5, flTime * GetRespawnTimeScalar(iTeam) );
	}

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running tournament mode
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsInTournamentMode( void )
{
	return mp_tournament.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running highlander mode
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsInHighlanderMode( void )
{
#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	// can't use highlander mode and the queue system
	if ( IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
		return false;

	return mp_highlander.GetBool();
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::GetBonusRoundTime( bool bFinal /*= false*/ )
{
	return bFinal ? mp_bonusroundtime_final.GetInt() : Max( 5, mp_bonusroundtime.GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we should even bother to do balancing stuff
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::ShouldBalanceTeams( void )
{
	if ( IsInTournamentMode() == true )
		return false;

	if ( IsInTraining() == true || IsInItemTestingMode() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the passed team change would cause unbalanced teams
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam  )
{
	// players are allowed to change to their own team
	if( iNewTeam == iCurrentTeam )
		return false;

	// if mp_teams_unbalance_limit is 0, don't check
	if ( ShouldBalanceTeams() == false )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	// if they are joining a non-playing team, allow
	if ( iNewTeam < FIRST_GAME_TEAM )
		return false;

	CTeam *pNewTeam = GetGlobalTeam( iNewTeam );

	if ( !pNewTeam )
	{
		Assert( 0 );
		return true;
	}

	// add one because we're joining this team
	int iNewTeamPlayers = pNewTeam->GetNumPlayers() + 1;

	// for each game team
	int i = FIRST_GAME_TEAM;

	CTeam *pTeam;

	for ( pTeam = GetGlobalTeam(i); pTeam != NULL; pTeam = GetGlobalTeam(++i) )
	{
		if ( pTeam == pNewTeam )
			continue;

		int iNumPlayers = pTeam->GetNumPlayers();

		if ( i == iCurrentTeam )
		{
			iNumPlayers = MAX( 0, iNumPlayers-1 );
		}

		if ( ( iNewTeamPlayers - iNumPlayers ) > mp_teams_unbalance_limit.GetInt() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam )
{
	if ( IsInArenaMode() == false || (IsInArenaMode() && tf_arena_use_queue.GetBool() == false) )
	{
		if ( ShouldBalanceTeams() == false )
		{
			return false;
		}
	}

#ifndef CLIENT_DLL
	if ( IsInCommentaryMode() )
		return false;
#endif

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam(i); pTeam != NULL; pTeam = GetGlobalTeam(++i) )
	{
		int iNumPlayers = pTeam->GetNumPlayers();

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;	
		}
	}

	if ( IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
	{
		if ( iMostPlayers == 0 && iMostPlayers == iLeastPlayers )
			return true;

		if ( iMostPlayers != iLeastPlayers )
			return true;
		
		return false;
	}

	if ( ( iMostPlayers - iLeastPlayers ) > mp_teams_unbalance_limit.GetInt() )
	{
		return true;
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetRoundState( int iRoundState )
{
	m_iRoundState = iRoundState;
	m_flLastRoundStateChangeTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_bOldInWaitingForPlayers = m_bInWaitingForPlayers;
	m_bOldInOvertime = m_bInOvertime;
	m_bOldInSetup = m_bInSetup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED || 
		 m_bOldInWaitingForPlayers != m_bInWaitingForPlayers ||
		 m_bOldInOvertime != m_bInOvertime ||
		 m_bOldInSetup != m_bInSetup )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( State_Get() == GR_STATE_STALEMATE )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
			if ( event )
			{
				event->SetInt( "reason", STALEMATE_JOIN_MID );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}

	if ( m_bInOvertime && ( m_bOldInOvertime != m_bInOvertime ) )
	{
		HandleOvertimeBegin();
	}
}
#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetTeamsRoundWinTracking( void )
{
	if ( m_GameTeams.Count() != 2 )
		return;

	m_GameTeams[0] = 0;
	m_GameTeams[1] = 0;
}
#endif // GAME_DLL

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
//-----------------------------------------------------------------------------
// Purpose: Are you now, or are you ever going to be, a member of the defending party?
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::GetPotentialPlayersLobbyPlayerInfo( CUtlVector<LobbyPlayerInfo_t> &vecLobbyPlayers, bool bIncludeBots /*= false*/ )
{
	GetAllPlayersLobbyInfo( vecLobbyPlayers, bIncludeBots );

	// Now scan through and remove the spectators
	for ( int i = vecLobbyPlayers.Count() - 1; i >= 0; --i )
	{
		switch ( vecLobbyPlayers[i].m_iTeam )
		{
			case TEAM_UNASSIGNED:
			case TF_TEAM_RED:
				break;

			case TF_TEAM_BLUE:
				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
					vecLobbyPlayers.FastRemove( i );
				break;

			case TEAM_SPECTATOR:
				vecLobbyPlayers.FastRemove( i );
				break;

			default:
				AssertMsg1( false, "Bogus team %d", vecLobbyPlayers[i].m_iTeam );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::GetAllPlayersLobbyInfo( CUtlVector<LobbyPlayerInfo_t> &vecPlayers, bool bIncludeBots )
{
	vecPlayers.RemoveAll();

	// Locate the lobby
	CTFLobby *pLobby = GTFGCClientSystem()->GetLobby();
	if ( pLobby )
	{
		for ( int i = 0 ; i < pLobby->GetNumMembers() ; ++i )
		{
			LobbyPlayerInfo_t &mbr = vecPlayers[vecPlayers.AddToTail()];
			mbr.m_nEntNum = 0; // assume he isn't in the game yet
			mbr.m_sPlayerName = pLobby->GetMemberDetails( i )->name().c_str();
			mbr.m_steamID = pLobby->GetMember( i );
			mbr.m_iTeam = TEAM_UNASSIGNED;
			mbr.m_bConnected = false;
			mbr.m_bBot = false;
			mbr.m_bInLobby = true;
			mbr.m_bSquadSurplus = pLobby->GetMemberDetails( i )->squad_surplus();
		}
	}

	// Scan all players
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{

		// Locate the info for this player, depending on whether
		// we're on the server or client
		#ifdef CLIENT_DLL
			player_info_t pi;
			if ( !engine->GetPlayerInfo( i, &pi ) )
				continue;
			if ( pi.ishltv || pi.isreplay )
				continue;
			bool bBot = pi.fakeplayer;
		#else
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
				continue;
			bool bBot = pPlayer->IsBot();
		#endif

		// Discard bots?
		if ( bBot && !bIncludeBots )
			continue;

		// See if we already found him in the lobby
		CSteamID steamID = GetSteamIDForPlayerIndex( i );
		#ifdef GAME_DLL
			CSteamID steamID2;
			if ( pPlayer->GetSteamID( &steamID2 ) )
			{
				Assert( steamID == steamID2 );
			}
		#endif
		LobbyPlayerInfo_t *mbr = NULL;
		if ( steamID.IsValid() )
		{
			for ( int j = 0 ; j < vecPlayers.Count() ; ++j )
			{
				if ( vecPlayers[j].m_steamID == steamID )
				{
					Assert( mbr == NULL );
					mbr = &vecPlayers[j];
					#ifndef _DEBUG
						break; // in debug, keep looking so the assert above can fire
					#endif
				}
			}
		}

		// Create a new entry for him if we didn't already find one
		if ( mbr == NULL )
		{
			mbr = &vecPlayers[vecPlayers.AddToTail()];
			mbr->m_bInLobby = false;
			mbr->m_steamID = steamID;
			mbr->m_bSquadSurplus = false;
		}

		// Fill in the rest of the info
		mbr->m_bBot = bBot;
		mbr->m_nEntNum = i;
		#ifdef CLIENT_DLL
			mbr->m_sPlayerName = g_PR->GetPlayerName( i );
			mbr->m_iTeam = g_PR->GetTeam( i );
			mbr->m_bConnected = g_PR->IsConnected( i );
		#else
			mbr->m_sPlayerName = pPlayer->GetPlayerName();
			mbr->m_iTeam = pPlayer->GetTeamNumber();
			mbr->m_bConnected = pPlayer->IsConnected();
		#endif
	}
}

#endif // #if defined(TF_CLIENT_DLL) || defined(TF_DLL)
