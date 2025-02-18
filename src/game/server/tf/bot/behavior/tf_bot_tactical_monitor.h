//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_tactical_monitor.h
// Behavior layer that interrupts for ammo/health/retreat/etc
// Michael Booth, June 2009

#ifndef TF_BOT_TACTICAL_MONITOR_H
#define TF_BOT_TACTICAL_MONITOR_H

class CObjectTeleporter;

class CTFBotTacticalMonitor : public Action< CTFBot >
{
public:
	virtual Action< CTFBot > *InitialContainedAction( CTFBot *me );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnNavAreaChanged( CTFBot *me, CNavArea *newArea, CNavArea *oldArea );
	virtual EventDesiredResult< CTFBot > OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	// @note Tom Bui: Currently used for the training stuff, but once we get that interface down, we will turn that
	// into a proper API
	virtual EventDesiredResult< CTFBot > OnCommandString( CTFBot *me, const char *command );

	virtual const char *GetName( void ) const	{ return "TacticalMonitor"; }

private:
	CountdownTimer m_maintainTimer;

	CountdownTimer m_acknowledgeAttentionTimer;
	CountdownTimer m_acknowledgeRetryTimer;
	CountdownTimer m_attentionTimer;

	CountdownTimer m_stickyBombCheckTimer;
	void MonitorArmedStickyBombs( CTFBot *me );

	bool ShouldOpportunisticallyTeleport( CTFBot *me ) const;
	CObjectTeleporter *FindNearbyTeleporter( CTFBot *me );
	CountdownTimer m_findTeleporterTimer;

	void AvoidBumpingEnemies( CTFBot *me );
};



#endif // TF_BOT_TACTICAL_MONITOR_H
