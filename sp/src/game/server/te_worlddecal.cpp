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
// Purpose: Dispatches world decal tempentity
//-----------------------------------------------------------------------------
class CTEWorldDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEWorldDecal, CBaseTempEntity );

					CTEWorldDecal( const char *name );
	virtual			~CTEWorldDecal( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nIndex );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEWorldDecal::CTEWorldDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEWorldDecal::~CTEWorldDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEWorldDecal::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nIndex = 0;
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

IMPLEMENT_SERVERCLASS_ST(CTEWorldDecal, DT_TEWorldDecal)
#if defined( TF_DLL )
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_INTEGRAL ),
#else
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
#endif
	SendPropInt( SENDINFO(m_nIndex), 9, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEWorldDecal objects
static CTEWorldDecal g_TEWorldDecal( "World Decal" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			index - 
//-----------------------------------------------------------------------------
void TE_WorldDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int index )
{
	g_TEWorldDecal.m_vecOrigin	= *pos;
	g_TEWorldDecal.m_nIndex		= index;

	// Send it over the wire
	g_TEWorldDecal.Create( filter, delay );
}