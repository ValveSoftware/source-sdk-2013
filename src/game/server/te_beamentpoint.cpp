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
#include "te_basebeam.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

//-----------------------------------------------------------------------------
// Purpose: Dispatches a beam ring between two entities
//-----------------------------------------------------------------------------
class CTEBeamEntPoint : public CTEBaseBeam
{
public:
	DECLARE_CLASS( CTEBeamEntPoint, CTEBaseBeam );
	DECLARE_SERVERCLASS();

					CTEBeamEntPoint( const char *name );
	virtual			~CTEBeamEntPoint( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	

public:
	CNetworkVar( int, m_nStartEntity );
	CNetworkVector( m_vecStartPoint );
	CNetworkVar( int, m_nEndEntity );
	CNetworkVector( m_vecEndPoint );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamEntPoint::CTEBeamEntPoint( const char *name ) :
	CTEBaseBeam( name )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
	m_vecStartPoint.Init();
	m_vecEndPoint.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamEntPoint::~CTEBeamEntPoint( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamEntPoint::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nStartEntity	= 1;

	m_nModelIndex	= g_sModelIndexSmoke;
	m_nStartFrame	= 0;
	m_nFrameRate	= 10;
	m_fLife			= 2.0;
	m_fWidth		= 1.0;
	m_fAmplitude	= 1.0;
	r				= 0;
	g				= 63;
	b				= 127;
	a				= 150;
	m_nSpeed		= 1;

	m_vecEndPoint = current_origin;
	
	Vector forward, right;

	m_vecEndPoint += Vector( 0, 0, 24 );

	AngleVectors( current_angles, &forward, &right, 0 );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecEndPoint, 50.0, forward, m_vecEndPoint.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBeamEntPoint, DT_TEBeamEntPoint)
	SendPropInt( SENDINFO(m_nStartEntity), 24, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nEndEntity), 24, SPROP_UNSIGNED ),
	SendPropVector( SENDINFO(m_vecStartPoint), -1, SPROP_COORD ),
	SendPropVector( SENDINFO(m_vecEndPoint), -1, SPROP_COORD ),
END_SEND_TABLE()


// Singleton to fire TEBeamEntPoint objects
static CTEBeamEntPoint g_TEBeamEntPoint( "BeamEntPoint" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//				int	start - 
//			*end - 
//			modelindex - 
//			startframe - 
//			framerate - 
//			msg_dest - 
//			delay - 
//			origin - 
//			recipient - 
//-----------------------------------------------------------------------------
void TE_BeamEntPoint( IRecipientFilter& filter, float delay,
	int	nStartEntity, const Vector *start, int nEndEntity, const Vector* end, 
	int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
{
	g_TEBeamEntPoint.m_nStartEntity = (nStartEntity > 0) ? (nStartEntity & 0x0FFF) | ((1 & 0xF)<<12) : 0;
	g_TEBeamEntPoint.m_nEndEntity	= (nEndEntity > 0) ? (nEndEntity & 0x0FFF) | ((1 & 0xF)<<12) : 0;
	g_TEBeamEntPoint.m_vecStartPoint = start ? *start : vec3_origin;
	g_TEBeamEntPoint.m_vecEndPoint	= end ? *end : vec3_origin;
	g_TEBeamEntPoint.m_nModelIndex	= modelindex;
	g_TEBeamEntPoint.m_nHaloIndex	= haloindex;
	g_TEBeamEntPoint.m_nStartFrame	= startframe;
	g_TEBeamEntPoint.m_nFrameRate	= framerate;
	g_TEBeamEntPoint.m_fLife		= life;
	g_TEBeamEntPoint.m_fWidth		= width;
	g_TEBeamEntPoint.m_fEndWidth	= endWidth;
	g_TEBeamEntPoint.m_nFadeLength	= fadeLength;
	g_TEBeamEntPoint.m_fAmplitude	= amplitude;
	g_TEBeamEntPoint.m_nSpeed		= speed;
	g_TEBeamEntPoint.r				= r;
	g_TEBeamEntPoint.g				= g;
	g_TEBeamEntPoint.b				= b;
	g_TEBeamEntPoint.a				= a;

	// Send it over the wire
	g_TEBeamEntPoint.Create( filter, delay );
}