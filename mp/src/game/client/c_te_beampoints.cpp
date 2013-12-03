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
// Purpose: BeamPoints TE
//-----------------------------------------------------------------------------
class C_TEBeamPoints : public C_TEBaseBeam
{
public:
	DECLARE_CLASS( C_TEBeamPoints, C_TEBaseBeam );
	DECLARE_CLIENTCLASS();

					C_TEBeamPoints( void );
	virtual			~C_TEBeamPoints( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecStartPoint;
	Vector			m_vecEndPoint;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamPoints::C_TEBeamPoints( void )
{
	m_vecStartPoint.Init();
	m_vecEndPoint.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamPoints::~C_TEBeamPoints( void )
{
}

void TE_BeamPoints( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, 
	int r, int g, int b, int a, int speed )
{
	beams->CreateBeamPoints( (Vector&)*start, (Vector&)*end, modelindex, haloindex, 0.0f, 
		life, width, endWidth, fadeLength, amplitude, a, 0.1 * speed, 
		startframe, 0.1 * (float)framerate, r, g, b );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamPoints::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamPoints( m_vecStartPoint, m_vecEndPoint, m_nModelIndex, m_nHaloIndex, 0.0f, 
		m_fLife, m_fWidth,  m_fEndWidth, m_nFadeLength, m_fAmplitude, a, 0.1 * m_nSpeed, 
		m_nStartFrame, 0.1 * m_nFrameRate, r, g, b );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBeamPoints, DT_TEBeamPoints, CTEBeamPoints)
	RecvPropVector( RECVINFO(m_vecStartPoint)),
	RecvPropVector( RECVINFO(m_vecEndPoint)),
END_RECV_TABLE()

