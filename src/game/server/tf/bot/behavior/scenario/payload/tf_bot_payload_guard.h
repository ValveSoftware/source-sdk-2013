//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_payload_guard.h
// Guard the payload and keep the attackers from getting near it
// Michael Booth, April 2010

#ifndef TF_BOT_PAYLOAD_GUARD_H
#define TF_BOT_PAYLOAD_GUARD_H

#include "Path/NextBotPathFollow.h"

class CTFBotPayloadGuard : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual EventDesiredResult< CTFBot > OnTerritoryContested( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryCaptured( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "PayloadGuard"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	
	Vector m_vantagePoint;
	CountdownTimer m_vantagePointTimer;
	Vector FindVantagePoint( CTFBot *me, CBaseEntity *cart );

	CountdownTimer m_moveToBlockTimer;

};

#endif // TF_BOT_PAYLOAD_GUARD_H
