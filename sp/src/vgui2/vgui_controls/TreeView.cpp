//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>

#define PROTECTED_THINGS_DISABLE

#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/TreeView.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>

#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

using namespace vgui;
enum 
{
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

#define TREE_INDENT_AMOUNT 20

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Displays an editable text field for the text control
//-----------------------------------------------------------------------------
class TreeNodeText : public TextEntry
{
	DECLARE_CLASS_SIMPLE( TreeNodeText, TextEntry );

public:
    TreeNodeText(Panel *parent, const char *panelName, TreeView *tree) : BaseClass(parent, panelName), m_pTree( tree )
    {
		m_bEditingInPlace = false;
		m_bLabelEditingAllowed = false;
		SetDragEnabled( false );
		SetDropEnabled( false );
		AddActionSignalTarget( this );
		m_bArmForEditing = false;
		m_bWaitingForRelease = false;
		m_lArmingTime = 0L;
		SetAllowKeyBindingChainToParent( true );
    }

	MESSAGE_FUNC( OnTextChanged, "TextChanged" )
	{
		GetParent()->InvalidateLayout();
	}

	bool IsKeyRebound( KeyCode code, int modifiers )
	{
		// If in editing mode, don't try and chain keypresses
		if ( m_bEditingInPlace )
		{
			return false;
		}

		return BaseClass::IsKeyRebound( code, modifiers );
	}

	virtual void PaintBackground()
	{
		BaseClass::PaintBackground();

		if ( !m_bLabelEditingAllowed )
			return;

		if ( !m_bEditingInPlace )
			return;

		int w, h;
		GetSize( w, h );
		surface()->DrawSetColor( GetFgColor() );
		surface()->DrawOutlinedRect( 0, 0, w, h );
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
    {
        TextEntry::ApplySchemeSettings(pScheme);
        SetBorder(NULL);
        SetCursor(dc_arrow);
    }

    virtual void OnKeyCodeTyped(KeyCode code)
    {
		if ( m_bEditingInPlace )
		{
			if ( code == KEY_ENTER )
			{
				FinishEditingInPlace();
			}
			else if ( code == KEY_ESCAPE )
			{
				FinishEditingInPlace( true );
			}
			else
			{
				BaseClass::OnKeyCodeTyped( code );
			}
			return;
		}
		else if ( code == KEY_ENTER && IsLabelEditingAllowed() )
		{
			EnterEditingInPlace();
		}
		else
		{
	        // let parent deal with it (don't chain back to TextEntry)
			CallParentFunction(new KeyValues("KeyCodeTyped", "code", code));
		}
    }

#define CLICK_TO_EDIT_DELAY_MSEC 500

	virtual void OnTick()
	{
		BaseClass::OnTick();
		if ( m_bArmForEditing )
		{
			long msecSinceArming = system()->GetTimeMillis() - m_lArmingTime;

			if ( msecSinceArming > CLICK_TO_EDIT_DELAY_MSEC )
			{
				m_bArmForEditing = false;
				m_bWaitingForRelease = false;
				ivgui()->RemoveTickSignal( GetVPanel() );
				EnterEditingInPlace();
			}
		}
	}

	virtual void OnMouseReleased( MouseCode code )
	{
		if ( m_bEditingInPlace )
		{
			BaseClass::OnMouseReleased( code );
			return;
		}
		else
		{
			if ( m_bWaitingForRelease && !IsBeingDragged() )
			{
				m_bArmForEditing = true;
				m_bWaitingForRelease = false;
				m_lArmingTime = system()->GetTimeMillis();
				ivgui()->AddTickSignal( GetVPanel() );
			}
			else
			{
				m_bWaitingForRelease = false;
			}
		}

        // let parent deal with it
		CallParentFunction(new KeyValues("MouseReleased", "code", code));
	}

	virtual void OnCursorMoved( int x, int y )
	{
		 // let parent deal with it
		CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
	}

	virtual void OnMousePressed(MouseCode code)
    {
		if ( m_bEditingInPlace )
		{
			BaseClass::OnMousePressed( code );
			return;
		}
		else
		{
			bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
			bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));

			// make sure there is only one item selected
			// before "WaitingForRelease" which leads to label editing.
			CUtlVector< int > list;
			m_pTree->GetSelectedItems( list );
			bool bIsOnlyOneItemSelected = ( list.Count() == 1 );

			if ( !shift && 
				!ctrl &&
				!m_bArmForEditing && 
				IsLabelEditingAllowed() && 
				bIsOnlyOneItemSelected && 
				IsTextFullySelected() && 
				!IsBeingDragged() )
			{
				m_bWaitingForRelease = true;
			}
		}

        // let parent deal with it
		CallParentFunction(new KeyValues("MousePressed", "code", code));
	}

	void	SetLabelEditingAllowed( bool state )
	{
		m_bLabelEditingAllowed = state;
	}

	bool	IsLabelEditingAllowed()
	{
		return m_bLabelEditingAllowed;
	}

    virtual void OnMouseDoublePressed(MouseCode code)
 	{
		// Once we are editing, double pressing shouldn't chain up
		if ( m_bEditingInPlace )
		{
			BaseClass::OnMouseDoublePressed( code );
			return;
		}

		if ( m_bArmForEditing )
		{
			m_bArmForEditing = false;
			m_bWaitingForRelease = false;
			ivgui()->RemoveTickSignal( GetVPanel() );
		}

		CallParentFunction(new KeyValues("MouseDoublePressed", "code", code));
    }

	void EnterEditingInPlace()
	{
		if ( m_bEditingInPlace )
			return;

		m_bEditingInPlace = true;
		char buf[ 1024 ];
		GetText( buf, sizeof( buf ) );
		m_OriginalText = buf;
		SetCursor(dc_ibeam);
		SetEditable( true );
		SelectNone();
		GotoTextEnd();
		RequestFocus();
		SelectAllText(false);
		m_pTree->SetLabelBeingEdited( true );
	}

	void FinishEditingInPlace( bool revert = false )
	{
		if ( !m_bEditingInPlace )
			return;

		m_pTree->SetLabelBeingEdited( false );
		SetEditable( false );
		SetCursor(dc_arrow);
		m_bEditingInPlace = false;
		char buf[ 1024 ];
		GetText( buf, sizeof( buf ) );

		// Not actually changed...
		if ( !Q_strcmp( buf, m_OriginalText.Get() ) )
			return;

		if ( revert )
		{
			SetText( m_OriginalText.Get() );
			GetParent()->InvalidateLayout();
		}
		else
		{
			KeyValues *kv = new KeyValues( "LabelChanged", "original", m_OriginalText.Get(), "changed", buf );
			PostActionSignal( kv );
		}
	}

	virtual void OnKillFocus()
	{
		BaseClass::OnKillFocus();

		FinishEditingInPlace();
	}

	virtual void OnMouseWheeled(int delta)
    {
		if ( m_bEditingInPlace )
		{
			BaseClass::OnMouseWheeled( delta );
			return;
		}

		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
    }
    // editable - cursor normal, and ability to edit text

	bool	IsBeingEdited() const
	{
		return m_bEditingInPlace;
	}

private:

	bool		m_bEditingInPlace;
	CUtlString	m_OriginalText;
	bool		m_bLabelEditingAllowed;

	bool		m_bArmForEditing;
	bool		m_bWaitingForRelease;
	long		m_lArmingTime;
	TreeView	*m_pTree;
};

//-----------------------------------------------------------------------------
// Purpose: icon for the tree node (folder icon, file icon, etc.)
//-----------------------------------------------------------------------------
class TreeNodeImage : public ImagePanel
{
public:
    TreeNodeImage(Panel *parent, const char *name) : ImagePanel(parent, name)
	{
		SetBlockDragChaining( true );
	}

 	//!! this could possibly be changed to just disallow mouse input on the image panel
    virtual void OnMousePressed(MouseCode code)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MousePressed", "code", code));
    }
	
    virtual void OnMouseDoublePressed(MouseCode code)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MouseDoublePressed", "code", code));
    }

    virtual void OnMouseWheeled(int delta)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
    }

	virtual void OnCursorMoved( int x, int y )
	{
		 // let parent deal with it
		CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
	}
};

//-----------------------------------------------------------------------------
// Purpose: Scrollable area of the tree control, holds the tree itself only
//-----------------------------------------------------------------------------
class TreeViewSubPanel : public Panel
{
public:
    TreeViewSubPanel(Panel *parent) : Panel(parent) {}

    virtual void ApplySchemeSettings(IScheme *pScheme)
    {
    	Panel::ApplySchemeSettings(pScheme);
    
    	SetBorder(NULL);
   }

	virtual void OnMouseWheeled(int delta)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
    }
	virtual void OnMousePressed(MouseCode code)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MousePressed", "code", code));
    }
    virtual void OnMouseDoublePressed(MouseCode code)
    {
        // let parent deal with it
		CallParentFunction(new KeyValues("MouseDoublePressed", "code", code));
    }

	virtual void OnCursorMoved( int x, int y )
	{
		 // let parent deal with it
		CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
	}
};

//-----------------------------------------------------------------------------
// Purpose: A single entry in the tree
//-----------------------------------------------------------------------------
class TreeNode : public Panel
{
	DECLARE_CLASS_SIMPLE( TreeNode, Panel );

public:
    TreeNode(Panel *parent, TreeView *pTreeView);
	~TreeNode();
    void SetText(const char *pszText);
    void SetFont(HFont font);
    void SetKeyValues(KeyValues *data);
    bool IsSelected();
	// currently unused, could be re-used if necessary
//	bool IsInFocus();
	virtual void PaintBackground();
    virtual void PerformLayout();
	TreeNode *GetParentNode();
    int GetChildrenCount();
	void ClearChildren();
	int ComputeInsertionPosition( TreeNode *pChild );
	int FindChild( TreeNode *pChild );
    void AddChild(TreeNode *pChild);
    void SetNodeExpanded(bool bExpanded);
    bool IsExpanded();
    int CountVisibleNodes();
	void CalculateVisibleMaxWidth();
	void OnChildWidthChange();
	int GetMaxChildrenWidth();
    int GetVisibleMaxWidth();
    int GetDepth();
    bool HasParent(TreeNode *pTreeNode);
    bool IsBeingDisplayed();
	virtual void SetVisible(bool state);
    virtual void Paint();
    virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void SetBgColor( Color color );
	virtual void SetFgColor( Color color );
    virtual void OnSetFocus();
    void SelectPrevChild(TreeNode *pCurrentChild);
    void SelectNextChild(TreeNode *pCurrentChild);

	int GetPrevChildItemIndex( TreeNode *pCurrentChild );
	int GetNextChildItemIndex( TreeNode *pCurrentChild );

	virtual void ClosePreviousParents( TreeNode *pPreviousParent );
	virtual void StepInto( bool bClosePrevious=true );
	virtual void StepOut( bool bClosePrevious=true );
	virtual void StepOver( bool bClosePrevious=true );
    virtual void OnKeyCodeTyped(KeyCode code);
 	virtual void OnMouseWheeled(int delta);
    virtual void OnMousePressed( MouseCode code);
	virtual void OnMouseReleased( MouseCode code);
	virtual void OnCursorMoved( int x, int y );
	virtual bool IsDragEnabled() const;
    void PositionAndSetVisibleNodes(int &nStart, int &nCount, int x, int &y);

    // counts items above this item including itself
    int CountVisibleIndex();

	virtual void OnCreateDragData( KeyValues *msg );
	// For handling multiple selections...
	virtual void OnGetAdditionalDragPanels( CUtlVector< Panel * >& dragabbles );
	virtual void OnMouseDoublePressed( MouseCode code );
	TreeNode *FindItemUnderMouse( int &nStart, int& nCount, int x, int &y, int mx, int my );
	MESSAGE_FUNC_PARAMS( OnLabelChanged, "LabelChanged", data );
	void EditLabel();
	void	SetLabelEditingAllowed( bool state );
	bool	IsLabelEditingAllowed() const;

	virtual bool IsDroppable( CUtlVector< KeyValues * >& msglist );
	virtual void OnPanelDropped( CUtlVector< KeyValues * >& msglist );
	virtual HCursor GetDropCursor( CUtlVector< KeyValues * >& msglist );
	virtual bool GetDropContextMenu( Menu *menu, CUtlVector< KeyValues * >& msglist );

	void				FindNodesInRange( CUtlVector< TreeNode * >& list, int startIndex, int endIndex );

	void				RemoveChildren();

	void SetSelectionTextColor( const Color& clr );
	void SetSelectionBgColor( const Color& clr );
	void SetSelectionUnfocusedBgColor( const Color& clr );
public:
	int                 m_ItemIndex;
	int					m_ParentIndex;
	KeyValues           *m_pData;
    CUtlVector<TreeNode *> m_Children;
    bool                m_bExpand;

private:

	void				FindNodesInRange_R( CUtlVector< TreeNode * >& list, bool& finished, bool& foundStart, int startIndex, int endIndex );

	int					m_iNodeWidth;
	int					m_iMaxVisibleWidth;

    TreeNodeText        *m_pText;
    TextImage           *m_pExpandImage;
    TreeNodeImage       *m_pImagePanel;

	bool				m_bExpandableWithoutChildren;

    TreeView            *m_pTreeView;
	int					m_nClickedItem;
	bool				m_bClickedSelected;
};


TreeNode::TreeNode(Panel *parent, TreeView *pTreeView) : 
	BaseClass(parent, "TreeNode" ),
	m_nClickedItem( 0 ),
	m_bClickedSelected( false )
{
    m_pData = NULL;
    m_pTreeView = pTreeView;
    m_ItemIndex = -1;
	m_iNodeWidth = 0; 
	m_iMaxVisibleWidth = 0;

    m_pExpandImage = new TextImage("+");
    m_pExpandImage->SetPos(3, 1);

    m_pImagePanel = new TreeNodeImage(this, "TreeImage");
    m_pImagePanel->SetPos(TREE_INDENT_AMOUNT, 3);

    m_pText = new TreeNodeText(this, "TreeNodeText",pTreeView);
    m_pText->SetMultiline(false);
    m_pText->SetEditable(false);
    m_pText->SetPos(TREE_INDENT_AMOUNT*2, 0);
	m_pText->AddActionSignalTarget( this );

    m_bExpand = false;
	m_bExpandableWithoutChildren = false;
}

TreeNode::~TreeNode()
{
	delete m_pExpandImage;
	if ( m_pData )
	{
		m_pData->deleteThis();
	}
}

void TreeNode::SetText(const char *pszText)
{
    m_pText->SetText(pszText);
	InvalidateLayout();
}

void TreeNode::SetLabelEditingAllowed( bool state )
{
	Assert( m_pTreeView->IsLabelEditingAllowed() );
	m_pText->SetLabelEditingAllowed( state );
}

bool TreeNode::IsLabelEditingAllowed() const
{
	return m_pText->IsLabelEditingAllowed();
}

bool TreeNode::GetDropContextMenu( Menu *menu, CUtlVector< KeyValues * >& msglist )
{
	return m_pTreeView->GetItemDropContextMenu( m_ItemIndex, menu, msglist );
}

bool TreeNode::IsDroppable( CUtlVector< KeyValues * >& msglist )
{
	return m_pTreeView->IsItemDroppable( m_ItemIndex, msglist );
}

void TreeNode::OnPanelDropped( CUtlVector< KeyValues * >& msglist )
{
	m_pTreeView->OnItemDropped( m_ItemIndex, msglist );
}

HCursor TreeNode::GetDropCursor( CUtlVector< KeyValues * >& msglist )
{
	return m_pTreeView->GetItemDropCursor( m_ItemIndex, msglist );
}


void TreeNode::OnCreateDragData( KeyValues *msg )
{
	// make sure the dragged item appears selected,
	// on the off chance it appears deselected by a cntl mousedown
	m_pTreeView->AddSelectedItem( m_ItemIndex, false );

	m_pTreeView->GenerateDragDataForItem( m_ItemIndex, msg );
}

// For handling multiple selections...
void TreeNode::OnGetAdditionalDragPanels( CUtlVector< Panel * >& dragabbles )
{
	CUtlVector< int > list;
	m_pTreeView->GetSelectedItems( list );
	int c = list.Count();
	// walk this in reverse order so that panels are in order of selection
	// even though GetSelectedItems returns items in reverse selection order
	for ( int i = c - 1; i >= 0; --i )
	{
		int itemIndex = list[ i ];
		// Skip self
		if ( itemIndex == m_ItemIndex )
			continue;

		dragabbles.AddToTail( ( Panel * )m_pTreeView->GetItem( itemIndex ) );
	}
}

void TreeNode::OnLabelChanged( KeyValues *data )
{
	char const *oldString = data->GetString( "original" );
	char const *newString = data->GetString( "changed" );
	if ( m_pTreeView->IsLabelEditingAllowed() )
	{
		m_pTreeView->OnLabelChanged( m_ItemIndex, oldString, newString );
	}
}

void TreeNode::EditLabel()
{
	if ( m_pText->IsLabelEditingAllowed() && 
		!m_pText->IsBeingEdited() )
	{
		m_pText->EnterEditingInPlace();
	}
}

void TreeNode::SetFont(HFont font)
{
    Assert( font );
    if ( !font )
        return;

    m_pText->SetFont(font);
	m_pExpandImage->SetFont(font);
	InvalidateLayout();
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        m_Children[i]->SetFont(font);
    }
}

void TreeNode::SetKeyValues(KeyValues *data)
{
	if ( m_pData != data )
	{
		if (m_pData)
		{
			m_pData->deleteThis();
		}

		m_pData = data->MakeCopy();
	}

    // set text
    m_pText->SetText(data->GetString("Text", ""));
 	m_bExpandableWithoutChildren = data->GetInt("Expand");
    InvalidateLayout();
}

bool TreeNode::IsSelected()
{
	return m_pTreeView->IsItemSelected( m_ItemIndex );
}

void TreeNode::PaintBackground()
{
	if ( !m_pText->IsBeingEdited() )
	{
		// setup panel drawing
		if ( IsSelected() )
		{
			m_pText->SelectAllText(false);
		}
		else
		{
			m_pText->SelectNoText();
		}
	}

	BaseClass::PaintBackground();
}


// currently unused, could be re-used if necessary
/*
bool TreeNode::IsInFocus()
{
    // check if our parent or one of it's children has focus
    VPANEL focus = input()->GetFocus();
    return (HasFocus() || (focus && ipanel()->HasParent(focus, GetVParent())));
}
*/

void TreeNode::PerformLayout()
{
    BaseClass::PerformLayout();

    int width = 0;
	if (m_pData->GetInt("SelectedImage", 0) == 0 &&
		m_pData->GetInt("Image", 0) == 0)
	{
		width = TREE_INDENT_AMOUNT;
	}
	else
	{
		width = TREE_INDENT_AMOUNT * 2;
	}

	m_pText->SetPos(width, 0);

    int contentWide, contentTall;
	m_pText->SetToFullWidth();
    m_pText->GetSize(contentWide, contentTall);
	contentWide += 10;
	m_pText->SetSize( contentWide, m_pTreeView->GetRowHeight() );
    width += contentWide;
    SetSize(width, m_pTreeView->GetRowHeight());

	m_iNodeWidth = width;
	CalculateVisibleMaxWidth();
}

TreeNode *TreeNode::GetParentNode()
{
	if (m_pTreeView->m_NodeList.IsValidIndex(m_ParentIndex))
	{
		return m_pTreeView->m_NodeList[m_ParentIndex];
	}
	return NULL;
}

int TreeNode::GetChildrenCount()
{
    return m_Children.Count();
}

int TreeNode::ComputeInsertionPosition( TreeNode *pChild )
{
	if ( !m_pTreeView->m_pSortFunc )
	{
		return GetChildrenCount() - 1;
	}

	int start = 0, end = GetChildrenCount() - 1;
	while (start <= end)
	{
		int mid = (start + end) >> 1;
		if ( m_pTreeView->m_pSortFunc( m_Children[mid]->m_pData, pChild->m_pData ) )
		{
			start = mid + 1;
		}
		else if ( m_pTreeView->m_pSortFunc( pChild->m_pData, m_Children[mid]->m_pData ) )
		{
			end = mid - 1;
		}
		else
		{
			return mid;
		}
	}
	return end;
}

int TreeNode::FindChild( TreeNode *pChild )
{
	if ( !m_pTreeView->m_pSortFunc )
	{
		AssertMsg( 0, "This code has never been tested. Is it correct?" );
		for ( int i = 0; i < GetChildrenCount(); ++i )
		{
			if ( m_Children[i] == pChild )
				return i;
		}
		return -1;
	}

	// Find the first entry <= to the child
	int start = 0, end = GetChildrenCount() - 1;
	while (start <= end)
	{
		int mid = (start + end) >> 1;

		if ( m_Children[mid] == pChild )
			return mid;

		if ( m_pTreeView->m_pSortFunc( m_Children[mid]->m_pData, pChild->m_pData ) )
		{
			start = mid + 1;
		}
		else
		{
			end = mid - 1;
		}
	}

	int nMax = GetChildrenCount();
	while( end < nMax )
	{
		// Stop when we reach a child that has a different value
		if ( m_pTreeView->m_pSortFunc( pChild->m_pData, m_Children[end]->m_pData ) )
			return -1;

		if ( m_Children[end] == pChild )
			return end;

		++end;
	}

	return -1;
}

void TreeNode::AddChild(TreeNode *pChild)
{
	int i = ComputeInsertionPosition( pChild );
	m_Children.InsertAfter( i, pChild );
}

void TreeNode::SetNodeExpanded(bool bExpanded)
{
    m_bExpand = bExpanded;

    if (m_bExpand)
    {
		// see if we have any child nodes
		if (GetChildrenCount() < 1)
		{
			// we need to get our children from the control
			m_pTreeView->GenerateChildrenOfNode(m_ItemIndex);

			// if we still don't have any children, then hide the expand button
			if (GetChildrenCount() < 1)
			{
				m_bExpand = false;
				m_bExpandableWithoutChildren = false;
				m_pTreeView->InvalidateLayout();
				return;
			}
		}

        m_pExpandImage->SetText("-");
    }
    else
    {
        m_pExpandImage->SetText("+");

		if ( m_bExpandableWithoutChildren && GetChildrenCount() > 0 )
		{
			m_pTreeView->RemoveChildrenOfNode( m_ItemIndex );
		}

        // check if we've closed down on one of our children, if so, we get the focus
        int selectedItem = m_pTreeView->GetFirstSelectedItem();
        if (selectedItem != -1 && m_pTreeView->m_NodeList[selectedItem]->HasParent(this))
        {
            m_pTreeView->AddSelectedItem( m_ItemIndex, true );
        }
    }
	CalculateVisibleMaxWidth();
    m_pTreeView->InvalidateLayout();
}

bool TreeNode::IsExpanded()
{
    return m_bExpand;
}

int TreeNode::CountVisibleNodes()
{
    int count = 1;  // count myself
    if (m_bExpand)
    {
        int i;
        for (i=0;i<m_Children.Count();i++)
        {
            count += m_Children[i]->CountVisibleNodes();
        }
    }
    return count;
}

void TreeNode::CalculateVisibleMaxWidth()
{
	int width;
	if (m_bExpand)
	{
		int childMaxWidth = GetMaxChildrenWidth();
		childMaxWidth += TREE_INDENT_AMOUNT;

		width = max(childMaxWidth, m_iNodeWidth);
	}
	else
	{
		width = m_iNodeWidth;
	}
	if (width != m_iMaxVisibleWidth)
	{
		m_iMaxVisibleWidth = width;
		if (GetParentNode())
		{
			GetParentNode()->OnChildWidthChange();
		}
		else
		{
			m_pTreeView->InvalidateLayout();
		}
	}
}

void TreeNode::OnChildWidthChange()
{
	CalculateVisibleMaxWidth();
}

int TreeNode::GetMaxChildrenWidth()
{
	int maxWidth = 0;
	int i;
    for (i=0;i<GetChildrenCount();i++)
    {
		int childWidth = m_Children[i]->GetVisibleMaxWidth();
		if (childWidth > maxWidth)
		{
			maxWidth = childWidth; 
		}
	}
	return maxWidth;
}	

int TreeNode::GetVisibleMaxWidth()
{
	return m_iMaxVisibleWidth;
}

int TreeNode::GetDepth()
{
    int depth = 0;
        TreeNode *pParent = GetParentNode();
    while (pParent)
    {								
        depth++;
        pParent = pParent->GetParentNode();
    }
    return depth;
}

bool TreeNode::HasParent(TreeNode *pTreeNode)
{
    TreeNode *pParent = GetParentNode();
    while (pParent)
    {
        if (pParent == pTreeNode)
            return true;
		pParent = pParent->GetParentNode();
    }
    return false;
}

bool TreeNode::IsBeingDisplayed()
{
    TreeNode *pParent = GetParentNode();
    while (pParent)
    {
        // our parents aren't showing us
        if (!pParent->m_bExpand)
            return false;

        pParent = pParent->GetParentNode();
    }
    return true;
}

void TreeNode::SetVisible(bool state)
{
    BaseClass::SetVisible(state);

    bool bChildrenVisible = state && m_bExpand;
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        m_Children[i]->SetVisible(bChildrenVisible);
    }
}

void TreeNode::Paint()
{
    if (GetChildrenCount() > 0 || m_bExpandableWithoutChildren)
    {
        m_pExpandImage->Paint();
    }

    // set image
    int imageIndex = 0;
    if (IsSelected())
    {
        imageIndex = m_pData->GetInt("SelectedImage", 0);
    }
    else
    {
        imageIndex = m_pData->GetInt("Image", 0);
    }

	if (imageIndex)
	{
		IImage *pImage = m_pTreeView->GetImage(imageIndex);
		if (pImage)
		{
			m_pImagePanel->SetImage(pImage);
		}
		m_pImagePanel->Paint();
	}

    m_pText->Paint();
}

void TreeNode::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    SetBorder( NULL );
	SetFgColor( m_pTreeView->GetFgColor() );
	SetBgColor( m_pTreeView->GetBgColor() );
	SetFont( m_pTreeView->GetFont() );
}

void TreeNode::SetSelectionTextColor( const Color& clr )
{
	if ( m_pText )
	{
		m_pText->SetSelectionTextColor( clr );
	}
}

void TreeNode::SetSelectionBgColor( const Color& clr )
{
	if ( m_pText )
	{
		m_pText->SetSelectionBgColor( clr );
	}
}

void TreeNode::SetSelectionUnfocusedBgColor( const Color& clr )
{
	if ( m_pText )
	{
		m_pText->SetSelectionUnfocusedBgColor( clr );
	}
}

void TreeNode::SetBgColor( Color color )
{
	BaseClass::SetBgColor( color );
	if ( m_pText )
	{
		m_pText->SetBgColor( color );
	}

}

void TreeNode::SetFgColor( Color color )
{
	BaseClass::SetFgColor( color );
	if ( m_pText )
	{
		m_pText->SetFgColor( color );
	}
}

void TreeNode::OnSetFocus()
{
    m_pText->RequestFocus();
}

int TreeNode::GetPrevChildItemIndex( TreeNode *pCurrentChild )
{
	int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if ( m_Children[i] == pCurrentChild )
		{
			if ( i <= 0 )
				return -1;

			TreeNode *pChild = m_Children[i-1];
			return pChild->m_ItemIndex;
		}
    }
	return -1;
}

int TreeNode::GetNextChildItemIndex( TreeNode *pCurrentChild )
{
	int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if ( m_Children[i] == pCurrentChild )
		{
			if ( i >= GetChildrenCount() - 1 )
				return -1;

			TreeNode *pChild = m_Children[i+1];
			return pChild->m_ItemIndex;
		}
    }
	return -1;
}

void TreeNode::SelectPrevChild(TreeNode *pCurrentChild)
{
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if (m_Children[i] == pCurrentChild)
            break;
    }

    // this shouldn't happen
    if (i == GetChildrenCount())
    {
        Assert(0);
        return;
    }

    // were we on the first child?
    if (i == 0)
    {
        // if so, then we take over!
        m_pTreeView->AddSelectedItem( m_ItemIndex, true );
    }
    else
    {
        // see if we need to find a grandchild of the previous sibling 
        TreeNode *pChild = m_Children[i-1];

        // if this child is expanded with children, then we have to find the last child
        while (pChild->m_bExpand && pChild->GetChildrenCount()>0)
        {
            // find the last child
            pChild = pChild->m_Children[pChild->GetChildrenCount()-1];
        }
        m_pTreeView->AddSelectedItem( pChild->m_ItemIndex, true );
    }
}

void TreeNode::SelectNextChild(TreeNode *pCurrentChild)
{
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if (m_Children[i] == pCurrentChild)
            break;
    }

    // this shouldn't happen
    if (i == GetChildrenCount())
    {
        Assert(0);
        return;
    }

    // were we on the last child?
    if (i == GetChildrenCount() - 1)
    {
        // tell our parent to get the next child
		if (GetParentNode())
        {
			GetParentNode()->SelectNextChild(this);
		}
    }
    else
    {
        m_pTreeView->AddSelectedItem( m_Children[i+1]->m_ItemIndex, true );
    }
}

void TreeNode::ClosePreviousParents( TreeNode *pPreviousParent )
{
	// close up all the open nodes we've just stepped out of.
	CUtlVector< int > selected;
	m_pTreeView->GetSelectedItems( selected );
	if ( selected.Count() == 0 )
	{
		Assert( 0 );
		return;
	}

	// Most recently clicked item
	TreeNode *selectedItem = m_pTreeView->GetItem( selected[ 0 ] );
	TreeNode *pNewParent = selectedItem->GetParentNode();
	if ( pPreviousParent && pNewParent )
	{
		while ( pPreviousParent->m_ItemIndex > pNewParent->m_ItemIndex )
		{
			pPreviousParent->SetNodeExpanded(false);
			pPreviousParent = pPreviousParent->GetParentNode();
		}
	}
}

void TreeNode::StepInto( bool bClosePrevious )
{
	if ( !m_bExpand )
    {
        SetNodeExpanded(true);
    }

    if ( ( GetChildrenCount() > 0 ) && m_bExpand )
    {
        m_pTreeView->AddSelectedItem( m_Children[0]->m_ItemIndex, true );
    }
    else if ( GetParentNode() )
    {
		TreeNode *pParent = GetParentNode();
        pParent->SelectNextChild(this);

		if ( bClosePrevious )
		{
			ClosePreviousParents( pParent );
		}
    }
}

void TreeNode::StepOut( bool bClosePrevious )
{
	TreeNode *pParent = GetParentNode();
	if ( pParent )
	{
		m_pTreeView->AddSelectedItem( pParent->m_ItemIndex, true );
		if ( pParent->GetParentNode() )
		{
			pParent->GetParentNode()->SelectNextChild(pParent);
		}
		if ( bClosePrevious )
		{
			ClosePreviousParents( pParent );
		}
		else
		{
			pParent->SetNodeExpanded(true);
		}
	}
}

void TreeNode::StepOver( bool bClosePrevious )
{
	TreeNode *pParent = GetParentNode();
	if ( pParent )
	{
		GetParentNode()->SelectNextChild(this);
		if ( bClosePrevious )
		{
			ClosePreviousParents( pParent );
		}
	}
}

void TreeNode::OnKeyCodeTyped(KeyCode code)
{
    switch (code)
    {
        case KEY_LEFT:
        {
            if (m_bExpand && GetChildrenCount() > 0)
            {
                SetNodeExpanded(false);
            }
            else
            {
                if (GetParentNode())
                {
                    m_pTreeView->AddSelectedItem( GetParentNode()->m_ItemIndex, true );
                }
            }
            break;
        }
        case KEY_RIGHT:
        {
            if (!m_bExpand)
            {
                SetNodeExpanded(true);
            }
            else if (GetChildrenCount() > 0)
            {
				m_pTreeView->AddSelectedItem( m_Children[0]->m_ItemIndex, true );
            }
            break;
        }
        case KEY_UP:
        {
            if (GetParentNode())
            {
                GetParentNode()->SelectPrevChild(this);
            }
            break;
        }
        case KEY_DOWN:
        {
            if (GetChildrenCount() > 0 && m_bExpand)
            {
                m_pTreeView->AddSelectedItem( m_Children[0]->m_ItemIndex, true );
            }
            else if (GetParentNode())
            {
                GetParentNode()->SelectNextChild(this);
            }
            break;
        }
		case KEY_SPACE:
        {
			bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
			bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
			bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
			if ( shift )
			{
				StepOut( !ctrl );
			}
			else if ( alt )
			{
				StepOver( !ctrl );
			}
			else
			{
				StepInto( !ctrl );
			}
			break;
        }
		case KEY_I:
		{
			StepInto();
			break;
		}
		case KEY_U:
		{
			StepOut();
			break;
		}
		case KEY_O:
		{
			StepOver();
			break;
		}
		case KEY_ESCAPE:
			{
				if ( m_pTreeView->GetSelectedItemCount() > 0 )
				{
					m_pTreeView->ClearSelection();
				}
				else
				{
					BaseClass::OnKeyCodeTyped(code);
				}
			}
			break;
		case KEY_A:
			{
				bool ctrldown = input()->IsKeyDown( KEY_LCONTROL ) ||  input()->IsKeyDown( KEY_RCONTROL );
				if ( ctrldown )
				{
					m_pTreeView->SelectAll();
				}
				else
				{
					BaseClass::OnKeyCodeTyped(code);
				}
			}
			break;
        default:
            BaseClass::OnKeyCodeTyped(code);
            return;
    }
}

void TreeNode::OnMouseWheeled(int delta)
{
	CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
}

void TreeNode::OnMouseDoublePressed( MouseCode code )
{
	int x, y;
	input()->GetCursorPos(x, y);

	if (code == MOUSE_LEFT)
	{
		ScreenToLocal(x, y);
		if (x > TREE_INDENT_AMOUNT)
		{
			SetNodeExpanded(!m_bExpand);
		}
	}
}

bool TreeNode::IsDragEnabled() const
{
	int x, y;
	input()->GetCursorPos(x, y);
	((TreeNode *)this)->ScreenToLocal(x, y);
	if ( x < TREE_INDENT_AMOUNT )
		return false;

	return BaseClass::IsDragEnabled();
}

void TreeNode::OnMouseReleased(MouseCode code)
{
	BaseClass::OnMouseReleased( code );

	if ( input()->GetMouseCapture() == GetVPanel() )
	{
		input()->SetMouseCapture( NULL );
		return;
	}
	int x, y;
	input()->GetCursorPos(x, y);
	ScreenToLocal(x, y);

	if ( x < TREE_INDENT_AMOUNT )
		return;

	bool ctrldown = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool shiftdown = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));

	if ( !ctrldown && !shiftdown && ( code == MOUSE_LEFT ) )
	{
		m_pTreeView->AddSelectedItem( m_ItemIndex, true );
	}
}

void TreeNode::OnCursorMoved( int x, int y )
{
	if ( input()->GetMouseCapture() != GetVPanel() )
		return;

	LocalToScreen( x, y );
	m_pTreeView->ScreenToLocal( x, y );
	int newItem = m_pTreeView->FindItemUnderMouse( x, y );
	if ( newItem == -1 )
	{
		// Fixme:  Figure out best item
		return;
	}

	int startItem = m_nClickedItem;
	int endItem = newItem;
	if ( startItem > endItem )
	{
		int temp = startItem;
		startItem = endItem;
		endItem = temp;
	}

	CUtlVector< TreeNode * > list;
	m_pTreeView->m_pRootNode->FindNodesInRange( list, startItem, endItem );

	int c = list.Count();
	for ( int i = 0; i < c; ++i )
	{
		TreeNode *item = list[ i ];
		if ( m_bClickedSelected )
		{
			m_pTreeView->AddSelectedItem( item->m_ItemIndex, false );
		}
		else
		{
			m_pTreeView->RemoveSelectedItem( item->m_ItemIndex );
		}
	}
}

void TreeNode::OnMousePressed( MouseCode code)
{
	BaseClass::OnMousePressed( code );

	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	int x, y;
	input()->GetCursorPos(x, y);

	bool bExpandTree = m_pTreeView->m_bLeftClickExpandsTree;

	if ( code == MOUSE_LEFT )
	{
		ScreenToLocal(x, y);
		if ( x < TREE_INDENT_AMOUNT )
		{
			if ( bExpandTree )
			{
				SetNodeExpanded(!m_bExpand);
			}
			// m_pTreeView->SetSelectedItem(m_ItemIndex);    // explorer doesn't actually select item when it expands an item
			// purposely commented out in case we want to change the behavior
		}
		else
		{
			m_nClickedItem = m_ItemIndex;
			if ( m_pTreeView->IsMultipleItemDragEnabled() )
			{
				input()->SetMouseCapture( GetVPanel() );
			}

			if ( shift )
			{
				m_pTreeView->RangeSelectItems( m_ItemIndex );
			}
			else
			{
				if ( !IsSelected() || ctrl )
				{
					if ( IsSelected() && ctrl )
					{
						m_pTreeView->RemoveSelectedItem( m_ItemIndex );
					}
					else
					{
						m_pTreeView->AddSelectedItem( m_ItemIndex, !ctrl );
					}
				}
				else if ( IsSelected() && m_pTreeView->IsMultipleItemDragEnabled() )
				{
					m_pTreeView->AddSelectedItem( m_ItemIndex, !shift );
				}
			}

			m_bClickedSelected = m_pTreeView->IsItemSelected( m_ItemIndex );
		}
	}
	else if (code == MOUSE_RIGHT)
	{
		// context menu selection
		// If the item was selected, leave selected items alone, otherwise make it the only selected item
		if ( !m_pTreeView->IsItemSelected( m_ItemIndex ) )
		{
			m_pTreeView->AddSelectedItem( m_ItemIndex, true );
		}

		// ask parent to context menu
		m_pTreeView->GenerateContextMenu(m_ItemIndex, x, y);
	}
}

void TreeNode::RemoveChildren()
{
	int c = m_Children.Count();
	for ( int i = c - 1 ; i >= 0 ; --i )
	{
		m_pTreeView->RemoveItem( m_Children[ i ]->m_ItemIndex, false, true );
	}
	m_Children.RemoveAll();
}

void TreeNode::FindNodesInRange( CUtlVector< TreeNode * >& list, int startIndex, int endIndex )
{
	list.RemoveAll();
	bool finished = false;
	bool foundstart = false;
	FindNodesInRange_R( list, finished, foundstart, startIndex, endIndex );
}

void TreeNode::FindNodesInRange_R( CUtlVector< TreeNode * >& list, bool& finished, bool& foundStart, int startIndex, int endIndex )
{
	if ( finished )
		return;
	if ( foundStart == true )
	{
		list.AddToTail( this );

		if ( m_ItemIndex == startIndex || m_ItemIndex == endIndex )
		{
			finished = true;
			return;
		}
	}
	else if ( m_ItemIndex == startIndex || m_ItemIndex == endIndex )
	{
		foundStart = true;
		list.AddToTail( this );
		if ( startIndex == endIndex )
		{
			finished = true;
			return;
		}
	}

	if ( !m_bExpand )
		return;


	int i;
	int c = GetChildrenCount();
    for (i=0;i<c;i++)
    {
		m_Children[i]->FindNodesInRange_R( list, finished, foundStart, startIndex, endIndex );
    }
}

void TreeNode::PositionAndSetVisibleNodes(int &nStart, int &nCount, int x, int &y)
{
    // position ourselves
    if (nStart == 0)
    {
        BaseClass::SetVisible(true);
        SetPos(x, y);
        y += m_pTreeView->GetRowHeight();      // m_nRowHeight
        nCount--;
    }
    else // still looking for first element
    {
        nStart--;
        BaseClass::SetVisible(false);
    }

    x += TREE_INDENT_AMOUNT;
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if (nCount > 0 && m_bExpand)
        {
            m_Children[i]->PositionAndSetVisibleNodes(nStart, nCount, x, y);
        }
        else
        {
            m_Children[i]->SetVisible(false);   // this will make all grand children hidden as well
        }
    }
}

TreeNode *TreeNode::FindItemUnderMouse( int &nStart, int& nCount, int x, int &y, int mx, int my )
{
    // position ourselves
    if (nStart == 0)
    {
		int posx, posy;
        GetPos(posx, posy);
		if ( my >= posy && my < posy + m_pTreeView->GetRowHeight() )
		{
			return this;
		}
		y += m_pTreeView->GetRowHeight();
        nCount--;
    }
    else // still looking for first element
    {
        nStart--;
    }

    x += TREE_INDENT_AMOUNT;
    int i;
    for (i=0;i<GetChildrenCount();i++)
    {
        if (nCount > 0 && m_bExpand)
        {
            TreeNode *child = m_Children[i]->FindItemUnderMouse(nStart, nCount, x, y, mx, my);
			if ( child != NULL )
			{
				return child;
			}
        }
    }

	return NULL;
}

// counts items above this item including itself
int TreeNode::CountVisibleIndex()
{
    int nCount = 1; // myself
    if (GetParentNode())
    {
        int i;
        for (i=0;i<GetParentNode()->GetChildrenCount();i++)
        {
            if (GetParentNode()->m_Children[i] == this)
                break;

            nCount += GetParentNode()->m_Children[i]->CountVisibleNodes();
        }
        return nCount + GetParentNode()->CountVisibleIndex();
    }
    else
        return nCount;
}


}; // namespace vgui

DECLARE_BUILD_FACTORY( TreeView );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TreeView::TreeView(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	m_bScrollbarExternal[ 0 ] = m_bScrollbarExternal[ 1 ] = false;
    m_nRowHeight = 20;
    m_pRootNode = NULL;
    m_pImageList = NULL;
    m_pSortFunc = NULL;
    m_Font = 0;

    m_pSubPanel = new TreeViewSubPanel(this);
    m_pSubPanel->SetVisible(true);
    m_pSubPanel->SetPos(0,0);

	m_pHorzScrollBar = new ScrollBar(this, "HorizScrollBar", false);
	m_pHorzScrollBar->AddActionSignalTarget(this);
	m_pHorzScrollBar->SetVisible(false);

	m_pVertScrollBar = new ScrollBar(this, "VertScrollBar", true);
	m_pVertScrollBar->SetVisible(false);
	m_pVertScrollBar->AddActionSignalTarget(this);

	m_bAllowLabelEditing = false;
	m_bDragEnabledItems = false;
	m_bDeleteImageListWhenDone = false;
	m_bLabelBeingEdited = false;
	m_bMultipleItemDragging = false;
	m_bLeftClickExpandsTree = true;
	m_bAllowMultipleSelections = false;
	m_nMostRecentlySelectedItem = -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TreeView::~TreeView()
{
	CleanUpImageList();
}


//-----------------------------------------------------------------------------
// Clean up the image list
//-----------------------------------------------------------------------------
void TreeView::CleanUpImageList( )
{
    if ( m_pImageList )
    {
        if ( m_bDeleteImageListWhenDone )
        {
            delete m_pImageList;
        }
		m_pImageList = NULL;
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::SetSortFunc(TreeViewSortFunc_t pSortFunc)
{
    m_pSortFunc = pSortFunc;
}

HFont TreeView::GetFont()
{
	return m_Font;
}	


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::SetFont(HFont font)
{
	Assert( font );
	if ( !font )
		return;

    m_Font = font;
	m_nRowHeight = surface()->GetFontTall(font) + 2;

    if (m_pRootNode)
    {
        m_pRootNode->SetFont(font);
    }
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::GetRowHeight()
{
    return m_nRowHeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::GetVisibleMaxWidth()
{
    if (m_pRootNode)
    {
        return m_pRootNode->GetVisibleMaxWidth();
    }
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::AddItem(KeyValues *data, int parentItemIndex)
{
    Assert(parentItemIndex == -1 || m_NodeList.IsValidIndex(parentItemIndex));

    TreeNode *pTreeNode = new TreeNode(m_pSubPanel, this);
	pTreeNode->SetDragEnabled( m_bDragEnabledItems );
    pTreeNode->m_ItemIndex = m_NodeList.AddToTail(pTreeNode);
    pTreeNode->SetKeyValues(data);

	if ( m_Font != 0 )
	{
		pTreeNode->SetFont( m_Font );
	}
	pTreeNode->SetBgColor( GetBgColor() );

	if ( data->GetInt( "droppable", 0 ) != 0 )
	{
		float flContextDelay = data->GetFloat( "drophoverdelay" );
		if ( flContextDelay )
		{
			pTreeNode->SetDropEnabled( true, flContextDelay );
		}
		else
		{
			pTreeNode->SetDropEnabled( true );
		}
	}

    // there can be only one root
    if (parentItemIndex == -1)
    {
        Assert(m_pRootNode == NULL);
        m_pRootNode = pTreeNode;
        pTreeNode->m_ParentIndex = -1;
    }
    else
    {
        pTreeNode->m_ParentIndex = parentItemIndex;

        // add to parent list
        pTreeNode->GetParentNode()->AddChild(pTreeNode);
    }

	SETUP_PANEL( pTreeNode );

	return pTreeNode->m_ItemIndex;
}


int TreeView::GetRootItemIndex()
{
	if ( m_pRootNode )
		return m_pRootNode->m_ItemIndex;
	else
		return -1;
}


int TreeView::GetNumChildren( int itemIndex )
{
	if ( itemIndex == -1 )
		return 0;

	return m_NodeList[itemIndex]->m_Children.Count();
}


int TreeView::GetChild( int iParentItemIndex, int iChild )
{
	return m_NodeList[iParentItemIndex]->m_Children[iChild]->m_ItemIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : itemIndex - 
// Output : TreeNode
//-----------------------------------------------------------------------------
TreeNode *TreeView::GetItem( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
	{
		Assert( 0 );
		return NULL;
	}

	return m_NodeList[ itemIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::GetItemCount(void)
{
    return m_NodeList.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues* TreeView::GetItemData(int itemIndex)
{
    if (!m_NodeList.IsValidIndex(itemIndex))
        return NULL;
    else
        return m_NodeList[itemIndex]->m_pData;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::RemoveItem(int itemIndex, bool bPromoteChildren, bool bFullDelete )
{
	// HACK: there's a bug with RemoveItem where panels are lingering. This gets around it temporarily.

	// FIXME: Negative item indices is a bogus interface method!
	// because what if you want to recursively remove everything under node 0?
	// Use the bFullDelete parameter instead.
	if ( itemIndex < 0 )
	{
		itemIndex = -itemIndex;
		bFullDelete = true;
	}

	if (!m_NodeList.IsValidIndex(itemIndex))
       return;

    TreeNode *pNode = m_NodeList[itemIndex];
    TreeNode *pParent = pNode->GetParentNode();

    // are we promoting the children
    if (bPromoteChildren && pParent)
    {
        int i;
        for (i=0;i<pNode->GetChildrenCount();i++)
        {
            TreeNode *pChild = pNode->m_Children[i];
            pChild->m_ParentIndex = pParent->m_ItemIndex;
        }
    }
    else
    {
        // delete our children
        if ( bFullDelete )
		{
			while ( pNode->GetChildrenCount() )
				RemoveItem( -pNode->m_Children[0]->m_ItemIndex, false );
		}
		else
		{		
			int i;
			for (i=0;i<pNode->GetChildrenCount();i++)
			{
				TreeNode *pDeleteChild = pNode->m_Children[i];
				RemoveItem(pDeleteChild->m_ItemIndex, false);
			}
		}
    }

    // remove from our parent's children list
    if (pParent)
    {
        pParent->m_Children.FindAndRemove(pNode);
    }

    // finally get rid of ourselves from the main list
    m_NodeList.Remove(itemIndex);
	
	if ( bFullDelete )
		delete pNode;
	else
		pNode->MarkForDeletion();
    
	// Make sure we don't leave ourselves with an invalid selected item.
	m_SelectedItems.FindAndRemove( pNode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::RemoveAll()
{
    int i;
    for (i=0;i<m_NodeList.MaxElementIndex();i++)
    {
        if (!m_NodeList.IsValidIndex(i))
            continue;

        m_NodeList[i]->MarkForDeletion();
    }
    m_NodeList.RemoveAll();
	m_pRootNode = NULL;
	ClearSelection();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TreeView::ModifyItem(int itemIndex, KeyValues *data)
{
    if (!m_NodeList.IsValidIndex(itemIndex))
        return false;

    TreeNode *pNode = m_NodeList[itemIndex];
	TreeNode *pParent = pNode->GetParentNode();
	bool bReSort = ( m_pSortFunc && pParent );
	int nChildIndex = -1;
	if ( bReSort )
	{
		nChildIndex = pParent->FindChild( pNode );
	}

    pNode->SetKeyValues(data);

	// Changing the data can cause it to re-sort
	if ( bReSort )
	{
		int nChildren = pParent->GetChildrenCount();
		bool bLeftBad = (nChildIndex > 0) && m_pSortFunc( pNode->m_pData, pParent->m_Children[nChildIndex-1]->m_pData );
		bool bRightBad = (nChildIndex < nChildren - 1) && m_pSortFunc( pParent->m_Children[nChildIndex+1]->m_pData, pNode->m_pData );
		if ( bLeftBad || bRightBad )
		{
			pParent->m_Children.Remove( nChildIndex );
			pParent->AddChild( pNode );
		}
	}

    InvalidateLayout();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: set the selection colors of an element in the tree view
//-----------------------------------------------------------------------------

void TreeView::SetItemSelectionTextColor( int itemIndex, const Color& clr )
{
	Assert( m_NodeList.IsValidIndex(itemIndex) );
	if ( !m_NodeList.IsValidIndex(itemIndex) )
		return;

	TreeNode *pNode = m_NodeList[itemIndex];
	pNode->SetSelectionTextColor( clr );
}

void TreeView::SetItemSelectionBgColor( int itemIndex, const Color& clr )
{
	Assert( m_NodeList.IsValidIndex(itemIndex) );
	if ( !m_NodeList.IsValidIndex(itemIndex) )
		return;

	TreeNode *pNode = m_NodeList[itemIndex];
	pNode->SetSelectionBgColor( clr );
}

void TreeView::SetItemSelectionUnfocusedBgColor( int itemIndex, const Color& clr )
{
	Assert( m_NodeList.IsValidIndex(itemIndex) );
	if ( !m_NodeList.IsValidIndex(itemIndex) )
		return;

	TreeNode *pNode = m_NodeList[itemIndex];
	pNode->SetSelectionUnfocusedBgColor( clr );
}

//-----------------------------------------------------------------------------
// Purpose: set the fg color of an element in the tree view
//-----------------------------------------------------------------------------
void TreeView::SetItemFgColor(int itemIndex, const Color& color)
{
	Assert( m_NodeList.IsValidIndex(itemIndex) );
	if ( !m_NodeList.IsValidIndex(itemIndex) )
		return;

	TreeNode *pNode = m_NodeList[itemIndex];
	pNode->SetFgColor( color );
}

//-----------------------------------------------------------------------------
// Purpose: set the bg color of an element in the tree view
//-----------------------------------------------------------------------------
void TreeView::SetItemBgColor(int itemIndex, const Color& color)
{
	Assert( m_NodeList.IsValidIndex(itemIndex) );
	if ( !m_NodeList.IsValidIndex(itemIndex) )
		return;

	TreeNode *pNode = m_NodeList[itemIndex];
	pNode->SetBgColor( color );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::GetItemParent(int itemIndex)
{
	return m_NodeList[itemIndex]->m_ParentIndex;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::SetImageList(ImageList *imageList, bool deleteImageListWhenDone)
{
	CleanUpImageList();
    m_pImageList = imageList;
    m_bDeleteImageListWhenDone = deleteImageListWhenDone;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IImage *TreeView::GetImage(int index)
{
    return m_pImageList->GetImage(index);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::GetSelectedItems( CUtlVector< int >& list )
{
	list.RemoveAll();

	int c = m_SelectedItems.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		list.AddToTail( m_SelectedItems[ i ]->m_ItemIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::GetSelectedItemData( CUtlVector< KeyValues * >& list )
{
	list.RemoveAll();

	int c = m_SelectedItems.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		list.AddToTail( m_SelectedItems[ i ]->m_pData );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TreeView::IsItemIDValid(int itemIndex)
{
    return m_NodeList.IsValidIndex(itemIndex);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int TreeView::GetHighestItemID()
{
    return m_NodeList.MaxElementIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::ExpandItem(int itemIndex, bool bExpand)
{
    if (!m_NodeList.IsValidIndex(itemIndex))
        return;

    m_NodeList[itemIndex]->SetNodeExpanded(bExpand);
    InvalidateLayout();
}

bool TreeView::IsItemExpanded( int itemIndex )
{
    if (!m_NodeList.IsValidIndex(itemIndex))
        return false;

    return m_NodeList[itemIndex]->IsExpanded();
}
	

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void TreeView::OnMouseWheeled(int delta)
{
	if ( !m_pVertScrollBar->IsVisible() )
	{
		return;
	}
	int val = m_pVertScrollBar->GetValue();
	val -= (delta * 3);
	m_pVertScrollBar->SetValue(val);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
	Repaint();
}

void TreeView::GetScrollBarSize( bool vertical, int& w, int& h )
{
	int idx = vertical ? 0 : 1;

	if ( m_bScrollbarExternal[ idx ] )
	{
		w = h = 0;
		return;
	}

	if ( vertical )
	{
		m_pVertScrollBar->GetSize( w, h );
	}
	else
	{
		m_pHorzScrollBar->GetSize( w, h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::PerformLayout()
{
    int wide, tall;
    GetSize( wide, tall );

    if ( !m_pRootNode )
	{
		m_pSubPanel->SetSize( wide, tall );
        return;
	}

	int sbhw, sbhh;
	GetScrollBarSize( false, sbhw, sbhh );
	int sbvw, sbvh;
	GetScrollBarSize( true, sbvw, sbvh );

    bool vbarNeeded = false;
    bool hbarNeeded = false;

    // okay we have to check if we need either scroll bars, since if we need one
    // it might make it necessary to have the other one
    int nodesVisible = tall / m_nRowHeight;

	// count the number of visible items
	int visibleItemCount = m_pRootNode->CountVisibleNodes();
    int maxWidth = m_pRootNode->GetVisibleMaxWidth() + 10; // 10 pixel buffer

    vbarNeeded = visibleItemCount > nodesVisible;

    if (!vbarNeeded)
    {
        if (maxWidth > wide)
        {
            hbarNeeded = true;

            // recalculate if vbar is needed now
            // double check that we really don't need it
            nodesVisible = (tall - sbhh) / m_nRowHeight;
            vbarNeeded = visibleItemCount > nodesVisible;
        }
    }
    else
    {
        // we've got the vertical bar here, so shrink the width
        hbarNeeded = maxWidth > (wide - (sbvw+2));

        if (hbarNeeded)
        {
            nodesVisible = (tall - sbhh) / m_nRowHeight;
        }
    }

    int subPanelWidth = wide;
    int subPanelHeight = tall;

	int vbarPos = 0;
    if (vbarNeeded)
    {
        subPanelWidth -= (sbvw + 2);
        int barSize = tall;
        if (hbarNeeded)
        {
            barSize -= sbhh;
        }

    	//!! need to make it recalculate scroll positions
    	m_pVertScrollBar->SetVisible(true);
    	m_pVertScrollBar->SetEnabled(false);
    	m_pVertScrollBar->SetRangeWindow( nodesVisible );
    	m_pVertScrollBar->SetRange( 0, visibleItemCount);	
    	m_pVertScrollBar->SetButtonPressedScrollValue( 1 );

		if ( !m_bScrollbarExternal[ 0 ] )
		{
    		m_pVertScrollBar->SetPos(wide - (sbvw + WINDOW_BORDER_WIDTH), 0);
    		m_pVertScrollBar->SetSize(sbvw, barSize - 2);
		}

        // need to figure out
        vbarPos = m_pVertScrollBar->GetValue();
    }
    else
    {
    	m_pVertScrollBar->SetVisible(false);
		m_pVertScrollBar->SetValue( 0 );
    }

    int hbarPos = 0;
    if (hbarNeeded)
    {
        subPanelHeight -= (sbhh + 2);
        int barSize = wide;
        if (vbarNeeded)
        {
            barSize -= sbvw;
        }
        m_pHorzScrollBar->SetVisible(true);
        m_pHorzScrollBar->SetEnabled(false);
        m_pHorzScrollBar->SetRangeWindow( barSize );
        m_pHorzScrollBar->SetRange( 0, maxWidth);	
        m_pHorzScrollBar->SetButtonPressedScrollValue( 10 );

		if ( !m_bScrollbarExternal[ 1 ] )
		{
			m_pHorzScrollBar->SetPos(0, tall - (sbhh + WINDOW_BORDER_WIDTH));
			m_pHorzScrollBar->SetSize(barSize - 2, sbhh);
		}

        hbarPos = m_pHorzScrollBar->GetValue();
    }
    else
    {
    	m_pHorzScrollBar->SetVisible(false);
		m_pHorzScrollBar->SetValue( 0 );
    }

    m_pSubPanel->SetSize(subPanelWidth, subPanelHeight);

	int y = 0;
    m_pRootNode->PositionAndSetVisibleNodes(vbarPos, visibleItemCount, -hbarPos, y);
    
    Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::MakeItemVisible(int itemIndex)
{
    // first make sure that all parents are expanded
    TreeNode *pNode = m_NodeList[itemIndex];
    TreeNode *pParent = pNode->GetParentNode();
    while (pParent)
    {
        if (!pParent->m_bExpand)
        {
            pParent->SetNodeExpanded(true);
        }
        pParent = pParent->GetParentNode();
    }

    // recalculate scroll bar due to possible exapnsion
    PerformLayout();

    if (!m_pVertScrollBar->IsVisible())
        return;

    int visibleIndex = pNode->CountVisibleIndex()-1;
    int range = m_pVertScrollBar->GetRangeWindow();
    int vbarPos = m_pVertScrollBar->GetValue();

    // do we need to scroll up or down?
    if (visibleIndex < vbarPos)
    {
        m_pVertScrollBar->SetValue(visibleIndex);
    }
    else if (visibleIndex+1 > vbarPos+range)
    {
        m_pVertScrollBar->SetValue(visibleIndex+1-range);
    }
    InvalidateLayout();
}

void TreeView::GetVBarInfo( int &top, int &nItemsVisible, bool& hbarVisible )
{
    int wide, tall;
    GetSize( wide, tall );
    nItemsVisible = tall / m_nRowHeight;

    if ( m_pVertScrollBar->IsVisible() )
	{
		top = m_pVertScrollBar->GetValue();
	}
	else
	{
		top = 0;
	}
	hbarVisible = m_pHorzScrollBar->IsVisible();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("TreeView.BgColor", GetSchemeColor("WindowDisabledBgColor", pScheme), pScheme));
	SetFont( pScheme->GetFont( "Default", IsProportional() ) );
	m_pSubPanel->SetBgColor( GetBgColor() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::SetBgColor( Color color ) 
{
	BaseClass::SetBgColor( color );
	m_pSubPanel->SetBgColor( color );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::OnSliderMoved( int position )
{
	InvalidateLayout();
	Repaint();
}

void TreeView::GenerateDragDataForItem( int itemIndex, KeyValues *msg )
{
	// Implemented by subclassed TreeView
}

void TreeView::SetDragEnabledItems( bool state )
{
	m_bDragEnabledItems = state;
}

void TreeView::OnLabelChanged( int itemIndex, char const *oldString, char const *newString )
{
}

bool TreeView::IsLabelEditingAllowed() const
{
	return m_bAllowLabelEditing;
}

void TreeView::SetLabelBeingEdited( bool state )
{
	m_bLabelBeingEdited = state;
}

bool TreeView::IsLabelBeingEdited() const
{
	return m_bLabelBeingEdited;
}

void TreeView::SetAllowLabelEditing( bool state )
{
	m_bAllowLabelEditing = state;
}

void TreeView::EnableExpandTreeOnLeftClick( bool bEnable )
{
	m_bLeftClickExpandsTree = bEnable;
}

int TreeView::FindItemUnderMouse( int mx, int my )
{
	mx = clamp( mx, 0, GetWide() - 1 );
	my = clamp( my, 0, GetTall() - 1 );
	if ( mx >= TREE_INDENT_AMOUNT )
	{
		// Find what's under this position
		// need to figure out
		int vbarPos = m_pVertScrollBar->IsVisible() ? m_pVertScrollBar->GetValue() : 0;
		int hbarPos = m_pHorzScrollBar->IsVisible() ? m_pHorzScrollBar->GetValue() : 0;
		int count = m_pRootNode->CountVisibleNodes();

		int y = 0;
		TreeNode *item = m_pRootNode->FindItemUnderMouse( vbarPos, count, -hbarPos, y, mx, my );
		if ( item  )
		{
			return item->m_ItemIndex;
		}
	}

	return -1;
}

void TreeView::OnMousePressed( MouseCode code )
{
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));

	// Try to map mouse position to a row
	if ( code == MOUSE_LEFT && m_pRootNode )
	{
		int mx, my;
		input()->GetCursorPos( mx, my );
		ScreenToLocal( mx, my );
		if ( mx >= TREE_INDENT_AMOUNT )
		{
			// Find what's under this position
			// need to figure out
			int vbarPos = m_pVertScrollBar->IsVisible() ? m_pVertScrollBar->GetValue() : 0;
			int hbarPos = m_pHorzScrollBar->IsVisible() ? m_pHorzScrollBar->GetValue() : 0;
			int count = m_pRootNode->CountVisibleNodes();

			int y = 0;
			TreeNode *item = m_pRootNode->FindItemUnderMouse( vbarPos, count, -hbarPos, y, mx, my );
			if ( item  )
			{
				if ( !item->IsSelected() )
				{
					AddSelectedItem( item->m_ItemIndex, !ctrl && !shift );
				}
				return;
			}
			else
			{
				ClearSelection();
			}
		}
	}

	BaseClass::OnMousePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void TreeView::SetAllowMultipleSelections( bool state )
{
	m_bAllowMultipleSelections = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool TreeView::IsMultipleSelectionAllowed() const
{
	return m_bAllowMultipleSelections;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : int
//-----------------------------------------------------------------------------
int TreeView::GetSelectedItemCount() const
{
	return m_SelectedItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void TreeView::ClearSelection()
{
	m_SelectedItems.RemoveAll();
	m_nMostRecentlySelectedItem = -1;
	PostActionSignal( new KeyValues( "TreeViewItemSelectionCleared" ) );
}

void TreeView::RangeSelectItems( int endItem )
{
	int startItem = m_nMostRecentlySelectedItem;
	ClearSelection();
	m_nMostRecentlySelectedItem = startItem;

	if ( !m_NodeList.IsValidIndex( startItem ) )
	{
		AddSelectedItem( endItem, false );
		return;
	}

	Assert( m_NodeList.IsValidIndex( endItem ) );

	if ( !m_pRootNode )
	{
		return;
	}

	CUtlVector< TreeNode * > list;
	m_pRootNode->FindNodesInRange( list, startItem, endItem );

	int c = list.Count();
	for ( int i = 0; i < c; ++i )
	{
		TreeNode *item = list[ i ];
		AddSelectedItem( item->m_ItemIndex, false );
	}
}

void TreeView::FindNodesInRange( int startItem, int endItem, CUtlVector< int >& itemIndices )
{
	CUtlVector< TreeNode * > nodes;
	m_pRootNode->FindNodesInRange( nodes, startItem, endItem );

	int c = nodes.Count();
	for ( int i = 0; i < c; ++i )
	{
		TreeNode *item = nodes[ i ];
		itemIndices.AddToTail( item->m_ItemIndex );
	}
}

void TreeView::RemoveSelectedItem( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return;

	TreeNode *sel = m_NodeList[ itemIndex ];
	Assert( sel );
	int slot = m_SelectedItems.Find( sel );
	if ( slot != m_SelectedItems.InvalidIndex() )
	{
		m_SelectedItems.Remove( slot );
		PostActionSignal( new KeyValues( "TreeViewItemDeselected", "itemIndex", itemIndex ) );

		m_nMostRecentlySelectedItem = itemIndex;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TreeView::AddSelectedItem( int itemIndex, bool clearCurrentSelection, bool requestFocus /* = true */, bool bMakeItemVisible /*= true*/ )
{
	if ( clearCurrentSelection )
	{
		ClearSelection();
	}

	// Assume it's bogus
    if ( !m_NodeList.IsValidIndex( itemIndex ) )
        return;

    TreeNode *sel = m_NodeList[ itemIndex ];
	Assert( sel );
	if ( requestFocus )
	{
		sel->RequestFocus();
	}

	// Item 0 is most recently selected!!!
	int slot = m_SelectedItems.Find( sel );
	if ( slot == m_SelectedItems.InvalidIndex() )
	{
		m_SelectedItems.AddToHead( sel );
	}
	else if ( slot != 0 )
	{
		m_SelectedItems.Remove( slot );
		m_SelectedItems.AddToHead( sel );
	}

	if ( bMakeItemVisible )
	{
		MakeItemVisible( itemIndex );
	}

    PostActionSignal( new KeyValues( "TreeViewItemSelected", "itemIndex", itemIndex ) );
    InvalidateLayout();

	if ( clearCurrentSelection )
	{
		m_nMostRecentlySelectedItem = itemIndex;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : int
//-----------------------------------------------------------------------------
int TreeView::GetFirstSelectedItem() const
{
	if ( m_SelectedItems.Count() <= 0 )
		return -1;
	return m_SelectedItems[ 0 ]->m_ItemIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : itemIndex - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool TreeView::IsItemSelected( int itemIndex )
{
	// Assume it's bogus
    if ( !m_NodeList.IsValidIndex( itemIndex ) )
        return false;

    TreeNode *sel = m_NodeList[ itemIndex ];
	return m_SelectedItems.Find( sel ) != m_SelectedItems.InvalidIndex();
}

void TreeView::SetLabelEditingAllowed( int itemIndex, bool state )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return;

	TreeNode *sel = m_NodeList[ itemIndex ];
	sel->SetLabelEditingAllowed( state );
}

void  TreeView::StartEditingLabel( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return;

	Assert( IsLabelEditingAllowed() );

	TreeNode *sel = m_NodeList[ itemIndex ];
	Assert( sel->IsLabelEditingAllowed() );
	if ( !sel->IsLabelEditingAllowed() )
		return;

	sel->EditLabel();
}

int TreeView::GetPrevChildItemIndex( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return -1;
	TreeNode *sel = m_NodeList[ itemIndex ];
	TreeNode *parent = sel->GetParentNode();
	if ( !parent )
		return -1;

	return parent->GetPrevChildItemIndex( sel );
}

int TreeView::GetNextChildItemIndex( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return -1;
	TreeNode *sel = m_NodeList[ itemIndex ];
	TreeNode *parent = sel->GetParentNode();
	if ( !parent )
		return -1;

	return parent->GetNextChildItemIndex( sel );
}

bool TreeView::IsItemDroppable( int itemIndex, CUtlVector< KeyValues * >& msglist )
{
	// Derived classes should implement
	return false;
}

void TreeView::OnItemDropped( int itemIndex, CUtlVector< KeyValues * >& msglist )
{
}

bool TreeView::GetItemDropContextMenu( int itemIndex, Menu *menu, CUtlVector< KeyValues * >& msglist )
{
	return false;
}

HCursor TreeView::GetItemDropCursor( int itemIndex, CUtlVector< KeyValues * >& msglist )
{
	return dc_arrow;
}

void TreeView::RemoveChildrenOfNode( int itemIndex )
{
	if ( !m_NodeList.IsValidIndex( itemIndex ) )
		return;

	TreeNode *node = m_NodeList[ itemIndex ];
	node->RemoveChildren();
}

ScrollBar *TreeView::SetScrollBarExternal( bool vertical, Panel *newParent )
{
	if ( vertical )
	{
		m_bScrollbarExternal[ 0 ] = true;
		m_pVertScrollBar->SetParent( newParent );
		return m_pVertScrollBar;
	}
	m_bScrollbarExternal[ 1 ] = true;
	m_pHorzScrollBar->SetParent( newParent );
	return m_pHorzScrollBar;
}

// if this is set, then clicking on one row and dragging will select a run or items, etc.
void TreeView::SetMultipleItemDragEnabled( bool state )
{
	m_bMultipleItemDragging = state;
}

bool TreeView::IsMultipleItemDragEnabled() const
{
	return m_bMultipleItemDragging;
}

void TreeView::SelectAll()
{
	m_SelectedItems.RemoveAll();
	FOR_EACH_LL( m_NodeList, i )
	{
		m_SelectedItems.AddToTail( m_NodeList[ i ] );
	}

	PostActionSignal( new KeyValues( "TreeViewItemSelected", "itemIndex", GetRootItemIndex() ) );
	InvalidateLayout();
}
