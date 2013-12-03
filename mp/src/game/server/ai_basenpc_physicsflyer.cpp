//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for many flying NPCs
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc_physicsflyer.h"
#include "ai_route.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "physics_saverestore.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAI_BasePhysicsFlyingBot )

	DEFINE_FIELD( m_vCurrentVelocity,			FIELD_VECTOR),
	DEFINE_FIELD( m_vCurrentBanking,			FIELD_VECTOR),
	DEFINE_FIELD( m_vNoiseMod,				FIELD_VECTOR),
	DEFINE_FIELD( m_fHeadYaw,					FIELD_FLOAT),
	DEFINE_FIELD( m_vLastPatrolDir,			FIELD_VECTOR),

	DEFINE_PHYSPTR( m_pMotionController ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose : Override to return correct velocity
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BasePhysicsFlyingBot::GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity)
{
	Assert( GetMoveType() == MOVETYPE_VPHYSICS );
	if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->GetVelocity( vVelocity, vAngVelocity );
	}
	else
	{
		if ( vVelocity )
		{
			vVelocity->Init();
		}
		if ( vAngVelocity )
		{
			vAngVelocity->Init();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn head yaw into facing direction
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CAI_BasePhysicsFlyingBot::BodyAngles()
{
	return QAngle(0,m_fHeadYaw,0);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BasePhysicsFlyingBot::TurnHeadToTarget(float flInterval, const Vector &MoveTarget )
{
	float desYaw = UTIL_AngleDiff(VecToYaw(MoveTarget - GetLocalOrigin()), 0 );

	m_fHeadYaw = desYaw;

	return;

	// If I've flipped completely around, reverse angles
	float fYawDiff = m_fHeadYaw - desYaw;
	if (fYawDiff > 180)
	{
		m_fHeadYaw -= 360;
	}
	else if (fYawDiff < -180)
	{
		m_fHeadYaw += 360;
	}

	// RIGHT NOW, this affects every flying bot. This rate should be member data that individuals
	// can manipulate. THIS change for MANHACKS E3 2003 (sjb)
	float iRate = 0.8;

	// Make frame rate independent
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_fHeadYaw	   = (iRate * m_fHeadYaw) + (1-iRate)*desYaw;
		timeToUse -= 0.1;
	}

	while( m_fHeadYaw > 360 )  
	{
		m_fHeadYaw -= 360.0f;
	}

	while( m_fHeadYaw < 0 )
	{
		m_fHeadYaw += 360.f;
	}

	// SetBoneController( 0, m_fHeadYaw );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CAI_BasePhysicsFlyingBot::MinGroundDist(void)
{
	return 0;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BasePhysicsFlyingBot::VelocityToAvoidObstacles(float flInterval)
{
	// --------------------------------
	//  Avoid banging into stuff
	// --------------------------------
	trace_t tr;
	Vector vTravelDir = m_vCurrentVelocity*flInterval;
	Vector endPos = GetAbsOrigin() + vTravelDir;
	AI_TraceEntity( this, GetAbsOrigin(), endPos, MASK_NPCSOLID|CONTENTS_WATER, &tr);
	if (tr.fraction != 1.0)
	{	
		// Bounce off in normal 
		Vector vBounce = tr.plane.normal * 0.5 * m_vCurrentVelocity.Length();
		return (vBounce);
	}
	
	// --------------------------------
	// Try to remain above the ground.
	// --------------------------------
	float flMinGroundDist = MinGroundDist();
	AI_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -flMinGroundDist), 
		MASK_NPCSOLID_BRUSHONLY|CONTENTS_WATER, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction < 1)
	{
		// Clamp veloctiy
		if (tr.fraction < 0.1)
		{
			tr.fraction = 0.1;
		}

		return Vector(0, 0, 50/tr.fraction);
	}
	return vec3_origin;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BasePhysicsFlyingBot::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		// Skip as done via bone controller
		case TASK_FACE_ENEMY:
		{
			TaskComplete();
			break;
		}
		// Activity is just idle (have no run)
		case TASK_RUN_PATH:
		{
			GetNavigator()->SetMovementActivity(ACT_IDLE);
			TaskComplete();
			break;
		}
		// Don't check for run/walk activity
		case TASK_SCRIPT_RUN_TO_TARGET:
		case TASK_SCRIPT_WALK_TO_TARGET:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				if (!GetNavigator()->SetGoal( GOALTYPE_TARGETENT ) )
				{
					TaskFail(FAIL_NO_ROUTE);
					GetNavigator()->ClearGoal();
				}
			}
			TaskComplete();
			break;
		}
		// Override to get more to get a directional path
		case TASK_GET_PATH_TO_RANDOM_NODE:  
		{
			if ( GetNavigator()->SetRandomGoal( pTask->flTaskData, m_vLastPatrolDir ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
			break;
		}
		default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}

//------------------------------------------------------------------------------

void CAI_BasePhysicsFlyingBot::MoveToTarget(float flInterval, const Vector &MoveTarget)
{
	Assert(0);	// This must be overridden in the leaf classes
}

//------------------------------------------------------------------------------

AI_NavPathProgress_t CAI_BasePhysicsFlyingBot::ProgressFlyPath( 
	float flInterval,
	const CBaseEntity *pNewTarget, 
	unsigned collisionMask, 
	bool bNewTrySimplify, 
	float strictPointTolerance)
{
  	AI_ProgressFlyPathParams_t params( collisionMask );
	params.strictPointTolerance = strictPointTolerance;
	params.SetCurrent( pNewTarget, bNewTrySimplify );

	AI_NavPathProgress_t progress = GetNavigator()->ProgressFlyPath( params );
	
	switch ( progress )
	{
		case AINPP_NO_CHANGE:
		case AINPP_ADVANCED:
		{
			MoveToTarget(flInterval, GetNavigator()->GetCurWaypointPos());
			break;
		}
		
		case AINPP_COMPLETE:
		{
			TaskMovementComplete();
			break;
		}
		
		case AINPP_BLOCKED: // function is not supposed to test blocking, just simple path progression
		default:
		{
			AssertMsg( 0, ( "Unexpected result" ) );
			break;
		}
	}

	return progress;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_BasePhysicsFlyingBot::CAI_BasePhysicsFlyingBot()
{
#ifdef _DEBUG
	m_vCurrentVelocity.Init();
	m_vCurrentBanking.Init();
	m_vLastPatrolDir.Init();
#endif
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_BasePhysicsFlyingBot::~CAI_BasePhysicsFlyingBot( void )
{
	physenv->DestroyMotionController( m_pMotionController );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
bool CAI_BasePhysicsFlyingBot::CreateVPhysics( void )
{
	// Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	m_pMotionController = physenv->CreateMotionController( this );
	m_pMotionController->AttachObject( pPhysicsObject, true );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//			&chasePosition - 
//-----------------------------------------------------------------------------
void CAI_BasePhysicsFlyingBot::TranslateNavGoal( CBaseEntity *pTarget, Vector &chasePosition )
{
	Assert( pTarget != NULL );

	if ( pTarget == NULL )
	{
		chasePosition = vec3_origin;
		return;
	}

	// Chase their eyes
	chasePosition = pTarget->GetAbsOrigin() + pTarget->GetViewOffset();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pController - 
//			*pObject - 
//			deltaTime - 
//			&linear - 
//			&angular - 
// Output : IMotionEvent::simresult_e
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CAI_BasePhysicsFlyingBot::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	static int count;

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	// Assert( pPhysicsObject );
	if (!pPhysicsObject) 
		return SIM_NOTHING;

	// move
	Vector actualVelocity;
	AngularImpulse actualAngularVelocity;
	pPhysicsObject->GetVelocity( &actualVelocity, &actualAngularVelocity );
	linear = (m_vCurrentVelocity - actualVelocity) * (0.1 / deltaTime) * 10.0;

	/*
	DevMsg("Sim %d : %5.1f %5.1f %5.1f\n", count++,
		m_vCurrentVelocity.x - actualVelocity.x, 
		m_vCurrentVelocity.y - actualVelocity.y, 
		m_vCurrentVelocity.z - actualVelocity.z );
	*/

	// do angles.
	Vector actualPosition;
	QAngle actualAngles;
	pPhysicsObject->GetPosition( &actualPosition, &actualAngles ); 

	// FIXME: banking currently disabled, forces simple upright posture
	angular.x = (UTIL_AngleDiff( m_vCurrentBanking.z, actualAngles.z ) - actualAngularVelocity.x) * (1 / deltaTime);
	angular.y = (UTIL_AngleDiff( m_vCurrentBanking.x, actualAngles.x ) - actualAngularVelocity.y) * (1 / deltaTime);

	// turn toward target
	angular.z = UTIL_AngleDiff( m_fHeadYaw, actualAngles.y + actualAngularVelocity.z * 0.1 ) * (1 / deltaTime);

	// angular = m_vCurrentAngularVelocity - actualAngularVelocity;

	// DevMsg("Sim %d : %.1f %.1f %.1f (%.1f)\n", count++, actualAngles.x, actualAngles.y, actualAngles.z, m_fHeadYaw );

	// FIXME: remove the stuff from MoveExecute();
	// FIXME: check MOVE?

	ClampMotorForces( linear, angular );

	return SIM_GLOBAL_ACCELERATION; // on my local axis.   SIM_GLOBAL_ACCELERATION
}

