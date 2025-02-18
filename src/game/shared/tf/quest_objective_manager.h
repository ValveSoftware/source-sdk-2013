//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef QUEST_OBJECTIVE_MANAGER_H
#define QUEST_OBJECTIVE_MANAGER_H

#include "GameEventListener.h"
#include "econ_item_constants.h"
#include "econ_item_inventory.h"
#include "tf_quest_restriction.h"
#include "econ_dynamic_recipe.h"
#include "shared_object_tracker.h"
#include "econ_quests.h"

#ifdef GAME_DLL
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
#endif


#if defined( _WIN32 )
#pragma once
#endif

using namespace GCSDK;


class CQuestItemTracker;
class CQuest;
class CQuestObjectiveDefinition;

class CBaseQuestObjectiveTracker : public CTFQuestEvaluator
{
public:
	DECLARE_CLASS( CBaseQuestObjectiveTracker, CBaseQuestObjectiveTracker )

	CBaseQuestObjectiveTracker( const QuestObjectiveInstance_t& objectiveInstance, CQuestItemTracker* pParent, const CSteamID& ownerSteamID );
	virtual ~CBaseQuestObjectiveTracker();

	uint32 GetObjectiveDefIndex() const { return m_objectiveInstance.GetObjectiveDef()->GetDefIndex(); }
	const QuestObjectiveInstance_t& GetObjectiveInstance() const { return m_objectiveInstance; }

	// CTFQuestConditionEvaluator specific
	virtual const char *GetConditionName() const OVERRIDE { return "tracker"; }
	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const;
	virtual const CTFPlayer *GetQuestOwner() const OVERRIDE;
	virtual void EvaluateCondition( CTFQuestEvaluator *pSender, int nScore ) OVERRIDE;
	virtual void ResetCondition() OVERRIDE;
	// The steamID of the guy we're tracking for this objective.  This might not
	// be the owner of the quest due to party shared progress
	const CSteamID& GetTrackingPlayer() const { return m_steamIDOwner; }

	bool UpdateConditions();

protected:
	const CTFPlayer* GetQuestOwningPlayer() const;
	void IncrementCount( int nIncrementValue );

	QuestObjectiveInstance_t m_objectiveInstance;

private:
	CTFQuestEvaluator *m_pEvaluator;
	CQuestItemTracker *m_pParent;
	CSteamID m_steamIDOwner;
};


class CQuestItemTracker : public CBaseSOTracker, public CGameEventListener
#ifdef GAME_DLL
	, public ISharedObjectListener
#endif
{
public:
	CQuestItemTracker( const CSharedObject* pItem, CSteamID SteamIDOwner, CSOTrackerManager* pManager );
	~CQuestItemTracker();

	virtual void OnUpdate() OVERRIDE;
	virtual void OnRemove() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	void UpdatePointsFromSOItem();

	const CBaseQuestObjectiveTracker* FindTrackerForDefIndex( uint32 nDefIndex ) const;
	inline const CUtlVector< const CBaseQuestObjectiveTracker* >& GetObjectiveTrackers() const { return m_vecObjectiveTrackers; }

	uint32 GetEarnedPoints( uint32 eType ) const;
	const CQuest* GetItem() const { return m_pQuest; }

	void IncrementCount( uint32 nIncrementValue, const QuestObjectiveInstance_t& objective, const CSteamID& steamIDScorer );
	virtual void CommitChangesToDB() OVERRIDE;

	int GetNumInactiveObjectives( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const;

#ifdef CLIENT_DLL
	void UpdateFromServer( uint32 nPoints0,
						   uint32 nPoints1,
						   uint32 nPoints2 );
#else
	void SendUpdateToClient( const CQuestObjectiveDefinition* pObjective, const CSteamID& steamIDScorer );
#endif

	virtual void Spew() const OVERRIDE;

private:

	void EnsureObjectiveTrackersForPlayer( const CSteamID& steamIDTrackingPlayer );
#ifdef GAME_DLL
	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};

	// The only one we care about
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;

	void UpdatePartyProgressPreference();
	void EnsureObjectiveTrackersForParty();
#endif

	void PruneUnneededObjectiveTrackers();
	bool DoesObjectiveNeedToBeTracked( const QuestObjectiveInstance_t& objective) const;

#ifdef GAME_DLL
	uint32 m_nStartingPoints[ EQuestPoints_ARRAYSIZE ];
	bool m_bHasUnneededObjectiveTrackers;
	CUtlMap< CSteamID, uint32 > m_mapQuestAssisters;
#endif
	uint32 m_nPoints[ EQuestPoints_ARRAYSIZE ];

	const CQuest* m_pQuest;
	
	CUtlVector< const CBaseQuestObjectiveTracker* > m_vecObjectiveTrackers;

	bool m_bObjectivesOnlyForOwner;
};

// A class to handle the creation and deletion of quest objective trackers. Automatically
// subscribes to the local player's SOCache and will subscribe to any connecting players'
// SOCaches when they connect.
class CQuestObjectiveManager : public CSOTrackerManager
{
public:
	DECLARE_CLASS( CQuestObjectiveManager, CSOTrackerManager )

	CQuestObjectiveManager();
	virtual ~CQuestObjectiveManager();

	virtual SOTrackerMap_t::KeyType_t GetKeyForObjectTracker( const CSharedObject* pItem, CSteamID steamIDOwner ) OVERRIDE;

#ifdef CLIENT_DLL
	void UpdateFromServer( itemid_t nID, uint32 nPoints0, uint32 nPoints1, uint32 nPoints2 );
#endif

private:
#ifdef GAME_DLL
	void SendMessageForCommit( const ::google::protobuf::Message* pProtoMessage ) const;
#endif

	virtual int GetType() const OVERRIDE;
	virtual const char* GetName() const { return "QuestObjectiveManager"; }
	virtual CFmtStr GetDebugObjectDescription( const CSharedObject* pItem ) const;
	virtual CBaseSOTracker* AllocateNewTracker( const CSharedObject* pItem, CSteamID steamIDOwner, CSOTrackerManager* pManager ) const OVERRIDE;
	virtual ::google::protobuf::Message* AllocateNewProtoMessage() const OVERRIDE;
	virtual void OnCommitRecieved( const ::google::protobuf::Message* pProtoMsg ) OVERRIDE;
	virtual bool ShouldTrackObject( const CSteamID & steamIDOwner, const CSharedObject *pObject ) const OVERRIDE;
	virtual int CompareRecords( const ::google::protobuf::Message* pNewProtoMsg, const ::google::protobuf::Message* pExistingProtoMsg ) const OVERRIDE;
};

CQuestObjectiveManager* QuestObjectiveManager();

#endif // QUEST_OBJECTIVE_MANAGER_H
