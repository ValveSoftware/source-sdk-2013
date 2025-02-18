//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "entityoutput.h"
#include "physics.h"
#include "explode.h"
#include "vphysics_interface.h"
#include "collisionutils.h"
#include "steamjet.h"
#include "eventqueue.h"
#include "soundflags.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "physics_cannister.h"
#include "globals.h"
#include "physics_saverestore.h"
#include "shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_CANNISTER_ASLEEP		0x0001
#define SF_CANNISTER_EXPLODE	0x0002

BEGIN_SIMPLE_DATADESC( CThrustController )

	DEFINE_FIELD( m_thrustVector,	FIELD_VECTOR ),
	DEFINE_FIELD( m_torqueVector,	FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_thrust,		FIELD_FLOAT, "thrust" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( physics_cannister, CPhysicsCannister );

BEGIN_DATADESC( CPhysicsCannister )

	DEFINE_OUTPUT( m_onActivate, "OnActivate" ),
	DEFINE_OUTPUT( m_OnAwakened, "OnAwakened" ),
	DEFINE_FIELD( m_thrustOrigin, FIELD_VECTOR ),	// this is a position, but in local space
	DEFINE_EMBEDDED( m_thruster ),
	DEFINE_PHYSPTR( m_pController ),
	DEFINE_FIELD( m_pJet, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_active, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_thrustTime, FIELD_FLOAT, "fuel" ),
	DEFINE_KEYFIELD( m_damage, FIELD_FLOAT, "expdamage" ),
	DEFINE_KEYFIELD( m_damageRadius, FIELD_FLOAT, "expradius" ),
	DEFINE_FIELD( m_activateTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_gasSound, FIELD_SOUNDNAME, "gassound" ),
	DEFINE_FIELD( m_bFired, FIELD_BOOLEAN ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),
	DEFINE_FIELD( m_hLauncher, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Wake", InputWake ),

	DEFINE_THINKFUNC( BeginShutdownThink ),
	DEFINE_ENTITYFUNC( ExplodeTouch ),

END_DATADESC()

void CPhysicsCannister::Spawn( void )
{
	Precache();
	SetModel( STRING(GetModelName()) );
	SetBloodColor( DONT_BLEED );

	AddSolidFlags( FSOLID_CUSTOMRAYTEST );
	m_takedamage = DAMAGE_YES;
	SetNextThink( TICK_NEVER_THINK );

	if ( m_iHealth <= 0 )
		m_iHealth = 25;

	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle( 0 );
	m_bFired = false;

	// not thrusting
	m_active = false;

	CreateVPhysics();
	if ( !VPhysicsGetObject() )
	{
		// must have a physics object or code will crash later
		UTIL_Remove(this);
	}
}

void CPhysicsCannister::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_pController )
	{
		m_pController->SetEventHandler( &m_thruster );
	}
}

bool CPhysicsCannister::CreateVPhysics()
{
	bool asleep = HasSpawnFlags(SF_CANNISTER_ASLEEP);

	VPhysicsInitNormal( SOLID_VPHYSICS, 0, asleep );
	return true;
}

bool CPhysicsCannister::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );

	if ( !IsBoxIntersectingRay( vecAbsMins, vecAbsMaxs, ray.m_Start, ray.m_Delta ) )
		return false;
	
	return BaseClass::TestCollision( ray, mask, trace );
}

Vector CPhysicsCannister::CalcLocalThrust( const Vector &offset )
{
	matrix3x4_t nozzleMatrix;
	Vector thrustDirection;

	GetAttachment( LookupAttachment("nozzle"), nozzleMatrix );
	MatrixGetColumn( nozzleMatrix, 2, thrustDirection );
	MatrixGetColumn( nozzleMatrix, 3, m_thrustOrigin );
	thrustDirection = -5*thrustDirection + offset;
	VectorNormalize( thrustDirection );
	return thrustDirection;
}


CPhysicsCannister::~CPhysicsCannister( void )
{
}

void CPhysicsCannister::Precache( void )
{
	PropBreakablePrecacheAll( GetModelName() );
	if ( m_gasSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_gasSound) );
	}
	BaseClass::Precache();
}

int CPhysicsCannister::OnTakeDamage( const CTakeDamageInfo &info )
{
	// HACKHACK: Shouldn't g_vecAttackDir be a parameter to this function?
	if ( !m_takedamage )
		return 0;

	if ( !m_active )
	{
		m_iHealth -= info.GetDamage();
		if ( m_iHealth < 0 )
		{
			Explode( info.GetAttacker() );
		}
		else
		{
			// explosions that don't destroy will activate
			// 50% of the time blunt damage will activate as well
			if ( (info.GetDamageType() & DMG_BLAST) ||
				( (info.GetDamageType() & (DMG_CLUB|DMG_SLASH|DMG_CRUSH) ) && random->RandomInt(1,100) < 50 ) )
			{
				CannisterActivate( info.GetAttacker(), g_vecAttackDir );
			}
		}
		return 1;
	}

	if ( (gpGlobals->curtime - m_activateTime) <= 0.1 )
		return 0;

	if ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_BURN|DMG_BLAST) )
	{
		Explode( info.GetAttacker() );
	}

	return 0;
}


void CPhysicsCannister::TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( !m_active && ptr->hitgroup != 0 )
	{
		Vector direction = -dir;
		direction.z -= 5;
		VectorNormalize( direction );
		CannisterActivate( info.GetAttacker(), direction );
	}
	BaseClass::TraceAttack( info, dir, ptr, pAccumulator );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::CannisterActivate( CBaseEntity *pActivator, const Vector &thrustOffset )
{
	// already active or spent
	if ( m_active || !m_thrustTime )
	{
		return;
	}

	m_hLauncher = pActivator;

	Vector thrustDirection = CalcLocalThrust( thrustOffset );
	m_onActivate.FireOutput( pActivator, this, 0 );
	m_thruster.CalcThrust( m_thrustOrigin, thrustDirection, VPhysicsGetObject() );
	m_pController = physenv->CreateMotionController( &m_thruster );
	IPhysicsObject *pPhys = VPhysicsGetObject();
	m_pController->AttachObject( pPhys, true );
	// Make sure the object is simulated
	pPhys->Wake();

	m_active = true;
	m_activateTime = gpGlobals->curtime;
	SetNextThink( gpGlobals->curtime + m_thrustTime );
	SetThink( &CPhysicsCannister::BeginShutdownThink );

	QAngle angles;
	VectorAngles( -thrustDirection, angles );
	m_pJet = dynamic_cast<CSteamJet *>( CBaseEntity::Create( "env_steam", m_thrustOrigin, angles, this ) );
	m_pJet->SetParent( this );

	float extra = m_thruster.m_thrust * (1/5000.f);
	extra = clamp( extra, 0.f, 1.f );

	m_pJet->m_SpreadSpeed = 15 * m_thruster.m_thrust * 0.001;
	m_pJet->m_Speed = 128 + 100 * extra;
	m_pJet->m_StartSize = 10;
	m_pJet->m_EndSize = 25;

	m_pJet->m_Rate = 52 + (int)extra*20;
	m_pJet->m_JetLength = 64;
	m_pJet->m_clrRender = m_clrRender;

	m_pJet->Use( this, this, USE_ON, 1 );
	if ( m_gasSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_ITEM;
		ep.m_pSoundName =  STRING(m_gasSound);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The cannister's been fired by a weapon, so it should stay pretty accurate
//-----------------------------------------------------------------------------
void CPhysicsCannister::CannisterFire( CBaseEntity *pActivator )
{
	m_bFired = true;

	// Increase thrust
	m_thruster.m_thrust *= 4;

	// Only last a short time
	m_thrustTime = 10.0;

	// Explode on contact
	SetTouch( &CPhysicsCannister::ExplodeTouch );

	CannisterActivate( pActivator, vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for activating the cannister.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputActivate( inputdata_t &data )
{
	CannisterActivate( data.pActivator, Vector(0,0.1,-0.25) );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for deactivating the cannister.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputDeactivate(inputdata_t &data)
{
	Deactivate();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the cannister go boom.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputExplode(inputdata_t &data)
{
	Explode( data.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for waking up the cannister if it is sleeping.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputWake( inputdata_t &data )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys != NULL )
	{
		pPhys->Wake();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::Deactivate(void)
{
	if ( !m_pController )
		return;

	m_pController->DetachObject( VPhysicsGetObject() );
	physenv->DestroyMotionController( m_pController );
	m_pController = NULL;
	SetNextThink( TICK_NEVER_THINK );
	m_thrustTime = 0;
	m_active = false;
	if ( m_pJet )
	{
		ShutdownJet();
	}
	if ( m_gasSound != NULL_STRING )
	{
		StopSound( entindex(), CHAN_ITEM, STRING(m_gasSound) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::Explode( CBaseEntity *pAttacker )
{
	// don't recurse
	m_takedamage = 0;
	Deactivate();

	Vector velocity;
	AngularImpulse angVelocity;
	IPhysicsObject *pPhysics = VPhysicsGetObject();

	pPhysics->GetVelocity( &velocity, &angVelocity );
	PropBreakableCreateAll( GetModelIndex(), pPhysics, GetAbsOrigin(), GetAbsAngles(), velocity, angVelocity, 1.0, 20, COLLISION_GROUP_DEBRIS );
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), pAttacker, m_damage, 0, true );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Explode when I next hit a damageable entity
//-----------------------------------------------------------------------------
void CPhysicsCannister::ExplodeTouch( CBaseEntity *pOther )
{
	if ( !pOther->m_takedamage )
		return;

	Explode( m_hLauncher );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	if ( m_bFired && m_active )
	{
		int otherIndex = !index;
		CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
		if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
			return;

		// If we hit hard enough. explode
		if ( pEvent->collisionSpeed > 1000 )
		{
			Explode( m_hLauncher );
			return;
		}
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::ShutdownJet( void )
{
	g_EventQueue.AddEvent( m_pJet, "kill", 5, NULL, NULL );

	m_pJet->m_bEmit = false;
	m_pJet->m_Rate = 0;
	m_pJet = NULL;
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: The think just shuts the cannister down
//-----------------------------------------------------------------------------
void CPhysicsCannister::BeginShutdownThink( void )
{
	Deactivate();
}

//-----------------------------------------------------------------------------
// Physics Attacker
//-----------------------------------------------------------------------------
void CPhysicsCannister::SetPhysicsAttacker( CBasePlayer *pEntity, float flTime )
{
	m_hPhysicsAttacker = pEntity;
	m_flLastPhysicsInfluenceTime = flTime;
}

	
//-----------------------------------------------------------------------------
// Purpose: Keep track of physgun influence
//-----------------------------------------------------------------------------
void CPhysicsCannister::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );
	if ( Reason == LAUNCHED_BY_CANNON )
	{
		CannisterActivate( pPhysGunUser, vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer *CPhysicsCannister::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose: Update the visible representation of the physic system's representation of this object
//-----------------------------------------------------------------------------
void CPhysicsCannister::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	// if this is the first time we have moved, fire our target
	if ( HasSpawnFlags( SF_CANNISTER_ASLEEP ) )
	{
		if ( !pPhysics->IsAsleep() )
		{
			m_OnAwakened.FireOutput(this, this);
			RemoveSpawnFlags( SF_CANNISTER_ASLEEP );
		}
	}
}