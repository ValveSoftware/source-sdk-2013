//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game movement for Half-Life 2: Deathmatch
//
//=============================================================================//

#include "cbase.h"
#include "hl2mp_gamemovement.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL2MPGameMovement::CHL2MPGameMovement()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2MPGameMovement::CheckJumpButton( void )
{
	bool bJump = BaseClass::CheckJumpButton();

	CHL2MP_Player *pPlayer = GetHL2MPPlayer();

	if ( !pPlayer )
		return bJump;

	if ( bJump )
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );

	return bJump;
}

static CHL2MPGameMovement g_GameMovement;

IGameMovement *g_pGameMovement = (IGameMovement *)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );
