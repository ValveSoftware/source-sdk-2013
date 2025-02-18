//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_TACTICAL_MONITOR_H
#define HL2MP_BOT_TACTICAL_MONITOR_H

class CObjectTeleporter;

class CHL2MPBotTacticalMonitor : public Action< CHL2MPBot >
{
public:
	virtual Action< CHL2MPBot > *InitialContainedAction( CHL2MPBot *me );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnNavAreaChanged( CHL2MPBot *me, CNavArea *newArea, CNavArea *oldArea );
	virtual EventDesiredResult< CHL2MPBot > OnOtherKilled( CHL2MPBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	// @note Tom Bui: Currently used for the training stuff, but once we get that interface down, we will turn that
	// into a proper API
	virtual EventDesiredResult< CHL2MPBot > OnCommandString( CHL2MPBot *me, const char *command );

	virtual const char *GetName( void ) const	{ return "TacticalMonitor"; }

private:
	CountdownTimer m_maintainTimer;

	CountdownTimer m_acknowledgeAttentionTimer;
	CountdownTimer m_acknowledgeRetryTimer;
	CountdownTimer m_attentionTimer;

	CountdownTimer m_stickyBombCheckTimer;
	void MonitorArmedStickyBombs( CHL2MPBot *me );

	void AvoidBumpingEnemies( CHL2MPBot *me );
};



#endif // HL2MP_BOT_TACTICAL_MONITOR_H
