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

//-----------------------------------------------------------------------------
// Purpose: Dispatches a beam ring between two entities
//-----------------------------------------------------------------------------
class CTEBeamFollow : public CTEBaseBeam
{
	DECLARE_CLASS( CTEBeamFollow, CTEBaseBeam );
public:

	DECLARE_SERVERCLASS();

					CTEBeamFollow( const char *name );
	virtual			~CTEBeamFollow( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

public:

	CNetworkVar( int, m_iEntIndex );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBeamFollow::CTEBeamFollow( const char *name ) :
	CTEBaseBeam( name )
{
	m_iEntIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBeamFollow::~CTEBeamFollow( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBeamFollow::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_iEntIndex	= 1;
}

IMPLEMENT_SERVERCLASS_ST(CTEBeamFollow, DT_TEBeamFollow)
	SendPropInt( SENDINFO(m_iEntIndex), 24, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEBeamEntPoint objects
static CTEBeamFollow g_TEBeamFollow( "BeamFollow" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			delay - 
//			iEntIndex - 
//			modelIndex - 
//			modelindex - 
//			haloIndex - 
//			life - 
//			width - 
//			endWidth - 
//			fadeLength - 
//			r - 
//			g - 
//			b - 
//			a - 
//-----------------------------------------------------------------------------
void TE_BeamFollow( IRecipientFilter& filter, float delay,
	int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
	float fadeLength,float r, float g, float b, float a )
{
	g_TEBeamFollow.m_iEntIndex		= (iEntIndex & 0x0FFF) | ((1 & 0xF)<<12);
	g_TEBeamFollow.m_nModelIndex	= modelIndex;
	g_TEBeamFollow.m_nHaloIndex		= haloIndex;
	g_TEBeamFollow.m_nStartFrame	= 0;
	g_TEBeamFollow.m_nFrameRate		= 0;
	g_TEBeamFollow.m_fLife			= life;
	g_TEBeamFollow.m_fWidth			= width;
	g_TEBeamFollow.m_fEndWidth		= endWidth;
	g_TEBeamFollow.m_nFadeLength	= fadeLength;
	g_TEBeamFollow.r				= r;
	g_TEBeamFollow.g				= g;
	g_TEBeamFollow.b				= b;
	g_TEBeamFollow.a				= a;

	// Send it over the wire
	g_TEBeamFollow.Create( filter, delay );
}