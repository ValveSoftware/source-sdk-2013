//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "animation.h"		// for NOMOTION

#include "ai_motor.h"
#include "ai_navigator.h"
#include "ai_basenpc.h"
#include "ai_localnavigator.h"
#include "ai_moveprobe.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef DEBUG
ConVar	ai_draw_motor_movement( "ai_draw_motor_movement","0" );
#endif

extern float	GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------

// Use these functions to set breakpoints to find out where movement is failing
#ifdef DEBUG
void DebugNoteMovementFailure()
{
}

// a place to put breakpoints
#pragma warning(push)
#pragma warning(disable:4189)
AIMoveResult_t DbgResult( AIMoveResult_t result )
{
	if ( result < AIMR_OK )
	{
		int breakHere = 1;
	}

	switch ( result )
	{
		case AIMR_BLOCKED_ENTITY:
			return AIMR_BLOCKED_ENTITY;
		case AIMR_BLOCKED_WORLD:
			return AIMR_BLOCKED_WORLD;
		case AIMR_BLOCKED_NPC:
			return AIMR_BLOCKED_NPC;
		case AIMR_ILLEGAL:
			return AIMR_ILLEGAL;
		case AIMR_OK:
			return AIMR_OK;
		case AIMR_CHANGE_TYPE:
			return AIMR_CHANGE_TYPE;
	};
	return AIMR_ILLEGAL;
};
#endif

//-----------------------------------------------------------------------------
//
// class CAI_Motor
//

BEGIN_SIMPLE_DATADESC( CAI_Motor )
	//							m_flMoveInterval	(think transient)
  	DEFINE_FIELD( m_IdealYaw,			FIELD_FLOAT ),
  	DEFINE_FIELD( m_YawSpeed,			FIELD_FLOAT ),
  	DEFINE_FIELD( m_vecVelocity,		FIELD_VECTOR ),
  	DEFINE_FIELD( m_vecAngularVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_nDismountSequence,	FIELD_INTEGER ),
	DEFINE_FIELD( m_vecDismount,		FIELD_VECTOR ),
	DEFINE_UTLVECTOR( m_facingQueue,	FIELD_EMBEDDED ), 
	DEFINE_FIELD( m_bYawLocked,			FIELD_BOOLEAN ),
	//							m_pMoveProbe
END_DATADESC()

//-----------------------------------------------------------------------------

CAI_Motor::CAI_Motor(CAI_BaseNPC *pOuter)
 :	CAI_Component( pOuter )
{
	m_flMoveInterval = 0;

	m_IdealYaw = 0;
	m_YawSpeed = 0;
	m_vecVelocity = Vector( 0, 0, 0 );
	m_pMoveProbe = NULL;
	m_bYawLocked = false;
}

//-----------------------------------------------------------------------------

CAI_Motor::~CAI_Motor() 
{
}

//-----------------------------------------------------------------------------

void CAI_Motor::Init( IAI_MovementSink *pMovementServices )	
{ 
	CAI_ProxyMovementSink::Init( pMovementServices );
	m_pMoveProbe = GetOuter()->GetMoveProbe(); // @TODO (toml 03-30-03): this is a "bad" way to grab this pointer. Components should have an explcit "init" phase.
}

//-----------------------------------------------------------------------------
// Step iteratively toward a destination position
//-----------------------------------------------------------------------------
AIMotorMoveResult_t CAI_Motor::MoveGroundStep( const Vector &newPos, CBaseEntity *pMoveTarget, float yaw, bool bAsFarAsCan, bool bTestZ, AIMoveTrace_t *pTraceResult )
{
	// By definition, this will produce different results than GroundMoveLimit() 
	// because there's no guarantee that it will step exactly one step 

	// See how far toward the new position we can step...
	// But don't actually test for ground geometric validity;
	// if it isn't valid, there's not much we can do about it
	AIMoveTrace_t moveTrace;
	unsigned testFlags = AITGM_IGNORE_FLOOR;

	if ( !bTestZ )
		testFlags |= AITGM_2D;

#ifdef DEBUG
	if ( ai_draw_motor_movement.GetBool() )
		testFlags |= AITGM_DRAW_RESULTS;
#endif

	GetMoveProbe()->TestGroundMove( GetLocalOrigin(), newPos, MASK_NPCSOLID, testFlags, &moveTrace );
	if ( pTraceResult )
	{
		*pTraceResult = moveTrace;
	}

	bool bHitTarget = (moveTrace.pObstruction && (pMoveTarget == moveTrace.pObstruction ));

	// Move forward either if there was no obstruction or if we're told to
	// move as far as we can, regardless
	bool bIsBlocked = IsMoveBlocked(moveTrace.fStatus);
	if ( !bIsBlocked || bAsFarAsCan || bHitTarget )
	{
#ifdef DEBUG
		if ( GetMoveProbe()->CheckStandPosition( GetLocalOrigin(), MASK_NPCSOLID ) && !GetMoveProbe()->CheckStandPosition( moveTrace.vEndPosition, MASK_NPCSOLID ) )
		{
			DevMsg( 2, "Warning: AI motor probably given invalid instructions\n" );
		}
#endif

		// The true argument here causes it to touch all triggers
		// in the volume swept from the previous position to the current position
		UTIL_SetOrigin(GetOuter(), moveTrace.vEndPosition, true);

		// check to see if our ground entity has changed
		// NOTE: This is to detect changes in ground entity as the movement code has optimized out
		// ground checks.  So now we have to do a simple recheck to make sure we detect when we've 
		// stepped onto a new entity.
		if ( GetOuter()->GetFlags() & FL_ONGROUND )
		{
			GetOuter()->PhysicsStepRecheckGround();
		}

		// skip tiny steps, but notify the shadow object of any large steps
		if ( moveTrace.flStepUpDistance > 0.1f )
		{
			float height = clamp( moveTrace.flStepUpDistance, 0.f, StepHeight() );
			IPhysicsObject *pPhysicsObject = GetOuter()->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				IPhysicsShadowController *pShadow = pPhysicsObject->GetShadowController();
				if ( pShadow )
				{
					pShadow->StepUp( height );
				}
			}
		}
		if ( yaw != -1 )
		{
			QAngle angles = GetLocalAngles();
			angles.y = yaw;
			SetLocalAngles( angles );
		}
		if ( bHitTarget )
			return AIM_PARTIAL_HIT_TARGET;
			
		if ( !bIsBlocked )
			return AIM_SUCCESS;
			
		if ( moveTrace.fStatus == AIMR_BLOCKED_NPC )
			return AIM_PARTIAL_HIT_NPC;

		return AIM_PARTIAL_HIT_WORLD;
	}
	return AIM_FAILED;
}


//-----------------------------------------------------------------------------
// Purpose:	Motion for climbing
// Input  :
// Output :	Returns bits (MoveStatus_b) regarding the move status
//-----------------------------------------------------------------------------

void CAI_Motor::MoveClimbStart(  const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw  )
{
	// @Note (toml 06-11-02): the following code is somewhat suspect. It
	// originated in CAI_BaseNPC::MoveClimb() from early June 2002
	// At the very least, state should be restored to original, not
	// slammed.
	//
	//	 -----Original Message-----
	//	From: 	Jay Stelly  
	//	Sent:	Monday, June 10, 2002 3:57 PM
	//	To:	Tom Leonard
	//	Subject:	RE: 
	//
	//	yes.
	//
	//	Also, there is some subtlety to using movetype.  I think in 
	//	general we want to keep things in MOVETYPE_STEP because it 
	//	implies a bunch of things in the external game physics 
	//	simulator.  There is a flag FL_FLY we use to 
	//	disable gravity on MOVETYPE_STEP characters.
	//
	//	>  -----Original Message-----
	//	> From: 	Tom Leonard  
	//	> Sent:	Monday, June 10, 2002 3:55 PM
	//	> To:	Jay Stelly
	//	> Subject:	
	//	> 
	//	> Should I worry at all that the following highlighted bits of 
	//	> code are not reciprocal for all state, and furthermore, stomp 
	//	> other state?

	if ( fabsf( climbDir.z ) < .1 )
	{
		SetActivity( GetNavigator()->GetMovementActivity() );
	}
	else
	{
		SetActivity( (climbDir.z > -0.01 ) ? ACT_CLIMB_UP : ACT_CLIMB_DOWN );
	}

	m_nDismountSequence = SelectWeightedSequence( ACT_CLIMB_DISMOUNT );
	if (m_nDismountSequence != ACT_INVALID)
	{
		GetOuter()->GetSequenceLinearMotion( m_nDismountSequence, &m_vecDismount );
	}
	else
	{
		m_vecDismount.Init();
	}

	GetOuter()->AddFlag( FL_FLY );		// No gravity
	SetSolid( SOLID_BBOX );
	SetGravity( 0.0 );
	SetGroundEntity( NULL );
}

AIMoveResult_t CAI_Motor::MoveClimbExecute( const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw, int climbNodesLeft )
{
	if ( fabsf( climbDir.z ) > .1 )
	{
		if ( GetActivity() != ACT_CLIMB_DISMOUNT )
		{
			Activity desiredActivity = (climbDir.z > -0.01 ) ? ACT_CLIMB_UP : ACT_CLIMB_DOWN;
			if ( GetActivity() != desiredActivity )
			{
				SetActivity( desiredActivity );
			}
		}

		if ( GetActivity() != ACT_CLIMB_UP && GetActivity() != ACT_CLIMB_DOWN && GetActivity() != ACT_CLIMB_DISMOUNT )
		{
			DevMsg( "Climber not in a climb activity!\n" );
			return AIMR_ILLEGAL;
		}

		if (m_nDismountSequence != ACT_INVALID)
		{
			if (GetActivity() == ACT_CLIMB_UP )
			{
				if (climbNodesLeft <= 2 && climbDist < fabs( m_vecDismount.z ))
				{
					// fixme: No other way to force m_nIdealSequence?
					GetOuter()->SetActivity( ACT_CLIMB_DISMOUNT );
					GetOuter()->SetCycle( GetOuter()->GetMovementFrame( m_vecDismount.z - climbDist ) );
				}
			}
		}
	}

	float climbSpeed = GetOuter()->GetInstantaneousVelocity();

	if (m_nDismountSequence != ACT_INVALID)
	{
		// catch situations where the climb mount/dismount finished before reaching goal
		climbSpeed = MAX( climbSpeed, 30.0 );
	}
	else
	{
		// FIXME: assume if they don't have a dismount animation then they probably don't really support climbing.
		climbSpeed = 100.0;
	}

	SetSmoothedVelocity( climbDir * climbSpeed );

	if ( climbDist < climbSpeed * GetMoveInterval() )
	{
		if (climbDist <= 1e-2)
			climbDist = 0;

		const float climbTime = climbDist / climbSpeed;
		
		SetMoveInterval( GetMoveInterval() - climbTime );
		SetLocalOrigin( climbDest );

		return AIMR_CHANGE_TYPE;
	}
	else
	{
		SetMoveInterval( 0 );
	}

	// --------------------------------------------
	// Turn to face the climb
	// --------------------------------------------
	SetIdealYawAndUpdate( yaw );

	return AIMR_OK;
}

void CAI_Motor::MoveClimbStop()
{
	if ( GetNavigator()->GetMovementActivity() > ACT_RESET )
		SetActivity( GetNavigator()->GetMovementActivity() );
	else
		SetActivity( ACT_IDLE );

	GetOuter()->RemoveFlag( FL_FLY );
	SetSmoothedVelocity( vec3_origin );
	SetGravity( 1.0 );
}

//-----------------------------------------------------------------------------
// Purpose:	Motion for jumping
// Input  :
// Output : Returns bits (MoveStatus_b) regarding the move status
//-----------------------------------------------------------------------------

void CAI_Motor::MoveJumpStart( const Vector &velocity )
{
	// take the npc off the ground and throw them in the air
	SetSmoothedVelocity( velocity );
	SetGravity( GetOuter()->GetJumpGravity() );
	SetGroundEntity( NULL );

	SetActivity( ACT_JUMP );

	SetIdealYawAndUpdate( velocity );
}

int CAI_Motor::MoveJumpExecute( )
{
	// needs to detect being hit
	UpdateYaw( );

	if (GetOuter()->GetActivity() == ACT_JUMP && GetOuter()->IsActivityFinished())
	{
		SetActivity( ACT_GLIDE );
	}

	// use all the time
	SetMoveInterval( 0 );

	return AIMR_OK;
}

AIMoveResult_t CAI_Motor::MoveJumpStop()
{
	SetSmoothedVelocity( Vector(0,0,0) );

	if (GetOuter()->GetActivity() == ACT_GLIDE)
	{
		float flTime = GetOuter()->GetGroundChangeTime();
		GetOuter()->AddStepDiscontinuity( flTime, GetAbsOrigin(), GetAbsAngles() );

		if ( SelectWeightedSequence( ACT_LAND ) == ACT_INVALID )
			return AIMR_CHANGE_TYPE;

		SetActivity( ACT_LAND );
		// FIXME: find out why the client doesn't interpolate immediatly after sequence change
		// GetOuter()->SetCycle( flTime - gpGlobals->curtime );
	}
	if (GetOuter()->GetActivity() != ACT_LAND || GetOuter()->IsActivityFinished())
	{
		return AIMR_CHANGE_TYPE;
	}

	SetMoveInterval( 0 );

	SetGravity( 1.0f );

	return AIMR_OK;
}

//-----------------------------------------------------------------------------

float CAI_Motor::GetIdealSpeed() const
{
	return GetOuter()->GetIdealSpeed();
}


float CAI_Motor::GetIdealAccel() const
{
	return GetOuter()->GetIdealAccel();
}

//-----------------------------------------------------------------------------

// how far will I go?
float CAI_Motor::MinStoppingDist( float flMinResult )
{
	// FIXME: should this be a constant rate or a constant time like it is now?
	float flDecelRate = GetIdealAccel();

	if (flDecelRate > 0.0)
	{
		// assuming linear deceleration, how long till my V hits 0?
		float t = GetCurSpeed() / flDecelRate;
		// and how far will I travel? (V * t - 1/2 A t^2)
		float flDist = GetCurSpeed() * t - 0.5 * flDecelRate * t * t;
	
		// this should always be some reasonable non-zero distance
		if (flDist > flMinResult)
			return flDist;
		return flMinResult;
	}
	return flMinResult;
}


//-----------------------------------------------------------------------------
// Purpose: how fast should I be going ideally
//-----------------------------------------------------------------------------
float CAI_Motor::IdealVelocity( void )
{
	// FIXME: this should be a per-entity setting so run speeds are not based on animation speeds
	return GetIdealSpeed() * GetPlaybackRate();
}

//-----------------------------------------------------------------------------

void CAI_Motor::ResetMoveCalculations()
{ 
}

//-----------------------------------------------------------------------------

void CAI_Motor::MoveStart()
{ 
}

//-----------------------------------------------------------------------------

void CAI_Motor::MoveStop()
{ 
	memset( &m_vecVelocity, 0, sizeof(m_vecVelocity) ); 
	GetOuter()->GetLocalNavigator()->ResetMoveCalculations();
}

//-----------------------------------------------------------------------------

void CAI_Motor::MovePaused()
{
}

//-----------------------------------------------------------------------------
// Purpose: what linear accel/decel rate do I need to hit V1 in d distance?
//-----------------------------------------------------------------------------
float DeltaV( float v0, float v1, float d )
{
	return 0.5 * (v1 * v1 - v0 * v0 ) / d;
}


//-----------------------------------------------------------------------------
float CAI_Motor::CalcIntervalMove()
{
	// assuming linear acceleration, how far will I travel?
	return 0.5 * (GetCurSpeed() + GetIdealSpeed()) * GetMoveInterval();
}

//-----------------------------------------------------------------------------
// Purpose: Move the npc to the next location on its route.
// Input  : vecDir - Normalized vector indicating the direction of movement.
//			flDistance - distance to move
//			flInterval - Time interval for this movement.
//			flGoalDistance - target distance
//			flGoalVelocity - target velocity
//-----------------------------------------------------------------------------

AIMotorMoveResult_t CAI_Motor::MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	// --------------------------------------------
	// turn in the direction of movement
	// --------------------------------------------
	MoveFacing( move );

	// --------------------------------------------
	return MoveGroundExecuteWalk( move, GetIdealSpeed(), CalcIntervalMove(), pTraceResult );
}


AIMotorMoveResult_t CAI_Motor::MoveGroundExecuteWalk( const AILocalMoveGoal_t &move, float speed, float dist, AIMoveTrace_t *pTraceResult )
{
	bool bReachingLocalGoal = ( dist > move.maxDist );

	// can I move farther in this interval than I'm supposed to?
	if ( bReachingLocalGoal )
	{
		if ( !(move.flags & AILMG_CONSUME_INTERVAL) )
		{
			// only use a portion of the time interval
			SetMoveInterval( GetMoveInterval() * (1 - move.maxDist / dist) );
		}
		else
			SetMoveInterval( 0 );
		dist = move.maxDist;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0 );
	}

	SetMoveVel( move.dir * speed );

	// --------------------------------------------
	// walk the distance
	// --------------------------------------------
	AIMotorMoveResult_t result = AIM_SUCCESS;
	if ( dist > 0.0 )
	{
		Vector vecFrom = GetLocalOrigin();
		Vector vecTo = vecFrom + move.dir * dist;
		
		result = MoveGroundStep( vecTo, move.pMoveTarget, -1, true, bReachingLocalGoal, pTraceResult );

		if ( result == AIM_FAILED )
			MoveStop();
	}
	else if ( !OnMoveStalled( move ) )
	{
		result = AIM_FAILED;
	}

	return result;
}


//-----------------------------------------------------------------------------
// Purpose: Move the npc to the next location on its route.
// Input  : pTargetEnt - 
//			vecDir - Normalized vector indicating the direction of movement.
//			flInterval - Time interval for this movement.
//-----------------------------------------------------------------------------

AIMotorMoveResult_t CAI_Motor::MoveFlyExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	// turn in the direction of movement
	MoveFacing( move );

	// calc accel/decel rates
	float flNewSpeed = GetIdealSpeed();
	SetMoveVel( move.dir * flNewSpeed );

	float flTotal = 0.5 * (GetCurSpeed() + flNewSpeed) * GetMoveInterval();

	float distance = move.maxDist;

	// can I move farther in this interval than I'm supposed to?
	if (flTotal > distance)
	{
		// only use a portion of the time interval
		SetMoveInterval( GetMoveInterval() * (1 - distance / flTotal) );
		flTotal = distance;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0 );
	}

	Vector vecStart, vecEnd;
	vecStart = GetLocalOrigin();
	VectorMA( vecStart, flTotal, move.dir, vecEnd );

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, vecStart, vecEnd, MASK_NPCSOLID, NULL, &moveTrace );
	if ( pTraceResult )
		*pTraceResult = moveTrace;
	
	// Check for total blockage
	if (fabs(moveTrace.flDistObstructed - flTotal) <= 1e-1)
	{
		// But if we bumped into our target, then we succeeded!
		if ( move.pMoveTarget && (moveTrace.pObstruction == move.pMoveTarget) )
			return AIM_PARTIAL_HIT_TARGET;

		return AIM_FAILED;
	}

	// The true argument here causes it to touch all triggers
	// in the volume swept from the previous position to the current position
	UTIL_SetOrigin(GetOuter(), moveTrace.vEndPosition, true);

	return (IsMoveBlocked(moveTrace.fStatus)) ? AIM_PARTIAL_HIT_WORLD : AIM_SUCCESS;
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------

void CAI_Motor::MoveFacing( const AILocalMoveGoal_t &move )
{
	if ( GetOuter()->OverrideMoveFacing( move, GetMoveInterval() ) )
		return;

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );

	int nSequence = GetSequence();
	float fSequenceMoveYaw = GetSequenceMoveYaw( nSequence );
	if ( fSequenceMoveYaw == NOMOTION )
	{
		fSequenceMoveYaw = 0;
	}

	if (!HasPoseParameter( nSequence, GetOuter()->LookupPoseMoveYaw() ))
	{
		SetIdealYawAndUpdate( UTIL_AngleMod( flMoveYaw - fSequenceMoveYaw ) );
	}
	else
	{
		// FIXME: move this up to navigator so that path goals can ignore these overrides.
		Vector dir;
		float flInfluence = GetFacingDirection( dir );
		dir = move.facing * (1 - flInfluence) + dir * flInfluence;
		VectorNormalize( dir );

		// ideal facing direction
		float idealYaw = UTIL_AngleMod( UTIL_VecToYaw( dir ) );
		
		// FIXME: facing has important max velocity issues
		SetIdealYawAndUpdate( idealYaw );	

		// find movement direction to compensate for not being turned far enough
		float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y );
		SetPoseParameter( GetOuter()->LookupPoseMoveYaw(), flDiff );
		/*
		if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			DevMsg( "move %.1f : diff %.1f  : ideal %.1f\n", flMoveYaw, flDiff, m_IdealYaw );
		}
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the ideal yaw and run the current or specified timestep 
//			worth of rotation.
//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawAndUpdate( float idealYaw, float yawSpeed)
{
	SetIdealYaw( idealYaw );
	if (yawSpeed == AI_CALC_YAW_SPEED)
		RecalculateYawSpeed();
	else if (yawSpeed != AI_KEEP_YAW_SPEED)
		SetYawSpeed( yawSpeed );
	UpdateYaw(-1);
}


//-----------------------------------------------------------------------------

void CAI_Motor::RecalculateYawSpeed() 
{ 
	SetYawSpeed( CalcYawSpeed() ); 
}

//-----------------------------------------------------------------------------

float AI_ClampYaw( float yawSpeedPerSec, float current, float target, float time )
{
	if (current != target)
	{
		float speed = yawSpeedPerSec * time;
		float move = target - current;

		if (target > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{// turning to the npc's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the npc's right
			if (move < -speed)
				move = -speed;
		}
		
		return UTIL_AngleMod(current + move);
	}
	
	return target;
}

//-----------------------------------------------------------------------------
// Purpose: Turns a npc towards its ideal yaw.
// Input  : yawSpeed - Yaw speed in degrees per 1/10th of a second.
//			flInterval - Time interval to turn, -1 uses time since last think.
// Output : Returns the number of degrees turned.
//-----------------------------------------------------------------------------
void CAI_Motor::UpdateYaw( int yawSpeed )
{
	// Don't do this if our yaw is locked
	if ( IsYawLocked() )
		return;

	GetOuter()->SetUpdatedYaw();

	float ideal, current, newYaw;
	
	if ( yawSpeed == -1 )
		yawSpeed = GetYawSpeed();

	// NOTE: GetIdealYaw() will never exactly be reached because UTIL_AngleMod
	// also truncates the angle to 16 bits of resolution. So lets truncate it here.
	current = UTIL_AngleMod( GetLocalAngles().y );
	ideal = UTIL_AngleMod( GetIdealYaw() );

	// FIXME: this needs a proper interval
	float dt = MIN( 0.2, gpGlobals->curtime - GetLastThink() );
	
	newYaw = AI_ClampYaw( (float)yawSpeed * 10.0, current, ideal, dt );
		
	if (newYaw != current)
	{
		QAngle angles = GetLocalAngles();
		angles.y = newYaw;
		SetLocalAngles( angles );
	}
}


//=========================================================
// DeltaIdealYaw - returns the difference ( in degrees ) between
// npc's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float CAI_Motor::DeltaIdealYaw ( void )
{
	float	flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );

	if ( flCurrentYaw == GetIdealYaw() )
	{
		return 0;
	}


	return UTIL_AngleDiff( GetIdealYaw(), flCurrentYaw );
}


//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawToTarget( const Vector &target, float noise, float offset )
{ 
	float base = CalcIdealYaw( target );
	base += offset;
	if ( noise > 0 )
	{
		noise *= 0.5;
		base += random->RandomFloat( -noise, noise );
		if ( base < 0 )
			base += 360;
		else if ( base >= 360 )
			base -= 360;
	}
	SetIdealYaw( base ); 
}

//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawToTargetAndUpdate( const Vector &target, float yawSpeed )
{ 
	SetIdealYawAndUpdate( CalcIdealYaw( target ), yawSpeed ); 
}


//-----------------------------------------------------------------------------
// Purpose: Keep track of multiple objects that the npc is interested in facing
//-----------------------------------------------------------------------------
void CAI_Motor::AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( pTarget, flImportance, flDuration, flRamp );
}


void CAI_Motor::AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( vecPosition, flImportance, flDuration, flRamp );
}

void CAI_Motor::AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( pTarget, vecPosition, flImportance, flDuration, flRamp );
}


float CAI_Motor::GetFacingDirection( Vector &vecDir )
{
	float flTotalInterest = 0.0;
	vecDir = Vector( 0, 0, 0 );

	int i;

	// clean up facing targets
	for (i = 0; i < m_facingQueue.Count();)
	{
		if (!m_facingQueue[i].IsActive())
		{
			m_facingQueue.Remove( i );
		}
		else
		{
			i++;
		}
	}

	for (i = 0; i < m_facingQueue.Count(); i++)
	{
		float flInterest = m_facingQueue[i].Interest( );
		Vector tmp = m_facingQueue[i].GetPosition() - GetAbsOrigin();

		// NDebugOverlay::Line( m_facingQueue[i].GetPosition(), GetAbsOrigin(), 255, 0, 0, false, 0.1 );

		VectorNormalize( tmp );

		vecDir = vecDir * (1 - flInterest) + tmp * flInterest;

		flTotalInterest = (1 - (1 - flTotalInterest) * (1 - flInterest));

		VectorNormalize( vecDir );
	}

	return flTotalInterest;
}


//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Motor::MoveNormalExecute( const AILocalMoveGoal_t &move )
{
	AI_PROFILE_SCOPE(CAI_Motor_MoveNormalExecute);
	
	// --------------------------------

	AIMotorMoveResult_t fMotorResult;
	AIMoveTrace_t 		moveTrace;
	
	if ( move.navType == NAV_GROUND )
	{
		fMotorResult = MoveGroundExecute( move, &moveTrace );
	}
	else
	{
		Assert( move.navType == NAV_FLY );
		fMotorResult = MoveFlyExecute( move, &moveTrace );
	}

	static AIMoveResult_t moveResults[] = 
	{
		AIMR_ILLEGAL,	                         // AIM_FAILED
		AIMR_OK,                                 // AIM_SUCCESS
		AIMR_BLOCKED_NPC,						 // AIM_PARTIAL_HIT_NPC
		AIMR_BLOCKED_WORLD,                      // AIM_PARTIAL_HIT_WORLD
		AIMR_BLOCKED_WORLD,                      // AIM_PARTIAL_HIT_TARGET
	};
	Assert( ARRAYSIZE( moveResults ) == AIM_NUM_RESULTS && fMotorResult >= 0 && fMotorResult <= ARRAYSIZE( moveResults ) );
	
	AIMoveResult_t result = moveResults[fMotorResult];
	
	if ( result != AIMR_OK )
	{
		OnMoveExecuteFailed( move, moveTrace, fMotorResult, &result );
		SetMoveInterval( 0 ); // always consume interval on failure, even if overridden by OnMoveExecuteFailed()
	}
	
	return DbgResult( result );
}

//-----------------------------------------------------------------------------
// Purpose: Look ahead my stopping distance, or at least my hull width
//-----------------------------------------------------------------------------
float CAI_Motor::MinCheckDist( void )
{
	// Take the groundspeed into account
	float flMoveDist = GetMoveInterval() * GetIdealSpeed();
	float flMinDist = MAX( MinStoppingDist(), flMoveDist);
	if ( flMinDist < GetHullWidth() )
		flMinDist = GetHullWidth();
	return flMinDist;
}

//-----------------------------------------------------------------------------

CAI_Navigator *CAI_Motor::GetNavigator( void )
{
	return GetOuter()->GetNavigator();
}
							
int CAI_Motor::SelectWeightedSequence ( Activity activity )
{
	return GetOuter()->SelectWeightedSequence ( activity );
}

float	CAI_Motor::GetSequenceGroundSpeed( int iSequence )
{
	return GetOuter()->GetSequenceGroundSpeed( iSequence );
}


//-----------------------------------------------------------------------------

void CAI_Motor::SetSmoothedVelocity(const Vector &vecVelocity)
{
	GetOuter()->SetAbsVelocity(vecVelocity);
}

Vector CAI_Motor::GetSmoothedVelocity()
{
	return GetOuter()->GetSmoothedVelocity();
}

float CAI_Motor::StepHeight() const
{
	return GetOuter()->StepHeight();
}

bool CAI_Motor::CanStandOn( CBaseEntity *pSurface ) const
{
	return GetOuter()->CanStandOn( pSurface );
}

float CAI_Motor::CalcIdealYaw( const Vector &vecTarget )
{
	return GetOuter()->CalcIdealYaw( vecTarget );
}

float CAI_Motor::SetBoneController( int iController, float flValue )
{
	return GetOuter()->SetBoneController( iController, flValue );
}

float CAI_Motor::GetSequenceMoveYaw( int iSequence )
{
	return GetOuter()->GetSequenceMoveYaw( iSequence );
}

void CAI_Motor::SetPlaybackRate( float flRate )
{
	return GetOuter()->SetPlaybackRate( flRate );
}

float CAI_Motor::GetPlaybackRate()
{
	return GetOuter()->GetPlaybackRate();
}

float CAI_Motor::SetPoseParameter( const char *szName, float flValue )
{
	return GetOuter()->SetPoseParameter( szName, flValue );
}

float CAI_Motor::GetPoseParameter( const char *szName )
{
	return GetOuter()->GetPoseParameter( szName );
}

bool CAI_Motor::HasPoseParameter( int iSequence, const char *szName )
{
	return GetOuter()->HasPoseParameter( iSequence, szName );
}

float CAI_Motor::SetPoseParameter( int iParameter, float flValue ) 
{ 
	return GetOuter()->SetPoseParameter( iParameter, flValue );  
}

bool CAI_Motor::HasPoseParameter( int iSequence, int iParameter ) 
{ 
	return GetOuter()->HasPoseParameter( iSequence, iParameter ); 
}

void CAI_Motor::SetMoveType( MoveType_t val, MoveCollide_t moveCollide )
{
	GetOuter()->SetMoveType( val, moveCollide );
}

//=============================================================================

