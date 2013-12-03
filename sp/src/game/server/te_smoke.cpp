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
// Purpose: Dispatches smoke tempentity
//-----------------------------------------------------------------------------
class CTESmoke : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTESmoke, CBaseTempEntity );

					CTESmoke( const char *name );
	virtual			~CTESmoke( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( float, m_fScale );
	CNetworkVar( int, m_nFrameRate );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTESmoke::CTESmoke( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nModelIndex = 0;
	m_fScale = 0;
	m_nFrameRate = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTESmoke::~CTESmoke( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTESmoke::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = g_sModelIndexSmoke;
	m_fScale = 5.0;
	m_nFrameRate = 12;
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTESmoke, DT_TESmoke)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropFloat( SENDINFO(m_fScale ), 8, SPROP_ROUNDDOWN, 0.0, 25.6 ),
	SendPropInt( SENDINFO(m_nFrameRate), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TESmoke objects
static CTESmoke g_TESmoke( "Smoke" );

void TE_Smoke( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, float scale, int framerate )
{
	g_TESmoke.m_vecOrigin	= *pos;
	g_TESmoke.m_nModelIndex = modelindex;	
	g_TESmoke.m_fScale		= scale;
	g_TESmoke.m_nFrameRate	= framerate;

	// Send it over the wire
	g_TESmoke.Create( filter, delay );
}