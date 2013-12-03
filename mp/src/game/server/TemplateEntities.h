//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Template entities are used by spawners to create copies of entities
//			that were configured by the level designer. This allows us to spawn
//			entities with arbitrary sets of key/value data and entity I/O
//			connections.
//
//=============================================================================//

#ifndef TEMPLATEENTITIES_H
#define TEMPLATEENTITIES_H
#ifdef _WIN32
#pragma once
#endif

#include "isaverestore.h"

class CBaseEntity;
class CPointTemplate;

int			Templates_Add(CBaseEntity *pEntity, const char *pszMapData, int nLen);
string_t	Templates_FindByIndex( int iIndex );
int			Templates_GetStringSize( int iIndex );
string_t	Templates_FindByTargetName(const char *pszName);
void		Templates_ReconnectIOForGroup( CPointTemplate *pGroup );

// Some templates have Entity I/O connecting the entities within the template.
// Unique versions of these templates need to be created whenever they're instanced.
void		Templates_StartUniqueInstance( void );
bool		Templates_IndexRequiresEntityIOFixup( int iIndex );
char		*Templates_GetEntityIOFixedMapData( int iIndex );

// Save / Restore
ISaveRestoreBlockHandler *GetTemplateSaveRestoreBlockHandler( void );

#endif // TEMPLATEENTITIES_H
