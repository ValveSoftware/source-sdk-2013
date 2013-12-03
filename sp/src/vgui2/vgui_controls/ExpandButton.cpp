//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include <stdarg.h>
#include <stdio.h>

#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>

#include <vgui_controls/Image.h>
#include <vgui_controls/ExpandButton.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( ExpandButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ExpandButton::ExpandButton( Panel *parent, const char *panelName ) : ToggleButton( parent, panelName, "" )
{
	m_bExpandable = true;
	m_hFont = INVALID_FONT;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ExpandButton::~ExpandButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ExpandButton::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_Color = GetSchemeColor( "ExpandButton.Color", pScheme );
	m_hFont = pScheme->GetFont("Marlett", IsProportional() );

	// don't draw a background
	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBorder *ExpandButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Expand the button
//-----------------------------------------------------------------------------
void ExpandButton::SetSelected(bool state)
{
	if ( m_bExpandable && ( state != IsSelected() ) )
	{
		// send a message saying we've been checked
		KeyValues *msg = new KeyValues("Expanded", "state", (int)state);
		PostActionSignal(msg);

		BaseClass::SetSelected(state);
	}
}


//-----------------------------------------------------------------------------
// Purpose: sets whether or not the state of the check can be changed
//-----------------------------------------------------------------------------
void ExpandButton::SetExpandable(bool state)
{
	m_bExpandable = state;
	Repaint();
}


void ExpandButton::Paint()
{
	surface()->DrawSetTextFont( m_hFont );

	wchar_t code = IsSelected( ) ? L'6' : L'4';
	wchar_t pString[2] = { code, 0 };

	// draw selected check
	int tw, th, w, h;
	GetSize( w, h );
	surface()->GetTextSize( m_hFont, pString, tw, th );
	surface()->DrawSetTextColor( m_Color );
	surface()->DrawSetTextPos( ( w - tw ) / 2, ( h - th ) / 2 );
	surface()->DrawUnicodeChar( code );
}

	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ExpandButton::OnExpanded(Panel *panel)
{
}
