//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Armor.
//
//=============================================================================//
#ifndef ENTITY_ARMOR_H
#define ENTITY_ARMOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF Armor class.
//

class CArmor : public CTFPowerup
{
public:
	DECLARE_CLASS( CArmor, CTFPowerup );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/car_battery01.mdl"; }
};

#endif // ENTITY_ARMOR_H


