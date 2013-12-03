//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef ACHIEVEMENTSANDSTATSINTERFACE_H
#define ACHIEVEMENTSANDSTATSINTERFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Panel.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/MessageDialog.h"
#include "vgui/ISurface.h"

class AchievementsAndStatsInterface
{
public:
    AchievementsAndStatsInterface() { }

    virtual void CreatePanel( vgui::Panel* pParent ) {}
    virtual void DisplayPanel() {}
    virtual void ReleasePanel() {}
	virtual int GetAchievementsPanelMinWidth( void ) const { return 0; }

protected:
    //-----------------------------------------------------------------------------
    // Purpose: Positions a dialog on screen.
    //-----------------------------------------------------------------------------
    void PositionDialog(vgui::PHandle dlg)
    {
        if (!dlg.Get())
            return;

        int x, y, ww, wt, wide, tall;
        vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
        dlg->GetSize(wide, tall);

        // Center it, keeping requested size
        dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
    }
};


#endif // ACHIEVEMENTSANDSTATSINTERFACE_H
