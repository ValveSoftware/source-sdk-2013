//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_quest_map_node.h"
#include "tf_quest_map.h"
#include "tf_quest_map_utils.h"
#include "econ_quests.h"
#include "gcsdk/enumutils.h"
#include "schemainitutils.h"
#include "tier2/fileutils.h"
#include <google/protobuf/text_format.h>
#ifdef CLIENT_DLL
	#include "tf_controls.h"
	#include "tf_item_inventory.h"
#endif

static const char* g_pszMapNodesFile = "scripts/quest_maps.txt";

REGISTER_PROTO_DEF_FACTORY( CQuestMapNodeDefinition, DEF_TYPE_QUEST_MAP_NODE )
REGISTER_PROTO_DEF_FACTORY( CQuestMapStarType, DEF_TYPE_QUEST_MAP_STAR_TYPE )


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;


CQuestMapNodeDefinition::CQuestMapNodeDefinition()
	: m_pAssociatedOperationDef( NULL )
{}

CQuestMapNodeDefinition::~CQuestMapNodeDefinition()
{}

bool CQuestMapNodeDefinition::BHasPassToUnlock( const CQuestMapHelper& helper ) const
{
	const CEconOperationDefinition* pOperation = GetAssociatedOperation();
	if ( pOperation )
	{
		// If there's a required item and a gateway item
		if ( pOperation->GetRequiredItemDefIndex() != INVALID_ITEM_DEF_INDEX && pOperation->GetGatewayItemDefIndex() != INVALID_ITEM_DEF_INDEX )
		{

#ifdef CLIENT_DLL
			if ( TFInventoryManager()->GetLocalTFInventory()->FindFirstItembyItemDef( pOperation->GetRequiredItemDefIndex() ) == NULL )
			{
				return false;
			}
#endif
		}
	}

	return true;
}

bool CQuestMapNodeDefinition::BHasRequiredNodesCompltedToUnlock( const CQuestMapHelper& helper ) const
{
	// Conditions aren't met?
	return BConditionsMet_Recursive( m_msgData.condition(), helper.GetQuestMapNodes(), NULL );
}

bool CQuestMapNodeDefinition::BHasStarsToUnlock( const CQuestMapHelper& helper ) const
{
	return helper.GetNumStarsAvailableToSpend( GetStarType() ) >= (int)GetNumStarsToUnlock();
}

bool CQuestMapNodeDefinition::BCanUnlock( const CQuestMapHelper& helper ) const
{
	return BIsActive()
		&& BHasPassToUnlock( helper )
		&& BHasRequiredNodesCompltedToUnlock( helper )
		&& BHasStarsToUnlock( helper );
}

bool CQuestMapNodeDefinition::BIsActive() const
{
	// If any of our offered quests are active, then we're active
	for( uint32 i=0; i < GetNumOfferedQuests(); ++i )
	{
		if ( GetOfferedQuest( i )->BActive() )
			return true;
	}

	return false;
}

#ifdef CLIENT_DLL
void CQuestMapNodeDefinition::GetCantUnlockReason( wchar_t* out_wszRequirements, int nRequirementsSize ) const
{
	auto& helper = GetQuestMapHelper();
	V_wcsncat( out_wszRequirements, g_pVGuiLocalize->Find( "TF_QuestView_Requirements" ), nRequirementsSize );

	if ( !BHasPassToUnlock( helper ) )
	{
		// And the user doesn't have the required item
		V_wcsncat( out_wszRequirements, L"\n\n", nRequirementsSize );	
		V_wcsncat( out_wszRequirements, g_pVGuiLocalize->Find( "TF_QuestMap_Tooltip_RequirePass" ), nRequirementsSize );				
	}

	if ( !BHasStarsToUnlock( helper ) )
	{
		V_wcsncat( out_wszRequirements, L"\n\n", nRequirementsSize );	
		V_wcsncat( out_wszRequirements, 
					LocalizeNumberWithToken( "TF_QuestView_Requirements_StarCount",  GetNumStarsToUnlock(), helper.GetNumStarsAvailableToSpend( GetStarType() ) ),
					nRequirementsSize );
	}

	if ( !BHasRequiredNodesCompltedToUnlock( helper ) )
	{
		V_wcsncat( out_wszRequirements, L"\n\n", nRequirementsSize );	
		V_wcsncat( out_wszRequirements, 
				   LocalizeNumberWithToken( "TF_QuestView_Requirement_NodeState",  GetNumStarsToUnlock(), helper.GetNumStarsAvailableToSpend( GetStarType() ) ),
				   nRequirementsSize );
	}
}
#endif

bool CQuestMapNodeDefinition::BConditionsMet_Recursive( const CMsgQuestMapNodeCondition& msgCondition
													  , const MapQuestNodes_t& nodes
													  , CUtlMap< uint32, bool >* pMapOutConnectedNodeCondition) const
{
	if ( msgCondition.has_logical() )
	{
		const CMsgQuestMapNodeCondition_Logic& msgLogic = msgCondition.logical();
		switch( msgLogic.operation() )
		{
			case LogicalOperation::AND:
			{
				int nNumNotFulfilled = 0;

				for( int i=0; i < msgLogic.sub_conditions_size(); ++i )
				{
					if ( !BConditionsMet_Recursive( msgLogic.sub_conditions( i ), nodes, pMapOutConnectedNodeCondition ) )
					{
						++nNumNotFulfilled;
					}
				}

				return nNumNotFulfilled == 0;
			}
			break;

			case LogicalOperation::OR:
			{
				// Not supported
				Assert( false );
			}
			break;

			case LogicalOperation::NOT:
			{
				// Not suppored
				Assert( false );
			}
			break;
		}
	}
	else if ( msgCondition.has_node_state() )
	{
		const CMsgQuestMapNodeCondition_NodeState& msgNodeState = msgCondition.node_state();

		auto LambdaAddNodeConnection = [&]( bool bCompleted )
		{
			if ( pMapOutConnectedNodeCondition )
			{
				pMapOutConnectedNodeCondition->Insert( msgNodeState.target_node_defid().defindex(), bCompleted );
			}
		};

		// See if they have the node, and have the required state
		auto idx = nodes.Find( msgNodeState.target_node_defid().defindex() );
		if ( idx != nodes.InvalidIndex() )
		{
			const CQuestMapNode* pTargetNode = nodes[ idx ];		
			if ( pTargetNode->BIsMedalEarned( QUEST_POINTS_NOVICE ) )
			{
				LambdaAddNodeConnection( true );
				return true;
			}
		}

		LambdaAddNodeConnection( false );
		return false;
		
	}

	return true;
}

#ifdef CLIENT_DLL
void CQuestMapNodeDefinition::GetConnectedNodes( CUtlMap< uint32, bool >& mapOutConnectedNodeCondition ) const
{
	if ( m_msgData.has_condition() )
	{
		mapOutConnectedNodeCondition.SetLessFunc( DefLessFunc( uint32 ) );
		BConditionsMet_Recursive( m_msgData.condition(), GetQuestMapHelper().GetQuestMapNodes(), &mapOutConnectedNodeCondition );
	}
}

const char* CQuestMapNodeDefinition::GetIconName() const
{ 
	if ( m_msgData.quest_options_size() == 1 )
	{
		return GetOfferedQuest( 0 )->GetNodeIconFileName();
	}

	return "cyoa/cyoa_multiplechoice_icon";
}

bool CQuestMapNodeDefinition::GetRewardItem( CEconItemView& item ) const
{
	if ( m_msgData.has_reward_item_name() )
	{
		auto pItemDef = GetItemSchema()->GetItemDefinitionByName( m_msgData.reward_item_name().c_str() );
		if ( pItemDef )
		{
			item.Init( pItemDef->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
			return true;
		}
	}

	return false;
}

#endif

bool CQuestMapNodeDefinition::BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors )
{
	CTypedProtoBufScriptObjectDefinition::BPostDataLoaded( pVecErrors );

	// Dig up our associated operation def if we have one
	Assert( GetItemSchema() );
	if ( !m_msgData.associated_operation().empty() )
	{
		const char *pszAssociatedOperation = m_msgData.associated_operation().c_str();
		m_pAssociatedOperationDef = GetItemSchema()->GetOperationByName( pszAssociatedOperation );
		Assert( m_pAssociatedOperationDef != NULL );
	}

	EvaluateLinks();

	return true;
}

void CQuestMapNodeDefinition::EvaluateLinks()
{
#ifdef CLIENT_DLL
	if ( m_bEvaluatingLinks )
		return;

	m_bEvaluatingLinks = true;

	// Make our known nodes re-do their connections
	FOR_EACH_VEC( m_vecConnectedNodes, i )
	{
		const_cast< CQuestMapNodeDefinition* >( m_vecConnectedNodes[ i ].m_pNode )->EvaluateLinks();
	}

	m_vecConnectedNodes.Purge();

	const DefinitionMap_t& mapNodeDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( GetDefType() );

	//
	// Go through all of the other nodes and check if we're one of THEIR pre-reqs.
	//
	FOR_EACH_MAP_FAST( mapNodeDefs, i )
	{
		const CQuestMapNodeDefinition* pOtherNode = static_cast< const CQuestMapNodeDefinition* >( mapNodeDefs[ i ] );
		if ( pOtherNode == this )
			continue;

		CUtlMap< uint32, bool > mapConnectedNodeCondition;
		pOtherNode->GetConnectedNodes( mapConnectedNodeCondition );

		FOR_EACH_MAP( mapConnectedNodeCondition, j )
		{
			if ( mapConnectedNodeCondition.Key( j ) == GetDefIndex() )
			{
				m_vecConnectedNodes.AddToTail( { pOtherNode, CONNECTS_TO } );
			}
		}
	}

	CUtlMap< uint32, bool > mapConnectedNodeCondition;
	GetConnectedNodes( mapConnectedNodeCondition );
	FOR_EACH_MAP( mapConnectedNodeCondition, j )
	{
		auto pOther = GetProtoScriptObjDefManager()->GetTypedDefinition< const CQuestMapNodeDefinition >( mapConnectedNodeCondition.Key( j ) );
		m_vecConnectedNodes.AddToTail( { pOther, CONNECTS_FROM } );	
	}

	m_bEvaluatingLinks = false;
#endif
}

void CQuestMapNodeDefinition::GetOfferedQuests( CUtlVector< const CQuestDefinition* >& vecOutQuestDefs ) const
{
	for ( int i=0; i < m_msgData.quest_options_size(); ++i )
	{
		vecOutQuestDefs.AddToTail( GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestDefinition >( m_msgData.quest_options( i ) ) );
	}
}

bool CQuestMapNodeDefinition::BIsMedalOffered( EQuestPoints eType ) const
{
	// Use the first!
	if ( GetNumOfferedQuests() > 0 )
	{
		auto pQuestDef = GetOfferedQuest( 0 );
		return pQuestDef->GetMaxPoints( eType ) > 0;
	}

	// No stars if no quest
	return false;
}

const char* CQuestMapNodeDefinition::GetNameLocToken() const
{ 
	return m_msgData.name_loctoken().c_str();
}

const CQuestDefinition* CQuestMapNodeDefinition::GetOfferedQuest( uint32 nIndex ) const	
{
	return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestDefinition >( m_msgData.quest_options( nIndex ) );
}

int CQuestMapNodeDefinition::GetRewardCredits() const
{
	switch ( m_msgData.cash_reward() )
	{
		case CASH_REWARD_NONE:		return 0;
		case CASH_REWARD_SMALL:		return 100;
		case CASH_REWARD_MEDIUM:	return 150;
		case CASH_REWARD_LARGE:		return 250;
	}

	Assert( false );
	return 0;
}

const CEconOperationDefinition* CQuestMapNodeDefinition::GetAssociatedOperation() const
{
	return m_pAssociatedOperationDef;
}

uint32 CQuestMapNodeDefinition::GetStarType() const
{
	auto pStarType = GetStarTypeDef();
	return pStarType ? pStarType->GetDefIndex() : 0;
}

const CQuestMapStarType* CQuestMapNodeDefinition::GetStarTypeDef() const
{
	return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapStarType >( m_msgData.star_type() );
}

CQuestMapNode::CQuestMapNode()
{
	Obj().set_account_id( 0 );
	Obj().set_defindex( 0 );
	Obj().set_node_id( 0 );
}

uint32 CQuestMapNode::GetBonusesCompleted() const
{
	int nTotal = 0;
	// star_0 is the primary objective
	nTotal += Obj().star_1_earned() ? 1 : 0;
	nTotal += Obj().star_2_earned() ? 1 : 0;
	return nTotal;
}

bool CQuestMapNode::BAllMedalsEarned() const
{
	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		if ( !BIsMedalEarned( (EQuestPoints)i ) )
			return false;
	}

	return true;
}

bool CQuestMapNode::BIsMedalOffered( EQuestPoints eType ) const
{
	return GetNodeDefinition()->BIsMedalOffered( eType );
}

bool CQuestMapNode::BIsMedalEarned( EQuestPoints eType ) const
{
	switch( eType )
	{
	case QUEST_POINTS_NOVICE: return Obj().star_0_earned() != 0;
	case QUEST_POINTS_ADVANCED: return Obj().star_1_earned() != 0;
	case QUEST_POINTS_EXPERT: return Obj().star_2_earned() != 0;
	default:
		Assert( false );
		return false;
	}
}

const CQuestMapNodeDefinition* CQuestMapNode::GetNodeDefinition() const
{
	return (CQuestMapNodeDefinition*)GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_QUEST_MAP_NODE, Obj().defindex() ) );
}

bool CQuestMapNode::BCanRedeemLoot() const 
{
	return !BHasLootBeenClaimed() && BIsMedalEarned( QUEST_POINTS_NOVICE );
}	

const CQuestDefinition* CQuestMapNode::GetSelectedQuest() const
{
	if ( Obj().has_selected_quest_def() && Obj().selected_quest_def() != 0 )
	{
		return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestDefinition >( Obj().selected_quest_def() );
	}

	return NULL;
}

#ifdef CLIENT_DLL
const char* CQuestMapNode::GetIconName() const
{
	const CQuestDefinition* pSelectedQuest = GetSelectedQuest();
	if ( pSelectedQuest )
		return pSelectedQuest->GetNodeIconFileName();

	const CQuestMapNodeDefinition* pDef = GetNodeDefinition();
	return pDef->GetIconName();
}
#endif

#ifdef CLIENT_DLL
bool CQuestMapNode::BCanClaimAnyLoot() const
{
	const CQuest* pQuest = GetQuestMapHelper().GetQuestForNode( GetID() );
#else
bool CQuestMapNode::BCanClaimAnyLoot( const CQuest* pQuest ) const
{
#endif

	if ( !pQuest )
		return false;

	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		if ( pQuest->BEarnedAllPointsForCategory( (EQuestPoints)i ) && !BIsMedalEarned( (EQuestPoints)i ) )
			return true;
	}

	return false;
}