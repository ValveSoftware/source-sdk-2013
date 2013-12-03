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

extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

//-----------------------------------------------------------------------------
// Purpose: Dispatches Sprite tempentity
//-----------------------------------------------------------------------------
class CTEGlowSprite : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEGlowSprite, CBaseTempEntity );

					CTEGlowSprite( const char *name );
	virtual			~CTEGlowSprite( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( float, m_fScale );
	CNetworkVar( float, m_fLife );
	CNetworkVar( int, m_nBrightness );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEGlowSprite::CTEGlowSprite( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_fLife = 0;
	m_nBrightness = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEGlowSprite::~CTEGlowSprite( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEGlowSprite::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = g_sModelIndexSmoke;
	m_fScale = 0.8;
	m_nBrightness = 200;
	m_fLife = 2.0;
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, -25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}


IMPLEMENT_SERVERCLASS_ST(CTEGlowSprite, DT_TEGlowSprite)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropFloat( SENDINFO(m_fScale ), 8, SPROP_ROUNDDOWN, 0.0, 25.6 ),
	SendPropFloat( SENDINFO(m_fLife ), 8, SPROP_ROUNDDOWN, 0.0, 25.6 ),
	SendPropInt( SENDINFO(m_nBrightness), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEGlowSprite objects
static CTEGlowSprite g_TEGlowSprite( "GlowSprite" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			modelindex - 
//			life - 
//			size - 
//			brightness - 
//-----------------------------------------------------------------------------
void TE_GlowSprite( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float life, float size, int brightness )
{
	g_TEGlowSprite.m_vecOrigin		= *pos;
	g_TEGlowSprite.m_nModelIndex	= modelindex;	
	g_TEGlowSprite.m_fLife			= life;
	g_TEGlowSprite.m_fScale			= size;
	g_TEGlowSprite.m_nBrightness	= brightness;

	// Send it over the wire
	g_TEGlowSprite.Create( filter, delay );
}