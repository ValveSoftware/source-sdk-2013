//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "hl2mp_bot.h"
#include "hl2mp_bot_body.h"


// 
// Return how often we should sample our target's position and 
// velocity to update our aim tracking, to allow realistic slop in tracking
//
float CHL2MPBotBody::GetHeadAimTrackingInterval( void ) const
{
	CHL2MPBot *me = (CHL2MPBot *)GetBot();

	switch( me->GetDifficulty() )
	{
	case CHL2MPBot::EXPERT:
		return 0.05f;

	case CHL2MPBot::HARD:
		return 0.1f;

	case CHL2MPBot::NORMAL:
		return 0.25f;

	case CHL2MPBot::EASY:
		return 1.0f;
	}

	return 0.0f;
}
