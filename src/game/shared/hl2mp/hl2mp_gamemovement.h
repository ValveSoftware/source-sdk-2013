//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game movement for Half-Life 2: Deathmatch
//
//=============================================================================//

#ifndef HL2MP_GAMEMOVEMENT_H
#define HL2MP_GAMEMOVEMENT_H
#pragma once

#include "hl_gamemovement.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#define CHL2MP_Player C_HL2MP_Player
#else
#include "hl2mp_player.h"
#endif

class CHL2MPGameMovement : public CHL2GameMovement
{
public:
	DECLARE_CLASS( CHL2MPGameMovement, CHL2GameMovement )

	CHL2MPGameMovement();

	virtual bool CheckJumpButton( void );

private:
	CHL2MP_Player *GetHL2MPPlayer( void ) { return ToHL2MPPlayer( player ); }
};

#endif // HL2MP_GAMEMOVEMENT_H
