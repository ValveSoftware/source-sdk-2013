//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_quest_map_panel.h"
#include "tf_quest_map_node_panel.h"
#include "tf_quest_map.h"
#include <vgui/IInput.h>
#include "tf_gc_client.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map_utils.h"
#include "tf_quest_map_controller.h"
#include "tf_quest_map_editor_panel.h"
#include "tf_quest_map_region_panel.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include "item_model_panel.h"
#include "tf_quest_map_node_view_panel.h"
#include "econ_item_inventory.h"
#include "tf_vgui_video.h"
#include "econ/econ_ui.h"
#include "store/store_panel.h"
#include "tf_item_inventory.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_hud_mainmenuoverride.h"
#include "c_tf_player.h"
#include "vguicenterprint.h"

ConVar tf_quest_map_intro_viewed( "tf_quest_map_intro_viewed", "0", FCVAR_ARCHIVE ); 
ConVar tf_quest_map_tuner_wobble_magnitude( "tf_quest_map_tuner_wobble_magnitude", "0.01" );
extern ConVar tf_quest_map_zoom_rest_scale;

class CQuestMapTooltip : public CTFTextToolTip
{
public:
	CQuestMapTooltip( vgui::Panel *parent ) : CTFTextToolTip( parent ) {}
	// Force the panel to reposition itself every frame.  If the tooltip is coming
	// from an image panel it won't do this, but for the quest panel this is what we want
	virtual void PerformLayout() { CTFTextToolTip::PerformLayout(); _isDirty = true; }
};


CQuestMapItemAdPanel::CQuestMapItemAdPanel( Panel* pParent, const char* pszName, const CQuestMapStoreItem* pStoreItemDef )
	: CItemAdPanel( pParent, pszName, INVALID_ITEM_DEF_INDEX )
	, m_pStoreItemDef( pStoreItemDef )
{
	auto& msgItem = m_pStoreItemDef->GetTypedMsg();

	auto *pItemDef = GetItemSchema()->GetItemDefinitionByName( msgItem.item_name().c_str() );
	if ( !pItemDef )
		return;

	m_item.Init( pItemDef->GetDefinitionIndex(), AE_UNIQUE, 1, 1 );
}

void CQuestMapItemAdPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/QuestMapRewardItemPanel.res" );
}

void CQuestMapItemAdPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	uint32 nLimit = m_pStoreItemDef->GetTypedMsg().purchase_limit();
	uint32 nPurchases = GetQuestMapHelper().GetNumRewardPurchases( m_pStoreItemDef->GetDefIndex() );
	bool bCanPurchase = nLimit == 0 || nPurchases < nLimit;

	SetControlVisible( "BuyButton", true, true );
	SetControlVisible( "MarketButton", false, true );
	uint32 nPrice = m_pStoreItemDef->GetTypedMsg().price().sint32();
	SetDialogVariable( "price", CFmtStr( "%d", nPrice ) );
	SetControlEnabled( "BuyButton", (int)nPrice <= GetQuestMapHelper().GetNumRewardCredits() && bCanPurchase, true );
	SetControlVisible( "SoldOutImage", !bCanPurchase );
	SetControlVisible( "LimitLabel", bCanPurchase && nLimit > 0 );
	SetDialogVariable( "limit", LocalizeNumberWithToken( "#TF_QuestMap_RewardStore_LimitedQuantity", nLimit - nPurchases ) );
}

void ConfirmPurchaseRewardItem( bool bConfirmed, void* pContext )
{
	if ( bConfirmed )
	{
		CQuestMapItemAdPanel* pPanel = (CQuestMapItemAdPanel*)(pContext);
		pPanel->OnConfirmPurchase();
	}
}

void CQuestMapItemAdPanel::OnCommand( const char *command )
{
	if ( FStrEq( "purchase", command ) )
	{
		// Buy the thing
		static wchar_t wszOutString[ 128 ];
		wchar_t wszCount1[ 16 ];
		_snwprintf( wszCount1, ARRAYSIZE( wszCount1 ), L"%d", m_pStoreItemDef->GetTypedMsg().price().sint32() );
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#TF_QuestMap_RewardStore_ConfirmPurchase_Body" );
		g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 2, wszCount1, m_item.GetItemName() );

		CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#TF_QuestMap_RewardStore_ConfirmPurchase_Title", wszOutString, "#TF_Coach_Yes", "#Cancel", &ConfirmPurchaseRewardItem, NULL );
		if ( pDialog )
		{
			pDialog->SetContext( this );
			pDialog->Show();
		}
	}
}

void CQuestMapItemAdPanel::OnConfirmPurchase()
{
	GetQuestMapController().PurchaseReward( m_pStoreItemDef );
}

DECLARE_BUILD_FACTORY( CQuestMapPanel );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestMapPanel* g_pQuestMapPanel = NULL;

CQuestMapPanel* GetQuestMapPanel()
{
	if ( g_pQuestMapPanel == NULL )
	{
		CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		g_pQuestMapPanel = new CQuestMapPanel( pMMOverride, "QuestMap" );
		g_pQuestMapPanel->MakeReadyForUse();
	}

	return g_pQuestMapPanel;
}

CQuestMapPanel::CQuestMapPanel( Panel *pParent, const char *pszPanelName )
	: BaseClass( pParent, pszPanelName )
	, m_mapRegions( DefLessFunc( uint32 ) )
	, m_bMapLoaded( false )
	, m_bViewingTutorial( false )
	, m_eIntroState( STATE_0 )
	, m_pKVRewardItemPanels( NULL )
	, m_flNextWobbleTime( 0.f )
	, m_pAdPanel( NULL )
{
	// So we can paint the radio needle *on top* of our children
	SetPostChildPaintEnabled( true );

	m_currentRegion.set_type( DEF_TYPE_QUEST_MAP_REGION );

	if (g_pVGuiLocalize)
	{
		g_pVGuiLocalize->AddFile( "resource/tf_quests_%language%.txt" );
	}

	Assert( g_pQuestMapPanel == NULL );
	g_pQuestMapPanel = this;

	// Item tooltip
	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );

	// Text tooltip
	m_pToolTip = new CQuestMapTooltip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	m_pMainContainer = new EditablePanel( this, "MainContainer" );
	m_pRewardsStoreButton = new CExImageButton( m_pMainContainer, "RewardsStoreButton", (const char*)NULL, this );
	m_pMapButton = new CExImageButton( m_pMainContainer, "MapButton", (const char*)NULL, this );
	m_pPowerSwitch = new CExImageButton( m_pMainContainer, "PowerSwitchButton", (const char*)NULL, this );

	m_pMapAreaPanel = new EditablePanel( m_pMainContainer, "MapAreaPanel" );
	m_pTurnInCompletePopup = new EditablePanel( m_pMapAreaPanel, "TurnInCompletePopup" );

	m_pAdPanel = new CCyclingAdContainerPanel( m_pMapAreaPanel, "CyclingAd" );

	// Quest objective tooltip
	m_pQuestObjectiveTooltip = new CQuestObjectiveTooltip( m_pMapAreaPanel, "ObjectiveTooltip" );
	m_pQuestObjectivePanel = new CQuestObjectivePanel( m_pMapAreaPanel, "QuestObjective" );
	m_pQuestObjectiveTooltip->SetObjectivePanel( m_pQuestObjectivePanel );
	// Node view
	m_pQuestNodeViewPanel = new CQuestNodeViewPanel( m_pMapAreaPanel, "SelectedNodeInfoPanel" ); 
	m_pQuestNodeViewPanel->SetItemModelPanelTooltip( m_pMouseOverTooltip );
	m_pQuestNodeViewPanel->SetTextTooltip( m_pToolTip );
	m_pQuestNodeViewPanel->SetObjectiveTooltip( m_pQuestObjectiveTooltip );

	// Rewards Store
	m_pRewardsShopPanel = new EditablePanel( m_pMapAreaPanel, "RewardsShop" );

	m_pIntroPanel = new EditablePanel( m_pMapAreaPanel, "Introduction" );
	m_IntroStages[ STATE_0 ].m_pStagePanel = new EditablePanel( m_pIntroPanel, "IntroStage1" );
	m_IntroStages[ STATE_1 ].m_pStagePanel = new EditablePanel( m_pIntroPanel, "IntroStage2" );
	m_IntroStages[ STATE_2 ].m_pStagePanel = new EditablePanel( m_pIntroPanel, "IntroStage3" );
	m_pVideoPanel = new CTFVideoPanel( m_pIntroPanel, "VideoPanel" );
	m_IntroStages[ STATE_0 ].m_pHoverButton = new CExImageButton( m_pIntroPanel, "HoverButtonStage1", (const char*)NULL, this );
	m_IntroStages[ STATE_0 ].m_pHoverButton->PassMouseTicksTo( this, true );

	m_IntroStages[ STATE_1 ].m_pHoverButton = new CExImageButton( m_pIntroPanel, "HoverButtonStage2", (const char*)NULL, this );
	m_IntroStages[ STATE_1 ].m_pHoverButton->PassMouseTicksTo( this, true );

	m_IntroStages[ STATE_2 ].m_pHoverButton = new CExImageButton( m_pIntroPanel, "HoverButtonStage3", (const char*)NULL, this );
	m_IntroStages[ STATE_2 ].m_pHoverButton->PassMouseTicksTo( this, true );

	ListenForGameEvent( "proto_def_changed" );
	ListenForGameEvent( "gameui_hidden" );
	ListenForGameEvent( "quest_request" );
	ListenForGameEvent( "quest_response" );
	ListenForGameEvent( "quest_map_data_changed" );
	ListenForGameEvent( "gc_new_session" );
	ListenForGameEvent( "quest_turn_in_state" );
	ListenForGameEvent( "items_acknowledged" );

	// TODO: Tutorial check here
	m_eScreenDisplay = SCREEN_INVALID;
	ChangeScreenDisplay( SCREEN_MAP ); // This needs to be after all the panel pointers are setup
}

CQuestMapPanel::~CQuestMapPanel()
{
	Assert( g_pQuestMapPanel == this );
	g_pQuestMapPanel = NULL;
}

void CQuestMapPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/QuestMapPanel.res" );
}

void CQuestMapPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	if ( m_pKVRewardItemPanels )
	{
		m_pKVRewardItemPanels->deleteThis();
		m_pKVRewardItemPanels = NULL;
	}

	KeyValues* pKVRewardKV = inResourceData->FindKey( "RewardItemKV" );
	if ( pKVRewardKV )
	{
		m_pKVRewardItemPanels = pKVRewardKV->MakeCopy();
	}
}

void CQuestMapPanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( "close", pCommand ) )
	{	
		SetVisible( false );
		return;
	}
	else if ( FStrEq( "anim_close", pCommand ) )
	{
		PlaySoundEntry( "CYOA.MapClose" );

		// Do the closing anim sequence
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "DelayQuestMapClose", false );	
		// Send the needle off to the left
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "tuner_pos", 0.f, 0.f, 0.4f, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
		m_pPowerSwitch->SetSelected( true );
		return;
	}
	else if ( FStrEq( "switch_on", pCommand ) )
	{
		PlaySoundEntry( "CYOA.Switch" );
		m_pPowerSwitch->SetSelected( false );
		return;
	}
	else if ( FStrEq( "switch_off", pCommand ) )
	{
		m_pPowerSwitch->SetSelected( true );
		return;
	}
	else if ( FStrEq( "debug_menu", pCommand ) )
	{
	}
	else if ( FStrEq( "endintro", pCommand ) )
	{
		m_bViewingTutorial = false;
		tf_quest_map_intro_viewed.SetValue( true );
		engine->ClientCmd_Unrestricted( "host_writeconfig" );
		UpdateControls();
		m_pIntroPanel->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestMap_Start", false );	
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_bMapLoaded ? "QuestMap_MapLoaded" : "QuestMap_LoadingLoop", false );	
	}
	else if ( FStrEq( "rewards_store", pCommand ) ) 
	{
		ChangeScreenDisplay( SCREEN_STORE );

		return;
	}
	else if ( FStrEq( "view_map", pCommand ) )
	{
		ChangeScreenDisplay( SCREEN_MAP );

		return;
	}
	else if ( FStrEq( "update_region_visiblity", pCommand ) )
	{
		PlayTransitionScreenEffects();
		UpdateRegionVisibility();
		return;
	}

	BaseClass::OnCommand( pCommand );
}

void CQuestMapPanel::ChangeScreenDisplay( EScreenDisplay eScreen )
{

	switch( eScreen )
	{
		case SCREEN_MAP:
		{
			// Nothing needed
		}
		break;

		case SCREEN_STORE:
		{
			CExScrollingEditablePanel* pItemScroller = m_pRewardsShopPanel->FindControl< CExScrollingEditablePanel >( "ItemsScroller", true );
			if ( pItemScroller )
			{
				pItemScroller->ResetScrollAmount();
			}

			// Have the rewards re-evaluate their state
			FOR_EACH_VEC( m_vecRewardsShopItemPanels, i )
			{
				m_vecRewardsShopItemPanels[ i ]->InvalidateLayout();
			}

			// Hide the node view
			m_pQuestNodeViewPanel->SetVisible( false );
		}
		break;
	};

	m_pRewardsStoreButton->SetSelected( eScreen == SCREEN_STORE );
	m_pRewardsStoreButton->SetMouseInputEnabled( eScreen != SCREEN_STORE );
	m_pMapButton->SetSelected( eScreen == SCREEN_MAP );
	m_pMapButton->SetMouseInputEnabled( eScreen != SCREEN_MAP );

	// Not changing anything
	if ( eScreen == m_eScreenDisplay )
		return;

	m_eScreenDisplay = eScreen;

	UpdatePassAdPanel();

	PlayTransitionScreenEffects();
	InvalidateLayout();
}

void CQuestMapPanel::UpdateIntroState()
{
	// Be default STATE_0 when not mousing over a button
	EIntroState eNewIntroState = STATE_0;

	for( int eState = STATE_1; eState <= NUM_INTRO_STATES; ++eState )
	{
		if ( m_IntroStages[ eState - 1 ].m_pHoverButton->IsArmed() )
		{
			eNewIntroState = (EIntroState)eState;
			m_pVideoPanel->BeginPlayback( CFmtStr( "media/cyoa_intro_stage%d.vid", eState ) );
			m_pVideoPanel->SetVisible( true );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_IntroStages[ eState - 1 ].m_pStagePanel, "QuestMapIntro_StageReveal", false );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pIntroPanel, "QuestMapIntro_ShowStage", false );
		}
	}

	
	if ( eNewIntroState == STATE_0 && m_eIntroState != STATE_0 )
	{
		m_pVideoPanel->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pIntroPanel, "QuestMapIntro_ClearStage", false );
	}

	m_pIntroPanel->SetControlVisible( "IntroStage0", eNewIntroState == STATE_0 );
	m_pIntroPanel->SetControlVisible( "IntroStage1", eNewIntroState == STATE_1 );
	m_pIntroPanel->SetControlVisible( "IntroStage2", eNewIntroState == STATE_2 );
	m_pIntroPanel->SetControlVisible( "IntroStage3", eNewIntroState == STATE_3 );

	m_eIntroState = eNewIntroState;
}

void CQuestMapPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nConTrackerLevel = 0;


	static CSchemaItemDefHandle pItemDef_ActivatedCampaign3Pass( "Activated Campaign 3 Pass" );
	CTFPlayerInventory *pInv = TFInventoryManager()->GetInventoryForPlayer( ClientSteamContext().GetLocalPlayerSteamID() );
	if ( pInv )
	{
		CEconItemView *pItem = pInv->GetFirstItemOfItemDef( pItemDef_ActivatedCampaign3Pass->GetDefinitionIndex() );
		if ( pItem )
		{
			nConTrackerLevel = pItem->GetStyle();
		}
	}

	ImagePanel* pScreen = m_pMainContainer->FindControl< ImagePanel >( "ScreenBorder" );
	switch ( nConTrackerLevel )
	{
		case 0: pScreen->SetImage( "cyoa/cyoa_pda_gravel" ); break;
		case 1: pScreen->SetImage( "cyoa/cyoa_pda_bronze" ); break;
		case 2: pScreen->SetImage( "cyoa/cyoa_pda_silver" ); break;
		case 3: pScreen->SetImage( "cyoa/cyoa_pda_gold" ); break;
	}

	Panel* pRewardsShopPanel = m_pMapAreaPanel->FindChildByName( "RewardsShop", true );
	if ( pRewardsShopPanel )
	{
		pRewardsShopPanel->SetVisible( m_eScreenDisplay == SCREEN_STORE );
	}

	UpdateRegionVisibility();
	UpdateStarsGlobalStatus();

	CExScrollingEditablePanel* pItemScroller = m_pRewardsShopPanel->FindControl< CExScrollingEditablePanel >( "ItemsScroller", true );
	Assert( pItemScroller );
	if ( !pItemScroller )
		return;

	FOR_EACH_VEC( m_vecRewardsShopItemPanels, i )
	{
		Panel* pRewardItem = m_vecRewardsShopItemPanels[ i ];
		int nXPos = ( i % 2 ) == 0 ? XRES( 10 ) : m_pRewardsShopPanel->GetWide() - pRewardItem->GetWide() - XRES( 10 );
		int nYPos = ( i / 2 ) * ( pRewardItem->GetTall() + YRES( 10 ) ) + YRES( 5 ) - pItemScroller->GetScrollAmount();
		pRewardItem->SetPos( nXPos, nYPos );
	}
}

void CQuestMapPanel::PostChildPaint()
{
	BaseClass::PostChildPaint();

	static int snWhiteTextureID = -1;
	if ( snWhiteTextureID == -1 )
	{
		snWhiteTextureID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( snWhiteTextureID, "vgui/white" , true, false);
		if (snWhiteTextureID == -1)
			return;
	}

	//
	// Check if we need to re-randomize the needle wobbling
	//
	if ( Plat_FloatTime() > m_flNextWobbleTime )
	{
		// A whole bunch of randomness to make the needle wobble
		float flLerpTime = Bias( RandomFloat( 0.1f, 1.f ), 0.2f );
		m_flNextWobbleTime = Plat_FloatTime() + flLerpTime;

		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "tuner_wobble", RandomFloat( -1.f, 1.f ), 0.f, flLerpTime, vgui::AnimationController::INTERPOLATOR_BIAS, RandomFloat( 0.25f, 0.75f ), true, false );
	}

	//
	// Draw the needle for the radio tuner
	//
	{
		vgui::surface()->DrawSetTexture( snWhiteTextureID );
		vgui::surface()->DrawSetColor( Color( 180, 0, 0, 255 ) );

		int nYPos = YRES( 396 );
		int nXPos = GetWide() * 0.5 - YRES( 80 );
		int nStride = YRES( 175 );
		int nTall = YRES( 35 );
		int nWide = YRES( 3 );

		int nX = RemapVal( m_flTunerPos + ( m_flTunerWobble * tf_quest_map_tuner_wobble_magnitude.GetFloat() ), 0.f, 1.f, (float)nXPos, (float)(nXPos + nStride) ) - ( nWide * 0.5f );

		surface()->DrawFilledRect( nX, nYPos, nX + nWide, nYPos + nTall );
	}
}

void CQuestMapPanel::SetVisible( bool bVisible )
{
	if ( IsVisible() == bVisible )
		return;

	UpdateControls( bVisible );

	BaseClass::SetVisible( bVisible );

	// Send a user command to the server to get our player model to animate
	// into or out of the CYOA PDA sequence
	engine->ServerCmd( CFmtStr( "cyoa_pda_open %d", bVisible ? 1 : 0 ) );

	if ( bVisible )
	{
		PlaySoundEntry( "CYOA.MapOpen" );
		// If they closed and re-opened the quest map, make sure the mouse
		// block is not visible.
		SetControlVisible( "MouseBlocker", false );

		m_pQuestNodeViewPanel->SetVisible( false );

		CExImageButton* pButton = FindControl< CExImageButton >( "PowerSwitchButton", true );
		if ( pButton )
		{
			pButton->SetSelected( false );
		}

		if ( tf_quest_map_intro_viewed.GetBool() )
		{
			m_pIntroPanel->SetVisible( false );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestMap_Start", false );	
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_bMapLoaded && GTFGCClientSystem()->BHealthyGCConnection() ? "QuestMap_MapLoaded" : "QuestMap_LoadingLoop", false );	
		}
		else 
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestMap_Start", false );	
			m_pIntroPanel->SetVisible( true );
			m_pIntroPanel->SetControlVisible( "IntroStage0", true );
			m_bViewingTutorial = true;
			m_pVideoPanel->BeginPlayback( "media/test.vid" );
		}
	}
	else
	{
		ChangeScreenDisplay( SCREEN_MAP );
	}
}

void CQuestMapPanel::PlayTransitionScreenEffects()
{
	PlaySoundEntry( "CYOA.StaticFade" );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestMap_StaticFadeOut", false );	
}

void CQuestMapPanel::QueueTurnInAnims()
{
	EQuestTurnInState eLastQueued = TURN_IN_BEGIN;
	float flTimeAccum = 0.f;
	auto lambdaQueue = [&]( EQuestTurnInState eState, float flTimeAfterLastStep )
	{
		flTimeAccum += flTimeAfterLastStep;
		Assert( eState > eLastQueued ); // Sanity
		PostMessage( this, new KeyValues( "FireTurnInStateEvent", "state", eState ), flTimeAccum );
		eLastQueued = eState;
	};

	auto& msgReport = GetQuestMapController().GetMostRecentProgressReport();
	float flStarTime = 0.f; 
	if ( msgReport.star_0_earned() ) flStarTime += 0.3f;
	if ( msgReport.star_1_earned() ) flStarTime += 0.3f;
	if ( msgReport.star_2_earned() ) flStarTime += 0.3f;

	if ( m_bTurnInSuccess )
	{
		lambdaQueue( TURN_IN_SHOW_SUCCESS, 0.f );
		lambdaQueue( TURN_IN_HIDE_SUCCESS, 1.3f );
		lambdaQueue( TURN_IN_SHOW_STARS_EARNED, 0.6f );
		if ( msgReport.reward_credits_earned() > 0 )
		{
			lambdaQueue( TURN_IN_SHOW_BLOOD_MONEY_EARNED, flStarTime );
		}

		if( msgReport.items_earned_size() > 0 )
		{
			lambdaQueue( TURN_IN_SHOW_ITEMS_EARNED_EARNED, 0.5f );
			lambdaQueue( TURN_IN_SHOW_ITEM_PICKUP_SCREEN, 2.f );
			// We have to wait here, because the user will take their time
			// or not come back at all, which is fine
		}
		else
		{
			lambdaQueue( TURN_IN_HIDE_NODE_VIEW, 2.f );
			lambdaQueue( TURN_IN_SHOW_NODE_UNLOCKS, 0.5f );
			if ( msgReport.reward_credits_earned() > 0 )
			{
				lambdaQueue( TURN_IN_SHOW_GLOBAL_BLOOD_MONEY, 1.f );
			}
			lambdaQueue( TURN_IN_SHOW_GLOBAL_STARS, 1.f );
			lambdaQueue( TURN_IN_COMPLETE, 0.f );
		}
	}
	else
	{
		lambdaQueue( TURN_IN_SHOW_FAILURE, 0.f );
		lambdaQueue( TURN_IN_HIDE_FAILURE, 2.f );
		lambdaQueue( TURN_IN_COMPLETE, 2.f );
	}
}

void CQuestMapPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "items_acknowledged" ) )
	{
		if ( m_bAwaitingItemConfirm )
		{
			m_bAwaitingItemConfirm = false;
			EQuestTurnInState eLastQueued = TURN_IN_BEGIN;
			float flTimeAccum = 0.f;
			auto lambdaQueue = [&]( EQuestTurnInState eState, float flTimeAfterLastStep )
			{
				flTimeAccum += flTimeAfterLastStep;
				Assert( eState > eLastQueued ); // Sanity
				PostMessage( this, new KeyValues( "FireTurnInStateEvent", "state", eState ), flTimeAccum );
				eLastQueued = eState;
			};

			auto& msgReport = GetQuestMapController().GetMostRecentProgressReport();

			lambdaQueue( TURN_IN_HIDE_NODE_VIEW, 0.5f );
			lambdaQueue( TURN_IN_SHOW_NODE_UNLOCKS, 0.5f );
			if ( msgReport.reward_credits_earned() > 0 )
			{
				lambdaQueue( TURN_IN_SHOW_GLOBAL_BLOOD_MONEY, 1.f );
			}
			lambdaQueue( TURN_IN_SHOW_GLOBAL_STARS, 1.f );
			lambdaQueue( TURN_IN_COMPLETE, 0.f );
		}
	}
	else if ( FStrEq( event->GetName(), "quest_response" ) )
	{
		if( event->GetInt( "request" ) != k_EMsgGCQuestNodeTurnIn )
			return;

		m_bTurnInSuccess = event->GetBool( "success" );
	}
	else if ( FStrEq( event->GetName(), "quest_request" ) )
	{
		if ( event->GetInt( "request" ) != k_EMsgGCQuestNodeTurnIn )
			return;

		// In 5 seconds, do whatever anims we need to do.  We'll (hopefully) hear back about
		// success within that time.  If we don't, we assume failure.
		PostMessage( this, new KeyValues( "QueueTurnInAnims" ), k_flQuestTurnInTime );
		PostMessage( this, new KeyValues( "FireTurnInStateEvent", "state", TURN_IN_BEGIN ), 0.0f );
		m_bTurnInSuccess = false;
	}
	else if ( FStrEq( event->GetName(), "quest_map_data_changed" ) )
	{
		UpdateControls();
		return;
	}
	else if ( FStrEq( event->GetName(), "proto_def_changed" ) && ( event->GetInt( "type" ) == DEF_TYPE_QUEST_MAP_NODE ) )
	{
		bool bVisible = IsVisible();
		UpdateControls();
		SetVisible( bVisible );
	}
	else if ( FStrEq( event->GetName(), "gc_new_session" ) )
	{
		UpdateControls();
		return;
	}
	else if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		// When the gameui hides, we need to hide so we're not still open if the gameui re-opens
		SetVisible( false );
	}
	else if ( FStrEq( event->GetName(), "quest_turn_in_state" ) )
	{
		EQuestTurnInState eState = (EQuestTurnInState)event->GetInt( "state" );
		auto& msgReport = GetQuestMapController().GetMostRecentProgressReport();

		auto lambdaShowGainsOverPanel = [&]( const char* pszPanelName,
											 const char* pszToken,
											 int nGain )
		{
			Panel* pPanel = m_pMapAreaPanel->FindChildByName( pszPanelName, true );
			if ( pPanel && nGain )
			{
				int nX, nY;
				pPanel->GetPos( nX, nY );
				pPanel->ParentLocalToScreen( nX, nY );
				CreateScrollingIndicator( nX,
										  nY - YRES( 20 ),
										  LocalizeNumberWithToken( pszToken, nGain ),
										  "MatchMaking.XPChime",
										  0.f,
										  0,
										  -25,
										  true );
			}
		};

		switch( eState )
		{
			case TURN_IN_BEGIN:
			{
				// We don't want to allow any input while we're doing turn-in animations
				SetControlVisible( "MouseBlocker", true );
				break;
			}

			case TURN_IN_SHOW_SUCCESS:
			{
				PlaySoundEntry( "Quest.TurnInAcceptedLight" );
				m_pTurnInCompletePopup->SetDialogVariable( "result",  g_pVGuiLocalize->Find( "#TF_QuestView_TurnInSuccess" ) );
				m_pTurnInCompletePopup->SetVisible( true );
				break;
			}

			case TURN_IN_HIDE_SUCCESS:
			{
				m_pTurnInCompletePopup->SetVisible( false );
				break;
			}

			case TURN_IN_SHOW_STARS_EARNED:
			{
				// This is a hack.  There's a few objective panels that are tying to play this sound
				// so it's stacking and sounding bad.  This is way easier than .res file plumbing
				// a setting.
				float flDelay = 0.f;
				auto lambdaPlayChime = [&]()
				{
					PostMessage( this, new KeyValues( "PlaySoundEntry", "sound", "MatchMaking.XPChime" ), flDelay );
					flDelay += 0.3f;
				};

				if ( msgReport.star_0_earned() ) lambdaPlayChime();
				if ( msgReport.star_1_earned() ) lambdaPlayChime();
				if ( msgReport.star_2_earned() ) lambdaPlayChime();
				break;
			}

			case TURN_IN_SHOW_BLOOD_MONEY_EARNED:
			{
				break;
			}

			case TURN_IN_SHOW_ITEM_PICKUP_SCREEN:
			{
				m_bAwaitingItemConfirm = true;
				InventoryManager()->ShowItemsPickedUp( true, false );
				PlaySoundEntry( "plng_contract_complete_give_item_allclass" );
				break;
			}

			case TURN_IN_SHOW_FAILURE:
			{
				m_pTurnInCompletePopup->SetDialogVariable( "result", g_pVGuiLocalize->Find( "#TF_QuestView_TurnInFailure" ) );
				m_pTurnInCompletePopup->SetVisible( true );
				break;
			}

			case TURN_IN_HIDE_FAILURE:
			{
				m_pTurnInCompletePopup->SetVisible( false );
				break;
			}
			
			case TURN_IN_SHOW_GLOBAL_BLOOD_MONEY:
			{
				lambdaShowGainsOverPanel( "RewardCreditsLabel", "#TF_QuestMap_BloodMoneyGained", msgReport.reward_credits_earned() );
				break;
			}

			case TURN_IN_SHOW_GLOBAL_STARS:
			{
				int nNumStarsEarned = 0;
				if ( msgReport.star_0_earned() ) nNumStarsEarned += 1;
				if ( msgReport.star_1_earned() ) nNumStarsEarned += 1;
				if ( msgReport.star_2_earned() ) nNumStarsEarned += 1;
				Assert( nNumStarsEarned > 0 );

				lambdaShowGainsOverPanel( "AvailableStarsLabel", "#TF_QuestMap_StarsGained", nNumStarsEarned );
				break;
			}

			case TURN_IN_COMPLETE:
			{
				SetControlVisible( "MouseBlocker", false );
				break;
			}

			// Nothing to do for these
			case TURN_IN_HIDE_NODE_VIEW:
			case TURN_IN_SHOW_NODE_UNLOCKS:
				break;
		};
	}
	
}


void CQuestMapPanel::SetRegion( const CQuestMapRegion* pRegion, bool bZoomIn )
{
	CQuestMapRegionPanel* pCurrentRegionPanel = m_mapRegions[ m_mapRegions.Find( m_currentRegion.defindex() ) ]; 
	CQuestMapRegionPanel* pNewRegionPanel = m_mapRegions[ m_mapRegions.Find( pRegion->GetDefIndex() ) ]; 
	Assert( pCurrentRegionPanel && pNewRegionPanel );
	if ( !pCurrentRegionPanel || !pNewRegionPanel )
		return;

	float flLinkX = 0.5, flLinkY = 0.5;

	const CQuestMapRegionPanel* pLinkContainingPanel = bZoomIn ? pCurrentRegionPanel : pNewRegionPanel;
	uint32 nLinkDefindex = bZoomIn ? pRegion->GetDefIndex() : m_currentRegion.defindex();
	const EditablePanel* pRegionLink = pLinkContainingPanel->GetRegionLinkPanel( nLinkDefindex );

	if ( pRegionLink )
	{
		flLinkX = pLinkContainingPanel->GetZoomPanel()->GetChildPositionInfo( pRegionLink )->m_flX;
		flLinkY = pLinkContainingPanel->GetZoomPanel()->GetChildPositionInfo( pRegionLink )->m_flY;
	}

	pCurrentRegionPanel->StartZoomAway( flLinkX, flLinkY, bZoomIn );
	pNewRegionPanel->StartZoomTo( flLinkX, flLinkY, bZoomIn );

	// Run our own animation command to manage things we need to do
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "RegionZoom" );

	m_currentRegion.set_defindex( pRegion->GetDefIndex() );

	// Make the tuner needle move
	{
		float flDestination = pRegion->GetRadioFreq();
		float flTime = tf_quest_map_zoom_transition_in_time * 2; // 2x for zoom-away + zoom-to

		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "tuner_pos", flDestination, 0.f, flTime, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
	}

	UpdatePassAdPanel();
}

void CQuestMapPanel::UpdatePassAdPanel()
{
	if ( !m_bMapLoaded ) 
		return;
	//
	// Update the item ad panel.  Go through all of the nodes on the region and check for any operations
	// that require an item to activate them.  If we find some, add them to our item ad panel and trigger
	// the item panel to show.
	//
	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_currentRegion.defindex() );
	Assert( pRegion );
	if ( !pRegion )
		return;

	// Cook up KVs for this item ad
	KeyValuesAD pKVItemAd( "items" ); // The panel will copy these

	auto& vecNodesWithin = pRegion->GetNodesInRegion();
	CUtlVector< uint32 > vecItemAdDefs; // Track what we've added so we dont add it multiple times
	// Loop the nodes looking for passes
	FOR_EACH_VEC( vecNodesWithin, i )
	{
		const CEconOperationDefinition* pOperation = vecNodesWithin[ i ]->GetAssociatedOperation();
		if ( pOperation )
		{
			if ( pOperation->GetGatewayItemDefIndex() == INVALID_ITEM_DEF_INDEX )
				continue;

			// Check if we got this one already
			if ( vecItemAdDefs.Find( pOperation->GetGatewayItemDefIndex() ) != vecItemAdDefs.InvalidIndex() )
				continue;

			// Check if they already have the pass.  Don't need to show it if they have it!
			if ( TFInventoryManager()->GetLocalTFInventory()->FindFirstItembyItemDef( pOperation->GetRequiredItemDefIndex() ) != NULL )
				continue;

			// Track that we're adding this
			vecItemAdDefs.AddToTail( pOperation->GetGatewayItemDefIndex() );

			// Sanity
			CEconItemDefinition* pGatewayItemDef = GetItemSchema()->GetItemDefinition( pOperation->GetGatewayItemDefIndex() );
			Assert( pGatewayItemDef );
			if ( !pGatewayItemDef )
				continue;

			// Hack to use the Contracts Pass instead of the Campaign Pass for Jungle Inferno
			static CSchemaItemDefHandle JungleInfernoCampaignDef( "Unused Campaign 3 Pass" );
			static CSchemaItemDefHandle JungleInfernoContractsDef( "Jungle Inferno Contracts Pass" );
			if ( pGatewayItemDef->GetDefinitionIndex() == JungleInfernoCampaignDef->GetDefinitionIndex() )
			{
				pGatewayItemDef = GetItemSchema()->GetItemDefinition( JungleInfernoContractsDef->GetDefinitionIndex() );
			}

			// Check if we're to show that it's in the store or in the market
			bool bStoreIsReady = EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetPriceSheet() && EconUI()->GetStorePanel()->GetCart() && steamapicontext && steamapicontext->SteamUser();
			bool bGatewayItemInStore = false;
			// Check if the gateway item is in the Mann Co Store
			if ( bStoreIsReady )
			{
				bGatewayItemInStore = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( pGatewayItemDef->GetDefinitionIndex() ) != NULL;
			}

			KeyValues* pKVItem = pKVItemAd->CreateNewKey();
			pKVItem->SetName( "0" );
			pKVItem->SetString( "item", pGatewayItemDef->GetDefinitionName() );
			pKVItem->SetInt( "show_market", bGatewayItemInStore ? 0 : 1 );
		}
	}

	m_pAdPanel->SetVisible( vecItemAdDefs.Count() && m_eScreenDisplay == SCREEN_MAP );
	if ( vecItemAdDefs.Count() > 0 )
	{
		if ( m_pAdPanel->BSetItemKVs( pKVItemAd ) )
		{
			m_pAdPanel->InvalidateLayout( true, true );
		}
	}
}

void CQuestMapPanel::RegionSelected( KeyValues *pParams )
{
	uint32 nRegionDefindex = pParams->GetInt( "defindex" );
	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( nRegionDefindex );
	if ( !pRegion )
	{
		Assert( false );
		return;
	}

	SetRegion( pRegion, true );
}

void CQuestMapPanel::RegionBackout()
{
	// Pop the top!
	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_currentRegion.defindex() );
	if ( !pRegion )
		return;

	const CQuestMapRegion* pParentRegion = pRegion->GetParent();
	if ( !pParentRegion )
		return;

	SetRegion( pParentRegion, false );
}

void CQuestMapPanel::DisableMouseBlocker()
{
	SetControlVisible( "MouseBlocker", false );
}


void CQuestMapPanel::FireTurnInStateEvent( KeyValues* pParams )
{
	auto pEvent = gameeventmanager->CreateEvent( "quest_turn_in_state" );
	if ( pEvent )
	{
		pEvent->SetInt( "state", pParams->GetInt( "state" ) );
		gameeventmanager->FireEventClientSide( pEvent );
	}
}

void CQuestMapPanel::OnPlaySoundEntry( KeyValues* pParams )
{
	PlaySoundEntry( pParams->GetString( "sound" ) );
}

void CQuestMapPanel::MapStateChangeSequence()
{
	auto pRegion = GetRegionPanel( m_currentRegion.defindex() );
	if ( !pRegion )
		return;

	PostMessage( pRegion->GetVPanel(), new KeyValues( "CloseNodeView" ), 0.5f );
	PostMessage( pRegion->GetVPanel(), new KeyValues( "ShowNodeUnlockChange" ), 1.f );
}

void CQuestMapPanel::OnCursorEntered()
{
	UpdateIntroState();
}

void CQuestMapPanel::OnCursorExited()
{
	UpdateIntroState();
}

const CQuestMapRegionPanel* CQuestMapPanel::GetRegionPanel( uint32 nRegionDefIndex ) const
{
	auto idx = m_mapRegions.Find( nRegionDefIndex );
	if ( idx == m_mapRegions.InvalidIndex() )
		return NULL;

	return m_mapRegions[ idx ];
}

//
// This is not cheap.  Try to do this as infrequently as possible
//
void CQuestMapPanel::UpdateControls( bool bIgnoreInvalidLayout )
{
	if ( !bIgnoreInvalidLayout && IsLayoutInvalid() )
		return;

	if ( !steamapicontext || 
		 !steamapicontext->SteamUser() )
	{
		return;
	}

	static ConVarRef mat_dxlevel( "mat_dxlevel" );
	if ( mat_dxlevel.GetInt() < 90 )
	{
		m_pIntroPanel->SetControlVisible( "StaticBG", false );
		m_pMapAreaPanel->SetControlVisible( "StaticOverlay", false );
	}

	if ( !GTFGCClientSystem()->BConnectedtoGC() )
		return;

	// Maybe they activated a pass?
	UpdatePassAdPanel();

	if ( m_vecRewardsShopItemPanels.IsEmpty() )
	{
		Panel* pItemScroller = m_pRewardsShopPanel->FindChildByName( "ItemsScroller", true );
		Assert( pItemScroller );
		if ( !pItemScroller )
			return;

		const DefinitionMap_t& mapStoreItems = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_STORE_ITEM );
		FOR_EACH_MAP( mapStoreItems, i )
		{
			const CQuestMapStoreItem* pStoreItem = (const CQuestMapStoreItem*)mapStoreItems[ i ];
			if ( pStoreItem->GetHeader().prefab_only() )
				continue;

			CQuestMapItemAdPanel* pItemAd = new CQuestMapItemAdPanel( pItemScroller, "ItemAd", pStoreItem );
			pItemAd->ApplySettings( m_pKVRewardItemPanels );
			pItemAd->MakeReadyForUse();
			pItemAd->SetItemTooltip( m_pMouseOverTooltip );
			m_vecRewardsShopItemPanels.AddToTail( pItemAd );
		}

		m_vecRewardsShopItemPanels.SortPredicate( []( const CQuestMapItemAdPanel* pLeft, const CQuestMapItemAdPanel* pRight ) 
		{
			uint32 nLeftGrup = pLeft->GetDefinition()->GetTypedMsg().sort_group();
			uint32 nRightGrup = pRight->GetDefinition()->GetTypedMsg().sort_group();

			if ( nLeftGrup != nRightGrup )
				return nLeftGrup < nRightGrup;

			uint8 rarityLeft = pLeft->GetItem().GetRarity();
			uint8 rarityRight = pRight->GetItem().GetRarity();

			if ( rarityLeft != rarityRight )
				return rarityLeft < rarityRight;


			CUtlString strLeft = CStrAutoEncode( pLeft->GetItem().GetItemName() ).ToString();
			CUtlString strRight = CStrAutoEncode( pRight->GetItem().GetItemName() ).ToString();

			int nDif = V_strnicmp( strLeft, strRight, Min( V_strlen( strLeft ), V_strlen( strRight ) ) );

			if ( nDif == 0 )
				return false;

			return nDif < 0;
		} );
	}
	else
	{
		FOR_EACH_VEC( m_vecRewardsShopItemPanels, i )
		{
			m_vecRewardsShopItemPanels[ i ]->InvalidateLayout();
		}
	}

	//
	// Create region panels for this map
	//
	const DefinitionMap_t& mapRegions = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_REGION );
	FOR_EACH_MAP_FAST( mapRegions, i )
	{
		const CQuestMapRegion* pRegion = ( const CQuestMapRegion* )mapRegions[ i ];

		auto idx = m_mapRegions.Find( pRegion->GetDefIndex() );
		if ( idx == m_mapRegions.InvalidIndex() )
		{
			CQuestMapRegionPanel* pRegionPanel = new CQuestMapRegionPanel( m_pMapAreaPanel, "Region", pRegion->GetID(), m_pQuestNodeViewPanel );
			pRegionPanel->AddActionSignalTarget( this );
			pRegionPanel->MakeReadyForUse();
			m_mapRegions.Insert( pRegion->GetDefIndex(), pRegionPanel );
		}
	}


	// TODO: Setup starting region.  Flag in the region?  Where your active contract is?
	// Now that we have the map def, we can set the starting region
	if ( !m_currentRegion.has_defindex() )
	{
		const CQuestMapRegion* pStartingRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( 0 );
		if ( !pStartingRegion )
			return;

		m_mapRegions[ pStartingRegion->GetDefIndex() ]->MakeReadyForUse();
		m_mapRegions[ pStartingRegion->GetDefIndex() ]->StartZoomTo( 0.5f, 0.5f, true );
		m_currentRegion.set_defindex( pStartingRegion->GetDefIndex() );
	}


	UpdateRegionVisibility();

	// Just got the map loaded.  Transition in
	if ( !m_bMapLoaded && IsVisible() )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestMap_MapLoaded", false );	
	}

	m_bMapLoaded = true;

	InvalidateLayout();
}


void CQuestMapPanel::UpdateStarsGlobalStatus()
{
	EditablePanel* pGlobalStatus = FindControl< EditablePanel >( "GlobalStatus", true );
	if ( pGlobalStatus )
	{
		auto lambdaSetTooltip = [&]( const char* pszPanelName, const char* pszLocToken )
		{
			Panel* pPanel = pGlobalStatus->FindChildByName( pszPanelName );
			if ( !pPanel )
				return;

			pPanel->SetTooltip( GetDashboardTooltip( k_eSmallFont ), pszLocToken );
		};

		auto pRegionDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_currentRegion );
		bool bShowStars = false;
		auto pStarTypeDef = pRegionDef->GetStarType();
		if ( pStarTypeDef )
		{
			bShowStars = true;
			pGlobalStatus->SetDialogVariable( "stars_available", CFmtStr( ": %d", GetQuestMapHelper().GetNumStarsAvailableToSpend( pStarTypeDef->GetDefIndex() ) ) );
			pGlobalStatus->SetDialogVariable( "stars_total", CFmtStr( ": %d/%d", GetQuestMapHelper().GetNumStarsEarned( pStarTypeDef->GetDefIndex() ), GetQuestMapHelper().GetNumStarsTotal( pStarTypeDef->GetDefIndex() ) ) );
		}

		pGlobalStatus->SetDialogVariable( "reward_credits", CFmtStr( ": %d", GetQuestMapHelper().GetNumRewardCredits() ) );
		pGlobalStatus->SetControlVisible( "AvailableStarsImage", bShowStars );
		pGlobalStatus->SetControlVisible( "AvailableStarsLabel", bShowStars );
		pGlobalStatus->SetControlVisible( "TotalStarsImage", bShowStars );
		pGlobalStatus->SetControlVisible( "TotalStarsLabel", bShowStars );
		lambdaSetTooltip( "BloodMoneyTooltip", "#TF_QuestMap_BloodMoney" );
		lambdaSetTooltip( "StarsAvailableTooltip", "#TF_QuestMap_StarsAvailableTooltip" );
		lambdaSetTooltip( "TotalStarsTooltip", "#TF_QuestMap_StarsTotalTooltip" );
	}
}

void CQuestMapPanel::UpdateRegionVisibility()
{
	// Make the right region show
	FOR_EACH_MAP( m_mapRegions, i )
	{
		uint32 nKey = m_mapRegions.Key( i );
		m_mapRegions[ i ]->SetVisible( nKey == m_currentRegion.defindex() && m_eScreenDisplay == SCREEN_MAP );
	}

	UpdateStarsGlobalStatus();
}

void CQuestMapPanel::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( IsVisible() && pObject->GetTypeID() == CEconItem::k_nTypeID && IsVisible() )
	{
		const CEconItem* pItem = assert_cast< const CEconItem* >( pObject );
		unacknowledged_item_inventory_positions_t reason = GetUnacknowledgedReason( pItem->GetInventoryToken() );
		if ( reason == UNACK_ITEM_QUEST_LOANER || reason == UNACK_ITEM_CYOA_BLOOD_MONEY_PURCHASE )
		{
			InventoryManager()->ShowItemsPickedUp( true, false );
		}

		static CSchemaItemDefHandle ContrackerItemDef( "Activated Campaign 3 Pass" );
		if ( pItem->GetItemDefinition() == ContrackerItemDef )
		{
			// Make the pass ad go away
			UpdatePassAdPanel();
			
			auto idx = m_mapRegions.Find( m_currentRegion.defindex() );
			if ( idx == m_mapRegions.InvalidIndex() )
				return;

			// Make the pass requirement tooltips refresh (go away)
			return m_mapRegions[ idx ]->UpdateControls();
		}
	}
}

void CQuestMapPanel::GoToCurrentQuest()
{
	MakeReadyForUse();

	auto* pActiveQuest = GetQuestMapHelper().GetActiveQuest();
	if ( !pActiveQuest )
		return;

	auto* pNode = GetQuestMapHelper().GetQuestMapNodeByID( pActiveQuest->GetSourceNodeID() );
	if ( !pNode )
		return;

	auto pRegionDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion  >( pNode->GetNodeDefinition()->GetRegionDefIndex() );
	if ( !pRegionDef )
		return;

	auto idx = m_mapRegions.Find( pRegionDef->GetDefIndex() );
	if ( idx == m_mapRegions.InvalidIndex() )
		return;

	auto *pRegionPanel = m_mapRegions[ idx ];
	if ( !pRegionPanel )
		return;

	// Go to the region with the node
	SetRegion( pRegionDef, true );

	// Delay the command to select the node a bit because we the UI needs to do the transitions first, or else
	// the arror from the node view panel will point to the wrong place
	PostMessage( pRegionPanel, new KeyValues( "NodeSelected", "node", pNode->GetNodeDefinition()->GetDefIndex() ), 0.5f );
}

//
// Debugging functions
//

CON_COMMAND( show_quest_log, "Show the quest map panel" )
{
	if ( GetQuestMapPanel()->IsVisible() )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide" );
		GetQuestMapPanel()->SetVisible( false );
	}
	else
	{
		CTFPlayer *pTFLocalPlayer = CTFPlayer::GetLocalTFPlayer();
		if ( pTFLocalPlayer && ( pTFLocalPlayer->IsTaunting() || pTFLocalPlayer->ShouldShowHudMenuTauntSelection() ) )
		{
			internalCenterPrint->Print( "#TF_CYOA_PDA_Taunting" );
		}
		else
		{
			engine->ClientCmd_Unrestricted( "gameui_activate" );
			GetQuestMapPanel()->SetVisible( true );
			GetQuestMapPanel()->GoToCurrentQuest();
		}
	}
}