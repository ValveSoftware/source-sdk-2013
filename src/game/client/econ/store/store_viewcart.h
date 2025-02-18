//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_VIEWCART_H
#define STORE_VIEWCART_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "GameEventListener.h"
#include "store/store_panel.h"

//-----------------------------------------------------------------------------
// Purpose: Shows a single item in the cart
//-----------------------------------------------------------------------------
class CCartViewItemEntry : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCartViewItemEntry, vgui::EditablePanel );
public:
	CCartViewItemEntry( vgui::Panel *parent, const char *name ) : vgui::EditablePanel(parent,name)
	{
		m_pEntry = NULL;
	}

	void	SetEntry( cart_item_t *pEntry, int iEntryIndex );
	cart_item_t *GetEntry( void ) { return m_pEntry; }

private:
	cart_item_t *m_pEntry;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStoreViewCartPanel : public vgui::Frame, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CStoreViewCartPanel, vgui::Frame );
public:
	CStoreViewCartPanel( Panel *parent );
	virtual ~CStoreViewCartPanel();

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void ShowPanel( bool bShow );
	virtual void FireGameEvent( IGameEvent *event );

	void	UpdateCartItemList( void );

private:
	vgui::EditablePanel		*m_pClientArea;
	vgui::EditablePanel		*m_pPurchaseFooter;
	KeyValues				*m_pItemEntryKVs;
	bool					m_bReapplyItemKVs;
	vgui::Label				*m_pEmptyCartLabel;
	vgui::ImagePanel		*m_pFeaturedItemImage;

	vgui::EditablePanel				*m_pItemListContainer;
	vgui::ScrollableEditablePanel	*m_pItemListContainerScroller;
	CUtlVector<CCartViewItemEntry*> m_pItemEntries;

	CPanelAnimationVar( int, m_iSheetInsetBottom, "sheetinset_bottom", "32" );
};

CStoreViewCartPanel *OpenStoreViewCartPanel( void );
CStoreViewCartPanel *GetStoreViewCartPanel( void );

#endif // STORE_VIEWCART_H
