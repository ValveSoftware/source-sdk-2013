//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: tf_populator_spawners
// Implementations of NPC Spawning Code for PvE related game modes (MvM)
//=============================================================================//
#ifndef TF_POPULATORS_H
#define TF_POPULATORS_H

#include "tf_population_manager.h"

class KeyValues;
class IPopulator;
class IPopulationSpawner;
class CPopulationManager;
class CWave;
class CSpawnLocation;

//-----------------------------------------------------------------------
class CSpawnLocation
{
public:
	CSpawnLocation();

	bool Parse( KeyValues *data );

	bool IsValid( void ) const;

	SpawnLocationResult FindSpawnLocation( Vector& vSpawnPosition );

private:
	CTFNavArea *SelectSpawnArea( void ) const;

	RelativePositionType m_relative;
	TFTeamSpawnVector_t m_teamSpawnVector;

	int m_nSpawnCount;
	int m_nRandomSeed;
	bool m_bClosestPointOnNav;
};

inline bool CSpawnLocation::IsValid( void ) const
{
	return m_relative != UNDEFINED || m_teamSpawnVector.Count() > 0;
}


//-----------------------------------------------------------------------
// For spawning bots/players at a specific position
class CPopulatorInternalSpawnPoint : public CPointEntity
{
	DECLARE_CLASS( CPopulatorInternalSpawnPoint, CPointEntity );
};

extern CHandle< CPopulatorInternalSpawnPoint > g_internalSpawnPoint;

//-----------------------------------------------------------------------
// A Populator manages the populating of entities in the environment.
class IPopulator
{
public:
	IPopulator( CPopulationManager *manager )
	{
		m_manager = manager;
		m_spawner = NULL;
	}

	virtual ~IPopulator()
	{
		if ( m_spawner )
		{
			delete m_spawner;
		}
		m_spawner = NULL;
	}

	virtual bool Parse( KeyValues *data ) = 0;

	virtual void PostInitialize( void ) { }		// create initial population at start of scenario
	virtual void Update( void ) { }				// continuously invoked to modify population over time
	virtual void UnpauseSpawning() {}

	virtual void OnPlayerKilled( CTFPlayer *corpse ) { }

	CPopulationManager *GetManager( void ) const { return m_manager; }

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const
	{
		if ( m_spawner )
		{
			return m_spawner->HasEventChangeAttributes( pszEventName );
		}

		return false;
	}

	IPopulationSpawner *m_spawner;

private:
	CPopulationManager *m_manager;
};


//-----------------------------------------------------------------------
// Invokes its spawner when mission conditions are met
class CMissionPopulator : public IPopulator
{
public:
	CMissionPopulator( CPopulationManager *manager );
	virtual ~CMissionPopulator() { }

	virtual bool Parse( KeyValues *data );

	virtual void Update( void );			// continuously invoked to modify population over time
	virtual void UnpauseSpawning( void );

	int BeginAtWave( void ) { return m_beginAtWaveIndex; }
	int StopAtWave( void ) { return m_stopAtWaveIndex; }

	CTFBot::MissionType GetMissionType( void ){ return m_mission; }

private:
	CTFBot::MissionType m_mission;
	CSpawnLocation m_where;

	bool UpdateMissionDestroySentries( void );
	bool UpdateMission( CTFBot::MissionType mission );

	enum StateType
	{
		NOT_STARTED,
		INITIAL_COOLDOWN,
		RUNNING
	};

	StateType m_state;

	float m_initialCooldown;
	float m_cooldownDuration;
	CountdownTimer m_cooldownTimer;
	CountdownTimer m_checkForDangerousSentriesTimer;
	int m_desiredCount;
	int m_beginAtWaveIndex;					// this mission becomes active at this wave number
	int m_stopAtWaveIndex;					// stop when this wave becomes active
};


//-----------------------------------------------------------------------
// Invokes its spawner at random positions scattered throughout 
// the environment at PostInitialize()
class CRandomPlacementPopulator : public IPopulator
{
public:
	CRandomPlacementPopulator( CPopulationManager *manager );
	virtual ~CRandomPlacementPopulator() { }

	virtual bool Parse( KeyValues *data );

	virtual void PostInitialize( void );		// create initial population at start of scenario

	int m_count;
	float m_minSeparation;
	unsigned int m_navAreaFilter;
};


//-----------------------------------------------------------------------
// Invokes its spawner periodically
class CPeriodicSpawnPopulator : public IPopulator
{
public:
	CPeriodicSpawnPopulator( CPopulationManager *manager );
	virtual ~CPeriodicSpawnPopulator() { }

	virtual bool Parse( KeyValues *data );

	virtual void PostInitialize( void );		// create initial population at start of scenario
	virtual void Update( void );			// continuously invoked to modify population over time
	virtual void UnpauseSpawning( void );

	CSpawnLocation m_where;
	float m_minInterval;
	float m_maxInterval;

private:
	CountdownTimer m_timer;
};


//-----------------------------------------------------------------------
// Spawns a group of entities within a Wave
class CWaveSpawnPopulator : public IPopulator
{
public:
	CWaveSpawnPopulator( CPopulationManager *manager );
	virtual ~CWaveSpawnPopulator();

	virtual bool Parse( KeyValues *data );

	virtual void Update( void );			// continuously invoked to modify population over time

	virtual void OnPlayerKilled( CTFPlayer *corpse );

	CSpawnLocation m_where;
	int m_totalCount;
	int m_remainingCount;
	int m_nClassCounts;
	int m_maxActive;						// the maximum number of entities active at one time
	int m_spawnCount;						// the number of entities to spawn at once
	float m_waitBeforeStarting;
	float m_waitBetweenSpawns;				// between spawns of mobs
	bool m_bWaitBetweenSpawnAfterDeath;

	CFmtStr m_startWaveWarningSound;
	EventInfo *m_startWaveOutput;

	CFmtStr m_firstSpawnWarningSound;
	EventInfo *m_firstSpawnOutput;

	CFmtStr m_lastSpawnWarningSound;
	EventInfo *m_lastSpawnOutput;

	CFmtStr m_doneWarningSound;
	EventInfo *m_doneOutput;

	int		m_totalCurrency;
	int		m_unallocatedCurrency;

	CUtlString m_name;
	CUtlString m_waitForAllSpawned;
	CUtlString m_waitForAllDead;

	bool IsDone( void ) const
	{
		return m_state == DONE;
	}

	bool IsDoneSpawningBots( void ) const
	{
		return m_state > SPAWNING;
	}

	// Invoked by td_setnextwave to finish off a wave
	void ForceFinish( void );
	void ForceReset( void ) 
	{
		m_unallocatedCurrency = m_totalCurrency;
		m_remainingCount = m_totalCount;
		m_state = PENDING; 
	}

	bool IsSupportWave( void ) const { return m_bSupportWave; }
	bool IsLimitedSupportWave( void ) const { return m_bLimitedSupport; }
	void SetParent( CWave *pParent ) { m_pParent = pParent; }
	int GetCurrencyAmountPerDeath( void );
	void OnNonSupportWavesDone( void );

private:
	bool IsFinishedSpawning( void );
	
	CountdownTimer m_timer;
	EntityHandleVector_t m_activeVector;
	int m_countSpawnedSoFar;
	int m_myReservedSlotCount;
	
	bool m_bSupportWave;
	bool m_bLimitedSupport;
	CWave *m_pParent;

	enum InternalStateType
	{
		PENDING,
		PRE_SPAWN_DELAY,
		SPAWNING,
		WAIT_FOR_ALL_DEAD,
		DONE
	};
	InternalStateType m_state;
	void SetState( InternalStateType eState );

	int ReservePlayerSlots( int count );	// reserve 'count' player slots so other WaveSpawns don't take them
	void ReleasePlayerSlots( int count );	// release 'count' player slots that have been previously reserved
	static int m_reservedPlayerSlotCount;

	bool m_bRandomSpawn;
	SpawnLocationResult m_spawnLocationResult;
	Vector m_vSpawnPosition;
};


struct WaveClassCount_t
{
	int nClassCount;
	string_t iszClassIconName;
	unsigned int iFlags;
};


//-----------------------------------------------------------------------
// Spawns sequential "waves" of entities over time.
// A wave consists of one or more WaveSpawns that all run concurrently.
// The wave is done when all contained WaveSpawns are done.
class CWave : public IPopulator
{
public:
	CWave( CPopulationManager *manager );
	virtual ~CWave();

	virtual bool Parse( KeyValues *data );
	virtual void Update( void );

	virtual void OnPlayerKilled( CTFPlayer *corpse );

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE;

	void ForceFinish();							// used when forcing a wave to finish
	void ForceReset();							// used when forcing a wave to start

	CWaveSpawnPopulator *FindWaveSpawnPopulator( const char *name );	// find a CWaveSpawnPopulator by name

	void AddClassType( string_t iszClassIconName, int nCount, unsigned int iFlags );
	
	int GetNumClassTypes( void ) const { return m_nWaveClassCounts.Count(); }
	void StartUpgradesAlertTimer ( float flTime ) { m_GetUpgradesAlertTimer.Start( flTime ); }
	void SetStartTime (float flTime) { m_flStartTime = flTime; }

	// inline
	bool IsCheckpoint( void ) const;
	CWave *GetNextWave( void ) const;
	void SetNextWave( CWave *wave );
	const char *GetDescription( void ) const;
	int GetTotalCurrency( void ) const;
	int GetEnemyCount( void ) const;
	int GetClassCount( int nIndex ) const;
	string_t GetClassIconName( int nIndex ) const;
	unsigned int GetClassFlags( int nIndex ) const;

	int NumTanksSpawned( void ) const;
	void IncrementTanksSpawned( void );

	int NumSentryBustersSpawned( void ) const;
	void IncrementSentryBustersSpawned( void );
	int NumSentryBustersKilled( void ) const;
	void IncrementSentryBustersKilled( void );
	void ResetSentryBustersKilled( void );

	int NumEngineersTeleportSpawned( void ) const;
	void IncrementEngineerTeleportSpawned( void );

private:
	bool IsDoneWithNonSupportWaves( void );

	void ActiveWaveUpdate();
	void WaveCompleteUpdate();
	void WaveIntermissionUpdate();

	CUtlVector< CWaveSpawnPopulator * > m_waveSpawnVector;

	bool m_isStarted;
	bool m_bFiredInitWaveOutput;
	int m_iEnemyCount;
	int m_nTanksSpawned;
	int m_nSentryBustersSpawned;
	int m_nNumEngineersTeleportSpawned;

	int m_nNumSentryBustersKilled;

	CUtlVector< WaveClassCount_t > m_nWaveClassCounts;
	int	m_totalCurrency;

	EventInfo *m_startOutput;
	EventInfo *m_doneOutput;
	EventInfo *m_initOutput;

	CFmtStr m_description;
	CFmtStr m_soundName;
	
	float m_waitWhenDone;
	CountdownTimer m_doneTimer;	

	bool m_bCheckBonusCreditsMin;
	bool m_bCheckBonusCreditsMax;
	float m_flBonusCreditsTime;

	bool m_bPlayedUpgradeAlert;
	CountdownTimer m_GetUpgradesAlertTimer;

	bool m_isEveryContainedWaveSpawnDone;
	float m_flStartTime;
};

inline const char *CWave::GetDescription( void ) const
{
	return m_description;
}

inline int CWave::GetTotalCurrency( void ) const
{
	return m_totalCurrency;
}

inline int CWave::GetEnemyCount( void ) const
{
	return m_iEnemyCount;
}

inline int CWave::GetClassCount( int nIndex ) const
{
	return m_nWaveClassCounts[ nIndex ].nClassCount;
}

inline string_t CWave::GetClassIconName( int nIndex ) const
{
	return m_nWaveClassCounts[ nIndex ].iszClassIconName;
}

inline unsigned int CWave::GetClassFlags( int nIndex ) const
{
	return m_nWaveClassCounts[ nIndex ].iFlags;
}

inline int CWave::NumTanksSpawned( void ) const
{
	return m_nTanksSpawned;
}

inline void CWave::IncrementTanksSpawned( void )
{
	m_nTanksSpawned++;
}


inline int CWave::NumSentryBustersSpawned( void ) const
{
	return m_nSentryBustersSpawned;
}

inline void CWave::IncrementSentryBustersSpawned( void )
{
	m_nSentryBustersSpawned++;
}

inline int CWave::NumSentryBustersKilled( void ) const
{
	return m_nNumSentryBustersKilled;
}

inline void CWave::IncrementSentryBustersKilled( void )
{
	m_nNumSentryBustersKilled++;
}

inline void CWave::ResetSentryBustersKilled( void )
{
	m_nNumSentryBustersKilled = 0;
}

inline int CWave::NumEngineersTeleportSpawned( void ) const
{
	return m_nNumEngineersTeleportSpawned;
}

inline void CWave::IncrementEngineerTeleportSpawned( void )
{
	m_nNumEngineersTeleportSpawned++;
}

#endif // TF_POPULATORS_H
