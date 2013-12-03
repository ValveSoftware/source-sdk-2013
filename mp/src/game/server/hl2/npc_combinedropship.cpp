//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Large vehicle what delivers combine troops.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_basenpc.h"
#include "soundenvelope.h"
#include "cbasehelicopter.h"
#include "ai_schedule.h"
#include "engine/IEngineSound.h"
#include "smoke_trail.h"
#include "IEffects.h"
#include "props.h"
#include "TemplateEntities.h"
#include "baseanimating.h"
#include "ai_senses.h"
#include "entitylist.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "npc_combines.h"
#include "soundent.h"
#include "mapentities.h"
#include "npc_rollermine.h"
#include "scripted.h"
#include "explode.h"
#include "gib.h"
#include "EntityFlame.h"
#include "entityblocker.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
#define SF_DROPSHIP_WAIT_FOR_DROPOFF_INPUT		( 1 << 15 )	

#define DROPSHIP_ACCEL_RATE				300

// Timers
#define DROPSHIP_LANDING_HOVER_TIME		5		// Time to spend on the ground if we have no troops to drop
#define DROPSHIP_TIME_BETWEEN_MINES		0.5f

// Special actions
#define DROPSHIP_DEFAULT_SOLDIERS		4
#define DROPSHIP_MAX_SOLDIERS			6

// Movement
#define DROPSHIP_BUFF_TIME				0.3f
#define DROPSHIP_MAX_LAND_TILT			2.5f
#define DROPSHIP_CONTAINER_HEIGHT		130.0f
#define DROPSHIP_MAX_SPEED				(60 * 17.6) // 120 miles per hour.

// Pathing data
#define	DROPSHIP_LEAD_DISTANCE			800.0f
#define	DROPSHIP_MIN_CHASE_DIST_DIFF	128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define	DROPSHIP_AVOID_DIST				256.0f
#define DROPSHIP_ARRIVE_DIST			128.0f

#define	CRATE_BBOX_MIN		(Vector( -100, -80, -60 ))
#define CRATE_BBOX_MAX		(Vector( 100, 80, 80 ))

// Size
// With crate
#define DROPSHIP_BBOX_CRATE_MIN		(-Vector(40,40,60))
#define DROPSHIP_BBOX_CRATE_MAX		(Vector(40,40,40))
// Without crate
#define DROPSHIP_BBOX_MIN			(-Vector(40,40,0))
#define DROPSHIP_BBOX_MAX			(Vector(40,40,40))

// Container gun
#define DROPSHIP_GUN_SPEED			10		// Rotation speed

#define DROPSHIP_CRATE_ROCKET_HITS	4

enum DROP_STATES 
{
	DROP_IDLE = 0,
	DROP_NEXT,
};

enum CRATE_TYPES 
{
	CRATE_JEEP = -3,
	CRATE_APC = -2,
	CRATE_STRIDER = -1,
	CRATE_ROLLER_HOPPER,
	CRATE_SOLDIER,
	CRATE_NONE,
};

ConVar	g_debug_dropship( "g_debug_dropship", "0" );
ConVar  sk_dropship_container_health( "sk_dropship_container_health", "750" );
ConVar	sk_npc_dmg_dropship( "sk_npc_dmg_dropship","5", FCVAR_NONE, "Dropship container cannon damage." );

//=====================================
// Animation Events
//=====================================
#define AE_DROPSHIP_RAMP_OPEN	1		// the tailgate is open.

//=====================================
// Custom activities
//=====================================
// Without Cargo
Activity ACT_DROPSHIP_FLY_IDLE;			// Flying. Vertical aspect 
Activity ACT_DROPSHIP_FLY_IDLE_EXAGG;	// Exaggerated version of the flying idle
// With Cargo
Activity ACT_DROPSHIP_FLY_IDLE_CARGO;	// Flying. Vertical aspect 
Activity ACT_DROPSHIP_DESCEND_IDLE;		// waiting to touchdown
Activity ACT_DROPSHIP_DEPLOY_IDLE;		// idle on the ground with door open. Troops are leaving.
Activity ACT_DROPSHIP_LIFTOFF;			// transition back to FLY IDLE

enum LandingState_t
{
	LANDING_NO = 0,

	// Dropoff
	LANDING_LEVEL_OUT,		// Heading to a point above the dropoff point
	LANDING_DESCEND,		// Descending from to the dropoff point
	LANDING_TOUCHDOWN,
	LANDING_UNLOADING,
	LANDING_UNLOADED,
	LANDING_LIFTOFF,

	// Pickup
	LANDING_SWOOPING,		// Swooping down to the target

	// Hovering, which we're saying is a type of landing since there's so much landing code to leverage
	LANDING_START_HOVER,
	LANDING_HOVER_LEVEL_OUT,
	LANDING_HOVER_DESCEND,
	LANDING_HOVER_TOUCHDOWN,
	LANDING_END_HOVER,
};


#define DROPSHIP_NEAR_SOUND_MIN_DISTANCE 1000
#define DROPSHIP_NEAR_SOUND_MAX_DISTANCE 2500
#define DROPSHIP_GROUND_WASH_MIN_ALTITUDE 100.0f
#define DROPSHIP_GROUND_WASH_MAX_ALTITUDE 750.0f



//=============================================================================
// The combine dropship container
//=============================================================================
#define DROPSHIP_CONTAINER_MODEL "models/combine_dropship_container.mdl"

#define DROPSHIP_CONTAINER_MAX_CHUNKS	3
static const char *s_pChunkModelName[DROPSHIP_CONTAINER_MAX_CHUNKS] = 
{
	"models/gibs/helicopter_brokenpiece_01.mdl",
	"models/gibs/helicopter_brokenpiece_02.mdl",
	"models/gibs/helicopter_brokenpiece_03.mdl",
};

#define DROPSHIP_CONTAINER_MAX_GIBS	1
static const char *s_pGibModelName[DROPSHIP_CONTAINER_MAX_GIBS] = 
{
	"models/combine_dropship_container.mdl",
};

class CCombineDropshipContainer : public CPhysicsProp
{
	DECLARE_CLASS( CCombineDropshipContainer, CPhysicsProp );
	DECLARE_DATADESC();

public:
	void Precache();
	virtual void Spawn();
	virtual bool OverridePropdata( void );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

private:
	enum
	{
		MAX_SMOKE_TRAILS = 4,
		MAX_EXPLOSIONS = 4,
	};

	// Should we trigger a damage effect?
	bool ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const;

	// Add a smoke trail since we've taken more damage
	void AddSmokeTrail( const Vector &vecPos );

	// Pow!
	void ThrowFlamingGib();

	// Create a corpse
	void CreateCorpse();

private:
	int m_nSmokeTrailCount;
	EHANDLE m_hLastInflictor;
	float m_flLastHitTime;
};

//=============================================================================
// The combine dropship
//=============================================================================
class CNPC_CombineDropship : public CBaseHelicopter
{
	DECLARE_CLASS( CNPC_CombineDropship, CBaseHelicopter );

public:
	~CNPC_CombineDropship();

	// Setup
	void	Spawn( void );
	void	Precache( void );

	void	Activate( void );

	// Thinking/init
	void	InitializeRotorSound( void );
	void	StopLoopingSounds();
	void	PrescheduleThink( void );

	// Flight/sound
	void	Hunt( void );
	void	Flight( void );
	float	GetAltitude( void );
	void	DoRotorWash( void );
	void	UpdateRotorSoundPitch( int iPitch );
	void	UpdatePickupNavigation( void );
	void	UpdateLandTargetNavigation( void );
	void	CalculateSoldierCount( int iSoldiers );

	// Updates the facing direction
	virtual void UpdateFacingDirection();

	// Combat
	void	GatherEnemyConditions( CBaseEntity *pEnemy );
	void	DoCombatStuff( void );
	void	SpawnTroop( void );
	void	DropMine( void );
	void	UpdateContainerGunFacing( Vector &vecMuzzle, Vector &vecToTarget, Vector &vecAimDir, float *flTargetRange );
	bool	FireCannonRound( void );
	void	DoImpactEffect( trace_t &tr, int nDamageType );
	void	StartCannon( void );
	void	StopCannon( void );
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	// Input handlers.
	void	InputLandLeave( inputdata_t &inputdata );
	void	InputLandTake( inputdata_t &inputdata );
	void	InputSetLandTarget( inputdata_t &inputdata );
	void	InputDropMines( inputdata_t &inputdata );
	void	InputDropStrider( inputdata_t &inputdata );
	void	InputDropAPC( inputdata_t &inputdata );

	void	InputPickup( inputdata_t &inputdata );
	void	InputSetGunRange( inputdata_t &inputdata );
	void	InputNPCFinishDustoff( inputdata_t &inputdata );
	void	InputStopWaitingForDropoff( inputdata_t &inputdata );

	void	InputHover( inputdata_t &inputdata );

	// From AI_TrackPather
	virtual void InputFlyToPathTrack( inputdata_t &inputdata );

	Vector	GetDropoffFinishPosition( Vector vecOrigin, CAI_BaseNPC *pNPC, Vector vecMins, Vector vecMaxs );
	void	LandCommon( bool bHover = false );

	Class_T Classify( void ) { return CLASS_COMBINE_GUNSHIP; }

	// Drop the soldier container
	void	DropSoldierContainer( );

	// Sounds
	virtual void UpdateRotorWashVolume();

private:
	void SetLandingState( LandingState_t landingState );
	LandingState_t GetLandingState() const { return (LandingState_t)m_iLandState; }
	bool IsHovering();
	void UpdateGroundRotorWashSound( float flAltitude );
	void UpdateRotorWashVolume( CSoundPatch *pRotorSound, float flVolume, float flDeltaTime );

private:
	// Timers
	float	m_flTimeTakeOff;
	float	m_flNextTroopSpawnAttempt;
	float	m_flDropDelay;			// delta between each mine
	float	m_flTimeNextAttack;
	float	m_flLastTime;
	
	// States and counters
	int		m_iMineCount;		// index for current mine # being deployed
	int		m_totalMinesToDrop;	// total # of mines to drop as a group (based upon triggered input)
	int		m_soldiersToDrop;
	int		m_iDropState;
	int		m_iLandState; 
	float	m_engineThrust;		// for tracking sound volume/pitch
	float	m_existPitch;
	float	m_existRoll;
	bool	m_bDropMines;		// signal to drop mines
	bool	m_bIsFiring;
	int		m_iBurstRounds;
	bool	m_leaveCrate;
	bool	m_bHasDroppedOff;
	int		m_iCrateType;
	float	m_flLandingSpeed;
	float	m_flGunRange;
	bool	m_bInvulnerable;

	QAngle	m_vecAngAcceleration;
	
	// Misc Vars
	CHandle<CBaseAnimating>	m_hContainer;
	EHANDLE		m_hPickupTarget;
	int			m_iContainerMoveType;
	bool		m_bWaitForDropoffInput;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	EHANDLE		m_hLandTarget;
	string_t	m_iszLandTarget;

	string_t	m_iszAPCVehicleName;

	// Templates for soldier's dropped off
	string_t	m_sNPCTemplate[ DROPSHIP_MAX_SOLDIERS ];
	string_t	m_sNPCTemplateData[ DROPSHIP_MAX_SOLDIERS ];	
	string_t	m_sDustoffPoints[ DROPSHIP_MAX_SOLDIERS ];	
	int			m_iCurrentTroopExiting;
	EHANDLE		m_hLastTroopToLeave;

	// Template for rollermines dropped by this dropship
	string_t	m_sRollermineTemplate;
	string_t	m_sRollermineTemplateData;

	// Cached attachment points
	int			m_iMuzzleAttachment;
	int			m_iMachineGunBaseAttachment;
	int			m_iMachineGunRefAttachment;
	int			m_iAttachmentTroopDeploy;
	int			m_iAttachmentDeployStart;

	// Sounds
	CSoundPatch		*m_pCannonSound;
	CSoundPatch		*m_pRotorOnGroundSound;
	CSoundPatch		*m_pDescendingWarningSound;
	CSoundPatch		*m_pNearRotorSound;

	// Outputs
	COutputEvent	m_OnFinishedDropoff;
	COutputEvent	m_OnFinishedPickup;

	COutputFloat	m_OnContainerShotDownBeforeDropoff;
	COutputEvent	m_OnContainerShotDownAfterDropoff;

protected:
	// Because the combine dropship is a leaf class, we can use
	// static variables to store this information, and save some memory.
	// Should the dropship end up having inheritors, their activate may
	// stomp these numbers, in which case you should make these ordinary members
	// again.
	static int m_poseBody_Accel, m_poseBody_Sway, m_poseCargo_Body_Accel, m_poseCargo_Body_Sway, 
		m_poseWeapon_Pitch, m_poseWeapon_Yaw;
	static bool m_sbStaticPoseParamsLoaded;
	virtual void	PopulatePoseParameters( void );
};

bool CNPC_CombineDropship::m_sbStaticPoseParamsLoaded = false;

int CNPC_CombineDropship::m_poseBody_Accel = 0;
int CNPC_CombineDropship::m_poseBody_Sway = 0;
int CNPC_CombineDropship::m_poseCargo_Body_Accel = 0;
int CNPC_CombineDropship::m_poseCargo_Body_Sway = 0;
int CNPC_CombineDropship::m_poseWeapon_Pitch = 0;
int CNPC_CombineDropship::m_poseWeapon_Yaw = 0;

//-----------------------------------------------------------------------------
// Purpose: Cache whatever pose parameters we intend to use
//-----------------------------------------------------------------------------
void	CNPC_CombineDropship::PopulatePoseParameters( void )
{
	if (!m_sbStaticPoseParamsLoaded)
	{
		m_poseBody_Accel		= LookupPoseParameter( "body_accel");
		m_poseBody_Sway			= LookupPoseParameter( "body_sway" );
		m_poseCargo_Body_Accel  = LookupPoseParameter( "cargo_body_accel" );
		m_poseCargo_Body_Sway   = LookupPoseParameter( "cargo_body_sway" );
		m_poseWeapon_Pitch		= LookupPoseParameter( "weapon_pitch" );
		m_poseWeapon_Yaw		= LookupPoseParameter( "weapon_yaw" );

		m_sbStaticPoseParamsLoaded = true;
	}

	BaseClass::PopulatePoseParameters();
}

//------------------------------------------------------------------------------
//
// Combine Dropship Container implementation:
//
//------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( prop_dropship_container, CCombineDropshipContainer )

BEGIN_DATADESC( CCombineDropshipContainer )

	DEFINE_FIELD( m_nSmokeTrailCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hLastInflictor,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastHitTime,		FIELD_TIME ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Precache 
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::Precache()
{
	PrecacheModel( DROPSHIP_CONTAINER_MODEL );

	// Set this here to quiet base prop warnings
	SetModel( DROPSHIP_CONTAINER_MODEL );

	BaseClass::Precache();

	int i;
	for ( i = 0; i < DROPSHIP_CONTAINER_MAX_CHUNKS; ++i )
	{
		PrecacheModel( s_pChunkModelName[i] );
	}

	for ( i = 0; i < DROPSHIP_CONTAINER_MAX_GIBS; ++i )
	{
		PrecacheModel( s_pGibModelName[i] );
	}

	PropBreakablePrecacheAll( GetModelName() );
}


//-----------------------------------------------------------------------------
// Spawn 
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::Spawn()
{
	// NOTE: Model must be set before spawn
	SetModel( DROPSHIP_CONTAINER_MODEL );
	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();

#ifdef _XBOX
	AddEffects( EF_NOSHADOW );
#endif //_XBOX

	m_iHealth = m_iMaxHealth = sk_dropship_container_health.GetFloat();
}


//-----------------------------------------------------------------------------
// Allows us to use vphysics
//-----------------------------------------------------------------------------
bool CCombineDropshipContainer::OverridePropdata( void )
{
	return true;
}


//-----------------------------------------------------------------------------
// Should we trigger a damage effect?
//-----------------------------------------------------------------------------
inline bool CCombineDropshipContainer::ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const
{
	int nPrevRange = (int)( ((float)nPrevHealth / (float)GetMaxHealth()) * nEffectCount );
	int nRange = (int)( ((float)GetHealth() / (float)GetMaxHealth()) * nEffectCount );
	return ( nRange != nPrevRange );
}


//-----------------------------------------------------------------------------
// Character killed (only fired once)
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::CreateCorpse()
{
	m_lifeState = LIFE_DEAD;

	Vector vecNormalizedMins, vecNormalizedMaxs;
	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMins, &vecNormalizedMins );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMaxs, &vecNormalizedMaxs );

	// Explode
	Vector vecAbsPoint;
	CPASFilter filter( GetAbsOrigin() );
	CollisionProp()->RandomPointInBounds( vecNormalizedMins, vecNormalizedMaxs, &vecAbsPoint);
	te->Explosion( filter, 0.0f, &vecAbsPoint, g_sModelIndexFireball, 
		random->RandomInt( 4, 10 ), random->RandomInt( 8, 15 ), TE_EXPLFLAG_NOPARTICLES, 100, 0 );

	// Break into chunks
	Vector angVelocity;
	QAngleToAngularImpulse( GetLocalAngularVelocity(), angVelocity );
	PropBreakableCreateAll( GetModelIndex(), VPhysicsGetObject(), GetAbsOrigin(), GetAbsAngles(), GetAbsVelocity(), angVelocity, 1.0, 250, COLLISION_GROUP_NPC, this );

	// Create flaming gibs
	int iChunks = random->RandomInt( 4, 6 );
	for ( int i = 0; i < iChunks; i++ )
	{
		ThrowFlamingGib();
	}

	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Character killed (only fired once)
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::ThrowFlamingGib( void )
{
	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );

	Vector vecNormalizedMins, vecNormalizedMaxs;
	CollisionProp()->WorldToNormalizedSpace( vecAbsMins, &vecNormalizedMins );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMaxs, &vecNormalizedMaxs );

	Vector vecAbsPoint;
	CPASFilter filter( GetAbsOrigin() );
	CollisionProp()->RandomPointInBounds( vecNormalizedMins, vecNormalizedMaxs, &vecAbsPoint);

	// Throw a flaming, smoking chunk.
	CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
	pChunk->Spawn( "models/gibs/hgibs.mdl" );
	pChunk->SetBloodColor( DONT_BLEED );

	QAngle vecSpawnAngles;
	vecSpawnAngles.Random( -90, 90 );
	pChunk->SetAbsOrigin( vecAbsPoint );
	pChunk->SetAbsAngles( vecSpawnAngles );

	int nGib = random->RandomInt( 0, DROPSHIP_CONTAINER_MAX_CHUNKS - 1 );
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

	SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
	if( pSmokeTrail )
	{
		pSmokeTrail->m_SpawnRate = 80;
		pSmokeTrail->m_ParticleLifetime = 0.8f;
		pSmokeTrail->m_StartColor.Init(0.3, 0.3, 0.3);
		pSmokeTrail->m_EndColor.Init(0.5, 0.5, 0.5);
		pSmokeTrail->m_StartSize = 10;
		pSmokeTrail->m_EndSize = 40;
		pSmokeTrail->m_SpawnRadius = 5;
		pSmokeTrail->m_Opacity = 0.4;
		pSmokeTrail->m_MinSpeed = 15;
		pSmokeTrail->m_MaxSpeed = 25;
		pSmokeTrail->SetLifetime( pChunk->m_lifeTime );
		pSmokeTrail->SetParent( pChunk, 0 );
		pSmokeTrail->SetLocalOrigin( vec3_origin );
		pSmokeTrail->SetMoveType( MOVETYPE_NONE );
	}
}


//-----------------------------------------------------------------------------
// Character killed (only fired once)
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::Event_Killed( const CTakeDamageInfo &info )
{
	if ( GetOwnerEntity() )
	{
		CNPC_CombineDropship *pDropship = assert_cast<CNPC_CombineDropship *>(GetOwnerEntity() );
		pDropship->DropSoldierContainer();
	}

	CreateCorpse();
}


//-----------------------------------------------------------------------------
// Damage effects 
//-----------------------------------------------------------------------------
int CCombineDropshipContainer::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth == 0 )
		return 0;

	// Airboat guns + explosive damage is all that can hurt it
	if (( info.GetDamageType() & (DMG_BLAST | DMG_AIRBOAT) ) == 0 )
		return 0;

	CTakeDamageInfo dmgInfo = info;

	int nPrevHealth = GetHealth();

	if ( info.GetDamageType() & DMG_BLAST )
	{
		// This check is necessary to prevent double-counting of rocket damage
		// from the blast hitting both the dropship + the container
		if ( (info.GetInflictor() != m_hLastInflictor) || (gpGlobals->curtime != m_flLastHitTime) )
		{
			m_iHealth -= (m_iMaxHealth / DROPSHIP_CRATE_ROCKET_HITS) + 1;
			m_hLastInflictor = info.GetInflictor();
			m_flLastHitTime = gpGlobals->curtime; 
		}
	}
	else
	{
		m_iHealth -= dmgInfo.GetDamage();
	}

	if ( m_iHealth <= 0 )
	{
		m_iHealth = 0;
		Event_Killed( dmgInfo );
		return 0;
	}

	// Spawn damage effects
	if ( nPrevHealth != GetHealth() )
	{
		if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_SMOKE_TRAILS ) )
		{
			AddSmokeTrail( dmgInfo.GetDamagePosition() );
		}

		if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_EXPLOSIONS ) )
		{
			ExplosionCreate( dmgInfo.GetDamagePosition(), vec3_angle, this, 1000, 500.0f, 
			SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0 );
			UTIL_ScreenShake( dmgInfo.GetDamagePosition(), 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

			ThrowFlamingGib();
		}
	}

	return 1;
}


//-----------------------------------------------------------------------------
// Add a smoke trail since we've taken more damage
//-----------------------------------------------------------------------------
void CCombineDropshipContainer::AddSmokeTrail( const Vector &vecPos )
{
	// Start this trail out with a bang!
	ExplosionCreate( vecPos, vec3_angle, this, 1000, 500.0f, SF_ENVEXPLOSION_NODAMAGE | 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0 );
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

	pSmokeTrail->m_SpawnRate = 20;
	pSmokeTrail->m_ParticleLifetime = 4.0f;
	pSmokeTrail->m_StartColor.Init( 0.7f, 0.7f, 0.7f );
	pSmokeTrail->m_EndColor.Init( 0.6, 0.6, 0.6 );
	pSmokeTrail->m_StartSize = 15;
	pSmokeTrail->m_EndSize = 50;
	pSmokeTrail->m_SpawnRadius = 15;
	pSmokeTrail->m_Opacity = 0.75f;
	pSmokeTrail->m_MinSpeed = 10;
	pSmokeTrail->m_MaxSpeed = 20;
	pSmokeTrail->m_MinDirectedSpeed	= 100.0f;
	pSmokeTrail->m_MaxDirectedSpeed	= 120.0f;
	pSmokeTrail->SetLifetime( 5 );
	pSmokeTrail->SetParent( this, nAttachment );
	if ( nAttachment == 0 )
	{
		pSmokeTrail->SetAbsOrigin( vecPos );
	}
	else
	{
		pSmokeTrail->SetLocalOrigin( vec3_origin );
	}

	Vector vecForward( -1, 0, 0 );
	QAngle angles;
	VectorAngles( vecForward, angles );
	pSmokeTrail->SetAbsAngles( angles );
	pSmokeTrail->SetMoveType( MOVETYPE_NONE );
}


//------------------------------------------------------------------------------
//
// Combine Dropship implementation:
//
//------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( npc_combinedropship, CNPC_CombineDropship );

BEGIN_DATADESC( CNPC_CombineDropship )

	DEFINE_FIELD( m_flTimeTakeOff, FIELD_TIME ),
	DEFINE_FIELD( m_flNextTroopSpawnAttempt, FIELD_TIME ),
	DEFINE_FIELD( m_flDropDelay, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeNextAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastTime, FIELD_TIME ),
	DEFINE_FIELD( m_iMineCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_totalMinesToDrop, FIELD_INTEGER ),
	DEFINE_FIELD( m_soldiersToDrop, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDropState, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDropMines, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iLandState, FIELD_INTEGER ),
	DEFINE_FIELD( m_engineThrust, FIELD_FLOAT ),
	DEFINE_FIELD( m_bIsFiring, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBurstRounds, FIELD_INTEGER ),
	DEFINE_FIELD( m_existPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_existRoll, FIELD_FLOAT ),
	DEFINE_FIELD( m_leaveCrate, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_iCrateType, FIELD_INTEGER, "CrateType" ),
	DEFINE_FIELD( m_flLandingSpeed, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flGunRange, FIELD_FLOAT, "GunRange" ),
	DEFINE_FIELD( m_vecAngAcceleration,FIELD_VECTOR ),
	DEFINE_FIELD( m_hContainer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPickupTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iContainerMoveType, FIELD_INTEGER ),
	DEFINE_FIELD( m_bWaitForDropoffInput, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hLandTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bHasDroppedOff, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bInvulnerable, FIELD_BOOLEAN, "Invulnerable" ),
	DEFINE_KEYFIELD( m_iszLandTarget, FIELD_STRING,	"LandTarget" ),
	DEFINE_SOUNDPATCH( m_pRotorOnGroundSound ),
	DEFINE_SOUNDPATCH( m_pDescendingWarningSound ),
	DEFINE_SOUNDPATCH( m_pNearRotorSound ),

	DEFINE_KEYFIELD( m_iszAPCVehicleName, FIELD_STRING,	"APCVehicleName" ),

	DEFINE_KEYFIELD( m_sRollermineTemplate, FIELD_STRING,	"RollermineTemplate" ),
	DEFINE_FIELD( m_sRollermineTemplateData, FIELD_STRING ),

	DEFINE_ARRAY( m_sNPCTemplateData, FIELD_STRING, DROPSHIP_MAX_SOLDIERS ),
	DEFINE_KEYFIELD( m_sNPCTemplate[0], FIELD_STRING,	"NPCTemplate" ),
	DEFINE_KEYFIELD( m_sNPCTemplate[1], FIELD_STRING,	"NPCTemplate2" ),
	DEFINE_KEYFIELD( m_sNPCTemplate[2], FIELD_STRING,	"NPCTemplate3" ),
	DEFINE_KEYFIELD( m_sNPCTemplate[3], FIELD_STRING,	"NPCTemplate4" ),
	DEFINE_KEYFIELD( m_sNPCTemplate[4], FIELD_STRING,	"NPCTemplate5" ),
	DEFINE_KEYFIELD( m_sNPCTemplate[5], FIELD_STRING,	"NPCTemplate6" ),
	// Here to shut classcheck up
	//DEFINE_ARRAY( m_sNPCTemplate, FIELD_STRING,  DROPSHIP_MAX_SOLDIERS  ),
	//DEFINE_ARRAY( m_sDustoffPoints, FIELD_STRING,  DROPSHIP_MAX_SOLDIERS  ),
	DEFINE_KEYFIELD( m_sDustoffPoints[0], FIELD_STRING,	"Dustoff1" ),
	DEFINE_KEYFIELD( m_sDustoffPoints[1], FIELD_STRING,	"Dustoff2" ),
	DEFINE_KEYFIELD( m_sDustoffPoints[2], FIELD_STRING,	"Dustoff3" ),
	DEFINE_KEYFIELD( m_sDustoffPoints[3], FIELD_STRING,	"Dustoff4" ),
	DEFINE_KEYFIELD( m_sDustoffPoints[4], FIELD_STRING,	"Dustoff5" ),
	DEFINE_KEYFIELD( m_sDustoffPoints[5], FIELD_STRING,	"Dustoff6" ),
	DEFINE_FIELD( m_iCurrentTroopExiting, FIELD_INTEGER ),
	DEFINE_FIELD( m_hLastTroopToLeave, FIELD_EHANDLE ),

	DEFINE_FIELD( m_iMuzzleAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMachineGunBaseAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMachineGunRefAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAttachmentTroopDeploy, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAttachmentDeployStart , FIELD_INTEGER ),

	DEFINE_SOUNDPATCH( m_pCannonSound ),
	
	DEFINE_INPUTFUNC( FIELD_INTEGER, "LandLeaveCrate", InputLandLeave ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "LandTakeCrate", InputLandTake ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetLandTarget", InputSetLandTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "DropMines", InputDropMines ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DropStrider", InputDropStrider ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DropAPC", InputDropAPC ),
	DEFINE_INPUTFUNC( FIELD_STRING, "Pickup", InputPickup ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetGunRange", InputSetGunRange ),
	DEFINE_INPUTFUNC( FIELD_STRING, "NPCFinishDustoff", InputNPCFinishDustoff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopWaitingForDropoff", InputStopWaitingForDropoff ),
	DEFINE_INPUTFUNC( FIELD_STRING, "Hover", InputHover ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FlyToPathTrack", InputFlyToPathTrack ),
	
	DEFINE_OUTPUT( m_OnFinishedDropoff, "OnFinishedDropoff" ),
	DEFINE_OUTPUT( m_OnFinishedPickup, "OnFinishedPickup" ),

	DEFINE_OUTPUT( m_OnContainerShotDownBeforeDropoff, "OnCrateShotDownBeforeDropoff" ),
	DEFINE_OUTPUT( m_OnContainerShotDownAfterDropoff, "OnCrateShotDownAfterDropoff" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose : Destructor
//------------------------------------------------------------------------------
CNPC_CombineDropship::~CNPC_CombineDropship(void)
{
	if ( m_hContainer )
	{
		UTIL_Remove( m_hContainer );		// get rid of container
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Spawn( void )
{
	Precache( );
	SetModel( "models/combine_dropship.mdl" );

#ifdef _XBOX
	AddEffects( EF_NOSHADOW );
#endif //_XBOX

	InitPathingData( DROPSHIP_ARRIVE_DIST, DROPSHIP_MIN_CHASE_DIST_DIFF, DROPSHIP_AVOID_DIST );

	m_iContainerMoveType = MOVETYPE_NONE;
	m_iCurrentTroopExiting = 0;
	m_bHasDroppedOff = false;
	m_iMuzzleAttachment = -1;
	m_iMachineGunBaseAttachment = -1;
	m_iMachineGunRefAttachment = -1;
	m_iAttachmentTroopDeploy = -1;
	m_iAttachmentDeployStart = -1;

	// create the correct bin for the ship to carry
	switch ( m_iCrateType )
	{
	case CRATE_ROLLER_HOPPER:
		break;

	case CRATE_SOLDIER:
		m_hContainer = (CBaseAnimating*)CreateEntityByName( "prop_dropship_container" );
		if ( m_hContainer )
		{
			m_hContainer->SetName( AllocPooledString("dropship_container") );
			m_hContainer->SetAbsOrigin( GetAbsOrigin() );
			m_hContainer->SetAbsAngles( GetAbsAngles() );
			m_hContainer->SetParent(this, 0);
			m_hContainer->SetOwnerEntity(this);
			m_hContainer->Spawn();

			IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetShadow( 1e4, 1e4, false, false );
				pPhysicsObject->UpdateShadow( m_hContainer->GetAbsOrigin(), m_hContainer->GetAbsAngles(), false, 0 );
			}

			m_hContainer->SetMoveType( MOVETYPE_PUSH );
			m_hContainer->SetGroundEntity( NULL );

			// Cache off container's attachment points
			m_iAttachmentTroopDeploy = m_hContainer->LookupAttachment( "deploy_landpoint" );
			m_iAttachmentDeployStart = m_hContainer->LookupAttachment( "Deploy_Start" );
			m_iMuzzleAttachment = m_hContainer->LookupAttachment( "muzzle" );
			m_iMachineGunBaseAttachment = m_hContainer->LookupAttachment( "gun_base" );
			// NOTE: gun_ref must have the same position as gun_base, but rotates with the gun
			m_iMachineGunRefAttachment = m_hContainer->LookupAttachment( "gun_ref" );
		}
		break;

	case CRATE_STRIDER:
		m_hContainer = (CBaseAnimating*)CreateEntityByName( "npc_strider" );
		m_hContainer->SetAbsOrigin( GetAbsOrigin() - Vector( 0, 0 , 100 ) );
		m_hContainer->SetAbsAngles( GetAbsAngles() );
		m_hContainer->SetParent(this, 0);
		m_hContainer->SetOwnerEntity(this);
		m_hContainer->Spawn();
		m_hContainer->SetAbsOrigin( GetAbsOrigin() - Vector( 0, 0 , 100 ) );
		break;

	case CRATE_APC:
		{
			m_soldiersToDrop = 0;
			m_hContainer = (CBaseAnimating*)gEntList.FindEntityByName( NULL, m_iszAPCVehicleName );
			if ( !m_hContainer )
			{
				Warning("Unable to find APC %s\n", STRING( m_iszAPCVehicleName ) ); 		
				break;
			}

			Vector apcPosition = GetAbsOrigin() - Vector( 0, 0 , 25 );
			QAngle apcAngles = GetAbsAngles();
			VMatrix mat, rot, result;
			MatrixFromAngles( apcAngles, mat );
			MatrixBuildRotateZ( rot, -90 );
			MatrixMultiply( mat, rot, result );
			MatrixToAngles( result, apcAngles );

			m_hContainer->Teleport( &apcPosition, &apcAngles, NULL );

			m_iContainerMoveType = m_hContainer->GetMoveType();

			IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetShadow( 1e4, 1e4, false, false );
			}

			m_hContainer->SetParent(this, 0);
			m_hContainer->SetOwnerEntity(this);
			m_hContainer->SetMoveType( MOVETYPE_PUSH );
			m_hContainer->SetGroundEntity( NULL );
			m_hContainer->UpdatePhysicsShadowToCurrentPosition(0);
		}
		break;

	case CRATE_JEEP:
		m_hContainer = (CBaseAnimating*)CreateEntityByName( "prop_dynamic_override" );
		if ( m_hContainer )
		{
			m_hContainer->SetModel( "models/buggy.mdl" );
			m_hContainer->SetName( AllocPooledString("dropship_jeep") );

			m_hContainer->SetAbsOrigin( GetAbsOrigin() );//- Vector( 0, 0 , 25 ) );
			QAngle angles = GetAbsAngles();
			VMatrix mat, rot, result;
			MatrixFromAngles( angles, mat );
			MatrixBuildRotateZ( rot, -90 );
			MatrixMultiply( mat, rot, result );
			MatrixToAngles( result, angles );
			m_hContainer->SetAbsAngles( angles );

			m_hContainer->SetParent(this, 0);
			m_hContainer->SetOwnerEntity(this);
			m_hContainer->SetSolid( SOLID_VPHYSICS );
			m_hContainer->Spawn();
		}
		break;

	case CRATE_NONE:
	default:
		break;
	}

	// Setup our bbox
	if ( m_hContainer )
	{
		UTIL_SetSize( this, DROPSHIP_BBOX_CRATE_MIN, DROPSHIP_BBOX_CRATE_MAX );
		SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_CARGO );
	}
	else
	{
		UTIL_SetSize( this, DROPSHIP_BBOX_MIN, DROPSHIP_BBOX_MAX );
		SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_EXAGG );
	}

	m_cullBoxMins = WorldAlignMins() - Vector(300,300,200);
	m_cullBoxMaxs = WorldAlignMaxs() + Vector(300,300,200);
	BaseClass::Spawn();

	// Dropship ignores all damage, but can deal it to its carried container
	m_takedamage = m_bInvulnerable ? DAMAGE_NO : DAMAGE_YES;
	if ( m_bInvulnerable && m_hContainer )
	{
		m_hContainer->m_takedamage = DAMAGE_NO;
	}

	m_iHealth = 100;
	m_flFieldOfView = 0.5; // 60 degrees
	m_iBurstRounds = 15;

	InitBoneControllers();
	InitCustomSchedules();

	m_flMaxSpeed = DROPSHIP_MAX_SPEED;
	m_flMaxSpeedFiring = BASECHOPPER_MAX_FIRING_SPEED;
	m_hPickupTarget = NULL;
	m_hLandTarget = NULL;

	//!!!HACKHACK
	// This tricks the AI code that constantly complains that the vehicle has no schedule.
	SetSchedule( SCHED_IDLE_STAND );

	SetLandingState( LANDING_NO );

	if ( HasSpawnFlags( SF_DROPSHIP_WAIT_FOR_DROPOFF_INPUT ) )
	{
		m_bWaitForDropoffInput = true;
	}
	else
	{
		m_bWaitForDropoffInput = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after spawning on map load or on a load from save game.
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::Activate( void )
{
	BaseClass::Activate();

	if ( !m_sRollermineTemplateData )
	{
		m_sRollermineTemplateData = NULL_STRING;
		if ( m_sRollermineTemplate != NULL_STRING )
		{
			// This must be the first time we're activated, not a load from save game.
			// Look up the template in the template database.
			m_sRollermineTemplateData = Templates_FindByTargetName(STRING(m_sRollermineTemplate));
			if ( m_sRollermineTemplateData == NULL_STRING )
			{
				Warning( "npc_combinedropship %s: Rollermine Template %s not found!\n", STRING(GetEntityName()), STRING(m_sRollermineTemplate) );
			}
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Precache( void )
{
	// Models
	PrecacheModel("models/combine_dropship.mdl");
	switch ( m_iCrateType )
	{
	case CRATE_SOLDIER:
		UTIL_PrecacheOther( "prop_dropship_container" );

		//
		// Precache the all templates that we are configured to spawn
		//
		for ( int i = 0; i < DROPSHIP_MAX_SOLDIERS; i++ )
		{
			if ( m_sNPCTemplate[i] != NULL_STRING )
			{
				if ( m_sNPCTemplateData[i] == NULL_STRING )
				{
					m_sNPCTemplateData[i] = Templates_FindByTargetName(STRING(m_sNPCTemplate[i]));
				}
				if ( m_sNPCTemplateData[i] != NULL_STRING )
				{
					CBaseEntity *pEntity = NULL;
					MapEntity_ParseEntity( pEntity, STRING(m_sNPCTemplateData[i]), NULL );
					if ( pEntity != NULL )
					{
						pEntity->Precache();
						UTIL_RemoveImmediate( pEntity );
					}
				}
				else
				{
					Warning( "npc_combinedropship %s: Template NPC %s not found!\n", STRING(GetEntityName()), STRING(m_sNPCTemplate[i]) );

					// Use the first template we've got
					m_sNPCTemplateData[i] = m_sNPCTemplateData[0];
				}

				// Make sure we've got a dustoff point for it
				if ( m_sDustoffPoints[i] == NULL_STRING )
				{
					Warning( "npc_combinedropship %s: Has no dustoff point for NPC %d!\n", STRING(GetEntityName()), i );
				}
			}
			else
			{
				m_sNPCTemplateData[i] = NULL_STRING;
			}
		}
		break;

	case CRATE_JEEP:
		PrecacheModel("models/buggy.mdl");
		break;

	default:
		break;
	}

	PrecacheScriptSound( "NPC_CombineDropship.RotorLoop" );
	PrecacheScriptSound( "NPC_CombineDropship.FireLoop" );
	PrecacheScriptSound( "NPC_CombineDropship.NearRotorLoop" );
	PrecacheScriptSound( "NPC_CombineDropship.OnGroundRotorLoop" );
	PrecacheScriptSound( "NPC_CombineDropship.DescendingWarningLoop" );
	PrecacheScriptSound( "NPC_CombineDropship.NearRotorLoop" );

	if ( m_sRollermineTemplate != NULL_STRING )
	{
		UTIL_PrecacheOther( "npc_rollermine" );
	}

	BaseClass::Precache();

}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Flight( void )
{
	// Only run the flight model in some flight states
	bool bRunFlight = ( GetLandingState() == LANDING_NO || 
							GetLandingState() == LANDING_LEVEL_OUT || 
							GetLandingState() == LANDING_LIFTOFF ||
							GetLandingState() == LANDING_SWOOPING ||
							GetLandingState() == LANDING_DESCEND ||
							GetLandingState() == LANDING_HOVER_LEVEL_OUT ||
							GetLandingState() == LANDING_HOVER_DESCEND );

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	float finspeed = 0;
	float swayspeed = 0;
	Vector vecImpulse = vec3_origin;

	//Adrian: Slowly lerp the orientation and position of the cargo into place...
	//We assume CRATE_NONE means the dropship just picked up some random phys object.
	if ( m_hContainer != NULL && ( m_iCrateType == CRATE_SOLDIER || m_iCrateType == CRATE_NONE ) )
	{
		if ( m_hContainer->GetLocalOrigin() != vec3_origin )
		{
			Vector vCurrentLocalOrigin = m_hContainer->GetLocalOrigin();
			Vector vLocalOrigin;
	
			VectorLerp( vCurrentLocalOrigin, vec3_origin, 0.05f, vLocalOrigin );
	
			m_hContainer->SetLocalOrigin( vLocalOrigin );
		}

		if ( m_hContainer->GetLocalAngles() != vec3_angle )
		{
			QAngle vCurrentLocalAngles = m_hContainer->GetLocalAngles();
			QAngle vLocalAngles;

			vLocalAngles = Lerp( 0.05f, vCurrentLocalAngles, vec3_angle );

			m_hContainer->SetLocalAngles( vLocalAngles );
		}
	}

	if ( bRunFlight )
	{
		if( GetFlags() & FL_ONGROUND )
		{
			// This would be really bad.
			SetGroundEntity( NULL );
		}

		// calc desired acceleration
		float dt = 1.0f;

		Vector	accel;
		float	accelRate = DROPSHIP_ACCEL_RATE;
		float	maxSpeed = GetMaxSpeed();

		if ( m_lifeState == LIFE_DYING )
		{
			accelRate *= 5.0;
			maxSpeed *= 5.0;
		}

		float flCurrentSpeed = GetAbsVelocity().Length();
		float flDist = MIN( flCurrentSpeed + accelRate, maxSpeed );

		Vector deltaPos;
		if ( GetLandingState() == LANDING_SWOOPING )
		{
			// Move directly to the target point
			deltaPos = GetDesiredPosition();
		}
		else
		{
			ComputeActualTargetPosition( flDist, dt, 0.0f, &deltaPos );
		}
		deltaPos -= GetAbsOrigin();

		//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + deltaPos, 0, 255, 0, true, 0.1f );

		// calc goal linear accel to hit deltaPos in dt time.
		accel.x = 2.0 * (deltaPos.x - GetAbsVelocity().x * dt) / (dt * dt);
		accel.y = 2.0 * (deltaPos.y - GetAbsVelocity().y * dt) / (dt * dt);
		accel.z = 2.0 * (deltaPos.z - GetAbsVelocity().z * dt + 0.5 * 384 * dt * dt) / (dt * dt);
		
		float flDistFromPath = 0.0f;
		Vector vecPoint, vecDelta;
		if ( IsOnPathTrack() && GetLandingState() == LANDING_NO )
		{
			// Also, add in a little force to get us closer to our current line segment if we can
			ClosestPointToCurrentPath( &vecPoint );
			VectorSubtract( vecPoint, GetAbsOrigin(), vecDelta );
 			flDistFromPath = VectorNormalize( vecDelta );
			if ( flDistFromPath > 200 )
			{
				// Strongly constrain to an n unit pipe around the current path
				// by damping out all impulse forces that would push us further from the pipe
				float flAmount = (flDistFromPath - 200) / 200.0f;
				flAmount = clamp( flAmount, 0, 1 );
				VectorMA( accel, flAmount * 200.0f, vecDelta, accel );
			}
		}

		// don't fall faster than 0.2G or climb faster than 2G
		accel.z = clamp( accel.z, 384 * 0.2, 384 * 2.0 );

		Vector goalUp = accel;
		VectorNormalize( goalUp );

		// calc goal orientation to hit linear accel forces
		float goalPitch = RAD2DEG( asin( DotProduct( forward, goalUp ) ) );
		float goalYaw = UTIL_VecToYaw( m_vecDesiredFaceDir );
		float goalRoll = RAD2DEG( asin( DotProduct( right, goalUp ) ) );

		// clamp goal orientations
		goalPitch = clamp( goalPitch, -45, 60 );
		goalRoll = clamp( goalRoll, -45, 45 );

		// calc angular accel needed to hit goal pitch in dt time.
		dt = 0.6;
		QAngle goalAngAccel;
		goalAngAccel.x = 2.0 * (AngleDiff( goalPitch, AngleNormalize( GetLocalAngles().x ) ) - GetLocalAngularVelocity().x * dt) / (dt * dt);
		goalAngAccel.y = 2.0 * (AngleDiff( goalYaw, AngleNormalize( GetLocalAngles().y ) ) - GetLocalAngularVelocity().y * dt) / (dt * dt);
		goalAngAccel.z = 2.0 * (AngleDiff( goalRoll, AngleNormalize( GetLocalAngles().z ) ) - GetLocalAngularVelocity().z * dt) / (dt * dt);

		goalAngAccel.x = clamp( goalAngAccel.x, -300, 300 );
		//goalAngAccel.y = clamp( goalAngAccel.y, -60, 60 );
		goalAngAccel.y = clamp( goalAngAccel.y, -120, 120 );
		goalAngAccel.z = clamp( goalAngAccel.z, -300, 300 );

		// limit angular accel changes to simulate mechanical response times
		dt = 0.1;
		QAngle angAccelAccel;
		angAccelAccel.x = (goalAngAccel.x - m_vecAngAcceleration.x) / dt;
		angAccelAccel.y = (goalAngAccel.y - m_vecAngAcceleration.y) / dt;
		angAccelAccel.z = (goalAngAccel.z - m_vecAngAcceleration.z) / dt;

		angAccelAccel.x = clamp( angAccelAccel.x, -1000, 1000 );
		angAccelAccel.y = clamp( angAccelAccel.y, -1000, 1000 );
		angAccelAccel.z = clamp( angAccelAccel.z, -1000, 1000 );

		m_vecAngAcceleration += angAccelAccel * 0.1;

		// DevMsg( "pitch %6.1f (%6.1f:%6.1f)  ", goalPitch, GetLocalAngles().x, m_vecAngVelocity.x );
		// DevMsg( "roll %6.1f (%6.1f:%6.1f) : ", goalRoll, GetLocalAngles().z, m_vecAngVelocity.z );
		// DevMsg( "%6.1f %6.1f %6.1f  :  ", goalAngAccel.x, goalAngAccel.y, goalAngAccel.z );
		// DevMsg( "%6.0f %6.0f %6.0f\n", angAccelAccel.x, angAccelAccel.y, angAccelAccel.z );

		ApplySidewaysDrag( right );
		ApplyGeneralDrag();
		
		QAngle angVel = GetLocalAngularVelocity();
		angVel += m_vecAngAcceleration * 0.1;

		//angVel.y = clamp( angVel.y, -60, 60 );
		//angVel.y = clamp( angVel.y, -120, 120 );
		angVel.y = clamp( angVel.y, -120, 120 );

		SetLocalAngularVelocity( angVel );

		m_flForce = m_flForce * 0.8 + (accel.z + fabs( accel.x ) * 0.1 + fabs( accel.y ) * 0.1) * 0.1 * 0.2;

		vecImpulse = m_flForce * up;
		
		if ( m_lifeState == LIFE_DYING )
		{
			vecImpulse.z = -38.4;  // 64ft/sec
		}
		else
		{
			vecImpulse.z -= 38.4;  // 32ft/sec
		}

		// Find our current velocity
		Vector vecVelDir = GetAbsVelocity();

		VectorNormalize( vecVelDir );

		if ( flDistFromPath > 100 )
		{
			// Strongly constrain to an n unit pipe around the current path
			// by damping out all impulse forces that would push us further from the pipe
			float flDot = DotProduct( vecImpulse, vecDelta );
			if ( flDot < 0.0f )
			{
				VectorMA( vecImpulse, -flDot * 0.1f, vecDelta, vecImpulse );
			}

			// Also apply an extra impulse to compensate for the current velocity
			flDot = DotProduct( vecVelDir, vecDelta );
			if ( flDot < 0.0f )
			{
				VectorMA( vecImpulse, -flDot * 0.1f, vecDelta, vecImpulse );
			}
		}
		
		// Find our acceleration direction
		Vector	vecAccelDir = vecImpulse;
		VectorNormalize( vecAccelDir );

		// Level out our plane of movement
		vecAccelDir.z	= 0.0f;
		vecVelDir.z		= 0.0f;
		forward.z		= 0.0f;
		right.z			= 0.0f;

		// Find out how "fast" we're moving in relation to facing and acceleration
		finspeed = m_flForce * DotProduct( vecVelDir, vecAccelDir );
		swayspeed = m_flForce * DotProduct( vecVelDir, right );
	}

	// Use the correct pose params for the state of our container
	int poseBodyAccel;
	int poseBodySway;
	if ( m_hContainer || GetLandingState() == LANDING_SWOOPING )
	{
		poseBodyAccel = m_poseCargo_Body_Accel;
		poseBodySway = m_poseCargo_Body_Sway;
		SetPoseParameter( m_poseBody_Accel, 0 );
		SetPoseParameter( m_poseBody_Sway, 0 );
	}
	else
	{
		poseBodyAccel = m_poseBody_Accel;
		poseBodySway = m_poseBody_Sway;
		SetPoseParameter( m_poseCargo_Body_Accel, 0 );
		SetPoseParameter( m_poseCargo_Body_Sway, 0 );
	}

	// If we're landing, deliberately tuck in the back end
	if ( GetLandingState() == LANDING_DESCEND || GetLandingState() == LANDING_TOUCHDOWN || 
		 GetLandingState() == LANDING_UNLOADING || GetLandingState() == LANDING_UNLOADED || IsHovering() )
	{
		finspeed = -60;
	}

	// Apply the acceleration blend to the fins
	float finAccelBlend = SimpleSplineRemapVal( finspeed, -60, 60, -1, 1 );
	float curFinAccel = GetPoseParameter( poseBodyAccel );
	curFinAccel = UTIL_Approach( finAccelBlend, curFinAccel, 0.1f );
	SetPoseParameter( poseBodyAccel, EdgeLimitPoseParameter( poseBodyAccel, curFinAccel ) );

	// Apply the spin sway to the fins
	float finSwayBlend = SimpleSplineRemapVal( swayspeed, -60, 60, -1, 1 );
	float curFinSway = GetPoseParameter( poseBodySway );
	curFinSway = UTIL_Approach( finSwayBlend, curFinSway, 0.1f );
	SetPoseParameter( poseBodySway, EdgeLimitPoseParameter( poseBodySway, curFinSway ) );

	if ( bRunFlight )
	{
		// Add in our velocity pulse for this frame
		ApplyAbsVelocityImpulse( vecImpulse );
	}

	//DevMsg("curFinAccel: %f, curFinSway: %f\n", curFinAccel, curFinSway );
}


//------------------------------------------------------------------------------
// Deals damage to what's behing carried
//------------------------------------------------------------------------------
int CNPC_CombineDropship::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo ) 
{
	// FIXME: To make this work for CRATE_STRIDER or CRATE_APC, we need to
	// add code to the strider + apc to make them not take double-damage from rockets
	// (owing to the blast hitting the crate + the dropship). See the dropship container
	// code above to see how to do it.
	if ( m_hContainer && !m_bInvulnerable )
	{
		if ( (inputInfo.GetDamageType() & DMG_AIRBOAT) || (m_iCrateType == CRATE_SOLDIER) )
		{
			m_hContainer->TakeDamage( inputInfo );
		}
	}

	// don't die
	return 0; 
}

//------------------------------------------------------------------------------
// Updates the facing direction
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateFacingDirection( void )
{
	if ( GetEnemy() )
	{
		if ( !IsCrashing() && m_flLastSeen + 5 > gpGlobals->curtime )
		{
			// If we've seen the target recently, face the target.
			//Msg( "Facing Target \n" );
			m_vecDesiredFaceDir = m_vecTargetPosition - GetAbsOrigin();
		}
		else
		{
			// Remain facing the way you were facing...
		}
	}
	else
	{
		// Face our desired position.
		if ( GetDesiredPosition().DistToSqr( GetAbsOrigin() ) > 1 )
		{
			m_vecDesiredFaceDir = GetDesiredPosition() - GetAbsOrigin();
		}
		else
		{
			GetVectors( &m_vecDesiredFaceDir, NULL, NULL );
		}
	}
	VectorNormalize( m_vecDesiredFaceDir ); 
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( this );
	m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_CombineDropship.RotorLoop" );
	m_pNearRotorSound = controller.SoundCreate( filter, entindex(), "NPC_CombineDropship.NearRotorLoop" );
	m_pRotorOnGroundSound = controller.SoundCreate( filter, entindex(), "NPC_CombineDropship.OnGroundRotorLoop" );
	m_pDescendingWarningSound = controller.SoundCreate( filter, entindex(), "NPC_CombineDropship.DescendingWarningLoop" );
	m_pCannonSound = controller.SoundCreate( filter, entindex(), "NPC_CombineDropship.FireLoop"  );

	// NOTE: m_pRotorSound is started up by the base class
	if ( m_pCannonSound )
	{
		controller.Play( m_pCannonSound, 0.0, 100 );
	}

	if ( m_pDescendingWarningSound )
	{
		controller.Play( m_pDescendingWarningSound, 0.0, 100 );
	}

	if ( m_pRotorOnGroundSound )
	{
		controller.Play( m_pRotorOnGroundSound, 0.0, 100 );
	}
	
	if ( m_pNearRotorSound )
	{
		controller.Play( m_pNearRotorSound, 0.0, 100 );
	}

	m_engineThrust = 1.0f;

	BaseClass::InitializeRotorSound();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::StopLoopingSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pCannonSound )
	{
		controller.SoundDestroy( m_pCannonSound );
		m_pCannonSound = NULL;
	}

	if ( m_pRotorOnGroundSound )
	{
		controller.SoundDestroy( m_pRotorOnGroundSound );
		m_pRotorOnGroundSound = NULL;
	}

	if ( m_pDescendingWarningSound )
	{
		controller.SoundDestroy( m_pDescendingWarningSound );
		m_pDescendingWarningSound = NULL;
	}

	if ( m_pNearRotorSound )
	{
		controller.SoundDestroy( m_pNearRotorSound );
		m_pNearRotorSound = NULL;
	}

	BaseClass::StopLoopingSounds();
}


//------------------------------------------------------------------------------
// Updates the rotor wash volume
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateRotorWashVolume( CSoundPatch *pRotorSound, float flVolume, float flDeltaTime )
{
	if ( !pRotorSound )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	float flVolDelta = flVolume - controller.SoundGetVolume( pRotorSound );
	if ( flVolDelta )
	{
		// We can change from 0 to 1 in 3 seconds. 
		// Figure out how many seconds flVolDelta will take.
		float flRampTime = fabs( flVolDelta ) * flDeltaTime; 
		controller.SoundChangeVolume( pRotorSound, flVolume, flRampTime );
	}
}


//------------------------------------------------------------------------------
// Updates the rotor wash volume
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateRotorWashVolume()
{
	float flNearFactor = 0.0f; 
	CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
	if (pPlayer)
	{
		float flDist = pPlayer->GetAbsOrigin().DistTo( GetAbsOrigin() );
		flDist = clamp( flDist, DROPSHIP_NEAR_SOUND_MIN_DISTANCE, DROPSHIP_NEAR_SOUND_MAX_DISTANCE );
		flNearFactor = RemapVal( flDist, DROPSHIP_NEAR_SOUND_MIN_DISTANCE, DROPSHIP_NEAR_SOUND_MAX_DISTANCE, 1.0f, 0.0f );
	}

	if ( m_pRotorSound )
	{
		UpdateRotorWashVolume( m_pRotorSound, m_engineThrust * GetRotorVolume() * (1.0f - flNearFactor), 3.0f );
	}

	if ( m_pNearRotorSound )
	{
		UpdateRotorWashVolume( m_pNearRotorSound, m_engineThrust * GetRotorVolume() * flNearFactor, 3.0f );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateRotorSoundPitch( int iPitch )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	float rotorPitch = 0.2 + m_engineThrust * 0.8;
	if ( m_pRotorSound )
	{
		controller.SoundChangePitch( m_pRotorSound, iPitch + rotorPitch, 0.1 );
	}

	if ( m_pNearRotorSound )
	{
		controller.SoundChangePitch( m_pNearRotorSound, iPitch + rotorPitch, 0.1 );
	}

	if (m_pRotorOnGroundSound)
	{
		controller.SoundChangePitch( m_pRotorOnGroundSound, iPitch + rotorPitch, 0.1 );
	}

	UpdateRotorWashVolume();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSoldiers - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::CalculateSoldierCount( int iSoldiers )
{
	if ( m_iCrateType >= 0 ) 
	{
		m_soldiersToDrop = clamp( iSoldiers, 0, DROPSHIP_MAX_SOLDIERS );
	}
	else
	{
		m_soldiersToDrop = 0;
	}
}

//------------------------------------------------------------------------------
// Purpose : Leave crate being carried
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputLandLeave( inputdata_t &inputdata )
{
	CalculateSoldierCount( inputdata.value.Int() );
	m_leaveCrate = true;
	LandCommon();
}

//------------------------------------------------------------------------------
// Purpose : Take crate being carried to next point
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputLandTake( inputdata_t &inputdata )
{
	CalculateSoldierCount( inputdata.value.Int() );
	m_leaveCrate = false;
	LandCommon();
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   : bHover - If true, means we're landing on a hover point, not the ground
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::LandCommon( bool bHover )
{
	// If we don't have a crate, we're not able to land
	if ( !m_hContainer && !bHover )
		return;

	//DevMsg( "Landing\n" );

	if( bHover )
	{
		SetLandingState( LANDING_HOVER_LEVEL_OUT );
	}
	else
	{
		SetLandingState( LANDING_LEVEL_OUT );
	}

	SetLocalAngularVelocity( vec3_angle );

	// Do we have a land target?
	if ( m_iszLandTarget != NULL_STRING )
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_iszLandTarget );
		if ( !pTarget )
		{
			Warning("npc_combinedropship %s couldn't find land target named %s\n", STRING(GetEntityName()), STRING(m_iszLandTarget) );
			return;
		}

		// Start heading to the point
		m_hLandTarget = pTarget;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputSetLandTarget( inputdata_t &inputdata )
{
	m_iszLandTarget = inputdata.value.StringID();
}

//------------------------------------------------------------------------------
// Purpose : Drop mine inputs... done this way so generic path_corners can be used
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputDropMines( inputdata_t &inputdata )
{
	m_totalMinesToDrop = inputdata.value.Int();
	if ( m_totalMinesToDrop >= 1 )	// catch bogus values being passed in
	{
		m_bDropMines = true;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputDropStrider( inputdata_t &inputdata )
{
	if ( !m_hContainer || !FClassnameIs( m_hContainer, "npc_strider" ) )
	{
		Warning("npc_combinedropship %s was told to drop Strider, but isn't carrying one!\n", STRING(GetEntityName()) );
		return;
	}

	QAngle angles = GetAbsAngles();

	angles.x = 0.0;
	angles.z = 0.0;

	m_hContainer->SetParent(NULL, 0);
	m_hContainer->SetOwnerEntity(NULL);
	m_hContainer->SetAbsAngles( angles );
	m_hContainer->SetAbsVelocity( vec3_origin );

	m_hContainer = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputDropAPC( inputdata_t &inputdata )
{
	if ( !m_hContainer || !FClassnameIs( m_hContainer, "prop_vehicle_apc" ) )
	{
		Warning("npc_combinedropship %s was told to drop APC, but isn't carrying one!\n", STRING(GetEntityName()) );
		return;
	}

	m_hContainer->SetParent(NULL, 0);
//	m_hContainer->SetOwnerEntity(NULL);

	Vector vecAbsVelocity = GetAbsVelocity();
	if ( vecAbsVelocity.z > 0 )
	{
		vecAbsVelocity.z = 0.0f;
	}
	if ( m_hContainer->GetHealth() > 0 )
	{
		vecAbsVelocity = vec3_origin;
	}

	m_hContainer->SetAbsVelocity( vecAbsVelocity );
	m_hContainer->SetMoveType( (MoveType_t)m_iContainerMoveType );

	// If the container has a physics object, remove it's shadow
	IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->RemoveShadowController();
	}

	UTIL_SetSize( this, DROPSHIP_BBOX_MIN, DROPSHIP_BBOX_MAX );

	m_hContainer = NULL;
	m_OnFinishedDropoff.FireOutput( this, this );
	SetLandingState( LANDING_NO );
	m_hLandTarget = NULL;
}


//-----------------------------------------------------------------------------
// Drop the soldier container
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::DropSoldierContainer( )
{
	m_hContainer->SetParent(NULL, 0);
//	m_hContainer->SetOwnerEntity(NULL);

	Vector vecAbsVelocity = GetAbsVelocity();
	if ( vecAbsVelocity.z > 0 )
	{
		vecAbsVelocity.z = 0.0f;
	}

	m_hContainer->SetAbsVelocity( vecAbsVelocity );
	m_hContainer->SetMoveType( MOVETYPE_VPHYSICS );

	// If we have a troop in the process of exiting, kill him.
	// We do this to avoid having to solve the AI problems resulting from it.
	if ( m_hLastTroopToLeave )
	{
		CTakeDamageInfo dmgInfo( this, this, vec3_origin, m_hContainer->GetAbsOrigin(), m_hLastTroopToLeave->GetMaxHealth(), DMG_GENERIC );
		m_hLastTroopToLeave->TakeDamage( dmgInfo );
	}

	// If the container has a physics object, remove it's shadow
	IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->RemoveShadowController();
		pPhysicsObject->SetVelocity( &vecAbsVelocity, &vec3_origin );
	}

	UTIL_SetSize( this, DROPSHIP_BBOX_MIN, DROPSHIP_BBOX_MAX );

	m_hContainer = NULL;
	SetLandingState( LANDING_NO );
	m_hLandTarget = NULL;

	if ( m_bHasDroppedOff )
	{
		m_OnContainerShotDownAfterDropoff.FireOutput( this, this );
	}
	else
	{
		int iTroopsNotUnloaded = (m_soldiersToDrop - m_iCurrentTroopExiting);
		if ( g_debug_dropship.GetInt() )
		{
			Msg("Dropship died, troops not unloaded: %d\n", iTroopsNotUnloaded );
		}

		m_OnContainerShotDownBeforeDropoff.Set( iTroopsNotUnloaded, this, this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Pick up a specified object
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputPickup( inputdata_t &inputdata )
{
	// Can't pickup if we're already carrying something
	if ( m_hContainer )
	{
		Warning("npc_combinedropship %s was told to pickup, but is already carrying something.\n", STRING(GetEntityName()) );
		return;
	}

	string_t iszTargetName = inputdata.value.StringID();
	if ( iszTargetName == NULL_STRING )
	{
		Warning("npc_combinedropship %s tried to pickup with no specified pickup target.\n", STRING(GetEntityName()) );
		return;
	}
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, iszTargetName );
	if ( !pTarget )
	{
		Warning("npc_combinedropship %s couldn't find pickup target named %s\n", STRING(GetEntityName()), STRING(iszTargetName) );
		return;
	}

	// Start heading to the point
	m_hPickupTarget = pTarget;

	m_bHasDroppedOff = false;

	// Disable collisions to my target
	m_hPickupTarget->SetOwnerEntity(this);
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		SetState( NPC_STATE_ALERT );
	}
	SetLandingState( LANDING_SWOOPING );
	m_flLandingSpeed = GetAbsVelocity().Length();

	UpdatePickupNavigation();
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the container's gun
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputSetGunRange( inputdata_t &inputdata )
{
	m_flGunRange = inputdata.value.Float();
}


//------------------------------------------------------------------------------
// Set the landing state
//------------------------------------------------------------------------------
void CNPC_CombineDropship::SetLandingState( LandingState_t landingState )
{
	if ( landingState == m_iLandState )
		return;

	if ( m_pDescendingWarningSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		if ( ( landingState == LANDING_DESCEND ) || ( landingState == LANDING_TOUCHDOWN ) || ( landingState == LANDING_UNLOADING ) || ( landingState == LANDING_UNLOADED ) || ( landingState == LANDING_HOVER_DESCEND ) )
		{
			controller.SoundChangeVolume( m_pDescendingWarningSound, m_bSuppressSound ? 0.0f : 1.0f, 0.3f );
		}
		else
		{
			controller.SoundChangeVolume( m_pDescendingWarningSound, 0.0f, 0.0f );
		}
	}
	
	m_iLandState = landingState;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CNPC_CombineDropship::IsHovering()
{
	bool bIsHovering = false;

	if( GetLandingState() > LANDING_START_HOVER && GetLandingState() < LANDING_END_HOVER )
	{
		bIsHovering = true;
	}

	return bIsHovering;
}

//------------------------------------------------------------------------------
// Update the ground rotorwash volume
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateGroundRotorWashSound( float flAltitude )
{
	float flVolume = RemapValClamped( flAltitude, DROPSHIP_GROUND_WASH_MIN_ALTITUDE, DROPSHIP_GROUND_WASH_MAX_ALTITUDE, 1.0f, 0.0f );
	UpdateRotorWashVolume( m_pRotorOnGroundSound, flVolume * GetRotorVolume(), 0.5f );
}


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::PrescheduleThink( void )
{	
	BaseClass::PrescheduleThink();

	// "npc_kill" destroys our container
	if (m_debugOverlays & OVERLAY_NPC_KILL_BIT)
	{
		if ( m_hContainer )
		{
			CTakeDamageInfo dmgInfo( this, this, vec3_origin, vec3_origin, 1000, DMG_BLAST );
			m_hContainer->TakeDamage( dmgInfo );
		}
	}

	// Update the ground rotorwash volume
	float flAltitude = GetAltitude();
	UpdateGroundRotorWashSound( flAltitude );

	// keep track of think time deltas for burn calc below
	float dt = gpGlobals->curtime - m_flLastTime;
	m_flLastTime = gpGlobals->curtime;

	switch( GetLandingState() )
	{
	case LANDING_NO:
		{
			if ( IsActivityFinished() && (GetActivity() != ACT_DROPSHIP_FLY_IDLE_EXAGG && GetActivity() != ACT_DROPSHIP_FLY_IDLE_CARGO) )
			{
				if ( m_hContainer )
				{
					SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_CARGO );
				}
				else
				{
					SetIdealActivity( (Activity)ACT_DROPSHIP_FLY_IDLE_EXAGG );
				}
			}

			DoRotorWash();
		}
		break;

	case LANDING_LEVEL_OUT:
	case LANDING_HOVER_LEVEL_OUT:
		{
			// Approach the drop point
			Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
			float flDistance = vecToTarget.Length();

			// Are we there yet?
			float flSpeed = GetAbsVelocity().Length();
			if ( flDistance < 70 && flSpeed < 100 )
			{
				m_flLandingSpeed = flSpeed;

				if( IsHovering() )
				{
					SetLandingState( LANDING_HOVER_DESCEND );
				}
				else
				{
					SetLandingState( LANDING_DESCEND );
				}

				// save off current angles so we can work them out over time
				QAngle angles = GetLocalAngles();
				m_existPitch = angles.x;
				m_existRoll = angles.z;
			}

			DoRotorWash();
		}
		break;

	case LANDING_DESCEND:
	case LANDING_HOVER_DESCEND:
		{
			/*
			if ( IsActivityFinished() && GetActivity() != ACT_DROPSHIP_DESCEND_IDLE )
			{
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}
			*/

			if( IsHovering() && m_hLandTarget != NULL )
			{
				// We're trying to hover above an arbitrary point, not above the ground. 
				// Recompute flAltitude to indicate the vertical distance from the land 
				// target so that touchdown is correctly detected.
				flAltitude = GetAbsOrigin().z - m_hLandTarget->GetAbsOrigin().z;
			}

			// Orient myself to the desired direction
			bool bStillOrienting = false;
			Vector targetDir;
			if ( m_hLandTarget )
			{
				// We've got a land target, so match it's orientation
				AngleVectors( m_hLandTarget->GetAbsAngles(), &targetDir );
			}
			else
			{
				// No land target. 
				targetDir = GetDesiredPosition() - GetAbsOrigin();
			}

			// Don't unload until we're facing the way the dropoff point specifies
			float flTargetYaw = UTIL_VecToYaw( targetDir );
			float flDeltaYaw = UTIL_AngleDiff( flTargetYaw, GetAbsAngles().y );
			if ( fabs(flDeltaYaw) > 5 )
			{
				bStillOrienting = true;
			}

			// Ensure we land on the drop point. Stop dropping if we're still turning.
			Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
			float flDistance = vecToTarget.Length();
			float flRampedSpeed = m_flLandingSpeed * (flDistance / 70);
			Vector vecVelocity = (flRampedSpeed / flDistance) * vecToTarget;
			
#define MAX_LAND_VEL	-300.0f
#define MIN_LAND_VEL	-75.0f
#define ALTITUDE_CAP	512.0f

			float flFactor = MIN( 1.0,  flAltitude / ALTITUDE_CAP );
			float flDescendVelocity = MIN( -75, MAX_LAND_VEL * flFactor );

			vecVelocity.z = flDescendVelocity;

			SetAbsVelocity( vecVelocity );

			if ( flAltitude < 72 )
			{
				QAngle angles = GetLocalAngles();

				// Level out quickly.
				angles.x = UTIL_Approach( 0.0, angles.x, 0.2 );
				angles.z = UTIL_Approach( 0.0, angles.z, 0.2 );

				SetLocalAngles( angles );
			}
			else
			{
				// randomly move as if buffeted by ground effects
				// gently flatten ship from starting pitch/yaw
				m_existPitch = UTIL_Approach( 0.0, m_existPitch, 1 );
				m_existRoll = UTIL_Approach( 0.0, m_existRoll, 1 );

				QAngle angles = GetLocalAngles();
				angles.x = m_existPitch + ( sin( gpGlobals->curtime * 3.5f ) * DROPSHIP_MAX_LAND_TILT );
				angles.z = m_existRoll + ( sin( gpGlobals->curtime * 3.75f ) * DROPSHIP_MAX_LAND_TILT );
				SetLocalAngles( angles );
			}

			DoRotorWash();

			// place danger sounds 1 foot above ground to get troops to scatter if they are below dropship
			Vector vecBottom = GetAbsOrigin();
			vecBottom.z += WorldAlignMins().z;
			Vector vecSpot = vecBottom + Vector(0, 0, -1) * (flAltitude - 12 );
			CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this, 0 );
			CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, vecSpot, 400, 0.1, this, 1 );
//			NDebugOverlay::Cross3D( vecSpot, -Vector(4,4,4), Vector(4,4,4), 255, 0, 255, false, 10.0f );

			// now check to see if player is below us, if so, cause heat damage to them (i.e. get them to move)
			trace_t tr;
			Vector vecBBoxMin = CRATE_BBOX_MIN;		// use flat box for check
			vecBBoxMin.z = -5;
			Vector vecBBoxMax = CRATE_BBOX_MAX;
			vecBBoxMax.z = 5;
			Vector pEndPoint = vecBottom + Vector(0, 0, -1) * ( flAltitude - 12 );
			AI_TraceHull( vecBottom, pEndPoint, vecBBoxMin, vecBBoxMax, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction < 1.0f )
			{
				// Damage anything that's blocking me
				if ( tr.m_pEnt && tr.m_pEnt->m_takedamage != DAMAGE_NO )
				{
					CTakeDamageInfo info( this, this, 20 * dt, DMG_BURN );
					tr.m_pEnt->TakeDamage( info );
				}
			}

			if ( !bStillOrienting && ((flAltitude <= 0.5f) || (m_iCrateType == CRATE_APC)) )
			{
				if( IsHovering() )
				{
					SetAbsVelocity( vec3_origin );
					SetLandingState( LANDING_HOVER_TOUCHDOWN );
				}
				else
				{
					SetLandingState( LANDING_TOUCHDOWN );
				}

				// upon landing, make sure ship is flat
				QAngle angles = GetLocalAngles();
				angles.x = 0;
				angles.z = 0;
				SetLocalAngles( angles );

				// TODO: Release cargo anim
				//SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
				return;
			}
		}
		break;

	case LANDING_TOUCHDOWN:
	case LANDING_HOVER_TOUCHDOWN:
		{
			/*
			if ( IsActivityFinished() && ( GetActivity() != ACT_DROPSHIP_DESCEND_IDLE ) )
			{
				SetActivity( (Activity)ACT_DROPSHIP_DESCEND_IDLE );
			}
			*/

			// Wait here if we're supposed to wait for the dropoff input
			if ( m_bWaitForDropoffInput )
				return;

			// Wait here till designer tells us to get moving again.
			if ( IsHovering() )
				return;

			SetLandingState( LANDING_UNLOADING );

			// If we're dropping off troops, we'll wait for them to be done. 
			// Otherwise, just pause on the ground for a few seconds and then leave.
			if ( m_soldiersToDrop && m_hContainer)
			{
				m_flTimeTakeOff = 0;
				m_flNextTroopSpawnAttempt = 0;

				// Open our container
				m_hContainer->SetSequence( m_hContainer->LookupSequence("open_idle") );

				// Start unloading troops
				m_iCurrentTroopExiting = 0;
				SpawnTroop();
			}
			else
			{
				float flHoverTime = ( m_iCrateType >= 0 ) ? DROPSHIP_LANDING_HOVER_TIME : 0.5f;
				m_flTimeTakeOff = gpGlobals->curtime + flHoverTime;
			}
		}
		break;

	case LANDING_UNLOADING:
		{
			// If we've got no specified takeoff time, we're still waiting for troops to exit. Idle.
			if ( !m_flTimeTakeOff )
			{
				float idleVolume = 0.2f;
				m_engineThrust = UTIL_Approach( idleVolume, m_engineThrust, 0.04f );
				if ( m_engineThrust > idleVolume ) 
				{
					// Make sure we're kicking up dust/water as long as engine thrust is up
					DoRotorWash();				
				}

				// If we've lost the last troop who was leaving, he probably got killed during dustoff.
				if ( !m_hLastTroopToLeave || !m_hLastTroopToLeave->IsAlive() )
				{
					// If we still have troops onboard, spawn the next one
					if ( m_iCurrentTroopExiting < m_soldiersToDrop )
					{
						SpawnTroop();
					}
					else
					{
						// We're out of troops, time to leave
						m_flTimeTakeOff = gpGlobals->curtime + 0.5;
					}
				}
			}
			else
			{
				if( gpGlobals->curtime > m_flTimeTakeOff )
				{
					SetLandingState( LANDING_LIFTOFF );
					SetActivity( (Activity)ACT_DROPSHIP_LIFTOFF );
					m_engineThrust = 1.0f;			// ensure max volume once we're airborne
					if ( m_bIsFiring )
					{
						StopCannon();				// kill cannon sounds if they are on
					}

					// detach container from ship
					if ( m_hContainer && m_leaveCrate )
					{
						m_hContainer->SetParent(NULL);
						m_hContainer->SetMoveType( (MoveType_t)m_iContainerMoveType );

						Vector vecAbsVelocity( 0, 0, GetAbsVelocity().z );
						if ( vecAbsVelocity.z > 0 )
						{
							vecAbsVelocity.z = 0.0f;
						}

						m_hContainer->SetAbsVelocity( vecAbsVelocity );

						// If the container has a physics object, remove it's shadow
						IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
						if ( pPhysicsObject )
						{
							pPhysicsObject->RemoveShadowController();
							pPhysicsObject->SetVelocity( &vecAbsVelocity, &vec3_origin );
						}

						m_hContainer = NULL;
						UTIL_SetSize( this, DROPSHIP_BBOX_MIN, DROPSHIP_BBOX_MAX );
					}
				}
				else if ( (m_flTimeTakeOff - gpGlobals->curtime) < 0.5f )
				{
					// Manage engine wash and volume
					m_engineThrust = UTIL_Approach( 1.0f, m_engineThrust, 0.1f );
					DoRotorWash();
				}
			}
		}
		break;

	case LANDING_LIFTOFF:
		{
			// Once we're off the ground, start flying again
			if ( flAltitude > 120 )		
			{
				SetLandingState( LANDING_NO );
				m_hLandTarget = NULL;
				m_bHasDroppedOff = true;
				m_OnFinishedDropoff.FireOutput( this, this );
			}

			if ( m_hContainer )
			{
				m_hContainer->SetSequence( m_hContainer->LookupSequence("close_idle") );
			}
		}
		break;

	case LANDING_SWOOPING:
		{
			// Did we lose our pickup target?
			if ( !m_hPickupTarget )
			{
				SetLandingState( LANDING_NO );
			}
			else
			{
				// Decrease altitude and speed to hit the target point.
				Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
				float flDistance = vecToTarget.Length();

				// Start cheating when we get near it
				if ( flDistance < 50 )
				{
					/*
					if ( flDistance > 10 )
					{
						// Cheat and ensure we touch the target
						float flSpeed = GetAbsVelocity().Length();
						Vector vecVelocity = vecToTarget;
						VectorNormalize( vecVelocity );
						SetAbsVelocity( vecVelocity * min(flSpeed,flDistance) );
					}
					else
					*/
					{
						// Grab the target
						m_hContainer = m_hPickupTarget;
						m_hPickupTarget = NULL;
						m_iContainerMoveType = m_hContainer->GetMoveType();
						if ( m_bInvulnerable && m_hContainer )
						{
							m_hContainer->m_takedamage = DAMAGE_NO;
						}

						// If the container has a physics object, move it to shadow
						IPhysicsObject *pPhysicsObject = m_hContainer->VPhysicsGetObject();
						if ( pPhysicsObject )
						{
							pPhysicsObject->EnableMotion( true );
							pPhysicsObject->SetShadow( 1e4, 1e4, false, false );
							pPhysicsObject->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, 0 );
						}

						m_hContainer->SetParent(this, 0);
						m_hContainer->SetMoveType( MOVETYPE_PUSH );
						m_hContainer->SetGroundEntity( NULL );

						m_OnFinishedPickup.FireOutput( this, this );
						SetLandingState( LANDING_NO );
					}
				}
			}

			DoRotorWash();
		}
		break;
	}

	if ( !(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
	{
		DoCombatStuff();
	}

	if ( GetActivity() != GetIdealActivity() )
	{
		//DevMsg( "setactivity" );
		SetActivity( GetIdealActivity() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define DROPSHIP_WASH_ALTITUDE 1024.0

void CNPC_CombineDropship::DoRotorWash( void )
{
	Vector	vecForward;
	GetVectors( &vecForward, NULL, NULL );

	Vector vecRotorHub = GetAbsOrigin() + vecForward * -64;

	DrawRotorWash( DROPSHIP_WASH_ALTITUDE, vecRotorHub );
}


//------------------------------------------------------------------------------
// Purpose : Spawn the next NPC in our template list
//------------------------------------------------------------------------------
void CNPC_CombineDropship::SpawnTroop( void )
{
	if ( !m_hContainer )
	{
		// We're done, take off.
		m_flTimeTakeOff = gpGlobals->curtime + 0.5;
		return;
	}

	// Are we fully unloaded? If so, take off. Otherwise, tell the next troop to exit.
	if ( m_iCurrentTroopExiting >= m_soldiersToDrop || m_sNPCTemplateData[m_iCurrentTroopExiting] == NULL_STRING )
	{
		// We're done, take off.
		m_flTimeTakeOff = gpGlobals->curtime + 0.5;
		return;
	}

	m_hLastTroopToLeave = NULL;

	// Not time to try again yet?
	if ( m_flNextTroopSpawnAttempt > gpGlobals->curtime )
		return;

	// HACK: This is a nasty piece of work. We want to make sure the deploy end is clear, and has enough
	// room with our deploying NPC, but we don't want to create the NPC unless it's clear, and we don't
	// know how much room he needs without spawning him. 
	// So, because we know that we only ever spawn combine soldiers at the moment, we'll just use their hull.
	// HACK: Add some bloat because the endpoint isn't perfectly aligned with NPC end origin
	Vector vecNPCMins = NAI_Hull::Mins( HULL_HUMAN ) - Vector(4,4,4);
	Vector vecNPCMaxs = NAI_Hull::Maxs( HULL_HUMAN ) + Vector(4,4,4);

	// Scare NPCs away from our deploy endpoint to keep them away
	Vector vecDeployEndPoint;
	QAngle vecDeployEndAngles;
	m_hContainer->GetAttachment( m_iAttachmentTroopDeploy, vecDeployEndPoint, vecDeployEndAngles );
	vecDeployEndPoint = GetDropoffFinishPosition( vecDeployEndPoint, NULL, vecNPCMins, vecNPCMaxs );
	CSoundEnt::InsertSound( SOUND_DANGER, vecDeployEndPoint, 120.0f, 2.0f, this );

	// Make sure there are no NPCs on the spot
	trace_t tr;
	CTraceFilterOnlyNPCsAndPlayer filter( this, COLLISION_GROUP_NONE );
	AI_TraceHull( vecDeployEndPoint, vecDeployEndPoint, vecNPCMins, vecNPCMaxs, MASK_SOLID, &filter, &tr );
	if ( tr.m_pEnt )
	{
		if ( g_debug_dropship.GetInt() == 2 )
		{
			NDebugOverlay::Box( vecDeployEndPoint, vecNPCMins, vecNPCMaxs, 255,0,0, 64, 0.5 );
		}

		m_flNextTroopSpawnAttempt = gpGlobals->curtime + 1;
		return;
	}

	if ( g_debug_dropship.GetInt() == 2 )
	{
		NDebugOverlay::Box( vecDeployEndPoint, vecNPCMins, vecNPCMaxs, 0,255,0, 64, 0.5 );
	}

	// Get the spawn point inside the container
	Vector vecSpawnOrigin;
	QAngle vecSpawnAngles;
	m_hContainer->GetAttachment( m_iAttachmentDeployStart, vecSpawnOrigin, vecSpawnAngles );

	// Spawn the templated NPC
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_sNPCTemplateData[m_iCurrentTroopExiting]), NULL );

	// Increment troop count
	m_iCurrentTroopExiting++;

	if ( !pEntity )
	{
		Warning("Dropship could not create template NPC\n" );
		return;
	}
	CAI_BaseNPC	*pNPC = pEntity->MyNPCPointer();
	Assert( pNPC );

	// Spawn an entity blocker.
	CBaseEntity *pBlocker = CEntityBlocker::Create( vecDeployEndPoint, vecNPCMins, vecNPCMaxs, pNPC, true );
	g_EventQueue.AddEvent( pBlocker, "Kill", 2.5, this, this );
	if ( g_debug_dropship.GetInt() == 2 )
	{
		NDebugOverlay::Box( vecDeployEndPoint, vecNPCMins, vecNPCMaxs, 255, 255, 255, 64, 2.5 );
	}

	// Ensure our NPCs are standing upright
	vecSpawnAngles[PITCH] = vecSpawnAngles[ROLL] = 0;

	// Move it to the container spawnpoint
	pNPC->SetAbsOrigin( vecSpawnOrigin );
	pNPC->SetAbsAngles( vecSpawnAngles );
	DispatchSpawn( pNPC );
	pNPC->m_NPCState = NPC_STATE_IDLE;
	pNPC->Activate();

	// Spawn a scripted sequence entity to make the NPC run out of the dropship
	CAI_ScriptedSequence *pSequence = (CAI_ScriptedSequence*)CreateEntityByName( "scripted_sequence" );
	pSequence->KeyValue( "m_iszEntity", STRING(pNPC->GetEntityName()) );
	pSequence->KeyValue( "m_iszPlay", "Dropship_Deploy" );
	pSequence->KeyValue( "m_fMoveTo", "4" );	// CINE_MOVETO_TELEPORT
	pSequence->KeyValue( "OnEndSequence", UTIL_VarArgs("%s,NPCFinishDustoff,%s,0,-1", STRING(GetEntityName()), STRING(pNPC->GetEntityName())) );
	pSequence->SetAbsOrigin( vecSpawnOrigin );
	pSequence->SetAbsAngles( vecSpawnAngles );
	pSequence->AddSpawnFlags( SF_SCRIPT_NOINTERRUPT | SF_SCRIPT_HIGH_PRIORITY | SF_SCRIPT_OVERRIDESTATE );
	pSequence->Spawn();
	pSequence->Activate();
	variant_t emptyVariant;
	pSequence->AcceptInput( "BeginSequence", this, this, emptyVariant, 0 );

	m_hLastTroopToLeave = pNPC;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a safe position above/below the specified origin for the NPC to finish it's dropoff on
// Input  : vecOrigin - 
//-----------------------------------------------------------------------------
Vector CNPC_CombineDropship::GetDropoffFinishPosition( Vector vecOrigin, CAI_BaseNPC *pNPC, Vector vecMins, Vector vecMaxs )
{
	// Use the NPC's if they're specified
	if ( pNPC )
	{
		vecMins = NAI_Hull::Mins( pNPC->GetHullType() );
		vecMaxs = NAI_Hull::Maxs( pNPC->GetHullType() );
	}

	trace_t tr;
	AI_TraceHull( vecOrigin + Vector(0,0,32), vecOrigin, vecMins, vecMaxs, MASK_SOLID, pNPC, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0 )
	{
		if ( g_debug_dropship.GetInt() == 1 )
		{
			NDebugOverlay::Box( vecOrigin, vecMins, vecMaxs, 255,0,0, 8, 4.0 );
		}

		// Try and find the ground
		AI_TraceHull( vecOrigin + Vector(0,0,32), vecOrigin, vecMins, vecMaxs, MASK_SOLID, pNPC, COLLISION_GROUP_NONE, &tr );
		if ( !tr.startsolid )
			return (tr.endpos + Vector(0,0,1));
	}
	else if ( g_debug_dropship.GetInt() == 1 )
	{
		NDebugOverlay::Box( vecOrigin, vecMins, vecMaxs, 0,255,0, 8, 4.0 );
	}

	return vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: A troop we dropped of has now finished the scripted sequence
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputNPCFinishDustoff( inputdata_t &inputdata )
{
	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, inputdata.value.StringID(), NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pEnt )
		return;

	CAI_BaseNPC *pNPC = pEnt->MyNPCPointer();
	Assert( pNPC );

	Vector vecOrigin = GetDropoffFinishPosition( pNPC->GetAbsOrigin(), pNPC, vec3_origin, vec3_origin );
	pNPC->SetAbsOrigin( vecOrigin );

	// Do we have a dustoff point?
	CBaseEntity *pDustoff = NULL;
	if ( m_sDustoffPoints[m_iCurrentTroopExiting-1] != NULL_STRING )
	{
		pDustoff = gEntList.FindEntityByName( NULL, m_sDustoffPoints[m_iCurrentTroopExiting-1] );
		if ( !pDustoff )
		{
			Warning("npc_combinedropship %s couldn't find dustoff target named %s\n", STRING(GetEntityName()), STRING(m_sDustoffPoints[m_iCurrentTroopExiting-1]) );
		}
	}

	if ( !pDustoff )
	{
		// Make a move away sound to scare the combine away from this point
		CSoundEnt::InsertSound( SOUND_MOVE_AWAY | SOUND_CONTEXT_COMBINE_ONLY, pNPC->GetAbsOrigin(), 128, 0.1 );
	}
	else
	{
		if ( g_debug_dropship.GetInt() == 1 )
		{
			NDebugOverlay::Box( pDustoff->GetAbsOrigin(), -Vector(10,10,10), Vector(10,10,10), 0,255,0, 8, 5.0 );
		}

		// Tell the NPC to move to the dustoff position
		pNPC->SetState( NPC_STATE_ALERT );
		pNPC->ScheduledMoveToGoalEntity( SCHED_DROPSHIP_DUSTOFF, pDustoff, ACT_RUN );
		pNPC->GetNavigator()->SetArrivalDirection( pDustoff->GetAbsAngles() );

		// Make sure they ignore a bunch of conditions
		static int g_Conditions[] = 
		{
			COND_CAN_MELEE_ATTACK1,
			COND_CAN_MELEE_ATTACK2,
			COND_CAN_RANGE_ATTACK1,
			COND_CAN_RANGE_ATTACK2,
			COND_ENEMY_DEAD,
			COND_HEAR_BULLET_IMPACT,
			COND_HEAR_COMBAT,
			COND_HEAR_DANGER,
			COND_NEW_ENEMY,
			COND_PROVOKED,
			COND_SEE_ENEMY,
			COND_SEE_FEAR,
			COND_SMELL,
			COND_LIGHT_DAMAGE,
			COND_HEAVY_DAMAGE,
			COND_PHYSICS_DAMAGE,
			COND_REPEATED_DAMAGE,
		};
		pNPC->SetIgnoreConditions( g_Conditions, ARRAYSIZE(g_Conditions) );
	}

	// Unload the next troop
	SpawnTroop();
}

//-----------------------------------------------------------------------------
// Purpose: Tells the dropship to stop waiting and dustoff
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::InputStopWaitingForDropoff( inputdata_t &inputdata )
{
	m_bWaitForDropoffInput = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputHover( inputdata_t &inputdata )
{
	m_iszLandTarget = inputdata.value.StringID();
	LandCommon( true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CombineDropship::InputFlyToPathTrack( inputdata_t &inputdata )
{
	if( IsHovering() )
	{
		SetLandingState( LANDING_NO );
		m_hLandTarget = NULL;
	}

	CAI_TrackPather::InputFlyToPathTrack( inputdata );
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_CombineDropship::GetAltitude( void )
{
	trace_t tr;
	Vector vecBottom = GetAbsOrigin();

	// Uneven terrain causes us problems, so trace our box down
	AI_TraceEntity( this, vecBottom, vecBottom - Vector(0,0,4096), MASK_SOLID_BRUSHONLY, &tr );

	float flAltitude = ( 4096 * tr.fraction );
	//DevMsg(" Altitude: %.3f\n", flAltitude );
	return flAltitude;
}


//-----------------------------------------------------------------------------
// Purpose: Drop rollermine from dropship
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::DropMine( void )
{
	NPC_Rollermine_DropFromPoint( GetAbsOrigin(), this, STRING(m_sRollermineTemplateData) );
}

//------------------------------------------------------------------------------
// Purpose : Fly towards our pickup target
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdatePickupNavigation( void )
{
	// Try and touch the top of the object
	Vector vecPickup = m_hPickupTarget->WorldSpaceCenter();
	vecPickup.z += (m_hPickupTarget->CollisionProp()->OBBSize().z * 0.5);
	SetDesiredPosition( vecPickup );

	//NDebugOverlay::Cross3D( GetDesiredPosition(), -Vector(32,32,32), Vector(32,32,32), 0, 255, 255, true, 0.1f );
}

//------------------------------------------------------------------------------
// Purpose : Fly towards our land target
//------------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateLandTargetNavigation( void )
{
	Vector vecPickup = m_hLandTarget->WorldSpaceCenter();
	vecPickup.z += 256;
	SetDesiredPosition( vecPickup );

	//NDebugOverlay::Cross3D( GetDesiredPosition(), -Vector(32,32,32), Vector(32,32,32), 0, 255, 255, true, 0.1f );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::Hunt( void )
{
	// If we have a pickup target, fly to it
	if ( m_hPickupTarget )
	{
		UpdatePickupNavigation();
	}
	else if ( m_hLandTarget )
	{
		UpdateLandTargetNavigation();
	}
	else if ( GetLandingState() == LANDING_NO )
	{
		UpdateTrackNavigation();
	}

	// don't face player ever, only face nav points
	Vector desiredDir = GetDesiredPosition() - GetAbsOrigin();
	VectorNormalize( desiredDir ); 
	// Face our desired position.
	m_vecDesiredFaceDir = desiredDir;

	if ( GetLandingState() == LANDING_DESCEND || GetLandingState() == LANDING_LEVEL_OUT || IsHovering() )
	{
		if ( m_hLandTarget )
		{
			// We've got a land target, so match it's orientation
			AngleVectors( m_hLandTarget->GetAbsAngles(), &m_vecDesiredFaceDir );
		}
		else
		{
			// No land target. 
			m_vecDesiredFaceDir = GetDesiredPosition() - GetAbsOrigin();
		}
	}

	UpdateEnemy();
	Flight();

	UpdatePlayerDopplerShift( );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	BaseClass::GatherEnemyConditions(pEnemy);

	// If we can't see the enemy for a few seconds, consider him unreachable
	if ( !HasCondition(COND_SEE_ENEMY) )
	{
		if ( gpGlobals->curtime - GetEnemyLastTimeSeen() >= 3.0f )
		{
			MarkEnemyAsEluded();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: do all of the stuff related to having an enemy, attacking, etc.
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::DoCombatStuff( void )
{
	// Handle mines
	if ( m_bDropMines )
	{
		switch( m_iDropState )
		{
		case DROP_IDLE:
			{
				m_iMineCount = m_totalMinesToDrop - 1;

				DropMine();
				// setup next individual drop time
				m_flDropDelay = gpGlobals->curtime + DROPSHIP_TIME_BETWEEN_MINES;
				// get ready to drop next mine, unless we're only supposed to drop 1
				if ( m_iMineCount )
				{
					m_iDropState = DROP_NEXT;
				}
				else
				{
					m_bDropMines = false;		// no more...
				}
				break;
			}
		case DROP_NEXT:
			{
				if ( gpGlobals->curtime > m_flDropDelay )		// time to drop next mine?
				{
					DropMine();
					m_flDropDelay = gpGlobals->curtime + DROPSHIP_TIME_BETWEEN_MINES;

					m_iMineCount--;
					if ( !m_iMineCount )
					{
						m_iDropState = DROP_IDLE;
						m_bDropMines = false;		// reset flag
					}
				}
				break;
			}
		}
	}

	// Handle guns
	bool bStopGun = true;
	if ( GetEnemy() )
	{
		bStopGun = !FireCannonRound();
	}

	if ( bStopGun && m_bIsFiring )
	{
		StopCannon();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the container's gun to face the enemy. 
// Input  : &vecMuzzle - The gun's muzzle/firing point
//			&vecAimDir - The gun's current aim direction
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::UpdateContainerGunFacing( Vector &vecMuzzle, Vector &vecToTarget, Vector &vecAimDir, float *flTargetRange )
{
	Assert( m_hContainer );

	// Get the desired aim vector
	vecToTarget = GetEnemy()->WorldSpaceCenter( );

	Vector vecBarrelPos, vecWorldBarrelPos;
	QAngle worldBarrelAngle, vecAngles;
	matrix3x4_t matRefToWorld;
	m_hContainer->GetAttachment( m_iMuzzleAttachment, vecMuzzle, vecAngles );
	vecWorldBarrelPos = vecMuzzle;
	worldBarrelAngle = vecAngles;
	m_hContainer->GetAttachment( m_iMachineGunRefAttachment, matRefToWorld );
	VectorITransform( vecWorldBarrelPos, matRefToWorld, vecBarrelPos );

	EntityMatrix parentMatrix;
	parentMatrix.InitFromEntity( m_hContainer, m_iMachineGunBaseAttachment );
	Vector target = parentMatrix.WorldToLocal( vecToTarget ); 

	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x*target.x + target.y*target.y;

	// Target is too close!  Can't aim at it
	if ( quadTarget > vecBarrelPos.LengthSqr() )
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
		float centerToGunYaw = atan2( vecBarrelPos.y, sqrt( quadTarget - (vecBarrelPos.y*vecBarrelPos.y) ) );

		float targetToCenterPitch = atan2( target.z, sqrt( quadTargetXY ) );
		float centerToGunPitch = atan2( -vecBarrelPos.z, sqrt( quadTarget - (vecBarrelPos.z*vecBarrelPos.z) ) );

		QAngle angles;
		angles.Init( RAD2DEG(targetToCenterPitch+centerToGunPitch), RAD2DEG( targetToCenterYaw + centerToGunYaw ), 0 );

		float flNewAngle = AngleNormalize( UTIL_ApproachAngle( angles.x, m_hContainer->GetPoseParameter(m_poseWeapon_Pitch), DROPSHIP_GUN_SPEED));
		m_hContainer->SetPoseParameter( m_poseWeapon_Pitch, flNewAngle );

		flNewAngle = AngleNormalize( UTIL_ApproachAngle( angles.y, m_hContainer->GetPoseParameter(m_poseWeapon_Yaw), DROPSHIP_GUN_SPEED));
		m_hContainer->SetPoseParameter( m_poseWeapon_Yaw, flNewAngle );
		m_hContainer->StudioFrameAdvance();
	}

	vecToTarget -= vecMuzzle;
	*flTargetRange = VectorNormalize( vecToTarget );
	AngleVectors( vecAngles, &vecAimDir );
}


//------------------------------------------------------------------------------
// Purpose: Fire a round from the cannon
// Notes:	Only call this if you have an enemy.
//			Returns true if the cannon round was actually fired
//------------------------------------------------------------------------------
bool CNPC_CombineDropship::FireCannonRound( void )
{
	// Try and aim my cannon at the enemy, if I have a container
	if ( !m_hContainer || (m_iCrateType < 0) )
		return false;

	// Update the container gun, and get the vector to the enemy, and the gun's current aim direction
	float flRange;
	Vector vecMuzzle, vecAimDir, vecToEnemy;
	UpdateContainerGunFacing( vecMuzzle, vecToEnemy, vecAimDir, &flRange );
	
	// Out of range?
	if ( flRange > m_flGunRange )
		return false;

	// Only fire if the target's close enough to our aim direction
	float flCosAngle = DotProduct( vecToEnemy, vecAimDir );
	if ( flCosAngle < DOT_15DEGREE )
	{
		m_flTimeNextAttack = gpGlobals->curtime + 0.1;
		return false;
	}

	// If we're out of rounds, reload
	if ( m_iBurstRounds <= 0 )
	{
		m_iBurstRounds = RandomInt( 10, 20 );
		m_flTimeNextAttack = gpGlobals->curtime + (m_iBurstRounds * 0.1);
		return false;
	}

	// HACK: Return true so the fire sound isn't stopped
	if ( m_flTimeNextAttack > gpGlobals->curtime )
		return true;

	m_iBurstRounds--;

	// If we're not currently firing, start it up
	if ( !m_bIsFiring )
	{
		StartCannon();
	}

	// Add a muzzle flash
	QAngle vecAimAngles;
	VectorAngles( vecAimDir, vecAimAngles );
	g_pEffects->MuzzleFlash( vecMuzzle, vecAimAngles, random->RandomFloat( 5.0f, 7.0f ), MUZZLEFLASH_TYPE_GUNSHIP );
	m_flTimeNextAttack = gpGlobals->curtime + 0.05;

	// Clamp to account for inaccuracy in aiming w/ pose parameters
	vecAimDir = vecToEnemy;

	// Fire the bullet
	int ammoType = GetAmmoDef()->Index("CombineCannon"); 
	FireBullets( 1, vecMuzzle, vecAimDir, VECTOR_CONE_2DEGREES, 8192, ammoType, 1, -1, -1, sk_npc_dmg_dropship.GetInt() );

	return true;
}

//------------------------------------------------------------------------------
// Scare AIs in the area where bullets are impacting
//------------------------------------------------------------------------------
void CNPC_CombineDropship::DoImpactEffect( trace_t &tr, int nDamageType )
{
	CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_REACT_TO_SOURCE, tr.endpos, 120.0f, 0.3f, this );

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//------------------------------------------------------------------------------
// Purpose : The proper way to begin the gunship cannon firing at the enemy.
// Input   : 
//		   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::StartCannon( void )
{
	m_bIsFiring = true;

	// Start up the cannon sound.
	if ( m_pCannonSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume(m_pCannonSound, 1.0, 0.0);
	}

}

//------------------------------------------------------------------------------
// Purpose : The proper way to cease the gunship cannon firing. 
// Input   : 
//		   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineDropship::StopCannon( void )
{
	m_bIsFiring = false;

	// Stop the cannon sound.
	if ( m_pCannonSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume(m_pCannonSound, 0.0, 0.1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used the gunship's tracer for now
//-----------------------------------------------------------------------------
void CNPC_CombineDropship::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	switch ( iTracerType )
	{
	case TRACER_LINE:
		{
			float flTracerDist;
			Vector vecDir;
			Vector vecEndPos;

			vecDir = tr.endpos - vecTracerSrc;

			flTracerDist = VectorNormalize( vecDir );

			UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 16000, true, "GunshipTracer" );
		}
		break;

	default:
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		break;
	}
}

AI_BEGIN_CUSTOM_NPC( npc_combinedropship, CNPC_CombineDropship )

	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE_EXAGG );
	DECLARE_ACTIVITY( ACT_DROPSHIP_DESCEND_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_DEPLOY_IDLE );
	DECLARE_ACTIVITY( ACT_DROPSHIP_LIFTOFF );

	DECLARE_ACTIVITY( ACT_DROPSHIP_FLY_IDLE_CARGO );

AI_END_CUSTOM_NPC()



