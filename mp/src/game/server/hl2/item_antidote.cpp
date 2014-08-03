//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== item_antidote.cpp ========================================================

  handling for the antidote object
*/
 
#include "cbase.h"
#include "player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"

class CItemAntidote : public CItem
{
public:
	DECLARE_CLASS( CItemAntidote, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_antidote.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/w_antidote.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);
		
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);
