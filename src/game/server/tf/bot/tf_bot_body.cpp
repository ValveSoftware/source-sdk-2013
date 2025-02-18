//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_body.cpp
// Team Fortress NextBot body interface
// Michael Booth, May 2010

#include "cbase.h"

#include "tf_bot.h"
#include "tf_bot_body.h"


// 
// Return how often we should sample our target's position and 
// velocity to update our aim tracking, to allow realistic slop in tracking
//
float CTFBotBody::GetHeadAimTrackingInterval( void ) const
{
	CTFBot *me = (CTFBot *)GetBot();

	// don't let Spies in MvM mode aim too precisely
	if ( TFGameRules()->IsMannVsMachineMode() && me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return 0.25f;
	}

	switch( me->GetDifficulty() )
	{
	case CTFBot::EXPERT:
		return 0.05f;

	case CTFBot::HARD:
		return 0.1f;

	case CTFBot::NORMAL:
		return 0.25f;

	case CTFBot::EASY:
		return 1.0f;
	}

	return 0.0f;
}
