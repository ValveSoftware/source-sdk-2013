//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches Fizz tempentity
//-----------------------------------------------------------------------------
class CTEFizz : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFizz, CBaseTempEntity );

					CTEFizz( const char *name );
	virtual			~CTEFizz( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

	virtual void	Precache( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nEntity );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nDensity );
	CNetworkVar( int, m_nCurrent );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFizz::CTEFizz( const char *name ) :
	CBaseTempEntity( name )
{
	m_nEntity = 0;
	m_nModelIndex = 0;
	m_nDensity = 0;
	m_nCurrent = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFizz::~CTEFizz( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEFizz::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = CBaseEntity::PrecacheModel( "sprites/bubble.vmt" );;
	m_nDensity = 200;
	m_nEntity = 1;
	m_nCurrent = 100;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTEFizz::Precache( void )
{
	CBaseEntity::PrecacheModel( "sprites/bubble.vmt" );
}


IMPLEMENT_SERVERCLASS_ST(CTEFizz, DT_TEFizz)
	SendPropInt( SENDINFO(m_nEntity), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nDensity), 8, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_nCurrent), 16 ),
END_SEND_TABLE()


// Singleton to fire TEFizz objects
static CTEFizz g_TEFizz( "Fizz" );

void TE_Fizz( IRecipientFilter& filter, float delay,
	const CBaseEntity *entity, int modelindex, int density, int current )
{
	Assert( entity );

	g_TEFizz.m_nEntity		= ENTINDEX( (edict_t *)entity->edict() );
	g_TEFizz.m_nModelIndex	= modelindex;	
	g_TEFizz.m_nDensity		= density;
	g_TEFizz.m_nCurrent		= current;

	// Send it over the wire
	g_TEFizz.Create( filter, delay );
}