//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef ICLIENTREPLAYCONTEXT_H
#define ICLIENTREPLAYCONTEXT_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/ireplaycontext.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

#define REPLAYHISTORYMANAGER_INTERFACE_VERSION_CLIENT		"VENGINE_CLIENT_REPLAY_HISTORY_MANAGER_001"

//----------------------------------------------------------------------------------------

class CReplay;
class CReplayPerformance;
class IReplayManager;
class IReplayMovieManager;
class IReplayMovieRenderer;
class IReplayScreenshotManager;
class IReplayPerformanceManager;
class IReplayPerformanceController;
class IReplayRenderQueue;

//----------------------------------------------------------------------------------------

class IClientReplayContext : public IReplayContext
{
public:
	virtual CReplay						*GetReplay( ReplayHandle_t hReplay ) = 0;	// Shorthand to GetReplayManager()->GetReplay()
	virtual IReplayManager				*GetReplayManager() = 0;
	virtual IReplayMovieRenderer		*GetMovieRenderer() = 0;
	virtual IReplayMovieManager			*GetMovieManager() = 0;
	virtual IReplayScreenshotManager	*GetScreenshotManager() = 0;
	virtual IReplayPerformanceManager	*GetPerformanceManager() = 0;
	virtual IReplayPerformanceController *GetPerformanceController() = 0;
	virtual IReplayRenderQueue			*GetRenderQueue() = 0;
	virtual void						SetMovieRenderer( IReplayMovieRenderer *pRenderer ) = 0;	// Set to be the panel that renders replay movies, or NULL when nothing is rendering
	virtual void						OnSignonStateFull() = 0;
	virtual void						OnClientSideDisconnect() = 0;	// Called when client disconnects
	virtual void						PlayReplay( ReplayHandle_t hReplay, int iPerformance, bool bPlaySound ) = 0;	// Play the given replay, from spawn tick to death tick
	virtual bool						ReconstructReplayIfNecessary( CReplay *pReplay ) = 0;
	virtual void						OnPlayerSpawn() = 0;	// Called on the client when player is spawned
	virtual void						OnPlayerClassChanged() = 0;	// Called when the player's class changes - we use this instead of an event for immediacy
	virtual void						GetPlaybackTimes( float &flOutTime, float &flOutLength, const CReplay *pReplay, const CReplayPerformance *pPerformance ) = 0;	// Calculate the current time and length of a replay or performance - takes in tick and out tick into account for performances - flCurTime should be gpGlobals->curtime. pPerformance can be NULL.
	virtual uint64						GetServerSessionId( ReplayHandle_t hReplay ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // ICLIENTREPLAYCONTEXT_H
