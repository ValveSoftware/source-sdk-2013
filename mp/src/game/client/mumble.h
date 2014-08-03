//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains an interface to enable positional audio support in Mumble
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef MUMBLE_H
#define MUMBLE_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "GameEventListener.h"

class CMumbleSystem : public CBaseGameSystemPerFrame, public CGameEventListener
{
public:
	// Methods of IGameSystem
	virtual bool	Init();
	virtual void	LevelInitPostEntity();
	virtual void	LevelShutdownPreEntity();
	virtual void	PostRender();

	// Methods of IGameSystem
	virtual void FireGameEvent( IGameEvent *event );

private:
	char m_szSteamIDCurrentServer[32];
	int m_cubSteamIDCurrentServer;
	bool m_bHasSetPlayerUniqueId;
	int m_nTeamSetInUniqueId;
};

IGameSystem *MumbleSystem();

#endif // MUMBLE_H