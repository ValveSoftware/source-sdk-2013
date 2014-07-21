//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== item_security.cpp ========================================================

  handling for the security item
*/

#include "cbase.h"
#include "player.h"
//#include "weapons.h"
#include "gamerules.h"
#include "items.h"

class CItemSecurity : public CItem
{
public:
	DECLARE_CLASS( CItemSecurity, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/w_security.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/w_security.mdl");
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

