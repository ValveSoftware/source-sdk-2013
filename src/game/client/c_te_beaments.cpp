//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_te_basebeam.h"
#include "iviewrender_beams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: BeamEnts TE
//-----------------------------------------------------------------------------
class C_TEBeamEnts : public C_TEBaseBeam
{
public:
	DECLARE_CLASS( C_TEBeamEnts, C_TEBaseBeam );
	DECLARE_CLIENTCLASS();

					C_TEBeamEnts( void );
	virtual			~C_TEBeamEnts( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nStartEntity;
	int				m_nEndEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamEnts::C_TEBeamEnts( void )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamEnts::~C_TEBeamEnts( void )
{
}

void TE_BeamEnts( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, 
	int r, int g, int b, int a, int speed )
{
	beams->CreateBeamEnts( start, end, modelindex, haloindex, 0.0f, 
		life, width, endWidth, fadeLength, amplitude, a, 0.1 * (float)speed, 
		startframe, 0.1 * (float)framerate, r, g, b );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamEnts::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamEnts( m_nStartEntity, m_nEndEntity, m_nModelIndex, m_nHaloIndex, 0.0f, 
		m_fLife, m_fWidth,  m_fEndWidth, m_nFadeLength, m_fAmplitude, a, 0.1 * m_nSpeed, 
		m_nStartFrame, 0.1 * m_nFrameRate, r, g, b );
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEBeamEnts, DT_TEBeamEnts, CTEBeamEnts );

BEGIN_RECV_TABLE(C_TEBeamEnts, DT_TEBeamEnts)
	RecvPropInt( RECVINFO(m_nStartEntity)),
	RecvPropInt( RECVINFO(m_nEndEntity)),
END_RECV_TABLE()

