//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_MVM_CRITERIA_H
#define TF_MATCHMAKING_DASHBOARD_MVM_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_matchmaking_dashboard_side_panel.h"

class CMVMCriteriaPanel;
namespace vgui
{
	class CheckButton;
}

class CTFDashboardMvMPanel : public CMatchMakingDashboardSidePanel,
							 public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFDashboardMvMPanel, CMatchMakingDashboardSidePanel);
	CTFDashboardMvMPanel( Panel *parent, const char *panelName );
	virtual ~CTFDashboardMvMPanel();

	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void SetAsActive( bool bActive ) OVERRIDE;

	MESSAGE_FUNC_PARAMS( OnSetNextEnabled, "SetNextEnabled", params );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

private:
	
	void UpdateNextButton();
	void UpdateFlowControlButtonsVisibility();

	CMVMCriteriaPanel* m_pCriteria;
	CExButton* m_pMannUpQueueButton;
	CExButton* m_pBootCampQueueButton;
	vgui::CheckButton* m_pLateJoinCheckButton;
	bool m_bLastHeardNextEnabledCommand = true;
};


#endif // TF_MATCHMAKING_DASHBOARD_MVM_CRITERIA_H
