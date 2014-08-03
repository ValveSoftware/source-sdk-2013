//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MAPENTITIES_H
#define MAPENTITIES_H
#ifdef _WIN32
#pragma once
#endif

#include "mapentities_shared.h"

// This class provides hooks into the map-entity loading process that allows CS to do some tricks
// when restarting the round. The main trick it tries to do is recreate all 
abstract_class IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname ) = 0;
	virtual CBaseEntity* CreateNextEntity( const char *pClassname ) = 0;
};

// Use the filter so you can prevent certain entities from being created out of the map.
// CSPort does this when restarting rounds. It wants to reload most entities from the map, but certain
// entities like the world entity need to be left intact.
void MapEntity_ParseAllEntities( const char *pMapData, IMapEntityFilter *pFilter=NULL, bool bActivateEntities=false );

const char *MapEntity_ParseEntity( CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter );
void MapEntity_PrecacheEntity( const char *pEntData, int &nStringSize );


//-----------------------------------------------------------------------------
// Hierarchical spawn 
//-----------------------------------------------------------------------------
struct HierarchicalSpawn_t
{
	CBaseEntity *m_pEntity;
	int			m_nDepth;
	CBaseEntity	*m_pDeferredParent;			// attachment parents can't be set until the parents are spawned
	const char	*m_pDeferredParentAttachment; // so defer setting them up until the second pass
};

void SpawnHierarchicalList( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities );

#endif // MAPENTITIES_H
