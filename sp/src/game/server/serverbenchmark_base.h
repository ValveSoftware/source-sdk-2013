//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SERVERBENCHMARK_BASE_H
#define SERVERBENCHMARK_BASE_H
#ifdef _WIN32
#pragma once
#endif


// The base server code calls into this.
class IServerBenchmark
{
public:
	virtual bool StartBenchmark() = 0;
	virtual void UpdateBenchmark() = 0;
	virtual void EndBenchmark() = 0;
	
	virtual bool IsBenchmarkRunning() = 0;
	virtual bool IsLocalBenchmarkPlayer( CBasePlayer *pPlayer ) = 0;

	// Game-specific benchmark code should use this.
	virtual int RandomInt( int nMin, int nMax ) = 0;
	virtual float RandomFloat( float flMin, float flMax ) = 0;
	virtual int GetTickOffset() = 0;
};

extern IServerBenchmark *g_pServerBenchmark;


//
// Each game can derive from this to hook into the server benchmark.
//
// Hooks should always use g_pServerBenchmark->RandomInt/Float to get random numbers
// so the benchmark is deterministic.
//
// If they use an absolute tick number for anything, then they should also call g_pServerBenchmark->GetTickOffset() 
// to get a tick count since the start of the benchmark instead of looking at gpGlobals->tickcount.
//
class CServerBenchmarkHook
{
public:
	CServerBenchmarkHook();

	virtual void StartBenchmark()  {}
	virtual void UpdateBenchmark() {}
	virtual void EndBenchmark() {}

	// Give a list of model names that can be spawned in for physics props during the simulation.
	virtual void GetPhysicsModelNames( CUtlVector<char*> &modelNames ) = 0;

	// The benchmark will call this to create a bot each time it wants to create a player.
	// If you want to manage the bots yourself, you can return NULL here.
	virtual CBasePlayer* CreateBot() = 0;

private:
	friend class CServerBenchmark;
	static CServerBenchmarkHook *s_pBenchmarkHook; // There can be only one!!
};


#endif // SERVERBENCHMARK_BASE_H
