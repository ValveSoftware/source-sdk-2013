//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef IREPLAYPLAYERCACHE_H
#define IREPLAYPLAYERCACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//----------------------------------------------------------------------------------------

#define REPLAYPLAYERCACHE_INTERFACE_VERSION		"VENGINE_REPLAY_PLAYER_CACHE_001"

//----------------------------------------------------------------------------------------

abstract_class IReplayPlayerCache : public IBaseInterface
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;

	virtual void SetupPlayer( int nEntIndex ) = 0;
	virtual void DeletePlayerEntry( int nEntIndex ) = 0;

	virtual bool PlayerHasCacheEntry( int nEntIndex ) = 0;

	virtual void SetPlayerClass( int nEntIndex, const char *pPlayerClass ) = 0;
	virtual void SetPlayerSpawnTick( int nEntIndex, int nTick ) = 0;
	virtual void SetPlayerDeathTick( int nEntIndex, int nTick ) = 0;

	virtual const char *GetPlayerClass( int nEntIndex ) = 0;
	virtual int GetPlayerSpawnTick( int nEntIndex ) = 0;
	virtual int GetPlayerDeathTick( int nEntIndex ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPLAYERCACHE_H
