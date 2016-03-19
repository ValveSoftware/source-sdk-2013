//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/PanelListPanel.h"

#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( PanelListPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PanelListPanel::PanelListPanel( vgui::Panel *parent, char const *panelName ) : EditablePanel( parent, panelName )
{
	SetBounds( 0, 0, 100, 100 );

	m_vbar = new ScrollBar(this, "PanelListPanelVScroll", true);
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget( this );

	m_pPanelEmbedded = new EditablePanel(this, "PanelListEmbedded");
	m_pPanelEmbedded->SetBounds(0, 0, 20, 20);
	m_pPanelEmbedded->SetPaintBackgroundEnabled( false );
	m_pPanelEmbedded->SetPaintBorderEnabled(false);

	m_iFirstColumnWidth = 100; // default width
	m_iNumColumns = 1; // 1 column by default

	if ( IsProportional() )
	{
		m_iDefaultHeight = scheme()->GetProportionalScaledValueEx( GetScheme(), DEFAULT_HEIGHT );
		m_iPanelBuffer = scheme()->GetProportionalScaledValueEx( GetScheme(), PANELBUFFER );
	}
	else
	{
		m_iDefaultHeight = DEFAULT_HEIGHT;
		m_iPanelBuffer = PANELBUFFER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PanelListPanel::~PanelListPanel()
{
	// free data from table
	DeleteAllItems();
}

void PanelListPanel::SetVerticalBufferPixels( int buffer )
{
	m_iPanelBuffer = buffer;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: counts the total vertical pixels
//-----------------------------------------------------------------------------
int	PanelListPanel::ComputeVPixelsNeeded()
{
	int iCurrentItem = 0;
	int iLargestH = 0;

	int pixels = 0;
	for ( int i = 0; i < m_SortedItems.Count(); i++ )
	{
		Panel *panel = m_DataItems[ m_SortedItems[i] ].panel;
		if ( !panel )
			continue;

		if ( panel->IsLayoutInvalid() )
		{
			panel->InvalidateLayout( true );
		}

		int iCurrentColumn = iCurrentItem % m_iNumColumns;

		int w, h;
		panel->GetSize( w, h );

		if ( iLargestH < h )
			iLargestH = h;

		if ( iCurrentColumn == 0 )
			pixels += m_iPanelBuffer; // add in buffer. between rows.

		if ( iCurrentColumn >= m_iNumColumns - 1 )
		{
			pixels += iLargestH;
			iLargestH = 0;
		}

		iCurrentItem++;
	}

	// Add in remaining largest height
	pixels += iLargestH;

	pixels += m_iPanelBuffer; // add in buffer below last item

	return pixels;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetCellRenderer( int row )
{
	if ( !m_SortedItems.IsValidIndex(row) )
		return NULL;

	Panel *panel = m_DataItems[ m_SortedItems[row] ].panel;
	return panel;
}

//-----------------------------------------------------------------------------
// Purpose: adds an item to the view
//			data->GetName() is used to uniquely identify an item
//			data sub items are matched against column header name to be used in the table
//-----------------------------------------------------------------------------
int PanelListPanel::AddItem( Panel *labelPanel, Panel *panel )
{
	Assert(panel);

	if ( labelPanel )
	{
		labelPanel->SetParent( m_pPanelEmbedded );
	}

	panel->SetParent( m_pPanelEmbedded );

	int itemID = m_DataItems.AddToTail();
	DATAITEM &newitem = m_DataItems[itemID];
	newitem.labelPanel = labelPanel;
	newitem.panel = panel;
	m_SortedItems.AddToTail(itemID);

	InvalidateLayout();
	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: iteration accessor
//-----------------------------------------------------------------------------
int	PanelListPanel::GetItemCount() const
{
	return m_DataItems.Count();
}

int PanelListPanel::GetItemIDFromRow( int nRow ) const
{
	if ( nRow < 0 || nRow >= GetItemCount() )
		return m_DataItems.InvalidIndex();
	return m_SortedItems[ nRow ];
}


//-----------------------------------------------------------------------------
// Iteration. Use these until they return InvalidItemID to iterate all the items.
//-----------------------------------------------------------------------------
int PanelListPanel::FirstItem() const
{
	return m_DataItems.Head();
}

int PanelListPanel::NextItem( int nItemID ) const
{
	return m_DataItems.Next( nItemID );
}

int PanelListPanel::InvalidItemID() const
{
	return m_DataItems.InvalidIndex( );
}


//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetItemLabel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].labelPanel;
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetItemPanel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::RemoveItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	DATAITEM &item = m_DataItems[itemID];
	if ( item.panel )
	{
		item.panel->MarkForDeletion();
	}
	if ( item.labelPanel )
	{
		item.labelPanel->MarkForDeletion();
	}

	m_DataItems.Remove(itemID);
	m_SortedItems.FindAndRemove(itemID);

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void PanelListPanel::DeleteAllItems()
{
	FOR_EACH_LL( m_DataItems, i )
	{
		if ( m_DataItems[i].panel )
		{
			m_DataItems[i].panel->MarkForDeletion();
			m_DataItems[i].panel = NULL;
		}
	}

	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void PanelListPanel::RemoveAll()
{
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	// move the scrollbar to the top of the list
	m_vbar->SetValue(0);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void PanelListPanel::PerformLayout()
{
	int wide, tall;
	GetSize( wide, tall );

	int vpixels = ComputeVPixelsNeeded();

	m_vbar->SetRange( 0, vpixels );
	m_vbar->SetRangeWindow( tall );
	m_vbar->SetButtonPressedScrollValue( tall / 4 ); // standard height of labels/buttons etc.

	m_vbar->SetPos( wide - m_vbar->GetWide() - 2, 0 );
	m_vbar->SetSize( m_vbar->GetWide(), tall - 2 );

	int top = m_vbar->GetValue();

	m_pPanelEmbedded->SetPos( 0, -top );
	m_pPanelEmbedded->SetSize( wide - m_vbar->GetWide(), vpixels );	// scrollbar will sit on top (zpos set explicitly)

	bool bScrollbarVisible = true;
	// If we're supposed to automatically hide the scrollbar when unnecessary, check it now
	if ( m_bAutoHideScrollbar )
	{
		bScrollbarVisible = (m_pPanelEmbedded->GetTall() > tall);
	}
	m_vbar->SetVisible( bScrollbarVisible );
	
	// Now lay out the controls on the embedded panel
	int y = 0;
	int h = 0;
	int totalh = 0;

	int xpos = m_iFirstColumnWidth + m_iPanelBuffer;
	int iColumnWidth = ( wide - xpos - m_vbar->GetWide() - 12 ) / m_iNumColumns;
	
	for ( int i = 0; i < m_SortedItems.Count(); i++ )
	{
		int iCurrentColumn = i % m_iNumColumns;

		// add in a little buffer between panels
		if ( iCurrentColumn == 0 )
			y += m_iPanelBuffer;

		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];

		if ( h < item.panel->GetTall() )
			h = item.panel->GetTall();

		if ( item.labelPanel )
		{
			item.labelPanel->SetBounds( 0, y, m_iFirstColumnWidth, item.panel->GetTall() );
		}

		item.panel->SetBounds( xpos + iCurrentColumn * iColumnWidth, y, iColumnWidth, item.panel->GetTall() );

		if ( iCurrentColumn >= m_iNumColumns - 1 )
		{
			y += h;
			totalh += h;

			h = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: scheme settings
//-----------------------------------------------------------------------------
void PanelListPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("ListPanel.BgColor", GetBgColor(), pScheme));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::OnSliderMoved( int position )
{
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::MoveScrollBarToTop()
{
	m_vbar->SetValue(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::SetFirstColumnWidth( int width )
{
	m_iFirstColumnWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int PanelListPanel::GetFirstColumnWidth()
{
	return m_iFirstColumnWidth;
}

void PanelListPanel::SetNumColumns( int iNumColumns )
{
	m_iNumColumns = iNumColumns;
}

int PanelListPanel::GetNumColumns( void )
{
	return m_iNumColumns;
}

//-----------------------------------------------------------------------------
// Purpose: moves the scrollbar with the mousewheel
//-----------------------------------------------------------------------------
void PanelListPanel::OnMouseWheeled(int delta)
{
	int val = m_vbar->GetValue();
	val -= (delta * DEFAULT_HEIGHT);
	m_vbar->SetValue(val);	
}

//-----------------------------------------------------------------------------
// Purpose: selection handler
//-----------------------------------------------------------------------------
void PanelListPanel::SetSelectedPanel( Panel *panel )
{
	if ( panel != m_hSelectedItem )
	{
		// notify the panels of the selection change
		if ( m_hSelectedItem )
		{
			PostMessage( m_hSelectedItem.Get(), new KeyValues("PanelSelected", "state", 0) );
		}
		if ( panel )
		{
			PostMessage( panel, new KeyValues("PanelSelected", "state", 1) );
		}
		m_hSelectedItem = panel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetSelectedPanel()
{
	return m_hSelectedItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::ScrollToItem( int itemNumber )
{
	if (!m_vbar->IsVisible())
	{
		return;
	}

	DATAITEM& item = m_DataItems[ m_SortedItems[ itemNumber ] ];
	if ( !item.panel )
		return;

	int x, y;
	item.panel->GetPos( x, y );
	int lx, ly;
	lx = x;
	ly = y;
	m_pPanelEmbedded->LocalToScreen( lx, ly );
	ScreenToLocal( lx, ly );

	int h = item.panel->GetTall();

	if ( ly >= 0 && ly + h < GetTall() )
		return;

	m_vbar->SetValue( y );
	InvalidateLayout();
}


