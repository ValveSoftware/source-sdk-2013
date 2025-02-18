//========= Copyright Valve Corporation, All rights reserved. =========	===//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sc_hinticon.h"
#include <vgui/IVGui.h>
#include "inputsystem/iinputsystem.h"
#include <vgui_controls/ImageList.h>
#include "imageutils.h"
#include "bitmap/bitmap.h"
#include <vgui/ISurface.h>


using namespace vgui;

DECLARE_BUILD_FACTORY( CSCHintIcon );

int CSCHintIcon::s_nVGUITextureForOrigin[k_EControllerActionOrigin_Count];

//-----------------------------------------------------------------------------
CSCHintIcon::CSCHintIcon( vgui::Panel *parent, const char* panelName ) :
	vgui::Panel( parent, panelName )
	, m_actionSetHandle( 0 )
	, m_nGlyphTexture( 0 )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSCHintIcon::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	auto szActionName = inResourceData->GetString( "actionName", nullptr );
	auto szActionSet = inResourceData->GetString( "actionSet", nullptr );

	// Msg( "actionName = %s actionSet = %s panel = %s\n", szActionName, szActionSet, this->GetName() );

	SetAction( szActionName, szActionSet );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSCHintIcon::SetAction( const char* szActionName, const char* szActionSet )
{
	if ( szActionSet )
	{
		m_strActionSet.Set( szActionSet );
		m_actionSetHandle = g_pInputSystem->GetActionSetHandle( szActionSet );
	}

	if ( szActionName )
	{
		m_strActionName.Set( szActionName );
	}

	if ( m_actionSetHandle )
	{
		auto origin = g_pInputSystem->GetSteamControllerActionOrigin( m_strActionName.Get(), m_actionSetHandle );
		m_nGlyphTexture = GetVGUITextureIDForActionOrigin( origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSCHintIcon::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetAction( nullptr, nullptr );					// nullptr = keep the same as it is now, but will cause origin glyph to be refreshed
}

void CSCHintIcon::PaintBackground()
{
	if ( m_nGlyphTexture )
	{
		vgui::surface()->DrawSetTexture( m_nGlyphTexture );
		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
		int wide, tall;
		GetSize( wide, tall );
		int size = MIN( wide, tall );
		int x = (wide - size) / 2;
		int y = (tall - size) / 2;
		vgui::surface()->DrawTexturedRect( x, y, x + size, y + size );
		vgui::surface()->DrawSetTexture( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSCHintIcon::GetVGUITextureIDForActionOrigin( EControllerActionOrigin eOrigin )
{
	if ( eOrigin == k_EControllerActionOrigin_None )
	{
		return 0;
	}
	else if ( s_nVGUITextureForOrigin[(int)eOrigin] != 0 )
	{
		return s_nVGUITextureForOrigin[(int)eOrigin];
	}
	else
	{
		auto pSteamController = g_pInputSystem->SteamControllerInterface();
		if ( pSteamController )
		{
			const char* szGlyph = pSteamController->GetGlyphForActionOrigin( eOrigin );
			if ( szGlyph )
			{
				Bitmap_t bmpGlyph;
				ConversionErrorType error = ImgUtl_LoadBitmap( szGlyph, bmpGlyph );
				if ( error == CE_SUCCESS )
				{
					int nTextureID = vgui::surface()->CreateNewTextureID( true );
					vgui::surface()->DrawSetTextureRGBA( nTextureID, bmpGlyph.GetBits(), bmpGlyph.Width(), bmpGlyph.Height(), 1, true );
					s_nVGUITextureForOrigin[(int)eOrigin] = nTextureID;
					return nTextureID;
				}
				else
				{
					Warning( "Unable to load glyph image %s\n", szGlyph );
				}
			}
			else
			{
				// Warning( "Steam API did not return glyph image for origin %d\n", (int)eOrigin );
			}
		}
	}
	
	return 0;
}