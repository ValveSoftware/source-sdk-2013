//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ITOOLSYSTEM_H
#define ITOOLSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "itoolentity.h"
#include "interface.h"
#include "materialsystem/imaterialproxy.h"
#include "inputsystem/iinputsystem.h"

class KeyValues;
struct SpatializationInfo_t;
struct AudioState_t;

//-----------------------------------------------------------------------------
// Purpose: All tools expose this interface, which includes both client and server
//  related hooks
//-----------------------------------------------------------------------------
class IToolSystem
{
public:
	// Name describing the tool
	virtual char const *GetToolName() = 0;

	// Called at the end of engine startup (after client .dll and server .dll have been loaded)
	virtual bool	Init() = 0;

	// Called during RemoveTool or when engine is shutting down
	virtual void	Shutdown() = 0;

	// Called after server.dll is loaded
	virtual bool	ServerInit( CreateInterfaceFn serverFactory ) = 0; 
	// Called after client.dll is loaded
	virtual bool	ClientInit( CreateInterfaceFn clientFactory ) = 0; 

	virtual void	ServerShutdown() = 0;
	virtual void	ClientShutdown() = 0;

	// Allow tool to override quitting, called before Shutdown(), return no to abort quitting
	virtual bool	CanQuit() = 0; 

	// Called when another system wiches to post a message to the tool and/or a specific entity
	// FIXME:  Are KeyValues too inefficient here?
    virtual void	PostMessage( HTOOLHANDLE hEntity, KeyValues *message ) = 0;

	// Called oncer per frame even when no level is loaded... (call ProcessMessages())
	virtual void	Think( bool finalTick ) = 0;

// Server calls:

	// Level init, shutdown
	virtual void	ServerLevelInitPreEntity() = 0;
	// entities are created / spawned / precached here
	virtual void	ServerLevelInitPostEntity() = 0;

	virtual void	ServerLevelShutdownPreEntity() = 0;
	// Entities are deleted / released here...
	virtual void	ServerLevelShutdownPostEntity() = 0;
	// end of level shutdown

	// Called each frame before entities think
	virtual void	ServerFrameUpdatePreEntityThink() = 0;
	// called after entities think
	virtual void	ServerFrameUpdatePostEntityThink() = 0;
	virtual void	ServerPreClientUpdate() = 0;
	virtual void	ServerPreSetupVisibility() = 0;

	// Used to allow the tool to spawn different entities when it's active
	virtual const char* GetEntityData( const char *pActualEntityData ) = 0;

// Client calls:
	// Level init, shutdown
	virtual void	ClientLevelInitPreEntity() = 0;
	// entities are created / spawned / precached here
	virtual void	ClientLevelInitPostEntity() = 0;

	virtual void	ClientLevelShutdownPreEntity() = 0;
	// Entities are deleted / released here...
	virtual void	ClientLevelShutdownPostEntity() = 0;
	// end of level shutdown
	// Called before rendering
	virtual void	ClientPreRender() = 0;
	virtual void	ClientPostRender() = 0;

	// Let tool override viewport for engine
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height ) = 0;

	// let tool override view/camera
	virtual bool	SetupEngineView( Vector &origin, QAngle &angles, float &fov ) = 0;

	// let tool override microphone
	virtual bool	SetupAudioState( AudioState_t &audioState ) = 0;

	// Should the client be allowed to render the view normally?
	virtual bool	ShouldGameRenderView() = 0;
	virtual bool	IsThirdPersonCamera() = 0;

	// is the current tool recording?
	virtual bool	IsToolRecording() = 0;

	virtual IMaterialProxy *LookupProxy( const char *proxyName ) = 0;

	// Possible hooks for rendering
	// virtual void	Think( float curtime, float frametime ) = 0;
	// virtual void Prerender() = 0;
	// virtual void Render3D() = 0;
	// virtual void Render2D() = 0;
// Tool activation/deactivation

	// This tool is being activated
	virtual void	OnToolActivate() = 0;
	// Another tool is being activated
	virtual void	OnToolDeactivate() = 0;

	virtual bool	TrapKey( ButtonCode_t key, bool down ) = 0;

	virtual bool	GetSoundSpatialization( int iUserData, int guid, SpatializationInfo_t& info ) = 0;

	// Unlike the client .dll pre/post render stuff, these get called no matter whether a map is loaded and they only get called once per frame!!!
	virtual void		RenderFrameBegin() = 0;
	virtual void		RenderFrameEnd() = 0;

	// wraps the entire frame - surrounding all other begin/end and pre/post calls
	virtual void		HostRunFrameBegin() = 0;
	virtual void		HostRunFrameEnd() = 0;

	// See enginevgui.h for paintmode_t enum definitions
	virtual void		VGui_PreRender( int paintMode ) = 0;
	virtual void		VGui_PostRender( int paintMode ) = 0;

	virtual void		VGui_PreSimulate() = 0;
	virtual void		VGui_PostSimulate() = 0;
};

// Pointer to a member method of IGameSystem
typedef void (IToolSystem::*ToolSystemFunc_t)();
typedef void (IToolSystem::*ToolSystemFunc_Int_t)( int arg );

#endif // ITOOLSYSTEM_H
