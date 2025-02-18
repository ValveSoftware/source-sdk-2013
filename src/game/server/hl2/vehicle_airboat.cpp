//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
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
#include "physics_saverestore.h"
#include "movevars_shared.h"
#include "npc_attackchopper.h"
#include "weapon_rpg.h"
#include "vphysics/constraints.h"
#include "world.h"
#include "rumble_shared.h"
// NVNT for airboat weapon fire
#include "haptics/haptic_utils.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_vehicle_autoaim_scale;

#define	VEHICLE_HITBOX_DRIVER	1

//
// Body groups.
//
#define AIRBOAT_BODYGROUP_GUN		1
#define AIRBOAT_BODYGROUP_PROP		2
#define AIRBOAT_BODYGROUP_BLUR		3

#define AIRBOAT_LOCK_SPEED			10		// Airboat must be going slower than this for player to enter or exit, in in/sec

#define AIRBOAT_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define AIRBOAT_FRAMETIME_MIN		1e-6

#define AIRBOAT_SPLASH_RIPPLE		0
#define AIRBOAT_SPLASH_SPRAY		1
#define AIRBOAT_SPLASH_RIPPLE_SIZE	20.0f

//
// Pose parameters.
//
#define AIRBOAT_GUN_YAW				"vehicle_weapon_yaw"
#define AIRBOAT_GUN_PITCH			"vehicle_weapon_pitch"
#define AIRBOAT_FRAME_FLEX_LEFT		"Frame_Flex_L"
#define AIRBOAT_FRAME_FLEX_RIGHT	"Frame_Flex_R"

#define CANNON_MAX_UP_PITCH			60.0f
#define CANNON_MAX_DOWN_PITCH		30.0f
#define CANNON_MAX_RIGHT_YAW		165.0f
#define CANNON_MAX_LEFT_YAW			75.0f

#define CANNON_HEAVY_SHOT_INTERVAL	0.2f
#define CANNON_SHAKE_INTERVAL		1.0f

static ConVar sk_airboat_max_ammo("sk_airboat_max_ammo", "100" );
static ConVar sk_airboat_recharge_rate("sk_airboat_recharge_rate", "15" );
static ConVar sk_airboat_drain_rate("sk_airboat_drain_rate", "10" );
static ConVar hud_airboathint_numentries( "hud_airboathint_numentries", "10", FCVAR_NONE );
static ConVar airboat_fatal_stress( "airboat_fatal_stress", "5000", FCVAR_NONE, "Amount of stress in kg that would kill the airboat driver." );

extern ConVar autoaim_max_dist;

class CPropAirboat : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropAirboat, CPropVehicleDriveable );

public:

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	// CPropVehicle
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	void			DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	bool			ShouldThink() { return true; }

	// CBaseEntity
	void			Think(void);
	void			Precache( void );
	void			Spawn( void );
	virtual void	OnRestore();
	virtual void	Activate();
	virtual void	UpdateOnRemove();
	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_USE_IN_RADIUS; };
	virtual void	DoMuzzleFlash( void );
	virtual void	StopLoopingSounds();

	// position to shoot at
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy );
	virtual Vector	GetSmoothedVelocity( void );

	virtual void	EnterVehicle( CBaseCombatCharacter *pPlayer );

	virtual bool	AllowBlockedExit( CBaseCombatCharacter *pPlayer, int nRole ) { return false; }
	virtual void	PreExitVehicle( CBaseCombatCharacter *pPlayer, int nRole );
	virtual void	ExitVehicle( int nRole );

	void			ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );
	void			DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void			DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );

	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void VPhysicsUpdate( IPhysicsObject *pPhysics );

	// Scraping noises for the various things we drive on. 
	virtual void	VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );

	bool HeadlightIsOn( void ) { return m_bHeadlightIsOn; }
	void HeadlightTurnOn( void );
	void HeadlightTurnOff( void );

	virtual bool ShouldDrawWaterImpacts( void );

	bool ShouldForceExit() { return m_bForcedExit; }
	void ClearForcedExit() { m_bForcedExit = false; }

	// Input handlers.
	void InputWake( inputdata_t &inputdata );
	void InputExitVehicle( inputdata_t &inputdata );
	void InputEnableGun( inputdata_t &inputdata );
	void InputStartRotorWashForces( inputdata_t &inputdata );
	void InputStopRotorWashForces( inputdata_t &inputdata );

	// Allows the shooter to change the impact effect of his bullets
	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	
	// Airboat passengers do not directly receive damage from blasts or radiation damage
	virtual bool PassengerShouldReceiveDamage( CTakeDamageInfo &info ) 
	{ 
		if ( info.GetDamageType() & DMG_VEHICLE )
			return true;

		return (info.GetDamageType() & (DMG_RADIATION|DMG_BLAST|DMG_CRUSH) ) == 0; 
	}
	
	const char *GetTracerType( void );

private:

	void			CreateAntiFlipConstraint();

	void			ApplyStressDamage( IPhysicsObject *pPhysics );
	float			CalculatePhysicsStressDamage( vphysics_objectstress_t *pStressOut, IPhysicsObject *pPhysics );

	void			CreateDangerSounds( void );

	void			FireGun( );

	void			UpdateSplashEffects( void );
	void			CreateSplash( int nSplashType );

	// Purpose: Aim Gun at a target
	void			AimGunAt( const Vector &endPos, float flInterval );

	// Purpose: Returns the direction the gun is currently aiming at
	void			GetGunAimDirection( Vector *resultDir );

	// Recharges the ammo based on speed
 	void			RechargeAmmo();

	// Removes the ammo...
	void			RemoveAmmo( float flAmmoAmount );

	// Purpose: 
	void			ComputeAimPoint( Vector *pVecAimPoint );
	
	// Do the right thing for the gun
	void			UpdateGunState( CUserCmd *ucmd );

	// Sound management
	void			CreateSounds();
	void			UpdateSound();
	void			UpdateWeaponSound();
	void			UpdateEngineSound( CSoundEnvelopeController &controller, float speedRatio );
	void			UpdateFanSound( CSoundEnvelopeController &controller, float speedRatio );
	void			UpdateWaterSound( CSoundEnvelopeController &controller, float speedRatio );

	void			UpdatePropeller();
	void			UpdateGauge();

	void			CreatePlayerBlocker();
	void			DestroyPlayerBlocker();
	void			EnablePlayerBlocker( bool bEnable );

private:

	enum
	{
		GUN_STATE_IDLE = 0,
		GUN_STATE_FIRING,
	};

	Vector			m_vecLastEyePos;
	Vector			m_vecLastEyeTarget;
	Vector			m_vecEyeSpeed;

	//float			m_flHandbrakeTime;			// handbrake after the fact to keep vehicles from rolling
	//bool			m_bInitialHandbrake;

	bool			m_bForcedExit;

	int				m_nGunRefAttachment;
	int				m_nGunBarrelAttachment;
	float			m_aimYaw;
	float			m_aimPitch;
	float			m_flChargeRemainder;
	float			m_flDrainRemainder;
	int				m_nGunState;
	float			m_flNextHeavyShotTime;
	float			m_flNextGunShakeTime;

	CNetworkVar( int, m_nAmmoCount );
	CNetworkVar( bool, m_bHeadlightIsOn );
	EHANDLE			m_hAvoidSphere;

	int				m_nSplashAttachment;

	float			m_flPrevThrottle;			// Throttle during last think. Used for detecting state changes.
	float			m_flSpinRate;				// Current rate of spin of propeller: 0 = min, 1.0 = max
	float			m_flTargetSpinRate;			// Target rate of spin of propeller: 0 = min, 1.0 = max
	float			m_flPropTime;				// Time to turn on/off the prop.
	float			m_flBlurTime;				// Time to turn on/off the blur.

	CSoundPatch		*m_pFanSound;
	CSoundPatch		*m_pFanMaxSpeedSound;
	CSoundPatch		*m_pEngineSound;
	CSoundPatch		*m_pWaterFastSound;
	CSoundPatch		*m_pWaterStoppedSound;
	CSoundPatch		*m_pGunFiringSound;

	float			m_flEngineIdleTime;			// Time to start playing the engine's idle sound.
	float			m_flEngineDuckTime;			// Time to reduce the volume of the engine's idle sound.

	bool			m_bFadeOutFan;				// Fade out fan sound after cruising at max speed for a while.

	int				m_nPrevWaterLevel;			// Used for detecting transitions into/out of water.
	float			m_flWaterStoppedPitchTime;	// Time to pitch shift the water stopped sound.

	float			m_flLastImpactEffectTime;
	int				m_iNumberOfEntries;
	
	IPhysicsConstraint *m_pAntiFlipConstraint;	// A ragdoll constraint that prevents us from flipping.
	
	CHandle<CEntityBlocker>	m_hPlayerBlocker;
	
	CNetworkVar( Vector, m_vecPhysVelocity );

	CNetworkVar( int, m_nExactWaterLevel );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nWaterLevel );

};

IMPLEMENT_SERVERCLASS_ST( CPropAirboat, DT_PropAirboat )
	SendPropBool( SENDINFO( m_bHeadlightIsOn ) ),
	SendPropInt( SENDINFO( m_nAmmoCount ), 9 ),
	SendPropInt( SENDINFO( m_nExactWaterLevel ) ),
	SendPropInt( SENDINFO( m_nWaterLevel ) ),
	SendPropVector( SENDINFO( m_vecPhysVelocity ) ),
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS( prop_vehicle_airboat, CPropAirboat );

BEGIN_DATADESC( CPropAirboat )
	DEFINE_FIELD( m_vecLastEyePos,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecLastEyeTarget,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecEyeSpeed,		FIELD_VECTOR ),

//	DEFINE_FIELD( m_flHandbrakeTime,	FIELD_TIME ),
//	DEFINE_FIELD( m_bInitialHandbrake,FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_nGunRefAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nGunBarrelAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_aimYaw,				FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flChargeRemainder,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flDrainRemainder,	FIELD_FLOAT ),
	DEFINE_FIELD( m_nGunState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextHeavyShotTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextGunShakeTime, FIELD_TIME ),
	DEFINE_FIELD( m_nAmmoCount,			FIELD_INTEGER ),
	DEFINE_FIELD( m_bHeadlightIsOn,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hAvoidSphere,		FIELD_EHANDLE ),
//	DEFINE_FIELD( m_nSplashAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hPlayerBlocker,		FIELD_EHANDLE ),

	DEFINE_FIELD( m_vecPhysVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( m_nExactWaterLevel,	FIELD_INTEGER ),

	DEFINE_FIELD( m_flPrevThrottle,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flSpinRate,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flTargetSpinRate,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flPropTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flBlurTime,			FIELD_TIME ),
	DEFINE_FIELD( m_bForcedExit,		FIELD_BOOLEAN ),

	DEFINE_SOUNDPATCH( m_pFanSound ),
	DEFINE_SOUNDPATCH( m_pFanMaxSpeedSound ),
	DEFINE_SOUNDPATCH( m_pEngineSound ),
	DEFINE_SOUNDPATCH( m_pWaterFastSound ),
	DEFINE_SOUNDPATCH( m_pWaterStoppedSound ),
	DEFINE_SOUNDPATCH( m_pGunFiringSound ),

	DEFINE_PHYSPTR( m_pAntiFlipConstraint ),

	DEFINE_FIELD( m_flEngineIdleTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flEngineDuckTime,			FIELD_TIME ),
	DEFINE_FIELD( m_bFadeOutFan,				FIELD_BOOLEAN ),

	DEFINE_FIELD( m_nPrevWaterLevel,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flWaterStoppedPitchTime,	FIELD_TIME ),

	DEFINE_FIELD( m_flLastImpactEffectTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iNumberOfEntries,			FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "EnableGun", InputEnableGun ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartRotorWashForces", InputStartRotorWashForces ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopRotorWashForces", InputStopRotorWashForces ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ExitVehicle", InputExitVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Wake", InputWake ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Airboat_engine_stop" );
	PrecacheScriptSound( "Airboat_engine_start" );

	PrecacheScriptSound( "Airboat.FireGunHeavy" );
	PrecacheScriptSound( "Airboat.FireGunRevDown");

	PrecacheScriptSound( "Airboat_engine_idle" );
	PrecacheScriptSound( "Airboat_engine_fullthrottle" );
	PrecacheScriptSound( "Airboat_fan_idle" );
	PrecacheScriptSound( "Airboat_fan_fullthrottle" );
	PrecacheScriptSound( "Airboat_water_stopped" );
	PrecacheScriptSound( "Airboat_water_fast" );
	PrecacheScriptSound( "Airboat_impact_splash" );
	PrecacheScriptSound( "Airboat_impact_hard" );

	PrecacheScriptSound( "Airboat_headlight_on" );
	PrecacheScriptSound( "Airboat_headlight_off" );

	PrecacheScriptSound( "Airboat.FireGunLoop" );

	PrecacheMaterial( "effects/splashwake1" );
	PrecacheMaterial( "effects/splashwake4" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::Spawn( void )
{
	m_nAmmoCount = m_bHasGun ? 0 : -1;
	m_hAvoidSphere = CreateHelicopterAvoidanceSphere( this, 0, 50.0f, false );
	m_flLastImpactEffectTime = -1;
	m_iNumberOfEntries = 0;

	// Setup vehicle as a ray-cast airboat.
	SetVehicleType( VEHICLE_TYPE_AIRBOAT_RAYCAST );
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );
	BaseClass::Spawn();

	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetAnimatedEveryTick( true );

	// Handbrake data.
	//m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	//m_bInitialHandbrake = false;
	m_VehiclePhysics.SetHasBrakePedal( false );

	m_flMinimumSpeedToEnterExit = AIRBOAT_LOCK_SPEED;

	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetBodygroup(AIRBOAT_BODYGROUP_GUN, m_bHasGun);
	SetBodygroup(AIRBOAT_BODYGROUP_PROP, true);

	SetPoseParameter( AIRBOAT_GUN_YAW, 0 );
	SetPoseParameter( AIRBOAT_GUN_PITCH, 0 );
	SetPoseParameter( AIRBOAT_FRAME_FLEX_LEFT, 0 );
	SetPoseParameter( AIRBOAT_FRAME_FLEX_RIGHT, 0 );

	m_aimYaw = 0;
	m_aimPitch = 0;
	m_bUnableToFire = true;
	m_nGunState = GUN_STATE_IDLE;

	SetPoseParameter( "Steer_Shock", 0.0f );

	// Get the physics object so we can adjust the buoyancy.
	IPhysicsObject *pPhysAirboat = VPhysicsGetObject();
	if ( pPhysAirboat )
	{
		pPhysAirboat->SetBuoyancyRatio( 0.0f );
		PhysSetGameFlags( pPhysAirboat, FVPHYSICS_HEAVY_OBJECT );
	}

	//CreateAntiFlipConstraint();
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll constraint that prevents us from flipping.
//-----------------------------------------------------------------------------
void CPropAirboat::CreateAntiFlipConstraint()
{
	constraint_ragdollparams_t ragdoll;
	ragdoll.Defaults();

	// Don't prevent the boat from moving, just flipping.
	ragdoll.onlyAngularLimits = true;

	// Put the ragdoll constraint in the space of the airboat.
	SetIdentityMatrix( ragdoll.constraintToAttached );
	BuildObjectRelativeXform( g_PhysWorldObject, VPhysicsGetObject(), ragdoll.constraintToReference );

	ragdoll.axes[0].minRotation = -100;
	ragdoll.axes[0].maxRotation = 100;
	ragdoll.axes[1].minRotation = -100;
	ragdoll.axes[1].maxRotation = 100;
	ragdoll.axes[2].minRotation = -180;
	ragdoll.axes[2].maxRotation = 180;

	m_pAntiFlipConstraint = physenv->CreateRagdollConstraint( g_PhysWorldObject, VPhysicsGetObject(), NULL, ragdoll );

	//NDebugOverlay::Cross3DOriented( ragdoll.constraintToReference, 128, 255, true, 100 );
}


//-----------------------------------------------------------------------------
// Attachment indices
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if ( m_hAvoidSphere )
	{
		UTIL_Remove( m_hAvoidSphere );
		m_hAvoidSphere = NULL;
	}
}


//-----------------------------------------------------------------------------
// Attachment indices
//-----------------------------------------------------------------------------
void CPropAirboat::Activate()
{
	BaseClass::Activate();

	m_nGunRefAttachment = LookupAttachment( "gun" );
	m_nGunBarrelAttachment = LookupAttachment( "muzzle" );
	m_nSplashAttachment = LookupAttachment( "splash_pt" );

	CreateSounds();

	CBaseServerVehicle *pServerVehicle = dynamic_cast<CBaseServerVehicle *>(GetServerVehicle());
	if ( pServerVehicle )
	{
		if( pServerVehicle->GetPassenger() )
		{
			// If a boat comes back from a save game with a driver, make sure the engine rumble starts up.
			pServerVehicle->StartEngineRumble();
		}
	}

	//CreatePlayerBlocker();
	//EnablePlayerBlocker( true );
}


void CPropAirboat::CreatePlayerBlocker()
{
	Assert( m_hPlayerBlocker == NULL );
	DestroyPlayerBlocker();
	
	m_hPlayerBlocker = CEntityBlocker::Create( GetAbsOrigin(), Vector( -84, -32, 0 ), Vector( 54, 32, 84 ), this, false );
	if ( m_hPlayerBlocker != NULL )
	{
		m_hPlayerBlocker->SetParent( this );
		m_hPlayerBlocker->SetLocalOrigin( vec3_origin );
		m_hPlayerBlocker->SetLocalAngles( vec3_angle );
		m_hPlayerBlocker->SetCollisionGroup( COLLISION_GROUP_PLAYER );
		m_hPlayerBlocker->AddSolidFlags( FSOLID_NOT_SOLID );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::DestroyPlayerBlocker()
{
	if ( m_hPlayerBlocker != NULL )
	{
		UTIL_Remove( m_hPlayerBlocker );
	}

	m_hPlayerBlocker = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
void CPropAirboat::EnablePlayerBlocker( bool bEnable )
{
	if ( m_hPlayerBlocker != NULL )
	{
		if ( bEnable )
		{
			m_hPlayerBlocker->RemoveSolidFlags( FSOLID_NOT_SOLID );
		}
		else
		{
			m_hPlayerBlocker->AddSolidFlags( FSOLID_NOT_SOLID );
		}
	}
}


//-----------------------------------------------------------------------------
// Update the weapon sounds
//-----------------------------------------------------------------------------
#define MIN_CHARGE_SOUND 0.4f
#define MIN_PITCH_CHANGE ( MIN_CHARGE_SOUND + ( ( 1.0f - MIN_CHARGE_SOUND ) / 3.0f ) )
#define VOLUME_CHANGE_TIME 0.5f

void CPropAirboat::UpdateWeaponSound()
{
	if ( HasGun() )
	{
		CSoundEnvelopeController *pController = &CSoundEnvelopeController::GetController();
		float flVolume = pController->SoundGetVolume( m_pGunFiringSound );
		if ( (m_nGunState == GUN_STATE_IDLE) || (m_nAmmoCount == 0) )
		{
			if ( flVolume != 0.0f )
			{
				pController->SoundChangeVolume( m_pGunFiringSound, 0.0f, 0.01f );
			}
		}
		else
		{
			if ( flVolume != 1.0f )
			{
				pController->SoundChangeVolume( m_pGunFiringSound, 1.0f, 0.01f );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Force the player to exit the vehicle.
//-----------------------------------------------------------------------------
void CPropAirboat::InputExitVehicle( inputdata_t &inputdata )
{
	m_bForcedExit = true;
}


//-----------------------------------------------------------------------------
// Purpose: Force the airboat to wake up. This was needed to fix a last-minute
//			bug for the XBox -- the airboat didn't fall with the platform
//			in d1_canals_10b.
//-----------------------------------------------------------------------------
void CPropAirboat::InputWake( inputdata_t &inputdata )
{
	VPhysicsGetObject()->Wake();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to enable or disable the airboat's mounted gun.
//-----------------------------------------------------------------------------
void CPropAirboat::InputEnableGun( inputdata_t &inputdata )
{
	m_bHasGun = inputdata.value.Bool();
	SetBodygroup(AIRBOAT_BODYGROUP_GUN, m_bHasGun);

	// When enabling the gun, give full ammo
	if ( m_bHasGun )
	{
		m_nAmmoCount = sk_airboat_max_ammo.GetInt();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to enable or disable the airboat's mounted gun.
//-----------------------------------------------------------------------------
void CPropAirboat::InputStartRotorWashForces( inputdata_t &inputdata )
{
	RemoveEFlags( EFL_NO_ROTORWASH_PUSH );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to enable or disable the airboat's mounted gun.
//-----------------------------------------------------------------------------
void CPropAirboat::InputStopRotorWashForces( inputdata_t &inputdata )
{
	AddEFlags( EFL_NO_ROTORWASH_PUSH );
}


//-----------------------------------------------------------------------------
// Creating vphysics
//-----------------------------------------------------------------------------
void CPropAirboat::OnRestore()
{
	BaseClass::OnRestore();

	IPhysicsObject *pPhysAirboat = VPhysicsGetObject();
	if ( pPhysAirboat )
	{
		pPhysAirboat->SetBuoyancyRatio( 0.0f );
		PhysSetGameFlags( pPhysAirboat, FVPHYSICS_HEAVY_OBJECT );
	}

	// If the player's in the vehicle, NPCs should ignore it 
	if ( GetDriver() )
	{
		SetNavIgnore();
	}
}


//-----------------------------------------------------------------------------
// Used for navigation
//-----------------------------------------------------------------------------
void CPropAirboat::EnterVehicle( CBaseCombatCharacter *pPlayer )
{
	BaseClass::EnterVehicle( pPlayer );

	//EnablePlayerBlocker( false );

	// NPCs like manhacks should try to hit us
	SetNavIgnore();

	// Play the engine start sound.
	float flDuration;
	EmitSound( "Airboat_engine_start", 0.0, &flDuration );
	m_VehiclePhysics.TurnOn();

	// Start playing the engine's idle sound as the startup sound finishes.
	m_flEngineIdleTime = gpGlobals->curtime + flDuration - 0.1;
}


//-----------------------------------------------------------------------------
// Purpose: Called when exiting, just before playing the exit animation.
//-----------------------------------------------------------------------------
void CPropAirboat::PreExitVehicle( CBaseCombatCharacter *pPlayer, int nRole )
{
	if ( HeadlightIsOn() )
	{
		HeadlightTurnOff();
	}

	// Stop shooting.
	m_nGunState = GUN_STATE_IDLE;

	CBaseEntity *pDriver = GetDriver();
	CBasePlayer *pPlayerDriver;
	if( pDriver && pDriver->IsPlayer() )
	{
		pPlayerDriver = dynamic_cast<CBasePlayer*>(pDriver);
		if( pPlayerDriver )
		{
			pPlayerDriver->RumbleEffect( RUMBLE_AIRBOAT_GUN, 0, RUMBLE_FLAG_STOP );
		}
	}

	BaseClass::PreExitVehicle( pPlayer, nRole );
}


//-----------------------------------------------------------------------------
// Purpose: Called when exiting, after completing the exit animation.
// Input  : iRole - 
//-----------------------------------------------------------------------------
void CPropAirboat::ExitVehicle( int nRole )
{
	CBaseEntity *pDriver = GetDriver();

	//EnablePlayerBlocker( true );

	BaseClass::ExitVehicle( nRole );

	if (!pDriver)
		return;

#if 0
	// On ORANGE BOX this is causing a big blank box to show up, which is worse
	// than the HUD hint persisting for a little while, so don't do it. (sjb)
	// clear the hint
	UTIL_HudHintText( pDriver, "" );
#endif

	// NPCs like manhacks should try to avoid us again
	ClearNavIgnore();

	// Play the engine shutoff sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );

	EmitSound_t ep;
	ep.m_nChannel = CHAN_BODY;
	ep.m_pSoundName = "Airboat_engine_stop";
	ep.m_flVolume = controller.SoundGetVolume( m_pEngineSound );
	ep.m_SoundLevel = SNDLVL_NORM;
	ep.m_nPitch = controller.SoundGetPitch( m_pEngineSound );

	EmitSound( filter, entindex(), ep );
	m_VehiclePhysics.TurnOff();

	// Shut off the airboat sounds.
	controller.SoundChangeVolume( m_pEngineSound, 0.0, 0.0 );
	controller.SoundChangeVolume( m_pFanSound, 0.0, 0.0 );
	controller.SoundChangeVolume( m_pFanMaxSpeedSound, 0.0, 0.0 );
	controller.SoundChangeVolume( m_pWaterStoppedSound, 0.0, 0.0 );
	controller.SoundChangeVolume( m_pWaterFastSound, 0.0, 0.0 );
	controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::HeadlightTurnOn( void )
{
	EmitSound( "Airboat_headlight_on" );
	m_bHeadlightIsOn = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::HeadlightTurnOff( void )
{
	EmitSound( "Airboat_headlight_off" );
	m_bHeadlightIsOn = false;
}


//-----------------------------------------------------------------------------
// position to shoot at
//-----------------------------------------------------------------------------
Vector CPropAirboat::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	Vector vecPosition;
	QAngle angles;
	if ( GetServerVehicle()->GetPassenger() )
	{
		// FIXME: Reconcile this with other functions that store a cached version of the results here?
		GetServerVehicle()->GetVehicleViewPosition( VEHICLE_ROLE_DRIVER, &vecPosition, &angles );
	}
	else
	{
		vecPosition = WorldSpaceCenter();
	}
	return vecPosition;
}


//-----------------------------------------------------------------------------
// Smoothed velocity
//-----------------------------------------------------------------------------
#define SMOOTHED_MIN_VELOCITY 75.0f
#define SMOOTHED_MAX_VELOCITY 150.0f

Vector CPropAirboat::GetSmoothedVelocity( void )
{
	// If we're going too slow, return the forward direction as the velocity
	// for NPC prediction purposes
	Vector vecSmoothedVelocity = BaseClass::GetSmoothedVelocity();
	float flSpeed = vecSmoothedVelocity.Length();
	if ( flSpeed >= SMOOTHED_MAX_VELOCITY )
		return vecSmoothedVelocity;

	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );
	vecForward *= MAX( flSpeed, 1.0f );
	if ( flSpeed <= SMOOTHED_MIN_VELOCITY )
		return vecForward;

	float flBlend = SimpleSplineRemapVal( flSpeed, SMOOTHED_MIN_VELOCITY, SMOOTHED_MAX_VELOCITY, 0.0f, 1.0f );
	VectorLerp( vecForward, vecSmoothedVelocity, flBlend, vecSmoothedVelocity );
	return vecSmoothedVelocity;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
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
// Purpose: 
// Input  : &info - 
//			&vecEnd - 
//			*pTraceFilter - 
//			*pVecTracerDest - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropAirboat::ShouldDrawWaterImpacts( void )
{
	// The airboat spits out so much crap that we need to do cheaper versions
	// of the impact effects. Also, we need to do less of them.
	if ( m_flLastImpactEffectTime >= gpGlobals->curtime )
		return false;

	m_flLastImpactEffectTime = gpGlobals->curtime + 0.05f;

	return true;
}

//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CPropAirboat::DoImpactEffect( trace_t &tr, int nDamageType )
{
	// The airboat spits out so much crap that we need to do cheaper versions
	// of the impact effects. Also, we need to do less of them.
	if ( m_flLastImpactEffectTime == gpGlobals->curtime )
		return;

	// Randomly drop out
	if ( random->RandomInt( 0, 5 ) )
		return;

	m_flLastImpactEffectTime = gpGlobals->curtime;
	UTIL_ImpactTrace( &tr, nDamageType, "AirboatGunImpact" );
} 
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropAirboat::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Do scaled up physics damage to the airboat
	CTakeDamageInfo physDmg = info;
	physDmg.ScaleDamage( 5 );
	if ( physDmg.GetDamageType() & DMG_BLAST )
	{
		physDmg.SetDamageForce( info.GetDamageForce() * 10 );
	}
	VPhysicsTakeDamage( physDmg );

	// Check to do damage to driver
	if ( m_hPlayer != NULL )
	{
		// Don't pass along physics damage
		if ( info.GetDamageType() & (DMG_CRUSH|DMG_RADIATION) )
			return 0;

		// Take the damage (strip out the DMG_BLAST)
		CTakeDamageInfo playerDmg = info;

		// Mark that we're passing it to the player so the base player accepts the damage
		playerDmg.SetDamageType( info.GetDamageType() | DMG_VEHICLE );

		// Deal the damage to the passenger
		m_hPlayer->TakeDamage( playerDmg );
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Scraping noises for the various things we drive on. 
//-----------------------------------------------------------------------------
void CPropAirboat::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	// don't make noise for hidden/invisible/sky materials
	const surfacedata_t *phit = physprops->GetSurfaceData( surfacePropsHit );
	const surfacedata_t *pprops = physprops->GetSurfaceData( surfaceProps );
	if ( phit->game.material == 'X' || pprops->game.material == 'X' )
		return;

	// FIXME: Make different scraping sounds here
	float flVolume = 0.3f;

	surfacedata_t *psurf = physprops->GetSurfaceData( surfaceProps );
	const char *pSoundName = physprops->GetString( psurf->sounds.scrapeRough );

	PhysFrictionSound( this, pObject, pSoundName, psurf->soundhandles.scrapeRough, flVolume );
}


//-----------------------------------------------------------------------------
// Purpose: Aim Gun at a target position.
//-----------------------------------------------------------------------------
// This fixes an optimizer bug that was causing targetYaw and targetPitch to
// always be reported as clamped, thus disabling the gun. Ack!
#pragma optimize("", off)
void CPropAirboat::AimGunAt( const Vector &aimPos, float flInterval )
{
	matrix3x4_t gunMatrix;
	GetAttachment( m_nGunRefAttachment, gunMatrix );

	// transform the target position into gun space
	Vector localTargetPosition;
	VectorITransform( aimPos, gunMatrix, localTargetPosition );
	VectorNormalize( localTargetPosition );
	m_bUnableToFire = false;
	m_vecGunCrosshair = aimPos;

	// do a look at in gun space (essentially a delta-lookat)
	QAngle localTargetAngles;
	VectorAngles( localTargetPosition, localTargetAngles );
	
	// convert to +/- 180 degrees
	localTargetAngles.x = UTIL_AngleDiff( localTargetAngles.x, 0 );	
	localTargetAngles.y = UTIL_AngleDiff( localTargetAngles.y, 0 );

	float targetYaw = m_aimYaw + localTargetAngles.y;
	float targetPitch = m_aimPitch + localTargetAngles.x;
	
	// Constrain our angles
	float newTargetYaw = clamp( targetYaw, -CANNON_MAX_RIGHT_YAW, CANNON_MAX_LEFT_YAW );
	float newTargetPitch = clamp( targetPitch, -CANNON_MAX_UP_PITCH, CANNON_MAX_DOWN_PITCH );

	// If the angles have been clamped, we're looking outside of our valid range
	if ( ( newTargetYaw != targetYaw ) || ( newTargetPitch != targetPitch ) )
	{
		m_bUnableToFire = true;
	}

	targetYaw = newTargetYaw;
	targetPitch = newTargetPitch;

	m_aimYaw = targetYaw;
	m_aimPitch = targetPitch;

	SetPoseParameter( AIRBOAT_GUN_YAW, m_aimYaw);
	SetPoseParameter( AIRBOAT_GUN_PITCH, m_aimPitch );

	InvalidateBoneCache();

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = GetPoseParameter( AIRBOAT_GUN_PITCH );
	m_aimYaw = GetPoseParameter( AIRBOAT_GUN_YAW );
}
#pragma optimize("", on)


//-----------------------------------------------------------------------------
// Removes the ammo...
//-----------------------------------------------------------------------------
void CPropAirboat::RemoveAmmo( float flAmmoAmount )
{
	m_flDrainRemainder += flAmmoAmount;
	int nAmmoToRemove = (int)m_flDrainRemainder;
	m_flDrainRemainder -= nAmmoToRemove;
	m_nAmmoCount -= nAmmoToRemove;
	if ( m_nAmmoCount < 0 )
	{
		m_nAmmoCount = 0;
		m_flDrainRemainder = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// Recharges the ammo...
//-----------------------------------------------------------------------------
void CPropAirboat::RechargeAmmo(void)
{
	if ( !m_bHasGun )
	{
		m_nAmmoCount = -1;
		return;
	}

	int nMaxAmmo = sk_airboat_max_ammo.GetInt();
	if ( m_nAmmoCount == nMaxAmmo )
		return;

	float flRechargeRate = sk_airboat_recharge_rate.GetInt();
	float flChargeAmount = flRechargeRate * gpGlobals->frametime;
	if ( m_flDrainRemainder != 0.0f )
	{
		if ( m_flDrainRemainder >= flChargeAmount )
		{
			m_flDrainRemainder -= flChargeAmount;
			return;
		}
		else
		{
			flChargeAmount -= m_flDrainRemainder;
			m_flDrainRemainder = 0.0f;
		}
	}

	m_flChargeRemainder += flChargeAmount;
	int nAmmoToAdd = (int)m_flChargeRemainder;
	m_flChargeRemainder -= nAmmoToAdd;
	m_nAmmoCount += nAmmoToAdd;
	if ( m_nAmmoCount > nMaxAmmo )
	{
		m_nAmmoCount = nMaxAmmo;
		m_flChargeRemainder = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::ComputeAimPoint( Vector *pVecAimPoint )
{
	Vector vecEyeDirection;

	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		// Use autoaim as the eye dir.
		autoaim_params_t params;

		params.m_fScale = AUTOAIM_SCALE_DEFAULT * sv_vehicle_autoaim_scale.GetFloat();
		params.m_fMaxDist = autoaim_max_dist.GetFloat();
		m_hPlayer->GetAutoaimVector( params );

		vecEyeDirection = params.m_vecAutoAimDir;
	}
	else
	{
		m_hPlayer->EyeVectors( &vecEyeDirection, NULL, NULL );
	}

	Vector vecEndPos;
	VectorMA( m_hPlayer->EyePosition(), MAX_TRACE_LENGTH, vecEyeDirection, vecEndPos );
	trace_t	trace;
	UTIL_TraceLine( m_hPlayer->EyePosition(), vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );
	*pVecAimPoint = trace.endpos;
}


//-----------------------------------------------------------------------------
// Purpose: Manages animation and sound state.
//-----------------------------------------------------------------------------
void CPropAirboat::Think(void)
{
	BaseClass::Think();

	// set handbrake after physics sim settles down
//	if ( gpGlobals->curtime < m_flHandbrakeTime )
//	{
//		SetNextThink( gpGlobals->curtime );
//	}
//	else if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
//	{
//		m_bInitialHandbrake = true;
//		m_VehiclePhysics.SetHandbrake( true );
//		m_VehiclePhysics.Think();
//	}

	// Find the vertical extents of the boat
	Vector startPos, endPos;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &startPos );
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &endPos );

	// Look for water along that volume.
	// Make a very vertically thin box and sweep it along the ray.
	Vector vecMins = CollisionProp()->OBBMins();
	Vector vecMaxs = CollisionProp()->OBBMaxs();
	vecMins.z = -0.1f;
	vecMaxs.z = 0.1f;

	trace_t	tr;
	UTIL_TraceHull( startPos, endPos, vecMins, vecMaxs, (CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &tr );

	// If we hit something, then save off the info
	if ( tr.fraction != 1.0f )
	{
		m_nExactWaterLevel = tr.endpos.z;

		// Classify what we're in
		if ( tr.contents & CONTENTS_SLIME )
		{
			// We fake this value to mean type, instead of level
			SetWaterLevel( 2 );
		}
		else
		{
			// This simply signifies water
			SetWaterLevel( 1 );
		}
	}
	else
	{
		// Not in water
		SetWaterLevel( 0 );
	}

	StudioFrameAdvance();

	// If the enter or exit animation has finished, tell the server vehicle
	if ( IsSequenceFinished() && ( m_bEnterAnimOn ||  m_bExitAnimOn ) )
	{
		// The first few time we get into the jeep, print the jeep help
		if ( m_iNumberOfEntries < hud_airboathint_numentries.GetInt() && !m_bExitAnimOn )
		{
			UTIL_HudHintText( m_hPlayer, "#Valve_Hint_BoatKeys" );
			m_iNumberOfEntries++;
		}
		
		GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, false );

		// Start the vehicle's idle animation
		ResetSequence(LookupSequence("propeller_spin1"));
		ResetClientsideFrame();
	}

	// FIXME: Slam the crosshair every think -- if we don't do this it disappears randomly, never to return.
	if ( ( m_hPlayer.Get() != NULL ) && !( m_bEnterAnimOn ||  m_bExitAnimOn ) )
	{
		m_hPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_VEHICLE_CROSSHAIR;
	}

	// Aim the gun
	if ( HasGun() && m_hPlayer.Get() && !m_bEnterAnimOn && !m_bExitAnimOn )
	{
		Vector vecAimPoint;
		ComputeAimPoint( &vecAimPoint );
		AimGunAt( vecAimPoint, gpGlobals->frametime );
	}

	if ( ShouldForceExit() )
	{
		ClearForcedExit();
		m_hPlayer->LeaveVehicle();
	}

	if ( HasGun() && ( m_nGunState == GUN_STATE_IDLE ) )
	{
		RechargeAmmo();
	}

	UpdateSound();
	UpdatePropeller();
	UpdateGauge();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::UpdatePropeller()
{
	if ((m_bExitAnimOn) || (m_bEnterAnimOn))
		return;

	#define SPIN_RATE_MED	0.2
	#define SPIN_RATE_HIGH	0.6

	// Determine target spin rate from throttle.
	float flTargetSpinRate = m_flThrottle;
	if ((flTargetSpinRate == 0) && (m_hPlayer))
	{
		// Always keep the fan moving a little when we have a driver.
		flTargetSpinRate = 0.2;
	}

	// Save the current spin rate to determine state transitions.
	float flPrevSpinRate = m_flSpinRate;

	// Determine new spin rate,
	if (m_flSpinRate < flTargetSpinRate)
	{
		if (flTargetSpinRate > 0)
		{
			m_flSpinRate += gpGlobals->frametime * 1.0;
		}
		else
		{
			m_flSpinRate += gpGlobals->frametime * 0.4;
		}

		if (m_flSpinRate > flTargetSpinRate)
		{
			m_flSpinRate = flTargetSpinRate;
		}
	}
	else if (m_flSpinRate > flTargetSpinRate)
	{
		m_flSpinRate -= gpGlobals->frametime * 0.4;
		if (m_flSpinRate < flTargetSpinRate)
		{
			m_flSpinRate = flTargetSpinRate;
		}
	}

	// Update prop & blur based on new spin rate.
	if (fabs(m_flSpinRate) > SPIN_RATE_HIGH)
	{
		if (fabs(flPrevSpinRate) <= SPIN_RATE_HIGH)
		{
			SetBodygroup(AIRBOAT_BODYGROUP_PROP, false);
			SetBodygroup(AIRBOAT_BODYGROUP_BLUR, true);
			SetSequence(LookupSequence("propeller_spin1"));
		}
	}
	else if (fabs(m_flSpinRate) > SPIN_RATE_MED)
	{
		if ((fabs(flPrevSpinRate) <= SPIN_RATE_MED) || (fabs(flPrevSpinRate) > SPIN_RATE_HIGH))
		{
			SetBodygroup(AIRBOAT_BODYGROUP_PROP, true);
			SetBodygroup(AIRBOAT_BODYGROUP_BLUR, true);
			SetSequence(LookupSequence("propeller_spin1"));
		}
	}
	else
	{
		if (fabs(flPrevSpinRate) > SPIN_RATE_MED)
		{
			SetBodygroup(AIRBOAT_BODYGROUP_PROP, true);
			SetBodygroup(AIRBOAT_BODYGROUP_BLUR, false);
			SetSequence(LookupSequence("propeller_spin1"));
		}
	}

	SetPlaybackRate( m_flSpinRate );

	m_flPrevThrottle = m_flThrottle;
}


//-----------------------------------------------------------------------------
// Purpose: Updates the speedometer.
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateGauge()
{
	CFourWheelVehiclePhysics *pPhysics = GetPhysics();
	int speed = pPhysics->GetSpeed();
	int maxSpeed = pPhysics->GetMaxSpeed();
	float speedRatio = clamp( (float)speed / (float)maxSpeed, 0, 1 );

	SetPoseParameter( "Gauge", speedRatio );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::CreateSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( this );

	if (!m_pEngineSound)
	{
		m_pEngineSound = controller.SoundCreate( filter, entindex(), "Airboat_engine_idle" );
		controller.Play( m_pEngineSound, 0, 100 );
	}

	if (!m_pFanSound)
	{
		m_pFanSound = controller.SoundCreate( filter, entindex(), "Airboat_fan_idle" );
		controller.Play( m_pFanSound, 0, 100 );
	}

	if (!m_pFanMaxSpeedSound)
	{
		m_pFanMaxSpeedSound = controller.SoundCreate( filter, entindex(), "Airboat_fan_fullthrottle" );
		controller.Play( m_pFanMaxSpeedSound, 0, 100 );
	}

	if (!m_pWaterStoppedSound)
	{
		m_pWaterStoppedSound = controller.SoundCreate( filter, entindex(), "Airboat_water_stopped" );
		controller.Play( m_pWaterStoppedSound, 0, 100 );
	}

	if (!m_pWaterFastSound)
	{
		m_pWaterFastSound = controller.SoundCreate( filter, entindex(), "Airboat_water_fast" );
		controller.Play( m_pWaterFastSound, 0, 100 );
	}

	if (!m_pGunFiringSound)
	{
		m_pGunFiringSound = controller.SoundCreate( filter, entindex(), "Airboat.FireGunLoop" );
		controller.Play( m_pGunFiringSound, 0, 100 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::StopLoopingSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy( m_pEngineSound );
	m_pEngineSound = NULL;

	controller.SoundDestroy( m_pFanSound );
	m_pFanSound = NULL;

	controller.SoundDestroy( m_pFanMaxSpeedSound );
	m_pFanMaxSpeedSound = NULL;

	controller.SoundDestroy( m_pWaterStoppedSound );
	m_pWaterStoppedSound = NULL;

	controller.SoundDestroy( m_pWaterFastSound );
	m_pWaterFastSound = NULL;

	controller.SoundDestroy( m_pGunFiringSound );
	m_pGunFiringSound = NULL;

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: Manage the state of the engine sound.
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateEngineSound( CSoundEnvelopeController &controller, float speedRatio )
{
	#define ENGINE_MIN_VOLUME	0.22
	#define ENGINE_MAX_VOLUME	0.62
	#define	ENGINE_MIN_PITCH	80
	#define	ENGINE_MAX_PITCH	140
	#define ENGINE_DUCK_TIME	4.0

	if ( controller.SoundGetVolume(m_pEngineSound ) == 0 )
	{ 
		if ( gpGlobals->curtime > m_flEngineIdleTime )
		{
			// If we've finished playing the engine start sound, start playing the idle sound.
			controller.Play( m_pEngineSound, ENGINE_MAX_VOLUME, 100 );

			// Ramp down the engine idle sound over time so that we can ramp it back up again based on speed.
			controller.SoundChangeVolume( m_pEngineSound, ENGINE_MIN_VOLUME, ENGINE_DUCK_TIME );
			controller.SoundChangePitch( m_pEngineSound, ENGINE_MIN_PITCH, ENGINE_DUCK_TIME );

			// Reduce the volume of the engine idle sound after our ears get 'used' to it.
			m_flEngineDuckTime = gpGlobals->curtime + ENGINE_DUCK_TIME;
		}
	}
	else if ( gpGlobals->curtime > m_flEngineDuckTime )
	{
		controller.SoundChangeVolume( m_pEngineSound, RemapValClamped(speedRatio, 0, 1.0, ENGINE_MIN_VOLUME, ENGINE_MAX_VOLUME ), 0.0 );
		controller.SoundChangePitch( m_pEngineSound, RemapValClamped( speedRatio, 0, 1.0, ENGINE_MIN_PITCH, ENGINE_MAX_PITCH ), 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateFanSound( CSoundEnvelopeController &controller, float speedRatio )
{
	#define FAN_MIN_VOLUME	0.0
	#define FAN_MAX_VOLUME	0.82
	#define FAN_DUCK_VOLUME	0.22
	#define FAN_CHANGE_VOLUME_TIME	1.0		// seconds over which to change the volume
	#define FAN_DUCK_TIME 2.0				// seconds over which to duck the fan sound

	// Manage the state of the fan sound.
	if (speedRatio >= 0.8)
	{
		// Crossfade between a 'max speed' fan sound and the normal fan sound.
		controller.SoundChangeVolume( m_pFanSound, RemapValClamped( speedRatio, 0.8, 1.0, FAN_MAX_VOLUME, FAN_MIN_VOLUME ), FAN_CHANGE_VOLUME_TIME );
		controller.SoundChangeVolume( m_pFanMaxSpeedSound, RemapValClamped( speedRatio, 0.8, 1.0, FAN_MIN_VOLUME, FAN_MAX_VOLUME ), FAN_CHANGE_VOLUME_TIME );

		if (!m_bFadeOutFan)
		{
			m_bFadeOutFan = true;
			controller.SoundChangeVolume( m_pFanSound, FAN_DUCK_VOLUME, FAN_DUCK_TIME );
		}
	}
	else
	{
		m_bFadeOutFan = false;
		controller.SoundChangeVolume( m_pFanSound, RemapValClamped( fabs(m_flThrottle), 0, 1.0, FAN_MIN_VOLUME, FAN_MAX_VOLUME ), 0.25 );
		controller.SoundChangeVolume( m_pFanMaxSpeedSound, 0.0, 0.0 );
	}

	controller.SoundChangePitch( m_pFanSound, 100 * (fabs(m_flThrottle) + 0.2), 0.25 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateWaterSound( CSoundEnvelopeController &controller, float speedRatio )
{
	int nWaterLevel = GetWaterLevel();

	// Manage the state of the water stopped sound (gentle lapping at the pontoons).
	if ( nWaterLevel == 0 )
	{
		controller.SoundChangeVolume(m_pWaterStoppedSound, 0.0, 0.0);
	}
	else
	{
		if ( m_nPrevWaterLevel == 0 )
		{
			Vector vecVelocityWorld;
			GetVelocity( &vecVelocityWorld, NULL );

			if ( ( fabs( vecVelocityWorld.x ) > 400 ) || ( fabs( vecVelocityWorld.y ) > 400 ) || ( fabs( vecVelocityWorld.z ) > 400 ) )
			{
				// Landed in the water. Play a splash sound.
				EmitSound( "Airboat_impact_splash" );

				if ( fabs( vecVelocityWorld.z ) > 200 )
				{
					// Landed hard in the water. Play a smack sound.
					EmitSound( "Airboat_impact_hard" );
				}
			}
		}

		if (speedRatio <= 0.1)
		{
			if (!controller.SoundGetVolume(m_pWaterStoppedSound))
			{
				// Fade in the water stopped sound over 2 seconds.
				controller.SoundChangeVolume(m_pWaterStoppedSound, 1.0, 2.0);
				m_flWaterStoppedPitchTime = gpGlobals->curtime + random->RandomFloat(1.0, 3.0);
			}
			else if (gpGlobals->curtime > m_flWaterStoppedPitchTime)
			{
				controller.SoundChangeVolume(m_pWaterStoppedSound, random->RandomFloat(0.2, 1.0), random->RandomFloat(1.0, 3.0));
				controller.SoundChangePitch(m_pWaterStoppedSound, random->RandomFloat(90, 110), random->RandomFloat(1.0, 3.0));
				m_flWaterStoppedPitchTime = gpGlobals->curtime + random->RandomFloat(2.0, 4.0);
			}
		}
		else
		{
			if (controller.SoundGetVolume(m_pWaterStoppedSound))
			{
				// Fade out the water stopped sound over 1 second.
				controller.SoundChangeVolume(m_pWaterStoppedSound, 0.0, 1.0);
			}
		}
	}

	// Manage the state of the water fast sound (water hissing under the pontoons).
	if ( nWaterLevel == 0 )
	{
		controller.SoundChangeVolume(m_pWaterFastSound, 0.0, 0.0);
	}
	else
	{
		controller.SoundChangeVolume( m_pWaterFastSound, speedRatio, 0.0 );
	}

	m_nPrevWaterLevel = nWaterLevel;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateSound()
{
	if (!GetDriver())
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// Sample the data that we need for sounds.
	CFourWheelVehiclePhysics *pPhysics = GetPhysics();
	int speed = pPhysics->GetSpeed();
	int maxSpeed = pPhysics->GetMaxSpeed();
	float speedRatio = clamp((float)speed / (float)maxSpeed, 0, 1);

	//Msg("speedRatio=%f\n", speedRatio);

	UpdateWeaponSound();
	UpdateEngineSound( controller, speedRatio );
	UpdateFanSound( controller, speedRatio );
	UpdateWaterSound( controller, speedRatio );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateSplashEffects( void )
{
	// Splash effects.
	CreateSplash( AIRBOAT_SPLASH_RIPPLE );
//	CreateSplash( AIRBOAT_SPLASH_SPRAY );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPropAirboat::GetTracerType( void ) 
{
	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
		return "AirboatGunHeavyTracer";

	return "AirboatGunTracer"; 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::DoMuzzleFlash( void )
{
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = m_nGunBarrelAttachment;
	data.m_flScale = 1.0f;
	DispatchEffect( "AirboatMuzzleFlash", data );

	BaseClass::DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define GUN_WINDUP_TIME 1.5f

// NVNT Convar for airboat gun magnitude
ConVar hap_airboat_gun_mag("hap_airboat_gun_mag", "3", 0);

void CPropAirboat::FireGun( )
{
	// Get the gun position.
	Vector	vecGunPosition;
	Vector vecForward;
	GetAttachment( m_nGunBarrelAttachment, vecGunPosition, &vecForward );
	
	// NOTE: For the airboat, unable to fire really means the aim is clamped
	Vector vecAimPoint;
	if ( !m_bUnableToFire )
	{
		// Trace from eyes and see what we hit.
		ComputeAimPoint( &vecAimPoint );
	}
	else
	{
		// We hit the clamp; just fire whichever way the gun is facing
		VectorMA( vecGunPosition, 1000.0f, vecForward, vecAimPoint );
	}
	
	// Get a ray from the gun to the target.
	Vector vecRay = vecAimPoint - vecGunPosition;
	VectorNormalize( vecRay );
	
	/*
	// Get the aiming direction
	Vector vecRay;
	AngleVectors( vecGunAngles, &vecRay );
	VectorNormalize( vecRay );
	*/
	
	CAmmoDef *pAmmoDef = GetAmmoDef();
	int ammoType = pAmmoDef->Index( "AirboatGun" );

#if defined( WIN32 ) && !defined( _X360 ) 
	// NVNT punch the players haptics by the magnitude cvar each round fired
	HapticPunch(m_hPlayer,0,0,hap_airboat_gun_mag.GetFloat());
#endif

	FireBulletsInfo_t info;
	info.m_vecSrc = vecGunPosition;
	info.m_vecDirShooting = vecRay;
	info.m_flDistance = 4096;
	info.m_iAmmoType = ammoType;
	info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;

	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
	{
		info.m_iShots = 1;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_flDamageForceScale = 1000.0f;
	}
	else
	{
		info.m_iShots = 2;
		info.m_vecSpread = VECTOR_CONE_5DEGREES;
	}

	FireBullets( info );

	CBaseEntity *pDriver = GetDriver();
	CBasePlayer *pPlayerDriver;
	if( pDriver && pDriver->IsPlayer() )
	{
		pPlayerDriver = dynamic_cast<CBasePlayer*>(pDriver);
		if( pPlayerDriver )
		{
			pPlayerDriver->RumbleEffect( RUMBLE_AIRBOAT_GUN, 0, RUMBLE_FLAG_LOOP|RUMBLE_FLAG_ONLYONE );
		}
	}

	DoMuzzleFlash();

	// NOTE: This must occur after FireBullets
	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
	{
		m_flNextHeavyShotTime = gpGlobals->curtime + CANNON_HEAVY_SHOT_INTERVAL; 
	}

	if ( gpGlobals->curtime >= m_flNextGunShakeTime )
	{
		UTIL_ScreenShakeObject( this, WorldSpaceCenter(), 0.2, 250.0, CANNON_SHAKE_INTERVAL, 250, SHAKE_START );
		m_flNextGunShakeTime = gpGlobals->curtime + 0.5 * CANNON_SHAKE_INTERVAL; 
	}

	// Specifically kill APC missiles in the cone. But we're going to totally cheat
	// because it's hard to hit them when they are close. 
	// Use the player's eye position as the center of the cone.
	if ( !m_hPlayer )
		return;

	Vector vecEyeDirection, vecEyePosition;
	if ( !m_bUnableToFire )
	{
		if ( IsX360() )
		{
			GetAttachment( m_nGunBarrelAttachment, vecEyePosition, &vecEyeDirection );
		}
		else
		{
			vecEyePosition = m_hPlayer->EyePosition();
			m_hPlayer->EyeVectors( &vecEyeDirection, NULL, NULL );
		}
	}
	else
	{
		vecEyePosition = vecGunPosition;
		vecEyeDirection = vecRay;
	}

	CAPCMissile *pEnt = FindAPCMissileInCone( vecEyePosition, vecEyeDirection, 2.5f );
	if ( pEnt && (pEnt->GetHealth() > 0) )
	{
		CTakeDamageInfo info( this, this, 1, DMG_AIRBOAT );
		CalculateBulletDamageForce( &info, ammoType, vecRay, pEnt->WorldSpaceCenter() );
		pEnt->TakeDamage( info );

		Vector vecVelocity = pEnt->GetAbsVelocity();

		// Pick a vector perpendicular to the vecRay which will push it away from the airboat
		Vector vecPerp;
		CrossProduct( Vector( 0, 0, 1 ), vecRay, vecPerp );
		vecPerp.z = 0.0f;
		if ( VectorNormalize( vecPerp ) > 1e-3 )
		{
			Vector vecCurrentDir;
			GetVectors( &vecCurrentDir, NULL, NULL );
			if ( DotProduct( vecPerp, vecCurrentDir ) > 0.0f )
			{
				vecPerp *= -1.0f;
			}

			vecPerp *= random->RandomFloat( 15, 25 );
			vecVelocity += vecPerp;
			pEnt->SetAbsVelocity( vecVelocity );
//			pEnt->DisableGuiding();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define FIRING_DISCHARGE_RATE (1.0f / 3.0f)

void CPropAirboat::UpdateGunState( CUserCmd *ucmd )
{
	bool bStopRumble = false;

	if ( ucmd->buttons & IN_ATTACK )
	{
		if ( m_nGunState == GUN_STATE_IDLE )
		{
//			AddGestureSequence( LookupSequence( "fire_gun" ) );
			m_nGunState = GUN_STATE_FIRING;
		}

		if ( m_nAmmoCount > 0 )
		{
			RemoveAmmo( FIRING_DISCHARGE_RATE );
			FireGun( );

			if ( m_nAmmoCount == 0 )
			{
				EmitSound( "Airboat.FireGunRevDown" );
				bStopRumble = true;
//				RemoveAllGestures();
			}
		}
	}
	else
	{
		if ( m_nGunState != GUN_STATE_IDLE )
		{
			if ( m_nAmmoCount != 0 )
			{
				EmitSound( "Airboat.FireGunRevDown" );
				bStopRumble = true;
//				RemoveAllGestures();
			}
			m_nGunState = GUN_STATE_IDLE;
		}
	}

	if( bStopRumble )
	{
		CBaseEntity *pDriver = GetDriver();
		CBasePlayer *pPlayerDriver;
		if( pDriver && pDriver->IsPlayer() )
		{
			pPlayerDriver = dynamic_cast<CBasePlayer*>(pDriver);
			if( pPlayerDriver )
			{
				pPlayerDriver->RumbleEffect( RUMBLE_AIRBOAT_GUN, 0, RUMBLE_FLAG_STOP );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	if ( ucmd->impulse == 100 )
	{
		if (HeadlightIsOn())
		{
			HeadlightTurnOff();
		}
        else 
		{
			HeadlightTurnOn();
		}
	}

	// Fire gun.
	if ( HasGun() )
	{
		UpdateGunState( ucmd );
	}

	m_VehiclePhysics.UpdateDriverControls( ucmd, TICK_INTERVAL );

	// Create splashes.
	UpdateSplashEffects();

	// Save this data.
	m_flThrottle = m_VehiclePhysics.GetThrottle();
	m_nSpeed = m_VehiclePhysics.GetSpeed();
	m_nRPM = m_VehiclePhysics.GetRPM();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropAirboat::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	if ( gpGlobals->frametime != 0 )
	{
		// Create danger sounds in front of the vehicle.
		CreateDangerSounds();

		// Play a sound around us to make NPCs pay attention to us
		if ( m_VehiclePhysics.GetThrottle() > 0 )
		{
			CSoundEnt::InsertSound( SOUND_PLAYER_VEHICLE, pPlayer->GetAbsOrigin(), 3500, 0.1f, pPlayer, SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER );
		}
	}

	Vector vecVelocityWorld;
	GetVelocity( &vecVelocityWorld, NULL );
	Vector vecVelocityLocal;
	WorldToEntitySpace( GetAbsOrigin() + vecVelocityWorld, &vecVelocityLocal );
	
	m_vecPhysVelocity = vecVelocityLocal;
}


//-----------------------------------------------------------------------------
// Purpose: Create danger sounds in front of the vehicle.
//-----------------------------------------------------------------------------
void CPropAirboat::CreateDangerSounds( void )
{
	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir, vecRight;

	GetVectors( &vecDir, &vecRight, NULL );

	const float soundDuration = 0.25;
	float speed = m_VehiclePhysics.GetHLSpeed();

	// Make danger sounds ahead of the vehicle
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

		// 0.7 seconds ahead
		vecSpot = vecStart + vecDir * (speed * 0.7f);
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, radius, soundDuration, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
		CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, vecSpot, radius, soundDuration, this, SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER );
		//NDebugOverlay::Box(vecSpot, Vector(-radius,-radius,-radius),Vector(radius,radius,radius), 255, 0, 255, 0, soundDuration);

#if 0
		// put sounds a bit to left and right but slightly closer to vehicle to make
		// a "cone" of sound  in front of it.
		trace_t	tr;
		vecSpot = vecStart + vecDir * (speed * 0.5f) - vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 1 );

		vecSpot = vecStart + vecDir * (speed * 0.5f) + vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 2);
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;
	if ( flFrameTime < AIRBOAT_FRAMETIME_MIN )
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
void CPropAirboat::ComputePDControllerCoefficients( float *pCoefficientsOut,
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
void CPropAirboat::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
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
	if ( vecDeltaPos.Length() > AIRBOAT_DELTA_LENGTH_MAX )
	{
		float flSign = vecForward.Dot( vecVehicleEyeSpeed ) >= 0.0f ? -1.0f : 1.0f;
		vecVehicleEyePos += flSign * ( vecForward * AIRBOAT_DELTA_LENGTH_MAX );
		m_vecLastEyePos = vecVehicleEyePos;
		m_vecEyeSpeed = vecVehicleEyeSpeed;
		return;
	}

	// Generate an updated (dampening) speed for use in next frames position extrapolation.
	float flCoefficients[2];
	ComputePDControllerCoefficients( flCoefficients, r_AirboatViewDampenFreq.GetFloat(), r_AirboatViewDampenDamp.GetFloat(), flFrameTime );
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
void CPropAirboat::DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get up vector.
	Vector vecUp;
	AngleVectors( vecVehicleEyeAngles, NULL, NULL, &vecUp );
	vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
	vecVehicleEyePos.z += r_AirboatViewZHeight.GetFloat() * vecUp.z;

	// NOTE: Should probably use some damped equation here.
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::CreateSplash( int nSplashType )
{
	if ( GetWaterLevel( ) == 0 )
		return;

	Vector vecSplashPoint;
	Vector vecForward, vecUp;
	GetAttachment( m_nSplashAttachment, vecSplashPoint, &vecForward, &vecUp, NULL );

	CEffectData	data;
	data.m_fFlags = 0;
	data.m_vOrigin = vecSplashPoint;
	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	switch ( nSplashType )
	{
		case AIRBOAT_SPLASH_SPRAY:
		{
			Vector vecSplashDir;
			vecSplashDir = ( vecForward + vecUp ) * 0.5f;
			VectorNormalize( vecSplashDir );
			data.m_vNormal = vecSplashDir;
			data.m_flScale = 10.0f + random->RandomFloat( 0, 10.0f * 0.25 );
			//DispatchEffect( "waterripple", data );
			DispatchEffect( "watersplash", data );
		}
		case AIRBOAT_SPLASH_RIPPLE:
		{
			/*
			Vector vecSplashDir;
			vecSplashDir = vecUp;
			data.m_vNormal = vecSplashDir;
			data.m_flScale = AIRBOAT_SPLASH_RIPPLE_SIZE + random->RandomFloat( 0, AIRBOAT_SPLASH_RIPPLE_SIZE * 0.25 );
			DispatchEffect( "waterripple", data );
			*/
		}
		default:
		{
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded to calculate stress damage.
//-----------------------------------------------------------------------------
void CPropAirboat::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	if ( airboat_fatal_stress.GetFloat() > 0 )
	{
		ApplyStressDamage( pPhysics );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the damage that should be dealt to the driver due to
//			stress (vphysics objects exerting pressure on us).
//-----------------------------------------------------------------------------
float CPropAirboat::CalculatePhysicsStressDamage( vphysics_objectstress_t *pStressOut, IPhysicsObject *pPhysics )
{
	vphysics_objectstress_t stressOut;
	CalculateObjectStress( pPhysics, this, &stressOut );

	//if ( ( stressOut.exertedStress > 100 ) || ( stressOut.receivedStress > 100 ) )
	//	Msg( "stress: %f %d %f\n", stressOut.exertedStress, stressOut.hasNonStaticStress, stressOut.receivedStress );

	// Make sure the stress isn't from being stuck inside some static object.
	// If we're being crushed by more than the fatal stress amount, kill the driver.
	if ( stressOut.hasNonStaticStress && ( stressOut.receivedStress > airboat_fatal_stress.GetFloat() ) )
	{
		// if stuck, don't do this!
		if ( !(pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING) )
		{
			// Kill the driver!
			return 1000;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Applies stress damage to the player/driver.
//-----------------------------------------------------------------------------
void CPropAirboat::ApplyStressDamage( IPhysicsObject *pPhysics )
{
	vphysics_objectstress_t stressOut;
	float damage = CalculatePhysicsStressDamage( &stressOut, pPhysics );
	if ( ( damage > 0 ) &&  ( m_hPlayer != NULL ) )
	{
		CTakeDamageInfo dmgInfo( GetWorldEntity(), GetWorldEntity(), vec3_origin, vec3_origin, damage, DMG_CRUSH );
		dmgInfo.SetDamageForce( Vector( 0, 0, -stressOut.receivedStress * GetCurrentGravity() * gpGlobals->frametime ) );
		dmgInfo.SetDamagePosition( GetAbsOrigin() );
		m_hPlayer->TakeDamage( dmgInfo );
	}
}

