//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_guard_area.h
// Defend an area against intruders
// Michael Booth, October 2009

#ifdef TF_RAID_MODE

#ifndef TF_BOT_GUARD_AREA_H
#define TF_BOT_GUARD_AREA_H

#include "Path/NextBotChasePath.h"

class CTFBotGuardArea : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?

	virtual EventDesiredResult< CTFBot > OnCommandApproach( CTFBot *me, const Vector &pos, float range );

	virtual const char *GetName( void ) const	{ return "GuardArea"; };

private:
	ChasePath m_chasePath;
	PathFollower m_pathToPoint;
	PathFollower m_pathToVantageArea;
	CountdownTimer m_vocalizeTimer;
	CountdownTimer m_repathTimer;
};

#endif // TF_RAID_MODE

#endif // TF_BOT_GUARD_AREA_H
