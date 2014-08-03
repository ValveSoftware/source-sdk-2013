//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "igamesystem.h"
#include "toolframework/iserverenginetools.h"
#include "init_factory.h"

//-----------------------------------------------------------------------------
// Purpose: This is an autogame system which is used to call back into the engine at appropriate points
// so that IToolSystems can get these hooks at the correct time
//-----------------------------------------------------------------------------
class CToolFrameworkServer : public CAutoGameSystemPerFrame, public IToolFrameworkServer
{
public:
	virtual bool Init();
	// Level init, shutdown
	virtual void LevelInitPreEntity();
	// entities are created / spawned / precached here
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntity();
	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink();
	// called after entities think
	virtual void FrameUpdatePostEntityThink();
	virtual void PreClientUpdate();
	virtual void PreSetupVisibility();


	IServerEngineTools	*m_pTools;
};

// Singleton
static CToolFrameworkServer g_ToolFrameworkServer;
IToolFrameworkServer *g_pToolFrameworkServer = &g_ToolFrameworkServer;

#ifndef NO_TOOLFRAMEWORK

bool ToolsEnabled()
{
	return g_ToolFrameworkServer.m_pTools && g_ToolFrameworkServer.m_pTools->InToolMode() && !engine->IsDedicatedServer();
}

#endif

bool CToolFrameworkServer::Init()
{
	factorylist_t list;
	FactoryList_Retrieve( list );

	// Latch onto internal interface
	m_pTools = ( IServerEngineTools * )list.engineFactory( VSERVERENGINETOOLS_INTERFACE_VERSION, NULL );

	if ( !m_pTools && !engine->IsDedicatedServer() )
	{
		return false;
	}

	return true;
}

void CToolFrameworkServer::LevelInitPreEntity()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->LevelInitPreEntityAllTools();
}

void CToolFrameworkServer::LevelInitPostEntity()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->LevelInitPostEntityAllTools();
}

void CToolFrameworkServer::LevelShutdownPreEntity()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->LevelShutdownPreEntityAllTools();
}

void CToolFrameworkServer::LevelShutdownPostEntity()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->LevelShutdownPostEntityAllTools();
}

void CToolFrameworkServer::FrameUpdatePreEntityThink()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->FrameUpdatePreEntityThinkAllTools();
}

void CToolFrameworkServer::FrameUpdatePostEntityThink()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->FrameUpdatePostEntityThinkAllTools();
}

void CToolFrameworkServer::PreClientUpdate()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->PreClientUpdateAllTools();
}

void CToolFrameworkServer::PreSetupVisibility()
{
	if ( !m_pTools )
	{
		return;
	}
	m_pTools->PreSetupVisibilityAllTools();
}
