//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LISTVIEWPANEL_H
#define LISTVIEWPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <utllinkedlist.h>
#include <utlvector.h>

namespace vgui
{

class ListViewPanel;
typedef bool (*ListViewSortFunc_t)(KeyValues *kv1, KeyValues *kv2);

class ListViewItem;

//-----------------------------------------------------------------------------
// Purpose: List Ctrl Panel with each item having an icon and text after it
//-----------------------------------------------------------------------------
class ListViewPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( ListViewPanel, Panel );

public:
	ListViewPanel(Panel *parent, const char *panelName);
	~ListViewPanel();

	virtual int  AddItem(const KeyValues *data, bool bScrollToItem, bool bSortOnAdd);
	virtual int  GetItemCount();
	virtual KeyValues *GetItem(int itemID);
	virtual void ApplyItemChanges(int itemID);
	virtual void RemoveItem(int itemID);
	virtual void DeleteAllItems();
	virtual int GetItemIDFromPos(int iPos);		// valid from [0, GetItemCount)

	virtual int  InvalidItemID();
	virtual bool IsValidItemID(int itemID);

	virtual void ScrollToItem(int itemID);

	virtual void SetSortFunc(ListViewSortFunc_t func);	
	virtual void SortList();

	// image handling
	virtual void SetImageList(ImageList *imageList, bool deleteImageListWhenDone);

	virtual void SetFont(HFont font);

	// returns the count of selected items
	virtual int GetSelectedItemsCount();

	// returns the selected item by selection index, valid in range [0, GetNumSelectedRows)
	virtual int GetSelectedItem(int selectionIndex);

	// sets no item as selected
	virtual void ClearSelectedItems();

	// adds a item to the select list
	virtual void AddSelectedItem(int itemID);

	// sets this single item as the only selected item
	virtual void SetSingleSelectedItem(int itemID);

protected:
	// overrides
	virtual void OnMouseWheeled(int delta);
	virtual void OnSizeChanged(int wide, int tall); 
	virtual void PerformLayout();
	virtual void Paint();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnMousePressed( MouseCode code);
	virtual void OnMouseDoublePressed( MouseCode code);
	virtual void OnKeyCodeTyped( KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	MESSAGE_FUNC( OnSliderMoved, "ScrollBarSliderMoved" );
	virtual int GetItemsPerColumn();

private:
	ScrollBar			*m_hbar;

	friend class ListViewItem;
	void 	OnItemMousePressed(ListViewItem* pItem, MouseCode code);
	void 	OnItemMouseDoublePressed(ListViewItem* pItem, MouseCode code);
	int 	GetItemsMaxWidth();
	int 	GetItemIndex(int itemID);
	void 	OnShiftSelect(int itemID);
	void 	FinishKeyPress(int itemID);

	CUtlLinkedList<ListViewItem*, int>		m_DataItems;
	CUtlVector<int>							m_SortedItems;
	ListViewSortFunc_t						m_pSortFunc;

	int 				m_iRowHeight;
	HFont				m_hFont;

	Color 		m_LabelFgColor;
	Color 		m_SelectionFgColor;

	// selection data
	CUtlVector<int> 	m_SelectedItems;		
	int					m_LastSelectedItemID;
	int					m_ShiftStartItemID;

	bool		m_bNeedsSort;
	bool 		m_bDeleteImageListWhenDone;
	ImageList 	*m_pImageList;
};


}

#endif // LISTVIEWPANEL_H
