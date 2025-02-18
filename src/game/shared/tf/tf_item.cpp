//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================//
#include "cbase.h"
#include "tf_item.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"

// NVNT haptics system interface
#include "haptics/ihaptics.h"
#else
#include "tf_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFItem, DT_TFItem )

BEGIN_NETWORK_TABLE( CTFItem, DT_TFItem )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Identifier.
//-----------------------------------------------------------------------------
unsigned int CTFItem::GetItemID( void ) const
{ 
	return TF_ITEM_UNDEFINED; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItem::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// SetParent with attachment point - look it up later if need be!
	SetOwnerEntity( pPlayer );
	SetParent( pPlayer );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );

	// Make invisible?
	if ( bInvisible )
	{
		AddEffects( EF_NODRAW );
	}

	// Add the item to the player's item inventory.
	pPlayer->SetItem( this );
	// NVNT if this is the client dll and the owner is the local
	//  player notify the haptics system.
#ifdef CLIENT_DLL
	if(pPlayer->IsLocalPlayer())
		haptics->ProcessHapticEvent(2,"Game","ctf_item_start");
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItem::Drop( CTFPlayer *pPlayer, bool bVisible, bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Remove the item from the player's item inventory.
	pPlayer->SetItem( NULL );

	// Make visible?
	if ( bVisible )
	{
		RemoveEffects( EF_NODRAW );
	}
	// NVNT if this is the client dll and the owner is the local
	//  player notify the haptics system we are dropping this item.
#ifdef CLIENT_DLL
	if(pPlayer->IsLocalPlayer())
		haptics->ProcessHapticEvent(2,"Game","ctf_item_stop");
#endif

	// Clear the parent.
	SetParent( NULL );
	SetOwnerEntity( NULL );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFItem::ShouldDraw()
{
	// If I'm carrying the flag in 1st person, don't draw it
	if ( ToTFPlayer(GetMoveParent())->InFirstPersonView() )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t CTFItem::ShadowCastType()
{
	if ( ToTFPlayer(GetMoveParent())->ShouldDrawThisPlayer() )
	{
		// Using the viewmodel.
		return SHADOWS_NONE;
	}

	return BaseClass::ShadowCastType();
}

#endif
