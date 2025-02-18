//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_SEEK_AND_DESTROY_H
#define HL2MP_BOT_SEEK_AND_DESTROY_H

#include "Path/NextBotChasePath.h"


//
// Roam around the map attacking enemies
//
class CHL2MPBotSeekAndDestroy : public Action< CHL2MPBot >
{
public:
	CHL2MPBotSeekAndDestroy( float duration = -1.0f );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual ActionResult< CHL2MPBot >	OnResume( CHL2MPBot *me, Action< CHL2MPBot > *interruptingAction );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot *me, const Path *path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual EventDesiredResult< CHL2MPBot > OnTerritoryCaptured( CHL2MPBot *me, int territoryID );
	virtual EventDesiredResult< CHL2MPBot > OnTerritoryLost( CHL2MPBot *me, int territoryID );
	virtual EventDesiredResult< CHL2MPBot > OnTerritoryContested( CHL2MPBot *me, int territoryID );

	virtual EventDesiredResult< CHL2MPBot > OnCommandApproach( CHL2MPBot *me, const Vector& pos, float range );

	virtual const char *GetName( void ) const	{ return "SeekAndDestroy"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CountdownTimer m_itemStolenTimer;
	EHANDLE m_hTargetEntity;
	bool m_bGoingToTargetEntity = false;
	Vector m_vGoalPos = vec3_origin;
	bool m_bTimerElapsed = false;
	void RecomputeSeekPath( CHL2MPBot *me );

	bool m_bOverrideApproach = false;
	Vector m_vOverrideApproach = vec3_origin;

	CountdownTimer m_giveUpTimer;
};


#endif // HL2MP_BOT_SEEK_AND_DESTROY_H
