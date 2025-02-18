//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_TEAMSTATUS_H
#define TF_TEAMSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_playerpanel.h"

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFTeamStatusPlayerPanel : public CTFPlayerPanel
{
	DECLARE_CLASS_SIMPLE( CTFTeamStatusPlayerPanel, CTFPlayerPanel );

public:

	CTFTeamStatusPlayerPanel( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void UpdateBorder( void ) OVERRIDE;
	virtual bool Update( void ) OVERRIDE;
	int GetPreviousTeam( void ) { return m_iTeam; }

private:

	vgui::ContinuousProgressBar *m_pHealthBar;
	vgui::ContinuousProgressBar *m_pOverhealBar;
	vgui::Panel *m_pClassImageBG;
	vgui::ImagePanel *m_pDeathFlag;
	int m_iTeam;

	CPanelAnimationVar( Color, m_ColorPortraitBGRedLocalPlayer, "color_portrait_bg_red_local_player", "195 169 168 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBGBlueLocalPlayer, "color_portrait_bg_blue_local_player", "168 169 195 255" );

	CPanelAnimationVar( Color, m_ColorPortraitBGRed, "color_portrait_bg_red", "119 62 61 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBGBlue, "color_portrait_bg_blue", "62 81 101 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBGRedDead, "color_portrait_bg_red_dead", "49 44 42 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBGBlueDead, "color_portrait_bg_blue_dead", "44 49 51 255" );
	CPanelAnimationVar( Color, m_ColorBarHealthHigh, "color_bar_health_high", "84 191 58 255" );
	CPanelAnimationVar( float, m_flPercentageHealthMed, "percentage_health_med", "0.6" );
	CPanelAnimationVar( Color, m_ColorBarHealthMed, "color_bar_health_med", "191 183 58 255" );
	CPanelAnimationVar( float, m_flPercentageHealthLow, "percentage_health_low", "0.3" );
	CPanelAnimationVar( Color, m_ColorBarHealthLow, "color_bar_health_low", "191 58 58 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBlendDeadRed, "color_portrait_blend_dead_red", "255 255 255 255" );
	CPanelAnimationVar( Color, m_ColorPortraitBlendDeadBlue, "color_portrait_blend_dead_blue", "255 255 255 255" );
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFTeamStatus : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFTeamStatus, vgui::EditablePanel );

public:

	CTFTeamStatus( Panel *parent, const char *panelName );
	~CTFTeamStatus();

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	
	void Reset();
	bool ShouldDraw( void );

	enum EGrowDir
	{
		GROW_EAST = 0,
		GROW_WEST
	};

protected:

	CTFTeamStatusPlayerPanel *GetOrAddPanel( int iPanelIndex );
	void RecalculatePlayerPanels( void );
	void UpdatePlayerPanels( void );

protected:

	CUtlVector<CTFTeamStatusPlayerPanel*> m_PlayerPanels;
	KeyValues *m_pPlayerPanelKVs;
	bool m_bReapplyPlayerPanelKVs;

	CPanelAnimationVarAliasType( int, m_iMaxSize, "max_size", "19", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_i6v6Gap, "6v6_gap", "19", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_i12v12Gap, "12v12_gap", "9", "proportional_int" );

	EGrowDir m_eTeam1GrowDir;
	CPanelAnimationVarAliasType( int, m_iTeam1BaseX, "team1_base_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iTeam1MaxExpand, "team1_max_expand", "0", "proportional_int" );

	EGrowDir m_eTeam2GrowDir;
	CPanelAnimationVarAliasType( int, m_iTeam2BaseX, "team2_base_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iTeam2MaxExpand, "team2_max_expand", "0", "proportional_int" );
};

#endif	// TF_TEAMSTATUS_H
