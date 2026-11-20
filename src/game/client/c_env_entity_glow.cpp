//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#ifdef GLOWS_ENABLE

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
class C_EnvEntityGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_EnvEntityGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_EnvEntityGlow();
	virtual ~C_EnvEntityGlow();
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

private:
	void CreateGlow();

	CGlowObject *pGlow;
	CNetworkVar( int, m_iMode );
	CNetworkVar( int, m_iStyle );
	CNetworkVar( color32, m_glowColor );
	CNetworkVar( bool, m_bDisabled );
	CNetworkHandle( CBaseEntity, m_hTarget );
};

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_EnvEntityGlow, DT_EnvEntityGlow, CEnvEntityGlow )
	RecvPropInt( RECVINFO( m_iMode ) ),
	RecvPropInt( RECVINFO( m_iStyle ) ),
	RecvPropInt( RECVINFO( m_glowColor ), 0, RecvProxy_IntToColor32 ),
	RecvPropBool( RECVINFO( m_bDisabled ) ),
	RecvPropEHandle( RECVINFO( m_hTarget ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
C_EnvEntityGlow::C_EnvEntityGlow()
{
}

//-----------------------------------------------------------------------------
C_EnvEntityGlow::~C_EnvEntityGlow()
{
	delete pGlow;
}

//-----------------------------------------------------------------------------
void C_EnvEntityGlow::CreateGlow()
{
	if ( pGlow )
	{
		delete pGlow;
		pGlow = nullptr;
	}

	if ( m_bDisabled || !m_hTarget )
	{
		return;
	}

	Vector cvec;
	color32 c = m_glowColor.Get();
	cvec[0] = c.r * (1.0f/255.0f);
	cvec[1] = c.g * (1.0f/255.0f);
	cvec[2] = c.b * (1.0f/255.0f);
	float a = c.a * (1.0f/255.0f);
	
	int iMode = m_iMode.Get();
	bool bDrawWhenOccluded = ( iMode == 0 ) || ( iMode == 1 );
	bool bDrawWhenVisible = ( iMode == 0 ) || ( iMode == 2 );
	Assert( bDrawWhenOccluded || bDrawWhenVisible );

	pGlow = new CGlowObject( m_hTarget, cvec, a, bDrawWhenOccluded, bDrawWhenVisible, (GlowStyle_t)m_iStyle.Get() );
}

//-----------------------------------------------------------------------------
void C_EnvEntityGlow::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );
	
	// this could avoid recreating the glow object on every update, but it
	// wouldn't be noticeably more efficient and it would add a ton of code here.
	CreateGlow();
}

#endif // GLOWS_ENABLE
