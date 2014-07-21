//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particlesphererenderer.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CParticleSphereRenderer::CParticleSphereRenderer()
{
	m_vBaseColor.Init();
	memset( &m_AmbientLight, 0, sizeof( m_AmbientLight ) );
	memset( &m_DirectionalLight, 0, sizeof( m_DirectionalLight ) );

	m_bUsingPixelShaders = false;
	m_iLastTickStartRenderCalled = -1;
	m_pParticleMgr = NULL;
}


void CParticleSphereRenderer::Init( CParticleMgr *pParticleMgr, IMaterial *pMaterial )
{
	m_pParticleMgr = pParticleMgr;

	// Figure out how we need to draw.
	bool bFound = false;
	IMaterialVar *pVar = pMaterial->FindVar( "$USINGPIXELSHADER", &bFound, false );
	if( bFound && pVar && pVar->GetIntValue() )
		m_bUsingPixelShaders = true;
	else
		m_bUsingPixelShaders = false;
}


void CParticleSphereRenderer::StartRender( VMatrix &effectMatrix )
{
	// We're about to be rendered.. set our directional lighting parameters for this particle system.
	if ( m_pParticleMgr )
	{
		m_pParticleMgr->SetDirectionalLightInfo( m_DirectionalLight );
	}

	m_iLastTickStartRenderCalled = gpGlobals->tickcount;
}
