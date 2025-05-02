//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_populator.h
// KeyValues driven procedural population system
// Michael Booth, April 2011

#ifndef TF_POPULATOR_H
#define TF_POPULATOR_H


#include "bot/tf_bot.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_populator_spawners.h"
#include "tf_populators.h"

class CMannVsMachineStats;
class KeyValues;
class IPopulator;
class CPopulationManager;
class CWave;

#define MVM_POP_FILE_PATH "scripts/population"

//-----------------------------------------------------------------------
class CPopulationManager : public CPointEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CPopulationManager, CPointEntity );
	DECLARE_DATADESC();

	CPopulationManager( void );
	virtual ~CPopulationManager();

	// CPointEntity
	virtual void Spawn( void );				// entity spawn

	// CGameEventListener
	virtual void FireGameEvent( IGameEvent *gameEvent );

	virtual void Reset( void );
	virtual bool Initialize( void );
	virtual void Precache( void );

	const char *GetPopulationFilename( void );
	const char *GetPopulationFilenameShort( void );
	void SetPopulationFilename( const char *populationFile );

	// Resolve a pop file shortname to a full name
	bool FindPopulationFileByShortName( const char *pShortName, CUtlString &outFullName );

	// Enumerate possible population files for this map, searching e.g. mapname_*.pop, //BSP/.../*.pop
	// This is separate from the explicit popfile lists for tours.
	static void FindDefaultPopulationFileShortNames( CUtlVector< CUtlString > &outVecShortNames );

	void SetupOnRoundStart( void );

	void Update( void );					// continuously invoked to modify population over time

	// Triggered by GameRules for any mode specific logic that isn't suitable for the entity think (e.g. not simulating)
	void GameRulesThink();

	void UpdateObjectiveResource( void );
	void ResetMap( void );

	void CycleMission ( void );
	bool LoadMissionCycleFile ( void );
	bool IsValidMvMMap( const char *pszMapName );
	bool IsValidPopfile( CUtlString fullPath );

	// Waves
	void ShowNextWaveDescription( void );
	void StartCurrentWave( void );
	void JumpToWave( uint32 waveNumber, float fCleanMoneyPercent = -1.0f );
	void WaveEnd ( bool bSuccess );

	CWave *GetCurrentWave( void );

	int	 GetWaveNumber( void ) { return m_iCurrentWaveIndex; }
	int GetTotalWaveCount( void ) { return m_waveVector.Count(); }

	// Check Points
	void ClearCheckpoint( void );
	void SetCheckpoint( int waveNumber );
	void RestoreCheckpoint( void );
	void RestoreItemToCheckpointState( CTFPlayer *player, CEconItemView *item );
	void ForgetOtherBottleUpgrades ( CTFPlayer *player, CEconItemView *pItem, int upgradeToKeep );
	void RestorePlayerCurrency ();

	CUtlVector< CUpgradeInfo > *GetPlayerUpgradeHistory ( CTFPlayer *player );	// Returns the Upgrade vector for a given player
	int GetPlayerCurrencySpent ( CTFPlayer *player );
	void AddPlayerCurrencySpent ( CTFPlayer *player, int cost );
	void SendUpgradesToPlayer ( CTFPlayer *player );

	void OnPlayerKilled( CTFPlayer *corpse );
	void OnCurrencyPackFade( void );
	void OnCurrencyCollected( int nAmount, bool bCountAsDropped, bool bIsBonus );
	int GetTotalPopFileCurrency( void );

	void AdjustMinPlayerSpawnTime( void );

	void MarkAllCurrentPlayersSafeToLeave();
	void MvMVictory( void );
	void PlayerDoneViewingLoot( const CTFPlayer* pPlayer );

	void GetSentryBusterDamageAndKillThreshold( int &nDamage, int &nKills ) const;

	bool IsInEndlessWaves ( void );

	// Endless
	float GetHealthMultiplier ( bool bIsTank = false );
	float GetDamageMultiplier ( void );
	void EndlessParseBotUpgrades( void );
	void EndlessRollEscalation ( void );
	void EndlessSetAttributesForBot( CTFBot *pBot );
	bool EndlessShouldResetFlag ();
	void EndlessFlagHasReset ();

	// inlined
	bool IsRestoringCheckpoint( void ) const;	// return true if we are in the process of restoring players to their checkpointed state
	float GetElapsedTime( void ) const;		// return elapsed time since scenario started
	int GetStartingCurrency( void ) const;
	int GetRespawnWaveTime( void ) const;
	bool CanBotsAttackWhileInSpawnRoom( void ) const;
	KeyValues *GetTemplate( const char *pszName ) const;

	bool IsAdvancedPopFile( void ) { return m_bAdvancedPopFile; }
	void SetMapRestartTime( float flTime ) { m_flMapRestartTime = flTime; }
	bool IsPopFileEventType( int fileType ) { return m_nMvMEventPopfileType == fileType; }
	
	void DebugWaveStats();

	void AllocateBots();

	void PauseSpawning();
	void UnpauseSpawning();
	bool IsSpawningPaused() const { return m_bSpawningPaused; }

	bool IsBonusRound() const { return m_bBonusRound; }
	CBaseCombatCharacter* GetBonusBoss() const { return m_hBonusBoss; }

	static bool GetWavesUseReadyBetween() { return true; }

	void SetDefaultEventChangeAttributesName( const char* pszDefaultEventChangeAttributesName ) { m_defaultEventChangeAttributesName = pszDefaultEventChangeAttributesName; }
	const char* GetDefaultEventChangeAttributesName() const { return m_defaultEventChangeAttributesName.String(); }
	bool HasEventChangeAttributes( const char* pszEventName ) const;

	static int CollectMvMBots ( CUtlVector< CTFPlayer *> *pBots );

	// Respec
	void RemovePlayerAndItemUpgradesFromHistory( CTFPlayer *pPlayer );
	void AddRespecToPlayer( CTFPlayer *pPlayer );
	void RemoveRespecFromPlayer( CTFPlayer *pPlayer );
	void SetNumRespecsForPlayer( CTFPlayer *pPlayer, int nCount );
	int GetNumRespecsAvailableForPlayer( CTFPlayer *pPlayer );
	int GetNumRespecsEarnedInWave( void ) { return m_nRespecsAwardedInWave; }
	int GetNumRespecsEarned( void ) { return m_nRespecsAwarded; }
	void ResetRespecPoints( void );

	// Buyback
	void SetBuybackCreditsForPlayer( CTFPlayer *pPlayer, int nCount );
	void RemoveBuybackCreditFromPlayer( CTFPlayer *pPlayer );
	int GetNumBuybackCreditsForPlayer( CTFPlayer *pPlayer );
	bool IsPlayerBeingTrackedForBuybacks( CTFPlayer *pPlayer );


private:
	struct CheckpointSnapshotInfo
	{
		CSteamID m_steamId;							// which player this snapshot is for	
		int m_currencySpent;						// how much money they had spent up to this check point
		CUtlVector< CUpgradeInfo > m_upgradeVector;	// the upgrades the player had as this checkpoint
	};

	struct PlayerUpgradeHistory
	{	
		CSteamID m_steamId;							// which player this snapshot is for
		CUtlVector< CUpgradeInfo > m_upgradeVector;	 
		int m_currencySpent;
	};

	void PostInitialize( void );
	bool Parse( void );	// read in population from file from m_filename

	CheckpointSnapshotInfo *FindCheckpointSnapshot( CTFPlayer *player ) const;
	CheckpointSnapshotInfo *FindCheckpointSnapshot( CSteamID id ) const;
	PlayerUpgradeHistory *FindOrAddPlayerUpgradeHistory ( CTFPlayer *player );
	PlayerUpgradeHistory *FindOrAddPlayerUpgradeHistory ( CSteamID steamId );

	void LoadLastKnownMission();
	bool LoadMvMMission( KeyValues *pNextMission );

	CUtlVector< IPopulator * > m_populatorVector;
	char m_popfileFull[ MAX_PATH ];
	char m_popfileShort[ MAX_PATH ];
	
	KeyValues	*m_pTemplates;

	bool m_bIsInitialized;
	bool m_bAllocatedBots;
	
	bool m_bBonusRound;
	CHandle< CBaseCombatCharacter > m_hBonusBoss;

	int m_nStartingCurrency;
	int m_nLobbyBonusCurrency;
	int m_nMvMEventPopfileType;
	int m_nRespawnWaveTime;
	bool m_bFixedRespawnWaveTime;
	bool m_canBotsAttackWhileInSpawnRoom;
	int m_sentryBusterDamageDealtThreshold;
	int m_sentryBusterKillThreshold;

	uint32	m_iCurrentWaveIndex;
	CUtlVector< CWave * > m_waveVector;		// pointers to waves within m_populationVector

	float m_flMapRestartTime;					// Restart the Map if gameover and this time elapses

	CUtlVector< PlayerUpgradeHistory * > m_playerUpgrades;		// list of all players and their upgrades who have played on this MVM rotation

	bool m_isRestoringCheckpoint;

	// checkpoint data must be static because the population manager entity is destroyed and recreated each round
	static CUtlVector< CheckpointSnapshotInfo * > m_checkpointSnapshot;	// snapshot of player state at the saved checkpoint

	static int m_checkpointWaveIndex;				// wave to restart at if scenario lost
	static int m_nNumConsecutiveWipes;

	bool m_bAdvancedPopFile;
	bool m_bCheckForCurrencyAchievement;

	CMannVsMachineStats *m_pMVMStats;
	KeyValues *m_pKvpMvMMapCycle;

	bool m_bSpawningPaused;
	bool m_bIsWaveJumping;
	bool m_bEndlessOn;
	CUtlVector< CMvMBotUpgrade > m_BotUpgradesList;
	CUtlVector< CMvMBotUpgrade > m_EndlessActiveBotUpgrades;
	CUniformRandomStream m_randomizer;
	CUtlVector< int > m_EndlessSeeds;
	bool m_bShouldResetFlag;
	CUtlVector< const CTFPlayer* > m_donePlayers;

	CUtlString m_defaultEventChangeAttributesName;

	// Respec
	CUtlMap< uint64, int > m_PlayerRespecPoints;	// The number of upgrade respecs players (steamID) have
	int m_nRespecsAwarded;
	int m_nRespecsAwardedInWave;
	int m_nCurrencyCollectedForRespec;

	// Buyback
	CUtlMap< uint64, int > m_PlayerBuybackPoints;	// The number of times a player can buyback

};

inline bool CPopulationManager::IsRestoringCheckpoint( void ) const
{
	return m_isRestoringCheckpoint;
}

inline int CPopulationManager::GetStartingCurrency( void ) const
{ 
	return m_nStartingCurrency + m_nLobbyBonusCurrency;
}

inline int CPopulationManager::GetRespawnWaveTime( void ) const
{ 
	return m_nRespawnWaveTime; 
}

inline bool CPopulationManager::CanBotsAttackWhileInSpawnRoom( void ) const
{
	return m_canBotsAttackWhileInSpawnRoom;
}

inline KeyValues *CPopulationManager::GetTemplate( const char *pszName ) const
{ 
	return m_pTemplates->FindKey( pszName ); 
}

// singleton accessor
extern CPopulationManager *g_pPopulationManager;


#endif // TF_POPULATOR_H
