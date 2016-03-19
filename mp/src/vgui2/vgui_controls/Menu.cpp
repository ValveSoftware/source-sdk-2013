//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vgui_controls/pch_vgui_controls.h"

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"
#define MENU_SEPARATOR_HEIGHT 3

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: divider line in a menu
//-----------------------------------------------------------------------------
class vgui::MenuSeparator : public Panel
{
public:
	DECLARE_CLASS_SIMPLE( MenuSeparator, Panel );

	MenuSeparator( Panel *parent, char const *panelName ) :
		BaseClass( parent, panelName )
	{
		SetPaintEnabled( true );
		SetPaintBackgroundEnabled( true );
		SetPaintBorderEnabled( false );
	}

	virtual void Paint()
	{
		int w, h;
		GetSize( w, h );

		surface()->DrawSetColor( GetFgColor() );
		surface()->DrawFilledRect( 4, 1, w-1, 2 );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		SetFgColor( pScheme->GetColor( "Menu.SeparatorColor", Color( 142, 142, 142, 255 ) ) );
		SetBgColor( pScheme->GetColor( "Menu.BgColor", Color( 0, 0, 0, 255 ) ) );
	}
};

DECLARE_BUILD_FACTORY( Menu );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Menu::Menu(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	m_Alignment = Label::a_west;
	m_iFixedWidth = 0;
	m_iMinimumWidth = 0;
	m_iNumVisibleLines = -1; // No limit
	m_iCurrentlySelectedItemID = m_MenuItems.InvalidIndex();
	m_pScroller = new ScrollBar(this, "MenuScrollBar", true);
	m_pScroller->SetVisible(false);
	m_pScroller->AddActionSignalTarget(this);
	_sizedForScrollBar = false;
	SetZPos(1);
	SetVisible(false);
	MakePopup(false);
	SetParent(parent);
	_recalculateWidth = true;
	m_bUseMenuManager = true;
	m_iInputMode = MOUSE;
	m_iCheckImageWidth = 0;
	m_iActivatedItem = 0;

	m_bUseFallbackFont = false;
	m_hFallbackItemFont = INVALID_FONT;

	if (IsProportional())
	{
		m_iMenuItemHeight =  scheme()->GetProportionalScaledValueEx( GetScheme(), DEFAULT_MENU_ITEM_HEIGHT );
	}
	else
	{
		m_iMenuItemHeight =  DEFAULT_MENU_ITEM_HEIGHT;
	}
	m_hItemFont = INVALID_FONT;


	m_eTypeAheadMode = COMPAT_MODE;
	m_szTypeAheadBuf[0] = '\0';
	m_iNumTypeAheadChars = 0;
	m_fLastTypeAheadTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Menu::~Menu()
{
	delete m_pScroller;
}

//-----------------------------------------------------------------------------
// Purpose: Remove all menu items from the menu. 
//-----------------------------------------------------------------------------
void Menu::DeleteAllItems()
{
	FOR_EACH_LL( m_MenuItems, i )
	{
		m_MenuItems[i]->MarkForDeletion();
	}
	
	m_MenuItems.RemoveAll();
	m_SortedItems.RemoveAll();
	m_VisibleSortedItems.RemoveAll();
	m_Separators.RemoveAll();
	int c = m_SeparatorPanels.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		m_SeparatorPanels[ i ]->MarkForDeletion();
	}
	m_SeparatorPanels.RemoveAll();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu.
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( MenuItem *panel )
{
	panel->SetParent( this );
	MEM_ALLOC_CREDIT();
	int itemID = m_MenuItems.AddToTail( panel );
	m_SortedItems.AddToTail(itemID);
	InvalidateLayout(false);
	_recalculateWidth = true;
	panel->SetContentAlignment( m_Alignment );
	if ( INVALID_FONT != m_hItemFont )
	{
		panel->SetFont( m_hItemFont );
	}
	if ( m_bUseFallbackFont && INVALID_FONT != m_hFallbackItemFont )
	{
		Label *l = panel;
		TextImage *ti = l->GetTextImage();
		if ( ti )
		{
			ti->SetUseFallbackFont( m_bUseFallbackFont, m_hFallbackItemFont );
		}
	}

	if ( panel->GetHotKey() )
	{
		SetTypeAheadMode( HOT_KEY_MODE );
	}

	return itemID;
}


//-----------------------------------------------------------------------------
// Remove a single item
//-----------------------------------------------------------------------------
void Menu::DeleteItem( int itemID )
{
	// FIXME: This doesn't work with separator panels yet
	Assert( m_SeparatorPanels.Count() == 0 );

	m_MenuItems[itemID]->MarkForDeletion();
	m_MenuItems.Remove( itemID );

	m_SortedItems.FindAndRemove( itemID );
	m_VisibleSortedItems.FindAndRemove( itemID );

	InvalidateLayout(false);
	_recalculateWidth = true;
}


//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu.
// Input  : *item - MenuItem 
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
//			*userData - any user data associated with this menu item
// Output:  itemID - ID of this item
//-----------------------------------------------------------------------------
int Menu::AddMenuItemCharCommand(MenuItem *item, const char *command, Panel *target, const KeyValues *userData)
{
	item->SetCommand(command);
	item->AddActionSignalTarget( target );
	item->SetUserData(userData);
	return AddMenuItem( item );
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu. 
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
// Output:  itemID - ID of this item
//-----------------------------------------------------------------------------
int Menu::AddMenuItemKeyValuesCommand( MenuItem *item, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	item->SetCommand(message);
	item->AddActionSignalTarget(target);
	item->SetUserData(userData);
	return AddMenuItem(item);
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu.
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
// Output:  itemID - ID of this item
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, itemText );
	return AddMenuItemCharCommand(item, command, target, userData);
}

int Menu::AddMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, wszItemText );
	return AddMenuItemCharCommand(item, command, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
// Output:  itemID - ID of this item
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( const char *itemText, const char *command, Panel *target, const KeyValues *userData  )
{
	return AddMenuItem(itemText, itemText, command, target, userData ) ;
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu. 
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, itemText );
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

int Menu::AddMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, wszItemText );
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	return AddMenuItem(itemText, itemText, message, target, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be the text of the command sent when the
//						item is selected.
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddMenuItem( const char *itemText, Panel *target , const KeyValues *userData )
{
	return AddMenuItem(itemText, itemText, target, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a checkable menu item to the menu.
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
//-----------------------------------------------------------------------------
int Menu::AddCheckableMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, const KeyValues *userData )
{
	MenuItem *item = new MenuItem(this, itemName, itemText, NULL, true);
	return AddMenuItemCharCommand(item, command, target, userData);
}

int Menu::AddCheckableMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, const KeyValues *userData )
{
	MenuItem *item = new MenuItem(this, itemName, wszItemText, NULL, true);
	return AddMenuItemCharCommand(item, command, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a checkable menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCheckableMenuItem( const char *itemText, const char *command, Panel *target, const KeyValues *userData  )
{
	return AddCheckableMenuItem(itemText, itemText, command, target, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a checkable menu item to the menu. 
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCheckableMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, itemText, NULL, true);
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

int Menu::AddCheckableMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	MenuItem *item = new MenuItem(this, itemName, wszItemText, NULL, true);
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a checkable menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCheckableMenuItem( const char *itemText, KeyValues *message, Panel *target, const KeyValues *userData  )
{
	return AddCheckableMenuItem(itemText, itemText, message, target, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a checkable menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be the text of the command sent when the
//						item is selected.
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCheckableMenuItem( const char *itemText, Panel *target, const KeyValues *userData  )
{
	return AddCheckableMenuItem(itemText, itemText, target, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a Cascading menu item to the menu.
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCascadingMenuItem( const char *itemName, const char *itemText, const char *command, Panel *target, Menu *cascadeMenu , const KeyValues *userData )
{
	MenuItem *item = new MenuItem(this, itemName, itemText, cascadeMenu );
	return AddMenuItemCharCommand(item, command, target, userData);
}

int Menu::AddCascadingMenuItem( const char *itemName, const wchar_t *wszItemText, const char *command, Panel *target, Menu *cascadeMenu , const KeyValues *userData )
{
	MenuItem *item = new MenuItem(this, itemName, wszItemText, cascadeMenu );
	return AddMenuItemCharCommand(item, command, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a Cascading menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*command -  Command text to be sent when menu item is selected	
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCascadingMenuItem( const char *itemText, const char *command, Panel *target, Menu *cascadeMenu , const KeyValues *userData )
{
	return AddCascadingMenuItem( itemText, itemText, command, target, cascadeMenu, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a Cascading menu item to the menu. 
// Input  : *itemName - Name of item 
//			*itemText - Name of item text that will appear in the manu.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCascadingMenuItem( const char *itemName, const char *itemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData )
{
	MenuItem *item = new MenuItem( this, itemName, itemText, cascadeMenu);
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

int Menu::AddCascadingMenuItem( const char *itemName, const wchar_t *wszItemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData )
{
	MenuItem *item = new MenuItem( this, itemName, wszItemText, cascadeMenu);
	return AddMenuItemKeyValuesCommand(item, message, target, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Add a Cascading menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be used as the name of the menu item panel.
//			*message - pointer to the message to send when the item is selected
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCascadingMenuItem( const char *itemText, KeyValues *message, Panel *target, Menu *cascadeMenu, const KeyValues *userData )
{
	return AddCascadingMenuItem(itemText, itemText, message, target, cascadeMenu, userData );
}

//-----------------------------------------------------------------------------
// Purpose: Add a Cascading menu item to the menu. 
// Input  : *itemText - Name of item text that will appear in the manu.
//						This will also be the text of the command sent when the
//						item is selected.
//			*target - Target panel of the command
//          *cascadeMenu - if the menu item opens a cascading menu, this is a 
//							ptr to the menu that opens on selecting the item
//-----------------------------------------------------------------------------
int Menu::AddCascadingMenuItem( const char *itemText, Panel *target, Menu *cascadeMenu, const KeyValues *userData )
{
	return AddCascadingMenuItem(itemText, itemText, target, cascadeMenu, userData);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the values of a menu item at the specified index
// Input  : index - the index of this item entry
//			*message - pointer to the message to send when the item is selected
//-----------------------------------------------------------------------------
void Menu::UpdateMenuItem(int itemID, const char *itemText, KeyValues *message, const KeyValues *userData)
{
	Assert( m_MenuItems.IsValidIndex(itemID) );
	if ( m_MenuItems.IsValidIndex(itemID) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		// make sure its enabled since disabled items get highlighted.
		if (menuItem)
		{
			menuItem->SetText(itemText);
			menuItem->SetCommand(message);
			if(userData)
			{
				menuItem->SetUserData(userData);
			}
		}
	}
	_recalculateWidth = true;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the values of a menu item at the specified index
//-----------------------------------------------------------------------------
void Menu::UpdateMenuItem(int itemID, const wchar_t *wszItemText, KeyValues *message, const KeyValues *userData)
{
	Assert( m_MenuItems.IsValidIndex(itemID) );
	if ( m_MenuItems.IsValidIndex(itemID) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		// make sure its enabled since disabled items get highlighted.
		if (menuItem)
		{
			menuItem->SetText(wszItemText);
			menuItem->SetCommand(message);
			if(userData)
			{
				menuItem->SetUserData(userData);
			}
		}
	}
	_recalculateWidth = true;
}


//-----------------------------------------------------------------------------
// Sets the content alignment of all items in the menu
//-----------------------------------------------------------------------------
void Menu::SetContentAlignment( Label::Alignment alignment )
{
	if ( m_Alignment != alignment )
	{
		m_Alignment = alignment;

		// Change the alignment of existing menu items
		int nCount = m_MenuItems.Count();
		for ( int i = 0; i < nCount; ++i )
		{
			m_MenuItems[i]->SetContentAlignment( alignment );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Locks down a specific width
//-----------------------------------------------------------------------------
void Menu::SetFixedWidth(int width)
{
	// the padding makes it so the menu has the label padding on each side of the menu.
	// makes the menu items look centered.
	m_iFixedWidth = width;
	InvalidateLayout(false);
}

//-----------------------------------------------------------------------------
// Purpose: sets the height of each menu item
//-----------------------------------------------------------------------------
void Menu::SetMenuItemHeight(int itemHeight)
{
	m_iMenuItemHeight = itemHeight;
}

int  Menu::GetMenuItemHeight() const
{
	return m_iMenuItemHeight;
}

int Menu::CountVisibleItems()
{
	int count = 0;
	int c = m_SortedItems.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		if ( m_MenuItems[ m_SortedItems[ i ] ]->IsVisible() )
			++count;
	}
	return count;
}

void Menu::ComputeWorkspaceSize( int& workWide, int& workTall )
{
	// make sure we factor in insets
	int ileft, iright, itop, ibottom;
	GetInset(ileft, iright, itop, ibottom);

	int workX, workY;
	surface()->GetWorkspaceBounds(workX, workY, workWide, workTall);
	workTall -= 20;
	workTall -= itop;
	workTall -= ibottom;
}

// Assumes relative coords in screenspace
void Menu::PositionRelativeToPanel( Panel *relative, MenuDirection_e direction, int nAdditionalYOffset /*=0*/, bool showMenu /*=false*/ )
{
	Assert( relative );
	int rx, ry, rw, rh;
	relative->GetBounds( rx, ry, rw, rh );
	relative->LocalToScreen( rx, ry );

	if ( direction == CURSOR )
	{
		// force the menu to appear where the mouse button was pressed
		input()->GetCursorPos(rx, ry);
		rw = rh = 0;
	}
	else if ( direction == ALIGN_WITH_PARENT && relative->GetVParent() )
	{
	   rx = 0, ry = 0;
	   relative->ParentLocalToScreen(rx, ry);
	   rx -= 1; // take border into account
	   ry += rh + nAdditionalYOffset;
	   rw = rh = 0;
	}
	else
	{
		rx = 0, ry = 0;
		relative->LocalToScreen(rx, ry);
	}

	int workWide, workTall;
	ComputeWorkspaceSize( workWide, workTall );

	// Final pos
	int x = 0, y = 0;

	int mWide, mTall;
	GetSize( mWide, mTall );

	switch( direction )
	{
	case Menu::UP: // Menu prefers to open upward
		{
			x = rx;
			int topOfReference = ry;
			y = topOfReference - mTall;
			if ( y < 0 )
			{
				int bottomOfReference = ry + rh + 1;
				int remainingPixels = workTall - bottomOfReference;

				// Can't fit on bottom, either, move to side
				if ( mTall >= remainingPixels )
				{
					y = workTall - mTall;
					x = rx + rw;
					// Try and place it to the left of the button
					if ( x + mWide > workWide )
					{
						x = rx - mWide;
					}
				}
				else
				{
					// Room at bottom
					y = bottomOfReference;
				}
			}
		}
		break;
		// Everyone else aligns downward...
	default:
	case Menu::LEFT:
	case Menu::RIGHT:
	case Menu::DOWN:
		{
			x = rx;
			int bottomOfReference = ry + rh + 1;
			y = bottomOfReference;
			if ( bottomOfReference + mTall >= workTall )
			{
				// See if there's run straight above
				if ( mTall >= ry ) // No room, try and push menu to right or left
				{
					y = workTall - mTall;
					x = rx + rw;
					// Try and place it to the left of the button
					if ( x + mWide > workWide )
					{
						x = rx - mWide;
					}
				}
				else
				{
					// Room at top
					y = ry - mTall;
				}
			}
		}
		break;
	}
	
	// Check left rightness
	if ( x + mWide > workWide )
	{
		x = workWide - mWide;
		Assert( x >= 0 ); // yikes!!!
	}
	else if ( x < 0 )
	{
		x = 0;
	}

	SetPos( x, y );
	if ( showMenu )
	{
		SetVisible( true );
	}
}

int Menu::ComputeFullMenuHeightWithInsets()
{
	// make sure we factor in insets
	int ileft, iright, itop, ibottom;
	GetInset(ileft, iright, itop, ibottom);

	int separatorHeight = 3;

	// add up the size of all the child panels
	// move the child panels to the correct place in the menu
	int totalTall = itop + ibottom;
	int i;
	for ( i = 0 ; i < m_SortedItems.Count() ; i++ )		// use sortedItems instead of MenuItems due to SetPos()
	{
		int itemId = m_SortedItems[i];

		MenuItem *child = m_MenuItems[ itemId ];
		Assert( child );
		if ( !child )
			continue;
		// These should all be visible at this point
		if ( !child->IsVisible() )
			continue;

		totalTall += m_iMenuItemHeight;

		// Add a separator if needed...
		int sepIndex = m_Separators.Find( itemId );
		if ( sepIndex != m_Separators.InvalidIndex() )
		{
			totalTall += separatorHeight;
		}
	}

	return totalTall;
}

//-----------------------------------------------------------------------------
// Purpose: Reformat according to the new layout
//-----------------------------------------------------------------------------
void Menu::PerformLayout()
{
	MenuItem *parent = GetParentMenuItem();
	bool cascading =  parent != NULL ? true : false;

	// make sure we factor in insets
	int ileft, iright, itop, ibottom;
	GetInset(ileft, iright, itop, ibottom);

	int workWide, workTall;

	ComputeWorkspaceSize( workWide, workTall );

	int fullHeightWouldRequire = ComputeFullMenuHeightWithInsets();

	bool bNeedScrollbar = fullHeightWouldRequire >= workTall; 

	int maxVisibleItems = CountVisibleItems();

	if ( m_iNumVisibleLines > 0 && 
		 maxVisibleItems > m_iNumVisibleLines )
	{
		bNeedScrollbar = true;
		maxVisibleItems = m_iNumVisibleLines;
	}

	// if we have a scroll bar
	if ( bNeedScrollbar )
	{
		// add it to the display
		AddScrollBar();

		// This fills in m_VisibleSortedItems as needed
		MakeItemsVisibleInScrollRange( m_iNumVisibleLines, min( fullHeightWouldRequire, workTall ) );
	}
	else
	{
		RemoveScrollBar();
		// Make everything visible
		m_VisibleSortedItems.RemoveAll();
		int i;
		int c = m_SortedItems.Count();
		for ( i = 0; i < c; ++i )
		{
			int itemID = m_SortedItems[ i ];
			MenuItem *child = m_MenuItems[ itemID ];
			if ( !child || !child->IsVisible() )
				continue;

			m_VisibleSortedItems.AddToTail( itemID );
		}

		// Hide the separators, the needed ones will be readded below
		c = m_SeparatorPanels.Count();
		for ( i = 0; i < c; ++i )
		{
			if ( m_SeparatorPanels[ i ] )
			{
				m_SeparatorPanels[ i ]->SetVisible( false );
			}
		}
	}
	
	// get the appropriate menu border
	LayoutMenuBorder();

	int trueW = GetWide();
	if ( bNeedScrollbar )
	{
		trueW -= m_pScroller->GetWide();
	}
	int separatorHeight = MENU_SEPARATOR_HEIGHT;

	// add up the size of all the child panels
	// move the child panels to the correct place in the menu
	int menuTall = 0;
	int totalTall = itop + ibottom;
	int i;
	for ( i = 0 ; i < m_VisibleSortedItems.Count() ; i++ )		// use sortedItems instead of MenuItems due to SetPos()
	{
		int itemId = m_VisibleSortedItems[i];

		MenuItem *child = m_MenuItems[ itemId ];
		Assert( child );
		if ( !child )
			continue;
		// These should all be visible at this point
		if ( !child->IsVisible() )
			continue;

		if ( totalTall >= workTall )
			break;

		if ( INVALID_FONT != m_hItemFont )
		{
			child->SetFont( m_hItemFont );
		}

		// take into account inset
		child->SetPos (0, menuTall);
		child->SetTall( m_iMenuItemHeight ); // Width is set in a second pass
		menuTall += m_iMenuItemHeight;
		totalTall += m_iMenuItemHeight;

		// this will make all the menuitems line up in a column with space for the checks to the left.
		if ( ( !child->IsCheckable() ) && ( m_iCheckImageWidth > 0 ) )
		{
			// Non checkable items have to move over
			child->SetTextInset( m_iCheckImageWidth, 0 );
		}
		else if ( child->IsCheckable() )
		{
			child->SetTextInset(0, 0); //TODO: for some reason I can't comment this out.
		}

		// Add a separator if needed...
		int sepIndex = m_Separators.Find( itemId );
		if ( sepIndex != m_Separators.InvalidIndex() )
		{
			MenuSeparator *sep = m_SeparatorPanels[ sepIndex ];
			Assert( sep );
			sep->SetVisible( true );
			sep->SetBounds( 0, menuTall, trueW, separatorHeight );
			menuTall += separatorHeight;
			totalTall += separatorHeight;
		}
	}
	
	if (!m_iFixedWidth)
	{
		_recalculateWidth = true;
		CalculateWidth();
	}
	else if (m_iFixedWidth)
	{
		_menuWide = m_iFixedWidth;
		// fixed width menus include the scroll bar in their width.
		if (_sizedForScrollBar)
		{
			_menuWide -= m_pScroller->GetWide();
		}
	}
	
	SizeMenuItems();
	
	int extraWidth = 0;
	if (_sizedForScrollBar)
	{
		extraWidth = m_pScroller->GetWide();
	}

	int mwide = _menuWide + extraWidth;
	if ( mwide > workWide )
	{
		mwide = workWide;
	}
	int mtall = menuTall + itop + ibottom;
	if ( mtall > workTall )
	{
		// Shouldn't happen
		mtall = workTall;
	}

	// set the new size of the menu
	SetSize( mwide, mtall );
	
	// move the menu to the correct position if it is a cascading menu.
	if ( cascading )
	{
		// move the menu to the correct position if it is a cascading menu.
		PositionCascadingMenu();
	}
		
	// set up scroll bar as appropriate
	if ( m_pScroller->IsVisible() )
	{
		LayoutScrollBar();
	}
	
	FOR_EACH_LL( m_MenuItems, j )
	{
		m_MenuItems[j]->InvalidateLayout(); // cause each menu item to redo its apply settings now we have sized ourselves
	}

	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: Force the menu to work out how wide it should be
//-----------------------------------------------------------------------------
void Menu::ForceCalculateWidth()
{
	_recalculateWidth = true;
	CalculateWidth();
	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Figure out how wide the menu should be if the menu is not fixed width
//-----------------------------------------------------------------------------
void Menu::CalculateWidth()
{
	if (!_recalculateWidth)
		return;

	_menuWide = 0;
	if (!m_iFixedWidth)
	{
		// find the biggest menu item
		FOR_EACH_LL( m_MenuItems, i )
		{		
			int wide, tall;
			m_MenuItems[i]->GetContentSize(wide, tall);
			if (wide > _menuWide - Label::Content)
			{
				_menuWide =  wide + Label::Content;	
			}
		}	
	}
	
	// enfoce a minimumWidth 
	if (_menuWide < m_iMinimumWidth)
	{		
		_menuWide = m_iMinimumWidth;
	}

	_recalculateWidth = false;
}

//-----------------------------------------------------------------------------
// Purpose: Set up the scroll bar attributes,size and location.
//-----------------------------------------------------------------------------
void Menu::LayoutScrollBar()
{
	//!! need to make it recalculate scroll positions
	m_pScroller->SetEnabled(false);
	m_pScroller->SetRangeWindow( m_VisibleSortedItems.Count() );
	m_pScroller->SetRange( 0, CountVisibleItems() );	
	m_pScroller->SetButtonPressedScrollValue( 1 );
	
	int wide, tall;
	GetSize (wide, tall);

	// make sure we factor in insets
	int ileft, iright, itop, ibottom;
	GetInset(ileft, iright, itop, ibottom);

	// with a scroll bar we take off the inset
	wide -= iright;

	m_pScroller->SetPos(wide - m_pScroller->GetWide(), 1);
	
	// scrollbar is inside the menu's borders.
	m_pScroller->SetSize(m_pScroller->GetWide(), tall - ibottom - itop);

}

//-----------------------------------------------------------------------------
// Purpose: Figure out where to open menu if it is a cascading menu
//-----------------------------------------------------------------------------
void Menu::PositionCascadingMenu()
{
	Assert(GetVParent());
	int parentX, parentY, parentWide, parentTall;
	// move the menu to the correct place below the menuItem
	ipanel()->GetSize(GetVParent(), parentWide, parentTall);
	ipanel()->GetPos(GetVParent(), parentX, parentY);
	
	parentX += parentWide, parentY = 0;

	ParentLocalToScreen(parentX, parentY);

	SetPos(parentX, parentY);
	
	// for cascading menus, 
	// make sure we're on the screen
	int workX, workY, workWide, workTall, x, y, wide, tall;
	GetBounds(x, y, wide, tall);
	surface()->GetWorkspaceBounds(workX, workY, workWide, workTall);
	
	if (x + wide > workX + workWide)
	{
		// we're off the right, move the menu to the left side
		// orignalX - width of the parentmenuitem - width of this menu.
		// add 2 pixels to offset one pixel onto the parent menu.
		x -= (parentWide + wide);
		x -= 2;
	}
	else
	{
		// alignment move it in the amount of the insets.
		x += 1;
	}

	if ( y + tall > workY + workTall )
	{
		int lastWorkY = workY + workTall;
		int pixelsOffBottom = ( y + tall ) - lastWorkY;

		y -= pixelsOffBottom;
		y -= 2;
	}
	else
	{
		y -= 1;
	}
	SetPos(x, y);
	
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: Size the menu items so they are the width of the menu.
//			Also size the menu items with cascading menus so the arrow fits in there.
//-----------------------------------------------------------------------------
void Menu::SizeMenuItems()
{
	int ileft, iright, itop, ibottom;
	GetInset(ileft, iright, itop, ibottom);
	
	// assign the sizes of all the menu item panels
	FOR_EACH_LL( m_MenuItems, i )
	{
		MenuItem *child = m_MenuItems[i];
		if (child )
		{
			// labels do thier own sizing. this will size the label to the width of the menu,
			// this will put the cascading menu arrow on the right side automatically.	
			child->SetWide(_menuWide - ileft - iright);			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes menu items visible in relation to where the scroll bar is
//-----------------------------------------------------------------------------
void Menu::MakeItemsVisibleInScrollRange( int maxVisibleItems, int nNumPixelsAvailable )
{
	// Detach all items from tree
	int i;
	FOR_EACH_LL( m_MenuItems, item )
	{
		m_MenuItems[ item ]->SetBounds( 0, 0, 0, 0 );
	}
	for ( i = 0; i < m_SeparatorPanels.Count(); ++i )
	{
		m_SeparatorPanels[ i ]->SetVisible( false );
	}

	m_VisibleSortedItems.RemoveAll();

	int tall = 0;

	int startItem = m_pScroller->GetValue();
	Assert( startItem >= 0 );
	do
	{
		if ( startItem >= m_SortedItems.Count() )
			break;

		int itemId = m_SortedItems[ startItem ];

		if ( !m_MenuItems[ itemId ]->IsVisible() )
		{
			++startItem;
			continue;
		}

		int itemHeight = m_iMenuItemHeight;
		int sepIndex = m_Separators.Find( itemId );
		if ( sepIndex != m_Separators.InvalidIndex() )
		{
			itemHeight += MENU_SEPARATOR_HEIGHT;
		}

		if ( tall + itemHeight > nNumPixelsAvailable )
			break;

		// Too many items
		if ( maxVisibleItems > 0 )
		{
			if ( m_VisibleSortedItems.Count() >= maxVisibleItems )
				break;
		}

		tall += itemHeight;
		// Re-attach this one
		m_VisibleSortedItems.AddToTail( itemId );
		++startItem;
	}
	while ( true );
}

//-----------------------------------------------------------------------------
// Purpose: Get the approproate menu border
//-----------------------------------------------------------------------------
void Menu::LayoutMenuBorder()
{
	IBorder *menuBorder;
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	menuBorder = pScheme->GetBorder("MenuBorder");	   
	
	if ( menuBorder )
	{
		SetBorder(menuBorder);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw a black border on the right side of the menu items
//-----------------------------------------------------------------------------
void Menu::Paint()
{
	if ( m_pScroller->IsVisible() )
	{			
		// draw black bar
		int wide, tall;
		GetSize (wide, tall);
		surface()->DrawSetColor(_borderDark);
		if( IsProportional() )
		{
			surface()->DrawFilledRect(wide - m_pScroller->GetWide(), -1, wide - m_pScroller->GetWide() + 1, tall);	
		}
		else
		{
			surface()->DrawFilledRect(wide - m_pScroller->GetWide(), -1, wide - m_pScroller->GetWide() + 1, tall);	
		}
	}	
}


//-----------------------------------------------------------------------------
// Purpose:	sets the max number of items visible (scrollbar appears with more)
// Input  : numItems - 
//-----------------------------------------------------------------------------
void Menu::SetNumberOfVisibleItems( int numItems )
{
	m_iNumVisibleLines = numItems;
	InvalidateLayout(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Menu::EnableUseMenuManager( bool bUseMenuManager )
{
	m_bUseMenuManager = bUseMenuManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
MenuItem *Menu::GetMenuItem(int itemID)
{
	if ( !m_MenuItems.IsValidIndex(itemID) )
		return NULL;
	
	return m_MenuItems[itemID];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Menu::IsValidMenuID(int itemID)
{
	return m_MenuItems.IsValidIndex(itemID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Menu::GetInvalidMenuID()
{
	return m_MenuItems.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: When a menuItem is selected, close cascading menus
//          if the menuItem selected has a cascading menu attached, we
//          want to keep that one open so skip it.
//          Passing NULL will close all cascading menus.
//-----------------------------------------------------------------------------
void Menu::CloseOtherMenus(MenuItem *item)
{
	FOR_EACH_LL( m_MenuItems, i )
	{
		if (m_MenuItems[i] == item)
			continue;

		m_MenuItems[i]->CloseCascadeMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Respond to string commands.
//-----------------------------------------------------------------------------
void Menu::OnCommand( const char *command )
{
	// forward on the message
	PostActionSignal(new KeyValues("Command", "command", command));
	
	Panel::OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: Handle key presses, Activate shortcuts
//-----------------------------------------------------------------------------
void Menu::OnKeyCodeTyped(KeyCode keycode)
{
	vgui::KeyCode code = GetBaseButtonCode( keycode );

	// Don't allow key inputs when disabled!
	if ( !IsEnabled() )
		return;

	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
	if (alt)
	{
		BaseClass::OnKeyCodeTyped( keycode );
		// Ignore alt when in combobox mode
		if (m_eTypeAheadMode != TYPE_AHEAD_MODE)
		{
			PostActionSignal(new KeyValues("MenuClose"));
		}
	}

	switch (code)
	{
	case KEY_ESCAPE:
	case KEY_XBUTTON_B: 
		{
			// hide the menu on ESC
			SetVisible(false);
			break;
		}
		// arrow keys scroll through items on the list.
		// they should also scroll the scroll bar if needed
	case KEY_UP:
	case KEY_XBUTTON_UP: 
	case KEY_XSTICK1_UP: 
		{	
			MoveAlongMenuItemList(MENU_UP, 0);
			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();
			}
			else
			{
				BaseClass::OnKeyCodeTyped( keycode ); // chain up
			}
			break;
		}
	case KEY_DOWN:
	case KEY_XBUTTON_DOWN: 
	case KEY_XSTICK1_DOWN: 
		{
			MoveAlongMenuItemList(MENU_DOWN, 0);
			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();	
			}
			else
			{
				BaseClass::OnKeyCodeTyped( keycode ); // chain up
			}
			break;
		}
	// for now left and right arrows just open or close submenus if they are there.
	case KEY_RIGHT:
	case KEY_XBUTTON_RIGHT: 
	case KEY_XSTICK1_RIGHT: 
		{
			// make sure a menuItem is currently selected
			if ( m_MenuItems.IsValidIndex(m_iCurrentlySelectedItemID) )
			{
				if (m_MenuItems[m_iCurrentlySelectedItemID]->HasMenu())
				{
					ActivateItem(m_iCurrentlySelectedItemID);
				}
				else
				{
					BaseClass::OnKeyCodeTyped( keycode );
				}
			}
			else
			{
				BaseClass::OnKeyCodeTyped( keycode );
			}
			break;
		}
	case KEY_LEFT:
	case KEY_XBUTTON_LEFT: 
	case KEY_XSTICK1_LEFT: 
		{
			// if our parent is a menu item then we are a submenu so close us.
			if (GetParentMenuItem())
			{
				SetVisible(false);
			}
			else
			{
				BaseClass::OnKeyCodeTyped( keycode );
			}
			break;
		}
	case KEY_ENTER:
	case KEY_XBUTTON_A:
		{
			// make sure a menuItem is currently selected
			if ( m_MenuItems.IsValidIndex(m_iCurrentlySelectedItemID) )
			{
				ActivateItem(m_iCurrentlySelectedItemID);
			}
			else
			{
				BaseClass::OnKeyCodeTyped( keycode ); // chain up
			}
			break;
		}

	case KEY_PAGEUP:
		{
			if ( m_iNumVisibleLines > 1 )
			{
				if ( m_iCurrentlySelectedItemID < m_iNumVisibleLines )
				{
					MoveAlongMenuItemList( MENU_UP * m_iCurrentlySelectedItemID, 0 );
				}
				else
				{
					MoveAlongMenuItemList(MENU_UP * m_iNumVisibleLines - 1, 0);
				}
			}
			else
			{
				MoveAlongMenuItemList(MENU_UP, 0);
			}

			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();	
			}
			break;
		}


	case KEY_PAGEDOWN:
		{
			if ( m_iNumVisibleLines > 1 )
			{
				if ( m_iCurrentlySelectedItemID + m_iNumVisibleLines >= GetItemCount() )
				{
					MoveAlongMenuItemList(MENU_DOWN * ( GetItemCount() - m_iCurrentlySelectedItemID - 1), 0);
				}
				else
				{
					MoveAlongMenuItemList(MENU_DOWN * m_iNumVisibleLines - 1, 0);
				}
			}
			else
			{
				MoveAlongMenuItemList(MENU_DOWN, 0);
			}

			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();	
			}
			break;
		}

	case KEY_HOME:
		{
			MoveAlongMenuItemList( MENU_UP * m_iCurrentlySelectedItemID, 0 );
			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();	
			}
			break;
		}


	case KEY_END:
		{
			MoveAlongMenuItemList(MENU_DOWN * ( GetItemCount() - m_iCurrentlySelectedItemID - 1), 0);
			if ( m_MenuItems.IsValidIndex( m_iCurrentlySelectedItemID ) )
			{
				m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();	
			}
			break;
		}
	}
	
	// don't chain back
}

void Menu::OnHotKey(wchar_t unichar)
{
	// iterate the menu items looking for one with the matching hotkey
	FOR_EACH_LL( m_MenuItems, i )
	{
		MenuItem *panel = m_MenuItems[i];
		if (panel->IsVisible())
		{
			Panel *hot = panel->HasHotkey(unichar);
			if (hot)
			{
				// post a message to the menuitem telling it it's hotkey was pressed
				PostMessage(hot, new KeyValues("Hotkey"));
				return;
			}
			// if the menuitem is a cascading menuitem and it is open, check its hotkeys too
			Menu *cascadingMenu = panel->GetMenu();
			if (cascadingMenu && cascadingMenu->IsVisible())
			{
				cascadingMenu->OnKeyTyped(unichar);
			}
		}
	}
}

void Menu::OnTypeAhead(wchar_t unichar)
{
	// Don't do anything if the menu is empty since there cannot be a selected item.
	if ( m_MenuItems.Count() <= 0)
		return;

	// expire the type ahead buffer after 0.5 seconds
	double tCurrentTime = Sys_FloatTime();
	if ( (tCurrentTime - m_fLastTypeAheadTime) > 0.5f )
	{
		m_iNumTypeAheadChars = 0;
		m_szTypeAheadBuf[0] = '\0';
	}
	m_fLastTypeAheadTime = tCurrentTime;

	// add current character to the type ahead buffer
	if ( m_iNumTypeAheadChars+1 < TYPEAHEAD_BUFSIZE )
	{
		m_szTypeAheadBuf[m_iNumTypeAheadChars++] = unichar;
	}

	int itemToSelect = m_iCurrentlySelectedItemID;
	if ( itemToSelect < 0 || itemToSelect >= m_MenuItems.Count())
	{
		itemToSelect = 0;
	}

	int i = itemToSelect;
	do
	{
		wchar_t menuItemName[255];
		m_MenuItems[i]->GetText(menuItemName, 254);

		// This is supposed to be case insensitive but we don't have a portable case
		// insensitive wide-character routine.
		if ( wcsncmp( m_szTypeAheadBuf, menuItemName, m_iNumTypeAheadChars) == 0 )			
		{
			itemToSelect = i;
			break;			
		}

		i = (i+1) % m_MenuItems.Count();
	} while ( i != itemToSelect );

	if ( itemToSelect >= 0 )
	{
		SetCurrentlyHighlightedItem( itemToSelect );
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle key presses, Activate shortcuts
// Input  : code - 
//-----------------------------------------------------------------------------
void Menu::OnKeyTyped(wchar_t unichar)
{
	if (! unichar)
	{
		return;
	}

	switch( m_eTypeAheadMode )
	{
	case HOT_KEY_MODE:
		OnHotKey(unichar);
		return;		

	case TYPE_AHEAD_MODE:
		OnTypeAhead(unichar);
		return;		

	case COMPAT_MODE:
	default:
		break;
	}

	int itemToSelect = m_iCurrentlySelectedItemID;
	if ( itemToSelect < 0 )
	{
		itemToSelect = 0;
	}

	int i;
    wchar_t menuItemName[255];

	i = itemToSelect + 1;
	if ( i >= m_MenuItems.Count() )
	{
		i = 0;
	}

	while ( i != itemToSelect )
	{
		 m_MenuItems[i]->GetText(menuItemName, 254);

		if ( tolower( unichar ) == tolower( menuItemName[0] ) )
		{
			itemToSelect = i;
			break;			
		}

		i++;
		if ( i >= m_MenuItems.Count() )
		{
			i = 0;
		}
	}

	if ( itemToSelect >= 0 )
    {
		SetCurrentlyHighlightedItem( itemToSelect );
		InvalidateLayout();
	}

	// don't chain back
}


void Menu::SetTypeAheadMode(MenuTypeAheadMode mode)
{
	m_eTypeAheadMode = mode;
}

int Menu::GetTypeAheadMode()
{
	return m_eTypeAheadMode;
}

//-----------------------------------------------------------------------------
// Purpose: Handle the mouse wheel event, scroll the selection
//-----------------------------------------------------------------------------
void Menu::OnMouseWheeled(int delta)
{
	if (!m_pScroller->IsVisible())
		return;
	
	int val = m_pScroller->GetValue();
	val -= delta;
	
	m_pScroller->SetValue(val);	

	// moving the slider redraws the scrollbar,
	// and so we should redraw the menu since the
	// menu draws the black border to the right of the scrollbar.
	InvalidateLayout();

	// don't chain back
}


//-----------------------------------------------------------------------------
// Purpose: Lose focus, hide menu
//-----------------------------------------------------------------------------
void Menu::OnKillFocus()
{
	// check to see if it's a child taking it
	if (!input()->GetFocus() || !ipanel()->HasParent(input()->GetFocus(), GetVPanel()))
	{
		// if we don't accept keyboard input, then we have to ignore the killfocus if it's not actually being stolen
		if (!IsKeyBoardInputEnabled() && !input()->GetFocus())
			return;

		// get the parent of this menu. 
		MenuItem *item = GetParentMenuItem();
		// if the parent is a menu item, this menu is a cascading menu
		// if the panel that is getting focus is the parent menu, don't close this menu.
		if ( (item) && (input()->GetFocus() == item->GetVParent()) )
		{
			// if we are in mouse mode and we clicked on the menuitem that
			// triggers the cascading menu, leave it open.
			if (m_iInputMode == MOUSE)
			{
				// return the focus to the cascading menu.
				MoveToFront();
				return;
			}
		}

		// forward the message to the parent.
		PostActionSignal(new KeyValues("MenuClose"));

		// hide this menu
		SetVisible(false);
	}
	
}

namespace vgui
{

class CMenuManager
{
public:
	void AddMenu( Menu *m )
	{
		if ( !m )
			return;

		int c = m_Menus.Count();
		for ( int i = 0 ; i < c; ++i )
		{
			if ( m_Menus[ i ].Get() == m )
				return;
		}

		DHANDLE< Menu > h;
		h = m;
		m_Menus.AddToTail( h );
	}

	void RemoveMenu( Menu *m )
	{
		if ( !m )
			return;

		int c = m_Menus.Count();
		for ( int i = c - 1 ; i >= 0; --i )
		{
			if ( m_Menus[ i ].Get() == m )
			{
				m_Menus.Remove( i );
				return;
			}
		}
	}

	void OnInternalMousePressed( Panel *other, MouseCode code )
	{
		int c = m_Menus.Count();
		if ( !c )
			return;

		int x, y;
		input()->GetCursorPos( x, y );

		bool mouseInsideMenuRelatedPanel = false;

		for ( int i = c - 1; i >= 0 ; --i )
		{
			Menu *m = m_Menus[ i ].Get();
			if ( !m )
			{
				m_Menus.Remove( i );
				continue;
			}

			// See if the mouse is within a menu
			if ( IsWithinMenuOrRelative( m, x, y ) )
			{
				mouseInsideMenuRelatedPanel = true;
			}
		}

		if ( mouseInsideMenuRelatedPanel )
		{
			return;
		}

		AbortMenus();
	}

	void AbortMenus()
	{
		// Close all of the menus
		int c = m_Menus.Count();
		for ( int i = c - 1; i >= 0 ; --i )
		{
			Menu *m = m_Menus[ i ].Get();
			if ( !m )
			{
				continue;
			}

			m_Menus.Remove( i );

			// Force it to close
			m->SetVisible( false );
		}

		m_Menus.RemoveAll();
	}

	bool IsWithinMenuOrRelative( Panel *panel, int x, int y )
	{
		VPANEL topMost = panel->IsWithinTraverse( x, y, true );
		if ( topMost )
		{
			// It's over the menu
			if ( topMost == panel->GetVPanel() )
			{
				return true;
			}

			// It's over something which is parented to the menu (i.e., a menu item)
			if ( ipanel()->HasParent( topMost, panel->GetVPanel() ) )
			{
				return true;
			}
		}

		if ( panel->GetParent() )
		{
			Panel *parent = panel->GetParent();

			topMost = parent->IsWithinTraverse( x, y, true );

			if ( topMost )
			{
				if ( topMost == parent->GetVPanel() )
				{
					return true;
				}

				/*
				// NOTE:  this check used to not cast to MenuButton, but it seems wrong to me
				//  since if the mouse is over another child of the parent panel to the menu then
				//  the menu stays visible.  I think this is bogus.
				Panel *pTopMost = ipanel()->GetPanel(topMost, GetControlsModuleName());

				if ( pTopMost &&
					ipanel()->HasParent( topMost, parent->GetVPanel() ) &&
					dynamic_cast< MenuButton * >( pTopMost ) )
				{
					Msg( "topMost %s has parent %s\n", 
						ipanel()->GetName( topMost ),
						parent->GetName() );

					return true;
				}
				*/
			}
		}

		return false;
	}

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, char *pchName )
	{
		validator.Push( "CMenuManager", this, pchName );
		m_Menus.Validate( validator, "m_Menus" );
		validator.Pop();
	}
#endif

private:

	// List of visible menus
	CUtlVector< DHANDLE< Menu > >	m_Menus;
};


// Singleton helper class
static CMenuManager g_MenuMgr;

void ValidateMenuGlobals( CValidator &validator )
{
#ifdef DBGFLAG_VALIDATE
	g_MenuMgr.Validate( validator, "g_MenuMgr" );
#endif
}

}  // end namespace vgui

//-----------------------------------------------------------------------------
// Purpose: Static method called on mouse released to see if Menu objects should be aborted
// Input  : *other - 
//			code - 
//-----------------------------------------------------------------------------
void Menu::OnInternalMousePressed( Panel *other, MouseCode code )
{
	g_MenuMgr.OnInternalMousePressed( other, code );
}

//-----------------------------------------------------------------------------
// Purpose: Set visibility of menu and its children as appropriate. 
//-----------------------------------------------------------------------------
void Menu::SetVisible(bool state)
{
	if (state == IsVisible())
		return;

	if ( state == false )
	{
		PostActionSignal(new KeyValues("MenuClose"));
		CloseOtherMenus(NULL);

		SetCurrentlySelectedItem(-1);

		g_MenuMgr.RemoveMenu( this );
	}
	else if ( state == true )
	{
		MoveToFront();
		RequestFocus();

		// Add to menu manager?
		if ( m_bUseMenuManager )
		{
			g_MenuMgr.AddMenu( this );
		}
	}
	
	// must be after movetofront()
	BaseClass::SetVisible(state);
	_sizedForScrollBar = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Menu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetFgColor(GetSchemeColor("Menu.TextColor", pScheme));
	SetBgColor(GetSchemeColor("Menu.BgColor", pScheme));

	_borderDark = pScheme->GetColor("BorderDark", Color(255, 255, 255, 0));

	FOR_EACH_LL( m_MenuItems, i )
	{
		if( m_MenuItems[i]->IsCheckable() )
		{
			int wide, tall;
			m_MenuItems[i]->GetCheckImageSize( wide, tall );

			m_iCheckImageWidth = max ( m_iCheckImageWidth, wide );
		}
	}
	_recalculateWidth = true;
	CalculateWidth();

	InvalidateLayout();
}

void Menu::SetBgColor( Color newColor )
{
	BaseClass::SetBgColor( newColor );
	FOR_EACH_LL( m_MenuItems, i )
	{
		if( m_MenuItems[i]->HasMenu() )
		{
			m_MenuItems[i]->GetMenu()->SetBgColor( newColor );
		}
	}
}

void Menu::SetFgColor( Color newColor )
{
	BaseClass::SetFgColor( newColor );
	FOR_EACH_LL( m_MenuItems, i )
	{
		if( m_MenuItems[i]->HasMenu() )
		{
			m_MenuItems[i]->GetMenu()->SetFgColor( newColor );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Menu::SetBorder(class IBorder *border)
{
	Panel::SetBorder(border);
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a MenuItem that is this menus parent, if it has one
//-----------------------------------------------------------------------------
MenuItem *Menu::GetParentMenuItem()
{
	return dynamic_cast<MenuItem *>(GetParent());
}

//-----------------------------------------------------------------------------
// Purpose: Hide the menu when an item has been selected
//-----------------------------------------------------------------------------
void Menu::OnMenuItemSelected(Panel *panel)
{
	SetVisible(false);
	m_pScroller->SetVisible(false);
	
	// chain this message up through the hierarchy so
	// all the parent menus will close
	
	// get the parent of this menu. 
	MenuItem *item = GetParentMenuItem();
	// if the parent is a menu item, this menu is a cascading menu
	if (item)
	{
		// get the parent of the menuitem. it should be a menu.
		Menu *parentMenu = item->GetParentMenu();
		if (parentMenu)
		{
			// send the message to this parent menu
			KeyValues *kv = new KeyValues("MenuItemSelected");
			kv->SetPtr("panel", panel);
			ivgui()->PostMessage(parentMenu->GetVPanel(),  kv, GetVPanel());
		}
	}

	bool activeItemSet = false;
	
	FOR_EACH_LL( m_MenuItems, i )
	{
		if( m_MenuItems[i] == panel )
		{
			activeItemSet = true;
			m_iActivatedItem = i;
			break;
		}
	}
	if( !activeItemSet )
	{
		FOR_EACH_LL( m_MenuItems, i )
		{
			if(m_MenuItems[i]->HasMenu() )
			{
				/*
				// GetActiveItem needs to return -1 or similar if it hasn't been set...
				if( m_MenuItems[i]->GetActiveItem() )
				{
					m_iActivatedItem = m_MenuItems[i]->GetActiveItem();
				}*/
			}
		}
	}

	// also pass it to the parent so they can respond if they like
	if (GetVParent())
	{
		KeyValues *kv = new KeyValues("MenuItemSelected");
		kv->SetPtr("panel", panel);

		ivgui()->PostMessage(GetVParent(), kv, GetVPanel());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Menu::GetActiveItem()
{
	return m_iActivatedItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *Menu::GetItemUserData(int itemID)
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		// make sure its enabled since disabled items get highlighted.
		if (menuItem && menuItem->IsEnabled())
		{
			return menuItem->GetUserData();
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void Menu::GetItemText(int itemID, wchar_t *text, int bufLenInBytes)
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		if (menuItem)
		{
			menuItem->GetText(text, bufLenInBytes);
			return;
		}
	}
	text[0] = 0;
}

void Menu::GetItemText(int itemID, char *text, int bufLenInBytes)
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		if (menuItem)
		{
			menuItem->GetText( text, bufLenInBytes );
			return;
		}
	}
	text[0] = 0;
}



//-----------------------------------------------------------------------------
// Purpose: Activate the n'th item in the menu list, as if that menu item had been selected by the user
//-----------------------------------------------------------------------------
void Menu::ActivateItem(int itemID)
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		// make sure its enabled since disabled items get highlighted.
		if (menuItem && menuItem->IsEnabled())
		{
			menuItem->FireActionSignal();
			m_iActivatedItem = itemID;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Menu::SilentActivateItem(int itemID)
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		// make sure its enabled since disabled items get highlighted.
		if (menuItem && menuItem->IsEnabled())
		{
			m_iActivatedItem = itemID;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Menu::ActivateItemByRow(int row)
{
	if (m_SortedItems.IsValidIndex(row))
	{
		ActivateItem(m_SortedItems[row]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of items currently in the menu list
//-----------------------------------------------------------------------------
int Menu::GetItemCount() const
{
	return m_MenuItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Menu::GetMenuID(int index)
{
	if ( !m_SortedItems.IsValidIndex(index) )
		return m_MenuItems.InvalidIndex();

	return m_SortedItems[index];
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of items currently visible in the menu list
//-----------------------------------------------------------------------------
int Menu::GetCurrentlyVisibleItemsCount()
{
	if (m_MenuItems.Count() < m_iNumVisibleLines)
	{
		int cMenuItems = 0;
		FOR_EACH_LL(m_MenuItems, i)
		{
			if (m_MenuItems[i]->IsVisible())
			{
				++cMenuItems;
			}
		}

		return cMenuItems;
	}
	return m_iNumVisibleLines;
}

//-----------------------------------------------------------------------------
// Purpose: Enables/disables choices in the list
//			itemText - string name of item in the list 
//			state - true enables, false disables
//-----------------------------------------------------------------------------
void Menu::SetItemEnabled(const char *itemName, bool state)
{
	FOR_EACH_LL( m_MenuItems, i )
	{
		if ((Q_stricmp(itemName, m_MenuItems[i]->GetName())) == 0)
		{
			m_MenuItems[i]->SetEnabled(state);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enables/disables choices in the list
//-----------------------------------------------------------------------------
void Menu::SetItemEnabled(int itemID, bool state)
{
	if ( !m_MenuItems.IsValidIndex(itemID) )
		return;

	m_MenuItems[itemID]->SetEnabled(state);
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides choices in the list
//-----------------------------------------------------------------------------
void Menu::SetItemVisible(const char *itemName, bool state)
{
	FOR_EACH_LL( m_MenuItems, i )
	{
		if ((Q_stricmp(itemName, m_MenuItems[i]->GetName())) == 0)
		{
			m_MenuItems[i]->SetVisible(state);
			InvalidateLayout();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides choices in the list
//-----------------------------------------------------------------------------
void Menu::SetItemVisible(int itemID, bool state)
{
	if ( !m_MenuItems.IsValidIndex(itemID) )
		return;

	m_MenuItems[itemID]->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: Make the scroll bar visible and narrow the menu
//			also make items visible or invisible in the list as appropriate
//-----------------------------------------------------------------------------
void Menu::AddScrollBar()
{
	m_pScroller->SetVisible(true);
	_sizedForScrollBar = true;
}

//-----------------------------------------------------------------------------
// Purpose: Make the scroll bar invisible and widen the menu
//-----------------------------------------------------------------------------
void Menu::RemoveScrollBar()
{
	m_pScroller->SetVisible(false);
	_sizedForScrollBar = false;
}

//-----------------------------------------------------------------------------
// Purpose: Invalidate layout if the slider is moved so items scroll 
//-----------------------------------------------------------------------------
void Menu::OnSliderMoved()
{
	CloseOtherMenus(NULL); // close any cascading menus

	// Invalidate so we redraw the menu!
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Toggle into mouse mode.
//-----------------------------------------------------------------------------
void Menu::OnCursorMoved(int x, int y)
{
	m_iInputMode = MOUSE;
	
	// chain up
	CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
	RequestFocus();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Toggle into keyboard mode. 
//-----------------------------------------------------------------------------
void Menu::OnKeyCodePressed(KeyCode code)
{
	m_iInputMode = KEYBOARD;
	// send the message to this parent in case this is a cascading menu
	if (GetVParent())
	{
		ivgui()->PostMessage(GetVParent(), new KeyValues("KeyModeSet"), GetVPanel());
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the item currently highlighted in the menu by ptr
//-----------------------------------------------------------------------------
void Menu::SetCurrentlySelectedItem(MenuItem *item)
{	
	int itemNum = -1;
	// find it in our list of menuitems
	FOR_EACH_LL( m_MenuItems, i )
	{
		MenuItem *child = m_MenuItems[i];
		if (child == item)
		{
			itemNum = i;
			break;
		}
	}
	Assert( itemNum >= 0 );

	SetCurrentlySelectedItem(itemNum);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Menu::ClearCurrentlyHighlightedItem()
{
	if ( m_MenuItems.IsValidIndex(m_iCurrentlySelectedItemID) )
	{
		m_MenuItems[m_iCurrentlySelectedItemID]->DisarmItem();
	}
	m_iCurrentlySelectedItemID = m_MenuItems.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the item currently highlighted in the menu by index
//-----------------------------------------------------------------------------
void Menu::SetCurrentlySelectedItem(int itemID)
{
	// dont deselect if its the same item
	if (itemID == m_iCurrentlySelectedItemID)
		return;

	if ( m_MenuItems.IsValidIndex(m_iCurrentlySelectedItemID) )
	{
		m_MenuItems[m_iCurrentlySelectedItemID]->DisarmItem();
	}

	PostActionSignal(new KeyValues("MenuItemHighlight", "itemID", itemID));
	m_iCurrentlySelectedItemID = itemID;
}

//-----------------------------------------------------------------------------
// This will set the item to be currenly selected and highlight it
// will not open cascading menu. This was added for comboboxes
// to have the combobox item highlighted in the menu when they open the
// dropdown.
//-----------------------------------------------------------------------------
void Menu::SetCurrentlyHighlightedItem(int itemID)
{
	SetCurrentlySelectedItem(itemID);
	int row = m_SortedItems.Find(itemID);
	// If we have no items, then row will be -1. The dev console, for example...
	Assert( ( m_SortedItems.Count() == 0 ) || ( row != -1 ) );
	if ( row == -1 )
		return;

	// if there is a scroll bar, and we scroll off lets move it.
	if ( m_pScroller->IsVisible() )
	{
		// now if we are off the scroll bar, it means we moved the scroll bar
		// by hand or set the item off the list 
		// so just snap the scroll bar straight to the item.
		if ( ( row >  m_pScroller->GetValue() + m_iNumVisibleLines - 1 ) ||
			 ( row < m_pScroller->GetValue() ) )
		{				
			if ( !m_pScroller->IsVisible() )
				return;
			
			m_pScroller->SetValue(row);	
		}
	}

	if ( m_MenuItems.IsValidIndex(m_iCurrentlySelectedItemID) )
	{
		if ( !m_MenuItems[m_iCurrentlySelectedItemID]->IsArmed() )
		{
			m_MenuItems[m_iCurrentlySelectedItemID]->ArmItem();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int Menu::GetCurrentlyHighlightedItem()
{
	return m_iCurrentlySelectedItemID;
}

//-----------------------------------------------------------------------------
// Purpose: Respond to cursor entering a menuItem.
//-----------------------------------------------------------------------------
void Menu::OnCursorEnteredMenuItem(int VPanel)
{
	VPANEL menuItem = (VPANEL)VPanel;
	// if we are in mouse mode
	if (m_iInputMode == MOUSE)
	{
		MenuItem *item = static_cast<MenuItem *>(ipanel()->GetPanel(menuItem, GetModuleName()));
		// arm the menu
		item->ArmItem();
		// open the cascading menu if there is one.
		item->OpenCascadeMenu();
		SetCurrentlySelectedItem(item);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Respond to cursor exiting a menuItem
//-----------------------------------------------------------------------------
void Menu::OnCursorExitedMenuItem(int VPanel)
{
	VPANEL menuItem = (VPANEL)VPanel;
	// only care if we are in mouse mode
	if (m_iInputMode == MOUSE)
	{
		MenuItem *item = static_cast<MenuItem *>(ipanel()->GetPanel(menuItem, GetModuleName()));
		// unhighlight the item.
		// note menuItems with cascading menus will stay lit.
		item->DisarmItem();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move up or down one in the list of items in the menu 
//			Direction is MENU_UP or MENU_DOWN
//-----------------------------------------------------------------------------
void Menu::MoveAlongMenuItemList(int direction, int loopCount)
{
	// Early out if no menu items to scroll through
	if (m_MenuItems.Count() <= 0)
		return;

	int itemID = m_iCurrentlySelectedItemID;
	int row = m_SortedItems.Find(itemID);
	row += direction;
	
	if ( row > m_SortedItems.Count() - 1 )
	{
		if ( m_pScroller->IsVisible() )
		{
			// stop at bottom of scrolled list
			row = m_SortedItems.Count() - 1;
		}
		else
		{
			// if no scroll bar we circle around
			row = 0;
		}
	}
	else if (row < 0)
	{
		if ( m_pScroller->IsVisible() )
		{
			// stop at top of scrolled list
			row = m_pScroller->GetValue();
		}
		else
		{
			// if no scroll bar circle around
			row = m_SortedItems.Count()-1;
		}
	}

	// if there is a scroll bar, and we scroll off lets move it.
	if ( m_pScroller->IsVisible() )
	{
		if ( row > m_pScroller->GetValue() + m_iNumVisibleLines - 1)
		{				
			int val = m_pScroller->GetValue();
			val -= -direction;
			
			m_pScroller->SetValue(val);	
			
			// moving the slider redraws the scrollbar,
			// and so we should redraw the menu since the
			// menu draws the black border to the right of the scrollbar.
			InvalidateLayout();
		}
		else if ( row < m_pScroller->GetValue() )
		{				
			int val = m_pScroller->GetValue();	
			val -= -direction;
			
			m_pScroller->SetValue(val);	
			
			// moving the slider redraws the scrollbar,
			// and so we should redraw the menu since the
			// menu draws the black border to the right of the scrollbar.
			InvalidateLayout();	
		}
	
		// now if we are still off the scroll bar, it means we moved the scroll bar
		// by hand and created a situation in which we moved an item down, but the
		// scroll bar is already too far down and should scroll up or vice versa
		// so just snap the scroll bar straight to the item.
		if ( ( row > m_pScroller->GetValue() + m_iNumVisibleLines - 1) ||
			 ( row < m_pScroller->GetValue() ) )
		{				
			m_pScroller->SetValue(row);	
		}
	}

	// switch it back to an itemID from row
	if ( m_SortedItems.IsValidIndex( row ) )
	{
		SetCurrentlySelectedItem( m_SortedItems[row] );
	}

	// don't allow us to loop around more than once
	if (loopCount < m_MenuItems.Count())
	{
		// see if the text is empty, if so skip
		wchar_t text[256];
		m_MenuItems[m_iCurrentlySelectedItemID]->GetText(text, 255);
		if (text[0] == 0 || !m_MenuItems[m_iCurrentlySelectedItemID]->IsVisible())
		{
			// menu item is empty, keep moving along
			MoveAlongMenuItemList(direction, loopCount + 1);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return which type of events the menu is currently interested in
//  MenuItems need to know because behaviour is different depending on mode.
//-----------------------------------------------------------------------------
int Menu::GetMenuMode()
{
	return m_iInputMode;
}

//-----------------------------------------------------------------------------
// Purpose: Set the menu to key mode if a child menu goes into keymode
// This mode change has to be chained up through the menu heirarchy
// so cascading menus will work when you do a bunch of stuff in keymode
// in high level menus and then switch to keymode in lower level menus.
//-----------------------------------------------------------------------------
void Menu::OnKeyModeSet()
{
	m_iInputMode = KEYBOARD;
}

//-----------------------------------------------------------------------------
// Purpose: Set the checked state of a menuItem
//-----------------------------------------------------------------------------
void Menu::SetMenuItemChecked(int itemID, bool state)
{
	m_MenuItems[itemID]->SetChecked(state);
}

//-----------------------------------------------------------------------------
// Purpose: Check if item is checked.
//-----------------------------------------------------------------------------
bool Menu::IsChecked(int itemID) 
{
	return m_MenuItems[itemID]->IsChecked();
}

//-----------------------------------------------------------------------------
// Purpose: Set the minmum width the menu has to be. This
// is useful if you have a menu that is sized to the largest item in it
// but you don't want the menu to be thinner than the menu button
//-----------------------------------------------------------------------------
void Menu::SetMinimumWidth(int width)
{
	m_iMinimumWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: Get the minmum width the menu
//-----------------------------------------------------------------------------
int Menu::GetMinimumWidth()
{
	return m_iMinimumWidth;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void Menu::AddSeparator()
{
	int lastID = m_MenuItems.Count() - 1;
	m_Separators.AddToTail( lastID );
	m_SeparatorPanels.AddToTail( new MenuSeparator( this, "MenuSeparator" ) );
}

void Menu::AddSeparatorAfterItem( int itemID )
{
	Assert( m_MenuItems.IsValidIndex( itemID ) );
	m_Separators.AddToTail( itemID );
	m_SeparatorPanels.AddToTail( new MenuSeparator( this, "MenuSeparator" ) );
}

void Menu::MoveMenuItem( int itemID, int moveBeforeThisItemID )
{
	int c = m_SortedItems.Count();
	int i;
	for ( i = 0; i < c; ++i )
	{
		if ( m_SortedItems[i] == itemID )
		{
			m_SortedItems.Remove( i );
			break;
		}
	}

	// Didn't find it
	if ( i >= c )
	{
		return;
	}

	// Now find insert pos
	c = m_SortedItems.Count();
	for ( i = 0; i < c; ++i )
	{
		if ( m_SortedItems[i] == moveBeforeThisItemID )
		{
			m_SortedItems.InsertBefore( i, itemID );
			break;
		}
	}
}

void Menu::SetFont( HFont font )
{
	m_hItemFont = font;
	if ( font )
	{
		m_iMenuItemHeight = surface()->GetFontTall( font ) + 2;
	}
	InvalidateLayout();
}


void Menu::SetCurrentKeyBinding( int itemID, char const *hotkey )
{
	if ( m_MenuItems.IsValidIndex( itemID ) )
	{
		MenuItem *menuItem = dynamic_cast<MenuItem *>(m_MenuItems[itemID]);
		menuItem->SetCurrentKeyBinding( hotkey );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method to display a context menu
// Input  : *parent - 
//			*menu - 
//-----------------------------------------------------------------------------
void Menu::PlaceContextMenu( Panel *parent, Menu *menu )
{
	Assert( parent );
	Assert( menu );
	if ( !menu || !parent )
		return;

	menu->SetVisible(false);
	menu->SetParent( parent );
	menu->AddActionSignalTarget( parent );

	// get cursor position, this is local to this text edit window
	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);

	menu->SetVisible(true);
	
	// relayout the menu immediately so that we know it's size
	menu->InvalidateLayout(true);
	int menuWide, menuTall;
	menu->GetSize(menuWide, menuTall);
	
	// work out where the cursor is and therefore the best place to put the menu
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if (wide - menuWide > cursorX)
	{
		// menu hanging right
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			menu->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			menu->SetPos(cursorX, cursorY - menuTall);
		}
	}
	else
	{
		// menu hanging left
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			menu->SetPos(cursorX - menuWide, cursorY);
		}
		else
		{
			// menu hanging up
			menu->SetPos(cursorX - menuWide, cursorY - menuTall);
		}
	}
	
	menu->RequestFocus();
}

void Menu::SetUseFallbackFont( bool bState, HFont hFallback )
{
	m_hFallbackItemFont = hFallback;
	m_bUseFallbackFont = bState;
}

#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Run a global validation pass on all of our data structures and memory
//			allocations.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void Menu::Validate( CValidator &validator, char *pchName )
{
	validator.Push( "vgui::Menu", this, pchName );

	m_MenuItems.Validate( validator, "m_MenuItems" );
	m_SortedItems.Validate( validator, "m_SortedItems" );

	BaseClass::Validate( validator, "vgui::Menu" );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE


MenuBuilder::MenuBuilder( Menu *pMenu, Panel *pActionTarget )
	: m_pMenu( pMenu )
	, m_pActionTarget( pActionTarget )
	, m_pszLastCategory( NULL )
{}

MenuItem* MenuBuilder::AddMenuItem( const char *pszButtonText, const char *pszCommand, const char *pszCategoryName )
{
	AddSepratorIfNeeded( pszCategoryName );
	return m_pMenu->GetMenuItem( m_pMenu->AddMenuItem( pszButtonText, new KeyValues( pszCommand ), m_pActionTarget ) );
}

MenuItem* MenuBuilder::AddCascadingMenuItem( const char *pszButtonText, Menu *pSubMenu, const char *pszCategoryName )
{
	AddSepratorIfNeeded( pszCategoryName );
	return m_pMenu->GetMenuItem( m_pMenu->AddCascadingMenuItem( pszButtonText, m_pActionTarget, pSubMenu ) );
}

void MenuBuilder::AddSepratorIfNeeded( const char *pszCategoryName )
{
	// Add a separator if the categories are different
	if ( m_pszLastCategory && V_stricmp( pszCategoryName, m_pszLastCategory ) != 0 )
	{
		m_pMenu->AddSeparator();
	}

	m_pszLastCategory = pszCategoryName;
}
