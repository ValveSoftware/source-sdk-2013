//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_MOVE_TO_VANTAGE_POINT_H
#define HL2MP_BOT_MOVE_TO_VANTAGE_POINT_H

#include "Path/NextBotChasePath.h"

class CHL2MPBotMoveToVantagePoint : public Action< CHL2MPBot >
{
public:
	CHL2MPBotMoveToVantagePoint( float maxTravelDistance = 2000.0f );
	virtual ~CHL2MPBotMoveToVantagePoint() { }

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot *me, const Path *path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "MoveToVantagePoint"; };

private:
	float m_maxTravelDistance;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CNavArea *m_vantageArea;
};

#endif // HL2MP_BOT_MOVE_TO_VANTAGE_POINT_H
