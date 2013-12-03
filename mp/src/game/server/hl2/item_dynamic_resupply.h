//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ITEM_DYNAMIC_RESUPPLY_H
#define ITEM_DYNAMIC_RESUPPLY_H

#ifdef _WIN32
#pragma once
#endif


// Spawnflags
#define SF_DYNAMICRESUPPLY_USE_MASTER			1
#define SF_DYNAMICRESUPPLY_IS_MASTER			2
#define SF_DYNAMICRESUPPLY_ALWAYS_SPAWN			4	// even if the player has met his target
#define SF_DYNAMICRESUPPLY_FALLBACK_TO_VIAL		8	// If we fail to spawn anything, spawn a health vial
#define SF_DYNAMICRESUPPLY_ALTERNATE_MASTER		16	// Don't assume role as master on activate, but don't think either

float DynamicResupply_GetDesiredHealthPercentage( void );
void DynamicResupply_InitFromAlternateMaster( CBaseEntity *pResupply, string_t iszMaster );

#endif // ITEM_DYNAMIC_RESUPPLY_H
