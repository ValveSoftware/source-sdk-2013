//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "matsys_controls/vmtpanel.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/ScrollBar.h"
#include "matsys_controls/matsyscontrols.h"
#include "vgui/IVGui.h"
#include "vgui_controls/ToolWindow.h"
#include "tier2/renderutils.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Enums
//-----------------------------------------------------------------------------
enum 
{
	SCROLLBAR_SIZE=18,  // the width of a scrollbar
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

#define SPHERE_RADIUS 10.0f


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CVMTPanel::CVMTPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_bUseActualSize = true;
	m_pMaterial = NULL;

	m_pHorizontalBar = new ScrollBar( this, "HorizScrollBar", false );
	m_pHorizontalBar->AddActionSignalTarget(this);
	m_pHorizontalBar->SetVisible(false);

	m_pVerticalBar = new ScrollBar( this, "VertScrollBar", true );
	m_pVerticalBar->AddActionSignalTarget(this);
	m_pVerticalBar->SetVisible(false);

	LookAt( SPHERE_RADIUS );

	m_pLightmapTexture.Init( "//platform/materials/debug/defaultlightmap", "editor" );
	m_DefaultEnvCubemap.Init( "editor/cubemap", "editor", true );
}

CVMTPanel::~CVMTPanel()
{
	m_pLightmapTexture.Shutdown();
	m_DefaultEnvCubemap.Shutdown();
	if (m_pMaterial)
	{
		m_pMaterial->DecrementReferenceCount();
	}
}


//-----------------------------------------------------------------------------
// Scheme
//-----------------------------------------------------------------------------
void CVMTPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBorder( pScheme->GetBorder( "MenuBorder") );
}


//-----------------------------------------------------------------------------
// Set the material to draw
//-----------------------------------------------------------------------------
void CVMTPanel::SetMaterial( IMaterial *pMaterial )
{
	if (pMaterial)
	{
		pMaterial->IncrementReferenceCount();
	}
	if (m_pMaterial)
	{
		m_pMaterial->DecrementReferenceCount();
	}
	m_pMaterial = pMaterial;
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Set rendering mode (stretch to full screen, or use actual size)
//-----------------------------------------------------------------------------
void CVMTPanel::RenderUsingActualSize( bool bEnable )
{
	m_bUseActualSize = bEnable;
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void CVMTPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	return;

	// Get the current size, see if it's big enough to view the entire thing
	int iWidth, iHeight;
	GetSize( iWidth, iHeight );

	// In the case of stretching, just stretch to the size and blow off
	// the scrollbars. Same holds true if there's no material
	if (!m_bUseActualSize || !m_pMaterial)
	{
		m_iViewableWidth = iWidth;
		m_iViewableHeight = iHeight;
		m_pHorizontalBar->SetVisible(false);
		m_pVerticalBar->SetVisible(false);
		return;
	}

	// Check the size of the material...
	int iMaterialWidth = m_pMaterial->GetMappingWidth();
	int iMaterialHeight = m_pMaterial->GetMappingHeight();

	// Check if the scroll bars are visible
	bool bHorizScrollVisible = (iMaterialWidth > iWidth);
	bool bVertScrollVisible = (iMaterialHeight > iHeight);

	m_pHorizontalBar->SetVisible(bHorizScrollVisible);
	m_pVerticalBar->SetVisible(bVertScrollVisible);

	// Shrink the bars if both are visible
	m_iViewableWidth = bVertScrollVisible ? iWidth - SCROLLBAR_SIZE - WINDOW_BORDER_WIDTH : iWidth; 
	m_iViewableHeight = bHorizScrollVisible ? iHeight - SCROLLBAR_SIZE - WINDOW_BORDER_WIDTH : iHeight; 

	// Set the position of the horizontal bar...
	if (bHorizScrollVisible)
	{
		m_pHorizontalBar->SetPos(0, iHeight - SCROLLBAR_SIZE);
		m_pHorizontalBar->SetSize( m_iViewableWidth, SCROLLBAR_SIZE );

		m_pHorizontalBar->SetRangeWindow( m_iViewableWidth );
		m_pHorizontalBar->SetRange( 0, iMaterialWidth );	

		// FIXME: Change scroll amount based on how much is not visible?
		m_pHorizontalBar->SetButtonPressedScrollValue( 5 );
	}

	// Set the position of the vertical bar...
	if (bVertScrollVisible)
	{
		m_pVerticalBar->SetPos(iWidth - SCROLLBAR_SIZE, 0);
		m_pVerticalBar->SetSize(SCROLLBAR_SIZE, m_iViewableHeight);

		m_pVerticalBar->SetRangeWindow( m_iViewableHeight );
		m_pVerticalBar->SetRange( 0, iMaterialHeight);	
		m_pVerticalBar->SetButtonPressedScrollValue( 5 );
	}
}


//-----------------------------------------------------------------------------
// paint it stretched to the window size
//-----------------------------------------------------------------------------
void CVMTPanel::DrawStretchedToPanel( CMeshBuilder &meshBuilder )
{
	// Draw a polygon the size of the panel
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( 0, 0, 0 );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( 0, m_iViewableHeight, 0 );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( m_iViewableWidth, m_iViewableHeight, 0 );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( m_iViewableWidth, 0, 0 );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.AdvanceVertex();
}


//-----------------------------------------------------------------------------
// paint it actual size
//-----------------------------------------------------------------------------
void CVMTPanel::DrawActualSize( CMeshBuilder &meshBuilder )
{
	// Check the size of the material...
	int iMaterialWidth = m_pMaterial->GetMappingWidth();
	int iMaterialHeight = m_pMaterial->GetMappingHeight();

	Vector2D ul;
	Vector2D lr;
	Vector2D tul;
	Vector2D tlr;

	if (m_iViewableWidth >= iMaterialWidth)
	{
		// Center the material if we've got enough horizontal space
		ul.x = (m_iViewableWidth - iMaterialWidth) * 0.5f;
		lr.x = ul.x + iMaterialWidth;
		tul.x = 0.0f; tlr.x = 1.0f;
	}
	else
	{
		// Use the scrollbars here...
		int val = m_pHorizontalBar->GetValue();
		tul.x = (float)val / (float)iMaterialWidth;
		tlr.x = tul.x + (float)m_iViewableWidth / (float)iMaterialWidth;

		ul.x = 0;
		lr.x = m_iViewableWidth;
	}

	if (m_iViewableHeight >= iMaterialHeight)
	{
		// Center the material if we've got enough vertical space
		ul.y = (m_iViewableHeight - iMaterialHeight) * 0.5f;
		lr.y = ul.y + iMaterialHeight;
		tul.y = 0.0f; tlr.y = 1.0f;
	}
	else
	{
		// Use the scrollbars here...
		int val = m_pVerticalBar->GetValue();

		tul.y = (float)val / (float)iMaterialHeight;
		tlr.y = tul.y + (float)m_iViewableHeight / (float)iMaterialHeight;

		ul.y = 0;
		lr.y = m_iViewableHeight;
	}

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( ul.x, ul.y, 0 );
	meshBuilder.TexCoord2f( 0, tul.x, tul.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( lr.x, ul.y, 0 );
	meshBuilder.TexCoord2f( 0, tlr.x, tul.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( lr.x, lr.y, 0 );
	meshBuilder.TexCoord2f( 0, tlr.x, tlr.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( ul.x, lr.y, 0 );
	meshBuilder.TexCoord2f( 0, tul.x, tlr.y );
	meshBuilder.AdvanceVertex();
}


//-----------------------------------------------------------------------------
// Draw it on a sphere
//-----------------------------------------------------------------------------
void CVMTPanel::RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi )
{
	int nVertices =  nTheta * nPhi;
	int nIndices = 2 * ( nTheta + 1 ) * ( nPhi - 1 );
	
	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	pRenderContext->FogMode( MATERIAL_FOG_NONE );
	pRenderContext->SetNumBoneWeights( 0 );
	pRenderContext->Bind( m_pMaterial );
	pRenderContext->BindLightmapTexture( m_pLightmapTexture );
	pRenderContext->BindLocalCubemap( m_DefaultEnvCubemap );

	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, nVertices, nIndices );

	bool bIsUsingLightmap = m_pMaterial->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_LIGHTMAP );
	bool bIsUsingBumpedLightmap = m_pMaterial->GetPropertyFlag( MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS );

	int nLightmapWidth = m_pLightmapTexture->GetActualWidth();
	float flHalfLuxel = 0.5f / nLightmapWidth;

	//
	// Build the index buffer.
	//
	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )(nTheta - 1);
			float v = i / ( float )(nPhi - 1);
			float theta = ( j != nTheta-1 ) ? 2.0f * M_PI * u : 0.0f;
			float phi = M_PI * v;

			Vector vecPos;
			vecPos.x = flRadius * sin(phi) * cos(theta);
			vecPos.y = flRadius * sin(phi) * sin(theta); 
			vecPos.z = flRadius * cos(phi);
			    
			Vector vecNormal = vecPos;
			VectorNormalize( vecNormal );

			Vector4D vecTangentS;
			Vector vecTangentT;
			vecTangentS.Init( -vecPos.y, vecPos.x, 0.0f, 1.0f );
			if ( VectorNormalize( vecTangentS.AsVector3D() ) == 0.0f )
			{
				vecTangentS.Init( 1.0f, 0.0f, 0.0f, 1.0f );
			}

			CrossProduct( vecNormal, vecTangentS.AsVector3D(), vecTangentT );

			unsigned char red = (int)( u * 255.0f );
			unsigned char green = (int)( v * 255.0f );
			unsigned char blue = (int)( v * 255.0f );
			unsigned char alpha = (int)( v * 255.0f );

			vecPos += vCenter;

			float u1, u2, v1, v2;
			u1 = u2 = u;
			v1 = v2 = v;

			if ( bIsUsingLightmap )
			{
				u1 = RemapVal( u1, 0.0f, 1.0f, flHalfLuxel, 0.25 - flHalfLuxel );

				if ( bIsUsingBumpedLightmap )
				{
					u2 = 0.25f;
					v2 = 0.0f;
				}
			}

			meshBuilder.Position3fv( vecPos.Base() );
			meshBuilder.Normal3fv( vecNormal.Base() );
			meshBuilder.Color4ub( red, green, blue, alpha );
			meshBuilder.TexCoord2f( 0, u, v );
			meshBuilder.TexCoord2f( 1, u1, v1 );
			meshBuilder.TexCoord2f( 2, u2, v2 );
			meshBuilder.TangentS3fv( vecTangentS.Base() );
			meshBuilder.TangentT3fv( vecTangentT.Base() );
			meshBuilder.UserData( vecTangentS.Base() );
			meshBuilder.AdvanceVertex();
		}
	}

	//
	// Emit the triangle strips.
	//
	int idx = 0;
	for ( i = 0; i < nPhi - 1; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			idx = nTheta * i + j;

			meshBuilder.FastIndex( idx );
			meshBuilder.FastIndex( idx + nTheta );
		}

		//
		// Emit a degenerate triangle to skip to the next row without
		// a connecting triangle.
		//
		if ( i < nPhi - 2 )
		{
			meshBuilder.FastIndex( idx + 1 );
			meshBuilder.FastIndex( idx + 1 + nTheta );
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Power of two FB texture
//-----------------------------------------------------------------------------
static CTextureReference s_pPowerOfTwoFrameBufferTexture;

static ITexture *GetPowerOfTwoFrameBufferTexture( void )
{
	if( !s_pPowerOfTwoFrameBufferTexture )
	{
		s_pPowerOfTwoFrameBufferTexture.Init( materials->FindTexture( "_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET ) );
	}
	
	return s_pPowerOfTwoFrameBufferTexture;
}


//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
void CVMTPanel::OnPaint3D()
{
	if (!m_pMaterial)
		return;

	// Deal with refraction
	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	if ( m_pMaterial->NeedsPowerOfTwoFrameBufferTexture() )
	{
		ITexture *pTexture = GetPowerOfTwoFrameBufferTexture();
		if ( pTexture && !pTexture->IsError() )
		{
			pRenderContext->CopyRenderTargetToTexture( pTexture );
			pRenderContext->SetFrameBufferCopyTexture( pTexture );
		}
	}

	// Draw a background (translucent objects will appear that way)

	// FIXME: Draw the outline of this panel?

	pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	RenderSphere( vec3_origin, SPHERE_RADIUS, 20, 20 );
	/*
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->LoadIdentity();
	pRenderContext->Ortho( 0, 0, m_iViewableWidth, m_iViewableHeight, 0, 1 );

	pRenderContext->Bind( m_pMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	if (!m_bUseActualSize)
	{
		DrawStretchedToPanel( meshBuilder );
	}
	else
	{
		DrawActualSize( meshBuilder );
	}

	meshBuilder.End();
	pMesh->Draw();
	*/
}
	    