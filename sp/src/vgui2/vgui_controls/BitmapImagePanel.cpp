//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>

#include "vgui_controls/BitmapImagePanel.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/IBorder.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

using namespace vgui;

//-----------------------------------------------------------------------------
/**
 * Simple utility function to allocate memory and duplicate a string
 */
static inline char *CloneString( const char *str )
{
	char *cloneStr = new char [ strlen(str)+1 ];
	strcpy( cloneStr, str );
	return cloneStr;
}

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CBitmapImagePanel, BitmapImagePanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBitmapImagePanel::CBitmapImagePanel( Panel *parent, char const *panelName, 
	char const *filename /*= NULL*/ ) : Panel( parent, panelName )
{
	m_pImage = NULL;

	SetBounds( 0, 0, 100, 100 );

	m_pszImageName = NULL;
	m_pszColorName = NULL;

	m_hardwareFiltered = false;
	m_preserveAspectRatio = false;
	m_contentAlignment = Label::a_center;

	if ( filename && filename[ 0 ] )
	{
		m_pImage = scheme()->GetImage( filename, NULL );
		m_pszImageName = CloneString( filename );
	}

	m_bgColor = Color(255, 255, 255, 255);
}
CBitmapImagePanel::~CBitmapImagePanel()
{
	delete [] m_pszImageName;
	delete [] m_pszColorName;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapImagePanel::ComputeImagePosition(int &x, int &y, int &w, int &h)
{
	if (!m_pImage)
	{
		x = y = w = h = 0;
		return;
	}

	if ( !m_preserveAspectRatio )
	{
		x = y = 0;
		GetSize( w, h );
		return;
	}

	int panelWide, panelTall;
	GetSize( panelWide, panelTall );

	int imageWide, imageTall;
	m_pImage->GetSize( imageWide, imageTall );

	if ( panelWide > 0 && panelTall > 0 && imageWide > 0 && imageTall > 0 )
	{
		float xScale = (float)panelWide / (float)imageWide;
		float yScale = (float)panelTall / (float)imageTall;
		float scale = min( xScale, yScale );

		w = (int) (imageWide * scale);
		h = (int) (imageTall * scale);

		switch (m_contentAlignment)
		{
		case Label::a_northwest:
			x = y = 0;
			break;
		case Label::a_north:
			x = (panelWide - w) / 2;
			y = 0;
			break;
		case Label::a_northeast:
			x = (panelWide - w);
			y = 0;
			break;
		case Label::a_west:
			x = 0;
			y = (panelTall - h) / 2;
			break;
		case Label::a_center:
			x = (panelWide - w) / 2;
			y = (panelTall - h) / 2;
			break;
		case Label::a_east:
			x = (panelWide - w);
			y = (panelTall - h) / 2;
			break;
		case Label::a_southwest:
			x = (panelWide - w);
			y = 0;
			break;
		case Label::a_south:
			x = (panelWide - w);
			y = (panelTall - h) / 2;
			break;
		case Label::a_southeast:
			x = (panelWide - w);
			y = (panelTall - h);
			break;
		default:
			x = y = 0;
			break;
		}
	}
	else
	{
		x = y = 0;
		w = panelWide;
		h = panelTall;
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapImagePanel::PaintBorder()
{
	int x, y, w, h;
	ComputeImagePosition(x, y, w, h);

	IBorder *pBorder = GetBorder();
	if ( pBorder )
		pBorder->Paint( x, y, x+w, y+h, -1, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapImagePanel::PaintBackground()
{
	if (!m_pImage)
		return;

	int x, y, w, h;
	ComputeImagePosition(x, y, w, h);

	m_pImage->SetPos(x, y);
	m_pImage->SetSize( w, h );
	m_pImage->SetColor( m_bgColor );
	surface()->DrawSetColor( m_bgColor );
	m_pImage->Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapImagePanel::setTexture( char const *filename, bool hardwareFiltered )
{
	m_hardwareFiltered = hardwareFiltered;

	if ( m_pszImageName )
	{
		delete[] m_pszImageName;
		m_pszImageName = NULL;
	}
	if ( filename && filename[ 0 ] )
	{
		m_pImage = scheme()->GetImage( filename, m_hardwareFiltered );
		m_pszImageName = CloneString( filename );
	}
	else
	{
		m_pImage = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapImagePanel::SetContentAlignment(Label::Alignment alignment)
{
	m_contentAlignment=alignment;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Gets control settings for editing
//-----------------------------------------------------------------------------
void CBitmapImagePanel::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	if (m_pszImageName)
	{
		outResourceData->SetString("image", m_pszImageName);
	}
	if (m_pszColorName)
	{
		outResourceData->SetString("imagecolor", m_pszColorName);
	}
	const char *alignmentString = "";
	switch ( m_contentAlignment )
	{
	case Label::a_northwest:	alignmentString = "north-west";	break;
	case Label::a_north:		alignmentString = "north";		break;
	case Label::a_northeast:	alignmentString = "north-east";	break;
	case Label::a_center:		alignmentString = "center";		break;
	case Label::a_east:			alignmentString = "east";		break;
	case Label::a_southwest:	alignmentString = "south-west";	break;
	case Label::a_south:		alignmentString = "south";		break;
	case Label::a_southeast:	alignmentString = "south-east";	break;
	case Label::a_west:	
	default:			alignmentString = "center";	break;
	}
	outResourceData->SetString( "imageAlignment", alignmentString );
	outResourceData->SetInt("preserveAspectRatio", m_preserveAspectRatio);
	outResourceData->SetInt("filtered", m_hardwareFiltered);
}

//-----------------------------------------------------------------------------
// Purpose: Applies designer settings from res file
//-----------------------------------------------------------------------------
void CBitmapImagePanel::ApplySettings(KeyValues *inResourceData)
{
	if ( m_pszImageName )
	{
		delete[] m_pszImageName;
		m_pszImageName = NULL;
	}

	if ( m_pszColorName )
	{
		delete[] m_pszColorName;
		m_pszColorName = NULL;
	}

	const char *imageName = inResourceData->GetString("image", "");
	if (*imageName)
	{
		setTexture( imageName );
	}

	const char *colorName = inResourceData->GetString("imagecolor", "");
	if (*colorName)
	{
		m_pszColorName = CloneString( colorName );
		InvalidateLayout(false,true); // force ApplySchemeSettings to run
	}

	const char *keyString = inResourceData->GetString("imageAlignment", "");
	if (keyString && *keyString)
	{
		int align = -1;

		if ( !stricmp(keyString, "north-west") )
		{
			align = Label::a_northwest;
		}
		else if ( !stricmp(keyString, "north") )
		{
			align = Label::a_north;
		}
		else if ( !stricmp(keyString, "north-east") )
		{
			align = Label::a_northeast;
		}
		else if ( !stricmp(keyString, "west") )
		{
			align = Label::a_west;
		}
		else if ( !stricmp(keyString, "center") )
		{
			align = Label::a_center;
		}
		else if ( !stricmp(keyString, "east") )
		{
			align = Label::a_east;
		}
		else if ( !stricmp(keyString, "south-west") )
		{
			align = Label::a_southwest;
		}
		else if ( !stricmp(keyString, "south") )
		{
			align = Label::a_south;
		}
		else if ( !stricmp(keyString, "south-east") )
		{
			align = Label::a_southeast;
		}

		if ( align != -1 )
		{
			SetContentAlignment( (Label::Alignment)align );
		}
	}

	keyString = inResourceData->GetString("preserveAspectRatio", "");
	if (keyString && *keyString)
	{
		m_preserveAspectRatio = atoi( keyString ) != 0;
	}

	keyString = inResourceData->GetString("filtered", "");
	if (keyString && *keyString)
	{
		m_hardwareFiltered = atoi( keyString ) != 0;
	}

	BaseClass::ApplySettings(inResourceData);
}

//-----------------------------------------------------------------------------
// Purpose:  load the image, this is done just before this control is displayed
//-----------------------------------------------------------------------------
void CBitmapImagePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);

	if ( m_pszColorName )
	{
		setImageColor( pScheme->GetColor( m_pszColorName, m_bgColor ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Describes editing details
//-----------------------------------------------------------------------------
const char *CBitmapImagePanel::GetDescription()
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s, string image, string imagecolor, alignment imageAlignment, int preserveAspectRatio, int filtered", BaseClass::GetDescription());
	return buf;
}
