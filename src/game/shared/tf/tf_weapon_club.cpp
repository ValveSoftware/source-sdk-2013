//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_club.h"
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
// Weapon Club tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFClub, DT_TFWeaponClub )

BEGIN_NETWORK_TABLE( CTFClub, DT_TFWeaponClub )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFClub )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_club, CTFClub );
PRECACHE_WEAPON_REGISTER( tf_weapon_club );

//=============================================================================
//
// Weapon Club functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFClub::CTFClub()
{
}
