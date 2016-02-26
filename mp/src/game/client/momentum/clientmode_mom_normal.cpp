//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================
#include "cbase.h"
#include "clientmode_mom_normal.h"
#include "momentum/mom_shareddefs.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include "ClientTimesDisplay.h"
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//extern bool g_bRollingCredits; MOM_TODO: reinstate this boolean!!

ConVar fov_desired("fov_desired", "90", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.\n", true, 90.0, true, 179.0);

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;

// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
    static ClientModeMOMNormal g_ClientModeNormal;
    return &g_ClientModeNormal;
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
    DECLARE_CLASS_SIMPLE(CHudViewport, CBaseViewport);

protected:
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);

        gHUD.InitColors(pScheme);

        SetPaintBackgroundEnabled(false);
    }

    IViewPortPanel *CreatePanelByName(const char *pzName)
    {
        IViewPortPanel *panel = BaseClass::CreatePanelByName(pzName);
        if (!panel)
        {
            if (!Q_strcmp(PANEL_TIMES, pzName))
            {
                panel = new CClientTimesDisplay(this);
            }
        }

        return panel;
    }

    virtual void CreateDefaultPanels(void)
    {
        AddNewPanel(CreatePanelByName(PANEL_TIMES), "PANEL_TIMES");

        BaseClass::CreateDefaultPanels();// MOM_TODO: do we want the other panels?
        /* don't create any panels yet*/
    };
};


//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeMOMNormal::ClientModeMOMNormal()
{
    m_pViewport = new CHudViewport();
    m_pViewport->Start(gameuifuncs, gameeventmanager);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeMOMNormal::~ClientModeMOMNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeMOMNormal::Init()
{
    BaseClass::Init();

    // Load up the combine control panel scheme
    g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx(enginevgui->GetPanel(PANEL_CLIENTDLL), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme");
    if (!g_hVGuiCombineScheme)
    {
        Warning("Couldn't load combine panel scheme!\n");
    }
}

bool ClientModeMOMNormal::ShouldDrawCrosshair(void)
{
    return true;//MOM_TODO: reinstate the g_bRollingCredits when hud_credits is copied over.
    //return (g_bRollingCredits == false);
}

int ClientModeMOMNormal::HudElementKeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{

    return BaseClass::HudElementKeyInput(down, keynum, pszCurrentBinding);
}