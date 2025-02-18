//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "backpack_panel.h"
#include "item_confirm_delete_dialog.h"
#include "vgui/ISurface.h"
#include "gamestringpool.h"
#include "iclientmode.h"
#include "econ_item_inventory.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/ScalableImagePanel.h"
#include "vgui/IInput.h"
#include "econ/tool_items/tool_items.h"
#include "econ_gcmessages.h"
#include "item_style_select_dialog.h"
#include "econ_item_system.h"
#include "econ_item_tools.h"
#include "econ_ui.h"
#include "gc_clientsystem.h"
#include "econ_store.h"
#include "rtime.h"
#include "econ_item_description.h"
#include "dynamic_recipe_subpanel.h"
#include "item_slot_panel.h"
#include "crate_detail_panels.h"
#include "tf_warinfopanel.h"
#include "character_info_panel.h"
#include "trading_start_dialog.h"
#include "vgui_controls/MenuItem.h"
#include "tf_duckleaderboard.h"
#include "tf_item_inventory.h"
#include "store/store_panel.h"
#include "strange_count_transfer_panel.h"
#include "collection_crafting_panel.h"
#include "halloween_offering_panel.h"
#include "store/v2/tf_store_preview_item2.h"
#include "item_ad_panel.h"
#include "client_community_market.h"
#include "tf_quest_map_panel.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>



ConVar tf_trade_up_use_count( "tf_trade_up_use_count", "3", FCVAR_ARCHIVE | FCVAR_HIDDEN );


void UseConsumableItem( CEconItemView *pItem, vgui::Panel* pParent );

const ItemSortTypeData_t g_BackpackSortTypes[] =
{
	{ "#Backpack_SortBy_Header",		kGCItemSort_NoSort },
	{ "#Backpack_SortBy_Rarity",		kGCItemSort_SortByRarity },
	{ "#Backpack_SortBy_Type",			kGCItemSort_SortByType },
	{ "#Backpack_SortBy_Class",			kTFGCItemSort_SortByClass },
	{ "#Backpack_SortBy_Slot",			kTFGCItemSort_SortBySlot },
	{ "#Backpack_SortBy_Date",			kGCItemSort_SortByDate },
};

// Array of borders for rarities. Three borders for each rarity: Base, Mouseover, and Selected
const char *g_szItemBorders[][5] =
{
	{ "BackpackItemBorder",				"BackpackItemMouseOverBorder",				"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder",				"BackpackItemGreyedOutSelectedBorder"				},		// AE_NORMAL = 0
	{ "BackpackItemBorder_1",			"BackpackItemMouseOverBorder_1",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_1",			"BackpackItemGreyedOutSelectedBorder_1"				},		// AE_RARITY1 = 1
	{ "BackpackItemBorder_2",			"BackpackItemMouseOverBorder_2",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_2",			"BackpackItemGreyedOutSelectedBorder_2"				},		// AE_RARITY2 = 2
	{ "BackpackItemBorder_Vintage",		"BackpackItemMouseOverBorder_Vintage",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Vintage",		"BackpackItemGreyedOutSelectedBorder_Vintage"		},		// AE_VINTAGE = 3
	{ "BackpackItemBorder_3",			"BackpackItemMouseOverBorder_3",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_3",			"BackpackItemGreyedOutSelectedBorder_3"				},		// AE_RARITY3
	{ "BackpackItemBorder_4",			"BackpackItemMouseOverBorder_4",			"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_4",			"BackpackItemGreyedOutSelectedBorder_4"				},		// AE_RARITY4
	{ "BackpackItemBorder_Unique",		"BackpackItemMouseOverBorder_Unique",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Unique",		"BackpackItemGreyedOutSelectedBorder_Unique"		},		// AE_UNIQUE
	{ "BackpackItemBorder_Community",	"BackpackItemMouseOverBorder_Community",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Community",	"BackpackItemGreyedOutSelectedBorder_Community"		},		// AE_COMMUNITY
	{ "BackpackItemBorder_Developer",	"BackpackItemMouseOverBorder_Developer",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Developer",	"BackpackItemGreyedOutSelectedBorder_Developer"		},		// AE_DEVELOPER
	{ "BackpackItemBorder_SelfMade",	"BackpackItemMouseOverBorder_SelfMade",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_SelfMade",		"BackpackItemGreyedOutSelectedBorder_SelfMade"		},		// AE_SELFMADE
	{ "BackpackItemBorder_Customized",	"BackpackItemMouseOverBorder_Customized",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Customized",	"BackpackItemGreyedOutSelectedBorder_Customized"	},		// AE_CUSTOMIZED
	{ "BackpackItemBorder_Strange",		"BackpackItemMouseOverBorder_Strange",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Strange",		"BackpackItemGreyedOutSelectedBorder_Strange"		},		// AE_STRANGE
	{ "BackpackItemBorder_Completed",	"BackpackItemMouseOverBorder_Completed",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Completed",	"BackpackItemGreyedOutSelectedBorder_Completed"		},		// AE_COMPLETED
	{ "BackpackItemBorder_Haunted",		"BackpackItemMouseOverBorder_Haunted",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Haunted",		"BackpackItemGreyedOutSelectedBorder_Haunted"		},		// AE_HAUNTED
	{ "BackpackItemBorder_Collectors",	"BackpackItemMouseOverBorder_Collectors",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_Collectors",	"BackpackItemGreyedOutSelectedBorder_Collectors"	},		// AE_COLLECTORS

	{ "BackpackItemBorder_PaintkitWeapon",	"BackpackItemMouseOverBorder_PaintkitWeapon",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_PaintkitWeapon",	"BackpackItemGreyedOutSelectedBorder_PaintkitWeapon"	},	// AE_Paintkit
	{ "BackpackItemBorder_RarityDefault",	"BackpackItemMouseOverBorder_RarityDefault",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityDefault",	"BackpackItemGreyedOutSelectedBorder_RarityDefault"		}, // AE_RARITY_DEFAULT,
	{ "BackpackItemBorder_RarityCommon",	"BackpackItemMouseOverBorder_RarityCommon",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityCommon",		"BackpackItemGreyedOutSelectedBorder_RarityCommon"		}, // AE_RARITY_COMMON,
	{ "BackpackItemBorder_RarityUncommon",	"BackpackItemMouseOverBorder_RarityUncommon",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityUncommon",	"BackpackItemGreyedOutSelectedBorder_RarityUncommon"	}, // AE_RARITY_UNCOMMON,
	{ "BackpackItemBorder_RarityRare",		"BackpackItemMouseOverBorder_RarityRare",		"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityRare",		"BackpackItemGreyedOutSelectedBorder_RarityRare"		}, // AE_RARITY_RARE,
	{ "BackpackItemBorder_RarityMythical",	"BackpackItemMouseOverBorder_RarityMythical",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityMythical",	"BackpackItemGreyedOutSelectedBorder_RarityMythical"	}, // AE_RARITY_MYTHICAL,
	{ "BackpackItemBorder_RarityLegendary",	"BackpackItemMouseOverBorder_RarityLegendary",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityLegendary",	"BackpackItemGreyedOutSelectedBorder_RarityLegendary"	}, // AE_RARITY_LEGENDARY,
	{ "BackpackItemBorder_RarityAncient",	"BackpackItemMouseOverBorder_RarityAncient",	"BackpackItemSelectedBorder",	"BackpackItemGreyedOutBorder_RarityAncient",	"BackpackItemGreyedOutSelectedBorder_RarityAncient"		}, // AE_RARITY_ANCIENT,
};	

COMPILE_TIME_ASSERT( ARRAYSIZE(g_szItemBorders) == AE_MAX_TYPES );

enum { kNoUserData = -1 };

static bool HasPaint ( const CEconItemView *pEconItemView, const char *, int )
{
	static CSchemaAttributeDefHandle pAttrDef_PaintRGB( "set item tint RGB" );
	static CSchemaAttributeDefHandle pAttrDef_PaintRGB2( "set item tint RGB 2" );

	return pEconItemView->FindAttribute( pAttrDef_PaintRGB )
		|| pEconItemView->FindAttribute( pAttrDef_PaintRGB2 );
}

static bool HasCustomAttribute ( const CEconItemView *pEconItemView, const char *szAttrName, int )
{
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( szAttrName );

	return pAttrDef
		 ? pEconItemView->FindAttribute( pAttrDef )
		 : NULL;
}

static bool HasCustomUserAttribute ( const CEconItemView *pEconItemView, const char *, int iUserData )
{
	Assert( iUserData != kNoUserData );

	CCountUserGeneratedAttributeIterator countIterator;
	pEconItemView->IterateAttributes( &countIterator );

	return countIterator.GetCount() > iUserData;
}	

static bool HasRemovableCustomName ( const CEconItemView *pEconItemView, const char *, int )
{
	if ( !pEconItemView->GetItemDefinition() )
		return false;

	if ( pEconItemView->GetQuality() == AE_UNIQUE && pEconItemView->GetItemDefinition()->GetArmoryDescString() && !V_stricmp( pEconItemView->GetItemDefinition()->GetArmoryDescString(), "stockitem" ) )
		return false;

	return pEconItemView->GetSOCData() && pEconItemView->GetSOCData()->GetCustomName();
}

static bool HasRemovableCustomDesc ( const CEconItemView *pEconItemView, const char *, int )
{
	if ( !pEconItemView->GetItemDefinition() )
		return false;

	if ( pEconItemView->GetQuality() == AE_UNIQUE && pEconItemView->GetItemDefinition()->GetArmoryDescString() && !V_stricmp( pEconItemView->GetItemDefinition()->GetArmoryDescString(), "stockitem" ) )
		return false;

	return pEconItemView->GetSOCData() && pEconItemView->GetSOCData()->GetCustomDesc();
}

enum EItemCustomizationRemoveType
{
	kCustomizationRemove_Paint,
	kCustomizationRemove_Name,
	kCustomizationRemove_Desc,
	kCustomizationRemove_CustomTexture,
	kCustomizationRemove_MakersMark,
	kCustomizationRemove_UniqueCraftIndex,
	kCustomizationRemove_StrangePart,
	kCustomizationRemove_StrangeScores,
	kCustomizationRemove_UpgradeCard,
	kCustomizationRemove_KillStreak,
	kCustomizationRemove_GiftedBy,
	kCustomizationRemove_Festivizer,
};

typedef bool (* HasRefurbishablePropertyFunc_t)( const CEconItemView *pEconItemView, const char *pArg, int iUserData );

void GetCustomDialogToken_PaintName( const CEconItemView *pEconItemView, int iUserData, CUtlConstWideString& out_String )
{
	extern const CEconItemDefinition *GetPaintItemDefinitionForPaintedItem( const IEconItemInterface *pEconItem );

	Assert( iUserData == kNoUserData );

	const CEconItemDefinition *pPaintItemDef = GetPaintItemDefinitionForPaintedItem( pEconItemView );
	if ( !pPaintItemDef )
	{
		out_String = L"";
		return;
	}

	out_String = GLocalizationProvider()->Find( pPaintItemDef->GetItemBaseName() );
}

void GetCustomDialogToken_StrangePartName( const CEconItemView *pEconItemView, int iUserData, CUtlConstWideString& out_String )
{
	extern uint32 GetScoreTypeForKillEaterAttr( const IEconItemInterface *pEconItem, const CEconItemAttributeDefinition *pAttribDef );
	extern const wchar_t *GetLocalizedStringForKillEaterTypeAttr( const CLocalizationProvider *pLocalizationProvider, uint32 unKillEaterEventType );		// return type changed from locchar_t * because the backpack panel only exists on the client

	uint32 unKillEaterBaseType = GetScoreTypeForKillEaterAttr( pEconItemView, GetKillEaterAttr_Type( iUserData ) );
	
	out_String = GetLocalizedStringForKillEaterTypeAttr( GLocalizationProvider(), unKillEaterBaseType );
}

void GetCustomDialogToken_UserAttributeName( const CEconItemView *pEconItemView, int iUserData, CUtlConstWideString& out_String )
{
	Assert( pEconItemView );

	const CEconItemAttributeDefinition *pAttrDef = GetCardUpgradeForIndex( pEconItemView, iUserData );
	if ( !pAttrDef )
	{
		out_String = L"unknown";
		return;
	}

	attrib_value_t attrVal;
	Verify( pEconItemView->FindAttribute( pAttrDef, &attrVal ) );
	CEconAttributeDescription attrDesc( GLocalizationProvider(), pAttrDef, attrVal );
	out_String = attrDesc.GetShortDescription();
}

typedef void (* GetCustomDialogLocalizationTokenFunc_t)( const CEconItemView *pEconItemView, int iUserData, CUtlConstWideString& out_String );

struct RefurbishableProperty
{
	HasRefurbishablePropertyFunc_t m_pFunc;
	GetCustomDialogLocalizationTokenFunc_t m_pGetCustomDialogLocalizationTokenFunc;
	const char *m_szArg;
	const char *m_pszSelectionUILocalizationToken;
	const char *m_szDialogTitle;
	const char *m_szDialogDesc;
	EItemCustomizationRemoveType m_eRemovalType;
	int m_iUserData;
};

// TODO: Add Gifted by Tag here
static RefurbishableProperty g_RemoveableAttributes[] =
{
	{ &HasRemovableCustomName,	NULL,								NULL,					"#RefurbishItem_RemoveNameCombo",			"#RefurbishItem_RemoveNameTitle",			"#RefurbishItem_RemoveName",			kCustomizationRemove_Name,				kNoUserData },	// does this item have a custom name?
	{ &HasRemovableCustomDesc,	NULL,								NULL,					"#RefurbishItem_RemoveDescCombo",			"#RefurbishItem_RemoveDescTitle",			"#RefurbishItem_RemoveDesc",			kCustomizationRemove_Desc,				kNoUserData },	// does this item have a custom description?
	{ &HasPaint,				&GetCustomDialogToken_PaintName,	"set item tint rgb",	"#RefurbishItem_RemovePaintCombo",			"#RefurbishItem_RemovePaintTitle",			"#RefurbishItem_RemovePaint",			kCustomizationRemove_Paint,				kNoUserData },	// is this item painted?
	{ &HasCustomAttribute,		NULL,								"custom texture hi",	"#RefurbishItem_RemoveCustomTextureCombo",	"#RefurbishItem_RemoveCustomTextureTitle",	"#RefurbishItem_RemoveCustomTexture",	kCustomizationRemove_CustomTexture,		kNoUserData },	// does this have a custom texture applied?
	{ &HasCustomAttribute,		NULL,								"makers mark id",		"#RefurbishItem_RemoveMakersMarkCombo",		"#RefurbishItem_RemoveMakersMarkTitle",		"#RefurbishItem_RemoveMakersMark",		kCustomizationRemove_MakersMark,		kNoUserData },	// was this item crafted by a specific dude?
	{ &HasCustomAttribute,		NULL,								"killstreak tier",		"#RefurbishItem_RemoveKillStreakCombo",		"#RefurbishItem_RemoveKillStreakTitle",		"#RefurbishItem_RemoveKillStreak",		kCustomizationRemove_KillStreak,		kNoUserData },	// Killstreak Effect
	{ &HasCustomAttribute,		NULL,								"gifter account id",	"#RefurbishItem_RemoveGifterCombo",			"#RefurbishItem_RemoveGifterTitle",			"#RefurbishItem_RemoveGifter",			kCustomizationRemove_GiftedBy,			kNoUserData },	// Gifted by
	{ &HasCustomAttribute,		NULL,								"is_festivized",		"#RefurbishItem_RemoveFestivizerCombo",		"#RefurbishItem_RemoveFestivizerTitle",		"#RefurbishItem_RemoveFestivizer",		kCustomizationRemove_Festivizer,		kNoUserData },	// Festivizer

	//"gifter account id",		// who gifted us this item? (will also remove "event date")
};

//-----------------------------------------------------------------------------
// Purpose: Look over this weapon to see if it has any strange stat counters to reset optionally.
//-----------------------------------------------------------------------------
static bool HasResettableScoreAttributes ( const CEconItemView *pEconItemView, const char *, int )
{
	if ( !pEconItemView )
		return false;

	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		uint32 unScore;
		if ( pEconItemView->FindAttribute( GetKillEaterAttr_Score( i ), &unScore ) && unScore > 0 )
			return true;
	}

	return false;
}

bool BCanPreviewPaintKit( const CEconItemView* pItem )
{
	if ( !pItem || !pItem->IsValid() )
		return false;

	if ( IsPaintKitTool( pItem->GetItemDefinition() ) )
		return true;

	return IsValidPickupWeaponSlot( pItem->GetItemDefinition()->GetDefaultLoadoutSlot() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetRemovableAttributesCount()
{
	return ARRAYSIZE( g_RemoveableAttributes )
		 + GetKillEaterAttrCount()
		 + GetMaxCardUpgradesPerItem()						// remove card upgrades
		 + 1;												// strange quality item score reset
}

RefurbishableProperty RemovableAttributes_GetAttributeDetails( int i )
{
	Assert( i >= 0 );
	Assert( i < GetRemovableAttributesCount() );

	if ( i < ARRAYSIZE( g_RemoveableAttributes ) )
		return g_RemoveableAttributes[i];
	
	// Which attribute in particular are we looking for?
	int iStrangePartIndex = i - ARRAYSIZE( g_RemoveableAttributes );
	if ( iStrangePartIndex < GetKillEaterAttrCount() )
	{
		int iKillEaterAttrIndex = (GetKillEaterAttrCount() - GetKillEaterAttrCount()) + iStrangePartIndex;

		// if we're looking at strange attributes...
		if ( GetKillEaterAttr_IsUserCustomizable( iKillEaterAttrIndex ) )
		{
			// Common properties for all strange part attributes.
			static RefurbishableProperty sStrangePartProperty = { &HasCustomAttribute, &GetCustomDialogToken_StrangePartName, NULL, "#RefurbishItem_RemoveStrangePartCombo", "#RefurbishItem_RemoveStrangePartTitle", "#RefurbishItem_RemoveStrangePart", kCustomizationRemove_StrangePart, kNoUserData };

			RefurbishableProperty partReturnProp = sStrangePartProperty;
			partReturnProp.m_szArg = GetKillEaterAttr_Score( iKillEaterAttrIndex )->GetDefinitionName();				// ...then we check for the presence of a score attribute if this slot is a strange part...
			partReturnProp.m_iUserData = iKillEaterAttrIndex;

			return partReturnProp;
		}
		
		// ...or the presence of a restriction attribute if this slot is a base slot that might have a filter
		static RefurbishableProperty sStrangeFilterProperty = { &HasCustomAttribute, &GetCustomDialogToken_StrangePartName, NULL, "#RefurbishItem_RemoveStrangeFilterCombo", "#RefurbishItem_RemoveStrangeFilterTitle", "#RefurbishItem_RemoveStrangeFilter", kCustomizationRemove_StrangePart, kNoUserData };

		RefurbishableProperty filterReturnProp = sStrangeFilterProperty;
		filterReturnProp.m_szArg = GetKillEaterAttr_Restriction( iKillEaterAttrIndex )->GetDefinitionName();
		filterReturnProp.m_iUserData = iKillEaterAttrIndex;

		return filterReturnProp;
	}

	// Look for any properties that were user-assigned. We allow users to remove them.
	int iCardUpgradeIndex = iStrangePartIndex - GetKillEaterAttrCount();
	if ( iCardUpgradeIndex < GetMaxCardUpgradesPerItem() )
	{
		// Common properties for all card upgrade attributes.
		static RefurbishableProperty sCardUpgradeProperty = { &HasCustomUserAttribute, &GetCustomDialogToken_UserAttributeName, NULL, "#RefurbishItem_RemoveSpellCombo", "#RefurbishItem_RemoveSpellTitle", "#RefurbishItem_RemoveSpellUpgrade", kCustomizationRemove_UpgradeCard, kNoUserData };

		RefurbishableProperty returnProp = sCardUpgradeProperty;
		// FIX THIS FOR CARDS / SPELLS?
		// returnProp.m_szArg = GetCustomDialogToken_UserAttributeName ?
		returnProp.m_iUserData = iCardUpgradeIndex;

		return returnProp;
	}

	// We might also be trying to reset the strange score counters.
	Assert( iStrangePartIndex == GetKillEaterAttrCount() + GetMaxCardUpgradesPerItem() );
	Assert( i == GetRemovableAttributesCount() - 1 );

	static RefurbishableProperty sStrangeScoreReset = { &HasResettableScoreAttributes, NULL, NULL, "#RefurbishItem_RemoveStrangeScoresCombo", "#RefurbishItem_RemoveStrangeScoresTitle", "#RefurbishItem_RemoveStrangeScores", kCustomizationRemove_StrangeScores, kNoUserData };
	return sStrangeScoreReset;
}

bool RemovableAttributes_DoesAttributeApply( int i, const CEconItemView *pEconItemView )
{
	static CSchemaAttributeDefHandle pAttr_CannotRestore( "cannot restore" );
	if ( pEconItemView->FindAttribute( pAttr_CannotRestore ) )
		return false;

	RefurbishableProperty attr = RemovableAttributes_GetAttributeDetails( i );

	return attr.m_pFunc( pEconItemView, attr.m_szArg, attr.m_iUserData );
}

bool RemovableAttributes_DoAnyAttributesApply( const CEconItemView *pEconItemView )
{
	static CSchemaAttributeDefHandle pAttr_CannotRestore( "cannot restore" );
	if ( pEconItemView->FindAttribute( pAttr_CannotRestore ) )
		return false;

	for ( int i = 0; i < GetRemovableAttributesCount(); i++ )
	{
		if ( RemovableAttributes_DoesAttributeApply( i, pEconItemView ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar cl_showbackpackrarities( "cl_showbackpackrarities", "0", FCVAR_ARCHIVE, "0 = Show no backpack icon border colors. 1 = Show item rarities within the backpack. 2 = Show item rarities only for Market-listable items." );
ConVar cl_show_market_data_on_items( "cl_show_market_data_on_items", "1", FCVAR_ARCHIVE, "0 = Never. 1 = Only when showing borders for Market-listable items. 2 = Always." );

ConVar tf_explanations_backpackpanel( "tf_explanations_backpackpanel", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );

ConVar tf_backpack_page_button_delay( "tf_backpack_page_button_delay", "0.5", FCVAR_ARCHIVE, "Amount of time the mouse cursor needs to hover over the page button to select the page." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBackpackPanel::CBackpackPanel( vgui::Panel *parent, const char *panelName ) : CBaseLoadoutPanel( parent, panelName )
{
	m_nQuickOpenTxn = 0;
	m_pContextMenu = NULL;
	m_pPageButtonKVs = NULL;
	m_mapSeenItems.SetLessFunc( DefLessFunc( itemid_t ) );
	m_bInitializedSeenItems = false;
	m_pNameFilterTextEntry = NULL;
	m_flFilterItemTime = 0.f;
	m_mapFilteringItems.SetLessFunc( DefLessFunc(int) );

	m_pNextPageButton = NULL;
	m_pPrevPageButton = NULL;
	m_pShowExplanationsButton = NULL;
	m_pCurPageLabel = NULL;
	m_pSortByComboBox = NULL;
	m_pShowRarityComboBox = NULL;
	m_pShowBaseItemsCheckbox = NULL;
	m_pDragToNextPageButton = NULL;
	m_pDragToPrevPageButton = NULL;
	m_flPreventDragPageSwitchUntil = 0;
	m_flStartExplanationsAt = 0;

	m_flMouseDownTime = 0;
	m_pItemDraggedFromPanel = NULL;
	m_iDraggedFromPage = 0;
	m_bMouseDownOnItemPanel = false;
	m_bDragging = false;
	m_iMouseDownX = m_iMouseDownY = 0;

	m_pMouseDragItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mousedragitempanel" ) );
	m_pCancelToolButton = NULL;
	m_pCraftButton = NULL;
	m_bShowBaseItems = false;
	m_pConfirmDeleteDialog = NULL;
	m_pToolIcon = NULL;
	m_eSelectionMode = StandardSelection;
	m_nLastToolPage = 0;
	m_pDynamicRecipePanel = NULL;
	m_pItemSlotPanel = NULL;
	m_pStrangeToolPanel = NULL;

	m_nNumActivePages = 0;

	m_pInspectCosmeticPanel = new CTFStorePreviewItemPanel2( this, "Resource/UI/econ/InspectionPanel_Cosmetic.res", "storepreviewitem", NULL );
	m_pCollectionCraftPanel = NULL;
	m_pHalloweenOfferingPanel = NULL;
	m_pMannCoTradePanel = NULL;

	CancelToolSelection();

	ListenForGameEvent( "econ_inventory_connected" );
}

CBackpackPanel::~CBackpackPanel()
{
	if ( m_pPageButtonKVs )
	{
		m_pPageButtonKVs->deleteThis();
		m_pPageButtonKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	if ( !m_pSortByComboBox && UsesRarityControls() )
	{
		m_pSortByComboBox = new vgui::ComboBox( this, "SortByComboBox", 5, false );
		m_pSortByComboBox->AddActionSignalTarget( this );
	}

	LoadControlSettings( GetResFile() );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pNameFilterTextEntry = FindControl<vgui::TextEntry>( "NameFilterTextEntry" );
	if ( m_pNameFilterTextEntry )
	{
		m_pNameFilterTextEntry->AddActionSignalTarget( this );
	}

	m_pCancelToolButton = dynamic_cast<CExButton*>( FindChildByName("CancelApplyToolButton") );
	m_pCraftButton = dynamic_cast<CExButton*>( FindChildByName("CraftButton") );
	m_pToolIcon = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("tool_icon") );

	m_pNextPageButton = dynamic_cast<CExButton*>( FindChildByName("NextPageButton") );
	m_pPrevPageButton = dynamic_cast<CExButton*>( FindChildByName("PrevPageButton") );
	m_pShowExplanationsButton = dynamic_cast<CExButton*>( FindChildByName("ShowExplanationsButton") );
	m_pDragToNextPageButton = dynamic_cast<CExButton*>( FindChildByName("DragToNextPageButton") );
	m_pDragToPrevPageButton = dynamic_cast<CExButton*>( FindChildByName("DragToPrevPageButton") );
	m_pCurPageLabel = dynamic_cast<vgui::Label*>( FindChildByName("CurPageLabel") );
	m_pShowRarityComboBox = dynamic_cast<vgui::ComboBox*>( FindChildByName( "ShowRarityComboBox" ) );
	if ( m_pShowRarityComboBox )
	{
		m_pShowRarityComboBox->AddActionSignalTarget( this );

		m_pShowRarityComboBox->AddItem( "#TF_Backpack_ShowNoBorders", NULL );
		m_pShowRarityComboBox->AddItem( "#TF_Backpack_ShowQualityBorders", NULL );
		m_pShowRarityComboBox->AddItem( "#TF_Backpack_ShowMarketableBorders", NULL );

		m_pShowRarityComboBox->ActivateItemByRow( cl_showbackpackrarities.GetInt() );
	}
	m_pShowBaseItemsCheckbox = dynamic_cast<vgui::CheckButton*>( FindChildByName( "ShowBaseItemsCheckbox" ) );
	if ( m_pShowBaseItemsCheckbox )
	{
		m_pShowBaseItemsCheckbox->AddActionSignalTarget( this );
		m_pShowBaseItemsCheckbox->SetSelected( m_bShowBaseItems );
	}

	m_pMouseDragItemPanel->SetBorder( pScheme->GetBorder("BackpackItemMouseOverBorder") );

	// Setup our combo box
	if ( m_pSortByComboBox )
	{
		m_pSortByComboBox->RemoveAll();
		vgui::HFont hFont = pScheme->GetFont( "HudFontSmallestBold", true );
		m_pSortByComboBox->SetFont( hFont );
		KeyValues *pKeyValues = new KeyValues( "data" );
		for ( int i = 0; i < ARRAYSIZE(g_BackpackSortTypes); i++ )
		{
			pKeyValues->SetInt( "sortby", i );
			m_pSortByComboBox->AddItem( g_BackpackSortTypes[i].szSortDesc, pKeyValues );
		}
		pKeyValues->deleteThis();
		m_pSortByComboBox->ActivateItemByRow( 0 );
		m_pSortByComboBox->GetMenu()->SetNumberOfVisibleItems( ARRAYSIZE(g_BackpackSortTypes) );
	}

	// Create page buttons
	const int nNumMaxPages = GetNumMaxPages();
	for ( int i=m_Pages.Count(); i<nNumMaxPages; ++i )
	{
		EditablePanel *pPage = vgui::SETUP_PANEL( new EditablePanel( this, CFmtStr( "page_%d", i ) ) );
		m_Pages.AddToTail( pPage );
	}

	if ( m_pInspectCosmeticPanel )
	{
		// Force it to load it's scheme now, because it needs to be done before we set it's visibility below
		m_pInspectCosmeticPanel->InvalidateLayout( false, true );
		m_pInspectCosmeticPanel->SetVisible( false );
	}
}

void CBackpackPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "pagebuttons_kv" );
	if ( pItemKV )
	{
		if ( m_pPageButtonKVs )
		{
			m_pPageButtonKVs->deleteThis();
		}
		m_pPageButtonKVs = new KeyValues("pagebuttons_kv");
		pItemKV->CopySubkeys( m_pPageButtonKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::AddNewItemPanel( int iPanelIndex )
{
	BaseClass::AddNewItemPanel( iPanelIndex );

	// Store a position for our new panel
	m_ItemModelPanelPos.AddToTail();
	m_ItemModelPanelPos[iPanelIndex].x = m_ItemModelPanelPos[iPanelIndex].y = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CItemModelPanel *CBackpackPanel::GetItemPanelAtPos( int x, int y )
{
	if ( !m_pItemModelPanels.Count() )
		return NULL;

	int iW = m_pItemModelPanels[0]->GetWide();
	int iH = m_pItemModelPanels[0]->GetTall();
	for ( int i = 0; i < m_ItemModelPanelPos.Count(); i++ )
	{
		if ( (x < m_ItemModelPanelPos[i].x) || (x > (m_ItemModelPanelPos[i].x + iW)) )
			continue;
		if ( (y < m_ItemModelPanelPos[i].y) || (y > (m_ItemModelPanelPos[i].y + iH)) )
			continue;
		return m_pItemModelPanels[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBackpackPanel::GetPageButtonIndexAtPos( int x, int y )
{
	if ( !m_Pages.Count() )
		return -1;

	int iW = m_Pages[0]->GetWide();
	int iH = m_Pages[0]->GetTall();
	for ( int i = 0; i < m_PageButtonPos.Count(); i++ )
	{
		if ( (x < m_PageButtonPos[i].x) || (x > (m_PageButtonPos[i].x + iW)) )
			continue;
		if ( (y < m_PageButtonPos[i].y) || (y > (m_PageButtonPos[i].y + iH)) )
			continue;
		return m_Pages[i]->IsVisible() ? i : -1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Change the text color on the page buttons based on the context of the
//			page they represent.
//-----------------------------------------------------------------------------
void CBackpackPanel::SetPageButtonTextColorBasedOnContents()
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	if ( m_Pages.Count() == 0 )
		return;

	if ( !pScheme )
		return;

	const Color& colorEmpty =  pScheme->GetColor( "TanDarker", Color( 235, 226, 202, 255 ) );
	const Color& colorPartial = Color( 170, 161, 137, 255 );
	const Color& colorFull = pScheme->GetColor( "TanLight", Color( 235, 226, 202, 255 ) );
	const Color& colorSelected = pScheme->GetColor( "TFOrange", Color( 145, 73, 59, 255 ) );

	CUtlVector<int> vecPageCount;
	CUtlVector<int> vecNewPageCount;
	vecPageCount.EnsureCount( m_Pages.Count() );
	vecNewPageCount.EnsureCount( m_Pages.Count() );
	// Initialize to 0
	FOR_EACH_VEC( vecPageCount, i )
	{
		vecPageCount[i] = 0;
		vecNewPageCount[i] = 0;
	}

	CPlayerInventory *pInv = InventoryManager()->GetLocalInventory();
	Assert( pInv );
	// Tally up how many items are on each page
	if ( pInv )
	{
		for ( int i = 0 ; i < pInv->GetItemCount() ; ++i )
		{
			CEconItemView *pItem = pInv->GetItem( i );
			const int nSlot = InventoryManager()->GetBackpackPositionFromBackend( pItem->GetInventoryPosition() ) - 1;
			const int nPage = nSlot / GetNumSlotsPerPage();	 
			if ( nPage >= 0 && nPage < m_Pages.Count() )
			{
				vecPageCount[ nPage ] = vecPageCount[ nPage ] + 1;

				// Unackknowledged items technically are on the 1st page, so dont count them
				if ( m_mapSeenItems.Find( pItem->GetItemID() ) == m_mapSeenItems.InvalidIndex()
					&& IsUnacknowledged( pItem->GetInventoryPosition() ) == false && !m_bShowBaseItems && !HasNameFilter() )
				{
					vecNewPageCount[ nPage ] = vecNewPageCount[ nPage ] + 1;
				}
			}
		}
	}

	// Set the color for each page button
	FOR_EACH_VEC( m_Pages, i )
	{
		const int nNewCount = vecNewPageCount[i];
		const int nCount = vecPageCount[i];
		CExButton* pButton = dynamic_cast<CExButton*>( m_Pages[i]->FindChildByName( "Button" ) );
		if ( pButton )
		{
			Color setColor = colorEmpty;
			const Color& bgColor = GetCurrentPage() == i ? colorSelected : pButton->GetButtonDefaultBgColor();

			if ( nCount == GetNumSlotsPerPage() )
				setColor = colorFull;
			else if ( nCount > 0 )
				setColor = colorPartial;

			pButton->SetSelectedColor( setColor, pButton->GetButtonSelectedBgColor() );
			pButton->SetDefaultColor( setColor, bgColor );
			pButton->SetArmedColor( setColor, pButton->GetButtonArmedBgColor() );
			pButton->SetDepressedColor( setColor, pButton->GetButtonDepressedBgColor() );
		}

		// Show our "NEW!" label if there's any unseen items on that page
		CExLabel* pNew = dynamic_cast<CExLabel*>( m_Pages[i]->FindChildByName( "New" ) );
		if ( pNew )
		{
			pNew->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::MarkItemIDDirty( itemid_t itemID )
{
	if ( m_vecDirtyItems.Find( itemID ) == m_vecDirtyItems.InvalidIndex() )
	{
		m_vecDirtyItems.AddToTail( itemID );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::PositionItemPanel( CItemModelPanel *pPanel, int iIndex )
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
void CBackpackPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		// Viewing the backpack. Layout all the buttons and hide the class image.
		m_pItemModelPanels[i]->SetVisible( true );
		m_pItemModelPanels[i]->SetNoItemText( "#SelectNoItemSlot" );

		PositionItemPanel( m_pItemModelPanels[i], i );

		// Cache off where we put the panel
		m_pItemModelPanels[i]->GetPos( m_ItemModelPanelPos[i].x, m_ItemModelPanelPos[i].y );

		// Take into account parent's position
		Panel* pParent = m_pItemModelPanels[i]->GetParent();
		if( pParent )
		{
			int x = 0,y = 0;
			pParent->GetPos( x, y );
			m_ItemModelPanelPos[i].x += x;
			m_ItemModelPanelPos[i].y += y;
		}
	}

	// adjust page buttons
	{
		m_nNumActivePages = GetNumPages();

		int iCenter = GetWide() * 0.5;

		int iPageBarWidth = 2 * abs( m_iItemBackpackOffcenterX );
		int iPageButtonWidth = ( iPageBarWidth - ( m_iPageButtonPerRow - 1 ) * m_iPageButtonXDelta ) / m_iPageButtonPerRow;
		int iPageButtonWidthPlusDelta = iPageButtonWidth + m_iPageButtonXDelta;
		int iPageButtonHeightPlusDelta = m_iPageButtonHeight + m_iPageButtonYDelta;
		int iStart = iCenter + m_iItemBackpackOffcenterX;

		m_PageButtonPos.EnsureCount( m_Pages.Count() );
		for ( int i=0; i<m_Pages.Count(); ++i )
		{
			EditablePanel *pPage = m_Pages[i];
			if ( pPage )
			{
				// Apply control settings here
				if ( m_pPageButtonKVs )
					pPage->ApplySettings( m_pPageButtonKVs );
				CExButton* pButton = dynamic_cast<CExButton*>( pPage->FindChildByName( "Button" ) );
				pPage->InvalidateLayout( true, true );
				// Make the button have the right command and send it's signals to us
				if ( pButton )
				{
					pButton->SetSelected( false );
					pButton->SetCommand( CFmtStr( "goto_page_%d", i ) );
					pButton->AddActionSignalTarget( this );
				}
				pPage->SetDialogVariable( "page", i+1 );
				
				bool bVisible = i < m_nNumActivePages;
				if ( bVisible )
				{
					int iRow = i /m_iPageButtonPerRow;
					int iColumn = i % m_iPageButtonPerRow;
					pPage->SetBounds( iStart + iColumn * iPageButtonWidthPlusDelta, m_iPageButtonYPos + iRow * iPageButtonHeightPlusDelta, iPageButtonWidth, m_iPageButtonHeight );
					pPage->GetPos( m_PageButtonPos[i].x , m_PageButtonPos[i].y );
				}
				pPage->SetVisible( bVisible );
			}
		}

		// Update colors and the "NEW!" labels
		SetPageButtonTextColorBasedOnContents();
	}

	if ( m_pNextPageButton )
	{
		m_pNextPageButton->SetVisible( true );
	}
	if ( m_pPrevPageButton )
	{
		m_pPrevPageButton->SetVisible( true );
	}
	if ( m_pCurPageLabel )
	{
		m_pCurPageLabel->SetVisible( true );
	}

	if ( m_pSortByComboBox )
	{
		m_pSortByComboBox->SetVisible( !InToolSelectionMode() );
	}
	if ( m_pShowRarityComboBox )
	{
		m_pShowRarityComboBox->SetVisible( true );
	}

	if ( m_pNextPageButton )
	{
		m_pNextPageButton->SetEnabled( GetNumPages() > 1 );
	}
	if ( m_pPrevPageButton )
	{
		m_pPrevPageButton->SetEnabled( GetNumPages() > 1 );
	}

	if ( !m_bDragging )
	{	
		if ( m_pDragToNextPageButton && m_pDragToPrevPageButton )
		{
			m_pDragToNextPageButton->SetVisible( false );
			m_pDragToPrevPageButton->SetVisible( false );
		}
	}

	bool bShowActions = (!m_bItemsOnly && !InToolSelectionMode());
	if ( m_pCraftButton )
	{
		m_pCraftButton->SetVisible( bShowActions );
	}
	if ( m_pCancelToolButton )
	{
		m_pCancelToolButton->SetVisible( InToolSelectionMode() );
	}

	if ( m_pShowExplanationsButton )
	{
		m_pShowExplanationsButton->SetVisible( !m_bItemsOnly );
	}
	if ( m_pShowBaseItemsCheckbox )
	{
		m_pShowBaseItemsCheckbox->SetVisible( !m_bItemsOnly );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::FireGameEvent( IGameEvent *event )
{
	static CSchemaItemDefHandle pItemDef_BasePaintCan( "Paint Can" );
	const char *type = event->GetName();
	if ( Q_strcmp( "econ_inventory_connected", type ) == 0 )
	{
		if ( !m_bInitializedSeenItems )
		{
			CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
			if ( pInventory )
			{
				for ( int i = 0; i < pInventory->GetItemCount(); i++ )
				{
					CEconItemView *pItem = pInventory->GetItem(i);
					m_mapSeenItems.Insert( pItem->GetItemID() );
				}
			}

			m_bInitializedSeenItems = true;
		}

		m_vecPaintCans.Purge();
		m_vecStrangeParts.Purge();
		const CEconItemSchema::ToolsItemDefinitionMap_t &toolDefs = GetItemSchema()->GetToolsItemDefinitionMap();

		// Store all of the active paint can item defs
		FOR_EACH_MAP_FAST( toolDefs, i )
		{
			const CEconItemDefinition *pItemDef = toolDefs[i];
			const IEconTool *pEconTool = pItemDef->GetEconTool();
			if ( !pEconTool )
				continue;

			// Paint can list
			// Ignore the stock paintcan thats only for armory purposes
			if ( !V_strcmp( pEconTool->GetTypeName(), "paint_can" ) && pItemDef_BasePaintCan != pItemDef ) 
			{
				// Paint Can
				m_vecPaintCans.AddToTail( pItemDef->GetDefinitionIndex() );
			}
			// Strange Parts List
			else if ( !V_strcmp( pEconTool->GetTypeName(), "strange_part" ) )
			{
				m_vecStrangeParts.AddToTail( pItemDef->GetDefinitionIndex() );
			}
		}
	}

	BaseClass::FireGameEvent( event );
}

void CBackpackPanel::CheckForQuickOpenKey()
{
	if ( !m_hQuickOpenCrate )
		return;

	// We only want to continue if it's the transaction we're listening for
	if ( EconUI()->GetStorePanel()->GetMostRecentSuccessfulTransactionID() != m_nQuickOpenTxn )
	{
		return;
	}

	CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
	if ( pInventory )
	{
		for ( int i = 0; i < pInventory->GetItemCount(); i++ )
		{
			CEconItemView *pInvItem = pInventory->GetItem( i );

			uint32 iPosition = pInvItem->GetInventoryPosition();
			if ( IsUnacknowledged( iPosition ) == false )
				continue;

			if ( InventoryManager()->GetBackpackPositionFromBackend( iPosition ) != 0 )
				continue;

			// Now make sure we haven't got a clientside saved ack for this item.
			if ( InventoryManager()->HasBeenAckedByClient( pInvItem ) )
				continue;

			// If item is not a drop we want to show the notification otherwise they'll get the notification on death
			int iFoundMethod = GetUnacknowledgedReason( iPosition );
			if ( iFoundMethod != UNACK_ITEM_PURCHASED )
				continue;

			if ( !pInvItem->GetStaticData()->IsTool() )
				continue;

			if( !CEconSharedToolSupport::ToolCanApplyTo( pInvItem, m_hQuickOpenCrate ) )
				continue;

			if ( !pInvItem->GetStaticData()->GetEconTool() )
				continue;

			if ( !Q_strcmp( pInvItem->GetStaticData()->GetEconTool()->GetTypeName(), "decoder_ring" ) == 0 )
				continue;

			ApplyTool( this, pInvItem, m_hQuickOpenCrate );
			CloseStoreStatusDialog();

			m_hQuickOpenCrate = NULL;
			m_nQuickOpenTxn = 0;
			return;
		}
	}

	m_hQuickOpenCrate = NULL;
	m_nQuickOpenTxn = 0;
}

//-----------------------------------------------------------------------------
// Purpose: When the store get's a new transaction ID, it comes here as well
//-----------------------------------------------------------------------------
void CBackpackPanel::SetCurrentTransactionID( uint64 nTxnID )
{
	// If we've got a quick open crate st, and no quick open transaction ID,
	// then we want to capture the incoming transaction ID so that we can
	// compare future incoming successful transactions to see if they have
	// the key we're expecting
	if ( m_hQuickOpenCrate && m_nQuickOpenTxn == 0 )
	{
		m_nQuickOpenTxn = nTxnID;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnShowPanel( bool bVisible, bool bReturningFromArmory )
{
	if ( bVisible )
	{
		m_pMouseDragItemPanel->SetVisible( false );

		if( m_pDynamicRecipePanel )
		{
			m_pDynamicRecipePanel->SetVisible( false );
		}

		if ( m_pItemSlotPanel )
		{
			m_pItemSlotPanel->SetVisible( false );
		}

		m_bShowBaseItems = false;
		if ( m_pShowBaseItemsCheckbox )
		{
			m_pShowBaseItemsCheckbox->SetSelected( m_bShowBaseItems );
		}

		if ( !bReturningFromArmory )
		{
			SetCurrentPage( 0 );
			CancelToolSelection();
		}

		m_nNumActivePages = 0;

	}
	else
	{
		if ( m_bDragging )
		{
			StopDrag( false );
		}
	}

	if ( m_pInspectCosmeticPanel )
	{
		m_pInspectCosmeticPanel->SetVisible( false );
	}

	if ( m_pCollectionCraftPanel )
	{
		m_pCollectionCraftPanel->SetVisible( false );
	}

	if ( m_pHalloweenOfferingPanel )
	{
		m_pHalloweenOfferingPanel->SetVisible( false );
	}

	if ( m_pMannCoTradePanel )
	{
		m_pMannCoTradePanel->SetVisible( false );
	}

	if ( m_pStrangeToolPanel )
	{
		m_pStrangeToolPanel->MarkForDeletion();
		m_pStrangeToolPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::PostShowPanel( bool bVisible )
{
	if ( bVisible )
	{
		DeSelectAllBackpackItemPanels();

		RequestFocus();

		// Clear out text field
		ClearNameFilter( true );
	}

	// If this is the first time we've opened the loadout, start the loadout explanations
	ConVar *pConVar = GetExplanationConVar();
	if ( bVisible && pConVar && !pConVar->GetBool() && ShouldShowExplanations() )
	{
		m_flStartExplanationsAt = Plat_FloatTime() + 0.5;
		vgui::ivgui()->AddTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBackpackPanel::GetNumPages( void )
{
	int iMaxItems = InventoryManager()->GetLocalInventory()->GetMaxItemCount();
	return (int)(ceil((float)iMaxItems / (float)BACKPACK_SLOTS_PER_PAGE));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::AssignItemToPanel( CItemModelPanel *pPanel, int iIndex )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	static int iItemBackpackPos = 0;
	if ( iIndex == 0 )
	{
		iItemBackpackPos = 0;
	}
	int iPanelBackpackPos = GetBackpackPosForPanelIndex(iIndex);

	static int iLastMapItem = -1;

	pPanel->SetShowQuantity( true );

	const wchar_t* wszFilter = GetNameFilter();
	bool bInToolSelection = InToolSelectionMode() && m_ToolSelectionItem.IsValid();

	CEconItemView *pItemData = NULL;
	CEconItemView tempItem;
	if ( m_bShowBaseItems )
	{
		const CEconItemDefinition* pItemDef = NULL;

		const CEconItemSchema::BaseItemDefinitionMap_t& mapItems = GetItemSchema()->GetBaseItemDefinitionMap();
		int iStart = iIndex == 0 ? mapItems.FirstInorder() : mapItems.NextInorder( iLastMapItem );
		for ( int it = iStart; it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
		{
			iLastMapItem = it;

			if ( mapItems[it]->IsBaseItem() && !mapItems[it]->IsHidden() )
			{
				// Instead of linking to this base item definition, link to the definition of what it will become
				// when we customize it.
				CFmtStr fmtStrCustomizedDefName( "Upgradeable %s", mapItems[it]->GetDefinitionName() );
				pItemDef = GetItemSchema()->GetItemDefinitionByName( fmtStrCustomizedDefName.Access() );
				
				// If we don't have an upgradeable version, we assume that we can't upgrade it and link to the base
				// definition instead. We expect this to only happen if the item won't actually be useable for whatever
				// purpose (name tags, etc.). We sanity-check this on the GC.
				if ( !pItemDef )
				{
					pItemDef = mapItems[it];
				}

				tempItem.Init( pItemDef->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );

				// skip this item if the tool cannot be applied to it
				if ( bInToolSelection && !CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, &tempItem ) )
				{
					pItemDef = NULL;
					continue;
				}

				if ( DoesItemPassSearchFilter( tempItem.GetDescription(), wszFilter ) )
				{
					break;
				}
			}

			pItemDef = NULL;
		}

		if ( pItemDef )
		{
			pItemData = &tempItem;

			++iItemBackpackPos;
		}
	}
	else if ( HasNameFilter() )
	{
		int iStart = iIndex == 0 ? m_mapFilteringItems.FirstInorder() : m_mapFilteringItems.NextInorder( iLastMapItem );
		for ( int it = iStart; it != m_mapFilteringItems.InvalidIndex(); it = m_mapFilteringItems.NextInorder( it ) )
		{
			iLastMapItem = it;

			CEconItemView *pItem = m_mapFilteringItems[it];

			// skip this item if the tool cannot be applied to it
			if ( bInToolSelection && !CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, pItem ) )
			{
				continue;
			}

			if ( !DoesItemPassSearchFilter( pItem->GetDescription(), wszFilter ) )
			{
				continue;
			}

			if ( ++iItemBackpackPos != iPanelBackpackPos )
			{
				continue;
			}

			pItemData = pItem;
			break;
		}
	}
	else if ( bInToolSelection )
	{
		CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
		if ( pInventory )
		{
			// Backpack positions start from 1
			Assert( iPanelBackpackPos > 0 && iPanelBackpackPos <= pInventory->GetMaxItemCount() );
			int iStart = iIndex == 0 ? 0 : iLastMapItem + 1;
			for ( int i = iStart; i < pInventory->GetItemCount(); i++ )
			{
				iLastMapItem = i;

				CEconItemView *pItem = pInventory->GetItem(i);

				if ( m_ToolSelectionItem.GetStaticData()->IsTool() )
				{
					if ( !CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, pItem ) )
					{
						continue;
					}
				}
				else
				{
					if ( !pItem->GetStaticData()->IsTool() )
					{
						continue;
					}

					if ( !CEconSharedToolSupport::ToolCanApplyTo( pItem, &m_ToolSelectionItem ) )
					{
						continue;
					}

					if ( ( m_ToolSelectionItem.GetStaticData()->GetCapabilities() & ITEM_CAP_DECODABLE ) && pItem->GetStaticData()->GetEconTool() && ( Q_strcmp( pItem->GetStaticData()->GetEconTool()->GetTypeName(), "decoder_ring" ) != 0 ) )
					{
						continue;
					}
				}

				if ( ++iItemBackpackPos != iPanelBackpackPos )
				{
					continue;
				}

				pItemData = pItem;
				break;
			}
		}
	}
	else
	{
		pItemData = InventoryManager()->GetItemByBackpackPosition( iPanelBackpackPos );
		iItemBackpackPos = iPanelBackpackPos;

		if ( pItemData == NULL && pPanel->GetItem() == NULL )
		{
			return;
		}

		int nDirtyIndex = pItemData ? m_vecDirtyItems.Find( pItemData->GetItemID() ) : m_vecDirtyItems.InvalidIndex();

		if ( pItemData	// Want to put in an item
		  && pPanel->GetItem()	// Panel has an item
		  && pItemData->GetItemID() == pPanel->GetItem()->GetItemID() // That panel has the same item that we want to put in
		  && nDirtyIndex == m_vecDirtyItems.InvalidIndex() ) // And that item is not dirtied.  
		{
			// We dont do anything
			return;
		}

		if ( nDirtyIndex != m_vecDirtyItems.InvalidIndex() )
		{
			m_vecDirtyItems.Remove( nDirtyIndex );
		}
	}

	if ( iItemBackpackPos != iPanelBackpackPos )
	{
		pItemData = NULL;
	}

	pPanel->SetItem( pItemData );

	bool bSeen = true;
	// Have we not seen this item before?
	if ( !m_bShowBaseItems && pItemData && m_mapSeenItems.Find( pItemData->GetItemID() ) == m_mapSeenItems.InvalidIndex() )
	{
		bSeen = false;
	}

	// Show our "NEW!" label if this item hasnt been seen
	CExLabel *pNewPanel = dynamic_cast< CExLabel* >( pPanel->FindChildByName( "New" ) );
	if ( pNewPanel )
	{
		pNewPanel->SetVisible( false );
	}

	pPanel->DirtyDescription();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::ClearNameFilter( bool bUpdateModelPanels )
{
	if ( m_wNameFilter.Count() == 0 )
		return;

	m_wNameFilter.RemoveAll();
	if( m_pNameFilterTextEntry )
	{
		m_pNameFilterTextEntry->SetText( "" );
	}

	if ( bUpdateModelPanels )
	{
		m_flFilterItemTime = gpGlobals->curtime + 0.1f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::UpdateFilteringItems()
{
	m_mapFilteringItems.RemoveAll();

	if ( !HasNameFilter() )
		return;

	CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
	if ( !pInventory )
		return;

	for ( int i = 0; i < pInventory->GetItemCount(); i++ )
	{
		CEconItemView *pItem = pInventory->GetItem(i);

		if ( pItem->GetItemDefinition()->IsHidden() )
			continue;

		int iBackpackPosition = InventoryManager()->GetBackpackPositionFromBackend( pItem->GetInventoryPosition() );
		m_mapFilteringItems.Insert( iBackpackPosition, pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::UpdateModelPanels( void )
{
	tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ );

	UpdateFilteringItems();
	
	// We're showing the backpack. Show all the items in our inventory
	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		m_pItemModelPanels[i]->SetShowEquipped( true );
		m_pItemModelPanels[i]->SetShowGreyedOutTooltip( true );
		AssignItemToPanel( m_pItemModelPanels[i], i );

		if ( !m_pItemModelPanels[i]->HasItem() && m_pItemModelPanels[i]->IsSelected() )
		{
			m_pItemModelPanels[i]->SetSelected( false );
		}

		SetBorderForItem( m_pItemModelPanels[i], false );
	}

	// Clean out.  We just did all the heavy lifting.
	m_vecDirtyItems.Purge();

	if ( InToolSelectionMode() && m_ToolSelectionItem.IsValid() )
	{
		wchar_t	wTemp[256];
		g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find( "BackpackApplyTool" ), 1, m_ToolSelectionItem.GetItemName() );
		SetDialogVariable( "loadoutclass", wTemp );
	}
	else
	{
		SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( "BackpackTitle" ) );
	}

	char szTmp[16];
	V_sprintf_safe( szTmp, "%d/%d", GetCurrentPage()+1, GetNumPages() );
	SetDialogVariable( "backpackpage", szTmp );

	// Now layout again to position our item buttons 
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Mark visited item model panels as seen
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemPanelEntered( vgui::Panel *panel )
{
	if ( m_pContextMenu && m_pContextMenu->IsVisible() )
		return;

	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel )
	{
		// Hide the "NEW!" label
		CExLabel *pNewPanel = dynamic_cast< CExLabel* >( pItemPanel->FindChildByName( "New" ) );
		if ( pNewPanel )
		{
			pNewPanel->SetVisible( false );
		}

		// Mark this item as "seen"
		CEconItemView *pItem = pItemPanel->GetItem();
		if ( pItem )
		{
			if ( m_mapSeenItems.Find( pItem->GetItemID() ) == m_mapSeenItems.InvalidIndex() )
			{
				m_mapSeenItems.Insert( pItem->GetItemID() );
				SetPageButtonTextColorBasedOnContents();
			}
		}
	}

	BaseClass::OnItemPanelEntered( panel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemPanelMousePressed( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && pItemPanel->IsGreyedOut() == false && AllowDragging( pItemPanel ) )
	{
		m_flMouseDownTime = gpGlobals->curtime;
		m_iMouseDownX = m_iMouseDownY = 0;
		m_pItemDraggedFromPanel = pItemPanel;
		m_iDraggedFromPage = GetCurrentPage();
		m_bDragging = false;
		m_bMouseDownOnItemPanel = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle the escape key since it doesn't show up in OnKeyCodePressed
//-----------------------------------------------------------------------------
void CBackpackPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE && InToolSelectionMode() )
	{
		CancelToolSelection();
	}
	else if ( code == KEY_ENTER )
	{
		// Do nothing.  This gets hit frequently when people type in the filter
		// text entry and then hit 'Enter', expecting it to execute the filter.
		// We automatically apply it, so let's just eat 'Enter', which was causing
		// us to activate some button on the main menu.
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles key press events in the backpack
//-----------------------------------------------------------------------------
void CBackpackPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// Ignore key events while the confirm delete dialog is up
	if( m_pConfirmDeleteDialog )
		return;

	// let our parent class handle all the arrow key/dpad stuff
	if( HandleItemSelectionKeyPressed( code ) )
	{
		return;
	}

	// Handle close here, CBasePanel parent doesn't support "DialogClosing" command
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if ( (nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_B) && InToolSelectionMode() )
	{
		CancelToolSelection();
	}
	else if( code == KEY_PAGEDOWN )
	{
		OnCommand( "nextpage" );
	}
	else if( code == KEY_PAGEUP )
	{
		OnCommand( "prevpage" );
	}
	else if ( ( nButtonCode == KEY_XBUTTON_A || code == KEY_ENTER || nButtonCode == STEAMCONTROLLER_A ) )
	{
		if( InToolSelectionMode() )
		{
			HandleToolItemSelection( GetFirstSelectedItem() );
		}
		else
		{
			OpenContextMenu();
		}
	}
	else if ( nButtonCode == KEY_XBUTTON_X || nButtonCode == STEAMCONTROLLER_X )
	{
		if( !InToolSelectionMode() )
		{
			OnCommand( "deleteitem" );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles key press events in the backpack
//-----------------------------------------------------------------------------
void CBackpackPanel::OnKeyCodeReleased( vgui::KeyCode code )
{
	if( ! HandleItemSelectionKeyReleased( code ) )
		BaseClass::OnKeyCodeReleased( code );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnMouseCaptureLost( void )
{
	if ( m_bDragging )
	{
		StopDrag( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnMouseReleased(vgui::MouseCode code)
{
	if ( code == MOUSE_LEFT )
	{
		if ( m_bDragging )
		{
			// When we're dragging, we have mouse capture, so the item panels aren't getting mouse input.
			// We need to find out what item panel we're over, and let it know.
			if ( m_pPrevDragOverItemPanel )
			{
				OnItemPanelMouseReleased( m_pPrevDragOverItemPanel );
			}
			else
			{
				StopDrag( false );
			}
		}
	}

	BaseClass::OnMouseReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnConfirmDelete( KeyValues *data )
{
	// Delete all the selected item
	if ( data )
	{
		int iConfirmed = data->GetInt( "confirmed", 0 );
		if ( iConfirmed )
		{
			for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
			{
				if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
				{
					EconUI()->Gamestats_ItemTransaction( IE_ITEM_DELETED, m_pItemModelPanels[i]->GetItem() );
					InventoryManager()->DropItem( m_pItemModelPanels[i]->GetItem()->GetItemID() );
				}
			}
			DeSelectAllBackpackItemPanels();
		}
	}

	m_pConfirmDeleteDialog = NULL;

	// If we're embedded in the discard item panel, it needs to know we made room. Send a message to our parent that it can catch.
	PostMessage( GetParent(), new KeyValues("ConfirmDlgResult", "confirmed", 2 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		if ( InToolSelectionMode() )
		{
			// They're selecting the item they'd like to apply a tool to
			HandleToolItemSelection( pItemPanel->GetItem() );
		}
		else if ( !m_bDragging )
		{
			// If they're not holding down ctrl, deselect all existing selections
			if ( !vgui::input()->IsKeyDown(KEY_LCONTROL) && !vgui::input()->IsKeyDown(KEY_RCONTROL) )
			{
				DeSelectAllBackpackItemPanels();
			}

			// Quick clicks just select the item
			ToggleSelectBackpackItemPanel( pItemPanel );

			if ( pItemPanel->IsSelected() )
			{
				OpenContextMenu();
			}
		}
		else
		{
			int iPanelIndex = GetBackpackPositionForPanel( pItemPanel );
			if ( !CanDragTo(pItemPanel, iPanelIndex) )
			{
				StopDrag(false);
			}
			else
			{
				StopDrag( true );
				if ( (pItemPanel != m_pItemDraggedFromPanel || m_iDraggedFromPage != GetCurrentPage() ) && m_pMouseDragItemPanel->HasItem() )
				{
					HandleDragTo( pItemPanel, iPanelIndex );
				}
				else if ( m_iDraggedFromPage == GetCurrentPage() )
				{
					m_pItemDraggedFromPanel->SetItem( m_pMouseDragItemPanel->GetItem() );
				}
			}
		}

		m_pItemDraggedFromPanel = NULL;
	}
}


bool GetDecodedByItemDefIndex( const CEconItemView *pItem, uint32 *pDecodedBy = NULL )
{
	static CSchemaAttributeDefHandle pAttrDef_DecodedBy( "decoded by itemdefindex" );

	if ( pDecodedBy )
	{
		return pItem->FindAttribute( pAttrDef_DecodedBy, pDecodedBy );
	}
	else
	{
		return pItem->FindAttribute( pAttrDef_DecodedBy );
	}
}

CEconItemView* GetFirstCompatibleKeyForCrate( const CEconItemView *pItem )
{
	// Check if we have any decoder rings that can be applied onto this
	CPlayerInventory *pInv = InventoryManager()->GetLocalInventory();
	Assert( pInv );
	if ( pInv )
	{
		for ( int i = 0; i < pInv->GetItemCount(); ++i )
		{
			CEconItemView *pInvItem = pInv->GetItem( i );

			if ( pInvItem->GetQuality() == AE_SELFMADE )
				continue;

			if ( pInvItem->GetStaticData()->IsTool() && CEconSharedToolSupport::ToolCanApplyTo( pInvItem, pItem ) && pInvItem->GetStaticData()->GetEconTool() && ( Q_strcmp( pInvItem->GetStaticData()->GetEconTool()->GetTypeName(), "decoder_ring" ) == 0 ) )
			{
				return pInvItem;
			}
		}
	}

	return NULL;
}

bool CanInventoryItemsApplyTo( const CEconItemView *pItem )
{
	// Check if we have any tools that can be applied onto this
	CPlayerInventory *pInv = InventoryManager()->GetLocalInventory();
	Assert( pInv );
	if ( pInv )
	{
		for ( int i = 0 ; i < pInv->GetItemCount() ; ++i )
		{
			CEconItemView *pInvItem = pInv->GetItem( i );
			if ( pInvItem->GetStaticData()->IsTool() && CEconSharedToolSupport::ToolCanApplyTo( pInvItem, pItem ) )
			{
				return true;
			}
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
bool CreateMarketPriceString( item_definition_index_t iDefIndex, wchar_t *pszString, int iBufferSize )
{
	// Get Market Price
	steam_market_gc_identifier_t ident;
	ident.m_unDefIndex = iDefIndex;
	ident.m_unQuality = AE_UNIQUE;			// Get this from default item def?

	const client_market_data_t *pClientMarketData = GetClientMarketData( ident );
	if ( !pClientMarketData )
		return false;

	const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

	// Set that price into the button
	wchar_t pszCurrencyString[kLocalizedPriceSizeInChararacters];
	MakeMoneyString( pszCurrencyString, ARRAYSIZE( pszCurrencyString ), pClientMarketData->m_unLowestPrice, eCurrency );

	wchar_t pszConstructed[kLocalizedPriceSizeInChararacters];
	g_pVGuiLocalize->ConstructString_safe( pszConstructed, g_pVGuiLocalize->Find( "#TF_MarketPrice" ), 1, pszCurrencyString );

	// copy result;
	V_wcsncpy( pszString, pszConstructed, iBufferSize );
	return true;
}
//-----------------------------------------------------------------------------
bool CreateStorePriceString( item_definition_index_t iDefIndex, wchar_t *pszString, int iBufferSize )
{
	// Get Market Price
	steam_market_gc_identifier_t ident;
	ident.m_unDefIndex = iDefIndex;
	ident.m_unQuality = AE_UNIQUE;			// Get this from default item def?

	// Get the price of the item
	const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( iDefIndex );
	if ( !pEntry )
		return false;

	const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

	// Set that price into the button
	wchar_t pszCurrencyString[kLocalizedPriceSizeInChararacters];
	MakeMoneyString( pszCurrencyString, ARRAYSIZE( pszCurrencyString ), pEntry->GetCurrentPrice( eCurrency ), eCurrency );

	wchar_t pszConstructed[kLocalizedPriceSizeInChararacters];
	g_pVGuiLocalize->ConstructString_safe( pszConstructed, g_pVGuiLocalize->Find( "#TF_StorePrice" ), 1, pszCurrencyString );

	// copy result;
	V_wcsncpy( pszString, pszConstructed, iBufferSize );
	return true;
}
//-----------------------------------------------------------------------------
void CBackpackPanel::AddCommerceSubmenus( Menu *pSubMenu, item_definition_index_t iItemDef, const char* pszActionFmt )
{
	wchar_t wPriceListing[256];
	// Store
	if ( CreateStorePriceString( iItemDef, wPriceListing, sizeof( wPriceListing ) ) )
	{
		int nIndex = pSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "%s%s%d", "store_", pszActionFmt, iItemDef ) ), this );
		vgui::MenuItem *pMenuItem = pSubMenu->GetMenuItem( nIndex );
		pMenuItem->SetText( wPriceListing );
		pMenuItem->InvalidateLayout( true, false );
	}
	
	// Market
	if ( CreateMarketPriceString( iItemDef, wPriceListing, sizeof( wPriceListing ) ) )
	{
		int nIndex = pSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "%s%s%d", "market_", pszActionFmt, iItemDef ) ), this );
		vgui::MenuItem *pMenuItem = pSubMenu->GetMenuItem( nIndex );
		pMenuItem->SetText( wPriceListing );
		pMenuItem->InvalidateLayout( true, false );
	}
	else
	{
		int nIndex = pSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "%s%s%d", "market_", pszActionFmt, iItemDef ) ), this );
		vgui::MenuItem *pMenuItem = pSubMenu->GetMenuItem( nIndex );
		pMenuItem->SetText( g_pVGuiLocalize->Find( "#TF_MarketUnavailable" ) );
		pMenuItem->InvalidateLayout( true, false );
	}

}
//-----------------------------------------------------------------------------
void CBackpackPanel::AddPaintToContextMenu( Menu *pPaintSubMenu, item_definition_index_t iPaintDef, bool bAddCommerce )
{
	GameItemDefinition_t * pPaintCanDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinition( iPaintDef ) );
	if ( !pPaintCanDef )
		return;

	wchar_t wBuff[256];
	char cBuff[256];
	V_swprintf_safe( wBuff, L"     %ls", g_pVGuiLocalize->Find( pPaintCanDef->GetItemBaseName() ) );

	char szItemName[256];
	g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( pPaintCanDef->GetItemBaseName() ), szItemName, sizeof( szItemName ) );
	V_sprintf_safe( cBuff, "     %s", szItemName );

	uint32 unPaintRGB0 = 0;
	uint32 unPaintRGB1 = 0;

	static CSchemaAttributeDefHandle pAttrDef_PaintRGB( "set item tint RGB" );
	static CSchemaAttributeDefHandle pAttrDef_PaintRGB2( "set item tint RGB 2" );

	float fRGB = 0.0f;

	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pPaintCanDef, pAttrDef_PaintRGB, &fRGB ) && fRGB != 0.0f )
	{
		unPaintRGB0 = fRGB;

		// We may or may not have a secondary paint color as well. If we don't, we just use the primary
		// paint color to fill both slots.
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pPaintCanDef, pAttrDef_PaintRGB2, &fRGB ) )
		{
			unPaintRGB1 = fRGB;
		}
		else
		{
			unPaintRGB1 = unPaintRGB0;
		}
	}

	if ( !bAddCommerce )
	{
		int nIndex = pPaintSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "paint%d", iPaintDef ) ), this );
		vgui::MenuItem *pMenuItem = pPaintSubMenu->GetMenuItem( nIndex );
		pMenuItem->SetText( wBuff );
		pMenuItem->InvalidateLayout( true, false );

		CItemMaterialCustomizationIconPanel *pCustomPanel = new CItemMaterialCustomizationIconPanel( pMenuItem, "paint" );
		pCustomPanel->SetZPos( -100 );
		pCustomPanel->SetTall( 30 );
		pCustomPanel->SetWide( 30 );
		pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( ( unPaintRGB0 & 0xFF0000 ) >> 16, 0, 255 ), clamp( ( unPaintRGB0 & 0xFF00 ) >> 8, 0, 255 ), clamp( ( unPaintRGB0 & 0xFF ), 0, 255 ), 255 ) );
		pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( ( unPaintRGB1 & 0xFF0000 ) >> 16, 0, 255 ), clamp( ( unPaintRGB1 & 0xFF00 ) >> 8, 0, 255 ), clamp( ( unPaintRGB1 & 0xFF ), 0, 255 ), 255 ) );
	}
	else
	{
		// 
		const char *pszContextMenuBorder = "NotificationDefault";
		const char *pszContextMenuFont = "HudFontMediumSecondary";

		Menu *pSubMenu = new Menu( this, "PaintSubMenu" );
		pSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
		pSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
		int iPos = pPaintSubMenu->AddCascadingMenuItem( cBuff, this, pSubMenu );

		CItemMaterialCustomizationIconPanel *pCustomPanel = new CItemMaterialCustomizationIconPanel( pPaintSubMenu, "paint" );
		pCustomPanel->SetZPos( 100 );
		pCustomPanel->SetPos( 0, iPos * pPaintSubMenu->GetMenuItemHeight() );
		pCustomPanel->SetTall( 30 );
		pCustomPanel->SetWide( 30 );
		pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( ( unPaintRGB0 & 0xFF0000 ) >> 16, 0, 255 ), clamp( ( unPaintRGB0 & 0xFF00 ) >> 8, 0, 255 ), clamp( ( unPaintRGB0 & 0xFF ), 0, 255 ), 255 ) );
		pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( ( unPaintRGB1 & 0xFF0000 ) >> 16, 0, 255 ), clamp( ( unPaintRGB1 & 0xFF00 ) >> 8, 0, 255 ), clamp( ( unPaintRGB1 & 0xFF ), 0, 255 ), 255 ) );

		AddCommerceSubmenus( pSubMenu, iPaintDef, "paint" );
	}
}
//
// Add commerce context options for an item.  Adds 'Store' and 'Market' options if appropriate (and Pricing) other wise just click to use
//
void CBackpackPanel::AddCommerceToContextMenu( Menu *pMenu, const char* pszActionFmt, item_definition_index_t iItemDefIndex, bool bAddMarket, bool bAddStore )
{
	GameItemDefinition_t * pItemDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinition( iItemDefIndex ) );
	if ( !pItemDef )
		return;

	//
	if ( !bAddMarket && !bAddStore )
	{
		int nIndex = pMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "%s%d", pszActionFmt, iItemDefIndex ) ), this );
		vgui::MenuItem *pMenuItem = pMenu->GetMenuItem( nIndex );
		pMenuItem->SetText( g_pVGuiLocalize->Find( pItemDef->GetItemBaseName() ) );
		pMenuItem->InvalidateLayout( true, false );
	}
	else
	{
		// 
		const char *pszContextMenuBorder = "NotificationDefault";
		const char *pszContextMenuFont = "HudFontMediumSecondary";

		Menu *pSubMenu = new Menu( this, "CommerceSubMenu" );
		pSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
		pSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
		pMenu->AddCascadingMenuItem( pItemDef->GetItemBaseName(), this, pSubMenu );

		AddCommerceSubmenus( pSubMenu, iItemDefIndex, pszActionFmt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens a context menu with actions relevant for the passed in item
//-----------------------------------------------------------------------------
void CBackpackPanel::OpenContextMenu()
{
	return;

	CUtlVector<CEconItemView*> vecSelectedItems; 
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->GetItem() )
		{
			vecSelectedItems.AddToTail( m_pItemModelPanels[i]->GetItem() );
		}
	}

	if ( m_pContextMenu )
		delete m_pContextMenu;

	m_pContextMenu = new Menu( this, "ContextMenu" );
	MenuBuilder contextMenuBuilder( m_pContextMenu, this );
	const char *pszContextMenuBorder = "NotificationDefault";
	const char *pszContextMenuFont = "HudFontMediumSecondary";
	m_pContextMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
	m_pContextMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

	if ( vecSelectedItems.Count() == 1 )
	{
		const char *pUserTxnCC = GCClientSystem()->GetTxnCountryCode();
		bool bUserChanceRestricted = pUserTxnCC && !BEconCountryAllowDecodableContainers( pUserTxnCC );

		const CEconItemView *pItem = vecSelectedItems.Head();
		const CTFItemDefinition *pItemDef = pItem->GetStaticData();
		static CSchemaItemDefHandle DuckBadgeItemDef( "Duck Badge" );
		static CSchemaItemDefHandle StrangeCountTransferItemDef( "Strange Count Transfer Tool" );
		static CSchemaItemDefHandle ContrackerItemDef( "Activated Campaign 3 Pass" );

		// Tools of any kind can't be used if they are in escrow.
		static CSchemaAttributeDefHandle pAttrib_ToolEscrowUntil( "tool escrow until date" );
		uint32 unEscrowTime;
		const bool bToolIsInEscrow = pItem->FindAttribute( pAttrib_ToolEscrowUntil, &unEscrowTime )
									&& unEscrowTime > CRTime::RTime32TimeCur();

		const IEconTool *pEconTool = pItem->GetItemDefinition()->GetEconTool();

		const bool bIsTool = pItem->GetStaticData()->IsTool() && (pEconTool != NULL);
		const bool bIsGCConsumable = ( ( pItem->GetStaticData()->GetCapabilities() & ITEM_CAP_USABLE_GC ) != 0 );
		bool bSkipAddTrade = false;		// Hack: We should really ask the tool if the command supplants trade.

		// Tool usage goes first.  The cursor starts on this element, so double-clicks will work like how they used to.
		// Strange Count Transfer
		if ( StrangeCountTransferItemDef == pItem->GetItemDefinition() )
		{
			contextMenuBuilder.AddMenuItem( "#ApplyOnItem", new KeyValues( "Context_OpenStrangeCountTransfer" ), "primaryaction" );
		}
		else if ( pItem->GetStaticData()->IsTool() && pEconTool == NULL )
		{
			// do nothing. not a real tool (basic balloons with color that we don't want to 'remove' the paint)
		}
		else if ( (bIsTool || bIsGCConsumable) && !bToolIsInEscrow && pEconTool->CanBeUsedNow( pItem ) )
		{
			Assert( pEconTool );

			// If user is chance restriction, and this is a decoder_ring, skip its default commands.
			bool bRestricted = bUserChanceRestricted && pEconTool && Q_strcmp( pEconTool->GetTypeName(), "decoder_ring" ) == 0;

			if ( !bRestricted )
			{
				const int nTokens = pEconTool->GetUseCommandCount( pItem );
				for ( int i = 0; i < nTokens; ++i )
				{
					const char *pszToolUsageString = pEconTool->GetUseCommandLocalizationToken( pItem, i );

					// If we didn't have a custom usage string, fall back to a sane default based on whether or
					// not we're a consumable or not.
					if ( !pszToolUsageString )
					{
						pszToolUsageString = bIsGCConsumable ? "#ConsumeItem" : "#ApplyOnItem";
					}

					const char *pszContext = pEconTool->GetUseCommand( pItem, i );
					contextMenuBuilder.AddMenuItem( pszToolUsageString, new KeyValues( pszContext ), "primaryaction" );
				}

				// Hack: We should really ask the tool if the command supplants trade. For now, if we have two
				// things, then one of them is trade, so skip it.
				bSkipAddTrade = nTokens > 1;
			}
		}
		else if ( pItem->GetItemDefinition()->GetCapabilities() & ITEM_CAP_DECODABLE )
		{
			static CSchemaAttributeDefHandle pAttrDef_CanShuffleCrateContents( "can shuffle crate contents" );

			if ( !bUserChanceRestricted && pItem->FindAttribute( pAttrDef_CanShuffleCrateContents ) )
			{
				contextMenuBuilder.AddMenuItem( "#ShuffleContents", new KeyValues( "Context_Shuffle" ), "primaryaction" );
			}

			if ( GetFirstCompatibleKeyForCrate( pItem ) != NULL )
			{
				contextMenuBuilder.AddMenuItem( "#UseKey", new KeyValues( "Context_OpenCrateWithKey" ), "primaryaction" );
			}

		}
		else if ( pItem->GetItemDefinition()->GetCapabilities() & ITEM_CAP_HAS_SLOTS )
		{
			// check if we have at least 1 slot criteria
			static CSchemaAttributeDefHandle pAttrDef_Slot( "item slot criteria 1" );
			if ( pItem->FindAttribute( pAttrDef_Slot ) )
			{	
				contextMenuBuilder.AddMenuItem( "#EditSlots", new KeyValues( "Context_EditSlot" ), "primaryaction" );
			}
		}
		else if ( DuckBadgeItemDef == pItem->GetItemDefinition() )
		{
			contextMenuBuilder.AddMenuItem( "#Duck_ViewLeaderboards", new KeyValues( "Context_OpenDuckLeaderboards" ), "primaryaction" );

			if ( CanInventoryItemsApplyTo( pItem ) )
			{
				contextMenuBuilder.AddMenuItem( "#UseDuckToken", new KeyValues( "Context_ApplyByItem" ), "primaryaction" );
			}
			
			if ( GetDecodedByItemDefIndex( pItem ) )
			{
				contextMenuBuilder.AddMenuItem( "#GetDuckToken", new KeyValues( "Context_GetItemFromStore" ), "primaryaction" );
			}
		}
		else if ( pItem->GetItemDefinition() == ContrackerItemDef )
		{
			contextMenuBuilder.AddMenuItem( "#Context_ConTracker", new KeyValues( "Context_OpenConTracker" ), "primaryaction" );
		}

		// 3D Inspect
		float flInspect = 0;
		static CSchemaAttributeDefHandle pAttrib_CosmeticAllowInspect( "cosmetic_allow_inspect" );
		if ( pItem && pItem->IsValid() && pItem->GetItemDefinition()->CanBackpackInspect() &&
			( BCanPreviewPaintKit( pItem ) || pItem->GetItemDefinition()->GetCollectionReference() != NULL || ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrib_CosmeticAllowInspect, &flInspect ) && flInspect != 0.f ) )
			)
		{
			if ( IsPaintKitTool( pItemDef ) )
			{
				// If they clicked on a paintkit item, let them preview the paintkit on all of the items it supports 
				contextMenuBuilder.AddMenuItem( "#Context_InspectModel", new KeyValues( "Context_PreviewItemsWithPaintkit" ), "primaryaction" );
			}
			else
			{
				contextMenuBuilder.AddMenuItem( "#Context_InspectModel", new KeyValues( "Context_InspectModel" ), "primaryaction" );
			}
		}

		// Add equip sub menu
		{
			Menu *pEquipSubMenu = NULL;
			for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
			{
				if ( !pItemDef->CanBeUsedByClass( iClass ) )
					continue;
			
				if ( pEquipSubMenu == NULL )
				{
					pEquipSubMenu = new Menu( this, "EquipMenu" );
					pEquipSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
					pEquipSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

					contextMenuBuilder.AddCascadingMenuItem( "#Context_Equip", pEquipSubMenu, "primaryaction" );
				}

				const char *pszClassName = NULL;
				switch ( iClass )
				{
					case TF_CLASS_SCOUT: 			pszClassName = "#TF_Class_Name_Scout"; break;
					case TF_CLASS_SNIPER: 			pszClassName = "#TF_Class_Name_Sniper"; break;
					case TF_CLASS_SOLDIER: 			pszClassName = "#TF_Class_Name_Soldier"; break;
					case TF_CLASS_DEMOMAN: 			pszClassName = "#TF_Class_Name_Demoman"; break;
					case TF_CLASS_MEDIC: 			pszClassName = "#TF_Class_Name_Medic"; break;
					case TF_CLASS_HEAVYWEAPONS: 	pszClassName = "#TF_Class_Name_HWGuy"; break;
					case TF_CLASS_PYRO: 			pszClassName = "#TF_Class_Name_Pyro"; break;
					case TF_CLASS_SPY: 				pszClassName = "#TF_Class_Name_Spy"; break;
					case TF_CLASS_ENGINEER: 		pszClassName = "#TF_Class_Name_Engineer"; break;
				}
			
				pEquipSubMenu->AddMenuItem( pszClassName, new KeyValues( "Command", "command", CFmtStr( "equipclass%d", iClass ) ), this );
			}
		}
		
		// For customizable items only
		if ( !pItem->IsTemporaryItem() )
		{
			bool bCanCraftUp = GetCollectionCraftingInvalidReason(pItem, NULL) == NULL;
			bool bCanStatClockTrade = GetCraftCommonStatClockInvalidReason(pItem, NULL) == NULL;
			Menu *pMannCoTradeSubMenu = NULL;

			if ( bCanCraftUp || bCanStatClockTrade )
			{
				pMannCoTradeSubMenu = new Menu(this, "MannCoTradeSubMenu");
				pMannCoTradeSubMenu->SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(pszContextMenuBorder));
				pMannCoTradeSubMenu->SetFont(scheme()->GetIScheme(GetScheme())->GetFont(pszContextMenuFont, IsProportional()));
				contextMenuBuilder.AddCascadingMenuItem("#Context_MannCoTrade", pMannCoTradeSubMenu, "customization");

				if ( bCanCraftUp )
				{
					int nIndex = pMannCoTradeSubMenu->AddMenuItem("", new KeyValues("Command", "command", "Context_CraftUpCollection"), this);
					vgui::MenuItem *pMenuItem = pMannCoTradeSubMenu->GetMenuItem(nIndex);
					pMenuItem->SetText("#Context_TradeUp");
					pMenuItem->InvalidateLayout(true, false);
				}

				if ( bCanStatClockTrade )
				{
					int nIndex = pMannCoTradeSubMenu->AddMenuItem("", new KeyValues("Command", "command", "Context_CraftCommonStatClock"), this);
					vgui::MenuItem *pMenuItem = pMannCoTradeSubMenu->GetMenuItem(nIndex);
					pMenuItem->SetText("#Context_CommonStatClock");
					pMenuItem->InvalidateLayout(true, false);
				}
			}



			// Campaign coin access trades
			static CSchemaAttributeDefHandle pAttrDef_IsOperationPass( "is_operation_pass" );
			if ( pItem->FindAttribute( pAttrDef_IsOperationPass ) )
			{
				Menu *pMannCoCoinTradeSubMenu = new Menu( this, "MannCoTradeSubMenu" );
				pMannCoCoinTradeSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
				pMannCoCoinTradeSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
				contextMenuBuilder.AddCascadingMenuItem( "#Context_MannCoTrade", pMannCoCoinTradeSubMenu, "customization" );

				int nIndex = pMannCoCoinTradeSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", "Context_CraftUpCollection" ), this );
				vgui::MenuItem *pMenuItem = pMannCoCoinTradeSubMenu->GetMenuItem( nIndex );
				pMenuItem->SetText( "#Context_TradeUp" );
				pMenuItem->InvalidateLayout( true, false );

				nIndex = pMannCoCoinTradeSubMenu->AddMenuItem("", new KeyValues("Command", "command", "Context_CraftCommonStatClock"), this);
				pMenuItem = pMannCoCoinTradeSubMenu->GetMenuItem(nIndex);
				pMenuItem->SetText("#Context_CommonStatClock");
				pMenuItem->InvalidateLayout(true, false);
			}

			// Halloween trade up offering.
			// Needs two attrs
			static CSchemaAttributeDefHandle pAttrDef_HalloweenOffering( "allow_halloween_offering" );
			static CSchemaAttributeDefHandle pAttrDef_DeactiveDate( "deactive date" );

			// Check the date
			uint32 unDeactiveDate = 0;
			uint32 unCurrentDate = CRTime::RTime32TimeCur();
			if ( pAttrDef_HalloweenOffering && pItem->FindAttribute( pAttrDef_HalloweenOffering ) && pItem->FindAttribute( pAttrDef_DeactiveDate, &unDeactiveDate ) && unDeactiveDate > unCurrentDate )
			{
				contextMenuBuilder.AddMenuItem( "#Context_HalloweenOffering", new KeyValues( "Context_HalloweenOffering" ), "customization" );
			}

			// Change name
			GameItemDefinition_t * pNameTagDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinitionByName( "Name Tag" ) );
			if ( CEconSharedToolSupport::ToolCanApplyToDefinition( dynamic_cast<const GameItemDefinition_t *>( pNameTagDef ), pItemDef ) )
			{
				contextMenuBuilder.AddMenuItem( "#Context_Rename", new KeyValues( "DoRename" ), "customization" );
			}

			// Change description
			GameItemDefinition_t * pDescTagDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinitionByName( "Description Tag" ) );
			if ( CEconSharedToolSupport::ToolCanApplyToDefinition( dynamic_cast<const GameItemDefinition_t *>( pDescTagDef ), pItemDef ) )
			{
				contextMenuBuilder.AddMenuItem( "#Context_Description", new KeyValues( "DoDescription" ), "customization" );
			}

			// Add paint options sub menu
			if ( m_vecPaintCans.Count() > 0 )
			{
				GameItemDefinition_t * pPaintCanDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinition( m_vecPaintCans[0] ) );
				if ( pPaintCanDef && CEconSharedToolSupport::ToolCanApplyToDefinition( dynamic_cast<const GameItemDefinition_t *>( pPaintCanDef ), pItemDef ) )
				{
					Menu *pPaintSubMenu = NULL;
					pPaintSubMenu = new Menu( this, "PaintSubMenu" );
					pPaintSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
					pPaintSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
					contextMenuBuilder.AddCascadingMenuItem( "#Context_Paint", pPaintSubMenu, "customization" );

					CUtlVector<item_definition_index_t> vecOwnedPaints;
					CUtlVector<item_definition_index_t> vecStorePaints;

					// Find out if the user owns this item or not and place in the proper bucket
					CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();

					FOR_EACH_VEC( m_vecPaintCans, i )
					{	
						if ( pLocalInv && pLocalInv->FindFirstItembyItemDef( m_vecPaintCans[i] ) )
						{
							vecOwnedPaints.AddToTail( m_vecPaintCans[i] );
						}
						else
						{
							vecStorePaints.AddToTail( m_vecPaintCans[i] );
						}
					}

					if ( vecOwnedPaints.Count() > 0 )
					{
						// Add Header and loop
						int nIndex = pPaintSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", "" ), this );
						vgui::MenuItem *pMenuItem = pPaintSubMenu->GetMenuItem( nIndex );
						pMenuItem->SetText( g_pVGuiLocalize->Find( "#TF_Owned" ) );
						pMenuItem->InvalidateLayout( true, false );

						FOR_EACH_VEC( vecOwnedPaints, i )
						{
							AddPaintToContextMenu( pPaintSubMenu, vecOwnedPaints[i], false );
						}
					}

					pPaintSubMenu->AddSeparator();
					if ( vecStorePaints.Count() > 0 )
					{
						// Add Header and loop
						int nIndex = pPaintSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", "" ), this );
						vgui::MenuItem *pMenuItem = pPaintSubMenu->GetMenuItem( nIndex );
						pMenuItem->SetText( g_pVGuiLocalize->Find( "#TF_Commerce" ) );
						pMenuItem->InvalidateLayout( true, false );

						FOR_EACH_VEC( vecStorePaints, i )
						{
							AddPaintToContextMenu( pPaintSubMenu, vecStorePaints[i], true );
						}
					}
				}
			}

			// Strange Parts
			if ( BIsItemStrange( pItem ) )
			{
				Menu *pStrangePartsSubMenu = NULL;
				CUtlVector<item_definition_index_t> vecOwnedParts;
				CUtlVector<item_definition_index_t> vecStoreParts;

				// Find out if the user owns this item or not and place in the proper bucket
				CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
				FOR_EACH_VEC( m_vecStrangeParts, i )
				{
					// Determine if this can be applied
					//GameItemDefinition_t *pStrangePartDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinition( m_vecStrangeParts[i] ) );
					CEconItemView partItemView;
					partItemView.Init( m_vecStrangeParts[i], AE_USE_SCRIPT_VALUE, 1 );
					if ( CEconSharedToolSupport::ToolCanApplyTo( &partItemView, pItem ) )
					{
						// Create menu
						if ( !pStrangePartsSubMenu )
						{
							pStrangePartsSubMenu = new Menu( this, "StrangePartsSubMenu" );
							pStrangePartsSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
							pStrangePartsSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
							contextMenuBuilder.AddCascadingMenuItem( "#Context_StrangeParts", pStrangePartsSubMenu, "customization" );
						}

						if ( pLocalInv && pLocalInv->FindFirstItembyItemDef( m_vecStrangeParts[i] ) )
						{
							vecOwnedParts.AddToTail( m_vecStrangeParts[i] );
						}
						else
						{
							vecStoreParts.AddToTail( m_vecStrangeParts[i] );
						}
					}
				}

				if ( pStrangePartsSubMenu )
				{
					if ( vecOwnedParts.Count() > 0 )
					{
						// Add Header and loop
						int nIndex = pStrangePartsSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", "" ), this );
						vgui::MenuItem *pMenuItem = pStrangePartsSubMenu->GetMenuItem( nIndex );
						pMenuItem->SetText( g_pVGuiLocalize->Find( "#TF_Owned" ) );
						pMenuItem->InvalidateLayout( true, false );

						FOR_EACH_VEC( vecOwnedParts, i )
						{
							AddCommerceToContextMenu( pStrangePartsSubMenu, "strangepart_", vecOwnedParts[i], false, false );
						}
					}

					pStrangePartsSubMenu->AddSeparator();
					if ( vecStoreParts.Count() > 0 )
					{
						// Add Header and loop 
						int nIndex = pStrangePartsSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", "" ), this );
						vgui::MenuItem *pMenuItem = pStrangePartsSubMenu->GetMenuItem( nIndex );
						pMenuItem->SetText( g_pVGuiLocalize->Find( "#TF_Market" ) );
						pMenuItem->InvalidateLayout( true, false );

						FOR_EACH_VEC( vecStoreParts, i )
						{
							AddCommerceToContextMenu( pStrangePartsSubMenu, "strangepart_", vecStoreParts[i], true, false );
						}
					}
				}
			}

			if ( pItem->IsMarketable() )
			{
				contextMenuBuilder.AddMenuItem( "#Context_MarketPlaceSell", new KeyValues( "DoSellMarketplace" ), "economy" );
			}

			// Trade to another player
			if ( pItem->IsTradable() && !bSkipAddTrade )
			{
				contextMenuBuilder.AddMenuItem( "#Context_Trade", new KeyValues( "DoTradeToPlayer" ), "economy" );
			}

			if ( pItem->GetItemDefinition()->GetCapabilities() & ITEM_CAP_CAN_BE_RESTORED )
			{
				if ( RemovableAttributes_DoAnyAttributesApply( pItem ) )
				{
					contextMenuBuilder.AddMenuItem( "#RefurbishItem", new KeyValues( "Context_RefurbishItem" ), "destructive" );
				}
			}
		}
	}
	else
	{
		// Check if ALL selected items can be crafted together
		bool bCanCraftUp = true;
		for( int i=0; i < COLLECTION_CRAFTING_ITEM_COUNT && i < vecSelectedItems.Count(); ++i )
		{
			CEconItemView* pPrevItem = ( i - 1 ) < 0 ? NULL : vecSelectedItems[ i - 1 ];
			bCanCraftUp &= GetCollectionCraftingInvalidReason( vecSelectedItems[ i ], pPrevItem ) == NULL;
		}

		bool bCanStatClockTrade = true;
		for (int i = 0; i < COLLECTION_CRAFTING_ITEM_COUNT && i < vecSelectedItems.Count(); ++i)
		{
			CEconItemView* pPrevItem = (i - 1) < 0 ? NULL : vecSelectedItems[i - 1];
			bCanStatClockTrade &= GetCraftCommonStatClockInvalidReason(vecSelectedItems[i], pPrevItem) == NULL;
		}

		Menu *pMannCoTradeSubMenu = NULL;

		if ( bCanCraftUp || bCanStatClockTrade )
		{
			pMannCoTradeSubMenu = new Menu(this, "MannCoTradeSubMenu");
			pMannCoTradeSubMenu->SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(pszContextMenuBorder));
			pMannCoTradeSubMenu->SetFont(scheme()->GetIScheme(GetScheme())->GetFont(pszContextMenuFont, IsProportional()));
			contextMenuBuilder.AddCascadingMenuItem("#Context_MannCoTrade", pMannCoTradeSubMenu, "customization");

			if (bCanCraftUp)
			{
				int nIndex = pMannCoTradeSubMenu->AddMenuItem("", new KeyValues("Command", "command", "Context_CraftUpCollection"), this);
				vgui::MenuItem *pMenuItem = pMannCoTradeSubMenu->GetMenuItem(nIndex);
				pMenuItem->SetText("#Context_TradeUp");
				pMenuItem->InvalidateLayout(true, false);
			}

			if (bCanStatClockTrade)
			{
				int nIndex = pMannCoTradeSubMenu->AddMenuItem("", new KeyValues("Command", "command", "Context_CraftCommonStatClock"), this);
				vgui::MenuItem *pMenuItem = pMannCoTradeSubMenu->GetMenuItem(nIndex);
				pMenuItem->SetText("#Context_CommonStatClock");
				pMenuItem->InvalidateLayout(true, false);
			}
		}
	}

	if ( !m_bShowBaseItems )
	{
		bool bDeleteAvailable = true;
		// Check that all of the selected items are deletable
		for( int i=0; i < vecSelectedItems.Count() && bDeleteAvailable; ++i )
		{
			static CSchemaAttributeDefHandle pAttrDef_NoDelete( "cannot delete" );
			bDeleteAvailable &= !vecSelectedItems[i]->FindAttribute( pAttrDef_NoDelete );
		}

		// Only show the delete button if every slected item is deletable
		if ( bDeleteAvailable )
		{
			contextMenuBuilder.AddMenuItem( "#TF_SteamWorkshop_Delete", new KeyValues( "DoDelete" ), "destructive" );
		}
	}

	// Position to the cursor's position
	int nX, nY;
	g_pVGuiInput->GetCursorPosition( nX, nY );
	m_pContextMenu->SetPos( nX - 1, nY - 1 );
	
	m_pContextMenu->SetVisible(true);
	m_pContextMenu->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemPanelMouseRightRelease( vgui::Panel *panel )
{
	{
		CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
		if ( pItemPanel && pItemPanel->IsVisible() )
		{
			// If they're not holding down ctrl, deselect all existing selections
			if ( !vgui::input()->IsKeyDown(KEY_LCONTROL) && !vgui::input()->IsKeyDown(KEY_RCONTROL) )
			{
				DeSelectAllBackpackItemPanels();
				ToggleSelectBackpackItemPanel( pItemPanel );
			}
			else if ( AllowSelection() && !pItemPanel->IsGreyedOut() )
			{
				if ( !pItemPanel->IsSelected() && pItemPanel->HasItem() )
				{
					pItemPanel->SetSelected( true );
				}
				SetBorderForItem( pItemPanel, false );
			}

			OpenContextMenu();
		}
	}
}

void CBackpackPanel::OnMouseMismatchedRelease( MouseCode code, Panel* pPressedPanel )
{
	if ( pPressedPanel )
	{
		OnMouseReleased( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::StartDrag( int x, int y )
{
	// don't allow item drag if there's a filter
	if ( HasNameFilter() )
		return;

	m_bDragging = true;
	HideMouseOverPanel();

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pMouseDragItemPanel->SetItem( m_pItemDraggedFromPanel->GetItem() );
	m_pMouseDragItemPanel->InvalidateLayout( true );

	m_pItemDraggedFromPanel->Dragged( true );

	// Calculate the mouse offset from the top left of the panel we're going to drag
	m_iDragOffsetX = m_pMouseDragItemPanel->GetWide() * 0.5f;
	m_iDragOffsetY = m_pMouseDragItemPanel->GetTall() * 0.5f;
	m_flPreventDragPageSwitchUntil = 0;

	m_pMouseDragItemPanel->SetVisible( true );

	m_pItemDraggedFromPanel->SetItem( NULL );
	SetBorderForItem( m_pItemDraggedFromPanel, false );
	m_pPrevDragOverItemPanel = NULL;

	vgui::input()->SetMouseCapture( GetVPanel() );

	if ( m_pDragToNextPageButton && m_pDragToPrevPageButton )
	{
		m_pDragToNextPageButton->SetVisible( GetNumPages() > 1 );
		m_pDragToPrevPageButton->SetVisible( GetNumPages() > 1 );
	}

	// play pickup sound
	CEconItemView *item = m_pMouseDragItemPanel->GetItem();
	if ( item )
	{
		const char *soundFilename = item->GetDefinitionString( "pickup_sound", "" );
		if ( soundFilename[0] )
		{
			vgui::surface()->PlaySound( soundFilename );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::StopDrag( bool bSucceeded )
{
	if ( !m_pItemDraggedFromPanel )
		return;

	if ( !bSucceeded )
	{
		if ( m_iDraggedFromPage == GetCurrentPage() )
		{
			m_pItemDraggedFromPanel->SetItem( m_pMouseDragItemPanel->GetItem() );
		}
		m_pItemDraggedFromPanel = NULL;
	}

	m_pMouseDragItemPanel->SetVisible( false );
	m_bDragging = false;

	vgui::input()->SetMouseCapture( NULL );

	if ( m_pDragToNextPageButton && m_pDragToPrevPageButton )
	{
		m_pDragToNextPageButton->SetVisible( false );
		m_pDragToPrevPageButton->SetVisible( false );
	}

	// play drop sound
	CEconItemView *item = m_pMouseDragItemPanel->GetItem();
	if ( item )
	{
		const char *soundFilename = item->GetDefinitionString( "drop_sound", "ui/item_default_drop.wav" );
		vgui::surface()->PlaySound( soundFilename );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBackpackPanel::GetBackpackPositionForPanel( CItemModelPanel *pItemPanel )
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
void CBackpackPanel::HandleDragTo( CItemModelPanel *pItemPanel, int iPanelIndex )
{
	// Find the position based on the panel we're dragging to
	if ( iPanelIndex != -1 )
	{
		// If the current panel is selected, unselect it
		if ( m_pItemModelPanels[iPanelIndex]->IsSelected() )
		{
			ToggleSelectBackpackItemPanel( m_pItemModelPanels[iPanelIndex] );
		}
		if ( m_pItemDraggedFromPanel->IsSelected() )
		{
			ToggleSelectBackpackItemPanel( m_pItemDraggedFromPanel );
		}

		// We "move" the items in the backpack immediately, because when the messages come back 
		// from steam they'll fix the positions if the move fails for some reason.
		CEconItemView *pItem = NULL;
		if ( m_pItemModelPanels[iPanelIndex]->HasItem() )
		{
			// We need to copy it because it's about to get stomped by the other item
			pItem = new CEconItemView( *m_pItemModelPanels[iPanelIndex]->GetItem() );
		}
		m_pItemModelPanels[iPanelIndex]->SetItem( m_pMouseDragItemPanel->GetItem() );

		if ( m_iDraggedFromPage == GetCurrentPage() )
		{
			m_pItemDraggedFromPanel->SetItem( pItem );
		}

		if ( pItem )
		{
			delete pItem;
		}

		// Tell the inventory to move the item
		// Translate it to the right page
		int iBackpackPosition = GetBackpackPosForPanelIndex( iPanelIndex );
		InventoryManager()->MoveItemToBackpackPosition( m_pMouseDragItemPanel->GetItem(), iBackpackPosition );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::OnTick( void )
{
	BaseClass::OnTick();

	bool bNeedsTick = false;
	if ( m_flStartExplanationsAt && m_flStartExplanationsAt < Plat_FloatTime() )
	{
		m_flStartExplanationsAt = 0;

		if ( ShouldShowExplanations() )
		{
			ConVar *pConVar = GetExplanationConVar();
			if ( pConVar )
			{
				pConVar->SetValue( 1 );
			}

			CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( FindChildByName("StartExplanation") );
			if ( pPopup )
			{
				pPopup->Popup();
			}
		}
	}
	else
	{
		bNeedsTick = true;
	}

	// To handle page movement while holding the mouse still over the page buttons, 
	// we need to keep calling OnCursorMoved() whenever we're dragging.
	if ( m_bDragging && m_pMouseDragItemPanel && m_pItemDraggedFromPanel && IsVisible() )
	{
		int mx,my;
		vgui::input()->GetCursorPos( mx, my );
		ScreenToLocal( mx, my );
		OnCursorMoved( mx,my );

		bNeedsTick = true;
	}

	if ( !bNeedsTick && !NeedsDerivedTickSignal() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_flFilterItemTime > 0 && gpGlobals->curtime >= m_flFilterItemTime )
	{
		SetCurrentPage( 0 );
		DeSelectAllBackpackItemPanels();
		UpdateModelPanels();

		m_flFilterItemTime = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::OnCursorMoved( int x, int y )
{
	if ( !m_pItemDraggedFromPanel )
		return;

	if ( m_bDragging && m_pMouseDragItemPanel )
	{
		m_pMouseDragItemPanel->SetPos( x - m_iDragOffsetX, y - m_iDragOffsetY );

		// When we're dragging, we have mouse capture, so the item panels aren't getting mouse input.
		// We need to find out what item panel we're over, and let it know.

		if ( m_flPreventDragPageSwitchUntil < Plat_FloatTime() )
		{
			// First, are we over the page turning areas?
			bool bDragNext = false;
			if ( m_pDragToNextPageButton && m_pDragToNextPageButton->IsVisible() )
			{
				int iDragX, iDragY;
				m_pDragToNextPageButton->GetPos( iDragX, iDragY );
				bDragNext = ( x >= iDragX && x <= (iDragX + m_pDragToNextPageButton->GetWide() ) );
			}
			if ( !bDragNext && m_pNextPageButton && m_pNextPageButton->IsEnabled() )
			{
				int iDragX, iDragY;
				m_pNextPageButton->GetPos( iDragX, iDragY );
				bDragNext = ( x >= iDragX && x <= (iDragX + m_pNextPageButton->GetWide()) && y >= iDragY && y <= (iDragY + m_pNextPageButton->GetTall()) );
			}
			if ( bDragNext )
			{
				OnCommand( "nextpage" );
				m_flPreventDragPageSwitchUntil = Plat_FloatTime() + tf_backpack_page_button_delay.GetFloat();
				return;
			}

			bool bDragPrev = false;
			if ( m_pDragToPrevPageButton && m_pDragToPrevPageButton->IsVisible() )
			{
				int iDragX, iDragY;
				m_pDragToPrevPageButton->GetPos( iDragX, iDragY );
				bDragPrev = ( x >= iDragX && x <= (iDragX + m_pDragToPrevPageButton->GetWide() ) );
			}
			if ( !bDragPrev && m_pPrevPageButton && m_pPrevPageButton->IsEnabled() )
			{
				int iDragX, iDragY;
				m_pPrevPageButton->GetPos( iDragX, iDragY );
				bDragPrev = ( x >= iDragX && x <= (iDragX + m_pPrevPageButton->GetWide()) && y >= iDragY && y <= (iDragY + m_pPrevPageButton->GetTall()) );
			}
			if ( bDragPrev )
			{
				OnCommand( "prevpage" );
				m_flPreventDragPageSwitchUntil = Plat_FloatTime() + tf_backpack_page_button_delay.GetFloat();
				return;
			}
		}

		// check if we're hovering page button
		static int iPrevHoveringPage = -1;
		static float flLastPageButtonEnterTime = 0.f;
		int iHoveringPage = GetPageButtonIndexAtPos( x, y );
		if ( iHoveringPage != -1 )
		{
			if ( iHoveringPage == GetCurrentPage() )
			{
				iPrevHoveringPage = -1;
				flLastPageButtonEnterTime = 0.f;
			}
			else if ( iPrevHoveringPage != iHoveringPage )
			{
				iPrevHoveringPage = iHoveringPage;
				flLastPageButtonEnterTime = Plat_FloatTime() + tf_backpack_page_button_delay.GetFloat();
			}
			else if ( flLastPageButtonEnterTime > 0 && flLastPageButtonEnterTime < Plat_FloatTime() )
			{
				flLastPageButtonEnterTime = 0.f;
				SetCurrentPage( iHoveringPage );
				UpdateModelPanels();
			}
			return;
		}
		else
		{
			// reset hovering page buttons data
			iPrevHoveringPage = -1;
			flLastPageButtonEnterTime = 0.f;
		}

		CItemModelPanel *pOverPanel = GetItemPanelAtPos( x, y );
		if ( m_pPrevDragOverItemPanel != pOverPanel )
		{
			if ( m_pPrevDragOverItemPanel )
			{
				OnItemPanelExited( m_pPrevDragOverItemPanel );
			}

			m_pPrevDragOverItemPanel = pOverPanel;

			if ( m_pPrevDragOverItemPanel )
			{
				OnItemPanelEntered( m_pPrevDragOverItemPanel );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemPanelCursorMoved( int x, int y )
{
	if ( !m_pItemDraggedFromPanel )
		return;

	if ( !m_bDragging && m_pItemDraggedFromPanel->HasItem() && !InToolSelectionMode() )
	{
		// Don't drag instantly, so it's easy to select
		if ( (gpGlobals->curtime - m_flMouseDownTime) > 0.3 )
		{
			StartDrag( x,y );
		}
		else
		{
			if ( !m_iMouseDownX )
			{
				m_iMouseDownX = x;
				m_iMouseDownY = y;
			}
			else if ( abs(m_iMouseDownX - x) > XRES(10) || abs(m_iMouseDownY - y) > YRES(10) )
			{
				StartDrag( x,y );
			}
		}
	}

	OnCursorMoved( x, y );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::ToggleSelectBackpackItemPanel( CItemModelPanel *pPanel )
{
	if ( !AllowSelection() || pPanel->IsGreyedOut() )
		return;

	if ( pPanel->IsSelected() || !pPanel->HasItem() )
	{
		pPanel->SetSelected( false );
	}
	else
	{
		pPanel->SetSelected( true );
	}
	SetBorderForItem( pPanel, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::DeSelectAllBackpackItemPanels( void )
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() )
		{
			m_pItemModelPanels[i]->SetSelected( false );
			SetBorderForItem( m_pItemModelPanels[i], false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever the selection changes in the item loadout panel
//-----------------------------------------------------------------------------
void CBackpackPanel::OnItemContentsChanged( CEconItemView *pEconItemView )
{
	Assert( pEconItemView );

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		CEconItemView *pInternalItem = m_pItemModelPanels[i] && m_pItemModelPanels[i]->HasItem()
									 ? m_pItemModelPanels[i]->GetItem()
									 : NULL;

		if ( *pInternalItem == *pEconItemView )
		{
			m_pItemModelPanels[i]->DirtyDescription();
			OnItemSelectionChanged();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CBackpackPanel::OnTextChanged( KeyValues *data )
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

	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );
	if ( pComboBox )
	{
		if ( pComboBox == m_pSortByComboBox )
		{
			// the class selection combo box changed, update class details
			KeyValues *pUserData = m_pSortByComboBox->GetActiveItemUserData();
			if ( !pUserData )
				return;

			enum { kSortType_Dummy = -1 };
			int iSortTypeSelectionIndex = pUserData->GetInt( "sortby", kSortType_Dummy );
			if ( iSortTypeSelectionIndex != kSortType_Dummy )
			{
				uint32 iSortType = g_BackpackSortTypes[iSortTypeSelectionIndex].iSortType;
				if ( iSortType != kGCItemSort_NoSort )
				{
					InventoryManager()->SortBackpackBy( iSortType );

					// Now go back to the "Sort by" header, and move the focus to the close button.
					m_pSortByComboBox->ActivateItemByRow( 0 );
				}
			}
		}
		else if ( pComboBox == m_pShowRarityComboBox )
		{
			cl_showbackpackrarities.SetValue( m_pShowRarityComboBox->GetActiveItem() );

			// Refresh all item borders
			for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
			{
				SetBorderForItem( m_pItemModelPanels[i], false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnButtonChecked( KeyValues *pData )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( pData->GetPtr("panel") );
	
	if ( m_bShowBaseItems != m_pShowBaseItemsCheckbox->IsSelected() && m_pShowBaseItemsCheckbox == pPanel && IsVisible() )
	{
		SetShowBaseItems( m_pShowBaseItemsCheckbox->IsSelected() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnCancelSelection( void )
{
	if ( m_pConfirmDeleteDialog )
	{
		m_pConfirmDeleteDialog->MarkForDeletion();
		m_pConfirmDeleteDialog = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBackpackPanel::GetGreyOutItemPanelReason( CItemModelPanel *pItemPanel )
{
	if ( InToolSelectionMode() )
	{
		bool bIsSelectedTool = (m_ToolSelectionItem.IsValid() && pItemPanel->HasItem()) ? (m_ToolSelectionItem == *pItemPanel->GetItem()) : false;
		if ( !bIsSelectedTool )
		{
			if ( m_ToolSelectionItem.GetStaticData()->IsTool() )
			{
				if ( !CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, pItemPanel->GetItem() ) )
				{
					return "#Econ_GreyOutReason_ToolCannotApply";
				}
			}
			else if ( pItemPanel->GetItem() && pItemPanel->GetItem()->GetStaticData()->IsTool() )
			{
				if ( !CEconSharedToolSupport::ToolCanApplyTo( pItemPanel->GetItem(), &m_ToolSelectionItem ) )
				{
					return "#Econ_GreyOutReason_ToolCannotApply";
				}
			}
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !pItemPanel )
		return;

	const char *pszBorder = NULL;

	bool bIsSelectedTool = (m_ToolSelectionItem.IsValid() && pItemPanel->HasItem()) ? (m_ToolSelectionItem == *pItemPanel->GetItem()) : false;

	// Handle grey out
	const char *pszGreyOutReason = GetGreyOutItemPanelReason( pItemPanel );
	const bool bGreyOut = pszGreyOutReason != NULL;

	pItemPanel->SetGreyedOut( pszGreyOutReason );

	int iRarity = GetItemQualityForBorder( pItemPanel );

	{
		if ( InToolSelectionMode() && bIsSelectedTool )
		{
			// We're in tool application mode, and this panel is the tool being used
			pszBorder = "BackpackItemBorder_SelfMade";

			if ( m_pToolIcon )
			{
				int iX, iY;
				pItemPanel->GetPos( iX, iY );
				m_pToolIcon->SetPos( iX, iY );
				m_pToolIcon->SetVisible( true );
			}
		}
		else if ( bGreyOut )
		{
			if ( pItemPanel->IsSelected() )
			{
				pszBorder = g_szItemBorders[iRarity][4];
			}
			else
			{
				pszBorder = g_szItemBorders[iRarity][3];
			}
		}
		else
		{

			if ( pItemPanel->IsSelected() )
			{
				pszBorder = g_szItemBorders[iRarity][2];
			}
			else if ( bMouseOver )
			{
				pszBorder = g_szItemBorders[iRarity][1];
			}
			else
			{
				pszBorder = g_szItemBorders[iRarity][0];
			}
		}
	}

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );	
	pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
}

//-----------------------------------------------------------------------------
class CTFRemoveItemCustomizationConfirmDialog : public CTFGenericConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFRemoveItemCustomizationConfirmDialog, CTFGenericConfirmDialog );
public:
	CTFRemoveItemCustomizationConfirmDialog( const RefurbishableProperty& prop, CEconItemView *pItem )
		: CTFGenericConfirmDialog( prop.m_szDialogTitle,	// dialog title
								   prop.m_szDialogDesc,		// dialog text
								   "#RefurbishItem_Yes",	// confirm button text
								   "#RefurbishItem_No",		// cancel button text
								   NULL,					// callback
								   NULL )					// parent
		, m_prop( prop )
		, m_Item( *pItem )									// copy in case our UI changes behind us
	{
		GetCustomDialogLocalizationTokenFunc_t m_pDialogCustomTokenFunc = m_prop.m_pGetCustomDialogLocalizationTokenFunc;
		if ( m_pDialogCustomTokenFunc )
		{
			CUtlConstWideString wsDialogCustomToken;
			(*m_pDialogCustomTokenFunc)( &m_Item, m_prop.m_iUserData, wsDialogCustomToken );
			if ( !wsDialogCustomToken.IsEmpty() )
			{
				AddStringToken( "confirm_dialog_token", wsDialogCustomToken.Get() );
			}
		}
	}

	virtual ~CTFRemoveItemCustomizationConfirmDialog() { }

	virtual void OnCommand( const char *command );

private:
	RefurbishableProperty m_prop;
	CEconItemView m_Item;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SendGCSimpleAttributeRemovalMessage( CEconItemView *pEconItemView, const char *szDesc, EGCItemMsg eItemMsg )
{
	EconUI()->Gamestats_ItemTransaction( IE_ITEM_REMOVED_ATTRIB, pEconItemView, szDesc );
	GCSDK::CProtoBufMsg<CMsgGCRemoveCustomizationAttributeSimple> msg( eItemMsg );
	msg.Body().set_item_id( pEconItemView->GetItemID() );
	GCClientSystem()->BSendMessage( msg );
}

void CTFRemoveItemCustomizationConfirmDialog::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );

	// Did the user say "yes, remove this particular attribute"? If so, notify the GC. We can't
	// remove multiple attributes at a time because each removal will cause a new item to be created,
	// invalidating the item reference we've got in this dialog.
	if ( !Q_strnicmp( command, "confirm", 7 ) )
	{
		// remove the attribute
		switch( m_prop.m_eRemovalType )
		{
			case kCustomizationRemove_Paint:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "paint", k_EMsgGCRemoveItemPaint );
				break;

			case kCustomizationRemove_Name:
			{
				EconUI()->Gamestats_ItemTransaction( IE_ITEM_REMOVED_ATTRIB, &m_Item, "name" );
				GCSDK::CGCMsg< MsgGCRemoveItemName_t > msg( k_EMsgGCRemoveItemName );
				msg.Body().m_unItemID = m_Item.GetItemID();
				msg.Body().m_bDescription = false;
				GCClientSystem()->BSendMessage( msg );
			}
			break;

			case kCustomizationRemove_Desc:
			{
				EconUI()->Gamestats_ItemTransaction( IE_ITEM_REMOVED_ATTRIB, &m_Item, "description" );
				GCSDK::CGCMsg< MsgGCRemoveItemName_t > msg( k_EMsgGCRemoveItemName );
				msg.Body().m_unItemID = m_Item.GetItemID();
				msg.Body().m_bDescription = true;
				GCClientSystem()->BSendMessage( msg );
			}
			break;

			case kCustomizationRemove_CustomTexture:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "custom_texture", k_EMsgGCRemoveCustomTexture );
				break;

			case kCustomizationRemove_MakersMark:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "makers_mark", k_EMsgGCRemoveMakersMark );
				break;
			
			case kCustomizationRemove_StrangePart:
			{
				Assert( m_prop.m_iUserData != kNoUserData );
				int iKillEaterAttrIndex = m_prop.m_iUserData;

				// What attribute did we select?
				const CEconItemAttributeDefinition *pAttrDef = GetKillEaterAttr_Type( iKillEaterAttrIndex );
				Assert( pAttrDef );

				// Make sure this item has this attribute.
				float fScoreType = kKillEaterEvent_PlayerKill;
				Verify( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( &m_Item, pAttrDef, &fScoreType ) || iKillEaterAttrIndex == 0 );
				
				// Dispatch message.
				EconUI()->Gamestats_ItemTransaction( IE_ITEM_REMOVED_ATTRIB, &m_Item, "strange_part" );
				GCSDK::CProtoBufMsg<CMsgGCRemoveStrangePart> msg( k_EMsgGCRemoveStrangePart );
				
				msg.Body().set_item_id( m_Item.GetItemID() );
				msg.Body().set_strange_part_score_type( (int)fScoreType );
				GCClientSystem()->BSendMessage( msg );
			}
			break;

			case kCustomizationRemove_StrangeScores:
			{
				Assert( m_prop.m_iUserData == kNoUserData );

				EconUI()->Gamestats_ItemTransaction( IE_ITEM_RESET_STRANGE_COUNTERS, &m_Item );
				GCSDK::CProtoBufMsg<CMsgGCResetStrangeScores> msg( k_EMsgGCResetStrangeScores );
				msg.Body().set_item_id( m_Item.GetItemID() );
				GCClientSystem()->BSendMessage( msg );
			}
			break;

			case kCustomizationRemove_UpgradeCard:
				{
					Assert( m_prop.m_iUserData != kNoUserData );
				
					// Make sure we selected a valid attribute that this item has.
					const CEconItemAttributeDefinition *pAttrDef = GetCardUpgradeForIndex( &m_Item, m_prop.m_iUserData );
					Assert( pAttrDef );
					Verify( m_Item.FindAttribute( pAttrDef ) );
				
					// Dispatch message.
					EconUI()->Gamestats_ItemTransaction( IE_ITEM_REMOVED_ATTRIB, &m_Item, "upgrade_card" );
					GCSDK::CProtoBufMsg<CMsgGCRemoveUpgradeCard> msg( k_EMsgGCRemoveUpgradeCard );
				
					msg.Body().set_item_id( m_Item.GetItemID() );
					msg.Body().set_attribute_index( pAttrDef->GetDefinitionIndex() );
					GCClientSystem()->BSendMessage( msg );
				}
				break;

			case kCustomizationRemove_KillStreak:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "killstreak", k_EMsgGCRemoveKillStreak );
				break;
			case kCustomizationRemove_GiftedBy:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "giftedby", k_EMsgGCRemoveGiftedBy );
				break;
			case kCustomizationRemove_Festivizer:
				SendGCSimpleAttributeRemovalMessage( &m_Item, "festivizer", k_EMsgGCRemoveFestivizer );
				break;
			default:
				AssertMsg( false, "Unknown item customization removal type!" );
				break;
		}
	}

	SetVisible( false );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CRefurbishItemDialog : public CComboBoxBackpackOverlayDialogBase
{
public:
	DECLARE_CLASS_SIMPLE( CRefurbishItemDialog, CComboBoxBackpackOverlayDialogBase );

public:
	CRefurbishItemDialog( vgui::Panel *pParent, CEconItemView *m_pItem ) : CComboBoxBackpackOverlayDialogBase( pParent, m_pItem ) { }

private:
	virtual void PopulateComboBoxOptions()
	{
		Assert( m_pItem );

		KeyValues *pKeyValues = new KeyValues( "data" );
		for ( int i = 0; i < GetRemovableAttributesCount(); i++ )
		{
			if ( RemovableAttributes_DoesAttributeApply( i, m_pItem ) )
			{
				pKeyValues->SetInt( "data", i );

				RefurbishableProperty prop = RemovableAttributes_GetAttributeDetails( i );

				CUtlConstWideString wsDialogCustomToken;
				if ( prop.m_pGetCustomDialogLocalizationTokenFunc )
				{
					(*prop.m_pGetCustomDialogLocalizationTokenFunc)( m_pItem, prop.m_iUserData, wsDialogCustomToken );
				}
				CConstructLocalizedString localizedUI( GLocalizationProvider()->Find( prop.m_pszSelectionUILocalizationToken ), wsDialogCustomToken.IsEmpty() ? L"" : wsDialogCustomToken.Get() );
				GetComboBox()->AddItem( localizedUI, pKeyValues );
			}
		}
		pKeyValues->deleteThis();

		Assert( GetComboBox()->GetItemCount() > 0 );

		GetComboBox()->ActivateItemByRow( 0 );
	}

	virtual void OnComboBoxApplication()
	{
		if ( !m_pItem )
			return;

		KeyValues *pKVActiveUserData = GetComboBox()->GetActiveItemUserData();
		int iIndex = pKVActiveUserData ? pKVActiveUserData->GetInt( "data", -1 ) : -1;
		if ( iIndex < 0 )
			return;

		const RefurbishableProperty RefurbProp = RemovableAttributes_GetAttributeDetails( iIndex );

		CTFRemoveItemCustomizationConfirmDialog *pDialog = new CTFRemoveItemCustomizationConfirmDialog( RefurbProp, m_pItem );
		if ( pDialog )
		{
			pDialog->Show();
		}
	}

	virtual const char *GetTitleLabelLocalizationToken() const { return "#TF_Item_RefurbishItemHeader"; }
};


//-----------------------------------------------------------------------------
// Purpose: Handles item selection while in tool selection mode
//-----------------------------------------------------------------------------
void CBackpackPanel::HandleToolItemSelection( CEconItemView *pItem )
{
	if ( !InToolSelectionMode() )
	{
		// must be in tool selection mode
		Assert( InToolSelectionMode() );
		return;
	}

	if ( pItem )
	{
		// Check if we should bring up the shuffle dialog instead of directly using the tool
		static CSchemaAttributeDefHandle pAttrDef_CanShuffleCrateContents( "can shuffle crate contents" );
		if ( pItem->FindAttribute( pAttrDef_CanShuffleCrateContents ) )
		{
			CInputStringForItemBackpackOverlayDialog *pDialog = vgui::SETUP_PANEL( new CInputStringForItemBackpackOverlayDialog( this, pItem, &m_ToolSelectionItem ) );
			if ( pDialog )
			{
				pDialog->Show();
				CancelToolSelection();
			}
		}
		else if ( m_ToolSelectionItem.FindAttribute( pAttrDef_CanShuffleCrateContents ) )
		{
			CInputStringForItemBackpackOverlayDialog *pDialog = vgui::SETUP_PANEL( new CInputStringForItemBackpackOverlayDialog( this, &m_ToolSelectionItem, pItem ) );
			if ( pDialog )
			{
				pDialog->Show();
				CancelToolSelection();
			}
		}
		// is a tool being applied onto this item
		else if ( m_ToolSelectionItem.GetStaticData()->IsTool() && ApplyTool( this, &m_ToolSelectionItem, pItem ) )
		{
			CancelToolSelection();
			UpdateModelPanels();
		}
		// is this item a tool that can be applied on a selected item
		else if ( pItem->GetStaticData()->IsTool() && ApplyTool( this, pItem, &m_ToolSelectionItem ) )
		{
			CancelToolSelection();
			UpdateModelPanels();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tries to use the item. This might switch the panel to a tool item
//			selection mode, or launch the recipe crafting panel.
//-----------------------------------------------------------------------------
void CBackpackPanel::SetupToolSelectionItem()
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
		{
			m_ToolSelectionItem = *m_pItemModelPanels[i]->GetItem();
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Open up the trade dialog
//-----------------------------------------------------------------------------
void CBackpackPanel::DoTradeToPlayer()
{
	OpenTradingStartDialog( this );
}

//-----------------------------------------------------------------------------
// Purpose: Use the trade dialog to send a gift to a player.
//-----------------------------------------------------------------------------
void CBackpackPanel::DoGiftToPlayer()
{
	OpenTradingStartDialog( this, &m_ToolSelectionItem );
}

//-----------------------------------------------------------------------------
// Purpose: Open up the overlay to sell the selected item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoSellMarketplace()
{
	CUtlVector< CItemModelPanel* > m_vecSelected;
	GetSelectedPanels( SELECT_FIRST, m_vecSelected );
	Assert( m_vecSelected.Count() );
	if( !m_vecSelected.Count() )
		return;

	if ( m_vecSelected.Count() && steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		CEconItemView *pItem = m_vecSelected.Head()->GetItem();
		const char *pszPrefix = "";
		if ( GetUniverse() == k_EUniverseBeta )
		{
			pszPrefix = "beta.";
		}
		uint32 nAssetContext = 2; // k_EEconContextBackpack
		char szURL[512];
		V_snprintf( szURL, sizeof(szURL), "http://%ssteamcommunity.com/my/inventory/?sellOnLoad=1#%d_%d_%llu", pszPrefix, engine->GetAppID(), nAssetContext, pItem->GetItemID() );
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use a description tag, or offer to buy one
//-----------------------------------------------------------------------------
void CBackpackPanel::DoDescription()
{
	static CSchemaItemDefHandle pItemDef_DescTag( "Description Tag" );
	if ( !AttemptToUseItem( pItemDef_DescTag->GetDefinitionIndex() ) )
	{
		AttemptToShowItemInStore( pItemDef_DescTag->GetDefinitionIndex() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use a name tag, or offer to buy one
//-----------------------------------------------------------------------------
void CBackpackPanel::DoRename()
{
	static CSchemaItemDefHandle pItemDef_NameTag( "Name Tag" );
	if ( !AttemptToUseItem( pItemDef_NameTag->GetDefinitionIndex() ) )
	{
		AttemptToShowItemInStore( pItemDef_NameTag->GetDefinitionIndex() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Delete the selected items
//-----------------------------------------------------------------------------
void CBackpackPanel::DoDelete()
{
	// Hide the mouseover panel
	HideMouseOverPanel();

	int iItemsToDelete = 0;
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
		{
			iItemsToDelete++;
		}
	}

	// Bring up confirm dialog
	CConfirmDeleteItemDialog *pConfirm = vgui::SETUP_PANEL( new CConfirmDeleteItemDialog( this, ( iItemsToDelete > 1 ) ) );
	if ( pConfirm )
	{
		pConfirm->Show();

		m_pConfirmDeleteDialog = pConfirm;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use the selected tool
//-----------------------------------------------------------------------------
void CBackpackPanel::DoApplyOnItem()
{
	SetupToolSelectionItem();

	if ( m_ToolSelectionItem.IsValid() )
	{
		const IEconTool *pEconTool = m_ToolSelectionItem.GetItemDefinition()->GetEconTool();

		// Gather all quest objective attributes
		CRecipeComponentMatchingIterator recipeIterator( NULL, NULL );
		if ( m_ToolSelectionItem.GetSOCData() )
		{
			m_ToolSelectionItem.GetSOCData()->IterateAttributes( &recipeIterator );
		}

		if( pEconTool && recipeIterator.GetMatchingComponentInputs().Count() > 0 )
		{
			// Launch new crafting window if we need to
			if( m_pDynamicRecipePanel == NULL )
			{
				m_pDynamicRecipePanel = vgui::SETUP_PANEL( new CDynamicRecipePanel( this, "dynamic_recipe_panel", &m_ToolSelectionItem ) );
			}

			// Set recipe item into panel
			if ( m_pDynamicRecipePanel )
			{
				m_pDynamicRecipePanel->SetVisible( true );
				m_pDynamicRecipePanel->SetNewRecipe( &m_ToolSelectionItem );
			}
			return;
		}

		// Check if we actually have any items we can use this tool on
		bool bHasValidTargetItem = false;
		CPlayerInventory *pInv = InventoryManager()->GetLocalInventory();
		Assert( pInv );
		if ( pInv )
		{
			for ( int i = 0 ; i < pInv->GetItemCount() ; ++i )
			{
				CEconItemView *pItem = pInv->GetItem( i );
				if ( CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, pItem ) )
				{
					bHasValidTargetItem = true;
					break;
				}
			}

			// If no applicable items, try stock items
			if ( !bHasValidTargetItem )
			{
				// this is really inefficient, maybe have a list of baseitems somewhere
				CEconItemView tempItem;
				const CEconItemDefinition* pItemDef = NULL;

				const CEconItemSchema::SortedItemDefinitionMap_t& mapItems = GetItemSchema()->GetSortedItemDefinitionMap();
				for ( int it = mapItems.FirstInorder(); it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
				{
					if ( mapItems[it]->IsBaseItem() && !mapItems[it]->IsHidden() )
					{
						CFmtStr fmtStrCustomizedDefName( "Upgradeable %s", mapItems[it]->GetDefinitionName() );
						pItemDef = GetItemSchema()->GetItemDefinitionByName( fmtStrCustomizedDefName.Access() );
						if ( pItemDef )
						{
							tempItem.Init( pItemDef->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
							if ( CEconSharedToolSupport::ToolCanApplyTo( &m_ToolSelectionItem, &tempItem ) )
							{
								bHasValidTargetItem = true;
								break;
							}
						}
					}
				}

				if ( bHasValidTargetItem )
				{
					// automatically switch to stock items
					OnCommand( "showbaseitems" );
				}
			}
		}

		if ( !bHasValidTargetItem )
		{
			ShowMessageBox( NULL, "#ToolNoTargetItems", "#GameUI_OK" );
			return;
		}

		m_eSelectionMode = ToolSelection;
		m_nLastToolPage = GetCurrentPage();

		if ( m_pMouseOverTooltip )
		{
			m_pMouseOverTooltip->HideTooltip();
		}

		ClearNameFilter( true );
		SetCurrentPage( 0 );
		UpdateModelPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: use a consumable item directly from the backpack
//-----------------------------------------------------------------------------
void CBackpackPanel::DoUseConsumableItem()
{
	SetupToolSelectionItem();

#ifdef TF_CLIENT_DLL
	UseConsumableItem( &m_ToolSelectionItem, this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Use the tool to unwrap an item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoUnwrapItem()
{
	DoUseConsumableItem();
}

//-----------------------------------------------------------------------------
// Purpose: Use the tool to deliver an item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoDeliverItem()
{
	SetupToolSelectionItem();

#ifdef TF_CLIENT_DLL
	const CEconTool_WrappedGift *pWrappedGiftTool = m_ToolSelectionItem.GetItemDefinition()
												  ? m_ToolSelectionItem.GetItemDefinition()->GetTypedEconTool<CEconTool_WrappedGift>()
												  : NULL;

	if ( pWrappedGiftTool && pWrappedGiftTool->BIsDirectGift() )
	{
		DoUseConsumableItem();
		return;
	}
	else if ( pWrappedGiftTool && pWrappedGiftTool->BIsGlobalGift() )
	{
		// If this is a global gift, we don't let the user pick a target so we're done as of now.
		extern void UseUntargetedGiftConfirm( bool bConfirmed, void *pContext );
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_DeliverGiftDialog_Title", "#TF_DeliverGiftDialog_Random_Text", 
														      "#TF_DeliverGiftDialog_Confirm", "#TF_DeliverGiftDialog_Cancel", 
														      &UseUntargetedGiftConfirm );

		pDialog->SetContext( &m_ToolSelectionItem );
		return;
	}

	DoGiftToPlayer();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Show
//-----------------------------------------------------------------------------
void CBackpackPanel::DoApplyByItem()
{
	SetupToolSelectionItem();

	m_eSelectionMode = ToolSelection;
	m_nLastToolPage = GetCurrentPage();

	if ( m_pMouseOverTooltip )
	{
		m_pMouseOverTooltip->HideTooltip();
	}

	ClearNameFilter( true );
	SetCurrentPage( 0 );
	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: open shuffle items dialog
//-----------------------------------------------------------------------------
void CBackpackPanel::DoShuffle()
{
	SetupToolSelectionItem();

	CInputStringForItemBackpackOverlayDialog *pDialog = vgui::SETUP_PANEL( new CInputStringForItemBackpackOverlayDialog( this, &m_ToolSelectionItem ) );
	if ( pDialog )
	{
		pDialog->Show();
	}
}

void CBackpackPanel::DoEditSlot()
{
	SetupToolSelectionItem();

	// Launch new slot window if we need to
	if( m_pItemSlotPanel == NULL )
	{
		m_pItemSlotPanel = vgui::SETUP_PANEL( new CItemSlotPanel( this ) );
	}
				
	// Set item into panel
	if ( m_pItemSlotPanel )
	{
		m_pItemSlotPanel->SetVisible( true );
		m_pItemSlotPanel->SetItem( m_ToolSelectionItem.GetSOCData() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Refurbish item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoRefurbishItem()
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
		{
			m_ComboBoxOverlaySelectionItem = *m_pItemModelPanels[i]->GetItem();
			break;
		}
	}

	CRefurbishItemDialog *pRefurbishDialog = vgui::SETUP_PANEL( new CRefurbishItemDialog( this, &m_ComboBoxOverlaySelectionItem ) );
	if ( pRefurbishDialog )
	{
		pRefurbishDialog->Show();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deode by item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoGetItemFromStore()
{
	SetupToolSelectionItem();

	uint32 iDecoableItemDef = 0;
	if ( GetDecodedByItemDefIndex( &m_ToolSelectionItem, &iDecoableItemDef ) )
	{
		// casting to the proper type since our econ system is dumb
		const float& value_as_float = (float&)iDecoableItemDef;
		CEconItemDefinition * pDefIndex = GetItemSchema()->GetItemDefinition( (int)value_as_float );

		EconUI()->GetStorePanel()->AddToCartAndCheckoutImmediately( pDefIndex->GetDefinitionIndex() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Open End of the Line duck leaderboards
//-----------------------------------------------------------------------------
void CBackpackPanel::DoOpenDuckLeaderboards()
{
	CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
	CDucksLeaderboardManager *pDuckLeaderboards = vgui::SETUP_PANEL( new CDucksLeaderboardManager( pCharInfo, "DucksLeaderboardPanel" ) );
	pDuckLeaderboards->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Strange Count Transfer Dialog
//-----------------------------------------------------------------------------
void CBackpackPanel::DoStrangeCountTransfer()
{
	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	Assert( vecSelected.Count() );
	if ( vecSelected.IsEmpty() )
		return;

	m_pStrangeToolPanel = vgui::SETUP_PANEL( new CStrangeCountTransferPanel( this, vecSelected[0]->GetItem() ) );
}

//-----------------------------------------------------------------------------
// Collection crafting
//-----------------------------------------------------------------------------
void CBackpackPanel::DoCraftUpCollection()
{
	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_ALL, vecSelected );

	//Assert( vecSelected.Count() );
	//if ( vecSelected.IsEmpty() )
	//	return;

	// Get all the items that were selected
	CUtlVector< const CEconItemView* > vecSelectedItems;
	FOR_EACH_VEC( vecSelected, i )
	{
		if ( vecSelected[ i ]->GetItem() && GetCollectionCraftingInvalidReason( vecSelected[ i ]->GetItem(), NULL ) == NULL )
		{
			vecSelectedItems.AddToTail( vecSelected[ i ]->GetItem() );
		}
	}

	// For tracking how many times they've opened this menu
	tf_trade_up_use_count.SetValue( tf_trade_up_use_count.GetInt() - 1 );

	// Open it up!
	GetCollectionCraftPanel()->Show( vecSelectedItems );
}

//-----------------------------------------------------------------------------
// Collection crafting
//-----------------------------------------------------------------------------
void CBackpackPanel::DoHalloweenOffering()
{
	// Open it up!
	if ( !m_pHalloweenOfferingPanel )
	{
		m_pHalloweenOfferingPanel = vgui::SETUP_PANEL( new CHalloweenOfferingPanel( this, m_pMouseOverTooltip ) );
		m_pHalloweenOfferingPanel->InvalidateLayout( true, true );
	}
		// empty
	CUtlVector< const CEconItemView* > vecSelectedItems;
	m_pHalloweenOfferingPanel->Show( vecSelectedItems );
}

//-----------------------------------------------------------------------------
// Craft Common StatClock
//-----------------------------------------------------------------------------
void CBackpackPanel::DoCraftCommonStatClock()
{
	// Open it up!
	if ( !m_pMannCoTradePanel )
	{
		m_pMannCoTradePanel = vgui::SETUP_PANEL( new CCraftCommonStatClockPanel( this, m_pMouseOverTooltip ) );		// make this more generic
		m_pMannCoTradePanel->InvalidateLayout( true, true );
	}

	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels(SELECT_ALL, vecSelected);

	CUtlVector< const CEconItemView* > vecSelectedItems;
	FOR_EACH_VEC(vecSelected, i)
	{
		if (vecSelected[i]->GetItem() && GetCraftCommonStatClockInvalidReason(vecSelected[i]->GetItem(), NULL) == NULL)
		{
			vecSelectedItems.AddToTail(vecSelected[i]->GetItem());
		}
	}

	m_pMannCoTradePanel->Show( vecSelectedItems );
}

//-----------------------------------------------------------------------------
// Open up the quest map
//-----------------------------------------------------------------------------
void CBackpackPanel::DoOpenConTracker()
{
	GetQuestMapPanel()->SetVisible( true );
	EconUI()->CloseEconUI();
}

//-----------------------------------------------------------------------------
// Purpose: Bring up the 3D inspect panel for the selected item
//-----------------------------------------------------------------------------
void CBackpackPanel::DoInspectModel()
{
	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	if ( vecSelected.IsEmpty() )
		return;

	CEconItemView *pItem = vecSelected[0]->GetItem();
	if ( !pItem )
		return;

	if ( BCanPreviewPaintKit( pItem ) )
	{
		CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
		pCharInfo->OpenToPaintkitPreview( vecSelected[0]->GetItem(), true, true );
	}
	else
	{
		bool bClassCanUse = false;
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; ++iClass )
		{
			if ( pItem->GetStaticData()->CanBeUsedByClass( iClass ) )
			{
				m_pInspectCosmeticPanel->PreviewItem( iClass, pItem );
				bClassCanUse = true;
				break;
			}
		}

		if ( !bClassCanUse )
		{
			m_pInspectCosmeticPanel->PreviewItem( TF_FIRST_NORMAL_CLASS, pItem );
		}

		m_pInspectCosmeticPanel->SetVisible( true );
	}
}

void CBackpackPanel::DoPreviewPaintkitsOnItem()
{
	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	if ( vecSelected.IsEmpty() )
		return;

	CEconItemView *pItem = vecSelected[0]->GetItem();
	if ( !pItem )
		return;

	if ( !BCanPreviewPaintKit( pItem ) )
		return;

	CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
	pCharInfo->OpenToPaintkitPreview( vecSelected[0]->GetItem(), true, false );
}

void CBackpackPanel::DoPreviewItemsWithPaintkit()
{
	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	if ( vecSelected.IsEmpty() )
		return;

	CEconItemView *pItem = vecSelected[0]->GetItem();
	if ( !pItem )
		return;

	if ( !BCanPreviewPaintKit( pItem ) )
		return;

	CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
	pCharInfo->OpenToPaintkitPreview( vecSelected[0]->GetItem(), false, true );
}


//-----------------------------------------------------------------------------
void CBackpackPanel::OpenInspectModelPanelAndCopyItem( CEconItemView *pItemView )
{
	if ( !pItemView )
		return;

	EconUI()->OpenEconUI( ECONUI_BACKPACK );

	// Figure out which preview to show
	float flInspect = 0;
	static CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItemView, pAttrib_WeaponAllowInspect, &flInspect ) && flInspect != 0.f )
	{
		CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
		pCharInfo->OpenToPaintkitPreview( pItemView, false, false );
	}
	else
	{
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; ++iClass )
		{
			if ( pItemView->GetStaticData()->CanBeUsedByClass( iClass ) )
			{
				m_pInspectCosmeticPanel->PreviewItemCopy( iClass, pItemView );
				m_pInspectCosmeticPanel->SetVisible( true );
				break;
			}
		}
	}
}

CCollectionCraftingPanel* CBackpackPanel::GetCollectionCraftPanel()
{
	if ( !m_pCollectionCraftPanel )
	{
		m_pCollectionCraftPanel = vgui::SETUP_PANEL( new CCollectionCraftingPanel( this, m_pMouseOverTooltip ) );
		m_pCollectionCraftPanel->InvalidateLayout( true, true );
	}

	return m_pCollectionCraftPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Buy a key, and then immediately use it on the selected crate once
//			tbe store transaction completes
//-----------------------------------------------------------------------------
void CBackpackPanel::DoBuyKeyAndOpenCrate()
{
	return;

	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	if ( vecSelected.IsEmpty() )
		return;

	m_hQuickOpenCrate.SetItem( vecSelected.Head()->GetItem() );

	uint32 iDecoableItemDef = 0;
	if ( GetDecodedByItemDefIndex( m_hQuickOpenCrate, &iDecoableItemDef ) )
	{
		// casting to the proper type since our econ system is dumb
		const float& value_as_float = (float&)iDecoableItemDef;
		CEconItemDefinition * pDefIndex = GetItemSchema()->GetItemDefinition( (int)value_as_float );

		EconUI()->GetStorePanel()->AddToCartAndCheckoutImmediately( pDefIndex->GetDefinitionIndex() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the first compatible key in our inventory, and use it on the
//			selected crate
//-----------------------------------------------------------------------------
void CBackpackPanel::DoOpenCrateWithKey()
{
	const char *pUserTxnCC = GCClientSystem()->GetTxnCountryCode();
	bool bUserChanceRestricted = pUserTxnCC && !BEconCountryAllowDecodableContainers( pUserTxnCC );

	if ( bUserChanceRestricted )
	{
		ShowMessageBox( "#ToolContainerRestrictedTitle", "#ToolContainerRestricted", "#GameUI_OK" );
		return;
	}

	CUtlVector< CItemModelPanel* > vecSelected;
	GetSelectedPanels( SELECT_FIRST, vecSelected );

	if ( vecSelected.IsEmpty() )
		return;

	CEconItemView *pCrate = vecSelected.Head()->GetItem();

	CEconItemView *pKey = GetFirstCompatibleKeyForCrate( pCrate );
	if ( !pKey )
		return;

	ApplyTool( this, pKey, pCrate );
}


//-----------------------------------------------------------------------------
// Purpose: Open up the loadout for a class
//-----------------------------------------------------------------------------
void CBackpackPanel::DoEquipForClass( int nClass )
{
	// Negative because reasons
	EconUI()->OpenEconUI( -nClass );
}

//-----------------------------------------------------------------------------
// Purpose: Given a paint can index, offer to use one or buy one
//-----------------------------------------------------------------------------
void CBackpackPanel::DoPaint( int nPaintItemIndex, bool bUseStore, bool bUseMarket )
{
	if ( !bUseStore && !bUseMarket )
	{
		AttemptToUseItem( nPaintItemIndex );
	}

	if ( bUseStore )
	{
		AttemptToShowItemInStore( nPaintItemIndex );
	}
	else if ( bUseMarket )
	{
		AttemptToShowItemInMarket( nPaintItemIndex );
	}
}
//-----------------------------------------------------------------------------
// Purpose: Given a strange part index, offer to use one or buy one (Market)
//-----------------------------------------------------------------------------
void CBackpackPanel::DoStrangePart( int nStrangePartIndex, bool bUseMarket )
{
	if ( !bUseMarket )
	{
		AttemptToUseItem( nStrangePartIndex );
	}

	if ( bUseMarket )
	{
		AttemptToShowItemInMarket( nStrangePartIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to find the first item with the passed-in item name in our inventory
//			and try to use it on the selected panel's item.  If we dont have an item
//			matching the name passed in, go to the store and prompt the user to buy one.
//-----------------------------------------------------------------------------
bool CBackpackPanel::AttemptToUseItem( item_definition_index_t iItemDefIndex )
{
	CUtlVector< CItemModelPanel* > m_vecSelected;
	GetSelectedPanels( SELECT_FIRST, m_vecSelected );
	Assert( m_vecSelected.Count() );
	if ( !m_vecSelected.Count() )
		return false;

	CEconItemView *pSelectedItem = m_vecSelected.Head()->GetItem();
	Assert( pSelectedItem );
	if ( !pSelectedItem )
		return false;

	CEconItemView *pNameTag = CTFPlayerInventory::GetFirstItemOfItemDef( iItemDefIndex );
	if ( pNameTag )
	{
		if ( ApplyTool( this, pNameTag, pSelectedItem ) )
		{
			CancelToolSelection();
			UpdateModelPanels();
		}
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
void CBackpackPanel::AttemptToShowItemInStore( item_definition_index_t iItemDefIndex )
{
	CUtlVector< CItemModelPanel* > m_vecSelected;
	GetSelectedPanels( SELECT_FIRST, m_vecSelected );
	Assert( m_vecSelected.Count() );
	if( !m_vecSelected.Count() )
		return;

	CEconItemView *pSelectedItem = m_vecSelected.Head()->GetItem();
	Assert( pSelectedItem );
	if ( !pSelectedItem )
		return;

	EconUI()->OpenStorePanel( iItemDefIndex, false );
}
//-----------------------------------------------------------------------------
void CBackpackPanel::AttemptToShowItemInMarket( item_definition_index_t iItemDefIndex )
{
	CUtlVector< CItemModelPanel* > m_vecSelected;
	GetSelectedPanels( SELECT_FIRST, m_vecSelected );
	Assert( m_vecSelected.Count() );
	if ( !m_vecSelected.Count() )
		return;

	CEconItemView *pSelectedItem = m_vecSelected.Head()->GetItem();
	Assert( pSelectedItem );
	if ( !pSelectedItem )
		return;

	CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemDefIndex );
	Assert( pItemDef );
	if ( !pItemDef )
		return;

	if ( !CBaseAdPanel::CheckForRequiredSteamComponents( "#StoreUpdate_SteamRequired", "#MMenu_OverlayRequired" ) )
		return;

	if ( pItemDef && steamapicontext && steamapicontext->SteamFriends() )
	{
		const char *pszPrefix = "";
		if ( GetUniverse() == k_EUniverseBeta )
		{
			pszPrefix = "beta.";
		}

		static char pszItemName[256];
		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( pItemDef->GetItemBaseName() ), pszItemName, sizeof( pszItemName ) );

		char szURL[512];
		V_snprintf( szURL, sizeof( szURL ), "http://%ssteamcommunity.com/market/listings/%d/%s", pszPrefix, engine->GetAppID(), pszItemName );
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
	}
}
//-----------------------------------------------------------------------------
// Purpose: Get the first, or all selected item model panels
//-----------------------------------------------------------------------------
void CBackpackPanel::GetSelectedPanels( ESelection eSelection, CUtlVector< CItemModelPanel* >& m_vecSelected ) const
{
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
		{
			m_vecSelected.AddToTail( m_pItemModelPanels[i] );

			if ( eSelection == SELECT_FIRST )
			{
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OnCommand( const char *command )
{
	if ( V_strncasecmp( command, "goto_page_", V_strlen( "goto_page_" ) ) == 0 )
	{
		int iPage = V_atoi( &command[ V_strlen( "goto_page_" ) ] );
		SetCurrentPage( iPage );
		UpdateModelPanels();
		return;
	}
	else if ( !Q_strnicmp( command, "nextpage", 8 ) )
	{
		if ( !m_bDragging )
		{
			DeSelectAllBackpackItemPanels();
		}

		SetCurrentPage( GetCurrentPage() + 1 );
		UpdateModelPanels();
		return;
	}
	else if ( !Q_strnicmp( command, "prevpage", 8 ) )
	{
		if ( !m_bDragging )
		{
			DeSelectAllBackpackItemPanels();
		}

		SetCurrentPage( GetCurrentPage() - 1 );
		UpdateModelPanels();
		return;
	}
	else if ( !Q_strnicmp( command, "useitem", 7 ) )
	{
		AssertMsg( 0, "Everything should be going through the context menu. Fix the calling code." );
		return;
	}
	else if ( !Q_strnicmp( command, "showbackpackitems", 17 ) )
	{
		SetShowBaseItems( false );
		if ( m_pShowBaseItemsCheckbox )
		{
			m_pShowBaseItemsCheckbox->SetSelected( false );
		}
		return;
	}
	else if ( !Q_strnicmp( command, "showbaseitems", 13 ) )
	{
		SetShowBaseItems( true );
		if ( m_pShowBaseItemsCheckbox )
		{
			m_pShowBaseItemsCheckbox->SetSelected( true );
		}
		return;
	}
	else if ( !Q_strnicmp( command, "canceltool", 10 ) )
	{
		CancelToolSelection();
		UpdateModelPanels();
		return;
	}
	else if ( !Q_stricmp( command, "show_explanations" ) )
	{
		if ( !m_flStartExplanationsAt )
		{
			m_flStartExplanationsAt = Plat_FloatTime();
			vgui::ivgui()->AddTickSignal( GetVPanel() );
		}
		RequestFocus();
	}
	else if ( !Q_stricmp( command, "showdetails" ) )
	{
		for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
		{
			if ( m_pItemModelPanels[i]->IsSelected() && m_pItemModelPanels[i]->HasItem() )
			{
				OpenArmory( m_pItemModelPanels[i]->GetItem() );
				break;
			}
		}
		UpdateModelPanels();
		return;
	}
	else if ( !V_strnicmp( command, "equipclass", 10 ) )
	{
		int nClass = atoi( command + 10 );
		DoEquipForClass( nClass );
	}
	else if ( !V_strnicmp( command, "paint", 5 ) )
	{
		int nIndex = atoi( command + 5 );
		DoPaint( nIndex, false, false );
	}
	else if ( !V_strnicmp( command, "market_paint", 12 ) )
	{
		int nIndex = atoi( command + 12 );
		DoPaint( nIndex, false, true );
	}
	else if ( !V_strnicmp( command, "store_paint", 11 ) )
	{
		int nIndex = atoi( command + 11 );
		DoPaint( nIndex, true, false );
	}
	else if ( !V_strnicmp( command, "strangepart_", 12 ) )
	{
		int nIndex = atoi( command + 12 );
		DoStrangePart( nIndex, false );
	}
	else if ( !V_strnicmp( command, "market_strangepart_", 19 ) )
	{
		int nIndex = atoi( command + 19 );
		DoStrangePart( nIndex, true );
	}
	else if ( !V_strnicmp( command, "Context_CraftUpCollection", 25 ) )
	{
		DoCraftUpCollection();
	}
	else if ( !V_strnicmp( command, "Context_CraftCommonStatClock", 25 ) )
	{
		DoCraftCommonStatClock();
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::OpenArmory( CEconItemView* item )
{
	PostMessage( GetParent()->GetParent()->GetParent(), new KeyValues("OpenArmoryDirect", "itemdef", item->GetItemDefIndex() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::CancelToolSelection( void )
{
	if ( m_eSelectionMode == StandardSelection )
		return;

	m_eSelectionMode = StandardSelection;

	ClearNameFilter( false );

	OnCommand( "showbackpackitems" );

	SetCurrentPage( m_nLastToolPage );
	m_nLastToolPage = 0;

	m_ToolSelectionItem.Invalidate();
	if ( m_pToolIcon )
	{
		m_pToolIcon->SetVisible( false );
	}
	DeSelectAllBackpackItemPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::SetShowBaseItems( bool bShow )
{
	bool bGoToFirstPage = m_bShowBaseItems != bShow;
	m_bShowBaseItems = bShow;
	if( bGoToFirstPage )
	{
		SetCurrentPage( 0 );
	}

	DeSelectAllBackpackItemPanels();
	if ( m_pToolIcon )
	{
		m_pToolIcon->SetVisible( false );
	}
	UpdateModelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar *CBackpackPanel::GetExplanationConVar( void )
{
	return &tf_explanations_backpackpanel;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBackpackPanel::SetCurrentPage( int nNewPage )
{
	if( m_pToolIcon )
	{
		m_pToolIcon->SetVisible( false );
	}

	if ( nNewPage < 0 )
	{
		nNewPage = GetNumPages() - 1;
	}
	else if ( nNewPage >= GetNumPages() )
	{
		nNewPage = 0;
	}

	// deselect old page button
	if ( m_Pages.Count() > GetCurrentPage() && m_Pages[GetCurrentPage()] )
	{
		CExButton* pButton = dynamic_cast<CExButton*>( m_Pages[GetCurrentPage()]->FindChildByName( "Button" ) );
		if ( pButton )
		{
			pButton->SetSelected( false );
		}
	}

	BaseClass::SetCurrentPage( nNewPage );

	// mark new page button as selected
	if ( m_Pages.Count() > GetCurrentPage() &&  m_Pages[GetCurrentPage()] )
	{
		CExButton* pButton = dynamic_cast<CExButton*>( m_Pages[GetCurrentPage()]->FindChildByName( "Button" ) );
		if ( pButton )
		{
			pButton->SetSelected( true );
		}
	}
}


int	CBackpackPanel::GetItemQualityForBorder( CItemModelPanel* pItemPanel ) const
{
	if ( pItemPanel->HasItem() && ( cl_showbackpackrarities.GetInt() > 0 || m_bForceShowBackpackRarities )
				   && ( cl_showbackpackrarities.GetInt() < 2 || pItemPanel->GetItem()->IsMarketable() ) )
	{
		uint8 nRarity = pItemPanel->GetItem()->GetRarity();
		if ( ( nRarity != k_unItemRarity_Any ) && ( pItemPanel->GetItem()->GetItemQuality() != AE_SELFMADE ) && ( pItemPanel->GetItem()->GetItemQuality() != AE_UNUSUAL ) )
		{
			// translate this quality to rarity
			return nRarity + AE_RARITY_DEFAULT;
		}

		return pItemPanel->GetItem()->GetItemQuality();
	}

	return 0;
}
