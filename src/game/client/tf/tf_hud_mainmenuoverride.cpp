//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_hud_mainmenuoverride.h"
#include "ienginevgui.h"
#include "tf_viewport.h"
#include "clientmode_tf.h"
#include "confirm_dialog.h"
#include <vgui/ILocalize.h>
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "tf_statsummary.h"
#include "rtime.h"
#include "tf_gcmessages.h"
#include "econ_notifications.h"
#include "c_tf_freeaccount.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "gcsdk/gcclient.h"
#include "gcsdk/gcclientjob.h"
#include "econ_item_system.h"
#include <vgui_controls/AnimationController.h>
#include "store/store_panel.h"
#include "gc_clientsystem.h"
#include <vgui_controls/ScrollBarSlider.h>
#include "filesystem.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_partyclient.h"
#include "sourcevr/isourcevirtualreality.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "tf_warinfopanel.h"
#include "quest_log_panel.h"
#include "tf_item_inventory.h"
#include "quest_log_panel.h"
#include "econ_quests.h"
#include "tf_streams.h"
#include "tf_matchmaking_shared.h"
#include "tf_lobby_container_frame_comp.h"
#include "tf_lobby_container_frame_mvm.h"
#include "tf_lobby_container_frame_casual.h"
#include "tf_badge_panel.h"
#include "tf_quest_map_panel.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_matchmaking_dashboard_comp_rank_tooltip.h"
#include "tf_rating_data.h"
#include "tf_progression.h"

#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/vgui/replayperformanceeditor.h"
#include "materialsystem/itexture.h"
#include "imageutils.h"
#include "icommandline.h"
#include "vgui/ISystem.h"
#include "mute_player_dialog.h"
#include "tf_quest_map_utils.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_pvp_rank_panel.h"

#include "econ_paintkit.h"
#include "ienginevgui.h"


#include "c_tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CMOTDManager CHudMainMenuOverride::m_MOTDManager;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

extern const char *g_sImagesBlue[];
extern int EconWear_ToIntCategory( float flWear );

void cc_tf_safemode_toggle( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	if ( pMMOverride )
	{
		pMMOverride->InvalidateLayout();
	}
}

void cc_tf_mainmenu_match_panel_type( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	if ( pMMOverride )
	{
		pMMOverride->UpdateRankPanelType();
	}
}


ConVar tf_recent_achievements( "tf_recent_achievements", "0", FCVAR_ARCHIVE );
ConVar tf_find_a_match_hint_viewed( "tf_find_a_match_hint_viewed", "0", FCVAR_ARCHIVE );
ConVar tf_training_has_prompted_for_training( "tf_training_has_prompted_for_training", "0", FCVAR_ARCHIVE, "Whether the user has been prompted for training" );
ConVar tf_training_has_prompted_for_offline_practice( "tf_training_has_prompted_for_offline_practice", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to try offline practice." );
ConVar tf_training_has_prompted_for_forums( "tf_training_has_prompted_for_forums", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to view the new user forums." );
ConVar tf_training_has_prompted_for_options( "tf_training_has_prompted_for_options", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to view the TF2 advanced options." );
ConVar tf_training_has_prompted_for_loadout( "tf_training_has_prompted_for_loadout", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to equip something in their loadout." );
ConVar cl_ask_bigpicture_controller_opt_out( "cl_ask_bigpicture_controller_opt_out", "0", FCVAR_ARCHIVE, "Whether the user has opted out of being prompted for controller support in Big Picture." );
ConVar cl_mainmenu_operation_motd_start( "cl_mainmenu_operation_motd_start", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar cl_mainmenu_operation_motd_reset( "cl_mainmenu_operation_motd_reset", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar cl_mainmenu_safemode( "cl_mainmenu_safemode", "0", FCVAR_NONE, "Enable safe mode", cc_tf_safemode_toggle );
ConVar cl_mainmenu_updateglow( "cl_mainmenu_updateglow", "1", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar tf_mainmenu_match_panel_type( "tf_mainmenu_match_panel_type", "7", FCVAR_ARCHIVE | FCVAR_HIDDEN, "The match group data to show on the main menu", cc_tf_mainmenu_match_panel_type );

void cc_promotional_codes_button_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	if ( pMMOverride )
	{
		( (CHudMainMenuOverride*)pMMOverride )->UpdatePromotionalCodes();
	}
}
ConVar cl_promotional_codes_button_show( "cl_promotional_codes_button_show", "1", FCVAR_ARCHIVE, "Toggles the 'View Promotional Codes' button in the main menu for players that have used the 'RIFT Well Spun Hat Claim Code'.", cc_promotional_codes_button_changed );

extern bool Training_IsComplete();

void PromptOrFireCommand( const char* pszCommand )
{
	if ( engine->IsInGame()  )
	{
		CTFDisconnectConfirmDialog *pDialog = BuildDisconnectConfirmDialog();
		if ( pDialog )
		{
			pDialog->Show();
			pDialog->AddConfirmCommand( pszCommand );
		}
	}
	else
	{
		engine->ClientCmd_Unrestricted( pszCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CHudMainMenuOverride::CHudMainMenuOverride( IViewPort *pViewPort ) : BaseClass( NULL, PANEL_MAINMENUOVERRIDE )
{
	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );
	SetVisible( true );

	m_pVRModeButton = NULL;
	m_pVRModeBackground = NULL;

	m_pButtonKV = NULL;
	m_pQuitButton = NULL;
	m_pDisconnectButton = NULL;
	m_pBackToReplaysButton = NULL;
	m_pStoreHasNewItemsImage = NULL;

	m_nLastMOTDRequestAt = 0;
	m_nLastMOTDRequestLanguage = k_Lang_English;
	m_bReloadedAllMOTDs = false;
	m_iCurrentMOTD = -1;
	m_bInitMOTD = false;

	m_pMOTDPanel = NULL;
	m_pMOTDShowPanel = NULL;
	m_pMOTDURLButton = NULL;
	m_pMOTDNextButton = NULL;
	m_pMOTDPrevButton = NULL;
	m_iNotiPanelWide = 0;

	m_bReapplyButtonKVs = false;
	
	m_pMOTDHeaderLabel = NULL;
	m_pMOTDHeaderIcon = NULL;
	m_pMOTDTitleLabel = NULL;
	m_pMOTDTitleImageContainer = NULL;
	m_pMOTDTitleImage = NULL;
	m_hTitleLabelFont = vgui::INVALID_FONT;


	m_bHaveNewMOTDs = false;
	m_bMOTDShownAtStartup = false;

	m_iCharacterImageIdx = -1;


	m_flCheckTrainingAt = 0;
	m_bWasInTraining = false;

	ScheduleItemCheck();

 	m_pToolTip = new CMainMenuToolTip( this );
 	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTipEmbeddedPanel->MoveToFront();
 	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	ListenForGameEvent( "gc_new_session" );
	ListenForGameEvent( "item_schema_initialized" );
	ListenForGameEvent( "store_pricesheet_updated" );
	ListenForGameEvent( "gameui_activated" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "server_spawn" );

	m_pRankPanel = new CPvPRankPanel( this, "rankpanel" );
	m_pRankModelPanel = new CPvPRankPanel( this, "rankmodelpanel" );

	// Create our MOTD scrollable section
	m_pMOTDPanel = new vgui::EditablePanel( this, "MOTD_Panel" );
	m_pMOTDPanel->SetVisible( true );
	m_pMOTDTextPanel = new vgui::EditablePanel( this, "MOTD_TextPanel" );
	m_pMOTDTextScroller = new vgui::ScrollableEditablePanel( m_pMOTDPanel, m_pMOTDTextPanel, "MOTD_TextScroller" );

	m_pMOTDTextScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pMOTDTextScroller->GetScrollbar()->SetPaintBorderEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->SetPaintBackgroundEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->GetButton(0)->SetPaintBorderEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->GetButton(0)->SetPaintBackgroundEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->GetButton(1)->SetPaintBorderEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->GetButton(1)->SetPaintBackgroundEnabled( false );
	m_pMOTDTextScroller->GetScrollbar()->SetAutoResize( PIN_TOPRIGHT, AUTORESIZE_DOWN, -24, 0, -16, 0 );

	m_pMOTDTextLabel = NULL;

	m_pNotificationsShowPanel = NULL;
	m_pNotificationsPanel = new vgui::EditablePanel( this, "Notifications_Panel" );
	m_pNotificationsControl = NotificationQueue_CreateMainMenuUIElement( m_pNotificationsPanel, "Notifications_Control" );
	m_pNotificationsScroller = new vgui::ScrollableEditablePanel( m_pNotificationsPanel, m_pNotificationsControl, "Notifications_Scroller" );

	m_pNotificationsPanel->SetVisible(false);
	m_pNotificationsControl->SetVisible(false);
	m_pNotificationsScroller->SetVisible(false);

	m_iNumNotifications = 0;

	m_pBackground = new vgui::ImagePanel( this, "Background" );
	m_pEventPromoContainer = new EditablePanel( this, "EventPromo" );
	m_pSafeModeContainer = new EditablePanel( this, "SafeMode" );

	m_bStabilizedInitialLayout = false;

	m_bBackgroundUsesCharacterImages = true;

	//m_pWatchStreamsPanel = new CTFStreamListPanel( this, "StreamListPanel" );
	m_pCharacterImagePanel = new ImagePanel( this, "TFCharacterImage" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CHudMainMenuOverride::~CHudMainMenuOverride( void )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_close", "main_menu_override" );

	if ( GetClientModeTFNormal()->GameUI() )
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride( NULL );
	}

	if ( m_pButtonKV )
	{
		m_pButtonKV->deleteThis();
		m_pButtonKV = NULL;
	}

	// Stop Animation Sequences
	if ( m_pNotificationsShowPanel )
	{
		g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( m_pNotificationsShowPanel );
	}

	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Override painting traversal to suppress main menu painting if we're not ready to show yet
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PaintTraverse( bool Repaint, bool allowForce )
{
	// Ugly hack: disable painting until we're done screwing around with updating the layout during initialization.
	// Use -menupaintduringinit command line parameter to reinstate old behavior
	if ( m_bStabilizedInitialLayout || CommandLine()->CheckParm("-menupaintduringinit") )
	{
		BaseClass::PaintTraverse( Repaint, allowForce );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnTick()
{
	if ( m_iNumNotifications != NotificationQueue_GetNumMainMenuNotifications() )
	{
		m_iNumNotifications = NotificationQueue_GetNumMainMenuNotifications();
		UpdateNotifications();
	}
	else if ( m_pNotificationsPanel->IsVisible() )
	{
		AdjustNotificationsPanelHeight();
	}

	static bool s_bRanOnce = false;
	if ( !s_bRanOnce )
	{
		s_bRanOnce = true;
		if ( char const *szConnectAdr = CommandLine()->ParmValue( "+connect" ) )
		{
			Msg( "Executing deferred connect command: %s\n", szConnectAdr );
			engine->ExecuteClientCmd( CFmtStr( "connect %s -%s\n", szConnectAdr, "ConnectStringOnCommandline" ) );
		}
	}


}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AttachToGameUI( void )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_open",	"main_menu_override" );

	if ( GetClientModeTFNormal()->GameUI() )
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride( GetVPanel() );
	}

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	SetCursor(dc_arrow);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ConVar tf_last_store_pricesheet_version( "tf_last_store_pricesheet_version", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD | FCVAR_HIDDEN );

void CHudMainMenuOverride::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp( type, "gc_new_session" ) == 0 )
	{
		char uilanguage[ 64 ];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

		//V_strcpy_safe( uilanguage, "german" );

		ELanguage nCurLang = PchLanguageToELanguage( uilanguage );

		// If we've changed language from when we last requested, we ask for all MOTDs again.
		if ( nCurLang != m_nLastMOTDRequestLanguage )
		{
			m_nLastMOTDRequestAt = 0;
			m_nLastMOTDRequestLanguage = nCurLang;
			m_bReloadedAllMOTDs = true;
		}

		// Ask the GC for the MOTD
		GCSDK::CGCMsg<MsgGCMOTDRequest_t> msg( k_EMsgGCMOTDRequest );
		msg.Body().m_eLanguage = nCurLang;
		msg.Body().m_nLastMOTDRequest = m_nLastMOTDRequestAt;
		GCClientSystem()->BSendMessage( msg );

		// Roll our last asked time forward here. It won't get written to our
		// cache file if we don't get a response from the GC.
		CRTime cTimeHack;
		m_nLastMOTDRequestAt = CRTime::RTime32TimeCur();

		// Load the store info, so we can display the current special

		UpdateRankPanelVisibility();
	}
	else if ( Q_strcmp( type, "item_schema_initialized" ) == 0 )
	{
		// Tell the schema to load its MOTD block from our clientside cache file
		CUtlVector< CUtlString > vecErrors;
		KeyValues *pEntriesKV = new KeyValues( "motd_entries");
		if ( pEntriesKV->LoadFromFile( g_pFullFileSystem, GC_MOTD_CACHE_FILE ) )
		{
			// Extract our last MOTD request time
			const char *pszTime = pEntriesKV->GetString( "last_request_time", NULL );
			m_nLastMOTDRequestAt = ( pszTime && pszTime[0] ) ? CRTime::RTime32FromString(pszTime) : 0;

			const char *pszLang = pEntriesKV->GetString( "last_request_language", NULL );
			m_nLastMOTDRequestLanguage = ( pszLang && pszLang[0] ) ? PchLanguageToELanguage(pszLang) : k_Lang_English;

			// Parse the entries
			GetMOTDManager().BInitMOTDEntries( pEntriesKV, &vecErrors );
			GetMOTDManager().PurgeUnusedMOTDEntries( pEntriesKV );
		}
	}
	else if ( Q_strcmp( type, "store_pricesheet_updated" ) == 0 )
	{
		// If the contents of the store have changed since the last time we went in and/or launched
		// the game, change the button color so that players know there's new content available.
		if ( EconUI() &&
			 EconUI()->GetStorePanel() &&
			 EconUI()->GetStorePanel()->GetPriceSheet() )
		{
			const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();

			// The cvar system can't deal with integers that lose data when represented as floating point
			// numbers. We don't really care about supreme accuracy for detecting changes -- worst case if
			// we change the price sheet almost exactly 18 hours apart, some subset of players won't get the
			// "new!" label and that's fine.
			const uint32 unPriceSheetVersion = (uint32)pPriceSheet->GetVersionStamp() & 0xffff;

			if ( unPriceSheetVersion != (uint32)tf_last_store_pricesheet_version.GetInt() )
			{
				tf_last_store_pricesheet_version.SetValue( (int)unPriceSheetVersion );

				if ( m_pStoreHasNewItemsImage )
				{
					m_pStoreHasNewItemsImage->SetVisible( true );
				}
			}
		}

		// might as well do this here too
		UpdatePromotionalCodes();

		LoadCharacterImageFile();

		if ( NeedsToChooseMostHelpfulFriend() )
		{
			NotifyNeedsToChooseMostHelpfulFriend();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "button_kv" );
	if ( pItemKV )
	{
		if ( m_pButtonKV )
		{
			m_pButtonKV->deleteThis();
		}
		m_pButtonKV = new KeyValues("button_kv");
		pItemKV->CopySubkeys( m_pButtonKV );

		m_bReapplyButtonKVs = true;
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySchemeSettings( IScheme *scheme )
{
	// We need to re-hook ourselves up to the TF client scheme, because the GameUI will try to change us their its scheme
	vgui::HScheme pScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(pScheme);
	SetProportional( true );

	m_bBackgroundUsesCharacterImages = true;
	m_pszForcedCharacterImage = NULL;

	bool bHolidayActive = false;
	KeyValues *pConditions = NULL;
	const char *pszHoliday = UTIL_GetActiveHolidayString();


	if ( pszHoliday && pszHoliday[0] )
	{
		pConditions = new KeyValues( "conditions" );

		char szCondition[64];
		Q_snprintf( szCondition, sizeof( szCondition ), "if_%s", pszHoliday );
		AddSubKeyNamed( pConditions, szCondition );

		if ( FStrEq( pszHoliday, "halloween" ) )
		{

			// for Halloween we also want to pick a random background
			int nBackground = RandomInt( 0, 5 );

			AddSubKeyNamed( pConditions, CFmtStr( "if_halloween_%d", nBackground ) );
			if ( ( nBackground == 3 ) || ( nBackground == 4 ) )
			{
				m_bBackgroundUsesCharacterImages = false;
			}
		}
		else if ( FStrEq( pszHoliday, "christmas" ) )
		{
			// for Christmas we also want to pick a random background
			int nBackground = RandomInt( 0, 1 );
			AddSubKeyNamed( pConditions, CFmtStr( "if_christmas_%d", nBackground ) );
		}

		bHolidayActive = true;
	}

	if ( !pConditions )
	{
		pConditions = new KeyValues( "conditions" );
	}

	// Put in ratio condition
	float aspectRatio = engine->GetScreenAspectRatio();
	AddSubKeyNamed( pConditions, aspectRatio >= 1.6 ? "if_wider" : "if_taller" );

	RemoveAllMenuEntries();

	LoadControlSettings( "resource/UI/MainMenuOverride.res", NULL, NULL, pConditions );

	BaseClass::ApplySchemeSettings( vgui::scheme()->GetIScheme(pScheme) );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pQuitButton = dynamic_cast<CExButton*>( FindChildByName("QuitButton") );
	m_pDisconnectButton = dynamic_cast<CExButton*>( FindChildByName("DisconnectButton") );
	m_pBackToReplaysButton = dynamic_cast<CExButton*>( FindChildByName("BackToReplaysButton") );
	m_pStoreHasNewItemsImage = dynamic_cast<ImagePanel*>( FindChildByName( "StoreHasNewItemsImage", true ) );
	m_pStoreButton = dynamic_cast<CExButton*>(FindChildByName("GeneralStoreButton"));
	if (m_pStoreButton)
	{
		m_pStoreButton->SetVisible(false);
	}

	m_pWatchStreamButton = dynamic_cast<EditablePanel*>(FindChildByName("WatchStreamButton"));
	if (m_pWatchStreamButton)
	{
		m_pWatchStreamButton->SetVisible(false);
		m_pWatchStreamButton->SetEnabled(false);
	}

	m_pQuestLogButton = dynamic_cast<EditablePanel*>(FindChildByName("QuestLogButton"));
	if (m_pQuestLogButton)
	{
		m_pQuestLogButton->SetVisible(false);
		m_pQuestLogButton->SetEnabled(false);
	}

	{
		Panel *pButton = FindChildByName( "VRModeButton" );
		if( pButton )
		{
			m_pVRModeButton = dynamic_cast< CExButton *>( pButton->GetChild( 0 ) );
		}
	}
	m_pVRModeBackground = FindChildByName( "VRBGPanel" );

	bool bShowVR = materials->GetCurrentConfigForVideoCard().m_nVRModeAdapter == materials->GetCurrentAdapter();
	if ( m_pVRModeBackground )
	{
		m_pVRModeBackground->SetVisible( bShowVR );
	}

	m_bIsDisconnectText = true;

	// Tell all the MOTD buttons that we want their messages
	m_pMOTDPrevButton = dynamic_cast<CExImageButton*>( m_pMOTDPanel->FindChildByName("MOTD_PrevButton") );
	m_pMOTDNextButton = dynamic_cast<CExImageButton*>( m_pMOTDPanel->FindChildByName("MOTD_NextButton") );
	m_pMOTDURLButton = dynamic_cast<CExButton*>( m_pMOTDPanel->FindChildByName("MOTD_URLButton") );

	// m_pNotificationsShowPanel shows number of unread notifications. Pressing it pops up the first notification.
	m_pNotificationsShowPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("Notifications_ShowButtonPanel") );

	m_pNotificationsShowPanel->SetVisible(false);

	m_iNotiPanelWide = m_pNotificationsPanel->GetWide();

	// m_pMOTDShowPanel shows that the player has an unread MOTD. Pressing it pops up the MOTD.
	m_pMOTDShowPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("MOTD_ShowButtonPanel") );
	m_pMOTDShowPanel->SetVisible(false);

	vgui::EditablePanel* pHeaderContainer = dynamic_cast<vgui::EditablePanel*>( m_pMOTDPanel->FindChildByName( "MOTD_HeaderContainer" ) );
	if ( pHeaderContainer )
	{
		m_pMOTDHeaderLabel = dynamic_cast<vgui::Label*>( pHeaderContainer->FindChildByName( "MOTD_HeaderLabel" ) );
	}

	m_pMOTDHeaderIcon = dynamic_cast<vgui::ImagePanel*>( m_pMOTDPanel->FindChildByName("MOTD_HeaderIcon") );

	m_pMOTDTitleLabel = dynamic_cast<vgui::Label*>( m_pMOTDPanel->FindChildByName("MOTD_TitleLabel") );
	if ( m_pMOTDTitleLabel )
	{
		m_hTitleLabelFont = m_pMOTDTitleLabel->GetFont();
	}

	m_pMOTDTextLabel = dynamic_cast<vgui::Label*>( m_pMOTDTextPanel->FindChildByName( "MOTD_TextLabel" ) );

	m_pMOTDTitleImageContainer = dynamic_cast<vgui::EditablePanel*>( m_pMOTDPanel->FindChildByName("MOTD_TitleImageContainer") );
	if ( m_pMOTDTitleImageContainer )
	{
		m_pMOTDTitleImage = dynamic_cast<vgui::ImagePanel*>( m_pMOTDTitleImageContainer->FindChildByName("MOTD_TitleImage") );
	}

	m_pNotificationsScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pNotificationsScroller->GetScrollbar()->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->SetPaintBackgroundEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBackgroundEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBackgroundEnabled( false );

	// Add tooltips for various buttons
	auto lambdaAddTooltip = [&]( const char* pszPanelName, const char* pszTooltipText )
	{
		Panel* pPanelToAddTooltipTipTo = FindChildByName( pszPanelName );
		if ( pPanelToAddTooltipTipTo)
		{
			pPanelToAddTooltipTipTo->SetTooltip( m_pToolTip, pszTooltipText );

			pPanelToAddTooltipTipTo->SetVisible(false);
		}
	};

	lambdaAddTooltip( "CommentaryButton", "#MMenu_Tooltip_Commentary" );
	lambdaAddTooltip( "CoachPlayersButton", "#MMenu_Tooltip_Coach" );
	lambdaAddTooltip( "ReportBugButton", "#MMenu_Tooltip_ReportBug" );
	lambdaAddTooltip( "AchievementsButton", "#MMenu_Tooltip_Achievements" );
	lambdaAddTooltip( "NewUserForumsButton", "#MMenu_Tooltip_NewUserForum" );
	lambdaAddTooltip( "ReplayButton", "#MMenu_Tooltip_Replay" );
	lambdaAddTooltip( "WorkshopButton", "#MMenu_Tooltip_Workshop" );
	lambdaAddTooltip( "SettingsButton", "#MMenu_Tooltip_Options" );
	lambdaAddTooltip( "TF2SettingsButton", "#MMenu_Tooltip_AdvOptions" );


	LoadCharacterImageFile();

	RemoveAllMenuEntries();
	LoadMenuEntries();

	UpdateNotifications();
	UpdatePromotionalCodes();

	ScheduleTrainingCheck( false );

	PerformKeyRebindings();

	GetMMDashboard();
	GetCompRanksTooltip();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadCharacterImageFile( void )
{
	m_pCharacterImagePanel->SetVisible( m_bBackgroundUsesCharacterImages );

	if ( !m_bBackgroundUsesCharacterImages )
	{
		return;
	}

	// If we've got a forced image, use that
	if ( m_pszForcedCharacterImage && *m_pszForcedCharacterImage )
	{
		m_pCharacterImagePanel->SetImage( m_pszForcedCharacterImage );
		return;
	}

	KeyValues *pCharacterFile = new KeyValues( "CharacterBackgrounds" );

	if ( pCharacterFile->LoadFromFile( g_pFullFileSystem, "scripts/CharacterBackgrounds.txt" ) )
	{
		CUtlVector<KeyValues *> vecUseableCharacters;

		const char* pszActiveWarName = NULL;
		const WarDefinitionMap_t& mapWars = GetItemSchema()->GetWarDefinitions();
		FOR_EACH_MAP_FAST( mapWars, i )
		{
			const CWarDefinition* pWarDef = mapWars[i];
			if ( pWarDef->IsActive() )
			{
				pszActiveWarName = pWarDef->GetDefName();
				break;
			}
		}

		bool bActiveOperation = false;

		// Uncomment if another operation happens
		//FOR_EACH_MAP_FAST( GetItemSchema()->GetOperationDefinitions(), iOperation )
		//{
		//	CEconOperationDefinition *pOperation = GetItemSchema()->GetOperationDefinitions()[iOperation];
		//	if ( !pOperation || !pOperation->IsActive() || !pOperation->IsCampaign() )
		//		continue;

		//	bActiveOperation = true;
		//	break;
		//}

		// Count the number of possible characters.
		FOR_EACH_SUBKEY( pCharacterFile, pCharacter )
		{
			bool bIsOperationCharacter = bActiveOperation && pCharacter->GetBool( "operation", false );

			EHoliday eHoliday = (EHoliday)UTIL_GetHolidayForString( pCharacter->GetString( "holiday_restriction" ) );


			const char* pszAssociatedWar = pCharacter->GetString( "war_restriction" );

			int iWeight = pCharacter->GetInt( "weight", 1 );

			// If a War is active, that's all we want to show.  If not, then bias towards holidays
			if ( pszActiveWarName != NULL )
			{
				if ( !FStrEq( pszAssociatedWar, pszActiveWarName ) )
				{
					iWeight = 0;
				}
			}
			else if ( eHoliday != kHoliday_None )
			{
				iWeight = UTIL_IsHolidayActive( eHoliday ) ? MAX( iWeight, 6 ) : 0;
			}
			else if ( bActiveOperation && !bIsOperationCharacter )
			{
				iWeight = 0;
			}
			else
			{
				// special cases for summer, halloween, fullmoon, and christmas...turn off anything not covered above
				if ( UTIL_IsHolidayActive( kHoliday_Summer ) || UTIL_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) || UTIL_IsHolidayActive( kHoliday_Christmas ) )
				{
					iWeight = 0;
				}
			}

			for ( int i = 0; i < iWeight; i++ )
			{
				vecUseableCharacters.AddToTail( pCharacter );
			}
		}

		// Pick a character at random.
		if ( vecUseableCharacters.Count() > 0 )
		{
			m_iCharacterImageIdx = rand() % vecUseableCharacters.Count();
		}

		// Make sure we found a character we can use.
		if ( vecUseableCharacters.IsValidIndex( m_iCharacterImageIdx ) )
		{
			KeyValues *pCharacter = vecUseableCharacters[m_iCharacterImageIdx];

			if ( IsFreeTrialAccount( ) && GetQuestMapPanel()->IsVisible() )
			{
				const char* text = pCharacter->GetString( "store_text" );
				if ( text )
				{
					StartHighlightAnimation( MMHA_STORE )->SetDialogVariable( "highlighttext", g_pVGuiLocalize->Find( text ) );
				}
			}

			const char* image_name = pCharacter->GetString( "image" );
			m_pCharacterImagePanel->SetImage( image_name );
		}
	}

	pCharacterFile->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadMenuEntries( void )
{
	KeyValues *datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences( true );	// VGUI uses escape sequences
	bool bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res", "custom_mod" );
	if ( !bLoaded )
	{
		bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res", "vgui" );
		if ( !bLoaded )
		{
			// only allow to load loose files when using insecure mode
			if ( CommandLine()->FindParm( "-insecure" ) )
			{
				bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res" );
			}
		}
	}

	for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char *label = dat->GetString("label", "<unknown>");
		const char *cmd = dat->GetString("command", NULL);
		const char *name = dat->GetName();
		int iStyle = dat->GetInt("style", 0 );

		if ( !cmd || !cmd[0] )
		{
			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = NULL;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool( "OnlyInGame" );
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool( "OnlyInReplay" );
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool( "OnlyAtMenu" );
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool( "OnlyWhenVREnabled" );
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			continue;
		}

		// Create the new editable panel (first, see if we have one already)
		vgui::EditablePanel *pPanel = dynamic_cast<vgui::EditablePanel *>( FindChildByName( name, true ) );
		if ( !pPanel )
		{
			Assert( false );	// We don't want to do this anymore.  We need an actual hierarchy so things can slide
								// around when the play buttin is pressed and the play options expand
			pPanel = new vgui::EditablePanel( this, name );
		}
		else
		{
			// It already exists in our .res file. Note that it's a custom button.
			iStyle = MMBS_CUSTOM;
		}

		if ( pPanel )
		{
			if ( m_pButtonKV && iStyle != MMBS_CUSTOM )
			{
				pPanel->ApplySettings( m_pButtonKV );
			}

			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = pPanel;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool( "OnlyInGame" );
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool( "OnlyInReplay" );
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool( "OnlyAtMenu" );
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool( "OnlyWhenVREnabled" );
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			m_pMMButtonEntries[iIdx].pszImage = dat->GetString( "subimage" );
			m_pMMButtonEntries[iIdx].pszTooltip = dat->GetString( "tooltip", NULL );

			// Tell the button that we'd like messages from it
			CExImageButton *pButton = dynamic_cast<CExImageButton*>( pPanel->FindChildByName("SubButton") );
			if ( pButton )
			{
				if ( m_pMMButtonEntries[iIdx].pszTooltip )
				{
					pButton->SetTooltip( m_pToolTip, m_pMMButtonEntries[iIdx].pszTooltip );
				}

				pButton->SetText( label );
				pButton->SetCommand( cmd );
				pButton->SetMouseInputEnabled( true );
				pButton->AddActionSignalTarget( GetVPanel() );

				if ( m_pMMButtonEntries[iIdx].pszImage && m_pMMButtonEntries[iIdx].pszImage[0] )
				{
					pButton->SetSubImage( m_pMMButtonEntries[iIdx].pszImage );
				}
			}
		}

		OnUpdateMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::RemoveAllMenuEntries( void )
{
	FOR_EACH_VEC_BACK( m_pMMButtonEntries, i )
	{
		if ( m_pMMButtonEntries[i].pPanel )
		{
			// Manually remove anything that's not going to be removed automatically
			if ( m_pMMButtonEntries[i].pPanel->IsBuildModeDeletable() == false )
			{
				m_pMMButtonEntries[i].pPanel->MarkForDeletion();
			}
		}
	}
	m_pMMButtonEntries.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PerformLayout( void )
{
	BaseClass::PerformLayout();

	bool bFirstButton = true;

	int iYPos = m_iButtonY;
	FOR_EACH_VEC( m_pMMButtonEntries, i )
	{
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if ( !bIsVisible )
			continue;

		if ( bFirstButton && m_pMMButtonEntries[i].pPanel != NULL )
		{
			m_pMMButtonEntries[i].pPanel->NavigateTo();
			bFirstButton = false;
		}

		// Don't reposition it if it's a custom button
		if ( m_pMMButtonEntries[i].iStyle == MMBS_CUSTOM )
			continue;

		// If we're a spacer, just leave a blank and move on
		if ( m_pMMButtonEntries[i].pPanel == NULL )
		{
			iYPos += YRES(20);
			continue;
		}

		m_pMMButtonEntries[i].pPanel->SetPos( (GetWide() * 0.5) + m_iButtonXOffset, iYPos );
		iYPos += m_pMMButtonEntries[i].pPanel->GetTall() + m_iButtonYDelta;
	}

	if ( m_pEventPromoContainer && m_pSafeModeContainer )
	{
		m_pEventPromoContainer->SetVisible( !cl_mainmenu_safemode.GetBool() );
		m_pSafeModeContainer->SetVisible( cl_mainmenu_safemode.GetBool() );
		if ( cl_mainmenu_safemode.GetBool() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pSafeModeContainer, "MMenu_SafeMode_Blink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( m_pSafeModeContainer );
		}
	}

	// Make the glows behind the update buttons pulse
	if ( m_pEventPromoContainer && cl_mainmenu_updateglow.GetInt() )
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >( "Background", true );
		if ( pUpdateBackground )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pUpdateBackground, "MMenu_UpdateButton_StartGlow" );
		}
	}

	m_pEventPromoContainer->SetVisible(false);

	UpdateRankPanelVisibility();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnUpdateMenu( void )
{
	// The dumb gameui.dll basepanel calls this every frame it's visible.
	// So try and do the least amount of work if nothing has changed.

	bool bSomethingChanged = false;
	bool bInGame = engine->IsInGame();
#if defined( REPLAY_ENABLED )
	bool bInReplay = g_pEngineClientReplay->IsPlayingReplayDemo();
#else
	bool bInReplay = false;
#endif
	bool bIsVREnabled = materials->GetCurrentConfigForVideoCard().m_nVRModeAdapter == materials->GetCurrentAdapter();

	// First, reapply any KVs we have to reapply
	if ( m_bReapplyButtonKVs )
	{
		m_bReapplyButtonKVs = false;

		if ( m_pButtonKV )
		{
			FOR_EACH_VEC( m_pMMButtonEntries, i )
			{
				if ( m_pMMButtonEntries[i].iStyle != MMBS_CUSTOM && m_pMMButtonEntries[i].pPanel )
				{
					m_pMMButtonEntries[i].pPanel->ApplySettings( m_pButtonKV );
				}
			}
		}
	}

	// Hide the character if we're in game.
	if ( bInGame || bInReplay )
	{
		if ( m_pCharacterImagePanel->IsVisible() )
		{
			m_pCharacterImagePanel->SetVisible( false );
		}
	}
	else if ( !bInGame && !bInReplay )
	{
		if ( !m_pCharacterImagePanel->IsVisible() )
		{
			m_pCharacterImagePanel->SetVisible( m_bBackgroundUsesCharacterImages );
		}
	}

	// Position the entries
	FOR_EACH_VEC( m_pMMButtonEntries, i )
	{
		bool shouldBeVisible = true;
		if ( m_pMMButtonEntries[i].bOnlyInGame && !bInGame )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyInReplay && !bInReplay )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyAtMenu && (bInGame || bInReplay) )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyVREnabled && ( !bIsVREnabled || ShouldForceVRActive() ) )
		{
			shouldBeVisible = false;
		}

		// Set the right visibility
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if ( bIsVisible != shouldBeVisible )
		{
			m_pMMButtonEntries[i].bIsVisible = shouldBeVisible;
			if ( m_pMMButtonEntries[i].pPanel )
			{
				m_pMMButtonEntries[i].pPanel->SetVisible( shouldBeVisible );
			}
			bSomethingChanged = true;
		}
	}

	if ( m_pQuitButton && m_pDisconnectButton && m_pBackToReplaysButton )
	{
		bool bShowQuit = !( bInGame || bInReplay );
		bool bShowDisconnect = bInGame && !bInReplay;

		if ( m_pQuitButton->IsVisible() != bShowQuit )
		{
			m_pQuitButton->SetVisible( bShowQuit );
		}

		if ( m_pBackToReplaysButton->IsVisible() != bInReplay )
		{
			m_pBackToReplaysButton->SetVisible( bInReplay );
		}

		if ( m_pDisconnectButton->IsVisible() != bShowDisconnect )
		{
			m_pDisconnectButton->SetVisible( bShowDisconnect );
		}

		if ( bShowDisconnect )
		{
			bool bIsDisconnectText = GTFGCClientSystem()->GetCurrentServerAbandonStatus() != k_EAbandonGameStatus_AbandonWithPenalty;
			if ( m_bIsDisconnectText != bIsDisconnectText )
			{
				m_bIsDisconnectText = bIsDisconnectText;
				m_pDisconnectButton->SetText( m_bIsDisconnectText ? "#GameUI_GameMenu_Disconnect" : "#TF_MM_Rejoin_Abandon" );
			}
		}
	}

	if ( m_pBackground )
	{
		if ( cl_mainmenu_operation_motd_reset.GetBool() && cl_mainmenu_operation_motd_start.GetBool() )
		{
			cl_mainmenu_operation_motd_start.SetValue( 0 );
			cl_mainmenu_operation_motd_reset.SetValue( 0 );
		}

		if ( !cl_mainmenu_operation_motd_start.GetInt() )
		{
			char sztime[k_RTimeRenderBufferSize];
			CRTime::RTime32ToString( CRTime::RTime32TimeCur(), sztime );
			cl_mainmenu_operation_motd_start.SetValue( sztime );
		}

		bool bShouldBeVisible = bInGame == false;
		if ( m_pBackground->IsVisible() != bShouldBeVisible )
		{
			m_pBackground->SetVisible( bShouldBeVisible );

			// Always show this on startup when we have a new campaign
			if ( m_bStabilizedInitialLayout && bShouldBeVisible && ( m_bHaveNewMOTDs || !m_bMOTDShownAtStartup ) )
			{
				RTime32 rtFirstLaunchTime = CRTime::RTime32FromString( cl_mainmenu_operation_motd_start.GetString() );
				RTime32 rtThreeDaysFromStart = CRTime::RTime32DateAdd( rtFirstLaunchTime, 7, k_ETimeUnitDay );
				if ( m_bHaveNewMOTDs || CRTime::RTime32TimeCur() < rtThreeDaysFromStart )
				{
					SetMOTDVisible( true );
					m_bMOTDShownAtStartup = true;
				}
			}
		}
	}

	if ( bSomethingChanged )
	{
		InvalidateLayout();

		ScheduleItemCheck();
	}

	if ( !bInGame && m_flCheckTrainingAt && m_flCheckTrainingAt < engine->Time() )
	{
		m_flCheckTrainingAt = 0;
		CheckTrainingStatus();
	}

	if ( !bInGame && m_flCheckUnclaimedItems && m_flCheckUnclaimedItems < engine->Time() )
	{
		m_flCheckUnclaimedItems = 0;
		CheckUnclaimedItems();
	}


	if ( m_pVRModeButton && m_pVRModeButton->IsVisible() )
	{
		if( UseVR() )
			m_pVRModeButton->SetText( "#MMenu_VRMode_Deactivate" );
		else
			m_pVRModeButton->SetText( "#MMenu_VRMode_Activate" );
	}

	if ( !IsLayoutInvalid() )
	{
		if ( !m_bStabilizedInitialLayout )
		{
			PostMessage( this, new KeyValues( "MainMenuStabilized" ), 2.f );
		}

		m_bStabilizedInitialLayout = true;
	}
}

void CHudMainMenuOverride::OnMainMenuStabilized()
{
	IGameEvent *event = gameeventmanager->CreateEvent( "mainmenu_stabilized" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we need to hound the player about unclaimed items.
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::CheckUnclaimedItems()
{
	// Only do this if we don't have a notification about unclaimed items already.
	for ( int i=0; i<NotificationQueue_GetNumNotifications(); i++ )
	{
		CEconNotification* pNotification = NotificationQueue_Get( i );
		if ( pNotification )
		{
			if ( !Q_strcmp( pNotification->GetUnlocalizedText(), "TF_HasNewItems") )
			{
				return;
			}
		}
	}

	// Only provide a notification if there are items to pick up.
	if ( TFInventoryManager()->GetNumItemPickedUpItems() == 0 )
		return;

	TFInventoryManager()->GetLocalTFInventory()->NotifyHasNewItems();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnConfirm( KeyValues *pParams )
{
	if ( pParams->GetBool( "confirmed" ) )
	{
		engine->ClientCmd_Unrestricted( "disconnect" );
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine training_showdlg" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateMOTD( bool bNewMOTDs )
{
	return;

	if ( m_bInitMOTD == false )
	{
		m_pMOTDPanel->InvalidateLayout( true, true );
		m_bInitMOTD = true;
	}

	if ( bNewMOTDs )
	{
		m_bHaveNewMOTDs = true;
		m_iCurrentMOTD = -1;
	}

	int iCount = GetMOTDManager().GetNumMOTDs();
	if ( !iCount || m_iCurrentMOTD < 0 )
	{
		m_iCurrentMOTD = (iCount-1);
	}

	// If we don't have an MOTD selected, show the most recent one
	CMOTDEntryDefinition *pMOTD = GetMOTDManager().GetMOTDByIndex( m_iCurrentMOTD );
	if ( pMOTD )
	{
		char uilanguage[ 64 ];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
		ELanguage nCurLang = PchLanguageToELanguage( uilanguage );

		RTime32 nTime = pMOTD->GetPostTime();
		wchar_t wzDate[64];

		char rgchDateBuf[ 128 ];
		BGetLocalFormattedDate( nTime, rgchDateBuf, sizeof( rgchDateBuf ) );

		// Start with the day ("Aug 21")
		CRTime cTime( nTime );
		g_pVGuiLocalize->ConvertANSIToUnicode( rgchDateBuf, wzDate, sizeof( wzDate ) );

		m_pMOTDPanel->SetDialogVariable( "motddate", wzDate );

		// Header Color and text
		if ( m_pMOTDHeaderLabel )
		{
			m_pMOTDHeaderLabel->SetText( pMOTD->GetHeaderTitle(nCurLang) );
			int iHeaderType = pMOTD->GetHeaderType();
			switch ( iHeaderType )
			{
			case 0:
				m_pMOTDHeaderLabel->SetBgColor( Color ( 183, 108, 58, 255 ) );
				break;
			case 1:
				m_pMOTDHeaderLabel->SetBgColor( Color ( 141, 178, 61, 255 ) );
				break;
			default:
				m_pMOTDHeaderLabel->SetBgColor( Color ( 183, 108, 58, 255 ) );
				break;
			}
		}

		if ( m_pMOTDHeaderIcon )
		{
			// Header Class icon
			if (  pMOTD->GetHeaderIcon() == NULL || Q_strcmp( pMOTD->GetHeaderIcon(), "" ) == 0)
			{
				m_pMOTDHeaderIcon->SetVisible(false);
			}
			else
			{
				m_pMOTDHeaderIcon->SetVisible(true);
				m_pMOTDHeaderIcon->SetImage( pMOTD->GetHeaderIcon() );
			}
		}

		// Set the Title and change font until it fits
		// title
		int iTitleWide = 0;
		int iTitleTall = 0;

		int iLabelWide = 0;
		int iLabelTall = 0;

		wchar_t wszText[512];
		g_pVGuiLocalize->ConvertANSIToUnicode( pMOTD->GetTitle(nCurLang), wszText, sizeof( wszText ) );

		if ( m_hTitleLabelFont != vgui::INVALID_FONT )
		{
			surface()->GetTextSize( m_hTitleLabelFont, wszText, iTitleWide, iTitleTall );
		}

		if ( m_pMOTDTitleLabel )
		{
			m_pMOTDTitleLabel->GetSize( iLabelWide, iLabelTall );

			if ( iTitleWide > iLabelWide )
			{
				IScheme *pScheme = scheme()->GetIScheme( m_pMOTDTitleLabel->GetScheme() );

				int hMediumBoldFont = pScheme->GetFont( "HudFontMediumBold" );
				surface()->GetTextSize( hMediumBoldFont, wszText, iTitleWide, iTitleTall );
				if ( iTitleWide > iLabelWide )
				{
					m_pMOTDTitleLabel->SetFont( pScheme->GetFont( "HudFontMediumSmallBold" ) );
				}
				else
				{
					m_pMOTDTitleLabel->SetFont( hMediumBoldFont );
				}
			}
			else
			{
				if ( m_hTitleLabelFont != vgui::INVALID_FONT )
				{
					m_pMOTDTitleLabel->SetFont( m_hTitleLabelFont );
				}
			}
		}
		m_pMOTDPanel->SetDialogVariable( "motdtitle", pMOTD->GetTitle(nCurLang) );

		// Body Text
		m_pMOTDTextPanel->SetDialogVariable( "motdtext", pMOTD->GetText(nCurLang) );

		// Image
		const char* pszImage = pMOTD->GetImage();

		if ( m_pMOTDTitleImage )
		{
			m_pMOTDTitleImage->SetShouldScaleImage( false );
			if ( pszImage == NULL || Q_strcmp( pszImage, "" ) == 0 || Q_strcmp( pszImage, "class_icons/filter_all_on") == 0 )
			{
				m_pMOTDTitleImage->SetImage( "../logo/new_tf2_logo" );
			}
			else
			{
				m_pMOTDTitleImage->SetImage( pszImage );
			}

			IImage *pImage = m_pMOTDTitleImage->GetImage();
			int iContentWide = 0;
			int iContentTall = 0;
			if ( m_pMOTDTitleImageContainer )
			{
				m_pMOTDTitleImageContainer->GetSize( iContentWide, iContentTall );
			}

			int iImgWide;
			int iImgTall;
			pImage->GetSize( iImgWide, iImgTall );

			// get the size of the content
			// perform a uniform scale along the horizontal
			float fImageScale = MIN( (float)iContentWide / (float)iImgWide, 1.0f );
			float fScaledTall = iImgTall * fImageScale;
			float fScaledWide = iImgWide * fImageScale;
			pImage->SetSize( fScaledWide, fScaledTall );

			// reposition the image so that its centered
			m_pMOTDTitleImage->SetPos( (iContentWide - fScaledWide) / 2, (iContentTall - fScaledTall) / 2 );
		}

		// We need to resize our text label to fit all the text
		if ( m_pMOTDTextLabel )
		{
			m_pMOTDTextLabel->InvalidateLayout( true );

			int wide, tall;
			m_pMOTDTextLabel->GetContentSize(wide, tall);
			m_pMOTDTextLabel->SetSize( m_pMOTDTextPanel->GetWide(), tall );
			m_pMOTDTextPanel->SetSize( m_pMOTDTextPanel->GetWide(), m_pMOTDTextLabel->GetTall() );
		}

		if ( m_pMOTDURLButton )
		{
			const char *pszURL = pMOTD->GetURL();
			m_pMOTDURLButton->SetVisible( (pszURL && pszURL[0]) );
		}
		if ( m_pMOTDPrevButton )
		{
			m_pMOTDPrevButton->SetEnabled( m_iCurrentMOTD > 0 );
			m_pMOTDPrevButton->SetSubImage( m_iCurrentMOTD > 0 ? "blog_back" : "blog_back_disabled" );
		}
		if ( m_pMOTDNextButton )
		{
			m_pMOTDNextButton->SetEnabled( m_iCurrentMOTD < (iCount-1) );
			m_pMOTDNextButton->SetSubImage( m_iCurrentMOTD < (iCount-1) ? "blog_forward" : "blog_forward_disabled" );
		}

		// Move our scrollbar to the top.
		m_pMOTDTextScroller->InvalidateLayout();
		m_pMOTDTextScroller->Repaint();
		m_pMOTDTextScroller->GetScrollbar()->SetValue( 0 );
		m_pMOTDTextScroller->GetScrollbar()->SetVisible( m_pMOTDTextPanel->GetTall() > m_pMOTDTextScroller->GetScrollbar()->GetTall() );
		m_pMOTDTextScroller->GetScrollbar()->InvalidateLayout();
		m_pMOTDTextScroller->GetScrollbar()->Repaint();
	}
	else
	{
		// Hide the MOTD, and the button to show it.
		SetMOTDVisible( false );

		if ( m_pMOTDShowPanel )
		{
			m_pMOTDShowPanel->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetMOTDButtonVisible( bool bVisible )
{
	if (m_pMOTDShowPanel)
	{
		m_pMOTDShowPanel->SetVisible(false);
	}
	if (m_pMOTDPanel)
	{
		m_pMOTDPanel->SetVisible(false);
	}
	return;

	if ( bVisible && m_pMOTDPanel && m_pMOTDPanel->IsVisible() )
		return;

	if ( m_pMOTDShowPanel )
	{
		// Show the notifications show panel button if we have new notifications.
		m_pMOTDShowPanel->SetVisible( bVisible );

		if ( bVisible && m_bHaveNewMOTDs )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pMOTDShowPanel, "HasMOTDBlink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pMOTDShowPanel, "HasMOTDBlinkStop" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetMOTDVisible( bool bVisible )
{
	m_pMOTDPanel->SetVisible( bVisible );

	if ( bVisible )
	{
		// Ensure the text is correct.
		UpdateMOTD( false );

		// Clear MOTD button.
		SetMOTDButtonVisible( true );
		SetNotificationsPanelVisible( false );
		//SetWatchStreamVisible( false );
		//SetNotificationsButtonVisible( false );

		// Consider new MOTDs as having been viewed.
		m_bHaveNewMOTDs = false;
	}
	else
	{
		SetMOTDButtonVisible( true );
		UpdateNotifications();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetQuestMapVisible( bool bVisible )
{
	return;

	if ( bVisible )
	{
		GetQuestMapPanel()->InvalidateLayout( true );
		SetMOTDVisible( false );
		SetNotificationsPanelVisible( false );
		//SetWatchStreamVisible( false );
	}

	GetQuestMapPanel()->SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
//void CHudMainMenuOverride::SetWatchStreamVisible( bool bVisible )
//{
//	m_pWatchStreamsPanel->SetVisible( bVisible );
//
//	if ( bVisible )
//	{
//		SetMOTDVisible( false );
//		SetNotificationsPanelVisible( false );
//	}
//}

bool CHudMainMenuOverride::CheckAndWarnForPREC( void )
{
	enum check_state
	{
		INVALID,
		FOUND,
		NOT_FOUND,
	};

	static check_state s_state = INVALID;
	if ( s_state == INVALID )
	{
		s_state = NOT_FOUND;

		ICvar::Iterator iter( g_pCVar );
		for ( iter.SetFirst() ; iter.IsValid() ; iter.Next() )
		{
			ConCommandBase *cmd = iter.Get();
			if ( cmd )
			{
				if ( !Q_strncmp( cmd->GetName(), "prec_", 5 ) )
				{
					s_state = FOUND;
					break;
				}
			}
		}
	}

	if ( s_state == FOUND )
	{
		ShowMessageBox( "#TF_Incompatible_AddOn", "#TF_PREC_Loaded" );
	}

	return ( s_state == FOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateNotifications()
{
	return;

	int iNumNotifications = NotificationQueue_GetNumMainMenuNotifications();

	wchar_t wszNumber[16]=L"";
	V_swprintf_safe( wszNumber, L"%i", iNumNotifications );
	wchar_t wszText[1024]=L"";
	g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "#MMenu_Notifications_Show" ), 1, wszNumber );

	m_pNotificationsPanel->SetDialogVariable( "notititle", wszText );

	bool bHasNotifications = iNumNotifications != 0;

	if ( m_pNotificationsShowPanel )
	{
		SetNotificationsButtonVisible( bHasNotifications );

		bool bBlinkNotifications = bHasNotifications && m_pNotificationsShowPanel->IsVisible();
		if ( bBlinkNotifications )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pNotificationsShowPanel, "HasNotificationsBlink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pNotificationsShowPanel, "HasNotificationsBlinkStop" );
		}
	}

	if ( !bHasNotifications )
	{
		SetNotificationsButtonVisible( false );
		SetNotificationsPanelVisible( false );
	}

	AdjustNotificationsPanelHeight();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsButtonVisible( bool bVisible )
{
	return;

	if ( bVisible && ( m_pNotificationsPanel && m_pNotificationsPanel->IsVisible() ) )
		return;

	if ( m_pNotificationsShowPanel )
	{
		// Show the notifications show panel button if we have new notifications.
		m_pNotificationsShowPanel->SetVisible( bVisible );

		// Set the notification count variable.
		if ( m_pNotificationsShowPanel )
		{
			m_pNotificationsShowPanel->SetDialogVariable( "noticount", NotificationQueue_GetNumMainMenuNotifications() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsPanelVisible( bool bVisible )
{
	return;

	if ( m_pNotificationsPanel )
	{
		bool bHasNotifications = NotificationQueue_GetNumMainMenuNotifications() != 0;

		if ( bHasNotifications )
		{
			UpdateNotifications();
		}

		m_pNotificationsPanel->SetVisible( bVisible );

		if ( bVisible )
		{
			m_pNotificationsScroller->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->SetValue( 0 );

			SetMOTDVisible( false );
			SetQuestMapVisible( false );
			//SetWatchStreamVisible( false );

			m_pNotificationsShowPanel->SetVisible( false );

			m_pNotificationsControl->OnTick();
			m_pNotificationsControl->PerformLayout();
			AdjustNotificationsPanelHeight();

			// Faster updating while open.
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
			vgui::ivgui()->AddTickSignal( GetVPanel(), 5 );
		}
		else
		{
			// Clear all notifications.
			if ( bHasNotifications )
			{
				SetNotificationsButtonVisible( true );
			}

			// Slower updating while closed.
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
			vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AdjustNotificationsPanelHeight()
{
	return;

	// Fit to our contents, which may change without notifying us.
	int iNotiTall = m_pNotificationsControl->GetTall();
	if ( iNotiTall > m_pNotificationsScroller->GetTall() )
		iNotiTall = m_pNotificationsScroller->GetTall();
	int iTargetTall = YRES(40) + iNotiTall;
	if ( m_pNotificationsPanel->GetTall() != iTargetTall )
		m_pNotificationsPanel->SetTall( iTargetTall );

	// Adjust visibility of the slider buttons and our width, as contents change.
	if ( m_pNotificationsScroller )
	{
		if ( m_pNotificationsScroller->GetScrollbar()->GetSlider() &&
			m_pNotificationsScroller->GetScrollbar()->GetSlider()->IsSliderVisible() )
		{
			m_pNotificationsPanel->SetWide( m_iNotiPanelWide +  m_pNotificationsScroller->GetScrollbar()->GetSlider()->GetWide() );
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible( true );
		}
		else
		{
			m_pNotificationsPanel->SetWide( m_iNotiPanelWide );
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdatePromotionalCodes( void )
{
	// should we show the promo codes button?
	vgui::Panel *pPromoCodesButton = FindChildByName( "ShowPromoCodesButton" );
	if ( pPromoCodesButton )
	{
		bool bShouldBeVisible = false;
		if ( steamapicontext && steamapicontext->SteamUser() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
			if ( pSOCache )
			{
				GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( k_EEconTypeClaimCode );
				bShouldBeVisible = pTypeCache != NULL && pTypeCache->GetCount() != 0;
			}
		}

		// The promo code button collides with the VR mode button. Turn off the promo code button
		// in that case since the people who deliberately enabled VR are much more likely to want that
		// than to claim their Well Spun Hat in Rift.
		bool bShowVR = materials->GetCurrentConfigForVideoCard().m_nVRModeAdapter == materials->GetCurrentAdapter();
		if( bShowVR )
		{
			bShouldBeVisible = false;
		}

		// has the player turned off this button?
		if ( !cl_promotional_codes_button_show.GetBool() )
		{
			bShouldBeVisible = false;
		}

		if ( pPromoCodesButton->IsVisible() != bShouldBeVisible )
		{
			pPromoCodesButton->SetVisible( bShouldBeVisible );
		}

		if ( m_pVRModeBackground )
		{
			m_pVRModeBackground->SetVisible( bShouldBeVisible || bShowVR );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudMainMenuOverride::IsVisible( void )
{
	/*
	// Only draw whenever the main menu is visible
	if ( GetClientModeTFNormal()->GameUI() && steamapicontext && steamapicontext->SteamFriends() )
		return GetClientModeTFNormal()->GameUI()->IsMainMenuVisible();
	return BaseClass::IsVisible();
	*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CExplanationPopup* CHudMainMenuOverride::StartHighlightAnimation( mm_highlight_anims iAnim )
{
	switch( iAnim )
	{
		case MMHA_TUTORIAL:		return ShowDashboardExplanation( "TutorialHighlight" );
		case MMHA_PRACTICE:		return ShowDashboardExplanation( "PracticeHighlight" );
		case MMHA_NEWUSERFORUM:	return ShowDashboardExplanation( "NewUserForumHighlight" );
		case MMHA_OPTIONS:		return ShowDashboardExplanation( "OptionsHighlightPanel" );
		case MMHA_LOADOUT:		return ShowDashboardExplanation( "LoadoutHighlightPanel" );
		case MMHA_STORE:		return ShowDashboardExplanation( "StoreHighlightPanel" );
	}

	Assert( false );
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Make the glows behind the update buttons stop pulsing
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::StopUpdateGlow()
{
	// Dont ever glow again
	if ( cl_mainmenu_updateglow.GetInt() )
	{
		cl_mainmenu_updateglow.SetValue( 0 );
		engine->ClientCmd_Unrestricted( "host_writeconfig" );
	}

	if ( m_pEventPromoContainer )
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >( "Background", true );
		if ( pUpdateBackground )
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pUpdateBackground, "MMenu_UpdateButton_StartGlow" );
			pUpdateBackground->SetControlVisible( "ViewDetailsGlow", false, true );
			pUpdateBackground->SetControlVisible( "ViewWarButtonGlow", false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show or hide the rank panels if the GC is connected
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateRankPanelVisibility()
{
	bool bConnectedToGC = GTFGCClientSystem()->BConnectedtoGC();

	m_pRankPanel->SetVisible( bConnectedToGC );
	m_pRankModelPanel->SetVisible( bConnectedToGC );
	SetControlVisible( "CycleRankTypeButton", bConnectedToGC );
	SetControlVisible( "NoGCMessage", !bConnectedToGC, true );
	SetControlVisible( "NoGCImage", !bConnectedToGC, true );
	UpdateRankPanelType();

	SetControlVisible("NoGCMessage", false);
	SetControlVisible("NoGCImage", false);
	SetControlVisible("RankBorder", false);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnCommand( const char *command )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(main_menu_override)", command );

	if ( Q_strnicmp( command, "soundentry", 10 ) == 0 )
	{
		PlaySoundEntry( command + 11 );
		return;
	}
	else if ( !Q_stricmp( command, "motd_viewurl" ) )
	{
		CMOTDEntryDefinition *pMOTD = GetMOTDManager().GetMOTDByIndex( m_iCurrentMOTD );
		if ( pMOTD )
		{
			const char *pszURL = pMOTD->GetURL();
			if ( pszURL && pszURL[0] )
			{
				if ( steamapicontext && steamapicontext->SteamFriends() )
				{
					steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( pszURL );
				}
			}
		}
		return;
	}
	else if ( !Q_stricmp( command, "view_newuser_forums" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://steamcommunity.com/app/440/discussions/" );
		}
		return;
	}
	else if ( !Q_stricmp( command, "opentf2options" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine opentf2options" );
	}
	else if ( !Q_stricmp( command, "motd_prev" ) )
	{
		if ( m_iCurrentMOTD > 0 )
		{
			m_iCurrentMOTD--;
			UpdateMOTD( false );
		}
		return;
	}
	else if ( !Q_stricmp( command, "motd_next" ) )
	{
		if ( m_iCurrentMOTD < (GetMOTDManager().GetNumMOTDs()-1) )
		{
			m_iCurrentMOTD++;
			UpdateMOTD( false );
		}
		return;
	}
	else if ( !Q_stricmp( command, "motd_show" ) )
	{
		SetMOTDVisible( !m_pMOTDPanel->IsVisible() );
	}
	else if ( !Q_stricmp( command, "motd_hide" ) )
	{
		SetMOTDVisible( false );
	}
	else if ( !Q_stricmp( command, "noti_show" ) )
	{
		SetNotificationsPanelVisible( true );
	}
	else if ( !Q_stricmp( command, "noti_hide" ) )
	{
		SetNotificationsPanelVisible( false );
	}
	else if ( !Q_stricmp( command, "notifications_update" ) )
	{
		// force visible if
		if ( NotificationQueue_GetNumMainMenuNotifications() != 0 )
		{
			SetNotificationsButtonVisible( true );
		}
		else
		{
			UpdateNotifications();
		}
	}
	else if ( !Q_stricmp( command, "test_anim" ) )
	{
		InvalidateLayout( true, true );

		StartHighlightAnimation( MMHA_TUTORIAL );
		StartHighlightAnimation( MMHA_PRACTICE );
		StartHighlightAnimation( MMHA_NEWUSERFORUM );
		StartHighlightAnimation( MMHA_OPTIONS );
		StartHighlightAnimation( MMHA_STORE );
		StartHighlightAnimation( MMHA_LOADOUT );
	}
	else if ( !Q_stricmp( command, "offlinepractice" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine training_showdlg" );
	}
	else if ( !Q_stricmp( command, "armory_open" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine open_charinfo_armory" );
	}
	else if ( !Q_stricmp( command, "engine disconnect" ) && engine->IsInGame() && TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() ) )
	{
		// If we're playing MvM, "New Game" should take us back to MvM matchmaking
		CTFDisconnectConfirmDialog *pDialog = BuildDisconnectConfirmDialog();
		if ( pDialog )
		{
			pDialog->Show();
		}
		return;
	}
	else if ( !Q_stricmp( command, "callvote" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine callvote" );
		if ( GetClientModeTFNormal()->GameUI() )
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand(  "ResumeGame" );
		}
		return;
	}
	else if ( !Q_stricmp( command, "showpromocodes" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			case k_EUniverseBeta:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://beta.steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://localhost/community/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			}
		}
	}
	else if ( !Q_stricmp( command, "exitreplayeditor" ) )
	{
 #if defined( REPLAY_ENABLED )
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return;

		pEditor->Exit_ShowDialogs();
#endif // REPLAY_ENABLED
	}
	else if ( FStrEq( "questlog", command ) )
	{
		SetQuestMapVisible( !GetQuestMapPanel()->IsVisible() );
	}
	else if ( FStrEq( "watch_stream", command ) )
	{
		//SetWatchStreamVisible( !m_pWatchStreamsPanel->IsVisible() );
		vgui::system()->ShellExecute( "open", "https://www.twitch.tv/directory/game/Team%20Fortress%202" );
	}
	else if ( FStrEq( "close_quest_map", command ) )
	{
		SetQuestMapVisible( false );
	}
	else if ( FStrEq( "view_update_page", command ) )
	{
		StopUpdateGlow();

		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/meetyourmatch" ); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://csham.valvesoftware.com/tf.com/meetyourmatch" ); break;
			}
		}
		else
		{
			OpenStoreStatusDialog( NULL, "#MMenu_OverlayRequired", true, false );
		}
		return;
	}
	else if ( FStrEq( "view_update_comic", command ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/gargoyles_and_gravel" ); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/gargoyles_and_gravel" ); break;
			}
		}
		else
		{
			OpenStoreStatusDialog( NULL, "#MMenu_OverlayRequired", true, false );
		}
		return;
	}
	else if ( FStrEq( "OpenMutePlayerDialog", command ) )
	{
		if ( !m_hMutePlayerDialog.Get() )
		{
			VPANEL hPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
			vgui::Panel* pPanel = vgui::ipanel()->GetPanel( hPanel, "BaseUI" );

			m_hMutePlayerDialog = vgui::SETUP_PANEL( new CMutePlayerDialog( pPanel ) );
			int x, y, ww, wt, wide, tall;
			vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
			m_hMutePlayerDialog->GetSize( wide, tall );

			// Center it, keeping requested size
			m_hMutePlayerDialog->SetPos( x + ( ( ww - wide ) / 2 ), y + ( ( wt - tall ) / 2 ) );
		}
		m_hMutePlayerDialog->Activate();
	}
	else if ( FStrEq( "open_rank_type_menu", command ) )
	{
		if ( m_pRankTypeMenu )
		{
			m_pRankTypeMenu->MarkForDeletion();
			m_pRankTypeMenu = NULL;
		}

		m_pRankTypeMenu = new Menu( this, "ranktypemenu" );

		MenuBuilder builder( m_pRankTypeMenu, this );
		const char *pszContextMenuBorder = "NotificationDefault";
		const char *pszContextMenuFont = "HudFontMediumSecondary";
		m_pRankTypeMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
		m_pRankTypeMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

		auto lambdaAddMatchTypeMenuOption = [ &builder ]( ETFMatchGroup eMatchGroup, bool bRequireRatingData = false )
		{
			auto pMatchGroup = GetMatchGroupDescription( eMatchGroup );
			Assert( pMatchGroup );
			if ( !pMatchGroup )
				return;

			if ( bRequireRatingData )
			{
				if ( !SteamUser() )
					return;

				EMMRating eRating = pMatchGroup->GetCurrentDisplayRank();
				CTFRatingData* pRatingData = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( SteamUser()->GetSteamID(), eRating );

				if ( !pRatingData )
					return;
			}

			wchar_t* pwszLocName = g_pVGuiLocalize->Find( pMatchGroup->GetNameLocToken() );
			CFmtStr strCommand( "view_match_rank_%d", eMatchGroup );
			builder.AddMenuItem( pwszLocName, strCommand.Get(), "type" );
		};
	
		lambdaAddMatchTypeMenuOption( k_eTFMatchGroup_Casual_12v12 );
		lambdaAddMatchTypeMenuOption( k_eTFMatchGroup_Ladder_6v6 );
		lambdaAddMatchTypeMenuOption( k_eTFMatchGroup_Event_Placeholder, true );

		// Position to the cursor's position
		int nX, nY;
		g_pVGuiInput->GetCursorPosition( nX, nY );
		m_pRankTypeMenu->SetPos( nX - 1, nY - 1 );

		m_pRankTypeMenu->SetVisible(true);
		m_pRankTypeMenu->AddActionSignalTarget(this);
	}
	else if ( V_strnicmp( "view_match_rank_", command, 16 ) == 0 )
	{
		ETFMatchGroup eMatchGroup = (ETFMatchGroup)atoi( command + 16 );
		tf_mainmenu_match_panel_type.SetValue( eMatchGroup );
	}
	else
	{
		// Pass it on to GameUI main menu
		if ( GetClientModeTFNormal()->GameUI() )
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( command );
			return;
		}
	}

	BaseClass::OnCommand( command );
}

void CHudMainMenuOverride::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_B && engine->IsInGame() )
	{
		OnCommand( "ResumeGame" );
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::CheckTrainingStatus( void )
{
	bool bNeedsTraining = tf_training_has_prompted_for_training.GetInt() <= 0;
	bool bNeedsPractice = tf_training_has_prompted_for_offline_practice.GetInt() <= 0;
	bool bShowForum = tf_training_has_prompted_for_forums.GetInt() <= 0;
	bool bShowOptions = tf_training_has_prompted_for_options.GetInt() <= 0;
	bool bWasInTraining = m_bWasInTraining;
	bool bDashboardSidePanels = GetMMDashboard()->BAnySidePanelsShowing();
	m_bWasInTraining = false;

	bool bShowLoadout = false;
	if ( tf_training_has_prompted_for_loadout.GetInt() <= 0 )
	{
		// See if we have any items in our inventory.
		int iNumItems = TFInventoryManager()->GetLocalTFInventory()->GetItemCount();
		if ( iNumItems > 0 )
		{
			bShowLoadout = true;
		}
	}

	if ( !tf_find_a_match_hint_viewed.GetBool() )
	{
		tf_find_a_match_hint_viewed.SetValue( true );
		ShowDashboardExplanation( "FindAMatch" );
	}
	else if ( !bDashboardSidePanels && bShowLoadout )
	{
		tf_training_has_prompted_for_loadout.SetValue( 1 );
		StartHighlightAnimation( MMHA_LOADOUT );
	}
	else if ( bDashboardSidePanels && bNeedsTraining)
	{
		tf_training_has_prompted_for_training.SetValue( 1 );

		auto pExplanation = StartHighlightAnimation( MMHA_TUTORIAL );
		pExplanation->AddActionSignalTarget( this );

		if ( pExplanation )
		{
			if ( UTIL_HasLoadedAnyMap() )
			{
				pExplanation->SetDialogVariable( "highlighttext", g_pVGuiLocalize->Find( "#MMenu_TutorialHighlight_Title2" ) );
			}
			else
			{
				pExplanation->SetDialogVariable( "highlighttext", g_pVGuiLocalize->Find( "#MMenu_TutorialHighlight_Title" ) );
			}
		}

		
	}
	else if ( bDashboardSidePanels && bWasInTraining && Training_IsComplete() == false && tf_training_has_prompted_for_training.GetInt() < 2 )
	{
		tf_training_has_prompted_for_training.SetValue( 2 );

		auto pExplanation = StartHighlightAnimation( MMHA_TUTORIAL );
		if ( pExplanation )
		{
			pExplanation->SetDialogVariable( "highlighttext", g_pVGuiLocalize->Find( "#MMenu_TutorialHighlight_Title3" ) );
		}
	}
	else if ( bDashboardSidePanels && bNeedsPractice )
	{
		tf_training_has_prompted_for_offline_practice.SetValue( 1 );
		StartHighlightAnimation( MMHA_PRACTICE );
	}
	else if ( bShowForum )
	{
		tf_training_has_prompted_for_forums.SetValue( 1 );
		StartHighlightAnimation( MMHA_NEWUSERFORUM );
	}
	else if ( bShowOptions )
	{
		tf_training_has_prompted_for_options.SetValue( 1 );
		StartHighlightAnimation( MMHA_OPTIONS );
	}
}

void CHudMainMenuOverride::UpdateRankPanelType()
{
	ETFMatchGroup eMatchGroup = (ETFMatchGroup)tf_mainmenu_match_panel_type.GetInt();

	// Sanitize the matchgroup they want to see.
	switch ( eMatchGroup )
	{
	case k_eTFMatchGroup_Casual_12v12:
	case k_eTFMatchGroup_Event_Placeholder:
	case k_eTFMatchGroup_Ladder_6v6:
		break;

	default:
		eMatchGroup = k_eTFMatchGroup_Casual_12v12;
	}

	m_pRankPanel->SetMatchGroup( eMatchGroup );
	m_pRankPanel->InvalidateLayout( true, true );
	m_pRankModelPanel->SetMatchGroup( eMatchGroup );
	m_pRankModelPanel->InvalidateLayout( true, true );

	m_pRankPanel->OnCommand( "begin_xp_lerp" );
	m_pRankModelPanel->OnCommand( "begin_xp_lerp" );

	// Show the comp ranks tooltip mouseover panel? (the little '(i)' image)
	bool bShowCompRankTooltip = false;
	auto pMatchGroup = GetMatchGroupDescription( eMatchGroup );
	if ( GetProgressionDesc( k_eProgression_Glicko ) == pMatchGroup->m_pProgressionDesc
		 && GTFGCClientSystem()->BConnectedtoGC() )
	{
		bShowCompRankTooltip = true;
	}

	Panel* pRankTooltipPanel = FindChildByName( "RankTooltipPanel" );
	if( pRankTooltipPanel )
	{
		pRankTooltipPanel->SetVisible( bShowCompRankTooltip );
		pRankTooltipPanel->SetTooltip( GetCompRanksTooltip(), nullptr );
	}
}


#define REMAP_COMMAND( oldCommand, newCommand ) \
	const char *pszKey##oldCommand = engine->Key_LookupBindingExact(#oldCommand); \
	const char *pszNewKey##oldCommand = engine->Key_LookupBindingExact(#newCommand); \
	if ( pszKey##oldCommand && !pszNewKey##oldCommand ) \
	{ \
		Msg( "Rebinding key %s to new command " #newCommand ".\n", pszKey##oldCommand ); \
		engine->ClientCmd_Unrestricted( VarArgs( "bind \"%s\" \"" #newCommand "\"\n", pszKey##oldCommand ) ); \
	}

//-----------------------------------------------------------------------------
// Purpose: Rebinds any binds for old commands to their new commands.
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PerformKeyRebindings( void )
{
	REMAP_COMMAND( inspect, +inspect );
	REMAP_COMMAND( taunt, +taunt );
	REMAP_COMMAND( use_action_slot_item, +use_action_slot_item );
	REMAP_COMMAND( use_action_slot_item_server, +use_action_slot_item_server );
}

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the MOTD request response
//-----------------------------------------------------------------------------
class CGCMOTDRequestResponse : public GCSDK::CGCClientJob
{
public:
	CGCMOTDRequestResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCMOTDRequestResponse_t> msg( pNetPacket );

		// No new entries?
		if ( !msg.Body().m_nEntries )
			return true;

		// No main menu panel?
		CHudMainMenuOverride *pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE );
		if ( !pMMPanel )
			return true;

		// Get our local language
		char uilanguage[ 64 ];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

		//V_strcpy_safe( uilanguage, "german" );

		KeyValues *pEntriesKV = new KeyValues( "motd_entries");

		// Try and load the cache file. If we fail, we'll just create a new one.
		if ( !pMMPanel->ReloadedAllMOTDs() )
		{
			pEntriesKV->LoadFromFile( g_pFullFileSystem, GC_MOTD_CACHE_FILE );
		}

		bool bNewMOTDs = false;

		// Store the time & language we last checked.
		char rtime_buf[k_RTimeRenderBufferSize];
		pEntriesKV->SetString( "last_request_time", CRTime::RTime32ToString( pMMPanel->GetLastMOTDRequestTime(), rtime_buf ) );
		pEntriesKV->SetString( "last_request_language", GetLanguageShortName( pMMPanel->GetLastMOTDRequestLanguage() ) );

		// Read in the entries one by one, and insert them into our keyvalues structure.
		for ( int i = 0; i < msg.Body().m_nEntries; i++ )
		{
			char pchMsgString[2048];

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;

			// If there's already an entry with this index, overwrite the data
			KeyValues *pNewEntry = pEntriesKV->FindKey( pchMsgString );
			if ( !pNewEntry )
			{
				pNewEntry = new KeyValues( pchMsgString );
				pEntriesKV->AddSubKey( pNewEntry );
			}
			pNewEntry->SetName( pchMsgString );

			RTime32 iTime;
			if ( !msg.BReadUintData( &iTime ) )
				return false;
			pNewEntry->SetString( "post_time", CRTime::RTime32ToString(iTime, rtime_buf) );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( VarArgs("title_%s", uilanguage), pchMsgString );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( VarArgs("text_%s", uilanguage), pchMsgString );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( "url", pchMsgString );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( "image", pchMsgString );

			int iHeadertype;
			if ( !msg.BReadIntData( &iHeadertype ) )
				return false;
			pNewEntry->SetString( "header_type", CFmtStr( "%d", iHeadertype ).Access() );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( VarArgs("header_%s", uilanguage), pchMsgString );

			if ( !msg.BReadStr( pchMsgString, Q_ARRAYSIZE( pchMsgString ) ) )
				return false;
			pNewEntry->SetString( "header_icon", pchMsgString );

			bNewMOTDs = true;
		}

		// Tell the schema to reload its MOTD block
		CUtlVector< CUtlString > vecErrors;
		pMMPanel->GetMOTDManager().BInitMOTDEntries( pEntriesKV, &vecErrors );
		pMMPanel->GetMOTDManager().PurgeUnusedMOTDEntries( pEntriesKV );

		// Save out our cache
		pEntriesKV->SaveToFile( g_pFullFileSystem, GC_MOTD_CACHE_FILE );

		// And tell the main menu to refresh the MOTD.
		//pMMPanel->SetMOTDVisible( bNewMOTDs ); HACK!  Temporarily turn this off!
		pMMPanel->UpdateMOTD( bNewMOTDs );
		return true;
	}
};

GC_REG_JOB( GCSDK::CGCClient, CGCMOTDRequestResponse, "CGCMOTDRequestResponse", k_EMsgGCMOTDRequestResponse, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::PerformLayout()
{
	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	// Resize our text labels to fit.
	int iW = 0;
	int iH = 0;
	for (int i = 0; i < m_pEmbeddedPanel->GetChildCount(); i++)
	{
		vgui::Label *pLabel = dynamic_cast<vgui::Label*>( m_pEmbeddedPanel->GetChild(i) );
		if ( !pLabel )
			continue;

		// Only checking to see if we have any text
		char szTmp[2];
		pLabel->GetText( szTmp, sizeof(szTmp) );
		if ( !szTmp[0] )
			continue;

		pLabel->InvalidateLayout(true);

		int iX, iY;
		pLabel->GetPos( iX, iY );
		iW = MAX( iW, ( pLabel->GetWide() + (iX * 2) ) );

		if ( iH == 0 )
		{
			iH += MAX( iH, pLabel->GetTall() + (iY * 2) );
		}
		else
		{
			iH += MAX( iH, pLabel->GetTall() );
		}
	}
	m_pEmbeddedPanel->SetSize( iW, iH );

	m_pEmbeddedPanel->SetVisible(true);

	PositionWindow( m_pEmbeddedPanel );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::HideTooltip()
{
	if ( m_pEmbeddedPanel )
	{
		m_pEmbeddedPanel->SetVisible(false);
	}

	BaseTooltip::HideTooltip();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::SetText(const char *pszText)
{
	if ( m_pEmbeddedPanel )
	{
		_isDirty = true;

		if ( pszText && pszText[0] == '#' )
		{
			m_pEmbeddedPanel->SetDialogVariable( "tiptext", g_pVGuiLocalize->Find( pszText ) );
		}
		else
		{
			m_pEmbeddedPanel->SetDialogVariable( "tiptext", pszText );
		}
		m_pEmbeddedPanel->SetDialogVariable( "tipsubtext", "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload the .res file
//-----------------------------------------------------------------------------
