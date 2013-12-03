//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple test entity for creating special effects
//
//=============================================================================//

#include "cbase.h"
#include "te_effect_dispatch.h"

// Declare the sparkler entity for the server-side
class CSparkler : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CSparkler, CBaseEntity );

	void Spawn( void );
	
	void InputToggle( inputdata_t &input );	// Input function for toggling our effect's state
	void InputScale( inputdata_t &input );

private:
	CNetworkVar( bool, m_bEmit );		// Marks whether the effect should be active or not
	CNetworkVar( float, m_flScale );	// The size and speed of the effect

	DECLARE_DATADESC();
};

// Link our class to the "env_sparkler" entity classname
LINK_ENTITY_TO_CLASS( env_sparkler, CSparkler );

// Declare our data description for this entity
BEGIN_DATADESC( CSparkler )
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),	// Declare our toggle input function
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Scale", InputScale ),
END_DATADESC()

// Declare the data-table for server/client communication
IMPLEMENT_SERVERCLASS_ST( CSparkler, DT_Sparkler )
	SendPropInt( SENDINFO( m_bEmit ), 1, SPROP_UNSIGNED ),	// Declare our boolean state variable
	SendPropFloat( SENDINFO( m_flScale ), 0, SPROP_NOSCALE ), 
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Spawn function for this entity
//-----------------------------------------------------------------------------
void CSparkler::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );	// Will not move on its own
	SetSolid( SOLID_NONE );			// Will not collide with anything

	// Set a size for culling
	UTIL_SetSize( this, -Vector(2,2,2), Vector(2,2,2) );

	// We must add this flag to receive network transmitions
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: Toggles the emission state of the effect
//-----------------------------------------------------------------------------
void CSparkler::InputToggle( inputdata_t &input )
{
	// Toggle our state
	m_bEmit = !m_bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: Change our scale via a float value
//-----------------------------------------------------------------------------
void CSparkler::InputScale( inputdata_t &input )
{
	// Change our scale
	m_flScale = input.value.Float();
}

// ============================================================================
//
//   Dispatch Effect version
//
// ============================================================================

//-----------------------------------------------------------------------------
// Purpose: Create a sparkle effect at the given location of the given size
// Input  : &position - Where to emit from
//			flSize - Size of the effect
//-----------------------------------------------------------------------------
void MakeSparkle( const Vector &origin, float flScale )
{
	CEffectData data;
	
	// Send our origin
	data.m_vOrigin = origin;
	
	// Send our scale
	data.m_flScale = flScale;

	// Send the effect off to the client
	DispatchEffect( "Sparkle", data );
}