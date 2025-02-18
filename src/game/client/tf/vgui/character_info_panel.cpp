//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "character_info_panel.h"
#include "tf_statsummary.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui/IInput.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include "charinfo_loadout_subpanel.h"
#include "charinfo_armory_subpanel.h"
#include "ienginevgui.h"
#include "tf_hud_statpanel.h"
#include "c_tf_player.h"
#include "tf_item_inventory.h"
#include "econ_notifications.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include "econ_ui.h"
#include "c_tf_gamestats.h"
#include "tf_item_pickup_panel.h"
#include "store/v1/tf_store_panel.h"
#include "store/v2/tf_store_panel2.h"
#include "store/tf_store.h"
#include "tf_matchmaking_dashboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static vgui::DHANDLE<CCharacterInfoPanel> g_CharInfoPanel;
CCharacterInfoPanel* GetCharInfoPanel( bool bRecreate )
{
	if ( bRecreate && g_CharInfoPanel.Get() )
	{
		g_CharInfoPanel->MarkForDeletion();
		g_CharInfoPanel = NULL;
	}

	if (!g_CharInfoPanel.Get())
	{
		g_CharInfoPanel = new CCharacterInfoPanel( NULL );
		g_CharInfoPanel->MakeReadyForUse();
		g_CharInfoPanel->InvalidateLayout( false, true );
	}
	return g_CharInfoPanel;
}

CON_COMMAND( reload_char_info, "Reloads the char info panel" )
{
	GetCharInfoPanel( true );
}

IEconRootUI* EconUI( void )
{
	return GetCharInfoPanel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerNotConnectedToSteamDialog *OpenServerNotConnectedToSteamDialog( vgui::Panel *pParent );

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CCharacterInfoPanel::CCharacterInfoPanel( Panel *parent ) : PropertyDialog(parent, "character_info")
{
	// Character info is parented to the game UI panel
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	// Character loadouts
	m_pLoadoutPanel = new CCharInfoLoadoutSubPanel(this);
	m_pLoadoutPanel->AddActionSignalTarget( this );
	AddPage( m_pLoadoutPanel, "#Loadout");

	// Stat summary
	CTFStatsSummaryPanel *pStatSummaryPanel = new CTFStatsSummaryPanel(this);
	pStatSummaryPanel->SetupForEmbedded();
	AddPage( pStatSummaryPanel, "#Stats");
	CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
	if ( pStatPanel )
	{
		// Ask for our embedded stat summary be updated immediately
		pStatPanel->UpdateStatSummaryPanel();
	}

	// Achievements
	//AddPage(new CCharacterInfoSubAchievements(this), "#Achievements");

	ListenForGameEvent( "gameui_hidden" );

	m_pLoadoutPanel->SetVisible( false );

	m_pNotificationsPresentPanel = NULL;
	m_bPreventClosure = false;
	m_iClosePanel = ECONUI_BASEUI;
	m_iDefaultTeam = TF_TEAM_RED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCharacterInfoPanel::~CCharacterInfoPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/CharInfoPanel.res" );

	SetOKButtonVisible(false);
	SetCancelButtonVisible(false);

	m_pNotificationsPresentPanel = FindChildByName( "NotificationsPresentPanel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::PerformLayout( void ) 
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
void CCharacterInfoPanel::ShowPanel(bool bShow)
{
	m_bPreventClosure = false;

	if ( bShow )
	{
		if ( GetPropertySheet()->GetActivePage() != m_pLoadoutPanel )
		{
			GetPropertySheet()->SetActivePage( m_pLoadoutPanel );
		}
		else
		{
			// VGUI doesn't tell the starting active page that it's active, so we post a pageshow to it
			ivgui()->PostMessage( m_pLoadoutPanel->GetVPanel(), new KeyValues("PageShow"), GetPropertySheet()->GetVPanel() );
		}

		//InvalidateLayout( false, true );
		Activate();

		int iClass = m_pLoadoutPanel->GetCurrentClassIndex();
		OpenLoadoutToClass( iClass, false );
	}
	else
	{
		PostMessage( m_pLoadoutPanel, new KeyValues("CancelSelection") );
	}

	bool bWasVisible = IsVisible() && m_pLoadoutPanel->IsVisible();
	SetVisible( bShow );
	if ( bWasVisible && !bShow )
	{
		m_pLoadoutPanel->OnCharInfoClosing();

		// Clear this out so it doesn't affect anything the next time the econ UI is opened
		m_iClosePanel = ECONUI_BASEUI;
		m_iDefaultTeam = TF_TEAM_RED;
	}

	m_pLoadoutPanel->SetVisible( bShow );

	// When we first appear, if we're on a server that couldn't get our loadout, show the failure dialog.
	if ( !bWasVisible && bShow )
	{
		if ( engine->IsInGame() )
		{
			C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocal && pLocal->m_Shared.IsLoadoutUnavailable() )
			{
				OpenServerNotConnectedToSteamDialog( this );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::FireGameEvent( IGameEvent *event )
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
void CCharacterInfoPanel::Close()
{
	ShowPanel( false );

	PostMessage( m_pLoadoutPanel, new KeyValues("CharInfoClosing") );

	// If we're connected to a game server, we also close the game UI.
	if ( engine->IsInGame() )
	{
		bool bClose = true;
		if ( m_bCheckForRoomOnExit )
		{
			// Check to make sure the player has room for all his items. If not, bring up the discard panel. Otherwise, go away.
			// We need to do this to catch players who used the "Change Loadout" button in the pickup panel, and may be out of room.
			bClose = !TFInventoryManager()->CheckForRoomAndForceDiscard();
		}

		if ( bClose )
		{
			engine->ClientCmd_Unrestricted( "gameui_hide" );
		}
	}

	// Notify any listeners that we're closed
	NotifyListenersOfCloseEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::NotifyListenersOfCloseEvent()
{
	FOR_EACH_VEC( m_vecOnCloseListeners, i )
	{
		if ( m_vecOnCloseListeners[i].Get() )
		{
			PostMessage( m_vecOnCloseListeners[i].Get(), new KeyValues( "EconUIClosed" ) );
		}
	}

	m_vecOnCloseListeners.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "back" ) )
	{
		// If we're inspecting an item, just close the inspection panel
		if ( m_pLoadoutPanel->GetInspectionPanel()->IsVisible() )
		{
			m_pLoadoutPanel->GetInspectionPanel()->OnCommand( "close" );
			// This is such a hack.  I don't have time to figure this out, so we're just going
			// to special case this.  Don't "open" the CHAP_LOADOUT if the backback was up or
			// else we'll get sucked back to CHAP_LOADOUT
			if ( !m_pLoadoutPanel->GetBackpackPanel()->IsVisible() )
			{
				m_pLoadoutPanel->OpenSubPanel( CHAP_LOADOUT );
			}
			return;
		}

		// If we're at the base loadout page, or if we want to force it, close the dialog completely...
		// NOTE: Right now we don't support closing from the item selection screen.
		const int iShowingPanel = m_pLoadoutPanel->GetShowingPanel();
		const int iCurrentClassIndex = m_pLoadoutPanel->GetCurrentClassIndex();
		const bool bIsInSelectionPanel = iShowingPanel == CHAP_LOADOUT && m_pLoadoutPanel->GetClassLoadoutPanel()->IsInSelectionPanel();
		const bool bNoClass = iCurrentClassIndex == TF_CLASS_UNDEFINED;
		const bool bAtClosePanel = !bIsInSelectionPanel &&
			( ( iShowingPanel == m_iClosePanel && bNoClass ) || ( iShowingPanel == CHAP_LOADOUT && -m_iClosePanel == iCurrentClassIndex ) );
		const bool bAtBaseLoadoutPage = iShowingPanel == CHAP_LOADOUT && bNoClass;
		if ( bAtClosePanel || bAtBaseLoadoutPage )
		{
			Close();
		}
		// In the item selection panel?
		else if ( bIsInSelectionPanel )
		{
			m_pLoadoutPanel->GetClassLoadoutPanel()->GetItemSelectionPanel()->OnBackPressed();
		}
		// In any other panel, just go back.
		else
		{
			ShowPanel( true );
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
void CCharacterInfoPanel::OpenLoadoutToClass( int iClassIndex, bool bOpenClassLoadout ) 
{ 
	Assert(iClassIndex >= TF_CLASS_UNDEFINED && iClassIndex < TF_CLASS_COUNT); 
	m_pLoadoutPanel->SetClassIndex( iClassIndex, bOpenClassLoadout );
	m_pLoadoutPanel->SetTeamIndex( m_iDefaultTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OpenLoadoutToBackpack( void ) 
{ 
	m_pLoadoutPanel->OpenToBackpack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OpenLoadoutToCrafting( void ) 
{ 
	m_pLoadoutPanel->OpenToCrafting();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OpenLoadoutToArmory( void ) 
{ 
	m_pLoadoutPanel->OpenToArmory();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OpenToPaintkitPreview( CEconItemView* pItem, bool bFixedItem, bool bFixedPaintkit )
{
	m_pLoadoutPanel->OpenToPaintkitPreview( pItem, bFixedItem, bFixedPaintkit );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OnOpenArmoryDirect( KeyValues *data )
{
	int iItemDef = data->GetInt( "itemdef", 0 );
	m_pLoadoutPanel->OpenToArmory( iItemDef );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		if ( !m_bPreventClosure )
		{
			OnCommand( "back" );
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
void CCharacterInfoPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if ( nButtonCode == KEY_XBUTTON_B )
	{
		if ( !m_bPreventClosure )
		{
			OnCommand( "back" );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::OnThink()
{
	if ( g_pClientMode && g_pClientMode->GetViewport() && g_pClientMode->GetViewportAnimationController() )
	{
		bool bShouldBeVisible = NotificationQueue_GetNumNotifications() != 0;

		bShouldBeVisible = false;

		if ( m_pNotificationsPresentPanel != NULL && m_pNotificationsPresentPanel->IsVisible() != bShouldBeVisible )
		{
			m_pNotificationsPresentPanel->SetVisible( bShouldBeVisible );
			if ( bShouldBeVisible )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "NotificationsPresentBlink" );
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "NotificationsPresentBlinkStop" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IEconRootUI	*CCharacterInfoPanel::OpenEconUI( int iDirectToPage, bool bCheckForInventorySpaceOnExit )
{
	if ( IsLayoutInvalid() )
	{
		MakeReadyForUse();
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	ShowPanel( true );

	if ( iDirectToPage == ECONUI_BACKPACK )
	{
		OpenLoadoutToBackpack();
	}
	else if ( iDirectToPage == ECONUI_CRAFTING )
	{
		OpenLoadoutToCrafting();
	}
	else if ( iDirectToPage == ECONUI_ARMORY )
	{
		OpenLoadoutToArmory();
	}
	else if ( iDirectToPage < 0 )
	{
		// Negative numbers go directly to the class loadout
		OpenLoadoutToClass( -(iDirectToPage), true );
	}

	SetCheckForRoomOnExit( bCheckForInventorySpaceOnExit );

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::CloseEconUI( void )
{
	if ( IsVisible() )
	{
		ShowPanel( false );
		NotifyListenersOfCloseEvent();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCharacterInfoPanel::IsUIPanelVisible( EconBaseUIPanels_t iPanel )
{
	if ( !IsVisible() )
		return false;

	switch ( iPanel )
	{
	case ECONUI_BACKPACK:
		return (GetBackpackPanel() && GetBackpackPanel()->IsVisible());

	case ECONUI_CRAFTING:
		return (GetCraftingPanel() && GetCraftingPanel()->IsVisible());

	case ECONUI_ARMORY:
		return (GetArmoryPanel() && GetArmoryPanel()->IsVisible());

	case ECONUI_TRADING:
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
void Open_CharInfo( const CCommand &args )
{
	EconUI()->OpenEconUI();
}
ConCommand open_charinfo( "open_charinfo", Open_CharInfo, "Open the character info panel", FCVAR_NONE );

void CCharacterInfoPanel::SetPreventClosure( bool bPrevent )
{ 
	m_bPreventClosure = bPrevent;

	Panel* pBackButton = FindChildByName( "BackButton" );
	if ( pBackButton )
	{
		pBackButton->SetEnabled( !bPrevent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_CharInfoDirect( const CCommand &args )
{
	// If we're in-game, start by opening the class we're currently playing
	int iClass = TF_CLASS_UNDEFINED;
	if ( engine->IsInGame() )
	{
		C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocal )
		{
			iClass = -(pLocal->m_Shared.GetDesiredPlayerClassIndex());
			if ( iClass == TF_CLASS_UNDEFINED )
			{
				iClass = -(pLocal->GetPlayerClass()->GetClassIndex());
			}
		}
	}

	// override with command arg
	if ( args.ArgC() > 1 )
	{
		iClass = -atoi( args.Arg( 1 ) );
	}

	EconUI()->OpenEconUI( iClass );	
}
ConCommand open_charinfo_direct( "open_charinfo_direct", Open_CharInfoDirect, "Open the character info panel directly to the class you're currently playing.", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_CharInfoBackpack( const CCommand &args )
{
	EconUI()->OpenEconUI( ECONUI_BACKPACK );	
}
ConCommand open_charinfo_backpack( "open_charinfo_backpack", Open_CharInfoBackpack, "Open the character info panel directly to backpack.", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_CharInfoCrafting( const CCommand &args )
{
	EconUI()->OpenEconUI( ECONUI_CRAFTING );	
}
ConCommand open_charinfo_crafting( "open_charinfo_crafting", Open_CharInfoCrafting, "Open the character info panel directly to crafting screen.", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_CharInfoArmory( const CCommand &args )
{
	EconUI()->OpenEconUI( ECONUI_ARMORY );	
}
ConCommand open_charinfo_armory( "open_charinfo_armory", Open_CharInfoArmory, "Open the character info panel directly to armory.", FCVAR_NONE );


//================================================================================================================================
// NOT CONNECTED TO STEAM WARNING DIALOG
//================================================================================================================================
static vgui::DHANDLE<CServerNotConnectedToSteamDialog> g_ServerNotConnectedPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerNotConnectedToSteamDialog::CServerNotConnectedToSteamDialog( vgui::Panel *pParent, const char *pElementName ) : BaseClass( pParent, "ServerNotConnectedToSteamDialog" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerNotConnectedToSteamDialog::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/ServerNotConnectedToSteam.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CServerNotConnectedToSteamDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		TFModalStack()->PopModal( this );
		SetVisible( false );
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerNotConnectedToSteamDialog *OpenServerNotConnectedToSteamDialog( vgui::Panel *pParent )
{
	if (!g_ServerNotConnectedPanel.Get())
	{
		g_ServerNotConnectedPanel = vgui::SETUP_PANEL( new CServerNotConnectedToSteamDialog( pParent, NULL ) );
	}
	g_ServerNotConnectedPanel->InvalidateLayout( false, true );

	g_ServerNotConnectedPanel->SetVisible( true );
	g_ServerNotConnectedPanel->MakePopup();
	g_ServerNotConnectedPanel->MoveToFront();
	g_ServerNotConnectedPanel->SetKeyBoardInputEnabled(true);
	g_ServerNotConnectedPanel->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_ServerNotConnectedPanel );
	return g_ServerNotConnectedPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBackpackPanel *CCharacterInfoPanel::GetBackpackPanel( void ) 
{ 
	return m_pLoadoutPanel->GetBackpackPanel(); 
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCraftingPanel *CCharacterInfoPanel::GetCraftingPanel( void ) 
{ 
	return m_pLoadoutPanel->GetCraftingPanel(); 
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CArmoryPanel *CCharacterInfoPanel::GetArmoryPanel( void ) 
{ 
	return m_pLoadoutPanel->GetArmoryPanel(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::Gamestats_ItemTransaction( int eventID, CEconItemView *item, const char *pszReason, int iQuality )
{
	C_CTF_GameStats.Event_ItemTransaction( eventID, item, pszReason, iQuality );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::Gamestats_Store( int eventID, CEconItemView* item, const char* panelName, int classId, 
			const cart_item_t* cartItem, int checkoutAttempts, const char* storeError, int totalPrice, int currencyCode )
{
	C_CTF_GameStats.Event_Store( eventID, item, panelName, classId, cartItem, checkoutAttempts, storeError, totalPrice, currencyCode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::SetExperimentValue( uint64 experimentValue )
{
	C_CTF_GameStats.SetExperimentValue( experimentValue );
}

static vgui::DHANDLE<CTFItemPickupPanel> g_TFItemPickupPanel;
static vgui::DHANDLE<CTFItemDiscardPanel> g_TFItemDiscardPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemPickupPanel *CCharacterInfoPanel::OpenItemPickupPanel( void )
{
	if (!g_TFItemPickupPanel.Get())
	{
		g_TFItemPickupPanel = vgui::SETUP_PANEL( new CTFItemPickupPanel( NULL ) );
		g_TFItemPickupPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_TFItemPickupPanel->ShowPanel( true );

	return g_TFItemPickupPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemDiscardPanel *CCharacterInfoPanel::OpenItemDiscardPanel( void )
{
	if (!g_TFItemDiscardPanel.Get())
	{
		g_TFItemDiscardPanel = vgui::SETUP_PANEL( new CTFItemDiscardPanel( NULL ) );
		g_TFItemDiscardPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_TFItemDiscardPanel->ShowPanel( true );

	return g_TFItemDiscardPanel;
}

static vgui::DHANDLE<CTFBaseStorePanel> g_StorePanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::CreateStorePanel( void )
{
	// Clean up previous store panel?
	if ( g_StorePanel.Get() != NULL )
	{
		g_StorePanel->MarkForDeletion();
	}

	// Create the store panel
	CTFBaseStorePanel *pStorePanel = NULL;
	if ( ShouldUseNewStore() )
	{
		pStorePanel = new CTFStorePanel2( NULL );
	}
	else
	{
		pStorePanel = new CTFStorePanel1( NULL );
	}

	g_StorePanel = vgui::SETUP_PANEL( pStorePanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePanel	*CCharacterInfoPanel::OpenStorePanel( int iItemDef, bool bAddToCart )
{
	return NULL;

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
CStorePanel	*CCharacterInfoPanel::GetStorePanel( void )
{
	return g_StorePanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::AddPanelCloseListener( vgui::Panel *pListener )
{
	if ( !pListener )
		return;

	VPanelHandle hPanel;
	hPanel.Set( pListener->GetVPanel() );
	m_vecOnCloseListeners.AddToHead( hPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharacterInfoPanel::SetClosePanel( int iPanel )
{
	AssertMsg( ( iPanel < 0 && IsValidTFPlayerClass( -iPanel ) ) ||
		( iPanel >= ECONUI_FIRST_PANEL && iPanel <= ECONUI_LAST_PANEL ),
		"Panel out of range!"
	);
	m_iClosePanel = iPanel;
}

void CCharacterInfoPanel::SetDefaultTeam( int iTeam )
{
	AssertMsg( iTeam == TF_TEAM_RED || iTeam == TF_TEAM_BLUE, "Invalid team" );
	m_iDefaultTeam = iTeam;
}

//================================================================================================================================
// NOT CONNECTED TO STEAM WARNING DIALOG
//================================================================================================================================
static vgui::DHANDLE<CCheatDetectionDialog> g_CheatDetectionDialog;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCheatDetectionDialog::CCheatDetectionDialog( vgui::Panel *pParent, const char *pElementName ) : BaseClass( pParent, "CheatDetectionDialog" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCheatDetectionDialog::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/CheatDetectionDialog.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCheatDetectionDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		TFModalStack()->PopModal( this );
		SetVisible( false );
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCheatDetectionDialog *OpenCheatDetectionDialog( vgui::Panel *pParent, const char *pszCheatMessage )
{
	if (!g_CheatDetectionDialog.Get())
	{
		g_CheatDetectionDialog = vgui::SETUP_PANEL( new CCheatDetectionDialog( pParent, NULL ) );
	}
	g_CheatDetectionDialog->InvalidateLayout( false, true );

	g_CheatDetectionDialog->SetVisible( true );
	g_CheatDetectionDialog->MakePopup();
	g_CheatDetectionDialog->MoveToFront();
	g_CheatDetectionDialog->SetKeyBoardInputEnabled(true);
	g_CheatDetectionDialog->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_CheatDetectionDialog );
	g_CheatDetectionDialog->SetDialogVariable( "reason", g_pVGuiLocalize->Find( pszCheatMessage ) );
	return g_CheatDetectionDialog;
}
