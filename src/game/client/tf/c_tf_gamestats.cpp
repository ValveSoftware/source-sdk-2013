//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side stat tracking
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_gamestats.h"
#include <vgui_controls/Controls.h>
#include "vgui/ISystem.h"
#include "steam/steam_api.h"
#include "steamworks_gamestats.h"
#include "c_tf_player.h"
#include "tf_hud_statpanel.h"
#include "econ_item_system.h"
#include "econ_ui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tf_stats_track( "tf_stats_track",
#ifdef _DEBUG
							 "0",
#else
							 "1",
#endif
							 FCVAR_DEVELOPMENTONLY, "Turn on//off tf stats tracking." );

ConVar tf_matchmaking_ogs_odds( "tf_matchmaking_ogs_odds", "0.05", FCVAR_HIDDEN, "Percentage (0...1) of quickplay queries that will report to OGS" );

const char *g_ItemEventNames[] =
{
	// STORE EVENTS
	"store_entered",
	"store_exited",
	"store_tab_changed",
	"store_item_selected",
	"store_item_previewed",
	"store_item_added_to_cart",
	"store_item_removed_from_cart",
	"store_checkout_attempt",
	"store_checkout_failure",
	"store_checkout_success",
	"store_checkout_item",

	// LOADOUT EVENTS
	"loadout_entered",
	"loadout_exited",

	// TRADE EVENTS
	"trading_entered",
	"trading_exited",
	"trading_went_to_armory",
	"trading_returned_from_armory",
	"trading_request_sent",
	"trading_request_received",
	"trading_request_rejected",
	"trading_request_accepted",
	"trading_trade_negotiated",
	"trading_trade_success",
	"trading_trade_failure",
	"trading_item_given",
	"trading_item_received",
	"trading_item_gifted",

	// CRAFTING EVENTS
	"crafting_entered",
	"crafting_exited",
	"crafting_went_to_armory",
	"crafting_returned_from_armory",
	"crafting_view_blueprints",
	"crafting_timeout",
	"crafting_failure",
	"crafting_success",
	"crafting_no_recipe_match",
	"crafting_attempt",
	"crafting_recipe_found",

	// ARMORY EVENTS
	"armory_entered",
	"armory_exited",
	"armory_select_item",
	"armory_browse_wiki",
	"armory_change_filter",

	// GENERAL EVENTS
	"item_received",
	"item_discarded",
	"item_deleted",
	"item_used_tool",
	"item_used_consumable",
	"item_removed_attrib",
	"item_changed_style"

	// NEW STORE EVENTS
	"store2_entered",	// This gets written *in addition* to IE_STORE_ENTERED

	// THESE STORED AS INTEGERS IN THE DATABASE SO THESE ARE NEW
	"item_reset_counters",
	"item_put_into_collection",

	""	// IE_COUNT
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_ItemEventNames ) == IE_COUNT );

C_CTFGameStats C_CTF_GameStats;

static bool WeaponInfoLessFunc( const int& e1, const int&e2 )
{
	return e1 < e2;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
C_CTFGameStats::C_CTFGameStats()
{
	m_ulExperimentValue = (uint64) ~0;

	gamestats = this;
	Clear();	

	m_mapWeaponInfo.SetLessFunc( WeaponInfoLessFunc );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
C_CTFGameStats::~C_CTFGameStats()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets all game stats to their default value
// Input  :  - 
//-----------------------------------------------------------------------------
void C_CTFGameStats::Clear( void )
{
	V_strncpy( m_szCountryCode, "unknown", ARRAYSIZE( m_szCountryCode ) );
	V_strncpy( m_szAudioLanguage, "unknown", ARRAYSIZE( m_szAudioLanguage ) );
	V_strncpy( m_szTextLanguage, "unknown", ARRAYSIZE( m_szTextLanguage ) );

	m_bRoundActive = false;
	m_bIsDisconnecting = false;
}

//-----------------------------------------------------------------------------
// Purpose: Adds our data to the gamestats data that gets uploaded.
//			Returns true if we added data, false if we didn't
//-----------------------------------------------------------------------------
bool C_CTFGameStats::AddDataForSend( KeyValues *pKV, StatSendType_t sendType )
{
	// we only have data to send at level shutdown
	if ( sendType != STATSEND_APPSHUTDOWN || 0 == tf_stats_track.GetInt() )
	{
		return false;
	}

	KeyValues *pKVData = new KeyValues( "tf_configdata" );

	if ( NULL == pKVData )
	{
		Clear();
		return false;
	}

	static ConVarRef sb_quick_list_bit_field( "sb_quick_list_bit_field" );
	if ( sb_quick_list_bit_field.IsValid() )
	{
		pKVData->SetInt( "QuickListBitField", sb_quick_list_bit_field.GetInt() );
	}
	else
	{
		pKVData->SetInt( "QuickListBitField", -1 );
	}
	pKVData->SetString( "TextLanguage", m_szTextLanguage );
	pKVData->SetString( "AudioLanguage", m_szAudioLanguage );
	pKVData->SetString( "CountryCode", m_szCountryCode );

	// Add our tf_configdata as a subkey to the main stat key
	pKV->AddSubKey( pKVData );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Since this is a very lean client stat reporter, we can grab all the info
//			that we need right in the init call
//-----------------------------------------------------------------------------
bool C_CTFGameStats::Init( void )
{
	// If we are on the PC and have access to all the interfaces we need
	if ( !IsX360() && engine && vgui::system() && steamapicontext && steamapicontext->SteamUtils() )
	{
		// We want to track the country code to help with localization. The countrycode is empty when testing on SteamBeta, so we won't get
		// data until users in the wild play
		const char * countryCode = steamapicontext->SteamUtils()->GetIPCountry();
		if ( countryCode != NULL )
		{
			V_strncpy( m_szCountryCode, countryCode, ARRAYSIZE( m_szCountryCode ) );
		}

		// Now lets get the text language that Steam is in (If the game supports the language, then the UI is changed to that language).
		engine->GetUILanguage( m_szTextLanguage, sizeof( m_szTextLanguage ) );
		
		V_strcpy_safe( m_szAudioLanguage, steamapicontext->SteamApps()->GetCurrentGameLanguage() );

		m_currentSession.m_SessionStart = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();

		ListenForGameEvent( "server_spawn" );
//		ListenForGameEvent( "host_quit" );
		ListenForGameEvent( "player_stats_updated" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "player_changeclass" );
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "client_disconnect" );

		// A client session lasts from when the application starts to when it is exited.
		GetSteamWorksSGameStatsUploader().StartSession();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: This system is shutting down.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Shutdown()
{
	if ( !steamapicontext )
		return;
	
	if ( !steamapicontext->SteamUser() )
		return;

	SW_GameStats_WriteClientSessionSummary();
	SW_GameStats_WriteClientWeapons();

	GetSteamWorksSGameStatsUploader().EndSession();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_LevelInit( void )
{
	m_currentMap.Init( engine->GetLevelName(), engine->GetLevelVersion(), 0, 0, GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	m_currentRound.m_iNumRounds = 0;
	m_currentSession.m_iMapsPlayed++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_LevelShutdown( float flElapsed )
{
	if ( !m_bIsDisconnecting )
	{
		SW_GameStats_WriteClientRound( 0, 0, RE_SERVER_MAP_CHANGE );
	}

	SW_GameStats_WriteClientMap();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the server shuts down or the client disconnects from the server.
//-----------------------------------------------------------------------------
void C_CTFGameStats::ClientDisconnect( int iReason )
{
	m_bIsDisconnecting = true;
	SW_GameStats_WriteClientRound( 0, 0, iReason );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CTFGameStats::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();
	if ( FStrEq( "server_spawn", pEventName ) )
	{
		if ( m_currentSession.m_FirstConnect == 0 )
		{
			m_currentSession.m_FirstConnect = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
		}

		// In case we join a round that is active, stuff the current time as the round start time.
		m_currentRound.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
		m_currentMap.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();

		m_bIsDisconnecting = false;
	}
	else if ( FStrEq( "host_quit", pEventName ) )
	{
		// We need this event because client_disconnect is called after the level unloads
		// when we're dealing with the client-host of a listen server. Frustrating exception to
		// the normal disconnect/shutdown rules.
		ClientDisconnect( RE_CLIENT_QUIT );
	}
	else if ( FStrEq( "client_disconnect", pEventName ) )
	{
		ClientDisconnect( RE_CLIENT_DISCONNECT );
	}
	else if ( FStrEq( "teamplay_round_win", pEventName ) )
	{
		int winningTeam = event->GetInt( "team" );
		float roundTime = event->GetFloat( "round_time" );
		int fullRound = event->GetInt( "full_round" );
		Event_RoundEnd( winningTeam, roundTime, fullRound );
	}
	else if ( FStrEq( "teamplay_round_active", pEventName ) )
	{
		Event_RoundActive();
	}
	else if ( FStrEq( "player_changeclass", pEventName ) )
	{
		int userid = event->GetInt( "userid" );
		int classid = event->GetInt( "class" );
		Event_PlayerChangeClass( userid, classid );
	}
	else if ( FStrEq( "player_hurt", pEventName ) )
	{
		Event_PlayerHurt( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_RoundActive()
{
	m_currentRound.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	m_currentMap.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	m_bRoundActive = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_RoundEnd( int winningTeam, float roundTime, int fullRound )
{
	SW_GameStats_WriteClientRound( winningTeam, fullRound, RE_ROUND_END );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_PlayerChangeClass( int userid, int classid )
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pTFPlayer )
		return;

	if ( pTFPlayer != UTIL_PlayerByIndex( engine->GetPlayerForUserID( userid ) ) )
		return;

	m_currentSession.m_ClassesPlayed.Set( classid );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_PlayerHurt( IGameEvent* event /*player_hurt*/ )
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pTFPlayer )
		return;

	int attackerid = event->GetInt( "attacker" );
	if ( pTFPlayer != UTIL_PlayerByIndex( engine->GetPlayerForUserID( attackerid ) ) )
		return;

	if ( !m_bRoundActive )
		return;

	// My kingdom for a proper damage effects system.
	// We need to ignore DOTS in some stats.
	int custom = event->GetInt( "custom" );
	bool bIsDamageOverTime = IsDOTDmg( custom );
	bool bIsTauntDamage = IsTauntDmg( custom );

	// Update weapon info with this hit.
	int weaponid = event->GetInt( "weaponid" );
	int damageamount = event->GetInt( "damageamount" );

	TF_Gamestats_WeaponInfo_t info;

	int idx = m_mapWeaponInfo.Find( weaponid );
	if ( idx == m_mapWeaponInfo.InvalidIndex() )
	{
		info.weaponID = weaponid;
		idx = m_mapWeaponInfo.Insert( weaponid, info );
	}
	else
	{
		info = m_mapWeaponInfo[idx];
	}

	info.totalDamage += damageamount;

	if ( !bIsDamageOverTime && !bIsTauntDamage && (gpGlobals->curtime != info.lastUpdateTime) )
	{
		bool crit = event->GetInt( "crit" );
		if ( crit )
		{
			info.critHits++;
		}
		info.shotsHit++;
		info.shotsMissed--;
	}

	if ( info.shotsHit )
	{
		info.avgDamage = info.totalDamage / info.shotsHit;
	}
	else
	{
		info.avgDamage = 0;
	}

	info.lastUpdateTime = gpGlobals->curtime;

	m_mapWeaponInfo[idx] = info;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_PlayerFiredWeapon( C_TFPlayer *pPlayer, bool bCritical )
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || (pTFPlayer != pPlayer) )
		return;

	if ( !m_bRoundActive )
		return;

	CTFWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
	if ( pTFWeapon )
	{
		// all shots are assumed to be misses until they hit
		int iWeaponID = pTFWeapon->GetWeaponID();

		TF_Gamestats_WeaponInfo_t info;

		int idx = m_mapWeaponInfo.Find( iWeaponID );
		if ( idx == m_mapWeaponInfo.InvalidIndex() )
		{
			info.weaponID = iWeaponID;
			idx = m_mapWeaponInfo.Insert( iWeaponID, info );
		}
		else
		{
			info = m_mapWeaponInfo[idx];
		}

		info.shotsFired++;
		info.shotsMissed++;
		if ( bCritical )
		{
			info.critsFired++;
		}

		m_mapWeaponInfo[idx] = info;
	}
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Client Data
// Sends the client's session summary report.
//-----------------------------------------------------------------------------
void C_CTFGameStats::SW_GameStats_WriteClientSessionSummary()
{
#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ClientSessionDetails" );

	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );

	RTime32 currentClock = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();

	pKVData->SetInt( "StartTime", m_currentSession.m_SessionStart );
	pKVData->SetInt( "EndTime", currentClock );

	if ( GetSteamWorksSGameStatsUploader().GetNumServerConnects() > 0 )
	{
		pKVData->SetInt( "ServerConnects", GetSteamWorksSGameStatsUploader().GetNumServerConnects() );
	}

	ConVarRef sb_firstopentime( "sb_firstopentime" );

	if ( m_currentSession.m_FirstConnect > 0 )
	{
		int iTimeFromStartToJoin = m_currentSession.m_FirstConnect - m_currentSession.m_SessionStart;
		pKVData->SetInt( "TimeFromStartToJoin", iTimeFromStartToJoin );
	}

	if ( sb_firstopentime.GetInt() > 0 &&
		 m_currentSession.m_FirstConnect > 0 )
	{
		int iTimeFromBrowseToJoin = m_currentSession.m_FirstConnect - sb_firstopentime.GetInt();
		if ( iTimeFromBrowseToJoin > 0 )
		{
			pKVData->SetInt( "TimeFromBrowseToJoin", iTimeFromBrowseToJoin );
		}
	}

	if ( sb_firstopentime.GetInt() > 0 )
	{
		int iTimeFromStartToBrowse = sb_firstopentime.GetInt() - m_currentSession.m_SessionStart;
		pKVData->SetInt( "TimeFromStartToBrowse", iTimeFromStartToBrowse );
	}

	ConVarRef sb_numtimesopened( "sb_numtimesopened" );
	pKVData->SetInt( "TimesOpenedServerBrowser", sb_numtimesopened.GetInt() );

	int iClassesPlayed = 0;
	for ( int i=TF_FIRST_NORMAL_CLASS; i<TF_LAST_NORMAL_CLASS; ++i )
	{
		if ( m_currentSession.m_ClassesPlayed.IsBitSet( i ) )
		{
			iClassesPlayed++;
		}
	}

	if ( iClassesPlayed > 0 )
	{
		pKVData->SetInt( "ClassesPlayed", iClassesPlayed );
	}

	if ( m_currentSession.m_iMapsPlayed > 0 )
	{
		pKVData->SetInt( "MapsPlayed", m_currentSession.m_iMapsPlayed );
	}

	if ( m_currentSession.m_iRoundsPlayed > 0 )
	{
		pKVData->SetInt( "RoundsPlayed", m_currentSession.m_iRoundsPlayed );
	}

	/*
	FavoriteClass
	FavoriteWeapon
	FavoriteMap
	*/

	if ( m_currentSession.m_Summary.iKills > 0 )
	{
		pKVData->SetInt( "Kills", m_currentSession.m_Summary.iKills );
	}

	if ( m_currentSession.m_Summary.iDeaths > 0 )
	{
		pKVData->SetInt( "Deaths", m_currentSession.m_Summary.iDeaths );
	}

	if ( m_currentSession.m_Summary.iSuicides > 0 )
	{
		pKVData->SetInt( "Suicides", m_currentSession.m_Summary.iSuicides );
	}

	if ( m_currentSession.m_Summary.iAssists > 0 )
	{
		pKVData->SetInt( "Assists", m_currentSession.m_Summary.iAssists );
	}

	if ( m_currentSession.m_Summary.iBuildingsBuilt > 0 )
	{
		pKVData->SetInt( "BuildingsBuilt", m_currentSession.m_Summary.iBuildingsBuilt );
	}

	if ( m_currentSession.m_Summary.iBuildingsDestroyed > 0 )
	{
		pKVData->SetInt( "BuildingsDestroyed", m_currentSession.m_Summary.iBuildingsDestroyed );
	}

	if ( m_currentSession.m_Summary.iHeadshots > 0 )
	{
		pKVData->SetInt( "Headshots", m_currentSession.m_Summary.iHeadshots );
	}

	if ( m_currentSession.m_Summary.iDominations > 0 )
	{
		pKVData->SetInt( "Dominations", m_currentSession.m_Summary.iDominations );
	}

	if ( m_currentSession.m_Summary.iRevenges > 0 )
	{
		pKVData->SetInt( "Revenges", m_currentSession.m_Summary.iRevenges );
	}

	if ( m_currentSession.m_Summary.iInvulns > 0 )
	{
		pKVData->SetInt( "Invulns", m_currentSession.m_Summary.iInvulns );
	}

	if ( m_currentSession.m_Summary.iTeleports > 0 )
	{
		pKVData->SetInt( "Teleports", m_currentSession.m_Summary.iTeleports );
	}

	if ( m_currentSession.m_Summary.iDamageDone > 0 )
	{
		pKVData->SetInt( "DamageDone", m_currentSession.m_Summary.iDamageDone );
	}

	if ( m_currentSession.m_Summary.iHealingDone > 0 )
	{
		pKVData->SetInt( "HealingDone", m_currentSession.m_Summary.iHealingDone );
	}

	if ( m_currentSession.m_Summary.iCrits > 0 )
	{
		pKVData->SetInt( "Crits", m_currentSession.m_Summary.iCrits );
	}

	if ( m_currentSession.m_Summary.iBackstabs > 0 )
	{
		pKVData->SetInt( "Backstabs", m_currentSession.m_Summary.iBackstabs );
	}

	if ( m_currentSession.m_Summary.iAchievementsEarned > 0 )
	{
		pKVData->SetInt( "AchievementsEarned", m_currentSession.m_Summary.iAchievementsEarned );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

extern CBaseGameStats_Driver CBGSDriver;

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Client Map Data
//-----------------------------------------------------------------------------
void C_CTFGameStats::SW_GameStats_WriteClientMap()
{
#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ClientMaps" );

	pKVData->SetInt( "MapIndex", m_currentSession.m_iMapsPlayed );
	pKVData->SetInt( "StartTime", m_currentMap.m_iMapStartTime );
	pKVData->SetInt( "EndTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	pKVData->SetString( "MapID", m_currentMap.m_Header.m_szMapName );

	const char* pszGameTypeID = GetGameTypeID();
	if ( pszGameTypeID )
	{
		pKVData->SetString( "GameTypeID", pszGameTypeID  );
	}

	pKVData->SetInt( "RoundsPlayed", m_currentMap.m_Header.m_iRoundsPlayed );
	pKVData->SetInt( "MapVersion", m_currentMap.m_Header.m_nMapRevision );

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Client Round Data
//-----------------------------------------------------------------------------
void C_CTFGameStats::SW_GameStats_WriteClientRound( int winningTeam, int fullRound, int endReason )
{
	if ( !m_bRoundActive )
		return;

	m_bRoundActive = false;

	m_currentSession.m_iRoundsPlayed++;
	m_currentMap.m_Header.m_iRoundsPlayed++;

#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ClientRounds" );

	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pTFPlayer )
		return;

	localplayerscoring_t *pData = pTFPlayer->m_Shared.GetRoundScoringData();
	if ( !pData )
		return;

 	pKVData->SetInt( "MapIndex", m_currentSession.m_iMapsPlayed );
	pKVData->SetInt( "RoundIndex", m_currentMap.m_Header.m_iRoundsPlayed );
	pKVData->SetInt( "StartTime", m_currentRound.m_iRoundStartTime );
	pKVData->SetInt( "EndTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	pKVData->SetString( "EndReason", ClampedArrayElement( g_aRoundEndReasons, endReason ) );

	winningTeam = clamp( winningTeam, 0, TF_TEAM_COUNT - 1 );
	pKVData->SetString( "WinningTeam", ClampedArrayElement( g_aTeamNames, winningTeam ) );

	if ( fullRound > 0 )
	{
		pKVData->SetInt( "FullRound", fullRound );
	}

	if ( pData->m_iPoints > 0 )
	{
		pKVData->SetInt( "PointsScored", pData->m_iPoints );
	}

	if ( pData->m_iBonusPoints > 0 )
	{
		pKVData->SetInt( "BonusPointsScored", pData->m_iBonusPoints );
	}

	if ( pData->m_iKills > 0 )
	{
		pKVData->SetInt( "Kills", pData->m_iKills );
	}

	if ( pData->m_iDeaths > 0 )
	{
		pKVData->SetInt( "Deaths", pData->m_iDeaths );
	}

	if ( pData->m_iSuicides > 0 )
	{
		pKVData->SetInt( "Suicides", pData->m_iSuicides );
	}

	if ( pData->m_iKillAssists > 0 )
	{
		pKVData->SetInt( "Assists", pData->m_iKillAssists );
	}

	if ( pData->m_iBuildingsBuilt > 0 )
	{
		pKVData->SetInt( "BuildingsBuilt", pData->m_iBuildingsBuilt );
	}

	if ( pData->m_iBuildingsDestroyed > 0 )
	{
		pKVData->SetInt( "BuildingsDestroyed", pData->m_iBuildingsDestroyed );
	}

	if ( pData->m_iHeadshots > 0 )
	{
		pKVData->SetInt( "Headshots", pData->m_iHeadshots );
	}

	if ( pData->m_iDominations > 0 )
	{
		pKVData->SetInt( "Dominations", pData->m_iDominations );
	}

	if ( pData->m_iRevenge > 0 )
	{
		pKVData->SetInt( "Revenges", pData->m_iRevenge );
	}

	if ( pData->m_iInvulns > 0 )
	{
		pKVData->SetInt( "Invulns", pData->m_iInvulns );		
	}

	if ( pData->m_iTeleports > 0 )
	{
		pKVData->SetInt( "Teleports", pData->m_iTeleports );
	}

	if ( pData->m_iDamageDone > 0 )
	{
		pKVData->SetInt( "DamageDone", pData->m_iDamageDone );
	}

	if ( pData->m_iHealPoints > 0 )
	{
		pKVData->SetInt( "HealingDone", pData->m_iHealPoints );
	}

	if ( pData->m_iCrits > 0 )
	{
		pKVData->SetInt( "Crits", pData->m_iCrits );
	}

	if ( pData->m_iBackstabs > 0 )
	{
		pKVData->SetInt( "Backstabs", pData->m_iBackstabs );
	}

	// Add totals to the current session.
	m_currentSession.m_Summary.iKills += pData->m_iKills;
	m_currentSession.m_Summary.iDeaths += pData->m_iDeaths;
	m_currentSession.m_Summary.iSuicides += pData->m_iSuicides;
	m_currentSession.m_Summary.iAssists += pData->m_iKillAssists;
	m_currentSession.m_Summary.iBuildingsBuilt += pData->m_iBuildingsBuilt;
	m_currentSession.m_Summary.iBuildingsDestroyed += pData->m_iBuildingsDestroyed;
	m_currentSession.m_Summary.iHeadshots += pData->m_iHeadshots;
	m_currentSession.m_Summary.iDominations += pData->m_iDominations;
	m_currentSession.m_Summary.iRevenges += pData->m_iRevenge;
	m_currentSession.m_Summary.iInvulns += pData->m_iInvulns;
	m_currentSession.m_Summary.iTeleports += pData->m_iTeleports;
	m_currentSession.m_Summary.iDamageDone += pData->m_iDamageDone;
	m_currentSession.m_Summary.iHealingDone += pData->m_iHealPoints;
	m_currentSession.m_Summary.iCrits += pData->m_iCrits;
	m_currentSession.m_Summary.iBackstabs += pData->m_iBackstabs;

	m_currentRound.Reset(); // Not used anymore?

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Weapons Data
//-----------------------------------------------------------------------------
void C_CTFGameStats::SW_GameStats_WriteClientWeapons()
{
#if !defined(NO_STEAM)
	for ( unsigned int i=0; i<m_mapWeaponInfo.Count(); ++i )
	{
		KeyValues* pKVData = new KeyValues( "TF2ClientWeapons" );

		TF_Gamestats_WeaponInfo_t info = m_mapWeaponInfo[i];

		pKVData->SetString( "WeaponID", GetWeaponIDName( info.weaponID ) );

		if ( info.shotsFired > 0 )
		{
			pKVData->SetInt( "ShotsFired", info.shotsFired );
		}

		if ( info.shotsHit > 0 )
		{
			pKVData->SetInt( "ShotsHit", info.shotsHit );
		}

		if ( info.shotsMissed > 0 )
		{
			pKVData->SetInt( "ShotsMissed", info.shotsMissed );
		}

		if ( info.critsFired > 0 )
		{
			pKVData->SetInt( "CritsFired", info.critsFired );
		}

		if ( info.critHits > 0 )
		{
			pKVData->SetInt( "CritHits", info.critHits );
		}

		if ( info.avgDamage > 0 )
		{
			pKVData->SetInt( "AvgDamage", info.avgDamage );
		}

		if ( info.totalDamage > 0 )
		{
			pKVData->SetInt( "TotalDamage", info.totalDamage );
		}

		GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Records client achievement events for reporting to steam.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_AchievementProgress( int achievementID, const char* achievementName )
{
	TF_Gamestats_AchievementEvent_t event( achievementID, achievementName );
	m_vecAchievementEvents.AddToTail( event );

	m_currentSession.m_Summary.iAchievementsEarned++;

	KeyValues* pKVData = new KeyValues( "TF2ClientAchievements" );
	pKVData->SetInt( "TimeEarned", event.eventTime );
	pKVData->SetInt( "AchievementNum", event.achievementNum );
	pKVData->SetString( "AchievementID", event.achievementID );
	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Weapon stats
//-----------------------------------------------------------------------------
TF_Gamestats_WeaponInfo_t::TF_Gamestats_WeaponInfo_t()
{
	weaponID = 0;
	critsFired = 0;
	shotsFired = 0;
	shotsHit = 0;
	shotsMissed = 0;
	avgDamage = 0;
	totalDamage = 0;
	critHits = 0;
	lastUpdateTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Achievement progress recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_AchievementEvent_t::TF_Gamestats_AchievementEvent_t( int in_achievementNum, const char* in_achievementID )
{
	eventTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	achievementNum = in_achievementNum;
	achievementID = in_achievementID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TF_Gamestats_ClientSession_t::TF_Gamestats_ClientSession_t()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the client session data.
//-----------------------------------------------------------------------------
void TF_Gamestats_ClientSession_t::Reset()
{
	Q_memset( &m_Summary, 0, sizeof( m_Summary ) );
	m_SessionStart = 0;
	m_FirstConnect = 0;
	m_iMapsPlayed = 0;
	m_iRoundsPlayed = 0;
	m_ClassesPlayed.ClearAll();
}

//-----------------------------------------------------------------------------
// Purpose: Item event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_ItemEvent::TF_Gamestats_ItemEvent( int in_eventNum, CEconItemView* in_item )
{
	static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );

	bUseNameBuf = false;

	eventTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	eventNum = in_eventNum;
	eventID = g_ItemEventNames[eventNum];

	if ( in_item )
	{
		itemDefIndex = in_item->GetItemDefIndex();
		itemID = in_item->GetItemID();
		itemName = in_item->GetStaticData()->GetDefinitionName();

		// If this is a crate, quality is the series.
		float fSeries;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( in_item, pAttrDef_SupplyCrateSeries, &fSeries ) )
		{
			bUseNameBuf = true;
			Q_snprintf( itemNameBuf, sizeof( itemNameBuf ), "%s, Series %i", itemName, (int)fSeries );
		}
	}
	else
	{
		itemDefIndex = 0;
		itemID = 0;
		itemName = NULL;
	}
}

//----------------------------------------------------------------------------
// Purpose: Records a catalog event for reporting.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_Catalog( int eventID, const char* filter, CEconItemView* item )
{
	TF_Gamestats_CatalogEvent event( eventID, item, filter );
	m_vecCatalogEvents.AddToTail( event );

	KeyValues* pKVData = new KeyValues( "TF2ClientCatalogEvents" );
	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
	pKVData->SetInt( "EventCount", m_vecCatalogEvents.Count() );
	pKVData->SetInt( "EventTime", event.eventTime );
	pKVData->SetString( "ItemEventID", event.eventID );
	if ( event.catalogFilter )
	{
		pKVData->SetString( "CatFilterID", event.catalogFilter );
	}
	if ( event.itemID > 0 )
	{
		pKVData->SetUint64( "GlobalIndex", event.itemID );
	}
	if ( event.itemDefIndex > 0 )
	{
		pKVData->SetInt( "DefIndex", event.itemDefIndex );
	}
	if ( event.GetItemName() != NULL )
	{
		pKVData->SetString( "ItemNameID", event.GetItemName() );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Catalog event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_CatalogEvent::TF_Gamestats_CatalogEvent( int in_eventNum, CEconItemView* in_item, const char* filter )
: TF_Gamestats_ItemEvent( in_eventNum, in_item )
{
	catalogFilter = filter;
}

//----------------------------------------------------------------------------
// Purpose: Records a catalog event for reporting.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_Crafting( int eventID, CEconItemView* item, int numAttempts, int recipeFound )
{
	TF_Gamestats_CraftingEvent event( eventID, item, numAttempts, recipeFound );
	m_vecCraftingEvents.AddToTail( event );

	KeyValues* pKVData = new KeyValues( "TF2ClientCraftingEvents" );
	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
	pKVData->SetInt( "EventCount", m_vecCraftingEvents.Count() );
	pKVData->SetInt( "EventTime", event.eventTime );
	pKVData->SetString( "ItemEventID", event.eventID );

	if ( event.itemID > 0 )
	{
		pKVData->SetUint64( "GlobalIndex", event.itemID );
	}
	if ( event.itemDefIndex > 0 )
	{
		pKVData->SetInt( "DefIndex", event.itemDefIndex );
	}
	if ( event.GetItemName() != NULL )
	{
		pKVData->SetString( "ItemNameID", event.GetItemName() );
	}

	if ( event.numAttempts > 0 )
	{
		pKVData->SetInt( "CraftAttemptNum", event.numAttempts );
	}

	if ( event.recipeFound > 0 )
	{
		pKVData->SetInt( "RecipeFound", event.recipeFound );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Crafting event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_CraftingEvent::TF_Gamestats_CraftingEvent( int in_eventNum, CEconItemView* in_item, int in_numAttempts, int in_recipe )
: TF_Gamestats_ItemEvent( in_eventNum, in_item )
{
	numAttempts = in_numAttempts;
	recipeFound = in_recipe;
}

//----------------------------------------------------------------------------
// Purpose: Records a store event for reporting.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_Store( int eventID, CEconItemView* item, const char* panelName, int classId, 
								 const cart_item_t* cartItem, int checkoutAttempts, const char* storeError, int totalPrice, int currencyCode )
{
	TF_Gamestats_StoreEvent event( eventID, item, panelName, classId, cartItem, checkoutAttempts, storeError, totalPrice, currencyCode );
	m_vecStoreEvents.AddToTail( event );

	KeyValues* pKVData = new KeyValues( "TF2ClientStoreEvents" );
	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
	pKVData->SetInt( "EventCount", m_vecStoreEvents.Count() );
	pKVData->SetInt( "EventTime", event.eventTime );
	pKVData->SetString( "ItemEventID", event.eventID );

	if ( event.eventNum == IE_STORE_ENTERED || event.eventNum == IE_STORE2_ENTERED )
	{
		if ( m_ulExperimentValue != ((int64) ~0) )
		{
			pKVData->SetUint64( "GlobalIndex", m_ulExperimentValue );
		}
	}
	else
	{
		if ( event.itemID > 0 )
		{
			pKVData->SetUint64( "GlobalIndex", event.itemID );
		}
	}

	if ( event.itemDefIndex > 0 )
	{
		pKVData->SetInt( "DefIndex", event.itemDefIndex );
	}
	if ( event.GetItemName() != NULL )
	{
		pKVData->SetString( "ItemNameID", event.GetItemName() );
	}

	if ( event.panelName != NULL )
	{
		pKVData->SetString( "StorePanelID", event.panelName );
	}
	if ( event.classId > 0 )
	{
		int iClass = clamp( event.classId, 0, TF_CLASS_COUNT-1 );
		pKVData->SetString( "ClassID", g_aPlayerClassNames_NonLocalized[iClass] );
	}
	if ( event.cartQuantity > 0 )
	{
		pKVData->SetInt( "NewQuantity", event.cartQuantity );
	}
	if ( event.cartItemCost > 0 )
	{
		pKVData->SetInt( "Price", event.cartItemCost );
	}
	if ( event.currencyCode > 0 )
	{
		pKVData->SetInt( "CurrencyCode", event.currencyCode-1 );
	}
	if ( event.checkoutAttempt > 0 )
	{
		pKVData->SetInt( "CheckoutAttemptNum", event.checkoutAttempt );
	}
	if ( event.storeError != NULL )
	{
		pKVData->SetString( "StoreErrorID", event.storeError );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Store event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_StoreEvent::TF_Gamestats_StoreEvent( int in_eventNum, CEconItemView* in_item, const char* in_panelName, int in_classId, 
												 const cart_item_t* in_cartItem, int in_checkoutAttempts, const char* in_storeError, int in_totalPrice, int in_currencyCode )
: TF_Gamestats_ItemEvent( in_eventNum, in_item )
{
	classId = in_classId;

	checkoutAttempt = in_checkoutAttempts;

	storeError = NULL;
	if ( in_storeError )
	{
		storeError = in_storeError;
	}

	panelName = NULL;
	if ( in_panelName )
	{
		panelName = in_panelName;
	}

	cartQuantity = 0;
	cartItemCost = 0;
	currencyCode = in_currencyCode;

	if ( in_cartItem )
	{
		CEconItemDefinition *pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( in_cartItem->pEntry->GetItemDefinitionIndex() );
		if ( pItemDef )
		{
			itemDefIndex = in_cartItem->pEntry->GetItemDefinitionIndex();
			itemName = pItemDef->GetDefinitionName();
			cartQuantity = in_cartItem->iQuantity;
			cartItemCost = in_cartItem->pEntry->GetCurrentPrice( (ECurrency)in_currencyCode );
		}
	}

	if ( in_eventNum == IE_STORE_CHECKOUT_SUCCESS && in_totalPrice > 0 )
	{
		cartItemCost = in_totalPrice;
	}
}

//----------------------------------------------------------------------------
// Purpose: Records a general item transaction event.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_ItemTransaction( int eventID, CEconItemView* item, const char* pszReason, int iQuality )
{
	TF_Gamestats_ItemTransactionEvent event( eventID, item, pszReason, iQuality );
	m_vecItemTransactionEvents.AddToTail( event );

	KeyValues* pKVData = new KeyValues( "TF2ClientItemTransactionEvents" );
	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
	pKVData->SetInt( "EventCount", m_vecItemTransactionEvents.Count() );
	pKVData->SetInt( "EventTime", event.eventTime );
	pKVData->SetString( "ItemEventID", event.eventID );
	if ( event.itemID > 0 )
	{
		pKVData->SetUint64( "GlobalIndex", event.itemID );
	}
	if ( event.itemDefIndex > 0 )
	{
		pKVData->SetInt( "DefIndex", event.itemDefIndex );
	}
	if ( event.GetItemName() != NULL )
	{
		pKVData->SetString( "ItemNameID", event.GetItemName() );
	}
	if ( event.reason )
	{
		pKVData->SetString( "ItemTransReasonID", event.reason );
	}

	if ( event.itemQuality > 0 )
	{
		pKVData->SetInt( "ItemQuality", event.itemQuality );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Transaction event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_ItemTransactionEvent::TF_Gamestats_ItemTransactionEvent( int in_eventNum, CEconItemView* in_item, const char* in_reason, int in_quality )
: TF_Gamestats_ItemEvent( in_eventNum, in_item )
{
	static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );

	reason = in_reason;
	if ( in_item )
	{
		itemQuality = in_item->GetItemQuality();

		// If this is a crate, quality is the series.
		float fSeries;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( in_item, pAttrDef_SupplyCrateSeries, &fSeries ) )
		{
			in_quality = fSeries;
		}
	}

	if ( in_quality > 0 )
	{
		// For crates, indicates the series.
		itemQuality = in_quality;
	}
}

void C_CTFGameStats::Event_Trading( TF_Gamestats_TradeEvent& event )
{
	KeyValues* pKVData = new KeyValues( "TF2ClientTradeEvents" );
	pKVData->SetUint64( "AccountID", steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
	pKVData->SetInt( "EventCount", m_vecTradeEvents.Count() );
	pKVData->SetInt( "EventTime", event.eventTime );
	pKVData->SetString( "ItemEventID", event.eventID );

	if ( event.itemID > 0 )
	{
		pKVData->SetUint64( "GlobalIndex", event.itemID );
	}
	if ( event.itemDefIndex > 0 )
	{
		pKVData->SetInt( "DefIndex", event.itemDefIndex );
	}
	if ( event.GetItemName() != NULL )
	{
		pKVData->SetString( "ItemNameID", event.GetItemName() );
	}

	if ( event.localPlayerPartyMatters )
	{
		pKVData->SetInt( "IsLocalPlayerPartyA", event.localPlayerIsPartyA );
	}

	if ( event.steamIDPartyA > 0 )
	{
		pKVData->SetUint64( "SteamIDPartyA", event.steamIDPartyA );
	}

	if ( event.steamIDPartyB > 0 )
	{
		pKVData->SetUint64( "SteamIDPartyB", event.steamIDPartyB );
	}

	if ( event.steamIDRequested > 0 )
	{
		pKVData->SetUint64( "RequestedTradeWithSteamID", event.steamIDRequested );
	}

	if ( event.tradeRequests > 0 )
	{
		pKVData->SetInt( "TradeRequestIndex", event.tradeRequests );
	}

	if ( event.tradeAttempts > 0 )
	{
		pKVData->SetInt( "TradeAttemptIndex", event.tradeAttempts );
	}

	if ( event.reason )
	{
		pKVData->SetString( "TradeReasonID", event.reason );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//----------------------------------------------------------------------------
// Purpose: Records an item trade event.
//-----------------------------------------------------------------------------
void C_CTFGameStats::Event_Trading( int eventID, CEconItemView* item, bool localPlayerIsPartyA,
								   uint64 steamIDPartyA, uint64 steamIDPartyB, int iTradeRequests, int iTradeAttempts )
{
	TF_Gamestats_TradeEvent event( eventID, item, localPlayerIsPartyA, 
		steamIDPartyA, steamIDPartyB, iTradeRequests, iTradeAttempts );
	m_vecTradeEvents.AddToTail( event );

	Event_Trading( event );
}

void C_CTFGameStats::Event_Trading( int eventID, uint64 steamIDRequested, int iTradeRequests, int iTradeAttempts )
{
	TF_Gamestats_TradeEvent event( eventID, steamIDRequested, iTradeRequests, iTradeAttempts );
	m_vecTradeEvents.AddToTail( event );

	Event_Trading( event );
}

void C_CTFGameStats::Event_Trading( int eventID, int iTradeRequests, const char* reason, int iTradeAttempts )
{
	TF_Gamestats_TradeEvent event( eventID, iTradeRequests, reason, iTradeAttempts );
	m_vecTradeEvents.AddToTail( event );

	Event_Trading( event );
}

//-----------------------------------------------------------------------------
// Purpose: Trade event recorder.
//-----------------------------------------------------------------------------
TF_Gamestats_TradeEvent::TF_Gamestats_TradeEvent( int in_eventNum, CEconItemView* in_item, bool in_localPlayerIsPartyA,
												 uint64 in_steamIDPartyA, uint64 in_steamIDPartyB, 
												 int in_tradeRequests, int in_tradeAttempts )
: TF_Gamestats_ItemEvent( in_eventNum, in_item )
{
	localPlayerPartyMatters = true;
	localPlayerIsPartyA = in_localPlayerIsPartyA;
	steamIDPartyA = in_steamIDPartyA;
	steamIDPartyB = in_steamIDPartyB;

	steamIDRequested = 0;
	tradeRequests = in_tradeRequests;
	tradeAttempts = in_tradeAttempts;

	reason = NULL;
}

TF_Gamestats_TradeEvent::TF_Gamestats_TradeEvent( int in_eventNum, uint64 in_steamIDRequested, 
												 int in_tradeRequests, int in_tradeAttempts )
: TF_Gamestats_ItemEvent( in_eventNum, NULL )
{
	localPlayerPartyMatters = false;
	localPlayerIsPartyA = false;
	steamIDPartyA = 0;
	steamIDPartyB = 0;

	steamIDRequested = in_steamIDRequested;
	tradeRequests = in_tradeRequests;
	tradeAttempts = in_tradeAttempts;

	reason = NULL;
}

TF_Gamestats_TradeEvent::TF_Gamestats_TradeEvent( int in_eventNum, int in_tradeRequests, const char* in_reason, int in_tradeAttempts )
: TF_Gamestats_ItemEvent( in_eventNum, NULL )
{
	localPlayerPartyMatters = false;
	localPlayerIsPartyA = false;
	steamIDPartyA = 0;
	steamIDPartyB = 0;

	steamIDRequested = 0;
	tradeRequests = in_tradeRequests;
	tradeAttempts = in_tradeAttempts;

	reason = in_reason;
}

//-----------------------------------------------------------------------------
// Purpose: Interface stats recorder.
//-----------------------------------------------------------------------------

static const bool g_bRecordClientInterfaceEventsToOGS = false;

/*static*/ void C_CTFGameStats::ImmediateWriteInterfaceEvent( const char *pszEventType, const char *pszEventDesc )
{
	if ( !g_bRecordClientInterfaceEventsToOGS )
		return;

	static uint32 s_unEventCount = 0;

	KeyValues* pKVData = new KeyValues( "TF2UIEvents" );
	pKVData->SetInt( "EventCounter", s_unEventCount++ );
	
	if ( pszEventType )
	{
		pKVData->SetString( "EventTypeID", pszEventType );
	}
	if ( pszEventDesc )
	{
		pKVData->SetString( "EventDescID", pszEventDesc );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: post results from first step of quickplay
//-----------------------------------------------------------------------------

int g_iQuickplaySessionIndex = 0;

void C_CTFGameStats::QuickplayResults( const TF_Gamestats_QuickPlay_t &info )
{

	// Update index
	++g_iQuickplaySessionIndex;

	// Determine sampling rate
	float odds = tf_matchmaking_ogs_odds.GetFloat();
	Assert( odds >= 0.0f && odds <= 1.0f );
	if ( GetUniverse() != k_EUniversePublic )
	{
		odds = 1.0f;
	}

	// Send full or abbreviated stats?
	float r = RandomFloat();
	int iDetailLevel = ( r < odds ) ? TF_Gamestats_QuickPlay_t::k_Server_Ineligible : TF_Gamestats_QuickPlay_t::k_Server_Pinged;

	// Count up servers that meet various criteria
	int nServersResponded = info.m_vecServers.Count();
	int nServersEligible = 0;
	int nServersSentToGC = 0;
	int nServersScoredByGC = 0;
	int nServersPinged = 0;
	for ( int i = 0 ; i < info.m_vecServers.Count() ; ++i )
	{
		const TF_Gamestats_QuickPlay_t::eServerStatus &s = info.m_vecServers[i].m_eStatus;
		if ( s >= TF_Gamestats_QuickPlay_t::k_Server_Eligible )
		{
			++nServersEligible;
		}
		if ( s >= TF_Gamestats_QuickPlay_t::k_Server_RequestedScore )
		{
			++nServersSentToGC;
		}
		if ( s >= TF_Gamestats_QuickPlay_t::k_Server_Scored && info.m_vecServers[i].m_fScoreGC > -999.0f )
		{
			++nServersScoredByGC;
		}
		if ( s >= TF_Gamestats_QuickPlay_t::k_Server_Pinged )
		{
			++nServersPinged;
		}
	}

	KeyValues* pKVData = new KeyValues( "TF2QuickPlaySession" );
	pKVData->SetInt( "QuickPlaySessionID", g_iQuickplaySessionIndex );
	pKVData->SetFloat( "UserHoursPlayed", info.m_fUserHoursPlayed );
	pKVData->SetString( "TF2GameModeIDSearched", info.m_sUserGameMode );
	pKVData->SetInt( "Result", info.m_eResultCode );
	pKVData->SetInt( "ReportDetail", iDetailLevel );
	pKVData->SetFloat( "SearchTime", info.m_fSearchTime );
	pKVData->SetInt( "ExperimentGroup", info.m_iExperimentGroup );
	pKVData->SetInt( "ServersResponded", nServersResponded );
	pKVData->SetInt( "ServersEligible", nServersEligible );
	pKVData->SetInt( "ServersSentToGC", nServersSentToGC );
	pKVData->SetInt( "ServersScoredByGC", nServersScoredByGC );
	pKVData->SetInt( "ServersPinged", nServersPinged );

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );

	for ( int i = 0 ; i < info.m_vecServers.Count() ; ++i )
	{
		const TF_Gamestats_QuickPlay_t::Server_t &s = info.m_vecServers[i];

		// Check if he's not important enough to report
		if ( s.m_eStatus < iDetailLevel )
		{
			continue;
		}

		pKVData = new KeyValues( "TF2QuickPlayResult" );
		pKVData->SetInt( "QuickPlaySessionID", g_iQuickplaySessionIndex );
		pKVData->SetInt( "ServerIP", s.m_ip );
		pKVData->SetInt( "ServerPort", s.m_port );
		pKVData->SetInt( "Status", s.m_eStatus );
		pKVData->SetInt( "ServerIsRegistered", s.m_bRegistered ? 1 : 0 );
		pKVData->SetInt( "ServerIsValve", s.m_bValve ? 1 : 0  );
		pKVData->SetInt( "ServerMapIsNewUserFriendly", s.m_bMapIsNewUserFriendly ? 1 : 0  );
		pKVData->SetInt( "ServerMapIsQuickPlayOK", s.m_bMapIsQuickPlayOK ? 1 : 0  );
		pKVData->SetInt( "ServerIsSecure", s.m_bSecure ? 1 : 0  );
		pKVData->SetInt( "ServerPlayers", s.m_nPlayers );
		pKVData->SetInt( "ServerMaxPlayers", s.m_nMaxPlayers );
		pKVData->SetString( "MapID", s.m_sMapName );
		pKVData->SetInt( "Ping", s.m_iPing );
		pKVData->SetFloat( "ScoreServer", s.m_fScoreServer );
		pKVData->SetFloat( "ScoreClient", s.m_fScoreClient );
		if ( s.m_eStatus >= TF_Gamestats_QuickPlay_t::k_Server_Scored && s.m_fScoreGC > -999.0f )
		{
			pKVData->SetFloat( "ScoreGC", s.m_fScoreGC );
		}
		pKVData->SetString( "TF2ServerTagsID", s.m_sTags );
		GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
	}
}
