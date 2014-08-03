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
// Purpose: Dispatches BSP decal tempentity
//-----------------------------------------------------------------------------
class CTEProjectedDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEProjectedDecal, CBaseTempEntity );

					CTEProjectedDecal( const char *name );
	virtual			~CTEProjectedDecal( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nIndex );
	CNetworkVar( float, m_flDistance );
	CNetworkQAngle( m_angRotation );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEProjectedDecal::CTEProjectedDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_flDistance = 64.0f;
	m_nIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEProjectedDecal::~CTEProjectedDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEProjectedDecal::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_flDistance = 1024.0f;
	m_nIndex = 0;
	m_vecOrigin = current_origin;
	m_angRotation = current_angles;

	Vector vecEnd;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 24.0, forward, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEProjectedDecal, DT_TEProjectedDecal)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropQAngles( SENDINFO(m_angRotation), 10 ),
	SendPropFloat( SENDINFO(m_flDistance), 10, SPROP_ROUNDUP, 0, 1024 ),
	SendPropInt( SENDINFO(m_nIndex), 9, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBSPDecal objects
static CTEProjectedDecal g_TEProjectedDecal( "Projected Decal" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			entity - 
//			index - 
//			modelindex - 
//-----------------------------------------------------------------------------
void TE_ProjectDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle *angles, float distance, int index )
{
	g_TEProjectedDecal.m_vecOrigin		= *pos;
	g_TEProjectedDecal.m_angRotation	= *angles;
	g_TEProjectedDecal.m_flDistance		= distance;	
	g_TEProjectedDecal.m_nIndex			= index;

	// Send it over the wire
	g_TEProjectedDecal.Create( filter, delay );
}