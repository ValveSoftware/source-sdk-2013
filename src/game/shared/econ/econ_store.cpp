//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Common objects and utilities related to the in-game item store
//
//=============================================================================

#include "cbase.h"
#include "econ_store.h"
#include "gcsdk/enumutils.h"

#if defined(CLIENT_DLL)
#include "econ_ui.h"
#include "store/store_panel.h"
#include "econ_item_inventory.h"
#else
#include "gcsdk/gcconstants.h"
#endif

#if defined(CLIENT_DLL) || defined(GAME_DLL)
#include "econ_item_system.h"
#endif

// For localization
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "tier0/icommandline.h"

#ifdef CLIENT_DLL
// For formatting in locale
#include <string>
#include <sstream>
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar store_version( "store_version", "1", FCVAR_CLIENTDLL | FCVAR_HIDDEN | FCVAR_ARCHIVE, "Which version of the store to display." );

ENUMSTRINGS_START( ECurrency )
	{ k_ECurrencyUSD, "USD" },
	{ k_ECurrencyGBP, "GBP" },
	{ k_ECurrencyEUR, "EUR" },
	{ k_ECurrencyRUB, "RUB" },
	{ k_ECurrencyBRL, "BRL" },
	{ k_ECurrencyJPY, "JPY" },
	{ k_ECurrencyNOK, "NOK" },
	{ k_ECurrencyIDR, "IDR" },
	{ k_ECurrencyMYR, "MYR" },
	{ k_ECurrencyPHP, "PHP" },
	{ k_ECurrencySGD, "SGD" },
	{ k_ECurrencyTHB, "THB" },
	{ k_ECurrencyVND, "VND" },
	{ k_ECurrencyKRW, "KRW" },
	{ k_ECurrencyTRY, "TRY" },
	{ k_ECurrencyUAH, "UAH" },
	{ k_ECurrencyMXN, "MXN" },
	{ k_ECurrencyCAD, "CAD" },
	{ k_ECurrencyAUD, "AUD" },
	{ k_ECurrencyNZD, "NZD" },
	{ k_ECurrencyPLN, "PLN" },
	{ k_ECurrencyCHF, "CHF" },
	{ k_ECurrencyCNY, "CNY" },
	{ k_ECurrencyTWD, "TWD" },
	{ k_ECurrencyHKD, "HKD" },
	{ k_ECurrencyINR, "INR" },
	{ k_ECurrencyAED, "AED" },
	{ k_ECurrencySAR, "SAR" },
	{ k_ECurrencyZAR, "ZAR" },
	{ k_ECurrencyCOP, "COP" },
	{ k_ECurrencyPEN, "PEN" },
	{ k_ECurrencyCLP, "CLP" },
	{ k_ECurrencyARS, "ARS" },
	{ k_ECurrencyCRC, "CRC" },
	{ k_ECurrencyILS, "ILS" },
	{ k_ECurrencyKWD, "KWD" },
	{ k_ECurrencyQAR, "QAR" },
	{ k_ECurrencyUYU, "UYU" },
	{ k_ECurrencyKZT, "KZT" },
	{ k_ECurrencyBYN, "BYN" },
	{ k_ECurrencyInvalid, "Invalid" }
ENUMSTRINGS_REVERSE( ECurrency, k_ECurrencyInvalid )

ENUMSTRINGS_START( EPurchaseState )
{ k_EPurchaseStateInvalid, "Invalid" },
{ k_EPurchaseStateInit, "Init" },
{ k_EPurchaseStateWaitingForAuthorization, "WaitingForAuthorization" },
{ k_EPurchaseStatePending, "Pending" },
{ k_EPurchaseStateComplete, "Complete" },
{ k_EPurchaseStateFailed, "Failed" },
{ k_EPurchaseStateCanceled, "Canceled" },
{ k_EPurchaseStateRefunded, "Refunded" },
{ k_EPurchaseStateChargeback, "Chargeback" },
{ k_EPurchaseStateChargebackReversed, "Chargeback Reversed" },
ENUMSTRINGS_END( EPurchaseState )

ENUMSTRINGS_START( EPurchaseResult )
{ k_EPurchaseResultOK, "OK" },
{ k_EPurchaseResultFail, "Fail" },
{ k_EPurchaseResultInvalidParam, "InvalidParam" },
{ k_EPurchaseResultInternalError, "InternalError" },
{ k_EPurchaseResultNotApproved, "NotApproved" },
{ k_EPurchaseResultAlreadyCommitted, "AlreadyCommitted" },
{ k_EPurchaseResultUserNotLoggedIn, "UserNotLoggedIn" },
{ k_EPurchaseResultWrongCurrency, "WrongCurrency" },
{ k_EPurchaseResultAccountError, "AccountError" },
{ k_EPurchaseResultInsufficientFunds, "InsufficientFunds Reversed" },
{ k_EPurchaseResultTimedOut, "TimedOut" },
{ k_EPurchaseResultAcctDisabled, "AcctDisabled" },
{ k_EPurchaseResultAcctCannotPurchase, "AcctCannotPurchase" },
{ k_EMicroTxnResultFailedFraudChecks, "PurchaseFailedSupport" }, // this string is mismatched so "fraud" doesn't appear in the client code
{ k_EPurchaseResultOldPriceSheet, "OldPriceSheet" },
{ k_EPurchaseResultTxnNotFound, "TxnNotFound" },
ENUMSTRINGS_END( EPurchaseResult )

ENUMSTRINGS_START( EGCTransactionAuditReason )
{ k_EGCTransactionAudit_GCTransactionCompleted, "Completed" },
{ k_EGCTransactionAudit_GCTransactionInit, "Init" },
{ k_EGCTransactionAudit_GCTransactionPostInit, "Post Init" },
{ k_EGCTransactionAudit_GCTransactionFinalize, "Finalize" },
{ k_EGCTransactionAudit_GCTransactionFinalizeFailed, "Finalize Failed" },
{ k_EGCTransactionAudit_GCTransactionCanceled, "Canceled" },
{ k_EGCTransactionAudit_SteamFailedMismatch, "Steam Failed State Mismatch" },
{ k_EGCTransactionAudit_GCRemovePurchasedItems, "Remove Purchased Items" },
{ k_EGCTransactionAudit_GCTransactionInsert, "Transaction Insert" },
{ k_EGCTransactionAudit_GCTransactionCompletedPostChargeback, "Completed (Post-Chargeback)" },
ENUMSTRINGS_END( EGCTransactionAuditReason )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void econ_store_entry_t::InitCategoryTags( const char *pTags )
{
	// Default to "unrentable".
	m_fRentalPriceScale = 100.0f;

	if ( !pTags || !pTags[0] )
		return;

	// Cache tags - this pointer comes out of a KeyValues that isn't deleted
	m_pchCategoryTags = pTags;

	// Split the tags apart
	CUtlVector< char * > vecTokens;
	V_SplitString( pTags, "+", vecTokens );

	// Sanity check the results of V_SplitString()
#ifdef _DEBUG
	const int nTagsLength = V_strlen( pTags );
	int nSepCount = 0;	// # of separators
	for ( int i = 0; i < nTagsLength; ++i )
	{
		if ( pTags[i] == '+' )
			++nSepCount;
	}
//	Assert( ( vecTokens.Size() - 1 ) == nSepCount );
#endif

	// Calculate the maximum that we want to charge for this rental.
	float fMaxRentalPriceScale = 0.0f;

	// Generate symbols for each tag
	FOR_EACH_VEC( vecTokens, i )
	{
		CategoryTag_t info;
		info.m_strName = vecTokens[i];
		info.m_unID = CEconStoreCategoryManager::GetCategoryID( vecTokens[i] );
		m_vecCategoryTags.AddToTail( info );

		const float fCategoryRentalPriceScale = GetEconPriceSheet()->GetRentalPriceScale( vecTokens[i] );
		fMaxRentalPriceScale = MAX( fMaxRentalPriceScale, fCategoryRentalPriceScale );
	}

	m_fRentalPriceScale = fMaxRentalPriceScale <= 0.0f || fMaxRentalPriceScale >= 100.0f
						? 100.0f
						: fMaxRentalPriceScale;
	
	// Clean up
	vecTokens.PurgeAndDeleteElements();
}

void econ_store_entry_t::SetItemDefinitionIndex( item_definition_index_t usDefIndex )
{
	AssertMsg( usDefIndex != INVALID_ITEM_DEF_INDEX, "Invalid item definition index!" );
	m_usDefIndex = usDefIndex;
}

bool econ_store_entry_t::IsListedInCategory( StoreCategoryID_t unID ) const
{
	AssertMsg( unID != CEconStoreCategoryManager::k_CategoryID_Invalid, "Did you mean to search for an invalid symbol?" );

	FOR_EACH_VEC( m_vecCategoryTags, i )
	{
		if ( m_vecCategoryTags[i].m_unID == unID )
			return true;
	}
	
	return false;
}

bool econ_store_entry_t::IsListedInSubcategories( const CEconStoreCategoryManager::StoreCategory_t &Category ) const
{
	FOR_EACH_VEC( Category.m_vecSubcategories, iSubCategory )
	{
		if ( IsListedInCategory( Category.m_vecSubcategories[iSubCategory]->m_unID ) )
			return true;
	}

	return false;
}

bool econ_store_entry_t::IsListedInCategoryOrSubcategories( const CEconStoreCategoryManager::StoreCategory_t &Category ) const
{
	return IsListedInCategory( Category.m_unID ) ||
		IsListedInSubcategories( Category );
}

bool econ_store_entry_t::IsOnSale( ECurrency eCurrency ) const
{
	return GetSalePrice( eCurrency ) > 0
		&& GetSalePrice( eCurrency ) < GetBasePrice( eCurrency );
}

bool econ_store_entry_t::IsRentable() const
{
	return m_fRentalPriceScale < 100.0f
		&& m_fRentalPriceScale > 0.0f;
}

#ifdef CLIENT_DLL
bool econ_store_entry_t::HasDiscount( ECurrency eCurrency, item_price_t *out_punOptionalBasePrice ) const
{
	// Items on sale always report as being discounted.
	if ( IsOnSale( eCurrency ) ||
		( GetItemDefinitionIndex() == InventoryManager()->GetLocalInventory()->GetPreviewItemDef() ) )
	{
		if ( out_punOptionalBasePrice )
		{
			*out_punOptionalBasePrice = GetBasePrice( eCurrency );
		}

		return true;
	}

	// If we're not a bundle we have no other way of being on sale -- abort.
	const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( GetItemDefinitionIndex() );
	if ( !pItemDef || !pItemDef->IsBundle() )
		return false;

	const bundleinfo_t *pBundleInfo = pItemDef->GetBundleInfo();
	Assert( pBundleInfo );

	item_price_t unTotalPriceOfBundleItems = 0;
	FOR_EACH_VEC( pBundleInfo->vecItemDefs, bundleIdx )
	{
		const econ_store_entry_t *pCurEntry = pBundleInfo->vecItemDefs[bundleIdx]
											? GetEconPriceSheet()->GetEntry( pBundleInfo->vecItemDefs[bundleIdx]->GetDefinitionIndex() )
											: NULL;
		if ( pCurEntry )
		{
			unTotalPriceOfBundleItems += pCurEntry->GetCurrentPrice( eCurrency );
		}
	}

	// Did the bundle provide an actual discount?
	const item_price_t unBasePrice = GetCurrentPrice( eCurrency );
	if ( unBasePrice < unTotalPriceOfBundleItems )
	{
		if ( out_punOptionalBasePrice )
		{
			*out_punOptionalBasePrice = unTotalPriceOfBundleItems;
		}

		return true;
	}

	// No discount. Leave the base price pointer untouched.
	return false;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
StoreCategoryID_t econ_store_entry_t::GetCategoryTagIDFromIndex( uint32 iIndex ) const
{
	if ( !IsValidCategoryTagIndex( iIndex ) )
		return CEconStoreCategoryManager::k_CategoryID_Invalid;

	return m_vecCategoryTags[ iIndex ].m_unID;
}

item_price_t econ_store_entry_t::GetCurrentPrice( ECurrency eCurrency ) const
{
#ifdef CLIENT_DLL
	if ( m_bIsMarketItem )
	{
		const client_market_data_t *pClientMarketData = GetClientMarketData( GetItemDefinitionIndex(), AE_UNIQUE );
		if ( !pClientMarketData )
			return 0;

		return pClientMarketData->m_unLowestPrice;
	}
#endif
	return IsOnSale( eCurrency )
		 ? GetSalePrice( eCurrency )
		 : GetBasePrice( eCurrency );
}

float econ_store_entry_t::GetRentalPriceScale() const
{
	Assert( IsRentable() );

	return m_fRentalPriceScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void econ_store_entry_t::ValidatePrice( ECurrency eCurrency, item_price_t unPrice )
{
	if ( m_bIsMarketItem )
		return;

	// If the price is 0 and this is not a pack item (an item that is not individually for sale, but is sold as part of a pack bundle), post an alert
	if ( unPrice == 0 && !m_bIsPackItem )
	{
		CFmtStr fmtError( "Warning: Invalid price for item (item def=%i)", GetItemDefinitionIndex() );
		AssertMsg( false, "%s", fmtError.Access() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconStorePriceSheet::CEconStorePriceSheet()
	: m_pKVRaw( NULL )
	, m_mapEntries( DefLessFunc( uint16 ) )
	, m_mapRentalPriceScales( DefLessFunc( const char * ) )
	, m_RTimeVersionStamp( 0 )
	, m_pStorePromotionFirstTimePurchaseItem( NULL )
	, m_pStorePromotionFirstTimeWebPurchaseItem( NULL )
	, m_unFeaturedItemIndex( 0 )
	, m_eEconStoreSortType( kEconStoreSortType_Name_AToZ )
{
	Clear();
	m_FeaturedItems.m_pchName = "featured_items";

	m_mapCurrencyPricePoints.SetLessFunc( &price_point_map_key_t::Less );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconStorePriceSheet::~CEconStorePriceSheet()
{
	Clear();
}


//-----------------------------------------------------------------------------
// Purpose: Clears out the parsed data
//-----------------------------------------------------------------------------
void CEconStorePriceSheet::Clear()
{
	if ( m_pKVRaw )
	{
		m_pKVRaw->deleteThis();
		m_pKVRaw = NULL;
	}

	m_RTimeVersionStamp = 0;
	m_mapEntries.RemoveAll();
	m_FeaturedItems.m_vecEntries.RemoveAll();
	m_flPreviewPeriodDiscount = 0;

	m_mapCurrencyPricePoints.Purge();

#ifdef CLIENT_DLL
	m_vecFeaturedItems.Purge();
#endif // CLIENT_DLL

	// Clear categories in category manager
	ClearEconStoreCategoryManager();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the entry details for a specific item
//-----------------------------------------------------------------------------
const econ_store_entry_t *CEconStorePriceSheet::GetEntry( item_definition_index_t usDefIndex ) const 
{
	int iIndex = m_mapEntries.Find( usDefIndex );
	if ( m_mapEntries.IsValidIndex( iIndex ) )
		return &m_mapEntries[iIndex];
	
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool BInsertSinglePricePoint( CurrencyPricePointMap_t& out_mapPricePoints, const price_point_map_key_t& key, item_price_t unValue )
{
	if ( out_mapPricePoints.Find( key ) != out_mapPricePoints.InvalidIndex() )
		return false;

	out_mapPricePoints.Insert( key, unValue );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool BInitializeCurrencyPricePoints( CurrencyPricePointMap_t& out_mapPricePoints, KeyValues *pKVPricePoints )
{
	if ( !pKVPricePoints )
		return false;

	// Zero-price-point is universal.
	FOR_EACH_CURRENCY( eCurrency )
	{
		const price_point_map_key_t key = { 0, eCurrency };
		if ( !BInsertSinglePricePoint( out_mapPricePoints, key, 0 ) )
			return false;
	}

	// Individual price points specified in our store config.
	FOR_EACH_TRUE_SUBKEY( pKVPricePoints, pKVUSD )
	{
		const item_price_t unUSD = Q_atoi( pKVUSD->GetName() );

		// USD key, for ease of lookup.
		{
			const price_point_map_key_t key = { unUSD, k_ECurrencyUSD };
			if ( !BInsertSinglePricePoint( out_mapPricePoints, key, unUSD ) )
				return false;
		}
		
		// Exchange rates.
		FOR_EACH_VALUE( pKVUSD, pKVOtherCurrency )
		{
			const char *pszCurrencyName = pKVOtherCurrency->GetName();
			const ECurrency eCurrency = ECurrencyFromName( pszCurrencyName );

			if ( eCurrency == k_ECurrencyInvalid )
			{
				// Spew
				// don't crash, just conintue
				continue;
			}

			const price_point_map_key_t key = { unUSD, eCurrency };
			if ( !BInsertSinglePricePoint( out_mapPricePoints, key, pKVOtherCurrency->GetInt() ) )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Parses the KV version of the price sheet
//-----------------------------------------------------------------------------
bool CEconStorePriceSheet::InitFromKV( KeyValues *pKVRoot )
{
	Clear();
	m_pKVRaw = pKVRoot->MakeCopy();

	// Initialize categories - needed to initialize store entries
	if ( !GEconStoreCategoryManager()->BInit( this, m_pKVRaw ) )
	{
		return false;
	}

	// Initialize map of currency price points.
	if ( !BInitializeCurrencyPricePoints( m_mapCurrencyPricePoints, pKVRoot->FindKey( "inventoryvalvegcpricesheet" ) ) )
	{
		return false;
	}

	m_unFeaturedItemIndex = pKVRoot->GetInt( "featured_item_index", 0 );

	// first time purchase gift
	m_pStorePromotionFirstTimePurchaseItem = GetItemSchema()->GetItemDefinitionByName( pKVRoot->GetString( "promotion_first_time_purchase_gift" ) );
	m_pStorePromotionFirstTimeWebPurchaseItem = GetItemSchema()->GetItemDefinitionByName( pKVRoot->GetString( "promotion_first_time_web_purchase_gift" ) );

	EUniverse eUniverse = GetUniverse();

	m_unPreviewPeriod = pKVRoot->GetInt( eUniverse == k_EUniversePublic ? "preview_period" : "preview_period_nonpublic" );
	m_unBonusDiscountPeriod = pKVRoot->GetInt( eUniverse == k_EUniversePublic ? "bonus_discount_period" : "bonus_discount_period_nonpublic" );
	m_flPreviewPeriodDiscount = pKVRoot->GetFloat( "preview_period_discount" );

#ifdef ENABLE_STORE_RENTAL_BACKEND
	KeyValues *pRentalPriceScalesKV = pKVRoot->FindKey( "rental_price_scale" );
	if ( pRentalPriceScalesKV )
	{
		FOR_EACH_SUBKEY( pRentalPriceScalesKV, pCategoryKV )
		{
			m_mapRentalPriceScales.InsertOrReplace( pCategoryKV->GetName(), pCategoryKV->GetFloat() );
		}
	}
#endif

	memset( &m_StorePromotionSpendForFreeItem, 0, sizeof( m_StorePromotionSpendForFreeItem ) );
	KeyValues *pPromotionsKV = m_pKVRaw->FindKey( "promotion_spend_for_free_item" );
	if ( pPromotionsKV )
	{
		item_price_t unFreeItemSpendAmountUSD = pPromotionsKV->GetInt( "price_threshold", 0 );
		FOR_EACH_CURRENCY( eCurrency )
		{
			const price_point_map_key_t key = { unFreeItemSpendAmountUSD, eCurrency };
			const CurrencyPricePointMap_t::IndexType_t unIdx = m_mapCurrencyPricePoints.Find( key );
			if ( unIdx == m_mapCurrencyPricePoints.InvalidIndex() )
			{
				continue;
			}

			m_StorePromotionSpendForFreeItem.m_rgusPriceThreshold[ eCurrency ] = m_mapCurrencyPricePoints[unIdx];
		}

#if !defined(CLIENT_DLL) && !defined(GAME_DLL)
		CEconItemSchema *pSchema = GEconManager()->GetItemSchema();
		m_StorePromotionSpendForFreeItem.m_pItemDef = pSchema->GetItemDefinitionByName( pPromotionsKV->GetString( "item_definition" ) );
		AssertMsg( m_StorePromotionSpendForFreeItem.m_pItemDef || !pPromotionsKV->GetString( "item_definition", NULL ),
				   "Could find not the specified item for \"promotion_spend_for_free_item\"" );
#endif
	}

	// Get the normal sections
	KeyValues *pEntriesKV = m_pKVRaw->FindKey( "entries" );
	if ( pEntriesKV )
	{
		FOR_EACH_TRUE_SUBKEY( pEntriesKV, pKVEntry )
		{
			if ( !BInitEntryFromKV( pKVEntry ) )
			{
				continue;
			}
		}
	}

	// Get a list of Market entries to populate in to the 'store'.  GC never cares or reads these items
	// Only for Client
#ifdef CLIENT_DLL
	KeyValues *pMarketEntriesKV = m_pKVRaw->FindKey( "market_entries" );
	if ( pMarketEntriesKV )
	{
		FOR_EACH_TRUE_SUBKEY( pMarketEntriesKV, pKVEntry )
		{
			if ( !BInitMarketEntryFromKV( pKVEntry ) )
			{
				return false;
			}
		}
	}

	KeyValues *pFeaturedItemsKV = m_pKVRaw->FindKey( "featured_items" );
	if ( pFeaturedItemsKV )
	{
		FOR_EACH_SUBKEY( pFeaturedItemsKV, pKVEntry )
		{
			const char *pszItemName = pKVEntry->GetName();
			const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszItemName );
			if ( !pDef )
			{
				AssertMsg1( 0, "Unable to find item: %s", pszItemName );
				continue;
			}

			m_vecFeaturedItems.AddToTail( pDef->GetDefinitionIndex() );
		}
	}
#endif

	// Generate a hash of all item def indices and cache it off
	m_unHashForAllItems = CalculateHashFromItems();


	// Now that store entries are loaded, let the category manager do more stuff
	if ( !GEconStoreCategoryManager()->BOnPriceSheetLoaded( this ) )
		return false;

	return true;
}

uint32 CEconStorePriceSheet::CalculateHashFromItems() const
{
	CRC32_t unHash;
	CRC32_Init( &unHash );

	FOR_EACH_MAP_FAST( m_mapEntries, idx )
	{
		const econ_store_entry_t &entry = m_mapEntries[idx];
		item_definition_index_t usDefIndexTmp = entry.GetItemDefinitionIndex();
		CRC32_ProcessBuffer( &unHash, &usDefIndexTmp, sizeof( usDefIndexTmp ) );
	}

	CRC32_Final( &unHash );

	return (uint32)unHash;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool BInitializeStoreEntryPricePoints( econ_store_entry_t& out_entry, const CurrencyPricePointMap_t& mapCurrencyPricePoints, KeyValues *pKVPrice, int nSalePercent )
{
	if ( !pKVPrice )
		return false;

	FOR_EACH_CURRENCY( eCurrency )
	{
		const price_point_map_key_t key = { (item_price_t)pKVPrice->GetInt(), eCurrency };
		const CurrencyPricePointMap_t::IndexType_t unIdx = mapCurrencyPricePoints.Find( key );

		// Looking for a price point that doesn't exist, or doesn't exist for this currency?
		if ( unIdx == mapCurrencyPricePoints.InvalidIndex() )
		{
			continue;
		}

		// Weird initialization pattern: we're making sure that the value we read from the KeyValues
		// block is the value we're storing in memory. We do this to avoid integer conversion problems,
		// especially overflow (!).
		const item_price_t unPrice = mapCurrencyPricePoints[unIdx];
		out_entry.SetBasePrice( eCurrency, unPrice );
#pragma warning(push)
#pragma warning(disable : 4389)
		Assert( out_entry.GetBasePrice( eCurrency ) == unPrice );	// NOTE: DO NOT CAST unPrice - CHECKING FOR OVERFLOW
#pragma warning(pop)

		if ( ( nSalePercent > 0 ) && ( nSalePercent < 100 ) )
		{
			const item_price_t unSalePrice = out_entry.CalculateSalePrice( out_entry.GetBasePrice( eCurrency ), eCurrency, (float)nSalePercent );
			out_entry.SetSalePrice( eCurrency, unSalePrice );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Parses the KV section piece and add a econ_store_entry_t
//-----------------------------------------------------------------------------
bool CEconStorePriceSheet::BInitEntryFromKV( KeyValues *pKVEntry )
{
	const bool bIsPackItem = pKVEntry->GetBool( "is_pack_item", false );

#if defined( CLIENT_DLL )
	// Skip pack items on the client
	if ( bIsPackItem )
		return true;
#endif

	econ_store_entry_t entry;
	entry.InitCategoryTags( pKVEntry->GetString( "category_tags" ) );

	const bool bIsNew = entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_New );
	entry.m_bLimited = entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Limited );
	entry.m_bNew = bIsNew;
	entry.m_bPreviewAllowed = !bIsNew && entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Weapons );
	entry.m_bIsMarketItem = false;
	const bool bIsHighlighted = entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Highlighted );
	entry.m_bHighlighted = bIsHighlighted;

	if ( entry.GetCategoryTagCount() == 0 )
	{
		AssertMsg1( 0, "Unable to get category_tags for entry: %s", pKVEntry->GetName() );
		return false;
	}

	const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pKVEntry->GetString( "item_link" ) );
	if ( !pDef )
	{
		AssertMsg1( 0, "Unable to find item: %s", pKVEntry->GetString( "item_link" ) );
		return false;
	}

	entry.SetItemDefinitionIndex( pDef->GetDefinitionIndex() );

	int iQuantity = pKVEntry->GetInt( "quantity", 1 );
	entry.SetQuantity( iQuantity );
	Assert( entry.GetQuantity() == iQuantity );

	entry.SetSteamGiftPackageID( pKVEntry->GetUint64( "steam_gift_package_id", 0 ) );

#if defined( TF_CLIENT_DLL ) || defined( TF_GC_DLL )
	entry.SetDate( pDef->GetFirstSaleDate() );
#else
	entry.SetDate( pKVEntry->GetString( "date", "1/1/1900" ) );
#endif

	// Is the item sold out of the store?
	entry.m_bSoldOut = pKVEntry->GetBool( "sold_out", false );

	// Is the item a pack item?
	entry.m_bIsPackItem = bIsPackItem;

	int nSalePercent = pKVEntry->GetInt( "sale_percentage", 0 );
	if ( ( nSalePercent < 0 ) || ( nSalePercent >= 100 ) )
	{
		AssertMsg( false, "Sale percentage for %s set to an invalid value: %d\n", pDef->GetDefinitionName(), nSalePercent );
		return false;
	}

	if ( !BInitializeStoreEntryPricePoints( entry, m_mapCurrencyPricePoints, pKVEntry->FindKey( "base_price" ), nSalePercent ) )
		return false;

	m_mapEntries.InsertOrReplace( entry.GetItemDefinitionIndex(), entry );
	return true;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Parses the KV section piece and add a econ_store_entry_t
//-----------------------------------------------------------------------------
bool CEconStorePriceSheet::BInitMarketEntryFromKV( KeyValues *pKVEntry )
{
	//"The Alien Cranium"
	//{
	//	"item_link" "The Alien Cranium"
	//	"category_tags" "Cosmetics+Halloween"
	//	"date" "11/13/2014"
	//	"base_price" "1299"
	//}

	econ_store_entry_t entry;
	entry.InitCategoryTags( pKVEntry->GetString( "category_tags" ) );

	const bool bIsNew = entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_New );
	entry.m_bLimited = entry.IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Limited );
	entry.m_bNew = bIsNew;
	entry.m_bPreviewAllowed = false;
	entry.m_bIsMarketItem = true;

	if ( entry.GetCategoryTagCount() == 0 )
	{
		AssertMsg1( 0, "Unable to get category_tags for entry: %s", pKVEntry->GetName() );
		return false;
	}

	const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pKVEntry->GetString( "item_link" ) );
	if ( !pDef )
	{
		AssertMsg1( 0, "Unable to find item: %s", pKVEntry->GetString( "item_link" ) );
		return false;
	}

	entry.SetItemDefinitionIndex( pDef->GetDefinitionIndex() );
	entry.SetQuantity( 1 );
	entry.SetDate( pDef->GetFirstSaleDate() );		// FIXME?

	entry.m_bSoldOut = false;
	entry.m_bIsPackItem = false;

	// Entry should never already exist in mapEntries
	if ( m_mapEntries.Find( entry.GetItemDefinitionIndex() ) != m_mapEntries.InvalidIndex() )
	{
		AssertMsg1( 0, "Market Entry already eixsts : %s", pKVEntry->GetString( "item_link" ) );
		return false;
	}

	m_mapEntries.InsertOrReplace( entry.GetItemDefinitionIndex(), entry );
	return true;
}
#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
// Performs calculation of a discounted price given a base price, and will then handle ensuring the appropriate number of zeros at the end of the price via rounding
//-----------------------------------------------------------------------------
/*static*/ item_price_t econ_store_entry_t::GetDiscountedPrice( item_price_t unBasePrice, ECurrency eCurrency, float fDiscountPercentage )
{
	Assert( fDiscountPercentage > 0.0f );
	Assert( fDiscountPercentage < 100.0f );

	//if the base price is zero, it should never be anything but zero
	if( unBasePrice == 0 )
		return unBasePrice;

	item_price_t unNewPrice = ( item_price_t )( ( double )unBasePrice * ( clamp( 100.0 - fDiscountPercentage, 0.0, 100.0 ) / 100.0 ) );

	//determine what the unit of granularity we want to use for pricing. For example if you use 1, we keep 1/100th level precision, if you use 100 we'll round it to the nearest 100th (in USD this would be round to
	//the nearest dollar, etc)
	//this information can be found on the partner site documentation for supported currencies
	item_price_t unPriceGranularity = 1;
	switch ( eCurrency )
	{
	case k_ECurrencyRUB: unPriceGranularity = 100;		break;
	case k_ECurrencyJPY: unPriceGranularity = 100;		break;
	case k_ECurrencyNOK: unPriceGranularity = 100;		break;
	case k_ECurrencyPHP: unPriceGranularity = 100;		break;
	case k_ECurrencyTHB: unPriceGranularity = 100;		break;
	case k_ECurrencyKRW: unPriceGranularity = 1000;		break;
	case k_ECurrencyUAH: unPriceGranularity = 100;		break;
	case k_ECurrencyIDR: unPriceGranularity = 100;		break;
	case k_ECurrencyVND: unPriceGranularity = 50000;	break;
	case k_ECurrencyCNY: unPriceGranularity = 100;		break;
	case k_ECurrencyTWD: unPriceGranularity = 100;		break;
	case k_ECurrencyINR: unPriceGranularity = 100;		break;
	case k_ECurrencyCOP: unPriceGranularity = 100;		break;
	case k_ECurrencyCLP: unPriceGranularity = 100;		break;
	case k_ECurrencyCRC: unPriceGranularity = 500;		break;
	case k_ECurrencyUYU: unPriceGranularity = 100;		break;
	case k_ECurrencyKZT: unPriceGranularity = 100;		break;
	}

	//now handle the rounding to the specified price granularity
	if( unPriceGranularity > 1 )
	{
		const item_price_t unRemainder = unNewPrice % unPriceGranularity;
		//make sure to never let the price go to zero though
		if( ( unRemainder >= unPriceGranularity / 2 ) || ( unNewPrice < unPriceGranularity ) )
		{
			//round up
			unNewPrice += unPriceGranularity - unRemainder;
		}
		else
		{
			//round down
			unNewPrice -= unRemainder;
		}

		//sanity check
		Assert( ( unNewPrice % unPriceGranularity == 0 ) && ( unNewPrice > 0 ) );
	}

	return unNewPrice;
}

//-----------------------------------------------------------------------------
// Purpose: This will calculate what the sale price will be for a particular
// item -- the result should be non-zero, but this does not mean the item is
// currently on sale. You can optionally pass in a pointer to a uint32 which
// will get you the actual discount percentage, which can be different for
// NXP, which has its own nonlinear pricing structure.
//-----------------------------------------------------------------------------
/*static*/ item_price_t econ_store_entry_t::CalculateSalePrice( item_price_t unPreDiscountPrice, ECurrency eCurrency, float fDiscountPercentage, int32 *out_pAdjustedDiscountPercentage/*=NULL*/ )
{
	item_price_t unSalePrice = 0;
/*
	// TF2 doesn't support NXP or RMB yet
	if ( eCurrency == k_ECurrencyNXP || eCurrency == k_ECurrencyRMB )
	{
		// For these currencies, we calculate the sale price based on the discount percentage times the *USD* base price -- rather than the discount percentage
		// times the base price for the given currency.
		const item_price_t unSalePrice_USD = econ_store_entry_t::GetDiscountedPrice( unPreDiscountPrice, k_ECurrencyUSD, fDiscountPercentage );
		unSalePrice = ( eCurrency == k_ECurrencyNXP ) ? ConvertUSDToNXP( unSalePrice_USD ) : ConvertUSDToRMB( unSalePrice_USD );

		// Ensure that the sale price is strictly less
		Assert( unSalePrice < unPreDiscountPrice );
		if ( unSalePrice >= unPreDiscountPrice )
		{
			unSalePrice = unPreDiscountPrice;
		}
	}
	else
*/
	{
		unSalePrice = econ_store_entry_t::GetDiscountedPrice( unPreDiscountPrice, eCurrency, fDiscountPercentage );
	}

	Assert( unSalePrice > 0 );

	if ( out_pAdjustedDiscountPercentage )
	{
		// Also set a percentage per currency, since they can be different. RMB and NXP, for example, 
		// calculate a sale price based on the USD sale price.
		const bool bUseDefaultDiscountPercentage = ( eCurrency == k_ECurrencyUSD );
		const double fActualPercent = 100.0 * ( 1.0 - (double)unSalePrice / (double)unPreDiscountPrice );

		const int32 nDiscountPercentageForCurrency = bUseDefaultDiscountPercentage ?
			RoundFloatToInt( fDiscountPercentage ) :
			RoundFloatToInt( fActualPercent );
		AssertMsg( nDiscountPercentageForCurrency >= 0 && nDiscountPercentageForCurrency < 100, "Invalid discount percentage of %u specified for item ( currency %u )", nDiscountPercentageForCurrency, eCurrency );

		*out_pAdjustedDiscountPercentage = nDiscountPercentageForCurrency;
	}

	return unSalePrice;
}

//----------------------------------------------------------------------------
// Purpose: Sort Entries by Name
//----------------------------------------------------------------------------
int ItemNameSortComparator( const econ_store_entry_t *const *ppEntryA, const econ_store_entry_t *const *ppEntryB )
{
	Assert( ppEntryA );
	Assert( ppEntryB );
	Assert( *ppEntryA );
	Assert( *ppEntryB );

	const CEconItemDefinition *pItemDefA = GetItemSchema()->GetItemDefinition( (*ppEntryA)->GetItemDefinitionIndex() );
	const CEconItemDefinition *pItemDefB = GetItemSchema()->GetItemDefinition( (*ppEntryB)->GetItemDefinitionIndex() );

	int nComp = Q_stricmp( pItemDefA->GetItemTypeName(), pItemDefB->GetItemTypeName() );
	if ( nComp != 0 )
		return nComp;

	// If the type matches, then sort by the item name
	return Q_stricmp( pItemDefA->GetItemBaseName(), pItemDefB->GetItemBaseName() );
}

//-----------------------------------------------------------------------------
// Purpose: Sort by whatever sort type our price sheet is requesting
//-----------------------------------------------------------------------------
int FirstSaleDateSortComparator( const econ_store_entry_t *const *ppItemA, const econ_store_entry_t *const *ppItemB )
{
	Assert( ppItemA );
	Assert( ppItemB );
	Assert( *ppItemA );
	Assert( *ppItemB );

	const CEconItemDefinition *pItemDefA = GetItemSchema()->GetItemDefinition( (*ppItemA)->GetItemDefinitionIndex() );
	const CEconItemDefinition *pItemDefB = GetItemSchema()->GetItemDefinition( (*ppItemB)->GetItemDefinitionIndex() );
	Assert( pItemDefA );
	Assert( pItemDefB );

	const char *pDateAddedA = pItemDefA->GetFirstSaleDate();
	const char *pDateAddedB = pItemDefB->GetFirstSaleDate();

	return -strcmp( pDateAddedA, pDateAddedB );
}

bool CEconStoreEntryLess::Less( const uint16& lhs, const uint16& rhs, void *pContext )
{
	CEconItemSchema *pSchema = GetItemSchema();

	CEconStorePriceSheet* pPriceSheet = (CEconStorePriceSheet*)pContext;
	eEconStoreSortType sortType = pPriceSheet->GetEconStoreSortType();
	const econ_store_entry_t *pItemA = pPriceSheet->GetEntry( lhs );
	const econ_store_entry_t *pItemB = pPriceSheet->GetEntry( rhs );
	Assert( pItemA );
	Assert( pItemB );

	CEconItemDefinition *pItemDefA = pSchema->GetItemDefinition( pItemA->GetItemDefinitionIndex() );
	CEconItemDefinition *pItemDefB = pSchema->GetItemDefinition( pItemB->GetItemDefinitionIndex() );

#ifdef CLIENT_DLL
	ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency(); 
#else
	ECurrency eCurrency = k_ECurrencyUSD;
#endif

	if ( pItemDefA && pItemDefB )
	{
		switch ( sortType )
		{
		case kEconStoreSortType_Price_HighestToLowest:
			if ( pItemA->GetCurrentPrice( eCurrency ) == pItemB->GetCurrentPrice( eCurrency ) )
			{
				return Q_strcmp( pItemDefA->GetItemBaseName(), pItemDefB->GetItemBaseName() ) < 0;
			}
			return pItemA->GetCurrentPrice( eCurrency ) > pItemB->GetCurrentPrice( eCurrency );

		case kEconStoreSortType_Price_LowestToHighest:
			if ( pItemA->GetCurrentPrice( eCurrency ) == pItemB->GetCurrentPrice( eCurrency ) )
			{
				return Q_strcmp( pItemDefA->GetItemBaseName(), pItemDefB->GetItemBaseName() ) < 0;
			}
			return pItemA->GetCurrentPrice( eCurrency ) < pItemB->GetCurrentPrice( eCurrency );

		case kEconStoreSortType_DevName_AToZ:
			return Q_strcmp( pItemDefA->GetItemBaseName(), pItemDefB->GetItemBaseName() ) < 0;
			
		case kEconStoreSortType_DevName_ZToA:
			return Q_strcmp( pItemDefA->GetItemBaseName(), pItemDefB->GetItemBaseName() ) > 0;

		case kEconStoreSortType_DateNewest:
		case kEconStoreSortType_DateOldest:
		{
			int iSortResult = FirstSaleDateSortComparator( &pItemA, &pItemB );

			if ( iSortResult < 0 )
				return sortType == kEconStoreSortType_DateNewest;

			if ( iSortResult > 0 )
				return sortType == kEconStoreSortType_DateOldest;

			// Intentionally fall through to sorting by localized name if our dates match.
		}
			
		case kEconStoreSortType_Name_AToZ:
		case kEconStoreSortType_Name_ZToA:
			if ( g_pVGuiLocalize )
			{
				wchar_t *wszItemNameA = g_pVGuiLocalize->Find( pItemDefA->GetItemBaseName() );
				wchar_t *wszItemNameB = g_pVGuiLocalize->Find( pItemDefB->GetItemBaseName() );
				if ( wszItemNameA && wszItemNameB )
				{
					// Note: locale-savvy string sorting uses wcscoll, not wcscmp
					return sortType == kEconStoreSortType_Name_ZToA
						 ? wcscoll( wszItemNameA, wszItemNameB ) > 0			// only sort in reverse alphabetical order if asked to -- fall-through cases uses forward order
						 : wcscoll( wszItemNameA, wszItemNameB ) < 0;
				}				
			}
			break;

		case kEconStoreSortType_ItemDefIndex:
			return pItemDefA->GetDefinitionIndex() < pItemDefB->GetDefinitionIndex();
			
		case kEconStoreSortType_ReverseItemDefIndex:
			return pItemDefB->GetDefinitionIndex() < pItemDefA->GetDefinitionIndex();
		}
	}

	// default, highest to lowest price
	return pItemA->GetCurrentPrice( eCurrency ) > pItemB->GetCurrentPrice( eCurrency );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: return the CLocale name that works with setlocale()
//-----------------------------------------------------------------------------
const char *GetLanguageCLocaleName( ELanguage eLang )
{
	if ( eLang == k_Lang_None )
		return "";

#ifdef _WIN32
	// table for Win32 is here: http://msdn.microsoft.com/en-us/library/hzz3tw78(v=VS.80).aspx
	// shortname works except for chinese

	switch ( eLang )
	{
	case k_Lang_Simplified_Chinese:
		return "chs"; // or "chinese-simplified"
	case k_Lang_Traditional_Chinese:
		return "cht"; // or "chinese-traditional"
	case k_Lang_Korean:
		return "korean"; // steam likes "koreana" for the name for some reason.
	case k_Lang_Brazilian:
		return "ptb"; // "portuguese-brazil" - that string fails even though it's in the MS lang table; ptb does work.
	default:
		return GetLanguageShortName( eLang );
	}

#else
	switch ( eLang )
	{
		case k_Lang_Simplified_Chinese:
		case k_Lang_Traditional_Chinese:
			return "zh_CN";
		default:
			;
	}

	// ICU codes work on linux/osx
	return GetLanguageICUName( eLang );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Get an I/O stream into the right local/settings for printing money - so to speak
//-----------------------------------------------------------------------------
static void InitStreamLocale( std::wostringstream &stream, ELanguage eLang, uint32 nExpectedAmount, ECurrency eCurrencyCode )
{
	const char *pszLocale = GetLanguageCLocaleName( eLang );

#ifdef _PS3
	stream.imbue(std::locale(pszLocale)); // no exception for PS3
#else
	try
	{
		stream.imbue(std::locale(pszLocale));
	}
	catch (const std::exception &e)
	{
		Log( "stream::imbue() failed with locale: '%s', exception: %s\n", pszLocale, e.what() );
		stream.imbue( std::locale("C") );
	}
#endif

	// Don't display fractional rubles
	// But if our amount is fractional, we should show it regardless
	// hack hack - certain currencies should not show fractional symbol - see if we can wire this through from config at some point
	if ( ( eCurrencyCode == k_ECurrencyRUB || 
		   eCurrencyCode == k_ECurrencyJPY || 
		   eCurrencyCode == k_ECurrencyIDR || 
		   eCurrencyCode == k_ECurrencyVND ||
		   eCurrencyCode == k_ECurrencyKRW ||
		   eCurrencyCode == k_ECurrencyUAH ||
		   eCurrencyCode == k_ECurrencyCNY ||
		   eCurrencyCode == k_ECurrencyCOP ||
		   eCurrencyCode == k_ECurrencyTWD ||
		   eCurrencyCode == k_ECurrencyINR ||
		   eCurrencyCode == k_ECurrencyCRC ||
		   eCurrencyCode == k_ECurrencyUYU ||
		   eCurrencyCode == k_ECurrencyKZT )
		&& nExpectedAmount % 100 == 0 )
	{
		stream.precision( 0 );
		stream.setf( std::ios_base::fixed );
	}
	else
	{
		stream.precision( 2 );
		stream.setf( std::ios_base::fixed, std::ios_base::floatfield );
	}
}

//
// Partial Integration from \steam\main\src\common\amount.cpp
// 
int MakeMoneyStringInternal( wchar_t *pchDest, uint32 nDest, item_price_t unPrice, ECurrency eCurrencyCode, ELanguage eLanguage )
{							  
	// Use the actual currency symbol with the local number formatting.
	// assume local locale - should not be used from server to send to client
	// without passing in a valid pszCLocale parameter.
	std::wostringstream stream;
	InitStreamLocale( stream, eLanguage, unPrice, eCurrencyCode );

	stream << (unPrice/100.0);

	const auto sAmount = stream.str();
	const wchar_t *wszAmount = sAmount.c_str();

	// this code will be used by the client. An old client might encounter a currency code it doesn't know about. Handle that.
	if ( eCurrencyCode == k_ECurrencyInvalid )
	{
		// just return the value
		return V_snwprintf( pchDest, nDest, L"%ls", wszAmount );
	}
	
	// select symbol
	const char *pchSymbol = "";
	switch( eCurrencyCode )
	{
	case k_ECurrencyUSD:
		pchSymbol = "$";
		break;

	case k_ECurrencyGBP:
		pchSymbol = "\xC2\xA3";
		break;

	case k_ECurrencyEUR:
		pchSymbol = "\xE2\x82\xAC";
		break;

	case k_ECurrencyCHF:
		pchSymbol = "CHF";
		break;

	case k_ECurrencyRUB:
		pchSymbol = "\xD1\x80\xD1\x83\xD0\xB1"; // localized py6
		break;

	case k_ECurrencyBRL:
		pchSymbol = "R$";
		break;

	case k_ECurrencyJPY:
		pchSymbol = "\xC2\xA5";
		break;

	case k_ECurrencyIDR:
		pchSymbol = "Rp";
		break;

	case k_ECurrencyMYR:
		pchSymbol = "RM";
		break;

	case k_ECurrencyPHP:
		pchSymbol = "\xE2\x82\xB1";
		break;

	case k_ECurrencySGD:
		pchSymbol = "S$";
		break;

	case k_ECurrencyTHB:
		pchSymbol = "\xE0\xB8\xBF";
		break;

	case k_ECurrencyVND:
		pchSymbol = "\xE2\x82\xAB";
		break;

	case k_ECurrencyKRW:
		pchSymbol = "\xe2\x82\xa9";
		break;

	case k_ECurrencyTRY:
		pchSymbol = "TL";
		break;

	case k_ECurrencyUAH:
		pchSymbol = "\xe2\x82\xb4";
		break;

	case k_ECurrencyMXN:
		pchSymbol = "Mex$";
		break;

	case k_ECurrencyCAD:
		pchSymbol = "C$";
		break;

	case k_ECurrencyAUD:
		pchSymbol = "A$";
		break;

	case k_ECurrencyNZD:
		pchSymbol = "NZ$";
		break;

	case k_ECurrencyNOK:
		pchSymbol = "kr";
		break;

	case k_ECurrencyPLN:
		pchSymbol = "z\xc5\x82";
		break;

	case k_ECurrencyCNY:
		pchSymbol = "\xc2\xa5";
		break;

	case k_ECurrencyINR:
		pchSymbol = "\xe2\x82\xb9";
		break;

	case k_ECurrencyCLP:
		pchSymbol = "CLP$";
		break;

	case k_ECurrencyPEN:
		pchSymbol = "S/.";
		break;

	case k_ECurrencyCOP:
		pchSymbol = "COL$";
		break;

	case k_ECurrencyZAR:
		pchSymbol = "R";
		break;

	case k_ECurrencyHKD:
		pchSymbol = "HK$";
		break;

	case k_ECurrencyTWD:
		pchSymbol = "NT$";
		break;

	case k_ECurrencySAR:
		pchSymbol = "SR";
		break;

	case k_ECurrencyAED:
		pchSymbol = "DH";
		break;

	case k_ECurrencyARS:
		pchSymbol = "ARS$";
		break;

	case k_ECurrencyCRC:
		pchSymbol = "\xE2\x82\xA1";
		break;

	case k_ECurrencyILS:
		pchSymbol = "\xE2\x82\xAA";
		break;

	case k_ECurrencyKWD:
		pchSymbol = "KD";
		break;

	case k_ECurrencyQAR:
		pchSymbol = "QR";
		break;

	case k_ECurrencyUYU:
		pchSymbol = "$U";
		break;

	case k_ECurrencyKZT:
		pchSymbol = "\xE2\x82\xB8";
		break;

	case k_ECurrencyBYN:
		pchSymbol = "Br";
		break;

	default:
		AssertMsg( false, "Unknown currency code" );
		pchSymbol = "$";
		break;
	}

	// BEGIN HACK GAME CLIENT CHARACTER SET CONVERSION
	wchar_t wsSymbol[ 16 ];
	V_UTF8ToUnicode( pchSymbol, wsSymbol, ARRAYSIZE( wsSymbol ) );
	// END HACK GAME CLIENT CHARACTER SET CONVERSION

	bool bFirstSymbolThenAmount = true;	// Whether to show "$5" or "5E"
	bool bSpaceBetweenTokens = false; // Whether to render a space between tokens like "$5" has no space, but "5 pyb" has a space

	switch ( eCurrencyCode )
	{
	case k_ECurrencyEUR:
	case k_ECurrencyUAH:
		bFirstSymbolThenAmount = false;
		bSpaceBetweenTokens = false;
		break;
	case k_ECurrencyRUB:
	case k_ECurrencyVND:
	case k_ECurrencyNOK:
	case k_ECurrencyTRY:
	case k_ECurrencyPLN:
	case k_ECurrencySAR:
	case k_ECurrencyAED:
	case k_ECurrencyKWD:
	case k_ECurrencyQAR:
		bFirstSymbolThenAmount = false;
		bSpaceBetweenTokens = true;
		break;
	case k_ECurrencyIDR:
	case k_ECurrencyMXN:
	case k_ECurrencyCAD:
	case k_ECurrencyAUD:
	case k_ECurrencyNZD:
	case k_ECurrencyCOP:
	case k_ECurrencyZAR:
	case k_ECurrencyHKD:
	case k_ECurrencyTWD:
	case k_ECurrencyCHF:
	case k_ECurrencyARS:
	case k_ECurrencyCLP:
	case k_ECurrencyINR:
		bFirstSymbolThenAmount = true;
		bSpaceBetweenTokens = true;
		break;
	default:
		bFirstSymbolThenAmount = true;
		bSpaceBetweenTokens = false;
	}

	return V_snwprintf( pchDest, nDest, L"%ls%ls%ls",
		( bFirstSymbolThenAmount ? wsSymbol : wszAmount ),
		( bSpaceBetweenTokens ? L" " : L"" ),
		( bFirstSymbolThenAmount ? wszAmount : wsSymbol ) );
}

static ELanguage GetStoreLanguage()
{
	if ( !engine )
		return k_Lang_English;

	char uilanguage[ 64 ];
	uilanguage[0] = 0;
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	return PchLanguageToELanguage( uilanguage );
}

void MakeMoneyString( wchar_t *pchDest, uint32 nDest, item_price_t unPrice, ECurrency eCurrencyCode )
{
	(void)MakeMoneyStringInternal( pchDest, nDest, unPrice, eCurrencyCode, GetStoreLanguage() );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconStorePriceSheet::BItemExistsInPriceSheet( item_definition_index_t unDefIndex ) const
{
	CEconStoreCategoryManager *pCategoryManager = GEconStoreCategoryManager();
	const int nCategoryCount = pCategoryManager->GetNumCategories();

	for ( int i = 0; i < nCategoryCount; ++i )
	{
		const CEconStoreCategoryManager::StoreCategory_t *pCat = pCategoryManager->GetCategoryFromIndex( i );

		// Intentionally not using CUtlSortVector<>::Find(), since it calls Less(), which is slow for m_vecEntries.
		FOR_EACH_VEC( pCat->m_vecEntries, j )
		{
			if ( pCat->m_vecEntries[j] == unDefIndex )
				return true;
		}
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ShouldUseNewStore()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetStoreVersion()
{
	return 2;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CEconStorePriceSheet *GetEconPriceSheet()
{
	return EconUI() && EconUI()->GetStorePanel()
		 ? EconUI()->GetStorePanel()->GetPriceSheet()
		 : NULL;
}

