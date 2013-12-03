//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vehicle_base.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "IEffects.h"
#include "beam_shared.h"
#include "weapon_gauss.h"
#include "soundenvelope.h"
#include "decals.h"
#include "soundent.h"
#include "te_effect_dispatch.h"

#define	VEHICLE_HITBOX_DRIVER	1

#define JETSKI_SHOCK_LENGTH_LONG2			0.35f	//
#define JETSKI_SHOCK_LENGTH_LONG			0.25f	// meters (about 12 inches)
#define JETSKI_SHOCK_LENGTH_REST			0.15f	// meters (about 8 inches)
#define JETSKI_SHOCK_LENGTH_SHORT			0.1f	// meters (about 4 inches)

#define JETSKI_PITCH_AND_ROLL_RATE			0.02f

#define JETSKI_FRICTION_MIN					0.3f
#define JETSKI_FRICTION_MAX					0.8f

#define JETSKI_SPLASH_DISTANCE				40.0f
#define JETSKI_SPLASH_SPRAY					1
#define JETSKI_SPLASH_SPRAY_SIZE			2.0f
#define JETSKI_SPLASH_GURGLE				2
#define JETSKI_SPLASH_GURGLE_SIZE			8.0f
#define JETSKI_SPLASH_RIPPLE				3
#define JETSKI_SPLASH_RIPPLE_SIZE			10.0f

#define JETSKI_DRAG_NO_THRUST_IN_WATER		10.0f
#define JETSKI_DRAG_NO_THRUST_ON_WATER		30.0f
#define JETSKI_DRAG_LEAN_ADD				10.0f
#define JETSKI_DRAG_IN_REVERSE				10.0f
#define JETSKI_DRAG_IN_THRUST				5.0f

#define JETSKI_STEERING_EPS					0.01f
#define JETSKI_STEERING_IN_PLACE			90.0f
#define JETSKI_STEERING_NORMAL				45.0f
#define JETSKI_STEERING_LEAN				60.0f

#define JETSKI_ROTATE_IN_PLACE_RATIO		0.35f

class CPropJetski : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropJetski, CPropVehicleDriveable );
	DECLARE_DATADESC();

public:

	// CPropVehicle
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );

	// CBaseEntity
	void			Think(void);
	void			Precache( void );
	void			Spawn( void );

	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

private:

	void			OnTurn( CUserCmd *ucmd );
	void			OnSpeed( CUserCmd *ucmd );
	void			UpdateTurnAndSpeed( void );
	
	void			CreateSplash( int nSplashType );

	bool			UpdateLean( CUserCmd *ucmd );
	float			CalculateFriction( CUserCmd *ucmd );
	float			CalculateDrag( CUserCmd *ucmd );

private:

	float						m_flSpringLengthApproach[4];	//
	float						m_flFrictionWheels[4];

	float						m_flHandbrakeTime;				// handbrake after the fact to keep vehicles from rolling
	bool						m_bInitialHandbrake;

	float						m_springLen[4];
	IPhysicsVehicleController  *m_pVehicle;
};

LINK_ENTITY_TO_CLASS( prop_vehicle_jetski, CPropJetski );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CPropJetski )

	DEFINE_AUTO_ARRAY( m_flSpringLengthApproach, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( m_flFrictionWheels, FIELD_FLOAT ),
	DEFINE_FIELD( m_flHandbrakeTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInitialHandbrake, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_springLen, FIELD_FLOAT ),
	DEFINE_PHYSPTR( m_pVehicle ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::Spawn( void )
{
	// Setup vehicle as a ray-cast jetski.
	SetVehicleType( VEHICLE_TYPE_JETSKI_RAYCAST );

	BaseClass::Spawn();

	// Handbrake data.
	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;
	m_VehiclePhysics.SetHasBrakePedal( false );

	// Slow reverse.
	m_VehiclePhysics.SetMaxReverseThrottle( -0.3f );

	// Setup vehicle variables.
	m_pVehicle = m_VehiclePhysics.GetVehicle();
	for ( int i = 0; i < 4; i++ )
	{
		m_springLen[i] = JETSKI_SHOCK_LENGTH_REST;
	}

	m_takedamage = DAMAGE_EVENTS_ONLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( ptr->hitbox == VEHICLE_HITBOX_DRIVER )
	{
		if ( m_hPlayer != NULL )
		{
			m_hPlayer->TakeDamage( info );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropJetski::OnTakeDamage( const CTakeDamageInfo &info )
{
	//Do scaled up physic damage to the car
	CTakeDamageInfo physDmg = info;
	physDmg.ScaleDamage( 25 );
	VPhysicsTakeDamage( physDmg );

	//Check to do damage to driver
	if ( m_hPlayer != NULL )
	{
		//Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		//Take the damage
		m_hPlayer->TakeDamage( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::Think(void)
{
	BaseClass::Think();
	
	// set handbrake after physics sim settles down
	if ( gpGlobals->curtime < m_flHandbrakeTime )
	{
		SetNextThink( gpGlobals->curtime );
	}
	else if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
	{
		m_bInitialHandbrake = true;
		m_VehiclePhysics.SetHandbrake( true );
		m_VehiclePhysics.Think();
	}


	// play enter animation
	if ( (m_bEnterAnimOn || m_bExitAnimOn) && !IsSequenceFinished() )
	{
		StudioFrameAdvance();
	}
	else if ( IsSequenceFinished() )
	{
		if ( m_bExitAnimOn )
		{
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				pPlayer->LeaveVehicle();		// now that sequence is finished, leave car
				Vector vecEyes;
				QAngle vecEyeAng;
				GetAttachment( "vehicle_driver_eyes", vecEyes, vecEyeAng );
				vecEyeAng.x = 0;
				vecEyeAng.z = 0;
				pPlayer->SnapEyeAngles( vecEyeAng );			
			}
			m_bExitAnimOn = false;
		}
		int iSequence = SelectWeightedSequence( ACT_IDLE );
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			m_flCycle = 0;
			m_flAnimTime = gpGlobals->curtime;
			ResetSequence( iSequence );
			ResetClientsideFrame();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropJetski::OnTurn( CUserCmd *ucmd )
{
#if 0
	// Check for lean and adjust the turning radius accordingly.
	if ( ucmd->buttons & IN_JUMP )
	{
		m_VehiclePhysics.SetSteeringDegrees( JETSKI_STEERING_LEAN );
	}
	else
	{
		m_VehiclePhysics.SetSteeringDegrees( JETSKI_STEERING_NORMAL );
	}

#endif
	float flSteering = m_VehiclePhysics.GetSteering();
	bool bLeft = ( flSteering < -JETSKI_STEERING_EPS );
	bool bRight = ( flSteering > JETSKI_STEERING_EPS );

	float flAbsSteering = fabsf( flSteering );
	float flInvAbsSteering = 1.0f - flAbsSteering;

	// Get the speed and ratio to max speed.
	float flSpeed = m_VehiclePhysics.GetSpeed();
	float flMaxSpeed = m_VehiclePhysics.GetMaxSpeed();
	float flRatio = flSpeed / flMaxSpeed;
	float flScale = 1.0f - flRatio;
	flScale *= 0.95f;
	flScale += 0.05f;

	flAbsSteering *= flScale;
	flInvAbsSteering *= flScale;

	m_flSpringLengthApproach[0] = JETSKI_SHOCK_LENGTH_SHORT;			// Front-Left
	m_flSpringLengthApproach[1] = JETSKI_SHOCK_LENGTH_SHORT;			// Front-Right
	m_flSpringLengthApproach[2] = JETSKI_SHOCK_LENGTH_SHORT;			// Back-Left
	m_flSpringLengthApproach[3] = JETSKI_SHOCK_LENGTH_SHORT;			// Back-Right

	return;

	// Roll right.
	if( bRight )	 
	{
		float flLengthRight = JETSKI_SHOCK_LENGTH_SHORT + ( JETSKI_SHOCK_LENGTH_REST - JETSKI_SHOCK_LENGTH_SHORT ) * flInvAbsSteering;
		float flLengthLeft = JETSKI_SHOCK_LENGTH_REST + ( JETSKI_SHOCK_LENGTH_LONG - JETSKI_SHOCK_LENGTH_REST ) * flAbsSteering;

		m_flSpringLengthApproach[0] = flLengthLeft;			// Front-Left
		m_flSpringLengthApproach[1] = flLengthRight;		// Front-Right
		m_flSpringLengthApproach[2] = flLengthLeft;			// Back-Left
		m_flSpringLengthApproach[3] = flLengthRight;		// Back-Right
	}
	// Roll left.
	else if ( bLeft )
	{
		float flLengthRight = JETSKI_SHOCK_LENGTH_REST + ( JETSKI_SHOCK_LENGTH_LONG - JETSKI_SHOCK_LENGTH_REST ) * flAbsSteering;
		float flLengthLeft = JETSKI_SHOCK_LENGTH_SHORT + ( JETSKI_SHOCK_LENGTH_REST - JETSKI_SHOCK_LENGTH_SHORT ) * flInvAbsSteering;

		m_flSpringLengthApproach[0] = flLengthLeft;			// Front-Left
		m_flSpringLengthApproach[1] = flLengthRight;		// Front-Right
		m_flSpringLengthApproach[2] = flLengthLeft;			// Back-Left
		m_flSpringLengthApproach[3] = flLengthRight;		// Back-Right
	}
	// Return springs to their normal height
	else		
	{
		m_flSpringLengthApproach[0] = JETSKI_SHOCK_LENGTH_REST;			// Front-Left
		m_flSpringLengthApproach[1] = JETSKI_SHOCK_LENGTH_REST;			// Front-Right
		m_flSpringLengthApproach[2] = JETSKI_SHOCK_LENGTH_REST;			// Back-Left
		m_flSpringLengthApproach[3] = JETSKI_SHOCK_LENGTH_REST;			// Back-Right
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CPropJetski::UpdateLean( CUserCmd *ucmd )
{
	// Are we leaning back?
	if ( ucmd->buttons & IN_JUMP )
	{
		m_pVehicle->SetLeanBack( true );
		return true;
	}

	m_pVehicle->SetLeanBack( false );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CPropJetski::CalculateFriction( CUserCmd *ucmd )
{
	// Get the speed and ratio to max speed.
	float flSpeed = m_VehiclePhysics.GetSpeed();
	float flMaxSpeed = m_VehiclePhysics.GetMaxSpeed();
	float flRatio = flSpeed / flMaxSpeed;

	float flFriction = JETSKI_FRICTION_MAX;

	flRatio = 1.0f - ( float )pow( flRatio, 4 );
	flFriction = JETSKI_FRICTION_MIN + ( JETSKI_FRICTION_MAX - JETSKI_FRICTION_MIN ) * flRatio;

	flFriction = clamp( flFriction, JETSKI_FRICTION_MIN, JETSKI_FRICTION_MAX );

	// Debug!
	Msg( "Speed = %f, Friction = %f", flSpeed, flFriction );

	return flFriction;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CPropJetski::CalculateDrag( CUserCmd *ucmd )
{
	// Get the speed and ratio to max speed.
	float flSpeed = m_VehiclePhysics.GetSpeed();
	float flMaxSpeed = m_VehiclePhysics.GetMaxSpeed();
	float flRatio = flSpeed / flMaxSpeed;

	float flDrag = 0.0f;

	bool bLean = UpdateLean( ucmd );
	
#if 0
	if ( bLean )
	{
		flDrag += JETSKI_DRAG_LEAN_ADD;
	}
		float flNormalizedRatio = ( flRatio - 0.4f ) * 1.667f;
		float flSplineRatio = SimpleSpline( flNormalizedRatio );

		flFriction = JETSKI_FRICTION_MAX + ( JETSKI_FRICTION_MIN - JETSKI_FRICTION_MAX ) * flSplineRatio;
		flDrag = JETSKI_DRAG_IN_WATER + ( JETSKI_DRAG_ON_WATER - JETSKI_DRAG_IN_WATER ) * flNormalizedRatio;

		// Leaning backwards.
		if ( bLean )
		{
			flDrag += JETSKI_DRAG_LEAN_ADD;
		}
	}

#define JETSKI_DRAG_NO_THRUST_IN_WATER		10.0f
#define JETSKI_DRAG_NO_THRUST_ON_WATER		30.0f
#define JETSKI_DRAG_LEAN_ADD				10.0f
#define JETSKI_DRAG_IN_REVERSE				10.0f
#define JETSKI_DRAG_IN_THRUST				5.0f
#endif

	// Debug
	Msg( "Drag = %f\n", flDrag );

	return flDrag;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropJetski::OnSpeed( CUserCmd *ucmd )
{
	// Get the physics object so we can adjust the drag.
	IPhysicsObject *pPhysJetski = VPhysicsGetObject();
	if ( !pPhysJetski )
		return;

	float flFriction = CalculateFriction( ucmd );
	float flDrag = CalculateDrag( ucmd );

	// Update the friction of the jetski "fake wheels."
	for ( int iWheel = 0; iWheel < 4; ++iWheel )
	{
		m_flFrictionWheels[iWheel] = flFriction;
	}

	// Update the Damp coefficient on the jetski.
	float flZero = 0.0f;
	pPhysJetski->SetDragCoefficient( &flDrag, &flZero );

#if 0
	// Splash effects.
	if ( flRatio > 0.1f )
	{
		CreateSplash( JETSKI_SPLASH_RIPPLE );
	}

	float flRandom = random->RandomFloat( 0.0f, 1.0f );
	if ( flRatio > 0.8f && flRandom < 0.15f )
	{
		CreateSplash( JETSKI_SPLASH_SPRAY );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::UpdateTurnAndSpeed( void )
{
#if 1
	// Update springs.
	m_springLen[0] = UTIL_Approach( m_flSpringLengthApproach[0], m_springLen[0], JETSKI_PITCH_AND_ROLL_RATE );
	m_pVehicle->SetSpringLength( 0, m_springLen[0] );
	m_springLen[1] = UTIL_Approach( m_flSpringLengthApproach[1], m_springLen[1], JETSKI_PITCH_AND_ROLL_RATE );
	m_pVehicle->SetSpringLength( 1, m_springLen[1] );
	m_springLen[2] = UTIL_Approach( m_flSpringLengthApproach[2], m_springLen[2], JETSKI_PITCH_AND_ROLL_RATE );
	m_pVehicle->SetSpringLength( 2, m_springLen[2] );
	m_springLen[3] = UTIL_Approach( m_flSpringLengthApproach[3], m_springLen[3], JETSKI_PITCH_AND_ROLL_RATE );
	m_pVehicle->SetSpringLength( 3, m_springLen[3] );
#endif

	// Update wheel frictions.
	for ( int iWheel = 0; iWheel < 4; ++iWheel )
	{
		m_pVehicle->SetWheelFriction( iWheel, m_flFrictionWheels[iWheel] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd )
{
	//Lose control when the player dies
	if ( pPlayer->IsAlive() == false )
		return;

	OnTurn( ucmd );
	OnSpeed( ucmd );
	UpdateTurnAndSpeed();

	m_VehiclePhysics.UpdateDriverControls( ucmd, ucmd->frametime );

	// Save this data.
	m_nSpeed = m_VehiclePhysics.GetSpeed();
	m_nRPM = m_VehiclePhysics.GetRPM();	
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropJetski::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	DriveVehicle( player, ucmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJetski::CreateSplash( int nSplashType )
{
	Vector vecSplashPoint;
	QAngle vecSplashAngles;
	GetAttachment( "splash_pt", vecSplashPoint, vecSplashAngles );

	Vector vecForward, vecUp;
	AngleVectors( vecSplashAngles, &vecForward, &vecUp, NULL );

	CEffectData	data;
	data.m_vOrigin = vecSplashPoint;
	
	switch ( nSplashType )
	{
	case JETSKI_SPLASH_SPRAY:
		{
			Vector vecSplashDir;
			vecSplashDir = ( vecForward + vecUp ) * 0.5f;
			VectorNormalize( vecSplashDir );
			data.m_vNormal = vecSplashDir;
			data.m_flScale = JETSKI_SPLASH_SPRAY_SIZE + random->RandomFloat( 0, JETSKI_SPLASH_SPRAY_SIZE * 0.25 );
			DispatchEffect( "waterripple", data );
			DispatchEffect( "watersplash", data );
		}
	case JETSKI_SPLASH_GURGLE:
		{
			Vector vecSplashDir;
			vecSplashDir = vecUp;
			data.m_vNormal = vecSplashDir;

			data.m_flScale = JETSKI_SPLASH_GURGLE_SIZE + random->RandomFloat( 0, JETSKI_SPLASH_GURGLE_SIZE * 0.25 );
			DispatchEffect( "waterripple", data );
			DispatchEffect( "watersplash", data );
		}
	case JETSKI_SPLASH_RIPPLE:
		{
			Vector vecSplashDir;
			vecSplashDir = vecUp;
			data.m_vNormal = vecSplashDir;
			data.m_flScale = JETSKI_SPLASH_RIPPLE_SIZE + random->RandomFloat( 0, JETSKI_SPLASH_RIPPLE_SIZE * 0.25 );
			DispatchEffect( "waterripple", data );
		}
	default:
		{
			return;
		}
	}
}