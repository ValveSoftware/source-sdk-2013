//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_grid.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


#define AssertCheck(expr, msg) \
	if(!(expr))\
	{\
		assert(!msg);\
		return 0;\
	}



// ------------------------------------------------------------------------------ //
// CGrid::CGridEntry.
// ------------------------------------------------------------------------------ //

CGrid::CGridEntry::CGridEntry()
{
	m_pPanel = NULL;
	m_bUnderline = false;
}

CGrid::CGridEntry::~CGridEntry()
{
}


// ------------------------------------------------------------------------------ //
// CGrid.
// ------------------------------------------------------------------------------ //

CGrid::CGrid()
{
	Clear();
}


CGrid::~CGrid()
{
	Term();
}


bool CGrid::SetDimensions(int xCols, int yRows)
{
	Term();

	m_GridEntries = new CGridEntry[xCols * yRows];
	m_Widths = new int[xCols*2 + yRows*2];
	m_Heights = m_Widths + xCols;
	m_ColOffsets = m_Heights + yRows;
	m_RowOffsets = m_ColOffsets + xCols;

	if(!m_GridEntries || !m_Widths)
	{
		Term();
		return false;
	}

	memset(m_Widths, 0, sizeof(int) * (xCols*2 + yRows*2));

	m_xCols = xCols;
	m_yRows = yRows;
	return true;
}


void CGrid::Term()
{
	delete [] m_GridEntries;
	delete [] m_Widths;
	Clear();
}


Panel* CGrid::GetEntry(int x, int y)
{
	return GridEntry(x, y)->m_pPanel;
}


bool CGrid::SetEntry(int x, int y, Panel *pPanel)
{
	CGridEntry *pEntry = GridEntry(x, y);
	if(!pEntry)
		return false;

	if (pEntry->m_pPanel)
	{
		pEntry->m_pPanel->SetParent( (Panel *)NULL );
	}

	pEntry->m_pPanel = pPanel;
	if (pPanel)
	{
		pPanel->SetParent(this);
	}

	m_bDirty = true;
	return true;
}


int CGrid::GetXSpacing()
{
	return m_xSpacing;
}


int CGrid::GetYSpacing()
{
	return m_ySpacing;
}


void CGrid::SetSpacing(int xSpacing, int ySpacing)
{
	if(xSpacing != m_xSpacing)
	{
		m_xSpacing = xSpacing;
		CalcColOffsets(0);
		m_bDirty = true;
	}

	if(ySpacing != m_ySpacing)
	{
		m_ySpacing = ySpacing;
		CalcRowOffsets(0);
		m_bDirty = true;
	}
}


bool CGrid::SetColumnWidth(int iColumn, int width)
{
	AssertCheck(iColumn >= 0 && iColumn < m_xCols, "CGrid::SetColumnWidth : invalid location specified");
	m_Widths[iColumn] = width;
	CalcColOffsets(iColumn+1);
	m_bDirty = true;
	return true;
}


bool CGrid::SetRowHeight(int iRow, int height)
{
	AssertCheck(iRow >= 0 && iRow < m_yRows, "CGrid::SetColumnWidth : invalid location specified");
	m_Heights[iRow] = height;
	CalcRowOffsets(iRow+1);
	m_bDirty = true;
	return true;
}


int CGrid::GetColumnWidth(int iColumn)
{
	AssertCheck(iColumn >= 0 && iColumn < m_xCols, "CGrid::GetColumnWidth: invalid location specified");
	return m_Widths[iColumn];
}


int CGrid::GetRowHeight(int iRow)
{
	AssertCheck(iRow >= 0 && iRow < m_yRows, "CGrid::GetRowHeight: invalid location specified");
	return m_Heights[iRow];
}


int CGrid::CalcFitColumnWidth(int iColumn)
{
	AssertCheck(iColumn >= 0 && iColumn < m_xCols, "CGrid::CalcFitColumnWidth: invalid location specified");

	int maxSize = 0;
	for(int i=0; i < m_yRows; i++)
	{
		Panel *pPanel = GridEntry(iColumn, i)->m_pPanel;
		if(!pPanel)
			continue;

		int w, h;
		pPanel->GetSize(w,h);
		if(w > maxSize)
			maxSize = w;
	}

	return maxSize;
}


int CGrid::CalcFitRowHeight(int iRow)
{
	AssertCheck(iRow >= 0 && iRow < m_yRows, "CGrid::CalcFitRowHeight: invalid location specified");

	int maxSize = 0;
	for(int i=0; i < m_xCols; i++)
	{
		Panel *pPanel = GridEntry(i, iRow)->m_pPanel;
		if(!pPanel)
			continue;

		int w, h;
		pPanel->GetSize(w,h);
		if(h > maxSize)
			maxSize = h;
	}

	return maxSize;
}


void CGrid::AutoSetRowHeights()
{
	for(int i=0; i < m_yRows; i++)
		SetRowHeight(i, CalcFitRowHeight(i));
}


bool CGrid::GetEntryBox(
	int col, int row, int &x, int &y, int &w, int &h)
{
	AssertCheck(col >= 0 && col < m_xCols && row >= 0 && row < m_yRows, "CGrid::GetEntryBox: invalid location specified");

	x = m_ColOffsets[col];
	w = m_Widths[col];

	y = m_RowOffsets[row];
	h = m_Heights[row];
	return true;	
}


bool CGrid::CopyColumnWidths(CGrid *pOther)
{
	if(!pOther || pOther->m_xCols != m_xCols)
		return false;

	for(int i=0; i < m_xCols; i++)
		m_Widths[i] = pOther->m_Widths[i];

	CalcColOffsets(0);
	m_bDirty = true;
	return true;
}


void CGrid::RepositionContents()
{
	for(int x=0; x < m_xCols; x++)
	{
		for(int y=0; y < m_yRows; y++)
		{
			Panel *pPanel = GridEntry(x,y)->m_pPanel;
			if(!pPanel)
				continue;

			pPanel->SetBounds(
				m_ColOffsets[x], 
				m_RowOffsets[y],
				m_Widths[x], 
				m_Heights[y]);
		}
	}

	m_bDirty = false;
}


int CGrid::CalcDrawHeight()
{
	if(m_yRows > 0)
	{
		return m_RowOffsets[m_yRows-1] + m_Heights[m_yRows - 1] + m_ySpacing;
	}
	else
	{
		return 0;
	}
}


void CGrid::Paint()
{
	if(m_bDirty)
		RepositionContents();

	Panel::Paint();

	int w, h;
	GetSize( w, h );
	int xx, yy;
	GetPos( xx, yy );
	// walk the grid looking for underlined rows
	int x = 0, y = 0;
	for (int row = 0; row < m_yRows; row++)
	{
		CGridEntry *cell = GridEntry(0, row);

		y = m_RowOffsets[ row ] + m_Heights[ row ] + m_ySpacing;
		if (cell->m_bUnderline)
		{
			vgui::surface()->DrawSetColor(cell->m_UnderlineColor[0], cell->m_UnderlineColor[1], cell->m_UnderlineColor[2], cell->m_UnderlineColor[3]);
			vgui::surface()->DrawFilledRect(x, y - (cell->m_iUnderlineOffset + 1), GetWide(), y - cell->m_iUnderlineOffset);
		}
	}
}

void CGrid::PaintBackground()
{
	Panel::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: sets underline color for a particular row
//-----------------------------------------------------------------------------
void CGrid::SetRowUnderline(int row, bool enabled, int offset, int r, int g, int b, int a)
{
	CGridEntry *cell = GridEntry(0, row);
	cell->m_bUnderline = enabled;
	if (enabled)
	{
		cell->m_iUnderlineOffset = offset;
		cell->m_UnderlineColor[0] = r;
		cell->m_UnderlineColor[1] = g;
		cell->m_UnderlineColor[2] = b;
		cell->m_UnderlineColor[3] = a;
	}
}

void CGrid::Clear()
{
	m_xCols = m_yRows = 0;
	m_Widths = NULL;
	m_GridEntries = NULL;
	m_xSpacing = m_ySpacing = 0;
	m_bDirty = false;
}


CGrid::CGridEntry* CGrid::GridEntry(int x, int y)
{
	AssertCheck(x >= 0 && x < m_xCols && y >= 0 && y < m_yRows, "CGrid::GridEntry: invalid location specified");
	return &m_GridEntries[y*m_xCols + x];
}


void CGrid::CalcColOffsets(int iStart)
{
	int cur = m_xSpacing;
	if(iStart != 0)
		cur += m_ColOffsets[iStart-1] + m_Widths[iStart-1];

	for(int i=iStart; i < m_xCols; i++)
	{
		m_ColOffsets[i] = cur;
		cur += m_Widths[i] + m_xSpacing;
	}
}


void CGrid::CalcRowOffsets(int iStart)
{
	int cur = m_ySpacing;
	if(iStart != 0)
		cur += m_RowOffsets[iStart-1];

	for(int i=iStart; i < m_yRows; i++)
	{
		m_RowOffsets[i] = cur;
		cur += m_Heights[i] + m_ySpacing;
	}
}

bool CGrid::GetCellAtPoint(int worldX, int worldY, int &row, int &col)
{
	row = -1; col = -1;
	for(int x=0; x < m_xCols; x++)
	{
		for(int y=0; y < m_yRows; y++)
		{
			Panel *pPanel = GridEntry(x,y)->m_pPanel;
			if (!pPanel)
				continue;

			if (pPanel->IsWithin(worldX, worldY))
			{
				col = x;
				row = y;
				return true;
			}
		}
	}
		
	return false;
}


