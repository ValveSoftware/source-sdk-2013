//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_nailgun.h"

//=============================================================================
//
// Weapon SMG tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFNailgun, DT_WeaponNailgun )

BEGIN_NETWORK_TABLE( CTFNailgun, DT_WeaponNailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFNailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_nailgun, CTFNailgun );
PRECACHE_WEAPON_REGISTER( tf_weapon_nailgun);

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFNailgun )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Nailgun functions.
//