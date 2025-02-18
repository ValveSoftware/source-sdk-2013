// NextBotPlayerLocomotion.h
// Locomotor for CBasePlayer derived bots
// Author: Michael Booth, November 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_PLAYER_LOCOMOTION_H_
#define _NEXT_BOT_PLAYER_LOCOMOTION_H_

#include "NextBot.h"
#include "NextBotLocomotionInterface.h"
#include "Path/NextBotPathFollow.h"

class CBasePlayer;

//--------------------------------------------------------------------------------------------------
/**
 * Basic player locomotion implementation
 */
class PlayerLocomotion : public ILocomotion
{
public:
	DECLARE_CLASS( PlayerLocomotion, ILocomotion );

	PlayerLocomotion( INextBot *bot );
	virtual ~PlayerLocomotion() { }

	virtual void Reset( void );							// reset to initial state
	virtual void Update( void );						// update internal state

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f );	// move directly towards the given position
	virtual void DriveTo( const Vector &pos );			// Move the bot to the precise given position immediately, 

	//
	// ILocomotion modifiers
	//
	virtual bool ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle );	// initiate a jump to an adjacent high ledge, return false if climb can't start
	virtual void JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward );	// initiate a jump across an empty volume of space to far side
	virtual void Jump( void );								// initiate a simple undirected jump in the air
	virtual bool IsClimbingOrJumping( void ) const;					// is jumping in any form
	virtual bool IsClimbingUpToLedge( void ) const;			// is climbing up to a high ledge
	virtual bool IsJumpingAcrossGap( void ) const;			// is jumping across a gap to the far side

	virtual void Run( void );								// set desired movement speed to running
	virtual void Walk( void );								// set desired movement speed to walking
	virtual void Stop( void );								// set desired movement speed to stopped
	virtual bool IsRunning( void ) const;
	virtual void SetDesiredSpeed( float speed );			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed
	virtual void SetMinimumSpeedLimit( float limit );		// speed cannot drop below this 
	virtual void SetMaximumSpeedLimit( float limit );		// speed cannot rise above this 

	virtual bool IsOnGround( void ) const;					// return true if standing on something
	virtual CBaseEntity *GetGround( void ) const;			// return the current ground entity or NULL if not on the ground
	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with

	virtual void ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal );		// climb the given ladder to the top and dismount
	virtual void DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal );		// descend the given ladder to the bottom and dismount
	virtual bool IsUsingLadder( void ) const;
	virtual bool IsAscendingOrDescendingLadder( void ) const;	// we are actually on the ladder right now, either climbing up or down
	virtual bool IsAbleToAutoCenterOnLadder( void ) const;

	virtual void FaceTowards( const Vector &target );		// rotate body to face towards "target"

	virtual void SetDesiredLean( const QAngle &lean )	{ }
	virtual const QAngle &GetDesiredLean( void ) const	{ static QAngle junk; return junk; }

	//
	// ILocomotion information
	//
	virtual const Vector &GetFeet( void ) const;			// return position of "feet" - point below centroid of bot at feet level

	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const;				// get maximum running speed
	virtual float GetWalkSpeed( void ) const;				// get maximum walking speed

	virtual float GetMaxAcceleration( void ) const;			// return maximum acceleration of locomotor
	virtual float GetMaxDeceleration( void ) const;			// return maximum deceleration of locomotor

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity

protected:
	virtual void AdjustPosture( const Vector &moveGoal );

private:
	CBasePlayer *m_player;									// the player we are locomoting	

	mutable bool m_isJumping;
	CountdownTimer m_jumpTimer;

	bool m_isClimbingUpToLedge;
	bool m_isJumpingAcrossGap;
	Vector m_landingGoal;
	bool m_hasLeftTheGround;

	float m_desiredSpeed;
	float m_minSpeedLimit;
	float m_maxSpeedLimit;

	bool TraverseLadder( void );			// when climbing/descending a ladder

	enum LadderState
	{
		NO_LADDER,							// not using a ladder
		APPROACHING_ASCENDING_LADDER,
		APPROACHING_DESCENDING_LADDER,
		ASCENDING_LADDER,
		DESCENDING_LADDER,
		DISMOUNTING_LADDER_TOP,
		DISMOUNTING_LADDER_BOTTOM,
	};

	LadderState m_ladderState;
	LadderState ApproachAscendingLadder( void );
	LadderState ApproachDescendingLadder( void );
	LadderState AscendLadder( void );
	LadderState DescendLadder( void );
	LadderState DismountLadderTop( void );
	LadderState DismountLadderBottom( void );
	
	const CNavLadder *m_ladderInfo;
	const CNavArea *m_ladderDismountGoal;
	CountdownTimer m_ladderTimer;			// a "give up" timer if things go awry

	bool IsClimbPossible( INextBot *me, const CBaseEntity *obstacle ) const;
};


inline float PlayerLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


inline float PlayerLocomotion::GetMaxJumpHeight( void ) const
{
	return 57.0f;
}


inline float PlayerLocomotion::GetDeathDropHeight( void ) const
{
	return 200.0f;
}


inline float PlayerLocomotion::GetMaxAcceleration( void ) const
{
	return 100.0f;
}

inline float PlayerLocomotion::GetMaxDeceleration( void ) const
{
	return 200.0f;
}

inline void PlayerLocomotion::Run( void )
{
	m_desiredSpeed = GetRunSpeed();
}

inline void PlayerLocomotion::Walk( void )
{
	m_desiredSpeed = GetWalkSpeed();
}

inline void PlayerLocomotion::Stop( void )
{
	m_desiredSpeed = 0.0f;
}

inline bool PlayerLocomotion::IsRunning( void ) const
{
	return true;
}

inline void PlayerLocomotion::SetDesiredSpeed( float speed )
{
	m_desiredSpeed = speed;
}

inline float PlayerLocomotion::GetDesiredSpeed( void ) const
{
	return clamp( m_desiredSpeed, m_minSpeedLimit, m_maxSpeedLimit );
}

inline void PlayerLocomotion::SetMinimumSpeedLimit( float limit )
{
	m_minSpeedLimit = limit;
}

inline void PlayerLocomotion::SetMaximumSpeedLimit( float limit )
{
	m_maxSpeedLimit = limit;
}

inline bool PlayerLocomotion::IsAbleToAutoCenterOnLadder( void ) const
{
	return IsUsingLadder() && (m_ladderState == ASCENDING_LADDER || m_ladderState == DESCENDING_LADDER);
}

inline bool PlayerLocomotion::IsAscendingOrDescendingLadder( void ) const
{
	switch( m_ladderState )
	{
	case ASCENDING_LADDER:
	case DESCENDING_LADDER:
	case DISMOUNTING_LADDER_TOP:
	case DISMOUNTING_LADDER_BOTTOM:
		return true;
	default:
		// Explicitly handle the default so that clang knows not to warn us.
		// warning: enumeration values 'NO_LADDER', 'APPROACHING_ASCENDING_LADDER', and 'APPROACHING_DESCENDING_LADDER' not handled in switch [-Wswitch-enum]
		break;
	}

	return false;
}


#endif // _NEXT_BOT_PLAYER_LOCOMOTION_H_
