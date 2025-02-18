//========= Copyright Valve Corporation, All rights reserved. ============//
// crybaby_boss_attack.h
// Halloween Boss 2011 chase and attack behavior
// Michael Booth, October 2011

#ifndef CRYBABY_BOSS_ATTACK_H
#define CRYBABY_BOSS_ATTACK_H

#include "NextBot/Path/NextBotChasePath.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CCryBabyBossAttack : public Action< CHeadlessHatman >
{
public:
	CCryBabyBossAttack( CTFPlayer *victim );
	virtual ~CCryBabyBossAttack() { }

	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );

	virtual EventDesiredResult< CHeadlessHatman > OnStuck( CHeadlessHatman *me );
	virtual EventDesiredResult< CHeadlessHatman > OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "CryBabyBossAttack"; }		// return name of this action

private:
	ChasePath m_chasePath;
	CHandle< CTFPlayer > m_victim;

	CountdownTimer m_axeSwingTimer;
	CountdownTimer m_attackTimer;
	CountdownTimer m_laughTimer;
	CountdownTimer m_footfallTimer;

	void UpdateAxeSwing( CHeadlessHatman *me );
	bool IsSwingingAxe( void ) const;

	bool IsVictimChaseable( CHeadlessHatman *me );

	CHandle< CBaseCombatCharacter > m_attackTarget;	// the victim I'm momentarily attacking
	CountdownTimer m_attackTargetFocusTimer;
};



#endif // CRYBABY_BOSS_ATTACK_H
