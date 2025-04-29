//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#define PROTECTED_THINGS_DISABLE

#include <vgui/IPanel.h>
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/IVGui.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/MenuButton.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( MenuButton, MenuButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
MenuButton::MenuButton(Panel *parent, const char *panelName, const char *text) : Button(parent, panelName, text)
{
	m_pMenu = NULL;
	m_iDirection = Menu::DOWN;
	m_pDropMenuImage = NULL;
	m_nImageIndex = -1;
	_openOffsetY = 0;
	m_bDropMenuButtonStyle = true;  // set to true so SetDropMenuButtonStyle() forces real init.

	SetDropMenuButtonStyle( false );
	SetUseCaptureMouse( false );
	SetButtonActivationType( ACTIVATE_ONPRESSED );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
MenuButton::~MenuButton() 
{
	delete m_pDropMenuImage;
}

//-----------------------------------------------------------------------------
// Purpose: attaches a menu to the menu button
//-----------------------------------------------------------------------------
void MenuButton::SetMenu(Menu *menu)
{
	m_pMenu = menu;

	if (menu)
	{
		m_pMenu->SetVisible(false);
		m_pMenu->AddActionSignalTarget(this);
		m_pMenu->SetParent(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Never draw a focus border
//-----------------------------------------------------------------------------
void MenuButton::DrawFocusBorder(int tx0, int ty0, int tx1, int ty1)
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the direction from the menu button the menu should open
//-----------------------------------------------------------------------------
void MenuButton::SetOpenDirection(Menu::MenuDirection_e direction)
{
	m_iDirection = direction;
}


//-----------------------------------------------------------------------------
// Purpose: hides the menu
//-----------------------------------------------------------------------------
void MenuButton::HideMenu(void)
{
	if (!m_pMenu)
		return;

	// hide the menu
	m_pMenu->SetVisible(false);

	// unstick the button
	BaseClass::ForceDepressed(false);
	Repaint();

	OnHideMenu(m_pMenu);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the menu button loses focus; hides the menu
//-----------------------------------------------------------------------------
void MenuButton::OnKillFocus( KeyValues *pParams )
{
	VPANEL hPanel = (VPANEL)pParams->GetPtr( "newPanel" );
	if ( m_pMenu && !m_pMenu->HasFocus() && hPanel != m_pMenu->GetVPanel() )
	{
		HideMenu();
	}
	BaseClass::OnKillFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the menu is closed
//-----------------------------------------------------------------------------
void MenuButton::OnMenuClose()
{
	HideMenu();
	PostActionSignal(new KeyValues("MenuClose"));
}

//-----------------------------------------------------------------------------
// Purpose: Sets the offset from where menu would normally be placed
//			Only is used if menu is ALIGN_WITH_PARENT
//-----------------------------------------------------------------------------
void MenuButton::SetOpenOffsetY(int yOffset)
{
	_openOffsetY = yOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool MenuButton::CanBeDefaultButton(void)
{
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handles hotkey accesses
//-----------------------------------------------------------------------------
void MenuButton::DoClick()
{
	if ( IsDropMenuButtonStyle() && 
		m_pDropMenuImage )
	{
		int mx, my;
		// force the menu to appear where the mouse button was pressed
		input()->GetCursorPos( mx, my );
		ScreenToLocal( mx, my );

		int contentW, contentH;
		m_pDropMenuImage->GetContentSize( contentW, contentH );
		int drawX = GetWide() - contentW - 2;
		if ( mx <= drawX || !OnCheckMenuItemCount() )
		{
			// Treat it like a "regular" button click
			BaseClass::DoClick();
			return;
		}
	}

	if ( !m_pMenu )
	{
		return;
	}

	// menu is already visible, hide the menu
	if (m_pMenu->IsVisible())
	{
		HideMenu();
		return;
	}

	// do nothing if menu is not enabled
	if (!m_pMenu->IsEnabled())
	{
		return;
	}
	// force the menu to compute required width/height
	m_pMenu->PerformLayout();

	// Now position it so it can fit in the workspace
	m_pMenu->PositionRelativeToPanel(this, m_iDirection, _openOffsetY );

	// make sure we're at the top of the draw order (and therefore our children as well)
	MoveToFront();

	// notify
	OnShowMenu(m_pMenu);

	// keep the button depressed
	BaseClass::ForceDepressed(true);

	// show the menu
	m_pMenu->SetVisible(true);

	// bring to focus
	m_pMenu->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MenuButton::OnKeyCodeTyped(KeyCode code)
{
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));

	if (!shift && !ctrl && !alt)
	{
		switch (code)
		{
		case KEY_ENTER:
			{
				if ( !IsDropMenuButtonStyle() )
				{
					DoClick();
				}
				break;
			}
		}
	}
	BaseClass::OnKeyCodeTyped(code);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MenuButton::OnCursorEntered()
{
	Button::OnCursorEntered();
	// post a message to the parent menu.
	// forward the message on to the parent of this menu.
	KeyValues *msg = new KeyValues ("CursorEnteredMenuButton");
	// tell the parent this menuitem is the one that was entered so it can open the menu if it wants
#ifdef PLATFORM_64BITS
	msg->SetPtr("VPanel", (void*)GetVPanel());
#else
	msg->SetInt("VPanel", GetVPanel());
#endif
	ivgui()->PostMessage(GetVParent(), msg, NULL);
}

// This style is like the IE "back" button where the left side acts like a regular button, the the right side has a little
//  combo box dropdown indicator and presents and submenu
void MenuButton::SetDropMenuButtonStyle( bool state )
{
	bool changed = m_bDropMenuButtonStyle != state;
	m_bDropMenuButtonStyle = state;
	if ( !changed )
		return;

	if ( state )
	{
		m_pDropMenuImage = new TextImage( "u" );
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_pDropMenuImage->SetFont(pScheme->GetFont("Marlett", IsProportional()));
		// m_pDropMenuImage->SetContentAlignment(Label::a_west);
		// m_pDropMenuImage->SetTextInset(3, 0);
		m_nImageIndex = AddImage( m_pDropMenuImage, 0 );
	}
	else
	{
		ResetToSimpleTextImage();
		delete m_pDropMenuImage;
		m_pDropMenuImage = NULL;
		m_nImageIndex = -1;
	}
}

void MenuButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pDropMenuImage )
	{
		SetImageAtIndex( 1, m_pDropMenuImage, 0 );
	}
}


void MenuButton::PerformLayout()
{
	BaseClass::PerformLayout();
	if ( !IsDropMenuButtonStyle() )
		return;

	Assert( m_nImageIndex >= 0 );
	if ( m_nImageIndex < 0 || !m_pDropMenuImage )
		return;

	int w, h;
	GetSize( w, h );

	int contentW, contentH;
	m_pDropMenuImage->ResizeImageToContent();
	m_pDropMenuImage->GetContentSize( contentW, contentH );

	SetImageBounds( m_nImageIndex, w - contentW - 2, contentW );
}

bool MenuButton::IsDropMenuButtonStyle() const
{
	return m_bDropMenuButtonStyle;
}

void MenuButton::Paint(void)
{
	BaseClass::Paint();

	if ( !IsDropMenuButtonStyle() )
		return;

	int contentW, contentH;
	m_pDropMenuImage->GetContentSize( contentW, contentH );
	m_pDropMenuImage->SetColor( IsEnabled() ? GetButtonFgColor() : GetDisabledFgColor1() );
	
	int drawX = GetWide() - contentW - 2;

	surface()->DrawSetColor(  IsEnabled() ? GetButtonFgColor() : GetDisabledFgColor1() );
	surface()->DrawFilledRect( drawX, 3, drawX + 1, GetTall() - 3 );
}

void MenuButton::OnCursorMoved( int x, int y )
{
	BaseClass::OnCursorMoved( x, y );

	if ( !IsDropMenuButtonStyle() )
		return;

	int contentW, contentH;
	m_pDropMenuImage->GetContentSize( contentW, contentH );
	int drawX = GetWide() - contentW - 2;
	if ( x <= drawX || !OnCheckMenuItemCount() )
	{
		SetButtonActivationType(ACTIVATE_ONPRESSEDANDRELEASED);
		SetUseCaptureMouse(true);
	}
	else
	{
		SetButtonActivationType(ACTIVATE_ONPRESSED);
		SetUseCaptureMouse(false);
	}
}

Menu *MenuButton::GetMenu()
{
	Assert( m_pMenu );
	return m_pMenu;
}