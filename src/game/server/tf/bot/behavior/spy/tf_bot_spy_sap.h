//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_sap.h
// Sap nearby enemy buildings
// Michael Booth, June 2010

#ifndef TF_BOT_SPY_SAP_H
#define TF_BOT_SPY_SAP_H

#include "Path/NextBotPathFollow.h"

class CTFBotSpySap : public Action< CTFBot >
{
public:
	CTFBotSpySap( CBaseObject *sapTarget );
	virtual ~CTFBotSpySap() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual ActionResult< CTFBot >	OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *blocker ) const;		// use this to signal the enemy we are focusing on, so we dont avoid them

	virtual const char *GetName( void ) const	{ return "SpySap"; };

private:
	CHandle< CBaseObject > m_sapTarget;

	CountdownTimer m_repathTimer;
	PathFollower m_path;

	CBaseObject *GetNearestKnownSappableTarget( CTFBot *me );
	bool AreAllDangerousSentriesSapped( CTFBot *me ) const;
};

#endif // TF_BOT_SPY_SAP_H
