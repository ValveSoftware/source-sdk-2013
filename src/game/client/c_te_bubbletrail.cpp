//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Bubble Trail TE
//-----------------------------------------------------------------------------
class C_TEBubbleTrail : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBubbleTrail, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBubbleTrail( void );
	virtual			~C_TEBubbleTrail( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecMins;
	Vector			m_vecMaxs;
	float			m_flWaterZ;
	int				m_nModelIndex;
	int				m_nCount;
	float			m_fSpeed;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBubbleTrail::C_TEBubbleTrail( void )
{
	m_vecMins.Init();
	m_vecMaxs.Init();
	m_flWaterZ = 0.0;
	m_nModelIndex = 0;
	m_nCount = 0;
	m_fSpeed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBubbleTrail::~C_TEBubbleTrail( void )
{
}

void TE_BubbleTrail( IRecipientFilter& filter, float delay,
	const Vector* mins, const Vector* maxs, float flWaterZ, int modelindex, int count, float speed )
{
	tempents->BubbleTrail( *mins, *maxs, flWaterZ, modelindex, count, speed );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBubbleTrail::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEBubbleTrail::PostDataUpdate" );

	tempents->BubbleTrail( m_vecMins, m_vecMaxs, m_flWaterZ, m_nModelIndex, m_nCount, m_fSpeed );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBubbleTrail, DT_TEBubbleTrail, CTEBubbleTrail)
	RecvPropVector( RECVINFO(m_vecMins)),
	RecvPropVector( RECVINFO(m_vecMaxs)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropFloat( RECVINFO(m_flWaterZ )),
	RecvPropInt( RECVINFO(m_nCount)),
	RecvPropFloat( RECVINFO(m_fSpeed )),
END_RECV_TABLE()
