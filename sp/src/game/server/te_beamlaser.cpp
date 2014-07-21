//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Beam used for Laser sights. Fades out when it's perpendicular to the viewpoint.
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
// Purpose: Beam used for Laser sights. Fades out when it's perpendicular to the viewpoint.
//-----------------------------------------------------------------------------
class CTEBeamLaser : public CTEBaseBeam
{
	DECLARE_CLASS( CTEBeamLaser, CTEBaseBeam );
public:
	DECLARE_SERVERCLASS();

					CTEBeamLaser( const char *name );
	virtual			~CTEBeamLaser( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
public:
	CNetworkVar( int, m_nStartEntity );
	CNetworkVar( int, m_nEndEntity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamLaser::CTEBeamLaser( const char *name ) :
	CTEBaseBeam( name )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamLaser::~CTEBeamLaser( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamLaser::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nStartEntity	= 1;
	m_nEndEntity	= 0;

	m_nModelIndex	= g_sModelIndexSmoke;
	m_nStartFrame	= 0;
	m_nFrameRate	= 10;
	m_fLife			= 2.0;
	m_fWidth		= 1.0;
	m_fAmplitude	= 1.0;
	r				= 127;
	g				= 63;
	b				= 0;
	a				= 150;
	m_nSpeed		= 1;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST( CTEBeamLaser, DT_TEBeamLaser)
	SendPropInt( SENDINFO(m_nStartEntity), 24, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nEndEntity), 24, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBeamLaser objects
static CTEBeamLaser g_TEBeamLaser( "BeamLaser" );

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
void TE_BeamLaser( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
{
	g_TEBeamLaser.m_nStartEntity = (start & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamLaser.m_nEndEntity	= (end & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamLaser.m_nModelIndex	= modelindex;
	g_TEBeamLaser.m_nHaloIndex	= haloindex;
	g_TEBeamLaser.m_nStartFrame	= startframe;
	g_TEBeamLaser.m_nFrameRate	= framerate;
	g_TEBeamLaser.m_fLife		= life;
	g_TEBeamLaser.m_fWidth		= width;
	g_TEBeamLaser.m_fEndWidth	= endWidth;
	g_TEBeamLaser.m_nFadeLength	= fadeLength;
	g_TEBeamLaser.m_fAmplitude	= amplitude;
	g_TEBeamLaser.m_nSpeed		= speed;
	g_TEBeamLaser.r				= r;
	g_TEBeamLaser.g				= g;
	g_TEBeamLaser.b				= b;
	g_TEBeamLaser.a				= a;

	// Send it over the wire
	g_TEBeamLaser.Create( filter, delay );
}