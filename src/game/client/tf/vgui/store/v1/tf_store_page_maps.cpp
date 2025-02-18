//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v1/tf_store_page_maps.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePage_Maps::CTFStorePage_Maps( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData ) 
:	BaseClass( parent, pPageData, "Resource/UI/econ/store/v1/StorePreviewItemPanel_Maps.res" )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePage_Maps::OnPageShow()
{
	BaseClass::OnPageShow();

	SetDetailsVisible( false );
}