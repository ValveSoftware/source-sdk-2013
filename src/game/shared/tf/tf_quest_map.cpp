//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_quest_map.h"
#include "tf_quest_map_node.h"
#include "tf_proto_script_obj_def.h"
#include "gcsdk/enumutils.h"
#include "schemainitutils.h"
#include "econ_quests.h"
#include "tf_proto_script_obj_def.h"

using namespace GCSDK;

REGISTER_PROTO_DEF_FACTORY( CQuestMapRegion, DEF_TYPE_QUEST_MAP_REGION )
REGISTER_PROTO_DEF_FACTORY( CQuestMapStoreItem, DEF_TYPE_QUEST_MAP_STORE_ITEM )

bool CQuestMapRegion::BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors )
{
	if ( GetTypedMsg().has_return_link() )
	{
		m_pParent = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( GetTypedMsg().return_link() );
		Assert( m_pParent );
	}

	// Snag all the node definitions within us
	m_vecNodesWithin.Purge();
	const DefinitionMap_t& mapType = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_NODE );

	FOR_EACH_MAP_FAST( mapType, idx )
	{
		const CQuestMapNodeDefinition* pNode = assert_cast< const CQuestMapNodeDefinition* >( mapType[ idx ] );
		if ( pNode->GetRegionDefIndex() == GetDefIndex() && !pNode->GetHeader().prefab_only() )
		{
			m_vecNodesWithin.AddToTail( pNode );
		}
	}

	return true;
}

const CQuestMapStarType* CQuestMapRegion::GetStarType() const
{
	if ( m_msgData.has_star_type() )
		return GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapStarType >( m_msgData.star_type() );

	return NULL;
}