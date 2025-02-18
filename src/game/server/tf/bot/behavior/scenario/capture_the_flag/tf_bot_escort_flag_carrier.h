//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort_flag_carrier.h
// Escort the flag carrier to their destination
// Michael Booth, May 2011

#ifndef TF_BOT_ESCORT_FLAG_CARRIER_H
#define TF_BOT_ESCORT_FLAG_CARRIER_H


#include "Path/NextBotPathFollow.h"
#include "bot/behavior/tf_bot_melee_attack.h"


//-----------------------------------------------------------------------------
class CTFBotEscortFlagCarrier : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "EscortFlagCarrier"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;

	CTFBotMeleeAttack m_meleeAttackAction;
};


#endif // TF_BOT_ESCORT_FLAG_CARRIER_H
