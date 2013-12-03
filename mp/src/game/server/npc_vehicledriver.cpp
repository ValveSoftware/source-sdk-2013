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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	DRIVER_DEBUG_PATH				1
#define	DRIVER_DEBUG_PATH_SPLINE		2

//------------------------------------
// 
//------------------------------------
ConVar g_debug_vehicledriver( "g_debug_vehicledriver", "0", FCVAR_CHEAT );

BEGIN_DATADESC( CNPC_VehicleDriver )
	DEFINE_KEYFIELD( m_iszVehicleName, FIELD_STRING, "vehicle" ),
//	DEFINE_FIELD( m_hVehicle,			FIELD_EHANDLE ),
	// DEFINE_FIELD( m_pVehicleInterface, FIELD_POINTER ),
	DEFINE_FIELD( m_hVehicleEntity,		FIELD_EHANDLE ),
	// DEFINE_FIELD( m_Waypoints,		FIELD_???? ),
	// DEFINE_FIELD( m_pCurrentWaypoint, FIELD_POINTER ),
	// DEFINE_FIELD( m_pNextWaypoint,	FIELD_POINTER ),
	DEFINE_FIELD( m_vecDesiredVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecDesiredPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPrevPoint,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPrevPrevPoint,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPostPoint,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPostPostPoint,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flDistanceAlongSpline, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flDriversMaxSpeed, FIELD_FLOAT, "drivermaxspeed" ),
	DEFINE_KEYFIELD( m_flDriversMinSpeed, FIELD_FLOAT, "driverminspeed" ),
	DEFINE_FIELD( m_flMaxSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flGoalSpeed,		FIELD_FLOAT ),
	//DEFINE_KEYFIELD( m_flInitialSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flSteering,			FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDriversMaxSpeed", InputSetDriversMaxSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDriversMinSpeed", InputSetDriversMinSpeed ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartForward", InputStartForward ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFiring", InputStartFiring ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopFiring", InputStopFiring ),
	DEFINE_INPUTFUNC( FIELD_STRING, "GotoPathCorner", InputGotoPathCorner ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_vehicledriver, CNPC_VehicleDriver );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_VehicleDriver::CNPC_VehicleDriver( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_VehicleDriver::~CNPC_VehicleDriver( void )
{
	ClearWaypoints();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_VehicleDriver::Spawn( void )
{
	Precache( );

	BaseClass::Spawn();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );

	SetModel( "models/roller_vehicledriver.mdl" );
	SetHullType(HULL_LARGE);
	SetHullSizeNormal();
	m_iMaxHealth = m_iHealth = 1;
	m_flFieldOfView	= VIEW_FIELD_FULL;

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );

	m_lifeState	= LIFE_ALIVE;
	SetCycle( 0 );
	ResetSequenceInfo();

	AddFlag( FL_NPC );

	m_flMaxSpeed = 0;
	m_flGoalSpeed = m_flInitialSpeed;

	m_vecDesiredVelocity = vec3_origin;
	m_vecPrevPoint = vec3_origin;
	m_vecPrevPrevPoint = vec3_origin;
	m_vecPostPoint = vec3_origin;
	m_vecPostPostPoint = vec3_origin;
	m_vecDesiredPosition = vec3_origin;
	m_flSteering = 45;
	m_flDistanceAlongSpline = 0.2;
	m_pCurrentWaypoint = m_pNextWaypoint = NULL;

	GetNavigator()->SetPathcornerPathfinding( false );

	NPCInit();

	m_takedamage = DAMAGE_NO;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::Precache( void )
{
	PrecacheModel( "models/roller_vehicledriver.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::Activate( void )
{
	BaseClass::Activate();

	// Restore doesn't need to do this
	if ( m_hVehicleEntity )
		return;

	// Make sure we've got a vehicle
	if ( m_iszVehicleName == NULL_STRING )
	{
		Warning( "npc_vehicledriver %s has no vehicle to drive.\n", STRING(GetEntityName()) );
		UTIL_Remove( this );
		return;
	}

	m_hVehicleEntity = (gEntList.FindEntityByName( NULL, STRING(m_iszVehicleName) ));
	if ( !m_hVehicleEntity )
	{
		Warning( "npc_vehicledriver %s couldn't find his vehicle named %s.\n", STRING(GetEntityName()), STRING(m_iszVehicleName) );
		UTIL_Remove( this );
		return;
	}

	m_pVehicleInterface = m_hVehicleEntity->GetServerVehicle();
	Assert( m_pVehicleInterface );
	if ( !m_pVehicleInterface->NPC_CanDrive() )
	{
		Warning( "npc_vehicledriver %s doesn't know how to drive vehicle %s.\n", STRING(GetEntityName()), STRING(m_hVehicleEntity->GetEntityName()) );
		UTIL_Remove( this );
		return;
	}

	// We've found our vehicle. Move to it and start following it.
	SetAbsOrigin( m_hVehicleEntity->WorldSpaceCenter() );
	m_pVehicleInterface->NPC_SetDriver( this );

	RecalculateSpeeds();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::OnRestore( void )
{
	BaseClass::OnRestore();
	if ( m_hVehicleEntity )
	{
		m_pVehicleInterface = m_hVehicleEntity->GetServerVehicle();
		Assert( m_pVehicleInterface );
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::UpdateOnRemove( void )
{
	// Leave our vehicle
	if ( m_pVehicleInterface )
	{
		m_pVehicleInterface->NPC_SetDriver( NULL );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::PrescheduleThink( void )
{
	if ( !m_hVehicleEntity )
	{
		m_pVehicleInterface = NULL;
		UTIL_Remove( this );
		return;
	}

	// Keep up with my vehicle
	SetAbsOrigin( m_hVehicleEntity->WorldSpaceCenter() );
	SetAbsAngles( m_hVehicleEntity->GetAbsAngles() );

	BaseClass::PrescheduleThink();

	if ( m_NPCState == NPC_STATE_IDLE )
	{
		m_pVehicleInterface->NPC_Brake();
		return;
	}

	// If we've been picked up by something (dropship probably), abort.
	if ( m_hVehicleEntity->GetParent() )
	{
		SetState( NPC_STATE_IDLE );
		ClearWaypoints();
		SetGoalEnt( NULL );
		return;
	}

	DriveVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_VehicleDriver::SelectSchedule( void )
{
	// Vehicle driver hangs in the air inside the vehicle, so we never need to fall to ground
	ClearCondition( COND_FLOATING_OFF_GROUND );

	if ( HasSpawnFlags(SF_VEHICLEDRIVER_INACTIVE) )
	{
		SetState( NPC_STATE_IDLE );
		return SCHED_VEHICLEDRIVER_INACTIVE;
	}

	if ( GetGoalEnt() )
		return SCHED_VEHICLEDRIVER_DRIVE_PATH;

	switch ( m_NPCState )
	{
	case NPC_STATE_IDLE:
		break;

	case NPC_STATE_ALERT:
		break;

	case NPC_STATE_COMBAT:
		{
			if ( HasCondition(COND_NEW_ENEMY) || HasCondition( COND_ENEMY_DEAD ) )
				return BaseClass::SelectSchedule();

			if ( HasCondition(COND_SEE_ENEMY) )
			{
				// we can see the enemy
				if ( HasCondition(COND_CAN_RANGE_ATTACK2) )
					return SCHED_RANGE_ATTACK2;
				if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
					return SCHED_RANGE_ATTACK1;

				// What to do here? Not necessarily easy to face enemy.
				//if ( HasCondition(COND_NOT_FACING_ATTACK) )
					//return SCHED_COMBAT_FACE;
			}

			// We can see him, but can't shoot him. Just wait and hope he comes closer.
			return SCHED_VEHICLEDRIVER_COMBAT_WAIT;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CNPC_VehicleDriver::RangeAttack1Conditions( float flDot, float flDist )
{
	// Vehicle not ready to fire again yet?
	if ( m_pVehicleInterface->Weapon_PrimaryCanFireAt() > gpGlobals->curtime )
		return 0;

	// Check weapon range
	float flMinRange, flMaxRange;
	m_pVehicleInterface->Weapon_PrimaryRanges( &flMinRange, &flMaxRange );
	if ( flDist < flMinRange )
		return COND_TOO_CLOSE_TO_ATTACK;
	if ( flDist > flMaxRange )
		return COND_TOO_FAR_TO_ATTACK;

	// Don't shoot backwards
	Vector vecForward;
	Vector vecToTarget = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
	VectorNormalize(vecToTarget);
	m_hVehicleEntity->GetVectors( &vecForward, NULL, NULL );
	float flForwardDot = DotProduct( vecForward, vecToTarget );
	if ( flForwardDot < 0 && fabs(flDot) < 0.5 )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//=========================================================
// RangeAttack2Conditions
//=========================================================
int CNPC_VehicleDriver::RangeAttack2Conditions( float flDot, float flDist )
{
	// Vehicle not ready to fire again yet?
	if ( m_pVehicleInterface->Weapon_SecondaryCanFireAt() > gpGlobals->curtime )
		return 0;

	// Check weapon range
	float flMinRange, flMaxRange;
	m_pVehicleInterface->Weapon_SecondaryRanges( &flMinRange, &flMaxRange );
	if ( flDist < flMinRange )
		return COND_TOO_CLOSE_TO_ATTACK;
	if ( flDist > flMaxRange )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_VehicleDriver::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
	case SCHED_COMBAT_FACE:
		{
			// Vehicles can't rotate, so don't try and face
			return TranslateSchedule( SCHED_CHASE_ENEMY );
		}
		break;

	case SCHED_ALERT_FACE:
		{
			// Vehicles can't rotate, so don't try and face
			return SCHED_ALERT_STAND;
		}
		break;

	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_FAIL:
		{
			return SCHED_FAIL;
		}
		break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		TaskComplete();
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		{
			// Vehicle ignores face commands, since it can't rotate on the spot.
			TaskComplete();
		}
		break;

	case TASK_VEHICLEDRIVER_GET_PATH:
		{
			if ( !GetGoalEnt() )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			CheckForTeleport();

			if ( g_debug_vehicledriver.GetInt() & DRIVER_DEBUG_PATH )
			{
				NDebugOverlay::Box( GetGoalEnt()->GetAbsOrigin(), -Vector(50,50,50), Vector(50,50,50), 255,255,255, true, 5);
			}

	  		AI_NavGoal_t goal( GOALTYPE_PATHCORNER, GetGoalEnt()->GetLocalOrigin(), ACT_WALK, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);
  			if ( !GetNavigator()->SetGoal( goal ) )
   			{
				TaskFail( FAIL_NO_ROUTE );
				return;
   			}

			TaskComplete();
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		{
			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->StopMoving();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}
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
void CNPC_VehicleDriver::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			// Vehicle driver has no animations, so fire a burst at the target
			CBaseEntity *pEnemy = GetEnemy();
			if ( pEnemy )
			{
				// TODO: Get a bodytarget from the firing point of the gun in the vehicle
				Vector vecTarget = GetEnemy()->BodyTarget( GetAbsOrigin(), false );
				m_pVehicleInterface->NPC_AimPrimaryWeapon( vecTarget );
				m_pVehicleInterface->NPC_PrimaryFire();
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
		}
		break;

	case TASK_RANGE_ATTACK2:
		{
			// Vehicle driver has no animations, so fire a burst at the target
			CBaseEntity *pEnemy = GetEnemy();
			if ( pEnemy )
			{
				// TODO: Get a bodytarget from the firing point of the gun in the vehicle
				Vector vecTarget = GetEnemy()->BodyTarget( GetAbsOrigin(), false );
				m_pVehicleInterface->NPC_AimSecondaryWeapon( vecTarget );
				m_pVehicleInterface->NPC_SecondaryFire();
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		{
			BaseClass::RunTask( pTask );

			if ( HasCondition(COND_SEE_ENEMY) )
			{
				// we can see the enemy
				if ( HasCondition(COND_CAN_RANGE_ATTACK2) )
				{
					ChainRunTask( TASK_RANGE_ATTACK2, pTask->flTaskData );
				}
				if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
				{
					ChainRunTask( TASK_RANGE_ATTACK1, pTask->flTaskData );
				}
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
void CNPC_VehicleDriver::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	BaseClass::GatherEnemyConditions(pEnemy);
}

//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
//-----------------------------------------------------------------------------
Disposition_t CNPC_VehicleDriver::IRelationType(CBaseEntity *pTarget)
{
	// If it's the player and they are a criminal, we hate them.
	if ( pTarget && pTarget->Classify() == CLASS_PLAYER)
	{
		if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
		{
			return(D_NU);
		}
	}

	return(BaseClass::IRelationType(pTarget));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_VehicleDriver::OverrideMove( float flInterval )
{
	if ( !m_hVehicleEntity )
		return true;

	// If we don't have a maxspeed, we've been stopped, so abort early
	// Or we've been picked up by something (dropship probably).
	if ( !m_flMaxSpeed || m_hVehicleEntity->GetParent() )
	{
		m_pVehicleInterface->NPC_Brake();
		return true;
	}

	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		if ( OverridePathMove( flInterval ) )
			return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::CalculatePostPoints( void )
{
	m_vecPostPoint = m_vecDesiredPosition;
	m_vecPostPostPoint = m_vecDesiredPosition;

	// If we have a waypoint beyond our current, use it instead.
	if ( !GetNavigator()->CurWaypointIsGoal() )
	{
		AI_Waypoint_t *pCurWaypoint = GetNavigator()->GetPath()->GetCurWaypoint();
		m_vecPostPoint = pCurWaypoint->GetNext()->GetPos();
		if ( pCurWaypoint->GetNext()->GetNext() )
		{
			m_vecPostPostPoint = pCurWaypoint->GetNext()->GetNext()->GetPos();
		}
		else
		{
			m_vecPostPostPoint = m_vecPostPoint;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destroy our current waypoints
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::ClearWaypoints( void )
{
	m_vecDesiredPosition = vec3_origin;
	if ( m_pCurrentWaypoint )
	{
		delete m_pCurrentWaypoint;
		m_pCurrentWaypoint = NULL;
	}
	if ( m_pNextWaypoint )
	{
		delete m_pNextWaypoint;
		m_pNextWaypoint = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've hit a waypoint. Handle it, and return true if this is the
//			end of the path.
//-----------------------------------------------------------------------------
bool CNPC_VehicleDriver::WaypointReached( void )
{
	// We reached our current waypoint.
	m_vecPrevPrevPoint = m_vecPrevPoint;
	m_vecPrevPoint = GetAbsOrigin();

	// If we've got to our goal, we're done here.
	if ( GetNavigator()->CurWaypointIsGoal() )
	{
		// Necessary for InPass outputs to be fired, is a no-op otherwise
		GetNavigator()->AdvancePath();
	
		// Stop pathing
		ClearWaypoints();
		TaskComplete();
		SetGoalEnt( NULL );
		return true;
	}

	AI_Waypoint_t *pCurWaypoint = GetNavigator()->GetPath()->GetCurWaypoint();
	if ( !pCurWaypoint )
		return false;

	// Check to see if the waypoint wants us to change speed
	if ( pCurWaypoint->Flags() & bits_WP_TO_PATHCORNER )
	{
		CBaseEntity *pEntity = pCurWaypoint->hPathCorner;
		if ( pEntity )
		{
			if ( pEntity->m_flSpeed > 0 )
			{
				if ( pEntity->m_flSpeed <= 1.0 )
				{
					m_flDriversMaxSpeed = pEntity->m_flSpeed;
					RecalculateSpeeds();
				}
				else
				{
					Warning("path_track %s tried to tell the npc_vehicledriver to set speed to %.3f. npc_vehicledriver only accepts values between 0 and 1.\n", STRING(pEntity->GetEntityName()), pEntity->m_flSpeed );
				}
			}
		}
	}

	// Get the waypoints for the next part of the path
	GetNavigator()->AdvancePath();
	if ( !GetNavigator()->GetPath()->GetCurWaypoint() )
	{
		ClearWaypoints();
		TaskComplete();
		SetGoalEnt( NULL );
		return true;
	}

	m_vecDesiredPosition = GetNavigator()->GetCurWaypointPos();	
	CalculatePostPoints();

	// Move to the next waypoint
	delete m_pCurrentWaypoint;
	m_pCurrentWaypoint = m_pNextWaypoint;
	m_Waypoints[1] = new CVehicleWaypoint( m_vecPrevPoint, m_vecDesiredPosition, m_vecPostPoint, m_vecPostPostPoint );
	m_pNextWaypoint = m_Waypoints[1];

	// Drop the spline marker back
	m_flDistanceAlongSpline = MAX( 0, m_flDistanceAlongSpline - 1.0 );

	CheckForTeleport();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_VehicleDriver::OverridePathMove( float flInterval )
{
	// Setup our initial path data if we've just started running a path
	if ( !m_pCurrentWaypoint )
	{
		m_vecPrevPoint = GetAbsOrigin();
		m_vecPrevPrevPoint = GetAbsOrigin();
		m_vecDesiredPosition = GetNavigator()->GetCurWaypointPos();	
		CalculatePostPoints();
	
		// Init our two waypoints
		m_Waypoints[0] = new CVehicleWaypoint( m_vecPrevPrevPoint, m_vecPrevPoint, m_vecDesiredPosition, m_vecPostPoint );
		m_Waypoints[1] = new CVehicleWaypoint( m_vecPrevPoint, m_vecDesiredPosition, m_vecPostPoint, m_vecPostPostPoint );
		m_pCurrentWaypoint = m_Waypoints[0];
		m_pNextWaypoint = m_Waypoints[1];

		m_flDistanceAlongSpline = 0.2;
	}

	// Have we reached our target? See if we've passed the current waypoint's plane.
	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	if ( BoxOnPlaneSide( vecAbsMins, vecAbsMaxs, &m_pCurrentWaypoint->planeWaypoint ) == 3 )
	{
		if ( WaypointReached() )
			return true;
	}

	// Did we bypass it and reach the next one already?
	if ( m_pNextWaypoint && BoxOnPlaneSide( vecAbsMins, vecAbsMaxs, &m_pNextWaypoint->planeWaypoint ) == 3 )
	{
		if ( WaypointReached() )
			return true;
	}

	// We may have just teleported, so check to make sure we have a waypoint
	if ( !m_pCurrentWaypoint || !m_pNextWaypoint )
		return false;

	// Figure out which spline we're trucking along
	CVehicleWaypoint *pCurrentSplineBeingTraversed = m_pCurrentWaypoint;
	if ( m_flDistanceAlongSpline > 1 )
	{
		pCurrentSplineBeingTraversed = m_pNextWaypoint;
	}

	// Get our current speed, and check it against the length of the spline to know how far to advance our marker
	AngularImpulse angVel;
	Vector vecVelocity;
	IPhysicsObject *pVehiclePhysics = m_hVehicleEntity->VPhysicsGetObject();

	if( !pVehiclePhysics )
	{
		// I think my vehicle has been destroyed.
		return false;
	}

	pVehiclePhysics->GetVelocity( &vecVelocity, &angVel );
	float flSpeed = vecVelocity.Length();
	float flIncTime = gpGlobals->curtime - GetLastThink();
	float flIncrement = flIncTime * (flSpeed / pCurrentSplineBeingTraversed->GetLength());

	// Now advance our point along the spline
	m_flDistanceAlongSpline = clamp( m_flDistanceAlongSpline + flIncrement, 0.f, 2.f);
	if ( m_flDistanceAlongSpline > 1 )
	{
		// We crossed the spline boundary
		pCurrentSplineBeingTraversed = m_pNextWaypoint;
	}

	Vector vSplinePoint = pCurrentSplineBeingTraversed->GetPointAt( m_flDistanceAlongSpline > 1 ? m_flDistanceAlongSpline-1 : m_flDistanceAlongSpline );
	Vector vSplineTangent = pCurrentSplineBeingTraversed->GetTangentAt( m_flDistanceAlongSpline > 1 ? m_flDistanceAlongSpline-1 : m_flDistanceAlongSpline );

	// Now that we've got the target spline point & tangent, use it to decide what our desired velocity is.
	// If we're close to the tangent, just use the tangent. Otherwise, Lerp towards it.
	Vector vecToDesired = (vSplinePoint - GetAbsOrigin());
	float flDistToDesired = VectorNormalize( vecToDesired );
	float flTangentLength = VectorNormalize( vSplineTangent );

	if ( flDistToDesired > (flTangentLength * 0.75) )
	{
		m_vecDesiredVelocity = vecToDesired * flTangentLength;
	}
	else
	{
		VectorLerp( vSplineTangent, vecToDesired * flTangentLength, (flDistToDesired / (flTangentLength * 0.5)), m_vecDesiredVelocity );
	}

	// Decrease speed according to the turn we're trying to make
	Vector vecRight;
	m_hVehicleEntity->GetVectors( NULL, &vecRight, NULL );
	Vector vecNormVel = m_vecDesiredVelocity;
	VectorNormalize( vecNormVel );
	float flDotRight = DotProduct( vecRight, vecNormVel );
	flSpeed = (1.0 - fabs(flDotRight));
	// Don't go slower than we've been told to go
	if ( flSpeed < m_flDriversMinSpeed )
	{
		flSpeed = m_flDriversMinSpeed;
	}
	m_vecDesiredVelocity = vecNormVel * (flSpeed * m_flMaxSpeed);

	// Bunch o'debug
	if ( g_debug_vehicledriver.GetInt() & DRIVER_DEBUG_PATH )
	{
		NDebugOverlay::Box( m_vecPrevPrevPoint, -Vector(15,15,15), Vector(15,15,15), 192,0,0, true, 0.1);
		NDebugOverlay::Box( m_vecPrevPoint, -Vector(20,20,20), Vector(20,20,20), 255,0,0, true, 0.1);
		NDebugOverlay::Box( m_vecPostPoint, -Vector(20,20,20), Vector(20,20,20), 0,192,0, true, 0.1);
		NDebugOverlay::Box( m_vecPostPostPoint, -Vector(20,20,20), Vector(20,20,20), 0,128,0, true, 0.1);
		NDebugOverlay::Box( vSplinePoint, -Vector(10,10,10), Vector(10,10,10), 0,0,255, true, 0.1);
		NDebugOverlay::Line( vSplinePoint, vSplinePoint + (vSplineTangent * 40), 0,0,255, true, 0.1);

		//NDebugOverlay::HorzArrow( pCurrentSplineBeingTraversed->splinePoints[0], pCurrentSplineBeingTraversed->splinePoints[1], 30, 255,255,255,0, false, 0.1f );
		//NDebugOverlay::HorzArrow( pCurrentSplineBeingTraversed->splinePoints[1], pCurrentSplineBeingTraversed->splinePoints[2], 20, 255,255,255,0, false, 0.1f );
		//NDebugOverlay::HorzArrow( pCurrentSplineBeingTraversed->splinePoints[2], pCurrentSplineBeingTraversed->splinePoints[3], 10, 255,255,255,0, false, 0.1f );

		// Draw the plane we're checking against for waypoint passing
		Vector vecPlaneRight;
		CrossProduct( m_pCurrentWaypoint->planeWaypoint.normal, Vector(0,0,1), vecPlaneRight );
		Vector vecPlane = m_pCurrentWaypoint->splinePoints[2];
		NDebugOverlay::Line( vecPlane + (vecPlaneRight * -100), vecPlane + (vecPlaneRight * 100), 255,0,0, true, 0.1);

		// Draw the next plane too
		CrossProduct( m_pNextWaypoint->planeWaypoint.normal, Vector(0,0,1), vecPlaneRight );
		vecPlane = m_pNextWaypoint->splinePoints[2];
		NDebugOverlay::Line( vecPlane + (vecPlaneRight * -100), vecPlane + (vecPlaneRight * 100), 192,0,0, true, 0.1);
	}

	if ( g_debug_vehicledriver.GetInt() & DRIVER_DEBUG_PATH_SPLINE )
	{
		for ( int i = 0; i < 10; i++ )
		{
			Vector vecTarget = m_pCurrentWaypoint->GetPointAt( 0.1 * i );
			Vector vecTangent = m_pCurrentWaypoint->GetTangentAt( 0.1 * i );
			VectorNormalize(vecTangent);
			NDebugOverlay::Box( vecTarget, -Vector(10,10,10), Vector(10,10,10), 255,0,0, true, 0.1 );
			NDebugOverlay::Line( vecTarget, vecTarget + (vecTangent * 10), 255,255,0, true, 0.1);
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This takes the current place the NPC's trying to get to, figures out
//			what keys to press to get the vehicle to go there, and then sends
//			them to the vehicle.
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::DriveVehicle( void )
{
	AngularImpulse angVel;
	Vector vecVelocity;
	IPhysicsObject *pVehiclePhysics = m_hVehicleEntity->VPhysicsGetObject();
	if ( !pVehiclePhysics )
		return;
	pVehiclePhysics->GetVelocity( &vecVelocity, &angVel );
	float flSpeed = VectorNormalize( vecVelocity );

	// If we have no target position to drive to, brake to a halt
	if ( !m_flMaxSpeed || m_vecDesiredPosition == vec3_origin )
	{
		if ( flSpeed > 1 )
		{
			m_pVehicleInterface->NPC_Brake();
		}
		return;
	}

	if ( g_debug_vehicledriver.GetInt() & DRIVER_DEBUG_PATH )
	{
		NDebugOverlay::Box(m_vecDesiredPosition, -Vector(20,20,20), Vector(20,20,20), 0,255,0, true, 0.1);
		NDebugOverlay::Line(GetAbsOrigin(), GetAbsOrigin() + m_vecDesiredVelocity, 0,255,0, true, 0.1);
	}

	m_flGoalSpeed = VectorNormalize(m_vecDesiredVelocity);

	// Is our target in front or behind us?
	Vector vecForward, vecRight;
	m_hVehicleEntity->GetVectors( &vecForward, &vecRight, NULL );
	float flDot = DotProduct( vecForward, m_vecDesiredVelocity );
	bool bBehind = ( flDot < 0 );
	float flVelDot = DotProduct( vecVelocity, m_vecDesiredVelocity );
	bool bGoingWrongWay = ( flVelDot < 0 );

	// Figure out whether we should accelerate / decelerate
	if ( bGoingWrongWay || (flSpeed < m_flGoalSpeed) )
	{
		// If it's behind us, go backwards not forwards
		if ( bBehind )
		{
			m_pVehicleInterface->NPC_ThrottleReverse();
		}
		else
		{
			m_pVehicleInterface->NPC_ThrottleForward();
		}
	}
	else
	{
		// Brake if we're go significantly too fast
		if ( (flSpeed - 200) > m_flGoalSpeed )
		{
			m_pVehicleInterface->NPC_Brake();
		}
		else
		{
			m_pVehicleInterface->NPC_ThrottleCenter();
		}
	}

	// Do we need to turn?
	float flDotRight = DotProduct( vecRight, m_vecDesiredVelocity );
	if ( bBehind )
	{
		// If we're driving backwards, flip our turning
		flDotRight *= -1;
	}
	// Map it to the vehicle's steering
	flDotRight *= (m_flSteering / 90);

	if ( flDotRight < 0 )
	{
		// Turn left
		m_pVehicleInterface->NPC_TurnLeft( -flDotRight );
	}
	else if ( flDotRight > 0 )
	{
		// Turn right
		m_pVehicleInterface->NPC_TurnRight( flDotRight );
	}
	else
	{
		m_pVehicleInterface->NPC_TurnCenter();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we should teleport to the current path corner
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::CheckForTeleport( void )
{
	if ( !GetGoalEnt() )
		return;

	CPathTrack *pTrack = dynamic_cast<CPathTrack *>( GetGoalEnt() );
	if ( !pTrack )
		return;

	// Does it have the teleport flag set?
	if ( pTrack->HasSpawnFlags( SF_PATH_TELEPORT ) )
	{
		IncrementInterpolationFrame();

		// Teleport the vehicle to the pathcorner
		Vector vecMins, vecMaxs;
		vecMins = m_hVehicleEntity->CollisionProp()->OBBMins();
		vecMaxs = m_hVehicleEntity->CollisionProp()->OBBMaxs();
		Vector vecTarget = pTrack->GetAbsOrigin() - (vecMins + vecMaxs) * 0.5;
		vecTarget.z += ((vecMaxs.z - vecMins.z) * 0.5) + 8;	// Safety buffer

		// Orient it to face the next point
		QAngle vecAngles = pTrack->GetAbsAngles();
		Vector vecToTarget = vec3_origin;
		if ( pTrack->GetNext() )
		{
			vecToTarget = (pTrack->GetNext()->GetAbsOrigin() - pTrack->GetAbsOrigin());
			VectorNormalize( vecToTarget );

			// Vehicles are rotated 90 degrees
			VectorAngles( vecToTarget, vecAngles );
			vecAngles[YAW] -= 90;
		}
		m_hVehicleEntity->Teleport( &vecTarget, &vecAngles, &vec3_origin );

		// Teleport the driver
		SetAbsOrigin( m_hVehicleEntity->WorldSpaceCenter() );
		SetAbsAngles( m_hVehicleEntity->GetAbsAngles() );

		m_vecPrevPoint = pTrack->GetAbsOrigin();

		// Move to the next waypoint, we've reached this one
		if ( GetNavigator()->GetPath() )
		{
			WaypointReached();
		}

		// Clear our waypoints, because the next waypoint is certainly invalid now.
		ClearWaypoints();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_VehicleDriver::GetDefaultNavGoalTolerance()
{
	return 48;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::RecalculateSpeeds( void )
{
	// Get data from the vehicle
	const vehicleparams_t *pParams = m_pVehicleInterface->GetVehicleParams();
	if ( pParams )
	{
		m_flMaxSpeed = pParams->engine.maxSpeed * m_flDriversMaxSpeed;
		m_flSteering = pParams->steering.degreesSlow;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputSetDriversMaxSpeed( inputdata_t &inputdata )
{
	m_flDriversMaxSpeed = inputdata.value.Float();

	RecalculateSpeeds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputSetDriversMinSpeed( inputdata_t &inputdata )
{
	m_flDriversMinSpeed = inputdata.value.Float();

	RecalculateSpeeds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputStartForward( inputdata_t &inputdata )
{
	CLEARBITS( m_spawnflags, SF_VEHICLEDRIVER_INACTIVE );
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		SetState( NPC_STATE_ALERT );
	}
	SetCondition( COND_PROVOKED );

	RecalculateSpeeds();
}

//-----------------------------------------------------------------------------
// Purpose: Tell the driver to stop moving
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputStop( inputdata_t &inputdata )
{
	m_flMaxSpeed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Tell the driver to start firing at targets
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputStartFiring( inputdata_t &inputdata )
{
	CLEARBITS( m_spawnflags, SF_VEHICLEDRIVER_INACTIVE );
	SetCondition( COND_PROVOKED );

	float flMinRange, flMaxRange;
	// If the vehicle has a weapon, set our capability
	if ( m_pVehicleInterface->NPC_HasPrimaryWeapon() )
	{
		CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
		m_pVehicleInterface->Weapon_PrimaryRanges( &flMinRange, &flMaxRange );

		// Ensure the look distances is long enough
		if ( m_flDistTooFar < flMaxRange || GetSenses()->GetDistLook() < flMaxRange )
		{
			m_flDistTooFar = flMaxRange;
			SetDistLook( flMaxRange );
		}
	}

	if ( m_pVehicleInterface->NPC_HasSecondaryWeapon() )
	{
		CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK2 );
		m_pVehicleInterface->Weapon_SecondaryRanges( &flMinRange, &flMaxRange );

		// Ensure the look distances is long enough
		if ( m_flDistTooFar < flMaxRange || GetSenses()->GetDistLook() < flMaxRange )
		{
			m_flDistTooFar = flMaxRange;
			SetDistLook( flMaxRange );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tell the driver to stop firing at targets
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputStopFiring( inputdata_t &inputdata )
{
	// If the vehicle has a weapon, set our capability
	CapabilitiesRemove( bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesRemove( bits_CAP_INNATE_RANGE_ATTACK2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_VehicleDriver::InputGotoPathCorner( inputdata_t &inputdata )
{
	string_t iszPathName = inputdata.value.StringID();
	if ( iszPathName != NULL_STRING )
	{
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, iszPathName );
		if ( !pEntity )
		{
			Warning("npc_vehicledriver %s couldn't find entity named %s\n", STRING(GetEntityName()), STRING(iszPathName) );
			return;
		}

		ClearWaypoints();

		// Drive to the point
		SetGoalEnt( pEntity );
		if ( m_NPCState == NPC_STATE_IDLE )
		{
			SetState( NPC_STATE_ALERT );
		}
		SetCondition( COND_PROVOKED );

		// Force him to start forward
		InputStartForward( inputdata );
	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_vehicledriver, CNPC_VehicleDriver )

	//Tasks
	DECLARE_TASK( TASK_VEHICLEDRIVER_GET_PATH )
	
	// Schedules
	DEFINE_SCHEDULE
	(
		SCHED_VEHICLEDRIVER_INACTIVE,

		"	Tasks"
		"		TASK_WAIT_INDEFINITE	0"
		""
		"	Interrupts"
		"		COND_PROVOKED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_VEHICLEDRIVER_COMBAT_WAIT,

		"	Tasks"
		"		TASK_WAIT				5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
	)

	DEFINE_SCHEDULE
	(
		SCHED_VEHICLEDRIVER_DRIVE_PATH,

		"	Tasks"
		"		TASK_VEHICLEDRIVER_GET_PATH		0"
		"		TASK_WALK_PATH					9999"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_WAIT_PVS					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_PROVOKED"
	)

AI_END_CUSTOM_NPC()
