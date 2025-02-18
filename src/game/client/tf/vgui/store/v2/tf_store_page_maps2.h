//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PAGE_MAPS2_H
#define STORE_PAGE_MAPS2_H
#ifdef _WIN32
#pragma once
#endif

#include "store/v2/tf_store_page2.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage_Maps2 : public CTFStorePage2
{
	DECLARE_CLASS_SIMPLE( CTFStorePage_Maps2, CTFStorePage2 );
public:
	CTFStorePage_Maps2( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData );
	virtual ~CTFStorePage_Maps2() {}

	virtual const char* GetPageResFile() { return "Resource/UI/econ/store/v2/StorePage_Maps.res"; }

protected:
	virtual void OnCommand( const char *command );
	virtual void OnPageShow( void );

	void			DisplayMapStampsDialog();
};

#endif // STORE_PAGE_MAPS2_H
