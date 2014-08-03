//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Behavior for NPCs riding in cars (with boys)
//
//=============================================================================

#include "cbase.h"
#include "ai_playerally.h"
#include "ai_motor.h"
#include "bone_setup.h"
#include "vehicle_base.h"
#include "entityblocker.h"
#include "ai_behavior_passenger.h"
#include "ai_pathfinder.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_moveprobe.h"
#include "env_debughistory.h"

// Custom activities
int ACT_PASSENGER_IDLE;
int	ACT_PASSENGER_RANGE_ATTACK1;

ConVar passenger_debug_transition( "passenger_debug_transition", "0" );
ConVar passenger_impact_response_threshold( "passenger_impact_response_threshold", "-350.0" );

#define ORIGIN_KEYNAME "origin"
#define ANGLES_KEYNAME "angles"

BEGIN_DATADESC( CAI_PassengerBehavior )

	DEFINE_EMBEDDED( m_vehicleState ),
	DEFINE_FIELD( m_bEnabled,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_PassengerIntent,	FIELD_INTEGER ),
	DEFINE_FIELD( m_PassengerState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hVehicle,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBlocker,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecTargetPosition,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecTargetAngles,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flOriginStartFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_flOriginEndFrame,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flAnglesStartFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAnglesEndFrame,	FIELD_FLOAT ),
	DEFINE_FIELD( m_nTransitionSequence,FIELD_INTEGER ),

END_DATADESC();

BEGIN_SIMPLE_DATADESC( passengerVehicleState_t )

	DEFINE_FIELD( m_bWasBoosting,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWasOverturned,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecLastLocalVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( m_vecDeltaVelocity,		FIELD_VECTOR ),
	DEFINE_FIELD( m_vecLastAngles,			FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextWarningTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flLastSpeedSqr,			FIELD_FLOAT ),
	DEFINE_FIELD( m_bPlayerInVehicle,		FIELD_BOOLEAN ),

END_DATADESC();

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CAI_PassengerBehavior::CAI_PassengerBehavior( void ) : 
m_bEnabled( false ), 
m_hVehicle( NULL ), 
m_PassengerState( PASSENGER_STATE_OUTSIDE ), 
m_PassengerIntent( PASSENGER_INTENT_NONE ),
m_nTransitionSequence( -1 )
{
}

//-----------------------------------------------------------------------------
// Purpose: Enables the behavior to run
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::Enable( CPropJeepEpisodic *pVehicle, bool bImmediateEnter /*= false*/ ) 
{ 
	if ( m_bEnabled && m_hVehicle.Get() )
		return;

	m_bEnabled = true; 
	m_hVehicle = pVehicle;
	SetPassengerState( PASSENGER_STATE_OUTSIDE );

	// Init our starting information about the vehicle
	InitVehicleState();
}

void CAI_PassengerBehavior::OnRestore()
{
	if ( m_bEnabled && !m_hVehicle.Get() )
	{
		Disable();
	}
	
	BaseClass::OnRestore();
}


//-----------------------------------------------------------------------------
// Purpose: Stops the behavior from being run
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::Disable( void )
{
	m_bEnabled = false;
	m_hVehicle = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the process of entering a vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::EnterVehicle( void )
{
	// If we're already in the vehicle, simply cancel out our intents
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE || GetPassengerState() == PASSENGER_STATE_ENTERING )
	{
		// Clear out an exit
		if ( m_PassengerIntent == PASSENGER_INTENT_EXIT )
		{
			m_PassengerIntent = PASSENGER_INTENT_NONE;
			ClearCondition( COND_PASSENGER_ENTERING );
			ClearCondition( COND_PASSENGER_EXITING );
		}

		return;
	}

	// Update our internal state
	m_PassengerIntent = PASSENGER_INTENT_ENTER;

	// Otherwise set this condition and go!
	SetCondition( COND_PASSENGER_ENTERING );
}

//-----------------------------------------------------------------------------
// Purpose: Starts the process of exiting a vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::ExitVehicle( void )
{
	// Must be in the seat
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE || GetPassengerState() == PASSENGER_STATE_EXITING )
	{
		// Clear out an entrance
		if ( m_PassengerIntent == PASSENGER_INTENT_ENTER )
		{
			m_PassengerIntent = PASSENGER_INTENT_NONE;
			SetCondition( COND_PASSENGER_CANCEL_ENTER );
			ClearCondition( COND_PASSENGER_ENTERING );
			ClearCondition( COND_PASSENGER_EXITING );
		}
		return;
	}

	// Update our internal state
	m_PassengerIntent = PASSENGER_INTENT_EXIT;

	//
	// Everything below this point will still attempt to exit the vehicle, once able
	//

	// Must have a valid vehicle
	if ( m_hVehicle == NULL )
		return;

	// Cannot exit while we're upside down
	if ( m_hVehicle->IsOverturned() )
		return;

	// Interrupt what we're doing
	SetCondition( COND_PASSENGER_EXITING );
}

//-----------------------------------------------------------------------------
// Purpose: FIXME - This should move into something a bit more flexible
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::AddPhysicsPush( float force )
{
	/*
	// Kick the vehicle so the player knows we've arrived
	Vector impulse = m_hVehicle->GetAbsOrigin() - GetOuter()->GetAbsOrigin();
	VectorNormalize( impulse );
	impulse.z = -0.75;
	VectorNormalize( impulse );
	Vector vecForce = impulse * force;
	
	m_hVehicle->VPhysicsGetObject()->ApplyForceOffset( vecForce, GetOuter()->GetAbsOrigin() );
	*/

	Vector vecDir;

	IPhysicsObject *pObject = GetOuter()->VPhysicsGetObject();
	Vector vecVelocity;
	pObject->GetVelocity( &vecVelocity, NULL );
	GetOuter()->GetVectors( NULL, NULL, &vecDir );
	vecDir.Negate();

	Vector vecForce = vecDir * force;

	m_hVehicle->VPhysicsGetObject()->ApplyForceOffset( vecForce, GetOuter()->GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::IsPassengerHostile( void )
{
	CBaseEntity *pPlayer = AI_GetSinglePlayer();
	
	// If the player hates or fears the passenger, they're hostile
	if ( GetOuter()->IRelationType( pPlayer ) == D_HT || GetOuter()->IRelationType( pPlayer ) == D_FR )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::InitVehicleState( void )
{
	// Set the player's state
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	m_vehicleState.m_bPlayerInVehicle = ( pPlayer && pPlayer->IsInAVehicle() && pPlayer->GetServerVehicle() == m_hVehicle->GetServerVehicle() );

	// Update our vehicle state so we don't confuse our previous velocity on the first frame!
	m_vehicleState.m_bWasBoosting = false;
	m_vehicleState.m_bWasOverturned = false;
	m_vehicleState.m_flNextWarningTime = 0.0f;
	m_vehicleState.m_vecDeltaVelocity = vec3_origin;
	m_vehicleState.m_flNextWarningTime = gpGlobals->curtime;
	m_vehicleState.m_vecLastAngles = m_hVehicle->GetAbsAngles();

	Vector	localVelocity;
	GetLocalVehicleVelocity( &m_vehicleState.m_vecLastLocalVelocity );

	m_vehicleState.m_flLastSpeedSqr = localVelocity.LengthSqr();
}

//-----------------------------------------------------------------------------
// Purpose: Puts the NPC in hierarchy with the vehicle and makes them intangible
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::FinishEnterVehicle( void )
{
	if ( m_hVehicle == NULL )
		return;

	// Get the ultimate position we want to be in
	Vector vecFinalPos;
	QAngle vecFinalAngles;
	GetEntryTarget( &vecFinalPos, &vecFinalAngles );

	// Make sure we're exactly where we need to be
	GetOuter()->SetLocalOrigin( vecFinalPos );
	GetOuter()->SetLocalAngles( vecFinalAngles );
	GetOuter()->SetMoveType( MOVETYPE_NONE );
	GetOuter()->GetMotor()->SetYawLocked( true );

	// We're now riding inside the vehicle
	SetPassengerState( PASSENGER_STATE_INSIDE );
	
	// If we've not been told to leave immediately, we're done
	if ( m_PassengerIntent == PASSENGER_INTENT_ENTER )
	{
		m_PassengerIntent = PASSENGER_INTENT_NONE;
	}
	
	// Tell the vehicle we've succeeded 
	m_hVehicle->NPC_FinishedEnterVehicle( GetOuter(), (IsPassengerHostile()==false) );
}

//-----------------------------------------------------------------------------
// Purpose: Removes the NPC from the car
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::FinishExitVehicle( void )
{
	if ( m_hVehicle == NULL )
		return;

	// Destroy the blocker
	if ( m_hBlocker != NULL )
	{
		UTIL_Remove( m_hBlocker );
		m_hBlocker = NULL;
	}

	// To do this, we need to be very sure we're in a good spot
	GetOuter()->SetCondition( COND_PROVOKED );
	GetOuter()->SetMoveType( MOVETYPE_STEP );
	GetOuter()->RemoveFlag( FL_FLY );
	GetOuter()->GetMotor()->SetYawLocked( false );

	// Re-enable the physical collisions for this NPC
	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if ( pPhysObj != NULL )
	{
		pPhysObj->EnableCollisions( true );
	}

	m_hVehicle->NPC_RemovePassenger( GetOuter() );
	m_hVehicle->NPC_FinishedExitVehicle( GetOuter(), (IsPassengerHostile()==false) );

	SetPassengerState( PASSENGER_STATE_OUTSIDE );

	// Stop our custom move sequence
	GetOuter()->m_iszSceneCustomMoveSeq = NULL_STRING;

	// If we've not been told to enter immediately, we're done
	if ( m_PassengerIntent == PASSENGER_INTENT_EXIT )
	{
		m_PassengerIntent = PASSENGER_INTENT_NONE;
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Build our custom interrupt cases for the behavior
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::BuildScheduleTestBits( void )
{
	// Always interrupt when we need to get in or out
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE || GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_ENTERING ) );
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_EXITING ) );
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_CANCEL_ENTER ) );
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose: Dictates whether or not the behavior is active and working
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::CanSelectSchedule( void )
{
	return m_bEnabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::CanExitVehicle( void )
{
	// Vehicle must not be overturned
	if ( m_hVehicle->IsOverturned() )
		return false;

	// Vehicle must be at rest
	Vector vecVelocity;
	m_hVehicle->GetVelocity( &vecVelocity, NULL );	
	if ( vecVelocity.LengthSqr() > Square( 8.0f ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Handles passengers deciding to enter or exit the vehicle
// Output : int
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::SelectTransitionSchedule( void )
{
	// Handle our various states
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Exiting schedule
		if ( HasCondition( COND_PASSENGER_EXITING ) || m_PassengerIntent == PASSENGER_INTENT_EXIT )
		{
			if ( CanExitVehicle( ) )
			{
				ClearCondition( COND_PASSENGER_EXITING );
				return SCHED_PASSENGER_EXIT_VEHICLE;
			}
		}
	}
	else if ( GetPassengerState() == PASSENGER_STATE_ENTERING || GetPassengerState() == PASSENGER_STATE_EXITING )
	{
		// The following code attempts to fix up a passenger being interrupted mid-transition
		Warning( "SelectSchedule() called on transitioning passenger!\n" );
		ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs( "%s(%d):  SelectSchedule() called on transitioning passenger!\n", GetOuter()->GetDebugName(), GetOuter()->entindex() ) );
		Assert( 0 );

		// Correct this issue immediately
		if ( GetPassengerState() == PASSENGER_STATE_EXITING )
		{
			// Force them out of the vehicle to where they want to be
			// The teleport function is overridden for passengers, meaning that they will clean up properly when called to do so
			GetOuter()->Teleport( &m_vecTargetPosition, &m_vecTargetAngles, NULL );
		}
		else if ( GetPassengerState() == PASSENGER_STATE_ENTERING )
		{
			// Force them into the proper position
			GetOuter()->SetLocalOrigin( m_vecTargetPosition );
			GetOuter()->SetLocalAngles( m_vecTargetAngles ); 
			FinishEnterVehicle();
		}

		// Stop playing our animation
		SetActivity( ACT_RESET );
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the schedule selection
// Output : int - Schedule to play
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::SelectSchedule( void )
{
	// Protect from this rare occurrence happening
	if ( m_hVehicle == NULL )
	{
		Assert( m_hVehicle != NULL );
		Warning( "Entity %s running passenger behavior without a valid vehicle!\n", GetName() );
		
		Disable();
		return BaseClass::SelectSchedule();
	}

	// See if we're transitioning in / out of the vehicle
	int nSchedule = SelectTransitionSchedule();
	if ( nSchedule != SCHED_NONE )
		return nSchedule;

	return SCHED_PASSENGER_IDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	switch( failedTask )
	{
	// For now, just sit back down
	case TASK_PASSENGER_DETACH_FROM_VEHICLE:
		return SCHED_PASSENGER_IDLE;
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: Finds a ground position at a given location with some delta up and down to check
// Input  : &in - position to check at
//			delta - amount of distance up and down to check
//			*out - ground position
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::FindGroundAtPosition( const Vector &in, float flUpDelta, float flDownDelta, Vector *out )
{
	Vector startPos = in + Vector( 0, 0, flUpDelta );	// Look up by delta
	Vector endPos   = in - Vector( 0, 0, flDownDelta );	// Look down by delta
	Vector hullMin  = GetOuter()->GetHullMins();
	Vector hullMax  = GetOuter()->GetHullMaxs();

	// Ignore ourself and the vehicle we're referencing
	CTraceFilterVehicleTransition ignoreFilter( m_hVehicle, GetOuter(), COLLISION_GROUP_NONE );

	trace_t tr;
	UTIL_TraceHull( startPos, endPos, hullMin, hullMax, MASK_NPCSOLID, &ignoreFilter, &tr );

	// Must not have ended up in solid space
	if ( tr.allsolid )
	{
		// Debug
		if ( passenger_debug_transition.GetBool() )
		{
			NDebugOverlay::SweptBox( tr.startpos, tr.endpos, hullMin, hullMax, vec3_angle, 255, 255, 0, 255, 1.0f );
		}
		
		return false;
	}

	// Must have ended up with feet on the ground
	if ( tr.DidHitWorld() || ( tr.m_pEnt && tr.m_pEnt->IsStandable() ) )
	{
		// Debug
		if ( passenger_debug_transition.GetBool() )
		{
			NDebugOverlay::SweptBox( tr.startpos, tr.endpos, hullMin, hullMax, vec3_angle, 0, 255, 0, 255, 1.0f );
		}
		
		*out = tr.endpos;
		return true;
	}

	// Ended up in the air
	if ( passenger_debug_transition.GetBool() )
	{
		NDebugOverlay::SweptBox( tr.startpos, tr.endpos, hullMin, hullMax, vec3_angle, 255, 0, 0, 255, 1.0f );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to verify that a test position is on the node graph
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::PointIsNavigable( const Vector &vecTargetPos )
{
	// Attempt to local move between this point and the nearest node, ignoring anything but world architecture
	AIMoveTrace_t moveTrace;
	int iNearestNode = GetOuter()->GetPathfinder()->NearestNodeToPoint( vecTargetPos );
	if ( iNearestNode != NO_NODE )
	{
		// Try a movement trace between the test position and the node
		GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, 
			g_pBigAINet->GetNodePosition(GetOuter()->GetHullType(), iNearestNode ), 
			vecTargetPos, 
			MASK_SOLID_BRUSHONLY, 
			NULL, 
			0, 
			&moveTrace );
		
		// See if we got close enough to call it arrived
		if ( ( moveTrace.vEndPosition - vecTargetPos ).LengthSqr() < Square( GetHullWidth() ) &&
			GetOuter()->GetMoveProbe()->CheckStandPosition( moveTrace.vEndPosition, MASK_SOLID_BRUSHONLY ) )
		{
			// NDebugOverlay::HorzArrow( vecTargetPos, moveTrace.vEndPosition, 8.0f, 255, 0, 0, 16, true, 4.0f );
			// NDebugOverlay::HorzArrow( vecTargetPos, g_pBigAINet->GetNodePosition(GetOuter()->GetHullType(), iNearestNode ), 8.0f, 255, 255, 0, 16, true, 4.0f );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the exit point for the passenger (on the ground)
// Input  : &vecOut - position the entity should be at when finished exiting
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::GetExitPoint( int nSequence, Vector *vecExitPoint, QAngle *vecExitAngles )
{
	bool bSucceeded = true;

	// Get the delta to the final position as will be dictated by this animation's auto movement
	Vector vecDeltaPos;
	QAngle vecDeltaAngles;
	GetOuter()->GetSequenceMovement( nSequence, 0.0f, 1.0f, vecDeltaPos, vecDeltaAngles );

	// Rotate the delta position by our starting angles
	Vector vecRotPos = vecDeltaPos;
	VectorRotate( vecRotPos, GetOuter()->GetAbsAngles(), vecDeltaPos );

	if ( vecExitPoint != NULL )
	{
		float flDownDelta = 64.0f;
		float flUpDelta = 16.0f;
		Vector vecGroundPos;
		
		bool bFoundGround = FindGroundAtPosition( GetOuter()->GetAbsOrigin() + vecDeltaPos, flUpDelta, flDownDelta, &vecGroundPos );
		if ( bFoundGround )
		{
			if ( PointIsNavigable( vecGroundPos ) == false )
			{
				bSucceeded = false;
			}
		}
		else
		{
			bSucceeded = false;
		}

		*vecExitPoint = vecGroundPos;
	}

	if ( vecExitAngles != NULL )
	{
		QAngle newAngles = GetOuter()->GetAbsAngles() + vecDeltaAngles;
		newAngles.x = UTIL_AngleMod( newAngles.x );
		newAngles.y = UTIL_AngleMod( newAngles.y );
		newAngles.z = UTIL_AngleMod( newAngles.z );
		*vecExitAngles = newAngles;
	}

	return bSucceeded;
}

//-----------------------------------------------------------------------------
// Purpose: Reserve our entry point
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::ReserveEntryPoint( VehicleSeatQuery_e eSeatSearchType )
{
	// FIXME: Move all this logic into the NPC_EnterVehicle function?
	// Find any seat to get into
	int nSeatID = m_hVehicle->GetServerVehicle()->NPC_GetAvailableSeat( GetOuter(), GetRoleName(), eSeatSearchType );
	if ( nSeatID != VEHICLE_SEAT_INVALID )
		return m_hVehicle->NPC_AddPassenger( GetOuter(), GetRoleName(), nSeatID );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the NPC can move between a start and end position of a transition
// Input  : &vecStartPos - start position
//			&vecEndPos - end position
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::IsValidTransitionPoint( const Vector &vecStartPos, const Vector &vecEndPos )
{
	// Now sweep a hull through space to see if we can validly exit there
	Vector vecHullMins = GetOuter()->GetHullMins() + Vector( 0, 0, GetOuter()->StepHeight()*2.0f );
	Vector vecHullMaxs = GetOuter()->GetHullMaxs() - Vector( 0, 0, GetOuter()->StepHeight() );

	trace_t tr;
	CTraceFilterVehicleTransition skipFilter( GetOuter(), m_hVehicle, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecStartPos, vecEndPos, vecHullMins, vecHullMaxs, MASK_NPCSOLID, &skipFilter, &tr );

	// If we're blocked, we can't get out there
	if ( tr.fraction < 1.0f || tr.allsolid )
	{
		if ( passenger_debug_transition.GetBool() )
		{
			NDebugOverlay::SweptBox( vecStartPos, vecEndPos, vecHullMins, vecHullMaxs, vec3_angle, 255, 0, 0, 64, 2.0f );
		}
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the proper sequence to use (weighted by priority or distance from current position)
//			to enter the vehicle.
// Input  : bNearest - Use distance as the criteria for a "best" sequence.  Otherwise the order of the
//					   seats is their priority.
// Output : int - sequence index
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::FindEntrySequence( bool bNearest /*= false*/ )
{
	// Get a list of all our animations
	const PassengerSeatAnims_t *pEntryAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_ENTRY );
	if ( pEntryAnims == NULL )
		return -1;

	// Get the ultimate position we'll end up at
	Vector	vecStartPos, vecEndPos;
	if ( m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPosition( GetOuter(), &vecEndPos, NULL ) == false )
		return -1;

	const CPassengerSeatTransition *pTransition;
	Vector	vecSeatDir;
	float	flNearestDist = FLT_MAX;
	float	flSeatDist;
	int		nNearestSequence = -1;
	int		nSequence;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pEntryAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		pTransition = &pEntryAnims->Element(i);
		nSequence = GetOuter()->LookupSequence( STRING( pTransition->GetAnimationName() ) );
		if ( nSequence == -1 )
			continue;

		// Test this entry for validity
		if ( GetEntryPoint( nSequence, &vecStartPos ) == false )
			continue;

		// Check to see if we can use this
		if ( IsValidTransitionPoint( vecStartPos, vecEndPos ) )
		{
			// If we're just looking for the first, we're done
			if ( bNearest == false )
				return nSequence;

			// Otherwise distance is the deciding factor
			vecSeatDir = ( vecStartPos - GetOuter()->GetAbsOrigin() );
			flSeatDist = VectorNormalize( vecSeatDir );

			// Closer, take it
			if ( flSeatDist < flNearestDist )
			{
				flNearestDist = flSeatDist;
				nNearestSequence = nSequence;
			}
		}
			
	}

	return nNearestSequence;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::FindExitSequence( void )
{
	// Get a list of all our animations
	const PassengerSeatAnims_t *pExitAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_EXIT );
	if ( pExitAnims == NULL )
		return -1;

	// Get the ultimate position we'll end up at
	Vector	vecStartPos, vecEndPos;
	if ( m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPosition( GetOuter(), &vecStartPos, NULL ) == false )
		return -1;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pExitAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		int nSequence = GetOuter()->LookupSequence( STRING( pExitAnims->Element(i).GetAnimationName() ) );
		if ( nSequence == -1 )
			continue;

		// Test this entry for validity
		if ( GetExitPoint( nSequence, &vecEndPos ) == false )
			continue;

		// Check to see if we can use this
		if ( IsValidTransitionPoint( vecStartPos, vecEndPos ) )
			return nSequence;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Reserve our exit point so nothing moves into it while we're moving
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::ReserveExitPoint( void )
{
	// Cannot exit while we're upside down
	// FIXME: This is probably redundant!
	if ( m_hVehicle->IsOverturned() )
		return false;

	// Find the exit activity to use
	int nSequence = FindExitSequence();
	if ( nSequence == -1 )
		return false;

	// Get the exit position
	Vector vecGroundPos;
	if ( GetExitPoint( nSequence, &vecGroundPos, &m_vecTargetAngles ) == false )
		return false;

	// We have to do this specially because the activities are not named
	SetTransitionSequence( nSequence );

	// Reserve this space
	Vector hullMin = GetOuter()->GetHullMins();
	Vector hullMax = GetOuter()->GetHullMaxs();
	m_hBlocker = CEntityBlocker::Create( vecGroundPos, hullMin, hullMax, GetOuter(), true );

	// Save this destination position so we can interpolate towards it
	m_vecTargetPosition = vecGroundPos;
	
	// Pitch and roll must be zero when we finish!
	m_vecTargetAngles.x = m_vecTargetAngles.z = 0.0f;

	if ( passenger_debug_transition.GetBool() )
	{
		Vector vecForward;
		AngleVectors( m_vecTargetAngles, &vecForward, NULL, NULL );
		Vector vecArrowEnd = m_vecTargetPosition + ( vecForward * 64.0f );
		NDebugOverlay::HorzArrow( m_vecTargetPosition, vecArrowEnd, 8.0f, 255, 255, 0, 64, true, 4.0f );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the exact point we'd like to start our animation from to enter
//			the vehicle.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::GetEntryPoint( int nSequence, Vector *vecEntryPoint, QAngle *vecEntryAngles )
{
	bool bSucceeded = true;

	// Get the delta to the final position as will be dictated by this animation's auto movement
	Vector vecDeltaPos;
	QAngle vecDeltaAngles;
	GetOuter()->GetSequenceMovement( nSequence, 1.0f, 0.0f, vecDeltaPos, vecDeltaAngles );

	// Get the final position we're trying to end up at
	Vector vecTargetPos;
	QAngle vecTargetAngles;
	GetEntryTarget( &vecTargetPos, &vecTargetAngles );

	// Rotate it to match
	Vector vecPreDelta = vecDeltaPos;
	VectorRotate( vecPreDelta, vecTargetAngles, vecDeltaPos );

	// Offset this into the proper worldspace position
	vecTargetPos = vecTargetPos + vecDeltaPos;

	// Output the position, if requested
	if ( vecEntryPoint != NULL )
	{
		m_hVehicle->EntityToWorldSpace( vecTargetPos, vecEntryPoint );

		// Trace down to the ground to see where we'll stand
		Vector vecGroundPos;
		if ( FindGroundAtPosition( (*vecEntryPoint), 16.0f, 64.0f, &vecGroundPos ) == false )
		{
			// We failed
			if ( passenger_debug_transition.GetBool() )
			{
				NDebugOverlay::SweptBox( (*vecEntryPoint), vecGroundPos, GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), vec3_angle, 255, 0, 0, 64, 2.0f );
			}

			// The floor could not be found
			bSucceeded = false;
		}

		// Take this position
		*vecEntryPoint = vecGroundPos;
	}

	// Output the angles, if requested
	if ( vecEntryAngles != NULL )
	{
		// Add our delta angles to find what angles to start at
		*vecEntryAngles = vecTargetAngles;
		vecEntryAngles->y = UTIL_AngleMod( vecTargetAngles.y + vecDeltaAngles.y );

		//Transform those angles to worldspace
		matrix3x4_t angToParent, angToWorld;
		AngleMatrix( (*vecEntryAngles), angToParent );
		ConcatTransforms( m_hVehicle->EntityToWorldTransform(), angToParent, angToWorld );
		MatrixAngles( angToWorld, (*vecEntryAngles) );
	}

	// Debug info
	if ( passenger_debug_transition.GetBool() && vecEntryPoint && vecEntryAngles )
	{
		NDebugOverlay::Axis( *vecEntryPoint, vecTargetAngles, 16, true, 4.0f );
		NDebugOverlay::Cross3D( *vecEntryPoint, 4, 255, 255, 0, true, 4.0f );
		
		if ( vecEntryAngles != NULL )
		{
			Vector vecForward;
			AngleVectors( (*vecEntryAngles), &vecForward, NULL, NULL );
			Vector vecArrowEnd = (*vecEntryPoint ) + ( vecForward * 64.0f );
			NDebugOverlay::HorzArrow( (*vecEntryPoint), vecArrowEnd, 8.0f, 0, 255, 0, 64, true, 4.0f );
		}
	}

	return bSucceeded;
}

//-----------------------------------------------------------------------------
// Purpose: Do the low-level work to detach us from our vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::DetachFromVehicle( void )
{
	// Detach from the parent
	GetOuter()->SetParent( NULL );
	GetOuter()->SetMoveType( MOVETYPE_STEP );
	GetOuter()->AddFlag( FL_FLY );
	GetOuter()->SetGroundEntity( NULL );
	GetOuter()->SetCollisionGroup( COLLISION_GROUP_NPC );
	m_hVehicle->RemovePhysicsChild( GetOuter() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::AttachToVehicle( void )
{
	// Parent to the vehicle
	GetOuter()->ClearForceCrouch();
	GetOuter()->SetParent( m_hVehicle );
	GetOuter()->AddFlag( FL_FLY );
	GetOuter()->SetGroundEntity( NULL );
	GetOuter()->SetCollisionGroup( COLLISION_GROUP_IN_VEHICLE );

	// Turn off physical interactions while we're in the vehicle
	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if ( pPhysObj != NULL )
	{
		pPhysObj->EnableCollisions( false );
	}

	// Set our destination target
	GetEntryTarget( &m_vecTargetPosition, &m_vecTargetAngles );

	// Get physics messages from our attached physics object
	m_hVehicle->AddPhysicsChild( GetOuter() );
}

//-----------------------------------------------------------------------------
// Purpose: Handle task starting
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PASSENGER_ENTER_VEHICLE:
		{
			// You must have set your entrance animation before this point!
			Assert( m_nTransitionSequence != -1 );

			// Start us playing the correct sequence
			GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );
			SetPassengerState( PASSENGER_STATE_ENTERING );

			// Overlaying any gestures will mess us up, so don't allow it
			GetOuter()->RemoveAllGestures();
		}
		break;

	case TASK_PASSENGER_EXIT_VEHICLE:
		{
			// You must have set your entrance animation before this point!
			Assert( m_nTransitionSequence != -1 );

			// Start us playing the correct sequence
			GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );
			
			// Overlaying any gestures will mess us up, so don't allow it
			GetOuter()->RemoveAllGestures();
		}
		break;

	case TASK_PASSENGER_ATTACH_TO_VEHICLE:
		{
			AttachToVehicle();
			TaskComplete();
		}
		break;

	case TASK_PASSENGER_DETACH_FROM_VEHICLE:
		{
			// Place an entity blocker where we're going to go
			if ( ReserveExitPoint() == false )
			{
				OnExitVehicleFailed();
				TaskFail("Failed to find valid exit point\n");
				return;
			}

			// Physically detach from the vehicle
			DetachFromVehicle();

			// Mark that we're now disembarking
			SetPassengerState( PASSENGER_STATE_EXITING );

			TaskComplete();
		}
		break;

	case TASK_PASSENGER_SET_IDEAL_ENTRY_YAW:
		{
			// Get the ideal facing to enter the vehicle
			QAngle vecEntryAngles;
			GetEntryPoint( m_nTransitionSequence, NULL, &vecEntryAngles );
			GetOuter()->GetMotor()->SetIdealYaw( vecEntryAngles.y );
			
			TaskComplete();
			return;
		}	
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle task running
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PASSENGER_ENTER_VEHICLE:
		{	
			// Correct for angular/spatial deviation
			Assert( GetSequence() == m_nTransitionSequence );
			if ( GetSequence() != m_nTransitionSequence )
			{
				Warning("Corrected entrance animation on vehicle enter!\n");
				GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );
				GetOuter()->GetNavigator()->ClearGoal();
				SetTransitionSequence( m_nTransitionSequence );
			}
			
			bool corrected = DoTransitionMovement();

			// We must be done with the animation and in the correct position
			if ( corrected == false )
			{
				FinishEnterVehicle();
				TaskComplete();
			}
		}
		break;

	case TASK_PASSENGER_EXIT_VEHICLE:
		{
			// Correct for angular/spatial deviation
			Assert( GetSequence() == m_nTransitionSequence );
			if ( GetSequence() != m_nTransitionSequence )
			{
				Warning("Corrected exit animation on vehicle exit!\n");
				GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );
				GetOuter()->GetNavigator()->ClearGoal();
				SetTransitionSequence( m_nTransitionSequence );
			}

			// Correct for angular/spatial deviation
			bool corrected = DoTransitionMovement();

			// We must be done with the animation and in the correct position
			if ( corrected == false )
			{
				FinishExitVehicle();
				TaskComplete();
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the blend amounts for position and angles, given a point in 
//			time within a sequence
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::GetSequenceBlendAmount( float flCycle, float *posBlend, float *angBlend )
{
	// Find positional blend, if requested
	if ( posBlend != NULL )
	{
		float flFrac = RemapValClamped( flCycle, m_flOriginStartFrame, m_flOriginEndFrame, 0.0f, 1.0f );
		(*posBlend) = SimpleSpline( flFrac );
	}

	// Find angular blend, if requested
	if ( angBlend != NULL )
	{
		float flFrac = RemapValClamped( flCycle, m_flAnglesStartFrame, m_flAnglesEndFrame, 0.0f, 1.0f );
		(*angBlend) = SimpleSpline( flFrac );		
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the target destination for the entry animation
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::GetEntryTarget( Vector *vecOrigin, QAngle *vecAngles )
{
	// Get the ultimate position we'll end up at
	m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPositionLocal( GetOuter(), vecOrigin, vecAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ideal position to be in to end up at the target at the
//			end of the animation.
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::GetTransitionAnimationIdeal( float flCycle, const Vector &vecTargetPos, const QAngle &vecTargetAngles, Vector *idealOrigin, QAngle *idealAngles )
{
	// Get the position in time working backwards from our goal
	Vector vecDeltaPos;
	QAngle vecDeltaAngles;
	GetOuter()->GetSequenceMovement( GetSequence(), 1.0f, flCycle, vecDeltaPos, vecDeltaAngles );

	// Rotate the delta by our local angles
	Vector vecPreDelta = vecDeltaPos;
	VectorRotate( vecPreDelta, vecTargetAngles, vecDeltaPos );

	// Ideal origin
	if ( idealOrigin != NULL )
	{
		*idealOrigin = ( vecTargetPos + vecDeltaPos );
	}

	// Ideal angles
	if ( idealAngles != NULL )
	{
		(*idealAngles).x = anglemod( vecTargetAngles.x + vecDeltaAngles.x );
		(*idealAngles).y = anglemod( vecTargetAngles.y + vecDeltaAngles.y );
		(*idealAngles).z = anglemod( vecTargetAngles.z + vecDeltaAngles.z );
	}
}

//-----------------------------------------------------------------------------
//	FIXME:	This is basically a complete duplication of GetIntervalMovement
//			which doesn't remove the x and z components of the angles.  This
//			should be consolidated to not replicate so much code! -- jdw
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::LocalIntervalMovement( float flInterval, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetOuter()->GetModelPtr();
	if ( pstudiohdr == NULL )
		return false;

	// Get our next cycle point
	float flNextCycle = GetNextCycleForInterval( GetSequence(), flInterval );

	// Fix-up loops
	if ( ( GetOuter()->SequenceLoops() == false ) && flNextCycle > 1.0f )
	{
		flInterval = GetOuter()->GetCycle() / ( GetOuter()->GetSequenceCycleRate( GetSequence() ) * GetOuter()->GetPlaybackRate() );
		flNextCycle = 1.0f;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	// Find the delta position and delta angles for this sequence
	if ( Studio_SeqMovement( pstudiohdr, GetOuter()->GetSequence(), GetOuter()->GetCycle(), flNextCycle, GetOuter()->GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		Vector vecPreDelta = deltaPos;
		VectorRotate( vecPreDelta, GetOuter()->GetLocalAngles(), deltaPos );
		
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles = GetLocalAngles() + deltaAngles;

		return true;
	}
	else
	{
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();

		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get the next cycle point in a sequence for a given interval
//-----------------------------------------------------------------------------
float CAI_PassengerBehavior::GetNextCycleForInterval( int nSequence, float flInterval )
{
	return GetOuter()->GetCycle() + flInterval * GetOuter()->GetSequenceCycleRate( GetSequence() ) * GetOuter()->GetPlaybackRate();
}

//-----------------------------------------------------------------------------
// Purpose: Draw debug information for the transitional movement
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::DrawDebugTransitionInfo( const Vector &vecIdealPos, const QAngle &vecIdealAngles, const Vector &vecAnimPos, const QAngle &vecAnimAngles )
{
	// Debug info
	if ( GetPassengerState() == PASSENGER_STATE_ENTERING )
	{	
		// Green - Ideal location
		Vector foo;
		m_hVehicle->EntityToWorldSpace( vecIdealPos, &foo );
		NDebugOverlay::Cross3D( foo, 2, 0, 255, 0, true, 0.1f );
		NDebugOverlay::Axis( foo, vecIdealAngles, 8, true, 0.1f );

		// Blue - Actual location
		m_hVehicle->EntityToWorldSpace( vecAnimPos, &foo );
		NDebugOverlay::Cross3D( foo, 2, 0, 0, 255, true, 0.1f );
		NDebugOverlay::Axis( foo, vecAnimAngles, 8, true, 0.1f );
	}
	else
	{
		// Green - Ideal location
		NDebugOverlay::Cross3D( vecIdealPos, 4, 0, 255, 0, true, 0.1f );
		NDebugOverlay::Axis( vecIdealPos, vecIdealAngles, 8, true, 0.1f );

		// Blue - Actual location
		NDebugOverlay::Cross3D( vecAnimPos, 2, 0, 0, 255, true, 0.1f );
		NDebugOverlay::Axis( vecAnimPos, vecAnimAngles, 8, true, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Local movement to enter or exit the vehicle
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::DoTransitionMovement( void )
{
	// Get our animation's extrapolated end position
	Vector	vecAnimPos;
	QAngle	vecAnimAngles;
	float	flInterval = GetOuter()->GetAnimTimeInterval();
	bool	bSequenceFinished;

	// Get the position we're moving to for this frame with our animation's motion
	if ( LocalIntervalMovement( flInterval, bSequenceFinished, vecAnimPos, vecAnimAngles ) )
	{	
		// Get the position we'd ideally be in
		Vector vecIdealPos;
		QAngle vecIdealAngles;
		float flNextCycle = GetNextCycleForInterval( GetOuter()->GetSequence(), flInterval );
		flNextCycle = clamp( flNextCycle, 0.0f, 1.0f );
		GetTransitionAnimationIdeal( flNextCycle, m_vecTargetPosition, m_vecTargetAngles, &vecIdealPos, &vecIdealAngles );

		// Get the amount of error to blend out
		float flPosBlend = 1.0f;
		float flAngBlend = 1.0f;
		GetSequenceBlendAmount( flNextCycle, &flPosBlend, &flAngBlend );

		// Find the error between our position and our ideal
		Vector vecDelta = ( vecIdealPos - vecAnimPos ) * flPosBlend;
		
		QAngle vecDeltaAngles;
		vecDeltaAngles.x = AngleDiff( vecIdealAngles.x, vecAnimAngles.x ) * flAngBlend;
		vecDeltaAngles.y = AngleDiff( vecIdealAngles.y, vecAnimAngles.y ) * flAngBlend;
		vecDeltaAngles.z = AngleDiff( vecIdealAngles.z, vecAnimAngles.z ) * flAngBlend;

		// Factor in the error
		GetOuter()->SetLocalOrigin( vecAnimPos + vecDelta );
		GetOuter()->SetLocalAngles( vecAnimAngles + vecDeltaAngles );

		// Draw our debug information
		if ( passenger_debug_transition.GetBool() )
		{
			DrawDebugTransitionInfo( vecIdealPos, vecIdealAngles, vecAnimPos, vecAnimAngles );
		}

		// We're done moving
		if ( bSequenceFinished )
			return false;

		// We're still correcting out the error
		return true;
	}

	// There was no movement in the animation
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Translate normal schedules into vehicle schedules
//-----------------------------------------------------------------------------
int CAI_PassengerBehavior::TranslateSchedule( int scheduleType )
{
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Always be seated when riding in the car!
		if ( scheduleType == SCHED_IDLE_STAND )
			return SCHED_PASSENGER_IDLE;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the velocity of the vehicle with respect to its orientation
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::GetLocalVehicleVelocity( Vector *pOut  )
{
	Vector velocity;
	m_hVehicle->GetVelocity( &velocity, NULL );
	m_hVehicle->WorldToEntitySpace( m_hVehicle->GetAbsOrigin() + velocity, pOut );
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions we can comment on or react to while riding in the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::GatherVehicleStateConditions( void )
{
	// Must have a vehicle to bother with this
	if ( m_hVehicle == NULL )
		return;

	// Clear out transient conditions
	ClearCondition( COND_PASSENGER_HARD_IMPACT );
	ClearCondition( COND_PASSENGER_ERRATIC_DRIVING );
	ClearCondition( COND_PASSENGER_JOSTLE_SMALL );
	ClearCondition( COND_PASSENGER_VEHICLE_STARTED );
	ClearCondition( COND_PASSENGER_VEHICLE_STOPPED );
	ClearCondition( COND_PASSENGER_PLAYER_ENTERED_VEHICLE );
	ClearCondition( COND_PASSENGER_PLAYER_EXITED_VEHICLE );

	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer )
	{
		if ( pPlayer->IsInAVehicle() && pPlayer->GetVehicle() == m_hVehicle->GetServerVehicle() )
		{
			if ( m_vehicleState.m_bPlayerInVehicle == false )
			{
				SetCondition( COND_PASSENGER_PLAYER_ENTERED_VEHICLE );
				m_vehicleState.m_bPlayerInVehicle = true;
			}
		}
		else
		{
			if ( m_vehicleState.m_bPlayerInVehicle )
			{
				SetCondition( COND_PASSENGER_PLAYER_EXITED_VEHICLE );
				m_vehicleState.m_bPlayerInVehicle = false;
			}
		}
	}

	// Get the vehicle's boost state
	if ( m_hVehicle->m_nBoostTimeLeft < 100.0f )
	{
		if ( m_vehicleState.m_bWasBoosting == false )
		{
			m_vehicleState.m_bWasBoosting = true;
		}
	}
	else
	{
		m_vehicleState.m_bWasBoosting = false;
	}

	// Detect being overturned
	if ( m_hVehicle->IsOverturned() )
	{
		SetCondition( COND_PASSENGER_OVERTURNED );

		if ( m_vehicleState.m_bWasOverturned == false )
		{
			m_vehicleState.m_bWasOverturned = true;
		}
	}
	else
	{
		ClearCondition( COND_PASSENGER_OVERTURNED ); 
		m_vehicleState.m_bWasOverturned = false;
	}

	// Get our local velocity
	Vector	localVelocity;
	GetLocalVehicleVelocity( &localVelocity );

	Vector	deltaVelocity = ( localVelocity - m_vehicleState.m_vecLastLocalVelocity );

	// Detect a sudden stop!
	if ( deltaVelocity.y < passenger_impact_response_threshold.GetFloat() )
	{
		SetCondition( COND_PASSENGER_HARD_IMPACT );
	}
	else if ( fabs( deltaVelocity.x ) > 200.0f || fabs( deltaVelocity.z ) > 75.0f )
	{
		// The X axis represents lateral movement and the Z axis represents vertical movement{
   		SetCondition( COND_PASSENGER_ERRATIC_DRIVING );
	} 
	else if ( fabs( deltaVelocity.x ) > 50.0f || fabs( deltaVelocity.z ) > 25.0f )
	{
		// Lightly jostled
      	SetCondition( COND_PASSENGER_JOSTLE_SMALL );
	}

	// Get our speed
	float flSpeedSqr = localVelocity.LengthSqr();

	// See if we've crossed over the threshold between movement to stillness
	if ( m_vehicleState.m_flLastSpeedSqr > STOPPED_VELOCITY_THRESHOLD_SQR && flSpeedSqr < STOPPED_VELOCITY_THRESHOLD_SQR )
	{
		SetCondition( COND_PASSENGER_VEHICLE_STOPPED );
	}
	else if ( m_vehicleState.m_flLastSpeedSqr < STARTED_VELOCITY_THRESHOLD_SQR && flSpeedSqr > STARTED_VELOCITY_THRESHOLD_SQR )
	{
		// See if we've crossed over the threshold between stillness to movement
		SetCondition( COND_PASSENGER_VEHICLE_STARTED );
	}

	// Save this as our last speed
	m_vehicleState.m_flLastSpeedSqr = flSpeedSqr;

	// Find our delta velocity from the last frame
	m_vehicleState.m_vecDeltaVelocity = ( localVelocity - m_vehicleState.m_vecLastLocalVelocity );
	m_vehicleState.m_vecLastLocalVelocity = localVelocity;
	
	// Get our angular velocity
	Vector vecVelocity;
	AngularImpulse angVelocty;
	m_hVehicle->GetVelocity( &vecVelocity, &angVelocty );
	QAngle angVel( angVelocty.x, angVelocty.y, angVelocty.z );
	
	// Blend this into the old values
	m_vehicleState.m_vecLastAngles = ( m_vehicleState.m_vecLastAngles * 0.2f ) + ( angVel * 0.8f );
}

//-----------------------------------------------------------------------------
// Purpose: Do some pre-schedule clean-up
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// If we're outside the vehicle, we need to turn this behavior off immediately
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE && HasCondition( COND_PASSENGER_CANCEL_ENTER ) )
	{
		// Clear out our passenger intent
		m_PassengerIntent = PASSENGER_INTENT_NONE;
		ClearCondition( COND_PASSENGER_CANCEL_ENTER );

		// Stop pathfinding
		GetOuter()->GetNavigator()->ClearGoal();

		// We're outside and have no intent to enter, so we're done
		Disable();

		// This must be stomped to cause our behavior to relinquish control
		GetOuter()->ClearSchedule("Passenger enter canceled");
	}

#ifdef DEBUG
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		Vector vecSeatOrigin;
		QAngle vecSeatAngles;
		if ( m_hVehicle && m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPositionLocal( GetOuter(), &vecSeatOrigin, &vecSeatAngles ) )
		{
			if ( ( GetLocalOrigin() - vecSeatOrigin ).LengthSqr() > Square( 0.1f ) )
			{
				Warning( "Passenger has strayed from seat position!\n" );
				// GetOuter()->SetLocalOrigin( vecSeatOrigin );
				// GetOuter()->SetLocalAngles( vecSeatAngles );
			}
		}
		else
		{
			Warning( "Passenger is in vehicle without a valid seat position! -- EJECTED\n" );
			GetOuter()->SetParent( NULL );
			Disable();

			return;
		}
	}
#endif // DEBUG
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions for our use in making decisions
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::GatherConditions( void )
{
	if ( IsEnabled() == false )
		return BaseClass::GatherConditions();

	// Sense the state of the car
	GatherVehicleStateConditions();

	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	if ( m_hVehicle == NULL )
		return;

	// Mark whether we're overturned or not
	bool bOverturned = m_hVehicle->IsOverturned();
	criteriaSet.AppendCriteria( "vehicle_overturned", bOverturned ? "1" : "0" );

	// Denote whether we're in the vehicle or not
	bool bInsideVehicle = ( GetPassengerState() == PASSENGER_STATE_INSIDE );
	criteriaSet.AppendCriteria( "vehicle_inside", bInsideVehicle ? "1" : "0" );

	// Note what angle we're at (extreme or normal)
	Vector vecUp( 0.0f, 0.0f, 1.0f );
	Vector vecVehicleUp;
	m_hVehicle->GetVectors( NULL, NULL, &vecVehicleUp );

	float flVehicleUp = DotProduct( vecVehicleUp, vecUp );
	criteriaSet.AppendCriteria( "vehicle_tilt", UTIL_VarArgs( "%.2f", flVehicleUp ) );

	// Set the vehicle's speed (necessary for certain types of movement judgments)
	float flVehicleSpeed = sqrt( m_vehicleState.m_flLastSpeedSqr );
	criteriaSet.AppendCriteria( "vehicle_speed", UTIL_VarArgs( "%f", flVehicleSpeed  ) );

	// Whether or not the passenger is currently able to enter the vehicle (only accounts for locking really)
	bool bCanExitVehicle = ( m_hVehicle->NPC_CanExitVehicle( GetOuter(), true ) );
	criteriaSet.AppendCriteria( "vehicle_can_exit", bCanExitVehicle ? "1" : "0" );

	// Whether or not the passenger is currently able to exit the vehicle (only accounts for locking really)
	bool bCanEnterVehicle = ( m_hVehicle->NPC_CanEnterVehicle( GetOuter(), true ) );
	criteriaSet.AppendCriteria( "vehicle_can_enter", bCanEnterVehicle ? "1" : "0" );
}

//-----------------------------------------------------------------------------
// Purpose: Cache off our frame numbers from the sequence keyvalue blocks
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::CacheBlendTargets( void )
{
	// Get the keyvalues for this sequence
	KeyValues *seqValues = GetOuter()->GetSequenceKeyValues( m_nTransitionSequence );
	if ( seqValues == NULL )
	{
		Assert( 0 );
		return;
	}

	// Get the entry/exit subkeys
	KeyValues *blendValues = seqValues->FindKey( "entryexit_blend" );
	if ( blendValues == NULL )
	{
		Assert( 0 );
		return;
	}

	// Find our frame range on this sequence
	int nMaxFrames = Studio_MaxFrame( GetOuter()->GetModelPtr(), m_nTransitionSequence, GetOuter()->GetPoseParameterArray() );

	// Find a key by this name
	KeyValues *subKeys = blendValues->FindKey( ORIGIN_KEYNAME );
	if ( subKeys )
	{
		// Retrieve our frame numbers
		m_flOriginStartFrame = subKeys->GetFloat( "startframe", 0.0f );
		m_flOriginEndFrame = subKeys->GetFloat( "endframe", nMaxFrames );

		// Convert to normalized values
		m_flOriginStartFrame = RemapValClamped( m_flOriginStartFrame, 0, nMaxFrames, 0.0f, 1.0f );
		m_flOriginEndFrame = RemapValClamped( m_flOriginEndFrame, 0, nMaxFrames, 0.0f, 1.0f );
	}

	// Find a key by this name
	subKeys = blendValues->FindKey( ANGLES_KEYNAME );
	if ( subKeys )
	{
		// Retrieve our frame numbers
		m_flAnglesStartFrame = subKeys->GetFloat( "startframe", 0.0f );
		m_flAnglesEndFrame = subKeys->GetFloat( "endframe", nMaxFrames );

		// Convert to normalized values
		m_flAnglesStartFrame = RemapValClamped( m_flAnglesStartFrame, 0, nMaxFrames, 0.0f, 1.0f );
		m_flAnglesEndFrame = RemapValClamped( m_flAnglesEndFrame, 0, nMaxFrames, 0.0f, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::SetTransitionSequence( int nSequence )
{
	// We need to use the ACT_SCRIPT_CUSTOM_MOVE scenario for this type of custom anim
	m_nTransitionSequence = nSequence;
	GetOuter()->m_iszSceneCustomMoveSeq = AllocPooledString( GetOuter()->GetSequenceName( m_nTransitionSequence ) );

	// Cache off our blending information at this point
	CacheBlendTargets();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::SpeakIfAllowed( AIConcept_t concept, const char *modifiers /*= NULL*/, bool bRespondingToPlayer /*= false*/, char *pszOutResponseChosen /*= NULL*/, size_t bufsize /*= 0*/ )
{
	// FIXME: Store this cast off?
	CAI_PlayerAlly *pAlly = dynamic_cast<CAI_PlayerAlly *>(GetOuter());
	if ( pAlly != NULL )
		return pAlly->SpeakIfAllowed( concept, modifiers, bRespondingToPlayer, pszOutResponseChosen, bufsize );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Forces us to begin a dynamic scripted scene
// Input  : *lpszInteractionName - Name of the sequence we'll play
//			*pOther - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::ForceVehicleInteraction( const char *lpszInteractionName, CBaseCombatCharacter *pOther )
{
	// Don't do this unless we're sitting in the cabin of the vehicle!
	if ( GetPassengerState() != PASSENGER_STATE_INSIDE )
		return false;

	// Set a sequence and fire it off!
	GetOuter()->m_iszSceneCustomMoveSeq = AllocPooledString( lpszInteractionName );
	GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );

	// Slam our schedule (very unsafe!)
	GetOuter()->SetSchedule( SCHED_PASSENGER_PLAY_SCRIPTED_ANIM );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Fix up teleport event when in the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	//First, safely remove me from the vehicle
	if ( GetPassengerState() != PASSENGER_STATE_OUTSIDE )
	{
		// Detach from the vehicle
		DetachFromVehicle();
		FinishExitVehicle();
		
		// Turn the behavior off
		GetOuter()->ClearSchedule( "ai_behavior_passenger: teleport while in vehicle" );
		Disable();
	}

	//Then allow the teleportation
	BaseClass::Teleport( newPosition, newAngles, newVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: We override this function because it can completely wreak havoc if
//			we're in the middle of a transition
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::ClearSchedule( const char *szReason )
{
	// Cannot do this while we're transitioning, but it's also a bug because the code that called it was probably relying on this to work!
	if ( GetPassengerState() == PASSENGER_STATE_ENTERING || GetPassengerState() == PASSENGER_STATE_EXITING )
	{
		Warning("ClearSchedule rejected due to transitioning passenger: %s\n", szReason );
		return;
	}

	// TODO: Even this will probably need more crafting depending on what we're doing in the vehicle
	// Otherwise allow it
	GetOuter()->ClearSchedule( szReason );
}

//-----------------------------------------------------------------------------
// Purpose: Dictate the terms for being interrupted by scripted schedules or scenes
//-----------------------------------------------------------------------------
bool CAI_PassengerBehavior::IsInterruptable( void )
{
	// NOTE: We should never be interrupted this way when in a car. This would effectively makes us go comatose if we
	//		 start a FACETO, MOVETO, or SEQUENCE command from a VCD.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehavior::CancelEnterVehicle( void )
{
	// Stop!
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
	{
		SetCondition( COND_PASSENGER_CANCEL_ENTER );
	}
}

// ----------------------------------------------
// Custom AI declarations
// ----------------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_PassengerBehavior )
{
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE )
	DECLARE_ACTIVITY( ACT_PASSENGER_RANGE_ATTACK1 )

	DECLARE_CONDITION( COND_PASSENGER_HARD_IMPACT )
	DECLARE_CONDITION( COND_PASSENGER_ENTERING )
	DECLARE_CONDITION( COND_PASSENGER_EXITING )
	DECLARE_CONDITION( COND_PASSENGER_VEHICLE_STARTED )
	DECLARE_CONDITION( COND_PASSENGER_VEHICLE_STOPPED )
	DECLARE_CONDITION( COND_PASSENGER_OVERTURNED )
	DECLARE_CONDITION( COND_PASSENGER_CANCEL_ENTER )
	DECLARE_CONDITION( COND_PASSENGER_ERRATIC_DRIVING )
	DECLARE_CONDITION( COND_PASSENGER_PLAYER_ENTERED_VEHICLE )
	DECLARE_CONDITION( COND_PASSENGER_PLAYER_EXITED_VEHICLE )
	DECLARE_CONDITION( COND_PASSENGER_JOSTLE_SMALL )

	DECLARE_TASK( TASK_PASSENGER_ENTER_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_EXIT_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_ATTACH_TO_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_DETACH_FROM_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_SET_IDEAL_ENTRY_YAW )

	// FIXME: Move to companion
	DEFINE_SCHEDULE
	(
	SCHED_PASSENGER_ENTER_VEHICLE,

	"	Tasks"
	"		TASK_PASSENGER_SET_IDEAL_ENTRY_YAW	0"
	"		TASK_FACE_IDEAL						0"
	"		TASK_PASSENGER_ATTACH_TO_VEHICLE	0"
	"		TASK_PASSENGER_ENTER_VEHICLE		0"
	""
	"	Interrupts"
	"		COND_NO_CUSTOM_INTERRUPTS"
	)

	DEFINE_SCHEDULE
	(
	SCHED_PASSENGER_EXIT_VEHICLE,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_PASSENGER_IDLE"
	"		TASK_PASSENGER_DETACH_FROM_VEHICLE	0"
	"		TASK_WAIT 0.1"  // We must wait one tick for us to start being updated
	"		TASK_PASSENGER_EXIT_VEHICLE	0"
	""
	"	Interrupts"
	"		COND_NO_CUSTOM_INTERRUPTS"
	"		COND_TASK_FAILED"
	)

	DEFINE_SCHEDULE
	( 
	SCHED_PASSENGER_IDLE,

	"	Tasks"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT					2"
	""
	"	Interrupts"
	"		COND_PROVOKED"
	"		COND_NEW_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_PASSENGER_EXITING"
	"		COND_HEAR_DANGER"
	)

	DEFINE_SCHEDULE
	( 
	SCHED_PASSENGER_PLAY_SCRIPTED_ANIM,
	
	"	Tasks"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_SCRIPT_CUSTOM_MOVE"
	""
	"	Interrupts"
	"		COND_PASSENGER_HARD_IMPACT"
	)

	AI_END_CUSTOM_SCHEDULE_PROVIDER()
}
