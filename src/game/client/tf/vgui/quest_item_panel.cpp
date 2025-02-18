//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "quest_item_panel.h"
#include "tf_hud_item_progress_tracker.h"
#include "quest_log_panel.h"
#include "c_tf_player.h"
#include "econ_item_description.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include "quest_objective_manager.h"
#include "econ_quests.h"
#include "confirm_dialog.h"
#include "tf_quest_restriction.h"
#include "item_model_panel.h"
#include "tf_gc_client.h"

#if 0

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>
															 
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

const float k_flQuestDecodeTime = 2.f;
const float k_flQuestTurnInTime = 5.f;
ConVar tf_quest_turn_in_confirm_opt_out( "tf_quest_turn_in_confirm_opt_out", "0", FCVAR_ARCHIVE, "If nonzero, don't confirm submitting a contract that does not have all of the bonus points" );

extern CQuestLogPanel *GetQuestLog();
extern CQuestTooltip* g_spTextTooltip;

extern const char *s_pszMMTypes[kMatchmakingTypeCount];
extern const char *s_pszGameModes[eNumGameCategories];

void SelectGroup( EMatchmakingGroupType eGroup, bool bSelected );
void SelectCategory( EGameCategory eCategory, bool bSelected );

void PromptOrFireCommand( const char* pszCommand );

static void ConfirmDiscardQuest( bool bConfirmed, void* pContext )
{
	CQuestItemPanel *pQuestItemPanel = ( CQuestItemPanel* )pContext;
	if ( pQuestItemPanel )
	{
		pQuestItemPanel->OnConfirmDelete( bConfirmed );
	}
}

static void ConfirmEquipLoaners( bool bConfirmed, void* pContext )
{
	CQuestItemPanel *pQuestItemPanel = ( CQuestItemPanel* )pContext;
	if ( pQuestItemPanel )
	{
		pQuestItemPanel->OnConfirmEquipLoaners( bConfirmed );
	}
}

static void ConfirmTurnInQuest( bool bConfirmed, void* pContext )
{
	if ( bConfirmed )
	{
		CQuestItemPanel *pQuestItemPanel = (CQuestItemPanel*)pContext;
		if ( pQuestItemPanel )
		{
			pQuestItemPanel->OnCompleteQuest();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: fill vecLoanerItems with loaners def indices from pQuest
//-----------------------------------------------------------------------------
static int GetLoanerListFromQuest( const CQuest *pQuest, CUtlVector< item_definition_index_t >& vecLoanerItems )
{
	if ( !pQuest || !pQuest->Obj().has_quest_id() )
		return 0;

	// loaners from the quest
	const CUtlVector< CTFRequiredQuestItemsSet >& vecQuestRequiredItems = pQuest->GetDefinition()->GetRequiredItemSets();
	FOR_EACH_VEC( vecQuestRequiredItems, i )
	{
		// don't add dups
		if ( vecLoanerItems.Find( vecQuestRequiredItems[i].GetLoanerItemDef() ) == vecLoanerItems.InvalidIndex() )
		{
			vecLoanerItems.AddToTail( vecQuestRequiredItems[i].GetLoanerItemDef() );
		}
	}

	return vecLoanerItems.Count();
}


//-----------------------------------------------------------------------------
// Purpose: fill vecGrantedLoaners with granted loaners from specific quest ID
//-----------------------------------------------------------------------------
static int GetLoanersFromLocalInventory( const itemid_t& questID, const CUtlVector< item_definition_index_t >& vecLoanerItems, CUtlVector< CEconItemView* >& vecGrantedLoaners )
{
	if ( vecLoanerItems.Count() > 0 )
	{
		CPlayerInventory *pLocalInv = InventoryManager()->GetLocalInventory();
		int nCount = pLocalInv->GetItemCount();
		for ( int i = 0; i < nCount; ++i )
		{
			CEconItemView* pItem = pLocalInv->GetItem( i );

			bool bIsLoaner = false;
			// check if the item is a loaner and is associated with this quest
			FOR_EACH_VEC( vecLoanerItems, iLoaner )
			{
				if ( vecLoanerItems[iLoaner] == pItem->GetItemDefIndex() && GetAssociatedQuestID( pItem ) == questID )
				{
					bIsLoaner = true;
					break;
				}
			}

			// already granted this loaner, remove from the list to give
			if ( bIsLoaner )
			{
				vecGrantedLoaners.AddToTail( pItem );
			}

			// found all given loaners
			if ( vecLoanerItems.Count() == vecGrantedLoaners.Count() )
			{
				break;
			}
		}
	}

	return vecGrantedLoaners.Count();
}


DECLARE_BUILD_FACTORY( CInputProxyPanel )
CInputProxyPanel::CInputProxyPanel( Panel *parent, const char *pszPanelName )
	: BaseClass( parent, pszPanelName )
{}

void CInputProxyPanel::AddPanelForCommand( EInputTypes eInputType, Panel* pPanel, const char* pszCommand )
{
	m_vecRedirectPanels[ eInputType ].AddToTail( { pPanel, pszCommand } );
}

void CInputProxyPanel::OnCursorMoved( int x, int y )
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_MOVE ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_MOVE ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_MOVE ][ i ].m_pszCommand, "x", x, "y", y ) );
	}
}

void CInputProxyPanel::OnCursorEntered()
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_ENTER ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_ENTER ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_ENTER ][ i ].m_pszCommand ) );
	}
}

void CInputProxyPanel::OnCursorExited()
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_EXIT ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_EXIT ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_EXIT ][ i ].m_pszCommand ) );
	}
}

void CInputProxyPanel::OnMousePressed(MouseCode code)
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_PRESS ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_PRESS ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_PRESS ][ i ].m_pszCommand, "code", code ) );
	}
}

void CInputProxyPanel::OnMouseDoublePressed(MouseCode code)
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_DOUBLE_PRESS ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_DOUBLE_PRESS ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_DOUBLE_PRESS ][ i ].m_pszCommand, "code", code ) );
	}
}

void CInputProxyPanel::OnMouseReleased(MouseCode code)
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_RELEASED ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_RELEASED ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_RELEASED ][ i ].m_pszCommand, "code", code ) );
	}
}

void CInputProxyPanel::OnMouseWheeled(int delta)
{
	FOR_EACH_VEC( m_vecRedirectPanels[ INPUT_MOUSE_WHEEL ], i )
	{
		PostMessage( m_vecRedirectPanels[ INPUT_MOUSE_WHEEL ][ i ].m_pPanel, new KeyValues( m_vecRedirectPanels[ INPUT_MOUSE_WHEEL ][ i ].m_pszCommand, "delta", delta ) );
	}
}


DECLARE_BUILD_FACTORY( CQuestStatusPanel )

CQuestStatusPanel::CQuestStatusPanel( Panel *parent, const char *pszPanelName )
	: EditablePanel( parent, pszPanelName )
	, m_pMovingContainer( NULL )
	, m_bShouldBeVisible( false )
{
	m_pMovingContainer = new EditablePanel( this, "movingcontainer" );
	m_transitionTimer.Invalidate();
}

void CQuestStatusPanel::SetShow( bool bShow )
{
	if ( bShow != m_bShouldBeVisible )
	{
		m_transitionTimer.Start( 0.6f );
	}
	m_bShouldBeVisible = bShow;
	SetVisible( m_bShouldBeVisible );
}

void CQuestStatusPanel::OnThink()
{
	BaseClass::OnThink();

	const int nStartY = m_bShouldBeVisible ? m_iHiddenY : m_iVisibleY;
	const int nEndY = m_bShouldBeVisible ? m_iVisibleY : m_iHiddenY;

	float flProgress = 1.f;
	if ( !m_transitionTimer.IsElapsed() )
	{
		flProgress = Bias( RemapValClamped( m_transitionTimer.GetElapsedTime(), 0.f , m_transitionTimer.GetCountdownDuration(), 0.f, 1.f ), 0.7f );
	}
	flProgress = RemapVal( flProgress, 0.f, 1.f, (float)nStartY, (float)nEndY );
 
	m_pMovingContainer->SetPos( m_pMovingContainer->GetXPos(), flProgress );
	SetVisible( m_bShouldBeVisible || flProgress > 0.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestItemPanel::CQuestItemPanel( Panel *parent, const char *pszPanelName, CQuest* pQuestItem, CScrollableQuestList* pQuestList )
	: EditablePanel( parent, pszPanelName )
	, m_pLiveQuest( NULL )
	, m_eState( STATE_NORMAL )
	, m_pTurnInContainer( NULL )
	, m_pTurnInDimmer( NULL )
	, m_pszCompleteSound( NULL )
	, m_pFrontFolderContainer( NULL )
	, m_pBackFolderContainer( NULL )
	, m_bCollapsed( true )
	, m_pQuestList( pQuestList )
	, m_pQuestPaperContainer( NULL )
	, m_pTitleButton( NULL )
	, m_pIdentifyContainer( NULL )
	, m_pIdentifyDimmer( NULL )
	, m_pKVCipherStrings( NULL )
	, m_pPhotoStatic( NULL )
	, m_pFlavorScrollingContainer( NULL )
	, m_pTurningInLabel( NULL )
	, m_pFindServerButton( NULL )
	, m_pLoanerContainerPanel( NULL )
	, m_pRequestLoanerItemsButton( NULL )
	, m_pEquipLoanerItemsButton( NULL )
	, m_pItemTrackerPanel( NULL )
	, m_pKVItemTracker( NULL )
	, m_pObjectiveExplanationLabel( NULL )
	, m_pEncodedStatus( NULL )
	, m_pInactiveStatus( NULL )
	, m_pReadyToTurnInStatus( NULL )
	, m_pExpirationLabel( NULL )
	, m_pTurnInButton( NULL )
	, m_bHasAllControls( false )
	, m_pDiscardButton( NULL )
	, m_pCompleteButton( NULL )
{
	m_quest.Obj().Clear();
	SetItem( pQuestItem );
	m_StateTimer.Invalidate();

	ListenForGameEvent( "quest_objective_completed" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "inventory_updated" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestItemPanel::~CQuestItemPanel()
{
	if ( m_pItemTrackerPanel )
	{
		m_pItemTrackerPanel->MarkForDeletion();
		m_pItemTrackerPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings ( pScheme );
	AddActionSignalTarget( GetQuestLog() );
	LoadResFileForCurrentItem();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::LoadResFileForCurrentItem()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	const char *pszResFile = "Resource/UI/quests/QuestItemPanel_Base.res";

	if ( m_quest.Obj().has_quest_id() )
	{
		const CQuestThemeDefinition *pTheme = m_quest.GetDefinition()->GetQuestTheme();
		if ( pTheme )
		{
			pszResFile = pTheme->GetQuestItemResFile();
		}
	}

	KeyValues *pConditions = new KeyValues( "conditions" );
	if ( pConditions )
	{
		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
		char szCondition[64];
		Q_snprintf( szCondition, sizeof( szCondition ), "if_%s", uilanguage );
		AddSubKeyNamed( pConditions, szCondition );
	}

	SetMouseInputEnabled( true );	// Slam this to true.  When panels get created, they'll inherit their parents' mouse enabled state
									// and if we've been fiddling with it, we might accidently create all child panels with mouse input disabled.
									// Setting this to true just before the controls are made gives them a chance to be mouse enabled if they want.
	LoadControlSettings( pszResFile, NULL, NULL, pConditions );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strReset );

	m_pMainContainer = FindControl<EditablePanel>( "MainContainer" );
	if ( m_pMainContainer )
	{
		m_pMainContainer->AddActionSignalTarget( this );
	}

	m_pQuestPaperContainer = FindControl<EditablePanel>( "QuestPaperContainer", true );
	m_pFrontFolderContainer = FindControl<EditablePanel>( "FrontFolderContainer", true );
	Assert( m_pFrontFolderContainer );
	if ( m_pFrontFolderContainer )
	{
		m_pFrontFolderImage = m_pFrontFolderContainer->FindControl<ImagePanel>( "FrontFolderImage", true );
		Assert( m_pFrontFolderImage );

		m_pEncodedStatus = m_pFrontFolderContainer->FindControl< CQuestStatusPanel >( "EncodedStatus", true );
		m_pInactiveStatus = m_pFrontFolderContainer->FindControl< CQuestStatusPanel >( "InactiveStatus", true );
		m_pReadyToTurnInStatus = m_pFrontFolderContainer->FindControl< CQuestStatusPanel >( "ReadyToTurnInStatus", true );
	}
	m_pBackFolderContainer = FindControl<EditablePanel>( "BackFolderContainer", true );
	Assert( m_pBackFolderContainer );
	if ( m_pBackFolderContainer )
	{
		m_pBackFolderImage = m_pBackFolderContainer->FindControl<ImagePanel>( "BackFolderImage", true );
		Assert( m_pBackFolderImage );
	}

	if ( m_pQuestPaperContainer )
	{
#if defined( STAGING_ONLY ) || defined( DEBUG )
		// don't do this in public
		m_pDiscardButton = new CExButton( m_pQuestPaperContainer, "Discard", "Discard", this, "discard_quest" );
		m_pDiscardButton->SetEnabled( true );
		m_pDiscardButton->SizeToContents();
		m_pDiscardButton->SetZPos( 101 );
		m_pDiscardButton->SetPos( 70, 40 );
		m_pDiscardButton->SetVisible( false );

		m_pCompleteButton = new CExButton( m_pQuestPaperContainer, "Complete", "Complete", this, "complete_quest" );
		m_pCompleteButton->SetEnabled( true );
		m_pCompleteButton->SizeToContents();
		m_pCompleteButton->SetZPos( 101 );
		m_pCompleteButton->SetPos( 100, 40 );
		m_pCompleteButton->SetVisible( false );
#endif // STAGING_ONLY || DEBUG

		m_pFindServerButton = m_pQuestPaperContainer->FindControl< CExButton >( "FindServerButton", true );

		m_pLoanerContainerPanel = m_pQuestPaperContainer->FindControl< EditablePanel >( "LoanerContainerPanel", true );
		if ( m_pLoanerContainerPanel )
		{
			m_pRequestLoanerItemsButton = m_pLoanerContainerPanel->FindControl< CExButton >( "RequestLoanerItemsButton", true );
			m_pEquipLoanerItemsButton = m_pLoanerContainerPanel->FindControl< CExButton >( "EquipLoanerItemsButton", true );
			for ( int i = 0; i < ARRAYSIZE( m_pLoanerItemModelPanel ); ++i )
			{
				m_pLoanerItemModelPanel[i] = m_pLoanerContainerPanel->FindControl< CItemModelPanel >( CFmtStr( "Loaner%dItemModelPanel", i + 1 ), true );
			}
		}

		m_pTitleButton = m_pQuestPaperContainer->FindControl<CExButton>( "TitleButton", true );
		m_pIdentifyContainer = m_pQuestPaperContainer->FindControl<EditablePanel>( "IdentifyButtonContainer", true );
		if ( m_pIdentifyContainer )
		{
			m_pIdentifyDimmer = m_pIdentifyContainer->FindControl<EditablePanel>( "Dimmer", true );
			m_pIdentifyButton = m_pIdentifyContainer->FindControl<CExButton>( "IdentifyButton", true );
		}
		Assert( m_pIdentifyContainer );

		m_pEncodedImage = m_pQuestPaperContainer->FindControl<ImagePanel>( "EncodedImage", true );

		m_pPhotoStatic = m_pQuestPaperContainer->FindControl<ImagePanel>( "StaticPhoto", true );
		Assert( m_pPhotoStatic );

		m_pFlavorScrollingContainer = m_pQuestPaperContainer->FindControl<CExScrollingEditablePanel>( "ScrollableBottomContainer", true );
		Assert( m_pFlavorScrollingContainer );

		if ( m_pFlavorScrollingContainer )
		{
			m_pObjectiveExplanationLabel = m_pFlavorScrollingContainer->FindControl< Label >( "QuestObjectiveExplanation", true );
		}

		CInputProxyPanel* pInputProxy = m_pQuestPaperContainer->FindControl< CInputProxyPanel >( "PaperInputProxyPanel", true );
		if ( pInputProxy )
		{
			// Make the scroller scroll
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_WHEEL, m_pFlavorScrollingContainer, "MouseWheeled" );

			// Make the title glow
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_ENTER, m_pTitleButton, "CursorEntered" );
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_EXIT, m_pTitleButton, "CursorExited" );

			// Capture clicks to expand/contract
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_RELEASED, this, "MouseReleased" );
		}

		m_pTurnInContainer = m_pQuestPaperContainer->FindControl< EditablePanel >( "TurnInContainer" );
		Assert( m_pTurnInContainer );
		if ( m_pTurnInContainer )
		{
			m_pTurnInDimmer = m_pTurnInContainer->FindControl< EditablePanel >( "Dimmer", true );
			Assert( m_pTurnInContainer );

			m_pTurnInButton = m_pTurnInContainer->FindControl< Button >( "TurnInButton", true );
			Assert( m_pTurnInButton );

			m_pTurnInSpinnerContainer = m_pTurnInContainer->FindControl< EditablePanel>( "TurnInSpinnerContainer", true );
			Assert( m_pTurnInSpinnerContainer );

			if ( m_pTurnInSpinnerContainer )
			{
				m_pTurningInLabel = m_pTurnInSpinnerContainer->FindControl< Label >( "TurningInLabel", true );
				Assert( m_pTurningInLabel );
			}
		}

		m_pAcceptedImage = m_pQuestPaperContainer->FindControl< ImagePanel >( "AcceptedImage", true );
		Assert( m_pAcceptedImage );
	}

	if ( m_pFrontFolderContainer )
	{
		CInputProxyPanel* pInputProxy = m_pFrontFolderContainer->FindControl< CInputProxyPanel >( "FrontInputProxyPanel", true );
		if ( pInputProxy )
		{
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_ENTER, m_pInactiveStatus, "CursorEntered" );
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_EXIT, m_pInactiveStatus, "CursorExited" );

			// Make the title glow
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_ENTER, m_pTitleButton, "CursorEntered" );
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_EXIT, m_pTitleButton, "CursorExited" );

			// Make the backdrop highlight
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_ENTER, this, "CollapsedGlowStart" );
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_EXIT, this, "CollapsedGlowEnd" );

			// Capture clicks to expand/contract
			pInputProxy->AddPanelForCommand( CInputProxyPanel::INPUT_MOUSE_RELEASED, this, "MouseReleased" );
		}
	}

	m_pExpirationLabel = FindControl<Label>( "QuestExpirationWarning", true );
	m_pFlavorText = FindControl<Label>( "QuestFlavorText", true );

	SetupObjectivesPanels( true );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_bHasAllControls =  m_pQuestPaperContainer
					  && m_pFrontFolderContainer
					  && m_pFrontFolderImage
					  && m_pBackFolderContainer
					  && m_pBackFolderImage
					  && m_pEncodedStatus
					  && m_pInactiveStatus
					  && m_pReadyToTurnInStatus
					  && m_pFlavorText
					  && m_pObjectiveExplanationLabel
					  && m_pExpirationLabel
					  && m_pTurnInContainer
					  && m_pTurnInDimmer
					  && m_pTurnInButton
					  && m_pIdentifyButton
					  && m_pTurnInSpinnerContainer
					  && m_pTitleButton
					  && m_pIdentifyDimmer
					  && m_pIdentifyContainer
					  && m_pPhotoStatic
					  && m_pAcceptedImage
					  && m_pTurningInLabel
					  && m_pFlavorScrollingContainer
					  && m_pItemTrackerPanel
					  && m_pEncodedImage
					  && m_pMainContainer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::ApplySettings( KeyValues *inResourceData )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::ApplySettings( inResourceData );

	if ( m_quest.Obj().has_quest_id() )
	{
		m_vecFoldersImages.Purge();
		KeyValues *pKVFoldersBlock = inResourceData->FindKey( "folders" );
		Assert( pKVFoldersBlock );
		if ( pKVFoldersBlock )
		{
			FOR_EACH_TRUE_SUBKEY( pKVFoldersBlock, pKVFolder )
			{
				auto& folder = m_vecFoldersImages[ m_vecFoldersImages.AddToTail() ];
				folder.m_strFront = pKVFolder->GetString( "front", NULL);
				folder.m_strBack = pKVFolder->GetString( "back", NULL );
			}
		}
		else
		{
			// Get our quest theme
			const CQuestThemeDefinition *pTheme = m_quest.GetDefinition()->GetQuestTheme();
			if ( pTheme )
			{
				Warning( "%s %s is missing 'folders' data\n", m_quest.GetDefinition()->GetCorrespondingOperationName(), pTheme->GetQuestItemResFile() );
			}
		}
	}

	if ( 1/*m_pKVItemTracker == NULL*/ )
	{
		KeyValues *pTrackerKV = inResourceData->FindKey( "tracker_kv" );
		
		if ( pTrackerKV )
		{
			m_pKVItemTracker = pTrackerKV->MakeCopy();
		}
	}

	m_strEncodedText		= inResourceData->GetString( "encoded_text", NULL );
	m_strExpireText			= inResourceData->GetString( "expire_text", NULL );
	m_strItemTrackerResFile = inResourceData->GetString( "TrackerPanelResFile", NULL );
	// Sound effects
	m_strTurnInSound		= inResourceData->GetString( "turn_in_sound", NULL );
	m_strTurnInSuccessSound = inResourceData->GetString( "turn_in_success_sound", NULL );
	m_strDecodeSound		= inResourceData->GetString( "decode_sound", NULL );
	m_strExpandSound		= inResourceData->GetString( "expand_sound", NULL );
	m_strCollapseSound		= inResourceData->GetString( "collapse_sound", NULL );

	// Animations
	m_strReset				= inResourceData->GetString( "anim_reset", NULL );
	m_strAnimExpand			= inResourceData->GetString( "anim_expand", NULL );
	m_strAnimCollapse		= inResourceData->GetString( "anim_collapse", NULL );
	m_strTurningIn			= inResourceData->GetString( "anim_turning_in", NULL );
	m_strHighlightOn		= inResourceData->GetString( "anim_highlight_on", NULL );
	m_strHighlightOff		= inResourceData->GetString( "anim_highlight_off", NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::PerformLayout( void )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::PerformLayout();

	if ( !HasAllControls() )
		return;

	m_pIdentifyContainer->SetVisible( m_eState == STATE_UNIDENTIFIED );
	m_pTurnInContainer->SetVisible( m_eState == STATE_COMPLETED || m_eState == STATE_TURNING_IN__GC_RESPONDED || m_eState == STATE_TURNING_IN__WAITING_FOR_GC);
	m_pTurnInButton->SetVisible( m_eState == STATE_COMPLETED );
	m_pTurnInSpinnerContainer->SetVisible( m_eState == STATE_TURNING_IN__GC_RESPONDED || m_eState == STATE_TURNING_IN__WAITING_FOR_GC );
	m_pPhotoStatic->SetVisible( m_eState == STATE_UNIDENTIFIED || m_eState == STATE_IDENTIFYING );
	m_pFindServerButton->SetVisible( m_eState == STATE_NORMAL );

	// only exist in non public build
	if ( m_pDiscardButton )
	{
		m_pDiscardButton->SetVisible( m_eState == STATE_NORMAL );
	}

	if ( m_pCompleteButton )
	{
		m_pCompleteButton->SetVisible( m_eState == STATE_NORMAL );
	}

	// loaners
	if ( m_eState == STATE_NORMAL || m_eState == STATE_COMPLETED )
	{	
		// get all loaners required from quest
		CUtlVector< item_definition_index_t > vecLoanerItems;
		bool bRequiredLoaners = GetLoanerListFromQuest( &m_quest, vecLoanerItems ) > 0;

		// get all granted loaners from this quest
		CUtlVector< CEconItemView* > vecGrantedLoaners;
		if ( bRequiredLoaners )
		{
			GetLoanersFromLocalInventory( m_quest.GetID(), vecLoanerItems, vecGrantedLoaners );
		}

		if ( bRequiredLoaners )
		{
			m_pLoanerContainerPanel->SetVisible( true );
			bool bAllGranted = vecLoanerItems.Count() == vecGrantedLoaners.Count();
			m_pRequestLoanerItemsButton->SetVisible( !bAllGranted );
			m_pEquipLoanerItemsButton->SetVisible( bAllGranted );

			for ( int i = 0; i < ARRAYSIZE( m_pLoanerItemModelPanel ); ++i )
			{
				// try to use the granted items first
				if ( i < vecGrantedLoaners.Count() )
				{
					m_pLoanerItemModelPanel[i]->SetItem( vecGrantedLoaners[i] );
					m_pLoanerItemModelPanel[i]->SetVisible( true );
				}
				// In case we don't get all the loaner items, use fake items as second option
				else if ( i < vecLoanerItems.Count() )
				{
					CEconItemView tempItem;
					tempItem.Init( vecLoanerItems[i], AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );

					m_pLoanerItemModelPanel[i]->SetItem( &tempItem );
					m_pLoanerItemModelPanel[i]->SetVisible( true );
				}
				else
				{
					m_pLoanerItemModelPanel[i]->SetVisible( false );
				}
			}
		}
		else
		{
			m_pLoanerContainerPanel->SetVisible( false );
		}
	}
	else
	{
		m_pLoanerContainerPanel->SetVisible( false );
	}



	m_pEncodedStatus->SetShow( m_eState == STATE_UNIDENTIFIED );
	m_pReadyToTurnInStatus->SetShow( m_eState == STATE_COMPLETED || m_eState == STATE_TURNING_IN__GC_RESPONDED || m_eState == STATE_TURNING_IN__WAITING_FOR_GC );

	float flDecodeAmount = 1.f;
	// Only cypher-style decoding needs to decode
	if ( m_eDecodeStyle == DECODE_STYLE_CYPHER && m_eState == STATE_UNIDENTIFIED )
	{
		flDecodeAmount = 0.f;
	}

	m_pEncodedImage->SetAlpha( m_eState == STATE_UNIDENTIFIED ? 255 : 0 );

	if ( m_quest.Obj().has_quest_id() )
	{
		m_pTitleButton->SetText( GetDecodedString( "name", flDecodeAmount ) );

		int nScrollableYOffset = 0;

		// TODO Brett: Lookup the operation?
		// Check if the quest is going to expire soon (within a week).  If so, show a "This is going to be destroyed" message.
		/*const CRTime nExpirationTime = m_hQuestItem->GetExpirationDate();
		const CRTime nOneWeekFromNow = CRTime::RTime32DateAdd( CRTime::RTime32TimeCur(), 1, k_ETimeUnitWeek );
		const bool bExpiringSoon = nExpirationTime.GetRTime32() != RTime32(0) && nExpirationTime < nOneWeekFromNow;
		m_pExpirationLabel->SetVisible( bExpiringSoon );
		if ( bExpiringSoon )
		{
			CLocalizedRTime32 locTime = { nExpirationTime.GetRTime32(), false, GLocalizationProvider(), NULL };
			m_pExpirationLabel->SetText( CConstructLocalizedString( g_pVGuiLocalize->Find( m_strExpireText ), locTime ) );
			m_pExpirationLabel->InvalidateLayout( true );
			m_pExpirationLabel->SizeToContents();
			nScrollableYOffset += m_pExpirationLabel->GetTall();
		}*/
		m_pExpirationLabel->SetVisible( false );

		m_pObjectiveExplanationLabel->SetPos( 0, nScrollableYOffset );
		m_pObjectiveExplanationLabel->SetText( GetDecodedString( "explanation", flDecodeAmount ) );
		m_pObjectiveExplanationLabel->InvalidateLayout( true ); // So we get the right height when we do SizeToContents below
		m_pObjectiveExplanationLabel->SizeToContents();
		nScrollableYOffset += m_pObjectiveExplanationLabel->GetTall();

		m_pFlavorText->SetText( GetDecodedString( "desc", flDecodeAmount ) );
		int nWide, nTall;
		m_pFlavorText->GetTextImage()->GetContentSize( nWide, nTall );
		m_pFlavorText->SetTall( nTall + 20 );

		m_pItemTrackerPanel->SetPos( m_pItemTrackerPanel->GetXPos(), nScrollableYOffset );
		nScrollableYOffset += m_pItemTrackerPanel->GetTall();

		// Put the flavor text below the obectives
		m_pFlavorText->SetPos( m_pFlavorText->GetXPos(), nScrollableYOffset );

		m_pFlavorScrollingContainer->InvalidateLayout( true );
		m_pFlavorScrollingContainer->InvalidateLayout();
		m_pFlavorScrollingContainer->ResetScrollAmount();


		// Randomize our folder images based on original ID
		if ( m_vecFoldersImages.Count() )
		{
			RandomSeed( m_quest.GetID() );
			int idx = RandomInt( 0, m_vecFoldersImages.Count() - 1 );

			m_pFrontFolderImage->SetImage( m_vecFoldersImages[ idx ].m_strFront );
			m_pBackFolderImage->SetImage( m_vecFoldersImages[ idx ].m_strBack );
		}
	}

	UpdateInvalidReasons();
	
	if ( m_pItemTrackerPanel )
	{
		auto& vecObjectives = m_pItemTrackerPanel->GetAttributePanels();
		FOR_EACH_VEC( vecObjectives, i )
		{
			// Only do this when unidentified.  The panel updates its own string, and we don't want to stomp it
			if ( m_eState == STATE_UNIDENTIFIED )
			{
				const wchar_t *pszString = GetDecodedString( CFmtStr( "objective%d", i ), flDecodeAmount );
				vecObjectives[ i ]->SetDialogVariable( "attr_desc", pszString );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestItemPanel::IsCursorOverMainContainer() const
{
	return m_pMainContainer ? m_pMainContainer->IsCursorOver() : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::SetupObjectivesPanels( bool bRecreate )
{
	if ( m_pItemTrackerPanel && bRecreate )
	{
		m_pItemTrackerPanel->MarkForDeletion();
		m_pItemTrackerPanel = NULL;
	}

	if ( !m_quest.Obj().has_quest_id() )
		return;

	if ( !m_pItemTrackerPanel )
	{
		m_pItemTrackerPanel = new CQuestProgressTrackerPanel( m_pFlavorScrollingContainer, "ItemTrackerPanel", &m_quest, m_strItemTrackerResFile );
		m_pItemTrackerPanel->SetAutoDelete( false );
		SETUP_PANEL( m_pItemTrackerPanel );
	}
	else
	{
		// Get all the panels created
		m_pItemTrackerPanel->SetItem( &m_quest );
		m_pItemTrackerPanel->InvalidateLayout( true );
	}

	m_pItemTrackerPanel->SetTall( m_pItemTrackerPanel->GetContentTall() );

	// Need to re-layout so the flavor text gets properly positioned under
	// all of the objectives
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::SetItem( CQuest* pItem )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if ( m_pLiveQuest == pItem )
	{
		return;
	}

	m_bCollapsed = true;
	m_quest.Obj().Clear();
	m_pLiveQuest = pItem;

	if ( pItem )
	{
		m_quest.Obj().CopyFrom( pItem->Obj() );
	}

	if ( m_pItemTrackerPanel && pItem )
	{
		m_pItemTrackerPanel->SetItem( m_pLiveQuest );
	}

	// By default
	SetState( STATE_NORMAL );

	if ( m_quest.Obj().has_quest_id() )
	{
		if ( m_quest.IsQuestReadyToTurnIn() )
		{
			SetState( STATE_COMPLETED );
		}

		// Snag the matchmaking map (if there is one)
		m_strMatchmakingGroupName = m_quest.GetDefinition()->GetMatchmakingGroupName();
		m_strMatchmakingCategoryName = m_quest.GetDefinition()->GetMatchmakingCategoryName();
		m_strMatchmakingMapName = m_quest.GetDefinition()->GetMatchmakingMapName();
	}

	// Reload res file so we get the right art
	LoadResFileForCurrentItem();

	// Capture strings after controls are created
	CaptureAndEncodeStrings();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Returns if the character is one that we don't want to re-encode
//			as another, or one that we don't want to encode another to in
//			order to maintain line breaks so that the decoding sequence is
//			easier for the user to follow.
//-----------------------------------------------------------------------------
bool IsNonEncodeCharacter( const wchar_t& wch)
{
	switch ( wch )
	{
		case L'\x000A':
		case L'\x000B':
		case L'\x000C':
		case L'\x000D':
		case L'\x0085':
		case L'\x2028':
		case L'\x2029':
		case L' ':
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::CaptureAndEncodeStrings()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !m_quest.Obj().has_quest_id() )
		return;

	// Clean up any existing values
	if ( m_pKVCipherStrings )
	{
		m_pKVCipherStrings->deleteThis();
		m_pKVCipherStrings = NULL;
	}

	m_pKVCipherStrings = new KeyValues( "cipherstrings" );
	KeyValues *pKVDecoded = m_pKVCipherStrings->CreateNewKey();
	pKVDecoded->SetName( "decoded" );

	{
		// Capture the description/flavor string
		const char *pszLocToken = m_quest.GetDefinition()->GetLocDescription();
		pKVDecoded->SetWString( "desc", g_pVGuiLocalize->Find( pszLocToken ) );
	}

	if ( m_pObjectiveExplanationLabel )
	{
		wchar_t wszBuff[512];	
		m_pObjectiveExplanationLabel->GetText( wszBuff, ARRAYSIZE( wszBuff ) );
		pKVDecoded->SetWString( "explanation", wszBuff );
	}

	auto& vecObjectives = m_pItemTrackerPanel->GetAttributePanels();
	// Capture objective strings
	FOR_EACH_VEC( vecObjectives, i )
	{
		CQuestObjectiveTextPanel *pObjective = vecObjectives[ i ];
		KeyValues *pKV = pObjective->GetDialogVariables();
		pKVDecoded->SetWString( CFmtStr( "objective%d", i ), pKV->GetWString( "attr_desc" ) );
	}

	// Create encoded strings from the decoded strings
	KeyValues *pKVEncoded = pKVDecoded->MakeCopy();
	pKVEncoded->SetName( "encoded" );
	
	m_pKVCipherStrings->AddSubKey( pKVEncoded );

	RandomSeed( m_quest.GetID() );

	// "encode" each string by scrambling
	FOR_EACH_VALUE( pKVEncoded, pKVString )
	{
		const wchar_t *pWString = pKVString->GetWString();
		wchar wszBuff[4096];
		loc_scpy_safe( wszBuff, pWString );
		int nStrLen = Q_wcslen( wszBuff );

		// Go through the entire string and swap each character
		// with another random character in the string
		int i=0;
		while( wszBuff[i] != 0 && i < ARRAYSIZE( wszBuff ) )
		{
			// Dont scramble spaces to maintain line breaks
			if ( !IsNonEncodeCharacter( wszBuff[i] )  )
			{
				// Scramble, but keep trying if we scramble to a space
				do 
				{
					wszBuff[i] = *(pWString + RandomInt( 0, nStrLen - 1 ) );
				} while ( IsNonEncodeCharacter( wszBuff[i] ) );
				
			}

			i++;
		}

		pKVEncoded->SetWString( pKVString->GetName(), wszBuff );
	}

	const char *pszLocToken = m_quest.GetDefinition()->GetLocName();
	const wchar_t* pwszName = g_pVGuiLocalize->Find( pszLocToken );
	// Force the encrypted version of the quest title to be "<Encrypted>".
	pKVEncoded->SetWString( "name", g_pVGuiLocalize->Find( m_strEncodedText ) );
	pKVDecoded->SetWString( "name", pwszName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnCommand( const char *command )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::OnCommand( command );

	if ( FStrEq( command, "discard_quest" ) )
	{
		OnDiscardQuest();
	}
	else if ( FStrEq( command, "complete_quest" ) )
	{
		GCSDK::CProtoBufMsg< CMsgGCQuestComplete_Debug > msg( k_EMsgGCQuestComplete_Debug );
		msg.Body().set_quest_id( m_pLiveQuest->GetID() );
		GCClientSystem()->BSendMessage( msg );
	}
	else if ( FStrEq( command, "select" ) )
	{
		m_pQuestList->SetSelected( this, false );
	}
	else if ( FStrEq( command, "turnin" ) ) 
	{
		if ( m_pLiveQuest )
		{
		/*	if ( !tf_quest_turn_in_confirm_opt_out.GetBool() && ( m_pLiveQuest->GetEarnedBonusPoints() != m_pLiveQuest->GetDefinition()->GetMaxBonusPoints() ) )
			{
				CTFGenericConfirmOptOutDialog *pPanel = ShowConfirmOptOutDialog( "#TF_Quest_TurnIn_Title", "#TF_Quest_TurnIn_Text",
																				 "#TF_Quest_TurnIn_Yes", "#TF_Quest_TurnIn_No",
																				 "#TF_Quest_TurnIn_Ask_Opt_Out", "tf_quest_turn_in_confirm_opt_out",
																				 ConfirmTurnInQuest );
				if ( pPanel )
				{
					pPanel->SetContext( this );
					return;
				}
			}
			else*/
			{
				OnCompleteQuest();
			}
		}
	}
	else if ( FStrEq( command, "request_loaner_items" ) )
	{
		if ( m_pLiveQuest )
		{
			GCSDK::CProtoBufMsg< CMsgGCQuestObjective_RequestLoanerItems > msg( k_EMsgGCQuestObjective_RequestLoanerItems );
			msg.Body().set_quest_id( m_pLiveQuest->GetID() );
			GCClientSystem()->BSendMessage( msg );
		}
	}
	else if ( FStrEq( command, "equip_loaner_items" ) )
	{
		OnEquipLoaners();
	}
	else if( Q_strnicmp( "playsound", command, 9 ) == 0 )
	{
		vgui::surface()->PlaySound( command + 10 );
	}
	else if ( FStrEq( "mm_casual_open", command ) )
	{
		if ( GTFGCClientSystem() )
		{
			if ( ( m_strMatchmakingGroupName != 0 ) || ( m_strMatchmakingCategoryName != 0 ) || ( m_strMatchmakingMapName != 0 ) )
			{
				GTFGCClientSystem()->ClearCasualSearchCriteria();

				if ( m_strMatchmakingGroupName != 0 )
				{
					int iGroupType = StringFieldToInt( m_strMatchmakingGroupName.Get(), s_pszMMTypes, ARRAYSIZE( s_pszMMTypes ) );
					if ( iGroupType > -1 )
					{
						SelectGroup( (EMatchmakingGroupType)iGroupType, true );
					}
				}

				if ( m_strMatchmakingCategoryName != 0 )
				{
					int iCategoryType = StringFieldToInt( m_strMatchmakingCategoryName.Get(), s_pszGameModes, ARRAYSIZE( s_pszGameModes ) );
					if ( iCategoryType > -1 )
					{
						SelectCategory( (EGameCategory)iCategoryType, true );
					}
				}

				if ( m_strMatchmakingMapName != 0 )
				{
					if ( GetItemSchema() )
					{
						const MapDef_t *pMap = GetItemSchema()->GetMasterMapDefByName( m_strMatchmakingMapName.Get() );
						if ( pMap )
						{
							GTFGCClientSystem()->SelectCasualMap( pMap->m_nDefIndex, true );
						}
					}
				}
			}
		}

		// Defaulting to 12v12
		GTFGCClientSystem()->SetLadderType( k_eTFMatchGroup_Casual_12v12 );
		PromptOrFireCommand( "OpenMatchmakingLobby casual" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const wchar_t* CQuestItemPanel::GetDecodedString( const char* pszKeyName, float flPercentDecoded )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	static wchar_t wszBuff[4096];
	KeyValues *pKVEncoded = m_pKVCipherStrings->FindKey( "encoded" );
	KeyValues *pKVDecoded = m_pKVCipherStrings->FindKey( "decoded" );

	// Trivial work?
	if ( flPercentDecoded <= 0.f )
	{
		return pKVEncoded->GetWString( pszKeyName );
	}
	else if ( flPercentDecoded >= 1.f )
	{
		return pKVDecoded->GetWString( pszKeyName );
	}

	loc_scpy_safe( wszBuff, pKVEncoded->GetWString( pszKeyName ) );
	const locchar_t* pwszDecoded = pKVDecoded->GetWString( pszKeyName );
	int nLength = loc_strlen( pwszDecoded );
	int nMaxCopy = nLength * flPercentDecoded;
	// Not using V_wcsncpy because it null terminates.
	wcsncpy( wszBuff, pwszDecoded, nMaxCopy );

	return wszBuff;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnThink()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	switch ( m_eState )
	{
	case STATE_IDENTIFYING:
	{
		// Have we finished?
		if ( m_StateTimer.HasStarted() && m_StateTimer.IsElapsed() )
		{
			m_pItemTrackerPanel->InvalidateLayout();
			m_StateTimer.Invalidate();
			SetState( STATE_NORMAL );

			// Play a reveal sound?
			const CQuestThemeDefinition *pTheme = m_pLiveQuest->GetDefinition()->GetQuestTheme();
			if ( pTheme )
			{
				const char *pszRevealSound = pTheme->GetRevealSound();
				if ( pszRevealSound && pszRevealSound[0] )
				{
					vgui::surface()->PlaySound( pszRevealSound );
				}
			}
		}

		float flPercent = m_StateTimer.GetElapsedTime() / m_StateTimer.GetCountdownDuration();

		switch( m_eDecodeStyle )
		{
		case DECODE_STYLE_CYPHER:
		{
			// Slowly "decode" the text in the lables
			m_pTitleButton->SetText( GetDecodedString( "name", flPercent ) );
			m_pFlavorText->SetText( GetDecodedString( "desc", flPercent ) );
			m_pObjectiveExplanationLabel->SetText( GetDecodedString( "explanation", flPercent ) );

			auto& vecObjectives = m_pItemTrackerPanel->GetAttributePanels();
			FOR_EACH_VEC( vecObjectives, i )
			{
				const wchar_t *pszString = GetDecodedString( CFmtStr( "objective%d", i ), flPercent );
				vecObjectives[ i ]->SetDialogVariable( "attr_desc", pszString );
			}
			break;
		}
		case DECODE_STYLE_PANEL_FADE:
		{
			// Slowly fade out the encode image
			m_pEncodedImage->SetAlpha( 255 * ( 1.f - flPercent ) );
			break;
		}
		default:
			Assert( 0 );
		}

		break;
	}
	case STATE_TURNING_IN__WAITING_FOR_GC:
	{
		// Have we finished?
		if ( m_StateTimer.HasStarted() && m_StateTimer.IsElapsed() )
		{
			m_pQuestList->SetCompletingPanel( NULL );
			m_StateTimer.Invalidate();

			// Bring up confirm dialog
			CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#TF_Trading_Timeout_Title", "#TF_Trading_Timeout_Text", "#TF_OK", NULL, NULL, NULL );
		
			if ( pDialog )
			{
				pDialog->SetContext( this );
				pDialog->Show();
			}

			SetState( STATE_COMPLETED );
		}
		
		// Intentionally fall through
	}
	case STATE_TURNING_IN__GC_RESPONDED:
	{
		// Have we finished?
		if ( m_StateTimer.HasStarted() && m_StateTimer.IsElapsed() )
		{
			SetState( STATE_SHOW_ACCEPTED );
			m_StateTimer.Start( 3.f );
			m_pAcceptedImage->SetVisible( true );

			vgui::surface()->PlaySound( m_strTurnInSuccessSound );
		}

		if ( m_pTurningInLabel )
		{
			int nPeriods = m_StateTimer.GetElapsedTime() / 0.3f;
			nPeriods %= 4; // Only do up to 3 periods

			wchar_t wszTurningInText[64];
			char szTurningInLocToken[128];
			m_pTurningInLabel->GetTextImage()->GetUnlocalizedText( szTurningInLocToken, ARRAYSIZE( szTurningInLocToken ) );
			V_snwprintf( wszTurningInText, ARRAYSIZE( wszTurningInText ), L"%ls", g_pVGuiLocalize->Find( szTurningInLocToken ) );

			while ( nPeriods > 0 )
			{
				V_wcsncat( wszTurningInText, L".", ARRAYSIZE( wszTurningInText ) );
				--nPeriods;
			}

			m_pTurningInLabel->SetText( wszTurningInText );
		}

		break;
	}
	case STATE_SHOW_ACCEPTED:
	{
		if ( m_StateTimer.HasStarted() && m_StateTimer.IsElapsed() )
		{
			m_pQuestList->SetCompletingPanel( NULL );
			m_StateTimer.Invalidate();
	
			engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

			InventoryManager()->ShowItemsPickedUp( true, false );
			GetQuestLog()->AttachToGameUI();
			GetQuestLog()->MarkQuestsDirty();
			m_pQuestList->PopulateQuestLists();

			engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );

			if ( m_pszCompleteSound )
			{
				vgui::surface()->PlaySound( m_pszCompleteSound );
			}
		}
		else
		{
			float flPercent = Clamp( m_StateTimer.GetElapsedTime() / 0.2f, 0.f, 1.f );
			m_nPaperXShakePos = sin( m_StateTimer.GetElapsedTime() * 200.f ) * ( 1.f - flPercent ) * 8.f;
			m_nPaperYShakePos = sin( m_StateTimer.GetElapsedTime() * 200.f ) * ( 1.f - flPercent ) * 8.f;
			m_pQuestPaperContainer->SetPos( m_nPaperXPos + m_nPaperXShakePos, m_nPaperYPos + m_nPaperYShakePos );
		}

		break;
	}
	case STATE_UNIDENTIFIED:
	{
		// Do nothing
		break;
	}
	case STATE_COMPLETED:
	{
		// Do nothing
		break;
	}
	default:
		// Do nothing
		break;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::FireGameEvent( IGameEvent *event )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if( FStrEq( event->GetName(), "quest_objective_completed" ) )
	{
		itemid_t nIDLow = 0x00000000FFFFFFFF & (itemid_t)event->GetInt( "quest_item_id_low" );
		itemid_t nIDHi =  0xFFFFFFFF00000000 & (itemid_t)event->GetInt( "quest_item_id_hi" ) << 32;
		itemid_t nID = nIDLow | nIDHi;
		if ( m_pLiveQuest && nID == m_pLiveQuest->GetID() )
		{
			SetupObjectivesPanels( false );

			if ( m_pLiveQuest->IsQuestReadyToTurnIn() )
			{
				SetState( STATE_COMPLETED );
			}

			PerformLayout();
		}
	}
	else if ( FStrEq( event->GetName(), "player_spawn" ) 
		   || FStrEq( event->GetName(), "client_disconnect" ) )
	{
		InvalidateLayout();
	}
}

void CQuestItemPanel::OnMouseReleased( MouseCode code )
{
	OnCommand( "select" );
}

//-----------------------------------------------------------------------------
// Purpose: Update our invalid reasons
//-----------------------------------------------------------------------------
void CQuestItemPanel::UpdateInvalidReasons()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	InvalidReasonsContainer_t invalidReasons;
	bool bAllAreInvalid = false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && m_pLiveQuest )
	{
		// Get the tracker for the items
		const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( m_pLiveQuest->GetID() );
		// Get invalid reasons
		if ( pItemTracker )
		{
			int nNumInvalid = pItemTracker->GetNumInactiveObjectives( pLocalPlayer, invalidReasons );
			bAllAreInvalid = pItemTracker->GetObjectiveTrackers().Count() == nNumInvalid;
		}

		// Build a string describing why the current quest can't be worked on
		if ( !invalidReasons.IsValid() )
		{
			CUtlVector< CUtlString > vecStrings;
			// Get the strings that explain each reasons
			GetInvalidReasonsNames( invalidReasons, vecStrings );

			wchar_t wszBuff[ 1024 ];

			// Start with the explanation
			V_swprintf_safe( wszBuff, L"%ls", g_pVGuiLocalize->Find( "#TF_QuestInvalid_Explanation" ) );

			// Add in each reason why the quest is invalid
			for( int i = 0; i < vecStrings.Count(); ++ i )
			{
				V_wcscat_safe( wszBuff, L"\n\n" );
				V_wcscat_safe( wszBuff, g_pVGuiLocalize->Find( vecStrings[i] ) );
			}

			// This gets snagged by CQuestTooltip
			m_pInactiveStatus->SetDialogVariable( "tiptext", wszBuff );
		}
	}
		
	// Visible if there's a reason why we're invalid
	bool bShow = bAllAreInvalid && m_eState == STATE_NORMAL;
	m_pInactiveStatus->SetShow( bShow );
	m_pInactiveStatus->SetMouseInputEnabled( bShow );
	m_pInactiveStatus->SetTooltip( g_spTextTooltip, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Start a glow
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnCollapsedGlowStart( void )
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strHighlightOn );	
}

//-----------------------------------------------------------------------------
// Purpose: Stop the glow
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnCollapsedGlowEnd( void )
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strHighlightOff );	
}

//-----------------------------------------------------------------------------
// Purpose: Delete the quest. 
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnDiscardQuest( void )
{
#if !defined(STAGING_ONLY) && !defined(DEBUG)
	// Not in public!
	return;
#endif

	if ( m_pQuestList->GetCompletingPanel() == NULL )
	{
		// Bring up confirm dialog
		CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#QuestConfirmDiscard_Title", "#QuestConfirmDiscard_Body", "#X_DiscardItem", "#Cancel", &ConfirmDiscardQuest, NULL );
		if ( pDialog )
		{
			pDialog->SetContext( this );
			pDialog->Show();
		}

		// Get our quest theme
		const CQuestThemeDefinition *pTheme = m_pLiveQuest->GetDefinition()->GetQuestTheme();
		if ( pTheme )
		{
			const char *pszDiscardSound = pTheme->GetDiscardSound();
			if ( pszDiscardSound && pszDiscardSound[0] )
			{
				vgui::surface()->PlaySound( pszDiscardSound );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Equip loaners for local player
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnEquipLoaners( void )
{
	if ( !m_pLiveQuest )
		return;

	if ( m_pQuestList->GetCompletingPanel() == NULL )
	{
		// Bring up confirm dialog
		CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#QuestConfirmEquipLoaners_Title", "#QuestConfirmEquipLoaners_Body", "#Equip", "#Cancel", &ConfirmEquipLoaners, NULL );
		if ( pDialog )
		{
			pDialog->SetContext( this );
			pDialog->Show();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to the GC to evaluate completion of this quest
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnCompleteQuest( void )
{
	if ( !m_pLiveQuest )
		return;

	// Double check that they're not just forcing the command
	if ( m_pLiveQuest->IsQuestReadyToTurnIn() && m_pQuestList->GetCompletingPanel() == NULL )
	{
		m_pQuestList->SetCompletingPanel( this );

		SetState( STATE_TURNING_IN__WAITING_FOR_GC );

		// Use the timer for turning in the quest
		m_StateTimer.Start( k_flQuestTurnInTime );
		vgui::surface()->PlaySound( m_strTurnInSound );

		GCSDK::CProtoBufMsg< CMsgGCQuestComplete_Request > msg( k_EMsgGCQuestComplete_Request );
	
		msg.Body().set_quest_id( m_pLiveQuest->GetID() );

		GCClientSystem()->BSendMessage( msg );

		PostActionSignal( new KeyValues("CompleteQuest") );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strTurningIn );	
		
		// Get our quest theme
		const CQuestThemeDefinition *pTheme = m_pLiveQuest->GetDefinition()->GetQuestTheme();
		if ( pTheme )
		{
			m_pszCompleteSound = pTheme->GetRewardSound();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnConfirmDelete( bool bConfirm )
{
	// Delete the quest
	if ( bConfirm && m_pLiveQuest )
	{
		GCSDK::CProtoBufMsg< CMsgGCQuestDiscard_Request > msg( k_EMsgGCQuestDiscard_Request );
	
		msg.Body().set_quest_id( m_pLiveQuest->GetID() );

		GCClientSystem()->BSendMessage( msg );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::OnConfirmEquipLoaners( bool bConfirm )
{
	// equip loaners
	if ( bConfirm && m_pLiveQuest )
	{
		// get all loaners required from quest
		CUtlVector< item_definition_index_t > vecLoanerItems;
		bool bRequiredLoaners = GetLoanerListFromQuest( m_pLiveQuest, vecLoanerItems );

		// get all granted loaners from this quest
		CUtlVector< CEconItemView* > vecGrantedLoaners;
		if ( bRequiredLoaners )
		{
			GetLoanersFromLocalInventory( m_pLiveQuest->GetID(), vecLoanerItems, vecGrantedLoaners );
		}

		for ( int i=0; i<vecGrantedLoaners.Count(); ++i )
		{
			CEconItemView *pItem = vecGrantedLoaners[i];
			if ( pItem )
			{
				// do it for first class that can equip
				for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; ++iClass )
				{
					if ( pItem->GetStaticData()->CanBeUsedByClass( iClass ) )
					{
						int iSlot = pItem->GetStaticData()->GetLoadoutSlot( iClass );
						TFInventoryManager()->EquipItemInLoadout( iClass, iSlot, pItem->GetItemID() );
						
						// take the player to character loadout page
						engine->ClientCmd_Unrestricted( CFmtStr( "open_charinfo_direct %d", iClass ) );
						break;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::QuestCompletedResponse()
{
	// If we werent the one listening, dont bother
	if ( m_eState != STATE_TURNING_IN__WAITING_FOR_GC )
		return;

	m_pQuestPaperContainer->GetPos( m_nPaperXPos, m_nPaperYPos );
	m_nPaperXShakePos = m_nPaperYShakePos = 0;

	SetState( STATE_TURNING_IN__GC_RESPONDED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemPanel::SetSelected( bool bSelected, bool bImmediate )
{
	bool bPrevCollapsedSide = m_bCollapsed;
	m_bCollapsed = ( !m_bCollapsed && bSelected ) || ( !bSelected );

	if ( !bImmediate && bPrevCollapsedSide != m_bCollapsed )
	{
		if ( m_bCollapsed )
		{
			vgui::surface()->PlaySound( m_strCollapseSound );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strAnimCollapse );	
		}
		else
		{
			vgui::surface()->PlaySound( m_strExpandSound );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strAnimExpand );	
		}
	}
	else 
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, m_strReset );
	}
}

void CQuestItemPanel::SetState( EItemPanelState_t eState )
{
	m_eState = eState;
	InvalidateLayout();
}



//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to handle a loaner item response
//-----------------------------------------------------------------------------
class CGCLoanerRequestResponse : public GCSDK::CGCClientJob
{
public:
	CGCLoanerRequestResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCQuestObjective_RequestLoanerResponse> msg( pNetPacket );

		// Show them the items they just got loaned!
		InventoryManager()->ShowItemsPickedUp( true, false );

		return true;
	}
};

GC_REG_JOB( GCSDK::CGCClient, CGCLoanerRequestResponse, "CGCLoanerRequestResponse", k_EMsgGCQuestObjective_RequestLoanerResponse, GCSDK::k_EServerTypeGCClient );

#endif // 0
