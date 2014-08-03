//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>

#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/MouseCode.h"
#include "vgui/Cursor.h"
#include "KeyValues.h"

#include "vgui_controls/URLLabel.h"
#include "vgui_controls/TextImage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

vgui::Panel *URLLabel_Factory()
{
	return new URLLabel(NULL, NULL, "URLLabel", NULL);
}

DECLARE_BUILD_FACTORY_CUSTOM( URLLabel, URLLabel_Factory );
//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
URLLabel::URLLabel(Panel *parent, const char *panelName, const char *text, const char *pszURL) : Label(parent, panelName, text)
{
    m_pszURL = NULL;
	m_bUnderline = false;
    m_iURLSize = 0;
    if (pszURL && strlen(pszURL) > 0)
    {
        SetURL(pszURL);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unicode constructor
//-----------------------------------------------------------------------------
URLLabel::URLLabel(Panel *parent, const char *panelName, const wchar_t *wszText, const char *pszURL) : Label(parent, panelName, wszText)
{
    m_pszURL = NULL;
	m_bUnderline = false;
    m_iURLSize = 0;
    if (pszURL && strlen(pszURL) > 0)
    {
        SetURL(pszURL);
    }
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
URLLabel::~URLLabel()
{
    if (m_pszURL)
        delete [] m_pszURL;
}

//-----------------------------------------------------------------------------
// Purpose: sets the URL
//-----------------------------------------------------------------------------
void URLLabel::SetURL(const char *pszURL)
{
	int iNewURLSize = strlen(pszURL);
	if (iNewURLSize > m_iURLSize || !m_pszURL)
	{
		delete [] m_pszURL;
		m_pszURL = new char [iNewURLSize + 1];
	}
	strcpy(m_pszURL, pszURL);
	m_iURLSize = iNewURLSize;
}

//-----------------------------------------------------------------------------
// Purpose: If we were left clicked on, launch the URL
//-----------------------------------------------------------------------------
void URLLabel::OnMousePressed(MouseCode code)
{
    if (code == MOUSE_LEFT)
    {
        if (m_pszURL)
		{
	        system()->ShellExecute("open", m_pszURL);
		}
    }
}

//-----------------------------------------------------------------------------
// Purpose: Applies resouce settings
//-----------------------------------------------------------------------------
void URLLabel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	const char *pszURL = inResourceData->GetString("URLText", NULL);
	if (pszURL)
	{
		if (pszURL[0] == '#')
		{
			// it's a localized url, look it up
			const wchar_t *ws = g_pVGuiLocalize->Find(pszURL + 1);
			if (ws)
			{
				char localizedUrl[512];
				g_pVGuiLocalize->ConvertUnicodeToANSI(ws, localizedUrl, sizeof(localizedUrl));
				SetURL(localizedUrl);
			}
		}
		else
		{
	        SetURL(pszURL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves them to disk
//-----------------------------------------------------------------------------
void URLLabel::GetSettings( KeyValues *outResourceData )
{
	BaseClass::GetSettings(outResourceData);

	if (m_pszURL)
	{
		outResourceData->SetString("URLText", m_pszURL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a description of the label string
//-----------------------------------------------------------------------------
const char *URLLabel::GetDescription( void )
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s, string URLText", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: scheme settings
//-----------------------------------------------------------------------------
void URLLabel::ApplySchemeSettings(IScheme *pScheme)
{
	// set our font to be underlined by default
	// the Label::ApplySchemeSettings() will override it if override set in scheme file
	SetFont( pScheme->GetFont( "DefaultUnderline", IsProportional() ) );
    BaseClass::ApplySchemeSettings(pScheme);
    SetCursor(dc_hand);
}

