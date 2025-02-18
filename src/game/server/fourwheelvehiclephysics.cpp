//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A moving vehicle that is used as a battering ram
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "fourwheelvehiclephysics.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "in_buttons.h"
#include "player.h"
#include "IEffects.h"
#include "physics_saverestore.h"
#include "vehicle_base.h"
#include "isaverestore.h"
#include "movevars_shared.h"
#include "te_effect_dispatch.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define STICK_EXTENTS	400.0f


#define DUST_SPEED			5		// speed at which dust starts
#define REAR_AXLE			1		// indexes of axlex
#define	FRONT_AXLE			0
#define MAX_GUAGE_SPEED		100.0	// 100 mph is max speed shown on guage

#define BRAKE_MAX_VALUE				1.0f
#define BRAKE_BACK_FORWARD_SCALAR	2.0f

ConVar r_vehicleBrakeRate( "r_vehicleBrakeRate", "1.5", FCVAR_CHEAT );

ConVar xbox_throttlebias("xbox_throttlebias", "100", FCVAR_ARCHIVE );
ConVar xbox_throttlespoof("xbox_throttlespoof", "200", FCVAR_ARCHIVE );
ConVar xbox_autothrottle("xbox_autothrottle", "1", FCVAR_ARCHIVE );
ConVar xbox_steering_deadzone( "xbox_steering_deadzone", "0.0" );

// remaps an angular variable to a 3 band function:
// 0 <= t < start :		f(t) = 0
// start <= t <= end :	f(t) = end * spline(( t-start) / (end-start) )  // s curve between clamped and linear
// end < t :			f(t) = t
float RemapAngleRange( float startInterval, float endInterval, float value )
{
	// Fixup the roll
	value = AngleNormalize( value );
	float absAngle = fabs(value);

	// beneath cutoff?
	if ( absAngle < startInterval )
	{
		value = 0;
	}
	// in spline range?
	else if ( absAngle <= endInterval )
	{
		float newAngle = SimpleSpline( (absAngle - startInterval) / (endInterval-startInterval) ) * endInterval;
		// grab the sign from the initial value
		if ( value < 0 )
		{
			newAngle *= -1;
		}
		value = newAngle;
	}
	// else leave it alone, in linear range
	
	return value;
}

enum vehicle_pose_params
{
	VEH_FL_WHEEL_HEIGHT=0,
	VEH_FR_WHEEL_HEIGHT,
	VEH_RL_WHEEL_HEIGHT,
	VEH_RR_WHEEL_HEIGHT,
	VEH_FL_WHEEL_SPIN,
	VEH_FR_WHEEL_SPIN,
	VEH_RL_WHEEL_SPIN,
	VEH_RR_WHEEL_SPIN,
	VEH_STEER,
	VEH_ACTION,
	VEH_SPEEDO,

};


BEGIN_DATADESC_NO_BASE( CFourWheelVehiclePhysics )

// These two are reset every time 
//	DEFINE_FIELD( m_pOuter, FIELD_EHANDLE ),
//											m_pOuterServerVehicle;

	// Quiet down classcheck
	// DEFINE_FIELD( m_controls, vehicle_controlparams_t ),

	// Controls
	DEFINE_FIELD( m_controls.throttle, FIELD_FLOAT ),
	DEFINE_FIELD( m_controls.steering, FIELD_FLOAT ),
	DEFINE_FIELD( m_controls.brake, FIELD_FLOAT ),
	DEFINE_FIELD( m_controls.boost, FIELD_FLOAT ),
	DEFINE_FIELD( m_controls.handbrake, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_controls.handbrakeLeft, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_controls.handbrakeRight, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_controls.brakepedal, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_controls.bHasBrakePedal, FIELD_BOOLEAN ),

	// This has to be handled by the containing class owing to 'owner' issues
//	DEFINE_PHYSPTR( m_pVehicle ),

	DEFINE_FIELD( m_nSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( m_nLastSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( m_nRPM, FIELD_INTEGER ),
	DEFINE_FIELD( m_fLastBoost, FIELD_FLOAT ),
	DEFINE_FIELD( m_nBoostTimeLeft, FIELD_INTEGER ),
	DEFINE_FIELD( m_nHasBoost, FIELD_INTEGER ),

	DEFINE_FIELD( m_maxThrottle, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxRevThrottle, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_actionSpeed, FIELD_FLOAT ),

	// This has to be handled by the containing class owing to 'owner' issues
//	DEFINE_PHYSPTR_ARRAY( m_pWheels ),

	DEFINE_FIELD( m_wheelCount, FIELD_INTEGER ),

	DEFINE_ARRAY( m_wheelPosition, FIELD_VECTOR, 4 ),
	DEFINE_ARRAY( m_wheelRotation, FIELD_VECTOR, 4 ),
	DEFINE_ARRAY( m_wheelBaseHeight, FIELD_FLOAT, 4 ),
	DEFINE_ARRAY( m_wheelTotalHeight, FIELD_FLOAT, 4 ),
	DEFINE_ARRAY( m_poseParameters, FIELD_INTEGER, 12 ),
	DEFINE_FIELD( m_actionValue, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_actionScale, FIELD_FLOAT, "actionScale" ),
	DEFINE_FIELD( m_debugRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_throttleRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_throttleStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_throttleActiveTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_turboTimer, FIELD_FLOAT ),

	DEFINE_FIELD( m_flVehicleVolume, FIELD_FLOAT ),
	DEFINE_FIELD( m_bIsOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLastThrottle, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLastBoost, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLastSkid, FIELD_BOOLEAN ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CFourWheelVehiclePhysics::CFourWheelVehiclePhysics( CBaseAnimating *pOuter )
{
	m_flVehicleVolume = 0.5;
	m_pOuter = NULL;
	m_pOuterServerVehicle = NULL;
	m_flMaxSpeed = 30;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CFourWheelVehiclePhysics::~CFourWheelVehiclePhysics ()
{
	physenv->DestroyVehicleController( m_pVehicle );
}

//-----------------------------------------------------------------------------
// A couple wrapper methods to perform common operations
//-----------------------------------------------------------------------------
inline int CFourWheelVehiclePhysics::LookupPoseParameter( const char *szName )
{
	return m_pOuter->LookupPoseParameter( szName );
}

inline float CFourWheelVehiclePhysics::GetPoseParameter( int iParameter )
{
	return m_pOuter->GetPoseParameter( iParameter );
}

inline float CFourWheelVehiclePhysics::SetPoseParameter( int iParameter, float flValue )
{
	Assert(IsFinite(flValue));
	return m_pOuter->SetPoseParameter( iParameter, flValue );
}

inline bool CFourWheelVehiclePhysics::GetAttachment( const char *szName, Vector &origin, QAngle &angles )
{
	return m_pOuter->GetAttachment( szName, origin, angles );
}

//-----------------------------------------------------------------------------
// Methods related to spawn
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::InitializePoseParameters()
{
	m_poseParameters[VEH_FL_WHEEL_HEIGHT] = LookupPoseParameter( "vehicle_wheel_fl_height" );
	m_poseParameters[VEH_FR_WHEEL_HEIGHT] = LookupPoseParameter( "vehicle_wheel_fr_height" );
	m_poseParameters[VEH_RL_WHEEL_HEIGHT] = LookupPoseParameter( "vehicle_wheel_rl_height" );
	m_poseParameters[VEH_RR_WHEEL_HEIGHT] = LookupPoseParameter( "vehicle_wheel_rr_height" );
	m_poseParameters[VEH_FL_WHEEL_SPIN] = LookupPoseParameter( "vehicle_wheel_fl_spin" );
	m_poseParameters[VEH_FR_WHEEL_SPIN] = LookupPoseParameter( "vehicle_wheel_fr_spin" );
	m_poseParameters[VEH_RL_WHEEL_SPIN] = LookupPoseParameter( "vehicle_wheel_rl_spin" );
	m_poseParameters[VEH_RR_WHEEL_SPIN] = LookupPoseParameter( "vehicle_wheel_rr_spin" );
	m_poseParameters[VEH_STEER] = LookupPoseParameter( "vehicle_steer" );
	m_poseParameters[VEH_ACTION] = LookupPoseParameter( "vehicle_action" );
	m_poseParameters[VEH_SPEEDO] = LookupPoseParameter( "vehicle_guage" );


	// move the wheels to a neutral position
	SetPoseParameter( m_poseParameters[VEH_SPEEDO], 0 );
	SetPoseParameter( m_poseParameters[VEH_STEER], 0 );
	SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_FR_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RR_WHEEL_HEIGHT], 0 );
	m_pOuter->InvalidateBoneCache();
}

//-----------------------------------------------------------------------------
// Purpose: Parses the vehicle's script
//-----------------------------------------------------------------------------
bool CFourWheelVehiclePhysics::ParseVehicleScript( const char *pScriptName, solid_t &solid, vehicleparams_t &vehicle)
{
	// Physics keeps a cache of these to share among spawns of vehicles or flush for debugging
	PhysFindOrAddVehicleScript( pScriptName, &vehicle, NULL );

	m_debugRadius = vehicle.axles[0].wheels.radius;
	CalcWheelData( vehicle );

	PhysModelParseSolid( solid, m_pOuter, m_pOuter->GetModelIndex() );
	
	// Allow the script to shift the center of mass
	if ( vehicle.body.massCenterOverride != vec3_origin )
	{
		solid.massCenterOverride = vehicle.body.massCenterOverride;
		solid.params.massCenterOverride = &solid.massCenterOverride;
	}

	// allow script to change the mass of the vehicle body
	if ( vehicle.body.massOverride > 0 )
	{
		solid.params.mass = vehicle.body.massOverride;
	}

	return true;
}

void CFourWheelVehiclePhysics::CalcWheelData( vehicleparams_t &vehicle )
{
	const char *pWheelAttachments[4] = { "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr" };
	Vector left, right;
	QAngle dummy;
	SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_FR_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RR_WHEEL_HEIGHT], 0 );
	m_pOuter->InvalidateBoneCache();
	if ( GetAttachment( "wheel_fl", left, dummy ) && GetAttachment( "wheel_fr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		Vector center = (left + right) * 0.5;
		vehicle.axles[0].offset = center;
		vehicle.axles[0].wheelOffset = right - center;
		// Cache the base height of the wheels in body space
		m_wheelBaseHeight[0] = left.z;
		m_wheelBaseHeight[1] = right.z;
	}

	if ( GetAttachment( "wheel_rl", left, dummy ) && GetAttachment( "wheel_rr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		Vector center = (left + right) * 0.5;
		vehicle.axles[1].offset = center;
		vehicle.axles[1].wheelOffset = right - center;
		// Cache the base height of the wheels in body space
		m_wheelBaseHeight[2] = left.z;
		m_wheelBaseHeight[3] = right.z;
	}
	SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_HEIGHT], 1 );
	SetPoseParameter( m_poseParameters[VEH_FR_WHEEL_HEIGHT], 1 );
	SetPoseParameter( m_poseParameters[VEH_RL_WHEEL_HEIGHT], 1 );
	SetPoseParameter( m_poseParameters[VEH_RR_WHEEL_HEIGHT], 1 );
	m_pOuter->InvalidateBoneCache();
	if ( GetAttachment( "wheel_fl", left, dummy ) && GetAttachment( "wheel_fr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		// Cache the height range of the wheels in body space
		m_wheelTotalHeight[0] = m_wheelBaseHeight[0] - left.z;
		m_wheelTotalHeight[1] = m_wheelBaseHeight[1] - right.z;
		vehicle.axles[0].wheels.springAdditionalLength = m_wheelTotalHeight[0];
	}

	if ( GetAttachment( "wheel_rl", left, dummy ) && GetAttachment( "wheel_rr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		// Cache the height range of the wheels in body space
		m_wheelTotalHeight[2] = m_wheelBaseHeight[0] - left.z;
		m_wheelTotalHeight[3] = m_wheelBaseHeight[1] - right.z;
		vehicle.axles[1].wheels.springAdditionalLength = m_wheelTotalHeight[2];
	}
	for ( int i = 0; i < 4; i++ )
	{
		if ( m_wheelTotalHeight[i] == 0.0f )
		{
			DevWarning("Vehicle %s has invalid wheel attachment for %s - no movement\n", STRING(m_pOuter->GetModelName()), pWheelAttachments[i]);
			m_wheelTotalHeight[i] = 1.0f;
		}
	}

	SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_FR_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RL_WHEEL_HEIGHT], 0 );
	SetPoseParameter( m_poseParameters[VEH_RR_WHEEL_HEIGHT], 0 );
	m_pOuter->InvalidateBoneCache();

	// Get raytrace offsets if they exist.
	if ( GetAttachment( "raytrace_fl", left, dummy ) && GetAttachment( "raytrace_fr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		Vector center = ( left + right ) * 0.5;
		vehicle.axles[0].raytraceCenterOffset = center;
		vehicle.axles[0].raytraceOffset = right - center;
	}

	if ( GetAttachment( "raytrace_rl", left, dummy ) && GetAttachment( "raytrace_rr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		Vector center = ( left + right ) * 0.5;
		vehicle.axles[1].raytraceCenterOffset = center;
		vehicle.axles[1].raytraceOffset = right - center;
	}
}


//-----------------------------------------------------------------------------
// Spawns the vehicle
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::Spawn( )
{
	Assert( m_pOuter );

	m_actionValue = 0;
	m_actionSpeed = 0;

	m_bIsOn = false;
	m_controls.handbrake = false;
	m_controls.handbrakeLeft = false;
	m_controls.handbrakeRight = false;
	m_controls.bHasBrakePedal = true;
	m_controls.bAnalogSteering = false;
	
	SetMaxThrottle( 1.0 );
	SetMaxReverseThrottle( -1.0f );

	InitializePoseParameters();
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the vehicle physics
//			Called by our outer vehicle in it's Spawn()
//-----------------------------------------------------------------------------
bool CFourWheelVehiclePhysics::Initialize( const char *pVehicleScript, unsigned int nVehicleType )
{
	// Ok, turn on the simulation now
	// FIXME: Disabling collisions here is necessary because we seem to be
	// getting a one-frame collision between the old + new collision models
	if ( m_pOuter->VPhysicsGetObject() )
	{
		m_pOuter->VPhysicsGetObject()->EnableCollisions(false);
	}
	m_pOuter->VPhysicsDestroyObject();

	// Create the vphysics model + teleport it into position
	solid_t solid;
	vehicleparams_t vehicle;
	if (!ParseVehicleScript( pVehicleScript, solid, vehicle ))
	{
		UTIL_Remove(m_pOuter);
		return false;
	}

	// NOTE: this needs to be greater than your max framerate (so zero is still instant)
	m_throttleRate = 10000.0;
	if ( vehicle.engine.throttleTime > 0 )
	{
		m_throttleRate = 1.0 / vehicle.engine.throttleTime;
	}

	m_flMaxSpeed = vehicle.engine.maxSpeed;

	IPhysicsObject *pBody = m_pOuter->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &solid );
	PhysSetGameFlags( pBody, FVPHYSICS_NO_SELF_COLLISIONS | FVPHYSICS_MULTIOBJECT_ENTITY );
	m_pVehicle = physenv->CreateVehicleController( pBody, vehicle, nVehicleType, physgametrace );
	m_wheelCount = m_pVehicle->GetWheelCount();
	for ( int i = 0; i < m_wheelCount; i++ )
	{
		m_pWheels[i] = m_pVehicle->GetWheel( i );
	}
	return true;
}


//-----------------------------------------------------------------------------
// Various steering parameters
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetThrottle( float flThrottle )
{
	m_controls.throttle = flThrottle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetMaxThrottle( float flMaxThrottle )
{
	m_maxThrottle = flMaxThrottle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetMaxReverseThrottle( float flMaxThrottle )
{
	m_flMaxRevThrottle = flMaxThrottle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetSteering( float flSteering, float flSteeringRate )
{
	if ( !flSteeringRate )
	{
		m_controls.steering = flSteering;
	}
	else
	{
		m_controls.steering = Approach( flSteering, m_controls.steering, flSteeringRate );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetSteeringDegrees( float flDegrees )
{
	vehicleparams_t &vehicleParams = m_pVehicle->GetVehicleParamsForChange();
	vehicleParams.steering.degreesSlow = flDegrees;
	vehicleParams.steering.degreesFast = flDegrees;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetAction( float flAction )
{
	m_actionSpeed = flAction;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::TurnOn( )
{
	if ( IsEngineDisabled() )
		return;

	if ( !m_bIsOn )
	{
		m_pOuterServerVehicle->SoundStart();
		m_bIsOn = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::TurnOff( )
{
	ResetControls();

	if ( m_bIsOn )
	{
		m_pOuterServerVehicle->SoundShutdown();
		m_bIsOn = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetBoost( float flBoost )
{
	if ( !IsEngineDisabled() )
	{
		m_controls.boost = flBoost;
	}
}

//------------------------------------------------------
// UpdateBooster - Calls UpdateBooster() in the vphysics
// code to allow the timer to be updated
//
// Returns: false if timer has expired (can use again and
//			can stop think
//			true if timer still running
//------------------------------------------------------
bool CFourWheelVehiclePhysics::UpdateBooster( void )
{
	float retval = m_pVehicle->UpdateBooster(gpGlobals->frametime );
	return ( retval > 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetHasBrakePedal( bool bHasBrakePedal )
{
	m_controls.bHasBrakePedal = bHasBrakePedal;
}

//-----------------------------------------------------------------------------
// Teleport
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::Teleport( matrix3x4_t& relativeTransform )
{
	// We basically just have to make sure the wheels are in the right place
	// after teleportation occurs

	for ( int i = 0; i < m_wheelCount; i++ )
	{
		matrix3x4_t matrix, newMatrix;
		m_pWheels[i]->GetPositionMatrix( &matrix );
		ConcatTransforms( relativeTransform, matrix, newMatrix );
		m_pWheels[i]->SetPositionMatrix( newMatrix, true );
	}
	
	// Wake the vehicle back up after a teleport
	if ( m_pOuterServerVehicle && m_pOuterServerVehicle->GetFourWheelVehicle() )
	{
		IPhysicsObject *pObj = m_pOuterServerVehicle->GetFourWheelVehicle()->VPhysicsGetObject();
		if ( pObj )
		{
			pObj->Wake();
		}
	}
}

#if 1
// For the #if 0 debug code below!
#define HL2IVP_FACTOR	METERS_PER_INCH
#define IVP2HL(x)		(float)(x * (1.0f/HL2IVP_FACTOR))
#define HL2IVP(x)		(double)(x * HL2IVP_FACTOR)		
#endif

//-----------------------------------------------------------------------------
// Debugging methods
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::DrawDebugGeometryOverlays()
{
	for ( int iWheel = 0; iWheel < m_wheelCount; iWheel++ )
	{
		IPhysicsObject *pWheel = m_pVehicle->GetWheel( iWheel );
		float radius = pWheel->GetSphereRadius();
		
		Vector vecPos;
		QAngle vecRot;
		pWheel->GetPosition( &vecPos, &vecRot );
		// draw the physics object position/orientation
		NDebugOverlay::Sphere( vecPos, vecRot, radius, 0, 255, 0, 0, false, 0 );
		// draw the animation position/orientation
		NDebugOverlay::Sphere(m_wheelPosition[iWheel], m_wheelRotation[iWheel], radius, 255, 255, 0, 0, false, 0);
	}

	// Render vehicle data.
	IPhysicsObject *pBody = m_pOuter->VPhysicsGetObject();
	if ( pBody )
	{
		const vehicleparams_t vehicleParams = m_pVehicle->GetVehicleParams();

		// Draw a red cube as the "center" of the vehicle.
		Vector vecBodyPosition; 
		QAngle angBodyDirection;
		pBody->GetPosition( &vecBodyPosition, &angBodyDirection );
		NDebugOverlay::BoxAngles( vecBodyPosition, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), angBodyDirection, 255, 0, 0, 0 ,0 );

		matrix3x4_t matrix;
		AngleMatrix( angBodyDirection, vecBodyPosition, matrix );

		// Draw green cubes at axle centers.
		Vector vecAxlePositions[2], vecAxlePositionsHL[2];
		vecAxlePositions[0] = vehicleParams.axles[0].offset;
		vecAxlePositions[1] = vehicleParams.axles[1].offset;

		VectorTransform( vecAxlePositions[0], matrix, vecAxlePositionsHL[0] );		
		VectorTransform( vecAxlePositions[1], matrix, vecAxlePositionsHL[1] );

		NDebugOverlay::BoxAngles( vecAxlePositionsHL[0], Vector( -3, -3, -3 ), Vector( 3, 3, 3 ), angBodyDirection, 0, 255, 0, 0 ,0 );
		NDebugOverlay::BoxAngles( vecAxlePositionsHL[1], Vector( -3, -3, -3 ), Vector( 3, 3, 3 ), angBodyDirection, 0, 255, 0, 0 ,0 );

		// Draw wheel raycasts in yellow
		vehicle_debugcarsystem_t debugCarSystem;
		m_pVehicle->GetCarSystemDebugData( debugCarSystem );
		for ( int iWheel = 0; iWheel < 4; ++iWheel )
		{
			Vector vecStart, vecEnd, vecImpact;

			// Hack for now.
			float tmpY = IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][0].z );
			vecStart.z = -IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][0].y );
			vecStart.y = tmpY;
			vecStart.x = IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][0].x );

			tmpY = IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][1].z );
			vecEnd.z = -IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][1].y );
			vecEnd.y = tmpY;
			vecEnd.x = IVP2HL( debugCarSystem.vecWheelRaycasts[iWheel][1].x );

			tmpY = IVP2HL( debugCarSystem.vecWheelRaycastImpacts[iWheel].z );
			vecImpact.z = -IVP2HL( debugCarSystem.vecWheelRaycastImpacts[iWheel].y );
			vecImpact.y = tmpY;
			vecImpact.x = IVP2HL( debugCarSystem.vecWheelRaycastImpacts[iWheel].x );

			NDebugOverlay::BoxAngles( vecStart, Vector( -1 , -1, -1 ), Vector( 1, 1, 1 ), angBodyDirection, 0, 255, 0, 0, 0  );
			NDebugOverlay::Line( vecStart, vecEnd, 255, 255, 0, true, 0 );
			NDebugOverlay::BoxAngles( vecEnd, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), angBodyDirection, 255, 0, 0, 0, 0 );

			NDebugOverlay::BoxAngles( vecImpact, Vector( -0.5f , -0.5f, -0.5f ), Vector( 0.5f, 0.5f, 0.5f ), angBodyDirection, 0, 0, 255, 0, 0  );
			DebugDrawContactPoints( m_pVehicle->GetWheel(iWheel) );
		}
	}
}

int CFourWheelVehiclePhysics::DrawDebugTextOverlays( int nOffset )
{
	const vehicle_operatingparams_t &params = m_pVehicle->GetOperatingParams();
	char tempstr[512];
	Q_snprintf( tempstr,sizeof(tempstr), "Speed %.1f  T/S/B (%.0f/%.0f/%.1f)", params.speed, m_controls.throttle, m_controls.steering, m_controls.brake );
	m_pOuter->EntityText( nOffset, tempstr, 0 );
	nOffset++;
	Msg( "%s", tempstr );

	Q_snprintf( tempstr,sizeof(tempstr), "Gear: %d, RPM %4d", params.gear, (int)params.engineRPM );
	m_pOuter->EntityText( nOffset, tempstr, 0 );
	nOffset++;
	Msg( " %s\n", tempstr );

	return nOffset;
}

//----------------------------------------------------
// Place dust at vector passed in
//----------------------------------------------------
void CFourWheelVehiclePhysics::PlaceWheelDust( int wheelIndex, bool ignoreSpeed )
{
	// New vehicles handle this deeper into the base class
	if ( hl2_episodic.GetBool() )
		return;

	// Old dust
	Vector	vecPos, vecVel;
	m_pVehicle->GetWheelContactPoint( wheelIndex, &vecPos, NULL );

	vecVel.Random( -1.0f, 1.0f );
	vecVel.z = random->RandomFloat( 0.3f, 1.0f );

	VectorNormalize( vecVel );

	// Higher speeds make larger dust clouds
	float flSize;
	if ( ignoreSpeed )
	{
		flSize = 1.0f;
	}
	else
	{
		flSize = RemapValClamped( m_nSpeed, DUST_SPEED, m_flMaxSpeed, 0.0f, 1.0f );
	}

	if ( flSize )
	{
		CEffectData	data;

		data.m_vOrigin = vecPos;
		data.m_vNormal = vecVel;
		data.m_flScale = flSize;

		DispatchEffect( "WheelDust", data );
	}
}

//-----------------------------------------------------------------------------
// Frame-based updating 
//-----------------------------------------------------------------------------
bool CFourWheelVehiclePhysics::Think()
{
	if (!m_pVehicle)
		return false;

	// Update sound + physics state
	const vehicle_operatingparams_t &carState = m_pVehicle->GetOperatingParams();
	const vehicleparams_t &vehicleData = m_pVehicle->GetVehicleParams();

	// Set save data.
	float carSpeed = fabs( INS2MPH( carState.speed ) );
	m_nLastSpeed = m_nSpeed;
	m_nSpeed = ( int )carSpeed;
	m_nRPM = ( int )carState.engineRPM;
	m_nHasBoost = vehicleData.engine.boostDelay;	// if we have any boost delay, vehicle has boost ability

	m_pVehicle->Update( gpGlobals->frametime, m_controls);

	// boost sounds
	if( IsBoosting() && !m_bLastBoost )
	{
		m_bLastBoost = true;
		m_turboTimer = gpGlobals->curtime + 2.75f;		// min duration for turbo sound
	}
	else if( !IsBoosting() && m_bLastBoost )
	{
		if ( gpGlobals->curtime >= m_turboTimer )
		{
			m_bLastBoost = false;
		}
	}

	m_fLastBoost = carState.boostDelay;
	m_nBoostTimeLeft =  carState.boostTimeLeft;

	// UNDONE: Use skid info from the physics system?
	// Only check wheels if we're not being carried by a dropship
	if ( m_pOuter->VPhysicsGetObject() && !m_pOuter->VPhysicsGetObject()->GetShadowController() )
	{
		const float skidFactor = 0.15f;
		const float minSpeed = DEFAULT_SKID_THRESHOLD / skidFactor;
		// we have to slide at least 15% of our speed at higher speeds to make the skid sound (otherwise it can be too frequent)
		float skidThreshold = m_bLastSkid ? DEFAULT_SKID_THRESHOLD : (carState.speed * 0.15f);
		if ( skidThreshold < DEFAULT_SKID_THRESHOLD )
		{
			// otherwise, ramp in the skid threshold to avoid the sound at really low speeds unless really skidding
			skidThreshold = RemapValClamped( fabs(carState.speed), 0, minSpeed, DEFAULT_SKID_THRESHOLD*8, DEFAULT_SKID_THRESHOLD );
		}
		// check for skidding, if we're skidding, need to play the sound
		if ( carState.skidSpeed > skidThreshold && m_bIsOn )
		{
			if ( !m_bLastSkid )	// only play sound once
			{
				m_bLastSkid = true;
				CPASAttenuationFilter filter( m_pOuter );
				m_pOuterServerVehicle->PlaySound( VS_SKID_FRICTION_NORMAL );
			}

			// kick up dust from the wheels while skidding
			for ( int i = 0; i < 4; i++ )
			{
				PlaceWheelDust( i, true );
			}
		}
		else if ( m_bLastSkid == true )
		{
			m_bLastSkid = false;
			m_pOuterServerVehicle->StopSound( VS_SKID_FRICTION_NORMAL );
		}

		// toss dust up from the wheels of the vehicle if we're moving fast enough
		if ( m_nSpeed >= DUST_SPEED && vehicleData.steering.dustCloud && m_bIsOn )
		{
			for ( int i = 0; i < 4; i++ )
			{
				PlaceWheelDust( i );
			}
		}
	}

	// Make the steering wheel match the input, with a little dampening.
	#define STEER_DAMPING	0.8
	float flSteer = GetPoseParameter( m_poseParameters[VEH_STEER] );
	float flPhysicsSteer = carState.steeringAngle / vehicleData.steering.degreesSlow;
	SetPoseParameter( m_poseParameters[VEH_STEER], (STEER_DAMPING * flSteer) + ((1 - STEER_DAMPING) * flPhysicsSteer) );

	m_actionValue += m_actionSpeed * m_actionScale * gpGlobals->frametime;
	SetPoseParameter( m_poseParameters[VEH_ACTION], m_actionValue );

	// setup speedometer
	if ( m_bIsOn == true )
	{
		float displaySpeed = m_nSpeed / MAX_GUAGE_SPEED;
		SetPoseParameter( m_poseParameters[VEH_SPEEDO], displaySpeed );
	}

	return m_bIsOn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFourWheelVehiclePhysics::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	// must be a wheel
	if ( pPhysics == m_pOuter->VPhysicsGetObject() )
		return true;

	// This is here so we can make the pose parameters of the wheels
	// reflect their current physics state
	for ( int i = 0; i < m_wheelCount; i++ )
	{
		if ( pPhysics == m_pWheels[i] )
		{
			Vector tmp;
			pPhysics->GetPosition( &m_wheelPosition[i], &m_wheelRotation[i] );

			// transform the wheel into body space
			VectorITransform( m_wheelPosition[i], m_pOuter->EntityToWorldTransform(), tmp );
			SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_HEIGHT + i], (m_wheelBaseHeight[i] - tmp.z) / m_wheelTotalHeight[i] );
			SetPoseParameter( m_poseParameters[VEH_FL_WHEEL_SPIN + i], -m_wheelRotation[i].z );
			return false;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Shared code to compute the vehicle view position
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::GetVehicleViewPosition( const char *pViewAttachment, float flPitchFactor, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachment( pViewAttachment, vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

#ifdef HL2_DLL
	// View dampening.
	if ( r_VehicleViewDampen.GetInt() )
	{
		m_pOuterServerVehicle->GetFourWheelVehicle()->DampenEyePosition( vehicleEyeOrigin, vehicleEyeAngles );
	}
#endif

	// Compute the relative rotation between the unperterbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Now perterb the attachment point
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO * flPitchFactor, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO * flPitchFactor, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perterbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}


//-----------------------------------------------------------------------------
// Control initialization
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::ResetControls()
{
	m_controls.handbrake = true;
	m_controls.handbrakeLeft = false;
	m_controls.handbrakeRight = false;
	m_controls.boost = 0;
	m_controls.brake = 0.0f;
	m_controls.throttle = 0;
	m_controls.steering = 0;
}

void CFourWheelVehiclePhysics::ReleaseHandbrake()
{
	m_controls.handbrake = false;
}

void CFourWheelVehiclePhysics::SetHandbrake( bool bBrake )
{
	m_controls.handbrake = bBrake;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::EnableMotion( void )
{
	for( int iWheel = 0; iWheel < m_wheelCount; ++iWheel )
	{
		m_pWheels[iWheel]->EnableMotion( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::DisableMotion( void )
{
	Vector vecZero( 0.0f, 0.0f, 0.0f );
	AngularImpulse angNone( 0.0f, 0.0f, 0.0f );

	for( int iWheel = 0; iWheel < m_wheelCount; ++iWheel )
	{
		m_pWheels[iWheel]->SetVelocity( &vecZero, &angNone );
		m_pWheels[iWheel]->EnableMotion( false );
	}
}

float CFourWheelVehiclePhysics::GetHLSpeed() const
{
	const vehicle_operatingparams_t &carState = m_pVehicle->GetOperatingParams();
	return carState.speed;
}

float CFourWheelVehiclePhysics::GetSteering() const
{
	return m_controls.steering;
}

float CFourWheelVehiclePhysics::GetSteeringDegrees() const
{
	const vehicleparams_t vehicleParams = m_pVehicle->GetVehicleParams();
	return vehicleParams.steering.degreesSlow;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SteeringRest( float carSpeed, const vehicleparams_t &vehicleData )
{
	float flSteeringRate = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
		vehicleData.steering.steeringRestRateSlow, vehicleData.steering.steeringRestRateFast );
	m_controls.steering = Approach(0, m_controls.steering, flSteeringRate * gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SteeringTurn( float carSpeed, const vehicleparams_t &vehicleData, bool bTurnLeft, bool bBrake, bool bThrottle )
{
	float flTargetSteering = bTurnLeft ? -1.0f : 1.0f;
	// steering speeds are stored in MPH
	float flSteeringRestRate = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
		vehicleData.steering.steeringRestRateSlow, vehicleData.steering.steeringRestRateFast );

	float carSpeedIns = MPH2INS(carSpeed);
	// engine speeds are stored in in/s
	if ( carSpeedIns > vehicleData.engine.maxSpeed )
	{
		flSteeringRestRate = RemapValClamped( carSpeedIns, vehicleData.engine.maxSpeed, vehicleData.engine.boostMaxSpeed, vehicleData.steering.steeringRestRateFast, vehicleData.steering.steeringRestRateFast*0.5f );
	}

	const vehicle_operatingparams_t &carState = m_pVehicle->GetOperatingParams();
	bool bIsBoosting = carState.isTorqueBoosting;

	// if you're recovering from a boost and still going faster than max, use the boost steering values
	bool bIsBoostRecover = (carState.boostTimeLeft == 100 || carState.boostTimeLeft == 0) ? false : true;
	float boostMinSpeed = vehicleData.engine.maxSpeed * vehicleData.engine.autobrakeSpeedGain;
	if ( !bIsBoosting && bIsBoostRecover && carSpeedIns > boostMinSpeed )
	{
		bIsBoosting = true;
	}

	if ( bIsBoosting )
	{
		flSteeringRestRate *= vehicleData.steering.boostSteeringRestRateFactor;
	}
	else if ( bThrottle )
	{
		flSteeringRestRate *= vehicleData.steering.throttleSteeringRestRateFactor;
	}

	float flSteeringRate = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
		vehicleData.steering.steeringRateSlow, vehicleData.steering.steeringRateFast );

	if ( fabs(flSteeringRate) < flSteeringRestRate )
	{
		if ( Sign(flTargetSteering) != Sign(m_controls.steering) )
		{
			flSteeringRate = flSteeringRestRate;
		}
	}
	if ( bIsBoosting )
	{
		flSteeringRate *= vehicleData.steering.boostSteeringRateFactor;
	}
	else if ( bBrake )
	{
		flSteeringRate *= vehicleData.steering.brakeSteeringRateFactor;
	}
	flSteeringRate *= gpGlobals->frametime;
	m_controls.steering = Approach( flTargetSteering, m_controls.steering, flSteeringRate );
	m_controls.bAnalogSteering = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SteeringTurnAnalog( float carSpeed, const vehicleparams_t &vehicleData, float sidemove )
{

	// OLD Code
#if 0
	float flSteeringRate = STEERING_BASE_RATE;

	float factor = clamp( fabs( sidemove ) / STICK_EXTENTS, 0.0f, 1.0f );

	factor *= 30;
	flSteeringRate *= log( factor );
	flSteeringRate *= gpGlobals->frametime;

	SetSteering( sidemove < 0.0f ? -1 : 1, flSteeringRate );
#else
	// This is tested with gamepads with analog sticks.  It gives full analog control allowing the player to hold shallow turns.
	float steering = ( sidemove / STICK_EXTENTS );

	float flSign = ( steering > 0 ) ? 1.0f : -1.0f;
	float flSteerAdj = RemapValClamped( fabs( steering ), xbox_steering_deadzone.GetFloat(), 1.0f, 0.0f, 1.0f );

	float flSteeringRate = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
		vehicleData.steering.steeringRateSlow, vehicleData.steering.steeringRateFast );
	flSteeringRate *= vehicleData.steering.throttleSteeringRestRateFactor;

	m_controls.bAnalogSteering = true;
	SetSteering( flSign * flSteerAdj, flSteeringRate * gpGlobals->frametime );
#endif
}

//-----------------------------------------------------------------------------
// Methods related to actually driving the vehicle
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::UpdateDriverControls( CUserCmd *cmd, float flFrameTime )
{
	const float SPEED_THROTTLE_AS_BRAKE = 2.0f;
	int nButtons = cmd->buttons;

	// Get vehicle data.
	const vehicle_operatingparams_t &carState = m_pVehicle->GetOperatingParams();
	const vehicleparams_t &vehicleData = m_pVehicle->GetVehicleParams();

	// Get current speed in miles/hour.
	float flCarSign = 0.0f;
	if (carState.speed >= SPEED_THROTTLE_AS_BRAKE) 
	{
		flCarSign = 1.0f;
	}
	else if ( carState.speed <= -SPEED_THROTTLE_AS_BRAKE )
	{
		flCarSign = -1.0f;
	}
	float carSpeed = fabs(INS2MPH(carState.speed));

	// If going forward and turning hard, keep the throttle applied.
	if( xbox_autothrottle.GetBool() && cmd->forwardmove > 0.0f )
	{
		if( carSpeed > GetMaxSpeed() * 0.75 )
		{
			if( fabs(cmd->sidemove) > cmd->forwardmove )
			{
				cmd->forwardmove = STICK_EXTENTS;
			}
		}
	}

	//Msg("F: %4.1f \tS: %4.1f!\tSTEER: %3.1f\n", cmd->forwardmove, cmd->sidemove, carState.steeringAngle);
	// If changing direction, use default "return to zero" speed to more quickly transition.
	if ( ( nButtons & IN_MOVELEFT ) || ( nButtons & IN_MOVERIGHT ) )
	{
		bool bTurnLeft = ( (nButtons & IN_MOVELEFT) != 0 );
		bool bBrake = ((nButtons & IN_BACK) != 0);
		bool bThrottleDown = ( (nButtons & IN_FORWARD) != 0 ) && !bBrake;
		SteeringTurn( carSpeed, vehicleData, bTurnLeft, bBrake, bThrottleDown );
	}
	else if ( cmd->sidemove != 0.0f )
	{
		SteeringTurnAnalog( carSpeed, vehicleData, cmd->sidemove );
	}
	else
	{
		SteeringRest( carSpeed, vehicleData );
	}

	// Set vehicle control inputs.
	m_controls.boost = 0;
	m_controls.handbrake = false;
	m_controls.handbrakeLeft = false;
	m_controls.handbrakeRight = false;
	m_controls.brakepedal = false;	
	bool bThrottle;

	//-------------------------------------------------------------------------
	// Analog throttle biasing - This code gives the player a bit of control stick
	// 'slop' in the opposite direction that they are driving. If a player is 
	// driving forward and makes a hard turn in which the stick actually goes
	// below neutral (toward reverse), this code continues to propel the car 
	// forward unless the player makes a significant motion towards reverse.
	// (The inverse is true when driving in reverse and the stick is moved slightly forward)
	//-------------------------------------------------------------------------
	CBaseEntity *pDriver = m_pOuterServerVehicle->GetDriver();
	CBasePlayer *pPlayerDriver;
	float flBiasThreshold = xbox_throttlebias.GetFloat();

	if( pDriver && pDriver->IsPlayer() )
	{
		pPlayerDriver = dynamic_cast<CBasePlayer*>(pDriver);

		if( cmd->forwardmove == 0.0f && (fabs(cmd->sidemove) < 200.0f) )
		{
			// If the stick goes neutral, clear out the bias. When the bias is neutral, it will begin biasing
			// in whichever direction the user next presses the analog stick.
			pPlayerDriver->SetVehicleAnalogControlBias( VEHICLE_ANALOG_BIAS_NONE );
		}
		else if( cmd->forwardmove > 0.0f)
		{
			if( pPlayerDriver->GetVehicleAnalogControlBias() == VEHICLE_ANALOG_BIAS_REVERSE )
			{
				// Player is pushing forward, but the controller is currently biased for reverse driving.
				// Must pass a threshold to be accepted as forward input. Otherwise we just spoof a reduced reverse input 
				// to keep the car moving in the direction the player probably expects.
				if( cmd->forwardmove < flBiasThreshold )
				{
					cmd->forwardmove = -xbox_throttlespoof.GetFloat();
				}
				else
				{
					// Passed the threshold. Allow the direction change to occur.
					pPlayerDriver->SetVehicleAnalogControlBias( VEHICLE_ANALOG_BIAS_FORWARD );
				}
			}
			else if( pPlayerDriver->GetVehicleAnalogControlBias() == VEHICLE_ANALOG_BIAS_NONE )
			{
				pPlayerDriver->SetVehicleAnalogControlBias( VEHICLE_ANALOG_BIAS_FORWARD );
			}
		}
		else if( cmd->forwardmove < 0.0f )
		{
			if( pPlayerDriver->GetVehicleAnalogControlBias() == VEHICLE_ANALOG_BIAS_FORWARD )
			{
				// Inverse of above logic
				if( cmd->forwardmove > -flBiasThreshold )
				{
					cmd->forwardmove = xbox_throttlespoof.GetFloat();
				}
				else
				{
					pPlayerDriver->SetVehicleAnalogControlBias( VEHICLE_ANALOG_BIAS_REVERSE );
				}
			}
			else if( pPlayerDriver->GetVehicleAnalogControlBias() == VEHICLE_ANALOG_BIAS_NONE )
			{
				pPlayerDriver->SetVehicleAnalogControlBias( VEHICLE_ANALOG_BIAS_REVERSE );
			}
		}
	}

	//=========================
	// analog control
	//=========================
	if( cmd->forwardmove > 0.0f )
	{
		float flAnalogThrottle = cmd->forwardmove / STICK_EXTENTS;

		flAnalogThrottle = clamp( flAnalogThrottle, 0.25f, 1.0f );

		bThrottle = true;
		if ( m_controls.throttle < 0 )
		{
			m_controls.throttle = 0;
		}

		float flMaxThrottle = MAX( 0.1, m_maxThrottle );
		if ( m_controls.steering != 0 )
		{
			float flThrottleReduce = 0;

			// ramp this in, don't just start at the slow speed reduction (helps accelerate from a stop)
			if ( carSpeed < vehicleData.steering.speedSlow )
			{
				flThrottleReduce = RemapValClamped( carSpeed, 0, vehicleData.steering.speedSlow, 
					0, vehicleData.steering.turnThrottleReduceSlow );
			}
			else
			{
				flThrottleReduce = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
					vehicleData.steering.turnThrottleReduceSlow, vehicleData.steering.turnThrottleReduceFast );
			}

			float limit = 1.0f - (flThrottleReduce * fabs(m_controls.steering));
			if ( limit < 0 )
				limit = 0;
			flMaxThrottle = MIN( flMaxThrottle, limit );
		}

		m_controls.throttle = Approach( flMaxThrottle * flAnalogThrottle, m_controls.throttle, flFrameTime * m_throttleRate );

		// Apply the brake.
		if ( ( flCarSign < 0.0f ) && m_controls.bHasBrakePedal )
		{
			m_controls.brake = Approach( BRAKE_MAX_VALUE, m_controls.brake, flFrameTime * r_vehicleBrakeRate.GetFloat() * BRAKE_BACK_FORWARD_SCALAR );
			m_controls.brakepedal = true;	
			m_controls.throttle = 0.0f;
			bThrottle = false;
		}
		else
		{
			m_controls.brake = 0.0f;
		}
	}
	else if( cmd->forwardmove < 0.0f )
	{
		float flAnalogBrake = fabs(cmd->forwardmove / STICK_EXTENTS);

		flAnalogBrake = clamp( flAnalogBrake, 0.25f, 1.0f );

		bThrottle = true;
		if ( m_controls.throttle > 0 )
		{
			m_controls.throttle = 0;
		}

		float flMaxThrottle = MIN( -0.1, m_flMaxRevThrottle  );
		m_controls.throttle = Approach( flMaxThrottle * flAnalogBrake, m_controls.throttle, flFrameTime * m_throttleRate );

		// Apply the brake.
		if ( ( flCarSign > 0.0f ) && m_controls.bHasBrakePedal )
		{
			m_controls.brake = Approach( BRAKE_MAX_VALUE, m_controls.brake, flFrameTime * r_vehicleBrakeRate.GetFloat() );
			m_controls.brakepedal = true;
			m_controls.throttle = 0.0f;
			bThrottle = false;
		}
		else
		{
			m_controls.brake = 0.0f;
		}
	}
	// digital control
	else if ( nButtons & IN_FORWARD )
	{
		bThrottle = true;
		if ( m_controls.throttle < 0 )
		{
			m_controls.throttle = 0;
		}

		float flMaxThrottle = MAX( 0.1, m_maxThrottle );

		if ( m_controls.steering != 0 )
		{
			float flThrottleReduce = 0;

			// ramp this in, don't just start at the slow speed reduction (helps accelerate from a stop)
			if ( carSpeed < vehicleData.steering.speedSlow )
			{
				flThrottleReduce = RemapValClamped( carSpeed, 0, vehicleData.steering.speedSlow, 
					0, vehicleData.steering.turnThrottleReduceSlow );
			}
			else
			{
				flThrottleReduce = RemapValClamped( carSpeed, vehicleData.steering.speedSlow, vehicleData.steering.speedFast, 
					vehicleData.steering.turnThrottleReduceSlow, vehicleData.steering.turnThrottleReduceFast );
			}
			
			float limit = 1.0f - (flThrottleReduce * fabs(m_controls.steering));
			if ( limit < 0 )
				limit = 0;
			flMaxThrottle = MIN( flMaxThrottle, limit );
		}

		m_controls.throttle = Approach( flMaxThrottle, m_controls.throttle, flFrameTime * m_throttleRate );

		// Apply the brake.
		if ( ( flCarSign < 0.0f ) && m_controls.bHasBrakePedal )
		{
			m_controls.brake = Approach( BRAKE_MAX_VALUE, m_controls.brake, flFrameTime * r_vehicleBrakeRate.GetFloat() * BRAKE_BACK_FORWARD_SCALAR );
			m_controls.brakepedal = true;	
			m_controls.throttle = 0.0f;
			bThrottle = false;
		}
		else
		{
			m_controls.brake = 0.0f;
		}
	}
	else if ( nButtons & IN_BACK )
	{
		bThrottle = true;
		if ( m_controls.throttle > 0 )
		{
			m_controls.throttle = 0;
		}

		float flMaxThrottle = MIN( -0.1, m_flMaxRevThrottle );
		m_controls.throttle = Approach( flMaxThrottle, m_controls.throttle, flFrameTime * m_throttleRate );

		// Apply the brake.
		if ( ( flCarSign > 0.0f ) && m_controls.bHasBrakePedal )
		{
			m_controls.brake = Approach( BRAKE_MAX_VALUE, m_controls.brake, flFrameTime * r_vehicleBrakeRate.GetFloat() );
			m_controls.brakepedal = true;
			m_controls.throttle = 0.0f;
			bThrottle = false;
		}
		else
		{
			m_controls.brake = 0.0f;
		}
	}
	else
	{
		bThrottle = false;
		m_controls.throttle = 0;
		m_controls.brake = 0.0f;
	}

	if ( ( nButtons & IN_SPEED ) && !IsEngineDisabled() && bThrottle )
	{
		m_controls.boost = 1.0f;
	}

	// Using has brakepedal for handbrake as well.
	if ( ( nButtons & IN_JUMP ) && m_controls.bHasBrakePedal )
	{
		m_controls.handbrake = true;	

		if ( cmd->sidemove < -100 )
		{
			m_controls.handbrakeLeft = true;
		}
		else if ( cmd->sidemove > 100 )
		{
			m_controls.handbrakeRight = true;
		}

		// Prevent playing of the engine revup when we're braking
		bThrottle = false;
	}

	if ( IsEngineDisabled() )
	{
		m_controls.throttle = 0.0f;
		m_controls.handbrake = true;
		bThrottle = false;
	}

	// throttle sounds
	// If we dropped a bunch of speed, restart the throttle
	if ( bThrottle && (m_nLastSpeed > m_nSpeed && (m_nLastSpeed - m_nSpeed > 10)) )
	{
		m_bLastThrottle = false;
	}

	// throttle down now but not before??? (or we're braking)
	if ( !m_controls.handbrake && !m_controls.brakepedal && bThrottle && !m_bLastThrottle )
	{
		m_throttleStartTime = gpGlobals->curtime;		// need to track how long throttle is down
		m_bLastThrottle = true;
	}
	// throttle up now but not before??
	else if ( !bThrottle && m_bLastThrottle && IsEngineDisabled() == false )
	{
		m_throttleActiveTime = gpGlobals->curtime - m_throttleStartTime;
		m_bLastThrottle = false;
	}

	float flSpeedPercentage = clamp( m_nSpeed / m_flMaxSpeed, 0.f, 1.f );
	vbs_sound_update_t params;
	params.Defaults();
	params.bReverse = (m_controls.throttle < 0);
	params.bThrottleDown = bThrottle;
	params.bTurbo = IsBoosting();
	params.bVehicleInWater = m_pOuterServerVehicle->IsVehicleBodyInWater();
	params.flCurrentSpeedFraction = flSpeedPercentage;
	params.flFrameTime = flFrameTime;
	params.flWorldSpaceSpeed = carState.speed;
	m_pOuterServerVehicle->SoundUpdate( params );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFourWheelVehiclePhysics::IsBoosting( void )
{
	const vehicleparams_t *pVehicleParams = &m_pVehicle->GetVehicleParams();
	const vehicle_operatingparams_t *pVehicleOperating = &m_pVehicle->GetOperatingParams();
	if ( pVehicleParams && pVehicleOperating )
	{	
		if ( ( pVehicleOperating->boostDelay - pVehicleParams->engine.boostDelay ) > 0.0f )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFourWheelVehiclePhysics::SetDisableEngine( bool bDisable )
{
	// Set the engine state.
	m_pVehicle->SetEngineDisabled( bDisable );
}

static int AddPhysToList( IPhysicsObject **pList, int listMax, int count, IPhysicsObject *pPhys )
{
	if ( pPhys )
	{
		if ( count < listMax )
		{
			pList[count] = pPhys;
			count++;
		}
	}
	return count;
}

int CFourWheelVehiclePhysics::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	int count = 0;
	// add the body
	count = AddPhysToList( pList, listMax, count, m_pOuter->VPhysicsGetObject() );
	for ( int i = 0; i < 4; i++ )
	{
		count = AddPhysToList( pList, listMax, count, m_pWheels[i] );
	}
	return count;
}
