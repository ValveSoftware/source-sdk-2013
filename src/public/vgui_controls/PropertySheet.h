//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui/VGUI.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/PHandle.h"
#include "utlvector.h"

namespace vgui
{

class PageTab;
class ImagePanel;

//-----------------------------------------------------------------------------
// Purpose: Tabbed property sheet.  Holds and displays a set of Panel's
//-----------------------------------------------------------------------------
class PropertySheet : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( PropertySheet, EditablePanel );

public:
	PropertySheet(Panel *parent, const char *panelName, bool draggableTabs = false );
	PropertySheet(Panel *parent, const char *panelName,ComboBox *combo);
	~PropertySheet();

	virtual bool IsDraggableTab() const;
	void		SetDraggableTabs( bool state );

	// Adds a page to the sheet.  The first page added becomes the active sheet.
	virtual void AddPage(Panel *page, const char *title, char const *imageName = NULL, bool showContextMenu = false );

	// sets the current page
	virtual void SetActivePage(Panel *page);

	// sets the width, in pixels, of the page tab buttons.
	virtual void SetTabWidth(int pixels);

	// Gets a pointer to the currently active page.
	virtual Panel *GetActivePage();

	// Removes (but doesn't delete) all pages
	virtual void	RemoveAllPages();

	// Removes all the pages and marks all the pages for deletion.
	virtual void	DeleteAllPages();

	// reloads the data in all the property page
	virtual void ResetAllData();

	// writes out any changed data to the doc
	virtual void ApplyChanges();

	// focus handling - passed on to current active page
	virtual void RequestFocus(int direction = 0);
	virtual bool RequestFocusPrev(VPANEL panel = NULL);
	virtual bool RequestFocusNext(VPANEL panel = NULL);

	// returns the ith panel 
	virtual Panel *GetPage(int i);

	// deletes this panel from the sheet
	virtual void DeletePage(Panel *panel);
	// removes this panel from the sheet, sets its parent to NULL, but does not delete it
	virtual void RemovePage(Panel *panel);

	// returns the current activated tab
	virtual Panel *GetActiveTab();

	// returns the title text of the tab
	virtual void GetActiveTabTitle( char *textOut, int bufferLen );

	// returns the title of tab "i"
	virtual bool GetTabTitle( int i, char *textOut, int bufferLen );

	// sets the title of tab "i"
	virtual bool SetTabTitle( int i, char *pchTitle );

	// returns the index of the active page
	virtual int GetActivePageNum();

	// returns the number of pages in the sheet
	virtual int GetNumPages();

	// disable the page with title "title" 
	virtual void DisablePage(const char *title);

	// enable the page with title "title" 
	virtual void EnablePage(const char *title);

	virtual void SetSmallTabs( bool state );
	virtual bool IsSmallTabs() const;

	/* MESSAGES SENT TO PAGES
		"PageShow"	- sent when a page is shown
		"PageHide"	- sent when a page is hidden
		"ResetData"	- sent when the data should be reloaded from doc
		"ApplyChanges" - sent when data should be written to doc
	*/

	virtual void OnPanelDropped( CUtlVector< KeyValues * >& msglist );
	virtual bool IsDroppable( CUtlVector< KeyValues * >& msglist );
	// Mouse is now over a droppable panel
	virtual void OnDroppablePanelPaint( CUtlVector< KeyValues * >& msglist, CUtlVector< Panel * >& dragPanels );
	
	void		ShowContextButtons( bool state );
	bool		ShouldShowContextButtons() const;

	int			FindPage( Panel *page ) const;

	bool		PageHasContextMenu( Panel *page ) const;

	void		SetKBNavigationEnabled( bool state );
	bool		IsKBNavigationEnabled() const;

	virtual bool HasUserConfigSettings() { return true; }

	void SetTabFont( vgui::HFont hFont );

protected:
	virtual void PaintBorder();
	virtual void PerformLayout();
	virtual Panel *HasHotkey(wchar_t key);
	virtual void ChangeActiveTab(int index);
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnCommand(const char *command);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void ApplySettings(KeyValues *inResourceData);

	// internal message handlers
	MESSAGE_FUNC_PTR( OnTabPressed, "TabPressed", panel );
	MESSAGE_FUNC_PTR_WCHARPTR( OnTextChanged, "TextChanged", panel, text );
	MESSAGE_FUNC_PARAMS( OnOpenContextMenu, "OpenContextMenu", params );
	MESSAGE_FUNC( OnApplyButtonEnable, "ApplyButtonEnable" );
	// called when default button has been set
	MESSAGE_FUNC_HANDLE( OnDefaultButtonSet, "DefaultButtonSet", button );
	// called when the current default button has been set
	MESSAGE_FUNC_HANDLE( OnCurrentDefaultButtonSet, "CurrentDefaultButtonSet", button);
    MESSAGE_FUNC( OnFindDefaultButton, "FindDefaultButton" );

private:
	
	// enable/disable the page with title "title" 
	virtual void SetPageEnabled(const char *title,bool state);

	struct Page_t
	{
		Page_t() :
			page( 0 ),
			contextMenu( false )
		{
		}

		Panel	*page;
		bool	contextMenu;
	};

	CUtlVector<Page_t> m_Pages;
	CUtlVector<PageTab *> m_PageTabs;
	Panel *_activePage;
	PageTab *_activeTab;
	int _tabWidth;
	int _activeTabIndex;
	ComboBox *_combo;
	bool _showTabs;
    bool _tabFocus;

	PHandle m_hPreviouslyActivePage;
	float m_flPageTransitionEffectTime;
	bool	m_bSmallTabs;
	HFont	m_tabFont;
	bool	m_bOverrideTabFont = false;
	bool	m_bDraggableTabs;
	bool	m_bContextButton;
	bool	m_bKBNavigationEnabled;

	CPanelAnimationVarAliasType( int, m_iPageYOffset, "yoffset", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iTabXIndent, "tabxindent", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTabXDelta, "tabxdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( bool, m_bTabFitText, "tabxfittotext", "1", "bool" );

	//=============================================================================
	// HPE_BEGIN:
	// [tj] These variables have been split into the initially specified size
	//		and the currently set size. This is so we can always recalculate the
	//		new value for resolution changes.
	//=============================================================================
	CPanelAnimationVarAliasType( int, m_iSpecifiedTabHeight, "tabheight", "28", "int" );
	CPanelAnimationVarAliasType( int, m_iSpecifiedTabHeightSmall, "tabheight_small", "14", "int" );

	int m_iTabHeight;
	int m_iTabHeightSmall;
	//=============================================================================
	// HPE_END
	//=============================================================================

	KeyValues	*m_pTabKV;
};

}; // namespace vgui

#endif // PROPERTYSHEET_H
