//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

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

#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/vgui/replayperformanceeditor.h"
#include "materialsystem/itexture.h"
#include "imageutils.h"
#include "icommandline.h"
#include "vgui/ISystem.h"
#include "report_player_dialog.h"

#ifdef SAXXYMAINMENU_ENABLED
#include "tf_hud_saxxycontest.h"
#endif

#include "c_tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void AddSubKeyNamed(KeyValues* pKeys, const char* pszName);

extern const char* g_sImagesBlue[];
extern int EconWear_ToIntCategory(float flWear);

void cc_tf_safemode_toggle(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	CHudMainMenuOverride* pMMOverride = (CHudMainMenuOverride*)(gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE));
	if (pMMOverride)
	{
		pMMOverride->InvalidateLayout();
	}
}

ConVar tf_recent_achievements("tf_recent_achievements", "0", FCVAR_ARCHIVE);
ConVar tf_training_has_prompted_for_training("tf_training_has_prompted_for_training", "0", FCVAR_ARCHIVE, "Whether the user has been prompted for training");
ConVar tf_training_has_prompted_for_offline_practice("tf_training_has_prompted_for_offline_practice", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to try offline practice.");
ConVar tf_training_has_prompted_for_forums("tf_training_has_prompted_for_forums", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to view the new user forums.");
ConVar tf_training_has_prompted_for_options("tf_training_has_prompted_for_options", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to view the TF2 advanced options.");
ConVar tf_training_has_prompted_for_loadout("tf_training_has_prompted_for_loadout", "0", FCVAR_ARCHIVE, "Whether the user has been prompted to equip something in their loadout.");
ConVar cl_ask_bigpicture_controller_opt_out("cl_ask_bigpicture_controller_opt_out", "0", FCVAR_ARCHIVE, "Whether the user has opted out of being prompted for controller support in Big Picture.");
ConVar cl_mainmenu_operation_motd_start("cl_mainmenu_operation_motd_start", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN);
ConVar cl_mainmenu_operation_motd_reset("cl_mainmenu_operation_motd_reset", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN);
ConVar cl_mainmenu_safemode("cl_mainmenu_safemode", "0", FCVAR_NONE, "Enable safe mode", cc_tf_safemode_toggle);
ConVar cl_mainmenu_updateglow("cl_mainmenu_updateglow", "1", FCVAR_ARCHIVE | FCVAR_HIDDEN);

void cc_promotional_codes_button_changed(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	IViewPortPanel* pMMOverride = (gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE));
	if (pMMOverride)
	{
		((CHudMainMenuOverride*)pMMOverride)->UpdatePromotionalCodes();
	}
}
ConVar cl_promotional_codes_button_show("cl_promotional_codes_button_show", "1", FCVAR_ARCHIVE, "Toggles the 'View Promotional Codes' button in the main menu for players that have used the 'RIFT Well Spun Hat Claim Code'.", cc_promotional_codes_button_changed);

extern bool Training_IsComplete();

//-----------------------------------------------------------------------------
// Callback to launch the lobby UI
//-----------------------------------------------------------------------------
//static void CL_OpenMatchmakingLobby(const CCommand& args)
//{
//	if (GTFGCClientSystem()->GetMatchmakingUIState() != eMatchmakingUIState_InGame)
//	{
//		const char* arg1 = "";
//		if (args.ArgC() > 1)
//		{
//			arg1 = args[1];
//		}
//
//		// Make sure we are connected to steam, or they are going to be disappointed
//		if (steamapicontext == NULL
//			|| steamapicontext->SteamUtils() == NULL
//			|| steamapicontext->SteamMatchmakingServers() == NULL
//			|| steamapicontext->SteamUser() == NULL
//			|| !steamapicontext->SteamUser()->BLoggedOn()
//			) {
//			Warning("Steam not properly initialized or connected.\n");
//			ShowMessageBox("#TF_MM_GenericFailure_Title", "#TF_MM_GenericFailure", "#GameUI_OK");
//			return;
//		}
//
//		// Make sure we have a GC connection
//		if (!GCClientSystem()->BConnectedtoGC())
//		{
//			Warning("Not connected to GC.\n");
//			ShowMessageBox("#TF_MM_NoGC_Title", "#TF_MM_NoGC", "#GameUI_OK");
//			return;
//		}
//
//		// If we're idle, use our argument to start matchmaking.
//		if (GTFGCClientSystem()->GetMatchmakingUIState() == eMatchmakingUIState_Inactive)
//		{
//			TF_MatchmakingMode mode = TF_Matchmaking_LADDER;
//			if (FStrEq(args[1], "mvm"))
//			{
//				mode = TF_Matchmaking_MVM;
//			}
//			else if (FStrEq(args[1], "ladder"))
//			{
//				mode = TF_Matchmaking_LADDER;
//			}
//			else if (FStrEq(args[1], "casual"))
//			{
//				mode = TF_Matchmaking_CASUAL;
//			}
//
//			GTFGCClientSystem()->BeginMatchmaking(mode);
//		}
//	}
//
//	CHudMainMenuOverride* pMMOverride = (CHudMainMenuOverride*)(gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE));
//	if (pMMOverride)
//	{
//		switch (GTFGCClientSystem()->GetSearchMode())
//		{
//		case TF_Matchmaking_MVM:
//			pMMOverride->OpenMvMMMPanel();
//			break;
//
//		case TF_Matchmaking_LADDER:
//			pMMOverride->OpenCompMMPanel();
//			break;
//
//		case TF_Matchmaking_CASUAL:
//			pMMOverride->OpenCasualMMPanel();
//
//		default:
//			return;
//		}
//	}
//}
//
//static ConCommand openmatchmakinglobby_command("OpenMatchmakingLobby", &CL_OpenMatchmakingLobby, "Activates the matchmaking lobby.");
//
//static void CL_ReloadMMPanels(const CCommand& args)
//{
//	CHudMainMenuOverride* pMMOverride = (CHudMainMenuOverride*)(gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE));
//	if (pMMOverride)
//	{
//		pMMOverride->ReloadMMPanels();
//	}
//}
//ConCommand reload_mm_panels("reload_mm_panels", &CL_ReloadMMPanels);

//-----------------------------------------------------------------------------
// Purpose: Prompt the user and ask if they really want to start training (if they are in a game)
//-----------------------------------------------------------------------------
class CTFConfirmTrainingDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE(CTFConfirmTrainingDialog, CConfirmDialog);
public:
	CTFConfirmTrainingDialog(const char* pText, const char* pTitle, vgui::Panel* parent) : BaseClass(parent), m_pText(pText), m_pTitle(pTitle) {}

	virtual const wchar_t* GetText()
	{
		return g_pVGuiLocalize->Find(m_pText);
	}

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		// Set the X to be bright, and the rest dull
		if (m_pConfirmButton)
		{
			m_pConfirmButton->SetText("#TF_Training_Prompt_ConfirmButton");
		}
		if (m_pCancelButton)
		{
			m_pCancelButton->SetText("#TF_Training_Prompt_CancelButton");
		}

		CExLabel* pTitle = dynamic_cast<CExLabel*>(FindChildByName("TitleLabel"));
		if (pTitle)
		{
			pTitle->SetText(m_pTitle);
		}
	}
protected:
	const char* m_pText;
	const char* m_pTitle;
};

class CCompetitiveAccessInfoPanel : public EditablePanel, public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE(CCompetitiveAccessInfoPanel, EditablePanel);
public:
	CCompetitiveAccessInfoPanel(Panel* pParent, const char* pszName)
		: EditablePanel(pParent, pszName)
	{
		m_pPhoneButton = NULL;
		m_pPremiumButton = NULL;
		m_pPhoneCheckImage = NULL;
		m_pPremiumCheckImage = NULL;
	}

	virtual void ApplySchemeSettings(IScheme* pScheme) OVERRIDE
	{
		BaseClass::ApplySchemeSettings(pScheme);
		LoadControlSettings("resource/ui/CompetitiveAccessInfo.res");

		m_pPhoneButton = FindControl< CExImageButton >("PhoneButton", true);
		m_pPremiumButton = FindControl< CExImageButton >("PremiumButton", true);
		m_pPhoneCheckImage = FindControl< ImagePanel >("PhoneCheckImage", true);
		m_pPremiumCheckImage = FindControl< ImagePanel >("PremiumCheckImage", true);
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		bool bIsFreeAccount = IsFreeTrialAccount();
		if (m_pPremiumButton)
		{
			m_pPremiumButton->SetEnabled(bIsFreeAccount);
		}
		if (m_pPremiumCheckImage)
		{
			m_pPremiumCheckImage->SetVisible(!bIsFreeAccount);
		}

		bool bIsPhoneVerified = GTFGCClientSystem()->BIsPhoneVerified();
		bool bIsPhoneIdentifying = GTFGCClientSystem()->BIsPhoneIdentifying();
		bool bPhoneReady = bIsPhoneVerified && bIsPhoneIdentifying;
		if (m_pPhoneButton)
		{
			m_pPhoneButton->SetEnabled(!bPhoneReady);
		}
		if (m_pPhoneCheckImage)
		{
			m_pPhoneCheckImage->SetVisible(bPhoneReady);
		}
	}

	virtual void OnCommand(const char* command) OVERRIDE
	{
		if (FStrEq(command, "close"))
		{
			SetVisible(false);
			return;
		}
		else if (FStrEq(command, "addphone"))
		{
			if (steamapicontext && steamapicontext->SteamFriends())
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("https://support.steampowered.com/kb_article.php?ref=8625-WRAH-9030#addphone");
			}
			return;
		}
		else if (FStrEq(command, "addpremium"))
		{
			if (steamapicontext && steamapicontext->SteamFriends())
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("https://steamcommunity.com/sharedfiles/filedetails/?id=143430756");
			}
			return;
		}

		BaseClass::OnCommand(command);
	}

	virtual void SOCreated(const CSteamID& steamIDOwner, const CSharedObject* pObject, ESOCacheEvent eEvent) OVERRIDE
	{
		if (pObject->GetTypeID() != CEconGameAccountClient::k_nTypeID)
			return;

		if (GTFGCClientSystem()->BHasCompetitiveAccess())
		{
			SetVisible(false);
		}
		else
		{
			InvalidateLayout();
		}
	}

	virtual void SOUpdated(const CSteamID& steamIDOwner, const CSharedObject* pObject, ESOCacheEvent eEvent) OVERRIDE
	{
		if (pObject->GetTypeID() != CEconGameAccountClient::k_nTypeID)
			return;

		if (GTFGCClientSystem()->BHasCompetitiveAccess())
		{
			SetVisible(false);
		}
		else
		{
			InvalidateLayout();
		}
	}

private:
	CExImageButton* m_pPhoneButton;
	CExImageButton* m_pPremiumButton;
	ImagePanel* m_pPhoneCheckImage;
	ImagePanel* m_pPremiumCheckImage;
};
DECLARE_BUILD_FACTORY(CCompetitiveAccessInfoPanel);

class CMainMenuPlayListEntry : public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CMainMenuPlayListEntry, EditablePanel);
public:

	enum EDisabledStates_t
	{
		NOT_DISABLED = 0,
		DISABLED_NO_COMP_ACCESS,
		DISABLED_NO_GC,
		DISABLED_MATCH_RUNNING,

		NUM_DISABLED_STATES
	};

	CMainMenuPlayListEntry(Panel* pParent, const char* pszName)
		: EditablePanel(pParent, pszName)
	{
		m_pToolTip = NULL;
	}

	~CMainMenuPlayListEntry()
	{
		if (m_pToolTip != NULL)
		{
			delete m_pToolTip;
			m_pToolTip = NULL;
		}
	}

	virtual void ApplySchemeSettings(IScheme* pScheme) OVERRIDE
	{
		BaseClass::ApplySchemeSettings(pScheme);
		LoadControlSettings("resource/ui/MainMenuPlayListEntry.res");

		CExImageButton* pLockImage = FindControl< CExImageButton >("LockImage");
		if (pLockImage)
		{
			EditablePanel* pToolTipPanel = FindControl< EditablePanel >("TooltipPanel");
			if (pToolTipPanel)
			{
				m_pToolTip = new CTFTextToolTip(this);
				m_pToolTip->SetEmbeddedPanel(pToolTipPanel);
				pToolTipPanel->MakePopup(false, true);
				pToolTipPanel->SetKeyBoardInputEnabled(false);
				pToolTipPanel->SetMouseInputEnabled(false);
				m_pToolTip->SetText("#TF_Competitive_Requirements");
				m_pToolTip->SetTooltipDelay(0);
				pLockImage->SetTooltip(m_pToolTip, "#TF_Competitive_Requirements");
			}
		}

		SetDisabledReason(NOT_DISABLED);
	}

	virtual void ApplySettings(KeyValues* inResourceData) OVERRIDE
	{
		BaseClass::ApplySettings(inResourceData);

		m_strImageName = inResourceData->GetString("image_name");
		m_strButtonCommand = inResourceData->GetString("button_command");
		m_strButtonToken = inResourceData->GetString("button_token");
		m_strDescToken = inResourceData->GetString("desc_token");
	}

	void SetDisabledReason(EDisabledStates_t eReason)
	{
		static const DisabledStateDesc_t s_DisabledStates[] = { { NULL,								NULL,				NULL }				// NOT_DISABLED
															  , { "#TF_Competitive_Requirements",	"comp_access_info", "locked_icon" }		// DISABLED_NO_COMP_ACCESS
															  , { "#TF_MM_NoGC",					NULL,				"gc_dc"		}		// DISABLED_NO_GC
															  , { "#TF_Competitive_MatchRunning",			NULL,				NULL } };			// DISABLED_MATCH_RUNNING

		COMPILE_TIME_ASSERT(ARRAYSIZE(s_DisabledStates) == NUM_DISABLED_STATES);

		const DisabledStateDesc_t& stateDisabled = s_DisabledStates[eReason];

		SetControlEnabled("ModeButton", stateDisabled.m_pszLocToken == NULL);
		SetControlVisible("LockImage", stateDisabled.m_pszLocToken != NULL);

		CExImageButton* pLockImage = FindControl< CExImageButton >("LockImage");
		if (pLockImage)
		{
			if (stateDisabled.m_pszImageName)
			{
				pLockImage->SetSubImage(stateDisabled.m_pszImageName);
			}

			// Button behavior
			pLockImage->SetEnabled(stateDisabled.m_pszButtonCommand != NULL);
			pLockImage->SetCommand(stateDisabled.m_pszButtonCommand);
			pLockImage->GetImage()->SetVisible(stateDisabled.m_pszImageName != NULL);

			m_pToolTip->SetText(stateDisabled.m_pszLocToken);
			pLockImage->SetTooltip(m_pToolTip, stateDisabled.m_pszLocToken);
			m_pToolTip->PerformLayout();
		}
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		ImagePanel* pModeImage = FindControl< ImagePanel >("ModeImage");
		if (pModeImage)
		{
			pModeImage->SetImage(m_strImageName);
		}

		Button* pButton = FindControl< Button >("ModeButton");
		if (pButton)
		{
			pButton->SetCommand(m_strButtonCommand);
		}

		Label* pLabel = FindControl< Label >("ModeButton");
		if (pLabel)
		{
			pLabel->SetText(m_strButtonToken);
		}
		pLabel = FindControl< Label >("DescLabel");
		if (pLabel)
		{
			pLabel->SetText(m_strDescToken);
		}
		pLabel = FindControl< Label >("DescLabelShadow");
		if (pLabel)
		{
			pLabel->SetText(m_strDescToken);
		}
	}



private:

	struct DisabledStateDesc_t
	{
		const char* m_pszLocToken;
		const char* m_pszButtonCommand;
		const char* m_pszImageName;
	};

	CUtlString m_strImageName;
	CUtlString m_strButtonCommand;
	CUtlString m_strButtonToken;
	CUtlString m_strDescToken;

	CTFTextToolTip* m_pToolTip;
};

DECLARE_BUILD_FACTORY(CMainMenuPlayListEntry);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMainMenuOverride::CHudMainMenuOverride(IViewPort* pViewPort) : BaseClass(NULL, PANEL_MAINMENUOVERRIDE)
{
	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete(false);
	SetVisible(true);

	m_bPlayListExpanded = false;
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

	m_pFeaturedItemPanel = NULL;//new CItemModelPanel( m_pStoreSpecialPanel, "FeaturedItemModelPanel" );
	m_bReapplyButtonKVs = false;

	m_pMouseOverItemPanel = vgui::SETUP_PANEL(new CItemModelPanel(this, "mouseoveritempanel"));
	m_pMouseOverTooltip = new CItemModelPanelToolTip(this);
	m_pMouseOverTooltip->SetupPanels(this, m_pMouseOverItemPanel);

	m_pMOTDHeaderLabel = NULL;
	m_pMOTDHeaderIcon = NULL;
	m_pMOTDTitleLabel = NULL;
	m_pMOTDTitleImageContainer = NULL;
	m_pMOTDTitleImage = NULL;
	m_hTitleLabelFont = vgui::INVALID_FONT;

	m_pQuestLogButton = new EditablePanel(this, "QuestLogButton");

#ifdef STAGING_ONLY
	m_bGeneratingIcons = false;
	m_pIconData = NULL;
#endif

	m_bHaveNewMOTDs = false;
	m_bMOTDShownAtStartup = false;

	m_pCharacterImagePanel = NULL;
	m_iCharacterImageIdx = -1;

#ifdef SAXXYMAINMENU_ENABLED
	m_pSaxxyAwardsPanel = NULL;
	m_pSaxxySettings = NULL;
#endif

	m_pWarLandingPage = new CWarLandingPanel(this, "WarPanel");

	m_flCheckTrainingAt = 0;
	m_bWasInTraining = false;
	m_flLastWarNagTime = 0.f;

	ScheduleItemCheck();

	m_pToolTip = new CMainMenuToolTip(this);
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel(this, "TooltipPanel");
	m_pToolTipEmbeddedPanel->MakePopup(false, true);
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled(false);
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled(false);
	m_pToolTip->SetEmbeddedPanel(m_pToolTipEmbeddedPanel);
	m_pToolTip->SetTooltipDelay(0);

	ListenForGameEvent("gc_connected");
	ListenForGameEvent("item_schema_initialized");
	ListenForGameEvent("store_pricesheet_updated");
	ListenForGameEvent("inventory_updated");
	ListenForGameEvent("gameui_activated");
	ListenForGameEvent("party_updated");

	// Create our MOTD scrollable section
	m_pMOTDPanel = new vgui::EditablePanel(this, "MOTD_Panel");
	m_pMOTDPanel->SetVisible(true);
	m_pMOTDTextPanel = new vgui::EditablePanel(this, "MOTD_TextPanel");
	m_pMOTDTextScroller = new vgui::ScrollableEditablePanel(m_pMOTDPanel, m_pMOTDTextPanel, "MOTD_TextScroller");

	m_pMOTDTextScroller->GetScrollbar()->SetAutohideButtons(true);
	m_pMOTDTextScroller->GetScrollbar()->SetPaintBorderEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->SetPaintBackgroundEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->GetButton(0)->SetPaintBorderEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->GetButton(0)->SetPaintBackgroundEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->GetButton(1)->SetPaintBorderEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->GetButton(1)->SetPaintBackgroundEnabled(false);
	m_pMOTDTextScroller->GetScrollbar()->SetAutoResize(PIN_TOPRIGHT, AUTORESIZE_DOWN, -24, 0, -16, 0);

	m_pMOTDTextLabel = NULL;

	m_pNotificationsShowPanel = NULL;
	m_pNotificationsPanel = new vgui::EditablePanel(this, "Notifications_Panel");
	m_pNotificationsControl = NotificationQueue_CreateMainMenuUIElement(m_pNotificationsPanel, "Notifications_Control");
	m_pNotificationsScroller = new vgui::ScrollableEditablePanel(m_pNotificationsPanel, m_pNotificationsControl, "Notifications_Scroller");

	m_iNumNotifications = 0;

	m_pFeaturedItemMouseOverPanel = new CItemModelPanel(this, "FeaturedItemMouseOverItemPanel");
	m_pFeaturedItemToolTip = new CSimplePanelToolTip(this);
	m_pFeaturedItemToolTip->SetControlledPanel(m_pFeaturedItemMouseOverPanel);

	m_pBackground = new vgui::ImagePanel(this, "Background");
	m_pEventPromoContainer = new EditablePanel(this, "EventPromo");
	m_pSafeModeContainer = new EditablePanel(this, "SafeMode");

	// Cause the quest UI to be created
	//GetQuestLog();

	m_bStabilizedInitialLayout = false;

	m_bBackgroundUsesCharacterImages = true;

	m_pWatchStreamsPanel = new CTFStreamListPanel(this, "StreamListPanel");

	vgui::ivgui()->AddTickSignal(GetVPanel(), 50);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMainMenuOverride::~CHudMainMenuOverride(void)
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent("interface_close", "main_menu_override");

	if (GetClientModeTFNormal()->GameUI())
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride(NULL);
	}

	if (m_pButtonKV)
	{
		m_pButtonKV->deleteThis();
		m_pButtonKV = NULL;
	}

	// Stop Animation Sequences
	if (m_pNotificationsShowPanel)
	{
		g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel(m_pNotificationsShowPanel);
	}

	vgui::ivgui()->RemoveTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Override painting traversal to suppress main menu painting if we're not ready to show yet
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PaintTraverse(bool Repaint, bool allowForce)
{
	// Ugly hack: disable painting until we're done screwing around with updating the layout during initialization.
	// Use -menupaintduringinit command line parameter to reinstate old behavior
	if (m_bStabilizedInitialLayout || CommandLine()->CheckParm("-menupaintduringinit"))
	{
		BaseClass::PaintTraverse(Repaint, allowForce);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnTick()
{
	if (m_iNumNotifications != NotificationQueue_GetNumNotifications())
	{
		m_iNumNotifications = NotificationQueue_GetNumNotifications();
		UpdateNotifications();
		//CheckForNewQuests();
	}
	else if (m_pNotificationsPanel->IsVisible())
	{
		AdjustNotificationsPanelHeight();
	}

	static bool s_bRanOnce = false;
	if (!s_bRanOnce)
	{
		s_bRanOnce = true;
		if (char const* szConnectAdr = CommandLine()->ParmValue("+connect"))
		{
			Msg("Executing deferred connect command: %s\n", szConnectAdr);
			engine->ExecuteClientCmd(CFmtStr("connect %s -%s\n", szConnectAdr, "ConnectStringOnCommandline"));
		}
	}

	// See if its time to nag about joining the war
	float flTimeSinceWarNag = Plat_FloatTime() - m_flLastWarNagTime;
	if (!m_bPlayListExpanded && m_pHighlightAnims[MMHA_WAR] && (flTimeSinceWarNag > 300.f || m_flLastWarNagTime == 0.f))
	{
		// Make sure our SOCache is ready
		GCSDK::CGCClientSharedObjectCache* pSOCache = NULL;
		if (steamapicontext && steamapicontext->SteamUser())
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			pSOCache = GCClientSystem()->GetSOCache(steamID);
		}

		// Need to be initialized.  If we're not, we'll get false positives
		// when we actually go to look for our war data
		if (pSOCache && pSOCache->BIsInitialized())
		{
			m_flLastWarNagTime = Plat_FloatTime();

			// Get war data
			const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex(PYRO_VS_HEAVY_WAR_DEF_INDEX);
			CWarData* pWarData = GetLocalPlayerWarData(pWarDef->GetDefIndex());
			war_side_t nAffiliation = INVALID_WAR_SIDE;
			if (pWarData)
			{
				// Get affiliation if they have one.
				nAffiliation = pWarData->Obj().affiliation();
			}

			// They haven't joined the war!  Nag 'em
			if (nAffiliation == INVALID_WAR_SIDE && pWarDef->IsActive())
			{
				StartHighlightAnimation(MMHA_WAR);
			}
		}
	}


#ifdef STAGING_ONLY
	if (m_bGeneratingIcons)
	{
		GenerateIconsThink();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AttachToGameUI(void)
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent("interface_open", "main_menu_override");

	if (GetClientModeTFNormal()->GameUI())
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride(GetVPanel());
	}

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetCursor(dc_arrow);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar tf_last_store_pricesheet_version("tf_last_store_pricesheet_version", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD | FCVAR_HIDDEN);

void CHudMainMenuOverride::FireGameEvent(IGameEvent* event)
{
	const char* type = event->GetName();

	if (FStrEq(type, "gameui_activated"))
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "MMenu_PlayList_Collapse_Immediate", false);
		m_bPlayListExpanded = false;
		return;
	}
	if (Q_strcmp(type, "gc_connected") == 0)
	{
		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage(uilanguage, sizeof(uilanguage));

		//V_strcpy_safe( uilanguage, "german" );

		ELanguage nCurLang = PchLanguageToELanguage(uilanguage);

		// If we've changed language from when we last requested, we ask for all MOTDs again.
		if (nCurLang != m_nLastMOTDRequestLanguage)
		{
			m_nLastMOTDRequestAt = 0;
			m_nLastMOTDRequestLanguage = nCurLang;
			m_bReloadedAllMOTDs = true;
		}

		// Ask the GC for the MOTD
		GCSDK::CGCMsg<MsgGCMOTDRequest_t> msg(k_EMsgGCMOTDRequest);
		msg.Body().m_eLanguage = nCurLang;
		msg.Body().m_nLastMOTDRequest = m_nLastMOTDRequestAt;
		GCClientSystem()->BSendMessage(msg);

		// Roll our last asked time forward here. It won't get written to our
		// cache file if we don't get a response from the GC.
		CRTime cTimeHack;
		m_nLastMOTDRequestAt = CRTime::RTime32TimeCur();

		// Load the store info, so we can display the current special
		CStorePanel::RequestPricesheet();
		//CheckForNewQuests();
		//UpdatePlaylistEntries();
	}
	else if (Q_strcmp(type, "item_schema_initialized") == 0)
	{
		// Tell the schema to load its MOTD block from our clientside cache file
		CUtlVector< CUtlString > vecErrors;
		KeyValues* pEntriesKV = new KeyValues("motd_entries");
		if (pEntriesKV->LoadFromFile(g_pFullFileSystem, GC_MOTD_CACHE_FILE))
		{
			// Extract our last MOTD request time
			const char* pszTime = pEntriesKV->GetString("last_request_time", NULL);
			m_nLastMOTDRequestAt = (pszTime && pszTime[0]) ? CRTime::RTime32FromString(pszTime) : 0;

			const char* pszLang = pEntriesKV->GetString("last_request_language", NULL);
			m_nLastMOTDRequestLanguage = (pszLang && pszLang[0]) ? PchLanguageToELanguage(pszLang) : k_Lang_English;

			// Parse the entries
			GetMOTDManager().BInitMOTDEntries(pEntriesKV, &vecErrors);
			GetMOTDManager().PurgeUnusedMOTDEntries(pEntriesKV);
		}
	}
	else if (Q_strcmp(type, "store_pricesheet_updated") == 0)
	{
		// If the contents of the store have changed since the last time we went in and/or launched
		// the game, change the button color so that players know there's new content available.
		if (EconUI() &&
			EconUI()->GetStorePanel() &&
			EconUI()->GetStorePanel()->GetPriceSheet())
		{
			const CEconStorePriceSheet* pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();

			// The cvar system can't deal with integers that lose data when represented as floating point
			// numbers. We don't really care about supreme accuracy for detecting changes -- worst case if
			// we change the price sheet almost exactly 18 hours apart, some subset of players won't get the
			// "new!" label and that's fine.
			const uint32 unPriceSheetVersion = (uint32)pPriceSheet->GetVersionStamp() & 0xffff;

			if (unPriceSheetVersion != (uint32)tf_last_store_pricesheet_version.GetInt())
			{
				tf_last_store_pricesheet_version.SetValue((int)unPriceSheetVersion);

				if (m_pStoreHasNewItemsImage)
				{
					m_pStoreHasNewItemsImage->SetVisible(true);
				}
			}
		}

		// might as well do this here too
		UpdatePromotionalCodes();

		LoadCharacterImageFile();

		if (NeedsToChooseMostHelpfulFriend())
		{
			NotifyNeedsToChooseMostHelpfulFriend();
		}
	}
	//else if (FStrEq("inventory_updated", type))
	//{
	//	CheckForNewQuests();
	//}
	//else if (FStrEq("party_updated", type))
	//{
	//	UpdatePlaylistEntries();
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySettings(KeyValues* inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	KeyValues* pItemKV = inResourceData->FindKey("button_kv");
	if (pItemKV)
	{
		if (m_pButtonKV)
		{
			m_pButtonKV->deleteThis();
		}
		m_pButtonKV = new KeyValues("button_kv");
		pItemKV->CopySubkeys(m_pButtonKV);

		m_bReapplyButtonKVs = true;
	}

#ifdef SAXXYMAINMENU_ENABLED
	KeyValues* pSaxxySettings = inResourceData->FindKey("SaxxySettings");
	if (pSaxxySettings)
	{
		if (m_pSaxxySettings)
		{
			m_pSaxxySettings->deleteThis();
		}
		m_pSaxxySettings = pSaxxySettings->MakeCopy();

		if (m_pSaxxyAwardsPanel)
		{
			m_pSaxxyAwardsPanel->ApplySettings(m_pSaxxySettings);
		}
	}
#endif

	m_bPlayListExpanded = false;

	//UpdatePlaylistEntries();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySchemeSettings(IScheme* scheme)
{
	// We need to re-hook ourselves up to the TF client scheme, because the GameUI will try to change us their its scheme
	vgui::HScheme pScheme = vgui::scheme()->LoadSchemeFromFileEx(enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(pScheme);
	SetProportional(true);

	m_pFeaturedItemMouseOverPanel->InvalidateLayout(true, true);
	m_bBackgroundUsesCharacterImages = true;

	bool bHolidayActive = false;
	KeyValues* pConditions = NULL;
	const char* pszHoliday = UTIL_GetActiveHolidayString();
	if (pszHoliday && pszHoliday[0])
	{
		pConditions = new KeyValues("conditions");

		char szCondition[64];
		Q_snprintf(szCondition, sizeof(szCondition), "if_%s", pszHoliday);
		AddSubKeyNamed(pConditions, szCondition);

		if (FStrEq(pszHoliday, "halloween"))
		{

			// for Halloween we also want to pick a random background
			int nBackground = RandomInt(0, 5);

			AddSubKeyNamed(pConditions, CFmtStr("if_halloween_%d", nBackground));
			if ((nBackground == 3) || (nBackground == 4))
			{
				m_bBackgroundUsesCharacterImages = false;
			}
		}
		else if (FStrEq(pszHoliday, "christmas"))
		{
			// for Christmas we also want to pick a random background
			int nBackground = RandomInt(0, 1);
			AddSubKeyNamed(pConditions, CFmtStr("if_christmas_%d", nBackground));
		}

		bHolidayActive = true;
	}

	//if (!bHolidayActive)
	//{
	//	FOR_EACH_MAP_FAST(GetItemSchema()->GetOperationDefinitions(), iOperation)
	//	{
	//		CEconOperationDefinition* pOperation = GetItemSchema()->GetOperationDefinitions()[iOperation];
	//		if (!pOperation || !pOperation->IsActive() || !pOperation->IsCampaign())
	//			continue;
	//	
	//		if (!pConditions)
	//			pConditions = new KeyValues("conditions");
	//	
	//		AddSubKeyNamed(pConditions, "if_operation");
	//		break;
	//	}
	//}

	if (!pConditions)
	{
		pConditions = new KeyValues("conditions");
	}

	// Put in ratio condition
	float aspectRatio = engine->GetScreenAspectRatio();
	AddSubKeyNamed(pConditions, aspectRatio >= 1.6 ? "if_wider" : "if_taller");

	RemoveAllMenuEntries();

	LoadControlSettings("resource/UI/MainMenuOverride.res", NULL, NULL, pConditions);

	BaseClass::ApplySchemeSettings(vgui::scheme()->GetIScheme(pScheme));

	if (pConditions)
	{
		pConditions->deleteThis();
	}

	m_pQuitButton = dynamic_cast<CExButton*>(FindChildByName("QuitButton"));
	m_pDisconnectButton = dynamic_cast<CExButton*>(FindChildByName("DisconnectButton"));
	m_pBackToReplaysButton = dynamic_cast<CExButton*>(FindChildByName("BackToReplaysButton"));
	m_pStoreHasNewItemsImage = dynamic_cast<ImagePanel*>(FindChildByName("StoreHasNewItemsImage", true));

	{
		Panel* pButton = FindChildByName("VRModeButton");
		if (pButton)
		{
			m_pVRModeButton = dynamic_cast<CExButton*>(pButton->GetChild(0));
		}
	}
	m_pVRModeBackground = FindChildByName("VRBGPanel");

	bool bShowVR = materials->GetCurrentConfigForVideoCard().m_nVRModeAdapter == materials->GetCurrentAdapter();
	if (m_pVRModeBackground)
	{
		m_pVRModeBackground->SetVisible(bShowVR);
	}

	m_bIsDisconnectText = true;

	// Tell all the MOTD buttons that we want their messages
	m_pMOTDPrevButton = dynamic_cast<CExImageButton*>(m_pMOTDPanel->FindChildByName("MOTD_PrevButton"));
	m_pMOTDNextButton = dynamic_cast<CExImageButton*>(m_pMOTDPanel->FindChildByName("MOTD_NextButton"));
	m_pMOTDURLButton = dynamic_cast<CExButton*>(m_pMOTDPanel->FindChildByName("MOTD_URLButton"));

	// m_pNotificationsShowPanel shows number of unread notifications. Pressing it pops up the first notification.
	m_pNotificationsShowPanel = dynamic_cast<vgui::EditablePanel*>(FindChildByName("Notifications_ShowButtonPanel"));

	m_iNotiPanelWide = m_pNotificationsPanel->GetWide();

	// m_pMOTDShowPanel shows that the player has an unread MOTD. Pressing it pops up the MOTD.
	m_pMOTDShowPanel = dynamic_cast<vgui::EditablePanel*>(FindChildByName("MOTD_ShowButtonPanel"));

	vgui::EditablePanel* pHeaderContainer = dynamic_cast<vgui::EditablePanel*>(m_pMOTDPanel->FindChildByName("MOTD_HeaderContainer"));
	if (pHeaderContainer)
	{
		m_pMOTDHeaderLabel = dynamic_cast<vgui::Label*>(pHeaderContainer->FindChildByName("MOTD_HeaderLabel"));
	}

	m_pMOTDHeaderIcon = dynamic_cast<vgui::ImagePanel*>(m_pMOTDPanel->FindChildByName("MOTD_HeaderIcon"));

	m_pMOTDTitleLabel = dynamic_cast<vgui::Label*>(m_pMOTDPanel->FindChildByName("MOTD_TitleLabel"));
	if (m_pMOTDTitleLabel)
	{
		m_hTitleLabelFont = m_pMOTDTitleLabel->GetFont();
	}

	m_pMOTDTextLabel = dynamic_cast<vgui::Label*>(m_pMOTDTextPanel->FindChildByName("MOTD_TextLabel"));

	m_pMOTDTitleImageContainer = dynamic_cast<vgui::EditablePanel*>(m_pMOTDPanel->FindChildByName("MOTD_TitleImageContainer"));
	if (m_pMOTDTitleImageContainer)
	{
		m_pMOTDTitleImage = dynamic_cast<vgui::ImagePanel*>(m_pMOTDTitleImageContainer->FindChildByName("MOTD_TitleImage"));
	}

	m_pNotificationsScroller->GetScrollbar()->SetAutohideButtons(true);
	m_pNotificationsScroller->GetScrollbar()->SetPaintBorderEnabled(false);
	m_pNotificationsScroller->GetScrollbar()->SetPaintBackgroundEnabled(false);
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBorderEnabled(false);
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBackgroundEnabled(false);
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBorderEnabled(false);
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBackgroundEnabled(false);

	// Add tooltips for various buttons
	CExImageButton* pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("CommentaryButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_Commentary");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("CoachPlayersButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_Coach");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("ReportBugButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_ReportBug");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("AchievementsButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_Achievements");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("NewUserForumsButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_NewUserForum");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("ReplayButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_Replay");
	}
	pImageButton = dynamic_cast<CExImageButton*>(FindChildByName("WorkshopButton"));
	if (pImageButton)
	{
		pImageButton->SetTooltip(m_pToolTip, "#MMenu_Tooltip_Workshop");
	}

	// Highlights
	m_pHighlightAnims[MMHA_TUTORIAL] = FindControl< CExplanationPopup >("TutorialHighlight");
	m_pHighlightAnims[MMHA_PRACTICE] = FindControl< CExplanationPopup >("PracticeHighlight");
	m_pHighlightAnims[MMHA_NEWUSERFORUM] = FindControl< CExplanationPopup >("NewUserForumHighlight");
	m_pHighlightAnims[MMHA_OPTIONS] = FindControl< CExplanationPopup >("OptionsHighlightPanel");
	m_pHighlightAnims[MMHA_LOADOUT] = FindControl< CExplanationPopup >("LoadoutHighlightPanel");
	m_pHighlightAnims[MMHA_STORE] = FindControl< CExplanationPopup >("StoreHighlightPanel");
	m_pHighlightAnims[MMHA_WAR] = FindControl< CExplanationPopup >("WarHighlightPanel");

	m_pCompetitiveAccessInfo = dynamic_cast<vgui::EditablePanel*>(FindChildByName("CompetitiveAccessInfoPanel"));

	LoadCharacterImageFile();

	RemoveAllMenuEntries();
	LoadMenuEntries();

	UpdateNotifications();
	UpdatePromotionalCodes();

	ScheduleTrainingCheck(false);

	PerformKeyRebindings();
	//CheckForNewQuests();

	// Asking for these will create them if they dont already exist.
	//GetCasualLobbyPanel()->InvalidateLayout(false, true);
	//GetCompLobbyPanel()->InvalidateLayout(false, true);
	//GetMvMLobbyPanel()->InvalidateLayout(false, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadCharacterImageFile(void)
{
	if (!m_bBackgroundUsesCharacterImages)
		return;

	m_pCharacterImagePanel = dynamic_cast<vgui::ImagePanel*>(FindChildByName("TFCharacterImage"));
	if (m_pCharacterImagePanel)
	{
		KeyValues* pCharacterFile = new KeyValues("CharacterBackgrounds");

		if (pCharacterFile->LoadFromFile(g_pFullFileSystem, "scripts/CharacterBackgrounds.txt"))
		{
			CUtlVector<KeyValues*> vecUseableCharacters;

			const char* pszActiveWarName = NULL;
			const WarDefinitionMap_t& mapWars = GetItemSchema()->GetWarDefinitions();
			FOR_EACH_MAP_FAST(mapWars, i)
			{
				const CWarDefinition* pWarDef = mapWars[i];
				if (pWarDef->IsActive())
				{
					pszActiveWarName = pWarDef->GetDefName();
					break;
				}
			}

			// Count the number of possible characters.
			FOR_EACH_SUBKEY(pCharacterFile, pCharacter)
			{
				EHoliday eHoliday = (EHoliday)UTIL_GetHolidayForString(pCharacter->GetString("holiday_restriction"));
				const char* pszAssociatedWar = pCharacter->GetString("war_restriction");

				int iWeight = 1;

				// If a War is active, that's all we want to show.  If not, then bias towards holidays
				if (pszActiveWarName != NULL)
				{
					if (!FStrEq(pszAssociatedWar, pszActiveWarName))
					{
						iWeight = 0;
					}
				}
				else if (eHoliday != kHoliday_None)
				{
					iWeight = UTIL_IsHolidayActive(eHoliday) ? 6 : 0;
				}

				for (int i = 0; i < iWeight; i++)
				{
					vecUseableCharacters.AddToTail(pCharacter);
				}
			}

			// Pick a character at random.
			if (m_iCharacterImageIdx < 0 && vecUseableCharacters.Count() > 0)
			{
				m_iCharacterImageIdx = rand() % vecUseableCharacters.Count();
			}

			// Make sure we found a character we can use.
			if (vecUseableCharacters.IsValidIndex(m_iCharacterImageIdx))
			{
				KeyValues* pCharacter = vecUseableCharacters[m_iCharacterImageIdx];

				if (IsFreeTrialAccount() && m_pHighlightAnims[MMHA_STORE] && !m_bPlayListExpanded)
				{
					const char* text = pCharacter->GetString("store_text");
					if (text)
					{
						m_pHighlightAnims[MMHA_STORE]->SetDialogVariable("highlighttext", g_pVGuiLocalize->Find(text));
						StartHighlightAnimation(MMHA_STORE);
					}
				}

				const char* image_name = pCharacter->GetString("image");
				m_pCharacterImagePanel->SetImage(image_name);
			}
		}

		pCharacterFile->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadMenuEntries(void)
{
	KeyValues* datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences(true);	// VGUI uses escape sequences
	bool bLoaded = datafile->LoadFromFile(g_pFullFileSystem, "Resource/GameMenuOld.res", "custom_mod");
	if (!bLoaded)
	{
		bLoaded = datafile->LoadFromFile(g_pFullFileSystem, "Resource/GameMenuOld.res", "vgui");
		if (!bLoaded)
		{
			// only allow to load loose files when using insecure mode
			//if (CommandLine()->FindParm("-insecure"))
			//{
				bLoaded = datafile->LoadFromFile(g_pFullFileSystem, "Resource/GameMenuOld.res");
			//}
		}
	}

	for (KeyValues* dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char* label = dat->GetString("label", "<unknown>");
		const char* cmd = dat->GetString("command", NULL);
		const char* name = dat->GetName();
		int iStyle = dat->GetInt("style", 0);

		if (!cmd || !cmd[0])
		{
			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = NULL;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool("OnlyInGame");
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool("OnlyInReplay");
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool("OnlyAtMenu");
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool("OnlyWhenVREnabled");
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			continue;
		}

		// Create the new editable panel (first, see if we have one already)
		vgui::EditablePanel* pPanel = dynamic_cast<vgui::EditablePanel*>(FindChildByName(name, true));
		if (!pPanel)
		{
			Assert(false);	// We don't want to do this anymore.  We need an actual hierarchy so things can slide
			// around when the play buttin is pressed and the play options expand
			pPanel = new vgui::EditablePanel(this, name);
		}
		else
		{
			// It already exists in our .res file. Note that it's a custom button.
			iStyle = MMBS_CUSTOM;
		}

		if (pPanel)
		{
			if (m_pButtonKV && iStyle != MMBS_CUSTOM)
			{
				pPanel->ApplySettings(m_pButtonKV);
			}

			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = pPanel;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool("OnlyInGame");
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool("OnlyInReplay");
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool("OnlyAtMenu");
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool("OnlyWhenVREnabled");
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			m_pMMButtonEntries[iIdx].pszImage = dat->GetString("subimage");
			m_pMMButtonEntries[iIdx].pszTooltip = dat->GetString("tooltip", NULL);

			// Tell the button that we'd like messages from it
			CExImageButton* pButton = dynamic_cast<CExImageButton*>(pPanel->FindChildByName("SubButton"));
			if (pButton)
			{
				if (m_pMMButtonEntries[iIdx].pszTooltip)
				{
					pButton->SetTooltip(m_pToolTip, m_pMMButtonEntries[iIdx].pszTooltip);
				}

				pButton->SetText(label);
				pButton->SetCommand(cmd);
				pButton->SetMouseInputEnabled(true);
				pButton->AddActionSignalTarget(GetVPanel());

				if (m_pMMButtonEntries[iIdx].pszImage && m_pMMButtonEntries[iIdx].pszImage[0])
				{
					pButton->SetSubImage(m_pMMButtonEntries[iIdx].pszImage);
				}
			}
		}

		OnUpdateMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::RemoveAllMenuEntries(void)
{
	FOR_EACH_VEC_BACK(m_pMMButtonEntries, i)
	{
		if (m_pMMButtonEntries[i].pPanel)
		{
			// Manually remove anything that's not going to be removed automatically
			if (m_pMMButtonEntries[i].pPanel->IsBuildModeDeletable() == false)
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
void CHudMainMenuOverride::PerformLayout(void)
{
	BaseClass::PerformLayout();

	bool bFirstButton = true;

	int iYPos = m_iButtonY;
	FOR_EACH_VEC(m_pMMButtonEntries, i)
	{
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if (!bIsVisible)
			continue;

		if (bFirstButton && m_pMMButtonEntries[i].pPanel != NULL)
		{
			m_pMMButtonEntries[i].pPanel->NavigateTo();
			bFirstButton = false;
		}

		// Don't reposition it if it's a custom button
		if (m_pMMButtonEntries[i].iStyle == MMBS_CUSTOM)
			continue;

		// If we're a spacer, just leave a blank and move on
		if (m_pMMButtonEntries[i].pPanel == NULL)
		{
			iYPos += YRES(20);
			continue;
		}

		m_pMMButtonEntries[i].pPanel->SetPos((GetWide() * 0.5) + m_iButtonXOffset, iYPos);
		iYPos += m_pMMButtonEntries[i].pPanel->GetTall() + m_iButtonYDelta;
	}

	if (m_pFeaturedItemMouseOverPanel->IsVisible())
	{
		m_pFeaturedItemMouseOverPanel->SetVisible(false);
	}

	if (m_pEventPromoContainer && m_pSafeModeContainer)
	{
		m_pEventPromoContainer->SetVisible(!cl_mainmenu_safemode.GetBool());
		m_pSafeModeContainer->SetVisible(cl_mainmenu_safemode.GetBool());
		if (cl_mainmenu_safemode.GetBool())
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pSafeModeContainer, "MMenu_SafeMode_Blink");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel(m_pSafeModeContainer);
		}
	}

	// Make the glows behind the update buttons pulse
	if (m_pEventPromoContainer && cl_mainmenu_updateglow.GetInt())
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >("Background", true);
		if (pUpdateBackground)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(pUpdateBackground, "MMenu_UpdateButton_StartGlow");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnUpdateMenu(void)
{
	// The dumb gameui.dll basepanel calls this every damn frame it's visible.
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
	if (m_bReapplyButtonKVs)
	{
		m_bReapplyButtonKVs = false;

		if (m_pButtonKV)
		{
			FOR_EACH_VEC(m_pMMButtonEntries, i)
			{
				if (m_pMMButtonEntries[i].iStyle != MMBS_CUSTOM && m_pMMButtonEntries[i].pPanel)
				{
					m_pMMButtonEntries[i].pPanel->ApplySettings(m_pButtonKV);
				}
			}
		}
	}

	// Hide the character if we're in game.
	if (bInGame || bInReplay)
	{
		if (m_pCharacterImagePanel && m_pCharacterImagePanel->IsVisible())
		{
			m_pCharacterImagePanel->SetVisible(false);
		}
	}
	else if (!bInGame && !bInReplay)
	{
		if (m_pCharacterImagePanel && !m_pCharacterImagePanel->IsVisible())
		{
			m_pCharacterImagePanel->SetVisible(true);
		}
	}

	// Position the entries
	FOR_EACH_VEC(m_pMMButtonEntries, i)
	{
		bool shouldBeVisible = true;
		if (m_pMMButtonEntries[i].bOnlyInGame && !bInGame)
		{
			shouldBeVisible = false;
		}
		else if (m_pMMButtonEntries[i].bOnlyInReplay && !bInReplay)
		{
			shouldBeVisible = false;
		}
		else if (m_pMMButtonEntries[i].bOnlyAtMenu && (bInGame || bInReplay))
		{
			shouldBeVisible = false;
		}
		else if (m_pMMButtonEntries[i].bOnlyVREnabled && (!bIsVREnabled || ShouldForceVRActive()))
		{
			shouldBeVisible = false;
		}

		// Set the right visibility
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if (bIsVisible != shouldBeVisible)
		{
			m_pMMButtonEntries[i].bIsVisible = shouldBeVisible;
			if (m_pMMButtonEntries[i].pPanel)
			{
				m_pMMButtonEntries[i].pPanel->SetVisible(shouldBeVisible);
			}
			bSomethingChanged = true;
		}
	}

	if (m_pQuitButton && m_pDisconnectButton && m_pBackToReplaysButton)
	{
		bool bShowQuit = !(bInGame || bInReplay);
		bool bShowDisconnect = bInGame && !bInReplay;

		if (m_pQuitButton->IsVisible() != bShowQuit)
		{
			m_pQuitButton->SetVisible(bShowQuit);
		}

		if (m_pBackToReplaysButton->IsVisible() != bInReplay)
		{
			m_pBackToReplaysButton->SetVisible(bInReplay);
		}

		if (m_pDisconnectButton->IsVisible() != bShowDisconnect)
		{
			m_pDisconnectButton->SetVisible(bShowDisconnect);
		}

		if (bShowDisconnect)
		{
			bool bIsDisconnectText = GTFGCClientSystem()->GetCurrentServerAbandonStatus() != k_EAbandonGameStatus_AbandonWithPenalty;
			if (m_bIsDisconnectText != bIsDisconnectText)
			{
				m_bIsDisconnectText = bIsDisconnectText;
				m_pDisconnectButton->SetText(m_bIsDisconnectText ? "#GameUI_GameMenu_Disconnect" : "#TF_MM_Rejoin_Abandon");
			}
		}
	}

	if (m_pBackground)
	{
		if (cl_mainmenu_operation_motd_reset.GetBool() && cl_mainmenu_operation_motd_start.GetBool())
		{
			cl_mainmenu_operation_motd_start.SetValue(0);
			cl_mainmenu_operation_motd_reset.SetValue(0);
		}

		if (!cl_mainmenu_operation_motd_start.GetInt())
		{
			char sztime[k_RTimeRenderBufferSize];
			CRTime::RTime32ToString(CRTime::RTime32TimeCur(), sztime);
			cl_mainmenu_operation_motd_start.SetValue(sztime);
		}

		bool bShouldBeVisible = bInGame == false;
		if (m_pBackground->IsVisible() != bShouldBeVisible)
		{
			m_pBackground->SetVisible(bShouldBeVisible);

			// Always show this on startup when we have a new campaign
			if (m_bStabilizedInitialLayout && bShouldBeVisible && (m_bHaveNewMOTDs || !m_bMOTDShownAtStartup))
			{
				RTime32 rtFirstLaunchTime = CRTime::RTime32FromString(cl_mainmenu_operation_motd_start.GetString());
				RTime32 rtThreeDaysFromStart = CRTime::RTime32DateAdd(rtFirstLaunchTime, 7, k_ETimeUnitDay);
				if (m_bHaveNewMOTDs || CRTime::RTime32TimeCur() < rtThreeDaysFromStart)
				{
					SetMOTDVisible(true);
					m_bMOTDShownAtStartup = true;
				}
			}
		}
	}

	if (bSomethingChanged)
	{
		InvalidateLayout();

		ScheduleItemCheck();
	}

	if (!bInGame && m_flCheckTrainingAt && m_flCheckTrainingAt < engine->Time())
	{
		m_flCheckTrainingAt = 0;
		CheckTrainingStatus();
	}

	if (!bInGame && m_flCheckUnclaimedItems && m_flCheckUnclaimedItems < engine->Time())
	{
		m_flCheckUnclaimedItems = 0;
		CheckUnclaimedItems();
	}

#ifdef SAXXYMAINMENU_ENABLED
	const bool bSaxxyShouldBeVisible = !bInGame && !bInReplay;
	if (!m_pSaxxyAwardsPanel && bSaxxyShouldBeVisible)
	{
		m_pSaxxyAwardsPanel = new CSaxxyAwardsPanel(this, "SaxxyPanel");

		if (m_pSaxxySettings)
		{
			m_pSaxxyAwardsPanel->ApplySettings(m_pSaxxySettings);
		}

		m_pSaxxyAwardsPanel->InvalidateLayout(true, true);
	}
	else if (m_pSaxxyAwardsPanel && !bSaxxyShouldBeVisible)
	{
		m_pSaxxyAwardsPanel->MarkForDeletion();
		m_pSaxxyAwardsPanel = NULL;
	}
#endif

	if (m_pVRModeButton && m_pVRModeButton->IsVisible())
	{
		if (UseVR())
			m_pVRModeButton->SetText("#MMenu_VRMode_Deactivate");
		else
			m_pVRModeButton->SetText("#MMenu_VRMode_Activate");
	}

	if (!IsLayoutInvalid())
	{
		m_bStabilizedInitialLayout = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Check to see if we need to hound the player about unclaimed items.
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::CheckUnclaimedItems()
{
	// Only do this if we don't have a notification about unclaimed items already.
	for (int i = 0; i < NotificationQueue_GetNumNotifications(); i++)
	{
		CEconNotification* pNotification = NotificationQueue_Get(i);
		if (pNotification)
		{
			if (!Q_strcmp(pNotification->GetUnlocalizedText(), "TF_HasNewItems"))
			{
				return;
			}
		}
	}

	// Only provide a notification if there are items to pick up.
	if (TFInventoryManager()->GetNumItemPickedUpItems() == 0)
		return;

	TFInventoryManager()->GetLocalTFInventory()->NotifyHasNewItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnConfirm(KeyValues* pParams)
{
	if (pParams->GetBool("confirmed"))
	{
		engine->ClientCmd_Unrestricted("disconnect");
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine training_showdlg");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateMOTD(bool bNewMOTDs)
{
	if (m_bInitMOTD == false)
	{
		m_pMOTDPanel->InvalidateLayout(true, true);
		m_bInitMOTD = true;
	}

	if (bNewMOTDs)
	{
		m_bHaveNewMOTDs = true;
		m_iCurrentMOTD = -1;
	}

	int iCount = GetMOTDManager().GetNumMOTDs();
	if (!iCount || m_iCurrentMOTD < 0)
	{
		m_iCurrentMOTD = (iCount - 1);
	}

	// If we don't have an MOTD selected, show the most recent one
	CMOTDEntryDefinition* pMOTD = GetMOTDManager().GetMOTDByIndex(m_iCurrentMOTD);
	if (pMOTD)
	{
		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage(uilanguage, sizeof(uilanguage));
		ELanguage nCurLang = PchLanguageToELanguage(uilanguage);

		RTime32 nTime = pMOTD->GetPostTime();
		wchar_t wzDate[64];

		char rgchDateBuf[128];
		BGetLocalFormattedDate(nTime, rgchDateBuf, sizeof(rgchDateBuf));

		// Start with the day ("Aug 21")
		CRTime cTime(nTime);
		g_pVGuiLocalize->ConvertANSIToUnicode(rgchDateBuf, wzDate, sizeof(wzDate));

		m_pMOTDPanel->SetDialogVariable("motddate", wzDate);

		// Header Color and text
		if (m_pMOTDHeaderLabel)
		{
			m_pMOTDHeaderLabel->SetText(pMOTD->GetHeaderTitle(nCurLang));
			int iHeaderType = pMOTD->GetHeaderType();
			switch (iHeaderType)
			{
			case 0:
				m_pMOTDHeaderLabel->SetBgColor(Color(183, 108, 58, 255));
				break;
			case 1:
				m_pMOTDHeaderLabel->SetBgColor(Color(141, 178, 61, 255));
				break;
			default:
				m_pMOTDHeaderLabel->SetBgColor(Color(183, 108, 58, 255));
				break;
			}
		}

		if (m_pMOTDHeaderIcon)
		{
			// Header Class icon
			if (pMOTD->GetHeaderIcon() == NULL || Q_strcmp(pMOTD->GetHeaderIcon(), "") == 0)
			{
				m_pMOTDHeaderIcon->SetVisible(false);
			}
			else
			{
				m_pMOTDHeaderIcon->SetVisible(true);
				m_pMOTDHeaderIcon->SetImage(pMOTD->GetHeaderIcon());
			}
		}

		// Set the Title and change font until it fits
		// title
		int iTitleWide = 0;
		int iTitleTall = 0;

		int iLabelWide = 0;
		int iLabelTall = 0;

		wchar_t wszText[512];
		g_pVGuiLocalize->ConvertANSIToUnicode(pMOTD->GetTitle(nCurLang), wszText, sizeof(wszText));

		if (m_hTitleLabelFont != vgui::INVALID_FONT)
		{
			surface()->GetTextSize(m_hTitleLabelFont, wszText, iTitleWide, iTitleTall);
		}

		if (m_pMOTDTitleLabel)
		{
			m_pMOTDTitleLabel->GetSize(iLabelWide, iLabelTall);

			if (iTitleWide > iLabelWide)
			{
				IScheme* pScheme = scheme()->GetIScheme(m_pMOTDTitleLabel->GetScheme());

				int hMediumBoldFont = pScheme->GetFont("HudFontMediumBold");
				surface()->GetTextSize(hMediumBoldFont, wszText, iTitleWide, iTitleTall);
				if (iTitleWide > iLabelWide)
				{
					m_pMOTDTitleLabel->SetFont(pScheme->GetFont("HudFontMediumSmallBold"));
				}
				else
				{
					m_pMOTDTitleLabel->SetFont(hMediumBoldFont);
				}
			}
			else
			{
				if (m_hTitleLabelFont != vgui::INVALID_FONT)
				{
					m_pMOTDTitleLabel->SetFont(m_hTitleLabelFont);
				}
			}
		}
		m_pMOTDPanel->SetDialogVariable("motdtitle", pMOTD->GetTitle(nCurLang));

		// Body Text
		m_pMOTDTextPanel->SetDialogVariable("motdtext", pMOTD->GetText(nCurLang));

		// Image
		const char* pszImage = pMOTD->GetImage();

		if (m_pMOTDTitleImage)
		{
			m_pMOTDTitleImage->SetShouldScaleImage(false);
			if (pszImage == NULL || Q_strcmp(pszImage, "") == 0 || Q_strcmp(pszImage, "class_icons/filter_all_on") == 0)
			{
				m_pMOTDTitleImage->SetImage("../logo/new_tf2_logo");
			}
			else
			{
				m_pMOTDTitleImage->SetImage(pszImage);
			}

			IImage* pImage = m_pMOTDTitleImage->GetImage();
			int iContentWide = 0;
			int iContentTall = 0;
			if (m_pMOTDTitleImageContainer)
			{
				m_pMOTDTitleImageContainer->GetSize(iContentWide, iContentTall);
			}

			int iImgWide;
			int iImgTall;
			pImage->GetSize(iImgWide, iImgTall);

			// get the size of the content
			// perform a uniform scale along the horizontal
			float fImageScale = MIN((float)iContentWide / (float)iImgWide, 1.0f);
			float fScaledTall = iImgTall * fImageScale;
			float fScaledWide = iImgWide * fImageScale;
			pImage->SetSize(fScaledWide, fScaledTall);

			// reposition the image so that its centered
			m_pMOTDTitleImage->SetPos((iContentWide - fScaledWide) / 2, (iContentTall - fScaledTall) / 2);
		}

		// We need to resize our text label to fit all the text
		if (m_pMOTDTextLabel)
		{
			m_pMOTDTextLabel->InvalidateLayout(true);

			int wide, tall;
			m_pMOTDTextLabel->GetContentSize(wide, tall);
			m_pMOTDTextLabel->SetSize(m_pMOTDTextPanel->GetWide(), tall);
			m_pMOTDTextPanel->SetSize(m_pMOTDTextPanel->GetWide(), m_pMOTDTextLabel->GetTall());
		}

		if (m_pMOTDURLButton)
		{
			const char* pszURL = pMOTD->GetURL();
			m_pMOTDURLButton->SetVisible((pszURL && pszURL[0]));
		}
		if (m_pMOTDPrevButton)
		{
			m_pMOTDPrevButton->SetEnabled(m_iCurrentMOTD > 0);
			m_pMOTDPrevButton->SetSubImage(m_iCurrentMOTD > 0 ? "blog_back" : "blog_back_disabled");
		}
		if (m_pMOTDNextButton)
		{
			m_pMOTDNextButton->SetEnabled(m_iCurrentMOTD < (iCount - 1));
			m_pMOTDNextButton->SetSubImage(m_iCurrentMOTD < (iCount - 1) ? "blog_forward" : "blog_forward_disabled");
		}

		// Move our scrollbar to the top.
		m_pMOTDTextScroller->InvalidateLayout();
		m_pMOTDTextScroller->Repaint();
		m_pMOTDTextScroller->GetScrollbar()->SetValue(0);
		m_pMOTDTextScroller->GetScrollbar()->SetVisible(m_pMOTDTextPanel->GetTall() > m_pMOTDTextScroller->GetScrollbar()->GetTall());
		m_pMOTDTextScroller->GetScrollbar()->InvalidateLayout();
		m_pMOTDTextScroller->GetScrollbar()->Repaint();
	}
	else
	{
		// Hide the MOTD, and the button to show it.
		SetMOTDVisible(false);

		if (m_pMOTDShowPanel)
		{
			m_pMOTDShowPanel->SetVisible(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetMOTDButtonVisible(bool bVisible)
{
	if (bVisible && m_pMOTDPanel && m_pMOTDPanel->IsVisible())
		return;

	if (m_pMOTDShowPanel)
	{
		// Show the notifications show panel button if we have new notifications.
		m_pMOTDShowPanel->SetVisible(bVisible);

		if (bVisible && m_bHaveNewMOTDs)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pMOTDShowPanel, "HasMOTDBlink");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pMOTDShowPanel, "HasMOTDBlinkStop");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetMOTDVisible(bool bVisible)
{
	m_pMOTDPanel->SetVisible(bVisible);

	if (bVisible)
	{
		// Ensure the text is correct.
		UpdateMOTD(false);

		// Clear MOTD button.
		SetMOTDButtonVisible(true);
		SetNotificationsPanelVisible(false);
		//SetQuestLogVisible(false);
		//SetWatchStreamVisible(false);
		//SetNotificationsButtonVisible( false );

		// Consider new MOTDs as having been viewed.
		m_bHaveNewMOTDs = false;
	}
	else
	{
		SetMOTDButtonVisible(true);
		UpdateNotifications();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void CHudMainMenuOverride::SetQuestLogVisible(bool bVisible)
//{
//	GetQuestLog()->ShowPanel(bVisible);
//
//	if (bVisible)
//	{
//		SetMOTDVisible(false);
//		SetNotificationsPanelVisible(false);
//		SetWatchStreamVisible(false);
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void CHudMainMenuOverride::SetWatchStreamVisible(bool bVisible)
//{
//	m_pWatchStreamsPanel->SetVisible(bVisible);
//
//	if (bVisible)
//	{
//		SetMOTDVisible(false);
//		SetNotificationsPanelVisible(false);
//		SetQuestLogVisible(false);
//	}
//}

bool CHudMainMenuOverride::CheckAndWarnForPREC(void)
{
	enum check_state
	{
		INVALID,
		FOUND,
		NOT_FOUND,
	};

	static check_state s_state = INVALID;
	if (s_state == INVALID)
	{
		s_state = NOT_FOUND;

		ICvar::Iterator iter(g_pCVar);
		for (iter.SetFirst(); iter.IsValid(); iter.Next())
		{
			ConCommandBase* cmd = iter.Get();
			if (cmd)
			{
				if (!Q_strncmp(cmd->GetName(), "prec_", 5))
				{
					s_state = FOUND;
					break;
				}
			}
		}
	}

	if (s_state == FOUND)
	{
		ShowMessageBox("#TF_Incompatible_AddOn", "#TF_PREC_Loaded");
	}

	return (s_state == FOUND);
}

//void CHudMainMenuOverride::OpenMvMMMPanel()
//{
//	if (CheckAndWarnForPREC())
//		return;
//
//	GetMvMLobbyPanel()->ShowPanel(true);
//}
//
//void CHudMainMenuOverride::OpenCompMMPanel()
//{
//	if (CheckAndWarnForPREC())
//		return;
//
//	GetCompLobbyPanel()->ShowPanel(true);
//}
//
//void CHudMainMenuOverride::OpenCasualMMPanel()
//{
//	if (CheckAndWarnForPREC())
//		return;
//
//	GetCasualLobbyPanel()->ShowPanel(true);
//}

//CLobbyContainerFrame_Comp* CHudMainMenuOverride::GetCompLobbyPanel()
//{
//	static CLobbyContainerFrame_Comp* pCompPanel = NULL;
//	if (pCompPanel == NULL)
//	{
//		pCompPanel = SETUP_PANEL(new CLobbyContainerFrame_Comp());
//	}
//
//	return pCompPanel;
//}
//
//CLobbyContainerFrame_MvM* CHudMainMenuOverride::GetMvMLobbyPanel()
//{
//	static CLobbyContainerFrame_MvM* pMvMPanel = NULL;
//	if (pMvMPanel == NULL)
//	{
//		pMvMPanel = SETUP_PANEL(new CLobbyContainerFrame_MvM());
//	}
//
//	return pMvMPanel;
//}

//CLobbyContainerFrame_Casual* CHudMainMenuOverride::GetCasualLobbyPanel()
//{
//	static CLobbyContainerFrame_Casual* pCasualPanel = NULL;
//	if (pCasualPanel == NULL)
//	{
//		pCasualPanel = SETUP_PANEL(new CLobbyContainerFrame_Casual());
//	}
//
//	return pCasualPanel;
//}

//void CHudMainMenuOverride::ReloadMMPanels()
//{
//	if (GetCasualLobbyPanel()->IsVisible())
//	{
//		GetCasualLobbyPanel()->InvalidateLayout(true, true);
//		GetCasualLobbyPanel()->ShowPanel(true);
//	}
//
//	if (GetCompLobbyPanel()->IsVisible())
//	{
//		GetCompLobbyPanel()->InvalidateLayout(true, true);
//		GetCompLobbyPanel()->ShowPanel(true);
//	}
//
//	if (GetMvMLobbyPanel()->IsVisible())
//	{
//		GetMvMLobbyPanel()->InvalidateLayout(true, true);
//		GetMvMLobbyPanel()->ShowPanel(true);
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateNotifications()
{
	int iNumNotifications = NotificationQueue_GetNumNotifications();

	wchar_t wszNumber[16] = L"";
	V_swprintf_safe(wszNumber, L"%i", iNumNotifications);
	wchar_t wszText[1024] = L"";
	g_pVGuiLocalize->ConstructString_safe(wszText, g_pVGuiLocalize->Find("#MMenu_Notifications_Show"), 1, wszNumber);

	m_pNotificationsPanel->SetDialogVariable("notititle", wszText);

	bool bHasNotifications = iNumNotifications != 0;

	if (m_pNotificationsShowPanel)
	{
		SetNotificationsButtonVisible(bHasNotifications);

		bool bBlinkNotifications = bHasNotifications && m_pNotificationsShowPanel->IsVisible();
		if (bBlinkNotifications)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pNotificationsShowPanel, "HasNotificationsBlink");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pNotificationsShowPanel, "HasNotificationsBlinkStop");
		}
	}

	if (!bHasNotifications)
	{
		SetNotificationsButtonVisible(false);
		SetNotificationsPanelVisible(false);
	}

	AdjustNotificationsPanelHeight();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsButtonVisible(bool bVisible)
{
	if (bVisible && (m_pNotificationsPanel && m_pNotificationsPanel->IsVisible()))
		return;

	if (m_pNotificationsShowPanel)
	{
		// Show the notifications show panel button if we have new notifications.
		m_pNotificationsShowPanel->SetVisible(bVisible);

		// Set the notification count variable.
		if (m_pNotificationsShowPanel)
		{
			m_pNotificationsShowPanel->SetDialogVariable("noticount", NotificationQueue_GetNumNotifications());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsPanelVisible(bool bVisible)
{
	if (m_pNotificationsPanel)
	{
		bool bHasNotifications = NotificationQueue_GetNumNotifications() != 0;

		if (bHasNotifications)
		{
			UpdateNotifications();
		}

		m_pNotificationsPanel->SetVisible(bVisible);

		if (bVisible)
		{
			m_pNotificationsScroller->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->SetValue(0);

			SetMOTDVisible(false);
			//SetQuestLogVisible(false);
			//SetWatchStreamVisible(false);

			m_pNotificationsShowPanel->SetVisible(false);

			m_pNotificationsControl->OnTick();
			m_pNotificationsControl->PerformLayout();
			AdjustNotificationsPanelHeight();

			// Faster updating while open.
			vgui::ivgui()->RemoveTickSignal(GetVPanel());
			vgui::ivgui()->AddTickSignal(GetVPanel(), 5);
		}
		else
		{
			// Clear all notifications.
			if (bHasNotifications)
			{
				SetNotificationsButtonVisible(true);
			}

			// Slower updating while closed.
			vgui::ivgui()->RemoveTickSignal(GetVPanel());
			vgui::ivgui()->AddTickSignal(GetVPanel(), 250);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AdjustNotificationsPanelHeight()
{
	// Fit to our contents, which may change without notifying us.
	int iNotiTall = m_pNotificationsControl->GetTall();
	if (iNotiTall > m_pNotificationsScroller->GetTall())
		iNotiTall = m_pNotificationsScroller->GetTall();
	int iTargetTall = YRES(40) + iNotiTall;
	if (m_pNotificationsPanel->GetTall() != iTargetTall)
		m_pNotificationsPanel->SetTall(iTargetTall);

	// Adjust visibility of the slider buttons and our width, as contents change.
	if (m_pNotificationsScroller)
	{
		if (m_pNotificationsScroller->GetScrollbar()->GetSlider() &&
			m_pNotificationsScroller->GetScrollbar()->GetSlider()->IsSliderVisible())
		{
			m_pNotificationsPanel->SetWide(m_iNotiPanelWide + m_pNotificationsScroller->GetScrollbar()->GetSlider()->GetWide());
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible(true);
		}
		else
		{
			m_pNotificationsPanel->SetWide(m_iNotiPanelWide);
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdatePromotionalCodes(void)
{
	// should we show the promo codes button?
	vgui::Panel* pPromoCodesButton = FindChildByName("ShowPromoCodesButton");
	if (pPromoCodesButton)
	{
		bool bShouldBeVisible = false;
		if (steamapicontext && steamapicontext->SteamUser())
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			GCSDK::CGCClientSharedObjectCache* pSOCache = GCClientSystem()->GetSOCache(steamID);
			if (pSOCache)
			{
				GCSDK::CGCClientSharedObjectTypeCache* pTypeCache = pSOCache->FindTypeCache(k_EEconTypeClaimCode);
				bShouldBeVisible = pTypeCache != NULL && pTypeCache->GetCount() != 0;
			}
		}

		// The promo code button collides with the VR mode button. Turn off the promo code button 
		// in that case since the people who deliberately enabled VR are much more likely to want that
		// than to claim their Well Spun Hat in Rift.
		bool bShowVR = materials->GetCurrentConfigForVideoCard().m_nVRModeAdapter == materials->GetCurrentAdapter();
		if (bShowVR)
		{
			bShouldBeVisible = false;
		}

		// has the player turned off this button?
		if (!cl_promotional_codes_button_show.GetBool())
		{
			bShouldBeVisible = false;
		}

		if (pPromoCodesButton->IsVisible() != bShouldBeVisible)
		{
			pPromoCodesButton->SetVisible(bShouldBeVisible);
		}

		if (m_pVRModeBackground)
		{
			m_pVRModeBackground->SetVisible(bShouldBeVisible || bShowVR);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMainMenuOverride::IsVisible(void)
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
void CHudMainMenuOverride::StartHighlightAnimation(mm_highlight_anims iAnim)
{
	vgui::surface()->PlaySound("ui/hint.wav");

	if (m_pHighlightAnims[iAnim])
	{
		m_pHighlightAnims[iAnim]->Popup();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::HideHighlight(mm_highlight_anims iAnim)
{
	if (m_pHighlightAnims[iAnim])
	{
		m_pHighlightAnims[iAnim]->Hide(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void CHudMainMenuOverride::TogglePlayListMenu(void)
//{
//	if (m_bPlayListExpanded)
//	{
//		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "MMenu_PlayList_Collapse", false);
//	}
//	else
//	{
//		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "MMenu_PlayList_Expand", false);
//		UpdatePlaylistEntries();
//	}
//
//	// These all rely on the playlist being in a specific state.  If we're 
//	// toggling, then there's no guarantees anything is where we think it is anymore
//	HideHighlight(MMHA_TUTORIAL);
//	HideHighlight(MMHA_PRACTICE);
//	HideHighlight(MMHA_LOADOUT);
//	HideHighlight(MMHA_STORE);
//	HideHighlight(MMHA_WAR);
//
//	m_bPlayListExpanded = !m_bPlayListExpanded;
//
//	CheckTrainingStatus();
//}

void PromptOrFireCommand(const char* pszCommand)
{
	if (engine->IsInGame())
	{
		CTFDisconnectConfirmDialog* pDialog = BuildDisconnectConfirmDialog();
		if (pDialog)
		{
			pDialog->Show();
			pDialog->AddConfirmCommand(pszCommand);
		}
	}
	else
	{
		engine->ClientCmd_Unrestricted(pszCommand);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make the glows behind the update buttons stop pulsing
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::StopUpdateGlow()
{
	// Dont ever glow again
	if (cl_mainmenu_updateglow.GetInt())
	{
		cl_mainmenu_updateglow.SetValue(0);
		engine->ClientCmd_Unrestricted("host_writeconfig");
	}

	if (m_pEventPromoContainer)
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >("Background", true);
		if (pUpdateBackground)
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence(pUpdateBackground, "MMenu_UpdateButton_StartGlow");
			pUpdateBackground->SetControlVisible("ViewDetailsGlow", false, true);
			pUpdateBackground->SetControlVisible("ViewWarButtonGlow", false, true);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnCommand(const char* command)
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent("on_command(main_menu_override)", command);

	//if (FStrEq("toggle_play_menu", command))
	//{
	//	TogglePlayListMenu();
	//	return;
	//}
	//else if (FStrEq("play_competitive", command))
	//{
	//	// Defaulting to 6v6
	//	GTFGCClientSystem()->SetLadderType(k_nMatchGroup_Ladder_6v6);
	//	PromptOrFireCommand("OpenMatchmakingLobby ladder");
	//	return;
	//}
	//else if (FStrEq("play_casual", command))
	//{
	//	// Defaulting to 12v12
	//	GTFGCClientSystem()->SetLadderType(k_nMatchGroup_Casual_12v12);
	//	PromptOrFireCommand("OpenMatchmakingLobby casual");
	//	return;
	//}
	//else 
	if (FStrEq("play_mvm", command))
	{
		PromptOrFireCommand("OpenMatchmakingLobby mvm");
		return;
	}
	else if (FStrEq("play_quickplay", command))
	{
		PromptOrFireCommand("OpenQuickplayDialog");
		return;
	}
	else if (FStrEq("play_training", command))
	{
		HideHighlight(MMHA_TUTORIAL);

		if (engine->IsInGame())
		{
			const char* pText = "#TF_Training_Prompt";
			const char* pTitle = "#TF_Training_Prompt_Title";
			if (TFGameRules() && TFGameRules()->IsInTraining())
			{
				pTitle = "#TF_Training_Restart_Title";
				pText = "#TF_Training_Restart_Text";
			}
			CTFConfirmTrainingDialog* pConfirm = vgui::SETUP_PANEL(new CTFConfirmTrainingDialog(pText, pTitle, this));
			if (pConfirm)
			{
				pConfirm->Show();
			}
		}
		else
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine training_showdlg");
		}
	}
	else if (Q_strnicmp(command, "soundentry", 10) == 0)
	{
		PlaySoundEntry(command + 11);
		return;
	}
	else if (!Q_stricmp(command, "motd_viewurl"))
	{
		CMOTDEntryDefinition* pMOTD = GetMOTDManager().GetMOTDByIndex(m_iCurrentMOTD);
		if (pMOTD)
		{
			const char* pszURL = pMOTD->GetURL();
			if (pszURL && pszURL[0])
			{
				if (steamapicontext && steamapicontext->SteamFriends())
				{
					steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(pszURL);
				}
			}
		}
		return;
	}
	else if (!Q_stricmp(command, "view_newuser_forums"))
	{
		HideHighlight(MMHA_NEWUSERFORUM);

		if (steamapicontext && steamapicontext->SteamFriends())
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("http://forums.steampowered.com/forums/forumdisplay.php?f=906");
		}
		return;
	}
	else if (!Q_stricmp(command, "opentf2options"))
	{
		HideHighlight(MMHA_OPTIONS);

		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine opentf2options");
	}
	else if (!Q_stricmp(command, "motd_prev"))
	{
		if (m_iCurrentMOTD > 0)
		{
			m_iCurrentMOTD--;
			UpdateMOTD(false);
		}
		return;
	}
	else if (!Q_stricmp(command, "motd_next"))
	{
		if (m_iCurrentMOTD < (GetMOTDManager().GetNumMOTDs() - 1))
		{
			m_iCurrentMOTD++;
			UpdateMOTD(false);
		}
		return;
	}
	else if (!Q_stricmp(command, "motd_show"))
	{
		SetMOTDVisible(!m_pMOTDPanel->IsVisible());
	}
	else if (!Q_stricmp(command, "motd_hide"))
	{
		SetMOTDVisible(false);
	}
	else if (!Q_stricmp(command, "noti_show"))
	{
		SetNotificationsPanelVisible(true);
	}
	else if (!Q_stricmp(command, "noti_hide"))
	{
		SetNotificationsPanelVisible(false);
	}
	else if (!Q_stricmp(command, "notifications_update"))
	{
		// force visible if 
		if (NotificationQueue_GetNumNotifications() != 0)
		{
			SetNotificationsButtonVisible(true);
		}
		else
		{
			UpdateNotifications();
		}
	}
	else if (!Q_stricmp(command, "test_anim"))
	{
		InvalidateLayout(true, true);

		StartHighlightAnimation(MMHA_TUTORIAL);
		StartHighlightAnimation(MMHA_PRACTICE);
		StartHighlightAnimation(MMHA_NEWUSERFORUM);
		StartHighlightAnimation(MMHA_OPTIONS);
		StartHighlightAnimation(MMHA_STORE);
		StartHighlightAnimation(MMHA_LOADOUT);
		StartHighlightAnimation(MMHA_WAR);
	}
	else if (!Q_stricmp(command, "offlinepractice"))
	{
		HideHighlight(MMHA_PRACTICE);

		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine training_showdlg");
	}
	else if (!Q_stricmp(command, "buyfeatured"))
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand(VarArgs("engine open_store %d 1", m_pFeaturedItemPanel ? m_pFeaturedItemPanel->GetItem()->GetItemDefIndex() : 0));
	}
	else if (!Q_stricmp(command, "armory_open"))
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine open_charinfo_armory");
	}
	else if (!Q_stricmp(command, "engine disconnect") && engine->IsInGame() && TFGameRules() && (TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode()))
	{
		// If we're playing MvM, "New Game" should take us back to MvM matchmaking
		CTFDisconnectConfirmDialog* pDialog = BuildDisconnectConfirmDialog();
		if (pDialog)
		{
			pDialog->Show();
		}
		return;
	}
	else if (!Q_stricmp(command, "callvote"))
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("engine callvote");
		if (GetClientModeTFNormal()->GameUI())
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand("ResumeGame");
		}
		return;
	}
	else if (!Q_stricmp(command, "showpromocodes"))
	{
		if (steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils())
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch (GetUniverse())
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(CFmtStr1024("http://steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64())); break;
			case k_EUniverseBeta:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(CFmtStr1024("http://beta.steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64())); break;
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(CFmtStr1024("http://localhost/community/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64())); break;
			}
		}
	}
	else if (!Q_stricmp(command, "exitreplayeditor"))
	{
#if defined( REPLAY_ENABLED )
		CReplayPerformanceEditorPanel* pEditor = ReplayUI_GetPerformanceEditor();
		if (!pEditor)
			return;

		pEditor->Exit_ShowDialogs();
#endif // REPLAY_ENABLED
	}
	else if (FStrEq("showcomic", command))
	{
		if (m_pWarLandingPage)
		{
			m_pWarLandingPage->InvalidateLayout(true, true);
			m_pWarLandingPage->SetVisible(true);
		}
	}
	//else if (FStrEq("questlog", command))
	//{
	//	SetQuestLogVisible(!GetQuestLog()->IsVisible());
	//}
	//else if (FStrEq("watch_stream", command))
	//{
	//	SetWatchStreamVisible(!m_pWatchStreamsPanel->IsVisible());
	//}
	else if (FStrEq("view_update_page", command))
	{
		StopUpdateGlow();

		if (steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled())
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch (GetUniverse())
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("http://www.teamfortress.com/meetyourmatch"); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("http://csham.valvesoftware.com/tf.com/meetyourmatch"); break;
			}
		}
		else
		{
			OpenStoreStatusDialog(NULL, "#MMenu_OverlayRequired", true, false);
		}
		return;
	}
	else if (FStrEq("view_update_comic", command))
	{
		if (steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled())
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch (GetUniverse())
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("http://www.teamfortress.com/gargoyles_and_gravel"); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage("http://www.teamfortress.com/gargoyles_and_gravel"); break;
			}
		}
		else
		{
			OpenStoreStatusDialog(NULL, "#MMenu_OverlayRequired", true, false);
		}
		return;
	}
	else if (FStrEq("view_war", command))
	{
		HideHighlight(MMHA_WAR);
		StopUpdateGlow();

		m_pWarLandingPage->InvalidateLayout(true, true);
		m_pWarLandingPage->SetVisible(true);

		return;
	}
	else if (FStrEq("comp_access_info", command))
	{
		if (m_pCompetitiveAccessInfo)
		{
			m_pCompetitiveAccessInfo->SetVisible(true);
		}
	}
	else if (FStrEq("OpenReportPlayerDialog", command))
	{
		if (!m_hReportPlayerDialog.Get())
		{
			m_hReportPlayerDialog = vgui::SETUP_PANEL(new CReportPlayerDialog(this));
			int x, y, ww, wt, wide, tall;
			vgui::surface()->GetWorkspaceBounds(x, y, ww, wt);
			m_hReportPlayerDialog->GetSize(wide, tall);
	
			// Center it, keeping requested size
			m_hReportPlayerDialog->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
		}
		m_hReportPlayerDialog->Activate();
	}
	else
	{
		// Pass it on to GameUI main menu
		if (GetClientModeTFNormal()->GameUI())
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand(command);
			return;
		}
	}

	BaseClass::OnCommand(command);
}

void CHudMainMenuOverride::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_XBUTTON_B && engine->IsInGame())
	{
		OnCommand("ResumeGame");
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::CheckTrainingStatus(void)
{
	bool bNeedsTraining = tf_training_has_prompted_for_training.GetInt() <= 0;
	bool bNeedsPractice = tf_training_has_prompted_for_offline_practice.GetInt() <= 0;
	bool bShowForum = tf_training_has_prompted_for_forums.GetInt() <= 0;
	bool bShowOptions = tf_training_has_prompted_for_options.GetInt() <= 0;
	bool bWasInTraining = m_bWasInTraining;
	m_bWasInTraining = false;

	bool bShowLoadout = false;
	if (tf_training_has_prompted_for_loadout.GetInt() <= 0)
	{
		// See if we have any items in our inventory.
		int iNumItems = TFInventoryManager()->GetLocalTFInventory()->GetItemCount();
		if (iNumItems > 0)
		{
			bShowLoadout = true;
		}
	}

	if (bShowLoadout && !m_bPlayListExpanded)
	{
		tf_training_has_prompted_for_loadout.SetValue(1);
		StartHighlightAnimation(MMHA_LOADOUT);
	}
	else if (bNeedsTraining && m_bPlayListExpanded)
	{
		tf_training_has_prompted_for_training.SetValue(1);

		if (m_pHighlightAnims[MMHA_TUTORIAL])
		{
			if (UTIL_HasLoadedAnyMap())
			{
				m_pHighlightAnims[MMHA_TUTORIAL]->SetDialogVariable("highlighttext", g_pVGuiLocalize->Find("#MMenu_TutorialHighlight_Title2"));
			}
			else
			{
				m_pHighlightAnims[MMHA_TUTORIAL]->SetDialogVariable("highlighttext", g_pVGuiLocalize->Find("#MMenu_TutorialHighlight_Title"));
			}
		}

		StartHighlightAnimation(MMHA_TUTORIAL);
	}
	else if (bWasInTraining && Training_IsComplete() == false && tf_training_has_prompted_for_training.GetInt() < 2 && m_bPlayListExpanded)
	{
		tf_training_has_prompted_for_training.SetValue(2);

		if (m_pHighlightAnims[MMHA_TUTORIAL])
		{
			m_pHighlightAnims[MMHA_TUTORIAL]->SetDialogVariable("highlighttext", g_pVGuiLocalize->Find("#MMenu_TutorialHighlight_Title3"));
		}

		StartHighlightAnimation(MMHA_TUTORIAL);
	}
	else if (bNeedsPractice && m_bPlayListExpanded)
	{
		tf_training_has_prompted_for_offline_practice.SetValue(1);
		StartHighlightAnimation(MMHA_PRACTICE);
	}
	else if (bShowForum)
	{
		tf_training_has_prompted_for_forums.SetValue(1);
		StartHighlightAnimation(MMHA_NEWUSERFORUM);
	}
	else if (bShowOptions)
	{
		tf_training_has_prompted_for_options.SetValue(1);
		StartHighlightAnimation(MMHA_OPTIONS);
	}
}

/*void CHudMainMenuOverride::CheckForNewQuests(void)
{
	CUtlVector< CEconItemView* > questItems;
	TFInventoryManager()->GetAllQuestItems(&questItems);

	ImagePanel* pImage = m_pQuestLogButton->FindControl< ImagePanel >("SubImage", true);
	if (pImage)
	{
		if (questItems.Count() > 0)
		{
			pImage->SetImage("button_quests");
		}
		else
		{
			pImage->SetImage("button_quests_disabled");
		}
	}

	EditablePanel* pNotiPanel = m_pQuestLogButton->FindControl< EditablePanel >("NotificationsContainer", true);
	if (pNotiPanel)
	{
		// how many quests are unidentified?
		int iUnidentified = 0;
		FOR_EACH_VEC(questItems, i)
		{
			if (IsQuestItemUnidentified(questItems[i]->GetSOCData()))
			{
				iUnidentified++;
			}
		}

		pNotiPanel->SetDialogVariable("noticount", iUnidentified);
		pNotiPanel->SetVisible(iUnidentified > 0);
	}
}

void CHudMainMenuOverride::UpdatePlaylistEntries(void)
{
	CMainMenuPlayListEntry::EDisabledStates_t eDisabledState = CMainMenuPlayListEntry::NOT_DISABLED;

	CTFParty* pParty = GTFGCClientSystem()->GetParty();
	if ((pParty && pParty->BOffline()) || !GTFGCClientSystem()->BConnectedtoGC() || GTFGCClientSystem()->BHasOutstandingMatchmakingPartyMessage())
	{
		eDisabledState = CMainMenuPlayListEntry::DISABLED_NO_GC;
	}

	// If we have a live match, and a we're not in it, but we should be in,
	// dont let the user click the MM UI buttons.  GTFGCClientSystem::Update() will nag them 
	// to rejoin their match or abandon.
	if (pParty && pParty->GetState() == CSOTFParty_State_IN_MATCH)
	{
		eDisabledState = CMainMenuPlayListEntry::DISABLED_MATCH_RUNNING;
	}

	CMainMenuPlayListEntry* pEntry = FindControl< CMainMenuPlayListEntry >("CasualEntry", true);
	if (pEntry)
	{
		pEntry->SetDisabledReason(eDisabledState);
	}

	pEntry = FindControl< CMainMenuPlayListEntry >("MvMEntry", true);
	if (pEntry)
	{
		pEntry->SetDisabledReason(eDisabledState);
	}

	pEntry = FindControl< CMainMenuPlayListEntry >("CompetitiveEntry", true);
	if (pEntry)
	{
		// Only check competitive access last
		if (eDisabledState == CMainMenuPlayListEntry::NOT_DISABLED)
		{
			eDisabledState = !GTFGCClientSystem()->BHasCompetitiveAccess() ? CMainMenuPlayListEntry::DISABLED_NO_COMP_ACCESS : eDisabledState;
		}
		pEntry->SetDisabledReason(eDisabledState);
	}
}

void CHudMainMenuOverride::SOEvent(const CSharedObject* pObject)
{
	if (pObject->GetTypeID() == CEconGameAccountClient::k_nTypeID)
	{
		UpdatePlaylistEntries();
	}

	if (pObject->GetTypeID() != CEconItem::k_nTypeID)
		return;

	CEconItem* pEconItem = (CEconItem*)pObject;

	// If the item is a competitive pass - update the main menu lock
	// From _items_main.txt
	const item_definition_index_t kCompetitivePassID = 1167;
	if (pEconItem->GetItemDefIndex() == kCompetitivePassID)
	{
		CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
		if (pMMPanel)
		{
			pMMPanel->UpdatePlaylistEntries();
		}
	}

}*/



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
void CHudMainMenuOverride::PerformKeyRebindings(void)
{
	REMAP_COMMAND(inspect, +inspect);
	REMAP_COMMAND(taunt, +taunt);
	REMAP_COMMAND(use_action_slot_item, +use_action_slot_item);
	REMAP_COMMAND(use_action_slot_item_server, +use_action_slot_item_server);
}

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the MOTD request response
//-----------------------------------------------------------------------------
class CGCMOTDRequestResponse : public GCSDK::CGCClientJob
{
public:
	CGCMOTDRequestResponse(GCSDK::CGCClient* pClient) : GCSDK::CGCClientJob(pClient) {}

	virtual bool BYieldingRunGCJob(GCSDK::IMsgNetPacket* pNetPacket)
	{
		GCSDK::CGCMsg<MsgGCMOTDRequestResponse_t> msg(pNetPacket);

		// No new entries?
		if (!msg.Body().m_nEntries)
			return true;

		// No main menu panel?
		CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
		if (!pMMPanel)
			return true;

		// Get our local language
		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage(uilanguage, sizeof(uilanguage));

		//V_strcpy_safe( uilanguage, "german" );

		KeyValues* pEntriesKV = new KeyValues("motd_entries");

		// Try and load the cache file. If we fail, we'll just create a new one.
		if (!pMMPanel->ReloadedAllMOTDs())
		{
			pEntriesKV->LoadFromFile(g_pFullFileSystem, GC_MOTD_CACHE_FILE);
		}

		bool bNewMOTDs = false;

		// Store the time & language we last checked.
		char rtime_buf[k_RTimeRenderBufferSize];
		pEntriesKV->SetString("last_request_time", CRTime::RTime32ToString(pMMPanel->GetLastMOTDRequestTime(), rtime_buf));
		pEntriesKV->SetString("last_request_language", GetLanguageShortName(pMMPanel->GetLastMOTDRequestLanguage()));

		// Read in the entries one by one, and insert them into our keyvalues structure.
		for (int i = 0; i < msg.Body().m_nEntries; i++)
		{
			char pchMsgString[2048];

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;

			// If there's already an entry with this index, overwrite the data
			KeyValues* pNewEntry = pEntriesKV->FindKey(pchMsgString);
			if (!pNewEntry)
			{
				pNewEntry = new KeyValues(pchMsgString);
				pEntriesKV->AddSubKey(pNewEntry);
			}
			pNewEntry->SetName(pchMsgString);

			RTime32 iTime;
			if (!msg.BReadUintData(&iTime))
				return false;
			pNewEntry->SetString("post_time", CRTime::RTime32ToString(iTime, rtime_buf));

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString(VarArgs("title_%s", uilanguage), pchMsgString);

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString(VarArgs("text_%s", uilanguage), pchMsgString);

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString("url", pchMsgString);

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString("image", pchMsgString);

			int iHeadertype;
			if (!msg.BReadIntData(&iHeadertype))
				return false;
			pNewEntry->SetString("header_type", CFmtStr("%d", iHeadertype).Access());

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString(VarArgs("header_%s", uilanguage), pchMsgString);

			if (!msg.BReadStr(pchMsgString, Q_ARRAYSIZE(pchMsgString)))
				return false;
			pNewEntry->SetString("header_icon", pchMsgString);

			bNewMOTDs = true;
		}

		// Tell the schema to reload its MOTD block
		CUtlVector< CUtlString > vecErrors;
		pMMPanel->GetMOTDManager().BInitMOTDEntries(pEntriesKV, &vecErrors);
		pMMPanel->GetMOTDManager().PurgeUnusedMOTDEntries(pEntriesKV);

		// Save out our cache
		pEntriesKV->SaveToFile(g_pFullFileSystem, GC_MOTD_CACHE_FILE);

		// And tell the main menu to refresh the MOTD.
		//pMMPanel->SetMOTDVisible( bNewMOTDs ); HACK!  Temporarily turn this off!
		pMMPanel->UpdateMOTD(bNewMOTDs);
		return true;
	}
};

GC_REG_JOB(GCSDK::CGCClient, CGCMOTDRequestResponse, "CGCMOTDRequestResponse", k_EMsgGCMOTDRequestResponse, GCSDK::k_EServerTypeGCClient);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainMenuToolTip::PerformLayout()
{
	if (!ShouldLayout())
		return;

	_isDirty = false;

	// Resize our text labels to fit.
	int iW = 0;
	int iH = 0;
	for (int i = 0; i < m_pEmbeddedPanel->GetChildCount(); i++)
	{
		vgui::Label* pLabel = dynamic_cast<vgui::Label*>(m_pEmbeddedPanel->GetChild(i));
		if (!pLabel)
			continue;

		// Only checking to see if we have any text
		char szTmp[2];
		pLabel->GetText(szTmp, sizeof(szTmp));
		if (!szTmp[0])
			continue;

		pLabel->InvalidateLayout(true);

		int iX, iY;
		pLabel->GetPos(iX, iY);
		iW = MAX(iW, (pLabel->GetWide() + (iX * 2)));

		if (iH == 0)
		{
			iH += MAX(iH, pLabel->GetTall() + (iY * 2));
		}
		else
		{
			iH += MAX(iH, pLabel->GetTall());
		}
	}
	m_pEmbeddedPanel->SetSize(iW, iH);

	m_pEmbeddedPanel->SetVisible(true);

	PositionWindow(m_pEmbeddedPanel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainMenuToolTip::HideTooltip()
{
	if (m_pEmbeddedPanel)
	{
		m_pEmbeddedPanel->SetVisible(false);
	}

	BaseTooltip::HideTooltip();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainMenuToolTip::SetText(const char* pszText)
{
	if (m_pEmbeddedPanel)
	{
		_isDirty = true;

		if (pszText && pszText[0] == '#')
		{
			m_pEmbeddedPanel->SetDialogVariable("tiptext", g_pVGuiLocalize->Find(pszText));
		}
		else
		{
			m_pEmbeddedPanel->SetDialogVariable("tiptext", pszText);
		}
		m_pEmbeddedPanel->SetDialogVariable("tipsubtext", "");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload the .res file
//-----------------------------------------------------------------------------
#if defined( STAGING_ONLY )
ConVar tf_icon_festive("tf_icon_festive", 0);
CON_COMMAND(mainmenu_refresh, "")
{
	CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
	if (!pMMPanel)
		return;

	pMMPanel->InvalidateLayout(true, true);
}

CON_COMMAND(create_icons, "Generate 512 x 512 Paint Kit Item Icons for SteamMarket, Specify min and max itemdef ranges if desired")
{
	tf_icon_festive.SetValue(false);
	CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
	if (!pMMPanel)
		return;

	int min = args.ArgC() > 1 ? atoi(args[1]) : -1;
	int max = args.ArgC() > 2 ? atoi(args[2]) : -1;

	pMMPanel->GenerateIcons(false, min, max);
}

CON_COMMAND(create_icons_large, "Generate 1024 x 1024 Paint Kit Item Icons for Testing, Specify min and max itemdef ranges if desired")
{
	tf_icon_festive.SetValue(false);
	CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
	if (!pMMPanel)
		return;

	int min = args.ArgC() > 1 ? atoi(args[1]) : -1;
	int max = args.ArgC() > 2 ? atoi(args[2]) : -1;;

	pMMPanel->GenerateIcons(true, min, max);
}

CON_COMMAND(create_icons_festive, "")
{
	tf_icon_festive.SetValue(true);

	CHudMainMenuOverride* pMMPanel = (CHudMainMenuOverride*)gViewPortInterface->FindPanelByName(PANEL_MAINMENUOVERRIDE);
	if (!pMMPanel)
		return;

	int min = args.ArgC() > 1 ? atoi(args[1]) : -1;
	int max = args.ArgC() > 2 ? atoi(args[2]) : -1;

	pMMPanel->GenerateIcons(false, min, max);
}

//-----------------------------------------------------------------------------
void BuildPaintkitItemInventoryImagePath(char* pchOutfile, int nMaxPath, const CTFItemDefinition* pItemDef, int iWear, bool bLargeTestIcons)
{
	//	CUtlString strDefName( pItemDef->GetDefinitionName() );
		//strDefName = strDefName.Replace( ' ', '_' );
		//strDefName.ToLower();
	//	const char *pchDefName = strDefName;

	bool bIsPaintkitItem = pItemDef->GetCustomPainkKitDefinition() != NULL;
	const char* pchOutputFolder;
	if (bIsPaintkitItem)
	{
		pchOutputFolder = bLargeTestIcons ? "resource/econ/generated_icons/LargeTest/" : "scripts/items/unencrypted/icons/generated_paintkit_icons/";
	}
	else
	{
		pchOutputFolder = "scripts/items/unencrypted/icons/generated_item_icons/";
	}

	//	const char *pWear = bIsPaintkitItem ? CFmtStr( "_wear%d", iWear ) : "";
	char fname[MAX_PATH];
	V_FileBase(pItemDef->GetInventoryImage(), fname, sizeof(fname));

	if (tf_icon_festive.GetBool() == true)
	{
		V_snprintf(pchOutfile, nMaxPath, "%s%s_festive.png",
			pchOutputFolder,
			fname
		);
	}
	else
	{
		V_snprintf(pchOutfile, nMaxPath, "%s%s.png",
			pchOutputFolder,
			fname
		);
	}
}
//-----------------------------------------------------------------------------
bool SaveImageIconAsPng(CEconItemView* pItem, ITexture* pInputTexture, const char* pszFilePath)
{
	bool bRet = false;
	ITexture* pTexture = materials->FindTexture("_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET);

	if (!pTexture)
		return bRet;

	// If this is 3 4, we're only generating the actual composite texture for SFM
	ConVarRef r_texcomp_dump("r_texcomp_dump");
	if (r_texcomp_dump.GetInt() == 4)
	{
		return true;
	}

	if (pTexture->GetImageFormat() == IMAGE_FORMAT_RGBA8888 ||
		pTexture->GetImageFormat() == IMAGE_FORMAT_ABGR8888 ||
		pTexture->GetImageFormat() == IMAGE_FORMAT_ARGB8888 ||
		pTexture->GetImageFormat() == IMAGE_FORMAT_BGRA8888 ||
		pTexture->GetImageFormat() == IMAGE_FORMAT_BGRX8888)
	{
		int width = Min(pInputTexture->GetActualWidth(), pTexture->GetActualWidth());
		int height = Min(pInputTexture->GetActualHeight(), pTexture->GetActualHeight());
		Rect_t SrcRect = { 0, 0, width, height };
		Rect_t DstRect = SrcRect;

		if ((width > 0) && (height > 0))
		{
			void* pixelValue = malloc(width * height * sizeof(RGBA8888_t));

			if (pixelValue)
			{
				CMatRenderContextPtr pRenderContext(materials);

				pRenderContext->PushRenderTargetAndViewport(pTexture, 0, 0, width, height);
				pRenderContext->CopyTextureToRenderTargetEx(0, pInputTexture, &SrcRect, &DstRect);

				pRenderContext->ReadPixels(0, 0, width, height, (unsigned char*)pixelValue, pInputTexture->GetImageFormat());

				CUtlBuffer outBuffer;
				ImgUtl_WriteRGBAAsPNGToBuffer(reinterpret_cast<const unsigned char*>(pixelValue), width, height, outBuffer);

				FileHandle_t hFileOut = g_pFullFileSystem->Open(pszFilePath, "wb");
				if (hFileOut != FILESYSTEM_INVALID_HANDLE)
				{
					Msg("Saved.. %s\n", pszFilePath);
					g_pFullFileSystem->Write(outBuffer.Base(), outBuffer.TellPut(), hFileOut);
					g_pFullFileSystem->Close(hFileOut);
					bRet = true;
				}

				// restore our previous state
				pRenderContext->PopRenderTargetAndViewport();

				free(pixelValue);
			}
		}
	}
	return bRet;
}
//-----------------------------------------------------------------------------
extern ConVar tf_paint_kit_force_wear;

ConVar tf_paint_kit_icon_generating_index("tf_paint_kit_icon_generating_index", 0);

void StartNextImage(CEconItemView* pItemData, CEmbeddedItemModelPanel* pItemModelPanel, const CUtlVector< item_definition_index_t >& vecItemDef, float flCurrentWear)
{
	// Init the item
	item_definition_index_t iDefIndex = vecItemDef[tf_paint_kit_icon_generating_index.GetInt()];
	pItemData->Init(iDefIndex, AE_PAINTKITWEAPON, AE_USE_SCRIPT_VALUE, true);
	pItemData->SetWeaponSkinBase(NULL);
	pItemData->SetWeaponSkinBaseCompositor(NULL);

	bool bIsPaintkitItem = pItemData->GetCustomPainkKitDefinition() != NULL;
	if (bIsPaintkitItem)
	{
		// Set up the wear
		static CSchemaAttributeDefHandle pAttrDef_TextureWear("set_item_texture_wear");
		pItemData->GetAttributeList()->SetRuntimeAttributeValue(pAttrDef_TextureWear, flCurrentWear);

		Msg("Force Setting PaintKit Wear for Icon Generation : Wear Level %d", tf_paint_kit_force_wear.GetInt());
	}
	else
	{
		// force use_model_cache_icon
		static CSchemaAttributeDefHandle pAttrDef_UseModelCacheIcon("use_model_cache_icon");
		uint32 unUseModelCacheIcon = 1;
		pItemData->GetAttributeList()->SetRuntimeAttributeValue(pAttrDef_UseModelCacheIcon, unUseModelCacheIcon);
	}

	// Add festive attr if enabled
	if (tf_icon_festive.GetBool() == true)
	{
		static CSchemaAttributeDefHandle pAttrDef_IsFestivized("is_festivized");
		pItemData->GetAttributeList()->SetRuntimeAttributeValue(pAttrDef_IsFestivized, 1);
	}
	//int iWear = EconWear_ToIntCategory( flCurrentWear );

	// Force set convar for wear
	//tf_paint_kit_force_wear.SetValue( iWear );

	// force image generation to use high res
	pItemData->SetWeaponSkinUseHighRes(true);

	pItemModelPanel->SetItem(pItemData);
	pItemModelPanel->InvalidateLayout(true, true);
}

extern ConVar tf_paint_kit_generating_icons;
void CHudMainMenuOverride::GenerateIconsThink()
{
	CEmbeddedItemModelPanel* pItemModelPanel = dynamic_cast<CEmbeddedItemModelPanel*>(FindChildByName("icon_generator"));
	if (!pItemModelPanel)
		return;

	ITexture* pIcon = pItemModelPanel->GetCachedGeneratedIcon();
	if (!pIcon)
		return;

	Msg("Saving.. [%d] - %s\n", m_pIconData->GetItemDefIndex(), m_pIconData->GetItemDefinition()->GetDefinitionName());

	// Generate filepath
	char outfile[MAX_PATH];
	BuildPaintkitItemInventoryImagePath(outfile, sizeof(outfile), m_pIconData->GetItemDefinition(), tf_paint_kit_force_wear.GetInt(), m_bGeneratingLargeTestIcons);
	SaveImageIconAsPng(m_pIconData, pIcon, outfile);

	// Generate next step
	while (true)
	{
		int iIndex = tf_paint_kit_icon_generating_index.GetInt();
		iIndex++;

		// Increment Index, if index is greater, increment wear
		if (iIndex >= m_vecIconDefs.Count())
		{
			iIndex = 0;
			if (tf_paint_kit_force_wear.GetInt() == 5)
			{
				// reset bg color
				SetBgColor(Color(0, 0, 0, 0));
				m_bGeneratingIcons = false;
				Msg("Icon Generating Completed");
				tf_paint_kit_generating_icons.SetValue(0);
				tf_paint_kit_icon_generating_index.SetValue(0);
				return;
			}
			else
			{
				tf_paint_kit_force_wear.SetValue(tf_paint_kit_force_wear.GetInt() + 1);
			}
		}

		tf_paint_kit_icon_generating_index.SetValue(iIndex);

		// only render non-paintkit item one time
		if (tf_paint_kit_force_wear.GetInt() > 0)
		{
			// search for the next paintkit item to draw
			item_definition_index_t iDefIndex = m_vecIconDefs[tf_paint_kit_icon_generating_index.GetInt()];
			CEconItemView temp;
			temp.Init(iDefIndex, AE_PAINTKITWEAPON, AE_USE_SCRIPT_VALUE, true);
			if (temp.GetCustomPainkKitDefinition())
			{
				// draw this one
				break;
			}
		}
	}

	// start next item, other wise increment wear and go again
	delete m_pIconData;
	m_pIconData = new CEconItemView;
	StartNextImage(m_pIconData, pItemModelPanel, m_vecIconDefs, 0.2f);
}

ConVar tf_icon_bgcolor_override("tf_icon_bgcolor_override", "");
ConVar tf_icon_allow_all_items("tf_icon_allow_all_items", "0");
void CHudMainMenuOverride::GenerateIcons(bool bLarge, int min /*= -1*/, int max /*= -1*/)
{
	CEmbeddedItemModelPanel* pItemModelPanel = dynamic_cast<CEmbeddedItemModelPanel*>(FindChildByName("icon_generator"));
	if (!pItemModelPanel)
		return;

	const char* pszBGColor = tf_icon_bgcolor_override.GetString();
	if (pszBGColor && *pszBGColor)
	{
		color32 bgcolor;
		UTIL_StringToColor32(&bgcolor, pszBGColor);
		SetBgColor(Color(bgcolor.r, bgcolor.g, bgcolor.b, 255));
	}


	const char* pchOutputFolder = bLarge ? "resource/econ/generated_icons/LargeTest/" : "scripts/items/unencrypted/icons/generated_paintkit_icons";
	g_pFullFileSystem->CreateDirHierarchy(pchOutputFolder, NULL);

	pItemModelPanel->SetTall(1024);
	pItemModelPanel->SetWide(1024);
	//pItemModelPanel->SetZPos( 1000 );

	pItemModelPanel->SetInventoryImageType(CEmbeddedItemModelPanel::IMAGETYPE_LARGE);
	pItemModelPanel->SetVisible(true);
	pItemModelPanel->m_bOfflineIconGeneration = true;

	int rtSize = bLarge ? 1024 : 512;

	// Create a larger render target
	materials->OverrideRenderTargetAllocation(true);

	materials->CreateNamedRenderTargetTextureEx2(
		"offline_icon_generation",
		rtSize, rtSize,
		RT_SIZE_DEFAULT,
		materials->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		0);
	materials->OverrideRenderTargetAllocation(false);

	// Populate list of item icons
	m_vecIconDefs.RemoveAll();
	const CEconItemSchema::SortedItemDefinitionMap_t& mapItems = GetItemSchema()->GetSortedItemDefinitionMap();

	int nPaintkitItems = 0;
	int nNormalItems = 0;

	FOR_EACH_MAP(mapItems, idxItem)
	{
		CEconItemDefinition* pItem = mapItems[idxItem];
		CTFItemDefinition* pTFDef = (CTFItemDefinition*)pItem;
		item_definition_index_t iDefIndex = pTFDef->GetDefinitionIndex();

		// skip numbers below min
		if (min != -1 && iDefIndex < min)
			continue;

		// skip numbers if above.  do not 'break;' since mapItems may NOT be in defindex order
		if (max != -1 && iDefIndex > max)
			continue;

		if (pTFDef->GetCustomPainkKitDefinition())
		{
			m_vecIconDefs.AddToTail(iDefIndex);

			nPaintkitItems++;
		}
		else if (tf_icon_allow_all_items.GetBool())
		{
			m_vecIconDefs.AddToTail(iDefIndex);

			nNormalItems++;
		}
	}

	if (!m_vecIconDefs.Count())
	{
		Msg("Didn't find any valid itemdefs to generate icons for");
		return;
	}

	Msg("Found %d Valid Item defs. %d normal items and %d paintkit items\n", m_vecIconDefs.Count(), nNormalItems, nPaintkitItems);

	int nNumGeneratingIcons = nNormalItems + nPaintkitItems * 5;
	Msg("Generating Icons for %d Items and 5 Wear Levels for each paintkit items (%d icons total)\n", m_vecIconDefs.Count(), nNumGeneratingIcons);


	// Starting conditions
	tf_paint_kit_force_wear.SetValue(1);
	tf_paint_kit_generating_icons.SetValue(1);

	// Create a dummy item
	m_pIconData = new CEconItemView;
	StartNextImage(m_pIconData, pItemModelPanel, m_vecIconDefs, 0.2);

	m_bGeneratingIcons = true;
	m_bGeneratingLargeTestIcons = bLarge;
}
#endif