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
#include "c_te_basebeam.h"
#include "iviewrender_beams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: BeamRingPoint TE
//-----------------------------------------------------------------------------
class C_TEBeamRingPoint : public C_TEBaseBeam
{
public:
	DECLARE_CLASS( C_TEBeamRingPoint, C_TEBaseBeam );
	DECLARE_CLIENTCLASS();

					C_TEBeamRingPoint( void );
	virtual			~C_TEBeamRingPoint( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecCenter;
	float			m_flStartRadius;
	float			m_flEndRadius;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamRingPoint::C_TEBeamRingPoint( void )
{
	m_vecCenter.Init();
	m_flStartRadius = 0.0f;
	m_flEndRadius = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamRingPoint::~C_TEBeamRingPoint( void )
{
}

void TE_BeamRingPoint( IRecipientFilter& filter, float delay,
	const Vector& center, float start_radius, float end_radius, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags )
{
	beams->CreateBeamRingPoint( center, start_radius, end_radius, modelindex, haloindex, 0.0f,
		life, width, 0.1 * spread, 0.0f, amplitude, a, 0.1 * speed, 
		startframe, 0.1 * framerate, r, g, b, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamRingPoint::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamRingPoint( m_vecCenter, m_flStartRadius, m_flEndRadius, m_nModelIndex, m_nHaloIndex, 0.0f,
		m_fLife, m_fWidth, m_fEndWidth, m_nFadeLength, m_fAmplitude, a, 0.1 * m_nSpeed, 
		m_nStartFrame, 0.1 * m_nFrameRate, r, g, b, m_nFlags );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBeamRingPoint, DT_TEBeamRingPoint, CTEBeamRingPoint)
	RecvPropVector( RECVINFO(m_vecCenter)),
	RecvPropFloat( RECVINFO(m_flStartRadius)),
	RecvPropFloat( RECVINFO(m_flEndRadius)),
END_RECV_TABLE()
