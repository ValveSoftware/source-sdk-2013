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
// Purpose: Dispatches Sprite tempentity
//-----------------------------------------------------------------------------
class CTESprite : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTESprite, CBaseTempEntity );

					CTESprite( const char *name );
	virtual			~CTESprite( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

	virtual void	Precache( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( float, m_fScale );
	CNetworkVar( int, m_nBrightness );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTESprite::CTESprite( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_nBrightness = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTESprite::~CTESprite( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTESprite::Precache( void )
{
	CBaseEntity::PrecacheModel("sprites/gunsmoke.vmt");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTESprite::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = CBaseEntity::PrecacheModel("sprites/gunsmoke.vmt");
	m_fScale = 0.8;
	m_nBrightness = 200;
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


IMPLEMENT_SERVERCLASS_ST(CTESprite, DT_TESprite)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropFloat( SENDINFO(m_fScale ), 8, SPROP_ROUNDDOWN, 0.0, 25.6 ),
	SendPropInt( SENDINFO(m_nBrightness), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TESprite objects
static CTESprite g_TESprite( "Sprite" );

void TE_Sprite( IRecipientFilter& filter, float delay,
	const Vector *pos, int modelindex, float size, int brightness )
{
	g_TESprite.m_vecOrigin		= *pos;
	g_TESprite.m_nModelIndex	= modelindex;	
	g_TESprite.m_fScale			= size;
	g_TESprite.m_nBrightness	= brightness;

	// Send it over the wire
	g_TESprite.Create( filter, delay );
}