//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_flag.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_team.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlag, DT_TFWeaponFlag )

BEGIN_NETWORK_TABLE( CTFFlag, DT_TFWeaponFlag )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlag )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flag, CTFFlag );
PRECACHE_WEAPON_REGISTER( tf_weapon_flag );

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlag::CTFFlag()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlag::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef GAME_DLL
        TFTeamMgr()->PlayerCenterPrint( ToTFPlayer( GetOwner() ), "#TF_Flag_AltFireToDrop" );
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlag::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	pPlayer->DropCurrentWeapon();
#endif
	pPlayer->SwitchToNextBestWeapon( this );
}

