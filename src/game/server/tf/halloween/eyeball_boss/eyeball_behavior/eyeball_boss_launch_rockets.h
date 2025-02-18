//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_launch_rockets.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_LAUNCH_ROCKETS_H
#define EYEBALL_BOSS_LAUNCH_ROCKETS_H


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossLaunchRockets : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "LaunchRockets"; }		// return name of this action

private:
	CountdownTimer m_initialDelayTimer;

	CountdownTimer m_launchTimer;
	int m_rocketsLeft;

	Vector m_lastTargetPosition;
};


#endif // EYEBALL_BOSS_LAUNCH_ROCKETS_H
