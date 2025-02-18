//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PANEL2_H
#define TF_STORE_PANEL2_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_panel_base.h"

class CStorePage;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePanel2 : public CTFBaseStorePanel
{
	DECLARE_CLASS_SIMPLE( CTFStorePanel2, CTFBaseStorePanel );
public:
	CTFStorePanel2( vgui::Panel *parent );

	// UI Layout
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void ShowPanel( bool bShow );
	virtual void OnAddToCart( void );

	// GC Management
	virtual void	PostTransactionCompleted( void );

private:
	virtual CStorePage	*CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData );
};

#endif // TF_STORE_PANEL2_H
