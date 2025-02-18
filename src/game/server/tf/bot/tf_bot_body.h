//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_body.h
// Team Fortress NextBot body interface
// Michael Booth, May 2010

#ifndef TF_BOT_BODY_H
#define TF_BOT_BODY_H

#include "NextBot/Player/NextBotPlayerBody.h"

//----------------------------------------------------------------------------
class CTFBotBody : public PlayerBody
{
public:
	CTFBotBody( INextBot *bot ) : PlayerBody( bot )
	{
	}

	virtual ~CTFBotBody() { }

	virtual float GetHeadAimTrackingInterval( void ) const;			// return how often we should sample our target's position and velocity to update our aim tracking, to allow realistic slop in tracking
};

#endif // TF_BOT_BODY_H
