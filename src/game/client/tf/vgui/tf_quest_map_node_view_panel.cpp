//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_quest_map_node_view_panel.h"
#include "tf_gc_client.h"
#include "tf_partyclient.h"
#include "tf_quest_map_node.h"
#include <vgui/ISurface.h>
#include "econ_quests.h"
#include "tf_hud_item_progress_tracker.h"
#include "tf_quest_map_controller.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include "item_model_panel.h"
#include <vgui/IInput.h>
#include "tf_quest_map_node_panel.h"
#include "tf_quest_map_editor_panel.h"

#define MAX_QUESTS_PER_NODE 3
void PromptOrFireCommand( const char* pszCommand );

void ConfirmSelectQuest( bool bConfirmed, void* pContext )
{
	if ( bConfirmed )
	{
		CQuestViewSubPanel* pQuestViewSubpanel = (CQuestViewSubPanel*)(pContext);
		pQuestViewSubpanel->OnConfirmSelectQuest();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CQuestObjectivePanel::CQuestObjectivePanel( Panel* pParent, const char* pszPanelname )
	: BaseClass( pParent, pszPanelname )
{
	m_pItemTrackerPanel = new CQuestProgressTrackerPanel( this, "ItemTrackerPanel", NULL, NULL, "resource/ui/quests/CYOA/QuestItemTrackerPanel_CYOA.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CQuestObjectivePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	SetTall( m_pItemTrackerPanel->GetYPos() + m_pItemTrackerPanel->GetContentTall() );

	if ( m_pItemTrackerPanel->IsLayoutInvalid() )
	{
		// This is so we get the correct layout of the tracker
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CQuestObjectivePanel::SetQuestData( const CQuest* pQuest, const CQuestDefinition* pQuestDef )
{
	if ( pQuestDef != m_pItemTrackerPanel->GetQuestDef() )
	{
		InvalidateLayout( false, true );
	}
	m_pItemTrackerPanel->SetQuest( pQuest );
	m_pItemTrackerPanel->SetQuestDef( pQuestDef );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CQuestObjectiveTooltip::CQuestObjectiveTooltip( vgui::Panel *parent, const char *text ) 
	: vgui::BaseTooltip( parent, text )
	, m_pObjectivePanel( NULL )
{
	m_hCurrentPanel = NULL;
	SetTooltipDelay( 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CQuestObjectiveTooltip::PerformLayout() 
{
	BaseClass::PerformLayout();

	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	CQuestViewSubPanel *pItemPanel = m_hCurrentPanel.Get();
	if ( m_pObjectivePanel && pItemPanel ) 
	{		
		m_pObjectivePanel->SetQuestData( NULL, m_hCurrentPanel->GetQuestDefindex() );

		int x,y;

		// If the panel is somewhere in a derived class, we need to get its position in our space
		if ( pItemPanel->GetParent() != m_pObjectivePanel->GetParent() )
		{
			int iItemAbsX, iItemAbsY;
			vgui::ipanel()->GetAbsPos( pItemPanel->GetVPanel(), iItemAbsX, iItemAbsY );
			int iParentAbsX, iParentAbsY;
			vgui::ipanel()->GetAbsPos( m_pObjectivePanel->GetParent()->GetVPanel(), iParentAbsX, iParentAbsY );

			x = (iItemAbsX - iParentAbsX);
			y = (iItemAbsY - iParentAbsY);
		}
		else
		{
			pItemPanel->GetPos( x, y );
		}

		int iXPos = 0;
		int iYPos = 0;
		int nXGap = XRES( 5 );

		if ( x + pItemPanel->GetWide() + m_pObjectivePanel->GetWide() + nXGap - XRES( 30 ) < m_pObjectivePanel->GetParent()->GetWide() )
		{
			// Going right
			iXPos = Min( x + nXGap + pItemPanel->GetWide(), m_pObjectivePanel->GetParent()->GetWide() - m_pObjectivePanel->GetWide() );
		}
		else
		{
			// Going left
			iXPos = Max( (int)XRES( 5 ), x - nXGap - (int)XRES( 15 ) - m_pObjectivePanel->GetWide() );
		}

		iYPos = y + ( pItemPanel->GetTall() / 2 ) - ( m_pObjectivePanel->GetTall() / 2 );

		m_pObjectivePanel->SetPos( iXPos, iYPos );
		m_pObjectivePanel->SetVisible( true );		
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CQuestObjectiveTooltip::ShowTooltip( Panel *currentPanel ) 
{ 
	if ( m_pObjectivePanel && currentPanel != m_hCurrentPanel.Get() ) 
	{
		CQuestViewSubPanel *pItemPanel = assert_cast<CQuestViewSubPanel *>(currentPanel);
		m_hCurrentPanel.Set( pItemPanel );
		pItemPanel->PostActionSignal( new KeyValues("ItemPanelEntered") );
		vgui::surface()->PlaySound( "ui/item_info_mouseover.wav" );
	}

	BaseClass::ShowTooltip( currentPanel );	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CQuestObjectiveTooltip::HideTooltip() 
{
	if ( m_pObjectivePanel ) 
	{
		m_pObjectivePanel->SetVisible( false ); 
	}
}


CQuestViewSubPanel::CQuestViewSubPanel( Panel* pParent,
										const char* pszPanelName,
										const CQuest* pQuest,
										const CQuestDefinition* pQuestDef,
										const CSOQuestMapNode& msgNodeData )
	: CExpandablePanel( pParent, pszPanelName )
	, m_pQuestDef( pQuestDef )
	, m_pQuest( pQuest )
	, m_pToolTip( NULL )
	, m_msgNodeData( msgNodeData )
{
	m_pObjectivePanel = new CQuestObjectivePanel( this, "objectives" );
	m_pActivateButton = new CExButton( this, "ActivateButton", (const char*)NULL, this, "activate_node" );
	m_pActivateButton->PassMouseTicksTo( this, true );
	m_pBGImage = new ImagePanel( this, "BGImage" );
	m_pAcceptTooltipHack = new EditablePanel( this, "EditableTooltip" );
	m_pInfo = new ImagePanel( this, "ObjectivesInfoImage" );

	m_pTurnInContainer = new EditablePanel( m_pObjectivePanel, "TurnInContainer" );

	ListenForGameEvent( "quest_response" );
}

void CQuestViewSubPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "Resource/UI/econ/QuestViewSubPanel.res" );
}

void CQuestViewSubPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );

	if ( !pNodeDef || !m_pQuestDef )
		return;

	m_pInfo->SetTooltip( m_pToolTip, "#TF_QuestView_ObjectivesInfo" );

	if ( pNodeDef->GetNumOfferedQuests() > 1 && !m_pQuest )
	{
		SetTooltip( m_pObjectiveTooltip, NULL );
	}
	else
	{
		SetTooltip( NULL, NULL );
	}

	// Show the info button if we're expanded
	SetControlVisible( "ObjectivesInfoImage", BIsExpanded() );

	int nCollapsedHeight = YRES( 45 );
	int nExpandedHeight = 0;

	m_pAcceptTooltipHack->SetVisible( false );
	SetControlVisible( "LockedIcon", false );

	
	bool bCanChoose = pNodeDef->BCanUnlock( GetQuestMapHelper() );

	const bool bActive = m_pQuest && m_pQuest->Obj().active();
	const bool bCompleted = m_pQuest && m_pQuest->BEarnedAllPointsForCategory( QUEST_POINTS_NOVICE ) &&
										m_pQuest->BEarnedAllPointsForCategory( QUEST_POINTS_ADVANCED ) &&
										m_pQuest->BEarnedAllPointsForCategory( QUEST_POINTS_EXPERT );
	const bool bCanActivate = bCanChoose || ( !bActive && !bCompleted && m_pQuest );
	const bool bCanTurnIn = GetQuestMapHelper().BCanNodeBeTurnedIn( m_msgNodeData.defindex() );


	SetControlVisible( "OrangeActiveBG", bActive );
	SetControlVisible( "BGImageDarkener", !bActive );

	Color colorInactive = GetColor( "TanDark" );
	Color colorActive = GetColor( "StoreGreen" );
	Color colorTanLight = GetColor( "TanLight" );

	m_pActivateButton->SetVisible( !bCompleted && !bActive && !bCanTurnIn );
	m_pActivateButton->SetEnabled( bCanActivate );
	// Make the activate button be very dark and uninviting when not enabled
	m_pActivateButton->SetDefaultColor( colorTanLight, bCanActivate ? colorActive : colorInactive );


	if ( !bCanActivate && !bCanChoose && !bActive )
	{
		wchar_t wszBuff[ 1024 ];
		memset( wszBuff, 0, sizeof( wszBuff ) );
		pNodeDef->GetCantUnlockReason( wszBuff, sizeof( wszBuff ) );

		m_pAcceptTooltipHack->SetVisible( true );
		m_pAcceptTooltipHack->SetTooltip( m_pToolTip, NULL );
		m_pAcceptTooltipHack->SetDialogVariable( "tiptext", wszBuff );
	}

	if ( m_bShowObjectives )
	{
		nExpandedHeight += YRES( 15 ); // Objectives label
		nExpandedHeight += m_pObjectivePanel->GetYPos() + m_pObjectivePanel->GetTall(); // Objectives is relative to Objectives label
	}

	SetExpandedHeight( nCollapsedHeight + nExpandedHeight );
	SetCollapsedHeight( nCollapsedHeight );

	if ( m_pQuestDef )
	{
		SetDialogVariable( "name", g_pVGuiLocalize->Find( m_pQuestDef->GetLocName() ) );
	}
	
	SetControlVisible( "TurnInContainer", bCanTurnIn, true );
}

void CQuestViewSubPanel::OnSizeChanged( int nWide, int nTall )
{
	BaseClass::OnSizeChanged( nWide, nTall );

	// So our parent will resize.  Yea it's a hack.
	if ( GetParent() )
	{
		GetParent()->GetParent()->InvalidateLayout();
	}
}

void CQuestViewSubPanel::OnCommand( const char *command )
{
	CSplitString strCommandArgs( command, " " ); 
	if ( FStrEq( "toggle_expand", command ) )
	{
		{
			PostActionSignal( new KeyValues( "QuestClicked", "name", GetName() ) );
		}
	}
	else if ( FStrEq( "unlock_quest", command ) || FStrEq( "activate_node", command )  )
	{
		// Already have a quest?
		if ( m_pQuest && m_pQuest->Obj().active() )
			return;
		//
		// The user wants to activate a new node.  Pop up a dialog to confirm
		//
		const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );

		// Make sure they can do what they want to do
		if ( !m_pQuest && !pNodeDef->BCanUnlock( GetQuestMapHelper() ) )
			return;

		// If they're about to select a multiple choice contract, make them confirm
		if ( pNodeDef->GetNumOfferedQuests() > 1 && !m_pQuest )
		{
			CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#TF_QuestView_ConfirmSelect_Title", "#TF_QuestView_ConfirmSelect_Multiple", "#TF_Coach_Yes", "#Cancel", &ConfirmSelectQuest, NULL );
			if ( pDialog )
			{
				pDialog->SetContext( this );
				pDialog->Show();
			}
		}
		else
		{
			OnConfirmSelectQuest();
		}

		InvalidateLayout();
	}
	else if ( FStrEq( "turn_in", command ) ) 
	{
		if ( !GetQuestMapHelper().BCanNodeBeTurnedIn( m_msgNodeData.defindex() ) )
			 return;

		GetQuestMapController().RedeemLootForNode( m_msgNodeData );

		BeginTurnInAnimation();
	}
	else if ( !Q_stricmp( "playsoundentry", strCommandArgs[ 0 ] ) )
	{
		if ( strCommandArgs.Count() == 2 )
		{
			PlaySoundEntry( strCommandArgs[ 1 ] );
		}
	}
}

void CQuestViewSubPanel::BeginTurnInAnimation()
{
	auto pNode = GetQuestMapHelper().GetQuestMapNode( m_msgNodeData.defindex() );
	if ( !pNode )
		return;

	m_bTurningIn = true;
	m_pTurnInContainer->SetVisible( false );

	KeyValues* pKVParams = new KeyValues( "StartTurnInAnimation" );
	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		if ( m_pQuest->GetEarnedPoints( (EQuestPoints)i ) && !pNode->BIsMedalEarned( (EQuestPoints)i ) )
		{
			pKVParams->SetBool( CFmtStr( "%d", i ), true );
		}
	}

	m_pObjectivePanel->PostMessageToChild( "ItemTrackerPanel", pKVParams );
}

void CQuestViewSubPanel::SetQuestData( const CQuest* pQuest, const CQuestDefinition* pQuestDef, bool bShowObjectives )
{
	// When it's the same quest coming back in, we can figure out what changed and play animations
	bool bQuestChange = m_pQuest != pQuest;
	bool bSameQuestDef = m_pQuestDef == pQuestDef && pQuestDef;
	m_bShowObjectives = bShowObjectives;
	m_pQuest = pQuest;
	m_pQuestDef = pQuestDef;

	m_pObjectivePanel->SetQuestData( pQuest, pQuestDef );

	m_pBGImage->SetImage( pQuestDef->GetNodeImageFileName() );

	if ( !bSameQuestDef || bQuestChange )
	{
		m_pObjectivePanel->InvalidateLayout( true, true );
		m_pObjectivePanel->MakeReadyForUse();
		InvalidateLayout( true );
		MakeReadyForUse();
	}
}

void CQuestViewSubPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( "quest_response", event->GetName() ) )
	{
		int nRequest = event->GetInt( "request" );
		bool bSuccess = event->GetBool( "success" );


		if ( nRequest == k_EMsgGC_QuestMapUnlockNode && bSuccess )
		{
			CMsgGCQuestMapUnlockNode msg;
			msg.ParseFromString( event->GetString( "msg" ) );

			if ( !m_pQuestDef || msg.quest_defindex() != m_pQuestDef->GetDefIndex() )
				return;

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestViewSubPanel_QuestPurchased" );	
			HideSelectQuestInfo();
			m_pObjectiveTooltip->HideTooltip();
			SetTooltip( NULL, NULL );
			InvalidateLayout();
		}
		else if ( nRequest == k_EMsgGCQuestNodeTurnIn )
		{
			if ( !bSuccess )
			{
				// TODO: Do something interesting!
				Assert( false );
			//	InvalidateLayout();
				return;
			}
		}
	}
}

void CQuestViewSubPanel::SetNodeData( const CSOQuestMapNode& msgNodeData )
{
	m_msgNodeData = msgNodeData;
	InvalidateLayout();
}

void CQuestViewSubPanel::OnConfirmSelectQuest()
{
	GetQuestMapController().SelectNodeQuest( m_msgNodeData, m_pQuestDef->GetDefIndex() );
}

void CQuestViewSubPanel::OnShowTurnInSuccess()
{
	
}

void CQuestViewSubPanel::OnCursorEntered()
{
	if ( !m_pQuest )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestViewSubPanel_SelectMouseOver" );
	}
}

void CQuestViewSubPanel::OnCursorExited()
{
	if ( !m_pQuest )
	{
		HideSelectQuestInfo();
	}
}

void CQuestViewSubPanel::HideSelectQuestInfo()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestViewSubPanel_SelectMouseExit" );
}

CQuestNodeViewPanel::CQuestNodeViewPanel( Panel *pParent, const char *pszPanelname )
	: BaseClass( pParent, pszPanelname )
	, m_pItemToolTip( NULL )
	, m_pToolTip( NULL )
	, m_bQuestCreated( false )
{
	m_pNodeStateBorder = new Panel( this, "StateBorderOverlay" );
	m_pTitleLabel = new Label( this, "TitleLabel", (const char*)NULL );
	m_pContentContainer = new EditablePanel( this, "ContentContainer" );
	m_pRewardsContainer = new EditablePanel( m_pContentContainer, "RewardContainer" );
	m_pRewardsInfo = new ImagePanel( m_pRewardsContainer, "RewardsInfoImage" );
	m_pContractsInfo = new ImagePanel( m_pContentContainer, "ContractsInfoImage" );
	m_pRewardItemPanel = new CItemModelPanel(  m_pRewardsContainer, "RewardItemModelPanel" );

	for( int i=0; i < MAX_QUESTS_PER_NODE; ++i )
	{
		CQuestViewSubPanel* pQuestTrackerPanel = new CQuestViewSubPanel( m_pContentContainer
																		 , CFmtStr( "QuestOption%d", i + 1 )
																		 , NULL
																		 , NULL
																		 , m_msgNodeData );
		pQuestTrackerPanel->SetTextTooltip( m_pToolTip );
		m_vecQuestSubPanels.AddToTail( pQuestTrackerPanel );
		pQuestTrackerPanel->AddActionSignalTarget( this );
	}

	ListenForGameEvent( "quest_response" );
	ListenForGameEvent( "quest_request" );
	ListenForGameEvent( "quest_turn_in_state" );
}

void CQuestNodeViewPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/QuestDefinitionViewPanel.res" );

	FOR_EACH_VEC( m_vecQuestSubPanels, i )
	{
		m_vecQuestSubPanels[ i ]->InvalidateLayout( true, true );
	}

	UpdateQuestSubPanels();
}

void CQuestNodeViewPanel::UpdateQuestSubPanels()
{
	const CQuest* pQuest = NULL;
	if ( m_msgNodeData.has_node_id() )
	{
		pQuest = GetQuestMapHelper().GetQuestForNode( m_msgNodeData.node_id() );
	}

	if ( pQuest )
	{
		// We've got a quest.  Show only how one quest subpanel, and hide the rest
		for ( int i=0; i < m_vecQuestSubPanels.Count(); ++i )
		{
			if ( i == 0 )
			{
				m_vecQuestSubPanels[ i ]->SetQuestData( pQuest, pQuest->GetDefinition(), true );
				m_vecQuestSubPanels[ i ]->SetVisible( true );
				m_pRewardsContainer->PinToSibling( m_vecQuestSubPanels[ i ]->GetName(), PIN_TOPLEFT, PIN_BOTTOMLEFT );
			}
			else
			{
				m_vecQuestSubPanels[ i ]->SetVisible( false );
			}
		}
	}
	else
	{
		// We don't have a quest selected yet.  Show all of the options.
		const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );
		CUtlVector< const CQuestDefinition* > vecQuestDefs;
		pNodeDef->GetOfferedQuests( vecQuestDefs );

		Assert( vecQuestDefs.Count() <= MAX_QUESTS_PER_NODE );

		int i=0;
		for ( i=0; i < vecQuestDefs.Count() && i < MAX_QUESTS_PER_NODE; ++i )
		{
			m_vecQuestSubPanels[ i ]->SetQuestData( NULL, vecQuestDefs[ i ], pNodeDef->GetNumOfferedQuests() == 1 );
			m_vecQuestSubPanels[ i ]->SetVisible( true );

			// Pin the rewards to the bottom-most guy
			m_pRewardsContainer->PinToSibling( m_vecQuestSubPanels[ i ]->GetName(), PIN_TOPLEFT, PIN_BOTTOMLEFT );
		}

		// Hide any extras
		for ( i; i < m_vecQuestSubPanels.Count(); ++i )
		{
			m_vecQuestSubPanels[ i ]->SetVisible( false );
		}
	}

	m_nExpandedHeight = GetExpandedHeight();
}

void CQuestNodeViewPanel::PerformLayout()
{

	if ( m_bTurningIn )
		return;

	// Need a valid quest defindex before we can fill anything out
	const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );
	if ( pNodeDef == NULL )
		return;

	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( pNodeDef->GetDefIndex() );
	// We may or may not have one of these
	const CQuest* pQuest = GetQuestMapHelper().GetQuestForNode( m_msgNodeData.node_id() );

	const bool bHasQuest = pNode && pQuest && pNode->GetSelectedQuest() != NULL;
	const bool bQuestActive = pQuest && pQuest->Obj().active();
	const bool bAnyCompleted = m_msgNodeData.star_0_earned() ||
		m_msgNodeData.star_1_earned() ||
		m_msgNodeData.star_2_earned();
	const bool bLootEarned = pNode && pNode->BHasLootBeenClaimed();

	SetDialogVariable( "name", g_pVGuiLocalize->Find( pNodeDef->GetNameLocToken() ) );

	//
	// Setup expire label
	//
	{
		CRTime timeExpire( k_RTime32Infinite ); // Forever, which is 2038.
		if ( pQuest )
		{
			// If we have a quest chosen, use that
			timeExpire = pQuest->GetDefinition()->GetExpireTime();
		}
		else
		{
			// If we don't look at all offered quests
			for( uint32 i=0; i < pNodeDef->GetNumOfferedQuests(); ++i )
			{
				timeExpire = Min( timeExpire, CRTime( pNodeDef->GetOfferedQuest( i )->GetExpireTime() ) );
			}
		}

		// Only show expire time for operations that end within the next year
		CRTime rtSoonestToShow( CRTime::RTime32TimeCur() );
		rtSoonestToShow = rtSoonestToShow.DateAdd( 1, k_ETimeUnitYear );
	
		if ( timeExpire < CRTime::RTime32TimeCur() || timeExpire > rtSoonestToShow )
		{
			SetDialogVariable( "expire_time", "" );
		}
		else
		{
			// Show "Expires <time>" in the label
			char szTimeBuffer[ k_RTimeRenderBufferSize ];
			CRTime::RTime32ToString( timeExpire.GetRTime32(), szTimeBuffer );
			wchar_t wszExpire[ 256 ];
			g_pVGuiLocalize->ConstructString_safe( wszExpire, g_pVGuiLocalize->Find( "#TF_QuestExpirationWarning" ), 1, CStrAutoEncode( szTimeBuffer ).ToWString() );
			SetDialogVariable( "expire_time", wszExpire );
		}
	}

	//
	// Figure out the tooltip text in the Contract area
	//
	if ( bHasQuest )
	{
		m_pContractsInfo->SetTooltip( m_pToolTip, "#TF_QuestView_ActiveContractInfo" );
	}
	else
	{
		m_pContractsInfo->SetTooltip( m_pToolTip, pNodeDef->GetNumOfferedQuests() == 1 ? "#TF_QuestView_SingleContractsInfo" : "#TF_QuestView_MultipleContractsInfo" );
	}

	//
	// Active quest tracker has variable tall.  m_pObjectivesContainer needs to resize to hold it all
	//
	int nQuestY = 0;
	FOR_EACH_VEC( m_vecQuestSubPanels, i )
	{
		CQuestViewSubPanel* pQuestTrackerPanel = m_vecQuestSubPanels[ i ];

		if ( i == 0 )
		{
			nQuestY = pQuestTrackerPanel->GetYPos();
		}

		nQuestY += pQuestTrackerPanel->GetTall();
	}

	int nNumCredits = pNodeDef->GetRewardCredits();
	bool bHasAReward = nNumCredits > 0 || m_pRewardItemPanel->IsVisible();

	m_pRewardsContainer->SetVisible( bHasAReward );
	if ( bHasAReward )
	{
		//
		// Cash container.  Setup all the medals that are earned
		//
		EditablePanel* pBloodMoneyContainer = m_pRewardsContainer->FindControl< EditablePanel >( "BloodMoneyContainer", true );
		if ( pBloodMoneyContainer )
		{
			pBloodMoneyContainer->SetVisible( nNumCredits > 0 );
			if ( nNumCredits > 0 )
			{
				pBloodMoneyContainer->SetTooltip( m_pToolTip, NULL );
				pBloodMoneyContainer->SetDialogVariable( "tiptext", g_pVGuiLocalize->Find( "#TF_QuestMap_BloodMoney" ) );
				pBloodMoneyContainer->SetControlVisible( "BloodMoneyObtainedIndicator", m_msgNodeData.loot_claimed() );
				pBloodMoneyContainer->SetDialogVariable( "cash", CFmtStr( "x%d", nNumCredits ).Get() );

				ImagePanel* pCashImage = pBloodMoneyContainer->FindControl< ImagePanel >( "CashImage" );
				if ( pCashImage )
				{
					switch ( pNodeDef->GetCashRewardType() )
					{
					case CASH_REWARD_LARGE:		pCashImage->SetImage( "cyoa/cyoa_cash_large" ); break;
					case CASH_REWARD_MEDIUM:	pCashImage->SetImage( "cyoa/cyoa_cash_medium" ); break;
					case CASH_REWARD_SMALL:		pCashImage->SetImage( "cyoa/cyoa_cash_small" ); break;
					case CASH_REWARD_NONE:
					default:
						Assert( false );
					}
				}
			}

			pBloodMoneyContainer->SetPos( m_pRewardItemPanel->IsVisible() ? XRES( 5 )
										 : ( m_pRewardsContainer->GetWide() / 2.f ) - ( pBloodMoneyContainer->GetWide() / 2.f )
										 , pBloodMoneyContainer->GetYPos() );
		}

		//
		// Item panel
		//
		if ( m_pRewardItemPanel->IsVisible() )
		{
			m_pRewardItemPanel->SetPos( pBloodMoneyContainer->IsVisible() ? m_pRewardsContainer->GetWide() - m_pRewardItemPanel->GetWide() - XRES( 5 )
										: ( m_pRewardsContainer->GetWide() / 2.f ) - ( pBloodMoneyContainer->GetWide() / 2.f )
										, m_pRewardItemPanel->GetYPos() );

		}


		//
		// Rewards container
		//
		{
			// Check mark over the loot if they've already got it
			m_pRewardsContainer->SetControlVisible( "RewardObtainedIndicator", bLootEarned, true );

			// Update the tooltip to reflect what they can get
			m_pRewardsInfo->SetTooltip( m_pToolTip, "#TF_QuestView_RewardsInfo" );
		}
	}

	bool bAnyPointsLeftToEarn = false;
	if ( pQuest )
	{
		for( int i = 0; !bAnyPointsLeftToEarn && i < EQuestPoints_ARRAYSIZE; ++i )
		{
			bAnyPointsLeftToEarn = pQuest->BEarnedAllPointsForCategory( (EQuestPoints)i ) == false;
		}
	}

	bool bTurnInVisible = GetQuestMapHelper().BCanNodeBeTurnedIn( pNodeDef->GetDefIndex() );

	if ( bTurnInVisible )
	{
		m_pNodeStateBorder->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( "CYOANodeViewBorder_TurnIn" ) );
		m_pTitleLabel->SetFgColor( GetColor( "TanLight") );
	}
	else if( bQuestActive )
	{
		m_pNodeStateBorder->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( "CYOANodeViewBorder_Active" ) );
		m_pTitleLabel->SetFgColor( GetColor( "TanLight" ) );
	}
	else
	{
		m_pNodeStateBorder->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( "CYOANodeViewBorder_Inactive" ) );
		m_pTitleLabel->SetFgColor( GetColor( "QuestMap_ActiveOrange" ) );
	}

	UpdateHeights();

	// Reposition now that we have new dimensions
	UpdatePosition();

	BaseClass::PerformLayout();

	// Manually position the expired label such that it touches the bottom
	Panel* pExpireLabel = FindChildByName( "ExpireLabel" );
	if ( pExpireLabel )
	{
		pExpireLabel->SetPos( pExpireLabel->GetXPos(), GetTall() - pExpireLabel->GetTall() ); 
	}
}

void CQuestNodeViewPanel::UpdateHeights()
{
	//
	// Expanded
	//
	{
		int nFullTall = 0;
		FOR_EACH_VEC( m_vecQuestSubPanels, i )
		{
			if ( !m_vecQuestSubPanels[ i ]->IsVisible() )
				continue;

			if ( nFullTall == 0 )
			{
				nFullTall = m_vecQuestSubPanels[ i ]->GetYPos();
			}
			else
			{
				nFullTall += m_vecQuestSubPanels[ i ]->GetYPos();
			}
		
			nFullTall += m_vecQuestSubPanels[ i ]->GetTall();
		}

		if ( m_pRewardsContainer->IsVisible() )
		{
			nFullTall += m_pRewardsContainer->GetTall();
		}

	//	nFullTall += YRES( 6 );
		nFullTall = Min( nFullTall, GetParent()->GetTall() - (int)YRES( 10 ) );
		SetExpandedHeight( nFullTall );
	}
	
	//
	// Collapsed
	//
	{
		int nFullTall = 0;
		FOR_EACH_VEC( m_vecQuestSubPanels, i )
		{
			if ( !m_vecQuestSubPanels[ i ]->IsVisible() )
				continue;

			if ( nFullTall == 0 )
			{
				nFullTall = m_vecQuestSubPanels[ i ]->GetYPos();
			}
			else
			{
				nFullTall += m_vecQuestSubPanels[ i ]->GetYPos();
			}

			nFullTall += m_vecQuestSubPanels[ i ]->GetTall();
		}

		if ( m_pRewardsContainer->IsVisible() )
		{
			nFullTall += m_pRewardsContainer->GetTall();
		}

		nFullTall += YRES( 15 );

		m_nCollapsedHeight = nFullTall;
	}
}

void CQuestNodeViewPanel::SelectedQuestArrived()
{
	m_bQuestCreated = false;
	UpdateQuestSubPanels();
	InvalidateLayout();
}

void CQuestNodeViewPanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "debug_menu" ) )
	{
		return;
	}
	else if ( FStrEq( pCommand, "close" ) ) 
	{
		SetVisible( false );
	}
	else if ( FStrEq( "turn_in", pCommand ) ) 
	{
		const CQuest* pQuest = NULL;
		if ( m_msgNodeData.has_node_id() )
		{
			pQuest = GetQuestMapHelper().GetQuestForNode( m_msgNodeData.node_id() );
		}

		if ( !pQuest || !pQuest->BEarnedAllPointsForCategory( QUEST_POINTS_NOVICE ) )
			return;

		GetQuestMapController().RedeemLootForNode( m_msgNodeData );
	}

	BaseClass::OnCommand( pCommand );
}

void CQuestNodeViewPanel::OnThink()
{
	BaseClass::OnThink();

	if ( m_bQuestCreated )
	{
		SelectedQuestArrived();
	}
}

void CQuestNodeViewPanel::Paint()
{
	BaseClass::Paint();

	Color colorActive = GetColor( "QuestMap_ActiveOrange" );

	static int snWhiteTextureID = -1;
	if ( snWhiteTextureID == -1 )
	{
		snWhiteTextureID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( snWhiteTextureID, "vgui/white" , true, false);
		if (snWhiteTextureID == -1)
			return;

	}

	vgui::surface()->DrawSetTexture( snWhiteTextureID );
	vgui::surface()->DrawSetColor( colorActive );

	vgui::Vertex_t verts[ 3 ];

	const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );
	if ( !pNodeDef )
		return;

	int nX, nY;
	GetPos( nX, nY );

	int nNodeX = m_nNodePanelX + m_nNodePanelWide / 2;
	int nNodeY = m_nNodePanelY + m_nNodePanelTall / 2;

	// We want to draw to the edge of the node.  The node uses this value for
	// it's radius, so let's use it too
	const float flArrowHeight = YRES( 10 );

	if ( nNodeX < nX )
	{
		// Node is to the left of us
		verts[ 0 ].m_Position.x = nNodeX - nX + YRES( node_medium_radius );
		verts[ 0 ].m_Position.y = nNodeY - nY;
		verts[ 0 ].m_TexCoord.x = verts[ 0 ].m_Position.x;
		verts[ 0 ].m_TexCoord.y = verts[ 0 ].m_Position.y;

		verts[ 1 ].m_Position.x = 0;
		verts[ 1 ].m_Position.y = nNodeY - flArrowHeight - nY;
		verts[ 1 ].m_TexCoord.x = verts[ 1 ].m_Position.x;
		verts[ 1 ].m_TexCoord.y = verts[ 1 ].m_Position.y;

		verts[ 2 ].m_Position.x = 0;
		verts[ 2 ].m_Position.y = nNodeY + flArrowHeight - nY;
		verts[ 2 ].m_TexCoord.x = verts[ 2 ].m_Position.x;
		verts[ 2 ].m_TexCoord.y = verts[ 2 ].m_Position.y;
	}
	else
	{
		// Node is to the right of us
		verts[ 0 ].m_Position.x = nNodeX - nX - YRES( node_medium_radius );
		verts[ 0 ].m_Position.y = nNodeY - nY;
		verts[ 0 ].m_TexCoord.x = verts[ 0 ].m_Position.x;
		verts[ 0 ].m_TexCoord.y = verts[ 0 ].m_Position.y;

		verts[ 1 ].m_Position.x = GetWide();
		verts[ 1 ].m_Position.y = nNodeY + flArrowHeight - nY;
		verts[ 1 ].m_TexCoord.x = verts[ 1 ].m_Position.x;
		verts[ 1 ].m_TexCoord.y = verts[ 1 ].m_Position.y;

		verts[ 2 ].m_Position.x = GetWide();
		verts[ 2 ].m_Position.y = nNodeY - flArrowHeight - nY;
		verts[ 2 ].m_TexCoord.x = verts[ 2 ].m_Position.x;
		verts[ 2 ].m_TexCoord.y = verts[ 2 ].m_Position.y;
	}

	vgui::surface()->DrawTexturedPolygon( 3, verts, false );
}

void CQuestNodeViewPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( "quest_response", event->GetName() ) )
	{
		int nRequest = event->GetInt( "request" );
		bool bSuccess = event->GetBool( "success" );

		if ( nRequest == k_EMsgGC_QuestMapUnlockNode && bSuccess )
		{
			CMsgGCQuestMapUnlockNode msg;
			msg.ParseFromString( event->GetString( "msg" ) );

			// If this msg wasnt for the node we're viewing, then we don't care
			if ( msg.node_defindex() != m_msgNodeData.defindex() )
				return;

			// Update our node data because the SObject that represents us just came in
			const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( m_msgNodeData.defindex() );
			Assert( pNode );

			if ( pNode )
			{
				SetData( pNode->Obj() );
			}

			UpdateQuestSubPanels();
		}
	}
	else if ( FStrEq( "quest_request", event->GetName() ) )
	{
		int nRequest = event->GetInt( "request" );

		if ( nRequest == k_EMsgGC_QuestMapUnlockNode )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestNodeView_QuestPurchased" );	
		}
	}
	else if ( FStrEq( event->GetName(), "quest_turn_in_state" ) )
	{
		EQuestTurnInState eState = (EQuestTurnInState)event->GetInt( "state" );

		auto lambdaHighlightPanel = [&]( const char* pszName, bool bShow )
		{
			Panel* pPanel = m_pRewardsContainer->FindChildByName( pszName, true );
			if ( pPanel && bShow )
			{
				pPanel->SetVisible( bShow );
				PlaySoundEntry( "MatchMaking.XPChime" );
				Color colorGreen = GetColor( "CreditsGreen" );
				Color colorHighlight = colorGreen;
				BrigthenColor( colorHighlight, 60 );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "bgcolor", colorHighlight, 0.f, 0.f, AnimationController::INTERPOLATOR_LINEAR, 0, true, false );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pPanel, "bgcolor", colorGreen, 0.2f, 1.5f, AnimationController::INTERPOLATOR_LINEAR, 0, false, false );
			}
		};

		auto& msgReport = GetQuestMapController().GetMostRecentProgressReport();

		switch( eState )
		{
		case TURN_IN_BEGIN:
		{
			m_bTurningIn = true;
			m_msgPreTurnInSoData = m_msgNodeData;
			PlaySoundEntry( "Quest.TurnInDecode" );
			break;
		}
	
		case TURN_IN_SHOW_BLOOD_MONEY_EARNED:
		{
			// Enable earned indicator
			lambdaHighlightPanel( "BloodMoneyObtainedIndicator", msgReport.reward_credits_earned() > 0 );
			break;
		}

		case TURN_IN_SHOW_ITEMS_EARNED_EARNED:
		{
			// Enable earned indicator
			lambdaHighlightPanel( "RewardObtainedIndicator", msgReport.items_earned_size() > 0 );
			break;
		}

		case TURN_IN_HIDE_NODE_VIEW:
		{
			SetVisible( false );
			break;
		}
		
		case TURN_IN_COMPLETE:
		{
			m_bTurningIn = false;
			break;
		}

		// Nothing to do for these
		case TURN_IN_SHOW_SUCCESS:
		case TURN_IN_HIDE_SUCCESS:
		case TURN_IN_SHOW_FAILURE:
		case TURN_IN_HIDE_FAILURE:
			break;
		};
	}
}

void CQuestNodeViewPanel::SetTextTooltip( CTFTextToolTip* pTooltip )
{
	m_pToolTip = pTooltip; 

	FOR_EACH_VEC( m_vecQuestSubPanels, i )
	{
		m_vecQuestSubPanels[ i ]->SetTextTooltip( pTooltip );
	}
}

void CQuestNodeViewPanel::SetObjectiveTooltip( CQuestObjectiveTooltip* pObjectiveTooltip )
{
	FOR_EACH_VEC( m_vecQuestSubPanels, i )
	{
		m_vecQuestSubPanels[ i ]->SetObjectiveTooltip( pObjectiveTooltip );
	}
}

void CQuestNodeViewPanel::SetData( const CSOQuestMapNode& msgNodeData )
{
	if ( m_msgNodeData.defindex() != msgNodeData.defindex() )
	{
		m_bHasItemsToShow = false;
	}

	m_msgNodeData.CopyFrom( msgNodeData );

	UpdateQuestSubPanels();

	const CQuest* pQuest = NULL;
	if ( m_msgNodeData.has_node_id() )
	{
		pQuest = GetQuestMapHelper().GetQuestForNode( m_msgNodeData.node_id() );
	}

	const CQuestMapNodeDefinition* pNodeDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapNodeDefinition >( m_msgNodeData.defindex() );

	FOR_EACH_VEC( m_vecQuestSubPanels, i )
	{
		bool bSingleQuestNode = pNodeDef && pNodeDef->GetNumOfferedQuests() == 1;

		bool bExpanded = ( bSingleQuestNode || pQuest ) && i == 0;
		
		m_vecQuestSubPanels[ i ]->SetCollapsed( !bExpanded, true );
		m_vecQuestSubPanels[ i ]->SetNodeData( msgNodeData );
	}

	InvalidateLayout();

	//
	// Rewards
	//

	CEconItemView tempItem;
	if ( pNodeDef->GetRewardItem( tempItem ) )
	{
		m_pRewardItemPanel->SetItem( &tempItem );
		m_pRewardItemPanel->SetVisible( true );
		m_pRewardItemPanel->SetMouseInputEnabled( true );
		m_pRewardItemPanel->SetTooltip( m_pItemToolTip, "" );
	}
	else
	{
		m_pRewardItemPanel->SetVisible( false );
	}

	//
	// Show different strings based on > 1 quest or not or quest chosen
	//
	if ( pNodeDef )
	{
		if ( m_msgNodeData.selected_quest_def() != 0 )
		{
			m_pContractsInfo->SetTooltip( m_pToolTip, "#TF_QuestView_ContractChosenInfo" );
		}
		else
		{
			m_pContractsInfo->SetTooltip( m_pToolTip, pNodeDef->GetNumOfferedQuests() > 1 ? "#TF_QuestView_MultipleContractsInfo" : "#TF_QuestView_SingleContractsInfo" );
		}
	}
}


void CQuestNodeViewPanel::UpdateQuestViewPanelForNode( CQuestMapNodePanel* pNodePanel )
{
	SetData( pNodePanel->GetLocalState() );

	pNodePanel->GetBounds( m_nNodePanelX, m_nNodePanelY, m_nNodePanelWide, m_nNodePanelTall );

	pNodePanel->ParentLocalToScreen( m_nNodePanelX, m_nNodePanelY );
	GetParent()->ScreenToLocal( m_nNodePanelX, m_nNodePanelY );

	UpdatePosition();
}

void CQuestNodeViewPanel::SetVisible( bool bVisible )
{
	BaseClass::SetVisible( bVisible );

	if ( !bVisible )
	{
		PostActionSignal( new KeyValues( "NodeViewClosed" ) );
	}
}

void CQuestNodeViewPanel::UpdatePosition()
{
	int nXPos = m_nNodePanelX + m_nNodePanelWide;
	int nYpos = m_nNodePanelY + ( m_nNodePanelWide / 2 ) - ( GetTall() / 2 );
	bool bLeftPinned = true;

	// Clipped on the right.  Go to the left of the node
	if ( nXPos + GetWide() > GetParent()->GetWide() )
	{
		nXPos = m_nNodePanelX - GetWide();
		bLeftPinned = false;
	}

	// Clipped on the left.  Go on the right side of the node
	if ( nXPos < 0 )
	{
		nXPos = m_nNodePanelX + m_nNodePanelWide;
		bLeftPinned = true;
	}

	const int nBottomBuffer = YRES( 30 );

	// Clipped on the bottom.  Align our buttom, with the bottom of the panel
	if ( nYpos + GetTall() > GetParent()->GetTall() - nBottomBuffer )
	{
		nYpos = GetParent()->GetTall() - GetTall() - nBottomBuffer;
	}

	const int nTopBuffer = YRES( 10 );

	// Clipped on the top.  Align our top with the top of the panel
	if ( nYpos < nTopBuffer )
	{
		nYpos = nTopBuffer;
	}

	// Move and take in the node's data
	SetPos( nXPos, nYpos );
}

void CQuestNodeViewPanel::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		const CQuest* pQuest = (const CQuest*)( pObject );
		if ( pQuest->GetSourceNodeID() == m_msgNodeData.node_id() )
		{
			m_bQuestCreated = true;
		}
	}
	else if ( pObject->GetTypeID() == CEconItem::k_nTypeID && IsVisible() )
	{
		const CEconItem* pItem = assert_cast< const CEconItem* >( pObject );
		unacknowledged_item_inventory_positions_t reason = GetUnacknowledgedReason( pItem->GetInventoryToken() );
		if ( reason == UNACK_ITEM_QUEST_OUTPUT || reason == UNACK_ITEM_QUEST_LOANER )
		{
			m_bHasItemsToShow = true;
		}
	}
}

void CQuestNodeViewPanel::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CQuestMapNode::k_nTypeID )
	{
		const CQuestMapNode* pNode = (const CQuestMapNode *)( pObject );
		if ( pNode->Obj().node_id() == m_msgNodeData.node_id() )
		{
			m_msgNodeData = pNode->Obj();
			InvalidateLayout();
		}
	}

	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		const CQuest* pQuest = (const CQuest*)pObject;
		if ( pQuest->GetSourceNodeID() == m_msgNodeData.node_id() )
		{
			UpdateQuestSubPanels();
			InvalidateLayout();
		}
	}
}

void CQuestNodeViewPanel::SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		const CQuest* pQuest = (const CQuest*)pObject;

		// Clear out any references to this quest in our sub panels. This really
		// only occurs when using debug commands, but let's be safe
		FOR_EACH_VEC( m_vecQuestSubPanels, i )
		{
			if ( m_vecQuestSubPanels[ i ]->GetQuestDefindex() && 
				 m_vecQuestSubPanels[ i ]->GetQuestDefindex()->GetDefIndex() == pQuest->GetDefinition()->GetDefIndex() )
			{
				m_vecQuestSubPanels[ i ]->SetQuestData( NULL, m_vecQuestSubPanels[ i ]->GetQuestDefindex(), false );
			}
		}
	}
}

void CQuestNodeViewPanel::QuestClicked( KeyValues *pParams )
{
	// Find the panel
	const char* pszName = pParams->GetString( "name" );
	int nIndex = m_vecQuestSubPanels.FindPredicate( [ &pszName ]
		( CQuestViewSubPanel* pPanel )
		{
			return FStrEq( pPanel->GetName(), pszName );
		}
	);

	// If we got a the guy, toggle him
	if ( nIndex != m_vecQuestSubPanels.InvalidIndex() )
	{
		// If he's being expanded, then make sure the others are collapsed
		FOR_EACH_VEC( m_vecQuestSubPanels, i )
		{
			m_vecQuestSubPanels[ i ]->SetCollapsed( i == nIndex ? false : true );
		}
	}
}

void CQuestNodeViewPanel::TurnInComplete()
{
	m_bTurningIn = false;
	InvalidateLayout( true );


}

