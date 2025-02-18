//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_QUEST_MAP_NODE_H
#define TF_QUEST_MAP_NODE_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#include "tf_quest_map_utils.h"
#include <google/protobuf/text_format.h>
#include "tf_proto_script_obj_def.h"

#if defined (CLIENT_DLL) || defined (GAME_DLL)
	#include "gc_clientsystem.h"
#endif


enum EEdgeDir
{
	CONNECTS_TO,
	CONNECTS_FROM,
};

struct Edges_t
{
	const CQuestMapNodeDefinition* m_pNode;
	EEdgeDir m_eConnection;
};

typedef CUtlVector< Edges_t > EdgeVec_t;

typedef CTypedProtoBufScriptObjectDefinition< CMsgQuestMapStarType, DEF_TYPE_QUEST_MAP_STAR_TYPE > CQuestMapStarType;

//
//	Nodes are points on a map.  They have conditions within them that determine
//	if they can be activated or not.  Once activated, they wait until something
//	marks them as completed.
//
//	On the client they describe their position and visuals on the map.
//
class CQuestMapNodeDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgQuestMapNodeDef, DEF_TYPE_QUEST_MAP_NODE >
{
public:

	CQuestMapNodeDefinition();
	virtual ~CQuestMapNodeDefinition();

	virtual const char* GetDisplayName() const OVERRIDE { return GetNameLocToken(); }

	bool BHasPassToUnlock( const CQuestMapHelper& helper ) const;
	bool BHasRequiredNodesCompltedToUnlock( const CQuestMapHelper& helper ) const;
	bool BHasStarsToUnlock( const CQuestMapHelper&  helper) const;
	bool BCanUnlock( const CQuestMapHelper& helper ) const;
	bool BIsActive() const; // As in, all offered quests are available
	
#ifdef CLIENT_DLL
	void GetCantUnlockReason( wchar_t* out_wszRequirements = NULL, int nRequirementsSize = 0 ) const;
	void GetConnectedNodes( CUtlMap< uint32, bool >& mapOutConnectedNodeCondition ) const;
	const char* GetIconName() const;
	bool GetRewardItem( CEconItemView& item ) const;
#endif
	void GetOfferedQuests( CUtlVector< const CQuestDefinition* >& vecOutQuestDefs ) const;
	bool BIsMedalOffered( EQuestPoints eType ) const; 

	const char* GetNameLocToken() const;
	float GetXPos() const					{ return m_msgData.x_pos(); }
	float GetYPos() const					{ return m_msgData.y_pos(); }

	// The region we live in
	uint32 GetRegionDefIndex() const		{ return m_msgData.owning_region().defindex(); }

	// Offered quests.  Nodes have a few potential quests that users can select from.
	uint32 GetNumOfferedQuests() const		{ return m_msgData.quest_options_size(); }
	const CQuestDefinition* GetOfferedQuest( uint32 nIndex ) const;
	const char* GetRewardItem() const		{ return m_msgData.has_reward_item_name()		? m_msgData.reward_item_name().c_str()			: NULL; }
	const char* GetRewardLootListName() const { return m_msgData.has_reward_lootlist_name() ? m_msgData.reward_lootlist_name().c_str()	: NULL; }
	int GetRewardCredits() const;
	ENodeCashReward GetCashRewardType() const { return m_msgData.cash_reward(); }
	const CEconOperationDefinition* GetAssociatedOperation() const;
	const EdgeVec_t& GetLinkedNodes() const { return m_vecConnectedNodes; }
	uint32 GetStarType() const;
	const CQuestMapStarType* GetStarTypeDef() const;
	uint32 GetNumStarsToUnlock() const { return m_msgData.stars_to_unlock(); }
private:
	bool BConditionsMet_Recursive( const CMsgQuestMapNodeCondition& msgCondition
								 , const MapQuestNodes_t& nodes
								 , CUtlMap< uint32, bool >* pMapOutConnectedNodeCondition ) const;

	virtual bool BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;
	void EvaluateLinks();

#ifdef CLIENT_DLL
	bool m_bEvaluatingLinks = false;
#endif
	EdgeVec_t m_vecConnectedNodes;
	const CEconOperationDefinition* m_pAssociatedOperationDef;
};


//---------------------------------------------------------------------------------
// Purpose: The shared object that contains data for a user's quest map
//---------------------------------------------------------------------------------
class CQuestMapNode : public GCSDK::CProtoBufSharedObject< CSOQuestMapNode, k_EEconTypeQuestMapNode >
{
public:
	CQuestMapNode();

	uint64 GetID() const { return Obj().node_id(); }
	uint32 GetBonusesCompleted() const;
	bool BAllMedalsEarned() const;
	bool BIsMedalOffered( EQuestPoints eType ) const; 
	bool BIsMedalEarned( EQuestPoints eType ) const;
	bool BHasLootBeenClaimed() const { return Obj().loot_claimed(); }
	bool BCanRedeemLoot() const;
	const CQuestMapNodeDefinition* GetNodeDefinition() const;
	const CQuestDefinition* GetSelectedQuest() const;

#ifdef CLIENT_DLL
	bool BCanClaimAnyLoot() const;
	const char* GetIconName() const;
#else
	bool BCanClaimAnyLoot( const CQuest* pQuest ) const;
#endif

	const CEconOperationDefinition* GetAssociatedOperation() const { return ( GetNodeDefinition() ? GetNodeDefinition()->GetAssociatedOperation() : NULL ); }
};


#endif // TF_QUEST_MAP_NODE_H
