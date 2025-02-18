//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "crafting_panel.h"
#include "dynamic_recipe_subpanel.h"
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
#include "econ_dynamic_recipe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static CDynamicRecipePanel* g_DynamicRecipePanel = NULL;
extern const char *g_szItemBorders[AE_MAX_TYPES][5];

//-----------------------------------------------------------------------------
// Purpose: Default to NULL item
//-----------------------------------------------------------------------------
CRecipeComponentItemModelPanel::CRecipeComponentItemModelPanel( vgui::Panel *parent, const char *name )
	: CItemModelPanel( parent, name )
	, m_nPageNumber( 0 )
{
	SetItem( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Add recipe to our list of recipes.  Each call to this function
//			effectively adds an item to the next page
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::AddRecipe( itemid_t nRecipe )
{
	RecipeItem_t& recipeItem = m_vecRecipes[m_vecRecipes.AddToTail()];
	recipeItem.m_nRecipeIndex = nRecipe;
	UpdateRecipeItem( &recipeItem );
}


//-----------------------------------------------------------------------------
// Purpose: Wipe all recipes from all pages
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::DeleteRecipes()
{
	m_vecDefaultItems.Purge();
	m_vecRecipes.Purge();
	SetItem( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Override to set the item to be our default item if NULL is passed in.
//			Also handles greying out the panels
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::SetItem( const CEconItemView *pItem )
{
	// Use the default item if they set NULL
	if( pItem == NULL )
	{
		SetBlankState();
	}
	else
	{
		BaseClass::SetItem( pItem );
		SetGreyedOut( NULL );
	}

	InvalidateLayout( true );
}


void CRecipeComponentItemModelPanel::SetBlankState()
{
	CEconItemView* pDefaultItem = NULL;
	if( m_nPageNumber < m_vecDefaultItems.Count() )
		pDefaultItem = m_vecDefaultItems[ m_nPageNumber ];
	BaseClass::SetItem( pDefaultItem );
	SetGreyedOut( "" );
}

//-----------------------------------------------------------------------------
// Purpose: Move a recipe item to a specific page
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::SetRecipeItem( itemid_t nRecipeItem, int nPageNumber )
{
	Assert( nPageNumber < m_vecRecipes.Count() );
	m_vecRecipes[ nPageNumber ].m_nRecipeIndex = nRecipeItem;

	UpdateRecipeItem( &m_vecRecipes[ nPageNumber ] );

	// Use the item that this item ID maps to
	SetItem( m_vecRecipes[ nPageNumber ].m_pRecipeItem  );
}


//-----------------------------------------------------------------------------
// Purpose: Add a default item to a page
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::AddDefaultItem( CEconItemView *pItem )
{
	m_vecDefaultItems[ m_vecDefaultItems.AddToTail() ] = pItem;
}


//-----------------------------------------------------------------------------
// Purpose: Get a recipe item on a given page
//-----------------------------------------------------------------------------
CEconItemView* CRecipeComponentItemModelPanel::GetRecipeItem( int nPageNumber ) const
{
	if( nPageNumber < m_vecRecipes.Count() )
	{
		return m_vecRecipes[nPageNumber].m_pRecipeItem;
	}
		
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Get the recipe index from a specific page
//-----------------------------------------------------------------------------
itemid_t CRecipeComponentItemModelPanel::GetRecipeIndex( int nPageNumber ) const
{
	if( nPageNumber < m_vecRecipes.Count() )
	{
		return m_vecRecipes[nPageNumber].m_nRecipeIndex;
	}
		
	return INVALID_ITEM_ID;
}



//-----------------------------------------------------------------------------
// Purpose: Iterate through all attributes on a item and turn recipe attributes
//			into input and output items.  Store those items in vectors for inputs
//			and outputs.  Inputs are sorted from least common to most common
//			so that the later pages are filled with more repeats than the early pages
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::CRecipeComponentAttributeCounter::OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value )
{
	static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
	Assert( pAttrib_CannotTrade );

	unsigned nCount = value.num_required() - value.num_fulfilled();

	if( value.component_flags() & DYNAMIC_RECIPE_FLAG_IS_OUTPUT )
	{
		CEconItem* pItem = m_vecTempEconItems[m_vecTempEconItems.AddToTail( new CEconItem() )];
		DecodeItemFromEncodedAttributeString( value, pItem );
		
		for( unsigned i=0; i < nCount; ++i )
		{
			CEconItemView& item = m_vecOutputItems[ m_vecOutputItems.AddToTail() ];
			
			item.SetItemDefIndex( pItem->GetDefinitionIndex() );
			item.SetItemQuality( pItem->GetQuality() );
			item.SetItemLevel( pItem->GetItemLevel() );
			item.SetItemID( pItem->GetItemID() );
			item.SetNonSOEconItem( pItem );	// Set the item into the econ item view.
			item.SetInitialized( true );
			item.SetItemOriginOverride( kEconItemOrigin_RecipeOutput ); // Spoof where we came from
				
			// Set the untradable flag if the attribute says so
			if( value.component_flags() & DYNAMIC_RECIPE_FLAG_IS_UNTRADABLE )
			{
				item.GetAttributeList()->SetRuntimeAttributeValue( pAttrib_CannotTrade, 0.0f );			// value doesn't matter -- we only check for presence/absence
			}
		}
	}
	else
	{
		m_nInputCount += nCount;

		CEconItem* pItem = m_vecTempEconItems[m_vecTempEconItems.AddToTail( new CEconItem() )];
		DecodeItemFromEncodedAttributeString( value, pItem );

		CUtlVector<InputComponent_t>& inputSeries = m_vecInputItems[ m_vecInputItems.AddToTail() ];
		for( unsigned i=0; i < nCount; ++i )
		{
			InputComponent_t& item = inputSeries[ inputSeries.AddToTail() ];
			item.m_ItemView.SetItemDefIndex( pItem->GetDefinitionIndex() );
			item.m_ItemView.SetItemQuality( pItem->GetQuality() );
			item.m_ItemView.SetItemLevel( 0 );
			item.m_ItemView.SetItemID( pItem->GetItemID() );
			item.m_ItemView.SetNonSOEconItem( pItem );	// Set the item into the econ item view.
			item.m_ItemView.SetInitialized( true );
			item.m_ItemView.SetItemOriginOverride( kEconItemOrigin_RecipeOutput ); // Spoof where we came from
			item.m_pAttrib = pAttrDef;
		}

		m_vecInputItems.Sort( LeastCommonInputSortFunc );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get the output item on a given page
//-----------------------------------------------------------------------------
CEconItemView* CDynamicRecipePanel::CRecipeComponentAttributeCounter::GetOutputItem( int i )
{
	if( i >= 0 && i < m_vecOutputItems.Count() )
		return &m_vecOutputItems[i];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the input item on a given page
//-----------------------------------------------------------------------------
CEconItemView* CDynamicRecipePanel::CRecipeComponentAttributeCounter::GetInputItem( int i ) 
{ 
	InputComponent_t* pInputComponent = GetInputComponent( i );
	if( pInputComponent )
	{
		return &pInputComponent->m_ItemView;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the attribute that the item in a given panel maps to
//-----------------------------------------------------------------------------
const CEconItemAttributeDefinition* CDynamicRecipePanel::CRecipeComponentAttributeCounter::GetInputAttrib( int i )
{
	InputComponent_t* pInputComponent = GetInputComponent( i );
	if( pInputComponent )
	{
		return pInputComponent->m_pAttrib;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sort input vectors based on count.  Fewer first.
//-----------------------------------------------------------------------------
int CDynamicRecipePanel::CRecipeComponentAttributeCounter::LeastCommonInputSortFunc( const CCopyableUtlVector<InputComponent_t> *p1, const CCopyableUtlVector<InputComponent_t> *p2 )
{
	return p1->Count() > p2->Count();
}


//-----------------------------------------------------------------------------
// Purpose: Find input component i from our 2D vector of input items
//-----------------------------------------------------------------------------
CDynamicRecipePanel::CRecipeComponentAttributeCounter::InputComponent_t* CDynamicRecipePanel::CRecipeComponentAttributeCounter::GetInputComponent( int i )
{
	int nAccum = 0;
	FOR_EACH_VEC( m_vecInputItems, nIndex )
	{
		int nCount = m_vecInputItems[ nIndex ].Count();

		if( i < nAccum + nCount )
		{
			return &m_vecInputItems[ nIndex ][ i - nAccum ];
		}

		nAccum += nCount;
	}

	return NULL;	
}

//-----------------------------------------------------------------------------
// Purpose: Reset all data in the iterator
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::CRecipeComponentAttributeCounter::Reset()
{ 
	m_vecInputItems.Purge();
	m_vecOutputItems.Purge();
	m_vecTempEconItems.PurgeAndDeleteElements();
	m_nInputCount = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Compare Check if m_pItemToMatch passes the criteria of any of the
//			attributes on m_pSourceItem.
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::CDynamicRecipeItemMatchFind::OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value )
{
	// Can't match ourself
	if( m_pSourceItem && m_pItemToMatch && m_pSourceItem->GetID() == m_pItemToMatch->GetID() )
		return true;

	if( value.component_flags() & DYNAMIC_RECIPE_FLAG_IS_OUTPUT )
		return true;

	if( !DefinedItemAttribMatch( value, m_pItemToMatch ) )
		return true;

	// Must be useable in crafting.  Expensive -- do this last.
	if( !m_pItemToMatch || !m_pItemToMatch->IsUsableInCrafting() )
		return true;

	// A match!
	m_bMatchesAny = true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Delete all recipe data
//-----------------------------------------------------------------------------
void CInputPanelItemModelPanel::DeleteRecipes()
{
	CRecipeComponentItemModelPanel::DeleteRecipes();
	m_vecAttrDef.Purge();
}


//-----------------------------------------------------------------------------
// Purpose: Add component info to a new page
//-----------------------------------------------------------------------------
void CInputPanelItemModelPanel::AddComponentInfo( const CEconItemAttributeDefinition *pComponentAttrib )
{
	m_vecAttrDef[ m_vecAttrDef.AddToTail() ] = pComponentAttrib;
	AddRecipe( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Check if a passed in database item matches the desired item in this
//			item panel.  Default to the current page
//-----------------------------------------------------------------------------
bool CInputPanelItemModelPanel::MatchesAttribCriteria( itemid_t itemID ) const
{
	return MatchesAttribCriteria( itemID, m_nPageNumber );
}


//-----------------------------------------------------------------------------
// Purpose: Check if a passed in database item matches the desired item in this
//			item panel on the specified page.
//-----------------------------------------------------------------------------
bool CInputPanelItemModelPanel::MatchesAttribCriteria( itemid_t itemID, int nPageNumber ) const
{
	if( !m_pDynamicRecipeItem )
		return false;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
		return false;

	const CEconItemView* pItem = pLocalInv->GetInventoryItemByItemID( itemID );

	if( !pItem || !pItem->IsUsableInCrafting() )
		return false;

	const CEconItemAttributeDefinition* pAttrDef = GetAttrib( nPageNumber );
	CAttribute_DynamicRecipeComponent attribValue;
	if( m_pDynamicRecipeItem->FindAttribute<CAttribute_DynamicRecipeComponent >( pAttrDef, &attribValue ) )
	{
		return DefinedItemAttribMatch( attribValue, pItem );
	}	

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Get the attribute that this panel represents
//-----------------------------------------------------------------------------
const CEconItemAttributeDefinition* CInputPanelItemModelPanel::GetAttrib( int nPageNumber ) const
{
	if( nPageNumber >= 0 && nPageNumber < m_vecAttrDef.Count() )
		return m_vecAttrDef[ nPageNumber ];

	return NULL;
}


void CInputPanelItemModelPanel::SetBlankState()
{
	// Get the default item
	CEconItemView* pDefaultItem = NULL;
	if( m_nPageNumber < m_vecDefaultItems.Count() )
		pDefaultItem = m_vecDefaultItems[ m_nPageNumber ];
		
	// Grey out
	SetGreyedOut( "" );

	// Check for the "item name text override" attribute on the default item
	static CSchemaAttributeDefHandle pAttrDef_ItemNameTextOverride( "item name text override" );
	CAttribute_String attrItemNameTextOverride;
	if ( pDefaultItem )
	{
		pDefaultItem->FindAttribute( pAttrDef_ItemNameTextOverride, &attrItemNameTextOverride );

		if ( FStrEq( attrItemNameTextOverride.value().c_str(), "#TF_ItemName_Item" ) )
		{
			// This is a dummy item.  Dont display an icon.  Just the name of the item
			CItemModelPanel::SetItem( NULL );
			SetAttribOnly( true );
			SetTextYPos( 0 );
			//SetNoItemText( pDefaultItem->GetItemName(), NULL, NULL );
			const wchar_t *pszItemname = pDefaultItem->GetItemName();
			if ( V_wcscmp( pszItemname, g_pVGuiLocalize->Find( "#TF_ItemName_Item" ) ) == 0 && pDefaultItem->GetQuality() == AE_PAINTKITWEAPON )
			{
				SetNoItemText( g_pVGuiLocalize->Find( "paintkitweapon" ), NULL, NULL );
			}
			else
			{
				SetNoItemText( pDefaultItem->GetItemName(), NULL, NULL );
			}
		}
		else
		{
			BaseClass::SetItem( pDefaultItem );
		}
	}
	else
	{
		// Construct an item from the attribute that describes this input
		CEconItem tempItem;
		const CEconItemAttributeDefinition* pAttrDef = GetAttrib( m_nPageNumber );
		CAttribute_DynamicRecipeComponent attribValue;
		if( m_pDynamicRecipeItem->FindAttribute<CAttribute_DynamicRecipeComponent >( pAttrDef, &attribValue ) )
		{
			DecodeItemFromEncodedAttributeString( attribValue, &tempItem );
		}

		// Shove it into an econitemview
		CEconItemView tempView;
		tempView.Init( tempItem.GetItemDefIndex(), tempItem.GetQuality(), 0 );
		tempView.SetNonSOEconItem( &tempItem );

		// Set its name as the text for this item model panel
		CItemModelPanel::SetItem( NULL );
		SetAttribOnly( true );
		SetTextYPos( 0 );
		SetNoItemText( tempView.GetItemName(), NULL, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is this in play
//-----------------------------------------------------------------------------
bool CRecipeComponentItemModelPanel::IsSlotAvailable( int nPageNumber )
{
	return nPageNumber < m_vecRecipes.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the passed in recipe item and changes the item we show
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::UpdateRecipeItem( RecipeItem_t* pRecipeItem )
{
	Assert( pRecipeItem );

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv == NULL )
		return;

	pRecipeItem->m_pRecipeItem = pLocalInv->GetInventoryItemByItemID( pRecipeItem->m_nRecipeIndex );
	SetItem( pRecipeItem->m_pRecipeItem );
}

//-----------------------------------------------------------------------------
// Purpose: Update the item we show for the current page
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::UpdateDisplayItem()
{
	if( m_nPageNumber < m_vecRecipes.Count() )
	{
		UpdateRecipeItem( &m_vecRecipes[ m_nPageNumber ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the current page
//-----------------------------------------------------------------------------
void CRecipeComponentItemModelPanel::SetPageNumber( int nPageNumber )
{
	Assert( nPageNumber >= 0 );
	m_nPageNumber = nPageNumber;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDynamicRecipePanel::CDynamicRecipePanel( vgui::Panel *parent, const char *panelName, CEconItemView* pRecipeItem ) 
	: CBackpackPanel( parent, panelName )
	, m_pDynamicRecipeItem( pRecipeItem )
	, m_pRecipeCraftButton( NULL )
	, m_nNumRecipeItems( 0 )
	, m_bAllRecipePanelsFilled( false )
	, m_bInputPanelsDirty( false )
	, m_nInputPage( 0 )
	, m_nOutputPage( 0 )
	, m_pCurInputPageLabel( NULL )
	, m_pNextInputPageButton( NULL )
	, m_pPrevInputPageButton( NULL )
	, m_flAbortCraftingAt( 0 )
	, m_pMouseOverItemPanel( NULL )
	, m_pNoMatchesLabel( NULL )
	, m_pUntradableOutputsLabel( NULL )
	, m_bShowUntradable( false )

{
	g_DynamicRecipePanel = this;

	m_pRecipeContainer = new vgui::EditablePanel( this, "recipecontainer" );
	m_pInventoryContainer = new vgui::EditablePanel( this, "inventorycontainer" );
	m_pShowUntradableItemsCheckbox = new vgui::CheckButton( m_pInventoryContainer, "untradablecheckbox", "#Dynamic_Recipe_Untradable_Checkbox" );
	m_pShowUntradableItemsCheckbox->AddActionSignalTarget( this );
	m_pInputsLabel = new CExLabel( m_pRecipeContainer, "InputLabel", "#Craft_Recipe_Inputs" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDynamicRecipePanel::~CDynamicRecipePanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get all our controls
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( GetResFile() );

	// This calls AddNewItemPanel
	BaseClass::ApplySchemeSettings( pScheme );

	CExButton* pCancelButton = dynamic_cast<CExButton*>( m_pInventoryContainer->FindChildByName("CancelButton") );
	if ( pCancelButton )
		pCancelButton->AddActionSignalTarget( this );

	m_pRecipeCraftButton = dynamic_cast<CExButton*>( m_pRecipeContainer->FindChildByName("CraftButton") );
	if ( m_pRecipeCraftButton )
		m_pRecipeCraftButton->AddActionSignalTarget( this );

	m_pNextPageButton = dynamic_cast<CExButton*>( m_pInventoryContainer->FindChildByName("NextPageButton") );
	if( m_pNextPageButton )
		m_pNextPageButton->AddActionSignalTarget( this );

	m_pPrevPageButton = dynamic_cast<CExButton*>( m_pInventoryContainer->FindChildByName("PrevPageButton") );
	if( m_pPrevPageButton )
		m_pPrevPageButton->AddActionSignalTarget( this );

	m_pCurPageLabel = dynamic_cast<vgui::Label*>( m_pInventoryContainer->FindChildByName("CurPageLabel") );
	Assert( m_pCurPageLabel );

	m_pNoMatchesLabel = dynamic_cast<CExLabel*>( m_pInventoryContainer->FindChildByName( "NoMatches" ) );
	Assert( m_pNoMatchesLabel );

	m_pUntradableOutputsLabel = dynamic_cast<CExLabel*>( m_pRecipeContainer->FindChildByName( "UntradableLabel" ) );
	Assert( m_pUntradableOutputsLabel );

	m_pOutputsLabel = dynamic_cast<CExLabel*>( m_pRecipeContainer->FindChildByName( "OutputLabel" ) );
	Assert( m_pOutputsLabel );

	m_pCurInputPageLabel = dynamic_cast<CExLabel*>( m_pRecipeContainer->FindChildByName( "CurInputPageLabel" ) );

	m_pNextInputPageButton = dynamic_cast<CExButton*>( m_pRecipeContainer->FindChildByName( "NextInputPageButton" ) );
	if( m_pNextInputPageButton )
		m_pNextInputPageButton->AddActionSignalTarget( this );

	m_pPrevInputPageButton = dynamic_cast<CExButton*>( m_pRecipeContainer->FindChildByName( "PrevInputPageButton" ) ); 
	if( m_pPrevInputPageButton )
		m_pPrevInputPageButton->AddActionSignalTarget( this );


	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: Update all the item panels and page buttons and labels
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();


	if( m_pSortByComboBox )
	{
		m_pSortByComboBox->SetVisible( false );
	}

	UpdateModelPanels();

	bool bNoMatches = m_nNumRecipeItems == 0;
	bool bMultiplePagesOfMatches = m_nNumRecipeItems > (unsigned)GetNumBackpackPanelsPerPage();
	// Some panels show and hide based on the number of matches
	if( m_pNoMatchesLabel)
		m_pNoMatchesLabel->SetVisible( bNoMatches );
	if( m_pNextPageButton ) 
		m_pNextPageButton->SetVisible( bMultiplePagesOfMatches );
	if( m_pPrevPageButton )
		m_pPrevPageButton->SetVisible( bMultiplePagesOfMatches );
	if( m_pCurPageLabel )
		m_pCurPageLabel->SetVisible( bMultiplePagesOfMatches );

	bool bMultiplePagesOfInputs = m_RecipeIterator.GetInputCount() > GetNumInputPanelsPerPage();
	if( m_pNextInputPageButton )
		m_pNextInputPageButton->SetVisible( bMultiplePagesOfInputs );
	if( m_pNextInputPageButton )
		m_pNextInputPageButton->SetEnabled( m_nInputPage < GetNumInputPages() - 1 );
	if( m_pCurInputPageLabel )
		m_pCurInputPageLabel->SetVisible( bMultiplePagesOfInputs );
	if( m_pPrevInputPageButton )
		m_pPrevInputPageButton->SetVisible( bMultiplePagesOfInputs );
	if( m_pPrevInputPageButton )
		m_pPrevInputPageButton->SetEnabled( m_nInputPage > 0 );

	if( m_pUntradableOutputsLabel )
		m_pUntradableOutputsLabel->SetVisible( m_pDynamicRecipeItem ? !m_pDynamicRecipeItem->IsTradable() : false );
}


//-----------------------------------------------------------------------------
// Purpose: Handle commands
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "back", 4 ) )
	{
		PostMessage( GetParent(), new KeyValues("CraftingClosed") );
		return;
	}
	else if ( !Q_strnicmp( command, "craft", 5 ) )
	{
		// Check if we should warn about partial completion
		if( WarnAboutPartialCompletion() )
		{
			if( CheckForUntradableItems() )
			{
				Craft();
			}
		}
	
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( true, true );	// deliberatly fallthrough to baseclass
	}
	else if( !Q_stricmp( command, "cancel" ) )
	{
		SetVisible( false );
		return;
	}
	else if ( !Q_strnicmp( command, "nextpage", 8 ) )
	{
		InvalidateLayout(); // deliberatly fallthrough to baseclass
	}
	else if ( !Q_strnicmp( command, "prevpage", 8 ) )
	{
		InvalidateLayout(); // deliberatly fallthrough to baseclass
	}
	else if( !Q_strnicmp( command, "nextinputpage", 13 ) )
	{
		if( m_nInputPage < GetNumInputPages() )
			m_nInputPage++;
		InvalidateLayout();
		return;
	}
	else if( !Q_strnicmp( command, "previnputpage", 13 ) )
	{
		if( m_nInputPage > 0 )
			m_nInputPage--;
		InvalidateLayout();
		return;
	}
	else if( !Q_strnicmp( command, "deleteitem", 10 ) ||
			 !Q_strnicmp( command, "useitem", 7 ) )
	{
		// Gobble up these commands
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: Gobble up all keyboard input for now!
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnKeyCodePressed( vgui::KeyCode /*code*/ )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnButtonChecked( KeyValues *pData )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( pData->GetPtr("panel") );
	
	if ( m_pShowUntradableItemsCheckbox == pPanel )
	{
		if ( m_bShowUntradable != m_pShowUntradableItemsCheckbox->IsSelected() )
		{
			m_bShowUntradable = m_pShowUntradableItemsCheckbox->IsSelected();
			InitItemPanels();
			UpdateModelPanels();
		}
	}
}


int	CDynamicRecipePanel::GetNumItemPanels( void ) 
{
	return DYNAMIC_RECIPE_INPUT_COUNT + DYNAMIC_RECIPE_OUTPUT_COUNT + DYNAMIC_RECIPE_PACKPACK_COUNT_PER_PAGE;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see that each and every input panel has a recipe item in it
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::AllRecipePanelsFilled( void )
{
	// Need to recalculate
	if( m_bInputPanelsDirty )
	{
		// Assume all filled, and go through and try to find one that's empty
		m_bAllRecipePanelsFilled = true;
		FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
		{
			CInputPanelItemModelPanel* pInputPanel = m_vecRecipeInputModelPanels[i];
			for( int j=0; j < GetNumInputPages(); ++j )
			{
				if( pInputPanel->GetRecipeItem( j ) == NULL && pInputPanel->GetAttrib( j ) != NULL )
				{
					m_bAllRecipePanelsFilled = false;
					break;
				}
			}
		}
	}

	return m_bAllRecipePanelsFilled;
}


//-----------------------------------------------------------------------------
// Purpose: Callback for the confirm partial completion dialog
//-----------------------------------------------------------------------------
void ConfirmDestroyItems( bool bConfirmed, void* pContext )
{
	CDynamicRecipePanel *pRecipePanel = ( CDynamicRecipePanel* )pContext;
	if ( pRecipePanel && bConfirmed )
	{
		if( pRecipePanel->CheckForUntradableItems() )
		{
			pRecipePanel->Craft();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Callback for the conrim untradable dialog
//-----------------------------------------------------------------------------
static void ConfirmUntradableCraft( bool bConfirmed, void* pContext )
{
	CDynamicRecipePanel *pRecipePanel = ( CDynamicRecipePanel* )pContext;
	if ( pRecipePanel && bConfirmed )
	{
		pRecipePanel->Craft();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Go through each input panel and find out if any of them contain an
//			item that is untradeable
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::CheckForUntradableItems( void )
{
	// We dont care if the recipe itself is already not tradable
	if( !m_pDynamicRecipeItem->IsTradable() )
	{
		return true;
	}

	bool bHasUntradable = false;
	for ( int i = 0; i < CRAFTING_SLOTS_INPUTPANELS; ++i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[i];
		for( int j = 0; j < GetNumInputPages(); ++j )
		{
			itemid_t nRecipeItemID = pPanel->GetRecipeIndex( j );

			if ( nRecipeItemID != 0 && nRecipeItemID != INVALID_ITEM_ID  )
			{
				CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( nRecipeItemID );
				if ( pItemData->IsTradable() == false )
				{
					bHasUntradable = true;
					break;
				}
			}
		}
	}

	if ( bHasUntradable )
	{
		enum { kWarningLength = 512 };
		locchar_t wszWarning[ kWarningLength ] = LOCCHAR("");

		loc_scpy_safe( wszWarning,
			CConstructLocalizedString( GLocalizationProvider()->Find( "Dynamic_Recipe_Untradable_Text" ),
									   m_pDynamicRecipeItem->GetItemName() ) );

		CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#Craft_Untradable_Title", wszWarning, "#GameUI_OK", "#Cancel", &ConfirmUntradableCraft, NULL );
		
		if ( pDialog )
		{
			pDialog->SetContext( this );
			pDialog->Show();
		}

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Check if not all of the inputs have items set into them.  Put up
//			a prompt and return false if so.
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::WarnAboutPartialCompletion( void )
{
	if( !AllRecipePanelsFilled() )
	{
		enum { kWarningLength = 512 };
		locchar_t wszWarning[ kWarningLength ] = LOCCHAR("");

		loc_scpy_safe( wszWarning,
			CConstructLocalizedString( GLocalizationProvider()->Find( "Dynamic_Recipe_Partial_Completion_Warning" ),
									   m_pDynamicRecipeItem->GetItemName() ) );

		CTFGenericConfirmDialog *pDialog = new CTFGenericConfirmDialog( "#Craft_Untradable_Title", wszWarning, "#GameUI_OK", "#Cancel", &ConfirmDestroyItems, NULL );
		if ( pDialog )
		{
			pDialog->SetContext( this );
			pDialog->Show();
		}


		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: They hit the craft button!  Cook up a message that we're going to send
//			to the GC that contains all of the item_ids and attributes they are
//			to apply to.  Open a crafting status dialog when after we send the message.
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::Craft()
{
	GCSDK::CProtoBufMsg<CMsgFulfillDynamicRecipeComponent> msg( k_EMsgGCFulfillDynamicRecipeComponent );
	msg.Body().set_tool_item_id( m_pDynamicRecipeItem->GetItemID() );

	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[i];

		for( int j=0; j < GetNumInputPages(); ++j )
		{
			itemid_t nRecipeItemID = pPanel->GetRecipeIndex( j );

			if( nRecipeItemID != 0 && nRecipeItemID != INVALID_ITEM_ID )
			{
				if( !pPanel->MatchesAttribCriteria( nRecipeItemID, j ) )
				{
					AssertMsg( 0, "Input panel has recipe item that does not pass its criteria" );
					// Something bad happened.  Return this item to the backpack.
					ReturnRecipeItemToBackpack( nRecipeItemID, pPanel, j );
					continue;
				}

				// Add component
				CMsgRecipeComponent* pComponent = msg.Body().add_consumption_components();
				pComponent->set_subject_item_id( nRecipeItemID );

				const CEconItemAttributeDefinition* pAttribute = pPanel->GetAttrib( j );
				if( !pAttribute )
				{
					AssertMsg( 0, "NULL attribute in panel what attempting to craft" );
					// Something bad happened.  Return this item to the backpack.
					ReturnRecipeItemToBackpack( nRecipeItemID, pPanel, j );
					continue;
				}
				pComponent->set_attribute_index( pAttribute->GetDefinitionIndex() );
			}
		}
	}

	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pDynamicRecipeItem, "consumed_item" );
	GCClientSystem()->BSendMessage( msg );

	// Open a craft status window to take focus away and let the user know something is happening
	OpenCraftingStatusDialog( this, "#CraftUpdate_Start", true, false, false );
	m_flAbortCraftingAt = vgui::system()->GetCurrentTime() + 10;
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: Think when we're waiting for a craft response
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnTick( void )
{
	BaseClass::OnTick();

	if( IsVisible() )
	{
		if( m_flAbortCraftingAt != 0.f )
		{
			// Timeout for crafting.  Let them know we failed.
			if( m_flAbortCraftingAt < vgui::system()->GetCurrentTime() )
			{
				OpenCraftingStatusDialog( this, "#CraftUpdate_Failed", false, true, false );
				m_flAbortCraftingAt = 0;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Close the craft status window if we close
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnShowPanel( bool bVisible, bool bReturningFromArmory )
{
	if ( !bVisible )
	{
		CloseCraftingStatusDialog();
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	BaseClass::OnShowPanel( bVisible, bReturningFromArmory );
}


//-----------------------------------------------------------------------------
// Purpose: Add the new panels into vectors for each group, and set parents to
//			appropriate frames
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::AddNewItemPanel( int iPanelIndex )
{
	if( IsInputPanel( iPanelIndex ) )
	{
		// Input item
		int nIndex = m_vecRecipeInputModelPanels.AddToTail();
		CInputPanelItemModelPanel* pPanel = vgui::SETUP_PANEL( new CInputPanelItemModelPanel( this, VarArgs("modelpanel%d", iPanelIndex), m_pDynamicRecipeItem ));
		m_pItemModelPanels.AddToTail( pPanel );
		pPanel->SetParent( m_pRecipeContainer );
		m_vecRecipeInputModelPanels[nIndex] = pPanel;
	}
	else if( IsOutputPanel( iPanelIndex ) )
	{
		// Output item
		CItemModelPanel* pPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, VarArgs("modelpanel%d", iPanelIndex) ) );
		m_vecRecipeOutputModelPanels.AddToTail( pPanel );
		m_pItemModelPanels.AddToTail( pPanel );
		pPanel->SetParent( m_pRecipeContainer );
	}
	else
	{
		// Inventory item
		int nIndex = m_vecBackpackModelPanels.AddToTail();
		CRecipeComponentItemModelPanel* pPanel = vgui::SETUP_PANEL( new CRecipeComponentItemModelPanel( this, VarArgs("modelpanel%d", iPanelIndex) ) );
		m_vecBackpackModelPanels[nIndex] = pPanel;
		m_pItemModelPanels.AddToTail( pPanel );
		pPanel->SetParent( m_pInventoryContainer );
	}

	CItemModelPanel* pPanel = m_pItemModelPanels.Tail();
	pPanel->SetActAsButton( true, true );
	pPanel->SetTooltip( m_pMouseOverTooltip, "" );
	pPanel->SetShowGreyedOutTooltip( true );
	pPanel->SetShowEquipped( true );

	// Store a position for our new panel
	m_ItemModelPanelPos.AddToTail();
	m_ItemModelPanelPos[iPanelIndex].x = m_ItemModelPanelPos[iPanelIndex].y = 0;

	Assert( iPanelIndex == (m_pItemModelPanels.Count()-1) );
}

//-----------------------------------------------------------------------------
// Purpose: Is this index an input panel
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::IsInputPanel( int iPanelIndex ) const
{ 
	return iPanelIndex < DYNAMIC_RECIPE_INPUT_COUNT;
}

//-----------------------------------------------------------------------------
// Purpose: Is this index an output panel
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::IsOutputPanel( int iPanelIndex) const
{ 
	return iPanelIndex < ( DYNAMIC_RECIPE_OUTPUT_COUNT + DYNAMIC_RECIPE_INPUT_COUNT ) && !IsInputPanel(iPanelIndex); 
}

//-----------------------------------------------------------------------------
// Purpose: Is this index a backpack panel
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::IsBackpackPanel( int iPanelIndex ) const
{ 
	return ( iPanelIndex >= ( DYNAMIC_RECIPE_INPUT_COUNT + DYNAMIC_RECIPE_OUTPUT_COUNT )
		&& !IsInputPanel( iPanelIndex )
		&& !IsOutputPanel( iPanelIndex ) );
}

//-----------------------------------------------------------------------------
// Purpose: Is this backpack panel on this page
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::IsInvPanelOnThisPage( unsigned nIndex ) const
{
	unsigned nNumPerPage = DYNAMIC_RECIPE_BACKPACK_ROWS * DYNAMIC_RECIPE_BACKPACK_COLS;
	unsigned nMinIndex = nNumPerPage * GetCurrentPage();
	unsigned nMaxIndex = nNumPerPage * (GetCurrentPage() + 1);

	return nIndex >= nMinIndex && nIndex < nMaxIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Given the number of items that match the recipe, how many pages
//			do we need to show
//-----------------------------------------------------------------------------
int CDynamicRecipePanel::GetNumPages()
{
	return ceil( float(m_nNumRecipeItems) / float(GetNumBackpackPanelsPerPage()) );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the page of the backpack panels
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::SetCurrentPage( int nNewPage )
{
	if ( nNewPage < 0 )
	{
		nNewPage = GetNumPages() - 1;
	}
	else if ( nNewPage >= GetNumPages() )
	{
		nNewPage = 0;
	}

	FOR_EACH_VEC( m_vecBackpackModelPanels, i )
	{
		m_vecBackpackModelPanels[i]->SetPageNumber( nNewPage );
	}

	BaseClass::SetCurrentPage( nNewPage );
}

//-----------------------------------------------------------------------------
// Purpose: Chance the current page for all input panels
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::SetCurrentInputPage( int nNewPage )
{
	m_nInputPage = nNewPage;

	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		m_vecRecipeInputModelPanels[i]->SetPageNumber( nNewPage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Given the number of input items, how many input pages do we need to show
//-----------------------------------------------------------------------------
int CDynamicRecipePanel::GetNumInputPages() const
{
	return ceil( float(m_RecipeIterator.GetInputCount()) / float(GetNumInputPanelsPerPage()) );
}

//-----------------------------------------------------------------------------
// Purpose: Given the number of output items, how many output pages do we need to show
//-----------------------------------------------------------------------------
int CDynamicRecipePanel::GetNumOutputPage() const
{
	return ceil( float(m_RecipeIterator.GetOutputCount()) / float(GetNumOutputPanelsPerPage()) );
}

//-----------------------------------------------------------------------------
// Purpose: Reset all of the item panels, their pages, their info and re-evaluate
//			the recipe item for attributes and the backpack for matching items
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::InitItemPanels()
{
	// Clear out every panel's items
	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		m_pItemModelPanels[i]->SetItem( NULL );
	}

	// Go through and set the item to all inventory panels to NULL
	FOR_EACH_VEC( m_vecBackpackModelPanels, i )
	{
		m_vecBackpackModelPanels[i]->DeleteRecipes();
	}

	// Go through our backpack and repopulate our backpack and matching components
	FindPossibleBackpackItems();

	// Reset to the first page
	SetCurrentInputPage( 0 );
	SetCurrentPage( 0 );

	// Go through and set default items on input panels
	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[i];

		// Clear out any recipes we have
		pPanel->DeleteRecipes();
		// Clear out stale info
		pPanel->SetDynamicRecipeItem( m_pDynamicRecipeItem );
	}

	for( int i=0; i < m_RecipeIterator.GetInputCount(); ++i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[ i % m_vecRecipeInputModelPanels.Count() ];

		pPanel->AddDefaultItem( m_RecipeIterator.GetInputItem( i ) );
		pPanel->AddComponentInfo( m_RecipeIterator.GetInputAttrib( i ) );
	}

	// Go through and set items into output panels
	FOR_EACH_VEC( m_vecRecipeOutputModelPanels, i )
	{
		CItemModelPanel* pPanel = m_vecRecipeOutputModelPanels[i];
		
		// Set appropriate item for output panel
		if( i < m_RecipeIterator.GetOutputCount() )
		{
			int nOutputIndex = (m_nOutputPage * DYNAMIC_RECIPE_OUTPUT_COUNT) + i;
			pPanel->SetItem( m_RecipeIterator.GetOutputItem( nOutputIndex ) );
		}
	}

	// Go through all the recipe items and set them into inventory panels
	PopulatePanelsForCurrentPage();

	UpdateModelPanels();
}


//-----------------------------------------------------------------------------
// Purpose: Fills in the backpack slots with the items for the current page
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::PopulatePanelsForCurrentPage()
{
	// Go through all the recipe items and set them into inventory panels
	FOR_EACH_VEC( m_vecBackpackModelPanels, i )
	{
		CRecipeComponentItemModelPanel* pPanel = m_vecBackpackModelPanels[i];
		// Update the item we display.  Our inventory might have shifted around
		pPanel->UpdateDisplayItem();

		bool bIsSlotOpen = pPanel->IsSlotAvailable( GetCurrentPage() );
		pPanel->SetVisible( bIsSlotOpen );
		pPanel->SetEnabled( bIsSlotOpen );
		pPanel->InvalidateLayout();
		SetBorderForItem( pPanel, false );
	}

	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[i];
		pPanel->SetPageNumber( m_nInputPage );
		pPanel->UpdateDisplayItem();

		bool bIsSlotOpen = pPanel->IsSlotAvailable( m_nInputPage );
		pPanel->SetVisible( bIsSlotOpen );
		pPanel->SetEnabled( bIsSlotOpen );
		pPanel->InvalidateLayout();
		SetBorderForItem( pPanel, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Go through the player's entire backpack and check if each of them
//			passes any of the input panel's attributes criteria
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::FindPossibleBackpackItems()
{
	Assert( m_pDynamicRecipeItem->GetSOCData() );
	// Iterate through the attributes on our recipe item
	// and create all of our input and output items.
	m_RecipeIterator.Reset();
	CEconItem* pEconItem = m_pDynamicRecipeItem->GetSOCData() ;
	pEconItem->IterateAttributes( &m_RecipeIterator );

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( pLocalInv == NULL )
		return;

	m_nNumRecipeItems = 0;

	// Go through our backpack and filter items that match our input criteria
	for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
	{
		CEconItemView *pItem = pLocalInv->GetItem( i );
		Assert( pItem );

		// If we're not showing untradable items, and this item is untradeable
		// then we just skip it
		if ( !pItem->IsTradable() && !m_bShowUntradable )
			continue;

		CDynamicRecipeItemMatchFind matchingIterator( m_pDynamicRecipeItem, pItem );
		m_pDynamicRecipeItem->IterateAttributes( &matchingIterator );
		if( matchingIterator.MatchesAnyAttributes() )
		{
			// Set in the recipe
			m_vecBackpackModelPanels[ m_nNumRecipeItems % GetNumBackpackPanelsPerPage() ]->AddRecipe( pItem->GetItemID() );
			++m_nNumRecipeItems;
		}
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Layout the item panels
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::PositionItemPanel( CItemModelPanel *pPanel, int iIndex  )
{
	int iCenter = 0;
	int iButtonX = 0, iButtonY = 0, iXPos = 0, iYPos = 0;

	// Position all of the panels
	if( IsInputPanel( iIndex ) )
	{
		iButtonX = (iIndex % CRAFTING_SLOTS_INPUT_COLUMNS);
		iButtonY = (iIndex / CRAFTING_SLOTS_INPUT_COLUMNS);
		iXPos = (iCenter + m_iItemCraftingOffcenterX) + (iButtonX * pPanel->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		iYPos = m_iItemYPos + (iButtonY * pPanel->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);

		pPanel->SetPos( iXPos, iYPos );
	}
	else if( IsOutputPanel( iIndex ) )
	{
		int iButtonIndex = iIndex - DYNAMIC_RECIPE_INPUT_COUNT;
		iButtonX = (iButtonIndex % CRAFTING_SLOTS_OUTPUT_COLUMNS);
		iButtonY = (iButtonIndex / CRAFTING_SLOTS_OUTPUT_COLUMNS);
		iXPos = (iCenter + m_iItemCraftingOffcenterX) + (iButtonX * pPanel->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		iYPos = m_iOutputItemYPos + (iButtonY * pPanel->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);

		pPanel->SetPos( iXPos, iYPos );
		pPanel->SetItem( m_RecipeIterator.GetOutputItem( iButtonIndex ) );
	}
	else if( IsBackpackPanel( iIndex ) )
	{
		int iButtonIndex = iIndex - DYNAMIC_RECIPE_INPUT_COUNT - DYNAMIC_RECIPE_OUTPUT_COUNT;
		iButtonX = (iButtonIndex % DYNAMIC_RECIPE_BACKPACK_COLS);
		iButtonY = (iButtonIndex / DYNAMIC_RECIPE_BACKPACK_COLS);
		iXPos = (iCenter + m_iInventoryXPos) + (iButtonX * pPanel->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		iYPos = m_iInventoryYPos + (iButtonY * pPanel->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);

		pPanel->SetPos( iXPos, iYPos );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Update each panel to see if it should show, what item to show, and if
//			it's greyed out.  Update the page labels here too.
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::UpdateModelPanels( void )
{
	// Check if any input panels have recipe items in them
	bool bAnyInputHasRecipe = false;
	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		CInputPanelItemModelPanel* pPanel = m_vecRecipeInputModelPanels[i];
		
		// Update the item we display.  Our inventory might have shifted around
		pPanel->UpdateDisplayItem();

		SetBorderForItem( pPanel, false );
		// Check for a recipe
		for( int j=0; j < GetNumInputPages(); ++j )
		{
			CEconItemView* pRecipe = pPanel->GetRecipeItem( j );
			bAnyInputHasRecipe |= pRecipe != NULL;
		}
		// Input panels are visible and enabled if their recipe slot is available
		pPanel->SetEnabled( pPanel->GetDefaultItem() != NULL );
		pPanel->SetVisible( pPanel->GetDefaultItem() != NULL );
	}

	static CSchemaAttributeDefHandle pAttrib_NoPartialComplete( "recipe no partial complete" );
	bool bPartialCompletionAllowed = !m_pDynamicRecipeItem->FindAttribute( pAttrib_NoPartialComplete );

	m_pInputsLabel->SetText( bPartialCompletionAllowed ? "#Craft_Recipe_Inputs" : "#Dynamic_Recipe_Outputs_No_Partial_Complete" );

	// If any of the input panels have a recipe in them, then crafting is available
	if ( m_pRecipeCraftButton )
	{
		bool bCraftEnabled = ( bPartialCompletionAllowed && bAnyInputHasRecipe ) || AllRecipePanelsFilled();
		m_pRecipeCraftButton->SetEnabled( bCraftEnabled );
	}
	if ( m_pRecipeCraftButton )
	{
		m_pRecipeCraftButton->SetText( AllRecipePanelsFilled()	? "#CraftConfirm"			: "#ToolCustomizeTextureOKButton" );
	}
	if ( m_pOutputsLabel )
	{
		m_pOutputsLabel->SetText( AllRecipePanelsFilled()		? "#Craft_Recipe_Outputs"	: "#Dynamic_Recipe_Outputs_Not_Complete");
		m_pOutputsLabel->SetFgColor( AllRecipePanelsFilled()	? Color( 200, 80, 60, 255 )	: Color( 117, 107, 94, 255 ) );
	}

	FOR_EACH_VEC( m_vecRecipeOutputModelPanels, i )
	{
		CItemModelPanel* pPanel = m_vecRecipeOutputModelPanels[i];
		SetBorderForItem( pPanel, false );

		// Output panels are visible and enabled if they have an item in them
		pPanel->SetVisible( pPanel->GetItem() != NULL );
		pPanel->SetEnabled( pPanel->GetItem() != NULL );
	}

	PopulatePanelsForCurrentPage();

	// Update the current backpack page numbers
	char szTmp[16];
	Q_snprintf(szTmp, 16, "%d/%d", GetCurrentPage() + 1, GetNumPages() );
	m_pInventoryContainer->SetDialogVariable( "backpackpage", szTmp );

	// Update the current input page numbers
	Q_snprintf(szTmp, 16, "%d/%d", m_nInputPage + 1, GetNumInputPages() );
	m_pRecipeContainer->SetDialogVariable( "inputpage", szTmp );

	DeSelectAllBackpackItemPanels();
}

//-----------------------------------------------------------------------------
// Purpose: If this is a backpack panel, send the item into the first accepting
//			input pane, if one exists, or else do nothing.  If this is a input
//			panel with a backpack item in it, send the backpack item back to the
//			backpack.
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnItemPanelMouseDoublePressed( vgui::Panel *panel )
{
	CRecipeComponentItemModelPanel *pSrcPanel = dynamic_cast < CRecipeComponentItemModelPanel * > ( panel );
	if( !pSrcPanel )
		return;

	int iIndex = GetBackpackPositionForPanel( pSrcPanel );

	// Send from an input panel to the first open backpack panel, even on a different page
	if( IsInputPanel( iIndex ) )
	{
		CInputPanelItemModelPanel *pInputPanel = assert_cast<CInputPanelItemModelPanel*>( pSrcPanel );
		Assert( pInputPanel );

		int nSrcRecipeIndex = pSrcPanel->GetRecipeIndex( pInputPanel->GetPageNumber() );
		CEconItemView* pRecipeItem = pInputPanel->GetRecipeItem( pInputPanel->GetPageNumber() );
		if( pRecipeItem )
		{
			// Just put it in the first open slot.
			ReturnRecipeItemToBackpack( nSrcRecipeIndex, pInputPanel, pInputPanel->GetPageNumber() );
		}
	}
	else if( IsBackpackPanel( iIndex ) )
	{
		// Sending from the backpack panel to the first matching input panel that's on the current input page.
		int nSrcRecipeIndex = pSrcPanel->GetRecipeIndex( GetCurrentPage() );
		FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
		{
			CInputPanelItemModelPanel *pDstPanel = m_vecRecipeInputModelPanels[i];
			if( pDstPanel->GetRecipeItem( pDstPanel->GetPageNumber() ) == NULL && pDstPanel->MatchesAttribCriteria( nSrcRecipeIndex ) )
			{
				// Next check if we just want to move the item
				SetRecipeComponentIntoPanel( nSrcRecipeIndex, pSrcPanel, pSrcPanel->GetPageNumber(), pDstPanel, pDstPanel->GetPageNumber() );
				break;
			}
		}
	}

	m_bInputPanelsDirty = true;
	// Force panels to update
	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: Border highlight if the panel has an item in it
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	m_pMouseOverItemPanel = pItemPanel;

	CEconItemView *pItem = pItemPanel->GetItem();

	// Recalc the borders on item panels
	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		SetBorderForItem( m_vecRecipeInputModelPanels[i], false );
	}

	if ( pItemPanel && IsVisible() )
	{
		SetBorderForItem( pItemPanel, pItem != NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn of border highlights
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	m_pMouseOverItemPanel = NULL;

	// Recalc the borders on item panels
	FOR_EACH_VEC( m_vecRecipeInputModelPanels, i )
	{
		SetBorderForItem( m_vecRecipeInputModelPanels[i], false );
	}
	
	if ( pItemPanel && !pItemPanel->IsSelected() )
	{
		SetBorderForItem( pItemPanel, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: We got a craft response!  Close out this window because we're going
//			to show the user their loot
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::OnRecipeCompleted()
{
	SetVisible( false );
	m_flAbortCraftingAt = 0.f;

}


//-----------------------------------------------------------------------------
// Purpose: Set the border for the item.  Input panels highlight when a dragged
//			item matches its criteria.  Output items highlight when all inputs
//			are fulfilled.
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	if ( !pItemPanel || !pItemPanel->IsVisible() )
		return;

	int iIndex = GetBackpackPositionForPanel( pItemPanel );
	if( IsInputPanel( iIndex ) )
	{
		// Special case for input panels.  They need to highlight when a dragged item
		// matches their criteria.
		CItemModelPanel* pPanel = m_bDragging ? m_pMouseDragItemPanel : m_pMouseOverItemPanel;
		CEconItemView* pPanelItem = pPanel ? pPanel->GetItem() : NULL;

		CInputPanelItemModelPanel* pInputPanel = dynamic_cast<CInputPanelItemModelPanel*>( pItemPanel );
		Assert( pInputPanel );
		// If this panel doesnt have a recipe, then we want some sort of greyed out look
		if( pInputPanel->GetRecipeItem( pInputPanel->GetPageNumber() ) == NULL )
		{
			const char *pszBorder = NULL;
			int iRarity = GetItemQualityForBorder( pItemPanel );

			// We only want backpack panels and the drag panel to be highlight sources
			bool bValidHighlightSourcePanel = pPanel == m_pMouseDragItemPanel 
			|| m_vecBackpackModelPanels.Find( static_cast<CRecipeComponentItemModelPanel*>(pPanel) ) != m_vecBackpackModelPanels.InvalidIndex();

			// If this panel can accept whatever the source panel is, we want to highlight a bit
			if( bValidHighlightSourcePanel && pPanelItem && pInputPanel->MatchesAttribCriteria( pPanelItem->GetItemID() ) )
			{
				if ( pItemPanel->GetItem() )
				{
					pszBorder = g_szItemBorders[iRarity][3];
					pItemPanel->SetGreyedOut( NULL );
				}
				else
				{
					pszBorder = g_szItemBorders[iRarity][0];
				}
			}
			else // Look gloomy and uninviting if not
			{
				pszBorder = g_szItemBorders[iRarity][3];
				pItemPanel->SetGreyedOut( "" );
			}

			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
			pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
			return;
		}
		
	}
	else if( IsOutputPanel( iIndex ) )
	{
		const char *pszBorder = NULL;
		int iRarity = GetItemQualityForBorder( pItemPanel );

		// The output panel greys out when any of the input panels are not fulfilled,
		// and lights up when they are all fulfilled
		if ( iRarity >= 0 && iRarity < ARRAYSIZE( g_szItemBorders ) )
		{
			if( AllRecipePanelsFilled() )
			{
				pszBorder = g_szItemBorders[iRarity][0];
				pItemPanel->SetGreyedOut( NULL );
			}
			else
			{
				pszBorder = g_szItemBorders[iRarity][3];
				pItemPanel->SetGreyedOut( NULL );
			}
		}

		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
		return;
	}

	// Backpack and output panels get default treatment
	BaseClass::SetBorderForItem( pItemPanel, bMouseOver );
}

//-----------------------------------------------------------------------------
// Purpose: Swap recipe items in srcpanel and dstpanel
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::SetRecipeComponentIntoPanel( itemid_t nSrcRecipeIndex, CRecipeComponentItemModelPanel* pSrcPanel, int nSrcPage, CRecipeComponentItemModelPanel* pDstPanel, int nDstPage )
{
	Assert( nSrcRecipeIndex != 0 );
	Assert( pSrcPanel );
	Assert( pDstPanel );
	Assert( pSrcPanel->GetRecipeItem( nSrcPage ) );

	// Get the recipe from the destination panel
	itemid_t pDstRecipeIndex = pDstPanel->GetRecipeIndex( nDstPage );
	// Swap recipes
	pSrcPanel->SetRecipeItem( pDstRecipeIndex, nSrcPage );
	pDstPanel->SetRecipeItem( nSrcRecipeIndex, nDstPage );
}

//-----------------------------------------------------------------------------
// Purpose: Check if a given panel is allowed to be dragged.  
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::AllowDragging( CItemModelPanel *pPanel )
{
	int iIndex = GetBackpackPositionForPanel( pPanel );

	// If this isn't an input or backpack panel, abort
	if( !IsInputPanel( iIndex ) && !IsBackpackPanel( iIndex ) )
		return false;

	CRecipeComponentItemModelPanel* pRecipePanel = dynamic_cast< CRecipeComponentItemModelPanel* >( pPanel );
	Assert( pRecipePanel );
	if( !pRecipePanel )
		return false;

	int nPageNumber = 0;
	if( IsBackpackPanel( iIndex ) )
	{ 
		nPageNumber = GetCurrentPage();
	}
	else if ( IsInputPanel( iIndex ) )
	{
		nPageNumber = m_nInputPage;
	}
	// Get the recipe item out of this panel
	CEconItemView* pRecipe = pRecipePanel->GetRecipeItem( nPageNumber );
	
	// If no recipe, abort
	if( !pRecipe )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Start dragging!
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::StartDrag( int x, int y )
{
	BaseClass::StartDrag( x, y );

	// Recalc the borders on item panels
	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		SetBorderForItem( m_pItemModelPanels[i], false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop dragging
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::StopDrag( bool bSucceeded )
{
	BaseClass::StopDrag( bSucceeded );

	// Recalc the borders on item panels
	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		SetBorderForItem( m_pItemModelPanels[i], false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Helper function to find out if an input panel can accept an item
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::InputPanelCanAcceptItem( CItemModelPanel* pPanel, itemid_t nItemID )
{
	Assert( IsInputPanel( GetBackpackPositionForPanel( pPanel ) ) );

	CInputPanelItemModelPanel* pInputPanel = dynamic_cast<CInputPanelItemModelPanel*>( pPanel );
	Assert( pInputPanel );

	return pInputPanel->MatchesAttribCriteria( nItemID );
}

//-----------------------------------------------------------------------------
// Purpose: Check if the dragged item can be dropped into a given panel
//-----------------------------------------------------------------------------
bool CDynamicRecipePanel::CanDragTo( CItemModelPanel *pItemPanel, int iPanelIndex )
{
	// From input to backpack panel
	int iDraggedFromPos = GetBackpackPositionForPanel(m_pItemDraggedFromPanel);
	if( IsInputPanel( iDraggedFromPos ) && IsBackpackPanel( iPanelIndex ) )
		return true;

	itemid_t itemID = m_pMouseDragItemPanel->GetItem() 
		? m_pMouseDragItemPanel->GetItem()->GetItemID() 
		: 0;

	// From backpack to input panel that can accept the item
	if( IsBackpackPanel( iDraggedFromPos ) 
		&& IsInputPanel( iPanelIndex )
		&& InputPanelCanAcceptItem( pItemPanel,itemID ) )
		return true;

	// From inoput to other input that can also accept the item
	if( IsInputPanel( iDraggedFromPos ) 
		&& IsInputPanel( iPanelIndex ) 
		&& InputPanelCanAcceptItem( pItemPanel, itemID ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handle the user releasing their drag on a given panel. Only valid
//			from backpack<->backpack, input<->input, backpack<->input
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::HandleDragTo( CItemModelPanel * /*pItemPanel*/, int iPanelIndex )
{
	int iDraggedFromPos = GetBackpackPositionForPanel(m_pItemDraggedFromPanel);

	// The destination panel needs to exist, be enabled and be visible or else we dont consider it.
	CItemModelPanel* pDestinationPanel = m_pItemModelPanels[ iPanelIndex ];
	if( !pDestinationPanel || !pDestinationPanel->IsEnabled() || !pDestinationPanel->IsVisible() )
	{
		return;
	}

	m_bInputPanelsDirty = true;

	// If we dragged from a inventory panel onto an input panel
	if( IsInputPanel( iPanelIndex ) && IsBackpackPanel( iDraggedFromPos ) )
	{
		CRecipeComponentItemModelPanel* pSrcPanel = dynamic_cast<CRecipeComponentItemModelPanel*>( m_pItemModelPanels[iDraggedFromPos] );
		CInputPanelItemModelPanel* pDstPanel = dynamic_cast<CInputPanelItemModelPanel*>( m_pItemModelPanels[iPanelIndex] );

		if( pSrcPanel && pDstPanel )
		{
			// They dragged an item from the backpack into an input slot.  Check if this is ok.
			itemid_t nSrcRecipeIndex = pSrcPanel->GetRecipeIndex( GetCurrentPage() );
			Assert( nSrcRecipeIndex != 0 );
			if( nSrcRecipeIndex != 0 && pDstPanel->MatchesAttribCriteria( nSrcRecipeIndex ) )
			{
				SetRecipeComponentIntoPanel( nSrcRecipeIndex, pSrcPanel, GetCurrentPage(), pDstPanel, pDstPanel->GetPageNumber() );
			}
		}
	}
	else if( IsInputPanel( iDraggedFromPos ) && IsBackpackPanel( iPanelIndex ) )
	{
		CRecipeComponentItemModelPanel* pSrcPanel = dynamic_cast<CRecipeComponentItemModelPanel*>( m_pItemModelPanels[iDraggedFromPos] );
		CRecipeComponentItemModelPanel* pDstPanel = dynamic_cast<CRecipeComponentItemModelPanel*>( m_pItemModelPanels[iPanelIndex] );

		// If we dragged from an input panel to a backpack panel, return the item
		// to its original backpack slot regardless of where they dragged it to
		if( pSrcPanel && pDstPanel )
		{
			// They dragged from an input panel to inventory panel.  Check if there's something in 
			// the inventory slot already.  If so, just move to the first open spot.
			itemid_t pSrcRecipeIndex = pSrcPanel->GetRecipeIndex( pSrcPanel->GetPageNumber() );
			itemid_t pDstRecipeIndex = pDstPanel->GetRecipeIndex( GetCurrentPage() );

			Assert( pSrcRecipeIndex != 0 );
			if( pSrcRecipeIndex != 0 && pDstRecipeIndex != 0 )
			{
				// There's already a recipe item in there.  Just put it in the first open slot.
				ReturnRecipeItemToBackpack( pSrcRecipeIndex, pSrcPanel, 0 );
			}
			else if( pSrcRecipeIndex )
			{
				// It's open!  Place in there
				SetRecipeComponentIntoPanel( pSrcRecipeIndex, pSrcPanel, pSrcPanel->GetPageNumber(), pDstPanel, GetCurrentPage() );
			}
		}
	}
	else if( IsInputPanel( iDraggedFromPos ) && IsInputPanel( iPanelIndex ) )
	{
		CInputPanelItemModelPanel* pSrcPanel = dynamic_cast<CInputPanelItemModelPanel*>( m_pItemModelPanels[iDraggedFromPos] );
		CInputPanelItemModelPanel* pDstPanel = dynamic_cast<CInputPanelItemModelPanel*>( m_pItemModelPanels[iPanelIndex] );

		// Dragged from an input panel to an input panel.
		if( pSrcPanel && pDstPanel )
		{
			itemid_t nSrcRecipeIndex = pSrcPanel->GetRecipeIndex( pSrcPanel->GetPageNumber() );
			itemid_t nDstRecipeIndex = pDstPanel->GetRecipeIndex( pDstPanel->GetPageNumber() );

			//  First see if we can swap our inputs
			if( nSrcRecipeIndex != 0 && nDstRecipeIndex != 0 )
			{
				if( pSrcPanel->MatchesAttribCriteria( nDstRecipeIndex ) &&
					pDstPanel->MatchesAttribCriteria( nSrcRecipeIndex ) )
				{
					SetRecipeComponentIntoPanel( nDstRecipeIndex, pSrcPanel, 0, pDstPanel, 0 );
				}
			}
			else if( nSrcRecipeIndex != 0 && nDstRecipeIndex == 0 && pDstPanel->MatchesAttribCriteria( nSrcRecipeIndex ) )
			{
				// Next check if we just want to move the item
				SetRecipeComponentIntoPanel( nSrcRecipeIndex, pSrcPanel, pSrcPanel->GetPageNumber(), pDstPanel, pDstPanel->GetPageNumber() );
			}
		}
	}
	else
	{
		AssertMsg( 0, "Unhandled drag case!" );
	}

	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: Return a given item to the first open slot in out backpack
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::ReturnRecipeItemToBackpack( itemid_t nItemID, CRecipeComponentItemModelPanel* pSrcPanel, int nSrcPage )
{
	// For each page, check each recipe slot, on each panel for an opening
	for( int i=0; i < GetNumPages(); ++i )
	{
		FOR_EACH_VEC( m_vecBackpackModelPanels, j )
		{
			CRecipeComponentItemModelPanel* pDstPanel = m_vecBackpackModelPanels[j];
			CEconItemView* pDstRecipe = pDstPanel->GetRecipeItem( i );
			bool bSlotIsOpen = pDstPanel->IsSlotAvailable( i );
			if( bSlotIsOpen && pDstRecipe == NULL )
			{
				SetRecipeComponentIntoPanel( nItemID, pSrcPanel, nSrcPage, pDstPanel, i );
				return;
			}
		}
	}

	AssertMsg( 0, "No open backpack slot found when returning item to backpack!" );
}

//-----------------------------------------------------------------------------
// Purpose: Set in a new recipe item.  Update labels and reset panels.
//-----------------------------------------------------------------------------
void CDynamicRecipePanel::SetNewRecipe( CEconItemView* pNewRecipeItem )
{
	m_flAbortCraftingAt = 0.f;
	m_pDynamicRecipeItem = pNewRecipeItem;
	// Update recipe title
	m_pRecipeContainer->SetDialogVariable( "recipetitle", m_pDynamicRecipeItem->GetItemName() );
	// By default, dont show untradable items
	m_pShowUntradableItemsCheckbox->SetSelected( false );
	// Repopulate the panels
	InitItemPanels();
	UpdateModelPanels();
}


class CWaitForConsumeDialog : public CGenericWaitingDialog
{
public:
	CWaitForConsumeDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );

		// Show them their loot!
		InventoryManager()->ShowItemsPickedUp( true );
	}
};

void CDynamicRecipePanel::OnCraftResponse( itemid_t nNewToolID, EGCMsgResponse eResponse )
{
	// We got a response.  We dont need to time-out
	m_flAbortCraftingAt = 0;

	switch( eResponse )
	{
		case k_EGCMsgResponseOK:
		{
			// If a new tool id comes back, that means we only partially completed the recipe
			if( nNewToolID != 0 )
			{
				OpenCraftingStatusDialog( this, "#Dynamic_Recipe_Response_Success", false, true, false );

				// Play a sound letting them know the item is gone
				const char *pszSoundFilename = m_pDynamicRecipeItem->GetDefinitionString( "recipe_partial_complete_sound", "ui/chem_set_add_element.wav" );
				vgui::surface()->PlaySound( pszSoundFilename );

				CEconItemView* pNewRecipe = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( nNewToolID );
				Assert( pNewRecipe );
				// Set the new recipe into the panel.  This resets all the item panels within.
				SetNewRecipe( pNewRecipe );
			}
			else
			{
				CloseCraftingStatusDialog();

				// No new tool, so we completed the recipe!
				const char *pszSoundFilename = m_pDynamicRecipeItem->GetDefinitionString( "recipe_complete_sound", "ui/chem_set_creation.wav" );
				vgui::surface()->PlaySound( pszSoundFilename );
				// Show the "Completing consumption" dialog for 5 seconds
				ShowWaitingDialog( new CWaitForConsumeDialog( NULL ), "#ToolConsumptionInProgress", true, false, 5.0f );

				// Hide the dynamic recipe panel after 6 seconds
				g_DynamicRecipePanel->PostMessage( g_DynamicRecipePanel, new KeyValues("RecipeCompleted"), 6.f );
			}
		}
		break;
		case k_EGCMsgResponseInvalid:
		{
			// Something was bad in the request.
			OpenCraftingStatusDialog( this, "#Dynamic_Recipe_Response_Invalid", false, true, false );
			
			InitItemPanels();
			UpdateModelPanels();
		}
		break;
		case k_EGCMsgResponseNoMatch:
		{
			// One or more of the items sent in didnt match
			OpenCraftingStatusDialog( this, "#Dynamic_Recipe_Response_NoMatch", false, true, false );

			InitItemPanels();
			UpdateModelPanels();
		}
		break;
		default:
		{
			OpenCraftingStatusDialog( this, "#Dynamic_Recipe_Response_Default", false, true, false );

			InitItemPanels();
			UpdateModelPanels();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the dynamic recipe
//-----------------------------------------------------------------------------
class CGCCompleteDynamicRecipeResponse : public GCSDK::CGCClientJob
{
public:
	CGCCompleteDynamicRecipeResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		itemid_t nNewToolID = INVALID_ITEM_ID;
		if( !msg.BReadUint64Data( &nNewToolID ) )
			return true;

		g_DynamicRecipePanel->OnCraftResponse( nNewToolID, (EGCMsgResponse)msg.Body().m_eResponse );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCCompleteDynamicRecipeResponse, "CGCCompleteDynamicRecipeResponse", k_EMsgGCFulfillDynamicRecipeComponentResponse, GCSDK::k_EServerTypeGCClient );

