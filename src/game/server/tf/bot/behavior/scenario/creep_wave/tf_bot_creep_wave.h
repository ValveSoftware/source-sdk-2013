//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_creep_wave.h
// Move in a "creep wave" to the next available control point to capture it
// Michael Booth, August 2010

#ifndef TF_BOT_CREEP_WAVE_H
#define TF_BOT_CREEP_WAVE_H

#ifdef TF_CREEP_MODE

#include "Path/NextBotPathFollow.h"
#include "Path/NextBotChasePath.h"


CTFBot *FindNearestEnemyCreep( CTFBot *me );


//-----------------------------------------------------------------------------
class CTFBotCreepWave : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnKilled( CTFBot *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnUnStuck( CTFBot *me );

	virtual const char *GetName( void ) const	{ return "CreepWave"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	IntervalTimer m_stuckTimer;
};



//-----------------------------------------------------------------------------
class CTFBotCreepAttack : public Action< CTFBot >
{
public:
	CTFBotCreepAttack( CTFPlayer *victim );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "CreepAttack"; };

private:
	CHandle< CTFPlayer > m_victim;
};


#endif // TF_CREEP_MODE

#endif // TF_BOT_CREEP_WAVE_H
