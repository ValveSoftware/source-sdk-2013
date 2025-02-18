//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Common objects and utilities related to the in-game item store
//
//=============================================================================

#ifndef ECON_STORE_H
#define ECON_STORE_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlSortVector.h"
#include "vstdlib/IKeyValuesSystem.h"
#include "econ/econ_storecategory.h"
#ifdef CLIENT_DLL
#include "client_community_market.h"
#endif // CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Error code enum for purchase messages
//-----------------------------------------------------------------------------
enum EPurchaseResult
{
	k_EPurchaseResultOK	= 1,					// Success
	k_EPurchaseResultFail = 2,					// Generic error
	k_EPurchaseResultInvalidParam = 3,			// Invalid parameter
	k_EPurchaseResultInternalError = 4,			// Internal error
	k_EPurchaseResultNotApproved = 5,			// Tried to finalize a transaction that has not yet been approved
	k_EPurchaseResultAlreadyCommitted = 6,		// Tried to finalize a transaction that has already been committed
	k_EPurchaseResultUserNotLoggedIn = 7,		// User is not logged into Steam
	k_EPurchaseResultWrongCurrency = 8,			// Microtransaction's currency does not match user's wallet currency
	k_EPurchaseResultAccountError = 9,			// User's account does not exist or is temporarily unavailable
	k_EPurchaseResultInvalidItem = 10,			// User is trying to purchase an item that doesn't exist or is not for sale
	k_EPurchaseResultNotEnoughBackpackSpace = 11,	// User did not have enough backpack space
	k_EPurchaseResultLimitedQuantityItemsUnavailable = 12,	// User tried to purchase limited-quantity items but there weren't enough left in stock

	k_EPurchaseResultInsufficientFunds = 100,	// User does not have wallet funds
	k_EPurchaseResultTimedOut = 101,			// Time limit for finalization has been exceeded
	k_EPurchaseResultAcctDisabled = 102,		// Steam account is disabled
	k_EPurchaseResultAcctCannotPurchase = 103,	// Steam account is not allowed to make a purchase
	k_EMicroTxnResultFailedFraudChecks = 104,   // Fraud checks inside of Steam failed

	k_EPurchaseResultOldPriceSheet = 150,		// Information on the purchase didn't match the current price sheet
	k_EPurchaseResultTxnNotFound = 151			// Could not find the transaction specified
};


const char *PchNameFromEPurchaseResult( EPurchaseResult ePurchaseState );

//-----------------------------------------------------------------------------
// Purpose: State of a transaction
//
// WARNING: VALUES STORED IN DATABASE. DO NOT RENUMBER!!!
//-----------------------------------------------------------------------------
enum EPurchaseState
{
	k_EPurchaseStateInvalid = 0,				// Invalid
	k_EPurchaseStateInit = 1,					// We have sent InitPurchase to Steam
	k_EPurchaseStateWaitingForAuthorization = 2, // We have gotten initial authorization from Steam. Waiting for user to authorize.
	k_EPurchaseStatePending = 3,				// We are attempting to commit the transaction
	k_EPurchaseStateComplete = 4,				// The transaction was successful
	k_EPurchaseStateFailed = 5,					// The transaction failed
	k_EPurchaseStateCanceled = 6,				// The transaction was canceled
	k_EPurchaseStateRefunded = 7,				// The transaction was refunded
	k_EPurchaseStateChargeback = 8,				// The transaction was charged back
	k_EPurchaseStateChargebackReversed = 9,		// A chargeback has failed and we got the money
	k_EPurchaseStateLast = k_EPurchaseStateChargebackReversed,
};

const char *PchNameFromEPurchaseState( EPurchaseState ePurchaseState );

// DO NOT RENUMBER! These values are stored in the audit log table.
enum EGCTransactionAuditReason
{
	k_EGCTransactionAudit_GCTransactionCompleted = 0,				// The transaction completed successfully on the GC.
	k_EGCTransactionAudit_GCTransactionInit = 1,					// The transaction was initialized.
	k_EGCTransactionAudit_GCTransactionPostInit = 2,				// The result of attempting to initialize the transaction. This is where the SteamTxnID is set.
	k_EGCTransactionAudit_GCTransactionFinalize = 3,				// We have started to finalize the transaction (so probably set it to pending).
	k_EGCTransactionAudit_GCTransactionFinalizeFailed = 4,			// Our attempt to finalize the transaction failed for some reason (not due to a timeout).
	k_EGCTransactionAudit_GCTransactionCanceled = 5,				// The client requested that we cancel the transaction.
	k_EGCTransactionAudit_SteamFailedMismatch = 6,					// Steam failed the transaction but we did not.
	k_EGCTransactionAudit_GCRemovePurchasedItems = 7,				// We are attempting to remove the purchased items from their backpack (due to rollback or failure).
	k_EGCTransactionAudit_GCTransactionInsert = 8,					// We are inserting a transaction, usually one created by a web interface (i.e.: a cd-key operation) instead of a standard store interaction.
	k_EGCTransactionAudit_GCTransactionCompletedPostChargeback = 9,	// We thought this transaction had been charged back but Steam came back later and told us it was successful after all.

	k_EGCTransactionAuditLast = k_EGCTransactionAudit_GCTransactionCompletedPostChargeback,
};

const char *PchNameFromEGCTransactionAuditReason( EGCTransactionAuditReason eAuditReason );

enum EGCTransactionAuditInsertReason
{
	k_EGCTransactionAuditInsert_Invalid = 0,					// 
	k_EGCTransactionAuditInsert_CDKey = 1,						// A CD Key was used for this transaction.
	k_EGCTransactionAuditInsert_SettlementNoMatch_Failed = 2,	// We inserted a failed record during settlement to unblock the process.
	k_EGCTransactionAuditInsert_SettlementNoMatch_Pending = 3,	// We inserted a pending record during settlement to unblock the process.

	k_EGCTransactionAuditInsertLast = k_EGCTransactionAuditInsert_SettlementNoMatch_Pending,
};

//-----------------------------------------------------------------------------
// Purpose: Currencies we support
//
// WARNING: VALUES STORED IN DATABASE. DO NOT RENUMBER!!!
// WARNING: THESE DON'T MATCH THE STEAM NUMERIC IDS!!! WE TALK USING CURRENCY
//			CODES LIKE "VND" AND IF YOU SAY "15" TERRIBLE THINGS WILL HAPPEN
//-----------------------------------------------------------------------------
enum ECurrency
{
	k_ECurrencyFirst = 0,
	k_ECurrencyUSD = 0,
	k_ECurrencyGBP = 1,
	k_ECurrencyEUR = 2,
	k_ECurrencyRUB = 3,
	k_ECurrencyBRL = 4,
	// space for Dota currencies
	k_ECurrencyJPY = 8,
	k_ECurrencyNOK = 9,
	k_ECurrencyIDR = 10,
	k_ECurrencyMYR = 11,
	k_ECurrencyPHP = 12,
	k_ECurrencySGD = 13,
	k_ECurrencyTHB = 14,
	k_ECurrencyVND = 15,
	k_ECurrencyKRW = 16,
	k_ECurrencyTRY = 17,
	k_ECurrencyUAH = 18,
	k_ECurrencyMXN = 19,
	k_ECurrencyCAD = 20,
	k_ECurrencyAUD = 21,
	k_ECurrencyNZD = 22,
	k_ECurrencyPLN = 23,
	k_ECurrencyCHF = 24,
	k_ECurrencyCNY = 25,
	k_ECurrencyTWD = 26,
	k_ECurrencyHKD = 27,
	k_ECurrencyINR = 28,
	k_ECurrencyAED = 29,
	k_ECurrencySAR = 30,
	k_ECurrencyZAR = 31,
	k_ECurrencyCOP = 32,
	k_ECurrencyPEN = 33,
	k_ECurrencyCLP = 34,
	k_ECurrencyARS = 35,
	k_ECurrencyCRC = 36,
	k_ECurrencyILS = 37,
	k_ECurrencyKWD = 38,
	k_ECurrencyQAR = 39,
	k_ECurrencyUYU = 40,
	k_ECurrencyKZT = 41,
	k_ECurrencyBYN = 42,

	// NOTE: Not actually the Maximum currency value, but the Terminator for the possible currency code range.
	k_ECurrencyMax = 43,

	// make this a big number so we can avoid having to move it when we add another currency type
	k_ECurrencyInvalid = 255,
	k_ECurrencyCDKeyTransaction = k_ECurrencyInvalid,
};

// Macro for looping across all valid currencies
#define FOR_EACH_CURRENCY( _i ) for ( ECurrency _i = GetFirstValidCurrency(); _i != k_ECurrencyInvalid; _i = GetNextValidCurrency( _i ) )

const char *PchNameFromECurrency( ECurrency eCurrency );	// NOTE: Defined with ENUMSTRINGS_START/ENUMSTRINGS_REVERSE macros
ECurrency ECurrencyFromName( const char *pchName );			//

inline bool BIsCurrencyValid( ECurrency eCurrency )
{
	switch ( eCurrency )
	{
	case k_ECurrencyUSD:
	case k_ECurrencyGBP:
	case k_ECurrencyEUR:
	case k_ECurrencyRUB:
	case k_ECurrencyBRL:
	case k_ECurrencyJPY:
	case k_ECurrencyNOK:
	case k_ECurrencyIDR:
	case k_ECurrencyMYR:
	case k_ECurrencyPHP:
	case k_ECurrencySGD:
	case k_ECurrencyTHB:
	case k_ECurrencyVND:
	case k_ECurrencyKRW:
	// case k_ECurrencyTRY: // not valid since 2023
	case k_ECurrencyUAH:
	case k_ECurrencyMXN:
	case k_ECurrencyCAD:
	case k_ECurrencyAUD:
	case k_ECurrencyNZD:
	case k_ECurrencyPLN:
	case k_ECurrencyCHF:
	case k_ECurrencyCNY:
	case k_ECurrencyTWD:
	case k_ECurrencyHKD:
	case k_ECurrencyINR:
	case k_ECurrencyAED:
	case k_ECurrencySAR:
	case k_ECurrencyZAR:
	case k_ECurrencyCOP:
	case k_ECurrencyPEN:
	case k_ECurrencyCLP:
	// case k_ECurrencyARS: // not valid since 2023
	case k_ECurrencyCRC:
	case k_ECurrencyILS:
	case k_ECurrencyKWD:
	case k_ECurrencyQAR:
	case k_ECurrencyUYU:
	case k_ECurrencyKZT:
	//case k_ECurrencyBYN: // not yet launched as of Autumn 2017
		return true;
	}

	return false;
}

inline ECurrency GetFirstValidCurrency()
{
	for ( int i = k_ECurrencyFirst; i < k_ECurrencyMax; i++ )
	{
		if ( BIsCurrencyValid( (ECurrency)i ) )
			return (ECurrency)i;
	}
	return k_ECurrencyInvalid;
}

inline ECurrency GetNextValidCurrency( ECurrency ePrevious )
{
	for ( int i = ePrevious + 1; i < k_ECurrencyMax; i++ )
	{
		if ( BIsCurrencyValid( (ECurrency)i ) )
			return (ECurrency)i;
	}
	return k_ECurrencyInvalid;
}

//-----------------------------------------------------------------------------
// Purpose: Simple struct for pairing a sort type with a localization string
//-----------------------------------------------------------------------------
struct ItemSortTypeData_t
{
	const char *szSortDesc;		// localization string
	uint32		iSortType;		// maps to the GC-specific sort value
};

// Description of a single item for sale
struct econ_store_entry_t
{
	// Constructor
	econ_store_entry_t()
	:	m_pchCategoryTags( NULL ),
		m_unGiftSteamPackageID( 0 ),
		m_bHighlighted( false )
	{
		V_memset( m_unBaseCosts, 0, sizeof(m_unBaseCosts) );
		V_memset( m_unSaleCosts, 0, sizeof(m_unSaleCosts) );
	}

	void SetItemDefinitionIndex( item_definition_index_t usDefIndex );
	item_definition_index_t GetItemDefinitionIndex() const { return m_usDefIndex; }
	
	void InitCategoryTags( const char *pTags );	// Sets m_pchCategoryTags and initializes m_vecTagIds and m_fRentalPriceScale

	bool IsListedInCategory( StoreCategoryID_t unID ) const;	// Is this item listed in the given category?
	bool IsListedInSubcategories( const CEconStoreCategoryManager::StoreCategory_t &Category ) const;	// Is this item listed in one of Category's subcategories?
	bool IsListedInCategoryOrSubcategories( const CEconStoreCategoryManager::StoreCategory_t &Category ) const;	// Is this item listed in Category or one of Category's subcategories?
	
	bool IsOnSale( ECurrency eCurrency ) const;
	bool IsRentable() const;
#ifdef CLIENT_DLL
	bool HasDiscount( ECurrency eCurrency, item_price_t *out_punOptionalBasePrice ) const;			// returns true if we're on sale or if we're a bundle with a discounted total price
#endif // CLIENT_DLL
	item_price_t GetCurrentPrice( ECurrency eCurrency ) const;
	float GetRentalPriceScale() const;

	uint32 GetGiftSteamPackageID() const { return m_unGiftSteamPackageID; }

	static item_price_t CalculateSalePrice( item_price_t unPreDiscountPrice, ECurrency eCurrency, float fDiscountPercentage, int32 *out_pAdjustedDiscountPercentage = NULL );

	item_price_t GetBasePrice( ECurrency eCurrency ) const
	{
		Assert( eCurrency >= k_ECurrencyFirst );
		Assert( eCurrency < k_ECurrencyMax );
		if ( !( eCurrency >= 0 && eCurrency < k_ECurrencyMax ) )
			return 0;
#ifdef CLIENT_DLL
		if ( m_bIsMarketItem )
		{
			const client_market_data_t *pClientMarketData = GetClientMarketData( GetItemDefinitionIndex(), AE_UNIQUE );
			if ( !pClientMarketData )
				return 0;
			return pClientMarketData->m_unLowestPrice;
		}
#endif
		// Weird-looking pattern: we're making sure that the value we're about to return fits correctly
		// into the variable we're about to put it into. We do this to avoid integer conversion problems,
		// especially overflow (!) where someone changes one of the return type or the storage type but
		// not the other.
		Assert( (item_price_t)m_unBaseCosts[eCurrency] == m_unBaseCosts[eCurrency] );
		return m_unBaseCosts[eCurrency];
	}

	item_price_t GetSalePrice( ECurrency eCurrency ) const
	{
		Assert( eCurrency >= k_ECurrencyFirst );
		Assert( eCurrency < k_ECurrencyMax );
		if ( !( eCurrency >= 0 && eCurrency < k_ECurrencyMax ) )
			return 0;
#ifdef CLIENT_DLL
		if ( m_bIsMarketItem )
		{
			const client_market_data_t *pClientMarketData = GetClientMarketData( GetItemDefinitionIndex(), AE_UNIQUE );
			if ( !pClientMarketData )
				return 0;
			return pClientMarketData->m_unLowestPrice;
		}
#endif
		// Weird-looking pattern: we're making sure that the value we're about to return fits correctly
		// into the variable we're about to put it into. We do this to avoid integer conversion problems,
		// especially overflow (!) where someone changes one of the return type or the storage type but
		// not the other. 
		Assert( (item_price_t)m_unSaleCosts[eCurrency] == m_unSaleCosts[eCurrency] );
		return m_unSaleCosts[eCurrency];
	}

	uint16 GetQuantity() const
	{
		return m_usQuantity;
	}

	const char* GetDate() const
	{
		return m_strDate.Get();
	}

	bool CanPreview() const
	{
		// No previewing of new items or weapons.
		return m_bPreviewAllowed;
	}

	void SetQuantity( uint16 usQuantity )
	{
		Assert( usQuantity > 0 );
		m_usQuantity = usQuantity;
	}

	void ValidatePrice( ECurrency eCurrency, item_price_t unPrice );

	void SetBasePrice( ECurrency eCurrency, item_price_t unPrice )
	{
		Assert( eCurrency >= k_ECurrencyFirst );
		Assert( eCurrency < k_ECurrencyMax );
		if ( !( eCurrency >= 0 && eCurrency < k_ECurrencyMax ) )
			return;

		ValidatePrice( eCurrency, unPrice );

		m_unBaseCosts[eCurrency] = unPrice;
	}

	void SetSalePrice( ECurrency eCurrency, item_price_t unPrice )
	{
		Assert( eCurrency >= k_ECurrencyFirst );
		Assert( eCurrency < k_ECurrencyMax );
		if ( !( eCurrency >= 0 && eCurrency < k_ECurrencyMax ) )
			return;

		ValidatePrice( eCurrency, unPrice );

		// It's legal to have a sale price of zero, meaninig "this item is not on sale" in this
		// currency.
		// Assert( unPrice > 0 );
		m_unSaleCosts[eCurrency] = unPrice;
	}

	void SetSteamGiftPackageID( uint32 unGiftSteamPackageID )
	{
		m_unGiftSteamPackageID = unGiftSteamPackageID;
	}
	
	void SetDate( const char* pszDate )
	{
		m_strDate.Set( pszDate );
	}


	bool IsValidCategoryTagIndex( uint32 iIndex ) const
	{
		AssertMsg( m_vecCategoryTags.IsValidIndex( iIndex ), "Category tag index out of range." );
		return m_vecCategoryTags.IsValidIndex( iIndex );
	}

	uint32 GetCategoryTagCount() const
	{
		return m_vecCategoryTags.Count();
	}

	const char *GetCategoryTagNameFromIndex( uint32 iIndex ) const
	{
		if ( !IsValidCategoryTagIndex( iIndex ) )
			return NULL;

		return m_vecCategoryTags[ iIndex ].m_strName;
	}

	StoreCategoryID_t GetCategoryTagIDFromIndex( uint32 iIndex ) const;

	const char *GetCategoryTagString() const
	{
		return m_pchCategoryTags;
	}

	bool							m_bLimited;							// Item is a limited sale
	bool							m_bNew;								// Item is new
	bool							m_bHighlighted;						// Item is highlighted
	CUtlString						m_strDate;							// Date Added
	bool							m_bSoldOut;							// True if the item is sold out from the store (for example if the item is a ticket or another physical item)
	bool							m_bPreviewAllowed;					// Is this item previewable?
	bool							m_bIsPackItem;						// Is this item a pack item? Pack items are items which are not individually for sale, but are sold via a bundle known as a "pack bundle"

	bool							m_bIsMarketItem;					// Is Market Item Link

private:
	// Helper function -- so we do this calculation in a single place. Use CalculateSalePrice() instead of trying to call this directly.
	static item_price_t GetDiscountedPrice( item_price_t unBasePrice, ECurrency eCurrency, float fDiscountPercentage );

	item_definition_index_t			m_usDefIndex;						// DefIndex of the item

	// Private data so that we can check in the accessor functions that the data fits before returning it.
	item_price_t					m_unBaseCosts[k_ECurrencyMax];		// Costs of the items indexed by ECurrency -- if the items are on sale, this will be the current sale price
	item_price_t					m_unSaleCosts[k_ECurrencyMax];		// Original costs of the items indexed by ECurrency -- if the items are on sale, this will be the pre-sale price
	uint16							m_usQuantity;						// Quantity sold in a single purchase (ie., dueling pistols come in stacks of five)
	float							m_fRentalPriceScale;				// 100.0 or greater means "unavailable to rent"
	uint32							m_unGiftSteamPackageID;				// if non-zero, when this item is purchased (including inside bundles, etc.), grant a gift copy of this Steam package

	struct CategoryTag_t
	{
		CUtlString				m_strName;								// Individual tag name, like "Weapons," "New," etc.
		StoreCategoryID_t		m_unID;									// The category ID
	};
	CCopyableUtlVector< CategoryTag_t >	m_vecCategoryTags;				// Category tag data
	
	const char					*m_pchCategoryTags;						// All tags - this string will something like: "New" or "Weapons+New" etc.
};


// Spend xxx amount of money, get a free item from the loot list
struct store_promotion_spend_for_free_item_t
{
	const CEconItemDefinition	  *m_pItemDef;
	item_price_t				   m_rgusPriceThreshold[k_ECurrencyMax];	// Price threshold to get an item from the loot list indexed by ECurrency
};


//-----------------------------------------------------------------------------
// Purpose: Class that represents what's currently for sale in TF
//-----------------------------------------------------------------------------
typedef enum
{
	kEconStoreSortType_Price_HighestToLowest = 0,
	kEconStoreSortType_Price_LowestToHighest = 1,
	kEconStoreSortType_DevName_AToZ = 2,
	kEconStoreSortType_DevName_ZToA = 3,
	kEconStoreSortType_Name_AToZ = 4,
	kEconStoreSortType_Name_ZToA = 5,
	kEconStoreSortType_ItemDefIndex = 6,
	kEconStoreSortType_ReverseItemDefIndex = 7,
	kEconStoreSortType_DateNewest = 8,
	kEconStoreSortType_DateOldest = 9,
} eEconStoreSortType;

struct price_point_map_key_t
{
	item_price_t m_unPriceUSD;
	ECurrency m_eCurrency;

	static bool Less( const price_point_map_key_t& a, const price_point_map_key_t& b )
	{
		if ( a.m_eCurrency == b.m_eCurrency )
			return a.m_unPriceUSD < b.m_unPriceUSD;

		return a.m_eCurrency < b.m_eCurrency;
	}
};

typedef CUtlMap<price_point_map_key_t, item_price_t> CurrencyPricePointMap_t;

class CEconStorePriceSheet
{
public:
	typedef CUtlMap<item_definition_index_t, econ_store_entry_t> StoreEntryMap_t;
	typedef CUtlMap<const char *, float> RentalPriceScaleMap_t;
	typedef CUtlVector<item_definition_index_t> FeaturedItems_t;

	CEconStorePriceSheet();
	~CEconStorePriceSheet();

	bool InitFromKV( KeyValues *pKVPrices );

	// Gets or sets the version stamp. This is just a number the GC can use
	// to know if the client is in sync without sending it down on every
	// request.
	RTime32 GetVersionStamp( void ) const { return m_RTimeVersionStamp; }
	void SetVersionStamp( RTime32 stamp ) { m_RTimeVersionStamp = stamp; }

	uint32 GetHashForAllItems() const { return m_unHashForAllItems; }

	typedef CUtlMap<uint16, econ_store_entry_t> EconStoreEntryMap_t;
	EconStoreEntryMap_t &GetEntries() { return m_mapEntries; }
	

	const StoreEntryMap_t &GetEntries() const { return m_mapEntries; }
	const CEconStoreCategoryManager::StoreCategory_t *GetFeaturedItems( void ) { return &m_FeaturedItems; }
	const econ_store_entry_t *GetEntry( item_definition_index_t usDefIndex ) const;

	uint32 GetFeaturedItemIndex() const { return m_unFeaturedItemIndex; }
	void SetFeaturedItemIndex( uint32 unIdx ) { m_unFeaturedItemIndex = unIdx; }

	void SetEconStoreSortType( eEconStoreSortType eType ) { m_eEconStoreSortType = eType; }
	eEconStoreSortType GetEconStoreSortType() { return m_eEconStoreSortType; }

	const store_promotion_spend_for_free_item_t *GetStorePromotion_SpendForFreeItem() const { return &m_StorePromotionSpendForFreeItem; }
	const CEconItemDefinition * GetStorePromotion_FirstTimePurchaseItem() const { return m_pStorePromotionFirstTimePurchaseItem; }
	const CEconItemDefinition * GetStorePromotion_FirstTimeWebPurchaseItem() const { return m_pStorePromotionFirstTimeWebPurchaseItem; }

	uint32 GetPreviewPeriod() const { return m_unPreviewPeriod; }
	uint32 GetBonusDiscountPeriod() const { return m_unBonusDiscountPeriod; }
	float GetPreviewPeriodDiscount() const { return m_flPreviewPeriodDiscount; }

	bool BItemExistsInPriceSheet( item_definition_index_t unDefIndex ) const;

	float GetRentalPriceScale( const char *pszCategory ) const
	{
		RentalPriceScaleMap_t::IndexType_t i = m_mapRentalPriceScales.Find( pszCategory );
		if ( i == RentalPriceScaleMap_t::InvalidIndex() )
			return 100.0f;

		return m_mapRentalPriceScales[i];
	}

	KeyValues *GetRawData() const { return m_pKVRaw; }


#ifdef CLIENT_DLL
	const FeaturedItems_t& GetFeaturedItems() const { return m_vecFeaturedItems; }
#endif // CLIENT_DLL

private:
	bool BInitEntryFromKV( KeyValues *pKVEntry );
#ifdef CLIENT_DLL
	bool BInitMarketEntryFromKV( KeyValues *pKVEntry );
#endif // CLIENT_DLL


private:
	void Clear();
	uint32 CalculateHashFromItems() const;
	
	KeyValues	*m_pKVRaw;
	RTime32		m_RTimeVersionStamp;
	CEconStoreCategoryManager::StoreCategory_t	m_FeaturedItems;		// Special section, not a tab, kept outside m_vecContents
	StoreEntryMap_t m_mapEntries;
	RentalPriceScaleMap_t m_mapRentalPriceScales;
	store_promotion_spend_for_free_item_t m_StorePromotionSpendForFreeItem;
	const CEconItemDefinition* m_pStorePromotionFirstTimePurchaseItem;
	const CEconItemDefinition* m_pStorePromotionFirstTimeWebPurchaseItem;

#ifdef CLIENT_DLL
	FeaturedItems_t m_vecFeaturedItems;
#endif // CLIENT_DLL


	// changes based on experiments
	uint32	m_unFeaturedItemIndex;
	eEconStoreSortType m_eEconStoreSortType;

	uint32 m_unPreviewPeriod;
	uint32 m_unBonusDiscountPeriod;
	float m_flPreviewPeriodDiscount;
	uint32 m_unHashForAllItems;

	// price point lookup
	CurrencyPricePointMap_t m_mapCurrencyPricePoints;
};

#ifdef CLIENT_DLL
void MakeMoneyString( wchar_t *pchDest, uint32 nDest, item_price_t unPrice, ECurrency eCurrencyCode );

bool ShouldUseNewStore();
int GetStoreVersion();
#endif // CLIENT_DLL

const CEconStorePriceSheet *GetEconPriceSheet();

#endif // ECON_STORE_H
