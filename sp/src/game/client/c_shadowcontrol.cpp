//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shadow control entity.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class C_ShadowControl : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ShadowControl, C_BaseEntity );

	DECLARE_CLIENTCLASS();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

private:
	Vector m_shadowDirection;
	color32 m_shadowColor;
	float m_flShadowMaxDist;
	bool m_bDisableShadows;
#ifdef MAPBASE
	bool m_bEnableLocalLightShadows;
#endif
};

IMPLEMENT_CLIENTCLASS_DT(C_ShadowControl, DT_ShadowControl, CShadowControl)
	RecvPropVector(RECVINFO(m_shadowDirection)),
#ifdef MAPBASE
	/*RecvPropInt(RECVINFO(m_shadowColor), 0, RecvProxy_Int32ToColor32),*/
	RecvPropInt(RECVINFO(m_shadowColor), 0, RecvProxy_IntToColor32),
#else
	RecvPropInt(RECVINFO(m_shadowColor)),
#endif
	RecvPropFloat(RECVINFO(m_flShadowMaxDist)),
	RecvPropBool(RECVINFO(m_bDisableShadows)),
#ifdef MAPBASE
	RecvPropBool(RECVINFO(m_bEnableLocalLightShadows)),
#endif
END_RECV_TABLE()


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_ShadowControl::OnDataChanged(DataUpdateType_t updateType)
{
	// Set the color, direction, distance...
	g_pClientShadowMgr->SetShadowDirection( m_shadowDirection );
	g_pClientShadowMgr->SetShadowColor( m_shadowColor.r, m_shadowColor.g, m_shadowColor.b );
	g_pClientShadowMgr->SetShadowDistance( m_flShadowMaxDist );
	g_pClientShadowMgr->SetShadowsDisabled( m_bDisableShadows );
#ifdef DYNAMIC_RTT_SHADOWS
	g_pClientShadowMgr->SetShadowFromWorldLightsEnabled( m_bEnableLocalLightShadows );
#endif
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ShadowControl::ShouldDraw()
{
	return false;
}

