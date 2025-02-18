//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_dead.h
// Push up daisies
// Michael Booth, May 2009

#ifndef TF_BOT_DEAD_H
#define TF_BOT_DEAD_H

#include "Path/NextBotChasePath.h"

class CTFBotDead : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "Dead"; };

private:
	IntervalTimer m_deadTimer;
};

#endif // TF_BOT_DEAD_H
