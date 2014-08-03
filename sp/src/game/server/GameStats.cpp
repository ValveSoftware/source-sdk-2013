//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"


#include "igamesystem.h"
#include "gamestats.h"
#include "tier1/utlstring.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"

#ifndef SWDS
#include "iregistry.h"
#endif

#include "tier1/utldict.h"
#include "tier0/icommandline.h"
#include "vehicle_base.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

#define GAMESTATS_LOG_FILE "gamestats.log"
#define GAMESTATS_PATHID "MOD"

/*
#define ONE_DAY_IN_SECONDS 86400

// Lower threshold in debug for testing...
#if defined( _DEBUG )
#define WALKED_AWAY_FROM_KEYBOARD_SECONDS 15.0f   // 15 seconds of movement == might be paused
#else
#define WALKED_AWAY_FROM_KEYBOARD_SECONDS 300.0f   // 5 minutes of no movement == might be paused
#endif
*/


extern IUploadGameStats *gamestatsuploader;
extern ConVar skill;

static CBaseGameStats s_GameStats_Singleton;
CBaseGameStats *gamestats = &s_GameStats_Singleton; //start out pointing at the basic version which does nothing by default

bool UserCmdChanged( const CUserCmd& lhs, const CUserCmd& rhs );
bool StatsTrackingIsFullyEnabled( void );


//used to drive most of the game stat event handlers as well as track basic stats under the hood of CBaseGameStats
class CBaseGameStats_Driver : public CAutoGameSystemPerFrame
{
public:
	CBaseGameStats_Driver( void );
	
	typedef CAutoGameSystemPerFrame BaseClass;

	// IGameSystem overloads
	virtual bool Init();
	virtual void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPreEntity();
	// Called during game save
	virtual void OnSave();
	// Called during game restore, after the local player has connected and entities have been fully restored
	virtual void OnRestore();

	virtual void FrameUpdatePostEntityThink();

	void PossibleMapChange( void );

	CUtlString		m_PrevMapName; //used to track "OnMapChange" events
	int				m_iLoadedVersion;
	char			m_szLoadedUserID[ 17 ];	// GUID

	bool			m_bEnabled; //false if incapable of uploading or the user doesn't want to enable stat tracking
	bool			m_bShuttingDown;
	bool			m_bInLevel;
	bool			m_bFirstLevel;
	time_t			m_tLastUpload;

	float			m_flLevelStartTime;


	bool			m_bStationary;
	float			m_flLastMovementTime;
	CUserCmd		m_LastUserCmd;
	bool			m_bGamePaused;
	float			m_flPauseStartTime;
};
static CBaseGameStats_Driver CBGSDriver;

CBaseGameStats_Driver::CBaseGameStats_Driver( void ) :
	BaseClass( "CGameStats" ),
	m_iLoadedVersion( -1 ),
	m_bEnabled( false ),
	m_bShuttingDown( false ),
	m_bInLevel( false ),
	m_bFirstLevel( true ),
	m_flLevelStartTime( 0.0f ),
	m_bStationary( false ),
	m_flLastMovementTime( 0.0f ),
	m_bGamePaused( false )
{
	m_szLoadedUserID[0] = 0;;
	m_tLastUpload = 0;
	m_LastUserCmd.Reset();
}

static FileHandle_t g_LogFileHandle = FILESYSTEM_INVALID_HANDLE;

CBaseGameStats::CBaseGameStats() :
	m_bLogging( false ),
	m_bLoggingToFile( false )
{
}

bool CBaseGameStats::StatTrackingAllowed( void )
{
	return CBGSDriver.m_bEnabled;
}

// Don't care about vcr hooks here...
#undef localtime
#undef asctime
#undef time

#include <time.h>

void CBaseGameStats::StatsLog( char const *fmt, ... )
{
	if ( !m_bLogging && !m_bLoggingToFile )
		return;

	char buf[ 2048 ];
	va_list argptr;
	va_start( argptr, fmt );
	Q_vsnprintf( buf, sizeof( buf ), fmt, argptr );
	va_end( argptr );

	// Prepend timestamp and spew it

	// Prepend the time.
	time_t aclock;
	time( &aclock );
	struct tm *newtime = localtime( &aclock );

	
	char timeString[ 128 ];
	Q_strncpy( timeString, asctime( newtime ), sizeof( timeString ) );
	// Get rid of the \n.
	char *pEnd = strstr( timeString, "\n" );
	if ( pEnd )
	{
		*pEnd = 0;
	}

	if ( m_bLogging )
	{
		DevMsg( "[GS %s - %7.2f] %s", timeString, gpGlobals->realtime, buf );
	}

	if ( m_bLoggingToFile )
	{
		if ( FILESYSTEM_INVALID_HANDLE == g_LogFileHandle )
		{
			g_LogFileHandle = filesystem->Open( GAMESTATS_LOG_FILE, "a", GAMESTATS_PATHID );
		}

		if ( FILESYSTEM_INVALID_HANDLE != g_LogFileHandle )
		{
			filesystem->FPrintf( g_LogFileHandle, "[GS %s - %7.2f] %s", timeString, gpGlobals->realtime, buf );
			filesystem->Flush( g_LogFileHandle );
		}
	}
}

static char s_szSaveFileName[256] = "";
static char s_szStatUploadRegistryKeyName[256] = "";
static char s_szPseudoUniqueID[20] = "";

const char *CBaseGameStats::GetStatSaveFileName( void )
{
	AssertMsg( s_szSaveFileName[0] != '\0', "Don't know what file to save stats to." );
	return s_szSaveFileName;
}

const char *CBaseGameStats::GetStatUploadRegistryKeyName( void )
{
	AssertMsg( s_szStatUploadRegistryKeyName[0] != '\0', "Don't know the registry key to use to mark stats uploads." );
	return s_szStatUploadRegistryKeyName;
}

const char *CBaseGameStats::GetUserPseudoUniqueID( void )
{
	AssertMsg( s_szPseudoUniqueID[0] != '\0', "Don't have a pseudo unique ID." );
	return s_szPseudoUniqueID;
}

void CBaseGameStats::Event_Init( void )
{
	SetHL2UnlockedChapterStatistic();
	SetSteamStatistic( filesystem->IsSteam() );
	SetCyberCafeStatistic( gamestatsuploader->IsCyberCafeUser() );
	ConVarRef pDXLevel( "mat_dxlevel" );
	if( pDXLevel.IsValid() )
	{
		SetDXLevelStatistic( pDXLevel.GetInt() );
	}
	++m_BasicStats.m_Summary.m_nCount;

	StatsLog( "CBaseGameStats::Event_Init [%dth session]\n", m_BasicStats.m_Summary.m_nCount );
}

void CBaseGameStats::Event_Shutdown( void )
{
	StatsLog( "CBaseGameStats::Event_Shutdown [%dth session]\n", m_BasicStats.m_Summary.m_nCount );

	StatsLog( "\n====================================================================\n\n" );
}

void CBaseGameStats::Event_MapChange( const char *szOldMapName, const char *szNewMapName )
{
	StatsLog( "CBaseGameStats::Event_MapChange to [%s]\n", szNewMapName );
}

void CBaseGameStats::Event_LevelInit( void )
{
	StatsLog( "CBaseGameStats::Event_LevelInit [%s]\n", CBGSDriver.m_PrevMapName.String() );

	BasicGameStatsRecord_t *map = gamestats->m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
	++map->m_nCount;

	// HACK HACK:  Punching this hole through only works in single player!!!
	if ( gpGlobals->maxClients == 1 )
	{
		ConVarRef closecaption( "closecaption" );
		if( closecaption.IsValid() )
			SetCaptionsStatistic( closecaption.GetBool() );

		SetHDRStatistic( gamestatsuploader->IsHDREnabled() );

		SetSkillStatistic( skill.GetInt() );
		SetSteamStatistic( filesystem->IsSteam() );
		SetCyberCafeStatistic( gamestatsuploader->IsCyberCafeUser() );
	}
}

void CBaseGameStats::Event_LevelShutdown( float flElapsed )
{
	BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
	Assert( map );
	map->m_nSeconds += (int)flElapsed;
	gamestats->m_BasicStats.m_Summary.m_nSeconds += (int)flElapsed;

	StatsLog( "CBaseGameStats::Event_LevelShutdown [%s] %.2f elapsed %d total\n", CBGSDriver.m_PrevMapName.String(), flElapsed, gamestats->m_BasicStats.m_Summary.m_nSeconds );
}

void CBaseGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	++m_BasicStats.m_Summary.m_nDeaths;

	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		++map->m_nDeaths;
		StatsLog( "  Player died %dth time in level [%s]!!!\n", map->m_nDeaths, CBGSDriver.m_PrevMapName.String() );
	}
	else
	{
		StatsLog( "  Player died, but not in a level!!!\n" );
		Assert( 0 );
	}

	StatsLog( "CBaseGameStats::Event_PlayerKilled [%s] [%dth death]\n", pPlayer->GetPlayerName(), m_BasicStats.m_Summary.m_nDeaths );
}

void CBaseGameStats::Event_SaveGame( void )
{
	StatsLog( "CBaseGameStats::Event_SaveGame [%s]\n", CBGSDriver.m_PrevMapName.String() );
}

void CBaseGameStats::Event_LoadGame( void )
{
	char const *pchSaveFile = engine->GetMostRecentlyLoadedFileName();
	StatsLog( "CBaseGameStats::Event_LoadGame [%s] from %s\n", CBGSDriver.m_PrevMapName.String(), pchSaveFile );
}

void CBaseGameStats::Event_Commentary()
{
	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		++map->m_nCommentary;
	}

	++m_BasicStats.m_Summary.m_nCommentary;

	StatsLog( "CBaseGameStats::Event_Commentary [%d]\n", m_BasicStats.m_Summary.m_nCommentary );
}

void CBaseGameStats::Event_Credits()
{
	StatsLog( "CBaseGameStats::Event_Credits\n" );

	float elapsed = 0.0f;
	if( CBGSDriver.m_bInLevel )
	{
		elapsed = gpGlobals->realtime - CBGSDriver.m_flLevelStartTime;
	}

	if( elapsed < 0.0f )
	{
		Assert( 0 );
		Warning( "EVENT_CREDITS with negative elapsed time (rt %f starttime %f)\n", gpGlobals->realtime, CBGSDriver.m_flLevelStartTime );
		elapsed = 0.0f;
	}

	// Only set this one time!!!
	if( gamestats->m_BasicStats.m_nSecondsToCompleteGame == 0 )
	{
		if( gamestats->UserPlayedAllTheMaps() )
		{
			gamestats->m_BasicStats.m_nSecondsToCompleteGame = elapsed + gamestats->m_BasicStats.m_Summary.m_nSeconds;
			gamestats->SaveToFileNOW();
		}
	}
}

void CBaseGameStats::Event_CrateSmashed()
{
	StatsLog( "CBaseGameStats::Event_CrateSmashed\n" );
}

void CBaseGameStats::Event_Punted( CBaseEntity *pObject )
{
	StatsLog( "CBaseGameStats::Event_Punted [%s]\n", pObject->GetClassname() );
}

void CBaseGameStats::Event_PlayerTraveled( CBasePlayer *pBasePlayer, float distanceInInches, bool bInVehicle, bool bSprinting )
{
}

void CBaseGameStats::Event_FlippedVehicle( CBasePlayer *pDriver, CPropVehicleDriveable *pVehicle )
{
	StatsLog( "CBaseGameStats::Event_FlippedVehicle [%s] flipped [%s]\n", pDriver->GetPlayerName(), pVehicle->GetClassname() );
}

// Called before .sav file is actually loaded (player should still be in previous level, if any)
void CBaseGameStats::Event_PreSaveGameLoaded( char const *pSaveName, bool bInGame )
{
	StatsLog( "CBaseGameStats::Event_PreSaveGameLoaded [%s] %s\n", pSaveName, bInGame ? "in-game" : "at console" );
}

bool CBaseGameStats::SaveToFileNOW( bool bForceSyncWrite /* = false */ )
{
	if ( !StatsTrackingIsFullyEnabled() )
		return false;

	CUtlBuffer buf;
	buf.PutShort( GAMESTATS_FILE_VERSION );
	buf.Put( s_szPseudoUniqueID, 16 );

	if( ShouldTrackStandardStats() )
		m_BasicStats.SaveToBuffer( buf ); 
	else
		buf.PutInt( GAMESTATS_STANDARD_NOT_SAVED );

	gamestats->AppendCustomDataToSaveBuffer( buf );

	char fullpath[ 512 ] = { 0 };
	if ( filesystem->FileExists( GetStatSaveFileName(), GAMESTATS_PATHID ) )
	{
		filesystem->RelativePathToFullPath( GetStatSaveFileName(), GAMESTATS_PATHID, fullpath, sizeof( fullpath ) );
	}
	else
	{
		// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
		char gamePath[256];
		engine->GetGameDir( gamePath, 256 );
		Q_StripTrailingSlash( gamePath );
		Q_snprintf( fullpath, sizeof( fullpath ), "%s/%s", gamePath, GetStatSaveFileName() );
		Q_strlower( fullpath );
		Q_FixSlashes( fullpath );
	}

	// StatsLog( "SaveToFileNOW '%s'\n", fullpath );

	if( CBGSDriver.m_bShuttingDown || bForceSyncWrite ) //write synchronously
	{
		filesystem->WriteFile( fullpath, GAMESTATS_PATHID, buf );

		StatsLog( "Shut down wrote to '%s'\n", fullpath );
	}
	else
	{
		// Allocate memory for async system to use (and free afterward!!!)
		size_t nBufferSize = buf.TellPut();
		void *pMem = malloc(nBufferSize);
		CUtlBuffer statsBuffer( pMem, nBufferSize );
		statsBuffer.Put( buf.Base(), nBufferSize );

		// Write data async
		filesystem->AsyncWrite( fullpath, statsBuffer.Base(), statsBuffer.TellPut(), true, false );
	}

	return true;
}

void CBaseGameStats::Event_PlayerConnected( CBasePlayer *pBasePlayer )
{
	StatsLog( "CBaseGameStats::Event_PlayerConnected [%s]\n", pBasePlayer->GetPlayerName() );
}

void CBaseGameStats::Event_PlayerDisconnected( CBasePlayer *pBasePlayer )
{
	StatsLog( "CBaseGameStats::Event_PlayerDisconnected\n", pBasePlayer->GetPlayerName() );
}

void CBaseGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info )
{
	//StatsLog( "CBaseGameStats::Event_PlayerDamage [%s] took %.2f damage\n", pBasePlayer->GetPlayerName(), info.GetDamage() );
}

void CBaseGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	StatsLog( "CBaseGameStats::Event_PlayerKilledOther [%s] killed [%s]\n", pAttacker->GetPlayerName(), pVictim->GetClassname() );
}

void CBaseGameStats::Event_WeaponFired( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName )
{
	StatsLog( "CBaseGameStats::Event_WeaponFired [%s] %s weapon [%s]\n", pShooter->GetPlayerName(), bPrimary ? "primary" : "secondary", pchWeaponName );
}

void CBaseGameStats::Event_WeaponHit( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName, const CTakeDamageInfo &info )
{
	StatsLog( "CBaseGameStats::Event_WeaponHit [%s] %s weapon [%s] damage [%f]\n", pShooter->GetPlayerName(), bPrimary ? "primary" : "secondary", pchWeaponName, info.GetDamage() );
}

void CBaseGameStats::Event_PlayerEnteredGodMode( CBasePlayer *pBasePlayer )
{
	StatsLog( "CBaseGameStats::Event_PlayerEnteredGodMode [%s] entered GOD mode\n", pBasePlayer->GetPlayerName() );
}

void CBaseGameStats::Event_PlayerEnteredNoClip( CBasePlayer *pBasePlayer )
{
	StatsLog( "CBaseGameStats::Event_PlayerEnteredNoClip [%s] entered NOCLIPe\n", pBasePlayer->GetPlayerName() );
}

void CBaseGameStats::Event_DecrementPlayerEnteredNoClip( CBasePlayer *pBasePlayer )
{
	StatsLog( "CBaseGameStats::Event_DecrementPlayerEnteredNoClip [%s] decrementing NOCLIPe\n", pBasePlayer->GetPlayerName() );
}

void CBaseGameStats::Event_IncrementCountedStatistic( const Vector& vecAbsOrigin, char const *pchStatisticName, float flIncrementAmount )
{
	StatsLog( "Incrementing %s by %f at pos (%d, %d, %d)\n", pchStatisticName, flIncrementAmount, (int)vecAbsOrigin.x, (int)vecAbsOrigin.y, (int)vecAbsOrigin.z );
}

bool CBaseGameStats::UploadStatsFileNOW( void )
{
	if( !StatsTrackingIsFullyEnabled() )
	{
		StatsLog( "UploadStatsFileNOW: stats tracking not fully enabled, not uploading file\n" );
		return false;
	}
	
	if (!HaveValidData() )
	{
		StatsLog( "UploadStatsFileNOW: no valid game data, not uploading file\n" );
		return false;
	}

	if ( !filesystem->FileExists( gamestats->GetStatSaveFileName(), GAMESTATS_PATHID ) )
	{
		StatsLog( "UploadStatsFileNOW: can't find stats file, not uploading file\n" );
		return false;
	}

	int curtime = Plat_FloatTime();

	CBGSDriver.m_tLastUpload = curtime;

	// Update the registry
#ifndef SWDS
	IRegistry *reg = InstanceRegistry( "Steam" );
	Assert( reg );
	reg->WriteInt( GetStatUploadRegistryKeyName(), CBGSDriver.m_tLastUpload );
	ReleaseInstancedRegistry( reg );
#endif

	CUtlBuffer buf;
	filesystem->ReadFile( GetStatSaveFileName(), GAMESTATS_PATHID, buf );
	unsigned int uBlobSize = buf.TellPut();
	if ( uBlobSize == 0 )
	{
		StatsLog( "UploadStatsFileNOW: can't read stats file, not uploading file\n" );
		return false;
	}

	const void *pvBlobData = ( const void * )buf.Base();

	if( gamestatsuploader )
	{
		bool bRet = gamestatsuploader->UploadGameStats( "",
												   1,
												   uBlobSize,
												   pvBlobData );

		StatsLog( "UploadStatsFileNow: UploadGameStats %s\n", bRet ? "succeeded" : "failed" );
	}

	return false;
}


void CBaseGameStats::LoadingEvent_PlayerIDDifferentThanLoadedStats( void )
{
	StatsLog( "CBaseGameStats::LoadingEvent_PlayerIDDifferentThanLoadedStats\n" );
}


bool CBaseGameStats::LoadFromFile( void )
{
	if ( filesystem->FileExists( gamestats->GetStatSaveFileName(), GAMESTATS_PATHID ) )
	{
		char fullpath[ 512 ];
		filesystem->RelativePathToFullPath( gamestats->GetStatSaveFileName(), GAMESTATS_PATHID, fullpath, sizeof( fullpath ) );
		StatsLog( "Loading stats from '%s'\n", fullpath );
	}
	
	CUtlBuffer buf; 
	if ( filesystem->ReadFile( gamestats->GetStatSaveFileName(), GAMESTATS_PATHID, buf ) )
	{
		bool bRetVal = true;

		int version = buf.GetShort();
		if ( version > GAMESTATS_FILE_VERSION )
			return false; //file is beyond our comprehension

		// Set global parse version
		CBGSDriver.m_iLoadedVersion = version;

		buf.Get( CBGSDriver.m_szLoadedUserID, 16 );
		CBGSDriver.m_szLoadedUserID[ sizeof( CBGSDriver.m_szLoadedUserID ) - 1 ] = 0;

		if ( s_szPseudoUniqueID[ 0 ] != 0 )
		{			
			if ( Q_stricmp( CBGSDriver.m_szLoadedUserID, s_szPseudoUniqueID ) )
			{
				//UserID changed, blow away log!!!
				filesystem->RemoveFile( gamestats->GetStatSaveFileName(), GAMESTATS_PATHID );
				filesystem->RemoveFile( GAMESTATS_LOG_FILE, GAMESTATS_PATHID );
				Warning( "Userid changed, clearing stats file\n" );
				CBGSDriver.m_szLoadedUserID[0] = '\0';
				CBGSDriver.m_iLoadedVersion = -1;
				gamestats->m_BasicStats.Clear();
				gamestats->LoadingEvent_PlayerIDDifferentThanLoadedStats();
				bRetVal = false;
			}
		
			if ( version <= GAMESTATS_FILE_VERSION_OLD5 )
			{
				gamestats->m_BasicStats.Clear();
				bRetVal = false;
			}
			else
			{
				// Peek ahead in buffer to see if we have the "no default stats" secret flag set.
				int iCheckForStandardStatsInFile = *( int * )buf.PeekGet();
				bool bValid = true;

				if ( iCheckForStandardStatsInFile != GAMESTATS_STANDARD_NOT_SAVED )
				{
					//the GAMESTATS_STANDARD_NOT_SAVED flag coincides with user completion time, rewind so the gamestats parser can grab it
					bValid = gamestats->m_BasicStats.ParseFromBuffer( buf, version );
				}
				else
				{
					// skip over the flag
					buf.GetInt();
				}

				if( !bValid )
				{
					m_BasicStats.Clear();
				}

				if( ( buf.TellPut() - buf.TellGet() ) != 0 ) //more data left, must be custom data
				{
					gamestats->LoadCustomDataFromBuffer( buf );
				}
			}
		}

		return bRetVal;
	}
	else
	{
		filesystem->RemoveFile( GAMESTATS_LOG_FILE, GAMESTATS_PATHID );
	}

	return false;	
}

bool CBaseGameStats_Driver::Init()
{
	const char *pGameDir = CommandLine()->ParmValue( "-game", "hl2" );

	//standardizing is a good thing
	char szLoweredGameDir[256];
	Q_strncpy( szLoweredGameDir, pGameDir, sizeof( szLoweredGameDir ) );
	Q_strlower( szLoweredGameDir );

	gamestats = gamestats->OnInit( gamestats, szLoweredGameDir );

	//determine constant strings needed for saving and uploading
	Q_strncpy( s_szSaveFileName, szLoweredGameDir, sizeof( s_szSaveFileName ) );
	Q_strncat( s_szSaveFileName, "_gamestats.dat", sizeof( s_szSaveFileName ) );

	Q_strncpy( s_szStatUploadRegistryKeyName, "GameStatsUpload_", sizeof( s_szStatUploadRegistryKeyName ) );
	Q_strncat( s_szStatUploadRegistryKeyName, szLoweredGameDir, sizeof( s_szStatUploadRegistryKeyName ) );

	gamestats->m_bLoggingToFile = CommandLine()->FindParm( "-gamestatsloggingtofile" ) ? true : false;
	if ( gamestats->m_bLoggingToFile )
	{
		gamestats->m_bLogging = true;
	}
	else
	{
		gamestats->m_bLogging = CommandLine()->FindParm( "-gamestatslogging" ) ? true : false;
	}

#if 0
	// This should only impact us internally!!!
	if ( IsPC() && !filesystem->IsSteam() )
	{
		Warning( "Forcing gamestats logging to file, remove before shipping!!!\n" );
		gamestats->m_bLoggingToFile = true;
	}
#endif

	if ( gamestatsuploader )
	{
		m_bEnabled = gamestatsuploader->IsGameStatsLoggingEnabled();
		if ( m_bEnabled )
		{
			gamestatsuploader->GetPseudoUniqueId( s_szPseudoUniqueID, sizeof( s_szPseudoUniqueID ) );
		}
	}

	if ( StatsTrackingIsFullyEnabled() )
	{
		// FIXME: Load m_tLastUpload from registry and save it back out, too
#ifndef SWDS
		IRegistry *reg = InstanceRegistry( "Steam" );
		Assert( reg );
		m_tLastUpload = reg->ReadInt( gamestats->GetStatUploadRegistryKeyName(), 0 );
		ReleaseInstancedRegistry( reg );
#endif

		//load existing stats
		gamestats->LoadFromFile();
	}
		
	if ( s_szPseudoUniqueID[ 0 ] != 0 )
	{			
		gamestats->Event_Init();

		if( gamestats->AutoSave_OnInit() )
			gamestats->SaveToFileNOW();

		if( gamestats->AutoUpload_OnInit() )
			gamestats->UploadStatsFileNOW();
	}
	else
	{
		m_bEnabled = false; //unable to generate a pseudo-unique ID, disable tracking
	}

	return true;
}

void CBaseGameStats_Driver::Shutdown()
{
	m_bShuttingDown = true;

	gamestats->Event_Shutdown();
	
	if( gamestats->AutoSave_OnShutdown() )
		gamestats->SaveToFileNOW();

	if( gamestats->AutoUpload_OnShutdown() )
		gamestats->UploadStatsFileNOW();

	if ( FILESYSTEM_INVALID_HANDLE != g_LogFileHandle )
	{
		filesystem->Close( g_LogFileHandle );
		g_LogFileHandle = FILESYSTEM_INVALID_HANDLE;
	}
}

void CBaseGameStats_Driver::PossibleMapChange( void )
{
	//detect and copy map changes
	if ( Q_stricmp( m_PrevMapName.String(), STRING( gpGlobals->mapname ) ) )
	{
		MEM_ALLOC_CREDIT();

		CUtlString PrevMapBackup = m_PrevMapName;

		m_PrevMapName = STRING( gpGlobals->mapname );

		gamestats->Event_MapChange( PrevMapBackup.String(), STRING( gpGlobals->mapname ) );

		if( gamestats->AutoSave_OnMapChange() )
			gamestats->SaveToFileNOW();

		if( gamestats->AutoUpload_OnMapChange() )
			gamestats->UploadStatsFileNOW();
	}
}



void CBaseGameStats_Driver::LevelInitPreEntity()
{
	m_bInLevel = true;
	m_bFirstLevel = false;

	if ( Q_stricmp( s_szPseudoUniqueID, "unknown" ) == 0 )
	{
		// "unknown" means this is a dedicated server and we weren't able to generate a unique ID (e.g. Linux server).
		// Change the unique ID to be a hash of IP & port.  We couldn't do this earlier because IP is not known until level
		// init time.
		ConVar *hostip = cvar->FindVar( "hostip" );
		ConVar *hostport = cvar->FindVar( "hostport" );
		if ( hostip && hostport )
		{
			int crcInput[2];
			crcInput[0] = hostip->GetInt();
			crcInput[1] = hostport->GetInt();
			if ( crcInput[0] && crcInput[1] )
			{
				CRC32_t crc = CRC32_ProcessSingleBuffer( crcInput, sizeof( crcInput ) );
				Q_snprintf( s_szPseudoUniqueID, ARRAYSIZE( s_szPseudoUniqueID ), "H:%x", crc );
			}
		}
	}

	PossibleMapChange();

	m_flPauseStartTime = 0.0f;
	m_flLevelStartTime = gpGlobals->realtime;

	gamestats->Event_LevelInit();

	if( gamestats->AutoSave_OnLevelInit() )
		gamestats->SaveToFileNOW();

	if( gamestats->AutoUpload_OnLevelInit() )
		gamestats->UploadStatsFileNOW();
}


void CBaseGameStats_Driver::LevelShutdownPreEntity()
{
	float flElapsed = gpGlobals->realtime - m_flLevelStartTime;

	if ( flElapsed < 0.0f )
	{
		Assert( 0 );
		Warning( "EVENT_LEVELSHUTDOWN:  with negative elapsed time (rt %f starttime %f)\n", gpGlobals->realtime, m_flLevelStartTime );
		flElapsed = 0.0f;
	}

	//Assert( m_bInLevel ); //so, apparently shutdowns can happen before inits
	if ( m_bInLevel && ( gpGlobals->eLoadType != MapLoad_Background ) )
	{
		gamestats->Event_LevelShutdown( flElapsed );

		if( gamestats->AutoSave_OnLevelShutdown() )
			gamestats->SaveToFileNOW( true );

		if( gamestats->AutoUpload_OnLevelShutdown() )
			gamestats->UploadStatsFileNOW();

		m_bInLevel = false;	
	}
}

void CBaseGameStats_Driver::OnSave()
{
	gamestats->Event_SaveGame();
}


void CBaseGameStats_Driver::OnRestore()
{
	PossibleMapChange();

	gamestats->Event_LoadGame();
}


void CBaseGameStats_Driver::FrameUpdatePostEntityThink()
{
	bool bGamePaused = ( gpGlobals->frametime == 0.0f );

	if ( !m_bInLevel )
	{
		m_flPauseStartTime = 0.0f;
	}
	else if ( m_bGamePaused != bGamePaused )
	{
		if ( bGamePaused )
		{
			m_flPauseStartTime = gpGlobals->realtime;
		}
		else if ( m_flPauseStartTime != 0.0f )
		{
			float flPausedTime = gpGlobals->realtime - m_flPauseStartTime;
			if ( flPausedTime < 0.0f )
			{
				Assert( 0 );
				Warning( "Game paused time showing up negative (rt %f pausestart %f)\n", gpGlobals->realtime, m_flPauseStartTime );
				flPausedTime = 0.0f;
			}

			// Remove this from global time counters

			//			Msg( "Pause:  adding %f to level starttime\n", flPausedTime );

			m_flLevelStartTime += flPausedTime;
			m_flPauseStartTime = 0.0f;

			//			Msg( "Paused for %.2f seconds\n", flPausedTime );
		}
		m_bGamePaused = bGamePaused;
	}
}

bool UserCmdChanged( const CUserCmd& lhs, const CUserCmd& rhs )
{
	if ( lhs.viewangles	!= rhs.viewangles )
		return true;
	if ( lhs.forwardmove != rhs.forwardmove )
		return true;
	if ( lhs.sidemove != rhs.sidemove )
		return true;
	if ( lhs.upmove	!= rhs.upmove )
		return true;
	if ( lhs.buttons != rhs.buttons )
		return true;
	if ( lhs.impulse != rhs.impulse )
		return true;
	if ( lhs.weaponselect != rhs.weaponselect )
		return true;
	if ( lhs.weaponsubtype != rhs.weaponsubtype )
		return true;
	if ( lhs.mousedx != rhs.mousedx )
		return true;
	if ( lhs.mousedy != rhs.mousedy )
		return true;
	return false;
}

bool StatsTrackingIsFullyEnabled( void )
{
	return CBGSDriver.m_bEnabled && gamestats->StatTrackingEnabledForMod();
}

void CBaseGameStats::Clear( void )
{
	gamestats->m_BasicStats.Clear();
}

void CBaseGameStats::SetSteamStatistic( bool bUsingSteam )
{
	if( CBGSDriver.m_bFirstLevel )
	{
		m_BasicStats.m_Summary.m_bSteam = bUsingSteam;
	}

	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		map->m_bSteam = bUsingSteam;
	}

	m_BasicStats.m_bSteam = bUsingSteam;
}

void CBaseGameStats::SetCyberCafeStatistic( bool bIsCyberCafeUser )
{
	if( CBGSDriver.m_bFirstLevel )
	{
		m_BasicStats.m_Summary.m_bCyberCafe = bIsCyberCafeUser;
	}

	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		map->m_bCyberCafe = bIsCyberCafeUser;
	}

	m_BasicStats.m_bCyberCafe = bIsCyberCafeUser;
}

void CBaseGameStats::SetHDRStatistic( bool bHDREnabled )
{
	if( bHDREnabled )
	{
		if( CBGSDriver.m_bInLevel )
		{
			BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
			++map->m_nHDR;
		}

		if( CBGSDriver.m_bFirstLevel )
		{
			++m_BasicStats.m_Summary.m_nHDR;
		}
	}
}

void CBaseGameStats::SetCaptionsStatistic( bool bClosedCaptionsEnabled )
{
	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		++map->m_nCaptions;
	}

	if( CBGSDriver.m_bFirstLevel )
	{
		++m_BasicStats.m_Summary.m_nCaptions;
	}
}

void CBaseGameStats::SetSkillStatistic( int iSkillSetting )
{
	int skill = clamp( iSkillSetting, 1, 3 ) - 1;

	if( CBGSDriver.m_bInLevel )
	{
		BasicGameStatsRecord_t *map = m_BasicStats.FindOrAddRecordForMap( CBGSDriver.m_PrevMapName.String() );
		++map->m_nSkill[ skill ];
	}

	if ( CBGSDriver. m_bFirstLevel )
	{
		++m_BasicStats.m_Summary.m_nSkill[ skill ];
	}
}

void CBaseGameStats::SetDXLevelStatistic( int iDXLevel )
{
	m_BasicStats.m_nDXLevel = iDXLevel;
}

void CBaseGameStats::SetHL2UnlockedChapterStatistic( void )
{
	// Now grab the hl2/cfg/config.cfg and suss out the sv_unlockedchapters cvar to estimate how far they got in HL2
	char const *relative = "cfg/config.cfg";
	char fullpath[ 512 ];
	char gamedir[256];
	engine->GetGameDir( gamedir, 256 );
	Q_snprintf( fullpath, sizeof( fullpath ), "%s/../hl2/%s", gamedir, relative );

	if ( filesystem->FileExists( fullpath ) )
	{
		FileHandle_t fh = filesystem->Open( fullpath, "rb" );
		if ( FILESYSTEM_INVALID_HANDLE != fh )
		{
			// read file into memory
			int size = filesystem->Size(fh);
			char *configBuffer = new char[ size + 1 ];
			filesystem->Read( configBuffer, size, fh );
			configBuffer[size] = 0;
			filesystem->Close( fh );

			// loop through looking for all the cvars to apply
			const char *search = Q_stristr(configBuffer, "sv_unlockedchapters" );
			if ( search )
			{
				// read over the token
				search = strtok( (char *)search, " \n" );
				search = strtok( NULL, " \n" );

				if ( search[0]== '\"' )
					++search;

				// read the value
				int iChapter = Q_atoi( search );
				m_BasicStats.m_nHL2ChaptureUnlocked = iChapter;
			}

			// free
			delete [] configBuffer;
		}
	}	
}

static void CC_ResetGameStats( const CCommand &args )
{
	gamestats->Clear();
	gamestats->SaveToFileNOW();
	gamestats->StatsLog( "CC_ResetGameStats : Server cleared game stats\n" );
}

static ConCommand resetGameStats("_resetgamestats", CC_ResetGameStats, "Erases current game stats and writes out a blank stats file", 0 );



class CPointGamestatsCounter : public CPointEntity
{
public:
	DECLARE_CLASS( CPointGamestatsCounter, CPointEntity );
	DECLARE_DATADESC();

	CPointGamestatsCounter();

protected:

	void InputSetName( inputdata_t &inputdata );
	void InputIncrement( inputdata_t &inputdata );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
private:

	string_t		m_strStatisticName;
	bool			m_bDisabled;
};

BEGIN_DATADESC( CPointGamestatsCounter )

	DEFINE_KEYFIELD( m_strStatisticName, FIELD_STRING, "Name" ),
	DEFINE_FIELD( m_bDisabled, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetName", InputSetName ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Increment", InputIncrement ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_gamestats_counter, CPointGamestatsCounter )


CPointGamestatsCounter::CPointGamestatsCounter() :
	m_strStatisticName( NULL_STRING ),
	m_bDisabled( false )
{
}

//-----------------------------------------------------------------------------
// Purpose: Changes name of statistic
//-----------------------------------------------------------------------------
void CPointGamestatsCounter::InputSetName( inputdata_t &inputdata )
{
	m_strStatisticName = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
// Purpose: Changes name of statistic
//-----------------------------------------------------------------------------
void CPointGamestatsCounter::InputIncrement( inputdata_t &inputdata )
{
	if ( m_bDisabled )
		return;

	if ( NULL_STRING == m_strStatisticName )
	{
		DevMsg( 1, "CPointGamestatsCounter::InputIncrement:  No stat name specified for point_gamestats_counter @%f, %f, %f [ent index %d]\n",
			GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, entindex() );
		return;
	}

	gamestats->Event_IncrementCountedStatistic( GetAbsOrigin(), STRING( m_strStatisticName ), inputdata.value.Float() );
}

void CPointGamestatsCounter::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

void CPointGamestatsCounter::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}
