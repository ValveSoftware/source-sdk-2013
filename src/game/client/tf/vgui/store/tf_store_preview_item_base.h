//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PREVIEW_ITEM_BASE_H
#define TF_STORE_PREVIEW_ITEM_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "store/store_preview_item.h"
#include "store/v1/tf_store_page.h"
#include "tf_shareddefs.h"
#include "tf_hud_mainmenuoverride.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFStorePreviewItemPanelBase : public CStorePreviewItemPanel
{
	DECLARE_CLASS_SIMPLE( CTFStorePreviewItemPanelBase, CStorePreviewItemPanel );
protected:
	// CTFStorePreviewItemPanelBase should not be intantiated directly
	CTFStorePreviewItemPanelBase( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner );

public:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void PerformLayout( void );
	virtual void OnTick( void );

	virtual void	PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry=NULL ) OVERRIDE;
	virtual void	SetState( preview_state_t iState );

	virtual int		GetPreviewTeam() const;

	MESSAGE_FUNC_PARAMS( OnClassIconSelected, "ClassIconSelected", data );
	MESSAGE_FUNC( OnHideClassIconMouseover, "HideClassIconMouseover" );
	MESSAGE_FUNC_PARAMS( OnShowClassIconMouseover, "ShowClassIconMouseover", data );

protected:
	void			UpdateModelPanel();
	virtual void	SetPlayerModelVisible( bool bVisible );
	virtual void	UpdatePlayerModelButtons( void );
	virtual void	UpdateCustomizeMenu( void );
	void			UpdateOptionsButton( void );
	void			UpdateNextWeaponButton( void );
	void			UpdateZoomButton( void );
	void			UpdateTeamButton( void );
	virtual void	UpdateIcons( void );

	void			SetPaint( item_definition_index_t iItemDef );
	void			SetStyle( style_index_t unStyle );
	void			SetUnusual( uint32 iUnusualIndex );
	const CUtlVector< int >	*GetUnusualList() const;
	virtual bool	AllowUnusualPreview() const
	{
		return false;
	}

	void			CyclePaint( bool bActuallyCycle = true );
	void			CycleStyle( void );
	void			ResetHandles( void );

	// This can be overridden to capture *any* "cycle text" that is being set, so one generic label can be used
	// by a derived class if needed.  Base version just sets the label's text.
	virtual void	SetCycleLabelText( vgui::Label *pTargetLabel, const char *pCycleText );

	vgui::Label					*m_pClassIconMouseoverLabel;
	CTFPlayerModelPanel			*m_pPlayerModelPanel;
	CUtlVector<CStorePreviewClassIcon*>	m_pClassIcons;

	int							m_iCurrentClass;
	int							m_iCurrentHeldItem;

	item_definition_index_t		m_unPaintDef;
	uint32						m_unPaintRGB0;
	uint32						m_unPaintRGB1;

	CExButton					*m_pRotRightButton;
	CExButton					*m_pRotLeftButton;
	CExButton					*m_pNextWeaponButton;
	CExButton					*m_pZoomButton;
	CExButton					*m_pOptionsButton;
	CExButton					*m_pTeamButton;
	vgui::Label					*m_pPaintNameLabel;
	vgui::Label					*m_pStyleNameLabel;

	Menu						*m_pCustomizeMenu;
	CUtlVector< item_definition_index_t > m_vecPaintCans;
};

#endif // TF_STORE_PREVIEW_ITEM_H
