//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_escape.h
// Flee!
// Michael Booth, June 2010

#ifndef TF_BOT_SPY_ESCAPE_H
#define TF_BOT_SPY_ESCAPE_H

#include "Path/NextBotPathFollow.h"

class CTFBotSpyEscape : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "SpyEscape"; };

private:
};

#endif // TF_BOT_SPY_ESCAPE_H
