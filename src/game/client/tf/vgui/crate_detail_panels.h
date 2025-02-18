//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CRATE_DETAIL_PANELS_H
#define CRATE_DETAIL_PANELS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "item_model_panel.h"
#include "econ_item_view.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ProgressBar.h>

class CInputStringForItemBackpackOverlayDialog : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CInputStringForItemBackpackOverlayDialog, vgui::EditablePanel );

public:
	CInputStringForItemBackpackOverlayDialog( vgui::Panel *pParent, CEconItemView *pItem, CEconItemView *pChosenKey = NULL );
	~CInputStringForItemBackpackOverlayDialog();

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	void Show();

protected:
	CItemModelPanel *GetPreviewModelPanel() { return m_pPreviewModelPanel; }

	CEconItemView			m_Item;

private:
	void CreateItemPanels();
	void FindUsableKey();

	vgui::ProgressBar				*m_pProgressBar;
	CExLabel						*m_pRareLootLabel;
	CExButton						*m_pUseKeyButton;
	CExButton						*m_pGetKeyButton;
	CExButton						*m_pShuffleButton;
	CItemModelPanel					*m_pPreviewModelPanel;
	vgui::TextEntry					*m_pTextEntry;
	CUtlVector< CItemModelPanel* >	m_vecContentsPanels;
	KeyValues						*m_pItemModelPanelKVs;
	CItemModelPanelToolTip			*m_pMouseOverTooltip;
	CItemModelPanel					*m_pMouseOverItemPanel;
	static float					m_sflNextShuffleTime;
	bool							m_bUpdateRecieved;
	CEconItemView					m_UseableKey;
};

#endif // CRATE_DETAIL_PANELS_H
