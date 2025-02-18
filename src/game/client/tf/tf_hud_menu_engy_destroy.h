//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_ENGY_DESTROY_H
#define TF_HUD_MENU_ENGY_DESTROY_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "tf_hud_menu_engy_build.h"
#include "tf_hud_base_build_menu.h"

using namespace vgui;

#define ALL_BUILDINGS	-1
#define NUM_ENGY_BUILDINGS 4

enum destroymenulayouts_t
{
	DESTROYMENU_DEFAULT = 0,
	DESTROYMENU_PIPBOY,
};

class CEngyDestroyMenuItem : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CEngyDestroyMenuItem, EditablePanel );

public:

	CEngyDestroyMenuItem( Panel *parent, const char *panelName ) : EditablePanel(parent, panelName) 
	{
	}

private:
};

class CHudMenuEngyDestroy : public CHudBaseBuildMenu
{
	DECLARE_CLASS_SIMPLE( CHudMenuEngyDestroy, EditablePanel );

public:
	CHudMenuEngyDestroy( const char *pElementName );

	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );

	virtual void	SetVisible( bool state );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void	OnTick( void );

	void ErrorSound( void );

	virtual int GetRenderGroupPriority() { return 50; }

	int			CalcCustomDestroyMenuLayout( void );

private:

	void InitBuildings();

	CEngyDestroyMenuItem *m_pActiveItems[NUM_ENGY_BUILDINGS];
	CEngyDestroyMenuItem *m_pInactiveItems[NUM_ENGY_BUILDINGS];
	CEngyDestroyMenuItem *m_pUnavailableItems[NUM_ENGY_BUILDINGS];

	destroymenulayouts_t		m_iCurrentDestroyMenuLayout;

	EngyConstructBuilding_t m_Buildings[NUM_ENGY_BUILDINGS];
};

#endif	// TF_HUD_MENU_ENGY_DESTROY_H
