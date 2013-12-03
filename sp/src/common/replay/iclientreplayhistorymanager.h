//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef ICLIENTREPLAYHISTORYMANAGER_H
#define ICLIENTREPLAYHISTORYMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/replayhandle.h"
#include "replay/screenshot.h"
#include "interface.h"
#include "qlimits.h"
#include "convar.h"
#include "engine/http.h"
#include "tier1/utllinkedlist.h"
#include "tier1/checksum_crc.h"
#include <time.h>

//----------------------------------------------------------------------------------------

class IReplayDownloadGroup;
class IReplayDownloadGroupHelper;
class CDmxElement;
class KeyValues;
struct CaptureScreenshotParams_t;
struct RenderMovieParams_t;
class CBaseReplay;
class CReplay;
class IReplayMovieRenderer;
class IReplayMovieManager;
class IReplayMovie;
class IReplayPerformanceManager;
class IGameEvent;

//----------------------------------------------------------------------------------------

class IClientReplayHistoryManager : public IBaseInterface
{
public:
	virtual bool			Init( CreateInterfaceFn fnCreateFactory ) = 0;
	virtual void			Shutdown() = 0;
	virtual void			Think() = 0;

	virtual bool			IsInitialized() const = 0;
	virtual bool			Commit( CBaseReplay *pNewReplay ) = 0;
	
	virtual void			Save() = 0;	// Write the entire index and any replays/groups/movies that are marked as dirty
	virtual void			FlagReplayForFlush( CBaseReplay *pReplay, bool bForceImmediateWrite ) = 0;	// Mark the given replay as dirty - flush to disk at the next opportunity (see CBaseReplayHistoryManager::FlushThink())

	virtual void			Nuke() = 0;

	virtual void			DeleteReplay( ReplayHandle_t hReplay, bool bNotifyUI ) = 0;
	virtual CBaseReplay		*GetReplay( ReplayHandle_t hReplay ) = 0;

	virtual const char		*GetBaseDirectory() = 0;	// Returns full directory to wherever replays.dmx lives, e.g. c:\program files (x86)\steam\steamapps\someuser\team fortress 2\game\tf\replays\client\ (or server\) - NOTE: includes trailing slash
	virtual const char		*GetReplaysSubDir() = 0;	// Returns "client" or "server"

	// For loop through all replays - indices should not be cached
	virtual int				GetReplayCount() const = 0;
//	virtual CBaseReplay		*GetReplayAtIndex( int nIndex ) = 0;

	virtual const char		*GetFullReplayPath() = 0;		// Get c:\...\game\tf\replays\<client or server>\

	// Client-specific
	virtual int				GetAdjustedDeathTick( CReplay *pReplay ) = 0;
	virtual void			FlagDownloadGroupForFlush( IReplayDownloadGroup *pGroup, bool bForceImmediate ) = 0;
	virtual void			FlagMovieForFlush( IReplayMovie *pMovie, bool bForceImmediate ) = 0;	// Flag the movie for flush - if pMovie is NULL, mark the index for flush
	virtual void			SetMovieRenderer( IReplayMovieRenderer *pRenderer ) = 0;	// Set to be the panel that renders replay movies, or NULL when nothing is rendering
	virtual bool			ShouldGameRenderView() = 0;	// Called from V_RenderView() to determine whether the game should render - used during movie rendering
	virtual int				GetUnrenderedReplayCount() = 0;	// Get the number of unrendered replays
	virtual void			UpdateCurrentReplayDataFromServer() = 0;	// Updates start tick, current file url, demo filename
	virtual void			LinkReplayToDownloadGroup() = 0;
	virtual void			CaptureScreenshot( CaptureScreenshotParams_t &params ) = 0;	// Schedules a screenshot capture at flDelay seconds in the future
	virtual void			DoCaptureScreenshot() = 0;					// Takes the screenshot right now
	virtual bool			ShouldCaptureScreenshot() = 0;				// Is screenshot scheduled to be taken right now?
	virtual void			GetUnpaddedScreenshotSize( int &nWidth, int &nHeight ) = 0;	// Get the dimensions for a screenshot if we take one right now, based on replay_screenshotresolution and the current aspect ratio
	virtual void			DeleteScreenshotsForReplay( CReplay *pReplay ) = 0;	// Deletes all screenshots associated with the given replay
	virtual void			PlayReplay( ReplayHandle_t hReplay ) = 0;	// Play the given replay, from spawn tick to death tick
	virtual void			RenderMovie( RenderMovieParams_t const& params ) = 0;	// Renders the given replay - or if params.hReplay is -1, render all unrendered replays
	virtual void			CompleteRender( bool bSuccess ) = 0;
	virtual void			OnClientSideDisconnect() = 0;	// Called when client disconnects
	virtual void			OnSignonStateFull() = 0;
	virtual void			OnPlayerSpawn() = 0;	// Called on the client when player is spawned
	virtual void			OnPlayerClassChanged() = 0;	// Called when the player's class changes - we use this instead of an event for immediacy
	virtual void			OnReplayRecordingCvarChanged() = 0;	// Called (on client only) when replay_recording is set to 1
	virtual void			OnGroupDeleted() = 0;
	virtual CReplay			*GetPlayingReplay() = 0;			// Get the currently playing replay, otherwise NULL if one isn't playing
	virtual CReplay			*GetReplayForCurrentLife() = 0;	// Gets the current replay (constant from local player spawn until next spawn/disconnect/exit)
	virtual bool			IsRendering() = 0;		// Are we currently rendering a movie?
	virtual void			CancelRender() = 0;	// If we're currently rendering, cancel
	virtual IReplayMovieManager		*GetMovieManager() = 0;
	virtual IReplayMovieRenderer	*GetMovieRenderer() = 0;
	virtual const RenderMovieParams_t	*GetRenderSettings() = 0;
	virtual IReplayDownloadGroupHelper	*GetDownloadGroupHelper() = 0;
	virtual IReplayPerformanceManager	*GetPerformanceManager() = 0;
};

//----------------------------------------------------------------------------------------

#endif // ICLIENTREPLAYHISTORYMANAGER_H
