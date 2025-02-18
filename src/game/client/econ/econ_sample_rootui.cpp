//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "econ_sample_rootui.h"
#include "vgui/IInput.h"
#include <vgui/ILocalize.h>
#include "ienginevgui.h"
#include "econ_item_inventory.h"
#include "backpack_panel.h"
#include "item_pickup_panel.h"
#include "store/store_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static vgui::DHANDLE<CEconSampleRootUI> g_EconSampleRootUIPanel;

#if !defined (TF_CLIENT_DLL)
IEconRootUI *EconUI( void )
{
	if (!g_EconSampleRootUIPanel.Get())
	{
		g_EconSampleRootUIPanel = vgui::SETUP_PANEL( new CEconSampleRootUI( NULL ) );
		g_EconSampleRootUIPanel->InvalidateLayout( false, true );
	}
	return g_EconSampleRootUIPanel;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CEconSampleRootUI::CEconSampleRootUI( vgui::Panel *parent ) : vgui::Frame(parent, "econ_sample_rootui")
{
	// Econ UI is parented to the game UI panel (not the client DLL panel).
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );
	SetCloseButtonVisible( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	// Create our subpanels
	m_pBackpackPanel = new CBackpackPanel( this, "backpack_panel" );

	// Start with just the base UI visible
	m_nVisiblePanel = ECONUI_BASEUI;
	UpdateSubPanelVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconSampleRootUI::~CEconSampleRootUI()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/Econ/RootUI.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::ShowPanel(bool bShow)
{
	m_bPreventClosure = false;
	SetVisible( bShow );

	if ( bShow )
	{
		MoveToFront();
		m_nVisiblePanel = ECONUI_BASEUI;
		UpdateSubPanelVisibility();

		InventoryManager()->ShowItemsPickedUp( true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		if ( m_bPreventClosure )
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
void CEconSampleRootUI::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		ShowPanel( false );

		// If we're connected to a game server, we also close the game UI.
		if ( engine->IsInGame() )
		{
			bool bClose = true;
			if ( m_bCheckForRoomOnExit )
			{
				// Check to make sure the player has room for all his items. If not, bring up the discard panel. Otherwise, go away.
				// We need to do this to catch players who used the "Change Loadout" button in the pickup panel, and may be out of room.
				bClose = !InventoryManager()->CheckForRoomAndForceDiscard();
			}
			
			if ( bClose )
			{
				engine->ClientCmd_Unrestricted( "gameui_hide" );
			}
		}
	}
	else if ( !Q_stricmp( command, "back" ) )
	{
		ShowPanel( true );
	}
	else if ( !Q_stricmp( command, "loadout" ) )
	{
		// Ignore selection while we don't have a steam connection
		if ( !InventoryManager()->GetLocalInventory()->RetrievedInventoryFromSteam() )
			return;

		OpenSubPanel( ECONUI_LOADOUT );
	}
	else if ( !Q_strnicmp( command, "backpack", 8 ) )
	{
		OpenSubPanel( ECONUI_BACKPACK );
	}
	else if ( !Q_strnicmp( command, "crafting", 8 ) )
	{
		OpenSubPanel( ECONUI_CRAFTING );
	}
	else if ( !Q_strnicmp( command, "armory", 6 ) )
	{
		OpenSubPanel( ECONUI_ARMORY );
	}
	else if ( !Q_strnicmp( command, "store", 5 ) )
	{
		EconUI()->OpenStorePanel( 0, false );	
	}
	else if ( !Q_strnicmp( command, "trading", 7 ) )
	{
// 		if ( IsFreeTrialAccount() )
// 		{
// 			ShowMessageBox( "#TF_Trial_CannotTrade_Title",  "#TF_Trial_CannotTrade_Text", "#GameUI_OK" );
//			return;
// 		}

		OpenTradingStartDialog();
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
void CEconSampleRootUI::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		if ( !m_bPreventClosure )
		{
			ShowPanel( false );
		}
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IEconRootUI	*CEconSampleRootUI::OpenEconUI( int iDirectToPage, bool bCheckForInventorySpaceOnExit )
{
	engine->ClientCmd_Unrestricted( "gameui_activate" );
	ShowPanel( true );

	if ( iDirectToPage == ECONUI_BACKPACK )
	{
	}
	else if ( iDirectToPage == ECONUI_CRAFTING )
	{
	}
	else if ( iDirectToPage == ECONUI_ARMORY )
	{
	}

	SetCheckForRoomOnExit( bCheckForInventorySpaceOnExit );

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::CloseEconUI( void )
{
	if ( IsVisible() )
	{
		ShowPanel( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconSampleRootUI::IsUIPanelVisible( EconBaseUIPanels_t iPanel )
{
	if ( !IsVisible() )
		return false;

	switch ( iPanel )
	{
	case ECONUI_BASEUI:
		return true;

	case ECONUI_BACKPACK:
		return (GetBackpackPanel() && GetBackpackPanel()->IsVisible());

	case ECONUI_CRAFTING:
		//return (GetCraftingPanel() && GetCraftingPanel()->IsVisible());
		break;

	case ECONUI_ARMORY:
		break;

	case ECONUI_TRADING:
		break;
	case ECONUI_LOADOUT:
		break;

	default:
		Assert(0);
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::OpenSubPanel( EconBaseUIPanels_t nPanel )
{
	m_nVisiblePanel = nPanel;
	UpdateSubPanelVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::OpenTradingStartDialog( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::UpdateSubPanelVisibility( void )
{
	bool bBackpackVisible = (m_nVisiblePanel == ECONUI_BACKPACK);
	if ( m_pBackpackPanel->IsVisible() != bBackpackVisible )
	{
		m_pBackpackPanel->ShowPanel( false, bBackpackVisible );
	}

	//m_pClassLoadoutPanel->SetVisible( false );
	//m_pArmoryPanel->SetVisible( false );
	//m_pCraftingPanel->ShowPanel( m_iCurrentClassIndex, true, (m_iPrevShowingPanel == CHAP_ARMORY) );
}

static vgui::DHANDLE<CItemPickupPanel> g_ItemPickupPanel;
static vgui::DHANDLE<CItemDiscardPanel> g_ItemDiscardPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel *CEconSampleRootUI::OpenItemPickupPanel( void )
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
CItemDiscardPanel *CEconSampleRootUI::OpenItemDiscardPanel( void )
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

static vgui::DHANDLE<CStorePanel> g_StorePanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconSampleRootUI::CreateStorePanel( void )
{
	// Clean up previous store panel?
	if ( g_StorePanel.Get() != NULL )
	{
		g_StorePanel->MarkForDeletion();
	}

	// Create the store panel
	g_StorePanel = vgui::SETUP_PANEL( new CStorePanel( NULL ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePanel	*CEconSampleRootUI::OpenStorePanel( int iItemDef, bool bAddToCart )
{
	// Make sure we've got the appropriate connections to Steam
	if ( !steamapicontext || !steamapicontext->SteamUtils() )
	{
		OpenStoreStatusDialog( NULL, "#StoreUpdate_SteamRequired", true, false );
		return NULL;
	}

	if ( !steamapicontext->SteamUtils()->IsOverlayEnabled() )
	{
		OpenStoreStatusDialog( NULL, "#StoreUpdate_OverlayRequired", true, false );
		return NULL;
	}

	if ( !CStorePanel::IsPricesheetLoaded() )
	{
		OpenStoreStatusDialog( NULL, "#StoreUpdate_Loading", false, false );

		CStorePanel::SetShouldShowWarnings( true );
		CStorePanel::RequestPricesheet();
		return NULL;
	}

	if ( !g_StorePanel )
		return NULL;

	engine->ClientCmd_Unrestricted( "gameui_activate" );

	if ( iItemDef )
	{
		g_StorePanel->StartAtItemDef( iItemDef, bAddToCart );
	}

	g_StorePanel->ShowPanel( true );

	return g_StorePanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePanel	*CEconSampleRootUI::GetStorePanel( void )
{
	return g_StorePanel;	
}

#ifdef _DEBUG
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_EconUI( const CCommand &args )
{
	EconUI()->OpenEconUI();
}
ConCommand open_econui( "open_econui", Open_EconUI );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_EconUIBackpack( const CCommand &args )
{
	EconUI()->OpenEconUI( ECONUI_BACKPACK );	
}
ConCommand open_econui_backpack( "open_econui_backpack", Open_EconUIBackpack, "Open the backpack.", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_EconUICrafting( const CCommand &args )
{
	EconUI()->OpenEconUI( ECONUI_CRAFTING );	
}
ConCommand open_econui_crafting( "open_econui_crafting", Open_EconUICrafting, "Open the crafting screen.", FCVAR_NONE );
#endif // _DEBUG
