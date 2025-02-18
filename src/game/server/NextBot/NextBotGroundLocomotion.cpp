//========= Copyright Valve Corporation, All rights reserved. ============//
// NextBotGroundLocomotion.cpp
// Basic ground-based movement for NextBotCombatCharacters
// Author: Michael Booth, February 2009
// Note: This is a refactoring of ZombieBotLocomotion from L4D

#include "cbase.h"

#include "func_break.h"
#include "func_breakablesurf.h"
#include "activitylist.h"
#include "BasePropDoor.h"

#include "nav.h"
#include "NextBot.h"
#include "NextBotGroundLocomotion.h"
#include "NextBotUtil.h"
#include "functorutils.h"
#include "SharedFunctorUtils.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable : 4355 )			// warning 'this' used in base member initializer list - we're using it safely


//----------------------------------------------------------------------------------------------------------
NextBotGroundLocomotion::NextBotGroundLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	m_nextBot = NULL;
	m_ladder = NULL;
	m_desiredLean.x = 0.0f;
	m_desiredLean.y = 0.0f;
	m_desiredLean.z = 0.0f;
	
	m_bRecomputePostureOnCollision = false;
	m_ignorePhysicsPropTimer.Invalidate();
}


//----------------------------------------------------------------------------------------------------------
NextBotGroundLocomotion::~NextBotGroundLocomotion()
{
}


//----------------------------------------------------------------------------------------------------------
/**
 * Reset locomotor to initial state
 */
void NextBotGroundLocomotion::Reset( void )
{
	BaseClass::Reset();
	m_bRecomputePostureOnCollision = false;
	m_ignorePhysicsPropTimer.Invalidate();

	m_nextBot = static_cast< NextBotCombatCharacter * >( GetBot()->GetEntity() );
	
	m_desiredSpeed = 0.0f;
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;

	m_desiredLean.x = 0.0f;
	m_desiredLean.y = 0.0f;
	m_desiredLean.z = 0.0f;
	
	m_ladder = NULL;

	m_isJumping = false;
	m_isJumpingAcrossGap = false;
	m_ground = NULL;
	m_groundNormal = Vector( 0, 0, 1.0f );
	m_isClimbingUpToLedge = false;
	m_isUsingFullFeetTrace = false;

	m_moveVector = Vector( 1, 0, 0 );
	
	m_priorPos = m_nextBot->GetPosition();
	m_lastValidPos = m_nextBot->GetPosition();

	m_inhibitObstacleAvoidanceTimer.Invalidate();

	m_accumApproachVectors = vec3_origin;
	m_accumApproachWeights = 0.0f;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Move the bot along a ladder
 */
bool NextBotGroundLocomotion::TraverseLadder( void )
{
	// not climbing a ladder right now
	return false;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Update internal state
 */
void NextBotGroundLocomotion::Update( void )
{
	VPROF_BUDGET( "NextBotGroundLocomotion::Update", "NextBot" );

	BaseClass::Update();

	const float deltaT = GetUpdateInterval();

	// apply accumulated position changes
	ApplyAccumulatedApproach();

	// need to do this first thing, because ground constraints, etc, can change it
	Vector origPos = GetFeet();

	IBody *body = GetBot()->GetBodyInterface();

	if ( TraverseLadder() )
	{
		// bot is climbing a ladder
		return;
	}

	if ( !body->IsPostureMobile() )
	{
		// sitting/lying on the ground - no slip
		m_acceleration.x = 0.0f;
		m_acceleration.y = 0.0f;
		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;
	}

	bool wasOnGround = IsOnGround();

	if ( !body->HasActivityType( IBody::MOTION_CONTROLLED_Z ) )
	{
		// fall if in the air
		if ( !IsOnGround() )
		{
			// no ground below us - fall
			m_acceleration.z -= GetGravity();
		}

		if ( !IsClimbingOrJumping() || m_velocity.z <= 0.0f )
		{
			// keep us on the ground
			UpdateGroundConstraint();
		}
	}

	Vector newPos = GetFeet();

	//
	// Update position physics
	//
	Vector right( m_moveVector.y, -m_moveVector.x, 0.0f );

	if ( IsOnGround() ) // || m_isClimbingUpToLedge )
	{
		if ( IsAttemptingToMove() )
		{
			float forwardSpeed = DotProduct( m_velocity, m_moveVector );
			Vector forwardVelocity = forwardSpeed * m_moveVector;
			Vector sideVelocity = DotProduct( m_velocity, right ) * right;

			Vector frictionAccel = vec3_origin;

			// only apply friction along forward direction if we are sliding backwards
			if ( forwardSpeed < 0.0f )
			{
				frictionAccel = -GetFrictionForward() * forwardVelocity;
			}

			// always apply lateral friction to counteract sideslip		
			frictionAccel += -GetFrictionSideways() * sideVelocity;

			m_acceleration.x += frictionAccel.x;
			m_acceleration.y += frictionAccel.y;
		}
		else
		{
			// come to a stop if we haven't been told to move
			m_acceleration = vec3_origin;
			m_velocity = vec3_origin;
		}
	}

	// compute new position, taking into account MOTION_CONTROLLED animations in progress
	if ( body->HasActivityType( IBody::MOTION_CONTROLLED_XY ) )
	{
		m_acceleration.x = 0.0f;
		m_acceleration.y = 0.0f;
		m_velocity.x = GetBot()->GetEntity()->GetAbsVelocity().x;
		m_velocity.y = GetBot()->GetEntity()->GetAbsVelocity().y;
	}
	else
	{
		// euler integration
		m_velocity.x += m_acceleration.x * deltaT;
		m_velocity.y += m_acceleration.y * deltaT;

		// euler integration		
		newPos.x += m_velocity.x * deltaT;
		newPos.y += m_velocity.y * deltaT;
	}

	if ( body->HasActivityType( IBody::MOTION_CONTROLLED_Z ) )
	{
		m_acceleration.z = 0.0f;
		m_velocity.z = GetBot()->GetEntity()->GetAbsVelocity().z;
	}
	else
	{
		// euler integration
		m_velocity.z += m_acceleration.z * deltaT;

		// euler integration		
		newPos.z += m_velocity.z * deltaT;
	}
	
	// move bot to new position, resolving collisions along the way
	UpdatePosition( newPos );


	// set actual velocity based on position change after collision resolution step
	Vector adjustedVelocity = ( GetFeet() - origPos ) / deltaT;

	if ( !body->HasActivityType( IBody::MOTION_CONTROLLED_XY ) )
	{
		m_velocity.x = adjustedVelocity.x;
		m_velocity.y = adjustedVelocity.y;
	}

	if ( !body->HasActivityType( IBody::MOTION_CONTROLLED_Z ) )
	{
		m_velocity.z = adjustedVelocity.z;
	}


	// collision resolution may create very high instantaneous velocities, limit it
	Vector2D groundVel = m_velocity.AsVector2D();
	m_actualSpeed = groundVel.NormalizeInPlace();

	if ( IsOnGround() )
	{
		if ( m_actualSpeed > GetRunSpeed() )
		{
			m_actualSpeed = GetRunSpeed();
			m_velocity.x = m_actualSpeed * groundVel.x;
			m_velocity.y = m_actualSpeed * groundVel.y;
		}

		// remove downward velocity when landing on the ground
		if ( !wasOnGround )
		{
			m_velocity.z = 0.0f;
			m_acceleration.z = 0.0f;
		}
	}
	else
	{
		// we're falling. if our velocity has become zero for any reason, shove it forward
		const float epsilon = 1.0f;
		if ( m_velocity.IsLengthLessThan( epsilon ) )
		{
			m_velocity = GetRunSpeed() * GetGroundMotionVector();
		}
	}

	// update entity velocity to that of locomotor
	m_nextBot->SetAbsVelocity( m_velocity );


#ifdef LEANING
	// lean sideways proportional to lateral acceleration
	QAngle lean = GetDesiredLean();
	
	float sideAccel = DotProduct( right, m_acceleration );
	float slide = sideAccel / GetMaxAcceleration();

	// max lean depends on how fast we're actually moving
	float maxLeanAngle = NextBotLeanMaxAngle.GetFloat() * m_actualSpeed / GetRunSpeed();

	// actual lean angle is proportional to lateral acceleration (sliding)
	float desiredSideLean = -maxLeanAngle * slide;
	
	lean.y += ( desiredSideLean - lean.y ) * NextBotLeanRate.GetFloat() * deltaT;

	SetDesiredLean( lean );
#endif // _DEBUG


	// reset acceleration accumulation
	m_acceleration = vec3_origin;

	// debug display
	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		// track position over time
		if ( IsOnGround() )
		{
			NDebugOverlay::Cross3D( GetFeet(), 1.0f, 0, 255, 0, true, 15.0f );
		}
		else
		{
			NDebugOverlay::Cross3D( GetFeet(), 1.0f, 0, 255, 255, true, 15.0f );
		}
	}
}


//----------------------------------------------------------------------------------------------------------
/**
 * Move directly towards given position.
 * We need to do this in-air as well to land jumps.
 */
void NextBotGroundLocomotion::Approach( const Vector &rawPos, float goalWeight )
{
	BaseClass::Approach( rawPos );

	m_accumApproachVectors += ( rawPos - GetFeet() ) * goalWeight;
	m_accumApproachWeights += goalWeight;
	m_bRecomputePostureOnCollision = true;
}


//----------------------------------------------------------------------------------------------------------
void NextBotGroundLocomotion::ApplyAccumulatedApproach( void )
{
	VPROF_BUDGET( "NextBotGroundLocomotion::ApplyAccumulatedApproach", "NextBot" );

	Vector rawPos = GetFeet();

	const float deltaT = GetUpdateInterval();

	if ( deltaT <= 0.0f )
		return;

	if ( m_accumApproachWeights > 0.0f )
	{
		Vector approachDelta = m_accumApproachVectors / m_accumApproachWeights;

		// limit total movement to our max speed
		float maxMove = GetRunSpeed() * deltaT;

		float desiredMove = approachDelta.NormalizeInPlace();
		if ( desiredMove > maxMove )
		{
			desiredMove = maxMove;
		}

		rawPos += desiredMove * approachDelta;

		m_accumApproachVectors = vec3_origin;
		m_accumApproachWeights = 0.0f;
	}

	// can only move in 2D - geometry moves us up and down
	Vector pos( rawPos.x, rawPos.y, GetFeet().z );
		
	if ( !GetBot()->GetBodyInterface()->IsPostureMobile() )
	{
		// body is not in a movable state right now
		return;
	}

	Vector currentPos = m_nextBot->GetPosition();

	// compute unit vector to goal position
	m_moveVector = pos - currentPos;
	m_moveVector.z = 0.0f;
	float change = m_moveVector.NormalizeInPlace();

	const float epsilon = 0.001f;
	if ( change < epsilon )
	{
		// no motion
		m_forwardLean = 0.0f;
		m_sideLean = 0.0f;
		return;
	}

/*	
	// lean forward/backward based on acceleration
	float desiredLean = m_acceleration / NextBotLeanForwardAccel.GetFloat();

	QAngle lean = GetDesiredLean();

	lean.x = NextBotLeanMaxAngle.GetFloat() * clamp( desiredLean, -1.0f, 1.0f );	

	SetDesiredLean( lean );
*/	

	Vector newPos;

	// if we just started a jump, don't snap to the ground - let us get in the air first
	if ( DidJustJump() || !IsOnGround() )
	{
		if ( false && m_isClimbingUpToLedge )	// causes bots to hang in air stuck against edges
		{
			// drive towards the approach position in XY to help reach ledge
			m_moveVector = m_ledgeJumpGoalPos - currentPos;
			m_moveVector.z = 0.0f;
			m_moveVector.NormalizeInPlace();
			
			m_acceleration += GetMaxAcceleration() * m_moveVector;
		}
	}
	else if ( IsOnGround() )
	{
		// on the ground - move towards the approach position
		m_isClimbingUpToLedge = false;
		
		// snap forward movement vector along floor
		const Vector &groundNormal = GetGroundNormal();
		
		Vector left( -m_moveVector.y, m_moveVector.x, 0.0f );
		m_moveVector = CrossProduct( left, groundNormal );
		m_moveVector.NormalizeInPlace();
		
		// limit maximum forward speed from self-acceleration
		float forwardSpeed = DotProduct( m_velocity, m_moveVector );
		
		float maxSpeed = MIN( m_desiredSpeed, GetSpeedLimit() );
		
		if ( forwardSpeed < maxSpeed )
		{
			float ratio = ( forwardSpeed <= 0.0f ) ? 0.0f : ( forwardSpeed / maxSpeed );
			float governor = 1.0f - ( ratio * ratio * ratio * ratio );
			
			// accelerate towards goal
			m_acceleration += governor * GetMaxAcceleration() * m_moveVector;
		}
	}
}


//----------------------------------------------------------------------------------------------------------
/**
 * Move the bot to the precise given position immediately, 
 */
void NextBotGroundLocomotion::DriveTo( const Vector &pos )
{
	BaseClass::DriveTo( pos );
	m_bRecomputePostureOnCollision = true;
	UpdatePosition( pos );
}


//--------------------------------------------------------------------------------------------
/*
 * Trace filter solely for use with DetectCollision() below.
 */
class GroundLocomotionCollisionTraceFilter : public CTraceFilterSimple
{
public:
	GroundLocomotionCollisionTraceFilter( INextBot *me, const IHandleEntity *passentity, int collisionGroup ) : CTraceFilterSimple( passentity, collisionGroup )
	{
		m_me = me;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );

			// don't collide with ourself
			if ( entity && m_me->IsSelf( entity ) )
				return false;

			return m_me->GetLocomotionInterface()->ShouldCollideWith( entity );
		}

		return false;
	}

	INextBot *m_me;
};


//----------------------------------------------------------------------------------------------------------
/**
 * Check for collisions during move and attempt to resolve them
 */
bool NextBotGroundLocomotion::DetectCollision( trace_t *pTrace, int &recursionLimit, const Vector &from, const Vector &to, const Vector &vecMins, const Vector &vecMaxs )
{
	IBody *body = GetBot()->GetBodyInterface();

	CBaseEntity *ignore = m_ignorePhysicsPropTimer.IsElapsed() ? NULL : m_ignorePhysicsProp;
	GroundLocomotionCollisionTraceFilter filter( GetBot(), ignore, body->GetCollisionGroup() );

	TraceHull( from, to, vecMins, vecMaxs, body->GetSolidMask(), &filter, pTrace );

	if ( !pTrace->DidHit() )
		return false;

	//
	// A collision occurred - resolve it
	//

	// bust through "flimsy" breakables and keep on going
	if ( pTrace->DidHitNonWorldEntity() && pTrace->m_pEnt != NULL )
	{
		CBaseEntity *other = pTrace->m_pEnt;

		if ( !other->MyCombatCharacterPointer() && IsEntityTraversable( other, IMMEDIATELY ) /*&& IsFlimsy( other )*/ )
		{
			if ( recursionLimit <= 0 ) 
				return true;

			--recursionLimit;

			// break the weak breakable we collided with
			CTakeDamageInfo damageInfo( GetBot()->GetEntity(), GetBot()->GetEntity(), 100.0f, DMG_CRUSH );
			CalculateExplosiveDamageForce( &damageInfo, GetMotionVector(), pTrace->endpos );
			other->TakeDamage( damageInfo );

			// retry trace now that the breakable is out of the way
			return DetectCollision( pTrace, recursionLimit, from, to, vecMins, vecMaxs );
		}
	}

	/// @todo Only invoke OnContact() and Touch() once per collision pair
	// inform other components of collision
	if ( GetBot()->ShouldTouch( pTrace->m_pEnt ) )
	{
		GetBot()->OnContact( pTrace->m_pEnt, pTrace );
	}

	INextBot *them = dynamic_cast< INextBot * >( pTrace->m_pEnt );
	if ( them && them->ShouldTouch( m_nextBot ) )
	{
		/// @todo construct mirror of trace
		them->OnContact( m_nextBot );
	}
	else
	{
		pTrace->m_pEnt->Touch( GetBot()->GetEntity() );
	}

	return true;
}


//----------------------------------------------------------------------------------------------------------
Vector NextBotGroundLocomotion::ResolveCollision( const Vector &from, const Vector &to, int recursionLimit )
{
	VPROF_BUDGET( "NextBotGroundLocomotion::ResolveCollision", "NextBotExpensive" );

	IBody *body = GetBot()->GetBodyInterface();
	if ( body == NULL || recursionLimit < 0 )
	{
		Assert( !m_bRecomputePostureOnCollision );
		return to;
	}

	// Only bother to recompute posture if we're currently standing or crouching
	if ( m_bRecomputePostureOnCollision )
	{
		if ( !body->IsActualPosture( IBody::STAND ) && !body->IsActualPosture( IBody::CROUCH ) )
		{
			m_bRecomputePostureOnCollision = false;
		}
	}

	// get bounding limits, ignoring step-upable height
	bool bPerformCrouchTest = false;
	Vector mins;
	Vector maxs;
	if ( m_isUsingFullFeetTrace )
	{
		mins = body->GetHullMins();
	}
	else
	{
		mins = body->GetHullMins() + Vector( 0, 0, GetStepHeight() );
	}
	if ( !m_bRecomputePostureOnCollision )
	{
		maxs = body->GetHullMaxs();
		if ( mins.z >= maxs.z )
		{
			// if mins.z is greater than maxs.z, the engine will Assert 
			// in UTIL_TraceHull, and it won't work as advertised.
			mins.z = maxs.z - 2.0f;	
		}
	}
	else
	{
		const float halfSize = body->GetHullWidth() / 2.0f;
		maxs.Init( halfSize, halfSize, body->GetStandHullHeight() );
		bPerformCrouchTest = true;
	}

	trace_t trace;
	Vector desiredGoal = to;
	Vector resolvedGoal;
	IBody::PostureType nPosture = IBody::STAND;
	while( true )
	{
		bool bCollided = DetectCollision( &trace, recursionLimit, from, desiredGoal, mins, maxs );
		if ( !bCollided )
		{
			resolvedGoal = desiredGoal;
			break;
		}

		// If we hit really close to our target, then stop
		if ( !trace.startsolid && desiredGoal.DistToSqr( trace.endpos ) < 1.0f )
		{
			resolvedGoal = trace.endpos;
			break;
		}

		// Check for crouch test, if it's necessary
		// Don't bother about checking for crouch if we hit an actor 
		// Also don't bother checking for crouch if we hit a plane that pushes us upwards 
		if ( bPerformCrouchTest )
		{
			// Don't do this work twice
			bPerformCrouchTest = false;

			nPosture = body->GetDesiredPosture();

			if ( !trace.m_pEnt->MyNextBotPointer() && !trace.m_pEnt->IsPlayer() )
			{
				// Here, our standing trace hit the world or something non-breakable
				// If we're not currently crouching, then see if we could travel
				// the entire distance if we were crouched
				if ( nPosture != IBody::CROUCH )
				{
					trace_t crouchTrace;
					NextBotTraversableTraceFilter crouchFilter( GetBot(), ILocomotion::IMMEDIATELY );
					Vector vecCrouchMax( maxs.x, maxs.y, body->GetCrouchHullHeight() );
					TraceHull( from, desiredGoal, mins, vecCrouchMax, body->GetSolidMask(), &crouchFilter, &crouchTrace );
					if ( crouchTrace.fraction >= 1.0f && !crouchTrace.startsolid )
					{
						nPosture = IBody::CROUCH;
					}
				}
			}
			else if ( nPosture == IBody::CROUCH )
			{
				// Here, our standing trace hit an actor

				// NOTE: This test occurs almost never, based on my tests
				// Converts from crouch to stand in the case where the player
				// is currently crouching, *and* his first trace (with the standing hull)
				// hits an actor *and* if he didn't hit that actor, he could have
				// moved standing the entire way to his desired endpoint
				trace_t standTrace;
				NextBotTraversableTraceFilter standFilter( GetBot(), ILocomotion::IMMEDIATELY );
				TraceHull( from, desiredGoal, mins, maxs, body->GetSolidMask(), &standFilter, &standTrace );
				if ( standTrace.fraction >= 1.0f && !standTrace.startsolid )
				{
					nPosture = IBody::STAND;
				}
			}

			// Our first trace was based on the standing hull.
			// If we need be crouched, the trace was bogus; we need to do another
			if ( nPosture == IBody::CROUCH )
			{
				maxs.z = body->GetCrouchHullHeight();
				continue;
			}
		}

		if ( trace.startsolid )
		{
			// stuck inside solid; don't move

			if ( trace.m_pEnt && !trace.m_pEnt->IsWorld() )
			{
				// only ignore physics props that are not doors
				if ( dynamic_cast< CPhysicsProp * >( trace.m_pEnt ) != NULL && dynamic_cast< CBasePropDoor * >( trace.m_pEnt ) == NULL )
				{
					IPhysicsObject *physics = trace.m_pEnt->VPhysicsGetObject();
					if ( physics && physics->IsMoveable() )
					{
						// we've intersected a (likely moving) physics prop - ignore it for awhile so we can move out of it
						m_ignorePhysicsProp = trace.m_pEnt;
						m_ignorePhysicsPropTimer.Start( 1.0f );
					}
				}
			}

			// return to last known non-interpenetrating position
			resolvedGoal = m_lastValidPos;

			break;
		}
		
		if ( --recursionLimit <= 0 )
		{
			// reached recursion limit, no more adjusting allowed
			resolvedGoal = trace.endpos;
			break;
		}
		
		// never slide downwards/concave to avoid getting stuck in the ground
		if ( trace.plane.normal.z < 0.0f )
		{
			trace.plane.normal.z = 0.0f;
			trace.plane.normal.NormalizeInPlace();	
		}

		// slide off of surface we hit
		Vector fullMove = desiredGoal - from;
		Vector leftToMove = fullMove * ( 1.0f - trace.fraction );

		// obey climbing slope limit
		if ( !body->HasActivityType( IBody::MOTION_CONTROLLED_Z ) && 
			 trace.plane.normal.z < GetTraversableSlopeLimit() && 
			 fullMove.z > 0.0f )
		{
			fullMove.z = 0.0f;
			trace.plane.normal.z = 0.0f;
			trace.plane.normal.NormalizeInPlace();
		}

		float blocked = DotProduct( trace.plane.normal, leftToMove );

		Vector unconstrained = fullMove - blocked * trace.plane.normal;

		if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
		{
			NDebugOverlay::Line( trace.endpos, 
								 trace.endpos + 20.0f * trace.plane.normal, 
								 255, 0, 150, true, 15.0f );
		}

		// check for collisions along remainder of move
		// But don't bother if we're not going to deflect much
		Vector remainingMove = from + unconstrained;
		if ( remainingMove.DistToSqr( trace.endpos ) < 1.0f )
		{
			resolvedGoal = trace.endpos;
			break;
		}

		desiredGoal = remainingMove;
	}

	if ( !trace.startsolid )
	{
		m_lastValidPos = resolvedGoal;
	}

	if ( m_bRecomputePostureOnCollision )
	{
		m_bRecomputePostureOnCollision = false;

		if ( !body->IsActualPosture( nPosture ) )
		{
			body->SetDesiredPosture( nPosture );
		}
	}

	return resolvedGoal;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Collect the closest actors
 */
class ClosestActorsScan
{
public:
	ClosestActorsScan( const Vector &spot, int team, float maxRange = 0.0f, CBaseCombatCharacter *ignore = NULL )
	{
		m_spot = spot;
		m_team = team;
		m_close = NULL;
		
		if ( maxRange > 0.0f )
		{
			m_closeRangeSq = maxRange * maxRange;
		}
		else
		{
			m_closeRangeSq = 999999999.9f;
		}
		
		m_ignore = ignore;
	}
	
	bool operator() ( CBaseCombatCharacter *actor )
	{
		if (actor == m_ignore)
			return true;
			
		if (actor->IsAlive() && (m_team == TEAM_ANY || actor->GetTeamNumber() == m_team))
		{
			Vector to = actor->WorldSpaceCenter() - m_spot;
			float rangeSq = to.LengthSqr();
			if (rangeSq < m_closeRangeSq)
			{
				m_closeRangeSq = rangeSq;
				m_close = actor;
			}
		}
		return true;
	}
	
	CBaseCombatCharacter *GetActor( void ) const
	{
		return m_close;
	}
	
	bool IsCloserThan( float range )
	{
		return (m_closeRangeSq < (range * range));
	}

	bool IsFartherThan( float range )
	{
		return (m_closeRangeSq > (range * range));
	}
	
	Vector m_spot;
	int m_team;
	CBaseCombatCharacter *m_close;
	float m_closeRangeSq;
	CBaseCombatCharacter *m_ignore;
};


#ifdef SKIPME
//----------------------------------------------------------------------------------------------------------
/**
 * Push away zombies that are interpenetrating
 */
Vector NextBotGroundLocomotion::ResolveZombieCollisions( const Vector &pos )
{
	Vector adjustedNewPos = pos;

	Infected *me = m_nextBot->MyInfectedPointer();
	const float hullWidth = me->GetBodyInterface()->GetHullWidth();

	// only avoid if we're actually trying to move somewhere, and are enraged
	if ( me != NULL && !IsUsingLadder() && !IsClimbingOrJumping() && IsOnGround() && m_nextBot->IsAlive() && IsAttemptingToMove() /*&& GetBot()->GetBodyInterface()->IsArousal( IBody::INTENSE )*/ )
	{
		VPROF_BUDGET( "NextBotGroundLocomotion::ResolveZombieCollisions", "NextBot" );

		const CUtlVector< CHandle< Infected > > &neighbors = me->GetNeighbors();
		Vector avoid = vec3_origin;
		float avoidWeight = 0.0f;

		FOR_EACH_VEC( neighbors, it )
		{
			Infected *them = neighbors[ it ];

			if ( them )
			{
				Vector toThem = them->GetAbsOrigin() - me->GetAbsOrigin();
				toThem.z = 0.0f;

				float range = toThem.NormalizeInPlace();

				if ( range < hullWidth )
				{
					// these two infected are in contact
					me->Touch( them );

					// move out of contact
					float penetration = hullWidth - range;

					float weight = 1.0f + ( 2.0f * penetration/hullWidth );
					avoid += -weight * toThem;
					avoidWeight += weight;
				}
			}
		}

		if ( avoidWeight > 0.0f )
		{
			adjustedNewPos += 3.0f * ( avoid / avoidWeight );
		}
	}

	return adjustedNewPos;
}
#endif // _DEBUG


//----------------------------------------------------------------------------------------------------------
/**
 * Move to newPos, resolving any collisions along the way
 */
void NextBotGroundLocomotion::UpdatePosition( const Vector &newPos )
{
	VPROF_BUDGET( "NextBotGroundLocomotion::UpdatePosition", "NextBot" );

	if ( NextBotStop.GetBool() || (m_nextBot->GetFlags() & FL_FROZEN) != 0 || newPos == m_nextBot->GetPosition() )
	{
		return;
	}

	// avoid very nearby Actors to simulate "mushy" collisions between actors in contact with each other
	//Vector adjustedNewPos = ResolveZombieCollisions( newPos );
	Vector adjustedNewPos = newPos;

	// check for collisions during move and resolve them	
	const int recursionLimit = 3;
	Vector safePos = ResolveCollision( m_nextBot->GetPosition(), adjustedNewPos, recursionLimit );

	// set the bot's position
	if ( GetBot()->GetIntentionInterface()->IsPositionAllowed( GetBot(), safePos ) != ANSWER_NO )
	{
		m_nextBot->SetPosition( safePos );
	}
}


//----------------------------------------------------------------------------------------------------------
/** 
 * Prevent bot from sliding through floor, and snap to the ground if we're very near it
 */
void NextBotGroundLocomotion::UpdateGroundConstraint( void )
{
	VPROF_BUDGET( "NextBotGroundLocomotion::UpdateGroundConstraint", "NextBotExpensive" );

	// if we're up on the upward arc of our jump, don't interfere by snapping to ground
	// don't do ground constraint if we're climbing a ladder
	if ( DidJustJump() || IsAscendingOrDescendingLadder() )
	{
		m_isUsingFullFeetTrace = false;
		return;
	}
		
	IBody *body = GetBot()->GetBodyInterface();
	if ( body == NULL )
	{
		return;
	}

	float halfWidth = body->GetHullWidth()/2.0f;
	
	// since we only care about ground collisions, keep hull short to avoid issues with low ceilings
	/// @TODO: We need to also check actual hull height to avoid interpenetrating the world
	float hullHeight = GetStepHeight();
	
	// always need tolerance even when jumping/falling to make sure we detect ground penetration
	// must be at least step height to avoid 'falling' down stairs
	const float stickToGroundTolerance = GetStepHeight() + 0.01f;

	trace_t ground;
	NextBotTraceFilterIgnoreActors filter( m_nextBot, body->GetCollisionGroup() );

	TraceHull( m_nextBot->GetPosition() + Vector( 0, 0, GetStepHeight() + 0.001f ),
					m_nextBot->GetPosition() + Vector( 0, 0, -stickToGroundTolerance ), 
					Vector( -halfWidth, -halfWidth, 0 ), 
					Vector( halfWidth, halfWidth, hullHeight ), 
					body->GetSolidMask(), &filter, &ground );

	if ( ground.startsolid )
	{
		// we're inside the ground - bad news
		if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) && !( gpGlobals->framecount % 60 ) )
		{
			DevMsg( "%3.2f: Inside ground, ( %.0f, %.0f, %.0f )\n", gpGlobals->curtime, m_nextBot->GetPosition().x, m_nextBot->GetPosition().y, m_nextBot->GetPosition().z );
		}
		return;
	}

	if ( ground.fraction < 1.0f )
	{
		// there is ground below us
		m_groundNormal = ground.plane.normal;

		m_isUsingFullFeetTrace = false;
		
		// zero velocity normal to the ground
		float normalVel = DotProduct( m_groundNormal, m_velocity );
		m_velocity -= normalVel * m_groundNormal;
		
		// check slope limit
		if ( ground.plane.normal.z < GetTraversableSlopeLimit() )
		{
			// too steep to stand here

			// too steep to be ground - treat it like a wall hit
			if ( ( m_velocity.x * ground.plane.normal.x + m_velocity.y * ground.plane.normal.y ) <= 0.0f )
			{
				GetBot()->OnContact( ground.m_pEnt, &ground );			
			}
			
			// we're contacting some kind of ground
			// zero accelerations normal to the ground

			float normalAccel = DotProduct( m_groundNormal, m_acceleration );
			m_acceleration -= normalAccel * m_groundNormal;

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				DevMsg( "%3.2f: NextBotGroundLocomotion - Too steep to stand here\n", gpGlobals->curtime );
				NDebugOverlay::Line( GetFeet(), GetFeet() + 20.0f * ground.plane.normal, 255, 150, 0, true, 5.0f );
			}

			// clear out upward velocity so we don't walk up lightpoles
			m_velocity.z = MIN( 0, m_velocity.z );
			m_acceleration.z = MIN( 0, m_acceleration.z );

			return;
		}
		
		// inform other components of collision if we didn't land on the 'world'
		if ( ground.m_pEnt && !ground.m_pEnt->IsWorld() )
		{
			GetBot()->OnContact( ground.m_pEnt, &ground );
		}

		// snap us to the ground 
		m_nextBot->SetPosition( ground.endpos );

		if ( !IsOnGround() )
		{
			// just landed
			m_nextBot->SetGroundEntity( ground.m_pEnt );
			m_ground = ground.m_pEnt;

			// landing stops any jump in progress
			m_isJumping = false;
			m_isJumpingAcrossGap = false;

			GetBot()->OnLandOnGround( ground.m_pEnt );
		}
	}
	else
	{
		// not on the ground
		if ( IsOnGround() )
		{
			GetBot()->OnLeaveGround( m_nextBot->GetGroundEntity() );
			if ( !IsClimbingUpToLedge() && !IsJumpingAcrossGap() )
			{
				m_isUsingFullFeetTrace = true; // We're in the air and there's space below us, so use the full trace
				m_acceleration.z -= GetGravity(); // start our gravity now
			}
		}		
	}
}


//----------------------------------------------------------------------------------------------------------
/*
void NextBotGroundLocomotion::StandUp( void )
{
	// make sure there is room to stand
	trace_t result;
	const float halfSize = GetHullWidth()/3.0f;
	Vector standHullMin( -halfSize, -halfSize, GetStepHeight() + 0.1f );
	Vector standHullMax( halfSize, halfSize, GetStandHullHeight() );
	
	TraceHull( GetFeet(), GetFeet(), standHullMin, standHullMax, MASK_NPCSOLID, m_nextBot, MASK_DEFAULTPLAYERSOLID, &result );

	if ( result.fraction >= 1.0f && !result.startsolid )
	{
		m_isCrouching = false;
	}
}
*/


//----------------------------------------------------------------------------------------------------------
/**
 * Initiate a climb to an adjacent high ledge
 */
bool NextBotGroundLocomotion::ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle )
{
	return false;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Initiate a jump across an empty volume of space to far side
 */
void NextBotGroundLocomotion::JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward )
{
	// can only jump if we're on the ground
	if ( !IsOnGround() )
	{
		return;
	}

	IBody *body = GetBot()->GetBodyInterface();
	if ( !body->StartActivity( ACT_JUMP ) )
	{
		// body can't jump right now
		return;
	}
	

	// scale impulse to land on target
	Vector toGoal = landingGoal - GetFeet();
	
	// equation doesn't work if we're jumping upwards
	float height = toGoal.z;
	toGoal.z = 0.0f;
	
	float range = toGoal.NormalizeInPlace();

	// jump out at 45 degree angle
	const float cos45 = 0.7071f;
	
	// avoid division by zero
	if ( height > 0.9f * range )
	{
		height = 0.9f * range;
	}
	
	// ballistic equation to find initial velocity assuming 45 degree inclination and landing at give range and height
	float launchVel = ( range / cos45 ) / sqrt( ( 2.0f * ( range - height ) ) / GetGravity() );

	Vector up( 0, 0, 1 );
	Vector ahead = up + toGoal;	
	ahead.NormalizeInPlace();

	//m_velocity = cos45 * launchVel * ahead;
	m_velocity = launchVel * ahead;
	m_acceleration = vec3_origin;
			
	m_isJumping = true;
	m_isJumpingAcrossGap = true;
	m_isClimbingUpToLedge = false;

	GetBot()->OnLeaveGround( m_nextBot->GetGroundEntity() );
}


//----------------------------------------------------------------------------------------------------------
/**
 * Initiate a simple undirected jump in the air
 */
void NextBotGroundLocomotion::Jump( void )
{
	// can only jump if we're on the ground
	if ( !IsOnGround() )
	{
		return;
	}

	IBody *body = GetBot()->GetBodyInterface();
	if ( !body->StartActivity( ACT_JUMP ) )
	{
		// body can't jump right now
		return;
	}

	// jump straight up
	m_velocity.z = sqrt( 2.0f * GetGravity() * GetMaxJumpHeight() );
			
	m_isJumping = true;
	m_isClimbingUpToLedge = false;

	GetBot()->OnLeaveGround( m_nextBot->GetGroundEntity() );
}


//----------------------------------------------------------------------------------------------------------
/**
 * Set movement speed to running
 */
void NextBotGroundLocomotion::Run( void )
{
	m_desiredSpeed = GetRunSpeed();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Set movement speed to walking
 */
void NextBotGroundLocomotion::Walk( void )
{
	m_desiredSpeed = GetWalkSpeed();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Set movement speed to stopeed
 */
void NextBotGroundLocomotion::Stop( void )
{
	m_desiredSpeed = 0.0f;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return true if standing on something
 */
bool NextBotGroundLocomotion::IsOnGround( void ) const
{
	return (m_nextBot->GetGroundEntity() != NULL);
}


//----------------------------------------------------------------------------------------------------------
/**
 * Invoked when bot leaves ground for any reason
 */
void NextBotGroundLocomotion::OnLeaveGround( CBaseEntity *ground )
{
	m_nextBot->SetGroundEntity( NULL );
	m_ground = NULL;

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		DevMsg( "%3.2f: NextBotGroundLocomotion::OnLeaveGround\n", gpGlobals->curtime );
	}
}


//----------------------------------------------------------------------------------------------------------
/** 
 * Invoked when bot lands on the ground after being in the air
 */
void NextBotGroundLocomotion::OnLandOnGround( CBaseEntity *ground )
{
	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		DevMsg( "%3.2f: NextBotGroundLocomotion::GetBot()->OnLandOnGround\n", gpGlobals->curtime );
	}
}


//----------------------------------------------------------------------------------------------------------
/**
 * Get maximum speed bot can reach, regardless of desired speed
 */
float NextBotGroundLocomotion::GetSpeedLimit( void ) const
{
	// if we're crouched, move at reduced speed
	if ( !GetBot()->GetBodyInterface()->IsActualPosture( IBody::STAND ) )
	{
		return 0.75f * GetRunSpeed();
	}

	// no limit
	return 99999999.9f;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Climb the given ladder to the top and dismount
 */
void NextBotGroundLocomotion::ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal )
{
	// if we're already climbing this ladder, don't restart
	if ( m_ladder == ladder && m_isGoingUpLadder )
	{
		return;
	}
	
	m_ladder = ladder;
	m_ladderDismountGoal = dismountGoal;
	m_isGoingUpLadder = true;

	IBody *body = GetBot()->GetBodyInterface();
	if ( body )
	{
		// line them up to climb in XY
		Vector mountSpot = m_ladder->m_bottom + m_ladder->GetNormal() * (0.75f * body->GetHullWidth());
		mountSpot.z = GetBot()->GetPosition().z;
		
		UpdatePosition( mountSpot );
		
		body->StartActivity( ACT_CLIMB_UP, IBody::MOTION_CONTROLLED_Z );
	}
}


//----------------------------------------------------------------------------------------------------------
/**
 * Descend the given ladder to the bottom and dismount
 */
void NextBotGroundLocomotion::DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal )
{
	// if we're already descending this ladder, don't restart
	if ( m_ladder == ladder && !m_isGoingUpLadder )
	{
		return;
	}

	m_ladder = ladder;
	m_ladderDismountGoal = dismountGoal;
	m_isGoingUpLadder = false;

	IBody *body = GetBot()->GetBodyInterface();
	if ( body )
	{
		// line them up to climb in XY
		Vector mountSpot = m_ladder->m_top + m_ladder->GetNormal() * (0.75f * body->GetHullWidth());
		mountSpot.z = GetBot()->GetPosition().z;

		UpdatePosition( mountSpot );

		float ladderYaw = UTIL_VecToYaw( -m_ladder->GetNormal() );

		QAngle angles = m_nextBot->GetLocalAngles();
		angles.y = ladderYaw;
		
		m_nextBot->SetLocalAngles( angles );

		body->StartActivity( ACT_CLIMB_DOWN, IBody::MOTION_CONTROLLED_Z );
	}
}


//----------------------------------------------------------------------------------------------------------
bool NextBotGroundLocomotion::IsUsingLadder( void ) const
{
	return ( m_ladder != NULL );
}


//----------------------------------------------------------------------------------------------------------
/**
 * We are actually on the ladder right now, either climbing up or down
 */
bool NextBotGroundLocomotion::IsAscendingOrDescendingLadder( void ) const
{
	return IsUsingLadder();
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return position of "feet" - point below centroid of bot at feet level
 */
const Vector &NextBotGroundLocomotion::GetFeet( void ) const
{
	return m_nextBot->GetPosition();
}


//----------------------------------------------------------------------------------------------------------
const Vector & NextBotGroundLocomotion::GetAcceleration( void ) const
{
	return m_acceleration;
}


//----------------------------------------------------------------------------------------------------------
void NextBotGroundLocomotion::SetAcceleration( const Vector &accel )
{
	m_acceleration = accel;
}


//----------------------------------------------------------------------------------------------------------
void NextBotGroundLocomotion::SetVelocity( const Vector &vel )
{
	m_velocity = vel;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Return current world space velocity
 */
const Vector &NextBotGroundLocomotion::GetVelocity( void ) const
{
	return m_velocity;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Invoked when an bot reaches its MoveTo goal
 */
void NextBotGroundLocomotion::OnMoveToSuccess( const Path *path )
{
	// stop
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
}


//----------------------------------------------------------------------------------------------------------
/**
 * Invoked when an bot fails to reach a MoveTo goal
 */
void NextBotGroundLocomotion::OnMoveToFailure( const Path *path, MoveToFailureType reason )
{
	// stop
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
}


//----------------------------------------------------------------------------------------------------------
bool NextBotGroundLocomotion::DidJustJump( void ) const
{
	return IsClimbingOrJumping() && (m_nextBot->GetAbsVelocity().z > 0.0f);
}


//----------------------------------------------------------------------------------------------------------
/**
 * Rotate body to face towards "target"
 */
void NextBotGroundLocomotion::FaceTowards( const Vector &target )
{
	const float deltaT = GetUpdateInterval();
	
	QAngle angles = m_nextBot->GetLocalAngles();
	
	float desiredYaw = UTIL_VecToYaw( target - GetFeet() );

	float angleDiff = UTIL_AngleDiff( desiredYaw, angles.y );
	
	float deltaYaw = GetMaxYawRate() * deltaT;
	
	if (angleDiff < -deltaYaw)
	{
		angles.y -= deltaYaw;
	}
	else if (angleDiff > deltaYaw)
	{
		angles.y += deltaYaw;
	}
	else
	{
		angles.y += angleDiff;
	}
	
	m_nextBot->SetLocalAngles( angles );
}



