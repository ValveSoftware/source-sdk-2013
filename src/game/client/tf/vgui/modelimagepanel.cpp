//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "modelimagepanel.h"
#include "iconrenderreceiver.h"
#include "materialsystem/imaterialvar.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "renderparm.h"


using namespace vgui;

const char *g_pszModelImagePanelRTName = "_rt_ModelImagePanel";
static vgui::DHANDLE<CModelImagePanel> s_hModelImageLockPanel;


DECLARE_BUILD_FACTORY( CModelImagePanel );

CModelImagePanel::CModelImagePanel( vgui::Panel *pParent, const char *pName )
	: BaseClass( pParent, pName )
{
	m_pCachedIcon = NULL;
	m_pCachedMaterial = NULL;
	m_iCachedTextureID = -1;
}

CModelImagePanel::~CModelImagePanel()
{
	InvalidateImage();
}

void CModelImagePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	InvalidateImage();
}

void CModelImagePanel::OnSizeChanged( int wide, int tall )
{
	BaseClass::OnSizeChanged( wide, tall );
	
	InvalidateImage();
}

void CModelImagePanel::Paint()
{
	// don't do anything for invalid model
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
	{
		return;
	}

	// check lock panel
	if ( s_hModelImageLockPanel )
	{
		// waiting for async copy to finish
		if ( s_hModelImageLockPanel->m_pCachedIcon && s_hModelImageLockPanel->m_pCachedIcon->GetTexture() )
		{
			s_hModelImageLockPanel = NULL;
		}
	}

	if ( m_pCachedIcon )
	{
		if ( m_pCachedIcon->GetTexture() )
		{
			if ( !m_pCachedMaterial && g_pMaterialSystem )
			{
				const char *pszTextureName = m_pCachedIcon->GetTexture()->GetName();
				KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
				pVMTKeyValues->SetString( "$basetexture", pszTextureName );
				pVMTKeyValues->SetInt( "$translucent", 1 );
				pVMTKeyValues->SetInt( "$vertexcolor", 1 );
				IMaterial *pMaterial = g_pMaterialSystem->FindProceduralMaterial( pszTextureName, TEXTURE_GROUP_VGUI, pVMTKeyValues );
				SafeAssign( &m_pCachedMaterial, pMaterial );

				bool bFound = false;
				IMaterialVar *pVar = m_pCachedMaterial->FindVar( "$basetexture", &bFound );
				if ( bFound && pVar )
				{
					pVar->SetTextureValue( m_pCachedIcon->GetTexture() );
					m_pCachedMaterial->RefreshPreservingMaterialVars();
				}
			}

			if ( m_iCachedTextureID == -1 )
			{
				m_iCachedTextureID = g_pMatSystemSurface->DrawGetTextureId( m_pCachedIcon->GetTexture() );
				g_pMatSystemSurface->DrawSetTextureMaterial( m_iCachedTextureID, m_pCachedMaterial );
			}
		}
		else
		{
			// still waiting for texture
			BaseClass::Paint();
			return;
		}
	}

	// just draw the texture if we got one.
	if ( m_iCachedTextureID != -1 )
	{
		surface()->DrawSetTexture( m_iCachedTextureID );
		surface()->DrawSetColor( 255, 255, 255, 255 );
		const int iWidth = GetWide();
		const int iHeight = GetTall();
		const int iMappingWitdh = m_pCachedMaterial->GetMappingWidth();
		const int iMappingHeight = m_pCachedMaterial->GetMappingHeight();
		float flTexW, flTexH;
		if ( iWidth > iMappingWitdh || iHeight > iMappingHeight )
		{
			float flScale = iWidth > iHeight ? (float)iMappingWitdh / iWidth : (float)iMappingHeight / iHeight;
			flTexW = ( flScale * iWidth ) / iMappingWitdh;
			flTexH = ( flScale * iHeight ) / iMappingHeight;
		}
		else
		{
			flTexW = (float)( iWidth - 1 ) / iMappingWitdh;
			flTexH = (float)( iHeight - 1 ) / iMappingHeight;
		}
		surface()->DrawTexturedSubRect( 0, 0, iWidth, iHeight, 0.f, 0.f, flTexW, flTexH );
		return;
	}
	
	// can't find available cache render target, don't do anything
	if ( s_hModelImageLockPanel && s_hModelImageLockPanel != this )
	{
		BaseClass::Paint();
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	// Turn off depth-write to dest alpha so that we get white there instead.  The code that uses
	// the render target needs a mask of where stuff was rendered.
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	g_pMatSystemSurface->Set3DPaintTempRenderTarget( g_pszModelImagePanelRTName );

	BaseClass::Paint();

	// copy the rendered weapon skin from the render target
	Assert( m_pCachedIcon == NULL );
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	char buffer[_MAX_PATH];
	CUtlString strMDLName = V_GetFileName( studioHdr.pszName() );
	V_sprintf_safe( buffer, "proc/icon/mdl_%s_body%d_skin%d_w%d_h%d", strMDLName.StripExtension().Get(), m_RootMDL.m_MDL.m_nBody, m_RootMDL.m_MDL.m_nSkin, GetWide(), GetTall() );
	SafeAssign( &m_pCachedIcon, new CIconRenderReceiver() );

	// If the icon still exists in the material system, don't bother regenerating it.
	if ( materials->IsTextureLoaded( buffer ) 
		)
	{
		ITexture* resTexture = materials->FindTexture( buffer, TEXTURE_GROUP_RUNTIME_COMPOSITE, false, 0 );
		if ( resTexture && resTexture->IsError() == false )
		{
			m_pCachedIcon->OnAsyncCreateComplete( resTexture, NULL );
		}
	}
	else
	{
		// No icon available yet, need to create it.
		ITexture *pRenderTarget = g_pMaterialSystem->FindTexture( g_pszModelImagePanelRTName, TEXTURE_GROUP_RENDER_TARGET );
		if ( pRenderTarget )
		{
			pRenderContext->AsyncCreateTextureFromRenderTarget( pRenderTarget, buffer, IMAGE_FORMAT_RGBA8888, false, TEXTUREFLAGS_IMMEDIATE_CLEANUP, m_pCachedIcon, NULL );
			
			// make this panel lock the render target
			s_hModelImageLockPanel = this;
		}
	}

	g_pMatSystemSurface->Reset3DPaintTempRenderTarget();
}

void CModelImagePanel::SetMDL( MDLHandle_t handle, void *pProxyData /*= NULL*/ )
{
	BaseClass::SetMDL( handle, pProxyData );
	InvalidateImage();
}

void CModelImagePanel::SetMDL( const char *pMDLName, void *pProxyData /*= NULL*/ )
{
	BaseClass::SetMDL( pMDLName, pProxyData );
}

void CModelImagePanel::SetMDLBody( unsigned int nBody )
{
	SetBody( nBody );
	InvalidateImage();
}

void CModelImagePanel::SetMDLSkin( int nSkin )
{
	SetSkin( nSkin );
	InvalidateImage();
}

void CModelImagePanel::InvalidateImage()
{
	SafeRelease( &m_pCachedIcon );
	SafeRelease( &m_pCachedMaterial );
	m_iCachedTextureID = -1;
}
