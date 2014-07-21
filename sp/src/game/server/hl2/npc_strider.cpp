//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Giant walking strider thing!
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "npc_strider.h"

#include "ai_senses.h"
#include "ai_task.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_basenpc.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"
#include "ai_link.h"
#include "ai_hint.h"
#include "ai_tacticalservices.h"
#include "ai_behavior_follow.h"
#include "simtimer.h"
#include "trains.h"
#include "npcevent.h"
#include "te_particlesystem.h"
#include "shake.h"
#include "soundent.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "studio.h"
#include "physics_bone_follower.h"
#include "ai_navigator.h"
#include "ai_route.h"
#include "ammodef.h"
#include "npc_bullseye.h"
#include "rope.h"
#include "ai_memory.h"
#include "player_pickup.h"
#include "collisionutils.h"
#include "in_buttons.h"
#include "steamjet.h"
#include "physics_prop_ragdoll.h"
#include "vehicle_base.h"
#include "coordsize.h"
#include "hl2_shareddefs.h"
#include "te_effect_dispatch.h"
#include "beam_flags.h"
#include "prop_combine_ball.h"
#include "explode.h"
#include "filters.h"
#include "saverestore_utlvector.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_interactionPlayerLaunchedRPG = 0;

// Changing this classname avoids bugs where ai_relationship entities that change the
// strider's relationship with bullseyes would affect its relationship with the focus
LINK_ENTITY_TO_CLASS( bullseye_strider_focus, CNPC_Bullseye );


//-----------------------------------------------------------------------------

ConVar strider_immolate( "strider_immolate", "0" );
ConVar sk_strider_health( "sk_strider_health", "350" );
ConVar npc_strider_height_adj("npc_strider_height_adj", "0" );
ConVar strider_eyepositions( "strider_eyepositions", "0" );
ConVar strider_show_focus( "strider_show_focus", "0" );
ConVar strider_distributed_fire( "strider_distributed_fire", "1" );
ConVar strider_show_cannonlos( "strider_show_cannonlos", "0" );
ConVar strider_show_weapon_los_z( "strider_show_weapon_los_z", "0" );
ConVar strider_show_weapon_los_condition( "strider_show_weapon_los_condition", "0" );
ConVar strider_idle_test( "strider_idle_test", "0" );
ConVar strider_always_use_procedural_height( "strider_always_use_procedural_height", "0" );
ConVar strider_test_height( "strider_test_height", "0" );
ConVar strider_pct_height_no_crouch_move( "strider_pct_height_no_crouch_move", "90" );

ConVar strider_peek_time( "strider_peek_time", "0.75" );
ConVar strider_peek_time_after_damage( "strider_peek_time_after_damage", "4.0" );
ConVar strider_peek_eye_dist( "strider_peek_eye_dist", "1.75" );
ConVar strider_peek_eye_dist_z( "strider_peek_eye_dist_z", "4.0" );
ConVar strider_free_pass_start_time( "strider_free_pass_start_time", "3" );
ConVar strider_free_pass_cover_dist( "strider_free_pass_cover_dist", "120" );
ConVar strider_free_pass_duration( "strider_free_pass_duration", "2" );
ConVar strider_free_pass_refill_rate( "strider_free_pass_refill_rate", "0.5" );
ConVar strider_free_pass_move_tolerance( "strider_free_pass_move_tolerance", "320" );
ConVar strider_free_knowledge( "strider_free_knowledge", "0.5" );

ConVar strider_free_pass_after_escorts_dead( "strider_free_pass_after_escorts_dead", "2.5" );
ConVar strider_free_pass_tolerance_after_escorts_dead( "strider_free_pass_tolerance_after_escorts_dead", "600" );

ConVar npc_strider_shake_ropes_radius( "npc_strider_shake_ropes_radius", "1200" );
ConVar npc_strider_shake_ropes_magnitude( "npc_strider_shake_ropes_magnitude", "150" );

ConVar strider_ar2_altfire_dmg( "strider_ar2_altfire_dmg", "25" );

// Number of RPG hits it takes to kill a strider on each skill level.
ConVar sk_strider_num_missiles1("sk_strider_num_missiles1", "5");
ConVar sk_strider_num_missiles2("sk_strider_num_missiles2", "7");
ConVar sk_strider_num_missiles3("sk_strider_num_missiles3", "7");

ConVar strider_missile_suppress_dist( "strider_missile_suppress_dist", "240" );
ConVar strider_missile_suppress_time( "strider_missile_suppress_time", "3" );


//-----------------------------------------------------------------------------

float GetCurrentGravity( void );

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

//-----------------------------------------------------------------------------

enum bodygroups
{
	STRIDER_BODYGROUP_VENT = 1,
};

//-----------------------------------------------------------------------------

#define STRIDER_DEFAULT_SHOOT_DURATION			2.5 // spend this much time stitching to each target.
#define STRIDER_SHOOT_ON_TARGET_TIME			0.5 // How much of DEFAULT_SHOOT_DURATION is spent on-target (vs. stitching up to a target)
#define STRIDER_SHOOT_VARIATION					1.0 // up to 1 second of variance
#define STRIDER_SHOOT_DOWNTIME					1.0 // This much downtime between bursts
#define STRIDER_SUBSEQUENT_TARGET_DURATION		1.5 // Spend this much time stitching to targets chosen by distributed fire.
#define STRIDER_IGNORE_TARGET_DURATION			1.0
#define STRIDER_IGNORE_PLAYER_DURATION			1.5
#define STRIDER_DEFAULT_RATE_OF_FIRE			5	// Rounds per second

#define STRIDER_EP1_RATE_OF_FIRE			10.0f
#define STRIDER_EP1_SHOOT_ON_TARGET_TIME	 0.3f
#define STRIDER_EP1_SHOOT_DURATION			 1.1f
#define STRIDER_EP1_SHOOT_DOWNTIME			 1.0f
#define STRIDER_EP1_SHOOT_VARIATION			 0.3f

//Animation events
#define STRIDER_AE_FOOTSTEP_LEFT		1
#define STRIDER_AE_FOOTSTEP_RIGHT		2
#define STRIDER_AE_FOOTSTEP_BACK		3
#define STRIDER_AE_FOOTSTEP_LEFTM		4
#define STRIDER_AE_FOOTSTEP_RIGHTM		5
#define STRIDER_AE_FOOTSTEP_BACKM		6
#define STRIDER_AE_FOOTSTEP_LEFTL		7
#define STRIDER_AE_FOOTSTEP_RIGHTL		8
#define STRIDER_AE_FOOTSTEP_BACKL		9
#define STRIDER_AE_WHOOSH_LEFT			11
#define STRIDER_AE_WHOOSH_RIGHT			12
#define STRIDER_AE_WHOOSH_BACK			13
#define STRIDER_AE_CREAK_LEFT			21
#define STRIDER_AE_CREAK_RIGHT			22
#define STRIDER_AE_CREAK_BACK			23
#define STRIDER_AE_SHOOTCANNON			100
#define STRIDER_AE_CANNONHIT			101
#define STRIDER_AE_SHOOTMINIGUN			105
#define STRIDER_AE_STOMPHITL			110
#define STRIDER_AE_STOMPHITR			111
#define STRIDER_AE_FLICKL				112
#define STRIDER_AE_FLICKR				113
#define STRIDER_AE_WINDUPCANNON			114

#define STRIDER_AE_DIE					999

// UNDONE: Share properly with the client code!!!
#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3
#define STOMP_IK_SLOT					11

// can hit anything within this range
#define STRIDER_STOMP_RANGE				260

// Crouch down if trying to shoot an enemy that's this close
#define STRIDER_CROUCH_RANGE			4000.0f

// Stand up again if crouched and engaging an enemy at this distance
#define STRIDER_STAND_RANGE				6000.0f

#define STRIDER_NO_TRACK_NAME			"null"

// Time after which if you haven't seen your enemy you stop facing him
#define STRIDER_TIME_STOP_FACING_ENEMY 3.0 

// Spawnflags
enum
{
	SF_CAN_STOMP_PLAYER					= 0x10000,
	SF_TAKE_MINIMAL_DAMAGE_FROM_NPCS	= 0x20000
};

const float STRIDER_SPEED = 500;
const float STRIDER_SPEED_CHANGE = .0067; // per think


static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs );

int s_iImpactEffectTexture = -1;

//==================================================
// Custom Activities
//==================================================

int	ACT_STRIDER_LOOKL;
int	ACT_STRIDER_LOOKR;
int ACT_STRIDER_DEPLOYRA1;
int ACT_STRIDER_AIMRA1;
int ACT_STRIDER_FINISHRA1;
int ACT_STRIDER_DODGER;
int ACT_STRIDER_DODGEL;
int ACT_STRIDER_STOMPL;
int ACT_STRIDER_STOMPR;
int ACT_STRIDER_FLICKL;
int ACT_STRIDER_FLICKR;
int ACT_STRIDER_CARRIED;
int ACT_STRIDER_DEPLOY;
int	ACT_STRIDER_GESTURE_DEATH;

// UNDONE: Split sleep into 3 activities (start, loop, end)
int ACT_STRIDER_SLEEP;

// These bones have physics shadows
// It allows a one-way interaction between the strider and
// the physics world
static const char *pFollowerBoneNames[] =
{
	// Head
	"Combine_Strider.Body_Bone",
#ifdef HL2_EPISODIC
	"Combine_Strider.Neck_Bone",
	"Combine_Strider.Gun_Bone1",
	"Combine_Strider.Gun_Bone2",
#endif //HL2_EPISODIC

	// lower legs
	"Combine_Strider.Leg_Left_Bone1",
	"Combine_Strider.Leg_Right_Bone1",
	"Combine_Strider.Leg_Hind_Bone1",
	
	// upper legs
	"Combine_Strider.Leg_Left_Bone",
	"Combine_Strider.Leg_Right_Bone",
	"Combine_Strider.Leg_Hind_Bone",
};

// NOTE: These indices must directly correlate with the above list!
enum
{
	STRIDER_BODY_FOLLOWER_INDEX = 0,
#ifdef HL2_EPISODIC
	STRIDER_NECK_FOLLOWER_INDEX,
	STRIDER_GUN1_FOLLOWER_INDEX,
	STRIDER_GUN2_FOLLOWER_INDEX,
#endif //HL2_EPISODIC
	
	STRIDER_LEFT_LEG_FOLLOWER_INDEX,
	STRIDER_RIGHT_LEG_FOLLOWER_INDEX,
	STRIDER_BACK_LEG_FOLLOWER_INDEX,

	STRIDER_LEFT_UPPERLEG_FOLLOWER_INDEX,
	STRIDER_RIGHT_UPPERLEG_FOLLOWER_INDEX,
	STRIDER_BACK_UPPERLEG_FOLLOWER_INDEX,
};

#define MINIGUN_MAX_YAW		90.0f
#define MINIGUN_MIN_YAW		-90.0f
#define MINIGUN_MAX_PITCH	45.0f
#define MINIGUN_MIN_PITCH	-45.0f

//-----------------------------------------------------------------------------
//
// CNPC_Strider
//
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CNPC_Strider, DT_NPC_Strider)
	SendPropVector(SENDINFO(m_vecHitPos), -1, SPROP_COORD),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 0 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 1 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 2 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 3 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 4 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 5 ), -1, SPROP_COORD ),
END_SEND_TABLE()

//-------------------------------------

LINK_ENTITY_TO_CLASS( npc_strider, CNPC_Strider );

//-------------------------------------

BEGIN_DATADESC( CNPC_Strider )

#ifdef HL2_EPISODIC
	DEFINE_UTLVECTOR( m_hAttachedBusters,	FIELD_EHANDLE ),
#endif // HL2_EPISODIC

	DEFINE_EMBEDDED( m_EnemyUpdatedTimer ),
	DEFINE_EMBEDDEDBYREF( m_pMinigun ),
	DEFINE_FIELD( m_miniGunAmmo,			FIELD_INTEGER ),
	DEFINE_FIELD( m_miniGunDirectAmmo,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nextStompTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nextShootTime,		FIELD_TIME ),
	DEFINE_FIELD( m_ragdollTime,			FIELD_TIME ),
	DEFINE_FIELD( m_miniGunShootDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_aimYaw,				FIELD_FLOAT ),
	DEFINE_FIELD( m_aimPitch,				FIELD_FLOAT ),
	DEFINE_FIELD( m_blastHit,				FIELD_VECTOR ),
	DEFINE_FIELD( m_blastNormal,			FIELD_VECTOR ),
	DEFINE_FIELD( m_vecHitPos,				FIELD_POSITION_VECTOR ),
	DEFINE_AUTO_ARRAY( m_vecIKTarget, FIELD_POSITION_VECTOR ),

	DEFINE_EMBEDDED( m_PlayerFreePass ), 
	
	DEFINE_EMBEDDED( m_PostureAnimationTimer ),

	DEFINE_EMBEDDED( m_BoneFollowerManager ),

	// m_iszStriderBusterName - recreated at load time
	// m_iszMagnadeClassname - recreated at load time
	// m_iszHunterClassname - recreated at load time

	DEFINE_FIELD( m_hRagdoll,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCannonTarget,		FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_AttemptCannonLOSTimer ),

	DEFINE_FIELD( m_flSpeedScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTargetSpeedScale, FIELD_FLOAT ),

	DEFINE_EMBEDDED( m_LowZCorrectionTimer ),
	DEFINE_FIELD( m_BodyTargetBone,		FIELD_INTEGER ),

	DEFINE_FIELD( m_iVisibleEnemies, FIELD_INTEGER ),
	DEFINE_FIELD( m_flTargetAcquiredTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bCrouchLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNoCrouchWalk, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDontCrouch, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNoMoveToLOS, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bDisableBoneFollowers, FIELD_BOOLEAN, "disablephysics" ),

	DEFINE_FIELD( m_idealHeight, FIELD_FLOAT ),
	DEFINE_FIELD( m_HeightVelocity, FIELD_FLOAT ),

	DEFINE_FIELD( m_prevYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_doTurn, FIELD_FLOAT ),
	DEFINE_FIELD( m_doLeft, FIELD_FLOAT ),
	DEFINE_FIELD( m_doRight, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextTurnAct, FIELD_TIME ),

	DEFINE_FIELD( m_strTrackName, FIELD_STRING ),
	DEFINE_FIELD( m_hFocus, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hSmoke,	FIELD_EHANDLE ),

	DEFINE_FIELD( m_flTimeLastAlertSound, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeNextHuntSound, FIELD_TIME ),
	DEFINE_FIELD( m_flTimePlayerMissileDetected, FIELD_TIME ),
	DEFINE_FIELD( m_hPlayersMissile, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bMinigunUseDirectFire, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bUseAggressiveBehavior, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bFastCrouch, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMinigunEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExploding, FIELD_BOOLEAN ),

	// inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMinigunTime", InputSetMinigunTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMinigunTarget", InputSetMinigunTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCannonTarget", InputSetCannonTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FlickRagdoll", InputFlickRagdoll ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Crouch", InputCrouch ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CrouchInstantly", InputCrouchInstantly ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stand", InputStand ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetHeight", InputSetHeight ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetPath", InputSetTargetPath ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearTargetPath", InputClearTargetPath ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableCrouchWalk", InputDisableCrouchWalk ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableCrouchWalk", InputEnableCrouchWalk ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableAggressiveBehavior", InputEnableAggressiveBehavior ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableAggressiveBehavior", InputDisableAggressiveBehavior ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMinigun", InputDisableMinigun ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMinigun", InputEnableMinigun ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "StopShootingMinigunForSeconds", InputStopShootingMinigunForSeconds ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableCrouch", InputDisableCrouch ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMoveToLOS", InputDisableMoveToLOS ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DisableCollisionWith", InputDisableCollisionWith ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EnableCollisionWith", InputEnableCollisionWith ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ScaleGroundSpeed", InputScaleGroundSpeed ),

	// Function Pointers
//	DEFINE_FUNCTION( JumpTouch ),
	DEFINE_THINKFUNC( CarriedThink ),
	DEFINE_THINKFUNC( CannonHitThink ),

END_DATADESC()

//---------------------------------------------------------

float CNPC_Strider::gm_strideLength;

int CNPC_Strider::gm_BodyHeightPoseParam;
int CNPC_Strider::gm_YawControl;
int CNPC_Strider::gm_PitchControl;
int CNPC_Strider::gm_CannonAttachment;

float CNPC_Strider::gm_zCannonDist;
float CNPC_Strider::gm_zMinigunDist;
Vector CNPC_Strider::gm_vLocalRelativePositionCannon;
Vector CNPC_Strider::gm_vLocalRelativePositionMinigun;

//---------------------------------------------------------
//---------------------------------------------------------
CNPC_Strider::CNPC_Strider()
{
	m_strTrackName = MAKE_STRING( STRIDER_NO_TRACK_NAME );
	m_hFocus = NULL;
	m_pMinigun = new CStriderMinigun;
	m_hSmoke = NULL;
	m_PlayerFreePass.SetOuter( this );
	m_bExploding = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
CNPC_Strider::~CNPC_Strider()
{
	delete m_pMinigun;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::Precache()
{
	if ( !GetModelName() )
	{
		SetModelName( MAKE_STRING( "models/combine_strider.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	PropBreakablePrecacheAll( GetModelName() );

	PrecacheScriptSound( "NPC_Strider.StriderBusterExplode" );
	PrecacheScriptSound( "explode_5" );
	PrecacheScriptSound( "NPC_Strider.Charge" );
	PrecacheScriptSound( "NPC_Strider.RagdollDetach" );
	PrecacheScriptSound( "NPC_Strider.Whoosh" );
	PrecacheScriptSound( "NPC_Strider.Creak" );
	PrecacheScriptSound( "NPC_Strider.Alert" );
	PrecacheScriptSound( "NPC_Strider.Pain" );
	PrecacheScriptSound( "NPC_Strider.Death" );
	PrecacheScriptSound( "NPC_Strider.FireMinigun" );
	PrecacheScriptSound( "NPC_Strider.Shoot" );
	PrecacheScriptSound( "NPC_Strider.OpenHatch" );
	PrecacheScriptSound( "NPC_Strider.Footstep" );
	PrecacheScriptSound( "NPC_Strider.Skewer" );
	PrecacheScriptSound( "NPC_Strider.Hunt" );
	PrecacheMaterial( "effects/water_highlight" );
	s_iImpactEffectTexture = PrecacheModel( "sprites/physbeam.vmt" );
	PrecacheMaterial( "sprites/bluelaser1" );
	PrecacheMaterial( "effects/blueblacklargebeam" );
	PrecacheMaterial( "effects/strider_pinch_dudv" );
	PrecacheMaterial( "effects/blueblackflash" );
	PrecacheMaterial( "effects/strider_bulge_dudv" );
	PrecacheMaterial( "effects/strider_muzzle" );

	PrecacheModel( "models/chefhat.mdl" );

	UTIL_PrecacheOther( "sparktrail" );

	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::Spawn()
{
	Precache();

	m_miniGunAmmo = GetAmmoDef()->Index("StriderMinigun"); 
	m_miniGunDirectAmmo = GetAmmoDef()->Index("StriderMinigunDirect");
	m_pMinigun->Init();

	EnableServerIK();
	
	SetModel( STRING( GetModelName() ) );

	BaseClass::Spawn();

	//m_debugOverlays |= OVERLAY_NPC_ROUTE_BIT | OVERLAY_BBOX_BIT;
	SetHullType( HULL_LARGE_CENTERED );
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType( NAV_FLY );
	m_flGroundSpeed	= STRIDER_SPEED;
	m_flSpeedScale = m_flTargetSpeedScale = 1.0;
	m_NPCState = NPC_STATE_NONE;
	m_bloodColor = DONT_BLEED;
	
	m_iHealth = sk_strider_health.GetFloat();
	m_iMaxHealth = 500;

	m_flFieldOfView = 0.0; // 180 degrees

	AddFlag( FL_FLY );
	SetCollisionGroup( HL2COLLISION_GROUP_STRIDER );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL );

	// Cached for quick comparisons
	m_iszStriderBusterName = AllocPooledString( "weapon_striderbuster" );
	m_iszMagnadeClassname  = AllocPooledString( "npc_grenade_magna" );
	m_iszHunterClassname   = AllocPooledString( "npc_hunter" );

	// BMCD: Force collision hooks
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	SetupGlobalModelData();
	
	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_INNATE_RANGE_ATTACK2 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_SQUAD );

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );

	// find the ground, move up to strider stand height
	Vector mins(-16,-16,-16), maxs(16,16,16);
	Vector origin = GetLocalOrigin();

	MoveToGround( &origin, this, mins, maxs );
	origin.z += GetMaxHeight();//(GetAbsOrigin().z - vecSurroundMins.z) + mins.z;

	SetLocalOrigin( origin );

	NPCInit();

	// Strider doesn't care about missiles for now.
	AddClassRelationship( CLASS_MISSILE, D_NU, 0 );

	m_bCrouchLocked = false;
	m_bMinigunEnabled = true;

	m_PostureAnimationTimer.Set( 8, 16 );

	m_BodyTargetBone = -1;

	CreateFocus();

	m_EnemyUpdatedTimer.Set( 0 );

	// Don't minigun things farther than 500 feet away. 
	m_flDistTooFar = 500.0f * 12.0f;

	GetEnemies()->SetFreeKnowledgeDuration( strider_free_knowledge.GetFloat() );

	m_hPlayersMissile.Set( NULL );
	m_flTimeNextHuntSound = gpGlobals->curtime - 1.0f;
}

void CNPC_Strider::SetupGlobalModelData()
{
	gm_BodyHeightPoseParam = LookupPoseParameter( "body_height" );
	gm_YawControl = LookupPoseParameter( "yaw" );
	gm_PitchControl = LookupPoseParameter( "pitch" );
	gm_CannonAttachment = LookupAttachment( "BigGun" );

	// BMCD: Get the conservative boxes from sequences
	Vector mins, maxs;
	ExtractBbox( SelectHeaviestSequence( ACT_WALK ), mins, maxs );
	CNPC_Strider::gm_strideLength = (maxs.x - mins.x) * 0.5;

	// UNDONE: use crouch when crouched
	CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );
}

void CNPC_Strider::OnRestore()
{
	BaseClass::OnRestore();
	SetupGlobalModelData();
	CreateVPhysics();

	// Cached for quick comparisons
	m_iszStriderBusterName = FindPooledString( "weapon_striderbuster" );
	m_iszMagnadeClassname  = FindPooledString( "npc_grenade_magna" );
}

bool CNPC_Strider::m_sbStaticPoseParamsLoaded = false;
int CNPC_Strider::m_poseMiniGunYaw = 0;
int CNPC_Strider::m_poseMiniGunPitch = 0;
//-----------------------------------------------------------------------------
// Purpose: Cache whatever pose parameters we intend to use
//-----------------------------------------------------------------------------
void CNPC_Strider::PopulatePoseParameters( void )
{
	if (!m_sbStaticPoseParamsLoaded)
	{
		m_poseMiniGunYaw		= LookupPoseParameter( "miniGunYaw");
		m_poseMiniGunPitch		= LookupPoseParameter( "miniGunPitch" );

		m_sbStaticPoseParamsLoaded = true;
	}

	BaseClass::PopulatePoseParameters();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CreateVPhysics()
{
	// The strider has bone followers for every solid part of its body, 
	// so there's no reason for the bounding box to be solid.
	//BaseClass::CreateVPhysics();

	if ( !m_bDisableBoneFollowers )
	{
		InitBoneFollowers();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Strider::InitBoneFollowers( void )
{
	// Don't do this if we're already loaded
	if ( m_BoneFollowerManager.GetNumBoneFollowers() != 0 )
		return;

	// Init our followers
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::PostNPCInit()
{
	BaseClass::PostNPCInit();

	if( CarriedByDropship() )
	{
		SetMoveType( MOVETYPE_NONE );
		SetActivity( (Activity)ACT_STRIDER_CARRIED );
		SetThink( &CNPC_Strider::CarriedThink );
		RemoveFlag( FL_FLY );
	}

	m_PlayerFreePass.SetPassTarget( UTIL_PlayerByIndex(1) );
	
	AI_FreePassParams_t freePassParams = 
	{
		strider_free_pass_start_time.GetFloat(),		// timeToTrigger
		strider_free_pass_duration.GetFloat(), 			// duration
		strider_free_pass_move_tolerance.GetFloat(), 	// moveTolerance
		strider_free_pass_refill_rate.GetFloat(), 		// refillRate
		strider_free_pass_cover_dist.GetFloat(), 		// coverDist
		strider_peek_time.GetFloat(), 					// peekTime
		strider_peek_time_after_damage.GetFloat(), 		// peekTimeAfterDamage
		strider_peek_eye_dist.GetFloat(), 				// peekEyeDist
		strider_peek_eye_dist_z.GetFloat(), 			// peekEyeDistZ
	};
	
	m_PlayerFreePass.SetParams( freePassParams );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::Activate()
{
	BaseClass::Activate();

	const char *pszBodyTargetBone = "combine_strider.neck_bone";

	m_BodyTargetBone = LookupBone( pszBodyTargetBone );

	if ( m_BodyTargetBone == -1 )
	{
		DevMsg( "Couldn't find npc_strider bone %s, which is used as target for others\n", pszBodyTargetBone );
	}

	gm_BodyHeightPoseParam = LookupPoseParameter( "body_height" );
	gm_YawControl = LookupPoseParameter( "yaw" );
	gm_PitchControl = LookupPoseParameter( "pitch" );
	gm_CannonAttachment = LookupAttachment( "BigGun" );

	if ( gm_zCannonDist == 0 )
	{
		// Have to create a virgin strider to ensure proper pose
		CNPC_Strider *pStrider = (CNPC_Strider *)CreateEntityByName( "npc_strider" );
		Assert(pStrider);
		pStrider->m_bDisableBoneFollowers = true; // don't create these since we're just going to destroy him
		DispatchSpawn( pStrider );

		pStrider->SetActivity( ACT_DIERAGDOLL );
		pStrider->InvalidateBoneCache();
		gm_zCannonDist = pStrider->CannonPosition().z - pStrider->GetAbsOrigin().z;

		// Currently just using the gun for the vertical component!
		Vector defEyePos;
		pStrider->GetAttachment( "minigunbase", defEyePos );
		gm_zMinigunDist = defEyePos.z - pStrider->GetAbsOrigin().z;

		Vector position;
		pStrider->GetAttachment( "biggun", position );
		VectorITransform( position, pStrider->EntityToWorldTransform(), gm_vLocalRelativePositionCannon );

		pStrider->GetAttachment( "minigun", position );
		VectorITransform( position, pStrider->EntityToWorldTransform(), gm_vLocalRelativePositionMinigun );
		UTIL_Remove( pStrider );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::UpdateOnRemove()
{
	m_BoneFollowerManager.DestroyBoneFollowers();

#ifdef HL2_EPISODIC
	m_hAttachedBusters.Purge();
#endif // HL2_EPISODIC

	BaseClass::UpdateOnRemove();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InitBoneControllers()
{
	BaseClass::InitBoneControllers( );
	
	SetHeight( GetMaxHeight() );
	SetIdealHeight( GetMaxHeight() );
}

//---------------------------------------------------------
//---------------------------------------------------------
Class_T CNPC_Strider::Classify()
{
	if( CarriedByDropship() )
		return CLASS_NONE;

	return CLASS_COMBINE;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::ShouldAttractAutoAim( CBaseEntity *pAimingEnt )
{
#ifdef HL2_EPISODIC
	if( m_hAttachedBusters.Count() > 0 )
		return false;
#endif//HL2_EPISODIC

	return BaseClass::ShouldAttractAutoAim( pAimingEnt );
}

//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_Strider::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		EntityText(text_offset,CFmtStr("Ideal Height: %.1f; Height: %.1f", GetIdealHeight(), GetHeight()),0);
		text_offset++;
		if ( m_PlayerFreePass.HasPass() )
		{
			EntityText(text_offset,CFmtStr("Free pass: %.1f", m_PlayerFreePass.GetTimeRemaining()),0);
			text_offset++;
		}

		CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
		if ( pPlayer )
		{
			if ( GetSenses()->ShouldSeeEntity( pPlayer ) && GetSenses()->CanSeeEntity( pPlayer ) )
			{
				EntityText(text_offset,"See player",0);
				text_offset++;
			}
			else
			{
				float temp = m_PlayerFreePass.GetTimeRemaining();
				m_PlayerFreePass.SetTimeRemaining( 0 );

				if ( BaseClass::FVisible( pPlayer ) && !FVisible( pPlayer ) )
				{
					EntityText(text_offset,"Player peeking",0);
					text_offset++;
				}
				m_PlayerFreePass.SetTimeRemaining( temp );
			}
		}

		if ( m_flTargetSpeedScale != 1.0 )
		{
			EntityText(text_offset,CFmtStr( "Speed scaled to %.1f", m_flGroundSpeed ),0);
			text_offset++;
		}
	}

	return text_offset;
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::EyePosition()
{
	Vector eyePosition = GetAdjustedOrigin();
	eyePosition.z += gm_zMinigunDist;
	return eyePosition;
}

//---------------------------------------------------------
//---------------------------------------------------------
const Vector &CNPC_Strider::GetViewOffset()
{
	Vector vOffset;

	vOffset.x = 0;
	vOffset.y = 0;
	vOffset.z = ( GetHeight() - GetMaxHeightModel() ) + gm_zMinigunDist;

	Assert( VectorsAreEqual( GetAbsOrigin() + vOffset, EyePosition(), 0.1 ));

	SetViewOffset( vOffset );

	return BaseClass::GetViewOffset();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::CalculateIKLocks( float currentTime )
{
	BaseClass::CalculateIKLocks( currentTime );
	if ( m_pIk && m_pIk->m_target.Count() )
	{
		Assert(m_pIk->m_target.Count() > STOMP_IK_SLOT);
		// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
		CIKTarget &target = m_pIk->m_target[STOMP_IK_SLOT];
		target.SetPos( m_vecHitPos.Get() );
		for ( int i = 0; i < NUM_STRIDER_IK_TARGETS; i++ )
		{
			target = m_pIk->m_target[i];

			if (!target.IsActive())
				continue;

			m_vecIKTarget.Set( i, target.est.pos );

#if 0
		// yellow box at target pos - helps debugging
		//if (i == 0)
			NDebugOverlay::Line( GetAbsOrigin(), m_vecIKTarget[i], 255, 255, 0, 0, 0.1 );
			NDebugOverlay::Box( m_vecIKTarget[i], Vector(-8,-8,-8), Vector(8,8,8), 255, 255, 0, 0, 4.0 );
#endif
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::NPCThink(void)
{
	if ( m_hRagdoll.Get() )
	{
		m_nextStompTime = gpGlobals->curtime + 5;
	}

	if ( m_flTargetSpeedScale > 0.01 )
	{
		float deltaSpeedScale = m_flSpeedScale - m_flTargetSpeedScale;
		if ( fabsf( deltaSpeedScale ) > .01 )
		{
			if ( deltaSpeedScale < 0 )
			{
				m_flSpeedScale += STRIDER_SPEED_CHANGE;
				if ( m_flSpeedScale > m_flTargetSpeedScale )
				{
					m_flSpeedScale = m_flTargetSpeedScale;
				}
			}
			else
			{
				m_flSpeedScale -= STRIDER_SPEED_CHANGE;
				if ( m_flSpeedScale < m_flTargetSpeedScale )
				{
					m_flSpeedScale = m_flTargetSpeedScale;
				}
			}
		}
		else
		{
			m_flSpeedScale = m_flTargetSpeedScale;
		}
	}
	else
	{
		m_flTargetSpeedScale = 1.0;
	}

	BaseClass::NPCThink();
	
	m_pMinigun->Think( this, 0.1 );

	// update follower bones
	m_BoneFollowerManager.UpdateBoneFollowers(this);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::PrescheduleThink()
{
	if( IsUsingAggressiveBehavior() && GetEnemy() && GetEnemy()->IsPlayer() )
	{
		AddFacingTarget( GetEnemy(), GetEnemies()->LastKnownPosition( GetEnemy() ) , 1.0, 2.0 );
	}

	// Next missile will kill me!
	if( GetHealth() <= 50 && random->RandomInt( 0, 20 ) == 0 )
	{
		CBaseEntity *pTrail = CreateEntityByName( "sparktrail" );
		pTrail->SetOwnerEntity( this );
		pTrail->Spawn();
	}

#if 0
	NDebugOverlay::Cross3D( GetAdjustedOrigin(), 16, 128, 128, 128, false, .1 );
	Vector vIdealOrigin = GetAbsOrigin();
	vIdealOrigin.z -= GetMaxHeightModel() - GetIdealHeight();
	NDebugOverlay::Cross3D( vIdealOrigin, 16, 255, 255, 255, false, .1 );
#endif

	if( strider_eyepositions.GetBool() )
	{
		NDebugOverlay::Cross3D( EyePosition(), 16, 0, 255, 0, false, 0.1 );
		NDebugOverlay::Cross3D( EyePositionCrouched(), 16, 255, 255, 0, false, 0.1 );
	}

	if( strider_show_focus.GetBool() )
	{
		QAngle angles;

		angles.x = gpGlobals->curtime * 20.0f;
		angles.y = angles.x * 0.5f;
		angles.z = 0.0f;

		NDebugOverlay::Cross3DOriented( GetFocus()->GetAbsOrigin(), angles, 24, 255, 255, 0, false, 0.1 );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::GatherConditions()
{
	if ( AIGetNumFollowers( this, m_iszHunterClassname ) == 0 )	
	{
		// This works with old data because need to do before base class so as to not choose as enemy
		if ( m_PlayerFreePass.HasPass() || ( !m_pMinigun->IsShooting() || GetEnemy() != m_PlayerFreePass.GetPassTarget() ) ) // no free pass when in midst of shooting at target
			m_PlayerFreePass.Update();
	}
	else
	{
		m_PlayerFreePass.Reset( strider_free_pass_after_escorts_dead.GetFloat(), strider_free_pass_tolerance_after_escorts_dead.GetFloat() );
	}

	if( IsUsingAggressiveBehavior() )
	{
		if( m_PlayerFreePass.HasPass() && !m_pMinigun->IsShooting() )
		{
			// Make the minigun stitch
			m_bMinigunUseDirectFire = false;
		}
	}

	//---------------------------------

	BaseClass::GatherConditions();

	if( IsUsingAggressiveBehavior() )
	{
		if( GetEnemy() )
		{
			if( HasCondition( COND_SEE_ENEMY ) )
			{
				// Keep setting up to play my hunt sound at some random time after losing sight of my enemy.
				m_flTimeNextHuntSound = gpGlobals->curtime + 1.0f;
			}
			else
			{
				if( gpGlobals->curtime >= m_flTimeNextHuntSound && !m_pMinigun->IsShooting() )
				{
					HuntSound();
				}
			}
		}

		if( m_hPlayersMissile )
		{
			if( !m_pMinigun->IsShooting() && GetEnemy() && GetEnemy()->IsPlayer() )
			{
				// If the missile is closer to the player than I am, stay suppressed. This is essentially
				// allowing the missile to strike me if it was fired off before I started shooting. 
				// If the missile passes me or goes way off course, I can shoot.
				float flPlayerMissileDist;
				float flPlayerStriderDist;

				flPlayerMissileDist = GetEnemy()->GetAbsOrigin().DistTo( m_hPlayersMissile->GetAbsOrigin() );
				flPlayerStriderDist = GetEnemy()->GetAbsOrigin().DistTo( EyePosition() );
				float flDiff = flPlayerMissileDist - flPlayerStriderDist;

				// Figure out how long it's been since I've fired my cannon because of a player's missile.
				float flTimeSuppressed = gpGlobals->curtime - m_flTimePlayerMissileDetected;

				if( flDiff < strider_missile_suppress_dist.GetFloat() && flTimeSuppressed < strider_missile_suppress_time.GetFloat() )
				{
					// Defer the minigun until/unless the missile has passed me by 10 feet
					m_pMinigun->StopShootingForSeconds( this, GetEnemy(), 0.5f );
				}
			}
		}
	}

	// This pair of conditions is nice to have around...
	if( m_pMinigun->IsShooting() )
	{
		SetCondition( COND_STRIDER_MINIGUN_SHOOTING );
	}
	else
	{
		SetCondition( COND_STRIDER_MINIGUN_NOT_SHOOTING );
	}

	if( GetCannonTarget() )
	{
		SetCondition( COND_STRIDER_HAS_CANNON_TARGET );

		if( strider_show_cannonlos.GetBool() )
		{
			trace_t tr;
			UTIL_TraceLine( CannonPosition(), GetCannonTarget()->WorldSpaceCenter(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, false, 0.1 );

			if( tr.fraction != 1.0 )
				NDebugOverlay::Line( tr.endpos, GetCannonTarget()->WorldSpaceCenter(), 255, 0, 0, false, 0.1 );
		}
	}
	else
	{
		ClearCondition( COND_STRIDER_HAS_CANNON_TARGET );
	}

	ClearCondition( COND_CAN_RANGE_ATTACK2 );
	ClearCondition( COND_STRIDER_HAS_LOS_Z );

	// If not locked into a crouch, look into adjusting height to attack targets.
	if( !m_bCrouchLocked && !m_bDontCrouch )
	{
		if( m_hCannonTarget != NULL )
		{	
			if( !IsStriderCrouching() && !IsStriderStanding() )
			{
				if ( WeaponLOSCondition( GetAdjustedOrigin(), m_hCannonTarget->GetAbsOrigin(), false ) )
				{
					SetCondition( COND_CAN_RANGE_ATTACK2 );
				}
				else
				{
					GatherHeightConditions( GetAdjustedOrigin(), m_hCannonTarget );
				}
			}
		}
		else if( GetEnemy() )
		{
			if ( strider_distributed_fire.GetBool() && !IsUsingAggressiveBehavior() )
			{
				m_iVisibleEnemies = 0;
				AIEnemiesIter_t iter;

				for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
				{
					if( IRelationType( pEMemory->hEnemy ) != D_NU && IRelationType( pEMemory->hEnemy ) != D_LI )
					{
						if( pEMemory->timeLastSeen == gpGlobals->curtime )
						{
							m_iVisibleEnemies++;
						}
					}
				}

				// If I'm on target and see more targets than just this one, move on to another target for a bit!
				// Because the Mingun's state will stay "on target" until the minigun gets a chance to think,
				// and this function may be called several times per strider think, don't call this code anymore in the same
				// think when a new enemy is chosen.
				//
				// Don't switch targets if shooting at a bullseye! Level designers depend on bullseyes.
				if( GetEnemy() && m_pMinigun->IsShooting() && GetTimeEnemyAcquired() != gpGlobals->curtime )
				{
					if( m_pMinigun->IsOnTarget( 3 ) && !FClassnameIs( GetEnemy(), "npc_bullseye" ) )
					{
						if( m_iVisibleEnemies > 1 )
						{
							// Time to ignore this guy for a little while and switch targets.
							GetEnemies()->SetTimeValidEnemy( GetEnemy(), gpGlobals->curtime + ( STRIDER_IGNORE_TARGET_DURATION * m_iVisibleEnemies ) );
							SetEnemy( NULL, false );
							ChooseEnemy();
						}
						else if( GetEnemy()->IsPlayer() && GetEnemy() == m_pMinigun->GetTarget() )
						{
							// Give the poor target a break.
							m_pMinigun->StopShootingForSeconds( this, GetEnemy(), GetMinigunShootDowntime() );
						}
					}
				}
			}

			if ( GetEnemy() ) // Can go null above
			{
				if ( !IsStriderCrouching() && !IsStriderStanding() &&
					 ( !HasCondition( COND_SEE_ENEMY ) || 
					   !WeaponLOSCondition( GetAdjustedOrigin(), GetEnemy()->BodyTarget( GetAdjustedOrigin() ), false ) ) )
				{
#if 0
					if ( !HasCondition( COND_STRIDER_SHOULD_CROUCH ) )
						SetIdealHeight( MIN( GetMaxHeight(), GetHeight() + 75.0 * 0.1 ) ); // default to rising up
#endif
					GatherHeightConditions( GetAdjustedOrigin(), GetEnemy() );
				}
			}
		}
		else
			SetIdealHeight( GetMaxHeight() );
	}
	else
	{
		if( m_hCannonTarget != NULL && CurrentWeaponLOSCondition( m_hCannonTarget->GetAbsOrigin(), false ) )
			SetCondition( COND_CAN_RANGE_ATTACK2 );
	}

	if( m_bDontCrouch )
	{
		if( HasCondition( COND_STRIDER_SHOULD_CROUCH ) )
		{
			Msg("TELL WEDGE I'M TRYING TO CROUCH!\n");
		}

		ClearCondition( COND_STRIDER_SHOULD_CROUCH );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::GatherHeightConditions( const Vector &vTestPos, CBaseEntity *pEntity )
{
	if ( HasCondition( COND_STRIDER_SHOULD_CROUCH ) )
		return;

	float maxZ = (GetAbsOrigin().z - (GetMaxHeightModel() - GetMaxHeight()));
	float minZ = (maxZ - ( GetMaxHeight() - GetMinHeight()));;
	float newHeight = FLT_MAX;

	if( FInViewCone( pEntity ) && GetWeaponLosZ( vTestPos, minZ, maxZ, GetHeightRange() * .1, pEntity, &newHeight ) )
	{
		bool bDoProceduralHeightChange = true;
		newHeight = GetMaxHeightModel() - ( GetAbsOrigin().z - newHeight);

		if ( m_LowZCorrectionTimer.Expired() )
		{
			// Hack to handle discrepency between ideal gun pos and actual pos due to strider head tilt in crouch pos
			if ( pEntity && fabs( newHeight - GetHeight() ) < 12 && newHeight < GetMinHeight() + GetHeightRange() * .33 )
			{
				Vector muzzlePos;
				Vector targetPos = pEntity->BodyTarget( GetAdjustedOrigin() );

				GetAttachment( "minigun", muzzlePos );
				
				trace_t tr;
				AI_TraceLine( muzzlePos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

				if ( ( tr.m_pEnt != pEntity) && tr.fraction != 1.0 && !CanShootThrough( tr, targetPos ) )
				{
					if ( !GetWeaponLosZ( vTestPos, minZ + GetHeightRange() * .33, maxZ, GetHeightRange() * .1, pEntity, &newHeight ) )
					{
						return;
					}
					newHeight = GetMaxHeightModel() - ( GetAbsOrigin().z - newHeight);
				}
				else
				{
					m_LowZCorrectionTimer.Set( 5.0 );
				}
			}
		}

		if ( !strider_always_use_procedural_height.GetBool() )
		{
			// If going from max to min, or min to max, use animations 60% of the time
			if ( fabsf(GetMaxHeight() - GetHeight()) < 0.1 && fabsf(GetMinHeight() - newHeight) < 0.1 )
			{
				if ( random->RandomInt(1, 10 ) <= 6 )
				{
					SetCondition( COND_STRIDER_SHOULD_CROUCH );
					bDoProceduralHeightChange = false;
				}
			}
			else if ( fabsf(GetMinHeight() - GetHeight()) < 0.1 && fabsf(GetMaxHeight() - newHeight) < 0.1 )
			{
				if ( random->RandomInt(1, 10 ) <= 6 )
				{
					SetCondition( COND_STRIDER_SHOULD_STAND );
					bDoProceduralHeightChange = false;
				}
			}

			// Otherwise, if going from near max or near min to the other, use animations based on time
			if ( bDoProceduralHeightChange && m_PostureAnimationTimer.Expired() )
			{
				if ( GetHeight() - GetMinHeight() > GetHeightRange() * .85 && newHeight - GetMinHeight() < GetHeightRange() * .15 )
				{
					m_PostureAnimationTimer.Reset();
					SetCondition( COND_STRIDER_SHOULD_CROUCH );
					bDoProceduralHeightChange = false;
				}
				else if ( newHeight - GetMinHeight() > GetHeightRange() * .85 && GetHeight() - GetMinHeight() < GetHeightRange() * .15 )
				{
					m_PostureAnimationTimer.Reset();
					SetCondition( COND_STRIDER_SHOULD_STAND );
					bDoProceduralHeightChange = false;
				}
			}
		}

		if ( bDoProceduralHeightChange )
		{
			SetCondition( COND_STRIDER_HAS_LOS_Z );
			SetIdealHeight( newHeight );
			if ( strider_test_height.GetFloat() > .1 )
				SetIdealHeight( strider_test_height.GetFloat() );
		}
		else
			SetIdealHeight( GetHeight() );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::OnStateChange( NPC_STATE oldState, NPC_STATE newState )
{
	if ( oldState == NPC_STATE_SCRIPT )
	{
		m_pMinigun->Enable( this, m_bMinigunEnabled );
	}
	else if ( newState == NPC_STATE_SCRIPT )
	{
		m_pMinigun->Enable( this, false );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();
	if (m_NPCState != NPC_STATE_SCRIPT)
	{
		SetCustomInterruptCondition( COND_STRIDER_DO_FLICK );
	}

	if( IsCurSchedule( SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK, false ) || IsCurSchedule( SCHED_ESTABLISH_LINE_OF_FIRE, false ) )
	{
		SetCustomInterruptCondition( COND_STRIDER_MINIGUN_SHOOTING );
	}

	if( IsCurSchedule( SCHED_IDLE_STAND ) )
	{
		SetCustomInterruptCondition( COND_STRIDER_HAS_CANNON_TARGET );
	}

	// If our base-class schedule breaks on CAN_RANGE_ATTACK1, then also break
	// on HAS_CANNON_TARGET.
	if( GetCurSchedule()->HasInterrupt( COND_CAN_RANGE_ATTACK1 ) && HasCondition( COND_STRIDER_HAS_CANNON_TARGET ) )
	{
		SetCustomInterruptCondition( COND_STRIDER_HAS_CANNON_TARGET );
	}

	if( IsCurSchedule( SCHED_IDLE_WALK ) || ( IsCurSchedule( SCHED_IDLE_STAND ) && hl2_episodic.GetBool() ) )
	{
		SetCustomInterruptCondition(COND_STRIDER_SHOULD_CROUCH);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::SelectSchedule()
{
/*
	if( GetMoveType() != MOVETYPE_FLY )
	{
		// Dropship just released me.
		SetMoveType( MOVETYPE_FLY );
		return SCHED_STRIDER_FALL_TO_GROUND;
	}
*/
	if ( strider_idle_test.GetBool() )
	{
		m_pMinigun->Enable( this, false );
		return SCHED_IDLE_STAND;
	}
	else
	{
		m_pMinigun->Enable( this, m_bMinigunEnabled );
	}

	if( m_NPCState == NPC_STATE_SCRIPT )
		return BaseClass::SelectSchedule();

	// If we're starting to die, then just wait for this to happen
	if ( m_lifeState == LIFE_DYING )
		return SCHED_IDLE_STAND;

	if( m_NPCState == NPC_STATE_DEAD )
		return SCHED_STRIDER_DIE;

	if( HasPendingTargetPath() )
	{
#if 0
		if( IsInCrouchedPosture() && !m_bCrouchLocked )
		{
			// Make the strider stand!
			return SCHED_STRIDER_STAND;
		}
		else
		{
			SetTargetPath();
		}
#else
		SetTargetPath();
#endif
	}

	//---------------------------------

	if( HasCondition( COND_STRIDER_SHOULD_CROUCH ) && GetHeight() - GetMinHeight() > GetHeightRange() * .5 )
	{
		return SCHED_STRIDER_CROUCH;
	}
	if( HasCondition( COND_STRIDER_SHOULD_STAND ) && !m_bCrouchLocked  && GetHeight() - GetMinHeight() < GetHeightRange() * .5 )
	{
		return SCHED_STRIDER_STAND;
	}

	if( HasCondition( COND_CAN_RANGE_ATTACK2 ) )
	{
		return SCHED_STRIDER_RANGE_ATTACK2;
	}
	else if( m_AttemptCannonLOSTimer.Expired() && HasCondition( COND_STRIDER_HAS_CANNON_TARGET ) )
	{
		m_AttemptCannonLOSTimer.Set( 5 );
		return SCHED_STRIDER_ESTABLISH_LINE_OF_FIRE_CANNON;
	}

	//---------------------------------

	if( m_NPCState == NPC_STATE_COMBAT )
	{
		if ( !HasCondition( COND_NEW_ENEMY ) )
		{
			if ( m_hRagdoll.Get() && (gpGlobals->curtime > m_ragdollTime  || HasCondition( COND_STRIDER_DO_FLICK ) ) )
			{
				return SCHED_STRIDER_FLICKL;
			}
		}

		if ( HasCondition( COND_STRIDER_SHOULD_CROUCH ) )
		{
			return SCHED_STRIDER_CROUCH;
		}

		if ( HasCondition( COND_STRIDER_SHOULD_STAND ) )
		{
			return SCHED_STRIDER_STAND;
		}
		
		if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{
			return SCHED_MELEE_ATTACK1;
		}
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			return SCHED_STRIDER_RANGE_ATTACK1;
		}

		ClearCondition( COND_STRIDER_ENEMY_UPDATED );
		if ( !m_EnemyUpdatedTimer.Expired() )
		{
			int baseResult = BaseClass::SelectSchedule();
			if ( baseResult != SCHED_COMBAT_FACE || !GetGoalEnt() )
				return baseResult;
		}

		if ( !GetGoalEnt() )
			return BaseClass::SelectSchedule();

		return SCHED_STRIDER_HUNT;
	}

	//---------------------------------

	if ( !GetGoalEnt() )
		return SCHED_IDLE_STAND;

	return SCHED_STRIDER_HUNT;
}

//---------------------------------------------------------
//---------------------------------------------------------

#define TIME_CARE_ENEMY 7.0

int CNPC_Strider::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_STRIDER_RANGE_ATTACK1;
	case SCHED_RANGE_ATTACK2:
		return SCHED_STRIDER_RANGE_ATTACK2;
	case SCHED_MELEE_ATTACK1:
		return SCHED_STRIDER_STOMPL;
	case SCHED_MELEE_ATTACK2:
		return SCHED_STRIDER_STOMPR;
	case SCHED_CHASE_ENEMY:
		{
			if( HasCondition( COND_SEE_ENEMY ) )
			{
				return SCHED_STRIDER_COMBAT_FACE;
			}

			return SCHED_STRIDER_CHASE_ENEMY;
		}

	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		if ( m_bNoMoveToLOS )
		{
			return SCHED_COMBAT_FACE;
		}

		if ( !m_bCrouchLocked )
		{
			if( IsInCrouchedPosture() )
			{
				if( m_pMinigun->IsShooting() )
				{
					// Don't stand yet.
					return SCHED_STRIDER_COMBAT_FACE;
				}
			}
			else if ( HasCondition( COND_STRIDER_HAS_LOS_Z ) && HasCondition( COND_SEE_ENEMY ) )
			{
				return SCHED_STRIDER_COMBAT_FACE;
			}
		}
		if ( scheduleType == SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK )
		{
			if ( gpGlobals->curtime - GetEnemyLastTimeSeen() < TIME_CARE_ENEMY )
				return SCHED_STRIDER_COMBAT_FACE;
			else if ( GetGoalEnt() )
				return SCHED_STRIDER_HUNT;
			else
			{
				if( IsUsingAggressiveBehavior() )
				{
					return SCHED_STRIDER_AGGRESSIVE_COMBAT_STAND;
				}

				return SCHED_COMBAT_PATROL;
			}
		}

		break;

	case SCHED_COMBAT_FACE:
		return SCHED_STRIDER_COMBAT_FACE;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY_LOS:
		ChainStartTask( TASK_GET_PATH_TO_ENEMY_LKP_LOS, pTask->flTaskData );
		break;

	case TASK_STRIDER_FALL_TO_GROUND:
		break;

	case TASK_STRIDER_FIRE_CANNON:
		FireCannon();
		TaskComplete();
		break;

	case TASK_STRIDER_SET_CANNON_HEIGHT:
		{
			if ( m_hCannonTarget )
				SetAbsIdealHeight( m_hCannonTarget->WorldSpaceCenter().z );
			TaskComplete();
		}
		break;

	case TASK_STRIDER_AIM:
		{
			// Stop the minigun for a bit, the big gun's about to shoot!
			m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 5 );

			//SetIdealActivity( (Activity)ACT_STRIDER_AIMRA1 );
			SetWait( pTask->flTaskData );
			m_aimYaw = 0;
			m_aimPitch = 0;
			// clear out the previous shooting
			SetPoseParameter( gm_YawControl, m_aimYaw );
			SetPoseParameter( gm_PitchControl, m_aimPitch );
			Vector vecShootPos;
			GetAttachment( gm_CannonAttachment, vecShootPos );
		
			// tell the client side effect to complete
			EntityMessageBegin( this, true );
				WRITE_BYTE( STRIDER_MSG_STREAKS );
				WRITE_VEC3COORD( vecShootPos );
			MessageEnd();
			CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
			EmitSound( filter2, entindex(), "NPC_Strider.Charge" );

			//	CPVSFilter filter( vecShootPos );
			//te->StreakSphere( filter, 0, 0, 150, 100, entindex(), gm_CannonAttachment );
		}
		break;

	case TASK_STRIDER_DODGE:
		break;

	case TASK_STRIDER_STOMP:
		{
			m_nextStompTime = gpGlobals->curtime + 5;
			Activity stompAct = (Activity) ( pTask->flTaskData > 0 ? ACT_STRIDER_STOMPR : ACT_STRIDER_STOMPL );
			ResetIdealActivity( stompAct );
		}
		break;

	case TASK_RANGE_ATTACK1:
		CBaseCombatWeapon *pWeapon;
		pWeapon = GetActiveWeapon();

		if( pWeapon )
		{
			pWeapon->PrimaryAttack();
		}
		else
		{
			TaskFail("no primary weapon");
		}

		TaskComplete();
		break;

	case TASK_STRIDER_BREAKDOWN:
		SetIdealActivity( (Activity)ACT_STRIDER_SLEEP );
		break;

	case TASK_STRIDER_REFRESH_HUNT_PATH:
		Assert( GetGoalEnt() );
		if ( GetGoalEnt() )
		{
			AI_NavGoal_t goal(GOALTYPE_PATHCORNER, GetGoalEnt()->GetLocalOrigin(), ACT_WALK, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);

			TranslateNavGoal( GetGoalEnt(), goal.dest );

			if ( ( m_debugOverlays & OVERLAY_NPC_SELECTED_BIT ) && ( m_debugOverlays & OVERLAY_NPC_ROUTE_BIT ) )
			{
				NDebugOverlay::Line( GetAbsOrigin() + Vector( 0, 0, 4), goal.dest, 255, 0, 255, true, 3 );
			}

			if ( GetNavigator()->SetGoal( goal ) )
			{
				TaskComplete();
				break;
			}
			TaskFail( FAIL_NO_ROUTE );
		}
		break;

	case TASK_STRIDER_START_MOVING:
		TaskComplete();
		break;

	case TASK_STRIDER_GET_PATH_TO_CANNON_LOS:
		{
			if ( GetCannonTarget() == NULL )
			{
				TaskFail("No Cannon Target");
				return;
			}
		
			AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
			Vector vecEnemy 	= GetCannonTarget()->WorldSpaceCenter();

			float flMaxRange = 4096;
			float flMinRange = 0;
			
			Vector posLos;
			bool found = false;

			if ( GetTacticalServices()->FindLateralLos( vecEnemy, &posLos ) )
			{
				found = true;
			}

			if ( !found && GetTacticalServices()->FindLos( vecEnemy, vecEnemy, flMinRange, flMaxRange, 1.0, &posLos ) )
			{
				found = true;
			}

			if ( found )
			{
				AI_NavGoal_t goal( posLos, ACT_RUN, AIN_HULL_TOLERANCE );

				GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
				GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );

				// Add the cannon target as a high priority facing entity.
				AddFacingTarget( GetCannonTarget(), GetCannonTarget()->WorldSpaceCenter(), 5.0, 5.0 );
			}
			else
			{
				TaskFail( "Can't get LOS to Cannon Target" );
			}
		}
		break;

	case TASK_STRIDER_GET_PATH_TO_CANNON_TARGET:
		{
			if( !m_hCannonTarget )
			{
				TaskFail( "No cannon target!\n" );
				return;
			}

			AI_NavGoal_t goal( m_hCannonTarget->GetAbsOrigin() );

			TranslateNavGoal( m_hCannonTarget, goal.dest );

			if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				DevWarning( 2, "GetPathToCannonTarget failed!!\n" );
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;

	case TASK_STRIDER_FACE_CANNON_TARGET:
		if ( m_hCannonTarget != NULL )
		{
			GetMotor()->SetIdealYawToTarget( m_hCannonTarget->WorldSpaceCenter() );
			SetTurnActivity(); 
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;
		
	case TASK_STRIDER_SET_HEIGHT:
		SetIdealHeight( pTask->flTaskData );
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY_LOS:
		ChainRunTask( TASK_GET_PATH_TO_ENEMY_LKP_LOS, pTask->flTaskData );
		break;

	case TASK_STRIDER_FALL_TO_GROUND:
		{
			// This doesn't work right now. (sjb)
			Vector vecVelocity = GetAbsVelocity();

			vecVelocity.z -= (GetCurrentGravity() * 0.1);

			SetAbsVelocity( vecVelocity );

			Vector pos = GetAbsOrigin();
			TranslateNavGoal( NULL, pos );

			if( GetAbsOrigin().z - pos.z <= 0.0f )
			{
				SetAbsVelocity( vec3_origin );
				TaskComplete();
			}
		}
		break;
	
	case TASK_STRIDER_AIM:
		{
			// BUGBUG: Need the real flInterval here, not just 0.1
			AimCannonAt( GetCannonTarget(), 0.1 );
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_STRIDER_DODGE:
		TaskComplete();
		break;

	case TASK_STRIDER_STOMP:
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		if ( GetEnemy() )
		{
			m_vecHitPos = CalculateStompHitPosition( GetEnemy() );
		}
		break;

	case TASK_STRIDER_BREAKDOWN:
		if ( IsActivityFinished() )
		{
			// UNDONE: Fix this bug!
			//Assert(!IsMarkedForDeletion());
			if ( !IsMarkedForDeletion() )
			{
				CTakeDamageInfo info;
				CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );
				TaskComplete();
				UTIL_Remove(this);
			}
		}
		break;

	case TASK_STRIDER_FACE_CANNON_TARGET:
   		GetMotor()->UpdateYaw();
   		if ( FacingIdeal() )
   		{
   			TaskComplete();
   		}
   		break;

	case TASK_PLAY_SEQUENCE:
		if( m_bFastCrouch && pTask->flTaskData == ACT_CROUCH )
		{
			SetPlaybackRate( 10.0f );
			if( IsSequenceFinished() )
			{
				m_bFastCrouch = false;
			}
		}

		// Hack to make sure client doesn't pop after stand/crouch is done
		if ( GetCycle() > 0.5 )
		{
			if ( IsStriderStanding() && GetHeight() != GetMaxHeight() )
				SetHeight( GetMaxHeight() );
			else if ( IsStriderCrouching() && GetHeight() != GetMinHeight() )
				SetHeight( GetMinHeight() );
			SetIdealHeight( GetHeight() );
		}
		BaseClass::RunTask( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
		break;		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Strider::Explode( void )
{
	Vector			velocity = vec3_origin;
	AngularImpulse	angVelocity = RandomAngularImpulse( -150, 150 );

	// Break into pieces
	breakablepropparams_t params( EyePosition(), GetAbsAngles(), velocity, angVelocity );
	params.impactEnergyScale = 1.0f;
	params.defBurstScale = 600.0f;
	params.defCollisionGroup = COLLISION_GROUP_NPC;
	PropBreakableCreateAll( GetModelIndex(), NULL, params, this, -1, true, true );

	// Go away
	m_lifeState = LIFE_DEAD;

	SetThink( &CNPC_Strider::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );

	AddEffects( EF_NODRAW );
	
	StopSmoking();

	m_BoneFollowerManager.DestroyBoneFollowers();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Strider::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt )
{
	if ( interactionType == g_interactionPlayerLaunchedRPG )
	{
		m_flTimePlayerMissileDetected = gpGlobals->curtime;
		m_hPlayersMissile = sourceEnt;
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::HandleAnimEvent( animevent_t *pEvent )
{
	Vector footPosition;

	switch( pEvent->event )
	{
	case STRIDER_AE_DIE:
		{
			Explode();
		}
		break;
	case STRIDER_AE_SHOOTCANNON:
		{
			FireCannon();
		}
		break;
	case STRIDER_AE_WINDUPCANNON:
		{
			AimCannonAt( GetCannonTarget(), 0.1 );

			// Stop the minigun for a bit, the big gun's about to shoot!
			m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 5 );

			m_aimYaw = 0;
			m_aimPitch = 0;

			// clear out the previous shooting
			SetPoseParameter( gm_YawControl, m_aimYaw );
			SetPoseParameter( gm_PitchControl, m_aimPitch );
			Vector vecShootPos;
			GetAttachment( gm_CannonAttachment, vecShootPos );
		
			// tell the client side effect to start
			EntityMessageBegin( this, true );
				WRITE_BYTE( STRIDER_MSG_STREAKS );
				WRITE_VEC3COORD( vecShootPos );
			MessageEnd();
			CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
			EmitSound( filter2, entindex(), "NPC_Strider.Charge" );
		}
		break;
	case STRIDER_AE_CANNONHIT:
		CreateConcussiveBlast( m_blastHit, m_blastNormal, this, 2.5 );
		break;

	case STRIDER_AE_SHOOTMINIGUN:
	{
		CBaseEntity *pTarget = gEntList.FindEntityGeneric( NULL, pEvent->options, this, this );
		if ( pTarget )
		{
			Vector vecTarget = pTarget->CollisionProp()->WorldSpaceCenter();
			ShootMinigun( &vecTarget, 0 );
		}
		break;
	}

	case STRIDER_AE_STOMPHITL:
		StompHit( STRIDER_LEFT_LEG_FOLLOWER_INDEX );
		break;
	case STRIDER_AE_STOMPHITR:
		StompHit( STRIDER_RIGHT_LEG_FOLLOWER_INDEX );
		break;
	case STRIDER_AE_FLICKL:
	case STRIDER_AE_FLICKR:
		{
			CBaseEntity *pRagdoll = m_hRagdoll;
			if ( pRagdoll )
			{
				CPASAttenuationFilter filter( pRagdoll, "NPC_Strider.RagdollDetach" );
				EmitSound( filter, pRagdoll->entindex(), "NPC_Strider.RagdollDetach" );
				DetachAttachedRagdoll( pRagdoll );
			}
			m_hRagdoll = NULL;
		}
		break;

	case STRIDER_AE_FOOTSTEP_LEFT:
	case STRIDER_AE_FOOTSTEP_LEFTM:
	case STRIDER_AE_FOOTSTEP_LEFTL:
		LeftFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_FOOTSTEP_RIGHT:
	case STRIDER_AE_FOOTSTEP_RIGHTM:
	case STRIDER_AE_FOOTSTEP_RIGHTL:
		RightFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_FOOTSTEP_BACK:
	case STRIDER_AE_FOOTSTEP_BACKM:
	case STRIDER_AE_FOOTSTEP_BACKL:
		BackFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_WHOOSH_LEFT:
		{
			GetAttachment( "left foot", footPosition );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_RIGHT:
		{
			GetAttachment( "right foot", footPosition );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_BACK:
		{
			GetAttachment( "back foot", footPosition );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_CREAK_LEFT:
	case STRIDER_AE_CREAK_BACK:
	case STRIDER_AE_CREAK_RIGHT:
		{
			EmitSound( "NPC_Strider.Creak" );
		}
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
Disposition_t CNPC_Strider::IRelationType( CBaseEntity *pTarget )
{
	if ( IsCannonTarget( pTarget ) )
		return D_HT;

	return BaseClass::IRelationType( pTarget );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority )
{
	if ( nDisposition ==  D_HT && pEntity->ClassMatches("npc_bullseye") )
		UpdateEnemyMemory( pEntity, pEntity->GetAbsOrigin() );
	BaseClass::AddEntityRelationship( pEntity, nDisposition, nPriority );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity )
{
	if ( IsCurSchedule( SCHED_STRIDER_RANGE_ATTACK2, false ) )
	{
		SetGoalEnt( pGoalEntity );
		return true;
	}
	return BaseClass::ScheduledMoveToGoalEntity( scheduleType, pGoalEntity, movementActivity );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity )
{
	m_strTrackName = pPathStart->GetEntityName();
	if ( IsCurSchedule( SCHED_STRIDER_RANGE_ATTACK2, false ) )
	{
		SetGoalEnt( pPathStart );
		return true;
	}
	return BaseClass::ScheduledFollowPath( scheduleType, pPathStart, movementActivity );
}


//---------------------------------------------------------
// Disables the minigun until EnableMinigun input is received.
//---------------------------------------------------------
void CNPC_Strider::InputDisableMinigun( inputdata_t &inputdata )
{
	m_bMinigunEnabled = false;
	m_pMinigun->Enable( this, false );
}


//---------------------------------------------------------
// Enables the minigun.
//---------------------------------------------------------
void CNPC_Strider::InputEnableMinigun( inputdata_t &inputdata )
{
	m_bMinigunEnabled = true;
	m_pMinigun->Enable( this, true );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputSetMinigunTime( inputdata_t &inputdata )
{
	m_miniGunShootDuration = inputdata.value.Float();
	m_pMinigun->SetShootDuration( m_miniGunShootDuration );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputSetMinigunTarget( inputdata_t &inputdata )
{
	CBaseEntity *pTargetEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );

	m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 0 );
	m_pMinigun->ShootAtTarget( this, pTargetEntity, m_miniGunShootDuration );
	m_miniGunShootDuration = 0;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputSetCannonTarget( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );

	if ( pTarget )
	{
		if ( m_hCannonTarget == pTarget )
			return;
			
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		CNPC_Strider *pStrider;
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			if ( ppAIs[i] != this && ppAIs[i]->ClassMatches( GetClassname() ) )
			{
				pStrider = (CNPC_Strider *)(ppAIs[i]);
				if ( pStrider->GetCannonTarget() == pTarget )
					return; // Already accounted for
			}
		}

		if( pTarget->MyCombatCharacterPointer() && pTarget->IsAlive() )
		{
			m_hCannonTarget = pTarget;
			m_AttemptCannonLOSTimer.Force();
			return;
		}
	}

	m_hCannonTarget = NULL;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputFlickRagdoll( inputdata_t &inputdata )
{
	if ( m_hRagdoll.Get() )
	{
		SetCondition( COND_STRIDER_DO_FLICK );
	}
}


/*
		IPhysicsObject *pPhysics0 = FindPhysicsObjectByName( STRING(m_nameAttach1), this );
		IPhysicsObject *pPhysics1 = FindPhysicsObjectByName( STRING(m_nameAttach2), this );

		if ( !pPhysics0 )
		{
			pPhysics0 = g_PhysWorldObject;
		}
		if ( !pPhysics1 )
		{
			pPhysics1 = g_PhysWorldObject;
		}

		if ( pPhysics0 != pPhysics1 )
		{
			m_disabled = !bEnable;
			m_succeeded = true;
			if ( bEnable )
			{
				PhysEnableEntityCollisions( pPhysics0, pPhysics1 );
			}
			else
			{
				PhysDisableEntityCollisions( pPhysics0, pPhysics1 );
			}
		}
		else
		{
			m_succeeded = false;
		}
*/
/*
#pragma warning(push)
#pragma warning(disable : 4706) // I know what I'm doing
//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableCollisionWith( inputdata_t &inputdata )
{
	IPhysicsObject *pIgnorePhys  = FindPhysicsObjectByName( inputdata.value.String(), this );
	if ( !pIgnorePhys )
		return;

	int idx;
	CBoneFollower *pFol;

	for (idx = 0 ; pFol = GetBoneFollowerByIndex(idx) ; ++idx) // stop when the function starts returning null (idx is no longer good)
	{
		IPhysicsObject *pFollowPhys = pFol->VPhysicsGetObject();
		Assert(pFollowPhys);
		PhysDisableEntityCollisions( pIgnorePhys, pFollowPhys );
	}
}
#pragma warning(pop)
*/


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableCollisionWith( inputdata_t &inputdata )
{
	IPhysicsObject *pIgnorePhys  = FindPhysicsObjectByName( inputdata.value.String(), this );
	if ( !pIgnorePhys )
		return;

	
	// CBoneFollower *pFol;

	for (int idx = m_BoneFollowerManager.GetNumBoneFollowers() - 1 ; idx >= 0 ; --idx) // stop when the function starts returning null (idx is no longer good)
	{
		IPhysicsObject *pFollowPhys = GetBoneFollowerByIndex(idx)->VPhysicsGetObject();
		Assert(pFollowPhys);
		PhysDisableEntityCollisions( pIgnorePhys, pFollowPhys );
	}
}


 
//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputEnableCollisionWith( inputdata_t &inputdata )
{
	IPhysicsObject *pIgnorePhys  = FindPhysicsObjectByName( inputdata.value.String(), this );
	if ( !pIgnorePhys )
		return;

	int idx;
	// CBoneFollower *pFol;

	for (idx = m_BoneFollowerManager.GetNumBoneFollowers() - 1 ; idx >= 0 ; --idx) // stop when the function starts returning null (idx is no longer good)
	{
		/*
		pFol = GetBoneFollowerByIndex(idx);
		Assert(pFol);
		IPhysicsObject *pFollowPhys = pFol->VPhysicsGetObject();
		Assert(pFollowPhys);
		*/
		IPhysicsObject *pFollowPhys = GetBoneFollowerByIndex(idx)->VPhysicsGetObject();
		PhysEnableEntityCollisions( pIgnorePhys, pFollowPhys );
	}
} 

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputCrouch( inputdata_t &inputdata )
{
	if ( !IsCurSchedule( SCHED_STRIDER_CROUCH ) && !IsInCrouchedPosture() )
		SetCondition( COND_STRIDER_SHOULD_CROUCH );
	m_bCrouchLocked = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputCrouchInstantly( inputdata_t &inputdata )
{
	if ( !IsCurSchedule( SCHED_STRIDER_CROUCH ) && !IsInCrouchedPosture() )
	{
		m_bFastCrouch = true;
		SetCondition( COND_STRIDER_SHOULD_CROUCH );
	}
	m_bCrouchLocked = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputStand( inputdata_t &inputdata )
{
	if ( !IsCurSchedule( SCHED_STRIDER_STAND ) && !IsInStandingPosture() )
		SetCondition( COND_STRIDER_SHOULD_STAND );
	m_bCrouchLocked = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputSetHeight( inputdata_t &inputdata )
{
	SetIdealHeight( inputdata.value.Float() );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputSetTargetPath( inputdata_t &inputdata )
{
	m_strTrackName = MAKE_STRING( inputdata.value.String() );
	SetGoalEnt( NULL );

	if( !IsStriderCrouching() && !IsStriderStanding() && !IsInCrouchedPosture() )
	{
		SetTargetPath();
	}

	// Otherwise, we just leave the track name set and the AI will
	// get to it as soon as possible (as soon as the strider can be
	// made to stand).
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputClearTargetPath( inputdata_t &inputdata )
{
	if (GetNavigator()->IsGoalActive() && GetNavigator()->GetGoalType() == GOALTYPE_PATHCORNER )
	{
		GetNavigator()->ClearGoal();
		ClearSchedule( "Target path cleared via input" );
	}
	m_strTrackName = MAKE_STRING( STRIDER_NO_TRACK_NAME );
	SetGoalEnt(NULL);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableCrouchWalk( inputdata_t &inputdata )
{
	m_bNoCrouchWalk = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputEnableCrouchWalk( inputdata_t &inputdata )
{
	m_bNoCrouchWalk = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputEnableAggressiveBehavior( inputdata_t &inputdata )
{
	m_bUseAggressiveBehavior = true;
	GetEnemies()->SetEnemyDiscardTime( 601.0f );	// Make the assert in SetFreeKnowledgeDuration() happy.
	GetEnemies()->SetFreeKnowledgeDuration( 600.0f );

	AI_FreePassParams_t params = m_PlayerFreePass.GetParams();
	params.duration = 0.8f;
	params.coverDist = 1200.0f;
	m_PlayerFreePass.SetParams( params );

	GetTacticalServices()->AllowFindLateralLos( false );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableAggressiveBehavior( inputdata_t &inputdata )
{
	m_bUseAggressiveBehavior = false;
	GetEnemies()->SetFreeKnowledgeDuration( strider_free_knowledge.GetFloat() );
	GetEnemies()->SetEnemyDiscardTime( AI_DEF_ENEMY_DISCARD_TIME );

	AI_FreePassParams_t params = m_PlayerFreePass.GetParams();
	params.duration = strider_free_pass_duration.GetFloat();
	params.coverDist = strider_free_pass_cover_dist.GetFloat();
	m_PlayerFreePass.SetParams( params );

	GetTacticalServices()->AllowFindLateralLos( true );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputStopShootingMinigunForSeconds( inputdata_t &inputdata )
{
	m_pMinigun->StopShootingForSeconds( this, NULL, inputdata.value.Float() );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableCrouch( inputdata_t &inputdata )
{
	m_bDontCrouch = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputDisableMoveToLOS( inputdata_t &inputdata )
{
	m_bNoMoveToLOS = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputExplode( inputdata_t &inputdata )
{
	CTakeDamageInfo killInfo;
	killInfo.SetAttacker( this );
	killInfo.SetInflictor( this );
	killInfo.SetDamage( GetHealth() );
	TakeDamage( killInfo );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::InputScaleGroundSpeed( inputdata_t &inputdata )
{
	m_flTargetSpeedScale = inputdata.value.Float();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	bool bIsVisible = BaseClass::FVisible( pEntity, traceMask, ppBlocker );
	
	if ( bIsVisible && pEntity == m_PlayerFreePass.GetPassTarget() )
	{
		bIsVisible = m_PlayerFreePass.ShouldAllowFVisible( bIsVisible );

		if( !bIsVisible && IsUsingAggressiveBehavior() && FInViewCone(pEntity) )
		{
			AlertSound();
		}
	}

	return bIsVisible;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	if ( m_BodyTargetBone != -1 )
	{
		Vector position;
		QAngle angles;
		GetBonePosition( m_BodyTargetBone, position, angles );
		return position;
	}
	return BaseClass::BodyTarget( posSrc, bNoisy );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::IsValidEnemy( CBaseEntity *pTarget )
{
	if ( HasCannonTarget() )
	{
		return IsCannonTarget(pTarget);
	}

	if ( pTarget->IsPlayer() )
	{
		if ( AIGetNumFollowers( this, m_iszHunterClassname ) > 0 )
			return false;
	}

	CBaseCombatCharacter *pEnemy = ToBaseCombatCharacter( pTarget );
	if ( pEnemy )
	{
		// Test our enemy filter
		if ( m_hEnemyFilter.Get()!= NULL && m_hEnemyFilter->PassesFilter( this, pEnemy ) == false )
			return false;

		return true;
	}

	return BaseClass::IsValidEnemy( pTarget );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer )
{
	if( pInformer && FClassnameIs( pInformer, "npc_cscanner" ) )
	{
		EmitSound( "NPC_Strider.Alert" );
		// Move Strider's focus to this location and make strider mad at it
		// (but less mad than at any other potential entities in the scene).
#if 1
		GetFocus()->SetAbsOrigin( position + Vector( 0, 0, 32 ) );
#else
		trace_t tr;
		AI_TraceLine( EyePosition(), position + Vector( 0, 0, 32 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		GetFocus()->SetAbsOrigin( tr.endpos );
#endif

		SetCondition( COND_STRIDER_ENEMY_UPDATED );

		m_EnemyUpdatedTimer.Set( 20 );

		AddEntityRelationship( GetFocus(), D_HT, -1 );

		if( pEnemy->IsPlayer() )
		{
			m_PlayerFreePass.Revoke();
		}
		
		BaseClass::UpdateEnemyMemory( GetFocus(), GetFocus()->GetAbsOrigin(), pInformer );

		// Change the informer to myself so that information provided by a scanner is 
		// as good as firsthand knowledge insofar as enemy memory is concerned.
		pInformer = this;
	}

	return BaseClass::UpdateEnemyMemory( pEnemy, position, pInformer );
}

//---------------------------------------------------------
// HACKHACK: The base class looks at distance from the head of the strider
// But when stomping, we need distance from the feet.  Recompute it here.
// UNDONE: make enemy distance aware of strider
//---------------------------------------------------------
float CNPC_Strider::StriderEnemyDistance( CBaseEntity *pEnemy )
{
	Vector enemyDelta = pEnemy->WorldSpaceCenter() - WorldSpaceCenter();
	
	// NOTE: We ignore rotation for computing height.  Assume it isn't an effect
	// we care about, so we simply use OBBSize().z for height.  
	// Otherwise you'd do this:
	// float enemyHeight = enemyMaxs.z - enemyMins.z;

	float enemyHeight = pEnemy->CollisionProp()->OBBSize().z;
	Vector striderSurroundMins, striderSurroundMaxs;
	CollisionProp()->WorldSpaceSurroundingBounds( &striderSurroundMins, &striderSurroundMaxs );
	float myHeight = striderSurroundMaxs.z - striderSurroundMins.z;
	
	// max distance our centers can be apart with the boxes still overlapping
	float flMaxZDist = ( enemyHeight + myHeight ) * 0.5f;

	// see if the enemy is closer to my head, feet or in between
	if ( enemyDelta.z > flMaxZDist )
	{
		// enemy feet above my head, compute distance from my head to his feet
		enemyDelta.z -= flMaxZDist;
	}
	else if ( enemyDelta.z < -flMaxZDist )
	{
		// enemy head below my feet, return distance between my feet and his head
		enemyDelta.z += flMaxZDist;
	}
	else
	{
		// boxes overlap in Z, no delta
		enemyDelta.z = 0;
	}

	return enemyDelta.Length();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::FCanCheckAttacks()
{
	// Strider has direct and indirect attacks, so he's always checking
	// as long as the enemy is in front of him.
	if( FInViewCone( GetEnemy() ) )
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::RangeAttack2Conditions( float flDot, float flDist )
{
	// All of this code has moved to GatherConditions(), since the 
	// strider uses the cannon on things that aren't the enemy!
	return COND_NONE;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( m_bDisableBoneFollowers )
	{
		return COND_NONE;
	}

	if (gpGlobals->curtime < m_nextStompTime)
	{
		return COND_NONE;
	}

	if ( IsInCrouchedPosture() )
	{
		return COND_NONE;
	}

	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
		return COND_NONE;

	// No more stabbing players.
	if ( pEnemy->IsPlayer() && !HasSpawnFlags(SF_CAN_STOMP_PLAYER) )
		return COND_NONE;

	if( !HasCondition( COND_SEE_ENEMY ) )
	{
		return COND_NONE;
	}

	// recompute this because the base class function does not work for the strider
	flDist = StriderEnemyDistance( pEnemy );
	
	if ( flDist > STRIDER_STOMP_RANGE )
	{
		return COND_NONE;
	}

	// strider will cross his feet, but only 6ft over
	Vector right;
	GetVectors( NULL, &right, NULL );
	if ( DotProduct( pEnemy->GetAbsOrigin() - GetAbsOrigin(), right ) > 72 )
	{
		return COND_NONE;
	}

	// Don't skewer if crouched too low.
	if( GetHeight() < GetMaxHeight() - ( (GetMaxHeight()-GetMinHeight()) / 2 ) )
	{
		return COND_NONE;
	}

	// Don't skewer if target is too high above or below ground.
	Vector vecGround = GetAbsOrigin();
	MoveToGround( &vecGround, this, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
	if( fabs( vecGround.z - GetEnemy()->GetAbsOrigin().z ) > 64.0f )
	{
		return COND_NONE;
	}

	// too far, but don't change schedules/movement
	return COND_CAN_MELEE_ATTACK1;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::MeleeAttack2Conditions( float flDot, float flDist )
{
	// HACKHACK: Disabled until we get a good right-leg animation
	return COND_NONE;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions)
{
	CBaseEntity *pTargetEnt;
	Vector vRootOffset;
	Vector vBarrelOffset;

	vRootOffset.x = vRootOffset.y = 0;
	if ( GetCannonTarget() )
	{
		//Assert( targetPos == GetCannonTarget()->GetAbsOrigin() );
		pTargetEnt = GetCannonTarget();
		vRootOffset.z = gm_zCannonDist;
		vBarrelOffset = gm_vLocalRelativePositionCannon;
	}
	else
	{
		pTargetEnt = GetEnemy();
		vRootOffset.z = gm_zMinigunDist;
		vBarrelOffset = gm_vLocalRelativePositionMinigun;
	}

	trace_t tr;
	AI_TraceLine( ownerPos + vRootOffset, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	// Hit the enemy, or hit nothing (traced all the way to a nonsolid enemy like a bullseye)
	if ( ( pTargetEnt && tr.m_pEnt == pTargetEnt) || tr.fraction == 1.0 || CanShootThrough( tr, targetPos ) )
	{
		Vector vBarrelPos;
		matrix3x4_t losTestToWorld;

		Vector xaxis;
		VectorSubtract( targetPos, ownerPos, xaxis );

		// @TODO (toml 03-07-04): Add an angle test
		//float flAngle = acos( xaxis.z / xaxis.Length() );

		xaxis.z = 0.0f;
		float flLength = VectorNormalize( xaxis );
		if ( flLength < 1e-3 )
			return false;

		Vector yaxis( -xaxis.y, xaxis.x, 0.0f );

		MatrixInitialize( losTestToWorld, ownerPos, xaxis, yaxis, Vector(0,0,1) );

		VectorTransform( vBarrelOffset, losTestToWorld, vBarrelPos );

		AI_TraceLine( vBarrelPos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if ( ( pTargetEnt && tr.m_pEnt == pTargetEnt) || tr.fraction == 1.0 || CanShootThrough( tr, targetPos ) )
		{
			if ( strider_show_weapon_los_condition.GetBool() )
			{
				NDebugOverlay::Line( ownerPos + vRootOffset, targetPos, 255, 0, 255, false, 0.1 );
				NDebugOverlay::Line( vBarrelPos, targetPos, 128, 0, 128, false, 0.1 );
			}
			return true;
		}
		else
		{
			if ( strider_show_weapon_los_condition.GetBool() )
			{
				NDebugOverlay::Line( ownerPos + vRootOffset, targetPos, 255, 0, 255, false, 0.1 );
				NDebugOverlay::Line( vBarrelPos, tr.endpos, 128, 0, 0, false, 0.1 );
			}
		}
	}
	else
	{
		if ( strider_show_weapon_los_condition.GetBool() )
		{
			NDebugOverlay::Line( ownerPos + vRootOffset, tr.endpos, 255, 0, 0, false, 0.1 );
		}
	}

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CurrentWeaponLOSCondition(const Vector &targetPos, bool bSetConditions)
{
	return WeaponLOSCondition( GetAdjustedOrigin(), targetPos, bSetConditions );
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::IsValidShootPosition( const Vector &vecCoverLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	if ( ( pNode && !pHint ) || ( pHint && pHint->HintType() != HINT_STRIDER_NODE ) )
		return false;
	return BaseClass::IsValidShootPosition( vecCoverLocation, pNode, pHint );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::TestShootPosition(const Vector &vecShootPos, const Vector &targetPos )	
{ 
	if ( BaseClass::TestShootPosition( vecShootPos, targetPos ) )
	{
		return true;
	}

	// What if I crouched?
	float zDelta = ( ( vecShootPos.z - targetPos.z ) + gm_vLocalRelativePositionMinigun.z ) - 12;
	if ( zDelta > 0 )
	{
		if ( zDelta > GetHeightRange() )
			zDelta = GetHeightRange();

		Vector vecTestPos = vecShootPos;
		vecTestPos.z -= zDelta;

		if ( BaseClass::TestShootPosition( vecTestPos, targetPos ) )
		{
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::Weapon_ShootPosition( )
{
	Vector vecShootPos;
	GetAttachment( gm_CannonAttachment, vecShootPos );

	return vecShootPos;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::DoImpactEffect( trace_t &tr, int nDamageType )
{
	BaseClass::DoImpactEffect( tr, nDamageType );

	// Add a halo
	CBroadcastRecipientFilter filter;
	te->BeamRingPoint( filter, 0.0, 
		tr.endpos,							//origin
		0,									//start radius
		64,									//end radius
		s_iImpactEffectTexture,				//texture
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

	g_pEffects->EnergySplash( tr.endpos, tr.plane.normal );
	
	// Punch the effect through?
	if( tr.m_pEnt && !tr.m_pEnt->MyNPCPointer() )
	{
		Vector vecDir = tr.endpos - tr.startpos;
		VectorNormalize( vecDir );

		trace_t retrace;

		Vector vecReTrace = tr.endpos + vecDir * 12;

		if( UTIL_PointContents( vecReTrace ) == CONTENTS_EMPTY )
		{
			AI_TraceLine( vecReTrace, vecReTrace - vecDir * 24, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &retrace );

			BaseClass::DoImpactEffect( retrace, nDamageType );
		}
	}
}

//---------------------------------------------------------
// Trace didn't hit the intended target, but should the strider
// shoot anyway? We use this to get the strider to destroy 
// breakables that are between him and his target.
//---------------------------------------------------------
bool CNPC_Strider::CanShootThrough( const trace_t &tr, const Vector &vecTarget )
{
	if( GetCannonTarget() )
	{
		// Cannon does not have this behavior.
		return false;
	}

	if( !tr.m_pEnt )
	{
		return false;
	}

	if( !tr.m_pEnt->GetHealth() )
	{
		return false;
	}

	// Would a trace ignoring this entity continue to the target?
	trace_t continuedTrace;
	AI_TraceLine( tr.endpos, vecTarget, MASK_SHOT, tr.m_pEnt, COLLISION_GROUP_NONE, &continuedTrace );

	if( continuedTrace.fraction != 1.0 )
	{
		if( continuedTrace.m_pEnt != GetEnemy() )
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::CreateFocus()
{
	m_hFocus = CreateEntityByName( "bullseye_strider_focus" );

	ASSERT( m_hFocus != NULL );
	m_hFocus->AddSpawnFlags( SF_BULLSEYE_NONSOLID | SF_BULLSEYE_NODAMAGE );
	m_hFocus->SetAbsOrigin( GetAbsOrigin() );
	m_hFocus->Spawn();
}

//---------------------------------------------------------
//---------------------------------------------------------
CNPC_Bullseye *CNPC_Strider::GetFocus()
{
	ASSERT( m_hFocus != NULL );

	CNPC_Bullseye *pBull = dynamic_cast<CNPC_Bullseye*>(m_hFocus.Get());

	return pBull;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::GetWeaponLosZ( const Vector &vOrigin, float minZ, float maxZ, float increment, CBaseEntity *pTarget, float *pResult )
{
	Vector vTestPos;
	Vector vTargetPos = pTarget->BodyTarget(vOrigin, false);
	Vector vIncrement( 0, 0, increment );

	// Try right where am
	if ( vOrigin.z >= minZ && vOrigin.z <= maxZ )
	{
		vTestPos = vOrigin;
		if ( WeaponLOSCondition( vTestPos, vTargetPos, false ) )
		{
			if ( strider_show_weapon_los_z.GetBool() )
				NDebugOverlay::Line( vTestPos, vTargetPos, 0, 255, 0, false, 0.1 );
			*pResult = vTestPos.z;
			return true;
		}
	}

	// Try at adjusted height of target
	vTestPos.z = ( vTargetPos.z - gm_vLocalRelativePositionMinigun.z ) + 12;

	if ( vTestPos.z >= minZ && vTestPos.z <= maxZ )
	{
		if ( WeaponLOSCondition( vTestPos, vTargetPos, false ) )
		{
			if ( strider_show_weapon_los_z.GetBool() )
				NDebugOverlay::Line( vTestPos, vTargetPos, 0, 255, 0, false, 0.1 );
			*pResult = vTestPos.z;
			return true;
		}
	}
	

	// Try at max height
	vTestPos.z = maxZ;
	if ( WeaponLOSCondition( vTestPos, vTargetPos, false ) )
	{
		if ( strider_show_weapon_los_z.GetBool() )
			NDebugOverlay::Line( vOrigin, vTargetPos, 0, 255, 0, false, 0.1 );
		*pResult = vTestPos.z;
		return true;
	}

	// Try min height
	vTestPos.z = minZ;
	if ( WeaponLOSCondition( vTestPos, vTargetPos, false ) )
	{
		if ( strider_show_weapon_los_z.GetBool() )
			NDebugOverlay::Line( vTestPos, vTargetPos, 0, 255, 0, false, 0.1 );
		*pResult = vTestPos.z;
		return true;
	}
	
	// Test up from min
	vTestPos = vOrigin + vIncrement;
	while ( vTestPos.z <= maxZ && !WeaponLOSCondition( vTestPos, vTargetPos, false ) )
	{
		vTestPos += vIncrement;
	}
	
	if ( vTestPos.z <= maxZ )
	{
		if ( strider_show_weapon_los_z.GetBool() )
			NDebugOverlay::Line( vTestPos, vTargetPos, 0, 255, 0, false, 0.1 );
		*pResult = vTestPos.z;
		return true;
	}

	// Test down
	vTestPos = vOrigin - vIncrement;
	while ( vTestPos.z >= minZ && !WeaponLOSCondition( vTestPos, vTargetPos, false ) )
	{
		vTestPos -= vIncrement;
	}
	
	if ( vTestPos.z >= minZ )
	{
		if ( strider_show_weapon_los_z.GetBool() )
			NDebugOverlay::Line( vTestPos, vTargetPos, 0, 255, 0, false, 0.1 );
		*pResult = vTestPos.z;
		return true;
	}
		
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::AlertSound()
{
	if( ( gpGlobals->curtime - m_flTimeLastAlertSound ) > 2.0f )
	{
		EmitSound( "NPC_Strider.Alert" );
		m_flTimeLastAlertSound = gpGlobals->curtime;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::PainSound( const CTakeDamageInfo &info )
{
	// This means that we've exploded into pieces and have no way to whimper
	if ( ShouldExplodeFromDamage( info ) )
		return;

	EmitSound( "NPC_Strider.Pain" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::DeathSound( const CTakeDamageInfo &info )
{
	// This means that we've exploded into pieces and have no way to whimper
	if ( m_bExploding )
	{
		EmitSound( "NPC_Strider.StriderBusterExplode" );
		EmitSound( "explode_5" );
		return;
	}

	EmitSound( "NPC_Strider.Death" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::HuntSound()
{
	if( m_PlayerFreePass.HasPass() )
	{
		EmitSound( "NPC_Strider.Hunt" );
		m_flTimeNextHuntSound = gpGlobals->curtime + random->RandomFloat( 8.0f, 12.0f );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;

	bool ricochetBullets = true;

	if ( info.GetAttacker()->IsPlayer() )
	{
		if ( !HasMemory( bits_MEMORY_PROVOKED ) )
		{
			GetEnemies()->ClearMemory( info.GetAttacker() );
			Remember( bits_MEMORY_PROVOKED );
			SetCondition( COND_LIGHT_DAMAGE );
		}
	}

#ifdef HL2_EPISODIC

	// Attempt to hit strider busters in the area
	float flDistSqr;
	for ( int i = 0; i < m_hAttachedBusters.Count(); i++ )
	{
		if ( m_hAttachedBusters[i] == NULL )
			continue;

		flDistSqr = ( m_hAttachedBusters[i]->WorldSpaceCenter() - inputInfo.GetDamagePosition() ).LengthSqr();
		if ( flDistSqr < Square( 50.0f ) )
		{
			// Kill the buster and stop the trace going through
			CTakeDamageInfo killInfo = inputInfo;
			killInfo.SetDamage( 100 );

			m_hAttachedBusters[i]->TakeDamage( killInfo );
			return;
		}
	}

#endif	//HL2_EPISODIC

	if ( (info.GetDamageType() & DMG_BULLET) && ricochetBullets )
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
		if ( ptr->hitgroup != HITGROUP_HEAD )
		{
			info.SetDamage( 0.01 );
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// don't take damage from my own weapons!!!
	if ( info.GetInflictor() && info.GetInflictor()->GetOwnerEntity() == this )
		return 0;

	// special interaction with combine balls
	if ( UTIL_IsCombineBall( info.GetInflictor() ) )
		return TakeDamageFromCombineBall( info );

	if ( info.GetDamageType() == DMG_GENERIC )
		return BaseClass::OnTakeDamage_Alive( info );

	if( IsUsingAggressiveBehavior() )
	{
		// Any damage the player inflicts gets my attention, even if it doesn't actually harm me.
		if ( info.GetAttacker()->IsPlayer() )
		{
			UpdateEnemyMemory( info.GetAttacker(), info.GetAttacker()->GetAbsOrigin() );
		}
	}

	//int healthIncrement = 5 - ( m_iHealth / ( m_iMaxHealth / 5 ) );
	if ( (info.GetDamageType() & DMG_BLAST) && info.GetMaxDamage() > 50 )
	{
		Vector headPos = BodyTarget( info.GetDamagePosition(), false );
		
		float dist = CalcDistanceToAABB( WorldAlignMins(), WorldAlignMaxs(), info.GetDamagePosition() - headPos );
		// close enough to do damage?
		if ( dist < 200 )
		{
			bool bPlayer = info.GetAttacker()->IsPlayer();
			if ( bPlayer )
			{
				m_PlayerFreePass.Revoke();
				AddFacingTarget( info.GetAttacker(), info.GetAttacker()->GetAbsOrigin(), 1.0, 2.0 );

				UpdateEnemyMemory( info.GetAttacker(), info.GetAttacker()->GetAbsOrigin() );
			}
			else
				AddFacingTarget( info.GetAttacker(), info.GetAttacker()->GetAbsOrigin(), 0.5, 2.0 );

			// Default to NPC damage value
			int damage = 20;

			if( HasSpawnFlags(SF_TAKE_MINIMAL_DAMAGE_FROM_NPCS) )
				damage = 1;

			if( bPlayer )
			{
				if( g_pGameRules->IsSkillLevel(SKILL_EASY) )
				{
					damage = GetMaxHealth() / sk_strider_num_missiles1.GetFloat();
				}
				else if( g_pGameRules->IsSkillLevel(SKILL_HARD) )
				{
					damage = GetMaxHealth() / sk_strider_num_missiles3.GetFloat();
				}
				else // Medium, or unspecified
				{
					damage = GetMaxHealth() / sk_strider_num_missiles2.GetFloat();
				}
			}

			m_iHealth -= damage;

			m_OnDamaged.FireOutput( info.GetAttacker(), this);

			if( info.GetAttacker()->IsPlayer() )
			{
				m_OnDamagedByPlayer.FireOutput( info.GetAttacker(), this );

				// This also counts as being harmed by player's squad.
				m_OnDamagedByPlayerSquad.FireOutput( info.GetAttacker(), this );
			}
			else
			{
				// See if the person that injured me is an NPC.
				CAI_BaseNPC *pAttacker = dynamic_cast<CAI_BaseNPC *>( info.GetAttacker() );
				CBasePlayer *pPlayer = AI_GetSinglePlayer();

				if( pAttacker && pAttacker->IsAlive() && pPlayer )
				{
					if( pAttacker->GetSquad() != NULL && pAttacker->IsInPlayerSquad() )
					{
						m_OnDamagedByPlayerSquad.FireOutput( info.GetAttacker(), this );
					}
				}
			}

			if ( m_iHealth <= ( m_iMaxHealth / 2 ) )
			{
				m_OnHalfHealth.FireOutput(this, this);
			}

			RestartGesture( ACT_GESTURE_SMALL_FLINCH );
			PainSound( info );

			// Interrupt our gun during the flinch
			m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 1.1f );

			GetEnemies()->OnTookDamageFrom( info.GetAttacker() );

			if( !IsSmoking() && m_iHealth <= sk_strider_health.GetInt() / 2 )
			{
				StartSmoking();
			}
			return damage;
		}

// NOTE: Currently radius damage doesn't even call this because it uses the origin, not the box for distance
#if 0
		NDebugOverlay::Box( headPos, WorldAlignMins(), WorldAlignMaxs(), 255, 0, 0, 0, 5.0 );
		NDebugOverlay::Cross3D( inf
			o.GetDamagePosition(), 24, 0, 255, 0, false, 5.0 );
		// too far from head, apply damage to nearest leg?
#endif
	}

#if 0
	if ( (info.GetDamageType() & DMG_BULLET) && info.GetDamage() > 1  && m_iHealth > 1 )
	{
		m_iHealth -= 1;
		return 1;
	}
#endif

	return 0;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Strider::TakeDamageFromCombineBall( const CTakeDamageInfo &info )
{
	float damage = info.GetDamage();

	// If it's only an AR2 alt-fire, we don't take much damage
	if ( UTIL_IsAR2CombineBall( info.GetInflictor() ) )
	{
		damage = strider_ar2_altfire_dmg.GetFloat();
	}
	else
	{
		// Always start smoking when we're struck by a normal combine ball
		StartSmoking();
	}

	if( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		// Route combine ball damage through the regular skill level code.
		damage = g_pGameRules->AdjustPlayerDamageInflicted(damage);
	}

	AddFacingTarget( info.GetInflictor(), info.GetInflictor()->GetAbsOrigin(), 0.5, 2.0 );
	if ( !UTIL_IsAR2CombineBall( info.GetInflictor() ) )
		RestartGesture( ACT_GESTURE_BIG_FLINCH );
	else
		RestartGesture( ACT_GESTURE_SMALL_FLINCH );
	
	PainSound( info );

	m_iHealth -= damage;

	return damage;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::Event_Killed( const CTakeDamageInfo &info )
{
	// Do a special death if we're killed by a combine ball in the Citadel
	if ( info.GetInflictor() && UTIL_IsCombineBall( info.GetInflictor() ) )
	{
		if ( m_lifeState == LIFE_DYING )
			return;

		// Tracker 23610:  Strider playing death sounds twice (AI_BaseNPC calls it in Event_Killed, too)
		// DeathSound( info );
		m_lifeState = LIFE_DYING;

		// Start dying
		RestartGesture( (Activity) ACT_STRIDER_GESTURE_DEATH );

		// Stop our mini-cannon
		m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 100.0f );
	}
	else
	{
		StopSmoking();

		m_BoneFollowerManager.DestroyBoneFollowers();
	
	}

	if( IsUsingAggressiveBehavior() )
	{
		// Lifted this code from the gunship.
		Vector vecExplode;
		GetAttachment( "minigun", vecExplode );
		ExplosionCreate( vecExplode, QAngle(0,0,1), this, 100, 128, false );
	}

	// Determine if we're going to explode into pieces
	m_bExploding = ShouldExplodeFromDamage( info );

	BaseClass::Event_Killed( info );

	// Stop our large cannon
	EntityMessageBegin( this, true );
		WRITE_BYTE( STRIDER_MSG_DEAD );
	MessageEnd();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::RagdollDeathEffect( CRagdollProp *pRagdoll, float flDuration )
{
	UTIL_ScreenShake( EyePosition(), 50, 150.0, 1.0, 1024, SHAKE_START );

	int i;
	for( i = 0 ; i < 2 ; i++ )
	{
		SmokeTrail *pSmoke = SmokeTrail::CreateSmokeTrail();
		if ( pSmoke )
		{
			if( i == 0 )
			{
				pSmoke->m_SpawnRate			= 16;
				pSmoke->m_Opacity 			= 0.25;
				pSmoke->m_StartColor.Init( 0.45f, 0.45f, 0.45f );
			}
			else
			{
				pSmoke->m_SpawnRate			= 32;
				pSmoke->m_Opacity 			= 0.3;
				pSmoke->m_StartColor.Init( 0.5f, 0.5f, 0.5f );
			}

			pSmoke->m_ParticleLifetime	= 3.0;
			pSmoke->m_StartSize			= 16;
			pSmoke->m_EndSize			= 64;
			pSmoke->m_SpawnRadius		= 20;
			pSmoke->m_MinSpeed			= 8;
			pSmoke->m_MaxSpeed			= 64;
			pSmoke->m_EndColor.Init( 0, 0, 0 );

			pSmoke->SetLifetime( flDuration );

			if( i == 0 )
			{
				pSmoke->FollowEntity( pRagdoll, "MiniGunBase" );
			}
			else
			{
				pSmoke->FollowEntity( pRagdoll, "vehicle_driver_eyes" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Strider::ShouldExplodeFromDamage( const CTakeDamageInfo &info )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	if ( pInflictor == NULL )
		return false;

	// Combine balls make us explode
	if ( UTIL_IsCombineBall( info.GetInflictor() ) )
		return true;

	// Stickybombs make us explode
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( pAttacker != NULL && (FClassnameIs( pAttacker, "weapon_striderbuster" ) ||
								FClassnameIs( pAttacker, "npc_grenade_magna" )))
		return true;

	if ( pInflictor == this && pAttacker == this )
		return true;

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
ConVarRef mat_dxlevel( "mat_dxlevel" );
bool CNPC_Strider::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector ) 
{ 
	// Combine balls make us explode
	if ( m_bExploding )
	{
		Explode();
	}
	else
	{
		// Otherwise just keel over
		CRagdollProp *pRagdoll = NULL;
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer && mat_dxlevel.GetInt() > 0 )
		{
			int dxlevel = mat_dxlevel.GetInt();
			int maxRagdolls = ( dxlevel >= 90 ) ? 2 : ( dxlevel >= 80 ) ? 1 : 0;

			if ( maxRagdolls > 0 )
			{
				CUtlVectorFixed<CRagdollProp *, 2> striderRagdolls;
				while ( ( pRagdoll = gEntList.NextEntByClass( pRagdoll ) ) != NULL )
				{
					if ( pRagdoll->GetModelName() == GetModelName() && !pRagdoll->IsFading() )
					{
						Assert( striderRagdolls.Count() < striderRagdolls.NumAllocated() );
						if ( striderRagdolls.Count() < striderRagdolls.NumAllocated() )
							striderRagdolls.AddToTail( pRagdoll );
					}
				}

				if ( striderRagdolls.Count() >= maxRagdolls )
				{
					float distSqrFurthest = FLT_MAX;
					CRagdollProp *pFurthest = NULL;

					for ( int i = 0; i < striderRagdolls.Count(); i++ )
					{
						float distSqrCur = CalcSqrDistanceToAABB( striderRagdolls[i]->WorldAlignMins(), striderRagdolls[i]->WorldAlignMaxs(), pPlayer->GetAbsOrigin() );
						if ( distSqrCur < distSqrFurthest )
						{
							distSqrFurthest = distSqrCur;
							pFurthest = striderRagdolls[i];
						}
					}

					if ( pFurthest )
						pFurthest->FadeOut( 0.75, 1.5 );
				}
			}

			pRagdoll = assert_cast<CRagdollProp *>( CreateServerRagdoll( this, m_nForceBone, info, HL2COLLISION_GROUP_STRIDER ) );
			pRagdoll->DisableAutoFade();

			if ( maxRagdolls == 0 )
			{
				pRagdoll->FadeOut( 6.0, .75 );
				RagdollDeathEffect( pRagdoll, 6.0f );
			}
			else
			{
				RagdollDeathEffect( pRagdoll, 600.0f );
			}
		}
		else
		{
			// Otherwise just keel over
			pRagdoll = assert_cast<CRagdollProp *>( CreateServerRagdoll( this, m_nForceBone, info, HL2COLLISION_GROUP_STRIDER ) );
			pRagdoll->DisableAutoFade();
		}
	}
	
	UTIL_Remove(this);

	return true; 
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::StartSmoking( void )
{
	if ( m_hSmoke != NULL )
		return;

	m_hSmoke = SmokeTrail::CreateSmokeTrail();
	
	if ( m_hSmoke )
	{
		m_hSmoke->m_SpawnRate			= 32;
		m_hSmoke->m_ParticleLifetime	= 3.0;
		m_hSmoke->m_StartSize			= 16;
		m_hSmoke->m_EndSize				= 64;
		m_hSmoke->m_SpawnRadius			= 20;
		m_hSmoke->m_MinSpeed			= 8;
		m_hSmoke->m_MaxSpeed			= 64;
		m_hSmoke->m_Opacity 			= 0.3;
		
		m_hSmoke->m_StartColor.Init( 0.25f, 0.25f, 0.25f );
		m_hSmoke->m_EndColor.Init( 0, 0, 0 );
		m_hSmoke->SetLifetime( 500.0f );
		m_hSmoke->FollowEntity( this, "MiniGunBase" );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::StopSmoking( float flDelay )
{
	if ( m_hSmoke )
	{
		m_hSmoke->SetLifetime( flDelay );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::SetHeight( float h )
{
	if ( h > GetMaxHeight() )
		h = GetMaxHeight();
	else if ( h < GetMinHeight() )
		h = GetMinHeight();

	SetPoseParameter( gm_BodyHeightPoseParam, h );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::SetIdealHeight( float h )
{
	if ( h > GetMaxHeight() )
		h = GetMaxHeight();
	else if ( h < GetMinHeight() )
		h = GetMinHeight();

	m_idealHeight = h;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::SetAbsIdealHeight( float z )
{
	float h = GetMaxHeight() - ( z - GetAbsOrigin().z );
	
	SetIdealHeight( h );
}

//---------------------------------------------------------
// At this moment, am I in the PROCESS of crouching? 
// as in, and I transitioning from standing to crouch?
//---------------------------------------------------------
bool CNPC_Strider::IsStriderCrouching()
{
	if( IsCurSchedule( SCHED_STRIDER_CROUCH, false ) )
		return true;

	return false;
}

//---------------------------------------------------------
// As IsStriderCrouching(), but for obvious differences.
//---------------------------------------------------------
bool CNPC_Strider::IsStriderStanding()
{
	if( IsCurSchedule( SCHED_STRIDER_STAND, false ) )
		return true;

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------

bool CNPC_Strider::OverrideMove( float flInterval )
{
	if ( GetCannonTarget() )
	{
		AddFacingTarget( GetCannonTarget(), GetCannonTarget()->GetAbsOrigin(), 1.0, 0.5 );
	}
	else if ( GetEnemy() )
	{
		bool bPlayer = GetEnemy()->IsPlayer();
		float timeSinceSeenEnemy = gpGlobals->curtime - GetEnemyLastTimeSeen();
		if ( ( !bPlayer && timeSinceSeenEnemy < STRIDER_TIME_STOP_FACING_ENEMY ) ||
			 ( bPlayer && !m_PlayerFreePass.HasPass() ) )
		{
			AddFacingTarget( GetEnemy(), GetEnemies()->LastKnownPosition( GetEnemy() ), 1.0, 0.5 );
		}

		if ( !m_bCrouchLocked && !m_hCannonTarget && GetIdealHeight() < GetMaxHeight() && timeSinceSeenEnemy > TIME_CARE_ENEMY )
			SetIdealHeight( GetMaxHeight() );
	}
	else if ( !m_bCrouchLocked && !m_hCannonTarget )
		SetIdealHeight( GetMaxHeight() );

	if ( strider_test_height.GetFloat() > .1 )
		SetIdealHeight( strider_test_height.GetFloat() );

	// If we're not supposed to crouch walk and we're under the threshold for what we consider crouch walking, stand back up!
	if ( !m_bCrouchLocked && m_bNoCrouchWalk && IsMoving() && ( GetIdealHeight() < ( GetMinHeight() + GetHeightRange() * ( strider_pct_height_no_crouch_move.GetFloat() / 100.0 ) ) ) )
	{
		SetIdealHeight( GetMinHeight() + GetHeightRange() * ( strider_pct_height_no_crouch_move.GetFloat() / 100.0 ) );
	}

	float heightMove = GetIdealHeight() - GetHeight();
	float heightMoveSign = ( heightMove < 0 ) ? -1 : 1;
	heightMove = fabsf( heightMove );
	if ( heightMove > 0.01 )
	{
		const float maxVelocity = 300;
		const float minVelocity = 10;

		#define HEIGHTINVDECAY	0.8	// maintain X percent of velocity when slowing down
		#define HEIGHTDECAYTIME	0.4161	// Sum( 1..cycle, HEIGHTINVDECAY^cycle ) 
		#define HEIGHTACCEL		0.5		// accel toward maxVelocity by X percent each cycle

		if (fabsf( m_HeightVelocity ) < minVelocity)
			m_HeightVelocity = minVelocity * heightMoveSign;

		if (heightMove < m_HeightVelocity * heightMoveSign * HEIGHTDECAYTIME )
		{
			m_HeightVelocity = m_HeightVelocity * HEIGHTINVDECAY;

			if (heightMove < m_HeightVelocity * heightMoveSign * flInterval)
			{
				m_HeightVelocity = heightMove * heightMoveSign / flInterval;
			}
		}
		else
		{
			m_HeightVelocity = m_HeightVelocity * (1.0f - HEIGHTACCEL) + maxVelocity * HEIGHTACCEL * heightMoveSign;
			if (heightMove < m_HeightVelocity * heightMoveSign * HEIGHTDECAYTIME)
			{
				m_HeightVelocity = heightMove * heightMoveSign / HEIGHTDECAYTIME;
			}
		}

		float newHeight = GetHeight() + m_HeightVelocity * flInterval;
		SetHeight( newHeight );
	}

	// FIXME: where should this go?
	MaintainTurnActivity( );

	return false;
}


void CNPC_Strider::MaintainTurnActivity( void )
{
	// detect that the npc has turned
	if (m_prevYaw != GetAbsAngles().y)
	{
		float diff = UTIL_AngleDiff( m_prevYaw, GetAbsAngles().y );
		if (diff < 0.0)
		{
			m_doLeft += -diff;
		}
		else
		{
			m_doRight += diff;
		}
		m_prevYaw = GetAbsAngles().y;
	}
	// accumulate turn angle, delay response for short turns
	m_doTurn += m_doRight + m_doLeft;
	if (!IsMoving() && m_doTurn > 180.0f && m_flNextTurnAct < gpGlobals->curtime )
	{
		int iSeq = ACT_INVALID;
		if (m_doLeft > m_doRight)
		{
			iSeq = SelectWeightedSequence( ACT_GESTURE_TURN_LEFT );
		}
		else
		{
			iSeq = SelectWeightedSequence( ACT_GESTURE_TURN_RIGHT );
		}
		if (iSeq != ACT_INVALID)
		{
			int iLayer = AddGestureSequence( iSeq );
			if (iLayer != -1)
			{
				SetLayerPriority( iLayer, 100 );
				// increase speed if we're getting behind or they're turning quickly
				if (m_doTurn > 360.0)
				{
					SetLayerPlaybackRate( iLayer, 1.5 );
				}
				m_flNextTurnAct = gpGlobals->curtime + GetLayerDuration( iLayer );
			}
			else
			{
				// too busy, try again in half a second
				m_flNextTurnAct = gpGlobals->curtime + 0.5;
			}
		}
		m_doTurn = m_doLeft = m_doRight = 0.0;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::IsUnusableNode(int iNodeID, CAI_Hint *pHint)
{
	if ( !BaseClass::IsUnusableNode(iNodeID, pHint) )
	{
		if ( pHint && pHint->HintType() == HINT_STRIDER_NODE )
			return false;
	}
	return true;
}

//---------------------------------------------------------
// Purpose: Compute the nav position for the strider's origin relative to this enemy.
//---------------------------------------------------------
void CNPC_Strider::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	if ( pEnemy )
	{
		if ( ! (pEnemy->GetFlags() & FL_ONGROUND) )
		{
			MoveToGround( &chasePosition, pEnemy, pEnemy->WorldAlignMins(), pEnemy->WorldAlignMaxs() );
		}

		// move down to enemy's feet for enemy origin at chasePosition
		chasePosition.z += pEnemy->WorldAlignMins().z;

	}
	else
	{
		MoveToGround( &chasePosition, NULL, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	}

	// move up to strider stand height
	chasePosition.z += GetMaxHeight() + npc_strider_height_adj.GetFloat();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::HasPendingTargetPath()
{
	bool bReturn = ( !FStrEq( STRING(m_strTrackName), STRIDER_NO_TRACK_NAME ) && !GetGoalEnt() );

	return bReturn;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::SetTargetPath()
{
	SetGoalEnt( NULL );
	CBaseEntity *pGoalEnt = gEntList.FindEntityByName( NULL, m_strTrackName );
	if ( pGoalEnt == NULL )
	{
		DevWarning( "%s: Could not find target path '%s'!\n", GetClassname(), STRING( m_strTrackName ) );

		// Don't try anymore. It just hurts the AI.
		m_strTrackName = MAKE_STRING( STRIDER_NO_TRACK_NAME );

		UTIL_Remove( this );
		return;
	}

	const Vector &absOrigin = GetAbsOrigin();
	CBaseEntity *pClosestEnt = NULL;
	CBaseEntity *pCurEnt = pGoalEnt;
	float distClosestSq = FLT_MAX;

	CUtlRBTree<CBaseEntity *> visits;
	SetDefLessFunc(visits);

	while ( pCurEnt && visits.Find( pCurEnt ) == visits.InvalidIndex() )
	{
		float distCurSq = ( pCurEnt->GetAbsOrigin() - absOrigin ).LengthSqr();
		if ( distCurSq < distClosestSq )
		{
			distClosestSq = distCurSq;
			pClosestEnt = pCurEnt;
		}
		visits.Insert( pCurEnt );
		pCurEnt = GetNavigator()->GetNextPathcorner( pCurEnt );
	}

	ScheduledFollowPath( SCHED_IDLE_WALK, pClosestEnt, ACT_WALK );
}

//---------------------------------------------------------
//---------------------------------------------------------

float CNPC_Strider::GetDefaultNavGoalTolerance()
{
	return 64;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::OnMovementComplete()
{
	if ( GetGoalEnt() && 
		 ( IsCurSchedule( SCHED_IDLE_WALK ) ||
		   IsCurSchedule( SCHED_ALERT_WALK ) ||
		   IsCurSchedule( SCHED_COMBAT_WALK ) ||
		   IsCurSchedule( SCHED_STRIDER_HUNT ) ) )
	{
		m_strTrackName = MAKE_STRING( STRIDER_NO_TRACK_NAME );
		SetGoalEnt( NULL );
	}
	BaseClass::OnMovementComplete();
}


//---------------------------------------------------------
//---------------------------------------------------------
float CNPC_Strider::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	return ( BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence ) * m_flSpeedScale );
}


//---------------------------------------------------------
//---------------------------------------------------------
float CNPC_Strider::MaxYawSpeed()
{
	switch( GetActivity() )
	{
	case ACT_90_LEFT:
	case ACT_90_RIGHT:
		return 10;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 10;

	case ACT_WALK:
		return 10;

	default:
		return 10;		// should be zero, but this makes it easy to see when he's turning with default AI code
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Strider::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "MiniGun" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "StriderMuzzleFlash", data );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread )
{
	if ( pTarget )
	{
		Vector muzzlePos;
		QAngle muzzleAng;

		GetAttachment( "minigun", muzzlePos, muzzleAng );
		
		Vector vecShootDir = *pTarget - muzzlePos;
		VectorNormalize( vecShootDir );

		if( m_bMinigunUseDirectFire )
		{
			// exactly on target w/tracer
			FireBullets( 1, muzzlePos, vecShootDir, vecSpread, 8192, m_miniGunDirectAmmo, 1 );
		}
		else
		{
			// exactly on target w/tracer
			FireBullets( 1, muzzlePos, vecShootDir, vecSpread, 8192, m_miniGunAmmo, 1 );
		}

		//g_pEffects->MuzzleFlash( muzzlePos, muzzleAng, random->RandomFloat( 2.0f, 4.0f ) , MUZZLEFLASH_TYPE_STRIDER );
		DoMuzzleFlash();

		EmitSound( "NPC_Strider.FireMinigun" );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::UpdateMinigunControls( float &yaw, float &pitch ) 
{
	SetPoseParameter( m_poseMiniGunYaw, yaw );
	SetPoseParameter( m_poseMiniGunPitch, pitch );

	yaw = GetPoseParameter( m_poseMiniGunYaw );
	pitch = GetPoseParameter( m_poseMiniGunPitch );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::GetViewCone( StriderMinigunViewcone_t &cone )
{
	cone.origin = EyePosition();
	GetVectors( &cone.axis, NULL, NULL );
	cone.cosAngle = 0.5; // 60 degree cone
	cone.length = 2048;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::OnMinigunStopShooting( CBaseEntity *pTarget )
{
	// Stop hating the focus.
	if( GetFocus() && pTarget == GetFocus() )
	{
		AddEntityRelationship( GetFocus(), D_NU, 0 );
	}

	if( IsUsingAggressiveBehavior() )
	{
		// From now on, direct fire. 
		m_bMinigunUseDirectFire = true;
	}
}

//---------------------------------------------------------
// How fast the minigun fires (rounds per second)
//---------------------------------------------------------
float CNPC_Strider::GetMinigunRateOfFire()
{
	if( IsUsingAggressiveBehavior() && m_bMinigunUseDirectFire )
		return STRIDER_EP1_RATE_OF_FIRE;

	return STRIDER_DEFAULT_RATE_OF_FIRE;
}

//---------------------------------------------------------
// How much of shoot duration is spent firing directly at
// the target (the balance of time is spent stitching towards)
//---------------------------------------------------------
float CNPC_Strider::GetMinigunOnTargetTime()
{
	if( IsUsingAggressiveBehavior() )
	{
		if( m_bMinigunUseDirectFire )
		{
			// On target the whole time. Just send a large number that
			// will be clipped, since shooting duration is random and
			// we don't know how long the burst will actually be.
			return 100.0f;
		}

		return STRIDER_EP1_SHOOT_ON_TARGET_TIME;
	}

	return STRIDER_SHOOT_ON_TARGET_TIME;
}

//---------------------------------------------------------
// How long (seconds) a burst of minigun fire lasts.
//---------------------------------------------------------
float CNPC_Strider::GetMinigunShootDuration()
{
	if( IsUsingAggressiveBehavior() )
		return STRIDER_EP1_SHOOT_DURATION;

	return STRIDER_DEFAULT_SHOOT_DURATION; 
}

//---------------------------------------------------------
// How long (seconds) a strider must wait between bursts
//---------------------------------------------------------
float CNPC_Strider::GetMinigunShootDowntime()
{
	if( IsUsingAggressiveBehavior() )
		return STRIDER_EP1_SHOOT_DOWNTIME;

	return STRIDER_SHOOT_DOWNTIME;
}

//---------------------------------------------------------
//---------------------------------------------------------
float CNPC_Strider::GetMinigunShootVariation()
{
	if( IsUsingAggressiveBehavior() )
	{
		if( m_bMinigunUseDirectFire )
			return 0.0f;

		return STRIDER_EP1_SHOOT_VARIATION;
	}

	return STRIDER_SHOOT_VARIATION;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::CannonPosition()
{
	Vector position;

	// Currently just using the gun for the vertical component!
	GetAttachment( "biggun", position );
	position.x = GetAbsOrigin().x;
	position.y = GetAbsOrigin().y;

	return position;
}

//---------------------------------------------------------
//---------------------------------------------------------
CBaseEntity *CNPC_Strider::GetCannonTarget()
{
	CBaseEntity *pTarget = m_hCannonTarget;
	return pTarget;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::HasCannonTarget() const
{
	return ( m_hCannonTarget.Get() != NULL );
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::IsCannonTarget( CBaseEntity *pTarget ) const
{
	CBaseEntity *pCannonTarget = m_hCannonTarget;
	if ( pCannonTarget && pCannonTarget == pTarget )
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------
// Purpose: Aim Gun at a target
// Output : Returns true if you hit the target, false if not there yet
//---------------------------------------------------------
bool CNPC_Strider::AimCannonAt( CBaseEntity *pEntity, float flInterval )
{
	if ( !pEntity )
		return true;

	matrix3x4_t gunMatrix;
	GetAttachment( gm_CannonAttachment, gunMatrix );

	// transform the enemy into gun space
	m_vecHitPos = pEntity->GetAbsOrigin();
	Vector localEnemyPosition;
	VectorITransform( pEntity->GetAbsOrigin(), gunMatrix, localEnemyPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	Vector unitAngles = Vector( localEnemyAngles.x, localEnemyAngles.y, localEnemyAngles.z ); 
	float angleDiff = VectorNormalize(unitAngles);
	const float aimSpeed = 16;

	// Exponentially approach the target
	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = MAX(yawSpeed,5);
	pitchSpeed = MAX(pitchSpeed,5);

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( gm_YawControl, m_aimYaw );
	SetPoseParameter( gm_PitchControl, m_aimPitch );

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = GetPoseParameter( gm_PitchControl );
	m_aimYaw = GetPoseParameter( gm_YawControl );

	// UNDONE: Zero out any movement past the limit and go ahead and fire if the strider hit its 
	// target except for clamping.  Need to clamp targets to limits and compare?
	if ( angleDiff < 1 )
		return true;

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::FireCannon() 
{
	ASSERT( m_hCannonTarget != NULL );
	if ( !m_hCannonTarget )
	{
		DevMsg( "Strider firing cannon at NULL target\n" );
		
		// Turn the cannon off
		EntityMessageBegin( this, true );
			WRITE_BYTE( STRIDER_MSG_DEAD );
		MessageEnd();

		return;
	}

	if ( GetNextThink( "CANNON_HIT" ) > gpGlobals->curtime )
	{
		DevMsg( "Strider refiring cannon?\n" );
		return;
	}

	m_nextShootTime = gpGlobals->curtime + 5;
	trace_t tr;
	Vector vecShootPos;
	GetAttachment( gm_CannonAttachment, vecShootPos );

	Vector vecShootDir;
	vecShootDir = m_hCannonTarget->WorldSpaceCenter() - vecShootPos;
	float flDist = VectorNormalize( vecShootDir );

	AI_TraceLine( vecShootPos, vecShootPos + vecShootDir * flDist, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	m_blastHit = tr.endpos;
	m_blastHit += tr.plane.normal * 16;
	m_blastNormal = tr.plane.normal;

	// tell the client side effect to complete
	EntityMessageBegin( this, true );
		WRITE_BYTE( STRIDER_MSG_BIG_SHOT );
		WRITE_VEC3COORD( tr.endpos );
	MessageEnd();

	CPASAttenuationFilter filter2( this, "NPC_Strider.Shoot" );
	EmitSound( filter2, entindex(), "NPC_Strider.Shoot");
	SetContextThink( &CNPC_Strider::CannonHitThink, gpGlobals->curtime + 0.2f, "CANNON_HIT" );
}

void CNPC_Strider::CannonHitThink()
{
	CBaseEntity *pCannonTarget = m_hCannonTarget;
	if ( pCannonTarget )
	{
		bool fAlive = pCannonTarget->IsAlive();

		CreateConcussiveBlast( m_blastHit, m_blastNormal, this, 2.5 );

		// If the target was alive, check to make sure it is now dead. If not,
		// Kill it and warn the designer.
		if( fAlive && pCannonTarget->IsAlive() )
		{
			DevWarning("* * * * * * * * * * * * * * *\n");
			DevWarning("NASTYGRAM: STRIDER failed to kill its cannon target. Killing directly...\n");
			DevWarning("* * * * * * * * * * * * * * *\n");

			CTakeDamageInfo info;

			info.SetDamage( pCannonTarget->GetHealth() );
			info.SetDamageType( DMG_GENERIC );
			info.SetAttacker( this );
			info.SetInflictor( this );

			pCannonTarget->TakeDamage( info );
		}
		
		// Clear this guy now that we've shot him
		m_hCannonTarget = NULL;
	}

	// Allow the cannon back on.
	m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 1 );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	if ( !HasMemory( bits_MEMORY_PROVOKED ) )
	{
		// if the player threw this in the last 1 seconds
		CBasePlayer *pPlayer = pEvent->pEntities[!index]->HasPhysicsAttacker( 1 );
		if ( pPlayer )
		{
			GetEnemies()->ClearMemory( pPlayer );
			Remember( bits_MEMORY_PROVOKED );
			SetCondition( COND_LIGHT_DAMAGE );
		}
	}

	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	if ( pOther && UTIL_IsCombineBall( pOther ) )
	{
		Vector damagePos;
		pEvent->pInternalData->GetContactPoint( damagePos );
		CTakeDamageInfo dmgInfo( pOther, pOther, vec3_origin, damagePos, (m_iMaxHealth / 3) + 1, DMG_BLAST | DMG_PREVENT_PHYSICS_FORCE );

		// FIXME: is there a better way for physics objects to keep track of what root entity responsible for them moving?
		CBasePlayer *pPlayer = pOther->HasPhysicsAttacker( 1.0 );
		if (pPlayer)
		{
			dmgInfo.SetAttacker( pPlayer );
		}

		// UNDONE: Find one near damagePos?
		m_nForceBone = 0;
		PhysCallbackDamage( this, dmgInfo, *pEvent, index );
		return;
	}

	BaseClass::VPhysicsShadowCollision( index, pEvent );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	// Let normal hitbox code handle rays
	if ( mask & CONTENTS_HITBOX )
	{
		return BaseClass::TestCollision( ray, mask, trace );
	}

	if ( IntersectRayWithBox( ray, WorldAlignMins() + GetAbsOrigin(), WorldAlignMaxs() + GetAbsOrigin(), DIST_EPSILON, &trace ) )
	{
		trace.hitbox = 0;
		trace.hitgroup = HITGROUP_HEAD;
		return true;
	}
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CarriedByDropship()
{
	if( GetOwnerEntity() && FClassnameIs( GetOwnerEntity(), "npc_combinedropship" ) )
		return true;

	return false;
}

//---------------------------------------------------------
// Carried by a dropship
//---------------------------------------------------------
void CNPC_Strider::CarriedThink()
{
	SetNextThink( gpGlobals->curtime + 0.05 );
	StudioFrameAdvance();

	Vector vecGround = GetAbsOrigin();
	TranslateNavGoal( NULL, vecGround );

	if( !CarriedByDropship() )
	{
		SetSolid( SOLID_BBOX );
		SetThink ( &CAI_BaseNPC::CallNPCThink );
	}
}




//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::LeftFootHit( float eventtime )
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "left foot", footPosition, angles );

	if ( hl2_episodic.GetBool() )
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.FootstepEverywhere" );
		EmitSound( filter, 0, "NPC_Strider.FootstepEverywhere", &footPosition, eventtime );
	}
	else
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
		EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );
	}

	FootFX( footPosition );

	return footPosition;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::RightFootHit( float eventtime )
{
	Vector footPosition;

	GetAttachment( "right foot", footPosition );
	
	if ( hl2_episodic.GetBool() )
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.FootstepEverywhere" );
		EmitSound( filter, 0, "NPC_Strider.FootstepEverywhere", &footPosition, eventtime );
	}
	else
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
		EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );
	}

	FootFX( footPosition );

	return footPosition;
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::BackFootHit( float eventtime )
{
	Vector footPosition;

	GetAttachment( "back foot", footPosition );

	if ( hl2_episodic.GetBool() )
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.FootstepEverywhere" );
		EmitSound( filter, 0, "NPC_Strider.FootstepEverywhere", &footPosition, eventtime );
	}
	else
	{
		CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
		EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );
	}

	FootFX( footPosition );

	return footPosition;
}

//---------------------------------------------------------
//---------------------------------------------------------
static Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex )
{
	int attachment = Studio_FindAttachment( pStudioHdr, pAttachmentName );

	Vector localAttach;
	const mstudioattachment_t &pAttachment = pStudioHdr->pAttachment(attachment);
	int iBone = pStudioHdr->GetAttachmentBone( attachment );
	MatrixGetColumn( pAttachment.local, 3, localAttach );

	matrix3x4_t inputToOutputBone;
	Studio_CalcBoneToBoneTransform( pStudioHdr, iBone, outputBoneIndex, inputToOutputBone );
	
	Vector out;
	VectorTransform( localAttach, inputToOutputBone, out );

	return out;
}

//-------------------------------------

void CNPC_Strider::StompHit( int followerBoneIndex )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	physfollower_t *bone = m_BoneFollowerManager.GetBoneFollower( followerBoneIndex );
	if ( !bone )
		return;

	const char *pBoneNames[] = {"left skewer", "right skewer" };
	int nameIndex = followerBoneIndex == STRIDER_LEFT_LEG_FOLLOWER_INDEX ? 0 : 1;
	Vector localHit = GetAttachmentPositionInSpaceOfBone( pStudioHdr, pBoneNames[nameIndex], bone->boneIndex );
	IPhysicsObject *pLegPhys = bone->hFollower->VPhysicsGetObject();

	// now transform into the worldspace of the current position of the leg's physics
	matrix3x4_t legToWorld;
	pLegPhys->GetPositionMatrix( &legToWorld );
	Vector hitPosition;
	VectorTransform( localHit, legToWorld, hitPosition );

	//NDebugOverlay::Box( hitPosition, Vector(-16,-16,-16), Vector(16,16,16), 0, 255, 0, 255, 1.0 );
	CBaseEntity *pEnemy = GetEnemy();
	CAI_BaseNPC *pNPC = pEnemy ? pEnemy->MyNPCPointer() : NULL;
	bool bIsValidTarget = pNPC && pNPC->GetModelPtr();
	if ( HasSpawnFlags( SF_CAN_STOMP_PLAYER ) )
	{
		bIsValidTarget = bIsValidTarget || ( pEnemy && pEnemy->IsPlayer() );
	}

	if ( !bIsValidTarget )
		return;

	Vector delta;
	VectorSubtract( pEnemy->GetAbsOrigin(), hitPosition, delta );
	if ( delta.LengthSqr() > (STRIDER_STOMP_RANGE * STRIDER_STOMP_RANGE) )
		return;

	// DVS E3 HACK: Assume we stab our victim midway between their eyes and their center.
	Vector vecStabPos = ( pEnemy->WorldSpaceCenter() + pEnemy->EyePosition() ) * 0.5f;
	hitPosition = pEnemy->GetAbsOrigin();

	Vector footPosition;
	GetAttachment( "left foot", footPosition );

	CPASAttenuationFilter filter( this, "NPC_Strider.Skewer" );
	EmitSound( filter, 0, "NPC_Strider.Skewer", &hitPosition );

	CTakeDamageInfo damageInfo( this, this, 500, DMG_CRUSH );
	Vector forward;
	pEnemy->GetVectors( &forward, NULL, NULL );
	damageInfo.SetDamagePosition( hitPosition );
	damageInfo.SetDamageForce( -50 * 300 * forward );
	pEnemy->TakeDamage( damageInfo );

	if ( !pNPC || pNPC->IsAlive() )
		return;

	Vector vecBloodDelta = footPosition - vecStabPos;
	vecBloodDelta.z = 0; // effect looks better 
	VectorNormalize( vecBloodDelta );
	UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );
	UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 11, FX_BLOODSPRAY_DROPS );
	CBaseEntity *pRagdoll = CreateServerRagdollAttached( pNPC, vec3_origin, -1, COLLISION_GROUP_DEBRIS, pLegPhys, this, bone->boneIndex, vecStabPos, -1, localHit );
	if ( pRagdoll )
	{
		// the strider might drag this through the world
		pRagdoll->AddSolidFlags( FSOLID_NOT_SOLID );

		m_hRagdoll = pRagdoll;
		m_ragdollTime = gpGlobals->curtime + 10;
		UTIL_Remove( pNPC );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Strider::FootFX( const Vector &origin )
{
	trace_t tr;
	AI_TraceLine( origin + Vector(0, 0, 48), origin - Vector(0,0,100), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	float yaw = random->RandomInt(0,120);
	
	if ( UTIL_PointContents( tr.endpos + Vector( 0, 0, 1 ) ) & MASK_WATER )
	{
		float flWaterZ = UTIL_FindWaterSurface( tr.endpos, tr.endpos.z, tr.endpos.z + 100.0f );

		CEffectData	data;
		data.m_fFlags = 0;
		data.m_vOrigin = tr.endpos;
		data.m_vOrigin.z = flWaterZ;
		data.m_vNormal = Vector( 0, 0, 1 );
		data.m_flScale = random->RandomFloat( 10.0, 14.0 );

		DispatchEffect( "watersplash", data );
	}
	else
	{
		for ( int i = 0; i < 3; i++ )
		{
			Vector dir = UTIL_YawToVector( yaw + i*120 ) * 10;
			VectorNormalize( dir );
			dir.z = 0.25;
			VectorNormalize( dir );
			g_pEffects->Dust( tr.endpos, dir, 12, 50 );
		}
	}
	UTIL_ScreenShake( tr.endpos, 4.0, 1.0, 0.5, 1000, SHAKE_START, false );
	
	if ( npc_strider_shake_ropes_radius.GetInt() )
	{
		CRopeKeyframe::ShakeRopes( tr.endpos, npc_strider_shake_ropes_radius.GetFloat(), npc_strider_shake_ropes_magnitude.GetFloat() );
	}

	//
	// My feet are scary things! NOTE: We might want to make danger sounds as the feet move
	// through the air. Then soldiers could run from the feet, which would look cool.
	//
	CSoundEnt::InsertSound( SOUND_DANGER|SOUND_CONTEXT_EXCLUDE_COMBINE, tr.endpos, 512, 1.0f, this );
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Strider::CalculateStompHitPosition( CBaseEntity *pEnemy )
{
	Vector skewerPosition, footPosition;
	GetAttachment( "left skewer", skewerPosition );
	GetAttachment( "left foot", footPosition );
	Vector vecStabPos = ( pEnemy->WorldSpaceCenter() + pEnemy->EyePosition() ) * 0.5f;

	return vecStabPos - skewerPosition + footPosition;
}

//-----------------------------------------------------------------------------
// Strider Navigation
//-----------------------------------------------------------------------------

static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs )
{
	trace_t tr;
	// Find point on floor where enemy would stand at chasePosition
	Vector floor = *position;
	floor.z -= 1024;
	AI_TraceHull( *position, floor, mins, maxs, MASK_NPCSOLID_BRUSHONLY, ignore, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1 )
	{
		position->z = tr.endpos.z;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void AdjustStriderNodePosition( CAI_Network *pNetwork, CAI_Node *pNode )
{
	if ( pNode->GetHint() && pNode->GetHint()->HintType() == HINT_STRIDER_NODE )
	{
		CNPC_Strider *pStrider = (CNPC_Strider *)gEntList.FindEntityByClassname( NULL, "npc_strider" );
		bool bCreated = false;
		if ( !pStrider )
		{
			bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
			CBaseEntity::SetAllowPrecache( true );
			pStrider = (CNPC_Strider *)CreateEntityByName( "npc_strider" );
			pStrider->m_bDisableBoneFollowers = true; // don't create these since we're just going to destroy him
			DispatchSpawn( pStrider );
			CBaseEntity::SetAllowPrecache( allowPrecache );
			bCreated = true;
		}

		if ( pStrider )
		{
			pStrider->TranslateNavGoal( NULL, pNode->AccessOrigin() );
			if ( bCreated )
				UTIL_Remove( pStrider );
		}
	}
}


//---------------------------------------------------------
// Implement a navigator so that the strider can get closer to the ground when he's walking on a slope
// This gives him enough reach for his legs to IK to lower ground points
//---------------------------------------------------------
void LookaheadPath( const Vector &current, AI_Waypoint_t *pWaypoint, float dist, Vector &nextPos )
{
	if ( !pWaypoint )
		return;

	Vector dir = pWaypoint->GetPos() - current;
	float dist2D = dir.Length2D();
	if ( dist2D > 0.1 )
	{
		if ( dist <= dist2D )
		{
			nextPos = ((dist / dist2D) * dir) + current;
			return;
		}
	}
	nextPos = pWaypoint->GetPos();
	dist -= dist2D;
	dir = nextPos;
	
	LookaheadPath( dir, pWaypoint->GetNext(), dist, nextPos );
}

//---------------------------------------------------------
// Find the forward slope and lower the strider height when moving downward
//---------------------------------------------------------
void CNPC_Strider::CNavigator::MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal )
{
	BaseClass::MoveCalcBaseGoal( pMoveGoal );
	Vector dest = pMoveGoal->target;
	LookaheadPath( GetAbsOrigin(), pMoveGoal->pPath->GetCurWaypoint(), CNPC_Strider::gm_strideLength, dest );

	//NDebugOverlay::Box( dest, Vector(-16,-16,-16), Vector(16,16,16), 0, 255, 0, 255, 0.1 );
	Vector unitDir = pMoveGoal->dir * CNPC_Strider::gm_strideLength;
	unitDir.z = dest.z - GetAbsOrigin().z;
	VectorNormalize( unitDir );
	float heightAdj = unitDir.z * CNPC_Strider::gm_strideLength;
	pMoveGoal->flags |= ( AILMG_NO_STEER | AILMG_NO_AVOIDANCE_PATHS );
	if ( heightAdj < -1 )
	{
		heightAdj = clamp( heightAdj, -192, 0 );
		pMoveGoal->target.z += heightAdj;
		pMoveGoal->dir = unitDir * CNPC_Strider::gm_strideLength * 0.1;
		pMoveGoal->dir.z += heightAdj;
		VectorNormalize( pMoveGoal->dir );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CNavigator::MoveUpdateWaypoint( AIMoveResult_t *pResult )
{
	// Note that goal & waypoint tolerances are handled in progress blockage cases (e.g., OnObstructionPreSteer)

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float 		   waypointDist = (pCurWaypoint->GetPos() - GetAbsOrigin()).Length2D();
	bool		   bIsGoal		= CurWaypointIsGoal();
	// HACKHACK: adjust this tolerance
	// The strider is following paths most of the time, but he needs a bunch of freedom
	// to adjust himself to make his feet look good.
	float		   tolerance	= 10.0;

	if ( waypointDist <= tolerance )
	{
		if ( bIsGoal )
		{
			OnNavComplete();
			*pResult = AIMR_OK;
			
		}
		else
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
		return true;
	}
	
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CNavigator::DoFindPathToPos()
{
	DbgNavMsg(  GetOuter(), "Strider overriding DoFindPathToPos\n");
	if ( BaseClass::DoFindPathToPos() )
	{
		// Do special optimization on our first path segment
		DbgNavMsg(  GetOuter(), "Strider base pathfind worked\n");
		CAI_WaypointList waypoints( GetPath()->GetCurWaypoint() );
		AI_Waypoint_t *pFirstWaypoint = waypoints.GetFirst();
		AI_Waypoint_t *pLast = waypoints.GetLast();

		if ( pFirstWaypoint->IsReducible() && 
			 pFirstWaypoint->GetNext() && 
			 pFirstWaypoint->GetNext()->iNodeID != NO_NODE && 
			 pFirstWaypoint->GetNext()->NavType() == GetNavType() )
		{
			// Find nearest point on the line segment of our path
			Vector vOrigin = GetOuter()->GetAbsOrigin();
			Vector vClosest;
			CalcClosestPointOnLineSegment( vOrigin,
				pFirstWaypoint->GetPos(), 
				pFirstWaypoint->GetNext()->GetPos(), 
				vClosest );

			// Find both these positions as offset from the ground
			TranslateNavGoal( GetPath()->GetTarget(), vClosest );
			TranslateNavGoal( GetPath()->GetTarget(), vOrigin );

			// If we're seemingly beyond the waypoint and our hull is near enough to the path segment, advance to the next node
			const float EPS = 0.1f;
			bool bPastStartNode = ( ( pFirstWaypoint->GetPos() - vClosest ).Length() > EPS );
			bool bNearPathSegment = ( ( vOrigin - vClosest ).Length() < GetHullWidth() * 0.5f );
			if ( bPastStartNode && bNearPathSegment )
			{
				GetPath()->Advance();
			}
		}

		if ( pLast->iNodeID == NO_NODE )
		{
			AI_Waypoint_t *pLastNodeWaypoint = pLast->GetPrev();

			if ( pLastNodeWaypoint && pLastNodeWaypoint->iNodeID == NO_NODE )
			{
				// Pathfinder triangulated to goal
				Assert( pLastNodeWaypoint->GetPrev() );
				if ( pLastNodeWaypoint->GetPrev() )
				{
					pLastNodeWaypoint->GetPrev()->SetNext( pLast );
					delete pLastNodeWaypoint;
					pLastNodeWaypoint = pLast->GetPrev();
				}
			}

			Assert( pLastNodeWaypoint );
			if ( pLastNodeWaypoint )
			{
				Assert( pLastNodeWaypoint->iNodeID != NO_NODE );
				if ( pLastNodeWaypoint->iNodeID != NO_NODE )
				{
					CAI_Node *pLastNode = GetNetwork()->GetNode( pLastNodeWaypoint->iNodeID );
					float bestDistSq = ( pLast->vecLocation - pLastNodeWaypoint->vecLocation ).LengthSqr();
					Vector vNewEnd = vec3_invalid;
					int segmentDestClosest = NO_NODE;

					for (int link=0; link < pLastNode->NumLinks();link++) 
					{
						CAI_Link *pLink = pLastNode->GetLinkByIndex(link);
						if ( !( pLink->m_LinkInfo & bits_LINK_OFF ) && (pLink->m_iAcceptedMoveTypes[GetHullType()] & bits_CAP_MOVE_FLY) )
						{
							CAI_Node *pTestNode = GetNetwork()->GetNode( pLink->DestNodeID( pLastNodeWaypoint->iNodeID ) );
							if ( pTestNode->GetHint() && pTestNode->GetHint()->HintType() == HINT_STRIDER_NODE )
							{
								Vector vClosest;
								CalcClosestPointOnLineSegment( pLast->vecLocation, 
															   pLastNodeWaypoint->vecLocation, pTestNode->GetPosition(GetHullType()), 
															   vClosest );
								float distTestSq = ( pLast->vecLocation - vClosest ).LengthSqr();
								if ( distTestSq < bestDistSq )
								{
									bestDistSq = distTestSq;
									vNewEnd = vClosest;
									segmentDestClosest = pTestNode->GetId();
								}
							}
						}
					}
					
					if ( vNewEnd == vec3_invalid )
					{
						DbgNavMsg(  GetOuter(), "Strider resetting goal to last node waypoint\n");
						pLastNodeWaypoint->SetNext( NULL );
						pLastNodeWaypoint->ModifyFlags( bits_WP_TO_GOAL, true );
						delete pLast;
					}
					else if ( !pLastNodeWaypoint->GetPrev() || pLastNodeWaypoint->GetPrev()->iNodeID != segmentDestClosest )
					{
						DbgNavMsg(  GetOuter(), "Strider resetting goal to nearest point on graph\n");
						pLast->vecLocation = vNewEnd;
					}
					else
					{
						DbgNavMsg(  GetOuter(), "Strider resetting goal to nearest point on graph, on node\n");
						pLastNodeWaypoint->vecLocation = vNewEnd;
						pLastNodeWaypoint->SetNext( NULL );
						pLastNodeWaypoint->ModifyFlags( bits_WP_TO_GOAL, true );
						delete pLast;
					}
					GetPath()->ResetGoalPosition( GetPath()->GetGoalWaypoint()->vecLocation );
				}
			}
		}
		else
			DbgNavMsg(  GetOuter(), "Goal ended on node\n");
		return true;
	}
	else
		DbgNavMsg(  GetOuter(), "Strider base pathfind failed\n");

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CNavigator::ShouldOptimizeInitialPathSegment( AI_Waypoint_t *pFirstWaypoint )
{
	// We do our own special initial path optimization DoFindPathToPos()
	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Strider::CNavigator::GetStoppingPath( CAI_WaypointList *pClippedWaypoints )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns if a bone follower is a leg piece or not
// Input  : *pFollower - Bone follower we're testing
// Output : Returns true if the bone follower is a part of the strider's legs
//-----------------------------------------------------------------------------
bool CNPC_Strider::IsLegBoneFollower( CBoneFollower *pFollower )
{
	// Find the index of this bone follower, if we have it
	int nFollowerIndex = GetBoneFollowerIndex( pFollower );
	if ( nFollowerIndex	== -1 )
		return false;

	// See if we're a leg
	if ( nFollowerIndex >= STRIDER_LEFT_LEG_FOLLOWER_INDEX &&
		 nFollowerIndex <= STRIDER_BACK_UPPERLEG_FOLLOWER_INDEX )
		 return true;

	// We're something else
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a bone follower via a supplied index
// Input  : nIndex - index our bone follower manager will use to retrieve the follower
// Output : NULL if not found, otherwise the bone follower we were seeking
//-----------------------------------------------------------------------------
CBoneFollower *CNPC_Strider::GetBoneFollowerByIndex( int nIndex )
{
	physfollower_t *pFollower = m_BoneFollowerManager.GetBoneFollower( nIndex );
	if ( pFollower != NULL )
		return pFollower->hFollower;
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index of a bone follower in our system
// Input  : *pFollower - Follower we're looking for
// Output : -1 if not found, otherwise the index of the follower
//-----------------------------------------------------------------------------
int CNPC_Strider::GetBoneFollowerIndex( CBoneFollower *pFollower )
{
	return m_BoneFollowerManager.GetBoneFollowerIndex( pFollower );
}

//-----------------------------------------------------------------------------
// Purpose: The strider ignores certain entities this way
// Input  : *pEntity - Entity in question
// Output : Returns true if we should collide with the entity
//-----------------------------------------------------------------------------
bool CNPC_Strider::ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity )
{
	if ( pEntity->m_iClassname == m_iszStriderBusterName )
		return false;

	return BaseClass::ShouldProbeCollideAgainstEntity( pEntity );
}


//-----------------------------------------------------------------------------
// Purpose: Lets us keep track of attached Strider busters
// Input  : *pAttached - strider buster that is attached
//-----------------------------------------------------------------------------
#ifdef HL2_EPISODIC
void CNPC_Strider::StriderBusterAttached( CBaseEntity *pAttached )
{
	// Add another to the list
	m_hAttachedBusters.AddToTail( pAttached );
	m_PlayerFreePass.Revoke();

	variant_t target;
	target.SetString( AllocPooledString( "!player" ) );
	g_EventQueue.AddEvent( this, "UpdateEnemyMemory", target, 1.0, this, this );
}


void CNPC_Strider::StriderBusterDetached( CBaseEntity *pAttached )
{
	int elem = m_hAttachedBusters.Find(pAttached);
	if (elem >= 0)
	{
		m_hAttachedBusters.FastRemove(elem);
	}
}

#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
//
// Strider Minigun
//
//-----------------------------------------------------------------------------

BEGIN_DATADESC_NO_BASE( CStriderMinigun )

	DEFINE_FIELD( m_enable,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_minigunState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nextBulletTime,		FIELD_TIME ),
	DEFINE_FIELD( m_burstTime,			FIELD_TIME ),
	DEFINE_FIELD( m_nextTwitchTime,		FIELD_TIME ),
	DEFINE_FIELD( m_randomState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bWarnedAI,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_shootDuration,		FIELD_TIME ),
	DEFINE_FIELD( m_vecAnchor,			FIELD_VECTOR ),
	DEFINE_FIELD( m_bOverrideEnemy,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecLastTargetPos,	FIELD_VECTOR ),
	DEFINE_FIELD( m_iOnTargetShots,		FIELD_INTEGER ),

	// Silence, Classcheck!
	// DEFINE_FIELD( m_yaw, StriderMinigunAnimController_t ),
	// DEFINE_FIELD( m_pitch, StriderMinigunAnimController_t ),
	
	DEFINE_FIELD( m_yaw.current,		FIELD_FLOAT ),
	DEFINE_FIELD( m_yaw.target,			FIELD_FLOAT ),
	DEFINE_FIELD( m_yaw.rate,			FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.current,		FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.target,		FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.rate,			FIELD_FLOAT ),


END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::Init()
{
	m_enable = true;
	m_nextTwitchTime = gpGlobals->curtime;
	m_randomState = 0;
	m_yaw.current = m_yaw.target = 0;
	m_pitch.current = m_pitch.target = 0;
	m_yaw.rate = 360;
	m_pitch.rate = 180;

	SetState( MINIGUN_OFF );
	m_burstTime = gpGlobals->curtime;
	m_nextBulletTime = FLT_MAX;
	m_vecAnchor = vec3_invalid;
	m_shootDuration = STRIDER_DEFAULT_SHOOT_DURATION;
	m_bOverrideEnemy = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CStriderMinigun::ShouldFindTarget( IMinigunHost *pHost )
{
	ASSERT( pHost != NULL );

	if( !GetTarget() )
	{
		// No target. Find one.
		return true;
	}

	if( m_bOverrideEnemy )
	{
		return false;
	}

	if( pHost->GetEntity()->GetEnemy() )
	{
		return GetTarget() != pHost->GetEntity()->GetEnemy();
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
float CStriderMinigun::GetAimError()
{
	return fabs(m_yaw.target-m_yaw.current) + fabs(m_pitch.target-m_pitch.current);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::AimAtPoint( IStriderMinigunHost *pHost, const Vector &vecPoint, bool bSnap )
{
	matrix3x4_t gunMatrix;
	CAI_BaseNPC *pOwner = pHost->GetEntity();

	int mingunAttachment = pOwner->LookupAttachment( "minigunbase" );
	pOwner->GetAttachment( mingunAttachment, gunMatrix );

	Vector forward, pos;
	MatrixGetColumn( gunMatrix, 0, forward );
	MatrixGetColumn( gunMatrix, 3, pos );

	// transform the point into gun space
	Vector localPointPosition;
	VectorITransform( vecPoint, gunMatrix, localPointPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localPointAngles;
	VectorAngles( localPointPosition, localPointAngles );

	// convert to +/- 180 degrees
	float pdiff, ydiff;
	pdiff = UTIL_AngleDiff( localPointAngles.x, 0 );
	ydiff = UTIL_AngleDiff( localPointAngles.y, 0 );

	m_pitch.target += 0.5 * pdiff;
	m_yaw.target -= 0.5 * ydiff;

	m_pitch.target = MAX( MINIGUN_MIN_PITCH, m_pitch.target );
	m_pitch.target = MIN( MINIGUN_MAX_PITCH, m_pitch.target );
	m_yaw.target = MAX( MINIGUN_MIN_YAW, m_yaw.target );
	m_yaw.target = MIN( MINIGUN_MAX_YAW, m_yaw.target );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::AimAtTarget( IStriderMinigunHost *pHost, CBaseEntity *pTarget, bool bSnap )
{
	if ( pTarget && !(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
	{
		Vector vecTargetPos = pTarget->BodyTarget( pHost->GetEntity()->EyePosition() );
		AimAtPoint( pHost, vecTargetPos, bSnap );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::ShootAtTarget( IStriderMinigunHost *pHost, CBaseEntity *pTarget, float shootTime )
{
	if ( !pTarget && !(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
		return;

	variant_t emptyVariant;
	pTarget->AcceptInput( "InputTargeted", pHost->GetEntity(), pHost->GetEntity(), emptyVariant, 0 );
	Enable( NULL, true );
	if ( shootTime <= 0 )
	{
		shootTime = random->RandomFloat( 4, 8 );
	}
	SetTarget( pHost, pTarget, true );

	StartShooting( pHost, pTarget, shootTime );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::StartShooting( IStriderMinigunHost *pHost, CBaseEntity *pTarget, float duration )
{
	bool bHasSetAnchor = false;

	SetState( MINIGUN_SHOOTING );
	
	m_nextBulletTime = gpGlobals->curtime;
	m_burstTime = gpGlobals->curtime + duration;

	m_shootDuration = duration;

	// don't twitch while shooting
	m_nextTwitchTime = FLT_MAX;

	if( pTarget->IsPlayer() )
	{
		// Don't shoot a player in the back if they aren't looking. 
		// Give them a chance to see they're being fired at.
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(pTarget);

		if( !pPlayer->FInViewCone( pHost->GetEntity() ) )
		{
			// Player doesn't see me. Try to start shooting so that they can see
			// the bullet impacts.
			m_vecAnchor = pPlayer->EyePosition();

			Vector vecPlayerLook;
			Vector vecToPlayer;

			// Check in 2D.
			vecPlayerLook = pPlayer->EyeDirection3D();
			vecPlayerLook.z = 0.0;
			VectorNormalize( vecPlayerLook );

			vecToPlayer = pPlayer->EyePosition() - pHost->GetEntity()->EyePosition();
			vecToPlayer.z = 0.0;
			VectorNormalize( vecToPlayer );

			float flDot = DotProduct( vecToPlayer, vecPlayerLook );

			if( flDot < 0.95 )
			{
				// If the player is looking sufficiently to a side, start 30 feet out in that direction.
				m_vecAnchor += pPlayer->EyeDirection3D() * 320;
				bHasSetAnchor = true;
			}
			else
			{
				// Start over their head, cause firing the direction they are looking will drill them.
				// in the back!
				m_vecAnchor += Vector( 0, 0, random->RandomFloat( 160, 240 ) );

				// Move it to one side of the other randomly, just to get it off center.
				Vector right;
				pPlayer->GetVectors( NULL, &right, NULL );
				m_vecAnchor += right * random->RandomFloat( -100, 100 );
				bHasSetAnchor = true;
			}
		}
	}

	if( !bHasSetAnchor )
	{
		m_vecAnchor = pTarget->WorldSpaceCenter();

		Vector right;
		pTarget->GetVectors( NULL, &right, NULL );

		// Start 5 or 10 feet off target.
		Vector offset = right * random->RandomFloat( 60, 120 );
		
		// Flip a coin to decide left or right.
		if( random->RandomInt( 0, 1 ) == 0 )
		{
			offset *= -1;
		}

		m_vecAnchor += offset;

		// Start below them, too.
		m_vecAnchor.z -= random->RandomFloat( 80, 200 );
	}

	pHost->OnMinigunStartShooting( pTarget );
}

//---------------------------------------------------------
// Fixes up the math for stitching.
//---------------------------------------------------------
void CStriderMinigun::ExtendShooting( float timeExtend )
{
	m_burstTime = gpGlobals->curtime + timeExtend;
	m_shootDuration = timeExtend;
	m_bWarnedAI = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::SetShootDuration( float duration )
{
}

//---------------------------------------------------------
// Is the gun turned as far as it can go?
//---------------------------------------------------------
bool CStriderMinigun::IsPegged( int dir )
{
	bool up, down, left, right, any;

	up = down = left = right = any = false;

	if( m_yaw.current >= 89.0 )
		any = right = true;

	if( m_yaw.current <= -89.0 )
		any = left = true;

	if( m_pitch.current >= 44.0 )
		any = down = true;

	if( m_pitch.current <= -44.0 )
		any = up = true;

	switch( dir )
	{
	case MINIGUN_PEGGED_UP:
		return up;

	case MINIGUN_PEGGED_DOWN:
		return down;

	case MINIGUN_PEGGED_LEFT:
		return left;

	case MINIGUN_PEGGED_RIGHT:
		return right;

	default:
		return (any && !up);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::StopShootingForSeconds( IStriderMinigunHost *pHost, CBaseEntity *pTarget, float duration )
{
	if ( IsShooting() )
	{
		SetState( MINIGUN_OFF );
	}

	m_burstTime = gpGlobals->curtime + duration;
	m_nextBulletTime = FLT_MAX;

	ClearOnTarget();
	m_nextTwitchTime = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	pHost->OnMinigunStopShooting( pTarget );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::SetState( int newState )
{
	m_minigunState = newState;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::SetTarget( IStriderMinigunHost *pHost, CBaseEntity *pTarget, bool bOverrideEnemy )
{
	m_hTarget = pTarget;

	if( pTarget )
	{
		// New target, we haven't scared this guy yet!
		m_bWarnedAI = false;

		if( m_vecAnchor == vec3_invalid )
		{
			Vector right;
			pHost->GetEntity()->GetVectors( NULL, &right, NULL );

			m_vecAnchor = pTarget->GetAbsOrigin() - Vector( 0, 0, 256 );
			m_vecAnchor += right * random->RandomFloat( -60.0f, 60.0f );
		}
	}

	ClearOnTarget();

	m_bOverrideEnemy = bOverrideEnemy;
}

//---------------------------------------------------------
// The strider minigun can track and fire at targets in a fairly
// large arc. It looks unnatural for a Strider to acquire a target
// off to one side and begin firing at it, so we don't let the
// minigun BEGIN shooting at a target unless the target is fairly
// well in front of the Strider. Once firing, the gun is allowed
// to track the target anywhere for the duration of that burst of
// minigun fire. This is tuned by eye. (sjb)
//---------------------------------------------------------
bool CStriderMinigun::CanStartShooting( IStriderMinigunHost *pHost, CBaseEntity *pTargetEnt )
{
	if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
		return false;

	if( !pTargetEnt )
		return false;
	
	if( gpGlobals->curtime < m_burstTime )
		return false;

	CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(pHost->GetEntity() );
	if ( pTargetEnt->IsPlayer() && pStrider->HasPass() )
		return false;

	if( !m_bOverrideEnemy )
	{
		if( pTargetEnt != pHost->GetEntity()->GetEnemy() )
		{
			return false;
		}

		// If the strider can't see the enemy, this may be because the enemy is
		// hiding behind something breakable. If the weapon has LOS, fire away.
		if( !pHost->GetEntity()->HasCondition( COND_SEE_ENEMY )  )
		{
			Assert( pStrider != NULL );

			if( !pStrider->WeaponLOSCondition( pStrider->GetAdjustedOrigin(), pTargetEnt->WorldSpaceCenter(), false ) )
			{
				if( pStrider->IsUsingAggressiveBehavior() && pTargetEnt->IsPlayer() && !pStrider->HasPass() )
				{
					// I can shoot the player's cover until he hides long enough to earn a free pass.
					float flTimeSinceLastSeen = gpGlobals->curtime - pStrider->GetEnemies()->LastTimeSeen(pTargetEnt);

					if( flTimeSinceLastSeen <= 2.0f )
						return true;
				}

				return false;
			}
		}
	}
	
	Vector los = ( pTargetEnt->WorldSpaceCenter() - pHost->GetEntity()->EyePosition() );

	// Following code stolen from FVisible. This check done in 2d intentionally.
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = pHost->GetEntity()->EyeDirection2D( );
	float flDot = DotProduct( los, facingDir );

	// Too far to a side.
	if ( flDot <= .707 )
		return false;

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::Enable( IMinigunHost *pHost, bool enable )
{
	m_enable = enable;
	if ( !m_enable )
	{
		m_yaw.current = m_yaw.target = 0;
		m_pitch.current = m_pitch.target = 0;
		if ( pHost )
		{
			// KENB pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CStriderMinigun::Think( IStriderMinigunHost *pHost, float dt )
{
	if ( !m_enable )
		return;

	if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
		return;

	if( ShouldFindTarget( pHost ) )
	{
		CBaseEntity *pOldTarget = GetTarget();

		// Take my host's enemy.
		SetTarget( pHost, pHost->GetEntity()->GetEnemy() );
		
		if( IsShooting() )
		{
			// Changing targets hot! 
			if( pOldTarget )
			{
				m_vecAnchor = pOldTarget->WorldSpaceCenter();
			}

			ExtendShooting( STRIDER_SUBSEQUENT_TARGET_DURATION + random->RandomFloat( 0, 0.5 ) );
		}
		
		pHost->NewTarget();
	}

	if ( !GetTarget() && m_nextTwitchTime <= gpGlobals->curtime )
	{
		// invert one and randomize the other.
		// This has the effect of making the gun cross the field of
		// view more often - like he's looking around
		m_randomState = !m_randomState;
		if ( m_randomState )
		{
			m_yaw.Random( MINIGUN_MIN_YAW, MINIGUN_MAX_YAW, 360, 720 );
			m_pitch.target = -m_pitch.target;
		}
		else
		{
			m_pitch.Random( MINIGUN_MIN_PITCH, MINIGUN_MAX_PITCH, 270, 360 );
			m_yaw.target = -m_yaw.target;
		}
		m_nextTwitchTime = gpGlobals->curtime + random->RandomFloat( 0.3, 2 );
	}

	CBaseEntity *pTargetEnt = m_hTarget.Get();

	if ( pTargetEnt )
	{
		pHost->GetEntity()->InvalidateBoneCache();
		AimAtTarget( pHost, pTargetEnt );
	}

	// Update the minigun's pose parameters using approaching. 
	m_yaw.Update( dt );
	m_pitch.Update( dt );

	pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );

	// Start or stop shooting.
	if( IsShooting() )
	{
		if( gpGlobals->curtime > m_burstTime || !pTargetEnt )
		{
			// Time to stop firing.
			if( m_bOverrideEnemy )
			{
				// Get rid of this target.
				SetTarget( pHost, NULL );
			}

			StopShootingForSeconds( pHost, pTargetEnt, pHost->GetMinigunShootDowntime() );
		}
	}
	else
	{
		if( CanStartShooting( pHost, pTargetEnt ) )
		{
			StartShooting( pHost, pTargetEnt, pHost->GetMinigunShootDuration() + random->RandomFloat( 0, pHost->GetMinigunShootVariation() ) );
		}
	}

	// Fire the next bullet!
	if ( m_nextBulletTime <= gpGlobals->curtime && !IsPegged() )
	{
		if( pTargetEnt && pTargetEnt == pHost->GetEntity()->GetEnemy() )
		{
			// Shooting at the Strider's enemy. Strafe to target!
			float flRemainingShootTime = m_burstTime - gpGlobals->curtime;

			// Skim a little time off of the factor, leave a moment of on-target
			// time. This guarantees that the minigun will strike the target a few times.
			float flFactor = (flRemainingShootTime - pHost->GetMinigunOnTargetTime() ) / m_shootDuration;

			flFactor = MAX( 0.0f, flFactor );

			Vector vecTarget = pTargetEnt->BodyTarget( assert_cast<CNPC_Strider *>(pHost->GetEntity())->GetAdjustedOrigin());

			Vector vecLine = m_vecAnchor - vecTarget;
			
			float flDist = VectorNormalize( vecLine );

			vecTarget += vecLine * flDist * flFactor;

			if( flFactor == 0.0 )
			{
				m_vecAnchor = vecTarget;
				RecordShotOnTarget();
			}

			if ( GetTarget() )
			{
				pHost->ShootMinigun( &vecTarget, GetAimError(), vec3_origin );

				if( flFactor <= 0.5 && !m_bWarnedAI )
				{
					m_bWarnedAI = true;

					CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_REACT_TO_SOURCE, pTargetEnt->EarPosition() + Vector( 0, 0, 1 ), 120, MAX( 1.0, flRemainingShootTime ), pHost->GetEntity() );
				}
			}
		}
		else
		{
			const Vector *pTargetPoint = pTargetEnt ? &pTargetEnt->GetAbsOrigin() : NULL;
			pHost->ShootMinigun( pTargetPoint, GetAimError() );
		}

		m_nextBulletTime = gpGlobals->curtime + (1.0f / pHost->GetMinigunRateOfFire() );
	}
}

LINK_ENTITY_TO_CLASS( sparktrail, CSparkTrail );

void CSparkTrail::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "DoSpark" );
}

void CSparkTrail::Spawn()
{
	Precache();

	EmitSound( "DoSpark" );

	m_iHealth = 20 + random->RandomInt( 0, 5 );
	UTIL_SetOrigin( this, GetOwnerEntity()->EyePosition() );

	Vector vecVelocity;

	vecVelocity.x = random->RandomFloat( 100, 400 );
	vecVelocity.y = random->RandomFloat( 100, 400 );
	vecVelocity.z = random->RandomFloat( 0, 100 );
	
	if( random->RandomInt( 0, 1 ) == 0 )
		vecVelocity.x *= -1;

	if( random->RandomInt( 0, 1 ) == 0 )
		vecVelocity.y *= -1;

	UTIL_SetSize( this, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_NONE );

	if( random->RandomInt( 0, 2 ) == 0 )
	{
		vecVelocity *= 2.0;
		m_iHealth /= 2;
		SetMoveType( MOVETYPE_FLY );
	}

	SetAbsVelocity( vecVelocity );

	SetThink( &CSparkTrail::SparkThink );
	SetNextThink( gpGlobals->curtime );
}

void CSparkTrail::SparkThink()
{
	SetNextThink( gpGlobals->curtime + 0.05 );

	g_pEffects->Sparks( GetAbsOrigin() );

	if( m_iHealth-- < 1 )
	{
		UTIL_Remove( this );
	}
}

BEGIN_DATADESC( CSparkTrail )
	DEFINE_THINKFUNC( SparkThink ),
END_DATADESC()


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_strider, CNPC_Strider )

	DECLARE_TASK( TASK_STRIDER_AIM )
	DECLARE_TASK( TASK_STRIDER_DODGE )
	DECLARE_TASK( TASK_STRIDER_STOMP )
	DECLARE_TASK( TASK_STRIDER_BREAKDOWN )
	DECLARE_TASK( TASK_STRIDER_START_MOVING )
	DECLARE_TASK( TASK_STRIDER_REFRESH_HUNT_PATH )
	DECLARE_TASK( TASK_STRIDER_GET_PATH_TO_CANNON_TARGET )
	DECLARE_TASK( TASK_STRIDER_FACE_CANNON_TARGET )
	DECLARE_TASK( TASK_STRIDER_SET_HEIGHT )
	DECLARE_TASK( TASK_STRIDER_GET_PATH_TO_CANNON_LOS )
	DECLARE_TASK( TASK_STRIDER_FIRE_CANNON )
	DECLARE_TASK( TASK_STRIDER_SET_CANNON_HEIGHT )
	DECLARE_TASK( TASK_STRIDER_FALL_TO_GROUND )

	DECLARE_ACTIVITY( ACT_STRIDER_LOOKL )
	DECLARE_ACTIVITY( ACT_STRIDER_LOOKR )
	DECLARE_ACTIVITY( ACT_STRIDER_DEPLOYRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_AIMRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_FINISHRA1 )
	DECLARE_ACTIVITY( ACT_STRIDER_DODGER )
	DECLARE_ACTIVITY( ACT_STRIDER_DODGEL )
	DECLARE_ACTIVITY( ACT_STRIDER_STOMPL )
	DECLARE_ACTIVITY( ACT_STRIDER_STOMPR )
	DECLARE_ACTIVITY( ACT_STRIDER_FLICKL )
	DECLARE_ACTIVITY( ACT_STRIDER_FLICKR )
	DECLARE_ACTIVITY( ACT_STRIDER_SLEEP )
	DECLARE_ACTIVITY( ACT_STRIDER_CARRIED )
	DECLARE_ACTIVITY( ACT_STRIDER_DEPLOY )
	DECLARE_ACTIVITY( ACT_STRIDER_GESTURE_DEATH )

	DECLARE_CONDITION( COND_STRIDER_DO_FLICK )
	DECLARE_CONDITION( COND_TRACK_PATH_GO )
	DECLARE_CONDITION( COND_STRIDER_SHOULD_CROUCH )
	DECLARE_CONDITION( COND_STRIDER_SHOULD_STAND )
	DECLARE_CONDITION( COND_STRIDER_MINIGUN_SHOOTING )
	DECLARE_CONDITION( COND_STRIDER_MINIGUN_NOT_SHOOTING )
	DECLARE_CONDITION( COND_STRIDER_HAS_CANNON_TARGET )
	DECLARE_CONDITION( COND_STRIDER_ENEMY_UPDATED )
	DECLARE_CONDITION( COND_STRIDER_HAS_LOS_Z )

	DECLARE_INTERACTION( g_interactionPlayerLaunchedRPG )
	
	//=========================================================
	// Hunt (Basic logic for strider thinking)
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_HUNT,

		"	Tasks"
		"		TASK_STRIDER_REFRESH_HUNT_PATH 0"
		"		TASK_STRIDER_START_MOVING	0"
		"		TASK_WAIT					4"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_STRIDER_SHOULD_CROUCH"
		"		COND_STRIDER_HAS_CANNON_TARGET"
		"		COND_STRIDER_ENEMY_UPDATED"
	)

	//=========================================================
	// Attack (Deploy/shoot/finish)
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY			0"
		"		TASK_RANGE_ATTACK1		0"
		"		TASK_WAIT				5" // let the immolator work its magic
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Attack (Deploy/shoot/finish)
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_RANGE_ATTACK2,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_STRIDER_FACE_CANNON_TARGET		0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							1"
		"		TASK_STRIDER_AIM					1.25"
		"		TASK_STRIDER_FIRE_CANNON			0"
		"		TASK_WAIT							1"
		"	"
		"	Interrupts"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_CROUCH,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_CROUCH"
		"	"
		"	Interrupts"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STAND"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_STRIDER_SET_HEIGHT	500"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Dodge Incoming missile
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_DODGE,

		"	Tasks"
		"		TASK_STRIDER_DODGE		0"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Break down and die
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_DIE,

		"	Tasks"
		"		TASK_STRIDER_BREAKDOWN		0"
		"	"
		"	Interrupts"
	)
	
	//=========================================================
	// Stomp on an enemy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_STOMPL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_STRIDER_STOMP		0"
		"	"
		"	Interrupts"
	);		

	// Stomp on an enemy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_STOMPR,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_STRIDER_STOMP		1"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_FLICKL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FLICKL"
		"	"
		"	Interrupts"
	)
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_FLICKR,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_STRIDER_FLICKR"
		"	"
		"	Interrupts"
	)
	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_ATTACK_CANNON_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_STRIDER_GET_PATH_TO_CANNON_TARGET		0"
		"		TASK_WALK_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"	"
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_STRIDER_SHOULD_CROUCH"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_CHASE_ENEMY,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_LOST_ENEMY"
		"		COND_STRIDER_HAS_CANNON_TARGET"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_COMBAT_FACE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_FACE_ENEMY	1"
		""
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_STRIDER_HAS_CANNON_TARGET"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_AGGRESSIVE_COMBAT_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					1"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SEE_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_IDLE_INTERRUPT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_ESTABLISH_LINE_OF_FIRE_CANNON,

		"	Tasks "
		"		TASK_STRIDER_GET_PATH_TO_CANNON_LOS		0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_STRIDER_FACE_CANNON_TARGET			0"
		""
		"	Interrupts "
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_STRIDER_SHOULD_CROUCH"
		"		COND_STRIDER_SHOULD_STAND"
	)

	DEFINE_SCHEDULE
	(
		SCHED_STRIDER_FALL_TO_GROUND,

		"	Tasks "
		"		TASK_STRIDER_FALL_TO_GROUND		0"
		""
		"	Interrupts "
	)


AI_END_CUSTOM_NPC()

//=============================================================================
