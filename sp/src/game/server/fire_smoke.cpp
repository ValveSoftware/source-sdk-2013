//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "fire_smoke.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseFire )

	DEFINE_FIELD( m_flStartScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_flScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_flScaleTime, FIELD_TIME ),
	DEFINE_FIELD( m_nFlags, FIELD_INTEGER ),

END_DATADESC()


//==================================================
// CBaseFire
//==================================================

CBaseFire::CBaseFire( void )
{
	m_flStartScale		= 0.0f;
	m_flScale			= 0.0f;
	m_flScaleTime		= 0.0f;
	m_nFlags			= bitsFIRE_NONE;
}

CBaseFire::~CBaseFire( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Take the current scale of the flame and move it towards a destination
// Input  : size - destination size
//			time - time to scale across
//-----------------------------------------------------------------------------
void CBaseFire::Scale( float size, float time )
{
	//Send to the client
	m_flScale		= size;
	m_flScaleTime	= time;
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded Scale() function to set size
// Input  : start - beginning sizek
//			size - destination size
//			time - time to scale across
//-----------------------------------------------------------------------------
void CBaseFire::Scale( float start, float size, float time )
{
	//Send to the client
	m_flStartScale	= start;
	m_flScale		= size;
	m_flScaleTime	= time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CBaseFire::Enable( int state )
{
	if ( state )
	{
		m_nFlags |= bitsFIRE_ACTIVE;
	}
	else
	{
		m_nFlags &= ~bitsFIRE_ACTIVE;
	}
}

//==================================================
// CFireSmoke
//==================================================

//Link the entity
LINK_ENTITY_TO_CLASS( _firesmoke, CFireSmoke );

//Send datatable
IMPLEMENT_SERVERCLASS_ST( CFireSmoke, DT_FireSmoke )
	SendPropFloat(  SENDINFO( m_flStartScale ), 0,	SPROP_NOSCALE),
	SendPropFloat(	SENDINFO( m_flScale ),		0,	SPROP_NOSCALE),
	SendPropFloat(	SENDINFO( m_flScaleTime ),	0,	SPROP_NOSCALE),
	SendPropInt(	SENDINFO( m_nFlags ),		8,  SPROP_UNSIGNED ),
	SendPropModelIndex(	SENDINFO( m_nFlameModelIndex ) ),
	SendPropModelIndex(	SENDINFO( m_nFlameFromAboveModelIndex ) ),
END_SEND_TABLE()

//Data description 
BEGIN_DATADESC( CFireSmoke )

	DEFINE_FIELD( m_flStartScale,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flScale,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flScaleTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nFlags,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nFlameModelIndex,	FIELD_MODELINDEX ),
	DEFINE_FIELD( m_nFlameFromAboveModelIndex,	FIELD_MODELINDEX ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CFireSmoke::CFireSmoke( void ) 
{
	//Client-side
	m_flScale			= 0.0f;
	m_flScaleTime		= 0.0f;
	m_nFlags			= bitsFIRE_NONE;

	//Server-side
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFireSmoke::~CFireSmoke( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFireSmoke::Precache()
{
	BaseClass::Precache();
}

void CFireSmoke::Spawn()
{
	Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CFireSmoke::EnableSmoke( int state )
{
	if ( state )
	{
		m_nFlags |= bitsFIRESMOKE_SMOKE;
	}
	else
	{
		m_nFlags &= ~bitsFIRESMOKE_SMOKE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CFireSmoke::EnableGlow( int state )
{
	if ( state )
	{
		m_nFlags |= bitsFIRESMOKE_GLOW;
	}
	else
	{
		m_nFlags &= ~bitsFIRESMOKE_GLOW;
	}
}

void CFireSmoke::EnableVisibleFromAbove( int state )
{
	if ( state )
	{
		m_nFlags |= bitsFIRESMOKE_VISIBLE_FROM_ABOVE;
	}
	else
	{
		m_nFlags &= ~bitsFIRESMOKE_VISIBLE_FROM_ABOVE;
	}
}