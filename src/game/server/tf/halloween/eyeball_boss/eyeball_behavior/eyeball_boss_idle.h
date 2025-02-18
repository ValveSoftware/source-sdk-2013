//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_idle.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_IDLE_H
#define EYEBALL_BOSS_IDLE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossIdle : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss > OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss > Update( CEyeballBoss *me, float interval );

	virtual ActionResult< CEyeballBoss > OnResume( CEyeballBoss *me, Action< CEyeballBoss > *interruptingAction );

	virtual EventDesiredResult< CEyeballBoss > OnInjured( CEyeballBoss *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CEyeballBoss > OnOtherKilled( CEyeballBoss *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Idle"; }		// return name of this action

private:
	CountdownTimer m_lookAroundTimer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CHandle< CBaseEntity > m_attacker;
	CountdownTimer m_talkTimer;
	CountdownTimer m_lifeTimer;
	CountdownTimer m_moveTimer;
	float m_lastWarnTime;
	int m_lastHealth;
	bool m_isLaughReady;
};

#endif // EYEBALL_BOSS_IDLE_H
