//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_GAMESTATS_SHARED_H
#define TF_GAMESTATS_SHARED_H
#ifdef _WIN32
#pragma once
#endif
#include "cbase.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "shareddefs.h"
#include "tf_shareddefs.h"

//=============================================================================
//
// TF Game Stats Enums
//
// NOTE: You may add to the end, but do not insert to this list!
//
enum TFStatType_t
{
	TFSTAT_UNDEFINED = 0,
	TFSTAT_SHOTS_HIT,
	TFSTAT_SHOTS_FIRED,
	TFSTAT_KILLS,
	TFSTAT_DEATHS,
	TFSTAT_DAMAGE,
	TFSTAT_CAPTURES,
	TFSTAT_DEFENSES,
	TFSTAT_DOMINATIONS,
	TFSTAT_REVENGE,
	TFSTAT_POINTSSCORED,
	TFSTAT_BUILDINGSDESTROYED,
	TFSTAT_HEADSHOTS,
	TFSTAT_PLAYTIME,
	TFSTAT_HEALING,
	TFSTAT_INVULNS,
	TFSTAT_KILLASSISTS,
	TFSTAT_BACKSTABS,
	TFSTAT_HEALTHLEACHED,
	TFSTAT_BUILDINGSBUILT,
	TFSTAT_MAXSENTRYKILLS,
	TFSTAT_TELEPORTS,
	TFSTAT_FIREDAMAGE,
	TFSTAT_BONUS_POINTS,
	TFSTAT_BLASTDAMAGE,
	TFSTAT_DAMAGETAKEN,
	TFSTAT_HEALTHKITS,
	TFSTAT_AMMOKITS,
	TFSTAT_CLASSCHANGES,
	TFSTAT_CRITS,
	TFSTAT_SUICIDES,
	TFSTAT_CURRENCY_COLLECTED,
	TFSTAT_DAMAGE_ASSIST,
	TFSTAT_HEALING_ASSIST,
	TFSTAT_DAMAGE_BOSS,
	TFSTAT_DAMAGE_BLOCKED,
	TFSTAT_DAMAGE_RANGED,
	TFSTAT_DAMAGE_RANGED_CRIT_RANDOM,
	TFSTAT_DAMAGE_RANGED_CRIT_BOOSTED,
	TFSTAT_REVIVED,
	TFSTAT_THROWABLEHIT,
	TFSTAT_THROWABLEKILL,
	TFSTAT_KILLSTREAK_MAX,
	TFSTAT_KILLS_RUNECARRIER,
	TFSTAT_FLAGRETURNS,
	TFSTAT_TOTAL
};

#define TFSTAT_FIRST (TFSTAT_UNDEFINED+1)
#define TFSTAT_LAST (TFSTAT_TOTAL-1)

extern const char *s_pStatStrings[ TFSTAT_TOTAL ];

enum TFMapStatType_t
{
	TFMAPSTAT_UNDEFINED = 0,
	TFMAPSTAT_PLAYTIME,
	TFMAPSTAT_TOTAL
};

#define TFMAPSTAT_FIRST (TFMAPSTAT_UNDEFINED+1)
#define TFMAPSTAT_LAST (TFMAPSTAT_TOTAL-1)

extern const char *s_pMapStatStrings[ TFMAPSTAT_TOTAL ];

enum TFRoundEndReason_t
{
	RE_ROUND_END,
	RE_CLIENT_DISCONNECT,
	RE_CLIENT_QUIT,
	RE_SERVER_MAP_CHANGE,
	RE_SERVER_SHUTDOWN,
	RE_TIME_LIMIT,
	RE_WIN_LIMIT,
	RE_WIN_DIFF_LIMIT,
	RE_ROUND_LIMIT,
	RE_NEXT_LEVEL_CVAR,
	MAX_ROUND_END_REASON
};

extern const char *g_aRoundEndReasons[MAX_ROUND_END_REASON];

//=============================================================================
//
// TF Player Round Stats
//
struct RoundStats_t
{
	int m_iStat[TFSTAT_TOTAL];

	RoundStats_t() { Reset(); };

	inline int Get( int i ) const
	{
		AssertMsg( i >= TFSTAT_UNDEFINED && i < TFSTAT_TOTAL, "Stat index out of range!" );
		return m_iStat[ i ];
	}

	inline void Set( int i, int nValue )
	{
		AssertMsg( i >= TFSTAT_UNDEFINED && i < TFSTAT_TOTAL, "Stat index out of range!" );
		m_iStat[ i ] = nValue;
	}

	void Reset()
	{
		for ( int i = 0; i < ARRAYSIZE( m_iStat ); i++ )
		{
			m_iStat[i] = 0;
		}
	};

	void AccumulateRound( const RoundStats_t &other )
	{
		for ( int i = 0; i < ARRAYSIZE( m_iStat ); i++ )
		{
			m_iStat[i] += other.m_iStat[i];
		}
	};
};

struct RoundMapStats_t
{
	int m_iStat[ TFMAPSTAT_TOTAL ];

	RoundMapStats_t() { Reset(); };

	inline int Get( int i ) const
	{
		AssertMsg( i >= TFMAPSTAT_UNDEFINED && i < TFMAPSTAT_TOTAL, "Map stat index out of range!" );
		return m_iStat[ i ];
	}

	inline void Set( int i, int nValue )
	{
		AssertMsg( i >= TFMAPSTAT_UNDEFINED && i < TFMAPSTAT_TOTAL, "Map stat index out of range!" );
		m_iStat[ i ] = nValue;
	}

	void Reset()
	{
		for ( int i = 0; i < ARRAYSIZE( m_iStat ); i++ )
		{
			m_iStat[i] = 0;
		}
	};

	void AccumulateRound( const RoundMapStats_t &other )
	{
		for ( int i = 0; i < ARRAYSIZE( m_iStat ); i++ )
		{
			m_iStat[i] += other.m_iStat[i];
		}
	};
};

enum TFGameStatsVersions_t
{
	TF_GAMESTATS_FILE_VERSION = 006,
	TF_GAMESTATS_MAGIC = 0xDEADBEEF
};

enum TFGameStatsLumpIds_t
{
	TFSTATS_LUMP_VERSION = 1,
	TFSTATS_LUMP_MAPHEADER,
	TFSTATS_LUMP_MAPDEATH,
	TFSTATS_LUMP_MAPDAMAGE,
	TFSTATS_LUMP_CLASS,	
	TFSTATS_LUMP_WEAPON,
	TFSTATS_LUMP_ENDTAG,
	MAX_LUMP_COUNT
};

struct TF_Gamestats_Version_t
{
	int m_iMagic;			// always TF_GAMESTATS_MAGIC
	int m_iVersion;
};

struct TF_Gamestats_ClassStats_t
{
	static const unsigned short LumpId = TFSTATS_LUMP_CLASS;	// Lump ids.
	int iSpawns;												// total # of spawns of this class
	int iTotalTime;												// aggregate player time in seconds in this class
	int iScore;													// total # of points scored by this class
	int iKills;													// total # of kills by this class
	int iDeaths;												// total # of deaths by this class
	int iAssists;												// total # of assists by this class
	int iCaptures;												// total # of captures by this class
	int iClassChanges;											// total # of times someone changed to this class

	void Accumulate( TF_Gamestats_ClassStats_t &other )
	{
		iSpawns += other.iSpawns;
		iTotalTime += other.iTotalTime;
		iScore += other.iScore;
		iKills += other.iKills;
		iDeaths += other.iDeaths;
		iAssists += other.iAssists;
		iCaptures += other.iCaptures;
		iClassChanges += other.iClassChanges;
	}
};

struct TF_Gamestats_WeaponStats_t
{
	static const unsigned short LumpId = TFSTATS_LUMP_WEAPON;	// Lump ids.
	int iShotsFired;
	int iCritShotsFired;
	int	iHits;
	int iTotalDamage;
	int iHitsWithKnownDistance;
	int64 iTotalDistance;

	void Accumulate( TF_Gamestats_WeaponStats_t &other )
	{
		iShotsFired += other.iShotsFired;
		iCritShotsFired += other.iCritShotsFired;
		iHits += other.iHits;
		iTotalDamage += other.iTotalDamage;
		iHitsWithKnownDistance += other.iHitsWithKnownDistance;
		iTotalDistance += other.iTotalDistance;
	}
};

//=============================================================================
//
// TF Game Level Stats Data
//
struct TF_Gamestats_LevelStats_t
{
public:

	TF_Gamestats_LevelStats_t();
	~TF_Gamestats_LevelStats_t();
	TF_Gamestats_LevelStats_t( const TF_Gamestats_LevelStats_t &stats );

	// Level start and end
	void Init( const char *pszMapName, int nMapVersion, int nIPAddr, short nPort, float flStartTime );
	void Shutdown( float flEndTime );

	void Accumulate( TF_Gamestats_LevelStats_t *pOther )
	{
		m_Header.Accumulate( pOther->m_Header );
		//m_aPlayerDeaths.AddVectorToTail( pOther->m_aPlayerDeaths );
		//m_aPlayerDamage.AddVectorToTail( pOther->m_aPlayerDamage );
		int i;
		for ( i = 0; i < ARRAYSIZE( m_aClassStats ); i++ )
		{
			m_aClassStats[i].Accumulate( pOther->m_aClassStats[i] );
		}
		for ( i = 0; i < ARRAYSIZE( m_aWeaponStats ); i++ )
		{
			m_aWeaponStats[i].Accumulate( pOther->m_aWeaponStats[i] );
		}

	}
public:

	// Level header data.
	struct LevelHeader_t
	{
		static const unsigned short LumpId = TFSTATS_LUMP_MAPHEADER;	// Lump ids.
		char			m_szMapName[64];							// Name of the map.
		int				m_nMapRevision;								// Version number for the map.
		unsigned int	m_nIPAddr;									// IP Address of the server - 4 bytes stored as an int.
		unsigned short	m_nPort;									// Port the server is using.	
		int				m_iRoundsPlayed;							// # of rounds played
		int				m_iTotalTime;								// total # of seconds of all rounds
		int				m_iBlueWins;								// # of blue team wins
		int				m_iRedWins;									// # of red team wins
		int				m_iStalemates;								// # of stalemates
		int				m_iBlueSuddenDeathWins;						// # of blue team wins during sudden death
		int				m_iRedSuddenDeathWins;						// # of red team wins during sudden death
		int				m_iLastCapChangedInRound[MAX_CONTROL_POINTS+1];		// # of times a round ended on each control point

		void Accumulate( LevelHeader_t &other )
		{
			m_iRoundsPlayed += other.m_iRoundsPlayed;
			m_iTotalTime += other.m_iTotalTime;
			m_iBlueWins += other.m_iBlueWins;
			m_iRedWins += other.m_iRedWins;
			m_iStalemates += other.m_iStalemates;
			m_iBlueSuddenDeathWins += other.m_iBlueSuddenDeathWins;
			m_iRedSuddenDeathWins += other.m_iRedSuddenDeathWins;
			for ( int i = 0; i <= MAX_CONTROL_POINTS; i++ )
			{
				m_iLastCapChangedInRound[i] += other.m_iLastCapChangedInRound[i]; 
			}
		}
	};

	// Player deaths.
	struct PlayerDeathsLump_t
	{
		static const unsigned short LumpId = TFSTATS_LUMP_MAPDEATH;	// Lump ids.
		short			nPosition[3];								// Position of death.
		short			iWeapon;									// Weapon that killed the player.
		unsigned short	iDistance;									// Distance the attacker was from the player.
		byte			iAttackClass;								// Class that killed the player.
		byte			iTargetClass;								// Class of the player killed.
	};

	// Player damage.
	struct PlayerDamageLump_t
	{
		static const unsigned short LumpId = TFSTATS_LUMP_MAPDAMAGE;	// Lump ids.
		float			fTime;										// Time of the damage event
		short			nTargetPosition[3];							// Position of target.
		short			nAttackerPosition[3];						// Position of attacker.
		short			iDamage;									// Total damage.
		short			iWeapon;									// Weapon used.
		byte			iAttackClass;								// Class of the attacker
		byte			iTargetClass;								// Class of the target
		byte			iCrit;										// was the shot a crit?
		byte			iKill;										// did the shot kill the target?
	};

	// Data.
	LevelHeader_t					m_Header;							// Level header.
	// Disabling These Fields
	//CUtlVector<PlayerDeathsLump_t>	m_aPlayerDeaths;					// Vector of player deaths.
	//CUtlVector<PlayerDamageLump_t>	m_aPlayerDamage;					// Vector of player damage.	
	bool							m_bIsRealServer;
	TF_Gamestats_ClassStats_t		m_aClassStats[TF_CLASS_COUNT_ALL];	// Vector of class data
	TF_Gamestats_WeaponStats_t		m_aWeaponStats[TF_WEAPON_COUNT];	// Vector of weapon data
	// Temporary data.
	bool							m_bInitialized;		// Has the map Map Stat Data been initialized.
	time_t							m_iMapStartTime;
	time_t							m_iRoundStartTime; // time_t version for steamworks stats
	float							m_flRoundStartTime;
	int								m_iPeakPlayerCount[TF_TEAM_COUNT];
};

struct TF_Gamestats_RoundStats_t
{
public:

	TF_Gamestats_RoundStats_t();
	~TF_Gamestats_RoundStats_t();

private:
	TF_Gamestats_RoundStats_t( const TF_Gamestats_RoundStats_t &stats ) {}

public:
	void Reset();
	void ResetSummary();

	struct RoundSummary_t
	{
		int iTeamQuit;
		int iPoints;
		int iBonusPoints;
		int iKills;
		int iDeaths;
		int	iSuicides;
		int	iAssists;
		int	iBuildingsBuilt;
		int	iBuildingsDestroyed;
		int	iHeadshots;
		int	iDominations;
		int	iRevenges;
		int	iInvulns;
		int	iTeleports;
		int	iDamageDone;
		int	iHealingDone;
		int	iCrits;
		int	iBackstabs;
		int iThrowableHits;
		int iThrowableKills;
	};

	RoundSummary_t				m_Summary;

	static time_t				m_iRoundStartTime;
	static int					m_iNumRounds;
};

struct TF_Gamestats_KillStats_t
{
public:
	TF_Gamestats_KillStats_t();
	~TF_Gamestats_KillStats_t();

private:
	TF_Gamestats_KillStats_t( const TF_Gamestats_KillStats_t &stats ) {}

public:
	void Reset();
};

// Old style killstats matrix.
struct KillStats_t
{
	KillStats_t() { Reset(); }

	void Reset()
	{
		Q_memset( iNumKilled, 0, sizeof( iNumKilled ) );
		Q_memset( iNumKilledBy, 0, sizeof( iNumKilledBy ) );
		Q_memset( iNumKilledByUnanswered, 0, sizeof( iNumKilledByUnanswered ) );
	}

	int iNumKilled[MAX_PLAYERS_ARRAY_SAFE];					// how many times this player has killed every other player
	int iNumKilledBy[MAX_PLAYERS_ARRAY_SAFE];				// how many times this player has been killed by every other player
	int iNumKilledByUnanswered[MAX_PLAYERS_ARRAY_SAFE];		// how many unanswered kills this player has been dealt by every other player
};

//=============================================================================
// LoadoutStats
struct LoadoutStats_t
{
	LoadoutStats_t() { Reset(); }

	void Reset()
	{
		V_memset( iLoadoutItemDefIndices, INVALID_ITEM_DEF_INDEX, sizeof( iLoadoutItemDefIndices ) );
		V_memset( iLoadoutItemQualities, AE_UNDEFINED, sizeof( iLoadoutItemQualities ) );
		V_memset( iLoadoutItemStyles, 0, sizeof( iLoadoutItemStyles ) );
		
		flStartTime = 0;
		iClass = TF_CLASS_UNDEFINED;
	}

	void Set ( int iPlayerClass )
	{
		iClass = iPlayerClass;
		flStartTime = gpGlobals->curtime;
	}

	void SetItemDef ( int iSlot, itemid_t iItemDef, entityquality_t iItemQuality, style_index_t iStyle )
	{
		iLoadoutItemDefIndices[iSlot] = iItemDef;
		iLoadoutItemQualities[iSlot] = iItemQuality;
		iLoadoutItemStyles[iSlot] = iStyle;
	}

	item_definition_index_t iLoadoutItemDefIndices[CLASS_LOADOUT_POSITION_COUNT];
	entityquality_t iLoadoutItemQualities[CLASS_LOADOUT_POSITION_COUNT];
	style_index_t iLoadoutItemStyles[CLASS_LOADOUT_POSITION_COUNT];
	float flStartTime;
	int iClass;
};

//=============================================================================
//
// TF Player Stats 
//
struct PlayerStats_t
{
	PlayerStats_t()	
	{
		Reset();
	};

	void Reset()
	{
		statsCurrentLife.Reset();
		statsCurrentRound.Reset();
		statsAccumulated.Reset();
		mapStatsCurrentLife.Reset();
		mapStatsCurrentRound.Reset();
		mapStatsAccumulated.Reset();
		statsKills.Reset();
		loadoutStats.Reset();
		iConnectTime = 0;
		iDisconnectTime = 0;
	}

	PlayerStats_t( const PlayerStats_t &other )
	{
		statsCurrentLife	= other.statsCurrentLife;
		statsCurrentRound	= other.statsCurrentRound;
		statsAccumulated	= other.statsAccumulated;
		mapStatsCurrentLife	= other.mapStatsCurrentLife;
		mapStatsCurrentRound = other.mapStatsCurrentRound;
		mapStatsAccumulated	= other.mapStatsAccumulated;
		loadoutStats		= other.loadoutStats;
		iConnectTime		= other.iConnectTime;
		iDisconnectTime		= other.iDisconnectTime;
	}

	RoundStats_t	statsCurrentLife;
	RoundStats_t	statsCurrentRound;
	RoundStats_t	statsAccumulated;
	RoundMapStats_t	mapStatsCurrentLife;
	RoundMapStats_t	mapStatsCurrentRound;
	RoundMapStats_t	mapStatsAccumulated;
	KillStats_t		statsKills;
	LoadoutStats_t	loadoutStats;
	int				iConnectTime;
	int				iDisconnectTime;
};

// reported stats structure that contains all stats data uploaded from TF server to Steam.  Note that this
// code is shared between TF server and processgamestats, which cracks the data file on the back end
struct TFReportedStats_t
{
	TFReportedStats_t();
	~TFReportedStats_t();
	void Clear();
	TF_Gamestats_LevelStats_t	*FindOrAddMapStats( const char *szMapName );
#ifdef GAME_DLL
	void AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer );
	bool LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer );
#endif 

	bool													m_bValidData;
	TF_Gamestats_LevelStats_t								*m_pCurrentGame;
	CUtlDict<TF_Gamestats_LevelStats_t, unsigned short>		m_dictMapStats;
};

struct ClassStats_t
{
	int					iPlayerClass;		// which class these stats refer to
	int					iNumberOfRounds;	// how many times player has played this class
	RoundStats_t		accumulated;
	RoundStats_t		max;
	RoundStats_t		currentRound;

	RoundStats_t		accumulatedMVM;
	RoundStats_t		maxMVM;

	ClassStats_t()
	{
		iPlayerClass	= TF_CLASS_UNDEFINED;
		iNumberOfRounds = 0;
	}

	void AccumulateRound( const RoundStats_t &other )
	{
		iNumberOfRounds++;
		accumulated.AccumulateRound( other );
		currentRound = other;
	}

	void AccumulateMVMRound( const RoundStats_t &other )
	{
		iNumberOfRounds++;
		accumulatedMVM.AccumulateRound( other );
		currentRound = other;
	}
};

struct MapStats_t
{
	map_identifier_t	iMapID;				// which map these stats refer to
	int					iNumberOfRounds;	// how many times player has played this map
	RoundMapStats_t		accumulated;
	RoundMapStats_t		currentRound;

	MapStats_t()
	{
		iMapID = 0xFFFFFFFF;
		iNumberOfRounds = 0;
	}

	void AccumulateRound( const RoundMapStats_t &other )
	{
		iNumberOfRounds++;
		accumulated.AccumulateRound( other );
		currentRound = other;
	}
};

//=============================================================================
// Beta Map Stats
//=============================================================================

//=============================================================================
// Robot Destruction
struct RobotDestructionStats_t
{
	RobotDestructionStats_t();

	void	Clear();
	int		GetRobotInteractionCount();
	int		GetRobotCoreInteractionCount();
	int		GetFlagInteractionCount();

	// Robot Cores Collected
	int		iCoresCollectedByTeam[ TF_TEAM_COUNT ];

	// Collected By What Class
	int		iCoreCollectedByClass[ TF_CLASS_COUNT ];

	// Robots Killed By Type
	// eRobotType::NUM_ROBOT_TYPES
	int		iBlueRobotsKilledByType[ 3 ];
	int		iRedRobotsKilledByType[ 3 ];

	int		iRobotsDamageFromClass[ TF_CLASS_COUNT ];

	// Player Interaction
	int		iRobotInteraction[MAX_PLAYERS_ARRAY_SAFE];
	int		iRobotCoreInteraction[MAX_PLAYERS_ARRAY_SAFE];
	int		iFlagInteraction[MAX_PLAYERS_ARRAY_SAFE];
};

//=============================================================================
// Cactus Canyon

//=============================================================================
// Passtime
struct PasstimeStats_t
{
	PasstimeStats_t() { Clear(); }
	void Clear();
	void AddBallFracSample( float f );
	void AddPassTravelDistSample( float f );

	// To get comprehensive class stats, we need an event log instead of a summary.
	// But for now this should cover what we need.
	// These class stats were specifically requested by Travis@br, in addition to
	// total kills by class. The total kills by class is tracked by TF already.
	struct Classes_t
	{
		int nTotalScores;
		int nTotalCarrySec;
	} classes[TF_CLASS_COUNT_ALL];

	struct RoundSummary_t
	{
		int nTotalPassesStarted;
		int nTotalPassesFailed;
		int nTotalPassesShotDown;
		int nTotalPassesCompleted;
		int nTotalPassesCompletedNearGoal;
		int nTotalPassesIntercepted;
		int nTotalPassesInterceptedNearGoal;
		int nTotalPassRequests;
		int nTotalTosses;
		int nTotalTossesCompleted;
		int nTotalTossesIntercepted;
		int nTotalTossesInterceptedNearGoal;
		int nTotalSteals;
		int nTotalStealsNearGoal; 
		int nTotalBallSpawnShots;
		int nTotalScores;
		int nTotalRecoveries;
		int nTotalCarrySec;
		int nTotalWinningTeamBallCarrySec;
		int nTotalLosingTeamBallCarrySec;
		int nTotalThrowCancels;
		int nTotalSpeedBoosts;
		int nTotalJumpPads;
		int nTotalCarrierSpeedBoosts;
		int nTotalCarrierJumpPads;
		int nTotalBallDeflects;
		int nBallNeutralSec;
		int nGoalType;
		int nRoundEndReason;
		int nRoundRemainingSec;
		int nRoundMaxSec;
		int nPlayersRedMax;
		int nPlayersBlueMax;
		int nScoreBlue;
		int nScoreRed;
		bool bStalemate;
		bool bSuddenDeath;
		bool bMeleeOnlySuddenDeath;

		// histogram used to create min/max/mean/med/mode/stdev stats
		uint32 nBallFracSampleCount;
		uint32 arrBallFracHist[ 256 ];
		uint32 nBallFracHistSum;

		// sample set used to create min/max/mean/med/stdev stats
		static const uint32 k_nMaxPassTravelDistSamples = 1024;
		uint32 nPassTravelDistSampleCount;
		uint16 arrPassTravelDistSamples[ k_nMaxPassTravelDistSamples ];
	} summary;
};

const char* GetGameTypeID();

#ifdef CLIENT_DLL
MapStats_t &GetMapStats( map_identifier_t iMapID );
#endif

#endif // TF_GAMESTATS_SHARED_H
