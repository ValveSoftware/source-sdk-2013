//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_move_to_vantage_point.h
// Move to a position where at least one enemy is visible
// Michael Booth, November 2009

#ifndef TF_BOT_MOVE_TO_VANTAGE_POINT_H
#define TF_BOT_MOVE_TO_VANTAGE_POINT_H

#include "Path/NextBotChasePath.h"

class CTFBotMoveToVantagePoint : public Action< CTFBot >
{
public:
	CTFBotMoveToVantagePoint( float maxTravelDistance = 2000.0f );
	virtual ~CTFBotMoveToVantagePoint() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "MoveToVantagePoint"; };

private:
	float m_maxTravelDistance;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CTFNavArea *m_vantageArea;
};

#endif // TF_BOT_MOVE_TO_VANTAGE_POINT_H
