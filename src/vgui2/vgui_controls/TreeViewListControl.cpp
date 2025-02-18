//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <assert.h>

#define PROTECTED_THINGS_DISABLE

#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include <vgui/IBorder.h>

#include <vgui_controls/TreeViewListControl.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/TreeView.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( CTreeViewListControl );

CTreeViewListControl::CTreeViewListControl( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName )
{
	m_pTree = NULL;
	m_BorderColor.SetColor( 255, 255, 255, 255 );
	m_TitleBarFont = NULL;
	m_TitleBarHeight = 20;
	SetPostChildPaintEnabled( true );
}

void CTreeViewListControl::SetTreeView( vgui::TreeView *pTree )
{
	m_pTree = pTree;
	if ( m_pTree )
	{
		m_pTree->SetParent( this );
		m_pTree->SetPaintBackgroundEnabled( false );
	}

	InvalidateLayout();
}

vgui::TreeView *CTreeViewListControl::GetTree()
{
	return m_pTree;
}

int CTreeViewListControl::GetTitleBarHeight()
{
	return m_TitleBarHeight;
}

void CTreeViewListControl::SetTitleBarInfo( vgui::HFont hFont, int titleBarHeight )
{
	m_TitleBarFont = hFont;
	m_TitleBarHeight = titleBarHeight;

	InvalidateLayout();
}

void CTreeViewListControl::SetBorderColor( Color clr )
{
	m_BorderColor = clr;
}

void CTreeViewListControl::SetNumColumns( int nColumns )
{
	m_Columns.Purge();
	m_Columns.SetSize( nColumns );
	InvalidateLayout();
}

int CTreeViewListControl::GetNumColumns() const
{
	return m_Columns.Count();
}

void CTreeViewListControl::SetColumnInfo( int iColumn, const char *pTitle, int width, int ciFlags )
{
	if ( iColumn < 0 || iColumn >= m_Columns.Count() )
	{
		Assert( false );
		return;
	}
	CColumnInfo *pInfo = &m_Columns[iColumn];
	pInfo->m_Title = pTitle;
	pInfo->m_Width = width;
	pInfo->m_ciFlags = ciFlags;

	InvalidateLayout();
}

int CTreeViewListControl::GetNumRows()
{
	return m_Rows.Count();
}

int CTreeViewListControl::GetTreeItemAtRow( int iRow )
{
	if ( iRow < 0 || iRow >= m_Rows.Count() )
		return -1;
	else
		return m_Rows[iRow];
}

void CTreeViewListControl::GetGridElementBounds( int iColumn, int iRow, int &left, int &top, int &right, int &bottom )
{
	left = m_Columns[iColumn].m_Left;
	right = m_Columns[iColumn].m_Right;
	
	// vgui doesn't seem to be drawing things exactly right. Like it you draw a line at (0,0) to (100,0),
	// then a rectangle from (1,1) to (100,100), it'll overwrite the line at the top.
	int treeTopBorder = 0;
	IBorder *treeBorder = m_pTree->GetBorder();
	if ( treeBorder )
	{
		int l, t, r, b;
		treeBorder->GetInset( l, t, r, b );
		treeTopBorder = t;
	}
	if ( iRow == -1 )
	{
		top = 1;
		bottom = m_TitleBarHeight - 2;
	}
	else if ( m_pTree )
	{
		int x, y;
		m_pTree->GetPos( x, y );

		top = treeTopBorder + m_TitleBarHeight + ( iRow * m_pTree->GetRowHeight() );
		bottom = top + m_pTree->GetRowHeight();
	}
	else
	{
		left = top = right = bottom = 0;
	}
}

void CTreeViewListControl::PerformLayout()
{
	RecalculateRows();
	RecalculateColumns();

	// Reposition the tree view.
	if ( m_pTree && m_Columns.Count() > 0 )
	{
		int left, top, right, bottom;
		GetGridElementBounds( 0, -1, left, top, right, bottom );

		top = m_TitleBarHeight;

		m_pTree->SetBounds( left, top, right - left, GetTall() - top );
	}

	BaseClass::PerformLayout();
}


void CTreeViewListControl::RecalculateRows()
{
	m_Rows.Purge();

	if ( !m_pTree || m_pTree->GetRootItemIndex() == -1 )
		return;

	int iRoot = m_pTree->GetRootItemIndex();
	RecalculateRows_R( iRoot );
}


void CTreeViewListControl::RecalculateRows_R( int index )
{
	m_Rows.AddToTail( index );
	if ( !m_pTree->IsItemExpanded( index ) )
		return;
	
	int nChildren = m_pTree->GetNumChildren( index );
	for ( int i=0; i < nChildren; i++ )
	{
		int iChild = m_pTree->GetChild( index, i );
		RecalculateRows_R( iChild );
	}
}

int CTreeViewListControl::GetScrollBarSize()
{
	return 0;
}

void CTreeViewListControl::RecalculateColumns()
{
	int rightEdge = GetWide()-1 - GetScrollBarSize();

	int x = 0;
	int c = m_Columns.Count();
	for ( int i=0; i < c; i++ )
	{
		m_Columns[i].m_Left = x + 1;
		int cw = m_Columns[i].m_Width;
		if ( i == c - 1 )
		{
			cw = rightEdge - x - 2;
		}
		m_Columns[i].m_Right = x + cw - 2;
		x += cw;
	}
}

void CTreeViewListControl::PostChildPaint()
{
	BaseClass::PostChildPaint();

	// Draw the grid lines.
	vgui::surface()->DrawSetColor( m_BorderColor );

	if ( m_Columns.Count() <= 0 )
		return;
	
	// Draw the horizontal lines.
	int endX = 0;
	endX = m_Columns[m_Columns.Count()-1].m_Right + 1;

	int bottomY = 0;
	for ( int i=0; i < m_Rows.Count(); i++ )
	{
		int left, top, right, bottom;
		GetGridElementBounds( 0, i, left, top, right, bottom );

		bottomY = bottom;
		vgui::surface()->DrawLine( 0, bottomY, endX, bottomY );
	}

	// Draw the vertical lines.
	int curX = 0;
	for ( int i=0; i < m_Columns.Count(); i++ )
	{
		vgui::surface()->DrawLine( curX, 0, curX, bottomY );
		curX += m_Columns[i].m_Width;
	}
	vgui::surface()->DrawLine( curX, 0, curX, bottomY );
}

void CTreeViewListControl::Paint()
{
	BaseClass::Paint();

	// Draw the title bars.
	DrawTitleBars();
}

void CTreeViewListControl::DrawTitleBars()
{
	int rightEdge = GetWide();

	for ( int i=0; i < m_Columns.Count(); i++ )
	{
		int left, top, right, bottom;
		GetGridElementBounds( i, -1, left, top, right, bottom );

		if ( left >= rightEdge )
			continue;

		vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
		vgui::surface()->DrawFilledRect( left, top, right, bottom );

		vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );

		const char *pTitleString = m_Columns[i].m_Title.String();

		wchar_t unicodeString[1024];
		g_pVGuiLocalize->ConvertANSIToUnicode( pTitleString, unicodeString, sizeof(unicodeString) );
	
		int wide, tall;
		surface()->GetTextSize( m_TitleBarFont, unicodeString, wide, tall );

		surface()->DrawSetTextFont( m_TitleBarFont );

		if ( m_Columns[i].m_ciFlags & CTreeViewListControl::CI_HEADER_LEFTALIGN )
		{
			int midy = (top+bottom)/2;
			surface()->DrawSetTextPos( left, midy  );
		}
		else
		{
			int textRight = min( right, rightEdge );

			int midx = (left+textRight)/2;
			int midy = (top+bottom)/2;

			surface()->DrawSetTextPos( midx - wide/2, midy - tall/2 );
		}
		
		surface()->DrawPrintText( unicodeString, strlen( pTitleString ) );
	}
}

