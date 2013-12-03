//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "glow_overlay.h"
#include "view.h"
#include "c_pixel_visibility.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_LightGlowOverlay : public CGlowOverlay
{
public:

	virtual void CalcSpriteColorAndSize( float flDot, CGlowSprite *pSprite, float *flHorzSize, float *flVertSize, Vector *vColor )
	{
		*flHorzSize = pSprite->m_flHorzSize;
		*flVertSize = pSprite->m_flVertSize;
		
		Vector viewDir = ( CurrentViewOrigin() - m_vecOrigin );
		float distToViewer = VectorNormalize( viewDir );

		if ( m_bOneSided )
		{
			if ( DotProduct( viewDir, m_vecDirection ) < 0.0f )
			{
				*vColor = Vector(0,0,0);
				return;
			}
		}

		float fade;

		// See if we're in the outer fade distance range
		if ( m_nOuterMaxDist > m_nMaxDist && distToViewer > m_nMaxDist )
		{
			fade = RemapValClamped( distToViewer, m_nMaxDist, m_nOuterMaxDist, 1.0f, 0.0f );
		}
		else
		{
			fade = RemapValClamped( distToViewer, m_nMinDist, m_nMaxDist, 0.0f, 1.0f );
		}
		
		*vColor = pSprite->m_vColor * fade * m_flGlowObstructionScale;
	}

	void SetOrigin( const Vector &origin ) { m_vecOrigin = origin; }
	
	void SetFadeDistances( int minDist, int maxDist, int outerMaxDist )
	{
		m_nMinDist = minDist;
		m_nMaxDist = maxDist;
		m_nOuterMaxDist = outerMaxDist;
	}

	void SetOneSided( bool state = true ) { m_bOneSided = state; }
	void SetModulateByDot( bool state = true ) { m_bModulateByDot = state; }

	void SetDirection( const Vector &dir ) { m_vecDirection = dir; VectorNormalize( m_vecDirection ); }

protected:

	Vector	m_vecOrigin;
	Vector	m_vecDirection;
	int		m_nMinDist;
	int		m_nMaxDist;
	int		m_nOuterMaxDist;
	bool	m_bOneSided;
	bool	m_bModulateByDot;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_LightGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_LightGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_LightGlow();

// C_BaseEntity overrides.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	Simulate( void );
	virtual void	ClientThink( void );

public:
	
	int					m_nHorizontalSize;
	int					m_nVerticalSize;
	int					m_nMinDist;
	int					m_nMaxDist;
	int					m_nOuterMaxDist;
	int					m_spawnflags;
	C_LightGlowOverlay	m_Glow;

	float				m_flGlowProxySize;
};

static void RecvProxy_HDRColorScale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_LightGlow *pLightGlow = ( C_LightGlow * )pStruct;

	pLightGlow->m_Glow.m_flHDRColorScale = pData->m_Value.m_Float;
}

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_LightGlow, DT_LightGlow, CLightGlow )
	RecvPropInt( RECVINFO(m_clrRender), 0, RecvProxy_IntToColor32 ),
	RecvPropInt( RECVINFO( m_nHorizontalSize ) ),
	RecvPropInt( RECVINFO( m_nVerticalSize ) ),
	RecvPropInt( RECVINFO( m_nMinDist ) ),
	RecvPropInt( RECVINFO( m_nMaxDist ) ),
	RecvPropInt( RECVINFO( m_nOuterMaxDist ) ),
	RecvPropInt( RECVINFO( m_spawnflags ) ),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
	RecvPropFloat(RECVINFO(m_flGlowProxySize)),
	RecvPropFloat("HDRColorScale", 0, SIZEOF_IGNORE, 0, RecvProxy_HDRColorScale),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
C_LightGlow::C_LightGlow() :
m_nHorizontalSize( 0 ), m_nVerticalSize( 0 ), m_nMinDist( 0 ), m_nMaxDist( 0 )
{
	m_Glow.m_bDirectional = false;
	m_Glow.m_bInSky = false;
}

void C_LightGlow::Simulate( void )
{
	BaseClass::Simulate();

	m_Glow.m_vPos = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_LightGlow::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_Glow.m_vPos = GetAbsOrigin();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Setup our flare.
		Vector vColor(
			m_clrRender->r / 255.0f,
			m_clrRender->g / 255.0f,
			m_clrRender->b / 255.0f );

		m_Glow.m_nSprites = 1;

		m_Glow.m_Sprites[0].m_flVertSize = (float) m_nVerticalSize;
		m_Glow.m_Sprites[0].m_flHorzSize = (float) m_nHorizontalSize;
		m_Glow.m_Sprites[0].m_vColor = vColor;
		
		m_Glow.SetOrigin( GetAbsOrigin() );
		m_Glow.SetFadeDistances( m_nMinDist, m_nMaxDist, m_nOuterMaxDist );
		m_Glow.m_flProxyRadius = m_flGlowProxySize;

		if ( m_spawnflags & SF_LIGHTGLOW_DIRECTIONAL )
		{
			m_Glow.SetOneSided();
		}

		SetNextClientThink( gpGlobals->curtime + RandomFloat(0,3.0) );
	}
	else if ( updateType == DATA_UPDATE_DATATABLE_CHANGED ) //Right now only color should change.
	{
		// Setup our flare.
		Vector vColor(
			m_clrRender->r / 255.0f,
			m_clrRender->g / 255.0f,
			m_clrRender->b / 255.0f );

		m_Glow.m_Sprites[0].m_vColor = vColor;
	}
	

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward, NULL, NULL );
	
	m_Glow.SetDirection( forward );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_LightGlow::ClientThink( void )
{
	Vector mins = GetAbsOrigin();
	if ( engine->IsBoxVisible( mins, mins ) )
	{
		m_Glow.Activate();
	}
	else
	{
		m_Glow.Deactivate();
	}

	SetNextClientThink( gpGlobals->curtime + RandomFloat(1.0,3.0) );
}
