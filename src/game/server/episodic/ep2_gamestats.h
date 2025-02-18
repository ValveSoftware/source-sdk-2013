//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef EP2_GAMESTATS_H
#define EP2_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "ep1_gamestats.h"
#include "tier1/utlstring.h"

// EP2 Game Stats
enum Ep2GameStatsVersions_t
{
	EP2_GAMESTATS_FILE_VERSION_01 = 001,
	EP2_GAMESTATS_FILE_VERSION_02 = 002,

	EP2_GAMESTATS_CURRENT_VERSION = EP2_GAMESTATS_FILE_VERSION_02,
};

enum Ep2GameStatsLumpIds_t
{
	EP2STATS_LUMP_HEADER = 1,
	EP2STATS_LUMP_DEATH,
	EP2STATS_LUMP_NPC,
	EP2STATS_LUMP_WEAPON,
	EP2STATS_LUMP_SAVEGAMEINFO,
	EP2STATS_LUMP_TAG,
	EP2STATS_LUMP_GENERIC,
	EP2_MAX_LUMP_COUNT
};

// EP2 Game Level Stats Data
struct Ep2LevelStats_t
{
public:
	enum FloatCounterTypes_t
	{
		COUNTER_DAMAGETAKEN = 0,

		NUM_FLOATCOUNTER_TYPES,
	};

	enum IntCounterTypes_t
	{
		COUNTER_CRATESSMASHED = 0,
		COUNTER_OBJECTSPUNTED,
		COUNTER_VEHICULARHOMICIDES,
		COUNTER_DISTANCE_INVEHICLE,
		COUNTER_DISTANCE_ONFOOT,
		COUNTER_DISTANCE_ONFOOTSPRINTING,
		COUNTER_FALLINGDEATHS,
		COUNTER_VEHICLE_OVERTURNED,
		COUNTER_LOADGAME_STILLALIVE,
		COUNTER_LOADS,
		COUNTER_SAVES,
		COUNTER_GODMODES,
		COUNTER_NOCLIPS,

		NUM_INTCOUNTER_TYPES,
	};

	Ep2LevelStats_t() :
		m_bInitialized( false ),
		m_flLevelStartTime( 0.0f )
	{
		Q_memset( m_IntCounters, 0, sizeof( m_IntCounters ) );
		Q_memset( m_FloatCounters, 0, sizeof( m_FloatCounters ) );
	}
	~Ep2LevelStats_t()
	{
	}

	Ep2LevelStats_t( const Ep2LevelStats_t &other )
	{
		m_bInitialized		= other.m_bInitialized;
		m_flLevelStartTime	= other.m_flLevelStartTime;
		m_Header			= other.m_Header;
		m_aPlayerDeaths		= other.m_aPlayerDeaths;
		Q_memcpy( m_IntCounters, other.m_IntCounters, sizeof( m_IntCounters ) );
		Q_memcpy( m_FloatCounters, other.m_FloatCounters, sizeof( m_FloatCounters ) );
		int i;
		for ( i = other.m_dictEntityDeaths.First(); i != other.m_dictEntityDeaths.InvalidIndex(); i = other.m_dictEntityDeaths.Next( i ) )
		{
            m_dictEntityDeaths.Insert( other.m_dictEntityDeaths.GetElementName( i ), other.m_dictEntityDeaths[ i ] );
		}
		for ( i = other.m_dictWeapons.First(); i != other.m_dictWeapons.InvalidIndex(); i = other.m_dictWeapons.Next( i ) )
		{
			m_dictWeapons.Insert( other.m_dictWeapons.GetElementName( i ), other.m_dictWeapons[ i ] );
		}
		m_SaveGameInfo = other.m_SaveGameInfo;
	}

	// Create and destroy.
	void Init( const char *pszMapName, float flStartTime, char const *pchTag, int nMapVersion )
	{
		// Initialize.
		m_Header.m_iVersion = EP2_GAMESTATS_CURRENT_VERSION;
		Q_strncpy( m_Header.m_szMapName, pszMapName, sizeof( m_Header.m_szMapName ) );
		m_Header.m_flTime = 0.0f;

		// Start the level timer.
		m_flLevelStartTime = flStartTime;

		Q_strncpy( m_Tag.m_szTagText, pchTag, sizeof( m_Tag.m_szTagText ) );
		m_Tag.m_nMapVersion = nMapVersion;
	}

	void Shutdown( float flEndTime )
	{
		m_Header.m_flTime = flEndTime - m_flLevelStartTime;
	}

	void AppendToBuffer( CUtlBuffer &SaveBuffer )
	{
		// Always write out as current version
		m_Header.m_iVersion = EP2_GAMESTATS_CURRENT_VERSION;

		// Write out the lumps.
		CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_HEADER, 1, sizeof( Ep2LevelStats_t::LevelHeader_t ), static_cast<void*>( &m_Header ) );
		CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_TAG, 1, sizeof( Ep2LevelStats_t::Tag_t ), static_cast< void * >( &m_Tag ) );
		CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_DEATH, m_aPlayerDeaths.Count(), sizeof( Ep2LevelStats_t::PlayerDeathsLump_t ), static_cast<void*>( m_aPlayerDeaths.Base() ) );
		{
			CUtlBuffer buf;
			buf.Put( (const void *)m_IntCounters, sizeof( m_IntCounters ) );
			buf.Put( (const void *)m_FloatCounters, sizeof( m_FloatCounters ) );
			buf.PutInt( m_dictEntityDeaths.Count() );
			for ( int i = m_dictEntityDeaths.First(); i != m_dictEntityDeaths.InvalidIndex(); i = m_dictEntityDeaths.Next( i ) )
			{
				buf.PutString( m_dictEntityDeaths.GetElementName( i ) );
				buf.Put( (const void *)&m_dictEntityDeaths[ i ], sizeof( Ep2LevelStats_t::EntityDeathsLump_t ) );
			}
			CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_NPC, 1, buf.TellPut(), buf.Base() );
		}
		{
			CUtlBuffer buf;
			buf.PutInt( m_dictWeapons.Count() );
			for ( int i = m_dictWeapons.First(); i != m_dictWeapons.InvalidIndex(); i = m_dictWeapons.Next( i ) )
			{
				buf.PutString( m_dictWeapons.GetElementName( i ) );
				buf.Put( (const void *)&m_dictWeapons[ i ], sizeof( Ep2LevelStats_t::WeaponLump_t ) );
			}
			CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_WEAPON, 1, buf.TellPut(), buf.Base() );
		}
		{
			CUtlBuffer buf;
			buf.PutString( m_SaveGameInfo.m_sCurrentSaveFile.String() );
			buf.PutInt( m_SaveGameInfo.m_nCurrentSaveFileTime );
			buf.PutInt( m_SaveGameInfo.m_Records.Count() );
			for ( int i = 0 ; i < m_SaveGameInfo.m_Records.Count(); ++i )
			{
				buf.Put( (const void *)&m_SaveGameInfo.m_Records[ i ], sizeof( Ep2LevelStats_t::SaveGameInfoRecord2_t ) );
			}
			CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_SAVEGAMEINFO, 1, buf.TellPut(), buf.Base() );
		}
		{
			CUtlBuffer buf;
			buf.PutShort( Ep2LevelStats_t::GenericStatsLump_t::LumpVersion );
			buf.PutInt( m_dictGeneric.Count() );
			for ( int i = m_dictGeneric.First(); i != m_dictGeneric.InvalidIndex(); i = m_dictGeneric.Next( i ) )
			{
				buf.PutString( m_dictGeneric.GetElementName( i ) );
				buf.Put( (const void *)&m_dictGeneric[ i ], sizeof( Ep2LevelStats_t::GenericStatsLump_t ) );
			}
			CBaseGameStats::AppendLump( EP2_MAX_LUMP_COUNT, SaveBuffer, EP2STATS_LUMP_GENERIC, 1, buf.TellPut(), buf.Base() );
		}
	}

	static void LoadData( CUtlDict<Ep2LevelStats_t, unsigned short>& items, CUtlBuffer &LoadBuffer )
	{
		// Read the next lump.
		unsigned short iLump = 0;
		unsigned short iLumpCount = 0;

		Ep2LevelStats_t *pItem = NULL;
 
		while( CBaseGameStats::GetLumpHeader( EP2_MAX_LUMP_COUNT, LoadBuffer, iLump, iLumpCount, true ) )
		{
			switch ( iLump )
			{
			case EP2STATS_LUMP_HEADER: 
				{
					Ep2LevelStats_t::LevelHeader_t header;
					CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( Ep2LevelStats_t::LevelHeader_t ), &header );
					pItem = &items[ items.Insert( header.m_szMapName ) ];
					pItem->m_Header = header;
					pItem->m_Tag.Clear();
					Assert( pItem );
				}
				break; 
			case EP2STATS_LUMP_TAG:
				{
					Assert( pItem );
					CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( Ep2LevelStats_t::Tag_t ), &pItem->m_Tag );
				}
				break;
			case EP2STATS_LUMP_DEATH:
				{
					Assert( pItem );
					pItem->m_aPlayerDeaths.SetCount( iLumpCount );
					CBaseGameStats::LoadLump( LoadBuffer, iLumpCount, sizeof( Ep2LevelStats_t::PlayerDeathsLump_t ), static_cast<void*>( pItem->m_aPlayerDeaths.Base() ) );
				}
				break;
			case EP2STATS_LUMP_NPC:
				{
					Assert( pItem );
					LoadBuffer.Get( ( void * )pItem->m_IntCounters, sizeof( pItem->m_IntCounters ) );
					LoadBuffer.Get( ( void * )pItem->m_FloatCounters, sizeof( pItem->m_FloatCounters ) );
					int c = LoadBuffer.GetInt();
					for ( int i = 0 ; i < c; ++i )
					{
						Ep2LevelStats_t::EntityDeathsLump_t data;
						char npcName[ 512 ];
						LoadBuffer.GetString( npcName );
						LoadBuffer.Get( &data, sizeof( data ) );
						pItem->m_dictEntityDeaths.Insert( npcName, data );
					}
				}
				break;
			case EP2STATS_LUMP_WEAPON:
				{
					Assert( pItem );
					int c = LoadBuffer.GetInt();
					for ( int i = 0 ; i < c; ++i )
					{
						Ep2LevelStats_t::WeaponLump_t data;
						char weaponName[ 512 ];
						LoadBuffer.GetString( weaponName );
						LoadBuffer.Get( &data, sizeof( data ) );
						pItem->m_dictWeapons.Insert( weaponName, data );
					}
				}
				break;
			case EP2STATS_LUMP_SAVEGAMEINFO:
				{
					Assert( pItem );
					Ep2LevelStats_t::SaveGameInfo_t *info = &pItem->m_SaveGameInfo;
					char sz[ 512 ];
					LoadBuffer.GetString( sz );
					info->m_sCurrentSaveFile = sz;
					info->m_nCurrentSaveFileTime = LoadBuffer.GetInt();
					int c = LoadBuffer.GetInt();
					for ( int i = 0 ; i < c; ++i )
					{
						Ep2LevelStats_t::SaveGameInfoRecord2_t rec;
						if ( pItem->m_Header.m_iVersion >= EP2_GAMESTATS_FILE_VERSION_02 )
						{
							LoadBuffer.Get( &rec, sizeof( rec ) );
						}
						else
						{
							size_t s = sizeof( Ep2LevelStats_t::SaveGameInfoRecord_t );
							LoadBuffer.Get( &rec, s );
						}
						info->m_Records.AddToTail( rec );
					}
					info->m_pCurrentRecord = NULL;
					if ( info->m_Records.Count() > 0 )
					{
						info->m_pCurrentRecord = &info->m_Records[ info->m_Records.Count() - 1 ];
					}
				}
				break;
			case EP2STATS_LUMP_GENERIC:
				{
					Assert( pItem );
					int version = LoadBuffer.GetShort();
					if ( version == Ep2LevelStats_t::GenericStatsLump_t::LumpVersion )
					{
						int c = LoadBuffer.GetInt();
						Assert( c < 2 * 1024 * 1024 );
						for ( int i = 0 ; i < c; ++i )
						{
							Ep2LevelStats_t::GenericStatsLump_t data;
							char pchStatName[ 512 ];
							LoadBuffer.GetString( pchStatName );
							LoadBuffer.Get( &data, sizeof( data ) );
							pItem->m_dictGeneric.Insert( pchStatName, data );
						}
					}
					else
					{
						Error( "Unsupported GenericStatsLump_t::LumpVersion" );
					}
				}
				break;
			}
		}
	}

public:
	// Level header data.
	struct LevelHeader_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_HEADER;	// Lump ids.
		byte			m_iVersion;									// Version of the game stats file.
		char			m_szMapName[64];							// Name of the map.
		float			m_flTime;									// Time spent in level.
	};

	// Simple "tag" applied to all data in database (e.g., "PLAYTEST")
	struct Tag_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_TAG;	

		void Clear()
		{
			Q_memset( m_szTagText, 0, sizeof( m_szTagText ) );
			m_nMapVersion = 0;
		}

		char			m_szTagText[ 8 ];
		int				m_nMapVersion;
	};

	// Player deaths.
	struct PlayerDeathsLump_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_DEATH;	// Lump ids.
		short	nPosition[3];										// Position of death.
//		short	iWeapon;											// Weapon that killed the player.
//		byte	iAttackClass;										// Class that killed the player.
//		byte	iTargetClass;										// Class of the player killed.
	};

	struct EntityDeathsLump_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_NPC;

		EntityDeathsLump_t() :
			m_nBodyCount( 0u ),
			m_nKilledPlayer( 0u )
		{
		}

		EntityDeathsLump_t( const EntityDeathsLump_t &other )
		{
			m_nBodyCount = other.m_nBodyCount;
			m_nKilledPlayer = other.m_nKilledPlayer;
		}

		unsigned int		m_nBodyCount;		// Number killed by player
		unsigned int		m_nKilledPlayer;	// Number of times entity killed player
	};

	struct WeaponLump_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_WEAPON;

		WeaponLump_t() :
			m_nShots( 0 ),
			m_nHits( 0 ),
			m_flDamageInflicted( 0.0 )
		{
		}

		WeaponLump_t( const WeaponLump_t &other )
		{
			m_nShots = other.m_nShots;
			m_nHits = other.m_nHits;
			m_flDamageInflicted = other.m_flDamageInflicted;
		}

		unsigned int		m_nShots;
		unsigned int		m_nHits;
		double				m_flDamageInflicted;
	};

	struct SaveGameInfoRecord_t
	{
		SaveGameInfoRecord_t() :
			m_nFirstDeathIndex( -1 ),
			m_nNumDeaths( 0 ),
			m_nSaveHealth( -1 )
		{
			Q_memset( m_nSavePos, 0, sizeof( m_nSavePos ) );
		}

		int		m_nFirstDeathIndex;
		int		m_nNumDeaths;
		// Health and player pos from the save file
		short	m_nSavePos[ 3 ];
		short	m_nSaveHealth;
	};

#pragma pack( 1 )
	// Adds save game type
	struct SaveGameInfoRecord2_t : public SaveGameInfoRecord_t
	{
		enum SaveType_t
		{
			TYPE_UNKNOWN = 0,
			TYPE_AUTOSAVE,
			TYPE_USERSAVE
		};

		SaveGameInfoRecord2_t() : 
			m_SaveType( (byte)TYPE_UNKNOWN )
		{
		}

		byte m_SaveType;
	};
#pragma pack()

	struct SaveGameInfo_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_SAVEGAMEINFO;

		SaveGameInfo_t() :
			m_nCurrentSaveFileTime( 0 ),
			m_pCurrentRecord( NULL )
		{
		}

		void Latch( char const *pchSaveName, unsigned int uFileTime )
		{
			m_pCurrentRecord = &m_Records[ m_Records.AddToTail() ];
			m_nCurrentSaveFileTime = uFileTime;
			m_sCurrentSaveFile = pchSaveName;
		}

		CUtlVector< SaveGameInfoRecord2_t > m_Records;
		SaveGameInfoRecord2_t				*m_pCurrentRecord;
		unsigned int						m_nCurrentSaveFileTime;
		CUtlString							m_sCurrentSaveFile;
	};

	struct GenericStatsLump_t
	{
		static const unsigned short LumpId = EP2STATS_LUMP_GENERIC;
		static const unsigned short LumpVersion = 1;

		GenericStatsLump_t() : 
			m_unCount( 0u ),
			m_flCurrentValue( 0.0 ) 
		{ 
			m_Pos[ 0 ] = m_Pos[ 1 ] = m_Pos[ 2 ] = 0; 
		}

		short						m_Pos[ 3 ];
		unsigned int				m_unCount;
		double						m_flCurrentValue;
	};

	// Data.
	LevelHeader_t					m_Header;			// Level header.
	Tag_t							m_Tag;
	CUtlVector<PlayerDeathsLump_t>	m_aPlayerDeaths;	// List of player deaths.
	CUtlDict< EntityDeathsLump_t, int >		m_dictEntityDeaths;
	CUtlDict< WeaponLump_t, int >	m_dictWeapons;
	CUtlDict< GenericStatsLump_t, int > m_dictGeneric;

	SaveGameInfo_t					m_SaveGameInfo;
	float							m_FloatCounters[ NUM_FLOATCOUNTER_TYPES ];
	uint64							m_IntCounters[ NUM_INTCOUNTER_TYPES ];

	// Temporary data.
	bool							m_bInitialized;		// Has the map Map Stat Data been initialized.
	float							m_flLevelStartTime;
};

#if defined( GAME_DLL )
class CEP2GameStats : public CEP1GameStats
{
	typedef CEP1GameStats BaseClass;

public:
	CEP2GameStats();
	virtual ~CEP2GameStats();

	virtual CBaseGameStats *OnInit( CBaseGameStats *pCurrentGameStats, char const *gamedir ) { return pCurrentGameStats; }

	virtual bool UserPlayedAllTheMaps( void );
	virtual const char *GetStatSaveFileName( void );
	virtual const char *GetStatUploadRegistryKeyName( void );

	// Buffers.
	virtual void AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer );
	virtual void LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer );

	// Events
	virtual void Event_LevelInit( void );
	virtual void Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info );
	virtual void Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info );
	virtual void Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual void Event_CrateSmashed();
	virtual void Event_Punted( CBaseEntity *pObject );
	virtual void Event_PlayerTraveled( CBasePlayer *pBasePlayer, float distanceInInches, bool bInVehicle, bool bSprinting );
	virtual void Event_WeaponFired( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName );
	virtual void Event_WeaponHit( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName, const CTakeDamageInfo &info );
	virtual void Event_SaveGame( void );
	virtual void Event_LoadGame( void );
	virtual void Event_FlippedVehicle( CBasePlayer *pDriver, CPropVehicleDriveable *pVehicle );
	// Called before .sav file is actually loaded (player should still be in previous level, if any)
	virtual void Event_PreSaveGameLoaded( char const *pSaveName, bool bInGame );
	virtual void Event_PlayerEnteredGodMode( CBasePlayer *pBasePlayer );
	virtual void Event_PlayerEnteredNoClip( CBasePlayer *pBasePlayer );
	virtual void Event_DecrementPlayerEnteredNoClip( CBasePlayer *pBasePlayer );
	// Generic statistics lump
	virtual void Event_IncrementCountedStatistic( const Vector& vecAbsOrigin, char const *pchStatisticName, float flIncrementAmount );

public:	//FIXME: temporary used for CC_ListDeaths command
	Ep2LevelStats_t	*FindOrAddMapStats( const char *szMapName );
	
public:

	Ep2LevelStats_t::EntityDeathsLump_t *FindDeathsLump( char const *npcName );
	Ep2LevelStats_t::WeaponLump_t *FindWeaponsLump( char const *pchWeaponName, bool bPrimary );
	Ep2LevelStats_t::GenericStatsLump_t *FindGenericLump( char const *pchStatName );
	// Utilities.
	Ep2LevelStats_t	*GetCurrentMap( void )			{ return m_pCurrentMap; }

	Ep2LevelStats_t									*m_pCurrentMap;
	CUtlDict<Ep2LevelStats_t, unsigned short>		m_dictMapStats;
	enum
	{
		INVEHICLE = 0,
		ONFOOT,
		ONFOOTSPRINTING,

		NUM_TRAVEL_TYPES
	};
	float											m_flInchesRemainder[ NUM_TRAVEL_TYPES ];
};
#endif

#endif // EP2_GAMESTATS_H
