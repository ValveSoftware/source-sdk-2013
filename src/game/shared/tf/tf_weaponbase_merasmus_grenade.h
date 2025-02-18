//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Halloween boss Merasmus grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_MERASMUS_GRENADE_H
#define TF_WEAPONBASE_MERASMUS_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase_grenadeproj.h"


// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponBaseMerasmusGrenade C_TFWeaponBaseMerasmusGrenade
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFWeaponBaseMerasmusGrenade : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFWeaponBaseMerasmusGrenade, CBaseGrenade );
	DECLARE_NETWORKCLASS();

	virtual int GetDamageCustom();
	virtual int GetCustomParticleIndex();

	// Client specific.
#ifdef CLIENT_DLL

public:
	virtual int	DrawModel( int flags );

#endif

};

#endif // TF_WEAPONBASE_MERASMUS_GRENADE_H
