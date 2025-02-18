//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_archer.h
// A NextBot non-player derived archer
// Michael Booth, November 2010

#ifndef BOT_NPC_ARCHER_H
#define BOT_NPC_ARCHER_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc.h"
#include "bot_npc_body.h"


class CTFPlayer;


//----------------------------------------------------------------------------
class CBotNPCArcher : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CBotNPCArcher, NextBotCombatCharacter );

	CBotNPCArcher();
	virtual ~CBotNPCArcher();

	virtual void Precache();
	virtual void Spawn( void );

	// INextBot
	DECLARE_INTENTION_INTERFACE( CBotNPCArcher );
	virtual NextBotGroundLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CBotNPCBody *GetBodyInterface( void ) const						{ return m_body; }

	virtual Vector EyePosition( void );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	CBaseAnimating *GetBow( void ) const;

	void SetHomePosition( const Vector &pos );
	const Vector &GetHomePosition( void ) const;

private:
	NextBotGroundLocomotion *m_locomotor;
	CBotNPCBody *m_body;

	CBaseAnimating *m_bow;
	Vector m_eyeOffset;

	Vector m_homePos;
};


inline void CBotNPCArcher::SetHomePosition( const Vector &pos )
{
	m_homePos = pos;
}

inline const Vector &CBotNPCArcher::GetHomePosition( void ) const
{
	return m_homePos;
}

inline Vector CBotNPCArcher::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}


inline CBaseAnimating *CBotNPCArcher::GetBow( void ) const
{
	return m_bow;
}

#endif // BOT_NPC_ARCHER_H
