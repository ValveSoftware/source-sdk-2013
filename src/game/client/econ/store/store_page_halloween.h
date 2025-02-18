//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PAGE_SPECIALPROMO_H
#define STORE_PAGE_SPECIALPROMO_H
#ifdef _WIN32
#pragma once
#endif

#include "store/v1/tf_store_page.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage_SpecialPromo : public CTFStorePage1
{
	DECLARE_CLASS_SIMPLE( CTFStorePage_SpecialPromo, CTFStorePage1 );
public:
	CTFStorePage_SpecialPromo( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData );

	virtual const char* GetPageResFile() { return pszResFile; }

protected:
//	virtual void OrderItemsForDisplay( CUtlVector<const econ_store_entry_t *>& vecItems ) const;

private:
	const char* pszResFile;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage_Popular : public CTFStorePage1
{
	DECLARE_CLASS_SIMPLE( CTFStorePage_Popular, CTFStorePage1 );
public:
	CTFStorePage_Popular( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData );

protected:
	virtual void	UpdateFilteredItems( void );
};


#endif // STORE_PAGE_SPECIALPROMO_H
