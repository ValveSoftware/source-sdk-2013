//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_defend_point.h
// Move to and defend current point from capture
// Michael Booth, February 2009

#ifndef TF_BOT_DEFEND_POINT_H
#define TF_BOT_DEFEND_POINT_H

#include "Path/NextBotPathFollow.h"
#include "Path/NextBotChasePath.h"

class CTFBotDefendPoint : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual EventDesiredResult< CTFBot > OnTerritoryContested( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryCaptured( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );

	virtual const char *GetName( void ) const	{ return "DefendPoint"; };

private:
	PathFollower m_path;				// for moving to a defense position
	ChasePath m_chasePath;				// for chasing enemies

	CountdownTimer m_repathTimer;
	CountdownTimer m_lookAroundTimer;
	CountdownTimer m_idleTimer;

	CTFNavArea *m_defenseArea;
	CTFNavArea *SelectAreaToDefendFrom( CTFBot *me );

	bool IsPointThreatened( CTFBot *me );
	bool WillBlockCapture( CTFBot *me ) const;
	bool m_isAllowedToRoam;
};


#endif // TF_BOT_DEFEND_POINT_H
