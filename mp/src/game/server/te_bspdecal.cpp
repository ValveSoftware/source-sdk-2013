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
class CTEBSPDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEBSPDecal, CBaseTempEntity );

					CTEBSPDecal( const char *name );
	virtual			~CTEBSPDecal( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nEntity );
	CNetworkVar( int, m_nIndex );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBSPDecal::CTEBSPDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nEntity = 0;
	m_nIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBSPDecal::~CTEBSPDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBSPDecal::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nEntity = 0;
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


IMPLEMENT_SERVERCLASS_ST(CTEBSPDecal, DT_TEBSPDecal)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropInt( SENDINFO(m_nEntity), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nIndex), 9, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBSPDecal objects
static CTEBSPDecal g_TEBSPDecal( "BSP Decal" );

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
void TE_BSPDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int entity, int index )
{
	g_TEBSPDecal.m_vecOrigin	= *pos;
	g_TEBSPDecal.m_nEntity		= entity;	
	g_TEBSPDecal.m_nIndex		= index;

	// Send it over the wire
	g_TEBSPDecal.Create( filter, delay );
}