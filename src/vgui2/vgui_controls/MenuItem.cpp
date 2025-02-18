//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "vgui/ISurface.h"
#include <KeyValues.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Check box image
//-----------------------------------------------------------------------------
class MenuItemCheckImage : public TextImage
{
public:
	MenuItemCheckImage(MenuItem *item) : TextImage( "g" )
	{
		_menuItem = item;

		SetSize(20, 13);
	}

	virtual void Paint()
	{
		DrawSetTextFont(GetFont());
		
		// draw background
		DrawSetTextColor(_menuItem->GetBgColor());
		DrawPrintChar(0, 0, 'g');

		// draw check
		if (_menuItem->IsChecked())
		{
			if (_menuItem->IsEnabled())
			{
				DrawSetTextColor(_menuItem->GetButtonFgColor());
				DrawPrintChar(0, 2, 'a');
			}
			else if (!_menuItem->IsEnabled())
			{
				// draw disabled version, with embossed look
				// offset image
				DrawSetTextColor(_menuItem->GetDisabledFgColor1());
				DrawPrintChar(1, 3, 'a');
				
				// overlayed image
				DrawSetTextColor(_menuItem->GetDisabledFgColor2());
				DrawPrintChar(0, 2, 'a');
			}
		}
	}

private:
	MenuItem *_menuItem;
};

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( MenuItem, MenuItem );

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input:	parent - the parent of this menu item, usually a menu
//			text - the name of the menu item as it appears in the menu
//			cascadeMenu - if this item triggers the opening of a cascading menu
//			provide a pointer to it.
//			MenuItems cannot be both checkable and trigger a cascade menu.
//-----------------------------------------------------------------------------
MenuItem::MenuItem(Menu *parent, const char *panelName, const char *text, Menu *cascadeMenu, bool checkable) : Button(parent, panelName, text)
{
	m_pCascadeMenu = cascadeMenu;
	m_bCheckable = checkable;
	SetButtonActivationType(ACTIVATE_ONRELEASED);
	m_pUserData = NULL;
	m_pCurrentKeyBinding = NULL;
	m_nOffsetFromMainMenu = 0;
	m_nPaddingY = 0;

	// only one arg should be passed in.
	Assert (!(cascadeMenu && checkable));
	
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input:	parent - the parent of this menu item, usually a menu
//			text - the name of the menu item as it appears in the menu
//			cascadeMenu - if this item triggers the opening of a cascading menu
//			provide a pointer to it.
//			MenuItems cannot be both checkable and trigger a cascade menu.
//-----------------------------------------------------------------------------
MenuItem::MenuItem(Menu *parent, const char *panelName, const wchar_t *wszText, Menu *cascadeMenu, bool checkable) : Button(parent, panelName, wszText)
{
	m_pCascadeMenu = cascadeMenu;
	m_bCheckable = checkable;
	SetButtonActivationType(ACTIVATE_ONRELEASED);
	m_pUserData = NULL;
	m_pCurrentKeyBinding = NULL;
	m_nOffsetFromMainMenu = 0;
	m_nPaddingY = 0;

	// only one arg should be passed in.
	Assert (!(cascadeMenu && checkable));
	
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
MenuItem::~MenuItem()
{
	delete m_pCascadeMenu;
	delete m_pCascadeArrow;	
	delete m_pCheck;
	if (m_pUserData)
	{
		m_pUserData->deleteThis();
	}
	delete m_pCurrentKeyBinding;
}

//-----------------------------------------------------------------------------
// Purpose: Basic initializer
//-----------------------------------------------------------------------------
void MenuItem::Init( void )
{
	m_pCascadeArrow	= NULL;
	m_pCheck = NULL;

	if (m_pCascadeMenu)
	{
		m_pCascadeMenu->SetParent(this);
		m_pCascadeArrow = new TextImage("4");	// this makes a right pointing arrow.

		m_pCascadeMenu->AddActionSignalTarget(this);
	}
	else if (m_bCheckable)
	{
		// move the text image over so we have room for the check
		SetTextImageIndex(1);
		m_pCheck = new MenuItemCheckImage(this);
		SetImageAtIndex(0, m_pCheck, CHECK_INSET);
		SetChecked(false);
	}

	SetButtonBorderEnabled( false );
	SetUseCaptureMouse( false );
	SetContentAlignment( Label::a_west );

	// note menus handle all the sizing of menuItem panels
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Menu *MenuItem::GetParentMenu()
{
	return (Menu *)GetParent();
}

//-----------------------------------------------------------------------------
// Purpose: Layout the Textimage and the Arrow part of the menuItem
//-----------------------------------------------------------------------------
void MenuItem::PerformLayout()
{
	Button::PerformLayout();
	// make the arrow image match the button layout.
	// this will make it brighten and dim like the menu buttons.
	if (m_pCascadeArrow)
	{
		m_pCascadeArrow->SetColor(GetButtonFgColor());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Close the cascading menu if we have one.
//-----------------------------------------------------------------------------
void MenuItem::CloseCascadeMenu()
{
	if (m_pCascadeMenu)
	{
		if (m_pCascadeMenu->IsVisible())
		{
			m_pCascadeMenu->SetVisible(false);
		}
		// disarm even if menu wasn't visible!
		SetArmed(false);  
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle cursor moving in a menuItem.
//-----------------------------------------------------------------------------
void MenuItem::OnCursorMoved(int x, int y)
{
	// if menu is in keymode and we moved the mouse
	// highlight this item
	if (GetParentMenu()->GetMenuMode() == Menu::KEYBOARD)
	{
		OnCursorEntered();
	}

	// chain up to parent
	CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse cursor entering a menuItem.
//-----------------------------------------------------------------------------
void MenuItem::OnCursorEntered()
{
	// post a message to the parent menu.
	// forward the message on to the parent of this menu.
	KeyValues *msg = new KeyValues ("CursorEnteredMenuItem");
	// tell the parent this menuitem is the one that was entered so it can highlight it
#ifdef PLATFORM_64BITS
	msg->SetPtr( "VPanel", (void*) GetVPanel() );
#else
	msg->SetInt("VPanel", GetVPanel());
#endif

	ivgui()->PostMessage(GetVParent(), msg, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse cursor exiting a menuItem. 
//-----------------------------------------------------------------------------
void MenuItem::OnCursorExited()
{
	// post a message to the parent menu.
	// forward the message on to the parent of this menu.
	KeyValues *msg = new KeyValues ("CursorExitedMenuItem");
	// tell the parent this menuitem is the one that was entered so it can unhighlight it
#ifdef PLATFORM_64BITS
	msg->SetPtr( "VPanel", (void*) GetVPanel() );
#else
	msg->SetInt( "VPanel", GetVPanel() );
#endif

	ivgui()->PostMessage(GetVParent(), msg, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse cursor exiting a menuItem. 
//-----------------------------------------------------------------------------
void MenuItem::OnKeyCodeReleased(KeyCode code)
{
	if (GetParentMenu()->GetMenuMode() == Menu::KEYBOARD && m_pCascadeMenu)
	{
		return;
	}
	// only disarm if we are not opening a cascading menu using keys.
	Button::OnKeyCodeReleased(code);
}

//-----------------------------------------------------------------------------
// Purpose: Highlight a menu item
//			Menu item buttons highlight if disabled, but won't activate.
//-----------------------------------------------------------------------------
void MenuItem::ArmItem()
{
	// close all other menus 
	GetParentMenu()->CloseOtherMenus(this);
	// arm the menuItem.
	Button::SetArmed(true);	

	// When you have a submenu with no scroll bar the menu
	// border will not be drawn correctly. This fixes it.
	Menu *parent = GetParentMenu();
	if ( parent )
	{
		parent->ForceCalculateWidth();
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Unhighlight a menu item
//-----------------------------------------------------------------------------
void MenuItem::DisarmItem()
{
	// normal behaviour is that the button becomes unarmed
	// do not unarm if there is a cascading menu. CloseCascadeMenu handles this.
	// and the menu handles it since we close at different times depending
	// on whether menu is handling mouse or key events.
	if (!m_pCascadeMenu)
	{
		Button::OnCursorExited();
	}

	// When you have a submenu with no scroll bar the menu
	// border will not be drawn correctly. This fixes it.
	Menu *parent = GetParentMenu();
	if ( parent )
	{
		parent->ForceCalculateWidth();
	}
	Repaint();
}

bool MenuItem::IsItemArmed()
{
	return Button::IsArmed();
}

//-----------------------------------------------------------------------------
// Purpose: Pass kill focus events up to parent, This will tell all panels
//          in the hierarchy to hide themselves, and enables cascading menus to
//		    all disappear on selecting an item at the end of the tree.
//-----------------------------------------------------------------------------
void MenuItem::OnKillFocus()
{
	GetParentMenu()->OnKillFocus();
}

//-----------------------------------------------------------------------------
// Purpose: fire the menu item as if it has been selected and 
//			Tell the owner that it is closing
//-----------------------------------------------------------------------------
void MenuItem::FireActionSignal()
{
	// cascading menus items don't trigger the parent menu to disappear
	// (they trigger the cascading menu to open/close when cursor is moved over/off them)
	if (!m_pCascadeMenu) 
	{	
		KeyValues *kv = new KeyValues("MenuItemSelected");
		kv->SetPtr("panel", this);
		ivgui()->PostMessage(GetVParent(), kv, GetVPanel());

	//	ivgui()->PostMessage(GetVParent(), new KeyValues("MenuItemSelected"), GetVPanel());		
		Button::FireActionSignal();
		// toggle the check next to the item if it is checkable
		if (m_bCheckable)
		{
			SetChecked( !m_bChecked );
		}
	}
	else
	{
		// if we are in keyboard mode, open the child menu.
		if (GetParentMenu()->GetMenuMode() == Menu::KEYBOARD)
		{
			OpenCascadeMenu();			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the cascading menu.
//-----------------------------------------------------------------------------
void MenuItem::OpenCascadeMenu()
{		
	if (m_pCascadeMenu)
	{
		// perform layout on menu, this way it will open in the right spot 
		// if the window's been moved
		m_pCascadeMenu->PerformLayout();
		m_pCascadeMenu->SetVisible(true);
		m_pCascadeMenu->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpse: Return true if this item triggers a cascading menu
//-----------------------------------------------------------------------------
bool MenuItem::HasMenu()
{
	return (m_pCascadeMenu != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Apply the resource scheme to the menu.
//-----------------------------------------------------------------------------
void MenuItem::ApplySchemeSettings(IScheme *pScheme)
{
	// chain back first
	Button::ApplySchemeSettings(pScheme);

	// get color settings
	SetDefaultColor(GetSchemeColor("Menu.TextColor", GetFgColor(), pScheme), GetSchemeColor("Menu.BgColor", GetBgColor(), pScheme));
	SetArmedColor(GetSchemeColor("Menu.ArmedTextColor", GetFgColor(), pScheme), GetSchemeColor("Menu.ArmedBgColor", GetBgColor(), pScheme));
	SetDepressedColor(GetSchemeColor("Menu.ArmedTextColor", GetFgColor(), pScheme), GetSchemeColor("Menu.ArmedBgColor", GetBgColor(), pScheme));

	SetTextInset(QuickPropScale( atoi(pScheme->GetResourceString("Menu.TextInset")) ), 0);
	
	// reload images since applyschemesettings in label wipes them out.
	if ( m_pCascadeArrow )
	{
		m_pCascadeArrow->SetFont(pScheme->GetFont("Marlett", IsProportional() ));
		m_pCascadeArrow->ResizeImageToContent();
		AddImage(m_pCascadeArrow, 0);
	}	
	else if (m_bCheckable)
	{
		( static_cast<MenuItemCheckImage *>(m_pCheck) )->SetFont( pScheme->GetFont("Marlett", IsProportional()));
		SetImageAtIndex(0, m_pCheck, CHECK_INSET);
		( static_cast<MenuItemCheckImage *>(m_pCheck) )->ResizeImageToContent();
	}

	if ( m_pCurrentKeyBinding )
	{
		m_pCurrentKeyBinding->SetFont(pScheme->GetFont("Default", IsProportional() ));
		m_pCurrentKeyBinding->ResizeImageToContent();
	}

	// Have the menu redo the layout
	// Get the parent to resize
	Menu * parent = GetParentMenu();
	if ( parent )
	{
		parent->ForceCalculateWidth();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return the size of the text portion of the label.
//			for normal menu items this is the same as the label size, but for
//			cascading menus it gives you the size of the text portion only, without
//			the arrow.
//-----------------------------------------------------------------------------
void MenuItem::GetTextImageSize(int &wide, int &tall)
{
   GetTextImage()->GetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the text portion of the label.
//			For normal menu items this is the same as the label size, but for
//			cascading menus it sizes textImage portion only, without
//			the arrow.
//-----------------------------------------------------------------------------
void MenuItem::SetTextImageSize(int wide, int tall)
{
   GetTextImage()->SetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Return the size of the arrow portion of the label.
//			If the menuItem is not a cascading menu, 0 is returned.
//-----------------------------------------------------------------------------
void MenuItem::GetArrowImageSize(int &wide, int &tall)
{
	wide = 0, tall = 0;
	if (m_pCascadeArrow)
	{
		m_pCascadeArrow->GetSize(wide, tall);
		return;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Return the size of the check portion of the label.
//-----------------------------------------------------------------------------
void MenuItem::GetCheckImageSize(int &wide, int &tall)
{
	wide = 0, tall = 0;
	if (m_pCheck)
	{
		// resize the image to the contents size
		( static_cast<MenuItemCheckImage *>(m_pCheck) )->ResizeImageToContent();
	    m_pCheck->GetSize(wide, tall);

		// include the inset for the check, since nobody but us know about the inset
		wide += CHECK_INSET;
		return;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Return a the menu that this menuItem contains
//          This is useful when the parent menu's commands must be
//          sent through all menus that are open as well (like hotkeys)
//-----------------------------------------------------------------------------
Menu *MenuItem::GetMenu()
{
	return m_pCascadeMenu;	
}

//-----------------------------------------------------------------------------
// Purpose: Get the border style for the button. Menu items have no border so
//			return null.
//-----------------------------------------------------------------------------
IBorder *MenuItem::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Set the menu to key mode if a child menu goes into keymode
//-----------------------------------------------------------------------------
void MenuItem::OnKeyModeSet()
{
	// send the message to this parent in case this is a cascading menu
	ivgui()->PostMessage(GetVParent(), new KeyValues("KeyModeSet"), GetVPanel());
}


//-----------------------------------------------------------------------------
// Purpose: Return if this menuitem is checkable or not
// This is used by menus to perform the layout properly.
//-----------------------------------------------------------------------------
bool MenuItem::IsCheckable()
{
	return m_bCheckable;	
}

//-----------------------------------------------------------------------------
// Purpose: Return if this menuitem is checked or not
//-----------------------------------------------------------------------------
bool MenuItem::IsChecked()
{
	return m_bChecked;	
}

//-----------------------------------------------------------------------------
// Purpose: Set the checked state of a checkable menuitem
//			Does nothing if item is not checkable
//-----------------------------------------------------------------------------
void MenuItem::SetChecked(bool state)
{
	if (m_bCheckable)
	{
		m_bChecked = state;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool MenuItem::CanBeDefaultButton(void)
{
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *MenuItem::GetUserData()
{
	if ( HasMenu() )
	{
		return m_pCascadeMenu->GetItemUserData( m_pCascadeMenu->GetActiveItem() );
	}
	else
	{
		return m_pUserData;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the user data
//-----------------------------------------------------------------------------
void MenuItem::SetUserData(const KeyValues *kv)
{
	if (m_pUserData)
	{
		m_pUserData->deleteThis();
		m_pUserData = NULL;
	}
	
	if ( kv )
	{
		m_pUserData = kv->MakeCopy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Passing in NULL removes this object
// Input  : *keyName - 
//-----------------------------------------------------------------------------
void MenuItem::SetCurrentKeyBinding( char const *keyName )
{
	if ( !keyName )
	{
		delete m_pCurrentKeyBinding;
		m_pCurrentKeyBinding = NULL;
		return;
	}

	if ( !m_pCurrentKeyBinding )
	{
		m_pCurrentKeyBinding = new TextImage( keyName );
	}
	else
	{
		char curtext[ 256 ];
		m_pCurrentKeyBinding->GetText( curtext, sizeof( curtext ) );
		if ( !Q_strcmp( curtext, keyName ) )
			return;

		m_pCurrentKeyBinding->SetText( keyName );
	}

	InvalidateLayout( false, true );
}

#define KEYBINDING_INSET 5

void MenuItem::Paint()
{
	BaseClass::Paint();
	if ( !m_pCurrentKeyBinding )
		return;

	int w, h;
	GetSize( w,  h );
	int iw, ih;
	m_pCurrentKeyBinding->GetSize( iw, ih );

	int x = w - iw - QuickPropScale( KEYBINDING_INSET );
	int y = ( h - ih ) / 2;

	if ( IsEnabled() )
	{
		m_pCurrentKeyBinding->SetPos( x, y );
		m_pCurrentKeyBinding->SetColor( GetButtonFgColor() );
		m_pCurrentKeyBinding->Paint();
	}
	else
	{
		m_pCurrentKeyBinding->SetPos( x + 1 , y + 1 );
		m_pCurrentKeyBinding->SetColor( GetDisabledFgColor1() );
		m_pCurrentKeyBinding->Paint();

		surface()->DrawFlushText();

		m_pCurrentKeyBinding->SetPos( x, y );
		m_pCurrentKeyBinding->SetColor( GetDisabledFgColor2() );
		m_pCurrentKeyBinding->Paint();
	}
}

void MenuItem::GetContentSize( int& cw, int &ch )
{
	BaseClass::GetContentSize( cw, ch );
	if ( m_nOffsetFromMainMenu > 0 )
	{
		cw += QuickPropScale( GetOffsetFromMainMenu() );
	}
	
	if ( !m_pCurrentKeyBinding )
		return;

	int iw, ih;
	m_pCurrentKeyBinding->GetSize( iw, ih );

	cw += iw + QuickPropScale( KEYBINDING_INSET );
	ch = max( ch, ih );
}
