//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_decoy.h"


#if !defined( CLIENT_DLL )
	#include "tf_player.h"
	#include "bot_npc/bot_npc_decoy.h"
#else
	#include "c_tf_player.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFDecoy, DT_WeaponDecoy )

BEGIN_NETWORK_TABLE( CTFDecoy, DT_WeaponDecoy )
END_NETWORK_TABLE()


/*
// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFDecoy )
END_DATADESC()
#endif
*/

LINK_ENTITY_TO_CLASS( tf_weapon_decoy, CTFDecoy );
PRECACHE_WEAPON_REGISTER( tf_weapon_decoy );


//-----------------------------------------------------------------------------
CTFDecoy::CTFDecoy()
{
}


//-----------------------------------------------------------------------------
void CTFDecoy::PrimaryAttack( void )
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

#if !defined( CLIENT_DLL )
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	if ( !CanAttack() )
		return;

	CBotNPCDecoy *decoy = (CBotNPCDecoy *)CreateEntityByName( "bot_npc_decoy" );
	if ( decoy )
	{
		decoy->SetOwnerEntity( pOwner );
		DispatchSpawn( decoy );

		m_flNextPrimaryAttack = gpGlobals->curtime + 5.0f;
	}
#endif // CLIENT_DLL
}

