//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Armor.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_armor.h"

//=============================================================================
//
// CTF Armor defines.
//

#define TF_ARMOR_PICKUP_SOUND	"Armor.Touch"
#define TF_ARMOR_CAPACITY		200

LINK_ENTITY_TO_CLASS( item_armor, CArmor );

//=============================================================================
//
// CTF Armor functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the armor
//-----------------------------------------------------------------------------
void CArmor::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the armor
//-----------------------------------------------------------------------------
void CArmor::Precache( void )
{
	PrecacheScriptSound( TF_ARMOR_PICKUP_SOUND );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the armor
//-----------------------------------------------------------------------------
bool CArmor::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pCTFPlayer = ToTFPlayer(pPlayer);

		if ( pCTFPlayer )
		{
			int iMaxArmor = pCTFPlayer->GetPlayerClass()->GetMaxArmor();
			int iCurrentArmor = pCTFPlayer->ArmorValue();

			if ( iCurrentArmor < iMaxArmor )
			{
				if ( iCurrentArmor + TF_ARMOR_CAPACITY >= iMaxArmor )
				{
					pCTFPlayer->SetArmorValue( iMaxArmor );
				}
				else
				{
					pCTFPlayer->SetArmorValue( iCurrentArmor + TF_ARMOR_CAPACITY );
				}

				CSingleUserRecipientFilter user( pPlayer );
				user.MakeReliable();

				UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( GetClassname() );
				MessageEnd();

				CPASAttenuationFilter filter( this, TF_ARMOR_PICKUP_SOUND );
				EmitSound( filter, entindex(), TF_ARMOR_PICKUP_SOUND );

				bSuccess = true;
			}
		}
	}

	return bSuccess;
}
