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
// Purpose: Displays a dynamic light
//-----------------------------------------------------------------------------
class CTEDynamicLight : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEDynamicLight, CBaseTempEntity );

					CTEDynamicLight( const char *name );
	virtual			~CTEDynamicLight( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( float, m_fRadius );
	CNetworkVar( int, r );
	CNetworkVar( int, g );
	CNetworkVar( int, b );
	CNetworkVar( int, exponent );
	CNetworkVar( float, m_fTime );
	CNetworkVar( float, m_fDecay );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEDynamicLight::CTEDynamicLight( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	r = 0;
	g = 0;
	b = 0;
	exponent = 0;
	m_fRadius = 0.0;
	m_fTime = 0.0;
	m_fDecay = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEDynamicLight::~CTEDynamicLight( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEDynamicLight::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	r = 255;
	g = 255;
	b = 63;
	m_vecOrigin = current_origin;

	m_fRadius = 200;
	m_fTime = 2.0;
	m_fDecay = 0.0;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEDynamicLight, DT_TEDynamicLight)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropInt( SENDINFO(r), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(exponent), 8, 0 ),
	SendPropFloat( SENDINFO(m_fRadius), 8, SPROP_ROUNDUP, 0, 2560.0 ),
	SendPropFloat( SENDINFO(m_fTime), 8, SPROP_ROUNDDOWN, 0, 25.6 ),
	SendPropFloat( SENDINFO(m_fDecay), 8, SPROP_ROUNDDOWN, 0, 2560.0 ),
END_SEND_TABLE()


// Singleton
static CTEDynamicLight g_TEDynamicLight( "Dynamic Light" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*org - 
//			r - 
//			g - 
//			b - 
//			radius - 
//			time - 
//			decay - 
//-----------------------------------------------------------------------------
void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay )
{
	// Set up parameters
	g_TEDynamicLight.m_vecOrigin = *org;
	g_TEDynamicLight.r = r;
	g_TEDynamicLight.g = g;
	g_TEDynamicLight.b = b;
	g_TEDynamicLight.exponent = exponent;
	g_TEDynamicLight.m_fRadius = radius;
	g_TEDynamicLight.m_fTime = time;
	g_TEDynamicLight.m_fDecay = decay;

	// Create it
	g_TEDynamicLight.Create( filter, delay );
}