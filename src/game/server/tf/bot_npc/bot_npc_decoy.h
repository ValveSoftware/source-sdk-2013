//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_decoy.h
// A NextBot non-player decoy that imitates a real player
// Michael Booth, January 2011

#ifndef BOT_NPC_DECOY_H
#define BOT_NPC_DECOY_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc.h"
#include "bot_npc_body.h"


class CTFPlayer;


//----------------------------------------------------------------------------
class CBotNPCDecoyLocomotion : public NextBotGroundLocomotion
{
public:
	DECLARE_CLASS( CBotNPCDecoyLocomotion, NextBotGroundLocomotion );

	CBotNPCDecoyLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CBotNPCDecoyLocomotion() { }

	virtual float GetRunSpeed( void ) const;			// get maximum running speed

	virtual float GetMaxAcceleration( void ) const;		// return maximum acceleration of locomotor
	virtual float GetMaxDeceleration( void ) const;		// return maximum deceleration of locomotor
};


//----------------------------------------------------------------------------
class CBotNPCDecoy : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CBotNPCDecoy, NextBotCombatCharacter );

	CBotNPCDecoy();
	virtual ~CBotNPCDecoy();

	virtual void Precache();
	virtual void Spawn( void );

	// INextBot
	DECLARE_INTENTION_INTERFACE( CBotNPCDecoy );
	virtual CBotNPCDecoyLocomotion *GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CBotNPCBody *GetBodyInterface( void ) const						{ return m_body; }

	virtual Vector EyePosition( void );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	Activity GetRunActivity( void ) const;

private:
	CBotNPCDecoyLocomotion *m_locomotor;
	CBotNPCBody *m_body;

	Vector m_eyeOffset;
	Activity m_runActivity;
};


inline Activity CBotNPCDecoy::GetRunActivity( void ) const
{
	return m_runActivity;
}


inline Vector CBotNPCDecoy::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}

#endif // BOT_NPC_DECOY_H
