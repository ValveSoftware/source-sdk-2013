//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_obj_sapper.h"
#include "tf_team.h"
#include "usermessages.h"
#include "player_resource.h"
#include "team.h"
#include "achievementmgr.h"
#include "hl2orange.spa.h"
#include "tf_weapon_medigun.h"
#include "NextBot/NextBotManager.h"
#include "team_control_point_master.h"
#include "steamworks_gamestats.h"
#include "vote_controller.h"
#include "tf_mann_vs_machine_stats.h"

#include "tf_passtime_logic.h"
#include "filesystem.h" // for temp passtime local stats logging
#include "passtime_convars.h"

#include "tf_matchmaking_shared.h"

#include "gc_clientsystem.h"
#include "tf_gcmessages.h"
#include "rtime.h"
#include "team_train_watcher.h"

extern ConVar tf_mm_trusted;

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tf_stats_nogameplaycheck( "tf_stats_nogameplaycheck", "0", FCVAR_NONE , "Disable normal check for valid gameplay, send stats regardless." );
//static ConVar tf_stats_track( "tf_stats_track", "1", FCVAR_NONE, "Turn on//off tf stats tracking." );
//static ConVar tf_stats_verbose( "tf_stats_verbose", "0", FCVAR_NONE, "Turn on//off verbose logging of stats." );

CTFGameStats CTF_GameStats;

const char *g_aClassNames[] =
{
	"TF_CLASS_UNDEFINED",
	"TF_CLASS_SCOUT",
	"TF_CLASS_SNIPER",
	"TF_CLASS_SOLDIER",
	"TF_CLASS_DEMOMAN",
	"TF_CLASS_MEDIC",
	"TF_CLASS_HEAVYWEAPONS",
	"TF_CLASS_PYRO",
	"TF_CLASS_SPY",
	"TF_CLASS_ENGINEER",
	"TF_CLASS_CIVILIAN",
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::CTFGameStats()
{
	gamestats = this;
	Clear();

	SetDefLessFunc( m_MapsPlaytime );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGameStats::~CTFGameStats()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clear out game stats
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFGameStats::Clear( void )
{
	m_bRoundActive = false;
	m_iRoundsPlayed = 0;
	m_iEvents = 0;
	m_iKillCount = 0;
	m_iPlayerUpdates = 0;
	m_reportedStats.Clear();
	Q_memset( m_aPlayerStats, 0, sizeof( m_aPlayerStats ) );
	m_rdStats.Clear();
	m_passtimeStats.Clear();
	m_iLoadoutChangesCount = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Adds our data to the gamestats data that gets uploaded.
//			Returns true if we added data, false if we didn't
//-----------------------------------------------------------------------------
bool CTFGameStats::AddDataForSend( KeyValues *pKV, StatSendType_t sendType )
{
	// we only have data to send at level shutdown
	if ( sendType != STATSEND_LEVELSHUTDOWN )
		return false;

	// do we have anything to report?
	if ( !m_reportedStats.m_bValidData )
		return false;

	// Save data per map.

	CUtlDict<TF_Gamestats_LevelStats_t, unsigned short> &dictMapStats = m_reportedStats.m_dictMapStats;
	Assert( dictMapStats.Count() <= 1 );

	for ( int iMap = dictMapStats.First(); iMap != dictMapStats.InvalidIndex(); iMap = dictMapStats.Next( iMap ) )
	{
		// Get the current map.
		TF_Gamestats_LevelStats_t *pCurrentMap = &dictMapStats[iMap];
		Assert( pCurrentMap );

		KeyValues *pKVData = new KeyValues( "tf_mapdata" );
		pKVData->SetInt( "RoundsPlayed", pCurrentMap->m_Header.m_iRoundsPlayed );
		pKVData->SetInt( "TotalTime", pCurrentMap->m_Header.m_iTotalTime );
		pKVData->SetInt( "BlueWins", pCurrentMap->m_Header.m_iBlueWins );
		pKVData->SetInt( "RedWins", pCurrentMap->m_Header.m_iRedWins );
		pKVData->SetInt( "Stalemates", pCurrentMap->m_Header.m_iStalemates );
		pKVData->SetInt( "BlueSuddenDeathWins", pCurrentMap->m_Header.m_iBlueSuddenDeathWins );
		pKVData->SetInt( "RedSuddenDeathWins", pCurrentMap->m_Header.m_iRedSuddenDeathWins );
		for ( int i = 0; i <= MAX_CONTROL_POINTS; i++ )
		{
			if ( pCurrentMap->m_Header.m_iLastCapChangedInRound[i] )
			{
				pKVData->SetInt( UTIL_VarArgs("RoundsEndingOnCP%d",i), pCurrentMap->m_Header.m_iLastCapChangedInRound[i] );
			}
		}
		pKV->AddSubKey( pKVData );

		// save class stats
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass ++ )
		{
			TF_Gamestats_ClassStats_t &classStats = pCurrentMap->m_aClassStats[iClass];
			if ( classStats.iTotalTime > 0 )
			{
				pKVData = new KeyValues( "tf_classdata" );
				pKVData->SetInt( "Class", iClass );
				pKVData->SetInt( "Spawns", classStats.iSpawns );
				pKVData->SetInt( "TotalTime", classStats.iTotalTime );
				pKVData->SetInt( "Score", classStats.iScore );
				pKVData->SetInt( "Kills", classStats.iKills );
				pKVData->SetInt( "Deaths", classStats.iDeaths );
				pKVData->SetInt( "Assists", classStats.iAssists );
				pKVData->SetInt( "Captures", classStats.iCaptures );
				pKVData->SetInt( "ClassChanges", classStats.iClassChanges );
				pKV->AddSubKey( pKVData );
			}	
		}

		// save weapon stats
		for ( int iWeapon = TF_WEAPON_NONE+1; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
		{
			TF_Gamestats_WeaponStats_t &weaponStats = pCurrentMap->m_aWeaponStats[iWeapon];
			if ( weaponStats.iShotsFired > 0 )
			{
				pKVData = new KeyValues( "tf_weapondata" );
				pKVData->SetInt( "WeaponID", iWeapon );
				pKVData->SetInt( "ShotsFired", weaponStats.iShotsFired );
				pKVData->SetInt( "ShotsFiredCrit", weaponStats.iCritShotsFired );
				pKVData->SetInt( "ShotsHit", weaponStats.iHits );
				pKVData->SetInt( "DamageTotal", weaponStats.iTotalDamage );
				pKVData->SetInt( "HitsWithKnownDistance", weaponStats.iHitsWithKnownDistance );
				pKVData->SetInt( "DistanceTotal", weaponStats.iTotalDistance );
				pKV->AddSubKey( pKVData );
			}	
		}

		//// save deaths
		//for ( int i = 0; i < pCurrentMap->m_aPlayerDeaths.Count(); i++ )
		//{
		//	TF_Gamestats_LevelStats_t::PlayerDeathsLump_t &playerDeath = pCurrentMap->m_aPlayerDeaths[i];

		//	pKVData = new KeyValues( "tf_deaths" );
		//	pKVData->SetInt( "DeathIndex", i );
		//	pKVData->SetInt( "X", playerDeath.nPosition[0] );
		//	pKVData->SetInt( "Y", playerDeath.nPosition[1] );
		//	pKVData->SetInt( "Z", playerDeath.nPosition[2] );
		//	pKV->AddSubKey( pKVData );
		//}
	}

	// clear stats since we've now reported these
	m_reportedStats.Clear();

	return true;
}

extern CBaseGameStats_Driver CBGSDriver;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameStats::Init( void )
{
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "tf_game_over" );
	ListenForGameEvent( "teamplay_game_over" );	

	// CTF Gameplay Events
	ListenForGameEvent( "teamplay_flag_event" );

	// CP Gameplay Events
	//ListenForGameEvent( "teamplay_point_startcapture" );
	ListenForGameEvent( "teamplay_point_captured" );
	//ListenForGameEvent( "teamplay_capture_blocked" );

	// Player Event
	ListenForGameEvent( "player_disconnect" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void StripNewlineFromString( char *string )
{
	int nStrlength = strlen( string ) - 1;
	if ( nStrlength >= 0 )
	{
		if ( string[nStrlength] == '\n' || string[nStrlength] == '\r' )
			string[nStrlength] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelInit( void )
{
	ClearCurrentGameData();

	// Get the host ip and port.
	int nIPAddr = 0;
	short nPort = 0;
	ConVar *hostip = cvar->FindVar( "hostip" );
	if ( hostip )
	{
		nIPAddr = hostip->GetInt();
	}			

	ConVar *hostport = cvar->FindVar( "hostip" );
	if ( hostport )
	{
		nPort = hostport->GetInt();
	}			

	m_reportedStats.m_pCurrentGame->Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );
	m_reportedStats.m_bValidData = false;

	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	map->Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );

	m_currentRoundRed.m_iNumRounds = 0;

	m_currentMap.Init( STRING( gpGlobals->mapname ), gpGlobals->mapversion, nIPAddr, nPort, gpGlobals->curtime );

	if ( !g_pStringTableServerMapCycle )
		return;

	// Parse the server's mapcycle for playtime tracking (used in voting)
	if ( !m_MapsPlaytime.Count() )
	{
		int index = g_pStringTableServerMapCycle->FindStringIndex( "ServerMapCycle" );
		if ( index != ::INVALID_STRING_INDEX )
		{
			int nLength = 0;
			const char *pszMapCycle = (const char *)g_pStringTableServerMapCycle->GetStringUserData( index, &nLength );
			if ( pszMapCycle && pszMapCycle[0] )
			{
				if ( pszMapCycle && nLength )
				{
					CUtlVector< char * > vecMapCycle;
					V_SplitString( pszMapCycle, "\n", vecMapCycle );

					if ( vecMapCycle.Count() )
					{
						for ( int index = 0; index < vecMapCycle.Count(); index++ )
						{
							// Initialize the list with random playtimes - to vary the first vote options
							int nSeed = RandomInt( 1, 300 );
							StripNewlineFromString( vecMapCycle[index] );

							// Canonicalize map name and ensure we know of it
							char szMapName[ 64 ] = { 0 };
							V_strncpy( szMapName, vecMapCycle[index], sizeof( szMapName ) );
							IVEngineServer::eFindMapResult eResult = engine->FindMap( szMapName, sizeof( szMapName ) );
							switch ( eResult )
							{
							case IVEngineServer::eFindMap_Found:
							case IVEngineServer::eFindMap_NonCanonical:
							case IVEngineServer::eFindMap_FuzzyMatch:
								m_MapsPlaytime.Insert( CUtlConstString( szMapName ), nSeed );
								break;
							case IVEngineServer::eFindMap_NotFound:
								// Don't know the canonical map name for stats here :-/
							case IVEngineServer::eFindMap_PossiblyAvailable:
							default:
								break;
							}
						}
					}

					vecMapCycle.PurgeAndDeleteElements();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens a new server session when a level is started.
//-----------------------------------------------------------------------------
void CTFGameStats::LevelInitPreEntity()
{
	// Start the server session.
	GetSteamWorksSGameStatsUploader().StartSession();
}

//-----------------------------------------------------------------------------
// Purpose: Closes the server session when the level is shutdown.
//-----------------------------------------------------------------------------
void CTFGameStats::LevelShutdownPreClearSteamAPIContext()
{
	// Write server specific end session rows.
	SW_WriteHostsRow();

	// End the server session.
	GetSteamWorksSGameStatsUploader().EndSession();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_LevelShutdown( float flElapsed )
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		flElapsed = gpGlobals->curtime - m_reportedStats.m_pCurrentGame->m_flRoundStartTime;
		m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flElapsed;
	}

	// Store data for the vote system (for issues that present choices based on stats)
	AccumulateVoteData();

	// add current game data in to data for this level
	AccumulateGameData();

	if ( m_bRoundActive )
	{
		m_bRoundActive = false;
		m_iRoundsPlayed++;
		SW_GameStats_WriteRound( -1, false, m_bServerShutdown ? RE_SERVER_SHUTDOWN : RE_SERVER_MAP_CHANGE );
		for ( int iPlayerIndex=1; iPlayerIndex<=MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				SW_GameStats_WritePlayer( pPlayer );
			}
		}
	}

	SW_GameStats_WriteMap();

	if ( m_bServerShutdown )
	{
		StopListeningForAllEvents();
	}

	m_bServerShutdown = false;
	m_bRoundActive = false;
	m_iRoundsPlayed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Resets all stats for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetPlayerStats( CTFPlayer *pPlayer )
{
	int iPlayerIndex = pPlayer->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndex) )
		return;

	PlayerStats_t &stats = m_aPlayerStats[iPlayerIndex];
	// reset the stats on this player
	stats.Reset();
	// reset the matrix of who killed whom with respect to this player
	ResetKillHistory( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the kill history for this player
//-----------------------------------------------------------------------------
void CTFGameStats::ResetKillHistory( CTFPlayer *pPlayer )
{
	int iPlayerIndex = pPlayer->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndex) )
		return;

	// for every other player, set all all the kills with respect to this player to 0
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{
		PlayerStats_t &statsOther = m_aPlayerStats[i];
		statsOther.statsKills.iNumKilled[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledBy[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledByUnanswered[iPlayerIndex] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets per-round stats for all players
//-----------------------------------------------------------------------------
void CTFGameStats::ResetRoundStats()
{
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{		
		m_aPlayerStats[i].statsCurrentRound.Reset();
	}
	m_currentRoundRed.Reset();
	m_currentRoundBlue.Reset();

	IGameEvent *event = gameeventmanager->CreateEvent( "stats_resetround" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increments specified stat for specified player by specified amount
//-----------------------------------------------------------------------------
void CTFGameStats::IncrementStat( CTFPlayer *pPlayer, TFStatType_t statType, int iValue )
{
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() && TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
		return;

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[statType] += iValue;
	stats.statsCurrentRound.m_iStat[statType] += iValue;
	stats.mapStatsCurrentLife.m_iStat[statType] += iValue;
	stats.mapStatsCurrentRound.m_iStat[statType] += iValue;
	stats.statsAccumulated.m_iStat[statType] += iValue;
	stats.mapStatsAccumulated.m_iStat[statType] += iValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::SendStatsToPlayer( CTFPlayer *pPlayer, bool bIsAlive )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// set the play time for the round
	stats.statsCurrentLife.m_iStat[TFSTAT_PLAYTIME] = (int) gpGlobals->curtime - pPlayer->GetSpawnTime();
	stats.statsCurrentLife.m_iStat[TFSTAT_POINTSSCORED] = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife, pPlayer );
	stats.statsCurrentLife.m_iStat[TFSTAT_MAXSENTRYKILLS] = pPlayer->GetMaxSentryKills();
	stats.mapStatsCurrentLife.m_iStat[TFMAPSTAT_PLAYTIME] = (int) gpGlobals->curtime - pPlayer->GetSpawnTime();

	// make a bit field of all the stats we want to send (all with non-zero values)
	int iStat;
	int iSendBits = 0;
	for ( iStat = TFSTAT_FIRST; iStat <= TFSTAT_LAST; iStat++ )
	{
		if ( stats.statsCurrentLife.m_iStat[iStat] > 0 )
		{
			iSendBits |= ( 1 << ( iStat - TFSTAT_FIRST ) );
		}
	}

	iStat = TFSTAT_FIRST;
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	UserMessageBegin( filter, "PlayerStatsUpdate" );
	WRITE_BYTE( pPlayer->GetPlayerClass()->GetClassIndex() );		// write the class
	WRITE_BYTE( bIsAlive );											// write if the player is currently alive
	WRITE_LONG( iSendBits );										// write the bit mask of which stats follow in the message

	// write all the non-zero stats according to the bit mask
	while ( iSendBits > 0 )
	{
		if ( iSendBits & 1 )
		{
			WRITE_LONG( stats.statsCurrentLife.m_iStat[iStat] );
		}
		iSendBits >>= 1;
		iStat ++;
	}
	MessageEnd();

	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t *pMapDef = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( V_strcmp( pMapDef->pszMapName, gpGlobals->mapname.ToCStr() ) == 0 )
		{
			iStat = 0;
			iSendBits = 0;

			for ( iStat = TFMAPSTAT_FIRST; iStat <= TFMAPSTAT_LAST; iStat++ )
			{
				if ( stats.mapStatsCurrentLife.m_iStat[iStat] > 0 )
				{
					iSendBits |= ( 1 << ( iStat - TFMAPSTAT_FIRST ) );
				}
			}

			iStat = TFMAPSTAT_FIRST;

			UserMessageBegin( filter, "MapStatsUpdate" );

			WRITE_UBITLONG( pMapDef->GetStatsIdentifier(), 32 );	// write the mapid
			WRITE_LONG( iSendBits );					// write the bit mask of which stats follow in the message

			// write all the non-zero stats according to the bit mask
			while ( iSendBits > 0 )
			{
				if ( iSendBits & 1 )
				{
					WRITE_LONG( stats.mapStatsCurrentLife.m_iStat[iStat] );
				}
				iSendBits >>= 1;
				iStat ++;
			}

			MessageEnd();
			break;
		}
	}	

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateAndResetPerLifeStats( CTFPlayer *pPlayer )
{
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// add score from previous life and reset current life stats
	int iScore = TFGameRules()->CalcPlayerScore( &stats.statsCurrentLife, pPlayer );
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iScore += iScore;
	}
	stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsAccumulated.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsCurrentLife.Reset();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerConnected( CBasePlayer *pPlayer )
{
	ResetPlayerStats( ToTFPlayer( pPlayer ) );

	PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pPlayer->entindex()];
	stats.iConnectTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDisconnectedTF( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pTFPlayer->entindex()];
	stats.iDisconnectTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	SW_GameStats_WritePlayer( pTFPlayer, true );

	ResetPlayerStats( pTFPlayer );

	if ( pTFPlayer->IsAlive() )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pTFPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iTeamQuit++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerChangedClass( CTFPlayer *pPlayer, int iOldClass, int iNewClass )
{
	if ( iNewClass >= TF_FIRST_NORMAL_CLASS && iNewClass <= TF_LAST_NORMAL_CLASS )
	{
		if ( m_reportedStats.m_pCurrentGame )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iNewClass].iClassChanges += 1;
		}
		IncrementStat( pPlayer, TFSTAT_CLASSCHANGES, 1 );

		// Record this in steamworks stats also...
		SW_ClassChange( pPlayer, iOldClass, iNewClass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerSpawned( CTFPlayer *pPlayer )
{	
	// if player is spawning as a member of valid team, increase the spawn count for his class
	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( TEAM_UNASSIGNED != iTeam && TEAM_SPECTATOR != iTeam )
	{
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iSpawns++;
		}
	}

	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	// calculate peak player count on each team
	for ( iTeam = FIRST_GAME_TEAM; iTeam < TF_TEAM_COUNT; iTeam++ )
	{
		int iPlayerCount = GetGlobalTeam( iTeam )->GetNumPlayers();
		if ( iPlayerCount > map->m_iPeakPlayerCount[iTeam] )
		{
			map->m_iPeakPlayerCount[iTeam] = iPlayerCount;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void CTFGameStats::Event_PlayerForceRespawn( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsAlive() && !TFGameRules()->PrevRoundWasWaitingForPlayers() )
	{		
		// send stats to player
		SendStatsToPlayer( pPlayer, true );

		// if player is alive before respawn, add time from this life to class stat
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pPlayer->GetSpawnTime() );
		}
	}

	AccumulateAndResetPerLifeStats( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerLeachedHealth( CTFPlayer *pPlayer, bool bDispenserHeal, float amount ) 
{
	// make sure value is sane
	Assert( amount >= 0 );
	Assert( amount < 1000 );

	if ( !bDispenserHeal )
	{
		// If this was a heal by enemy medic and the first such heal that the server is aware of for this player,
		// send an achievement event to client.  On the client, it will award achievement if player doesn't have it yet
		PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
		if ( 0 == stats.statsAccumulated.m_iStat[TFSTAT_HEALTHLEACHED] )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_GET_HEALED_BYENEMY );
		}
	}

	IncrementStat( pPlayer, TFSTAT_HEALTHLEACHED, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TF_Gamestats_RoundStats_t* CTFGameStats::GetRoundStatsForTeam( int iTeamNumber )
{
	if ( iTeamNumber == TF_TEAM_BLUE )
		return &m_currentRoundBlue;
	else if ( iTeamNumber == TF_TEAM_RED )
		return &m_currentRoundRed;
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealedOther( CTFPlayer *pPlayer, float amount ) 
{
	// make sure value is sane
	int iAmount = (int) amount;
	Assert( iAmount >= 0 );
	Assert( iAmount <= 1000 );
	if ( iAmount < 0 || iAmount > 1000 )
	{
		DevMsg( "CTFGameStats: bogus healing value of %d reported, ignoring\n", iAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_HEALING, (int) amount );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iHealingDone += amount;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  How much health effects like mad milk generate - awarded to the provider
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealedOtherAssist( CTFPlayer *pPlayer, float amount ) 
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pPlayer && pMatch )
	{
		// Anti-farming in official matchmaking modes
		if ( gpGlobals->curtime - pPlayer->GetLastDamageReceivedTime() > 90.f ||
			 gpGlobals->curtime - pPlayer->GetLastEntityDamagedTime() > 90.f )
		{
			return;
		}
	}

	Assert ( pPlayer );

	// make sure value is sane
	int iAmount = (int) amount;
	Assert( iAmount >= 0 );
	Assert( iAmount <= 1000 );
	if ( iAmount < 0 || iAmount > 1000 )
	{
		DevMsg( "CTFGameStats: bogus healing value of %d reported, ignoring\n", iAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_HEALING_ASSIST, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose:  Raw damage blocked due to effects like invuln, projectile shields, etc
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerBlockedDamage( CTFPlayer *pPlayer, int nAmount ) 
{
	Assert( pPlayer && nAmount > 0 && nAmount < 3000 );
	if ( nAmount < 0 || nAmount > 3000 )
	{
		DevMsg( "CTFGameStats: bogus blocked damage value of %d reported, ignoring\n", nAmount );
		return;
	}
	IncrementStat( pPlayer, TFSTAT_DAMAGE_BLOCKED, nAmount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistKill( CTFPlayer *pAttacker, CBaseEntity *pVictim )
{
	// increment player's stat
	IncrementStat( pAttacker, TFSTAT_KILLASSISTS, 1 );
	// increment reported class stats
	int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iAssists++;

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iAssists++;
		}
	}

	if ( pVictim->IsPlayer() )
	{
		// keep track of how many times every player kills every other player
		CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
		TrackKillStats( pAttacker, pPlayerVictim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerInvulnerable( CTFPlayer *pPlayer ) 
{
	IncrementStat( pPlayer, TFSTAT_INVULNS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iInvulns++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCreatedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSBUILT, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBuildingsBuilt++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDestroyedBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_BUILDINGSDESTROYED, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBuildingsDestroyed++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_AssistDestroyBuilding( CTFPlayer *pPlayer, CBaseObject *pBuilding )
{
	// sappers are buildings from the code's point of view but not from the player's, don't count them
	CObjectSapper *pSapper = dynamic_cast<CObjectSapper *>( pBuilding );
	if ( pSapper )
		return;

	IncrementStat( pPlayer, TFSTAT_KILLASSISTS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Headshot( CTFPlayer *pKiller, bool bBowShot )
{
	IncrementStat( pKiller, TFSTAT_HEADSHOTS, 1 );
	if ( bBowShot ) // Extra points for a bow shot.
	{
		IncrementStat( pKiller, TFSTAT_BONUS_POINTS, 5 ); // Extra point.
	}

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iHeadshots++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_Backstab( CTFPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_BACKSTABS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iBackstabs++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerStunBall( CTFPlayer *pAttacker, bool bSpecial )
{
	if ( bSpecial )
	{
		IncrementStat( pAttacker, TFSTAT_BONUS_POINTS, 20 );
	}
	else
	{
		IncrementStat( pAttacker, TFSTAT_BONUS_POINTS, 10 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerAwardBonusPoints( CTFPlayer *pPlayer, CBaseEntity *pSource, int nCount )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pPlayer && pMatch )
	{
		// Anti-farming in official matchmaking modes
		if ( gpGlobals->curtime - pPlayer->GetLastDamageReceivedTime() > 90.f ||
			 gpGlobals->curtime - pPlayer->GetLastEntityDamagedTime() > 90.f )
		{
			 return;
		}
	}

	IncrementStat( pPlayer, TFSTAT_BONUS_POINTS, nCount );

	// This event ends up drawing a combattext number
	if ( pPlayer && pSource )
	{
		if ( nCount >= 10 )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			filter.MakeReliable();
			UserMessageBegin( filter, "PlayerBonusPoints" );
			WRITE_BYTE( nCount );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pSource->entindex() );
			MessageEnd();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerHealthkitPickup( CTFPlayer *pTFPlayer )
{
	IncrementStat( pTFPlayer, TFSTAT_HEALTHKITS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerAmmokitPickup( CTFPlayer *pTFPlayer )
{
	IncrementStat( pTFPlayer, TFSTAT_AMMOKITS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerUsedTeleport( CTFPlayer *pTeleportOwner, CTFPlayer *pTeleportingPlayer )
{
	// We don't count the builder's teleports
	if ( pTeleportOwner && pTeleportOwner != pTeleportingPlayer )
	{
		IncrementStat( pTeleportOwner, TFSTAT_TELEPORTS, 1 );

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pTeleportOwner->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iTeleports++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerFiredWeapon( CTFPlayer *pPlayer, bool bCritical ) 
{
	// If normal gameplay state, track weapon stats. 
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		CTFWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			// record shots fired in reported per-weapon stats
			int iWeaponID = pTFWeapon->GetWeaponID();

			if ( m_reportedStats.m_pCurrentGame != NULL )
			{
				TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[iWeaponID];
				pWeaponStats->iShotsFired++;
				if ( bCritical )
				{
					pWeaponStats->iCritShotsFired++;
					TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
					if ( round )
					{
						round->m_Summary.iCrits++;
					}
					IncrementStat( pPlayer, TFSTAT_CRITS, 1 );
				}
			}

			// need a better place to do this
			pPlayer->OnMyWeaponFired( pTFWeapon );

			// inform the bots
			TheNextBots().OnWeaponFired( pPlayer, pTFWeapon );
		}
	}

	IncrementStat( pPlayer, TFSTAT_SHOTS_FIRED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	// defensive guard against insanely huge damage values that apparently get into the stats system once in a while -- ignore insane values
	const int INSANE_PLAYER_DAMAGE = TFGameRules()->IsMannVsMachineMode() ? 5000 : 1500;

	if ( sv_cheats && !sv_cheats->GetBool() )
	{
		Assert( iDamageTaken >= 0 );
	}
	if ( ( iDamageTaken < 0 ) || ( iDamageTaken > INSANE_PLAYER_DAMAGE ) )
		return;

	CObjectSentrygun *pSentry = NULL;
	CTFPlayer *pTarget = ToTFPlayer( pBasePlayer );
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( info.GetAttacker() );
		if ( !pSentry )
			return;

		pAttacker = pSentry->GetOwner();
	}
	// don't count damage to yourself
	if ( pTarget == pAttacker )
		return;

	if ( pAttacker )
	{
		IncrementStat( pAttacker, TFSTAT_DAMAGE, iDamageTaken );

		if ( info.GetDamageType() & (DMG_BURN | DMG_IGNITE) )
		{
			IncrementStat( pAttacker, TFSTAT_FIREDAMAGE, iDamageTaken );
		}
		if ( info.GetDamageType() & DMG_BLAST )
		{
			IncrementStat( pAttacker, TFSTAT_BLASTDAMAGE, iDamageTaken );
		}
		// Ranged stats
		if ( !( info.GetDamageType() & DMG_MELEE ) )
		{
			IncrementStat( pAttacker, TFSTAT_DAMAGE_RANGED, iDamageTaken );

			if ( info.GetDamageType() & DMG_CRITICAL )
			{
				if ( pAttacker->m_Shared.IsCritBoosted() )
				{
					IncrementStat( pAttacker, TFSTAT_DAMAGE_RANGED_CRIT_BOOSTED, iDamageTaken );

				}
				else
				{
					IncrementStat( pAttacker, TFSTAT_DAMAGE_RANGED_CRIT_RANDOM, iDamageTaken );
				}
			}
		}

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iDamageDone += iDamageTaken;
		}

		//Report MvM damage to bots
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats )
			{
				if ( pTarget && pTarget->IsBot() )
				{
					if ( pTarget->IsMiniBoss() )
					{
						pStats->PlayerEvent_DealtDamageToGiants( pAttacker, iDamageTaken );
					}
					else
					{
						pStats->PlayerEvent_DealtDamageToBots( pAttacker, iDamageTaken );
					}
				}
			}
		}
	}

	if ( pTarget )
	{
		IncrementStat( pTarget, TFSTAT_DAMAGETAKEN, iDamageTaken );
	}

	TF_Gamestats_LevelStats_t::PlayerDamageLump_t damage;
	Vector killerOrg(0, 0, 0);

	// set the location where the target was hit
	const Vector &org = pTarget ? pTarget->GetAbsOrigin() : vec3_origin;
	damage.nTargetPosition[ 0 ] = static_cast<int>( org.x );
	damage.nTargetPosition[ 1 ] = static_cast<int>( org.y );
	damage.nTargetPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pAttacker, pInflictor, pTarget );

	if ( !pSentry )
	{
		pSentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
	}

	if ( pSentry != NULL )
	{
		killerOrg = pSentry->GetAbsOrigin();
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = ( info.GetDamageType() & DMG_BLAST ) ? TF_WEAPON_SENTRY_ROCKET : TF_WEAPON_SENTRY_BULLET;
	} 
	else if ( dynamic_cast<CObjectDispenser *>( pInflictor ) )
	{
		damage.iAttackClass = TF_CLASS_ENGINEER;
		damage.iWeapon = TF_WEAPON_DISPENSER;
	}
	else
	{
		CTFPlayer *pTFAttacker = ToTFPlayer( pScorer );
		if ( pTFAttacker )
		{
			CTFPlayerClass *pAttackerClass = pTFAttacker->GetPlayerClass();
			damage.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pTFAttacker->GetAbsOrigin();
		}
		else
		{
			damage.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}

		// find the weapon the killer used
		damage.iWeapon = GetWeaponFromDamage( info );
	}

	// If normal gameplay state, track weapon stats. 
	if ( ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( damage.iWeapon != TF_WEAPON_NONE  ) )
	{
		// record hits & damage in reported per-weapon stats
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			TF_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[damage.iWeapon];
			pWeaponStats->iHits++;
			pWeaponStats->iTotalDamage += iDamageTaken;

			// Try and figure out where the damage is coming from
			Vector vecDamageOrigin = info.GetReportedPosition();
			// If we didn't get an origin to use, try using the attacker's origin
			if ( vecDamageOrigin == vec3_origin )
			{
				if ( pSentry )
				{
					vecDamageOrigin = pSentry->GetAbsOrigin();
				}
				else
				{
					vecDamageOrigin = killerOrg;
				}					
			}
			if ( vecDamageOrigin != vec3_origin )
			{
				pWeaponStats->iHitsWithKnownDistance++;
				int iDistance = (int) vecDamageOrigin.DistTo( pBasePlayer->GetAbsOrigin() );
				//				Msg( "Damage distance: %d\n", iDistance );
				pWeaponStats->iTotalDistance += iDistance;
			}
		}
	}

	Assert( damage.iAttackClass != TF_CLASS_UNDEFINED );

	// record the time the damage occurred
	damage.fTime = gpGlobals->curtime;

	// store the attacker's position
	damage.nAttackerPosition[ 0 ] = static_cast<int>( killerOrg.x );
	damage.nAttackerPosition[ 1 ] = static_cast<int>( killerOrg.y );
	damage.nAttackerPosition[ 2 ] = static_cast<int>( killerOrg.z );

	// set the class of the target
	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	damage.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	Assert( damage.iTargetClass != TF_CLASS_UNDEFINED );

	// record the damage done
	damage.iDamage = info.GetDamage();

	// record if it was a crit
	damage.iCrit = ( ( info.GetDamageType() & DMG_CRITICAL ) != 0 );

	// record if it was a kill
	damage.iKill = ( pTarget->GetHealth() <= 0 );

	// add it to the list of damages
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		//m_reportedStats.m_pCurrentGame->m_aPlayerDamage.AddToTail( damage );
		m_reportedStats.m_pCurrentGame->m_bIsRealServer = true;
	}	
}

//-----------------------------------------------------------------------------
// Purpose:  How much damage effects like jarate add - awarded to the provider
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDamageAssist( CBasePlayer *pProvider, int iBonusDamage )
{
	Assert( pProvider );

	const int INSANE_PLAYER_DAMAGE = 5000;
	Assert( iBonusDamage >= 0 );
	Assert( iBonusDamage <= INSANE_PLAYER_DAMAGE );
	if ( iBonusDamage < 0 || iBonusDamage > INSANE_PLAYER_DAMAGE )
		return;

	CTFPlayer *pTFProvider = ToTFPlayer( pProvider );
	if ( pTFProvider )
	{
		IncrementStat( pTFProvider, TFSTAT_DAMAGE_ASSIST, iBonusDamage );
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Damage done to all boss types
//-----------------------------------------------------------------------------
void CTFGameStats::Event_BossDamage( CBasePlayer *pAttacker, int iDamage )
{
	const int INSANE_DAMAGE = 5000;
	Assert( iDamage >= 0 );
	Assert( iDamage <= INSANE_DAMAGE );
	if ( iDamage < 0 || iDamage > INSANE_DAMAGE )
		return;

	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
	if ( pTFAttacker )
	{
		IncrementStat( pTFAttacker, TFSTAT_DAMAGE_BOSS, iDamage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerSuicide( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = dynamic_cast<CTFPlayer*>( pPlayer );
	if ( pTFPlayer )
	{
		IncrementStat( pTFPlayer, TFSTAT_SUICIDES, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// This also gets called when the victim is a building.  That gets tracked separately as building destruction, don't count it here
	if ( !pVictim->IsPlayer() )
		return;

	CTFPlayer *pPlayerAttacker = static_cast< CTFPlayer * >( pAttacker );

	IncrementStat( pPlayerAttacker, TFSTAT_KILLS, 1 );

	// keep track of how many times every player kills every other player
	CTFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
	TrackKillStats( pAttacker, pPlayerVictim );

	int iClass = pPlayerAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iKills++;

		TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
		if ( round )
		{
			round->m_Summary.iKills++;
		}
	}

	// Throwable Kill tracking
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_THROWABLE )
	{
		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( pTFAttacker )
		{
			Event_PlayerThrowableKill( pTFAttacker );
		}
	}

	// Scouts get additional points for killing medics that were actively healing.
	if ( (iClass == TF_CLASS_SCOUT) && pPlayerVictim && (pPlayerVictim->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC) )
	{
		// Determine if the medic was using their (now holstered) heal gun.
		CWeaponMedigun *pMedigun = (CWeaponMedigun *) pPlayerVictim->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
		if ( pMedigun && pMedigun->m_bWasHealingBeforeDeath )
		{
			IncrementStat( pPlayerAttacker, TFSTAT_BONUS_POINTS, 10 );
		}
	}

	// Players get points for killing a Rune carrier
	if ( pPlayerVictim && pPlayerVictim->m_Shared.IsCarryingRune() )
	{
		IncrementStat( pPlayerAttacker, TFSTAT_KILLS_RUNECARRIER, 1 );
	}

}

void CTFGameStats::Event_KillDetail( CTFPlayer* pKiller, CTFPlayer* pVictim, CTFPlayer* pAssister, IGameEvent* event /*player_death*/, const CTakeDamageInfo &info )
{
	SW_GameStats_WriteKill( pKiller, pVictim, pAssister, event, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_RoundStart()
{
	m_iGameEndReason = 0;
	m_currentRoundRed.Reset();
	m_currentRoundBlue.Reset();
	m_bRoundActive = true;
	m_iKillCount = 0;
	m_iPlayerUpdates = 0;
	m_currentRoundRed.m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();

	m_rdStats.Clear();
	m_passtimeStats.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin )
{
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	Assert( map );
	if ( !map )
		return;

	m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flRoundTime;
	m_reportedStats.m_pCurrentGame->m_flRoundStartTime = gpGlobals->curtime;

//	if ( !bFullRound )
//		return;

	map->m_Header.m_iRoundsPlayed++;
	switch ( iWinningTeam )
	{
	case TF_TEAM_RED:
		map->m_Header.m_iRedWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iRedSuddenDeathWins++;
		}
		break;
	case TF_TEAM_BLUE:
		map->m_Header.m_iBlueWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iBlueSuddenDeathWins++;
		}
		break;
	case TEAM_UNASSIGNED:
		map->m_Header.m_iStalemates++;
		break;
	default:
		Assert( false );
		break;
	}

	// Determine which control point was last captured
	if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP || TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT || TFGameRules()->GetGameType() == TF_GAMETYPE_ARENA ) )
	{
		int iLastCP = TFGameRules()->GetLastCapPointChanged();
		if ( iLastCP >= 0 && iLastCP <= MAX_CONTROL_POINTS )
		{
			map->m_Header.m_iLastCapChangedInRound[iLastCP]++;
		}
	}

	// add current game data in to data for this level
	AccumulateGameData();

	m_bRoundActive = false;
	m_iRoundsPlayed++;
	SW_GameStats_WriteRound( iWinningTeam, bFullRound, RE_ROUND_END );
	for ( int iPlayerIndex=1; iPlayerIndex<=MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer )
		{
			SW_GameStats_WritePlayer( pPlayer );
		}
	}

	SW_PasstimeRoundEnded();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_GameEnd()
{
	// Send a stats update to all players who are still alive.  (Dead one have already
	// received theirs when they died.)
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			SendStatsToPlayer( pPlayer, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCapturedPoint( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, 1 );
	// increment reported stats
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iCaptures++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerReturnedFlag( CTFPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_FLAGRETURNS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerScoresEscortPoints( CTFPlayer *pPlayer, int iPoints )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, iPoints );

	SW_GameEvent( pPlayer, "escort_scored", iPoints );

	// increment reported stats
	//int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	//if ( m_reportedStats.m_pCurrentGame != NULL )
	//{
	//	m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iCaptures++;
	//}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDefendedPoint( CTFPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_DEFENSES, 1 );

	ConVarRef tf_gamemode_cp( "tf_gamemode_cp" );
	ConVarRef tf_gamemode_payload( "tf_gamemode_payload" );
	if ( tf_gamemode_cp.GetInt() == 1 )
	{
		SW_GameEvent( pPlayer, "point_blocked", 1 );
	}
	else if ( tf_gamemode_payload.GetInt() == 1 )
	{
		SW_GameEvent( pPlayer, "escort_blocked", 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerDominatedOther( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_DOMINATIONS, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iDominations++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerRevenge( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_REVENGE, 1 );

	TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pAttacker->GetTeamNumber() );
	if ( round )
	{
		round->m_Summary.iRevenges++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	Assert( pPlayer );
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	IncrementStat( pTFPlayer, TFSTAT_DEATHS, 1 );
	SendStatsToPlayer( pTFPlayer, false );

	// TF_Gamestats_LevelStats_t::PlayerDeathsLump_t death;
	Vector killerOrg;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	// death.nPosition[ 0 ] = static_cast<int>( org.x );
	// death.nPosition[ 1 ] = static_cast<int>( org.y );
	// death.nPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, pPlayer ) );

	if ( pInflictor && pInflictor->IsBaseObject() && dynamic_cast< CObjectSentrygun * >( pInflictor ) != NULL )
	{
		killerOrg = pInflictor->GetAbsOrigin();
	}
	else
	{		
		if ( pScorer )
		{
			// CTFPlayerClass *pAttackerClass = pScorer->GetPlayerClass();
			// death.iAttackClass = ( !pAttackerClass ) ? TF_CLASS_UNDEFINED : pAttackerClass->GetClassIndex();
			killerOrg = pScorer->GetAbsOrigin();
		}
		else
		{
			// death.iAttackClass = TF_CLASS_UNDEFINED;
			killerOrg = org;
		}
	}

	// set the class of the target
	// CTFPlayerClass *pTargetClass = ( pTFPlayer ) ? pTFPlayer->GetPlayerClass() : NULL;
	// death.iTargetClass = ( !pTargetClass ) ? TF_CLASS_UNDEFINED : pTargetClass->GetClassIndex();

	// find the weapon the killer used
	// death.iWeapon = GetWeaponFromDamage( info );

	// calculate the distance to the killer
	// death.iDistance = static_cast<unsigned short>( ( killerOrg - org ).Length() );

	// add it to the list of deaths
	TF_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( map )
	{
		//const int MAX_REPORTED_DEATH_COORDS = 2000;	// limit list of death coords reported so it doesn't grow unbounded.
		//if ( map->m_aPlayerDeaths.Count() < MAX_REPORTED_DEATH_COORDS )
		//{
		//	map->m_aPlayerDeaths.AddToTail( death );
		//}

		int iClass = ToTFPlayer( pPlayer )->GetPlayerClass()->GetClassIndex();

		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iDeaths++;
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );

			TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pPlayer->GetTeamNumber() );
			if ( round )
			{
				round->m_Summary.iDeaths++;
			}
			if ( pKiller != NULL && pKiller == pPlayer )
			{
				TF_Gamestats_RoundStats_t* round = GetRoundStatsForTeam( pKiller->GetTeamNumber() );
				if ( round )
				{
					round->m_Summary.iSuicides++;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFGameStats::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		StoreGameEndReason( event->GetString( "reason" ) );

		Event_GameEnd();
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		StoreGameEndReason( event->GetString( "reason" ) );

		Event_GameEnd();
	}
	else if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		Event_RoundStart();
	}
	else if ( Q_strcmp( "teamplay_flag_event", pEventName ) == 0 )
	{
		SW_FlagEvent( event->GetInt( "player" ), event->GetInt( "carrier"), event->GetInt( "eventtype") );
	}
	else if ( Q_strcmp( "teamplay_point_startcapture", pEventName ) == 0 )
	{
		// Not sure this is necessary to track, since it's only sent when a cap is freshly started.
		// Should probably see what the community needs are or what our needs are after we release the initial set of stats.
		/*
		const char *cappers = event->GetString( "cappers" );
		for ( int i = 0; i < Q_strlen( cappers ); i++ )
		{
			SW_CapEvent( event->GetInt( "cp" ), cappers[i], "point_start_capture", 0 );
		}
		*/
	}
	else if ( Q_strcmp( "teamplay_point_captured", pEventName ) == 0 )
	{
		const char *cappers = event->GetString( "cappers" );
		for ( int i = 0; i < Q_strlen( cappers ); i++ )
		{
			SW_CapEvent( event->GetInt( "cp" ), cappers[i], "point_captured", 1 );
		}
	}
	else if ( Q_strcmp( "player_disconnect", pEventName ) == 0 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt("userid") ) );
		if ( pPlayer )
		{
			CTF_GameStats.Event_PlayerLoadoutChanged( pPlayer, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTFGameStats::StoreGameEndReason( const char* reason )
{
	if ( Q_strcmp( reason, "Reached Time Limit" ) == 0 )
	{
		m_iGameEndReason = RE_TIME_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Win Limit" ) == 0 )
	{
		m_iGameEndReason = RE_WIN_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Win Difference Limit" ) == 0 )
	{
		m_iGameEndReason = RE_WIN_DIFF_LIMIT;
	}
	else if ( Q_strcmp( reason, "Reached Round Limit" ) == 0 )
	{
		m_iGameEndReason = RE_ROUND_LIMIT;
	}
	else if ( Q_strcmp( reason, "NextLevel CVAR" ) == 0 )
	{
		m_iGameEndReason = RE_NEXT_LEVEL_CVAR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateGameData()
{
	// find or add a bucket for this level
	TF_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	// get current game data
	TF_Gamestats_LevelStats_t *game = m_reportedStats.m_pCurrentGame;
	if ( !map || !game )
		return;

	if ( IsRealGameplay( game ) )
	{
		// if this looks like real game play, add it to stats
		map->Accumulate( game );
		m_reportedStats.m_bValidData = true;
	}
	m_currentMap.Accumulate( game ); // Steamworks stats always accumulate.

	ClearCurrentGameData();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameStats::AccumulateVoteData( void )
{
	if ( !g_voteControllerGlobal || !g_voteControllerRed || !g_voteControllerBlu )
		return;

	if ( !g_pStringTableServerMapCycle )
		return;

	// Find the current map and update playtime
	int iIndex = m_MapsPlaytime.Find( CUtlConstString( STRING( gpGlobals->mapname ) ) );
	if ( iIndex != m_MapsPlaytime.InvalidIndex() )
	{
		TF_Gamestats_LevelStats_t *CurrentGame = m_reportedStats.m_pCurrentGame;
		//Msg( "%s -- old: %i  ", STRING( gpGlobals->mapname ), m_MapsPlaytime[iIndex] );
		m_MapsPlaytime[iIndex] += CurrentGame->m_Header.m_iTotalTime;
		//Msg( "new: %i\n", STRING( gpGlobals->mapname ), m_MapsPlaytime[iIndex] );
	}
}

struct MapNameAndPlaytime_t
{
	const char* szName;
	int nTime;
};

// Returns negative if elem2 > elem1, positive if elem2 < elem1, and zero if elem 1 == elem2
static int __cdecl SortMapPlaytime( const void *elem1, const void *elem2 )
{
	int time1 = static_cast< const MapNameAndPlaytime_t * >( elem1 )->nTime;
	int time2 = static_cast< const MapNameAndPlaytime_t * >( elem2 )->nTime;
	
	if ( time2 < time1 )
		return -1;

	if ( time2 > time1 )
		return 1;
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:  Method used by the vote system to retrieve various data, like map playtime
//-----------------------------------------------------------------------------
bool CTFGameStats::GetVoteData( const char *szIssueName, int nNumOptions, CUtlVector <const char*> &vecNames )
{
	// Feeds lowest playtime maps to the vote system to present as options
	if ( Q_strcmp( szIssueName, "NextLevel" ) == 0 )
	{
		// This can only happen if we don't get any maps from the mapcycle file
		if ( !m_MapsPlaytime.Count() )
			return false;

		vecNames.EnsureCapacity( MIN( nNumOptions, (int) m_MapsPlaytime.Count() ) );

		// What's the next map in the mapcycle? Place that first in the output
		m_szNextMap[0] = '\0';
		CMultiplayRules *pRules = dynamic_cast< CMultiplayRules * >( GameRules() );
		if ( pRules )
		{
			pRules->GetNextLevelName( m_szNextMap, sizeof( m_szNextMap ) );
			if ( m_szNextMap[0] )
			{
				vecNames.AddToTail( m_szNextMap );
			}
		}

		CUtlVector< MapNameAndPlaytime_t > vecMapsAndPlaytime;
		vecMapsAndPlaytime.EnsureCapacity( m_MapsPlaytime.Count() );

		// Feed the maps into a vector for sorting
		FOR_EACH_MAP_FAST( m_MapsPlaytime, iIndex )
		{
			const char *szItemName = m_MapsPlaytime.Key( iIndex ).Get();
			int nItemTime = m_MapsPlaytime.Element( iIndex );
			// Exclude the next map (already added) and the current map (omitted)
			if ( Q_strcmp( szItemName, m_szNextMap ) != 0 &&
				 Q_strcmp( szItemName, STRING( gpGlobals->mapname ) ) != 0 )
			{
				int iVec = vecMapsAndPlaytime.AddToTail();
				vecMapsAndPlaytime[ iVec ].szName = szItemName;
				vecMapsAndPlaytime[ iVec ].nTime = nItemTime;
			}
		}
		qsort( vecMapsAndPlaytime.Base(), vecMapsAndPlaytime.Count(), sizeof( MapNameAndPlaytime_t ), SortMapPlaytime );

		// Copy sorted maps to output until we have got enough options
		FOR_EACH_VEC( vecMapsAndPlaytime, iVec )
		{
			if ( vecNames.Count() >= nNumOptions )
				break;
			vecNames.AddToTail( vecMapsAndPlaytime[ iVec ].szName );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Clears data for current game
//-----------------------------------------------------------------------------
void CTFGameStats::ClearCurrentGameData()
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		delete m_reportedStats.m_pCurrentGame;
	}
	m_reportedStats.m_pCurrentGame = new TF_Gamestats_LevelStats_t;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the stats of who has killed whom
//-----------------------------------------------------------------------------
void CTFGameStats::TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim )
{
	int iPlayerIndexAttacker = pAttacker->entindex();
	int iPlayerIndexVictim = pVictim->entindex();
	
	if ( !IsIndexIntoPlayerArrayValid(iPlayerIndexAttacker) || !IsIndexIntoPlayerArrayValid(iPlayerIndexVictim) )
		return;

	PlayerStats_t &statsAttacker = m_aPlayerStats[iPlayerIndexAttacker];
	PlayerStats_t &statsVictim = m_aPlayerStats[iPlayerIndexVictim];

	statsVictim.statsKills.iNumKilledBy[iPlayerIndexAttacker]++;
	statsVictim.statsKills.iNumKilledByUnanswered[iPlayerIndexAttacker]++;
	statsAttacker.statsKills.iNumKilled[iPlayerIndexVictim]++;
	statsAttacker.statsKills.iNumKilledByUnanswered[iPlayerIndexVictim] = 0;
}

struct PlayerStats_t *CTFGameStats::FindPlayerStats( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	return &m_aPlayerStats[pPlayer->entindex()];
}

bool CTFGameStats::IsRealGameplay( TF_Gamestats_LevelStats_t *game )
{
	// Sanity-check that this looks like real game play -- must have minimum # of players on both teams,
	// minimum time and some damage to players must have occurred
	if ( tf_stats_nogameplaycheck.GetInt() )
		return true;

	bool bIsRealGameplay = ( 
		( game->m_iPeakPlayerCount[TF_TEAM_RED] >= TFGameRules()->GetStatsMinimumPlayers()  ) &&
		( game->m_iPeakPlayerCount[TF_TEAM_BLUE] >= TFGameRules()->GetStatsMinimumPlayers() ) &&
		( game->m_Header.m_iTotalTime >= TFGameRules()->GetStatsMinimumPlayedTime() ) && ( game->m_bIsRealServer ) 
		);

	return bIsRealGameplay;
}

//-----------------------------------------------------------------------------
// Purpose: //Deprecated
//-----------------------------------------------------------------------------
//static void CC_ListDeaths( const CCommand &args )
//{
//	if ( !UTIL_IsCommandIssuedByServerAdmin() )
//		return;
//
//	Msg( "Command Deprecated");
//
//	//TF_Gamestats_LevelStats_t *map = CTF_GameStats.m_reportedStats.m_pCurrentGame;
//	//if ( !map )
//	//	return;
//
//	//for( int i = 0; i < map->m_aPlayerDeaths.Count(); i++ )
//	//{
//	//	Msg( "%s killed %s with %s at (%d,%d,%d), distance %d\n",
//	//		g_aClassNames[ map->m_aPlayerDeaths[ i ].iAttackClass ],
//	//		g_aClassNames[ map->m_aPlayerDeaths[ i ].iTargetClass ],
//	//		WeaponIdToAlias( map->m_aPlayerDeaths[ i ].iWeapon ), 
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
//	//		map->m_aPlayerDeaths[ i ].nPosition[ 2 ],
//	//		map->m_aPlayerDeaths[ i ].iDistance );
//	//}
//
//	//Msg( "\n---------------------------------\n\n" );
//
//	//for( int i = 0; i < map->m_aPlayerDamage.Count(); i++ )
//	//{
//	//	Msg( "%.2f : %s at (%d,%d,%d) caused %d damage to %s with %s at (%d,%d,%d)%s%s\n",
//	//		map->m_aPlayerDamage[ i ].fTime,
//	//		g_aClassNames[ map->m_aPlayerDamage[ i ].iAttackClass ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 0 ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 1 ],
//	//		map->m_aPlayerDamage[ i ].nAttackerPosition[ 2 ],
//	//		map->m_aPlayerDamage[ i ].iDamage,
//	//		g_aClassNames[ map->m_aPlayerDamage[ i ].iTargetClass ],
//	//		WeaponIdToAlias( map->m_aPlayerDamage[ i ].iWeapon ), 
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 0 ],
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 1 ],
//	//		map->m_aPlayerDamage[ i ].nTargetPosition[ 2 ],
//	//		map->m_aPlayerDamage[ i ].iCrit ? ", CRIT!" : "",
//	//		map->m_aPlayerDamage[ i ].iKill ? ", KILL" : ""	);
//	//}
//
//	//Msg( "\n---------------------------------\n\n" );
//	//Msg( "listed %d deaths\n", map->m_aPlayerDeaths.Count() );
//	//Msg( "listed %d damages\n\n", map->m_aPlayerDamage.Count() );
//}
//
//static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", FCVAR_DEVELOPMENTONLY );

CON_COMMAND_F( tf_dumpplayerstats, "Dumps current player stats", FCVAR_DEVELOPMENTONLY )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsConnected() )
		{
			PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pPlayer->entindex()];
			Msg( "%s:\n", pPlayer->GetPlayerName() );
			for ( int iStat = TFSTAT_FIRST; iStat <= TFSTAT_LAST; iStat++ )
			{
				Msg( "   Stat %d = %d (round), %d (map)\n", iStat, stats.statsCurrentRound.m_iStat[iStat], stats.statsAccumulated.m_iStat[iStat] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Map Data
//-----------------------------------------------------------------------------
void CTFGameStats::SW_GameStats_WriteMap()
{
#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ServerMaps" );
	pKVData->SetInt( "MapIndex", CBGSDriver.m_iNumLevels );
	pKVData->SetInt( "StartTime", m_currentMap.m_iMapStartTime );
	pKVData->SetInt( "EndTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	pKVData->SetString( "MapID", m_currentMap.m_Header.m_szMapName );

	const char* pszGameTypeID = GetGameTypeID();
	if ( pszGameTypeID )
	{
		pKVData->SetString( "GameTypeID", pszGameTypeID  );
	}

	pKVData->SetInt( "RoundsPlayed", m_iRoundsPlayed );

	if ( m_currentMap.m_Header.m_iBlueWins > 0 )
	{
		pKVData->SetInt( "BlueWins", m_currentMap.m_Header.m_iBlueWins );
	}

	if ( m_currentMap.m_Header.m_iRedWins > 0 )
	{
		pKVData->SetInt( "RedWins", m_currentMap.m_Header.m_iRedWins );
	}

	int iRedScore = GetGlobalTFTeam( TF_TEAM_RED )->GetScore();
	if ( iRedScore )
	{
		pKVData->SetInt( "RedScore", iRedScore );
	}

	int iBlueScore = GetGlobalTFTeam( TF_TEAM_BLUE )->GetScore();
	if ( iBlueScore )
	{
		pKVData->SetInt( "BlueScore", iBlueScore );
	}

	if ( m_currentMap.m_Header.m_iStalemates > 0 )
	{
		pKVData->SetInt( "Stalemates", m_currentMap.m_Header.m_iStalemates );
	}

	if ( m_currentMap.m_Header.m_iBlueSuddenDeathWins > 0 )
	{
		pKVData->SetInt( "BlueSuddenDeathWins", m_currentMap.m_Header.m_iBlueSuddenDeathWins );
	}

	if ( m_currentMap.m_Header.m_iRedSuddenDeathWins > 0 )
	{
		pKVData->SetInt( "RedSuddenDeathWins", m_currentMap.m_Header.m_iRedSuddenDeathWins );
	}

	if ( m_bServerShutdown )
	{
		m_iGameEndReason = RE_SERVER_SHUTDOWN;
	}
	else if ( !m_iGameEndReason )
	{
		m_iGameEndReason = RE_SERVER_MAP_CHANGE;
	}
	int iReason = clamp( m_iGameEndReason, 0, MAX_ROUND_END_REASON-1 );
	pKVData->SetString( "EndReason", g_aRoundEndReasons[iReason] );

	pKVData->SetInt( "MapVersion", m_currentMap.m_Header.m_nMapRevision );

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Round Data
//-----------------------------------------------------------------------------
void CTFGameStats::SW_GameStats_WriteRound( int iWinningTeam, bool bFullRound, int iEndReason )
{
#if !defined(NO_STEAM)
	// Flush data gathered so far...
	GetSteamWorksSGameStatsUploader().FlushStats();

	// Round info.
	KeyValues* pKVData = new KeyValues( "TF2ServerRounds" );
	pKVData->SetInt( "MapIndex", 0 );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed );

	const char* pszGameTypeID = GetGameTypeID();
	if ( pszGameTypeID )
	{
		pKVData->SetString( "GameTypeID", pszGameTypeID  );
	}

	pKVData->SetInt( "EndTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	pKVData->SetInt( "StartTime", m_currentRoundRed.m_iRoundStartTime );

	pKVData->SetString( "EndReason", ClampedArrayElement( g_aRoundEndReasons, iEndReason ) );

	iWinningTeam = clamp( iWinningTeam, 0, TF_TEAM_COUNT - 1 );
	pKVData->SetString( "WinningTeam", ClampedArrayElement( g_aTeamNames, iWinningTeam ) );

	if ( bFullRound )
	{
		pKVData->SetInt( "FullRound", bFullRound );
	}

	int nRoundsRemaining = 0;
	if ( ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT ) && TFGameRules()->HasMultipleTrains() )
	{
		if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] && g_hControlPointMasters[0]->PlayingMiniRounds() )
		{
			nRoundsRemaining = g_hControlPointMasters[0]->NumPlayableControlPointRounds();
		}
	}
	else
	{
		if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
		{
			nRoundsRemaining = g_hControlPointMasters[0]->NumPlayableControlPointRounds();
		}
	}
	if ( nRoundsRemaining )
	{
		pKVData->SetInt( "RoundsRemaining", nRoundsRemaining );
	}

	int iRedScore = GetGlobalTFTeam( TF_TEAM_RED )->GetScore();
	if ( iRedScore )
	{
		pKVData->SetInt( "RedScore", iRedScore );
	}

	int iBlueScore = GetGlobalTFTeam( TF_TEAM_BLUE )->GetScore();
	if ( iBlueScore )
	{
		pKVData->SetInt( "BlueScore", iBlueScore );
	}

	int iRedFlags = GetGlobalTFTeam( TF_TEAM_RED )->GetFlagCaptures();
	if ( iRedFlags )
	{
		pKVData->SetInt( "RedFlagCaps", iRedFlags );
	}

	int iBlueFlags = GetGlobalTFTeam( TF_TEAM_BLUE )->GetFlagCaptures();
	if ( iBlueFlags )
	{
		pKVData->SetInt( "BlueFlagCaps", iBlueFlags );
	}

	for ( int iTeam = FIRST_GAME_TEAM; iTeam < TF_TEAM_COUNT; iTeam++ )
	{
		int iPlayerCount = GetGlobalTeam( iTeam )->GetNumPlayers();
		if ( iPlayerCount == 0 )
			continue;
		switch ( iTeam )
		{
		case TF_TEAM_BLUE:
			pKVData->SetInt( "BluePlayerCount", iPlayerCount );
			break;
		case TF_TEAM_RED:
			pKVData->SetInt( "RedPlayerCount", iPlayerCount );
			break;
		}
	}

	bool bStalemate = iWinningTeam == TEAM_UNASSIGNED;
	if ( bStalemate )
	{
		pKVData->SetInt( "Stalemate", bStalemate );
	}

	if ( m_currentRoundRed.m_Summary.iTeamQuit > 0 )
	{
		pKVData->SetInt( "RedTeamQuit", m_currentRoundRed.m_Summary.iTeamQuit );
	}

	if ( m_currentRoundBlue.m_Summary.iTeamQuit > 0 )
	{
		pKVData->SetInt( "BlueTeamQuit", m_currentRoundBlue.m_Summary.iTeamQuit );
	}

	if ( m_currentRoundRed.m_Summary.iKills > 0 )
	{
		pKVData->SetInt( "RedKills", m_currentRoundRed.m_Summary.iKills );
	}

	if ( m_currentRoundBlue.m_Summary.iKills > 0 )
	{
		pKVData->SetInt( "BlueKills", m_currentRoundBlue.m_Summary.iKills );
	}

	if ( m_currentRoundRed.m_Summary.iDeaths > 0 )
	{
		pKVData->SetInt( "RedDeaths", m_currentRoundRed.m_Summary.iDeaths );
	}

	if ( m_currentRoundBlue.m_Summary.iDeaths > 0 )
	{
		pKVData->SetInt( "BlueDeaths", m_currentRoundBlue.m_Summary.iDeaths );
	}

	if ( m_currentRoundRed.m_Summary.iSuicides > 0 )
	{
		pKVData->SetInt( "RedSuicides", m_currentRoundRed.m_Summary.iSuicides );
	}

	if ( m_currentRoundBlue.m_Summary.iSuicides > 0 )
	{
		pKVData->SetInt( "BlueSuicides", m_currentRoundBlue.m_Summary.iSuicides );
	}

	if ( m_currentRoundRed.m_Summary.iAssists > 0 )
	{
		pKVData->SetInt( "RedAssists", m_currentRoundRed.m_Summary.iAssists );
	}

	if ( m_currentRoundBlue.m_Summary.iAssists > 0 )
	{
		pKVData->SetInt( "BlueAssists", m_currentRoundBlue.m_Summary.iAssists );
	}

	if ( m_currentRoundRed.m_Summary.iBuildingsBuilt > 0 )
	{
		pKVData->SetInt( "RedBuildingsBuilt", m_currentRoundRed.m_Summary.iBuildingsBuilt );
	}

	if ( m_currentRoundBlue.m_Summary.iBuildingsBuilt > 0 )
	{
		pKVData->SetInt( "BlueBuildingsBuilt", m_currentRoundBlue.m_Summary.iBuildingsBuilt );
	}

	if ( m_currentRoundRed.m_Summary.iBuildingsDestroyed > 0 )
	{
		pKVData->SetInt( "RedBuildingsDestroyed", m_currentRoundRed.m_Summary.iBuildingsDestroyed );
	}

	if ( m_currentRoundBlue.m_Summary.iBuildingsDestroyed > 0 )
	{
		pKVData->SetInt( "BlueBuildingsDestroyed", m_currentRoundBlue.m_Summary.iBuildingsDestroyed );
	}

	if ( m_currentRoundRed.m_Summary.iHeadshots > 0 )
	{
		pKVData->SetInt( "RedHeadshots", m_currentRoundRed.m_Summary.iHeadshots );
	}

	if ( m_currentRoundBlue.m_Summary.iHeadshots > 0 )
	{
		pKVData->SetInt( "BlueHeadshots", m_currentRoundBlue.m_Summary.iHeadshots );
	}

	if ( m_currentRoundRed.m_Summary.iDominations > 0 )
	{
		pKVData->SetInt( "RedDominations", m_currentRoundRed.m_Summary.iDominations );
	}

	if ( m_currentRoundBlue.m_Summary.iDominations > 0 )
	{
		pKVData->SetInt( "BlueDominations", m_currentRoundBlue.m_Summary.iDominations );
	}

	if ( m_currentRoundRed.m_Summary.iRevenges > 0 )
	{
		pKVData->SetInt( "RedRevenges", m_currentRoundRed.m_Summary.iRevenges );
	}

	if ( m_currentRoundBlue.m_Summary.iRevenges > 0 )
	{
		pKVData->SetInt( "BlueRevenges", m_currentRoundBlue.m_Summary.iRevenges );
	}

	if ( m_currentRoundRed.m_Summary.iInvulns > 0 )
	{
		pKVData->SetInt( "RedInvulns", m_currentRoundRed.m_Summary.iInvulns );
	}

	if ( m_currentRoundBlue.m_Summary.iInvulns > 0 )
	{
		pKVData->SetInt( "BlueInvulns", m_currentRoundBlue.m_Summary.iInvulns );
	}

	if ( m_currentRoundRed.m_Summary.iTeleports > 0 )
	{
		pKVData->SetInt( "RedTeleports", m_currentRoundRed.m_Summary.iTeleports );
	}

	if ( m_currentRoundBlue.m_Summary.iTeleports > 0 )
	{
		pKVData->SetInt( "BlueTeleports", m_currentRoundBlue.m_Summary.iTeleports );
	}

	if ( m_currentRoundRed.m_Summary.iDamageDone > 0 )
	{
		pKVData->SetInt( "RedDamageDone", m_currentRoundRed.m_Summary.iDamageDone );
	}

	if ( m_currentRoundBlue.m_Summary.iDamageDone > 0 )
	{
		pKVData->SetInt( "BlueDamageDone", m_currentRoundBlue.m_Summary.iDamageDone );
	}

	if ( m_currentRoundRed.m_Summary.iHealingDone > 0 )
	{
		pKVData->SetInt( "RedHealingDone", m_currentRoundRed.m_Summary.iHealingDone );
	}

	if ( m_currentRoundBlue.m_Summary.iHealingDone > 0 )
	{
		pKVData->SetInt( "BlueHealingDone", m_currentRoundBlue.m_Summary.iHealingDone );
	}

	if ( m_currentRoundRed.m_Summary.iCrits > 0 )
	{
		pKVData->SetInt( "RedCrits", m_currentRoundRed.m_Summary.iCrits );
	}

	if ( m_currentRoundBlue.m_Summary.iCrits > 0 )
	{
		pKVData->SetInt( "BlueCrits", m_currentRoundBlue.m_Summary.iCrits );
	}

	if ( m_currentRoundRed.m_Summary.iBackstabs > 0 )
	{
		pKVData->SetInt( "RedBackstabs", m_currentRoundRed.m_Summary.iBackstabs );
	}

	if ( m_currentRoundBlue.m_Summary.iBackstabs > 0 )
	{
		pKVData->SetInt( "BlueBackstabs", m_currentRoundBlue.m_Summary.iBackstabs );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Player Data
// Player reports are sent every round and when a player disconnects.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_GameStats_WritePlayer( CTFPlayer *pPlayer, bool bDisconnected )
{
	// Everytime we write out a player we also want to write out their loadout stats
	Event_PlayerLoadoutChanged( pPlayer, true );
#if !defined(NO_STEAM)
	if ( !pPlayer )
		return;

	if ( pPlayer->IsBot() )
		return;

	CSteamID steamIDForPlayer;
	if ( !pPlayer->GetSteamID( &steamIDForPlayer ) )
		return;

	PlayerStats_t &stats = CTF_GameStats.m_aPlayerStats[pPlayer->entindex()];

	// Player info.
	KeyValues* pKVData = new KeyValues( "TF2ServerPlayers" );
	pKVData->SetInt( "MapIndex", 0 );

	int iRoundIndex = m_iRoundsPlayed;
	if ( bDisconnected && m_bRoundActive )
		iRoundIndex++; // Increment so we report the current round if the player disconnected before the round was over.
	pKVData->SetInt( "RoundIndex", iRoundIndex );

	pKVData->SetInt( "PlayerUpdates", ++m_iPlayerUpdates );

	pKVData->SetUint64( "AccountIDPlayer", steamIDForPlayer.ConvertToUint64() );

	pKVData->SetInt( "ConnectTime", stats.iConnectTime );
	pKVData->SetInt( "DisconnectTime", MAX( stats.iDisconnectTime, 0 ) );

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_KILLS] > 0 )
	{
		pKVData->SetInt( "Kills", stats.statsCurrentRound.m_iStat[TFSTAT_KILLS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_DEATHS] > 0 )
	{
		pKVData->SetInt( "Deaths", stats.statsCurrentRound.m_iStat[TFSTAT_DEATHS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_SUICIDES] > 0 )
	{
		pKVData->SetInt( "Suicides", stats.statsCurrentRound.m_iStat[TFSTAT_SUICIDES] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_KILLASSISTS] > 0 )
	{
		pKVData->SetInt( "Assists", stats.statsCurrentRound.m_iStat[TFSTAT_KILLASSISTS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_BUILDINGSBUILT] > 0 )
	{
		pKVData->SetInt( "BuildingsBuilt", stats.statsCurrentRound.m_iStat[TFSTAT_BUILDINGSBUILT] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_BUILDINGSDESTROYED] > 0 )
	{
		pKVData->SetInt( "BuildingsDestroyed", stats.statsCurrentRound.m_iStat[TFSTAT_BUILDINGSDESTROYED] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_HEADSHOTS] > 0 )
	{
		pKVData->SetInt( "Headshots", stats.statsCurrentRound.m_iStat[TFSTAT_HEADSHOTS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_DOMINATIONS] > 0 )
	{
		pKVData->SetInt( "Dominations", stats.statsCurrentRound.m_iStat[TFSTAT_DOMINATIONS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_REVENGE] > 0 )
	{
		pKVData->SetInt( "Revenges", stats.statsCurrentRound.m_iStat[TFSTAT_REVENGE] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_DAMAGE] > 0 )
	{
		pKVData->SetInt( "DamageDone", stats.statsCurrentRound.m_iStat[TFSTAT_DAMAGE] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_DAMAGETAKEN] > 0 )
	{
		pKVData->SetInt( "DamageTaken", stats.statsCurrentRound.m_iStat[TFSTAT_DAMAGETAKEN] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_HEALING] > 0 )
	{
		pKVData->SetInt( "HealingDone", stats.statsCurrentRound.m_iStat[TFSTAT_HEALING] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_HEALTHKITS] > 0 )
	{
		pKVData->SetInt( "HealthKits", stats.statsCurrentRound.m_iStat[TFSTAT_HEALTHKITS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_AMMOKITS] > 0 )
	{
		pKVData->SetInt( "AmmoKits", stats.statsCurrentRound.m_iStat[TFSTAT_AMMOKITS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_CLASSCHANGES] > 0 )
	{
		pKVData->SetInt( "ClassChanges", stats.statsCurrentRound.m_iStat[TFSTAT_CLASSCHANGES] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_INVULNS] > 0 )
	{
		pKVData->SetInt( "Invulns", stats.statsCurrentRound.m_iStat[TFSTAT_INVULNS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_CRITS] > 0 )
	{
		pKVData->SetInt( "Crits", stats.statsCurrentRound.m_iStat[TFSTAT_CRITS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_BACKSTABS] > 0 )
	{
		pKVData->SetInt( "Backstabs", stats.statsCurrentRound.m_iStat[TFSTAT_BACKSTABS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_TELEPORTS] > 0 )
	{
		pKVData->SetInt( "Teleports", stats.statsCurrentRound.m_iStat[TFSTAT_TELEPORTS] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_SHOTS_HIT] > 0 )
	{
		pKVData->SetInt( "ShotsHit", stats.statsCurrentRound.m_iStat[TFSTAT_SHOTS_HIT] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_SHOTS_FIRED] > 0 )
	{
		pKVData->SetInt( "ShotsFired", stats.statsCurrentRound.m_iStat[TFSTAT_SHOTS_FIRED] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] > 0 )
	{
		pKVData->SetInt( "PointsScored", stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_CAPTURES] > 0 )
	{
		pKVData->SetInt( "Captures", stats.statsCurrentRound.m_iStat[TFSTAT_CAPTURES] );
	}

	if ( stats.statsCurrentRound.m_iStat[TFSTAT_DEFENSES] > 0 )
	{
		pKVData->SetInt( "Defenses", stats.statsCurrentRound.m_iStat[TFSTAT_DEFENSES] );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Steamworks Database Kill Data
//-----------------------------------------------------------------------------
void CTFGameStats::SW_GameStats_WriteKill( CTFPlayer* pKiller, CTFPlayer* pVictim, CTFPlayer* pAssister, IGameEvent* event /*player_death*/, const CTakeDamageInfo &info )
{
	if ( !pKiller || !pVictim )
		return; // Recorded kills must have a killer and a victim.

	if ( pKiller->IsBot() && pVictim->IsBot() )
	{
		if ( !pAssister || pAssister->IsBot() )
			return; // Don't record kills that only involve bots.
	}

	// 08/26/2010 - For now, don't record kills involving bots at all.
	// This is to work around an issue with inserting duplicate keys.
	if ( pKiller->IsBot() || pVictim->IsBot() )
		return;
	if ( pAssister && pAssister->IsBot() )
		return;

	if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
		return; // Only record kills during an active round.

	if ( !m_bRoundActive )
		return;

	// Kills info.
	KeyValues* pKVData = new KeyValues( "TF2ServerKills" );
	pKVData->SetInt( "MapIndex", 0 );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed+1 );

	CSteamID steamIDForPlayer;
	pVictim->GetSteamID( &steamIDForPlayer );
	if ( !pVictim->IsBot() )
	{
		pKVData->SetUint64( "AccountIDVictim", steamIDForPlayer.ConvertToUint64() );
	}
	else
	{
		pKVData->SetUint64( "AccountIDVictim", 1 );
	}

	pKiller->GetSteamID( &steamIDForPlayer );
	if ( !pKiller->IsBot() )
	{
		pKVData->SetUint64( "AccountIDKiller", steamIDForPlayer.ConvertToUint64() );
	}
	else
	{
		pKVData->SetUint64( "AccountIDKiller", 1 );
	}

	if ( pAssister && !pAssister->IsBot() )
	{
		pAssister->GetSteamID( &steamIDForPlayer );
		pKVData->SetUint64( "AccountIDAssister", steamIDForPlayer.ConvertToUint64() );
	}
	else
	{
		if ( !pAssister )
		{
			pKVData->SetUint64( "AccountIDAssister", 0 );
		}
		else if ( pAssister->IsBot() )
		{
			pKVData->SetUint64( "AccountIDAssister", 1 );
		}
	}

	pKVData->SetInt( "KillCount", ++m_iKillCount );

	pKVData->SetInt( "KillTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	// Participant Details
	const Vector& victimPos = pVictim->GetAbsOrigin();
	pKVData->SetString( "VictimClass", ClampedArrayElement( g_aPlayerClassNames_NonLocalized, pVictim->GetPlayerClass()->GetClassIndex() ) );
	pKVData->SetFloat( "VictimLocationX", victimPos.x );
	pKVData->SetFloat( "VictimLocationY", victimPos.y );
	pKVData->SetFloat( "VictimLocationZ", victimPos.z );

	const Vector& killerPos = pKiller->GetAbsOrigin();
	pKVData->SetString( "KillerClass", ClampedArrayElement( g_aPlayerClassNames_NonLocalized, pKiller->GetPlayerClass()->GetClassIndex() ) );
	pKVData->SetFloat( "KillerLocationX", killerPos.x );
	pKVData->SetFloat( "KillerLocationY", killerPos.y );
	pKVData->SetFloat( "KillerLocationZ", killerPos.z );

	if ( pAssister )
	{
		const Vector& assisterPos = pAssister->GetAbsOrigin();
		pKVData->SetString( "AssisterClass", ClampedArrayElement( g_aPlayerClassNames_NonLocalized, pAssister->GetPlayerClass()->GetClassIndex() ) );
		pKVData->SetFloat( "AssisterLocationX", assisterPos.x );
		pKVData->SetFloat( "AssisterLocationY", assisterPos.y );
		pKVData->SetFloat( "AssisterLocationZ", assisterPos.z );
	}

	int damageBits = event->GetInt( "damagebits" );

	const char* log_name = event->GetString( "weapon_logclassname" );
	pKVData->SetString( "WeaponLogNameID", log_name );

	pKVData->SetString( "WeaponID", GetWeaponIDName( event->GetInt( "weaponid" ) ) );

	ETFDmgCustom iCustomKill = (ETFDmgCustom)event->GetInt( "customkill" );
	if ( iCustomKill > 0 )
	{
		pKVData->SetString( "CustomDamageInfo", GetCustomDamageName( iCustomKill ) );
	}

	int16 damage = clamp( RoundFloatToInt( info.GetDamage() ), 32767, -32767 );
	pKVData->SetInt( "Damage", damage );

	pKVData->SetFloat( "Distance", victimPos.DistTo( killerPos ) );

	Vector damagePos = info.GetDamagePosition();
	pKVData->SetFloat( "DamageSourceX", damagePos.x );
	pKVData->SetFloat( "DamageSourceY", damagePos.y );
	pKVData->SetFloat( "DamageSourceZ", damagePos.z );

	bool bTest = ( damageBits & DMG_CRITICAL ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsCrit", bTest );
	}

	// Break down the death flags to make this data easier to handle.
	int deathFlags = event->GetInt( "death_flags" );

	bTest = ( deathFlags & TF_DEATH_DOMINATION ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsDomination", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_REVENGE ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsRevenge", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_ASSISTER_DOMINATION ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsAssisterDomination", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_ASSISTER_REVENGE ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsAssisterRevenge", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_FIRST_BLOOD ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsFirstBlood", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_FEIGN_DEATH ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsFeignDeath", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_INTERRUPTED ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsInterrupted", bTest );
	}

	bTest = ( deathFlags & TF_DEATH_GIBBED ) > 0;
	if ( bTest )
	{
		pKVData->SetInt( "IsGibbed", bTest );
	}

	bTest = ( pKiller == pVictim );
	if ( bTest )
	{
		pKVData->SetInt( "IsSuicide", bTest );
	}

	// Break down the interesting condition flags.
	int stunFlags = event->GetInt( "stun_flags" );

	bTest = ( pVictim->m_Shared.InCond( TF_COND_STUNNED ) &&
		( (stunFlags & TF_STUN_LOSER_STATE) || (stunFlags & TF_STUN_CONTROLS) ) );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimBallStunned", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_URINE );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimJarated", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_BLEEDING );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimBleeding", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_BURNING );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimBurning", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_DISGUISED );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimDisguised", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_STEALTHED );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimStealthed", bTest );
	}

	bTest = pVictim->m_Shared.InCond( TF_COND_ZOOMED );
	if ( bTest )
	{
		pKVData->SetInt( "IsVictimZoomed", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_CRITBOOSTED );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerCritBoosted", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_CRITBOOSTED_RAGE_BUFF );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerRageCritBoosted", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_OFFENSEBUFF );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerSoldierBuffed", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_DEFENSEBUFF );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerSoldierDefenseBuffed", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_REGENONDAMAGEBUFF );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerSoldierRegenBuffed", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_SHIELD_CHARGE );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerShieldCharging", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_DEMO_BUFF );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerEyelanderBuffed", bTest );
	}

	bTest = pKiller->m_Shared.InCond( TF_COND_ZOOMED );
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerZoomed", bTest );
	}

	bTest = pKiller->m_Shared.IsInvulnerable();
	if ( bTest )
	{
		pKVData->SetInt( "IsKillerInvulnerable", bTest );
	}

	int16 victim_health = clamp( pVictim->GetHealthBefore(), 32767, -32767 );
	pKVData->SetInt( "VictimHealth", victim_health );

	int16 killer_health = clamp( pKiller->GetHealth(), 32767, -32767 );
	pKVData->SetInt( "KillerHealth", killer_health );

	if ( pAssister )
	{
		int16 assister_health = clamp( pAssister->GetHealth(), 32767, -32767 );
		pKVData->SetInt( "AssisterHealth", assister_health );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------
// Purpose: Records player team activity during a round.
//-----------------------------------------------------------------------------
void CTFGameStats::Event_TeamChange( CTFPlayer* pPlayer, int oldTeam, int newTeam )
{
	if ( pPlayer->IsBot() )
		return;

	CSteamID steamIDForPlayer;
	if ( !pPlayer->GetSteamID( &steamIDForPlayer ) )
		return;

	if ( oldTeam == newTeam )
		return;

//	if ( oldTeam == 0 || newTeam == 0 )
//		return;

#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ServerTeamChanges" );
	pKVData->SetInt( "MapIndex", 0 );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed+1 );
//	pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	pKVData->SetString( "OldTeam", ClampedArrayElement( g_aTeamNames, oldTeam ) );
	pKVData->SetString( "NewTeam", ClampedArrayElement( g_aTeamNames, newTeam ) );

	pKVData->SetInt( "ChangeCount", pPlayer->GetTeamChangeCount() );
	pKVData->SetUint64( "AccountIDPlayer", steamIDForPlayer.ConvertToUint64() );
	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Records players touching currency packs - primarily MvM, but future modes will likely use
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerCollectedCurrency( CBasePlayer *pPlayer, int nAmount )
{
	Assert( pPlayer );

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		IncrementStat( pTFPlayer, TFSTAT_CURRENCY_COLLECTED, nAmount );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Records the item set a player is using and for how long (until class, map, server or loadout  change)
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerLoadoutChanged( CTFPlayer *pPlayer, bool bForceReport )
{
	// Steam needs to be updated to take in the new table. So we disable the table first
#if !defined(NO_STEAM)
	if ( !pPlayer )
		return;

	if ( pPlayer->IsBot() )
		return;

	CSteamID steamIDForPlayer;
	if ( !pPlayer->GetSteamID( &steamIDForPlayer ) )
		return;

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// Not enough time reported, reset
	int iSecondsUsed = (int)( gpGlobals->curtime - stats.loadoutStats.flStartTime );
	bool bActuallyChanged = false;
	bool bIsInit = false;

	int iPrevClass = stats.loadoutStats.iClass;
	bActuallyChanged |= pPlayer->GetPlayerClass()->GetClassIndex() != iPrevClass;

	// if this is the first time through, class is invalid and we dont want to report anything

	// Table updated, using v2
	KeyValues* pKVData = new KeyValues( "TF2ServerPlayerLoadoutv2" );
	
	int iSlotCount = LOADOUT_POSITION_MISC2 + 1;
	for ( int iSlot = 0; iSlot < iSlotCount; ++iSlot )
	{
		int iDefIndex = stats.loadoutStats.iLoadoutItemDefIndices[ iSlot ];
		bIsInit |= iDefIndex != INVALID_ITEM_DEF_INDEX;

		pKVData->SetInt( CFmtStr("SlotDef%d", iSlot), iDefIndex );
		pKVData->SetInt( CFmtStr("SlotQuality%d", iSlot), stats.loadoutStats.iLoadoutItemQualities[ iSlot ] );
		pKVData->SetInt( CFmtStr("SlotStyle%d", iSlot), stats.loadoutStats.iLoadoutItemStyles[ iSlot ] );

		// Check to see if the item actually changed
		item_definition_index_t iItemDef = INVALID_ITEM_DEF_INDEX;
		CEconItemView *pItem = pPlayer->GetLoadoutItem( pPlayer->GetPlayerClass()->GetClassIndex(), iSlot );
		if ( pItem )
		{
			iItemDef = pItem->GetItemDefIndex();
		}

		// Check if there was actually a change
		bActuallyChanged |= stats.loadoutStats.iLoadoutItemDefIndices[ iSlot ] != iItemDef;

		// Set the new items
		int iItemQuality = pItem ? pItem->GetItemQuality() : AE_UNDEFINED;
		style_index_t iItemStyle = pItem ? pItem->GetStyle() : 0;
		stats.loadoutStats.SetItemDef( iSlot, iItemDef, iItemQuality, iItemStyle );
	}

	pKVData->SetInt( "ID", ++m_iLoadoutChangesCount );
	pKVData->SetInt( "SecondsEquipped", iSecondsUsed );
	pKVData->SetInt( "Class", iPrevClass );
	pKVData->SetInt( "AccountIDPlayer", (int)steamIDForPlayer.ConvertToUint64() );	// OGS does not actually support uints

	if ( iPrevClass < TF_FIRST_NORMAL_CLASS || iPrevClass >= TF_LAST_NORMAL_CLASS )
	{
		bIsInit = false;
	}

	//	pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	// Ignore forced respawn calls that trigger this but don't actually change your loadout
	// do not report if time used is less then 5 minutes
	if ( ( bActuallyChanged || bForceReport ) )
	{
		if ( stats.loadoutStats.flStartTime > 0 && iSecondsUsed > 300 && bIsInit )
		{
			GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
		}
	
		stats.loadoutStats.Set( pPlayer->GetPlayerClass()->GetClassIndex() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerRevived( CTFPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_REVIVED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerThrowableHit( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_THROWABLEHIT, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerThrowableKill( CTFPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_THROWABLEKILL, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Track only their highest - not cumulative
//-----------------------------------------------------------------------------
void CTFGameStats::Event_PlayerEarnedKillStreak( CTFPlayer *pAttacker )
{
	if ( !pAttacker )
		return;

	PlayerStats_t &stats = m_aPlayerStats[pAttacker->entindex()];
	int nCount = pAttacker->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Kills );
	int nMax = stats.statsCurrentRound.m_iStat[TFSTAT_KILLSTREAK_MAX];
	if ( nCount > nMax )
	{
		stats.statsCurrentRound.m_iStat[TFSTAT_KILLSTREAK_MAX] = nCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Halloween!
//-----------------------------------------------------------------------------
void CTFGameStats::Event_HalloweenBossEvent( uint8 unBossType, uint16 unBossLevel, uint8 unEventType, uint8 unPlayersInvolved, float fElapsedTime )
{
	//if ( !GCClientSystem() )
	//	return;

	//static uint8 unEventCounter = 0;

	//GCSDK::CProtoBufMsg<CMsgHalloween_ServerBossEvent> msg( k_EMsgGC_Halloween_ServerBossEvent );

	//msg.Body().set_event_counter( unEventCounter++ );
	//msg.Body().set_timestamp( CRTime::RTime32TimeCur() );
	//msg.Body().set_boss_type( unBossType );
	//msg.Body().set_boss_level( unBossLevel );
	//msg.Body().set_event_type( unEventType );
	//msg.Body().set_players_involved( unPlayersInvolved );
	//msg.Body().set_elapsed_time( fElapsedTime );

	//GCClientSystem()->BSendMessage( msg );
}


//-----------------------------------------------------------------------------
// Purpose: Records player class activity during a round.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_ClassChange( CTFPlayer* pPlayer, int oldClass, int newClass )
{
	if ( pPlayer->IsBot() )
		return;

	CSteamID steamIDForPlayer;
	if ( !pPlayer->GetSteamID( &steamIDForPlayer ) )
		return;

	if ( oldClass == newClass )
		return;

//	if ( oldClass == 0 || newClass == 0 )
//		return;

#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ServerClassChanges" );
//	pKVData->SetInt( "MapIndex", CBGSDriver.m_iNumLevels+1 );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed+1 );
//	pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	oldClass = clamp( oldClass, 0, TF_CLASS_COUNT-1 );
	pKVData->SetString( "OldClass", g_aPlayerClassNames_NonLocalized[oldClass] );

	newClass = clamp( newClass, 0, TF_CLASS_COUNT-1 );
	pKVData->SetString( "NewClass", g_aPlayerClassNames_NonLocalized[newClass] );

	pKVData->SetInt( "ChangeCount", pPlayer->GetClassChangeCount() );
	pKVData->SetUint64( "AccountIDPlayer", steamIDForPlayer.ConvertToUint64() );
	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Records player scoring activity during a round.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_GameEvent( CTFPlayer* pPlayer, const char* pszEventID, int iPoints )
{
	if ( pPlayer && pPlayer->IsBot() )
		return;

#if !defined(NO_STEAM)
	KeyValues* pKVData = new KeyValues( "TF2ServerGameEvents" );
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed+1 );
//	pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	pKVData->SetInt( "ChangeCount", ++m_iEvents );
	pKVData->SetString( "EventID", pszEventID );
	if ( iPoints )
	{
		pKVData->SetInt( "Points", iPoints );
	}
	if ( pPlayer )
	{
		pKVData->SetString( "Team", ClampedArrayElement( g_aTeamNames, pPlayer->GetTeamNumber() ) );

		pKVData->SetFloat( "LocationX", pPlayer->GetAbsOrigin().x );
		pKVData->SetFloat( "LocationY", pPlayer->GetAbsOrigin().y );
		pKVData->SetFloat( "LocationZ", pPlayer->GetAbsOrigin().z );

		CSteamID steamIDForPlayer;
		if ( pPlayer->GetSteamID( &steamIDForPlayer ) )
		{
			pKVData->SetUint64( "AccountIDPlayer", steamIDForPlayer.ConvertToUint64() );
		}
		else
		{
			pKVData->SetUint64( "AccountIDPlayer", 0 );
		}
	}
	else
	{
		pKVData->SetString( "Team", g_aTeamNames[0] );
		pKVData->SetUint64( "AccountIDPlayer", 0 );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Records flag activity during a match.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_FlagEvent( int iPlayer, int iCarrier, int iEventType )
{
	CTFPlayer* pPlayer = ToTFPlayer( UTIL_PlayerByIndex(iPlayer) );

	int iPoints = 0;
	const char* pszEventID = NULL;
	switch ( iEventType )
	{
	case TF_FLAGEVENT_PICKUP:
		pszEventID = "flag_pickup";
		break;

	case TF_FLAGEVENT_CAPTURE:
		pszEventID = "flag_captured";
		iPoints = 1;
		break;

	case TF_FLAGEVENT_DEFEND:
		pszEventID = "flag_defended";
		iPoints = 1;
		break;

	case TF_FLAGEVENT_DROPPED:
		pszEventID = "flag_dropped";
		break;

	case TF_FLAGEVENT_RETURNED:
		pszEventID = "flag_returned";
		break;
	}

	SW_GameEvent( pPlayer, pszEventID, iPoints );
}

//-----------------------------------------------------------------------------
// Purpose: Records control point activity during a match.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_CapEvent( int iPoint, int iPlayer, const char* pszEventID, int iPoints )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex(iPlayer) );
	if ( !pPlayer )
		return;

	SW_GameEvent( pPlayer, pszEventID, iPoints );
}

//-----------------------------------------------------------------------------
// Purpose: Uploads the hosts row, which has general information about the game server.
//-----------------------------------------------------------------------------
void CTFGameStats::SW_WriteHostsRow()
{
#if !defined(NO_STEAM)

	// Gather info we'll be sending...
	int maxClients			= gpGlobals->maxClients;
	bool isDedicated		= engine->IsDedicatedServer();
	bool isVACSecure		= SteamGameServer_BSecure();
	bool cheatsWereOn		= TFGameRules() && TFGameRules()->HaveCheatsBeenEnabledDuringLevel();
	bool isPassword			= GetSteamWorksSGameStatsUploader().IsPassworded();

	KeyValues* pKVData = new KeyValues( "TF2ServerHosts" );

	// Server Browse Info
	pKVData->SetInt( "ServerIP", GetSteamWorksSGameStatsUploader().GetServerIP() );
	pKVData->SetString( "ServerName", GetSteamWorksSGameStatsUploader().GetHostName() );

	// Server Config
	pKVData->SetInt( "PlayerSlots", maxClients );
	if ( isDedicated )
	{
		pKVData->SetInt( "IsDedicated", isDedicated );
	}
	if ( isPassword )
	{
		pKVData->SetInt( "IsPassworded", isPassword );
	}
	if ( isVACSecure )
	{
		pKVData->SetInt( "IsVACSecure", isVACSecure );
	}
	if ( cheatsWereOn )
	{
		pKVData->SetInt( "IsCheats", cheatsWereOn );
	}
	if ( mp_timelimit.GetInt() )
	{
		pKVData->SetInt( "TimeLimit", mp_timelimit.GetInt() );
	}
	ConVarRef tf_flag_caps_per_round( "tf_flag_caps_per_round" );
	if ( tf_flag_caps_per_round.GetInt() )
	{
		pKVData->SetInt( "FlagCapsPerRound", tf_flag_caps_per_round.GetInt() );
	}
	if ( mp_maxrounds.GetInt() )
	{
		pKVData->SetInt( "MaxRounds", mp_maxrounds.GetInt() );
	}
	if ( mp_winlimit.GetInt() )
	{
		pKVData->SetInt( "WinLimit", mp_winlimit.GetInt() );
	}
	ConVarRef mp_disable_respawn_times( "mp_disable_respawn_times" );
	if ( mp_disable_respawn_times.GetInt() )
	{
		pKVData->SetInt( "DisableRespawnTimes", mp_disable_respawn_times.GetInt() );
	}
	if ( mp_stalemate_meleeonly.GetInt() )
	{
		pKVData->SetInt( "StalemateMeleeOnly", mp_stalemate_meleeonly.GetInt() );
	}
	if ( mp_forceautoteam.GetInt() )
	{
		pKVData->SetInt( "ForceAutoTeam", mp_forceautoteam.GetInt() );
	}

	// Server Activity Info
	RTime32 starttime = GetSteamWorksSGameStatsUploader().GetStartTime();
	if ( starttime )
	{
		pKVData->SetInt( "ServerStartTime", starttime );
	}

	RTime32 endtime = GetSteamWorksSGameStatsUploader().GetEndTime();
	if ( endtime )
	{
		pKVData->SetInt( "ServerEndTime", endtime );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );

#endif
}

//-----------------------------------------------------------------------------
#undef min
#undef max
struct PasstimeHistogramStats
{
	double min, max, mean, median, mode, stdev;
	PasstimeHistogramStats() : min(0), max(0), mean(0), median(0), mode(0), stdev(0) {}
};

static int qsort_ascending_uint16( const void *a, const void *b )
{
	return *((uint16*)b) - *((uint16*)a);
}

template<int TMaxSamples>
static PasstimeHistogramStats Passtime_SampleStats( const uint16 (&samples)[TMaxSamples], uint16 iSampleCount )
{
	PasstimeHistogramStats result;
	if ( (iSampleCount <= 1) || (iSampleCount > TMaxSamples) )
	{
		return result;
	}

	// mode is useless, so don't bother

	// sort for median
	qsort( (void*) &samples[0], iSampleCount, sizeof(samples[0]), &qsort_ascending_uint16 );

	//
	// Sum, Min, Max
	//
	double sum = 0;
	result.min = DBL_MAX;
	result.max = DBL_MIN;
	for ( uint32 i = 0; i < iSampleCount; ++i )
	{
		float s = samples[i];
		sum += s;
		result.min = MIN( s, result.min );
		result.max = MAX( s, result.max );
	}

	//
	// Mean
	//
	result.mean = (double)sum / (double)iSampleCount;

	//
	// Median
	//
	result.median = samples[ iSampleCount / 2 ]; // close enough

	//
	// Stdev
	//
	for ( uint32 i = 0; i < iSampleCount; ++i )
	{
		double s = samples[i];
		result.stdev += (s - result.mean) * (s - result.mean);
	}
	result.stdev = sqrt( result.stdev / ((double)(iSampleCount - 1)) );

	return result;
}

template<int TBinCount>
static PasstimeHistogramStats Passtime_HistogramStats( const uint32 (&hist)[TBinCount], uint32 iHistSum, uint32 iSampleCount )
{
	PasstimeHistogramStats result;
	if ( iSampleCount <= 1 )
	{
		return result;
	}

	//
	// Mean
	//
	result.mean = (float)iHistSum / (float)iSampleCount;

	//
	// Min
	//
	for ( uint32 i = 0; i < 256; ++i )
	{
		if ( hist[i] != 0 )
		{
			result.min = i;
			break;
		}
	}

	//
	// Max
	//
	for ( int32 i = 255; i >= 0; --i )
	{
		if ( hist[i] != 0 )
		{
			result.max = i;
			break;
		}
	}

	//
	// Median
	//
	int iMedSample = iSampleCount / 2;
	int iMedian;
	for ( iMedian = 0; iMedian < 256; ++iMedian )
	{
		if ( hist[iMedian] != 0 )
			break;
	}
	while( (iMedSample > 0) && (iMedian < 256) )
	{
		iMedSample -= hist[iMedian];
		++iMedian;
	}
	result.median = iMedian - 1;

	//
	// Mode, stdev
	//
	uint32 iLargestCount = 0;
	result.mode = -1; // wat
	for ( uint32 i = 0; i < 256; ++i )
	{
		uint32 iSampleCount = hist[i];
		for ( uint32 j = 0; j < iSampleCount; ++j )
		{
			// this feels dumb
			result.stdev += (i - result.mean) * (i - result.mean);
		}

		if ( iSampleCount > iLargestCount )
		{
			iLargestCount = iSampleCount;
			result.mode = i;
		}
	}
	result.stdev = sqrt( result.stdev / ((double)iSampleCount - 1) );

	return result;
}

void CTFGameStats::SW_PasstimeRoundEnded()
{
#if !defined(NO_STEAM)
	if ( !TFGameRules() || !g_pPasstimeLogic )
	{
		return;
	}

	// Flush data gathered so far...
	GetSteamWorksSGameStatsUploader().FlushStats();

	KeyValues *pKVData = new KeyValues( "TF2ServerPasstimeRoundEndedv2" );
	pKVData->SetString( "MapID", m_currentMap.m_Header.m_szMapName );							// Reference table
	pKVData->SetInt( "RoundIndex", m_iRoundsPlayed );

	pKVData->SetInt( "TotalPassesStarted", m_passtimeStats.summary.nTotalPassesStarted );
	pKVData->SetInt( "TotalPassesFailed", m_passtimeStats.summary.nTotalPassesFailed );
	pKVData->SetInt( "TotalPassesShotDown", m_passtimeStats.summary.nTotalPassesShotDown );
	pKVData->SetInt( "TotalPassesCompleted", m_passtimeStats.summary.nTotalPassesCompleted );
	pKVData->SetInt( "TotalPassesCompletedNearGoal", m_passtimeStats.summary.nTotalPassesCompletedNearGoal );
	pKVData->SetInt( "TotalPassesIntercepted", m_passtimeStats.summary.nTotalPassesIntercepted );
	pKVData->SetInt( "TotalPassesInterceptedNearGoal", m_passtimeStats.summary.nTotalPassesInterceptedNearGoal );
	pKVData->SetInt( "TotalPassRequests", m_passtimeStats.summary.nTotalPassRequests );
	pKVData->SetInt( "TotalTosses", m_passtimeStats.summary.nTotalTosses );
	pKVData->SetInt( "TotalTossesCompleted", m_passtimeStats.summary.nTotalTossesCompleted );
	pKVData->SetInt( "TotalTossesIntercepted", m_passtimeStats.summary.nTotalTossesIntercepted );
	pKVData->SetInt( "TotalTossesInterceptedNearGoal", m_passtimeStats.summary.nTotalTossesInterceptedNearGoal );
	pKVData->SetInt( "TotalSteals", m_passtimeStats.summary.nTotalSteals );
	pKVData->SetInt( "TotalStealsNearGoal", m_passtimeStats.summary.nTotalStealsNearGoal );
	pKVData->SetInt( "TotalBallSpawnShots", m_passtimeStats.summary.nTotalBallSpawnShots );
	pKVData->SetInt( "TotalScores", m_passtimeStats.summary.nTotalScores );
	pKVData->SetInt( "TotalRecoveries", m_passtimeStats.summary.nTotalRecoveries );
	pKVData->SetInt( "TotalCarrySec", m_passtimeStats.summary.nTotalCarrySec );
	pKVData->SetInt( "TotalWinningTeamBallCarrySec", m_passtimeStats.summary.nTotalWinningTeamBallCarrySec );
	pKVData->SetInt( "TotalLosingTeamBallCarrySec", m_passtimeStats.summary.nTotalLosingTeamBallCarrySec );
	pKVData->SetInt( "TotalThrowCancels", m_passtimeStats.summary.nTotalThrowCancels );
	pKVData->SetInt( "TotalSpeedBoosts", m_passtimeStats.summary.nTotalSpeedBoosts );
	pKVData->SetInt( "TotalJumpPads", m_passtimeStats.summary.nTotalJumpPads );
	pKVData->SetInt( "TotalCarrierSpeedBoosts", m_passtimeStats.summary.nTotalCarrierSpeedBoosts );
	pKVData->SetInt( "TotalCarrierJumpPads", m_passtimeStats.summary.nTotalCarrierJumpPads );
	pKVData->SetInt( "BallNeutralSec", m_passtimeStats.summary.nBallNeutralSec );
	pKVData->SetInt( "GoalType", m_passtimeStats.summary.nGoalType );
	pKVData->SetInt( "RoundEndReason", m_passtimeStats.summary.nRoundEndReason );
	pKVData->SetInt( "RoundRemainingSec", m_passtimeStats.summary.nRoundRemainingSec );
	pKVData->SetInt( "RoundMaxSec", m_passtimeStats.summary.nRoundMaxSec );
	pKVData->SetInt( "RoundElapsedSec", m_passtimeStats.summary.nRoundMaxSec - m_passtimeStats.summary.nRoundRemainingSec );
	pKVData->SetInt( "PlayersBlueMax", m_passtimeStats.summary.nPlayersBlueMax );
	pKVData->SetInt( "PlayersRedMax", m_passtimeStats.summary.nPlayersRedMax );
	pKVData->SetInt( "ScoreRed", m_passtimeStats.summary.nScoreRed );
	pKVData->SetInt( "ScoreBlue", m_passtimeStats.summary.nScoreBlue );

	pKVData->SetBool( "Stalemate", m_passtimeStats.summary.bStalemate );
	pKVData->SetBool( "SuddenDeath", m_passtimeStats.summary.bSuddenDeath );
	pKVData->SetBool( "MeleeOnlySuddenDeath", m_passtimeStats.summary.bMeleeOnlySuddenDeath );

	auto ballFracStats = Passtime_HistogramStats( m_passtimeStats.summary.arrBallFracHist,
		m_passtimeStats.summary.nBallFracHistSum, m_passtimeStats.summary.nBallFracSampleCount );
	pKVData->SetInt( "BallFracHistMin", (int)round( ballFracStats.min ) );
	pKVData->SetInt( "BallFracHistMax", (int)round( ballFracStats.max ) );
	pKVData->SetInt( "BallFracHistMean", (int)round( ballFracStats.mean ) );
	pKVData->SetInt( "BallFracHistMedian", (int)round( ballFracStats.median ) );
	pKVData->SetInt( "BallFracHistMode", (int)round( ballFracStats.mode ) );
	pKVData->SetInt( "BallFracHistStdev", (int)round( ballFracStats.stdev ) );
	pKVData->SetInt( "BallFracHistSampleCount", (int)m_passtimeStats.summary.nBallFracSampleCount ); // for approx global average

	auto passTravelDistStats = Passtime_SampleStats(
		m_passtimeStats.summary.arrPassTravelDistSamples, m_passtimeStats.summary.nPassTravelDistSampleCount );
	pKVData->SetInt( "PassTravelDistMin", (int)round( passTravelDistStats.min ) );
	pKVData->SetInt( "PassTravelDistMax", (int)round( passTravelDistStats.max ) );
	pKVData->SetInt( "PassTravelDistMean", (int)round( passTravelDistStats.mean ) );
	pKVData->SetInt( "PassTravelDistMedian", (int)round( passTravelDistStats.median ) );
	//pKVData->SetInt( "PassTravelDistMode", (int) round( passTravelDistStats.mode ) ); meaningless
	pKVData->SetInt( "PassTravelDistStdev", (int)round( passTravelDistStats.stdev ) );
	pKVData->SetInt( "PassTravelDistSampleCount", (int)m_passtimeStats.summary.nPassTravelDistSampleCount ); // for approx global average

	// have to flatten class stats because stats system can't handle nested tables
	{
		char aClassKey[32] = { 0, };
		for ( int nClass = TF_FIRST_NORMAL_CLASS; nClass <= TF_LAST_NORMAL_CLASS; ++nClass )
		{
			V_sprintf_safe( aClassKey, "TotalScores_%s", g_aRawPlayerClassNamesShort[nClass] );
			pKVData->SetInt( aClassKey, m_passtimeStats.classes[nClass].nTotalScores );
			V_sprintf_safe( aClassKey, "TotalCarrySec_%s", g_aRawPlayerClassNamesShort[nClass] );
			pKVData->SetInt( aClassKey, m_passtimeStats.classes[nClass].nTotalCarrySec );
		}
	}


	if ( tf_passtime_save_stats.GetBool() )
	{
		// do this before AddStatsForUpload because it might actually just delete pKVData

		// i need to copy it because i need to add some keys and i don't want there to be any possibility
		// of tainting the kv that's sent to the stats server
		auto pKVCopy = pKVData->MakeCopy();
		auto iNow = CRTime::RTime32TimeCur();
		char filename[128];
		V_sprintf_safe(filename, "passtime_stats_%u.txt", iNow);
		
		// add keys to simulate what the stats server usually adds to the database automatically
		pKVData->SetInt( "SessionID", 0 );
		pKVData->SetInt( "TimeReported", iNow );

		pKVData->SaveToFile( g_pFullFileSystem, filename );
		pKVCopy->deleteThis();
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );

#endif
}

//-----------------------------------------------------------------------------
void CTFGameStats::Event_PowerUpModeDeath( CTFPlayer *pKiller, CTFPlayer *pVictim )
{
#if !defined(NO_STEAM)

	//START_TABLE( k_ESchemaCatalogOGS, TF2PowerUpModeKillsv2, TABLE_PROP_NORMAL )
	//	INT_FIELD( llSessionID, SessionID, uint64 )				// Reporting server
	//	INT_FIELD( nAccountID, AccountID, int32 )				// Player
	//	INT_FIELD( nID, ID, int32 )								// ID
	//	INT_FIELD( bIsTrustedServer, IsTrustedServer, bool )
	//	INT_FIELD( nKillerClass, KillerClass, int16 )
	//	INT_FIELD( nKillerRune, KillerRune, int16 )
	//	INT_FIELD( nKillerKillstreak, KillerKillstreak, int16 )
	
	//	INT_FIELD( nKillerPrimary, KillerPrimary, int32 )
	//	INT_FIELD( nKillerSecondary, KillerSecondary, int32 )
	//	INT_FIELD( nKillerMelee, KillerMelee, int32 )

	//	INT_FIELD( nVictimClass, VictimClass, int16 )
	//	INT_FIELD( nVictimRune, VictimRune, int16 )
	//	INT_FIELD( nVictimKillstreak, VictimKillstreak, int16 )
	//	INT_FIELD( RTime32UpdateTime, TimeSubmitted, RTime32 )
	//	PRIMARY_KEYS_CLUSTERED( 80, nAccountID, RTime32UpdateTime, llSessionID )
	//	WIPE_TABLE_BETWEEN_TESTS( k_EWipePolicyWipeForAllTests )
	//	PARTITION_INTERVAL( k_EPartitionIntervalDaily )
	//	OWNING_APPLICATION( 440 )
	//	END_TABLE

	if ( !TFGameRules() || !TFGameRules()->IsPowerupMode() )
		return;

	if ( !pKiller || !pVictim )
		return;

	if ( pKiller->IsBot() || pVictim->IsBot() )
		return;

	CSteamID killerID;
	pKiller->GetSteamID( &killerID );

	if ( !killerID.IsValid() || !killerID.BIndividualAccount() )
		return;


	KeyValues* pKVData = new KeyValues( "TF2PowerUpModeKillsv2" );

	pKVData->SetInt( "AccountID", (int)killerID.GetAccountID() );
	pKVData->SetInt( "ID", (int)m_iEvents++ );
	pKVData->SetInt( "KillerClass", pKiller->GetPlayerClass()->GetClassIndex() );
	pKVData->SetInt( "KillerRune", GetConditionFromRuneType( pKiller->m_Shared.GetCarryingRuneType() ) );
	pKVData->SetInt( "KillerKillstreak",  pKiller->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_KillsAll ) );
	
	CEconItemView *pItem = pKiller->GetLoadoutItem( pKiller->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_PRIMARY );
	item_definition_index_t iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "KillerPrimary", iItemDef );

	pItem = pKiller->GetLoadoutItem( pKiller->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_SECONDARY );
	iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "KillerSecondary", iItemDef );
	
	pItem = pKiller->GetLoadoutItem( pKiller->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_MELEE );
	iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "KillerMelee", iItemDef );
	
	pKVData->SetInt( "VictimClass", pVictim->GetPlayerClass()->GetClassIndex() );
	pKVData->SetInt( "VictimRune", GetConditionFromRuneType( pVictim->m_Shared.GetCarryingRuneType() ) );
	pKVData->SetInt( "VictimKillstreak",  pVictim->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_KillsAll ) );
	
	//pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );

#endif // !NO_STEAM
}

//-----------------------------------------------------------------------------
void CTFGameStats::Event_PowerUpRuneDuration( CTFPlayer *pPlayer, int iDuration, int nRune )
{
#if !defined(NO_STEAM)

//-----------------------------------------------------------------------------
// OGS: TF2 PowerUp Mode - Power Up duration
//-----------------------------------------------------------------------------

	//START_TABLE( k_ESchemaCatalogOGS, TF2PowerUpModeRuneDuration, TABLE_PROP_NORMAL )
	//	INT_FIELD( llSessionID, SessionID, uint64 )				// Reporting server
	//	INT_FIELD( nAccountID, AccountID, uint32 )				// Player
	//	INT_FIELD( nID, ID, int32 )								// ID
	//	INT_FIELD( bIsTrustedServer, IsTrustedServer, bool )
	//	INT_FIELD( nPlayerClass, PlayerClass, int16 )
	//	INT_FIELD( nPlayerRune, PlayerRune, int16 )
	//	INT_FIELD( nPlayerKillstreak, PlayerKillstreak, int16 )
	//	INT_FIELD( nRuneDuration, RuneDuration, int32 )
	//	INT_FIELD( nPlayerPrimary, PlayerPrimary, int32 )
	//	INT_FIELD( nPlayerSecondary, PlayerSecondary, int32 )
	//	INT_FIELD( nPlayerMelee, PlayerMelee, int32 )
	//	INT_FIELD( RTime32UpdateTime, TimeSubmitted, RTime32 )
	//	PRIMARY_KEYS_CLUSTERED( 80, nAccountID, nID, RTime32UpdateTime, llSessionID )
	//	WIPE_TABLE_BETWEEN_TESTS( k_EWipePolicyWipeForAllTests )
	//	PARTITION_INTERVAL( k_EPartitionIntervalDaily )
	//	OWNING_APPLICATION( 440 )
	//	END_TABLE

	if ( !TFGameRules() || !TFGameRules()->IsPowerupMode() )
		return;

	if ( !pPlayer || pPlayer->IsBot() )
		return;

	CSteamID playerID;
	pPlayer->GetSteamID( &playerID );

	if ( !playerID.IsValid() || !playerID.BIndividualAccount() )
		return;


	KeyValues* pKVData = new KeyValues( "TF2PowerUpModeRuneDuration" );

	pKVData->SetInt( "AccountID", (int)playerID.GetAccountID() );
	pKVData->SetInt( "ID", (int)m_iEvents++ );
	pKVData->SetInt( "PlayerClass", pPlayer->GetPlayerClass()->GetClassIndex() );
	pKVData->SetInt( "PlayerRune", nRune );
	pKVData->SetInt( "PlayerKillstreak", pPlayer->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_KillsAll ) );
	pKVData->SetInt( "RuneDuration", iDuration );

	CEconItemView *pItem = pPlayer->GetLoadoutItem( pPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_PRIMARY );
	item_definition_index_t iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "PlayerPrimary", iItemDef );

	pItem = pPlayer->GetLoadoutItem( pPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_SECONDARY );
	iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "PlayerSecondary", iItemDef );

	pItem = pPlayer->GetLoadoutItem( pPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_MELEE );
	iItemDef = pItem ? pItem->GetItemDefIndex() : 0;
	pKVData->SetInt( "PlayerMelee", iItemDef );

	//pKVData->SetInt( "TimeSubmitted", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif // !NO_STEAM
}
