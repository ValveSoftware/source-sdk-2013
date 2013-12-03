//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#if defined( GAME_DLL )
#include "cbase.h"
#endif
#include "ep2_gamestats.h"
#include "tier1/utlbuffer.h"
#include "vehicle_base.h"
#include "tier1/utlstring.h"
#include "filesystem.h"
#include "icommandline.h"

static CEP2GameStats s_CEP2GameStats_Singleton;
CBaseGameStats *g_pEP2GameStats = &s_CEP2GameStats_Singleton;


CEP2GameStats::CEP2GameStats( void )
{
	Q_memset( m_flInchesRemainder, 0, sizeof( m_flInchesRemainder ) );
	m_pCurrentMap = NULL;
	m_dictMapStats.Purge();
}

const char *CEP2GameStats::GetStatSaveFileName( void )
{
	//overriding the default for backwards compatibility with release stat tracking code
	return "ep2_gamestats.dat"; 
}

const char *CEP2GameStats::GetStatUploadRegistryKeyName( void )
{
	//overriding the default for backwards compatibility with release stat tracking code
	return "GameStatsUpload_Ep2"; 
}


static char const *ep2Maps[] =
{
	"ep2_outland_01",
	"ep2_outland_02",
	"ep2_outland_03",
	"ep2_outland_04",
	"ep2_outland_05",
	"ep2_outland_06",
	"ep2_outland_06a", 
	"ep2_outland_07", 
	"ep2_outland_08", 
	"ep2_outland_09",
	"ep2_outland_10",
	"ep2_outland_10a",
	"ep2_outland_11", 
	"ep2_outland_11a",
	"ep2_outland_12",
	"ep2_outland_12a"
};


bool CEP2GameStats::UserPlayedAllTheMaps( void )
{
	int c = ARRAYSIZE( ep2Maps );
	for ( int i = 0; i < c; ++i )
	{
		int idx = m_BasicStats.m_MapTotals.Find( ep2Maps[ i ] );
		if( idx == m_BasicStats.m_MapTotals.InvalidIndex() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CEP2GameStats::~CEP2GameStats()
{
	m_pCurrentMap = NULL;
	m_dictMapStats.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &SaveBuffer - 
//-----------------------------------------------------------------------------
void CEP2GameStats::AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer )
{
	// Save data per map.
	for ( int iMap =  m_dictMapStats.First(); iMap != m_dictMapStats.InvalidIndex(); iMap =  m_dictMapStats.Next( iMap ) )
	{
		// Get the current map.
		Ep2LevelStats_t *pCurrentMap = &m_dictMapStats[iMap];
		Assert( pCurrentMap );
		pCurrentMap->AppendToBuffer( SaveBuffer );
	}
}

void CEP2GameStats::LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer )
{
	Ep2LevelStats_t::LoadData( m_dictMapStats, LoadBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEP2GameStats::Event_LevelInit( void )
{
	BaseClass::Event_LevelInit();

	char const *pchTag = NULL;
	CommandLine()->CheckParm( "-gamestatstag", &pchTag );
	if ( !pchTag )
	{
		pchTag = "";
	}

	m_pCurrentMap = FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	m_pCurrentMap->Init( STRING( gpGlobals->mapname ), gpGlobals->curtime, pchTag, gpGlobals->mapversion );
}

Ep2LevelStats_t::EntityDeathsLump_t *CEP2GameStats::FindDeathsLump( char const *npcName )
{
	if ( !m_pCurrentMap )
		return NULL;

	char const *name = npcName;
	// Hack to fixup name
	if ( !Q_stricmp( name, "npc_ministrider" ) )
	{
		name = "npc_hunter";
	}

	if ( Q_strnicmp( name, "npc_", 4 ) )
		return NULL;

	int idx = m_pCurrentMap->m_dictEntityDeaths.Find( name );
	if ( idx == m_pCurrentMap->m_dictEntityDeaths.InvalidIndex() )
	{
		idx = m_pCurrentMap->m_dictEntityDeaths.Insert( name );
	}

	return &m_pCurrentMap->m_dictEntityDeaths[ idx ];
}

Ep2LevelStats_t::WeaponLump_t *CEP2GameStats::FindWeaponsLump( char const *pchWeaponName, bool bPrimary )
{
	if ( !m_pCurrentMap )
		return NULL;

	if ( !pchWeaponName )
	{
		AssertOnce( !"FindWeaponsLump pchWeaponName == NULL" );
		return NULL;
	}

	char lookup[ 512 ];
	Q_snprintf( lookup, sizeof( lookup ), "%s_%s", pchWeaponName, bPrimary ? "primary" : "secondary" );
	int idx = m_pCurrentMap->m_dictWeapons.Find( lookup );
	if ( idx == m_pCurrentMap->m_dictWeapons.InvalidIndex() )
	{
		idx = m_pCurrentMap->m_dictWeapons.Insert( lookup );
	}

	return &m_pCurrentMap->m_dictWeapons[ idx ];
}

// Finds the generic stats lump
Ep2LevelStats_t::GenericStatsLump_t *CEP2GameStats::FindGenericLump( char const *pchStatName )
{
	if ( !m_pCurrentMap )
		return NULL;
	if ( !pchStatName || !*pchStatName )
		return NULL;

	int idx = m_pCurrentMap->m_dictGeneric.Find( pchStatName );
	if ( idx == m_pCurrentMap->m_dictGeneric.InvalidIndex() )
	{
		idx = m_pCurrentMap->m_dictGeneric.Insert( pchStatName );
	}

	return &m_pCurrentMap->m_dictGeneric[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szMapName - 
// Output : Ep2LevelStats_t
//-----------------------------------------------------------------------------
Ep2LevelStats_t *CEP2GameStats::FindOrAddMapStats( const char *szMapName )
{
	int iMap = m_dictMapStats.Find( szMapName );
	if( iMap == m_dictMapStats.InvalidIndex() )
	{
		iMap = m_dictMapStats.Insert( szMapName );
	}	

	return &m_dictMapStats[iMap];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEP2GameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info )
{
	BaseClass::Event_PlayerDamage( pBasePlayer, info );

	m_pCurrentMap->m_FloatCounters[ Ep2LevelStats_t::COUNTER_DAMAGETAKEN ] += info.GetDamage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEP2GameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_PlayerKilledOther( pAttacker, pVictim, info );

	if ( pAttacker )
	{
		StatsLog( "Attacker: %s\n", pAttacker->GetClassname() );
	}
	
	if ( !pVictim  )
	{
		return;
	}

	char const *pchVictim = pVictim->GetClassname();
	Ep2LevelStats_t::EntityDeathsLump_t *lump = FindDeathsLump( pchVictim );
	if ( lump )
	{
		++lump->m_nBodyCount;
		StatsLog( "Player has killed %d %s's\n", lump->m_nBodyCount, pchVictim );

		CPropVehicleDriveable *veh = dynamic_cast< CPropVehicleDriveable * >( pAttacker );
		if ( !veh )
			veh = dynamic_cast< CPropVehicleDriveable * >( info.GetInflictor() );
		if ( veh )
		{
			CBaseEntity *driver = veh->GetDriver();
			if ( driver && driver->IsPlayer() )
			{
				++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_VEHICULARHOMICIDES ];
				StatsLog( "  Vehicular homicide [%I64d] of %s's\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_VEHICULARHOMICIDES ], pchVictim );
			}
		}
	}
	else
	{
		StatsLog( "Player killed %s (not tracked)\n", pchVictim );
	}
}

void CEP2GameStats::Event_Punted( CBaseEntity *pObject )
{
	BaseClass::Event_Punted( pObject );
	++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_OBJECTSPUNTED ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEP2GameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	BaseClass::Event_PlayerKilled( pPlayer, info );

	if ( info.GetDamageType() & DMG_FALL )
	{
		++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_FALLINGDEATHS ];
	}

	Ep2LevelStats_t::PlayerDeathsLump_t death;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	death.nPosition[ 0 ] = static_cast<short>( org.x );
	death.nPosition[ 1 ] = static_cast<short>( org.y );
	death.nPosition[ 2 ] = static_cast<short>( org.z );

	StatsLog( "CEP2GameStats::Event_PlayerKilled at location [%d %d %d]\n", (int)death.nPosition[ 0 ], (int)death.nPosition[ 1 ], (int)death.nPosition[ 2 ] );

	// set the class of the attacker
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();

	if ( pInflictor )
	{
		StatsLog( "Inflictor: %s\n", pInflictor->GetClassname() );
	}

	if ( pKiller )
	{
		char const *pchKiller = pKiller->GetClassname();
		Ep2LevelStats_t::EntityDeathsLump_t *lump = FindDeathsLump( pchKiller );
		if ( lump )
		{
			++lump->m_nKilledPlayer;
			StatsLog( "Player has been killed %d times by %s's\n", lump->m_nKilledPlayer, pchKiller );
		}
		else
		{
			StatsLog( "Player killed by %s (not tracked)\n", pchKiller );
		}
	}

	// add it to the list of deaths
	Ep2LevelStats_t *map = FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	int slot = map->m_aPlayerDeaths.AddToTail( death );

	Ep2LevelStats_t::SaveGameInfoRecord2_t *rec = map->m_SaveGameInfo.m_pCurrentRecord;
	if ( rec )
	{
		if ( rec->m_nFirstDeathIndex == -1 )
		{
			rec->m_nFirstDeathIndex = slot;
		}
		++rec->m_nNumDeaths;

		StatsLog( "Player has died %d times since last save/load\n", rec->m_nNumDeaths );
	}
}

void CEP2GameStats::Event_CrateSmashed()
{
	BaseClass::Event_CrateSmashed();

	++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_CRATESSMASHED ];
}

void CEP2GameStats::Event_PlayerTraveled( CBasePlayer *pBasePlayer, float distanceInInches, bool bInVehicle, bool bSprinting )
{
	BaseClass::Event_PlayerTraveled( pBasePlayer, distanceInInches, bInVehicle, bSprinting );

	int iIndex = INVEHICLE;
	if ( !bInVehicle )
	{
		iIndex = bSprinting ? ONFOOTSPRINTING : ONFOOT;
	}

	m_flInchesRemainder[ iIndex ] += distanceInInches;
	uint64 intPart = (uint64)m_flInchesRemainder[ iIndex ];
	m_flInchesRemainder[ iIndex ] -= intPart;
	if ( intPart > 0 )
	{
		if ( bInVehicle )
		{
			m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_DISTANCE_INVEHICLE ] += intPart;
		}
		else
		{
			if ( bSprinting )
			{
				m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_DISTANCE_ONFOOTSPRINTING ] += intPart;
			}
			else
			{
				m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_DISTANCE_ONFOOT ] += intPart;
			}
		}
	}

	Ep2LevelStats_t *map = m_pCurrentMap;
	if ( !map )
		return;

	Ep2LevelStats_t::SaveGameInfoRecord2_t *rec = map->m_SaveGameInfo.m_pCurrentRecord;

	if ( rec && 
		rec->m_nSaveHealth == -1 )
	{
		Vector pos = pBasePlayer->GetAbsOrigin();
		rec->m_nSavePos[ 0 ] = (short)pos.x;
		rec->m_nSavePos[ 1 ] = (short)pos.y;
		rec->m_nSavePos[ 2 ] = (short)pos.z;
		rec->m_nSaveHealth = clamp( pBasePlayer->GetHealth(), 0, 100 );
	}
}

void CEP2GameStats::Event_WeaponFired( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName )
{
	BaseClass::Event_WeaponFired( pShooter, bPrimary, pchWeaponName );

	Ep2LevelStats_t::WeaponLump_t *lump = FindWeaponsLump( pchWeaponName, bPrimary );
	if ( lump )
	{
		++lump->m_nShots;
	}
}

void CEP2GameStats::Event_WeaponHit( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName, const CTakeDamageInfo &info )
{
	BaseClass::Event_WeaponHit( pShooter, bPrimary, pchWeaponName, info );
	Ep2LevelStats_t::WeaponLump_t *lump = FindWeaponsLump( pchWeaponName, bPrimary );
	if ( lump )
	{
		++lump->m_nHits;
		lump->m_flDamageInflicted += info.GetDamage();
	}
}

void CEP2GameStats::Event_SaveGame( void )
{
	BaseClass::Event_SaveGame();

	Ep2LevelStats_t *map = m_pCurrentMap;
	if ( !map )
		return;

	++map->m_IntCounters[ Ep2LevelStats_t::COUNTER_SAVES ];
	StatsLog( " %I64uth save on this map\n", map->m_IntCounters[ Ep2LevelStats_t::COUNTER_SAVES ] );

	char const *pchSaveFile = engine->GetSaveFileName();
	if ( !pchSaveFile || !pchSaveFile[ 0 ] )
		return;

	char name[ 512 ];
	Q_strncpy( name, pchSaveFile, sizeof( name ) );
	Q_strlower( name );
	Q_FixSlashes( name );

	unsigned int uFileTime = filesystem->GetFileTime( name, "GAME" );
	// Latch off previous
	map->m_SaveGameInfo.Latch( name, uFileTime );

	Ep2LevelStats_t::SaveGameInfoRecord2_t *rec = map->m_SaveGameInfo.m_pCurrentRecord;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( pPlayer )
	{
		Vector pos = pPlayer->GetAbsOrigin();
		rec->m_nSavePos[ 0 ] = (short)pos.x;
		rec->m_nSavePos[ 1 ] = (short)pos.y;
		rec->m_nSavePos[ 2 ] = (short)pos.z;
		rec->m_nSaveHealth = clamp( pPlayer->GetHealth(), 0, 100 );
		rec->m_SaveType = Q_stristr( pchSaveFile, "autosave" ) ? 
			Ep2LevelStats_t::SaveGameInfoRecord2_t::TYPE_AUTOSAVE : Ep2LevelStats_t::SaveGameInfoRecord2_t::TYPE_USERSAVE;

		StatsLog( "save pos %i %i %i w/ health %d\n",
			rec->m_nSavePos[ 0 ],
			rec->m_nSavePos[ 1 ],
			rec->m_nSavePos[ 2 ],
			rec->m_nSaveHealth );

	}
}

void CEP2GameStats::Event_LoadGame( void )
{
	BaseClass::Event_LoadGame();

	Ep2LevelStats_t *map = m_pCurrentMap;
	if ( !map )
		return;

	++map->m_IntCounters[ Ep2LevelStats_t::COUNTER_LOADS ];
	StatsLog( " %I64uth load on this map\n", map->m_IntCounters[ Ep2LevelStats_t::COUNTER_LOADS ] );

	char const *pchSaveFile = engine->GetMostRecentlyLoadedFileName();
	if ( !pchSaveFile || !pchSaveFile[ 0 ] )
		return;

	char name[ 512 ];
	Q_snprintf( name, sizeof( name ), "save/%s", pchSaveFile );
	Q_DefaultExtension( name, IsX360() ? ".360.sav" : ".sav", sizeof( name ) );
	Q_FixSlashes( name );
	Q_strlower( name );

	Ep2LevelStats_t::SaveGameInfo_t *pSaveGameInfo = &map->m_SaveGameInfo;

	if ( pSaveGameInfo->m_nCurrentSaveFileTime == 0 || 
		pSaveGameInfo->m_sCurrentSaveFile != name )
	{
		unsigned int uFileTime = filesystem->GetFileTime( name, "GAME" );

		// Latch off previous
		StatsLog( "Relatching save game file due to time or filename change (%s : %u)\n", name, uFileTime );
		pSaveGameInfo->Latch( name, uFileTime );
	}
}

void CEP2GameStats::Event_FlippedVehicle( CBasePlayer *pDriver, CPropVehicleDriveable *pVehicle )
{
	BaseClass::Event_FlippedVehicle( pDriver, pVehicle );
	++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_VEHICLE_OVERTURNED ];
	StatsLog( "%I64u time vehicle overturned\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_VEHICLE_OVERTURNED ] );
}

void CEP2GameStats::Event_PreSaveGameLoaded( char const *pSaveName, bool bInGame )
{
	BaseClass::Event_PreSaveGameLoaded( pSaveName, bInGame );

	// Not currently in a level
	if ( !bInGame )
		return;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( !pPlayer )
		return;

	// We're loading a saved game while the player is still alive (are they stuck?)
	if ( pPlayer->IsAlive() )
	{
		++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_LOADGAME_STILLALIVE ];
		StatsLog( "%I64u game loaded with living player\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_LOADGAME_STILLALIVE ] );
	}
}

void CEP2GameStats::Event_PlayerEnteredGodMode( CBasePlayer *pBasePlayer )
{
	BaseClass::Event_PlayerEnteredGodMode( pBasePlayer );
	++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_GODMODES ];
	StatsLog( "%I64u time entering godmode\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_GODMODES ] );
}

void CEP2GameStats::Event_PlayerEnteredNoClip( CBasePlayer *pBasePlayer )
{
	BaseClass::Event_PlayerEnteredNoClip( pBasePlayer );
	++m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_NOCLIPS ];
	StatsLog( "%I64u time entering NOCLIP\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_NOCLIPS ] );
}

void CEP2GameStats::Event_DecrementPlayerEnteredNoClip( CBasePlayer *pBasePlayer )
{
	BaseClass::Event_DecrementPlayerEnteredNoClip( pBasePlayer );
	if ( m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_NOCLIPS ] > 0 )
	{
		--m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_NOCLIPS ];
	}
	StatsLog( "%I64u decrement entering NOCLIP (entering vehicle doesn't count)\n", m_pCurrentMap->m_IntCounters[ Ep2LevelStats_t::COUNTER_NOCLIPS ] );
}

// Generic statistics lump
void CEP2GameStats::Event_IncrementCountedStatistic( const Vector& vecAbsOrigin, char const *pchStatisticName, float flIncrementAmount )
{
	BaseClass::Event_IncrementCountedStatistic( vecAbsOrigin, pchStatisticName, flIncrementAmount );

	// Find the generic lump
	Ep2LevelStats_t::GenericStatsLump_t *lump = FindGenericLump( pchStatisticName );
	if ( lump )
	{
		lump->m_Pos[ 0 ] = (short)vecAbsOrigin.x;
		lump->m_Pos[ 1 ] = (short)vecAbsOrigin.y;
		lump->m_Pos[ 2 ] = (short)vecAbsOrigin.z;
		lump->m_flCurrentValue += (double)flIncrementAmount;
		++lump->m_unCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void CC_ListDeaths( const CCommand &args )
{
	Ep2LevelStats_t *map = s_CEP2GameStats_Singleton.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	if ( !map )
		return;

	int nRendered = 0;
	for ( int i = map->m_aPlayerDeaths.Count() - 1; i >= 0 ; --i, ++nRendered )
	{
		Vector org( map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 2 ] + 36.0f );

		// FIXME: This might overflow
		NDebugOverlay::Box( org, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), 0, 255, 0, 128, 10.0f );

		/*
		Msg( "%s killed %s with %s at (%d,%d,%d)\n",
			g_aClassNames[ map->m_aPlayerDeaths[ i ].iAttackClass ],
			g_aClassNames[ map->m_aPlayerDeaths[ i ].iTargetClass ],
			WeaponIdToAlias( map->m_aPlayerDeaths[ i ].iWeapon ), 
			map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 2 ] );
		*/

		if ( nRendered > 150 )
			break;
	}
	Msg( "\nlisted %d deaths\n", map->m_aPlayerDeaths.Count() );
}

static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", 0 );