//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PAGE1_H
#define TF_STORE_PAGE1_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_page_base.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePage1 : public CTFStorePageBase
{
	DECLARE_CLASS_SIMPLE( CTFStorePage1, CTFStorePageBase );
public:
	CTFStorePage1( Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile = NULL );

	virtual const char *GetPageResFile( void );
	virtual void	OnCommand( const char *command );
	virtual void	ShowPreview( int iClass, const econ_store_entry_t* pEntry );

	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC_PTR( OnItemDetails, "ItemDetails", panel );

	virtual void	UpdateFilterComboBox( void );
	virtual void	GetFiltersForDef( GameItemDefinition_t *pDef, CUtlVector<int> *pVecFilters );
	virtual void	OnTick( void );

	virtual CStorePreviewItemPanel	*CreatePreviewPanel( void );
};

#endif // TF_STORE_PAGE1_H
