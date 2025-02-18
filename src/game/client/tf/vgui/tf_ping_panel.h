//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PING_PANEL_H
#define TF_PING_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_matchmaking_dashboard_side_panel.h"
#include <../common/GameUI/cvarslider.h>

using namespace vgui;

class CTFPingPanel : public CMatchMakingDashboardSidePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFPingPanel, CMatchMakingDashboardSidePanel )
public:
	CTFPingPanel( Panel* pPanel, const char *pszName, ETFMatchGroup eMatchGroup );
	~CTFPingPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

private:
	void CleanupPingPanels();
	void RegeneratePingPanels();
	void UpdateCurrentPing();

	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );
	MESSAGE_FUNC( OnSliderMoved, "SliderMoved" );

	CPanelAnimationVarAliasType( int, m_iDataCenterY, "datacenter_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iDataCenterYSpace, "datacenter_y_space", "0", "proportional_int" );

	ComboBox* m_pInviteModeComboBox = NULL;
	CvarToggleCheckButton<UIConVarRef> *m_pCustomPingCheckBox = NULL;
	CvarToggleCheckButton<UIConVarRef> *m_pIgnoreInvitesCheckBox = NULL;
	CvarToggleCheckButton<UIConVarRef> *m_pKeepTeamTogetherCheckBox = NULL;
	Label *m_pCurrentPingLabel;
	CCvarSlider *m_pPingSlider;

	struct PingPanelInfo
	{
		EditablePanel *m_pPanel;
		float m_flPopulationRatio;
		int m_nPing;
	};
	CUtlVector< PingPanelInfo > m_vecDataCenterPingPanels;

	ETFMatchGroup m_eMatchGroup;
};

#endif // TF_PING_PANEL_H
