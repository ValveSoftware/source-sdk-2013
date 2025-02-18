//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_boomerang.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Boomerang tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBoomerang, DT_TFWeaponBoomerang )

BEGIN_NETWORK_TABLE( CTFBoomerang, DT_TFWeaponBoomerang )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBoomerang )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_boomerang, CTFBoomerang );
PRECACHE_WEAPON_REGISTER( tf_weapon_boomerang );

//=============================================================================
//
// Weapon Boomerang functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBoomerang::CTFBoomerang()
{
}
