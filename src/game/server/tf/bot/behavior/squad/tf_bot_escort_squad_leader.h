//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort_squad_leader.h
// Escort the squad leader to their destination
// Michael Booth, Octoboer 2011

#ifndef TF_BOT_ESCORT_SQUAD_LEADER_H
#define TF_BOT_ESCORT_SQUAD_LEADER_H


#include "Path/NextBotPathFollow.h"
#include "bot/behavior/tf_bot_melee_attack.h"


//-----------------------------------------------------------------------------
class CTFBotEscortSquadLeader : public Action< CTFBot >
{
public:
	CTFBotEscortSquadLeader( Action< CTFBot > *actionToDoAfterSquadDisbands = NULL );
	virtual ~CTFBotEscortSquadLeader() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "EscortSquadLeader"; };

private:
	Action< CTFBot > *m_actionToDoAfterSquadDisbands;
	CTFBotMeleeAttack m_meleeAttackAction;

	PathFollower m_formationPath;
	CountdownTimer m_pathTimer;

	const Vector &GetFormationForwardVector( CTFBot *me );
	Vector m_formationForward;
};


//-----------------------------------------------------------------------------
class CTFBotWaitForOutOfPositionSquadMember : public Action< CTFBot >
{
public:
	virtual ~CTFBotWaitForOutOfPositionSquadMember() { }

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "WaitForOutOfPositionSquadMember"; };

private:
	CountdownTimer m_waitTimer;
};


#endif // TF_BOT_ESCORT_SQUAD_LEADER_H
