//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_launch_rockets.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_LAUNCH_ROCKETS_H
#define BOSS_ALPHA_LAUNCH_ROCKETS_H

#ifdef TF_RAID_MODE

//---------------------------------------------------------------------------------------------
class CBossAlphaLaunchRockets : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBossAlpha >	OnSuspend( CBossAlpha *me, Action< CBossAlpha > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "LaunchRockets"; }		// return name of this action

private:
	CountdownTimer m_timer;

	CountdownTimer m_launchTimer;
	int m_rocketsLeft;

	int m_animLayer;

	CHandle< CBaseCombatCharacter > m_target;
	Vector m_lastTargetPosition;
};


#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_LAUNCH_ROCKETS_H
