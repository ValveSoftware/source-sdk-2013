//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ISERVERENGINETOOLS_H
#define ISERVERENGINETOOLS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: exposed from engine to game .dll
//-----------------------------------------------------------------------------
class IServerEngineTools : public IBaseInterface
{
public:
	// Level init, shutdown
	virtual void LevelInitPreEntityAllTools() = 0;
	// entities are created / spawned / precached here
	virtual void LevelInitPostEntityAllTools() = 0;

	virtual void LevelShutdownPreEntityAllTools() = 0;
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntityAllTools() = 0;
	// end of level shutdown

	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThinkAllTools() = 0;
	// called after entities think
	virtual void FrameUpdatePostEntityThinkAllTools() = 0;

	virtual void PreClientUpdateAllTools() = 0;
	// FIXME:  PostClientUpdateAllTools()???

	// The server uses this to call into the tools to get the actual
	// entities to spawn on startup
	virtual const char* GetEntityData( const char *pActualEntityData ) = 0;

	virtual void PreSetupVisibilityAllTools() = 0;

	virtual bool InToolMode() = 0;
};

#define VSERVERENGINETOOLS_INTERFACE_VERSION "VSERVERENGINETOOLS001"

#endif // ISERVERENGINETOOLS_H
