//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PREVIEW_ITEM1_H
#define TF_STORE_PREVIEW_ITEM1_H
#ifdef _WIN32
#pragma once
#endif

#include "store/tf_store_preview_item_base.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePreviewItemPanel1 : public CTFStorePreviewItemPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFStorePreviewItemPanel1, CTFStorePreviewItemPanelBase );
public:
	CTFStorePreviewItemPanel1( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );
	virtual void OnTick( void );

	virtual void	PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry=NULL ) OVERRIDE;
	virtual void	SetState( preview_state_t iState );

	MESSAGE_FUNC_PARAMS( OnClassIconSelected, "ClassIconSelected", data );
	MESSAGE_FUNC( OnHideClassIconMouseover, "HideClassIconMouseover" );
	MESSAGE_FUNC_PARAMS( OnShowClassIconMouseover, "ShowClassIconMouseover", data );

private:
	virtual void	UpdateIcons( void );
};

#endif // TF_STORE_PREVIEW_ITEM1_H
