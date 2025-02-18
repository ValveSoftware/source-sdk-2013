//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_LOCOMOTION_H
#define HL2MP_BOT_LOCOMOTION_H

#include "NextBot/Player/NextBotPlayerLocomotion.h"

//----------------------------------------------------------------------------
class CHL2MPBotLocomotion : public PlayerLocomotion
{
public:
	DECLARE_CLASS( CHL2MPBotLocomotion, PlayerLocomotion );

	CHL2MPBotLocomotion( INextBot *bot ) : PlayerLocomotion( bot )
	{
	}

	virtual ~CHL2MPBotLocomotion() { }

	virtual void Update( void );								// (EXTEND) update internal state

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f );	// move directly towards the given position

	virtual float GetMaxJumpHeight( void ) const;				// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const;				// get maximum running speed

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const;	// return true if given area can be used for navigation
	virtual bool IsEntityTraversable( CBaseEntity *obstacle, TraverseWhenType when = EVENTUALLY ) const;

protected:
	virtual void AdjustPosture( const Vector &moveGoal ) { }	// never crouch to navigate
};

inline float CHL2MPBotLocomotion::GetMaxJumpHeight( void ) const
{
	// https://developer.valvesoftware.com/wiki/Dimensions_(Half-Life_2_and_Counter-Strike:_Source)#Jumping
	return 56.0f;
}

#endif // HL2MP_BOT_LOCOMOTION_H
