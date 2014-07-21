//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"
#include "sun_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LIGHTGLOW_MAXDIST_BITS		16
#define LIGHTGLOW_MAXDIST_MAX_VALUE	((1 << LIGHTGLOW_MAXDIST_BITS)-1)

#define LIGHTGLOW_OUTERMAXDIST_BITS	16
#define LIGHTGLOW_OUTERMAXDIST_MAX_VALUE	((1 << LIGHTGLOW_OUTERMAXDIST_BITS)-1)

class CLightGlow : public CBaseEntity
{
public:
	DECLARE_CLASS( CLightGlow, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

					CLightGlow();
					
	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual int		UpdateTransmitState( void );

	void InputColor(inputdata_t &data);

public:
	CNetworkVar( int, m_nHorizontalSize );
	CNetworkVar( int, m_nVerticalSize );
	CNetworkVar( int, m_nMinDist );
	CNetworkVar( int, m_nMaxDist );
	CNetworkVar( int, m_nOuterMaxDist );

	CNetworkVar( float, m_flGlowProxySize );
	CNetworkVar( float, m_flHDRColorScale );
};

extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CLightGlow, DT_LightGlow )
	SendPropInt( SENDINFO(m_clrRender), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropInt( SENDINFO(m_nHorizontalSize), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nVerticalSize), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMinDist), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMaxDist), LIGHTGLOW_MAXDIST_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nOuterMaxDist), LIGHTGLOW_OUTERMAXDIST_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_spawnflags), 8, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_COORD ),
	SendPropQAngles	(SENDINFO(m_angRotation), 13, 0, SendProxy_Angles ),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
	SendPropFloat( SENDINFO(m_flGlowProxySize ), 6,	SPROP_ROUNDUP,	0.0f,	64.0f ),
	SendPropFloat( SENDINFO_NAME( m_flHDRColorScale, HDRColorScale ), 0,	SPROP_NOSCALE,	0.0f,	100.0f ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_lightglow, CLightGlow );

BEGIN_DATADESC( CLightGlow )

	DEFINE_KEYFIELD( m_nVerticalSize,		FIELD_INTEGER,	"VerticalGlowSize" ),
	DEFINE_KEYFIELD( m_nHorizontalSize,		FIELD_INTEGER,	"HorizontalGlowSize" ),
	DEFINE_KEYFIELD( m_nMinDist,			FIELD_INTEGER,	"MinDist" ),
	DEFINE_KEYFIELD( m_nMaxDist,			FIELD_INTEGER,	"MaxDist" ),
	DEFINE_KEYFIELD( m_nOuterMaxDist,		FIELD_INTEGER,	"OuterMaxDist" ),
	DEFINE_KEYFIELD( m_flGlowProxySize,		FIELD_FLOAT,	"GlowProxySize" ),
	DEFINE_KEYFIELD( m_flHDRColorScale,		FIELD_FLOAT,	"HDRColorScale" ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "Color",  InputColor ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
CLightGlow::CLightGlow( void )
{
	m_nHorizontalSize = 0.0f;
	m_nVerticalSize = 0.0f;
	m_nMinDist = 0.0f;
	m_nMaxDist = 0.0f;

	m_flGlowProxySize = 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLightGlow::Spawn( void )
{
	BaseClass::Spawn();

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: Always transmit light glows to clients to avoid spikes as we enter
//			or leave PVS. Done because we often have many glows in an area.
//-----------------------------------------------------------------------------
int CLightGlow::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLightGlow::Activate()
{
	BaseClass::Activate();

	if ( m_nMaxDist > LIGHTGLOW_MAXDIST_MAX_VALUE )
	{
		Warning( "env_lightglow maxdist too large (%d should be %d).\n", m_nMaxDist.Get(), LIGHTGLOW_MAXDIST_MAX_VALUE );
		m_nMaxDist = LIGHTGLOW_MAXDIST_MAX_VALUE;
	}

	if ( m_nOuterMaxDist > LIGHTGLOW_OUTERMAXDIST_MAX_VALUE )
	{
		Warning( "env_lightglow outermaxdist too large (%d should be %d).\n", m_nOuterMaxDist.Get(), LIGHTGLOW_OUTERMAXDIST_MAX_VALUE );
		m_nOuterMaxDist = LIGHTGLOW_OUTERMAXDIST_MAX_VALUE;
	}
}

void CLightGlow::InputColor(inputdata_t &inputdata)
{
	m_clrRender = inputdata.value.Color32();
}
