//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_EUREKA_TELEPORT_H
#define TF_HUD_MENU_EUREKA_TELEPORT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "IconPanel.h"
#include "tf_controls.h"
#include "hudelement.h"
#include "tf_hud_menu_engy_build.h"

using namespace vgui;

class CHudEurekaEffectTeleportMenu : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudEurekaEffectTeleportMenu, EditablePanel );

public:
	CHudEurekaEffectTeleportMenu( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	SetVisible( bool bState ) OVERRIDE;
	virtual void	OnTick( void ) OVERRIDE;
	int				HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		GetRenderGroupPriority() OVERRIDE { return 51; }
	void			WantsToTeleport( void );

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

private:
	void SetSelectedItem( eEurekaTeleportTargets eSelectedTeleportTarget );
	void SendTeleportMessage( eEurekaTeleportTargets eTeleportTarget );
	bool CanTeleport() const;

	bool m_bWantsToTeleport;
	eEurekaTeleportTargets m_eSelectedTeleportTarget;
	buildmenulayouts_t		m_eCurrentBuildMenuLayout;

	EditablePanel *m_pAvilableTargets[EUREKA_NUM_TARGETS];
	EditablePanel *m_pUnavailableTargets[EUREKA_NUM_TARGETS];
	CIconPanel *m_pActiveSelection;
};

#endif	// TF_HUD_MENU_EUREKA_TELEPORT_H
