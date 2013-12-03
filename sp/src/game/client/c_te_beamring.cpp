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
// Purpose: BeamRing TE
//-----------------------------------------------------------------------------
class C_TEBeamRing : public C_TEBaseBeam
{
public:
	DECLARE_CLASS( C_TEBeamRing, C_TEBaseBeam );
	DECLARE_CLIENTCLASS();

					C_TEBeamRing( void );
	virtual			~C_TEBeamRing( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nStartEntity;
	int				m_nEndEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamRing::C_TEBeamRing( void )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamRing::~C_TEBeamRing( void )
{
}

void TE_BeamRing( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, int spread, float amplitude, int r, int g, int b, int a, int speed, int flags )
{
	beams->CreateBeamRing( start, end, modelindex, haloindex, 0.0f,
		life, width, 0.1 * spread, 0.0f, amplitude, a, 0.1 * speed, 
		startframe, 0.1 * framerate, r, g, b, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamRing::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamRing( m_nStartEntity, m_nEndEntity, m_nModelIndex, m_nHaloIndex, 0.0f,
		m_fLife, m_fWidth, m_fEndWidth, m_nFadeLength, m_fAmplitude, a, 0.1 * m_nSpeed, 
		m_nStartFrame, 0.1 * m_nFrameRate, r, g, b, m_nFlags );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBeamRing, DT_TEBeamRing, CTEBeamRing)
	RecvPropInt( RECVINFO(m_nStartEntity)),
	RecvPropInt( RECVINFO(m_nEndEntity)),
END_RECV_TABLE()
