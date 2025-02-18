//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASE_LOADOUT_PANEL_H
#define BASE_LOADOUT_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "econ_controls.h"
#include "item_pickup_panel.h"
#include "GameEventListener.h"
#include "tf_item_card_panel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseLoadoutPanel : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CBaseLoadoutPanel, vgui::EditablePanel );
public:
	CBaseLoadoutPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CBaseLoadoutPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	void		 ShowPanel( int iClass, bool bBackpack, bool bReturningFromArmory = false );
	virtual void FireGameEvent( IGameEvent *event );

	virtual int	 GetNumSlotsPerPage( void ) { return 1; }
	virtual int	 GetNumColumns( void ) { return 99; }
	virtual int	 GetNumRows( void ) { return 99; }
	virtual int	 GetNumPages( void ) { return 1; }
	virtual int	 GetCurrentPage() const { return m_nCurrentPage; }
	virtual void SetCurrentPage( int nNewPage );

	virtual int	 GetNumItemPanels( void ) { Assert(0); return 0; };
	virtual void OnShowPanel( bool bVisible, bool bReturningFromArmory ) { return; }
	virtual void PostShowPanel( bool bVisible ) { return; }
	CItemModelPanel *FindBestPanelNavigationForDirection( const CItemModelPanel *pCurrentPanel, const Vector2D &vPos, const Vector2D &vDirection );
	void LinkModelPanelControllerNavigation( bool bForceRelink );

	virtual void AddNewItemPanel( int iPanelIndex );

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );

	void			HideMouseOverPanel( void );
	CItemModelPanel *GetMouseOverPanel( void ) { return m_pMouseOverItemPanel; }
	CItemModelPanelToolTip *GetMouseOverToolTipPanel( void ) { return m_pMouseOverTooltip; }

protected:
	virtual void	UpdateModelPanels( void ) { return; }
	virtual void	SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	virtual bool	IsIgnoringItemPanelEnters( void ) { return false; }
	virtual void	ApplyKVsToItemPanels( void );
	virtual void	CreateItemPanels( void );
	virtual void	OnItemSelectionChanged() {}
	bool	HandleItemSelectionKeyPressed( vgui::KeyCode code ) ;
	bool	HandleItemSelectionKeyReleased( vgui::KeyCode code ) ;

	// helpers to get selected items
	int GetFirstSelectedItemIndex( bool bIncludeEmptySlots );
	CItemModelPanel *GetFirstSelectedItemModelPanel( bool bIncludeEmptySlots );
	CEconItemView *GetFirstSelectedItem();
	bool GetAdjacentItemIndex( int nIndex, int nPage, int *pnNewIndex, int *pnNewPage, int dx, int dy );
	void SelectAdjacentItem( int dx, int dy );

protected:
	CUtlVector<CItemModelPanel*>	m_pItemModelPanels;
	vgui::Label			*m_pTitleLabel;

	KeyValues			*m_pItemModelPanelKVs;
	bool				m_bReapplyItemKVs;
	bool				m_bTooltipKeyPressed;
	int					m_nCurrentPage;

	vgui::Label			*m_pCaratLabel;
	vgui::Label			*m_pClassLabel;

	CPanelAnimationVarAliasType( int, m_iItemXPosOffcenterA, "item_xpos_offcenter_a", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemXPosOffcenterB, "item_xpos_offcenter_b", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemYPos, "item_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemYDelta, "item_ydelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonXPosOffcenter, "button_xpos_offcenter", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonYPos, "button_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonYDelta, "button_ydelta", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iItemBackpackOffcenterX, "item_backpack_offcenter_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemBackpackXDelta, "item_backpack_xdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemBackpackYDelta, "item_backpack_ydelta", "0", "proportional_int" );
	CPanelAnimationVar( bool, m_bItemsOnly, "items_only", "0" );
	CPanelAnimationVar( bool, m_bForceShowBackpackRarities, "force_show_backpack_rarities", "0" );

	CPanelAnimationVarAliasType( int, m_iDeleteButtonXPos, "button_override_delete_xpos", "0", "proportional_int" );

protected:
	CItemModelPanel		*m_pMouseOverItemPanel;
	CItemModelPanelToolTip	*m_pMouseOverTooltip;
	CItemModelPanel		*m_pItemPanelBeingMousedOver;

};

#endif // BASE_LOADOUT_PANEL_H
