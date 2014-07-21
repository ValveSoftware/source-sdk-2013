//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Beam that's used for the sniper's laser sight
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tempentity.h"
#include "c_te_basebeam.h"
#include "iviewrender_beams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Beam used for Laser sights. Fades out when it's perpendicular to the viewpoint.
//-----------------------------------------------------------------------------
class C_TEBeamLaser : public C_TEBaseBeam
{
	DECLARE_CLASS( C_TEBeamLaser, C_TEBaseBeam );
public:
	DECLARE_CLIENTCLASS();

	C_TEBeamLaser( void );
	virtual			~C_TEBeamLaser( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nStartEntity;
	int				m_nEndEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamLaser::C_TEBeamLaser( void )
{
	m_nStartEntity	= 0;
	m_nEndEntity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamLaser::~C_TEBeamLaser( void )
{
}

void TE_BeamLaser( IRecipientFilter& filter, float delay,
	int	start, int end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed )
{
	beams->CreateBeamEnts( start, end, modelindex, haloindex, 0.0f, 
		life, width, endWidth, fadeLength, amplitude, a, 0.1 * speed, 
		startframe, 0.1 * framerate, r, g, b, TE_BEAMLASER );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamLaser::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamEnts( m_nStartEntity, m_nEndEntity, m_nModelIndex, m_nHaloIndex, 0.0f, 
		m_fLife, m_fWidth,  m_fEndWidth, m_nFadeLength, m_fAmplitude, a, 0.1 * m_nSpeed, 
		m_nStartFrame, 0.1 * m_nFrameRate, r, g, b, TE_BEAMLASER );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBeamLaser, DT_TEBeamLaser, CTEBeamLaser)
	RecvPropInt(RECVINFO(m_nStartEntity)),
	RecvPropInt( RECVINFO(m_nEndEntity)),
END_RECV_TABLE()
