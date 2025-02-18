//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_locomotion.h
// Team Fortress NextBot locomotion interface
// Michael Booth, May 2010

#ifndef TF_BOT_LOCOMOTION_H
#define TF_BOT_LOCOMOTION_H

#include "NextBot/Player/NextBotPlayerLocomotion.h"

//----------------------------------------------------------------------------
class CTFBotLocomotion : public PlayerLocomotion
{
public:
	DECLARE_CLASS( CTFBotLocomotion, PlayerLocomotion );

	CTFBotLocomotion( INextBot *bot ) : PlayerLocomotion( bot )
	{
	}

	virtual ~CTFBotLocomotion() { }

	virtual void Update( void );								// (EXTEND) update internal state

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f );	// move directly towards the given position

	virtual float GetMaxJumpHeight( void ) const;				// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const;				// get maximum running speed

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const;	// return true if given area can be used for navigation
	virtual bool IsEntityTraversable( CBaseEntity *obstacle, TraverseWhenType when = EVENTUALLY ) const;

	//
	// ILocomotion modifiers
	//
	virtual void Jump( void ) OVERRIDE;								// initiate a simple undirected jump in the air

protected:
	virtual void AdjustPosture( const Vector &moveGoal ) { }	// never crouch to navigate
};

inline float CTFBotLocomotion::GetMaxJumpHeight( void ) const
{
	// http://developer.valvesoftware.com/wiki/TF2/Team_Fortress_2_Mapper%27s_Reference
	return 72.0f;
}

#endif // TF_BOT_LOCOMOTION_H
