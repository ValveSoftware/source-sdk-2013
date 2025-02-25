//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "base_loadout_panel.h"
#include "item_confirm_delete_dialog.h"
#include "vgui/ISurface.h"
#include "gamestringpool.h"
#include "iclientmode.h"
#include "econ_item_inventory.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui/IInput.h"
#include "econ_ui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseLoadoutPanel::CBaseLoadoutPanel( vgui::Panel *parent, const char *panelName ) : EditablePanel(parent, panelName )
{
	SetParent( parent );

	// Use the client scheme
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pItemModelPanelKVs = NULL;
	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );


	m_pItemPanelBeingMousedOver = NULL;
	m_pCaratLabel = NULL;
	m_pClassLabel = NULL;
	m_nCurrentPage = 0;
	m_bTooltipKeyPressed = false;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	ListenForGameEvent( "inventory_updated" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseLoadoutPanel::~CBaseLoadoutPanel()
{
	if ( m_pItemModelPanelKVs )
	{
		m_pItemModelPanelKVs->deleteThis();
		m_pItemModelPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pCaratLabel = dynamic_cast<vgui::Label*>( FindChildByName("CaratLabel") );
	m_pClassLabel = dynamic_cast<vgui::Label*>( FindChildByName("ClassLabel") );

	m_bReapplyItemKVs = true;
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		SetBorderForItem( m_pItemModelPanels[i], false );
	}

	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	CreateItemPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pItemModelPanelKVs )
		{
			m_pItemModelPanelKVs->deleteThis();
		}
		m_pItemModelPanelKVs = new KeyValues("modelpanels_kv");
		pItemKV->CopySubkeys( m_pItemModelPanelKVs );
	}
}

extern const char *g_szItemBorders[AE_MAX_TYPES][5];
extern ConVar cl_showbackpackrarities;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	if ( !pItemPanel )
		return;

	const char *pszBorder = NULL;

	if ( pItemPanel->IsGreyedOut() )
	{
		if( pItemPanel->IsSelected() )
		{
			pszBorder = "BackpackItemGrayedOut_Selected";
		}
		else
		{
			pszBorder = "BackpackItemGrayedOut";
		}
	}
	else
	{
		int iRarity = 0;
		if ( pItemPanel->HasItem() && cl_showbackpackrarities.GetBool() ) 
		{
			iRarity = pItemPanel->GetItem()->GetItemQuality() ;

			uint8 nRarity = pItemPanel->GetItem()->GetItemDefinition()->GetRarity();
			if ( ( nRarity != k_unItemRarity_Any ) && ( iRarity != AE_SELFMADE ) && ( iRarity != AE_UNUSUAL ) )
			{
				// translate this quality to rarity
				iRarity = nRarity + AE_RARITY_DEFAULT;
			}
		}

		if ( pItemPanel->IsSelected() )
		{
			pszBorder = g_szItemBorders[iRarity][2];
		}
		if ( bMouseOver )
		{
			pszBorder = g_szItemBorders[iRarity][1];
		}
		else
		{
			pszBorder = g_szItemBorders[iRarity][0];
		}
	}

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::ApplyKVsToItemPanels( void )
{
	if ( m_pItemModelPanelKVs )
	{
		for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
		{
			m_pItemModelPanels[i]->ApplySettings( m_pItemModelPanelKVs );
			SetBorderForItem( m_pItemModelPanels[i], false );
			m_pItemModelPanels[i]->InvalidateLayout();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::PerformLayout( void ) 
{
	if ( m_bReapplyItemKVs )
	{
		m_bReapplyItemKVs = false;
		ApplyKVsToItemPanels();
	}

	BaseClass::PerformLayout();

	// If we're items only, we hide various elements
	if ( m_pCaratLabel )
	{
		m_pCaratLabel->SetVisible( !m_bItemsOnly );
	}

	if ( m_pClassLabel )
	{
		m_pClassLabel->SetVisible( !m_bItemsOnly );
	}

	if ( m_pMouseOverItemPanel->IsVisible() )
	{
		// The mouseover panel was visible. Fake a panel entry into the original panel to get it to show up again properly.
		if ( m_pItemPanelBeingMousedOver )
		{
			OnItemPanelEntered( m_pItemPanelBeingMousedOver );
		}
		else
		{
			HideMouseOverPanel();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::AddNewItemPanel( int iPanelIndex )
{
	CItemModelPanel *pPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, VarArgs("modelpanel%d", iPanelIndex) ) );
	pPanel->SetActAsButton( true, true );
	m_pItemModelPanels.AddToTail( pPanel );

		pPanel->SetTooltip( m_pMouseOverTooltip, "" );

	Assert( iPanelIndex == (m_pItemModelPanels.Count()-1) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::CreateItemPanels( void )
{
	int iNumPanels = GetNumItemPanels();
	if ( m_pItemModelPanels.Count() < iNumPanels )
	{
		for ( int i = m_pItemModelPanels.Count(); i < iNumPanels; i++ )
		{
			AddNewItemPanel(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::ShowPanel( int iClass, bool bBackpack, bool bReturningFromArmory )
{
	bool bShow = (iClass != 0 || bBackpack);
	OnShowPanel( bShow, bReturningFromArmory );

	SetVisible( bShow );

	if ( bShow )
	{
		HideMouseOverPanel();

		CreateItemPanels();

		UpdateModelPanels();

		// make the first slot be selected so controller input will work
		static ConVarRef joystick( "joystick" );
		if( joystick.IsValid() && joystick.GetBool() && m_pItemModelPanels.Count() && m_pItemModelPanels[0] )
		{
			m_pItemModelPanels[0]->SetSelected( true );
			m_pItemModelPanels[0]->RequestFocus();
		}
	}
	else
	{
		// clear items from panels to make sure that items get invalidate on show panel
		FOR_EACH_VEC( m_pItemModelPanels, i )
		{
			m_pItemModelPanels[i]->SetItem( NULL );
		}
	}

	if ( !bReturningFromArmory )
	{
		PostShowPanel( bShow );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::OnCommand( const char *command )
{
	engine->ClientCmd( const_cast<char *>( command ) );

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::FireGameEvent( IGameEvent *event )
{
	// If we're not visible, ignore all events
	if ( !IsVisible() )
		return;

	const char *type = event->GetName();
	if ( Q_strcmp( "inventory_updated", type ) == 0 )
	{
		// We need to refresh our model panels, because the items may have changed.
		UpdateModelPanels();
	}
}

CItemModelPanel *CBaseLoadoutPanel::FindBestPanelNavigationForDirection( const CItemModelPanel *pCurrentPanel, const Vector2D &vPos, const Vector2D &vDirection )
{
	CItemModelPanel *pBestPanel = NULL;

	// Start with the worst allowable score
	float flDistance = GetWide() + GetTall();
	float flDot = -1.0f;
	float flClosenessScore = flDistance * ( 1.5f - flDot );

	for ( int j = 0; j < m_pItemModelPanels.Count(); j++ )
	{
		CItemModelPanel *pTempPanel = m_pItemModelPanels[ j ];
		if ( !pTempPanel || pTempPanel == pCurrentPanel )
			continue;

		// Get temp center position
		int nX, nY;
		pTempPanel->GetPos( nX, nY );
		nX += pTempPanel->GetWide() / 2;
		nY += pTempPanel->GetTall() / 2;
		Vector2D vTempPos( nX, nY );

		// Get distance and dot
		Vector2D vDiff = vTempPos - vPos;
		float flTempDistance = Vector2DNormalize( vDiff );
		float flTempDot = vDiff.Dot( vDirection );

		// Must be somewhat in the correct direction
		if ( flTempDot <= 0.0f )
			continue;

		float flTempScore = flTempDistance * ( 1.5f - flTempDot );

		if ( flClosenessScore > flTempScore )
		{
			flClosenessScore = flTempScore;
			flDistance = flTempDistance;
			flDot = flTempDot;
			pBestPanel = pTempPanel;
		}
	}

	return pBestPanel;
}

void CBaseLoadoutPanel::LinkModelPanelControllerNavigation( bool bForceRelink )
{
	if ( m_pItemModelPanels.Count() < 2 )
		return;

	// first unlink everything
	if( bForceRelink )
	{
		for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
		{
			CItemModelPanel *pCurrentPanel = m_pItemModelPanels[ i ];
			if ( !pCurrentPanel )
				continue;

			pCurrentPanel->SetNavUp( (vgui::Panel*)NULL );
			pCurrentPanel->SetNavDown( (vgui::Panel*)NULL );
			pCurrentPanel->SetNavLeft( (vgui::Panel*)NULL );
			pCurrentPanel->SetNavRight( (vgui::Panel*)NULL );
		}
	}

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		CItemModelPanel *pCurrentPanel = m_pItemModelPanels[ i ];
		if ( !pCurrentPanel )
			continue;

		// Get center position
		int nX, nY;
		pCurrentPanel->GetPos( nX, nY );
		nX += pCurrentPanel->GetWide() / 2;
		nY += pCurrentPanel->GetTall() / 2;
		Vector2D vPos( nX, nY );

		if ( !pCurrentPanel->GetNavUpName() || pCurrentPanel->GetNavUpName()[ 0 ] == '\0' )
		{
			CItemModelPanel *pBestPanel = FindBestPanelNavigationForDirection( pCurrentPanel, vPos, Vector2D( 0, -1 ) );
			if ( pBestPanel )
			{
				pCurrentPanel->SetNavUp( pBestPanel->GetName() );
				pBestPanel->SetNavDown( pCurrentPanel->GetName() );
			}
		}

		if ( !pCurrentPanel->GetNavDownName() || pCurrentPanel->GetNavDownName()[ 0 ] == '\0' )
		{
			CItemModelPanel *pBestPanel = FindBestPanelNavigationForDirection( pCurrentPanel, vPos, Vector2D( 0, 1 ) );
			if ( pBestPanel )
			{
				pCurrentPanel->SetNavDown( pBestPanel->GetName() );
				pBestPanel->SetNavUp( pCurrentPanel->GetName() );
			}
		}

		if ( !pCurrentPanel->GetNavLeftName() || pCurrentPanel->GetNavLeftName()[ 0 ] == '\0' )
		{
			CItemModelPanel *pBestPanel = FindBestPanelNavigationForDirection( pCurrentPanel, vPos, Vector2D( -1, 0 ) );
			if ( pBestPanel )
			{
				pCurrentPanel->SetNavLeft( pBestPanel->GetName() );
				pBestPanel->SetNavRight( pCurrentPanel->GetName() );
			}
		}

		if ( !pCurrentPanel->GetNavRightName() || pCurrentPanel->GetNavRightName()[ 0 ] == '\0' )
		{
			CItemModelPanel *pBestPanel = FindBestPanelNavigationForDirection( pCurrentPanel, vPos, Vector2D( 1, 0 ) );
			if ( pBestPanel )
			{
				pCurrentPanel->SetNavRight( pBestPanel->GetName() );
				pBestPanel->SetNavLeft( pCurrentPanel->GetName() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		CEconItemView *pItem = pItemPanel->GetItem();
		if ( pItem && !IsIgnoringItemPanelEnters() && !pItemPanel->IsGreyedOut() )
		{
			m_pItemPanelBeingMousedOver = pItemPanel;
		}

		if ( !pItemPanel->IsSelected() )
		{
			SetBorderForItem( pItemPanel, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		if ( !pItemPanel->IsSelected() )
		{
			SetBorderForItem( pItemPanel, false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::HideMouseOverPanel( void )
{
	if ( m_pMouseOverItemPanel->IsVisible() )
	{
		m_pMouseOverItemPanel->SetVisible( false );
		m_pItemPanelBeingMousedOver = NULL;
	}

}


//-----------------------------------------------------------------------------
// Purpose: Returns the index of the first selected item.
//-----------------------------------------------------------------------------
int CBaseLoadoutPanel::GetFirstSelectedItemIndex( bool bIncludeEmptySlots )
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && ( bIncludeEmptySlots || m_pItemModelPanels[i]->HasItem() ) )
		{
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the first selected item model panel or NULL if there is no
//			such panel.
//-----------------------------------------------------------------------------
CItemModelPanel *CBaseLoadoutPanel::GetFirstSelectedItemModelPanel (bool bIncludeEmptySlots )
{
	int i = GetFirstSelectedItemIndex( bIncludeEmptySlots );
	if( i == -1 )
		return NULL;
	else
		return m_pItemModelPanels[ i ];
}


//-----------------------------------------------------------------------------
// Purpose: Returns the first selected econ item view or NULL if there is no
//			selected item
//-----------------------------------------------------------------------------
CEconItemView *CBaseLoadoutPanel::GetFirstSelectedItem()
{
	CItemModelPanel *pItemModelPanel = GetFirstSelectedItemModelPanel( false );
	if( pItemModelPanel )
		return pItemModelPanel->GetItem();
	else
		return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next item in the specified direction, possibly switching
//			pages to get there
//-----------------------------------------------------------------------------
bool CBaseLoadoutPanel::GetAdjacentItemIndex( int nIndex, int nPage, int *pnNewIndex, int *pnNewPage, int dx, int dy )
{
	// if we don't have a valid index the right answer is always the first item on the first page
	if( nIndex == -1 )
	{
		*pnNewIndex = 0;
		*pnNewPage = nPage;
		return true;
	}

	int nRow = nIndex / GetNumColumns() + dy;
	int nColumn = nIndex % GetNumColumns() + dx;

	// just limit us to the top and bottom edges
	if( nRow < 0 || nRow >= GetNumRows() )
		return false;

	// for columns, try to switch pages
	int nNewPage = nPage;
	while( nColumn < 0 )
	{
		if( nNewPage == 0 )
			break;

		nNewPage--;
		nColumn += GetNumColumns();
	}

	while( nColumn >= GetNumColumns() )
	{
		if( nNewPage == GetNumPages() - 1 )
			break;

		nNewPage++;
		nColumn -= GetNumColumns();
	}

	if( nColumn < 0 )
	{
		if( nNewPage != nPage )
		{
			nColumn = 0;
		}
		else
		{
			return false;
		}
	}
	else if( nColumn >= GetNumColumns() )
	{
		if( nNewPage != nPage )
		{
			nColumn = GetNumColumns() - 1;
		}
		else
		{
			return false;
		}
	}

	// never change to an invisible panel
	int nNewIndex = nRow * GetNumColumns() + nColumn;
	if( nNewIndex >= m_pItemModelPanels.Count() || !m_pItemModelPanels[ nNewIndex ]->IsVisible() )
	{
		// try to find a model panel that's still valid so we find the last one on the last valid row
		while( nNewIndex >= 0 && !m_pItemModelPanels[ nNewIndex ]->IsVisible() )
			nNewIndex--;

		if( nNewIndex < 0 || nNewIndex == nIndex )
			return false;
	}

	*pnNewPage = nNewPage;
	*pnNewIndex = nNewIndex;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: selects the next item in the specified direction, possibly switching
//			pages to get there
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::SelectAdjacentItem( int dx, int dy )
{
	int nSelected = GetFirstSelectedItemIndex( true );
	int nNewPage, nNewSelected;
	bool bFoundNext = GetAdjacentItemIndex( nSelected, m_nCurrentPage, &nNewSelected, &nNewPage, dx, dy );
	if( !bFoundNext )
	{
		vgui::surface()->PlaySound( "player/suit_denydevice.wav" );
		return;
	}

	// change pages
	if( nNewPage != m_nCurrentPage )
	{
		Assert( nNewPage >= 0 && nNewPage < GetNumPages() );
		SetCurrentPage( nNewPage );
		UpdateModelPanels();
	}

	// select the new model
	if( nSelected != nNewSelected )
	{
		if( nSelected != -1 && m_pItemModelPanels[ nSelected ]->IsSelected() )
		{
			m_pItemModelPanels[ nSelected ]->SetSelected( false );
			SetBorderForItem( m_pItemModelPanels[ nSelected ], false );
		}
		if( nNewSelected != -1 && !m_pItemModelPanels[ nNewSelected ]->IsSelected() )
		{
			m_pItemModelPanels[ nNewSelected ]->SetSelected( true );
			SetBorderForItem( m_pItemModelPanels[ nNewSelected ], false );

			if( m_bTooltipKeyPressed )
			{
				if( m_pItemModelPanels[ nNewSelected ]->HasItem() )
				{
					m_pMouseOverTooltip->ShowTooltip( m_pItemModelPanels[ nNewSelected ] );
				}
				else
				{
					m_pMouseOverTooltip->HideTooltip();
				}
			}
		
		}
	}

	OnItemSelectionChanged();
}


//-----------------------------------------------------------------------------
// Purpose: Processes up/down/left/right keys for selecting items in the panel
//-----------------------------------------------------------------------------
bool	CBaseLoadoutPanel::HandleItemSelectionKeyPressed( vgui::KeyCode code ) 
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if ( nButtonCode == KEY_XBUTTON_UP || 
			  nButtonCode == KEY_XSTICK1_UP ||
			  nButtonCode == KEY_XSTICK2_UP || 
			  nButtonCode == KEY_UP )
	{
		SelectAdjacentItem( 0, -1 );
		return true;
	}
	else if ( nButtonCode == KEY_XBUTTON_DOWN || 
			  nButtonCode == KEY_XSTICK1_DOWN ||
			  nButtonCode == KEY_XSTICK2_DOWN ||
			  nButtonCode == STEAMCONTROLLER_DPAD_DOWN ||
			  nButtonCode == KEY_DOWN )
	{
		SelectAdjacentItem( 0, 1 );
		return true;
	}
	else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
			  nButtonCode == KEY_XSTICK1_RIGHT ||
			  nButtonCode == KEY_XSTICK2_RIGHT || 
			  nButtonCode == STEAMCONTROLLER_DPAD_RIGHT ||
			  nButtonCode == KEY_RIGHT )
	{
		SelectAdjacentItem( 1, 0 );
		return true;
	}
	else if ( nButtonCode == KEY_XBUTTON_LEFT || 
			  nButtonCode == KEY_XSTICK1_LEFT ||
			  nButtonCode == KEY_XSTICK2_LEFT || 
			  nButtonCode == STEAMCONTROLLER_DPAD_LEFT ||
			  nButtonCode == KEY_LEFT )
	{
		SelectAdjacentItem( -1, 0 );
		return true;
	}
	else if ( code == KEY_PAGEDOWN || 
			nButtonCode == KEY_XBUTTON_RIGHT_SHOULDER )
	{
		if( m_nCurrentPage < GetNumPages() - 1 )
		{
			SetCurrentPage( m_nCurrentPage + 1 );
			UpdateModelPanels();
		}
		return true;
	}
	else if ( code == KEY_PAGEUP || 
			nButtonCode == KEY_XBUTTON_LEFT_SHOULDER )
	{
		if( m_nCurrentPage > 0 )
		{
			SetCurrentPage( m_nCurrentPage - 1 );
			UpdateModelPanels();
		}
		return true;
	}
	else if ( nButtonCode == KEY_XBUTTON_Y )
	{
		m_bTooltipKeyPressed = true;
		CItemModelPanel *pSelection = GetFirstSelectedItemModelPanel( false );
		if( pSelection )
		{
			m_pMouseOverTooltip->ResetDelay();
			m_pMouseOverTooltip->ShowTooltip( pSelection );
		}
		return true;
	}
	else
	{
		return false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Processes up/down/left/right keys for selecting items in the panel
//-----------------------------------------------------------------------------
bool	CBaseLoadoutPanel::HandleItemSelectionKeyReleased( vgui::KeyCode code ) 
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );
	if( nButtonCode == KEY_XBUTTON_Y )
	{
		m_bTooltipKeyPressed = false;
		m_pMouseOverTooltip->HideTooltip();
		return true;
	}
	else
	{
		return false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseLoadoutPanel::SetCurrentPage( int nNewPage )
{
	if( nNewPage < 0 || nNewPage >= GetNumPages() )
		return;

	m_nCurrentPage = nNewPage;
}


