//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for generating store meta data. Abstract methods need
// to be overridden on a per-product basis.
//
//-------------------------------------------------------------------------------------------------------------------------------

#include "cbase.h"
#include "econ_storecategory.h"
#include "econ_store.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-------------------------------------------------------------------------------------------------------------------------------

/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Invalid	= (StoreCategoryID_t)0;
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_New		= CEconStoreCategoryManager::GetCategoryID( "New" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Weapons	= CEconStoreCategoryManager::GetCategoryID( "Weapons" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Limited	= CEconStoreCategoryManager::GetCategoryID( "Limited" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Maps		= CEconStoreCategoryManager::GetCategoryID( "Maps" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Cosmetics	= CEconStoreCategoryManager::GetCategoryID( "Cosmetics" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Taunts		= CEconStoreCategoryManager::GetCategoryID( "Taunts" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Tools		= CEconStoreCategoryManager::GetCategoryID( "Tools" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Bundles	= CEconStoreCategoryManager::GetCategoryID( "Bundles" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Collections= CEconStoreCategoryManager::GetCategoryID( "Collections" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Popular	= CEconStoreCategoryManager::GetCategoryID( "Popular" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_OnSale		= CEconStoreCategoryManager::GetCategoryID( "OnSale" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Featured	= CEconStoreCategoryManager::GetCategoryID( "Featured" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_ClassBundles	= CEconStoreCategoryManager::GetCategoryID( "Class_Bundles" );
/*static*/ StoreCategoryID_t CEconStoreCategoryManager::k_CategoryID_Highlighted = CEconStoreCategoryManager::GetCategoryID( "Highlighted" );


//-------------------------------------------------------------------------------------------------------------------------------

CEconStoreCategoryManager::CEconStoreCategoryManager()
{
	m_unHomeCategoryID = k_CategoryID_Invalid;
}

bool CEconStoreCategoryManager::BInit( CEconStorePriceSheet *pPriceSheet, KeyValues *pStoreMetaDataKV )
{
	KeyValues *pCategoriesKV = pStoreMetaDataKV->FindKey( "categories" );
	if ( !pCategoriesKV )
	{
		AssertMsg( 0, "Could not find 'categories' subkey!" );
		return false;
	}

	FOR_EACH_TRUE_SUBKEY( pCategoriesKV, pKVCurCategory )
	{
		const int iIndex = m_vecCategories.AddToTail();
		StoreCategory_t &curCategory = m_vecCategories[ iIndex ];
		if ( !BInitCategory( pPriceSheet, &curCategory, pKVCurCategory ) )
			return false;

		// If the current category is the home page, cache off its ID
		if ( curCategory.m_bIsHome )
		{
			m_unHomeCategoryID = curCategory.m_unID;
		}
	}

	// Verify that any parents point to valid categories
	FOR_EACH_VEC( m_vecCategories, i )
	{
		const StoreCategory_t &curCategory = m_vecCategories[i];

		// Skip current category if it refers to invalid, which is fine
		if ( curCategory.m_unParentCategoryID == k_CategoryID_Invalid )
			continue;

		// A category can't be a parent to itself
		if ( curCategory.m_unID == curCategory.m_unParentCategoryID )
		{
			AssertMsg( 0, "Store category %s is using itself as a parent category!", curCategory.m_pchName );
		}
		
		// Attempt to find the current section's parent ID
		bool bFound = false;
		FOR_EACH_VEC( m_vecCategories, j )
		{
			// Don't compare against self
			if ( i == j )
				continue;

			if ( m_vecCategories[j].m_unID == curCategory.m_unParentCategoryID )
			{
				bFound = true;
				break;
			}
		}

		// If we couldn't find the current section's parent ID, assert
		if ( !bFound )
		{
			AssertMsg( 0, "Category %s refers to an unknown parent category - check your spelling!", curCategory.m_pchName );
		}
	}

	// Setup child category lists - looping twice to keep this code clean and easy to read
	FOR_EACH_VEC( m_vecCategories, i )
	{
		const StoreCategory_t &curCategory = m_vecCategories[i];

		if ( k_CategoryID_Invalid == curCategory.m_unParentCategoryID )
			continue;

		StoreCategory_t *pParentCategory = GetStoreCategoryFromID( curCategory.m_unParentCategoryID );
		if ( !pParentCategory )
			continue;

		pParentCategory->m_vecSubcategories.AddToTail( &curCategory );
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------

bool CEconStoreCategoryManager::BInitCategory( CEconStorePriceSheet *pPriceSheet, StoreCategory_t *pCategory, KeyValues* pKVTab )
{
	const char *pDefaultResFile = "Resource/UI/econ/store/v2/StorePage.res";

	// Get the main category name
	const char* pCategoryName = pKVTab->GetName();
	if ( !pCategoryName || !pCategoryName[0] )
	{
		AssertMsg( 0, "Invalid category name!" );
		return false;
	}

	const bool bIsHome = pKVTab->GetBool( "home", false );
	pCategory->m_bIsHome = bIsHome;

	pCategory->m_pchRawName = pCategoryName;
	pCategory->m_unID = GetCategoryID( pCategoryName );	

	pCategory->m_bUseLargeCells = pKVTab->GetBool( "use_large_cells", false );
	pCategory->m_bVisible = pKVTab->GetBool( "visible", true );
	pCategory->m_bInGameOnly = pKVTab->GetBool( "ingame_only", false );
	pCategory->m_bDefaultTab = pKVTab->GetBool( "default", false );

	const char *pchLabelTokenWebOverride = NULL;
	pCategory->m_pchName = pchLabelTokenWebOverride ? pchLabelTokenWebOverride : pKVTab->GetString( "label_token", "#Store_Unknown" );

	pCategory->m_pchPageClass = pKVTab->GetString( "page_class", "CStorePage" );
	pCategory->m_pchPageRes = pKVTab->GetString( "page_res", pDefaultResFile );
	pCategory->m_pchSortType = pKVTab->GetString( "sort_type", "" );

	// Important for web store but not needed for VGUI store

	// Look for a parent category for non-home categories
	if ( !bIsHome )
	{
		const char *pParentCategoryName = pKVTab->GetString( "parent", NULL );
		if ( pParentCategoryName )
		{
			pCategory->m_unParentCategoryID = GetCategoryID( pParentCategoryName );
		}
		else
		{
			pCategory->m_unParentCategoryID = k_CategoryID_Invalid;
		}
	}

	return true;
}

bool CEconStoreCategoryManager::BOnPriceSheetLoaded( CEconStorePriceSheet *pPriceSheet )
{
	// Go through all categories/subcategories and add a list of items to each.
	// If an item belongs to a subcategory, it will also be added to its parent category. For example,
	// a hat will be added to both the "hats" subcategory and the "items" parent category.
	FOR_EACH_VEC( m_vecCategories, iCategory )
	{
		StoreCategory_t &Category = m_vecCategories[iCategory];

		// find all entries that match
		const CEconStorePriceSheet::EconStoreEntryMap_t &mapEntries = pPriceSheet->GetEntries();
		FOR_EACH_MAP_FAST( mapEntries, idx )
		{
			const econ_store_entry_t &entry = mapEntries[idx];

			if ( entry.IsListedInCategoryOrSubcategories( Category ) )
			{
				Category.m_vecEntries.InsertNoSort( entry.GetItemDefinitionIndex() );
			}		
		}
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------

/*static*/ StoreCategoryID_t CEconStoreCategoryManager::GetCategoryID( const char *pCategoryName )
{
	// Make the input lower case
	CUtlString strLowerCase = pCategoryName;
	strLowerCase.ToLower();

	return CRC32_ProcessSingleBuffer( (void*)strLowerCase.Get(), strLowerCase.Length() );
}

//-------------------------------------------------------------------------------------------------------------------------------

const CEconStoreCategoryManager::StoreCategory_t *CEconStoreCategoryManager::GetStoreCategoryFromID( StoreCategoryID_t unID ) const
{
	return const_cast< CEconStoreCategoryManager * >( this )->GetStoreCategoryFromID( unID );
}

CEconStoreCategoryManager::StoreCategory_t *CEconStoreCategoryManager::GetStoreCategoryFromID( StoreCategoryID_t unID )
{
	if ( k_CategoryID_Invalid != unID )
	{
		FOR_EACH_VEC( m_vecCategories, i )
		{
			if ( unID == m_vecCategories[i].m_unID )
				return &m_vecCategories[i];
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------------------------------------------------------------

static CEconStoreCategoryManager *gs_pEconStoreCategoryManager = NULL;
CEconStoreCategoryManager *GEconStoreCategoryManager()
{
	if ( !gs_pEconStoreCategoryManager )
	{
		gs_pEconStoreCategoryManager = new CEconStoreCategoryManager();
	}

	return gs_pEconStoreCategoryManager;
}

void ClearEconStoreCategoryManager()
{
	delete gs_pEconStoreCategoryManager;
	gs_pEconStoreCategoryManager = NULL;
}