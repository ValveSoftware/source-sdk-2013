//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_SMOKEGRENADE_H
#define WEAPON_SMOKEGRENADE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_basecsgrenade.h"


#ifdef CLIENT_DLL
	
	#define CSmokeGrenade C_SmokeGrenade

#endif


//-----------------------------------------------------------------------------
// Smoke grenades
//-----------------------------------------------------------------------------
class CSmokeGrenade : public CBaseCSGrenade
{
public:
	DECLARE_CLASS( CSmokeGrenade, CBaseCSGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CSmokeGrenade() {}

	virtual CSWeaponID GetWeaponID( void ) const { return WEAPON_SMOKEGRENADE; }

#ifdef CLIENT_DLL

#else
	DECLARE_DATADESC();

	void EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer );

#endif

	CSmokeGrenade( const CSmokeGrenade & ) {}
};


#endif // WEAPON_SMOKEGRENADE_H
