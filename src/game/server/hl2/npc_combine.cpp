//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_route.h"
#include "ai_interactions.h"
#include "ai_tacticalservices.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "npc_combine.h"
#include "activitylist.h"
#include "player.h"
#include "basecombatweapon.h"
#include "basegrenade_shared.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "grenade_frag.h"
#include "ndebugoverlay.h"
#include "weapon_physcannon.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "npc_headcrab.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_fCombineQuestion;				// true if an idle grunt asked a question. Cleared when someone answers. YUCK old global from grunt code

#define COMBINE_SKIN_DEFAULT		0
#define COMBINE_SKIN_SHOTGUNNER		1


#define COMBINE_GRENADE_THROW_SPEED 650
#define COMBINE_GRENADE_TIMER		3.5
#define COMBINE_GRENADE_FLUSH_TIME	3.0		// Don't try to flush an enemy who has been out of sight for longer than this.
#define COMBINE_GRENADE_FLUSH_DIST	256.0	// Don't try to flush an enemy who has moved farther than this distance from the last place I saw him.

#define COMBINE_LIMP_HEALTH				20
#define	COMBINE_MIN_GRENADE_CLEAR_DIST	250

#define COMBINE_EYE_STANDING_POSITION	Vector( 0, 0, 66 )
#define COMBINE_GUN_STANDING_POSITION	Vector( 0, 0, 57 )
#define COMBINE_EYE_CROUCHING_POSITION	Vector( 0, 0, 40 )
#define COMBINE_GUN_CROUCHING_POSITION	Vector( 0, 0, 36 )
#define COMBINE_SHOTGUN_STANDING_POSITION	Vector( 0, 0, 36 )
#define COMBINE_SHOTGUN_CROUCHING_POSITION	Vector( 0, 0, 36 )
#define COMBINE_MIN_CROUCH_DISTANCE		256.0

//-----------------------------------------------------------------------------
// Static stuff local to this file.
//-----------------------------------------------------------------------------
// This is the index to the name of the shotgun's classname in the string pool
// so that we can get away with an integer compare rather than a string compare.
string_t	s_iszShotgunClassname;

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionCombineBash		= 0; // melee bash attack

//=========================================================
// Combines's Anim Events Go Here
//=========================================================
#define COMBINE_AE_RELOAD			( 2 )
#define COMBINE_AE_KICK				( 3 )
#define COMBINE_AE_AIM				( 4 )
#define COMBINE_AE_GREN_TOSS		( 7 )
#define COMBINE_AE_GREN_LAUNCH		( 8 )
#define COMBINE_AE_GREN_DROP		( 9 )
#define COMBINE_AE_CAUGHT_ENEMY		( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.

int COMBINE_AE_BEGIN_ALTFIRE;
int COMBINE_AE_ALTFIRE;

//=========================================================
// Combine activities
//=========================================================
//Activity ACT_COMBINE_STANDING_SMG1;
//Activity ACT_COMBINE_CROUCHING_SMG1;
//Activity ACT_COMBINE_STANDING_AR2;
//Activity ACT_COMBINE_CROUCHING_AR2;
//Activity ACT_COMBINE_WALKING_AR2;
//Activity ACT_COMBINE_STANDING_SHOTGUN;
//Activity ACT_COMBINE_CROUCHING_SHOTGUN;
Activity ACT_COMBINE_THROW_GRENADE;
Activity ACT_COMBINE_LAUNCH_GRENADE;
Activity ACT_COMBINE_BUGBAIT;
Activity ACT_COMBINE_AR2_ALTFIRE;
Activity ACT_WALK_EASY;
Activity ACT_WALK_MARCH;

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{	
	SQUAD_SLOT_GRENADE1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_GRENADE2,
	SQUAD_SLOT_ATTACK_OCCLUDER,
	SQUAD_SLOT_OVERWATCH,
};

enum TacticalVariant_T
{
	TACTICAL_VARIANT_DEFAULT = 0,
	TACTICAL_VARIANT_PRESSURE_ENEMY,				// Always try to close in on the player.
	TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE,	// Act like VARIANT_PRESSURE_ENEMY, but go to VARIANT_DEFAULT once within 30 feet
};

enum PathfindingVariant_T
{
	PATHFINDING_VARIANT_DEFAULT = 0,
};


#define bits_MEMORY_PAIN_LIGHT_SOUND		bits_MEMORY_CUSTOM1
#define bits_MEMORY_PAIN_HEAVY_SOUND		bits_MEMORY_CUSTOM2
#define bits_MEMORY_PLAYER_HURT				bits_MEMORY_CUSTOM3

LINK_ENTITY_TO_CLASS( npc_combine, CNPC_Combine );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Combine )

DEFINE_FIELD( m_nKickDamage, FIELD_INTEGER ),
DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),
DEFINE_FIELD( m_hForcedGrenadeTarget, FIELD_EHANDLE ),
DEFINE_FIELD( m_bShouldPatrol, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bFirstEncounter, FIELD_BOOLEAN ),
DEFINE_FIELD( m_flNextPainSoundTime, FIELD_TIME ),
DEFINE_FIELD( m_flNextAlertSoundTime, FIELD_TIME ),
DEFINE_FIELD( m_flNextGrenadeCheck, FIELD_TIME ),
DEFINE_FIELD( m_flNextLostSoundTime, FIELD_TIME ),
DEFINE_FIELD( m_flAlertPatrolTime, FIELD_TIME ),
DEFINE_FIELD( m_flNextAltFireTime, FIELD_TIME ),
DEFINE_FIELD( m_nShots, FIELD_INTEGER ),
DEFINE_FIELD( m_flShotDelay, FIELD_FLOAT ),
DEFINE_FIELD( m_flStopMoveShootTime, FIELD_TIME ),
DEFINE_KEYFIELD( m_iNumGrenades, FIELD_INTEGER, "NumGrenades" ),
DEFINE_EMBEDDED( m_Sentences ),

//							m_AssaultBehavior (auto saved by AI)
//							m_StandoffBehavior (auto saved by AI)
//							m_FollowBehavior (auto saved by AI)
//							m_FuncTankBehavior (auto saved by AI)
//							m_RappelBehavior (auto saved by AI)
//							m_ActBusyBehavior (auto saved by AI)

DEFINE_INPUTFUNC( FIELD_VOID,	"LookOff",	InputLookOff ),
DEFINE_INPUTFUNC( FIELD_VOID,	"LookOn",	InputLookOn ),

DEFINE_INPUTFUNC( FIELD_VOID,	"StartPatrolling",	InputStartPatrolling ),
DEFINE_INPUTFUNC( FIELD_VOID,	"StopPatrolling",	InputStopPatrolling ),

DEFINE_INPUTFUNC( FIELD_STRING,	"Assault", InputAssault ),

DEFINE_INPUTFUNC( FIELD_VOID,	"HitByBugbait",		InputHitByBugbait ),

DEFINE_INPUTFUNC( FIELD_STRING,	"ThrowGrenadeAtTarget",	InputThrowGrenadeAtTarget ),

DEFINE_FIELD( m_iLastAnimEventHandled, FIELD_INTEGER ),
DEFINE_FIELD( m_fIsElite, FIELD_BOOLEAN ),
DEFINE_FIELD( m_vecAltFireTarget, FIELD_VECTOR ),

DEFINE_KEYFIELD( m_iTacticalVariant, FIELD_INTEGER, "tacticalvariant" ),
DEFINE_KEYFIELD( m_iPathfindingVariant, FIELD_INTEGER, "pathfindingvariant" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Constructor.
//------------------------------------------------------------------------------
CNPC_Combine::CNPC_Combine()
{
	m_vecTossVelocity = vec3_origin;
}


//-----------------------------------------------------------------------------
// Create components
//-----------------------------------------------------------------------------
bool CNPC_Combine::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_Sentences.Init( this, "NPC_Combine.SentenceParameters" );
	return true;
}


//------------------------------------------------------------------------------
// Purpose: Don't look, only get info from squad.
//------------------------------------------------------------------------------
void CNPC_Combine::InputLookOff( inputdata_t &inputdata )
{
	m_spawnflags |= SF_COMBINE_NO_LOOK;
}

//------------------------------------------------------------------------------
// Purpose: Enable looking.
//------------------------------------------------------------------------------
void CNPC_Combine::InputLookOn( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_COMBINE_NO_LOOK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine::InputStartPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine::InputStopPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine::InputAssault( inputdata_t &inputdata )
{
	m_AssaultBehavior.SetParameters( AllocPooledString(inputdata.value.String()), CUE_DONT_WAIT, RALLY_POINT_SELECT_DEFAULT );
}

//-----------------------------------------------------------------------------
// We were hit by bugbait
//-----------------------------------------------------------------------------
void CNPC_Combine::InputHitByBugbait( inputdata_t &inputdata )
{
	SetCondition( COND_COMBINE_HIT_BY_BUGBAIT );
}

//-----------------------------------------------------------------------------
// Purpose: Force the combine soldier to throw a grenade at the target
//			If I'm a combine elite, fire my combine ball at the target instead.
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Combine::InputThrowGrenadeAtTarget( inputdata_t &inputdata )
{
	// Ignore if we're inside a scripted sequence
	if ( m_NPCState == NPC_STATE_SCRIPT && m_hCine )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		DevMsg("%s (%s) received ThrowGrenadeAtTarget input, but couldn't find target entity '%s'\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	m_hForcedGrenadeTarget = pEntity;
	m_flNextGrenadeCheck = 0;

	ClearSchedule( "Told to throw grenade via input" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Combine::Precache()
{
	PrecacheModel("models/Weapons/w_grenade.mdl");
	UTIL_PrecacheOther( "npc_handgrenade" );

	PrecacheScriptSound( "NPC_Combine.GrenadeLaunch" );
	PrecacheScriptSound( "NPC_Combine.WeaponBash" );
	PrecacheScriptSound( "Weapon_CombineGuard.Special1" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::Activate()
{
	s_iszShotgunClassname = FindPooledString( "weapon_shotgun" );
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Combine::Spawn( void )
{
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_flFieldOfView			= -0.2;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState				= NPC_STATE_NONE;
	m_flNextGrenadeCheck	= gpGlobals->curtime + 1;
	m_flNextPainSoundTime	= 0;
	m_flNextAlertSoundTime	= 0;
	m_bShouldPatrol			= false;

	//	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB);
	// JAY: Disabled jump for now - hard to compare to HL1
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND );

	CapabilitiesAdd( bits_CAP_AIM_GUN );

	// Innate range attack for grenade
	// CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK2 );

	// Innate range attack for kicking
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1 );

	// Can be in a squad
	CapabilitiesAdd( bits_CAP_SQUAD);
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );

	CapabilitiesAdd( bits_CAP_DUCK );				// In reloading and cover

	CapabilitiesAdd( bits_CAP_NO_HIT_SQUADMATES );

	m_bFirstEncounter	= true;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_flStopMoveShootTime = FLT_MAX; // Move and shoot defaults on.
	m_MoveAndShootOverlay.SetInitialDelay( 0.75 ); // But with a bit of a delay.

	m_flNextLostSoundTime		= 0;
	m_flAlertPatrolTime			= 0;

	m_flNextAltFireTime = gpGlobals->curtime;

	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Combine::CreateBehaviors()
{
	AddBehavior( &m_RappelBehavior );
	AddBehavior( &m_ActBusyBehavior );
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_FuncTankBehavior );

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::PostNPCInit()
{
	if( IsElite() )
	{
		// Give a warning if a Combine Soldier is equipped with anything other than
		// an AR2. 
		if( !GetActiveWeapon() || !FClassnameIs( GetActiveWeapon(), "weapon_ar2" ) )
		{
			DevWarning("**Combine Elite Soldier MUST be equipped with AR2\n");
		}
	}

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::GatherConditions()
{
	BaseClass::GatherConditions();

	ClearCondition( COND_COMBINE_ATTACK_SLOT_AVAILABLE );

	if( GetState() == NPC_STATE_COMBAT )
	{
		if( IsCurSchedule( SCHED_COMBINE_WAIT_IN_COVER, false ) )
		{
			// Soldiers that are standing around doing nothing poll for attack slots so
			// that they can respond quickly when one comes available. If they can 
			// occupy a vacant attack slot, they do so. This holds the slot until their
			// schedule breaks and schedule selection runs again, essentially reserving this
			// slot. If they do not select an attack schedule, then they'll release the slot.
			if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				SetCondition( COND_COMBINE_ATTACK_SLOT_AVAILABLE );
			}
		}

		if( IsUsingTacticalVariant(TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE) )
		{
			if( GetEnemy() != NULL && !HasCondition(COND_ENEMY_OCCLUDED) )
			{
				// Now we're close to our enemy, stop using the tactical variant.
				if( GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin()) < Square(30.0f * 12.0f) )
					m_iTacticalVariant = TACTICAL_VARIANT_DEFAULT;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	// Speak any queued sentences
	m_Sentences.UpdateSentenceQueue();

	if ( IsOnFire() )
	{
		SetCondition( COND_COMBINE_ON_FIRE );
	}
	else
	{
		ClearCondition( COND_COMBINE_ON_FIRE );
	}

	extern ConVar ai_debug_shoot_positions;
	if ( ai_debug_shoot_positions.GetBool() )
		NDebugOverlay::Cross3D( EyePosition(), 16, 0, 255, 0, false, 0.1 );

	if( gpGlobals->curtime >= m_flStopMoveShootTime )
	{
		// Time to stop move and shoot and start facing the way I'm running.
		// This makes the combine look attentive when disengaging, but prevents
		// them from always running around facing you.
		//
		// Only do this if it won't be immediately shut off again.
		if( GetNavigator()->GetPathTimeToGoal() > 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 5.0f );
			m_flStopMoveShootTime = FLT_MAX;
		}
	}

	if( m_flGroundSpeed > 0 && GetState() == NPC_STATE_COMBAT && m_MoveAndShootOverlay.IsSuspended() )
	{
		// Return to move and shoot when near my goal so that I 'tuck into' the location facing my enemy.
		if( GetNavigator()->GetPathTimeToGoal() <= 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 0 );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::DelayAltFireAttack( float flDelay )
{
	float flNextAltFire = gpGlobals->curtime + flDelay;

	if( flNextAltFire > m_flNextAltFireTime )
	{
		// Don't let this delay order preempt a previous request to wait longer.
		m_flNextAltFireTime = flNextAltFire;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::DelaySquadAltFireAttack( float flDelay )
{
	// Make sure to delay my own alt-fire attack.
	DelayAltFireAttack( flDelay );

	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember( &iter ) : NULL;
	while ( pSquadmate )
	{
		CNPC_Combine *pCombine = dynamic_cast<CNPC_Combine*>(pSquadmate);

		if( pCombine && pCombine->IsElite() )
		{
			pCombine->DelayAltFireAttack( flDelay );
		}

		pSquadmate = m_pSquad->GetNextMember( &iter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: degrees to turn in 0.1 seconds
//-----------------------------------------------------------------------------
float CNPC_Combine::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 45;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
		return 25;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		return 35;
	default:
		return 35;
		break;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CNPC_Combine::ShouldMoveAndShoot()
{
	// Set this timer so that gpGlobals->curtime can't catch up to it. 
	// Essentially, we're saying that we're not going to interfere with 
	// what the AI wants to do with move and shoot. 
	//
	// If any code below changes this timer, the code is saying 
	// "It's OK to move and shoot until gpGlobals->curtime == m_flStopMoveShootTime"
	m_flStopMoveShootTime = FLT_MAX;

	if( IsCurSchedule( SCHED_COMBINE_HIDE_AND_RELOAD, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	if( IsCurSchedule( SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	if( IsCurSchedule( SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND, false ) )
		return false;

	if( HasCondition( COND_NO_PRIMARY_AMMO, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( m_pSquad && IsCurSchedule( SCHED_COMBINE_TAKE_COVER1, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	return BaseClass::ShouldMoveAndShoot();
}

//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Combine::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_Combine::Classify ( void )
{
	return CLASS_COMBINE;
}


//-----------------------------------------------------------------------------
// Continuous movement tasks
//-----------------------------------------------------------------------------
bool CNPC_Combine::IsCurTaskContinuousMove()
{
	const Task_t* pTask = GetTask();
	if ( pTask && (pTask->iTask == TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY) )
		return true;

	return BaseClass::IsCurTaskContinuousMove();
}


//-----------------------------------------------------------------------------
// Chase the enemy, updating the target position as the player moves
//-----------------------------------------------------------------------------
void CNPC_Combine::StartTaskChaseEnemyContinuously( const Task_t *pTask )
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		TaskFail( FAIL_NO_ENEMY );
		return;
	}

	// We're done once we get close enough
	if ( WorldSpaceCenter().DistToSqr( pEnemy->WorldSpaceCenter() ) <= pTask->flTaskData * pTask->flTaskData )
	{
		TaskComplete();
		return;
	}

	// TASK_GET_PATH_TO_ENEMY
	if ( IsUnreachable( pEnemy ) )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	if ( !GetNavigator()->SetGoal( GOALTYPE_ENEMY, AIN_NO_PATH_TASK_FAIL ) )
	{
		// no way to get there =( 
		DevWarning( 2, "GetPathToEnemy failed!!\n" );
		RememberUnreachable( pEnemy );
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	// NOTE: This is TaskRunPath here.
	if ( TranslateActivity( ACT_RUN ) != ACT_INVALID )
	{
		GetNavigator()->SetMovementActivity( ACT_RUN );
	}
	else
	{
		GetNavigator()->SetMovementActivity(ACT_WALK);
	}

	// Cover is void once I move
	Forget( bits_MEMORY_INCOVER );

	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
		return;
	}

	// No shooting delay when in this mode
	m_MoveAndShootOverlay.SetInitialDelay( 0.0 );

	if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}

	// set that we're probably going to stop before the goal
	GetNavigator()->SetArrivalDistance( pTask->flTaskData );
	m_vSavePosition = GetEnemy()->WorldSpaceCenter();
}

void CNPC_Combine::RunTaskChaseEnemyContinuously( const Task_t *pTask )
{
	if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}

	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		TaskFail( FAIL_NO_ENEMY );
		return;
	}

	// We're done once we get close enough
	if ( WorldSpaceCenter().DistToSqr( pEnemy->WorldSpaceCenter() ) <= pTask->flTaskData * pTask->flTaskData )
	{
		GetNavigator()->StopMoving();
		TaskComplete();
		return;
	}

	// Recompute path if the enemy has moved too much
	if ( m_vSavePosition.DistToSqr( pEnemy->WorldSpaceCenter() ) < (pTask->flTaskData * pTask->flTaskData) )
		return;

	if ( IsUnreachable( pEnemy ) )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	if ( !GetNavigator()->RefindPathToGoal() )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	m_vSavePosition = pEnemy->WorldSpaceCenter();
}


//=========================================================
// start task
//=========================================================
void CNPC_Combine::StartTask( const Task_t *pTask )
{
	// NOTE: This reset is required because we change it in TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY
	m_MoveAndShootOverlay.SetInitialDelay( 0.75 );

	switch ( pTask->iTask )
	{
	case TASK_COMBINE_SET_STANDING:
		{
			if ( pTask->flTaskData == 1.0f)
			{
				Stand();
			}
			else
			{
				Crouch();
			}
			TaskComplete();
		}
		break;

	case TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY:
		StartTaskChaseEnemyContinuously( pTask );
		break;

	case TASK_COMBINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET:
		SetIdealActivity( (Activity)(int)pTask->flTaskData );
		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecAltFireTarget, AI_KEEP_YAW_SPEED );
		break;

	case TASK_COMBINE_SIGNAL_BEST_SOUND:
		if( IsInSquad() && GetSquad()->NumMembers() > 1 )
		{
			CBasePlayer *pPlayer = AI_GetSinglePlayer();

			if( pPlayer && OccupyStrategySlot( SQUAD_SLOT_EXCLUSIVE_HANDSIGN ) && pPlayer->FInViewCone( this ) )
			{
				CSound *pSound;
				pSound = GetBestSound();

				Assert( pSound != NULL );

				if ( pSound )
				{
					Vector right, tosound;

					GetVectors( NULL, &right, NULL );

					tosound = pSound->GetSoundReactOrigin() - GetAbsOrigin();
					VectorNormalize( tosound);

					tosound.z = 0;
					right.z = 0;

					if( DotProduct( right, tosound ) > 0 )
					{
						// Right
						SetIdealActivity( ACT_SIGNAL_RIGHT );
					}
					else
					{
						// Left
						SetIdealActivity( ACT_SIGNAL_LEFT );
					}

					break;
				}
			}
		}

		// Otherwise, just skip it.
		TaskComplete();
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			// If Primary Attack
			if ((int)pTask->flTaskData == 1)
			{
				// -----------------------------------------------------------
				// If enemy isn't facing me and I haven't attacked in a while
				// annouce my attack before I start wailing away
				// -----------------------------------------------------------
				CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

				if	(pBCC && pBCC->IsPlayer() && (!pBCC->FInViewCone ( this )) &&
					(gpGlobals->curtime - m_flLastAttackTime > 3.0) )
				{
					m_flLastAttackTime = gpGlobals->curtime;

					m_Sentences.Speak( "COMBINE_ANNOUNCE", SENTENCE_PRIORITY_HIGH );

					// Wait two seconds
					SetWait( 2.0 );

					if ( !IsCrouching() )
					{
						SetActivity(ACT_IDLE);
					}
					else
					{
						SetActivity(ACT_COWER); // This is really crouch idle
					}
				}
				// -------------------------------------------------------------
				//  Otherwise move on
				// -------------------------------------------------------------
				else
				{
					TaskComplete();
				}
			}
			else
			{
				m_Sentences.Speak( "COMBINE_THROW_GRENADE", SENTENCE_PRIORITY_MEDIUM );
				SetActivity(ACT_IDLE);

				// Wait two seconds
				SetWait( 2.0 );
			}
			break;
		}	

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		BaseClass::StartTask( pTask );
		break;

	case TASK_COMBINE_FACE_TOSS_DIR:
		break;

	case TASK_COMBINE_GET_PATH_TO_FORCED_GREN_LOS:
		{
			if ( !m_hForcedGrenadeTarget )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			float flMaxRange = 2000;
			float flMinRange = 0;

			Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
			Vector vecEnemyEye = vecEnemy + m_hForcedGrenadeTarget->GetViewOffset();

			Vector posLos;
			bool found = false;

			if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
			{
				float dist = ( posLos - vecEnemyEye ).Length();
				if ( dist < flMaxRange && dist > flMinRange )
					found = true;
			}

			if ( !found && GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, &posLos ) )
			{
				found = true;
			}

			if ( !found )
			{
				TaskFail( FAIL_NO_SHOOT );
			}
			else
			{
				// else drop into run task to offer an interrupt
				m_vInterruptSavePosition = posLos;
			}
		}
		break;

	case TASK_COMBINE_IGNORE_ATTACKS:
		// must be in a squad
		if (m_pSquad && m_pSquad->NumMembers() > 2)
		{
			// the enemy must be far enough away
			if (GetEnemy() && (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).Length() > 512.0 )
			{
				m_flNextAttack	= gpGlobals->curtime + pTask->flTaskData;
			}
		}
		TaskComplete( );
		break;

	case TASK_COMBINE_DEFER_SQUAD_GRENADES:
		{
			if ( m_pSquad )
			{
				// iterate my squad and stop everyone from throwing grenades for a little while.
				AISquadIter_t iter;

				CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember( &iter ) : NULL;
				while ( pSquadmate )
				{
					CNPC_Combine *pCombine = dynamic_cast<CNPC_Combine*>(pSquadmate);

					if( pCombine )
					{
						pCombine->m_flNextGrenadeCheck = gpGlobals->curtime + 5;
					}

					pSquadmate = m_pSquad->GetNextMember( &iter );
				}
			}

			TaskComplete();
			break;
		}

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		{
			if( pTask->iTask == TASK_FACE_ENEMY && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				TaskComplete();
				return;
			}

			BaseClass::StartTask( pTask );
			bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
			if (bIsFlying)
			{
				SetIdealActivity( ACT_GLIDE );
			}

		}
		break;

	case TASK_FIND_COVER_FROM_ENEMY:
		{
			if (GetHintGroup() == NULL_STRING)
			{
				CBaseEntity *pEntity = GetEnemy();

				// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
				// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
				if ( pEntity )
				{
					// NOTE: This is a good time to check to see if the player is hurt.
					// Have the combine notice this and call out
					if ( !HasMemory(bits_MEMORY_PLAYER_HURT) && pEntity->IsPlayer() && pEntity->GetHealth() <= 20 )
					{
						if ( m_pSquad )
						{
							m_pSquad->SquadRemember(bits_MEMORY_PLAYER_HURT);
						}

						m_Sentences.Speak( "COMBINE_PLAYERHIT", SENTENCE_PRIORITY_INVALID );
						JustMadeSound( SENTENCE_PRIORITY_HIGH );
					}
					if ( pEntity->MyNPCPointer() )
					{
						if ( !(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_WEAPON_RANGE_ATTACK1) && 
							!(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_INNATE_RANGE_ATTACK1) )
						{
							TaskComplete();
							return;
						}
					}
				}
			}
			BaseClass::StartTask( pTask );
		}
		break;
	case TASK_RANGE_ATTACK1:
		{
			m_nShots = GetActiveWeapon()->GetRandomBurst();
			m_flShotDelay = GetActiveWeapon()->GetFireRate();

			m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
			ResetIdealActivity( ACT_RANGE_ATTACK1 );
			m_flLastAttackTime = gpGlobals->curtime;
		}
		break;

	case TASK_COMBINE_DIE_INSTANTLY:
		{
			CTakeDamageInfo info;

			info.SetAttacker( this );
			info.SetInflictor( this );
			info.SetDamage( m_iHealth );
			info.SetDamageType( pTask->flTaskData );
			info.SetDamageForce( Vector( 0.1, 0.1, 0.1 ) );

			TakeDamage( info );

			TaskComplete();
		}
		break;

	default: 
		BaseClass:: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_Combine::RunTask( const Task_t *pTask )
{
	/*
	{
	CBaseEntity *pEnemy = GetEnemy();
	if (pEnemy)
	{
	NDebugOverlay::Line(Center(), pEnemy->Center(), 0,255,255, false, 0.1);
	}

	}
	*/

	/*
	if (m_iMySquadSlot != SQUAD_SLOT_NONE)
	{
	char text[64];
	Q_snprintf( text, strlen( text ), "%d", m_iMySquadSlot );

	NDebugOverlay::Text( Center() + Vector( 0, 0, 72 ), text, false, 0.1 );
	}
	*/

	switch ( pTask->iTask )
	{
	case TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY:
		RunTaskChaseEnemyContinuously( pTask );
		break;

	case TASK_COMBINE_SIGNAL_BEST_SOUND:
		AutoMovement( );
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			// Stop waiting if enemy facing me or lost enemy
			CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
			if	(!pBCC || pBCC->FInViewCone( this ))
			{
				TaskComplete();
			}

			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_COMBINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET:
		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecAltFireTarget, AI_KEEP_YAW_SPEED );

		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_COMBINE_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			GetMotor()->SetIdealYawToTargetAndUpdate( GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

			if ( FacingIdeal() )
			{
				TaskComplete( true );
			}
			break;
		}

	case TASK_COMBINE_GET_PATH_TO_FORCED_GREN_LOS:
		{
			if ( !m_hForcedGrenadeTarget )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			if ( GetTaskInterrupt() > 0 )
			{
				ClearTaskInterrupt();

				Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
				AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

				GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
				GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
			}
			else
			{
				TaskInterrupt();
			}
		}
		break;

	case TASK_RANGE_ATTACK1:
		{
			AutoMovement( );

			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
			}
			else
			{
				GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
			}

			if ( gpGlobals->curtime >= m_flNextAttack )
			{
				if ( IsActivityFinished() )
				{
					if (--m_nShots > 0)
					{
						// DevMsg("ACT_RANGE_ATTACK1\n");
						ResetIdealActivity( ACT_RANGE_ATTACK1 );
						m_flLastAttackTime = gpGlobals->curtime;
						m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
					}
					else
					{
						// DevMsg("TASK_RANGE_ATTACK1 complete\n");
						TaskComplete();
					}
				}
			}
			else
			{
				// DevMsg("Wait\n");
			}
		}
		break;

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Combine::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	Vector result = BaseClass::BodyTarget( posSrc, bNoisy );

	// @TODO (toml 02-02-04): this seems wrong. Isn't this already be accounted for 
	// with the eye position used in the base BodyTarget()
	if ( GetFlags() & FL_DUCKING )
		result -= Vector(0,0,24);

	return result;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_Combine::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if( m_spawnflags & SF_COMBINE_NO_LOOK )
	{
		// When no look is set, if enemy has eluded the squad, 
		// he's always invisble to me
		if (GetEnemies()->HasEludedMe(pEntity))
		{
			return false;
		}
	}
	return BaseClass::FVisible(pEntity, traceMask, ppBlocker);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::Event_Killed( const CTakeDamageInfo &info )
{
	// if I was killed before I could finish throwing my grenade, drop
	// a grenade item that the player can retrieve.
	if( GetActivity() == ACT_RANGE_ATTACK2 )
	{
		if( m_iLastAnimEventHandled != COMBINE_AE_GREN_TOSS )
		{
			// Drop the grenade as an item.
			Vector vecStart;
			GetAttachment( "lefthand", vecStart );

			CBaseEntity *pItem = DropItem( "weapon_frag", vecStart, RandomAngle(0,360) );

			if ( pItem )
			{
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();

				if ( pObj )
				{
					Vector			vel;
					vel.x = random->RandomFloat( -100.0f, 100.0f );
					vel.y = random->RandomFloat( -100.0f, 100.0f );
					vel.z = random->RandomFloat( 800.0f, 1200.0f );
					AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

					vel[2] = 0.0f;
					pObj->AddVelocity( &vel, &angImp );
				}

				// In the Citadel we need to dissolve this
				if ( PlayerHasMegaPhysCannon() )
				{
					CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>(pItem);

					pWeapon->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Override.  Don't update if I'm not looking
// Input  :
// Output : Returns true is new enemy, false is known enemy
//-----------------------------------------------------------------------------
bool CNPC_Combine::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer )
{
	if( m_spawnflags & SF_COMBINE_NO_LOOK )
	{
		return false;
	}

	return BaseClass::UpdateEnemyMemory( pEnemy, position, pInformer );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Combine::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if (gpGlobals->curtime < m_flNextAttack)
	{
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK2 );
	}

	SetCustomInterruptCondition( COND_COMBINE_HIT_BY_BUGBAIT );

	if ( !IsCurSchedule( SCHED_COMBINE_BURNING_STAND ) )
	{
		SetCustomInterruptCondition( COND_COMBINE_ON_FIRE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_Combine::NPC_TranslateActivity( Activity eNewActivity )
{
	//Slaming this back to ACT_COMBINE_BUGBAIT since we don't want ANYTHING to change our activity while we burn.
	if ( HasCondition( COND_COMBINE_ON_FIRE ) )
		return BaseClass::NPC_TranslateActivity( ACT_COMBINE_BUGBAIT );

	if (eNewActivity == ACT_RANGE_ATTACK2)
	{
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if (Weapon_OwnsThisType( "weapon_grenadelauncher" ) )
		{
			return ( Activity )ACT_COMBINE_LAUNCH_GRENADE;
		}
		else
		{
			return ( Activity )ACT_COMBINE_THROW_GRENADE;
		}
	}
	else if (eNewActivity == ACT_IDLE)
	{
		if ( !IsCrouching() && ( m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT ) )
		{
			eNewActivity = ACT_IDLE_ANGRY;
		}
	}

	if ( m_AssaultBehavior.IsRunning() )
	{
		switch ( eNewActivity )
		{
		case ACT_IDLE:
			eNewActivity = ACT_IDLE_ANGRY;
			break;

		case ACT_WALK:
			eNewActivity = ACT_WALK_AIM;
			break;

		case ACT_RUN:
			eNewActivity = ACT_RUN_AIM;
			break;
		}
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//-----------------------------------------------------------------------------
// Purpose: Overidden for human grunts because they  hear the DANGER sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::GetSoundInterests( void )
{
	return	SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER | SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this NPC can hear the specified sound
//-----------------------------------------------------------------------------
bool CNPC_Combine::QueryHearSound( CSound *pSound )
{
	if ( pSound->SoundContext() & SOUND_CONTEXT_COMBINE_ONLY )
		return true;

	if ( pSound->SoundContext() & SOUND_CONTEXT_EXCLUDE_COMBINE )
		return false;

	return BaseClass::QueryHearSound( pSound );
}


//-----------------------------------------------------------------------------
// Purpose: Announce an assault if the enemy can see me and we are pretty 
//			close to him/her
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::AnnounceAssault(void)
{
	if (random->RandomInt(0,5) > 1)
		return;

	// If enemy can see me make assualt sound
	CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();

	if (!pBCC)
		return;

	if (!FOkToMakeSound())
		return;

	// Make sure we are pretty close
	if ( WorldSpaceCenter().DistToSqr( pBCC->WorldSpaceCenter() ) > (2000 * 2000))
		return;

	// Make sure we are in view cone of player
	if (!pBCC->FInViewCone ( this ))
		return;

	// Make sure player can see me
	if ( FVisible( pBCC ) )
	{
		m_Sentences.Speak( "COMBINE_ASSAULT" );
	}
}


void CNPC_Combine::AnnounceEnemyType( CBaseEntity *pEnemy )
{
	const char *pSentenceName = "COMBINE_MONST";
	switch ( pEnemy->Classify() )
	{
	case CLASS_PLAYER:
		pSentenceName = "COMBINE_ALERT";
		break;

	case CLASS_PLAYER_ALLY:
	case CLASS_CITIZEN_REBEL:
	case CLASS_CITIZEN_PASSIVE:
	case CLASS_VORTIGAUNT:
		pSentenceName = "COMBINE_MONST_CITIZENS";
		break;

	case CLASS_PLAYER_ALLY_VITAL:
		pSentenceName = "COMBINE_MONST_CHARACTER";
		break;

	case CLASS_ANTLION:
		pSentenceName = "COMBINE_MONST_BUGS";
		break;

	case CLASS_ZOMBIE:
		pSentenceName = "COMBINE_MONST_ZOMBIES";
		break;

	case CLASS_HEADCRAB:
	case CLASS_BARNACLE:
		pSentenceName = "COMBINE_MONST_PARASITES";
		break;
	}

	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
}

void CNPC_Combine::AnnounceEnemyKill( CBaseEntity *pEnemy )
{
	if (!pEnemy )
		return;

	const char *pSentenceName = "COMBINE_KILL_MONST";
	switch ( pEnemy->Classify() )
	{
	case CLASS_PLAYER:
		pSentenceName = "COMBINE_PLAYER_DEAD";
		break;

		// no sentences for these guys yet
	case CLASS_PLAYER_ALLY:
	case CLASS_CITIZEN_REBEL:
	case CLASS_CITIZEN_PASSIVE:
	case CLASS_VORTIGAUNT:
		break;

	case CLASS_PLAYER_ALLY_VITAL:
		break;

	case CLASS_ANTLION:
		break;

	case CLASS_ZOMBIE:
		break;

	case CLASS_HEADCRAB:
	case CLASS_BARNACLE:
		break;
	}

	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
}

//-----------------------------------------------------------------------------
// Select the combat schedule
//-----------------------------------------------------------------------------
int CNPC_Combine::SelectCombatSchedule()
{
	// -----------
	// dead enemy
	// -----------
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		// call base class, all code to handle dead enemies is centralized there.
		return SCHED_NONE;
	}

	// -----------
	// new enemy
	// -----------
	if ( HasCondition( COND_NEW_ENEMY ) )
	{
		CBaseEntity *pEnemy = GetEnemy();
		bool bFirstContact = false;
		float flTimeSinceFirstSeen = gpGlobals->curtime - GetEnemies()->FirstTimeSeen( pEnemy );

		if( flTimeSinceFirstSeen < 3.0f )
			bFirstContact = true;

		if ( m_pSquad && pEnemy )
		{
			if ( HasCondition( COND_SEE_ENEMY ) )
			{
				AnnounceEnemyType( pEnemy );
			}

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlot( SQUAD_SLOT_ATTACK1 ) )
			{
				// Start suppressing if someone isn't firing already (SLOT_ATTACK1). This means
				// I'm the guy who spotted the enemy, I should react immediately.
				return SCHED_COMBINE_SUPPRESS;
			}

			if ( m_pSquad->IsLeader( this ) || ( m_pSquad->GetLeader() && m_pSquad->GetLeader()->GetEnemy() != pEnemy ) )
			{
				// I'm the leader, but I didn't get the job suppressing the enemy. We know this because
				// This code only runs if the code above didn't assign me SCHED_COMBINE_SUPPRESS.
				if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					return SCHED_RANGE_ATTACK1;
				}

				if( HasCondition(COND_WEAPON_HAS_LOS) && IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					// If everyone else is attacking and I have line of fire, wait for a chance to cover someone.
					if( OccupyStrategySlot( SQUAD_SLOT_OVERWATCH ) )
					{
						return SCHED_COMBINE_ENTER_OVERWATCH;
					}
				}
			}
			else
			{
				if ( m_pSquad->GetLeader() && FOkToMakeSound( SENTENCE_PRIORITY_MEDIUM ) )
				{
					JustMadeSound( SENTENCE_PRIORITY_MEDIUM );	// squelch anything that isn't high priority so the leader can speak
				}

				// First contact, and I'm solo, or not the squad leader.
				if( HasCondition( COND_SEE_ENEMY ) && CanGrenadeEnemy() )
				{
					if( OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
					{
						return SCHED_RANGE_ATTACK2;
					}
				}

				if( !bFirstContact && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					if( random->RandomInt(0, 100) < 60 )
					{
						return SCHED_ESTABLISH_LINE_OF_FIRE;
					}
					else
					{
						return SCHED_COMBINE_PRESS_ATTACK;
					}
				}

				return SCHED_TAKE_COVER_FROM_ENEMY;
			}
		}
	}

	// ---------------------
	// no ammo
	// ---------------------
	if ( ( HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition ( COND_LOW_PRIMARY_AMMO ) ) && !HasCondition( COND_CAN_MELEE_ATTACK1) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}

	// ----------------------
	// LIGHT DAMAGE
	// ----------------------
	if ( HasCondition( COND_LIGHT_DAMAGE ) )
	{
		if ( GetEnemy() != NULL )
		{
			// only try to take cover if we actually have an enemy!

			// FIXME: need to take cover for enemy dealing the damage

			// A standing guy will either crouch or run.
			// A crouching guy tries to stay stuck in.
			if( !IsCrouching() )
			{
				if( GetEnemy() && random->RandomFloat( 0, 100 ) < 50 && CouldShootIfCrouching( GetEnemy() ) )
				{
					Crouch();
				}
				else
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
					// m_Sentences.Speak( "COMBINE_COVER" );
					return SCHED_TAKE_COVER_FROM_ENEMY;
				}
			}
		}
		else
		{
			// How am I wounded in combat with no enemy?
			Assert( GetEnemy() != NULL );
		}
	}

	// If I'm scared of this enemy run away
	if ( IRelationType( GetEnemy() ) == D_FR )
	{
		if (HasCondition( COND_SEE_ENEMY )	|| 
			HasCondition( COND_SEE_FEAR )	|| 
			HasCondition( COND_LIGHT_DAMAGE ) || 
			HasCondition( COND_HEAVY_DAMAGE ))
		{
			FearSound();
			//ClearCommandGoal();
			return SCHED_RUN_FROM_ENEMY;
		}

		// If I've seen the enemy recently, cower. Ignore the time for unforgettable enemies.
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find( GetEnemy() );
		if ( (pMemory && pMemory->bUnforgettable) || (GetEnemyLastTimeSeen() > (gpGlobals->curtime - 5.0)) )
		{
			// If we're facing him, just look ready. Otherwise, face him.
			if ( FInAimCone( GetEnemy()->EyePosition() ) )
				return SCHED_COMBAT_STAND;

			return SCHED_FEAR_FACE;
		}
	}

	int attackSchedule = SelectScheduleAttack();
	if ( attackSchedule != SCHED_NONE )
		return attackSchedule;

	if (HasCondition(COND_ENEMY_OCCLUDED))
	{
		// stand up, just in case
		Stand();
		DesireStand();

		if( GetEnemy() && !(GetEnemy()->GetFlags() & FL_NOTARGET) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		{
			// Charge in and break the enemy's cover!
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}

		// If I'm a long, long way away, establish a LOF anyway. Once I get there I'll
		// start respecting the squad slots again.
		float flDistSq = GetEnemy()->WorldSpaceCenter().DistToSqr( WorldSpaceCenter() );
		if ( flDistSq > Square(3000) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;

		// Otherwise tuck in.
		Remember( bits_MEMORY_INCOVER );
		return SCHED_COMBINE_WAIT_IN_COVER;
	}

	// --------------------------------------------------------------
	// Enemy not occluded but isn't open to attack
	// --------------------------------------------------------------
	if ( HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( (HasCondition( COND_TOO_FAR_TO_ATTACK ) || IsUsingTacticalVariant(TACTICAL_VARIANT_PRESSURE_ENEMY) ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
		{
			return SCHED_COMBINE_PRESS_ATTACK;
		}

		AnnounceAssault(); 
		return SCHED_COMBINE_ASSAULT;
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::SelectSchedule( void )
{
	if ( IsWaitingToRappel() && BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}

	if ( HasCondition(COND_COMBINE_ON_FIRE) )
		return SCHED_COMBINE_BURNING_STAND;

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( m_hForcedGrenadeTarget )
	{
		if ( m_flNextGrenadeCheck < gpGlobals->curtime )
		{
			Vector vecTarget = m_hForcedGrenadeTarget->WorldSpaceCenter();

			if ( IsElite() )
			{
				if ( FVisible( m_hForcedGrenadeTarget ) )
				{
					m_vecAltFireTarget = vecTarget;
					m_hForcedGrenadeTarget = NULL;
					return SCHED_COMBINE_AR2_ALTFIRE;
				}
			}
			else
			{
				// If we can, throw a grenade at the target. 
				// Ignore grenade count / distance / etc
				if ( CheckCanThrowGrenade( vecTarget ) )
				{
					m_hForcedGrenadeTarget = NULL;
					return SCHED_COMBINE_FORCED_GRENADE_THROW;
				}
			}
		}

		// Can't throw at the target, so lets try moving to somewhere where I can see it
		if ( !FVisible( m_hForcedGrenadeTarget ) )
		{
			return SCHED_COMBINE_MOVE_TO_FORCED_GREN_LOS;
		}
	}

	if ( m_NPCState != NPC_STATE_SCRIPT)
	{
		// If we're hit by bugbait, thrash around
		if ( HasCondition( COND_COMBINE_HIT_BY_BUGBAIT ) )
		{
			// Don't do this if we're mounting a func_tank
			if ( m_FuncTankBehavior.IsMounted() == true )
			{
				m_FuncTankBehavior.Dismount();
			}

			ClearCondition( COND_COMBINE_HIT_BY_BUGBAIT );
			return SCHED_COMBINE_BUGBAIT_DISTRACTION;
		}

		// We've been told to move away from a target to make room for a grenade to be thrown at it
		if ( HasCondition( COND_HEAR_MOVE_AWAY ) )
		{
			return SCHED_MOVE_AWAY;
		}

		// These things are done in any state but dead and prone
		if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE )
		{
			// Cower when physics objects are thrown at me
			if ( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
			{
				return SCHED_FLINCH_PHYSICS;
			}

			// grunts place HIGH priority on running away from danger sounds.
			if ( HasCondition(COND_HEAR_DANGER) )
			{
				CSound *pSound;
				pSound = GetBestSound();

				Assert( pSound != NULL );
				if ( pSound)
				{
					if (pSound->m_iType & SOUND_DANGER)
					{
						// I hear something dangerous, probably need to take cover.
						// dangerous sound nearby!, call it out
						const char *pSentenceName = "COMBINE_DANGER";

						CBaseEntity *pSoundOwner = pSound->m_hOwner;
						if ( pSoundOwner )
						{
							CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>(pSoundOwner);
							if ( pGrenade && pGrenade->GetThrower() )
							{
								if ( IRelationType( pGrenade->GetThrower() ) != D_LI )
								{
									// special case call out for enemy grenades
									pSentenceName = "COMBINE_GREN";
								}
							}
						}

						m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );

						// If the sound is approaching danger, I have no enemy, and I don't see it, turn to face.
						if( !GetEnemy() && pSound->IsSoundType(SOUND_CONTEXT_DANGER_APPROACH) && pSound->m_hOwner && !FInViewCone(pSound->GetSoundReactOrigin()) )
						{
							GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
							return SCHED_COMBINE_FACE_IDEAL_YAW;
						}

						return SCHED_TAKE_COVER_FROM_BEST_SOUND;
					}

					// JAY: This was disabled in HL1.  Test?
					if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_COMBAT) ))
					{
						GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
					}
				}
			}
		}

		if( BehaviorSelectSchedule() )
		{
			return BaseClass::SelectSchedule();
		}
	}

	switch	( m_NPCState )
	{
	case NPC_STATE_IDLE:
		{
			if ( m_bShouldPatrol )
				return SCHED_COMBINE_PATROL;
		}
		// NOTE: Fall through!

	case NPC_STATE_ALERT:
		{
			if( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) )
			{
				AI_EnemyInfo_t *pDanger = GetEnemies()->GetDangerMemory();
				if( pDanger && FInViewCone(pDanger->vLastKnownLocation) && !BaseClass::FVisible(pDanger->vLastKnownLocation) )
				{
					// I've been hurt, I'm facing the danger, but I don't see it, so move from this position.
					return SCHED_TAKE_COVER_FROM_ORIGIN;
				}
			}

			if( HasCondition( COND_HEAR_COMBAT ) )
			{
				CSound *pSound = GetBestSound();

				if( pSound && pSound->IsSoundType( SOUND_COMBAT ) )
				{
					if( m_pSquad && m_pSquad->GetSquadMemberNearestTo( pSound->GetSoundReactOrigin() ) == this && OccupyStrategySlot( SQUAD_SLOT_INVESTIGATE_SOUND ) )
					{
						return SCHED_INVESTIGATE_SOUND;
					}
				}
			}

			// Don't patrol if I'm in the middle of an assault, because I'll never return to the assault. 
			if ( !m_AssaultBehavior.HasAssaultCue() )
			{
				if( m_bShouldPatrol || HasCondition( COND_COMBINE_SHOULD_PATROL ) )
					return SCHED_COMBINE_PATROL;
			}
		}
		break;

	case NPC_STATE_COMBAT:
		{
			int nSched = SelectCombatSchedule();
			if ( nSched != SCHED_NONE )
				return nSched;
		}
		break;
	}

	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Combine::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_COMBINE_TAKE_COVER1 )
	{
		if( IsInSquad() && IsStrategySlotRangeOccupied(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2) && HasCondition(COND_SEE_ENEMY) )
		{
			// This eases the effects of an unfortunate bug that usually plagues shotgunners. Since their rate of fire is low,
			// they spend relatively long periods of time without an attack squad slot. If you corner a shotgunner, usually 
			// the other members of the squad will hog all of the attack slots and pick schedules to move to establish line of
			// fire. During this time, the shotgunner is prevented from attacking. If he also cannot find cover (the fallback case)
			// he will stand around like an idiot, right in front of you. Instead of this, we have him run up to you for a melee attack.
			return SCHED_COMBINE_MOVE_TO_MELEE;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Should we charge the player?
//-----------------------------------------------------------------------------
bool CNPC_Combine::ShouldChargePlayer()
{
	return GetEnemy() && GetEnemy()->IsPlayer() && PlayerHasMegaPhysCannon() && !IsLimitingHintGroups();
}


//-----------------------------------------------------------------------------
// Select attack schedules
//-----------------------------------------------------------------------------
#define COMBINE_MEGA_PHYSCANNON_ATTACK_DISTANCE	192
#define COMBINE_MEGA_PHYSCANNON_ATTACK_DISTANCE_SQ	(COMBINE_MEGA_PHYSCANNON_ATTACK_DISTANCE*COMBINE_MEGA_PHYSCANNON_ATTACK_DISTANCE)

int CNPC_Combine::SelectScheduleAttack()
{
	// Drop a grenade?
	if ( HasCondition( COND_COMBINE_DROP_GRENADE ) )
		return SCHED_COMBINE_DROP_GRENADE;

	// Kick attack?
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	// If I'm fighting a combine turret (it's been hacked to attack me), I can't really
	// hurt it with bullets, so become grenade happy.
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_COMBINE && FClassnameIs(GetEnemy(), "npc_turret_floor") )
	{
		// Don't do this until I've been fighting the turret for a few seconds
		float flTimeAtFirstHand = GetEnemies()->TimeAtFirstHand(GetEnemy());
		if ( flTimeAtFirstHand != AI_INVALID_TIME )
		{
			float flTimeEnemySeen = gpGlobals->curtime - flTimeAtFirstHand;
			if ( flTimeEnemySeen > 4.0 )
			{
				if ( CanGrenadeEnemy() && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
					return SCHED_RANGE_ATTACK2;
			}
		}

		// If we're not in the viewcone of the turret, run up and hit it. Do this a bit later to
		// give other squadmembers a chance to throw a grenade before I run in.
		if ( !GetEnemy()->MyNPCPointer()->FInViewCone( this ) && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
			return SCHED_COMBINE_CHARGE_TURRET;
	}

	// When fighting against the player who's wielding a mega-physcannon, 
	// always close the distance if possible
	// But don't do it if you're in a nav-limited hint group
	if ( ShouldChargePlayer() )
	{
		float flDistSq = GetEnemy()->WorldSpaceCenter().DistToSqr( WorldSpaceCenter() );
		if ( flDistSq <= COMBINE_MEGA_PHYSCANNON_ATTACK_DISTANCE_SQ )
		{
			if( HasCondition(COND_SEE_ENEMY) )
			{
				if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
					return SCHED_RANGE_ATTACK1;
			}
			else
			{
				if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
					return SCHED_COMBINE_PRESS_ATTACK;
			}
		}

		if ( HasCondition(COND_SEE_ENEMY) && !IsUnreachable( GetEnemy() ) )
		{
			return SCHED_COMBINE_CHARGE_PLAYER;
		}
	}

	// Can I shoot?
	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{

		// JAY: HL1 behavior missing?
#if 0
		if ( m_pSquad )
		{
			// if the enemy has eluded the squad and a squad member has just located the enemy
			// and the enemy does not see the squad member, issue a call to the squad to waste a 
			// little time and give the player a chance to turn.
			if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return SCHED_GRUNT_FOUND_ENEMY;
			}
		}
#endif

		// Engage if allowed
		if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		{
			return SCHED_RANGE_ATTACK1;
		}

		// Throw a grenade if not allowed to engage with weapon.
		if ( CanGrenadeEnemy() )
		{
			if ( OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
			{
				return SCHED_RANGE_ATTACK2;
			}
		}

		DesireCrouch();
		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	if ( GetEnemy() && !HasCondition(COND_SEE_ENEMY) )
	{
		// We don't see our enemy. If it hasn't been long since I last saw him,
		// and he's pretty close to the last place I saw him, throw a grenade in 
		// to flush him out. A wee bit of cheating here...

		float flTime;
		float flDist;

		flTime = gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() );
		flDist = ( GetEnemy()->GetAbsOrigin() - GetEnemies()->LastSeenPosition( GetEnemy() ) ).Length();

		//Msg("Time: %f   Dist: %f\n", flTime, flDist );
		if ( flTime <= COMBINE_GRENADE_FLUSH_TIME && flDist <= COMBINE_GRENADE_FLUSH_DIST && CanGrenadeEnemy( false ) && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
		{
			return SCHED_RANGE_ATTACK2;
		}
	}

	if (HasCondition(COND_WEAPON_SIGHT_OCCLUDED))
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlot( SQUAD_SLOT_ATTACK_OCCLUDER ) )
		{
			return SCHED_SHOOT_ENEMY_COVER;
		}
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( m_pSquad )
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if (	g_pGameRules->IsSkillLevel( SKILL_HARD )	&& 
					HasCondition(COND_CAN_RANGE_ATTACK2)		&&
					OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
				{
					m_Sentences.Speak( "COMBINE_THROW_GRENADE" );
					return SCHED_COMBINE_TOSS_GRENADE_COVER1;
				}
				else
				{
					if ( ShouldChargePlayer() && !IsUnreachable( GetEnemy() ) )
						return SCHED_COMBINE_CHARGE_PLAYER;

					return SCHED_COMBINE_TAKE_COVER1;
				}
			}
			else
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if ( random->RandomInt(0,1) && HasCondition(COND_CAN_RANGE_ATTACK2) )
				{
					return SCHED_COMBINE_GRENADE_COVER1;
				}
				else
				{
					if ( ShouldChargePlayer() && !IsUnreachable( GetEnemy() ) )
						return SCHED_COMBINE_CHARGE_PLAYER;

					return SCHED_COMBINE_TAKE_COVER1;
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND;
		}
		break;
	case SCHED_COMBINE_TAKECOVER_FAILED:
		{
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );
			}

			// Run somewhere randomly
			return TranslateSchedule( SCHED_FAIL ); 
			break;
		}
		break;
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			if( !IsCrouching() )
			{
				if( GetEnemy() && CouldShootIfCrouching( GetEnemy() ) )
				{
					Crouch();
					return SCHED_COMBAT_FACE;
				}
			}

			if( HasCondition( COND_SEE_ENEMY ) )
			{
				return TranslateSchedule( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			else if ( !m_AssaultBehavior.HasAssaultCue() )
			{
				// Don't patrol if I'm in the middle of an assault, because 
				// I'll never return to the assault. 
				if ( GetEnemy() )
				{
					RememberUnreachable( GetEnemy() );
				}

				return TranslateSchedule( SCHED_COMBINE_PATROL );
			}
		}
		break;
	case SCHED_COMBINE_ASSAULT:
		{
			CBaseEntity *pEntity = GetEnemy();

			// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
			// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
			if (pEntity && pEntity->MyNPCPointer())
			{
				if ( !(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_WEAPON_RANGE_ATTACK1))
				{
					return TranslateSchedule( SCHED_ESTABLISH_LINE_OF_FIRE );
				}
			}
			// don't charge forward if there's a hint group
			if (GetHintGroup() != NULL_STRING)
			{
				return TranslateSchedule( SCHED_ESTABLISH_LINE_OF_FIRE );
			}
			return SCHED_COMBINE_ASSAULT;
		}
	case SCHED_ESTABLISH_LINE_OF_FIRE:
		{
			// always assume standing
			// Stand();

			if( CanAltFireEnemy(true) && OccupyStrategySlot(SQUAD_SLOT_SPECIAL_ATTACK) )
			{
				// If an elite in the squad could fire a combine ball at the player's last known position,
				// do so!
				return SCHED_COMBINE_AR2_ALTFIRE;
			}

			if( IsUsingTacticalVariant( TACTICAL_VARIANT_PRESSURE_ENEMY ) && !IsRunningBehavior() )
			{
				if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					return SCHED_COMBINE_PRESS_ATTACK;
				}
			}

			return SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE;
		}
		break;
	case SCHED_HIDE_AND_RELOAD:
		{
			// stand up, just in case
			// Stand();
			// DesireStand();
			if( CanGrenadeEnemy() && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) && random->RandomInt( 0, 100 ) < 20 )
			{
				// If I COULD throw a grenade and I need to reload, 20% chance I'll throw a grenade before I hide to reload.
				return SCHED_COMBINE_GRENADE_AND_RELOAD;
			}

			// No running away in the citadel!
			if ( ShouldChargePlayer() )
				return SCHED_RELOAD;

			return SCHED_COMBINE_HIDE_AND_RELOAD;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if ( HasCondition( COND_NO_PRIMARY_AMMO ) || HasCondition( COND_LOW_PRIMARY_AMMO ) )
			{
				// Ditch the strategy slot for attacking (which we just reserved!)
				VacateStrategySlot();
				return TranslateSchedule( SCHED_HIDE_AND_RELOAD );
			}

			if( CanAltFireEnemy(true) && OccupyStrategySlot(SQUAD_SLOT_SPECIAL_ATTACK) )
			{
				// Since I'm holding this squadslot, no one else can try right now. If I die before the shot 
				// goes off, I won't have affected anyone else's ability to use this attack at their nearest
				// convenience.
				return SCHED_COMBINE_AR2_ALTFIRE;
			}

			if ( IsCrouching() || ( CrouchIsDesired() && !HasCondition( COND_HEAVY_DAMAGE ) ) )
			{
				// See if we can crouch and shoot
				if (GetEnemy() != NULL)
				{
					float dist = (GetLocalOrigin() - GetEnemy()->GetLocalOrigin()).Length();

					// only crouch if they are relatively far away
					if (dist > COMBINE_MIN_CROUCH_DISTANCE)
					{
						// try crouching
						Crouch();

						Vector targetPos = GetEnemy()->BodyTarget(GetActiveWeapon()->GetLocalOrigin());

						// if we can't see it crouched, stand up
						if (!WeaponLOSCondition(GetLocalOrigin(),targetPos,false))
						{
							Stand();
						}
					}
				}
			}
			else
			{
				// always assume standing
				Stand();
			}

			return SCHED_COMBINE_RANGE_ATTACK1;
		}
	case SCHED_RANGE_ATTACK2:
		{
			// If my weapon can range attack 2 use the weapon
			if (GetActiveWeapon() && GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK2)
			{
				return SCHED_RANGE_ATTACK2;
			}
			// Otherwise use innate attack
			else
			{
				return SCHED_COMBINE_RANGE_ATTACK2;
			}
		}
		// SCHED_COMBAT_FACE:
		// SCHED_COMBINE_WAIT_FACE_ENEMY:
		// SCHED_COMBINE_SWEEP:
		// SCHED_COMBINE_COVER_AND_RELOAD:
		// SCHED_COMBINE_FOUND_ENEMY:

	case SCHED_VICTORY_DANCE:
		{
			return SCHED_COMBINE_VICTORY_DANCE;
		}
	case SCHED_COMBINE_SUPPRESS:
		{
#define MIN_SIGNAL_DIST	256
			if ( GetEnemy() != NULL && GetEnemy()->IsPlayer() && m_bFirstEncounter )
			{
				float flDistToEnemy = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).Length();

				if( flDistToEnemy >= MIN_SIGNAL_DIST )
				{
					m_bFirstEncounter = false;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
					return SCHED_COMBINE_SIGNAL_SUPPRESS;
				}
			}

			return SCHED_COMBINE_SUPPRESS;
		}
	case SCHED_FAIL:
		{
			if ( GetEnemy() != NULL )
			{
				return SCHED_COMBINE_COMBAT_FAIL;
			}
			return SCHED_FAIL;
		}

	case SCHED_COMBINE_PATROL:
		{
			// If I have an enemy, don't go off into random patrol mode.
			if ( GetEnemy() && GetEnemy()->IsAlive() )
				return SCHED_COMBINE_PATROL_ENEMY;

			return SCHED_COMBINE_PATROL;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
//=========================================================
void CNPC_Combine::OnStartSchedule( int scheduleType ) 
{
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Combine::HandleAnimEvent( animevent_t *pEvent )
{
	Vector vecShootDir;
	Vector vecShootOrigin;
	bool handledEvent = false;

	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if ( pEvent->event == COMBINE_AE_BEGIN_ALTFIRE )
		{
			EmitSound( "Weapon_CombineGuard.Special1" );
			handledEvent = true;
		}
		else if ( pEvent->event == COMBINE_AE_ALTFIRE )
		{
			if( IsElite() )
			{
				animevent_t fakeEvent;

				fakeEvent.pSource = this;
				fakeEvent.event = EVENT_WEAPON_AR2_ALTFIRE;
				GetActiveWeapon()->Operator_HandleAnimEvent( &fakeEvent, this );

				// Stop other squad members from combine balling for a while.
				DelaySquadAltFireAttack( 10.0f );

				// I'm disabling this decrementor. At the time of this change, the elites
				// don't bother to check if they have grenades anyway. This means that all
				// elites have infinite combine balls, even if the designer marks the elite
				// as having 0 grenades. By disabling this decrementor, yet enabling the code
				// that makes sure the elite has grenades in order to fire a combine ball, we
				// preserve the legacy behavior while making it possible for a designer to prevent
				// elites from shooting combine balls by setting grenades to '0' in hammer. (sjb) EP2_OUTLAND_10
				// m_iNumGrenades--;
			}

			handledEvent = true;
		}
		else
		{
			BaseClass::HandleAnimEvent( pEvent );
		}
	}
	else
	{
		switch( pEvent->event )
		{
		case COMBINE_AE_AIM:	
			{
				handledEvent = true;
				break;
			}
		case COMBINE_AE_RELOAD:

			// We never actually run out of ammo, just need to refill the clip
			if (GetActiveWeapon())
			{
				GetActiveWeapon()->WeaponSound( RELOAD_NPC );
				GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
				GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2();  
			}
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
			handledEvent = true;
			break;

		case COMBINE_AE_GREN_TOSS:
			{
				Vector vecSpin;
				vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

				Vector vecStart;
				GetAttachment( "lefthand", vecStart );

				if( m_NPCState == NPC_STATE_SCRIPT )
				{
					// Use a fixed velocity for grenades thrown in scripted state.
					// Grenades thrown from a script do not count against grenades remaining for the AI to use.
					Vector forward, up, vecThrow;

					GetVectors( &forward, NULL, &up );
					vecThrow = forward * 750 + up * 175;
					Fraggrenade_Create( vecStart, vec3_angle, vecThrow, vecSpin, this, COMBINE_GRENADE_TIMER, true );
				}
				else
				{
					// Use the Velocity that AI gave us.
					Fraggrenade_Create( vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, COMBINE_GRENADE_TIMER, true );
					m_iNumGrenades--;
				}

				// wait six seconds before even looking again to see if a grenade can be thrown.
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;
			}
			handledEvent = true;
			break;

		case COMBINE_AE_GREN_LAUNCH:
			{
				EmitSound( "NPC_Combine.GrenadeLaunch" );

				CBaseEntity *pGrenade = CreateNoSpawn( "npc_contactgrenade", Weapon_ShootPosition(), vec3_angle, this );
				pGrenade->KeyValue( "velocity", m_vecTossVelocity );
				pGrenade->Spawn( );

				if ( g_pGameRules->IsSkillLevel(SKILL_HARD) )
					m_flNextGrenadeCheck = gpGlobals->curtime + random->RandomFloat( 2, 5 );// wait a random amount of time before shooting again
				else
					m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			}
			handledEvent = true;
			break;

		case COMBINE_AE_GREN_DROP:
			{
				Vector vecStart;
				GetAttachment( "lefthand", vecStart );

				Fraggrenade_Create( vecStart, vec3_angle, m_vecTossVelocity, vec3_origin, this, COMBINE_GRENADE_TIMER, true );
				m_iNumGrenades--;
			}
			handledEvent = true;
			break;

		case COMBINE_AE_KICK:
			{
				// Does no damage, because damage is applied based upon whether the target can handle the interaction
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, -Vector(16,16,18), Vector(16,16,18), 0, DMG_CLUB );
				CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
				if (pBCC)
				{
					Vector forward, up;
					AngleVectors( GetLocalAngles(), &forward, NULL, &up );

					if ( !pBCC->DispatchInteraction( g_interactionCombineBash, NULL, this ) )
					{
						if ( pBCC->IsPlayer() )
						{
							pBCC->ViewPunch( QAngle(-12,-7,0) );
							pHurt->ApplyAbsVelocityImpulse( forward * 100 + up * 50 );
						}

						CTakeDamageInfo info( this, this, m_nKickDamage, DMG_CLUB );
						CalculateMeleeDamageForce( &info, forward, pBCC->GetAbsOrigin() );
						pBCC->TakeDamage( info );

						EmitSound( "NPC_Combine.WeaponBash" );
					}
				}			

				m_Sentences.Speak( "COMBINE_KICK" );
				handledEvent = true;
				break;
			}

		case COMBINE_AE_CAUGHT_ENEMY:
			m_Sentences.Speak( "COMBINE_ALERT" );
			handledEvent = true;
			break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
		}
	}

	if( handledEvent )
	{
		m_iLastAnimEventHandled = pEvent->event;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at an arbitrary position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_Combine::Weapon_ShootPosition( )
{
	bool bStanding = !IsCrouching();
	Vector right;
	GetVectors( NULL, &right, NULL );

	if ((CapabilitiesGet() & bits_CAP_DUCK) )
	{
		if ( IsCrouchedActivity( GetActivity() ) )
		{
			bStanding = false;
		}
	}

	// FIXME: rename this "estimated" since it's not based on animation
	// FIXME: the orientation won't be correct when testing from arbitary positions for arbitary angles

	if  ( bStanding )
	{
		if( HasShotgun() )
		{
			return GetAbsOrigin() + COMBINE_SHOTGUN_STANDING_POSITION + right * 8;
		}
		else
		{
			return GetAbsOrigin() + COMBINE_GUN_STANDING_POSITION + right * 8;
		}
	}
	else
	{
		if( HasShotgun() )
		{
			return GetAbsOrigin() + COMBINE_SHOTGUN_CROUCHING_POSITION + right * 8;
		}
		else
		{
			return GetAbsOrigin() + COMBINE_GUN_CROUCHING_POSITION + right * 8;
		}
	}
}


//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CNPC_Combine::SpeakSentence( int sentenceType )
{
	switch( sentenceType )
	{
	case 0: // assault
		AnnounceAssault();
		break;

	case 1: // Flanking the player
		// If I'm moving more than 20ft, I need to talk about it
		if ( GetNavigator()->GetPath()->GetPathLength() > 20 * 12.0f )
		{
			m_Sentences.Speak( "COMBINE_FLANK" );
		}
		break;
	}
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Combine::PainSound ( void )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	if ( gpGlobals->curtime > m_flNextPainSoundTime )
	{
		const char *pSentenceName = "COMBINE_PAIN";
		float healthRatio = (float)GetHealth() / (float)GetMaxHealth();
		if ( !HasMemory(bits_MEMORY_PAIN_LIGHT_SOUND) && healthRatio > 0.9 )
		{
			Remember( bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "COMBINE_TAUNT";
		}
		else if ( !HasMemory(bits_MEMORY_PAIN_HEAVY_SOUND) && healthRatio < 0.25 )
		{
			Remember( bits_MEMORY_PAIN_HEAVY_SOUND );
			pSentenceName = "COMBINE_COVER";
		}

		m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		m_flNextPainSoundTime = gpGlobals->curtime + 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::LostEnemySound( void)
{
	if ( gpGlobals->curtime <= m_flNextLostSoundTime )
		return;

	const char *pSentence;
	if (!(CBaseEntity*)GetEnemy() || gpGlobals->curtime - GetEnemyLastTimeSeen() > 10)
	{
		pSentence = "COMBINE_LOST_LONG"; 
	}
	else
	{
		pSentence = "COMBINE_LOST_SHORT";
	}

	if ( m_Sentences.Speak( pSentence ) >= 0 )
	{
		m_flNextLostSoundTime = gpGlobals->curtime + random->RandomFloat(5.0,15.0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::FoundEnemySound( void)
{
	m_Sentences.Speak( "COMBINE_REFIND_ENEMY", SENTENCE_PRIORITY_HIGH );
}

//-----------------------------------------------------------------------------
// Purpose: Implemented by subclasses to give them an opportunity to make
//			a sound before they attack
// Input  :
// Output :
//-----------------------------------------------------------------------------

// BUGBUG: It looks like this is never played because combine don't do SCHED_WAKE_ANGRY or anything else that does a TASK_SOUND_WAKE
void CNPC_Combine::AlertSound( void)
{
	if ( gpGlobals->curtime > m_flNextAlertSoundTime )
	{
		m_Sentences.Speak( "COMBINE_GO_ALERT", SENTENCE_PRIORITY_HIGH );
		m_flNextAlertSoundTime = gpGlobals->curtime + 10.0f;
	}
}

//=========================================================
// NotifyDeadFriend
//=========================================================
void CNPC_Combine::NotifyDeadFriend ( CBaseEntity* pFriend )
{
	if ( GetSquad()->NumMembers() < 2 )
	{
		m_Sentences.Speak( "COMBINE_LAST_OF_SQUAD", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_NORMAL );
		JustMadeSound();
		return;
	}
	// relaxed visibility test so that guys say this more often
	//if( FInViewCone( pFriend ) && FVisible( pFriend ) )
	{
		m_Sentences.Speak( "COMBINE_MAN_DOWN" );
	}
	BaseClass::NotifyDeadFriend(pFriend);
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Combine::DeathSound ( void )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	m_Sentences.Speak( "COMBINE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_Combine::IdleSound( void )
{
	if (g_fCombineQuestion || random->RandomInt(0,1))
	{
		if (!g_fCombineQuestion)
		{
			// ask question or make statement
			switch (random->RandomInt(0,2))
			{
			case 0: // check in
				if ( m_Sentences.Speak( "COMBINE_CHECK" ) >= 0 )
				{
					g_fCombineQuestion = 1;
				}
				break;

			case 1: // question
				if ( m_Sentences.Speak( "COMBINE_QUEST" ) >= 0 )
				{
					g_fCombineQuestion = 2;
				}
				break;

			case 2: // statement
				m_Sentences.Speak( "COMBINE_IDLE" );
				break;
			}
		}
		else
		{
			switch (g_fCombineQuestion)
			{
			case 1: // check in
				if ( m_Sentences.Speak( "COMBINE_CLEAR" ) >= 0 )
				{
					g_fCombineQuestion = 0;
				}
				break;
			case 2: // question 
				if ( m_Sentences.Speak( "COMBINE_ANSWER" ) >= 0 )
				{
					g_fCombineQuestion = 0;
				}
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//			This is for Grenade attacks.  As the test for grenade attacks
//			is expensive we don't want to do it every frame.  Return true
//			if we meet minimum set of requirements and then test for actual
//			throw later if we actually decide to do a grenade attack.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Combine::RangeAttack2Conditions( float flDot, float flDist ) 
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the combine has grenades, hasn't checked lately, and
//			can throw a grenade at the target point.
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Combine::CanThrowGrenade( const Vector &vecTarget )
{
	if( m_iNumGrenades < 1 )
	{
		// Out of grenades!
		return false;
	}

	if (gpGlobals->curtime < m_flNextGrenadeCheck )
	{
		// Not allowed to throw another grenade right now.
		return false;
	}

	float flDist;
	flDist = ( vecTarget - GetAbsOrigin() ).Length();

	if( flDist > 1024 || flDist < 128 )
	{
		// Too close or too far!
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( m_flGroundSpeed != 0 )
		return false;

#if 0
	Vector vecEnemyLKP = GetEnemyLKP();
	if ( !( GetEnemy()->GetFlags() & FL_ONGROUND ) && GetEnemy()->GetWaterLevel() == 0 && vecEnemyLKP.z > (GetAbsOrigin().z + WorldAlignMaxs().z)  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}
#endif

	// ---------------------------------------------------------------------
	// Are any of my squad members near the intended grenade impact area?
	// ---------------------------------------------------------------------
	if ( m_pSquad )
	{
		if (m_pSquad->SquadMemberInRange( vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST ))
		{
			// I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			// Tell my squad members to clear out so I can get a grenade in
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY | SOUND_CONTEXT_COMBINE_ONLY, vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST, 0.1 );
			return false;
		}
	}

	return CheckCanThrowGrenade( vecTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the combine can throw a grenade at the specified target point
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Combine::CheckCanThrowGrenade( const Vector &vecTarget )
{
	//NDebugOverlay::Line( EyePosition(), vecTarget, 0, 255, 0, false, 5 );

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: this is only valid for hand grenades, not RPG's
	Vector vecToss;
	Vector vecMins = -Vector(4,4,4);
	Vector vecMaxs = Vector(4,4,4);
	if( FInViewCone( vecTarget ) && CBaseEntity::FVisible( vecTarget ) )
	{
		vecToss = VecCheckThrow( this, EyePosition(), vecTarget, COMBINE_GRENADE_THROW_SPEED, 1.0, &vecMins, &vecMaxs );
	}
	else
	{
		// Have to try a high toss. Do I have enough room?
		trace_t tr;
		AI_TraceLine( EyePosition(), EyePosition() + Vector( 0, 0, 64 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		if( tr.fraction != 1.0 )
		{
			return false;
		}

		vecToss = VecCheckToss( this, EyePosition(), vecTarget, -1, 1.0, true, &vecMins, &vecMaxs );
	}

	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // 1/3 second.
		return true;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::CanAltFireEnemy( bool bUseFreeKnowledge )
{
	if (!IsElite() )
		return false;

	if (IsCrouching())
		return false;

	if( gpGlobals->curtime < m_flNextAltFireTime )
		return false;

	if( !GetEnemy() )
		return false;

	if (gpGlobals->curtime < m_flNextGrenadeCheck )
		return false;

	// See Steve Bond if you plan on changing this next piece of code!! (SJB) EP2_OUTLAND_10
	if (m_iNumGrenades < 1)
		return false;

	CBaseEntity *pEnemy = GetEnemy();

	if( !pEnemy->IsPlayer() && (!pEnemy->IsNPC() || !pEnemy->MyNPCPointer()->IsPlayerAlly()) )
		return false;

	Vector vecTarget;

	// Determine what point we're shooting at
	if( bUseFreeKnowledge )
	{
		vecTarget = GetEnemies()->LastKnownPosition( pEnemy ) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}
	else
	{
		vecTarget = GetEnemies()->LastSeenPosition( pEnemy ) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}

	// Trace a hull about the size of the combine ball (don't shoot through grates!)
	trace_t tr;

	Vector mins( -12, -12, -12 );
	Vector maxs( 12, 12, 12 );

	Vector vShootPosition = EyePosition();

	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->GetAttachment( "muzzle", vShootPosition );
	}

	// Trace a hull about the size of the combine ball.
	UTIL_TraceHull( vShootPosition, vecTarget, mins, maxs, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	float flLength = (vShootPosition - vecTarget).Length();

	flLength *= tr.fraction;

	//If the ball can travel at least 65% of the distance to the player then let the NPC shoot it.
	if( tr.fraction >= 0.65 && flLength > 128.0f )
	{
		// Target is valid
		m_vecAltFireTarget = vecTarget;
		return true;
	}


	// Check again later
	m_vecAltFireTarget = vec3_origin;
	m_flNextGrenadeCheck = gpGlobals->curtime + 1.0f;
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::CanGrenadeEnemy( bool bUseFreeKnowledge )
{
	if( IsElite() )
		return false;

	CBaseEntity *pEnemy = GetEnemy();

	Assert( pEnemy != NULL );

	if( pEnemy )
	{
		// I'm not allowed to throw grenades during dustoff
		if ( IsCurSchedule(SCHED_DROPSHIP_DUSTOFF) )
			return false;

		if( bUseFreeKnowledge )
		{
			// throw to where we think they are.
			return CanThrowGrenade( GetEnemies()->LastKnownPosition( pEnemy ) );
		}
		else
		{
			// hafta throw to where we last saw them.
			return CanThrowGrenade( GetEnemies()->LastSeenPosition( pEnemy ) );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For combine melee attack (kick/hit)
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 64)
	{
		return COND_NONE; // COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NONE; // COND_NOT_FACING_ATTACK;
	}

	// Check Z
	if ( GetEnemy() && fabs(GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > 64 )
		return COND_NONE;

	if ( dynamic_cast<CBaseHeadcrab *>(GetEnemy()) != NULL )
	{
		return COND_NONE;
	}

	// Make sure not trying to kick through a window or something. 
	trace_t tr;
	Vector vecSrc, vecEnd;

	vecSrc = WorldSpaceCenter();
	vecEnd = GetEnemy()->WorldSpaceCenter();

	AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if( tr.m_pEnt != GetEnemy() )
	{
		return COND_NONE;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_Combine::EyePosition( void ) 
{
	if ( !IsCrouching() )
	{
		return GetAbsOrigin() + COMBINE_EYE_STANDING_POSITION;
	}
	else
	{
		return GetAbsOrigin() + COMBINE_EYE_CROUCHING_POSITION;
	}

	/*
	Vector m_EyePos;
	GetAttachment( "eyes", m_EyePos );
	return m_EyePos;
	*/
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_Combine::GetAltFireTarget()
{
	Assert( IsElite() );

	return m_vecAltFireTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nActivity - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_Combine::EyeOffset( Activity nActivity )
{
	if (CapabilitiesGet() & bits_CAP_DUCK)
	{
		if ( IsCrouchedActivity( nActivity ) )
			return COMBINE_EYE_CROUCHING_POSITION;

	}
	// if the hint doesn't tell anything, assume current state
	if ( !IsCrouching() )
	{
		return COMBINE_EYE_STANDING_POSITION;
	}
	else
	{
		return COMBINE_EYE_CROUCHING_POSITION;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_Combine::GetCrouchEyeOffset( void )
{
	return COMBINE_EYE_CROUCHING_POSITION;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::SetActivity( Activity NewActivity )
{
	BaseClass::SetActivity( NewActivity );

	m_iLastAnimEventHandled = -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
NPC_STATE CNPC_Combine::SelectIdealState( void )
{
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL )
			{
				if ( !HasCondition( COND_ENEMY_DEAD ) )
				{
					// Lost track of my enemy. Patrol.
					SetCondition( COND_COMBINE_SHOULD_PATROL );
				}
				return NPC_STATE_ALERT;
			}
			else if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				AnnounceEnemyKill(GetEnemy());
			}
		}

	default:
		{
			return BaseClass::SelectIdealState();
		}
	}

	return GetIdealState();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true; // already have the slot I need

		if( !HasStrategySlotRange( SQUAD_SLOT_GRENADE1, SQUAD_SLOT_ATTACK_OCCLUDER ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WeaponProficiency_t CNPC_Combine::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_ar2" ) )
	{
		if( hl2_episodic.GetBool() )
		{
			return WEAPON_PROFICIENCY_VERY_GOOD;
		}
		else
		{
			return WEAPON_PROFICIENCY_GOOD;
		}
	}
	else if( FClassnameIs( pWeapon, "weapon_shotgun" )	)
	{
		if( m_nSkin != COMBINE_SKIN_SHOTGUNNER )
		{
			m_nSkin = COMBINE_SKIN_SHOTGUNNER;
		}

		return WEAPON_PROFICIENCY_PERFECT;
	}
	else if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_GOOD;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::HasShotgun()
{
	if( GetActiveWeapon() && GetActiveWeapon()->m_iClassname == s_iszShotgunClassname )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Only supports weapons that use clips.
//-----------------------------------------------------------------------------
bool CNPC_Combine::ActiveWeaponIsFullyLoaded()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();

	if( !pWeapon )
		return false;

	if( !pWeapon->UsesClipsForAmmo1() )
		return false;

	return ( pWeapon->Clip1() >= pWeapon->GetMaxClip1() );
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  The type of interaction, extra info pointer, and who started it
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Combine::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt)
{
	if ( interactionType == g_interactionTurretStillStanding )
	{
		// A turret that I've kicked recently is still standing 5 seconds later. 
		if ( sourceEnt == GetEnemy() )
		{
			// It's still my enemy. Time to grenade it.
			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			m_vecTossVelocity = forward * 10;
			SetCondition( COND_COMBINE_DROP_GRENADE );
			ClearSchedule( "Failed to kick over turret" );
		}
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char* CNPC_Combine::GetSquadSlotDebugName( int iSquadSlot )
{
	switch( iSquadSlot )
	{
	case SQUAD_SLOT_GRENADE1:			return "SQUAD_SLOT_GRENADE1";	
		break;
	case SQUAD_SLOT_GRENADE2:			return "SQUAD_SLOT_GRENADE2";	
		break;
	case SQUAD_SLOT_ATTACK_OCCLUDER:	return "SQUAD_SLOT_ATTACK_OCCLUDER";	
		break;
	case SQUAD_SLOT_OVERWATCH:			return "SQUAD_SLOT_OVERWATCH";
		break;
	}

	return BaseClass::GetSquadSlotDebugName( iSquadSlot );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::IsUsingTacticalVariant( int variant )
{
	if( variant == TACTICAL_VARIANT_PRESSURE_ENEMY && m_iTacticalVariant == TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE )
	{
		// Essentially, fib. Just say that we are a 'pressure enemy' soldier.
		return true;
	}

	return m_iTacticalVariant == variant;
}

//-----------------------------------------------------------------------------
// For the purpose of determining whether to use a pathfinding variant, this
// function determines whether the current schedule is a schedule that 
// 'approaches' the enemy. 
//-----------------------------------------------------------------------------
bool CNPC_Combine::IsRunningApproachEnemySchedule()
{
	if( IsCurSchedule( SCHED_CHASE_ENEMY ) )
		return true;

	if( IsCurSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ) )
		return true;

	if( IsCurSchedule( SCHED_COMBINE_PRESS_ATTACK, false ) )
		return true;

	return false;
}

bool CNPC_Combine::ShouldPickADeathPose( void ) 
{ 
	return !IsCrouching(); 
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_combine, CNPC_Combine )

//Tasks
DECLARE_TASK( TASK_COMBINE_FACE_TOSS_DIR )
DECLARE_TASK( TASK_COMBINE_IGNORE_ATTACKS )
DECLARE_TASK( TASK_COMBINE_SIGNAL_BEST_SOUND )
DECLARE_TASK( TASK_COMBINE_DEFER_SQUAD_GRENADES )
DECLARE_TASK( TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY )
DECLARE_TASK( TASK_COMBINE_DIE_INSTANTLY )
DECLARE_TASK( TASK_COMBINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET )
DECLARE_TASK( TASK_COMBINE_GET_PATH_TO_FORCED_GREN_LOS )
DECLARE_TASK( TASK_COMBINE_SET_STANDING )

//Activities
DECLARE_ACTIVITY( ACT_COMBINE_THROW_GRENADE )
DECLARE_ACTIVITY( ACT_COMBINE_LAUNCH_GRENADE )
DECLARE_ACTIVITY( ACT_COMBINE_BUGBAIT )
DECLARE_ACTIVITY( ACT_COMBINE_AR2_ALTFIRE )
DECLARE_ACTIVITY( ACT_WALK_EASY )
DECLARE_ACTIVITY( ACT_WALK_MARCH )

DECLARE_ANIMEVENT( COMBINE_AE_BEGIN_ALTFIRE )
DECLARE_ANIMEVENT( COMBINE_AE_ALTFIRE )

DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE1 )
DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE2 )

DECLARE_CONDITION( COND_COMBINE_NO_FIRE )
DECLARE_CONDITION( COND_COMBINE_DEAD_FRIEND )
DECLARE_CONDITION( COND_COMBINE_SHOULD_PATROL )
DECLARE_CONDITION( COND_COMBINE_HIT_BY_BUGBAIT )
DECLARE_CONDITION( COND_COMBINE_DROP_GRENADE )
DECLARE_CONDITION( COND_COMBINE_ON_FIRE )
DECLARE_CONDITION( COND_COMBINE_ATTACK_SLOT_AVAILABLE )

DECLARE_INTERACTION( g_interactionCombineBash );

//=========================================================
// SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND
//
//	hide from the loudest sound source (to run from grenade)
//=========================================================
DEFINE_SCHEDULE	
(
 SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND,

 "	Tasks"
 "		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND"
 "		 TASK_STOP_MOVING					0"
 "		 TASK_COMBINE_SIGNAL_BEST_SOUND		0"
 "		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
 "		 TASK_RUN_PATH						0"
 "		 TASK_WAIT_FOR_MOVEMENT				0"
 "		 TASK_REMEMBER						MEMORY:INCOVER"
 "		 TASK_FACE_REASONABLE				0"
 ""
 "	Interrupts"
 )

 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND,

 "	Tasks"
 "		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_COWER"
 "		 TASK_GET_PATH_AWAY_FROM_BEST_SOUND		600"
 "		 TASK_RUN_PATH_TIMED					2"
 "		 TASK_STOP_MOVING						0"
 ""
 "	Interrupts"
 )
 //=========================================================
 //	SCHED_COMBINE_COMBAT_FAIL
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_COMBAT_FAIL,

 "	Tasks"
 "		TASK_STOP_MOVING			0"
 "		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
 "		TASK_WAIT_FACE_ENEMY		2"
 "		TASK_WAIT_PVS				0"
 ""
 "	Interrupts"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 )

 //=========================================================
 // SCHED_COMBINE_VICTORY_DANCE
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_VICTORY_DANCE,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_FACE_ENEMY						0"
 "		TASK_WAIT							1.5"
 "		TASK_GET_PATH_TO_ENEMY_CORPSE		0"
 "		TASK_WALK_PATH						0"
 "		TASK_WAIT_FOR_MOVEMENT				0"
 "		TASK_FACE_ENEMY						0"
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_VICTORY_DANCE"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 )

 //=========================================================
 // SCHED_COMBINE_ASSAULT
 //=========================================================
 DEFINE_SCHEDULE 
 (
 SCHED_COMBINE_ASSAULT,

 "	Tasks "
 "		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE"
 "		TASK_SET_TOLERANCE_DISTANCE		48"
 "		TASK_GET_PATH_TO_ENEMY_LKP		0"
 "		TASK_COMBINE_IGNORE_ATTACKS		0.2"
 "		TASK_SPEAK_SENTENCE				0"
 "		TASK_RUN_PATH					0"
 //		"		TASK_COMBINE_MOVE_AND_AIM		0"
 "		TASK_WAIT_FOR_MOVEMENT			0"
 "		TASK_COMBINE_IGNORE_ATTACKS		0.0"
 ""
 "	Interrupts "
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_ENEMY_UNREACHABLE"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_TOO_FAR_TO_ATTACK"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 )

 DEFINE_SCHEDULE 
 (
 SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE,

 "	Tasks "
 "		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
 "		TASK_SET_TOLERANCE_DISTANCE		48"
 "		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
 "		TASK_COMBINE_SET_STANDING		1"
 "		TASK_SPEAK_SENTENCE				1"
 "		TASK_RUN_PATH					0"
 "		TASK_WAIT_FOR_MOVEMENT			0"
 "		TASK_COMBINE_IGNORE_ATTACKS		0.0"
 "		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
 "	"
 "	Interrupts "
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 //"		COND_CAN_RANGE_ATTACK1"
 //"		COND_CAN_RANGE_ATTACK2"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_HEAVY_DAMAGE"
 )

 //=========================================================
 // SCHED_COMBINE_PRESS_ATTACK
 //=========================================================
 DEFINE_SCHEDULE 
 (
 SCHED_COMBINE_PRESS_ATTACK,

 "	Tasks "
 "		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE"
 "		TASK_SET_TOLERANCE_DISTANCE		72"
 "		TASK_GET_PATH_TO_ENEMY_LKP		0"
 "		TASK_COMBINE_SET_STANDING		1"
 "		TASK_RUN_PATH					0"
 "		TASK_WAIT_FOR_MOVEMENT			0"
 ""
 "	Interrupts "
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_ENEMY_UNREACHABLE"
 "		COND_NO_PRIMARY_AMMO"
 "		COND_LOW_PRIMARY_AMMO"
 "		COND_TOO_CLOSE_TO_ATTACK"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 )

 //=========================================================
 // SCHED_COMBINE_COMBAT_FACE
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_COMBAT_FACE,

 "	Tasks"
 "		TASK_STOP_MOVING			0"
 "		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
 "		TASK_FACE_ENEMY				0"
 "		 TASK_WAIT					1.5"
 //"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_SWEEP"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 )

 //=========================================================
 // 	SCHED_HIDE_AND_RELOAD	
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_HIDE_AND_RELOAD,

 "	Tasks"
 "		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_RELOAD"
 "		TASK_FIND_COVER_FROM_ENEMY	0"
 "		TASK_RUN_PATH				0"
 "		TASK_WAIT_FOR_MOVEMENT		0"
 "		TASK_REMEMBER				MEMORY:INCOVER"
 "		TASK_FACE_ENEMY				0"
 "		TASK_RELOAD					0"
 ""
 "	Interrupts"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_HEAVY_DAMAGE"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 )

 //=========================================================
 // SCHED_COMBINE_SIGNAL_SUPPRESS
 //	don't stop shooting until the clip is
 //	empty or combine gets hurt.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_SIGNAL_SUPPRESS,

 "	Tasks"
 "		TASK_STOP_MOVING				0"
 "		TASK_FACE_IDEAL					0"
 "		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_SIGNAL_GROUP"
 "		TASK_COMBINE_SET_STANDING		0"
 "		TASK_RANGE_ATTACK1				0"
 ""
 "	Interrupts"
 "		COND_ENEMY_DEAD"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 "		COND_NO_PRIMARY_AMMO"
 "		COND_WEAPON_BLOCKED_BY_FRIEND"
 "		COND_WEAPON_SIGHT_OCCLUDED"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_COMBINE_NO_FIRE"
 )

 //=========================================================
 // SCHED_COMBINE_SUPPRESS
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_SUPPRESS,

 "	Tasks"
 "		TASK_STOP_MOVING			0"
 "		TASK_FACE_ENEMY				0"
 "		TASK_COMBINE_SET_STANDING	0"
 "		TASK_RANGE_ATTACK1			0"
 ""
 "	Interrupts"
 "		COND_ENEMY_DEAD"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 "		COND_NO_PRIMARY_AMMO"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_COMBINE_NO_FIRE"
 "		COND_WEAPON_BLOCKED_BY_FRIEND"
 )

 //=========================================================
 // 	SCHED_COMBINE_ENTER_OVERWATCH
 //
 // Parks a combine soldier in place looking at the player's
 // last known position, ready to attack if the player pops out
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_ENTER_OVERWATCH,

 "	Tasks"
 "		TASK_STOP_MOVING			0"
 "		TASK_COMBINE_SET_STANDING	0"
 "		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
 "		TASK_FACE_ENEMY				0"
 "		TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_OVERWATCH"
 ""
 "	Interrupts"
 "		COND_HEAR_DANGER"
 "		COND_NEW_ENEMY"
 )

 //=========================================================
 // 	SCHED_COMBINE_OVERWATCH
 //
 // Parks a combine soldier in place looking at the player's
 // last known position, ready to attack if the player pops out
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_OVERWATCH,

 "	Tasks"
 "		TASK_WAIT_FACE_ENEMY		10"
 ""
 "	Interrupts"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_ENEMY_DEAD"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 "		COND_NO_PRIMARY_AMMO"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_NEW_ENEMY"
 )

 //=========================================================
 // SCHED_COMBINE_WAIT_IN_COVER
 //	we don't allow danger or the ability
 //	to attack to break a combine's run to cover schedule but
 //	when a combine is in cover we do want them to attack if they can.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_WAIT_IN_COVER,

 "	Tasks"
 "		TASK_STOP_MOVING				0"
 "		TASK_COMBINE_SET_STANDING		0"
 "		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Translated to cover
 "		TASK_WAIT_FACE_ENEMY			1"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_COMBINE_ATTACK_SLOT_AVAILABLE"
 )

 //=========================================================
 // SCHED_COMBINE_TAKE_COVER1
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_TAKE_COVER1  ,

 "	Tasks"
 "		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_COMBINE_TAKECOVER_FAILED"
 "		TASK_STOP_MOVING				0"
 "		TASK_WAIT					0.2"
 "		TASK_FIND_COVER_FROM_ENEMY	0"
 "		TASK_RUN_PATH				0"
 "		TASK_WAIT_FOR_MOVEMENT		0"
 "		TASK_REMEMBER				MEMORY:INCOVER"
 "		TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"
 ""
 "	Interrupts"
 )

 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_TAKECOVER_FAILED,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 ""
 "	Interrupts"
 )

 //=========================================================
 // SCHED_COMBINE_GRENADE_COVER1
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_GRENADE_COVER1,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_FIND_COVER_FROM_ENEMY			99"
 "		TASK_FIND_FAR_NODE_COVER_FROM_ENEMY	384"
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SPECIAL_ATTACK2"
 "		TASK_CLEAR_MOVE_WAIT				0"
 "		TASK_RUN_PATH						0"
 "		TASK_WAIT_FOR_MOVEMENT				0"
 "		TASK_SET_SCHEDULE					SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"
 ""
 "	Interrupts"
 )

 //=========================================================
 // SCHED_COMBINE_TOSS_GRENADE_COVER1
 //
 //	 drop grenade then run to cover.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_TOSS_GRENADE_COVER1,

 "	Tasks"
 "		TASK_FACE_ENEMY						0"
 "		TASK_RANGE_ATTACK2 					0"
 "		TASK_SET_SCHEDULE					SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
 ""
 "	Interrupts"
 )

 //=========================================================
 // SCHED_COMBINE_RANGE_ATTACK1
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_RANGE_ATTACK1,

 "	Tasks"
 "		TASK_STOP_MOVING				0"
 "		TASK_FACE_ENEMY					0"
 "		TASK_ANNOUNCE_ATTACK			1"	// 1 = primary attack
 "		TASK_WAIT_RANDOM				0.3"
 "		TASK_RANGE_ATTACK1				0"
 "		TASK_COMBINE_IGNORE_ATTACKS		0.5"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_HEAVY_DAMAGE"
 "		COND_LIGHT_DAMAGE"
 "		COND_LOW_PRIMARY_AMMO"
 "		COND_NO_PRIMARY_AMMO"
 "		COND_WEAPON_BLOCKED_BY_FRIEND"
 "		COND_TOO_CLOSE_TO_ATTACK"
 "		COND_GIVE_WAY"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_COMBINE_NO_FIRE"
 ""
 // Enemy_Occluded				Don't interrupt on this.  Means
 //								comibine will fire where player was after
 //								he has moved for a little while.  Good effect!!
 // WEAPON_SIGHT_OCCLUDED		Don't block on this! Looks better for railings, etc.
 )

 //=========================================================
 // AR2 Alt Fire Attack
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_AR2_ALTFIRE,

 "	Tasks"
 "		TASK_STOP_MOVING									0"
 "		TASK_ANNOUNCE_ATTACK								1"
 "		TASK_COMBINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET		ACTIVITY:ACT_COMBINE_AR2_ALTFIRE"
 ""
 "	Interrupts"
 )

 //=========================================================
 // Mapmaker forced grenade throw
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_FORCED_GRENADE_THROW,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_COMBINE_FACE_TOSS_DIR			0"
 "		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
 "		TASK_COMBINE_DEFER_SQUAD_GRENADES	0"
 ""
 "	Interrupts"
 )

 //=========================================================
 // Move to LOS of the mapmaker's forced grenade throw target
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_MOVE_TO_FORCED_GREN_LOS,

 "	Tasks "
 "		TASK_SET_TOLERANCE_DISTANCE					48"
 "		TASK_COMBINE_GET_PATH_TO_FORCED_GREN_LOS	0"
 "		TASK_SPEAK_SENTENCE							1"
 "		TASK_RUN_PATH								0"
 "		TASK_WAIT_FOR_MOVEMENT						0"
 "	"
 "	Interrupts "
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_HEAVY_DAMAGE"
 )

 //=========================================================
 // 	SCHED_COMBINE_RANGE_ATTACK2	
 //
 //	secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
 //	combines's grenade toss requires the enemy be occluded.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_RANGE_ATTACK2,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_COMBINE_FACE_TOSS_DIR			0"
 "		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
 "		TASK_COMBINE_DEFER_SQUAD_GRENADES	0"
 "		TASK_SET_SCHEDULE					SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"	// don't run immediately after throwing grenade.
 ""
 "	Interrupts"
 )


 //=========================================================
 // Throw a grenade, then run off and reload.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_GRENADE_AND_RELOAD,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_COMBINE_FACE_TOSS_DIR			0"
 "		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
 "		TASK_COMBINE_DEFER_SQUAD_GRENADES	0"
 "		TASK_SET_SCHEDULE					SCHEDULE:SCHED_HIDE_AND_RELOAD"	// don't run immediately after throwing grenade.
 ""
 "	Interrupts"
 )

 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_PATROL,

 "	Tasks"
 "		TASK_STOP_MOVING				0"
 "		TASK_WANDER						900540" 
 "		TASK_WALK_PATH					0"
 "		TASK_WAIT_FOR_MOVEMENT			0"
 "		TASK_STOP_MOVING				0"
 "		TASK_FACE_REASONABLE			0"
 "		TASK_WAIT						3"
 "		TASK_WAIT_RANDOM				3"
 "		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBINE_PATROL" // keep doing it
 ""
 "	Interrupts"
 "		COND_ENEMY_DEAD"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_NEW_ENEMY"
 "		COND_SEE_ENEMY"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 )

 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_BUGBAIT_DISTRACTION,

 "	Tasks"
 "		TASK_STOP_MOVING		0"
 "		TASK_RESET_ACTIVITY		0"
 "		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_COMBINE_BUGBAIT"
 ""
 "	Interrupts"
 ""
 )

 //=========================================================
 // SCHED_COMBINE_CHARGE_TURRET
 //
 //	Used to run straight at enemy turrets to knock them over.
 //  Prevents squadmates from throwing grenades during.
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_CHARGE_TURRET,

 "	Tasks"
 "		TASK_COMBINE_DEFER_SQUAD_GRENADES	0"
 "		TASK_STOP_MOVING					0"
 "		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
 "		TASK_GET_CHASE_PATH_TO_ENEMY		300"
 "		TASK_RUN_PATH						0"
 "		TASK_WAIT_FOR_MOVEMENT				0"
 "		TASK_FACE_ENEMY						0"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_ENEMY_UNREACHABLE"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_TOO_CLOSE_TO_ATTACK"
 "		COND_TASK_FAILED"
 "		COND_LOST_ENEMY"
 "		COND_BETTER_WEAPON_AVAILABLE"
 "		COND_HEAR_DANGER"
 )

 //=========================================================
 // SCHED_COMBINE_CHARGE_PLAYER
 //
 //	Used to run straight at enemy player since physgun combat
 //  is more fun when the enemies are close
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_CHARGE_PLAYER,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
 "		TASK_COMBINE_CHASE_ENEMY_CONTINUOUSLY		192"
 "		TASK_FACE_ENEMY						0"
 ""
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_ENEMY_UNREACHABLE"
 "		COND_CAN_MELEE_ATTACK1"
 "		COND_CAN_MELEE_ATTACK2"
 "		COND_TASK_FAILED"
 "		COND_LOST_ENEMY"
 "		COND_HEAR_DANGER"
 )

 //=========================================================
 // SCHED_COMBINE_DROP_GRENADE
 //
 //	Place a grenade at my feet
 //=========================================================
 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_DROP_GRENADE,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SPECIAL_ATTACK2"
 "		TASK_FIND_COVER_FROM_ENEMY			99"
 "		TASK_FIND_FAR_NODE_COVER_FROM_ENEMY	384"
 "		TASK_CLEAR_MOVE_WAIT				0"
 "		TASK_RUN_PATH						0"
 "		TASK_WAIT_FOR_MOVEMENT				0"
 ""
 "	Interrupts"
 )

 //=========================================================
 // SCHED_COMBINE_PATROL_ENEMY
 //
 // Used instead if SCHED_COMBINE_PATROL if I have an enemy.
 // Wait for the enemy a bit in the hopes of ambushing him.
 //=========================================================
 DEFINE_SCHEDULE	
 (
 SCHED_COMBINE_PATROL_ENEMY,

 "	Tasks"
 "		TASK_STOP_MOVING					0"
 "		TASK_WAIT_FACE_ENEMY				1" 
 "		TASK_WAIT_FACE_ENEMY_RANDOM			3" 
 ""
 "	Interrupts"
 "		COND_ENEMY_DEAD"
 "		COND_LIGHT_DAMAGE"
 "		COND_HEAVY_DAMAGE"
 "		COND_HEAR_DANGER"
 "		COND_HEAR_MOVE_AWAY"
 "		COND_NEW_ENEMY"
 "		COND_SEE_ENEMY"
 "		COND_CAN_RANGE_ATTACK1"
 "		COND_CAN_RANGE_ATTACK2"
 )

 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_BURNING_STAND,

 "	Tasks"
 "		TASK_SET_ACTIVITY				ACTIVITY:ACT_COMBINE_BUGBAIT"
 "		TASK_RANDOMIZE_FRAMERATE		20"
 "		TASK_WAIT						2"
 "		TASK_WAIT_RANDOM				3"
 "		TASK_COMBINE_DIE_INSTANTLY		DMG_BURN"
 "		TASK_WAIT						1.0"
 "	"
 "	Interrupts"
 )

 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_FACE_IDEAL_YAW,

 "	Tasks"
 "		TASK_FACE_IDEAL				0"
 "	"
 "	Interrupts"
 )

 DEFINE_SCHEDULE
 (
 SCHED_COMBINE_MOVE_TO_MELEE,

 "	Tasks"
 "		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
 "		TASK_GET_PATH_TO_SAVEPOSITION				0"
 "		TASK_RUN_PATH								0"
 "		TASK_WAIT_FOR_MOVEMENT						0"
  "	"
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_ENEMY_DEAD"
 "		COND_CAN_MELEE_ATTACK1"
 )

 AI_END_CUSTOM_NPC()
