//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_STAFF_ATTACK_H
#define MERASMUS_STAFF_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusStaffAttack : public Action< CMerasmus >
{
public:
	CMerasmusStaffAttack( CTFPlayer* pTarget );
	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval );

	virtual const char *GetName( void ) const	{ return "Staff Attack"; }		// return name of this action

private:
	CountdownTimer m_staffSwingTimer;
	CountdownTimer m_hitTimer;
	CHandle< CTFPlayer > m_hTarget;

	PathFollower m_path;
};

#endif // MERASMUS_STAFF_ATTACK_H
