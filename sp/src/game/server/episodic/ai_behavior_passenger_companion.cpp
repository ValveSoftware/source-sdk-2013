//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Companion NPCs riding in cars
//
//=============================================================================

#include "cbase.h"
#include "ai_speech.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "ai_navgoaltype.h"
#include "ai_memory.h"
#include "ai_behavior_passenger_companion.h"
#include "ai_squadslot.h"
#include "npc_playercompanion.h"
#include "ai_route.h"
#include "saverestore_utlvector.h"
#include "cplane.h"
#include "util_shared.h"
#include "sceneentity.h"

bool SphereWithinPlayerFOV( CBasePlayer *pPlayer, const Vector &vecCenter, float flRadius );

#define	PASSENGER_NEAR_VEHICLE_THRESHOLD	64.0f

#define MIN_OVERTURNED_DURATION			1.0f // seconds
#define MIN_FAILED_EXIT_ATTEMPTS		4
#define MIN_OVERTURNED_WARN_DURATION	4.0f // seconds

ConVar passenger_collision_response_threshold( "passenger_collision_response_threshold", "250.0" );
ConVar passenger_debug_entry( "passenger_debug_entry", "0" );
ConVar passenger_use_leaning("passenger_use_leaning", "1" );
extern ConVar passenger_debug_transition;

// Custom activities
Activity ACT_PASSENGER_IDLE_AIM;
Activity ACT_PASSENGER_RELOAD;
Activity ACT_PASSENGER_OVERTURNED;
Activity ACT_PASSENGER_IMPACT;
Activity ACT_PASSENGER_IMPACT_WEAPON;
Activity ACT_PASSENGER_POINT;
Activity ACT_PASSENGER_POINT_BEHIND;
Activity ACT_PASSENGER_IDLE_READY;
Activity ACT_PASSENGER_GESTURE_JOSTLE_LARGE;
Activity ACT_PASSENGER_GESTURE_JOSTLE_SMALL;
Activity ACT_PASSENGER_GESTURE_JOSTLE_LARGE_STIMULATED;
Activity ACT_PASSENGER_GESTURE_JOSTLE_SMALL_STIMULATED;
Activity ACT_PASSENGER_COWER_IN;
Activity ACT_PASSENGER_COWER_LOOP;
Activity ACT_PASSENGER_COWER_OUT;
Activity ACT_PASSENGER_IDLE_FIDGET;

BEGIN_DATADESC( CAI_PassengerBehaviorCompanion )

	DEFINE_EMBEDDED( m_VehicleMonitor ),

	DEFINE_UTLVECTOR( m_FailedEntryPositions, FIELD_EMBEDDED ),
	
	DEFINE_FIELD( m_flOverturnedDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_flUnseenDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_nExitAttempts, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextOverturnWarning, FIELD_TIME ),
	DEFINE_FIELD( m_flEnterBeginTime, FIELD_TIME ),
	DEFINE_FIELD( m_hCompanion, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextJostleTime, FIELD_TIME ),
	DEFINE_FIELD( m_nVisibleEnemies, FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastLateralLean, FIELD_FLOAT ),
	DEFINE_FIELD( m_flEntraceUpdateTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextEnterAttempt, FIELD_TIME ),
	DEFINE_FIELD( m_flNextFidgetTime, FIELD_TIME ),

END_DATADESC();

BEGIN_SIMPLE_DATADESC( FailPosition_t )
	
	DEFINE_FIELD( vecPosition, FIELD_VECTOR ),
	DEFINE_FIELD( flTime, FIELD_TIME ),

END_DATADESC();

CAI_PassengerBehaviorCompanion::CAI_PassengerBehaviorCompanion( void ) : 
m_flUnseenDuration( 0.0f ),
m_flNextOverturnWarning( 0.0f ),
m_flOverturnedDuration( 0.0f ), 
m_nExitAttempts( 0 ),
m_flNextEnterAttempt( 0.0f ),
m_flLastLateralLean( 0.0f ),
m_flNextJostleTime( 0.0f )
{
	memset( &m_vehicleState, 0, sizeof( m_vehicleState ) );
	m_VehicleMonitor.ClearMark();
}

void CAI_PassengerBehaviorCompanion::Enable( CPropJeepEpisodic *pVehicle, bool bImmediateEnter /*= false*/ )
{
	BaseClass::Enable( pVehicle );

	// Store this up for quick reference later on
	m_hCompanion = dynamic_cast<CNPC_PlayerCompanion *>(GetOuter());

	// See if we want to sit in the vehicle immediately
	if ( bImmediateEnter )
	{
		// Find the seat and sit in it
		if ( ReserveEntryPoint( VEHICLE_SEAT_ANY ) )
		{
			// Attach
			AttachToVehicle();

			// This will slam us into the right position and clean up
			FinishEnterVehicle();
			GetOuter()->IncrementInterpolationFrame();

			// Start our schedule immediately
			ClearSchedule( "Immediate entry to vehicle" );
		}
	}
}

//-----------------------------------------------------------------------------
// Set up the shot regulator based on the equipped weapon
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::OnUpdateShotRegulator( void )
{
	if ( GetVehicleSpeed() > 250 )
	{
		// Default values
		GetOuter()->GetShotRegulator()->SetBurstInterval( 0.1f, 0.5f );
		GetOuter()->GetShotRegulator()->SetBurstShotCountRange( 1, 4 );
		GetOuter()->GetShotRegulator()->SetRestInterval( 0.25f, 1.0f );
	}
	else
	{
		BaseClass::OnUpdateShotRegulator();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::IsValidEnemy( CBaseEntity *pEntity )
{
	// The target must be much closer in the vehicle 
	float flDistSqr = ( pEntity->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
	if ( flDistSqr > Square( (40*12) ) && pEntity->Classify() != CLASS_BULLSEYE )
		return false;

	// Determine if the target is going to move past us?
	return BaseClass::IsValidEnemy( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed the vehicle is moving at
// Output : units per second
//-----------------------------------------------------------------------------
float CAI_PassengerBehaviorCompanion::GetVehicleSpeed( void )
{
	if ( m_hVehicle == NULL )
	{
		Assert(0);
		return -1.0f;
	}

	Vector	vecVelocity;
	m_hVehicle->GetVelocity( &vecVelocity, NULL );

	// Get our speed
	return vecVelocity.Length();
}

//-----------------------------------------------------------------------------
// Purpose: Detect oncoming collisions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherVehicleCollisionConditions( const Vector &localVelocity )
{
	// Look for walls in front of us
	if ( localVelocity.y > passenger_collision_response_threshold.GetFloat() )
	{
		// Detect an upcoming collision
		Vector vForward;
		m_hVehicle->GetVectors( &vForward, NULL, NULL );

		// Use a smaller bounding box to make it detect mostly head-on impacts
		Vector	mins, maxs;
		mins.Init( -24, -24, 32 );
		maxs.Init(  24,  24, 64 );

		float dt = 0.6f;  // Seconds
		float distance = localVelocity.y * dt;

		// Find our angular velocity as a vector
		Vector vecAngularVelocity;
		vecAngularVelocity.z = 0.0f;
		SinCos( DEG2RAD( m_vehicleState.m_vecLastAngles.z * dt ), &vecAngularVelocity.y, &vecAngularVelocity.x );
		
		Vector vecOffset;
		VectorRotate( vecAngularVelocity, m_hVehicle->GetAbsAngles() + QAngle( 0, 90, 0 ), vecOffset );

		vForward += vecOffset;
		VectorNormalize( vForward );

		// Trace ahead of us to see what's there
		CTraceFilterNoNPCsOrPlayer filter( m_hVehicle, COLLISION_GROUP_NONE ); // We don't care about NPCs or the player (certainly if they're in the vehicle!)

		trace_t	tr;
		UTIL_TraceHull( m_hVehicle->GetAbsOrigin(), m_hVehicle->GetAbsOrigin() + ( vForward * distance ), mins, maxs, MASK_SOLID, &filter, &tr );
		
		bool bWarnCollision = true;
		if ( tr.DidHit() )
		{
			// We need to see how "head-on" to the surface we are
			float impactDot = DotProduct( tr.plane.normal, vForward );

			// Don't warn over grazing blows or slopes
			if ( impactDot < -0.9f && tr.plane.normal.z < 0.75f )
			{
				// Make sure this is a worthwhile thing to warn about
				if ( tr.m_pEnt )
				{
					// If it's physical and moveable, then ignore it because we'll probably smash or move it
					IPhysicsObject *pObject = tr.m_pEnt->VPhysicsGetObject();
					if ( pObject && pObject->IsMoveable() )
					{
						bWarnCollision = false;
					}
				}

				// Note that we should say something to the player about it
				if ( bWarnCollision )
				{
					SetCondition( COND_PASSENGER_WARN_COLLISION );
				}
			}
		}
	}

	if ( passenger_use_leaning.GetBool() )
	{
		// Calculate how our body is leaning
		CalculateBodyLean();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Speak various lines about the state of the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::SpeakVehicleConditions( void )
{
	Assert( m_hVehicle != NULL );
	if (  m_hVehicle == NULL )
		return;

	// Speak if we just hit something
	if ( HasCondition( COND_PASSENGER_HARD_IMPACT ) )
	{
		SpeakIfAllowed( TLK_PASSENGER_IMPACT );
	}

	// Speak if we're overturned
	if ( HasCondition( COND_PASSENGER_OVERTURNED ) )
	{
		SpeakIfAllowed(	TLK_PASSENGER_OVERTURNED );
	}

	// Speak if we're about to hit something
	if ( HasCondition( COND_PASSENGER_WARN_COLLISION ) )
	{
		// Make Alyx look at the impending impact 
		Vector vecForward;
		m_hVehicle->GetVectors( &vecForward, NULL, NULL );
		Vector vecLookPos = m_hVehicle->WorldSpaceCenter() + ( vecForward * 64.0f );
		GetOuter()->AddLookTarget( vecLookPos, 1.0f, 1.0f );

		SpeakIfAllowed( TLK_PASSENGER_WARN_COLLISION );
		ClearCondition( COND_PASSENGER_WARN_COLLISION );
	}

	// Speak if the player is driving like a madman
	if ( HasCondition( COND_PASSENGER_ERRATIC_DRIVING ) )
	{
		SpeakIfAllowed( TLK_PASSENGER_ERRATIC_DRIVING );
	}

	// The vehicle has come to a halt
	if ( HasCondition( COND_PASSENGER_VEHICLE_STOPPED ) )
	{
		float flDist = ( WorldSpaceCenter() - m_hVehicle->WorldSpaceCenter() ).Length();
		CFmtStrN<128> modifiers( "vehicle_distance:%f", flDist );
		SpeakIfAllowed( TLK_PASSENGER_VEHICLE_STOPPED, modifiers );
	}

	// The vehicle has started to move
	if ( HasCondition( COND_PASSENGER_VEHICLE_STARTED ) )
	{
		float flDist = ( WorldSpaceCenter() - m_hVehicle->WorldSpaceCenter() ).Length();
		CFmtStrN<128> modifiers( "vehicle_distance:%f", flDist );
		SpeakIfAllowed( TLK_PASSENGER_VEHICLE_STARTED, modifiers );
	}

	// Player got in
	if ( HasCondition( COND_PASSENGER_PLAYER_EXITED_VEHICLE ) )
	{
		CPropJeepEpisodic *pJalopy = dynamic_cast<CPropJeepEpisodic*>(m_hVehicle.Get());
		if( pJalopy != NULL && pJalopy->NumRadarContacts() > 0 )
		{
			SpeakIfAllowed( TLK_PASSENGER_PLAYER_EXITED, "radar_has_targets" );
		}
		else
		{
			SpeakIfAllowed( TLK_PASSENGER_PLAYER_EXITED );
		}
	}

	// Player got out
	if ( HasCondition( COND_PASSENGER_PLAYER_ENTERED_VEHICLE ) )
	{
		SpeakIfAllowed( TLK_PASSENGER_PLAYER_ENTERED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not we should jostle at this moment
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanPlayJostle( bool bLargeJostle )
{
	// We've been told to suppress the jostle
	if ( m_flNextJostleTime > gpGlobals->curtime )
		return false;

	// Can't do this if we're at a high readiness level
	if ( m_hCompanion && m_hCompanion->ShouldBeAiming() )
		return false;

	// Can't do this when we're upside-down
	if ( HasCondition( COND_PASSENGER_OVERTURNED ) )
		return false;

	// Allow our normal impact code to handle this one instead
	if ( HasCondition( COND_PASSENGER_HARD_IMPACT ) || IsCurSchedule( SCHED_PASSENGER_IMPACT ) )
		return false;

	if ( bLargeJostle )
	{
		// Don't bother under certain circumstances
		if ( IsCurSchedule( SCHED_PASSENGER_COWER ) || 
			 IsCurSchedule( SCHED_PASSENGER_FIDGET ) )
			return false;
	}
	else
	{
		// Don't interrupt a larger gesture
		if ( GetOuter()->IsPlayingGesture( ACT_PASSENGER_GESTURE_JOSTLE_LARGE ) || GetOuter()->IsPlayingGesture( ACT_PASSENGER_GESTURE_JOSTLE_LARGE_STIMULATED ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions we can comment on or react to while riding in the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherVehicleStateConditions( void )
{
	// Gather the base class
	BaseClass::GatherVehicleStateConditions();

	// See if we're going to collide with anything soon
	GatherVehicleCollisionConditions( m_vehicleState.m_vecLastLocalVelocity );

	// Say anything we're meant to through the response rules
	SpeakVehicleConditions();
}

//-----------------------------------------------------------------------------
// Purpose: Handles exit failure notifications
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::OnExitVehicleFailed( void )
{
	m_nExitAttempts++;
}

//-----------------------------------------------------------------------------
// Purpose: Track how long we've been overturned
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::UpdateStuckStatus( void )
{
	if ( m_hVehicle == NULL )
		return;

	// Always clear this to start out with
	ClearCondition( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE );

	// If we can't exit the vehicle, then don't bother with these checks
	if ( m_hVehicle->NPC_CanExitVehicle( GetOuter(), true ) == false )
		return;

	bool bVisibleToPlayer = false;
	bool bPlayerInVehicle = false;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	if ( pPlayer )
	{
		bVisibleToPlayer = pPlayer->FInViewCone( GetOuter()->GetAbsOrigin() );
		bPlayerInVehicle = pPlayer->IsInAVehicle();
	}

	// If we're not overturned, just reset our counter
	if ( m_vehicleState.m_bWasOverturned == false )
	{
		m_flOverturnedDuration = 0.0f;
		m_flUnseenDuration = 0.0f;
	}
	else
	{
		// Add up the time since we last checked
		m_flOverturnedDuration += ( gpGlobals->curtime - GetLastThink() );
	}

	// Warn about being stuck upside-down if it's been long enough
	if ( m_flOverturnedDuration > MIN_OVERTURNED_WARN_DURATION && m_flNextOverturnWarning < gpGlobals->curtime )
	{
		SetCondition( COND_PASSENGER_WARN_OVERTURNED );
	}

	// If the player can see us or is still in the vehicle, we never exit
	if ( bVisibleToPlayer || bPlayerInVehicle )
	{
		// Reset our timer
		m_flUnseenDuration = 0.0f;
		return;
	}

	// Add up the time since we last checked
	m_flUnseenDuration += ( gpGlobals->curtime - GetLastThink() );

	// If we've been overturned for long enough or tried to exit one too many times
	if ( m_vehicleState.m_bWasOverturned )
	{
		if ( m_flUnseenDuration > MIN_OVERTURNED_DURATION )
		{
			SetCondition( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE );
		}
	}
	else if ( m_nExitAttempts >= MIN_FAILED_EXIT_ATTEMPTS )
	{
		// The player can't be looking at us
		SetCondition( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions for our use in making decisions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherConditions( void )
{
	// Code below relies on these conditions being set first!
	BaseClass::GatherConditions();

	// We're not enabled
	if ( IsEnabled() == false )
		return;

	// In-car conditions
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// If we're jostling, then note that
		if ( HasCondition( COND_PASSENGER_ERRATIC_DRIVING ) )
		{
			if ( CanPlayJostle( true ) )
			{
				// Add the gesture to be played.  If it's already playing, the underlying function will simply opt-out
				int nSequence = GetOuter()->AddGesture( GetOuter()->NPC_TranslateActivity( ACT_PASSENGER_GESTURE_JOSTLE_LARGE ), true );

				GetOuter()->SetNextAttack( gpGlobals->curtime + ( GetOuter()->SequenceDuration( nSequence ) * 2.0f ) );
				GetOuter()->GetShotRegulator()->FireNoEarlierThan( GetOuter()->GetNextAttack() );

				// Push out our fidget into the future so that we don't act unnaturally over bumpy terrain
				ExtendFidgetDelay( random->RandomFloat( 1.5f, 3.0f ) );
			}
		}
		else if ( HasCondition( COND_PASSENGER_JOSTLE_SMALL ) )
		{
			if ( CanPlayJostle( false ) )
			{
				// Add the gesture to be played.  If it's already playing, the underlying function will simply opt-out
				GetOuter()->AddGesture( GetOuter()->NPC_TranslateActivity( ACT_PASSENGER_GESTURE_JOSTLE_SMALL ), true );

				// Push out our fidget into the future so that we don't act unnaturally over bumpy terrain
				ExtendFidgetDelay( random->RandomFloat( 1.5f, 3.0f ) );
			}
		}

		// See if we're upside-down
		UpdateStuckStatus();

		// See if we're able to fidget
		if ( CanFidget() )
		{
			SetCondition( COND_PASSENGER_CAN_FIDGET );
		}
	}

	// Clear this out
	ClearCondition( COND_PASSENGER_CAN_ENTER_IMMEDIATELY );

	// Make sure a vehicle doesn't stray from its mark
	if ( IsCurSchedule( SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE ) )
	{
		if ( m_VehicleMonitor.TargetMoved( m_hVehicle ) )
		{
			SetCondition( COND_PASSENGER_VEHICLE_MOVED_FROM_MARK );
		}

		// If we can get in the car right away, set us up to do so
		int nNearestSequence;
		if ( CanEnterVehicleImmediately( &nNearestSequence, &m_vecTargetPosition, &m_vecTargetAngles ) )
		{
			SetTransitionSequence( nNearestSequence );
			SetCondition( COND_PASSENGER_ENTERING );
			SetCondition( COND_PASSENGER_CAN_ENTER_IMMEDIATELY );
		}
	}

	// Clear the number for now
	m_nVisibleEnemies = 0;
	
	AIEnemiesIter_t iter;
	for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		if( GetOuter()->IRelationType( pEMemory->hEnemy ) == D_HT )
		{
			if( pEMemory->timeLastSeen == gpGlobals->curtime )
			{
				m_nVisibleEnemies++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::AimGun( void )
{
	// If there is no aiming target, return to center
	if ( GetEnemy() == NULL )
	{
		GetOuter()->RelaxAim();
		return;
	}

	// Otherwise try and shoot down the barrel
	Vector vecForward, vecRight, vecUp;
	GetOuter()->GetVectors( &vecForward, &vecRight, &vecUp );
	Vector vecTorso = GetAbsOrigin() + ( vecUp * 48.0f );

	Vector vecShootDir = GetOuter()->GetShootEnemyDir( vecTorso, false );
	
	Vector vecDirToEnemy = GetEnemy()->GetAbsOrigin() - vecTorso;
	VectorNormalize( vecDirToEnemy );

	bool bRightSide = ( DotProduct( vecDirToEnemy, vecRight ) > 0.0f );
	float flTargetDot = ( bRightSide ) ? -0.7f : 0.0f;

	if ( DotProduct( vecForward, vecDirToEnemy ) <= flTargetDot )
	{
		// Don't aim at something that's outside our reach
  		GetOuter()->RelaxAim();
	}
	else
	{
		// Aim at it
		GetOuter()->SetAim( vecShootDir );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allow us to deny selecting a schedule if we're not in a state to do so
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanSelectSchedule( void )
{
	if ( BaseClass::CanSelectSchedule() == false )
		return false;

	// We're in a period where we're allowing our base class to override us
	if ( m_flNextEnterAttempt > gpGlobals->curtime )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Deal with enter/exit of the vehicle
//-----------------------------------------------------------------------------
int	CAI_PassengerBehaviorCompanion::SelectTransitionSchedule( void )
{
	// Attempt to instantly enter the vehicle
	if ( HasCondition( COND_PASSENGER_CAN_ENTER_IMMEDIATELY ) )
	{
		// Snap to position and begin to animate into the seat
		EnterVehicleImmediately();
		return SCHED_PASSENGER_ENTER_VEHICLE_IMMEDIATELY;
	}

	// Entering schedule
	if ( HasCondition( COND_PASSENGER_ENTERING ) || m_PassengerIntent == PASSENGER_INTENT_ENTER )
	{
		if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
		{
			ClearCondition( COND_PASSENGER_ENTERING );
			m_PassengerIntent = PASSENGER_INTENT_NONE;
			return SCHED_NONE;
		}

		// Don't attempt to enter for a period of time
		if ( m_flNextEnterAttempt > gpGlobals->curtime )
			return SCHED_NONE;

		ClearCondition( COND_PASSENGER_ENTERING );

		// Failing that, run to the right place
		return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE;
	}

	return BaseClass::SelectTransitionSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Select schedules when we're riding in the car
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectScheduleInsideVehicle( void )
{
	// Overturned
	if ( HasCondition( COND_PASSENGER_OVERTURNED ) )
		return SCHED_PASSENGER_OVERTURNED;

	if ( HasCondition( COND_PASSENGER_HARD_IMPACT ) )
	{
		// Push out our fidget into the future so that we don't act unnaturally over bumpy terrain
		ExtendFidgetDelay( random->RandomFloat( 1.5f, 3.0f ) );
		m_flNextJostleTime = gpGlobals->curtime + random->RandomFloat( 2.5f, 4.0f );
		return SCHED_PASSENGER_IMPACT;
	}

	// Look for exiting the vehicle
	if ( HasCondition( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE ) )
		return SCHED_PASSENGER_EXIT_STUCK_VEHICLE;

	// Cower if we're about to get nailed
	if ( HasCondition( COND_HEAR_DANGER ) && IsCurSchedule( SCHED_PASSENGER_COWER ) == false )
	{
		SpeakIfAllowed( TLK_DANGER );
		return SCHED_PASSENGER_COWER;
	}

	// Fire on targets 
	if ( GetEnemy() )
	{
		// Limit how long we'll keep an enemy if there are many on screen
		if ( HasCondition( COND_NEW_ENEMY ) && m_nVisibleEnemies > 1 )
		{
			GetEnemies()->SetTimeValidEnemy( GetEnemy(), random->RandomFloat( 0.5f, 1.0f ) );
		}

		// Always face
		GetOuter()->AddLookTarget( GetEnemy(), 1.0f, 2.0f );
		
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && ( GetOuter()->GetShotRegulator()->IsInRestInterval() == false ) )
			return SCHED_PASSENGER_RANGE_ATTACK1;
	}

	// Reload when we have the chance
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) && HasCondition( COND_SEE_ENEMY ) == false )
			return SCHED_PASSENGER_RELOAD;

	// Say an overturned line
	if ( HasCondition( COND_PASSENGER_WARN_OVERTURNED ) )
	{
		SpeakIfAllowed( TLK_PASSENGER_REQUEST_UPRIGHT );
		m_flNextOverturnWarning = gpGlobals->curtime + random->RandomFloat( 5.0f, 10.0f );
		ClearCondition( COND_PASSENGER_WARN_OVERTURNED );
	}

	// Should we fidget?
	if ( HasCondition( COND_PASSENGER_CAN_FIDGET ) )
	{
		ExtendFidgetDelay( random->RandomFloat( 6.0f, 12.0f ) );
		return SCHED_PASSENGER_FIDGET;
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Select schedules while we're outside the car
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectScheduleOutsideVehicle( void )
{
	// FIXME: How can we get in here?
	Assert( m_hVehicle );
	if ( m_hVehicle == NULL )
		return SCHED_NONE;

	// Handle our mark moving
	if ( HasCondition( COND_PASSENGER_VEHICLE_MOVED_FROM_MARK ) )
	{
		// Reset our mark
		m_VehicleMonitor.SetMark( m_hVehicle, 36.0f );
		ClearCondition( COND_PASSENGER_VEHICLE_MOVED_FROM_MARK );
	}

	// If we want to get in, the try to do so
	if ( m_PassengerIntent == PASSENGER_INTENT_ENTER )
	{
		// If we're not attempting to enter the vehicle again, just fall to the base class
		if ( m_flNextEnterAttempt > gpGlobals->curtime )
			return BaseClass::SelectSchedule();

		// Otherwise try and enter thec car
		return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE;
	}
	
	// This means that we're outside the vehicle with no intent to enter, which should have disabled us!
	Disable();
	Assert( 0 );

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&vecCenter - 
//			flRadius - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool SphereWithinPlayerFOV( CBasePlayer *pPlayer, const Vector &vecCenter, float flRadius )
{
	// TODO: For safety sake, we might want to do a more fully qualified test against the frustum using the bbox

	// If the player can see us, then we can't enter immediately anyway
	if ( pPlayer == NULL )
		return false;

	// Find the length to the point
	Vector los = ( vecCenter - pPlayer->EyePosition() );
	float flLength = VectorNormalize( los );

	// Get the player's forward direction
	Vector vecPlayerForward;
	pPlayer->EyeVectors( &vecPlayerForward, NULL, NULL );

	// This is the additional number of degrees to add to account for our distance
	float flArcAddition = atan2( flRadius, flLength );

	// Find if the sphere is within our FOV
	float flDot = DotProduct( los, vecPlayerForward );
	float flPlayerFOV = cos( DEG2RAD( pPlayer->GetFOV() / 2.0f ) );
	
	return ( flDot > (flPlayerFOV-flArcAddition) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanEnterVehicleImmediately( int *pResultSequence, Vector *pResultPos, QAngle *pResultAngles )
{
	// Must wait a short time before trying to do this (otherwise we stack up on the player!)
	if ( ( gpGlobals->curtime - m_flEnterBeginTime ) < 0.5f )
		return false;

	// Vehicle can't be moving too quickly
	if ( GetVehicleSpeed() > 150 )
		return false;

	// If the player can see us, then we can't enter immediately anyway
	CBasePlayer *pPlayer = AI_GetSinglePlayer();	
	if ( pPlayer == NULL )
		return false;

	Vector vecPosition = GetOuter()->WorldSpaceCenter();
	float flRadius = GetOuter()->CollisionProp()->BoundingRadius2D();
	if ( SphereWithinPlayerFOV( pPlayer, vecPosition, flRadius ) )
		return false;

	// Reserve an entry point
	if ( ReserveEntryPoint( VEHICLE_SEAT_ANY ) == false )
		return false;

	// Get a list of all our animations
	const PassengerSeatAnims_t *pEntryAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_ENTRY );
	if ( pEntryAnims == NULL )
		return -1;

	// Get the ultimate position we'll end up at
	Vector vecStartPos, vecEndPos;
	QAngle vecStartAngles;
	if ( m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPosition( GetOuter(), &vecEndPos, NULL ) == false )
		return -1;

	// Categorize the passenger in terms of being on the left or right side of the vehicle
	Vector vecRight;
	m_hVehicle->GetVectors( NULL, &vecRight, NULL );
	
	CPlane lateralPlane;
	lateralPlane.InitializePlane( vecRight, m_hVehicle->WorldSpaceCenter() );

	bool bPlaneSide = lateralPlane.PointInFront( GetOuter()->GetAbsOrigin() );

	Vector	vecPassengerOffset = ( GetOuter()->WorldSpaceCenter() - GetOuter()->GetAbsOrigin() );

	const CPassengerSeatTransition *pTransition;
	float	flNearestDistSqr = FLT_MAX;
	float	flSeatDistSqr;
	int		nNearestSequence = -1;
	int		nSequence;
	Vector	vecNearestPos;
	QAngle	vecNearestAngles;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pEntryAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		pTransition = &pEntryAnims->Element(i);
		nSequence = GetOuter()->LookupSequence( STRING( pTransition->GetAnimationName() ) );
		if ( nSequence == -1 )
			continue;

		// Test this entry for validity
		if ( GetEntryPoint( nSequence, &vecStartPos, &vecStartAngles ) == false )
			continue;

		// See if the passenger would be visible if standing at this position
		if ( SphereWithinPlayerFOV( pPlayer, (vecStartPos+vecPassengerOffset), flRadius ) )
			continue;

		// Otherwise distance is the deciding factor
		flSeatDistSqr = ( vecStartPos - GetOuter()->GetAbsOrigin() ).LengthSqr();

		// We must be within a certain distance to the vehicle
		if ( flSeatDistSqr > Square( 25*12 ) )
			continue;

		// We cannot cross between the plane which splits the vehicle laterally in half down the middle
		// This avoids cases where the character magically ends up on one side of the vehicle after they were
		// clearly just on the other side.  
		if ( lateralPlane.PointInFront( vecStartPos ) != bPlaneSide )
			continue;

		// Closer, take it
		if ( flSeatDistSqr < flNearestDistSqr )
		{
			flNearestDistSqr = flSeatDistSqr;
			nNearestSequence = nSequence;
			vecNearestPos = vecStartPos;
			vecNearestAngles = vecStartAngles;
		}
	}

	// Fail if we didn't find anything
	if ( nNearestSequence == -1 )
		return false;

	// Return the results
	if ( pResultSequence )
	{
		*pResultSequence = nNearestSequence;
	}

	if ( pResultPos )
	{
		*pResultPos = vecNearestPos;
	}

	if ( pResultAngles )
	{
		*pResultAngles = vecNearestAngles;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Put us into the vehicle immediately
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::EnterVehicleImmediately( void )
{
	// Now play the animation
	GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );
	GetOuter()->GetNavigator()->ClearGoal();

	// Put us there and get going (no interpolation!)
	GetOuter()->Teleport( &m_vecTargetPosition, &m_vecTargetAngles, &vec3_origin );
	GetOuter()->IncrementInterpolationFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the schedule selection
// Output : int - Schedule to play
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectSchedule( void )
{
	// First, keep track of our transition state (enter/exit)
	int nSched = SelectTransitionSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	// Handle schedules based on our passenger state
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
	{
		nSched = SelectScheduleOutsideVehicle();
		if ( nSched != SCHED_NONE )
			return nSched;
	}
	else if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		nSched = SelectScheduleInsideVehicle();
		if ( nSched != SCHED_NONE )
			return nSched;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	switch( failedTask )
	{
	case TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT:
		{
			// This is not allowed!
			if ( GetPassengerState() != PASSENGER_STATE_OUTSIDE )
			{
				Assert( 0 );
				return SCHED_FAIL;
			}

			// If we're not close enough, then get nearer the target
			if ( UTIL_DistApprox( m_hVehicle->GetAbsOrigin(), GetOuter()->GetAbsOrigin() ) > PASSENGER_NEAR_VEHICLE_THRESHOLD )
				return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED;
		}
		
		// Fall through

	case TASK_GET_PATH_TO_NEAR_VEHICLE:
		m_flNextEnterAttempt = gpGlobals->curtime + 3.0f;
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: Start to enter the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::EnterVehicle( void )
{
	BaseClass::EnterVehicle();

	m_nExitAttempts = 0;
	m_VehicleMonitor.SetMark( m_hVehicle, 8.0f );
	m_flEnterBeginTime = gpGlobals->curtime;
	
	// Remove this flag because we're sitting so close we always think we're going to hit the player
	// FIXME: We need to store this state so we don't incorrectly restore it later
	GetOuter()->CapabilitiesRemove( bits_CAP_NO_HIT_PLAYER );

	// Discard enemies quickly
	GetOuter()->GetEnemies()->SetEnemyDiscardTime( 2.0f );

	SpeakIfAllowed( TLK_PASSENGER_BEGIN_ENTRANCE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::FinishEnterVehicle( void )
{
	BaseClass::FinishEnterVehicle();

	// We succeeded
	ResetVehicleEntryFailedState();

	// Push this out into the future so we don't always fidget immediately in the vehicle
	ExtendFidgetDelay( random->RandomFloat( 4.0, 15.0f ) );

	SpeakIfAllowed( TLK_PASSENGER_FINISH_ENTRANCE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::ExitVehicle( void )
{
	BaseClass::ExitVehicle();

	SpeakIfAllowed( TLK_PASSENGER_BEGIN_EXIT );
}

//-----------------------------------------------------------------------------
// Purpose: Vehicle has been completely exited
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::FinishExitVehicle( void )
{
	BaseClass::FinishExitVehicle();

	m_nExitAttempts = 0;
	m_VehicleMonitor.ClearMark();

	// FIXME: We need to store this state so we don't incorrectly restore it later
	GetOuter()->CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER );

	// FIXME: Restore this properly
	GetOuter()->GetEnemies()->SetEnemyDiscardTime( AI_DEF_ENEMY_DISCARD_TIME );

	SpeakIfAllowed( TLK_PASSENGER_FINISH_EXIT );
}

//-----------------------------------------------------------------------------
// Purpose: Tries to build a route to the entry point of the target vehicle.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::FindPathToVehicleEntryPoint( void )
{
	// Set our custom move name
	// bool bFindNearest = ( GetOuter()->m_NPCState == NPC_STATE_COMBAT || GetOuter()->m_NPCState == NPC_STATE_ALERT );
	bool bFindNearest = true;	// For the sake of quick gameplay, just make Alyx move directly!
	int nSequence = FindEntrySequence( bFindNearest );
	if ( nSequence  == -1 )
		return false;

	// We have to do this specially because the activities are not named
	SetTransitionSequence( nSequence );

	// Get the entry position
	Vector vecEntryPoint;
	QAngle vecEntryAngles;
	if ( GetEntryPoint( m_nTransitionSequence, &vecEntryPoint, &vecEntryAngles ) == false )
	{
		MarkVehicleEntryFailed( vecEntryPoint );
		return false;
	}

	// If we're already close enough, just succeed
	float flDistToGoalSqr = ( GetOuter()->GetAbsOrigin() - vecEntryPoint ).LengthSqr();
	if ( flDistToGoalSqr < Square(3*12) )
		return true;

	// Setup our goal
	AI_NavGoal_t goal( GOALTYPE_LOCATION );
	// goal.arrivalActivity = ACT_SCRIPT_CUSTOM_MOVE;
	goal.dest = vecEntryPoint;

	// See if we need a radial route around the car, to our goal
	if ( UseRadialRouteToEntryPoint( vecEntryPoint ) )
	{
		// Find the bounding radius of the vehicle
		Vector vecCenterPoint = m_hVehicle->WorldSpaceCenter();
		vecCenterPoint.z = vecEntryPoint.z;
		bool bClockwise;
		float flArc = GetArcToEntryPoint( vecCenterPoint, vecEntryPoint, bClockwise );
		float flRadius = m_hVehicle->CollisionProp()->BoundingRadius2D();

		// Try and set a radial route
		if ( GetOuter()->GetNavigator()->SetRadialGoal( vecEntryPoint, vecCenterPoint, flRadius, flArc, 64.0f, bClockwise ) == false )
		{
			// Try the opposite way
			flArc = 360.0f - flArc;

			// Try the opposite way around
			if ( GetOuter()->GetNavigator()->SetRadialGoal( vecEntryPoint, vecCenterPoint, flRadius, flArc, 64.0f, !bClockwise ) == false )
			{
				// Try and set a direct route as a last resort
				if ( GetOuter()->GetNavigator()->SetGoal( goal ) == false )
					return false;
			}
		}

		// We found a goal
		GetOuter()->GetNavigator()->SetArrivalDirection( vecEntryAngles );
		GetOuter()->GetNavigator()->SetArrivalSpeed( 64.0f );
		return true;
	}
	else
	{
		// Try and set a direct route
		if ( GetOuter()->GetNavigator()->SetGoal( goal ) )
		{
			GetOuter()->GetNavigator()->SetArrivalDirection( vecEntryAngles );
			GetOuter()->GetNavigator()->SetArrivalSpeed( 64.0f );
			return true;
		}
	}

	// We failed, so remember it
	MarkVehicleEntryFailed( vecEntryPoint );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Tests the route and position to see if it's valid
// Input  : &vecTestPos - position to test
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanExitAtPosition( const Vector &vecTestPos )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
		return false;

	// Can't be in our potential view
	if ( pPlayer->FInViewCone( vecTestPos ) )
		return false;

	// NOTE: There's no reason to do this since this is only called from a node's reported position
	// Find the exact ground at this position
	//Vector vecGroundPos;
	//if ( FindGroundAtPosition( vecTestPos, 16.0f, 64.0f, &vecGroundPos ) == false )
	//	return false;

	// Get the ultimate position we'll end up at
	Vector	vecStartPos;
	if ( m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPosition( GetOuter(), &vecStartPos, NULL ) == false )
		return false;

	// See if we can move from where we are to that position in space
	if ( IsValidTransitionPoint( vecStartPos, vecTestPos ) == false )
		return false;

	// Trace down to the ground
	// FIXME: This piece of code is redundant and happening in IsValidTransitionPoint() as well
	/*
	Vector vecGroundPos;
	if ( FindGroundAtPosition( vecTestPos, GetOuter()->StepHeight(), 64.0f, &vecGroundPos ) == false )
		return false;
	*/

	// Try and sweep a box through space and make sure it's clear of obstructions
	/*
	trace_t tr;
	CTraceFilterVehicleTransition skipFilter( GetOuter(), m_hVehicle, COLLISION_GROUP_NONE );
	
	// These are very approximated (and magical) numbers to allow passengers greater head room and leg room when transitioning
	Vector vecMins = GetOuter()->GetHullMins() + Vector( 0, 0, GetOuter()->StepHeight()*2.0f ); // FIXME: 
	Vector vecMaxs = GetOuter()->GetHullMaxs() - Vector( 0, 0, GetOuter()->StepHeight() );
	
	UTIL_TraceHull( GetOuter()->GetAbsOrigin(), vecGroundPos, vecMins, vecMaxs, MASK_NPCSOLID, &skipFilter, &tr );

	// If we're blocked, we can't get out there
	if ( tr.fraction < 1.0f || tr.allsolid || tr.startsolid )
	{
		if ( passenger_debug_transition.GetBool() )
		{
			NDebugOverlay::SweptBox( GetOuter()->GetAbsOrigin(), vecGroundPos, vecMins, GetOuter()->GetHullMaxs(), vec3_angle, 255, 0, 0, 64, 2.0f );
		}
		return false;
	}
	*/

	return true;
}

#define	NUM_EXIT_ITERATIONS	8

//-----------------------------------------------------------------------------
// Purpose: Find a position we can use to exit the vehicle via teleportation
// Input  : *vecResult - safe place to exit to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::GetStuckExitPos( Vector *vecResult )
{
	// Get our right direction
	Vector vecVehicleRight;
	m_hVehicle->GetVectors( NULL, &vecVehicleRight, NULL );
	
	// Get the vehicle's rough horizontal bounds
	float	flVehicleRadius = m_hVehicle->CollisionProp()->BoundingRadius2D();

	// Use the vehicle's center as our hub
	Vector	vecCenter = m_hVehicle->WorldSpaceCenter();

	// Angle whose tan is: y/x
	float	flCurAngle = atan2f( vecVehicleRight.y, vecVehicleRight.x );
	float	flAngleIncr = (M_PI*2.0f)/(float)NUM_EXIT_ITERATIONS;
	Vector	vecTestPos;

	// Test a number of discrete exit routes
	for ( int i = 0; i <= NUM_EXIT_ITERATIONS-1; i++ )
	{
		// Get our position
		SinCos( flCurAngle, &vecTestPos.y, &vecTestPos.x );
		vecTestPos.z = 0.0f;
		vecTestPos *= flVehicleRadius;
		vecTestPos += vecCenter;

		// Now find the nearest node and use that
		int nNearNode = GetOuter()->GetPathfinder()->NearestNodeToPoint( vecTestPos );
		if ( nNearNode != NO_NODE )
		{
			Vector vecNodePos = g_pBigAINet->GetNodePosition( GetOuter()->GetHullType(), nNearNode );

			// Test the position
			if ( CanExitAtPosition( vecNodePos ) )
			{
				// Take the result
				*vecResult = vecNodePos;
				return true;
			}

			// Move to the next iteration
			flCurAngle += flAngleIncr;
		}
	}

	// None found
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to get out of an overturned vehicle when the player isn't looking
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::ExitStuckVehicle( void )
{
	// Try and find an exit position
	Vector vecExitPos;
	if ( GetStuckExitPos( &vecExitPos ) == false )
		return false;

	// Detach from the parent
	GetOuter()->SetParent( NULL );

	// Do all necessary clean-up
	FinishExitVehicle();

	// Teleport to the destination 
	// TODO: Make sure that the player can't see this!
	GetOuter()->Teleport( &vecExitPos, &vec3_angle, &vec3_origin );
	GetOuter()->IncrementInterpolationFrame();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::StartTask( const Task_t *pTask )
{
	// We need to override these so we never face
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		if ( pTask->iTask == TASK_FACE_TARGET || 
			 pTask->iTask == TASK_FACE_ENEMY ||
			 pTask->iTask == TASK_FACE_IDEAL ||
			 pTask->iTask == TASK_FACE_HINTNODE ||
			 pTask->iTask == TASK_FACE_LASTPOSITION ||
			 pTask->iTask == TASK_FACE_PATH ||
			 pTask->iTask == TASK_FACE_PLAYER ||
			 pTask->iTask == TASK_FACE_REASONABLE ||
			 pTask->iTask == TASK_FACE_SAVEPOSITION ||
			 pTask->iTask == TASK_FACE_SCRIPT )
		{
			return TaskComplete();
		}
	}

	switch ( pTask->iTask )
	{
	case TASK_RUN_TO_VEHICLE_ENTRANCE:
		{
			// Get a move on!
			GetOuter()->GetNavigator()->SetMovementActivity( ACT_RUN );
		}
		break;

	case TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT:
		{
			if ( GetPassengerState() != PASSENGER_STATE_OUTSIDE )
			{
				Assert( 0 );
				TaskFail( "Trying to run while inside a vehicle!\n");
				return;
			}

			// Reserve an entry point
			if ( ReserveEntryPoint( VEHICLE_SEAT_ANY ) == false )
			{
				TaskFail( "No valid entry point!\n" );
				return;
			}

			// Find where we're going
			if ( FindPathToVehicleEntryPoint() )
			{
				TaskComplete();
				return;
			}

			// We didn't find a path
			TaskFail( "TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT: Unable to run to entry point" );
		}
		break;

	case TASK_GET_PATH_TO_TARGET:
		{
			GetOuter()->SetTarget( m_hVehicle );
			BaseClass::StartTask( pTask );
		}
		break;

	case TASK_GET_PATH_TO_NEAR_VEHICLE:
		{
			if ( m_hVehicle == NULL )
			{
				TaskFail("Lost vehicle pointer\n");
				return;
			}

			// Find the passenger offset we're going for
			Vector vecRight;
			m_hVehicle->GetVectors( NULL, &vecRight, NULL );
			Vector vecTargetOffset = vecRight * 64.0f;

			// Try and find a path near there
			AI_NavGoal_t goal( GOALTYPE_TARGETENT, vecTargetOffset, AIN_DEF_ACTIVITY, 64.0f, AIN_UPDATE_TARGET_POS, m_hVehicle );
			GetOuter()->SetTarget( m_hVehicle );
			if ( GetOuter()->GetNavigator()->SetGoal( goal ) )
			{
				TaskComplete();
				return;
			}

			TaskFail( "Unable to find path to get closer to vehicle!\n" );
			return;
		}

		break;

	case TASK_PASSENGER_RELOAD:
		{
			GetOuter()->SetIdealActivity( ACT_PASSENGER_RELOAD );
			return;
		}
		break;
	
	case TASK_PASSENGER_EXIT_STUCK_VEHICLE:
		{
			if ( ExitStuckVehicle() )
			{
				TaskComplete();
				return;
			}

			TaskFail("Unable to exit overturned vehicle!\n");
		}
		break;

	case TASK_PASSENGER_OVERTURNED:
		{
			// Go into our overturned animation
			if ( GetOuter()->GetActivity() != ACT_PASSENGER_OVERTURNED )
			{
				GetOuter()->SetActivity( ACT_RESET );
				GetOuter()->SetActivity( ACT_PASSENGER_OVERTURNED );
			}

			TaskComplete();
		}
		break;

	case TASK_PASSENGER_IMPACT:
		{
			// Stomp anything currently playing on top of us, this has to take priority
			GetOuter()->RemoveAllGestures();

			// Go into our impact animation
			GetOuter()->ResetIdealActivity( ACT_PASSENGER_IMPACT );
			
			// Delay for twice the duration of our impact animation
			int nSequence = GetOuter()->SelectWeightedSequence( ACT_PASSENGER_IMPACT ); 
			float flSeqDuration = GetOuter()->SequenceDuration( nSequence );
			float flStunTime = flSeqDuration  + random->RandomFloat( 1.0f, 2.0f );
			GetOuter()->SetNextAttack( gpGlobals->curtime + flStunTime );
			ExtendFidgetDelay( flStunTime );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::IsCurTaskContinuousMove( void )
{
	const Task_t *pCurTask = GetCurTask();
	if ( pCurTask && pCurTask->iTask == TASK_RUN_TO_VEHICLE_ENTRANCE )
		return true;

	return BaseClass::IsCurTaskContinuousMove();
}

//-----------------------------------------------------------------------------
// Purpose: Update our path if we're running towards the vehicle (since it can move)
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::UpdateVehicleEntrancePath( void )
{
	// If it's too soon to check again, don't bother
	if ( m_flEntraceUpdateTime > gpGlobals->curtime )
		return true;

	// Find out if we need to update
	if ( m_VehicleMonitor.TargetMoved2D( m_hVehicle ) == false )
	{
		m_flEntraceUpdateTime = gpGlobals->curtime + 0.5f;
		return true;
	}

	// Don't attempt again for some amount of time
	m_flEntraceUpdateTime = gpGlobals->curtime + 1.0f;

	int nSequence = FindEntrySequence( true );
	if ( nSequence  == -1 )
		return false;

	SetTransitionSequence( nSequence );

	// Get the entry position
	Vector vecEntryPoint;
	QAngle vecEntryAngles;
	if ( GetEntryPoint( m_nTransitionSequence, &vecEntryPoint, &vecEntryAngles ) == false )
		return false;

	// Move the entry point forward in time a bit to predict where it'll be
	Vector vecVehicleSpeed = m_hVehicle->GetSmoothedVelocity();

	// Tack on the smoothed velocity
	vecEntryPoint += vecVehicleSpeed; // one second

	// Update our entry point
	if ( GetOuter()->GetNavigator()->UpdateGoalPos( vecEntryPoint ) == false )
		return false;

	// Reset the goal angles
	GetNavigator()->SetArrivalDirection( vecEntryAngles );
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PASSENGER_RELOAD:
		{
			if ( GetOuter()->IsSequenceFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_PASSENGER_IMPACT:
		{
			if ( GetOuter()->IsSequenceFinished() )
			{
				TaskComplete();
				return;
			}
		}
		break;

	case TASK_RUN_TO_VEHICLE_ENTRANCE:
		{
			// Update our entrance point if we can
			if ( UpdateVehicleEntrancePath() == false )
			{
				TaskFail("Unable to find entrance to vehicle");
				break;
			}
			
			// See if we're close enough to our goal
			if ( GetOuter ()->GetNavigator()->IsGoalActive() == false )
			{
				// See if we're close enough now to enter the vehicle
				Vector vecEntryPoint;
				GetEntryPoint( m_nTransitionSequence, &vecEntryPoint );
				if ( ( vecEntryPoint - GetAbsOrigin() ).Length2DSqr() < Square( 36.0f ) )
				{
					if ( GetNavigator()->GetArrivalActivity() != ACT_INVALID )
					{
						SetActivity( GetNavigator()->GetArrivalActivity() );
					}
					
					TaskComplete();
				}
				else
				{
					TaskFail( "Unable to navigate to vehicle" );
				}
			}

			// Keep merrily going!
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add custom interrupt conditions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::BuildScheduleTestBits( void )
{
	// Always break on being able to exit
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE ) );
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_HARD_IMPACT) );
		
		if ( IsCurSchedule( SCHED_PASSENGER_OVERTURNED ) == false )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_OVERTURNED ) );
		}

		// Append the ability to break on fidgeting
		if ( IsCurSchedule( SCHED_PASSENGER_IDLE ) )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_CAN_FIDGET ) );
		}

		// Add this so we're prompt about exiting the vehicle when able to
		if ( m_PassengerIntent == PASSENGER_INTENT_EXIT )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_VEHICLE_STOPPED ) );
		}
	}

	BaseClass::BuildScheduleTestBits();
}
//-----------------------------------------------------------------------------
// Purpose: Determines if the passenger should take a radial route to the goal
// Input  : &vecEntryPoint - Point of entry
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::UseRadialRouteToEntryPoint( const Vector &vecEntryPoint )
{
	// Get the center position of the vehicle we'll radiate around
	Vector vecCenterPos = m_hVehicle->WorldSpaceCenter();
	vecCenterPos.z = vecEntryPoint.z;

	// Find out if we need to go around the vehicle 
	float flDistToVehicleCenter = ( vecCenterPos - GetOuter()->GetAbsOrigin() ).Length();
	float flDistToGoal = ( vecEntryPoint - GetOuter()->GetAbsOrigin() ).Length();
	if ( flDistToGoal > flDistToVehicleCenter )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Find the arc in degrees to reach our goal position
// Input  : &vecCenterPoint - Point around which the arc rotates
//			&vecEntryPoint - Point we're trying to reach
//			&bClockwise - If we should move clockwise or not to get there
// Output : float - degrees around arc to follow
//-----------------------------------------------------------------------------
float CAI_PassengerBehaviorCompanion::GetArcToEntryPoint( const Vector &vecCenterPoint, const Vector &vecEntryPoint, bool &bClockwise )
{
	// We want the entry point to be at the same level as the center to make this a two dimensional problem
	Vector vecEntryPointAdjusted = vecEntryPoint;
	vecEntryPointAdjusted.z = vecCenterPoint.z;

	// Direction from vehicle center to passenger
	Vector vecVehicleToPassenger = ( GetOuter()->GetAbsOrigin() - vecCenterPoint );
	VectorNormalize( vecVehicleToPassenger );

	// Direction from vehicle center to entry point
	Vector vecVehicleToEntry = ( vecEntryPointAdjusted - vecCenterPoint );
	VectorNormalize( vecVehicleToEntry );

	float flVehicleToPassengerYaw = UTIL_VecToYaw( vecVehicleToPassenger );
	float flVehicleToEntryYaw = UTIL_VecToYaw( vecVehicleToEntry );
	float flArcDist = UTIL_AngleDistance( flVehicleToEntryYaw, flVehicleToPassengerYaw );

	bClockwise = ( flArcDist < 0.0f );
	return fabs( flArcDist );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all failed points
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::ResetVehicleEntryFailedState( void )
{
	m_FailedEntryPositions.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Adds a failed position to the list and marks when it occurred
// Input  : &vecPosition - Position that failed
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::MarkVehicleEntryFailed( const Vector &vecPosition )
{
	FailPosition_t failPos;
	failPos.flTime = gpGlobals->curtime;
	failPos.vecPosition = vecPosition;
	m_FailedEntryPositions.AddToTail( failPos );

	// Show this as failed
	if ( passenger_debug_entry.GetBool() )
	{
		NDebugOverlay::Box( vecPosition, -Vector(8,8,8), Vector(8,8,8), 255, 0, 0, 0, 2.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if a vector is near enough to a previously failed position
// Input  : &vecPosition - position to test
// Output : Returns true if the point is near enough another to be considered equivalent
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::PointIsWithinEntryFailureRadius( const Vector &vecPosition )
{
	// Test this point against our known failed points and reject it if it's too near
	for ( int i = 0; i < m_FailedEntryPositions.Count(); i++ )
	{
		// If our time has expired, kill the position
		if ( ( gpGlobals->curtime - m_FailedEntryPositions[i].flTime ) > 3.0f )
		{
			// Show that we've cleared it
			if ( passenger_debug_entry.GetBool() )
			{
				NDebugOverlay::Box( m_FailedEntryPositions[i].vecPosition, -Vector(12,12,12), Vector(12,12,12), 255, 255, 0, 0, 2.0f );
			}

  			m_FailedEntryPositions.Remove( i );
			continue;
		}

		// See if this position is too near our last failed attempt
		if ( ( vecPosition - m_FailedEntryPositions[i].vecPosition ).LengthSqr() < Square(3*12) )
		{
			// Show that this was denied
			if ( passenger_debug_entry.GetBool() )
			{
				NDebugOverlay::Box( m_FailedEntryPositions[i].vecPosition, -Vector(12,12,12), Vector(12,12,12), 255, 0, 0, 128, 2.0f );
			}

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Find the proper sequence to use (weighted by priority or distance from current position)
//			to enter the vehicle.
// Input  : bNearest - Use distance as the criteria for a "best" sequence.  Otherwise the order of the
//					   seats is their priority.
// Output : int - sequence index
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::FindEntrySequence( bool bNearest /*= false*/ )
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
	float	flNearestDistSqr = FLT_MAX;
	float	flSeatDistSqr;
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

		// See if this entry position is in our list of known unreachable places
		if ( PointIsWithinEntryFailureRadius( vecStartPos ) )
			continue;

		// Check to see if we can use this
		if ( IsValidTransitionPoint( vecStartPos, vecEndPos ) )
		{
			// If we're just looking for the first, we're done
			if ( bNearest == false )
				return nSequence;

			// Otherwise distance is the deciding factor
			flSeatDistSqr = ( vecStartPos - GetOuter()->GetAbsOrigin() ).LengthSqr();

			// Closer, take it
			if ( flSeatDistSqr < flNearestDistSqr )
			{
				flNearestDistSqr = flSeatDistSqr;
				nNearestSequence = nSequence;
			}
		}

	}

	return nNearestSequence;
}

//-----------------------------------------------------------------------------
// Purpose: Override certain animations
//-----------------------------------------------------------------------------
Activity CAI_PassengerBehaviorCompanion::NPC_TranslateActivity( Activity activity )
{
	Activity newActivity = BaseClass::NPC_TranslateActivity( activity );

	// Handle animations from inside the vehicle
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Alter idle depending on the vehicle's state
		if ( newActivity == ACT_IDLE )
		{
			// Always play the overturned animation
			if ( m_vehicleState.m_bWasOverturned )
				return ACT_PASSENGER_OVERTURNED;
		}
	}

	return newActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanExitVehicle( void )
{
	if ( BaseClass::CanExitVehicle() == false )
		return false;

	// If we're tipped too much, we can't exit
	Vector vecUp;
	GetOuter()->GetVectors( NULL, NULL, &vecUp );
	if ( DotProduct( vecUp, Vector(0,0,1) ) < DOT_45DEGREE )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: NPC needs to get to their marks, so do so with urgent navigation
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::IsNavigationUrgent( void )
{
	// If we're running to the vehicle, do so urgently
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE && m_PassengerIntent == PASSENGER_INTENT_ENTER )
		return true;

	return BaseClass::IsNavigationUrgent();
}

//-----------------------------------------------------------------------------
// Purpose: Calculate our body lean based on our delta velocity
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::CalculateBodyLean( void )
{
	// Calculate our lateral displacement from a perfectly centered start
	float flLateralDisp = SimpleSplineRemapVal( m_vehicleState.m_vecLastAngles.z, 100.0f, -100.0f, -1.0f, 1.0f );
	flLateralDisp = clamp( flLateralDisp, -1.0f, 1.0f );

	// FIXME: Framerate dependent!
	m_flLastLateralLean = ( m_flLastLateralLean * 0.2f ) + ( flLateralDisp * 0.8f );

	// Here we can make Alyx do something different on an "extreme" lean condition
	if ( fabs( m_flLastLateralLean ) > 0.75f )
	{
		// Large lean, make us react?
	}

	// Set these parameters
	GetOuter()->SetPoseParameter( "vehicle_lean", m_flLastLateralLean );
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not we're allowed to fidget
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanFidget( void )
{
	// Can't fidget again too quickly
	if ( m_flNextFidgetTime > gpGlobals->curtime )
		return false;

	// FIXME: Really we want to check our readiness level at this point
	if ( GetOuter()->GetEnemy() != NULL )
		return false;

	// Don't fidget unless we're at low readiness
	if ( m_hCompanion && ( m_hCompanion->GetReadinessLevel() > AIRL_RELAXED ) )
		return false;

	// Don't fidget while we're in a script
	if ( GetOuter()->IsInAScript() || GetOuter()->GetIdealState() == NPC_STATE_SCRIPT || IsRunningScriptedScene( GetOuter() ) )
		return false;

	// If we're upside down, don't bother
	if ( HasCondition( COND_PASSENGER_OVERTURNED ) )
		return false;

	// Must be visible to the player
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer && pPlayer->FInViewCone( GetOuter()->EyePosition() ) == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Extends the fidget delay by the time specified
// Input  : flDuration - in seconds
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::ExtendFidgetDelay( float flDuration )
{
	// If we're already expired, just set this as the next time
	if ( m_flNextFidgetTime < gpGlobals->curtime )
	{
		m_flNextFidgetTime = gpGlobals->curtime + flDuration;
	}
	else
	{
		// Otherwise bump the delay farther into the future
		m_flNextFidgetTime += flDuration;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We never want to be marked as crouching when inside a vehicle
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::IsCrouching( void )
{
	return false;
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_PassengerBehaviorCompanion )
{
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE_AIM )
	DECLARE_ACTIVITY( ACT_PASSENGER_RELOAD )
	DECLARE_ACTIVITY( ACT_PASSENGER_OVERTURNED )
	DECLARE_ACTIVITY( ACT_PASSENGER_IMPACT )
	DECLARE_ACTIVITY( ACT_PASSENGER_IMPACT_WEAPON )
	DECLARE_ACTIVITY( ACT_PASSENGER_POINT )
	DECLARE_ACTIVITY( ACT_PASSENGER_POINT_BEHIND )
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE_READY )
	DECLARE_ACTIVITY( ACT_PASSENGER_GESTURE_JOSTLE_LARGE )
	DECLARE_ACTIVITY( ACT_PASSENGER_GESTURE_JOSTLE_SMALL )
	DECLARE_ACTIVITY( ACT_PASSENGER_GESTURE_JOSTLE_LARGE_STIMULATED )
	DECLARE_ACTIVITY( ACT_PASSENGER_GESTURE_JOSTLE_SMALL_STIMULATED )
	DECLARE_ACTIVITY( ACT_PASSENGER_COWER_IN )
	DECLARE_ACTIVITY( ACT_PASSENGER_COWER_LOOP )
	DECLARE_ACTIVITY( ACT_PASSENGER_COWER_OUT )
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE_FIDGET )

	DECLARE_TASK( TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT )
	DECLARE_TASK( TASK_GET_PATH_TO_NEAR_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_RELOAD )
	DECLARE_TASK( TASK_PASSENGER_EXIT_STUCK_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_OVERTURNED )
	DECLARE_TASK( TASK_PASSENGER_IMPACT )
	DECLARE_TASK( TASK_RUN_TO_VEHICLE_ENTRANCE )

	DECLARE_CONDITION( COND_PASSENGER_VEHICLE_MOVED_FROM_MARK )
	DECLARE_CONDITION( COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE )
	DECLARE_CONDITION( COND_PASSENGER_WARN_OVERTURNED )
	DECLARE_CONDITION( COND_PASSENGER_WARN_COLLISION )
	DECLARE_CONDITION( COND_PASSENGER_CAN_FIDGET )
	DECLARE_CONDITION( COND_PASSENGER_CAN_ENTER_IMMEDIATELY )

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_TOLERANCE_DISTANCE		36"	// 3 ft
		"		TASK_SET_ROUTE_SEARCH_TIME		5"
		"		TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT	0"
		"		TASK_RUN_TO_VEHICLE_ENTRANCE	0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_PASSENGER_ENTER_VEHICLE"
		""
		"	Interrupts"
		"		COND_PASSENGER_CAN_ENTER_IMMEDIATELY"
		"		COND_PASSENGER_CANCEL_ENTER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_PASSENGER_ENTER_VEHICLE_PAUSE"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_TOLERANCE_DISTANCE		36"
		"		TASK_SET_ROUTE_SEARCH_TIME		3"
		"		TASK_GET_PATH_TO_NEAR_VEHICLE	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_PASSENGER_CANCEL_ENTER"
	)

	DEFINE_SCHEDULE
	( 
		SCHED_PASSENGER_ENTER_VEHICLE_PAUSE,

		"	Tasks"
		"		TASK_STOP_MOVING			1"
		"		TASK_FACE_TARGET			0"
		"		TASK_WAIT					2"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_PASSENGER_CANCEL_ENTER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_HEAR_DANGER"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
	)
	
	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_EXIT_STUCK_VEHICLE,

		"	Tasks"
		"		TASK_PASSENGER_EXIT_STUCK_VEHICLE		0"
		""
		"	Interrupts"
	)
	
	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RELOAD,

		"	Tasks"
		"		TASK_PASSENGER_RELOAD		0"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_OVERTURNED,

		"	Tasks"
		"		TASK_PASSENGER_OVERTURNED	0"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_IMPACT,

		"	Tasks"
		"		TASK_PASSENGER_IMPACT	0"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_ENTER_VEHICLE_IMMEDIATELY,

		"	Tasks"
		"		TASK_PASSENGER_ATTACH_TO_VEHICLE	0"
		"		TASK_PASSENGER_ENTER_VEHICLE		0"
		""
		"	Interrupts"
		"		COND_NO_CUSTOM_INTERRUPTS"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_COWER,
		
		"	Tasks"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_PASSENGER_COWER_IN"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_PASSENGER_COWER_LOOP"
		"		TASK_WAIT_UNTIL_NO_DANGER_SOUND		0"
		"		TASK_WAIT							2"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_PASSENGER_COWER_OUT"
		""
		"	Interrupts"
		"		COND_NO_CUSTOM_INTERRUPTS"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_FIDGET,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_PASSENGER_IDLE_FIDGET"
		""
		"	Interrupts"
		"		COND_NO_CUSTOM_INTERRUPTS"
	)

	AI_END_CUSTOM_SCHEDULE_PROVIDER()
}
