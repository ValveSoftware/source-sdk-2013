//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_ROCKET_H
#define MERASMUS_ROCKET_H

class CMerasmusThrowingGrenade : public Action< CMerasmus >
{
public:
	CMerasmusThrowingGrenade( CTFPlayer* pTarget );

	virtual ActionResult< CMerasmus > OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus > Update( CMerasmus *me, float interval );
	virtual void					  OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction );

	virtual const char *GetName( void ) const	{ return "Rocket"; }		// return name of this action
private:
	CHandle< CTFPlayer > m_hTarget;
	CountdownTimer m_throwTimer;
	CountdownTimer m_releaseGrenadeTimer;

	PathFollower m_path;
};

#endif // MERASMUS_ROCKET_H
