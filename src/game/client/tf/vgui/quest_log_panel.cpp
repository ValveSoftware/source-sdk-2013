//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "quest_log_panel.h"
#include "ienginevgui.h"
#include "c_tf_gamestats.h"
#include "store/store_panel.h"
#include "econ/econ_ui.h"
#include "clientmode_tf.h"
#include "tf_hud_mainmenuoverride.h"
#include "vgui_int.h"
#include "IGameUIFuncs.h" // for key bindings
#include <vgui_controls/AnimationController.h>
#include "tf_item_inventory.h"
#include "vgui/IInput.h"
#include "item_ad_panel.h"
#include "vgui_controls/ProgressBar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#if 0
															 
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

static CItemModelPanelToolTip* g_spItemTooltip = NULL;
CQuestTooltip* g_spTextTooltip = NULL;

CQuestLogPanel *GetQuestLog()
{
	CQuestLogPanel *pQuestLogPanel = (CQuestLogPanel*)gViewPortInterface->FindPanelByName( PANEL_QUEST_LOG );
	return pQuestLogPanel;
}

//-------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CScrollableQuestList::CScrollableQuestList( vgui::Panel *parent, const char *pszPanelName ) 
	: EditablePanel( parent, pszPanelName )
	, m_pCompletingPanel( NULL )
	, m_bQuestsLayoutDirty( false )
	, m_pszNoQuests( NULL )
	, m_pszNeedAPass( NULL )
	, m_pszNotPossible( NULL )
{
	m_pContainer = new EditablePanel( this, "Container" );
	m_vecQuestItemPanels.SetSize( 2 );

	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[ i ] = new CQuestItemPanel( m_pContainer, "QuestItemPanel", NULL, this );
	}
}

CScrollableQuestList::~CScrollableQuestList()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::ApplySchemeSettings( pScheme );

	const char *pszResFile = "Resource/UI/econ/ScrollableQuestList.res";

	// Check if the operation wants to override our default res file
	const auto& mapOperations = GetItemSchema()->GetOperationDefinitions();
	FOR_EACH_MAP_FAST( mapOperations, i )
	{
		CEconOperationDefinition* pCurrentOperation = mapOperations[i];
		// Take the first active operation's res file that's different than default
		if ( pCurrentOperation->IsActive() && pCurrentOperation->GetQuestListOverrideResFile() )
		{
			// Use the first found for now
			pszResFile = pCurrentOperation->GetQuestListOverrideResFile();
			break;
		}
	}

	LoadControlSettings( pszResFile );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::ApplySettings( KeyValues *inResourceData )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::ApplySettings( inResourceData );

	m_pszNoQuests		= inResourceData->GetString( "no_quests", "#QuestLog_NoQuests" );
	m_pszNeedAPass		= inResourceData->GetString( "need_a_pass", "#QuestLog_NeedPassForContracts" );
	m_pszNotPossible	= inResourceData->GetString( "not_possible", "#QuestLog_NoContractsPossible" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::PerformLayout( void ) 
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::PerformLayout();	

	m_pContainer->InvalidateLayout( true );

	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[ i ]->InvalidateLayout( true, true );
		m_vecQuestItemPanels[ i ]->SetZPos( 1 + i );
	}

	PositionQuestItemPanels();

	UpdateEmptyMessage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::OnThink()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if ( m_bQuestsLayoutDirty )
	{
		m_bQuestsLayoutDirty = false;

		PositionQuestItemPanels();
	}

	// Conditionally turn mouse input on/off based on where the mouse is
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[i]->SetMouseInputEnabled( m_vecQuestItemPanels[i]->IsCursorOverMainContainer() );
	}
}

void CScrollableQuestList::OnCommand( const char *command )
{
	if ( FStrEq( command, "deselect_all" ) )
	{
		SetSelected( NULL, false );
	}

	BaseClass::OnCommand( command );
}

int QuestSort_AcquiredTime( CQuestItemPanel* const* p1, CQuestItemPanel* const* p2 )
{
	if ( !(*p1)->GetItem() )
		return -1;

	if ( !(*p2)->GetItem() )
		return 1;

	// Newest items first
	return (*p1)->GetItem()->GetID() - (*p2)->GetItem()->GetID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::PositionQuestItemPanels()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	// We dont do anything when a quest is completing
	if ( m_pCompletingPanel != NULL )
		return;

	int nVisible = 0;
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		CQuestItemPanel* pPanel = m_vecQuestItemPanels[i];
		if ( pPanel )
		{
			pPanel->SetVisible( pPanel->GetItem() );

			if ( pPanel->GetItem() )
			{
				++nVisible;
			}
		}
	}

	CExLabel *pLabel = FindControl<CExLabel>( "EmptyLabel", true );
	if ( pLabel )
	{
		pLabel->SetVisible( nVisible == 0 );
	}

	// Check for a selected panel
	const CQuestItemPanel* pSelected = NULL;
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		if ( m_vecQuestItemPanels[i]->IsSelected() )
		{
			Assert( pSelected == NULL );
			pSelected = m_vecQuestItemPanels[i];
		}
	}

	struct FolderCommands_t
	{
		const char* m_pszSelected;
		const char* m_pszOtherIsSelected;
		const char* m_pszNoneSelected;
	};

	const FolderCommands_t folderCommands[] = { { "QuestItem_Back_Selected", "QuestItem_Back_OtherSelected", "QuestItem_Back_NoneSelected" }
											  , { "QuestItem_Front_Selected", "QuestItem_Front_OtherSelected", "QuestItem_Front_NoneSelected" } };

	// Update the positions
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		// This is the selected panel
		if ( pSelected == m_vecQuestItemPanels[ i ] )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_vecQuestItemPanels[ i ], folderCommands[i].m_pszSelected );	
		}
		else if ( pSelected )	// Some other panel is selected
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_vecQuestItemPanels[ i ], folderCommands[i].m_pszOtherIsSelected );	
		}
		else // No panel is selected
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_vecQuestItemPanels[ i ], folderCommands[i].m_pszNoneSelected );	
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::SetSelected( CQuestItemPanel *pItem, bool bImmediately )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[ i ]->SetSelected( m_vecQuestItemPanels[ i ] == pItem, bImmediately );
	}

	PositionQuestItemPanels();
}


//-----------------------------------------------------------------------------
// Purpose: Update what message we show when we have no quests
//-----------------------------------------------------------------------------
void CScrollableQuestList::UpdateEmptyMessage()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	CCyclingAdContainerPanel* pPassStoreAd = FindControl< CCyclingAdContainerPanel >( "ItemAd", true );
	if ( !pPassStoreAd )
		return;

	pPassStoreAd->SetVisible( false );
	SetDialogVariable( "noquests", "" );

	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		// Case 1 above
		if ( m_vecQuestItemPanels[ i ]->GetItem() )
			return;
	}

	// By default, there's no operations going on, and there's no ad to show
	const char *pszNoQuestsText = m_pszNotPossible;

	// Find any active quest-dropping operations
	const auto& mapOperations = GetItemSchema()->GetOperationDefinitions();
	FOR_EACH_MAP_FAST( mapOperations, iOperation )
	{
		CEconOperationDefinition *pOperation = mapOperations[ iOperation ];
		const CSchemaLootListDefHandle pOperationLootlist( pOperation->GetOperationLootlist() );
		// Must still be dropping, and be dropping quests
		if ( CRTime::RTime32TimeCur() < pOperation->GetStopGivingToPlayerDate() && pOperationLootlist )
		{
			// If there's a required item and a gateway item
			if ( pOperation->GetRequiredItemDefIndex() != INVALID_ITEM_DEF_INDEX && pOperation->GetGatewayItemDefIndex() != INVALID_ITEM_DEF_INDEX )
			{
				// And the user doesn't have the required item
				if ( TFInventoryManager()->GetLocalTFInventory()->FindFirstItembyItemDef( pOperation->GetRequiredItemDefIndex() ) == NULL )
				{
					// The user needs to get the item
					pszNoQuestsText = m_pszNeedAPass;

					bool bStoreIsReady = EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetPriceSheet() && EconUI()->GetStorePanel()->GetCart() && steamapicontext && steamapicontext->SteamUser();
					bool bGatewayItemInStore = false;
					// Check if the gateway item is in the Mann Co Store
					if ( bStoreIsReady )
					{
						bGatewayItemInStore = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( pOperation->GetGatewayItemDefIndex() ) != NULL;
					}

					CEconItemDefinition* pGatewayItemDef = GetItemSchema()->GetItemDefinition( pOperation->GetGatewayItemDefIndex() );
					Assert( pGatewayItemDef );
					if ( !pGatewayItemDef )
						return;

					// Cook up KVs for this item ad
					KeyValuesAD pKVItemAd( "items" ); // The panel will copy these
					KeyValues* pKVItem = pKVItemAd->CreateNewKey();
					pKVItem->SetName( "0" );
					pKVItem->SetString( "item", pGatewayItemDef->GetDefinitionName() );
					pKVItem->SetInt( "show_market", bGatewayItemInStore ? 0 : 1 );

					pPassStoreAd->SetVisible( true );
					pPassStoreAd->SetItemKVs( pKVItemAd );

					// This is the most important thing to communicate.  Don't let other operations stomp it.
					break;
				}
			}
	
			pszNoQuestsText = m_pszNoQuests;
			// Don't break.  Give more important operations a chance to present
		}
	}

	SetDialogVariable( "noquests", g_pVGuiLocalize->Find( pszNoQuestsText ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::PopulateQuestLists()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	// We dont do anything when a quest is completing
	if ( m_pCompletingPanel != NULL )
		return;

	DirtyQuestLayout();

	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		return;
	}
	
	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamapicontext->SteamUser()->GetSteamID() );
	
	if ( !pSOCache )
		return;

	auto pQuestCache = pSOCache->FindTypeCache( CQuest::k_nTypeID );
	if ( !pQuestCache )
		return;

	CUtlVector< CQuest* > vecUntrackedQuests;
	for( uint32 i=0; i < pQuestCache->GetCount(); ++i )
	{
		vecUntrackedQuests.AddToTail( (CQuest*) pQuestCache->GetObject( i ) );
	}

	CUtlVector< CQuestItemPanel* > vecAvailablePanels;

	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		bool bFound = false;
		if ( m_vecQuestItemPanels[ i ]->GetItem() )
		{
			FOR_EACH_VEC_BACK( vecUntrackedQuests, j )
			{
				CQuest* pQuest = vecUntrackedQuests[ j ];
				// See if the item in the panel is still in the list of items we own
				if ( m_vecQuestItemPanels[ i ]->GetItem()->GetID() == pQuest->GetID() )
				{
					// Refresh it
					m_vecQuestItemPanels[ i ]->InvalidateLayout();
					vecUntrackedQuests.Remove( j );
					bFound = true;
					break;
				}
			}
		}

		// Didn't find it.  Clear it out
		if ( !bFound )
		{
			if ( m_vecQuestItemPanels[ i ]->GetItem() )
			{
				m_vecQuestItemPanels[ i ]->SetItem( NULL );
			}

			if ( m_vecQuestItemPanels[ i ]->IsSelected() )
			{
				SetSelected( m_vecQuestItemPanels[ i ], false );
			}

			vecAvailablePanels.AddToTail( m_vecQuestItemPanels[ i ] );
		}
	}

	for ( int i = 0 ; i < vecUntrackedQuests.Count(); ++i )
	{
		CQuest *pItem = vecUntrackedQuests[i];

		if ( i < vecAvailablePanels.Count() )
		{
			vecAvailablePanels[ i ]->SetItem( pItem );
		}
		else if ( i >= m_vecQuestItemPanels.Count() )
		{
			Assert( !"Ran out of quest panels!" );
		}
	}

	// Sort the panels to make sure they're in order
	m_vecQuestItemPanels.Sort( &QuestSort_AcquiredTime );

	// Sorting is done manually
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[ i ]->SetZPos( i + 1 );
	}

	PositionQuestItemPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrollableQuestList::QuestCompletedResponse()
{
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		m_vecQuestItemPanels[i]->QuestCompletedResponse();
	}

//	PopulateQuestLists();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if any quest item panels are in the passed in state
//-----------------------------------------------------------------------------
bool CScrollableQuestList::AnyQuestItemPanelsInState( CQuestItemPanel::EItemPanelState_t eState ) const
{
	FOR_EACH_VEC( m_vecQuestItemPanels, i )
	{
		if ( m_vecQuestItemPanels[i]->GetState() == eState )
			return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestLogPanel::CQuestLogPanel( IViewPort *pViewPort )
		: EditablePanel( NULL, PANEL_QUEST_LOG )
		, m_bWaitingForComplete( false )
		, m_pQuestList( NULL )
		, m_bInventoryDirty( true )
		, m_iQuestLogKey( BUTTON_CODE_INVALID )
{
	if (g_pVGuiLocalize)
	{
		g_pVGuiLocalize->AddFile( "resource/tf_quests_%language%.txt" );
	}

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "inventory_updated" );
	ListenForGameEvent( "gameui_hidden" );
	ListenForGameEvent( "econ_inventory_connected" );

	// Create the item model panel tooltip
	m_pMouseOverItemPanel = new CItemModelPanel( this, "mouseoveritempanel" );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );

	// Create the text tooltip
	m_pToolTip = new CQuestTooltip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
//	m_pToolTipEmbeddedPanel->MakePopup();
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	EditablePanel *pMainContainer = new EditablePanel( this, "MainContainer" );
	m_pQuestList = new CScrollableQuestList( pMainContainer, "QuestList" );

	m_pProgressPanel = new EditablePanel( this, "ProgressPanel" );

	m_pDebugButton = new CExButton( pMainContainer, "Options", "Options", this, "open_debug_menu" );
}

//-----------------------------------------------------------------------------
// Purpose: Look into the moused-over panel and take "tiptext" from its dialog
//			variables and set it as our own.
//-----------------------------------------------------------------------------
void CQuestTooltip::ShowTooltip( Panel *pCurrentPanel )
{
	EditablePanel* pEditableCurrentPanel = dynamic_cast< EditablePanel* >( pCurrentPanel );
	if ( pEditableCurrentPanel )
	{
		KeyValues* pKVVariables = pEditableCurrentPanel->GetDialogVariables();
		const wchar_t *pwszTipText = pKVVariables->GetWString( "tiptext", L"" );
		m_pEmbeddedPanel->SetDialogVariable( "tiptext", pwszTipText );
	}

	BaseClass::ShowTooltip( pCurrentPanel );
}

//-----------------------------------------------------------------------------
// Purpose: Position ourselves down and to the right as far as posible
//-----------------------------------------------------------------------------
void CQuestTooltip::PositionWindow( Panel *pTipPanel )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	int iTipW, iTipH;
	pTipPanel->GetSize( iTipW, iTipH );

	int cursorX, cursorY;
	vgui::input()->GetCursorPos(cursorX, cursorY);

	int px, py, wide, tall;
	ipanel()->GetAbsPos( m_pEmbeddedPanel->GetParent()->GetVPanel(), px, py );
	m_pEmbeddedPanel->GetParent()->GetSize(wide, tall);

	if ( !m_pEmbeddedPanel->IsPopup() )
	{
		// Move the cursor into our parent space
		cursorX -= px;
		cursorY -= py;
	}

	// Dangle as far down and as far right as possible
	int nXPos = cursorX - Max( 0, ( ( iTipW + cursorX ) - wide ) );
	int nYPos = ( cursorY + 20 )- Max( 0, ( ( iTipH + cursorY + 20 ) - tall ) ) ;

	pTipPanel->SetPos( nXPos, nYPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestLogPanel::~CQuestLogPanel()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::AttachToGameUI( void )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_open",	"quest_log_panel" );

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
const char *CQuestLogPanel::GetName( void )
{
	return PANEL_QUEST_LOG;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::ApplySchemeSettings( IScheme *pScheme )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::ApplySchemeSettings( pScheme );

	const char *pszResFile = "Resource/UI/econ/QuestLogPanel.res";

	// Check if the operation wants to override our default res file
	const auto& mapOperations = GetItemSchema()->GetOperationDefinitions();
	FOR_EACH_MAP_FAST( mapOperations, i )
	{
		CEconOperationDefinition* pOperation = mapOperations[i];
		if ( pOperation->IsActive() && pOperation->IsCampaign() )
		{
			// Use the first found for now
			if ( pOperation->GetQuestLogOverrideResFile() )
			{
				pszResFile = pOperation->GetQuestLogOverrideResFile();
			}
			break;
		}
	}

	LoadControlSettings( pszResFile );

	g_spItemTooltip = m_pMouseOverTooltip;
	g_spTextTooltip = m_pToolTip;

	// The outer dim / close button
	{
		Button* pButton = FindControl<Button>( "OutsideCloseButton" );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
			pButton->SetPaintBackgroundEnabled( false );
		}
	}

	m_pQuestList->InvalidateLayout( false, true );

	Assert( m_pQuestList );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( GetUniverse() != k_EUniversePublic )
	{
		int x, y;
		m_pDebugButton->GetParent()->GetPos( x, y );
		int w, h;
		m_pDebugButton->GetParent()->GetSize( w, h );
		m_pDebugButton->SizeToContents();
		m_pDebugButton->SetVisible( true );
		m_pDebugButton->SetPos( x + w - m_pDebugButton->GetWide() - 60, y + 15 );
		m_pDebugButton->SetZPos( 1000 );
	}
	else
	{
		m_pDebugButton->SetVisible( false );
	}

	UpdateQuestsItemPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "close" ) )
	{
		if ( enginevgui->IsGameUIVisible() )
		{
			ShowPanel( false );
		}
		else
		{
			IViewPortPanel *pQuestLog = ( gViewPortInterface->FindPanelByName( PANEL_QUEST_LOG ) );
			if ( pQuestLog )
			{
				gViewPortInterface->ShowPanel( pQuestLog, false );
			}
		}
	}
	else if ( Q_stricmp( "open_debug_menu", pCommand ) == 0 )
	{
		if ( GetUniverse() == k_EUniverseBeta || GetUniverse() == k_EUniverseDev )
		{
			const char *pszContextMenuBorder = "NotificationDefault";
			const char *pszContextMenuFont = "HudFontMediumSecondary";

			Menu *pContextMenu = new Menu( this, "ContextMenu" );
			pContextMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
			pContextMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
		

			MenuBuilder contextMenuBuilder( pContextMenu, this );

			const auto& mapOperations = GetItemSchema()->GetOperationDefinitions();
			FOR_EACH_MAP_FAST( mapOperations, iOperation )
			{
				bool bHasAnyQuests = false;
				Menu* pOperationSubMenu = NULL;

				CEconOperationDefinition *pOperation = mapOperations[ iOperation ];
				const CEconLootListDefinition *pLootListDef = GetItemSchema()->GetLootListByName( pOperation->GetOperationLootlist() );
				if ( pLootListDef )
				{
					auto& vecContents = pLootListDef->GetLootListContents();
					FOR_EACH_VEC( vecContents, i )
					{
						if ( vecContents[i].m_iItemOrLootlistDef > 0 )
						{
							const GameItemDefinition_t* pItemDef = (GameItemDefinition_t*)GetItemSchema()->GetItemDefinition( vecContents[i].m_iItemOrLootlistDef );
							if ( pItemDef && pItemDef->GetQuestDef() )
							{
								if ( !bHasAnyQuests )
								{
									bHasAnyQuests = true;

									pOperationSubMenu = new Menu( this, "OperationSubMenu" );
									pOperationSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
									pOperationSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont ) );

									contextMenuBuilder.AddCascadingMenuItem( pOperation->GetName(), pOperationSubMenu, "operations" );
								}

								pOperationSubMenu->AddMenuItem( pItemDef->GetItemBaseName(), CFmtStr( "give%s", pItemDef->GetDefinitionName() ), this );
							}
						}
					}
				}
			}

			bool bHasAnyQuests = false;
			Menu* pAllSubMenu = NULL;

			FOR_EACH_MAP_FAST( GetItemSchema()->GetItemDefinitionMap(), i )
			{
				const GameItemDefinition_t* pItemDef = (GameItemDefinition_t*)GetItemSchema()->GetItemDefinitionMap()[ i ];
				if ( pItemDef->GetQuestDef() )
				{
					if ( !bHasAnyQuests )
					{
						bHasAnyQuests = true;

						pAllSubMenu = new Menu( this, "AllSubMenu" );
						pAllSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
						pAllSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont ) );

						contextMenuBuilder.AddCascadingMenuItem( "all", pAllSubMenu, "all" );
					}

					pAllSubMenu->AddMenuItem( pItemDef->GetItemBaseName(), CFmtStr( "give%s", pItemDef->GetDefinitionName() ), this );
				}
			}

			// Position to the cursor's position
			int nX, nY;
			g_pVGuiInput->GetCursorPosition( nX, nY );
			pContextMenu->SetPos( nX - 1, nY - 1 );
	
			pContextMenu->SetVisible(true);
			pContextMenu->AddActionSignalTarget(this);
		}
	}
	else if ( Q_strnicmp( "give", pCommand, 4 ) == 0 )
	{
		if ( GetUniverse() != k_EUniversePublic )
		{
			if ( !steamapicontext || !steamapicontext->SteamUser() )
			{
				Msg("Not connected to Steam.\n");
				return;
			}

			CSteamID steamIDForPlayer = steamapicontext->SteamUser()->GetSteamID();
			if ( !steamIDForPlayer.IsValid() )
			{
				Msg("Failed to find a valid steamID for the local player.\n");
				return;
			}

			quest_def_index_t nDefIndex = atoi( pCommand + 4 );
			const CQuestDefinition* pQuestDef = GetItemSchema()->GetQuestDefinitionByDefIndex( nDefIndex );
			if ( !pQuestDef )
			{
				Msg( "Failed to find quest def index %d.\n", nDefIndex );
				return;
			}

			Msg( "Sending request to generate '%s' for Local Player (%llu)\n", pQuestDef->GetName(), steamIDForPlayer.ConvertToUint64() );

			GCSDK::CProtoBufMsg<CMsgGCQuestDevGive> msg( k_EMsgGC_QuestDevGive );
			msg.Body().set_quest_def_index( nDefIndex );
			
			GCClientSystem()->BSendMessage( msg );
		}
	}
}

void CQuestLogPanel::SOEvent( const GCSDK::CSharedObject *pObject )
{
	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		m_bInventoryDirty = true;

		if ( IsVisible() )
		{
			if ( m_pQuestList )
			{
				m_pQuestList->PopulateQuestLists();
				m_pQuestList->UpdateEmptyMessage();
			}

			UpdateQuestsItemPanels();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::FireGameEvent( IGameEvent *event )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	// Listen for inventory updates in case our item gets changed while the user
	// is looking at us.  We want to re-do our entire layout since a quest might
	// have been equipped / destroyed / completed and we need to re-categorize
	// all the user's quests.
	if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		ShowPanel( false );
		return;
	}
	else if ( FStrEq( event->GetName(), "econ_inventory_connected" ) )
	{
		m_bInventoryDirty = true;
		InvalidateLayout( false, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::ShowPanel( bool bShow )
{
	// Snag this so we know what to listen for
	m_iQuestLogKey = gameuifuncs->GetButtonCodeForBind( "show_quest_log" );

	if ( m_pQuestList && bShow )
	{
		m_pQuestList->SetSelected( NULL, true );
		m_pQuestList->DirtyQuestLayout();
	}

	SetVisible( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::OnKeyCodePressed( KeyCode code )
{
	if ( code == m_iQuestLogKey || code == STEAMCONTROLLER_B )
	{
		ShowPanel( false );
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		if ( IsVisible() )
		{
			SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::SetVisible( bool bState ) 
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	// Default to showing the active quests upon opening
	if ( bState == true )
	{
		UpdateQuestsItemPanels();

		IGameEvent *event = gameeventmanager->CreateEvent( "questlog_opened" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}

		if ( enginevgui->IsGameUIVisible() )
		{
			AttachToGameUI();
		}
		else
		{
			ipanel()->SetParent( GetVPanel(), VGui_GetClientDLLRootPanel() );

			MakePopup( false, true );
			SetKeyBoardInputEnabled( true );
			SetMouseInputEnabled( true );
			MoveToFront();
		}

		engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );
		vgui::surface()->PlaySound( "ui/panel_open.wav" );
	}
	else if ( IsVisible() )
	{
		// Detach from the GameUI when we hide
		IViewPortPanel *pMMOverride = gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE );
		if ( pMMOverride )
		{
			((CHudMainMenuOverride*)pMMOverride)->AttachToGameUI();	
		}

		engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );
		vgui::surface()->PlaySound( "ui/panel_close.wav" );
	}

	BaseClass::SetVisible( bState );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::QuestCompletedResponse()
{
	m_bWaitingForComplete = false;
	m_bInventoryDirty = true;

	if ( m_pQuestList )
		m_pQuestList->QuestCompletedResponse();

	//UpdateQuestsItemPanels();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::UpdateQuestsItemPanels()
{
	UpdateBadgeProgressPanels();

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	if ( m_bInventoryDirty )
	{
		m_pQuestList->QuestCompletedResponse();

		if ( m_pQuestList )
		{
			m_pQuestList->PopulateQuestLists();
			m_pQuestList->UpdateEmptyMessage();
		}
	}

	m_bInventoryDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::MarkQuestsDirty()
{
	m_bInventoryDirty = true;

	if ( IsVisible() )
	{
		UpdateQuestsItemPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if any quest item panels are in the passed in state
//-----------------------------------------------------------------------------
bool CQuestLogPanel::AnyQuestItemPanelsInState( CQuestItemPanel::EItemPanelState_t eState ) const
{
	return m_pQuestList->AnyQuestItemPanelsInState( eState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::OnCompleteQuest( void )
{
	m_bWaitingForComplete = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestLogPanel::UpdateBadgeProgressPanels()
{
	CEconOperationDefinition *pCurrentOperation = NULL;
	const auto& mapOperations = GetItemSchema()->GetOperationDefinitions();
	FOR_EACH_MAP_FAST( mapOperations, i )
	{
		CEconOperationDefinition* pOperation = mapOperations[i];
		if ( pOperation->IsActive() && pOperation->IsCampaign() )
		{
			pCurrentOperation = pOperation;
			break;
		}
	}

	if ( pCurrentOperation )
	{
		CEconItemView *pCoin = TFInventoryManager()->GetLocalTFInventory()->FindFirstItembyItemDef( pCurrentOperation->GetRequiredItemDefIndex() );
		if ( pCoin )
		{
			m_pProgressPanel->SetVisible( true );
			CItemModelPanel *pCoinPanel = m_pProgressPanel->FindControl< CItemModelPanel >( "CoinModelPanel" );
			if ( pCoinPanel )
			{
				pCoinPanel->SetItem( pCoin );
			}

			uint32 nNumCompletedContracts = 0;
			{
				uint32 nCompletedContractsTemp = 0;
				GetKilleaterValueByEvent( pCoin, kKillEaterEvent_CosmeticOperationContractsCompleted, nCompletedContractsTemp );
				nNumCompletedContracts = Max( nNumCompletedContracts, nCompletedContractsTemp );
				// Halloween has it's own thing for whatever reason
				GetKilleaterValueByEvent( pCoin, kKillEaterEvent_HalloweenContractsCompleted, nCompletedContractsTemp );
				nNumCompletedContracts = Max( nNumCompletedContracts, nCompletedContractsTemp );
				Assert( pCurrentOperation->GetMaxDropCount() > 0 );
			}

			uint32 nNumContractPoints = 0;
			{
				uint32 nContractsPointsTemp = 0;
				GetKilleaterValueByEvent( pCoin, kKillEaterEvent_CosmeticOperationContractsPoints, nContractsPointsTemp );
				nNumContractPoints = Max( nContractsPointsTemp, nNumContractPoints );
				// Halloween has it's own thing for whatever reason
				GetKilleaterValueByEvent( pCoin, kKillEaterEvent_HalloweenSouls, nContractsPointsTemp );
				nNumContractPoints = Max( nContractsPointsTemp, nNumContractPoints );
			}

			EditablePanel *pBadgeContainer = m_pProgressPanel->FindControl< EditablePanel >( "BadgeMeterContainer" );
			if ( pBadgeContainer )
			{
				ContinuousProgressBar *pBadgeProgressBar = pBadgeContainer->FindControl< ContinuousProgressBar >( "BadgeProgressMeter" );
				if ( pBadgeProgressBar )
				{
					const char *pszLevelingDataName = GetItemSchema()->GetKillEaterScoreTypeLevelingDataName( kKillEaterEvent_HalloweenSouls );
					Assert( pszLevelingDataName );

					const CUtlVector<CItemLevelingDefinition> *pLevelingData = GetItemSchema()->GetItemLevelingData( pszLevelingDataName );
					Assert( pLevelingData );

					int nRequiredPointsToNextRank = 0;
					int nRequiredPointsToCurrentRank = 0;
					const char *pszCurrentLevelName = NULL;
					FOR_EACH_VEC( (*pLevelingData), i )
					{
						pszCurrentLevelName = (*pLevelingData)[i].GetNameLocalizationKey();

						const uint32 nRank = (*pLevelingData)[i].GetRequiredScore();
						if ( nNumContractPoints < nRank )
						{
							nRequiredPointsToNextRank = nRank;
							break;
						}
						else
						{
							nRequiredPointsToCurrentRank = nRank;
						}
					}

					// if no next level, just use max points
					if ( nRequiredPointsToNextRank == 0 )
					{
						// assuming each contract's worth 130 (100 point + 30 bonus)
						nRequiredPointsToNextRank = pCurrentOperation->GetMaxDropCount() * 130;
					}

					float flProgress = (float)( nNumContractPoints - nRequiredPointsToCurrentRank ) / (float)( nRequiredPointsToNextRank - nRequiredPointsToCurrentRank );
					pBadgeProgressBar->SetProgress( flProgress );

					CExLabel *pLabel = m_pProgressPanel->FindControl< CExLabel >( "BadgeProgressLabel" );
					if ( pLabel )
					{
						pLabel->SetText( CConstructLocalizedString( g_pVGuiLocalize->Find( "QuestLog_BadgeProgress" ), g_pVGuiLocalize->Find( pszCurrentLevelName ) ) );
					}

					CExLabel *pScoreLabel = pBadgeContainer->FindControl< CExLabel >( "BadgeProgressMeterText" );
					if ( pScoreLabel )
					{
						pScoreLabel->SetText( CFmtStr( "%d/%d", nNumContractPoints, nRequiredPointsToNextRank ) );
					}
				}
			}

			EditablePanel *pContractContainer = m_pProgressPanel->FindControl< EditablePanel >( "ContractMeterContainer" );
			if ( pContractContainer )
			{
				ContinuousProgressBar *pContractProgressBar = pContractContainer->FindControl< ContinuousProgressBar >( "ContractsCompletedProgressMeter" );
				if ( pContractProgressBar )
				{
					pContractProgressBar->SetProgress( (float)nNumCompletedContracts / (float)pCurrentOperation->GetMaxDropCount() );

					CExLabel *pLabel = pContractContainer->FindControl< CExLabel >( "ContractsCompletedProgressMeterText" );
					if ( pLabel )
					{
						pLabel->SetText( CFmtStr( "%d", nNumCompletedContracts ) );
					}
				}
			}			
		}
		else
		{
			m_pProgressPanel->SetVisible( false );
		}
	}
	else
	{
		m_pProgressPanel->SetVisible( false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler for when a quest has been completed
//-----------------------------------------------------------------------------
class CGCCompleteQuestCompleteResponse : public GCSDK::CGCClientJob
{
public:
	CGCCompleteQuestCompleteResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		itemid_t nNewToolID = 0;
		if( !msg.BReadUint64Data( &nNewToolID ) )
			return true;

		CQuestLogPanel *pQuestLog = GetQuestLog();
		if ( pQuestLog )
		{
			pQuestLog->QuestCompletedResponse();
		}

		return true;
	}
};

GC_REG_JOB( GCSDK::CGCClient, CGCCompleteQuestCompleteResponse, "CGCCompleteQuestCompleteResponse", k_EMsgGCQuestCompleted, GCSDK::k_EServerTypeGCClient );



#endif
