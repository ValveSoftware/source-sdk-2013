//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_TF_GAMESTATS_H
#define C_TF_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "gamestats.h"
#include "tf_gamestats_shared.h"
#include "GameEventListener.h"
#include "c_tf_player.h"
#include "econ_store.h"
#include "store/store_panel.h"

class CTFPlayer;

struct TF_Gamestats_ClientSession_t
{
public:

	TF_Gamestats_ClientSession_t();

private:
	TF_Gamestats_ClientSession_t( const TF_Gamestats_ClientSession_t &stats ) {}

public:
	void Reset();

	struct SessionSummary_t
	{
		int iClassesPlayed;
		int iMapsPlayed;
		int iRoundsPlayed;
		int iFavoriteClass;
		int iFavoriteWeapon;
		char szFavoriteMap[64];
		int	iKills;
		int iDeaths;
		int iSuicides;
		int iAssists;
		int	iBuildingsBuilt;
		int iBuildingsUpgraded;
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
		int iAchievementsEarned;
	};

	SessionSummary_t			m_Summary;

	RTime32						m_SessionStart;
	RTime32						m_FirstConnect;
	int							m_iMapsPlayed;
	int							m_iRoundsPlayed;
	CBitVecT< CFixedBitVecBase<32> >	m_ClassesPlayed;
};

struct TF_Gamestats_WeaponInfo_t
{
	TF_Gamestats_WeaponInfo_t();

	int weaponID;
	int critsFired;
	int shotsFired;
	int shotsHit;
	int shotsMissed;
	int avgDamage;
	int totalDamage;
	int critHits;
	float lastUpdateTime;
};

struct TF_Gamestats_AchievementEvent_t
{
	TF_Gamestats_AchievementEvent_t( int in_achievementNum, const char* in_achievementID );

	int		eventTime;
	int		achievementNum;
	const char* achievementID;
};

// Item event baseclass.
class TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_ItemEvent( int in_eventNum, CEconItemView* in_item );

	int		eventNum;
	int		eventTime;
	const char* eventID;

	item_definition_index_t itemDefIndex;
	itemid_t itemID;
	const char* itemName;
	char itemNameBuf[512];
	bool bUseNameBuf;

	const char* GetItemName()
	{
		if ( bUseNameBuf )
			return itemNameBuf;
		else
			return itemName;
	}
};

// Mann Co Catalog Usage Tracking
class TF_Gamestats_CatalogEvent : public TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_CatalogEvent( int in_eventNum, CEconItemView* in_item, const char* in_filter );

	const char* catalogFilter;
};

// Crafting System Usage Tracking
class TF_Gamestats_CraftingEvent : public TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_CraftingEvent( int in_eventNum, CEconItemView* in_item, int in_numAttempts, int in_recipe );

	int		numAttempts;
	int		recipeFound;
};

// Store Usage Tracking
class TF_Gamestats_StoreEvent : public TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_StoreEvent( int in_eventNum, CEconItemView* in_item, 
		const char* in_panelName, int in_classId, const cart_item_t* in_cartItem,
		int in_checkoutAttempts, const char* in_storeError, int in_totalPrice, int in_currencyCode );

	int classId;
	int cartQuantity;
	int cartItemCost;
	int currencyCode;
	int checkoutAttempt;
	const char* storeError;
	const char* panelName;
};

// General client-subjective item transaction tracking.
class TF_Gamestats_ItemTransactionEvent : public TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_ItemTransactionEvent( int in_eventNum, CEconItemView* in_item, const char* in_reason, int in_quality );

	const char* reason;
	int itemQuality;
};

// Trade Usage Tracking
class TF_Gamestats_TradeEvent : public TF_Gamestats_ItemEvent
{
public:
	TF_Gamestats_TradeEvent( int eventID, CEconItemView* item, bool localPlayerIsPartyA,
		uint64 steamIDPartyA, uint64 steamIDPartyB, int iTradeRequests, int iTradeAttempts );
	TF_Gamestats_TradeEvent( int eventID, uint64 steamIDRequested, int iTradeRequests, int iTradeAttempts );
	TF_Gamestats_TradeEvent( int eventID, int iTradeRequests, const char* reason, int iTradeAttempts );

	bool localPlayerPartyMatters;
	bool localPlayerIsPartyA;
	uint64 steamIDPartyA;
	uint64 steamIDPartyB;

	uint64 steamIDRequested;
	int tradeRequests;
	int tradeAttempts;

	const char* reason;
};

// Matchmaking stats
struct TF_Gamestats_QuickPlay_t
{
	// Status code for the search as a whole
	enum eResult
	{
		k_Result_InternalError = -1,
		k_Result_UserCancel = 10,
		k_Result_NoServersFound = 20,
		k_Result_NoServersMetCrtieria = 30,
		//k_Result_NeverHeardBackFromGC = 40,
		//k_Result_ReceivedZeroGCScores = 50,
		k_Result_FinalPingFailed = 60,
		k_Result_TriedToConnect = 100,
	};

	// Status codes for the servers
	enum eServerStatus
	{
		k_Server_Invalid = -1, // we have a bug if this gets reported
		k_Server_Ineligible = 10,
		k_Server_Eligible = 20,
		k_Server_RequestedScore = 30,
		k_Server_Scored = 40,
		k_Server_Pinged = 50,
		k_Server_PingTimedOut = 60,
		k_Server_PingIneligible = 70,
		k_Server_Connected = 100,
	};

	TF_Gamestats_QuickPlay_t()
	{
		m_fUserHoursPlayed = -1.0f;
		m_sUserGameMode;
		m_fSearchTime = -1.0;
		m_eResultCode = k_Result_UserCancel;
		m_iExperimentGroup = 0;
	}

	float m_fUserHoursPlayed;
	CUtlString m_sUserGameMode;
	float m_fSearchTime;
	eResult m_eResultCode;
	int m_iExperimentGroup; // TF2ScoringNumbers_t::ExperimentGroup_t

	struct Server_t
	{
		uint32 m_ip;
		uint16 m_port;
		bool m_bRegistered;
		bool m_bValve;
		bool m_bSecure;
		bool m_bMapIsNewUserFriendly;
		bool m_bMapIsQuickPlayOK;
		int m_nPlayers;
		int m_nMaxPlayers;
		CUtlString m_sMapName;
		CUtlString m_sTags;
		int m_iPing;
		float m_fScoreClient;
		float m_fScoreServer;
		float m_fScoreGC;
		eServerStatus m_eStatus;
	};
	CUtlVector<Server_t> m_vecServers;
};

//=============================================================================
//
// TF Game Stats Class
//

class C_CTFGameStats : public CBaseGameStats, public CGameEventListener, public CAutoGameSystem
{
public:

	// Constructor/Destructor.
	C_CTFGameStats( void );
	~C_CTFGameStats( void );

	virtual void Clear( void );

	virtual bool UseOldFormat() { return false; }
	virtual bool AddDataForSend( KeyValues *pKV, StatSendType_t sendType );

	virtual bool Init();
	virtual void Shutdown();

	void ResetRoundStats();

	void ClientDisconnect( int iReason );

	// Events.
	virtual void Event_LevelInit( void );
	virtual void Event_LevelShutdown( float flElapsed );
	virtual void Event_RoundActive();
	virtual void Event_RoundEnd( int winningTeam, float roundTime, int fullRound );
	virtual void Event_PlayerChangeClass( int userid, int classid );
	virtual void Event_AchievementProgress( int achievementID, const char* achievementName );
	virtual void Event_PlayerHurt( IGameEvent* event /*player_hurt*/ );
	virtual void Event_PlayerFiredWeapon( C_TFPlayer *pPlayer, bool bCritical );
	virtual void Event_Catalog( int eventID, const char* filter=NULL, CEconItemView* item=NULL );
	virtual void Event_Crafting( int eventID, CEconItemView* item=NULL, int numAttempts=0, int recipeFound=0 );
	virtual void Event_Store( int eventID, CEconItemView* item=NULL, const char* panelName=NULL, 
		int classId=0, const cart_item_t* in_cartItem=NULL, int in_checkoutAttempts=0, const char* storeError=NULL, int in_totalPrice=0, int in_currencyCode=0 );
	virtual void Event_ItemTransaction( int eventID, CEconItemView* item, const char* pszReason=NULL, int iQuality=0 );
	virtual void Event_Trading( int eventID, CEconItemView* item=NULL, bool localPlayerIsPartyA=false,
		uint64 steamIDPartyA=0, uint64 steamIDPartyB=0, int iTradeRequests=0, int iTradeAttempts=0 );
	virtual void Event_Trading( int eventID, uint64 steamIDRequested=0, int iTradeRequests=0, int iTradeAttempts=0 );
	virtual void Event_Trading( int eventID, int iTradeRequests=0, const char* reason=NULL, int iTradeAttempts=0 );
	virtual void Event_Trading( TF_Gamestats_TradeEvent& event );

	virtual void FireGameEvent( IGameEvent * event );

	void SW_GameStats_WriteClientSessionSummary();
	void SW_GameStats_WriteClientWeapons();
	void SW_GameStats_WriteClientRound( int winningTeam, int fullRound, int endReason );
	void SW_GameStats_WriteClientMap();

	void SetExperimentValue( uint64 experimentValue ) { m_ulExperimentValue = experimentValue; }

	static void ImmediateWriteInterfaceEvent( const char *pszEventType, const char *pszEventDesc );

	/*
	void SW_GameStats_WriteClientAchievements();
	void SW_GameStats_WriteClientCatalogEvents();
	void SW_GameStats_WriteClientCraftingEvents();
	void SW_GameStats_WriteClientStoreEvents();
	void SW_GameStats_WriteClientItemTransactionEvents();
	void SW_GameStats_WriteClientTradeEvents();
	*/

	void QuickplayResults( const TF_Gamestats_QuickPlay_t &info );

private:
	char	m_szCountryCode[64];
	char	m_szAudioLanguage[64];
	char	m_szTextLanguage[64];

	TF_Gamestats_ClientSession_t	m_currentSession;
	TF_Gamestats_RoundStats_t		m_currentRound;
	TF_Gamestats_LevelStats_t		m_currentMap;
	CUtlVector<TF_Gamestats_AchievementEvent_t>	m_vecAchievementEvents;
	CUtlMap<int, TF_Gamestats_WeaponInfo_t>	m_mapWeaponInfo;

	CUtlVector<TF_Gamestats_CatalogEvent> m_vecCatalogEvents;
	CUtlVector<TF_Gamestats_CraftingEvent> m_vecCraftingEvents;
	CUtlVector<TF_Gamestats_StoreEvent> m_vecStoreEvents;
	CUtlVector<TF_Gamestats_ItemTransactionEvent> m_vecItemTransactionEvents;
	CUtlVector<TF_Gamestats_TradeEvent> m_vecTradeEvents;

	uint64 m_ulExperimentValue;

	bool m_bRoundActive;
	bool m_bIsDisconnecting;
};

extern C_CTFGameStats C_CTF_GameStats;

#endif // C_TF_GAMESTATS_H
