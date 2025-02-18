//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PAGE_MAPS_H
#define STORE_PAGE_MAPS_H
#ifdef _WIN32
#pragma once
#endif

#include "store/v1/tf_store_page.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage_Maps : public CTFStorePage1
{
	DECLARE_CLASS_SIMPLE( CTFStorePage_Maps, CTFStorePage1 );
public:
	CTFStorePage_Maps( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData );
	virtual ~CTFStorePage_Maps() {}

	virtual const char* GetPageResFile() { return "Resource/UI/econ/store/v1/StorePage_Maps.res"; }

	virtual void OnPageShow( void );

protected:
};

#endif // STORE_PAGE_MAPS_H
