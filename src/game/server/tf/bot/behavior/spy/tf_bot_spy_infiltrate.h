//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_infiltrate.h
// Move into position behind enemy lines and wait for victims
// Michael Booth, June 2010

#ifndef TF_BOT_SPY_INFILTRATE_H
#define TF_BOT_SPY_INFILTRATE_H

#include "Path/NextBotPathFollow.h"

class CTFBotSpyInfiltrate : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );
	virtual ActionResult< CTFBot >	OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnTerritoryCaptured( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "SpyInfiltrate"; };

private:
	CountdownTimer m_repathTimer;
	PathFollower m_path;

	CTFNavArea *m_hideArea;
	bool FindHidingSpot( CTFBot *me );
	CountdownTimer m_findHidingSpotTimer;

	CountdownTimer m_waitTimer;

	bool m_hasEnteredCombatZone;
};


#endif // TF_BOT_SPY_INFILTRATE_H
