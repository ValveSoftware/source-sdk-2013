//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the set of functions for manipulating entity hierarchies.
//
// $NoKeywords: $
//=============================================================================//

#ifndef HIERARCHY_H
#define HIERARCHY_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CBaseEntity;

void		UnlinkFromParent( CBaseEntity *pRemove );
void		TransferChildren( CBaseEntity *pOldParent, CBaseEntity *pNewParent );
void		LinkChild( CBaseEntity *pParent, CBaseEntity *pChild );
void		UnlinkAllChildren( CBaseEntity *pParent );
int			GetAllChildren( CBaseEntity *pParent, CUtlVector<CBaseEntity *> &list );
bool		EntityIsParentOf( CBaseEntity *pParent, CBaseEntity *pEntity );
int			GetAllInHierarchy( CBaseEntity *pParent, CUtlVector<CBaseEntity *> &list );

#endif // HIERARCHY_H
