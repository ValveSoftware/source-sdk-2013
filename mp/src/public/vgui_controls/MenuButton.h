//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MENUBUTTON_H
#define MENUBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>
#include "vgui_controls/Menu.h"

namespace vgui
{

class Menu;
class TextImage;

//-----------------------------------------------------------------------------
// Purpose: Button that displays a menu when pressed
//-----------------------------------------------------------------------------
class MenuButton : public Button
{
	DECLARE_CLASS_SIMPLE( MenuButton, Button );

public:
	MenuButton(Panel *parent, const char *panelName, const char *text);
	~MenuButton();

	// functions designed to be overriden
	virtual void OnShowMenu(Menu *menu) {}
	virtual void OnHideMenu(Menu *menu) {}
	virtual int	 OnCheckMenuItemCount() { return 0; }

	virtual void SetMenu(Menu *menu);
	virtual void HideMenu(void);
	virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1);
	MESSAGE_FUNC( OnMenuClose, "MenuClose" );
	MESSAGE_FUNC_PARAMS( OnKillFocus, "KillFocus", kv );		// called after the panel loses the keyboard focus
	virtual void DoClick();
	virtual void SetOpenOffsetY(int yOffset);

    virtual bool CanBeDefaultButton(void);

	// sets the direction in which the menu opens from the button, defaults to down
	virtual void SetOpenDirection(Menu::MenuDirection_e direction);

	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnCursorEntered();

	virtual void Paint();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnCursorMoved( int x, int y );

	// This style is like the IE "back" button where the left side acts like a regular button, the the right side has a little
	//  combo box dropdown indicator and presents and submenu
	void  SetDropMenuButtonStyle( bool state );
	bool	IsDropMenuButtonStyle() const;

	Menu	*GetMenu();

private:
	
	Menu *m_pMenu;
	Menu::MenuDirection_e m_iDirection;

	int _openOffsetY; // vertical offset of menu from the menu button

	bool		m_bDropMenuButtonStyle : 1;
	TextImage	*m_pDropMenuImage;
	int			m_nImageIndex;
};

}; // namespace vgui

#endif // MENUBUTTON_H
