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
#include "te_particlesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

//-----------------------------------------------------------------------------
// Purpose: Dispatches smoke tempentity
//-----------------------------------------------------------------------------
class CTELargeFunnel : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTELargeFunnel, CTEParticleSystem );
	DECLARE_SERVERCLASS();

					CTELargeFunnel( const char *name );
	virtual			~CTELargeFunnel( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
public:
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nReversed );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTELargeFunnel::CTELargeFunnel( const char *name ) :
	BaseClass( name )
{
	m_nModelIndex = 0;
	m_nReversed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTELargeFunnel::~CTELargeFunnel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTELargeFunnel::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = g_sModelIndexSmoke;
	m_nReversed = 0;
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin.Get(), 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin.Get(), 25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTELargeFunnel, DT_TELargeFunnel)
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nReversed), 2, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TELargeFunnel objects
static CTELargeFunnel g_TELargeFunnel( "Large Funnel" );

void TE_LargeFunnel( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, int reversed )
{
	g_TELargeFunnel.m_vecOrigin		= *pos;
	g_TELargeFunnel.m_nModelIndex	= modelindex;	
	g_TELargeFunnel.m_nReversed		= reversed;

	// Send it over the wire
	g_TELargeFunnel.Create( filter, delay );
}