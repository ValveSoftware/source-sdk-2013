//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_tactical_monitor.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_TACTICAL_MONITOR_H
#define BOSS_ALPHA_TACTICAL_MONITOR_H

#ifdef TF_RAID_MODE

class CBossAlpha;


//---------------------------------------------------------------------------------------------
class CBossAlphaTacticalMonitor : public Action< CBossAlpha >
{
public:
	virtual Action< CBossAlpha > *InitialContainedAction( CBossAlpha *me );

	virtual ActionResult< CBossAlpha > OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );

	virtual const char *GetName( void ) const	{ return "TacticalMonitor"; }		// return name of this action

private:
	CountdownTimer m_backOffCooldownTimer;
	CountdownTimer m_getOffMeTimer;
};


#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_TACTICAL_MONITOR_H
