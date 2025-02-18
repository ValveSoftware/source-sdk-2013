//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef PLAYERSPAWNCACHE_H
#define PLAYERSPAWNCACHE_H
#ifdef _WIN32
#pragma once
#endif

//--------------------------------------------------------------------------------

#include "GameEventListener.h"

//--------------------------------------------------------------------------------

//
// I’m not sure if player spawn cache is the most descriptive name, but essentially
// there is a singleton instance of CPlayerSpawnCache for the local player which has
// a set of counters/pointers/etc. that get cleared every time a map loads.  This
// can be useful for situations where a player’s net connection chokes and they get
// a full update, which recreates their C_TF_Player entirely or otherwise invalidates
// a bunch of data in the local player.  I believe it’s already known that there is
// a class of bugs stemming from this behavior.
//
// Right now the cache is used as a way to display a message to the player if they
// connect to a server that’s recording replays.  As soon as they choose their player
// class, a counter is checked, and if it’s zero the message is displayed.  The counter
// is then incremented.  This is a sort of odd use for it actually.  A better example
// would be what I’m going to do next, which is that if the player’s net connection
// chokes (or if you host_timescale at too great a speed and cause a full update on the
// client), the replay system will think that you’ve already saved a replay for that life.
// So this example will be a perfect time to use the player spawn cache because you can
// maintain some level of persistence in the face of your entire local player getting
// nuked.
//
// Just add any data members you'd like to access to the CPlayerSpawnCache::Data_t 
// struct and it will be cleared automatically (via a memset) whenever a new map is
// loaded.
//
// It's possible that PreReset()/PostReset() or the like will be necessary for this
// class to reach its full potential.
//
class CPlayerSpawnCache : public CGameEventListener
{
public:
	static CPlayerSpawnCache &Instance();

	// Counters
	struct Data_t
	{
		int m_nDisplayedConnectedRecording;
		int	m_nDisplaySaveReplay;	// Don't display the "Press [f6] to save this life" the first time the spectator GUI is shown
	} m_Data;

private:
	CPlayerSpawnCache();

	virtual void FireGameEvent( IGameEvent *pEvent );

	void Reset();
};

//--------------------------------------------------------------------------------

#endif // PLAYERSPAWNCACHE_H
