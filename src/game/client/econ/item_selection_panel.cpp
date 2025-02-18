//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "item_selection_panel.h"
#include "vgui/ISurface.h"
#include "c_tf_player.h"
#include "gamestringpool.h"
#include "iclientmode.h"
#include "tf_item_inventory.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui/IInput.h"
#include "item_model_panel.h"
#include "econ_item_constants.h"
#include "econ_item_system.h"
#include "econ_item_description.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_item_selection_panel_sort_type( "tf_item_selection_panel_sort_type", 0, FCVAR_NONE, "0 - Sort is off, 1 - Sort is Alphabet (Pub)" );

const char *g_szEquipSlotHeader[] = 
{
	"#ItemSel_PRIMARY",		// LOADOUT_POSITION_PRIMARY = 0,
	"#ItemSel_SECONDARY",	// LOADOUT_POSITION_SECONDARY,
	"#ItemSel_MELEE",		// LOADOUT_POSITION_MELEE,
	"#ItemSel_UTILITY",		// LOADOUT_POSITION_UTILITY // Staging
	"#ItemSel_PDA",			// LOADOUT_POSITION_BUILDING,
	"#ItemSel_PDA",			// LOADOUT_POSITION_PDA,
	"#ItemSel_PDA",			// LOADOUT_POSITION_PDA2
	"#ItemSel_MISC",		// LOADOUT_POSITION_HEAD
	"#ItemSel_MISC",		// LOADOUT_POSITION_MISC
	"#ItemSel_ACTION",		// LOADOUT_POSITION_ACTION
	"#ItemSel_MISC",		// LOADOUT_POSITION_MISC2
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT2
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT3
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT4
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT5
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT6
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT7
	"#ItemSel_TAUNT",		// LOADOUT_POSITION_TAUNT8
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szEquipSlotHeader ) == CLASS_LOADOUT_POSITION_COUNT );

static bool ShouldItemNotStack( CEconItemView *pItemData )
{
	CEconItem *pSOCData = pItemData->GetSOCData();
	if ( pSOCData )
	{
		if ( pSOCData->BHasDynamicAttributes() )
			return true;

		if ( pSOCData->GetOrigin() == kEconItemOrigin_UntradableFreeContractReward )
			return true;
	}

	return false;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemSelectionPanel::CItemSelectionPanel(Panel *parent) : CBaseLoadoutPanel(parent, "ItemSelectionPanel")
{
	m_pCaller = parent;
	m_pSelectionItemModelPanelKVs = NULL;
	m_pDuplicateLabelKVs = NULL;
	m_bShowingEntireBackpack = false;
	m_iItemsInSelection = 0;

	m_bShowDuplicates = false;
	m_bForceBackpack = false;
	m_pOnlyAllowUniqueQuality = NULL;
	m_pShowBackpack = NULL;
	m_pShowSelection = NULL;
	m_pNextPageButton = NULL;
	m_pPrevPageButton = NULL;
	m_pCurPageLabel = NULL;
	m_pNoItemsInSelectionLabel = NULL;
	m_bGotMousePressed = false;
	m_pNameFilterTextEntry = NULL;

	m_DuplicateCounts.SetLessFunc( DefLessFunc( DuplicateCountsMap_t::KeyType_t ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemSelectionPanel::~CItemSelectionPanel()
{
	if ( m_pSelectionItemModelPanelKVs )
	{
		m_pSelectionItemModelPanelKVs->deleteThis();
		m_pSelectionItemModelPanelKVs = NULL;
	}
	if ( m_pDuplicateLabelKVs )
	{
		m_pDuplicateLabelKVs->deleteThis();
		m_pDuplicateLabelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	m_pNameFilterTextEntry = NULL;

	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetSchemeFile() );

	m_pNoItemsInSelectionLabel = dynamic_cast<vgui::Label*>( FindChildByName("NoItemsLabel") );
	m_pOnlyAllowUniqueQuality = dynamic_cast<vgui::CheckButton *>( FindChildByName("OnlyAllowUniqueQuality") );
	m_pShowBackpack = dynamic_cast<CExButton*>( FindChildByName("ShowBackpack") );
	m_pShowSelection = dynamic_cast<CExButton*>( FindChildByName("ShowSelection") );
	m_pNextPageButton = dynamic_cast<CExButton*>( FindChildByName("NextPageButton") );
	m_pPrevPageButton = dynamic_cast<CExButton*>( FindChildByName("PrevPageButton") );
	m_pCurPageLabel = dynamic_cast<vgui::Label*>( FindChildByName("CurPageLabel") );

	// Give individual selection panels the ability to specify whether or not they want to show the
	// checkbox controlling quality filtering. It really only makes sense for things like crafting,
	// not equipment selection.
	if ( m_pOnlyAllowUniqueQuality )
	{
		m_pOnlyAllowUniqueQuality->SetVisible( DisplayOnlyAllowUniqueQualityCheckbox() );

		// By default, if the checkbox is visible, it's enabled. Users can disable it manually if
		// they want to craft potentially-more-valuable items.
		if ( m_pOnlyAllowUniqueQuality->IsVisible() )
		{
			m_pOnlyAllowUniqueQuality->SetSelected( true );
		}
	}

	
	m_pNameFilterTextEntry = FindControl<vgui::TextEntry>( "NameFilterTextEntry" );
	if ( m_pNameFilterTextEntry )
	{
		m_pNameFilterTextEntry->AddActionSignalTarget( this );
	}

	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "modelpanels_selection_kv" );
	if ( pItemKV )
	{
		if ( m_pSelectionItemModelPanelKVs )
		{
			m_pSelectionItemModelPanelKVs->deleteThis();
		}
		m_pSelectionItemModelPanelKVs = new KeyValues("modelpanels_selection_kv");
		pItemKV->CopySubkeys( m_pSelectionItemModelPanelKVs );
	}

	KeyValues *pLabelKV = inResourceData->FindKey( "duplicatelabels_kv" );
	if ( pLabelKV )
	{
		if ( m_pDuplicateLabelKVs )
		{
			m_pDuplicateLabelKVs->deleteThis();
		}
		m_pDuplicateLabelKVs = new KeyValues("duplicatelabels_kv");
		pLabelKV->CopySubkeys( m_pDuplicateLabelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::ApplyKVsToItemPanels( void )
{
	BaseClass::ApplyKVsToItemPanels();

	if ( !m_bShowingEntireBackpack )
	{
		if ( m_pSelectionItemModelPanelKVs )
		{
			for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
			{
				m_pItemModelPanels[i]->ApplySettings( m_pSelectionItemModelPanelKVs );
				m_pItemModelPanels[i]->UpdatePanels();
			}
		}

		if ( m_pDuplicateLabelKVs )
		{
			FOR_EACH_VEC( m_pDuplicateCountLabels, i )
			{
				if ( !m_pDuplicateCountLabels[i]->IsVisible() )
					continue;

				m_pDuplicateCountLabels[i]->ApplySettings( m_pDuplicateLabelKVs );
				m_pDuplicateCountLabels[i]->SetMouseInputEnabled( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		// In backpack mode we show empty slots. Otherwise we don't.
		bool bVisible = m_bShowingEntireBackpack;
		if ( !bVisible )
		{
			bVisible = (i < GetNumSlotsPerPage()) && ShouldItemPanelBeVisible( m_pItemModelPanels[i], i );
		}
		m_pItemModelPanels[i]->SetVisible( bVisible );

		if ( bVisible )
		{
			PositionItemPanel( m_pItemModelPanels[i], i );
		}

		UpdateDuplicateCounts();
	}

	FOR_EACH_VEC( m_pDuplicateCountLabels, i )
	{
		if ( m_pDuplicateCountLabels[i]->IsVisible() )
		{
			int iXPos, iYPos;
			m_pItemModelPanels[i]->GetPos( iXPos, iYPos );
			m_pDuplicateCountLabels[i]->SetPos( iXPos, iYPos );
		}
	}

	m_pShowBackpack->SetVisible( !m_bShowingEntireBackpack && !m_bForceBackpack );
	m_pShowSelection->SetVisible( m_bShowingEntireBackpack && !m_bForceBackpack );

	m_pNextPageButton->SetVisible( true );
	m_pPrevPageButton->SetVisible( true );
	m_pCurPageLabel->SetVisible( true );
	m_pNextPageButton->SetEnabled( GetNumPages() > 1 );
	m_pPrevPageButton->SetEnabled( GetNumPages() > 1 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_flFilterItemTime && gpGlobals->curtime >= m_flFilterItemTime )
	{
		SetCurrentPage( 0 );
		//DeSelectAllBackpackItemPanels();
		UpdateModelPanels();

		m_flFilterItemTime = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vguicancel" ) )
	{
		PostMessageSelectionReturned( INVALID_ITEM_ID );

		OnClose();
		return;
	}
	else if ( !Q_strnicmp( command, "nextpage", 8 ) )
	{
		HideMouseOverPanel();
		SetCurrentPage( GetCurrentPage() + 1 );
		UpdateModelPanels();
		return;
	}
	else if ( !Q_strnicmp( command, "prevpage", 8 ) )
	{
		HideMouseOverPanel();
		SetCurrentPage( GetCurrentPage() - 1 );
		UpdateModelPanels();
		return;
	}
	else if ( !Q_strnicmp( command, "show_backpack", 8 ) )
	{
		m_bReapplyItemKVs = true;
		m_bShowingEntireBackpack = true;
		UpdateModelPanels();
		//Repaint();
		return;
	}
	else if ( !Q_strnicmp( command, "show_selection", 8 ) )
	{
		m_bReapplyItemKVs = true;
		m_bShowingEntireBackpack = false;
		UpdateModelPanels();
		//Repaint();
		return;
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
void CItemSelectionPanel::OnButtonChecked( KeyValues *pData )
{
	Assert( reinterpret_cast<vgui::Panel *>( pData->GetPtr("panel") ) == m_pOnlyAllowUniqueQuality );
	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the escape key because it doesn't come through as "pressed"
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		// 0 implies do nothing, INVALID_ITEM_ID means stock and we dont want to equip stock
		PostMessageSelectionReturned( 0 );	
		OnClose();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles keypresses in the item selection panel
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// let our parent class handle all the arrow key/dpad stuff
	if( HandleItemSelectionKeyPressed( code ) )
	{
		return;
	}

	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if( nButtonCode == KEY_XBUTTON_A  || code == KEY_ENTER || nButtonCode == STEAMCONTROLLER_A )
	{
		CItemModelPanel *pItemPanel = GetFirstSelectedItemModelPanel( true );
		if( pItemPanel && !pItemPanel->IsGreyedOut() )
		{
			NotifySelectionReturned( pItemPanel );
		}
	}
	else if( nButtonCode == KEY_XBUTTON_B )
	{
		PostMessageSelectionReturned( INVALID_ITEM_ID );
		OnClose();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles key release events in the backpack
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnKeyCodeReleased( vgui::KeyCode code )
{
	if( ! HandleItemSelectionKeyReleased( code ) )
		BaseClass::OnKeyCodeReleased( code );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnClose( void )
{
	BaseClass::OnClose();

	if ( ShouldDeleteOnClose() )
	{
		// Delete ourself now that we're done
		MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::SetVisible( bool bState )
{
	BaseClass::SetVisible( bState );

	if( bState )
	{
		m_wNameFilter.RemoveAll();
		if( m_pNameFilterTextEntry )
		{
			m_pNameFilterTextEntry->SetText( "" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnItemPanelMousePressed( vgui::Panel *panel )
{
	// This is annoying. Why do I get mouse released events for releases that started with a push before I was visible?
	m_bGotMousePressed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	if ( !m_bGotMousePressed )
		return;
	m_bGotMousePressed = false;

	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	NotifySelectionReturned( pItemPanel );
}


//-----------------------------------------------------------------------------
// Purpose: Lets the parent know what the selection was
//-----------------------------------------------------------------------------
void CItemSelectionPanel::NotifySelectionReturned( CItemModelPanel *pItemPanel )
{
	if ( pItemPanel && IsVisible() )
	{
		CEconItemView *pItemData = pItemPanel->GetItem();
		if ( GetItemNotSelectableReason( pItemData ) != NULL )
			return;

		if ( DisableItemSelectionFromGrayedOutPanels() && pItemPanel->IsGreyedOut() )
			return;

		itemid_t ulItemID = INVALID_ITEM_ID;
		if ( pItemData && pItemData->IsValid() )
		{
			ulItemID = pItemData->GetItemID();
		}

		PostMessageSelectionReturned( ulItemID );
	}

	OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::UpdateModelPanels( void )
{
	// If we're showing the whole backpack, go through the inventory like the backpack does.
	if ( m_bShowingEntireBackpack )
	{
		UpdateModelPanelsForSelection();

		if ( m_pNoItemsInSelectionLabel )
		{
			m_pNoItemsInSelectionLabel->SetVisible( false );
		}
	}
	else
	{
		// Clear the dupe counts.
		m_DuplicateCounts.Purge();
		UpdateModelPanelsForSelection();

		if ( m_pNoItemsInSelectionLabel )
		{
			m_pNoItemsInSelectionLabel->SetVisible( m_iItemsInSelection == 0 );
		}
	}

	// Update the current backpack page
	char szTmp[16];
	Q_snprintf(szTmp, 16, "%d/%d", GetCurrentPage()+1, GetNumPages() );
	SetDialogVariable( "backpackpage", szTmp );

	// And we're done!
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::UpdateDuplicateCounts( void )
{
	bool bShow = (m_bShowDuplicates && !m_bShowingEntireBackpack);
	if ( !bShow )
	{
		FOR_EACH_VEC( m_pDuplicateCountLabels, i )
		{
			m_pDuplicateCountLabels[i]->SetVisible( false );
		}
		return;
	}

	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		if ( i >= GetNumSlotsPerPage() )
			break;

		if ( !m_pItemModelPanels[i]->IsVisible() )
		{
			m_pDuplicateCountLabels[i]->SetVisible( false );
			continue;
		}

		if ( m_pDuplicateCountLabels.Count() <= i )
		{
			CExLabel *pLabel = new CExLabel( this, "", "x1" );
			m_pDuplicateCountLabels.AddToTail( pLabel );

			if ( m_pDuplicateLabelKVs )
			{
				pLabel->ApplySettings( m_pDuplicateLabelKVs );
			}
			pLabel->MakeReadyForUse();
			pLabel->InvalidateLayout( true );
			pLabel->SetMouseInputEnabled( false );
		}

		CEconItemView *pItem = m_pItemModelPanels[i]->GetItem();
		if ( !pItem || !pItem->IsValid() || ShouldItemNotStack( pItem ) )
		{
			m_pDuplicateCountLabels[i]->SetVisible( false );
			continue;
		}

		int iIndex = m_DuplicateCounts.Find( item_stack_type_t( pItem->GetItemDefIndex(), pItem->GetQuality() ) );
		if ( iIndex == m_DuplicateCounts.InvalidIndex() || m_DuplicateCounts[iIndex] <= 1 )
		{
			m_pDuplicateCountLabels[i]->SetVisible( false );
			continue;
		}

		wchar_t wzCost[10];
		_snwprintf( wzCost, ARRAYSIZE( wzCost ), L"x%d", m_DuplicateCounts[iIndex] );
		m_pDuplicateCountLabels[i]->SetText( wzCost );
		m_pDuplicateCountLabels[i]->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::CreateItemPanels( void )
{
	// Always create the maximum number of panels
	int iNumPanels = BACKPACK_SLOTS_PER_PAGE;
	if ( m_pItemModelPanels.Count() < iNumPanels )
	{
		for ( int i = m_pItemModelPanels.Count(); i < iNumPanels; i++ )
		{
			AddNewItemPanel(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CItemSelectionPanel::GetNumPages( void )
{
	int iNumItems = 0;
	if ( m_bShowingEntireBackpack )
	{
		iNumItems = InventoryManager()->GetLocalInventory()->GetMaxItemCount();
	}
	else
	{
		iNumItems = m_iItemsInSelection;
	}

	return (int)(ceil((float)iNumItems / (float)GetNumItemPanels()));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSelectionPanel::SetCurrentPage( int nNewPage )
{
	if ( nNewPage < 0 )
	{
		nNewPage = GetNumPages() - 1;
	}
	else if ( nNewPage >= GetNumPages() )
	{
		nNewPage = 0;
	}

	BaseClass::SetCurrentPage( nNewPage );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionPanel::PositionItemPanel( CItemModelPanel *pPanel, int iIndex )
{
	int iCenter = GetWide() * 0.5;
	int iButtonX = (iIndex % GetNumColumns());
	int iButtonY = (iIndex / GetNumColumns());
	int iXPos = (iCenter + m_iItemBackpackOffcenterX) + (iButtonX * m_pItemModelPanels[iIndex]->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
	int iYPos = m_iItemYPos + (iButtonY * m_pItemModelPanels[iIndex]->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);

	m_pItemModelPanels[iIndex]->SetPos( iXPos, iYPos );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionPanel::PostMessageSelectionReturned( itemid_t ulItemID )
{
	KeyValues *pKey = new KeyValues( "SelectionReturned" );
	pKey->SetUint64( "itemindex", ulItemID );
	PostMessage( m_pCaller, pKey );
}

//=====================================================================================================================
// EQUIP SLOT ITEM SELECTION PANEL
//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEquipSlotItemSelectionPanel::CEquipSlotItemSelectionPanel(Panel *parent, int iClass, int iSlot) : CItemSelectionPanel( parent )
{
	m_iClass = iClass;
	m_iSlot = iSlot;

	m_pWeaponLabel = NULL;

	m_flFilterItemTime = 0.f;

	m_iCurrentItemID = INVALID_ITEM_ID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEquipSlotItemSelectionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pWeaponLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemSlotLabel") );

	TFPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
	SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( pData->m_szLocalizableName ) );

	if ( m_pWeaponLabel )
	{
		m_pWeaponLabel->SetText( g_szEquipSlotHeader[m_iSlot] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEquipSlotItemSelectionPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEquipSlotItemSelectionPanel::ShouldItemPanelBeVisible( CItemModelPanel *pPanel, int iPanelIndex )
{
	// If we don't have an item, but we're the first model panel on a slot
	// that has no base item, we still want to be visible because we're the 
	// panel that allows players to select "Empty" for the slot.
	return ( pPanel->HasItem() || (iPanelIndex == 0 && !TFInventoryManager()->SlotContainsBaseItems( GEconItemSchema().GetEquipTypeFromClassIndex( m_iClass ), m_iSlot )) );
}

//-----------------------------------------------------------------------------
// Helper classes/functions for CItemSelectionPanel::UpdateModelPanels().
//-----------------------------------------------------------------------------
// Used to sort/verify uniqueness of user-facing items.
struct RarityEconIdKey
{
	int m_iQualitySort;
	item_definition_index_t m_defIndex;
	uint32 m_unKillEaterScore;

	RarityEconIdKey ( )
		: m_iQualitySort( -1 )
		, m_defIndex( INVALID_ITEM_DEF_INDEX )
		, m_unKillEaterScore( 0 )
	{
		//
	}

	RarityEconIdKey ( int iQuality, item_definition_index_t defIndex, uint32 unKillEaterScore )
		: m_iQualitySort( EconQuality_GetRarityScore( (EEconItemQuality)iQuality ) )
		, m_defIndex( defIndex )
		, m_unKillEaterScore( unKillEaterScore )
	{
		//
	}

	bool operator< ( const RarityEconIdKey& rhs ) const
	{
		return m_defIndex < rhs.m_defIndex
			|| m_iQualitySort < rhs.m_iQualitySort
			|| m_unKillEaterScore < rhs.m_unKillEaterScore;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int SortRarityEconIdKeysBackpack ( CEconItemView *const *a, CEconItemView *const *b )
{
	Assert( a );
	Assert( *a );
	Assert( b );
	Assert( *b );

	// Sorting by the backpack order doesn't need to check any subproperties like level because
	// every item should have a unique backpack slot already/
	return ExtractBackpackPositionFromBackend( (*a)->GetInventoryPosition() ) < ExtractBackpackPositionFromBackend( (*b)->GetInventoryPosition() )
		? -1
		: 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int SortRarityEconIdKeysAlphabetical_Views ( CEconItemView *const *a, CEconItemView *const *b )
{
	Assert( a );
	Assert( *a );
	Assert( b );
	Assert( *b );

	// First pass -- sort backpack items by user-visible display name.
	// Note: locale-savvy string sorting uses wcscoll, not wcscmp
	int iStrCmpRes = wcscoll( (*a)->GetItemName(), (*b)->GetItemName() );
	if ( iStrCmpRes != 0 )
		return iStrCmpRes;

	// Sort by kill eater score as a last-ditch ordering attempt.
	static CSchemaAttributeDefHandle pAttrDef_KillEaterScore( "kill eater" );

	uint32 unKillEaterScoreA = 0,
		   unKillEaterScoreB = 0;

	(*a)->FindAttribute( pAttrDef_KillEaterScore, &unKillEaterScoreA );
	(*b)->FindAttribute( pAttrDef_KillEaterScore, &unKillEaterScoreB );

	// Our names match so sort by quality for similarly-named items.
	if ( EconQuality_GetRarityScore( (EEconItemQuality)(*a)->GetItemQuality() ) < EconQuality_GetRarityScore( (EEconItemQuality)(*b)->GetItemQuality() ) ||
		 unKillEaterScoreA < unKillEaterScoreB ||
		(*a)->GetItemLevel() < (*b)->GetItemLevel() )
	{
		return -1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int SortRarityEconIdKeysAlphabetical ( const CEquippableItemsForSlotGenerator::CEquippableResult *a, const CEquippableItemsForSlotGenerator::CEquippableResult *b )
{
	return SortRarityEconIdKeysAlphabetical_Views( &a->m_pEconItemView, &b->m_pEconItemView );
}

//-----------------------------------------------------------------------------
static int SortRarityEconIdKeysDate( const CEquippableItemsForSlotGenerator::CEquippableResult *a, const CEquippableItemsForSlotGenerator::CEquippableResult *b )
{
	return ( a->m_pEconItemView->GetID() > b->m_pEconItemView->GetID() ) ? -1 : ( a->m_pEconItemView->GetID() < b->m_pEconItemView->GetID() ) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Purpose: figure out what items should be displayed to the user for a specific
//			loadout and what order they should appear in.
//-----------------------------------------------------------------------------
CEquippableItemsForSlotGenerator::CEquippableItemsForSlotGenerator( int iClass, int iSlot, equip_region_mask_t unUsedEquipRegionMask, unsigned int unFlags )
	: m_pEquippedItemView( NULL )
{
	m_DuplicateCountsMap.SetLessFunc( DefLessFunc( DuplicateCountMap_t::KeyType_t ) );

	// Misc and building slot items have multiple positions they can be assigned to in the loadout
	// screen but internally they're all tagged as "misc slot" items so that's what we search for.
	int iSearchSlot = iSlot;
	if ( GEconItemSchema().GetAccountIndex() == iClass )
	{
		if ( IsQuestSlot( iSearchSlot ) )
		{
			iSearchSlot = ACCOUNT_LOADOUT_POSITION_ACCOUNT1;
		}
	}
	else
	{
		if ( IsMiscSlot( iSearchSlot ) )
		{
			iSearchSlot = LOADOUT_POSITION_MISC;
		}
		else if ( IsBuildingSlot( iSearchSlot ) )
		{
			iSearchSlot = LOADOUT_POSITION_BUILDING;
		}
		else if ( IsTauntSlot( iSearchSlot ) )
		{
			iSearchSlot = LOADOUT_POSITION_TAUNT;
		}
	}
	
	

	// To start with, generate a list of all potentially-useable items that we want to consider for
	// the UI. We'll strip this down based on duplicates/gameplay restrictions and sort it at the end.
	CUtlVector<CEconItemView*> vecItems;
	int iNumItems = TFInventoryManager()->GetAllUsableItemsForSlot( iClass, iSearchSlot, &vecItems );

	CEconItemView *pEquippedItem = NULL;
	
	typedef CUtlMap<RarityEconIdKey, CEquippableItemsForSlotGenerator::CEquippableResult> HighestLevelMap_t;
	HighestLevelMap_t mapHighestLevel;
	SetDefLessFunc( mapHighestLevel );

	// We also prevent items with different kill eater scores from stacking.
	static CSchemaAttributeDefHandle pAttrDef_KillEaterScore( "kill eater" );

	// Iterate over the list of all the items that we consider as potential display candidates.
	for ( int i = 0; i < iNumItems; i++ )
	{
		CEconItemView *pItem = vecItems[i];

		// Before doing any sort of culling, count the number of unique instances of this particular
		// definition.
		{
			const item_stack_type_t stackType( pItem->GetItemDefIndex(), pItem->GetQuality() );
			DuplicateCountMap_t::IndexType_t iIndex = m_DuplicateCountsMap.Find( stackType );
			if ( iIndex == m_DuplicateCountsMap.InvalidIndex() )
			{
				m_DuplicateCountsMap.Insert( stackType, 1 );
			}
			else
			{
				m_DuplicateCountsMap[ iIndex ]++;
			}
		}

		// Track whether this is our currently-equipped item.
		CEconItemView *pCurItemData = TFInventoryManager()->GetItemInLoadoutForClass( iClass, iSlot );
		if ( pCurItemData && pCurItemData->GetItemID() && pCurItemData->GetItemID() == pItem->GetItemID() )
		{
			pEquippedItem = pItem;
		}

		// If this item conflicts with items we already have equipped, we note that so that it shows up
		// differently.
		CEquippableItemsForSlotGenerator::EItemDisplayType eDisplayType = kSlotDisplay_Normal;

		if ( pItem->GetItemDefinition()->GetEquipRegionMask() & unUsedEquipRegionMask )
		{
			eDisplayType = kSlotDisplay_Disabled_EquipRegionConflict;
		}

		// If we're listing *all* items, including duplicates, we just add everything to the list at once and
		// move on. We still do the above equipped-item specialcasing.
		if ( unFlags & kSlotGenerator_ShowDuplicates )
		{
			m_vecDisplayItems.AddToTail( CEquippableItemsForSlotGenerator::CEquippableResult( pItem, eDisplayType ) );
			continue;
		}

		// Has this item been modified by the user in some way? If so, always list.
		if ( ShouldItemNotStack( pItem ) )
		{
			m_vecDisplayItems.AddToTail( CEquippableItemsForSlotGenerator::CEquippableResult( pItem, eDisplayType ) );
			continue;
		}

		// Throw this item into the running map of the highest level item of this type we've seen so far.
		// "Of this type" means "has a matching rarity and item definition index", so uniques will be sorted
		// differently from unusuals, but both will show their highest-level item.
		//
		// This code does make the assumption that nothing in the key will be able to affect the display type.
		// (ie., a higher-level item will never be equippable where a lower-level item is not)
		{
			uint32 unKillEaterScore = 0;
			pItem->FindAttribute( pAttrDef_KillEaterScore, &unKillEaterScore );

			RarityEconIdKey keyEconId( pItem->GetItemQuality(), pItem->GetItemDefIndex(), unKillEaterScore );
			HighestLevelMap_t::IndexType_t iIndex = mapHighestLevel.Find( keyEconId );
			if ( iIndex == mapHighestLevel.InvalidIndex() || pItem->GetItemLevel() > mapHighestLevel[iIndex].m_pEconItemView->GetItemLevel() )
			{
				mapHighestLevel.InsertOrReplace( keyEconId, CEquippableItemsForSlotGenerator::CEquippableResult( pItem, eDisplayType ) );
			}
		}
	}

	// Take the resulting list of items we've force-added and items we've taken the highest-level representative
	// from and put them all into our unsorted display list.
	FOR_EACH_MAP( mapHighestLevel, i )
	{
		m_vecDisplayItems.AddToTail( mapHighestLevel[i] );
	}

	// Always leave a base item in the list. We don't have to worry about level changes or other weird overlaps
	// for base weapons. If we don't have an equipped item, this must be the equipped item.
	CEconItemView *pBaseItem = TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
	if ( pBaseItem && pBaseItem->IsValid() )
	{
		if ( !pEquippedItem )
		{
			pEquippedItem = pBaseItem;
		}
		m_vecDisplayItems.AddToTail( pBaseItem );
	}

	// If the equipped item is in the list, remove it if we're going to manually add it at the top of the display
	// list later. We add it to the list above and remove it here to make sure that we don't get weird effects
	// when the equipped item would be the highest level example of an item, etc.
	if ( pEquippedItem )
	{
		if ( unFlags & kSlotGenerator_EquippedSpecialHandling )
		{
			m_vecDisplayItems.FindAndFastRemove( pEquippedItem );
			m_pEquippedItemView = pEquippedItem;
		}
		// Special case adding our equipped item even if it doesn't meet the ordinary display criteria. It might
		// not be the highest level or rarity, but the user equipped it somehow so make sure it shows up.
		else if ( m_vecDisplayItems.Find( pEquippedItem ) == -1 )
		{
			m_vecDisplayItems.AddToTail( pEquippedItem );
		}
	}

	// Remove duplicates and sort to put it into the order they'll be displayed to the user.
	// Sort the items based on selection type
	int iSortType = tf_item_selection_panel_sort_type.GetInt();
	switch ( iSortType )
	{
		case 0:			m_vecDisplayItems.Sort( &SortRarityEconIdKeysDate );					break;
		case 1:			m_vecDisplayItems.Sort( &SortRarityEconIdKeysAlphabetical );			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
equip_region_mask_t GenerateEquipRegionConflictMask( int iClass, int iUpToSlot, int iIgnoreSlot )
{
	Assert( iUpToSlot <= CLASS_LOADOUT_POSITION_COUNT );

	equip_region_mask_t unEquippedRegionMask = 0;
	for ( int i = 0; i < iUpToSlot; i++ )
	{
		if ( i == iIgnoreSlot )
			continue;

		const CEconItemView *pEquippedItemView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i );
		if ( !pEquippedItemView )
			continue;

		unEquippedRegionMask |= pEquippedItemView->GetItemDefinition()->GetEquipRegionConflictMask();
	}

	return unEquippedRegionMask;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEquipSlotItemSelectionPanel::UpdateModelPanelsForSelection( void )
{
	Assert( !DisplayOnlyAllowUniqueQualityCheckbox() );

	const bool bShowEquippedItemFirst = true;

	CEquippableItemsForSlotGenerator::EquippableResultsVec_t vecDisplayItems;
	const wchar_t* wscFilter = m_wNameFilter.Count() ? m_wNameFilter.Base() : NULL;

	// Generate a mask that is "every equip region we're using except for whatever is in this current slot" and use
	// that to generate a list of everything we could possibly equip for this current slot.
	const equip_region_mask_t unUsedEquipRegionMask = GenerateEquipRegionConflictMask( m_iClass, CLASS_LOADOUT_POSITION_COUNT, m_iSlot );

	CEquippableItemsForSlotGenerator equippableItems( m_iClass,
													m_iSlot,
													unUsedEquipRegionMask,
													bShowEquippedItemFirst ? CEquippableItemsForSlotGenerator::kSlotGenerator_EquippedSpecialHandling : CEquippableItemsForSlotGenerator::kSlotGenerator_None );


	if( m_bShowingEntireBackpack )
	{
		int iNumItems = TFInventoryManager()->GetLocalTFInventory()->GetMaxItemCount();
		for ( int i = 1; i <= iNumItems; i++ )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetItemByBackpackPosition(i);
			if ( pItemData && pItemData->IsValid() )
			{
				if( !DoesItemPassSearchFilter( pItemData->GetDescription(), wscFilter ) )
					continue;

				CEquippableItemsForSlotGenerator::CEquippableResult& result = vecDisplayItems[ vecDisplayItems.AddToTail( pItemData ) ];
				result.m_eDisplayType = GetItemNotSelectableReason( pItemData ) ? CEquippableItemsForSlotGenerator::kSlotDisplay_Invalid : CEquippableItemsForSlotGenerator::kSlotDisplay_Normal;
			}
		}
	}
	else
	{
		// Copy the generated data back into our local structures.
		DeepCopyMap( equippableItems.GetDuplicateCountMap(), &m_DuplicateCounts );

		const CEquippableItemsForSlotGenerator::EquippableResultsVec_t& allDisplayItems = equippableItems.GetDisplayItems();
		for ( int i=0; i<allDisplayItems.Count(); ++i )
		{
			if ( DoesItemPassSearchFilter( allDisplayItems[i].m_pEconItemView->GetDescription(), wscFilter ) )
			{
				vecDisplayItems.AddToTail( allDisplayItems[i] );
			}
		}
	}

	CEconItemView *pEquippedItem = equippableItems.GetEquippedItem();
	if ( pEquippedItem )
	{
		if ( bShowEquippedItemFirst && DoesItemPassSearchFilter( pEquippedItem->GetDescription(), wscFilter ) )
		{
			vecDisplayItems.AddToHead( pEquippedItem );
		}

		m_iCurrentItemID =  pEquippedItem->GetItemID();
	}

	// If the loadout slot is one that players can have empty, put a "nothing" entry at the end of the list. 
	if ( !TFInventoryManager()->SlotContainsBaseItems( GEconItemSchema().GetEquipTypeFromClassIndex( m_iClass ), m_iSlot ) )
	{
		vecDisplayItems.AddToHead( NULL );
	}

	m_iItemsInSelection = vecDisplayItems.Count();

	// Make sure we're not on an invalid page.
	if ( GetCurrentPage() >= GetNumPages() )
	{
		SetCurrentPage( 0 );
	}

	int nOldSelection = GetFirstSelectedItemIndex( true );
	int nPageStart = GetCurrentPage() * GetNumSlotsPerPage();
	nOldSelection += nPageStart;

	static ConVarRef joystick( "joystick" );
	if ( joystick.IsValid() && joystick.GetBool() )
	{
		if( nOldSelection == -1 || nOldSelection >= vecDisplayItems.Count() )
			nOldSelection = nPageStart;
	}


	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		int iItemIndex = i + nPageStart;

		// We only show the equipped state for the equipped items, below
		m_pItemModelPanels[i]->SetShowEquipped( false );
		m_pItemModelPanels[i]->SetShowGreyedOutTooltip( true );
		m_pItemModelPanels[i]->SetGreyedOut( NULL );
		m_pItemModelPanels[i]->SetNoItemText( "#SelectNoItemSlot" );
		bool bSelected = joystick.IsValid() && joystick.GetBool() && iItemIndex == nOldSelection;
		m_pItemModelPanels[i]->SetSelected( bSelected );
		m_pItemModelPanels[i]->SetShowQuantity( true );
		m_pItemModelPanels[i]->SetForceShowEquipped( false );

		bool bShowEquipped = false;
		if ( vecDisplayItems.Count() > iItemIndex )
		{
			const char* pszGreyOutReason = NULL;
			if( m_bShowingEntireBackpack )
			{
				pszGreyOutReason = GetItemNotSelectableReason( vecDisplayItems[iItemIndex].m_pEconItemView );
			}
			else
			{
				pszGreyOutReason = vecDisplayItems[iItemIndex].m_eDisplayType == CEquippableItemsForSlotGenerator::kSlotDisplay_Normal ? NULL : "#Econ_GreyOutReason_EquipRegionConflict";
			}
			// Show equipped state on base items too
			bShowEquipped = false;
			// Check if this item is already equipped, potentially in another slot
			if ( vecDisplayItems[iItemIndex].m_pEconItemView )
			{
				bShowEquipped |= vecDisplayItems[iItemIndex].m_pEconItemView->IsEquippedForClass( m_iClass );
			}
			
			// Check if this item is the currently equipped item
			if ( pEquippedItem )
			{
				bShowEquipped |= pEquippedItem == vecDisplayItems[iItemIndex].m_pEconItemView;
			}

			m_pItemModelPanels[i]->SetForceShowEquipped( bShowEquipped );
			m_pItemModelPanels[i]->SetItem( vecDisplayItems[iItemIndex].m_pEconItemView );
			m_pItemModelPanels[i]->SetGreyedOut( pszGreyOutReason );
		}
		else
		{
			m_pItemModelPanels[i]->SetItem( NULL );
		}

		SetBorderForItem( m_pItemModelPanels[i], false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEquipSlotItemSelectionPanel::GetItemNotSelectableReason( const CEconItemView *pItem ) const
{
	if ( !pItem )
		return NULL;

	CTFItemDefinition *pItemData = pItem->GetStaticData();

	if ( !pItemData->CanBeUsedByClass(m_iClass) )
		return "#Econ_GreyOutReason_CannotBeUsedByThisClass";

	extern bool AreSlotsConsideredIdentical( EEquipType_t eEquipType, int iBaseSlot, int iTestSlot );
	if ( !AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItemData->GetLoadoutSlot(m_iClass), m_iSlot ) )
		return "#Econ_GreyOutReason_CannotBeUsedInThisSlot";

	// Should we gray out this item? This will happen if we're coming from the loadout and we have equip region
	// conflicts of some kind.
	const equip_region_mask_t unUsedEquipRegionMask = GenerateEquipRegionConflictMask( m_iClass, CLASS_LOADOUT_POSITION_COUNT, m_iSlot );
	if ( pItemData->GetEquipRegionMask() & unUsedEquipRegionMask )
		return "#Econ_GreyOutReason_EquipRegionConflict";

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEquipSlotItemSelectionPanel::OnBackPressed()
{
	Assert( m_iCurrentItemID != INVALID_ITEM_DEF_INDEX );
	PostMessageSelectionReturned( m_iCurrentItemID );
	OnClose();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::TextEntry *pTextEntry = dynamic_cast<vgui::TextEntry *>( pPanel );
	if ( pTextEntry )
	{
		if ( pTextEntry == m_pNameFilterTextEntry )
		{
			m_wNameFilter.RemoveAll();
			if ( m_pNameFilterTextEntry->GetTextLength() )
			{
				m_wNameFilter.EnsureCount( m_pNameFilterTextEntry->GetTextLength() + 1 );
				m_pNameFilterTextEntry->GetText( m_wNameFilter.Base(), m_wNameFilter.Count() * sizeof(wchar_t) );
				V_wcslower( m_wNameFilter.Base() );
			}
			m_flFilterItemTime = gpGlobals->curtime + 0.5f;
			return;
		}
	}
}


//=====================================================================================================================
// ITEM CRITERIA BASED SELECTION PANEL
//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemCriteriaSelectionPanel::CItemCriteriaSelectionPanel(Panel *parent, const CItemSelectionCriteria *pCriteria, itemid_t pExceptions[], int iNumExceptions ) : CItemSelectionPanel( parent )
{
	m_pCriteria = pCriteria;

	UpdateExceptions( pExceptions, iNumExceptions );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemCriteriaSelectionPanel::UpdateExceptions( itemid_t pExceptions[], int iNumExceptions )
{
	m_Exceptions.Purge();

	for ( int i = 0; i < iNumExceptions; i++ )
	{
		m_Exceptions.AddToTail( pExceptions[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemCriteriaSelectionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	vgui::Label *pLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemSlotLabel") );
	if ( pLabel )
	{
		pLabel->SetVisible( false );
	}
	SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( "#Craft_SelectItemPanel" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemCriteriaSelectionPanel::ShouldItemPanelBeVisible( CItemModelPanel *pPanel, int iPanelIndex )
{
	// First "Empty" panel is always visible.
	return ( pPanel->HasItem() || iPanelIndex == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemCriteriaSelectionPanel::UpdateModelPanelsForSelection( void )
{
	CUtlVector<CEconItemView*> vecDisplayItems;

	CUtlVector<item_stack_type_t> vecDefsFound;

	const wchar_t* wscFilter = m_wNameFilter.Count() ? m_wNameFilter.Base() : NULL;
	
	int iNumItems = TFInventoryManager()->GetLocalTFInventory()->GetMaxItemCount();
	for ( int i = 1; i <= iNumItems; i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetItemByBackpackPosition(i);
		bool bAdd = false;
		if ( pItemData && pItemData->IsValid() )
		{
			// If this is a valid item, we want to show this if we're showing our entire backpack
			// or we doing the tailored list and this item matches.
			if ( m_bShowingEntireBackpack || GetItemNotSelectableReason( pItemData ) == NULL )
			{
				// The item also needs to pass the text search filter as well
				if( DoesItemPassSearchFilter( pItemData->GetDescription(), wscFilter ) )
				{
					bAdd = true;
				}
			}
		}
		// When showing our entire backpack we take everything so long as we arent filtering
		else if( wscFilter == NULL && m_bShowingEntireBackpack )
		{
			bAdd = true;
		}

		if( bAdd )
		{
			// For actual items see if we should stack.  Only do so if we're NOT showing
			// the entire backpack
			if( pItemData && !m_bShowingEntireBackpack )
			{
				// Has this item been modified by the user in some way? If so, always list,
				// but don't add it to our duplicate count, or our "found indices" list.
				item_definition_index_t iDefIndex = pItemData->GetItemDefIndex();
				if ( ShouldItemNotStack( pItemData ) )
				{
					vecDisplayItems.AddToTail( pItemData );
					continue;
				}

				item_stack_type_t stackType( iDefIndex, pItemData->GetQuality() );
				int iIndex = m_DuplicateCounts.Find( stackType );
				if ( iIndex == m_DuplicateCounts.InvalidIndex() )
				{
					m_DuplicateCounts.Insert( stackType, 1 );
				}
				else
				{
					m_DuplicateCounts[ iIndex ]++;
				}

				if ( vecDefsFound.Find( stackType ) != vecDefsFound.InvalidIndex() )
					continue;

				vecDefsFound.AddToTail( stackType );
			}

			vecDisplayItems.AddToTail( pItemData );
		}
	}

	// Sort them alphabetically if not viewing entire backpack
	if( !m_bShowingEntireBackpack )
	{
		vecDisplayItems.Sort( &SortRarityEconIdKeysAlphabetical_Views );
	}

	// Add an "Empty" item to the start
	vecDisplayItems.AddToHead( NULL );

	m_iItemsInSelection = vecDisplayItems.Count();

	// Make sure we're not on an invalid page.
	if ( GetCurrentPage() >= GetNumPages() )
	{
		SetCurrentPage( 0 );
	}

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		int iItemIndex = i + (GetCurrentPage() * GetNumSlotsPerPage());
		if ( vecDisplayItems.Count() > iItemIndex )
		{
			m_pItemModelPanels[i]->SetItem( vecDisplayItems[iItemIndex] );
		}
		else
		{
			m_pItemModelPanels[i]->SetItem( NULL );
		}

		// We only show the equipped state for the equipped items, below
		m_pItemModelPanels[i]->SetShowEquipped( true );
		m_pItemModelPanels[i]->SetShowGreyedOutTooltip( true );
		m_pItemModelPanels[i]->SetGreyedOut( GetItemNotSelectableReason( m_pItemModelPanels[i]->GetItem() ) );
		m_pItemModelPanels[i]->SetNoItemText( "#SelectNoItemSlot" );

		SetBorderForItem( m_pItemModelPanels[i], false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CItemCriteriaSelectionPanel::GetItemNotSelectableReason( const CEconItemView *pItem ) const
{
	if ( !pItem )
		return NULL;

	// Ignore it if it's in our exceptions list
	if ( m_Exceptions.Find( pItem->GetItemID() ) != m_Exceptions.InvalidIndex() )
		return "";

	// Matching all items?
	if ( !m_pCriteria )
		return NULL;

	CTFItemDefinition *pItemData = pItem->GetStaticData();
	return m_pCriteria->BEvaluate( pItemData ) ? NULL : "";
}

//=====================================================================================================================
// CRAFTING SELECTION PANEL
//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCraftingItemSelectionPanel::CCraftingItemSelectionPanel(Panel *parent ) 
	: CItemCriteriaSelectionPanel( parent, NULL, NULL, 0 )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraftingItemSelectionPanel::UpdateOnShow( const CItemSelectionCriteria *pCriteria, bool bForceBackpack, itemid_t pExceptions[], int iNumExceptions )
{
	m_pCriteria = pCriteria;
	UpdateExceptions( pExceptions, iNumExceptions );

	if ( m_bShowingEntireBackpack != bForceBackpack )
	{
		SetCurrentPage( 0 );
		m_bShowingEntireBackpack = bForceBackpack;
		m_bReapplyItemKVs = true;
	}
	m_bForceBackpack = bForceBackpack;

	if ( !m_bForceBackpack )
	{
		SetCurrentPage( 0 );
	}

	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CCraftingItemSelectionPanel::GetItemNotSelectableReason( const CEconItemView *pItem ) const
{
	if ( !pItem )
		return NULL;

	// Must not be marked no-craft
	if ( !pItem->IsUsableInCrafting() )
		return "#Econ_GreyOutReason_ItemNotCraftable";

	// Are we filtering out items of non-unique quality that we might not want to accidentally craft?
	if ( m_pOnlyAllowUniqueQuality && m_pOnlyAllowUniqueQuality->IsSelected() && pItem->GetQuality() != AE_UNIQUE )
		return "#Econ_GreyOutReason_ItemSpecialQuality";

	return BaseClass::GetItemNotSelectableReason( pItem );
}


//=====================================================================================================================
// ACCOUNT SLOT ITEM SELECTION PANEL
//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAccountSlotItemSelectionPanel::CAccountSlotItemSelectionPanel( Panel *pParent, int iSlot, const char *pszTitleToken ) 
	: CEquipSlotItemSelectionPanel( pParent, GEconItemSchema().GetAccountIndex(), iSlot )
	, m_pszTitleToken( pszTitleToken )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAccountSlotItemSelectionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::BaseClass::ApplySchemeSettings( pScheme );

	m_pWeaponLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemSlotLabel") );
	if ( m_pWeaponLabel )
	{
		m_pWeaponLabel->SetVisible( false );
	}

	SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( m_pszTitleToken ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CAccountSlotItemSelectionPanel::GetItemNotSelectableReason( const CEconItemView *pItem ) const
{
	if ( !pItem )
		return NULL;

	CTFItemDefinition *pItemData = pItem->GetStaticData();

	if ( pItemData->GetEquipType() != EEquipType_t::EQUIP_TYPE_ACCOUNT ) 
		return "#Econ_GreyOutReason_CannotBeUsedByThisClass";

	extern bool AreSlotsConsideredIdentical( EEquipType_t eEquipType, int iBaseSlot, int iTestSlot );
	if ( !AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItemData->GetLoadoutSlot(m_iClass), m_iSlot ) )
		return "#Econ_GreyOutReason_CannotBeUsedInThisSlot";

	return NULL;
}