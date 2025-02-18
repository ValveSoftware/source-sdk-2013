//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates mann vs machine stats
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MANN_VS_MACHINE_STATS_H
#define TF_MANN_VS_MACHINE_STATS_H

#include "tf_player_shared.h"

#ifdef CLIENT_DLL
#define CTFPlayer C_TFPlayer
#define CMannVsMachineStats C_MannVsMachineStats
#define CMannVsMachineWaveStats C_MannVsMachineWaveStats
#define CMannVsMachineLocalWaveStats C_MannVsMachineLocalWaveStats
#define CMannVsMachineUpgradeEvent C_MannVsMachineUpgradeEvent
#define CMannVsMachinePlayerWaveStats C_MannVsMachinePlayerWaveStats
class C_TFPlayer;
#else
class CTFPlayer;
#endif

//class CMannVsMachineStats;

//-----------------------------------------------------------------------------
// Public interface to hide the inner-workings (which allows me to iterate on it and not force everyone to recompile)

//-----------------------------------------------------------------------------
// The types of events sent down to clients.  Add to the end of the appropriate section
// for backwards compatibility.
enum eMannVsMachineEvent
{
	kMVMEvent_Player_Points,
	kMVMEvent_Player_Death,
	kMVMEvent_Player_PickedUpCredits,
	kMVMEvent_Player_BoughtInstantRespawn,
	kMVMEvent_Player_BoughtBottle,
	kMVMEvent_Player_BoughtUpgrade,
	kMVMEvent_Player_ActiveUpgrades,
	// max
	kMVMEvent_Max = 255
};

enum eMvMEnemyTypes
{
	kMvMEnemy_Bot,
	kMvMEnemy_Giant,
	kMvMEnemy_Tank
};

//-----------------------------------------------------------------------------
// Stats for a wave
struct CMannVsMachineWaveStats
{
	CMannVsMachineWaveStats()
	{
		nCreditsDropped = 0;
		nCreditsAcquired = 0;
		nCreditsBonus = 0;
		nPlayerDeaths = 0;
		nBuyBacks = 0;
		nAttempts = 0;
	}

	void ClearStats () 
	{
		nCreditsDropped = 0;
		nCreditsAcquired = 0;
		nCreditsBonus = 0;
		nPlayerDeaths = 0;
		nBuyBacks = 0;
		nAttempts = 0;
	}

	void operator+=( const CMannVsMachineWaveStats &rhs )
	{
		nCreditsDropped += rhs.nCreditsDropped;
		nCreditsAcquired += rhs.nCreditsAcquired;
		nCreditsBonus += rhs.nCreditsBonus;
		nPlayerDeaths += rhs.nPlayerDeaths;
		nBuyBacks += rhs.nBuyBacks;
		nAttempts += rhs.nAttempts;
	}

	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CMannVsMachineWaveStats );

	CNetworkVar( uint32, nCreditsDropped );
	CNetworkVar( uint32, nCreditsAcquired );
	CNetworkVar( uint32, nCreditsBonus );
	CNetworkVar( uint32, nPlayerDeaths );
	CNetworkVar( uint32, nBuyBacks );
	CNetworkVar( uint32, nAttempts );
};

//-----------------------------------------------------------------------------
// Stats for a wave
struct CMannVsMachineLocalWaveStats
{
	CMannVsMachineLocalWaveStats()
	{
		nCreditsDropped = 0;
		nCreditsAcquired = 0;
		nCreditsBonus = 0;
		nPlayerDeaths = 0;
		nBuyBacks = 0;
		nAttempts = 0;
	}
	
	CMannVsMachineLocalWaveStats( const CMannVsMachineLocalWaveStats &rhs )
	{
		nCreditsDropped = rhs.nCreditsDropped;
		nCreditsAcquired = rhs.nCreditsAcquired;
		nCreditsBonus = rhs.nCreditsBonus;
		nPlayerDeaths = rhs.nPlayerDeaths;
		nBuyBacks = rhs.nBuyBacks;
		nAttempts = rhs.nAttempts;
	}

	CMannVsMachineLocalWaveStats operator=( const CMannVsMachineLocalWaveStats &rhs )
	{
		nCreditsDropped = rhs.nCreditsDropped;
		nCreditsAcquired = rhs.nCreditsAcquired;
		nCreditsBonus = rhs.nCreditsBonus;
		nPlayerDeaths = rhs.nPlayerDeaths;
		nBuyBacks = rhs.nBuyBacks;
		nAttempts = rhs.nAttempts;
		return *this;
	}

	CMannVsMachineLocalWaveStats operator=( const CMannVsMachineWaveStats &rhs )
	{
		nCreditsDropped = rhs.nCreditsDropped;
		nCreditsAcquired = rhs.nCreditsAcquired;
		nCreditsBonus = rhs.nCreditsBonus;
		nPlayerDeaths = rhs.nPlayerDeaths;
		nBuyBacks = rhs.nBuyBacks;
		nAttempts = rhs.nAttempts;
		return *this;
	}

	void operator+=( const CMannVsMachineWaveStats &rhs )
	{
		nCreditsDropped += rhs.nCreditsDropped;
		nCreditsAcquired += rhs.nCreditsAcquired;
		nCreditsBonus += rhs.nCreditsBonus;
		nPlayerDeaths += rhs.nPlayerDeaths;
		nBuyBacks += rhs.nBuyBacks;
		nAttempts += rhs.nAttempts;
	}

	uint32 nCreditsDropped;
	uint32 nCreditsAcquired;
	uint32 nCreditsBonus;
	uint32 nPlayerDeaths;
	uint32 nBuyBacks;
	uint32 nAttempts;
};

//-----------------------------------------------------------------------------
// Player stats for a wave
struct CMannVsMachineUpgradeEvent
{
	uint16	nItemDef;
	uint16	nAttributeDef;
	uint16	nQuality;
};
typedef CUtlVector< CMannVsMachineUpgradeEvent > tMVMUpgradesVector;

//-----------------------------------------------------------------------------
struct CMannVsMachinePlayerStats
{
	CMannVsMachinePlayerStats()
		: nDeaths( 0 )
		, nBotDamage( 0 )
		, nGiantDamage( 0 )
		, nTankDamage( 0 )
	{
	}

	CMannVsMachinePlayerStats( const CMannVsMachinePlayerStats &rhs )
	{
		nDeaths = rhs.nDeaths;
		nBotDamage = rhs.nBotDamage;
		nGiantDamage = rhs.nGiantDamage;
		nTankDamage = rhs.nTankDamage;
	}

	uint32 nDeaths;	
	uint32 nBotDamage;
	uint32 nGiantDamage;
	uint32 nTankDamage;
};

//-----------------------------------------------------------------------------
struct CPlayerWaveSpendingStats
{
	CPlayerWaveSpendingStats()
		: nCreditsSpentOnBuyBacks( 0 )
		, nCreditsSpentOnBottles( 0 )
		, nCreditsSpentOnUpgrades ( 0 )
	{
	}

	CPlayerWaveSpendingStats( const CPlayerWaveSpendingStats &rhs )
	{
		nCreditsSpentOnBuyBacks = rhs.nCreditsSpentOnBuyBacks;
		nCreditsSpentOnBottles = rhs.nCreditsSpentOnBottles;
		nCreditsSpentOnUpgrades = rhs.nCreditsSpentOnUpgrades;
	}

	CPlayerWaveSpendingStats operator=( const CPlayerWaveSpendingStats &rhs )
	{
		nCreditsSpentOnBuyBacks = rhs.nCreditsSpentOnBuyBacks;
		nCreditsSpentOnBottles = rhs.nCreditsSpentOnBottles;
		nCreditsSpentOnUpgrades = rhs.nCreditsSpentOnUpgrades;
		return *this;
	}

	void operator+=( const CPlayerWaveSpendingStats &rhs )
	{
		nCreditsSpentOnBuyBacks += rhs.nCreditsSpentOnBuyBacks;
		nCreditsSpentOnBottles += rhs.nCreditsSpentOnBottles;
		nCreditsSpentOnUpgrades += rhs.nCreditsSpentOnUpgrades;
	}

	uint32 nCreditsSpentOnBuyBacks;
	uint32 nCreditsSpentOnBottles;
	uint32 nCreditsSpentOnUpgrades;		// Bottles are NOT upgrades in this list
};
//-----------------------------------------------------------------------------

// get the current wave
uint32 MannVsMachineStats_GetCurrentWave();

// Reporting currency stats for game code
uint32 MannVsMachineStats_GetAcquiredCredits( int idxWave = -1, bool bIncludeBonus = true );
uint32 MannVsMachineStats_GetDroppedCredits( int idxWave = -1 );
uint32 MannVsMachineStats_GetMissedCredits( int idxWave = -1 );

#ifdef GAME_DLL 

// initialize the stats for Mann Vs Machine
void MannVsMachineStats_Init();

struct edict_t;

// Reset the player events associated with the player, such as when they disconnect
void MannVsMachineStats_ResetPlayerEvents( CTFPlayer *pTFPlayer );

// Round events
void MannVsMachineStats_RoundEvent_CreditsDropped( uint32 waveIdx, int nAmount );

// Player events
void MannVsMachineStats_PlayerEvent_PointsChanged( CTFPlayer *pTFPlayer, int nPoints );
void MannVsMachineStats_PlayerEvent_Died( CTFPlayer *pTFPlayer );
void MannVsMachineStats_PlayerEvent_Upgraded( CTFPlayer *pTFPlayer, uint16 nItemDef, uint16 nAttributeDef, uint16 nQuality, int16 nCost, bool bIsBottle );
void MannVsMachineStats_PlayerEvent_PickedUpCredits( CTFPlayer *pTFPlayer, uint32 idxWave, int nCreditsAmount );
void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn( CTFPlayer *pTFPlayer, int nCost );

void MannVsMachineStats_SetPopulationFile( const char * pPopulationFile );
#endif // GAME_DLL

struct CAllPlayerSpendingStats
{
	CPlayerWaveSpendingStats m_playerStats[MAX_PLAYERS_ARRAY_SAFE];
};

//-----------------------------------------------------------------------------
// Container class for all the mann vs machine stats we track for a round and for a player
class CMannVsMachineStats : public CBaseEntity
{
	DECLARE_CLASS( CMannVsMachineStats, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();

	CMannVsMachineStats();
	virtual ~CMannVsMachineStats();

	uint32 GetCurrentWave() const { return m_iCurrentWaveIdx; }

	// CBaseEntity
	virtual	int ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	void ResetStats( );
	void ResetPlayerEvents( CTFPlayer *pTFPlayer );
	void ResetUpgradeSpending( CTFPlayer *pTFPlayer );

	// Dropped, Acquired stats
	uint32 GetAcquiredCredits( int iWaveIdx, bool bWithBonus = true );
	uint32 GetDroppedCredits( int iWaveIdx );
	uint32 GetMissedCredits( int iWaveIdx );
	uint32 GetBonusCredits ( int iWaveIdx );

#ifdef GAME_DLL
	virtual int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }

	void SetMapName ( const char *pMapName ) { m_pMapName = pMapName; }
	void SetPopFile ( const char *pPopFile ) { m_pPopFileName = pPopFile; }

	// Call when the round is over
	void RoundOver( bool bHumansWon );

	// Set the current wave that will be updated
	void SetCurrentWave( uint32 idxWave );

	// Round events.
	void RoundEvent_WaveStart();
	void RoundEvent_WaveEnd( bool bSuccess );
	void RoundEvent_AcquiredCredits( uint32 idxWave, int nAmount, bool bIsBonus );
	void RoundEvent_CreditsDropped( uint32 waveIdx, int nAmount );

	// player events
	void PlayerEvent_PointsChanged( CTFPlayer *pTFPlayer, int nPoints );
	void PlayerEvent_Died( CTFPlayer *pTFPlayer );
	void PlayerEvent_Upgraded( CTFPlayer *pTFPlayer, uint16 nItemDef, uint16 nAttributeDef, uint8 nQuality, int16 nCost, bool bIsBottle );
	void PlayerEvent_PickedUpCredits( CTFPlayer *pTFPlayer, uint32 idxWave, int nCreditsAmount );
	void PlayerEvent_BoughtInstantRespawn( CTFPlayer *pTFPlayer, int nCost );

	void PlayerEvent_DealtDamageToBots( CTFPlayer *pTFPlayer, int damage );
	void PlayerEvent_DealtDamageToGiants( CTFPlayer *pTFPlayer, int damage );
	void PlayerEvent_DealtDamageToTanks( CTFPlayer *pTFPlayer, int damage );

	// send a user message to clients so that they can record what's going on a per player basis
	void NotifyPlayerEvent( CTFPlayer *pTFPlayer, uint32 idxWave, eMannVsMachineEvent eType, int nValue, int nParam = 0 );
	void NotifyTargetPlayerEvent( CTFPlayer *pTFPlayer, uint32 idxWave, eMannVsMachineEvent eType, int nCost );

	void SendUpgradesToPlayer( CTFPlayer *pTFPlayer, CUtlVector< CUpgradeInfo > *upgrades );

	void NotifyPlayerActiveUpgradeCosts( CTFPlayer *pTFPlayer, int nSpending );

#endif // GAME_DLL

	// Shared stats
	void ClearCurrentPlayerWaveSpendingStats( int idxWave );
	CPlayerWaveSpendingStats *GetSpending( int iWaveIndex, uint64 steamId );
	int GetUpgradeSpending( CTFPlayer *pTFPlayer = NULL );
	int GetBottleSpending( CTFPlayer *pTFPlayer = NULL );
	int GetBuyBackSpending( CTFPlayer *pTFPlayer = NULL );

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t updateType );

	// Message from Server about Client Upgrades
	void ClearLocalPlayerUpgrades ();
	void AddLocalPlayerUpgrade( int iPlayerClass, item_definition_index_t iItemDef );
	
	int GetLocalPlayerUpgradeSpending( int idxWave );
	int GetLocalPlayerBottleSpending( int idxWave );
	int GetLocalPlayerBuyBackSpending ( int idxWave );

	// Client Side Reporting
	void SW_ReportClientUpgradePurchase( uint8 waveIdx, uint16 nItemDef, uint16 nAttributeDef, uint8 nQuality, int16 nCost );
	void SW_ReportClientBuyBackPurchase( uint8 waveIdx, uint16 nCost );
	void SW_ReportClientWaveSummary( uint16 serverWaveID, CMannVsMachinePlayerStats stats );

	CUtlVector< CUpgradeInfo > *GetLocalPlayerUpgrades() { return &m_vecLocalPlayerUpgrades; }
	CPlayerWaveSpendingStats *GetLocalSpending ( int iWaveIdx );		// Helper
	
	void SetPlayerActiveUpgradeCosts( uint64 playerId, int nSpending );
	int GetPlayerActiveUpgradeCosts( uint64 playerId );

#endif // CLIENT_DLL

	// Respec
	uint16 GetNumRespecsEarnedInWave( void ) { return m_nRespecsAwardedInWave; }
	uint32 GetAcquiredCreditsForRespec( void ) { return m_iCurrencyCollectedForRespec; }
#ifdef GAME_DLL
	void SetNumRespecsEarnedInWave( uint16 nNum ) { m_nRespecsAwardedInWave = nNum; }
	void SetAcquiredCreditsForRespec( uint16 nNum ) { m_iCurrencyCollectedForRespec = nNum; }
#endif // GAME_DLL

private:
	// helper
	CMannVsMachineLocalWaveStats GetWaveStats( int iWaveIdx );

	void OnStatsChanged();

#ifdef GAME_DLL

	void ResetWaveStats();

	// Submitting Data to OGS
	void SW_ReportWaveSummary ( int waveIndex, bool bIsSuccess );

	CMannVsMachinePlayerStats m_playerStats[MAX_PLAYERS_ARRAY_SAFE];

	const char *m_pPopFileName;
	const char *m_pMapName;

#endif // GAME_DLL
	
	CMannVsMachineWaveStats m_runningTotalWaveStats;
	CMannVsMachineWaveStats m_previousWaveStats;
	CMannVsMachineWaveStats m_currentWaveStats;

	CNetworkVar( uint32, m_iCurrentWaveIdx );
	CNetworkVar( uint32, m_iServerWaveID );

	// Shared stats
	CUtlMap< uint64, CPlayerWaveSpendingStats > m_currWaveSpendingStats;
	CUtlMap< uint64, CPlayerWaveSpendingStats > m_prevWaveSpendingStats;		// CurrWave - 1
	CUtlMap< uint64, CPlayerWaveSpendingStats > m_allPrevWaveSpendingStats;		// Total of all previous Waves

	// Respec
	CNetworkVar( uint32, m_iCurrencyCollectedForRespec );	// Tracks total money collected, regardless of wave status
	CNetworkVar( uint16, m_nRespecsAwardedInWave );

#ifdef CLIENT_DLL
	CUtlVector< CUpgradeInfo > m_vecLocalPlayerUpgrades;

	CUtlMap< uint64, int > m_teamActiveUpgrades;
#endif
};

CMannVsMachineStats *MannVsMachineStats_GetInstance();

#endif // TF_MANN_VS_MACHINE_STATS_H
