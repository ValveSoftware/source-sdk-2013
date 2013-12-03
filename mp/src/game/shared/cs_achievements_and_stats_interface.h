//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef CSACHIEVEMENTSANDSTATSINTERFACE_H
#define CSACHIEVEMENTSANDSTATSINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "achievements_and_stats_interface.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/MessageDialog.h"

#include "cs_gamestats_shared.h"
#include "../client/cstrike/VGUI/achievement_stats_summary.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"


#if defined(CSTRIKE_DLL) && defined(CLIENT_DLL)

class CSAchievementsAndStatsInterface : public AchievementsAndStatsInterface
{
public:
    CSAchievementsAndStatsInterface();

    virtual void CreatePanel( vgui::Panel* pParent );
    virtual void DisplayPanel();
    virtual void ReleasePanel();
	virtual int GetAchievementsPanelMinWidth( void ) const { return cAchievementsDialogMinWidth; }

protected:
    vgui::DHANDLE<vgui::Frame>  m_pAchievementAndStatsSummary;
};

#endif

#endif // CSACHIEVEMENTSANDSTATSINTERFACE_H
