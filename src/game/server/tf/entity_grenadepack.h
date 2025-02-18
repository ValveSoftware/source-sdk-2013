//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF GrenadePack.
//
//=============================================================================//
#ifndef ENTITY_GRENADEPACK_H
#define ENTITY_GRENADEPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF GrenadePack class.
//

class CGrenadePack : public CTFPowerup
{
public:
	DECLARE_CLASS( CGrenadePack, CTFPowerup );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );
};

#endif // ENTITY_GRENADEPACK_H


