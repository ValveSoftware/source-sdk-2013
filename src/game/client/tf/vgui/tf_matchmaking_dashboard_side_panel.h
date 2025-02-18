//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_SIDE_PANEL_H
#define TF_MATCHMAKING_DASHBOARD_SIDE_PANEL_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"
#include "tf_gc_client.h"

enum EStackSide_t
{
	k_eSideLeft = 0,
	k_eSideRight
};

class CMatchMakingDashboardSidePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMatchMakingDashboardSidePanel, vgui::EditablePanel );
public:
	CMatchMakingDashboardSidePanel( Panel *parent, const char *panelName, const char* pszResFile, EStackSide_t eSide );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void SetAsActive( bool bActive );
	EStackSide_t GetSide() const { return m_eSide; }

	MESSAGE_FUNC( OnUpdateVisiblity, "UpdateVisiblity" );

protected:

	void SetupButtonAndTooltip( vgui::Button* pButton, EditablePanel* pTooltipPanel, wchar_t* pwszTipText );
	void SetupQueueButton( ETFMatchGroup eGroup );

private:

	CUtlString m_strResFile;
	class CExButton* m_pReturnButton;
	vgui::Panel* m_pShade;
	vgui::ImagePanel* m_pInnerGradient;
	EStackSide_t m_eSide;
};

#endif // TF_MATCHMAKING_DASHBOARD_SIDE_PANEL_H
