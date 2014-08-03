//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== item_suit.cpp ========================================================

  handling for the player's suit.
*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
public:
	DECLARE_CLASS( CItemSuit, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/hevsuit.mdl" );
		BaseClass::Spawn( );
		
		CollisionProp()->UseTriggerBounds( false, 0 );
	}
	void Precache( void )
	{
		PrecacheModel ("models/items/hevsuit.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
	    //SecobMod__Information: These two lines are commented out for play-through of official hl2/ep1/ep2 maps. Your choice whether to re-enable it or not.
		//if ( pPlayer->IsSuitEquipped() )
		//return FALSE;

		if ( m_spawnflags & SF_SUIT_SHORTLOGON )
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A0");		// short version of suit logon,
		else
			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_AAx");	// long version of suit logon

		pPlayer->EquipSuit();
				
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);
