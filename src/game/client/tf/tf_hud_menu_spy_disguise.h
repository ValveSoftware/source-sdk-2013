//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_SPY_DISGUISE_H
#define TF_HUD_MENU_SPY_DISGUISE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <game_controls/IconPanel.h>
#include <vgui/tf_controls.h>
#include "tf_hud_base_build_menu.h"

using namespace vgui;

#define ALL_BUILDINGS	-1

class CHudMenuSpyDisguise : public CHudBaseBuildMenu
{
	DECLARE_CLASS_SIMPLE( CHudMenuSpyDisguise, EditablePanel );

public:
	CHudMenuSpyDisguise( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void ) OVERRIDE;

	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	SetVisible( bool state );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority( void ) { return 50; }
	void SelectDisguise( int iClass, int iTeam );

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

private:
	void SetSelectedItem( int iSlot );
	void ToggleDisguiseTeam( void );
	void ToggleSelectionIcons( bool bGroup );
	void FindToggleBinding( void );

private:
	EditablePanel *m_pClassItems_Red[9];
	CIconPanel *m_pKeyIcons_Red[9];
	CExLabel *m_pKeyLabels_Red[9];
	CExLabel *m_pKeyLabelsNew_Red[9];

	EditablePanel *m_pClassItems_Blue[9];
	CIconPanel *m_pKeyIcons_Blue[9];
	CExLabel *m_pKeyLabels_Blue[9];
	CExLabel *m_pKeyLabelsNew_Blue[9];

	CIconPanel *m_pKeyIcons_Category[3];
	CExLabel *m_pKeyLabels_Category[3];

	EditablePanel *m_pActiveSelection;

	int m_iShowingTeam;
	
	int m_iSelectedItem;

	int m_iGroupSelection;

	bool m_bInConsoleMode;
};

#endif	// TF_HUD_MENU_SPY_DISGUISE_H
