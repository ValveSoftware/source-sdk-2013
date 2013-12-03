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

//-----------------------------------------------------------------------------
// Purpose: Dispatches blood stream tempentity
//-----------------------------------------------------------------------------
class CTEBloodStream : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTEBloodStream, CTEParticleSystem );

					CTEBloodStream( const char *name );
	virtual			~CTEBloodStream( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecDirection );
	CNetworkVar( int, r );
	CNetworkVar( int, g );
	CNetworkVar( int, b );
	CNetworkVar( int, a );
	CNetworkVar( int, m_nAmount );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBloodStream::CTEBloodStream( const char *name ) :
	BaseClass( name )
{
	m_vecDirection.Init();
	r = 0;
	g = 0;
	b = 0;
	a = 0;
	m_nAmount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBloodStream::~CTEBloodStream( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBloodStream::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	r = 247;
	g = 0;
	b = 0;
	a = 255;
	m_nAmount	= random->RandomInt(50, 150);
	m_vecOrigin = current_origin;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;	
	VectorNormalize( forward );

	m_vecOrigin += forward * 50;

	m_vecDirection = UTIL_RandomBloodVector();

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBloodStream, DT_TEBloodStream)
	SendPropVector( SENDINFO(m_vecDirection), 11, 0, -10.0, 10.0 ),
	SendPropInt( SENDINFO(r), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(a), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nAmount), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton to fire TEBloodStream objects
static CTEBloodStream g_TEBloodStream( "Blood Stream" );

//-----------------------------------------------------------------------------
// Purpose: Creates a blood stream
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*org - 
//			*dir - 
//			r - 
//			g - 
//			b - 
//			a - 
//			amount - 
//-----------------------------------------------------------------------------
void TE_BloodStream( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector* dir, int r, int g, int b, int a, int amount )
{
	g_TEBloodStream.m_vecOrigin = *org;
	g_TEBloodStream.m_vecDirection = *dir;	
	g_TEBloodStream.r = r;
	g_TEBloodStream.g = g;
	g_TEBloodStream.b = b;
	g_TEBloodStream.a = a;
	g_TEBloodStream.m_nAmount = amount;

	// Send it over the wire
	g_TEBloodStream.Create( filter, delay );
}