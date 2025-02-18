//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENERGYWAVE_H
#define ENERGYWAVE_H
#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon.h"
#include "energy_wave.h"


//-----------------------------------------------------------------------------
// Purpose: Shield
//-----------------------------------------------------------------------------
class CEnergyWave : public CBaseEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CEnergyWave, CBaseEntity );
	DECLARE_SERVERCLASS();

public:
	void Spawn( void );
	void Precache( void );

public:
	static CEnergyWave* Create( CBaseEntity *pentOwner );
};


#endif //ENERGYWAVE_H
