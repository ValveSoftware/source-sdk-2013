//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//-------------------------------------------------------------------------------------------------------------------------------

#ifndef ECON_STORECATEGORY_H
#define ECON_STORECATEGORY_H
#ifdef _WIN32
#pragma once
#endif


//-------------------------------------------------------------------------------------------------------------------------------

typedef CRC32_t StoreCategoryID_t;

//-------------------------------------------------------------------------------------------------------------------------------

// sort by price highest to lowest
class CEconStoreEntryLess
{
public:
	bool Less( const uint16& lhs, const uint16& rhs, void *pContext );
};

//-------------------------------------------------------------------------------------------------------------------------------

class CEconStoreCategoryManager
{
public:
	CEconStoreCategoryManager();

	bool BInit( CEconStorePriceSheet *pPriceSheet, KeyValues *pStoreMetaDataKV );
	bool BOnPriceSheetLoaded( CEconStorePriceSheet *pPriceSheet );

	static StoreCategoryID_t GetCategoryID( const char *pCategoryName );

	static StoreCategoryID_t k_CategoryID_Invalid;
	static StoreCategoryID_t k_CategoryID_New;
	static StoreCategoryID_t k_CategoryID_Weapons;
	static StoreCategoryID_t k_CategoryID_Limited;
	static StoreCategoryID_t k_CategoryID_Maps;
	static StoreCategoryID_t k_CategoryID_Cosmetics;
	static StoreCategoryID_t k_CategoryID_Taunts;
	static StoreCategoryID_t k_CategoryID_Tools;
	static StoreCategoryID_t k_CategoryID_Bundles;
	static StoreCategoryID_t k_CategoryID_Collections;
	static StoreCategoryID_t k_CategoryID_Popular;
	static StoreCategoryID_t k_CategoryID_OnSale;
	static StoreCategoryID_t k_CategoryID_Featured;
	static StoreCategoryID_t k_CategoryID_ClassBundles;
	static StoreCategoryID_t k_CategoryID_Highlighted;

	struct StoreCategory_t
	{
		StoreCategory_t() { V_memset( this, 0, sizeof( StoreCategory_t ) ); }

		int GetNumSubcategories() const { return m_vecSubcategories.Count(); }
		bool HasSubcategories() const { return m_vecSubcategories.Count() > 1; }
		bool BIsSubcategory() const { return m_unParentCategoryID != CEconStoreCategoryManager::k_CategoryID_Invalid; }

		bool		m_bIsHome:1;				// Home page? Default=no.
		bool		m_bUseLargeCells:1;			// Display large icons in the store. Default=no.
		bool		m_bDefaultTab:1;			// Is this the default tab? Default=no
		bool		m_bVisible:1;				// Should this tab be displayed in the store? Default=yes.
		bool		m_bInGameOnly:1;			// Is this category only to be displayed in the in-game store (vs. the web store)?
		const char *m_pchRawName;				// Raw name of the tab
		const char *m_pchName;					// Name of the tab
		const char *m_pchPageClass;				// Code class of the store page.
		const char *m_pchSortType;				// How to sort the page.
		const char *m_pchPageRes;				// Res file to use for the page.
		StoreCategoryID_t 	m_unID;						// A unique ID that is stable across sessions
		StoreCategoryID_t	m_unParentCategoryID;
		CUtlVector<const StoreCategory_t *>	m_vecSubcategories;	// A list of ID's for all subcategories
		CUtlSortVector<uint16, CEconStoreEntryLess>	m_vecEntries;		// Vector of items for sale
	};

	const StoreCategoryID_t GetHomeCategoryID() const { Assert( m_unHomeCategoryID != k_CategoryID_Invalid ); return m_unHomeCategoryID; }

	const StoreCategory_t *GetStoreCategoryFromID( StoreCategoryID_t unID ) const;
	int	GetNumCategories( void ) const { return m_vecCategories.Count(); }
	const StoreCategory_t *GetCategoryFromIndex( int i ) const { Assert(i >= 0 && i < m_vecCategories.Count()); return &m_vecCategories[i]; }

	StoreCategory_t *GetFeaturedItems() const { return NULL; }

private:
	bool BInitCategory( CEconStorePriceSheet *pPriceSheet, StoreCategory_t *pCategory, KeyValues *pKVTab );

	StoreCategory_t *GetStoreCategoryFromID( StoreCategoryID_t unID );

	CUtlVector< StoreCategory_t > m_vecCategories;

	StoreCategoryID_t	m_unHomeCategoryID;	// The ID for the home tab
};

//-------------------------------------------------------------------------------------------------------------------------------

CEconStoreCategoryManager *GEconStoreCategoryManager();
void ClearEconStoreCategoryManager();

//-------------------------------------------------------------------------------------------------------------------------------

#endif
