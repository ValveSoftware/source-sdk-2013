//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_TAUNT_SELECTION_H
#define TF_HUD_MENU_TAUNT_SELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <game_controls/IconPanel.h>
#include <vgui/tf_controls.h>
#include "base_loadout_panel.h"

using namespace vgui;

#define NUM_TAUNT_SLOTS 8

class CHudMenuTauntSelection : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuTauntSelection, EditablePanel );

public:
	CHudMenuTauntSelection( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	SetVisible( bool state );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority( void ) { return 90; }
	void SelectTaunt( int iTaunt );

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

private:
	void SetSelectedItem( int iSlot );
	void FindTauntKeyBinding( void );
	void UpdateItemModelPanels();

private:
	CItemModelPanel *m_pItemModelPanels[NUM_TAUNT_SLOTS];
	// CIconPanel *m_pKeyIcons[NUM_TAUNT_SLOTS];
	// CExLabel *m_pKeyLabels[NUM_TAUNT_SLOTS];
	
	int m_iSelectedItem;
};

#endif	// TF_HUD_MENU_TAUNT_SELECTION_H
