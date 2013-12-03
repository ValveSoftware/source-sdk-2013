//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_GRENADE_H
#define WEAPON_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_basesdkgrenade.h"


#ifdef CLIENT_DLL
	
	#define CSDKGrenade C_SDKGrenade

#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CSDKGrenade : public CBaseSDKGrenade
{
public:
	DECLARE_CLASS( CSDKGrenade, CBaseSDKGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CSDKGrenade() {}

	virtual SDKWeaponID GetWeaponID( void ) const		{ return WEAPON_GRENADE; }

#ifdef CLIENT_DLL

#else
	DECLARE_DATADESC();

	virtual void EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer );
	
#endif

	CSDKGrenade( const CSDKGrenade & ) {}
};


#endif // WEAPON_GRENADE_H
