//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Zombies on cars!
//
//=============================================================================

#include "cbase.h"
#include "npcevent.h"
#include "ai_motor.h"
#include "ai_senses.h"
#include "vehicle_jeep_episodic.h"
#include "npc_alyx_episodic.h"
#include "ai_behavior_passenger_zombie.h"

#define JUMP_ATTACH_DIST_THRESHOLD			1000
#define JUMP_ATTACH_FACING_THRESHOLD		DOT_45DEGREE

#define ATTACH_PREDICTION_INTERVAL			0.2f
#define ATTACH_PREDICTION_FACING_THRESHOLD	0.75f
#define	ATTACH_PREDICTION_DIST_THRESHOLD	128

int ACT_PASSENGER_MELEE_ATTACK1;
int ACT_PASSENGER_THREATEN;
int ACT_PASSENGER_FLINCH;
int ACT_PASSENGER_ZOMBIE_LEAP_LOOP;

BEGIN_DATADESC( CAI_PassengerBehaviorZombie )

	DEFINE_FIELD( m_flLastVerticalLean, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLastLateralLean,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextLeapTime,		FIELD_TIME ),

END_DATADESC();

extern int AE_PASSENGER_PHYSICS_PUSH;

//==============================================================================================
// Passenger damage table
//==============================================================================================
static impactentry_t zombieLinearTable[] =
{
	{ 200*200, 100 },
};

static impactentry_t zombieAngularTable[] =
{
	{ 100*100, 100 },
};

impactdamagetable_t gZombiePassengerImpactDamageTable =
{
		zombieLinearTable,
		zombieAngularTable,

		ARRAYSIZE(zombieLinearTable),
		ARRAYSIZE(zombieAngularTable),

		24*24,		// minimum linear speed squared
		360*360,	// minimum angular speed squared (360 deg/s to cause spin/slice damage)
		2,			// can't take damage from anything under 2kg

		5,			// anything less than 5kg is "small"
		5,			// never take more than 5 pts of damage from anything under 5kg
		36*36,		// <5kg objects must go faster than 36 in/s to do damage

		VPHYSICS_LARGE_OBJECT_MASS,		// large mass in kg 
		4,			// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
		5,			// large mass falling scale (emphasize falling/crushing damage over sideways impacts since the stress will kill you anyway)
		0.0f,		// min vel
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CAI_PassengerBehaviorZombie::CAI_PassengerBehaviorZombie( void ) : 
m_flLastVerticalLean( 0.0f ), 
m_flLastLateralLean( 0.0f ),
m_flNextLeapTime( 0.0f )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorZombie::CanEnterVehicle( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Translate into vehicle passengers
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::TranslateSchedule( int scheduleType )
{
	// We do different animations when inside the vehicle
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		if ( scheduleType == SCHED_MELEE_ATTACK1 )
			return SCHED_PASSENGER_ZOMBIE_MELEE_ATTACK1;

		if ( scheduleType == SCHED_RANGE_ATTACK1 )
			return SCHED_PASSENGER_ZOMBIE_RANGE_ATTACK1;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CAI_PassengerBehaviorZombie::NPC_TranslateActivity( Activity activity )
{
	Activity nNewActivity = BaseClass::NPC_TranslateActivity( activity );
	if ( activity == ACT_IDLE )
		return (Activity) ACT_PASSENGER_IDLE;

	return nNewActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Suppress melee attacks against enemies for the given duration
// Input  : flDuration - Amount of time to suppress the attacks
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::SuppressAttack( float flDuration )
{
	GetOuter()->SetNextAttack( gpGlobals->curtime + flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if an enemy is inside a vehicle or not
// Output : Returns true if the enemy is outside the vehicle.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorZombie::EnemyInVehicle( void )
{
	// Obviously they're not...
	if ( GetOuter()->GetEnemy() == NULL )
		return false;

	// See if they're in a vehicle, currently
	CBaseCombatCharacter *pCCEnemy = GetOuter()->GetEnemy()->MyCombatCharacterPointer();
	if ( pCCEnemy && pCCEnemy->IsInAVehicle() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Select a schedule when we're outside of the vehicle
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::SelectOutsideSchedule( void )
{
	// Attaching to target
	if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		return SCHED_PASSENGER_ZOMBIE_RANGE_ATTACK1;

	// Attack the player if we're able
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		return SCHED_MELEE_ATTACK1;

	// Attach to the vehicle
	if ( HasCondition( COND_PASSENGER_ZOMBIE_CAN_ATTACH_TO_VEHICLE ) )
		return SCHED_PASSENGER_ZOMBIE_ATTACH;

	// Otherwise chase after him
	return SCHED_PASSENGER_ZOMBIE_RUN_TO_VEHICLE;
}

//-----------------------------------------------------------------------------
// Purpose: Pick a schedule for being "inside" the vehicle
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::SelectInsideSchedule( void )
{
	// Attacking target
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		return SCHED_PASSENGER_ZOMBIE_MELEE_ATTACK1;

	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: Move the zombie to the vehicle
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::SelectSchedule( void )
{
	// See if our enemy got out
	if ( GetOuter()->GetEnemy() != NULL && EnemyInVehicle() == false  )
	{
		if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
		{
			// Exit the vehicle
			SetCondition( COND_PASSENGER_EXITING );
		}
		else if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
		{
			// Our target has left the vehicle and we're outside as well, so give up
			Disable();
			return BaseClass::SelectSchedule();
		}
	}

	// Entering schedule
	if ( HasCondition( COND_PASSENGER_ENTERING ) )
	{
		ClearCondition( COND_PASSENGER_ENTERING );
		return SCHED_PASSENGER_ZOMBIE_ENTER_VEHICLE;
	}

	// Exiting schedule
	if ( HasCondition( COND_PASSENGER_EXITING ) )
	{
		ClearCondition( COND_PASSENGER_EXITING );
		return SCHED_PASSENGER_ZOMBIE_EXIT_VEHICLE;
	}

	// Select different schedules based on our state
	PassengerState_e nState = GetPassengerState();
	int nNewSchedule = SCHED_NONE;

	if ( nState == PASSENGER_STATE_INSIDE )
	{
		nNewSchedule = SelectInsideSchedule();
		if ( nNewSchedule != SCHED_NONE )
			return nNewSchedule;
	}
	else if ( nState == PASSENGER_STATE_OUTSIDE )
	{
		nNewSchedule = SelectOutsideSchedule();
		if ( nNewSchedule != SCHED_NONE )
			return nNewSchedule;
	}

	// Worst case he just stands here
	Assert(0);
	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorZombie::CanJumpToAttachToVehicle( void )
{
	// FIXME: Probably move this up one level and out of this function
	if ( m_flNextLeapTime > gpGlobals->curtime )
		return false;

	// Predict an attachment jump
	CBaseEntity *pEnemy = GetOuter()->GetEnemy();

	Vector	vecPredictedPosition;
	UTIL_PredictedPosition( pEnemy, 1.0f, &vecPredictedPosition );

	float flDist = UTIL_DistApprox( vecPredictedPosition, GetOuter()->GetAbsOrigin() );

	// If we're facing them enough, allow the jump
	if ( ( flDist < JUMP_ATTACH_DIST_THRESHOLD ) && UTIL_IsFacingWithinTolerance( GetOuter(), pEnemy, JUMP_ATTACH_FACING_THRESHOLD ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if we can jump to be on the enemy's vehicle
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CAI_PassengerBehaviorZombie::CanBeOnEnemyVehicle( void )
{
	CBaseCombatCharacter *pEnemy = ToBaseCombatCharacter( GetOuter()->GetEnemy() );
	if ( pEnemy != NULL )
	{
		IServerVehicle *pVehicle = pEnemy->GetVehicle();
		if ( pVehicle && pVehicle->NPC_HasAvailableSeat( GetRoleName() ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Always clear the base conditions
	ClearCondition( COND_CAN_MELEE_ATTACK1 );

	// Behavior when outside the vehicle
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
	{
		if ( CanBeOnEnemyVehicle() && CanJumpToAttachToVehicle() )
		{
			SetCondition( COND_CAN_RANGE_ATTACK1 );
		}
		
		// Determine if we can latch on to the vehicle (out of sight)
		ClearCondition( COND_PASSENGER_ZOMBIE_CAN_ATTACH_TO_VEHICLE );
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		
		if ( pPlayer != NULL && 
			 GetOuter()->GetEnemy() == pPlayer && 
			 pPlayer->GetVehicleEntity() == m_hVehicle )
		{
			// Can't be visible to the player and must be close enough
			bool bNotVisibleToPlayer = ( pPlayer->FInViewCone( GetOuter() ) == false );
			float flDistSqr = ( pPlayer->GetAbsOrigin() - GetOuter()->GetAbsOrigin() ).LengthSqr();
			bool bInRange = ( flDistSqr < Square(250.0f) );
			if ( bNotVisibleToPlayer && bInRange )
			{
				// We can latch on and "enter" the vehicle
				SetCondition( COND_PASSENGER_ZOMBIE_CAN_ATTACH_TO_VEHICLE );
			}
			else if ( bNotVisibleToPlayer == false && flDistSqr < Square(128.0f) )
			{
				// Otherwise just hit the vehicle in anger
				SetCondition( COND_CAN_MELEE_ATTACK1 );
			}
		}
	}

	// Behavior when on the car
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Check for melee attack
		if ( GetOuter()->GetNextAttack() < gpGlobals->curtime )
		{
			SetCondition( COND_CAN_MELEE_ATTACK1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle death case
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_hVehicle )
	{
		// Stop taking messages from the vehicle
		m_hVehicle->RemovePhysicsChild( GetOuter() );
		m_hVehicle->NPC_RemovePassenger( GetOuter() );
		m_hVehicle->NPC_FinishedExitVehicle( GetOuter(), false );
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Build our custom interrupt cases for the behavior
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::BuildScheduleTestBits( void )
{
	// Always interrupt when we need to get in or out
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
	{
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_CAN_RANGE_ATTACK1 ) );
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_PASSENGER_ENTERING ) );
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose: Get the absolute position of the desired attachment point
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::GetAttachmentPoint( Vector *vecPoint )
{
	Vector vecEntryOffset, vecFinalOffset;
	GetEntryTarget( &vecEntryOffset, NULL );
	VectorRotate( vecEntryOffset, m_hVehicle->GetAbsAngles(), vecFinalOffset );
	*vecPoint = ( m_hVehicle->GetAbsOrigin() + vecFinalOffset );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::FindExitSequence( void )
{
	// Get a list of all our animations
	const PassengerSeatAnims_t *pExitAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_EXIT );
	if ( pExitAnims == NULL )
		return -1;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pExitAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		int nSequence = GetOuter()->LookupSequence( STRING( pExitAnims->Element(i).GetAnimationName() ) );
		Assert( nSequence != -1 );
		if ( nSequence == -1 )
			continue;

		return nSequence;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::StartDismount( void )
{
	// Leap off the vehicle
	int nSequence = FindExitSequence();
	Assert( nSequence != -1 );

	SetTransitionSequence( nSequence );
	GetOuter()->SetIdealActivity( ACT_SCRIPT_CUSTOM_MOVE );

	// This removes the NPC from the vehicle's handling and fires all necessary outputs
	m_hVehicle->RemovePhysicsChild( GetOuter() );
	m_hVehicle->NPC_RemovePassenger( GetOuter() );
	m_hVehicle->NPC_FinishedExitVehicle( GetOuter(), (IsPassengerHostile()==false) );

	// Detach from the parent
	GetOuter()->SetParent( NULL );
	GetOuter()->SetMoveType( MOVETYPE_STEP );
	GetMotor()->SetYawLocked( false );

	QAngle vecAngles = GetAbsAngles();
	vecAngles.z = 0.0f;
	GetOuter()->SetAbsAngles( vecAngles );

	// HACK: Will this work?
	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if ( pPhysObj != NULL )
	{
		pPhysObj->EnableCollisions( true );
	}

	// Clear this
	m_PassengerIntent = PASSENGER_INTENT_NONE;
	SetPassengerState( PASSENGER_STATE_EXITING );

	// Get the velocity
	Vector vecUp, vecJumpDir;
	GetOuter()->GetVectors( &vecJumpDir, NULL, &vecUp );

	// Move back and up
	vecJumpDir *= random->RandomFloat( -400.0f, -500.0f );
	vecJumpDir += vecUp * 150.0f;
	GetOuter()->SetAbsVelocity( vecJumpDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::FinishDismount( void )
{
	SetPassengerState( PASSENGER_STATE_OUTSIDE );
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FACE_HINTNODE:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_SAVEPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_IDEAL:
	case TASK_FACE_SCRIPT:
	case TASK_FACE_PATH:
		TaskComplete();
		break;

	case TASK_PASSENGER_ZOMBIE_RANGE_ATTACK1:
		break;

	case TASK_MELEE_ATTACK1:
		{
			// Only override this if we're "in" the vehicle
			if ( GetPassengerState() != PASSENGER_STATE_INSIDE )
			{
				BaseClass::StartTask( pTask );
				break;
			}

			// Swipe
			GetOuter()->SetIdealActivity( (Activity) ACT_PASSENGER_MELEE_ATTACK1 );
			
			// Randomly attack again in the future
			float flWait = random->RandomFloat( 0.0f, 1.0f );
			SuppressAttack( flWait );
		}
		break;

	case TASK_PASSENGER_ZOMBIE_DISMOUNT:
		{
			// Start the process of dismounting from the vehicle
			StartDismount();
		}
		break;

	case TASK_PASSENGER_ZOMBIE_ATTACH:
		{
			if ( AttachToVehicle() )
			{
				TaskComplete();
				return;
			}

			TaskFail( "Unable to attach to vehicle!" );
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
void CAI_PassengerBehaviorZombie::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PASSENGER_ZOMBIE_RANGE_ATTACK1:
		{
			// Face the entry point
			Vector vecAttachPoint;
			GetAttachmentPoint( &vecAttachPoint );
			GetOuter()->GetMotor()->SetIdealYawToTarget( vecAttachPoint );

			// All done when you touch the ground
			if ( GetOuter()->GetFlags() & FL_ONGROUND )
			{
				m_flNextLeapTime = gpGlobals->curtime + 2.0f;
				TaskComplete();
				return;
			}
		}
		break;

	case TASK_MELEE_ATTACK1:

		if ( GetOuter()->IsSequenceFinished() )
		{
			TaskComplete();
		}

		break;

	case TASK_PASSENGER_ZOMBIE_DISMOUNT:
		{
			if ( GetOuter()->IsSequenceFinished() )
			{
				// Completely separate from the vehicle
				FinishDismount();
				TaskComplete();
			}

			break;
		}

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the relative cost of an entry point based on facing
// Input  : &vecEntryPos - Position we're evaluating
// Output : Returns the cost as a modified distance value
//-----------------------------------------------------------------------------
float CAI_PassengerBehaviorZombie::GetEntryPointCost( const Vector &vecEntryPos )
{
	// FIXME: We don't care about cost any longer!
	return 1.0f;

	// Find the direction from us to the entry point
	Vector vecEntryDir = ( vecEntryPos - GetAbsOrigin() );
	float flCost = VectorNormalize( vecEntryDir );
	
	// Get our current facing
	Vector vecDir;
	GetOuter()->GetVectors( &vecDir, NULL, NULL );

	// Scale our cost by how closely it matches our facing
	float flDot = DotProduct( vecEntryDir, vecDir );
	if ( flDot < 0.0f )
		return FLT_MAX;

	flCost *= RemapValClamped( flDot, 1.0f, 0.0f, 1.0f, 2.0f );

	return flCost;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNearest - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorZombie::FindEntrySequence( bool bNearest /*= false*/ )
{
	// Get a list of all our animations
	const PassengerSeatAnims_t *pEntryAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_ENTRY );
	if ( pEntryAnims == NULL )
		return -1;

	Vector vecStartPos;
	const CPassengerSeatTransition *pTransition;
	float flBestCost = FLT_MAX;
	float flCost;
	int nBestSequence = -1;
	int nSequence = -1;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pEntryAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		pTransition = &pEntryAnims->Element(i);
		nSequence = GetOuter()->LookupSequence( STRING( pTransition->GetAnimationName() ) );

		Assert( nSequence != -1 );
		if ( nSequence == -1 )
			continue;

		// Test this entry for validity
		GetEntryPoint( nSequence, &vecStartPos );

		// Evaluate the cost
		flCost = GetEntryPointCost( vecStartPos );
		if ( flCost < flBestCost )
		{
			nBestSequence = nSequence;
			flBestCost = flCost;
			continue;
		}
	}

	return nBestSequence;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::ExitVehicle( void )
{
	BaseClass::ExitVehicle();

	// Remove us as a passenger
	m_hVehicle->NPC_RemovePassenger( GetOuter() );
	m_hVehicle->NPC_FinishedExitVehicle( GetOuter(), false );

}

//-----------------------------------------------------------------------------
// Purpose: Calculate our body lean based on our delta velocity
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::CalculateBodyLean( void )
{
	// Calculate our lateral displacement from a perfectly centered start
	float flLateralDisp = SimpleSplineRemapVal( m_vehicleState.m_vecLastAngles.z, 100.0f, -100.0f, -1.0f, 1.0f );
	flLateralDisp = clamp( flLateralDisp, -1.0f, 1.0f );

	// FIXME: Framerate dependant!
	m_flLastLateralLean = ( m_flLastLateralLean * 0.2f ) + ( flLateralDisp * 0.8f );

	// Factor in a "stun" if the zombie was moved too far off course
	if ( fabs( m_flLastLateralLean ) > 0.75f )
	{
		SuppressAttack( 0.5f );
	}

	// Calc our vertical displacement
	float flVerticalDisp = SimpleSplineRemapVal( m_vehicleState.m_vecDeltaVelocity.z, -50.0f, 50.0f, -1.0f, 1.0f );
	flVerticalDisp = clamp( flVerticalDisp, -1.0f, 1.0f );

	// FIXME: Framerate dependant!
	m_flLastVerticalLean = ( m_flLastVerticalLean * 0.75f ) + ( flVerticalDisp * 0.25f );
	
	// Set these parameters
	GetOuter()->SetPoseParameter( "lean_lateral", m_flLastLateralLean );
	GetOuter()->SetPoseParameter( "lean_vertical", m_flLastVerticalLean );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::GatherVehicleStateConditions( void )
{
	// Call to the base
	BaseClass::GatherVehicleStateConditions();

	// Only do this if we're on the vehicle
	if ( GetPassengerState() != PASSENGER_STATE_INSIDE )
		return;

	// Calculate how our body is leaning
	CalculateBodyLean();

	// The forward delta of the vehicle
	float flLateralDelta = ( m_vehicleState.m_vecDeltaVelocity.x + m_vehicleState.m_vecDeltaVelocity.y );

	// Detect a sudden stop
	if ( flLateralDelta < -350.0f )
	{
		if ( m_hVehicle )
		{
			Vector vecDamageForce;
			m_hVehicle->GetVelocity( &vecDamageForce, NULL );
			VectorNormalize( vecDamageForce );
			vecDamageForce *= random->RandomFloat( 50000.0f, 60000.0f );
			
			//NDebugOverlay::HorzArrow( GetAbsOrigin(), GetAbsOrigin() + ( vecDamageForce * 256.0f ), 16.0f, 255, 0, 0, 16, true, 2.0f );

			// Fake it!
			CTakeDamageInfo info( m_hVehicle, m_hVehicle, vecDamageForce, GetOuter()->WorldSpaceCenter(), 200, (DMG_CRUSH|DMG_VEHICLE) );
			GetOuter()->TakeDamage( info );
		}
	}
	else if ( flLateralDelta < -150.0f )
	{
		// FIXME: Realistically this should interrupt and play a schedule to do it
		GetOuter()->SetIdealActivity( (Activity) ACT_PASSENGER_FLINCH );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_PASSENGER_PHYSICS_PUSH )
	{
		// Add a push into the vehicle
		float flForce = (float) atof( pEvent->options );
		AddPhysicsPush( flForce * 0.75f );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Attach to the vehicle if we're able
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorZombie::AttachToVehicle( void )
{
	// Must be able to enter the vehicle
	if ( m_hVehicle->NPC_CanEnterVehicle( GetOuter(), false ) == false )
		return false;

	// Reserve the seat
	if ( ReserveEntryPoint( VEHICLE_SEAT_ANY ) == false )
		return false;

	// Use the best one we've found
	int nSequence = FindEntrySequence();
	if ( nSequence == -1 )
		return false;

	// Take the transition sequence
	SetTransitionSequence( nSequence );

	// Get in the vehicle
	EnterVehicle();

	// Start our scripted sequence with any other passengers
	// Find Alyx
	// TODO: Iterate through the list of passengers in the vehicle and find one we can interact with
	CNPC_Alyx *pAlyx = CNPC_Alyx::GetAlyx();
	if ( pAlyx )
	{
		// Tell Alyx to play along!
		pAlyx->ForceVehicleInteraction( GetOuter()->GetSequenceName( nSequence ), GetOuter() );
	}

	return true;
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_PassengerBehaviorZombie )
{
	DECLARE_ACTIVITY( ACT_PASSENGER_MELEE_ATTACK1 )
	DECLARE_ACTIVITY( ACT_PASSENGER_THREATEN )
	DECLARE_ACTIVITY( ACT_PASSENGER_FLINCH )
	DECLARE_ACTIVITY( ACT_PASSENGER_ZOMBIE_LEAP_LOOP )

	DECLARE_TASK( TASK_PASSENGER_ZOMBIE_RANGE_ATTACK1 )
	DECLARE_TASK( TASK_PASSENGER_ZOMBIE_DISMOUNT )
	DECLARE_TASK( TASK_PASSENGER_ZOMBIE_ATTACH )

	DECLARE_CONDITION( COND_PASSENGER_ZOMBIE_CAN_ATTACH_TO_VEHICLE )

	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_ENTER_VEHICLE,

		"	Tasks"
		"		TASK_PASSENGER_ATTACH_TO_VEHICLE	0"
		"		TASK_PASSENGER_ENTER_VEHICLE		0"
		""
		"	Interrupts"
		)

	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_EXIT_VEHICLE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_PASSENGER_IDLE"
		"		TASK_STOP_MOVING			0"
		"		TASK_PASSENGER_ZOMBIE_DISMOUNT	0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		)

	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_MELEE_ATTACK1		0"
		""
		"	Interrupts"
		)
	
	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_PASSENGER_RANGE_ATTACK1"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_PASSENGER_ZOMBIE_LEAP_LOOP"
		"		TASK_PASSENGER_ZOMBIE_RANGE_ATTACK1	0"
		"	"
		"	Interrupts"
		)

	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_RUN_TO_VEHICLE,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	2400"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_TASK_FAILED"
		"		COND_LOST_ENEMY"
		"		COND_PASSENGER_ZOMBIE_CAN_ATTACH_TO_VEHICLE"
		)

	DEFINE_SCHEDULE
		(
		SCHED_PASSENGER_ZOMBIE_ATTACH,

		"	Tasks"
		"		TASK_PASSENGER_ZOMBIE_ATTACH	0"
		""
		"	Interrupts"
		)

		AI_END_CUSTOM_SCHEDULE_PROVIDER()
}
