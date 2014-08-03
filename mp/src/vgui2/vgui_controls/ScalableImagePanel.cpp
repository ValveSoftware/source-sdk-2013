//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>

#include <vgui/IBorder.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/IBorder.h>
#include <KeyValues.h>

#include <vgui_controls/ScalableImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( ScalableImagePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ScalableImagePanel::ScalableImagePanel(Panel *parent, const char *name) : Panel(parent, name)
{
	m_iSrcCornerHeight = 0;
	m_iSrcCornerWidth = 0;

	m_iCornerHeight = 0;
	m_iCornerWidth = 0;

	m_pszImageName = NULL;
	m_pszDrawColorName = NULL;

	m_DrawColor = Color(255,255,255,255);

	m_flCornerWidthPercent = 0;
	m_flCornerHeightPercent = 0;

	m_iTextureID = surface()->CreateNewTextureID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ScalableImagePanel::~ScalableImagePanel()
{
	delete [] m_pszImageName;
	delete [] m_pszDrawColorName;

	if ( vgui::surface() && m_iTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iTextureID );
		m_iTextureID = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScalableImagePanel::SetImage(const char *imageName)
{
	if ( *imageName )
	{
		char szImage[MAX_PATH];

		const char *pszDir = "vgui/";
		int len = Q_strlen(imageName) + 1;
		len += strlen(pszDir);
		Q_snprintf( szImage, len, "%s%s", pszDir, imageName );

		if ( m_pszImageName && V_stricmp( szImage, m_pszImageName ) == 0 )
			return;

		delete [] m_pszImageName;
		m_pszImageName = new char[ len ];
		Q_strncpy(m_pszImageName, szImage, len );
	}
	else
	{
		delete [] m_pszImageName;
		m_pszImageName = NULL;
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScalableImagePanel::PaintBackground()
{
	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetColor( m_DrawColor.r(), m_DrawColor.g(), m_DrawColor.b(), GetAlpha() );
	surface()->DrawSetTexture( m_iTextureID );

	int x = 0;
	int y = 0;

	float uvx = 0;
	float uvy = 0;
	float uvw = 0, uvh = 0;

	float drawW, drawH;

	int row, col;
	for ( row=0;row<3;row++ )
	{
		x = 0;
		uvx = 0;

		if ( row == 0 || row == 2 )
		{
			//uvh - row 0 or 2, is src_corner_height
			uvh = m_flCornerHeightPercent;
			drawH = m_iCornerHeight;
		}
		else
		{
			//uvh - row 1, is tall - ( 2 * src_corner_height ) ( min 0 )
			uvh = max( 1.0 - 2 * m_flCornerHeightPercent, 0.0f );
			drawH = max( 0, ( tall - 2 * m_iCornerHeight ) );
		}

		for ( col=0;col<3;col++ )
		{
			if ( col == 0 || col == 2 )
			{
				//uvw - col 0 or 2, is src_corner_width
				uvw = m_flCornerWidthPercent;
				drawW = m_iCornerWidth;
			}
			else
			{
				//uvw - col 1, is wide - ( 2 * src_corner_width ) ( min 0 )
				uvw = max( 1.0 - 2 * m_flCornerWidthPercent, 0.0f );
				drawW = max( 0, ( wide - 2 * m_iCornerWidth ) );
			}

			Vector2D uv11( uvx, uvy );
			Vector2D uv21( uvx+uvw, uvy );
			Vector2D uv22( uvx+uvw, uvy+uvh );
			Vector2D uv12( uvx, uvy+uvh );

			vgui::Vertex_t verts[4];
			verts[0].Init( Vector2D( x, y ), uv11 );
			verts[1].Init( Vector2D( x+drawW, y ), uv21 );
			verts[2].Init( Vector2D( x+drawW, y+drawH ), uv22 );
			verts[3].Init( Vector2D( x, y+drawH ), uv12  );

			vgui::surface()->DrawTexturedPolygon( 4, verts );	

			x += drawW;
			uvx += uvw;
		}

		y += drawH;
		uvy += uvh;
	}

	vgui::surface()->DrawSetTexture(0);
}

//-----------------------------------------------------------------------------
// Purpose: Gets control settings for editing
//-----------------------------------------------------------------------------
void ScalableImagePanel::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);

	if (m_pszDrawColorName)
	{
		outResourceData->SetString("drawcolor", m_pszDrawColorName);
	}

	outResourceData->SetInt("src_corner_height", m_iSrcCornerHeight);
	outResourceData->SetInt("src_corner_width", m_iSrcCornerWidth);

	outResourceData->SetInt("draw_corner_height", m_iCornerHeight);
	outResourceData->SetInt("draw_corner_width", m_iCornerWidth);

	if (m_pszImageName)
	{
		outResourceData->SetString("image", m_pszImageName);
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Applies designer settings from res file
//-----------------------------------------------------------------------------
void ScalableImagePanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	delete [] m_pszDrawColorName;
	m_pszDrawColorName = NULL;

	const char *pszDrawColor = inResourceData->GetString("drawcolor", "");
	if (*pszDrawColor)
	{
		int r = 0, g = 0, b = 0, a = 255;
		int len = Q_strlen(pszDrawColor) + 1;
		m_pszDrawColorName = new char[ len ];
		Q_strncpy( m_pszDrawColorName, pszDrawColor, len );

		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			// it's a direct color
			m_DrawColor = Color(r, g, b, a);
		}
		else
		{
			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
			m_DrawColor = pScheme->GetColor(pszDrawColor, Color(0, 0, 0, 0));
		}
	}

	m_iSrcCornerHeight = inResourceData->GetInt( "src_corner_height" );
	m_iSrcCornerWidth = inResourceData->GetInt( "src_corner_width" );

	m_iCornerHeight = inResourceData->GetInt( "draw_corner_height" );
	m_iCornerWidth = inResourceData->GetInt( "draw_corner_width" );

	if ( IsProportional() )
	{
		// scale the x and y up to our screen co-ords
		m_iCornerHeight = scheme()->GetProportionalScaledValueEx(GetScheme(), m_iCornerHeight);
		m_iCornerWidth = scheme()->GetProportionalScaledValueEx(GetScheme(), m_iCornerWidth);
	}

	const char *imageName = inResourceData->GetString("image", "");
	SetImage( imageName );

	InvalidateLayout();
}

void ScalableImagePanel::PerformLayout( void )
{
	if ( m_pszImageName )
	{
		surface()->DrawSetTextureFile( m_iTextureID, m_pszImageName, true, false);
	}

	// get image dimensions, compare to m_iSrcCornerHeight, m_iSrcCornerWidth
	int wide,tall;
	surface()->DrawGetTextureSize( m_iTextureID, wide, tall );

	m_flCornerWidthPercent = ( wide > 0 ) ? ( (float)m_iSrcCornerWidth / (float)wide ) : 0;
	m_flCornerHeightPercent = ( tall > 0 ) ? ( (float)m_iSrcCornerHeight / (float)tall ) : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Describes editing details
//-----------------------------------------------------------------------------
const char *ScalableImagePanel::GetDescription()
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s string image, int src_corner_height, int src_corner_width, int draw_corner_height, int draw_corner_width", BaseClass::GetDescription());
	return buf;
}