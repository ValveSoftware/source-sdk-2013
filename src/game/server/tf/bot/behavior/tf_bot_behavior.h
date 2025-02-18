//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_behavior.h
// Team Fortress NextBot behaviors
// Michael Booth, February 2009

#ifndef TF_BOT_BEHAVIOR_H
#define TF_BOT_BEHAVIOR_H

#include "Path/NextBotPathFollow.h"

class CTFBotMainAction : public Action< CTFBot >
{
public:
	virtual Action< CTFBot > *InitialContainedAction( CTFBot *me );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnKilled( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnInjured( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );

	virtual EventDesiredResult< CTFBot > OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;
	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType	ShouldHurry( const INextBot *me ) const;							// are we in a hurry?

	virtual Vector SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const;		// given a subject, return the world space position we should aim at
	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector &pos ) const;

	virtual const CKnownEntity *	SelectMoreDangerousThreat( const INextBot *me, 
															   const CBaseCombatCharacter *subject,
															   const CKnownEntity *threat1, 
															   const CKnownEntity *threat2 ) const;	// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion

	virtual const char *GetName( void ) const	{ return "MainAction"; };

private:
	CountdownTimer m_reloadTimer;
	mutable CountdownTimer m_aimAdjustTimer;
	mutable float m_aimErrorRadius;
	mutable float m_aimErrorAngle;

	float m_yawRate;
	float m_priorYaw;
	IntervalTimer m_steadyTimer;

	int m_nextDisguise;

	bool m_isWaitingForFullReload;

	void FireWeaponAtEnemy( CTFBot *me );

	CHandle< CBaseEntity > m_lastTouch;
	float m_lastTouchTime;

	bool IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const;
	const CKnownEntity *SelectCloserThreat( CTFBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const;
	const CKnownEntity *GetHealerOfThreat( const CKnownEntity *threat ) const;

	const CKnownEntity *SelectMoreDangerousThreatInternal( const INextBot *me, 
														   const CBaseCombatCharacter *subject,
														   const CKnownEntity *threat1, 
														   const CKnownEntity *threat2 ) const;


	void Dodge( CTFBot *me );

	IntervalTimer m_undergroundTimer;

	CountdownTimer m_reevaluateClassTimer;
};



#endif // TF_BOT_BEHAVIOR_H
