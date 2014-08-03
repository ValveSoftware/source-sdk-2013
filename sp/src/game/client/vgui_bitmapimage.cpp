//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_bitmapimage.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include "panelmetaclassmgr.h"
#include <KeyValues.h>
#include <vgui/IPanel.h>
#include <bitmap/bitmap.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Check box image
//-----------------------------------------------------------------------------
BitmapImage::BitmapImage()
{
	m_clr.SetColor( 255, 255, 255, 255 );
	m_pos[ 0 ] = m_pos[ 1 ]  = 0;
	m_pPanelSize = NULL;
	m_nTextureId = -1;
	m_bProcedural = false;

	SetViewport( false, 0.0f, 0.0f, 0.0f, 0.0f );
}

BitmapImage::BitmapImage( vgui::VPANEL parent, const char *filename )
{
	m_nTextureId = -1;
	m_clr.SetColor( 255, 255, 255, 255 );
	m_pos[ 0 ] = m_pos[ 1 ]  = 0;
	Init( parent, filename );
	SetViewport( false, 0.0f, 0.0f, 0.0f, 0.0f );
}

BitmapImage::~BitmapImage()
{
	DestroyTexture();
}

void BitmapImage::DestroyTexture()
{
	if ( m_nTextureId != -1 )
	{
		vgui::surface()->DestroyTextureID( m_nTextureId );
		m_nTextureId = -1;
		m_bProcedural = false;
	}
}

bool BitmapImage::Init( vgui::VPANEL pParent, const char *pFileName )
{
	UsePanelRenderSize( pParent );
	if ( pFileName != NULL )
	{
		DestroyTexture();
		m_nTextureId = vgui::surface()->CreateNewTextureID();
		m_bProcedural = false;
		vgui::surface()->DrawSetTextureFile( m_nTextureId, pFileName , true, true);
		GetSize( m_Size[0], m_Size[1] );
	}
	return true;
}

bool BitmapImage::Init( vgui::VPANEL pParent, KeyValues* pInitData )
{
	char const* pMaterialName = pInitData->GetString( "material" );
	if ( !pMaterialName || !pMaterialName[ 0 ] )
		return false;

	// modulation color
	Color color;
	if (!ParseRGBA( pInitData, "color", color ))
		color.SetColor( 255, 255, 255, 255 );

	Init( pParent, pMaterialName );
	SetColor( color );

	return true;
}

//-----------------------------------------------------------------------------
// FIXME: Bleah!!! Don't want two different KeyValues
/*-----------------------------------------------------------------------------
bool BitmapImage::Init( vgui::VPANEL pParent, KeyValues* pInitData )
{
	char const* pMaterialName = pInitData->GetString( "material" );
	if ( !pMaterialName || !pMaterialName[ 0 ] )
		return false;

	// modulation color
	Color color;
	if (!ParseRGBA( pInitData, "color", color ))
		color.SetColor( 255, 255, 255, 255 );

	Init( pParent, pMaterialName );
	SetColor( color );

	return true;
} */

void BitmapImage::SetImageFile( const char *newImage )
{
	if ( m_nTextureId == -1 || m_bProcedural )
	{
		DestroyTexture();
		m_nTextureId = vgui::surface()->CreateNewTextureID();
		m_bProcedural = false;
	}

	vgui::surface()->DrawSetTextureFile( m_nTextureId, newImage , true, true);
}

void BitmapImage::UsePanelRenderSize( vgui::VPANEL pPanel )
{
	m_pPanelSize = pPanel;
}

vgui::VPANEL BitmapImage::GetRenderSizePanel( void )
{
	return m_pPanelSize;
}

void BitmapImage::SetRenderSize( int x, int y )
{
	m_Size[0] = x;
	m_Size[1] = y;
}

void BitmapImage::DoPaint( int x, int y, int wide, int tall, float yaw, float flAlphaModulate )
{
	vgui::surface()->DrawSetTexture( m_nTextureId );

	int r, g, b, a;
	m_clr.GetColor( r, g, b, a );
	a *= flAlphaModulate;
	vgui::surface()->DrawSetColor( r, g, b, a );
	
	if (yaw == 0)
	{
		if ( !m_bUseViewport )
		{
			vgui::surface()->DrawTexturedRect( x, y, x + wide, y + tall );
		}
		else
		{
			vgui::surface()->DrawTexturedSubRect( x, y, x + wide, y + tall,
				m_rgViewport[ 0 ],
				m_rgViewport[ 1 ],
				m_rgViewport[ 2 ],
				m_rgViewport[ 3 ]
				);
		}
	}
	else
	{
		// Rotated version of the bitmap!
		// Rotate about the center of the bitmap
		vgui::Vertex_t verts[4];
		Vector2D center( x + (wide * 0.5f), y + (tall * 0.5f) );

		// Choose a basis...
		float yawRadians = -yaw * M_PI / 180.0f;
		Vector2D axis[2];
		axis[0].x = cos(yawRadians);
		axis[0].y = sin(yawRadians);
		axis[1].x = -axis[0].y;
		axis[1].y = axis[0].x;

		verts[0].m_TexCoord.Init( 0, 0 );
		Vector2DMA( center, -0.5f * wide, axis[0], verts[0].m_Position );
		Vector2DMA( verts[0].m_Position, -0.5f * tall, axis[1], verts[0].m_Position );

		verts[1].m_TexCoord.Init( 1, 0 );
		Vector2DMA( verts[0].m_Position, wide, axis[0], verts[1].m_Position );

		verts[2].m_TexCoord.Init( 1, 1 );
		Vector2DMA( verts[1].m_Position, tall, axis[1], verts[2].m_Position );

		verts[3].m_TexCoord.Init( 0, 1 );
		Vector2DMA( verts[0].m_Position, tall, axis[1], verts[3].m_Position );

		vgui::surface()->DrawTexturedPolygon( 4, verts );
	}
}

void BitmapImage::DoPaint( vgui::VPANEL pPanel, float yaw, float flAlphaModulate )
{
	int wide, tall;
	if ( pPanel )
	{
		vgui::ipanel()->GetSize(pPanel, wide, tall );
	}
	else
	{
		wide = m_Size[0];
		tall = m_Size[1];
	}

	DoPaint( m_pos[0], m_pos[1], wide, tall, yaw, flAlphaModulate );
}

void BitmapImage::Paint()
{
	DoPaint( m_pPanelSize );
}

void BitmapImage::SetColor( const Color& clr )
{
	m_clr = clr;
}

Color BitmapImage::GetColor( )
{
	return m_clr;
}

void BitmapImage::GetColor( int& r,int& g,int& b,int& a )
{
	m_clr.GetColor( r,g,b,a );
}

void BitmapImage::GetSize( int& wide, int& tall )
{
	vgui::surface()->DrawGetTextureSize( m_nTextureId, wide, tall );
}

void BitmapImage::SetPos( int x, int y )
{
	m_pos[ 0 ] = x;
	m_pos[ 1 ] = y;
}


//-----------------------------------------------------------------------------
// Helper method to initialize a bitmap image from KeyValues data..
//-----------------------------------------------------------------------------
bool InitializeImage( KeyValues *pInitData, const char* pSectionName, vgui::Panel *pParent, BitmapImage* pBitmapImage )
{
	KeyValues *pBitmapImageSection;
	if (pSectionName)
	{
		pBitmapImageSection = pInitData->FindKey( pSectionName );
		if ( !pBitmapImageSection )
			return false;
	}
	else
	{
		pBitmapImageSection = pInitData;
	}

	return pBitmapImage->Init( pParent->GetVPanel(), pBitmapImageSection );
}


//-----------------------------------------------------------------------------
// FIXME: How sad. We need to make KeyValues + vgui::KeyValues be the same. Bleah
/*-----------------------------------------------------------------------------
bool InitializeImage( KeyValues *pInitData, const char* pSectionName, vgui::Panel *pParent, BitmapImage* pBitmapImage )
{
	KeyValues *pBitmapImageSection;
	if (pSectionName)
	{
		pBitmapImageSection = pInitData->FindKey( pSectionName );
		if ( !pBitmapImageSection )
			return false;
	}
	else
	{
		pBitmapImageSection = pInitData;
	}

	return pBitmapImage->Init( pParent->GetVPanel(), pBitmapImageSection );
} */



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : use - 
//			left - 
//			top - 
//			right - 
//			bottom - 
//-----------------------------------------------------------------------------
void BitmapImage::SetViewport( bool use, float left, float top, float right, float bottom )
{
	m_bUseViewport = use;
	m_rgViewport[ 0 ] = left;
	m_rgViewport[ 1 ] = top;
	m_rgViewport[ 2 ] = right;
	m_rgViewport[ 3 ] = bottom;
}

//-----------------------------------------------------------------------------
void BitmapImage::SetBitmap( const Bitmap_t &bitmap )
{
	if ( m_nTextureId == -1 || !m_bProcedural )
	{
		DestroyTexture();
		m_nTextureId = vgui::surface()->CreateNewTextureID( true );
		m_bProcedural = true;
	}

	vgui::surface()->DrawSetTextureRGBA( m_nTextureId, bitmap.GetBits(), bitmap.Width(), bitmap.Height(), 1, true );

	// Initialize render size, if we don't already have one
	if ( m_Size[0] == 0 )
	{
		m_Size[0] = bitmap.Width();
		m_Size[1] = bitmap.Height();
	}
}
