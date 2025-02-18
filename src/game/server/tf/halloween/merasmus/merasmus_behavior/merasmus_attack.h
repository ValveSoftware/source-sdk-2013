//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_ATTACK_H
#define MERASMUS_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusAttack : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );

	virtual EventDesiredResult< CMerasmus > OnStuck( CMerasmus *me );
	virtual EventDesiredResult< CMerasmus > OnContact( CMerasmus *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

private:
	PathFollower m_path;

	Vector m_homePos;
	CountdownTimer m_homePosRecalcTimer;
	void RecomputeHomePosition( void );

	CountdownTimer m_attackTimer;

	CountdownTimer m_grenadeTimer;
	void RandomGrenadeTimer();

	CountdownTimer m_zapTimer;
	void RandomZapTimer();

	CountdownTimer m_bombHeadTimer;
	CountdownTimer m_tauntTimer;

	CHandle< CTFPlayer > m_attackTarget;	// the victim I'm momentarily attacking
	CountdownTimer m_attackTargetFocusTimer;
	bool IsPotentiallyChaseable( CMerasmus *me, CTFPlayer *victim );
	void SelectVictim( CMerasmus *me );
};



//---------------------------------------------------------------------------------------------
class CMerasmusTaunt : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );
	virtual void						OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction );

	virtual const char *GetName( void ) const	{ return "Taunt"; }		// return name of this action

private:
	CountdownTimer m_timer;
};


#endif // MERASMUS_ATTACK_H
