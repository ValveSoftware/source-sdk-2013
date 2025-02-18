//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_attack_flag_defenders.h
// Attack enemies that are preventing the flag from reaching its destination
// Michael Booth, May 2011

#ifndef TF_BOT_ATTACK_FLAG_DEFENDERS_H
#define TF_BOT_ATTACK_FLAG_DEFENDERS_H

#include "Path/NextBotPathFollow.h"
#include "bot/behavior/tf_bot_attack.h"


//-----------------------------------------------------------------------------
class CTFBotAttackFlagDefenders : public CTFBotAttack
{
public:
	CTFBotAttackFlagDefenders( float minDuration = -1.0f );
	virtual ~CTFBotAttackFlagDefenders() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "AttackFlagDefenders"; }

private:
	CountdownTimer m_minDurationTimer;
	CountdownTimer m_watchFlagTimer;
	CHandle< CTFPlayer > m_chasePlayer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};


#endif // TF_BOT_ATTACK_FLAG_DEFENDERS_H
