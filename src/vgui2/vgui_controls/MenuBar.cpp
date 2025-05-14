//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/IBorder.h>
#include <vgui/ISurface.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/MenuBar.h>
#include <vgui_controls/MenuButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


enum 
{
	MENUBARINDENT = 4, // indent from top and bottom of panel.
};

DECLARE_BUILD_FACTORY( MenuBar );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
MenuBar::MenuBar(Panel *parent, const char *panelName) : 
	Panel(parent, panelName),
	m_nRightEdge( 0 )
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
MenuBar::~MenuBar()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MenuBar::AddButton(MenuButton *button)
{
	button->SetParent(this);
	button->AddActionSignalTarget(this);
	m_pMenuButtons.AddToTail(button);
}

//-----------------------------------------------------------------------------
// This will add the menu to the menu bar
//-----------------------------------------------------------------------------
void MenuBar::AddMenu( const char *pButtonName, Menu *pMenu )
{
	MenuButton *pMenuButton = new MenuButton(this, pButtonName, pButtonName);
	pMenuButton->SetMenu(pMenu);
	AddButton(pMenuButton);
}


//-----------------------------------------------------------------------------
// Purpose: Handle key presses, Activate shortcuts
//-----------------------------------------------------------------------------
void MenuBar::OnKeyCodeTyped(KeyCode code)
{
	switch(code)
	{
	case KEY_RIGHT:
		{
			// iterate the menu items looking for one that is open
			// if we find one open, open the one to the right
			for (int i = 0; i < m_pMenuButtons.Count() - 1; i++)
			{
				MenuButton *panel = m_pMenuButtons[i];
				if (panel->IsDepressed())
				{
					m_pMenuButtons[i]->DoClick();
					m_pMenuButtons[i+1]->DoClick();
					break;
				}
			}
			break;
		}
	case KEY_LEFT:
		{
			// iterate the menu items looking for one that is open
			// if we find one open, open the one to the left
			for (int i = 1; i < m_pMenuButtons.Count(); i++)
			{
				MenuButton *panel = m_pMenuButtons[i];
				if (panel->IsDepressed())
				{
					m_pMenuButtons[i]->DoClick();
					m_pMenuButtons[i-1]->DoClick();
					break;
				}
			}
			break;
		}
	default:
		{
			break;
		}
	}
	
	// don't chain back
}

//-----------------------------------------------------------------------------
// Purpose: Handle key presses, Activate shortcuts
// Input  : code - 
//-----------------------------------------------------------------------------
void MenuBar::OnKeyTyped(wchar_t unichar)
{
	if (unichar)
	{
		// iterate the menu items looking for one with the matching hotkey
		for (int i = 0; i < m_pMenuButtons.Count(); i++)
		{
			MenuButton *panel = m_pMenuButtons[i];
			if (panel->IsVisible())
			{
				Panel *hot = panel->HasHotkey(unichar);
				if (hot)
				{
					// post a message to the menuitem telling it it's hotkey was pressed
					PostMessage(hot, new KeyValues("Hotkey"));
					return;
				}
			}
		}
	}
	
	// don't chain back
}

void MenuBar::Paint()
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	for ( int i = 0; i < m_pMenuButtons.Count(); i++)
	{
		if (!m_pMenuButtons[i]->IsArmed())
			m_pMenuButtons[i]->SetDefaultBorder(NULL);
		else
		{
			m_pMenuButtons[i]->SetDefaultBorder(pScheme->GetBorder( "ButtonBorder"));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
void MenuBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// get the borders we need
	SetBorder(pScheme->GetBorder("ButtonBorder"));

	// get the background color
	SetBgColor(pScheme->GetColor( "MenuBar.BgColor", GetBgColor() ));

}



//-----------------------------------------------------------------------------
// Purpose: Reformat according to the new layout
//-----------------------------------------------------------------------------
void MenuBar::PerformLayout()
{
	int nBarWidth, nBarHeight;
	GetSize( nBarWidth, nBarHeight );

	// Now position + resize all buttons
	int x = QuickPropScale( MENUBARINDENT );
	for ( int i = 0; i < m_pMenuButtons.Count(); ++i )
	{
		int nWide, nTall;

		m_pMenuButtons[i]->GetContentSize(nWide, nTall);
		m_pMenuButtons[i]->SetPos( x, QuickPropScale( MENUBARINDENT ) );
		m_pMenuButtons[i]->SetSize( nWide + QuickPropScale( Label::Content ), nBarHeight - 2 * QuickPropScale( MENUBARINDENT ) );

		x += nWide + QuickPropScale( MENUBARINDENT );
	}

	m_nRightEdge = x;
}

//-----------------------------------------------------------------------------
// Purpose: Get the size of the menus in the bar (so other children can be added to menu bar)
// Input  : w - 
//			int&h - 
//-----------------------------------------------------------------------------
void MenuBar::GetContentSize( int& w, int&h )
{
	w = m_nRightEdge + QuickPropScale( 2 );
	h = GetTall();
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
void MenuBar::OnMenuClose()
{
	RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
#ifdef PLATFORM_64BITS
void MenuBar::OnCursorEnteredMenuButton(vgui::Panel* VPanel)
#else
void MenuBar::OnCursorEnteredMenuButton(int VPanel)
#endif
{
	VPANEL menuButton = (VPANEL)VPanel;
	// see if we had a menu open
	for ( int i = 0; i < m_pMenuButtons.Count(); i++)
	{
		// one of our buttons was pressed.
		if (m_pMenuButtons[i]->IsDepressed())
		{
			int oldbutton = i;
			// now see if menuButton is one of ours.
			for ( int j = 0; j < m_pMenuButtons.Count(); j++)
			{
				MenuButton *button = static_cast<MenuButton *>(ipanel()->GetPanel(menuButton, GetModuleName()));
				// it is one of ours.
				if ( button == m_pMenuButtons[j])
				{
					// if its a different button than the one we already had open,
					if (j != oldbutton)
					{
						// close this menu and open the one we just entered 
						m_pMenuButtons[oldbutton]->DoClick();
						button->DoClick();
					}
				}
			}
		}
	}
}
