//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Header file for player-thrown grenades.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_BASE_H
#define GRENADE_BASE_H
#pragma once

#include "basegrenade_shared.h"

class CSprite;

#define GRENADE_TIMER		5		// Try 5 seconds instead of 3?

//-----------------------------------------------------------------------------
// Purpose: Base Thrown-Grenade class
//-----------------------------------------------------------------------------
class CThrownGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CThrownGrenade, CBaseGrenade );

	void	Spawn( void );
	void	Thrown( Vector vecOrigin, Vector vecVelocity, float flExplodeTime );
};



#endif // GRENADE_BASE_H
