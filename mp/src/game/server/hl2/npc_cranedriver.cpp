//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "ai_network.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_task.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_route.h"
#include "entitylist.h"
#include "soundenvelope.h"
#include "gamerules.h"
#include "ndebugoverlay.h"
#include "soundflags.h"
#include "trains.h"
#include "globalstate.h"
#include "vehicle_base.h"
#include "npc_vehicledriver.h"
#include "vehicle_crane.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar g_debug_vehicledriver;

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_CRANE_RANGE_ATTACK1 = LAST_VEHICLEDRIVER_SCHED,
	SCHED_CRANE_FIND_LARGE_OBJECT,
	SCHED_CRANE_PICKUP_OBJECT,
	SCHED_CRANE_FORCED_GO,
	SCHED_CRANE_CHASE_ENEMY,
	SCHED_CRANE_FORCED_DROP,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_CRANE_GET_POSITION_OVER_ENEMY = LAST_VEHICLEDRIVER_TASK,
	TASK_CRANE_GET_POSITION_OVER_LASTPOSITION,
	TASK_CRANE_GET_POSITION_OVER_OBJECT,
	TASK_CRANE_TURN_MAGNET_OFF,
	TASK_CRANE_FIND_OBJECT_TO_PICKUP,
	TASK_CRANE_DROP_MAGNET,
	TASK_END_FORCED_DROP,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_CraneDriver : public CNPC_VehicleDriver
{
	DECLARE_CLASS( CNPC_CraneDriver, CNPC_VehicleDriver );
public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void	Spawn( void );
	void	Activate( void );
	
	// AI
	int		RangeAttack1Conditions( float flDot, float flDist );
	int		TranslateSchedule( int scheduleType );
	int		SelectSchedule( void );
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	SetDesiredPosition( const Vector &vecPosition );

	// Driving
	void	DriveVehicle( void );
	bool	OverrideMove( float flInterval );

	// Inputs
	void	InputForcePickup( inputdata_t &inputdata );
	void	InputForceDrop( inputdata_t &inputdata );

protected:
	CHandle<CPropCrane>	m_hCrane;

	EHANDLE					m_hPickupTarget;
	float					m_flDistanceToTarget;
	CUtlVector< EHANDLE >	m_PreviouslyPickedUpObjects;
	bool					m_bForcedPickup;
	bool					m_bForcedDropoff;
	float					m_flDropWait;
	float					m_flReleasePause;
	float					m_flReleaseAt;

	// Outputs
	COutputEvent			m_OnPickedUpObject;
	COutputEvent			m_OnDroppedObject;
	COutputEvent			m_OnPausingBeforeDrop;
};

BEGIN_DATADESC( CNPC_CraneDriver )
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "ForcePickup", InputForcePickup ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ForceDrop", InputForceDrop ),

	//DEFINE_FIELD( m_hCrane, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPickupTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flDistanceToTarget, FIELD_FLOAT ),
	DEFINE_UTLVECTOR( m_PreviouslyPickedUpObjects, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bForcedPickup, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bForcedDropoff, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDropWait, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flReleasePause, FIELD_FLOAT, "releasepause" ),
	DEFINE_FIELD( m_flReleaseAt, FIELD_FLOAT ),

	// Outputs
	DEFINE_OUTPUT( m_OnPickedUpObject, "OnPickedUpObject" ),
	DEFINE_OUTPUT( m_OnDroppedObject, "OnDroppedObject" ),
	DEFINE_OUTPUT( m_OnPausingBeforeDrop, "OnPausingBeforeDrop" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_cranedriver, CNPC_CraneDriver );

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_CraneDriver::Spawn( void )
{
	BaseClass::Spawn();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );

	m_flDistTooFar = 2048.0;
	SetDistLook( 2048 );

	m_PreviouslyPickedUpObjects.Purge();
	m_hPickupTarget = NULL;
	m_bForcedPickup = false;
	m_bForcedDropoff = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::Activate( void )
{
	BaseClass::Activate();

	m_hCrane = dynamic_cast<CPropCrane*>((CBaseEntity*)m_hVehicleEntity);
	if ( !m_hCrane )
	{
		Warning( "npc_cranedriver %s couldn't find his crane named %s.\n", STRING(GetEntityName()), STRING(m_iszVehicleName) );
		UTIL_Remove( this );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CraneDriver::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( !HasCondition( COND_SEE_ENEMY ) )
		return COND_NONE;

	// Do our distance check in 2D
	Vector2D vecOrigin2D( m_hCrane->GetAbsOrigin().x, m_hCrane->GetAbsOrigin().y );
	Vector2D vecEnemy2D( GetEnemy()->GetAbsOrigin().x, GetEnemy()->GetAbsOrigin().y );
	flDist = (vecOrigin2D - vecEnemy2D).Length();

	// Maximum & Minimum size of the crane's reach
	if ( flDist > MAX_CRANE_FLAT_REACH )
		return COND_TOO_FAR_TO_ATTACK;

	// Crane can't reach any closer than this
	if ( flDist < MIN_CRANE_FLAT_REACH )
		return COND_TOO_CLOSE_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CraneDriver::SelectSchedule( void )
{
	if ( HasSpawnFlags(SF_VEHICLEDRIVER_INACTIVE) )
		return BaseClass::SelectSchedule();

	// If we've got an object to pickup, so go get it
	if ( m_hPickupTarget )
	{
		// Only clear the pickup target if we managed to pick something up
		if ( m_hCrane->GetTotalMassOnCrane() > 0 )
		{
			if ( m_bForcedPickup )
			{
				m_OnPickedUpObject.FireOutput( m_hPickupTarget, this );
			}

			// Remember what we dropped so we go try something else if we can.
			m_PreviouslyPickedUpObjects.AddToTail( m_hPickupTarget );
			m_hPickupTarget = NULL;
		}
		else
		{
			if ( m_NPCState == NPC_STATE_IDLE )
			{
				SetIdealState( NPC_STATE_ALERT );
			}
			return SCHED_CRANE_PICKUP_OBJECT;
		}
	}

	// If we're currently being forced to pickup something, do only that
	if ( m_bForcedPickup )
	{
		if ( m_hPickupTarget )
			return SCHED_CRANE_PICKUP_OBJECT;

		// We've picked up our target, we're waiting to be told where to put it
		return SCHED_IDLE_STAND;
	}

	// If we've been told to drop something off, do that
	if ( m_bForcedDropoff )
		return SCHED_CRANE_FORCED_DROP;

	switch ( m_NPCState )
	{
	case NPC_STATE_IDLE:
		break;

	case NPC_STATE_ALERT:
		break;

	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Do we have anything on the crane? If not, look for something
			if ( m_hCrane->GetTotalMassOnCrane() == 0 )
				return SCHED_CRANE_FIND_LARGE_OBJECT;

			// We've got something on the crane, so try and drop it on the enemy
			return SCHED_CRANE_RANGE_ATTACK1;
		}

		// We can't attack him, so if we don't have anything on the crane, grab something
		if ( m_hCrane->GetTotalMassOnCrane() == 0 )
			return SCHED_CRANE_FIND_LARGE_OBJECT;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CraneDriver::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
	case SCHED_COMBAT_FACE:
			// Vehicles can't rotate, so don't try and face
			return TranslateSchedule( SCHED_CHASE_ENEMY );

	case SCHED_ALERT_FACE:
			// Vehicles can't rotate, so don't try and face
			return SCHED_ALERT_STAND;

	case SCHED_FORCED_GO:
		return SCHED_CRANE_FORCED_GO;

	case SCHED_CHASE_ENEMY:
		return SCHED_CRANE_CHASE_ENEMY;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		break;

	case TASK_CRANE_GET_POSITION_OVER_ENEMY:
		{
			if ( !GetEnemy() )
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			SetDesiredPosition( GetEnemy()->GetAbsOrigin() );
			TaskComplete();
		}
		break;

	case TASK_CRANE_GET_POSITION_OVER_OBJECT:
		{
			if ( !m_hPickupTarget )
			{
				TaskFail("No object to pickup!");
				return;
			}

			SetDesiredPosition( m_hPickupTarget->GetAbsOrigin() );
			TaskComplete();
		}
		break;

	case TASK_CRANE_GET_POSITION_OVER_LASTPOSITION:
		{
			SetDesiredPosition( m_vecLastPosition );
			TaskComplete();
		}
		break;

	case TASK_CRANE_TURN_MAGNET_OFF:
		{
			// If we picked up something, and we were being forced to pick something up, fire our output
			if ( m_hCrane->GetTotalMassOnCrane() > 0 && m_bForcedDropoff )
			{
				// Are we supposed to pause first?
				if ( m_flReleasePause )
				{
					m_flReleaseAt = gpGlobals->curtime + m_flReleasePause;
					m_OnPausingBeforeDrop.FireOutput( this, this );
					return;
				}

				m_OnDroppedObject.FireOutput( this, this );
			}

			m_hCrane->TurnMagnetOff();
			TaskComplete();
		}
		break;

	case TASK_END_FORCED_DROP:
		{
			m_bForcedDropoff = false;
			TaskComplete();
		}
		break;

	case TASK_CRANE_FIND_OBJECT_TO_PICKUP:
		{
			Vector2D vecOrigin2D( m_hCrane->GetAbsOrigin().x, m_hCrane->GetAbsOrigin().y );

			// Find a large physics object within our reach to pickup
			float flLargestMass = 0;
			CBaseEntity *pLargestEntity = NULL;
			
			CBaseEntity *pList[1024];
			Vector delta( m_flDistTooFar, m_flDistTooFar, m_flDistTooFar*2 );
			int count = UTIL_EntitiesInBox( pList, 1024, m_hCrane->GetAbsOrigin() - delta, m_hCrane->GetAbsOrigin() + delta, 0 );
			for ( int i = 0; i < count; i++ )
			{
				if ( !pList[i] ) 
					continue;
				// Ignore the crane & the magnet
				if ( pList[i] == m_hCrane || pList[i] == m_hCrane->GetMagnet() )
					continue;
				if ( m_PreviouslyPickedUpObjects.Find( pList[i] ) != m_PreviouslyPickedUpObjects.InvalidIndex() )
					continue;

				// Get the VPhysics object
				IPhysicsObject *pPhysics = pList[i]->VPhysicsGetObject();
				if ( pPhysics && pList[i]->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					float flMass = pPhysics->GetMass();
					if ( flMass > flLargestMass && (flMass < MAXIMUM_CRANE_PICKUP_MASS) && (flMass > MINIMUM_CRANE_PICKUP_MASS) )
					{
						// Biggest one we've found so far

						// Now make sure it's within our reach
						// Do our distance check in 2D
						Vector2D vecOrigin2D( m_hCrane->GetAbsOrigin().x, m_hCrane->GetAbsOrigin().y );
						Vector2D vecEnemy2D( pList[i]->GetAbsOrigin().x, pList[i]->GetAbsOrigin().y );
						float flDist = (vecOrigin2D - vecEnemy2D).Length();
						// Maximum & Minimum size of the crane's reach
						if ( flDist > MAX_CRANE_FLAT_REACH )
							continue;
						if ( flDist < MIN_CRANE_FLAT_REACH )
							continue;

						flLargestMass = flMass;
						pLargestEntity = pList[i];
					}
				}
			}

			// If we didn't find anything new, clear our list of targets
			if ( !pLargestEntity )
			{
				m_PreviouslyPickedUpObjects.Purge();
			}

			if ( !pLargestEntity )
			{
				TaskFail("Couldn't find anything to pick up!");
				return;
			}

			m_hPickupTarget = pLargestEntity;
			TaskComplete();
		}
		break;

	case TASK_CRANE_DROP_MAGNET:
		{
			// Drop the magnet, but only end the task once the magnet's back up
			m_pVehicleInterface->NPC_SecondaryFire();

			// Don't check to see if drop's finished until this time is up.
			// This is necessary because the crane won't start dropping this
			// frame, and our cranedriver will think it's finished immediately.
			m_flDropWait = gpGlobals->curtime + 0.5;
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
void CNPC_CraneDriver::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		{
			// Is the magnet over the target, and are we not moving too fast?
			AngularImpulse angVel;
			Vector vecVelocity;
			CBaseEntity *pMagnet = m_hCrane->GetMagnet();
			IPhysicsObject *pVehiclePhysics = pMagnet->VPhysicsGetObject();
			pVehiclePhysics->GetVelocity( &vecVelocity, &angVel );
			float flVelocity = 100;
			float flDistance = 90;

			// More accurate on forced drops
			if ( m_bForcedPickup || m_bForcedDropoff )
			{
				flVelocity = 10;
				flDistance = 30;
			}

			if ( m_flDistanceToTarget < flDistance && m_hCrane->GetTurnRate() < 0.1 && vecVelocity.Length() < flVelocity )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_CRANE_DROP_MAGNET:
		{
			// Wait for the magnet to get back up
			if ( m_flDropWait < gpGlobals->curtime && !m_hCrane->IsDropping() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_CRANE_TURN_MAGNET_OFF:
		{
			// We're waiting for the pause length before dropping whatever's on our magnet
			if ( gpGlobals->curtime > m_flReleaseAt )
			{
				if ( m_bForcedDropoff )
				{
					m_OnDroppedObject.FireOutput( this, this );
				}

				m_hCrane->TurnMagnetOff();
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
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_CraneDriver::OverrideMove( float flInterval )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::SetDesiredPosition( const Vector &vecPosition )
{
	m_vecDesiredPosition = vecPosition;
	m_flDistanceToTarget = 999;
}

//-----------------------------------------------------------------------------
// Purpose: This takes the current place the NPC's trying to get to, figures out
//			what keys to press to get the vehicle to go there, and then sends
//			them to the vehicle.
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::DriveVehicle( void )
{
	// No targets?
	if ( !GetEnemy() && m_vecDesiredPosition == vec3_origin )
		return;

	Vector vecTarget = m_vecDesiredPosition;
	// Track our targets
	if ( m_hPickupTarget )
	{
		vecTarget = m_hPickupTarget->GetAbsOrigin();
	}
	else if ( !m_bForcedPickup && !m_bForcedDropoff && GetEnemy() )
	{
		vecTarget = GetEnemy()->GetAbsOrigin();
	}

	// Move the crane over the target
	// Use the crane type as a targeting point
	Vector vecCraneTip = m_hCrane->GetCraneTipPosition();
	Vector2D vecCraneTip2D( vecCraneTip.x, vecCraneTip.y );
	Vector2D vecTarget2D( vecTarget.x, vecTarget.y );
	Vector2D vecOrigin2D( m_hCrane->GetAbsOrigin().x, m_hCrane->GetAbsOrigin().y );

	if ( g_debug_vehicledriver.GetInt() )
	{
		NDebugOverlay::Box( vecTarget, -Vector(50,50,50), Vector(50,50,50), 0,255,0, true, 0.1 );
		NDebugOverlay::Box( vecCraneTip, -Vector(2,2,5000), Vector(2,2,5), 0,255,0, true, 0.1 );
		NDebugOverlay::Box( vecTarget, -Vector(2,2,5), Vector(2,2,5000), 0,255,0, true, 0.1 );
	}
	// Store off the distance to our target
	m_flDistanceToTarget = (vecTarget2D - vecCraneTip2D).Length();

	// First determine whether we need to extend / retract the arm
	float flDistToTarget = (vecOrigin2D - vecTarget2D).LengthSqr();
	float flDistToCurrent = (vecOrigin2D - vecCraneTip2D).LengthSqr();
	float flDelta = fabs(flDistToTarget - flDistToCurrent);
	// Slow down as we get closer, but do it based upon our current extension rate
	float flMinDelta = 50 + (50 * fabs(m_hCrane->GetExtensionRate() / CRANE_EXTENSION_RATE_MAX));
	flMinDelta *= flMinDelta;
	if ( flDelta > flMinDelta )
	{
		if ( flDistToCurrent > flDistToTarget )
		{
			// Retract
			m_pVehicleInterface->NPC_ThrottleReverse();
		}
		else if ( flDistToCurrent < flDistToTarget )
		{
			// Extend
			m_pVehicleInterface->NPC_ThrottleForward();
		}
	}
	else
	{
		m_pVehicleInterface->NPC_ThrottleCenter();
	}

	// Then figure out if we need to rotate. Do it all in 2D space.
	Vector vecRight, vecForward;
	m_hCrane->GetVectors( &vecForward, &vecRight, NULL );
	vecRight.z = 0;
	vecForward.z = 0;
	VectorNormalize( vecRight );
	VectorNormalize( vecForward );
	Vector vecToTarget = ( vecTarget - m_hCrane->GetAbsOrigin() );
	vecToTarget.z = 0;
	VectorNormalize( vecToTarget );
	float flDotRight = DotProduct( vecRight, vecToTarget );
	float flDotForward = DotProduct( vecForward, vecToTarget );

	// Start slowing if we're going to hit the point soon
	float flTurnInDeg = RAD2DEG( acos(flDotForward) );
	float flSpeed = m_hCrane->GetMaxTurnRate() * (flTurnInDeg / 15.0);
	flSpeed = MIN( m_hCrane->GetMaxTurnRate(), flSpeed );
	if ( fabs(flSpeed) < 0.05 )
	{
		// We're approaching the target, so stop turning
		m_pVehicleInterface->NPC_TurnCenter();
	}
	else
	{
		if ( flDotRight < 0 )
		{
			// Turn right
			m_pVehicleInterface->NPC_TurnRight( flSpeed );
		}
		else if ( flDotRight > 0 )
		{
			// Turn left
			m_pVehicleInterface->NPC_TurnLeft( flSpeed );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force the driver to pickup a specific entity
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::InputForcePickup( inputdata_t &inputdata )
{
	string_t iszPickupName = inputdata.value.StringID();
	if ( iszPickupName != NULL_STRING )
	{
		// Turn the magnet off now to drop anything we might have already on the magnet
		m_hCrane->TurnMagnetOff();
		m_hPickupTarget = gEntList.FindEntityByName( NULL, iszPickupName, NULL, inputdata.pActivator, inputdata.pCaller );
		m_bForcedPickup = true;
		m_bForcedDropoff = false;
		SetCondition( COND_PROVOKED );
		CLEARBITS( m_spawnflags, SF_VEHICLEDRIVER_INACTIVE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force the driver to drop his held entity at a specific point
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CraneDriver::InputForceDrop( inputdata_t &inputdata )
{
	string_t iszDropName = inputdata.value.StringID();
	if ( iszDropName != NULL_STRING )
	{
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, iszDropName, NULL, inputdata.pActivator, inputdata.pCaller );
		if ( !pEntity )
		{
			Warning("Crane couldn't find entity named %s\n", STRING(iszDropName) );
			return;
		}
		m_bForcedPickup = false;
		m_bForcedDropoff = true;
		SetDesiredPosition( pEntity->GetAbsOrigin() );
		SetCondition( COND_PROVOKED );
		CLEARBITS( m_spawnflags, SF_VEHICLEDRIVER_INACTIVE );
	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_cranedriver, CNPC_CraneDriver )

	//Tasks
	DECLARE_TASK( TASK_CRANE_GET_POSITION_OVER_ENEMY )
	DECLARE_TASK( TASK_CRANE_GET_POSITION_OVER_LASTPOSITION )
	DECLARE_TASK( TASK_CRANE_GET_POSITION_OVER_OBJECT )
	DECLARE_TASK( TASK_CRANE_TURN_MAGNET_OFF )
	DECLARE_TASK( TASK_END_FORCED_DROP )
	DECLARE_TASK( TASK_CRANE_FIND_OBJECT_TO_PICKUP )
	DECLARE_TASK( TASK_CRANE_DROP_MAGNET )

	//Schedules
	//==================================================
	// SCHED_ANTLION_CHASE_ENEMY_BURROW
	//==================================================

	DEFINE_SCHEDULE
	(
	SCHED_CRANE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_CRANE_GET_POSITION_OVER_ENEMY	0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_CRANE_TURN_MAGNET_OFF			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_TOO_FAR"
		"		COND_PROVOKED"
	)

	DEFINE_SCHEDULE
	(
	SCHED_CRANE_FIND_LARGE_OBJECT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_CRANE_FIND_OBJECT_TO_PICKUP	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_TOO_FAR"
	)

	DEFINE_SCHEDULE
	(
	SCHED_CRANE_PICKUP_OBJECT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_CRANE_GET_POSITION_OVER_OBJECT		0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_CRANE_DROP_MAGNET					0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_TOO_FAR"
		"		COND_PROVOKED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CRANE_FORCED_GO,

		"	Tasks"
		"		TASK_CRANE_GET_POSITION_OVER_LASTPOSITION	0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_CRANE_TURN_MAGNET_OFF					0"
		"		TASK_WAIT									2"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CRANE_CHASE_ENEMY,

		"	Tasks"
		"		TASK_CRANE_GET_POSITION_OVER_ENEMY			0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_WAIT									5"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_LOST_ENEMY"
		"		COND_PROVOKED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_CRANE_FORCED_DROP,

		"	Tasks"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_CRANE_TURN_MAGNET_OFF					0"
		"		TASK_END_FORCED_DROP						0"
		"		TASK_WAIT									2"
		"	"
		"	Interrupts"
	)
	
AI_END_CUSTOM_NPC()
