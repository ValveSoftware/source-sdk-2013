//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains an interface to log some events to the Steam Share 
//			timeline via ISteamVideo 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef STEAMSHARE_H
#define STEAMSHARE_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "GameEventListener.h"

class CSteamShareSystem : public CBaseGameSystemPerFrame, public CGameEventListener
{
public:
	// Methods of IGameSystem
	virtual bool	Init();

	// Methods of IGameEventListener2
	virtual void FireGameEvent( IGameEvent *event );

private:
};

extern IGameSystem *SteamShareSystem();

#endif // STEAMSHARE_H
