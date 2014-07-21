//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Ichthyosaur - buh bum...  buh bum...
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_interactions.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "activitylist.h"
#include "game.h"
#include "npcevent.h"
#include "player.h"
#include "entitylist.h"
#include "soundenvelope.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_ichthyosaur_health( "sk_ichthyosaur_health", "0" );
ConVar	sk_ichthyosaur_melee_dmg( "sk_ichthyosaur_melee_dmg", "0" );

#define	ICHTHYOSAUR_MODEL	"models/ichthyosaur.mdl"

#define	ICH_HEIGHT_PREFERENCE	16.0f
#define	ICH_DEPTH_PREFERENCE	8.0f

#define	ICH_WAYPOINT_DISTANCE	64.0f

#define	ICH_AE_BITE				11
#define	ICH_AE_BITE_START		12

#define	ICH_SWIM_SPEED_WALK		150
#define	ICH_SWIM_SPEED_RUN		500

#define	ICH_MIN_TURN_SPEED		4.0f
#define	ICH_MAX_TURN_SPEED		30.0f

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())

#define	FEELER_COLLISION			0
#define	FEELER_COLLISION_VISUALIZE	(FEELER_COLLISION&&0)

enum IchthyosaurMoveType_t
{
	ICH_MOVETYPE_SEEK = 0,	// Fly through the target without stopping.
	ICH_MOVETYPE_ARRIVE		// Slow down and stop at target.
};

//
// CNPC_Ichthyosaur
//

class CNPC_Ichthyosaur : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_Ichthyosaur, CAI_BaseNPC );
	DECLARE_DATADESC();

	CNPC_Ichthyosaur( void ) {}

	int		SelectSchedule( void );
	int		MeleeAttack1Conditions( float flDot, float flDist );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int		TranslateSchedule( int type );

	void	Precache( void );
	void	Spawn( void );
	void	MoveFlyExecute( CBaseEntity *pTargetEnt, const Vector & vecDir, float flDistance, float flInterval );
	void	HandleAnimEvent( animevent_t *pEvent );
	void	PrescheduleThink( void );
	bool	OverrideMove( float flInterval );
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition );
	float	GetDefaultNavGoalTolerance();

	float	MaxYawSpeed( void );

	Class_T Classify( void )	{	return CLASS_ANTLION;	}	//FIXME: No classification for various wildlife?

	bool	FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );

private:

	bool	SteerAvoidObstacles( Vector &Steer, const Vector &Velocity, const Vector &Forward, const Vector &Right, const Vector &Up );
	bool	Beached( void );

	void	DoMovement( float flInterval, const Vector &MoveTarget, int eMoveType );
	void	SteerArrive( Vector &Steer, const Vector &Target );
	void	SteerSeek( Vector &Steer, const Vector &Target );
	void	ClampSteer( Vector &SteerAbs, Vector &SteerRel, Vector &forward, Vector &right, Vector &up );
	void	AddSwimNoise( Vector *velocity );

	void	Bite( void );
	void	EnsnareVictim( CBaseEntity *pVictim );
	void	ReleaseVictim( void );
	void	DragVictim( float moveDist );

	void	SetPoses( Vector moveRel, float speed );

	//void	IchTouch( CBaseEntity *pOther );

	float	GetGroundSpeed( void );

#if FEELER_COLLISION
	Vector	DoProbe( const Vector &Probe );
	Vector	m_LastSteer;
#endif

	CBaseEntity	*m_pVictim;

	static const Vector	m_vecAccelerationMax;
	static const Vector	m_vecAccelerationMin;

	Vector	m_vecLastMoveTarget;
		
	float	m_flNextBiteTime;
	float	m_flHoldTime;
	float	m_flSwimSpeed;
	float	m_flTailYaw;
	float	m_flTailPitch;

	float	m_flNextPingTime;
	float	m_flNextGrowlTime;

	bool	m_bHasMoveTarget;
	bool	m_bIgnoreSurface;

	//CSoundPatch	*m_pSwimSound;
	//CSoundPatch	*m_pVoiceSound;
	
	DEFINE_CUSTOM_AI;
};

//Acceleration definitions
const Vector CNPC_Ichthyosaur::m_vecAccelerationMax	= Vector(  256,  1024,  512 );
const Vector CNPC_Ichthyosaur::m_vecAccelerationMin	= Vector( -256, -1024, -512 );

//Data description
BEGIN_DATADESC( CNPC_Ichthyosaur )

// Silence classcheck
//	DEFINE_FIELD( m_LastSteer, FIELD_VECTOR ),

	DEFINE_FIELD( m_pVictim,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_vecLastMoveTarget,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextBiteTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flHoldTime,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flSwimSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flTailYaw,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flTailPitch,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextPingTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextGrowlTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_bHasMoveTarget,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIgnoreSurface,		FIELD_BOOLEAN ),

	//DEFINE_FUNCTION( IchTouch ),

END_DATADESC()

//Schedules
enum IchSchedules
{
	SCHED_ICH_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_ICH_PATROL_RUN,
	SCHED_ICH_PATROL_WALK,
	SCHED_ICH_DROWN_VICTIM,
	SCHED_ICH_MELEE_ATTACK1,
	SCHED_ICH_THRASH,
};

//Tasks
enum IchTasks
{
	TASK_ICH_GET_PATH_TO_RANDOM_NODE = LAST_SHARED_TASK,
	TASK_ICH_GET_PATH_TO_DROWN_NODE,
	TASK_ICH_THRASH_PATH,
};

//Activities
int	ACT_ICH_THRASH;
int	ACT_ICH_BITE_HIT;
int	ACT_ICH_BITE_MISS;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::InitCustomSchedules( void ) 
{
	INIT_CUSTOM_AI( CNPC_Ichthyosaur );

	//Interaction	REGISTER_INTERACTION( g_interactionAntlionAttacked );

	//Schedules
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_CHASE_ENEMY );
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_PATROL_RUN );
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_PATROL_WALK );
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_DROWN_VICTIM );
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_MELEE_ATTACK1 );
	ADD_CUSTOM_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_THRASH );
	
	//Tasks
	ADD_CUSTOM_TASK( CNPC_Ichthyosaur,		TASK_ICH_GET_PATH_TO_RANDOM_NODE );
	ADD_CUSTOM_TASK( CNPC_Ichthyosaur,		TASK_ICH_GET_PATH_TO_DROWN_NODE );
	ADD_CUSTOM_TASK( CNPC_Ichthyosaur,		TASK_ICH_THRASH_PATH );

	//Conditions	ADD_CUSTOM_CONDITION( CNPC_CombineGuard,	COND_ANTLIONGRUB_HEARD_SQUEAL );

	//Activities
	ADD_CUSTOM_ACTIVITY( CNPC_Ichthyosaur,	ACT_ICH_THRASH );
	ADD_CUSTOM_ACTIVITY( CNPC_Ichthyosaur,	ACT_ICH_BITE_HIT );
	ADD_CUSTOM_ACTIVITY( CNPC_Ichthyosaur,	ACT_ICH_BITE_MISS );

	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_CHASE_ENEMY );
	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_PATROL_RUN );
	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_PATROL_WALK );
	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_DROWN_VICTIM );
	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_MELEE_ATTACK1 );
	AI_LOAD_SCHEDULE( CNPC_Ichthyosaur,	SCHED_ICH_THRASH );
}

LINK_ENTITY_TO_CLASS( npc_ichthyosaur, CNPC_Ichthyosaur );
IMPLEMENT_CUSTOM_AI( npc_ichthyosaur, CNPC_Ichthyosaur );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::Precache( void )
{
	PrecacheModel( ICHTHYOSAUR_MODEL );

	PrecacheScriptSound( "NPC_Ichthyosaur.Bite" );
	PrecacheScriptSound( "NPC_Ichthyosaur.BiteMiss" );
	PrecacheScriptSound( "NPC_Ichthyosaur.AttackGrowl" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::Spawn( void )
{
	Precache();

	SetModel( ICHTHYOSAUR_MODEL );

	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType( NAV_FLY );
	m_NPCState				= NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth				= sk_ichthyosaur_health.GetFloat();
	m_iMaxHealth			= m_iHealth;
	m_flFieldOfView			= -0.707;	// 270 degrees
	SetDistLook( 1024 );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	AddFlag( FL_FLY | FL_STEPMOVEMENT );

	m_flGroundSpeed			= ICH_SWIM_SPEED_RUN;

	m_bIgnoreSurface		= false;

	m_flSwimSpeed			= 0.0f;
	m_flTailYaw				= 0.0f;
	m_flTailPitch			= 0.0f;

	m_flNextBiteTime		= gpGlobals->curtime;
	m_flHoldTime			= gpGlobals->curtime;
	m_flNextPingTime		= gpGlobals->curtime;
	m_flNextGrowlTime		= gpGlobals->curtime;

#if FEELER_COLLISION

	Vector	forward;

	GetVectors( &forward, NULL, NULL );

	m_vecCurrentVelocity	= forward * m_flGroundSpeed;

#endif

	//SetTouch( IchTouch );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_INNATE_MELEE_ATTACK1 );

	NPCInit();

	//m_pSwimSound	= ENVELOPE_CONTROLLER.SoundCreate( edict(), CHAN_BODY,	"xxxCONVERTTOGAMESOUNDS!!!npc/ichthyosaur/ich_amb1wav", ATTN_NORM );
	//m_pVoiceSound	= ENVELOPE_CONTROLLER.SoundCreate( edict(), CHAN_VOICE,	"xxxCONVERTTOGAMESOUNDS!!!npc/ichthyosaur/water_breathwav", ATTN_IDLE );

	//ENVELOPE_CONTROLLER.Play( m_pSwimSound,	1.0f, 100 );
	//ENVELOPE_CONTROLLER.Play( m_pVoiceSound,1.0f, 100 );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
/*
void CNPC_Ichthyosaur::IchTouch( CBaseEntity *pOther )
{
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_Ichthyosaur::SelectSchedule( void )
{
	if ( m_NPCState == NPC_STATE_COMBAT )
	{
		if ( m_flHoldTime > gpGlobals->curtime )
			return SCHED_ICH_DROWN_VICTIM;

		if ( m_flNextBiteTime > gpGlobals->curtime )
			return	SCHED_PATROL_RUN;

		if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
			return	SCHED_MELEE_ATTACK1;

		return SCHED_CHASE_ENEMY;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_Ichthyosaur::OverrideMove( float flInterval )
{
	m_flGroundSpeed = GetGroundSpeed();

	if ( m_bHasMoveTarget )
	{
		DoMovement( flInterval, m_vecLastMoveTarget, ICH_MOVETYPE_ARRIVE );
	}
	else
	{
		DoMovement( flInterval, GetLocalOrigin(), ICH_MOVETYPE_ARRIVE );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &probe - 
// Output : Vector
//-----------------------------------------------------------------------------
#if FEELER_COLLISION

Vector CNPC_Ichthyosaur::DoProbe( const Vector &probe )
{
	trace_t	tr;
	float	fraction = 1.0f;
	bool	collided = false;
	Vector	normal	 = Vector( 0, 0, -1 );

	float	waterLevel = UTIL_WaterLevel( GetAbsOrigin(), GetAbsOrigin().z, GetAbsOrigin().z+150 );

	waterLevel -= GetAbsOrigin().z;
	waterLevel /= 150;

	if ( waterLevel < 1.0f )
	{
		collided = true;
		fraction = waterLevel;
	}

	AI_TraceHull( GetAbsOrigin(), probe, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	
	if ( ( collided == false ) || ( tr.fraction < fraction ) )
	{
		fraction	= tr.fraction;
		normal		= tr.plane.normal;
	}

	if ( ( fraction < 1.0f ) && ( GetEnemy() == NULL || tr.u.ent != GetEnemy()->pev ) )
	{
#if FEELER_COLLISION_VISUALIZE
		NDebugOverlay::Line( GetLocalOrigin(), probe, 255, 0, 0, false, 0.1f );
#endif
		
		Vector	probeDir = probe - GetLocalOrigin();

		Vector	normalToProbeAndWallNormal = probeDir.Cross( normal );
		Vector	steeringVector = normalToProbeAndWallNormal.Cross( probeDir );

		Vector	velDir = GetAbsVelocity();
		VectorNormalize( velDir );

		float	steeringForce = m_flGroundSpeed * ( 1.0f - fraction ) * normal.Dot( velDir );

		if ( steeringForce < 0.0f )
		{
			steeringForce = -steeringForce;
		}

		velDir = steeringVector;
		VectorNormalize( velDir );

		steeringVector = steeringForce * velDir;
		
		return steeringVector;
	}

#if FEELER_COLLISION_VISUALIZE
	NDebugOverlay::Line( GetLocalOrigin(), probe, 0, 255, 0, false, 0.1f );
#endif

	return Vector( 0.0f, 0.0f, 0.0f );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Move the victim of a drag along with us
// Input  : moveDist - our amount of travel
//-----------------------------------------------------------------------------

#define	DRAG_OFFSET	50.0f

void CNPC_Ichthyosaur::DragVictim( float moveDist )
{
	Vector	mins, maxs;
	float	width;
	
	mins	= WorldAlignMins();
	maxs	= WorldAlignMaxs();
	width	= ( maxs.y - mins.y ) * 0.5f;

	Vector	forward, up;
	GetVectors( &forward, NULL, &up );

	Vector	newPos = GetAbsOrigin() + ( (forward+(up*0.25f)) * ( moveDist + width + DRAG_OFFSET ) );

	trace_t	tr;
	AI_TraceEntity( this, m_pVictim->GetAbsOrigin(), newPos, MASK_NPCSOLID, &tr );

	if ( ( tr.fraction == 1.0f ) && ( tr.m_pEnt != this ) )
	{
		UTIL_SetOrigin( m_pVictim, tr.endpos );
	}
	else
	{
		ReleaseVictim();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines the pose parameters for the bending of the body and tail speed
// Input  : moveRel - the dot products for the deviation off of each direction (f,r,u)
//			speed - speed of the fish
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::SetPoses( Vector moveRel, float speed )
{
	float	movePerc, moveBase;

	//Find out how fast we're moving in our animations boundaries
	if ( GetIdealActivity() == ACT_WALK )
	{
		moveBase = 0.5f;
		movePerc = moveBase * ( speed / ICH_SWIM_SPEED_WALK );
	}
	else
	{
		moveBase = 1.0f;
		movePerc = moveBase * ( speed / ICH_SWIM_SPEED_RUN );
	}
	
	Vector	tailPosition;
	float	flSwimSpeed = movePerc;

	//Forward deviation
	if ( moveRel.x > 0 )
	{
		flSwimSpeed *= moveBase + (( moveRel.x / m_vecAccelerationMax.x )*moveBase);
	}
	else if ( moveRel.x < 0 )
	{
		flSwimSpeed *= moveBase - (( moveRel.x / m_vecAccelerationMin.x )*moveBase);
	}

	//Vertical deviation
	if ( moveRel.z > 0 )
	{
		tailPosition[PITCH]	= -90.0f * ( moveRel.z / m_vecAccelerationMax.z );
	}
	else if ( moveRel.z < 0 )
	{
		tailPosition[PITCH]	= 90.0f * ( moveRel.z / m_vecAccelerationMin.z );
	}
	else
	{
		tailPosition[PITCH]	= 0.0f;
	}

	//Lateral deviation
	if ( moveRel.y > 0 )
	{
		tailPosition[ROLL]	= 25 * moveRel.y / m_vecAccelerationMax.y;
		tailPosition[YAW]	= -1.0f * moveRel.y / m_vecAccelerationMax.y;
	}
	else if ( moveRel.y < 0 )
	{
		tailPosition[ROLL]	= -25 * moveRel.y / m_vecAccelerationMin.y;
		tailPosition[YAW]	= moveRel.y / m_vecAccelerationMin.y;
	}
	else
	{
		tailPosition[ROLL]	= 0.0f;
		tailPosition[YAW]	= 0.0f;
	}
	
	//Clamp
	flSwimSpeed			= clamp( flSwimSpeed, 0.25f, 1.0f );
	tailPosition[YAW]	= clamp( tailPosition[YAW], -90.0f, 90.0f );
	tailPosition[PITCH]	= clamp( tailPosition[PITCH], -90.0f, 90.0f );

	//Blend
	m_flTailYaw		= ( m_flTailYaw * 0.8f ) + ( tailPosition[YAW] * 0.2f );
	m_flTailPitch	= ( m_flTailPitch * 0.8f ) + ( tailPosition[PITCH] * 0.2f );
	m_flSwimSpeed	= ( m_flSwimSpeed * 0.8f ) + ( flSwimSpeed * 0.2f );

	//Pose the body
	SetPoseParameter( 0, m_flSwimSpeed );
	SetPoseParameter( 1, m_flTailYaw );
	SetPoseParameter( 2, m_flTailPitch );
	
	//FIXME: Until the sequence info is reset properly after SetPoseParameter
	if ( ( GetActivity() == ACT_RUN ) || ( GetActivity() == ACT_WALK ) )
	{
		ResetSequenceInfo();
	}

	//Face our current velocity
	GetMotor()->SetIdealYawAndUpdate( UTIL_AngleMod( CalcIdealYaw( GetAbsOrigin() + GetAbsVelocity() ) ), AI_KEEP_YAW_SPEED );

	float	pitch = 0.0f;

	if ( speed != 0.0f )
	{
		pitch = -RAD2DEG( asin( GetAbsVelocity().z / speed ) );
	}

	//FIXME: Framerate dependant
	QAngle angles = GetLocalAngles();

	angles.x = (angles.x * 0.8f) + (pitch * 0.2f);
	angles.z = (angles.z * 0.9f) + (tailPosition[ROLL] * 0.1f);

	SetLocalAngles( angles );
}

#define	LATERAL_NOISE_MAX	2.0f
#define	LATERAL_NOISE_FREQ	1.0f

#define	VERTICAL_NOISE_MAX	2.0f
#define	VERTICAL_NOISE_FREQ	1.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : velocity - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::AddSwimNoise( Vector *velocity )
{
	Vector	right, up;

	GetVectors( NULL, &right, &up );

	float	lNoise, vNoise;

	lNoise = LATERAL_NOISE_MAX * sin( gpGlobals->curtime * LATERAL_NOISE_FREQ );
	vNoise = VERTICAL_NOISE_MAX * sin( gpGlobals->curtime * VERTICAL_NOISE_FREQ );

	(*velocity) += ( right * lNoise ) + ( up * vNoise );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//			&m_LastMoveTarget - 
//			eMoveType - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::DoMovement( float flInterval, const Vector &MoveTarget, int eMoveType )
{
	// dvs: something is setting this bit, causing us to stop moving and get stuck that way
	Forget( bits_MEMORY_TURNING );

	Vector Steer, SteerAvoid, SteerRel;
	Vector forward, right, up;

	//Get our orientation vectors.
	GetVectors( &forward, &right, &up);

	if ( ( GetActivity() == ACT_MELEE_ATTACK1 ) && ( GetEnemy() != NULL ) )
	{
		SteerSeek( Steer, GetEnemy()->GetAbsOrigin() );
	}
	else
	{
		//If we are approaching our goal, use an arrival steering mechanism.
		if ( eMoveType == ICH_MOVETYPE_ARRIVE )
		{
			SteerArrive( Steer, MoveTarget );
		}
		else
		{
			//Otherwise use a seek steering mechanism.
			SteerSeek( Steer, MoveTarget );
		}
	}
	
#if FEELER_COLLISION

	Vector f, u, l, r, d;

	float	probeLength = GetAbsVelocity().Length();

	if ( probeLength < 150 )
		probeLength = 150;

	if ( probeLength > 500 )
		probeLength = 500;

	f = DoProbe( GetLocalOrigin() + (probeLength * forward) );
	r = DoProbe( GetLocalOrigin() + (probeLength/3 * (forward+right)) );
	l = DoProbe( GetLocalOrigin() + (probeLength/3 * (forward-right)) );
	u = DoProbe( GetLocalOrigin() + (probeLength/3 * (forward+up)) );
	d = DoProbe( GetLocalOrigin() + (probeLength/3 * (forward-up)) );

	SteerAvoid = f+r+l+u+d;
	
	//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin()+SteerAvoid, 255, 255, 0, false, 0.1f );	

	if ( SteerAvoid.LengthSqr() )
	{
		Steer = (SteerAvoid*0.5f);
	}

	m_vecVelocity = m_vecVelocity + (Steer*0.5f);

	VectorNormalize( m_vecVelocity );

	SteerRel.x = forward.Dot( m_vecVelocity );
	SteerRel.y = right.Dot( m_vecVelocity );
	SteerRel.z = up.Dot( m_vecVelocity );

	m_vecVelocity *= m_flGroundSpeed;

#else

	//See if we need to avoid any obstacles.
	if ( SteerAvoidObstacles( SteerAvoid, GetAbsVelocity(), forward, right, up ) )
	{
		//Take the avoidance vector
		Steer = SteerAvoid;
	}

	//Clamp our ideal steering vector to within our physical limitations.
	ClampSteer( Steer, SteerRel, forward, right, up );

	ApplyAbsVelocityImpulse( Steer * flInterval );
	
#endif

	Vector vecNewVelocity = GetAbsVelocity();
	float flLength = vecNewVelocity.Length();

	//Clamp our final speed
	if ( flLength > m_flGroundSpeed )
	{
		vecNewVelocity *= ( m_flGroundSpeed / flLength );
		flLength = m_flGroundSpeed;
	}

	Vector	workVelocity = vecNewVelocity;

	AddSwimNoise( &workVelocity );

	// Pose the fish properly
	SetPoses( SteerRel, flLength );

	//Drag our victim before moving
	if ( m_pVictim != NULL )
	{
		DragVictim( (workVelocity*flInterval).Length() );
	}

	//Move along the current velocity vector
	if ( WalkMove( workVelocity * flInterval, MASK_NPCSOLID ) == false )
	{
		//Attempt a half-step
		if ( WalkMove( (workVelocity*0.5f) * flInterval,  MASK_NPCSOLID) == false )
		{
			//Restart the velocity
			//VectorNormalize( m_vecVelocity );
			vecNewVelocity *= 0.5f;
		}
		else
		{
			//Cut our velocity in half
			vecNewVelocity *= 0.5f;
		}
	}

	SetAbsVelocity( vecNewVelocity );

}

//-----------------------------------------------------------------------------
// Purpose: Gets a steering vector to arrive at a target location with a
//			relatively small velocity.
// Input  : Steer - Receives the ideal steering vector.
//			Target - Target position at which to arrive.
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::SteerArrive(Vector &Steer, const Vector &Target)
{
	Vector Offset = Target - GetLocalOrigin();
	float fTargetDistance = Offset.Length();

	float fIdealSpeed = m_flGroundSpeed * (fTargetDistance / ICH_WAYPOINT_DISTANCE);
	float fClippedSpeed = MIN( fIdealSpeed, m_flGroundSpeed );
	
	Vector DesiredVelocity( 0, 0, 0 );

	if ( fTargetDistance > ICH_WAYPOINT_DISTANCE )
	{
		DesiredVelocity = (fClippedSpeed / fTargetDistance) * Offset;
	}

	Steer = DesiredVelocity - GetAbsVelocity();
}


//-----------------------------------------------------------------------------
// Purpose: Gets a steering vector to move towards a target position as quickly
//			as possible.
// Input  : Steer - Receives the ideal steering vector.
//			Target - Target position to seek.
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::SteerSeek( Vector &Steer, const Vector &Target )
{
	Vector offset = Target - GetLocalOrigin();
	
	VectorNormalize( offset );
	
	Vector DesiredVelocity = m_flGroundSpeed * offset;
	
	Steer = DesiredVelocity - GetAbsVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &Steer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Ichthyosaur::SteerAvoidObstacles(Vector &Steer, const Vector &Velocity, const Vector &Forward, const Vector &Right, const Vector &Up)
{
	trace_t	tr;

	bool	collided = false;
	Vector	dir = Velocity;
	float	speed = VectorNormalize( dir );

	//Look ahead one second and avoid whatever is in our way.
	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + (dir*speed), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	Vector	forward;

	GetVectors( &forward, NULL, NULL );

	//If we're hitting our enemy, just continue on
	if ( ( GetEnemy() != NULL ) && ( tr.m_pEnt == GetEnemy() ) )
		return false;

	if ( tr.fraction < 1.0f )
	{
		CBaseEntity *pBlocker = tr.m_pEnt;
		
		if ( ( pBlocker != NULL ) && ( pBlocker->MyNPCPointer() != NULL ) )
		{
			DevMsg( 2, "Avoiding an NPC\n" );

			Vector HitOffset = tr.endpos - GetAbsOrigin();

			Vector SteerUp = CrossProduct( HitOffset, Velocity );
			Steer = CrossProduct(  SteerUp, Velocity  );
			VectorNormalize( Steer );

			/*Vector probeDir = tr.endpos - GetAbsOrigin();
			Vector normalToProbeAndWallNormal = probeDir.Cross( tr.plane.normal );
			
			Steer = normalToProbeAndWallNormal.Cross( probeDir );
			VectorNormalize( Steer );*/

			if ( tr.fraction > 0 )
			{
				Steer = (Steer * Velocity.Length()) / tr.fraction;
				//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin()+Steer, 255, 0, 0, false, 0.1f );
			}
			else
			{
				Steer = (Steer * 1000 * Velocity.Length());
				//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin()+Steer, 255, 0, 0, false, 0.1f );
			}
		}
		else
		{
			if ( ( pBlocker != NULL ) && ( pBlocker == GetEnemy() ) )
			{
				DevMsg( "Avoided collision\n" );
				return false;
			}

			DevMsg( 2, "Avoiding the world\n" );
			
			Vector	steeringVector = tr.plane.normal;

			if ( tr.fraction == 0.0f )
				return false;

			Steer = steeringVector * ( Velocity.Length() / tr.fraction );
			
			//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin()+Steer, 255, 0, 0, false, 0.1f );
		}

		//return true;
		collided = true;
	}

	//Try to remain 8 feet above the ground.
	AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -ICH_HEIGHT_PREFERENCE), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		Steer += Vector( 0, 0, m_vecAccelerationMax.z / tr.fraction );
		collided = true;
	}
	
	//Stay under the surface
	if ( m_bIgnoreSurface == false )
	{
		float waterLevel = ( UTIL_WaterLevel( GetAbsOrigin(), GetAbsOrigin().z, GetAbsOrigin().z+ICH_DEPTH_PREFERENCE ) - GetAbsOrigin().z ) / ICH_DEPTH_PREFERENCE;

		if ( waterLevel < 1.0f )
		{
			Steer += -Vector( 0, 0, m_vecAccelerationMax.z / waterLevel );
			collided = true;
		}
	}

	return collided;
}


//-----------------------------------------------------------------------------
// Purpose: Clamps the desired steering vector based on the limitations of this
//			vehicle.
// Input  : SteerAbs - The vector indicating our ideal steering vector. Receives
//				the clamped steering vector in absolute (x,y,z) coordinates.
//			SteerRel - Receives the clamped steering vector in relative (forward, right, up)
//				coordinates.
//			forward - Our current forward vector.
//			right - Our current right vector.
//			up - Our current up vector.
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::ClampSteer(Vector &SteerAbs, Vector &SteerRel, Vector &forward, Vector &right, Vector &up)
{
	float fForwardSteer = DotProduct(SteerAbs, forward);
	float fRightSteer = DotProduct(SteerAbs, right);
	float fUpSteer = DotProduct(SteerAbs, up);

	if (fForwardSteer > 0)
	{
		fForwardSteer = MIN(fForwardSteer, m_vecAccelerationMax.x);
	}
	else
	{
		fForwardSteer = MAX(fForwardSteer, m_vecAccelerationMin.x);
	}

	if (fRightSteer > 0)
	{
		fRightSteer = MIN(fRightSteer, m_vecAccelerationMax.y);
	}
	else
	{
		fRightSteer = MAX(fRightSteer, m_vecAccelerationMin.y);
	}

	if (fUpSteer > 0)
	{
		fUpSteer = MIN(fUpSteer, m_vecAccelerationMax.z);
	}
	else
	{
		fUpSteer = MAX(fUpSteer, m_vecAccelerationMin.z);
	}

	SteerAbs = (fForwardSteer*forward) + (fRightSteer*right) + (fUpSteer*up);

	SteerRel.x = fForwardSteer;
	SteerRel.y = fRightSteer;
	SteerRel.z = fUpSteer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTargetEnt - 
//			vecDir - 
//			flDistance - 
//			flInterval - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::MoveFlyExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flDistance, float flInterval )
{
	IchthyosaurMoveType_t eMoveType = ( GetNavigator()->CurWaypointIsGoal() ) ? ICH_MOVETYPE_ARRIVE : ICH_MOVETYPE_SEEK;

	m_flGroundSpeed = GetGroundSpeed();

	Vector	moveGoal = GetNavigator()->GetCurWaypointPos();

	//See if we can move directly to our goal
	if ( ( GetEnemy() != NULL ) && ( GetNavigator()->GetGoalTarget() == (CBaseEntity *) GetEnemy() ) )
	{
		trace_t	tr;
		Vector	goalPos = GetEnemy()->GetAbsOrigin() + ( GetEnemy()->GetSmoothedVelocity() * 0.5f );

		AI_TraceHull( GetAbsOrigin(), goalPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, GetEnemy(), COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
		{
			moveGoal = tr.endpos;
		}
	}

	//Move
	DoMovement( flInterval, moveGoal, eMoveType );

	//Save the info from that run
	m_vecLastMoveTarget	= moveGoal;
	m_bHasMoveTarget	= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Ichthyosaur::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	// don't look through water
	if ( GetWaterLevel() != pEntity->GetWaterLevel() )
		return false;

	return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
}

//-----------------------------------------------------------------------------
// Purpose: Get our conditions for a melee attack
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Ichthyosaur::MeleeAttack1Conditions( float flDot, float flDist )
{
	Vector	predictedDir	= ( (GetEnemy()->GetAbsOrigin()+(GetEnemy()->GetSmoothedVelocity())) - GetAbsOrigin() );	
	float	flPredictedDist = VectorNormalize( predictedDir );
	
	Vector	vBodyDir;
	GetVectors( &vBodyDir, NULL, NULL );

	float	flPredictedDot	= DotProduct( predictedDir, vBodyDir );

	if ( flPredictedDot < 0.8f )
		return COND_NOT_FACING_ATTACK;

	if ( ( flPredictedDist > ( GetAbsVelocity().Length() * 0.5f) ) && ( flDist > 128.0f ) )
		return COND_TOO_FAR_TO_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case ICH_AE_BITE:
		Bite();
		break;

	case ICH_AE_BITE_START:
		{
			EmitSound( "NPC_Ichthyosaur.AttackGrowl" );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::Bite( void )
{
	//Don't allow another bite too soon
	if ( m_flNextBiteTime > gpGlobals->curtime )
		return;

	CBaseEntity *pHurt;
	
	//FIXME: E3 HACK - Always damage bullseyes if we're scripted to hit them
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		pHurt = GetEnemy();
	}
	else
	{
		pHurt = CheckTraceHullAttack( 108, Vector(-32,-32,-32), Vector(32,32,32), 0, DMG_CLUB );
	}

	//Hit something
	if ( pHurt != NULL )
	{
		CTakeDamageInfo info( this, this, sk_ichthyosaur_melee_dmg.GetInt(), DMG_CLUB );

		if ( pHurt->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pHurt );

			if ( pPlayer )
			{
				if ( ( ( m_flHoldTime < gpGlobals->curtime ) && ( pPlayer->m_iHealth < (pPlayer->m_iMaxHealth*0.5f)) ) || ( pPlayer->GetWaterLevel() < 1 ) )
				{
					//EnsnareVictim( pHurt );
				}
				else
				{
					info.SetDamage( sk_ichthyosaur_melee_dmg.GetInt() * 3 );
				}
				CalculateMeleeDamageForce( &info, GetAbsVelocity(), pHurt->GetAbsOrigin() );
				pHurt->TakeDamage( info );

				color32 red = {64, 0, 0, 255};
				UTIL_ScreenFade( pPlayer, red, 0.5, 0, FFADE_IN  );

				//Disorient the player
				QAngle angles = pPlayer->GetLocalAngles();

				angles.x += random->RandomInt( 60, 25 );
				angles.y += random->RandomInt( 60, 25 );
				angles.z = 0.0f;

				pPlayer->SetLocalAngles( angles );

				pPlayer->SnapEyeAngles( angles );
			}
		}
		else
		{
			CalculateMeleeDamageForce( &info, GetAbsVelocity(), pHurt->GetAbsOrigin() );
			pHurt->TakeDamage( info );
		}

		m_flNextBiteTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 4.0f );

		//Bubbles!
		UTIL_Bubbles( pHurt->GetAbsOrigin()+Vector(-32.0f,-32.0f,-32.0f), pHurt->GetAbsOrigin()+Vector(32.0f,32.0f,0.0f), random->RandomInt( 16, 32 ) );
		
		// Play a random attack hit sound
		EmitSound( "NPC_Ichthyosaur.Bite" );

		if ( GetActivity() == ACT_MELEE_ATTACK1 )
		{
			SetActivity( (Activity) ACT_ICH_BITE_HIT );
		}
		
		return;
	}

	//Play the miss animation and sound
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		SetActivity( (Activity) ACT_ICH_BITE_MISS );
	}

	//Miss sound
	EmitSound( "NPC_Ichthyosaur.BiteMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Ichthyosaur::Beached( void )
{
	trace_t	tr;
	Vector	testPos;

	testPos = GetAbsOrigin() - Vector( 0, 0, ICH_DEPTH_PREFERENCE );
	
	AI_TraceHull( GetAbsOrigin(), testPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	return ( tr.fraction < 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	//Ambient sounds
	/*
	if ( random->RandomInt( 0, 20 ) == 10 )
	{
		if ( random->RandomInt( 0, 1 ) )
		{
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pSwimSound, random->RandomFloat( 0.0f, 0.5f ), 1.0f );
		}
		else
		{
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pVoiceSound, random->RandomFloat( 0.0f, 0.5f ), 1.0f );
		}
	}
	*/

	//Pings
	if ( m_flNextPingTime < gpGlobals->curtime )
	{
		m_flNextPingTime = gpGlobals->curtime + random->RandomFloat( 3.0f, 8.0f );
	}
	
	//Growls
	if ( ( m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT ) && ( m_flNextGrowlTime < gpGlobals->curtime ) )
	{
		m_flNextGrowlTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 6.0f );
	}

	//Randomly emit bubbles
	if ( random->RandomInt( 0, 10 ) == 0 )
	{
		UTIL_Bubbles( GetAbsOrigin()+(GetHullMins()*0.5f), GetAbsOrigin()+(GetHullMaxs()*0.5f), 1 );
	}

	//Check our water level
	if ( GetWaterLevel() != 3 )
	{
		if ( GetWaterLevel() < 2 )
		{
			DevMsg( 2, "Came out of water\n" );
			
			if ( Beached() )
			{
				SetSchedule( SCHED_ICH_THRASH );

				Vector vecNewVelocity = GetAbsVelocity();
				vecNewVelocity[2] = 8.0f;
				SetAbsVelocity( vecNewVelocity );
			}
		}
		else
		{
			//TODO: Wake effects
		}
	}

	//If we have a victim, update them
	if ( m_pVictim != NULL )
	{
		//See if it's time to release the victim
		if ( m_flHoldTime < gpGlobals->curtime )
		{
			ReleaseVictim();
			return;
		}

		Bite();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pevInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
int	CNPC_Ichthyosaur::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	//Release the player if he's struck us while being held
	if ( m_flHoldTime > gpGlobals->curtime )
	{
		ReleaseVictim();
		
		//Don't give them as much time to flee
		m_flNextBiteTime = gpGlobals->curtime + 2.0f;

		SetSchedule( SCHED_ICH_THRASH );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::EnsnareVictim( CBaseEntity *pVictim )
{
	CBaseCombatCharacter* pBCC = (CBaseCombatCharacter *) pVictim;

	if ( pBCC && pBCC->DispatchInteraction( g_interactionBarnacleVictimGrab, NULL, this ) )
	{
		if ( pVictim->IsPlayer() )
		{
			CBasePlayer	*pPlayer = dynamic_cast< CBasePlayer * >((CBaseEntity *) pVictim);

			if ( pPlayer )
			{
				m_flHoldTime = MAX( gpGlobals->curtime+3.0f, pPlayer->PlayerDrownTime() - 2.0f );
			}
		}
		else
		{
			m_flHoldTime	= gpGlobals->curtime + 4.0f;
		}
	
		m_pVictim = pVictim;
		m_pVictim->AddSolidFlags( FSOLID_NOT_SOLID );

		SetSchedule( SCHED_ICH_DROWN_VICTIM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::ReleaseVictim( void )
{
	CBaseCombatCharacter *pBCC = (CBaseCombatCharacter *) m_pVictim;

	pBCC->DispatchInteraction( g_interactionBarnacleVictimReleased, NULL, this );

	m_pVictim->RemoveSolidFlags( FSOLID_NOT_SOLID );

	m_pVictim			= NULL;
	m_flNextBiteTime	= gpGlobals->curtime + 8.0f;
	m_flHoldTime		= gpGlobals->curtime - 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : speed to move at
//-----------------------------------------------------------------------------
float CNPC_Ichthyosaur::GetGroundSpeed( void )
{
	if ( m_flHoldTime > gpGlobals->curtime )
		return	ICH_SWIM_SPEED_WALK/2.0f;

	if ( GetIdealActivity() == ACT_WALK )
		return ICH_SWIM_SPEED_WALK;

	if ( GetIdealActivity() == ACT_ICH_THRASH )
		return ICH_SWIM_SPEED_WALK;

	return ICH_SWIM_SPEED_RUN;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Ichthyosaur::TranslateSchedule( int type )
{
	if ( type == SCHED_CHASE_ENEMY )	return SCHED_ICH_CHASE_ENEMY;
	//if ( type == SCHED_IDLE_STAND )		return SCHED_PATROL_WALK;
	if ( type == SCHED_PATROL_RUN )		return SCHED_ICH_PATROL_RUN;
	if ( type == SCHED_PATROL_WALK )	return SCHED_ICH_PATROL_WALK;
	if ( type == SCHED_MELEE_ATTACK1 )	return SCHED_ICH_MELEE_ATTACK1;

	return BaseClass::TranslateSchedule( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ICH_THRASH_PATH:
		GetNavigator()->SetMovementActivity( (Activity) ACT_ICH_THRASH );
		TaskComplete();
		break;

	case TASK_ICH_GET_PATH_TO_RANDOM_NODE:
		{
			if ( GetEnemy() == NULL || !GetNavigator()->SetRandomGoal( GetEnemy()->GetLocalOrigin(), pTask->flTaskData ) )
			{
				if (!GetNavigator()->SetRandomGoal( pTask->flTaskData ) )
				{
					TaskFail(FAIL_NO_REACHABLE_NODE);
					return;
				}
			}
					
			TaskComplete();
		}
		break;

	case TASK_ICH_GET_PATH_TO_DROWN_NODE:
		{
			Vector	drownPos = GetLocalOrigin() - Vector( 0, 0, pTask->flTaskData );

			if ( GetNavigator()->SetGoal( drownPos, AIN_CLEAR_TARGET ) == false )
			{
				TaskFail( FAIL_NO_ROUTE );
				return;
			}

			TaskComplete();
		}
		break;

	case TASK_MELEE_ATTACK1:
		m_flPlaybackRate = 1.0f;
		BaseClass::StartTask(pTask);
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ICH_GET_PATH_TO_RANDOM_NODE:
		return;
		break;

	case TASK_ICH_GET_PATH_TO_DROWN_NODE:
		return;
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : desired yaw speed
//-----------------------------------------------------------------------------
float CNPC_Ichthyosaur::MaxYawSpeed( void )
{
	if ( GetIdealActivity() == ACT_MELEE_ATTACK1 )
		return 16.0f;

	if ( GetIdealActivity() == ACT_ICH_THRASH )
		return 16.0f;

	//Ramp up the yaw speed as we increase our speed
	return ICH_MIN_TURN_SPEED + ( (ICH_MAX_TURN_SPEED-ICH_MIN_TURN_SPEED) * ( fabs(GetAbsVelocity().Length()) / ICH_SWIM_SPEED_RUN ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//			&tolerance - 
//-----------------------------------------------------------------------------
void CNPC_Ichthyosaur::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	Vector offset = pEnemy->EyePosition() - pEnemy->GetAbsOrigin();
	chasePosition += offset;
}

float CNPC_Ichthyosaur::GetDefaultNavGoalTolerance()
{
	return GetHullWidth()*2.0f;	
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//==================================================
// SCHED_ICH_CHASE_ENEMY
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_CHASE_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ICH_PATROL_WALK"
	"		TASK_SET_TOLERANCE_DISTANCE		64"
	"		TASK_SET_GOAL					GOAL:ENEMY"
	"		TASK_GET_PATH_TO_GOAL			PATH:TRAVEL"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_LOST_ENEMY"
	"		COND_TASK_FAILED"
);
	
//==================================================
// SCHED_ICH_PATROL_RUN
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_PATROL_RUN,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"		TASK_SET_TOLERANCE_DISTANCE			64"
	"		TASK_SET_ROUTE_SEARCH_TIME			4"
	"		TASK_ICH_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	""
	"	Interrupts"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

//==================================================
// SCHED_ICH_PATROL_WALK
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_PATROL_WALK,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"		TASK_SET_TOLERANCE_DISTANCE			64"
	"		TASK_SET_ROUTE_SEARCH_TIME			4"
	"		TASK_ICH_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	""
	"	Interrupts"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

//==================================================
// SCHED_ICH_DROWN_VICTIM
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_DROWN_VICTIM,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"		TASK_SET_TOLERANCE_DISTANCE			64"
	"		TASK_SET_ROUTE_SEARCH_TIME			4"
	"		TASK_ICH_GET_PATH_TO_DROWN_NODE		256"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// SCHED_ICH_MELEE_ATTACK1
//=========================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_MELEE_ATTACK1,

	"	Tasks"
	"		TASK_ANNOUNCE_ATTACK	1"
	"		TASK_MELEE_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_OCCLUDED"
);

//==================================================
// SCHED_ICH_THRASH
//==================================================

AI_DEFINE_SCHEDULE
(
	SCHED_ICH_THRASH,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"		TASK_SET_TOLERANCE_DISTANCE			64"
	"		TASK_SET_ROUTE_SEARCH_TIME			4"
	"		TASK_ICH_GET_PATH_TO_RANDOM_NODE	64"
	"		TASK_ICH_THRASH_PATH				0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	""
	"	Interrupts"
);
