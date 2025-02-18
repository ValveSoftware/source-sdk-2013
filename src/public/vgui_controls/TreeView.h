//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TREEVIEW_H
#define TREEVIEW_H

#ifdef _WIN32
#pragma once
#endif

#include <utllinkedlist.h>
#include <utlvector.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class KeyValues;

namespace vgui
{

class ExpandButton;
class TreeNode;
class TreeViewSubPanel;

// sorting function, should return true if node1 should be displayed before node2
typedef bool (*TreeViewSortFunc_t)(KeyValues *node1, KeyValues *node2);

class TreeView : public Panel
{
	DECLARE_CLASS_SIMPLE( TreeView, Panel );

public:
    TreeView(Panel *parent, const char *panelName);
    ~TreeView();

    void SetSortFunc(TreeViewSortFunc_t pSortFunc);

    virtual int AddItem(KeyValues *data, int parentItemIndex);

	virtual int GetRootItemIndex();
	virtual int GetNumChildren( int itemIndex );
	virtual int GetChild( int iParentItemIndex, int iChild ); // between 0 and GetNumChildren( iParentItemIndex ).

    virtual int GetItemCount(void);
    virtual KeyValues *GetItemData(int itemIndex);
     virtual void RemoveItem(int itemIndex, bool bPromoteChildren, bool bRecursivelyRemove = false );
    virtual void RemoveAll();
    virtual bool ModifyItem(int itemIndex, KeyValues *data);
	virtual int GetItemParent(int itemIndex);

    virtual void SetFont(HFont font);

    virtual void SetImageList(ImageList *imageList, bool deleteImageListWhenDone);

	void SetAllowMultipleSelections( bool state );
	bool IsMultipleSelectionAllowed() const;

	virtual void ClearSelection();
    virtual void AddSelectedItem( int itemIndex, bool clearCurrentSelection, bool requestFocus = true, bool bMakeItemVisible = true );
	virtual void RemoveSelectedItem( int itemIndex );
	virtual void SelectAll();

	virtual bool IsItemSelected( int itemIndex );
	virtual void RangeSelectItems( int clickedItem );
	virtual void FindNodesInRange( int startItem, int endItem, CUtlVector< int >& itemIndices );

	// returns the id of the currently selected item, -1 if nothing is selected
	virtual int GetSelectedItemCount() const;
	virtual int GetFirstSelectedItem() const;
	virtual void GetSelectedItems( CUtlVector< int >& list );
	virtual void GetSelectedItemData( CUtlVector< KeyValues * >& list );

	// set colors for individual elments
	virtual void SetItemFgColor(int itemIndex, const Color& color);
	virtual void SetItemBgColor(int itemIndex, const Color& color);
	virtual void SetItemSelectionTextColor( int itemIndex, const Color& clr );
	virtual void SetItemSelectionBgColor( int itemIndex, const Color& clr );
	virtual void SetItemSelectionUnfocusedBgColor( int itemIndex, const Color& clr );

	// returns true if the itemID is valid for use
	virtual bool IsItemIDValid(int itemIndex);

	// item iterators
	// iterate from [0..GetHighestItemID()], 
	// and check each with IsItemIDValid() before using
	virtual int GetHighestItemID();

    virtual void ExpandItem(int itemIndex, bool bExpand);
	virtual bool IsItemExpanded( int itemIndex );

    virtual void MakeItemVisible(int itemIndex);
	
	// This tells which of the visible items is the top one.
	virtual void GetVBarInfo( int &top, int &nItemsVisible, bool& hbarVisible );

	virtual HFont GetFont();

	virtual void GenerateDragDataForItem( int itemIndex, KeyValues *msg );
	virtual void SetDragEnabledItems( bool state );

	virtual void OnLabelChanged( int itemIndex, char const *oldString, char const *newString );
	virtual bool IsLabelEditingAllowed() const;
	virtual bool IsLabelBeingEdited() const;
	virtual void SetAllowLabelEditing( bool state );

	/* message sent

		"TreeViewItemSelected"  int "itemIndex"
			called when the selected item changes
		"TreeViewItemDeselected" int "itemIndex"
			called when item is deselected
	*/
    int GetRowHeight();
	int GetVisibleMaxWidth();
	virtual void OnMousePressed(MouseCode code);

	// By default, the tree view expands nodes on left-click. This enables/disables that feature
	void EnableExpandTreeOnLeftClick( bool bEnable );

	virtual void SetLabelEditingAllowed( int itemIndex, bool state );
	virtual void StartEditingLabel( int itemIndex );

	virtual bool IsItemDroppable( int itemIndex, CUtlVector< KeyValues * >& msglist );
	virtual void OnItemDropped( int itemIndex, CUtlVector< KeyValues * >& msglist );
	virtual bool GetItemDropContextMenu( int itemIndex, Menu *menu, CUtlVector< KeyValues * >& msglist );
	virtual HCursor GetItemDropCursor( int itemIndex, CUtlVector< KeyValues * >& msglist );

	virtual int		GetPrevChildItemIndex( int itemIndex );
	virtual int		GetNextChildItemIndex( int itemIndex );

	virtual void PerformLayout();

	// Makes the scrollbar parented to some other panel...
	ScrollBar	*SetScrollBarExternal( bool vertical, Panel *newParent );
	void		GetScrollBarSize( bool vertical, int& w, int& h );

	void		SetMultipleItemDragEnabled( bool state ); // if this is set, then clicking on one row and dragging will select a run or items, etc.
	bool		IsMultipleItemDragEnabled() const;

	int			FindItemUnderMouse( int mx, int my );

protected:
	// functions to override
	// called when a node, marked as "Expand", needs to generate it's child nodes when expanded
	virtual void GenerateChildrenOfNode(int itemIndex) {}

	// override to open a custom context menu on a node being selected and right-clicked
	virtual void GenerateContextMenu( int itemIndex, int x, int y ) {}

	// overrides
	virtual void OnMouseWheeled(int delta);
	virtual void OnSizeChanged(int wide, int tall); 
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_INT( OnSliderMoved, "ScrollBarSliderMoved", position );
	virtual void SetBgColor( Color color );

private:
    friend class TreeNode;
	friend class TreeNodeText;

	TreeNode* GetItem( int itemIndex );
	virtual void RemoveChildrenOfNode( int itemIndex );
	void SetLabelBeingEdited( bool state );

	// Clean up the image list
	void CleanUpImageList( );

	// to be accessed by TreeNodes
    IImage* GetImage(int index);        

	// bools
	bool m_bAllowLabelEditing : 1;
	bool m_bDragEnabledItems : 1;
	bool m_bDeleteImageListWhenDone : 1;
	bool m_bLeftClickExpandsTree : 1;
	bool m_bLabelBeingEdited : 1;
	bool m_bMultipleItemDragging : 1;
	bool m_bAllowMultipleSelections : 1;

    // cross reference - no hierarchy ordering in this list
    CUtlLinkedList<TreeNode *, int>   m_NodeList;
   	ScrollBar					*m_pHorzScrollBar, *m_pVertScrollBar;
	int							m_nRowHeight;

	ImageList					*m_pImageList;
    TreeNode					*m_pRootNode;
    TreeViewSortFunc_t			m_pSortFunc;
    HFont						m_Font;

    CUtlVector< TreeNode * >	m_SelectedItems;
    TreeViewSubPanel			*m_pSubPanel;

	int							m_nMostRecentlySelectedItem;
	bool						m_bScrollbarExternal[ 2 ]; // 0 = vert, 1 = horz
};

}

#endif // TREEVIEW_H
