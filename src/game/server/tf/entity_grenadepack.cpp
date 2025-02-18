//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF GrenadePack.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_grenadepack.h"

//=============================================================================
//
// CTF GrenadePack defines.
//

#define TF_GRENADEPACK_MODEL			"models/items/grenade_pack.mdl"
#define TF_GRENADEPACK_PICKUP_SOUND		"GrenadePack.Touch"
#define TF_GRENADEPACK_GRENADES1		4
#define TF_GRENADEPACK_GRENADES2		4

LINK_ENTITY_TO_CLASS( item_grenadepack, CGrenadePack );

//=============================================================================
//
// CTF GrenadePack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the grenadepack
//-----------------------------------------------------------------------------
void CGrenadePack::Spawn( void )
{
	Precache();
	SetModel( TF_GRENADEPACK_MODEL );

	BaseClass::Spawn();

	// Grenades have been removed, so remove ourself.
	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the grenadepack
//-----------------------------------------------------------------------------
void CGrenadePack::Precache( void )
{
	PrecacheModel( TF_GRENADEPACK_MODEL );
	PrecacheScriptSound( TF_GRENADEPACK_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the grenadepack
//-----------------------------------------------------------------------------
bool CGrenadePack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		// try to give primary grenades
		if ( pPlayer->GiveAmmo( TF_GRENADEPACK_GRENADES1, TF_AMMO_GRENADES1, true ) )
		{
			bSuccess = true;
		}

		// try to give secondary grenades
		if ( pPlayer->GiveAmmo( TF_GRENADEPACK_GRENADES2, TF_AMMO_GRENADES2, true ) )
		{
			bSuccess = true;
		}

		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TF_GRENADEPACK_PICKUP_SOUND );
		}
	}

	return bSuccess;
}
