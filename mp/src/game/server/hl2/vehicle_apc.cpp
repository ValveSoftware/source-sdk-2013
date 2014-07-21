//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vehicle_apc.h"
#include "ammodef.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "weapon_rpg.h"
#include "in_buttons.h"
#include "globalstate.h"
#include "soundent.h"
#include "ai_basenpc.h"
#include "ndebugoverlay.h"
#include "gib.h"
#include "EntityFlame.h"
#include "smoke_trail.h"
#include "explode.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ROCKET_ATTACK_RANGE_MAX 5500.0f
#define ROCKET_ATTACK_RANGE_MIN 1250.0f

#define MACHINE_GUN_ATTACK_RANGE_MAX 1250.0f
#define MACHINE_GUN_ATTACK_RANGE_MIN 0.0f

#define MACHINE_GUN_MAX_UP_PITCH	30
#define MACHINE_GUN_MAX_DOWN_PITCH	10
#define MACHINE_GUN_MAX_LEFT_YAW	30
#define MACHINE_GUN_MAX_RIGHT_YAW	30

#define MACHINE_GUN_BURST_SIZE		10
#define MACHINE_GUN_BURST_TIME		0.075f
#define MACHINE_GUN_BURST_PAUSE_TIME	2.0f

#define ROCKET_SALVO_SIZE				5
#define ROCKET_DELAY_TIME				1.5
#define ROCKET_MIN_BURST_PAUSE_TIME		3
#define ROCKET_MAX_BURST_PAUSE_TIME		4
#define ROCKET_SPEED					800
#define DEATH_VOLLEY_ROCKET_COUNT		4
#define DEATH_VOLLEY_MIN_FIRE_TIME		0.333
#define DEATH_VOLLEY_MAX_FIRE_TIME		0.166

extern short g_sModelIndexFireball; // Echh...


ConVar sk_apc_health( "sk_apc_health", "750" );


#define APC_MAX_CHUNKS	3
static const char *s_pChunkModelName[APC_MAX_CHUNKS] = 
{
	"models/gibs/helicopter_brokenpiece_01.mdl",
	"models/gibs/helicopter_brokenpiece_02.mdl",
	"models/gibs/helicopter_brokenpiece_03.mdl",
};

#define APC_MAX_GIBS	6
static const char *s_pGibModelName[APC_MAX_GIBS] = 
{
	"models/combine_apc_destroyed_gib01.mdl",
	"models/combine_apc_destroyed_gib02.mdl",
	"models/combine_apc_destroyed_gib03.mdl",
	"models/combine_apc_destroyed_gib04.mdl",
	"models/combine_apc_destroyed_gib05.mdl",
	"models/combine_apc_destroyed_gib06.mdl",
};


LINK_ENTITY_TO_CLASS( prop_vehicle_apc, CPropAPC );


BEGIN_DATADESC( CPropAPC )

	DEFINE_FIELD( m_flDangerSoundTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flHandbrakeTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bInitialHandbrake,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nSmokeTrailCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flMachineGunTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iMachineGunBurstLeft,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nMachineGunMuzzleAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nMachineGunBaseAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_vecBarrelPos,		FIELD_VECTOR ),
	DEFINE_FIELD( m_bInFiringCone,		FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_hLaserDot,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRocketTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_iRocketSalvoLeft,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flRocketTime,		FIELD_TIME ),
//	DEFINE_FIELD( m_nRocketAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nRocketSide,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hSpecificRocketTarget, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_strMissileHint,	FIELD_STRING, "missilehint" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Destroy", InputDestroy ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FireMissileAt", InputFireMissileAt ),

	DEFINE_OUTPUT( m_OnDeath,				"OnDeath" ),
	DEFINE_OUTPUT( m_OnFiredMissile,		"OnFiredMissile" ),
	DEFINE_OUTPUT( m_OnDamaged,				"OnDamaged" ),
	DEFINE_OUTPUT( m_OnDamagedByPlayer,		"OnDamagedByPlayer" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Precache( void )
{
	BaseClass::Precache();

	int i;
	for ( i = 0; i < APC_MAX_CHUNKS; ++i )
	{
		PrecacheModel( s_pChunkModelName[i] );
	}

	for ( i = 0; i < APC_MAX_GIBS; ++i )
	{
		PrecacheModel( s_pGibModelName[i] );
	}

	PrecacheScriptSound( "Weapon_AR2.Single" );
	PrecacheScriptSound( "PropAPC.FireRocket" );
	PrecacheScriptSound( "combine.door_lock" );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropAPC::Spawn( void )
{
	BaseClass::Spawn();
	SetBlocksLOS( true );
	m_iHealth = m_iMaxHealth = sk_apc_health.GetFloat();
	SetCycle( 0 );
	m_iMachineGunBurstLeft = MACHINE_GUN_BURST_SIZE;
	m_iRocketSalvoLeft = ROCKET_SALVO_SIZE;
	m_nRocketSide = 0;
	m_lifeState = LIFE_ALIVE;
	m_bInFiringCone = false;

	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;

	// Reset the gun to a default pose.
	SetPoseParameter( "vehicle_weapon_pitch", 0 );
	SetPoseParameter( "vehicle_weapon_yaw", 90 );

	CreateAPCLaserDot();

	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		AddFlag( FL_AIMTARGET );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a laser
//-----------------------------------------------------------------------------
void CPropAPC::CreateAPCLaserDot( void )
{
	// Create a laser if we don't have one
	if ( m_hLaserDot == NULL )
	{
		m_hLaserDot = CreateLaserDot( GetAbsOrigin(), this, false );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CPropAPC::ShouldAttractAutoAim( CBaseEntity *pAimingEnt )
{
	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE && pAimingEnt->IsPlayer() && GetDriver() )
	{
		return true;
	}

	return BaseClass::ShouldAttractAutoAim( pAimingEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Activate()
{
	BaseClass::Activate();

	m_nRocketAttachment = LookupAttachment( "cannon_muzzle" );
	m_nMachineGunMuzzleAttachment = LookupAttachment( "muzzle" );
	m_nMachineGunBaseAttachment = LookupAttachment( "gun_base" );

	// NOTE: gun_ref must have the same position as gun_base, but rotates with the gun
	int nMachineGunRefAttachment = LookupAttachment( "gun_def" );

	Vector vecWorldBarrelPos;
	matrix3x4_t matRefToWorld;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecWorldBarrelPos );
	GetAttachment( nMachineGunRefAttachment, matRefToWorld );
	VectorITransform( vecWorldBarrelPos, matRefToWorld, m_vecBarrelPos );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::UpdateOnRemove( void )
{
	if ( m_hLaserDot )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	m_pServerVehicle = new CAPCFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
Class_T	CPropAPC::ClassifyPassenger( CBaseCombatCharacter *pPassenger, Class_T defaultClassification )
{ 
	return CLASS_COMBINE;	
}


//-----------------------------------------------------------------------------
// Purpose: Damage events as modified for the passenger of the APC, not the APC itself
//-----------------------------------------------------------------------------
float CPropAPC::PassengerDamageModifier( const CTakeDamageInfo &info ) 
{ 
	CTakeDamageInfo DmgInfo = info;

	// bullets, slashing and headbutts don't hurt us in the apc, neither do rockets
	if( (DmgInfo.GetDamageType() & DMG_BULLET) || (DmgInfo.GetDamageType() & DMG_SLASH) ||
		(DmgInfo.GetDamageType() & DMG_CLUB) || (DmgInfo.GetDamageType() & DMG_BLAST) )
		return (0);

	// Accept everything else by default
	return 1.0; 
}


//-----------------------------------------------------------------------------
// position of eyes
//-----------------------------------------------------------------------------
Vector CPropAPC::EyePosition( )
{
	Vector vecEyePosition;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5, 0.5, 1.0 ), &vecEyePosition );
	return vecEyePosition;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CPropAPC::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		return WorldSpaceCenter();
	}

	return BaseClass::BodyTarget( posSrc, bNoisy );
}
	
//-----------------------------------------------------------------------------
// Add a smoke trail since we've taken more damage
//-----------------------------------------------------------------------------
void CPropAPC::AddSmokeTrail( const Vector &vecPos )
{
	// Start this trail out with a bang!
	ExplosionCreate( vecPos, vec3_angle, this, 1000, 500.0f, SF_ENVEXPLOSION_NODAMAGE | 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | 
		SF_ENVEXPLOSION_NOFIREBALLSMOKE, 0 );
	UTIL_ScreenShake( vecPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	if ( m_nSmokeTrailCount == MAX_SMOKE_TRAILS )
		return;

	SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
	if( !pSmokeTrail )
		return;

	// See if there's an attachment for this smoke trail
	char buf[32];
	Q_snprintf( buf, 32, "damage%d", m_nSmokeTrailCount );
	int nAttachment = LookupAttachment( buf );

	++m_nSmokeTrailCount;

	pSmokeTrail->m_SpawnRate = 4;
	pSmokeTrail->m_ParticleLifetime = 5.0f;
	pSmokeTrail->m_StartColor.Init( 0.7f, 0.7f, 0.7f );
	pSmokeTrail->m_EndColor.Init( 0.6, 0.6, 0.6 );
	pSmokeTrail->m_StartSize = 32;
	pSmokeTrail->m_EndSize = 64;
	pSmokeTrail->m_SpawnRadius = 4;
	pSmokeTrail->m_Opacity = 0.5f;
	pSmokeTrail->m_MinSpeed = 16;
	pSmokeTrail->m_MaxSpeed = 16;
	pSmokeTrail->m_MinDirectedSpeed	= 16.0f;
	pSmokeTrail->m_MaxDirectedSpeed	= 16.0f;
	pSmokeTrail->SetLifetime( 5 );
	pSmokeTrail->SetParent( this, nAttachment );

	Vector vecForward( 0, 0, 1 );
	QAngle angles;
	VectorAngles( vecForward, angles );

	if ( nAttachment == 0 )
	{
		pSmokeTrail->SetAbsOrigin( vecPos );
		pSmokeTrail->SetAbsAngles( angles );
	}
	else
	{
		pSmokeTrail->SetLocalOrigin( vec3_origin );
		pSmokeTrail->SetLocalAngles( angles );
	}

	pSmokeTrail->SetMoveType( MOVETYPE_NONE );
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CPropAPC::ExplodeAndThrowChunk( const Vector &vecExplosionPos )
{
	ExplosionCreate( vecExplosionPos, vec3_angle, this, 1000, 500.0f, 
		SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS	|
		SF_ENVEXPLOSION_NOSMOKE  | SF_ENVEXPLOSION_NOFIREBALLSMOKE, 0 );
	UTIL_ScreenShake( vecExplosionPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	// Drop a flaming, smoking chunk.
	CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
	pChunk->Spawn( "models/gibs/hgibs.mdl" );
	pChunk->SetBloodColor( DONT_BLEED );

	QAngle vecSpawnAngles;
	vecSpawnAngles.Random( -90, 90 );
	pChunk->SetAbsOrigin( vecExplosionPos );
	pChunk->SetAbsAngles( vecSpawnAngles );

	int nGib = random->RandomInt( 0, APC_MAX_CHUNKS - 1 );
	pChunk->Spawn( s_pChunkModelName[nGib] );
	pChunk->SetOwnerEntity( this );
	pChunk->m_lifeTime = random->RandomFloat( 6.0f, 8.0f );
	pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
	
	// Set the velocity
	if ( pPhysicsObject )
	{
		pPhysicsObject->EnableMotion( true );
		Vector vecVelocity;

		QAngle angles;
		angles.x = random->RandomFloat( -40, 0 );
		angles.y = random->RandomFloat( 0, 360 );
		angles.z = 0.0f;
		AngleVectors( angles, &vecVelocity );
		
		vecVelocity *= random->RandomFloat( 300, 900 );
		vecVelocity += GetAbsVelocity();

		AngularImpulse angImpulse;
		angImpulse = RandomAngularImpulse( -180, 180 );

		pChunk->SetAbsVelocity( vecVelocity );
		pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
	}

	CEntityFlame *pFlame = CEntityFlame::Create( pChunk, false );
	if ( pFlame != NULL )
	{
		pFlame->SetLifetime( pChunk->m_lifeTime );
	}
}


//-----------------------------------------------------------------------------
// Should we trigger a damage effect?
//-----------------------------------------------------------------------------
inline bool CPropAPC::ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const
{
	int nPrevRange = (int)( ((float)nPrevHealth / (float)GetMaxHealth()) * nEffectCount );
	int nRange = (int)( ((float)GetHealth() / (float)GetMaxHealth()) * nEffectCount );
	return ( nRange != nPrevRange );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Event_Killed( const CTakeDamageInfo &info )
{
	m_OnDeath.FireOutput( info.GetAttacker(), this );

	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );

	Vector vecNormalizedMins, vecNormalizedMaxs;
	CollisionProp()->WorldToNormalizedSpace( vecAbsMins, &vecNormalizedMins );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMaxs, &vecNormalizedMaxs );

	Vector vecAbsPoint;
	CPASFilter filter( GetAbsOrigin() );
	for (int i = 0; i < 5; i++)
	{
		CollisionProp()->RandomPointInBounds( vecNormalizedMins, vecNormalizedMaxs, &vecAbsPoint );
		te->Explosion( filter, random->RandomFloat( 0.0, 1.0 ),	&vecAbsPoint, 
			g_sModelIndexFireball, random->RandomInt( 4, 10 ), 
			random->RandomInt( 8, 15 ), 
			( i < 2 ) ? TE_EXPLFLAG_NODLIGHTS : TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NODLIGHTS,
			100, 0 );
	}

	// TODO: make the gibs spawn in sync with the delayed explosions
	int nGibs = random->RandomInt( 1, 4 );
	for ( int i = 0; i < nGibs; i++)
	{
		// Throw a flaming, smoking chunk.
		CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
		pChunk->Spawn( "models/gibs/hgibs.mdl" );
		pChunk->SetBloodColor( DONT_BLEED );

		QAngle vecSpawnAngles;
		vecSpawnAngles.Random( -90, 90 );
		pChunk->SetAbsOrigin( vecAbsPoint );
		pChunk->SetAbsAngles( vecSpawnAngles );

		int nGib = random->RandomInt( 0, APC_MAX_CHUNKS - 1 );
		pChunk->Spawn( s_pChunkModelName[nGib] );
		pChunk->SetOwnerEntity( this );
		pChunk->m_lifeTime = random->RandomFloat( 6.0f, 8.0f );
		pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
		
		// Set the velocity
		if ( pPhysicsObject )
		{
			pPhysicsObject->EnableMotion( true );
			Vector vecVelocity;

			QAngle angles;
			angles.x = random->RandomFloat( -20, 20 );
			angles.y = random->RandomFloat( 0, 360 );
			angles.z = 0.0f;
			AngleVectors( angles, &vecVelocity );
			
			vecVelocity *= random->RandomFloat( 300, 900 );
			vecVelocity += GetAbsVelocity();

			AngularImpulse angImpulse;
			angImpulse = RandomAngularImpulse( -180, 180 );

			pChunk->SetAbsVelocity( vecVelocity );
			pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
		}

		CEntityFlame *pFlame = CEntityFlame::Create( pChunk, false );
		if ( pFlame != NULL )
		{
			pFlame->SetLifetime( pChunk->m_lifeTime );
		}
	}

	UTIL_ScreenShake( vecAbsPoint, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	if( hl2_episodic.GetBool() )
	{
		// EP1 perf hit
		Ignite( 6, false );
	}
	else
	{
		Ignite( 60, false );
	}

	m_lifeState = LIFE_DYING;

	// Spawn a lesser amount if the player is close
	m_iRocketSalvoLeft = DEATH_VOLLEY_ROCKET_COUNT;
	m_flRocketTime = gpGlobals->curtime;
}



//-----------------------------------------------------------------------------
// Purpose: Blows it up!
//-----------------------------------------------------------------------------
void CPropAPC::InputDestroy( inputdata_t &inputdata )
{
	CTakeDamageInfo info( this, this, m_iHealth, DMG_BLAST );
	info.SetDamagePosition( WorldSpaceCenter() );
	info.SetDamageForce( Vector( 0, 0, 1 ) );
	TakeDamage( info );
}


//-----------------------------------------------------------------------------
// Aim the next rocket at a specific target
//-----------------------------------------------------------------------------
void CPropAPC::InputFireMissileAt( inputdata_t &inputdata )
{
	string_t strMissileTarget = MAKE_STRING( inputdata.value.String() );
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, strMissileTarget, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( pTarget == NULL )
	{
		DevWarning( "%s: Could not find target '%s'!\n", GetClassname(), STRING( strMissileTarget ) );
		return;
	}

	m_hSpecificRocketTarget = pTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropAPC::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth == 0 )
		return 0;

	m_OnDamaged.FireOutput( info.GetAttacker(), this );

	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		m_OnDamagedByPlayer.FireOutput( info.GetAttacker(), this );
	}

	CTakeDamageInfo dmgInfo = info;
	if ( dmgInfo.GetDamageType() & (DMG_BLAST | DMG_AIRBOAT) )
	{
		int nPrevHealth = GetHealth();

		m_iHealth -= dmgInfo.GetDamage();
		if ( m_iHealth <= 0 )
		{
			m_iHealth = 0;
			Event_Killed( dmgInfo );
			return 0;
		}

		// Chain
//		BaseClass::OnTakeDamage( dmgInfo );

		// Spawn damage effects
		if ( nPrevHealth != GetHealth() )
		{
			if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_SMOKE_TRAILS ) )
			{
				AddSmokeTrail( dmgInfo.GetDamagePosition() );
			}

			if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_EXPLOSIONS ) )
			{
				ExplodeAndThrowChunk( dmgInfo.GetDamagePosition() );
			}
		}
	}
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
void CPropAPC::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	if ( m_flDangerSoundTime > gpGlobals->curtime )
		return;

	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir;

	GetVectors( &vecDir, NULL, NULL );

	// Make danger sounds ahead of the APC
	trace_t	tr;
	Vector	vecSpot, vecLeftDir, vecRightDir;

	// lay down sound path
	vecSpot = vecStart + vecDir * 600;
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	// put sounds a bit to left and right but slightly closer to APC to make a "cone" of sound 
	// in front of it
	QAngle leftAngles = vehicleAngles;
	leftAngles[YAW] += 20;
	VehicleAngleVectors( leftAngles, &vecLeftDir, NULL, NULL );
	vecSpot = vecStart + vecLeftDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	QAngle rightAngles = vehicleAngles;
	rightAngles[YAW] -= 20;
	VehicleAngleVectors( rightAngles, &vecRightDir, NULL, NULL );
	vecSpot = vecStart + vecRightDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this);

	m_flDangerSoundTime = gpGlobals->curtime + 0.3;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Think( void )
{
	BaseClass::Think();

	SetNextThink( gpGlobals->curtime );

	if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
	{
		m_bInitialHandbrake = true;
		m_VehiclePhysics.SetHandbrake( true );
		m_VehiclePhysics.Think();
	}

	StudioFrameAdvance();

	if ( IsSequenceFinished() )
	{
		int iSequence = SelectWeightedSequence( ACT_IDLE );
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			SetCycle( 0 );
			m_flAnimTime = gpGlobals->curtime;
			ResetSequence( iSequence );
			ResetClientsideFrame();
		}
	}

	if (m_debugOverlays & OVERLAY_NPC_KILL_BIT)
	{
		CTakeDamageInfo info( this, this, m_iHealth, DMG_BLAST );
		info.SetDamagePosition( WorldSpaceCenter() );
		info.SetDamageForce( Vector( 0, 0, 1 ) );
		TakeDamage( info );
	}
}


//-----------------------------------------------------------------------------
// Aims the secondary weapon at a target 
//-----------------------------------------------------------------------------
void CPropAPC::AimSecondaryWeaponAt( CBaseEntity *pTarget )
{
	m_hRocketTarget = pTarget;

	// Update the rocket target
	CreateAPCLaserDot();

	if ( m_hRocketTarget )
	{
		m_hLaserDot->SetAbsOrigin( m_hRocketTarget->BodyTarget( WorldSpaceCenter(), false ) );
	}
	SetLaserDotTarget( m_hLaserDot, m_hRocketTarget );
	EnableLaserDot( m_hLaserDot, m_hRocketTarget != NULL );
}

	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	switch( m_lifeState )
	{
	case LIFE_ALIVE:
		{
			int iButtons = ucmd->buttons;
			if ( iButtons & IN_ATTACK )
			{
				FireMachineGun();
			}
			else if ( iButtons & IN_ATTACK2 )
			{
				FireRocket();
			}
		}
		break;

	case LIFE_DYING:
		FireDying( );
		break;

	case LIFE_DEAD:
		return;
	}

	BaseClass::DriveVehicle( flFrameTime, ucmd, iButtonsDown, iButtonsReleased );
}

void CPropAPC::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	BaseClass::Use( pActivator, pCaller, useType, value );

	if ( pActivator->IsPlayer() )
	{
		 EmitSound ( "combine.door_lock" );
	}
}


//-----------------------------------------------------------------------------
// Primary gun 
//-----------------------------------------------------------------------------
void CPropAPC::AimPrimaryWeapon( const Vector &vecWorldTarget ) 
{
	EntityMatrix parentMatrix;
	parentMatrix.InitFromEntity( this, m_nMachineGunBaseAttachment );
	Vector target = parentMatrix.WorldToLocal( vecWorldTarget ); 

	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x*target.x + target.y*target.y;

	// Target is too close!  Can't aim at it
	if ( quadTarget > m_vecBarrelPos.LengthSqr() )
	{
		// We're trying to aim the offset barrel at an arbitrary point.
		// To calculate this, I think of the target as being on a sphere with 
		// it's center at the origin of the gun.
		// The rotation we need is the opposite of the rotation that moves the target 
		// along the surface of that sphere to intersect with the gun's shooting direction
		// To calculate that rotation, we simply calculate the intersection of the ray 
		// coming out of the barrel with the target sphere (that's the new target position)
		// and use atan2() to get angles

		// angles from target pos to center
		float targetToCenterYaw = atan2( target.y, target.x );
		float centerToGunYaw = atan2( m_vecBarrelPos.y, sqrt( quadTarget - (m_vecBarrelPos.y*m_vecBarrelPos.y) ) );

		float targetToCenterPitch = atan2( target.z, sqrt( quadTargetXY ) );
		float centerToGunPitch = atan2( -m_vecBarrelPos.z, sqrt( quadTarget - (m_vecBarrelPos.z*m_vecBarrelPos.z) ) );

		QAngle angles;
		angles.Init( -RAD2DEG(targetToCenterPitch+centerToGunPitch), RAD2DEG( targetToCenterYaw + centerToGunYaw ), 0 );

		SetPoseParameter( "vehicle_weapon_yaw", angles.y );
		SetPoseParameter( "vehicle_weapon_pitch", angles.x );
		StudioFrameAdvance();

		float curPitch = GetPoseParameter( "vehicle_weapon_pitch" );
		float curYaw = GetPoseParameter( "vehicle_weapon_yaw" );
		m_bInFiringCone = (fabs(curPitch - angles.x) < 1e-3) && (fabs(curYaw - angles.y) < 1e-3);
	}
	else
	{
		m_bInFiringCone = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPropAPC::GetTracerType( void ) 
{
	return "HelicopterTracer"; 
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CPropAPC::DoImpactEffect( trace_t &tr, int nDamageType )
{
	UTIL_ImpactTrace( &tr, nDamageType, "HelicopterImpact" );
} 


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::DoMuzzleFlash( void )
{
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = m_nMachineGunMuzzleAttachment;
	data.m_flScale = 1.0f;
	DispatchEffect( "ChopperMuzzleFlash", data );

	BaseClass::DoMuzzleFlash();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::FireMachineGun( void )
{
	if ( m_flMachineGunTime > gpGlobals->curtime )
		return;

	// If we're still firing the salvo, fire quickly
	m_iMachineGunBurstLeft--;
	if ( m_iMachineGunBurstLeft > 0 )
	{
		m_flMachineGunTime = gpGlobals->curtime + MACHINE_GUN_BURST_TIME;
	}
	else
	{
		// Reload the salvo
		m_iMachineGunBurstLeft = MACHINE_GUN_BURST_SIZE;
		m_flMachineGunTime = gpGlobals->curtime + MACHINE_GUN_BURST_PAUSE_TIME;
	}

	Vector vecMachineGunShootPos;
	Vector vecMachineGunDir;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecMachineGunShootPos, &vecMachineGunDir );
	
	// Fire the round
	int	bulletType = GetAmmoDef()->Index("AR2");
	FireBullets( 1, vecMachineGunShootPos, vecMachineGunDir, VECTOR_CONE_8DEGREES, MAX_TRACE_LENGTH, bulletType, 1 );
	DoMuzzleFlash();

	EmitSound( "Weapon_AR2.Single" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::GetRocketShootPosition( Vector *pPosition )
{
	GetAttachment( m_nRocketAttachment, *pPosition );
}


//-----------------------------------------------------------------------------
// Create a corpse 
//-----------------------------------------------------------------------------
void CPropAPC::CreateCorpse( )
{
	m_lifeState = LIFE_DEAD;

	for ( int i = 0; i < APC_MAX_GIBS; ++i )
	{
		CPhysicsProp *pGib = assert_cast<CPhysicsProp*>(CreateEntityByName( "prop_physics" ));
		pGib->SetAbsOrigin( GetAbsOrigin() );
		pGib->SetAbsAngles( GetAbsAngles() );
		pGib->SetAbsVelocity( GetAbsVelocity() );
		pGib->SetModel( s_pGibModelName[i] );
		pGib->Spawn();
		pGib->SetMoveType( MOVETYPE_VPHYSICS );

		float flMass = pGib->GetMass();
		if ( flMass < 200 )
		{
			Vector vecVelocity;
			pGib->GetMassCenter( &vecVelocity );
			vecVelocity -= WorldSpaceCenter();
			vecVelocity.z = fabs(vecVelocity.z);
			VectorNormalize( vecVelocity );

			// Apply a force that would make a 100kg mass travel 150 - 300 m/s
			float flRandomVel = random->RandomFloat( 150, 300 );
			vecVelocity *= (100 * flRandomVel) / flMass;
			vecVelocity.z += 100.0f;
			AngularImpulse angImpulse = RandomAngularImpulse( -500, 500 );
			
			IPhysicsObject *pObj = pGib->VPhysicsGetObject();
			if ( pObj != NULL )
			{
				pObj->AddVelocity( &vecVelocity, &angImpulse );
			}
			pGib->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		}	
		if( hl2_episodic.GetBool() )
		{
			// EP1 perf hit
			pGib->Ignite( 6, false );
		}
		else
		{
			pGib->Ignite( 60, false );
		}
	}

	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Death volley 
//-----------------------------------------------------------------------------
void CPropAPC::FireDying( )
{
	if ( m_flRocketTime > gpGlobals->curtime )
		return;

	Vector vecRocketOrigin;
	GetRocketShootPosition(	&vecRocketOrigin );

	Vector vecDir;
	vecDir.Random( -1.0f, 1.0f );
	if ( vecDir.z < 0.0f )
	{
		vecDir.z *= -1.0f;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, ROCKET_SPEED * random->RandomFloat( 0.75f, 1.25f ), vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *) CAPCMissile::Create( vecRocketOrigin, angles, vecVelocity, this );
	float flDeathTime = random->RandomFloat( 0.3f, 0.5f );
	if ( random->RandomFloat( 0.0f, 1.0f ) < 0.3f )
	{
		pRocket->ExplodeDelay( flDeathTime );
	}
	else
	{
		pRocket->AugerDelay( flDeathTime );
	}

	// Make erratic firing
	m_flRocketTime = gpGlobals->curtime + random->RandomFloat( DEATH_VOLLEY_MIN_FIRE_TIME, DEATH_VOLLEY_MAX_FIRE_TIME );
	if ( --m_iRocketSalvoLeft <= 0 )
	{
		CreateCorpse();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::FireRocket( void )
{
	if ( m_flRocketTime > gpGlobals->curtime )
		return;

	// If we're still firing the salvo, fire quickly
	m_iRocketSalvoLeft--;
	if ( m_iRocketSalvoLeft > 0 )
	{
		m_flRocketTime = gpGlobals->curtime + ROCKET_DELAY_TIME;
	}
	else
	{
		// Reload the salvo
		m_iRocketSalvoLeft = ROCKET_SALVO_SIZE;
		m_flRocketTime = gpGlobals->curtime + random->RandomFloat( ROCKET_MIN_BURST_PAUSE_TIME, ROCKET_MAX_BURST_PAUSE_TIME );
	}

	Vector vecRocketOrigin;
	GetRocketShootPosition(	&vecRocketOrigin );

	static float s_pSide[] = { 0.966, 0.866, 0.5, -0.5, -0.866, -0.966 };

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	Vector vecDir;
	CrossProduct( Vector( 0, 0, 1 ), forward, vecDir );
	vecDir.z = 1.0f;
	vecDir.x *= s_pSide[m_nRocketSide];
	vecDir.y *= s_pSide[m_nRocketSide];
	if ( ++m_nRocketSide >= 6 )
	{
		m_nRocketSide = 0;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, ROCKET_SPEED, vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *)CAPCMissile::Create( vecRocketOrigin, angles, vecVelocity, this );
	pRocket->IgniteDelay();

	if ( m_hSpecificRocketTarget )
	{
		pRocket->AimAtSpecificTarget( m_hSpecificRocketTarget );
		m_hSpecificRocketTarget = NULL;
	}
	else if ( m_strMissileHint != NULL_STRING )
	{
		pRocket->SetGuidanceHint( STRING( m_strMissileHint ) );
	}

	EmitSound( "PropAPC.FireRocket" );
	m_OnFiredMissile.FireOutput( this, this );
}
									 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPropAPC::MaxAttackRange() const
{
	return ROCKET_ATTACK_RANGE_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::OnRestore( void )
{
	IServerVehicle *pServerVehicle = GetServerVehicle();
	if ( pServerVehicle != NULL )
	{
		// Restore the passenger information we're holding on to
		pServerVehicle->RestorePassengerInfo();
	}
}

//========================================================================================================================================
// APC FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::NPC_AimPrimaryWeapon( Vector vecTarget )
{
	CPropAPC *pAPC = ((CPropAPC*)m_pVehicle);
	pAPC->AimPrimaryWeapon( vecTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::NPC_AimSecondaryWeapon( Vector vecTarget )
{
	// Add some random noise
//	Vector vecOffset = vecTarget + RandomVector( -128, 128 );
//	((CPropAPC*)m_pVehicle)->AimSecondaryWeaponAt( vecOffset );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = MACHINE_GUN_ATTACK_RANGE_MIN;
	*flMaxRange = MACHINE_GUN_ATTACK_RANGE_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = ROCKET_ATTACK_RANGE_MIN;
	*flMaxRange = ROCKET_ATTACK_RANGE_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's primary weapon can fire again
//-----------------------------------------------------------------------------
float CAPCFourWheelServerVehicle::Weapon_PrimaryCanFireAt( void )
{
	return ((CPropAPC*)m_pVehicle)->PrimaryWeaponFireTime();
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's secondary weapon can fire again
//-----------------------------------------------------------------------------
float CAPCFourWheelServerVehicle::Weapon_SecondaryCanFireAt( void )
{
	return ((CPropAPC*)m_pVehicle)->SecondaryWeaponFireTime();
}

