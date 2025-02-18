//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_quest_map_utils.h"
#include "gcsdk/enumutils.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map.h"
#include "econ_quests.h"
#include "econ_game_account_client.h"
#ifdef CLIENT_DLL
	#include "tf_item_inventory.h"
#endif

using namespace GCSDK;


#ifdef CLIENT_DLL
uint64 GetNodeDefPairKey( uint32 nDefindex1, uint32 nDefIndex2 )
{
	uint64 key = 0;
	key |=  (uint64)Min( nDefindex1, nDefIndex2 ) << 32;
	key |= Max( nDefindex1, nDefIndex2 );
	return key;
}

const CQuestMapHelper& GetQuestMapHelper()
{
	static CQuestMapHelper s_LocalPlayerHelper;
	return s_LocalPlayerHelper;
}
#endif

CQuestMapHelper::CQuestMapHelper( CSteamID steamID )
	: m_mapNodes( DefLessFunc( uint32 ) )
	, m_steamID( steamID )
	, m_mapStarsAvailableToSpend( DefLessFunc( uint32 ) )
	, m_mapStarsEarned( DefLessFunc( uint32 ) )
	, m_mapStarsTotal( DefLessFunc( uint32 ) )
	, m_mapRewardDefindexPurchases( DefLessFunc( uint32 ) )
{
	Refresh();
}

#ifdef CLIENT_DLL
CQuestMapHelper::CQuestMapHelper()
	: m_mapNodes( DefLessFunc( uint32 ) )
	, m_mapNodeConnections( DefLessFunc( uint64 ) )
	, m_mapRegionHasAvailableContracts( DefLessFunc( uint32 ) )
	, m_mapStarsAvailableToSpend( DefLessFunc( uint32 ) )
	, m_mapStarsEarned( DefLessFunc( uint32 ) )
	, m_mapStarsTotal( DefLessFunc( uint32 ) )
	, m_mapRewardDefindexPurchases( DefLessFunc( uint32 ) )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		m_steamID = steamapicontext->SteamUser()->GetSteamID();
		Refresh();
	}
}

void CQuestMapHelper::SOEvent( const GCSDK::CSharedObject *pObject )
{
	if ( pObject->GetTypeID() == CQuestMapNode::k_nTypeID ||
		 pObject->GetTypeID() == CEconGameAccountClient::k_nTypeID ||
		 pObject->GetTypeID() == CQuest::k_nTypeID ||
		 pObject->GetTypeID() == CQuestMapRewardPurchase::k_nTypeID )
	{
		Refresh();
	}
}

bool CQuestMapHelper::BIsNodeConnectionFulfilled( const CQuestMapNodeDefinition* pNode1, const CQuestMapNodeDefinition* pNode2 ) const
{
	uint64 key = GetNodeDefPairKey( pNode1->GetDefIndex(), pNode2->GetDefIndex() );
	auto idx = m_mapNodeConnections.Find( key );
	if ( idx == m_mapNodeConnections.InvalidIndex() )
	{
		Assert( false );
		return false;
	}

	return m_mapNodeConnections[ idx ];
}

bool CQuestMapHelper::BRegionHasAvailableContracts( uint32 nRegionDefIndex ) const
{
	auto idx = m_mapRegionHasAvailableContracts.Find( nRegionDefIndex );
	if ( idx == m_mapRegionHasAvailableContracts.InvalidIndex() )
		return false;

	return m_mapRegionHasAvailableContracts[ idx ];
}
#endif

const CQuest* CQuestMapHelper::GetActiveQuest() const
{
	if ( m_vecQuests.IsValidIndex( m_nActiveQuestIndex ) )
	{
		return m_vecQuests[m_nActiveQuestIndex];
	}

	return NULL;
}

#ifndef GAME_DLL
int CQuestMapHelper::GetNumRewardCredits() const 
{ 
	return m_nRewardCredits;
}
#endif // GAME_DLL

template< typename MapType, typename KeyType >
typename MapType::ElemType_t& GetMapValue( MapType& map, KeyType key )
{
	auto idx = map.Find( key );
	if ( idx == map.InvalidIndex() )
	{
		idx = map.Insert( key, 0 );
	}

	return map[ idx ];
}

int CQuestMapHelper::GetNumStarsAvailableToSpend( uint32 nTypeDefindex ) const
{
	auto idx = m_mapStarsAvailableToSpend.Find( nTypeDefindex );
	if ( idx == m_mapStarsAvailableToSpend.InvalidIndex() )
		return 0;

	return m_mapStarsAvailableToSpend[ idx ];
}

int CQuestMapHelper::GetNumStarsEarned( uint32 nTypeDefindex ) const
{
	auto idx = m_mapStarsEarned.Find( nTypeDefindex );
	if ( idx == m_mapStarsEarned.InvalidIndex() )
		return 0;

	return m_mapStarsEarned[ idx ];
}

int CQuestMapHelper::GetNumStarsTotal( uint32 nTypeDefindex ) const
{
	auto idx = m_mapStarsTotal.Find( nTypeDefindex );
	if ( idx == m_mapStarsTotal.InvalidIndex() )
		return 0;

	return m_mapStarsTotal[ idx ];
}

bool CQuestMapHelper::BCanNodeBeTurnedIn( uint32 nNodeDefindex ) const
{
	auto pNode = GetQuestMapNode( nNodeDefindex );
	if ( !pNode )
		return false;

	auto pQuest = GetQuestForNode( pNode->GetID() );
	if ( !pQuest )
		return false;

	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		EQuestPoints eType = (EQuestPoints)i;
		if ( pNode->BIsMedalOffered( eType ) &&
			 !pNode->BIsMedalEarned( eType ) &&
			 pQuest->BEarnedAllPointsForCategory( eType ) )
			return true;
	}

	return false;
}

int CQuestMapHelper::GetNumRewardPurchases( uint32 nDefindex ) const
{
	auto idx = m_mapRewardDefindexPurchases.Find( nDefindex );
	// No entry means no purchases
	if ( idx == m_mapRewardDefindexPurchases.InvalidIndex() )
		return 0;

	return m_mapRewardDefindexPurchases[ idx ];
}

void CQuestMapHelper::Refresh()
{
	// Reset
	memset( m_nTotalMedals, 0, sizeof( m_nTotalMedals ) );
	memset( m_nCollectedMedals, 0, sizeof( m_nCollectedMedals ) );
	m_nLockedAndRequirementsMetNodes = 0;
	m_nNumUnlockedNodes = 0;
	m_mapNodes.Purge();
	m_vecQuests.Purge();
	m_nActiveQuestIndex = m_vecQuests.InvalidIndex();
	m_mapStarsAvailableToSpend.Purge();
	m_mapStarsEarned.Purge();
	m_mapStarsTotal.Purge();
	m_mapRewardDefindexPurchases.Purge();
#ifndef GAME_DLL
	m_nRewardCredits = 0;
#endif // GAME_DLL
#ifdef CLIENT_DLL
	m_mapNodeConnections.Purge();
	m_mapRegionHasAvailableContracts.Purge();
#endif // CLIENT_DLL

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( m_steamID );
	if ( pSOCache && GetProtoScriptObjDefManager()->BDefinitionsLoaded() )
	{
		//
		// Reward and Node credits
		//
#ifndef GAME_DLL
		auto pClientAccount = pSOCache->GetSingleton< CEconGameAccountClient >();
		if ( !pClientAccount )
		{
			Assert( false );
			Warning( "No CEconGameAccountClient to grab reward and node credits!\n" );
			return;
		}
		m_nRewardCredits = pClientAccount->Obj().quest_reward_credits();
#endif // GAME_DLL

		// 
		// Nodes
		//
		auto *pNodesCache = pSOCache->FindTypeCache( CQuestMapNode::k_nTypeID );
		if ( pNodesCache )
		{
			for( uint32 i=0; i < pNodesCache->GetCount(); ++i )
			{
				CQuestMapNode* pNode = (CQuestMapNode*)pNodesCache->GetObject( i );
				if ( pNode->GetNodeDefinition() != NULL )
				{
					m_mapNodes.Insert( pNode->Obj().defindex(), pNode );
				}
				else
				{
					Assert( false );
					Warning( "Node %llu has defindex %d, which doesn't exist!\n", pNode->GetID(), pNode->Obj().defindex() );
				}
			}

			FOR_EACH_MAP_FAST( m_mapNodes, i )
			{
				const CQuestMapNode* pNode = m_mapNodes[ i ];
				auto nStarType = pNode->GetNodeDefinition()->GetStarType();

				for( int j = 0; j < EQuestPoints_ARRAYSIZE; ++j )
				{
					m_nTotalMedals[ j ] += 1;

					// If they collected it, add one
					if ( pNode->BIsMedalEarned( (EQuestPoints)j ) )
					{
						m_nCollectedMedals[ j ] += 1;
						GetMapValue( m_mapStarsAvailableToSpend, nStarType ) += 1;
					}
				}

				// Tally up number of activated quests
				++m_nNumUnlockedNodes;
				GetMapValue( m_mapStarsAvailableToSpend, nStarType ) -= pNode->GetNodeDefinition()->GetNumStarsToUnlock();
			}
		}

		auto& mapNodeDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_NODE );
		FOR_EACH_MAP_FAST( mapNodeDefs, i )
		{
			const CQuestMapNodeDefinition* pNodeDef = (const CQuestMapNodeDefinition*)mapNodeDefs[ i ];
			auto pNode = GetQuestMapNode( pNodeDef->GetDefIndex() );
			bool bBronzeMedalEarned = ( pNode && pNode->BIsMedalEarned( QUEST_POINTS_NOVICE ) );
			if ( pNodeDef->BCanUnlock( *this ) && !bBronzeMedalEarned )
			{
				// Tally up locked, but requirements met (unlockable)
				++m_nLockedAndRequirementsMetNodes;
#ifdef CLIENT_DLL
				m_mapRegionHasAvailableContracts.Insert( pNodeDef->GetRegionDefIndex(), true );
#endif
			}

#ifdef CLIENT_DLL
			CUtlMap< uint32, bool > mapConnectedNodeCondition;
			pNodeDef->GetConnectedNodes( mapConnectedNodeCondition );
			FOR_EACH_MAP_FAST( mapConnectedNodeCondition, j )
			{
				uint64 key = GetNodeDefPairKey( mapConnectedNodeCondition.Key( j ), pNodeDef->GetDefIndex() );

				if ( m_mapNodeConnections.Find( key ) == m_mapNodeConnections.InvalidIndex() )
				{
					m_mapNodeConnections.Insert( key, mapConnectedNodeCondition[ j ] );
				}
			}
#endif
			// Tally up how many stars they COULD go earn right now
			auto nStarType = pNodeDef->GetStarType();
			for( int j=0; j < EQuestPoints_ARRAYSIZE; ++j )
			{
				// Skip stars that arent earned
				if ( !pNodeDef->BIsMedalOffered( (EQuestPoints)j ) )
					continue;

				if ( pNode && pNode->BIsMedalEarned( (EQuestPoints)j ) )
					GetMapValue( m_mapStarsEarned, nStarType ) += 1;

				GetMapValue( m_mapStarsTotal, nStarType ) += 1;		
			}
		}

		//
		// Quests
		//
		auto *pQuestsCache = pSOCache->FindTypeCache( CQuest::k_nTypeID );
		if ( pQuestsCache )
		{
			for( uint32 i=0; i < pQuestsCache->GetCount(); ++i )
			{
				m_vecQuests.AddToTail( (CQuest*)pQuestsCache->GetObject( i ) );

				if ( m_vecQuests.Tail()->Obj().active() )
				{
					// we should only have one active quest at a time
					Assert( m_nActiveQuestIndex == m_vecQuests.InvalidIndex() );
					m_nActiveQuestIndex = i;
				}
			}
		}

		//
		// Reward purchases
		//
		auto *pRewardsCache = pSOCache->FindTypeCache( CQuestMapRewardPurchase::k_nTypeID );
		if ( pRewardsCache )
		{
			for( uint32 i=0; i < pRewardsCache->GetCount(); ++i )
			{
				const CQuestMapRewardPurchase* pPurchase = (CQuestMapRewardPurchase*)pRewardsCache->GetObject( i );
				m_mapRewardDefindexPurchases.InsertOrReplace( pPurchase->Obj().defindex(), pPurchase->Obj().count() );
			}
		}

#ifdef CLIENT_DLL
		// Tell everyone the state of the quest map changed
		IGameEvent *event = gameeventmanager->CreateEvent( "quest_map_data_changed" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
#endif

	}
}

QUESTHELPER_CONSTNESS CQuestMapNode* CQuestMapHelper::GetQuestMapNode( uint32 nNodeDef ) const
{
	auto idx = m_mapNodes.Find( nNodeDef );
	if ( idx != m_mapNodes.InvalidIndex() )
	{
		return m_mapNodes[ idx ];
	}

	return NULL;
}

QUESTHELPER_CONSTNESS CQuestMapNode* CQuestMapHelper::GetQuestMapNodeByID( uint32 nNodeID ) QUESTHELPER_CONSTNESS
{
	FOR_EACH_MAP_FAST( m_mapNodes, i )
	{
		if ( m_mapNodes[ i ]->Obj().node_id() == nNodeID )
			return m_mapNodes[ i ];
	}

	return NULL;
}

QUESTHELPER_CONSTNESS CQuest* CQuestMapHelper::GetQuestForNode( uint32 nNodeID ) const
{ 
	FOR_EACH_VEC( m_vecQuests, i )
	{
		if ( m_vecQuests[ i ]->Obj().quest_map_node_source_id() == nNodeID )
			return m_vecQuests[ i ];
	}

	return NULL;
}

QUESTHELPER_CONSTNESS CQuest* CQuestMapHelper::GetQuestByID( uint32 nQuestID ) QUESTHELPER_CONSTNESS
{
	FOR_EACH_VEC( m_vecQuests, i )
	{
		if ( m_vecQuests[ i ]->Obj().quest_id() == nQuestID )
			return m_vecQuests[ i ];
	}

	return NULL;
}

QUESTHELPER_CONSTNESS CQuest* CQuestMapHelper::GetQuestByDefindex( uint32 nDefIndex ) const
{
	FOR_EACH_VEC( m_vecQuests, i )
	{
		if ( m_vecQuests[ i ]->Obj().defindex() == nDefIndex )
			return m_vecQuests[ i ];
	}

	return NULL;
}