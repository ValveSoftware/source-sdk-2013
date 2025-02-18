//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_BEHAVIOR_H
#define HL2MP_BOT_BEHAVIOR_H

#include "Path/NextBotPathFollow.h"

class CHL2MPBotMainAction : public Action< CHL2MPBot >
{
public:
	virtual Action< CHL2MPBot > *InitialContainedAction( CHL2MPBot *me );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnKilled( CHL2MPBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CHL2MPBot > OnInjured( CHL2MPBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CHL2MPBot > OnContact( CHL2MPBot *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );

	virtual EventDesiredResult< CHL2MPBot > OnOtherKilled( CHL2MPBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

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

	bool m_isWaitingForFullReload;

	void FireWeaponAtEnemy( CHL2MPBot *me );

	CHandle< CBaseEntity > m_lastTouch;
	float m_lastTouchTime;

	bool IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const;
	const CKnownEntity *SelectCloserThreat( CHL2MPBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const;
	const CKnownEntity *GetHealerOfThreat( const CKnownEntity *threat ) const;

	const CKnownEntity *SelectMoreDangerousThreatInternal( const INextBot *me, 
														   const CBaseCombatCharacter *subject,
														   const CKnownEntity *threat1, 
														   const CKnownEntity *threat2 ) const;


	void Dodge( CHL2MPBot *me );

	IntervalTimer m_undergroundTimer;

	CountdownTimer m_reevaluateClassTimer;
};



#endif // HL2MP_BOT_BEHAVIOR_H
