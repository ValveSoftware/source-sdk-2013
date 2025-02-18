//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_mini.h
// A NextBot non-player derived actor
// Michael Booth, March 2011

#ifndef BOT_NPC_MINI_H
#define BOT_NPC_MINI_H

#ifdef TF_RAID_MODE

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc_body.h"
#include "bot/map_entities/tf_spawner_boss.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"


//----------------------------------------------------------------------------
class CBotNPCMiniRockets : public CBossAlpha
{
public:
	DECLARE_CLASS( CBotNPCMiniRockets, CBossAlpha );

	virtual void Precache();
	virtual void Spawn( void );

	virtual bool HasAbility( Ability ability ) const;

	virtual bool IsMiniBoss( void ) const					{ return true; }

	virtual float GetMoveSpeed( void ) const				{ return 150.0f; }

	virtual int GetRocketLaunchCount( void ) const			{ return 3; }
	virtual float GetRocketDamage( void ) const				{ return 25.0f; }
	virtual float GetRocketAimError( void ) const			{ return 3.0f; }
	virtual float GetRocketInterval( void ) const			{ return 0.5f; }
	virtual const char *GetRocketSoundEffect( void ) const	{ return "RobotMiniBoss.LaunchRocket"; }

	virtual float GetBecomeStunnedDamage( void ) const		{ return 300.0f; }
};

inline bool CBotNPCMiniRockets::HasAbility( Ability ability ) const
{
	const int myAbilities = CAN_BE_STUNNED | CAN_FIRE_ROCKETS;

	return myAbilities & ability ? true : false;
}


//----------------------------------------------------------------------------
class CBotNPCMiniNuker : public CBossAlpha
{
public:
	DECLARE_CLASS( CBotNPCMiniNuker, CBossAlpha );

	virtual void Precache();
	virtual void Spawn( void );

	virtual bool HasAbility( Ability ability ) const;

	virtual bool IsMiniBoss( void ) const					{ return true; }

	virtual float GetMoveSpeed( void ) const				{ return 150.0f; }

	virtual float GetGrenadeInterval( void ) const			{ return 2.0f; }

	virtual float GetBecomeStunnedDamage( void ) const		{ return 300.0f; }
};

inline bool CBotNPCMiniNuker::HasAbility( Ability ability ) const
{
	const int myAbilities = CAN_BE_STUNNED | CAN_NUKE | CAN_LAUNCH_STICKIES;

	return myAbilities & ability ? true : false;
}



#endif // TF_RAID_MODE

#endif // BOT_NPC_MINI_H
