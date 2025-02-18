//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef ICLIENTREPLAY_H
#define ICLIENTREPLAY_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

#define CLIENT_REPLAY_INTERFACE_VERSION		"ClientReplay001"

//----------------------------------------------------------------------------------------

class IReplayFactory;
class IReplayScreenshotSystem;
class IReplayPerformancePlaybackHandler;
class KeyValues;
class IReplayCamera;
class CReplayPerformance;
struct RenderMovieParams_t;
class IGameEvent;

//----------------------------------------------------------------------------------------

//
// Allows the replay and engine DLL's to talk to the client.
//
class IClientReplay : public IBaseInterface
{
public:
	virtual uint64			GetServerSessionId() = 0; 
	virtual bool			CacheReplayRagdolls( const char* pFilename, int nStartTick ) = 0;	// Cache replay ragdolls
	virtual IReplayScreenshotSystem *GetReplayScreenshotSystem() = 0;	// Get the client's replay screenshot system
	virtual IReplayPerformancePlaybackHandler *GetPerformancePlaybackHandler() = 0;
	virtual IReplayCamera	*GetReplayCamera() = 0;
	virtual void			DisplayReplayMessage( const char *pLocalizeStr, bool bUrgent, bool bDlg, const char *pSound ) = 0;
	virtual void			DisplayReplayMessage( const wchar_t *pText, bool bUrgent, bool bDlg, const char *pSound ) = 0;
	virtual void			InitPerformanceEditor( ReplayHandle_t hReplay ) = 0;
	virtual void			HidePerformanceEditor() = 0;
	virtual bool			ShouldRender() = 0;
	virtual void			PlaySound( const char *pSound ) = 0;
	virtual void			UploadOgsData( KeyValues *pData, bool bIncludeTimeField ) = 0;
	virtual bool			ShouldCompletePendingReplay( IGameEvent *pEvent ) = 0;

	virtual void			OnSaveReplay( ReplayHandle_t hNewReplay, bool bShowInputDlg ) = 0;
	virtual void			OnDeleteReplay( ReplayHandle_t hReplay ) = 0;	// Called before replay is actually removed from the replay manager
	virtual void			OnPlaybackComplete( ReplayHandle_t hReplay, int iPerformance ) = 0;
	virtual void			OnRenderStart() = 0;
	virtual void			OnRenderComplete( const RenderMovieParams_t &RenderParams, bool bCancelled, bool bSuccess, bool bShowBrowser ) = 0;
	virtual bool			OnConfirmQuit() = 0;
	virtual bool			OnEndOfReplayReached() = 0;
};

//----------------------------------------------------------------------------------------

#endif // ICLIENTREPLAY_H
