//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_deliver_flag.h
// Take the flag we are holding to its destination
// Michael Booth, May 2011

#ifndef TF_BOT_DELIVER_FLAG_H
#define TF_BOT_DELIVER_FLAG_H

#include "Path/NextBotPathFollow.h"


//-----------------------------------------------------------------------------
class CTFBotDeliverFlag : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;

	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "DeliverFlag"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	float m_flTotalTravelDistance;

	bool UpgradeOverTime( CTFBot *me );
	CountdownTimer m_upgradeTimer;

#define DONT_UPGRADE -1
	int m_upgradeLevel;

	CountdownTimer m_buffPulseTimer;
};


//-----------------------------------------------------------------------------
class CTFBotPushToCapturePoint : public Action< CTFBot >
{
public:
	CTFBotPushToCapturePoint( Action< CTFBot > *nextAction = NULL );
	virtual ~CTFBotPushToCapturePoint() { }

	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual EventDesiredResult< CTFBot > OnNavAreaChanged( CTFBot *me, CNavArea *newArea, CNavArea *oldArea );

	virtual const char *GetName( void ) const	{ return "PushToCapturePoint"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;

	Action< CTFBot > *m_nextAction;
};


#endif // TF_BOT_DELIVER_FLAG_H
