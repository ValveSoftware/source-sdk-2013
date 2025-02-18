//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PANEL1_H
#define TF_STORE_PANEL1_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_panel_base.h"

class CStorePage;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePanel1 : public CTFBaseStorePanel
{
	DECLARE_CLASS_SIMPLE( CTFStorePanel1, CTFBaseStorePanel );
public:
	CTFStorePanel1( vgui::Panel *parent );

	// UI Layout
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();

	// GC Management
	virtual void	PostTransactionCompleted( void );

private:
	virtual CStorePage	*CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData );
};

#endif // TF_STORE_PANEL1_H
