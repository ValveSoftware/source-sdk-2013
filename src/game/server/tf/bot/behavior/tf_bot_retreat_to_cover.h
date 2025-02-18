//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_retreat_to_cover.h
// Retreat to local cover from known threats
// Michael Booth, June 2009

#ifndef TF_BOT_RETREAT_TO_COVER_H
#define TF_BOT_RETREAT_TO_COVER_H

class CTFBotRetreatToCover : public Action< CTFBot >
{
public:
	CTFBotRetreatToCover( float hideDuration = -1.0f );
	CTFBotRetreatToCover( Action< CTFBot > *actionToChangeToOnceCoverReached );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "RetreatToCover"; };

private:
	float m_hideDuration;
	Action< CTFBot > *m_actionToChangeToOnceCoverReached;

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	CTFNavArea *m_coverArea;
	CountdownTimer m_waitInCoverTimer;

	CTFNavArea *FindCoverArea( CTFBot *me );
};



#endif // TF_BOT_RETREAT_TO_COVER_H
