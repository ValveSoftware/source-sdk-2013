//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple brush entity that moves when touched
//
//=============================================================================//

#include "cbase.h"

class CMyBrushEntity : public CBaseToggle
{
public:
	DECLARE_CLASS( CMyBrushEntity, CBaseToggle );
	DECLARE_DATADESC();

	void Spawn( void );
	bool CreateVPhysics( void );

	void BrushTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( my_brush_entity, CMyBrushEntity );

// Start of our data description for the class
BEGIN_DATADESC( CMyBrushEntity )
	
	// Declare this function as being a touch function
	DEFINE_ENTITYFUNC( BrushTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CMyBrushEntity::Spawn( void )
{
	// We want to capture touches from other entities
	SetTouch( &CMyBrushEntity::BrushTouch );

	// We should collide with physics
	SetSolid( SOLID_VPHYSICS );
	
	// We push things out of our way
	SetMoveType( MOVETYPE_PUSH );
	
	// Use our brushmodel
	SetModel( STRING( GetModelName() ) );

	// Create our physics hull information
	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: Setup our physics information so we collide properly
//-----------------------------------------------------------------------------
bool CMyBrushEntity::CreateVPhysics( void )
{
	// For collisions with physics objects
	VPhysicsInitShadow( false, false );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Move away from an entity that touched us
// Input  : *pOther - the entity we touched
//-----------------------------------------------------------------------------
void CMyBrushEntity::BrushTouch( CBaseEntity *pOther )
{
	// Get the collision information
	const trace_t &tr = GetTouchTrace();

	// We want to move away from the impact point along our surface
	Vector	vecPushDir = tr.plane.normal;
	vecPushDir.Negate();
	vecPushDir.z = 0.0f;

	// Move slowly in that direction
	LinearMove( GetAbsOrigin() + ( vecPushDir * 64.0f ), 32.0f );
}
