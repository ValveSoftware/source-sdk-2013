//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "passtime_game_events.h"
#include "igameevents.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
using namespace PasstimeGameEvents;

//-----------------------------------------------------------------------------
namespace 
{
	//-----------------------------------------------------------------------------
	template<class T>
	IGameEvent* CreateEvent()
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( T::s_eventName );
		Assert( pEvent );
		return pEvent;
	}

	//-----------------------------------------------------------------------------
	template<class T>
	bool IsType( IGameEvent *pEvent )
	{
		return FStrEq( pEvent->GetName(), T::s_eventName );
	}
}

//-----------------------------------------------------------------------------
const char *const BallGet::s_eventName = "pass_get";
const char *const BallGet::s_keyOwnerIndex = "owner";
const char *const BallGet::s_keyTeam = "team";

BallGet::BallGet( IGameEvent *pEvent )
	: ownerIndex( pEvent->GetInt( s_keyOwnerIndex ) )
	, team( pEvent->GetInt( s_keyTeam ) )
{
	Assert( IsType<BallGet>( pEvent ) );
}

BallGet::BallGet( int ownerIndex_, int team_ ) 
	: ownerIndex( ownerIndex_ )
	, team( team_ )
{
}

void BallGet::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<BallGet>() )
	{
		pEvent->SetInt( s_keyOwnerIndex, ownerIndex );
		pEvent->SetInt( s_keyTeam, team );
		gameeventmanager->FireEvent(pEvent);
	}
}

//-----------------------------------------------------------------------------
const char *const Score::s_eventName = "pass_score";
const char *const Score::s_keyScorerIndex = "scorer";
const char *const Score::s_keyAssisterIndex = "assister";
const char *const Score::s_keyNumPoints = "points";

Score::Score( IGameEvent *pEvent )
	: scorerIndex( pEvent->GetInt( s_keyScorerIndex ) )
	, assisterIndex( pEvent->GetInt( s_keyAssisterIndex ) )
	, numPoints( pEvent->GetInt( s_keyNumPoints ) )
{
	Assert( IsType<Score>( pEvent ) );
}

Score::Score( int scorerIndex_, int assisterIndex_, int numPoints_ )
	: scorerIndex( scorerIndex_ )
	, assisterIndex( assisterIndex_ )
	, numPoints( numPoints_ )
{
}

Score::Score( int scorerIndex_, int numPoints_ )
	: scorerIndex( scorerIndex_ )
	, assisterIndex( -1 )
	, numPoints( numPoints_ )
{
}

void Score::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<Score>() )
	{
		pEvent->SetInt( s_keyScorerIndex, scorerIndex );
		pEvent->SetInt( s_keyAssisterIndex, assisterIndex );
		pEvent->SetInt( s_keyNumPoints, numPoints );
		gameeventmanager->FireEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
const char *const BallFree::s_eventName = "pass_free";
const char *const BallFree::s_keyOwnerIndex = "owner";
const char *const BallFree::s_keyAttackerIndex = "attacker";

BallFree::BallFree( IGameEvent *pEvent )
	: ownerIndex( pEvent->GetInt( s_keyOwnerIndex ) )
	, attackerIndex( pEvent->GetInt( s_keyAttackerIndex ) )
{
	Assert( IsType<BallFree>( pEvent ) );
}

BallFree::BallFree()
	: ownerIndex( -1 )
	, attackerIndex( -1 )
{
}

BallFree::BallFree( int ownerIndex_ )
	: ownerIndex( ownerIndex_ )
	, attackerIndex( -1 )
{
}

BallFree::BallFree( int ownerIndex_, int attackerIndex_ )
	: ownerIndex( ownerIndex_ )
	, attackerIndex( attackerIndex_ )
{
}

void BallFree::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<BallFree>() )
	{
		pEvent->SetInt( s_keyOwnerIndex, ownerIndex );
		pEvent->SetInt( s_keyAttackerIndex, attackerIndex );
		gameeventmanager->FireEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
const char *const PassCaught::s_eventName = "pass_pass_caught";
const char *const PassCaught::s_keyPasserIndex = "passer";
const char *const PassCaught::s_keyCatcherIndex = "catcher";
const char *const PassCaught::s_keyDist = "dist";
const char *const PassCaught::s_keyDuration = "duration";

PassCaught::PassCaught( IGameEvent *pEvent )
	: passerIndex( pEvent->GetInt( s_keyPasserIndex ) )
	, catcherIndex( pEvent->GetInt( s_keyCatcherIndex ) )
	, dist( pEvent->GetFloat( s_keyDist ) )
	, duration( pEvent->GetFloat( s_keyDuration ) ) 
{
	Assert( IsType<PassCaught>( pEvent ) );
}

PassCaught::PassCaught()
	: passerIndex( -1 )
	, catcherIndex( -1 ) 
	, dist( 0 )
	, duration( 0 )
{
}

PassCaught::PassCaught( int passerIndex_, int catcherIndex_, float dist_, float duration_ )
	: passerIndex( passerIndex_ )
	, catcherIndex( catcherIndex_ )
	, dist( dist_ )
	, duration( duration_ )
{
}

void PassCaught::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<PassCaught>() )
	{
		pEvent->SetInt( s_keyPasserIndex, passerIndex );
		pEvent->SetInt( s_keyCatcherIndex, catcherIndex );
		pEvent->SetFloat( s_keyDist, dist );
		pEvent->SetFloat( s_keyDuration, duration );
		gameeventmanager->FireEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
const char *const BallStolen::s_eventName = "pass_ball_stolen";
const char *const BallStolen::s_keyVictimIndex = "victim";
const char *const BallStolen::s_keyAttackerIndex = "attacker";

BallStolen::BallStolen( IGameEvent *pEvent )
	: victimIndex( pEvent->GetInt( s_keyVictimIndex ) )
	, attackerIndex( pEvent->GetInt( s_keyAttackerIndex ) )
{
	Assert( IsType<BallStolen>( pEvent ) );
}

BallStolen::BallStolen()
	: victimIndex( -1 )
	, attackerIndex( -1 )
{
}

BallStolen::BallStolen( int victimIndex_, int attackerIndex_ )
	: victimIndex( victimIndex_ )
	, attackerIndex( attackerIndex_ )
{
}

void BallStolen::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<BallStolen>() )
	{
		pEvent->SetInt( s_keyVictimIndex, victimIndex );
		pEvent->SetInt( s_keyAttackerIndex, attackerIndex );
		gameeventmanager->FireEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
const char *const BallBlocked::s_eventName = "pass_ball_blocked";
const char *const BallBlocked::s_keyOwnerIndex = "owner";
const char *const BallBlocked::s_keyBlockerIndex = "blocker";

BallBlocked::BallBlocked( IGameEvent *pEvent )
	: ownerIndex( pEvent->GetInt( s_keyOwnerIndex ) )
	, blockerIndex( pEvent->GetInt( s_keyBlockerIndex ) )
{
	Assert( IsType<BallBlocked>( pEvent ) );
}

BallBlocked::BallBlocked()
	: ownerIndex( -1 )
	, blockerIndex( -1 )
{
}

BallBlocked::BallBlocked( int ownerIndex_, int blockerIndex_ )
	: ownerIndex( ownerIndex_ )
	, blockerIndex( blockerIndex_ )
{
}

void BallBlocked::Fire()
{
	if ( IGameEvent *pEvent = CreateEvent<BallBlocked>() )
	{
		pEvent->SetInt( s_keyOwnerIndex, ownerIndex );
		pEvent->SetInt( s_keyBlockerIndex, blockerIndex );
		gameeventmanager->FireEvent( pEvent );
	}
}
