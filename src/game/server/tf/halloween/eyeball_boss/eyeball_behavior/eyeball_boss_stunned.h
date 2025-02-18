//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_stunned.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_STUNNED_H
#define EYEBALL_BOSS_STUNNED_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossStunned : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );
	virtual void							OnEnd( CEyeballBoss *me, Action< CEyeballBoss > *nextAction );

	virtual EventDesiredResult< CEyeballBoss > OnInjured( CEyeballBoss *me, const CTakeDamageInfo &info )
	{ 
		// don't get stunned while stunned
		return TryToSustain( RESULT_CRITICAL );
	}

	virtual const char *GetName( void ) const	{ return "Stunned"; }		// return name of this action

private:
	CountdownTimer m_stunTimer;
	float m_spinRate;
};

#endif // EYEBALL_BOSS_STUNNED_H
