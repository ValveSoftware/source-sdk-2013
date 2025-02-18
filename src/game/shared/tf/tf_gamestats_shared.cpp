//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#ifdef GAME_DLL
	#include "gamestats.h"
#else
	#include "tf_hud_statpanel.h"
#endif
#include "tf_gamestats_shared.h"

#ifndef NO_STEAM
#include "steamworks_gamestats.h"
#endif

int TF_Gamestats_RoundStats_t::m_iNumRounds = 0;
time_t TF_Gamestats_RoundStats_t::m_iRoundStartTime = 0;

//-----------------------------------------------------------------------------

const char *s_pStatStrings[ TFSTAT_TOTAL ] =
{
	"TFSTAT_UNDEFINED",
	"TFSTAT_SHOTS_HIT",
	"TFSTAT_SHOTS_FIRED",
	"TFSTAT_KILLS",
	"TFSTAT_DEATHS",
	"TFSTAT_DAMAGE",
	"TFSTAT_CAPTURES",
	"TFSTAT_DEFENSES",
	"TFSTAT_DOMINATIONS",
	"TFSTAT_REVENGE",
	"TFSTAT_POINTSSCORED",
	"TFSTAT_BUILDINGSDESTROYED",
	"TFSTAT_HEADSHOTS",
	"TFSTAT_PLAYTIME",
	"TFSTAT_HEALING",
	"TFSTAT_INVULNS",
	"TFSTAT_KILLASSISTS",
	"TFSTAT_BACKSTABS",
	"TFSTAT_HEALTHLEACHED",
	"TFSTAT_BUILDINGSBUILT",
	"TFSTAT_MAXSENTRYKILLS",
	"TFSTAT_TELEPORTS",
	"TFSTAT_FIREDAMAGE",
	"TFSTAT_BONUS_POINTS",
	"TFSTAT_BLASTDAMAGE",
	"TFSTAT_DAMAGETAKEN",
	"TFSTAT_HEALTHKITS",
	"TFSTAT_AMMOKITS",
	"TFSTAT_CLASSCHANGES",
	"TFSTAT_CRITS",
	"TFSTAT_SUICIDES",
	"TFSTAT_CURRENCY_COLLECTED",
	"TFSTAT_DAMAGE_ASSIST",
	"TFSTAT_HEALING_ASSIST",
	"TFSTAT_DAMAGE_BOSS",
	"TFSTAT_DAMAGE_BLOCKED",
	"TFSTAT_DAMAGE_RANGED",
	"TFSTAT_DAMAGE_RANGED_CRIT_RANDOM",
	"TFSTAT_DAMAGE_RANGED_CRIT_BOOSTED",
	"TFSTAT_REVIVED",
};

const char *s_pMapStatStrings[ TFMAPSTAT_TOTAL ] =
{
	"TFSTAT_UNDEFINED",
	"TFSTAT_PLAYTIME",
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_LevelStats_t::TF_Gamestats_LevelStats_t()
{
	m_bInitialized = false;
	m_iRoundStartTime = 0;
	m_flRoundStartTime = 0;
	m_Header.m_iRoundsPlayed = 0;
	m_Header.m_iTotalTime = 0;
	m_Header.m_iBlueWins = 0;
	m_Header.m_iRedWins = 0;
	m_Header.m_iStalemates = 0;
	m_Header.m_iBlueSuddenDeathWins = 0;
	m_Header.m_iRedSuddenDeathWins = 0;
	Q_memset( m_aClassStats, 0, sizeof( m_aClassStats ) );
	Q_memset( m_aWeaponStats, 0, sizeof( m_aWeaponStats ) );
	Q_memset( m_iPeakPlayerCount, 0, sizeof( m_iPeakPlayerCount ) );

	for ( int i = 0; i <= MAX_CONTROL_POINTS; i++ )
	{
		m_Header.m_iLastCapChangedInRound[i] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_LevelStats_t::~TF_Gamestats_LevelStats_t()
{
	//m_aPlayerDeaths.Purge();
	//m_aPlayerDamage.Purge();
	m_bIsRealServer	= false;
}

//-----------------------------------------------------------------------------
// Purpose: Copy constructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_LevelStats_t::TF_Gamestats_LevelStats_t( const TF_Gamestats_LevelStats_t &stats )
{
	m_bInitialized		= stats.m_bInitialized;
	m_iRoundStartTime	= stats.m_iRoundStartTime;
	m_flRoundStartTime	= stats.m_flRoundStartTime;
	m_iMapStartTime		= stats.m_iMapStartTime;
	m_Header			= stats.m_Header;
	m_bIsRealServer		= stats.m_bIsRealServer;
	//m_aPlayerDeaths		= stats.m_aPlayerDeaths;
	//m_aPlayerDamage		= stats.m_aPlayerDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszMapName - 
//			nIPAddr - 
//			nPort - 
//			flStartTime - 
//-----------------------------------------------------------------------------
void TF_Gamestats_LevelStats_t::Init( const char *pszMapName, int nMapRevision, int nIPAddr, short nPort, float flStartTime  )
{
	Q_memset( &m_Header, 0, sizeof( m_Header ) ); // TODO: This is correct for steamworks stats, but probably breaks old stats!!!

	V_FileBase( pszMapName, m_Header.m_szMapName, sizeof( m_Header.m_szMapName ) );

	m_Header.m_nMapRevision = nMapRevision;
	m_Header.m_nIPAddr = nIPAddr;
	m_Header.m_nPort = nPort;
	
#ifndef NO_STEAM
	// Start the level timer.
	m_iMapStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	m_iRoundStartTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
	m_flRoundStartTime = gpGlobals->curtime;
#endif

	m_bIsRealServer = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flEndTime - 
//-----------------------------------------------------------------------------
void TF_Gamestats_LevelStats_t::Shutdown( float flEndTime )
{
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_RoundStats_t::TF_Gamestats_RoundStats_t()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_RoundStats_t::~TF_Gamestats_RoundStats_t()
{
}

//-----------------------------------------------------------------------------
// Purpose: resets the state of stat tracking
//-----------------------------------------------------------------------------
void TF_Gamestats_RoundStats_t::Reset()
{
	ResetSummary();
	m_iRoundStartTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TF_Gamestats_RoundStats_t::ResetSummary()
{
	Q_memset( &m_Summary, 0, sizeof( m_Summary ) );

}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_KillStats_t::TF_Gamestats_KillStats_t()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
TF_Gamestats_KillStats_t::~TF_Gamestats_KillStats_t()
{
}

//-----------------------------------------------------------------------------
// Purpose: resets the state of stat tracking
//-----------------------------------------------------------------------------
void TF_Gamestats_KillStats_t::Reset()
{
//	Q_memset( &m_Summary, 0, sizeof( m_Summary ) );
//	m_flRoundStartTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
TFReportedStats_t::TFReportedStats_t()
{
	Clear();
	m_bValidData = false;
	m_pCurrentGame = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
TFReportedStats_t::~TFReportedStats_t()
{
	if ( m_pCurrentGame )
	{
		delete m_pCurrentGame;
		m_pCurrentGame = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clears data
//-----------------------------------------------------------------------------
void TFReportedStats_t::Clear()
{
	m_pCurrentGame = NULL;
	m_dictMapStats.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szMapName - 
// Output : TF_Gamestats_LevelStats_t
//-----------------------------------------------------------------------------
TF_Gamestats_LevelStats_t *TFReportedStats_t::FindOrAddMapStats( const char *szMapName )
{
	int iMap = m_dictMapStats.Find( szMapName );
	if( iMap == m_dictMapStats.InvalidIndex() )
	{
		iMap = m_dictMapStats.Insert( szMapName );
	}	

	return &m_dictMapStats[iMap];
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Saves data to buffer
//-----------------------------------------------------------------------------
void TFReportedStats_t::AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer )
{
	// save a version lump at beginning of file
	TF_Gamestats_Version_t versionLump;
	versionLump.m_iMagic = TF_GAMESTATS_MAGIC;
	versionLump.m_iVersion = TF_GAMESTATS_FILE_VERSION;
	CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_VERSION, 1, sizeof( versionLump ), &versionLump );

	// Save data per map.
	for ( int iMap =  m_dictMapStats.First(); iMap != m_dictMapStats.InvalidIndex(); iMap =  m_dictMapStats.Next( iMap ) )
	{
		// Get the current map.
		TF_Gamestats_LevelStats_t *pCurrentMap = &m_dictMapStats[iMap];
		Assert( pCurrentMap );

		// Write out the lumps.
		CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_MAPHEADER, 1, sizeof( TF_Gamestats_LevelStats_t::LevelHeader_t ), static_cast<void*>( &pCurrentMap->m_Header ) );
		//CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_MAPDEATH, pCurrentMap->m_aPlayerDeaths.Count(), sizeof( TF_Gamestats_LevelStats_t::PlayerDeathsLump_t ), static_cast<void*>( pCurrentMap->m_aPlayerDeaths.Base() ) );
		//CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_MAPDAMAGE, pCurrentMap->m_aPlayerDamage.Count(), sizeof( TF_Gamestats_LevelStats_t::PlayerDamageLump_t ), static_cast<void*>( pCurrentMap->m_aPlayerDamage.Base() ) );
		CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_CLASS, ARRAYSIZE( pCurrentMap->m_aClassStats ), sizeof( pCurrentMap->m_aClassStats[0] ), 
			static_cast<void*>( pCurrentMap->m_aClassStats ) );
		CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_WEAPON, ARRAYSIZE( pCurrentMap->m_aWeaponStats ), sizeof( pCurrentMap->m_aWeaponStats[0] ), 
			static_cast<void*>( pCurrentMap->m_aWeaponStats ) );
	}

	// Append an end tag to verify we've reached end of file and data was sane.  (Sometimes we receive stat files that start sane but become filled
	// with garbage partway through.)
	CBaseGameStats::AppendLump( MAX_LUMP_COUNT, SaveBuffer, TFSTATS_LUMP_ENDTAG, 1, sizeof( versionLump ), &versionLump );
}

//-----------------------------------------------------------------------------
// Purpose: Loads data from buffer
//-----------------------------------------------------------------------------
bool TFReportedStats_t::LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer )
{
	// read the version lump of beginning of file and verify version
	bool bGotEndTag = false;
	unsigned short iLump = 0;
	unsigned short iLumpCount = 0;
	if ( !CBaseGameStats::GetLumpHeader( MAX_LUMP_COUNT, LoadBuffer, iLump, iLumpCount ) )
		return false;
	if ( iLump != TFSTATS_LUMP_VERSION )
	{
		Msg( "Didn't find version header.  Expected lump type TFSTATS_LUMP_VERSION, got lump type %d.  Skipping file.\n", iLump );
		return false;
	}
	TF_Gamestats_Version_t versionLump;
	CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( versionLump ), &versionLump );
	if ( versionLump.m_iMagic != TF_GAMESTATS_MAGIC )
	{
		Msg( "Incorrect magic # in version header.  Expected %x, got %x.  Skipping file.\n", TF_GAMESTATS_MAGIC, versionLump.m_iMagic );
		return false;
	}
	if ( versionLump.m_iVersion != TF_GAMESTATS_FILE_VERSION )
	{
		Msg( "Mismatched file version.  Expected file version %d, got %d. Skipping file.\n", TF_GAMESTATS_FILE_VERSION, versionLump.m_iVersion  );
		return false;
	}

	TF_Gamestats_LevelStats_t *pCurrentGame = NULL;

	// read all the lumps in the file
	while( CBaseGameStats::GetLumpHeader( MAX_LUMP_COUNT, LoadBuffer, iLump, iLumpCount ) )
	{
		switch ( iLump )
		{
		case TFSTATS_LUMP_MAPHEADER: 
			{
				TF_Gamestats_LevelStats_t::LevelHeader_t header;
				CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( TF_Gamestats_LevelStats_t::LevelHeader_t ), &header );

				// quick sanity check on some data -- we get some stat files that start out OK but are corrupted later in the file
				if ( ( header.m_iRoundsPlayed < 0 ) || ( header.m_iTotalTime < 0 ) || ( header.m_iRoundsPlayed > 1000 ) )
					return false;

				// if there's no interesting data, skip this file.  (Need to have server not send it in this case.)
				if ( header.m_iTotalTime == 0 )
					return false;

				pCurrentGame = FindOrAddMapStats( header.m_szMapName );
				if ( pCurrentGame )
				{
					pCurrentGame->m_Header = header;
				}
				break; 
			}
		case TFSTATS_LUMP_MAPDEATH:
			{
				//CUtlVector<TF_Gamestats_LevelStats_t::PlayerDeathsLump_t> playerDeaths;

				//playerDeaths.SetCount( iLumpCount );
				//CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( TF_Gamestats_LevelStats_t::PlayerDeathsLump_t ), static_cast<void*>( playerDeaths.Base() ) );
				//if ( pCurrentGame )
				//{
				//	pCurrentGame->m_aPlayerDeaths = playerDeaths;
				//}
				break;
			}
		case TFSTATS_LUMP_MAPDAMAGE:
			{
				//CUtlVector<TF_Gamestats_LevelStats_t::PlayerDamageLump_t> playerDamage;

				//playerDamage.SetCount( iLumpCount );
				//CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( TF_Gamestats_LevelStats_t::PlayerDamageLump_t ), static_cast<void*>( playerDamage.Base() ) );
				//if ( pCurrentGame )
				//{
				//	pCurrentGame->m_aPlayerDamage = playerDamage;
				//}
				break;
			}		
		case TFSTATS_LUMP_CLASS:
			{
				Assert( pCurrentGame );
				if ( !pCurrentGame )
					return false;
				Assert ( iLumpCount == ARRAYSIZE( pCurrentGame->m_aClassStats ) );
				if ( iLumpCount == ARRAYSIZE( pCurrentGame->m_aClassStats ) )
				{
					CBaseGameStats::LoadLump( LoadBuffer, ARRAYSIZE( pCurrentGame->m_aClassStats ), sizeof( pCurrentGame->m_aClassStats[0] ), 
						pCurrentGame->m_aClassStats );

					// quick sanity check on some data -- we get some stat files that start out OK but are corrupted later in the file
					for ( int i = 0; i < ARRAYSIZE( pCurrentGame->m_aClassStats ); i++ )
					{
						TF_Gamestats_ClassStats_t &classStats = pCurrentGame->m_aClassStats[i];
						if ( ( classStats.iSpawns < 0 ) || ( classStats.iSpawns > 10000 ) || ( classStats.iTotalTime < 0 ) || ( classStats.iTotalTime > 36000 * 20 ) ||
							( classStats.iKills < 0 ) || ( classStats.iKills > 10000 ) )
						{
							return false;
						}
					}			
				}
				else
				{
					// mismatched lump size, possibly from different build, don't know how it interpret it, just skip over it
					return false;
				}				
				break;
			}
		case TFSTATS_LUMP_WEAPON:
			{
				Assert( pCurrentGame );
				if ( !pCurrentGame )
					return false;
				Assert ( iLumpCount == ARRAYSIZE( pCurrentGame->m_aWeaponStats ) );
				if ( iLumpCount == ARRAYSIZE( pCurrentGame->m_aWeaponStats ) )
				{
					CBaseGameStats::LoadLump( LoadBuffer, ARRAYSIZE( pCurrentGame->m_aWeaponStats ), sizeof( pCurrentGame->m_aWeaponStats[0] ), 
						pCurrentGame->m_aWeaponStats );

					// quick sanity check on some data -- we get some stat files that start out OK but are corrupted later in the file
					if ( ( pCurrentGame->m_aWeaponStats[TF_WEAPON_MEDIGUN].iShotsFired < 0 ) || ( pCurrentGame->m_aWeaponStats[TF_WEAPON_MEDIGUN].iShotsFired > 100000 )
						|| ( pCurrentGame->m_aWeaponStats[TF_WEAPON_FLAMETHROWER_ROCKET].iShotsFired != 0 ) ) // check that unused weapon has 0 shots
					{
						return false;
					}
					
				}				
				else
				{
					// mismatched lump size, possibly from different build, don't know how it interpret it, just skip over it
					return false;				
				}				
				break;
			}
		case TFSTATS_LUMP_ENDTAG:
			{
				// check that end tag is valid -- should be version lump again
				TF_Gamestats_Version_t versionLump;
				CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( versionLump ), &versionLump );
				if ( versionLump.m_iMagic != TF_GAMESTATS_MAGIC )
				{
					Msg( "Incorrect magic # in version header.  Expected %x, got %x.  Skipping file.\n", TF_GAMESTATS_MAGIC, versionLump.m_iMagic );
					return false;
				}
				if ( versionLump.m_iVersion != TF_GAMESTATS_FILE_VERSION )
				{
					Msg( "Mismatched file version.  Expected file version %d, got %d. Skipping file.\n", TF_GAMESTATS_FILE_VERSION, versionLump.m_iVersion  );
					return false;
				}
				bGotEndTag = true;
				break;
			}

		}
	}

	return bGotEndTag;
}
#endif

//-----------------------------------------------------------------------------
// TF2 Beta Maps
// Robot Destruction
//-----------------------------------------------------------------------------
RobotDestructionStats_t::RobotDestructionStats_t()
{
	Clear();
}

//-----------------------------------------------------------------------------
void RobotDestructionStats_t::Clear()
{
	V_memset( &iRobotInteraction, 0, sizeof( iRobotInteraction ) );
	V_memset( &iRobotCoreInteraction, 0, sizeof( iRobotCoreInteraction ) );
	V_memset( &iFlagInteraction, 0, sizeof( iFlagInteraction ) );

	V_memset( &iCoresCollectedByTeam, 0, sizeof( iCoresCollectedByTeam ) );
	V_memset( &iCoreCollectedByClass, 0, sizeof( iCoreCollectedByClass ) );

	V_memset( &iBlueRobotsKilledByType, 0, sizeof( iBlueRobotsKilledByType ) );
	V_memset( &iRedRobotsKilledByType, 0, sizeof( iRedRobotsKilledByType ) );
	V_memset( &iRobotsDamageFromClass, 0, sizeof( iRobotsDamageFromClass ) );
	
}

//-----------------------------------------------------------------------------
int	RobotDestructionStats_t::GetRobotInteractionCount()
{
	int iCount = 0;
	for ( int i = 1; i < MAX_PLAYERS; ++i )
	{
		if ( iRobotInteraction[i] )
		{
			iCount++;
		}
	}
	return iCount;
}
//-----------------------------------------------------------------------------
int	RobotDestructionStats_t::GetRobotCoreInteractionCount()
{
	int iCount = 0;
	for ( int i = 1; i < MAX_PLAYERS; ++i )
	{
		if ( iRobotCoreInteraction[i] )
		{
			iCount++;
		}
	}
	return iCount;
}
//-----------------------------------------------------------------------------
int	RobotDestructionStats_t::GetFlagInteractionCount()
{
	int iCount = 0;
	for ( int i = 1; i < MAX_PLAYERS; ++i )
	{
		if ( iFlagInteraction[i] )
		{
			iCount++;
		}
	}
	return iCount;
}

//-----------------------------------------------------------------------------
const char* g_aRoundEndReasons[] =
{
	"round_end",
	"client_disconnect",
	"client_quit",
	"server_map_change",
	"server_shutdown",
	"time_limit_reached",
	"win_limit_reached",
	"win_diff_limit_reached",
	"round_limit_reached",
	"next_level_cvar",
};

// Get a string describing the current game type.
const char* GetGameTypeID()
{
	ConVarRef tf_gamemode_arena( "tf_gamemode_arena" );
	ConVarRef tf_gamemode_cp( "tf_gamemode_cp" );
	ConVarRef tf_gamemode_ctf( "tf_gamemode_ctf" );
	ConVarRef tf_gamemode_sd( "tf_gamemode_sd" );
	ConVarRef tf_gamemode_payload( "tf_gamemode_payload" );
	ConVarRef tf_gamemode_mvm( "tf_gamemode_mvm" );
	ConVarRef tf_powerup_mode( "tf_powerup_mode" );
	ConVarRef tf_gamemode_passtime( "tf_gamemode_passtime" );

	const char* pszGameTypeID = NULL;
	if ( tf_gamemode_arena.GetBool() )
	{
		pszGameTypeID = "arena";
	}
	else if ( tf_gamemode_cp.GetBool() )
	{
		pszGameTypeID = "cp";
	}
	else if ( tf_gamemode_ctf.GetBool() )
	{
		if ( tf_powerup_mode.GetBool() )
		{
			pszGameTypeID = "ctf_mannpower";
		}
		else
		{
			pszGameTypeID = "ctf";
		}
	}
	else if ( tf_gamemode_sd.GetBool() )
	{
		pszGameTypeID = "sd";
	}
	else if ( tf_gamemode_payload.GetBool() )
	{
		pszGameTypeID = "payload";
	}
	else if ( tf_gamemode_mvm.GetBool() )
	{
		pszGameTypeID = "mvm";
	}
	else if ( tf_gamemode_passtime.GetBool() )
	{
		pszGameTypeID = "pass"; // intentionally not "passtime"
	}
	else
	{
		pszGameTypeID = "custom";
	}

	return pszGameTypeID;
}

//-----------------------------------------------------------------------------
// TF2 Beta Maps
// Passtime
//-----------------------------------------------------------------------------
void PasstimeStats_t::Clear()
{
	memset( &summary, 0, sizeof(summary) );
	memset( &classes, 0, sizeof(classes) );
}

//-----------------------------------------------------------------------------
void PasstimeStats_t::AddBallFracSample( float f )
{
	Assert( f >= 0 && f <= 1.0f );
	int iBin = (uint8) Floor2Int( f * 255 );
	summary.nBallFracHistSum += iBin;
	++summary.arrBallFracHist[ iBin ];
	++summary.nBallFracSampleCount;
}

//-----------------------------------------------------------------------------
void PasstimeStats_t::AddPassTravelDistSample( float f )
{
	if ( summary.nPassTravelDistSampleCount >= summary.k_nMaxPassTravelDistSamples )
		return;
	Assert( f >= 0 );
	summary.arrPassTravelDistSamples[ summary.nPassTravelDistSampleCount ] = (uint16) Float2Int( f );
	++summary.nPassTravelDistSampleCount;
}

#ifdef CLIENT_DLL
MapStats_t &GetMapStats( map_identifier_t iMapID )
{
	return CTFStatPanel::GetMapStats( iMapID );
}
#endif
