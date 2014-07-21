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
class CTEBeamPoints : public CTEBaseBeam
{
public:
	DECLARE_CLASS( CTEBeamPoints, CTEBaseBeam );
	DECLARE_SERVERCLASS();

					CTEBeamPoints( const char *name );
	virtual			~CTEBeamPoints( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
public:
	CNetworkVector( m_vecStartPoint );
	CNetworkVector( m_vecEndPoint );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamPoints::CTEBeamPoints( const char *name ) :
	CTEBaseBeam( name )
{
	m_vecStartPoint.Init();
	m_vecEndPoint.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamPoints::~CTEBeamPoints( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamPoints::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nModelIndex	= g_sModelIndexSmoke;
	m_nStartFrame	= 0;
	m_nFrameRate	= 10;
	m_fLife			= 2.0;
	m_fWidth		= 1.0;
	m_fAmplitude	= 1;
	r				= 0;
	g				= 63;
	b				= 127;
	a				= 150;
	m_nSpeed		= 1;
	
	m_vecStartPoint = current_origin;
	
	Vector forward, right;

	m_vecStartPoint += Vector( 0, 0, 30 );

	AngleVectors( current_angles, &forward, &right, 0 );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecStartPoint, 75.0, forward, m_vecStartPoint.GetForModify() );
	VectorMA( m_vecStartPoint, 25.0, right, m_vecEndPoint.GetForModify() );
	VectorMA( m_vecStartPoint, -25.0, right, m_vecStartPoint.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST( CTEBeamPoints, DT_TEBeamPoints)
	SendPropVector( SENDINFO(m_vecStartPoint), -1, SPROP_COORD ),
	SendPropVector( SENDINFO(m_vecEndPoint), -1, SPROP_COORD ),
END_SEND_TABLE()


// Singleton to fire TEBeamPoints objects
static CTEBeamPoints g_TEBeamPoints( "BeamPoints" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*start - 
//			*end - 
//			modelindex - 
//			startframe - 
//			framerate - 
//			msg_dest - 
//			delay - 
//			origin - 
//			recipient - 
//-----------------------------------------------------------------------------
void TE_BeamPoints( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
{
	g_TEBeamPoints.m_vecStartPoint	= *start;
	g_TEBeamPoints.m_vecEndPoint	= *end;
	g_TEBeamPoints.m_nModelIndex	= modelindex;
	g_TEBeamPoints.m_nHaloIndex		= haloindex;
	g_TEBeamPoints.m_nStartFrame	= startframe;
	g_TEBeamPoints.m_nFrameRate		= framerate;
	g_TEBeamPoints.m_fLife			= life;
	g_TEBeamPoints.m_fWidth			= width;
	g_TEBeamPoints.m_fEndWidth		= endWidth;
	g_TEBeamPoints.m_nFadeLength	= fadeLength;
	g_TEBeamPoints.m_fAmplitude		= amplitude;
	g_TEBeamPoints.m_nSpeed			= speed;
	g_TEBeamPoints.r				= r;
	g_TEBeamPoints.g				= g;
	g_TEBeamPoints.b				= b;
	g_TEBeamPoints.a				= a;

	// Send it over the wire
	g_TEBeamPoints.Create( filter, delay );
}