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

extern short		g_sModelIndexBloodDrop;		// (in combatweapon.cpp) holds the sprite index for the initial blood
extern short		g_sModelIndexBloodSpray;	// (in combatweapon.cpp) holds the sprite index for splattered blood

//-----------------------------------------------------------------------------
// Purpose: Display's a blood sprite
//-----------------------------------------------------------------------------
class CTEBloodSprite : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEBloodSprite, CBaseTempEntity );

					CTEBloodSprite( const char *name );
	virtual			~CTEBloodSprite( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecDirection );
	CNetworkVar( int, m_nSprayModel );
	CNetworkVar( int, m_nDropModel );
	CNetworkVar( int, r );
	CNetworkVar( int, g );
	CNetworkVar( int, b );
	CNetworkVar( int, a );
	CNetworkVar( int, m_nSize );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBloodSprite::CTEBloodSprite( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nSprayModel = 0;
	m_nDropModel = 0;
	r = 0;
	g = 0;
	b = 0;
	a = 0;
	m_nSize = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBloodSprite::~CTEBloodSprite( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBloodSprite::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	r = 255;
	g = 255;
	b = 63;
	a = 255;
	m_nSize	= 16;
	m_vecOrigin = current_origin;
	
	m_nSprayModel = g_sModelIndexBloodSpray;
	m_nDropModel = g_sModelIndexBloodDrop;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEBloodSprite, DT_TEBloodSprite)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDirection), -1, SPROP_COORD),
	SendPropInt( SENDINFO(r), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(a), 8, SPROP_UNSIGNED ),
	SendPropModelIndex( SENDINFO(m_nSprayModel) ),
	SendPropModelIndex( SENDINFO(m_nDropModel) ),
	SendPropInt( SENDINFO(m_nSize), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton
static CTEBloodSprite g_TEBloodSprite( "Blood Sprite" );

//-----------------------------------------------------------------------------
// Purpose: Public interface
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*org - 
//			r - 
//			g - 
//			b - 
//			a - 
//			size - 
//-----------------------------------------------------------------------------
void TE_BloodSprite( IRecipientFilter& filter, float delay,
	const Vector *org, const Vector *dir, int r, int g, int b, int a, int size )
{
	// Set up parameters
	g_TEBloodSprite.m_vecOrigin		= *org;
	g_TEBloodSprite.m_vecDirection	= *dir;
	g_TEBloodSprite.r = r;
	g_TEBloodSprite.g = g;
	g_TEBloodSprite.b = b;
	g_TEBloodSprite.a = a;
	g_TEBloodSprite.m_nSize = size;

	// Implicit
	g_TEBloodSprite.m_nSprayModel = g_sModelIndexBloodSpray;
	g_TEBloodSprite.m_nDropModel = g_sModelIndexBloodDrop;

	// Create it
	g_TEBloodSprite.Create( filter, delay );
}