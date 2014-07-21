//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexBubbles;// holds the index for the bubbles model

enum
{
	BUBBLE_TRAIL_COUNT_BITS = 8,
	BUBBLE_TRAIL_MAX_COUNT = ( (1 << BUBBLE_TRAIL_COUNT_BITS) - 1 ),
};


//-----------------------------------------------------------------------------
// Purpose: Dispatches bubble trail
//-----------------------------------------------------------------------------
class CTEBubbleTrail : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEBubbleTrail, CBaseTempEntity );

					CTEBubbleTrail( const char *name );
	virtual			~CTEBubbleTrail( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecMins );
	CNetworkVector( m_vecMaxs );
	CNetworkVar( float, m_flWaterZ );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nCount );
	CNetworkVar( float, m_fSpeed );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBubbleTrail::CTEBubbleTrail( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecMins.Init();
	m_vecMaxs.Init();
	m_flWaterZ = 0.0;
	m_nModelIndex = 0;
	m_nCount = 0;
	m_fSpeed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBubbleTrail::~CTEBubbleTrail( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBubbleTrail::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_vecMins = current_origin;
	
	Vector forward;

	m_vecMins.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecMins, 100.0, forward, m_vecMins.GetForModify() );

	m_vecMaxs = m_vecMins + Vector( 256, 256, 256 );

	m_fSpeed = 8;
	m_nCount = 20;
	m_flWaterZ = 0;

	m_nModelIndex = g_sModelIndexBubbles;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBubbleTrail, DT_TEBubbleTrail)
	SendPropVector( SENDINFO(m_vecMins), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecMaxs), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropFloat( SENDINFO(m_flWaterZ ), 17, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
	SendPropInt( SENDINFO(m_nCount), BUBBLE_TRAIL_COUNT_BITS, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fSpeed ), 17, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
END_SEND_TABLE()


// Singleton to fire TEBubbleTrail objects
static CTEBubbleTrail g_TEBubbleTrail( "Bubble Trail" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*mins - 
//			*maxs - 
//			height - 
//			modelindex - 
//			count - 
//			speed - 
//-----------------------------------------------------------------------------
void TE_BubbleTrail( IRecipientFilter& filter, float delay,
	const Vector* mins, const Vector* maxs, float flWaterZ, int modelindex, int count, float speed )
{
	g_TEBubbleTrail.m_vecMins = *mins;
	g_TEBubbleTrail.m_vecMaxs = *maxs;
	g_TEBubbleTrail.m_flWaterZ = flWaterZ;
	g_TEBubbleTrail.m_nModelIndex = modelindex;
	g_TEBubbleTrail.m_nCount = MIN( count, BUBBLE_TRAIL_MAX_COUNT );
	g_TEBubbleTrail.m_fSpeed = speed;

	// Send it over the wire
	g_TEBubbleTrail.Create( filter, delay );
}