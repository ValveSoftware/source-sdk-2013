//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_payload_push.h
// Push the cart to the goal
// Michael Booth, April 2010

#ifndef TF_BOT_PAYLOAD_PUSH_H
#define TF_BOT_PAYLOAD_PUSH_H

#include "Path/NextBotPathFollow.h"

class CTFBotPayloadPush : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "PayloadPush"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	float m_hideAngle;
};

#endif // TF_BOT_PAYLOAD_PUSH_H
