//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#ifndef ITEM_QUICKSWITCH_H
#define ITEM_QUICKSWITCH_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ScrollableEditablePanel.h"

class CLoadoutPresetPanel;

class CItemQuickSwitchPanel : public vgui::EditablePanel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CItemQuickSwitchPanel, vgui::EditablePanel );
public:
	CItemQuickSwitchPanel( const char *pElementName );
	virtual ~CItemQuickSwitchPanel();

	void		 OpenQS( void );
	void		 CloseQS( void );
	bool		 ShouldDraw( void ) { return IsVisible(); }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void OnTick( void );

	void		 UpdateEquippedItem( void );
	bool		 CalculateClassAndSlot();
	void		 UpdateModelPanels( void );
	void		 SetButtonToItem( int iButton, CEconItemView *pItem );

	bool		 IsValid( void );

	virtual void FireGameEvent( IGameEvent *event );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	MESSAGE_FUNC( OnItemPresetLoaded, "ItemPresetLoaded" );

	MESSAGE_FUNC_PTR( OnIPMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );

private:
	int								m_iClass;		// Class of the player we're selecting an item for
	int								m_iSlot;		// Slot on the player that we're selecting an item for
	bool							m_bLoadoutHasChanged;

	vgui::EditablePanel				*m_pItemContainer;
	vgui::ScrollableEditablePanel	*m_pItemContainerScroller;
	vgui::Label						*m_pWeaponLabel;
	vgui::Label						*m_pEquipYourClassLabel;
	vgui::Label						*m_pNoItemsToEquipLabel;
	vgui::Label						*m_pEquippedLabel;

	CLoadoutPresetPanel				*m_pLoadoutPresetPanel;

	KeyValues						*m_pItemKV;
	CUtlVector<CItemModelPanel	*>	m_pItemPanels;

	CPanelAnimationVarAliasType( int, m_iItemPanelXPos, "itempanel_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelYDelta, "itempanel_ydelta", "0", "proportional_int" );
};

#endif // ITEM_QUICKSWITCH_H
