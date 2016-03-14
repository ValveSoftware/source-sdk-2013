#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_mom_normal.h"
#include "panelmetaclassmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default FOV for HL2
ConVar default_fov("default_fov", "90", FCVAR_CHEAT);

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

#define SCREEN_FILE		"scripts/vgui_screens.txt"

class CMomentumModeManager : public IVModeManager
{
public:
    CMomentumModeManager(void);
    virtual		~CMomentumModeManager(void);

    virtual void	Init(void);
    virtual void	SwitchMode(bool commander, bool force);
    virtual void	OverrideView(CViewSetup *pSetup);
    virtual void	CreateMove(float flInputSampleTime, CUserCmd *cmd);
    virtual void	LevelInit(const char *newmap);
    virtual void	LevelShutdown(void);
};

CMomentumModeManager::CMomentumModeManager(void)
{
}

CMomentumModeManager::~CMomentumModeManager(void)
{
}

void CMomentumModeManager::Init(void)
{
    g_pClientMode = GetClientModeNormal();
    PanelMetaClassMgr()->LoadMetaClassDefinitionFile(SCREEN_FILE);
}

void CMomentumModeManager::SwitchMode(bool commander, bool force)
{
}

void CMomentumModeManager::OverrideView(CViewSetup *pSetup)
{
}

void CMomentumModeManager::CreateMove(float flInputSampleTime, CUserCmd *cmd)
{
}

void CMomentumModeManager::LevelInit(const char *newmap)
{
    g_pClientMode->LevelInit(newmap);
}

void CMomentumModeManager::LevelShutdown(void)
{
    g_pClientMode->LevelShutdown();
}


static CMomentumModeManager g_MOMModeManager;
IVModeManager *modemanager = &g_MOMModeManager;