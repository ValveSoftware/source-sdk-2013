//========= Copyright Valve Corporation, All rights reserved. ============//
// NextBotGroundLocomotion.h
// Basic ground-based movement for NextBotCombatCharacters
// Author: Michael Booth, February 2009
// Note: This is a refactoring of ZombieBotLocomotion from L4D

#ifndef NEXT_BOT_GROUND_LOCOMOTION_H
#define NEXT_BOT_GROUND_LOCOMOTION_H

#include "NextBotLocomotionInterface.h"
#include "nav_mesh.h"


class NextBotCombatCharacter;

//----------------------------------------------------------------------------------------------------------------
/**
 * Basic ground-based movement for NextBotCombatCharacters.
 * This locomotor resolves collisions and assumes a ground-based bot under the influence of gravity.
 */
class NextBotGroundLocomotion : public ILocomotion
{
public:
	DECLARE_CLASS( NextBotGroundLocomotion, ILocomotion );

	NextBotGroundLocomotion( INextBot *bot );
	virtual ~NextBotGroundLocomotion();
	
	virtual void Reset( void );							// reset locomotor to initial state
	virtual void Update( void );						// update internal state

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f );	// move directly towards the given position
	virtual void DriveTo( const Vector &pos );			// Move the bot to the precise given position immediately, 

	virtual bool ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle );	// initiate a jump to an adjacent high ledge, return false if climb can't start
	virtual void JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward );	// initiate a jump across an empty volume of space to far side
	virtual void Jump( void );							// initiate a simple undirected jump in the air
	virtual bool IsClimbingOrJumping( void ) const;		// is jumping in any form
	virtual bool IsClimbingUpToLedge( void ) const;		// is climbing up to a high ledge
	virtual bool IsJumpingAcrossGap( void ) const;		// is jumping across a gap to the far side

	virtual void Run( void );							// set desired movement speed to running
	virtual void Walk( void );							// set desired movement speed to walking
	virtual void Stop( void );							// set desired movement speed to stopped
	virtual bool IsRunning( void ) const;
	virtual void SetDesiredSpeed( float speed );		// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;		// returns the current desired speed

	virtual float GetSpeedLimit( void ) const;			// get maximum speed bot can reach, regardless of desired speed

	virtual bool IsOnGround( void ) const;				// return true if standing on something
	virtual void OnLeaveGround( CBaseEntity *ground );	// invoked when bot leaves ground for any reason
	virtual void OnLandOnGround( CBaseEntity *ground );	// invoked when bot lands on the ground after being in the air
	virtual CBaseEntity *GetGround( void ) const;		// return the current ground entity or NULL if not on the ground
	virtual const Vector &GetGroundNormal( void ) const;// surface normal of the ground we are in contact with

	virtual void ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal );		// climb the given ladder to the top and dismount
	virtual void DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal );		// descend the given ladder to the bottom and dismount
	virtual bool IsUsingLadder( void ) const;
	virtual bool IsAscendingOrDescendingLadder( void ) const;	// we are actually on the ladder right now, either climbing up or down

	virtual void FaceTowards( const Vector &target );	// rotate body to face towards "target"

	virtual void SetDesiredLean( const QAngle &lean );
	virtual const QAngle &GetDesiredLean( void ) const;

	virtual const Vector &GetFeet( void ) const;		// return position of "feet" - the driving point where the bot contacts the ground

	virtual float GetStepHeight( void ) const;			// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;		// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;		// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const;			// get maximum running speed
	virtual float GetWalkSpeed( void ) const;			// get maximum walking speed

	virtual float GetMaxAcceleration( void ) const;		// return maximum acceleration of locomotor
	virtual float GetMaxDeceleration( void ) const;		// return maximum deceleration of locomotor

	virtual const Vector &GetAcceleration( void ) const;	// return current world space acceleration
	virtual void SetAcceleration( const Vector &accel );	// set world space acceleration

	virtual const Vector &GetVelocity( void ) const;	// return current world space velocity
	virtual void SetVelocity( const Vector &vel );		// set world space velocity

	virtual void OnMoveToSuccess( const Path *path );		// invoked when an bot reaches its MoveTo goal
	virtual void OnMoveToFailure( const Path *path, MoveToFailureType reason );	// invoked when an bot fails to reach a MoveTo goal

private:
	void UpdatePosition( const Vector &newPos );			// move to newPos, resolving any collisions along the way
	void UpdateGroundConstraint( void );					// keep ground solid
	Vector ResolveCollisionV0( Vector from, Vector to, int recursionLimit );

	Vector ResolveZombieCollisions( const Vector &pos );	// push away zombies that are interpenetrating
	Vector ResolveCollision( const Vector &from, const Vector &to, int recursionLimit );	// check for collisions along move
	bool DetectCollision( trace_t *pTrace, int &nDestructionAllowed, const Vector &from, const Vector &to, const Vector &vecMins, const Vector &vecMaxs );
	void ApplyAccumulatedApproach( void );
	bool DidJustJump( void ) const;							// return true if we just started a jump
	bool TraverseLadder( void );							// return true if we are climbing a ladder

	virtual float GetGravity( void ) const;					// return gravity force acting on bot
	virtual float GetFrictionForward( void ) const;			// return magnitude of forward friction
	virtual float GetFrictionSideways( void ) const;		// return magnitude of lateral friction
	virtual float GetMaxYawRate( void ) const;				// return max rate of yaw rotation


private:
	NextBotCombatCharacter *m_nextBot;

	Vector m_priorPos;										// last update's position
	Vector m_lastValidPos;									// last valid position (not interpenetrating)
	
	Vector m_acceleration;
	Vector m_velocity;
	
	float m_desiredSpeed;									// speed bot wants to be moving
	float m_actualSpeed;									// actual speed bot is moving

	float m_maxRunSpeed;

	float m_forwardLean;
	float m_sideLean;
	QAngle m_desiredLean;
	
	bool m_isJumping;										// if true, we have jumped and have not yet hit the ground
	bool m_isJumpingAcrossGap;								// if true, we have jumped across a gap and have not yet hit the ground
	EHANDLE m_ground;										// have to manage this ourselves, since MOVETYPE_CUSTOM always NULLs out GetGroundEntity()
	Vector m_groundNormal;									// surface normal of the ground we are in contact with
	bool m_isClimbingUpToLedge;									// true if we are jumping up to an adjacent ledge
	Vector m_ledgeJumpGoalPos;
	bool m_isUsingFullFeetTrace;							// true if we're in the air and tracing the lowest StepHeight in ResolveCollision

	const CNavLadder *m_ladder;								// ladder we are currently climbing/descending
	const CNavArea *m_ladderDismountGoal;					// the area we enter when finished with our ladder move
	bool m_isGoingUpLadder;									// if false, we're going down

	CountdownTimer m_inhibitObstacleAvoidanceTimer;			// when active, turn off path following feelers

	CountdownTimer m_wiggleTimer;							// for wiggling
	NavRelativeDirType m_wiggleDirection;

	mutable Vector m_eyePos;								// for use with GetEyes(), etc.

	Vector m_moveVector;									// the direction of our motion in XY plane
	float m_moveYaw;										// global yaw of movement direction

	Vector m_accumApproachVectors;							// weighted sum of Approach() calls since last update
	float m_accumApproachWeights;
	bool m_bRecomputePostureOnCollision;

	CountdownTimer m_ignorePhysicsPropTimer;				// if active, don't collide with physics props (because we got stuck in one)
	EHANDLE m_ignorePhysicsProp;							// which prop to ignore
};


inline float NextBotGroundLocomotion::GetGravity( void ) const
{
	return 1000.0f;
}

inline float NextBotGroundLocomotion::GetFrictionForward( void ) const
{
	return 0.0f;
}

inline float NextBotGroundLocomotion::GetFrictionSideways( void ) const
{
	return 3.0f;
}

inline float NextBotGroundLocomotion::GetMaxYawRate( void ) const
{
	return 250.0f;
}

inline CBaseEntity *NextBotGroundLocomotion::GetGround( void ) const
{
	return m_ground;
}


inline const Vector &NextBotGroundLocomotion::GetGroundNormal( void ) const
{
	return m_groundNormal;
}


inline void NextBotGroundLocomotion::SetDesiredLean( const QAngle &lean )
{
	m_desiredLean = lean;
}


inline const QAngle &NextBotGroundLocomotion::GetDesiredLean( void ) const
{
	return m_desiredLean;
}


inline void NextBotGroundLocomotion::SetDesiredSpeed( float speed )
{
	m_desiredSpeed = speed;
}


inline float NextBotGroundLocomotion::GetDesiredSpeed( void ) const
{
	return m_desiredSpeed;
}


inline bool NextBotGroundLocomotion::IsClimbingOrJumping( void ) const
{
	return m_isJumping;
}

inline bool NextBotGroundLocomotion::IsClimbingUpToLedge( void ) const
{
	return m_isClimbingUpToLedge;
}

inline bool NextBotGroundLocomotion::IsJumpingAcrossGap( void ) const
{
	return m_isJumpingAcrossGap;
}

inline bool NextBotGroundLocomotion::IsRunning( void ) const
{
	/// @todo Rethink interface to distinguish actual state vs desired state (do we want to be running, or are we actually at running speed right now)
	return m_actualSpeed > 0.9f * GetRunSpeed();
}


inline float NextBotGroundLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


inline float NextBotGroundLocomotion::GetMaxJumpHeight( void ) const
{
	return 180.0f; // 120.0f; // 84.0f; // 58.0f;
}


inline float NextBotGroundLocomotion::GetDeathDropHeight( void ) const
{
	return 200.0f;
}


inline float NextBotGroundLocomotion::GetRunSpeed( void ) const
{
	return 150.0f;
}


inline float NextBotGroundLocomotion::GetWalkSpeed( void ) const
{
	return 75.0f;
}

inline float NextBotGroundLocomotion::GetMaxAcceleration( void ) const
{
	return 500.0f;
}

inline float NextBotGroundLocomotion::GetMaxDeceleration( void ) const
{
	return 500.0f;
}


#endif // NEXT_BOT_GROUND_LOCOMOTION_H

