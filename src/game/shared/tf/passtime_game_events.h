//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PASSTIME_GAME_EVENTS_H
#define PASSTIME_GAME_EVENTS_H
#ifdef _WIN32
#pragma once
#endif

class IGameEvent;
namespace PasstimeGameEvents
{
	// TODO: this was done following valve's style of having different events 
	// for everything, but these particular events have a lot of overlap and 
	// might be better implemented as a single "ball event" that has an enum 
	// specifying what kind it is. It would cut down on the number of strcmp 
	// calls in the event handling functions. Or maybe we could just not use 
	// 1000s of strcmps for each event dispatch and use a lookup table of some kind.

	//-----------------------------------------------------------------------------
	struct BallGet
	{
		BallGet( IGameEvent *pEvent );
		BallGet( int ownerIndex, int team );
		void Fire();
		
		static const char *const s_eventName;
		static const char *const s_keyOwnerIndex;
		static const char *const s_keyTeam;
		int ownerIndex;
		int team;
	};
	
	//-----------------------------------------------------------------------------
	struct Score
	{
		Score( IGameEvent *pEvent );
		Score( int scorerIndex, int assisterIndex, int numPoints );
		Score( int scorerIndex_, int numPoints_ );
		void Fire();
		
		static const char *const s_eventName;
		static const char *const s_keyScorerIndex;
		static const char *const s_keyAssisterIndex;
		static const char *const s_keyNumPoints;
		int scorerIndex;
		int assisterIndex;
		int numPoints;
	};

	//-----------------------------------------------------------------------------
	struct BallFree
	{
		BallFree( IGameEvent *pEvent );
		BallFree();
		BallFree( int ownerIndex );
		BallFree( int ownerIndex, int attackerIndex );
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyOwnerIndex;
		static const char *const s_keyAttackerIndex;
		int ownerIndex;
		int attackerIndex;
	};

	//-----------------------------------------------------------------------------
	struct PassCaught
	{
		PassCaught( IGameEvent *pEvent );
		PassCaught();
		PassCaught( int passerIndex, int catcherIndex, float dist, float duration );
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyPasserIndex;
		static const char *const s_keyCatcherIndex;
		static const char *const s_keyDist;
		static const char *const s_keyDuration;
		int passerIndex;
		int catcherIndex;
		float dist;
		float duration;
	};

	//-----------------------------------------------------------------------------
	struct BallStolen
	{
		BallStolen( IGameEvent *pEvent );
		BallStolen();
		BallStolen( int victimIndex, int attackerIndex );
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyVictimIndex;
		static const char *const s_keyAttackerIndex;
		int victimIndex;
		int attackerIndex;
	};

	//-----------------------------------------------------------------------------
	struct BallBlocked
	{
		BallBlocked( IGameEvent *pEvent );
		BallBlocked();
		BallBlocked( int ownerIndex, int blockerIndex );
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyOwnerIndex;
		static const char *const s_keyBlockerIndex;
		int ownerIndex;
		int blockerIndex;
	};
}

#endif // PASSTIME_GAME_EVENTS_H  
