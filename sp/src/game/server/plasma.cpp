//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "plasma.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//==================================================
// CPlasma
//==================================================

//Link the entity
LINK_ENTITY_TO_CLASS( _plasma, CPlasma );

//Send datatable
IMPLEMENT_SERVERCLASS_ST( CPlasma, DT_Plasma )
	SendPropFloat(	SENDINFO( m_flScale ),		0,	SPROP_NOSCALE),
	SendPropFloat(	SENDINFO( m_flScaleTime ),	0,	SPROP_NOSCALE),
	SendPropInt(	SENDINFO( m_nFlags ),		8,  SPROP_UNSIGNED ),
	SendPropModelIndex( SENDINFO( m_nPlasmaModelIndex )),
	SendPropModelIndex( SENDINFO( m_nPlasmaModelIndex2 )),
	SendPropModelIndex( SENDINFO( m_nGlowModelIndex )),
END_SEND_TABLE()

//Data description 
BEGIN_DATADESC( CPlasma )

	//Client-side
	DEFINE_FIELD( m_flScale,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flScaleTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nFlags,			FIELD_INTEGER ),

//	DEFINE_FIELD( m_nPlasmaModelIndex, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nPlasmaModelIndex2, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nGlowModelIndex,	FIELD_INTEGER ),

	//Server-side

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CPlasma::CPlasma( void ) 
{
	//Client-side
	m_flScale				= 0.0f;
	m_flScaleTime			= 0.0f;
	m_nFlags				= bitsFIRE_NONE;
	m_nPlasmaModelIndex		= PrecacheModel( "sprites/plasma1.vmt" );
	m_nPlasmaModelIndex2	= PrecacheModel( "sprites/plasma1.vmt" );//<<TEMP>>
	m_nGlowModelIndex		= PrecacheModel( "sprites/fire_floor.vmt" );
	//Server-side
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlasma::~CPlasma( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CPlasma::EnableSmoke( int state )
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
//-----------------------------------------------------------------------------
void CPlasma::Precache( void )
{
	PrecacheModel( "sprites/plasma1.vmt" );
	PrecacheModel( "sprites/fire_floor.vmt" );
}
