//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_sniper_attack.h
// Attack a threat as a Sniper
// Michael Booth, February 2009

#ifndef TF_BOT_SNIPER_ATTACK_H
#define TF_BOT_SNIPER_ATTACK_H

#include "Path/NextBotChasePath.h"

class CTFBotSniperAttack : public Action< CTFBot >
{
public:
	static bool IsPossible( CTFBot *me );			// return true if this Action has what it needs to perform right now

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	void OnEnd( CTFBot *me, Action< CTFBot > *nextAction );
	virtual ActionResult< CTFBot >	OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual Vector SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const;		// given a subject, return the world space position we should aim at

	virtual const CKnownEntity *	SelectMoreDangerousThreat( const INextBot *me, 
															   const CBaseCombatCharacter *subject,
															   const CKnownEntity *threat1, 
															   const CKnownEntity *threat2 ) const;	// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion

	virtual const char *GetName( void ) const	{ return "SniperAttack"; };

private:
	CountdownTimer m_lingerTimer;

	bool IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const;
};

#endif // TF_BOT_SNIPER_ATTACK_H
