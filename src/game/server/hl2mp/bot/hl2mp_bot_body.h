//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_BODY_H
#define HL2MP_BOT_BODY_H

#include "NextBot/Player/NextBotPlayerBody.h"

//----------------------------------------------------------------------------
class CHL2MPBotBody : public PlayerBody
{
public:
	CHL2MPBotBody( INextBot* bot ) : PlayerBody( bot )
	{
	}

	virtual ~CHL2MPBotBody() { }

	virtual float GetHeadAimTrackingInterval( void ) const;			// return how often we should sample our target's position and velocity to update our aim tracking, to allow realistic slop in tracking
};

#endif // HL2MP_BOT_BODY_H
