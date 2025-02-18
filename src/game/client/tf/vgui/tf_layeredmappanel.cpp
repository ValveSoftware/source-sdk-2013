//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_layeredmappanel.h"

using namespace vgui;

//=========================================================
// CLayeredMapToolTip
//=========================================================
CLayeredMapToolTip::CLayeredMapToolTip(vgui::Panel *parent, const char *text ) : vgui::BaseTooltip( parent, text )
{
	m_hCurrentPanel = NULL;
}

//=========================================================
void CLayeredMapToolTip::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( !ShouldLayout() )
		return;

	CTFLayeredMapItemPanel *pMapItemPanel = m_hCurrentPanel.Get();
	if ( !pMapItemPanel )
		return;

	m_pControlledPanel->SetVisible( false );

	int x,y;
	pMapItemPanel->GetPos( x, y );

	int iXPos = 0;
	int iYPos = 0;

	// Loop through the positions in our strategy, and hope we find a valid spot
	for ( int i = 0; i < NUM_POSITIONS_PER_STRATEGY; i++ )
	{
		itempanel_tooltippos_t iPos = g_iTooltipStrategies[IPTTP_TOP_SIDE][i];
		GetPosition( iPos, pMapItemPanel, x, y, &iXPos, &iYPos );

		if ( ValidatePosition( pMapItemPanel, x, y, &iXPos, &iYPos ) )
			break;
	}

	m_pControlledPanel->SetPos( iXPos, iYPos );
	m_pControlledPanel->SetVisible( true );
}

//=========================================================
void CLayeredMapToolTip::ShowTooltip( vgui::Panel *currentPanel )
{
	// Set Data on visible panel
	if ( !m_pControlledPanel )
		return;
	
	if ( currentPanel != m_hCurrentPanel.Get() ) 
	{
		CTFLayeredMapItemPanel *pMapItemPanel = dynamic_cast<CTFLayeredMapItemPanel*>( currentPanel );
		m_hCurrentPanel.Set( pMapItemPanel );
		m_pControlledPanel->SetVisible( false );
		if ( pMapItemPanel )
		{
			KeyValues *pData = pMapItemPanel->GetItemKvData();
			if ( pData )
			{
				m_pControlledPanel->SetDialogVariable( "tooltipdescription", pData->GetString( "name", "No Name" ) );
			}
		}
	}
	BaseClass::ShowTooltip( currentPanel );	
}

//=========================================================
void CLayeredMapToolTip::HideTooltip()
{
	if ( !m_pControlledPanel )
		return;

	m_pControlledPanel->SetVisible( false );
	m_hCurrentPanel = NULL;
}

//=========================================================
void CLayeredMapToolTip::SetupPanels( CTFLayeredMapPanel *pParentPanel, vgui::EditablePanel *pControlledPanel ) 
{ 
	m_pParentPanel = pParentPanel; 
	m_pControlledPanel = pControlledPanel; 
}

//=========================================================
void CLayeredMapToolTip::GetPosition( itempanel_tooltippos_t iTooltipPosition, CTFLayeredMapItemPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	switch ( iTooltipPosition )
	{
	case IPTTP_LEFT:
		*iXPos = (iItemX - m_pControlledPanel->GetWide() + XRES(18));
		*iYPos = iItemY - YRES(7);
		break;
	case IPTTP_RIGHT: 
		*iXPos = (iItemX + pItemPanel->GetWide() - XRES(20));
		*iYPos = iItemY - YRES(7);
		break;
	case IPTTP_LEFT_CENTERED:
		*iXPos = (iItemX - m_pControlledPanel->GetWide()) - XRES(4);
		*iYPos = (iItemY - (m_pControlledPanel->GetTall() * 0.5));
		break;
	case IPTTP_RIGHT_CENTERED:
		*iXPos = (iItemX + pItemPanel->GetWide()) + XRES(4);
		*iYPos = (iItemY - (m_pControlledPanel->GetTall() * 0.5));
		break;
	case IPTTP_ABOVE:
		*iXPos = (iItemX + (pItemPanel->GetWide() * 0.5)) - (m_pControlledPanel->GetWide() * 0.5);
		*iYPos = (iItemY - m_pControlledPanel->GetTall() - YRES(4));
		break;
	case IPTTP_BELOW:
		*iXPos = (iItemX + (pItemPanel->GetWide() * 0.5)) - (m_pControlledPanel->GetWide() * 0.5);
		*iYPos = (iItemY + pItemPanel->GetTall() + YRES(4));
		break;
	}
}

//=========================================================
bool CLayeredMapToolTip::ValidatePosition( CTFLayeredMapItemPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	bool bSucceeded = true;

	// Make sure the popup stays onscreen.
	if ( *iXPos < 0 )
	{
		*iXPos = 0;
	}
	else if ( (*iXPos + m_pControlledPanel->GetWide()) > m_pParentPanel->GetWide() )
	{
		int iXPosNew = m_pParentPanel->GetWide() - m_pControlledPanel->GetWide();
		// make sure it is still on the screen
		if ( iXPosNew >= 0 )
		{
			*iXPos = iXPosNew;
		}
		else
		{
			bSucceeded = false;
		}
	}

	if ( *iYPos < 0 )
	{
		*iYPos = 0;
	}
	else if ( (*iYPos + m_pControlledPanel->GetTall() + YRES(32)) > m_pParentPanel->GetTall() )
	{
		// Move it up above our item
		int iYPosNew = iItemY - m_pControlledPanel->GetTall() - YRES(4);
		// make sure it is still on the screen
		if ( iYPosNew >= 0 )
		{
			*iYPos = iYPosNew;
		}
		else
		{
			bSucceeded = false;
		}
	}

	if ( bSucceeded )
	{
		// We also fail if moving it to keep it on screen moved it over the item panel itself
		Vector2D vecToolTipMin, vecToolTipMax, vecItemMin, vecItemMax;
		vecToolTipMin.x = *iXPos;
		vecToolTipMin.y = *iYPos;
		vecToolTipMax.x = vecToolTipMin.x + m_pControlledPanel->GetWide();
		vecToolTipMax.y = vecToolTipMin.y + m_pControlledPanel->GetTall();

		vecItemMin.x = iItemX;
		vecItemMin.y = iItemY;
		vecItemMax.x = vecItemMin.x + m_hCurrentPanel->GetWide();
		vecItemMax.y = vecItemMin.y + m_hCurrentPanel->GetTall();

		bSucceeded = !( vecToolTipMin.x < vecItemMax.x && vecToolTipMax.x > vecItemMin.x &&	vecToolTipMin.y < vecItemMax.y && vecToolTipMax.y > vecItemMin.y );
	}

	return bSucceeded;
}

//-----------------------------------------------------------------------------
// CTFLayeredImagePanelItem
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CTFLayeredMapItemPanel );

CTFLayeredMapItemPanel::CTFLayeredMapItemPanel( Panel *parent, const char *pName ) : vgui::EditablePanel( parent, pName )
{
	m_bIsCompleted = false;
	m_bIsMouseOvered = false;

	m_kvData = NULL;
}

//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/LayeredMapPanelItem.res" );

	// get References to the 4 images
	m_pIsCompleted = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("IsCompletedImage") );
	m_pIsCompletedHighlight = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("IsCompletedHighlight") );

	m_pNotCompleted = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("NotCompletedImage") );
	m_pNotCompletedHighlight = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("NotCompletedHighlight") );

	m_pIsCompleted->SetVisible( true );
	m_pIsCompleted->SetMouseInputEnabled( false );
	m_pIsCompletedHighlight->SetVisible( false );
	m_pIsCompletedHighlight->SetMouseInputEnabled( false );
	m_pNotCompleted->SetVisible( false );
	m_pNotCompleted->SetMouseInputEnabled( false );
	m_pNotCompletedHighlight->SetVisible( false );
	m_pNotCompletedHighlight->SetMouseInputEnabled( false );

	SetMouseInputEnabled( true );
}

//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	// Store Information needed for a mouse over

	KeyValues *kvData = inResourceData->FindKey( "mapitem_kv" );
	if ( kvData )
	{
		if ( m_kvData )
		{
			m_kvData->deleteThis();
		}
		m_kvData = new KeyValues( "mapitem_kv" );
		kvData->CopySubkeys( m_kvData );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::OnCursorEntered( void )
{
	BaseClass::OnCursorEntered();
	m_pIsCompletedHighlight->SetVisible( m_bIsCompleted );
	m_pNotCompletedHighlight->SetVisible( !m_bIsCompleted );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::OnCursorExited( void )
{
	BaseClass::OnCursorExited();
	m_pIsCompletedHighlight->SetVisible( false );
	m_pNotCompletedHighlight->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::OnMousePressed(vgui::MouseCode code)
{
	SetCompletionState( !m_bIsCompleted );
}

//-----------------------------------------------------------------------------
void CTFLayeredMapItemPanel::SetCompletionState( bool bIsCompleted )
{
	m_bIsCompleted = bIsCompleted;
	m_pIsCompleted->SetVisible( m_bIsCompleted );
	m_pNotCompleted->SetVisible( !m_bIsCompleted );
}

//-----------------------------------------------------------------------------
// CTFLayeredMapPanel
// - MultiImage Panel that places all images on top of each other.
// An external manager may then individually toggle each image on or off
// Each image is sized to fit the panel
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CTFLayeredMapPanel );

CTFLayeredMapPanel::CTFLayeredMapPanel( Panel *parent, const char *pName ) : vgui::EditablePanel( parent, pName )
{
	m_pLayeredMapKv = NULL;
}

//-----------------------------------------------------------------------------
void CTFLayeredMapPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/LayeredMapPanel.res" );

	m_MapItems.PurgeAndDeleteElements();

	// Get ToolTip
	m_pToolTipPanel = dynamic_cast<EditablePanel*>( FindChildByName( "ToolTipPanel" ) );
	m_pToolTip = new CLayeredMapToolTip( this );
	m_pToolTip->SetupPanels( this, m_pToolTipPanel );

	if ( m_pLayeredMapKv )
	{
		int iItemCount = m_pLayeredMapKv->GetInt( "item_count", 0 );
		for ( int i = 0; i < iItemCount; ++i )
		{
			CTFLayeredMapItemPanel *pItem = dynamic_cast<CTFLayeredMapItemPanel*>( FindChildByName( VarArgs( "MapItem%d", i ) ) );
			if ( pItem )
			{
				pItem->SetTooltip( m_pToolTip, "" );
				m_MapItems.AddToTail( pItem );
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CTFLayeredMapPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	if ( inResourceData )
	{
		if ( m_pLayeredMapKv )
		{
			m_pLayeredMapKv->deleteThis();
		}
		m_pLayeredMapKv = new KeyValues( "layeredMapPanel_kv" );
		inResourceData->CopySubkeys( m_pLayeredMapKv );
	}
}

