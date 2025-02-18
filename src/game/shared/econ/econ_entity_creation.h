//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ITEM_CREATION_H
#define ITEM_CREATION_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "econ_item_system.h"
#include "econ_entity.h"

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
#include "tf_shareddefs.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: Game system that handles initializing the item system, and generating items as full game entities
//-----------------------------------------------------------------------------
class CItemGeneration : public CAutoGameSystem
{
public:
	CItemGeneration( void );

	// Generate a random item matching the specified criteria
	CBaseEntity *GenerateRandomItem( CItemSelectionCriteria *pCriteria, const Vector &vecOrigin, const QAngle &vecAngles, const char* pszOverrideClassName = NULL );

	// Generate a random item matching the specified definition index
	CBaseEntity *GenerateItemFromDefIndex( int iDefIndex, const Vector &vecOrigin, const QAngle &vecAngles );

	// Generate an item from the specified item data
	CBaseEntity *GenerateItemFromScriptData( const CEconItemView *pData, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszOverrideClassName );

	// Generate the base item for a class's loadout slot 
	CBaseEntity *GenerateBaseItem( struct baseitemcriteria_t *pCriteria );

private:
	// Create a new instance of the chosen item
	CBaseEntity *SpawnItem( int iChosenItem, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, int iItemLevel, entityquality_t entityQuality, const char *pszOverrideClassName );
	CBaseEntity *SpawnItem( const CEconItemView *pData, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, const char *pszOverrideClassName );
	CBaseEntity *PostSpawnItem( CBaseEntity *pItem, IHasAttributes *pItemInterface, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles );
};

extern CItemGeneration *ItemGeneration( void );

#endif // ITEM_CREATION_H
