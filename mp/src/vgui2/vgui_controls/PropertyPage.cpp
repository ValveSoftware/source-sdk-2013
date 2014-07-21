//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vgui/IScheme.h"
#include "vgui/KeyCode.h"
#include "vgui/ISurface.h"
#include "KeyValues.h"

#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/Controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertyPage::PropertyPage(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PropertyPage::~PropertyPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when page is loaded.  Data should be reloaded from document into controls.
//-----------------------------------------------------------------------------
void PropertyPage::OnResetData()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into document.
//-----------------------------------------------------------------------------
void PropertyPage::OnApplyChanges()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageHide()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pageTab - 
//-----------------------------------------------------------------------------
void PropertyPage::OnPageTabActivated(Panel *pageTab)
{
	_pageTab = pageTab;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyPage::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
        // left and right only get propogated to parents if our tab has focus
	case KEY_RIGHT:
		{
            if (_pageTab != 0 && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	case KEY_LEFT:
		{
            if (_pageTab != 0 && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	default:
		BaseClass::OnKeyCodeTyped(code);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyPage::SetVisible(bool state)
{
    if (IsVisible() && !state)
    {
        // if we're going away and we have a current button, get rid of it
        if (GetFocusNavGroup().GetCurrentDefaultButton())
        {
            GetFocusNavGroup().SetCurrentDefaultButton(NULL);
        }
    }

    BaseClass::SetVisible(state);
}

