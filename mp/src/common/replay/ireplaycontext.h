//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYCONTEXT_H
#define IREPLAYCONTEXT_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class IRecordingSessionManager;
class IRecordingSessionBlockManager;
class IRecordingSession;
class IReplayErrorSystem;

//----------------------------------------------------------------------------------------

class IReplayContext : public IBaseInterface
{
public:
	virtual bool			Init( CreateInterfaceFn fnCreateFactory ) = 0;
	virtual void			Shutdown() = 0;

	virtual void			Think() = 0;

	virtual bool			IsInitialized() const = 0;
	
	virtual const char		*GetRelativeBaseDir() const = 0;	// Returns path to wherever the index .dmx lives relative to the game path, e.g. "replay\client\"
	virtual const char		*GetBaseDir() const = 0;	// Returns full directory to wherever the index .dmx lives, e.g. c:\program files (x86)\steam\steamapps\<username>\team fortress 2\tf\replays\<client|server>\ -- NOTE: includes trailing slash
	virtual const char		*GetReplaySubDir() const = 0;	// Returns "client" or "server"

	virtual IReplayErrorSystem	*GetErrorSystem() = 0;
	virtual IRecordingSessionManager		*GetRecordingSessionManager() = 0;
	virtual IRecordingSessionBlockManager	*GetRecordingSessionBlockManager() = 0;
	virtual IRecordingSession	*GetRecordingSession( ReplayHandle_t hSession ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYCONTEXT_H
