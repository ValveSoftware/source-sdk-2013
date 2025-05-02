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
		Score( int scorerIndex, int assisterIndex, int numPoints, bool isDeathBomb, bool isPanacea, bool isWinstrat );
		Score( int scorerIndex_, int numPoints_, bool isPanacea_, bool isWinstrat_ );
		void Fire();
		
		static const char *const s_eventName;
		static const char *const s_keyScorerIndex;
		static const char *const s_keyAssisterIndex;
		static const char *const s_keyNumPoints;
		static const char *const s_keyIsDeathBomb;
		static const char *const s_keyIsPanacea;
		static const char *const s_keyIsWinstrat;

		int scorerIndex;
		int assisterIndex;
		int numPoints;

		bool isDeathBomb;
		bool isPanacea;
		bool isWinstrat;
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
		PassCaught( int passerIndex, int catcherIndex, float dist, float duration, bool isHandoff, bool isBlock );
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyPasserIndex;
		static const char *const s_keyCatcherIndex;
		static const char *const s_keyDist;
		static const char *const s_keyDuration;
		static const char *const s_keyIsHandoff;
		static const char *const s_keyIsBlock;
		int passerIndex;
		int catcherIndex;
		float dist;
		float duration;
		bool isHandoff;
		bool isBlock;
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
	//-----------------------------------------------------------------------------
	struct BallDirected
	{
		BallDirected( IGameEvent *pEvent );
		BallDirected( int attackerIndex, int inflictorIndex, const char *inflictorName );
		BallDirected();

		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyAttackerIndex;
		static const char *const s_keyInflictorIndex;
		static const char *const s_keyInflictorName;

		int attackerIndex;
		int inflictorIndex;
		const char *inflictorName;
	};
	struct BallSplashed
	{
		BallSplashed( IGameEvent *pEvent );
		BallSplashed( int attackerIndex, const char *inflictorName, bool isDirect );
		BallSplashed();
		
		void Fire();

		static const char *const s_eventName;
		static const char *const s_keyAttackerIndex;
		static const char *const s_keyIsDirect;
		static const char *const s_keyInflictorName;

		int attackerIndex;
		bool isDirect;
		const char *inflictorName;
	};
}

#endif // PASSTIME_GAME_EVENTS_H  
