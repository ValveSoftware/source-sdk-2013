// NextBotLocomotionInterface.h
// NextBot interface for movement through the environment
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_LOCOMOTION_INTERFACE_H_
#define _NEXT_BOT_LOCOMOTION_INTERFACE_H_

#include "NextBotComponentInterface.h"
#ifdef TF_DLL
#include "tf/nav_mesh/tf_nav_area.h"
#else
class CTFNavArea;

inline HSCRIPT ToHScript( CNavArea *pArea )
{
	return NULL;
}

inline HSCRIPT ToHScript( CTFNavArea *pArea )
{
	return NULL;
}

inline CNavArea *ToNavArea( HSCRIPT hScript )
{
	return NULL;
}
#endif

class Path;
class INextBot;
class CNavLadder;

//----------------------------------------------------------------------------------------------------------------
/**
 * The interface encapsulating *how* a bot moves through the world (walking? flying? etc)
 */
class ILocomotion : public INextBotComponent
{
public:
	ILocomotion( INextBot *bot );
	virtual ~ILocomotion();
	
	virtual void Reset( void );								// (EXTEND) reset to initial state
	virtual void Update( void );							// (EXTEND) update internal state
	
	//
	// The primary locomotive method
	// Depending on the physics of the bot's motion, it may not actually
	// reach the given position precisely.
	// The 'weight' can be used to combine multiple Approach() calls within
	// a single frame into a single goal (ie: weighted average)
	//
	virtual void Approach( const Vector &goalPos, float goalWeight = 1.0f );	// (EXTEND) move directly towards the given position

	//
	// Move the bot to the precise given position immediately, 
	// updating internal state as needed
	// Collision resolution is done to prevent interpenetration, which may prevent
	// the bot from reaching the given position. If no collisions occur, the
	// bot will be at the given position when this method returns.
	//
	virtual void DriveTo( const Vector &pos );				// (EXTEND) Move the bot to the precise given position immediately, 

	//
	// Locomotion modifiers
	//
	virtual bool ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle ) { return true; }	// initiate a jump to an adjacent high ledge, return false if climb can't start
	bool ScriptClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, HSCRIPT hObstacle )
	{
		return this->ClimbUpToLedge( landingGoal, landingForward, ToEnt( hObstacle ) );
	}
	virtual void JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward ) { }	// initiate a jump across an empty volume of space to far side
	virtual void Jump( void ) { }							// initiate a simple undirected jump in the air
	virtual bool IsClimbingOrJumping( void ) const;			// is jumping in any form
	virtual bool IsClimbingUpToLedge( void ) const;			// is climbing up to a high ledge
	virtual bool IsJumpingAcrossGap( void ) const;			// is jumping across a gap to the far side
	virtual bool IsScrambling( void ) const;				// is in the middle of a complex action (climbing a ladder, climbing a ledge, jumping, etc) that shouldn't be interrupted

	virtual void Run( void ) { }							// set desired movement speed to running
	virtual void Walk( void ) { }							// set desired movement speed to walking
	virtual void Stop( void ) { }							// set desired movement speed to stopped
	virtual bool IsRunning( void ) const;
	virtual void SetDesiredSpeed( float speed ) { }			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual void SetSpeedLimit( float speed ) { }					// set maximum speed bot can reach, regardless of desired speed
	virtual float GetSpeedLimit( void ) const { return 1000.0f; }	// get maximum speed bot can reach, regardless of desired speed

	virtual bool IsOnGround( void ) const;					// return true if standing on something
	virtual void OnLeaveGround( CBaseEntity *ground ) { }	// invoked when bot leaves ground for any reason
	void ScriptOnLeaveGround( HSCRIPT hGround ) { this->OnLeaveGround( ToEnt( hGround ) ); }
	virtual void OnLandOnGround( CBaseEntity *ground ) { }	// invoked when bot lands on the ground after being in the air
	void ScriptOnLandOnGround( HSCRIPT hGround ) { this->OnLandOnGround( ToEnt( hGround ) ); }
	virtual CBaseEntity *GetGround( void ) const;			// return the current ground entity or NULL if not on the ground
	HSCRIPT ScriptGetGround( void ) const { return ToHScript( this->GetGround() ); }
	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with
	virtual float GetGroundSpeed( void ) const;				// return current world space speed in XY plane
	virtual const Vector &GetGroundMotionVector( void ) const;	// return unit vector in XY plane describing our direction of motion - even if we are currently not moving

	virtual void ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal ) { }		// climb the given ladder to the top and dismount
	virtual void DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal ) { }	// descend the given ladder to the bottom and dismount
	virtual bool IsUsingLadder( void ) const;				// we are moving to get on, ascending/descending, and/or dismounting a ladder
	virtual bool IsAscendingOrDescendingLadder( void ) const;	// we are actually on the ladder right now, either climbing up or down
	virtual bool IsAbleToAutoCenterOnLadder( void ) const { return false; }

	virtual void FaceTowards( const Vector &target ) { }	// rotate body to face towards "target"

	virtual void SetDesiredLean( const QAngle &lean ) { }
	virtual const QAngle &GetDesiredLean( void ) const;
	

	//
	// Locomotion information
	//
	virtual bool IsAbleToJumpAcrossGaps( void ) const;		// return true if this bot can jump across gaps in its path
	virtual bool IsAbleToClimb( void ) const;				// return true if this bot can climb arbitrary geometry it encounters

	virtual const Vector &GetFeet( void ) const;			// return position of "feet" - the driving point where the bot contacts the ground

	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const;				// get maximum running speed
	virtual float GetWalkSpeed( void ) const;				// get maximum walking speed

	virtual float GetMaxAcceleration( void ) const;			// return maximum acceleration of locomotor
	virtual float GetMaxDeceleration( void ) const;			// return maximum deceleration of locomotor

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity
	virtual float GetSpeed( void ) const;					// return current world space speed (magnitude of velocity)
	virtual const Vector &GetMotionVector( void ) const;	// return unit vector describing our direction of motion - even if we are currently not moving

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const;	// return true if given area can be used for navigation
	bool ScriptIsAreaTraversable( HSCRIPT hBaseArea ) const { return this->IsAreaTraversable( ToNavArea( hBaseArea ) ); }

	virtual float GetTraversableSlopeLimit( void ) const;	// return Z component of unit normal of steepest traversable slope

	// return true if the given entity can be ignored during locomotion
	enum TraverseWhenType 
	{ 
		IMMEDIATELY,		// the entity will not block our motion - we'll carry right through
		EVENTUALLY			// the entity will block us until we spend effort to open/destroy it
	};

	/**
	 * Return true if this locomotor could potentially move along the line given.
	 * If false is returned, fraction of walkable ray is returned in 'fraction'
	 */
	virtual bool IsPotentiallyTraversable( const Vector &from, const Vector &to, TraverseWhenType when = EVENTUALLY, float *fraction = NULL ) const;
	float ScriptFractionPotentiallyTraversable( const Vector &from, const Vector &to, bool bImmediately )
	{
		float flFraction = 0.0f;
		this->IsPotentiallyTraversable( from, to, bImmediately ? IMMEDIATELY : EVENTUALLY, &flFraction );
		return flFraction;
	}
	float ScriptIsPotentiallyTraversable( const Vector &from, const Vector &to, bool bImmediately )
	{
		return this->IsPotentiallyTraversable( from, to, bImmediately ? IMMEDIATELY : EVENTUALLY, NULL );
	}

	/**
	 * Return true if there is a possible "gap" that will need to be jumped over
	 * If true is returned, fraction of ray before gap is returned in 'fraction'
	 */
	virtual bool HasPotentialGap( const Vector &from, const Vector &to, float *fraction = NULL ) const;
	float ScriptFractionPotentialGap( const Vector &from, const Vector &to )
	{
		float flFraction = 0.0f;
		this->HasPotentialGap( from, to, &flFraction );
		return flFraction;
	}
	float ScriptHasPotentialGap( const Vector &from, const Vector &to )
	{
		return this->HasPotentialGap( from, to, NULL );
	}

	// return true if there is a "gap" here when moving in the given direction
	virtual bool IsGap( const Vector &pos, const Vector &forward ) const;

	virtual bool IsEntityTraversable( CBaseEntity *obstacle, TraverseWhenType when = EVENTUALLY ) const;
	bool ScriptIsEntityTraversable( HSCRIPT hObstable, bool bImmediately ) const
	{
		CBaseEntity *pEntity = ToEnt( hObstable );
		if ( !pEntity )
			return false;
		
		return this->IsEntityTraversable( pEntity, bImmediately ? IMMEDIATELY : EVENTUALLY );
	}

	//
	// Stuck state.  If the locomotor cannot make progress, it becomes "stuck" and can only leave 
	// this stuck state by successfully moving and becoming un-stuck.
	//
	virtual bool IsStuck( void ) const;				// return true if bot is stuck 
	virtual float GetStuckDuration( void ) const;	// return how long we've been stuck
	virtual void ClearStuckStatus( const char *reason = "" );	// reset stuck status to un-stuck

	virtual bool IsAttemptingToMove( void ) const;	// return true if we have tried to Approach() or DriveTo() very recently

	void TraceHull( const Vector& start, const Vector& end, const Vector &mins, const Vector &maxs, unsigned int fMask, ITraceFilter *pFilter, trace_t *pTrace ) const;

	/**
	 * Should we collide with this entity?
	 */
	virtual bool ShouldCollideWith( const CBaseEntity *object ) const	{ return true; }

	//- Script access to locomotion functions ------------------------------------------------------------------
	DECLARE_ENT_SCRIPTDESC();

protected:
	virtual void AdjustPosture( const Vector &moveGoal );
	virtual void StuckMonitor( void );

private:
	Vector m_motionVector;
	Vector m_groundMotionVector;
	float m_speed;
	float m_groundSpeed;

	// stuck monitoring
	bool m_isStuck;									// if true, we are stuck
	IntervalTimer m_stuckTimer;						// how long we've been stuck
	CountdownTimer m_stillStuckTimer;				// for resending stuck events
	Vector m_stuckPos;								// where we got stuck
	IntervalTimer m_moveRequestTimer;
};


inline bool ILocomotion::IsAbleToJumpAcrossGaps( void ) const
{
	return true;
}

inline bool ILocomotion::IsAbleToClimb( void ) const
{
	return true;
}

inline bool ILocomotion::IsAttemptingToMove( void ) const
{
	return m_moveRequestTimer.HasStarted() && m_moveRequestTimer.GetElapsedTime() < 0.25f;
}

inline bool ILocomotion::IsScrambling( void ) const
{
	return !IsOnGround() || IsClimbingOrJumping() || IsAscendingOrDescendingLadder();
}

inline bool ILocomotion::IsClimbingOrJumping( void ) const
{
	return false;
}

inline bool ILocomotion::IsClimbingUpToLedge( void ) const
{
	return false;
}

inline bool ILocomotion::IsJumpingAcrossGap( void ) const
{
	return false;
}

inline bool ILocomotion::IsRunning( void ) const
{
	return false;
}

inline float ILocomotion::GetDesiredSpeed( void ) const
{
	return 0.0f;
}

inline bool ILocomotion::IsOnGround( void ) const
{
	return false;
}

inline CBaseEntity *ILocomotion::GetGround( void ) const
{
	return NULL;
}

inline const Vector &ILocomotion::GetGroundNormal( void ) const
{
	return vec3_origin;
}

inline float ILocomotion::GetGroundSpeed( void ) const
{
	return m_groundSpeed;
}

inline const Vector & ILocomotion::GetGroundMotionVector( void ) const
{
	return m_groundMotionVector;
}

inline bool ILocomotion::IsUsingLadder( void ) const
{
	return false;
}

inline bool ILocomotion::IsAscendingOrDescendingLadder( void ) const
{
	return false;
}

inline const QAngle &ILocomotion::GetDesiredLean( void ) const
{
	return vec3_angle;
}

inline float ILocomotion::GetStepHeight( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetMaxJumpHeight( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetDeathDropHeight( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetRunSpeed( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetWalkSpeed( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetMaxAcceleration( void ) const
{
	return 0.0f;
}

inline float ILocomotion::GetMaxDeceleration( void ) const
{
	return 0.0f;
}

inline const Vector &ILocomotion::GetVelocity( void ) const
{
	return vec3_origin;
}

inline float ILocomotion::GetSpeed( void ) const
{
	return m_speed;
}

inline const Vector & ILocomotion::GetMotionVector( void ) const
{
	return m_motionVector;
}

inline float ILocomotion::GetTraversableSlopeLimit( void ) const	
{ 
	return 0.6; 
}

inline bool ILocomotion::IsStuck( void ) const
{
	return m_isStuck;
}

inline float ILocomotion::GetStuckDuration( void ) const
{
	return ( IsStuck() ) ? m_stuckTimer.GetElapsedTime() : 0.0f;
}

inline void ILocomotion::TraceHull( const Vector& start, const Vector& end, const Vector &mins, const Vector &maxs, unsigned int fMask, ITraceFilter *pFilter, trace_t *pTrace ) const
{
//	VPROF_BUDGET( "ILocomotion::TraceHull", "TraceHull" );
	Ray_t ray;
	ray.Init( start, end, mins, maxs );
	enginetrace->TraceRay( ray, fMask, pFilter, pTrace );
}

#endif // _NEXT_BOT_LOCOMOTION_INTERFACE_H_

