//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_SCENARIO_MONITOR_H
#define HL2MP_BOT_SCENARIO_MONITOR_H

class CHL2MPBotScenarioMonitor : public Action< CHL2MPBot >
{
public:
	virtual Action< CHL2MPBot > *InitialContainedAction( CHL2MPBot *me );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "ScenarioMonitor"; }

private:
	CountdownTimer m_ignoreLostFlagTimer;
	CountdownTimer m_lostFlagTimer;

	virtual Action< CHL2MPBot > *DesiredScenarioAndClassAction( CHL2MPBot *me );
};


#endif // HL2MP_BOT_SCENARIO_MONITOR_H
