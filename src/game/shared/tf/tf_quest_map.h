//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_QUEST_MAP_H
#define TF_QUEST_MAP_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"
#include "tf_proto_script_obj_def.h"
#include "tf_quest_map_node.h"
#if defined (CLIENT_DLL) || defined (GAME_DLL)
	#include "gc_clientsystem.h"
#endif


typedef CTypedProtoBufScriptObjectDefinition< CMsgQuestMapStoreItem, DEF_TYPE_QUEST_MAP_STORE_ITEM > CQuestMapStoreItem;

class CQuestMapRegion : public CTypedProtoBufScriptObjectDefinition< CMsgQuestMapRegionDef, DEF_TYPE_QUEST_MAP_REGION >
{
public:

	CQuestMapRegion()
		: m_pParent( NULL )
	{}
	virtual ~CQuestMapRegion() {}

	const char* GetRegionNameLocToken() const { return m_msgData.name().c_str(); }
	const char* GetResFile() const { return m_msgData.resfile().c_str(); }
	int GetNumLinks() const { return m_msgData.links_size(); }
	const CMsgQuestMapRegionDef_RegionLink& GetLink( int i ) const { return m_msgData.links( i ); }
	virtual bool BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;
	float GetRadioFreq() const { return m_msgData.radio_freq(); }
	float GetZoomScale() const { return m_msgData.zoom_scale(); }
	const CUtlVector< const CQuestMapNodeDefinition* >& GetNodesInRegion() const { return m_vecNodesWithin; }
	const CQuestMapStarType* GetStarType() const;

	const CQuestMapRegion* GetParent() const { return m_pParent; }

private:

	const CQuestMapRegion* m_pParent;
	CUtlVector< const CQuestMapNodeDefinition* > m_vecNodesWithin;
};

#endif // TF_QUEST_MAP_H
