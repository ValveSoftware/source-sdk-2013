//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_leave_spawn_room.h
// Assume the enemy is watching our spawn - escape it
// Michael Booth, September 2011

#ifndef TF_BOT_LEAVE_SPAWN_ROOM_H
#define TF_BOT_LEAVE_SPAWN_ROOM_H

#include "Path/NextBotPathFollow.h"

class CTFBotSpyLeaveSpawnRoom : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "SpyLeaveSpawnRoom"; };

private:
	CountdownTimer m_waitTimer;
	int m_attempt;
};

#endif // TF_BOT_LEAVE_SPAWN_ROOM_H
