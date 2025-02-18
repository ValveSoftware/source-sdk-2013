//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYSYSTEM_H
#define IREPLAYSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "appframework/IAppSystem.h"

//----------------------------------------------------------------------------------------

class IClientReplayContext;
class IServerReplayContext;
class IGameEvent;

//----------------------------------------------------------------------------------------

abstract_class IReplaySystem : public IAppSystem
{
public:
	// IAppSystem:
	virtual bool		Connect( CreateInterfaceFn fnFactory ) = 0;
	virtual void		Disconnect() = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void		Shutdown() = 0;

	// To be called client- & server-side
	virtual void		Think() = 0;
	virtual bool		IsReplayEnabled() = 0;
	virtual bool		IsRecording() = 0;

	// To be called client-side only - on dedicated servers, only subs defined
	virtual bool		CL_Init( CreateInterfaceFn fnClientFactory ) = 0; 
	virtual void		CL_Shutdown() = 0;
	virtual void		CL_Render() = 0;
	virtual IClientReplayContext	*CL_GetContext() = 0;

	// To be called server-side only
	virtual bool		SV_Init( CreateInterfaceFn fnFactory ) = 0;
	virtual void		SV_Shutdown() = 0;
	virtual void		SV_EndRecordingSession( bool bForceSynchronousPublish = false ) = 0;
	virtual void		SV_SendReplayEvent( const char *pEventName, int nClientSlot ) = 0;
	virtual void		SV_SendReplayEvent( IGameEvent *pEvent, int nClientSlot ) = 0;
	virtual bool		SV_ShouldBeginRecording( bool bIsInWaitingForPlayers ) = 0;
	virtual void		SV_NotifyReplayRequested() = 0;
	virtual IServerReplayContext	*SV_GetContext() = 0;
};

//----------------------------------------------------------------------------------------

#define REPLAY_INTERFACE_VERSION "ReplaySystem001"

//----------------------------------------------------------------------------------------

#endif // IREPLAYSYSTEM_H
