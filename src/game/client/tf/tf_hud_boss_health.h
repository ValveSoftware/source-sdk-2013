//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef HUD_BOSS_HEALTH_METER_H
#define HUD_BOSS_HEALTH_METER_H

#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>


class CHudBossHealthMeter : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBossHealthMeter, vgui::EditablePanel );

public:
	CHudBossHealthMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	Init( void );

	virtual void	OnTick( void );

	void Update( void );		// update HUD due to data changes

private:
	vgui::ContinuousProgressBar *m_pStunMeter;

	vgui::EditablePanel *m_pHealthBarPanel;

	vgui::ImagePanel *m_pBarImagePanel;
	Color m_bossActiveBarColor;

	vgui::ImagePanel *m_pBorderImagePanel;
	Color m_bossActiveBorderColor;

	Color m_inactiveColor;

	CPanelAnimationVarAliasType( int, m_nHealthAlivePosY, "health_alive_pos_y", "42", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nHealthDeadPosY, "health_dead_pos_y", "90", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nHealthBarWide, "health_bar_wide", "168", "proportional_xpos" );
};

#endif // HUD_BOSS_HEALTH_METER_H
