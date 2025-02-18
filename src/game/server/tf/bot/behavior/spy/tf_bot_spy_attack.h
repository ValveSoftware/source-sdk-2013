//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_attack.h
// Backstab or pistol, as appropriate
// Michael Booth, June 2010

#ifndef TF_BOT_SPY_ATTACK_H
#define TF_BOT_SPY_ATTACK_H

#include "Path/NextBotChasePath.h"


//-------------------------------------------------------------------------------
class CTFBotSpyAttack : public Action< CTFBot >
{
public:
	CTFBotSpyAttack( CTFPlayer *victim );
	virtual ~CTFBotSpyAttack() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnInjured( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;							// are we in a hurry?
	virtual QueryResultType ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *blocker ) const;		// use this to signal the enemy we are focusing on, so we dont avoid them

	virtual const CKnownEntity *	SelectMoreDangerousThreat( const INextBot *me, 
															   const CBaseCombatCharacter *subject,
															   const CKnownEntity *threat1, 
															   const CKnownEntity *threat2 ) const;	// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion

	virtual const char *GetName( void ) const	{ return "SpyAttack"; };

private:
	CHandle< CTFPlayer > m_victim;
	ChasePath m_path;
	bool m_isCoverBlown;
	CountdownTimer m_chuckleTimer;
	CountdownTimer m_decloakTimer;
};


#endif // TF_BOT_SPY_ATTACK_H
