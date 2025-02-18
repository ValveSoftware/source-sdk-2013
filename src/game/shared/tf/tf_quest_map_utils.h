//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_QUEST_MAP_UTILS_H
#define TF_QUEST_MAP_UTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#include "econ_quests.h"
#if defined (CLIENT_DLL)
	#include "gc_clientsystem.h"
	#include "local_steam_shared_object_listener.h"
#endif


class CQuestMapNode;

// I dont want to duplicate the accessors for the same functions on client+server/gc where just the constness changes
	#define QUESTHELPER_CONSTNESS const

typedef CUtlMap< uint32, QUESTHELPER_CONSTNESS CQuestMapNode* > MapQuestNodes_t;

class CQuestMapHelper
#ifdef CLIENT_DLL
	: public CLocalSteamSharedObjectListener
#endif
{
public:
	CQuestMapHelper( CSteamID steamID );
#ifdef CLIENT_DLL
	CQuestMapHelper();

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	void SOEvent( const GCSDK::CSharedObject *pObject );

	bool BIsNodeConnectionFulfilled( const CQuestMapNodeDefinition* pNode1, const CQuestMapNodeDefinition* pNode2 ) const;
	bool BRegionHasAvailableContracts( uint32 nRegionDefIndex ) const;
#endif
	CSteamID GetOwnerSteamID() const { return m_steamID; }
	const CQuest* GetActiveQuest() const;
	
	uint32 GetNumCollectedMedals( EQuestPoints eType ) const { return m_nCollectedMedals[ eType ]; }
	uint32 GetNumTotalMedals( EQuestPoints eType ) const { return m_nTotalMedals[ eType ]; }
#ifndef GAME_DLL
	int GetNumRewardCredits() const;
#endif // GAME_DLL
	int GetNumCurrentlyUnlockableNodes() const { return m_nLockedAndRequirementsMetNodes; }
	int GetNumUnlockedNodes() const { return m_nNumUnlockedNodes; }
	int GetNumStarsAvailableToSpend( uint32 nTypeDefindex ) const;
	int GetNumStarsEarned( uint32 nTypeDefindex ) const;
	int GetNumStarsTotal( uint32 nTypeDefindex ) const;
	bool BCanNodeBeTurnedIn( uint32 nNodeDefindex ) const;
	int GetNumRewardPurchases( uint32 nDefindex ) const;

	QUESTHELPER_CONSTNESS CQuestMapNode* GetQuestMapNode( uint32 nNodeDef ) const;
	QUESTHELPER_CONSTNESS CQuestMapNode* GetQuestMapNodeByID( uint32 nNodeDef ) QUESTHELPER_CONSTNESS;
	const MapQuestNodes_t& GetQuestMapNodes() const { return m_mapNodes; }
	QUESTHELPER_CONSTNESS CQuest* GetQuestForNode( uint32 nNodeID ) const;
	QUESTHELPER_CONSTNESS CQuest* GetQuestByID( uint32 nQuestID ) QUESTHELPER_CONSTNESS;
	QUESTHELPER_CONSTNESS CQuest* GetQuestByDefindex( uint32 nDefIndex ) const;
	QUESTHELPER_CONSTNESS CUtlVector< CQuest* >& GetAllQuests() QUESTHELPER_CONSTNESS { return m_vecQuests; }

private:

#ifndef CLIENT_DLL
	// You gotta construct with a steamID on the GC
	CQuestMapHelper() {}
#endif

	void Refresh();

	MapQuestNodes_t m_mapNodes;
	CUtlVector< CQuest* > m_vecQuests;
	CSteamID m_steamID;

#ifdef CLIENT_DLL
	CUtlMap< uint64, bool > m_mapNodeConnections;
	CUtlMap< uint32, bool > m_mapRegionHasAvailableContracts;
#endif

	uint32 m_nTotalMedals[ EQuestPoints_ARRAYSIZE ];
	uint32 m_nCollectedMedals[ EQuestPoints_ARRAYSIZE ];
	int m_nActiveQuestIndex;
	int m_nLockedAndRequirementsMetNodes;
	int m_nNumUnlockedNodes;
	CUtlMap< uint32, int > m_mapStarsAvailableToSpend;
	CUtlMap< uint32, int > m_mapStarsEarned;
	CUtlMap< uint32, int > m_mapStarsTotal;
	CUtlMap< uint32, int > m_mapRewardDefindexPurchases;

#ifndef GAME_DLL
	int m_nRewardCredits;
#endif // GAME_DLL
};

#ifdef CLIENT_DLL
// This global version is for the local client only
const CQuestMapHelper& GetQuestMapHelper();
uint64 GetNodeDefPairKey( uint32 nDefindex1, uint32 nDefIndex2 );
#endif


#endif // TF_QUEST_MAP_UTILS_H
