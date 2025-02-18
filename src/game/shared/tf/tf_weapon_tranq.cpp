//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_tranq.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Tranq tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFTranq, DT_WeaponTranq )

BEGIN_NETWORK_TABLE( CTFTranq, DT_WeaponTranq )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFTranq )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_tranq, CTFTranq );
PRECACHE_WEAPON_REGISTER( tf_weapon_tranq );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFTranq )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Tranq functions.
//
