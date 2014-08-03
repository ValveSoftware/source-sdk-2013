//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>
#include <ctype.h>

#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ILocalize.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/ListViewPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;
	
enum 
{
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

namespace vgui
{
class ListViewItem : public Label
{
	DECLARE_CLASS_SIMPLE( ListViewItem, Label );

public:
	ListViewItem(Panel *parent) : Label(parent, NULL, "")
	{
		m_pListViewPanel = (ListViewPanel*) parent;
		m_pData = NULL;
		m_bSelected = false;
		SetPaintBackgroundEnabled(true);
	}

	~ListViewItem()
	{
		if (m_pData)
		{
			m_pData->deleteThis();
			m_pData = NULL;
		}
	}

	void SetData(const KeyValues *data)
	{
		if (m_pData)
		{
			m_pData->deleteThis();
		}
		m_pData = data->MakeCopy();
	}

	virtual void OnMousePressed( MouseCode code)
	{
		m_pListViewPanel->OnItemMousePressed(this, code);
	}

	virtual void OnMouseDoublePressed( MouseCode code)
	{
		// double press should only select the item
		m_pListViewPanel->OnItemMouseDoublePressed(this, code);
	}

	KeyValues *GetData()
	{ 
		return m_pData;
	}

	void SetSelected(bool bSelected)
	{
		if (bSelected == m_bSelected)
			return;

		m_bSelected = bSelected;
		if (bSelected)
		{
			RequestFocus();
		}

		UpdateImage();
		InvalidateLayout();
		Repaint();
	}

	virtual void PerformLayout()
	{
		TextImage *textImage = GetTextImage();
		if (m_bSelected)
		{
			VPANEL focus = input()->GetFocus();
			// if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
			if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())))
			{
				textImage->SetColor(m_ArmedFgColor2);
			}
			else
			{
				textImage->SetColor(m_FgColor2);
			}
		}
		else
		{
			textImage->SetColor(GetFgColor());					
		}
		BaseClass::PerformLayout();
		Repaint();
	}

	virtual void PaintBackground()	
	{
		int wide, tall;
		GetSize(wide, tall);

		if ( m_bSelected )
		{
            VPANEL focus = input()->GetFocus();
            // if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
            if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())))
            {
			    surface()->DrawSetColor(m_ArmedBgColor);
            }
            else
            {
			    surface()->DrawSetColor(m_SelectionBG2Color);
            }
		}
		else
		{
			surface()->DrawSetColor(GetBgColor());
		}
		surface()->DrawFilledRect(0, 0, wide, tall);
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		m_ArmedFgColor2 = GetSchemeColor("ListPanel.SelectedTextColor", pScheme);
		m_ArmedBgColor = GetSchemeColor("ListPanel.SelectedBgColor", pScheme);

		m_FgColor1 = GetSchemeColor("ListPanel.TextColor", pScheme);
		m_FgColor2 = GetSchemeColor("ListPanel.SelectedTextColor", pScheme);

		m_BgColor = GetSchemeColor("ListPanel.BgColor", GetBgColor(), pScheme);
		m_BgColor = GetSchemeColor("ListPanel.TextBgColor", m_BgColor, pScheme);
		m_SelectionBG2Color = GetSchemeColor("ListPanel.SelectedOutOfFocusBgColor", pScheme);
		SetBgColor(m_BgColor);
		SetFgColor(m_FgColor1);

		UpdateImage();
	}

	void UpdateImage()
	{
		if ( m_pListViewPanel->m_pImageList )
		{
			int imageIndex = 0;
			if ( m_bSelected )
			{
				imageIndex = m_pData->GetInt("imageSelected", 0);
			}
			if ( imageIndex == 0 )
			{
				imageIndex = m_pData->GetInt("image", 0);
			}
			if ( m_pListViewPanel->m_pImageList->IsValidIndex(imageIndex) )
			{
				SetImageAtIndex(0, m_pListViewPanel->m_pImageList->GetImage(imageIndex), 0);
			}
			else
			{
				// use the default 
				SetImageAtIndex(0, m_pListViewPanel->m_pImageList->GetImage(1), 0);
			}
			SizeToContents();
			InvalidateLayout();
		}
	}

private:

	Color m_FgColor1;
	Color m_FgColor2;
	Color m_BgColor;
	Color m_ArmedFgColor2;
	Color m_ArmedBgColor;
	Color m_SelectionBG2Color;

	//IBorder			  *_keyFocusBorder;		// maybe in the future when I'm the 'active' but not selected item, I'll have a border

	KeyValues 			*m_pData;
	ListViewPanel		*m_pListViewPanel;
	bool				m_bSelected;
};
}

static bool DefaultSortFunc(KeyValues *kv1, KeyValues *kv2)
{
	const char *string1 = kv1->GetString("text");
	const char *string2 = kv2->GetString("text");
	return Q_stricmp(string1, string2) < 0;
}

DECLARE_BUILD_FACTORY( ListViewPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ListViewPanel::ListViewPanel(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	m_iRowHeight = 20;
	m_bNeedsSort = false;
	m_hFont = NULL;
	m_pImageList = NULL;
	m_bDeleteImageListWhenDone = false;
	m_pSortFunc = DefaultSortFunc;
	m_ShiftStartItemID = -1;

	m_hbar = new ScrollBar(this, "HorizScrollBar", false);
	m_hbar->AddActionSignalTarget(this);
	m_hbar->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ListViewPanel::~ListViewPanel()
{
	DeleteAllItems();

	delete m_hbar;

	if ( m_bDeleteImageListWhenDone )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::AddItem(const KeyValues *data, bool bScrollToItem, bool bSortOnAdd)
{
	ListViewItem *pNewItem = new ListViewItem(this);
	pNewItem->SetData(data);
	if (m_hFont)
	{
		pNewItem->SetFont(m_hFont);
	}
	int itemID = m_DataItems.AddToTail(pNewItem);
	ApplyItemChanges(itemID);
	m_SortedItems.AddToTail(itemID);

	if ( bSortOnAdd )
	{
		m_bNeedsSort = true;
	}

	InvalidateLayout();

	if ( bScrollToItem )
	{
		ScrollToItem(itemID);
	}

	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::ScrollToItem(int itemID)
{
	if (!m_hbar->IsVisible())
	{
		return;
	}
	int val = m_hbar->GetValue();

	int wide, tall;
	GetSize( wide, tall );

	int maxWidth = GetItemsMaxWidth();
	int maxColVisible = wide / maxWidth;
	int itemsPerCol = GetItemsPerColumn();

	int itemIndex = m_SortedItems.Find(itemID);
	int desiredCol = itemIndex / itemsPerCol;
	if (desiredCol < val || desiredCol >= (val + maxColVisible) )
	{
		m_hbar->SetValue(desiredCol);
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::GetItemCount()
{
	return m_DataItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *ListViewPanel::GetItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID]->GetData();
}

//-----------------------------------------------------------------------------
// Purpose: Get ItemID from position in panel - valid from [0, GetItemCount)
//-----------------------------------------------------------------------------
int ListViewPanel::GetItemIDFromPos(int iPos)
{
	if ( m_SortedItems.IsValidIndex(iPos) )
	{
		return m_SortedItems[iPos];
	}
	else
	{
		return m_DataItems.InvalidIndex();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::ApplyItemChanges(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;
		
	KeyValues *kv = m_DataItems[itemID]->GetData();
	ListViewItem *pLabel = m_DataItems[itemID];

	pLabel->SetText(kv->GetString("text"));
	pLabel->SetTextImageIndex(1);
	pLabel->SetImagePreOffset(1, 5);

	TextImage *pTextImage = pLabel->GetTextImage();
	pTextImage->ResizeImageToContent();

	pLabel->UpdateImage();
	pLabel->SizeToContents();
	pLabel->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::RemoveItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	m_DataItems[itemID]->MarkForDeletion();

	// mark the keyValues for deletion
	m_DataItems.Remove(itemID);
	m_SortedItems.FindAndRemove(itemID);
	m_SelectedItems.FindAndRemove(itemID);

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::DeleteAllItems()
{
	FOR_EACH_LL( m_DataItems, index )
	{
		m_DataItems[index]->MarkForDeletion();
	}
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();
	m_SelectedItems.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::InvalidItemID()
{
	return m_DataItems.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ListViewPanel::IsValidItemID(int itemID)
{
	return m_DataItems.IsValidIndex(itemID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::SetSortFunc(ListViewSortFunc_t func)
{
	if ( func )
	{
		m_pSortFunc = func;
		SortList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::SortList()
{
	m_SortedItems.RemoveAll();

	// find all the items in this section
	for( int i = m_DataItems.Head(); i != m_DataItems.InvalidIndex(); i = m_DataItems.Next( i ) )
	{
		// insert the items sorted
		if (m_pSortFunc)
		{
			int insertionPoint;
			for (insertionPoint = 0; insertionPoint < m_SortedItems.Count(); insertionPoint++)
			{
				if ( m_pSortFunc(m_DataItems[i]->GetData(), m_DataItems[m_SortedItems[insertionPoint]]->GetData() ) )
					break;
			}
	
			if (insertionPoint == m_SortedItems.Count())
			{
				m_SortedItems.AddToTail(i);
			}
			else
			{
				m_SortedItems.InsertBefore(insertionPoint, i);
			}
		}
		else
		{
			// just add to the end
			m_SortedItems.AddToTail(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::SetImageList(ImageList *imageList, bool deleteImageListWhenDone)
{
	// get rid of existing list image if there's one and we're supposed to get rid of it
	if ( m_pImageList && m_bDeleteImageListWhenDone )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}

	m_bDeleteImageListWhenDone = deleteImageListWhenDone;
	m_pImageList = imageList;

	FOR_EACH_LL( m_DataItems, i )
	{
		m_DataItems[i]->UpdateImage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::SetFont(HFont font)
{
	Assert( font );
	if ( !font )
		return;

	m_hFont = font;
	m_iRowHeight = surface()->GetFontTall(font) + 1;
	
	FOR_EACH_LL( m_DataItems, i )
	{
		m_DataItems[i]->SetFont(m_hFont);
		TextImage *pTextImage = m_DataItems[i]->GetTextImage();
		pTextImage->ResizeImageToContent();
		m_DataItems[i]->SizeToContents();
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::GetSelectedItemsCount()
{
	return m_SelectedItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::GetSelectedItem(int selectionIndex)
{
	if ( m_SelectedItems.IsValidIndex(selectionIndex) )
		return m_SelectedItems[selectionIndex];

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::ClearSelectedItems()
{
	int i;
	for (i = 0 ; i < m_SelectedItems.Count(); i++)
	{
		if ( m_DataItems.IsValidIndex(m_SelectedItems[i]) )
		{
			m_DataItems[m_SelectedItems[i]]->SetSelected(false);
		}
	}
	m_SelectedItems.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::AddSelectedItem(int itemID)
{
	if ( m_SelectedItems.Find(itemID) == -1 )
	{
		m_SelectedItems.AddToTail(itemID);
		m_DataItems[itemID]->SetSelected(true);
		m_LastSelectedItemID = itemID;
		m_ShiftStartItemID = itemID;
		PostActionSignal(new KeyValues("ListViewItemSelected"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::SetSingleSelectedItem(int itemID)
{
	ClearSelectedItems();
	AddSelectedItem(itemID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnMouseWheeled(int delta)
{
	int val = m_hbar->GetValue();
	val -= delta;
	m_hbar->SetValue(val);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
	Repaint();
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::GetItemsMaxWidth()
{
	int maxWidth = 0;
	FOR_EACH_LL( m_DataItems, i )
	{
		int labelWide, labelTall;
		m_DataItems[i]->GetSize(labelWide, labelTall);
		if (labelWide > maxWidth)
		{
			maxWidth = labelWide + 25;
		}
	}
	return maxWidth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::PerformLayout()
{
	if (m_bNeedsSort)
	{
		SortList();
	}

	if ( m_DataItems.Count() == 0 )
		return;

	int wide, tall;
	GetSize(wide, tall);

	int maxWidth = GetItemsMaxWidth();
	if (maxWidth < 24)
	{
		maxWidth = 24;
	}
	int maxColVisible = wide / maxWidth;

	m_hbar->SetVisible(false);
	int itemsPerCol = GetItemsPerColumn();
	if (itemsPerCol < 1)
	{
		itemsPerCol = 1;
	}
	int cols = ( GetItemCount() + (itemsPerCol - 1) ) / itemsPerCol;

	int startItem = 0;
	if ( cols > maxColVisible)
	{
		m_hbar->SetVisible(true);

		// recalulate # per column now that we've made the hbar visible
		itemsPerCol = GetItemsPerColumn();
		cols = ( GetItemCount() + (itemsPerCol - 1) ) / (itemsPerCol > 0 ? itemsPerCol : 1 );

		m_hbar->SetEnabled(false);
		m_hbar->SetRangeWindow( maxColVisible );
		m_hbar->SetRange( 0, cols);	
		m_hbar->SetButtonPressedScrollValue( 1 );
	
		m_hbar->SetPos(0, tall - (m_hbar->GetTall()+WINDOW_BORDER_WIDTH));
		m_hbar->SetSize(wide - (WINDOW_BORDER_WIDTH*2), m_hbar->GetTall());
		m_hbar->InvalidateLayout();

		int val = m_hbar->GetValue();
		startItem += val*itemsPerCol;
	}
	else
	{
		m_hbar->SetVisible(false);
	}
	int lastItemVisible = startItem + (( maxColVisible + 1 )* itemsPerCol) - 1;

	int itemsThisCol = 0;
	int x = 0;
	int y = 0;
	int i;
	for ( i = 0 ; i < m_SortedItems.Count() ; i++ )
	{
		if ( i >= startItem && i <= lastItemVisible )
		{
			m_DataItems[ m_SortedItems[i] ]->SetVisible(true);
			m_DataItems[ m_SortedItems[i] ]->SetPos(x, y);
			itemsThisCol++;
			if ( itemsThisCol == itemsPerCol )
			{
				y = 0;
				x += maxWidth;
				itemsThisCol = 0;
			}
			else
			{
				y += m_iRowHeight;
			}
		}
		else
		{
			m_DataItems[ m_SortedItems[i] ]->SetVisible(false);
		}
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::Paint()
{
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(GetSchemeColor("ListPanel.BgColor", pScheme));
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));

	m_LabelFgColor = GetSchemeColor("ListPanel.TextColor", pScheme);
	m_SelectionFgColor = GetSchemeColor("ListPanel.SelectedTextColor", m_LabelFgColor, pScheme);
		
	m_hFont = pScheme->GetFont("Default", IsProportional());
	SetFont(m_hFont);
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnMousePressed( MouseCode code)
{
	if (code == MOUSE_LEFT || code == MOUSE_RIGHT)
	{
		ClearSelectedItems();		
		RequestFocus();
	}
	// check for context menu open
	if (code == MOUSE_RIGHT)
	{
		// post it, but with the invalid row
		PostActionSignal(new KeyValues("OpenContextMenu", "itemID", -1));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnShiftSelect(int itemID)
{
	// if we dont' have a valid selected ItemID - then we just choose the first item
	if ( !m_DataItems.IsValidIndex(m_ShiftStartItemID) )
	{
		m_ShiftStartItemID = m_DataItems.Head();
	}

	// find out if the just pressed item is "earlier" or is the 'last selected item' 
	int lowerPos = -1, upperPos = -1;
	int i;
	for ( i = 0 ; i < m_SortedItems.Count() ; i++ )
	{
		if ( m_SortedItems[i] == itemID )
		{
			lowerPos = i;
			upperPos = m_SortedItems.Find(m_ShiftStartItemID);
			break;
		}
		else if ( m_SortedItems[i] == m_ShiftStartItemID )
		{
			lowerPos = m_SortedItems.Find(m_ShiftStartItemID);
			upperPos = i;
			break;
		}
	}
	assert(lowerPos <= upperPos);
	if ( !input()->IsKeyDown(KEY_LCONTROL) && !input()->IsKeyDown(KEY_RCONTROL) )
	{
		ClearSelectedItems();		
	}

	for ( i = lowerPos ; i <= upperPos ; i ++)
	{
		// do not use AddSelectedItem because we don't want to switch the shiftStartItemID
		m_DataItems[ m_SortedItems[i] ]->SetSelected(true);
		m_SelectedItems.AddToTail(m_SortedItems[i]);
		m_LastSelectedItemID = itemID;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnItemMousePressed(ListViewItem* pItem, MouseCode code)
{
	int itemID = m_DataItems.Find(pItem);
	if (!m_DataItems.IsValidIndex(itemID))
		return;
	
	// check for context menu open
	if (code == MOUSE_RIGHT)
	{
		// if this is a new item - unselect everything else
		if ( m_SelectedItems.Find(itemID) == -1)
		{
			ClearSelectedItems();		
			AddSelectedItem(itemID);
		}

		PostActionSignal(new KeyValues("OpenContextMenu", "itemID", itemID));
	}
	else
	{
		if ( input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT) ) 
		{
			OnShiftSelect(itemID);
		}
		else if ( input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
		{
			if ( m_SelectedItems.Find(itemID) != -1)
			{
				m_SelectedItems.FindAndRemove(itemID);
				pItem->SetSelected(false);
	
				// manually select these since we 'last' clicked on these items
				m_ShiftStartItemID = itemID;
				m_LastSelectedItemID = itemID;
				m_DataItems[itemID]->RequestFocus();
			}
			else
			{
				AddSelectedItem(itemID);
			}
		}
		else
		{
			ClearSelectedItems();		
			AddSelectedItem(itemID);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnMouseDoublePressed( MouseCode code)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnItemMouseDoublePressed(ListViewItem* pItem, MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		OnKeyCodeTyped(KEY_ENTER);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::FinishKeyPress(int itemID)
{
	if ( input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT) ) 
	{
		OnShiftSelect(itemID);
	}
	else if ( input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
	{
		m_DataItems[itemID]->RequestFocus();
		m_LastSelectedItemID = itemID;
	}
	else
	{
		SetSingleSelectedItem(itemID);
	}
	ScrollToItem(itemID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( m_DataItems.Count() == 0 )
		return;

	switch (code)
	{
		case KEY_HOME:
		{
			if (m_SortedItems.Count() > 0)
			{
				int itemID = m_SortedItems[0];
				FinishKeyPress(itemID);
			}
			break;
		}
		case KEY_END:
		{
			if (m_DataItems.Count() > 0)
			{
				int itemID = m_SortedItems[ m_SortedItems.Count() - 1 ];
				FinishKeyPress(itemID);
			}
			break;
		}

		case KEY_UP:
		{
			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos--;
			if (itemPos < 0)
				itemPos = 0;

			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}
		
		case KEY_DOWN:
		{
			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos++;
			if (itemPos >= m_DataItems.Count())
				itemPos = m_DataItems.Count() - 1;

			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}

		case KEY_LEFT:
		{
			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos -= GetItemsPerColumn();
			if (itemPos < 0)
			{
				itemPos = 0;
			}
			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}

		case KEY_RIGHT:
		{
			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos += GetItemsPerColumn();
			if (itemPos >= m_SortedItems.Count())
			{
				itemPos = m_SortedItems.Count() - 1;
			}
			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}

		case KEY_PAGEUP:
		{
			int wide, tall;
			GetSize(wide, tall);

			int maxWidth = GetItemsMaxWidth();
			if (maxWidth == 0)
			{
				maxWidth = wide;
			}
			int maxColVisible = wide / maxWidth;
			int delta = maxColVisible * GetItemsPerColumn();

			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos -= delta;
			if (itemPos < 0)
			{
				itemPos = 0;
			}

			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}
		case KEY_PAGEDOWN:
		{
			int wide, tall;
			GetSize(wide, tall);

			int maxWidth = GetItemsMaxWidth();
			if (maxWidth == 0)
			{
				maxWidth = wide;
			}
			int maxColVisible = wide / maxWidth;
			int delta = maxColVisible * GetItemsPerColumn();

			int itemPos = m_SortedItems.Find( m_LastSelectedItemID );
			itemPos += delta;
			if (itemPos >= m_SortedItems.Count())
			{
				itemPos = m_SortedItems.Count() - 1;
			}

			FinishKeyPress(m_SortedItems[itemPos]);
			break;
		}
		default:
		{
			BaseClass::OnKeyCodeTyped(code);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnKeyTyped(wchar_t unichar)
{
	if (!iswcntrl(unichar))
	{
		wchar_t uniString[2];
		uniString[0] = unichar;
		uniString[1] = 0;

		char buf[2];
		g_pVGuiLocalize->ConvertUnicodeToANSI(uniString, buf, sizeof(buf));

		int i; 
		int itemPos = m_SortedItems.Find(m_LastSelectedItemID);
		if ( m_SortedItems.IsValidIndex(itemPos))
		{
			itemPos++;
			// start from the item AFTER our last selected Item and go to end
			for ( i = itemPos ; i != m_SortedItems.Count(); i++)
			{
				KeyValues *kv = m_DataItems[ m_SortedItems[i] ]->GetData();
				const char *pszText = kv->GetString("text");
				if (!strnicmp(pszText, buf, 1))
				{
					// select the next of this letter
					SetSingleSelectedItem(m_SortedItems[i]);
					ScrollToItem(m_SortedItems[i]);
					return;
				}
			}
			// if the after this item we couldn't fine an item with  the same letter, fall through and just start from the beginning of list to the last selected item
		}

		for ( i = 0 ; i < m_SortedItems.Count() ; i++ )
		{
			// we've gone all the way around - break - if we had a valid index, this is one more that last selectedItem, if not it's an illegal index
			if ( i == itemPos)
				break;

			KeyValues *kv = m_DataItems[ m_SortedItems[i] ]->GetData();
			const char *pszText = kv->GetString("text");
			if (!strnicmp(pszText, buf, 1))
			{
				SetSingleSelectedItem(m_SortedItems[i]);
				ScrollToItem(m_SortedItems[i]);
				return;
			}
		}
	}
	else
		BaseClass::OnKeyTyped(unichar);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ListViewPanel::OnSliderMoved()
{
	InvalidateLayout();
	Repaint();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int ListViewPanel::GetItemsPerColumn()
{
	int wide, tall;
	GetSize(wide, tall);

	if ( m_hbar->IsVisible() )
	{
		tall -= m_hbar->GetTall();
	}

	return tall / m_iRowHeight;	// should round down
}

