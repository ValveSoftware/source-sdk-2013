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
// Purpose: Dispatches a beam between two entities
//-----------------------------------------------------------------------------
class CTEBeamEnts : public CTEBaseBeam
{
public:
	DECLARE_CLASS( CTEBeamEnts, CTEBaseBeam );
	DECLARE_SERVERCLASS();

					CTEBeamEnts( const char *name );
	virtual			~CTEBeamEnts( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );


public:
	CNetworkVar( int, m_nStartEntity );
	CNetworkVar( int, m_nEndEntity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamEnts::CTEBeamEnts( const char *name ) :
	CTEBaseBeam( name )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamEnts::~CTEBeamEnts( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamEnts::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nStartEntity	= 1;
	m_nEndEntity	= 0;

	m_nModelIndex	= g_sModelIndexSmoke;
	m_nStartFrame	= 0;
	m_nFrameRate	= 10;
	m_fLife			= 2.0;
	m_fWidth		= 1.0;
	m_fAmplitude	= 1;
	r				= 127;
	g				= 63;
	b				= 0;
	a				= 150;
	m_nSpeed		= 1;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBeamEnts, DT_TEBeamEnts)
	SendPropInt( SENDINFO(m_nStartEntity), 24, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nEndEntity), 24, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBeamEnts objects
static CTEBeamEnts g_TEBeamEnts( "BeamEnts" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//				int	start - 
//			end - 
//			modelindex - 
//			startframe - 
//			framerate - 
//			msg_dest - 
//			delay - 
//			origin - 
//			recipient - 
//-----------------------------------------------------------------------------
void TE_BeamEnts( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
{
	g_TEBeamEnts.m_nStartEntity = (start & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamEnts.m_nEndEntity	= (end & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamEnts.m_nModelIndex	= modelindex;
	g_TEBeamEnts.m_nHaloIndex	= haloindex;
	g_TEBeamEnts.m_nStartFrame	= startframe;
	g_TEBeamEnts.m_nFrameRate	= framerate;
	g_TEBeamEnts.m_fLife		= life;
	g_TEBeamEnts.m_fWidth		= width;
	g_TEBeamEnts.m_fEndWidth	= endWidth;
	g_TEBeamEnts.m_nFadeLength	= fadeLength;
	g_TEBeamEnts.m_fAmplitude	= amplitude;
	g_TEBeamEnts.m_nSpeed		= speed;
	g_TEBeamEnts.r				= r;
	g_TEBeamEnts.g				= g;
	g_TEBeamEnts.b				= b;
	g_TEBeamEnts.a				= a;

	// Send it over the wire
	g_TEBeamEnts.Create( filter, delay );
}