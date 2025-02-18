//========= Copyright Valve Corporation, All rights reserved. ============//
// ghost.h
// A spooky halloween ghost bot
// Michael Booth, October 2011

#ifndef GHOST_H
#define GHOST_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"

#define GHOST_SPEED 90
#define GHOST_SCARE_RADIUS 192

class CTFPlayer;

//----------------------------------------------------------------------------
class CGhostLocomotion : public NextBotGroundLocomotion
{
public:
	DECLARE_CLASS( CGhostLocomotion, NextBotGroundLocomotion );

	CGhostLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CGhostLocomotion() { }

	virtual float GetRunSpeed( void ) const;			// get maximum running speed

	virtual float GetMaxAcceleration( void ) const;		// return maximum acceleration of locomotor
	virtual float GetMaxDeceleration( void ) const;		// return maximum deceleration of locomotor
};


//----------------------------------------------------------------------------
class CGhost : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CGhost, NextBotCombatCharacter );

	CGhost();
	virtual ~CGhost();

	static void PrecacheGhost();
	virtual void Precache();
	virtual void Spawn( void );

	void SetLifetime( float duration );
	float GetLifetime( void ) const;

	// INextBot
	DECLARE_INTENTION_INTERFACE( CGhost );
	virtual CGhostLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }

	virtual Vector EyePosition( void );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;


private:
	CGhostLocomotion *m_locomotor;
	Vector m_eyeOffset;
	Vector m_homePos;
	float m_lifetime;
};


inline void CGhost::SetLifetime( float duration )
{
	m_lifetime = duration;
}

inline float CGhost::GetLifetime( void ) const
{
	return m_lifetime;
}

inline Vector CGhost::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}


extern CGhost *SpawnGhost( const Vector &spot, const QAngle &angles, float lifetime = 10.0f );

#endif // GHOST_H
