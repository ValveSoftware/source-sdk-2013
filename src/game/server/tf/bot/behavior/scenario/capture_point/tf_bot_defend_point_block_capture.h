//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_defend_point_block_capture.h
// Move to and defend current point from capture
// Michael Booth, February 2009

#ifndef TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
#define TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H


class CTFBotDefendPointBlockCapture : public Action< CTFBot >
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

	virtual QueryResultType			ShouldHurry( const INextBot *me ) const;							// are we in a hurry?
	virtual QueryResultType			ShouldRetreat( const INextBot *me ) const;

	virtual const char *GetName( void ) const	{ return "BlockCapture"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CTeamControlPoint *m_point;
	CTFNavArea *m_defenseArea;

	bool IsPointSafe( CTFBot *me );
};


#endif // TF_BOT_DEFEND_POINT_BLOCK_CAPTURE_H
