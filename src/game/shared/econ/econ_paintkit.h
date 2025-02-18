//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to paintkits
//
//=============================================================================

#ifndef ECON_PAINTKIT
#define ECON_PAINTKIT
#ifdef _WIN32
#pragma once
#endif

#include "tf_proto_script_obj_def.h"

#ifdef CLIENT_DLL
#include "tf_quest_map_editor_panel.h"
#endif

//-----------------------------------------------------------------------------
// CQuestObjectiveDefinition
//-----------------------------------------------------------------------------
class CPaintKitVariables : public CTypedProtoBufScriptObjectDefinition< CMsgPaintKit_Variables, DEF_TYPE_PAINTKIT_VARIABLES >
{
public:

	CPaintKitVariables( void ) {}
	virtual ~CPaintKitVariables( void ) {}
};

class CPaintKitOperation : public CTypedProtoBufScriptObjectDefinition< CMsgPaintKit_Operation, DEF_TYPE_PAINTKIT_OPERATION >
{
public:

	CPaintKitOperation( void ) {}
	virtual ~CPaintKitOperation( void ) {}
};

class CPaintKitItemDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgPaintKit_ItemDefinition, DEF_TYPE_PAINTKIT_ITEM_DEFINITION >
{
public:

	CPaintKitItemDefinition( void ) {}
	virtual ~CPaintKitItemDefinition( void ) {}
};

class CPaintKitDefinition : public CTypedProtoBufScriptObjectDefinition< CMsgPaintKit_Definition, DEF_TYPE_PAINTKIT_DEFINITION >
{
public:

	CPaintKitDefinition( void )
#ifdef CLIENT_DLL
		: m_mapCacheKV( DefLessFunc( PaintKitMapKey_t ) )
#endif // CLIENT_DLL
	{
		m_bHasPaintKitTool = false;
	}
	virtual ~CPaintKitDefinition( void ) {}

	virtual const char* GetDisplayName() const OVERRIDE { return GetDescriptionToken(); }

	const char *GetDescriptionToken( void ) const { return m_msgData.loc_desctoken().c_str(); }
	bool BHasTeamTextures() const { return m_msgData.has_team_textures(); }

	bool CanApplyToItem( item_definition_index_t iDefIndex ) const;
	// Returns all of the items this paintkit can be applied to from a paintkit tool context 
	// (ie. You CAN apply to a shotgun, but you CANT apply to the paintkit tool)
	int GetSupportedItems( CUtlVector< item_definition_index_t > *pVecItemIcons = NULL ) const;

	// Fills out a vector that contains all of the items that can render this paintkit, **NOT** the items that this paintkit
	// can be applied to.  Returns the count.
	int GetItemsThatCanRenderThisPaintkit( CUtlVector< item_definition_index_t > *pVecItemIcons = NULL ) const;

	const char* GetMaterialOverride( item_definition_index_t iDefIndex ) const;

#ifdef CLIENT_DLL
	KeyValues *GetItemPaintKitDefinitionKV( item_definition_index_t iDefIndex, int iPaintKitIndex ) const;
#endif // CLIENT_DLL
private:
	void GenerateSupportedItems() const;

#ifdef CLIENT_DLL
	typedef std::pair<item_definition_index_t, int>	PaintKitMapKey_t;
	mutable CUtlMap< PaintKitMapKey_t, KeyValues* > m_mapCacheKV; // mutable because const* is dumb
#endif // CLIENT_DLL

	struct SupportedItem_t
	{
		item_definition_index_t m_itemDef;
		const char *m_pszMaterialOverride;
	};
	mutable CUtlVector< SupportedItem_t > m_vecSupportedItems;
	mutable bool m_bHasPaintKitTool;
};

#ifdef CLIENT_DLL
struct PaintKitVariable_t
{
	CUtlString	m_strValue;
	bool		m_bCanOverride = true;
};
typedef CUtlDict< PaintKitVariable_t > VariableDict_t;


#endif //CLIENT_DLL

#endif // ECON_PAINTKIT
