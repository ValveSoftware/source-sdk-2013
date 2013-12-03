//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_squadslot.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_interactions.h"
#include "ndebugoverlay.h"
#include "explode.h"
#include "bitstring.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "decals.h"
#include "antlion_dust.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "beam_shared.h"
#include "iservervehicle.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vehicle_base.h"
#include "eventqueue.h"
#include "te_effect_dispatch.h"
#include "npc_rollermine.h"
#include "func_break.h"
#include "soundenvelope.h"
#include "mapentities.h"
#include "RagdollBoogie.h"
#include "physics_collisionevent.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ROLLERMINE_MAX_TORQUE_FACTOR	5
extern short g_sModelIndexWExplosion;

ConVar	sk_rollermine_shock( "sk_rollermine_shock","0");
ConVar	sk_rollermine_stun_delay("sk_rollermine_stun_delay", "1");
ConVar	sk_rollermine_vehicle_intercept( "sk_rollermine_vehicle_intercept","1");

enum
{
	ROLLER_SKIN_REGULAR = 0,
	ROLLER_SKIN_FRIENDLY,
	ROLLER_SKIN_DETONATE,
};
//-----------------------------------------------------------------------------
// CRollerController implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CRollerController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

	AngularImpulse	m_vecAngular;
	Vector			m_vecLinear;

	void Off( void ) { m_fIsStopped = true; }
	void On( void ) { m_fIsStopped = false; }

	bool IsOn( void ) { return !m_fIsStopped; }

private:
	bool	m_fIsStopped;
};

BEGIN_SIMPLE_DATADESC( CRollerController )

	DEFINE_FIELD( m_vecAngular, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecLinear, FIELD_VECTOR ),
	DEFINE_FIELD( m_fIsStopped, FIELD_BOOLEAN ),

END_DATADESC()


//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CRollerController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if( m_fIsStopped )
	{
		return SIM_NOTHING;
	}

	linear = m_vecLinear;
	angular = m_vecAngular;
	
	return IMotionEvent::SIM_LOCAL_ACCELERATION;
}
//-----------------------------------------------------------------------------


#define ROLLERMINE_IDLE_SEE_DIST					2048
#define ROLLERMINE_NORMAL_SEE_DIST					2048
#define ROLLERMINE_WAKEUP_DIST						256
#define ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE		300		// See every other than vehicles upto this distance (i.e. old idle see dist)
#define ROLLERMINE_SEE_VEHICLESONLY_BEYOND_NORMAL	800		// See every other than vehicles upto this distance (i.e. old normal see dist)

#define ROLLERMINE_RETURN_TO_PLAYER_DIST			(200*200)

#define ROLLERMINE_MIN_ATTACK_DIST	1
#define ROLLERMINE_MAX_ATTACK_DIST	4096

#define	ROLLERMINE_OPEN_THRESHOLD	256

#define ROLLERMINE_VEHICLE_OPEN_THRESHOLD	400
#define ROLLERMINE_VEHICLE_HOP_THRESHOLD	300

#define ROLLERMINE_HOP_DELAY				2			// Don't allow hops faster than this

//#define ROLLERMINE_REQUIRED_TO_EXPLODE_VEHICLE		4

#define ROLLERMINE_FEAR_DISTANCE			(300*300)

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERMINE_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_ROLLERMINE_CHASE_ENEMY,
	SCHED_ROLLERMINE_BURIED_WAIT,
	SCHED_ROLLERMINE_BURIED_UNBURROW,
	SCHED_ROLLERMINE_FLEE,
	SCHED_ROLLERMINE_ALERT_STAND,
	SCHED_ROLLERMINE_NUDGE_TOWARDS_NODES,
	SCHED_ROLLERMINE_PATH_TO_PLAYER,
	SCHED_ROLLERMINE_ROLL_TO_PLAYER,
	SCHED_ROLLERMINE_POWERDOWN,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERMINE_CHARGE_ENEMY = LAST_SHARED_TASK,
	TASK_ROLLERMINE_BURIED_WAIT,
	TASK_ROLLERMINE_UNBURROW,
	TASK_ROLLERMINE_GET_PATH_TO_FLEE,
	TASK_ROLLERMINE_NUDGE_TOWARDS_NODES,
	TASK_ROLLERMINE_RETURN_TO_PLAYER,
	TASK_ROLLERMINE_POWERDOWN,
};


// This are little 'sound event' flags. Set the flag after you play the
// sound, and the sound will not be allowed to play until the flag is then cleared.
#define ROLLERMINE_SE_CLEAR		0x00000000
#define ROLLERMINE_SE_CHARGE	0x00000001
#define ROLLERMINE_SE_TAUNT		0x00000002
#define ROLLERMINE_SE_SHARPEN	0x00000004
#define ROLLERMINE_SE_TOSSED	0x00000008

enum rollingsoundstate_t { ROLL_SOUND_NOT_READY = 0, ROLL_SOUND_OFF, ROLL_SOUND_CLOSED, ROLL_SOUND_OPEN };

//=========================================================
//=========================================================
class CNPC_RollerMine : public CNPCBaseInteractive<CAI_BaseNPC>, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CNPC_RollerMine, CNPCBaseInteractive<CAI_BaseNPC> );
	DECLARE_SERVERCLASS();

public:

	CNPC_RollerMine( void ) { m_bTurnedOn = true; m_bUniformSight = false; }
	~CNPC_RollerMine( void );

	void	Spawn( void );
	bool	CreateVPhysics();
	void	RunAI();
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	SpikeTouch( CBaseEntity *pOther );
	void	ShockTouch( CBaseEntity *pOther );
	void	CloseTouch( CBaseEntity *pOther );
	void	EmbedTouch( CBaseEntity *pOther );
	float	GetAttackDamageScale( CBaseEntity *pVictim );
	void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void	Precache( void );
	void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void	OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );
	void	StopLoopingSounds( void );
	void	PrescheduleThink();
	bool	ShouldSavePhysics()	{ return true; }
	void	OnRestore();
	void	Bury( trace_t *tr );
	bool	QuerySeeEntity(CBaseEntity *pSightEnt, bool bOnlyHateOrFearIfNPC = false );

	int		RangeAttack1Conditions ( float flDot, float flDist );
	int		SelectSchedule( void );
	int		TranslateSchedule( int scheduleType );
	int		GetHackedIdleSchedule( void );

	bool	OverrideMove( float flInterval ) { return true; }
	bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool	IsPlayerVehicle( CBaseEntity *pEntity );
	bool	IsShocking() { return gpGlobals->curtime < m_flShockTime ? true : false; }
	void	UpdateRollingSound();
	void	UpdatePingSound();
	void	StopRollingSound();
	void	StopPingSound();
	float	RollingSpeed();
	float	GetStunDelay();
	void	EmbedOnGroundImpact();
	void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	void	DrawDebugGeometryOverlays()
	{
		if (m_debugOverlays & OVERLAY_BBOX_BIT) 
		{
			float dist = GetSenses()->GetDistLook();
			Vector range(dist, dist, 64);
			NDebugOverlay::Box( GetAbsOrigin(), -range, range, 255, 0, 0, 0, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
	// UNDONE: Put this in the qc file!
	Vector EyePosition() 
	{
		// This takes advantage of the fact that the system knows
		// that the abs origin is at the center of the rollermine
		// and that the OBB is actually world-aligned despite the
		// fact that SOLID_VPHYSICS is being used
		Vector eye = CollisionProp()->GetCollisionOrigin();
		eye.z += CollisionProp()->OBBMaxs().z;
		return eye;
	}

	int		OnTakeDamage( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	Class_T	Classify() 
	{ 
		if( !m_bTurnedOn )
			return CLASS_NONE;

		//About to blow up after being hacked so do damage to the player.
		if ( m_bHackedByAlyx && ( m_flPowerDownDetonateTime > 0.0f && m_flPowerDownDetonateTime <= gpGlobals->curtime ) )
			return CLASS_COMBINE;

		return ( m_bHeld || m_bHackedByAlyx ) ? CLASS_HACKED_ROLLERMINE : CLASS_COMBINE; 
	}

	virtual bool ShouldGoToIdleState() 
	{ 
		return gpGlobals->curtime > m_flGoIdleTime ? true : false;
	}

	virtual	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	// Vehicle interception
	bool	EnemyInVehicle( void );
	float	VehicleHeading( CBaseEntity *pVehicle );

	NPC_STATE SelectIdealState();

	// Vehicle sticking
	void		StickToVehicle( CBaseEntity *pOther );
	void		AnnounceArrivalToOthers( CBaseEntity *pOther );
	void		UnstickFromVehicle( void );
	CBaseEntity *GetVehicleStuckTo( void );
	int			CountRollersOnMyVehicle( CUtlVector<CNPC_RollerMine*> *pRollerList );
	void		InputConstraintBroken( inputdata_t &inputdata );
	void		InputRespondToChirp( inputdata_t &inputdata );
	void		InputRespondToExplodeChirp( inputdata_t &inputdata );
	void		InputJoltVehicle( inputdata_t &inputdata );
	void		InputTurnOn( inputdata_t &inputdata );
	void		InputTurnOff( inputdata_t &inputdata );
	void		InputPowerdown( inputdata_t &inputdata );

	void		PreventUnstickUntil( float flTime ) { m_flPreventUnstickUntil = flTime; }

	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void		SetRollerSkin( void );

	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnPhysGunPickup;

protected:
	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

	bool	BecomePhysical();
	void	WakeNeighbors();
	bool	WakeupMine( CAI_BaseNPC *pNPC );

	void	Open( void );
	void	Close( void );
	void	Explode( void );
	void	PreDetonate( void );
	void	Hop( float height );

	void	ShockTarget( CBaseEntity *pOther );

	bool	IsActive() { return m_flActiveTime > gpGlobals->curtime ? false : true; }

	// INPCInteractive Functions
	virtual bool	CanInteractWith( CAI_BaseNPC *pUser ) { return true; }
	virtual	bool	HasBeenInteractedWith()	{ return m_bHackedByAlyx; }
	virtual void	NotifyInteraction( CAI_BaseNPC *pUser );

	CSoundPatch					*m_pRollSound;
	CSoundPatch					*m_pPingSound;

	CRollerController			m_RollerController;
	IPhysicsMotionController	*m_pMotionController;

	float	m_flSeeVehiclesOnlyBeyond;
	float	m_flChargeTime;
	float	m_flGoIdleTime;
	float	m_flShockTime;
	float	m_flForwardSpeed;
	int		m_iSoundEventFlags;
	rollingsoundstate_t m_rollingSoundState;

	CNetworkVar( bool, m_bIsOpen );
	CNetworkVar( float, m_flActiveTime );	//If later than the current time, this will force the mine to be active

	bool	m_bHeld;		//Whether or not the player is holding the mine
	EHANDLE m_hVehicleStuckTo;
	float	m_flPreventUnstickUntil;
	float	m_flNextHop;
	bool	m_bStartBuried;
	bool	m_bBuried;
	bool	m_bIsPrimed;
	bool	m_wakeUp;
	bool	m_bEmbedOnGroundImpact;
	CNetworkVar( bool,	m_bHackedByAlyx );

	// Constraint used to stick us to a vehicle
	IPhysicsConstraint *m_pConstraint;

	bool	m_bTurnedOn;
	bool	m_bUniformSight;

	CNetworkVar( bool,	m_bPowerDown );
	float	m_flPowerDownTime;
	float	m_flPowerDownDetonateTime;

	static string_t gm_iszDropshipClassname;
};

string_t CNPC_RollerMine::gm_iszDropshipClassname;

LINK_ENTITY_TO_CLASS( npc_rollermine, CNPC_RollerMine );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerMine )

	DEFINE_SOUNDPATCH( m_pRollSound ),
	DEFINE_SOUNDPATCH( m_pPingSound ),
	DEFINE_EMBEDDED( m_RollerController ),
	DEFINE_PHYSPTR( m_pMotionController ),

	DEFINE_FIELD( m_flSeeVehiclesOnlyBeyond, FIELD_FLOAT ),
	DEFINE_FIELD( m_flActiveTime, FIELD_TIME ),
	DEFINE_FIELD( m_flChargeTime, FIELD_TIME ),
	DEFINE_FIELD( m_flGoIdleTime, FIELD_TIME ),
	DEFINE_FIELD( m_flShockTime, FIELD_TIME ),
	DEFINE_FIELD( m_flForwardSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_bIsOpen, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHeld, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hVehicleStuckTo, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flPreventUnstickUntil, FIELD_TIME ),
	DEFINE_FIELD( m_flNextHop, FIELD_FLOAT ),
	DEFINE_FIELD( m_bIsPrimed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iSoundEventFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_rollingSoundState, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bStartBuried, FIELD_BOOLEAN, "StartBuried" ),
	DEFINE_FIELD( m_bBuried, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_wakeUp, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEmbedOnGroundImpact, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHackedByAlyx, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bPowerDown,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPowerDownTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flPowerDownDetonateTime,	FIELD_TIME ),

	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_FIELD( m_bTurnedOn, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bUniformSight, FIELD_BOOLEAN, "uniformsightdist" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ConstraintBroken", InputConstraintBroken ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RespondToChirp", InputRespondToChirp ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RespondToExplodeChirp", InputRespondToExplodeChirp ),
	DEFINE_INPUTFUNC( FIELD_VOID, "JoltVehicle", InputJoltVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PowerDown", InputPowerdown ),

	// Function Pointers
	DEFINE_ENTITYFUNC( SpikeTouch ),
	DEFINE_ENTITYFUNC( ShockTouch ),
	DEFINE_ENTITYFUNC( CloseTouch ),
	DEFINE_ENTITYFUNC( EmbedTouch ),
	DEFINE_THINKFUNC( Explode ),
	DEFINE_THINKFUNC( PreDetonate ),

	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),

	DEFINE_BASENPCINTERACTABLE_DATADESC(),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_RollerMine, DT_RollerMine )
	SendPropInt(SENDINFO(m_bIsOpen), 1, SPROP_UNSIGNED ),
	SendPropFloat(SENDINFO(m_flActiveTime), 0, SPROP_NOSCALE ),
	SendPropInt(SENDINFO(m_bHackedByAlyx), 1, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_bPowerDown), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

bool NPC_Rollermine_IsRollermine( CBaseEntity *pEntity )
{
	CNPC_RollerMine *pRoller = dynamic_cast<CNPC_RollerMine *>(pEntity);
	return pRoller ? true : false;
}

CBaseEntity *NPC_Rollermine_DropFromPoint( const Vector &originStart, CBaseEntity *pOwner, const char *pszTemplate )
{
	CBaseEntity *pEntity = NULL;
	CNPC_RollerMine *pMine = NULL;

	// Use the template, if we have it
	if ( pszTemplate && pszTemplate[0] )
	{
		MapEntity_ParseEntity( pEntity, pszTemplate, NULL );
		pMine = dynamic_cast<CNPC_RollerMine *>(pEntity);
	}
	else
	{
		pMine = (CNPC_RollerMine*)CreateEntityByName("npc_rollermine");
	}

	if ( pMine )
	{
		pMine->SetAbsOrigin( originStart );
		pMine->SetOwnerEntity( pOwner );
		pMine->Spawn();

		if ( !pszTemplate || !pszTemplate[0] )
		{
			pMine->EmbedOnGroundImpact();
		}
	}
	else
	{
		Warning( "NULL Ent in Rollermine Create!\n" );
	}

	return pMine;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_RollerMine::~CNPC_RollerMine( void )
{
	if ( m_pMotionController != NULL )
	{
		physenv->DestroyMotionController( m_pMotionController );
		m_pMotionController = NULL;
	}

	UnstickFromVehicle();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Precache( void )
{
	PrecacheModel( "models/roller.mdl" );
	PrecacheModel( "models/roller_spikes.mdl" );

	PrecacheModel( "sprites/bluelight1.vmt" );
	PrecacheModel( "sprites/rollermine_shock.vmt" );
	PrecacheModel( "sprites/rollermine_shock_yellow.vmt" );

	PrecacheScriptSound( "NPC_RollerMine.Taunt" );
	PrecacheScriptSound( "NPC_RollerMine.OpenSpikes" );
	PrecacheScriptSound( "NPC_RollerMine.Warn" );
	PrecacheScriptSound( "NPC_RollerMine.Shock" );
	PrecacheScriptSound( "NPC_RollerMine.ExplodeChirp" );
	PrecacheScriptSound( "NPC_RollerMine.Chirp" );
	PrecacheScriptSound( "NPC_RollerMine.ChirpRespond" );
	PrecacheScriptSound( "NPC_RollerMine.ExplodeChirpRespond" );
	PrecacheScriptSound( "NPC_RollerMine.JoltVehicle" );
	PrecacheScriptSound( "NPC_RollerMine.Tossed" );
	PrecacheScriptSound( "NPC_RollerMine.Hurt" );

	PrecacheScriptSound( "NPC_RollerMine.Roll" );
	PrecacheScriptSound( "NPC_RollerMine.RollWithSpikes" );
	PrecacheScriptSound( "NPC_RollerMine.Ping" );
	PrecacheScriptSound( "NPC_RollerMine.Held" );

	PrecacheScriptSound( "NPC_RollerMine.Reprogram" );

	PrecacheMaterial( "effects/rollerglow" );

	gm_iszDropshipClassname = AllocPooledString( "npc_combinedropship" ); // For fast string compares.
#ifdef HL2_EPISODIC
	PrecacheScriptSound( "RagdollBoogie.Zap" );
#endif

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Spawn( void )
{
	Precache();

	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_FORCE_WORLD_ALIGNED | FSOLID_NOT_STANDABLE );

	BaseClass::Spawn();

	AddEFlags( EFL_NO_DISSOLVE );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );

	m_pRollSound = NULL;

	m_bIsOpen = true;
	Close();

	m_bPowerDown = false;
	
	m_flFieldOfView		= -1.0f;
	m_flForwardSpeed	= -1200;
	m_bloodColor		= DONT_BLEED;

	SetHullType(HULL_SMALL_CENTERED);

	SetHullSizeNormal();

	m_flActiveTime		= 0;

	m_bBuried = m_bStartBuried;
	if ( m_bStartBuried )
	{
		trace_t tr;
		Bury( &tr );
	}

	NPCInit();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetDistLook( ROLLERMINE_IDLE_SEE_DIST );

	if( m_bUniformSight )
	{
		m_flSeeVehiclesOnlyBeyond = ROLLERMINE_IDLE_SEE_DIST;
	}
	else
	{
		m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE;
	}

	//Suppress superfluous warnings from animation system
	m_flGroundSpeed = 20;
	m_NPCState		= NPC_STATE_NONE;

	m_rollingSoundState = ROLL_SOUND_OFF;

	m_pConstraint = NULL;
	m_hVehicleStuckTo = NULL;
	m_flPreventUnstickUntil = 0;
	m_flNextHop = 0;

	m_flPowerDownDetonateTime = 0.0f;
	m_bPowerDown = false;
	m_flPowerDownTime = 0.0f;

	//Set their yaw speed to 0 so the motor doesn't rotate them.
	GetMotor()->SetYawSpeed( 0.0f );
	SetRollerSkin();
}

//-----------------------------------------------------------------------------
// Set the contents types that are solid by default to this NPC
//-----------------------------------------------------------------------------
unsigned int CNPC_RollerMine::PhysicsSolidMaskForEntity( void ) const 
{ 
	if ( HasSpawnFlags( SF_ROLLERMINE_PROP_COLLISION ) )
		return MASK_SOLID;

	return MASK_NPCSOLID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Bury( trace_t *tr )
{
	AI_TraceHull( GetAbsOrigin() + Vector(0,0,64), GetAbsOrigin() - Vector( 0, 0, MAX_TRACE_LENGTH ), Vector(-16,-16,-16), Vector(16,16,16), MASK_NPCSOLID, this, GetCollisionGroup(), tr );

	//NDebugOverlay::Box( tr->startpos, Vector(-16,-16,-16), Vector(16,16,16), 255, 0, 0, 64, 10.0 );
	//NDebugOverlay::Box( tr->endpos, Vector(-16,-16,-16), Vector(16,16,16), 0, 255, 0, 64, 10.0 );

	// Move into the ground layer
	Vector buriedPos = tr->endpos - Vector( 0, 0, GetHullHeight() * 0.5 );
	Teleport( &buriedPos, NULL, &vec3_origin );
	SetMoveType( MOVETYPE_NONE );

	SetSchedule( SCHED_ROLLERMINE_BURIED_WAIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::WakeupMine( CAI_BaseNPC *pNPC )
{
	if ( pNPC && pNPC->m_iClassname == m_iClassname && pNPC != this )
	{
		CNPC_RollerMine *pMine = dynamic_cast<CNPC_RollerMine *>(pNPC);
		if ( pMine )
		{
			if ( pMine->m_NPCState == NPC_STATE_IDLE )
			{
				pMine->m_wakeUp = false;
				pMine->SetIdealState( NPC_STATE_ALERT );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::WakeNeighbors()
{
	if ( !m_wakeUp || !IsActive() )
		return;
	m_wakeUp = false;

	if ( m_pSquad )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			WakeupMine( pSquadMember );
		}
		return;
	}

	CBaseEntity *entityList[64];
	Vector range(ROLLERMINE_WAKEUP_DIST,ROLLERMINE_WAKEUP_DIST,64);
	int boxCount = UTIL_EntitiesInBox( entityList, ARRAYSIZE(entityList), GetAbsOrigin()-range, GetAbsOrigin()+range, FL_NPC );
	//NDebugOverlay::Box( GetAbsOrigin(), -range, range, 255, 0, 0, 64, 10.0 );
	int wakeCount = 0;
	while ( boxCount > 0 )
	{
		boxCount--;
		CAI_BaseNPC *pNPC = entityList[boxCount]->MyNPCPointer();
		if ( WakeupMine( pNPC ) )
		{
			wakeCount++;
			if ( wakeCount >= 2 )
				return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if ( NewState == NPC_STATE_IDLE )
	{
		SetDistLook( ROLLERMINE_IDLE_SEE_DIST );
		m_flDistTooFar = ROLLERMINE_IDLE_SEE_DIST;

		if( m_bUniformSight )
		{
			m_flSeeVehiclesOnlyBeyond = ROLLERMINE_IDLE_SEE_DIST;
		}
		else
		{
			m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_IDLE;
		}
		
		m_RollerController.m_vecAngular = vec3_origin;
		m_wakeUp = true;
	}
	else
	{
		if ( OldState == NPC_STATE_IDLE )
		{
			// wake the neighbors!
			WakeNeighbors();
		}
		SetDistLook( ROLLERMINE_NORMAL_SEE_DIST );

		if( m_bUniformSight )
		{
			m_flSeeVehiclesOnlyBeyond = ROLLERMINE_NORMAL_SEE_DIST;
		}
		else
		{
			m_flSeeVehiclesOnlyBeyond = ROLLERMINE_SEE_VEHICLESONLY_BEYOND_NORMAL;
		}

		m_flDistTooFar = ROLLERMINE_NORMAL_SEE_DIST;
	}
	BaseClass::OnStateChange( OldState, NewState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
NPC_STATE CNPC_RollerMine::SelectIdealState( void )
{
	switch ( m_NPCState )
	{
		case NPC_STATE_COMBAT:
		{
			if ( HasCondition( COND_ENEMY_TOO_FAR ) )
			{
				ClearEnemyMemory();
				SetEnemy( NULL );
				m_flGoIdleTime = gpGlobals->curtime + 10;
				return NPC_STATE_ALERT;
			}
		}
	}

	return BaseClass::SelectIdealState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::BecomePhysical( void )
{
	VPhysicsDestroyObject();

	RemoveSolidFlags( FSOLID_NOT_SOLID );

	//Setup the physics controller on the roller
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags() , false );

	if ( pPhysicsObject == NULL )
		return false;

	m_pMotionController = physenv->CreateMotionController( &m_RollerController );
	m_pMotionController->AttachObject( pPhysicsObject, true );

	SetMoveType( MOVETYPE_VPHYSICS );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_pMotionController )
	{
		m_pMotionController->SetEventHandler( &m_RollerController );
	}

	// If we're stuck to a vehicle over a level transition, restart our jolt inputs
	if ( GetVehicleStuckTo() )
	{
		if ( !g_EventQueue.HasEventPending( this, "JoltVehicle" ) )
		{
			g_EventQueue.AddEvent( this, "JoltVehicle", RandomFloat(3,6), NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::CreateVPhysics()
{
	if ( m_bBuried )
	{
		VPhysicsInitStatic();
		return true;
	}
	else
	{
		return BecomePhysical();
	}
}	

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_RollerMine::RunAI()
{
	if( m_bTurnedOn )
	{
		// Scare combine if hacked by Alyx.
		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

		Vector vecVelocity;

		if ( pPhysicsObject != NULL )
		{
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );
		}

		if( !m_bHeld && vecVelocity.Length() > 64.0 )
		{
			if( m_bHackedByAlyx )
			{
				// Scare combine
				CSoundEnt::InsertSound( (SOUND_DANGER | SOUND_CONTEXT_COMBINE_ONLY | SOUND_CONTEXT_REACT_TO_SOURCE | SOUND_CONTEXT_DANGER_APPROACH), WorldSpaceCenter() + Vector( 0, 0, 32 ) + vecVelocity * 0.5f, 120.0f, 0.2f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
			}
			else
			{
				// Scare player allies
				CSoundEnt::InsertSound( (SOUND_DANGER | SOUND_CONTEXT_EXCLUDE_COMBINE | SOUND_CONTEXT_REACT_TO_SOURCE | SOUND_CONTEXT_DANGER_APPROACH), WorldSpaceCenter() + Vector( 0, 0, 32 ) + vecVelocity * 0.5f, 120.0f, 0.2f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
			}
		}

		BaseClass::RunAI();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerMine::RangeAttack1Conditions( float flDot, float flDist )
{
	if( HasCondition( COND_SEE_ENEMY ) == false )
		return COND_NONE;

	if ( EnemyInVehicle() )
		return COND_CAN_RANGE_ATTACK1;

	if( flDist > ROLLERMINE_MAX_ATTACK_DIST  )
		return COND_TOO_FAR_TO_ATTACK;
	
	if (flDist < ROLLERMINE_MIN_ATTACK_DIST )
		return COND_TOO_CLOSE_TO_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerMine::SelectSchedule( void )
{
	if ( m_bPowerDown )
		return SCHED_ROLLERMINE_POWERDOWN;

	if ( m_bBuried )
	{
		if ( HasCondition(COND_NEW_ENEMY) || HasCondition(COND_LIGHT_DAMAGE) )
			return SCHED_ROLLERMINE_BURIED_UNBURROW;

		return SCHED_ROLLERMINE_BURIED_WAIT;
	}

	//If we're held, don't try and do anything
	if ( ( m_bHeld ) || !IsActive() || m_hVehicleStuckTo )
		return SCHED_ALERT_STAND;

	// If we can see something we're afraid of, run from it
	if ( HasCondition( COND_SEE_FEAR ) )
		return SCHED_ROLLERMINE_FLEE;

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:

		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			return SCHED_ROLLERMINE_RANGE_ATTACK1;
		
		return SCHED_ROLLERMINE_CHASE_ENEMY;
		break;

	default:
		break;
	}

	// Rollermines never wait to fall to the ground
	ClearCondition( COND_FLOATING_OFF_GROUND );

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_RollerMine::GetHackedIdleSchedule( void )
{
	// If we've been hacked, return to the player
	if ( !m_bHackedByAlyx || m_bHeld )
		return SCHED_NONE;

	// Are we near the player?
	CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );
	if ( !pPlayer )
		return SCHED_NONE;

	if ( !HasCondition(COND_SEE_PLAYER) )
		return SCHED_ROLLERMINE_PATH_TO_PLAYER;

	if ( GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() ) > ROLLERMINE_RETURN_TO_PLAYER_DIST )
		return SCHED_ROLLERMINE_ROLL_TO_PLAYER;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_RollerMine::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		{
			int iSched = GetHackedIdleSchedule();
			if ( iSched != SCHED_NONE )
				return iSched;

			return SCHED_IDLE_STAND;
		}
		break;

	case SCHED_ALERT_STAND:
		{
			int iSched = GetHackedIdleSchedule();
			if ( iSched != SCHED_NONE )
				return iSched;

			return SCHED_ROLLERMINE_ALERT_STAND;
		}
		break;

	case SCHED_ROLLERMINE_RANGE_ATTACK1:
		if( HasCondition(COND_ENEMY_OCCLUDED) )
		{
			// Because of an unfortunate arrangement of cascading failing schedules, the rollermine
			// could end up here with instructions to drive towards the target, although the target is
			// not in sight. Nudge around randomly until we're back on the nodegraph.
			return SCHED_ROLLERMINE_NUDGE_TOWARDS_NODES;
		}
		break;
	}

	return scheduleType;
}


#if 0
#define	ROLLERMINE_DETECTION_RADIUS		350
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::DetectedEnemyInProximity( void )
{
	CBaseEntity *pEnt = NULL;
	CBaseEntity *pBestEnemy = NULL;
	float		flBestDist = MAX_TRACE_LENGTH;

	while ( ( pEnt = gEntList.FindEntityInSphere( pEnt, GetAbsOrigin(), ROLLERMINE_DETECTION_RADIUS ) ) != NULL )
	{
		if ( IRelationType( pEnt ) != D_HT )
			continue;

		float distance = ( pEnt->GetAbsOrigin() - GetAbsOrigin() ).Length();
		
		if ( distance >= flBestDist )
			continue;

		pBestEnemy = pEnt;
		flBestDist = distance;
	}

	if ( pBestEnemy != NULL )
	{
		SetEnemy( pBestEnemy );
		return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSightEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::QuerySeeEntity(CBaseEntity *pSightEnt, bool bOnlyHateOrFearIfNPC)
{
	if ( IRelationType( pSightEnt ) == D_FR )
	{
		// Only see feared objects up close
		float flDist = (WorldSpaceCenter() - pSightEnt->WorldSpaceCenter()).LengthSqr();
		if ( flDist > ROLLERMINE_FEAR_DISTANCE )
			return false;
	}

	return BaseClass::QuerySeeEntity(pSightEnt, bOnlyHateOrFearIfNPC);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FACE_REASONABLE:
	case TASK_FACE_SAVEPOSITION:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_AWAY_FROM_SAVEPOSITION:
	case TASK_FACE_HINTNODE:
	case TASK_FACE_ENEMY:
	case TASK_FACE_PLAYER:
	case TASK_FACE_PATH:
	case TASK_FACE_IDEAL:
		// This only applies to NPCs that aren't spheres with omnidirectional eyesight.
		TaskComplete();
		break;

	case TASK_ROLLERMINE_UNBURROW:
		
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			SetMoveType( MOVETYPE_NOCLIP );
			SetAbsVelocity( Vector( 0, 0, 256 ) );
			Open();

			trace_t	tr;
			AI_TraceLine( GetAbsOrigin()+Vector(0,0,1), GetAbsOrigin()-Vector(0,0,64), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction < 1.0f )
			{
				UTIL_CreateAntlionDust( tr.endpos + Vector(0,0,24), GetLocalAngles() );		
			}
		}

		return;
		break;

	case TASK_ROLLERMINE_BURIED_WAIT:
		if ( HasCondition( COND_SEE_ENEMY ) )
		{
			TaskComplete();
		}
		break;

	case TASK_STOP_MOVING:

		//Stop turning
		m_RollerController.m_vecAngular = vec3_origin;
		
		TaskComplete();
		return;
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		{
			// TASK_RUN_PATH and TASK_WALK_PATH work different on the rollermine and run until movement is done,
			// so movement is already complete when entering this task.
			TaskComplete();
		}
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}

			pPhysicsObject->Wake();
		}
		break;

	case TASK_ROLLERMINE_CHARGE_ENEMY:
	case TASK_ROLLERMINE_RETURN_TO_PLAYER:
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			
			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}
			
			pPhysicsObject->Wake();

			m_flChargeTime = gpGlobals->curtime;
		}

		break;

	case TASK_ROLLERMINE_GET_PATH_TO_FLEE:
		{
			// Find the nearest thing we're afraid of, and move away from it.
			float flNearest = ROLLERMINE_FEAR_DISTANCE;
			EHANDLE hNearestEnemy = NULL;
			AIEnemiesIter_t iter;
			for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst( &iter ); pEMemory != NULL; pEMemory = GetEnemies()->GetNext( &iter ) )
			{
				CBaseEntity *pEnemy = pEMemory->hEnemy;
				if ( !pEnemy || !pEnemy->IsAlive() )
					continue;
				if ( IRelationType( pEnemy ) != D_FR )
					continue;			

				float flDist = (WorldSpaceCenter() - pEnemy->WorldSpaceCenter()).LengthSqr();
				if ( flDist < flNearest )
				{
					flNearest = flDist;
					hNearestEnemy = pEnemy;
				}
			}

			if ( !hNearestEnemy )
			{
				TaskFail("Couldn't find nearest feared object.");
				break;
			}

			GetMotor()->SetIdealYawToTarget( hNearestEnemy->WorldSpaceCenter() );
			ChainStartTask( TASK_MOVE_AWAY_PATH, pTask->flTaskData );
		}
		break;

	case TASK_ROLLERMINE_NUDGE_TOWARDS_NODES:
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

			if( pPhysicsObject )
			{
				// Try a few times to find a direction to shove ourself
				for( int i = 0 ; i < 4 ; i++ )
				{
					int x,y;

					x = random->RandomInt( -1, 1 );
					y = random->RandomInt( -1, 1 );

					Vector vecNudge(x, y, 0.0f);

					trace_t tr;

					// Try to move in a direction with a couple of feet of clearance.
					UTIL_TraceLine( WorldSpaceCenter(), WorldSpaceCenter() + vecNudge * 24.0f, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

					if( tr.fraction == 1.0 )
					{
						vecNudge *= (pPhysicsObject->GetMass() * 75.0f);
						vecNudge += Vector(0,0,pPhysicsObject->GetMass() * 75.0f);
						pPhysicsObject->ApplyForceCenter( vecNudge );
						break;
					}
				}
			}

			TaskComplete();
		}
		break;

	case TASK_ROLLERMINE_POWERDOWN:
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERMINE_UNBURROW:
		{	
			Vector	vCenter = WorldSpaceCenter();

			// Robin: HACK: Bloat the rollermine check to catch the model switch (roller.mdl->roller_spikes.mdl)
			trace_t	tr;
			AI_TraceHull( vCenter, vCenter, Vector(-16,-16,-16), Vector(16,16,16), MASK_NPCSOLID, this, GetCollisionGroup(), &tr );

			if ( tr.fraction == 1 && tr.allsolid != 1 && (tr.startsolid != 1) )
			{
				if ( BecomePhysical() )
				{
					Hop( 256 );
					m_bBuried = false;
					TaskComplete();
					SetIdealState( NPC_STATE_ALERT );
				}
			}
		}

		return;
		break;

	case TASK_ROLLERMINE_BURIED_WAIT:
		if ( HasCondition( COND_SEE_ENEMY ) || HasCondition( COND_LIGHT_DAMAGE ) )
		{
			TaskComplete();
		}
		break;

	case TASK_ROLLERMINE_GET_PATH_TO_FLEE:
		{
			ChainRunTask( TASK_MOVE_AWAY_PATH, pTask->flTaskData );
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		{
			// TASK_RUN_PATH and TASK_WALK_PATH work different on the rollermine and run until movement is done,
			// so movement is already complete when entering this task.
			TaskComplete();
		}
		break;

	case TASK_RUN_PATH:
	case TASK_WALK_PATH:

		if ( m_bHeld || m_hVehicleStuckTo )
		{
			TaskFail( "Player interrupted by grabbing" );
			break;
		}

		// If we were fleeing, but we've lost sight of the thing scaring us, stop
		if ( IsCurSchedule(SCHED_ROLLERMINE_FLEE) && !HasCondition( COND_SEE_FEAR ) )
		{
			TaskComplete();
			break;
		}

		if ( !GetNavigator()->IsGoalActive() )
		{
			TaskComplete();
			return;
		}

		// Start turning early
		if( (GetLocalOrigin() - GetNavigator()->GetCurWaypointPos() ).Length() <= 64 )
		{
			if( GetNavigator()->CurWaypointIsGoal() )
			{
				// Hit the brakes a bit.
				float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );
				Vector vecRight;
				AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

				m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, -m_flForwardSpeed * 5 );

				TaskComplete();
				return;
			}

			GetNavigator()->AdvancePath();	
		}

		{
			float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );

			Vector vecRight;
			Vector vecToPath; // points at the path
			AngleVectors( QAngle( 0, yaw, 0 ), &vecToPath, &vecRight, NULL );

			// figure out if the roller is turning. If so, cut the throttle a little.
			float flDot;
			Vector vecVelocity;

			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			
			if ( pPhysicsObject == NULL )
			{
				assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}
			
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );

			VectorNormalize( vecVelocity );

			vecVelocity.z = 0;

			flDot = DotProduct( vecVelocity, vecToPath );

			m_RollerController.m_vecAngular = vec3_origin;

			if( flDot > 0.25 && flDot < 0.7 )
			{
				// Feed a little torque backwards into the axis perpendicular to the velocity.
				// This will help get rid of momentum that would otherwise make us overshoot our goal.
				Vector vecCompensate;

				vecCompensate.x = vecVelocity.y;
				vecCompensate.y = -vecVelocity.x;
				vecCompensate.z = 0;

				m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			}

			if( m_bHackedByAlyx )
			{
				// Move faster. 
				m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed * 2.0f );
			}
			else
			{
				m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed );
			}
		}
		break;

	case TASK_ROLLERMINE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
			{
				TaskFail( FAIL_NO_ENEMY );
				break;
			}

			if ( m_bHeld || m_hVehicleStuckTo )
			{
				TaskComplete();
				break;
			}

			CBaseEntity *pEnemy = GetEnemy();
			Vector vecTargetPosition = pEnemy->GetAbsOrigin();

			// If we're chasing a vehicle, try and get ahead of it
			if ( EnemyInVehicle() )
			{
				CBaseCombatCharacter *pCCEnemy = pEnemy->MyCombatCharacterPointer();
				float flT;

				// Project it's velocity and find our closest point on that line. Do it all in 2d space.
				Vector vecVehicleVelocity = pCCEnemy->GetVehicleEntity()->GetSmoothedVelocity();
				Vector vecProjected = vecTargetPosition + (vecVehicleVelocity * 1.0);
				Vector2D vecProjected2D( vecProjected.x, vecProjected.y );
				Vector2D vecTargetPosition2D( vecTargetPosition.x, vecTargetPosition.y );
				Vector2D vecOrigin2D( GetAbsOrigin().x, GetAbsOrigin().y );
				Vector2D vecIntercept2D;

				CalcClosestPointOnLine2D( vecOrigin2D, vecTargetPosition2D, vecProjected2D, vecIntercept2D, &flT );
				Vector vecIntercept( vecIntercept2D.x, vecIntercept2D.y, GetAbsOrigin().z );
				
				//NDebugOverlay::Line( vecTargetPosition, vecProjected, 0,255,0, true, 0.1 );

				// If we're ahead of the line somewhere, try to intercept
				if ( flT > 0 )
				{
					// If it's beyond the end of the intercept line, just move towards the end of the line
					if ( flT > 1 )
					{
						vecIntercept.x = vecProjected.x;
						vecIntercept.y = vecProjected.y;
					}

					// If we're closer to the intercept point than to the vehicle, move towards the intercept
					if ( (GetAbsOrigin() - vecTargetPosition).LengthSqr() > (GetAbsOrigin() - vecIntercept).LengthSqr() )
					{
						//NDebugOverlay::Box( vecIntercept, -Vector(20,20,20), Vector(20,20,20), 255,0,0, 0.1, 0.1 );

						// Only use this position if it's clear
						if ( enginetrace->GetPointContents( vecIntercept ) != CONTENTS_SOLID )
						{
							vecTargetPosition = vecIntercept;
						}
					}
				}

				//NDebugOverlay::Box( vecTargetPosition, -Vector(20,20,20), Vector(20,20,20), 255,255,255, 0.1, 0.1 );
			}

			float flTorqueFactor;
			Vector vecToTarget = vecTargetPosition - GetLocalOrigin();
			float yaw = UTIL_VecToYaw( vecToTarget );
			Vector vecRight;

			AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

			//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin() + (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()), 0,255,0, true, 0.1 );

			float flDot;

			// Figure out whether to continue the charge.
			// (Have I overrun the target?)			
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

			if ( pPhysicsObject == NULL )
			{
//				Assert(0);
				TaskFail("Roller lost internal physics object?");
				return;
			}

			Vector vecVelocity;
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );
			VectorNormalize( vecVelocity );

			VectorNormalize( vecToTarget );

			flDot = DotProduct( vecVelocity, vecToTarget );

			// more torque the longer the roller has been going.
			flTorqueFactor = 1 + (gpGlobals->curtime - m_flChargeTime) * 2;

			float flMaxTorque = ROLLERMINE_MAX_TORQUE_FACTOR;
			
			// Friendly rollermines go a little slower
			if ( HasSpawnFlags( SF_ROLLERMINE_FRIENDLY ) )
			{
				flMaxTorque *= 0.75;
			}

			if( flTorqueFactor < 1 )
			{
				flTorqueFactor = 1;
			}
			else if( flTorqueFactor > flMaxTorque)
			{
				flTorqueFactor = flMaxTorque;
			}

			Vector vecCompensate;

			vecCompensate.x = vecVelocity.y;
			vecCompensate.y = -vecVelocity.x;
			vecCompensate.z = 0;
			VectorNormalize( vecCompensate );

			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed  * flTorqueFactor );
		
			// Taunt when I get closer
			if( !(m_iSoundEventFlags & ROLLERMINE_SE_TAUNT) && UTIL_DistApprox( GetLocalOrigin(), vecTargetPosition ) <= 400 )
			{
				m_iSoundEventFlags |= ROLLERMINE_SE_TAUNT; // Don't repeat.

				EmitSound( "NPC_RollerMine.Taunt" );
			}

			// Jump earlier when chasing a vehicle
			float flThreshold = ROLLERMINE_OPEN_THRESHOLD;
			if ( EnemyInVehicle() )
			{
				flThreshold = ROLLERMINE_VEHICLE_OPEN_THRESHOLD;
			}

			// Open the spikes if i'm close enough to cut the enemy!!
  			if( ( m_bIsOpen == false ) && ( ( UTIL_DistApprox( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() ) <= flThreshold ) || !IsActive() ) )
			{
				Open();
			}
			else if ( m_bIsOpen )
			{
				float flDistance = UTIL_DistApprox( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() );
				if ( flDistance >= flThreshold )
				{
					// Otherwise close them if the enemy is getting away!
					Close();
				}
				else if ( EnemyInVehicle() && flDistance < ROLLERMINE_VEHICLE_HOP_THRESHOLD )
				{
					// Keep trying to hop when we're ramming a vehicle, so we're visible to the player
					if ( vecVelocity.x != 0 && vecVelocity.y != 0 && flTorqueFactor > 3 && flDot > 0.0 )
					{
						Hop( 300 );
					}
				}
			}

			// If we drive past, close the blades and make a new plan.
			if ( !EnemyInVehicle() )
			{
				if( vecVelocity.x != 0 && vecVelocity.y != 0 )
				{
					if( gpGlobals->curtime - m_flChargeTime > 1.0 && flTorqueFactor > 1 &&  flDot < 0.0 )
					{
						if( m_bIsOpen )
						{
							Close();
						}

						TaskComplete();
					}
				}
			}
		}
		break;

	case TASK_ROLLERMINE_RETURN_TO_PLAYER:
		{
			if ( ConditionsGathered() && !HasCondition(COND_SEE_PLAYER) )
			{
				TaskFail( FAIL_NO_PLAYER );
				return;
			}

			CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );
			if ( !pPlayer || m_bHeld || m_hVehicleStuckTo )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			Vector vecTargetPosition = pPlayer->GetAbsOrigin();
			float flTorqueFactor;
			Vector vecToTarget = vecTargetPosition - GetLocalOrigin();
			float yaw = UTIL_VecToYaw( vecToTarget );
			Vector vecRight;

			AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

			float flDot;

			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			if ( pPhysicsObject == NULL )
			{
				TaskFail("Roller lost internal physics object?");
				return;
			}

			Vector vecVelocity;
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );
			VectorNormalize( vecVelocity );
			VectorNormalize( vecToTarget );

			flDot = DotProduct( vecVelocity, vecToTarget );

			// more torque the longer the roller has been going.
			flTorqueFactor = 1 + (gpGlobals->curtime - m_flChargeTime) * 2;

			float flMaxTorque = ROLLERMINE_MAX_TORQUE_FACTOR * 0.75;
			if( flTorqueFactor < 1 )
			{
				flTorqueFactor = 1;
			}
			else if( flTorqueFactor > flMaxTorque)
			{
				flTorqueFactor = flMaxTorque;
			}

			Vector vecCompensate;

			vecCompensate.x = vecVelocity.y;
			vecCompensate.y = -vecVelocity.x;
			vecCompensate.z = 0;
			VectorNormalize( vecCompensate );

			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed  * flTorqueFactor );

			// Once we're near the player, slow & stop
			if ( GetAbsOrigin().DistToSqr( vecTargetPosition ) < (ROLLERMINE_RETURN_TO_PLAYER_DIST*2.0) )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_ROLLERMINE_POWERDOWN:
		{
			if ( m_flPowerDownTime <= gpGlobals->curtime )
			{
				m_flNextHop = gpGlobals->curtime;
				m_flPowerDownTime = gpGlobals->curtime + RandomFloat( 0.3, 0.9 );
				EmitSound( "NPC_RollerMine.Hurt" );

				CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 0.5f, this );

				if ( m_bIsOpen == false )
				{
					Open();
				}
				else
				{
					Close();
				}
			}

			if ( m_flPowerDownDetonateTime <= gpGlobals->curtime )
			{
				SetThink( &CNPC_RollerMine::PreDetonate );
				SetNextThink( gpGlobals->curtime + 0.5f );
			}

			// No TaskComplete() here, because the task will never complete. The rollermine will explode.
		}
		break;	

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Open( void )
{
	// Friendly rollers cannot open
	if ( HasSpawnFlags( SF_ROLLERMINE_FRIENDLY ) )
		return;

	if ( m_bIsOpen == false )
	{
		SetModel( "models/roller_spikes.mdl" );
        SetRollerSkin();

		EmitSound( "NPC_RollerMine.OpenSpikes" );

		SetTouch( &CNPC_RollerMine::ShockTouch );
		m_bIsOpen = true;

		// Don't hop if we're constrained
		if ( !m_pConstraint )
		{
			if ( EnemyInVehicle() )
			{
				Hop( 256 );
			}
			else if ( !GetEnemy() || GetEnemy()->Classify() != CLASS_BULLSEYE )		// Don't hop when attacking bullseyes
			{
				Hop( 128 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::SetRollerSkin( void )
{
	if ( m_bPowerDown == true )
	{
		m_nSkin = (int)ROLLER_SKIN_DETONATE;
	}
	else if ( m_bHackedByAlyx == true )
	{
		m_nSkin = (int)ROLLER_SKIN_FRIENDLY;
	}
	else
	{
		m_nSkin = (int)ROLLER_SKIN_REGULAR;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Close( void )
{
	// Not allowed to close while primed, because we're going to detonate on touch
	if ( m_bIsPrimed )
		return;

	if ( m_bIsOpen && !IsShocking() )
	{
		SetModel( "models/roller.mdl" );

		SetRollerSkin();

		SetTouch( NULL );
		m_bIsOpen = false;

		m_iSoundEventFlags = ROLLERMINE_SE_CLEAR;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerMine::SpikeTouch( CBaseEntity *pOther )
{
	/*
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	if ( m_bHeld )
		return;

	if ( pOther->IsPlayer() )
		return;

	if ( pOther->m_takedamage != DAMAGE_YES )
		return;

	// If we just hit a breakable glass object, don't explode. We want to blow through it.
	CBreakable *pBreakable = dynamic_cast<CBreakable*>(pOther);
	if ( pBreakable && pBreakable->GetMaterialType() == matGlass )
		return;

	Explode();
	EmitSound( "NPC_RollerMine.Warn" );
	*/

	//FIXME: Either explode within certain rules, never explode, or just shock the hit victim
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::CloseTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	if ( IsShocking() )
		return;

	bool bOtherIsDead = ( pOther->MyNPCPointer() && !pOther->MyNPCPointer()->IsAlive() );
	bool bOtherIsNotarget = ( ( pOther->GetFlags() & FL_NOTARGET ) != 0 );

	if ( !bOtherIsDead && !bOtherIsNotarget )
	{
		Disposition_t disp = IRelationType(pOther);

		if ( (disp == D_HT || disp == D_FR) )
		{
			ShockTouch( pOther );
			return;
		}
	}

	Close();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::EmbedTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	m_bEmbedOnGroundImpact = false;

	// Did we hit the world?
	if ( pOther->entindex() == 0 )
	{
		m_bBuried = true;
		trace_t tr;
		Bury( &tr );

		// Destroy out physics object and become static
		VPhysicsDestroyObject();
		CreateVPhysics();

		// Drop a decal on the ground where we impacted
		UTIL_DecalTrace( &tr, "Rollermine.Crater" );

		// Make some dust
		UTIL_CreateAntlionDust( tr.endpos, GetLocalAngles() );
	}

	// Don't try and embed again
	SetTouch( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::IsPlayerVehicle( CBaseEntity *pEntity )
{
	IServerVehicle *pVehicle = pEntity->GetServerVehicle();
	if ( pVehicle )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pVehicle->GetPassenger() );
		if ( pPlayer != NULL )
		{
			Disposition_t disp = IRelationType(pPlayer);

			if ( disp == D_HT || disp == D_FR )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_RollerMine::GetAttackDamageScale( CBaseEntity *pVictim )
{
	// If we're friendly, don't damage players or player-friendly NPCs, even with collisions
	if ( HasSpawnFlags( SF_ROLLERMINE_FRIENDLY ) )
	{
		if ( pVictim->IsPlayer() )
			return 0;
		
		if ( pVictim->MyNPCPointer() )
		{
			// If we don't hate the player, we're immune
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
			if ( pPlayer && pVictim->MyNPCPointer()->IRelationType( pPlayer ) != D_HT )
				return 0.0;
		}
	}

	return BaseClass::GetAttackDamageScale( pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::ShockTarget( CBaseEntity *pOther )
{
	CBeam *pBeam;

	if( m_bHackedByAlyx )
	{
		pBeam = CBeam::BeamCreate( "sprites/rollermine_shock_yellow.vmt", 4 );
	}
	else
	{
		pBeam = CBeam::BeamCreate( "sprites/rollermine_shock.vmt", 4 );
	}

	int startAttach = -1;

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(pOther);

	if ( pBeam != NULL )
	{
		pBeam->EntsInit( pOther, this );

		if ( pAnimating && pAnimating->GetModel() )
		{
			startAttach = pAnimating->LookupAttachment("beam_damage" );
			pBeam->SetStartAttachment( startAttach );
		}

		// Change this up a little for first person hits
		if ( pOther->IsPlayer() )
		{
			pBeam->SetEndWidth( 8 );
			pBeam->SetNoise( 4 );
			pBeam->LiveForTime( 0.2f );
		}
		else
		{
			pBeam->SetEndWidth( 16 );
			pBeam->SetNoise( 16 );
			pBeam->LiveForTime( 0.5f );
		}
		
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( 1 );
		pBeam->SetBrightness( 255 );
		pBeam->SetColor( 255, 255, 255 );
		pBeam->RelinkBeam();
	}
	
	Vector shockPos = pOther->WorldSpaceCenter();

	if ( startAttach > 0 && pAnimating )
	{
		pAnimating->GetAttachment( startAttach, shockPos );
	}

	Vector shockDir = ( GetAbsOrigin() - shockPos );
	VectorNormalize( shockDir );

	CPVSFilter filter( shockPos );
	te->GaussExplosion( filter, 0.0f, shockPos, shockDir, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::NotifyInteraction( CAI_BaseNPC *pUser )
{
	// For now, turn green so we can tell who is hacked.
	m_bHackedByAlyx = true; 
	SetRollerSkin();
	GetEnemies()->SetFreeKnowledgeDuration( 30.0f );

	// Play the hax0red sound
	EmitSound( "NPC_RollerMine.Reprogram" );

	// Force the rollermine open here. At very least, this ensures that the 
	// correct, smaller bounding box is recomputed around it.
	Open();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::ShockTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	if ( m_bHeld || m_hVehicleStuckTo || gpGlobals->curtime < m_flShockTime )
		return;

	// error?
	Assert( !m_bIsPrimed );

	Disposition_t disp = IRelationType(pOther);

	// Ignore anyone that I'm friendly or neutral to.
	if( disp != D_HT && disp != D_FR)
		return;

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	// Calculate a collision force
	Vector impulse = WorldSpaceCenter() - pOther->WorldSpaceCenter();
	impulse.z = 0;
	VectorNormalize( impulse );
	impulse.z = 0.75;
	VectorNormalize( impulse );
	impulse *= 600;

	// Stun the roller
	m_flActiveTime = gpGlobals->curtime + GetStunDelay();

	// If we're a 'friendly' rollermine, just push the player a bit
	if ( HasSpawnFlags( SF_ROLLERMINE_FRIENDLY ) )
	{
		if ( pOther->IsPlayer() )
		{
			Vector vecForce = -impulse * 0.5;
			pOther->ApplyAbsVelocityImpulse( vecForce );
		}
		return;
	}

	// jump up at a 30 degree angle away from the guy we hit
	SetTouch( &CNPC_RollerMine::CloseTouch );
	Vector vel;
	pPhysics->SetVelocity( &impulse, NULL );
	EmitSound( "NPC_RollerMine.Shock" );
	// Do a shock effect
	ShockTarget( pOther );

	m_flShockTime = gpGlobals->curtime + 1.25;

	// Calculate physics force
	Vector out;
	pOther->CollisionProp()->CalcNearestPoint( WorldSpaceCenter(), &out );

	Vector vecForce = ( -impulse * pPhysics->GetMass() * 10 );
	CTakeDamageInfo	info( this, this, vecForce, out, sk_rollermine_shock.GetFloat(), DMG_SHOCK );

	if( FClassnameIs( pOther, "npc_combine_s" ) )
	{
		if( pOther->GetHealth() <= (pOther->GetMaxHealth() / 2) ) 
		{
			// Instant special death for a combine soldier who has less than half health.
			Vector vecDamageForce = pOther->WorldSpaceCenter() - WorldSpaceCenter();
			VectorNormalize( vecDamageForce );

			IPhysicsObject *pPhysics = pOther->VPhysicsGetObject();

			if( pPhysics )
			{
				vecDamageForce *= (pPhysics->GetMass() * 200.0f);

				// Slam Z component with some good, reliable upwards velocity.
				vecDamageForce.z = pPhysics->GetMass() * 200.0f;
			}

			pOther->MyCombatCharacterPointer()->BecomeRagdollBoogie( this, vecDamageForce, 5.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );
			return;
		}
		else
		{
			info.SetDamage( pOther->GetMaxHealth()/2 );
		}
	}

	pOther->TakeDamage( info );

	// Knock players back a bit
	if ( pOther->IsPlayer() )
	{
		vecForce = -impulse;
		pOther->ApplyAbsVelocityImpulse( vecForce );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Make sure we don't keep hitting the same entity
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	if ( pEvent->deltaCollisionTime < 0.5 && (pOther == this) )
		return;

	BaseClass::VPhysicsCollision( index, pEvent );

	// If we've just hit a vehicle, we want to stick to it
	if ( m_bHeld || m_hVehicleStuckTo || !IsPlayerVehicle( pOther ) )
	{
		// Are we supposed to be embedding ourselves?
		if ( m_bEmbedOnGroundImpact )
		{
			// clear the flag so we don't queue more than once
			m_bEmbedOnGroundImpact = false;
			// call this when physics is done
			g_PostSimulationQueue.QueueCall( this, &CNPC_RollerMine::EmbedTouch, pOther );
		}
		return;
	}

	StickToVehicle( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StickToVehicle( CBaseEntity *pOther )
{
	IPhysicsObject *pOtherPhysics = pOther->VPhysicsGetObject();
	if ( !pOtherPhysics )
		return;

	// Don't stick to the wheels
	if ( pOtherPhysics->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
		return;

	// Destroy our constraint. This can happen if we had our constraint broken
	// and we still haven't cleaned up our constraint.
	UnstickFromVehicle();

	// We've hit the vehicle that the player's in.
	// Stick to it and slow it down.
	m_hVehicleStuckTo = pOther;

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	// Constrain us to the vehicle
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( pOtherPhysics, pPhysics );
	fixed.constraint.Defaults();
	fixed.constraint.forceLimit	= ImpulseScale( pPhysics->GetMass(), 200 );
	fixed.constraint.torqueLimit = ImpulseScale( pPhysics->GetMass(), 800 );
	m_pConstraint = physenv->CreateFixedConstraint( pOtherPhysics, pPhysics, NULL, fixed );
	m_pConstraint->SetGameData( (void *)this );

	// Kick the vehicle so the player knows we've arrived
	Vector impulse = pOther->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( impulse );
	impulse.z = -0.75;
	VectorNormalize( impulse );
	impulse *= 600;
	Vector vecForce = impulse * pPhysics->GetMass() * 10;
	pOtherPhysics->ApplyForceOffset( vecForce, GetAbsOrigin() );

	// Get the velocity at the point we're sticking to
	Vector vecVelocity;
	pOtherPhysics->GetVelocityAtPoint( GetAbsOrigin(), &vecVelocity );
	AngularImpulse angNone( 0.0f, 0.0f, 0.0f );
	pPhysics->SetVelocity( &vecVelocity, &angNone );

	// Make sure we're spiky
	Open();

	AnnounceArrivalToOthers( pOther );

	// Also, jolt the vehicle sometime in the future
	g_EventQueue.AddEvent( this, "JoltVehicle", RandomFloat(3,6), NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_RollerMine::CountRollersOnMyVehicle( CUtlVector<CNPC_RollerMine*> *pRollerList )
{
	CBaseEntity *entityList[64];
	Vector range(256,256,256);
	pRollerList->AddToTail( this );
	int boxCount = UTIL_EntitiesInBox( entityList, ARRAYSIZE(entityList), GetAbsOrigin()-range, GetAbsOrigin()+range, FL_NPC );
	for ( int i = 0; i < boxCount; i++ )
	{
		CAI_BaseNPC *pNPC = entityList[i]->MyNPCPointer();
		if ( pNPC && pNPC->m_iClassname == m_iClassname && pNPC != this )
		{
			// Found another rollermine
			CNPC_RollerMine *pMine = dynamic_cast<CNPC_RollerMine*>(pNPC);
			Assert( pMine );

			// Is he stuck to the same vehicle?
			if ( pMine->GetVehicleStuckTo() == GetVehicleStuckTo() )
			{
				pRollerList->AddToTail( pMine );
			}
		}
	}

	return pRollerList->Count();
}

//-----------------------------------------------------------------------------
// Purpose: Tell other rollermines on the vehicle that I've just arrived
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::AnnounceArrivalToOthers( CBaseEntity *pOther )
{
	// Now talk to any other rollermines stuck to the same vehicle
	CUtlVector<CNPC_RollerMine*> aRollersOnVehicle;
	int iRollers = CountRollersOnMyVehicle( &aRollersOnVehicle );

	// Stop all rollers on the vehicle falling off due to the force of the arriving one
	for ( int i = 0; i < iRollers; i++ )
	{
		aRollersOnVehicle[i]->PreventUnstickUntil( gpGlobals->curtime + 1 );
	}

	// See if we've got enough rollers on the vehicle to start being mean
	/*
	if ( iRollers >= ROLLERMINE_REQUIRED_TO_EXPLODE_VEHICLE )
	{
		// Alert the others
		EmitSound( "NPC_RollerMine.ExplodeChirp" );

		// Tell everyone to explode shortly
		for ( int i = 0; i < iRollers; i++ )
		{
			variant_t emptyVariant;
			g_EventQueue.AddEvent( aRollersOnVehicle[i], "RespondToExplodeChirp", RandomFloat(2,5), NULL, NULL );
		}
	}
	else 
	{
	*/
	// If there's other rollers on the vehicle, talk to them
	if ( iRollers > 1 )
	{
		// Chirp to the others
		EmitSound( "NPC_RollerMine.Chirp" );

		// Tell the others to respond (skip first slot, because that's me)
		for ( int i = 1; i < iRollers; i++ )
		{
			variant_t emptyVariant;
			g_EventQueue.AddEvent( aRollersOnVehicle[i], "RespondToChirp", RandomFloat(2,3), NULL, NULL );
		}
	}
//	}
}

//-----------------------------------------------------------------------------
// Purpose: Physics system has just told us our constraint has been broken
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputConstraintBroken( inputdata_t &inputdata )
{
	// Prevent rollermines being dislodged right as they stick
	if ( m_flPreventUnstickUntil > gpGlobals->curtime )
		return;

	// We can't delete it here safely
	UnstickFromVehicle();
	Close();

	// dazed
	m_RollerController.m_vecAngular.Init();
	m_flActiveTime = gpGlobals->curtime + GetStunDelay();
}

//-----------------------------------------------------------------------------
// Purpose: Respond to another rollermine that's chirped at us
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputRespondToChirp( inputdata_t &inputdata )
{
	EmitSound( "NPC_RollerMine.ChirpRespond" );
}

//-----------------------------------------------------------------------------
// Purpose: Respond to another rollermine's signal to detonate
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputRespondToExplodeChirp( inputdata_t &inputdata )
{
	EmitSound( "NPC_RollerMine.ExplodeChirpRespond" );

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: Apply a physics force to the vehicle we're in
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputJoltVehicle( inputdata_t &inputdata )
{
	Assert( GetVehicleStuckTo() );

	// First, tell all rollers on the vehicle not to fall off
	CUtlVector<CNPC_RollerMine*> aRollersOnVehicle;
	int iRollers = CountRollersOnMyVehicle( &aRollersOnVehicle );
	for ( int i = 0; i < iRollers; i++ )
	{
		aRollersOnVehicle[i]->PreventUnstickUntil( gpGlobals->curtime + 1 );
	}

	// Now smack the vehicle
	Vector impulse = GetVehicleStuckTo()->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( impulse );
	// Randomly apply a little vertical lift, to get the wheels off the ground
	impulse.z = RandomFloat( 0.5, 1.0 );
	VectorNormalize( impulse );
	IPhysicsObject *pVehiclePhysics = GetVehicleStuckTo()->VPhysicsGetObject();
	Vector vecForce = impulse * ImpulseScale( pVehiclePhysics->GetMass(), RandomFloat(150,250) );
	pVehiclePhysics->ApplyForceOffset( vecForce, GetAbsOrigin() );

	// Play sounds & effects
	EmitSound( "NPC_RollerMine.JoltVehicle" );

	// UNDONE: Good Zap effects
	/*
	CBeam *pBeam = CBeam::BeamCreate( "sprites/rollermine_shock.vmt", 4 );
	if ( pBeam )
	{
		pBeam->EntsInit( GetVehicleStuckTo(), this );
		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>( GetVehicleStuckTo() );
		if ( pAnimating )
		{
			int startAttach = pAnimating->LookupAttachment("beam_damage" );
			pBeam->SetStartAttachment( startAttach );
		}
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( 8 );
		pBeam->SetEndWidth( 8 );
		pBeam->SetBrightness( 255 );
		pBeam->SetColor( 255, 255, 255 );
		pBeam->LiveForTime( 0.5f );
		pBeam->RelinkBeam();
		pBeam->SetNoise( 30 );
	}
	*/

	ShockTarget( GetVehicleStuckTo() );

	// Jolt again soon
	g_EventQueue.AddEvent( this, "JoltVehicle", RandomFloat(3,6), NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputTurnOn( inputdata_t &inputdata )
{
	m_RollerController.On();
	m_bTurnedOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputTurnOff( inputdata_t &inputdata )
{
	m_RollerController.Off();
	m_bTurnedOn = false;
	StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::InputPowerdown( inputdata_t &inputdata )
{
	m_bPowerDown = true;
	m_flPowerDownTime = gpGlobals->curtime + RandomFloat( 0.1, 0.5 );
	m_flPowerDownDetonateTime = m_flPowerDownTime + RandomFloat( 1.5, 4.0 );

	ClearSchedule( "Received power down input" );
}

//-----------------------------------------------------------------------------
// Purpose: If we were stuck to a vehicle, remove ourselves
//-----------------------------------------------------------------------------
void CNPC_RollerMine::UnstickFromVehicle( void )
{
	if ( m_pConstraint )
	{
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}

	// Cancel any pending jolt events
	g_EventQueue.CancelEventOn( this, "JoltVehicle" );

	m_hVehicleStuckTo = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_RollerMine::GetVehicleStuckTo( void )
{
	if ( !m_pConstraint )
		return NULL;

	return m_hVehicleStuckTo;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	// Are we just being punted?
	if ( reason == PUNTED_BY_CANNON )
	{
		// Be stunned
		m_flActiveTime = gpGlobals->curtime + GetStunDelay();
		return;
	}

	//Stop turning
	m_RollerController.m_vecAngular = vec3_origin;

	UnstickFromVehicle();

	m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );
	m_bHeld = true;
	m_RollerController.Off();
	EmitSound( "NPC_RollerMine.Held" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	m_bHeld = false;
	m_flActiveTime = gpGlobals->curtime + GetStunDelay();
	m_RollerController.On();
	
	// explode on contact if launched from the physgun
	if ( Reason == LAUNCHED_BY_CANNON )
	{
		if ( m_bIsOpen )
		{
			//m_bIsPrimed = true;
			SetTouch( &CNPC_RollerMine::SpikeTouch );
			// enable world/prop touch too
			VPhysicsGetObject()->SetCallbackFlags( VPhysicsGetObject()->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH|CALLBACK_GLOBAL_TOUCH_STATIC );
		}
		EmitSound( "NPC_RollerMine.Tossed" );
	}

	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : float
//-----------------------------------------------------------------------------
int CNPC_RollerMine::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !(info.GetDamageType() & DMG_BURN) )
	{
		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			AngularImpulse	angVel;
			angVel.Random( -400.0f, 400.0f );
			VPhysicsGetObject()->AddVelocity( NULL, &angVel );
			m_RollerController.m_vecAngular *= 0.8f;

			VPhysicsTakeDamage( info );
		}
		SetCondition( COND_LIGHT_DAMAGE );
	}

	if ( info.GetDamageType() & (DMG_BURN|DMG_BLAST) )
	{
		if ( info.GetAttacker() && info.GetAttacker()->m_iClassname != m_iClassname )
		{
			SetThink( &CNPC_RollerMine::PreDetonate );
			SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1f, 0.5f ) );
		}
		else
		{
			// dazed
			m_RollerController.m_vecAngular.Init();
			m_flActiveTime = gpGlobals->curtime + GetStunDelay();
			Hop( 300 );
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the roller to hop into the air
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Hop( float height )
{
	if ( m_flNextHop > gpGlobals->curtime )
		return;

	if ( GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysObj = VPhysicsGetObject();
		pPhysObj->ApplyForceCenter( Vector(0,0,1) * height * pPhysObj->GetMass() );
		
		AngularImpulse	angVel;
		angVel.Random( -400.0f, 400.0f );
		pPhysObj->AddVelocity( NULL, &angVel );

		m_flNextHop = gpGlobals->curtime + ROLLERMINE_HOP_DELAY;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes warning noise before actual explosion occurs
//-----------------------------------------------------------------------------
void CNPC_RollerMine::PreDetonate( void )
{
	Hop( 300 );

	SetTouch( NULL );
	SetThink( &CNPC_RollerMine::Explode );
	SetNextThink( gpGlobals->curtime + 0.5f );

	EmitSound( "NPC_RollerMine.Hurt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::Explode( void )
{
	m_takedamage = DAMAGE_NO;

	//FIXME: Hack to make thrown mines more deadly and fun
	float expDamage = m_bIsPrimed ? 100 : 25;

	//If we've been hacked and we're blowing up cause we've been shut down then do moderate damage.
	if ( m_bPowerDown == true )
	{
		expDamage = 50;
	}

	// Underwater explosion?
	if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		CEffectData	data;
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = expDamage;
		data.m_flScale = 128;
		data.m_fFlags = ( SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE );
		DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		ExplosionCreate( WorldSpaceCenter(), GetLocalAngles(), this, expDamage, 128, true );
	}

	CTakeDamageInfo	info( this, this, 1, DMG_GENERIC );
	Event_Killed( info );

	// Remove myself a frame from now to avoid doing it in the middle of running AI
	SetThink( &CNPC_RollerMine::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

const float MAX_ROLLING_SPEED = 720;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_RollerMine::RollingSpeed()
{
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( !m_hVehicleStuckTo && !m_bHeld && pPhysics && !pPhysics->IsAsleep() )
	{
		AngularImpulse angVel;
		pPhysics->GetVelocity( NULL, &angVel );
		float rollingSpeed = angVel.Length() - 90;
		rollingSpeed = clamp( rollingSpeed, 1, MAX_ROLLING_SPEED );
		rollingSpeed *= (1/MAX_ROLLING_SPEED);
		return rollingSpeed;
	}
	return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_RollerMine::GetStunDelay()
{
	if( m_bHackedByAlyx )
	{
		return 0.1f;
	}
	else
	{
		return sk_rollermine_stun_delay.GetFloat();
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've been dropped by a dropship. Embed in the ground if we land on it.
//-----------------------------------------------------------------------------
void CNPC_RollerMine::EmbedOnGroundImpact()
{
	m_bEmbedOnGroundImpact = true;

	SetTouch( &CNPC_RollerMine::EmbedTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::PrescheduleThink()
{
	// Are we underwater?
	if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		// As soon as we're far enough underwater, detonate
		Vector vecAboveMe = GetAbsOrigin() + Vector(0,0,64);
		if ( UTIL_PointContents( vecAboveMe ) & MASK_WATER )
		{
			Explode();
			return;
		}
	}

	UpdateRollingSound();
	UpdatePingSound();
	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::UpdateRollingSound()
{
	if ( m_rollingSoundState == ROLL_SOUND_NOT_READY )
		return;

	rollingsoundstate_t soundState = ROLL_SOUND_OFF;
	float rollingSpeed = RollingSpeed();
	if ( rollingSpeed > 0 )
	{
		soundState = m_bIsOpen ? ROLL_SOUND_OPEN : ROLL_SOUND_CLOSED;
	}


	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CSoundParameters params;
	switch( soundState )
	{
	case ROLL_SOUND_CLOSED:
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.Roll", params, NULL );
		break;
	case ROLL_SOUND_OPEN:
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.RollWithSpikes", params, NULL );
		break;

	case ROLL_SOUND_OFF:
		// no sound
		break;
	}

	// start the new sound playing if necessary
	if ( m_rollingSoundState != soundState )
	{
		StopRollingSound();

		m_rollingSoundState = soundState;

		if ( m_rollingSoundState == ROLL_SOUND_OFF )
			return;

		CPASAttenuationFilter filter( this );
		m_pRollSound = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
		controller.Play( m_pRollSound, params.volume, params.pitch );
		m_rollingSoundState = soundState;
	}

	if ( m_pRollSound )
	{
		// for tuning
		//DevMsg("SOUND: %s, VOL: %.1f\n", m_rollingSoundState == ROLL_SOUND_CLOSED ? "CLOSED" : "OPEN ", rollingSpeed );
		controller.SoundChangePitch( m_pRollSound, params.pitchlow + (params.pitchhigh - params.pitchlow) * rollingSpeed, 0.1 );
		controller.SoundChangeVolume( m_pRollSound, params.volume * rollingSpeed, 0.1 );
	}
}


void CNPC_RollerMine::StopRollingSound()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRollSound );
	m_pRollSound = NULL;
}

void CNPC_RollerMine::UpdatePingSound()
{
	float pingSpeed = 0;
	if ( m_bIsOpen && !IsShocking() && !m_bHeld )
	{
		CBaseEntity *pEnemy = GetEnemy();
		if ( pEnemy )
		{
			pingSpeed = EnemyDistance( pEnemy );
			pingSpeed = clamp( pingSpeed, 1, ROLLERMINE_OPEN_THRESHOLD );
			pingSpeed *= (1.0f/ROLLERMINE_OPEN_THRESHOLD);
		}
	}

	if ( pingSpeed > 0 )
	{
		pingSpeed = 1-pingSpeed;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CSoundParameters params;
		CBaseEntity::GetParametersForSound( "NPC_RollerMine.Ping", params, NULL );
		if ( !m_pPingSound )
		{
			CPASAttenuationFilter filter( this );
			m_pPingSound = controller.SoundCreate( filter, entindex(), params.channel, params.soundname, params.soundlevel );
			controller.Play( m_pPingSound, params.volume, 101 );
		}

		controller.SoundChangePitch( m_pPingSound, params.pitchlow + (params.pitchhigh - params.pitchlow) * pingSpeed, 0.1 );
		controller.SoundChangeVolume( m_pPingSound, params.volume, 0.1 );
		//DevMsg("PING: %.1f\n", pingSpeed );

	}
	else
	{
		StopPingSound();
	}
}


void CNPC_RollerMine::StopPingSound()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pPingSound );
	m_pPingSound = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::StopLoopingSounds( void )
{
	StopRollingSound();
	StopPingSound();
	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::IsValidEnemy( CBaseEntity *pEnemy )
{
	// If the enemy's over the vehicle detection range, and it's not a player in a vehicle, ignore it
	if ( pEnemy )
	{
		float flDistance = GetAbsOrigin().DistTo( pEnemy->GetAbsOrigin() );
		if ( flDistance >= m_flSeeVehiclesOnlyBeyond )
		{
			// Handle vehicles
			CBaseCombatCharacter *pCCEnemy = pEnemy->MyCombatCharacterPointer();
			if ( pCCEnemy != NULL && pCCEnemy->IsInAVehicle() )
			{
				// If we're buried, we only care when they're heading directly towards us
				if ( m_bBuried )
					return ( VehicleHeading( pCCEnemy->GetVehicle()->GetVehicleEnt() ) > DOT_20DEGREE );

				// If we're not buried, chase him as long as he's not heading away from us
				return ( VehicleHeading( pCCEnemy->GetVehicleEntity() ) > 0 );
			}

			return false;
		}

		// Never pick something I fear
		if ( IRelationType( pEnemy ) == D_FR )
			return false;

		// Don't attack flying things.
		if ( pEnemy->GetMoveType() == MOVETYPE_FLY )
			return false;
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_RollerMine::EnemyInVehicle( void )
{
	// Clearly the enemy is not...
	if ( GetEnemy() == NULL )
		return false;

	// If the target is in a vehicle, let the convar choose
	CBaseCombatCharacter *pCCEnemy = GetEnemy()->MyCombatCharacterPointer();
	if ( pCCEnemy != NULL && pCCEnemy->IsInAVehicle() )
		return ( sk_rollermine_vehicle_intercept.GetBool() );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_RollerMine::VehicleHeading( CBaseEntity *pVehicle )
{
	Vector vecVelocity = pVehicle->GetSmoothedVelocity();
	float flSpeed = VectorNormalize( vecVelocity );
	Vector vecToMine = GetAbsOrigin() - pVehicle->GetAbsOrigin();
	VectorNormalize( vecToMine );

	// If it's not moving, consider it moving towards us, but not directly
	// This will enable already active rollers to chase the vehicle if it's stationary.
	if ( flSpeed < 10 )
		return 0.1;

	return DotProduct( vecVelocity, vecToMine );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecDir - 
//			*ptr - 
//-----------------------------------------------------------------------------
void CNPC_RollerMine::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( info.GetDamageType() & (DMG_BULLET | DMG_CLUB) )
	{
		CTakeDamageInfo newInfo( info );

		// If we're stuck to the car, increase it even more
		if ( GetVehicleStuckTo() )
		{
			newInfo.SetDamageForce( info.GetDamageForce() * 40 );
		}
		else
		{
			newInfo.SetDamageForce( info.GetDamageForce() * 20 );
		}

		BaseClass::TraceAttack( newInfo, vecDir, ptr, pAccumulator );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_rollermine, CNPC_RollerMine )

	//Tasks
	DECLARE_TASK( TASK_ROLLERMINE_CHARGE_ENEMY )
	DECLARE_TASK( TASK_ROLLERMINE_BURIED_WAIT )
	DECLARE_TASK( TASK_ROLLERMINE_UNBURROW )
	DECLARE_TASK( TASK_ROLLERMINE_GET_PATH_TO_FLEE )
	DECLARE_TASK( TASK_ROLLERMINE_NUDGE_TOWARDS_NODES )
	DECLARE_TASK( TASK_ROLLERMINE_RETURN_TO_PLAYER )
	DECLARE_TASK( TASK_ROLLERMINE_POWERDOWN )

	//Schedules

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_BURIED_WAIT,

		"	Tasks"
		"		TASK_ROLLERMINE_BURIED_WAIT		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_BURIED_UNBURROW,

		"	Tasks"
		"		TASK_ROLLERMINE_UNBURROW		0"
		"	"
		"	Interrupts"
	)
	
	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ROLLERMINE_CHARGE_ENEMY	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_TOO_FAR"
	)
	
	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ROLLERMINE_RANGE_ATTACK1"
		"		TASK_SET_TOLERANCE_DISTANCE		24"
		"		TASK_GET_PATH_TO_ENEMY			0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_ENEMY_TOO_FAR"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_TASK_FAILED"
		"		COND_SEE_FEAR"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_FLEE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_IDLE_STAND"
		"		TASK_ROLLERMINE_GET_PATH_TO_FLEE	300"
		"		TASK_RUN_PATH						0"
		"		TASK_STOP_MOVING					0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_TASK_FAILED"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_ALERT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_REASONABLE		0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT					2"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
	"		COND_SMELL"
	"		COND_HEAR_COMBAT"		// sound flags
	"		COND_HEAR_WORLD"
	"		COND_HEAR_PLAYER"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_BULLET_IMPACT"
	"		COND_IDLE_INTERRUPT"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ROLLERMINE_NUDGE_TOWARDS_NODES,

	"	Tasks"
	"		TASK_ROLLERMINE_NUDGE_TOWARDS_NODES		0"
	"		TASK_WAIT								1.5"
	""
	"	Interrupts"
	""
	)

	DEFINE_SCHEDULE
	(
		SCHED_ROLLERMINE_PATH_TO_PLAYER,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ROLLERMINE_ALERT_STAND"
		"		TASK_SET_TOLERANCE_DISTANCE		200"
		"		TASK_GET_PATH_TO_PLAYER			0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_SMELL"
		"		COND_HEAR_COMBAT"		// sound flags
		"		COND_HEAR_WORLD"
		"		COND_HEAR_PLAYER"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_IDLE_INTERRUPT"
		"		COND_SEE_PLAYER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ROLLERMINE_ROLL_TO_PLAYER,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ROLLERMINE_ALERT_STAND"
		"		TASK_SET_TOLERANCE_DISTANCE			200"
		"		TASK_ROLLERMINE_RETURN_TO_PLAYER	0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_SMELL"
		"		COND_HEAR_COMBAT"		// sound flags
		"		COND_HEAR_WORLD"
		"		COND_HEAR_PLAYER"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_IDLE_INTERRUPT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ROLLERMINE_POWERDOWN,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_ROLLERMINE_POWERDOWN	0"
		""
		"	Interrupts"
		""
	);

AI_END_CUSTOM_NPC()
