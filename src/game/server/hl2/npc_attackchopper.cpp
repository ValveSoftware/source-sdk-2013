//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "ai_network.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_task.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "soundenvelope.h"
#include "gib.h"
#include "gamerules.h"
#include "ammodef.h"
#include "grenade_homer.h"
#include "cbasehelicopter.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "globals.h"
#include "explode.h"
#include "movevars_shared.h"
#include "smoke_trail.h"
#include "ar2_explosion.h"
#include "collisionutils.h"
#include "props.h"
#include "EntityFlame.h"
#include "decals.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "ai_spotlight.h"
#include "vphysics/constraints.h"
#include "physics_saverestore.h"
#include "ai_memory.h"
#include "npc_attackchopper.h"

#ifdef HL2_EPISODIC
#include "physics_bone_follower.h"
#endif // HL2_EPISODIC

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------
// Bone controllers
// -------------------------------------
#define CHOPPER_DRONE_NAME	"models/combine_helicopter/helicopter_bomb01.mdl"
#define CHOPPER_MODEL_NAME	"models/combine_helicopter.mdl"
#define CHOPPER_MODEL_CORPSE_NAME	"models/combine_helicopter_broken.mdl"
#define CHOPPER_RED_LIGHT_SPRITE	"sprites/redglow1.vmt"

#define CHOPPER_MAX_SMALL_CHUNKS	1
#define CHOPPER_MAX_CHUNKS	3
static const char *s_pChunkModelName[CHOPPER_MAX_CHUNKS] = 
{
	"models/gibs/helicopter_brokenpiece_01.mdl",
	"models/gibs/helicopter_brokenpiece_02.mdl",
	"models/gibs/helicopter_brokenpiece_03.mdl",
};

#define BOMB_SKIN_LIGHT_ON		1
#define BOMB_SKIN_LIGHT_OFF		0


#define	HELICOPTER_CHUNK_COCKPIT	"models/gibs/helicopter_brokenpiece_04_cockpit.mdl"
#define	HELICOPTER_CHUNK_TAIL		"models/gibs/helicopter_brokenpiece_05_tailfan.mdl"
#define	HELICOPTER_CHUNK_BODY		"models/gibs/helicopter_brokenpiece_06_body.mdl"


#define CHOPPER_MAX_SPEED			(60 * 17.6f)
#define CHOPPER_MAX_FIRING_SPEED	250.0f
#define CHOPPER_MAX_GUN_DIST		2000.0f

#define CHOPPER_ACCEL_RATE			500
#define CHOPPER_ACCEL_RATE_BOOST	1500

#define DEFAULT_FREE_KNOWLEDGE_DURATION 5.0f

// -------------------------------------
// Pathing data
#define	CHOPPER_LEAD_DISTANCE			800.0f
#define	CHOPPER_MIN_CHASE_DIST_DIFF		128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define CHOPPER_MIN_AGGRESSIVE_CHASE_DIST_DIFF 64.0f
#define	CHOPPER_AVOID_DIST				512.0f
#define	CHOPPER_ARRIVE_DIST				128.0f

#define CHOPPER_MAX_CIRCLE_OF_DEATH_FOLLOW_SPEED	450.0f
#define CHOPPER_MIN_CIRCLE_OF_DEATH_RADIUS	150.0f
#define CHOPPER_MAX_CIRCLE_OF_DEATH_RADIUS	350.0f

#define CHOPPER_BOMB_DROP_COUNT 6

// Bullrush
#define CHOPPER_BULLRUSH_MODE_DISTANCE g_helicopter_bullrush_distance.GetFloat()
#define CHOPPER_BULLRUSH_ENEMY_BOMB_DISTANCE g_helicopter_bullrush_bomb_enemy_distance.GetFloat()
#define CHOPPER_BULLRUSH_ENEMY_BOMB_TIME g_helicopter_bullrush_bomb_time.GetFloat()
#define CHOPPER_BULLRUSH_ENEMY_BOMB_SPEED g_helicopter_bullrush_bomb_speed.GetFloat()
#define CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET g_helicopter_bullrush_shoot_height.GetFloat()

#define CHOPPER_GUN_CHARGE_TIME		g_helicopter_chargetime.GetFloat()
#define CHOPPER_GUN_IDLE_TIME		g_helicopter_idletime.GetFloat()
#define CHOPPER_GUN_MAX_FIRING_DIST	g_helicopter_maxfiringdist.GetFloat()

#define BULLRUSH_IDLE_PLAYER_FIRE_TIME 6.0f

#define DRONE_SPEED	sk_helicopter_drone_speed.GetFloat()

#define SF_HELICOPTER_LOUD_ROTOR_SOUND		0x00010000
#define SF_HELICOPTER_ELECTRICAL_DRONE		0x00020000
#define SF_HELICOPTER_LIGHTS				0x00040000
#define SF_HELICOPTER_IGNORE_AVOID_FORCES	0x00080000
#define SF_HELICOPTER_AGGRESSIVE			0x00100000
#define SF_HELICOPTER_LONG_SHADOW			0x00200000

#define CHOPPER_SLOW_BOMB_SPEED	250

#define CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED	250
#define CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_SQ	(CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED * CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED)

#define CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_2	450
#define CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_2_SQ	(CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_2 * CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_2)

// CVars
ConVar	sk_helicopter_health( "sk_helicopter_health","5600");
ConVar	sk_helicopter_firingcone( "sk_helicopter_firingcone","20.0", 0, "The angle in degrees of the cone in which the shots will be fired" );
ConVar	sk_helicopter_burstcount( "sk_helicopter_burstcount","12", 0, "How many shot bursts to fire after charging up. The bigger the number, the longer the firing is" );
ConVar	sk_helicopter_roundsperburst( "sk_helicopter_roundsperburst","5", 0, "How many shots to fire in a single burst" );

ConVar	sk_helicopter_grenadedamage( "sk_helicopter_grenadedamage","25.0", 0, "The amount of damage the helicopter grenade deals." );
ConVar	sk_helicopter_grenaderadius( "sk_helicopter_grenaderadius","275.0", 0, "The damage radius of the helicopter grenade." );
ConVar	sk_helicopter_grenadeforce( "sk_helicopter_grenadeforce","55000.0", 0, "The physics force that the helicopter grenade exerts." );
ConVar	sk_helicopter_grenade_puntscale( "sk_helicopter_grenade_puntscale","1.5", 0, "When physpunting a chopper's grenade, scale its velocity by this much." );

// Number of bomb hits it takes to kill a chopper on each skill level.
ConVar sk_helicopter_num_bombs1("sk_helicopter_num_bombs1", "3");
ConVar sk_helicopter_num_bombs2("sk_helicopter_num_bombs2", "5");
ConVar sk_helicopter_num_bombs3("sk_helicopter_num_bombs3", "5");

ConVar	sk_npc_dmg_helicopter_to_plr( "sk_npc_dmg_helicopter_to_plr","3", 0, "Damage helicopter shots deal to the player" );
ConVar	sk_npc_dmg_helicopter( "sk_npc_dmg_helicopter","6", 0, "Damage helicopter shots deal to everything but the player" );

ConVar	sk_helicopter_drone_speed( "sk_helicopter_drone_speed","450.0", 0, "How fast does the zapper drone move?" );

ConVar	g_helicopter_chargetime( "g_helicopter_chargetime","2.0", 0, "How much time we have to wait (on average) between the time we start hearing the charging sound + the chopper fires" );
ConVar	g_helicopter_bullrush_distance("g_helicopter_bullrush_distance", "5000");
ConVar	g_helicopter_bullrush_bomb_enemy_distance("g_helicopter_bullrush_bomb_enemy_distance", "0");
ConVar	g_helicopter_bullrush_bomb_time("g_helicopter_bullrush_bomb_time", "10");
ConVar	g_helicopter_idletime( "g_helicopter_idletime","3.0", 0, "How much time we have to wait (on average) after we fire before we can charge up again" );
ConVar	g_helicopter_maxfiringdist( "g_helicopter_maxfiringdist","2500.0", 0, "The maximum distance the player can be from the chopper before it stops firing" );
ConVar	g_helicopter_bullrush_bomb_speed( "g_helicopter_bullrush_bomb_speed","850.0", 0, "The maximum distance the player can be from the chopper before it stops firing" );
ConVar	g_helicopter_bullrush_shoot_height( "g_helicopter_bullrush_shoot_height","650.0", 0, "The maximum distance the player can be from the chopper before it stops firing" );
ConVar	g_helicopter_bullrush_mega_bomb_health( "g_helicopter_bullrush_mega_bomb_health","0.25", 0, "Fraction of the health of the chopper before it mega-bombs" );

ConVar	g_helicopter_bomb_danger_radius( "g_helicopter_bomb_danger_radius", "120" );

Activity ACT_HELICOPTER_DROP_BOMB;
Activity ACT_HELICOPTER_CRASHING;

static const char *s_pBlinkLightThinkContext = "BlinkLights";
static const char *s_pSpotlightThinkContext = "SpotlightThink";
static const char *s_pRampSoundContext = "RampSound";
static const char *s_pWarningBlinkerContext = "WarningBlinker";
static const char *s_pAnimateThinkContext = "Animate";

#define CHOPPER_LIGHT_BLINK_TIME		1.0f
#define CHOPPER_LIGHT_BLINK_TIME_SHORT	0.1f

#define BOMB_LIFETIME	2.5f	// Don't access this directly. Call GetBombLifetime();
#define BOMB_RAMP_SOUND_TIME 1.0f

enum
{
	MAX_HELICOPTER_LIGHTS = 3,
};

enum
{
	SF_GRENADE_HELICOPTER_MEGABOMB = 0x1,
};

#define GRENADE_HELICOPTER_MODEL "models/combine_helicopter/helicopter_bomb01.mdl"

LINK_ENTITY_TO_CLASS( info_target_helicopter_crash, CPointEntity );


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
static inline float ClampSplineRemapVal( float flValue, float flMinValue, float flMaxValue, float flOutMin, float flOutMax )
{
	Assert( flMinValue <= flMaxValue );
	float flClampedVal = clamp( flValue, flMinValue, flMaxValue );
	return SimpleSplineRemapVal( flClampedVal, flMinValue, flMaxValue, flOutMin, flOutMax );
}


//-----------------------------------------------------------------------------
// The bombs the attack helicopter drops 
//-----------------------------------------------------------------------------
enum
{
	SKIN_REGULAR,
	SKIN_DUD,
};

class CGrenadeHelicopter : public CBaseGrenade
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CGrenadeHelicopter, CBaseGrenade );

	virtual void Precache( );
	virtual void Spawn( );
	virtual void UpdateOnRemove();
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );
	virtual void PhysicsSimulate( void );
	virtual float GetShakeAmplitude( void ) { return 25.0; }
	virtual float GetShakeRadius( void ) { return sk_helicopter_grenaderadius.GetFloat() * 2; }
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void		 SetExplodeOnContact( bool bExplode ) { m_bExplodeOnContact = bExplode; }

	virtual QAngle PreferredCarryAngles( void ) { return QAngle( -12, 98, 55 ); }
	virtual bool HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return true; }

	float GetBombLifetime();

#ifdef HL2_EPISODIC
	virtual void OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );
	virtual Class_T Classify( void ) {	return CLASS_MISSILE; }
	void SetCollisionObject( CBaseEntity *pEntity ) { m_hCollisionObject = pEntity; }
	void SendMissEvent();	
	bool IsThrownByPlayer();

	virtual bool	ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return ( reason == PHYSGUN_FORCE_LAUNCHED ); }
	virtual Vector  PhysGunLaunchVelocity( const Vector &forward, float flMass );

	void InputExplodeIn( inputdata_t &inputdata );
#endif // HL2_EPISODIC

private:
	// Pow!
	void DoExplosion( const Vector &vecOrigin, const Vector &vecVelocity );
	void ExplodeThink();
	void RampSoundThink();
	void WarningBlinkerThink();
	void StopWarningBlinker();
	void AnimateThink();
	void ExplodeConcussion( CBaseEntity *pOther );
	void BecomeActive();
	void ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	bool m_bActivated;
	bool m_bExplodeOnContact;
	CSoundPatch	*m_pWarnSound;

	EHANDLE m_hWarningSprite;
	bool m_bBlinkerAtTop;


#ifdef HL2_EPISODIC
	float m_flLifetime;
	EHANDLE	m_hCollisionObject;	// Pointer to object we re-enable collisions with when picked up
	bool m_bPickedUp;
	float m_flBlinkFastTime;
	COutputEvent m_OnPhysGunOnlyPickup;
#endif // HL2_EPISODIC
};


//-----------------------------------------------------------------------------
// The bombs the attack helicopter drops 
//-----------------------------------------------------------------------------
class CBombDropSensor : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CBombDropSensor, CBaseEntity );

	void Spawn();

	// Drop a bomb at a particular location
	void	InputDropBomb( inputdata_t &inputdata );
	void	InputDropBombStraightDown( inputdata_t &inputdata );
	void	InputDropBombAtTarget( inputdata_t &inputdata );
	void	InputDropBombAtTargetAlways( inputdata_t &inputdata );
	void	InputDropBombDelay( inputdata_t &inputdata );
};

//-----------------------------------------------------------------------------
// This entity is used to create boxes that the helicopter can't bomb in
//-----------------------------------------------------------------------------
class CBombSuppressor : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CBombSuppressor, CBaseEntity );

	virtual void Spawn( );
	virtual void Activate();
	virtual void UpdateOnRemove();

	static bool CanBomb( const Vector &vecPosition );

private:
	typedef CHandle<CBombSuppressor> BombSuppressorHandle_t;	
	static CUtlVector< BombSuppressorHandle_t > s_BombSuppressors; 
};

	enum 
	{
		CHUNK_COCKPIT,
		CHUNK_BODY,
		CHUNK_TAIL
	};

//-----------------------------------------------------------------------------
// This entity is used for helicopter gibs with specific properties
//-----------------------------------------------------------------------------
class CHelicopterChunk : public CBaseAnimating
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CHelicopterChunk, CBaseAnimating );

	virtual void Spawn( void );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	static CHelicopterChunk *CreateHelicopterChunk( const Vector &vecPos, const QAngle &vecAngles, const Vector &vecVelocity, const char *pszModelName, int chunkID );

	int		m_nChunkID;
	
	CHandle<CHelicopterChunk>	m_hMaster;
	IPhysicsConstraint			*m_pTailConstraint;
	IPhysicsConstraint			*m_pCockpitConstraint;

protected:

	void	CollisionCallback( CHelicopterChunk *pCaller );

	void	FallThink( void );

	bool	m_bLanded;
};

//-----------------------------------------------------------------------------
// The attack helicopter 
//-----------------------------------------------------------------------------
class CNPC_AttackHelicopter : public CBaseHelicopter
{
public:
	DECLARE_CLASS( CNPC_AttackHelicopter, CBaseHelicopter );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	CNPC_AttackHelicopter();
	~CNPC_AttackHelicopter();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual bool	CreateComponents();
	virtual int		ObjectCaps();

#ifdef HL2_EPISODIC
	virtual bool	CreateVPhysics( void );
#endif // HL2_EPISODIC

	virtual void	UpdateOnRemove();
	virtual void	StopLoopingSounds();

	int		BloodColor( void ) { return DONT_BLEED; }
	Class_T Classify ( void ) { return CLASS_COMBINE_GUNSHIP; }
	virtual int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	// Shot spread
	virtual Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget );

	// More Enemy visibility check
	virtual bool FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );

	// Think!
	virtual void PrescheduleThink( void );

	// Purpose: Set the gunship's paddles flailing!
	virtual void Event_Killed( const CTakeDamageInfo &info );

	// Drop a bomb at a particular location
	void	InputDropBomb( inputdata_t &inputdata );
	void	InputDropBombStraightDown( inputdata_t &inputdata );
	void	InputDropBombAtTarget( inputdata_t &inputdata );
	void	InputDropBombAtTargetAlways( inputdata_t &inputdata );
	void	InputDropBombAtTargetInternal( inputdata_t &inputdata, bool bCheckFairness );
	void	InputDropBombDelay( inputdata_t &inputdata );
	void	InputStartCarpetBombing( inputdata_t &inputdata );
	void	InputStopCarpetBombing( inputdata_t &inputdata );

	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual const char *GetTracerType( void );

	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	virtual void DoMuzzleFlash( void );

	// FIXME: Work this back into the base class
	virtual bool ShouldUseFixedPatrolLogic() { return true; }

protected:

	int m_poseWeapon_Pitch, m_poseWeapon_Yaw, m_poseRudder;
	virtual void	PopulatePoseParameters( void );

private:
	enum GunState_t
	{
		GUN_STATE_IDLE = 0,
		GUN_STATE_CHARGING,
		GUN_STATE_FIRING,
	};

	// Gets the max speed of the helicopter
	virtual float GetMaxSpeed();
	virtual float GetMaxSpeedFiring();

	// Startup the chopper
	virtual void Startup();

	void	InitializeRotorSound( void );

	// Weaponry
	bool	FireGun( void );

	// Movement:	
	virtual void Flight( void );

	// Changes the main thinking method of helicopters
	virtual void Hunt( void );

	// For scripted times where it *has* to shoot
	void	InputResetIdleTime( inputdata_t &inputdata );
	void	InputSetHealthFraction( inputdata_t &inputdata );
	void	InputStartBombExplodeOnContact( inputdata_t &inputdata );
	void	InputStopBombExplodeOnContact( inputdata_t &inputdata );

	void	InputEnableAlwaysTransition( inputdata_t &inputdata );
	void	InputDisableAlwaysTransition( inputdata_t &inputdata );
	void 	InputOutsideTransition( inputdata_t &inputdata );
	void 	InputSetOutsideTransitionTarget( inputdata_t &inputdata );

	// Turns off the gun
	void	InputGunOff( inputdata_t &inputdata );

	// Vehicle attack modes
	void	InputStartBombingVehicle( inputdata_t &inputdata );
	void	InputStartTrailingVehicle( inputdata_t &inputdata );
	void	InputStartDefaultBehavior( inputdata_t &inputdata );
	void	InputStartAlwaysLeadingVehicle( inputdata_t &inputdata );

	// Deadly shooting, tex!
	void	InputEnableDeadlyShooting( inputdata_t &inputdata );
	void	InputDisableDeadlyShooting( inputdata_t &inputdata );
	void	InputStartNormalShooting( inputdata_t &inputdata );
	void	InputStartLongCycleShooting( inputdata_t &inputdata );
	void	InputStartContinuousShooting( inputdata_t &inputdata );
	void	InputStartFastShooting( inputdata_t &inputdata );

	int		GetShootingMode( );
	bool	IsDeadlyShooting();

	// Bombing suppression
	void	InputEnableBombing( inputdata_t &inputdata );
	void 	InputDisableBombing( inputdata_t &inputdata );

	// Visibility tests
	void	InputDisablePathVisibilityTests( inputdata_t &inputdata );
	void 	InputEnablePathVisibilityTests( inputdata_t &inputdata );

	// Death, etc.
	void	InputSelfDestruct( inputdata_t &inputdata );

	// Enemy visibility check
	CBaseEntity *FindTrackBlocker( const Vector &vecViewPoint, const Vector &vecTargetPos );

	// Special path navigation when we explicitly want to follow a path
	void UpdateFollowPathNavigation();

	// Find interesting nearby things to shoot
	int BuildMissTargetList( int nCount, CBaseEntity **ppMissCandidates );

	// Shoot when the player's your enemy :			 
	void ShootAtPlayer( const Vector &vBasePos, const Vector &vGunDir );

	// Shoot when the player's your enemy + he's driving a vehicle
	void ShootAtVehicle( const Vector &vBasePos, const Vector &vGunDir );

	// Shoot where we're facing
	void ShootAtFacingDirection( const Vector &vBasePos, const Vector &vGunDir, bool bFirstShotAccurate );

	// Updates the facing direction
	void UpdateFacingDirection( const Vector &vecActualDesiredPosition );

	// Various states of the helicopter firing...
	bool PoseGunTowardTargetDirection( const Vector &vTargetDir );

	// Compute the position to fire at (vehicle + non-vehicle case)
	void ComputeFireAtPosition( Vector *pVecActualTargetPosition );
	void ComputeVehicleFireAtPosition( Vector *pVecActualTargetPosition );

	// Various states of the helicopter firing...
	bool DoGunIdle( const Vector &vecGunDir, const Vector &vTargetDir );
	bool DoGunCharging( );
	bool DoGunFiring( const Vector &vBasePos, const Vector &vGunDir, const Vector &vecFireAtPosition );
	void FireElectricityGun( );

	// Chooses a point within the circle of death to fire in
	void PickDirectionToCircleOfDeath( const Vector &vBasePos, const Vector &vecFireAtPosition, Vector *pResult );

	// Gets a vehicle the enemy is in (if any)
	CBaseEntity *GetEnemyVehicle();

	// Updates the perpendicular path distance for the chopper	
	float UpdatePerpPathDistance( float flMaxPathOffset );

	// Purpose :
	void UpdateEnemyLeading( void );

	// Drop those bombs!
	void DropBombs( );

	// Should we drop those bombs?
	bool ShouldDropBombs( void );

	// Returns the max firing distance
	float GetMaxFiringDistance();

	// Make sure we don't hit too many times
	void FireBullets( const FireBulletsInfo_t &info );

	// Is it "fair" to drop this bomb?
	bool IsBombDropFair( const Vector &vecBombStartPos, const Vector &vecVelocity );

	// Actually drops the bomb
	void CreateBomb( bool bCheckForFairness = true, Vector *pVecVelocity = NULL, bool bMegaBomb = false );
	CGrenadeHelicopter *SpawnBombEntity( const Vector &vecPos, const Vector &vecVelocity ); // Spawns the bomb entity and sets it up

	// Deliberately aims as close as possible w/o hitting
	void AimCloseToTargetButMiss( CBaseEntity *pTarget, float flMinDist, float flMaxDist, const Vector &shootOrigin, Vector *pResult );

	// Pops a shot inside the circle of death using the burst rules
	void ShootInsideCircleOfDeath( const Vector &vBasePos, const Vector &vecFireAtPosition );

	// How easy is the target to hit?
	void UpdateTargetHittability();

	// Add a smoke trail since we've taken more damage
	void AddSmokeTrail( const Vector &vecPos );

	// Destroy all smoke trails
	void DestroySmokeTrails();

	// Creates the breakable husk of an attack chopper
	void CreateChopperHusk();

	// Pow!
	void ExplodeAndThrowChunk( const Vector &vecExplosionPos );

	// Drop a corpse!
	void DropCorpse( int nDamage );

	// Should we trigger a damage effect?
	bool ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const;

	// Become indestructible
	void InputBecomeIndestructible( inputdata_t &inputdata );

	// Purpose :
	float CreepTowardEnemy( float flSpeed, float flMinSpeed, float flMaxSpeed, float flMinDist, float flMaxDist );

	// Start bullrush
	void InputStartBullrushBehavior( inputdata_t &inputdata );

	void GetMaxSpeedAndAccel( float *pMaxSpeed, float *pAccelRate );
	void ComputeAngularVelocity( const Vector &vecGoalUp, const Vector &vecFacingDirection );
	void ComputeVelocity( const Vector &deltaPos, float flAdditionalHeight, float flMinDistFromSegment, float flMaxDistFromSegment, Vector *pVecAccel );
	void FlightDirectlyOverhead( void );

	// Methods related to computing leading distance
	float ComputeBombingLeadingDistance( float flSpeed, float flSpeedAlongPath, bool bEnemyInVehicle );
	float ComputeBullrushLeadingDistance( float flSpeed, float flSpeedAlongPath, bool bEnemyInVehicle );

	bool IsCarpetBombing() { return m_bIsCarpetBombing == true; }

	// Update the bullrush state
	void UpdateBullrushState( void );

	// Whether to shoot at or bomb an idle player
	bool ShouldBombIdlePlayer( void );

	// Different bomb-dropping behavior
	void BullrushBombs( );

	// Switch to idle
	void SwitchToBullrushIdle( void );

	// Secondary mode
	void SetSecondaryMode( int nMode, bool bRetainTime = false );
	bool IsInSecondaryMode( int nMode );
	float SecondaryModeTime( ) const;

	// Should the chopper shoot the idle player?
	bool ShouldShootIdlePlayerInBullrush();

	// Shutdown shooting during bullrush
	void ShutdownGunDuringBullrush( );

	// Updates the enemy
	virtual float	EnemySearchDistance( );

	// Prototype zapper
	bool IsValidZapTarget( CBaseEntity *pTarget );
	void CreateZapBeam( const Vector &vecTargetPos );
	void CreateEntityZapEffect( CBaseEntity *pEnt );

	// Blink lights
	void BlinkLightsThink();

	// Spotlights
	void SpotlightThink();
	void SpotlightStartup();
	void SpotlightShutdown();

	CBaseEntity *GetCrashPoint()	{ return m_hCrashPoint.Get(); }

private:
	enum
	{
		ATTACK_MODE_DEFAULT = 0,
		ATTACK_MODE_BOMB_VEHICLE,
		ATTACK_MODE_TRAIL_VEHICLE,
		ATTACK_MODE_ALWAYS_LEAD_VEHICLE,
		ATTACK_MODE_BULLRUSH_VEHICLE,
	};

	enum
	{
		MAX_SMOKE_TRAILS = 5,
		MAX_EXPLOSIONS = 13,
		MAX_CORPSES = 2,
	};

	enum
	{
		BULLRUSH_MODE_WAIT_FOR_ENEMY = 0,
		BULLRUSH_MODE_GET_DISTANCE,
		BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED,
		BULLRUSH_MODE_DROP_BOMBS_FOLLOW_PLAYER,
		BULLRUSH_MODE_SHOOT_GUN,
		BULLRUSH_MODE_MEGA_BOMB,
		BULLRUSH_MODE_SHOOT_IDLE_PLAYER,
	};

	enum
	{
		SHOOT_MODE_DEFAULT = 0,
		SHOOT_MODE_LONG_CYCLE,
		SHOOT_MODE_CONTINUOUS,
		SHOOT_MODE_FAST,
	};

#ifdef HL2_EPISODIC
	void InitBoneFollowers( void );
	CBoneFollowerManager	m_BoneFollowerManager;
#endif // HL2_EPISODIC

	CAI_Spotlight	m_Spotlight;
	Vector		m_angGun;
	QAngle		m_vecAngAcceleration;
	int			m_iAmmoType;
	float		m_flLastCorpseFall;
	GunState_t	m_nGunState;
	float		m_flChargeTime;
	float		m_flIdleTimeDelay;
	int			m_nRemainingBursts;
	int			m_nGrenadeCount;
	float 		m_flPathOffset;
	float 		m_flAcrossTime;
	float		m_flCurrPathOffset;
	int			m_nBurstHits;
	int			m_nMaxBurstHits;
	float		m_flCircleOfDeathRadius;
	int			m_nAttackMode;
	float		m_flInputDropBombTime;
	CHandle<CBombDropSensor>	m_hSensor;
	float		m_flAvoidMetric;
	AngularImpulse m_vecLastAngVelocity;
	CHandle<CBaseEntity>	m_hSmokeTrail[MAX_SMOKE_TRAILS];
	int			m_nSmokeTrailCount;
	bool		m_bIndestructible;
	float		m_flGracePeriod;
	bool		m_bBombsExplodeOnContact;
	bool		m_bNonCombat;

	int			m_nNearShots;
	int			m_nMaxNearShots;

	// Bomb dropping attachments
	int			m_nGunTipAttachment;
	int			m_nGunBaseAttachment;
	int			m_nBombAttachment;
	int			m_nSpotlightAttachment;
	float		m_flLastFastTime;

	// Secondary modes
	int			m_nSecondaryMode;
	float		m_flSecondaryModeStartTime;

	// Bullrush behavior
	bool		m_bRushForward;
	float		m_flBullrushAdditionalHeight;
	int			m_nBullrushBombMode;
	float		m_flNextBullrushBombTime;
	float		m_flNextMegaBombHealth;

	// Shooting method
	int			m_nShootingMode;
	bool		m_bDeadlyShooting;

	// Bombing suppression
	bool		m_bBombingSuppressed;

	// Blinking lights
	CHandle<CSprite> m_hLights[MAX_HELICOPTER_LIGHTS];
	bool		m_bShortBlink;

	// Path behavior
	bool		m_bIgnorePathVisibilityTests;

	// Teleport
	bool		m_bAlwaysTransition;
	string_t	m_iszTransitionTarget;

	// Special attacks
	bool		m_bIsCarpetBombing;

	// Fun damage effects
	float		m_flGoalRollDmg;
	float		m_flGoalYawDmg;

	// Sounds
	CSoundPatch	*m_pGunFiringSound;

	// Outputs
	COutputInt	m_OnHealthChanged;
	COutputEvent m_OnShotDown;

	// Crashing
	EHANDLE		m_hCrashPoint;
};

#ifdef HL2_EPISODIC
static const char *pFollowerBoneNames[] =
{
	"Chopper.Body"
};
#endif // HL2_EPISODIC

LINK_ENTITY_TO_CLASS( npc_helicopter, CNPC_AttackHelicopter );

BEGIN_DATADESC( CNPC_AttackHelicopter )

	DEFINE_ENTITYFUNC( FlyTouch ),

	DEFINE_EMBEDDED( m_Spotlight ),
#ifdef HL2_EPISODIC
	DEFINE_EMBEDDED( m_BoneFollowerManager ),
#endif
	DEFINE_FIELD( m_angGun,				FIELD_VECTOR ),
	DEFINE_FIELD( m_vecAngAcceleration,	FIELD_VECTOR ),
	DEFINE_FIELD( m_iAmmoType,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastCorpseFall,	FIELD_TIME ),
	DEFINE_FIELD( m_nGunState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flChargeTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flIdleTimeDelay,	FIELD_FLOAT ),
	DEFINE_FIELD( m_nRemainingBursts,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nGrenadeCount,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flPathOffset,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flAcrossTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flCurrPathOffset,	FIELD_FLOAT ),
	DEFINE_FIELD( m_nBurstHits,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxBurstHits,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flCircleOfDeathRadius,	FIELD_FLOAT ),
	DEFINE_FIELD( m_nAttackMode,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flInputDropBombTime,	FIELD_TIME ),
	DEFINE_FIELD( m_hSensor,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_flAvoidMetric,		FIELD_FLOAT ),
	DEFINE_FIELD( m_vecLastAngVelocity,	FIELD_VECTOR ),
	DEFINE_AUTO_ARRAY( m_hSmokeTrail,	FIELD_EHANDLE ),
	DEFINE_FIELD( m_nSmokeTrailCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nNearShots,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxNearShots,		FIELD_INTEGER ),
//	DEFINE_FIELD( m_nGunTipAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nGunBaseAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nBombAttachment,	FIELD_INTEGER ),
//	DEFINE_FIELD( m_nSpotlightAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastFastTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nSecondaryMode,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flSecondaryModeStartTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bRushForward,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flBullrushAdditionalHeight,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nBullrushBombMode,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextBullrushBombTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextMegaBombHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_nShootingMode,		FIELD_INTEGER ),
	DEFINE_FIELD( m_bDeadlyShooting,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBombingSuppressed,	FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_pGunFiringSound ),
	DEFINE_AUTO_ARRAY( m_hLights,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_bIgnorePathVisibilityTests, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bShortBlink,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIndestructible,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBombsExplodeOnContact, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_bAlwaysTransition, FIELD_BOOLEAN, "AlwaysTransition" ),
	DEFINE_KEYFIELD( m_iszTransitionTarget, FIELD_STRING, "TransitionTarget" ),
	DEFINE_FIELD( m_bIsCarpetBombing, FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableAlwaysTransition", InputEnableAlwaysTransition ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableAlwaysTransition", InputDisableAlwaysTransition ),
	DEFINE_INPUTFUNC( FIELD_VOID, "OutsideTransition",	InputOutsideTransition ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTransitionTarget", InputSetOutsideTransitionTarget ),

	DEFINE_KEYFIELD( m_flGracePeriod,	FIELD_FLOAT, "GracePeriod" ),
	DEFINE_KEYFIELD( m_flMaxSpeed,		FIELD_FLOAT, "PatrolSpeed" ),
	DEFINE_KEYFIELD( m_bNonCombat,		FIELD_BOOLEAN,	"NonCombat" ),

	DEFINE_FIELD( m_hCrashPoint,		FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ResetIdleTime", InputResetIdleTime ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartAlwaysLeadingVehicle", InputStartAlwaysLeadingVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartBombingVehicle", InputStartBombingVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartTrailingVehicle", InputStartTrailingVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartDefaultBehavior", InputStartDefaultBehavior ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartBullrushBehavior", InputStartBullrushBehavior ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DropBomb", InputDropBomb ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DropBombStraightDown", InputDropBombStraightDown ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DropBombAtTargetAlways", InputDropBombAtTargetAlways ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DropBombAtTarget", InputDropBombAtTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "DropBombDelay", InputDropBombDelay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartCarpetBombing", InputStartCarpetBombing ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopCarpetBombing", InputStopCarpetBombing ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BecomeIndestructible", InputBecomeIndestructible ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableDeadlyShooting", InputEnableDeadlyShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableDeadlyShooting", InputDisableDeadlyShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartNormalShooting", InputStartNormalShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartLongCycleShooting", InputStartLongCycleShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartContinuousShooting", InputStartContinuousShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFastShooting", InputStartFastShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GunOff", InputGunOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetHealthFraction", InputSetHealthFraction ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartBombExplodeOnContact", InputStartBombExplodeOnContact ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopBombExplodeOnContact", InputStopBombExplodeOnContact ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePathVisibilityTests", InputDisablePathVisibilityTests ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePathVisibilityTests", InputEnablePathVisibilityTests ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SelfDestruct", InputSelfDestruct ),

	DEFINE_THINKFUNC( BlinkLightsThink ),
	DEFINE_THINKFUNC( SpotlightThink ),

	DEFINE_OUTPUT( m_OnHealthChanged, "OnHealthChanged" ),
	DEFINE_OUTPUT( m_OnShotDown, "OnShotDown" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
CNPC_AttackHelicopter::CNPC_AttackHelicopter() : 
	m_bNonCombat( false ),
	m_flGracePeriod( 2.0f ),
	m_bBombsExplodeOnContact( false )
{
	m_flMaxSpeed = 0;
}

CNPC_AttackHelicopter::~CNPC_AttackHelicopter(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Shuts down looping sounds when we are killed in combat or deleted.
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::StopLoopingSounds()
{
	BaseClass::StopLoopingSounds();

	if ( m_pGunFiringSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pGunFiringSound );
		m_pGunFiringSound = NULL;
	}
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void Chopper_PrecacheChunks( CBaseEntity *pChopper )
{
	for ( int i = 0; i < CHOPPER_MAX_CHUNKS; ++i )
	{
		pChopper->PrecacheModel( s_pChunkModelName[i] );
	}

	pChopper->PrecacheModel( HELICOPTER_CHUNK_COCKPIT );
	pChopper->PrecacheModel( HELICOPTER_CHUNK_TAIL );
	pChopper->PrecacheModel( HELICOPTER_CHUNK_BODY );
}
 
//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Precache( void )
{
	BaseClass::Precache();

	if ( !HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
	{
		PrecacheModel( CHOPPER_MODEL_NAME );
	}
	else
	{
		PrecacheModel( CHOPPER_DRONE_NAME );
	}

	PrecacheModel( CHOPPER_RED_LIGHT_SPRITE );
	//PrecacheModel( CHOPPER_MODEL_CORPSE_NAME );
	
	// If we're never going to engage in combat, we don't need to load these assets!
	if ( m_bNonCombat == false )
	{
		UTIL_PrecacheOther( "grenade_helicopter" );
		UTIL_PrecacheOther( "env_fire_trail" );
		Chopper_PrecacheChunks( this );
		PrecacheModel("models/combine_soldier.mdl");
	}

	PrecacheScriptSound("NPC_AttackHelicopter.ChargeGun");
	if ( HasSpawnFlags( SF_HELICOPTER_LOUD_ROTOR_SOUND ) )
	{
		PrecacheScriptSound("NPC_AttackHelicopter.RotorsLoud");
	}
	else
	{
		PrecacheScriptSound("NPC_AttackHelicopter.Rotors");
	}
	PrecacheScriptSound( "NPC_AttackHelicopter.DropMine" );
	PrecacheScriptSound( "NPC_AttackHelicopter.BadlyDamagedAlert" );
	PrecacheScriptSound( "NPC_AttackHelicopter.CrashingAlarm1" );
	PrecacheScriptSound( "NPC_AttackHelicopter.MegabombAlert" );

	PrecacheScriptSound( "NPC_AttackHelicopter.RotorBlast" );
	PrecacheScriptSound( "NPC_AttackHelicopter.EngineFailure" );
	PrecacheScriptSound( "NPC_AttackHelicopter.FireGun" );
	PrecacheScriptSound( "NPC_AttackHelicopter.Crash" );
	PrecacheScriptSound( "HelicopterBomb.HardImpact" );

	PrecacheScriptSound( "ReallyLoudSpark" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.Ping" );
}

int CNPC_AttackHelicopter::ObjectCaps() 
{ 
	int caps = BaseClass::ObjectCaps();
	if ( m_bAlwaysTransition )
		caps |= FCAP_NOTIFY_ON_TRANSITION;
	return caps; 
}

void CNPC_AttackHelicopter::InputOutsideTransition( inputdata_t &inputdata )
{
	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, m_iszTransitionTarget );

	if ( pEnt )
	{
		Vector teleportLocation = pEnt->GetAbsOrigin();
		QAngle teleportAngles = pEnt->GetAbsAngles();
		Teleport( &teleportLocation, &teleportAngles, &vec3_origin );
		Teleported();
	}
	else
	{
		DevMsg( 2, "NPC \"%s\" failed to find a suitable transition a point\n", STRING(GetEntityName()) );
	}
}

void CNPC_AttackHelicopter::InputSetOutsideTransitionTarget( inputdata_t &inputdata )
{
	m_iszTransitionTarget = MAKE_STRING( inputdata.value.String() );
}


//-----------------------------------------------------------------------------
// Create components
//-----------------------------------------------------------------------------
bool CNPC_AttackHelicopter::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_Spotlight.Init( this, AI_SPOTLIGHT_NO_DLIGHTS, 45.0f, 500.0f );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose :
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::Spawn( void )
{
	Precache( );

	m_bIndestructible = false;
	m_bDeadlyShooting = false;
	m_bBombingSuppressed = false;
	m_bIgnorePathVisibilityTests = false;

	if ( !HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
	{
		SetModel( CHOPPER_MODEL_NAME );
	}
	else
	{
		SetModel( CHOPPER_DRONE_NAME );
	}

	ExtractBbox( SelectHeaviestSequence( ACT_IDLE ), m_cullBoxMins, m_cullBoxMaxs ); 
	GetEnemies()->SetFreeKnowledgeDuration( DEFAULT_FREE_KNOWLEDGE_DURATION );

	float flLoadedSpeed = m_flMaxSpeed;
	BaseClass::Spawn();

	float flChaseDist = HasSpawnFlags( SF_HELICOPTER_AGGRESSIVE ) ? 
		CHOPPER_MIN_AGGRESSIVE_CHASE_DIST_DIFF : CHOPPER_MIN_CHASE_DIST_DIFF;
	InitPathingData( CHOPPER_ARRIVE_DIST, flChaseDist, CHOPPER_AVOID_DIST );
	SetFarthestPathDist( GetMaxFiringDistance() );

	m_takedamage = DAMAGE_YES;
	m_nGunState = GUN_STATE_IDLE;
	SetHullType( HULL_LARGE_CENTERED );
	
	SetHullSizeNormal();

#ifdef HL2_EPISODIC
	CreateVPhysics();
#endif // HL2_EPISODIC

	SetPauseState( PAUSE_NO_PAUSE );

	m_iMaxHealth = m_iHealth = sk_helicopter_health.GetInt();
	
	m_flMaxSpeed = flLoadedSpeed;
	if ( m_flMaxSpeed <= 0 )
	{
		m_flMaxSpeed = CHOPPER_MAX_SPEED;
	}
	m_flNextMegaBombHealth = m_iMaxHealth - m_iMaxHealth * g_helicopter_bullrush_mega_bomb_health.GetFloat();

	m_nGrenadeCount = CHOPPER_BOMB_DROP_COUNT;

	m_flFieldOfView = -1.0; // 360 degrees
	m_flIdleTimeDelay = 0.0f;
	m_iAmmoType = GetAmmoDef()->Index("HelicopterGun"); 

	InitBoneControllers();

	m_fHelicopterFlags = BITS_HELICOPTER_GUN_ON;
	m_bSuppressSound = false;

	m_flAcrossTime = -1.0f;
	m_flPathOffset = 0.0f;
	m_flCurrPathOffset = 0.0f;
	m_nAttackMode = ATTACK_MODE_DEFAULT;
	m_flInputDropBombTime = gpGlobals->curtime;
	SetActivity( ACT_IDLE );

	int nBombAttachment = LookupAttachment("bomb");
	m_hSensor = static_cast<CBombDropSensor*>(CreateEntityByName( "npc_helicoptersensor" ));
	m_hSensor->Spawn();
	m_hSensor->SetParent( this, nBombAttachment );
	m_hSensor->SetLocalOrigin( vec3_origin );
	m_hSensor->SetLocalAngles( vec3_angle );
	m_hSensor->SetOwnerEntity( this );

	AddFlag( FL_AIMTARGET );

	m_hCrashPoint.Set( NULL );
}

#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_AttackHelicopter::CreateVPhysics( void )
{
	InitBoneFollowers();
	return BaseClass::CreateVPhysics();
}
#endif // HL2_EPISODIC

//------------------------------------------------------------------------------
// Startup the chopper
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Startup()
{
	BaseClass::Startup();

	if ( HasSpawnFlags( SF_HELICOPTER_LIGHTS ) )
	{
		for ( int i = 0; i < MAX_HELICOPTER_LIGHTS; ++i )
		{
			// See if there's an attachment for this smoke trail
			char buf[32];
			Q_snprintf( buf, 32, "Light_Red%d", i );
			int nAttachment = LookupAttachment( buf );
			if ( nAttachment == 0 )
			{
				m_hLights[i] = NULL;
				continue;
			}

			m_hLights[i] = CSprite::SpriteCreate( CHOPPER_RED_LIGHT_SPRITE, vec3_origin, false );
			if ( !m_hLights[i] )
				continue;

			m_hLights[i]->SetParent( this, nAttachment );
			m_hLights[i]->SetLocalOrigin( vec3_origin );
			m_hLights[i]->SetLocalVelocity( vec3_origin );
			m_hLights[i]->SetMoveType( MOVETYPE_NONE );
			m_hLights[i]->SetTransparency( kRenderTransAdd, 255, 255, 255, 200, kRenderFxNone );
			m_hLights[i]->SetScale( 1.0f );
			m_hLights[i]->TurnOn();
		}

		SetContextThink( &CNPC_AttackHelicopter::BlinkLightsThink, gpGlobals->curtime + CHOPPER_LIGHT_BLINK_TIME_SHORT, s_pBlinkLightThinkContext );
	}
}


//------------------------------------------------------------------------------
// Startup the chopper
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::BlinkLightsThink()
{
	bool bIsOn = false;
	for ( int i = 0; i < MAX_HELICOPTER_LIGHTS; ++i )
	{
		if ( !m_hLights[i] )
			continue;

		if ( m_hLights[i]->GetScale() > 0.1f )
		{
			m_hLights[i]->SetScale( 0.1f, CHOPPER_LIGHT_BLINK_TIME_SHORT );
		}
		else
		{
			m_hLights[i]->SetScale( 0.5f, 0.0f );
			bIsOn = true;
		}
	}

	float flTime;
	if ( bIsOn )
	{
		flTime = CHOPPER_LIGHT_BLINK_TIME_SHORT;
	}
	else
	{
		flTime = m_bShortBlink ? CHOPPER_LIGHT_BLINK_TIME_SHORT : CHOPPER_LIGHT_BLINK_TIME;
		m_bShortBlink = !m_bShortBlink;
	}

	SetContextThink( &CNPC_AttackHelicopter::BlinkLightsThink, gpGlobals->curtime + flTime, s_pBlinkLightThinkContext );
}


//------------------------------------------------------------------------------
// Start up spotlights
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::SpotlightStartup()
{
	if ( !HasSpawnFlags( SF_HELICOPTER_LIGHTS ) )
		return;

	Vector vecForward;
	Vector vecOrigin;
	GetAttachment( m_nSpotlightAttachment, vecOrigin, &vecForward );
	m_Spotlight.SpotlightCreate( m_nSpotlightAttachment, vecForward );
	SpotlightThink();
}


//------------------------------------------------------------------------------
// Shutdown spotlights
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::SpotlightShutdown()
{
	m_Spotlight.SpotlightDestroy();
	SetContextThink( NULL, gpGlobals->curtime, s_pSpotlightThinkContext );
}


//------------------------------------------------------------------------------
// Spotlights
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::SpotlightThink()
{
	// NOTE: This function should deal with all deactivation cases
	if ( m_lifeState != LIFE_ALIVE ) 
	{
		SpotlightShutdown();
		return;
	}

	switch( m_nAttackMode )
	{
	case ATTACK_MODE_BULLRUSH_VEHICLE:
		{
			switch ( m_nSecondaryMode )
			{
			case BULLRUSH_MODE_SHOOT_GUN:
				{
					Vector vecForward;
					Vector vecOrigin;
					GetAttachment( m_nSpotlightAttachment, vecOrigin, &vecForward );
					m_Spotlight.SetSpotlightTargetDirection( vecForward );
				}
				break;

			case BULLRUSH_MODE_SHOOT_IDLE_PLAYER:
				if ( GetEnemy() )
				{
					m_Spotlight.SetSpotlightTargetPos( GetEnemy()->WorldSpaceCenter() );
				}
				break;

			default:
				SpotlightShutdown();
				return;
			}
		}
		break;

	default:
		SpotlightShutdown();
		return;
	}

	m_Spotlight.Update();
	SetContextThink( &CNPC_AttackHelicopter::SpotlightThink, gpGlobals->curtime + TICK_INTERVAL, s_pSpotlightThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: Always transition along with the player
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputEnableAlwaysTransition( inputdata_t &inputdata )
{
	m_bAlwaysTransition = true;
}

//-----------------------------------------------------------------------------
// Purpose: Stop always transitioning along with the player
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDisableAlwaysTransition( inputdata_t &inputdata )
{
	m_bAlwaysTransition = false;
}

//------------------------------------------------------------------------------
// On Remove
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
	StopLoopingSounds();
	UTIL_Remove(m_hSensor);
	DestroySmokeTrails();
	for ( int i = 0; i < MAX_HELICOPTER_LIGHTS; ++i )
	{
		if ( m_hLights[i] )
		{
			UTIL_Remove( m_hLights[i] );
			m_hLights[i] = NULL;
		}
	}

#ifdef HL2_EPISODIC
	m_BoneFollowerManager.DestroyBoneFollowers();
#endif // HL2_EPISODIC
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Activate( void )
{
	BaseClass::Activate();
	m_nGunBaseAttachment = LookupAttachment("gun");
	m_nGunTipAttachment = LookupAttachment("muzzle");
	m_nBombAttachment = LookupAttachment("bomb");
	m_nSpotlightAttachment = LookupAttachment("spotlight");

	if ( HasSpawnFlags( SF_HELICOPTER_LONG_SHADOW ) )
	{
		SetShadowCastDistance( 2048 );
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNPC_AttackHelicopter::GetTracerType( void ) 
{
	return "HelicopterTracer"; 
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::DoImpactEffect( trace_t &tr, int nDamageType )
{
	UTIL_ImpactTrace( &tr, nDamageType, "HelicopterImpact" );
} 


//------------------------------------------------------------------------------
// Purpose : Create our rotor sound
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InitializeRotorSound( void )
{
	if ( !m_pRotorSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );

		if ( HasSpawnFlags( SF_HELICOPTER_LOUD_ROTOR_SOUND ) )
		{
			m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.RotorsLoud" );
		}
		else
		{
			m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.Rotors" );
		}

		m_pRotorBlast = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.RotorBlast" );
		m_pGunFiringSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.FireGun" );
		controller.Play( m_pGunFiringSound, 0.0, 100 );
	}
	else
	{
		Assert(m_pRotorSound);
		Assert(m_pRotorBlast);
		Assert(m_pGunFiringSound);
	}


	BaseClass::InitializeRotorSound();
}


//------------------------------------------------------------------------------
// Gets the max speed of the helicopter
//------------------------------------------------------------------------------
float CNPC_AttackHelicopter::GetMaxSpeed()
{
	if ( HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
		return DRONE_SPEED;

	if ( ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && IsInSecondaryMode( BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED ) )
		return CHOPPER_BULLRUSH_ENEMY_BOMB_SPEED;

	if ( !GetEnemyVehicle() )
		return BaseClass::GetMaxSpeed();

	return 3000.0f;
}

float CNPC_AttackHelicopter::GetMaxSpeedFiring()
{
	if ( HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
		return DRONE_SPEED;

	if ( ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && IsInSecondaryMode( BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED ) )
		return CHOPPER_BULLRUSH_ENEMY_BOMB_SPEED;

	if ( !GetEnemyVehicle() )
		return BaseClass::GetMaxSpeedFiring();

	return 3000.0f;
}
  

//------------------------------------------------------------------------------
// Returns the max firing distance
//------------------------------------------------------------------------------
float CNPC_AttackHelicopter::GetMaxFiringDistance()
{
	if ( !GetEnemyVehicle() )
		return CHOPPER_GUN_MAX_FIRING_DIST;

	return 8000.0f;
}


//------------------------------------------------------------------------------
// Updates the enemy
//------------------------------------------------------------------------------
float CNPC_AttackHelicopter::EnemySearchDistance( )
{
	return 6000.0f;
}


//------------------------------------------------------------------------------
// Leading behaviors
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputStartBombingVehicle( inputdata_t &inputdata )
{
	m_nAttackMode = ATTACK_MODE_BOMB_VEHICLE;
	SetLeadingDistance( 1500.0f );
}

void CNPC_AttackHelicopter::InputStartTrailingVehicle( inputdata_t &inputdata )
{
	m_nAttackMode = ATTACK_MODE_TRAIL_VEHICLE;
	SetLeadingDistance( -1500.0f );
}

void CNPC_AttackHelicopter::InputStartDefaultBehavior( inputdata_t &inputdata )
{
	m_nAttackMode = ATTACK_MODE_DEFAULT;
}

void CNPC_AttackHelicopter::InputStartAlwaysLeadingVehicle( inputdata_t &inputdata )
{
	m_nAttackMode = ATTACK_MODE_ALWAYS_LEAD_VEHICLE;
	SetLeadingDistance( 0.0f );
}

void CNPC_AttackHelicopter::InputStartBullrushBehavior( inputdata_t &inputdata )
{
	if ( m_nAttackMode != ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		m_nAttackMode = ATTACK_MODE_BULLRUSH_VEHICLE;
		SetSecondaryMode( BULLRUSH_MODE_WAIT_FOR_ENEMY );
		SetLeadingDistance( 0.0f );
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputStartCarpetBombing( inputdata_t &inputdata )
{
	m_bIsCarpetBombing = true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputStopCarpetBombing( inputdata_t &inputdata )
{
	m_bIsCarpetBombing = false;
}

//------------------------------------------------------------------------------
// Become indestructible
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputBecomeIndestructible( inputdata_t &inputdata )
{
	m_bIndestructible = true;
}


//------------------------------------------------------------------------------
// Deadly shooting, tex!
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputEnableDeadlyShooting( inputdata_t &inputdata )
{
	m_bDeadlyShooting = true;
}

void CNPC_AttackHelicopter::InputDisableDeadlyShooting( inputdata_t &inputdata )
{
	m_bDeadlyShooting = false;
}

void CNPC_AttackHelicopter::InputStartNormalShooting( inputdata_t &inputdata )
{
	m_nShootingMode = SHOOT_MODE_DEFAULT;
}

void CNPC_AttackHelicopter::InputStartLongCycleShooting( inputdata_t &inputdata )
{
	m_nShootingMode = SHOOT_MODE_LONG_CYCLE;
}

void CNPC_AttackHelicopter::InputStartContinuousShooting( inputdata_t &inputdata )
{
	m_nShootingMode = SHOOT_MODE_CONTINUOUS;
}

void CNPC_AttackHelicopter::InputStartFastShooting( inputdata_t &inputdata )
{
	m_nShootingMode = SHOOT_MODE_FAST;
}

//------------------------------------------------------------------------------
// Deadly shooting, tex!
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::IsDeadlyShooting()
{
	if ( m_bDeadlyShooting )
		return true;

	if (( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && IsInSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER ) )
	{
		return (!GetEnemyVehicle()) && GetEnemy() && GetEnemy()->IsPlayer();
	}

	return false;
}

int CNPC_AttackHelicopter::GetShootingMode( )
{
	if ( IsDeadlyShooting() )
		return SHOOT_MODE_LONG_CYCLE;

	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
		return SHOOT_MODE_CONTINUOUS;

	return m_nShootingMode;
}


//-----------------------------------------------------------------------------
// Bombing suppression
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputEnableBombing( inputdata_t &inputdata )
{
	m_bBombingSuppressed = false;
}

void CNPC_AttackHelicopter::InputDisableBombing( inputdata_t &inputdata )
{
	m_bBombingSuppressed = true;
}
	

//-----------------------------------------------------------------------------
// Visibility tests
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDisablePathVisibilityTests( inputdata_t &inputdata )
{
	m_bIgnorePathVisibilityTests = true;
	GetEnemies()->SetUnforgettable( GetEnemy(), true );
}

void CNPC_AttackHelicopter::InputEnablePathVisibilityTests( inputdata_t &inputdata )
{
	m_bIgnorePathVisibilityTests = false;
	GetEnemies()->SetUnforgettable( GetEnemy(), false );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputSelfDestruct( inputdata_t &inputdata )
{
	m_lifeState = LIFE_ALIVE; // Force to die properly.
	CTakeDamageInfo info( this, this, Vector(0, 0, 1), WorldSpaceCenter(), GetMaxHealth(), CLASS_MISSILE );
	TakeDamage( info );
}

//-----------------------------------------------------------------------------
// For scripted times where it *has* to shoot
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputSetHealthFraction( inputdata_t &inputdata )
{
	// Sets the health fraction, no damage effects
	if ( inputdata.value.Float() > 0 )
	{
		SetHealth( GetMaxHealth() * inputdata.value.Float() * 0.01f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputStartBombExplodeOnContact( inputdata_t &inputdata )
{
	m_bBombsExplodeOnContact = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputStopBombExplodeOnContact( inputdata_t &inputdata )
{
	m_bBombsExplodeOnContact = false;
}
	
//------------------------------------------------------------------------------
// For scripted times where it *has* to shoot
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputResetIdleTime( inputdata_t &inputdata )
{
	if ( m_nGunState == GUN_STATE_IDLE )
	{
		m_flNextAttack = gpGlobals->curtime;
	}
}


//-----------------------------------------------------------------------------
// This trace filter ignores all breakables + physics props 
//-----------------------------------------------------------------------------
class CTraceFilterChopper : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterChopper, CTraceFilterSimple );

public:
	CTraceFilterChopper( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
};

CTraceFilterChopper::CTraceFilterChopper( const IHandleEntity *passentity, int collisionGroup ) :
	CTraceFilterSimple( passentity, collisionGroup )
{
}

bool CTraceFilterChopper::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEnt = static_cast<IServerUnknown*>(pServerEntity)->GetBaseEntity();
	if ( pEnt )
	{
		if ( FClassnameIs( pEnt, "func_breakable" ) || 
			 FClassnameIs( pEnt, "func_physbox" ) || 
			 FClassnameIs( pEnt, "prop_physics" ) || 
			 FClassnameIs( pEnt, "physics_prop" ) )
		{
			return false;
		}
	}

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Enemy visibility check
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_AttackHelicopter::FindTrackBlocker( const Vector &vecViewPoint, const Vector &vecTargetPos )
{
	if ( m_bIgnorePathVisibilityTests )
		return NULL;

	CTraceFilterChopper chopperFilter( this, COLLISION_GROUP_NONE );

	trace_t	tr;
	AI_TraceHull( vecViewPoint, vecTargetPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT, &chopperFilter, &tr );

	if ( tr.fraction != 1.0f )
	{
		Assert( tr.m_pEnt );
	}

	return (tr.fraction != 1.0f) ? tr.m_pEnt : NULL;
}


//-----------------------------------------------------------------------------
// More Enemy visibility check
//-----------------------------------------------------------------------------
bool CNPC_AttackHelicopter::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

#if 0
	// FIXME: only block LOS through opaque water
	// don't look through water
	if ((m_nWaterLevel != 3 && pEntity->m_nWaterLevel == 3) 
		|| (m_nWaterLevel == 3 && pEntity->m_nWaterLevel == 0))
		return false;
#endif

	Vector vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	Vector vecTargetOrigin = pEntity->EyePosition();

	CTraceFilterChopper chopperFilter( this, COLLISION_GROUP_NONE );

	trace_t tr;
	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, traceMask, &chopperFilter, &tr);
	
	if (tr.fraction != 1.0)
	{
		// Got line of sight!
		if ( tr.m_pEnt == pEntity )
			return true;

		// Got line of sight on the vehicle the player is driving!
		if ( pEntity && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( pEntity );
			if ( tr.m_pEnt == pPlayer->GetVehicleEntity() )
				return true;
		}

		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}
		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}


//------------------------------------------------------------------------------
// Shot spread
//------------------------------------------------------------------------------
#define PLAYER_TIGHTEN_FACTOR 0.75f
Vector CNPC_AttackHelicopter::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	float flSinConeDegrees = sin( sk_helicopter_firingcone.GetFloat() * PLAYER_TIGHTEN_FACTOR * 0.5f * (3.14f / 180.0f) );
	Vector vecSpread( flSinConeDegrees, flSinConeDegrees, flSinConeDegrees );
	return vecSpread;
}


//------------------------------------------------------------------------------
// Find interesting nearby things to shoot
//------------------------------------------------------------------------------
int CNPC_AttackHelicopter::BuildMissTargetList( int nCount, CBaseEntity **ppMissCandidates )
{
	int numMissCandidates = 0;

	CBaseEntity *pEnts[256];
	Vector radius( 150, 150, 150 );
	const Vector &vecSource = GetEnemy()->WorldSpaceCenter();

	int numEnts = UTIL_EntitiesInBox( pEnts, 256, vecSource - radius, vecSource+radius, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if ( pEnts[i] == NULL )
			continue;

		if ( numMissCandidates >= nCount )
			break;

		// Miss candidates cannot include the player or his vehicle
		if ( pEnts[i] == GetEnemyVehicle() || pEnts[i] == GetEnemy() )
			continue;

		// See if it's a good target candidate
		if ( FClassnameIs( pEnts[i], "prop_dynamic" ) || 
			 FClassnameIs( pEnts[i], "prop_physics" ) || 
			 FClassnameIs( pEnts[i], "physics_prop" ) )
		{
			ppMissCandidates[numMissCandidates++] = pEnts[i];
		}
	}

	return numMissCandidates;
}


//------------------------------------------------------------------------------
// Gets a vehicle the enemy is in (if any)
//------------------------------------------------------------------------------
CBaseEntity *CNPC_AttackHelicopter::GetEnemyVehicle()
{
	if ( !GetEnemy() )
		return NULL;

	if ( !GetEnemy()->IsPlayer() )
		return NULL;

	return static_cast<CBasePlayer*>(GetEnemy())->GetVehicleEntity();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ShootAtPlayer( const Vector &vBasePos, const Vector &vGunDir )
{
	// Fire one shots per round right at the player, using usual rules
	FireBulletsInfo_t info;
	info.m_vecSrc = vBasePos;
	info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
	info.m_flDistance = MAX_COORD_RANGE;
	info.m_iAmmoType = m_iAmmoType;
	info.m_iTracerFreq = 1;
	info.m_vecDirShooting = GetActualShootTrajectory( vBasePos );
	info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;

	DoMuzzleFlash();

	QAngle	vGunAng;
	VectorAngles( vGunDir, vGunAng );
	
	FireBullets( info );

	// Fire the rest of the bullets at objects around the player
	CBaseEntity *ppNearbyTargets[16];
	int nActualTargets = BuildMissTargetList( 16, ppNearbyTargets ); 

	// Randomly sort it...
	int i;
	for ( i = 0; i < nActualTargets; ++i )
	{
		int nSwap = random->RandomInt( 0, nActualTargets - 1 ); 
		V_swap( ppNearbyTargets[i], ppNearbyTargets[nSwap] );
	}

	// Just shoot where we're facing
	float flSinConeDegrees = sin( sk_helicopter_firingcone.GetFloat() * 0.5f * (3.14f / 180.0f) );
	Vector vecSpread( flSinConeDegrees, flSinConeDegrees, flSinConeDegrees );

	// How many times should we hit the player this time?
	int nDesiredHitCount = (int)(((float)( m_nMaxBurstHits - m_nBurstHits ) / (float)m_nRemainingBursts) + 0.5f);
	int nNearbyTargetCount = 0;
	int nPlayerShotCount = 0;
	for ( i = sk_helicopter_roundsperburst.GetInt() - 1; --i >= 0; )
	{
		// Find something interesting around the enemy to shoot instead of just missing.
		if ( nActualTargets > nNearbyTargetCount )
		{
			// FIXME: Constrain to the firing cone?
			ppNearbyTargets[nNearbyTargetCount]->CollisionProp()->RandomPointInBounds( Vector(.25, .25, .25), Vector(.75, .75, .75), &info.m_vecDirShooting );
			info.m_vecDirShooting -= vBasePos;
			VectorNormalize( info.m_vecDirShooting );
			info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
			info.m_flDistance = MAX_COORD_RANGE;
			info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;
			
			FireBullets( info );

			++nNearbyTargetCount;
			continue;
		}

		if ( GetEnemy() && ( nPlayerShotCount < nDesiredHitCount ))
		{
			GetEnemy()->CollisionProp()->RandomPointInBounds( Vector(0, 0, 0), Vector(1, 1, 1), &info.m_vecDirShooting );
			info.m_vecDirShooting -= vBasePos;
			VectorNormalize( info.m_vecDirShooting );
			info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
			info.m_flDistance = MAX_COORD_RANGE;
			info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;
			FireBullets( info );
			++nPlayerShotCount;
			continue;
		}

		// Nothing nearby; just fire randomly...
		info.m_vecDirShooting = vGunDir;
		info.m_vecSpread = vecSpread;
		info.m_flDistance = 8192;
		info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;

		FireBullets( info );
	}
}


//-----------------------------------------------------------------------------
// Chooses a point within the circle of death to fire in
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::PickDirectionToCircleOfDeath( const Vector &vBasePos, const Vector &vecFireAtPosition, Vector *pResult )
{
	*pResult = vecFireAtPosition;
	float x, y;
	do
	{
		x = random->RandomFloat( -1.0f, 1.0f ); 
		y = random->RandomFloat( -1.0f, 1.0f ); 
	} while ( (x * x + y * y) > 1.0f );

	pResult->x += x * m_flCircleOfDeathRadius; 
	pResult->y += y * m_flCircleOfDeathRadius; 

	*pResult -= vBasePos;
	VectorNormalize( *pResult );
}


//-----------------------------------------------------------------------------
// Deliberately aims as close as possible w/o hitting
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::AimCloseToTargetButMiss( CBaseEntity *pTarget, float flMinDist, float flMaxDist, const Vector &shootOrigin, Vector *pResult )
{
	Vector vecDirection;
	VectorSubtract( pTarget->WorldSpaceCenter(), shootOrigin, vecDirection );
	float flDist = VectorNormalize( vecDirection );
	float flRadius = pTarget->BoundingRadius() + random->RandomFloat( flMinDist, flMaxDist );

	float flMinRadius = flRadius;
	if ( flDist > flRadius )
	{
		flMinRadius = flDist * flRadius / sqrt( flDist * flDist - flRadius * flRadius );
	}

	// Choose random points in a plane perpendicular to the shoot origin.
	Vector vecRandomDir;
	vecRandomDir.Random( -1.0f, 1.0f );
	VectorMA( vecRandomDir, -DotProduct( vecDirection, vecRandomDir ), vecDirection, vecRandomDir );
	VectorNormalize( vecRandomDir );
	vecRandomDir *= flMinRadius;
	vecRandomDir += pTarget->WorldSpaceCenter();

	VectorSubtract( vecRandomDir, shootOrigin, *pResult );
	VectorNormalize( *pResult );
}


//-----------------------------------------------------------------------------
// Make sure we don't hit too many times
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::FireBullets( const FireBulletsInfo_t &info )
{
	// Use this to count the number of hits in a burst
	bool bIsPlayer = GetEnemy() && GetEnemy()->IsPlayer();
	if ( !bIsPlayer )
	{
		BaseClass::FireBullets( info );
		return;
	}

	if ( !GetEnemyVehicle() && !IsDeadlyShooting() )
	{
		if ( m_nBurstHits >= m_nMaxBurstHits )
		{
			FireBulletsInfo_t actualInfo = info;
			actualInfo.m_pAdditionalIgnoreEnt = GetEnemy();
			BaseClass::FireBullets( actualInfo );
			return;
		}
	}

	CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(GetEnemy());

	int nPrevHealth = pPlayer->GetHealth();
	int nPrevArmor = pPlayer->ArmorValue();

	BaseClass::FireBullets( info );

	if (( pPlayer->GetHealth() < nPrevHealth ) || ( pPlayer->ArmorValue() < nPrevArmor ))
	{
		++m_nBurstHits;
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ShootInsideCircleOfDeath( const Vector &vBasePos, const Vector &vecFireAtPosition )
{
	Vector vecFireDirection;
	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		PickDirectionToCircleOfDeath( vBasePos, vecFireAtPosition, &vecFireDirection );
	}
	else if ( ( m_nNearShots < m_nMaxNearShots ) || !GetEnemyVehicle() )
	{
		if ( ( m_nBurstHits < m_nMaxBurstHits ) || !GetEnemy() )
		{
			++m_nNearShots;
			PickDirectionToCircleOfDeath( vBasePos, vecFireAtPosition, &vecFireDirection );
		}
		else
		{
			m_nNearShots += 6;
			AimCloseToTargetButMiss( GetEnemy(), 20.0f, 50.0f, vBasePos, &vecFireDirection );
		}
	}
	else
	{
		AimCloseToTargetButMiss( GetEnemyVehicle(), 10.0f, 80.0f, vBasePos, &vecFireDirection );
	}

	FireBulletsInfo_t info( 1, vBasePos, vecFireDirection, VECTOR_CONE_PRECALCULATED, MAX_COORD_RANGE, m_iAmmoType );
	info.m_iTracerFreq = 1;
	info.m_nFlags = FIRE_BULLETS_TEMPORARY_DANGER_SOUND;

	FireBullets( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "ChopperMuzzleFlash", data );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
#define HIT_VEHICLE_SPEED_MIN 200.0f
#define HIT_VEHICLE_SPEED_MAX 500.0f

void CNPC_AttackHelicopter::ShootAtVehicle( const Vector &vBasePos, const Vector &vecFireAtPosition )
{
	int nShotsRemaining = sk_helicopter_roundsperburst.GetInt();

	DoMuzzleFlash();

	// Do special code against episodic drivers
	if ( hl2_episodic.GetBool() )
	{
		Vector vecVelocity;
		GetEnemyVehicle()->GetVelocity( &vecVelocity, NULL );
		
		float flSpeed = clamp( vecVelocity.Length(), 0.0f, 400.0f );
		float flRange = RemapVal( flSpeed, 0.0f, 400.0f, 0.05f, 1.0f );

		// Alter each shot's trajectory based on our speed
		for ( int i = 0; i < nShotsRemaining; i++ )
		{
			Vector vecShotDir;
			
			// If they're at a dead stand-still, just hit them
			if ( flRange <= 0.1f )
			{
				VectorSubtract( GetEnemy()->EyePosition(), vBasePos, vecShotDir );

				Vector vecOffset;
				vecOffset.Random( -40.0f, 40.0f );
				vecShotDir += vecOffset;
				VectorNormalize( vecShotDir );
			}
			else
			{
				// Aim in a cone around them
				AimCloseToTargetButMiss( GetEnemy(), (3*12) * flRange, (10*12) * flRange, vBasePos, &vecShotDir );
			}
			
			FireBulletsInfo_t info( 1, vBasePos, vecShotDir, VECTOR_CONE_PRECALCULATED, MAX_COORD_RANGE, m_iAmmoType );
			info.m_iTracerFreq = 1;
			FireBullets( info );
		}

		// We opt out of the rest of the function
		// FIXME: Should we emulate the below functionality and have half the bullets attempt to miss admirably? -- jdw
		return;
	}

	// Pop one at the player based on how fast he's going
	if ( m_nBurstHits < m_nMaxBurstHits )
	{
		Vector vecDir;						   
		VectorSubtract( GetEnemy()->EyePosition(), vBasePos, vecDir );
		
		Vector vecOffset;
		vecOffset.Random( -5.0f, 5.0f );
		vecDir += vecOffset;
		VectorNormalize( vecDir );

		FireBulletsInfo_t info( 1, vBasePos, vecDir, VECTOR_CONE_PRECALCULATED, MAX_COORD_RANGE, m_iAmmoType );
		info.m_iTracerFreq = 1;
		FireBullets( info );
		--nShotsRemaining;
	}

	// Fire half of the bullets within the circle of death, the other half at interesting things
	int i;
	int nFireInCircle = nShotsRemaining >> 1;
	nShotsRemaining -= nFireInCircle;
	for ( i = 0; i < nFireInCircle; ++i )
	{
		ShootInsideCircleOfDeath( vBasePos, vecFireAtPosition );
	}

	// Fire the rest of the bullets at objects around the enemy
	CBaseEntity *ppNearbyTargets[16];
	int nActualTargets = BuildMissTargetList( 16, ppNearbyTargets ); 

	// Randomly sort it...
	for ( i = 0; i < nActualTargets; ++i )
	{
		int nSwap = random->RandomInt( 0, nActualTargets - 1 ); 
		V_swap( ppNearbyTargets[i], ppNearbyTargets[nSwap] );
	}

	// Just shoot where we're facing
	float flSinConeDegrees = sin( sk_helicopter_firingcone.GetFloat() * 0.5f * (3.14f / 180.0f) );
	Vector vecSpread( flSinConeDegrees, flSinConeDegrees, flSinConeDegrees );

	for ( i = nShotsRemaining; --i >= 0; )
	{
		// Find something interesting around the enemy to shoot instead of just missing.
		if ( nActualTargets > i )
		{
			Vector vecFireDirection;
			ppNearbyTargets[i]->CollisionProp()->RandomPointInBounds( Vector(.25, .25, .25), Vector(.75, .75, .75), &vecFireDirection );
			vecFireDirection -= vBasePos;
			VectorNormalize( vecFireDirection );

			// FIXME: Constrain to the firing cone?

			// I put in all the default arguments simply so I could guarantee the first shot of one of the bursts always hits
			FireBulletsInfo_t info( 1, vBasePos, vecFireDirection, VECTOR_CONE_PRECALCULATED, MAX_COORD_RANGE, m_iAmmoType );
			info.m_iTracerFreq = 1;
			FireBullets( info );
		}
		else
		{
			ShootInsideCircleOfDeath( vBasePos, vecFireAtPosition );
		}
	}
}


//------------------------------------------------------------------------------
// Various states of the helicopter firing...
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::PoseGunTowardTargetDirection( const Vector &vTargetDir )
{
	Vector vecOut;
	VectorIRotate( vTargetDir, EntityToWorldTransform(), vecOut );

	QAngle angles;
	VectorAngles(vecOut, angles);

	if (angles.y > 180)
	{
		angles.y = angles.y - 360;
	}
	else if (angles.y < -180)
	{
		angles.y = angles.y + 360;
	}
	if (angles.x > 180)
	{
		angles.x = angles.x - 360;
	}
	else if (angles.x < -180)
	{
		angles.x = angles.x + 360;
	}

	if ( ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && !IsInSecondaryMode(BULLRUSH_MODE_SHOOT_IDLE_PLAYER) && GetEnemy())
	{
		if ( GetEnemyVehicle() )
		{
			angles.x = clamp( angles.x, -12.0f, 0.0f );
			angles.y = clamp( angles.y, -10.0f, 10.0f );
		}
		else
		{
			angles.x = clamp( angles.x, -10.0f, 10.0f );
			angles.y = clamp( angles.y, -10.0f, 10.0f );
		}
	}

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


//------------------------------------------------------------------------------
// Compute the enemy position (non-vehicle case)
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ComputeFireAtPosition( Vector *pVecActualTargetPosition )
{
	// Deal with various leading behaviors...
	*pVecActualTargetPosition = m_vecTargetPosition;
}


//------------------------------------------------------------------------------
// Compute the enemy position (non-vehicle case)
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ComputeVehicleFireAtPosition( Vector *pVecActualTargetPosition )
{
	CBaseEntity *pVehicle = GetEnemyVehicle();

	// Make sure the circle of death doesn't move more than N units
	// This will cause the target to have to maintain a large enough speed
	*pVecActualTargetPosition = pVehicle->BodyTarget( GetAbsOrigin(), false );

//	NDebugOverlay::Box( *pVecActualTargetPosition,
//		Vector(-m_flCircleOfDeathRadius, -m_flCircleOfDeathRadius, 0), 
//		Vector(m_flCircleOfDeathRadius, m_flCircleOfDeathRadius, 0), 
//		0, 0, 255, false, 0.1f );
}

	
//------------------------------------------------------------------------------
// Here's what we do when we're looking for a target
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::DoGunIdle( const Vector &vGunDir, const Vector &vTargetDir )
{
	// When bullrushing, skip the idle
	if ( ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && 
		( IsInSecondaryMode( BULLRUSH_MODE_SHOOT_GUN ) || IsInSecondaryMode(BULLRUSH_MODE_SHOOT_IDLE_PLAYER) ) )
	{
		EmitSound( "NPC_AttackHelicopter.ChargeGun" );
		m_flChargeTime = gpGlobals->curtime + CHOPPER_GUN_CHARGE_TIME;
		m_nGunState = GUN_STATE_CHARGING;
		m_flCircleOfDeathRadius = CHOPPER_MAX_CIRCLE_OF_DEATH_RADIUS;
		return true;
	}

	// Can't continually fire....
	if (m_flNextAttack > gpGlobals->curtime)
		return false;

	// Don't fire if we're too far away, or if the enemy isn't in front of us
	if (!GetEnemy())
		return false;

	float flMaxDistSqr = GetMaxFiringDistance();
	flMaxDistSqr *= flMaxDistSqr;

	float flDistSqr = WorldSpaceCenter().DistToSqr( GetEnemy()->WorldSpaceCenter() );
	if (flDistSqr > flMaxDistSqr)
		return false;

	// If he's mostly within the cone, shoot away!
	float flChargeCone = sk_helicopter_firingcone.GetFloat() * 0.5f;
	if ( flChargeCone < 15.0f )
	{
		flChargeCone = 15.0f;	
	}

	float flCosConeDegrees = cos( flChargeCone * (3.14f / 180.0f) );
	float fDotPr = DotProduct( vGunDir, vTargetDir );
	if (fDotPr < flCosConeDegrees)
		return false;

	// Fast shooting doesn't charge up
	if( m_nShootingMode == SHOOT_MODE_FAST )
	{
		m_flChargeTime = gpGlobals->curtime;
		m_nGunState = GUN_STATE_CHARGING;
		m_flAvoidMetric = 0.0f;
		m_vecLastAngVelocity.Init( 0, 0, 0 );
	}
	else
	{
		EmitSound( "NPC_AttackHelicopter.ChargeGun" );
		float flChargeTime = CHOPPER_GUN_CHARGE_TIME;
		float flVariance = flChargeTime * 0.1f;
		m_flChargeTime = gpGlobals->curtime + random->RandomFloat(flChargeTime - flVariance, flChargeTime + flVariance);
		m_nGunState = GUN_STATE_CHARGING;
		m_flAvoidMetric = 0.0f;
		m_vecLastAngVelocity.Init( 0, 0, 0 );
	}

	return true;	
}


//------------------------------------------------------------------------------
// How easy is the target to hit?
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::UpdateTargetHittability()
{
	// This simply is a measure of how much juking is going on.
	// Along with how much steering is happening.
	if ( GetEnemyVehicle() )
	{
		Vector vecVelocity;
		AngularImpulse vecAngVelocity;
		GetEnemyVehicle()->GetVelocity( &vecVelocity, &vecAngVelocity );

		float flDist = fabs( vecAngVelocity.z - m_vecLastAngVelocity.z );
		m_flAvoidMetric += flDist;
		m_vecLastAngVelocity = vecAngVelocity;
	}
}


//------------------------------------------------------------------------------
// Here's what we do when we're getting ready to fire
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::DoGunCharging( )
{
	// Update the target hittability, which will indicate how many hits we'll accept.
	UpdateTargetHittability();

	if ( m_flChargeTime > gpGlobals->curtime )
		return false;

	m_nGunState = GUN_STATE_FIRING;

	if ( HasSpawnFlags( SF_HELICOPTER_AGGRESSIVE ) )
	{
		SetPauseState( PAUSE_AT_NEXT_LOS_POSITION );
	}

	int nHitFactor = 1;
	switch( GetShootingMode() )
	{
	case SHOOT_MODE_DEFAULT:
	case SHOOT_MODE_FAST:
		{
			int nBurstCount = sk_helicopter_burstcount.GetInt();
			m_nRemainingBursts = random->RandomInt( nBurstCount, 2.0 * nBurstCount );
			m_flIdleTimeDelay = 0.1f * ( m_nRemainingBursts - nBurstCount );
		}
		break;

	case SHOOT_MODE_LONG_CYCLE:
		{
			m_nRemainingBursts = 60;
			m_flIdleTimeDelay = 0.0f;
			nHitFactor = 2;
		}
		break;

	case SHOOT_MODE_CONTINUOUS:
		if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
		{
			// We're relying on the special aiming behavior for bullrushing to just randomly deal damage
			m_nRemainingBursts = 1;	
			m_flIdleTimeDelay = 0.0f;
		}
		else
		{
			m_nRemainingBursts = 0;	
			m_flIdleTimeDelay = 0.0f;
			nHitFactor = 1000;
		}
		break;
	}

	if ( !GetEnemyVehicle() )
	{
		m_nMaxBurstHits = !IsDeadlyShooting() ? random->RandomInt( 6, 9 ) : 200;
		m_nMaxNearShots = 10000;
	}
	else
	{
		Vector vecVelocity;
		GetEnemyVehicle()->GetVelocity( &vecVelocity, NULL );
		float flSpeed = vecVelocity.Length();
		flSpeed = clamp( flSpeed, 150.0f, 600.0f );
		flSpeed = RemapVal( flSpeed, 150.0f, 600.0f, 0.0f, 1.0f );
		float flAvoid = clamp( m_flAvoidMetric, 100.0f, 400.0f );
		flAvoid = RemapVal( flAvoid, 100.0f, 400.0f, 0.0f, 1.0f );

		float flTotal = 0.5f * ( flSpeed + flAvoid );
		int nHitCount = (int)(RemapVal( flTotal, 0.0f, 1.0f, 7, -0.5 ) + 0.5f);

		int nMin = nHitCount >= 1 ? nHitCount - 1 : 0;
		m_nMaxBurstHits = random->RandomInt( nMin, nHitCount + 1 );

		int nNearShots = (int)(RemapVal( flTotal, 0.0f, 1.0f, 70, 5 ) + 0.5f);
		int nMinNearShots = nNearShots >= 5 ? nNearShots - 5 : 0;
		m_nMaxNearShots = random->RandomInt( nMinNearShots, nNearShots + 5 );

		// Set up the circle of death parameters at this point
		m_flCircleOfDeathRadius = SimpleSplineRemapVal( flTotal, 0.0f, 1.0f, 
			CHOPPER_MIN_CIRCLE_OF_DEATH_RADIUS, CHOPPER_MAX_CIRCLE_OF_DEATH_RADIUS );
	}

	m_nMaxBurstHits *= nHitFactor;
	m_nMaxNearShots *= nHitFactor;

	m_nBurstHits = 0;
	m_nNearShots = 0;
	return true;
}


//------------------------------------------------------------------------------
// Shoot where we're facing
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ShootAtFacingDirection( const Vector &vBasePos, const Vector &vGunDir, bool bFirstShotAccurate )
{
	// Just shoot where we're facing
	float flSinConeDegrees = sin( sk_helicopter_firingcone.GetFloat() * 0.5f * (3.14f / 180.0f) );
	Vector vecSpread( flSinConeDegrees, flSinConeDegrees, flSinConeDegrees );

	int nShotCount = sk_helicopter_roundsperburst.GetInt();
	if ( bFirstShotAccurate && GetEnemy() )
	{
		// Check to see if the enemy is within his firing cone
		if ( GetEnemy() )
		{
			// Find the closest point to the gunDir
			const Vector &vecCenter = GetEnemy()->WorldSpaceCenter();

			float t;
			Vector vNearPoint;
			Vector vEndPoint;
			VectorMA( vBasePos, 1024.0f, vGunDir, vEndPoint );
			CalcClosestPointOnLine( vecCenter, vBasePos, vEndPoint, vNearPoint, &t );
			if ( t > 0.0f )
			{
				Vector vecDelta;
				VectorSubtract( vecCenter, vBasePos, vecDelta );
				float flDist = VectorNormalize( vecDelta );
				float flPerpDist = vecCenter.DistTo( vNearPoint );
				float flSinAngle = flPerpDist / flDist;
				if ( flSinAngle <= flSinConeDegrees )
				{
					FireBulletsInfo_t info( 1, vBasePos, vecDelta, VECTOR_CONE_PRECALCULATED, 8192, m_iAmmoType );
					info.m_iTracerFreq = 1;
					FireBullets( info );
					--nShotCount;
				}
			}
		}
	}

#ifdef HL2_EPISODIC 
	if( GetEnemy() != NULL )
	{
		CSoundEnt::InsertSound( SOUND_DANGER, GetEnemy()->WorldSpaceCenter(), 180.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
	}
#endif//HL2_EPISODIC

	DoMuzzleFlash();

	FireBulletsInfo_t info( nShotCount, vBasePos, vGunDir, vecSpread, 8192, m_iAmmoType );
	info.m_iTracerFreq = 1;
	FireBullets( info );
}


//-----------------------------------------------------------------------------
// Can we zap it?
//-----------------------------------------------------------------------------
bool CNPC_AttackHelicopter::IsValidZapTarget( CBaseEntity *pTarget )
{
	// Don't use the player or vehicle as a zap target, we'll do that ourselves.
	if ( pTarget->IsPlayer() || pTarget->GetServerVehicle() )
		return false;

	if ( pTarget == this )
		return false;

	if ( !pTarget->IsSolid() )
		return false;

	Assert( pTarget );
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pTarget->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		int material = pList[i]->GetMaterialIndex();
		const surfacedata_t *pSurfaceData = physprops->GetSurfaceData( material );

		// Is flesh or metal? Go for it!
		if ( pSurfaceData->game.material == CHAR_TEX_METAL || 
			pSurfaceData->game.material == CHAR_TEX_FLESH || 
			pSurfaceData->game.material == CHAR_TEX_VENT || 
			pSurfaceData->game.material == CHAR_TEX_GRATE || 
			pSurfaceData->game.material == CHAR_TEX_COMPUTER || 
			pSurfaceData->game.material == CHAR_TEX_BLOODYFLESH || 
			pSurfaceData->game.material == CHAR_TEX_ALIENFLESH )
		{
			return true;
		}
	}
	return false;
}


//------------------------------------------------------------------------------
// Effects
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::CreateZapBeam( const Vector &vecTargetPos )
{
	CEffectData	data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = 0; // m_nGunTipAttachment;
	data.m_vOrigin = vecTargetPos;
	data.m_flScale = 5;
	DispatchEffect( "TeslaZap", data ); 
}

void CNPC_AttackHelicopter::CreateEntityZapEffect( CBaseEntity *pEnt )
{
	CEffectData	data;
	data.m_nEntIndex = pEnt->entindex();
	data.m_flMagnitude = 10;
	data.m_flScale = 1.0f;
	DispatchEffect( "TeslaHitboxes", data );
}


//------------------------------------------------------------------------------
// Here's what we do when we *are* firing
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::FireElectricityGun( )
{
	if ( m_flNextAttack > gpGlobals->curtime )
		return;

	EmitSound( "ReallyLoudSpark" );

	CBaseEntity *ppEnts[256];
	Vector vecCenter = WorldSpaceCenter();
	float flRadius = 500.0f;
	vecCenter.z -= flRadius * 0.8f;
	int nEntCount = UTIL_EntitiesInSphere( ppEnts, 256, vecCenter, flRadius, 0 );
	CBaseEntity *ppCandidates[256];
	int nCandidateCount = 0;
	int i;
	for ( i = 0; i < nEntCount; i++ )
	{
		if ( ppEnts[i] == NULL )
			continue;

		// Zap metal or flesh things.
		if ( !IsValidZapTarget( ppEnts[i] ) )
			continue;

		ppCandidates[ nCandidateCount++ ] = ppEnts[i];
	}

	// First, put a bolt in front of the player, at random
	float flDist = 1024;
	if ( GetEnemy() )
	{
		Vector vecDelta;
		Vector2DSubtract( GetEnemy()->WorldSpaceCenter().AsVector2D(), WorldSpaceCenter().AsVector2D(), vecDelta.AsVector2D() );
		vecDelta.z = 0.0f;

		flDist = VectorNormalize( vecDelta );
		Vector vecPerp( -vecDelta.y, vecDelta.x, 0.0f );
		int nBoltCount = (int)(ClampSplineRemapVal( flDist, 256.0f, 1024.0f, 8, 0 ) + 0.5f);

		for ( i = 0; i < nBoltCount; ++i )
		{
			Vector vecTargetPt = GetEnemy()->WorldSpaceCenter();
			VectorMA( vecTargetPt, random->RandomFloat( flDist + 100, flDist + 500 ), vecDelta, vecTargetPt );
			VectorMA( vecTargetPt, random->RandomFloat( -500, 500 ), vecPerp, vecTargetPt );
			vecTargetPt.z += random->RandomFloat( -500, 500 );
			CreateZapBeam( vecTargetPt );
		}
	}

	// Next, choose the number of bolts...
	int nBoltCount = random->RandomInt( 8, 16 );
	for ( i = 0; i < nBoltCount; ++i )
	{
		if ( (nCandidateCount > 0) && random->RandomFloat( 0.0f, 1.0f ) < 0.6f )
		{
			--nCandidateCount;

			Vector vecTarget;
			ppCandidates[nCandidateCount]->CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecTarget );
			CreateZapBeam( vecTarget );
			CreateEntityZapEffect( ppCandidates[nCandidateCount] );
		}
		else
		{
			// Select random point *on* sphere
			Vector vecTargetPt;
			float flEffectRadius = random->RandomFloat( flRadius * 1.2, flRadius * 1.5f );
			float flTheta = random->RandomFloat( 0.0f, 2.0f * M_PI );
			float flPhi = random->RandomFloat( -0.5f * M_PI, 0.5f * M_PI );
			vecTargetPt.x = cos(flTheta) * cos(flPhi);
			vecTargetPt.y = sin(flTheta) * cos(flPhi);
			vecTargetPt.z = sin(flPhi);
			vecTargetPt *= flEffectRadius;
			vecTargetPt += vecCenter;

			CreateZapBeam( vecTargetPt );
		}
	}

	// Finally, put a bolt right at the player, at random 
	float flHitRatio = ClampSplineRemapVal( flDist, 128.0f, 512.0f, 0.75f, 0.0f );
	if ( random->RandomFloat( 0.0f, 1.0f ) < flHitRatio )
	{
		if ( GetEnemyVehicle() )
		{
			Vector vecTarget;
			GetEnemyVehicle()->CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecTarget );
			CreateZapBeam( vecTarget );
			CreateEntityZapEffect( GetEnemyVehicle() );

			CTakeDamageInfo info( this, this, 5, DMG_SHOCK );
			GetEnemy()->TakeDamage( info );
		}
		else if ( GetEnemy() )
		{
			Vector vecTarget;
			GetEnemy()->CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecTarget );
			CreateZapBeam( vecTarget );

			CTakeDamageInfo info( this, this, 5, DMG_SHOCK );
			GetEnemy()->TakeDamage( info );
		}
	}

	m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.3f, 1.0f );
}


//------------------------------------------------------------------------------
// Here's what we do when we *are* firing
//------------------------------------------------------------------------------
#define INTERVAL_BETWEEN_HITS 4

bool CNPC_AttackHelicopter::DoGunFiring( const Vector &vBasePos, const Vector &vGunDir, const Vector &vecFireAtPosition )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	float flVolume = controller.SoundGetVolume( m_pGunFiringSound );
	if ( flVolume != 1.0f )
	{
		controller.SoundChangeVolume( m_pGunFiringSound, 1.0, 0.01f );
	}

	if ( ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE ) && ( IsInSecondaryMode( BULLRUSH_MODE_SHOOT_GUN ) ) )
	{
		ShootAtFacingDirection( vBasePos, vGunDir, m_nRemainingBursts == 0 );
	}
	else if ( GetEnemyVehicle() )
	{
		ShootAtVehicle( vBasePos, vecFireAtPosition );
	}
	else if ( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		if ( !IsDeadlyShooting() )
		{
			ShootAtPlayer( vBasePos, vGunDir );
		}
		else
		{
			ShootAtFacingDirection( vBasePos, vGunDir, true );
		}
	}
	else
	{
		ShootAtFacingDirection( vBasePos, vGunDir, false );
	}

	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		if ( --m_nRemainingBursts < 0 )
		{
			m_nRemainingBursts = INTERVAL_BETWEEN_HITS;
		}
		return true;
	}

	--m_nRemainingBursts;
	if ( m_nRemainingBursts > 0 )
		return true;

	controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.01f );
	float flIdleTime = CHOPPER_GUN_IDLE_TIME;
	float flVariance = flIdleTime * 0.1f;
	m_flNextAttack = gpGlobals->curtime + m_flIdleTimeDelay + random->RandomFloat(flIdleTime - flVariance, flIdleTime + flVariance);
	m_nGunState = GUN_STATE_IDLE;
	SetPauseState( PAUSE_NO_PAUSE );
	return true;
}


//------------------------------------------------------------------------------
// Is it "fair" to drop this bomb?
//------------------------------------------------------------------------------
#define MIN_BOMB_DISTANCE_SQR ( 600.0f * 600.0f )

bool CNPC_AttackHelicopter::IsBombDropFair( const Vector &vecBombStartPos, const Vector &vecBombVelocity )
{
	if ( (m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE) && IsInSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER ) )
		return true;

	// Can happen if you're noclipping around
	if ( !GetEnemy() )
		return false;

	// If the player is moving slowly, it's fair
	if ( GetEnemy()->GetSmoothedVelocity().LengthSqr() < ( CHOPPER_SLOW_BOMB_SPEED * CHOPPER_SLOW_BOMB_SPEED ) )
		return true;

	// Skip out if we're right above or behind the player.. that's unfair
	if ( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		// How much time will it take to fall?
		// dx = 0.5 * a * t^2
		Vector vecTarget = GetEnemy()->BodyTarget( GetAbsOrigin(), false );
		float dz = vecBombStartPos.z - vecTarget.z;
		float dt = (dz > 0.0f) ? sqrt( 2 * dz / GetCurrentGravity() ) : 0.0f;

		// Where will the enemy be in that time?
		Vector vecEnemyVel = GetEnemy()->GetSmoothedVelocity();
		VectorMA( vecTarget, dt, vecEnemyVel, vecTarget );

		// Where will the bomb be in that time?
		Vector vecBomb;
		VectorMA( vecBombStartPos, dt, vecBombVelocity, vecBomb );

		float flEnemySpeed = vecEnemyVel.LengthSqr();
		flEnemySpeed = clamp( flEnemySpeed, 200.0f, 500.0f );
		float flDistFactorSq = RemapVal( flEnemySpeed, 200.0f, 500.0f, 0.3f, 1.0f );
		flDistFactorSq *= flDistFactorSq;

		// If it's too close, then we're not doing it.
		if ( vecBomb.AsVector2D().DistToSqr( vecTarget.AsVector2D() ) < (flDistFactorSq * MIN_BOMB_DISTANCE_SQR) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Create the bomb entity and set it up
// Input  : &vecPos - Position to spawn at
//			&vecVelocity - velocity to spawn with
//-----------------------------------------------------------------------------
CGrenadeHelicopter *CNPC_AttackHelicopter::SpawnBombEntity( const Vector &vecPos, const Vector &vecVelocity )
{
	// Create the grenade and set it up
	CGrenadeHelicopter *pGrenade = static_cast<CGrenadeHelicopter*>(CreateEntityByName( "grenade_helicopter" ));
	pGrenade->SetAbsOrigin( vecPos );
	pGrenade->SetOwnerEntity( this );
	pGrenade->SetThrower( this );
	pGrenade->SetAbsVelocity( vecVelocity );
	DispatchSpawn( pGrenade );
	pGrenade->SetExplodeOnContact( m_bBombsExplodeOnContact );

#ifdef HL2_EPISODIC
	// Disable collisions with the owner's bone followers while we drop
	physfollower_t *pFollower = m_BoneFollowerManager.GetBoneFollower( 0 );
	if ( pFollower )
	{
		CBaseEntity *pBoneFollower = pFollower->hFollower;
		PhysDisableEntityCollisions( pBoneFollower, pGrenade );
		pGrenade->SetCollisionObject( pBoneFollower );
	}
#endif // HL2_EPISODIC

	return pGrenade;
}

//------------------------------------------------------------------------------
// Actually drops the bomb
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::CreateBomb( bool bCheckForFairness, Vector *pVecVelocity, bool bMegaBomb )
{
	if ( m_bBombingSuppressed )
		return;

	Vector vTipPos;
	GetAttachment( m_nBombAttachment, vTipPos );

	if ( !CBombSuppressor::CanBomb( vTipPos ) )
		return;
	
	// Compute velocity
	Vector vecActualVelocity;
	if ( !pVecVelocity )
	{
		Vector vecAcross;
		vecActualVelocity = GetAbsVelocity();
		CrossProduct( vecActualVelocity, Vector( 0, 0, 1 ), vecAcross );
		VectorNormalize( vecAcross );
		vecAcross *= random->RandomFloat( 10.0f, 30.0f );
		vecAcross *= random->RandomFloat( 0.0f, 1.0f ) < 0.5f ? 1.0f : -1.0f;

		// Blat out z component of velocity if it's moving upward....
		if ( vecActualVelocity.z > 0 )
		{
			vecActualVelocity.z = 0.0f;
		}

		vecActualVelocity += vecAcross;
	}
	else
	{
		vecActualVelocity = *pVecVelocity;
	}

	if ( bCheckForFairness )
	{
		if ( !IsBombDropFair( vTipPos, vecActualVelocity ) )
			return;
	}

	AddGesture( (Activity)ACT_HELICOPTER_DROP_BOMB );
	EmitSound( "NPC_AttackHelicopter.DropMine" );

	// Make the bomb and send it off
	CGrenadeHelicopter *pGrenade = SpawnBombEntity( vTipPos, vecActualVelocity );
	if ( pGrenade && bMegaBomb )
	{
		pGrenade->AddSpawnFlags( SF_GRENADE_HELICOPTER_MEGABOMB );
	}
}


//------------------------------------------------------------------------------
// Drop a bomb at a particular location
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBomb( inputdata_t &inputdata )
{
	if ( m_flInputDropBombTime > gpGlobals->curtime )
		return;

	// Prevent two triggers from being hit the same frame
	m_flInputDropBombTime = gpGlobals->curtime + 0.01f;

	CreateBomb(	);

	// If we're in the middle of a bomb dropping schedule, wait to drop another bomb.
	if ( ShouldDropBombs() )
	{
		m_flNextAttack = gpGlobals->curtime + 0.5f + random->RandomFloat( 0.3f, 0.6f );
	}
}


//------------------------------------------------------------------------------
// Drops a bomb straight downwards
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBombStraightDown( inputdata_t &inputdata )
{
	if ( m_flInputDropBombTime > gpGlobals->curtime )
		return;

	// Prevent two triggers from being hit the same frame
	m_flInputDropBombTime = gpGlobals->curtime + 0.01f;

	Vector vTipPos;
	GetAttachment( m_nBombAttachment, vTipPos );

	// Make the bomb drop straight down
	SpawnBombEntity( vTipPos, vec3_origin );

	// If we're in the middle of a bomb dropping schedule, wait to drop another bomb.
	if ( ShouldDropBombs() )
	{
		m_flNextAttack = gpGlobals->curtime + 0.5f + random->RandomFloat( 0.3f, 0.6f );
	}
}


//------------------------------------------------------------------------------
// Drop a bomb at a particular location
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBombAtTargetInternal( inputdata_t &inputdata, bool bCheckFairness )
{
	if ( m_flInputDropBombTime > gpGlobals->curtime )
		return;

	// Prevent two triggers from being hit the same frame
	m_flInputDropBombTime = gpGlobals->curtime + 0.01f;

	// Find our specified target
	string_t strBombTarget = MAKE_STRING( inputdata.value.String() );
	CBaseEntity *pBombEnt = gEntList.FindEntityByName( NULL, strBombTarget );
	if ( pBombEnt == NULL )
	{
		Warning( "%s: Could not find bomb drop target '%s'!\n", GetClassname(), STRING( strBombTarget ) );
		return;
	}

	Vector vTipPos;
	GetAttachment( m_nBombAttachment, vTipPos );

	// Compute the time it would take to fall to the target
	Vector vecTarget = pBombEnt->BodyTarget( GetAbsOrigin(), false );
	float dz = vTipPos.z - vecTarget.z;
	if ( dz <= 0.0f )
	{
		Warning("Bomb target %s is above the chopper!\n", STRING( strBombTarget ) );
		return;
	}
	float dt = sqrt( 2 * dz / GetCurrentGravity() );

	// Compute the velocity that would make it happen
	Vector vecVelocity;
	VectorSubtract( vecTarget, vTipPos, vecVelocity );
	vecVelocity /= dt;
	vecVelocity.z = 0.0f;
	
	if ( bCheckFairness )
	{
		if ( !IsBombDropFair( vTipPos, vecVelocity ) )
			return;
	}

	// Make the bomb and send it off
	SpawnBombEntity( vTipPos, vecVelocity );

	// If we're in the middle of a bomb dropping schedule, wait to drop another bomb.
	if ( ShouldDropBombs() )
	{
		m_flNextAttack = gpGlobals->curtime + 1.5f + random->RandomFloat( 0.1f, 0.2f );
	}
}


//------------------------------------------------------------------------------
// Drop a bomb at a particular location
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBombAtTargetAlways( inputdata_t &inputdata )
{
	InputDropBombAtTargetInternal( inputdata, false );
}

	
//------------------------------------------------------------------------------
// Drop a bomb at a particular location
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBombAtTarget( inputdata_t &inputdata )
{
	InputDropBombAtTargetInternal( inputdata, true );
}


//------------------------------------------------------------------------------
// Drop a bomb at a particular location
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputDropBombDelay( inputdata_t &inputdata )
{
	m_flInputDropBombTime = gpGlobals->curtime + inputdata.value.Float();

	if ( ShouldDropBombs() )
	{
		m_flNextAttack = m_flInputDropBombTime;
	}
}


//------------------------------------------------------------------------------
// Drop those bombs!
//------------------------------------------------------------------------------
#define MAX_BULLRUSH_BOMB_DISTANCE_SQR ( 3072.0f * 3072.0f )

void CNPC_AttackHelicopter::DropBombs( )
{
	// Can't continually fire....
	if (m_flNextAttack > gpGlobals->curtime)
		return;

	// Otherwise, behave as normal.
	if ( m_nAttackMode != ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		if ( GetEnemy() && GetEnemy()->IsPlayer() )
		{
			if ( GetEnemy()->GetSmoothedVelocity().LengthSqr() > ( CHOPPER_SLOW_BOMB_SPEED * CHOPPER_SLOW_BOMB_SPEED ) )
			{
				// Don't drop bombs if you are behind the player, unless the player is moving slowly
				float flLeadingDistSq = GetLeadingDistance() * 0.75f;
				flLeadingDistSq *= flLeadingDistSq;

				Vector vecPoint;
				ClosestPointToCurrentPath( &vecPoint );
				if ( vecPoint.AsVector2D().DistToSqr( GetDesiredPosition().AsVector2D() ) > flLeadingDistSq )
					return;
			}
		}
	}
	else
	{
		// Skip out if we're bullrushing but too far from the player
		if ( GetEnemy() )
		{
			if ( GetEnemy()->GetAbsOrigin().AsVector2D().DistToSqr( GetAbsOrigin().AsVector2D() ) > MAX_BULLRUSH_BOMB_DISTANCE_SQR )
				return;
		}
	}

	CreateBomb( );

	m_flNextAttack = gpGlobals->curtime + 0.5f + random->RandomFloat( 0.3f, 0.6f );

	if ( (m_nAttackMode != ATTACK_MODE_BULLRUSH_VEHICLE) )
	{
		if ( --m_nGrenadeCount <= 0 )
		{
			m_nGrenadeCount = CHOPPER_BOMB_DROP_COUNT;
			m_flNextAttack += random->RandomFloat( 1.5f, 3.0f );
		}
	}
}


//------------------------------------------------------------------------------
// Should we drop those bombs?
//------------------------------------------------------------------------------
#define BOMB_GRACE_PERIOD 1.5f
#define BOMB_MIN_SPEED 150.0

bool CNPC_AttackHelicopter::ShouldDropBombs( void )
{
	if ( IsCarpetBombing() )
		return true;

	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		// Distance determines whether or not we should do this
		if ((m_nSecondaryMode == BULLRUSH_MODE_SHOOT_IDLE_PLAYER) && (SecondaryModeTime() >= BULLRUSH_IDLE_PLAYER_FIRE_TIME))
			return ShouldBombIdlePlayer();

		return (( m_nSecondaryMode == BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED ) || ( m_nSecondaryMode == BULLRUSH_MODE_DROP_BOMBS_FOLLOW_PLAYER ));
	}

	if (!IsLeading() || !GetEnemyVehicle())
		return false;

	if (( m_nAttackMode != ATTACK_MODE_BOMB_VEHICLE ) && ( m_nAttackMode != ATTACK_MODE_ALWAYS_LEAD_VEHICLE ))
		return false;

	if ( m_nGunState != GUN_STATE_IDLE )
		return false;

	// This is for bombing. If you get hit, give a grace period to get back to speed
	float flSpeedSqr = GetEnemy()->GetSmoothedVelocity().LengthSqr();
	if ( flSpeedSqr >= BOMB_MIN_SPEED * BOMB_MIN_SPEED )
	{
		m_flLastFastTime = gpGlobals->curtime;
	}
	else
	{
		if ( ( gpGlobals->curtime - m_flLastFastTime ) < BOMB_GRACE_PERIOD )
			return false;
	}

	float flSpeedAlongPath = TargetSpeedAlongPath();
	if ( m_nAttackMode == ATTACK_MODE_BOMB_VEHICLE )
		return ( flSpeedAlongPath > -BOMB_MIN_SPEED );

	// This is for ALWAYS_LEAD
	if ( fabs(flSpeedAlongPath) < 50.0f )
		return false;

	float flLeadingDist = ComputeDistanceToLeadingPosition( );
	flLeadingDist = GetLeadingDistance() - flLeadingDist;
	if ( flSpeedAlongPath < 0.0f )
	{
		return flLeadingDist < 300.0f;
	}
	else
	{
		return flLeadingDist > -300.0f;
	}
}


//------------------------------------------------------------------------------
// Different bomb-dropping behavior
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::BullrushBombs( )
{
	if ( gpGlobals->curtime < m_flNextBullrushBombTime )
		return;

	if ( m_nBullrushBombMode & 0x1 )
	{
		CreateBomb( false, NULL, true );
	}
	else
	{
		Vector vecAcross;
		Vector vecVelocity = GetAbsVelocity();
		CrossProduct( vecVelocity, Vector( 0, 0, 1 ), vecAcross );
		VectorNormalize( vecAcross );
		vecAcross *= random->RandomFloat( 300.0f, 500.0f );

		// Blat out z component of velocity if it's moving upward....
		if ( vecVelocity.z > 0 )
		{
			vecVelocity.z = 0.0f;
		}
		vecVelocity += vecAcross;
		CreateBomb( false, &vecVelocity, true );

		VectorMA( vecVelocity, -2.0f, vecAcross, vecVelocity );
		CreateBomb( false, &vecVelocity, true );
	}

	m_nBullrushBombMode = !m_nBullrushBombMode;
	m_flNextBullrushBombTime = gpGlobals->curtime + 0.2f;
}


//-----------------------------------------------------------------------------
// Purpose: Turn the gun off
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InputGunOff( inputdata_t &inputdata )
{
	BaseClass::InputGunOff( inputdata );

	if ( m_pGunFiringSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.01f );
	}
}


//------------------------------------------------------------------------------
// Fire that gun baby!
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::FireGun( void )
{
	// Do the test electricity gun
	if ( HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
	{
		FireElectricityGun( );
		return true;
	}

	// HACK: CBaseHelicopter ignores this, and fire forever at the last place it saw the player. Why?
	if (( m_nGunState == GUN_STATE_IDLE ) && ( m_nAttackMode != ATTACK_MODE_BULLRUSH_VEHICLE ) && !IsCarpetBombing() )
	{
		if ( (m_flLastSeen + 1 <= gpGlobals->curtime) || (m_flPrevSeen + m_flGracePeriod > gpGlobals->curtime) )
			return false;
	}

	if ( IsCarpetBombing() )
	{
		BullrushBombs();
		return false;
	}

	if ( ShouldDropBombs() )
	{
		DropBombs( );
		return false;
	}

	// Drop those bullrush bombs when shooting...
	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		if ( IsInSecondaryMode( BULLRUSH_MODE_MEGA_BOMB ) )
		{
			BullrushBombs( );
			return false;
		}

		// Don't fire if we're bullrushing and we're getting distance
		if ( !IsInSecondaryMode( BULLRUSH_MODE_SHOOT_GUN ) && !IsInSecondaryMode(BULLRUSH_MODE_SHOOT_IDLE_PLAYER) )
			return false;

		// If we're in the grace period on this mode, then don't fire
		if ( IsInSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER ) && (SecondaryModeTime() < BULLRUSH_IDLE_PLAYER_FIRE_TIME) )
		{
			// Stop our gun sound
			if ( m_nGunState != GUN_STATE_IDLE )
			{
				ShutdownGunDuringBullrush();
			}
			
			return false;
		}
	}

	// Get gun attachment points
	Vector vBasePos;
	GetAttachment( m_nGunBaseAttachment, vBasePos );

	// Aim perfectly while idle, but after charging, the gun don't move so fast.
	Vector vecFireAtPosition;
	if ( !GetEnemyVehicle() || (m_nGunState == GUN_STATE_IDLE) )
	{
		ComputeFireAtPosition( &vecFireAtPosition );
	}
	else
	{
		ComputeVehicleFireAtPosition( &vecFireAtPosition );
	}
	
	Vector vTargetDir = vecFireAtPosition - vBasePos;
	VectorNormalize( vTargetDir );

	// Makes the model of the gun point to where we're aiming.
	if ( !PoseGunTowardTargetDirection( vTargetDir ) )
		return false;

	// Are we charging?
	if ( m_nGunState == GUN_STATE_CHARGING )
	{
		if ( !DoGunCharging( ) )
			return false;
	}

	Vector vTipPos;
	GetAttachment( m_nGunTipAttachment, vTipPos );

	Vector vGunDir = vTipPos - vBasePos;
	VectorNormalize( vGunDir );

	// Are we firing?
	if ( m_nGunState == GUN_STATE_FIRING )
	{
		return DoGunFiring( vTipPos, vGunDir, vecFireAtPosition );
	}

	return DoGunIdle( vGunDir, vTargetDir );
}


//-----------------------------------------------------------------------------
// Should we trigger a damage effect?
//-----------------------------------------------------------------------------
inline bool CNPC_AttackHelicopter::ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const
{
	int nPrevRange = (int)( ((float)nPrevHealth / (float)GetMaxHealth()) * nEffectCount );
	int nRange = (int)( ((float)GetHealth() / (float)GetMaxHealth()) * nEffectCount );
	return ( nRange != nPrevRange );
}


//-----------------------------------------------------------------------------
// Add a smoke trail since we've taken more damage
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::AddSmokeTrail( const Vector &vecPos )
{
	if ( m_nSmokeTrailCount == MAX_SMOKE_TRAILS )
		return;

	// See if there's an attachment for this smoke trail
	int nAttachment = LookupAttachment( UTIL_VarArgs( "damage%d", m_nSmokeTrailCount ) );

	if ( nAttachment == 0 )
		return;

	// The final smoke trail is a flaming engine
	if ( m_nSmokeTrailCount == 0 || m_nSmokeTrailCount % 2 )
	{
		CFireTrail *pFireTrail = CFireTrail::CreateFireTrail();

		if ( pFireTrail == NULL )
			return;

		m_hSmokeTrail[m_nSmokeTrailCount] = pFireTrail;

		pFireTrail->FollowEntity( this, UTIL_VarArgs( "damage%d", m_nSmokeTrailCount ) );
		pFireTrail->SetParent( this, nAttachment );
		pFireTrail->SetLocalOrigin( vec3_origin );
		pFireTrail->SetMoveType( MOVETYPE_NONE );
		pFireTrail->SetLifetime( -1 );
	}
	else
	{
		SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
		if( !pSmokeTrail )
			return;

		m_hSmokeTrail[m_nSmokeTrailCount] = pSmokeTrail;

		pSmokeTrail->m_SpawnRate = 48;
		pSmokeTrail->m_ParticleLifetime = 0.5f;
		pSmokeTrail->m_StartColor.Init(0.15, 0.15, 0.15);
		pSmokeTrail->m_EndColor.Init(0.0, 0.0, 0.0);
		pSmokeTrail->m_StartSize = 24;
		pSmokeTrail->m_EndSize = 80;
		pSmokeTrail->m_SpawnRadius = 8;
		pSmokeTrail->m_Opacity = 0.2;
		pSmokeTrail->m_MinSpeed = 16;
		pSmokeTrail->m_MaxSpeed = 64;
		pSmokeTrail->SetLifetime(-1);
		pSmokeTrail->SetParent( this, nAttachment );
		pSmokeTrail->SetLocalOrigin( vec3_origin );
		pSmokeTrail->SetMoveType( MOVETYPE_NONE );
	}

	m_nSmokeTrailCount++;
}


//-----------------------------------------------------------------------------
// Destroy all smoke trails
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::DestroySmokeTrails()
{
	for ( int i = m_nSmokeTrailCount; --i >= 0; )
	{
		UTIL_Remove( m_hSmokeTrail[i] );
		m_hSmokeTrail[i] = NULL;
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecChunkPos - 
//-----------------------------------------------------------------------------
void Chopper_CreateChunk( CBaseEntity *pChopper, const Vector &vecChunkPos, const QAngle &vecChunkAngles, const char *pszChunkName, bool bSmall )
{
	// Drop a flaming, smoking chunk.
	CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
	pChunk->Spawn( pszChunkName );
	pChunk->SetBloodColor( DONT_BLEED );

	pChunk->SetAbsOrigin( vecChunkPos );
	pChunk->SetAbsAngles( vecChunkAngles );

	pChunk->SetOwnerEntity( pChopper );
	
	if ( bSmall )
	{
		pChunk->m_lifeTime = random->RandomFloat( 0.5f, 1.0f );
		pChunk->SetSolidFlags( FSOLID_NOT_SOLID );
		pChunk->SetSolid( SOLID_BBOX );
		pChunk->AddEffects( EF_NODRAW );
		pChunk->SetGravity( UTIL_ScaleForGravity( 400 ) );
	}
	else
	{
		pChunk->m_lifeTime = 5.0f;
	}
	
	pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	
	// Set the velocity
	Vector vecVelocity;
	AngularImpulse angImpulse;

	QAngle angles;
	angles.x = random->RandomFloat( -70, 20 );
	angles.y = random->RandomFloat( 0, 360 );
	angles.z = 0.0f;
	AngleVectors( angles, &vecVelocity );
	
	vecVelocity *= random->RandomFloat( 550, 800 );
	vecVelocity += pChopper->GetAbsVelocity();

	angImpulse = RandomAngularImpulse( -180, 180 );

	pChunk->SetAbsVelocity( vecVelocity );

	if ( bSmall == false )
	{
		IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
		
		if ( pPhysicsObject )
		{
			pPhysicsObject->EnableMotion( true );
			pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
		}
	}
	
	CFireTrail *pFireTrail = CFireTrail::CreateFireTrail();

	if ( pFireTrail == NULL )
		return;

	pFireTrail->FollowEntity( pChunk, "" );
	pFireTrail->SetParent( pChunk, 0 );
	pFireTrail->SetLocalOrigin( vec3_origin );
	pFireTrail->SetMoveType( MOVETYPE_NONE );
	pFireTrail->SetLifetime( pChunk->m_lifeTime );
}

//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ExplodeAndThrowChunk( const Vector &vecExplosionPos )
{
	CEffectData data;
	data.m_vOrigin = vecExplosionPos;
	DispatchEffect( "HelicopterMegaBomb", data );

	EmitSound( "BaseExplosionEffect.Sound" );

	UTIL_ScreenShake( vecExplosionPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	if(GetCrashPoint() != NULL)
	{
		// Make it clear that I'm done for.
		ExplosionCreate( vecExplosionPos, QAngle(0,0,1), this, 100, 128, false );
	}

	if ( random->RandomInt( 0, 4 ) )
	{
		for ( int i = 0; i < 2; i++ )
		{
			Chopper_CreateChunk( this, vecExplosionPos, RandomAngle(0, 360), g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ), true );
		}
	}
	else
	{
		Chopper_CreateChunk( this, vecExplosionPos, RandomAngle(0, 360), s_pChunkModelName[random->RandomInt( 0, CHOPPER_MAX_SMALL_CHUNKS - 1 )], false );
	}
}


//-----------------------------------------------------------------------------
// Drop a corpse!
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::DropCorpse( int nDamage )
{
	// Don't drop another corpse if the next guy's not out on the gun yet
	if ( m_flLastCorpseFall > gpGlobals->curtime )
		return;

	// Clamp damage to prevent ridiculous ragdoll velocity
	if( nDamage > 250.0f )
		nDamage = 250.0f;

	m_flLastCorpseFall = gpGlobals->curtime + 3.0;

	// Spawn a ragdoll combine guard
	float forceScale = nDamage * 75 * 4;
	Vector vecForceVector = RandomVector(-1,1);
	vecForceVector.z = 0.5;
	vecForceVector *= forceScale;

	CBaseEntity *pGib = CreateRagGib( "models/combine_soldier.mdl", GetAbsOrigin(), GetAbsAngles(), vecForceVector );
	if ( pGib )
	{
		pGib->SetOwnerEntity( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Take no damage from trace attacks unless it's blast damage. RadiusDamage() sometimes calls
	// TraceAttack() as a means for delivering blast damage. Usually when the explosive penetrates
	// the target. (RPG missiles do this sometimes).
	if ( ( info.GetDamageType() & DMG_AIRBOAT ) || 
		 ( info.GetInflictor()->Classify() == CLASS_MISSILE ) || 
		 ( info.GetAttacker()->Classify() == CLASS_MISSILE ) )
	{
		BaseClass::BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AttackHelicopter::OnTakeDamage( const CTakeDamageInfo &info )
{
	// We don't take blast damage from anything but the airboat or missiles (or myself!)
	if( info.GetInflictor() != this )
	{
		if ( ( ( info.GetDamageType() & DMG_AIRBOAT ) == 0 ) && 
			( info.GetInflictor()->Classify() != CLASS_MISSILE ) && 
			( info.GetAttacker()->Classify() != CLASS_MISSILE ) )
			return 0;
	}

	if ( m_bIndestructible )
	{
		if ( GetHealth() < info.GetDamage() )
			return 0;
	}

	// helicopter takes extra damage from its own grenades
	CGrenadeHelicopter *pGren = dynamic_cast<CGrenadeHelicopter *>(info.GetInflictor());
	if ( pGren && info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		CTakeDamageInfo fudgedInfo = info;

		float damage;
		if( g_pGameRules->IsSkillLevel(SKILL_EASY) )
		{
			damage = GetMaxHealth() / sk_helicopter_num_bombs1.GetFloat();
		}
		else if( g_pGameRules->IsSkillLevel(SKILL_HARD) )
		{
			damage = GetMaxHealth() / sk_helicopter_num_bombs3.GetFloat();
		}
		else // Medium, or unspecified
		{
			damage = GetMaxHealth() / sk_helicopter_num_bombs2.GetFloat();
		}
		damage = ceilf( damage );
		fudgedInfo.SetDamage( damage );
		fudgedInfo.SetMaxDamage( damage );

		return BaseClass::OnTakeDamage( fudgedInfo );
	}

	return BaseClass::OnTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: Take damage from trace attacks if they hit the gunner
//-----------------------------------------------------------------------------
int CNPC_AttackHelicopter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	int nPrevHealth = GetHealth();

	if ( ( info.GetInflictor() != NULL ) && ( info.GetInflictor()->GetOwnerEntity() != NULL ) && ( info.GetInflictor()->GetOwnerEntity() == this ) )
	{
		// Don't take damage from my own bombs. (Unless the player grabbed them and threw them back)
		return 0;
	}

	// Chain
	int nRetVal = BaseClass::OnTakeDamage_Alive( info );

	if( info.GetDamageType() & DMG_BLAST )
	{
		// Apply a force push that makes us look like we're reacting to the damage
		Vector	damageDir = info.GetDamageForce();
		VectorNormalize( damageDir );
		ApplyAbsVelocityImpulse( damageDir * 500.0f );

		// Knock the helicopter off of the level, too.
		Vector vecRight, vecForce;
		float flDot;
		GetVectors( NULL, &vecRight, NULL );
		vecForce = info.GetDamageForce();
		VectorNormalize( vecForce );

		flDot = DotProduct( vecForce, vecRight );

		m_flGoalRollDmg = random->RandomFloat( 10, 30 );

		if( flDot <= 0.0f )
		{
			// Missile hit the right side.
			m_flGoalRollDmg *= -1;
		}
	}

	// Spawn damage effects
	if ( nPrevHealth != GetHealth() )
	{
		// Give the badly damaged call to say we're going to mega bomb soon
		if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
		{
			if (( nPrevHealth > m_flNextMegaBombHealth ) && (GetHealth() <= m_flNextMegaBombHealth) ) 
			{
				EmitSound( "NPC_AttackHelicopter.BadlyDamagedAlert" );
			}
		}

		if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_SMOKE_TRAILS ) )
		{
			AddSmokeTrail( info.GetDamagePosition() );
		}

		if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_CORPSES ) )
		{
			if ( nPrevHealth != GetMaxHealth() )
			{
				DropCorpse( info.GetDamage() );
			}
		}

		if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_EXPLOSIONS ) )
		{
			ExplodeAndThrowChunk( info.GetDamagePosition() );
		}

		int nPrevPercent = (int)(100.0f * nPrevHealth / GetMaxHealth());
		int nCurrPercent = (int)(100.0f * GetHealth() / GetMaxHealth());
		if (( (nPrevPercent + 9) / 10 ) != ( (nCurrPercent + 9) / 10 ))
		{
			m_OnHealthChanged.Set( nCurrPercent, this, this );
		}
	}

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Chopper_BecomeChunks( CBaseEntity *pChopper )
{
	QAngle vecChunkAngles = pChopper->GetAbsAngles();
	Vector vecForward, vecUp;
	pChopper->GetVectors( &vecForward, NULL, &vecUp );

#ifdef HL2_EPISODIC
	CNPC_AttackHelicopter *pAttackHelicopter;
	pAttackHelicopter = dynamic_cast<CNPC_AttackHelicopter*>(pChopper);
	if( pAttackHelicopter != NULL )
	{
		// New for EP2, we may be tailspinning, (crashing) and playing an animation that is spinning
		// our root bone, which means our model is not facing the way our entity is facing. So we have
		// to do some attachment point math to get the proper angles to use for computing the relative
		// positions of the gibs. The attachment points called DAMAGE0 is properly oriented and attached
		// to the chopper body so we can use its angles.
		int iAttach = pAttackHelicopter->LookupAttachment( "damage0" );
		Vector vecAttachPos;

		if( iAttach > -1 )
		{
			pAttackHelicopter->GetAttachment(iAttach, vecAttachPos, vecChunkAngles );
			AngleVectors( vecChunkAngles, &vecForward, NULL, &vecUp );
		}
	}
#endif//HL2_EPISODIC


	Vector vecChunkPos = pChopper->GetAbsOrigin();

	Vector vecRight(0,0,0);

	if( hl2_episodic.GetBool() )
	{
		// We need to get a right hand vector to toss the cockpit and tail pieces
		// so their motion looks like a continuation of the tailspin animation
		// that the chopper plays before crashing.
		pChopper->GetVectors( NULL, &vecRight, NULL );
	}

	// Body
	CHelicopterChunk *pBodyChunk = CHelicopterChunk::CreateHelicopterChunk( vecChunkPos, vecChunkAngles, pChopper->GetAbsVelocity(), HELICOPTER_CHUNK_BODY, CHUNK_BODY );
	Chopper_CreateChunk( pChopper, vecChunkPos, RandomAngle( 0, 360 ), s_pChunkModelName[random->RandomInt( 0, CHOPPER_MAX_CHUNKS - 1 )], false );

	vecChunkPos = pChopper->GetAbsOrigin() + ( vecForward * 100.0f ) + ( vecUp * -38.0f );

	// Cockpit
	CHelicopterChunk *pCockpitChunk = CHelicopterChunk::CreateHelicopterChunk( vecChunkPos, vecChunkAngles, pChopper->GetAbsVelocity() + vecRight * -800.0f, HELICOPTER_CHUNK_COCKPIT, CHUNK_COCKPIT );
	Chopper_CreateChunk( pChopper, vecChunkPos, RandomAngle( 0, 360 ), s_pChunkModelName[random->RandomInt( 0, CHOPPER_MAX_CHUNKS - 1 )], false );

	pCockpitChunk->m_hMaster = pBodyChunk;

	vecChunkPos = pChopper->GetAbsOrigin() + ( vecForward * -175.0f );

	// Tail
	CHelicopterChunk *pTailChunk = CHelicopterChunk::CreateHelicopterChunk( vecChunkPos, vecChunkAngles, pChopper->GetAbsVelocity() + vecRight * 800.0f, HELICOPTER_CHUNK_TAIL, CHUNK_TAIL );
	Chopper_CreateChunk( pChopper, vecChunkPos, RandomAngle( 0, 360 ), s_pChunkModelName[random->RandomInt( 0, CHOPPER_MAX_CHUNKS - 1 )], false );

	pTailChunk->m_hMaster = pBodyChunk;

	// Constrain all the pieces together loosely
	IPhysicsObject *pBodyObject = pBodyChunk->VPhysicsGetObject();
	Assert( pBodyObject );

	IPhysicsObject *pCockpitObject = pCockpitChunk->VPhysicsGetObject();
	Assert( pCockpitObject );

	IPhysicsObject *pTailObject = pTailChunk->VPhysicsGetObject();
	Assert( pTailObject );

	IPhysicsConstraintGroup *pGroup = NULL;
	
	// Create the constraint
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( pBodyObject, pTailObject );
	fixed.constraint.Defaults();

	pBodyChunk->m_pTailConstraint = physenv->CreateFixedConstraint( pBodyObject, pTailObject, pGroup, fixed );

	fixed.Defaults();
	fixed.InitWithCurrentObjectState( pBodyObject, pCockpitObject );
	fixed.constraint.Defaults();

	pBodyChunk->m_pCockpitConstraint = physenv->CreateFixedConstraint( pBodyObject, pCockpitObject, pGroup, fixed );
}

//-----------------------------------------------------------------------------
// Purpose: Start us crashing
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::Event_Killed( const CTakeDamageInfo &info )
{
	if( m_lifeState == LIFE_ALIVE )
	{
		m_OnShotDown.FireOutput( this, this );
	}

	m_lifeState			= LIFE_DYING;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.1f );

	if( GetCrashPoint() == NULL )
	{
		CBaseEntity *pCrashPoint = gEntList.FindEntityByClassname( NULL, "info_target_helicopter_crash" );
		if( pCrashPoint != NULL )
		{
			m_hCrashPoint.Set( pCrashPoint );
			SetDesiredPosition( pCrashPoint->GetAbsOrigin() );

			// Start the failing engine sound
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundDestroy( m_pRotorSound );

			CPASAttenuationFilter filter( this );
			m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.EngineFailure" );
			controller.Play( m_pRotorSound, 1.0, 100 );

			// Tailspin!!
			SetActivity( ACT_HELICOPTER_CRASHING );

			// Intentionally returning with m_lifeState set to LIFE_DYING
			return;
		}
	}

	SendOnKilledGameEvent( info );

	Chopper_BecomeChunks( this );
	StopLoopingSounds();

	m_lifeState = LIFE_DEAD;

	EmitSound( "NPC_CombineGunship.Explode" );

	SetThink( &CNPC_AttackHelicopter::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );

	AddEffects( EF_NODRAW );

	// Makes the slower rotors fade back in
	SetStartupTime( gpGlobals->curtime + 99.0f );

	m_iHealth = 0;
	m_takedamage = DAMAGE_NO;

	m_OnDeath.FireOutput( info.GetAttacker(), this );
}

//------------------------------------------------------------------------------
// Creates the breakable husk of an attack chopper
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::CreateChopperHusk()
{
	// We're embedded into the ground
	CBaseEntity *pCorpse = CreateEntityByName( "prop_physics" );
	pCorpse->SetAbsOrigin( GetAbsOrigin() );
	pCorpse->SetAbsAngles( GetAbsAngles() );
	pCorpse->SetModel( CHOPPER_MODEL_CORPSE_NAME );
	pCorpse->AddSpawnFlags( SF_PHYSPROP_MOTIONDISABLED );
	pCorpse->Spawn();
	pCorpse->SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Think!	
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::PrescheduleThink( void )
{
	if ( m_flGoalRollDmg != 0.0f )
	{
		m_flGoalRollDmg = UTIL_Approach( 0, m_flGoalRollDmg, 2.0f );
	}

	switch( m_lifeState )
	{
	case LIFE_DYING:
		{
			if( GetCrashPoint() != NULL )
			{
				// Stay on this, no matter what.
				SetDesiredPosition( GetCrashPoint()->WorldSpaceCenter() );
			}

			if ( random->RandomInt( 0, 4 ) == 0 )
			{
				Vector	explodePoint;		
				CollisionProp()->RandomPointInBounds( Vector(0.25,0.25,0.25), Vector(0.75,0.75,0.75), &explodePoint );
				
				ExplodeAndThrowChunk( explodePoint );
			}
		}
		break;
	}

	BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
float CNPC_AttackHelicopter::UpdatePerpPathDistance( float flMaxPathOffset )
{
	if ( !IsLeading() || !GetEnemy() )
	{
		m_flCurrPathOffset = 0.0f;
		return 0.0f;
	}

	float flNewPathOffset = TargetDistanceToPath();

	// Make bomb dropping more interesting
	if ( ShouldDropBombs() )
	{
		float flSpeedAlongPath = TargetSpeedAlongPath();

		if ( flSpeedAlongPath > 10.0f )
		{
			float flLeadTime = GetLeadingDistance() / flSpeedAlongPath;
			flLeadTime = clamp( flLeadTime, 0.0f, 2.0f );
			flNewPathOffset += 0.25 * flLeadTime * TargetSpeedAcrossPath();
		}

		flSpeedAlongPath = clamp( flSpeedAlongPath, 100.0f, 500.0f );
		float flSinHeight = SimpleSplineRemapVal( flSpeedAlongPath, 100.0f, 500.0f, 0.0f, 200.0f );
		flNewPathOffset += flSinHeight * sin( 2.0f * M_PI * (gpGlobals->curtime / 6.0f) );
	}

	if ( (flMaxPathOffset != 0.0f) && (flNewPathOffset > flMaxPathOffset) )
	{
		flNewPathOffset = flMaxPathOffset;
	}

	float flMaxChange = 1000.0f * (gpGlobals->curtime - GetLastThink());
	if ( fabs( flNewPathOffset - m_flCurrPathOffset ) < flMaxChange )
	{
		m_flCurrPathOffset = flNewPathOffset;
	}
	else
	{
		float flSign = (m_flCurrPathOffset < flNewPathOffset) ? 1.0f : -1.0f;
		m_flCurrPathOffset += flSign * flMaxChange;
	}

	return m_flCurrPathOffset;
}


//-----------------------------------------------------------------------------
// Computes the max speed + acceleration:	
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::GetMaxSpeedAndAccel( float *pMaxSpeed, float *pAccelRate )
{
	*pAccelRate = CHOPPER_ACCEL_RATE;
	*pMaxSpeed = GetMaxSpeed();
	if ( GetEnemyVehicle() )
	{
		*pAccelRate *= 9.0f;
	}
}


//-----------------------------------------------------------------------------
// Computes the acceleration:	
//-----------------------------------------------------------------------------
#define HELICOPTER_GRAVITY	384
#define HELICOPTER_DT		0.1f
#define HELICOPTER_MIN_DZ_DAMP	-500.0f
#define HELICOPTER_MAX_DZ_DAMP	-1000.0f
#define HELICOPTER_FORCE_BLEND 0.8f
#define HELICOPTER_FORCE_BLEND_VEHICLE 0.2f

void CNPC_AttackHelicopter::ComputeVelocity( const Vector &vecTargetPosition, 
	float flAdditionalHeight, float flMinDistFromSegment, float flMaxDistFromSegment, Vector *pVecAccel )
{
	Vector deltaPos;
	VectorSubtract( vecTargetPosition, GetAbsOrigin(), deltaPos ); 

	// calc goal linear accel to hit deltaPos in dt time.
	// This is solving the equation xf = 0.5 * a * dt^2 + vo * dt + xo
	float dt = 1.0f;
	pVecAccel->x = 2.0f * (deltaPos.x - GetAbsVelocity().x * dt) / (dt * dt);
	pVecAccel->y = 2.0f * (deltaPos.y - GetAbsVelocity().y * dt) / (dt * dt);
	pVecAccel->z = 2.0f * (deltaPos.z - GetAbsVelocity().z * dt) / (dt * dt) + HELICOPTER_GRAVITY;

	float flDistFromPath = 0.0f;
	Vector vecPoint, vecDelta;
	if ( flMaxDistFromSegment != 0.0f )
	{
		// Also, add in a little force to get us closer to our current line segment if we can
		ClosestPointToCurrentPath( &vecPoint );

		if ( flAdditionalHeight != 0.0f )
		{
			Vector vecEndPoint, vecClosest;
			vecEndPoint = vecPoint;
			vecEndPoint.z += flAdditionalHeight;
			CalcClosestPointOnLineSegment( GetAbsOrigin(), vecPoint, vecEndPoint, vecClosest );
			vecPoint = vecClosest;
		}

		VectorSubtract( vecPoint, GetAbsOrigin(), vecDelta );
 		flDistFromPath = VectorNormalize( vecDelta );
		if ( flDistFromPath > flMaxDistFromSegment )
		{
			// Strongly constrain to an n unit pipe around the current path
			// by damping out all impulse forces that would push us further from the pipe
			float flAmount = (flDistFromPath - flMaxDistFromSegment) / 200.0f;
			flAmount = clamp( flAmount, 0, 1 );
			VectorMA( *pVecAccel, flAmount * 200.0f, vecDelta, *pVecAccel );
		}
	}

	// Apply avoidance forces
	if ( !HasSpawnFlags( SF_HELICOPTER_IGNORE_AVOID_FORCES ) )
	{
		Vector vecAvoidForce;
		CAvoidSphere::ComputeAvoidanceForces( this, 350.0f, 2.0f, &vecAvoidForce );
		*pVecAccel += vecAvoidForce;
		CAvoidBox::ComputeAvoidanceForces( this, 350.0f, 2.0f, &vecAvoidForce );
		*pVecAccel += vecAvoidForce;
	}

	// don't fall faster than 0.2G or climb faster than 2G
	pVecAccel->z = clamp( pVecAccel->z, HELICOPTER_GRAVITY * 0.2f, HELICOPTER_GRAVITY * 2.0f );

	// The lift factor owing to horizontal movement
	float flHorizLiftFactor = fabs( pVecAccel->x ) * 0.10f + fabs( pVecAccel->y ) * 0.10f;

	// If we're way above the path, dampen horizontal lift factor
	float flNewHorizLiftFactor = clamp( deltaPos.z, HELICOPTER_MAX_DZ_DAMP, HELICOPTER_MIN_DZ_DAMP );
	flNewHorizLiftFactor = SimpleSplineRemapVal( flNewHorizLiftFactor, HELICOPTER_MIN_DZ_DAMP, HELICOPTER_MAX_DZ_DAMP, flHorizLiftFactor, 2.5f * (HELICOPTER_GRAVITY * 0.2) );
	float flDampening = (flNewHorizLiftFactor != 0.0f) ? (flNewHorizLiftFactor / flHorizLiftFactor) : 1.0f;
	if ( flDampening < 1.0f )
	{
		pVecAccel->x *= flDampening;
		pVecAccel->y *= flDampening;
		flHorizLiftFactor = flNewHorizLiftFactor;
	}

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	// First, attenuate the current force
	float flForceBlend = GetEnemyVehicle() ? HELICOPTER_FORCE_BLEND_VEHICLE : HELICOPTER_FORCE_BLEND;
	m_flForce *= flForceBlend;

	// Now add force based on our acceleration factors
	m_flForce += ( pVecAccel->z + flHorizLiftFactor ) * HELICOPTER_DT * (1.0f - flForceBlend);

	// The force is always *locally* upward based; we pitch + roll the chopper to get movement
	Vector vecImpulse;
	VectorMultiply( up, m_flForce, vecImpulse );
	
	// NOTE: These have to be done *before* the additional path distance drag forces are applied below
	ApplySidewaysDrag( right );
	ApplyGeneralDrag();

	// If LIFE_DYING, maintain control as long as we're flying to a crash point.
	if ( m_lifeState != LIFE_DYING || (m_lifeState == LIFE_DYING && GetCrashPoint() != NULL) )
	{
		vecImpulse.z += -HELICOPTER_GRAVITY * HELICOPTER_DT;

		if ( flMinDistFromSegment != 0.0f && ( flDistFromPath > flMinDistFromSegment ) )
		{
			Vector	vecVelDir = GetAbsVelocity();

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
	}
	else
	{
		// No more upward lift...
		vecImpulse.z = -HELICOPTER_GRAVITY * HELICOPTER_DT;

		// Damp the horizontal impulses; we should pretty much be falling ballistically
		vecImpulse.x *= 0.1f;
		vecImpulse.y *= 0.1f;
	}

	// Add in our velocity pulse for this frame
	ApplyAbsVelocityImpulse( vecImpulse );
}



//-----------------------------------------------------------------------------
// Computes the max speed + acceleration:	
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::ComputeAngularVelocity( const Vector &vecGoalUp, const Vector &vecFacingDirection )
{
	QAngle goalAngAccel;
	if ( m_lifeState != LIFE_DYING || (m_lifeState == LIFE_DYING && GetCrashPoint() != NULL) )
	{
		Vector forward, right, up;
		GetVectors( &forward, &right, &up );

		Vector goalUp = vecGoalUp;
		VectorNormalize( goalUp );

		// calc goal orientation to hit linear accel forces
		float goalPitch = RAD2DEG( asin( DotProduct( forward, goalUp ) ) );
		float goalYaw = UTIL_VecToYaw( vecFacingDirection );
		float goalRoll = RAD2DEG( asin( DotProduct( right, goalUp ) ) + m_flGoalRollDmg );
		goalPitch *= 0.75f;

		// clamp goal orientations
		goalPitch = clamp( goalPitch, -30, 45 );
		goalRoll = clamp( goalRoll, -45, 45 );

		// calc angular accel needed to hit goal pitch in dt time.
		float dt = 0.6;
		goalAngAccel.x = 2.0 * (AngleDiff( goalPitch, AngleNormalize( GetAbsAngles().x ) ) - GetLocalAngularVelocity().x * dt) / (dt * dt);
		goalAngAccel.y = 2.0 * (AngleDiff( goalYaw, AngleNormalize( GetAbsAngles().y ) ) - GetLocalAngularVelocity().y * dt) / (dt * dt);
		goalAngAccel.z = 2.0 * (AngleDiff( goalRoll, AngleNormalize( GetAbsAngles().z ) ) - GetLocalAngularVelocity().z * dt) / (dt * dt);

		goalAngAccel.x = clamp( goalAngAccel.x, -300, 300 );
		//goalAngAccel.y = clamp( goalAngAccel.y, -60, 60 );
		goalAngAccel.y = clamp( goalAngAccel.y, -120, 120 );
		goalAngAccel.z = clamp( goalAngAccel.z, -300, 300 );
	}
	else
	{
		goalAngAccel.x	= 0;
		goalAngAccel.y = random->RandomFloat( 50, 120 );
		goalAngAccel.z	= 0;
	}

	// limit angular accel changes to similate mechanical response times
	QAngle angAccelAccel;
	float dt = 0.1;
	angAccelAccel.x = (goalAngAccel.x - m_vecAngAcceleration.x) / dt;
	angAccelAccel.y = (goalAngAccel.y - m_vecAngAcceleration.y) / dt;
	angAccelAccel.z = (goalAngAccel.z - m_vecAngAcceleration.z) / dt;

	angAccelAccel.x = clamp( angAccelAccel.x, -1000, 1000 );
	angAccelAccel.y = clamp( angAccelAccel.y, -1000, 1000 );
	angAccelAccel.z = clamp( angAccelAccel.z, -1000, 1000 );

	// DevMsg( "pitch %6.1f (%6.1f:%6.1f)  ", goalPitch, GetLocalAngles().x, m_vecAngVelocity.x );
	// DevMsg( "roll %6.1f (%6.1f:%6.1f) : ", goalRoll, GetLocalAngles().z, m_vecAngVelocity.z );
	// DevMsg( "%6.1f %6.1f %6.1f  :  ", goalAngAccel.x, goalAngAccel.y, goalAngAccel.z );
	// DevMsg( "%6.0f %6.0f %6.0f\n", angAccelAccel.x, angAccelAccel.y, angAccelAccel.z );

	m_vecAngAcceleration += angAccelAccel * 0.1;

	QAngle angVel = GetLocalAngularVelocity();
	angVel += m_vecAngAcceleration * 0.1;
	angVel.y = clamp( angVel.y, -120, 120 );

	// Fix up pitch and yaw to tend toward small values
	if ( m_lifeState == LIFE_DYING && GetCrashPoint() == NULL )
	{
		float flPitchDiff = random->RandomFloat( -5, 5 ) - GetAbsAngles().x;
		angVel.x = flPitchDiff * 0.1f;
		float flRollDiff = random->RandomFloat( -5, 5 ) - GetAbsAngles().z;
		angVel.z = flRollDiff * 0.1f;
	}

	SetLocalAngularVelocity( angVel );

	float flAmt = clamp( angVel.y, -30, 30 ); 
	float flRudderPose = RemapVal( flAmt, -30, 30, 45, -45 );
	SetPoseParameter( "rudder", flRudderPose );
}


//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::FlightDirectlyOverhead( void )
{
	Vector vecTargetPosition = m_vecTargetPosition;
	CBaseEntity *pEnemy = GetEnemy();
	if ( HasEnemy() && FVisible( pEnemy ) )
	{
		if ( GetEnemy()->IsPlayer() )
		{
			CBaseEntity *pEnemyVehicle = assert_cast<CBasePlayer*>(GetEnemy())->GetVehicleEntity();
			if ( pEnemyVehicle )
			{
				Vector vecEnemyVel = pEnemyVehicle->GetSmoothedVelocity();
				Vector vecRelativePosition;
				VectorSubtract( GetAbsOrigin(), pEnemyVehicle->GetAbsOrigin(), vecRelativePosition );
				float flDist = VectorNormalize( vecRelativePosition );
				float flEnemySpeed = VectorNormalize( vecEnemyVel );
				float flDot = DotProduct( vecRelativePosition, vecEnemyVel );  
				float flSpeed = GetMaxSpeed() * 0.3f; //GetAbsVelocity().Length();

				float a = flSpeed * flSpeed - flEnemySpeed * flEnemySpeed;
				float b = 2.0f * flEnemySpeed * flDist * flDot;
				float c = - flDist * flDist;

				float flDiscrim = b * b - 4 * a * c;
				if ( flDiscrim >= 0 )
				{
					float t = ( -b + sqrt( flDiscrim ) ) / (2 * a);
					t = clamp( t, 0.0f, 4.0f );
					VectorMA( pEnemyVehicle->GetAbsOrigin(), t * flEnemySpeed, vecEnemyVel, vecTargetPosition );
				}
			}
		}
	}

//	if ( GetCurrentPathTargetPosition() )
//	{
//		vecTargetPosition.z = GetCurrentPathTargetPosition()->z;
//	}

	NDebugOverlay::Cross3D( vecTargetPosition, -Vector(32,32,32), Vector(32,32,32), 0, 0, 255, true, 0.1f );

	UpdateFacingDirection( vecTargetPosition );

	Vector accel;
	ComputeVelocity( vecTargetPosition, 0.0f, 0.0f, 0.0f, &accel );
	ComputeAngularVelocity( accel, m_vecDesiredFaceDir );
}


//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::Flight( void )
{
	if( GetFlags() & FL_ONGROUND )
	{
		// This would be really bad.
		SetGroundEntity( NULL );
	}

	// Determine the distances we must lie from the path
	float flMaxPathOffset = MaxDistanceFromCurrentPath();
	float flPerpDist = UpdatePerpPathDistance( flMaxPathOffset );

	float flMinDistFromSegment, flMaxDistFromSegment;
	if ( !IsLeading() )
	{
		flMinDistFromSegment = 0.0f;
		flMaxDistFromSegment = 0.0f;
	}
	else
	{
		flMinDistFromSegment = fabs(flPerpDist) + 100.0f;
		flMaxDistFromSegment = fabs(flPerpDist) + 200.0f;
		if ( flMaxPathOffset != 0.0 )
		{
			if ( flMaxDistFromSegment > flMaxPathOffset - 100.0f )
			{
				flMaxDistFromSegment = flMaxPathOffset - 100.0f;
			}

			if ( flMinDistFromSegment > flMaxPathOffset - 200.0f )
			{
				flMinDistFromSegment = flMaxPathOffset - 200.0f;
			}
		}
	}

	float maxSpeed, accelRate;
	GetMaxSpeedAndAccel( &maxSpeed, &accelRate );

	Vector vecTargetPosition;
	float flCurrentSpeed = GetAbsVelocity().Length();
	float flDist = MIN( flCurrentSpeed + accelRate, maxSpeed );
	float dt = 1.0f;
	ComputeActualTargetPosition( flDist, dt, flPerpDist, &vecTargetPosition );

	// Raise high in the air when doing the shooting attack
	float flAdditionalHeight = 0.0f;
	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		flAdditionalHeight = clamp( m_flBullrushAdditionalHeight, 0.0f, flMaxPathOffset );
		vecTargetPosition.z += flAdditionalHeight;
	}

	Vector accel;
	UpdateFacingDirection( vecTargetPosition );
	ComputeVelocity( vecTargetPosition, flAdditionalHeight, flMinDistFromSegment, flMaxDistFromSegment, &accel );
	ComputeAngularVelocity( accel, m_vecDesiredFaceDir );
}


//------------------------------------------------------------------------------
// Updates the facing direction
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::UpdateFacingDirection( const Vector &vecActualDesiredPosition )
{
	bool bIsBullrushing = ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE );

	bool bSeenTargetRecently = HasSpawnFlags( SF_HELICOPTER_AGGRESSIVE ) || ( m_flLastSeen + 5 > gpGlobals->curtime ); 
	if ( GetEnemy() && !bIsBullrushing )
	{
		if ( !IsLeading() )
		{
			if( IsCarpetBombing() && hl2_episodic.GetBool() )
			{
				m_vecDesiredFaceDir = vecActualDesiredPosition - GetAbsOrigin();
			}
			else if ( !IsCrashing() && bSeenTargetRecently )
			{
				// If we've seen the target recently, face the target.
				m_vecDesiredFaceDir = m_vecTargetPosition - GetAbsOrigin();
			}
			else
			{
				// Remain facing the way you were facing...
			}
		}
		else
		{
			if ( ShouldDropBombs() || IsCarpetBombing() )
			{
				m_vecDesiredFaceDir = vecActualDesiredPosition - GetAbsOrigin();
			}
			else
			{
				m_vecDesiredFaceDir = m_vecTargetPosition - GetAbsOrigin();
			}
		}
	}
	else
	{
		// Face our desired position
		float flDistSqr = vecActualDesiredPosition.AsVector2D().DistToSqr( GetAbsOrigin().AsVector2D() );
		if ( flDistSqr <= 50 * 50 )
		{
			if (( flDistSqr > 1 * 1 ) && bSeenTargetRecently && IsInSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER ) ) 
			{
				m_vecDesiredFaceDir = m_vecTargetPosition - GetAbsOrigin();
				m_vecDesiredFaceDir.z = 0.0f;
			}
			else
			{
				GetVectors( &m_vecDesiredFaceDir, NULL, NULL );
			}
		}
		else
		{
			m_vecDesiredFaceDir = vecActualDesiredPosition - GetAbsOrigin();
		}
	}
	VectorNormalize( m_vecDesiredFaceDir ); 
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
#define ENEMY_CREEP_RATE	400
float CNPC_AttackHelicopter::CreepTowardEnemy( float flSpeed, float flMinSpeed, float flMaxSpeed, float flMinDist, float flMaxDist )
{
	float dt = gpGlobals->curtime - GetLastThink();
	float flEnemyCreepDist = ENEMY_CREEP_RATE * dt;

	// When the player is slow, creep toward him within a second or two
	float flLeadingDist = ClampSplineRemapVal( flSpeed, flMinSpeed, flMaxSpeed, flMinDist, flMaxDist );
	float flCurrentDist = GetLeadingDistance( );
	if ( fabs(flLeadingDist - flCurrentDist) > flEnemyCreepDist )
	{
		float flSign = ( flLeadingDist < flCurrentDist ) ? -1.0f : 1.0f;
		flLeadingDist = flCurrentDist + flSign * flEnemyCreepDist;
	}

	return flLeadingDist;
}


#define MIN_ENEMY_SPEED	300


//------------------------------------------------------------------------------
// Computes how far to lead the player when bombing
//------------------------------------------------------------------------------
float CNPC_AttackHelicopter::ComputeBombingLeadingDistance( float flSpeed, float flSpeedAlongPath, bool bEnemyInVehicle )
{
	if ( ( flSpeed <= MIN_ENEMY_SPEED ) && bEnemyInVehicle )
	{
		return CreepTowardEnemy( flSpeed, 0.0f, MIN_ENEMY_SPEED, 0.0f, 1000.0f );
	}

	return ClampSplineRemapVal( flSpeedAlongPath, 200.0f, 600.0f, 1000.0f, 2000.0f );
}


//------------------------------------------------------------------------------
// Computes how far to lead the player when bullrushing
//------------------------------------------------------------------------------
float CNPC_AttackHelicopter::ComputeBullrushLeadingDistance( float flSpeed, float flSpeedAlongPath, bool bEnemyInVehicle )
{
	switch ( m_nSecondaryMode )
	{
	case BULLRUSH_MODE_WAIT_FOR_ENEMY:
		return 0.0f;

	case BULLRUSH_MODE_GET_DISTANCE:
		return m_bRushForward ? -CHOPPER_BULLRUSH_MODE_DISTANCE : CHOPPER_BULLRUSH_MODE_DISTANCE;

	case BULLRUSH_MODE_DROP_BOMBS_FOLLOW_PLAYER:
//		return m_bRushForward ? 1500.0f : -1500.0f;
		return ComputeBombingLeadingDistance( flSpeed, flSpeedAlongPath, bEnemyInVehicle ); 

	case BULLRUSH_MODE_SHOOT_IDLE_PLAYER:
		return 0.0f;

	case BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED:
		return m_bRushForward ? 7000 : -7000;

	case BULLRUSH_MODE_MEGA_BOMB:
		return m_bRushForward ? CHOPPER_BULLRUSH_MODE_DISTANCE : -CHOPPER_BULLRUSH_MODE_DISTANCE;

	case BULLRUSH_MODE_SHOOT_GUN:
		{
			float flLeadDistance = 1000.f - CHOPPER_BULLRUSH_ENEMY_BOMB_DISTANCE;
			return m_bRushForward ? flLeadDistance : -flLeadDistance;
		}
	}

	Assert(0);
	return 0.0f;
}


//------------------------------------------------------------------------------
// Secondary mode
//------------------------------------------------------------------------------
inline void CNPC_AttackHelicopter::SetSecondaryMode( int nMode, bool bRetainTime )
{
	m_nSecondaryMode = nMode;
	if (!bRetainTime)
	{
		m_flSecondaryModeStartTime = gpGlobals->curtime;
	}
}

inline bool CNPC_AttackHelicopter::IsInSecondaryMode( int nMode )
{
	return m_nSecondaryMode == nMode;
}

inline float CNPC_AttackHelicopter::SecondaryModeTime( ) const
{
	return gpGlobals->curtime - m_flSecondaryModeStartTime;
}


//------------------------------------------------------------------------------
// Switch to idle
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::SwitchToBullrushIdle( void )
{
	// Put us directly into idle gun state (we're in firing state)
	m_flNextAttack = gpGlobals->curtime;
	m_nGunState = GUN_STATE_IDLE;
	m_nRemainingBursts = 0;
	m_flBullrushAdditionalHeight = 0.0f;
	SetPauseState( PAUSE_NO_PAUSE );

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.1f );
}


//------------------------------------------------------------------------------
// Should the chopper shoot the idle player?
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::ShouldShootIdlePlayerInBullrush()
{
	// Once he starts shooting, then don't stop until the player is moving pretty fast
	float flSpeedSqr = IsInSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER ) ? CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_2_SQ : CHOPPER_BULLRUSH_SLOW_SHOOT_SPEED_SQ;
	return ( GetEnemy() && GetEnemy()->GetSmoothedVelocity().LengthSqr() <= flSpeedSqr );
}


//------------------------------------------------------------------------------
// Shutdown shooting during bullrush
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::ShutdownGunDuringBullrush( )
{
	// Put us directly into idle gun state (we're in firing state)
	m_flNextAttack = gpGlobals->curtime;
	m_nGunState = GUN_STATE_IDLE;
	m_nRemainingBursts = 0;
	SetPauseState( PAUSE_NO_PAUSE );

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pGunFiringSound, 0.0, 0.1f );
}

#define	HELICOPTER_MIN_IDLE_BOMBING_DIST	350.0f
#define HELICOPTER_MIN_IDLE_BOMBING_SPEED	350.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AttackHelicopter::ShouldBombIdlePlayer( void )
{
	// Must be settled over a position and not moving too quickly to do this
	if ( GetAbsVelocity().LengthSqr() > Square(HELICOPTER_MIN_IDLE_BOMBING_SPEED) )
		return false;

	// Must be within a certain range of the target
	float flDistToTargetSqr = (GetEnemy()->WorldSpaceCenter() - GetAbsOrigin()).Length2DSqr();
	
	if ( flDistToTargetSqr < Square(HELICOPTER_MIN_IDLE_BOMBING_DIST) )
		return true;

	// Can't bomb this
	return false;
}

//------------------------------------------------------------------------------
// Update the bullrush state
//------------------------------------------------------------------------------
#define BULLRUSH_GOAL_TOLERANCE 200
#define BULLRUSH_BOMB_MAX_DISTANCE 3500

void CNPC_AttackHelicopter::UpdateBullrushState( void )
{
	if ( !GetEnemy() || IsInForcedMove() )
	{
		if ( !IsInSecondaryMode( BULLRUSH_MODE_WAIT_FOR_ENEMY ) )
		{
			SwitchToBullrushIdle();
			SetSecondaryMode( BULLRUSH_MODE_WAIT_FOR_ENEMY );
		}
	}

	switch( m_nSecondaryMode )
	{
	case BULLRUSH_MODE_WAIT_FOR_ENEMY:
		{
			m_flBullrushAdditionalHeight = CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET;
			if ( GetEnemy() && !IsInForcedMove() )
			{
				// This forces us to not start trying checking positions 
				// until we have been on the path for a little while
				if ( SecondaryModeTime() > 0.3f )
				{
					float flDistanceToGoal = ComputeDistanceToTargetPosition();
					Vector vecPathDir;
					CurrentPathDirection( &vecPathDir );
					bool bMovingForward = DotProduct2D( GetAbsVelocity().AsVector2D(), vecPathDir.AsVector2D() ) >= 0.0f;
					if ( flDistanceToGoal * (bMovingForward ? 1.0f : -1.0f) > 1000 )
					{
						m_bRushForward = bMovingForward;
						SetSecondaryMode( BULLRUSH_MODE_SHOOT_GUN );
						SpotlightStartup();
					}
					else
					{
						m_bRushForward = !bMovingForward;
						SetSecondaryMode( BULLRUSH_MODE_GET_DISTANCE );
					}
				}
			}
			else
			{
				m_flSecondaryModeStartTime = gpGlobals->curtime;
			}
		}
		break;

	case BULLRUSH_MODE_GET_DISTANCE:
		{
			m_flBullrushAdditionalHeight = CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET;

			float flDistanceToGoal = ComputeDistanceToTargetPosition();
			if ( m_bRushForward )
			{
				if ( flDistanceToGoal < (CHOPPER_BULLRUSH_MODE_DISTANCE - 1000) )
					break;
			}
			else
			{
				if ( flDistanceToGoal > -(CHOPPER_BULLRUSH_MODE_DISTANCE - 1000) )
					break;
			}

			if ( GetHealth() <= m_flNextMegaBombHealth )
			{
				m_flNextMegaBombHealth -= GetMaxHealth() * g_helicopter_bullrush_mega_bomb_health.GetFloat();
				m_flNextBullrushBombTime = gpGlobals->curtime;
				SetSecondaryMode( BULLRUSH_MODE_MEGA_BOMB );
				EmitSound( "NPC_AttackHelicopter.MegabombAlert" );
			}
			else
			{
				SetSecondaryMode( BULLRUSH_MODE_SHOOT_GUN );
				SpotlightStartup();
			}
		}
		break;

	case BULLRUSH_MODE_MEGA_BOMB:
		{
			m_flBullrushAdditionalHeight = CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET;

			float flDistanceToGoal = ComputeDistanceToTargetPosition();
			if ( m_bRushForward )
			{
				if ( flDistanceToGoal > -(CHOPPER_BULLRUSH_MODE_DISTANCE - 1000) )
					break;
			}
			else
			{
				if ( flDistanceToGoal < (CHOPPER_BULLRUSH_MODE_DISTANCE - 1000) )
					break;
			}

			m_bRushForward = !m_bRushForward;
			SetSecondaryMode( BULLRUSH_MODE_GET_DISTANCE );
		}
		break;

	case BULLRUSH_MODE_SHOOT_GUN:
		{
			// When shooting, stop when we cross the player's position
			// Then start bombing. Use the fixed speed version if we're too far
			// from the enemy or if he's travelling in the opposite direction.
			// Otherwise, do the standard bombing behavior for a while.
			float flDistanceToGoal = ComputeDistanceToTargetPosition();

			float flShootingHeight = CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET;
			float flSwitchToBombDist = CHOPPER_BULLRUSH_ENEMY_BOMB_DISTANCE;
			float flDropDownDist = 2000.0f;
			if ( m_bRushForward )
			{
				m_flBullrushAdditionalHeight = ClampSplineRemapVal( flDistanceToGoal, 
					flSwitchToBombDist, flSwitchToBombDist + flDropDownDist, 0.0f, flShootingHeight );
				if ( flDistanceToGoal > flSwitchToBombDist )
					break;
			}
			else
			{
				m_flBullrushAdditionalHeight = ClampSplineRemapVal( flDistanceToGoal, 
					-flSwitchToBombDist - flDropDownDist, -flSwitchToBombDist, flShootingHeight, 0.0f );
				if ( flDistanceToGoal < -flSwitchToBombDist )
					break;
			}

			if ( ShouldShootIdlePlayerInBullrush() )
			{
				SetSecondaryMode( BULLRUSH_MODE_SHOOT_IDLE_PLAYER );
			}
			else
			{
				ShutdownGunDuringBullrush( );
				SetSecondaryMode( BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED );
			}
		}
		break;

	case BULLRUSH_MODE_SHOOT_IDLE_PLAYER:
		{
			// Shut down our gun if we're switching to bombing
			if ( ShouldBombIdlePlayer() )
			{
				// Must not already be shutdown
				if (( m_nGunState != GUN_STATE_IDLE ) && (SecondaryModeTime() >= BULLRUSH_IDLE_PLAYER_FIRE_TIME))
				{
					ShutdownGunDuringBullrush( );
				}
			}

//			m_nBurstHits = 0;
			m_flCircleOfDeathRadius = ClampSplineRemapVal( SecondaryModeTime(), BULLRUSH_IDLE_PLAYER_FIRE_TIME, BULLRUSH_IDLE_PLAYER_FIRE_TIME + 5.0f, 256.0f, 64.0f );
			m_flBullrushAdditionalHeight = CHOPPER_BULLRUSH_SHOOTING_VERTICAL_OFFSET;
			if ( !ShouldShootIdlePlayerInBullrush() )
			{
				ShutdownGunDuringBullrush( );
				SetSecondaryMode( BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED );
			}
		}
		break;

	case BULLRUSH_MODE_DROP_BOMBS_FOLLOW_PLAYER:
		{
			m_flBullrushAdditionalHeight = 0.0f;
			float flDistanceToGoal = ComputeDistanceToTargetPosition();
			if ( fabs( flDistanceToGoal ) > 2000.0f )
			{
				SetSecondaryMode( BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED, true );
				break;
			}
		}
		// FALL THROUGH!!

	case BULLRUSH_MODE_DROP_BOMBS_FIXED_SPEED:
		{
			float flDistanceToGoal = ComputeDistanceToTargetPosition();

			m_flBullrushAdditionalHeight = 0.0f;
			if (( SecondaryModeTime() >= CHOPPER_BULLRUSH_ENEMY_BOMB_TIME ) || ( flDistanceToGoal > BULLRUSH_BOMB_MAX_DISTANCE ))
			{
				m_bRushForward = !m_bRushForward;
				SetSecondaryMode( BULLRUSH_MODE_GET_DISTANCE );
			}
		}
		break;
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::UpdateEnemyLeading( void )
{
	bool bEnemyInVehicle = true;
	CBaseEntity *pTarget = GetEnemyVehicle();
	if ( !pTarget )
	{
		bEnemyInVehicle = false;
		if ( (m_nAttackMode == ATTACK_MODE_DEFAULT) || !GetEnemy() )
		{
			EnableLeading( false );
			return;
		}

		pTarget = GetEnemy();
	}

	EnableLeading( true );

	float flLeadingDist = 0.0f;
	float flSpeedAlongPath = TargetSpeedAlongPath();
	float flSpeed = pTarget->GetSmoothedVelocity().Length();

	// Do the test electricity gun
	if ( HasSpawnFlags(SF_HELICOPTER_ELECTRICAL_DRONE) )
	{
		if ( flSpeedAlongPath < 200.0f )
		{
			flLeadingDist = ClampSplineRemapVal( flSpeedAlongPath, 0.0f, 200.0f, 100.0f, -200.0f );
		}
		else
		{
			flLeadingDist = ClampSplineRemapVal( flSpeedAlongPath, 200.0f, 600.0f, -200.0f, -500.0f );
		}
		SetLeadingDistance( flLeadingDist );
		return;
	}

	switch( m_nAttackMode )
	{
	case ATTACK_MODE_BULLRUSH_VEHICLE:
		flLeadingDist = ComputeBullrushLeadingDistance( flSpeed, flSpeedAlongPath, bEnemyInVehicle );
		break;

	case ATTACK_MODE_ALWAYS_LEAD_VEHICLE:
		if (( flSpeed <= MIN_ENEMY_SPEED ) && (bEnemyInVehicle) )
		{
			flLeadingDist = CreepTowardEnemy( flSpeed, 0.0f, MIN_ENEMY_SPEED, 0.0f, 1000.0f );
		}
		else
		{
			if ( flSpeedAlongPath > 0.0f )
			{
				flLeadingDist = ClampSplineRemapVal( flSpeedAlongPath, 200.0f, 600.0f, 1000.0f, 2000.0f );
			}
			else
			{
				flLeadingDist = ClampSplineRemapVal( flSpeedAlongPath, -600.0f, -200.0f, -2000.0f, -1000.0f );
			}
		}
		break;

	case ATTACK_MODE_BOMB_VEHICLE:
		flLeadingDist = ComputeBombingLeadingDistance( flSpeed, flSpeedAlongPath, bEnemyInVehicle );
		break;

	case ATTACK_MODE_DEFAULT:
	case ATTACK_MODE_TRAIL_VEHICLE:
		if (( flSpeed <= MIN_ENEMY_SPEED ) && (bEnemyInVehicle))
		{
			flLeadingDist = CreepTowardEnemy( flSpeed, 150.0f, MIN_ENEMY_SPEED, 500.0f, -1000.0f );
		}
		else
		{
			flLeadingDist = ClampSplineRemapVal( flSpeedAlongPath, -600.0f, -200.0f, -2500.0f, -1000.0f );
		}
		break;
	}

	SetLeadingDistance( flLeadingDist );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInfo - 
//			bAlways - 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Make our smoke trails always come with us
	for ( int i = 0; i < m_nSmokeTrailCount; i++ )
	{
		m_hSmokeTrail[i]->SetTransmit( pInfo, bAlways );
	}
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Hunt( void )
{
	if ( m_lifeState == LIFE_DEAD )
	{
		return;
	}

	if ( m_lifeState == LIFE_DYING )
	{
		Flight();
		UpdatePlayerDopplerShift( );
		return;
	}

	// FIXME: Hack to allow us to change the firing distance
	SetFarthestPathDist( GetMaxFiringDistance() );

	UpdateEnemy();

	// Give free knowledge of the enemy position if the chopper is "aggressive"
	if ( HasSpawnFlags( SF_HELICOPTER_AGGRESSIVE ) && GetEnemy() )
	{
		m_vecTargetPosition = GetEnemy()->WorldSpaceCenter();
	}

	// Test for state transitions when in bullrush mode
	if ( m_nAttackMode == ATTACK_MODE_BULLRUSH_VEHICLE )
	{
		UpdateBullrushState();
	}

	UpdateEnemyLeading();

	UpdateTrackNavigation( );

	Flight();

	UpdatePlayerDopplerShift( );

	FireWeapons();

	if ( !(m_fHelicopterFlags & BITS_HELICOPTER_GUN_ON) )
	{
		// !!!HACKHACK This is a fairly unsavoury hack that allows the attack
		// chopper to continue to carpet bomb even with the gun turned off
		// (Normally the chopper will carpet bomb inside FireGun(), but FireGun()
		// doesn't get called by the above call to FireWeapons() if the gun is turned off)
		// Adding this little exception here lets me avoid going into the CBaseHelicopter and
		// making some functions virtual that don't want to be virtual.
		if ( IsCarpetBombing() )
		{
			BullrushBombs();
		}
	}

#ifdef HL2_EPISODIC
	// Update our bone followers
	m_BoneFollowerManager.UpdateBoneFollowers(this);
#endif // HL2_EPISODIC
}

//-----------------------------------------------------------------------------
// Purpose: Cache whatever pose parameters we intend to use
//-----------------------------------------------------------------------------
void	CNPC_AttackHelicopter::PopulatePoseParameters( void )
{
	m_poseWeapon_Pitch = LookupPoseParameter("weapon_pitch");
	m_poseWeapon_Yaw = LookupPoseParameter("weapon_yaw");
	m_poseRudder = LookupPoseParameter("rudder");

	BaseClass::PopulatePoseParameters();
}

#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::InitBoneFollowers( void )
{
	// Don't do this if we're already loaded
	if ( m_BoneFollowerManager.GetNumBoneFollowers() != 0 )
		return;

	// Init our followers
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
}
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_helicopter, CNPC_AttackHelicopter )

//	DECLARE_TASK(  )

	DECLARE_ACTIVITY( ACT_HELICOPTER_DROP_BOMB );
	DECLARE_ACTIVITY( ACT_HELICOPTER_CRASHING );

//	DECLARE_CONDITION( COND_ )

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



//------------------------------------------------------------------------------
//
// A sensor used to drop bombs only in the correct points
//
//------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( npc_helicoptersensor, CBombDropSensor );

BEGIN_DATADESC( CBombDropSensor )

	DEFINE_INPUTFUNC( FIELD_VOID, "DropBomb", InputDropBomb ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DropBombStraightDown", InputDropBombStraightDown ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DropBombAtTargetAlways", InputDropBombAtTargetAlways ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DropBombAtTarget", InputDropBombAtTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "DropBombDelay", InputDropBombDelay ),

END_DATADESC()


void CBombDropSensor::Spawn()
{
	BaseClass::Spawn();
	UTIL_SetSize(this, Vector(-30,-30,-30), Vector(30,30,30) );
	SetSolid(SOLID_BBOX);

	// Shots pass through
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
}

// Drop a bomb at a particular location
void CBombDropSensor::InputDropBomb( inputdata_t &inputdata )
{
	inputdata_t myVersion = inputdata;
	myVersion.pActivator = this;
	assert_cast<CNPC_AttackHelicopter*>(GetOwnerEntity())->InputDropBomb( myVersion );
}

void CBombDropSensor::InputDropBombStraightDown( inputdata_t &inputdata )
{
	inputdata_t myVersion = inputdata;
	myVersion.pActivator = this;
	assert_cast<CNPC_AttackHelicopter*>(GetOwnerEntity())->InputDropBombStraightDown( myVersion );
}

void CBombDropSensor::InputDropBombAtTarget( inputdata_t &inputdata )
{
	inputdata_t myVersion = inputdata;
	myVersion.pActivator = this;
	assert_cast<CNPC_AttackHelicopter*>(GetOwnerEntity())->InputDropBombAtTarget( myVersion );
}

void CBombDropSensor::InputDropBombAtTargetAlways( inputdata_t &inputdata )
{
	inputdata_t myVersion = inputdata;
	myVersion.pActivator = this;
	assert_cast<CNPC_AttackHelicopter*>(GetOwnerEntity())->InputDropBombAtTargetAlways( myVersion );
}

void CBombDropSensor::InputDropBombDelay( inputdata_t &inputdata )
{
	inputdata_t myVersion = inputdata;
	myVersion.pActivator = this;
	assert_cast<CNPC_AttackHelicopter*>(GetOwnerEntity())->InputDropBombDelay( myVersion );
}

//------------------------------------------------------------------------------
//
// The bombs the helicopter drops on the player
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Save/load
//------------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( grenade_helicopter, CGrenadeHelicopter );

BEGIN_DATADESC( CGrenadeHelicopter )

	DEFINE_FIELD( m_bActivated,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExplodeOnContact,	FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_pWarnSound ),

	DEFINE_FIELD( m_hWarningSprite,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_bBlinkerAtTop,		FIELD_BOOLEAN ),

#ifdef HL2_EPISODIC
	DEFINE_FIELD( m_flLifetime,			FIELD_FLOAT ),
	DEFINE_FIELD( m_hCollisionObject,	FIELD_EHANDLE ),
	DEFINE_FIELD( m_bPickedUp,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flBlinkFastTime,	FIELD_TIME ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "ExplodeIn", InputExplodeIn ),

	DEFINE_OUTPUT( m_OnPhysGunOnlyPickup, "OnPhysGunOnlyPickup" ),
#endif // HL2_EPISODIC

	DEFINE_THINKFUNC( ExplodeThink ),
	DEFINE_THINKFUNC( AnimateThink ),
	DEFINE_THINKFUNC( RampSoundThink ),
	DEFINE_THINKFUNC( WarningBlinkerThink ),
	DEFINE_ENTITYFUNC( ExplodeConcussion ),

END_DATADESC()

#define SF_HELICOPTER_GRENADE_DUD	(1<<16)	// Will not become active on impact, only when launched via physcannon

//------------------------------------------------------------------------------
// Precache
//------------------------------------------------------------------------------
void CGrenadeHelicopter::Precache( void )
{
	BaseClass::Precache( );
	PrecacheModel( GRENADE_HELICOPTER_MODEL );

	PrecacheScriptSound( "ReallyLoudSpark" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.Ping" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.PingCaptured" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.HardImpact" );
}


//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CGrenadeHelicopter::Spawn( void )
{
	Precache();

	// point sized, solid, bouncing
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetModel( GRENADE_HELICOPTER_MODEL );

	if ( HasSpawnFlags( SF_HELICOPTER_GRENADE_DUD ) )
	{
		m_nSkin = (int)SKIN_DUD;
	}

	if ( !HasSpawnFlags( SF_GRENADE_HELICOPTER_MEGABOMB ) )
	{
		IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags(), false );
		SetMoveType( MOVETYPE_VPHYSICS );

		Vector vecAbsVelocity = GetAbsVelocity();
		pPhysicsObject->AddVelocity( &vecAbsVelocity, NULL );
	}
	else
	{
		SetSolid( SOLID_BBOX );
		SetCollisionBounds( Vector( -12.5, -12.5, -12.5 ), Vector( 12.5, 12.5, 12.5 ) );
		VPhysicsInitShadow( false, false );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		SetElasticity( 0.5f );
		AddEffects( EF_NOSHADOW );
	}

	// We're always being dropped beneath the helicopter; need to not
	// be affected by the rotor wash
	AddEFlags( EFL_NO_ROTORWASH_PUSH );

	// contact grenades arc lower
	QAngle angles;
	VectorAngles(GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	
	SetThink( NULL );
	
	// Tumble in air
	QAngle vecAngVel( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVel );
	
	// Explode on contact
	SetTouch( &CGrenadeHelicopter::ExplodeConcussion );

	// use a lower gravity for grenades to make them easier to see
	SetGravity( UTIL_ScaleForGravity( 400 ) );

#ifdef HL2_EPISODIC
	m_bPickedUp = false;
	m_flLifetime = BOMB_LIFETIME * 2.0;
#endif // HL2_EPISODIC

	if ( hl2_episodic.GetBool() )
	{
		// Disallow this, we'd rather deal with them as physobjects
		m_takedamage = DAMAGE_NO;
	}
	else
	{
		// Allow player to blow this puppy up in the air
		m_takedamage = DAMAGE_YES;
	}

	m_bActivated = false;
	m_pWarnSound = NULL;
	m_bExplodeOnContact = false;

	m_flDamage = sk_helicopter_grenadedamage.GetFloat();

	g_pNotify->AddEntity( this, this );

	if( hl2_episodic.GetBool() )
	{
		SetContextThink( &CGrenadeHelicopter::AnimateThink, gpGlobals->curtime, s_pAnimateThinkContext );
	}
}


//------------------------------------------------------------------------------
// On Remve
//------------------------------------------------------------------------------
void CGrenadeHelicopter::UpdateOnRemove()
{
	if( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pWarnSound );
	}
	g_pNotify->ClearEntity( this );
	BaseClass::UpdateOnRemove();
}


#ifdef HL2_EPISODIC
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CGrenadeHelicopter::InputExplodeIn( inputdata_t &inputdata )
{
	m_flLifetime = inputdata.value.Float();
	
	if ( HasSpawnFlags( SF_HELICOPTER_GRENADE_DUD ) )
	{
		// We are a dud no more!
		RemoveSpawnFlags( SF_HELICOPTER_GRENADE_DUD );
		m_nSkin = (int)SKIN_REGULAR;
	}

	m_bActivated = false;
	BecomeActive();
}
#endif


//------------------------------------------------------------------------------
// Activate!
//------------------------------------------------------------------------------
void CGrenadeHelicopter::BecomeActive()
{
	if ( m_bActivated )
		return;

	if ( IsMarkedForDeletion() )
		return;

	m_bActivated = true;

	bool bMegaBomb = HasSpawnFlags(SF_GRENADE_HELICOPTER_MEGABOMB);

	SetThink( &CGrenadeHelicopter::ExplodeThink );
	
	if ( hl2_episodic.GetBool() )
	{
		if ( HasSpawnFlags( SF_HELICOPTER_GRENADE_DUD ) == false )
		{
			SetNextThink( gpGlobals->curtime + GetBombLifetime() );
		}
		else
		{
			// NOTE: A dud will not explode after a set time, only when launched!
			SetThink( NULL );
			return;
		}
	}
	else
	{
		SetNextThink( gpGlobals->curtime + GetBombLifetime() );
	}

	if ( !bMegaBomb )
	{
		SetContextThink( &CGrenadeHelicopter::RampSoundThink, gpGlobals->curtime + GetBombLifetime() - BOMB_RAMP_SOUND_TIME, s_pRampSoundContext );

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CReliableBroadcastRecipientFilter filter;
		m_pWarnSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopterGrenade.Ping" );
		controller.Play( m_pWarnSound, 1.0, PITCH_NORM );
	}

	SetContextThink( &CGrenadeHelicopter::WarningBlinkerThink, gpGlobals->curtime + (GetBombLifetime() - 2.0f), s_pWarningBlinkerContext );

#ifdef HL2_EPISODIC
	m_flBlinkFastTime = gpGlobals->curtime + GetBombLifetime() - 1.0f;
#endif//HL2_EPISODIC
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CGrenadeHelicopter::RampSoundThink( )
{
	if ( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pWarnSound, 140, BOMB_RAMP_SOUND_TIME );
	}

	SetContextThink( NULL, gpGlobals->curtime, s_pRampSoundContext );
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CGrenadeHelicopter::WarningBlinkerThink()
{
#ifndef HL2_EPISODIC
	return;
#endif

/*
	if( !m_hWarningSprite.Get() )
	{
		Vector up;
		GetVectors( NULL, NULL, &up );

		// Light isn't on, so create the sprite.
		m_hWarningSprite = CSprite::SpriteCreate( "sprites/redglow1.vmt", GetAbsOrigin() + up * 10.0f, false );
		CSprite *pSprite = (CSprite *)m_hWarningSprite.Get();

		if( pSprite != NULL )
		{
			pSprite->SetParent( this, LookupAttachment("top") );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
			pSprite->SetScale( 0.35, 0.0 );
		}

		m_bBlinkerAtTop = true;

		ResetSequence( LookupActivity( "ACT_ARM" ) );
	}
	else
*/
	{
		// Just flip it to the other attachment.
		if( m_bBlinkerAtTop )
		{
			//m_hWarningSprite->SetParentAttachment( "SetParentAttachment", "bottom", false );
			m_nSkin = (int)SKIN_REGULAR;
			m_bBlinkerAtTop = false;
		}
		else
		{
			//m_hWarningSprite->SetParentAttachment( "SetParentAttachment", "top", false );
			m_nSkin = (int)SKIN_DUD;
			m_bBlinkerAtTop = true;
		}
	}

	// Frighten people
	CSoundEnt::InsertSound ( SOUND_DANGER, WorldSpaceCenter(), g_helicopter_bomb_danger_radius.GetFloat(), 0.2f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );

#ifdef HL2_EPISODIC
	if( gpGlobals->curtime >= m_flBlinkFastTime )
	{
		SetContextThink( &CGrenadeHelicopter::WarningBlinkerThink, gpGlobals->curtime + 0.1f, s_pWarningBlinkerContext );
	}
	else
	{
		SetContextThink( &CGrenadeHelicopter::WarningBlinkerThink, gpGlobals->curtime + 0.2f, s_pWarningBlinkerContext );
	}
#endif//HL2_EPISODIC
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CGrenadeHelicopter::StopWarningBlinker()
{
	if( m_hWarningSprite.Get() )
	{
		UTIL_Remove( m_hWarningSprite.Get() );
		m_hWarningSprite.Set( NULL );
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CGrenadeHelicopter::AnimateThink()
{
	StudioFrameAdvance();
	SetContextThink( &CGrenadeHelicopter::AnimateThink, gpGlobals->curtime + 0.1f, s_pAnimateThinkContext );
}

//------------------------------------------------------------------------------
// Entity events... these are events targetted to a particular entity
//------------------------------------------------------------------------------
void CGrenadeHelicopter::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	BaseClass::OnEntityEvent( event, pEventData );

	if ( event == ENTITY_EVENT_WATER_TOUCH )
	{
		BecomeActive();
	}
}


//------------------------------------------------------------------------------
// If we hit water, then stop
//------------------------------------------------------------------------------
void CGrenadeHelicopter::PhysicsSimulate( void )
{
	Vector vecPrevPosition = GetAbsOrigin();
	
	BaseClass::PhysicsSimulate();

	if (!m_bActivated && (GetMoveType() != MOVETYPE_VPHYSICS))
	{
		if ( GetWaterLevel() > 1 )
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			BecomeActive();
		}

		// Stuck condition, can happen pretty often
		if ( vecPrevPosition == GetAbsOrigin() )
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			BecomeActive();
		}
	}
}


//------------------------------------------------------------------------------
// If we hit something, start the timer
//------------------------------------------------------------------------------
void CGrenadeHelicopter::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );
	BecomeActive();

#ifndef HL2_EPISODIC	// in ep2, don't do this here, do it in Touch()
	if ( m_bExplodeOnContact )
	{
		Vector vecVelocity;
		GetVelocity( &vecVelocity, NULL );
		DoExplosion( GetAbsOrigin(), vecVelocity );
	}
#endif
	

	if( hl2_episodic.GetBool() )
	{
		float flImpactSpeed = pEvent->preVelocity->Length();
		if( flImpactSpeed > 400.0f && pEvent->pEntities[ 1 ]->IsWorld() )
		{
			EmitSound( "NPC_AttackHelicopterGrenade.HardImpact" );
		}
	}
}


#if HL2_EPISODIC
//------------------------------------------------------------------------------
// double launch velocity for ep2_outland_08
//------------------------------------------------------------------------------
Vector CGrenadeHelicopter::PhysGunLaunchVelocity( const Vector &forward, float flMass )
{
	// return ( striderbuster_shot_velocity.GetFloat() * forward );

	return BaseClass::PhysGunLaunchVelocity(forward,flMass) * sk_helicopter_grenaderadius.GetFloat();
}
#endif


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
float CGrenadeHelicopter::GetBombLifetime()
{
#if HL2_EPISODIC
	return m_flLifetime;
#else
	return BOMB_LIFETIME;
#endif
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
int CGrenadeHelicopter::OnTakeDamage( const CTakeDamageInfo &info )
{
	// We don't take blast damage
	if ( info.GetDamageType() & DMG_BLAST )
		return 0;

	return BaseClass::OnTakeDamage( info );
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CGrenadeHelicopter::DoExplosion( const Vector &vecOrigin, const Vector &vecVelocity )
{
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity() ? GetOwnerEntity() : this, sk_helicopter_grenadedamage.GetFloat(), 
		sk_helicopter_grenaderadius.GetFloat(), (SF_ENVEXPLOSION_NOSPARKS|SF_ENVEXPLOSION_NODLIGHTS|SF_ENVEXPLOSION_NODECAL|SF_ENVEXPLOSION_NOFIREBALL|SF_ENVEXPLOSION_NOPARTICLES), 
		sk_helicopter_grenadeforce.GetFloat(), this );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}

	CEffectData data;

	// If we're under water do a water explosion
	if ( GetWaterLevel() != 0 && (GetWaterType() & CONTENTS_WATER) )
	{
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = 128;
		data.m_flScale = 128;
		data.m_fFlags = 0;
		DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		// Otherwise do a normal explosion
		data.m_vOrigin = GetAbsOrigin();
		DispatchEffect( "HelicopterMegaBomb", data );
	}

	UTIL_Remove( this );
}


//------------------------------------------------------------------------------
// I think I Pow!
//------------------------------------------------------------------------------
void CGrenadeHelicopter::ExplodeThink(void)
{
#ifdef HL2_EPISODIC
	// remember if we were thrown by player, we can only determine this prior to explosion
	bool bIsThrownByPlayer = IsThrownByPlayer();
	int iHealthBefore = 0;
	// get the health of the helicopter we came from prior to our explosion
	CNPC_AttackHelicopter *pOwner = dynamic_cast<CNPC_AttackHelicopter *>( GetOriginalThrower() );
	if ( pOwner )
	{
		iHealthBefore = pOwner->GetHealth();
	}
#endif // HL2_EPISODIC

	Vector vecVelocity;
	GetVelocity( &vecVelocity, NULL );
	DoExplosion( GetAbsOrigin(), vecVelocity );

#ifdef HL2_EPISODIC
	// if we were thrown by player, look at health of helicopter after explosion and determine if we damaged it
	if ( bIsThrownByPlayer && pOwner && ( iHealthBefore > 0 ) )
	{
		int iHealthAfter = pOwner->GetHealth();
		if ( iHealthAfter == iHealthBefore )
		{
			// The player threw us, we exploded due to timer, and we did not damage the helicopter that fired us.  Send a miss event
			SendMissEvent();
		}
	}
#endif // HL2_EPISODIC

}


//------------------------------------------------------------------------------
// I think I Pow!
//------------------------------------------------------------------------------
void CGrenadeHelicopter::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	ResolveFlyCollisionBounce( trace, vecVelocity, 0.1f );
}


//------------------------------------------------------------------------------
// Contact grenade, explode when it touches something
//------------------------------------------------------------------------------
void CGrenadeHelicopter::ExplodeConcussion( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;

	if ( !m_bExplodeOnContact )
	{
		if ( pOther->IsWorld() )
			return;

		if ( hl2_episodic.GetBool() )
		{
			// Don't hit anything other than vehicles
			if ( pOther->GetCollisionGroup() != COLLISION_GROUP_VEHICLE )
				return;
		}
	}

#ifdef HL2_EPISODIC
	CBaseEntity *pEntityHit = pOther;
	if ( pEntityHit->ClassMatches( "phys_bone_follower" ) && pEntityHit->GetOwnerEntity() )
	{
		pEntityHit = pEntityHit->GetOwnerEntity();
	}
	if ( ( CLASS_COMBINE_GUNSHIP != pEntityHit->Classify() ) || !pEntityHit->ClassMatches( "npc_helicopter" ) )
	{
		// We hit something other than a helicopter.  If the player threw us, send a miss event
		if ( IsThrownByPlayer() )
		{
			SendMissEvent();
		}		
	}
#endif // HL2_EPISODIC

	Vector vecVelocity;
	GetVelocity( &vecVelocity, NULL );
	DoExplosion( GetAbsOrigin(), vecVelocity );
}


#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: The bomb will act differently when picked up by the player
//-----------------------------------------------------------------------------
void CGrenadeHelicopter::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	if ( reason == PICKED_UP_BY_CANNON )
	{
		if ( !m_bPickedUp )
		{
			if( m_hWarningSprite.Get() != NULL )
			{
				UTIL_Remove( m_hWarningSprite );
				m_hWarningSprite.Set(NULL);
			}

			// Turn on
			BecomeActive();

			// Change the warning sound to a captured sound.
			SetContextThink( &CGrenadeHelicopter::RampSoundThink, gpGlobals->curtime + GetBombLifetime() - BOMB_RAMP_SOUND_TIME, s_pRampSoundContext );

			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundDestroy( m_pWarnSound );

			CReliableBroadcastRecipientFilter filter;
			m_pWarnSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopterGrenade.PingCaptured" );
			controller.Play( m_pWarnSound, 1.0, PITCH_NORM );

			// Reset our counter so the player has more time
			SetThink( &CGrenadeHelicopter::ExplodeThink );
			SetNextThink( gpGlobals->curtime + GetBombLifetime() );

			SetContextThink( &CGrenadeHelicopter::WarningBlinkerThink, gpGlobals->curtime + GetBombLifetime() - 2.0f, s_pWarningBlinkerContext );

#ifdef HL2_EPISODIC
			m_nSkin = (int)SKIN_REGULAR;
			m_flBlinkFastTime = gpGlobals->curtime + GetBombLifetime() - 1.0f;
#endif//HL2_EPISODIC
			
			// Stop us from sparing damage to the helicopter that dropped us
			SetOwnerEntity( pPhysGunUser );
			PhysEnableEntityCollisions( this, m_hCollisionObject );

			// Don't do this again!
			m_bPickedUp = true;

			m_OnPhysGunOnlyPickup.FireOutput( pPhysGunUser, this );
		}
	}

	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHelicopter::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason )
{
	if ( reason == LAUNCHED_BY_CANNON )
	{
		// Enable world touches.
		unsigned int flags = VPhysicsGetObject()->GetCallbackFlags();
		VPhysicsGetObject()->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );

		// Explode on contact
		SetTouch( &CGrenadeHelicopter::ExplodeConcussion );
		m_bExplodeOnContact = true;

	}

	BaseClass::OnPhysGunDrop( pPhysGunUser, reason );
}

//-----------------------------------------------------------------------------
// Purpose: Returns if the player threw this grenade w/phys gun
//-----------------------------------------------------------------------------
bool CGrenadeHelicopter::IsThrownByPlayer()
{
	// if player is the owner and we're set to explode on contact, then the player threw this grenade.
	return ( ( GetOwnerEntity() == UTIL_GetLocalPlayer() ) && m_bExplodeOnContact );
}

//-----------------------------------------------------------------------------
// Purpose: If player threw this grenade, sends a miss event
//-----------------------------------------------------------------------------
void CGrenadeHelicopter::SendMissEvent()
{
	// send a miss event
	IGameEvent *event = gameeventmanager->CreateEvent( "helicopter_grenade_punt_miss" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
//
// This entity is used to create little force spheres that the helicopters should avoid. 
//
//-----------------------------------------------------------------------------
CUtlVector< CAvoidSphere::AvoidSphereHandle_t > CAvoidSphere::s_AvoidSpheres; 

#define SF_AVOIDSPHERE_AVOID_BELOW	0x00010000

LINK_ENTITY_TO_CLASS( npc_heli_avoidsphere, CAvoidSphere );

BEGIN_DATADESC( CAvoidSphere )

	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Creates an avoidance sphere
//-----------------------------------------------------------------------------
CBaseEntity *CreateHelicopterAvoidanceSphere( CBaseEntity *pParent, int nAttachment, float flRadius, bool bAvoidBelow )
{
	CAvoidSphere *pSphere = static_cast<CAvoidSphere*>(CreateEntityByName( "npc_heli_avoidsphere" ));
	pSphere->Init( flRadius );
	if ( bAvoidBelow )
	{
		pSphere->AddSpawnFlags( SF_AVOIDSPHERE_AVOID_BELOW );
	}
	pSphere->Spawn();
	pSphere->SetParent( pParent, nAttachment );
	pSphere->SetLocalOrigin( vec3_origin );
	pSphere->SetLocalAngles( vec3_angle );
	pSphere->SetOwnerEntity( pParent );
	return pSphere;
}


//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------
void CAvoidSphere::Init( float flRadius )
{
	m_flRadius = flRadius;
}


//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CAvoidSphere::Activate( )
{
	BaseClass::Activate();
	s_AvoidSpheres.AddToTail( this );
}

void CAvoidSphere::UpdateOnRemove( )
{
	s_AvoidSpheres.FindAndRemove( this );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
void CAvoidSphere::ComputeAvoidanceForces( CBaseEntity *pEntity, float flEntityRadius, 
										  float flAvoidTime, Vector *pVecAvoidForce )
{
	pVecAvoidForce->Init( );

	Vector vecEntityDelta;
	VectorMultiply( pEntity->GetAbsVelocity(), flAvoidTime, vecEntityDelta );
	Vector vecEntityCenter = pEntity->WorldSpaceCenter();

	for ( int i = s_AvoidSpheres.Count(); --i >= 0; )
	{
		CAvoidSphere *pSphere = s_AvoidSpheres[i].Get();
		const Vector &vecAvoidCenter = pSphere->WorldSpaceCenter();

		// NOTE: This test can be thought of sweeping a sphere through space
		// and seeing if it intersects the avoidance sphere
		float flTotalRadius = flEntityRadius + pSphere->m_flRadius;
		float t1, t2;
		if ( !IntersectRayWithSphere( vecEntityCenter, vecEntityDelta, 
				vecAvoidCenter, flTotalRadius, &t1, &t2 ) )
		{
			continue;
		}

		// NOTE: The point of closest approach is at the average t value
		Vector vecClosestApproach;
		float flAverageT = (t1 + t2) * 0.5f;
		VectorMA( vecEntityCenter, flAverageT, vecEntityDelta, vecClosestApproach );

		// Add velocity to make it be pushed out away from the sphere center
		// without totally counteracting its velocity.
		Vector vecDir;
		VectorSubtract( vecClosestApproach, vecAvoidCenter, vecDir );
		float flZDist = vecDir.z;
		float flDist = VectorNormalize( vecDir );
		float flDistToTravel;
		if ( flDist < 0.01f )
		{
			flDist = 0.01f;
			vecDir.Init( 0, 0, 1 );
			flDistToTravel = flTotalRadius;
		}
		else
		{
			// make the chopper always avoid *above*
			// That means if a force would be applied to push the chopper down,
			// figure out a new distance to travel that would push the chopper up.
			if ( flZDist < 0.0f && !pSphere->HasSpawnFlags(SF_AVOIDSPHERE_AVOID_BELOW) )
			{
				Vector vecExitPoint;
				vecDir.z = -vecDir.z;
				VectorMA( vecAvoidCenter, flTotalRadius, vecDir, vecExitPoint );
				VectorSubtract( vecExitPoint, vecClosestApproach, vecDir );
				flDistToTravel = VectorNormalize( vecDir );
			}
			else
			{
				Assert( flDist <= flTotalRadius );
				flDistToTravel = flTotalRadius - flDist;
			}
		}

		// The actual force amount is easy to think about:
		// We need to change the position by dx over a time dt, so dv = dx/dt
		// But so it doesn't explode, lets clamp t1 to a not-unreasonable time
		if ( t1 < 0.25f )
		{
			t1 = 0.25f;
		}

		float flForce = 1.25f * flDistToTravel / t1;
		vecDir *= flForce;

		*pVecAvoidForce += vecDir;
	}
}


//-----------------------------------------------------------------------------
//
// This entity is used to create little force boxes that the helicopters should avoid. 
//
//-----------------------------------------------------------------------------
CUtlVector< CAvoidBox::AvoidBoxHandle_t > CAvoidBox::s_AvoidBoxes; 

#define SF_AVOIDBOX_AVOID_BELOW	0x00010000

LINK_ENTITY_TO_CLASS( npc_heli_avoidbox, CAvoidBox );

BEGIN_DATADESC( CAvoidBox )
END_DATADESC()


//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CAvoidBox::Spawn( )
{
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}

void CAvoidBox::Activate( )
{
	BaseClass::Activate();
	s_AvoidBoxes.AddToTail( this );
}

void CAvoidBox::UpdateOnRemove( )
{
	s_AvoidBoxes.FindAndRemove( this );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
void CAvoidBox::ComputeAvoidanceForces( CBaseEntity *pEntity, float flEntityRadius, float flAvoidTime, Vector *pVecAvoidForce )
{
	pVecAvoidForce->Init( );

	Vector vecEntityDelta, vecEntityEnd;
	VectorMultiply( pEntity->GetAbsVelocity(), flAvoidTime, vecEntityDelta );
	Vector vecEntityCenter = pEntity->WorldSpaceCenter();
	VectorAdd( vecEntityCenter, vecEntityDelta, vecEntityEnd );

	Vector vecVelDir = pEntity->GetAbsVelocity();
	VectorNormalize( vecVelDir );

	for ( int i = s_AvoidBoxes.Count(); --i >= 0; )
	{
		CAvoidBox *pBox = s_AvoidBoxes[i].Get();

		const Vector &vecAvoidCenter = pBox->WorldSpaceCenter();

		// NOTE: This test can be thought of sweeping a sphere through space
		// and seeing if it intersects the avoidance box
		float flTotalRadius = flEntityRadius + pBox->BoundingRadius();
		float t1, t2;
		if ( !IntersectInfiniteRayWithSphere( vecEntityCenter, vecEntityDelta, 
				vecAvoidCenter, flTotalRadius, &t1, &t2 ) )
		{
			continue;
		}

		if (( t2 < 0.0f ) || ( t1 > 1.0f ))
			continue;

		// Unlike the avoid spheres, we also need to make sure the ray intersects the box
		Vector vecLocalCenter, vecLocalDelta;
		pBox->CollisionProp()->WorldToCollisionSpace( vecEntityCenter, &vecLocalCenter );
		pBox->CollisionProp()->WorldDirectionToCollisionSpace( vecEntityDelta, &vecLocalDelta );

		Vector vecBoxMin( -flEntityRadius, -flEntityRadius, -flEntityRadius );
		Vector vecBoxMax( flEntityRadius, flEntityRadius, flEntityRadius );
		vecBoxMin += pBox->CollisionProp()->OBBMins();
		vecBoxMax += pBox->CollisionProp()->OBBMaxs();

		trace_t tr;
		if ( !IntersectRayWithBox( vecLocalCenter, vecLocalDelta, vecBoxMin, vecBoxMax, 0.0f, &tr ) )
			continue;

		// NOTE: The point of closest approach is at the average t value
		Vector vecClosestApproach;
		float flAverageT = (t1 + t2) * 0.5f;
		VectorMA( vecEntityCenter, flAverageT, vecEntityDelta, vecClosestApproach );
		
		// Add velocity to make it be pushed out away from the sphere center
		// without totally counteracting its velocity.
		Vector vecDir;
		VectorSubtract( vecClosestApproach, vecAvoidCenter, vecDir );

		// Limit unnecessary sideways motion
		if ( ( tr.plane.type != 3 ) || ( tr.plane.normal[2] > 0.0f ) )
		{
			vecDir.x *= 0.1f;
			vecDir.y *= 0.1f;
		}

		float flZDist = vecDir.z;
		float flDist = VectorNormalize( vecDir );
		float flDistToTravel;
		if ( flDist < 10.0f )
		{
			flDist = 10.0f;
			vecDir.Init( 0, 0, 1 );
			flDistToTravel = flTotalRadius;
		}
		else
		{
			// make the chopper always avoid *above*
			// That means if a force would be applied to push the chopper down,
			// figure out a new distance to travel that would push the chopper up.
			if ( flZDist < 0.0f && !pBox->HasSpawnFlags(SF_AVOIDSPHERE_AVOID_BELOW) )
			{
				Vector vecExitPoint;
				vecDir.z = -vecDir.z;
				VectorMA( vecAvoidCenter, flTotalRadius, vecDir, vecExitPoint );
				VectorSubtract( vecExitPoint, vecClosestApproach, vecDir );
				flDistToTravel = VectorNormalize( vecDir );
			}
			else
			{
				Assert( flDist <= flTotalRadius );
				flDistToTravel = flTotalRadius - flDist;
			}
		}

		// The actual force amount is easy to think about:
		// We need to change the position by dx over a time dt, so dv = dx/dt
		// But so it doesn't explode, lets clamp t1 to a not-unreasonable time
		if ( t1 < 0.25f )
		{
			t1 = 0.25f;
		}

		float flForce = 1.5f * flDistToTravel / t1;
		vecDir *= flForce;

		*pVecAvoidForce += vecDir;
	}
}


//-----------------------------------------------------------------------------
//
// This entity is used to create little force boxes that the helicopters should avoid. 
//
//-----------------------------------------------------------------------------
CUtlVector< CBombSuppressor::BombSuppressorHandle_t > CBombSuppressor::s_BombSuppressors; 

LINK_ENTITY_TO_CLASS( npc_heli_nobomb, CBombSuppressor );

BEGIN_DATADESC( CBombSuppressor )
END_DATADESC()


//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CBombSuppressor::Spawn( )
{
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}

void CBombSuppressor::Activate( )
{
	BaseClass::Activate();
	s_BombSuppressors.AddToTail( this );
}

void CBombSuppressor::UpdateOnRemove( )
{
	s_BombSuppressors.FindAndRemove( this );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
bool CBombSuppressor::CanBomb( const Vector &vecPosition )
{
	for ( int i = s_BombSuppressors.Count(); --i >= 0; )
	{
		CBombSuppressor *pBox = s_BombSuppressors[i].Get();
		if ( pBox->CollisionProp()->IsPointInBounds( vecPosition ) )
			return false;
	}

	return true;
}

LINK_ENTITY_TO_CLASS( helicopter_chunk, CHelicopterChunk );

BEGIN_DATADESC( CHelicopterChunk )

	DEFINE_THINKFUNC( FallThink ),

	DEFINE_FIELD( m_bLanded, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hMaster, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nChunkID, FIELD_INTEGER ),
	DEFINE_PHYSPTR( m_pTailConstraint ),
	DEFINE_PHYSPTR( m_pCockpitConstraint ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHelicopterChunk::Spawn( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHelicopterChunk::FallThink( void )
{
	if ( m_bLanded )
	{
		SetThink( NULL );
		return;
	}
	
	if ( random->RandomInt( 0, 8 ) == 0 )
	{
		CEffectData data;
		data.m_vOrigin = GetAbsOrigin() + RandomVector( -64, 64 );
		DispatchEffect( "HelicopterMegaBomb", data );

		EmitSound( "BaseExplosionEffect.Sound" );
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CHelicopterChunk::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	if ( m_bLanded == false )
	{
		int otherIndex = !index;
		CBaseEntity *pOther = pEvent->pEntities[otherIndex];
		if ( !pOther )
			return;
		
		if ( pOther->IsWorld() )
		{		
			CollisionCallback( this );

			m_bLanded = true;
			SetThink( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pCaller - 
//-----------------------------------------------------------------------------
void CHelicopterChunk::CollisionCallback( CHelicopterChunk *pCaller )
{
	if ( m_bLanded )
		return;

	if ( m_hMaster != NULL )
	{
		m_hMaster->CollisionCallback( this );
	}
	else
	{
		// Break our other constraints
		if ( m_pTailConstraint )
		{
			physenv->DestroyConstraint( m_pTailConstraint );
			m_pTailConstraint = NULL;
		}
		
		if ( m_pCockpitConstraint )
		{
			physenv->DestroyConstraint( m_pCockpitConstraint );
			m_pCockpitConstraint = NULL;
		}
		
		// Add a dust cloud
		AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( GetAbsOrigin() );

		if ( pExplosion != NULL )
		{
			pExplosion->SetLifetime( 10 );
		}

		// Make a loud noise
		EmitSound( "NPC_AttackHelicopter.Crash" );

		m_bLanded = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPos - 
//			&vecAngles - 
//			&vecVelocity - 
//			*pszModelName - 
// Output : CHelicopterChunk
//-----------------------------------------------------------------------------
CHelicopterChunk *CHelicopterChunk::CreateHelicopterChunk( const Vector &vecPos, const QAngle &vecAngles, const Vector &vecVelocity, const char *pszModelName, int chunkID )
{
	// Drop a flaming, smoking chunk.
	CHelicopterChunk *pChunk = CREATE_ENTITY( CHelicopterChunk, "helicopter_chunk" );
	
	if ( pChunk == NULL )
		return NULL;

	pChunk->Spawn();

	pChunk->SetAbsOrigin( vecPos );
	pChunk->SetAbsAngles( vecAngles );

	pChunk->SetModel( pszModelName );

	pChunk->m_nChunkID = chunkID;
	pChunk->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE );

	IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
	
	// Set the velocity
	if ( pPhysicsObject )
	{
		pPhysicsObject->EnableMotion( true );
		Vector vecChunkVelocity;
		AngularImpulse angImpulse;

		vecChunkVelocity = vecVelocity;
		angImpulse = vec3_origin;

		pPhysicsObject->SetVelocity(&vecChunkVelocity, &angImpulse );
	}
	
	pChunk->SetThink( &CHelicopterChunk::FallThink );
	pChunk->SetNextThink( gpGlobals->curtime + 0.1f );

	pChunk->m_bLanded = false;

	SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
	pSmokeTrail->FollowEntity( pChunk, "damage" );

	pSmokeTrail->m_SpawnRate = 4;
	pSmokeTrail->m_ParticleLifetime	= 2.0f;

	pSmokeTrail->m_StartColor.Init( 0.7f, 0.7f, 0.7f );
	pSmokeTrail->m_EndColor.Init( 0.6, 0.6, 0.6 );

	pSmokeTrail->m_StartSize	= 32;
	pSmokeTrail->m_EndSize	= 64;
	pSmokeTrail->m_SpawnRadius= 8;
	pSmokeTrail->m_MinSpeed	= 0;
	pSmokeTrail->m_MaxSpeed	= 8;
	pSmokeTrail->m_Opacity	= 0.35f;

	CFireTrail *pFireTrail = CFireTrail::CreateFireTrail();

	if ( pFireTrail == NULL )
		return pChunk;

	pFireTrail->FollowEntity( pChunk, "damage" );
	pFireTrail->SetParent( pChunk, 1 );
	pFireTrail->SetLocalOrigin( vec3_origin );
	pFireTrail->SetMoveType( MOVETYPE_NONE );
	pFireTrail->SetLifetime( 10.0f );

	return pChunk;
}
