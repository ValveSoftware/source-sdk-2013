//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_hide.h
// Move to a hiding spot
// Michael Booth, September 2011

#ifndef TF_BOT_SPY_HIDE
#define TF_BOT_SPY_HIDE

#include "Path/NextBotPathFollow.h"

class CTFBotSpyHide : public Action< CTFBot >
{
public:
	CTFBotSpyHide( CTFPlayer *victim = NULL );
	virtual ~CTFBotSpyHide() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "SpyHide"; };

private:
	CHandle< CTFPlayer > m_initialVictim;

	HidingSpot *m_hidingSpot;
	bool FindHidingSpot( CTFBot *me );
	CountdownTimer m_findTimer;

	PathFollower m_path;
	CountdownTimer m_repathTimer;
	bool m_isAtGoal;

	float m_incursionThreshold;

	CountdownTimer m_talkTimer;
};

#endif // TF_BOT_SPY_HIDE
