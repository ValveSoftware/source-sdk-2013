//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase off-shoot of tf_glow (created using SDK code only)
//
//===========================================================================//

#include "cbase.h"
#include "glow_outline_effect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class C_PointGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PointGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	~C_PointGlow();

	enum
	{
		GLOW_VIS_ALWAYS,
		GLOW_VIS_NOT_WHEN_VISIBLE,
		GLOW_VIS_ONLY_WHEN_VISIBLE,
	};

	void OnDataChanged( DataUpdateType_t type );

	CGlowObject			*GetGlowObject( void ){ return m_pGlowEffect; }
	void				UpdateGlowEffect( void );
	void				DestroyGlowEffect( void );

	EHANDLE m_hGlowTarget;
	color32 m_GlowColor;

	bool				m_bGlowDisabled;
	CGlowObject			*m_pGlowEffect;
};

IMPLEMENT_CLIENTCLASS_DT( C_PointGlow, DT_PointGlow, CPointGlow )
	RecvPropEHandle( RECVINFO( m_hGlowTarget ) ),
	RecvPropInt( RECVINFO( m_GlowColor ), 0, RecvProxy_IntToColor32 ),
	RecvPropBool( RECVINFO( m_bGlowDisabled ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PointGlow::~C_PointGlow()
{
	DestroyGlowEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointGlow::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateGlowEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointGlow::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect
	if ( !m_bGlowDisabled )
	{
		Vector4D vecColor( m_GlowColor.r, m_GlowColor.g, m_GlowColor.b, m_GlowColor.a );
		for (int i = 0; i < 4; i++)
		{
			if (vecColor[i] == 0.0f)
				continue;

			vecColor[i] /= 255.0f;
		}

		m_pGlowEffect = new CGlowObject( m_hGlowTarget, vecColor.AsVector3D(), vecColor.w, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointGlow::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
