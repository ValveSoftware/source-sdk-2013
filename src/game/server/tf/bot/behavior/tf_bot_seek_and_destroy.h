//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_seek_and_destroy.h
// Roam the environment, attacking victims
// Michael Booth, January 2010

#ifndef TF_BOT_SEEK_AND_DESTROY_H
#define TF_BOT_SEEK_AND_DESTROY_H

#include "Path/NextBotChasePath.h"


//
// Roam around the map attacking enemies
//
class CTFBotSeekAndDestroy : public Action< CTFBot >
{
public:
	CTFBotSeekAndDestroy( float duration = -1.0f );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual EventDesiredResult< CTFBot > OnTerritoryCaptured( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryContested( CTFBot *me, int territoryID );

	virtual const char *GetName( void ) const	{ return "SeekAndDestroy"; };

private:
	CTFNavArea *m_goalArea;
	CTFNavArea *ChooseGoalArea( CTFBot *me );
	bool m_isPointLocked;

	PathFollower m_path;
	CountdownTimer m_repathTimer;
	void RecomputeSeekPath( CTFBot *me );

	CountdownTimer m_giveUpTimer;
};


#endif // TF_BOT_SEEK_AND_DESTROY_H
