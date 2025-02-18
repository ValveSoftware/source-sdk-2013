//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_backstab.h
// Chase behind a victim and backstab them
// Michael Booth, June 2010

#ifndef TF_BOT_SPY_BACKSTAB_H
#define TF_BOT_SPY_BACKSTAB_H

#include "Path/NextBotPathFollow.h"

class CTFBotSpyBackstab : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "SpyBackstab"; };

private:
};

#endif // TF_BOT_SPY_BACKSTAB_H
