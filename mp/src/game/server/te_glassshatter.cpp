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
#include "shattersurfacetypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches Glass Shatter tempentity
//-----------------------------------------------------------------------------
class CTEShatterSurface : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEShatterSurface, CBaseTempEntity );

	CTEShatterSurface( const char *name );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_vecAngles );
	CNetworkVector( m_vecForce );
	CNetworkVector( m_vecForcePos );
	CNetworkVar( float, m_flWidth );
	CNetworkVar( float, m_flHeight );
	CNetworkVar( float, m_flShardSize );
	CNetworkVar( int, m_nSurfaceType );
	CNetworkArray( byte, m_uchFrontColor, 3 );
	CNetworkArray( byte, m_uchBackColor, 3 );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEShatterSurface::CTEShatterSurface( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecAngles.Init();
	m_vecForce.Init();
	m_vecForcePos.Init();
	m_flWidth			= 16.0f;
	m_flHeight			= 16.0f;
	m_flShardSize		= 3.0f;
	m_uchFrontColor.Set( 0, 255 );
	m_uchFrontColor.Set( 1, 255 );
	m_uchFrontColor.Set( 2, 255 );
	m_uchBackColor.Set( 0, 255 );
	m_uchBackColor.Set( 1, 255 );
	m_uchBackColor.Set( 2, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEShatterSurface::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_vecOrigin = current_origin;

	Vector vecEnd;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 1024.0, forward, vecEnd );

	trace_t tr;

	UTIL_TraceLine( m_vecOrigin, vecEnd, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	m_vecOrigin = tr.endpos;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEShatterSurface, DT_TEShatterSurface)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecAngles), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecForce), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecForcePos), -1, SPROP_COORD),
	SendPropFloat( SENDINFO(m_flWidth), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO(m_flHeight), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO(m_flShardSize), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_nSurfaceType), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchFrontColor, 0 ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchFrontColor, 1 ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchFrontColor, 2 ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchBackColor, 0 ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchBackColor, 1 ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_ARRAYELEM( m_uchBackColor, 2 ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEShatterSurface objects
static CTEShatterSurface g_TEShatterSurface( "Surface Shatter" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			entity - 
//			index - 
//-----------------------------------------------------------------------------
void TE_ShatterSurface( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle* angle, const Vector* force, const Vector* forcepos, 
	float width, float height, float shardsize, ShatterSurface_t surfacetype,
	int front_r, int front_g, int front_b, int back_r, int back_g, int back_b)
{
	g_TEShatterSurface.m_vecOrigin			= *pos;
	g_TEShatterSurface.m_vecAngles			= *angle;
	g_TEShatterSurface.m_vecForce			= *force;
	g_TEShatterSurface.m_vecForcePos		= *forcepos;
	g_TEShatterSurface.m_flWidth			= width;
	g_TEShatterSurface.m_flHeight			= height;
	g_TEShatterSurface.m_flShardSize		= shardsize;
	g_TEShatterSurface.m_nSurfaceType		= surfacetype;
	g_TEShatterSurface.m_uchFrontColor.Set( 0, front_r );
	g_TEShatterSurface.m_uchFrontColor.Set( 1, front_g );
	g_TEShatterSurface.m_uchFrontColor.Set( 2, front_b );
	g_TEShatterSurface.m_uchBackColor.Set( 0, back_r );
	g_TEShatterSurface.m_uchBackColor.Set( 1, back_g );
	g_TEShatterSurface.m_uchBackColor.Set( 2, back_b );

	// Send it over the wire
	g_TEShatterSurface.Create( filter, delay );
}
