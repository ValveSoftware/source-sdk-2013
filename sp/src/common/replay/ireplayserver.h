//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef IREPLAYSERVER_H
#define IREPLAYSERVER_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "interface.h"

//-----------------------------------------------------------------------------

class IServer;
class IReplayDirector;
class IGameEvent;
struct netadr_s;
class CServerReplay;

//-----------------------------------------------------------------------------
// Interface the Replay module exposes to the engine
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_REPLAYSERVER	"ReplayServer001"

class IReplayServer : public IBaseInterface
{
public:
	virtual	~IReplayServer() {}

	virtual	IServer	*GetBaseServer() = 0; // get Replay base server interface
	virtual	IReplayDirector *GetDirector() = 0;	// get director interface
	virtual	int		GetReplaySlot() = 0; // return entity index-1 of Replay in game
	virtual float	GetOnlineTime() = 0; // seconds since broadcast started
	virtual void	BroadcastEvent(IGameEvent *event) = 0; // send a director command to all specs
	virtual bool	IsRecording() = 0;
	virtual void	StartRecording() = 0;
	virtual void	StopRecording() = 0;
};

#endif
