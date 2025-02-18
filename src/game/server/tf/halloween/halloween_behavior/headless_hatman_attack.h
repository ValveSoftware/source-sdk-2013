//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_attack.h
// Halloween Boss chase and attack behavior
// Michael Booth, October 2010

#ifndef HEADLESS_HATMAN_ATTACK_H
#define HEADLESS_HATMAN_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CHeadlessHatmanAttack : public Action< CHeadlessHatman >
{
public:
	virtual ActionResult< CHeadlessHatman >	OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction );
	virtual ActionResult< CHeadlessHatman >	Update( CHeadlessHatman *me, float interval );

	virtual EventDesiredResult< CHeadlessHatman > OnStuck( CHeadlessHatman *me );
	virtual EventDesiredResult< CHeadlessHatman > OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

private:
	PathFollower m_path;

	Vector m_homePos;
	CountdownTimer m_homePosRecalcTimer;
	void RecomputeHomePosition( void );

	CountdownTimer m_axeSwingTimer;
	CountdownTimer m_attackTimer;
	CountdownTimer m_laughTimer;
	CountdownTimer m_scareTimer;
	CountdownTimer m_warnITTimer;
	CountdownTimer m_footfallTimer;

	void AttackTarget( CHeadlessHatman *me, CBaseCombatCharacter *pTarget, float flAttackRange );
	void UpdateAxeSwing( CHeadlessHatman *me );
	bool IsSwingingAxe( void ) const;

	CHandle< CTFPlayer > m_closestVisible;

	CHandle< CTFPlayer > m_lastIT;
	CountdownTimer m_chaseVictimTimer;
	void ValidateChaseVictim( CHeadlessHatman *me );
	bool IsPotentiallyChaseable( CHeadlessHatman *me, CTFPlayer *victim );

	CHandle< CBaseCombatCharacter > m_attackTarget;	// the victim I'm momentarily attacking
	CountdownTimer m_attackTargetFocusTimer;
	void SelectVictim( CHeadlessHatman *me );
};



#endif // HEADLESS_HATMAN_ATTACK_H
