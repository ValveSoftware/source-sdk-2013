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
class CTEBeamRing : public CTEBaseBeam
{
public:
	DECLARE_CLASS( CTEBeamRing, CTEBaseBeam );
	DECLARE_SERVERCLASS();

					CTEBeamRing( const char *name );
	virtual			~CTEBeamRing( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
public:
	CNetworkVar( int, m_nStartEntity );
	CNetworkVar( int, m_nEndEntity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamRing::CTEBeamRing( const char *name ) :
	CTEBaseBeam( name )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamRing::~CTEBeamRing( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamRing::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nStartEntity	= 1;
	m_nEndEntity	= 0;

	m_nModelIndex	= g_sModelIndexSmoke;
	m_nStartFrame	= 0;
	m_nFrameRate	= 2;
	m_fLife			= 10.0;
	m_fWidth		= 2.0;
	m_fAmplitude	= 1;
	r				= 255;
	g				= 255;
	b				= 0;
	a				= 127;
	m_nSpeed		= 5;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}


IMPLEMENT_SERVERCLASS_ST( CTEBeamRing, DT_TEBeamRing)
	SendPropInt( SENDINFO(m_nStartEntity), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nEndEntity), MAX_EDICT_BITS, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBeamRing objects
static CTEBeamRing g_TEBeamRing( "BeamRing" );

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
void TE_BeamRing( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags )
{
	g_TEBeamRing.m_nStartEntity = (start & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamRing.m_nEndEntity	= (end & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamRing.m_nModelIndex	= modelindex;
	g_TEBeamRing.m_nHaloIndex	= haloindex;
	g_TEBeamRing.m_nStartFrame	= startframe;
	g_TEBeamRing.m_nFrameRate	= framerate;
	g_TEBeamRing.m_fLife		= life;
	g_TEBeamRing.m_fWidth		= width;
	g_TEBeamRing.m_fEndWidth	= width;
	g_TEBeamRing.m_nFadeLength	= 0;
	g_TEBeamRing.m_fAmplitude	= amplitude;
	g_TEBeamRing.m_nSpeed		= speed;
	g_TEBeamRing.r				= r;
	g_TEBeamRing.g				= g;
	g_TEBeamRing.b				= b;
	g_TEBeamRing.a				= a;
	g_TEBeamRing.m_nFlags		= flags;

	// Send it over the wire
	g_TEBeamRing.Create( filter, delay );
}