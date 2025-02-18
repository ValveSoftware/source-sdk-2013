//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_destroy_enemy_sentry.h
// Destroy an enemy sentry gun
// Michael Booth, June 2010

#ifndef TF_BOT_DESTROY_ENEMY_SENTRY_H
#define TF_BOT_DESTROY_ENEMY_SENTRY_H

#include "Path/NextBotChasePath.h"

//---------------------------------------------------------------------------------
class CTFBotDestroyEnemySentry : public Action< CTFBot >
{
public:
	static bool IsPossible( CTFBot *me );			// return true if this Action has what it needs to perform right now

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType	ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "DestroyEnemySentry"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CountdownTimer m_abandonTimer;

	bool m_canMove;

#ifdef TF_CREEP_MODE
	CountdownTimer m_creepTimer;
#endif

	Vector m_safeAttackSpot;
	bool m_hasSafeAttackSpot;
	void ComputeSafeAttackSpot( CTFBot *me );
	void ComputeCornerAttackSpot( CTFBot *me );

	bool m_isAttackingSentry;
	bool m_wasUber;

	ActionResult< CTFBot > EquipLongRangeWeapon( CTFBot *me );

	CHandle< CObjectSentrygun > m_targetSentry;
};


//---------------------------------------------------------------------------------
class CTFBotUberAttackEnemySentry : public Action< CTFBot >
{
public:
	CTFBotUberAttackEnemySentry( CObjectSentrygun *sentryTarget );
	virtual ~CTFBotUberAttackEnemySentry() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType	ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "UberAttackEnemySentry"; };

private:
	bool m_wasIgnoringEnemies;

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	CHandle< CObjectSentrygun > m_targetSentry;
};


#endif // TF_BOT_DESTROY_ENEMY_SENTRY_H
