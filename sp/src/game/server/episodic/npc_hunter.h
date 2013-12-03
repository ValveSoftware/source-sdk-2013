//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Expose an IsAHunter function
//
//=============================================================================//

#ifndef NPC_HUNTER_H
#define NPC_HUNTER_H

#if defined( _WIN32 )
#pragma once
#endif

class CBaseEntity;

/// true if given entity pointer is a hunter.
bool Hunter_IsHunter(CBaseEntity *pEnt);

// call throughs for member functions

void Hunter_StriderBusterAttached( CBaseEntity *pHunter, CBaseEntity *pAttached );
void Hunter_StriderBusterDetached( CBaseEntity *pHunter, CBaseEntity *pAttached );
void Hunter_StriderBusterLaunched( CBaseEntity *pBuster );

#endif
