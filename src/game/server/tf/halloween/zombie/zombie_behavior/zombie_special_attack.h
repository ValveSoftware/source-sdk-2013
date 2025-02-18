//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef ZOMBIE_SPECIAL_ATTACK_H
#define ZOMBIE_SPECIAL_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CZombieSpecialAttack : public Action< CZombie >
{
public:
	virtual ActionResult< CZombie >	OnStart( CZombie *me, Action< CZombie > *priorAction );
	virtual ActionResult< CZombie >	Update( CZombie *me, float interval );

	virtual const char *GetName( void ) const	{ return "Special Attack"; }		// return name of this action
private:

	void DoSpecialAttack( CZombie *me );

	CountdownTimer m_stompTimer;
};

#endif // ZOMBIE_SPECIAL_ATTACK_H
