//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_behavior.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_BEHAVIOR_H
#define EYEBALL_BOSS_BEHAVIOR_H


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossBehavior : public Action< CEyeballBoss >
{
public:
	virtual Action< CEyeballBoss > *InitialContainedAction( CEyeballBoss *me );

	virtual ActionResult< CEyeballBoss > OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss > Update( CEyeballBoss *me, float interval );

	virtual EventDesiredResult< CEyeballBoss > OnInjured( CEyeballBoss *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CEyeballBoss > OnKilled( CEyeballBoss *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action

private:
	CountdownTimer m_stunCooldownTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossDead : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss > OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss > Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "Dead"; }		// return name of this action

private:
	CountdownTimer m_giveUpTimer;
};



#endif // EYEBALL_BOSS_BEHAVIOR_H
