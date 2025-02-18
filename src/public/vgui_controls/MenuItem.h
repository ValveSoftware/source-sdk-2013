//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MENUITEM_H
#define MENUITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Menu.h>

namespace vgui
{

class IBorder;
class TextImage;
class Menu;
class Image;

//-----------------------------------------------------------------------------
// Purpose: The items in a menu
//			MenuItems MUST have the Menu class as parents.
//-----------------------------------------------------------------------------
class MenuItem : public Button
{
	DECLARE_CLASS_SIMPLE( MenuItem, Button );

public:
	MenuItem(Menu *parent, const char *panelName, const char *text, Menu *cascadeMenu = NULL, bool checkable = false);
	MenuItem(Menu *parent, const char *panelName, const wchar_t *wszText, Menu *cascadeMenu = NULL, bool checkable = false);
	~MenuItem();

	virtual void Paint();

	// Activate the menu item as if it had been selected by the user
	virtual void FireActionSignal();

    virtual bool CanBeDefaultButton(void);

	// Handle mouse cursor entering a MenuItem.
	void OnCursorEntered();
	// Handle mouse cursor exiting a MenuItem. 
	void OnCursorExited();

	// Close the cascading menu if we have one.
	void CloseCascadeMenu();

	// Pass kill focus events up to parent on loss of focus
	MESSAGE_FUNC( OnKillFocus, "MenuClose" );

	// Return true if this item triggers a cascading menu
	bool HasMenu();

	// Set the size of the text portion of the label.
	void SetTextImageSize(int wide, int tall);

	//Return the size of the text portion of the label.
	void GetTextImageSize(int &wide, int &tall);

	// Return the size of the arrow portion of the label.
	void GetArrowImageSize(int &wide, int &tall);

	// Return the size of the check portion of the label.
	void GetCheckImageSize(int &wide, int &tall);

	// Return the menu that this menuItem contains
	Menu *GetMenu();

	virtual void PerformLayout();

	// Respond to cursor movement
	void OnCursorMoved(int x, int y);

	// Highlight item
	MESSAGE_FUNC( ArmItem, "ArmItem" );
	// Unhighlight item.
	MESSAGE_FUNC( DisarmItem, "DisarmItem" );

	// is the item highlighted?
	bool IsItemArmed();

	// Open cascading menu if there is one.
	void OpenCascadeMenu();

	bool IsCheckable();
	bool IsChecked();

	// Set a checkable menuItem checked or unchecked.
	void SetChecked(bool state);

	KeyValues *GetUserData();
	void SetUserData(const KeyValues *kv);

	int GetActiveItem() { if ( m_pCascadeMenu ) { return m_pCascadeMenu->GetActiveItem(); } else { return 0; }} 

	Menu *GetParentMenu();

	void SetCurrentKeyBinding( char const *keyName );

	virtual void GetContentSize( int& cw, int &ch );

	void SetOffsetFromMainMenu(int nOffset) { m_nOffsetFromMainMenu = nOffset; }
	int GetOffsetFromMainMenu() const { return m_nOffsetFromMainMenu; }

	void SetPaddingY( int nPadding ) { m_nPaddingY = nPadding; }
	int GetPaddingY() const { return m_nPaddingY; }

protected:
	void OnKeyCodeReleased(KeyCode code);
	void OnMenuClose();
	MESSAGE_FUNC( OnKeyModeSet, "KeyModeSet" );

	// vgui overrides
	virtual void Init( void );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

private:
	enum { CHECK_INSET = 6 };
	Menu *m_pCascadeMenu;  // menu triggered to open upon selecting this menu item
 	bool m_bCheckable;     // can this menu item have a little check to the left of it when you select it?
	bool m_bChecked;       // whether item is checked or not.
	TextImage *m_pCascadeArrow; // little arrow that appears to the right of menuitems that open a menu
	Image *m_pCheck;  // the check that appears to the left of checked menu items
	TextImage *m_pBlankCheck;  // a blank image same size as the check for when items are not checked.

	TextImage	*m_pCurrentKeyBinding; // An optional indicator for the key currently bound to this menu item

	KeyValues *m_pUserData;

	int m_nOffsetFromMainMenu;
	int m_nPaddingY;
};

} // namespace vgui

#endif // MENUITEM_H
