//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef ZOMBIE_ATTACK_H
#define ZOMBIE_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CZombieAttack : public Action< CZombie >
{
public:
	virtual ActionResult< CZombie >	OnStart( CZombie *me, Action< CZombie > *priorAction );
	virtual ActionResult< CZombie >	Update( CZombie *me, float interval );

	virtual EventDesiredResult< CZombie > OnStuck( CZombie *me );
	virtual EventDesiredResult< CZombie > OnContact( CZombie *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CZombie > OnOtherKilled( CZombie *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

private:
	PathFollower m_path;

	CHandle< CBaseCombatCharacter > m_attackTarget;
	CountdownTimer m_attackTimer;
	CountdownTimer m_specialAttackTimer;
	CountdownTimer m_attackTargetFocusTimer;
	CountdownTimer m_tauntTimer;

	bool IsPotentiallyChaseable( CZombie *me, CBaseCombatCharacter *victim );
	void SelectVictim( CZombie *me );
};

#endif // ZOMBIE_ATTACK_H
