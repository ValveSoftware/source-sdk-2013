//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========
//
// Purpose: Simple model entity that randomly moves and changes direction
//			when activated.
//
//=============================================================================

#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "items.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"

class CCompanionCube : public CItem
{
public:
	DECLARE_CLASS( CCompanionCube, CItem );
	DECLARE_DATADESC();
	void Spawn( void );
	void Precache( void );
	void ItemTouch ( CBaseEntity *pOther );
private:
};

LINK_ENTITY_TO_CLASS( companioncube, CCompanionCube );
PRECACHE_REGISTER(companioncube);

// Start of our data description for the class
BEGIN_DATADESC( CCompanionCube )
	// Function Pointers
	//DEFINE_ENTITYFUNC( ItemTouch ),
	// Outputs
	DEFINE_OUTPUT( m_OnPlayerTouch, "OnPlayerTouch" ),
END_DATADESC()

// Name of our entity's model
#define	ENTITY_MODEL	"models/props/metal_box.mdl"

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CCompanionCube::Precache( void )
{
	PrecacheModel( ENTITY_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CCompanionCube::Spawn( void )
{
	Precache();
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	
	//Don't want turrets seeing us if crouched behind it.
	SetBlocksLOS( true );
	
	SetTouch(&CCompanionCube::ItemTouch);
	
	SetModel( ENTITY_MODEL );
	UTIL_SetSize( this, -Vector(10,10,10), Vector(10,10,10) );

	if ( CreateItemVPhysicsObject() == false )
		return;

	m_takedamage = DAMAGE_EVENTS_ONLY;


#if defined( HL2MP )
	SetThink( &CItem::FallThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CCompanionCube::ItemTouch( CBaseEntity *pOther )
{
Msg ("Item touch!");
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// Must be a valid pickup scenario (no blocking). Though this is a more expensive
	// check than some that follow, this has to be first Obecause it's the only one
	// that inhibits firing the output OnCacheInteraction.
	if ( ItemCanBeTouchedByPlayer( pPlayer ) == false )
		return;

	//m_OnCacheInteraction.FireOutput(pOther, this);

	// Can I even pick stuff up?
	if ( !pPlayer->IsAllowedToPickupWeapons() )
		return;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if ( MyTouch( pPlayer ) )
	{
	//	m_OnPlayerTouch.FireOutput(pOther, this);

		SetTouch( NULL );
		SetThink( NULL );

		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
	}
/*	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}*/
}