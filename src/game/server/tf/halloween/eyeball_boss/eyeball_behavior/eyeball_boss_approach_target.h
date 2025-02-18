//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_approach_target.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_APPROACH_TARGET_H
#define EYEBALL_BOSS_APPROACH_TARGET_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossApproachTarget : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );
	virtual void OnEnd( CEyeballBoss *me, Action< CEyeballBoss > *nextAction );

	virtual const char *GetName( void ) const	{ return "ApproachTarget"; }		// return name of this action

private:
	CountdownTimer m_lingerTimer;
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_minChaseTimer;
};

#endif // EYEBALL_BOSS_APPROACH_TARGET_H
