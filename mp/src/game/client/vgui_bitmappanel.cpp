//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#pragma warning (disable: 4514)
#include "vgui_bitmappanel.h"
#include <KeyValues.h>
#include "panelmetaclassmgr.h"
#include "vgui_bitmapimage.h"

#ifdef INVASION_CLIENT_DLL
#include "hud_commander_statuspanel.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_BUILD_FACTORY( CBitmapPanel );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBitmapPanel::CBitmapPanel( ) :	BaseClass( NULL, "CBitmapPanel" ), m_pImage(0)
{
	SetPaintBackgroundEnabled( false );
	m_szMouseOverText[ 0 ] = 0;
	m_r = m_g = m_b = m_a = 255;
	m_bOwnsImage = true;
}

CBitmapPanel::CBitmapPanel( vgui::Panel *pParent, const char *pName ) : 
	BaseClass( pParent, pName ), m_pImage(0)
{
	SetPaintBackgroundEnabled( false );
	m_szMouseOverText[ 0 ] = 0;
	m_r = m_g = m_b = m_a = 255;
	m_bOwnsImage = true;
}

CBitmapPanel::~CBitmapPanel()
{
	if (m_pImage && m_bOwnsImage)
	{
		delete m_pImage;
		m_pImage = NULL;
	}
}


//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CBitmapPanel::Init( KeyValues* pInitData )
{
	Assert( pInitData );

	// modulation color
	if (!ParseRGBA( pInitData, "color", m_r, m_g, m_b, m_a ))
		return false;

	int x, y, w, h;
	if (!ParseRect( pInitData, "position", x, y, w, h ))
		return false;

	const char *mouseover = pInitData->GetString( "mousehint", "" );
	if ( mouseover && mouseover[ 0 ] )
	{
		Q_strncpy( m_szMouseOverText, mouseover, sizeof( m_szMouseOverText ) );
	}

	// Set the size...
	SetPos( x, y );
	SetSize( w, h );

	char const* pClassImage = pInitData->GetString( "material" );
	if ( !pClassImage || !pClassImage[ 0 ] )
		return false;

	// hook in the bitmap
	m_pImage = new BitmapImage( GetVPanel(), pClassImage );
	m_bOwnsImage = true;

	return true;
}


//-----------------------------------------------------------------------------
// initialization from build-mode dialog style .res files
//-----------------------------------------------------------------------------
void CBitmapPanel::ApplySettings(KeyValues *pInitData)
{
	BaseClass::ApplySettings(pInitData);

	if (m_pImage && m_bOwnsImage)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	// modulation color. Can't use ParseRGBA since this uses a vgui::KeyValues (feh)
	m_r = m_g = m_b = m_a = 255;
	const char *pColorString = pInitData->GetString( "color", "255 255 255 255" );
	if ( pColorString && pColorString[ 0 ] )
	{
		// Try and scan them in
		int r = 0, g = 0, b = 0, a = 0;
		int scanned;
		scanned = sscanf( pColorString, "%i %i %i %i", &r, &g, &b, &a );
		if ( scanned == 4 )
		{
			m_r = r; m_g = g; m_b = b; m_a = a;
		}
		else
		{
			Warning( "Couldn't scan four color values from %s\n", pColorString );
		}
	}

	m_szMouseOverText[0] = 0;

	char const* pClassImage = pInitData->GetString( "material" );
	if ( pClassImage && pClassImage[ 0 ] && m_bOwnsImage )
	{
		// hook in the bitmap
		m_pImage = new BitmapImage( GetVPanel(), pClassImage );
	}
}


//-----------------------------------------------------------------------------
// Draws the puppy
//-----------------------------------------------------------------------------
void CBitmapPanel::Paint( void )
{
	if ( !m_pImage )
		return;

	Color color;
	color.SetColor( m_r, m_g, m_b, m_a );
	m_pImage->SetColor( color );
	m_pImage->DoPaint( GetVPanel() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapPanel::OnCursorEntered()
{
#ifdef INVASION_CLIENT_DLL
	if ( m_szMouseOverText[ 0 ] )
	{
		StatusPrint( TYPE_HINT, "%s", m_szMouseOverText );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBitmapPanel::OnCursorExited()
{
#ifdef INVASION_CLIENT_DLL
	if ( m_szMouseOverText[ 0 ] )
	{
		StatusClear();
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBitmapPanel::GetMouseOverText( void )
{
	return m_szMouseOverText;
}


//-----------------------------------------------------------------------------
// Purpose: Sets up the panel
//			Used by panels that aren't created by the commander overlay factory (i.e. aren't parsed from a keyvalues file)
//-----------------------------------------------------------------------------
void CBitmapPanel::SetImage( BitmapImage *pImage )
{
	m_pImage = pImage;
	m_bOwnsImage = (pImage == NULL);

	// Get the color from the image
	if ( m_pImage )
	{
		m_pImage->GetColor( m_r, m_g, m_b, m_a );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set image data directly
//-----------------------------------------------------------------------------
void CBitmapPanel::SetBitmap( const Bitmap_t &bitmap )
{

	// Make sure we have an image that we own
	if ( m_pImage == NULL || !m_bOwnsImage )
	{
		delete m_pImage;
		m_pImage = new BitmapImage( GetVPanel(), NULL );
		m_bOwnsImage = true;
	}

	// Set the bitmap
	m_pImage->SetBitmap( bitmap );
}
