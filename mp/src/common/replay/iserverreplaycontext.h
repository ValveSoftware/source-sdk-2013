//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef ISERVERREPLAYCONTEXT_H
#define ISERVERREPLAYCONTEXT_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/ireplaycontext.h"

//----------------------------------------------------------------------------------------

class IGameEvent;
class IReplaySessionRecorder;

//----------------------------------------------------------------------------------------

#define REPLAYHISTORYMANAGER_INTERFACE_VERSION_SERVER		"VENGINE_SERVER_REPLAY_HISTORY_MANAGER_001"

//----------------------------------------------------------------------------------------

class IServerReplayContext : public IReplayContext
{
public:
	virtual void			FlagForConVarSanityCheck() = 0;	// Checks replay_enable / replay_local_fileserver_path / replay_downloadurlport / replay_downloadurlpath
	virtual IGameEvent		*CreateReplaySessionInfoEvent() = 0;	// Create "replay_sessioninfo" event w/ appropriate fields filled in
	virtual IReplaySessionRecorder	*GetSessionRecorder() = 0;
	virtual const char		*GetLocalFileServerPath() const = 0;	// Returns the local path where session blocks and such should be published for download
	virtual void			CreateSessionOnClient( int nClientSlot ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // ISERVERREPLAYCONTEXT_H
