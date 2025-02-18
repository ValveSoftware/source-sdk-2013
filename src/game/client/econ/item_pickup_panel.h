//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_PICKUP_PANEL_H
#define ITEM_PICKUP_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include "GameEventListener.h"
#include "basemodel_panel.h"
#include "basemodelpanel.h"
#include "econ_item_inventory.h"
#include "econ_item_view.h"
#include "item_model_panel.h"
#include "vgui_controls/TextEntry.h"
#include "econ_controls.h"

class CEconItemView;
class CBackpackPanel;

#define ITEMPICKUP_NUM_MODELPANELS		5	// 1 in the center of the screen, 2 to each side to handle smooth sliding.
#define ITEMPICKUP_MODELPANEL_CENTER	2	// Panel that's in the center of the queue

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CItemPickupPanel : public vgui::Frame, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CItemPickupPanel, vgui::Frame );
public:
	CItemPickupPanel( Panel *parent, bool bPopup = true );
	virtual ~CItemPickupPanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	virtual void	ShowPanel( bool bShow );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	OnKeyCodeTyped( vgui::KeyCode code );
	virtual void	OnKeyCodePressed( vgui::KeyCode code );

	void			AddItem( CEconItemView *pItem );
	void			SetReturnToGame( bool bReturn ) { m_bReturnToGame = bReturn; }

#ifdef DEBUG
	void			DebugRandomizePickupMethods( void ) { m_bRandomizePickupMethods = true; }
#endif

	MESSAGE_FUNC_PARAMS( OnConfirmDelete, "ConfirmDlgResult", data );

protected:
	virtual void	UpdateModelPanels( void );
	virtual void	AcknowledgeItems ( void );

protected:
	int								m_iSelectedItem;

	struct founditem_t
	{
		CEconItemView				pItem;
		bool						bDiscarded;
	};
	CUtlVector<founditem_t>			m_aItems;
	CItemModelPanel					*m_aModelPanels[ITEMPICKUP_NUM_MODELPANELS];
	vgui::Button					*m_pNextButton;
	vgui::Button					*m_pPrevButton;
	vgui::Button					*m_pOpenLoadoutButton;
	CExButton						*m_pCloseButton;
	CExImageButton					*m_pDiscardButton;
	vgui::Label						*m_pItemsFoundLabel;
	vgui::Label						*m_pItemFoundMethod;
	KeyValues						*m_pModelPanelsKV;
	vgui::EditablePanel				*m_pConfirmDeleteDialog;
	vgui::Label						*m_pDiscardedLabel;
	bool							m_bRandomizePickupMethods;
	CSimplePanelToolTip				*m_pToolTip;
	bool							m_bReturnToGame;

	CPanelAnimationVarAliasType( float, m_iModelPanelSpacing, "modelpanels_spacing", "40", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iModelPanelW, "modelpanels_width", "128", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iModelPanelH, "modelpanels_height", "80", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_iModelPanelY, "modelpanels_ypos", "120", "proportional_float" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CItemDiscardPanel : public vgui::Frame, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CItemDiscardPanel, vgui::Frame );
public:
	CItemDiscardPanel( Panel *parent, bool bPopup = true );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	virtual void	ShowPanel( bool bShow );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	OnKeyCodeTyped( vgui::KeyCode code );
	virtual void	OnKeyCodePressed( vgui::KeyCode code );

	void			SetItem( CEconItemView *pItem );

	MESSAGE_FUNC_PARAMS( OnConfirmDelete, "ConfirmDlgResult", data );

protected:
	CItemModelPanel		*m_pModelPanel;
	CBackpackPanel		*m_pBackpackPanel;
	vgui::EditablePanel *m_pConfirmDeleteDialog;
	vgui::Label			*m_pDiscardedLabel;
	vgui::Label			*m_pDiscardedLabelCarat;
	bool				m_bDiscardedNewItem;
	bool				m_bMadeRoom;
	CExButton			*m_pDiscardButton;
	CExButton			*m_pCloseButton;
	CSimplePanelToolTip	*m_pItemToolTip;
	CItemModelPanel		*m_pItemMouseOverPanel;
};

#endif // ITEM_PICKUP_PANEL_H
