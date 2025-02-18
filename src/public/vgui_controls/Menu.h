//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef MENU_H
#define MENU_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <utllinkedlist.h>
#include <utlvector.h>

namespace vgui
{

class MenuItem;
class ScrollBar;
class MenuSeparator;

//-----------------------------------------------------------------------------
// Purpose: A menu is a list of items that can be selected with one click, navigated
//			with arrow keys and/or hot keys, and have a lit behavior when mouse over.
//			It is NOT the button which opens the menu, but only the menu itself.
//
// Behaviour spec:
// Menu navigation can be done in 2 modes, via keyboard keys and via mouse.
// Clicking on menu button opens menu.
// Only one item in a menu is highlighted at a time.
// Only one submenu in a menu is open at a time.
// Disabled menuitems get highlighted via mouse and keys but will not activate.
//
// Mouse:
//   Moving mouse into a menuitem highlights it.
//   If the menuitem has a cascading menu, the menu opens when the mouse enters
//    the menuitem. The cascading menuitem stays highlighted while its menu is open.
//   No submenu items are highlighted by default.
//   Moving the mouse into another menuitem closes any previously open submenus in the list.
//   Clicking once in the menu item activates the menu item and closes all menus.
//   Moving the mouse off a menuitem unhighlights it.
//   The scroll bar arrows can be used to move up/down the menu one item at a time.
//   The clicking and dragging on the scroll bar nob also scrolls the menu items.
//   If a highlighed menuitem scrolls off, and the user then begins navigating via keys,
//    the menu will snap the scroll bar so the highlighted item is visible.
//   If user has been navigating via keys, moving the mouse over a menu item 
//    highlights it.
// Mousewheel:
//   You must have the mouse inside the menu/scroll bar to use the wheel.
//   The mouse wheel moves the highlighted menuitem up or down the list.
//   If the list has no scroll bar the wheel will cycle from the bottom of the list
//    to the top of the list and vice versa.
//   If the list has a scrollbar the mouse wheel will stop at the top or bottom
//    of the list. 
//   If the mouse is over the scroll bar no items are highlighted.
// Keyboard:
//   When a menu is opened, no items are highlighted.
//   If a menuitem has a cascading menu it does not open when the item is highlighted.
//   The down arrow selects the next item in the list. 
//    (first item if none are highlighted and there is a scrollbar).
//   The up arrow selects the previous item in the list 
//    (first item if none are highlighted and there is a scrollbar, last item if none are
//    highlighted and there is no scrollbar).
//   Selecting a new menuitem closes any previously open submenus in the list.
//   The enter key activates the selected item and closes all menus.
//   If the selected item has a cascading menu, activating it opens its submenu.
//   These may also be activated by pressing the right arrow.
//   Pressing the left arrow closes the submenu.
//   When the submenu is opened the cascading menuitem stays highlighted.
//   No items in the submenu are highlighted when it is opened.
//   
//   Note: Cascading menuitems in menus with a scrollbar is not supported.
//         Its a clunky UI and if we want this we should design a better solution,
//         perhaps along the lines of how explorer's bookmarks does it.
//         It currently functions, but there are some arm/disarm bugs.
//
//
//-----------------------------------------------------------------------------
class Menu : public Panel
{
	DECLARE_CLASS_SIMPLE( Menu, Panel );
	friend class MenuItem;
public:
	enum MenuDirection_e
	{
		LEFT,
		RIGHT,
		UP,
		DOWN,
		CURSOR,	// make the menu appear under the mouse cursor
		ALIGN_WITH_PARENT, // make the menu appear under the parent
	};

	Menu(Panel *parent, const char *panelName);
	~Menu();

	static void PlaceContextMenu( Panel *parent, Menu *menu );
	static void OnInternalMousePressed( Panel *other, MouseCode code );

	virtual void PositionRelativeToPanel( Panel *reference, MenuDirection_e direction, int nAdditionalYOffset = 0, bool showMenu = false );

	  // the menu.  For combo boxes, it's the edit/field, etc. etc.

	// Add a simple text item to the menu
	virtual int AddMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, const KeyValues *userData = NULL );
	virtual int AddMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, const KeyValues *userData = NULL );

	virtual int AddMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target , const KeyValues *userData = NULL);
	virtual int AddMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target , const KeyValues *userData = NULL);
	
	virtual int AddMenuItem( const char *itemText, const char *command, Panel *target , const KeyValues *userData = NULL);
	virtual int AddMenuItem( const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData = NULL );
	virtual int AddMenuItem( const char *itemText, Panel *target, const KeyValues *userData = NULL );

	// Add a checkable item to the menu
	virtual int AddCheckableMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, const KeyValues *userData = NULL );
	virtual int AddCheckableMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, const KeyValues *userData = NULL );

	virtual int AddCheckableMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData = NULL );
	virtual int AddCheckableMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target, const KeyValues *userData = NULL );

	virtual int AddCheckableMenuItem( const char *itemText, const char *command, Panel *target , const KeyValues *userData = NULL);
	virtual int AddCheckableMenuItem( const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData = NULL );
	virtual int AddCheckableMenuItem( const char *itemText, Panel *target, const KeyValues *userData = NULL );

	// Add a cascading menu item to the menu
	virtual int AddCascadingMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );
	virtual int AddCascadingMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );

	virtual int AddCascadingMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );
	virtual int AddCascadingMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );
	
	virtual int AddCascadingMenuItem( const char *itemText, const char *command, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );
	virtual int AddCascadingMenuItem( const char *itemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );
	virtual int AddCascadingMenuItem( const char *itemText, Panel *target, Menu *cascadeMenu, const KeyValues *userData = NULL );

	// Add a custom panel to the menu
	virtual int AddMenuItem( MenuItem *panel );

	virtual void AddSeparator();
	virtual void AddSeparatorAfterItem( int itemID );

	// Sets the values of a menu item at the specified index
	virtual void UpdateMenuItem(int itemID, const char *itemText,KeyValues *message, const KeyValues *userData = NULL);
	virtual void UpdateMenuItem(int itemID, const wchar_t *wszItemText,KeyValues *message, const KeyValues *userData = NULL);

	virtual void MoveMenuItem( int itemID, int moveBeforeThisItemID );

	virtual bool IsValidMenuID(int itemID);
	virtual int GetInvalidMenuID();

	KeyValues *GetItemUserData(int itemID);
	void GetItemText(int itemID, wchar_t *text, int bufLenInBytes);
	void GetItemText(int itemID, char *text, int bufLenInBytes);

	virtual void SetItemEnabled(const char *itemName, bool state);
	virtual void SetItemEnabled(int itemID, bool state);
	virtual void SetItemVisible(const char *itemName, bool visible);
	virtual void SetItemVisible(int itemID, bool visible);

	// Remove a single item
	void DeleteItem( int itemID );

	// Clear the menu, deleting all the menu items within
	void DeleteAllItems();

	// Override the auto-width setting with a single fixed width
	virtual void SetFixedWidth( int width );

	// Sets the content alignment of all items in the menu
	void SetContentAlignment( Label::Alignment alignment );

	// sets the height of each menu item
	virtual void SetMenuItemHeight(int itemHeight);
	virtual int  GetMenuItemHeight() const;

	// Set the max number of items visible (scrollbar appears with more)
	virtual void SetNumberOfVisibleItems( int numItems );

	// Add the menu to the menu manager (see Menu::SetVisible())?
	void EnableUseMenuManager( bool bUseMenuManager );

	// Set up the menu items layout
	virtual void PerformLayout( void );

	virtual void SetBorder(class IBorder *border);
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// Set type ahead behaviour
	enum MenuTypeAheadMode
	{
		COMPAT_MODE = 0,
		HOT_KEY_MODE,
		TYPE_AHEAD_MODE,
	};
	virtual void SetTypeAheadMode(MenuTypeAheadMode mode);
	virtual int GetTypeAheadMode();

	// Hotkey handling
	virtual void OnKeyTyped(wchar_t unichar);
	// Menu nagivation etc.
	virtual void OnKeyCodeTyped( KeyCode code );

	// Visibility
	virtual void SetVisible(bool state);

	// Activates item in the menu list, as if that menu item had been selected by the user
	virtual void ActivateItem(int itemID);
	virtual void SilentActivateItem(int itemID); // activate item, but don't fire the action signal
	virtual void ActivateItemByRow(int row);
	virtual int GetActiveItem();		// returns the itemID (not the row) of the active item

	// Return the number of items currently in the menu list
	virtual int GetItemCount() const;

	// return the menuID of the n'th item in the menu list, valid from [0, GetItemCount)
	virtual int GetMenuID(int index);
	
	// Return the number of items currently visible in the menu list
	int GetCurrentlyVisibleItemsCount();
	
	MenuItem *GetMenuItem(int itemID);
	void CloseOtherMenus(MenuItem *item);
	virtual void OnKillFocus();

	int GetMenuMode();
	enum MenuMode
	{
		MOUSE = 0,
		KEYBOARD,
	};

	void SetCurrentlyHighlightedItem(int itemID);
	int GetCurrentlyHighlightedItem();
	void ClearCurrentlyHighlightedItem();
	
	// Set the checked state of a checkable menuItem
	void SetMenuItemChecked(int itemID, bool state);
	bool IsChecked(int index); // check if item is checked.

	
	void SetMinimumWidth(int width);
	int  GetMinimumWidth();

	// baseclass overrides to chain colors through to cascade menus
	virtual void SetFgColor( Color newColor );
	virtual void SetBgColor( Color newColor );

	virtual void SetFont( HFont font );

	// Pass in NULL hotkey to remove hotkey
	void SetCurrentKeyBinding( int itemID, char const *hotkey );

	void ForceCalculateWidth();

	void SetUseFallbackFont( bool bState, HFont hFallback );

protected:
	// helper functions	
	int AddMenuItemCharCommand(MenuItem *item, const char *command, Panel *target, const KeyValues *userData);
	int AddMenuItemKeyValuesCommand(MenuItem *item, KeyValues *message, Panel *target, const KeyValues *userData);

	// vgui result reporting
	virtual void OnCommand( const char *command );
	MESSAGE_FUNC_PTR( OnMenuItemSelected, "MenuItemSelected", panel );
	virtual void AddScrollBar();
	virtual void RemoveScrollBar();
	MESSAGE_FUNC( OnSliderMoved, "ScrollBarSliderMoved" );
	virtual void Paint();
	virtual void LayoutMenuBorder();
	virtual void MakeItemsVisibleInScrollRange( int maxVisibleItems, int nNumPixelsAvailable );
	virtual void OnMouseWheeled(int delta);
	// Alternate OnKeyTyped behaviors
	virtual void OnHotKey(wchar_t unichar);
	virtual void OnTypeAhead(wchar_t unichar);

	int	CountVisibleItems();
	void ComputeWorkspaceSize( int& workWide, int& workTall );
	int ComputeFullMenuHeightWithInsets();

	void CalculateWidth();

	void LayoutScrollBar();
	void PositionCascadingMenu();
	void SizeMenuItems();
	void OnCursorMoved(int x, int y);
	void OnKeyCodePressed(KeyCode code);
	void OnMenuClose();
	MESSAGE_FUNC( OnKeyModeSet, "KeyModeSet" );

	void SetCurrentlySelectedItem(MenuItem *item);
	void SetCurrentlySelectedItem(int itemID);
	MESSAGE_FUNC_INT( OnCursorEnteredMenuItem, "CursorEnteredMenuItem", VPanel);
	MESSAGE_FUNC_INT( OnCursorExitedMenuItem, "CursorExitedMenuItem", VPanel);

	void MoveAlongMenuItemList(int direction, int loopCount); 

	enum 
	{
		DEFAULT_MENU_ITEM_HEIGHT = 22, // height of items in the menu
		MENU_UP = -1, // used for moving up/down list of menu items in the menu
		MENU_DOWN = 1
	};

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, char *pchName );
#endif // DBGFLAG_VALIDATE

private:
	MenuItem *GetParentMenuItem();

	int 			m_iMenuItemHeight;
	int 			m_iFixedWidth;
	int 			m_iMinimumWidth; // a minimum width the menu has to be if it is not fixed width
	int 			m_iNumVisibleLines;	// number of items in menu before scroll bar adds on
	ScrollBar 		*m_pScroller;

	CUtlLinkedList<MenuItem*, int> 	m_MenuItems;

	CUtlVector<int>					m_VisibleSortedItems;
	CUtlVector<int>					m_SortedItems;		// used for visual 
	CUtlVector<int>					m_Separators;       // menu item ids after  which separators should be shown
	CUtlVector<MenuSeparator *>		m_SeparatorPanels;

	bool 			_sizedForScrollBar: 1 ;  // whether menu has been sized for a scrollbar
	bool			m_bUseFallbackFont : 1;
	bool 			_recalculateWidth : 1;
	bool			m_bUseMenuManager : 1;

	int 			_menuWide;
	int 			m_iCurrentlySelectedItemID;
	int 			m_iInputMode;
	int 			m_iCheckImageWidth; // the size of the check box spot on a checkable menu.
	int 			m_iProportionalScrollBarSize;
	Label::Alignment	m_Alignment;
	Color 			_borderDark;
	int 			m_iActivatedItem;
	HFont			m_hItemFont;
	HFont			m_hFallbackItemFont;

	// for managing type ahead
	#define			TYPEAHEAD_BUFSIZE 256
	MenuTypeAheadMode m_eTypeAheadMode;
	wchar_t			m_szTypeAheadBuf[TYPEAHEAD_BUFSIZE];
	int				m_iNumTypeAheadChars;
	double			m_fLastTypeAheadTime;
};


//-----------------------------------------------------------------------------
// Helper class to create menu
//-----------------------------------------------------------------------------
class MenuBuilder
{
public:

	MenuBuilder( Menu *pMenu, Panel *pActionTarget );

	MenuItem* AddMenuItem( const char *pszButtonText, const char *pszCommand, const char *pszCategoryName );

	MenuItem* AddCascadingMenuItem( const char *pszButtonText, Menu *pSubMenu, const char *pszCategoryName );

private:

	void AddSepratorIfNeeded( const char *pszCategoryName );

	Menu *m_pMenu;
	Panel *m_pActionTarget;
	const char *m_pszLastCategory;
};

} // namespace vgui

#endif // MENU_H
