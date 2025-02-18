//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_RETREAT_TO_COVER_H
#define HL2MP_BOT_RETREAT_TO_COVER_H

class CHL2MPBotRetreatToCover : public Action< CHL2MPBot >
{
public:
	CHL2MPBotRetreatToCover( float hideDuration = -1.0f );
	CHL2MPBotRetreatToCover( Action< CHL2MPBot > *actionToChangeToOnceCoverReached );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot *me, const Path *path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "RetreatToCover"; };

private:
	float m_hideDuration;
	Action< CHL2MPBot > *m_actionToChangeToOnceCoverReached;

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	CNavArea *m_coverArea;
	CountdownTimer m_waitInCoverTimer;

	CNavArea *FindCoverArea( CHL2MPBot *me );
};



#endif // HL2MP_BOT_RETREAT_TO_COVER_H
