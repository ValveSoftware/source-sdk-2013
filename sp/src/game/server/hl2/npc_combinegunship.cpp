//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "ai_network.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_task.h"
#include "ai_motor.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "soundenvelope.h"
#include "gib.h"
#include "gamerules.h"
#include "ammodef.h"
#include "cbasehelicopter.h"
#include "npcevent.h"
#include "ndebugoverlay.h"
#include "decals.h"
#include "explode.h" // temp (sjb)
#include "smoke_trail.h" // temp (sjb)
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ar2_explosion.h"
#include "te_effect_dispatch.h"
#include "rope.h"
#include "effect_dispatch_data.h"
#include "trains.h"
#include "globals.h"
#include "physics_prop_ragdoll.h"
#include "iservervehicle.h"
#include "soundent.h"
#include "npc_citizen17.h"
#include "physics_saverestore.h"
#include "hl2_shareddefs.h"
#include "props.h"
#include "npc_attackchopper.h"
#include "citadel_effects_shared.h"
#include "eventqueue.h"
#include "beam_flags.h"
#include "ai_eventresponse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GUNSHIP_MSG_BIG_SHOT			1
#define GUNSHIP_MSG_STREAKS				2

#define GUNSHIP_NUM_DAMAGE_OUTPUTS		4

extern short		g_sModelIndexFireball;		// holds the index for the fireball

int g_iGunshipEffectIndex = -1;

#define GUNSHIP_ACCEL_RATE 500

// Spawnflags
#define SF_GUNSHIP_NO_GROUND_ATTACK		( 1 << 12 )	
#define SF_GUNSHIP_USE_CHOPPER_MODEL	( 1 << 13 )

ConVar sk_gunship_burst_size("sk_gunship_burst_size", "15" );
ConVar sk_gunship_burst_min("sk_gunship_burst_min", "800" );
ConVar sk_gunship_burst_dist("sk_gunship_burst_dist", "768" );

// Number of times the gunship must be struck by explosive damage
ConVar	sk_gunship_health_increments( "sk_gunship_health_increments", "0" );

/*

Wedge's notes:

  Gunship should move its head according to flight model when the target is behind the gunship,
  or when the target is too far away to shoot at. Otherwise, the head should aim at the target.

	Negative angvelocity.y is a RIGHT turn.
	Negative angvelocity.x is UP

*/

#define GUNSHIP_AP_MUZZLE	5

#define GUNSHIP_MAX_SPEED			1056.0f

#define GUNSHIP_MAX_FIRING_SPEED	200.0f
#define GUNSHIP_MIN_ROCKET_DIST		1000.0f
#define GUNSHIP_MAX_GUN_DIST		2000.0f
#define GUNSHIP_ARRIVE_DIST			128.0f

#define GUNSHIP_HOVER_SPEED			300.0f // play hover animation if moving slower than this.

#define GUNSHIP_AE_THRUST			1

#define GUNSHIP_HEAD_MAX_UP			-65
#define GUNSHIP_HEAD_MAX_DOWN		60
#define GUNSHIP_HEAD_MAX_LEFT		60
#define GUNSHIP_HEAD_MAX_RIGHT		-60

#define	BASE_STITCH_VELOCITY		800		//Units per second
#define	MAX_STITCH_VELOCITY			1000	//Units per second

#define	GUNSHIP_LEAD_DISTANCE		800.0f
#define	GUNSHIP_AVOID_DIST			512.0f
#define	GUNSHIP_STITCH_MIN			512.0f

#define	GUNSHIP_MIN_CHASE_DIST_DIFF	128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it

#define	MIN_GROUND_ATTACK_DIST			500.0f // Minimum distance a target has to be for the gunship to consider using the ground attack weapon
#define	MIN_GROUND_ATTACK_HEIGHT_DIFF	128.0f // Target's position and hit position must be within this threshold vertically

#define GUNSHIP_WASH_ALTITUDE		1024.0f

#define	GUNSHIP_MIN_DAMAGE_THRESHOLD	50.0f

#define GUNSHIP_INNER_NAV_DIST			400.0f
#define GUNSHIP_OUTER_NAV_DIST			800.0f

#define GUNSHIP_BELLYBLAST_TARGET_HEIGHT	512.0		// Height above targets that the gunship wants to be when bellyblasting

#define GUNSHIP_MISSILE_MAX_RESPONSE_TIME	0.4
#define GUNSHIP_MAX_HITS_PER_BURST			5

#define GUNSHIP_FLARE_IGNORE_TIME		6.0

//=====================================
// Custom activities
//=====================================
Activity ACT_GUNSHIP_PATROL;
Activity ACT_GUNSHIP_HOVER;
Activity ACT_GUNSHIP_CRASH;

#define	GUNSHIP_DEBUG_LEADING	1
#define	GUNSHIP_DEBUG_PATH		2
#define	GUNSHIP_DEBUG_STITCHING	3
#define GUNSHIP_DEBUG_BELLYBLAST 4

ConVar g_debug_gunship( "g_debug_gunship", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Dying gunship ragdoll controller
//-----------------------------------------------------------------------------
class CGunshipRagdollMotion : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();
public:
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
	{
		linear = Vector(0,0,400);
		angular = Vector(0,600,100);

		return SIM_GLOBAL_ACCELERATION;
	}
};

BEGIN_SIMPLE_DATADESC( CGunshipRagdollMotion )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetGunshipCrash : public CPointEntity
{
	DECLARE_CLASS( CTargetGunshipCrash, CPointEntity );
public:
	DECLARE_DATADESC();	

	void	InputEnable( inputdata_t &inputdata )
	{
		m_bDisabled = false;
	}
	void	InputDisable( inputdata_t &inputdata )
	{
		m_bDisabled = true;
	}
	bool	IsDisabled( void )
	{
		return m_bDisabled;
	}
	void	GunshipCrashedOnTarget( void )
	{
		m_OnCrashed.FireOutput( this, this );
	}

private:
	bool			m_bDisabled;

	COutputEvent	m_OnCrashed;
};

LINK_ENTITY_TO_CLASS( info_target_gunshipcrash, CTargetGunshipCrash );

BEGIN_DATADESC( CTargetGunshipCrash )
	DEFINE_FIELD( m_bDisabled, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs
	DEFINE_OUTPUT( m_OnCrashed,			"OnCrashed" ),
END_DATADESC()


//===================================================================
// Gunship - the combine dugongic like attack vehicle.
//===================================================================
class CNPC_CombineGunship : public CBaseHelicopter
{
public:
	DECLARE_CLASS( CNPC_CombineGunship, CBaseHelicopter );

	CNPC_CombineGunship( void );
	~CNPC_CombineGunship( void );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DEFINE_CUSTOM_AI;

	void	PlayPatrolLoop( void );
	void	PlayAngryLoop( void );

	void	Spawn( void );
	void	Precache( void );
	void	OnRestore( void );
	void	PrescheduleThink( void );
	void	HelicopterPostThink( void );
	void	StopLoopingSounds( void );

	bool	IsValidEnemy( CBaseEntity *pEnemy );
	void	GatherEnemyConditions( CBaseEntity *pEnemy );

	void	Flight( void );

	bool	FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	FireDamageOutputsUpto( int iDamageNumber );

	virtual float GetAcceleration( void ) { return 15; }

	virtual void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual void	DoImpactEffect( trace_t &tr, int nDamageType );

	void	MoveHead( void );
	void	UpdateDesiredPosition( void );
	void	DoCombat( void );
	bool	ChooseEnemy( void );
	void	DoMuzzleFlash( void );
	void	Ping( void );

	void	FireCannonRound( void );

	// Gunship death process
	void	Event_Killed( const CTakeDamageInfo &info );
	void	BeginCrash( void );				// I'm going to go to a crash point and die there
	void	BeginDestruct( void );			// I want to die now, so create my ragdoll
	void	SelfDestruct( void );			// I'm now fully dead, so remove myself.
	void	CreateSmokeTrail( void );
	bool	FindNearestGunshipCrash( void );
	
	int		BloodColor( void ) { return DONT_BLEED; }
	void	GibMonster( void );

	void	UpdateRotorSoundPitch( int iPitch );
	void	InitializeRotorSound( void );

	void	ApplyGeneralDrag( void );
	void	ApplySidewaysDrag( const Vector &vecRight );

	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	void	UpdateEnemyTarget( void );

	Vector	GetEnemyTarget( void );
	Vector	GetMissileTarget( void );

	float	GroundDistToPosition( const Vector &pos );

	bool	FireGun( void );
	bool	IsTargettingMissile( void );
	
	Class_T Classify( void ) { return CLASS_COMBINE_GUNSHIP; } // for now
	float	GetAutoAimRadius() { return 144.0f; }

	// Input functions
	void	InputSetPenetrationDepth( inputdata_t &inputdata );
	void	InputOmniscientOn( inputdata_t &inputdata );
	void	InputOmniscientOff( inputdata_t &inputdata );
	void	InputBlindfireOn( inputdata_t &inputdata );
	void	InputBlindfireOff( inputdata_t &inputdata );
	void	InputSelfDestruct( inputdata_t &inputdata );
	void	InputSetDockingBBox( inputdata_t &inputdata );
	void	InputSetNormalBBox( inputdata_t &inputdata );
	void	InputEnableGroundAttack( inputdata_t &inputdata );
	void	InputDisableGroundAttack( inputdata_t &inputdata );
	void	InputDoGroundAttack( inputdata_t &inputdata );
	
	//NOTENOTE: I'm rather queasy about adding these, as they can lead to nasty bugs...
	void	InputBecomeInvulnerable( inputdata_t &inputdata );
	void	InputBecomeVulnerable( inputdata_t &inputdata );

	bool	PoseGunTowardTargetDirection( const Vector &vTargetDir );
	void	StartCannonBurst( int iBurstSize );
	void	StopCannonBurst( void );

	bool	CheckGroundAttack( void );
	void	StartGroundAttack( void );
	void	StopGroundAttack( bool bDoAttack );
	Vector	GetGroundAttackHitPosition( void );
	void	DoGroundAttackExplosion( void );
	void	DrawRotorWash( float flAltitude, const Vector &vecRotorOrigin );

	void	ManageWarningBeam( void );
	void	DoBellyBlastDamage( trace_t &tr, Vector vMins, Vector vMaxs );

	// Updates the facing direction
	void	UpdateFacingDirection( void );
	void	CreateBellyBlastEnergyCore( void );

protected:
	// Because the combine gunship is a leaf class, we can use
	// static variables to store this information, and save some memory.
	// Should the gunship end up having inheritors, their activate may
	// stomp these numbers, in which case you should make these ordinary members
	// again.
	static int m_poseFlex_Horz, m_poseFlex_Vert, m_posePitch, m_poseYaw, m_poseFin_Accel, m_poseFin_Sway;
	static int m_poseWeapon_Pitch, m_poseWeapon_Yaw;

	static bool m_sbStaticPoseParamsLoaded;
	virtual void	PopulatePoseParameters( void );

private:
	// Outputs
	COutputEvent	m_OnFireCannon;
	COutputEvent	m_OnCrashed;

	COutputEvent	m_OnFirstDamage;	// First damage tick
	COutputEvent	m_OnSecondDamage;
	COutputEvent	m_OnThirdDamage;
	COutputEvent	m_OnFourthDamage;
	// Keep track of which damage outputs we've fired. This is necessary
	// to ensure that the game doesn't break if a mapmaker has outputs that
	// must be fired on gunships, and the player switches skill levels 
	// midway through a gunship battle.
	bool			m_bDamageOutputsFired[GUNSHIP_NUM_DAMAGE_OUTPUTS];	

	float	m_flNextGroundAttack;	// Time to wait before the next ground attack
	bool	m_bIsGroundAttacking;	// Denotes that we are ground attacking
	bool	m_bCanGroundAttack;		// Denotes whether we can ground attack or not
	float	m_flGroundAttackTime;	// Delay before blast happens from ground attack

	CHandle<SmokeTrail>	m_pSmokeTrail;
	EHANDLE			m_hGroundAttackTarget;

	CSoundPatch		*m_pAirExhaustSound;
	CSoundPatch		*m_pAirBlastSound;
	CSoundPatch		*m_pCannonSound;

	CBaseEntity		*m_pRotorWashModel;
	QAngle			m_vecAngAcceleration;

	float			m_flEndDestructTime;

	int				m_iDoSmokePuff;
	int				m_iAmmoType;
	int				m_iBurstSize;
	
	bool			m_fBlindfire;
	bool			m_fOmniscient;
	bool			m_bIsFiring;
	int				m_iBurstHits;
	bool			m_bPreFire;
	bool			m_bInvulnerable;

	float			m_flTimeNextPing;
	float			m_flPenetrationDepth; 
	float			m_flDeltaT;
	float			m_flTimeNextAttack;
	float			m_flNextSeeEnemySound;
	float			m_flNextRocket;
	float			m_flBurstDelay;

	Vector			m_vecAttackPosition;
	Vector			m_vecAttackVelocity;

	// Used when the gunships using the chopper model
	Vector			m_angGun;

	// For my death throes
	IPhysicsMotionController		*m_pCrashingController;
	CGunshipRagdollMotion			m_crashCallback;
	EHANDLE							m_hRagdoll;
	CHandle<CTargetGunshipCrash>	m_hCrashTarget;
	float							m_flNextGunshipCrashFind;

	CHandle<CCitadelEnergyCore>		m_hEnergyCore;

	CNetworkVector( m_vecHitPos );

	// If true, playing patrol loop.
	// Else, playing angry.
	bool			m_fPatrolLoopPlaying;
};

LINK_ENTITY_TO_CLASS( npc_combinegunship, CNPC_CombineGunship );

IMPLEMENT_SERVERCLASS_ST( CNPC_CombineGunship, DT_CombineGunship )
	SendPropVector(SENDINFO(m_vecHitPos), -1, SPROP_COORD),
END_SEND_TABLE()

BEGIN_DATADESC( CNPC_CombineGunship )

	DEFINE_ENTITYFUNC( FlyTouch ),

	DEFINE_FIELD( m_flNextGroundAttack,FIELD_TIME ),
	DEFINE_FIELD( m_bIsGroundAttacking,FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCanGroundAttack,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flGroundAttackTime,FIELD_TIME ),
	DEFINE_FIELD( m_pRotorWashModel,	FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pSmokeTrail,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hGroundAttackTarget, FIELD_EHANDLE ),
	DEFINE_SOUNDPATCH( m_pAirExhaustSound ),
	DEFINE_SOUNDPATCH( m_pAirBlastSound ),
	DEFINE_SOUNDPATCH( m_pCannonSound ),
	DEFINE_FIELD( m_vecAngAcceleration,FIELD_VECTOR ),
	DEFINE_FIELD( m_flDeltaT,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeNextAttack,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextSeeEnemySound,	FIELD_TIME ),
	DEFINE_FIELD( m_flEndDestructTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextRocket,		FIELD_TIME ),
	DEFINE_FIELD( m_iDoSmokePuff,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iAmmoType,			FIELD_INTEGER ),
	DEFINE_FIELD( m_iBurstSize,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flBurstDelay,		FIELD_FLOAT ),
	DEFINE_FIELD( m_fBlindfire,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fOmniscient,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsFiring,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBurstHits,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flTimeNextPing,	FIELD_TIME ),
	DEFINE_FIELD( m_flPenetrationDepth,FIELD_FLOAT ),
	DEFINE_FIELD( m_vecAttackPosition,	FIELD_VECTOR ),
	DEFINE_FIELD( m_vecAttackVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( m_angGun,				FIELD_VECTOR ),
	DEFINE_PHYSPTR( m_pCrashingController ),
	DEFINE_EMBEDDED( m_crashCallback ),
	DEFINE_FIELD( m_hRagdoll,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCrashTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecHitPos,			FIELD_VECTOR ),
	DEFINE_FIELD( m_fPatrolLoopPlaying,FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPreFire,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInvulnerable,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextGunshipCrashFind, FIELD_TIME ),

	DEFINE_FIELD( m_hEnergyCore, FIELD_EHANDLE ),

	DEFINE_ARRAY( m_bDamageOutputsFired, FIELD_BOOLEAN, GUNSHIP_NUM_DAMAGE_OUTPUTS ),

	// Function pointers
	DEFINE_INPUTFUNC( FIELD_VOID, "OmniscientOn", InputOmniscientOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "OmniscientOff", InputOmniscientOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPenetrationDepth", InputSetPenetrationDepth ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BlindfireOn", InputBlindfireOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BlindfireOff", InputBlindfireOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SelfDestruct", InputSelfDestruct ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetDockingBBox", InputSetDockingBBox ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetNormalBBox", InputSetNormalBBox ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableGroundAttack", InputEnableGroundAttack ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableGroundAttack", InputDisableGroundAttack ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DoGroundAttack", InputDoGroundAttack ),

	DEFINE_OUTPUT( m_OnFireCannon,		"OnFireCannon" ),
	DEFINE_OUTPUT( m_OnFirstDamage,	"OnFirstDamage" ),
	DEFINE_OUTPUT( m_OnSecondDamage,	"OnSecondDamage" ),
	DEFINE_OUTPUT( m_OnThirdDamage,	"OnThirdDamage" ),
	DEFINE_OUTPUT( m_OnFourthDamage,	"OnFourthDamage" ),
	DEFINE_OUTPUT( m_OnCrashed,			"OnCrashed" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_CombineGunship::CNPC_CombineGunship( void )
{ 
	m_hGroundAttackTarget = NULL;
	m_pSmokeTrail	= NULL;
	m_iAmmoType		= -1; 
	m_pCrashingController = NULL;
	m_hRagdoll = NULL;
	m_hCrashTarget = NULL;
}


void CNPC_CombineGunship::CreateBellyBlastEnergyCore( void )
{
	CCitadelEnergyCore *pCore = static_cast<CCitadelEnergyCore*>( CreateEntityByName( "env_citadel_energy_core" ) );

	if ( pCore == NULL )
		return;

	m_hEnergyCore = pCore;

	int iAttachment = LookupAttachment( "BellyGun" );

	Vector vOrigin;
	QAngle vAngle;

	GetAttachment( iAttachment, vOrigin, vAngle );

	pCore->SetAbsOrigin( vOrigin );
	pCore->SetAbsAngles( vAngle );

	DispatchSpawn( pCore );
	pCore->Activate();

	pCore->SetParent( this, iAttachment );
	pCore->SetScale( 4.0f );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CombineGunship::Spawn( void )
{
	Precache( );

	if ( HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
	{
		SetModel( "models/combine_helicopter.mdl" );
	}
	else
	{
		SetModel( "models/gunship.mdl" );
	}
	
	ExtractBbox( SelectHeaviestSequence( ACT_GUNSHIP_PATROL ), m_cullBoxMins, m_cullBoxMaxs ); 
	BaseClass::Spawn();

	InitPathingData( GUNSHIP_ARRIVE_DIST, GUNSHIP_MIN_CHASE_DIST_DIFF, sk_gunship_burst_min.GetFloat() );
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	m_takedamage = DAMAGE_YES;

	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();

	m_iMaxHealth = m_iHealth = 100;

	m_flFieldOfView = -0.707; // 270 degrees

	m_fHelicopterFlags |= BITS_HELICOPTER_GUN_ON;

	InitBoneControllers();

	InitCustomSchedules();

	SetActivity( (Activity)ACT_GUNSHIP_PATROL );
	SetCollisionGroup( HL2COLLISION_GROUP_GUNSHIP );

	m_flMaxSpeed = GUNSHIP_MAX_SPEED;
	m_flMaxSpeedFiring = GUNSHIP_MAX_SPEED;

	m_flTimeNextAttack = gpGlobals->curtime;
	m_flNextSeeEnemySound = gpGlobals->curtime;

	// Init the pose parameters
	SetPoseParameter( "flex_horz", 0 );
	SetPoseParameter( "flex_vert", 0 );
	SetPoseParameter( "fin_accel", 0 );
	SetPoseParameter( "fin_sway", 0 );

	if( m_iAmmoType == -1 )
	{
		// Since there's no weapon to index the ammo type,
		// do it manually here.
		m_iAmmoType = GetAmmoDef()->Index("CombineCannon"); 
	}

	//!!!HACKHACK
	// This tricks the AI code that constantly complains that the gunship has no schedule.
	SetSchedule( SCHED_IDLE_STAND );

	AddRelationship( "env_flare D_LI 9",	NULL );
	AddRelationship( "rpg_missile D_HT 99", NULL );

	m_flTimeNextPing = gpGlobals->curtime + 2;

	m_flPenetrationDepth = 24;
	m_flBurstDelay = 2.0f;

	// Blindfire and Omniscience default to off
	m_fBlindfire		= false;
	m_fOmniscient		= false;
	m_bIsFiring			= false;
	m_bPreFire			= false;
	m_bInvulnerable		= false;
	
	// See if we should start being able to attack
	m_bCanGroundAttack	= ( m_spawnflags & SF_GUNSHIP_NO_GROUND_ATTACK ) ? false : true;

	m_flEndDestructTime = 0;

	m_iBurstSize = 0;
	m_iBurstHits = 0;

	// Do not dissolve
	AddEFlags( EFL_NO_DISSOLVE );

	for ( int i = 0; i < GUNSHIP_NUM_DAMAGE_OUTPUTS; i++ )
	{
		m_bDamageOutputsFired[i] = false;
	}

	CapabilitiesAdd( bits_CAP_SQUAD);

	if ( hl2_episodic.GetBool() == true )
	{
		CreateBellyBlastEnergyCore();
	}

	// Allows autoaim to help attack the gunship.
	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		AddFlag( FL_AIMTARGET );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restore the motion controller
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::OnRestore( void )
{
	BaseClass::OnRestore();

	if ( m_pCrashingController )
	{
		m_pCrashingController->SetEventHandler( &m_crashCallback );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CombineGunship::Precache( void )
{
	if ( HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
	{
		PrecacheModel( "models/combine_helicopter.mdl" );
		Chopper_PrecacheChunks( this );
	}
	else
	{
		PrecacheModel("models/gunship.mdl");
	}

	PrecacheModel("sprites/lgtning.vmt");

	PrecacheMaterial( "effects/ar2ground2" );
	PrecacheMaterial( "effects/blueblackflash" );
	
	PrecacheScriptSound( "NPC_CombineGunship.SearchPing" );
	PrecacheScriptSound( "NPC_CombineGunship.PatrolPing" );
	PrecacheScriptSound( "NPC_Strider.Charge" );
	PrecacheScriptSound( "NPC_Strider.Shoot" );
	PrecacheScriptSound( "NPC_CombineGunship.SeeEnemy" );
	PrecacheScriptSound( "NPC_CombineGunship.CannonStartSound" );
	PrecacheScriptSound( "NPC_CombineGunship.Explode");
	PrecacheScriptSound( "NPC_CombineGunship.Pain" );
	PrecacheScriptSound( "NPC_CombineGunship.CannonStopSound" );

	PrecacheScriptSound( "NPC_CombineGunship.DyingSound" );
	PrecacheScriptSound( "NPC_CombineGunship.CannonSound" );
	PrecacheScriptSound( "NPC_CombineGunship.RotorSound" );
	PrecacheScriptSound( "NPC_CombineGunship.ExhaustSound" );
	PrecacheScriptSound( "NPC_CombineGunship.RotorBlastSound" );

	if ( hl2_episodic.GetBool() == true )
	{
		UTIL_PrecacheOther( "env_citadel_energy_core" );
		g_iGunshipEffectIndex = PrecacheModel( "sprites/physbeam.vmt" );
	}

	PropBreakablePrecacheAll( MAKE_STRING("models/gunship.mdl") );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Cache whatever pose parameters we intend to use
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::m_sbStaticPoseParamsLoaded = false;
int CNPC_CombineGunship::m_poseFlex_Horz = 0; 
int CNPC_CombineGunship::m_poseFlex_Vert = 0; 
int CNPC_CombineGunship::m_posePitch = 0; 
int CNPC_CombineGunship::m_poseYaw = 0; 
int CNPC_CombineGunship::m_poseFin_Accel = 0; 
int CNPC_CombineGunship::m_poseFin_Sway = 0; 
int CNPC_CombineGunship::m_poseWeapon_Pitch = 0; 
int CNPC_CombineGunship::m_poseWeapon_Yaw = 0;
void	CNPC_CombineGunship::PopulatePoseParameters( void )
{
	if (!m_sbStaticPoseParamsLoaded)
	{
		m_poseFlex_Horz		= LookupPoseParameter( "flex_horz");
		m_poseFlex_Vert			= LookupPoseParameter( "flex_vert" );
		m_posePitch  = LookupPoseParameter( "pitch" );
		m_poseYaw   = LookupPoseParameter( "yaw" );
		m_poseFin_Accel   = LookupPoseParameter( "fin_accel" );
		m_poseFin_Sway   = LookupPoseParameter( "fin_sway" );

		m_poseWeapon_Pitch		= LookupPoseParameter( "weapon_pitch" );
		m_poseWeapon_Yaw		= LookupPoseParameter( "weapon_yaw" );

		m_sbStaticPoseParamsLoaded = true;
	}

	BaseClass::PopulatePoseParameters();
}


//------------------------------------------------------------------------------
// Purpose : 
//------------------------------------------------------------------------------
CNPC_CombineGunship::~CNPC_CombineGunship(void)
{
	StopLoopingSounds();

	if ( m_pCrashingController )
	{
		physenv->DestroyMotionController( m_pCrashingController );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::Ping( void )
{
	if( IsCrashing() )
		return;

	if( GetEnemy() != NULL )
	{
		if( !HasCondition(COND_SEE_ENEMY) && gpGlobals->curtime > m_flTimeNextPing )
		{
			EmitSound( "NPC_CombineGunship.SearchPing" );
			m_flTimeNextPing = gpGlobals->curtime + 3;
		}
	}
	else
	{
		if( gpGlobals->curtime > m_flTimeNextPing )
		{
			EmitSound( "NPC_CombineGunship.PatrolPing" );
			m_flTimeNextPing = gpGlobals->curtime + 3;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CombineGunship::GroundDistToPosition( const Vector &pos )
{
	Vector vecDiff;
	VectorSubtract( GetAbsOrigin(), pos, vecDiff );

	// Only interested in the 2d dist
	vecDiff.z = 0;

	return vecDiff.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::PlayPatrolLoop( void )
{
	m_fPatrolLoopPlaying = true;
	/*
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pPatrolSound, 1.0, 1.0 );
	controller.SoundChangeVolume( m_pAngrySound, 0.0, 1.0 );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::PlayAngryLoop( void )
{
	m_fPatrolLoopPlaying = false;
	/*
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pPatrolSound, 0.0, 1.0 );
	controller.SoundChangeVolume( m_pAngrySound, 1.0, 1.0 );
	*/
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::HelicopterPostThink( void )
{
	// After HelicopterThink()
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		if ( m_bIsFiring )
		{
			// Fire more shots at the dead body for effect
			if ( m_iBurstSize > 8 )
			{
				m_iBurstSize = 8;
			}
		}

		// Fade out search sound, fade in patrol sound.
		PlayPatrolLoop();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetGroundAttackHitPosition( void )
{
	trace_t	tr;
	Vector	vecShootPos, vecShootDir;

	GetAttachment( "BellyGun", vecShootPos, &vecShootDir, NULL, NULL );

	AI_TraceLine( vecShootPos, vecShootPos + Vector( 0, 0, -MAX_TRACE_LENGTH ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if ( m_hGroundAttackTarget )
	{
		return Vector( tr.endpos.x, tr.endpos.y, m_hGroundAttackTarget->WorldSpaceCenter().z );
	}
	return tr.endpos;	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::CheckGroundAttack( void )
{
	if ( m_bCanGroundAttack == false )
		return false;

	if ( m_bIsGroundAttacking )
		return false;

	// Must have an enemy
	if ( GetEnemy() == NULL )
		return false;

	// Must not have done it too recently
	if ( m_flNextGroundAttack > gpGlobals->curtime )
		return false;

	Vector	predPos, predDest;
	
	// Find where the enemy is most likely to be in two seconds
	UTIL_PredictedPosition( GetEnemy(), 1.0f, &predPos );
	UTIL_PredictedPosition( this, 1.0f, &predDest );

	Vector	predGap = ( predDest - predPos );
	predGap.z = 0;

	float	predDistance = predGap.Length();

	// Must be within distance
	if ( predDistance > MIN_GROUND_ATTACK_DIST )
		return false;

	// Can't ground attack missiles
	if ( IsTargettingMissile() )
		return false;

	//FIXME: Check to make sure we're not firing too far above or below the target
	if ( fabs( GetGroundAttackHitPosition().z - GetEnemy()->WorldSpaceCenter().z ) > MIN_GROUND_ATTACK_HEIGHT_DIFF )
		return false;

	//FIXME: Check for ground movement capabilities?

	//TODO: Check for friendly-fire

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StartGroundAttack( void )
{
	// Mark us as attacking
	m_bIsGroundAttacking = true;
	m_flGroundAttackTime = gpGlobals->curtime + 3.0f;

	// Setup the attack effects
	Vector	vecShootPos;

	GetAttachment( "BellyGun", vecShootPos );

	EntityMessageBegin( this, true );
		WRITE_BYTE( GUNSHIP_MSG_STREAKS );
		WRITE_VEC3COORD( vecShootPos );
	MessageEnd();

	CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
	EmitSound( filter2, entindex(), "NPC_Strider.Charge" );

	Vector	endpos = GetGroundAttackHitPosition();
	
	CSoundEnt::InsertSound ( SOUND_DANGER, endpos, 1024, 0.5f );

	if ( hl2_episodic.GetBool() == true )
	{
		if ( m_hEnergyCore )
		{
			variant_t value;
			value.SetFloat( 3.0f );

			g_EventQueue.AddEvent( m_hEnergyCore, "StartCharge", value, 0, this, this );
		}
	}
}

#define GUNSHIP_BELLY_BLAST_RADIUS 256.0f
#define BELLY_BLAST_MAX_PUNCH 5

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ManageWarningBeam( void )
{
	Vector vecSrc, vecShootDir;
	GetAttachment( "BellyGun", vecSrc, NULL, NULL, NULL );

	trace_t	tr;
	CTraceFilterSkipTwoEntities filter( m_hGroundAttackTarget, this, COLLISION_GROUP_NONE );

	UTIL_TraceLine( vecSrc, m_vecHitPos, MASK_SOLID, &filter, &tr );

	int iPunch = 0;

	while ( tr.endpos != m_vecHitPos )
	{
		iPunch++;

		if ( iPunch > BELLY_BLAST_MAX_PUNCH )
			break;

		if ( tr.fraction != 1.0 )
		{
			if ( tr.m_pEnt )
			{
				CTakeDamageInfo	info( this, this, 1.0f, DMG_ENERGYBEAM );

				Vector vTargetDir = tr.m_pEnt->BodyTarget( tr.endpos, false ) - tr.endpos;

				VectorNormalize( vTargetDir );

				info.SetDamagePosition( tr.endpos + ( tr.plane.normal * 64.0f ) );
				info.SetDamageForce( vTargetDir * 100 );

				if ( tr.m_pEnt->m_takedamage != DAMAGE_NO )
				{
					// Deal damage
					tr.m_pEnt->TakeDamage( info );
				}
			}

			Vector vDir = m_vecHitPos - vecSrc;
			VectorNormalize( vDir );

			Vector vStartPunch = tr.endpos + vDir * 1;

			UTIL_TraceLine( vStartPunch, m_vecHitPos, MASK_SOLID, &filter, &tr );

			if ( tr.startsolid )
			{
				float flLength = (vStartPunch - tr.endpos).Length();

				Vector vEndPunch = vStartPunch + vDir * ( flLength * tr.fractionleftsolid );

				UTIL_TraceLine( vEndPunch, m_vecHitPos, MASK_SOLID, &filter, &tr );

				trace_t tr2;
				UTIL_TraceLine( vEndPunch, vEndPunch - vDir * 2, MASK_SOLID, &filter, &tr2 );

				if ( (m_flGroundAttackTime - gpGlobals->curtime) <= 2.0f )
				{
					g_pEffects->EnergySplash( tr2.endpos + vDir * 8, tr2.plane.normal, true );
				}

				g_pEffects->Sparks( tr2.endpos, 3.0f - (m_flGroundAttackTime-gpGlobals->curtime), 3.5f - (m_flGroundAttackTime-gpGlobals->curtime), &tr2.plane.normal );

			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoBellyBlastDamage( trace_t &tr, Vector vMins, Vector vMaxs )
{
	CBaseEntity*	pList[100];

	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_BELLYBLAST )
	{
		NDebugOverlay::Box( tr.endpos, vMins, vMaxs, 255, 255, 0, true, 5.0f );
	}

	int count = UTIL_EntitiesInBox( pList, 100, tr.endpos + vMins, tr.endpos + vMaxs, 0 );

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[i];

		if ( pEntity == this )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		float damage = 150;

		if ( pEntity->IsPlayer() )
		{
			float damageDist = ( pEntity->GetAbsOrigin() - tr.endpos ).Length();
			damage = RemapValClamped( damageDist, 0, 300, 200, 0 );
		}

		CTakeDamageInfo	info( this, this, damage, DMG_DISSOLVE );

		Vector vTargetDir = pEntity->BodyTarget( tr.endpos, false ) - tr.endpos;

		VectorNormalize( vTargetDir );

		info.SetDamagePosition( tr.endpos + ( tr.plane.normal * 64.0f ) );
		info.SetDamageForce( vTargetDir * 25000 );

		// Deal damage
		pEntity->TakeDamage( info );

		trace_t	groundTrace;
		UTIL_TraceLine( pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &groundTrace );

		if ( tr.fraction < 1.0f )
		{
			CEffectData	data;

			// Find the floor and add a dissolve explosion at that point
			data.m_flRadius = GUNSHIP_BELLY_BLAST_RADIUS * 0.5f;
			data.m_vNormal	= groundTrace.plane.normal;
			data.m_vOrigin	= groundTrace.endpos;

			DispatchEffect( "AR2Explosion", data );
		}

		// If the creature was killed, then dissolve it
		if ( pEntity->GetHealth() <= 0.0f )
		{
			if ( pEntity->GetBaseAnimating() != NULL && !pEntity->IsEFlagSet( EFL_NO_DISSOLVE ) )
			{
				pEntity->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoGroundAttackExplosion( void )
{
	// Fire the bullets
	Vector vecSrc, vecShootDir;
	Vector vecAttachmentOrigin;
	GetAttachment( "BellyGun", vecAttachmentOrigin, &vecShootDir, NULL, NULL );

	vecSrc = vecAttachmentOrigin;

	if ( m_hGroundAttackTarget )
	{
		vecSrc = m_hGroundAttackTarget->GetAbsOrigin();
	}

	Vector impactPoint = vecSrc + ( Vector( 0, 0, -1 ) * MAX_TRACE_LENGTH );

	trace_t	tr;
	UTIL_TraceLine( vecSrc, impactPoint, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	UTIL_DecalTrace( &tr, "Scorch" );

	if ( hl2_episodic.GetBool() == true )
	{
		g_pEffects->EnergySplash( tr.endpos, tr.plane.normal );

		CBroadcastRecipientFilter filter;
		te->BeamRingPoint( filter, 0.0, 
			tr.endpos,							//origin
			0,									//start radius
			GUNSHIP_BELLY_BLAST_RADIUS,			//end radius
			g_iGunshipEffectIndex,				//texture
			0,									//halo index
			0,									//start frame
			0,									//framerate
			0.2,								//life
			10,									//width
			0,									//spread
			0,									//amplitude
			255,								//r
			255,								//g
			255,								//b
			50,									//a
			0,									//speed
			FBEAM_FADEOUT
			);
	}

	// Send the effect over
	CEffectData	data;

	// Do an extra effect if we struck the world
	if ( tr.m_pEnt && tr.m_pEnt->IsWorld() )
	{
		data.m_flRadius = GUNSHIP_BELLY_BLAST_RADIUS;
		data.m_vNormal	= tr.plane.normal;
		data.m_vOrigin	= tr.endpos;
		
		DispatchEffect( "AR2Explosion", data );
	}

	float flZLength = vecAttachmentOrigin.z - tr.endpos.z;

	Vector vBeamMins = Vector( -16, -16, 0 );
	Vector vBeamMaxs = Vector( 16, 16, flZLength );

	DoBellyBlastDamage( tr, vBeamMins, vBeamMaxs );

	Vector vBlastMins = Vector( -GUNSHIP_BELLY_BLAST_RADIUS, -GUNSHIP_BELLY_BLAST_RADIUS, 0 );
	Vector vBlastMaxs = Vector( GUNSHIP_BELLY_BLAST_RADIUS, GUNSHIP_BELLY_BLAST_RADIUS, 96 );

	DoBellyBlastDamage( tr, vBlastMins, vBlastMaxs );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StopGroundAttack( bool bDoAttack )
{
	if ( !m_bIsGroundAttacking )
		return;

	// Mark us as no longer attacking
	m_bIsGroundAttacking = false;
	m_flNextGroundAttack = gpGlobals->curtime + 4.0f;
	m_flTimeNextAttack	 = gpGlobals->curtime + 2.0f;

	Vector	hitPos = GetGroundAttackHitPosition();

	// tell the client side effect to complete
	EntityMessageBegin( this, true );
		WRITE_BYTE( GUNSHIP_MSG_BIG_SHOT );
		WRITE_VEC3COORD( hitPos );
	MessageEnd();

	if ( hl2_episodic.GetBool() == true )
	{
		if ( m_hEnergyCore )
		{
			variant_t value;
			value.SetFloat( 1.0f );

			g_EventQueue.AddEvent( m_hEnergyCore, "Stop", value, 0, this, this );
		}
	}

	// Only attack if told to
	if ( bDoAttack )
	{
		CPASAttenuationFilter filter2( this, "NPC_Strider.Shoot" );
		EmitSound( filter2, entindex(), "NPC_Strider.Shoot");

		ApplyAbsVelocityImpulse( Vector( 0, 0, 200.0f ) );

		//ExplosionCreate( hitPos, QAngle( 0, 0, 1 ), this, 500, 500, true );
		DoGroundAttackExplosion();
	}

	// If we were attacking a target, revert to our previous target
	if ( m_hGroundAttackTarget )
	{
		m_hGroundAttackTarget = NULL;
		if ( GetDestPathTarget() )
		{
			// Return to our old path
			SetupNewCurrentTarget( GetDestPathTarget() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DrawRotorWash( float flAltitude, const Vector &vecRotorOrigin )
{
	// If we have a ragdoll, we want the wash under that, not me
	if ( m_hRagdoll )
	{
		BaseClass::DrawRotorWash( flAltitude, m_hRagdoll->GetAbsOrigin() );
		return;
	}

	BaseClass::DrawRotorWash( flAltitude, vecRotorOrigin );
}

//------------------------------------------------------------------------------
// Purpose : Override the desired position if your derived helicopter is doing something special
//------------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateDesiredPosition( void )
{
	if ( m_hCrashTarget )
	{
		SetDesiredPosition( m_hCrashTarget->WorldSpaceCenter() + Vector(0,0,128) );
	}
	else if ( m_hGroundAttackTarget )
	{
		SetDesiredPosition( m_hGroundAttackTarget->GetAbsOrigin() + Vector(0,0,GUNSHIP_BELLYBLAST_TARGET_HEIGHT) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: do all of the stuff related to having an enemy, attacking, etc.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoCombat( void )
{
	// Check for enemy change-overs
	if ( HasEnemy() )
	{
		if ( HasCondition( COND_NEW_ENEMY ) )
		{
			if ( GetEnemy() && GetEnemy()->IsPlayer() && m_flNextSeeEnemySound < gpGlobals->curtime )
			{
				m_flNextSeeEnemySound = gpGlobals->curtime + 5.0;

				if ( !HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
				{
					EmitSound( "NPC_CombineGunship.SeeEnemy" );
				}
			}

			// If we're shooting at a missile, do it immediately!
			if ( IsTargettingMissile() )
			{
				EmitSound( "NPC_CombineGunship.SeeMissile" );

				// Allow the gunship to attack again immediately
				if ( ( m_flTimeNextAttack > gpGlobals->curtime ) && ( ( m_flTimeNextAttack - gpGlobals->curtime ) > GUNSHIP_MISSILE_MAX_RESPONSE_TIME ) )
				{
					m_flTimeNextAttack = gpGlobals->curtime + GUNSHIP_MISSILE_MAX_RESPONSE_TIME;
					m_iBurstSize = sk_gunship_burst_size.GetInt();
				}
			}

			// Fade in angry sound, fade out patrol sound.
			PlayAngryLoop();
		}
	}

	// Do we have a belly blast target?
	if ( m_hGroundAttackTarget && !m_bIsGroundAttacking )
	{
		// If we're over it, blast. Can't use GetDesiredPosition() because it's not updated yet.
		Vector vecTarget = m_hGroundAttackTarget->GetAbsOrigin() + Vector(0,0,GUNSHIP_BELLYBLAST_TARGET_HEIGHT);
		Vector vecToTarget = (vecTarget - GetAbsOrigin());
		float flDistance = vecToTarget.Length();

		// Get the difference between our velocity & the target's velocity
		Vector vec2DVelocity = GetAbsVelocity();
		Vector vec2DTargetVelocity = m_hGroundAttackTarget->GetAbsVelocity();
		vec2DVelocity.z = vec2DTargetVelocity.z = 0;
		float flVelocityDiff = (vec2DVelocity - vec2DTargetVelocity).Length();
		if ( flDistance < 100 && flVelocityDiff < 200 )
		{
			StartGroundAttack();
		}
	}

	// Update our firing
	if ( m_bIsFiring )
	{
		// Fire if we have rounds remaining in this burst
		if ( ( m_iBurstSize > 0 ) && ( gpGlobals->curtime > m_flTimeNextAttack ) )
		{
			UpdateEnemyTarget();
			FireCannonRound();
		}
		else if ( m_iBurstSize < 1 )
		{
			// We're done firing
			StopCannonBurst();
			
			if ( IsTargettingMissile() )
			{
				m_flTimeNextAttack = gpGlobals->curtime + 0.5f;
			}
		}
	}
	else
	{
		// If we're not firing, look at the enemy
		if ( GetEnemy() )
		{
			m_vecAttackPosition = GetEnemy()->EyePosition();
		}

#ifdef BELLYBLAST
		// Check for a ground attack
		if ( CheckGroundAttack() )
		{
			StartGroundAttack();
		}
#endif

		// See if we're attacking
		if ( m_bIsGroundAttacking )
		{
			m_vecHitPos = GetGroundAttackHitPosition();

			ManageWarningBeam();

			// If our time is up, fire the blast and be done
			if ( m_flGroundAttackTime < gpGlobals->curtime )
			{
				// Fire!
				StopGroundAttack( true );
			}
		}
	}

	// If we're using the chopper model, align the gun towards the target
	if ( HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
	{
		Vector vGunPosition;
		GetAttachment( "gun", vGunPosition );
		Vector vecToAttackPos = (m_vecAttackPosition - vGunPosition);
		PoseGunTowardTargetDirection( vecToAttackPos );
	}

	// Forget flares once I've seen them for a while
	float flDeltaSeen = m_flLastSeen - m_flPrevSeen;
	if ( GetEnemy() != NULL && GetEnemy()->Classify() == CLASS_FLARE && flDeltaSeen > GUNSHIP_FLARE_IGNORE_TIME )
	{
		AddEntityRelationship( GetEnemy(), D_NU, 5 );

		PlayPatrolLoop();

		// Forget the flare now.
		SetEnemy( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::ChooseEnemy( void )
{
	// If we're firing, don't switch enemies. This stops the gunship occasionally 
	// stopping a burst before he's really fired at all, which makes him look indecisive.
	if ( m_bIsFiring )
		return true;

	return BaseClass::ChooseEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: There's a lot of code in here now. We should consider moving 
//			helicopters and such to scheduled AI. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::MoveHead( void )
{
	float flYaw = GetPoseParameter( m_poseFlex_Horz );
	float flPitch = GetPoseParameter( m_poseFlex_Vert );

/*
	This head-turning code will cause the head to POP when switching from looking at the enemy
	to looking according to the flight model. I will fix this later. Right now I'm turning
	the code over to Ken for some aiming fixups. (sjb)
*/

	while( 1 )
	{
		if ( GetEnemy() != NULL )
		{
			Vector vecToEnemy, vecAimDir;
			float	flDot;

			Vector vTargetPos, vGunPosition;
			Vector vecTargetOffset;
			QAngle vGunAngles;

			GetAttachment( "muzzle", vGunPosition, vGunAngles );

			vTargetPos = GetEnemyTarget();

			VectorSubtract( vTargetPos, vGunPosition, vecToEnemy );
			VectorNormalize( vecToEnemy );
			
			// get angles relative to body position
			AngleVectors( GetAbsAngles(), &vecAimDir );
			flDot = DotProduct( vecAimDir, vecToEnemy );

			// Look at Enemy!!
			if ( flDot > 0.3f )
			{
				float flDiff;

				float flDesiredYaw = VecToYaw(vTargetPos - vGunPosition);
				flDiff = UTIL_AngleDiff( flDesiredYaw, vGunAngles.y ) * 0.90;
				flYaw = UTIL_Approach( flYaw + flDiff, flYaw, 5.0 );	

				float flDesiredPitch = UTIL_VecToPitch(vTargetPos - vGunPosition);
				flDiff = UTIL_AngleDiff( flDesiredPitch, vGunAngles.x ) * 0.90;
				flPitch = UTIL_Approach( flPitch + flDiff, flPitch, 5.0 );	

				break;
			}
		}
 
		// Look where going!
#if 1 // old way- look according to rotational velocity
		flYaw = UTIL_Approach( GetLocalAngularVelocity().y, flYaw, 2.0 * 10 * m_flDeltaT );	
		flPitch = UTIL_Approach( GetLocalAngularVelocity().x, flPitch, 2.0 * 10 * m_flDeltaT );	
#else // new way- look towards the next waypoint?
		// !!!UNDONE
#endif
		break;
	}

	// Set the body flexes
	SetPoseParameter( m_poseFlex_Vert, flPitch );
	SetPoseParameter( m_poseFlex_Horz, flYaw );
}


//-----------------------------------------------------------------------------
// Purpose: There's a lot of code in here now. We should consider moving 
//			helicopters and such to scheduled AI. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::PrescheduleThink( void )
{
	m_flDeltaT = gpGlobals->curtime - GetLastThink();

	// Are we crashing?
	if ( m_flEndDestructTime && gpGlobals->curtime > m_flEndDestructTime )
	{
		// We're dead, remove ourselves
		SelfDestruct();
		return;
	}

	if( m_lifeState == LIFE_ALIVE )
	{
		// Chopper doesn't ping
		if ( !HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
		{
			Ping();
		}

		DoCombat();
		MoveHead();
	}
	else if( m_lifeState == LIFE_DYING )
	{
		// Increase the number of explosions as he gets closer to death
		bool bCreateExplosion = false;
		float flTimeLeft = m_flEndDestructTime - gpGlobals->curtime;
		if ( flTimeLeft > 1.5 )
		{
			bCreateExplosion = (random->RandomInt( 0, 3 ) == 0);
		}
		else 
		{
			bCreateExplosion = (random->RandomInt( 0, 2 ) == 0);
		}

		if ( bCreateExplosion )
		{
			Vector explodePoint;
			if ( m_hRagdoll )
			{
				m_hRagdoll->CollisionProp()->RandomPointInBounds( Vector(0.25,0.25,0.25), Vector(0.75,0.75,0.75), &explodePoint );
			}
			else
			{
				CollisionProp()->RandomPointInBounds( Vector(0.25,0.25,0.25), Vector(0.75,0.75,0.75), &explodePoint );

				// Knock the gunship a little, but not if we're trying to fly to a point
				if ( !m_hCrashTarget )
				{
					Vector vecPush = (GetAbsOrigin() - explodePoint);
					VectorNormalize( vecPush );
					ApplyAbsVelocityImpulse( vecPush * 128 );
				}
			}

			ExplosionCreate( explodePoint, QAngle(0,0,1), this, 100, 128, false );
		}

		// Have we reached our crash point?
		if ( m_flNextGunshipCrashFind && !m_hRagdoll )
		{
			// Update nearest crash point. The RPG that killed us may have knocked us
			// closer to a different point than the one we were near when we first died.
			if ( m_flNextGunshipCrashFind < gpGlobals->curtime )
			{
				FindNearestGunshipCrash();
			}

			if ( m_hCrashTarget )
			{
				MoveHead();

				UpdateDesiredPosition();

				// If we're over it, destruct
				Vector vecToTarget = (GetDesiredPosition() - GetAbsOrigin());
				if ( vecToTarget.LengthSqr() < (384 * 384) )
				{
					BeginDestruct();
					m_OnCrashed.FireOutput( this, this );
					m_hCrashTarget->GunshipCrashedOnTarget();
					return;
				}
			}
		}
	}

	BaseClass::PrescheduleThink();

#ifdef JACOBS_GUNSHIP	
	SetPoseParameter( m_posePitch, random->RandomFloat( GUNSHIP_HEAD_MAX_LEFT, GUNSHIP_HEAD_MAX_RIGHT ) );
	SetPoseParameter( m_poseYaw, random->RandomFloat( GUNSHIP_HEAD_MAX_UP, GUNSHIP_HEAD_MAX_DOWN ) );
#endif

}

//------------------------------------------------------------------------------
// Purpose :	If the enemy is in front of the gun, load up a burst. 
//				Actual gunfire is handled in PrescheduleThink
// Input   : 
// Output  : 
//------------------------------------------------------------------------------
bool CNPC_CombineGunship::FireGun( void )
{
	if ( m_lifeState != LIFE_ALIVE )
		return false;

	if ( m_bIsGroundAttacking )
		return false;

	if ( GetEnemy() && !m_bIsFiring && gpGlobals->curtime > m_flTimeNextAttack )
	{
		// We want to decelerate to attack
		if (m_flGoalSpeed > GetMaxSpeedFiring() )
		{
			m_flGoalSpeed = GetMaxSpeedFiring();
		}

		bool bTargetingMissile = IsTargettingMissile();
		if ( !bTargetingMissile && !m_bPreFire )
		{
			m_bPreFire = true;
			m_flTimeNextAttack = gpGlobals->curtime + 0.5f;
			
			EmitSound( "NPC_CombineGunship.CannonStartSound" );
			return false;
		}

		//TODO: Emit the danger noise and wait until it's finished

		// Don't fire at an occluded enemy unless blindfire is on.
		if ( HasCondition( COND_ENEMY_OCCLUDED ) && ( m_fBlindfire == false ) )
			return false;

		// Don't shoot if the enemy is too close
		if ( !bTargetingMissile && GroundDistToPosition( GetEnemy()->GetAbsOrigin() ) < GUNSHIP_STITCH_MIN )
			return false;

		Vector vecAimDir, vecToEnemy;
		Vector vecMuzzle, vecEnemyTarget;

		GetAttachment( "muzzle", vecMuzzle, &vecAimDir, NULL, NULL );
		vecEnemyTarget = GetEnemyTarget();

		// Aim with the muzzle's attachment point.
		VectorSubtract( vecEnemyTarget, vecMuzzle, vecToEnemy );

		VectorNormalize( vecToEnemy );
		VectorNormalize( vecAimDir );

		if ( DotProduct( vecToEnemy, vecAimDir ) > 0.9 )
		{
			StartCannonBurst( sk_gunship_burst_size.GetInt() );
			return true;
		}

		return false;
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose: Fire a round from the cannon
// Notes:	Only call this if you have an enemy.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::FireCannonRound( void )
{
	Vector vecPenetrate;
	trace_t tr;

	Vector vecToEnemy, vecEnemyTarget;
	Vector vecMuzzle;
	Vector vecAimDir;

	GetAttachment( "muzzle", vecMuzzle, &vecAimDir );
	vecEnemyTarget = GetEnemyTarget();
	
	// Aim with the muzzle's attachment point.
	VectorSubtract( vecEnemyTarget, vecMuzzle, vecToEnemy );
	VectorNormalize( vecToEnemy );

	// If the gun is wildly off target, stop firing!
	// FIXME  - this should use a vector pointing 
	// to the enemy's location PLUS the stitching 
	// error! (sjb) !!!BUGBUG

	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
	{
		QAngle vecAimAngle;
		Vector	vForward, vRight, vUp;
		GetAttachment( "muzzle", vecMuzzle, &vForward, &vRight, &vUp );
		AngleVectors( vecAimAngle, &vForward, &vRight, &vUp );
		NDebugOverlay::Line( vecMuzzle, vecEnemyTarget, 255, 255, 0, true, 1.0f );

		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vForward * 64.0f ), 255, 0, 0, true, 1.0f );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vRight * 32.0f ), 0, 255, 0, true, 1.0f );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vUp * 32.0f ), 0, 0, 255, true, 1.0f );
	}

	// Robin: Check the dotproduct to the enemy, NOT to the offsetted firing angle
	// Fixes problems firing at close enemies, where the enemy is valid but
	// the offset firing stitch isn't.
	Vector vecDotCheck = vecToEnemy;
	if ( GetEnemy() )
	{
		VectorSubtract( GetEnemy()->GetAbsOrigin(), vecMuzzle, vecDotCheck );
		VectorNormalize( vecDotCheck );
	}

	if ( DotProduct( vecDotCheck, vecAimDir ) < 0.8f )
	{
		StopCannonBurst();
		return;
	}

	DoMuzzleFlash();

	m_OnFireCannon.FireOutput( this, this, 0 );

	m_flTimeNextAttack = gpGlobals->curtime + 0.05f;

	float flPrevHealth = 0;
	if ( GetEnemy() )
	{
		flPrevHealth = GetEnemy()->GetHealth();
	}

	// Make sure we hit missiles
	if ( IsTargettingMissile() )
	{
		// Fire a fake shot
		FireBullets( 1, vecMuzzle, vecToEnemy, VECTOR_CONE_5DEGREES, 8192, m_iAmmoType, 1 );

		CBaseEntity *pMissile = GetEnemy();

		Vector	missileDir, threatDir;

		AngleVectors( pMissile->GetAbsAngles(), &missileDir );

		threatDir = ( WorldSpaceCenter() - pMissile->GetAbsOrigin() );
		float	threatDist = VectorNormalize( threatDir );

		// Check that the target is within some threshold
		if ( ( DotProduct( threatDir, missileDir ) > 0.95f ) && ( threatDist < 1024.0f ) )
		{
			if ( random->RandomInt( 0, 1 ) == 0 )
			{
				CTakeDamageInfo info( this, this, 200, DMG_MISSILEDEFENSE );
				CalculateBulletDamageForce( &info, m_iAmmoType, -threatDir, WorldSpaceCenter() );
				GetEnemy()->TakeDamage( info );
			}
		}
		else
		{
			//FIXME: Some other metric
		}
	}
	else
	{
		m_iBurstSize--;

		// Fire directly at the target
		FireBulletsInfo_t info( 1, vecMuzzle, vecToEnemy, vec3_origin, MAX_COORD_RANGE, m_iAmmoType );
		info.m_iTracerFreq = 1;
		CAmmoDef *pAmmoDef = GetAmmoDef();
		info.m_iPlayerDamage = pAmmoDef->PlrDamage( m_iAmmoType );

		// If we've already hit the player, do 0 damage. This ensures we don't hit the
		// player multiple times during a single burst.
		if ( m_iBurstHits >= GUNSHIP_MAX_HITS_PER_BURST )
		{
			info.m_iPlayerDamage = 1;
		}

		FireBullets( info );

		if ( GetEnemy() && flPrevHealth != GetEnemy()->GetHealth() )
		{
			m_iBurstHits++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "GunshipMuzzleFlash", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	bool fReturn = BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	if( m_fOmniscient )
	{
		if( !fReturn )
		{
			// Set this condition so that we can check it later and know that the 
			// enemy truly is occluded, but the gunship regards it as visible due 
			// to omniscience.
			SetCondition( COND_ENEMY_OCCLUDED );
		}
		else
		{
			ClearCondition( COND_ENEMY_OCCLUDED );
		}

		return true;
	}

	if( fReturn )
	{
		ClearCondition( COND_ENEMY_OCCLUDED );
	}
	else
	{
		SetCondition( COND_ENEMY_OCCLUDED );
	}

	return fReturn;
}


//-----------------------------------------------------------------------------
// Purpose: Change the depth that gunship bullets can penetrate through solids
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetPenetrationDepth( inputdata_t &inputdata )
{
	m_flPenetrationDepth = inputdata.value.Float();
}


//-----------------------------------------------------------------------------
// Purpose: Allow the gunship to sense its enemy's location even when enemy
//			is hidden from sight.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputOmniscientOn( inputdata_t &inputdata )
{
	m_fOmniscient = true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the gunship to its default requirement that it see the 
//			enemy to know its current position
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputOmniscientOff( inputdata_t &inputdata )
{
	m_fOmniscient = false;
}


//-----------------------------------------------------------------------------
// Purpose: Allows the gunship to fire at an unseen enemy. The gunship is relying
//			on hitting the target with bullets that will punch through the 
//			cover that the enemy is hiding behind. (Such as the Depot lighthouse)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBlindfireOn( inputdata_t &inputdata )
{
	m_fBlindfire = true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the gunship to default rules for attacking the enemy. The
//			enemy must be seen to be fired at.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBlindfireOff( inputdata_t &inputdata )
{
	m_fBlindfire = false;
}

//-----------------------------------------------------------------------------
// Purpose: Set the gunship's paddles flailing!
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	StopCannonBurst();

	// Replace the rotor sound with broken engine sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );

	// BUGBUG: Isn't this sound just going to get stomped when the base class calls StopLoopingSounds() ??
	CPASAttenuationFilter filter2( this );
	m_pRotorSound = controller.SoundCreate( filter2, entindex(), "NPC_CombineGunship.DyingSound" );
	controller.Play( m_pRotorSound, 1.0, 100 );

	m_OnDeath.FireOutput( info.GetAttacker(), this );
	SendOnKilledGameEvent( info );

	BeginCrash();

	// we deliberately do not call BaseClass::EventKilled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::BeginCrash( void )
{
	m_lifeState = LIFE_DYING;
	StopGroundAttack( false );

	// Increase our smoke trail
	CreateSmokeTrail();
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->SetLifetime( -1 );
		m_pSmokeTrail->m_StartSize = 64;
		m_pSmokeTrail->m_EndSize = 128;
		m_pSmokeTrail->m_Opacity = 0.5f;
	}

	if ( !FindNearestGunshipCrash() )
	{
		// We couldn't find a crash target, so just die right here.
		BeginDestruct();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::FindNearestGunshipCrash( void )
{
	// Find the nearest crash point. If we find one, we'll try to fly to it and die.
	// If we can't find one, we'll die right here.
	bool bFoundAnyCrashTargets = false;
 	float flNearest = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	CTargetGunshipCrash *pNearest = NULL;
	CBaseEntity *pEnt = NULL;
	while( (pEnt = gEntList.FindEntityByClassname(pEnt, "info_target_gunshipcrash")) != NULL )
	{
		CTargetGunshipCrash *pCrashTarget = assert_cast<CTargetGunshipCrash*>(pEnt);
		if ( pCrashTarget->IsDisabled() )
			continue;

		bFoundAnyCrashTargets = true;

		float flDist = ( pEnt->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr();
		if( flDist < flNearest )
		{
			trace_t tr;
			UTIL_TraceLine( WorldSpaceCenter(), pEnt->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
			if( tr.fraction == 1.0 )
			{
				pNearest = pCrashTarget;
				flNearest = flDist;
			}
			else if ( g_debug_gunship.GetInt() )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), tr.endpos, 255,0,0, true, 99);
			}
		}
	}

	if ( !pNearest )
	{
		// If we found a gunship crash, but none near enough, claim we did find one, so that we
		// don't blow up yet. This will give us 3 seconds to attempt to find one before dying.
		if ( !m_hCrashTarget && bFoundAnyCrashTargets )
		{
			m_flNextGunshipCrashFind = gpGlobals->curtime + 0.5;
			m_flEndDestructTime = gpGlobals->curtime + 3.0;
			return true;
		}

		return false;
	}

	// Fly to the crash point and destruct there
  	m_hCrashTarget = pNearest;
	m_flNextGunshipCrashFind = gpGlobals->curtime + 0.5;
	m_flEndDestructTime = 0;

	if ( g_debug_gunship.GetInt() )
	{
		NDebugOverlay::Line(GetAbsOrigin(), m_hCrashTarget->GetAbsOrigin(), 0,255,0, true, 0.5);
		NDebugOverlay::Box( m_hCrashTarget->GetAbsOrigin(), -Vector(200,200,200), Vector(200,200,200), 0,255,0, 128, 0.5 );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: I'm now ready to die. Create my ragdoll & hide myself.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::BeginDestruct( void )
{
	m_flEndDestructTime = gpGlobals->curtime + 3.0;

	// Clamp velocity
	if( hl2_episodic.GetBool() && GetAbsVelocity().Length() > 700.0f )
	{
		Vector vecVelocity = GetAbsVelocity(); 
		VectorNormalize( vecVelocity );
		SetAbsVelocity( vecVelocity * 700.0f );
	}

	CTakeDamageInfo info;
	info.SetDamage( 40000 );
	CalculateExplosiveDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );

	// Don't create a ragdoll if we're going to explode into gibs
	if ( !m_hCrashTarget )
		return;

	// Switch to damaged skin
	m_nSkin = 1;

	if ( HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
	{
		Chopper_BecomeChunks( this );
		SetThink( &CNPC_CombineGunship::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		AddEffects( EF_NODRAW );
		return;
	}

	// Create the ragdoll
	m_hRagdoll = CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );
	if ( !m_hRagdoll )
	{
		// Failed, just explode
		SelfDestruct();
		return;
	}

	m_hRagdoll->SetName( AllocPooledString( UTIL_VarArgs("%s_ragdoll", STRING(GetEntityName()) ) ) );

	// Tell the smoke trail to follow the ragdoll
	CreateSmokeTrail();
	if ( m_pSmokeTrail )
	{
		// Force the smoke trail to stay on, and tell it to follow the ragdoll
		m_pSmokeTrail->SetLifetime( -1 );
		m_pSmokeTrail->FollowEntity( m_hRagdoll );
		
		m_pSmokeTrail->m_StartSize = 64;
		m_pSmokeTrail->m_EndSize = 128;
		m_pSmokeTrail->m_Opacity = 0.5f;
	}

	/* 
	// ROBIN: Disabled this for now.
	//
	// Create the crashing controller and attach it to the ragdoll physics objects
	m_pCrashingController = physenv->CreateMotionController( &m_crashCallback );
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = m_hRagdoll->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		m_pCrashingController->AttachObject( pList[i], false );
	}
	*/

	// Hide myself, because the ragdoll's now taken my place
	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//-----------------------------------------------------------------------------
// Purpose: Create a smoke trail
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::CreateSmokeTrail( void )
{
	if ( m_pSmokeTrail )
		return;

	m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_SpawnRate			= 48;
		m_pSmokeTrail->m_ParticleLifetime	= 2.5f;
		
		m_pSmokeTrail->m_StartColor.Init( 0.25f, 0.25f, 0.25f );
		m_pSmokeTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pSmokeTrail->m_StartSize		= 24;
		m_pSmokeTrail->m_EndSize		= 128;
		m_pSmokeTrail->m_SpawnRadius	= 4;
		m_pSmokeTrail->m_MinSpeed		= 8;
		m_pSmokeTrail->m_MaxSpeed		= 64;
		m_pSmokeTrail->m_Opacity		= 0.2f;

		m_pSmokeTrail->SetLifetime( -1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ApplyGeneralDrag( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	
	// See if we need to stop more quickly
	if ( m_bIsGroundAttacking )
	{
		vecNewVelocity *= 0.95f;
	}
	else
	{
		vecNewVelocity *= 0.995;
	}

	SetAbsVelocity( vecNewVelocity );
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------

void CNPC_CombineGunship::Flight( void )
{
	if( GetFlags() & FL_ONGROUND )
	{
		//This would be really bad.
		SetGroundEntity( NULL );
	}

	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_PATH )
	{
		NDebugOverlay::Line(GetLocalOrigin(), GetDesiredPosition(), 0,0,255, true, 0.1);
	}

	// calc desired acceleration
	float dt = 1.0f;

	Vector	accel;
	float	accelRate = GUNSHIP_ACCEL_RATE;
	float	maxSpeed = GetMaxSpeed(); 

	if ( m_lifeState == LIFE_DYING && m_hCrashTarget != NULL )
	{
		// Gunship can fly faster to the place where it's supposed to crash, but
		// maintain normal speeds if we haven't found a place to crash.
		accelRate *= 2.0;
		maxSpeed *= 4.0;
	}

	float flCurrentSpeed = GetAbsVelocity().Length();
	float flDist = MIN( flCurrentSpeed + accelRate, maxSpeed );

	Vector deltaPos;
	if ( m_lifeState == LIFE_DYING || m_hGroundAttackTarget )
	{
		// Move directly to the target point
		deltaPos = GetDesiredPosition();
	}
	else
	{
		ComputeActualTargetPosition( flDist, dt, 0.0f, &deltaPos );
	}
	deltaPos -= GetAbsOrigin();

	// calc goal linear accel to hit deltaPos in dt time.
	accel.x = 2.0 * (deltaPos.x - GetAbsVelocity().x * dt) / (dt * dt);
	accel.y = 2.0 * (deltaPos.y - GetAbsVelocity().y * dt) / (dt * dt);
	accel.z = 2.0 * (deltaPos.z - GetAbsVelocity().z * dt + 0.5 * 384 * dt * dt) / (dt * dt);
	
	float flDistFromPath = 0.0f;
	Vector vecPoint, vecDelta;
	if ( m_lifeState != LIFE_DYING && IsOnPathTrack() )
	{
		// Also, add in a little force to get us closer to our current line segment if we can
		ClosestPointToCurrentPath( &vecPoint );
		VectorSubtract( vecPoint, GetAbsOrigin(), vecDelta );
 		flDistFromPath = VectorNormalize( vecDelta );
		if ( flDistFromPath > GUNSHIP_OUTER_NAV_DIST )
		{
			// Strongly constrain to an n unit pipe around the current path
			// by damping out all impulse forces that would push us further from the pipe
			float flAmount = (flDistFromPath - GUNSHIP_OUTER_NAV_DIST) / 200.0f;
			flAmount = clamp( flAmount, 0, 1 );
			VectorMA( accel, flAmount * 200.0f, vecDelta, accel );
		}
	}

	Vector vecAvoidForce;
	CAvoidSphere::ComputeAvoidanceForces( this, 350.0f, 2.0f, &vecAvoidForce );
	accel += vecAvoidForce;
	CAvoidBox::ComputeAvoidanceForces( this, 350.0f, 2.0f, &vecAvoidForce );
	accel += vecAvoidForce;
	
	if ( m_lifeState != LIFE_DYING || m_hCrashTarget == NULL )
	{
		// don't fall faster than 0.2G or climb faster than 2G
		accel.z = clamp( accel.z, 384 * 0.2, 384 * 2.0 );
	}

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

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

	// limit angular accel changes to similate mechanical response times
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

	Vector vecImpulse = m_flForce * up;
	
	if ( !m_hCrashTarget && m_lifeState == LIFE_DYING && !hl2_episodic.GetBool() )
	{
		// Force gunship to the ground if it doesn't have a specific place to crash.
		// EXCEPT In episodic, where forcing it to the ground means it crashes where the player can't see (attic showdown) (sjb)
		vecImpulse.z = -10;
	}
	else
	{
		vecImpulse.z -= 38.4;  // 32ft/sec	
	}
	
	// Find our current velocity
	Vector vecVelDir = GetAbsVelocity();
	VectorNormalize( vecVelDir );

	if ( flDistFromPath > GUNSHIP_INNER_NAV_DIST )
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
	float speed = m_flForce * DotProduct( vecVelDir, vecAccelDir );// * DotProduct( forward, vecVelDir );

	// Apply the acceleration blend to the fins
	float finAccelBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
	float curFinAccel = GetPoseParameter( m_poseFin_Accel );
	
	curFinAccel = UTIL_Approach( finAccelBlend, curFinAccel, 0.5f );
	SetPoseParameter( m_poseFin_Accel, curFinAccel );

	speed = m_flForce * DotProduct( vecVelDir, right );

	// Apply the spin sway to the fins
	float finSwayBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
	float curFinSway = GetPoseParameter( m_poseFin_Sway );

	curFinSway = UTIL_Approach( finSwayBlend, curFinSway, 0.5f );
	SetPoseParameter( m_poseFin_Sway, curFinSway );

	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_PATH )
	{
		NDebugOverlay::Line(GetLocalOrigin(), GetLocalOrigin() + vecImpulse, 255,0,0, true, 0.1);
	}

	// Add in our velocity pulse for this frame
	ApplyAbsVelocityImpulse( vecImpulse );
}

//------------------------------------------------------------------------------
// Updates the facing direction
//------------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateFacingDirection( void )
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
// Purpose : Fire up the Gunships 'second' rotor sound. The Search sound.
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	
	CPASAttenuationFilter filter( this );

	m_pCannonSound		= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.CannonSound" );
	m_pRotorSound		= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.RotorSound" );
	m_pAirExhaustSound	= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.ExhaustSound" );
	m_pAirBlastSound	= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.RotorBlastSound" );
	
	controller.Play( m_pCannonSound, 0.0, 100 );
	controller.Play( m_pAirExhaustSound, 0.0, 100 );
	controller.Play( m_pAirBlastSound, 0.0, 100 );

	BaseClass::InitializeRotorSound();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateRotorSoundPitch( int iPitch )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// Apply the pitch to both sounds. 
	controller.SoundChangePitch( m_pAirExhaustSound, iPitch, 0.1 );

	// FIXME: Doesn't work in multiplayer
	CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
	if (pPlayer)
	{
		Vector pos;
		Vector up;
		GetAttachment( "rotor", pos, NULL, NULL, &up );

		float flDistance = (pPlayer->WorldSpaceCenter() - pos).Length2DSqr();

		// Fade in exhaust when we're far from the player
		float flVolume = clamp( RemapVal( flDistance, (900*900), (1800*1800), 1, 0 ), 0, 1 );
		controller.SoundChangeVolume( m_pAirExhaustSound, flVolume * GetRotorVolume(), 0.1 );

		// Fade in the blast when it's close to the player (in 2D)
		flVolume = clamp( RemapVal( flDistance, (600*600), (700*700), 1, 0 ), 0, 1 );
		controller.SoundChangeVolume( m_pAirBlastSound, flVolume * GetRotorVolume(), 0.1 );
	}

	BaseClass::UpdateRotorSoundPitch( iPitch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ApplySidewaysDrag( const Vector &vecRight )
{
	Vector vecVelocity = GetAbsVelocity();
	if( m_lifeState == LIFE_ALIVE )
	{
		vecVelocity.x *= (1.0 - fabs( vecRight.x ) * 0.04);
		vecVelocity.y *= (1.0 - fabs( vecRight.y ) * 0.04);
		vecVelocity.z *= (1.0 - fabs( vecRight.z ) * 0.04);
	}
	else
	{
		vecVelocity.x *= (1.0 - fabs( vecRight.x ) * 0.03);
		vecVelocity.y *= (1.0 - fabs( vecRight.y ) * 0.03);
		vecVelocity.z *= (1.0 - fabs( vecRight.z ) * 0.09);
	}
	SetAbsVelocity( vecVelocity );
}

//------------------------------------------------------------------------------
// Purpose: Explode the gunship.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::SelfDestruct( void )
{
	SetThink( NULL );
	m_lifeState = LIFE_DEAD;
	
	StopLoopingSounds();
	StopCannonBurst();

	Vector vecVelocity = GetAbsVelocity();
	vecVelocity.z = 0.0; // stop falling.
	SetAbsVelocity( vecVelocity );

	CBaseEntity *pBreakEnt = this;

	// If we've ragdolled, play the explosions on the ragdoll instead
	Vector vecOrigin;
	if ( m_hRagdoll )
	{
		m_hRagdoll->EmitSound( "NPC_CombineGunship.Explode" );
		vecOrigin = m_hRagdoll->GetAbsOrigin();
		pBreakEnt = m_hRagdoll;
	}
	else
	{
		EmitSound( "NPC_CombineGunship.Explode" );
		vecOrigin = GetAbsOrigin();
	}

	// Create some explosions on the gunship body
	Vector vecDelta;
	for( int i = 0 ; i < 6 ; i++ )
	{
		vecDelta = RandomVector( -200,200 );
		ExplosionCreate( vecOrigin + vecDelta, QAngle( -90, 0, 0 ), this, 10, 10, false );
	}

	AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( vecOrigin );
	if ( pExplosion )
	{
		pExplosion->SetLifetime( 10 );
	}

	// If we don't have a crash target, explode into chunks
	if ( !m_hCrashTarget )
	{
		Vector angVelocity;
		QAngleToAngularImpulse( pBreakEnt->GetLocalAngularVelocity(), angVelocity );
		PropBreakableCreateAll( pBreakEnt->GetModelIndex(), pBreakEnt->VPhysicsGetObject(), pBreakEnt->GetAbsOrigin(), pBreakEnt->GetAbsAngles(), pBreakEnt->GetAbsVelocity(), angVelocity, 1.0, 800, COLLISION_GROUP_NPC, pBreakEnt );

		// Throw out some small chunks too
		CPVSFilter filter( GetAbsOrigin() );
	 	for ( int i = 0; i < 20; i++ )
		{
			Vector gibVelocity = RandomVector(-100,100) * 10;
			int iModelIndex = modelinfo->GetModelIndex( g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ) );	
			te->BreakModel( filter, 0.0, GetAbsOrigin(), vec3_angle, Vector(40,40,40), gibVelocity, iModelIndex, 400, 1, 2.5, BREAK_METAL );
		}

		if ( m_hRagdoll )
		{
			UTIL_Remove( m_hRagdoll );
		}
	}
	else
	{
		if ( m_pSmokeTrail )
		{
			// If we have a ragdoll, let it smoke for a few more seconds
			if ( m_hRagdoll )
			{
				m_pSmokeTrail->SetLifetime(3.0f);
			}
			else
			{
				m_pSmokeTrail->SetLifetime(0.1f);
			}
			m_pSmokeTrail = NULL;
		}
	}

	UTIL_Remove( this );

	// Record this so a nearby citizen can respond.
	if ( GetCitizenResponse() )
	{
		GetCitizenResponse()->AddResponseTrigger( CR_PLAYER_KILLED_GUNSHIP );
	}

#ifdef HL2_EPISODIC
	NPCEventResponse()->TriggerEvent( "TLK_CITIZEN_RESPONSE_KILLED_GUNSHIP", false, false );
#endif
}


//------------------------------------------------------------------------------
// Purpose : Explode the gunship.
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSelfDestruct( inputdata_t &inputdata )
{
	BeginCrash();
}

//------------------------------------------------------------------------------
// Purpose : Shrink the gunship's bbox so that it fits in docking bays
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetDockingBBox( inputdata_t &inputdata )
{
	Vector vecSize( 32, 32, 32 );

	UTIL_SetSize( this, vecSize * -1, vecSize );
}

//------------------------------------------------------------------------------
// Purpose : Set the gunship BBox to normal size
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetNormalBBox( inputdata_t &inputdata )
{
	Vector vecBBMin, vecBBMax;

	ExtractBbox( SelectHeaviestSequence( ACT_GUNSHIP_PATROL ), vecBBMin, vecBBMax ); 

	// Trim the bounding box a bit. It's huge.
#define GUNSHIP_TRIM_BOX 38
	vecBBMin.x += GUNSHIP_TRIM_BOX;
	vecBBMax.x -= GUNSHIP_TRIM_BOX;
	vecBBMin.y += GUNSHIP_TRIM_BOX;
	vecBBMax.y -= GUNSHIP_TRIM_BOX;

	UTIL_SetSize( this, vecBBMin, vecBBMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputEnableGroundAttack( inputdata_t &inputdata )
{
	m_bCanGroundAttack = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputDisableGroundAttack( inputdata_t &inputdata )
{
	m_bCanGroundAttack = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputDoGroundAttack( inputdata_t &inputdata )
{
	// Was a target node specified?
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.StringID(), NULL, inputdata.pActivator, inputdata.pCaller );
	if ( pEntity )
	{
		// Mapmaker wants us to ground attack a specific target
		m_hGroundAttackTarget = pEntity;
	}
	else
	{
		StartGroundAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vGunPosition - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateEnemyTarget( void )
{
	Vector	vGunPosition;

	GetAttachment( "muzzle", vGunPosition );

	// Follow mode
	Vector	enemyPos;
	bool	bTargettingPlayer;
	
	if ( GetEnemy() != NULL )
	{
		CBaseCombatCharacter *pCCEnemy = GetEnemy()->MyCombatCharacterPointer();
		if ( pCCEnemy != NULL && pCCEnemy->IsInAVehicle() )
		{
			// Update against a driving target
			enemyPos = GetEnemy()->WorldSpaceCenter();
		}
		else 
		{
			enemyPos = GetEnemy()->EyePosition();
		}
		bTargettingPlayer = GetEnemy()->IsPlayer();
	}
	else
	{
		enemyPos = m_vecAttackPosition;
		bTargettingPlayer = false;
	}

	// Direction towards the enemy
	Vector	targetDir = enemyPos - m_vecAttackPosition;
	VectorNormalize( targetDir );

	// Direction from the gunship to the enemy
	Vector	enemyDir = enemyPos - vGunPosition;
	VectorNormalize( enemyDir );

	float	lastSpeed = VectorNormalize( m_vecAttackVelocity );
	QAngle	chaseAngles, lastChaseAngles;

	VectorAngles( targetDir, chaseAngles );
	VectorAngles( m_vecAttackVelocity, lastChaseAngles );

	// Debug info
	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
	{
		// Final position
		NDebugOverlay::Cross3D( m_vecAttackPosition, -Vector(2,2,2), Vector(2,2,2), 0, 0, 255, true, 4.0f );
	}

	float yawDiff = UTIL_AngleDiff( lastChaseAngles[YAW], chaseAngles[YAW] );

	int	maxYaw;
	if ( bTargettingPlayer )
	{
		maxYaw = 6;
	}
	else
	{
		maxYaw = 30;
	}	

	yawDiff = clamp( yawDiff, -maxYaw, maxYaw );

	chaseAngles[PITCH]	= 0.0f;
	chaseAngles[ROLL]	= 0.0f;

	bool bMaxHits = ( m_iBurstHits >= GUNSHIP_MAX_HITS_PER_BURST || (GetEnemy() && !GetEnemy()->IsAlive()) );

	if ( bMaxHits )
	{
		// We've hit our target. Stop chasing, and return to max speed.
		chaseAngles[YAW] = lastChaseAngles[YAW];
		lastSpeed = BASE_STITCH_VELOCITY;
	}
	else
	{
		// Move towards the target yaw
		chaseAngles[YAW] = UTIL_AngleMod( lastChaseAngles[YAW] - yawDiff );
	}

	// If we've hit the target already, or we're not close enough to it, then just stitch along
	if ( bMaxHits || ( m_vecAttackPosition - enemyPos ).LengthSqr() > (64 * 64) )
	{
		AngleVectors( chaseAngles, &targetDir );

		// Update our new velocity
		m_vecAttackVelocity = targetDir * lastSpeed;

		if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
		{
			NDebugOverlay::Line( m_vecAttackPosition, m_vecAttackPosition + (m_vecAttackVelocity * 0.1), 255, 0, 0, true, 4.0f );
		}

		// Move along that velocity for this step in time
		m_vecAttackPosition += ( m_vecAttackVelocity * 0.1f );
		m_vecAttackPosition.z = enemyPos.z;
	}
	else
	{
		// Otherwise always continue to hit an NPC when close enough
		m_vecAttackPosition = enemyPos;
	}
}

//------------------------------------------------------------------------------
// Purpose: Utility function to aim the helicopter gun at the direction
//------------------------------------------------------------------------------
bool CNPC_CombineGunship::PoseGunTowardTargetDirection( const Vector &vTargetDir )
{
	Vector vecOut;
	VectorIRotate( vTargetDir, EntityToWorldTransform(), vecOut );

	QAngle angles;
	VectorAngles(vecOut, angles);
	angles.y = AngleNormalize( angles.y );
	angles.x = AngleNormalize( angles.x );

	if (angles.x > m_angGun.x)
	{
		m_angGun.x = MIN( angles.x, m_angGun.x + 12 );
	}
	if (angles.x < m_angGun.x)
	{
		m_angGun.x = MAX( angles.x, m_angGun.x - 12 );
	}
	if (angles.y > m_angGun.y)
	{
		m_angGun.y = MIN( angles.y, m_angGun.y + 12 );
	}
	if (angles.y < m_angGun.y)
	{
		m_angGun.y = MAX( angles.y, m_angGun.y - 12 );
	}

	SetPoseParameter( m_poseWeapon_Pitch, -m_angGun.x );
	SetPoseParameter( m_poseWeapon_Yaw, m_angGun.y );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetMissileTarget( void )
{
	return GetEnemy()->GetAbsOrigin();
}

//------------------------------------------------------------------------------
// Purpose : Get the target position for the enemy- the position we fire upon.
//			 this is often modified by m_flAttackOffset to provide the 'stitching'
//			 behavior that's so popular with the kids these days (sjb)
//
// Input   : vGunPosition - location of gunship's muzzle
//		   : pTarget = vector to paste enemy target into.
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetEnemyTarget( void )
{
	// Make sure we have an enemy
	if ( GetEnemy() == NULL )
		return m_vecAttackPosition;

	// If we're locked onto a missile, use special code to try and destroy it
	if ( IsTargettingMissile() )
		return GetMissileTarget();

	return m_vecAttackPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoImpactEffect( trace_t &tr, int nDamageType )
{
	UTIL_ImpactTrace( &tr, nDamageType, "ImpactGunship" );

	// These glow effects don't sort properly, so they're cut for E3 2003 (sjb)
#if 0 
	CEffectData data;

	data.m_vOrigin = tr.endpos;
	data.m_vNormal = vec3_origin;
	data.m_vAngles = vec3_angle;

	DispatchEffect( "GunshipImpact", data );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Make the gunship's signature blue tracer!
// Input  : &vecTracerSrc - 
//			&tr - 
//			iTracerType - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
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

			UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 8000, true, "GunshipTracer" );
		}
		break;

	default:
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecDir - 
//			*ptr - 
// Output : int
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Reflect bullets
	if ( info.GetDamageType() & DMG_BULLET )
	{
		if ( random->RandomInt( 0, 2 ) == 0 )
		{
			Vector vecRicochetDir = vecDir * -1;

			vecRicochetDir.x += random->RandomFloat( -0.5, 0.5 );
			vecRicochetDir.y += random->RandomFloat( -0.5, 0.5 );
			vecRicochetDir.z += random->RandomFloat( -0.5, 0.5 );

			VectorNormalize( vecRicochetDir );

			Vector end = ptr->endpos + vecRicochetDir * 1024;
			UTIL_Tracer( ptr->endpos, end, entindex(), TRACER_DONT_USE_ATTACHMENT, 3500 );
		}

		// If this is from a player, record it so a nearby citizen can respond.
		if ( info.GetAttacker()->IsPlayer() )
		{
			if ( GetCitizenResponse() )
			{
				GetCitizenResponse()->AddResponseTrigger( CR_PLAYER_SHOT_GUNSHIP );
			}

#ifdef HL2_EPISODIC
			NPCEventResponse()->TriggerEvent( "TLK_CITIZEN_RESPONSE_SHOT_GUNSHIP", false, false );
#endif
		}

		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: This is necessary to ensure that the game doesn't break if a mapmaker has outputs that
//			must be fired on gunships, and the player switches skill levels 
//			midway through a gunship battle.
// Input  : iDamageNumber - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::FireDamageOutputsUpto( int iDamageNumber )
{
	for ( int i = 0; i <= iDamageNumber; i++ )
	{
		if ( !m_bDamageOutputsFired[i] )
		{
			m_bDamageOutputsFired[i] = true;

			switch ( i )
			{
			case 0:
				//Msg("Fired first\n");
				m_OnFirstDamage.FireOutput( this, this );
				break;
			
			case 1:
				//Msg("Fired second\n");
				m_OnSecondDamage.FireOutput( this, this );
				break;
			
			case 2:
				//Msg("Fired third\n");
				m_OnThirdDamage.FireOutput( this, this );
				break;
			
			case 3:
				//Msg("Fired fourth\n");
				m_OnFourthDamage.FireOutput( this, this );
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
// Damage filtering
//------------------------------------------------------------------------------
int	CNPC_CombineGunship::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	// Allow npc_kill to kill me
	if ( inputInfo.GetDamageType() != DMG_GENERIC )
	{
		// Ignore mundane bullet damage.
		if ( ( inputInfo.GetDamageType() & DMG_BLAST ) == false )
			return 0;

		// Ignore blasts less than this amount
		if ( inputInfo.GetDamage() < GUNSHIP_MIN_DAMAGE_THRESHOLD )
			return 0;
	}

	// Only take blast damage
	CTakeDamageInfo info = inputInfo;

	// Make a pain sound
	if ( !HasSpawnFlags( SF_GUNSHIP_USE_CHOPPER_MODEL ) )
	{
		EmitSound( "NPC_CombineGunship.Pain" );
	}

	Vector	damageDir = info.GetDamageForce();
	VectorNormalize( damageDir );

	// Don't get knocked around if I'm ground attacking
	if ( !m_bIsGroundAttacking )
	{
		ApplyAbsVelocityImpulse( damageDir * 200.0f );
	}
	
	if ( m_bInvulnerable == false )
	{
		// Take a percentage of our health away
		// Adjust health for damage
		int iHealthIncrements = sk_gunship_health_increments.GetInt();
		if ( g_pGameRules->IsSkillLevel( SKILL_EASY ) )
		{
			iHealthIncrements = ceil( iHealthIncrements * 0.5 );
		}
		else if ( g_pGameRules->IsSkillLevel( SKILL_HARD ) )
		{
			iHealthIncrements = floor( iHealthIncrements * 1.5 );
		}
		info.SetDamage( ( GetMaxHealth() / (float)iHealthIncrements ) + 1 );
		
		// Find out which "stage" we're at in our health
		int healthIncrement = iHealthIncrements - ( GetHealth() / (float)(( GetMaxHealth() / (float)iHealthIncrements )) );
		switch ( healthIncrement )
		{
		case 1:
			// If we're on Easy, we're half dead now, so fire the rest of our outputs too
			// This is done in case the mapmaker's connected those inputs to something important 
			// that has to happen before the gunship dies.
			if ( g_pGameRules->IsSkillLevel( SKILL_EASY ) )
			{
				FireDamageOutputsUpto( 3 );
			}
			else
			{
				FireDamageOutputsUpto( 1 );
			}
			break;

		default:
			FireDamageOutputsUpto( healthIncrement );
			break;
		}

		// Start smoking when we're almost dead
		CreateSmokeTrail();

		if ( m_pSmokeTrail )
		{
			if ( healthIncrement < 2 )
			{
				m_pSmokeTrail->SetLifetime( 8.0 );
			}

			m_pSmokeTrail->FollowEntity( this, "exhaustl" );
		}

		// Move with the target
		Vector	gibVelocity = GetAbsVelocity() + (-damageDir * 200.0f);

		// Dump out metal gibs
		CPVSFilter filter( GetAbsOrigin() );
	 	for ( int i = 0; i < 10; i++ )
		{
			int iModelIndex = modelinfo->GetModelIndex( g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ) );	
			te->BreakModel( filter, 0.0, GetAbsOrigin(), vec3_angle, Vector(40,40,40), gibVelocity, iModelIndex, 400, 1, 2.5, BREAK_METAL );
		}
	}

	return BaseClass::OnTakeDamage_Alive( info );
}


//------------------------------------------------------------------------------
// Purpose : The proper way to begin the gunship cannon firing at the enemy.
// Input   : iBurstSize - the size of the burst, in rounds.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::StartCannonBurst( int iBurstSize )
{
	m_iBurstSize = iBurstSize;
	m_iBurstHits = 0;

	m_flTimeNextAttack = gpGlobals->curtime;

	// Start up the cannon sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pCannonSound, 1.0, 0 );

	m_bIsFiring = true;

	// Setup the initial position of the burst
	if ( GetEnemy() )
	{
		// Follow mode
		Vector	enemyPos;
		UTIL_PredictedPosition( GetEnemy(), 2.0f, &enemyPos );

		QAngle offsetAngles;
		Vector offsetDir = ( WorldSpaceCenter() - enemyPos );
		VectorNormalize( offsetDir );
		VectorAngles( offsetDir, offsetAngles );

		int angleOffset = random->RandomInt( 15, 30 );
		if ( random->RandomInt( 0, 1 ) )
		{
			angleOffset *= -1;
		}
		offsetAngles[YAW] += angleOffset;
		offsetAngles[PITCH] = 0;
		offsetAngles[ROLL] = 0;

		AngleVectors( offsetAngles, &offsetDir );

		float stitchOffset;
		float enemyDist = GroundDistToPosition( GetEnemy()->GetAbsOrigin() );
		if ( enemyDist < ( sk_gunship_burst_dist.GetFloat() + GUNSHIP_STITCH_MIN ) )
		{
			stitchOffset = GUNSHIP_STITCH_MIN;
		}
		else
		{
			stitchOffset = sk_gunship_burst_dist.GetFloat();
		}

		// Move out to the start of our stitch run
		m_vecAttackPosition = enemyPos + ( offsetDir * stitchOffset );
		m_vecAttackPosition.z = enemyPos.z;

		// Point at our target
		m_vecAttackVelocity = -offsetDir * BASE_STITCH_VELOCITY;

		CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_REACT_TO_SOURCE, enemyPos, 512, 0.2f, this );
	}
}


//------------------------------------------------------------------------------
// Purpose : The proper way to cease the gunship cannon firing. 
//------------------------------------------------------------------------------
void CNPC_CombineGunship::StopCannonBurst( void )
{
	m_iBurstHits = 0;
	m_bIsFiring = false;
	m_bPreFire = false;

	// Reduce the burst time when we get lower in health
	float flPerc = (float)GetHealth() / (float)GetMaxHealth();
	float flDelay = clamp( flPerc * m_flBurstDelay, 0.5, m_flBurstDelay );

	// If we didn't finish the burst, don't wait so long
	flPerc = 1.0 - (m_iBurstSize / sk_gunship_burst_size.GetFloat());
	flDelay *= flPerc;

	m_flTimeNextAttack = gpGlobals->curtime + flDelay;
	m_iBurstSize = 0;

	// Stop the cannon sound.
	if ( m_pCannonSound != NULL )
	{
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pCannonSound, 0.0, 0.05 );
	}

	EmitSound( "NPC_CombineGunship.CannonStopSound" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StopLoopingSounds( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pCannonSound )
	{
		controller.SoundDestroy( m_pCannonSound );
		m_pCannonSound = NULL;
	}

	if ( m_pRotorSound )
	{
		controller.SoundDestroy( m_pRotorSound );
		m_pRotorSound = NULL;
	}

	if ( m_pAirExhaustSound )
	{
		controller.SoundDestroy( m_pAirExhaustSound );
		m_pAirExhaustSound = NULL;
	}

	if ( m_pAirBlastSound )
	{
		controller.SoundDestroy( m_pAirBlastSound );
		m_pAirBlastSound = NULL;
	}

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::IsValidEnemy( CBaseEntity *pEnemy )
{
	// Always track missiles
	if ( pEnemy->IsAlive() && !pEnemy->MyNPCPointer() && FClassnameIs( pEnemy, "rpg_missile" ) )
		return true;

	// If we're shooting off a burst, don't pick up a new enemy
	if ( ( m_bIsFiring ) && ( ( GetEnemy() == NULL ) || ( GetEnemy() != pEnemy ) ) )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::GatherEnemyConditions( CBaseEntity *pEnemy )
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
// Purpose: Tells us whether or not we're targetting an incoming missile
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::IsTargettingMissile( void )
{
	if ( GetEnemy() == NULL )
		return false;

	if ( FClassnameIs( GetEnemy(), "rpg_missile" ) == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBecomeInvulnerable( inputdata_t &input )
{
	m_bInvulnerable = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBecomeVulnerable( inputdata_t &input )
{
	m_bInvulnerable = false;
}

AI_BEGIN_CUSTOM_NPC( npc_combinegunship, CNPC_CombineGunship )

//	DECLARE_TASK(  )

	DECLARE_ACTIVITY( ACT_GUNSHIP_PATROL );
	DECLARE_ACTIVITY( ACT_GUNSHIP_HOVER );
	DECLARE_ACTIVITY( ACT_GUNSHIP_CRASH );

	//DECLARE_CONDITION( COND_ )

	//=========================================================
//	DEFINE_SCHEDULE
//	(
//		SCHED_DUMMY,
//
//		"	Tasks"
//		"		TASK_FACE_ENEMY			0"
//		"	"
//		"	Interrupts"
//	)


AI_END_CUSTOM_NPC()

