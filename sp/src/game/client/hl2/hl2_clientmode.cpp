//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"
#include "panelmetaclassmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default FOV for HL2
ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

#define SCREEN_FILE		"scripts/vgui_screens.txt"

class CHLModeManager : public IVModeManager
{
public:
				CHLModeManager( void );
	virtual		~CHLModeManager( void );

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
};

CHLModeManager::CHLModeManager( void )
{
}

CHLModeManager::~CHLModeManager( void )
{
}

void CHLModeManager::Init( void )
{
	g_pClientMode = GetClientModeNormal();
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
}

void CHLModeManager::SwitchMode( bool commander, bool force )
{
}

void CHLModeManager::OverrideView( CViewSetup *pSetup )
{
}

void CHLModeManager::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
}

void CHLModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
}

void CHLModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}


static CHLModeManager g_HLModeManager;
IVModeManager *modemanager = &g_HLModeManager;

