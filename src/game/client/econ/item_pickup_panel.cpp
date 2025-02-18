//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "baseviewport.h"
#include "item_pickup_panel.h"
#include "econ_ui.h"
#include "econ_item_inventory.h"
#include "econ_item_constants.h"
#include "item_confirm_delete_dialog.h"
#include "backpack_panel.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_freeaccount.h"
#include "clientmode_tf.h"
#include "quest_log_panel.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel::CItemPickupPanel( Panel *parent, bool bPopup ) : Frame( parent, "item_pickup", bPopup )
{
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pNextButton = new vgui::Button( this, "NextButton", "#Next" );	
	m_pPrevButton = new vgui::Button( this, "PrevButton", "#Prev" );	
	m_pDiscardButton = NULL;
	m_pDiscardedLabel = NULL;
	m_pOpenLoadoutButton = NULL;
	m_pConfirmDeleteDialog = NULL;
	m_pModelPanelsKV = NULL;
	m_bRandomizePickupMethods = false;
	m_pItemFoundMethod = NULL;
	m_pCloseButton = NULL;

	for ( int i = 0; i < ITEMPICKUP_NUM_MODELPANELS; i++ )
	{
		m_aModelPanels[i] = new CItemModelPanel( this, VarArgs("modelpanel%d", i) );
	}

	ListenForGameEvent( "gameui_hidden" );

	vgui::EditablePanel *pToolTipPanel = new vgui::EditablePanel( this, "DiscardButtonTooltip" );
	m_pToolTip = new CSimplePanelToolTip( this );
	m_pToolTip->SetControlledPanel( pToolTipPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel::~CItemPickupPanel()
{
	if ( m_pModelPanelsKV )
	{
		m_pModelPanelsKV->deleteThis();
		m_pModelPanelsKV = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/ItemPickupPanel.res" );

	m_pItemsFoundLabel = dynamic_cast<vgui::Label*>(FindChildByName( "ItemsFoundLabel" ));
	m_pDiscardedLabel = dynamic_cast<vgui::Label*>( FindChildByName("DiscardedLabel") );
	m_pItemFoundMethod = dynamic_cast<vgui::Label*>(FindChildByName( "SelectedItemFoundMethodLabel" ));

	for ( int i = 0; i < ITEMPICKUP_NUM_MODELPANELS; i++ )
	{
		m_aModelPanels[i]->SetPaintBackgroundType( 2 );
	}

	m_pDiscardButton = dynamic_cast<CExImageButton*>( FindChildByName("DiscardButton") );
	if ( m_pDiscardButton )
	{
		m_pDiscardButton->SetTooltip( m_pToolTip, "" );
	}

	m_pOpenLoadoutButton = dynamic_cast<vgui::Button*>( FindChildByName("OpenLoadoutButton") );
	m_pCloseButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pModelKV = inResourceData->FindKey( "modelpanelskv" );
	if ( pModelKV )
	{
		if ( m_pModelPanelsKV )
		{
			m_pModelPanelsKV->deleteThis();
		}
		m_pModelPanelsKV = new KeyValues("modelpanelkv");
		pModelKV->CopySubkeys( m_pModelPanelsKV );

		for ( int i = 0; i < ITEMPICKUP_NUM_MODELPANELS; i++ )
		{
			m_aModelPanels[i]->ApplySettings( m_pModelPanelsKV );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();

	// Now lay out our model panels
	for ( int i = 0; i < ITEMPICKUP_NUM_MODELPANELS; i++ )
	{
		int iPos = (i - ITEMPICKUP_MODELPANEL_CENTER);
		int iXPos = (GetWide() * 0.5) + (iPos * m_iModelPanelSpacing) + (iPos * m_iModelPanelW) - (m_iModelPanelW * 0.5);
		m_aModelPanels[i]->SetBounds( iXPos, m_iModelPanelY, m_iModelPanelW, m_iModelPanelH );
		m_aModelPanels[i]->SetVisible( m_aModelPanels[i]->HasItem() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		// Start with the first item centered, and the rest off to the right.
		m_iSelectedItem = 0;
		UpdateModelPanels();
		MoveToFront();		// Any open dialogs from the main menu (options, servers, etc.) showing up sometimes - this should fix.
	}
	else
	{
		for ( int i = 0; i < m_aItems.Count(); i++ )
		{
			if ( !m_aItems[i].bDiscarded )
			{
				// if not a real item, ignore
				if ( m_aItems[i].pItem.GetSOCData() == NULL )
				{
					continue;
				}

				// ignore items that are purchased or are store promotional items
				CEconItem *pEconItem = m_aItems[i].pItem.GetSOCData();
				switch ( pEconItem->GetOrigin() )
				{
				case kEconItemOrigin_PreviewItem:
				case kEconItemOrigin_Purchased:
				case kEconItemOrigin_StorePromotion:
					continue;
				}
			}
		}

		if ( m_pConfirmDeleteDialog )
		{
			m_pConfirmDeleteDialog->MarkForDeletion();
			m_pConfirmDeleteDialog = NULL;
		}

		m_aItems.Purge();

		auto pEvent = gameeventmanager->CreateEvent( "items_acknowledged" );
		if ( pEvent )
		{
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}

	SetMouseInputEnabled( bShow );
	SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::FireGameEvent( IGameEvent *event )
{
	if ( !IsVisible() )
		return;

	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		ShowPanel( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
	{
		AcknowledgeItems();
		ShowPanel( false );
	}
	else if ( !Q_stricmp( command, "changeloadout" ) )
	{
		AcknowledgeItems();
		ShowPanel( false );
		EconUI()->OpenEconUI( ECONUI_LOADOUT, true );
	}
	else if ( !Q_stricmp( command, "nextitem" ) )
	{
		m_iSelectedItem = clamp( m_iSelectedItem+1, 0, m_aItems.Count()-1 );
		UpdateModelPanels();
	}
	else if ( !Q_stricmp( command, "previtem" ) )
	{
		m_iSelectedItem = clamp( m_iSelectedItem-1, 0, m_aItems.Count()-1 );
		UpdateModelPanels();
	}
	else if ( !Q_stricmp( command, "discarditem" ) )
	{
		// Bring up confirm dialog
		CConfirmDeleteItemDialog *pConfirm = vgui::SETUP_PANEL( new CConfirmDeleteItemDialog( this ) );
		if ( pConfirm )
		{
			pConfirm->Show();

			m_pConfirmDeleteDialog = pConfirm;
		}
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::UpdateModelPanels( void )
{
	for ( int i = 0; i < ITEMPICKUP_NUM_MODELPANELS; i++ )
	{
		int iPos = (i - ITEMPICKUP_MODELPANEL_CENTER);
		int iItem = m_iSelectedItem + iPos;
		if ( iItem < 0 || iItem >= m_aItems.Count() )
		{
			m_aModelPanels[i]->SetItem( NULL );
			m_aModelPanels[i]->SetVisible( false );
			continue;
		}

		m_aModelPanels[i]->SetItem( &m_aItems[iItem].pItem );
		m_aModelPanels[i]->SetVisible( true );
	}

	// Enable next & prev buttons if there are other items to the sides
	bool bCanScrollRight = (m_iSelectedItem + 1) < m_aItems.Count();
	m_pNextButton->SetVisible( bCanScrollRight );
	m_pNextButton->SetEnabled( bCanScrollRight );
	bool bCanScrollLeft = ((m_iSelectedItem - 1) >= 0) && m_aItems.Count();
	m_pPrevButton->SetVisible( bCanScrollLeft );
	m_pPrevButton->SetEnabled( bCanScrollLeft );

	bool bDiscarded = false;
	if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
	{
		bDiscarded = m_aItems[m_iSelectedItem].bDiscarded;
	}

	bool bAllowDiscard = true;

	int iFoundMethod = 0;
	if ( m_bRandomizePickupMethods )
	{
		iFoundMethod = RandomInt( 0, ARRAYSIZE(g_pszItemPickupMethodStrings)-1 );
	}
	else if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
	{
		iFoundMethod = GetUnacknowledgedReason( m_aItems[m_iSelectedItem].pItem.GetInventoryPosition() );
		bAllowDiscard = ( iFoundMethod <= UNACK_ITEM_DROPPED );
		iFoundMethod--;

		if ( iFoundMethod < 0 || iFoundMethod >= ARRAYSIZE(g_pszItemPickupMethodStrings) )
		{
			iFoundMethod = 0;
		}
	}

	// Hide the discard button if it's not a random find
	m_pDiscardButton->SetVisible( !bDiscarded && bAllowDiscard );
	m_pOpenLoadoutButton->SetVisible( !bDiscarded );
	m_pDiscardedLabel->SetVisible( bDiscarded );

	if ( m_pItemFoundMethod )
	{
		enum
		{
			COLOR_NORMAL = 1,
			COLOR_HIGHLIGHTED = 2,
		};

		m_pItemFoundMethod->GetTextImage()->ClearColorChangeStream();

		wchar_t wszTmp[1024];
		wchar_t *pLocText = g_pVGuiLocalize->Find( g_pszItemPickupMethodStrings[iFoundMethod] );
		if ( pLocText )
		{
			// Loop through and replace the color changes
			Color baseColor = m_pItemFoundMethod->GetFgColor();
			wchar_t *wpszChar = pLocText;
			int iOutPos = 0;
			while ( *wpszChar )
			{
				if ( *wpszChar == COLOR_NORMAL )
				{
					m_pItemFoundMethod->GetTextImage()->AddColorChange( baseColor, iOutPos );
				}
				else if ( *wpszChar == COLOR_HIGHLIGHTED )
				{
					m_pItemFoundMethod->GetTextImage()->AddColorChange( Color(200,80,60,255), iOutPos );
				}
				else
				{
					wszTmp[iOutPos] = *wpszChar;
					iOutPos++;
				}
				wpszChar++;

				if ( iOutPos >= (ARRAYSIZE(wszTmp) - 1) )
				{
					wszTmp[iOutPos] = L'\0';
					break;
				}
			}

			if ( iOutPos < (ARRAYSIZE(wszTmp) - 1) )
			{
				wszTmp[iOutPos] = L'\0';
			}

			m_pItemFoundMethod->SetText( wszTmp );
		}
	}

	if ( m_pItemsFoundLabel )
	{
		if ( m_aItems.Count() > 1 )
		{
			m_pItemsFoundLabel->SetText( "#NewItemsAcquired" );
			SetDialogVariable( "numitems", m_aItems.Count() );
		}
		else
		{
			m_pItemsFoundLabel->SetText( "#NewItemAcquired" );
		}
	}

	SetDialogVariable( "selecteditem", m_iSelectedItem+1 );

	// Update the loadout button as appropriate
	if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
	{
		if ( m_aItems[m_iSelectedItem].pItem.IsValid() && !bDiscarded )
		{
			SetDialogVariable("loadouttext", g_pVGuiLocalize->Find( "#OpenGeneralLoadout" ) );
		}
	}

	if ( m_pCloseButton )
	{
		const char *pszCloseString = (m_bReturnToGame && engine->IsInGame()) ? "#CloseItemPanel" : "#GameUI_OK";
		m_pCloseButton->SetText( g_pVGuiLocalize->Find( pszCloseString ) );
	}
	m_pCloseButton->RequestFocus();
}

//-----------------------------------------------------------------------------
void CItemPickupPanel::AcknowledgeItems( void ) 
{
	// On command, AckKnowledge all these items
	for ( int i = 0; i < m_aItems.Count(); i++ )
	{
		InventoryManager()->AcknowledgeItem( &m_aItems[i].pItem, false );
	}

	InventoryManager()->SaveAckFile();

	// If we were crafting, and the craft panel is up, we return to that instead.
	if ( EconUI()->IsUIPanelVisible( ECONUI_CRAFTING ) )
		return;

	// Check to make sure the player has room for all his items. If not, bring up the discard panel. Otherwise, go away.
	if ( !InventoryManager()->CheckForRoomAndForceDiscard(  ) )
	{
		// If we're connected to a game server, we also close the game UI.
		if ( m_bReturnToGame && engine->IsInGame() )
		{
			engine->ClientCmd_Unrestricted( "gameui_hide" );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Handles the escape key
//-----------------------------------------------------------------------------
void	CItemPickupPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	if( code == KEY_ESCAPE )
	{
		OnCommand( "vguicancel" );
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles button press code for controllers and enter
//-----------------------------------------------------------------------------
void	CItemPickupPanel::OnKeyCodePressed( vgui::KeyCode code )
{	
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if( code == KEY_ENTER || nButtonCode == KEY_XBUTTON_A || nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_A || nButtonCode == STEAMCONTROLLER_B )
	{
		OnCommand( "vguicancel" );
	}
	else if( code == KEY_HOME || nButtonCode == KEY_XBUTTON_Y || nButtonCode == STEAMCONTROLLER_Y )
	{
		OnCommand( "changeloadout" );
	}
	
	else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
			  nButtonCode == KEY_XSTICK1_RIGHT ||
			  nButtonCode == KEY_XSTICK2_RIGHT || 
			  nButtonCode == KEY_RIGHT )
	{
		OnCommand( "nextitem" );
	}
	else if ( nButtonCode == KEY_XBUTTON_LEFT || 
			  nButtonCode == KEY_XSTICK1_LEFT ||
			  nButtonCode == KEY_XSTICK2_LEFT || 
			  nButtonCode == KEY_LEFT )
	{
		OnCommand( "previtem" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::AddItem( CEconItemView *pItem )
{
	// Guard against duplicates
	FOR_EACH_VEC( m_aItems, i )
	{
		if ( m_aItems[i].pItem.GetItemID() == pItem->GetItemID() )
			return;
	}

	int iIdx = m_aItems.AddToTail();
	m_aItems[iIdx].pItem = *pItem;
	m_aItems[iIdx].bDiscarded = false;

	if ( IsVisible() )
	{
		UpdateModelPanels();
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemPickupPanel::OnConfirmDelete( KeyValues *data )
{
	m_pConfirmDeleteDialog = NULL;
	m_pCloseButton->RequestFocus();

	int iConfirmed = data->GetInt( "confirmed", 0 );
	if ( !iConfirmed )
		return;

	if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
	{
		if ( m_aItems[m_iSelectedItem].pItem.IsValid() )
		{
			CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
			if ( pInventory )
			{
				EconUI()->Gamestats_ItemTransaction( IE_ITEM_DISCARDED, &m_aItems[m_iSelectedItem].pItem );

				InventoryManager()->DropItem( m_aItems[m_iSelectedItem].pItem.GetItemID() );
				InvalidateLayout();

				m_aItems[m_iSelectedItem].bDiscarded = true;
				m_pDiscardButton->SetVisible( false );
				m_pOpenLoadoutButton->SetVisible( false );
				m_pDiscardedLabel->SetVisible( true );

				// If we've discarded all our items, we exit immediately
				bool bFoundUndiscarded = false;
				for ( int i = 0; i < m_aItems.Count(); i++ )
				{
					if ( !m_aItems[i].bDiscarded )
					{
						bFoundUndiscarded = true;
						break;
					}
				}

				if ( !bFoundUndiscarded )
				{
					OnCommand( "vguicancel" );
				}
			}
		}
	}
}

static vgui::DHANDLE<CItemPickupPanel> g_ItemPickupPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel *OpenItemPickupPanel( void )
{
	if (!g_ItemPickupPanel.Get())
	{
		g_ItemPickupPanel = vgui::SETUP_PANEL( new CItemPickupPanel( NULL ) );
		g_ItemPickupPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_ItemPickupPanel->ShowPanel( true );

	return g_ItemPickupPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel *GetItemPickupPanel( void )
{
	return g_ItemPickupPanel.Get();
}

//=======================================================================================================================================================
// ITEM DISCARD PANEL
//=======================================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemDiscardPanel::CItemDiscardPanel( Panel *parent, bool bPopup ) : Frame( parent, "item_discard", bPopup )
{
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pModelPanel = new CItemModelPanel( this, "modelpanel" );

	m_pBackpackPanel = new CBackpackPanel( this, "backpack_panel" );
	m_pConfirmDeleteDialog = NULL;

	m_bDiscardedNewItem = false;
	m_bMadeRoom = false;
	m_pDiscardedLabelCarat = NULL;
	m_pDiscardedLabel = NULL;
	m_pDiscardButton = NULL;
	m_pCloseButton = NULL;

	m_pItemMouseOverPanel = new CItemModelPanel( this, "ItemMouseOverItemPanel" );
	m_pItemToolTip = new CSimplePanelToolTip( this );
	m_pItemToolTip->SetControlledPanel( m_pItemMouseOverPanel );
	m_pModelPanel->SetTooltip( m_pItemToolTip, "" );
	m_pModelPanel->SetActAsButton( false, true );

	ListenForGameEvent( "gameui_hidden" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pItemMouseOverPanel->InvalidateLayout( true, true );
	m_pModelPanel->InvalidateLayout( true, true );
	LoadControlSettings( "Resource/UI/econ/ItemDiscardPanel.res" );

	// Highlight the X on the discard button
	m_pDiscardButton = dynamic_cast<CExButton*>( FindChildByName("DiscardButton") );
	if ( m_pDiscardButton )
	{
		SetXToRed( m_pDiscardButton );
	}
	m_pCloseButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );

	m_pDiscardedLabel = dynamic_cast<vgui::Label*>( FindChildByName("DiscardedLabel") );
	m_pDiscardedLabelCarat = dynamic_cast<vgui::Label*>( FindChildByName("DiscardedCaratLabel") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();

	m_pDiscardedLabelCarat->SetVisible( m_bDiscardedNewItem );
	m_pDiscardedLabel->SetVisible( m_bDiscardedNewItem );
	m_pDiscardButton->SetVisible( !m_bDiscardedNewItem && !m_bMadeRoom );
	m_pCloseButton->SetVisible( m_bDiscardedNewItem || m_bMadeRoom );

	if ( m_pItemMouseOverPanel->IsVisible() )
	{
		m_pItemMouseOverPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		m_bDiscardedNewItem = false;
		m_bMadeRoom = false;
		m_pBackpackPanel->ShowPanel( 0, true );
		InvalidateLayout();
	}
	else
	{
		if ( m_pConfirmDeleteDialog )
		{
			m_pConfirmDeleteDialog->MarkForDeletion();
			m_pConfirmDeleteDialog = NULL;
		}
	}

	SetMouseInputEnabled( bShow );
	SetVisible( bShow );

#ifdef TF_CLIENT_DLL
	// If the player made room and is a trial account, suggest that they upgrade to get more space.
	if ( bShow && IsFreeTrialAccount() )
	{
		ShowUpgradeMessageBox( "#TF_TrialNeedSpace_Title", "#TF_TrialNeedSpace_Text" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::FireGameEvent( IGameEvent *event )
{
	if ( !IsVisible() )
		return;

	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		// If they haven't discarded down to <MAX items, bring us right back up again
		if ( InventoryManager()->CheckForRoomAndForceDiscard() )
		{
			engine->ClientCmd_Unrestricted( "gameui_activate" );
		}
		else
		{
			ShowPanel( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
	{
		ShowPanel( false );

		// Check to make sure the player has room for all his items. If not, bring up the discard panel. Otherwise, go away.
		if ( !InventoryManager()->CheckForRoomAndForceDiscard() )
		{
			// If we're connected to a game server, we also close the game UI.
			if ( engine->IsInGame() )
			{
				engine->ClientCmd_Unrestricted( "gameui_hide" );
			}
		}
	}
	else if ( !Q_stricmp( command, "discarditem" ) )
	{
		// Bring up confirm dialog
		CConfirmDeleteItemDialog *pConfirm = vgui::SETUP_PANEL( new CConfirmDeleteItemDialog( this ) );
		if ( pConfirm )
		{
			pConfirm->Show();

			m_pConfirmDeleteDialog = pConfirm;
		}
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::SetItem( CEconItemView *pItem )
{
	if ( m_pModelPanel )
	{
		m_pModelPanel->SetItem( pItem );
	}

	if ( m_pItemMouseOverPanel )
	{
		m_pItemMouseOverPanel->SetItem( pItem );
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_pItemMouseOverPanel->InvalidateLayout(true);
		m_pItemMouseOverPanel->SetBorder( pScheme->GetBorder("MainMenuBGBorder") );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemDiscardPanel::OnConfirmDelete( KeyValues *data )
{
	if ( !m_bDiscardedNewItem && !m_bMadeRoom && data && m_pModelPanel->HasItem() )
	{
		int iConfirmed = data->GetInt( "confirmed", 0 );
		if ( iConfirmed == 2 )
		{
			// Player discarded an item from the backpack to make room
			m_bMadeRoom = true;
			InvalidateLayout();
		}
		else if ( iConfirmed )
		{
			CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
			if ( !pInventory )
				return;

			InventoryManager()->DropItem( m_pModelPanel->GetItem()->GetItemID() );
			m_bDiscardedNewItem = true;
			InvalidateLayout();
		}

		m_pBackpackPanel->RequestFocus();
	}

	m_pConfirmDeleteDialog = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Handles the escape key
//-----------------------------------------------------------------------------
void	CItemDiscardPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	if( code == KEY_ESCAPE )
	{
		OnCommand( "vguicancel" );
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles button press code for controllers and enter
//-----------------------------------------------------------------------------
void	CItemDiscardPanel::OnKeyCodePressed( vgui::KeyCode code )
{	
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if( nButtonCode == KEY_XBUTTON_B )
	{
		OnCommand( "vguicancel" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}



static vgui::DHANDLE<CItemDiscardPanel> g_ItemDiscardPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemDiscardPanel *OpenItemDiscardPanel( void )
{
	if (!g_ItemDiscardPanel.Get())
	{
		g_ItemDiscardPanel = vgui::SETUP_PANEL( new CItemDiscardPanel( NULL ) );
		g_ItemDiscardPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_ItemDiscardPanel->ShowPanel( true );

	return g_ItemDiscardPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemDiscardPanel *GetItemDiscardPanel( void )
{
	return g_ItemDiscardPanel.Get();
}


