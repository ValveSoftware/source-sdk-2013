//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_attack.h
// Attack a threat
// Michael Booth, February 2009

#ifndef TF_BOT_ATTACK_H
#define TF_BOT_ATTACK_H

#include "Path/NextBotChasePath.h"


//-------------------------------------------------------------------------------
class CTFBotAttack : public Action< CTFBot >
{
public:
	CTFBotAttack( void );
	virtual ~CTFBotAttack() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "Attack"; };

private:
	PathFollower m_path;
	ChasePath m_chasePath;
	CountdownTimer m_repathTimer;
};


#endif // TF_BOT_ATTACK_H
