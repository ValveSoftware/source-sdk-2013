//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TREEVIEWLISTCONTROL_H
#define TREEVIEWLISTCONTROL_H
#ifdef _WIN32
#pragma once
#endif


#include <utllinkedlist.h>
#include <utlvector.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include "utlsymbol.h"


namespace vgui
{

// --------------------------------------------------------------------------------- //
// CTreeViewListControl
//
// This control has N columns, with a tree view in the leftmost column.
// --------------------------------------------------------------------------------- //

class CTreeViewListControl : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTreeViewListControl, Panel );

public:

	CTreeViewListControl( vgui::Panel *pParent, const char *pName );

	// Set the tree view to be displayed on the left. If this isn't set, then nothing displays in here.
	virtual void SetTreeView( vgui::TreeView *pTree );

	// Set the height of the title bar.
	virtual void SetTitleBarInfo( vgui::HFont hFont, int titleBarHeight );

	// Set the color to draw the border lines in.
	virtual void SetBorderColor( Color clr );
	
	// Initialize the column headers.. This info includes the tree view on the left, so this 
	virtual void SetNumColumns( int nColumns );
	virtual int GetNumColumns() const;
	// ciFlags is a combination of CI_ flags.
	virtual void SetColumnInfo( int iColumn, const char *pTitle, int width, int ciFlags=0 );

	// Use this to render your stuff. Iterate over the rows in the tree view and 
	virtual int GetNumRows();
	virtual int GetTreeItemAtRow( int iRow ); // You can use m_pTree->GetItemData to get at the data for the row.

	// Use this to find out the client area to render in for each grid element.
	// The returned box is inclusive.
	// The rule is that the the top and left pixels in each grid element are reserved for lines.
	virtual void GetGridElementBounds( int iColumn, int iRow, int &left, int &top, int &right, int &bottom );

	virtual vgui::TreeView *GetTree();

	virtual int	GetTitleBarHeight();

	virtual int	GetScrollBarSize();

// Overrides.
public:

	// This is where it recalculates the row infos.
	virtual void PerformLayout();
	
	// Usually, you'll want to override paint. After calling the base, use GetNumRows() to 
	// iterate over the data in the tree control and fill in the other columns.
	virtual void Paint();
	virtual void PostChildPaint();

	// You can override this to change the way the title bars are drawn.
	virtual void DrawTitleBars();


public:

	enum
	{
		// By default, column header text is centered.
		CI_HEADER_LEFTALIGN	=0x0001
	};
	

protected:

	void RecalculateRows();
	void RecalculateRows_R( int index );
	void RecalculateColumns();

private:

	vgui::TreeView *m_pTree;

	class CColumnInfo
	{
	public:
		CColumnInfo()
		{
			m_Width = m_Left = m_Right = m_ciFlags = 0;
		}

		CUtlSymbol m_Title;
		int m_Width;
		int m_Left;
		int m_Right;
		int m_ciFlags;	// Combination of CI_ flags.
	};
	CUtlVector<CColumnInfo> m_Columns;
	
	vgui::HFont m_TitleBarFont;
	int m_TitleBarHeight;

	// These are indices into the tree view.
	CUtlVector<int> m_Rows;

	Color m_BorderColor;
};

} // namespace


#endif // TREEVIEWLISTCONTROL_H
