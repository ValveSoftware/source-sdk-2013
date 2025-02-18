//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "crafting_panel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "c_tf_player.h"
#include "gamestringpool.h"
#include "iclientmode.h"
#include "tf_item_inventory.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include <vgui_controls/TextEntry.h>
#include "vgui/IInput.h"
#include "gcsdk/gcclient.h"
#include "gcsdk/gcclientjob.h"
#include "character_info_panel.h"
#include "charinfo_loadout_subpanel.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "tf_hud_notification_panel.h"
#include "tf_hud_chat.h"
#include "c_tf_gamestats.h"
#include "confirm_dialog.h"
#include "econ_notifications.h"
#include "gc_clientsystem.h"
#include "charinfo_loadout_subpanel.h"
#include "item_selection_criteria.h"
#include "rtime.h"
#include "c_tf_freeaccount.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_explanations_craftingpanel( "tf_explanations_craftingpanel", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );

struct recipefilter_data_t
{
	const char *pszTooltipString;
	const char *pszButtonImage;
	const char *pszButtonImageMouseover;
};
recipefilter_data_t g_RecipeFilters[NUM_RECIPE_CATEGORIES] =
{
	{ "#RecipeFilter_Crafting",	"crafticon_crafting_items", "crafticon_crafting_items_over" },			// RECIPE_CATEGORY_CRAFTINGITEMS,
	{ "#RecipeFilter_CommonItems", "crafticon_common_items", "crafticon_common_items_over" },		// RECIPE_CATEGORY_COMMONITEMS,
	{ "#RecipeFilter_RareItems", "crafticon_rare_items", "crafticon_rare_items_over" },			// RECIPE_CATEGORY_RAREITEMS,
	{ "#RecipeFilter_Special", "crafticon_special_blueprints", "crafticon_special_blueprints_over" }			// RECIPE_CATEGORY_SPECIAL,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
wchar_t *LocalizeRecipeStringPiece( const char *pszString, wchar_t *pszConverted, int nConvertedSizeInBytes ) 
{
	if ( !pszString )
		return L"";

	if ( pszString[0] == '#' )
		return g_pVGuiLocalize->Find( pszString );

	g_pVGuiLocalize->ConvertANSIToUnicode( pszString, pszConverted, nConvertedSizeInBytes );
	return pszConverted;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetItemPanelToRecipe( CItemModelPanel *pPanel, const CEconCraftingRecipeDefinition *pRecipeDef, bool bShowName )
{
	wchar_t	wcTmpName[512];
	wchar_t	wcTmpDesc[512];
	int iNegAttribsBegin = 0;

	if ( !pRecipeDef )
	{
		Q_wcsncpy( wcTmpName, g_pVGuiLocalize->Find( "#Craft_Recipe_Custom" ), sizeof( wcTmpName ) );
		Q_wcsncpy( wcTmpDesc, g_pVGuiLocalize->Find( "#Craft_Recipe_CustomDesc" ), sizeof( wcTmpDesc ) );
		iNegAttribsBegin = Q_wcslen( wcTmpDesc );
	}
	else
	{
		if ( bShowName )
		{
			wchar_t *pName_A = g_pVGuiLocalize->Find( pRecipeDef->GetName_A() );
			g_pVGuiLocalize->ConstructString_safe( wcTmpName, g_pVGuiLocalize->Find( pRecipeDef->GetName() ), 1, pName_A );
		}
		else
		{
			wcTmpName[0] = '\0';
		}

		wchar_t wcTmpA[32];
		wchar_t wcTmpB[32];
		wchar_t wcTmpC[32];
		wchar_t	wcTmp[512];

		// Build the input string
		wchar_t *pInp_A = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_A(), wcTmpA, sizeof( wcTmpA ) );
		wchar_t *pInp_B = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_B(), wcTmpB, sizeof( wcTmpB ) );
		wchar_t *pInp_C = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_C(), wcTmpC, sizeof( wcTmpC ) );
		g_pVGuiLocalize->ConstructString_safe( wcTmpDesc, g_pVGuiLocalize->Find( pRecipeDef->GetDescInputs() ), 3, pInp_A, pInp_B, pInp_C );
		iNegAttribsBegin = Q_wcslen(wcTmpDesc);

		// Build the output string
		wchar_t *pOut_A = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_A(), wcTmpA, sizeof( wcTmpA ) );
		wchar_t *pOut_B = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_B(), wcTmpB, sizeof( wcTmpB ) );
		wchar_t *pOut_C = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_C(), wcTmpC, sizeof( wcTmpC ) );
		g_pVGuiLocalize->ConstructString_safe( wcTmp, g_pVGuiLocalize->Find( pRecipeDef->GetDescOutputs() ), 3, pOut_A, pOut_B, pOut_C );

		// Concatenate, and mark the text changes
		V_wcscat_safe( wcTmpDesc, L"\n" );
		V_wcscat_safe( wcTmpDesc, wcTmp );
	}

	pPanel->SetAttribOnly( !bShowName );
	pPanel->SetTextYPos( 0 );
	pPanel->SetItem( NULL );
	pPanel->SetNoItemText( wcTmpName, wcTmpDesc, iNegAttribsBegin );
	pPanel->InvalidateLayout(true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PositionMouseOverPanelForRecipe( vgui::Panel *pScissorPanel, vgui::Panel *pRecipePanel, vgui::ScrollableEditablePanel *pRecipeScroller, CItemModelPanel *pMouseOverItemPanel )
{
	int x,y;
	vgui::ipanel()->GetAbsPos( pRecipePanel->GetVPanel(), x, y );
	int xs,ys;
	vgui::ipanel()->GetAbsPos( pMouseOverItemPanel->GetParent()->GetVPanel(), xs, ys );
	x -= xs;
	y -= ys;

	int iXPos = (x + (pRecipePanel->GetWide() * 0.5)) - (pMouseOverItemPanel->GetWide() * 0.5);
	int iYPos = (y + pRecipePanel->GetTall());

	// Make sure the popup stays onscreen.
	if ( iXPos < 0 )
	{
		iXPos = 0;
	}
	else if ( (iXPos + pMouseOverItemPanel->GetWide()) > pMouseOverItemPanel->GetParent()->GetWide() )
	{
		iXPos = pMouseOverItemPanel->GetParent()->GetWide() - pMouseOverItemPanel->GetWide();
	}

	if ( iYPos < 0 )
	{
		iYPos = 0;
	}
	else if ( (iYPos + pMouseOverItemPanel->GetTall() + YRES(32)) > pMouseOverItemPanel->GetParent()->GetTall() )
	{
		// Move it up above our item
		iYPos = y - pMouseOverItemPanel->GetTall() - YRES(4);
	}

	pMouseOverItemPanel->SetPos( iXPos, iYPos );
	pMouseOverItemPanel->SetVisible( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCraftingPanel::CCraftingPanel( vgui::Panel *parent, const char *panelName ) : CBaseLoadoutPanel( parent, panelName )
{
	m_pRecipeListContainer = new vgui::EditablePanel( this, "recipecontainer" );
	m_pRecipeListContainerScroller = new vgui::ScrollableEditablePanel( this, m_pRecipeListContainer, "recipecontainerscroller" );
	m_pSelectedRecipeContainer = new vgui::EditablePanel( this, "selectedrecipecontainer" );
	m_pRecipeButtonsKV = NULL;
	m_pRecipeFilterButtonsKV = NULL;
	m_bEventLogging = false;
	m_iCraftingAttempts = 0;
	m_iRecipeCategoryFilter = RECIPE_CATEGORY_CRAFTINGITEMS;
	m_iCurrentlySelectedRecipe = -1;
	CleanupPostCraft( true );

	m_pToolTip = new CTFTextToolTip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	m_pSelectionPanel = NULL;
	m_iSelectingForSlot = 0;

	m_pCraftButton = NULL;
	m_pUpgradeButton = NULL;
	m_pFreeAccountLabel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCraftingPanel::~CCraftingPanel( void )
{
	if ( m_pRecipeButtonsKV )
	{
		m_pRecipeButtonsKV->deleteThis();
		m_pRecipeButtonsKV = NULL;
	}
	if ( m_pRecipeFilterButtonsKV )
	{
		m_pRecipeFilterButtonsKV->deleteThis();
		m_pRecipeFilterButtonsKV = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( GetResFile() );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pRecipeListContainerScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pCraftButton = dynamic_cast<CExButton*>( m_pSelectedRecipeContainer->FindChildByName("CraftButton") );
	if ( m_pCraftButton )
	{
		m_pCraftButton->AddActionSignalTarget( this );
	}
	m_pUpgradeButton = dynamic_cast<CExButton*>( m_pSelectedRecipeContainer->FindChildByName("UpgradeButton") );
	if ( m_pUpgradeButton )
	{
		m_pUpgradeButton->AddActionSignalTarget( this );
	}
	m_pFreeAccountLabel = dynamic_cast<CExLabel*>( m_pSelectedRecipeContainer->FindChildByName("FreeAccountLabel") );

	CreateRecipeFilterButtons();
	UpdateRecipeFilter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "recipebuttons_kv" );
	if ( pItemKV )
	{
		if ( m_pRecipeButtonsKV )
		{
			m_pRecipeButtonsKV->deleteThis();
		}
		m_pRecipeButtonsKV = new KeyValues("recipebuttons_kv");
		pItemKV->CopySubkeys( m_pRecipeButtonsKV );
	}
	
	KeyValues *pButtonKV = inResourceData->FindKey( "recipefilterbuttons_kv" );
	if ( pButtonKV )
	{
		if ( m_pRecipeFilterButtonsKV )
		{
			m_pRecipeFilterButtonsKV->deleteThis();
		}
		m_pRecipeFilterButtonsKV = new KeyValues("recipefilterbuttons_kv");
		pButtonKV->CopySubkeys( m_pRecipeFilterButtonsKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	// Need to lay these out before we start making item panels inside them
	m_pRecipeListContainer->InvalidateLayout( true );
	m_pRecipeListContainerScroller->InvalidateLayout( true );

	// Position the recipe filters
	FOR_EACH_VEC( m_pRecipeFilterButtons, i )
	{
		if ( m_pRecipeFilterButtonsKV )
		{
			m_pRecipeFilterButtons[i]->ApplySettings( m_pRecipeFilterButtonsKV );
			m_pRecipeFilterButtons[i]->InvalidateLayout();
		} 

		int iButtonW, iButtonH;
		m_pRecipeFilterButtons[i]->GetSize( iButtonW, iButtonH );

		int iXPos = (GetWide() * 0.5) + m_iFilterOffcenterX + ((iButtonW + m_iFilterDeltaX) * i);
		int iYPos = m_iFilterYPos;// + ((iButtonH + m_iFilterDeltaY) * i);
		m_pRecipeFilterButtons[i]->SetPos( iXPos, iYPos );
	}

	// Position the recipe buttons
	for ( int i = 0; i < m_pRecipeButtons.Count(); i++ )
	{
		if ( m_pRecipeButtonsKV )
		{
			m_pRecipeButtons[i]->ApplySettings( m_pRecipeButtonsKV );
			m_pRecipeButtons[i]->InvalidateLayout();
		} 

		int iYDelta = m_pRecipeButtons[0]->GetTall() + YRES(2);

		// Once we've setup our first item, we know how large to make the container
		if ( i == 0 )
		{
			m_pRecipeListContainer->SetSize( m_pRecipeListContainer->GetWide(), iYDelta * m_pRecipeButtons.Count() );
		}

		int x,y;
		m_pRecipeButtons[i]->GetPos( x,y );
		m_pRecipeButtons[i]->SetPos( x, (iYDelta * i) );
	}

	// Now that the container has been sized, tell the scroller to re-evaluate
	m_pRecipeListContainerScroller->InvalidateLayout();
	m_pRecipeListContainerScroller->GetScrollbar()->InvalidateLayout();

	// Then position all our item panels
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		PositionItemPanel( m_pItemModelPanels[i], i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::CreateRecipeFilterButtons( void )
{
	for ( int i = 0; i < NUM_RECIPE_CATEGORIES; i++ )
	{
		if ( m_pRecipeFilterButtons.Count() <= i )
		{
			CImageButton *pNewButton = new CImageButton( this, g_RecipeFilters[i].pszTooltipString );
			m_pRecipeFilterButtons.AddToTail( pNewButton );
		}

		m_pRecipeFilterButtons[i]->SetInactiveImage( g_RecipeFilters[i].pszButtonImage );
		m_pRecipeFilterButtons[i]->SetActiveImage( g_RecipeFilters[i].pszButtonImageMouseover );
		m_pRecipeFilterButtons[i]->SetTooltip( m_pToolTip, g_RecipeFilters[i].pszTooltipString );
		const char *pszCommand = VarArgs("selectfilter%d", i );
		m_pRecipeFilterButtons[i]->SetCommand( pszCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::UpdateRecipeFilter( void )
{
	int iMatchingRecipes = 0;
	m_iCurrentlySelectedRecipe = -1;
	m_iCurrentRecipeTotalInputs = 0;
	m_iCurrentRecipeTotalOutputs = 0;

	FOR_EACH_VEC( m_pRecipeFilterButtons, i )
	{
		bool bForceDepressed = ( i == m_iRecipeCategoryFilter );
		m_pRecipeFilterButtons[i]->ForceDepressed( bForceDepressed );
	}

	// Loop through the known recipes, and see which ones match our category filter
	for ( int i = 0; i < TFInventoryManager()->GetLocalTFInventory()->GetRecipeCount(); i++ )
	{
		const CEconCraftingRecipeDefinition *pRecipeDef = TFInventoryManager()->GetLocalTFInventory()->GetRecipeDef(i);
		if ( !pRecipeDef )
			continue;

		if ( pRecipeDef->IsDisabled() )
			continue;

		if ( pRecipeDef->GetCategory() != m_iRecipeCategoryFilter )
			continue;

		wchar_t	wTemp[256];
		wchar_t *pName_A = g_pVGuiLocalize->Find( pRecipeDef->GetName_A() );
		g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find( pRecipeDef->GetName() ), 1, pName_A );
		SetButtonToRecipe( iMatchingRecipes, pRecipeDef->GetDefinitionIndex(), wTemp );

		iMatchingRecipes++;
	}

	// Add a "Custom" option to the bottom of the Special recipe list
	if ( m_iRecipeCategoryFilter == RECIPE_CATEGORY_SPECIAL )
	{
		SetButtonToRecipe( iMatchingRecipes, RECIPE_CUSTOM, g_pVGuiLocalize->Find("#Craft_Recipe_Custom") );	
		iMatchingRecipes++;
	}

	// Delete excess buttons
	for ( int i = m_pRecipeButtons.Count() - 1; i >= iMatchingRecipes; i-- )
	{
		m_pRecipeButtons[i]->MarkForDeletion();
		m_pRecipeButtons.Remove( i	);
	}

	// Move the scrollbar to the top
	m_pRecipeListContainerScroller->GetScrollbar()->SetValue( 0 );	

	UpdateSelectedRecipe( true );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnCancelSelection( void )
{
	if ( m_pSelectionPanel )
	{
		m_pSelectionPanel->SetVisible( false );
	}

	CloseCraftingStatusDialog();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnSelectionReturned( KeyValues *data )
{
	if ( data )
	{
		uint64 ulIndex = data->GetUint64( "itemindex", INVALID_ITEM_ID );
		if ( ulIndex == INVALID_ITEM_ID )
		{
			// should this be INVALID_ITEM_ID?		
			m_InputItems[m_iSelectingForSlot] = 0;
		}
		else
		{
			m_InputItems[m_iSelectingForSlot] = ulIndex;
		}

		UpdateModelPanels();
		UpdateCraftButton();
	}

	// It'll have deleted itself, so we don't need to clean it up
	OnCancelSelection();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnShowPanel( bool bVisible, bool bReturningFromArmory )
{
	if ( bVisible )
	{
		if ( m_pSelectionPanel )
		{
			m_pSelectionPanel->SetVisible( false );
		}

		memset( m_InputItems, 0, sizeof(m_InputItems) );
		memset( m_ItemPanelCriteria, 0, sizeof(m_ItemPanelCriteria) );
		m_iCurrentlySelectedRecipe = -1;
		m_iCurrentRecipeTotalInputs = 0;
		m_iCurrentRecipeTotalOutputs = 0;
		UpdateRecipeFilter();

		if ( !m_bEventLogging )
		{
			m_bEventLogging = true;
			C_CTF_GameStats.Event_Crafting( IE_CRAFTING_ENTERED );
		}
	}
	else
	{
		CloseCraftingStatusDialog();
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	BaseClass::OnShowPanel( bVisible, bReturningFromArmory );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnClosing()
{
	if ( m_bEventLogging )
	{
		C_CTF_GameStats.Event_Crafting( IE_CRAFTING_EXITED );
		m_bEventLogging = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCraftingPanel::PositionItemPanel( CItemModelPanel *pPanel, int iIndex )
{
	int iCenter = 0;
	int iButtonX, iButtonY, iXPos, iYPos;

	if ( IsInputItemPanel(iIndex) )
	{
		iButtonX = (iIndex % CRAFTING_SLOTS_INPUT_COLUMNS);
		iButtonY = (iIndex / CRAFTING_SLOTS_INPUT_COLUMNS);
		iXPos = (iCenter + m_iItemCraftingOffcenterX) + (iButtonX * m_pItemModelPanels[iIndex]->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		iYPos = m_iItemYPos + (iButtonY * m_pItemModelPanels[iIndex]->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);
	}
	else
	{
		int iButtonIndex = iIndex - CRAFTING_SLOTS_INPUTPANELS;
		iButtonX = (iButtonIndex % CRAFTING_SLOTS_OUTPUT_COLUMNS);
		iButtonY = (iButtonIndex / CRAFTING_SLOTS_OUTPUT_COLUMNS);
		iXPos = (iCenter + m_iItemCraftingOffcenterX) + (iButtonX * m_pItemModelPanels[iIndex]->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		iYPos = m_iOutputItemYPos + (iButtonY * m_pItemModelPanels[iIndex]->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);
	}

	m_pItemModelPanels[iIndex]->SetPos( iXPos, iYPos );
	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCraftingPanel::UpdateRecipeItems( bool bClearInputItems )
{
	if ( bClearInputItems )
	{
		memset( m_InputItems, 0, sizeof(m_InputItems) );
	}

	memset( m_ItemPanelCriteria, 0, sizeof(m_ItemPanelCriteria) );
	m_iCurrentRecipeTotalInputs = 0;
	m_iCurrentRecipeTotalOutputs = 0;

	if ( m_iCurrentlySelectedRecipe == -1 )
		return;

	/*
	// Build lists of items divided by class & loadout slot, so recipes can quickly test themselves
	CUtlVector<CEconItem*> vecAllItems;
	CUtlVector<CEconItem*> vecItemsByClass[ LOADOUT_COUNT ];
	CUtlVector<CEconItem*> vecItemsBySlot[ LOADOUT_POSITION_COUNT ];

	for ( int i = 1; i <= TFInventoryManager()->GetLocalTFInventory()->GetMaxItemCount(); i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetItemByBackpackPosition(i);
		if ( pItemData && pItemData->IsValid() )
		{
			CEconItem *pSOCData = pItemData->GetSOCData();
			vecAllItems.AddToTail( pSOCData );

			CTFItemDefinition *pItemDef = pItemData->GetStaticData();

			// Put it in class lists for any class that can use it. Use the zeroth list as all-class items.
			if ( pItemDef->CanBeUsedByAllClasses() )
			{
				vecItemsByClass[0].AddToTail( pSOCData );
			}
			for (int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
			{
				if ( pItemDef->CanBeUsedByClass(iClass) )
				{
					vecItemsByClass[iClass].AddToTail( pSOCData );
				}
			}

			// Put it in the slot lists for any slot that it can be equipped in
			for (int iSlot = 0; iSlot < LOADOUT_POSITION_COUNT; iSlot++ )
			{
				if ( pItemDef->CanBePlacedInSlot( iSlot ) )
				{
					vecItemsBySlot[iSlot].AddToTail( pSOCData );
				}
			}
		}
	}
	*/

	// Find the items needed for the specified recipe
	if ( m_iCurrentlySelectedRecipe == RECIPE_CUSTOM )
	{
		// Custom recipe. Show all open buttons, and let them put anything in there.
		m_iCurrentRecipeTotalInputs = CRAFTING_SLOTS_INPUTPANELS;
		m_iCurrentRecipeTotalOutputs = 0;

		FOR_EACH_VEC( m_pItemModelPanels, i )
		{
			m_pItemModelPanels[i]->SetNoItemText( "" );
		}
	}
	else
	{
		const CTFCraftingRecipeDefinition *pRecipeDef = (CTFCraftingRecipeDefinition*)TFInventoryManager()->GetLocalTFInventory()->GetRecipeDefByDefIndex( m_iCurrentlySelectedRecipe );
		if ( pRecipeDef )
		{
			m_iCurrentRecipeTotalInputs = pRecipeDef->GetTotalInputItemsRequired();
			m_iCurrentRecipeTotalOutputs = pRecipeDef->GetTotalOutputItems();

			CUtlVector<itemid_t> vecItemsUsed;

			// Set the text in each of the item panels
			const CUtlVector<CItemSelectionCriteria> *vecInputCriteria;
			vecInputCriteria = pRecipeDef->GetInputItems();
			CUtlVector<uint32> vecInputDupes;
			vecInputDupes = pRecipeDef->GetInputItemDupeCounts();

			int iModelPanel = 0;
			FOR_EACH_VEC( *vecInputCriteria, i )
			{
				const char *pszNoItemText = GetItemTextForCriteria( &(*vecInputCriteria)[i] );

				int iNumPanels = vecInputDupes[i] ? vecInputDupes[i] : 1;
				for ( int iPanel = 0; iPanel < iNumPanels; iPanel++ )
				{
					m_ItemPanelCriteria[iModelPanel] = &(*vecInputCriteria)[i];
					if ( m_pItemModelPanels[iModelPanel] )
					{
						m_pItemModelPanels[iModelPanel]->SetNoItemText( pszNoItemText );
					}
					iModelPanel++;
				}
			}

			// Set the output items as well
			CUtlVector<CItemSelectionCriteria> vecOutputCriteria;
			vecOutputCriteria = pRecipeDef->GetOutputItems();
			FOR_EACH_VEC( vecOutputCriteria, i )
			{
				int iOutputPanel = CRAFTING_SLOTS_INPUTPANELS + i;

				CEconItemDefinition *pDef = GetItemDefFromCriteria( &vecOutputCriteria[i] );
				if ( pDef )
				{
					//m_pItemModelPanels[iOutputPanel]->SetNoItemText( pszNoItemText );
					CEconItemView *pItemData = new CEconItemView();
					pItemData->Init( pDef->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
					if ( m_pItemModelPanels[iOutputPanel] )
					{
						m_pItemModelPanels[iOutputPanel]->SetItem( pItemData );
					}
					delete pItemData;
					continue;
				}

				// If we didn't manage to extract an output, just use the recipe output string
				wchar_t wcTmpA[32];
				wchar_t wcTmpB[32];
				wchar_t wcTmpC[32];
				wchar_t	wcTmp[512];
				wchar_t *pOut_A = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_A(), wcTmpA, sizeof( wcTmpA ) );
				wchar_t *pOut_B = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_B(), wcTmpB, sizeof( wcTmpB ) );
				wcTmp[0] = '\0';
				V_wcscat_safe( wcTmp, pOut_A );
				V_wcscat_safe( wcTmp, L" " );
				V_wcscat_safe( wcTmp, pOut_B );
				if ( Q_strnicmp( pRecipeDef->GetDescOutputs(), "#RDO_ABC", 8 ) == 0 )
				{
					wchar_t *pOut_C = LocalizeRecipeStringPiece( pRecipeDef->GetDescO_C(), wcTmpC, sizeof( wcTmpC ) );
					V_wcscat_safe( wcTmp, L" " );
					V_wcscat_safe( wcTmp, pOut_C );
				}

				if ( m_pItemModelPanels[iOutputPanel] )
				{
					m_pItemModelPanels[iOutputPanel]->SetItem( NULL );
					m_pItemModelPanels[iOutputPanel]->SetNoItemText( wcTmp );
				}
			}
		}
	}

	// Now check to see if they've got the right items in there
	UpdateCraftButton();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCraftingPanel::UpdateCraftButton( void )
{
	if ( m_iCurrentlySelectedRecipe == -1 )
		return;

	bool bAllowedToUse = true;

	const CEconCraftingRecipeDefinition *pRecipeDef = NULL;
	if ( m_iCurrentlySelectedRecipe != RECIPE_CUSTOM )
	{
		pRecipeDef = (CTFCraftingRecipeDefinition*)TFInventoryManager()->GetLocalTFInventory()->GetRecipeDefByDefIndex( m_iCurrentlySelectedRecipe );
		if ( !pRecipeDef )
			return;

		bAllowedToUse = ( !IsFreeTrialAccount() || !pRecipeDef->IsPremiumAccountOnly() );
	}

	if ( m_pCraftButton )
	{
		m_pCraftButton->SetVisible( bAllowedToUse );
	}	
	if ( m_pUpgradeButton )
	{
		m_pUpgradeButton->SetVisible( !bAllowedToUse );
	}
	if ( m_pFreeAccountLabel )
	{
		m_pFreeAccountLabel->SetVisible( !bAllowedToUse );
	}

	if ( !bAllowedToUse )
		return;

	bool bCraftButtonActive = false;
	if ( m_iCurrentlySelectedRecipe == RECIPE_CUSTOM )
	{
		// Need at least one item in a slot
		for ( int i = 0; i < CRAFTING_SLOTS_INPUTPANELS; i++ )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( m_InputItems[i] );
			if ( pItemData )
			{
				bCraftButtonActive = true;
				break;
			}
		}
	}
	else
	{
		CUtlVector<CEconItem*> vecAllItems;
		for ( int i = 0; i < CRAFTING_SLOTS_INPUTPANELS; i++ )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( m_InputItems[i] );
			if ( pItemData )
			{
				vecAllItems.AddToTail( pItemData->GetSOCData() );
			}
		}

		bCraftButtonActive = pRecipeDef->ItemListMatchesInputs( &vecAllItems, NULL, false, NULL );
	}

	if ( m_pCraftButton )
	{
		m_pCraftButton->SetEnabled( bCraftButtonActive );
	}	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCraftingPanel::GetItemTextForCriteria( const CItemSelectionCriteria *pCriteria )
{
	// Otherwise, look at the first condition, and see if we can determine what the item is
	const char *pszVal = pCriteria->GetValueForFirstConditionOfType( k_EOperator_String_EQ );
	if ( pszVal && pszVal[0] )
	{
		// Is it a loadout slot?
		int iSlot = StringFieldToInt( pszVal, ItemSystem()->GetItemSchema()->GetLoadoutStrings( EEquipType_t::EQUIP_TYPE_CLASS ), true );
		if ( iSlot != -1 )
			return ItemSystem()->GetItemSchema()->GetLoadoutStringsForDisplay( EEquipType_t::EQUIP_TYPE_CLASS )[iSlot];

		// Is it a craft material type?
		if ( V_stricmp( pszVal, "weapon" ) == 0 )
		{
			return "#RI_W";
		}
		else if ( V_stricmp( pszVal, "hat" ) == 0 )
		{
			return "#RI_Hg";
		}
		else if ( V_stricmp( pszVal, "craft_token" ) == 0 )
		{
			return "#RI_T";
		}
		else if ( V_stricmp( pszVal, "class_token" ) == 0 )
		{
			return "#CI_T_C";
		}
		else if ( V_stricmp( pszVal, "slot_token" ) == 0 )
		{
			return "#CI_T_S";
		}

		// Is it an item name?
		CEconItemDefinition *pDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName(pszVal);
		if ( pDef )
			return pDef->GetItemBaseName();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconItemDefinition *CCraftingPanel::GetItemDefFromCriteria( const CItemSelectionCriteria *pCriteria )
{
	// Otherwise, look at the first condition, and see if we can determine what the item is
	const char *pszVal = pCriteria->GetValueForFirstConditionOfType( k_EOperator_String_EQ );
	if ( pszVal && pszVal[0] )
		return ItemSystem()->GetItemSchema()->GetItemDefinitionByName(pszVal);

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCraftingPanel::AddNewItemPanel( int iPanelIndex )
{
	BaseClass::AddNewItemPanel( iPanelIndex );

	// Move the model panels to our selected recipe container
	m_pItemModelPanels[iPanelIndex]->SetParent( m_pSelectedRecipeContainer );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCraftingPanel::UpdateModelPanels( void )
{
	BaseClass::UpdateModelPanels();

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( IsInputItemPanel(i) )
		{
			if ( m_InputItems[i] != 0 )
			{
				CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( m_InputItems[i] );
				m_pItemModelPanels[i]->SetItem( pItemData );
				m_pItemModelPanels[i]->SetVisible( true );
				m_pItemModelPanels[i]->SetShowEquipped( true );
				SetBorderForItem( m_pItemModelPanels[i], false );
			}
			else
			{
				m_pItemModelPanels[i]->SetItem( NULL );

				// Always show the number of slots that the recipe uses
				bool bVisible = (m_iCurrentRecipeTotalInputs > i);
				m_pItemModelPanels[i]->SetVisible( bVisible );
			}
		}
		else
		{
			bool bVisible = ((m_iCurrentRecipeTotalOutputs + CRAFTING_SLOTS_INPUTPANELS) > i);
			m_pItemModelPanels[i]->SetVisible( bVisible );
		}
	}

	vgui::Panel *pLabel = m_pSelectedRecipeContainer->FindChildByName("OutputLabel");
	if ( pLabel )
	{
		pLabel->SetVisible( m_iCurrentRecipeTotalOutputs > 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::SetButtonToRecipe( int iButton, int iDefIndex, wchar_t *pszText )
{
	// Re-use existing buttons, or make new ones if we need more
	CRecipeButton *pRecipeButton = NULL;
	if ( iButton < m_pRecipeButtons.Count() )
	{
		pRecipeButton = m_pRecipeButtons[iButton];
	}
	else
	{
		pRecipeButton = new CRecipeButton( m_pRecipeListContainer, "selectrecipe", "", this, "selectrecipe" );
		if ( m_pRecipeButtonsKV )
		{
			pRecipeButton->ApplySettings( m_pRecipeButtonsKV );
		} 
		pRecipeButton->MakeReadyForUse();
		m_pRecipeButtons.AddToTail( pRecipeButton );
	}

	const char *pszCommand = VarArgs("selectrecipe%d", iDefIndex );
	pRecipeButton->SetCommand( pszCommand );
	pRecipeButton->SetText( pszText );
	pRecipeButton->SetDefIndex( iDefIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::UpdateSelectedRecipe( bool bClearInputItems )
{
	for ( int i = 0; i < m_pRecipeButtons.Count(); i++ )
	{
		bool bSelected = m_pRecipeButtons[i]->m_iRecipeDefIndex == m_iCurrentlySelectedRecipe;
		m_pRecipeButtons[i]->ForceDepressed( bSelected );
		m_pRecipeButtons[i]->RecalculateDepressedState();

		if ( bSelected )
		{
			wchar_t wszText[1024];
			m_pRecipeButtons[i]->GetText( wszText, ARRAYSIZE( wszText ) );
			m_pSelectedRecipeContainer->SetDialogVariable( "recipetitle", wszText );

			if ( m_iCurrentlySelectedRecipe == RECIPE_CUSTOM )
			{
				m_pSelectedRecipeContainer->SetDialogVariable( "recipeinputstring", g_pVGuiLocalize->Find("#Craft_Recipe_CustomDesc") );
			}
			else
			{
				const CTFCraftingRecipeDefinition *pRecipeDef = (CTFCraftingRecipeDefinition*)TFInventoryManager()->GetLocalTFInventory()->GetRecipeDefByDefIndex( m_iCurrentlySelectedRecipe );
				if ( pRecipeDef )
				{
					// Build the input string
					wchar_t wcTmpA[32];
					wchar_t wcTmpB[32];
					wchar_t wcTmpC[32];
					wchar_t	wcTmpDesc[512];
					wchar_t *pInp_A = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_A(), wcTmpA, sizeof( wcTmpA ) );
					wchar_t *pInp_B = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_B(), wcTmpB, sizeof( wcTmpB ) );
					wchar_t *pInp_C = LocalizeRecipeStringPiece( pRecipeDef->GetDescI_C(), wcTmpC, sizeof( wcTmpC ) );
					g_pVGuiLocalize->ConstructString_safe( wcTmpDesc, g_pVGuiLocalize->Find( pRecipeDef->GetDescInputs() ), 3, pInp_A, pInp_B, pInp_C );
					m_pSelectedRecipeContainer->SetDialogVariable( "recipeinputstring", wcTmpDesc );
				}
			}
		}
	}

	m_pSelectedRecipeContainer->SetVisible( m_iCurrentlySelectedRecipe != -1 );

	UpdateRecipeItems( bClearInputItems );
	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "selectrecipe", 12 ) )
	{
		const char *pszNum = command+12;
		if ( pszNum && pszNum[0] )
		{
			m_iCurrentlySelectedRecipe = atoi(pszNum);
			UpdateSelectedRecipe( true );
		}

		return;
	}
	if ( !Q_strnicmp( command, "selectfilter", 12 ) )
	{
		const char *pszNum = command+12;
		if ( pszNum && pszNum[0] )
		{
			m_iRecipeCategoryFilter = (recipecategories_t)atoi(pszNum);
			UpdateRecipeFilter();
		}

		return;
	}
	else if ( !Q_strnicmp( command, "back", 4 ) )
	{
		PostMessage( GetParent(), new KeyValues("CraftingClosed") );
		return;
	}
	else if ( !Q_strnicmp( command, "craft", 5 ) )
	{
		if ( CheckForUntradableItems() )
		{
			Craft();
		}
		return;
	}
	else if ( !Q_stricmp( command, "upgrade" ) )
	{
		EconUI()->CloseEconUI();
		EconUI()->OpenStorePanel( STOREPANEL_SHOW_UPGRADESTEPS, false );
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( true, true );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnRecipePanelEntered( vgui::Panel *panel )
{
	CRecipeButton *pRecipePanel = dynamic_cast < CRecipeButton * > ( panel );

	if ( pRecipePanel && IsVisible() && !IsIgnoringItemPanelEnters() )
	{
		const CEconCraftingRecipeDefinition *pRecipeDef = NULL;
		if ( pRecipePanel->m_iRecipeDefIndex != RECIPE_CUSTOM )
		{
			pRecipeDef = TFInventoryManager()->GetLocalTFInventory()->GetRecipeDefByDefIndex( pRecipePanel->m_iRecipeDefIndex );
		}

		SetItemPanelToRecipe( GetMouseOverPanel(), pRecipeDef, false );
		PositionMouseOverPanelForRecipe( this, pRecipePanel, m_pRecipeListContainerScroller, GetMouseOverPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnRecipePanelExited( vgui::Panel *panel )
{
	GetMouseOverPanel()->SetAttribOnly( false );
	GetMouseOverPanel()->SetTextYPos( YRES(20) );
	GetMouseOverPanel()->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CCraftingPanel::GetItemPanelIndex( CItemModelPanel *pItemPanel )
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i] == pItemPanel  )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnItemPanelMousePressed( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && !pItemPanel->IsGreyedOut() )
	{
		int iPos = GetItemPanelIndex(pItemPanel);
		if ( IsInputItemPanel(iPos) )
		{
			m_iSelectingForSlot = iPos;

			// Create it the first time around
			if ( !m_pSelectionPanel )
			{
				m_pSelectionPanel = new CCraftingItemSelectionPanel( this );
			}

			if ( m_iCurrentlySelectedRecipe == RECIPE_CUSTOM )
			{
				m_pSelectionPanel->UpdateOnShow( NULL, true, m_InputItems, ARRAYSIZE(m_InputItems) );
			}
			else
			{
				// Clicked on an item in the crafting area. Open up the selection panel.
				m_pSelectionPanel->UpdateOnShow( m_ItemPanelCriteria[iPos], false, m_InputItems, ARRAYSIZE(m_InputItems) );
			}

			m_pSelectionPanel->ShowDuplicateCounts( true );
			m_pSelectionPanel->ShowPanel( 0, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ConfirmCraft( bool bConfirmed, void* pContext )
{
	CCraftingPanel *pCraftingPanel = ( CCraftingPanel* )pContext;
	if ( bConfirmed )
	{
		pCraftingPanel->Craft();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCraftingPanel::CheckForUntradableItems( void )
{
	bool bHasUntradable = false;
	for ( int i = 0; i < CRAFTING_SLOTS_INPUTPANELS; i++ )
	{
		if ( m_InputItems[i] != 0 )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( m_InputItems[i] );
			if ( pItemData->IsTradable() == false )
			{
				bHasUntradable = true;
				break;
			}
		}
	}

	if ( bHasUntradable )
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#Craft_Untradable_Title", "#Craft_Untradable_Text", "#GameUI_OK", "#Cancel", &ConfirmCraft );
		pDialog->SetContext( this );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::Craft( void )
{
	// Build our list of items that we're trying to craft
	++m_iCraftingAttempts;
	CUtlVector<itemid_t> vecCraftingItems;
	for ( int i = 0; i < CRAFTING_SLOTS_INPUTPANELS; i++ )
	{
		if ( m_InputItems[i] != 0 )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( m_InputItems[i] );
			C_CTF_GameStats.Event_Crafting( IE_CRAFTING_ATTEMPT, pItemData, m_iCraftingAttempts );
			vecCraftingItems.AddToTail( m_InputItems[i] );
		}
	}

	if ( !vecCraftingItems.Count() )
		return;

	GCSDK::CGCMsg<MsgGCCraft_t> msg( k_EMsgGCCraft );
	msg.Body().m_nRecipeDefIndex = m_iCurrentlySelectedRecipe;
	msg.Body().m_nItemCount = vecCraftingItems.Count();
	for ( int i = 0; i < vecCraftingItems.Count(); i++ )
	{
		msg.AddUint64Data( vecCraftingItems[i] );
	}
	GCClientSystem()->BSendMessage( msg );

	OpenCraftingStatusDialog( this, "#CraftUpdate_Start", true, false, false );

	// Start ticking so we can give up waiting if we don't get a response from the GC
	// We use the VGUI time, because we may not be in a game at all.
	m_flAbortCraftingAt = vgui::system()->GetCurrentTime() + 10;
	m_bWaitingForCraftItems = false;
	m_iRecipeIndexTried = m_iCurrentlySelectedRecipe;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnCraftResponse( EGCMsgResponse eResponse, CUtlVector<uint64> *vecCraftedIndices, int iRecipeUsed )
{
	switch ( eResponse )
	{
		case k_EGCMsgResponseNoMatch:
		{
			C_CTF_GameStats.Event_Crafting( IE_CRAFTING_NO_RECIPE_MATCH, NULL, m_iCraftingAttempts );
			CleanupPostCraft( m_iCurrentlySelectedRecipe != RECIPE_CUSTOM );
			OpenCraftingStatusDialog( this, "#CraftUpdate_NoMatch", false, true, false );
		}
		break;

		case k_EGCMsgResponseDenied:
		{
			// Craft denied.
			C_CTF_GameStats.Event_Crafting( IE_CRAFTING_FAILURE, NULL, m_iCraftingAttempts );
			CleanupPostCraft( m_iCurrentlySelectedRecipe != RECIPE_CUSTOM );
			OpenCraftingStatusDialog( this, "#CraftUpdate_Denied", false, true, false );			
		}
		break;

		// We've got the list of items crafted. We save off the item list until our item cache has all the items.
		case k_EGCMsgResponseOK:
		{
			// Start ticking, and wait until the cache contains all the items in the list.
			m_bWaitingForCraftItems = true;
			m_vecNewlyCraftedItems = *vecCraftedIndices;

			if ( iRecipeUsed != m_iRecipeIndexTried && iRecipeUsed != -1 )
			{
				m_iNewRecipeIndex = iRecipeUsed;
			}
		}
		break;

		default:
		{
			// Craft failed in some way.
			C_CTF_GameStats.Event_Crafting( IE_CRAFTING_FAILURE, NULL, m_iCraftingAttempts );
			OpenCraftingStatusDialog( this, "#CraftUpdate_Failed", false, true, false );
			CleanupPostCraft( m_iCurrentlySelectedRecipe != RECIPE_CUSTOM );
		}
		break;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::ShowCraftFinish( void )
{
	TFInventoryManager()->ShowItemsCrafted( &m_vecNewlyCraftedItems );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( IsVisible() )
	{
		if ( m_flAbortCraftingAt )
		{
			if ( m_flAbortCraftingAt < vgui::system()->GetCurrentTime() )
			{
				C_CTF_GameStats.Event_Crafting( IE_CRAFTING_TIMEOUT, NULL, m_iCraftingAttempts );
				CleanupPostCraft( m_iCurrentlySelectedRecipe != RECIPE_CUSTOM );
				OpenCraftingStatusDialog( this, "#CraftUpdate_Failed", false, true, false );
				return;
			}
		}

		if ( m_bWaitingForCraftItems )
		{
			// If all the items in our newly crafted list are in the cache, we can show the pickup.
			FOR_EACH_VEC_BACK( m_vecNewlyCraftedItems, i )
			{
				CEconItemView* pNewItem = InventoryManager()->GetLocalInventory()->GetInventoryItemByItemID( m_vecNewlyCraftedItems[i] );
				if ( pNewItem == NULL )
					return;
				C_CTF_GameStats.Event_Crafting( IE_CRAFTING_SUCCESS, pNewItem, m_iCraftingAttempts );
			}

			m_bWaitingForCraftItems = false;

			// We have all the new items, show the pickup
			OpenCraftingStatusDialog( this, "#CraftUpdate_Success", false, true, true );
			CleanupPostCraft( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingPanel::CleanupPostCraft( bool bClearInputItems )
{
	m_flAbortCraftingAt = 0;
	m_bWaitingForCraftItems = false;

	UpdateSelectedRecipe( bClearInputItems );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar *CCraftingPanel::GetExplanationConVar( void )
{
	return &tf_explanations_craftingpanel;
}

//================================================================================================================================
// NOT CONNECTED TO STEAM WARNING DIALOG
//================================================================================================================================
static vgui::DHANDLE<CCraftingStatusDialog> g_CraftingStatusPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCraftingStatusDialog::CCraftingStatusDialog( vgui::Panel *pParent, const char *pElementName ) : BaseClass( pParent, "CraftingStatusDialog" )
{
	m_pRecipePanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "RecipeItemModelPanel" ) );
	m_bShowNewRecipe = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingStatusDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_bShowNewRecipe )
	{
		LoadControlSettings( "resource/UI/NewRecipeFoundDialog.res" );
	}
	else
	{
		LoadControlSettings( "resource/UI/CraftingStatusDialog.res" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingStatusDialog::OnCommand( const char *command )
{
	bool bClose = false;

	if ( !Q_stricmp( command, "close" ) )
	{
		// If we were a success, show the player their new crafted items
		if ( m_bShowOnExit )
		{
			if ( EconUI()->GetCraftingPanel() )
			{
				EconUI()->GetCraftingPanel()->ShowCraftFinish();
			}

			m_bShowOnExit = false;
		}

		bClose = true;
	}
	else if ( !Q_stricmp( command, "forceclose" ) )
	{
		bClose = true;
	}

	if ( bClose )
	{
		m_bShowOnExit = false;
		TFModalStack()->PopModal( this );
		SetVisible( false );
		MarkForDeletion();

		EconUI()->SetPreventClosure( false );
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingStatusDialog::OnTick( void )
{
	if ( !m_bAnimateEllipses || !IsVisible() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
	else
	{
		m_iNumEllipses = ((m_iNumEllipses+1) % 4);
	}

	switch ( m_iNumEllipses )
	{
		case 3: SetDialogVariable( "ellipses", L"..." ); break;
		case 2: SetDialogVariable( "ellipses", L".." ); break;
		case 1: SetDialogVariable( "ellipses", L"." ); break;
		default: SetDialogVariable( "ellipses", L"" ); break;
	}

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingStatusDialog::UpdateSchemeForVersion( bool bRecipe )
{
	m_bShowNewRecipe = bRecipe;
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingStatusDialog::ShowStatusUpdate( bool bAnimateEllipses, bool bAllowClose, bool bShowOnExit )
{
	m_bShowNewRecipe = false;

	CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
	if ( pButton )
	{
		pButton->SetVisible( bAllowClose );
		pButton->SetEnabled( bAllowClose );
	}

	m_bAnimateEllipses = bAnimateEllipses;
	if ( m_bAnimateEllipses )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 500 );
		SetDialogVariable( "ellipses", L"" );
		m_iNumEllipses = 0;
	}
	else
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		SetDialogVariable( "ellipses", L"" );
	}

	m_bShowOnExit = bShowOnExit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetupCraftingStatusDialog( vgui::Panel *pParent )
{
	if (!g_CraftingStatusPanel.Get())
	{
		g_CraftingStatusPanel = vgui::SETUP_PANEL( new CCraftingStatusDialog( pParent, NULL ) );
	}
	g_CraftingStatusPanel->SetVisible( true );
	g_CraftingStatusPanel->MakePopup();
	g_CraftingStatusPanel->MoveToFront();
	g_CraftingStatusPanel->SetKeyBoardInputEnabled(true);
	g_CraftingStatusPanel->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_CraftingStatusPanel );

	EconUI()->SetPreventClosure( true );
}

CCraftingStatusDialog *OpenCraftingStatusDialog( vgui::Panel *pParent, const char *pszText, bool bAnimateEllipses, bool bAllowClose, bool bShowOnExit )
{
	SetupCraftingStatusDialog( pParent );
	g_CraftingStatusPanel->UpdateSchemeForVersion( false );
	g_CraftingStatusPanel->SetDialogVariable( "updatetext", g_pVGuiLocalize->Find( pszText ) );
	g_CraftingStatusPanel->ShowStatusUpdate( bAnimateEllipses, bAllowClose, bShowOnExit );
	return g_CraftingStatusPanel;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CloseCraftingStatusDialog( void )
{
	if ( g_CraftingStatusPanel )
	{
		g_CraftingStatusPanel->OnCommand( "forceclose" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the craft response
//-----------------------------------------------------------------------------
class CGCCraftResponse : public GCSDK::CGCClientJob
{
public:
	CGCCraftResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		CUtlVector<uint64> vecCraftedIndices;
		uint16 iItems = 0;
		if ( !msg.BReadUint16Data( &iItems ) )
			return true;
		vecCraftedIndices.SetSize( iItems );
		for ( int i = 0; i < iItems; i++ )
		{
			if( !msg.BReadUint64Data( &vecCraftedIndices[i] ) )
				return true;
		}

		if ( EconUI()->GetCraftingPanel() )
		{
			EconUI()->GetCraftingPanel()->OnCraftResponse( (EGCMsgResponse)msg.Body().m_eResponse, &vecCraftedIndices, msg.Body().m_nResponseIndex );
		}

		//Msg("RECEIVED CGCCraftResponse: %d\n", msg.Body().m_eResponse );
		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCCraftResponse, "CGCCraftResponse", k_EMsgGCCraftResponse, GCSDK::k_EServerTypeGCClient );



//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the Golden Wrench broadcast message
//-----------------------------------------------------------------------------
class CGCGoldenWrenchBroadcast : public GCSDK::CGCClientJob
{
public:
	CGCGoldenWrenchBroadcast( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgTFGoldenWrenchBroadcast> msg( pNetPacket );

		// @todo Tom Bui: should we display this in some other manner?  This gets covered up by the crafting panel.
		CHudNotificationPanel *pNotifyPanel = GET_HUDELEMENT( CHudNotificationPanel );
		if ( pNotifyPanel )
		{
			bool bDeleted = msg.Body().deleted();
			wchar_t szPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			UTIL_GetFilteredPlayerNameAsWChar( CSteamID(), msg.Body().user_name().c_str(), szPlayerName );
			wchar_t szWrenchNumber[16]=L"";
			_snwprintf( szWrenchNumber, ARRAYSIZE( szWrenchNumber ), L"%i", msg.Body().wrench_number() );
			wchar_t szNotification[1024]=L"";
			g_pVGuiLocalize->ConstructString_safe( szNotification, 
											  g_pVGuiLocalize->Find( bDeleted ? "#TF_HUD_Event_GoldenWrench_D": "#TF_HUD_Event_GoldenWrench_C" ), 
											  2, szPlayerName, szWrenchNumber );
			pNotifyPanel->SetupNotifyCustom( szNotification, HUD_NOTIFY_GOLDEN_WRENCH, 10.0f );

			// echo to chat
			CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
			if ( pHUDChat )
			{
				char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( szNotification, szAnsi, sizeof(szAnsi) );

				pHUDChat->Printf( CHAT_FILTER_NONE, "%s", szAnsi );
			}

			// play a sound
			vgui::surface()->PlaySound( bDeleted ? "vo/announcer_failure.mp3" : "vo/announcer_success.mp3" );
		}

		//Msg("RECEIVED CGCCraftResponse: %d\n", msg.Body().m_eResponse );
		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCGoldenWrenchBroadcast, "CGCGoldenWrenchBroadcast", k_EMsgGCGoldenWrenchBroadcast, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the Saxxy broadcast message
//-----------------------------------------------------------------------------
class CGSaxxyBroadcast : public GCSDK::CGCClientJob
{
public:
	CGSaxxyBroadcast( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgTFSaxxyBroadcast> msg( pNetPacket );

		CEconNotification *pNotification = new CEconNotification();
		pNotification->SetText( "#TF_Event_Saxxy_Deleted" );
		pNotification->SetLifetime( 30.0f );

		{
			// Who deleted this?
			wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			UTIL_GetFilteredPlayerNameAsWChar( CSteamID(), msg.Body().has_user_name() ? msg.Body().user_name().c_str() : NULL, wszPlayerName );
			pNotification->AddStringToken( "owner", wszPlayerName );

			// What category was the Saxxy for?
			char szCategory[MAX_ATTRIBUTE_DESCRIPTION_LENGTH];
			Q_snprintf( szCategory, sizeof( szCategory ), "Replay_Contest_Category%d", msg.Body().category_number() );

			pNotification->AddStringToken( "category", g_pVGuiLocalize->Find( szCategory ) );
		}

		NotificationQueue_Add( pNotification );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGSaxxyBroadcast, "CGSaxxyBroadcast", k_EMsgGCSaxxyBroadcast, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive any generic item deletion notification
//-----------------------------------------------------------------------------
class CClientItemBroadcastNotificationJob : public GCSDK::CGCClientJob
{
public:
	CClientItemBroadcastNotificationJob( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCTFSpecificItemBroadcast> msg( pNetPacket );

		CEconNotification *pNotification = new CEconNotification();
		pNotification->SetText( msg.Body().was_destruction() ? "#TF_Event_Item_Deleted" : "#TF_Event_Item_Created" );
		pNotification->SetLifetime( 30.0f );

		// Who deleted this?
		wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
		UTIL_GetFilteredPlayerNameAsWChar( CSteamID(), msg.Body().has_user_name() ? msg.Body().user_name().c_str() : NULL, wszPlayerName );
		pNotification->AddStringToken( "owner", wszPlayerName );

		// What type of item was this?
		const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( msg.Body().item_def_index() );
		if ( pItemDef )
		{
			pNotification->AddStringToken( "item_name", g_pVGuiLocalize->Find( pItemDef->GetItemBaseName() ) );

			NotificationQueue_Add( pNotification );
		}

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CClientItemBroadcastNotificationJob, "CClientItemBroadcastNotificationJob", k_EMsgGCTFSpecificItemBroadcast, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the Saxxy Awarded broadcast message
//-----------------------------------------------------------------------------
class CGSaxxyAwardedBroadcast : public GCSDK::CGCClientJob
{
private:
	// embedded notification for custom trigger
	class CSaxxyAwardedNotification : public CEconNotification
	{
	public:
		CSaxxyAwardedNotification()
		{
			SetSoundFilename( "vo/announcer_success.mp3" );
		}

		virtual EType NotificationType() { return eType_Trigger; }

		virtual void Trigger()
		{
			if ( steamapicontext && steamapicontext->SteamFriends() )
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/saxxyawards/winners.php" );
			}
			MarkForDeletion();
		}
	};

public:

	CGSaxxyAwardedBroadcast( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgSaxxyAwarded > msg( pNetPacket );

		CEconNotification *pNotification = new CSaxxyAwardedNotification();
		pNotification->SetText( "#TF_Event_Saxxy_Awarded" );
		pNotification->SetLifetime( 30.0f );

		{
			// Winners
			CFmtStr1024 strWinners;
			for ( int i = 0; i < msg.Body().winner_names_size(); ++i )
			{
				char szPlayerName[ MAX_PLAYER_NAME_LENGTH ];
				V_strcpy_safe( szPlayerName, msg.Body().winner_names( i ).c_str() );
				UTIL_GetFilteredPlayerName( CSteamID(), szPlayerName );
				strWinners.Append( szPlayerName );
				if ( i + 1 < msg.Body().winner_names_size() )
				{
					strWinners.Append( "\n" );
				}
			}
			wchar_t wszPlayerNames[ 1024 ];
			g_pVGuiLocalize->ConvertANSIToUnicode( strWinners.Access(), wszPlayerNames, sizeof( wszPlayerNames ) );
			pNotification->AddStringToken( "winners", wszPlayerNames );

			// year
			CRTime cTime;
			cTime.SetToCurrentTime();
			cTime.SetToGMT( false );
			locchar_t wszYear[10];
			loc_sprintf_safe( wszYear, LOCCHAR( "%04u" ), cTime.GetYear() );
			pNotification->AddStringToken( "year", wszYear );

			// What category was the Saxxy for?
			char szCategory[MAX_ATTRIBUTE_DESCRIPTION_LENGTH];
			Q_snprintf( szCategory, sizeof( szCategory ), "Replay_Contest_Category%d", msg.Body().category() );
			pNotification->AddStringToken( "category", g_pVGuiLocalize->Find( szCategory ) );
		}

		NotificationQueue_Add( pNotification );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGSaxxyAwardedBroadcast, "CGSaxxyAwardedBroadcast", k_EMsgGCSaxxy_Awarded, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive a generic system broadcast message
//-----------------------------------------------------------------------------
class CGCSystemMessageBroadcast : public GCSDK::CGCClientJob
{
public:
	CGCSystemMessageBroadcast( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
		if ( !pHUDChat )
			return false;

		GCSDK::CProtoBufMsg<CMsgSystemBroadcast> msg( pNetPacket );

		// retrieve the text
		const char *pchMessage = msg.Body().message().c_str();
		wchar_t *pwMessage = g_pVGuiLocalize->Find( pchMessage );
		wchar_t wszConvertedText[2048] = L"";
		if ( pwMessage == NULL )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pchMessage, wszConvertedText, sizeof( wszConvertedText ) );
			pwMessage = wszConvertedText;
		}

		Color color( 0xff, 0xcc, 0x33, 255 );
		KeyValuesAD keyValues( "System Message" );
		keyValues->SetWString( "message", pwMessage );
		keyValues->SetColor( "custom_color", color );

		// print to chat log
		wchar_t wszLocalizedString[2048] = L"";
		g_pVGuiLocalize->ConstructString_safe( wszLocalizedString, "#Notification_System_Message", keyValues );
		pHUDChat->SetCustomColor( color );
		pHUDChat->Printf( CHAT_FILTER_NONE, "%ls", wszLocalizedString );

		// send to notification
		CEconNotification* pNotification = new CEconNotification();
		pNotification->SetText( "#Notification_System_Message" );
		pNotification->SetKeyValues( keyValues );
		pNotification->SetLifetime( 30.0f );
		pNotification->SetSoundFilename( "ui/system_message_alert.wav" );
		NotificationQueue_Add( pNotification );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCSystemMessageBroadcast, "CGCSystemMessageBroadcast", k_EMsgGCSystemMessage, GCSDK::k_EServerTypeGCClient );
