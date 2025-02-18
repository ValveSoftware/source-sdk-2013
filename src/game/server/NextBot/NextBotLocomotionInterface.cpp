// NextBotLocomotionInterface.cpp
// Common functionality for all NextBot locomotors
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "BasePropDoor.h"

#include "nav_area.h"
#include "NextBot.h"
#include "NextBotUtil.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// how far a bot must move to not be considered "stuck"
#define STUCK_RADIUS 100.0f



//----------------------------------------------------------------------------------------------------------
/**
 * Reset to initial state
 */
ILocomotion::ILocomotion( INextBot *bot ) : INextBotComponent( bot )
{
	Reset();
}

ILocomotion::~ILocomotion()
{
}

void ILocomotion::Reset( void )
{
	INextBotComponent::Reset();

	m_motionVector = Vector( 1.0f, 0.0f, 0.0f );
	m_speed = 0.0f;
	m_groundMotionVector = m_motionVector;
	m_groundSpeed = m_speed;

	m_moveRequestTimer.Invalidate();

	m_isStuck = false;
	m_stuckTimer.Invalidate();
	m_stuckPos = vec3_origin;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Update internal state
 */
void ILocomotion::Update( void )
{
	StuckMonitor();

	// maintain motion vector and speed values
	const Vector &vel = GetVelocity();
	m_speed = vel.Length();
	m_groundSpeed = vel.AsVector2D().Length();

	const float velocityThreshold = 10.0f;
	if ( m_speed > velocityThreshold )
	{
		m_motionVector = vel / m_speed;
	}

	if ( m_groundSpeed > velocityThreshold )
	{
		m_groundMotionVector.x = vel.x / m_groundSpeed;
		m_groundMotionVector.y = vel.y / m_groundSpeed;
		m_groundMotionVector.z = 0.0f;
	}

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		// show motion vector
		NDebugOverlay::HorzArrow( GetFeet(), GetFeet() + 25.0f * m_groundMotionVector, 3.0f, 100, 255, 0, 255, true, 0.1f );
		NDebugOverlay::HorzArrow( GetFeet(), GetFeet() + 25.0f * m_motionVector, 5.0f, 255, 255, 0, 255, true, 0.1f );
	}
}


//----------------------------------------------------------------------------
void ILocomotion::AdjustPosture( const Vector &moveGoal )
{
	// This function has no effect if we're not standing or crouching
	IBody *body = GetBot()->GetBodyInterface();
	if ( !body->IsActualPosture( IBody::STAND ) && !body->IsActualPosture( IBody::CROUCH ) )
		return;

	//
	// Stand or crouch as needed
	//

	// get bounding limits, ignoring step-upable height
	const Vector &mins = body->GetHullMins() + Vector( 0, 0, GetStepHeight() );

	const float halfSize = body->GetHullWidth()/2.0f;
	Vector standMaxs( halfSize, halfSize, body->GetStandHullHeight() );

	trace_t trace;
	NextBotTraversableTraceFilter filter( GetBot(), ILocomotion::IMMEDIATELY );

	// snap forward movement vector along floor
	const Vector &groundNormal = GetGroundNormal();
	const Vector &feet = GetFeet();
	Vector moveDir = moveGoal - feet;
	float moveLength = moveDir.NormalizeInPlace();
	Vector left( -moveDir.y, moveDir.x, 0.0f );
	Vector goal = feet + moveLength * CrossProduct( left, groundNormal ).Normalized();

	TraceHull( feet, goal, mins, standMaxs, body->GetSolidMask(), &filter, &trace );

	if ( trace.fraction >= 1.0f && !trace.startsolid )
	{
		// no collision while standing
		if ( body->IsActualPosture( IBody::CROUCH ) )
		{
			body->SetDesiredPosture( IBody::STAND );
		}
		return;
	}

	if ( body->IsActualPosture( IBody::CROUCH ) )
		return;

	// crouch hull check
	Vector crouchMaxs( halfSize, halfSize, body->GetCrouchHullHeight() );

	TraceHull( feet, goal, mins, crouchMaxs, body->GetSolidMask(), &filter, &trace );

	if ( trace.fraction >= 1.0f && !trace.startsolid )
	{
		// no collision while crouching
		body->SetDesiredPosture( IBody::CROUCH );
	}
}


//----------------------------------------------------------------------------------------------------------
/**
 * Move directly towards the given position
 */
void ILocomotion::Approach( const Vector &goalPos, float goalWeight )
{
	// there is a desire to move
	m_moveRequestTimer.Start();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Move the bot to the precise given position immediately
 */
void ILocomotion::DriveTo( const Vector &pos )
{
	// there is a desire to move
	m_moveRequestTimer.Start();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return true if this locomotor could potentially move along the line given.
 * If false is returned, fraction of walkable ray is returned in 'fraction'
 */
bool ILocomotion::IsPotentiallyTraversable( const Vector &from, const Vector &to, TraverseWhenType when, float *fraction ) const
{
	VPROF_BUDGET( "Locomotion::IsPotentiallyTraversable", "NextBotExpensive" );

	// if 'to' is high above us, it's not directly traversable
	// Adding a bit of fudge room to allow for floating point roundoff errors
	if ( ( to.z - from.z ) > GetMaxJumpHeight() + 0.1f )
	{
		Vector along = to - from;
		along.NormalizeInPlace();
		if ( along.z > GetTraversableSlopeLimit() )
		{
			if ( fraction )
			{
				*fraction = 0.0f;
			}
			return false;
		}
	}

	trace_t result;
	NextBotTraversableTraceFilter filter( GetBot(), when );

	// use a small hull since we cannot simulate collision resolution and avoidance along the way
	const float probeSize = 0.25f * GetBot()->GetBodyInterface()->GetHullWidth(); // Cant be TOO small, or open stairwells/grates/etc will cause problems
	const float probeZ = GetStepHeight();

	Vector hullMin( -probeSize, -probeSize, probeZ );
	Vector hullMax( probeSize, probeSize, GetBot()->GetBodyInterface()->GetCrouchHullHeight() );
	TraceHull( from, to, hullMin, hullMax, GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

/*
	if ( result.DidHit() )
	{
		NDebugOverlay::SweptBox( from, result.endpos, hullMin, hullMax, vec3_angle, 255, 0, 0, 255, 9999.9f );
		NDebugOverlay::SweptBox( result.endpos, to, hullMin, hullMax, vec3_angle, 255, 255, 0, 255, 9999.9f );
	}
	else
	{
		NDebugOverlay::SweptBox( from, to, hullMin, hullMax, vec3_angle, 255, 255, 0, 255, 0.1f );
	}
*/

	if ( fraction )
	{
		*fraction = result.fraction;
	}

	return ( result.fraction >= 1.0f ) && ( !result.startsolid );
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return true if there is a possible "gap" that will need to be jumped over
 * If true is returned, fraction of ray before gap is returned in 'fraction'
 */
bool ILocomotion::HasPotentialGap( const Vector &from, const Vector &desiredTo, float *fraction ) const
{
	VPROF_BUDGET( "Locomotion::HasPotentialGap", "NextBot" );

	// find section of this ray that is actually traversable
	float traversableFraction;
	IsPotentiallyTraversable( from, desiredTo, IMMEDIATELY, &traversableFraction );

	// compute end of traversable ray
	Vector to = from + ( desiredTo - from ) * traversableFraction;

	Vector forward = to - from;
	float length = forward.NormalizeInPlace();

	IBody *body = GetBot()->GetBodyInterface();

	float step = body->GetHullWidth()/2.0f;

	// scan along the line checking for gaps
	Vector pos = from;
	Vector delta = step * forward;
	for( float t = 0.0f; t < (length + step); t += step )
	{
		if ( IsGap( pos, forward ) )
		{
			if ( fraction )
			{
				*fraction = ( t - step ) / ( length + step );
			}
			
			return true;
		}

		pos += delta;		
	}

	if ( fraction )
	{
		*fraction = 1.0f;
	}

	return false;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return true if there is a "gap" here when moving in the given direction.
 * A "gap" is a vertical dropoff that is too high to jump back up to.
 */
bool ILocomotion::IsGap( const Vector &pos, const Vector &forward ) const
{
	VPROF_BUDGET( "Locomotion::IsGap", "NextBotSpiky" );

	IBody *body = GetBot()->GetBodyInterface();

	//float halfWidth = ( body ) ? body->GetHullWidth()/2.0f : 1.0f;

	// can't really jump effectively when crouched anyhow
	//float hullHeight = ( body ) ? body->GetStandHullHeight() : 1.0f;

	// use a small hull since we cannot simulate collision resolution and avoidance along the way
	const float halfWidth = 1.0f;
	const float hullHeight = 1.0f;

	unsigned int mask = ( body ) ? body->GetSolidMask() : MASK_PLAYERSOLID;

	trace_t ground;

	NextBotTraceFilterIgnoreActors filter( GetBot()->GetEntity(), COLLISION_GROUP_NONE );

	TraceHull( pos + Vector( 0, 0, GetStepHeight() ),	// start up a bit to handle rough terrain
					pos + Vector( 0, 0, -GetMaxJumpHeight() ), 
					Vector( -halfWidth, -halfWidth, 0 ), Vector( halfWidth, halfWidth, hullHeight ), 
					mask, &filter, &ground );

// 	int r,g,b;
// 
// 	if ( ground.fraction >= 1.0f && !ground.startsolid )
// 	{
// 		r = 255, g = 0, b = 0;
// 	}
// 	else
// 	{
// 		r = 0, g = 255, b = 0;
// 	}
// 
// 	NDebugOverlay::SweptBox( pos,
// 							 pos + Vector( 0, 0, -GetStepHeight() ),
// 							 Vector( -halfWidth, -halfWidth, 0 ), Vector( halfWidth, halfWidth, hullHeight ),
// 							 vec3_angle,
// 							 r, g, b, 255, 3.0f );

	// if trace hit nothing, there's a gap ahead of us
	return ( ground.fraction >= 1.0f && !ground.startsolid );
}


//----------------------------------------------------------------------------------------------------------
bool ILocomotion::IsEntityTraversable( CBaseEntity *obstacle, TraverseWhenType when ) const
{
	if ( obstacle->IsWorld() )
		return false;

	// assume bot will open a door in its path
	if ( FClassnameIs( obstacle, "prop_door*" ) || FClassnameIs( obstacle, "func_door*" ) )
	{
		CBasePropDoor *door = dynamic_cast< CBasePropDoor * >( obstacle );

		if ( door && door->IsDoorOpen() )
		{
			// open doors are obstacles
			return false;
		}

		return true;
	}

	// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
	if ( FClassnameIs( obstacle, "func_brush" ) )
	{
		CFuncBrush *brush = (CFuncBrush *)obstacle;
		
		switch ( brush->m_iSolidity )
		{
			case CFuncBrush::BRUSHSOLID_ALWAYS:
				return false;
			case CFuncBrush::BRUSHSOLID_NEVER:
				return true;
			case CFuncBrush::BRUSHSOLID_TOGGLE:
				return true;
		}
	}

	if ( when == IMMEDIATELY )
	{
		// special rules in specific games can immediately break some breakables, etc.
		return false;
	}

	// assume bot will EVENTUALLY break breakables in its path
	return GetBot()->IsAbleToBreak( obstacle );
}


//--------------------------------------------------------------------------------------------------------------
bool ILocomotion::IsAreaTraversable( const CNavArea *baseArea ) const
{ 
	return !baseArea->IsBlocked( GetBot()->GetEntity()->GetTeamNumber() );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Reset stuck status to un-stuck
 */
void ILocomotion::ClearStuckStatus( const char *reason )
{
	if ( IsStuck() )
	{
		m_isStuck = false;

		// tell other components we're no longer stuck
		GetBot()->OnUnStuck();
	}

	// always reset stuck monitoring data in case we cleared preemptively are were not yet stuck
	m_stuckPos = GetFeet();
	m_stuckTimer.Start();

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		DevMsg( "%3.2f: ClearStuckStatus: %s %s\n", gpGlobals->curtime, GetBot()->GetDebugIdentifier(), reason );
	}
}


//--------------------------------------------------------------------------------------------------------------
/** 
 * Stuck check
 */
void ILocomotion::StuckMonitor( void )
{
	// a timer is needed to smooth over a few frames of inactivity due to state changes, etc.
	// we only want to detect idle situations when the bot really doesn't "want" to move.
	const float idleTime = 0.25f;
	if ( m_moveRequestTimer.IsGreaterThen( idleTime ) )
	{
		// we have no desire to move, and therefore cannot emit stuck events

		// prepare our internal state for when the bot starts to move next
		m_stuckPos = GetFeet();
		m_stuckTimer.Start();

		return;
	}

// 	if ( !IsOnGround() )
// 	{
// 		// can't be stuck when in-air
// 		ClearStuckStatus( "Off the ground" );
// 		return;
// 	}

// 	if ( IsUsingLadder() )
// 	{
// 		// can't be stuck when on a ladder (for now)
// 		ClearStuckStatus( "On a ladder" );
// 		return;
// 	}

	if ( IsStuck() )
	{
		// we are/were stuck - have we moved enough to consider ourselves "dislodged"
		if ( GetBot()->IsRangeGreaterThan( m_stuckPos, STUCK_RADIUS ) )
		{
			// we've just become un-stuck
			ClearStuckStatus( "UN-STUCK" );
		}
		else
		{
			// still stuck - periodically resend the event
			if ( m_stillStuckTimer.IsElapsed() )
			{
				m_stillStuckTimer.Start( 1.0f );

				if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
				{
					DevMsg( "%3.2f: %s STILL STUCK\n", gpGlobals->curtime, GetBot()->GetDebugIdentifier() );
					NDebugOverlay::Circle( m_stuckPos + Vector( 0, 0, 5.0f ), QAngle( -90.0f, 0, 0 ), 5.0f, 255, 0, 0, 255, true, 1.0f );
				}

				GetBot()->OnStuck();
			}
		}
	}
	else
	{
		// we're not stuck - yet

		if ( /*IsClimbingOrJumping() || */GetBot()->IsRangeGreaterThan( m_stuckPos, STUCK_RADIUS ) )
		{
			// we have moved - reset anchor
			m_stuckPos = GetFeet();

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::Cross3D( m_stuckPos, 3.0f, 255, 0, 255, true, 3.0f );
			}

			m_stuckTimer.Start();
		}
		else
		{
			// within stuck range of anchor. if we've been here too long, we're stuck
			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::Line( GetBot()->GetEntity()->WorldSpaceCenter(), m_stuckPos, 255, 0, 255, true, 0.1f );
			}

			float minMoveSpeed = 0.1f * GetDesiredSpeed() + 0.1f;
			float escapeTime = STUCK_RADIUS / minMoveSpeed;
			if ( m_stuckTimer.IsGreaterThen( escapeTime ) )
			{
				// we have taken too long - we're stuck
				m_isStuck = true;

				if ( GetBot()->IsDebugging( NEXTBOT_ERRORS ) )
				{
					DevMsg( "%3.2f: %s STUCK at position( %3.2f, %3.2f, %3.2f )\n", gpGlobals->curtime, GetBot()->GetDebugIdentifier(), m_stuckPos.x, m_stuckPos.y, m_stuckPos.z );

					NDebugOverlay::Circle( m_stuckPos + Vector( 0, 0, 15.0f ), QAngle( -90.0f, 0, 0 ), 3.0f, 255, 255, 0, 255, true, 1.0f );
					NDebugOverlay::Circle( m_stuckPos + Vector( 0, 0, 5.0f ), QAngle( -90.0f, 0, 0 ), 5.0f, 255, 0, 0, 255, true, 9999999.9f );
				}

				// tell other components we've become stuck
				GetBot()->OnStuck();
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
const Vector &ILocomotion::GetFeet( void ) const
{
	return GetBot()->GetEntity()->GetAbsOrigin();
}

//--------------------------------------------------------------------------------------------------------------

BEGIN_ENT_SCRIPTDESC( ILocomotion, INextBotComponent, "Next bot locomotion" )
	DEFINE_SCRIPTFUNC( Approach, "The primary locomotive method. Sets the goal destination for the bot" )
	DEFINE_SCRIPTFUNC( DriveTo, "Move the bot to the precise given position immediately, updating internal state" )
	DEFINE_SCRIPTFUNC_WRAPPED( ClimbUpToLedge, "Initiate a jump to an adjacent high ledge, return false if climb can't start" )
	DEFINE_SCRIPTFUNC( JumpAcrossGap, "Initiate a jump across an empty volume of space to far side" )
	DEFINE_SCRIPTFUNC( Jump, "Initiate a simple undirected jump in the air" )
	DEFINE_SCRIPTFUNC( IsClimbingOrJumping, "Is jumping in any form" )
	DEFINE_SCRIPTFUNC( IsClimbingUpToLedge, "Is climbing up to a high ledge" )
	DEFINE_SCRIPTFUNC( IsJumpingAcrossGap, "Is jumping across a gap to the far side" )
	DEFINE_SCRIPTFUNC( IsScrambling, "Is in the middle of a complex action (climbing a ladder, climbing a ledge, jumping, etc) that shouldn't be interrupted" )
	DEFINE_SCRIPTFUNC( Run, "Set desired movement speed to running" )
	DEFINE_SCRIPTFUNC( Walk, "Set desired movement speed to walking" )
	DEFINE_SCRIPTFUNC( Stop, "Set desired movement speed to stopped" )
	DEFINE_SCRIPTFUNC( IsRunning, "Is running?" )
	DEFINE_SCRIPTFUNC( SetDesiredSpeed, "Set desired speed for locomotor movement" )
	DEFINE_SCRIPTFUNC( GetDesiredSpeed, "Get desired speed for locomotor movement" )
	DEFINE_SCRIPTFUNC( SetSpeedLimit, "Set maximum speed bot can reach, regardless of desired speed" )
	DEFINE_SCRIPTFUNC( GetSpeedLimit, "Get maximum speed bot can reach, regardless of desired speed" )
	DEFINE_SCRIPTFUNC( IsOnGround, "Return true if standing on something" )
	DEFINE_SCRIPTFUNC_WRAPPED( OnLeaveGround, "Manually run the OnLeaveGround callback. Typically invoked when bot leaves ground for any reason" )
	DEFINE_SCRIPTFUNC_WRAPPED( OnLandOnGround, "Manually run the OnLandOnGround callback. Typically invoked when bot lands on the ground after being in the air" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetGround, "Return the current ground entity or NULL if not on the ground" )
	DEFINE_SCRIPTFUNC( GetGroundNormal, "Surface normal of the ground we are in contact with" )
	DEFINE_SCRIPTFUNC( GetGroundSpeed, "Return current world space speed in XY plane" )
	DEFINE_SCRIPTFUNC( GetGroundMotionVector, "Return unit vector in XY plane describing our direction of motion - even if we are currently not moving" )
	DEFINE_SCRIPTFUNC( FaceTowards, "Rotate body to face towards target" )
	DEFINE_SCRIPTFUNC( IsAbleToJumpAcrossGaps, "Return true if this bot can jump across gaps in its path" )
	DEFINE_SCRIPTFUNC( IsAbleToClimb, "Return true if this bot can climb arbitrary geometry it encounters" )
	DEFINE_SCRIPTFUNC( GetFeet, "Return position of feet - the driving point where the bot contacts the ground" )
	DEFINE_SCRIPTFUNC( GetStepHeight, "If delta Z is greater than this, we have to jump to get up" )
	DEFINE_SCRIPTFUNC( GetMaxJumpHeight, "Return maximum height of a jump" )
	DEFINE_SCRIPTFUNC( GetDeathDropHeight, "Distance at which we will die if we fall" )
	DEFINE_SCRIPTFUNC( GetRunSpeed, "Get maximum running speed" )
	DEFINE_SCRIPTFUNC( GetWalkSpeed, "Get maximum walking speed" )
	DEFINE_SCRIPTFUNC( GetMaxAcceleration, "Return maximum acceleration of locomotor" )
	DEFINE_SCRIPTFUNC( GetMaxDeceleration, "Return maximum deceleration of locomotor" )
	DEFINE_SCRIPTFUNC( GetVelocity, "Return current world space velocity" )
	DEFINE_SCRIPTFUNC( GetSpeed, "Return current world space speed (magnitude of velocity)" )
	DEFINE_SCRIPTFUNC( GetMotionVector, "Return unit vector describing our direction of motion - even if we are currently not moving" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsAreaTraversable, "Return true if given area can be used for navigation" )
	DEFINE_SCRIPTFUNC( GetTraversableSlopeLimit, "Return Z component of unit normal of steepest traversable slope" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsPotentiallyTraversable, "Return true if this locomotor could potentially move along the line given." )
	DEFINE_SCRIPTFUNC_WRAPPED( FractionPotentiallyTraversable, "If the locomotor could not move along the line given, returns the fraction of the walkable ray." )
	DEFINE_SCRIPTFUNC_WRAPPED( HasPotentialGap, "Return true if there is a possible gap that will need to be jumped over" )
	DEFINE_SCRIPTFUNC_WRAPPED( FractionPotentialGap, "If the locomotor cannot jump over the gap, returns the fraction of the jumpable ray" )
	DEFINE_SCRIPTFUNC( IsGap, "Return true if there is a gap here when moving in the given direction" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsEntityTraversable, "Return true if the entity handle is traversable" )
	DEFINE_SCRIPTFUNC( IsStuck, "Return true if bot is stuck. If the locomotor cannot make progress, it becomes stuck and can only leave this stuck state by successfully movingand becoming un-stuck." )
	DEFINE_SCRIPTFUNC( GetStuckDuration, "Return how long we've been stuck" )
	DEFINE_SCRIPTFUNC( ClearStuckStatus, "Reset stuck status to un-stuck" )
	DEFINE_SCRIPTFUNC( IsAttemptingToMove, "Return true if we have tried to Approach() or DriveTo() very recently" )
END_SCRIPTDESC();
