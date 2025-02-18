//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vgui_controls/ScrollableEditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "vgui_controls/Button.h"
#include "KeyValues.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


using namespace vgui;

ScrollableEditablePanel::ScrollableEditablePanel( vgui::Panel *pParent, vgui::EditablePanel *pChild, const char *pName ) :
	BaseClass( pParent, pName )
{
	m_pChild = pChild;
	m_pChild->SetParent( this );

	m_pScrollBar = new vgui::ScrollBar( this, "VerticalScrollBar", true ); 
	m_pScrollBar->SetWide( 16 );
	m_pScrollBar->SetAutoResize( PIN_TOPRIGHT, AUTORESIZE_DOWN, 0, 0, -16, 0 );
	m_pScrollBar->AddActionSignalTarget( this );
}

void ScrollableEditablePanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	KeyValues *pScrollbarKV = pInResourceData->FindKey( "Scrollbar" );
	if ( pScrollbarKV )
	{
		m_pScrollBar->ApplySettings( pScrollbarKV );
	}
}

void ScrollableEditablePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pChild->SetWide( GetWide() - m_pScrollBar->GetWide() );
	m_pScrollBar->SetRange( 0, m_pChild->GetTall() );
	m_pScrollBar->SetRangeWindow( GetTall() );

	if ( m_pScrollBar->GetSlider() )
	{
		m_pScrollBar->GetSlider()->SetFgColor( GetFgColor() );
	}
	if ( m_pScrollBar->GetButton(0) )
	{
		m_pScrollBar->GetButton(0)->SetFgColor( GetFgColor() );
	}
	if ( m_pScrollBar->GetButton(1) )
	{
		m_pScrollBar->GetButton(1)->SetFgColor( GetFgColor() );
	}
}


//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void ScrollableEditablePanel::OnScrollBarSliderMoved()
{
	InvalidateLayout();

	int nScrollAmount = m_pScrollBar->GetValue();
	m_pChild->SetPos( 0, -nScrollAmount ); 
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void ScrollableEditablePanel::OnMouseWheeled(int delta)
{
	int val = m_pScrollBar->GetValue();
	val -= (delta * 50);
	m_pScrollBar->SetValue( val );
}

