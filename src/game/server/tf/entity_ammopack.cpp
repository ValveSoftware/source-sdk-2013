//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_ammopack.h"
#include "tf_gamestats.h"

//=============================================================================
//
// CTF AmmoPack defines.
//

#define TF_AMMOPACK_PICKUP_SOUND	"AmmoPack.Touch"

LINK_ENTITY_TO_CLASS( item_ammopack_full, CAmmoPack );
LINK_ENTITY_TO_CLASS( item_ammopack_small, CAmmoPackSmall );
LINK_ENTITY_TO_CLASS( item_ammopack_medium, CAmmoPackMedium );

//=============================================================================
//
// CTF AmmoPack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Precache( void )
{
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );
	PrecacheModel( TF_AMMOPACK_LARGE_BDAY ); // always precache this for PyroVision

	BaseClass::Precache();

	UpdateModelIndexOverrides();
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CAmmoPack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		float flPackRatio = PackRatios[GetPowerupSize()];

		int iMaxPrimary = pTFPlayer->GetMaxAmmo(TF_AMMO_PRIMARY);
		if ( pTFPlayer->GiveAmmo( ceil(iMaxPrimary * flPackRatio), TF_AMMO_PRIMARY, true, kAmmoSource_Pickup ) )
		{
			bSuccess = true;
		}

		int iMaxSecondary = pTFPlayer->GetMaxAmmo(TF_AMMO_SECONDARY);
		if ( pTFPlayer->GiveAmmo( ceil(iMaxSecondary * flPackRatio), TF_AMMO_SECONDARY, true, kAmmoSource_Pickup ) )
		{
			bSuccess = true;
		}

		int iMaxMetal = pTFPlayer->GetMaxAmmo(TF_AMMO_METAL);
		if ( pTFPlayer->GiveAmmo( ceil(iMaxMetal * flPackRatio), TF_AMMO_METAL, true, kAmmoSource_Pickup ) )
		{
			bSuccess = true;
		}

		if ( pTFPlayer->m_Shared.AddToSpyCloakMeter( 100.0f * flPackRatio ) )
		{
			bSuccess = true;
		}

		if ( pTFPlayer->AddToSpyKnife( 100.0f * flPackRatio, false ) )
		{
			bSuccess = true;
		}

		int iAmmoIsCharge = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, iAmmoIsCharge, ammo_gives_charge );
		if ( iAmmoIsCharge )
		{
			float flCurrentCharge = pTFPlayer->m_Shared.GetDemomanChargeMeter();
			if ( flCurrentCharge < 100.0f )
			{
				if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
				{
					flPackRatio *= 0.2;
				}
				pTFPlayer->m_Shared.SetDemomanChargeMeter( flCurrentCharge + flPackRatio * 100.0f );
				bSuccess = true;
			}
		}

		if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			int iMaxGrenades1 = pTFPlayer->GetMaxAmmo(TF_AMMO_GRENADES1);
			if ( pTFPlayer->GiveAmmo( ceil(iMaxGrenades1 * flPackRatio), TF_AMMO_GRENADES1, true, kAmmoSource_Pickup ) )
			{
				bSuccess = true;
			}
		}

		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TF_AMMOPACK_PICKUP_SOUND );

			CTF_GameStats.Event_PlayerAmmokitPickup( pTFPlayer );

			IGameEvent * event = gameeventmanager->CreateEvent( "item_pickup" );
			if( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetString( "item", GetAmmoPackName() );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	return bSuccess;
}
