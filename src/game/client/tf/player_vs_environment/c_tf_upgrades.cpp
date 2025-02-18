//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "c_tf_upgrades.h"
#include "tf_upgrades_shared.h"
#include "dt_utlvector_recv.h"
#include <vgui/ILocalize.h>
#include <iinput.h>
#include "iclientmode.h"
#include "item_model_panel.h"
#include "econ_item_system.h"
#include "tf_weaponbase_gun.h"
#include "c_tf_buff_banner.h"
#include "tf_item_powerup_bottle.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Image.h"
#include "tf_hud_playerstatus.h"
#include "tf_gamerules.h"
#include "econ_item_description.h"
#include "charinfo_loadout_subpanel.h"
#include "c_tf_objective_resource.h"
#include "vgui/IInput.h"
#include "tf_tips.h"
#include "econ_ui.h"
#include "econ_item_preset.h"
#include "gc_clientsystem.h"
#include "achievementmgr.h"
#include "tf_hud_statpanel.h"
#include "tf_mann_vs_machine_stats.h"
#include "c_tf_playerresource.h"

#define UPGRADE_PANEL_LEVEL_LABEL_COUNT 10


extern CAchievementMgr g_AchievementMgrTF;

ConVar tf_mvm_tabs_discovered( "tf_mvm_tabs_discovered", "0", FCVAR_ARCHIVE, "Remember how many times players have clicked tabs." );

Color CUpgradeBuyPanel::m_rgbaDefaultFG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaDefaultBG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaArmedFG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaArmedBG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaDepressedFG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaDepressedBG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaSelectedFG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaSelectedBG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaDisabledFG( 0, 0, 0, 255 );
Color CUpgradeBuyPanel::m_rgbaDisabledBG( 0, 0, 0, 255 );


CUpgradeBuyPanel::CUpgradeBuyPanel( Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
{
	m_pIcon = new vgui::ImagePanel( this, "Icon" );
	m_pPriceLabel = new vgui::Label( this, "PriceLabel", "" );
	m_pShortDescriptionLabel = new vgui::Label( this, "ShortDescriptionLabel", "" );
	m_pIncrementButton = new CImageButton( this, "IncrementButton" );
	m_pDecrementButton = new CImageButton( this, "DecrementButton" );

	m_pSkillTreeButtonKVs = NULL;

	m_nPurchases = 0;

	m_nGridPositionX = 0;
	m_nGridPositionY = 0;

	m_bInspectMode = false;
}

CUpgradeBuyPanel::~CUpgradeBuyPanel()
{
	if ( m_pSkillTreeButtonKVs )
	{
		m_pSkillTreeButtonKVs->deleteThis();
		m_pSkillTreeButtonKVs = NULL;
	}
}

void CUpgradeBuyPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/UpgradeBuyPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	//m_pPriceLabel->SetMouseInputEnabled( false );

	m_rgbaDefaultFG = pScheme->GetColor( "UpgradeDefaultFg", m_rgbaDefaultFG );
	m_rgbaDefaultBG = pScheme->GetColor( "UpgradeDefaultBg", m_rgbaDefaultBG );
	m_rgbaArmedFG = pScheme->GetColor( "UpgradeArmedFg", m_rgbaArmedFG );
	m_rgbaArmedBG = pScheme->GetColor( "UpgradeArmedBg", m_rgbaArmedBG );
	m_rgbaDepressedFG = pScheme->GetColor( "UpgradeDepressedFg", m_rgbaDepressedFG );
	m_rgbaDepressedBG = pScheme->GetColor( "UpgradeDepressedBg", m_rgbaDepressedBG );
	m_rgbaSelectedFG = pScheme->GetColor( "UpgradeSelectedFg", m_rgbaSelectedFG );
	m_rgbaSelectedBG = pScheme->GetColor( "UpgradeSelectedBg", m_rgbaSelectedBG );
	m_rgbaDisabledFG = pScheme->GetColor( "UpgradeDisabledFg", m_rgbaDisabledFG );
	m_rgbaDisabledBG = pScheme->GetColor( "UpgradeDisabledBg", m_rgbaDisabledBG );
}

void CUpgradeBuyPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "skilltreebuttons_kv" );
	if ( pItemKV )
	{
		if ( m_pSkillTreeButtonKVs )
		{
			m_pSkillTreeButtonKVs->deleteThis();
		}
		m_pSkillTreeButtonKVs = new KeyValues( "skilltreebuttons_kv" );
		pItemKV->CopySubkeys( m_pSkillTreeButtonKVs );
	}
}

void CUpgradeBuyPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

void CUpgradeBuyPanel::OnCommand( const char *command )
{
	if ( StringHasPrefix( command, "mvm_upgrade" ) )
	{
		GetParent()->GetParent()->OnCommand( command );
		return;
	}
	else if ( StringHasPrefix( command, "mvm_downgrade" ) )
	{
		GetParent()->GetParent()->OnCommand( command );
		return;
	}

	BaseClass::OnCommand( command );
}

bool CUpgradeBuyPanel::ValidateUpgradeStepData( void ) 
{
	m_bOverCap = false;

	if ( !m_hPlayer )
		return false;
	
	int nNumSteps = GetUpgradeStepData( m_hPlayer, m_nWeaponSlot, m_nUpgradeIndex, m_nCurrentStep, m_bOverCap );
	if ( nNumSteps )
	{
		SetNumLevelImages( nNumSteps );
		m_pIcon->SetImage( g_MannVsMachineUpgrades.m_Upgrades[ m_nUpgradeIndex ].szIcon );
		return true;
	}
	return false;
}

void CUpgradeBuyPanel::SetNumLevelImages( int nValues )
{
	if ( nValues >= m_SkillTreeImages.Count() )
	{
		// Add needed images
		for ( int i = m_SkillTreeImages.Count(); i < nValues; ++i )
		{
			vgui::ImagePanel *pImage = new vgui::ImagePanel( this, "SkillTreeImage" );
			m_SkillTreeImages.AddToTail( pImage );
			pImage->ApplySettings( m_pSkillTreeButtonKVs );
			pImage->SetPos( m_iUpgradeButtonXPos + pImage->GetWide() * i, m_iUpgradeButtonYPos );
		}
	}
	else if ( nValues < m_SkillTreeImages.Count() )
	{
		// Clear out extra images
		for ( int i = m_SkillTreeImages.Count() - 1; i >= nValues; --i )
		{
			m_SkillTreeImages[ i ]->MarkForDeletion();
			m_SkillTreeImages.Remove( i );
		}
	}
}

void CUpgradeBuyPanel::SetSkillTreeButtonColors( int nImage, ColorSet nColorSet )
{
	Assert( nImage >= 0 && nImage < m_SkillTreeImages.Count() );

	vgui::ImagePanel *pImage = m_SkillTreeImages[ nImage ];

	if ( pImage )
	{
		switch ( nColorSet )
		{
		case COLOR_SET_DEFAULT:
		case COLOR_SET_DISABLED:
			pImage->SetImage( "pve/upgrade_unowned" );
			break;

		case COLOR_SET_OWNED:
			pImage->SetImage( "pve/upgrade_owned" );
			break;

		case COLOR_SET_PURCHASED:
			pImage->SetImage( "pve/upgrade_purchased" );
			break;
		}
	}
}

void CUpgradeBuyPanel::UpdateImages( int nCurrentMoney )
{
	// If we're not at max, update our prices and see if our affordability has changed
	costlabel_chache_t nAffordability = CLCACHE_AFFORDABLE;

	int nImageTotal = m_SkillTreeImages.Count();

	int nCurrentStep = m_nCurrentStep + m_nPurchases;

	int nNumRemaining = ( nImageTotal - nCurrentStep );
	int nAffordablePurchases = m_nPrice <= 0 ? INT_MAX : nCurrentMoney / m_nPrice;
	int nUnaffordablePurchases = nNumRemaining - nAffordablePurchases;

	if ( nUnaffordablePurchases > 0 )
	{
		nAffordability = static_cast< costlabel_chache_t >( CLCACHE_NOT_AFFORDABLE_1 + ( nUnaffordablePurchases - 1 ) );
	}

	CMannVsMachineUpgrades *pMannVsMachineUpgrade = &( g_MannVsMachineUpgrades.m_Upgrades[ m_nUpgradeIndex ] );

	bool bSellAllowed = ( TFObjectiveResource()->GetMannVsMachineWaveCount() <= 1 || m_nPurchases > 0 );

	// we'll decided if it's enabled below
	m_pIncrementButton->SetEnabled( false );
	m_pIncrementButton->SetInactiveImage( "pve/buy_disabled" );
	m_pIncrementButton->SetActiveImage( "pve/buy_disabled" );
	m_pIcon->SetImage( VarArgs( "%s_bw", pMannVsMachineUpgrade->szIcon ) );

	if ( m_bInspectMode )
	{
		m_pDecrementButton->SetEnabled( false );
		m_pDecrementButton->SetVisible( false );
		m_pIncrementButton->SetEnabled( true );
		m_pIncrementButton->SetVisible( true );

	}
	else if ( bSellAllowed )
	{
		m_pDecrementButton->SetVisible( true );

		if ( nCurrentStep > 0 )
		{
			m_pDecrementButton->SetEnabled( true );
			m_pDecrementButton->SetInactiveImage( "pve/sell_enabled" );
			m_pDecrementButton->SetActiveImage( "pve/sell_selected" );
		}
		else
		{
			m_pDecrementButton->SetVisible( true );
			m_pDecrementButton->SetEnabled( false );
			m_pDecrementButton->SetInactiveImage( "pve/sell_disabled" );
			m_pDecrementButton->SetActiveImage( "pve/sell_disabled" );
		}
	}
	else
	{
		m_pDecrementButton->SetVisible( false );
	}

	for ( int nUpgradeButton = 0; nUpgradeButton < nImageTotal; ++nUpgradeButton )
	{
		if ( nUpgradeButton < MIN( m_nCurrentStep, nCurrentStep ) )
		{
			// This was an existing upgrade, so show a big dot
			SetSkillTreeButtonColors( nUpgradeButton, CUpgradeBuyPanel::COLOR_SET_OWNED );
		}
		else if ( nUpgradeButton < nCurrentStep )
		{
			// This purchase is in flight so show a smaller dot
			SetSkillTreeButtonColors( nUpgradeButton, CUpgradeBuyPanel::COLOR_SET_PURCHASED );
		}
		else
		{
			if ( nAffordability == CLCACHE_AFFORDABLE || nUpgradeButton >= nCurrentStep + nAffordability )
			{
				SetSkillTreeButtonColors( nUpgradeButton, CUpgradeBuyPanel::COLOR_SET_DEFAULT );

				m_pIncrementButton->SetEnabled( true );
				m_pIncrementButton->SetInactiveImage( "pve/buy_enabled" );
				m_pIncrementButton->SetActiveImage( "pve/buy_selected" );
				m_pIcon->SetImage( pMannVsMachineUpgrade->szIcon );
			}
			else
			{
				SetSkillTreeButtonColors( nUpgradeButton, CUpgradeBuyPanel::COLOR_SET_DISABLED );
			}
		}
	}

	V_strcpy_safe( m_szAttribName, pMannVsMachineUpgrade->szAttrib );
}

//====================================================================================================================
// CHudUpgradePanel - HUD Element that provides the interface to the upgrade options
//====================================================================================================================
DECLARE_HUDELEMENT( CHudUpgradePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudUpgradePanel::CHudUpgradePanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudUpgradePanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_pItemModelPanelKVs = NULL;
	m_bShowUpgradeMenu = false;
	m_bCancelUpgrades = false;
	m_bOpenLoadout = false;

	SetMouseInputEnabled( true );

	MakePopup();

	m_pSelectWeaponPanel = new vgui::EditablePanel( this, "SelectWeaponPanel" );
	m_pTipPanel = new vgui::EditablePanel( this, "TipPanel" );

	m_pUpgradeItemStatsLabel = NULL;
	m_pPlayerUpgradeButton = NULL;
	m_pActiveTabPanel = NULL;
	m_pMouseOverTabPanel = NULL;
	m_pMouseOverUpgradePanel = NULL;
	m_pActiveUpgradeBuyPanel = NULL;
	m_pPlayerRespecButton = NULL;

	m_nCurrency = 0;
	m_nUpgradeActivity = 0;

	m_bHighlightedTab = ( tf_mvm_tabs_discovered.GetInt() > 3 );

	m_bNavUpDownPressed = true;
	m_bNavLeftRightPressed = true;
	m_bNavButtonPressed = true;
	m_bUsingController = false;
	m_bInspectMode = false;
	m_hPlayer = NULL;

	ListenForGameEvent( "player_currency_changed" );
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "upgrades_file_changed" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudUpgradePanel::~CHudUpgradePanel()
{
	if ( IsActive() )
	{
		engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );
	}

	if ( m_pItemModelPanelKVs )
	{
		m_pItemModelPanelKVs->deleteThis();
		m_pItemModelPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudUpgradePanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	UpdateModelPanels();

	CExButton *pButton = dynamic_cast< CExButton* >( m_pSelectWeaponPanel->FindChildByName("CancelButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = dynamic_cast< CExButton* >( m_pSelectWeaponPanel->FindChildByName("CloseButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = dynamic_cast< CExButton* >( m_pSelectWeaponPanel->FindChildByName("PlayerUpgradeButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = dynamic_cast< CExButton* >( m_pSelectWeaponPanel->FindChildByName("QuickEquipButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = dynamic_cast< CExButton* >( m_pSelectWeaponPanel->FindChildByName("LoadoutButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = dynamic_cast< CExButton* >( m_pTipPanel->FindChildByName("NextTipButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	UpdateTip();

	m_pUpgradeItemStatsLabel = dynamic_cast< CExLabel* >( m_pSelectWeaponPanel->FindChildByName("UpgradeItemStatsLabel") );

	m_pPlayerUpgradeButton = m_pSelectWeaponPanel->FindChildByName( "PlayerUpgradeButton" );
	m_pActiveTabPanel = m_pSelectWeaponPanel->FindChildByName( "ActiveTabPanel" );
	m_pMouseOverTabPanel = m_pSelectWeaponPanel->FindChildByName( "MouseOverTabPanel" );
	m_pMouseOverUpgradePanel = m_pSelectWeaponPanel->FindChildByName( "MouseOverUpgradePanel" );
	m_pPlayerRespecButton = m_pSelectWeaponPanel->FindChildByName( "RespecButton" );
	if ( m_pPlayerRespecButton )
	{
		m_pPlayerRespecButton->AddActionSignalTarget( this );
	}
	m_pActiveUpgradeBuyPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::ApplySettings( KeyValues *inResourceData )
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudUpgradePanel::ShouldDraw( void )
{
	if ( !m_bInspectMode )
	{
		m_hPlayer = C_TFPlayer::GetLocalTFPlayer();
	}

	// Local mode
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( m_hPlayer && pLocalPlayer == m_hPlayer )
	{
		if ( !m_hPlayer->IsAlive() || m_hPlayer->m_Shared.IsLoser() || m_hPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			m_bWasInZone = false;
			return false;
		}

		bool bInZone = m_hPlayer->m_Shared.IsInUpgradeZone();

		// check for other popups
		if ( bInZone && !CHudElement::ShouldDraw() ) 
		{
			CTFStatPanel *pStatPanel = GetStatPanel();
			if ( pStatPanel->IsVisible() )
			{
				pStatPanel->Hide();
				return false;
			}
		}

		if ( m_bWasInZone != bInZone )
		{
			if ( bInZone )
			{
				m_bShowUpgradeMenu = true;
				m_bCancelUpgrades = false;
				m_bOpenLoadout = false;
			}
			m_bWasInZone = bInZone;
		}
	}
	
	return m_bShowUpgradeMenu;
}

void CHudUpgradePanel::SetVisible( bool bVisible )
{
	if ( !bVisible && IsActive() )
	{
		engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );
	}

	BaseClass::SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::SetActive( bool bActive )
{
	if ( bActive && !IsActive() )
	{
		// Reset our upgrade activity
		m_nUpgradeActivity = 0;

		m_pActiveUpgradeBuyPanel = NULL;
		m_bNavUpDownPressed = true;
		m_bNavLeftRightPressed = true;
		m_bNavButtonPressed = true;
		m_bUsingController = false;

		KeyValues *kv = new KeyValues( "MvM_UpgradesBegin" );
		engine->ServerCmdKeyValues( kv );
		
		// Get our currency
		if ( m_hPlayer && !m_bInspectMode )
		{
			m_nCurrency = m_hPlayer->GetCurrency();
		}
		else
		{
			m_nCurrency = 0;
		}

		UpdateTip();

		UpgradeItemInSlot( LOADOUT_POSITION_PRIMARY );

		m_pTipPanel->SetVisible( true );
		
		if ( tf_mvm_tabs_discovered.GetInt() >= 3 )
		{
			m_bHighlightedTab = true;
		}

		// Becoming visible. Get ready.
		UpdateModelPanels();

		// Accept button is disabled till you buy or sell something
		m_pSelectWeaponPanel->SetControlEnabled( "CloseButton", false );

		SetKeyBoardInputEnabled( false );
		engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );

		// Move the mouse cursor to the middle of our panel
		int x,y,w,h;
		vgui::ipanel()->GetAbsPos( GetVPanel(), x, y );
		GetSize( w, h );
		//vgui::IInput *input()
		vgui::input()->SetCursorPos( x + (w * 0.5), y + (h * 0.5) );

		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

		// Tick once right away to get the currency updated
		OnTick();

		m_bAwardMaxSlotAchievement = false;
	}
	else if ( !bActive && IsActive() )
	{
		if ( m_bCancelUpgrades )
		{
			CancelUpgrades();
		}

		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

		if ( m_bOpenLoadout )
		{
			engine->ClientCmd_Unrestricted( "open_charinfo_direct" );
		}

		// let the server know that we've close the menu with the number of upgrades
		// so the response rules can do their thing
		KeyValues *kv = new KeyValues( "MvM_UpgradesDone" );
		kv->SetInt( "num_upgrades", m_nUpgradeActivity );
		engine->ServerCmdKeyValues( kv );
	}

	CHudElement::SetActive( bActive );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( IsVisible() )
	{
		UpdateJoystickControls();
		UpdateHighlights();
		UpdateItemStatsLabel();

		if ( m_hPlayer )
		{
			if ( !m_hPlayer->m_Shared.IsInUpgradeZone() && !m_bInspectMode )
			{
				OnCommand( "cancel" );
			}

			if ( m_pPlayerRespecButton && g_TF_PR && m_hPlayer )
			{
				bool bAllowed = TFGameRules() && TFGameRules()->IsMannVsMachineRespecEnabled();
				bool bEnabled = bAllowed && TFGameRules()->CanPlayerUseRespec( m_hPlayer ) && !m_bInspectMode;

				if ( m_pPlayerRespecButton->IsVisible() != bAllowed )
				{
					m_pPlayerRespecButton->SetVisible( bAllowed );
				}
				if ( m_pPlayerRespecButton->IsEnabled() != bEnabled )
				{
					m_pPlayerRespecButton->SetEnabled( bEnabled );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "player_currency_changed" ) )
	{
		if ( IsActive() && !m_bInspectMode )
		{
			int nCurrency = event->GetInt( "currency" );
			if ( m_nCurrency != nCurrency )
			{
				m_nCurrency = nCurrency;
				UpdateButtonStates( m_nCurrency );
			}
		}
	}
	else if ( FStrEq( event->GetName(), "game_newmap" ) )
	{
		// Doing this prevents stale data after changing to a map that uses different upgrades data.
		UpdateModelPanels();
	}
	else if ( FStrEq( event->GetName(), "upgrades_file_changed" ) )
	{
		const char *pszPath = event->GetString( "path" );
		if ( pszPath[0] == '\0' )
		{
			g_MannVsMachineUpgrades.LoadUpgradesFile();
			UpdateModelPanels();
		}
		else
		{
			g_MannVsMachineUpgrades.LoadUpgradesFileFromPath( pszPath );
			UpdateModelPanels();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::PlayerInventoryChanged( C_TFPlayer *pPlayer )
{
	if ( !m_hPlayer || !pPlayer || pPlayer != m_hPlayer )
		return;

	// Go through every item and compare it to the item stored with the matching (slot) panel
	for ( int nSlot = 0; nSlot < CLASS_LOADOUT_POSITION_COUNT; nSlot++ )
	{
		if ( nSlot == LOADOUT_POSITION_HEAD || nSlot == LOADOUT_POSITION_MISC )
			continue;

		CEconItemView *pCurrentItem = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, nSlot );
		if ( !pCurrentItem )
			continue;

		for ( int nPanelIndex = 0; nPanelIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nPanelIndex )
		{
			ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[nPanelIndex] );
			if ( !pItemSlotBuyPanel )
				continue;

			if ( !pItemSlotBuyPanel->upgradeBuyPanels.Count() )
				continue;

			// Do we have a matching panel for this slot?
			if ( pItemSlotBuyPanel->nSlot != nSlot )
				continue;

			// If the item has changed since the panel was created, cancel out of the upgrade screen
			if ( pItemSlotBuyPanel->GetItemID() != pCurrentItem->GetItemID() )
			{
				OnCommand( "cancel" );
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::UpdateModelPanels( void )
{
	m_iVisibleItemPanels = 0;

	FOR_EACH_VEC( m_pItemPanels, i )
	{
		m_pItemPanels[i]->SetVisible( false );
	}

	// slot -1 is used for player self upgrades, -2 is INVALID
	m_ItemSlotBuyPanels[ 0 ].nSlot = ItemSlotBuyPanels::CHARACTER_UPGRADE;
	for ( int iTab = 1; iTab < MAX_ITEM_SLOT_BUY_PANELS; ++iTab )
	{
		m_ItemSlotBuyPanels[ iTab ].nSlot = ItemSlotBuyPanels::INVALID_SLOT;
	}

	if ( m_hPlayer )
	{
		CreateItemModelPanel( LOADOUT_POSITION_PRIMARY );
		CreateItemModelPanel( LOADOUT_POSITION_SECONDARY );
		CreateItemModelPanel( LOADOUT_POSITION_MELEE );

		if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		{
			CreateItemModelPanel( LOADOUT_POSITION_BUILDING );
		}
		else if ( m_hPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			CreateItemModelPanel( LOADOUT_POSITION_PDA );
		}

		Panel *pSentryIcon = m_pSelectWeaponPanel->FindChildByName( "SentryIcon" );
		if ( pSentryIcon )
		{
			pSentryIcon->SetMouseInputEnabled( false );
			pSentryIcon->SetVisible( m_hPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) );
		}

		CreateItemModelPanel( LOADOUT_POSITION_ACTION );
	}

	UpdateUpgradeButtons();
	UpdateItemStatsLabel();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Position the item panels
	int iVisiblePanel = 0;
	FOR_EACH_VEC( m_pItemPanels, i )
	{
		CItemModelPanel *pItemPanel = m_pItemPanels[ i ];

		if ( !pItemPanel )
			continue;

		if ( !pItemPanel->IsVisible() )
			continue;

		int iPanelWide = pItemPanel->GetWide();
		int iLeft = m_iItemPanelXPos + iPanelWide + m_iItemPanelXDelta;
		int iXPos = iLeft + ( iPanelWide + m_iItemPanelXDelta ) * iVisiblePanel;
		int iYPos = m_iItemPanelYPos;
		pItemPanel->SetPos( iXPos, iYPos );

		vgui::Panel *pInactiveTabPanel = m_pSelectWeaponPanel->FindChildByName( VarArgs( "InactiveTabPanel%i", iVisiblePanel + 2 ) );
		if ( pInactiveTabPanel )
		{
			pInactiveTabPanel->SetPos( iXPos, iYPos );
			pInactiveTabPanel->SetVisible( true );
		}

		iVisiblePanel++;
	}

	for ( int i = iVisiblePanel; i < 5; ++i )
	{
		vgui::Panel *pInactiveTabPanel = m_pSelectWeaponPanel->FindChildByName( VarArgs( "InactiveTabPanel%i", i + 2 ) );
		if ( pInactiveTabPanel )
		{
			pInactiveTabPanel->SetVisible( false );
		}
	}

	CTFClassImage *pClassImage = static_cast< CTFClassImage* >( m_pSelectWeaponPanel->FindChildByName( "ClassImage" ) );
	if ( pClassImage )
	{
		pClassImage->SetMouseInputEnabled( false );

		if ( m_hPlayer )
		{
			pClassImage->SetClass( m_hPlayer->GetTeamNumber(), m_hPlayer->GetPlayerClass()->GetClassIndex(), false );
		}
	}

	for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
	{
		ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

		if ( pItemSlotBuyPanel->upgradeBuyPanels.Count() > 0 )
		{
			int iButton = 0;

			FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
			{
				CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];

				pUpgradeBuyPanel->m_nGridPositionX = iButton % 2;
				pUpgradeBuyPanel->m_nGridPositionY = iButton / 2;

				int iXPos = m_iUpgradeBuyPanelXPos + ( pUpgradeBuyPanel->GetWide() + m_iUpgradeBuyPanelDelta ) * pUpgradeBuyPanel->m_nGridPositionX;
				int iYPos = m_iUpgradeBuyPanelYPos + ( pUpgradeBuyPanel->GetTall() + m_iUpgradeBuyPanelDelta ) * pUpgradeBuyPanel->m_nGridPositionY;

				pUpgradeBuyPanel->SetPos( iXPos, iYPos );

				iButton++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel && IsVisible() )
	{
		SetBorderForItem( pItemPanel, true );

		// Get the name of the item that was selected
		CEconItemView *pCurItemData = pItemPanel->GetItem();
		if ( pCurItemData )
		{
			const wchar_t *wszItemName = pCurItemData->GetItemName();

			// Set the text displaying which item the upgrades are for ("Shovel", etc.)
			m_pSelectWeaponPanel->SetDialogVariable( "upgrade_description", wszItemName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel && IsVisible() )
	{
		SetBorderForItem( pItemPanel, false );

		// Set the text displaying which item the upgrades are for ("Shovel", etc.)
		m_pSelectWeaponPanel->SetDialogVariable( "upgrade_description", L"" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::CreateItemModelPanel( int iLoadoutSlot )
{
	if ( !m_hPlayer )
		return;

	int iClass = m_hPlayer->GetPlayerClass()->GetClassIndex();
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
		return;

	CItemModelPanel *pItemPanel = NULL;

	if ( m_iVisibleItemPanels < m_pItemPanels.Count() )
	{
		pItemPanel = m_pItemPanels[ m_iVisibleItemPanels ];
	}

	char szCommand[ 32 ];
	V_snprintf( szCommand, sizeof( szCommand ), "UpgradeButton%d", iLoadoutSlot );

	if ( !pItemPanel )
	{
		// Create our item panel
		pItemPanel = new CItemModelPanel( m_pSelectWeaponPanel, szCommand );
		m_pItemPanels.AddToTail( pItemPanel );

		if ( m_pItemModelPanelKVs )
		{
			pItemPanel->ApplySettings( m_pItemModelPanelKVs );
		} 

		pItemPanel->MakeReadyForUse();
		pItemPanel->SetActAsButton( true, true );
		pItemPanel->SendPanelEnterExits( true );
		pItemPanel->AddActionSignalTarget( this );
	}

	// Get the item entity. We use the entity, not the item in the loadout, because we want
	// the dynamic attributes that have already been purchases and attached.
	CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, iLoadoutSlot );
	pItemPanel->SetItem( pCurItemData );
	pItemPanel->SetGreyedOut( NULL );

	bool bVisible = pCurItemData && pCurItemData->IsValid();
	if ( iLoadoutSlot == LOADOUT_POSITION_ACTION && ( !bVisible || !dynamic_cast< CTFPowerupBottle* >( m_hPlayer->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION ) ) ) )
	{
		static CSchemaItemDefHandle pItemDef_PowerupCanteen( CTFItemSchema::k_rchMvMPowerupBottleItemDefName );

		if ( pItemDef_PowerupCanteen )
		{
			// Nothing in action slot or non-bottle in action slot
			// Show a disabled powerup bottle instead
			CEconItemView *pItemData = new CEconItemView();
			pItemData->Init( pItemDef_PowerupCanteen->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
			pItemData->SetClientItemFlags( kEconItemFlagClient_Preview | kEconItemFlagClient_StoreItem );

			pItemPanel->SetItem( pItemData );
			pItemPanel->SetGreyedOut( "#TF_PVE_Need_Powerup_Equipped" );

			bVisible = pItemData && pItemData->IsValid();
		}
	}

	pItemPanel->SetVisible( bVisible );

	if ( bVisible )
	{
		m_ItemSlotBuyPanels[ m_iVisibleItemPanels + 1 ].nSlot = iLoadoutSlot;

		pItemPanel->SetName( szCommand );
		pItemPanel->SetModelIsHidden( iLoadoutSlot == LOADOUT_POSITION_PDA );

		SetBorderForItem( pItemPanel, false );

		m_iVisibleItemPanels++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::OnItemPanelMousePressed( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( !m_hPlayer )
		return;

	if ( pItemPanel && pItemPanel->GetItem() && IsVisible() && m_pActiveTabPanel )
	{
		int iLoadoutSlot = pItemPanel->GetItem()->GetStaticData()->GetLoadoutSlot( m_hPlayer->GetPlayerClass()->GetClassIndex() );
		if ( m_iWeaponSlotBeingUpgraded == iLoadoutSlot )
			return;

		UpgradeItemInSlot( iLoadoutSlot );

		if ( !m_bHighlightedTab )
		{
			// They've clicked a tab... no need to blink them any longer
			m_bHighlightedTab = true;
			tf_mvm_tabs_discovered.SetValue( tf_mvm_tabs_discovered.GetInt() + 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	extern const char *g_szItemBorders[][5];

	if ( !pItemPanel )
		return;

	const char *pszBorder = g_szItemBorders[0][ bMouseOver ];
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::UpgradeItemInSlot( int iSlot )
{
	if ( !m_hPlayer )
		return;

	if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY ) && iSlot == LOADOUT_POSITION_PRIMARY )
	{
		// Spy doesn't have a primary
		iSlot = LOADOUT_POSITION_SECONDARY;
	}

	m_iWeaponSlotBeingUpgraded = iSlot;

	if ( m_pSelectWeaponPanel )
	{
		m_pSelectWeaponPanel->SetDialogVariable( "upgrade_description", "" );
	}

	if ( m_pMouseOverUpgradePanel )
	{
		for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
		{
			ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

			if ( m_iWeaponSlotBeingUpgraded == pItemSlotBuyPanel->nSlot )
			{
				m_pActiveUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels.Count() > 0 ? pItemSlotBuyPanel->upgradeBuyPanels[ 0 ] : NULL;

				FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
				{
					pItemSlotBuyPanel->upgradeBuyPanels[ i ]->SetVisible( true );	
				}
			}
			else
			{
				FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
				{
					pItemSlotBuyPanel->upgradeBuyPanels[ i ]->SetVisible( false );
				}
			}
		}
	}

	UpdateMouseOverHighlight();

	UpdateButtonStates( m_nCurrency );

	// Get the name of the item that was selected
	const wchar_t *wszItemName = L"";
	if ( m_iWeaponSlotBeingUpgraded == -1 )
	{
		wszItemName = g_pVGuiLocalize->Find( g_aPlayerClassNames[ m_hPlayer->GetPlayerClass()->GetClassIndex() ] );
	}
	else
	{
		CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, m_iWeaponSlotBeingUpgraded );
		if ( pCurItemData )
		{
			wszItemName = pCurItemData->GetItemName();
		}
	}

	// Set the text displaying which item the upgrades are for ("Shovel", etc.)
	m_pSelectWeaponPanel->SetDialogVariable( "upgrade_label", wszItemName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::UpdateUpgradeButtons( void )
{
	if ( !TFGameRules() )
		return;

	if ( !m_hPlayer )
		return;

	for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
	{
		ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

		if ( pItemSlotBuyPanel->nSlot != -2 )
		{
			for ( int nUpgrade = 0; nUpgrade < pItemSlotBuyPanel->upgradeBuyPanels.Count(); ++nUpgrade )
			{
				pItemSlotBuyPanel->upgradeBuyPanels[ nUpgrade ]->MarkForDeletion();
			}

			pItemSlotBuyPanel->upgradeBuyPanels.RemoveAll();

			FOR_EACH_VEC( g_MannVsMachineUpgrades.m_Upgrades, i )
			{
				CMannVsMachineUpgrades *pUpgrade = &(g_MannVsMachineUpgrades.m_Upgrades[ i ]);

				// Don't create button if it belongs to the wrong group
				int nUIGroup = pUpgrade->nUIGroup;

				if ( ( pItemSlotBuyPanel->nSlot == -1 && ( nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM || nUIGroup == UIGROUP_POWERUPBOTTLE ) ) || 
					 ( pItemSlotBuyPanel->nSlot != -1 && nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER ) )
				{
					continue;
				}

				CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( pUpgrade->szAttrib );
				if ( !pAttribDef )
				{
					Warning("BAD ATTRIBUTE IN MVM_UPGRADES: %s\n", pUpgrade->szAttrib );
					continue;
				}

				// Don't create button if it doesn't work for this weapon
				if ( !TFGameRules()->CanUpgradeWithAttrib( m_hPlayer, pItemSlotBuyPanel->nSlot, pAttribDef->GetDefinitionIndex(), pUpgrade ) )
				{
					continue;
				}

				int nCost = pItemSlotBuyPanel->nSlot >= 0 ? 
					TFGameRules()->GetCostForUpgrade( &( g_MannVsMachineUpgrades.m_Upgrades[ i ] ), pItemSlotBuyPanel->nSlot, m_hPlayer->GetPlayerClass()->GetClassIndex(), m_hPlayer ) :
					g_MannVsMachineUpgrades.m_Upgrades[ i ].nCost;

				CUpgradeBuyPanel *pUpgradeBuyPanel = new CUpgradeBuyPanel( m_pSelectWeaponPanel, "UpgradeBuyPanel" );
				pUpgradeBuyPanel->SetPlayer( m_hPlayer );
				pUpgradeBuyPanel->m_nPrice = nCost;
				pUpgradeBuyPanel->m_nUpgradeIndex = i;
				pUpgradeBuyPanel->m_nWeaponSlot = pItemSlotBuyPanel->nSlot;
				pUpgradeBuyPanel->SetInspectMode( ( m_bInspectMode ) ? true : false );

				// Store the item equipped at this time, so we can monitor for a change and mark the panel as dirty
				CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, pItemSlotBuyPanel->nSlot );
				if ( pCurItemData )
				{
					pItemSlotBuyPanel->SetItemID( pCurItemData->GetItemID() );
				}

				if ( !m_bInspectMode )
				{
					pUpgradeBuyPanel->m_pIncrementButton->SetCommand( VarArgs( "mvm_upgrade%03d_%i", pUpgradeBuyPanel->m_nUpgradeIndex, pUpgradeBuyPanel->m_nPrice ) );
					pUpgradeBuyPanel->m_pDecrementButton->SetCommand( VarArgs( "mvm_downgrade%03d_%i", pUpgradeBuyPanel->m_nUpgradeIndex, pUpgradeBuyPanel->m_nPrice ) );
				}
				pUpgradeBuyPanel->SetZPos( 30 );
				pUpgradeBuyPanel->SetVisible( true );
				pUpgradeBuyPanel->InvalidateLayout( true, true );

				if ( !pUpgradeBuyPanel->ValidateUpgradeStepData() )
				{
					pUpgradeBuyPanel->MarkForDeletion();
					continue;
				}

				pItemSlotBuyPanel->upgradeBuyPanels.Insert( pUpgradeBuyPanel );

				float flValue = pUpgrade->flIncrement;
				int iFormat = pAttribDef->GetDescriptionFormat();
				if ( iFormat == ATTDESCFORM_VALUE_IS_PERCENTAGE || iFormat == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE )
				{
					flValue += 1.0;
				}

				CEconAttributeDescription AttrDesc( GLocalizationProvider(), pAttribDef, flValue );
				const wchar_t *pDescription = AttrDesc.GetShortDescription().Get();

				if ( pDescription && pDescription[ 0 ] )
				{
					pUpgradeBuyPanel->m_pShortDescriptionLabel->SetText( pDescription );
				}

				locchar_t wzCost[64];
				loc_sprintf_safe( wzCost, LOCCHAR( "%d" ), pUpgradeBuyPanel->m_nPrice );

				pUpgradeBuyPanel->m_pPriceLabel->SetText( wzCost );
			}
		}
	}

	UpdateButtonStates( m_nCurrency );

	InvalidateLayout();
}

void CHudUpgradePanel::UpdateJoystickControls( void )
{
	static ConVarRef joystick( "joystick" );
	if ( !joystick.IsValid() || !joystick.GetBool() )
	{
		return;
	}

	bool bUp = ::input->Joystick_GetForward() < 0.0f || ::input->Joystick_GetPitch() < 0.0f || vgui::input()->IsKeyDown( KEY_XBUTTON_UP ) || vgui::input()->IsKeyDown( KEY_UP );
	bool bDown = ::input->Joystick_GetForward() > 0.0f || ::input->Joystick_GetPitch() > 0.0f || vgui::input()->IsKeyDown( KEY_XBUTTON_DOWN ) || vgui::input()->IsKeyDown( KEY_DOWN );
	bool bNavUpDownPressed = bUp || bDown;

	bool bLeft = ::input->Joystick_GetSide() < 0.0f || ::input->Joystick_GetYaw() < 0.0f || vgui::input()->IsKeyDown( KEY_XBUTTON_LEFT ) || vgui::input()->IsKeyDown( KEY_LEFT );
	bool bRight = ::input->Joystick_GetSide() > 0.0f || ::input->Joystick_GetYaw() > 0.0f || vgui::input()->IsKeyDown( KEY_XBUTTON_RIGHT ) || vgui::input()->IsKeyDown( KEY_RIGHT );
	bool bNavLeftRightPressed = bLeft || bRight;

	bool bAccept = vgui::input()->IsKeyDown( KEY_XBUTTON_A ) || vgui::input()->IsKeyDown( KEY_ENTER ) || vgui::input()->IsKeyDown( STEAMCONTROLLER_A );
	bool bBack = vgui::input()->IsKeyDown( KEY_XBUTTON_X ) || vgui::input()->IsKeyDown( KEY_BACKSPACE ) || vgui::input()->IsKeyDown( STEAMCONTROLLER_X );
	bool bDone = vgui::input()->IsKeyDown( KEY_XBUTTON_B ) || vgui::input()->IsKeyDown( KEY_ESCAPE ) || vgui::input()->IsKeyDown( STEAMCONTROLLER_B );
	bool bNext = vgui::input()->IsKeyDown( KEY_XBUTTON_RIGHT_SHOULDER ) || vgui::input()->IsKeyDown( KEY_PAGEDOWN );
	bool bPrev = vgui::input()->IsKeyDown( KEY_XBUTTON_LEFT_SHOULDER ) || vgui::input()->IsKeyDown( KEY_PAGEUP );
	bool bNavButtonPressed = bAccept || bBack || bDone || bNext || bPrev;

	if ( m_bNavUpDownPressed )
	{
		if ( !bNavUpDownPressed )
		{
			m_bNavUpDownPressed = false;
		}
	}
	else if ( bNavUpDownPressed )
	{
		m_bNavUpDownPressed = true;
		m_bUsingController = true;

		int nGridPosX = m_pActiveUpgradeBuyPanel ? m_pActiveUpgradeBuyPanel->m_nGridPositionX : 0;
		int nGridPosY = m_pActiveUpgradeBuyPanel ? m_pActiveUpgradeBuyPanel->m_nGridPositionY : 0;

		if ( bUp )
		{
			nGridPosY -= 1;			
		}
		else if ( bDown )
		{
			nGridPosY += 1;
		}

		for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
		{
			ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

			if ( m_iWeaponSlotBeingUpgraded == pItemSlotBuyPanel->nSlot )
			{
				FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
				{
					CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];
					if ( pUpgradeBuyPanel && pUpgradeBuyPanel->IsVisible() && 
						 pUpgradeBuyPanel->m_nGridPositionX == nGridPosX && pUpgradeBuyPanel->m_nGridPositionY == nGridPosY )
					{
						m_pActiveUpgradeBuyPanel = pUpgradeBuyPanel;
						UpdateMouseOverHighlight();
						break;
					}
				}

				break;
			}
		}
	}

	if ( m_bNavLeftRightPressed )
	{
		if ( !bNavLeftRightPressed )
		{
			m_bNavLeftRightPressed = false;
		}
	}
	else if ( bNavLeftRightPressed )
	{
		m_bNavLeftRightPressed = true;
		m_bUsingController = true;

		int nGridPosX = m_pActiveUpgradeBuyPanel ? m_pActiveUpgradeBuyPanel->m_nGridPositionX : 0;
		int nGridPosY = m_pActiveUpgradeBuyPanel ? m_pActiveUpgradeBuyPanel->m_nGridPositionY : 0;

		if ( bLeft )
		{
			nGridPosX -= 1;			
		}
		else if ( bRight )
		{
			nGridPosX += 1;
		}

		for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
		{
			ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

			if ( m_iWeaponSlotBeingUpgraded == pItemSlotBuyPanel->nSlot )
			{
				FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
				{
					CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];
					if ( pUpgradeBuyPanel && pUpgradeBuyPanel->IsVisible() && 
						 pUpgradeBuyPanel->m_nGridPositionX == nGridPosX && pUpgradeBuyPanel->m_nGridPositionY == nGridPosY )
					{
						m_pActiveUpgradeBuyPanel = pUpgradeBuyPanel;
						UpdateMouseOverHighlight();
						break;
					}
				}

				break;
			}
		}
	}

	if ( m_bNavButtonPressed )
	{
		if ( !bNavButtonPressed )
		{
			m_bNavButtonPressed = false;
		}
	}
	else if ( bNavButtonPressed )
	{
		m_bNavButtonPressed = true;
		m_bUsingController = true;

		if ( bAccept )
		{
			if ( m_pActiveUpgradeBuyPanel && m_pActiveUpgradeBuyPanel->m_pIncrementButton && m_pActiveUpgradeBuyPanel->m_pIncrementButton->IsEnabled() && m_pActiveUpgradeBuyPanel->m_pIncrementButton->IsVisible() )
			{
				OnCommand( VarArgs( "mvm_upgrade%03d_%i", m_pActiveUpgradeBuyPanel->m_nUpgradeIndex, m_pActiveUpgradeBuyPanel->m_nPrice ) );
			}
			else if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_ACTION && GetLocalPlayerBottleFromInventory() != NULL )
			{
				OnCommand( "quick_equip_bottle" );
			}
		}
		else if ( bBack )
		{
			if ( m_pActiveUpgradeBuyPanel && m_pActiveUpgradeBuyPanel->m_pDecrementButton && m_pActiveUpgradeBuyPanel->m_pDecrementButton->IsEnabled() && m_pActiveUpgradeBuyPanel->m_pDecrementButton->IsVisible() )
			{
				OnCommand( VarArgs( "mvm_downgrade%03d_%i", m_pActiveUpgradeBuyPanel->m_nUpgradeIndex, m_pActiveUpgradeBuyPanel->m_nPrice ) );
			}
		}
		else if ( bDone )
		{
			OnCommand( "close" );
		}
		else if ( bNext )
		{
			OnCommand( "next" );
		}
		else if ( bPrev )
		{
			OnCommand( "prev" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::UpdateHighlights( void )
{
	if ( !m_hPlayer )
		return;

	if ( m_iWeaponSlotBeingUpgraded == -1 )
	{
		int x, y;
		m_pPlayerUpgradeButton->GetPos( x, y );
		m_pActiveTabPanel->SetPos( x - YRES( 2 ), y - YRES( 2 ) );
	}
	else
	{
		FOR_EACH_VEC( m_pItemPanels, i )
		{
			CItemModelPanel *pItemPanel = m_pItemPanels[ i ];
			if ( pItemPanel )
			{
				int iLoadoutSlot = pItemPanel->GetItem()->GetStaticData()->GetLoadoutSlot( m_hPlayer->GetPlayerClass()->GetClassIndex() );
				if ( m_iWeaponSlotBeingUpgraded == iLoadoutSlot )
				{
					int x, y;
					pItemPanel->GetPos( x, y );
					m_pActiveTabPanel->SetPos( x - YRES( 2 ), y - YRES( 2 ) );
					break;
				}
			}
		}
	}

	// Highlight the panel we're mousing over and cycle upgrade tabs
	vgui::Panel *pMouseOverPanel = vgui::ipanel()->GetPanel( vgui::input()->GetMouseOver(), "ClientDLL" );

	if ( !m_bUsingController )
	{
		if ( pMouseOverPanel )
		{
			vgui::Panel *pTabPanel = pMouseOverPanel;

			bool bIsUpgradeButton = StringHasPrefix( pMouseOverPanel->GetName(), "UpgradeButton" );
			bool bIsPlayerUpgradeButton = V_strcmp( pMouseOverPanel->GetName(), "PlayerUpgradeButton" ) == 0;

			if ( bIsPlayerUpgradeButton )
			{
				// Set the text displaying which item the upgrades are for ("Shovel", etc.)
				m_pSelectWeaponPanel->SetDialogVariable( "upgrade_description", g_pVGuiLocalize->Find( g_aPlayerClassNames[ m_hPlayer->GetPlayerClass()->GetClassIndex() ] ) );
			}

			if ( bIsUpgradeButton && vgui::input()->IsMouseDown( MOUSE_LEFT ) )
			{
				if ( StringHasPrefix( pMouseOverPanel->GetName(), "UpgradeButton" ) )
				{
					OnItemPanelMousePressed( pTabPanel );
				}
			}

			if ( !( bIsUpgradeButton || bIsPlayerUpgradeButton ) )
			{
				// It's not mousing over any of the buttons
				pTabPanel = NULL;

				// If the player hasn't discovered tabs, cycle the highlight
				if ( !m_bHighlightedTab )
				{
					bool bBlink = ( static_cast< int >( gpGlobals->curtime * 10.0f ) % 2 ) == 0;
					if ( bBlink )
					{
						int nPanel = static_cast< int >( gpGlobals->curtime * 2.5f ) % 7;
						vgui::Panel *pInactiveTabPanel = m_pSelectWeaponPanel->FindChildByName( VarArgs( "InactiveTabPanel%i", nPanel + 1 ) );
						if ( pInactiveTabPanel && pInactiveTabPanel->IsVisible() )
						{
							pTabPanel = pInactiveTabPanel;
						}
					}
				}
			}

			if ( pTabPanel )
			{
				// Highlight the selected button
				m_pMouseOverTabPanel->SetVisible( true );

				int x, y;
				pTabPanel->GetPos( x, y );

				m_pMouseOverTabPanel->SetPos( x - YRES( 1 ), y - YRES( 1 ) );
			}
			else
			{
				m_pMouseOverTabPanel->SetVisible( false );
			}
		}
		else
		{
			m_pMouseOverTabPanel->SetVisible( false );
		}

		for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
		{
			ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

			FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
			{
				CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];

				// Hide the ones not in the current tab
				if ( m_iWeaponSlotBeingUpgraded != pItemSlotBuyPanel->nSlot )
				{
					pUpgradeBuyPanel->SetVisible( false );
					continue;
				}
			
				pUpgradeBuyPanel->SetVisible( true );

				if ( pMouseOverPanel && ( pMouseOverPanel == pUpgradeBuyPanel || pMouseOverPanel->GetParent() == pUpgradeBuyPanel ) )
				{
					m_pActiveUpgradeBuyPanel = pUpgradeBuyPanel;

					UpdateMouseOverHighlight();
				}
			}
		}
	}
}

void CHudUpgradePanel::UpdateMouseOverHighlight( void ) 
{
	if ( !m_pActiveUpgradeBuyPanel )
	{
		m_pMouseOverUpgradePanel->SetVisible( false );
		return;
	}

	// Highlight the selected upgrade panel
	m_pMouseOverUpgradePanel->SetVisible( true );

	int x, y;
	m_pActiveUpgradeBuyPanel->GetPos( x, y );
	m_pMouseOverUpgradePanel->SetPos( x - YRES( 1 ), y - YRES( 1 ) );

	CMannVsMachineUpgrades *pUpgrade = &(g_MannVsMachineUpgrades.m_Upgrades[ m_pActiveUpgradeBuyPanel->m_nUpgradeIndex ]);
	CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( pUpgrade->szAttrib );

	float flValue = pUpgrade->flIncrement;

	int iFormat = pAttribDef->GetDescriptionFormat();
	if ( iFormat == ATTDESCFORM_VALUE_IS_PERCENTAGE || iFormat == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE )
	{
		flValue += 1.0;
	}

	CEconAttributeDescription AttrDesc( GLocalizationProvider(), pAttribDef, flValue );

	m_pSelectWeaponPanel->SetDialogVariable( "upgrade_description", AttrDesc.GetDescription().Get() );
	m_pSelectWeaponPanel->SetDialogVariable( "upgrade_stats", "" );
}

void CHudUpgradePanel::UpdateButtonStates( int nCurrentMoney, int nUpgrade /*= 0*/, int nNumPurchased /*= 0*/ )
{	
	for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
	{
		ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

		if ( m_iWeaponSlotBeingUpgraded != pItemSlotBuyPanel->nSlot )
		{
			continue;
		}

		bool bSellAllowed = ( TFObjectiveResource()->GetMannVsMachineWaveCount() <= 1 );
		bool bHideBottleHintText = true;

		if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_ACTION || ( TFGameRules() && TFGameRules()->GetUpgradeTier( nUpgrade ) ) )
		{
			if ( nNumPurchased > 0 )
			{
				// Need to sell all other powerups
				FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, j )
				{
					CUpgradeBuyPanel *pActionBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ j ];

					if ( pActionBuyPanel->m_nUpgradeIndex != nUpgrade )
					{
						int nCurrentCount = pActionBuyPanel->m_nPurchases + pActionBuyPanel->m_nCurrentStep;
						if ( nCurrentCount > 0 )
						{
							int nAmmountToSell = bSellAllowed ? nCurrentCount : pActionBuyPanel->m_nPurchases;
							nCurrentMoney += nAmmountToSell * pActionBuyPanel->m_nPrice;
							m_nCurrency += nAmmountToSell * pActionBuyPanel->m_nPrice;
							pActionBuyPanel->m_nPurchases -= nCurrentCount;

							KeyValues *kv = new KeyValues( "MVM_Upgrade" );

							// Refunded for these ones
							if ( nAmmountToSell )
							{
								KeyValues *kvSub = new KeyValues( "upgrade" );
								kvSub->SetInt( "itemslot", pActionBuyPanel->m_nWeaponSlot );
								kvSub->SetInt( "upgrade", pActionBuyPanel->m_nUpgradeIndex );
								kvSub->SetInt( "count", -nAmmountToSell );
								kv->AddSubKey( kvSub );
							}

							// Not refunded for ones we previously bought
							if ( nCurrentCount != nAmmountToSell )
							{
								KeyValues *kvSub = new KeyValues( "upgrade" );
								kvSub->SetInt( "itemslot", pActionBuyPanel->m_nWeaponSlot );
								kvSub->SetInt( "upgrade", pActionBuyPanel->m_nUpgradeIndex );
								kvSub->SetInt( "count", -( nCurrentCount - nAmmountToSell ) );
								kvSub->SetBool( "free", true );
								kv->AddSubKey( kvSub );
							}

							engine->ServerCmdKeyValues( kv );
						}
					}
				}
			}

			if ( pItemSlotBuyPanel->upgradeBuyPanels.Count() <= 0 && !m_bInspectMode && m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_ACTION )
			{
				bHideBottleHintText = false;

				bool bHasBottle = ( GetLocalPlayerBottleFromInventory() != NULL );

				wchar_t wszFinalLabel[ 512 ] = L"";
				const wchar_t *pLocalized = g_pVGuiLocalize->Find( bHasBottle ? "#TF_PVE_Unequipped_Powerup_Bottle" : "#TF_PVE_No_Powerup_Bottle" );
				if ( pLocalized )
				{
					wchar_t wszLabel[ 512 ];
					V_wcsncpy( wszLabel, pLocalized, sizeof( wszLabel ) );

					UTIL_ReplaceKeyBindings( wszLabel, 0, wszFinalLabel, sizeof( wszFinalLabel ) );
				}

				m_pSelectWeaponPanel->SetDialogVariable( "powerup_hint", wszFinalLabel );
				m_pSelectWeaponPanel->SetControlVisible( "QuickEquipButton", bHasBottle );
				m_pSelectWeaponPanel->SetControlVisible( "LoadoutButton", bHasBottle );
			}
		}

		if ( bHideBottleHintText )
		{
			m_pSelectWeaponPanel->SetDialogVariable( "powerup_hint", "" );
			m_pSelectWeaponPanel->SetControlVisible( "QuickEquipButton", false );
			m_pSelectWeaponPanel->SetControlVisible( "LoadoutButton", false );
		}

		bool bSoldFinalPowerUp = false;

		// Achievement tracking
		int nUpgradesAtMax = 0;
		int nUpgrades = pItemSlotBuyPanel->upgradeBuyPanels.Count();
		bool bIsResistMax[4] = { 0 };

		FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
		{
			CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];

			if ( pUpgradeBuyPanel->m_nUpgradeIndex == nUpgrade && nNumPurchased != 0 )
			{
				// Purchased or sold something
				pUpgradeBuyPanel->m_nPurchases += nNumPurchased;

				KeyValues *kv = new KeyValues( "MVM_Upgrade" );
				KeyValues *kvSub = new KeyValues( "upgrade" );
				kvSub->SetInt( "itemslot", pUpgradeBuyPanel->m_nWeaponSlot );
				kvSub->SetInt( "upgrade", pUpgradeBuyPanel->m_nUpgradeIndex );

				if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_ACTION && !bSellAllowed )
				{
					if ( pUpgradeBuyPanel->m_nPurchases <= 0 && nNumPurchased > 0 )
					{
						// We already bought this previously
						if ( pUpgradeBuyPanel->m_nPurchases < 0 )
						{
							nNumPurchased += -pUpgradeBuyPanel->m_nPurchases;
							pUpgradeBuyPanel->m_nPurchases = 0;
						}

						nCurrentMoney += nNumPurchased * pUpgradeBuyPanel->m_nPrice;
						kvSub->SetBool( "free", true );
					}
					else if ( nNumPurchased < 0 && pUpgradeBuyPanel->m_nPurchases == 0 && pUpgradeBuyPanel->m_nCurrentStep == 0 )
					{
						bSoldFinalPowerUp = true;
					}
				}

				kvSub->SetInt( "count", nNumPurchased );

				kv->AddSubKey( kvSub );
				engine->ServerCmdKeyValues( kv );
			}

			pUpgradeBuyPanel->UpdateImages( nCurrentMoney );

			// Check if the upgrade will be maxed when considering purchases
			bool bMax = false;
			if ( pUpgradeBuyPanel->m_nPurchases + pUpgradeBuyPanel->m_nCurrentStep == pUpgradeBuyPanel->m_SkillTreeImages.Count() )
			{
				nUpgradesAtMax++;
				bMax = true;
			}

			// For ACHIEVEMENT_TF_MVM_MAX_PLAYER_RESISTANCES
			if ( bMax && m_iWeaponSlotBeingUpgraded == -1 )
			{
				// This is ugly - I blame time vs workload
				if ( pUpgradeBuyPanel->m_szAttribName )
				{
					CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( pUpgradeBuyPanel->m_szAttribName );
					if ( pAttribDef )
					{
						switch ( pAttribDef->GetDefinitionIndex() )
						{
						case 60:	// "dmg taken from fire reduced"
							{
								bIsResistMax[0] = true;
								break;
							}
						case 62:	// "dmg taken from crit reduced"
							{
								bIsResistMax[1] = true;
								break;
							}
						case 64:	// "dmg taken from blast reduced"
							{
								bIsResistMax[2] = true;
								break;
							}
						case 66:	// "dmg taken from bullets reduced"
							{
								bIsResistMax[3] = true;
								break;
							}
						}
					}
				}
			}
		}

		// Check for ACHIEVEMENT_TF_MVM_MAX_PRIMARY_UPGRADES
		if ( nUpgrades && nUpgrades == nUpgradesAtMax )
		{
			if ( m_hPlayer )
			{
				// Some spies may perceive the knife as their primary, while others
				// might think it's their pistol - which is actually a secondary
				if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY ) )
				{
					if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_MELEE ||
						 m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_SECONDARY )
					{
						m_bAwardMaxSlotAchievement = true;
					}
				}
				// Some engineers may perceive the sentry as their primary, so allow it
				else if ( m_hPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					CEconEntity *pEntity = NULL;
					CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, m_iWeaponSlotBeingUpgraded, &pEntity );
					CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( pEntity );
					if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD )
					{
						m_bAwardMaxSlotAchievement = true;
					}
				}

				// Everyone else
				if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_PRIMARY )
				{
					m_bAwardMaxSlotAchievement = true;
				}
			}
		}
		else
		{
			m_bAwardMaxSlotAchievement = false;
		}

		// Check for ACHIEVEMENT_TF_MVM_MAX_PLAYER_RESISTANCES
		if ( m_iWeaponSlotBeingUpgraded == -1 )
		{
			m_bAwardMaxResistAchievement = true;
			for ( int i = 0; i < ARRAYSIZE( bIsResistMax ); i++ )
			{
				if ( !bIsResistMax[i] )
				{
					m_bAwardMaxResistAchievement = false;
					break;
				}
			}
		}

		if ( m_iWeaponSlotBeingUpgraded == LOADOUT_POSITION_ACTION && !bSellAllowed && bSoldFinalPowerUp )
		{
			// Buy back already purchased bottles if they sold all of another type
			FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, j )
			{
				CUpgradeBuyPanel *pActionBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ j ];

				if ( pActionBuyPanel->m_nUpgradeIndex != nUpgrade )
				{
					int nCurrentCount = pActionBuyPanel->m_nPurchases + pActionBuyPanel->m_nCurrentStep;
					if ( nCurrentCount == 0 && pActionBuyPanel->m_nCurrentStep > 0 )
					{
						KeyValues *kv = new KeyValues( "MVM_Upgrade" );

						KeyValues *kvSub = new KeyValues( "upgrade" );
						kvSub->SetInt( "itemslot", pActionBuyPanel->m_nWeaponSlot );
						kvSub->SetInt( "upgrade", pActionBuyPanel->m_nUpgradeIndex );
						kvSub->SetInt( "count", pActionBuyPanel->m_nCurrentStep );
						kvSub->SetBool( "free", true );
						kv->AddSubKey( kvSub );

						engine->ServerCmdKeyValues( kv );

						pActionBuyPanel->m_nPurchases = 0;

						pActionBuyPanel->UpdateImages( nCurrentMoney );
					}
				}
			}
		}
	}

	// Credits count
	wchar_t wzCount[10];
	_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", nCurrentMoney );
	m_pSelectWeaponPanel->SetDialogVariable( "credits", wzCount );
}

void CHudUpgradePanel::UpdateItemStatsLabel( void )
{
	if ( !m_pUpgradeItemStatsLabel )
		return;

	m_pUpgradeItemStatsLabel->SetText( "" );

	if ( !m_hPlayer )
		return;

	wchar_t wszAttribDesc[4096];
	wszAttribDesc[0] = '\0';
	m_pUpgradeItemStatsLabel->GetTextImage()->ClearColorChangeStream();

	CEconItemDescription itemDesc;
	CEconItemDescription::CVisibleAttributeDisplayer attrIt;

	if ( m_iWeaponSlotBeingUpgraded == -1 )
	{
		m_hPlayer->m_AttributeList.IterateAttributes( &attrIt );
	}
	else
	{
		CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( m_hPlayer, m_iWeaponSlotBeingUpgraded );
		if ( pCurItemData )
		{
			pCurItemData->IterateAttributes( &attrIt );
		}
	}
	
	attrIt.SortAttributes();
	attrIt.Finalize( NULL, &itemDesc, GLocalizationProvider() );

	int iOutputAttributeLineCount = 0;
	for ( unsigned int i = 0; i < itemDesc.GetLineCount(); i++ )
	{
		const econ_item_description_line_t& line = itemDesc.GetLine(i);
		if ( line.unMetaType & kDescLineFlag_Attribute )
		{
			AddItemStatText( line.sText.Get(), line.eColor, wszAttribDesc, ARRAYSIZE( wszAttribDesc ) );
			++iOutputAttributeLineCount;
		}
	}

	if ( iOutputAttributeLineCount == 0 )
	{
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		Color col = pScheme->GetColor( GetColorNameForAttribColor( ATTRIB_COL_NEUTRAL ), Color(255,255,255,255) );
		m_pUpgradeItemStatsLabel->GetTextImage()->AddColorChange( col, Q_wcslen( wszAttribDesc ) );

		wchar_t *pNone = g_pVGuiLocalize->Find( "#TF_PassiveAttribs_None" );
		if ( pNone )
		{
			wcsncat( wszAttribDesc, pNone, ARRAYSIZE(wszAttribDesc) );
		}
	}

	m_pUpgradeItemStatsLabel->SetText( wszAttribDesc );
}

void CHudUpgradePanel::CancelUpgrades( void )
{
	bool bSellAllowed = ( TFObjectiveResource() ? TFObjectiveResource()->GetMannVsMachineWaveCount() <= 1 : true );

	KeyValues *kv = new KeyValues( "MVM_Upgrade" );

	for ( int nSlotIndex = 0; nSlotIndex < ARRAYSIZE( m_ItemSlotBuyPanels ); ++nSlotIndex )
	{
		ItemSlotBuyPanels *pItemSlotBuyPanel = &( m_ItemSlotBuyPanels[ nSlotIndex ] );

		FOR_EACH_VEC( pItemSlotBuyPanel->upgradeBuyPanels, i )
		{
			CUpgradeBuyPanel *pUpgradeBuyPanel = pItemSlotBuyPanel->upgradeBuyPanels[ i ];

			if ( pUpgradeBuyPanel->m_nPurchases != 0 )
			{
				KeyValues *kvSub = new KeyValues( "upgrade" );
				kvSub->SetInt( "itemslot", pUpgradeBuyPanel->m_nWeaponSlot );
				kvSub->SetInt( "upgrade", pUpgradeBuyPanel->m_nUpgradeIndex );
				kvSub->SetInt( "count", -pUpgradeBuyPanel->m_nPurchases );		// revert previous buy and sell

				if ( pItemSlotBuyPanel->nSlot == LOADOUT_POSITION_ACTION && !bSellAllowed && 
					 pUpgradeBuyPanel->m_nPurchases < 0 )
				{
					// The player wasn't refunded these so the cancel gives it back for free
					kvSub->SetBool( "free", true );
				}

				kv->AddSubKey( kvSub );
			}
		}
	}

	engine->ServerCmdKeyValues( kv );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::AddItemStatText( const locchar_t *loc_AttrDescText, attrib_colors_t eColor, wchar_t *out_wszAttribDesc, int iAttribDescSize )
{
	Assert( loc_AttrDescText );
	Assert( out_wszAttribDesc );

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	Assert( pScheme );

	// Insert the color change at the current position
	Color col = pScheme->GetColor( GetColorNameForAttribColor( eColor ), Color(255, 255, 255, 255) );
	m_pUpgradeItemStatsLabel->GetTextImage()->AddColorChange( col, Q_wcslen( out_wszAttribDesc ) );

	// Now append the text of the attribute
	wcsncat( out_wszAttribDesc, loc_AttrDescText, iAttribDescSize );
	wcsncat( out_wszAttribDesc, L"\n", iAttribDescSize );
}

CEconItemView* CHudUpgradePanel::GetLocalPlayerBottleFromInventory( void )
{
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv )
	{
		static CSchemaItemDefHandle pItemDef_MvmPowerupBottle( CTFItemSchema::k_rchMvMPowerupBottleItemDefName );
		Assert( pItemDef_MvmPowerupBottle );
		if ( pItemDef_MvmPowerupBottle )
		{
			for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
			{
				CEconItemView *pItem = pLocalInv->GetItem( i );
				Assert( pItem );
				if ( pItem->GetItemDefinition() == pItemDef_MvmPowerupBottle )
				{
					return pItem;
				}
			}
		}
	}

	return NULL;
}

bool CHudUpgradePanel::QuickEquipBottle( void )
{
	if ( !m_hPlayer )
		return false;

	if ( m_bInspectMode )
		return false;

	CPlayerInventory *pPlayerInventory = TFInventoryManager()->GetLocalInventory();
	if ( !pPlayerInventory )
		return false;

	GCSDK::CGCClientSharedObjectCache *pSOCache = pPlayerInventory->GetSOC();
	if ( !pSOCache )
		return false;

	int nClass = m_hPlayer->GetPlayerClass()->GetClassIndex();

	CEconItemView *pBottle = GetLocalPlayerBottleFromInventory();
	if ( !pBottle )
		return false;

	itemid_t iItemId = pBottle->GetItemID();
	if ( iItemId == INVALID_ITEM_ID )
		return false;

	// This clears any in-progress purchases, otherwise they could lose money and upgrades
	m_bShowUpgradeMenu = false;
	m_bCancelUpgrades = true;
	m_bOpenLoadout = false;
	m_bInspectMode = false;
	m_hPlayer = NULL;

	TFInventoryManager()->EquipItemInLoadout( nClass, LOADOUT_POSITION_ACTION, iItemId );

	// Tell the GC to tell server that we should respawn if we're in a respawn room

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudUpgradePanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		m_bShowUpgradeMenu = false;
		m_bCancelUpgrades = false;
		m_bOpenLoadout = false;

		// See if we need to award achievements
		if ( m_bAwardMaxSlotAchievement )
		{
			g_AchievementMgrTF.AwardAchievement( ACHIEVEMENT_TF_MVM_MAX_PRIMARY_UPGRADES );
		}
		if ( m_bAwardMaxResistAchievement )
		{
			g_AchievementMgrTF.AwardAchievement( ACHIEVEMENT_TF_MVM_MAX_PLAYER_RESISTANCES );
		}

		return;
	}
	else if ( !Q_stricmp( command, "cancel" ) )
	{
		m_bShowUpgradeMenu = false;
		m_bCancelUpgrades = true;
		m_bOpenLoadout = false;
		m_bInspectMode = false;
		m_hPlayer = NULL;
		return;
	}
	else if ( !Q_stricmp( command, "next" ) )
	{
		int nPrevLoadoutSlot = -1;
		int nLoadoutSlot = -1;

		if ( m_hPlayer )
		{
			int i;
			for ( i = 0; i < m_pItemPanels.Count(); i++ )
			{
				CItemModelPanel *pItemPanel = m_pItemPanels[ i ];

				if ( !pItemPanel )
					continue;

				if ( !pItemPanel->IsVisible() )
					continue;

				nPrevLoadoutSlot = nLoadoutSlot;

				nLoadoutSlot = pItemPanel->GetItem()->GetStaticData()->GetLoadoutSlot( m_hPlayer->GetPlayerClass()->GetClassIndex() );
				if ( m_iWeaponSlotBeingUpgraded == nPrevLoadoutSlot )
					break;
			}

			if ( i >= m_pItemPanels.Count() )
			{
				UpgradeItemInSlot( -1 );
			}
			else
			{
				UpgradeItemInSlot( nLoadoutSlot );
			}
		}
	}
	else if ( !Q_stricmp( command, "prev" ) )
	{
		int nPrevLoadoutSlot = -1;
		int nLoadoutSlot = -1;

		if ( m_hPlayer )
		{
			int i;
			for ( i = m_pItemPanels.Count() - 1; i >= 0; i-- )
			{
				CItemModelPanel *pItemPanel = m_pItemPanels[ i ];

				if ( !pItemPanel )
					continue;

				if ( !pItemPanel->IsVisible() )
					continue;

				nPrevLoadoutSlot = nLoadoutSlot;

				nLoadoutSlot = pItemPanel->GetItem()->GetStaticData()->GetLoadoutSlot( m_hPlayer->GetPlayerClass()->GetClassIndex() );
				if ( m_iWeaponSlotBeingUpgraded == nPrevLoadoutSlot )
					break;
			}

			if ( i < 0 )
			{
				UpgradeItemInSlot( -1 );
			}
			else
			{
				UpgradeItemInSlot( nLoadoutSlot );
			}
		}
	}
	else if ( V_strcmp( command, "quick_equip_bottle" ) == 0 )
	{
		QuickEquipBottle();

		return;
	}
	else if ( V_strcmp( command, "open_charinfo_direct" ) == 0 )
	{
		m_bShowUpgradeMenu = false;
		m_bCancelUpgrades = true;
		m_bOpenLoadout = true;
		return;
	}
	else if ( !Q_stricmp( command, "PlayerUpgrade" ) )
	{
		UpgradeItemInSlot( -1 );

		if ( !m_bHighlightedTab )
		{
			// They've clicked a tab... no need to blink them any longer
			m_bHighlightedTab = true;
			tf_mvm_tabs_discovered.SetValue( tf_mvm_tabs_discovered.GetInt() + 1 );
		}

		return;
	}
	else if ( StringHasPrefix( command, "mvm_upgrade" ) )
	{
		char szUpgradeNums[ 10 ];
		V_strncpy( szUpgradeNums, command + V_strlen( "mvm_upgrade" ), sizeof( szUpgradeNums ) );
		szUpgradeNums[ 3 ] = '\0';

		int nUpgrade = atoi( szUpgradeNums );
		int nPrice = atoi( szUpgradeNums + 4 );

		if ( m_hPlayer )
		{
			m_nCurrency -= nPrice;
			UpdateButtonStates( m_nCurrency, nUpgrade, 1 );
			m_pSelectWeaponPanel->SetControlEnabled( "CloseButton", true );

			m_nUpgradeActivity++;
		}
	}
	else if ( StringHasPrefix( command, "mvm_downgrade" ) )
	{
		char szUpgradeNums[ 10 ];
		V_strncpy( szUpgradeNums, command + V_strlen( "mvm_downgrade" ), sizeof( szUpgradeNums ) );
		szUpgradeNums[ 3 ] = '\0';

		int nUpgrade = atoi( szUpgradeNums );
		int nPrice = atoi( szUpgradeNums + 4 );

		if ( m_hPlayer )
		{
			m_nCurrency += nPrice;
			UpdateButtonStates( m_nCurrency, nUpgrade, -1 );
			m_pSelectWeaponPanel->SetControlEnabled( "CloseButton", true );

			m_nUpgradeActivity--;
		}
	}
	else if ( StringHasPrefix( command, "nexttip" ) )
	{
		UpdateTip();
	}
	else if ( V_strcmp( command, "respec" ) == 0 )
	{
		if ( m_hPlayer )
		{
			CancelUpgrades();
			OnCommand( "close" );
			KeyValues *kv = new KeyValues( "MVM_Respec" );
			engine->ServerCmdKeyValues( kv );
		}
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

void CHudUpgradePanel::UpdateTip()
{
	int iClassUsed;
	m_pTipPanel->SetDialogVariable( "tiptext", g_TFTips.GetRandomMvMTip( iClassUsed ) );
}


bool MannVsMachine_GetUpgradeInfo( int iAttribute, int iQuality, float &flValue )
{
	FOR_EACH_VEC( g_MannVsMachineUpgrades.m_Upgrades, i )
	{
		CEconItemAttributeDefinition *pAttrib = ItemSystem()->GetItemSchema()->GetAttributeDefinitionByName( g_MannVsMachineUpgrades.m_Upgrades[i].szAttrib );
		if ( !pAttrib )
			continue;

		if ( pAttrib->GetDefinitionIndex() == iAttribute && g_MannVsMachineUpgrades.m_Upgrades[i].nQuality == iQuality )
		{
			flValue = g_MannVsMachineUpgrades.m_Upgrades[i].flIncrement;
			return true;
		}
	}
	return false;
}
