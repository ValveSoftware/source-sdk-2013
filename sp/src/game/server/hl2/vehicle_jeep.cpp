//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "IEffects.h"
#include "beam_shared.h"
#include "weapon_gauss.h"
#include "soundenvelope.h"
#include "decals.h"
#include "soundent.h"
#include "grenade_ar2.h"
#include "te_effect_dispatch.h"
#include "hl2_player.h"
#include "ndebugoverlay.h"
#include "movevars_shared.h"
#include "bone_setup.h"
#include "ai_basenpc.h"
#include "ai_hint.h"
#include "npc_crow.h"
#include "globalstate.h"
#include "vehicle_jeep.h"
#include "eventqueue.h"
#include "rumble_shared.h"
// NVNT haptic utils
#include "haptics/haptic_utils.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	VEHICLE_HITBOX_DRIVER		1
#define LOCK_SPEED					10
#define JEEP_GUN_YAW				"vehicle_weapon_yaw"
#define JEEP_GUN_PITCH				"vehicle_weapon_pitch"
#define JEEP_GUN_SPIN				"gun_spin"
#define	JEEP_GUN_SPIN_RATE			20

#define CANNON_MAX_UP_PITCH			20
#define CANNON_MAX_DOWN_PITCH		20
#define CANNON_MAX_LEFT_YAW			90
#define CANNON_MAX_RIGHT_YAW		90

#define OVERTURNED_EXIT_WAITTIME	2.0f

#define JEEP_AMMOCRATE_HITGROUP		5

#define JEEP_STEERING_SLOW_ANGLE	50.0f
#define JEEP_STEERING_FAST_ANGLE	15.0f

#define	JEEP_AMMO_CRATE_CLOSE_DELAY	2.0f

#define JEEP_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define JEEP_FRAMETIME_MIN		1e-6

// Seagull perching
const char *g_pJeepThinkContext = "JeepSeagullThink";
#define	JEEP_SEAGULL_THINK_INTERVAL		10.0		// Interval between checks for seagull perches
#define	JEEP_SEAGULL_POOP_INTERVAL		45.0		// Interval between checks for seagull poopage
#define JEEP_SEAGULL_HIDDEN_TIME		15.0		// Time for which the player must be hidden from the jeep for a seagull to perch
#define JEEP_SEAGULL_MAX_TIME			60.0		// Time at which a seagull will definately perch on the jeep

ConVar	sk_jeep_gauss_damage( "sk_jeep_gauss_damage", "15" );
ConVar	hud_jeephint_numentries( "hud_jeephint_numentries", "10", FCVAR_NONE );
ConVar	g_jeepexitspeed( "g_jeepexitspeed", "100", FCVAR_CHEAT );

extern ConVar autoaim_max_dist;
extern ConVar sv_vehicle_autoaim_scale;


//=============================================================================
//
// Jeep water data.
//

BEGIN_SIMPLE_DATADESC( JeepWaterData_t )
	DEFINE_ARRAY( m_bWheelInWater,			FIELD_BOOLEAN,	JEEP_WHEEL_COUNT ),
	DEFINE_ARRAY( m_bWheelWasInWater,			FIELD_BOOLEAN,	JEEP_WHEEL_COUNT ),
	DEFINE_ARRAY( m_vecWheelContactPoints,	FIELD_VECTOR,	JEEP_WHEEL_COUNT ),
	DEFINE_ARRAY( m_flNextRippleTime,			FIELD_TIME,		JEEP_WHEEL_COUNT ),
	DEFINE_FIELD( m_bBodyInWater,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBodyWasInWater,			FIELD_BOOLEAN ),
END_DATADESC()	

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle with weaponry
//-----------------------------------------------------------------------------
class CJeepFourWheelServerVehicle : public CFourWheelServerVehicle
{
	typedef CFourWheelServerVehicle BaseClass;
// IServerVehicle
public:
	bool		NPC_HasPrimaryWeapon( void ) { return true; }
	void		NPC_AimPrimaryWeapon( Vector vecTarget );
	int			GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked );
};

BEGIN_DATADESC( CPropJeep )
	DEFINE_FIELD( m_bGunHasBeenCutOff, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDangerSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nBulletType, FIELD_INTEGER ),
	DEFINE_FIELD( m_bCannonCharging, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flCannonTime, FIELD_TIME ),
	DEFINE_FIELD( m_flCannonChargeStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecGunOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_SOUNDPATCH( m_sndCannonCharge ),
	DEFINE_FIELD( m_nSpinPos, FIELD_INTEGER ),
	DEFINE_FIELD( m_aimYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_throttleDisableTime, FIELD_TIME ),
	DEFINE_FIELD( m_flHandbrakeTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInitialHandbrake, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flOverturnedTime, FIELD_TIME ),
	DEFINE_FIELD( m_flAmmoCrateCloseTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecLastEyePos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecLastEyeTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecEyeSpeed, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecTargetSpeed, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bHeadlightIsOn, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_WaterData ),

	DEFINE_FIELD( m_iNumberOfEntries, FIELD_INTEGER ),
	DEFINE_FIELD( m_nAmmoType, FIELD_INTEGER ),

	DEFINE_FIELD( m_flPlayerExitedTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastSawPlayerAt, FIELD_TIME ),
	DEFINE_FIELD( m_hLastPlayerInVehicle, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hSeagull, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bHasPoop, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ShowHudHint", InputShowHudHint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartRemoveTauCannon", InputStartRemoveTauCannon ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FinishRemoveTauCannon", InputFinishRemoveTauCannon ),

	DEFINE_THINKFUNC( JeepSeagullThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPropJeep, DT_PropJeep )
	SendPropBool( SENDINFO( m_bHeadlightIsOn ) ),
END_SEND_TABLE();

// This is overriden for the episodic jeep
#ifndef HL2_EPISODIC
LINK_ENTITY_TO_CLASS( prop_vehicle_jeep, CPropJeep );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropJeep::CPropJeep( void )
{
	m_bHasGun = true;
	m_bGunHasBeenCutOff = false;
	m_bCannonCharging = false;
	m_flCannonChargeStartTime = 0;
	m_flCannonTime = 0;
	m_nBulletType = -1;
	m_flOverturnedTime = 0.0f;
	m_iNumberOfEntries = 0;

	m_vecEyeSpeed.Init();

	InitWaterData();

	m_bUnableToFire = true;
	m_flAmmoCrateCloseTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	m_pServerVehicle = new CJeepFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::Precache( void )
{
	UTIL_PrecacheOther( "npc_seagull" );

	PrecacheScriptSound( "PropJeep.AmmoClose" );
	PrecacheScriptSound( "PropJeep.FireCannon" );
	PrecacheScriptSound( "PropJeep.FireChargedCannon" );
	PrecacheScriptSound( "PropJeep.AmmoOpen" );

	PrecacheScriptSound( "Jeep.GaussCharge" );

	PrecacheModel( GAUSS_BEAM_SPRITE );

	BaseClass::Precache();
}

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropJeep::Spawn( void )
{
	// Setup vehicle as a real-wheels car.
	SetVehicleType( VEHICLE_TYPE_CAR_WHEELS );

	BaseClass::Spawn();
	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;

	m_flMinimumSpeedToEnterExit = LOCK_SPEED;

	m_nBulletType = GetAmmoDef()->Index("GaussEnergy");

	CAmmoDef *pAmmoDef = GetAmmoDef();
	m_nAmmoType = pAmmoDef->Index("GaussEnergy");

	if ( m_bHasGun )
	{
		SetBodygroup( 1, true );

		// Initialize pose parameters
		SetPoseParameter( JEEP_GUN_YAW, 0 );
		SetPoseParameter( JEEP_GUN_PITCH, 0 );
		m_nSpinPos = 0;
		SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );
		m_aimYaw = 0;
		m_aimPitch = 0;
	}
	else
	{
		SetBodygroup( 1, false );
	}

	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CPropJeep::Activate()
{
	BaseClass::Activate();

	CBaseServerVehicle *pServerVehicle = dynamic_cast<CBaseServerVehicle *>(GetServerVehicle());
	if ( pServerVehicle )
	{
		if( pServerVehicle->GetPassenger() )
		{
			// If a jeep comes back from a save game with a driver, make sure the engine rumble starts up.
			pServerVehicle->StartEngineRumble();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
void CPropJeep::DoImpactEffect( trace_t &tr, int nDamageType )
{
	//Draw our beam
	DrawBeam( tr.startpos, tr.endpos, 2.4 );

	if ( (tr.surface.flags & SURF_SKY) == false )
	{
		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

		UTIL_ImpactTrace( &tr, m_nBulletType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;
	if ( ptr->hitbox != VEHICLE_HITBOX_DRIVER )
	{
		if ( inputInfo.GetDamageType() & DMG_BULLET )
		{
			info.ScaleDamage( 0.0001 );
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: Modifies the passenger's damage taken through us
//-----------------------------------------------------------------------------
float CPropJeep::PassengerDamageModifier( const CTakeDamageInfo &info )
{
	if ( info.GetInflictor() && FClassnameIs( info.GetInflictor(), "hunter_flechette" ) )
		return 0.1f;

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropJeep::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	//Do scaled up physics damage to the car
	CTakeDamageInfo info = inputInfo;
	info.ScaleDamage( 25 );
	
	// HACKHACK: Scale up grenades until we get a better explosion/pressure damage system
	if ( inputInfo.GetDamageType() & DMG_BLAST )
	{
		info.SetDamageForce( inputInfo.GetDamageForce() * 10 );
	}
	
	VPhysicsTakeDamage( info );

	// reset the damage
	info.SetDamage( inputInfo.GetDamage() );

	// small amounts of shock damage disrupt the car, but aren't transferred to the player
	if ( info.GetDamageType() == DMG_SHOCK )
	{
		if ( info.GetDamage() <= 10 )
		{
			// take 10% damage and make the engine stall
			info.ScaleDamage( 0.1 );
			m_throttleDisableTime = gpGlobals->curtime + 2;
		}
	}

	//Check to do damage to driver
	if ( GetDriver() )
	{
		// Never take crush damage
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		// Scale the damage and mark that we're passing it in so the base player accepts the damage
		info.ScaleDamage( PassengerDamageModifier( info ) );
		info.SetDamageType( info.GetDamageType() | DMG_VEHICLE );
		
		// Deal the damage to the passenger
		GetDriver()->TakeDamage( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CPropJeep::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	GetAttachment( eyeAttachmentIndex, matrix );
	MatrixGetColumn( matrix, 3, shotPos );

	if ( bNoisy )
	{
		shotPos[0] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[1] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[2] += random->RandomFloat( -8.0f, 8.0f );
	}

	return shotPos;
}

//-----------------------------------------------------------------------------
// Purpose: Aim Gun at a target
//-----------------------------------------------------------------------------
void CPropJeep::AimGunAt( Vector *endPos, float flInterval )
{
	Vector	aimPos = *endPos;

	// See if the gun should be allowed to aim
	if ( IsOverturned() || m_bEngineLocked || m_bHasGun == false )
	{
		SetPoseParameter( JEEP_GUN_YAW, 0 );
		SetPoseParameter( JEEP_GUN_PITCH, 0 );
		SetPoseParameter( JEEP_GUN_SPIN, 0 );
		return;

		// Make the gun go limp and look "down"
		Vector	v_forward, v_up;
		AngleVectors( GetLocalAngles(), NULL, &v_forward, &v_up );
		aimPos = WorldSpaceCenter() + ( v_forward * -32.0f ) - Vector( 0, 0, 128.0f );
	}

	matrix3x4_t gunMatrix;
	GetAttachment( LookupAttachment("gun_ref"), gunMatrix );

	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform( aimPos, gunMatrix, localEnemyPosition );

	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	// Constrain our angles
	float newTargetYaw	= clamp( targetYaw, -CANNON_MAX_LEFT_YAW, CANNON_MAX_RIGHT_YAW );
	float newTargetPitch = clamp( targetPitch, -CANNON_MAX_DOWN_PITCH, CANNON_MAX_UP_PITCH );

	// If the angles have been clamped, we're looking outside of our valid range
	if ( fabs(newTargetYaw-targetYaw) > 1e-4 || fabs(newTargetPitch-targetPitch) > 1e-4 )
	{
		m_bUnableToFire = true;
	}

	targetYaw = newTargetYaw;
	targetPitch = newTargetPitch;

	// Exponentially approach the target
	float yawSpeed = 8;
	float pitchSpeed = 8;

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( JEEP_GUN_YAW, -m_aimYaw);
	SetPoseParameter( JEEP_GUN_PITCH, -m_aimPitch );

	InvalidateBoneCache();

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = -GetPoseParameter( JEEP_GUN_PITCH );
	m_aimYaw = -GetPoseParameter( JEEP_GUN_YAW );

	// Now draw crosshair for actual aiming point
	Vector	vecMuzzle, vecMuzzleDir;
	QAngle	vecMuzzleAng;

	GetAttachment( "Muzzle", vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );

	trace_t	tr;
	UTIL_TraceLine( vecMuzzle, vecMuzzle + (vecMuzzleDir * MAX_TRACE_LENGTH), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// see if we hit something, if so, adjust endPos to hit location
	if ( tr.fraction < 1.0 )
	{
		m_vecGunCrosshair = vecMuzzle + ( vecMuzzleDir * MAX_TRACE_LENGTH * tr.fraction );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::InitWaterData( void )
{
	m_WaterData.m_bBodyInWater = false;
	m_WaterData.m_bBodyWasInWater = false;

	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		m_WaterData.m_bWheelInWater[iWheel] = false;
		m_WaterData.m_bWheelWasInWater[iWheel] = false;
		m_WaterData.m_vecWheelContactPoints[iWheel].Init();
		m_WaterData.m_flNextRippleTime[iWheel] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::HandleWater( void )
{
	// Only check the wheels and engine in water if we have a driver (player).
	if ( !GetDriver() )
		return;

	// Check to see if we are in water.
	if ( CheckWater() )
	{
		for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
		{
			// Create an entry/exit splash!
			if ( m_WaterData.m_bWheelInWater[iWheel] != m_WaterData.m_bWheelWasInWater[iWheel] )
			{
				CreateSplash( m_WaterData.m_vecWheelContactPoints[iWheel] );
				CreateRipple( m_WaterData.m_vecWheelContactPoints[iWheel] );
			}
			
			// Create ripples.
			if ( m_WaterData.m_bWheelInWater[iWheel] && m_WaterData.m_bWheelWasInWater[iWheel] )
			{
				if ( m_WaterData.m_flNextRippleTime[iWheel] < gpGlobals->curtime )
				{
					// Stagger ripple times
					m_WaterData.m_flNextRippleTime[iWheel] = gpGlobals->curtime + RandomFloat( 0.1, 0.3 );
					CreateRipple( m_WaterData.m_vecWheelContactPoints[iWheel] );
				}
			}
		}
	}

	// Save of data from last think.
	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		m_WaterData.m_bWheelWasInWater[iWheel] = m_WaterData.m_bWheelInWater[iWheel];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropJeep::CheckWater( void )
{
	bool bInWater = false;

	// Check all four wheels.
	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		// Get the current wheel and get its contact point.
		IPhysicsObject *pWheel = m_VehiclePhysics.GetWheel( iWheel );
		if ( !pWheel )
			continue;

		// Check to see if we hit water.
		if ( pWheel->GetContactPoint( &m_WaterData.m_vecWheelContactPoints[iWheel], NULL ) )
		{
			m_WaterData.m_bWheelInWater[iWheel] = ( UTIL_PointContents( m_WaterData.m_vecWheelContactPoints[iWheel] ) & MASK_WATER ) ? true : false;
			if ( m_WaterData.m_bWheelInWater[iWheel] )
			{
				bInWater = true;
			}
		}
	}

	// Check the body and the BONNET.
	int iEngine = LookupAttachment( "vehicle_engine" );
	Vector vecEnginePoint;
	QAngle vecEngineAngles;
	GetAttachment( iEngine, vecEnginePoint, vecEngineAngles );

	m_WaterData.m_bBodyInWater = ( UTIL_PointContents( vecEnginePoint ) & MASK_WATER ) ? true : false;
	if ( m_WaterData.m_bBodyInWater )
	{
		if ( m_bHasPoop )
		{
			RemoveAllDecals();
			m_bHasPoop = false;
		}

		if ( !m_VehiclePhysics.IsEngineDisabled() )
		{
			m_VehiclePhysics.SetDisableEngine( true );
		}
	}
	else
	{
		if ( m_VehiclePhysics.IsEngineDisabled() )
		{
			m_VehiclePhysics.SetDisableEngine( false );
		}
	}

	if ( bInWater )
	{
		// Check the player's water level.
		CheckWaterLevel();
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CheckWaterLevel( void )
{
	CBaseEntity *pEntity = GetDriver();
	if ( pEntity && pEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pEntity );
		
		Vector vecAttachPoint;
		QAngle vecAttachAngles;
		
		// Check eyes. (vehicle_driver_eyes point)
		int iAttachment = LookupAttachment( "vehicle_driver_eyes" );
		GetAttachment( iAttachment, vecAttachPoint, vecAttachAngles );

		// Add the jeep's Z view offset
		Vector vecUp;
		AngleVectors( vecAttachAngles, NULL, NULL, &vecUp );
		vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
		vecAttachPoint.z += r_JeepViewZHeight.GetFloat() * vecUp.z;

		bool bEyes = ( UTIL_PointContents( vecAttachPoint ) & MASK_WATER ) ? true : false;
		if ( bEyes )
		{
			pPlayer->SetWaterLevel( WL_Eyes );
			return;
		}

		// Check waist.  (vehicle_engine point -- see parent function).
		if ( m_WaterData.m_bBodyInWater )
		{
			pPlayer->SetWaterLevel( WL_Waist );
			return;
		}

		// Check feet. (vehicle_feet_passenger0 point)
		iAttachment = LookupAttachment( "vehicle_feet_passenger0" );
		GetAttachment( iAttachment, vecAttachPoint, vecAttachAngles );
		bool bFeet = ( UTIL_PointContents( vecAttachPoint ) & MASK_WATER ) ? true : false;
		if ( bFeet )
		{
			pPlayer->SetWaterLevel( WL_Feet );
			return;
		}

		// Not in water.
		pPlayer->SetWaterLevel( WL_NotInWater );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateSplash( const Vector &vecPosition )
{
	// Splash data.
	CEffectData	data;
	data.m_fFlags = 0;
	data.m_vOrigin = vecPosition;
	data.m_vNormal.Init( 0.0f, 0.0f, 1.0f );
	VectorAngles( data.m_vNormal, data.m_vAngles );
	data.m_flScale = 10.0f + random->RandomFloat( 0, 2 );

	// Create the splash..
	DispatchEffect( "watersplash", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateRipple( const Vector &vecPosition )
{
	// Ripple data.
	CEffectData	data;
	data.m_fFlags = 0;
	data.m_vOrigin = vecPosition;
	data.m_vNormal.Init( 0.0f, 0.0f, 1.0f );
	VectorAngles( data.m_vNormal, data.m_vAngles );
	data.m_flScale = 10.0f + random->RandomFloat( 0, 2 );
	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	// Create the ripple.
	DispatchEffect( "waterripple", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::Think( void )
{
	BaseClass::Think();

	CBasePlayer	*pPlayer = UTIL_GetLocalPlayer();

	if ( m_bEngineLocked )
	{
		m_bUnableToFire = true;
		
		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}
	else if ( m_bHasGun )
	{
		// Start this as false and update it again each frame
		m_bUnableToFire = false;

		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}

	// Water!?
	HandleWater();

	SetSimulationTime( gpGlobals->curtime );
	
	SetNextThink( gpGlobals->curtime );
	SetAnimatedEveryTick( true );

    if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
	{
		m_bInitialHandbrake = true;
		m_VehiclePhysics.SetHandbrake( true );
		m_VehiclePhysics.Think();
	}

	// Check overturned status.
	if ( !IsOverturned() )
	{
		m_flOverturnedTime = 0.0f;
	}
	else
	{
		m_flOverturnedTime += gpGlobals->frametime;
	}

	// spin gun if charging cannon
	//FIXME: Don't bother for E3
	if ( m_bCannonCharging )
	{
		m_nSpinPos += JEEP_GUN_SPIN_RATE;
		SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );
	}

	// Aim gun based on the player view direction.
	if ( m_bHasGun && m_hPlayer && !m_bExitAnimOn && !m_bEnterAnimOn )
	{
		Vector vecEyeDir, vecEyePos;
		m_hPlayer->EyePositionAndVectors( &vecEyePos, &vecEyeDir, NULL, NULL );

		if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
		{
			autoaim_params_t params;

			params.m_fScale = AUTOAIM_SCALE_DEFAULT * sv_vehicle_autoaim_scale.GetFloat();
			params.m_fMaxDist = autoaim_max_dist.GetFloat();
			m_hPlayer->GetAutoaimVector( params );

			// Use autoaim as the eye dir if there is an autoaim ent.
			vecEyeDir = params.m_vecAutoAimDir;
		}

		// Trace out from the player's eye point.
		Vector	vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH );
		trace_t	trace;
		UTIL_TraceLine( vecEyePos, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );

		// See if we hit something, if so, adjust end position to hit location.
		if ( trace.fraction < 1.0 )
		{
   			vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH * trace.fraction );
		}

		//m_vecLookCrosshair = vecEndPos;
		AimGunAt( &vecEndPos, 0.1f );
	}

	StudioFrameAdvance();

	// If the enter or exit animation has finished, tell the server vehicle
	if ( IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn) )
	{
		if ( m_bEnterAnimOn )
		{
			m_VehiclePhysics.ReleaseHandbrake();
			StartEngine();

			// HACKHACK: This forces the jeep to play a sound when it gets entered underwater
			if ( m_VehiclePhysics.IsEngineDisabled() )
			{
				CBaseServerVehicle *pServerVehicle = dynamic_cast<CBaseServerVehicle *>(GetServerVehicle());
				if ( pServerVehicle )
				{
					pServerVehicle->SoundStartDisabled();
				}
			}

			// The first few time we get into the jeep, print the jeep help
			if ( m_iNumberOfEntries < hud_jeephint_numentries.GetInt() )
			{
				g_EventQueue.AddEvent( this, "ShowHudHint", 1.5f, this, this );
			}
		}
		
		if ( hl2_episodic.GetBool() )
		{
			// Set its running animation idle
			if ( m_bEnterAnimOn )
			{
				// Idle running
				int nSequence = SelectWeightedSequence( ACT_IDLE_STIMULATED );
				if ( nSequence > ACTIVITY_NOT_AVAILABLE )
				{
					SetCycle( 0 );
					m_flAnimTime = gpGlobals->curtime;
					ResetSequence( nSequence );
					ResetClientsideFrame();					
				}
			}
		}

		// If we're exiting and have had the tau cannon removed, we don't want to reset the animation
		if ( hl2_episodic.GetBool() )
		{
			// Reset on exit anim
			GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, m_bExitAnimOn );
		}
		else
		{
			GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, !(m_bExitAnimOn && TauCannonHasBeenCutOff()) );
		}
	}

	// See if the ammo crate needs to close
	if ( ( m_flAmmoCrateCloseTime < gpGlobals->curtime ) && ( GetSequence() == LookupSequence( "ammo_open" ) ) )
	{
		m_flAnimTime = gpGlobals->curtime;
		m_flPlaybackRate = 0.0;
		SetCycle( 0 );
		ResetSequence( LookupSequence( "ammo_close" ) );
	}
	else if ( ( GetSequence() == LookupSequence( "ammo_close" ) ) && IsSequenceFinished() )
	{
		m_flAnimTime = gpGlobals->curtime;
		m_flPlaybackRate = 0.0;
		SetCycle( 0 );
		
		int nSequence = SelectWeightedSequence( ACT_IDLE );
		ResetSequence( nSequence );

		CPASAttenuationFilter sndFilter( this, "PropJeep.AmmoClose" );
		EmitSound( sndFilter, entindex(), "PropJeep.AmmoClose" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &startPos - 
//			&endPos - 
//			width - 
//			useMuzzle - 
//-----------------------------------------------------------------------------
void CPropJeep::DrawBeam( const Vector &startPos, const Vector &endPos, float width )
{
	//Tracer down the middle
	UTIL_Tracer( startPos, endPos, 0, TRACER_DONT_USE_ATTACHMENT, 6500, false, "GaussTracer" );

	//Draw the main beam shaft
	CBeam *pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, 0.5 );
	
	pBeam->SetStartPos( startPos );
	pBeam->PointEntInit( endPos, this );
	pBeam->SetEndAttachment( LookupAttachment("Muzzle") );
	pBeam->SetWidth( width );
	pBeam->SetEndWidth( 0.05f );
	pBeam->SetBrightness( 255 );
	pBeam->SetColor( 255, 185+random->RandomInt( -16, 16 ), 40 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );

	//Draw electric bolts along shaft
	pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, 3.0f );
	
	pBeam->SetStartPos( startPos );
	pBeam->PointEntInit( endPos, this );
	pBeam->SetEndAttachment( LookupAttachment("Muzzle") );

	pBeam->SetBrightness( random->RandomInt( 64, 255 ) );
	pBeam->SetColor( 255, 255, 150+random->RandomInt( 0, 64 ) );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );
	pBeam->SetNoise( 1.6f );
	pBeam->SetEndWidth( 0.1f );
}

// NVNT Convar for jeep cannon magnitude
ConVar hap_jeep_cannon_mag("hap_jeep_cannon_mag", "10", 0);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::FireCannon( void )
{
	//Don't fire again if it's been too soon
	if ( m_flCannonTime > gpGlobals->curtime )
		return;

	if ( m_bUnableToFire )
		return;

	m_flCannonTime = gpGlobals->curtime + 0.2f;
	m_bCannonCharging = false;

	//Find the direction the gun is pointing in
	Vector aimDir;
	GetCannonAim( &aimDir );

#if defined( WIN32 ) && !defined( _X360 ) 
	// NVNT apply a punch on fire
	HapticPunch(m_hPlayer,0,0,hap_jeep_cannon_mag.GetFloat());
#endif
	FireBulletsInfo_t info( 1, m_vecGunOrigin, aimDir, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_nAmmoType );

	info.m_nFlags = FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS;
	info.m_pAttacker = m_hPlayer;

	FireBullets( info );

	// Register a muzzleflash for the AI
	if ( m_hPlayer )
	{
		m_hPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
		m_hPlayer->RumbleEffect( RUMBLE_PISTOL, 0, RUMBLE_FLAG_RESTART	);
	}

	CPASAttenuationFilter sndFilter( this, "PropJeep.FireCannon" );
	EmitSound( sndFilter, entindex(), "PropJeep.FireCannon" );
	
	// make cylinders of gun spin a bit
	m_nSpinPos += JEEP_GUN_SPIN_RATE;
	//SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );	//FIXME: Don't bother with this for E3, won't look right
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::FireChargedCannon( void )
{
	bool penetrated = false;

	m_bCannonCharging	= false;
	m_flCannonTime		= gpGlobals->curtime + 0.5f;

	StopChargeSound();

	CPASAttenuationFilter sndFilter( this, "PropJeep.FireChargedCannon" );
	EmitSound( sndFilter, entindex(), "PropJeep.FireChargedCannon" );

	if( m_hPlayer )
	{
		m_hPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );
	}

	//Find the direction the gun is pointing in
	Vector aimDir;
	GetCannonAim( &aimDir );

	Vector endPos = m_vecGunOrigin + ( aimDir * MAX_TRACE_LENGTH );
	
	//Shoot a shot straight out
	trace_t	tr;
	UTIL_TraceLine( m_vecGunOrigin, endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	
	ClearMultiDamage();

	//Find how much damage to do
	float flChargeAmount = ( gpGlobals->curtime - m_flCannonChargeStartTime ) / MAX_GAUSS_CHARGE_TIME;

	//Clamp this
	if ( flChargeAmount > 1.0f )
	{
		flChargeAmount = 1.0f;
	}

	//Determine the damage amount
	//FIXME: Use ConVars!
	float flDamage = 15 + ( ( 250 - 15 ) * flChargeAmount );

	CBaseEntity *pHit = tr.m_pEnt;
	
	//Look for wall penetration
	if ( tr.DidHitWorld() && !(tr.surface.flags & SURF_SKY) )
	{
		//Try wall penetration
		UTIL_ImpactTrace( &tr, m_nBulletType, "ImpactJeep" );
		UTIL_DecalTrace( &tr, "RedGlowFade" );

		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		
		Vector	testPos = tr.endpos + ( aimDir * 48.0f );

		UTIL_TraceLine( testPos, tr.endpos, MASK_SHOT, GetDriver(), COLLISION_GROUP_NONE, &tr );
			
		if ( tr.allsolid == false )
		{
			UTIL_DecalTrace( &tr, "RedGlowFade" );

			penetrated = true;
		}
	}
	else if ( pHit != NULL )
	{
		CTakeDamageInfo dmgInfo( this, GetDriver(), flDamage, DMG_SHOCK );
		CalculateBulletDamageForce( &dmgInfo, GetAmmoDef()->Index("GaussEnergy"), aimDir, tr.endpos, 1.0f + flChargeAmount * 4.0f );

		//Do direct damage to anything in our path
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}

	ApplyMultiDamage();

	//Kick up an effect
	if ( !(tr.surface.flags & SURF_SKY) )
	{
  		UTIL_ImpactTrace( &tr, m_nBulletType, "ImpactJeep" );

		//Do a gauss explosion
		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
	}

	//Show the effect
	DrawBeam( m_vecGunOrigin, tr.endpos, 9.6 );

	// Register a muzzleflash for the AI
	if ( m_hPlayer )
	{
		m_hPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5f );
	}

	//Rock the car
	IPhysicsObject *pObj = VPhysicsGetObject();

	if ( pObj != NULL )
	{
		Vector	shoveDir = aimDir * -( flDamage * 500.0f );

		pObj->ApplyForceOffset( shoveDir, m_vecGunOrigin );
	}

	//Do radius damage if we didn't penetrate the wall
	if ( penetrated == true )
	{
		RadiusDamage( CTakeDamageInfo( this, this, flDamage, DMG_SHOCK ), tr.endpos, 200.0f, CLASS_NONE, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::ChargeCannon( void )
{
	//Don't fire again if it's been too soon
	if ( m_flCannonTime > gpGlobals->curtime )
		return;

	//See if we're starting a charge
	if ( m_bCannonCharging == false )
	{
		m_flCannonChargeStartTime = gpGlobals->curtime;
		m_bCannonCharging = true;

		//Start charging sound
		CPASAttenuationFilter filter( this );
		m_sndCannonCharge = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "Jeep.GaussCharge", ATTN_NORM );

		if ( m_hPlayer )
		{
			m_hPlayer->RumbleEffect( RUMBLE_FLAT_LEFT, (int)(0.1 * 100), RUMBLE_FLAG_RESTART | RUMBLE_FLAG_LOOP | RUMBLE_FLAG_INITIAL_SCALE );
		}

		assert(m_sndCannonCharge!=NULL);
		if ( m_sndCannonCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCannonCharge, 1.0f, 50 );
			(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCannonCharge, 250, 3.0f );
		}

		return;
	}
	else
	{
		float flChargeAmount = ( gpGlobals->curtime - m_flCannonChargeStartTime ) / MAX_GAUSS_CHARGE_TIME;
		if ( flChargeAmount > 1.0f )
		{
			flChargeAmount = 1.0f;
		}

		float rumble = flChargeAmount * 0.5f;

		if( m_hPlayer )
		{
			m_hPlayer->RumbleEffect( RUMBLE_FLAT_LEFT, (int)(rumble * 100), RUMBLE_FLAG_UPDATE_SCALE );
		}
	}

	//TODO: Add muzzle effect?

	//TODO: Check for overcharge and have the weapon simply fire or instead "decharge"?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::StopChargeSound( void )
{
	if ( m_sndCannonCharge != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCannonCharge, 0.1f );
	}

	if( m_hPlayer )
	{
		m_hPlayer->RumbleEffect( RUMBLE_FLAT_LEFT, 0, RUMBLE_FLAG_STOP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds the true aiming position of the gun (looks at what player 
//			is looking at and adjusts)
// Input  : &resultDir - direction to be calculated
//-----------------------------------------------------------------------------
void CPropJeep::GetCannonAim( Vector *resultDir )
{
	Vector	muzzleOrigin;
	QAngle	muzzleAngles;

	GetAttachment( LookupAttachment("gun_ref"), muzzleOrigin, muzzleAngles );

	AngleVectors( muzzleAngles, resultDir );
}

//-----------------------------------------------------------------------------
// Purpose: If the player uses the jeep while at the back, he gets ammo from the crate instead
//-----------------------------------------------------------------------------
void CPropJeep::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	
	if ( pPlayer == NULL)
		return;

	// Find out if the player's looking at our ammocrate hitbox 
	Vector vecForward;
	pPlayer->EyeVectors( &vecForward, NULL, NULL );

	trace_t tr;
	Vector vecStart = pPlayer->EyePosition();
	UTIL_TraceLine( vecStart, vecStart + vecForward * 1024, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, pPlayer, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.m_pEnt == this && tr.hitgroup == JEEP_AMMOCRATE_HITGROUP )
	{
		// Player's using the crate.
		// Fill up his SMG ammo.
		pPlayer->GiveAmmo( 300, "SMG1");
		
		if ( ( GetSequence() != LookupSequence( "ammo_open" ) ) && ( GetSequence() != LookupSequence( "ammo_close" ) ) )
		{
			// Open the crate
			m_flAnimTime = gpGlobals->curtime;
			m_flPlaybackRate = 0.0;
			SetCycle( 0 );
			ResetSequence( LookupSequence( "ammo_open" ) );
			
			CPASAttenuationFilter sndFilter( this, "PropJeep.AmmoOpen" );
			EmitSound( sndFilter, entindex(), "PropJeep.AmmoOpen" );
		}

		m_flAmmoCrateCloseTime = gpGlobals->curtime + JEEP_AMMO_CRATE_CLOSE_DELAY;
		return;
	}

	// Fall back and get in the vehicle instead
	BaseClass::Use( pActivator, pCaller, useType, value );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropJeep::CanExitVehicle( CBaseEntity *pEntity )
{
	return ( !m_bEnterAnimOn && !m_bExitAnimOn && !m_bLocked && (m_nSpeed <= g_jeepexitspeed.GetFloat() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;
	if ( flFrameTime < JEEP_FRAMETIME_MIN )
	{
		vecVehicleEyePos = m_vecLastEyePos;
		DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, 0.0f );
		return;
	}

	// Keep static the sideways motion.

	// Dampen forward/backward motion.
	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );

	// Blend up/down motion.
	DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );
}

//-----------------------------------------------------------------------------
// Use the controller as follows:
// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
//-----------------------------------------------------------------------------
void CPropJeep::ComputePDControllerCoefficients( float *pCoefficientsOut,
												  float flFrequency, float flDampening,
												  float flDeltaTime )
{
	float flKs = 9.0f * flFrequency * flFrequency;
	float flKd = 4.5f * flFrequency * flDampening;

	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );

	pCoefficientsOut[0] = flKs * flScale;
	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropJeep::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get forward vector.
	Vector vecForward;
	AngleVectors( vecVehicleEyeAngles, &vecForward);

	// Simulate the eye position forward based on the data from last frame
	// (assumes no acceleration - it will get that from the "spring").
	Vector vecCurrentEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;

	// Calculate target speed based on the current vehicle eye position and the last vehicle eye position and frametime.
	Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
	m_vecLastEyeTarget = vecVehicleEyePos;	

	// Calculate the speed and position deltas.
	Vector vecDeltaSpeed = vecVehicleEyeSpeed - m_vecEyeSpeed;
	Vector vecDeltaPos = vecVehicleEyePos - vecCurrentEyePos;

	// Clamp.
	if ( vecDeltaPos.Length() > JEEP_DELTA_LENGTH_MAX )
	{
		float flSign = vecForward.Dot( vecVehicleEyeSpeed ) >= 0.0f ? -1.0f : 1.0f;
		vecVehicleEyePos += flSign * ( vecForward * JEEP_DELTA_LENGTH_MAX );
		m_vecLastEyePos = vecVehicleEyePos;
		m_vecEyeSpeed = vecVehicleEyeSpeed;
		return;
	}

	// Generate an updated (dampening) speed for use in next frames position extrapolation.
	float flCoefficients[2];
	ComputePDControllerCoefficients( flCoefficients, r_JeepViewDampenFreq.GetFloat(), r_JeepViewDampenDamp.GetFloat(), flFrameTime );
	m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );

	// Save off data for next frame.
	m_vecLastEyePos = vecCurrentEyePos;

	// Move eye forward/backward.
	Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
	vecVehicleEyePos -= vecForwardOffset;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropJeep::DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get up vector.
	Vector vecUp;
	AngleVectors( vecVehicleEyeAngles, NULL, NULL, &vecUp );
	vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
	vecVehicleEyePos.z += r_JeepViewZHeight.GetFloat() * vecUp.z;

	// NOTE: Should probably use some damped equation here.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If we are overturned and hit any key - leave the vehicle (IN_USE is already handled!).
	if ( m_flOverturnedTime > OVERTURNED_EXIT_WAITTIME )
	{
		if ( (ucmd->buttons & (IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT|IN_SPEED|IN_JUMP|IN_ATTACK|IN_ATTACK2) ) && !m_bExitAnimOn )
		{
			// Can't exit yet? We're probably still moving. Swallow the keys.
			if ( !CanExitVehicle(player) )
				return;

			if ( !GetServerVehicle()->HandlePassengerExit( m_hPlayer ) && ( m_hPlayer != NULL ) )
			{
				m_hPlayer->PlayUseDenySound();
			}
			return;
		}
	}

	// If the throttle is disabled or we're upside-down, don't allow throttling (including turbo)
	CUserCmd tmp;
	if ( ( m_throttleDisableTime > gpGlobals->curtime ) || ( IsOverturned() ) )
	{
		m_bUnableToFire = true;
		
		tmp = (*ucmd);
		tmp.buttons &= ~(IN_FORWARD|IN_BACK|IN_SPEED);
		ucmd = &tmp;
	}
	
	BaseClass::SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	int iButtons = ucmd->buttons;

	//Adrian: No headlights on Superfly.
/*	if ( ucmd->impulse == 100 )
	{
		if (HeadlightIsOn())
		{
			HeadlightTurnOff();
		}
        else 
		{
			HeadlightTurnOn();
		}
	}*/
		
	// Only handle the cannon if the vehicle has one
	if ( m_bHasGun )
	{
		// If we're holding down an attack button, update our state
		if ( IsOverturned() == false )
		{
			if ( iButtons & IN_ATTACK )
			{
				if ( m_bCannonCharging )
				{
					FireChargedCannon();
				}
				else
				{
					FireCannon();
				}
			}
			else if ( iButtons & IN_ATTACK2 )
			{
				ChargeCannon();
			}
		}

		// If we've released our secondary button, fire off our cannon
		if ( ( iButtonsReleased & IN_ATTACK2 ) && ( m_bCannonCharging ) )
		{
			FireChargedCannon();
		}
	}

	BaseClass::DriveVehicle( flFrameTime, ucmd, iButtonsDown, iButtonsReleased );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropJeep::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	// Create dangers sounds in front of the vehicle.
	CreateDangerSounds();
}

//-----------------------------------------------------------------------------
// Purpose: Create danger sounds in front of the vehicle.
//-----------------------------------------------------------------------------
void CPropJeep::CreateDangerSounds( void )
{
	QAngle dummy;
	GetAttachment( "Muzzle", m_vecGunOrigin, dummy );

	if ( m_flDangerSoundTime > gpGlobals->curtime )
		return;

	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir, vecRight;

	GetVectors( &vecDir, &vecRight, NULL );

	const float soundDuration = 0.25;
	float speed = m_VehiclePhysics.GetHLSpeed();

	// Make danger sounds ahead of the jeep
	if ( fabs(speed) > 120 )
	{
		Vector	vecSpot;

		float steering = m_VehiclePhysics.GetSteering();
		if ( steering != 0 )
		{
			if ( speed > 0 )
			{
				vecDir += vecRight * steering * 0.5;
			}
			else
			{
				vecDir -= vecRight * steering * 0.5;
			}
			VectorNormalize(vecDir);
		}
		const float radius = speed * 0.4;

		// 0.3 seconds ahead of the jeep
		vecSpot = vecStart + vecDir * (speed * 1.1f);
		CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_PLAYER_VEHICLE, vecSpot, radius, soundDuration, this, 0 );
		CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER | SOUND_CONTEXT_PLAYER_VEHICLE, vecSpot, radius, soundDuration, this, 1 );
		//NDebugOverlay::Box(vecSpot, Vector(-radius,-radius,-radius),Vector(radius,radius,radius), 255, 0, 255, 0, soundDuration);

#if 0
		trace_t	tr;
		// put sounds a bit to left and right but slightly closer to Jeep to make a "cone" of sound 
		// in front of it
		vecSpot = vecStart + vecDir * (speed * 0.75f) - vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 1 );

		vecSpot = vecStart + vecDir * (speed * 0.75f) + vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 2);
#endif
	}

	// Make engine sounds even when we're not going fast.
	CSoundEnt::InsertSound( SOUND_PLAYER | SOUND_CONTEXT_PLAYER_VEHICLE, GetAbsOrigin(), 800, soundDuration, this, 0 );

	m_flDangerSoundTime = gpGlobals->curtime + 0.1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( !pPlayer )
		return;

	CheckWater();
	BaseClass::EnterVehicle( pPassenger );

	// Start looking for seagulls to land
	m_hLastPlayerInVehicle = m_hPlayer;
	SetContextThink( NULL, 0, g_pJeepThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::ExitVehicle( int nRole )
{
	HeadlightTurnOff();

	BaseClass::ExitVehicle( nRole );

	//If the player has exited, stop charging
	StopChargeSound();
	m_bCannonCharging = false;

	// Remember when we last saw the player
	m_flPlayerExitedTime = gpGlobals->curtime;
	m_flLastSawPlayerAt = gpGlobals->curtime;

	if ( GlobalEntity_GetState( "no_seagulls_on_jeep" ) == GLOBAL_OFF )
	{
		// Look for fly nodes
		CHintCriteria hintCriteria;
		hintCriteria.SetHintType( HINT_CROW_FLYTO_POINT );
		hintCriteria.AddIncludePosition( GetAbsOrigin(), 4500 );
		CAI_Hint *pHint = CAI_HintManager::FindHint( GetAbsOrigin(), hintCriteria );
		if ( pHint )
		{
			// Start looking for seagulls to perch on me
			SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_THINK_INTERVAL, g_pJeepThinkContext );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if we should spawn a seagull on the jeep
//-----------------------------------------------------------------------------
void CPropJeep::JeepSeagullThink( void )
{
	if ( !m_hLastPlayerInVehicle )
		return;

	CBaseEntity *pBlocker;

	// Do we already have a seagull?
	if ( m_hSeagull )
	{
		CNPC_Seagull *pSeagull = dynamic_cast<CNPC_Seagull *>( m_hSeagull.Get() );

		if ( pSeagull )
		{
			// Is he still on us?
			if ( pSeagull->m_bOnJeep == true )
			{
				// Make the existing seagull spawn more poop over time
				if ( pSeagull->IsAlive() )
				{
					AddSeagullPoop( pSeagull->GetAbsOrigin() );
				}

				SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_POOP_INTERVAL, g_pJeepThinkContext );
			}
			else
			{
				// Our seagull's moved off us. 
				m_hSeagull = NULL;
				SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_THINK_INTERVAL, g_pJeepThinkContext );
			}
		}
		
		return;
	}

	// Only spawn seagulls if we're upright and out of water
	if ( m_WaterData.m_bBodyInWater || IsOverturned() )
	{
		SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_THINK_INTERVAL, g_pJeepThinkContext );
		return;
	}

	// Is the player visible?
	if ( FVisible( m_hLastPlayerInVehicle, MASK_SOLID_BRUSHONLY, &pBlocker ) )
	{
		m_flLastSawPlayerAt = gpGlobals->curtime;
		SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_THINK_INTERVAL, g_pJeepThinkContext );
		return;
	}

	// Start checking quickly
	SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + 0.2, g_pJeepThinkContext );

	// Not taken enough time yet?
	float flHiddenTime = (gpGlobals->curtime - m_flLastSawPlayerAt);
	if ( flHiddenTime < JEEP_SEAGULL_HIDDEN_TIME )
		return;

	// Random chance based upon the time it's taken
	float flChance = clamp( flHiddenTime / JEEP_SEAGULL_MAX_TIME, 0.0, 1.0 );
	if ( RandomFloat(0,1) < flChance )
	{
		SpawnPerchedSeagull();

		// Don't think for a while
		SetContextThink( &CPropJeep::JeepSeagullThink, gpGlobals->curtime + JEEP_SEAGULL_POOP_INTERVAL, g_pJeepThinkContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::SpawnPerchedSeagull( void )
{
	// Find a point on the car to sit
	Vector vecOrigin;
	QAngle vecAngles;
	int iAttachment = Studio_FindRandomAttachment( GetModelPtr(), "seagull_perch" );
	if ( iAttachment == -1 )
		return;

	// Spawn the seagull
	GetAttachment( iAttachment+1, vecOrigin, vecAngles );
	//vecOrigin.z += 16;

	CNPC_Seagull *pSeagull = (CNPC_Seagull*)CBaseEntity::Create("npc_seagull", vecOrigin, vecAngles, NULL );

	if ( !pSeagull )
		return;
	
	pSeagull->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	pSeagull->SetGroundEntity( this );
	pSeagull->AddFlag( FL_ONGROUND );
	pSeagull->SetOwnerEntity( this );
	pSeagull->SetMoveType( MOVETYPE_FLY );
	pSeagull->m_bOnJeep = true;
	
	m_hSeagull = pSeagull;

	AddSeagullPoop( vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//-----------------------------------------------------------------------------
void CPropJeep::AddSeagullPoop( const Vector &vecOrigin )
{
	// Drop some poop decals!
	int iDecals = RandomInt( 1,2 );
	for ( int i = 0; i < iDecals; i++ )
	{
		Vector vecPoop = vecOrigin;

		// get circular gaussian spread
		float x, y, z;
		do 
		{
			x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);
		vecPoop += Vector( x * 90, y * 90, 128 );
		
		trace_t tr;
		UTIL_TraceLine( vecPoop, vecPoop - Vector(0,0,512), MASK_SHOT, m_hSeagull, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace( &tr, "BirdPoop" );
	}

	m_bHasPoop = true;
}

//-----------------------------------------------------------------------------
// Purpose: Show people how to drive!
//-----------------------------------------------------------------------------
void CPropJeep::InputShowHudHint( inputdata_t &inputdata )
{
	CBaseServerVehicle *pServerVehicle = dynamic_cast<CBaseServerVehicle *>(GetServerVehicle());
	if ( pServerVehicle )
	{
		if( pServerVehicle->GetPassenger( VEHICLE_ROLE_DRIVER ) )
		{
			UTIL_HudHintText( m_hPlayer, "#Valve_Hint_JeepKeys" );
			m_iNumberOfEntries++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::InputStartRemoveTauCannon( inputdata_t &inputdata )
{
	// Start the gun removal animation
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle( 0 );
	ResetSequence( LookupSequence( "tau_levitate" ) );

	m_bGunHasBeenCutOff = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::InputFinishRemoveTauCannon( inputdata_t &inputdata )
{
	// Remove & hide the gun
	SetBodygroup( 1, false );
	m_bHasGun = false;
}

//========================================================================================================================================
// JEEP FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CJeepFourWheelServerVehicle::NPC_AimPrimaryWeapon( Vector vecTarget )
{
	((CPropJeep*)m_pVehicle)->AimGunAt( &vecTarget, 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecEyeExitEndpoint - 
// Output : int
//-----------------------------------------------------------------------------
int CJeepFourWheelServerVehicle::GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked )
{
	bAllPointsBlocked = false;

	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	// If we don't have the gun anymore, we want to get out using the "gun-less" animation
	if ( pAnimating && ((CPropJeep*)m_pVehicle)->TauCannonHasBeenCutOff() )
	{
		// HACK: We know the tau-cannon removed exit anim uses the first upright anim's exit details
		trace_t tr;

		// Convert our offset points to worldspace ones
		Vector vehicleExitOrigin = m_ExitAnimations[0].vecExitPointLocal;
		QAngle vehicleExitAngles = m_ExitAnimations[0].vecExitAnglesLocal;
		UTIL_ParentToWorldSpace( pAnimating, vehicleExitOrigin, vehicleExitAngles );

		// Ensure the endpoint is clear by dropping a point down from above
		vehicleExitOrigin -= VEC_VIEW;
		Vector vecMove = Vector(0,0,64);
		Vector vecStart = vehicleExitOrigin + vecMove;
		Vector vecEnd = vehicleExitOrigin - vecMove;
  		UTIL_TraceHull( vecStart, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );

		Assert( !tr.startsolid && tr.fraction < 1.0 );
		m_vecCurrentExitEndPoint = vecStart + ((vecEnd - vecStart) * tr.fraction);
		vecEyeExitEndpoint = m_vecCurrentExitEndPoint + VEC_VIEW;
		m_iCurrentExitAnim = 0;
		return pAnimating->LookupSequence( "exit_tauremoved" );
	}

	return BaseClass::GetExitAnimToUse( vecEyeExitEndpoint, bAllPointsBlocked );
}
