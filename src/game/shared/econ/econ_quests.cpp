//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to dynamic recipes
//
//=============================================================================


#include "cbase.h"
#include "econ_quests.h"
#include "schemainitutils.h"

#ifdef CLIENT_DLL
	#include "tf_quest_map_controller.h"
	#include "tf_gc_client.h"
#endif

	#include "quest_objective_manager.h"


#include "memdbgon.h"

														 		            			 			 							
QuestPointsDef_t g_QuestPointsDefs[] = { 
	
	{ 
		"#QuestPoints0_BarText",					// Bar text
		"#QuestPoints0_ObjectiveText",				// Objective text	
		"cyoa/medal_1",								// Active medal
		"cyoa/medal_1_inactive",					// Inactive medal
		"Quest.StatusTickNovice",					// Objective sound
		"Quest.StatusTickNoviceFriend",				// Objective by party sound
		"Quest.StatusTickNoviceComplete",			// Points completed sound
		"#QuestPoints0_Complete",					// Completed report string
		"#QuestReport_Points0Scored",				// Scored points report string
	},
										
	{ 
		"#QuestPoints1_BarText", 					// Bar text
		"#QuestPoints1_ObjectiveText",				// Objective text	
		"cyoa/medal_2",								// Active medal
		"cyoa/medal_2_inactive",					// Inactive medal
		"Quest.StatusTickAdvanced",					// Objective sound
		"Quest.StatusTickAdvancedFriend",			// Objective by party sound
		"Quest.StatusTickAdvancedComplete",			// Points completed sound
		"#QuestPoints1_Complete",					// Completed report string
		"#QuestReport_Points1Scored"				// Scored points report string
	},

	{ 
		"#QuestPoints2_BarText",					// Bar text
		"#QuestPoints2_ObjectiveText",				// Objective text	
		"cyoa/medal_3",								// Active medal
		"cyoa/medal_3_inactive",					// Inactive medal
		"Quest.StatusTickExpert",					// Objective sound
		"Quest.StatusTickExpertFriend",				// Objective by party sound
		"Quest.StatusTickExpertComplete",			// Points completed sound
		"#QuestPoints2_Complete",					// Completed report string
		"#QuestReport_Points2Scored"				// Scored points report string
	} 
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_QuestPointsDefs ) == EQuestPoints_ARRAYSIZE );

extern bool InitPerClassRandomChanceStringArray( KeyValues *pPerClassData, CRandomChanceString (&outputArray)[LOADOUT_COUNT], CUtlVector<CUtlString>* pVecErrors );

#ifdef CLIENT_DLL
CON_COMMAND( set_party_contract_progress_enabled, "Set whether or not you'd like your party memebers to be able to make progress on your Contracts along with you." )
{
	if ( args.ArgC() != 2 )
	{
		Warning( "Must specify '1' or '0' to enable or disable party Contract progress!\n" );
		return;
	}

	bool bDisable = !atoi( args[ 1 ] ) != 0; // We're flipping the logic here!

	// Don't pester the GC
	if ( GetQuestMapController().BBusyWithARequest() )
	{
		Warning( "Can't change party Contract progress preference right now.  Try again in a few seconds.\n" );
		return;
	}

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
		return;

	// Check if the owner has opted-out of party quest progress
	auto pLocalSOCache = pLocalInv->GetSOC();
	Assert( pLocalSOCache );
	if ( pLocalSOCache )
	{	
		CEconGameAccountClient *pGameAccount = pLocalSOCache->GetSingleton< CEconGameAccountClient >();
		if ( pGameAccount->Obj().disable_party_quest_progress() != bDisable )
		{
			GetQuestMapController().SetPartyProgressDisableState( bDisable );
		}
	}
}
#endif
		
REGISTER_PROTO_DEF_FACTORY( CQuestThemeDefinition, DEF_TYPE_QUEST_THEME )
CQuestThemeDefinition::CQuestThemeDefinition()
{
}

CQuestThemeDefinition::~CQuestThemeDefinition()
{}

const char *CQuestThemeDefinition::GetGiveSoundForClass( int iClass ) const
{
	if ( iClass >= m_msgData.give_sounds_size() )
		return NULL;

	return UTIL_GetRandomSoundFromEntry( GetRandomWeightedString( m_msgData.give_sounds( iClass - 1 ) ) ); 
}

const char *CQuestThemeDefinition::GetCompleteSoundForClass( int iClass ) const
{
	if ( iClass >= m_msgData.complete_sounds_size() )
		return NULL;

	return UTIL_GetRandomSoundFromEntry( GetRandomWeightedString( m_msgData.complete_sounds( iClass - 1 ) ) ); 
}

const char *CQuestThemeDefinition::GetFullyCompleteSoundForClass( int iClass ) const
{ 
	if ( iClass >= m_msgData.fully_complete_sounds_size() )
		return NULL;

	return UTIL_GetRandomSoundFromEntry( GetRandomWeightedString( m_msgData.fully_complete_sounds( iClass - 1 ) ) ); 
}

/*static*/ const char* CQuestThemeDefinition::GetRandomWeightedString( const CMsgQuestTheme_WeightedStringSet& msgSet )
{
	uint32 nTotalWeight = 0;
	for( int i=0; i < msgSet.weighted_strings_size(); ++i )
	{
		nTotalWeight += msgSet.weighted_strings( i ).weight();
	}

	uint32 nRandom = RandomInt( 0, nTotalWeight - 1 );
	uint32 nAccum = 0;
	for( int i=0; i < msgSet.weighted_strings_size(); ++i )
	{
		nAccum += msgSet.weighted_strings( i ).weight();
		if ( nRandom < nAccum )
		{
			return msgSet.weighted_strings( i ).string().c_str();
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestObjectiveConditionsDefinition::CQuestObjectiveConditionsDefinition( void )
	: m_nDefIndex( INVALID_QUEST_OBJECTIVE_CONDITIONS_INDEX )
	, m_pConditionsKey( NULL )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestObjectiveConditionsDefinition::~CQuestObjectiveConditionsDefinition( void )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestObjectiveConditionsDefinition::BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors )
{
	m_nDefIndex = atoi( pKVItem->GetName() );
	SCHEMA_INIT_CHECK( m_nDefIndex != INVALID_QUEST_OBJECTIVE_CONDITIONS_INDEX, "Invalid quest objective conditions def index!" );

	m_vecRequiredItemSets.Purge();

	KeyValues* pKVRequiredItemsBlock = pKVItem->FindKey( "required_items" );
	if ( pKVRequiredItemsBlock )
	{
		FOR_EACH_TRUE_SUBKEY( pKVRequiredItemsBlock, pRequiredItem )
		{
			m_vecRequiredItemSets[ m_vecRequiredItemSets.AddToTail() ].BInitFromKV( pRequiredItem );
		}
	}

	m_pConditionsKey = pKVItem->FindKey( "condition_logic" );
	SCHEMA_INIT_CHECK( m_pConditionsKey != NULL, "Missing conditions block for condition def %d!", m_nDefIndex );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestObjectiveConditionsDefinition::BPostInit( CUtlVector<CUtlString> *pVecErrors )
{
	// Verify all of the item defindex
	FOR_EACH_VEC( m_vecRequiredItemSets, i )
	{
		SCHEMA_INIT_SUBSTEP( m_vecRequiredItemSets[i].BPostInit( pVecErrors ) );
	}

	return SCHEMA_INIT_SUCCESS();
}

REGISTER_PROTO_DEF_FACTORY( CQuestObjectiveDefinition, DEF_TYPE_QUEST_OBJECTIVE )
CQuestObjectiveDefinition::CQuestObjectiveDefinition( void )
	: m_pKVConditions( NULL )
{}

CQuestObjectiveDefinition::~CQuestObjectiveDefinition()
{
	if ( m_pKVConditions )
	{
		m_pKVConditions->deleteThis();
	}
}

void ReplaceTemplateVariables( const CMsgQuestObjectiveDef& msg, KeyValues* pKVRoot )
{
	FOR_EACH_TRUE_SUBKEY( pKVRoot, pKVSubKey )
	{
		ReplaceTemplateVariables( msg, pKVSubKey );
	}

	FOR_EACH_VALUE( pKVRoot, pKVValue )
	{
		const char* pszValue = pKVValue->GetString();
		if ( pszValue == NULL || pszValue[0] != '$' )
			continue;

		const char* pszVar = V_strstr( pszValue + 1, "var" );

		if ( pszVar != pszValue + 1 )
			continue;

		int nVarNum = atoi( pszVar + 3 ) - 1;

		if ( nVarNum < msg.condition_vars_size() )
		{
			pKVValue->SetStringValue( msg.condition_vars( nVarNum ).string().c_str() );
		}
		else
		{
			AssertMsg1( false, "Not enough condition variables specified in objective %s", msg.header().name().c_str() );
		}
	}
}

bool CQuestObjectiveDefinition::BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	SCHEMA_INIT_SUBSTEP( CTypedProtoBufScriptObjectDefinition::BPostDataLoaded( pVecErrors ) );

	const CQuestObjectiveConditionsDefinition* pDef = GetItemSchema()->GetQuestObjectiveConditionByDefIndex( m_msgData.conditions_defindex() );
	Assert( pDef != NULL );
	SCHEMA_INIT_CHECK( pDef != NULL, "No conditions for quest objective %s!", GetName() );
	m_pKVConditions = pDef->GetKeyValues()->MakeCopy();
	ReplaceTemplateVariables( m_msgData, m_pKVConditions );

	if ( m_pKVConditions )
	{
		// Conditions don't get created until needed on the server, so let's create them right now
		// as a test to make sure they're valid and fail early rather than later.
		CTFQuestCondition *pTempConditions = NULL;

		const char *pszType = m_pKVConditions->GetString( "type" );
		pTempConditions = CreateEvaluatorByName( pszType, NULL );

		SCHEMA_INIT_CHECK( pTempConditions != NULL, "Failed to create evaluators" );

		if ( !pTempConditions->BInitFromKV( m_pKVConditions, pVecErrors ) )
		{
			delete pTempConditions;
			KeyValuesDumpAsDevMsg( m_pKVConditions );
			CUtlString strError( CFmtStr( "Failed to init conditions for quest objective: %d: %s", GetDefIndex(), GetName() ) );
			AssertMsg( false, "%s", strError.Get() );
			SCHEMA_INIT_CHECK( false, "%s", strError.Get() );
		}

		// clean up after test parsing quest conditions
		delete pTempConditions;
	}

	return SCHEMA_INIT_SUCCESS();
}

const CQuestObjectiveConditionsDefinition* CQuestObjectiveDefinition::GetConditions() const
{
	return GetItemSchema()->GetQuestObjectiveConditionByDefIndex( m_msgData.conditions_defindex() );
}

void CQuestObjectiveDefinition::AddModifiers( CTFQuestEvaluator* pEvaluator ) const
{
	//
	// Class modifiers
	//
	uint32 nClassesMask = 0;
	for( int i=0; i < m_msgData.classes_vars_size(); ++i )
	{
		if ( m_msgData.classes_vars( i ).string().length() )
		{
			nClassesMask |= 1 << GetClassIndexFromString( m_msgData.classes_vars( i ).string().c_str() );
		}
	}

	if ( nClassesMask )
	{
		pEvaluator->AddModifiers( new CTFClassQuestModifier( nClassesMask ) );
	}

	//
	// Map modifiers
	//
	CTFMapQuestModifier* pMapModifier = NULL;
	for( int i=0; i < m_msgData.map_size(); ++i )
	{
		if ( pMapModifier == NULL )
		{ 
			pMapModifier = new CTFMapQuestModifier;
			pEvaluator->AddModifiers( pMapModifier );
		}

		pMapModifier->AddMapName( m_msgData.map( i ).c_str() );
	}

	//
	// Game Mode modifiers
	//
	uint32 nGameModesMask = 0;
	for( int i=0; i < m_msgData.game_mode_size(); ++i )
	{
		nGameModesMask |= 1 << m_msgData.game_mode( i );
	}

	if ( nGameModesMask )
	{
		pEvaluator->AddModifiers( new CTFGameModeQuestModifier( nGameModesMask ) );
	}

	// Team modifier
	if ( m_msgData.has_team() )
	{
		pEvaluator->AddModifiers( new CTFTeamQuestModifier( m_msgData.team() ) );
	}

	// Conditions modifier
	if ( m_msgData.conditions_size() > 0 )
	{
		CTFConditionQuestModifier* pCondMod = new CTFConditionQuestModifier( m_msgData.condition_logic() );

		for ( int i=0; i < m_msgData.conditions_size(); ++i )
		{
			pCondMod->AddCondition( (ETFCond)m_msgData.conditions( i ) );
		}

		pEvaluator->AddModifiers( pCondMod );
	}

	// Items modifier
	if ( m_msgData.item_name_size() > 0 )
	{
		CTFEquippedItemsQuestModifier* pItemMod = new CTFEquippedItemsQuestModifier( m_msgData.item_logic() );

		for( int i=0; i < m_msgData.item_name_size(); ++i )
		{
			CEconItemDefinition* pItem = GetItemSchema()->GetItemDefinitionByName( m_msgData.item_name( i ).c_str() );
			if ( pItem )
			{
				pItemMod->AddItem( pItem->GetItemDefinitionName() );
			}
		}

		pEvaluator->AddModifiers( pItemMod );
	}

	if ( m_msgData.has_jump_state() )
	{
		pEvaluator->AddModifiers( new CTFJumpStateQuestModifier( m_msgData.jump_state() ) );
	}
}

KeyValues *CQuestObjectiveDefinition::GetConditionsKeyValues() const
{
	return m_pKVConditions;
}


REGISTER_PROTO_DEF_FACTORY( CQuestDefinition, DEF_TYPE_QUEST )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestDefinition::CQuestDefinition( void )
{}

bool CQuestDefinition::BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors )
{
	CTypedProtoBufScriptObjectDefinition< CMsgQuestDef, DEF_TYPE_QUEST >::BPostDataLoaded( pVecErrors );

	m_vecObjectives.Purge();

	for ( int i=0; i < m_msgData.objectives_size(); ++i )
	{
		CMsgQuestDef_ObjectiveInstance obj = m_msgData.objectives( i );
		QuestObjectiveInstance_t instance( obj );
		m_vecObjectives.AddToTail( instance );
	}

	// Dig up our associated operation def if we have one
	Assert( GetItemSchema() );
	if ( !m_msgData.operation().empty() )
	{
		const char *pszAssociatedOperation = m_msgData.operation().c_str();
		m_pOperation = GetItemSchema()->GetOperationByName( pszAssociatedOperation );
		Assert( m_pOperation != NULL );
	}

	bool bAllItemsValid = true;

	for( int i = 0; i < m_msgData.loaner_names_size(); ++i )
	{
		const CEconItemDefinition* pItemDef =  GetItemSchema()->GetItemDefinitionByName( m_msgData.loaner_names( i ).c_str() );
		if ( !pItemDef )
		{
			bAllItemsValid = false;
			if ( pVecErrors )
			{
				pVecErrors->AddToTail( CFmtStr( "Invalid loaner item name %s in %d: %s", m_msgData.loaner_names( i ).c_str(), GetDefIndex(), GetName() ).Get() );
			}
		}
		else
		{
			m_vecLoanerDefindex.AddToTail( pItemDef->GetDefinitionIndex() );
		}
	}

	return bAllItemsValid;
}

uint32 CQuestDefinition::GetMaxPoints( uint32 nIndex ) const 
{
	switch ( nIndex )
	{ 
	case 0: return m_msgData.max_points_0();
	case 1: return m_msgData.max_points_1();
	case 2: return m_msgData.max_points_2();
	}

	Assert( false );
	return 0;
}

bool CQuestDefinition::BActive() const
{
	if ( !m_pOperation )
		return true;

	RTime32 rtNow = CRTime::RTime32TimeCur();
	if ( GetEnableTime() && rtNow < GetEnableTime() )
		return false;

	if ( GetExpireTime() && rtNow > GetExpireTime() )
		return false;

	return true;
}

RTime32 CQuestDefinition::GetEnableTime() const
{
	if ( m_pOperation )
	{
		return m_pOperation->GetStartDate();
	}

	return 0;
}

RTime32 CQuestDefinition::GetExpireTime() const
{
	if ( m_pOperation )
	{
		return m_pOperation->GetStopContractsDate();
	}

	return 0;
}

void CQuestDefinition::AddModifiers( CTFQuestEvaluator* pEvaluator ) const
{
	//
	// Map modifiers
	//
	CTFMapQuestModifier* pMapModifier = NULL;
	for( int i=0; i < m_msgData.map_size(); ++i )
	{
		if ( pMapModifier == NULL )
		{ 
			pMapModifier = new CTFMapQuestModifier;
			pEvaluator->AddModifiers( pMapModifier );
		}

		pMapModifier->AddMapName( m_msgData.map( i ).c_str() );
	}
}

const CQuestThemeDefinition *CQuestDefinition::GetQuestTheme() const
{
	return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestThemeDefinition >( m_msgData.theme().defindex() );
}

CQuest::CQuest()
{
	Obj().set_defindex( 0 );
}

CQuest::CQuest( CSOQuest soData )
{
	Obj().CopyFrom( soData );
}


const CQuestDefinition* CQuest::GetDefinition() const
{
	return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestDefinition >( Obj().defindex() );
}

uint32 CQuest::GetEarnedPoints( EQuestPoints eType ) const
{
	switch( eType )
	{
	case QUEST_POINTS_NOVICE: return Obj().points_0();
	case QUEST_POINTS_ADVANCED: return Obj().points_1();
	case QUEST_POINTS_EXPERT: return Obj().points_2();
	default:
		Assert( false );
		return 0;
	}
}

bool CQuest::BEarnedAllPointsForCategory( EQuestPoints eType ) const
{
	return GetEarnedPoints( eType ) >= GetDefinition()->GetMaxPoints( eType );
}


CQuestMapRewardPurchase::CQuestMapRewardPurchase()
{
	Obj().set_account_id( 0 );
	Obj().set_defindex( 0 );
	Obj().set_count( 0 );
	Obj().set_map_cycle( 0 );
	Obj().set_purchase_id( 0 );
}

CQuestMapRewardPurchase::CQuestMapRewardPurchase( CSOQuestMapRewardPurchase soData )
{
	Obj().CopyFrom( soData );
}

