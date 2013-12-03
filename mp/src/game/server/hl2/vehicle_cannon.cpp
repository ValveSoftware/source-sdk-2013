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
#include "soundenvelope.h"
#include "soundent.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vcollide_parse.h"
#include "ndebugoverlay.h"
#include "npc_vehicledriver.h"
#include "hl2_player.h"
#include "explode.h"
#include "particle_smokegrenade.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPropCannon;

// Crane bones that have physics followers
static const char *pCannonFollowerBoneNames[] =
{
	"base",
	"arm",
	"platform",
};

#define CANNON_EXTENSION_RATE_MAX	0.01
#define CANNON_TURN_RATE_MAX			1.2

#define MAX_CANNON_FLAT_REACH		1400.0
#define MIN_CANNON_FLAT_REACH		700.0
#define CANNON_EXTENSION_ACCEL		0.006
#define CANNON_EXTENSION_DECEL		0.02
#define CANNON_TURN_ACCEL			0.2
#define CANNON_DECEL					0.5

#define CANNON_SLOWRAISE_TIME		5.0

#define CANNON_PROJECTILE_MODEL "models/props_combine/headcrabcannister01a.mdl"

ConVar g_cannon_reloadtime( "g_cannon_reloadtime", "3", FCVAR_CHEAT | FCVAR_GAMEDLL );
ConVar g_cannon_max_traveltime( "g_cannon_max_traveltime", "1.5", FCVAR_CHEAT | FCVAR_GAMEDLL );
ConVar g_cannon_debug( "g_cannon_debug", "0", FCVAR_CHEAT | FCVAR_GAMEDLL );
ConVar g_cannon_damageandradius( "g_cannon_damageandradius", "512", FCVAR_CHEAT | FCVAR_GAMEDLL );

// Turning stats
enum
{
	CANNON_TURNING_NOT,
	CANNON_TURNING_LEFT,
	CANNON_TURNING_RIGHT,
};

//-----------------------------------------------------------------------------
// Purpose: Crane vehicle server
//-----------------------------------------------------------------------------
class CCannonServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	void	GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL );


protected:
	CPropCannon *GetCannon( void );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropCannon : public CBaseProp, public IDrivableVehicle
{
	DECLARE_CLASS( CPropCannon, CBaseProp );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CPropCannon( void )
	{
		m_ServerVehicle.SetVehicle( this );
	}

	//IDrivableVehicle's Pure Virtuals
	virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer );
	virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );
	virtual void		SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = bOn; }
	virtual void		SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = bOn; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	virtual void		EnterVehicle( CBaseCombatCharacter *pPassenger );
	virtual bool		AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole ) { return true; }
	virtual bool		AllowMidairExit( CBaseCombatCharacter *pPassenger, int nRole ) { return false; }
	virtual void		PreExitVehicle( CBaseCombatCharacter *pPassenger, int nRole ) {}
	virtual void		ExitVehicle( int nRole );
	virtual string_t GetVehicleScriptName() { return m_vehicleScript; }
	virtual bool		PassengerShouldReceiveDamage( CTakeDamageInfo &info ) { return false; }
	
	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle *GetServerVehicle() { return &m_ServerVehicle; }

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual int	 ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Think( void );
	
	virtual	bool CreateVPhysics( void );
	virtual void UpdateOnRemove( void );

	void	DriveCannon( int iDriverButtons, int iButtonsPressed );
	void	ResetUseKey( CBasePlayer *pPlayer );
	void	InitCannonSpeeds( void );
	void	RunCraneMovement( float flTime );
	void	LaunchProjectile( void );
	void	ProjectileExplosion( void );

private:
	
	string_t		m_vehicleScript;

	// Entering / Exiting
	bool				m_bLocked;
	
	CNetworkVar( bool,	m_bEnterAnimOn );
	CNetworkVar( bool,	m_bExitAnimOn );
	CNetworkVector(		m_vecEyeExitEndpoint );

	CNetworkHandle( CBasePlayer, m_hPlayer );
	
	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;

	int				m_iTurning;
	float			m_flTurn;

	// Crane arm extension / retraction
	bool			m_bExtending;
	float			m_flExtension;
	float			m_flExtensionRate;

	// Speeds
	float			m_flMaxExtensionSpeed;
	float			m_flMaxTurnSpeed;
	float			m_flExtensionAccel;
	float			m_flExtensionDecel;
	float			m_flTurnAccel;
	float			m_flTurnDecel;

	float			m_flFlyTime;
	Vector			m_vCrashPoint;

	float			m_flNextAttackTime;

protected:
	// Contained IServerVehicle
	CCannonServerVehicle		m_ServerVehicle;

	// Contained Bone Follower manager
	CBoneFollowerManager		m_BoneFollowerManager;
};

LINK_ENTITY_TO_CLASS( prop_vehicle_cannon, CPropCannon );

BEGIN_DATADESC( CPropCannon )
	DEFINE_KEYFIELD( m_vehicleScript, FIELD_STRING, "vehiclescript" ),
	DEFINE_OUTPUT( m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( m_playerOff, "PlayerOff" ),

	DEFINE_EMBEDDED( m_BoneFollowerManager ),

	DEFINE_FIELD( m_iTurning, FIELD_INTEGER ),
	DEFINE_FIELD( m_flTurn, FIELD_FLOAT ),
	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ), 

	DEFINE_FIELD( m_bExtending, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flExtension, FIELD_FLOAT ),
	DEFINE_FIELD( m_flExtensionRate, FIELD_FLOAT ),

	DEFINE_FIELD( m_flMaxExtensionSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxTurnSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flExtensionAccel, FIELD_FLOAT ),
	DEFINE_FIELD( m_flExtensionDecel, FIELD_FLOAT ),

	DEFINE_FIELD( m_flTurnAccel, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTurnDecel, FIELD_FLOAT ),

	DEFINE_FIELD( m_flFlyTime, FIELD_TIME ),
	DEFINE_FIELD( m_vCrashPoint, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_flNextAttackTime, FIELD_TIME ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CPropCannon, DT_PropCannon)
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropBool(SENDINFO(m_bEnterAnimOn)),
	SendPropBool(SENDINFO(m_bExitAnimOn)),
	SendPropVector(SENDINFO(m_vecEyeExitEndpoint), -1, SPROP_COORD),
END_SEND_TABLE();

//------------------------------------------------
// Precache
//------------------------------------------------
void CPropCannon::Precache( void )
{
	BaseClass::Precache();
	m_ServerVehicle.Initialize( STRING( m_vehicleScript ) );

	PrecacheModel( CANNON_PROJECTILE_MODEL );
	
	PrecacheScriptSound( "HeadcrabCanister.LaunchSound" );
	PrecacheScriptSound( "HeadcrabCanister.Explosion" );
	PrecacheScriptSound( "Weapon_Mortar.Incomming" );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropCannon::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NOCLIP );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_flTurn = 0;
	m_flExtension = 0;

	m_flFlyTime = 0.0f;
	m_flNextAttackTime = gpGlobals->curtime;

	InitCannonSpeeds();
	
	SetPoseParameter( "armextensionpose", m_flExtension );

	CreateVPhysics();
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::InitCannonSpeeds( void )
{
	m_flMaxExtensionSpeed = CANNON_EXTENSION_RATE_MAX * 2;
	m_flMaxTurnSpeed = CANNON_TURN_RATE_MAX * 2;
	m_flExtensionAccel = CANNON_EXTENSION_ACCEL * 2;
	m_flExtensionDecel = CANNON_EXTENSION_DECEL * 2;
	m_flTurnAccel = CANNON_TURN_ACCEL * 2;
	m_flTurnDecel = CANNON_DECEL * 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPropCannon::GetDriver( void ) 
{ 
	return m_hPlayer; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	if ( pPassenger == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Remove any player who may be in the vehicle at the moment
		if ( m_hPlayer )
		{
			ExitVehicle( VEHICLE_ROLE_DRIVER );
		}

		m_hPlayer = pPlayer;
		m_playerOn.FireOutput( pPlayer, this, 0 );

		//m_ServerVehicle.SoundStart();
	}
	else
	{
		// NPCs not supported yet - jdw
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::ExitVehicle( int nRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );
	m_playerOff.FireOutput( pPlayer, this, 0 );
	m_bEnterAnimOn = false;

	//m_ServerVehicle.SoundShutdown( 1.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropCannon::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;
	
	// Prevent entering if the vehicle's locked
	return ( !m_bLocked );
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropCannon::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or rotating
	// Adrian: Check also if I'm currently jumping in or out.
	return ( !m_bLocked && (GetLocalAngularVelocity() == vec3_angle) && m_bExitAnimOn == false && m_bEnterAnimOn == false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

//-----------------------------------------------------------------------------
// Purpose: Pass player movement into the crane's driving system
//-----------------------------------------------------------------------------
void CPropCannon::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If the player's entering/exiting the vehicle, prevent movement
	if ( !m_bEnterAnimOn && !m_bExitAnimOn )
	{
		DriveCannon( ucmd->buttons, player->m_afButtonPressed );
	}

	// Run the crane's movement
	RunCraneMovement( gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: Crane rotates around with +left and +right, and extends/retracts 
//			the cable with +forward and +back.
//-----------------------------------------------------------------------------
void CPropCannon::DriveCannon( int iDriverButtons, int iButtonsPressed )
{
	bool bWasExtending = m_bExtending;

	// Handle rotation of the crane
	if ( iDriverButtons & IN_MOVELEFT )
	{
		// Try adding some randomness to make it feel shaky? 
		float flTurnAdd = m_flTurnAccel;
		// If we're turning back on ourselves, use decel speed
		if ( m_flTurn < 0 )
		{
			flTurnAdd = MAX( flTurnAdd, m_flTurnDecel );
		}

		m_flTurn = UTIL_Approach( m_flMaxTurnSpeed, m_flTurn, flTurnAdd * gpGlobals->frametime );

		m_iTurning = CANNON_TURNING_LEFT;
	}
	else if ( iDriverButtons & IN_MOVERIGHT )
	{
		// Try adding some randomness to make it feel shaky?
		float flTurnAdd = m_flTurnAccel;
		// If we're turning back on ourselves, increase the rate
		if ( m_flTurn > 0 )
		{
			flTurnAdd = MAX( flTurnAdd, m_flTurnDecel );
		}
		m_flTurn = UTIL_Approach( -m_flMaxTurnSpeed, m_flTurn, flTurnAdd * gpGlobals->frametime );

		m_iTurning = CANNON_TURNING_RIGHT;
	}
	else
	{
		m_flTurn = UTIL_Approach( 0, m_flTurn, m_flTurnDecel * gpGlobals->frametime );
		m_iTurning = CANNON_TURNING_NOT;
	}

	SetLocalAngularVelocity( QAngle(0,m_flTurn * 10,0) );

	// Handle extension / retraction of the arm
	if ( iDriverButtons & IN_FORWARD )
	{
		m_flExtensionRate = UTIL_Approach( m_flMaxExtensionSpeed, m_flExtensionRate, m_flExtensionAccel * gpGlobals->frametime );
		m_bExtending = true;
	}
	else if ( iDriverButtons & IN_BACK )
	{
		m_flExtensionRate = UTIL_Approach( -m_flMaxExtensionSpeed, m_flExtensionRate, m_flExtensionAccel * gpGlobals->frametime );
		m_bExtending = true;
	}
	else
	{
		m_flExtensionRate = UTIL_Approach( 0, m_flExtensionRate, m_flExtensionDecel * gpGlobals->frametime );
		m_bExtending = false;
	}

	//Msg("Turn: %f\nExtensionRate: %f\n", m_flTurn, m_flExtensionRate );

	//If we're holding down an attack button, update our state
	if ( iButtonsPressed & (IN_ATTACK | IN_ATTACK2) )
	{
		if ( m_flNextAttackTime <= gpGlobals->curtime )
		{
			LaunchProjectile();
		}
	}

	float flSpeedPercentage = clamp( fabs(m_flTurn) / m_flMaxTurnSpeed, 0, 1 );
	vbs_sound_update_t params;
	params.Defaults();
	params.bThrottleDown = (m_iTurning != CANNON_TURNING_NOT);
	params.flCurrentSpeedFraction = flSpeedPercentage;
	params.flWorldSpaceSpeed = 0;

	m_ServerVehicle.SoundUpdate( params );

	// Play sounds for arm extension / retraction
	if ( m_bExtending && !bWasExtending )
	{
		m_ServerVehicle.StopSound( VS_ENGINE2_STOP );
		m_ServerVehicle.PlaySound( VS_ENGINE2_START );
	}
	else if ( !m_bExtending && bWasExtending )
	{
		m_ServerVehicle.StopSound( VS_ENGINE2_START );
		m_ServerVehicle.PlaySound( VS_ENGINE2_STOP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropCannon::RunCraneMovement( float flTime )
{
	if ( m_flExtensionRate )
	{
		// Extend / Retract the crane
		m_flExtension = clamp( m_flExtension + (m_flExtensionRate * 10 * flTime), 0, 2 );
		SetPoseParameter( "armextensionpose", m_flExtension );
		StudioFrameAdvance();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CPropCannon::ItemPostFrame( CBasePlayer *player )
{

}

void CPropCannon::ProjectileExplosion( void )
{
	ExplosionCreate( m_vCrashPoint, vec3_angle, NULL, 512, 512, false );

	// do damage
	CTakeDamageInfo info( this, this, g_cannon_damageandradius.GetInt(), DMG_BLAST );

	info.SetDamagePosition( m_vCrashPoint );
	RadiusDamage( info, m_vCrashPoint, g_cannon_damageandradius.GetInt(), CLASS_NONE, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( GetDriver() )
	{
		BaseClass::Think();
	}

	if ( GetDriver() )
	{
		BaseClass::Think();
	
		// play enter animation
		StudioFrameAdvance();

		// If the enter or exit animation has finished, tell the server vehicle
		if ( IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn) )
		{
			if ( m_bEnterAnimOn )
			{
				// Finished entering, display the hint for using the crane
				//UTIL_HudHintText( m_hPlayer, "#Valve_Hint_CraneKeys" );
			}
			
			GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, true );
		}
	}
	else
	{
		// Run the crane's movement
	//	RunCraneMovement( 0.1 );
	}

	// Update follower bones
	m_BoneFollowerManager.UpdateBoneFollowers(this);

	if ( m_flFlyTime > 0.0f )
	{
		if ( m_flFlyTime - 1.0f <= gpGlobals->curtime && m_flFlyTime - 0.8f > gpGlobals->curtime)
		{
			CPASAttenuationFilter filter( this );
				
			EmitSound_t ep;
			ep.m_nChannel = CHAN_STATIC;
			ep.m_pSoundName = "Weapon_Mortar.Incomming";
			ep.m_flVolume = 255;
			ep.m_SoundLevel = SNDLVL_180dB;
	
			EmitSound( filter, entindex(), ep );
		}

		if ( m_flFlyTime <= gpGlobals->curtime )
		{
			if ( m_vCrashPoint != vec3_origin )
			{
				ProjectileExplosion();
			}

			CEffectData	data;

			data.m_vOrigin = m_vCrashPoint;
			data.m_flScale = 512;
			DispatchEffect( "ThumperDust", data );

			m_flFlyTime = 0.0f;
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	GetServerVehicle()->HandlePassengerEntry( pPlayer, (value>0) );
}

void CPropCannon::LaunchProjectile( void )
{
	//ADRIANTODO: Come back to this once we get the right model and remove all the fix ups caused by temp content.

	Vector vTipPos, vTipForward, vTipRight, vUp;

	GetAttachment( "cable_tip", vTipPos, &vTipForward, &vTipRight, &vUp );

	bool bCollided = false;
	bool bInSky = false;
	float gravity = -gpGlobals->frametime * 600;
	Vector vOrigin = vTipPos;
	Vector vVelocity = vTipRight * 2500;

	float flDistance = 0.0f;

	int iFailSafe = 0;
	
	while ( bCollided == false && iFailSafe < 100000 )
	{
		Vector vOldOrigin = vOrigin;
		vOrigin = vOrigin + vVelocity * gpGlobals->frametime;

		flDistance += (vOrigin - vOldOrigin).Length();

		if ( g_cannon_debug.GetBool() == true )
		{
			NDebugOverlay::Line( vOldOrigin, vOrigin, 0, 255, 0, true, 5 );
		}

		trace_t pm;		
		UTIL_TraceLine( vOldOrigin, vOrigin, MASK_SOLID, this, COLLISION_GROUP_NONE, &pm );

		if ( pm.surface.flags & SURF_SKY || pm.allsolid == true ) 
		{
			bInSky = true;
			iFailSafe++;
		}
		else
		{
			bInSky = false;
		}

		iFailSafe++;

		if ( pm.fraction != 1.0f && bInSky == false )
		{
			bCollided = true;
			vOrigin = pm.endpos;

			if ( g_cannon_debug.GetBool() == true )
			{
				NDebugOverlay::Box( vOrigin, Vector( 256, 256, 256 ), Vector( -256, -256, -256 ), 255, 0, 0, 0, 5 );
			}
		}
		else
		{
			vVelocity[2] += gravity;
		}
	}
	
	float flTravelTime = flDistance / vVelocity.Length();

	if ( flTravelTime > g_cannon_max_traveltime.GetFloat() )
	{
		flTravelTime = g_cannon_max_traveltime.GetFloat();

		if ( bCollided == false )
		{
			vOrigin = vec3_origin; 
		}
	}

	m_flFlyTime = gpGlobals->curtime + flTravelTime;
	m_vCrashPoint = vOrigin;

	m_flNextAttackTime = gpGlobals->curtime + g_cannon_reloadtime.GetFloat();
	
	EmitSound( "HeadcrabCanister.LaunchSound" );

	UTIL_ScreenShake( GetDriver()->GetAbsOrigin(), 50.0, 150.0, 1.0, 750, SHAKE_START, true );
}

//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================
CPropCannon *CCannonServerVehicle::GetCannon( void )
{
	return (CPropCannon*)GetDrivableVehicle();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropCannon::CreateVPhysics( void )
{
	BaseClass::CreateVPhysics();
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pCannonFollowerBoneNames), pCannonFollowerBoneNames );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCannon::UpdateOnRemove( void )
{
	m_BoneFollowerManager.DestroyBoneFollowers();
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCannonServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*= NULL*/ )
{
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CBasePlayer *pPlayer = ToBasePlayer( GetDrivableVehicle()->GetDriver() );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.

	float flPitchFactor = 1.0;
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetCannon()->GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

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
