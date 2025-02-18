//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to quests
//
//=============================================================================

#ifndef ECON_QUESTS
#define ECON_QUESTS
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.h"
#include "tf_quest_constants.h"
#include "gcsdk/protobufsharedobject.h"
#include "tf_proto_script_obj_def.h"

class CTFQuestEvaluator;

const float k_flQuestTurnInTime = 5.f;

struct QuestPointsDef_t
{
	const char* m_pszBarText;
	const char* m_pszObjectiveText;
	const char* m_pszActiveBadgeImage;
	const char* m_pszInactiveBadgeImage;
	const char* m_pszObjectiveCompletedSound;
	const char* m_pszObjectiveCompletedSoundParty;
	const char* m_pszPointsCompletedSound;
	const char* m_pszCompletionText;
	const char* m_pszScoredText;
};

extern QuestPointsDef_t g_QuestPointsDefs[];

class CQuestThemeDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgQuestTheme, DEF_TYPE_QUEST_THEME >
{
public:

	CQuestThemeDefinition( void );
	virtual ~CQuestThemeDefinition( void );

	const char *GetNotificationResFile() const { return m_msgData.notification_res().c_str(); }
	const char *GetQuestItemResFile() const { return m_msgData.quest_item_res().c_str(); }
	const char *GetInGameTrackerResFile() const { return m_msgData.in_game_tracker_res().c_str(); }

	const char *GetGiveSoundForClass( int iClass ) const;
	const char *GetCompleteSoundForClass( int iClass ) const;
	const char *GetFullyCompleteSoundForClass( int iClass ) const;
	const char *GetDiscardSound() const { return UTIL_GetRandomSoundFromEntry( m_msgData.discard_sound().c_str() ); }
	const char *GetRewardSound() const { return UTIL_GetRandomSoundFromEntry( m_msgData.reward_sound().c_str() ); }
	const char *GetRevealSound() const { return UTIL_GetRandomSoundFromEntry( m_msgData.reveal_sound().c_str() ); }

private:
	static const char* GetRandomWeightedString( const CMsgQuestTheme_WeightedStringSet& msgSet );
};

//-----------------------------------------------------------------------------
// CTFQuestObjectiveConditionsDefinition
// These contain the actual logic that can be used by multiple objectives.
//-----------------------------------------------------------------------------
class CQuestObjectiveConditionsDefinition
{
public:
	CQuestObjectiveConditionsDefinition( void );
	virtual ~CQuestObjectiveConditionsDefinition( void );

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool BPostInit( CUtlVector<CUtlString> *pVecErrors = NULL );

	ObjectiveConditionDefIndex_t GetDefIndex() const { return m_nDefIndex; }
	KeyValues *GetKeyValues() const { return m_pConditionsKey; }

	const CUtlVector< CTFRequiredQuestItemsSet >& GetRequiredItemSets() const { return m_vecRequiredItemSets; }

private:
	ObjectiveConditionDefIndex_t m_nDefIndex;
	KeyValues  *m_pConditionsKey;
	
	CUtlVector< CTFRequiredQuestItemsSet > m_vecRequiredItemSets;
};


//-----------------------------------------------------------------------------
// CQuestObjectiveDefinition
//-----------------------------------------------------------------------------
class CQuestObjectiveDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgQuestObjectiveDef, DEF_TYPE_QUEST_OBJECTIVE >
{
public:

	CQuestObjectiveDefinition( void );
	virtual ~CQuestObjectiveDefinition( void );

	virtual bool BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;

	const char *GetDescriptionToken( void ) const { return m_msgData.loc_desctoken().c_str(); }
	const CQuestObjectiveConditionsDefinition* GetConditions() const;
	int GetPoints() const { return m_msgData.points(); }
	void AddModifiers( CTFQuestEvaluator* pEvaluator ) const;
	KeyValues *GetConditionsKeyValues() const;

private:

	KeyValues* m_pKVConditions;
};

struct QuestObjectiveInstance_t
{
public:
	QuestObjectiveInstance_t( const CMsgQuestDef_ObjectiveInstance& obj )
		: m_objective( obj )
		, m_pDef( NULL )
	{
		m_pDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestObjectiveDefinition >( obj.objective().defindex() );
		Assert( m_pDef );
	}

	QuestObjectiveInstance_t& operator=( const QuestObjectiveInstance_t& other )
	{
		m_pDef = other.m_pDef;
		m_objective = other.m_objective;

		return *this;
	}

	EQuestPoints GetPointsType() const { return m_objective.point_type(); }
	int GetPoints() const
	{
		if ( m_objective.has_point_value() )
			return m_objective.point_value();

		if ( m_pDef )
			return m_pDef->GetPoints();

		Assert( false );
		return 0;
	}
	const CQuestObjectiveDefinition* GetObjectiveDef() const { return m_pDef; }

private:
	CMsgQuestDef_ObjectiveInstance m_objective;
	const CQuestObjectiveDefinition* m_pDef;
};
typedef CUtlVector< QuestObjectiveInstance_t > QuestObjectiveDefVec_t;

//-----------------------------------------------------------------------------
// CQuestDefinition
//-----------------------------------------------------------------------------
class CQuestDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgQuestDef, DEF_TYPE_QUEST >
{
public:

	CQuestDefinition( void );

	virtual const char* GetDisplayName() const OVERRIDE { return GetLocName(); }

	virtual bool BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;

	uint32 GetMaxPoints( uint32 nIndex ) const;

	bool BActive() const; // As in, after the enable time and before the expire time
	RTime32 GetEnableTime() const;
	RTime32 GetExpireTime() const;

	const char *GetMatchmakingGroupName() const { return m_msgData.mm_criteria().group_name().c_str(); }
	const char *GetMatchmakingCategoryName() const { return m_msgData.mm_criteria().category_name().c_str(); }
	const char *GetMatchmakingMapName() const { return m_msgData.mm_criteria().map_name().c_str(); }

	const QuestObjectiveDefVec_t& GetObjectives() const { return m_vecObjectives; }
	const CQuestThemeDefinition *GetQuestTheme() const;
	const char *GetLocName() const { return m_msgData.name_loctoken().c_str(); }
	
	const char *GetCorrespondingOperationName() const { return m_msgData.operation().c_str(); }
	const CUtlVector< item_definition_index_t >& GetLoanerItems() const { return m_vecLoanerDefindex; }
	const char* GetNodeImageFileName() const { return m_msgData.node_image().c_str(); }
	const char* GetNodeIconFileName() const { return m_msgData.icon_image().c_str(); }

	void AddModifiers( CTFQuestEvaluator* pEvaluator ) const;
private:

	CUtlVector< item_definition_index_t > m_vecLoanerDefindex;
	QuestObjectiveDefVec_t m_vecObjectives;
	const CEconOperationDefinition* m_pOperation = NULL;
};


//---------------------------------------------------------------------------------
// Purpose: The shared object that contains data for a user's quest
//---------------------------------------------------------------------------------
class CQuest : public GCSDK::CProtoBufSharedObject< CSOQuest, k_EEConTypeQuest >
{
public:
	CQuest();
	CQuest( CSOQuest soData );

	uint64 GetID() const { return Obj().quest_id(); }
	const CQuestDefinition* GetDefinition() const;

	bool BEarnedAllPointsForCategory( EQuestPoints eType ) const;
	uint32 GetEarnedPoints( EQuestPoints eType ) const;
	uint32 GetSourceNodeID() const { return Obj().quest_map_node_source_id(); }
};

//---------------------------------------------------------------------------------
// Purpose: The shared object that contains data for a user's quest
//---------------------------------------------------------------------------------
class CQuestMapRewardPurchase : public GCSDK::CProtoBufSharedObject< CSOQuestMapRewardPurchase, k_EEconTypeQuestMapRewardPurchase >
{
public:
	CQuestMapRewardPurchase();
	CQuestMapRewardPurchase( CSOQuestMapRewardPurchase soData );
};


#endif // ECON_QUESTS
