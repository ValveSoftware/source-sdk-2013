//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STRANGE_COUNT_TRANSFER_H
#define STRANGE_COUNT_TRANSFER_H
#ifdef _WIN32
#pragma once
#endif

#include "backpack_panel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_gcmessages.h"
#include "econ_gcmessages.h"
#include "tf_imagepanel.h"
#include "tf_controls.h"
#include "item_selection_panel.h"
#include "confirm_dialog.h"

//-----------------------------------------------------------------------------
// A panel to let users choose 2 weapons to tranfer strange counts with
//-----------------------------------------------------------------------------
class CStrangeCountTransferPanel : public vgui::EditablePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CStrangeCountTransferPanel, vgui::EditablePanel );
	CStrangeCountTransferPanel( vgui::Panel *parent, CEconItemView* pToolItem );
	~CStrangeCountTransferPanel( void );

	virtual const char *GetResFile( void ) { return "Resource/UI/econ/StrangeCountTransferDialog.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );
	MESSAGE_FUNC_PARAMS( OnSelectionReturned, "SelectionReturned", data );

private:

	void UpdateOKButton();

	CTFTextToolTip					*m_pToolTip;
	vgui::EditablePanel				*m_pToolTipEmbeddedPanel;

	DHANDLE<CItemCriteriaSelectionPanel> m_hSelectionPanel;

	CExButton				*m_pOKButton;
	CEconItemView			*m_pToolItem;
	CItemModelPanel			*m_pSelectingItemModelPanel;
	CItemModelPanel			*m_pSourceStrangeModelPanel;
	CItemModelPanel			*m_pTargetStrangeModelPanel;
	CItemModelPanel			*m_pMouseOverItemPanel;
	CItemModelPanelToolTip	*m_pMouseOverTooltip;
};

#endif // STRANGE_COUNT_TRANSFER
