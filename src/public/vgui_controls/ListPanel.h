//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LISTPANEL_H
#define LISTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <utllinkedlist.h>
#include <utlvector.h>
#include <utlrbtree.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class KeyValues;

namespace vgui
{

class ScrollBar;
class TextImage;
class ImagePanel;
class Label;
class Button;
class IDraggerEvent;
class FastSortListPanelItem;

//-----------------------------------------------------------------------------
// Purpose: Generic class for ListPanel items
//-----------------------------------------------------------------------------
class ListPanelItem
{
public:
	ListPanelItem() :
		kv( 0 ),
		userData( 0 ),
		m_pDragData( 0 ),
		m_bImage( false ),
		m_nImageIndex( -1 ),
		m_nImageIndexSelected( -1 ),
		m_pIcon( 0 )
	{
	}

	KeyValues		*kv;
	unsigned int 	userData;
	KeyValues		*m_pDragData;
	bool			m_bImage;
	int				m_nImageIndex;
	int				m_nImageIndexSelected;
	IImage			*m_pIcon;
};

typedef int __cdecl SortFunc( 
	ListPanel *pPanel, 
	const ListPanelItem &item1,
	const ListPanelItem &item2 );

//-----------------------------------------------------------------------------
// Purpose: A spread-sheet type data view, similar to MFC's 
//-----------------------------------------------------------------------------
class ListPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( ListPanel, Panel );

public:
	ListPanel(Panel *parent, const char *panelName);
	~ListPanel();

	// COLUMN HANDLING
	// all indices are 0 based, limit of 255 columns
	// columns are resizable by default
	enum ColumnFlags_e
	{
		COLUMN_FIXEDSIZE		= 0x01, // set to have the column be a fixed size
		COLUMN_RESIZEWITHWINDOW	= 0x02, // set to have the column grow with the parent dialog growing
		COLUMN_IMAGE			= 0x04,	// set if the column data is not text, but instead the index of the image to display
		COLUMN_HIDDEN			= 0x08,	// column is hidden by default
		COLUMN_UNHIDABLE		= 0x10,	// column is unhidable
	};

	// adds a column header
	virtual void AddColumnHeader(int index, const char *columnName, const char *columnText, int startingWidth, int minWidth, int maxWidth, int columnFlags = 0); 
	virtual void AddColumnHeader(int index, const char *columnName, const char *columnText, int width, int columnFlags = 0);

	virtual void RemoveColumn(int column);	// removes a column
	virtual int  FindColumn(const char *columnName);
	virtual void SetColumnHeaderHeight( int height );
	virtual void SetColumnHeaderText(int column, const char *text);
	virtual void SetColumnHeaderText(int column, wchar_t *text);
	virtual void SetColumnHeaderImage(int column, int imageListIndex);
	virtual void SetColumnHeaderTooltip(int column, const char *tooltipText);
	virtual void SetColumnTextAlignment( int column, int align );

	// Get information about the column headers.
	virtual int GetNumColumnHeaders() const;
	virtual bool GetColumnHeaderText( int index, char *pOut, int maxLen );

	virtual void SetSortFunc(int column, SortFunc *func);
	virtual void SetSortColumn(int column);
	virtual void SortList( void );
	virtual void SetColumnSortable(int column, bool sortable);
	virtual void SetColumnVisible(int column, bool visible);
	int GetSortColumn() const;

	// sets whether the user can add/remove columns (defaults to off)
	virtual void SetAllowUserModificationOfColumns(bool allowed);
	
	// DATA HANDLING
	// data->GetName() is used to uniquely identify an item
	// data sub items are matched against column header name to be used in the table
	virtual int AddItem(const KeyValues *data, unsigned int userData, bool bScrollToItem, bool bSortOnAdd); // Takes a copy of the data for use in the table. Returns the index the item is at.
	void SetItemDragData( int itemID, const KeyValues *data ); // Makes a copy of the keyvalues to store in the table. Used when dragging from the table. Only used if the caller enables drag support
	virtual int	GetItemCount( void );			// returns the number of VISIBLE items
	virtual int GetItem(const char *itemName);	// gets the row index of an item by name (data->GetName())
	virtual KeyValues *GetItem(int itemID); // returns pointer to data the row holds
	virtual int GetItemCurrentRow(int itemID);		// returns -1 if invalid index or item not visible
	virtual int GetItemIDFromRow(int currentRow);			// returns -1 if invalid row
	virtual unsigned int GetItemUserData(int itemID);
	virtual ListPanelItem *GetItemData(int itemID);
	virtual void SetUserData( int itemID, unsigned int userData );
	virtual int GetItemIDFromUserData( unsigned int userData );
	virtual void ApplyItemChanges(int itemID); // applies any changes to the data, performed by modifying the return of GetItem() above
	virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
	virtual void RereadAllItems(); // updates the view with the new data

	virtual void RemoveAll();		// clears and deletes all the memory used by the data items
	virtual void DeleteAllItems();	// obselete, use RemoveAll();

	virtual void GetCellText(int itemID, int column, OUT_Z_BYTECAP(bufferSizeInBytes) wchar_t *buffer, int bufferSizeInBytes); // returns the data held by a specific cell
	virtual IImage *GetCellImage(int itemID, int column); //, ImagePanel *&buffer); // returns the image held by a specific cell

	// Use these until they return InvalidItemID to iterate all the items.
	virtual int FirstItem() const;
	virtual int NextItem( int iItem ) const;

	virtual int InvalidItemID() const;
	virtual bool IsValidItemID(int itemID);

	// sets whether the dataitem is visible or not
	// it is removed from the row list when it becomes invisible, but stays in the indexes
	// this is much faster than a normal remove
	virtual void SetItemVisible(int itemID, bool state);
	virtual void SetItemDisabled(int itemID, bool state );
	bool IsItemVisible( int itemID );

	virtual void SetFont(HFont font);

	// image handling
	virtual void SetImageList(ImageList *imageList, bool deleteImageListWhenDone);

	// SELECTION
	
	// returns the count of selected items
	virtual int GetSelectedItemsCount();

	// returns the selected item by selection index, valid in range [0, GetNumSelectedRows)
	virtual int GetSelectedItem(int selectionIndex);

	// sets no item as selected
	virtual void ClearSelectedItems();

	virtual bool IsItemSelected( int itemID );

	// adds a item to the select list
	virtual void AddSelectedItem( int itemID );

	// sets this single item as the only selected item
	virtual void SetSingleSelectedItem( int itemID );

	// returns the selected column, -1 for particular column selected
	virtual int GetSelectedColumn();

	// whether or not to select specific cells (off by default)
	virtual void SetSelectIndividualCells(bool state);

	// whether or not multiple cells/rows can be selected
	void SetMultiselectEnabled( bool bState );
	bool IsMultiselectEnabled() const;

	// sets a single cell - all other previous rows are cleared
	virtual void SetSelectedCell(int row, int column);

	virtual bool GetCellAtPos(int x, int y, int &row, int &column);	// returns true if any found, row and column are filled out. x, y are in screen space
	virtual bool GetCellBounds( int row, int column, int& x, int& y, int& wide, int& tall );

	// sets the text which is displayed when the list is empty
	virtual void SetEmptyListText(const char *text);
	virtual void SetEmptyListText(const wchar_t *text);

	// relayout the scroll bar in response to changing the items in the list panel
	// do this if you RemoveAll()
	void ResetScrollBar();

	// Attaches drag data to a particular item
	virtual void OnCreateDragData( KeyValues *msg );

	void		SetIgnoreDoubleClick( bool state );

	// set up a field for editing
	virtual void EnterEditMode(int itemID, int column, vgui::Panel *editPanel);

	// leaves editing mode
	virtual void LeaveEditMode();

	// returns true if we are currently in inline editing mode
	virtual bool IsInEditMode();

	MESSAGE_FUNC_INT( ResizeColumnToContents, "ResizeColumnToContents", column );

#ifdef _X360
	virtual void NavigateTo();
#endif
	/// Version number for file format of user config.  This defaults to 1,
	/// and if you rearrange columns you can increment it to cause any old
	/// user configs (which will be screwed up) to be discarded.
	int m_nUserConfigFileVersion;

protected:
	// PAINTING
	virtual Panel *GetCellRenderer(int row, int column);

	// overrides
	virtual void OnMouseWheeled(int delta);
	virtual void OnSizeChanged(int wide, int tall); 
	virtual void PerformLayout();
	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnMousePressed( MouseCode code );
	virtual void OnMouseDoublePressed( MouseCode code );
#ifdef _X360
	virtual void OnKeyCodePressed(KeyCode code);
#else
	virtual void OnKeyCodePressed( KeyCode code );
#endif
	MESSAGE_FUNC( OnSliderMoved, "ScrollBarSliderMoved" );
	MESSAGE_FUNC_INT_INT( OnColumnResized, "ColumnResized", column, delta );
	MESSAGE_FUNC_INT( OnSetSortColumn, "SetSortColumn", column );
	MESSAGE_FUNC( OpenColumnChoiceMenu, "OpenColumnChoiceMenu" );
	MESSAGE_FUNC_INT( OnToggleColumnVisible, "ToggleColumnVisible", col );
	virtual float GetRowsPerPage();
	virtual int GetStartItem();

	// user configuration
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);
	virtual void GetUserConfigSettings(KeyValues *userConfig);
	virtual bool HasUserConfigSettings();

	/* MESSAGES SENT
		"ItemSelected" - query which items are selected
		"ItemDeselected" - query which items are selected
	*/

public:
	virtual void SetSortColumnEx( int iPrimarySortColumn, int iSecondarySortColumn, bool bSortAscending );
	void GetSortColumnEx( int &iPrimarySortColumn, int &iSecondarySortColumn, bool &bSortAscending ) const;

private:
	// Cleans up allocations associated with a particular item
	void CleanupItem( FastSortListPanelItem *data );

	// adds the item into the column indexes
	void IndexItem(int itemID);

	// Purpose: 
	void UpdateSelection( vgui::MouseCode code, int x, int y, int row, int column );

	// Handles multiselect 
	void HandleMultiSelection( int itemID, int row, int column );

	// Handles addselect 
	void HandleAddSelection( int itemID, int row, int column );

	// pre-sorted columns
	struct IndexItem_t
	{
		ListPanelItem *dataItem;
		int duplicateIndex;
	};
	typedef CUtlRBTree<IndexItem_t, int> IndexRBTree_t;

	struct column_t
	{
		Button *m_pHeader;
		int	m_iMinWidth;
		int	m_iMaxWidth;
		bool m_bResizesWithWindow;
		Panel *m_pResizer;
		SortFunc *m_pSortFunc;
		bool m_bTypeIsText;
		bool m_bHidden;
		bool m_bUnhidable;
		IndexRBTree_t m_SortedTree;		
		int m_nContentAlignment;
	};

	// list of the column headers
	CUtlLinkedList<column_t, unsigned char> 		m_ColumnsData;

	// persistent list of all columns ever created, indexes into m_ColumnsData - used for matching up DATAITEM m_SortedTreeIndexes
	CUtlVector<unsigned char>						m_ColumnsHistory;

	// current list of columns, indexes into m_ColumnsData
	CUtlVector<unsigned char>						m_CurrentColumns;

	int				    m_iColumnDraggerMoved; // which column dragger was moved->which header to resize
	int					m_lastBarWidth;

	CUtlLinkedList<FastSortListPanelItem*, int>		m_DataItems;
	CUtlVector<int>									m_VisibleItems;

	// set to true if the table needs to be sorted before it's drawn next
	int 				m_iSortColumn;
	int 				m_iSortColumnSecondary;

	void 				ResortColumnRBTree(int col);
	static bool 		RBTreeLessFunc(vgui::ListPanel::IndexItem_t &item1, vgui::ListPanel::IndexItem_t &item2);

	TextImage			*m_pTextImage; // used in rendering
	ImagePanel			*m_pImagePanel; // used in rendering
	Label				*m_pLabel;	  // used in rendering
	ScrollBar			*m_hbar;
	ScrollBar			*m_vbar;

	int				m_iSelectedColumn;

	bool 			m_bNeedsSort : 1;
	bool 			m_bSortAscending : 1;
	bool 			m_bSortAscendingSecondary : 1;
	bool			m_bCanSelectIndividualCells : 1;
	bool			m_bShiftHeldDown : 1;
	bool			m_bMultiselectEnabled : 1;
	bool			m_bAllowUserAddDeleteColumns : 1;
	bool 			m_bDeleteImageListWhenDone : 1;
	bool			m_bIgnoreDoubleClick : 1;

	int				m_iHeaderHeight;
	int 			m_iRowHeight;
	
	// selection data
	CUtlVector<int> 	m_SelectedItems;		// array of selected rows
	int					m_LastItemSelected;	// remember the last row selected for future shift clicks

	int 		m_iTableStartX;
	int	 		m_iTableStartY;

	Color 		m_LabelFgColor;
	Color		m_DisabledColor;
	Color 		m_SelectionFgColor;
	Color		m_DisabledSelectionFgColor;

	ImageList 	*m_pImageList;
	TextImage 	*m_pEmptyListText;

	PHandle		m_hEditModePanel;
	int			m_iEditModeItemID;
	int			m_iEditModeColumn;

	void ResetColumnHeaderCommands();
};

}

#endif // LISTPANEL_H
