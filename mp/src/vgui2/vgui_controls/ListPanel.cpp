//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>

#define PROTECTED_THINGS_DISABLE

#include <vgui/Cursor.h>
#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/Tooltip.h>

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"

using namespace vgui;

enum 
{
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef clamp
#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )
#endif

//-----------------------------------------------------------------------------
//
// Button at the top of columns used to re-sort
//
//-----------------------------------------------------------------------------
class ColumnButton : public Button
{
public:
	ColumnButton(vgui::Panel *parent, const char *name, const char *text);

	// Inherited from Button
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnMousePressed(MouseCode code);

	void OpenColumnChoiceMenu();
};

ColumnButton::ColumnButton(vgui::Panel *parent, const char *name, const char *text) : Button(parent, name, text)
{
	SetBlockDragChaining( true );
}

void ColumnButton::ApplySchemeSettings(IScheme *pScheme)
{
	Button::ApplySchemeSettings(pScheme);

	SetContentAlignment(Label::a_west);
	SetFont(pScheme->GetFont("DefaultSmall", IsProportional()));
}

// Don't request focus.
// This will keep items in the listpanel selected.
void ColumnButton::OnMousePressed(MouseCode code)
{
	if (!IsEnabled())
		return;

	if (code == MOUSE_RIGHT)
	{
		OpenColumnChoiceMenu();
		return;
	}
	
	if (!IsMouseClickEnabled(code))
		return;
	
	if (IsUseCaptureMouseEnabled())
	{
		{
			SetSelected(true);
			Repaint();
		}
		
		// lock mouse input to going to this button
		input()->SetMouseCapture(GetVPanel());
	}
}

void ColumnButton::OpenColumnChoiceMenu()
{
	CallParentFunction(new KeyValues("OpenColumnChoiceMenu"));
}


//-----------------------------------------------------------------------------
//
// Purpose: Handles resizing of columns
//
//-----------------------------------------------------------------------------
class Dragger : public Panel
{
public:
	Dragger(int column);

	// Inherited from Panel
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnCursorMoved(int x, int y);
	virtual void SetMovable(bool state);

private:
	int m_iDragger;
	bool m_bDragging;
	int m_iDragPos;
	bool m_bMovable; // whether this dragger is movable using mouse or not
};


Dragger::Dragger(int column)
{
	m_iDragger = column;
	SetPaintBackgroundEnabled(false);
	SetPaintEnabled(false);
	SetPaintBorderEnabled(false);
	SetCursor(dc_sizewe);
	m_bDragging = false;
	m_bMovable = true; // movable by default
	m_iDragPos = 0;
	SetBlockDragChaining( true );
}

void Dragger::OnMousePressed(MouseCode code)
{
	if (m_bMovable)
	{
		input()->SetMouseCapture(GetVPanel());
		
		int x, y;
		input()->GetCursorPos(x, y);
		m_iDragPos = x;
		m_bDragging = true;
	}
}

void Dragger::OnMouseDoublePressed(MouseCode code)
{
	if (m_bMovable)
	{
		// resize the column to the size of it's contents
		PostMessage(GetParent(), new KeyValues("ResizeColumnToContents", "column", m_iDragger));
	}
}

void Dragger::OnMouseReleased(MouseCode code)
{
	if (m_bMovable)
	{
		input()->SetMouseCapture(NULL);
		m_bDragging = false;
	}
}

void Dragger::OnCursorMoved(int x, int y)
{
	if (m_bDragging)
	{
		input()->GetCursorPos(x, y);
		KeyValues *msg = new KeyValues("ColumnResized");
		msg->SetInt("column", m_iDragger);
		msg->SetInt("delta", x - m_iDragPos);
		m_iDragPos = x;
		if (GetVParent())
		{
			ivgui()->PostMessage(GetVParent(), msg, GetVPanel());
		}
	}
}

void Dragger::SetMovable(bool state)
{
	m_bMovable = state;
	// disable cursor change if the dragger is not movable
	if( IsVisible() )
	{
		if (state)
		{
			// if its not movable we stick with the default arrow
			// if parent windows Start getting fancy cursors we should probably retrive a parent
			// cursor and set it to that
			SetCursor(dc_sizewe); 
		}
		else 
		{
			SetCursor(dc_arrow); 
		}
	}
}



namespace vgui
{
// optimized for sorting
class FastSortListPanelItem : public ListPanelItem
{
public:
	// index into accessing item to sort
	CUtlVector<int> m_SortedTreeIndexes;

	// visibility flag (for quick hide/filter)
	bool visible;

		// precalculated sort orders
	int primarySortIndexValue;
	int secondarySortIndexValue;
};
}

static ListPanel *s_pCurrentSortingListPanel = NULL;
static const char *s_pCurrentSortingColumn = NULL;
static bool	s_currentSortingColumnTypeIsText = false;

static SortFunc *s_pSortFunc = NULL;
static bool s_bSortAscending = true;
static SortFunc *s_pSortFuncSecondary = NULL;
static bool s_bSortAscendingSecondary = true;


//-----------------------------------------------------------------------------
// Purpose: Basic sort function, for use in qsort
//-----------------------------------------------------------------------------
static int __cdecl AscendingSortFunc(const void *elem1, const void *elem2)
{
	int itemID1 = *((int *) elem1);
	int itemID2 = *((int *) elem2);

	// convert the item index into the ListPanelItem pointers
	vgui::ListPanelItem *p1, *p2;
	p1 = s_pCurrentSortingListPanel->GetItemData(itemID1);
	p2 = s_pCurrentSortingListPanel->GetItemData(itemID2);
	
	int result = s_pSortFunc( s_pCurrentSortingListPanel, *p1, *p2 );
	if (result == 0)
	{
		// use the secondary sort functino
		result = s_pSortFuncSecondary( s_pCurrentSortingListPanel, *p1, *p2 );

		if (!s_bSortAscendingSecondary)
		{
			result = -result;
		}

		if (result == 0)
		{
			// sort by the pointers to make sure we get consistent results
			if (p1 > p2)
			{
				result = 1;
			}
			else
			{
				result = -1;
			}
		}
	}
	else
	{
		// flip result if not doing an ascending sort
		if (!s_bSortAscending)
		{
			result = -result;
		}
	}

	return result;
}


//-----------------------------------------------------------------------------
// Purpose: Default column sorting function, puts things in alpabetical order
//          If images are the same returns 1, else 0
//-----------------------------------------------------------------------------
static int __cdecl DefaultSortFunc(
	ListPanel *pPanel, 
	const ListPanelItem &item1,
	const ListPanelItem &item2 )
{
	const vgui::ListPanelItem *p1 = &item1;
	const vgui::ListPanelItem *p2 = &item2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	const char *col = s_pCurrentSortingColumn;
	if (s_currentSortingColumnTypeIsText) // textImage column
	{
		if (p1->kv->FindKey(col, true)->GetDataType() == KeyValues::TYPE_INT)
		{
			// compare ints
			int s1 = p1->kv->GetInt(col, 0);
			int s2 = p2->kv->GetInt(col, 0);

			if (s1 < s2)
			{
				return -1;
			}
			else if (s1 > s2)
			{
				return 1;
			}
			return 0;
		}
		else
		{
			// compare as string
			const char *s1 = p1->kv->GetString(col, "");
			const char *s2 = p2->kv->GetString(col, "");

			return Q_stricmp(s1, s2);
		}
	}
	else    // its an imagePanel column
	{
	   	const ImagePanel *s1 = (const ImagePanel *)p1->kv->GetPtr(col, NULL);
		const ImagePanel *s2 = (const ImagePanel *)p2->kv->GetPtr(col, NULL);

		if (s1 < s2)
		{
			return -1;
		}
		else if (s1 > s2)
		{
			return 1;
		}
		return 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sorts items by comparing precalculated list values
//-----------------------------------------------------------------------------
static int __cdecl FastSortFunc(
	ListPanel *pPanel, 
	const ListPanelItem &item1,
	const ListPanelItem &item2 )
{
	const vgui::FastSortListPanelItem *p1 = (vgui::FastSortListPanelItem *)&item1;
	const vgui::FastSortListPanelItem *p2 = (vgui::FastSortListPanelItem *)&item2;

	Assert(p1 && p2);

	// compare the precalculated indices
	if (p1->primarySortIndexValue < p2->primarySortIndexValue)
	{
		return 1;
	}
	else if (p1->primarySortIndexValue > p2->primarySortIndexValue)
	{
		return -1;

	}

	// they're equal, compare the secondary indices
	if (p1->secondarySortIndexValue < p2->secondarySortIndexValue)
	{
		return 1;
	}
	else if (p1->secondarySortIndexValue > p2->secondarySortIndexValue)
	{
		return -1;

	}

	// still equal; just compare the pointers (so we get deterministic results)
	return (p1 < p2) ? 1 : -1;
}

static int s_iDuplicateIndex = 1;

//-----------------------------------------------------------------------------
// Purpose: sorting function used in the column index redblack tree
//-----------------------------------------------------------------------------
bool ListPanel::RBTreeLessFunc(vgui::ListPanel::IndexItem_t &item1, vgui::ListPanel::IndexItem_t &item2)
{
	int result = s_pSortFunc( s_pCurrentSortingListPanel, *item1.dataItem, *item2.dataItem);
	if (result == 0)
	{
		// they're the same value, set their duplicate index to reflect that
		if (item1.duplicateIndex)
		{
			item2.duplicateIndex = item1.duplicateIndex;
		}
		else if (item2.duplicateIndex)
		{
			item1.duplicateIndex = item2.duplicateIndex;
		}
		else
		{
			item1.duplicateIndex = item2.duplicateIndex = s_iDuplicateIndex++;
		}
	}
	return (result > 0);
}


DECLARE_BUILD_FACTORY( ListPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ListPanel::ListPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_bIgnoreDoubleClick = false;
	m_bMultiselectEnabled = true;
	m_iEditModeItemID = 0;
	m_iEditModeColumn = 0;

	m_iHeaderHeight = 20;
	m_iRowHeight = 20;
	m_bCanSelectIndividualCells = false;
	m_iSelectedColumn = -1;
	m_bAllowUserAddDeleteColumns = false;

	m_hbar = new ScrollBar(this, "HorizScrollBar", false);
	m_hbar->AddActionSignalTarget(this);
	m_hbar->SetVisible(false);
	m_vbar = new ScrollBar(this, "VertScrollBar", true);
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget(this);

	m_pLabel = new Label(this, NULL, "");
	m_pLabel->SetVisible(false);
	m_pLabel->SetPaintBackgroundEnabled(false);
	m_pLabel->SetContentAlignment(Label::a_west);

	m_pTextImage = new TextImage( "" );
	m_pImagePanel = new ImagePanel(NULL, "ListImage");
	m_pImagePanel->SetAutoDelete(false);

	m_iSortColumn = -1;
	m_iSortColumnSecondary = -1;
	m_bSortAscending = true;
	m_bSortAscendingSecondary = true;

	m_lastBarWidth = 0;
	m_iColumnDraggerMoved = -1;
	m_bNeedsSort = false;
	m_LastItemSelected = -1;

	m_pImageList = NULL;
	m_bDeleteImageListWhenDone = false;
	m_pEmptyListText = new TextImage("");

	m_nUserConfigFileVersion = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ListPanel::~ListPanel()
{
	// free data from table
	RemoveAll();

	// free column headers
	unsigned char i;
	for ( i = m_ColumnsData.Head(); i != m_ColumnsData.InvalidIndex(); i= m_ColumnsData.Next( i ) )
	{
		m_ColumnsData[i].m_pHeader->MarkForDeletion();
		m_ColumnsData[i].m_pResizer->MarkForDeletion();
	}
	m_ColumnsData.RemoveAll();

	delete m_pTextImage;
	delete m_pImagePanel;
	delete m_vbar;

	if ( m_bDeleteImageListWhenDone )
	{
		delete m_pImageList;
	}

	delete m_pEmptyListText;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetImageList(ImageList *imageList, bool deleteImageListWhenDone)
{
	// get rid of existing list image if there's one and we're supposed to get rid of it
	if ( m_pImageList && m_bDeleteImageListWhenDone )
	{
		delete m_pImageList;
	}

	m_bDeleteImageListWhenDone = deleteImageListWhenDone;
	m_pImageList = imageList;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetColumnHeaderHeight( int height )
{
	m_iHeaderHeight = height;
}

//-----------------------------------------------------------------------------
// Purpose: adds a column header. 
//			this->FindChildByName(columnHeaderName) can be used to retrieve a pointer to a header panel by name
//
// if minWidth and maxWidth are BOTH NOTRESIZABLE or RESIZABLE
// the min and max size will be calculated automatically for you with that attribute
// columns are resizable by default
// if min and max size are specified column is resizable
//
// A small note on passing numbers for minWidth and maxWidth, 
// If the initial window size is larger than the sum of the original widths of the columns,
// you can wind up with the columns "snapping" to size after the first window focus
// This is because the dxPerBar being calculated in PerformLayout()
// is making resizable bounded headers exceed thier maxWidths at the Start. 
// Solution is to either put in support for redistributing the extra dx being truncated and
// therefore added to the last column on window opening, which is what causes the snapping.
// OR to
// ensure the difference between the starting sum of widths is not too much smaller/bigger 
// than the starting window size so the starting dx doesn't cause snapping to occur.
// The easiest thing is to simply set it so your column widths add up to the starting size of the window on opening.
//
// Another note: Always give bounds for the last column you add or make it not resizable.
//
// Columns can have text headers or images for headers (e.g. password icon)
//-----------------------------------------------------------------------------
void ListPanel::AddColumnHeader(int index, const char *columnName, const char *columnText, int width, int columnFlags)
{
	if (columnFlags & COLUMN_FIXEDSIZE && !(columnFlags & COLUMN_RESIZEWITHWINDOW))
	{
		// for fixed size columns, set the min & max widths to be the same as the initial width
		AddColumnHeader( index, columnName, columnText, width, width, width, columnFlags);
	}
	else
	{
		AddColumnHeader( index, columnName, columnText, width, 20, 10000, columnFlags);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new column
//-----------------------------------------------------------------------------
void ListPanel::AddColumnHeader(int index, const char *columnName, const char *columnText, int width, int minWidth, int maxWidth, int columnFlags)
{
	Assert (minWidth <= width);
	Assert (maxWidth >= width);

	// get our permanent index
	unsigned char columnDataIndex = m_ColumnsData.AddToTail();

	// put this index on the tail, so all item's m_SortedTreeIndexes have a consistent mapping
	m_ColumnsHistory.AddToTail(columnDataIndex);

	// put this column in the right place visually
	m_CurrentColumns.InsertBefore(index, columnDataIndex);

	// create the actual column object
	column_t &column = m_ColumnsData[columnDataIndex];

	// create the column header button
	Button *pButton = SETUP_PANEL(new ColumnButton(this, columnName, columnText));  // the cell rendering mucks with the button visibility during the solvetraverse loop,
																					//so force applyschemesettings to make sure its run
	pButton->SetSize(width, 24);
	pButton->AddActionSignalTarget(this);
	pButton->SetContentAlignment(Label::a_west);
	pButton->SetTextInset(5, 0);

	column.m_pHeader = pButton;
	column.m_iMinWidth = minWidth;
	column.m_iMaxWidth = maxWidth;
	column.m_bResizesWithWindow = columnFlags & COLUMN_RESIZEWITHWINDOW;
	column.m_bTypeIsText = !(columnFlags & COLUMN_IMAGE);
	column.m_bHidden = false;
	column.m_bUnhidable = (columnFlags & COLUMN_UNHIDABLE);
	column.m_nContentAlignment = Label::a_west;

	Dragger *dragger = new Dragger(index);
	dragger->SetParent(this);
	dragger->AddActionSignalTarget(this);
	dragger->MoveToFront();
	if (minWidth == maxWidth || (columnFlags & COLUMN_FIXEDSIZE)) 
	{
		// not resizable so disable the slider 
	   dragger->SetMovable(false);
	}
	column.m_pResizer = dragger;

	// add default sort function
	column.m_pSortFunc = NULL;
	
	// Set the SortedTree less than func to the generic RBTreeLessThanFunc
	m_ColumnsData[columnDataIndex].m_SortedTree.SetLessFunc((IndexRBTree_t::LessFunc_t)RBTreeLessFunc);

	// go through all the headers and make sure their Command has the right column ID
	ResetColumnHeaderCommands();

	// create the new data index
	ResortColumnRBTree(index);

	// ensure scroll bar is topmost compared to column headers
	m_vbar->MoveToFront();

	// fix up our visibility
	SetColumnVisible(index, !(columnFlags & COLUMN_HIDDEN));

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Recreates a column's RB Sorted Tree
//-----------------------------------------------------------------------------
void ListPanel::ResortColumnRBTree(int col)
{
	Assert(m_CurrentColumns.IsValidIndex(col));

	unsigned char dataColumnIndex = m_CurrentColumns[col];
	int columnHistoryIndex = m_ColumnsHistory.Find(dataColumnIndex);
	column_t &column = m_ColumnsData[dataColumnIndex];

	IndexRBTree_t &rbtree = column.m_SortedTree;

	// remove all elements - we're going to create from scratch
	rbtree.RemoveAll();

	s_pCurrentSortingListPanel = this;
	s_currentSortingColumnTypeIsText = column.m_bTypeIsText; // type of data in the column
	SortFunc *sortFunc = column.m_pSortFunc;
	if ( !sortFunc )
	{
		sortFunc = DefaultSortFunc;
	}
	s_pSortFunc = sortFunc;
	s_bSortAscending = true;
	s_pSortFuncSecondary = NULL;

	// sort all current data items for this column
	FOR_EACH_LL( m_DataItems, i )
	{
		IndexItem_t item;
		item.dataItem = m_DataItems[i];
		item.duplicateIndex = 0;

		FastSortListPanelItem *dataItem = (FastSortListPanelItem*) m_DataItems[i];

		// if this item doesn't already have a SortedTreeIndex for this column,
		// if can only be because this is the brand new column, so add it to the SortedTreeIndexes
		if (dataItem->m_SortedTreeIndexes.Count() == m_ColumnsHistory.Count() - 1 &&
			columnHistoryIndex == m_ColumnsHistory.Count() - 1)
		{
			dataItem->m_SortedTreeIndexes.AddToTail();
		}

		Assert( dataItem->m_SortedTreeIndexes.IsValidIndex(columnHistoryIndex) );

		dataItem->m_SortedTreeIndexes[columnHistoryIndex] = rbtree.Insert(item);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets the "SetSortColumn" command for each column - in case columns were added or removed
//-----------------------------------------------------------------------------
void ListPanel::ResetColumnHeaderCommands()
{
	int i;
	for ( i = 0 ; i < m_CurrentColumns.Count() ; i++ )
	{
		Button *pButton = m_ColumnsData[m_CurrentColumns[i]].m_pHeader;
		pButton->SetCommand(new KeyValues("SetSortColumn", "column", i));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the header text for a particular column.
//-----------------------------------------------------------------------------
void ListPanel::SetColumnHeaderText(int col, const char *text)
{
	m_ColumnsData[m_CurrentColumns[col]].m_pHeader->SetText(text);
}
void ListPanel::SetColumnHeaderText(int col, wchar_t *text)
{
	m_ColumnsData[m_CurrentColumns[col]].m_pHeader->SetText(text);
}

void ListPanel::SetColumnTextAlignment( int col, int align )
{
	m_ColumnsData[m_CurrentColumns[col]].m_nContentAlignment = align;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the column header to have an image instead of text
//-----------------------------------------------------------------------------
void ListPanel::SetColumnHeaderImage(int column, int imageListIndex)
{
	Assert(m_pImageList);
	m_ColumnsData[m_CurrentColumns[column]].m_pHeader->SetTextImageIndex(-1);
	m_ColumnsData[m_CurrentColumns[column]].m_pHeader->SetImageAtIndex(0, m_pImageList->GetImage(imageListIndex), 0);
}

//-----------------------------------------------------------------------------
// Purpose: associates a tooltip with the column header
//-----------------------------------------------------------------------------
void ListPanel::SetColumnHeaderTooltip(int column, const char *tooltipText)
{
	m_ColumnsData[m_CurrentColumns[column]].m_pHeader->GetTooltip()->SetText(tooltipText);
	m_ColumnsData[m_CurrentColumns[column]].m_pHeader->GetTooltip()->SetTooltipFormatToSingleLine();
	m_ColumnsData[m_CurrentColumns[column]].m_pHeader->GetTooltip()->SetTooltipDelay(0);
}

int ListPanel::GetNumColumnHeaders() const
{
	return m_CurrentColumns.Count();
}

bool ListPanel::GetColumnHeaderText( int index, char *pOut, int maxLen )
{
	if ( index < m_CurrentColumns.Count() )
	{
		m_ColumnsData[m_CurrentColumns[index]].m_pHeader->GetText( pOut, maxLen );
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetColumnSortable(int col, bool sortable)
{
	if (sortable)
	{
		m_ColumnsData[m_CurrentColumns[col]].m_pHeader->SetCommand(new KeyValues("SetSortColumn", "column", col));
	}
	else
	{
		m_ColumnsData[m_CurrentColumns[col]].m_pHeader->SetCommand((const char *)NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Changes the visibility of a column
//-----------------------------------------------------------------------------
void ListPanel::SetColumnVisible(int col, bool visible)
{
	column_t &column = m_ColumnsData[m_CurrentColumns[col]];
	bool bHidden = !visible;
	if (column.m_bHidden == bHidden)
		return;

	if (column.m_bUnhidable)
		return;

	column.m_bHidden = bHidden;
	if (bHidden)
	{
		column.m_pHeader->SetVisible(false);
		column.m_pResizer->SetVisible(false);
	}
	else
	{
		column.m_pHeader->SetVisible(true);
		column.m_pResizer->SetVisible(true);
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::RemoveColumn(int col)
{
	if ( !m_CurrentColumns.IsValidIndex( col ) )
		return;

	// find the appropriate column data 
	unsigned char columnDataIndex = m_CurrentColumns[col];

	// remove it from the current columns
	m_CurrentColumns.Remove(col);

	// zero out this entry in m_ColumnsHistory
	unsigned char i;
	for ( i = 0 ; i < m_ColumnsHistory.Count() ; i++ )
	{
		if ( m_ColumnsHistory[i] == columnDataIndex )
		{
			m_ColumnsHistory[i] = m_ColumnsData.InvalidIndex();
			break;
		}
	}
	Assert( i != m_ColumnsHistory.Count() );

	// delete and remove the column data
	m_ColumnsData[columnDataIndex].m_SortedTree.RemoveAll();
	m_ColumnsData[columnDataIndex].m_pHeader->MarkForDeletion();
	m_ColumnsData[columnDataIndex].m_pResizer->MarkForDeletion();
	m_ColumnsData.Remove(columnDataIndex);

	ResetColumnHeaderCommands();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index of a column by column->GetName()
//-----------------------------------------------------------------------------
int ListPanel::FindColumn(const char *columnName)
{
	for (int i = 0; i < m_CurrentColumns.Count(); i++)
	{
		if (!stricmp(columnName, m_ColumnsData[m_CurrentColumns[i]].m_pHeader->GetName()))
		{
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: adds an item to the view
//			data->GetName() is used to uniquely identify an item
//			data sub items are matched against column header name to be used in the table
//-----------------------------------------------------------------------------
int ListPanel::AddItem( const KeyValues *item, unsigned int userData, bool bScrollToItem, bool bSortOnAdd)
{
	FastSortListPanelItem *newitem = new FastSortListPanelItem;
	newitem->kv = item->MakeCopy();
	newitem->userData = userData;
	newitem->m_pDragData = NULL;
	newitem->m_bImage = newitem->kv->GetInt( "image" ) != 0 ? true : false;
	newitem->m_nImageIndex = newitem->kv->GetInt( "image" );
	newitem->m_nImageIndexSelected = newitem->kv->GetInt( "imageSelected" );
	newitem->m_pIcon = reinterpret_cast< IImage * >( newitem->kv->GetPtr( "iconImage" ) );

	int itemID = m_DataItems.AddToTail(newitem);
	int displayRow = m_VisibleItems.AddToTail(itemID);
	newitem->visible = true;

	// put the item in each column's sorted Tree Index
	IndexItem(itemID);

	if ( bSortOnAdd )
	{
		m_bNeedsSort = true;
	}

	InvalidateLayout();
	
	if ( bScrollToItem )
	{
		// scroll to last item
		m_vbar->SetValue(displayRow);
	}
	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetUserData( int itemID, unsigned int userData )
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	m_DataItems[itemID]->userData = userData;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the first itemID with a matching userData
//-----------------------------------------------------------------------------
int ListPanel::GetItemIDFromUserData( unsigned int userData )
{
	FOR_EACH_LL( m_DataItems, itemID )
	{
		if (m_DataItems[itemID]->userData == userData)
			return itemID;
	}
	// not found
	return InvalidItemID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	ListPanel::GetItemCount( void )
{
	return m_VisibleItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: gets the item ID of an item by name (data->GetName())
//-----------------------------------------------------------------------------
int ListPanel::GetItem(const char *itemName)
{
	FOR_EACH_LL( m_DataItems, i )
	{
		if (!stricmp(m_DataItems[i]->kv->GetName(), itemName))
		{
			return i;
		}
	}

	// failure
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: returns pointer to data the itemID holds
//-----------------------------------------------------------------------------
KeyValues *ListPanel::GetItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID]->kv;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListPanel::GetItemCurrentRow(int itemID)
{
	return m_VisibleItems.Find(itemID);
}


//-----------------------------------------------------------------------------
// Attaches drag data to a particular item 
//-----------------------------------------------------------------------------
void ListPanel::SetItemDragData( int itemID, const KeyValues *data )
{
	ListPanelItem *pItem = m_DataItems[ itemID ];
	if ( pItem->m_pDragData )
	{
		pItem->m_pDragData->deleteThis();
	}
	pItem->m_pDragData = data->MakeCopy();
}


//-----------------------------------------------------------------------------
// Attaches drag data to a particular item 
//-----------------------------------------------------------------------------
void ListPanel::OnCreateDragData( KeyValues *msg )
{
	int nCount = GetSelectedItemsCount();
	if ( nCount == 0 )
		return;

	for ( int i = 0; i < nCount; ++i )
	{
		int nItemID = GetSelectedItem( i );

		KeyValues *pDragData = m_DataItems[ nItemID ]->m_pDragData;
		if ( pDragData )
		{
			KeyValues *pDragDataCopy = pDragData->MakeCopy();
			msg->AddSubKey( pDragDataCopy );
		}
	}

	// Add the keys of the last item directly into the root also
	int nLastItemID = GetSelectedItem( nCount - 1 );
	KeyValues *pLastItemDrag = m_DataItems[ nLastItemID ]->m_pDragData;
	if ( pLastItemDrag )
	{
		pLastItemDrag->CopySubkeys( msg );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListPanel::GetItemIDFromRow(int currentRow)
{
	if (!m_VisibleItems.IsValidIndex(currentRow))
		return -1;

	return m_VisibleItems[currentRow];
}


int ListPanel::FirstItem() const
{
	return m_DataItems.Head();
}


int ListPanel::NextItem( int iItem ) const
{
	return m_DataItems.Next( iItem );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListPanel::InvalidItemID() const
{
	return m_DataItems.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ListPanel::IsValidItemID(int itemID)
{
	return m_DataItems.IsValidIndex(itemID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ListPanelItem *ListPanel::GetItemData( int itemID )
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;
	
	return m_DataItems[ itemID ];
}

//-----------------------------------------------------------------------------
// Purpose: returns user data for itemID
//-----------------------------------------------------------------------------
unsigned int ListPanel::GetItemUserData(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return 0;

	return m_DataItems[itemID]->userData;
}


//-----------------------------------------------------------------------------
// Purpose: updates the view with any changes to the data
// Input  : itemID - index to update
//-----------------------------------------------------------------------------
void ListPanel::ApplyItemChanges(int itemID)
{
	// reindex the item and then redraw
	IndexItem(itemID);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Adds the item into the column indexes
//-----------------------------------------------------------------------------
void ListPanel::IndexItem(int itemID)
{
	FastSortListPanelItem *newitem = (FastSortListPanelItem*) m_DataItems[itemID];

	// remove the item from the indexes and re-add
	int maxCount = min(m_ColumnsHistory.Count(), newitem->m_SortedTreeIndexes.Count());
	for (int i = 0; i < maxCount; i++)
	{
		IndexRBTree_t &rbtree = m_ColumnsData[m_ColumnsHistory[i]].m_SortedTree;
		rbtree.RemoveAt(newitem->m_SortedTreeIndexes[i]);
	}

	// make sure it's all free
	newitem->m_SortedTreeIndexes.RemoveAll();

	// reserve one index per historical column - pad it out
	newitem->m_SortedTreeIndexes.AddMultipleToTail(m_ColumnsHistory.Count());

	// set the current sorting list (since the insert will need to sort)
	s_pCurrentSortingListPanel = this;

	// add the item into the RB tree for each column
	for (int i = 0; i < m_ColumnsHistory.Count(); i++)
	{
		// skip over any removed columns
		if ( m_ColumnsHistory[i] == m_ColumnsData.InvalidIndex() )
			continue;

		column_t &column = m_ColumnsData[m_ColumnsHistory[i]];

		IndexItem_t item;
		item.dataItem = newitem;
		item.duplicateIndex = 0;

		IndexRBTree_t &rbtree = column.m_SortedTree;

		// setup sort state
		s_pCurrentSortingListPanel = this;
		s_pCurrentSortingColumn = column.m_pHeader->GetName(); // name of current column for sorting
		s_currentSortingColumnTypeIsText = column.m_bTypeIsText; // type of data in the column
		
		SortFunc *sortFunc = column.m_pSortFunc;
		if (!sortFunc)
		{
			sortFunc = DefaultSortFunc;
		}
		s_pSortFunc = sortFunc;
		s_bSortAscending = true;
		s_pSortFuncSecondary = NULL;

		// insert index		
		newitem->m_SortedTreeIndexes[i] = rbtree.Insert(item);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::RereadAllItems()
{
	//!! need to make this more efficient
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Cleans up allocations associated with a particular item
//-----------------------------------------------------------------------------
void ListPanel::CleanupItem( FastSortListPanelItem *data )
{
	if ( data )
	{
		if (data->kv)
		{
			data->kv->deleteThis();
		}
		if (data->m_pDragData)
		{
			data->m_pDragData->deleteThis();
		}
		delete data;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes an item at the specified item
//-----------------------------------------------------------------------------
void ListPanel::RemoveItem(int itemID)
{
#ifdef _X360
	bool renavigate = false;
	if(HasFocus())
	{
		for(int i = 0; i < GetSelectedItemsCount(); ++i)
		{
			if(itemID == GetSelectedItem(i))
			{
				renavigate = true;
				break;
			}
		}
	}
#endif

	FastSortListPanelItem *data = (FastSortListPanelItem*) m_DataItems[itemID];
	if (!data)
		return;

	// remove from column sorted indexes
	int i;
	for ( i = 0; i < m_ColumnsHistory.Count(); i++ )
	{
		if ( m_ColumnsHistory[i] == m_ColumnsData.InvalidIndex())
			continue;

		IndexRBTree_t &rbtree = m_ColumnsData[m_ColumnsHistory[i]].m_SortedTree;
		rbtree.RemoveAt(data->m_SortedTreeIndexes[i]);
	}

	// remove from selection
	m_SelectedItems.FindAndRemove(itemID);
	PostActionSignal( new KeyValues("ItemDeselected") );

	// remove from visible items
	m_VisibleItems.FindAndRemove(itemID);

	// remove from data
	m_DataItems.Remove(itemID);
	CleanupItem( data );
	InvalidateLayout();

#ifdef _X360
	if(renavigate)
	{
		NavigateTo();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void ListPanel::RemoveAll()
{
	// remove all sort indexes
	for (int i = 0; i < m_ColumnsHistory.Count(); i++)
	{
		m_ColumnsData[m_ColumnsHistory[i]].m_SortedTree.RemoveAll();
	}

	FOR_EACH_LL( m_DataItems, index )
	{
		FastSortListPanelItem *pItem = m_DataItems[index];
		CleanupItem( pItem );
	}

	m_DataItems.RemoveAll();
	m_VisibleItems.RemoveAll();
	ClearSelectedItems();

	InvalidateLayout();

#ifdef _X360
	if(HasFocus())
	{
		NavigateTo();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: obselete, use RemoveAll();
//-----------------------------------------------------------------------------
void ListPanel::DeleteAllItems()
{
	RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::ResetScrollBar()
{
	// delete and reallocate to besure the scroll bar's
	// information is correct.
	delete m_vbar;
	m_vbar = new ScrollBar(this, "VertScrollBar", true);
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: returns the count of selected rows
//-----------------------------------------------------------------------------
int ListPanel::GetSelectedItemsCount()
{
	return m_SelectedItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns the selected item by selection index
// Input  : selectionIndex - valid in range [0, GetNumSelectedRows)
// Output : int - itemID
//-----------------------------------------------------------------------------
int ListPanel::GetSelectedItem(int selectionIndex)
{
	if ( m_SelectedItems.IsValidIndex(selectionIndex))
		return m_SelectedItems[selectionIndex];

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListPanel::GetSelectedColumn()
{
	return m_iSelectedColumn;
}


//-----------------------------------------------------------------------------
// Purpose: Clears all selected rows
//-----------------------------------------------------------------------------
void ListPanel::ClearSelectedItems()
{
	int nPrevCount = m_SelectedItems.Count();
	m_SelectedItems.RemoveAll();
	if ( nPrevCount > 0 )
	{
		PostActionSignal( new KeyValues("ItemDeselected") );
	}
	m_LastItemSelected = -1;
	m_iSelectedColumn = -1;
}


//-----------------------------------------------------------------------------
bool ListPanel::IsItemSelected( int itemID )
{
	return m_DataItems.IsValidIndex( itemID ) && m_SelectedItems.HasElement( itemID );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::AddSelectedItem( int itemID )
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	Assert( !m_SelectedItems.HasElement( itemID ) );

	m_LastItemSelected = itemID;
	m_SelectedItems.AddToTail( itemID );
	PostActionSignal( new KeyValues("ItemSelected") );
	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetSingleSelectedItem( int itemID )
{
	ClearSelectedItems();
	AddSelectedItem(itemID);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetSelectedCell(int itemID, int col)
{
	if ( !m_bCanSelectIndividualCells )
	{
		SetSingleSelectedItem(itemID);
		return;
	}

	// make sure it's a valid cell
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;
	
	if ( !m_CurrentColumns.IsValidIndex(col) )
		return;
	
	SetSingleSelectedItem( itemID );
	m_iSelectedColumn = col;
}


//-----------------------------------------------------------------------------
// Purpose: returns the data held by a specific cell
//-----------------------------------------------------------------------------
void ListPanel::GetCellText(int itemID, int col, wchar_t *wbuffer, int bufferSizeInBytes)
{
	if ( !wbuffer || !bufferSizeInBytes )
		return;

	wcscpy( wbuffer, L"" );

	KeyValues *itemData = GetItem( itemID );
	if ( !itemData )
	{
		return;
	}

	// Look up column header
	if ( col < 0 || col >= m_CurrentColumns.Count() )
	{
		return;
	}

	const char *key = m_ColumnsData[m_CurrentColumns[col]].m_pHeader->GetName();
	if ( !key || !key[ 0 ] )
	{
		return;
	}

	char const *val = itemData->GetString( key, "" );
	if ( !val || !key[ 0 ] )
		return;

	const wchar_t *wval = NULL;

	if ( val[ 0 ] == '#' )
	{
		StringIndex_t si = g_pVGuiLocalize->FindIndex( val + 1 );
		if ( si != INVALID_LOCALIZE_STRING_INDEX )
		{
			wval = g_pVGuiLocalize->GetValueByIndex( si );
		}
	}

	if ( !wval )
	{
		wval = itemData->GetWString( key, L"" );
	}

	wcsncpy( wbuffer, wval, bufferSizeInBytes/sizeof(wchar_t) );
	wbuffer[ (bufferSizeInBytes/sizeof(wchar_t)) - 1 ] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: returns the data held by a specific cell
//-----------------------------------------------------------------------------
IImage *ListPanel::GetCellImage(int itemID, int col) //, ImagePanel *&buffer)
{
//	if ( !buffer )
//		return;

	KeyValues *itemData = GetItem( itemID );
	if ( !itemData )
	{
		return NULL;
	}

	// Look up column header
	if ( col < 0 || col >= m_CurrentColumns.Count() )
	{
		return NULL;
	}

	const char *key = m_ColumnsData[m_CurrentColumns[col]].m_pHeader->GetName();
	if ( !key || !key[ 0 ] )
	{
		return NULL;
	}

	if ( !m_pImageList )
	{
		return NULL;
	}

	int imageIndex = itemData->GetInt( key, 0 );
	if ( m_pImageList->IsValidIndex(imageIndex) )
	{
		if ( imageIndex > 0 )
		{
			return m_pImageList->GetImage(imageIndex);
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *ListPanel::GetCellRenderer(int itemID, int col)
{
	Assert( m_pTextImage );
	Assert( m_pImagePanel );
	
	column_t &column = m_ColumnsData[ m_CurrentColumns[col] ];

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	m_pLabel->SetContentAlignment( (Label::Alignment)column.m_nContentAlignment );

	if ( column.m_bTypeIsText ) 
	{
		wchar_t tempText[ 256 ];

		// Grab cell text
		GetCellText( itemID, col, tempText, 256 );
		KeyValues *item = GetItem( itemID );
		m_pTextImage->SetText(tempText);
        int cw, tall;
        m_pTextImage->GetContentSize(cw, tall);

		// set cell size
		Panel *header = column.m_pHeader;
	    int wide = header->GetWide();
		m_pTextImage->SetSize( min( cw, wide - 5 ), tall);

		m_pLabel->SetTextImageIndex( 0 );
		m_pLabel->SetImageAtIndex(0, m_pTextImage, 3);
			
		bool selected = false;
		if ( m_SelectedItems.HasElement(itemID) && ( !m_bCanSelectIndividualCells || col == m_iSelectedColumn ) )
		{
			selected = true;
            VPANEL focus = input()->GetFocus();
            // if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
            if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())))
            {
                m_pLabel->SetBgColor(GetSchemeColor("ListPanel.SelectedBgColor", pScheme));
    			// selection
            }
            else
            {
                m_pLabel->SetBgColor(GetSchemeColor("ListPanel.SelectedOutOfFocusBgColor", pScheme));
            }

			if ( item->IsEmpty("cellcolor") == false )
			{
	            m_pTextImage->SetColor( item->GetColor( "cellcolor" ) );
			}
			else if ( item->GetInt("disabled", 0) == 0 )
			{
	            m_pTextImage->SetColor(m_SelectionFgColor);
			}
			else 
			{
	            m_pTextImage->SetColor(m_DisabledSelectionFgColor);
			}

            m_pLabel->SetPaintBackgroundEnabled(true);
		}
		else
		{
			if ( item->IsEmpty("cellcolor") == false )
			{
	            m_pTextImage->SetColor( item->GetColor( "cellcolor" ) );
			}
			else if ( item->GetInt("disabled", 0) == 0 )
			{
				m_pTextImage->SetColor(m_LabelFgColor);
			}
			else
			{
				m_pTextImage->SetColor(m_DisabledColor);
			}
			m_pLabel->SetPaintBackgroundEnabled(false);
		}

		FastSortListPanelItem *listItem = m_DataItems[ itemID ];
		if ( col == 0 &&
			listItem->m_bImage && m_pImageList )
		{
			IImage *pImage = NULL;
			if ( listItem->m_pIcon )
			{
				pImage = listItem->m_pIcon;
			}
			else
			{
				int imageIndex = selected ? listItem->m_nImageIndexSelected : listItem->m_nImageIndex;
				if ( m_pImageList->IsValidIndex(imageIndex) )
				{
					pImage = m_pImageList->GetImage(imageIndex);
				}
			}

			if ( pImage )
			{
				m_pLabel->SetTextImageIndex( 1 );
				m_pLabel->SetImageAtIndex(0, pImage, 0);
				m_pLabel->SetImageAtIndex(1, m_pTextImage, 3);
			}
		}
		
		return m_pLabel;
	}
	else 	// if its an Image Panel
	{
		if ( m_SelectedItems.HasElement(itemID) && ( !m_bCanSelectIndividualCells || col == m_iSelectedColumn ) )
		{
            VPANEL focus = input()->GetFocus();
            // if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
            if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())))
            {
                m_pLabel->SetBgColor(GetSchemeColor("ListPanel.SelectedBgColor", pScheme));
    			// selection
            }
            else
            {
                m_pLabel->SetBgColor(GetSchemeColor("ListPanel.SelectedOutOfFocusBgColor", pScheme));
            }
			// selection
			m_pLabel->SetPaintBackgroundEnabled(true);
		}
		else
		{
			m_pLabel->SetPaintBackgroundEnabled(false);
		}

		IImage *pIImage = GetCellImage(itemID, col);
		m_pLabel->SetImageAtIndex(0, pIImage, 0);

		return m_pLabel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void ListPanel::PerformLayout()
{
	if ( m_CurrentColumns.Count() == 0 )
		return;
	
	if (m_bNeedsSort)
	{
		SortList();
	}

	int rowsperpage = (int) GetRowsPerPage();

	// count the number of visible items
	int visibleItemCount = m_VisibleItems.Count();

	//!! need to make it recalculate scroll positions
	m_vbar->SetVisible(true);
	m_vbar->SetEnabled(false);
	m_vbar->SetRangeWindow( rowsperpage );
	m_vbar->SetRange( 0, visibleItemCount);	
	m_vbar->SetButtonPressedScrollValue( 1 );

	int wide, tall;
	GetSize( wide, tall );
	m_vbar->SetPos(wide - (m_vbar->GetWide()+WINDOW_BORDER_WIDTH), 0);
	m_vbar->SetSize(m_vbar->GetWide(), tall - 2);
	m_vbar->InvalidateLayout();

	int buttonMaxXPos = wide - (m_vbar->GetWide()+WINDOW_BORDER_WIDTH);
	
	int nColumns = m_CurrentColumns.Count();
	// number of bars that can be resized
	int numToResize=0;
	if (m_iColumnDraggerMoved != -1) // we're resizing in response to a column dragger
	{
		numToResize = 1; // only one column will change size, the one we dragged
	}
	else	// we're resizing in response to a window resize
	{
		for (int i = 0; i < nColumns; i++)
		{
			if ( m_ColumnsData[m_CurrentColumns[i]].m_bResizesWithWindow // column is resizable in response to window
				&& !m_ColumnsData[m_CurrentColumns[i]].m_bHidden) 
			{
				numToResize++;
			}
		}
	}

	int dxPerBar; // zero on window first opening
	
	// location of the last column resizer
	int oldSizeX = 0, oldSizeY = 0;
	int lastColumnIndex = nColumns-1;
	for (int i = nColumns-1; i >= 0; --i)
	{
		if (!m_ColumnsData[m_CurrentColumns[i]].m_bHidden)
		{
			m_ColumnsData[m_CurrentColumns[i]].m_pHeader->GetPos(oldSizeX, oldSizeY);
			lastColumnIndex = i;
			break;
		}
	}

	bool bForceShrink = false;
	if ( numToResize == 0 )
	{
		// make sure we've got enough to be within minwidth
		int minWidth=0;
		for (int i = 0; i < nColumns; i++)
		{
			if (!m_ColumnsData[m_CurrentColumns[i]].m_bHidden)
			{
				minWidth += m_ColumnsData[m_CurrentColumns[i]].m_iMinWidth;
			}
		}
		
		// if all the minimum widths cannot fit in the space given, then we will shrink ALL columns an equal amount
		if (minWidth > buttonMaxXPos)
		{
			int dx = buttonMaxXPos - minWidth;
			dxPerBar=(int)((float)dx/(float)nColumns);
			bForceShrink = true;
		}
		else
		{
			dxPerBar = 0;
		}
		m_lastBarWidth = buttonMaxXPos;

	}
	else if ( oldSizeX != 0 ) // make sure this isnt the first time we opened the window
	{
		int dx = buttonMaxXPos - m_lastBarWidth;  // this is how much we grew or shrank.

		// see how many bars we have and now much each should grow/shrink
		dxPerBar=(int)((float)dx/(float)numToResize);
		m_lastBarWidth = buttonMaxXPos;
	}
	else // this is the first time we've opened the window, make sure all our colums fit! resize if needed
	{
		int startingBarWidth=0;
		for (int i = 0; i < nColumns; i++)
		{
			if (!m_ColumnsData[m_CurrentColumns[i]].m_bHidden)
			{
				startingBarWidth += m_ColumnsData[m_CurrentColumns[i]].m_pHeader->GetWide();
			}
		}
		int dx = buttonMaxXPos - startingBarWidth;  // this is how much we grew or shrank.
		// see how many bars we have and now much each should grow/shrink
		dxPerBar=(int)((float)dx/(float)numToResize);
		m_lastBarWidth = buttonMaxXPos;
	}

	// Make sure nothing is smaller than minwidth to start with or else we'll get into trouble below.
	for ( int i=0; i < nColumns; i++ )
	{
		column_t &column = m_ColumnsData[m_CurrentColumns[i]];
		Panel *header = column.m_pHeader;
		if ( header->GetWide() < column.m_iMinWidth )
			header->SetWide( column.m_iMinWidth );
	}

	// This was a while(1) loop and we hit an infinite loop case, so now we max out the # of times it can loop.
	for ( int iLoopSanityCheck=0; iLoopSanityCheck < 1000; iLoopSanityCheck++ )
	{
		// try and place headers as is - before we have to force items to be minimum width
		int x = -1;
		int i;
		for ( i = 0; i < nColumns; i++)
		{
			column_t &column = m_ColumnsData[m_CurrentColumns[i]];
			Panel *header = column.m_pHeader;
			if (column.m_bHidden)
			{
				header->SetVisible(false);
				continue;
			}

			header->SetPos(x, 0);
			header->SetVisible(true);

			// if we couldn't fit this column - then we need to force items to be minimum width
			if ( x+column.m_iMinWidth >= buttonMaxXPos && !bForceShrink )
			{
				break;
			}
	
			int hWide = header->GetWide();

			// calculate the column's width
			// make it so the last column always attaches to the scroll bar
			if ( i == lastColumnIndex )
			{
				hWide = buttonMaxXPos-x; 
			}
			else if (i == m_iColumnDraggerMoved ) // column resizing using dragger
			{
				hWide += dxPerBar; // adjust width of column
			}
			else if ( m_iColumnDraggerMoved == -1 )		// window is resizing
			{
				// either this column is allowed to resize OR we are forcing it because we're shrinking all columns
				if ( column.m_bResizesWithWindow || bForceShrink )
				{
					Assert ( column.m_iMinWidth <= column.m_iMaxWidth );
					hWide += dxPerBar; // adjust width of column
				}
			}

			// enforce column mins and max's - unless we're FORCING it to shrink
			if ( hWide < column.m_iMinWidth && !bForceShrink ) 
			{
				hWide = column.m_iMinWidth; // adjust width of column
			}
			else if ( hWide > column.m_iMaxWidth )
			{
				hWide = column.m_iMaxWidth;
			}
	
			header->SetSize(hWide, m_vbar->GetWide());
			x += hWide;
	
			// set the resizers
			Panel *sizer = column.m_pResizer;
			if ( i == lastColumnIndex )
			{
				sizer->SetVisible(false);
			}
			else
			{
				sizer->SetVisible(true);
			}
			sizer->MoveToFront();
			sizer->SetPos(x - 4, 0);
			sizer->SetSize(8, m_vbar->GetWide());
		}

		// we made it all the way through
		if ( i == nColumns )
			break;
	
		// we do this AFTER trying first, to let as many columns as possible try and get to their
		// desired width before we forcing the minimum width on them

		// get the total desired width of all the columns
		int totalDesiredWidth = 0;
		for ( i = 0 ; i < nColumns ; i++ )
		{
			if (!m_ColumnsData[m_CurrentColumns[i]].m_bHidden)
			{
				Panel *pHeader = m_ColumnsData[m_CurrentColumns[i]].m_pHeader;
				totalDesiredWidth += pHeader->GetWide();
			}
		}

		// shrink from the most right column to minimum width until we can fit them all
		Assert(totalDesiredWidth > buttonMaxXPos);
		for ( i = nColumns-1; i >= 0 ; i--)
		{
			column_t &column = m_ColumnsData[m_CurrentColumns[i]];
			if (!column.m_bHidden)
			{
				Panel *pHeader = column.m_pHeader;

				totalDesiredWidth -= pHeader->GetWide();
				if ( totalDesiredWidth + column.m_iMinWidth <= buttonMaxXPos )
				{
					int newWidth = buttonMaxXPos - totalDesiredWidth;
					pHeader->SetSize( newWidth, m_vbar->GetWide() );
					break;
				}

				totalDesiredWidth += column.m_iMinWidth;
				pHeader->SetSize(column.m_iMinWidth, m_vbar->GetWide());
			}
		}
		// If we don't allow this to shrink, then as we resize, it can get stuck in an infinite loop.
		dxPerBar -= 5;
		if ( dxPerBar < 0 )
			dxPerBar = 0;

		if ( i == -1 )
		{
			break;
		}
	}

	// setup edit mode
	if ( m_hEditModePanel.Get() )
	{
		m_iTableStartX = 0; 
		m_iTableStartY = m_iHeaderHeight + 1;

		int nTotalRows = m_VisibleItems.Count();
		int nRowsPerPage = GetRowsPerPage();

		// find the first visible item to display
		int nStartItem = 0;
		if (nRowsPerPage <= nTotalRows)
		{
			nStartItem = m_vbar->GetValue();
		}

		bool bDone = false;
		int drawcount = 0;
		for (int i = nStartItem; i < nTotalRows && !bDone; i++)
		{
			int x = 0;
			if (!m_VisibleItems.IsValidIndex(i))
				continue;

			int itemID = m_VisibleItems[i];
			
			// iterate the columns
			for (int j = 0; j < m_CurrentColumns.Count(); j++)
			{
				Panel *header = m_ColumnsData[m_CurrentColumns[j]].m_pHeader;

				if (!header->IsVisible())
					continue;

				int wide = header->GetWide();

				if ( itemID == m_iEditModeItemID &&
					 j == m_iEditModeColumn )
				{

					m_hEditModePanel->SetPos( x + m_iTableStartX + 2, (drawcount * m_iRowHeight) + m_iTableStartY);
					m_hEditModePanel->SetSize( wide, m_iRowHeight - 1 );

					bDone = true;
				}

				x += wide;
			}

			drawcount++;
		}
	}

	Repaint();
	m_iColumnDraggerMoved = -1; // reset to invalid column
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Renders the cells
//-----------------------------------------------------------------------------
void ListPanel::Paint()
{
	if (m_bNeedsSort)
	{
		SortList();
	}

	// draw selection areas if any
	int wide, tall;
  	GetSize( wide, tall );

	m_iTableStartX = 0; 
	m_iTableStartY = m_iHeaderHeight + 1;

	int nTotalRows = m_VisibleItems.Count();
	int nRowsPerPage = GetRowsPerPage();

	// find the first visible item to display
	int nStartItem = 0;
	if (nRowsPerPage <= nTotalRows)
	{
		nStartItem = m_vbar->GetValue();
	}

	int vbarInset = m_vbar->IsVisible() ? m_vbar->GetWide() : 0;
	int maxw = wide - vbarInset - 8;

//	debug timing functions
//	double startTime, endTime;
//	startTime = system()->GetCurrentTime();

	// iterate through and draw each cell
	bool bDone = false;
	int drawcount = 0;
	for (int i = nStartItem; i < nTotalRows && !bDone; i++)
	{
		int x = 0;
		if (!m_VisibleItems.IsValidIndex(i))
			continue;

		int itemID = m_VisibleItems[i];
		
		// iterate the columns
		for (int j = 0; j < m_CurrentColumns.Count(); j++)
		{
			Panel *header = m_ColumnsData[m_CurrentColumns[j]].m_pHeader;
			Panel *render = GetCellRenderer(itemID, j);

			if (!header->IsVisible())
				continue;

			int wide = header->GetWide();

			if (render)
			{
				// setup render panel
				if (render->GetVParent() != GetVPanel())
				{
					render->SetParent(GetVPanel());
				}
				if (!render->IsVisible())
				{
					render->SetVisible(true);
				}
				int xpos = x + m_iTableStartX + 2;

				render->SetPos( xpos, (drawcount * m_iRowHeight) + m_iTableStartY);

				int right = min( xpos + wide, maxw );
				int usew = right - xpos;
				render->SetSize( usew, m_iRowHeight - 1 );

				// mark the panel to draw immediately (since it will probably be recycled to draw other cells)
				render->Repaint();
				surface()->SolveTraverse(render->GetVPanel());
				int x0, y0, x1, y1;
				render->GetClipRect(x0, y0, x1, y1);
				if ((y1 - y0) < (m_iRowHeight - 3))
				{
					bDone = true;
					break;
				}
				surface()->PaintTraverse(render->GetVPanel());
			}
			/*
			// work in progress, optimized paint for text
			else
			{
				// just paint it ourselves
				char tempText[256];
				// Grab cell text
				GetCellText(i, j, tempText, sizeof(tempText));
				surface()->DrawSetTextPos(x + m_iTableStartX + 2, (drawcount * m_iRowHeight) + m_iTableStartY);

				for (const char *pText = tempText; *pText != 0; pText++)
				{
					surface()->DrawUnicodeChar((wchar_t)*pText);
				}
			}
			*/

			x += wide;
		}

		drawcount++;
	}

	m_pLabel->SetVisible(false);

	// if the list is empty, draw some help text
	if (m_VisibleItems.Count() < 1 && m_pEmptyListText)
	{
		m_pEmptyListText->SetPos(m_iTableStartX + 8, m_iTableStartY + 4);
		m_pEmptyListText->SetSize(wide - 8, m_iRowHeight);
		m_pEmptyListText->Paint();
	}

//	endTime = system()->GetCurrentTime();
//	ivgui()->DPrintf2("ListPanel::Paint() (%.3f sec)\n", (float)(endTime - startTime));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::PaintBackground()
{
	BaseClass::PaintBackground();
}


//-----------------------------------------------------------------------------
// Handles multiselect 
//-----------------------------------------------------------------------------
void ListPanel::HandleMultiSelection( int itemID, int row, int column )
{
	// deal with 'multiple' row selection

	// convert the last item selected to a row so we can multiply select by rows NOT items
	int lastSelectedRow = (m_LastItemSelected != -1) ? m_VisibleItems.Find( m_LastItemSelected ) : row;
	int startRow, endRow;
	if ( row < lastSelectedRow )
	{
		startRow = row;
		endRow = lastSelectedRow;
	}
	else
	{
		startRow = lastSelectedRow;
		endRow = row;
	}

	// clear the selection if neither control key was down - we are going to readd ALL selected items
	// in case the user changed the 'direction' of the shift add
	if ( !input()->IsKeyDown(KEY_LCONTROL) && !input()->IsKeyDown(KEY_RCONTROL) )
	{
		ClearSelectedItems();
	}

	// add any items that we haven't added
	for (int i = startRow; i <= endRow; i++)
	{
		// get the item indexes for these rows
		int selectedItemID = m_VisibleItems[i];
		if ( !m_SelectedItems.HasElement(selectedItemID) )
		{
			AddSelectedItem( selectedItemID );
		}
	}
}


//-----------------------------------------------------------------------------
// Handles multiselect 
//-----------------------------------------------------------------------------
void ListPanel::HandleAddSelection( int itemID, int row, int column )
{
	// dealing with row selection
	if ( m_SelectedItems.HasElement( itemID ) )
	{
		// this row is already selected, remove
		m_SelectedItems.FindAndRemove( itemID );
		PostActionSignal( new KeyValues("ItemDeselected") );
		m_LastItemSelected = itemID;
	}
	else
	{
		// add the row to the selection
		AddSelectedItem( itemID );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::UpdateSelection( MouseCode code, int x, int y, int row, int column )
{
	// make sure we're clicking on a real item
	if ( row < 0 || row >= m_VisibleItems.Count() )
	{
		ClearSelectedItems();
		return;
	}

	int itemID = m_VisibleItems[ row ];

	// if we've right-clicked on a selection, don't change the selection
	if ( code == MOUSE_RIGHT && m_SelectedItems.HasElement( itemID ) )
		return;

	if ( m_bCanSelectIndividualCells )
	{
		if ( input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
		{
			// we're ctrl selecting the same cell, clear it
			if ( ( m_LastItemSelected == itemID ) && ( m_iSelectedColumn == column ) && ( m_SelectedItems.Count() == 1 ) )
			{
				ClearSelectedItems();
			}
			else
			{
				SetSelectedCell( itemID, column );
			}
		}
		else
		{
			SetSelectedCell( itemID, column );
		}
		return;
	}

	if ( !m_bMultiselectEnabled )
	{
		SetSingleSelectedItem( itemID );
		return;
	}

	if ( input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT) ) 
	{
		// check for multi-select
		HandleMultiSelection( itemID, row, column );
	}
	else if ( input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
	{
		// check for row-add select
		HandleAddSelection( itemID, row, column );
	}
	else
	{
		// no CTRL or SHIFT keys
		// reset the selection Start point
//			if ( ( m_LastItemSelected != itemID ) || ( m_SelectedItems.Count() > 1 ) )
		{
			SetSingleSelectedItem( itemID );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::OnMousePressed( MouseCode code )
{
	if (code == MOUSE_LEFT || code == MOUSE_RIGHT)
	{
		if ( m_VisibleItems.Count() > 0 )
		{
			// determine where we were pressed
			int x, y, row, column;
			input()->GetCursorPos(x, y);
			GetCellAtPos(x, y, row, column);

			UpdateSelection( code, x, y, row, column );
		}

		// get the key focus
		RequestFocus();
	}

	// check for context menu open
	if (code == MOUSE_RIGHT)
	{
		if ( m_SelectedItems.Count() > 0 )
		{
			PostActionSignal( new KeyValues("OpenContextMenu", "itemID", m_SelectedItems[0] ));
		}
		else
		{
			// post it, but with the invalid row
			PostActionSignal( new KeyValues("OpenContextMenu", "itemID", -1 ));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void ListPanel::OnMouseWheeled(int delta)
{
	if (m_hEditModePanel.Get())
	{
		// ignore mouse wheel in edit mode, forward right up to parent
		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
		return;
	}

	int val = m_vbar->GetValue();
	val -= (delta * 3);
	m_vbar->SetValue(val);
}

//-----------------------------------------------------------------------------
// Purpose: Double-click act like the the item under the mouse was selected
//			and then the enter key hit
//-----------------------------------------------------------------------------
void ListPanel::OnMouseDoublePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		// select the item
		OnMousePressed(code);

		// post up an enter key being hit if anything was selected
		if (GetSelectedItemsCount() > 0 && !m_bIgnoreDoubleClick )
		{
			OnKeyCodeTyped(KEY_ENTER);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef _X360
void ListPanel::OnKeyCodePressed(KeyCode code)
{
	int nTotalRows = m_VisibleItems.Count();
	int nTotalColumns = m_CurrentColumns.Count();
	if ( nTotalRows == 0 )
		return;

	// calculate info for adjusting scrolling
	int nStartItem = GetStartItem();
	int nRowsPerPage = (int)GetRowsPerPage();

	int nSelectedRow = 0;
	if ( m_DataItems.IsValidIndex( m_LastItemSelected ) )
	{
		nSelectedRow = m_VisibleItems.Find( m_LastItemSelected );
	}
 	int nSelectedColumn = m_iSelectedColumn;

	switch(code)
	{
	case KEY_XBUTTON_UP:
	case KEY_XSTICK1_UP:
	case KEY_XSTICK2_UP:
		if(GetItemCount() < 1 || nSelectedRow == nStartItem)
		{
			ClearSelectedItems();
			BaseClass::OnKeyCodePressed(code);
			return;
		}
		else
		{
			nSelectedRow -= 1;
		}
		break;
	case KEY_XBUTTON_DOWN:
	case KEY_XSTICK1_DOWN:
	case KEY_XSTICK2_DOWN:
		{
			int itemId = GetSelectedItem(0);
			if(itemId != -1 && GetItemCurrentRow(itemId) == (nTotalRows - 1))
			{
				ClearSelectedItems();
				BaseClass::OnKeyCodePressed(code);
				return;
			}
			else
			{
				nSelectedRow += 1;
			}
		}
		break;
	case KEY_XBUTTON_LEFT:
	case KEY_XSTICK1_LEFT:
	case KEY_XSTICK2_LEFT:
		if (m_bCanSelectIndividualCells && (GetSelectedItemsCount() == 1) && (nSelectedColumn >= 0) )
		{
			nSelectedColumn--;
			if (nSelectedColumn < 0)
			{
				nSelectedColumn = 0;
			}
			break;
		}
		break;
	case KEY_XBUTTON_RIGHT:
	case KEY_XSTICK1_RIGHT:
	case KEY_XSTICK2_RIGHT:
		if (m_bCanSelectIndividualCells && (GetSelectedItemsCount() == 1) && (nSelectedColumn >= 0) )
		{
			nSelectedColumn++;
			if (nSelectedColumn >= nTotalColumns)
			{
				nSelectedColumn = nTotalColumns - 1;
			}
			break;
		}
		break;
	case KEY_XBUTTON_A:
		PostActionSignal( new KeyValues("ListPanelItemChosen", "itemID", m_SelectedItems[0] ));
		break;
	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}

	// make sure newly selected item is a valid range
	nSelectedRow = clamp(nSelectedRow, 0, nTotalRows - 1);

	int row = m_VisibleItems[ nSelectedRow ];

	// This will select the cell if in single select mode, or the row in multiselect mode
	if ( ( row != m_LastItemSelected ) || ( nSelectedColumn != m_iSelectedColumn ) || ( m_SelectedItems.Count() > 1 ) )
	{
		SetSelectedCell( row, nSelectedColumn );
	}

	// move the newly selected item to within the visible range
	if ( nRowsPerPage < nTotalRows )
	{
		int nStartItem = m_vbar->GetValue();
		if ( nSelectedRow < nStartItem )
		{
			// move the list back to match
			m_vbar->SetValue( nSelectedRow );
		}
		else if ( nSelectedRow >= nStartItem + nRowsPerPage )
		{
			// move list forward to match
			m_vbar->SetValue( nSelectedRow - nRowsPerPage + 1);
		}
	}

	// redraw
	InvalidateLayout();
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::OnKeyCodePressed(KeyCode code)
{
	if (m_hEditModePanel.Get())
	{
		// ignore arrow keys in edit mode
		// forward right up to parent so that tab focus change doesn't occur
		CallParentFunction(new KeyValues("KeyCodePressed", "code", code));
		return;
	}

	int nTotalRows = m_VisibleItems.Count();
	int nTotalColumns = m_CurrentColumns.Count();
	if ( nTotalRows == 0 )
	{
		BaseClass::OnKeyCodePressed(code);
		return;
	}

	// calculate info for adjusting scrolling
	int nStartItem = GetStartItem();
	int nRowsPerPage = (int)GetRowsPerPage();

	int nSelectedRow = 0;
	if ( m_DataItems.IsValidIndex( m_LastItemSelected ) )
	{
		nSelectedRow = m_VisibleItems.Find( m_LastItemSelected );
	}
 	int nSelectedColumn = m_iSelectedColumn;

	switch (code)
	{
	case KEY_HOME:
		nSelectedRow = 0;
		break;

	case KEY_END:
		nSelectedRow = nTotalRows - 1;
		break;

	case KEY_PAGEUP:
		if (nSelectedRow <= nStartItem)
		{
			// move up a page
			nSelectedRow -= (nRowsPerPage - 1);
		}
		else
		{
			// move to the top of the current page
			nSelectedRow = nStartItem;
		}
		break;

	case KEY_PAGEDOWN:
		if (nSelectedRow >= (nStartItem + nRowsPerPage-1))
		{
			// move down a page
			nSelectedRow += (nRowsPerPage - 1);
		}
		else
		{
			// move to the bottom of the current page
			nSelectedRow = nStartItem + (nRowsPerPage - 1);
		}
		break;

	case KEY_UP:
	case KEY_XBUTTON_UP:
	case KEY_XSTICK1_UP:
	case KEY_XSTICK2_UP:
		if ( nTotalRows > 0 )
		{
			nSelectedRow--;
			break;
		}
		// fall through

	case KEY_DOWN:
	case KEY_XBUTTON_DOWN:
	case KEY_XSTICK1_DOWN:
	case KEY_XSTICK2_DOWN:
		if ( nTotalRows > 0 )
		{
			nSelectedRow++;
			break;
		}
		// fall through

	case KEY_LEFT:
	case KEY_XBUTTON_LEFT:
	case KEY_XSTICK1_LEFT:
	case KEY_XSTICK2_LEFT:
		if (m_bCanSelectIndividualCells && (GetSelectedItemsCount() == 1) && (nSelectedColumn >= 0) )
		{
			nSelectedColumn--;
			if (nSelectedColumn < 0)
			{
				nSelectedColumn = 0;
			}
			break;
		}
		// fall through

	case KEY_RIGHT:
	case KEY_XBUTTON_RIGHT:
	case KEY_XSTICK1_RIGHT:
	case KEY_XSTICK2_RIGHT:
		if (m_bCanSelectIndividualCells && (GetSelectedItemsCount() == 1) && (nSelectedColumn >= 0) )
		{
			nSelectedColumn++;
			if (nSelectedColumn >= nTotalColumns)
			{
				nSelectedColumn = nTotalColumns - 1;
			}
			break;
		}
		// fall through

	default:
		// chain back
		BaseClass::OnKeyCodePressed(code);
		return;
	};

	// make sure newly selected item is a valid range
	nSelectedRow = clamp(nSelectedRow, 0, nTotalRows - 1);

	int row = m_VisibleItems[ nSelectedRow ];

	// This will select the cell if in single select mode, or the row in multiselect mode
	if ( ( row != m_LastItemSelected ) || ( nSelectedColumn != m_iSelectedColumn ) || ( m_SelectedItems.Count() > 1 ) )
	{
		SetSelectedCell( row, nSelectedColumn );
	}

	// move the newly selected item to within the visible range
	if ( nRowsPerPage < nTotalRows )
	{
		int nStartItem = m_vbar->GetValue();
		if ( nSelectedRow < nStartItem )
		{
			// move the list back to match
			m_vbar->SetValue( nSelectedRow );
		}
		else if ( nSelectedRow >= nStartItem + nRowsPerPage )
		{
			// move list forward to match
			m_vbar->SetValue( nSelectedRow - nRowsPerPage + 1);
		}
	}

	// redraw
	InvalidateLayout();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ListPanel::GetCellBounds( int row, int col, int& x, int& y, int& wide, int& tall )
{
	if ( col < 0 || col >= m_CurrentColumns.Count() )
		return false;

	if ( row < 0 || row >= m_VisibleItems.Count() )
		return false;

	// Is row on screen?
	int startitem = GetStartItem();
	if ( row < startitem || row >= ( startitem + GetRowsPerPage() ) )
		return false;

	y = m_iTableStartY;
	y += ( row - startitem ) * m_iRowHeight;
	tall = m_iRowHeight;

	// Compute column cell
	x = m_iTableStartX;
	// walk columns
	int c = 0;
	while ( c < col)
	{
		x += m_ColumnsData[m_CurrentColumns[c]].m_pHeader->GetWide();
		c++;
	}
	wide = m_ColumnsData[m_CurrentColumns[c]].m_pHeader->GetWide();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if any found, row and column are filled out
//-----------------------------------------------------------------------------
bool ListPanel::GetCellAtPos(int x, int y, int &row, int &col)
{
	// convert to local
	ScreenToLocal(x, y);

	// move to Start of table
	x -= m_iTableStartX;
	y -= m_iTableStartY;

	int startitem = GetStartItem();
	// make sure it's still in valid area
	if ( x >= 0 && y >= 0 )
	{
		// walk the rows (for when row height is independant each row)  
		// NOTE: if we do height independent rows, we will need to change GetCellBounds as well
		for ( row = startitem ; row < m_VisibleItems.Count() ; row++ )
		{
			if ( y < ( ( ( row - startitem ) + 1 ) * m_iRowHeight ) )
				break;
		}

		// walk columns
		int startx = 0;
		for ( col = 0 ; col < m_CurrentColumns.Count() ; col++ )
		{
			startx += m_ColumnsData[m_CurrentColumns[col]].m_pHeader->GetWide();

			if ( x < startx )
				break;
		}

		// make sure we're not out of range
		if ( ! ( row == m_VisibleItems.Count() || col == m_CurrentColumns.Count() ) )
		{
			return true;
		}
	}

	// out-of-bounds
	row = col = -1;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::ApplySchemeSettings(IScheme *pScheme)
{
	// force label to apply scheme settings now so we can override it
	m_pLabel->InvalidateLayout(true);

	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(GetSchemeColor("ListPanel.BgColor", pScheme));
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));

	m_pLabel->SetBgColor(GetSchemeColor("ListPanel.BgColor", pScheme));

	m_LabelFgColor = GetSchemeColor("ListPanel.TextColor", pScheme);
	m_DisabledColor = GetSchemeColor("ListPanel.DisabledTextColor", m_LabelFgColor, pScheme);
	m_SelectionFgColor = GetSchemeColor("ListPanel.SelectedTextColor", m_LabelFgColor, pScheme);
	m_DisabledSelectionFgColor = GetSchemeColor("ListPanel.DisabledSelectedTextColor", m_LabelFgColor, pScheme);

	m_pEmptyListText->SetColor(GetSchemeColor("ListPanel.EmptyListInfoTextColor", pScheme));
		
	SetFont( pScheme->GetFont("Default", IsProportional() ) );
	m_pEmptyListText->SetFont( pScheme->GetFont( "Default", IsProportional() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetSortFunc(int col, SortFunc *func)
{
	Assert(col < m_CurrentColumns.Count());
	unsigned char dataColumnIndex = m_CurrentColumns[col];

	if ( !m_ColumnsData[dataColumnIndex].m_bTypeIsText && func != NULL)
	{
		m_ColumnsData[dataColumnIndex].m_pHeader->SetMouseClickEnabled(MOUSE_LEFT, 1);
	}

	m_ColumnsData[dataColumnIndex].m_pSortFunc = func;

	// resort this column according to new sort func
    ResortColumnRBTree(col);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetSortColumn(int column)
{
	m_iSortColumn = column;
}

int ListPanel::GetSortColumn() const
{
	return m_iSortColumn;
}

void ListPanel::SetSortColumnEx( int iPrimarySortColumn, int iSecondarySortColumn, bool bSortAscending )
{
	m_iSortColumn = iPrimarySortColumn;
	m_iSortColumnSecondary = iSecondarySortColumn;
	m_bSortAscending = bSortAscending;
}

void ListPanel::GetSortColumnEx( int &iPrimarySortColumn, int &iSecondarySortColumn, bool &bSortAscending ) const
{
	iPrimarySortColumn = m_iSortColumn;
	iSecondarySortColumn = m_iSortColumnSecondary;
	bSortAscending = m_bSortAscending;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SortList( void )
{
	m_bNeedsSort = false;

	if ( m_VisibleItems.Count() <= 1 )
	{
		return;
	}

	// check if the last selected item is on the screen - if so, we should try to maintain it on screen 
	int startItem = GetStartItem();
	int rowsperpage = (int) GetRowsPerPage();
	int screenPosition = -1;
	if ( m_LastItemSelected != -1 && m_SelectedItems.Count() > 0 )
	{
		int selectedItemRow = m_VisibleItems.Find(m_LastItemSelected);
		if ( selectedItemRow >= startItem && selectedItemRow <= ( startItem + rowsperpage ) )
		{
			screenPosition = selectedItemRow - startItem;
		}
	}

	// get the required sorting functions
	s_pCurrentSortingListPanel = this;

	// setup globals for use in qsort
	s_pSortFunc = FastSortFunc;
	s_bSortAscending = m_bSortAscending;
	s_pSortFuncSecondary = FastSortFunc;
	s_bSortAscendingSecondary = m_bSortAscendingSecondary;

	// walk the tree and set up the current indices
	if (m_CurrentColumns.IsValidIndex(m_iSortColumn))
	{
		IndexRBTree_t &rbtree = m_ColumnsData[m_CurrentColumns[m_iSortColumn]].m_SortedTree;
		unsigned int index = rbtree.FirstInorder();
		unsigned int lastIndex = rbtree.LastInorder();
		int prevDuplicateIndex = 0;
		int sortValue = 1;
		while (1)
		{
			FastSortListPanelItem *dataItem = (FastSortListPanelItem*) rbtree[index].dataItem;
			if (dataItem->visible)
			{
				// only increment the sort value if we're a different token from the previous
				if (!prevDuplicateIndex || prevDuplicateIndex != rbtree[index].duplicateIndex)
				{
					sortValue++;
				}
				dataItem->primarySortIndexValue = sortValue;
				prevDuplicateIndex = rbtree[index].duplicateIndex;
			}

			if (index == lastIndex)
				break;

			index = rbtree.NextInorder(index);
		}
	}

	// setup secondary indices
	if (m_CurrentColumns.IsValidIndex(m_iSortColumnSecondary))
	{
		IndexRBTree_t &rbtree = m_ColumnsData[m_CurrentColumns[m_iSortColumnSecondary]].m_SortedTree;
		unsigned int index = rbtree.FirstInorder();
		unsigned int lastIndex = rbtree.LastInorder();
		int sortValue = 1;
		int prevDuplicateIndex = 0;
		while (1)
		{
			FastSortListPanelItem *dataItem = (FastSortListPanelItem*) rbtree[index].dataItem;
			if (dataItem->visible)
			{
				// only increment the sort value if we're a different token from the previous
				if (!prevDuplicateIndex || prevDuplicateIndex != rbtree[index].duplicateIndex)
				{
					sortValue++;
				}
				dataItem->secondarySortIndexValue = sortValue;

				prevDuplicateIndex = rbtree[index].duplicateIndex;
			}

			if (index == lastIndex)
				break;

			index = rbtree.NextInorder(index);
		}
	}

	// quick sort the list
	qsort(m_VisibleItems.Base(), (size_t) m_VisibleItems.Count(), (size_t) sizeof(int), AscendingSortFunc);

	if ( screenPosition != -1 )
	{
		int selectedItemRow = m_VisibleItems.Find(m_LastItemSelected);

		// if we can put the last selected item in exactly the same spot, put it there, otherwise
		// we need to be at the top of the list
		if (selectedItemRow > screenPosition)
		{
			m_vbar->SetValue(selectedItemRow - screenPosition);
		}
		else
		{
			m_vbar->SetValue(0);
		}
	}

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::SetFont(HFont font)
{
	Assert( font );
	if ( !font )
		return;

	m_pTextImage->SetFont(font);
	m_iRowHeight = surface()->GetFontTall(font) + 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListPanel::OnSliderMoved()
{
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : deltax - deltas from current position
//-----------------------------------------------------------------------------
void ListPanel::OnColumnResized(int col, int delta)
{
	m_iColumnDraggerMoved = col;

	column_t& column = m_ColumnsData[m_CurrentColumns[col]];

	Panel *header = column.m_pHeader;
	int wide, tall;
	header->GetSize(wide, tall);


	wide += delta;

	// enforce minimum sizes for the header
	if ( wide < column.m_iMinWidth )
	{
		wide = column.m_iMinWidth;
	}
	// enforce maximum sizes for the header
	if ( wide > column.m_iMaxWidth )
	{
		wide = column.m_iMaxWidth;
	}

	// make sure we have enough space for the columns to our right
	int panelWide, panelTall;
	GetSize( panelWide, panelTall );
	int x, y;
	header->GetPos(x, y);
	int restColumnsMinWidth = 0;
	int i;
	for ( i = col+1 ; i < m_CurrentColumns.Count() ; i++ )
	{
		column_t& nextCol = m_ColumnsData[m_CurrentColumns[i]];
		restColumnsMinWidth += nextCol.m_iMinWidth;
	}
	panelWide -= ( x + restColumnsMinWidth + m_vbar->GetWide() + WINDOW_BORDER_WIDTH );
	if ( wide > panelWide )
	{
		wide = panelWide;
	}

	header->SetSize(wide, tall);

	// the adjacent header will be moved automatically in PerformLayout()
	header->InvalidateLayout();
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: sets which column we should sort with
//-----------------------------------------------------------------------------
void ListPanel::OnSetSortColumn(int column)
{
	// if it's the primary column already, flip the sort direction
	if (m_iSortColumn == column)
	{
		m_bSortAscending = !m_bSortAscending;
	}
	else
	{
		// switching sort columns, keep the old one as the secondary sort
		m_iSortColumnSecondary = m_iSortColumn;
		m_bSortAscendingSecondary = m_bSortAscending;
	}

	SetSortColumn(column);

	SortList();
}

//-----------------------------------------------------------------------------
// Purpose: sets whether the item is visible or not
//-----------------------------------------------------------------------------
void ListPanel::SetItemVisible(int itemID, bool state)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	FastSortListPanelItem *data = (FastSortListPanelItem*) m_DataItems[itemID];
	if (data->visible == state)
		return;

	m_bNeedsSort = true;

	data->visible = state;
	if (data->visible)
	{
		// add back to end of list
		m_VisibleItems.AddToTail(itemID);
	}
	else
	{
		// remove from selection if it is there.
		if (m_SelectedItems.HasElement(itemID))
		{
			m_SelectedItems.FindAndRemove(itemID);
			PostActionSignal( new KeyValues("ItemDeselected") );
		}

		// remove from data
		m_VisibleItems.FindAndRemove(itemID);
	
		InvalidateLayout();
	}
}


//-----------------------------------------------------------------------------
// Is the item visible?
//-----------------------------------------------------------------------------
bool ListPanel::IsItemVisible( int itemID )
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return false;

	FastSortListPanelItem *data = (FastSortListPanelItem*) m_DataItems[itemID];
	return data->visible;
}

	
//-----------------------------------------------------------------------------
// Purpose: sets whether the item is disabled or not (effects item color)
//-----------------------------------------------------------------------------
void ListPanel::SetItemDisabled(int itemID, bool state)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	m_DataItems[itemID]->kv->SetInt( "disabled", state );
}

//-----------------------------------------------------------------------------
// Purpose: Calculate number of rows per page 
//-----------------------------------------------------------------------------
float ListPanel::GetRowsPerPage()
{
	float rowsperpage = (float)( GetTall() - m_iHeaderHeight ) / (float)m_iRowHeight;
	return rowsperpage;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the item we should Start on
//-----------------------------------------------------------------------------
int ListPanel::GetStartItem()
{
	// if rowsperpage < total number of rows
	if ( GetRowsPerPage() < (float) m_VisibleItems.Count() )
	{
		return m_vbar->GetValue();
	}
	return 0;	// otherwise Start at top
}

//-----------------------------------------------------------------------------
// Purpose: whether or not to select specific cells (off by default)
//-----------------------------------------------------------------------------
void ListPanel::SetSelectIndividualCells(bool state)
{
	m_bCanSelectIndividualCells = state;
}


//-----------------------------------------------------------------------------
// whether or not multiple cells/rows can be selected
//-----------------------------------------------------------------------------
void ListPanel::SetMultiselectEnabled( bool bState )
{
	m_bMultiselectEnabled = bState;
}

bool ListPanel::IsMultiselectEnabled() const
{
	return m_bMultiselectEnabled;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text which is displayed when the list is empty
//-----------------------------------------------------------------------------
void ListPanel::SetEmptyListText(const char *text)
{
	m_pEmptyListText->SetText(text);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text which is displayed when the list is empty
//-----------------------------------------------------------------------------
void ListPanel::SetEmptyListText(const wchar_t *text)
{
	m_pEmptyListText->SetText(text);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: opens the content menu
//-----------------------------------------------------------------------------
void ListPanel::OpenColumnChoiceMenu()
{
	if (!m_bAllowUserAddDeleteColumns)
		return;

	Menu *menu = new Menu(this, "ContextMenu");

	int x, y;
	input()->GetCursorPos(x, y);
	menu->SetPos(x, y);

	// add all the column choices to the menu
	for ( int i = 0 ; i < m_CurrentColumns.Count() ; i++ )
	{
		column_t &column = m_ColumnsData[m_CurrentColumns[i]];

		char name[128];
		column.m_pHeader->GetText(name, sizeof(name));
		int itemID = menu->AddCheckableMenuItem(name, new KeyValues("ToggleColumnVisible", "col", m_CurrentColumns[i]), this);
		menu->SetMenuItemChecked(itemID, !column.m_bHidden);

		if (column.m_bUnhidable)
		{
			menu->SetItemEnabled(itemID, false);
		}
	}

	menu->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Resizes a column
//-----------------------------------------------------------------------------
void ListPanel::ResizeColumnToContents(int column)
{
	// iterate all the items in the column, getting the size of each
	column_t &col = m_ColumnsData[m_CurrentColumns[column]];

	if (!col.m_bTypeIsText)
		return;

	// start with the size of the column text
	int wide = 0, minRequiredWidth = 0, tall = 0;
	col.m_pHeader->GetContentSize( minRequiredWidth, tall );

	// iterate every item
	for (int i = 0; i < m_VisibleItems.Count(); i++)
	{
		if (!m_VisibleItems.IsValidIndex(i))
			continue;

		// get the cell
		int itemID = m_VisibleItems[i];

		// get the text
		wchar_t tempText[ 256 ];
		GetCellText( itemID, column, tempText, 256 );
		m_pTextImage->SetText(tempText);

		m_pTextImage->GetContentSize(wide, tall);

		if ( wide > minRequiredWidth )
		{
			minRequiredWidth = wide;
		}
	}

	// Introduce a slight buffer between columns
	minRequiredWidth += 4;

	// call the resize
	col.m_pHeader->GetSize(wide, tall);
	OnColumnResized(column, minRequiredWidth - wide);
}

//-----------------------------------------------------------------------------
// Purpose: Changes the visibilty of a column
//-----------------------------------------------------------------------------
void ListPanel::OnToggleColumnVisible(int col)
{
	if (!m_CurrentColumns.IsValidIndex(col))
		return;

	// toggle the state of the column
	column_t &column = m_ColumnsData[m_CurrentColumns[col]];
	SetColumnVisible(col, column.m_bHidden);
}

//-----------------------------------------------------------------------------
// Purpose: sets user settings
//-----------------------------------------------------------------------------
void ListPanel::ApplyUserConfigSettings(KeyValues *userConfig)
{
	// Check for version mismatch, then don't load settings.  (Just revert to the defaults.)
	int version = userConfig->GetInt( "configVersion", 1 );
	if ( version != m_nUserConfigFileVersion )
	{
		return;
	}

	// We save/restore m_lastBarWidth because all of the column widths are saved relative to that size.
	// If we don't save it, you can run into this case:
	//    - Window width is 500, load sizes setup relative to a 1000-width window
	//	  - Set window size to 1000
	//    - In PerformLayout, it thinks the window has grown by 500 (since m_lastBarWidth is 500 and new window width is 1000)
	//      so it pushes out any COLUMN_RESIZEWITHWINDOW columns to their max extent and shrinks everything else to its min extent.
	m_lastBarWidth = userConfig->GetInt( "lastBarWidth", 0 );
	
	// read which columns are hidden
	for ( int i = 0; i < m_CurrentColumns.Count(); i++ )
	{
		char name[64];
		_snprintf(name, sizeof(name), "%d_hidden", i);

		int hidden = userConfig->GetInt(name, -1);
		if (hidden == 0)
		{
			SetColumnVisible(i, true);
		}
		else if (hidden == 1)
		{
			SetColumnVisible(i, false);
		}

		_snprintf(name, sizeof(name), "%d_width", i);
		int nWidth = userConfig->GetInt( name, -1 );
		if ( nWidth >= 0 )
		{
			column_t &column = m_ColumnsData[m_CurrentColumns[i]];
			column.m_pHeader->SetWide( nWidth );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns user config settings for this control
//-----------------------------------------------------------------------------
void ListPanel::GetUserConfigSettings(KeyValues *userConfig)
{
	if ( m_nUserConfigFileVersion != 1 )
	{
		userConfig->SetInt( "configVersion", m_nUserConfigFileVersion );
	}

	userConfig->SetInt( "lastBarWidth", m_lastBarWidth );

	// save which columns are hidden
	for ( int i = 0 ; i < m_CurrentColumns.Count() ; i++ )
	{
		column_t &column = m_ColumnsData[m_CurrentColumns[i]];

		char name[64];
		_snprintf(name, sizeof(name), "%d_hidden", i);
		userConfig->SetInt(name, column.m_bHidden ? 1 : 0);

		_snprintf(name, sizeof(name), "%d_width", i);
		userConfig->SetInt( name, column.m_pHeader->GetWide() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: optimization, return true if this control has any user config settings
//-----------------------------------------------------------------------------
bool ListPanel::HasUserConfigSettings()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void ListPanel::SetAllowUserModificationOfColumns(bool allowed)
{
	m_bAllowUserAddDeleteColumns = allowed;
}

void ListPanel::SetIgnoreDoubleClick( bool state )
{
	m_bIgnoreDoubleClick = state;
}

//-----------------------------------------------------------------------------
// Purpose: set up a field for editing
//-----------------------------------------------------------------------------
void ListPanel::EnterEditMode(int itemID, int column, vgui::Panel *editPanel)
{
	m_hEditModePanel = editPanel;
	m_iEditModeItemID = itemID;
	m_iEditModeColumn = column;
	editPanel->SetParent(this);
	editPanel->SetVisible(true);
	editPanel->RequestFocus();
	editPanel->MoveToFront();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: leaves editing mode
//-----------------------------------------------------------------------------
void ListPanel::LeaveEditMode()
{
	if (m_hEditModePanel.Get())
	{
		m_hEditModePanel->SetVisible(false);
		m_hEditModePanel->SetParent((Panel *)NULL);
		m_hEditModePanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are currently in inline editing mode
//-----------------------------------------------------------------------------
bool ListPanel::IsInEditMode()
{
	return (m_hEditModePanel.Get() != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef _X360
void ListPanel::NavigateTo()
{
	BaseClass::NavigateTo();
	// attempt to select the first item in the list when we get focus
	if(GetItemCount())
	{
		SetSingleSelectedItem(FirstItem());
	}
	else // if we have no items, change focus
	{
		if(!NavigateDown())
		{
			NavigateUp();
		}
	}
}
#endif
