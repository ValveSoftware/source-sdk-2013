//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "matsys_controls/proceduraltexturepanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tier1/KeyValues.h"
#include "pixelwriter.h"


using namespace vgui;


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CProceduralTexturePanel::CProceduralTexturePanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_pImageBuffer = NULL;
	m_bMaintainProportions = false;
	m_bUsePaintRect = false;
	m_PaintRect.x = m_PaintRect.y = 0;
	m_PaintRect.width = m_PaintRect.height = 0;
}

CProceduralTexturePanel::~CProceduralTexturePanel()
{
	CleanUp();
}

	
//-----------------------------------------------------------------------------
// initialization, shutdown
//-----------------------------------------------------------------------------
bool CProceduralTexturePanel::Init( int nWidth, int nHeight, bool bAllocateImageBuffer )
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	if ( bAllocateImageBuffer )
	{
		m_pImageBuffer = new BGRA8888_t[nWidth * nHeight];
	}
	
	m_TextureSubRect.x = m_TextureSubRect.y = 0;
	m_TextureSubRect.width = nWidth;
	m_TextureSubRect.height = nHeight;

	char pTemp[512];
	Q_snprintf( pTemp, 512, "__%s", GetName() );

	ITexture *pTex = MaterialSystem()->CreateProceduralTexture( pTemp, TEXTURE_GROUP_VGUI,
			m_nWidth, m_nHeight, IMAGE_FORMAT_BGRX8888, 
			TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP | 
			TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_PROCEDURAL | TEXTUREFLAGS_SINGLECOPY );
	pTex->SetTextureRegenerator( this );
	m_ProceduralTexture.Init( pTex );

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", pTemp );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	pVMTKeyValues->SetInt( "$nodebug", 1 );
	m_ProceduralMaterial.Init( MaterialSystem()->CreateMaterial( pTemp, pVMTKeyValues ));

	m_nTextureID = MatSystemSurface()->CreateNewTextureID( false );
	MatSystemSurface()->DrawSetTextureMaterial( m_nTextureID, m_ProceduralMaterial );
	return true;
}

void CProceduralTexturePanel::Shutdown()
{
	CleanUp();
}


//-----------------------------------------------------------------------------
// Maintain proportions when drawing
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::MaintainProportions( bool bEnable )
{
	m_bMaintainProportions = bEnable;
}

	
//-----------------------------------------------------------------------------
// Returns the image buffer + dimensions
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::CleanUp()
{
	if ( (ITexture*)m_ProceduralTexture )
	{
		m_ProceduralTexture->SetTextureRegenerator( NULL );
	}
	m_ProceduralTexture.Shutdown();
	m_ProceduralMaterial.Shutdown();

	if ( m_pImageBuffer )
	{
		delete[] m_pImageBuffer;
		m_pImageBuffer = NULL;
	}
}


//-----------------------------------------------------------------------------
// Default implementation of regenerate texture bits 
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
{
	Assert( m_pImageBuffer );
	Assert( pVTFTexture->FrameCount() == 1 );
	Assert( pVTFTexture->FaceCount() == 1 );
	Assert( !pTexture->IsMipmapped() );

	int nWidth, nHeight, nDepth;
	pVTFTexture->ComputeMipLevelDimensions( 0, &nWidth, &nHeight, &nDepth );
	Assert( nDepth == 1 );
	Assert( nWidth == m_nWidth && nHeight == m_nHeight );

	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
		pVTFTexture->ImageData( 0, 0, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );

	for ( int y = 0; y < nHeight; ++y )
	{
		pixelWriter.Seek( 0, y );
		BGRA8888_t *pTexel = &m_pImageBuffer[y * m_nWidth];
		for ( int x = 0; x < nWidth; ++x, ++pTexel )
		{
			pixelWriter.WritePixel( pTexel->r, pTexel->g, pTexel->b, pTexel->a );
		}
	}
}


//-----------------------------------------------------------------------------
// Returns the image buffer + dimensions
//-----------------------------------------------------------------------------
BGRA8888_t *CProceduralTexturePanel::GetImageBuffer()
{
	Assert( m_pImageBuffer );
	return m_pImageBuffer;
}

int CProceduralTexturePanel::GetImageWidth() const
{
	return m_nWidth;
}

int CProceduralTexturePanel::GetImageHeight() const
{
	return m_nHeight;
}


//-----------------------------------------------------------------------------
// Sets the paint rect
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::SetPaintRect( const Rect_t *pPaintRect )
{
	m_bUsePaintRect = ( pPaintRect != NULL );
	if ( m_bUsePaintRect )
	{
		m_PaintRect = *pPaintRect;
	}
}

	
//-----------------------------------------------------------------------------
// Sets the draw rect
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::SetTextureSubRect( const Rect_t &subRect )
{
	m_TextureSubRect = subRect;
}


//-----------------------------------------------------------------------------
// Redownloads the procedural texture
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::DownloadTexture()
{
	m_ProceduralTexture->Download();
}


//-----------------------------------------------------------------------------
// Paints the texture
//-----------------------------------------------------------------------------
void CProceduralTexturePanel::Paint( void )
{
	vgui::surface()->DrawSetTexture( m_nTextureID );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	int x = 0;
	int y = 0;
	int w, h;
	GetSize( w, h );
	if ( m_bUsePaintRect )
	{
		x = m_PaintRect.x;
		y = m_PaintRect.y;
		w = m_PaintRect.width;
		h = m_PaintRect.height;
	}

	if ( m_bMaintainProportions )
	{
		if ( m_TextureSubRect.width > m_TextureSubRect.height )
		{
			h = w * m_TextureSubRect.height / m_TextureSubRect.width;
		}
		else
		{
			w = h * m_TextureSubRect.width / m_TextureSubRect.height;
		}
	}

	// Rotated version of the bitmap!
	// Rotate about the center of the bitmap
	vgui::Vertex_t verts[4];
	verts[0].m_Position.Init( x, y );
	verts[0].m_TexCoord.Init( (float)m_TextureSubRect.x / m_nWidth, (float)m_TextureSubRect.y / m_nHeight );

	verts[1].m_Position.Init( w+x, y );
	verts[1].m_TexCoord.Init( (float)(m_TextureSubRect.x + m_TextureSubRect.width) / m_nWidth, (float)m_TextureSubRect.y / m_nHeight );

	verts[2].m_Position.Init( w+x, h+y );
	verts[2].m_TexCoord.Init( (float)(m_TextureSubRect.x + m_TextureSubRect.width) / m_nWidth, (float)(m_TextureSubRect.y + m_TextureSubRect.height) / m_nHeight );

	verts[3].m_Position.Init( x, h+y );
	verts[3].m_TexCoord.Init( (float)m_TextureSubRect.x / m_nWidth, (float)(m_TextureSubRect.y + m_TextureSubRect.height) / m_nHeight );

	vgui::surface()->DrawTexturedPolygon( 4, verts );
}
