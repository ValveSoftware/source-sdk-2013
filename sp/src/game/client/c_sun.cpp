//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_sun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void RecvProxy_HDRColorScale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Sun *pSun = ( C_Sun * )pStruct;

	pSun->m_Overlay.m_flHDRColorScale = pData->m_Value.m_Float;
	pSun->m_GlowOverlay.m_flHDRColorScale = pData->m_Value.m_Float;
}

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_Sun, DT_Sun, CSun )
	
	RecvPropInt( RECVINFO(m_clrRender), 0, RecvProxy_IntToColor32 ),
	RecvPropInt( RECVINFO(m_clrOverlay), 0, RecvProxy_IntToColor32 ),
	RecvPropVector( RECVINFO( m_vDirection ) ),
	RecvPropInt( RECVINFO( m_bOn ) ),
	RecvPropInt( RECVINFO( m_nSize ) ),
	RecvPropInt( RECVINFO( m_nOverlaySize ) ),
	RecvPropInt( RECVINFO( m_nMaterial ) ),
	RecvPropInt( RECVINFO( m_nOverlayMaterial ) ),
	RecvPropFloat("HDRColorScale", 0, SIZEOF_IGNORE, 0, RecvProxy_HDRColorScale),
	
END_RECV_TABLE()

C_Sun::C_Sun()
{
	m_Overlay.m_bDirectional = true;
	m_Overlay.m_bInSky = true;

	m_GlowOverlay.m_bDirectional = true;
	m_GlowOverlay.m_bInSky = true;
}


C_Sun::~C_Sun()
{
}


void C_Sun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// We have to do special setup on our colors because we're tinting an additive material.
	// If we don't have at least one component at full strength, the luminosity of the material
	// will change and that will cause the material to become more translucent  This would be incorrect
	// for the sun, which should always be completely opaque at its core.  Here, we renormalize the
	// components to make sure only hue is altered.

	float maxComponent = MAX ( m_clrRender->r, MAX ( m_clrRender->g, m_clrRender->b ) );

	Vector vOverlayColor;
	Vector vMainColor;

	// Re-normalize the color ranges
	if ( maxComponent <= 0.0f )
	{
		// This is an error, set to pure white
		vMainColor.Init( 1.0f, 1.0f, 1.0f );
	}
	else
	{
		vMainColor.x = m_clrRender->r / maxComponent;
		vMainColor.y = m_clrRender->g / maxComponent;
		vMainColor.z = m_clrRender->b / maxComponent;
	}
	
	// If we're non-zero, use the value (otherwise use the value we calculated above)
	if ( m_clrOverlay.r != 0 || m_clrOverlay.g != 0 || m_clrOverlay.b != 0 )
	{
		// Get our overlay color
		vOverlayColor.x = m_clrOverlay.r / 255.0f;
		vOverlayColor.y = m_clrOverlay.g / 255.0f;
		vOverlayColor.z = m_clrOverlay.b / 255.0f;
	}
	else
	{
		vOverlayColor = vMainColor;
	}

	// 
	// Setup the core overlay
	//

	m_Overlay.m_vDirection = m_vDirection;
	m_Overlay.m_nSprites = 1;

	m_Overlay.m_Sprites[0].m_vColor = vMainColor;
	m_Overlay.m_Sprites[0].m_flHorzSize = m_nSize;
	m_Overlay.m_Sprites[0].m_flVertSize = m_nSize;

	const model_t* pModel = (m_nMaterial != 0) ? modelinfo->GetModel( m_nMaterial ) : NULL;
	const char *pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";
	m_Overlay.m_Sprites[0].m_pMaterial = materials->FindMaterial( pModelName, TEXTURE_GROUP_OTHER );
	m_Overlay.m_flProxyRadius = 0.05f; // about 1/20th of the screen

	//
	// Setup the external glow overlay
	//

	m_GlowOverlay.m_vDirection = m_vDirection;
	m_GlowOverlay.m_nSprites = 1;

	m_GlowOverlay.m_Sprites[0].m_vColor = vOverlayColor;
	m_GlowOverlay.m_Sprites[0].m_flHorzSize = m_nOverlaySize;
	m_GlowOverlay.m_Sprites[0].m_flVertSize = m_nOverlaySize;

	pModel = (m_nOverlayMaterial != 0) ? modelinfo->GetModel( m_nOverlayMaterial ) : NULL;
	pModelName = pModel ? modelinfo->GetModelName( pModel ) : "";
	m_GlowOverlay.m_Sprites[0].m_pMaterial = materials->FindMaterial( pModelName, TEXTURE_GROUP_OTHER );

	// This texture will fade away as the dot between camera and sun changes
	m_GlowOverlay.SetModulateByDot();
	m_GlowOverlay.m_flProxyRadius = 0.05f; // about 1/20th of the screen


	// Either activate or deactivate.
	if ( m_bOn )
	{
		m_Overlay.Activate();
		m_GlowOverlay.Activate();
	}
	else
	{
		m_Overlay.Deactivate();
		m_GlowOverlay.Deactivate();
	}
}



