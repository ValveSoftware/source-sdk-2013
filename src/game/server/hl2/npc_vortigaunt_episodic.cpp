//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Vortigaunt - now much friendlier!
//
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "globalstate.h"
#include "npcevent.h"
#include "Sprite.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"
#include "ai_squadslot.h"
#include "npc_antlion.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "ai_senses.h"

#include "npc_vortigaunt_episodic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HAND_LEFT	0
#define HAND_RIGHT	1
#define HAND_BOTH	2

class CVortigauntChargeToken;

#define	VORTIGAUNT_LIMP_HEALTH				20
#define	VORTIGAUNT_SENTENCE_VOLUME			(float)0.35 // volume of vortigaunt sentences
#define	VORTIGAUNT_VOL						0.35		// volume of vortigaunt sounds
#define	VORTIGAUNT_ATTN						ATTN_NORM	// attenutation of vortigaunt sentences
#define	VORTIGAUNT_HEAL_RECHARGE			30.0		// How long to rest between heals
#define	VORTIGAUNT_ZAP_GLOWGROW_TIME		0.5			// How long does glow last
#define	VORTIGAUNT_HEAL_GLOWGROW_TIME		1.4			// How long does glow last
#define	VORTIGAUNT_GLOWFADE_TIME			0.5			// How long does it fade
#define VORTIGAUNT_CURE_LIFESPAN			8.0			// cure tokens only live this long (so they don't get stuck on geometry)

#define	VORTIGAUNT_BLUE_FADE_TIME			2.25f		// takes this long to fade from green to blue or back

#define VORTIGAUNT_FEAR_ZOMBIE_DIST_SQR		Square(60)	// back away from zombies inside this radius

#define	PLAYER_CRITICAL_HEALTH_PERC			0.15f

#define TLK_SQUISHED_GRUB "TLK_SQUISHED_GRUB" // response rule for vortigaunts when they step on a grub

static const char *VORTIGAUNT_LEFT_CLAW = "leftclaw";
static const char *VORTIGAUNT_RIGHT_CLAW = "rightclaw";

static const char *VORT_CURE = "VORT_CURE";
static const char *VORT_CURESTOP = "VORT_CURESTOP";
static const char *VORT_CURE_INTERRUPT = "VORT_CURE_INTERRUPT";
static const char *VORT_ATTACK = "VORT_ATTACK";
static const char *VORT_MAD = "VORT_MAD";
static const char *VORT_SHOT = "VORT_SHOT";
static const char *VORT_PAIN = "VORT_PAIN";
static const char *VORT_DIE = "VORT_DIE";
static const char *VORT_KILL = "VORT_KILL";
static const char *VORT_LINE_FIRE = "VORT_LINE_FIRE";
static const char *VORT_POK = "VORT_POK";
static const char *VORT_EXTRACT_START = "VORT_EXTRACT_START";
static const char *VORT_EXTRACT_FINISH = "VORT_EXTRACT_FINISH";

// Target must be within this range to heal
#define	HEAL_RANGE			(40*12) //ft
#define	HEAL_SEARCH_RANGE	(40*12) //ft

ConVar sk_vortigaunt_armor_charge( "sk_vortigaunt_armor_charge","30");
ConVar sk_vortigaunt_armor_charge_per_token( "sk_vortigaunt_armor_charge_per_token","5");

ConVar sk_vortigaunt_health( "sk_vortigaunt_health","0");
ConVar sk_vortigaunt_dmg_claw( "sk_vortigaunt_dmg_claw","0");
ConVar sk_vortigaunt_dmg_rake( "sk_vortigaunt_dmg_rake","0");
ConVar sk_vortigaunt_dmg_zap( "sk_vortigaunt_dmg_zap","0");
ConVar sk_vortigaunt_zap_range( "sk_vortigaunt_zap_range", "100", FCVAR_NONE, "Range of vortigaunt's ranged attack (feet)" );
ConVar sk_vortigaunt_vital_antlion_worker_dmg("sk_vortigaunt_vital_antlion_worker_dmg", "0.2", FCVAR_NONE, "Vital-ally vortigaunts scale damage taken from antlion workers by this amount." );

ConVar g_debug_vortigaunt_aim( "g_debug_vortigaunt_aim", "0" );

// FIXME: Move to shared code!
#define VORTFX_ZAPBEAM		0	// Beam that damages the target
#define VORTFX_ARMBEAM		1	// Smaller beams from the hands as we charge up

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionVortigauntStomp		= 0;
int	g_interactionVortigauntStompFail	= 0;
int	g_interactionVortigauntStompHit		= 0;
int	g_interactionVortigauntKick			= 0;
int	g_interactionVortigauntClaw			= 0;

//=========================================================
// Vortigaunt activities
//=========================================================
int	ACT_VORTIGAUNT_AIM;
int ACT_VORTIGAUNT_START_HEAL;
int ACT_VORTIGAUNT_HEAL_LOOP;
int ACT_VORTIGAUNT_END_HEAL;
int ACT_VORTIGAUNT_TO_ACTION;
int ACT_VORTIGAUNT_TO_IDLE;
int ACT_VORTIGAUNT_HEAL; // Heal gesture
int ACT_VORTIGAUNT_DISPEL;
int ACT_VORTIGAUNT_ANTLION_THROW;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
int AE_VORTIGAUNT_CLAW_LEFT;
int AE_VORTIGAUNT_CLAW_RIGHT;
int AE_VORTIGAUNT_ZAP_POWERUP;
int AE_VORTIGAUNT_ZAP_SHOOT;
int AE_VORTIGAUNT_ZAP_DONE;
int AE_VORTIGAUNT_HEAL_STARTGLOW;
int AE_VORTIGAUNT_HEAL_STARTBEAMS;
int AE_VORTIGAUNT_HEAL_STARTSOUND;
int AE_VORTIGAUNT_SWING_SOUND;
int AE_VORTIGAUNT_SHOOT_SOUNDSTART;
int AE_VORTIGAUNT_HEAL_PAUSE;

int AE_VORTIGAUNT_START_DISPEL;	// Start the warm-up
int AE_VORTIGAUNT_ACCEL_DISPEL;	// Indicates we're ramping up
int AE_VORTIGAUNT_DISPEL;		// Actual blast

int AE_VORTIGAUNT_START_HURT_GLOW;	// Start the hurt handglow: 0=left, 1=right
int AE_VORTIGAUNT_STOP_HURT_GLOW;	// Turn off the hurt handglow: 0=left, 1=right

int	AE_VORTIGAUNT_START_HEAL_GLOW;	// 0 - Left, 1 - Right
int AE_VORTIGAUNT_STOP_HEAL_GLOW;	// '

//-----------------------------------------------------------------------------
// Squad slots
//-----------------------------------------------------------------------------
enum SquadSlot_T
{	
	SQUAD_SLOT_HEAL_PLAYER = LAST_SHARED_SQUADSLOT,
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Vortigaunt )

	DEFINE_FIELD( m_eHealState,				FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextHealTokenTime,	FIELD_TIME ),
	DEFINE_ARRAY( m_hHandEffect,			FIELD_EHANDLE, 2 ),
	DEFINE_FIELD( m_flNextHealTime,			FIELD_TIME ),
	DEFINE_FIELD( m_bPlayerRequestedHeal,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDispelTestTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHealHinderedTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nLightningSprite,		FIELD_INTEGER),
	DEFINE_FIELD( m_fGlowAge,				FIELD_FLOAT),
	DEFINE_FIELD( m_fGlowChangeTime,		FIELD_TIME),
	DEFINE_FIELD( m_bGlowTurningOn,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_nCurGlowIndex,			FIELD_INTEGER),
	DEFINE_FIELD( m_flNextHealTime,			FIELD_TIME),
	DEFINE_FIELD( m_flPainTime,				FIELD_TIME),
	DEFINE_FIELD( m_nextLineFireTime,		FIELD_TIME),
	DEFINE_KEYFIELD( m_bArmorRechargeEnabled,FIELD_BOOLEAN, "ArmorRechargeEnabled" ),
	DEFINE_FIELD( m_bForceArmorRecharge,	FIELD_BOOLEAN),
	DEFINE_FIELD( m_bExtractingBugbait,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_iLeftHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iRightHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hHealTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_flBlueEndFadeTime,		FIELD_TIME ),
	DEFINE_FIELD( m_bIsBlue,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsBlack,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flAimDelay,				FIELD_TIME ),
	DEFINE_FIELD( m_bCarryingNPC,			FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bRegenerateHealth,	FIELD_BOOLEAN, "HealthRegenerateEnabled" ),

	// m_AssaultBehavior	(auto saved by AI)
	// m_LeadBehavior
	// DEFINE_FIELD( m_bStopLoopingSounds, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_USEFUNC( Use ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableArmorRecharge",	InputEnableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableArmorRecharge", InputDisableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"ChargeTarget",			InputChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ExtractBugbait",		InputExtractBugbait ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableHealthRegeneration",	InputEnableHealthRegeneration ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableHealthRegeneration",InputDisableHealthRegeneration ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Dispel",				InputDispel ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"BeginCarryNPC",		InputBeginCarryNPC ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EndCarryNPC",			InputEndCarryNPC ),

	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "TurnBlue", InputTurnBlue ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "TurnBlack", InputTurnBlack ),

	// Outputs
	DEFINE_OUTPUT(m_OnFinishedExtractingBugbait,	"OnFinishedExtractingBugbait"),
	DEFINE_OUTPUT(m_OnFinishedChargingTarget,		"OnFinishedChargingTarget"),
	DEFINE_OUTPUT(m_OnPlayerUse,					"OnPlayerUse" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_vortigaunt, CNPC_Vortigaunt );

IMPLEMENT_SERVERCLASS_ST( CNPC_Vortigaunt, DT_NPC_Vortigaunt )
	SendPropTime( SENDINFO (m_flBlueEndFadeTime ) ),
	SendPropBool( SENDINFO( m_bIsBlue )),
	SendPropBool( SENDINFO ( m_bIsBlack ) ),
END_SEND_TABLE()

// for special behavior with rollermines
static bool IsRoller( CBaseEntity *pRoller )
{
	return FClassnameIs( pRoller, "npc_rollermine" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_Vortigaunt::CNPC_Vortigaunt( void ) : 
m_bPlayerRequestedHeal( false ),
m_flNextHealTime( 3.0f ), // Let the player settle before we decide to do this
m_nNumTokensToSpawn( 0 ),
m_flAimDelay( 0.0f ),
m_eHealState( HEAL_STATE_NONE )
{
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the player is below a certain percentage
//		    of their maximum health
// Input  : flPerc - Percentage to test against
// Output : Returns true if less than supplied parameter
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::PlayerBelowHealthPercentage( CBasePlayer *pPlayer, float flPerc )
{
	if ( pPlayer == NULL )
		return false;

	if ( pPlayer->ArmorValue() )
		return false;

	float flMaxHealth = pPlayer->GetMaxHealth();
	if ( flMaxHealth == 0.0f )
		return false;

	float flHealthPerc = (flMaxHealth != 0 ) ? (float) pPlayer->GetHealth() / flMaxHealth : 0.0f;
	return ( flHealthPerc <= flPerc );
}

#define	VORT_START_EXTRACT_SENTENCE		500
#define VORT_FINISH_EXTRACT_SENTENCE	501

//------------------------------------------------------------------------------
// Purpose: Make the vort speak a line
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::SpeakSentence( int sentenceType )
{
	if (sentenceType == VORT_START_EXTRACT_SENTENCE)
	{
		Speak( VORT_EXTRACT_START );
	}
	else if (sentenceType == VORT_FINISH_EXTRACT_SENTENCE)
	{
		Speak( VORT_EXTRACT_FINISH );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{

	case TASK_ANNOUNCE_ATTACK:
		{
			// We override this to add our innate weapon
			if ( m_AnnounceAttackTimer.Expired() )
			{
				if ( SpeakIfAllowed( TLK_ATTACKING, "attacking_with_weapon:zap" ) )
				{
					m_AnnounceAttackTimer.Set( 10, 30 );
				}
			}

			BaseClass::StartTask( pTask );
			break;
		}

	// Sets our target to the entity that we cached earlier.
	case TASK_VORTIGAUNT_GET_HEAL_TARGET:
	{
		if ( m_hHealTarget == NULL )
		{
			TaskFail( FAIL_NO_TARGET );
		}
		else
		{
			SetTarget( m_hHealTarget );
			TaskComplete();
		}
		
		break;
	}
	
	case TASK_VORTIGAUNT_EXTRACT_WARMUP:
	{
		ResetIdealActivity( (Activity) ACT_VORTIGAUNT_TO_ACTION );
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT:
	{
		SetActivity( (Activity) ACT_RANGE_ATTACK1 );
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT_COOLDOWN:
	{
		ResetIdealActivity( (Activity)ACT_VORTIGAUNT_TO_IDLE );
		break;
	}

	case TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT:
	{
		// Cheat, and fire both outputs
		m_OnFinishedExtractingBugbait.FireOutput( this, this );
		TaskComplete();
		break;
	}

	case TASK_VORTIGAUNT_HEAL:
	{
		// Start the layer up and give it a higher priority than normal
		int nLayer = AddGesture( (Activity) ACT_VORTIGAUNT_HEAL );
		SetLayerPriority( nLayer, 1.0f );

		m_eHealState = HEAL_STATE_WARMUP;
		
		CBasePlayer *pPlayer = ToBasePlayer( m_hHealTarget );
		if ( pPlayer == NULL )
		{
			TaskFail( "NULL Player in heal schedule!\n" );
			return;
		}

		// Figure out how many tokens to spawn
		float flArmorDelta = (float) sk_vortigaunt_armor_charge.GetInt() - pPlayer->ArmorValue();
		m_nNumTokensToSpawn = ceil( flArmorDelta / sk_vortigaunt_armor_charge_per_token.GetInt() );
		
		// If we're forced to recharge, then at least send one
		if ( m_bForceArmorRecharge && m_nNumTokensToSpawn <= 0 )
			m_nNumTokensToSpawn = 1;
		
		TaskComplete();
		break;
	}

	case TASK_VORTIGAUNT_WAIT_FOR_PLAYER:
	{
		// Wait for the player to get near (before starting the bugbait sequence)
		break;
	}

	case TASK_VORTIGAUNT_DISPEL_ANTLIONS:
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( TLK_VORTIGAUNT_DISPEL );
		}

		ResetIdealActivity( (Activity) ACT_VORTIGAUNT_DISPEL );
		break;
	}

	default:
		{
			BaseClass::StartTask( pTask );	
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			// If our enemy is gone, dead or out of sight, pick a new one (only if we're not delaying this behavior)
			if ( ( HasCondition( COND_ENEMY_OCCLUDED ) ||
				 GetEnemy() == NULL ||
				 GetEnemy()->IsAlive() == false ) &&
				 m_flAimDelay < gpGlobals->curtime )
			{
				CBaseEntity *pNewEnemy = BestEnemy();
				if ( pNewEnemy != NULL )
				{
					SetEnemy( pNewEnemy );
					SetState( NPC_STATE_COMBAT );
				}
			}

			BaseClass::RunTask( pTask );
			break;
		}
	
	case TASK_VORTIGAUNT_EXTRACT_WARMUP:
	{
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}
	
	case TASK_VORTIGAUNT_EXTRACT:
	{
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT_COOLDOWN:
	{
		if ( IsActivityFinished() )
		{
			m_bExtractingBugbait = false;
			TaskComplete();
		}
		break;
	}

	case TASK_VORTIGAUNT_WAIT_FOR_PLAYER:
	{
		// Wait for the player to get near (before starting the bugbait sequence)
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer != NULL )
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
			SetTurnActivity();
			if ( GetMotor()->DeltaIdealYaw() < 10 )
			{
				// Wait for the player to get close enough
				if ( ( GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr() < Square(32*12) )
				{
					TaskComplete();
				}
			}
		}
		else
		{
			TaskFail( FAIL_NO_PLAYER );
		}
		break;
	}

	case TASK_VORTIGAUNT_DISPEL_ANTLIONS:
	{
		if ( IsSequenceFinished() )
		{
			TaskComplete();
		}

		break;
	}

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::AlertSound( void )
{
	if ( GetEnemy() != NULL && IsOkToCombatSpeak() )
	{
		Speak( VORT_ATTACK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows each sequence to have a different turn rate associated with it.
// Output : float CNPC_Vortigaunt::MaxYawSpeed
//-----------------------------------------------------------------------------
float CNPC_Vortigaunt::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 35;
		break;
	case ACT_WALK:
		return 35;
		break;
	case ACT_RUN:
		return 45;
		break;
	default:
		return 35;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Normal facing position is the eyes, but with the vort eyes on such a
//			long swing arm, this causes stability issues when an npc is trying to
//			face a vort that's also turning their head
// Output : 
//-----------------------------------------------------------------------------
Vector  CNPC_Vortigaunt::FacingPosition( void )
{
	return WorldSpaceCenter();
}


//-----------------------------------------------------------------------------
// Purpose: Normal body target is the mid-point between the center and the eyes, but
//			the vort's eyes are so far offset, that this is usually in the middle of 
//			empty space
// Output : 
//-----------------------------------------------------------------------------

Vector CNPC_Vortigaunt::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	Vector low = WorldSpaceCenter() - ( WorldSpaceCenter() - GetAbsOrigin() ) * .25;

	Vector high;
	int iBone = LookupBone( "ValveBiped.neck1" );
	if (iBone >= 0)
	{
		QAngle angHigh;
		GetBonePosition( iBone, high, angHigh );
	}
	else
	{
		high = WorldSpaceCenter();
	}

	Vector delta = high - low;
	Vector result;
	if ( bNoisy )
	{
		// bell curve
		float rand1 = random->RandomFloat( 0.0, 0.5 );
		float rand2 = random->RandomFloat( 0.0, 0.5 );
		result = low + delta * rand1 + delta * rand2;
	}
	else
		result = low + delta * 0.5; 

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Try a more predictive approach
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	// Try and figure out a rough idea of where we'll be after a certain time delta and base our
	// conditions on that instead.  This is necessary because the vortigaunt takes a long time to
	// deliver his attack and looks very strange if he starts to attack when he'd never be able to hit
	// due to movement.

	const float flTimeDelta = 0.5f;
	Vector vecNewOwnerPos;
	Vector vecNewTargetPos;
	UTIL_PredictedPosition( this, flTimeDelta, &vecNewOwnerPos );
	UTIL_PredictedPosition( GetEnemy(), flTimeDelta, &vecNewTargetPos );

	Vector vecDelta = vecNewTargetPos - GetEnemy()->GetAbsOrigin();
	Vector vecFinalTargetPos = GetEnemy()->BodyTarget( vecNewOwnerPos ) + vecDelta;

	// Debug data
	/*
	NDebugOverlay::Box( GetEnemy()->BodyTarget( vecNewOwnerPos ), -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, 0, 3.0f );
	NDebugOverlay::Box( vecFinalTargetPos, -Vector(4,4,4), Vector(4,4,4), 255, 255, 0, 0, 3.0f );
	NDebugOverlay::HorzArrow( GetEnemy()->BodyTarget( vecNewOwnerPos ), vecFinalTargetPos, 12.0f, 255, 0, 0, 16.0f, true, 3.0f );
	NDebugOverlay::HorzArrow( vecNewOwnerPos, GetEnemy()->BodyTarget( vecNewOwnerPos ), 8.0f, 255, 255, 0, 32.0f, true, 3.0f );
	*/

	return BaseClass::InnateWeaponLOSCondition( vecNewOwnerPos, vecFinalTargetPos, bSetConditions );
}

//------------------------------------------------------------------------------
// Purpose : For innate range attack
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy() == NULL )
		return COND_NONE;

	if ( gpGlobals->curtime < m_flNextAttack )
		return COND_NONE;

	// Don't do shooting while playing a scene
	if ( IsCurSchedule( SCHED_SCENE_GENERIC ) )
		return COND_NONE;

	// dvs: Allow up-close range attacks for episodic as the vort's melee
	// attack is rather ineffective.
#ifndef HL2_EPISODIC
	if ( flDist <= 70 )
	{
		return( COND_TOO_CLOSE_TO_ATTACK );
	}
	else
#else
	if ( flDist < 32.0f )
		return COND_TOO_CLOSE_TO_ATTACK;
#endif // HL2_EPISODIC
	if ( flDist > InnateRange1MaxRange() )
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

#ifdef HL2_EPISODIC

	// Do an extra check for workers near myself or the player
	if ( IsAntlionWorker( GetEnemy() ) )
	{
		// See if it's too close to me
		if ( ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).LengthSqr() < Square( AntlionWorkerBurstRadius() ) )
			return COND_TOO_CLOSE_TO_ATTACK;

		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer && ( pPlayer->GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).LengthSqr() < Square( AntlionWorkerBurstRadius() ) )
		{
			// Warn the player to get away!
			CFmtStrN<128> modifiers( "antlion_worker:true" );
			SpeakIfAllowed( TLK_DANGER, modifiers );
			return COND_NONE;
		}
	}

#endif // HL2_EPISODIC

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: Test for close-up dispel
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( m_flDispelTestTime > gpGlobals->curtime )
		return COND_NONE;

	m_flDispelTestTime = gpGlobals->curtime + 1.0f;

	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_ANTLION )
	{
		if ( NumAntlionsInRadius(128) > 3 )
		{
			m_flDispelTestTime = gpGlobals->curtime + 15.0f;
			return COND_VORTIGAUNT_DISPEL_ANTLIONS;
		}
	}

	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flRadius - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::NumAntlionsInRadius( float flRadius )
{
	CBaseEntity *sEnemySearch[16];
	int nNumAntlions = 0;
	int nNumEnemies = UTIL_EntitiesInBox( sEnemySearch, ARRAYSIZE(sEnemySearch), GetAbsOrigin()-Vector(flRadius,flRadius,flRadius), GetAbsOrigin()+Vector(flRadius,flRadius,flRadius), FL_NPC );
	for ( int i = 0; i < nNumEnemies; i++ )
	{
		// We only care about antlions
		if ( sEnemySearch[i] == NULL || sEnemySearch[i]->Classify() != CLASS_ANTLION )
			continue;

		nNumAntlions++;
	}

	return nNumAntlions;
}

//-----------------------------------------------------------------------------
// Purpose: Used for a more powerful, concussive blast
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::RangeAttack2Conditions( float flDot, float flDist )
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::HandleAnimEvent( animevent_t *pEvent )
{
	// Start our heal glows (choreo driven)
	if ( pEvent->event == AE_VORTIGAUNT_START_HEAL_GLOW )
	{
		StartHandGlow( VORTIGAUNT_BEAM_HEAL, atoi( pEvent->options ) );
		return;
	}
	
	// Stop our heal glows (choreo driven)
	if ( pEvent->event == AE_VORTIGAUNT_STOP_HEAL_GLOW )
	{
		EndHandGlow();
		return;
	}

	// Start our hurt glows (choreo driven)
	if ( pEvent->event == AE_VORTIGAUNT_START_HURT_GLOW )
	{
		StartHandGlow( VORTIGAUNT_BEAM_DISPEL, atoi( pEvent->options ) );
		return;
	}
	
	// Stop our hurt glows (choreo driven)
	if ( pEvent->event == AE_VORTIGAUNT_STOP_HURT_GLOW )
	{
		EndHandGlow();
		return;
	}

	// Start our dispel effect
	if ( pEvent->event == AE_VORTIGAUNT_START_DISPEL )
	{
		StartHandGlow( VORTIGAUNT_BEAM_DISPEL, HAND_LEFT );
		StartHandGlow( VORTIGAUNT_BEAM_DISPEL, HAND_RIGHT );

		// Boom!
		//EmitSound( "NPC_Vortigaunt.DispelImpact" );
		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.DispelImpact", params, NULL ) )
		{
			CPASAttenuationFilter filter( this );
			EmitSound_t ep( params );
			ep.m_nChannel = CHAN_BODY;
			EmitSound( filter, entindex(), ep );
		}
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_ACCEL_DISPEL )
	{
		// TODO: Increase the size?
		return;
	}
	
	// Kaboom!
	if ( pEvent->event == AE_VORTIGAUNT_DISPEL )
	{
		DispelAntlions( GetAbsOrigin(), 400.0f );
		return;
	}

	// Start of our heal loop
	if ( pEvent->event == AE_VORTIGAUNT_HEAL_PAUSE )
	{
		StartHealing();
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_ZAP_POWERUP )
	{
		if ( m_fGlowChangeTime > gpGlobals->curtime )
			return;

		int nHand = 0;
		if ( pEvent->options )
		{
			nHand = atoi( pEvent->options );
		}

		if ( ( nHand == HAND_LEFT ) || (nHand == HAND_BOTH ) )
		{
			ArmBeam( VORTIGAUNT_BEAM_ZAP, HAND_LEFT );
		}
		
		if ( ( nHand == HAND_RIGHT ) || (nHand == HAND_BOTH ) )
		{
			ArmBeam( VORTIGAUNT_BEAM_ZAP, HAND_RIGHT );
		}
		
		// Make hands glow if not already glowing
		if ( m_fGlowAge == 0 )
		{
			if ( ( nHand == HAND_LEFT ) || (nHand == HAND_BOTH ) )
			{
				StartHandGlow( VORTIGAUNT_BEAM_ZAP, HAND_LEFT );
			}
			 
			if ( ( nHand == HAND_RIGHT ) || (nHand == HAND_BOTH ) )
			{
				StartHandGlow( VORTIGAUNT_BEAM_ZAP, HAND_RIGHT );
			}
			m_fGlowAge = 1;
		}

		CPASAttenuationFilter filter( this );
		
		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.ZapPowerup", params, NULL ) )
		{
			EmitSound_t ep( params );
			//ep.m_nPitch = 100 + m_iBeams * 10;
			ep.m_nPitch = 150;
	
			EmitSound( filter, entindex(), ep );

			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_ZAP_SHOOT )
	{
		ClearBeams();

		ClearMultiDamage();

		int nHand = 0;
		if ( pEvent->options )
		{
			nHand = atoi( pEvent->options );
		}

		if ( ( nHand == HAND_LEFT ) || (nHand == HAND_BOTH ) )
		{
			ZapBeam( HAND_LEFT );
		}
		
		if ( ( nHand == HAND_RIGHT ) || (nHand == HAND_BOTH ) )
		{
			ZapBeam( HAND_RIGHT );
		}

		EndHandGlow();

		EmitSound( "NPC_Vortigaunt.ClawBeam" );
		m_bStopLoopingSounds = true;
		ApplyMultiDamage();

		// Suppress our aiming until we're done with the animation
		m_flAimDelay = gpGlobals->curtime + 0.75f;

		if ( m_bExtractingBugbait )
		{
			// Spawn bugbait!
			CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_bugbait" );
			if ( pWeapon )
			{
				// Starting above the body, spawn closer and closer to the vort until it's clear
				Vector vecSpawnOrigin = GetTarget()->WorldSpaceCenter() + Vector(0, 0, 32);
				int iNumAttempts = 4;
				Vector vecToVort = (WorldSpaceCenter() - vecSpawnOrigin);
				float flDistance = VectorNormalize( vecToVort ) / (iNumAttempts-1);
				int i = 0;
				for (; i < iNumAttempts; i++ )
				{
					trace_t tr;
					CTraceFilterSkipTwoEntities traceFilter( GetTarget(), this, COLLISION_GROUP_NONE );
					AI_TraceLine( vecSpawnOrigin, vecSpawnOrigin, MASK_SHOT, &traceFilter, &tr );

					if ( tr.fraction == 1.0 && !tr.m_pEnt )
					{
						// Make sure it can fit there
						AI_TraceHull( vecSpawnOrigin, vecSpawnOrigin, -Vector(16,16,16), Vector(16,16,48), MASK_SHOT, &traceFilter, &tr );
						if ( tr.fraction == 1.0 && !tr.m_pEnt )
							break;
					}

					//NDebugOverlay::Box( vecSpawnOrigin, pWeapon->WorldAlignMins(), pWeapon->WorldAlignMins(), 255,0,0, 64, 100 );

					// Move towards the vort
					vecSpawnOrigin = vecSpawnOrigin + (vecToVort * flDistance);
				}

				// HACK: If we've still failed, just spawn it on the player 
				if ( i == iNumAttempts )
				{
					CBasePlayer	*pPlayer = AI_GetSinglePlayer();
					if ( pPlayer )
					{
						vecSpawnOrigin = pPlayer->WorldSpaceCenter();
					}
				}

				//NDebugOverlay::Box( vecSpawnOrigin, -Vector(20,20,20), Vector(20,20,20), 0,255,0, 64, 100 );

				pWeapon->SetAbsOrigin( vecSpawnOrigin );
				pWeapon->Drop( Vector(0,0,1) );
			}

			CEffectData	data;
			
			data.m_vOrigin = GetTarget()->WorldSpaceCenter();
			data.m_vNormal = WorldSpaceCenter() - GetTarget()->WorldSpaceCenter();
			VectorNormalize( data.m_vNormal );
			
			data.m_flScale = 4;

			DispatchEffect( "AntlionGib", data );
		}

		// Stagger the next time we can attack
		m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 2.0f, 3.0f );
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_ZAP_DONE )
	{
		ClearBeams();
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTGLOW )
	{
		// Make hands glow
		StartHandGlow( VORTIGAUNT_BEAM_HEAL, HAND_RIGHT );
		m_eHealState = HEAL_STATE_WARMUP;
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTSOUND )
	{
		CPASAttenuationFilter filter( this );

		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.StartHealLoop", params, NULL ) )
		{
			EmitSound_t ep( params );
			//ep.m_nPitch = 100 + m_iBeams * 10;
			ep.m_nPitch = 150;

			EmitSound( filter, entindex(), ep );
			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_SWING_SOUND )
	{
		EmitSound( "NPC_Vortigaunt.Swing" );	
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_SHOOT_SOUNDSTART )
	{
		if ( m_fGlowChangeTime > gpGlobals->curtime )
			return;

		CPASAttenuationFilter filter( this );
		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.StartShootLoop", params, NULL ) )
		{
			EmitSound_t ep( params );
			//ep.m_nPitch = 100 + m_iBeams * 10;
			ep.m_nPitch = 150;

			EmitSound( filter, entindex(), ep );
			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_NPC_LEFTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepLeft", pEvent->eventtime );
		return;
	}

	if ( pEvent->event == AE_NPC_RIGHTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepRight", pEvent->eventtime );
		return;
	}
	
	BaseClass::HandleAnimEvent( pEvent );
}


//------------------------------------------------------------------------------
// Purpose : Turn blue or green
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::InputTurnBlue( inputdata_t &data )
{
	bool goBlue = data.value.Bool();
	if (goBlue != m_bIsBlue)
	{
		m_bIsBlue = goBlue;
		m_flBlueEndFadeTime = gpGlobals->curtime + VORTIGAUNT_BLUE_FADE_TIME;
	}
}

//------------------------------------------------------------------------------
// Purpose : Turn blue or green
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::InputTurnBlack( inputdata_t &data )
{
	bool goBlack = data.value.Bool();
	if (goBlack != m_bIsBlack)
	{
		m_bIsBlack = goBlack;
	}
}

//------------------------------------------------------------------------------
// Purpose : Translate some activites for the Vortigaunt
//------------------------------------------------------------------------------
Activity CNPC_Vortigaunt::NPC_TranslateActivity( Activity eNewActivity )
{
	// This is a hack solution for the Vort carrying Alyx in Ep2
	if ( IsCarryingNPC() )
	{
		if ( eNewActivity == ACT_IDLE )
			return ACT_IDLE_CARRY;

		if ( eNewActivity == ACT_WALK || eNewActivity == ACT_WALK_AIM || eNewActivity == ACT_RUN || eNewActivity == ACT_RUN_AIM )
			return ACT_WALK_CARRY;
	}

	// NOTE: This is a stand-in until the readiness system can handle non-weapon holding NPC's
	if ( eNewActivity == ACT_IDLE )
	{
		// More than relaxed means we're stimulated
		if ( GetReadinessLevel() >= AIRL_STIMULATED )
			return ACT_IDLE_STIMULATED;
	}

	if ( eNewActivity == ACT_RANGE_ATTACK2 )
		return (Activity) ACT_VORTIGAUNT_DISPEL;

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::UpdateOnRemove( void)
{
	ClearBeams();
	ClearHandGlow();

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Event_Killed( const CTakeDamageInfo &info )
{
	ClearBeams();
	ClearHandGlow();

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Spawn( void )
{
#if !defined( HL2_EPISODIC )
	// Disable back-away
	AddSpawnFlags( SF_NPC_NO_PLAYER_PUSHAWAY );
#endif // HL2_EPISODIC

	// Allow multiple models (for slaves), but default to vortigaunt.mdl
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/vortigaunt.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	BaseClass::Spawn();

	m_HackedGunPos.x = 0.0f;
	m_HackedGunPos.y = 0.0f;
	m_HackedGunPos.z = 48.0f;

	SetHullType( HULL_WIDE_HUMAN );
	SetHullSizeNormal();

	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_vortigaunt_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 64 ) );// position of the eyes relative to monster's origin.

	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesRemove( bits_CAP_USE_SHOT_REGULATOR );

	m_flEyeIntegRate		= 0.6f;		// Got a big eyeball so turn it slower
	m_bForceArmorRecharge	= false;
	m_flHealHinderedTime	= 0.0f;
	
	m_nCurGlowIndex	= 0;

	m_bStopLoopingSounds	= false;

	m_iLeftHandAttachment = LookupAttachment( VORTIGAUNT_LEFT_CLAW );
	m_iRightHandAttachment = LookupAttachment( VORTIGAUNT_RIGHT_CLAW );

	NPCInit();

	SetUse( &CNPC_Vortigaunt::Use );

	// Setup our re-fire times when moving and shooting
	GetShotRegulator()->SetBurstInterval( 2.0f, 2.0f );
	GetShotRegulator()->SetBurstShotCountRange( 1, 1 );
	GetShotRegulator()->SetRestInterval( 2.0f, 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Precache()
{
	UTIL_PrecacheOther( "vort_charge_token" );

	PrecacheModel( STRING( GetModelName() ) );

	m_nLightningSprite = PrecacheModel("sprites/lgtning.vmt");
	PrecacheModel("sprites/vortring1.vmt");

	// HACK: Only precache this for EP2 because reslists cannot be rebuilt - 08/22/07 - jdw
	if ( hl2_episodic.GetBool() )
	{
		char modDir[MAX_PATH];
		if ( UTIL_GetModDir( modDir, sizeof(modDir) ) )
		{
			if ( !Q_stricmp( modDir, "ep2" ) )
			{
				PrecacheMaterial( "effects/rollerglow" );
			}
		}
	}

	PrecacheScriptSound( "NPC_Vortigaunt.SuitOn" );
	PrecacheScriptSound( "NPC_Vortigaunt.SuitCharge" );
	PrecacheScriptSound( "NPC_Vortigaunt.ZapPowerup" );
	PrecacheScriptSound( "NPC_Vortigaunt.ClawBeam" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartHealLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.Swing" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartShootLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepLeft" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepRight" );
	PrecacheScriptSound( "NPC_Vortigaunt.DispelStart" );
	PrecacheScriptSound( "NPC_Vortigaunt.DispelImpact" );
	PrecacheScriptSound( "NPC_Vortigaunt.Explode" );

	PrecacheParticleSystem( "vortigaunt_beam" );
	PrecacheParticleSystem( "vortigaunt_beam_charge" );
	PrecacheParticleSystem( "vortigaunt_hand_glow" );

	PrecacheMaterial( "sprites/light_glow02_add" );

	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// Purpose: Interpret a player +USE'ing us
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	// Foremost, try and heal a wounded player
	if ( HealBehaviorAvailable() )
	{
		// See if we should heal the player
		CBaseEntity *pHealTarget = FindHealTarget();
		if ( pHealTarget != NULL )
		{
			SetCondition( COND_PROVOKED );
			SetHealTarget( pHealTarget, true );
			return;
		}
	}
	
	// Next, try to speak the +USE concept
	if ( IsOkToSpeakInResponseToPlayer() && m_eHealState == HEAL_STATE_NONE )
	{
		if ( Speak( TLK_USE ) == false )
		{
			// If we haven't said hi, say that first
			if ( !SpokeConcept( TLK_HELLO ) )
			{
				Speak( TLK_HELLO );
			}
			else
			{
				Speak( TLK_IDLE );
			}
		}
		else
		{
			// Don't say hi after you've said your +USE speech
			SetSpokeConcept( TLK_HELLO, NULL );	
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Vortigaunt::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime < m_flPainTime )
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

	Speak( VORT_PAIN );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Vortigaunt::DeathSound( const CTakeDamageInfo &info )
{
	Speak( VORT_DIE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;

	if ( (info.GetDamageType() & DMG_SHOCK) && FClassnameIs( info.GetAttacker(), GetClassname() ) )
	{
		// mask off damage from other vorts for now
		info.SetDamage( 0.01 );
	}

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f );
		}
		break;
	case 10:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			info.SetDamage( info.GetDamage() - 20 );
			if (info.GetDamage() <= 0)
			{
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
				info.SetDamage( 0.01 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ALERT_FACE_BESTSOUND:
		return SCHED_VORT_ALERT_FACE_BESTSOUND;
		break;

	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		
		// Stand still if we're in the middle of an attack.  Failing to do so can make us miss our shot!
		if ( IsPlayingGesture( ACT_GESTURE_RANGE_ATTACK1 ) )
			return SCHED_COMBAT_FACE;

		return SCHED_VORT_FLEE_FROM_BEST_SOUND;
		break;

	case SCHED_COWER:
	case SCHED_PC_COWER:
		// Vort doesn't have cower animations
		return SCHED_COMBAT_FACE;
		break;

	case SCHED_RANGE_ATTACK1:
		
		// If we're told to fire when we're already firing, just face our target.  If we don't do this, we get a bizarre double-shot
		if ( IsPlayingGesture( ACT_GESTURE_RANGE_ATTACK1 ) )
			return SCHED_COMBAT_FACE;

		// Otherwise we use our own schedule to attack
		return SCHED_VORTIGAUNT_RANGE_ATTACK;
		break;

	/*
	case SCHED_CHASE_ENEMY:
	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		
		// Don't go running off after an enemy just because we're in an attack delay!  This has to do with
		// the base systems assuming that held weapons are driving certain decisions when this creature
		// uses an innate ability.
		if ( ( GetNextAttack() > gpGlobals->curtime ) && HasCondition( COND_ENEMY_TOO_FAR ) == false )
			return SCHED_COMBAT_FACE;

		break;
		*/
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the heal target for the vort and preps him for completing the action
// Input  : *pTarget - Target we're after
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::SetHealTarget( CBaseEntity *pTarget, bool bPlayerRequested )
{
	SetCondition( COND_VORTIGAUNT_CAN_HEAL );
	OccupyStrategySlot( SQUAD_SLOT_HEAL_PLAYER );
	m_hHealTarget = pTarget;
	m_bPlayerRequestedHeal = bPlayerRequested;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a player in range that can be healed
// Output : Target that can be healed
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_Vortigaunt::FindHealTarget( void )
{
	// Need to be looking at the player to decide to heal them.
	//if ( HasCondition( COND_SEE_PLAYER ) == false )
	//	return false;

	// Find a likely target in range
	CBaseEntity *pEntity = PlayerInRange( GetAbsOrigin(), HEAL_SEARCH_RANGE );

	// Make sure we can heal that target
	if ( ShouldHealTarget( pEntity ) == false )
		return NULL;

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not the vort is able to attempt to heal targets
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::HealBehaviorAvailable( void )
{
	// Cannot already be healing
	if ( m_eHealState != HEAL_STATE_NONE )
		return false;

	// Must be allowed to do this behavior
	if ( m_bArmorRechargeEnabled == false )
		return false;

	// Don't interrupt a script
	if ( IsInAScript() || m_NPCState == NPC_STATE_SCRIPT )
		return false;

	// Cannot interrupt bugbait extraction
	if ( IsCurSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT ) )
		return false;

	// Don't bother while we're under attack
	if ( GetEnemy() != NULL )
		return false;

	// Can't heal if we're leading the player
	if ( IsLeading() )
		return false;

	// Must be a valid squad activity to do
	if ( IsStrategySlotRangeOccupied( SQUAD_SLOT_HEAL_PLAYER, SQUAD_SLOT_HEAL_PLAYER ) )
		return false;

	// Heal is valid
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::ShouldHealTarget( CBaseEntity *pTarget )
{
	// Must have a valid target
	if ( pTarget == NULL )
		return false;

	// If we're scripting or waiting to run one, we won't heal a target
	if ( IsInAScript() || HasSpawnFlags( SF_NPC_WAIT_FOR_SCRIPT ) )
		return false;

	// We only heal players
	CBasePlayer *pPlayer = ToBasePlayer( pTarget );
	if ( pPlayer == NULL )
		return false;

	// Make sure the player's got a suit
	if ( pPlayer->IsSuitEquipped() == false )
		return false;

	// Don't heal a target we can't see..?
	if ( pPlayer->GetFlags() & FL_NOTARGET )
		return false;

	// See if the player needs armor
	if ( pPlayer->ArmorValue() >= (sk_vortigaunt_armor_charge.GetFloat()*0.66f) )
		return false;

	// Must be alive!
	if ( pPlayer->IsAlive() == false )
		return false;

	// Only consider things in here if the player is NOT at critical health or the heal is a passive one (not requested)
	if ( PlayerBelowHealthPercentage( pPlayer, PLAYER_CRITICAL_HEALTH_PERC ) == false || m_bPlayerRequestedHeal )
	{
		// Don't heal when fighting
		if ( m_NPCState == NPC_STATE_COMBAT )
			return false;

		// No enemies
		if ( GetEnemy() )
			return false;

		// No recent damage
		if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
			return false;
	}

	// Allow the heal
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectHealSchedule( void )
{
	// If our lead behavior has a goal, don't wait around to heal anyone
	if ( m_LeadBehavior.HasGoal() )
		return SCHED_NONE;

	// Break out of healing if a script has started
	if ( IsInAScript() && m_bForceArmorRecharge == false )
	{
		if ( m_eHealState != HEAL_STATE_NONE )
		{
			StopHealing( true );
		}

		return SCHED_NONE;
	}

	// Cannot already be healing the player
	if ( m_hHealTarget != NULL )
	{
		// For now, just grab the global, single player
		CBasePlayer *pPlayer = ToBasePlayer( m_hHealTarget );

		// Check for an interruption occurring
		if ( PlayerBelowHealthPercentage( pPlayer, PLAYER_CRITICAL_HEALTH_PERC ) == false && HasCondition( COND_HEAVY_DAMAGE ) )
		{
			StopHealing( true );
			return SCHED_NONE;
		}

		// See if we're in an ideal position to heal
		if ( m_eHealState != HEAL_STATE_HEALING && m_eHealState != HEAL_STATE_WARMUP && HasCondition( COND_VORTIGAUNT_HEAL_VALID ) )
			return SCHED_VORTIGAUNT_HEAL;

		// If the player is too far away or blocked, give chase
		if ( HasCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR ) ||
			 HasCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED ) )
			return SCHED_VORTIGAUNT_RUN_TO_PLAYER;

		// Stand and face the player
		if ( HasCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US ) || HasCondition( COND_VORTIGAUNT_HEAL_VALID ) )
			return SCHED_VORTIGAUNT_FACE_PLAYER;
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Watch this function path for a route around our normal schedule changing callbacks
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearSchedule( const char *szReason )
{
	MaintainGlows();

	BaseClass::ClearSchedule( szReason );
}

//-----------------------------------------------------------------------------
// Purpose: Watch our glows and turn them off appropriately
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::OnScheduleChange( void )
{
	BaseClass::OnScheduleChange();

	// If we're in the middle of healing, don't bother doing this
	if ( m_eHealState != HEAL_STATE_NONE )
		return;

	// If we're changing sequences, always clear
	EndHandGlow( VORTIGAUNT_BEAM_ALL );
	m_fGlowChangeTime = gpGlobals->curtime + 0.1f;	// No more glows for this amount of time!
}

//------------------------------------------------------------------------------
// Purpose: Select a schedule
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectSchedule( void )
{
	// Always recharge in this case
	if ( m_bForceArmorRecharge )
	{
		m_flNextHealTime = 0;
		int nSchedule = SelectHealSchedule();
			return nSchedule;
	}

#ifndef HL2_EPISODIC
	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();
#else
	if ( IsInAScript() )
		return BaseClass::SelectSchedule();
#endif

	// If we're currently supposed to be doing something scripted, do it immediately.
	if ( m_bExtractingBugbait )
		return SCHED_VORTIGAUNT_EXTRACT_BUGBAIT;

	int schedule = SelectHealSchedule();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( HasCondition(COND_VORTIGAUNT_DISPEL_ANTLIONS ) )
	{
		ClearCondition( COND_VORTIGAUNT_DISPEL_ANTLIONS );
		return SCHED_VORTIGAUNT_DISPEL_ANTLIONS;
	}

	// Heal a player if they can be
	if ( HasCondition( COND_VORTIGAUNT_CAN_HEAL ) )
		return SCHED_VORTIGAUNT_HEAL;

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule == SCHED_BACK_AWAY_FROM_ENEMY )
	{
		if ( GetEnemy() && GetSenses()->CanSeeEntity( GetEnemy() ) )
		{
			return SCHED_RANGE_ATTACK1;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::DeclineFollowing( void )
{
	Speak( VORT_POK );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if you're willing to be idly talked to by other friends.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::CanBeUsedAsAFriend( void )
{
	// We don't want to be used if we're busy
	if ( IsCurSchedule( SCHED_VORTIGAUNT_HEAL ) )
		return false;
	
	if ( IsCurSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT ) )
		return false;

	return BaseClass::CanBeUsedAsAFriend();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define VORT_360_VIEW_DIST_SQR	((60*12)*(60*12))
bool CNPC_Vortigaunt::FInViewCone( CBaseEntity *pEntity )
{
	// Vort can see 360 degrees but only at limited distance
	if( ( pEntity->IsNPC() || pEntity->IsPlayer() ) && pEntity->GetAbsOrigin().DistToSqr(GetAbsOrigin()) <= VORT_360_VIEW_DIST_SQR )
		return true;

	return BaseClass::FInViewCone( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Start our heal loop
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartHealing( void )
{
	// Find the layer and stop it from moving forward in the cycle
	int nLayer = FindGestureLayer( (Activity) ACT_VORTIGAUNT_HEAL );
	SetLayerPlaybackRate( nLayer, 0.0f );

	// We're now in the healing loop
	m_eHealState = HEAL_STATE_HEALING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StopHealing( bool bInterrupt )
{
	// Clear out our healing states
	m_eHealState = HEAL_STATE_NONE;
	m_bForceArmorRecharge = false;
	m_hHealTarget = NULL;

	EndHandGlow( VORTIGAUNT_BEAM_HEAL );
	VacateStrategySlot();
	
	// See if we're completely interrupting the heal or just ending normally
	if ( bInterrupt )
	{
		RemoveGesture( (Activity) ACT_VORTIGAUNT_HEAL );
		m_flNextHealTime = gpGlobals->curtime + 2.0f;
	}
	else
	{
		// Start our animation back up again
		int nLayer = FindGestureLayer( (Activity) ACT_VORTIGAUNT_HEAL );
		SetLayerPlaybackRate( nLayer, 1.0f );

		m_flNextHealTime = gpGlobals->curtime + VORTIGAUNT_HEAL_RECHARGE;
		m_OnFinishedChargingTarget.FireOutput( this, this );
	}

	// Give us time to stop our animation before we start attacking (otherwise we get weird collisions)
	SetNextAttack( gpGlobals->curtime + 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Update our heal schedule and gestures if we're currently healing
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::MaintainHealSchedule( void )
{
	// Need to be healing
	if ( m_eHealState == HEAL_STATE_NONE )
		return;

	// For now, we only heal the player
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
		return;

	// FIXME: How can this happen?
	if ( m_AssaultBehavior.GetOuter() != NULL )
	{
		// Interrupt us on an urgent assault
		if ( m_AssaultBehavior.IsRunning() && ( m_AssaultBehavior.IsUrgent() || m_AssaultBehavior.OnStrictAssault() ) )
		{
			StopHealing( true );
			return;
		}
	}

	// Don't let us shoot while we're healing
	GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 0.5f );

	// If we're in the healing phase, heal our target (if able)
	if ( m_eHealState == HEAL_STATE_HEALING )
	{
		// FIXME: We need to have better logic controlling this
		if ( HasCondition( COND_VORTIGAUNT_HEAL_VALID ) )
		{
			if ( m_flNextHealTokenTime < gpGlobals->curtime )
			{
				CBasePlayer *pPlayer = ToBasePlayer( m_hHealTarget );

				// We're done, so stop playing the animation
				if ( m_nNumTokensToSpawn <= 0 || ( m_bForceArmorRecharge == false && ( pPlayer && pPlayer->ArmorValue() >= sk_vortigaunt_armor_charge.GetInt() ) ) )
				{
					m_flHealHinderedTime = 0.0f;
					m_nNumTokensToSpawn = 0;
					SpeakIfAllowed( VORT_CURESTOP );
					StopHealing( false );
					return;
				}

				// Create a charge token
				Vector vecHandPos;
				QAngle vecHandAngles;
				GetAttachment( m_iRightHandAttachment, vecHandPos, vecHandAngles );
				CVortigauntChargeToken::CreateChargeToken( vecHandPos, this, m_hHealTarget );
				m_flNextHealTokenTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
				m_nNumTokensToSpawn--;

				// If we're stopping, delay our animation a bit so it's not so robotic
				if ( m_nNumTokensToSpawn <= 0 )
				{
					m_nNumTokensToSpawn = 0;
					m_flNextHealTokenTime = gpGlobals->curtime + 1.0f;
				}
			}
		}
		else
		{
			/*
			// NOTENOTE: It's better if the vort give up than ignore things around him to try and continue -- jdw

			// Increment a counter to let us know how long we've failed
			m_flHealHinderedTime += gpGlobals->curtime - GetLastThink();
			
			if ( m_flHealHinderedTime > 2.0f )
			{
				// If too long, stop trying
				StopHealing();
			}
			*/

			bool bInterrupt = false;
			if ( HasCondition( COND_NEW_ENEMY ) )
			{
				bInterrupt = true;
			}

			StopHealing( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CNPC_Vortigaunt::InAttackSequence( void )
{
	if ( m_MoveAndShootOverlay.IsMovingAndShooting() )
		return true;
	
	if ( GetActivity() == ACT_RANGE_ATTACK1 )
		return true;

	if ( GetActivity() == ACT_VORTIGAUNT_DISPEL )
		return true;

	if ( IsPlayingGesture( ACT_GESTURE_RANGE_ATTACK1 ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Watch our beams and make sure we don't leave them on mistakenly
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::MaintainGlows( void )
{
	// Verify that if we're not in an attack gesture, that we're not doing an attack glow
	if ( InAttackSequence() == false && m_eHealState == HEAL_STATE_NONE )
	{
		EndHandGlow( VORTIGAUNT_BEAM_ALL );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Squelch looping sounds and glows after a restore.
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::OnRestore( void )
{
	BaseClass::OnRestore();

	m_bStopLoopingSounds = true;
}

//-----------------------------------------------------------------------------
// Purpose: Do various non-schedule specific maintainence
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::PrescheduleThink( void )
{
	// Update our healing (if active)
	MaintainHealSchedule();

	// Let the base class have a go
	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &move - 
//			flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	// If we're in our aiming gesture, then always face our target as we run
	Activity nActivity = NPC_TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 );
	if ( IsPlayingGesture( nActivity ) || 
	     IsCurSchedule( SCHED_PC_MOVE_TOWARDS_COVER_FROM_BEST_SOUND ) || 
	     IsCurSchedule( SCHED_VORT_FLEE_FROM_BEST_SOUND ) ||
		 IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) )
	{
		Vector vecEnemyLKP = GetEnemyLKP();
		AddFacingTarget( GetEnemy(), vecEnemyLKP, 1.0, 0.2 );
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::BuildScheduleTestBits( void )
{
	// Call to base
	BaseClass::BuildScheduleTestBits();

	// Allow healing to interrupt us if we're standing around
	if ( IsCurSchedule( SCHED_IDLE_STAND ) || 
		 IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		if ( m_eHealState == HEAL_STATE_NONE )
		{
			SetCustomInterruptCondition( COND_VORTIGAUNT_CAN_HEAL );
			SetCustomInterruptCondition( COND_VORTIGAUNT_DISPEL_ANTLIONS );
		}
	}

	// Always interrupt when healing
	if ( m_eHealState != HEAL_STATE_NONE )
	{
		// Interrupt if we're not already adjusting
		if ( IsCurSchedule( SCHED_VORTIGAUNT_RUN_TO_PLAYER ) == false )
		{
			SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
			SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
			
			// Interrupt if we're not already turning
			if ( IsCurSchedule( SCHED_VORTIGAUNT_FACE_PLAYER ) == false )
			{
				SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );
			}
		}
	}

	if ( IsCurSchedule( SCHED_COMBAT_STAND ) )
	{
		SetCustomInterruptCondition( COND_VORTIGAUNT_DISPEL_ANTLIONS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Small beam from arm to nearby geometry
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::ArmBeam( int beamType, int nHand )
{
	trace_t tr;
	float flDist = 1.0;
	int side = ( nHand == HAND_LEFT ) ? -1 : 1;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
	Vector vecSrc = GetLocalOrigin() + up * 36 + right * side * 16 + forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = forward * random->RandomFloat( -1, 1 ) + right * side * random->RandomFloat( 0, 1 ) + up * random->RandomFloat( -1, 1 );
		trace_t tr1;
		AI_TraceLine ( vecSrc, vecSrc + vecAim * (10*12), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1);
		
		// Don't hit the sky
		if ( tr1.surface.flags & SURF_SKY )
			continue;

		// Choose a farther distance if we have one
		if ( flDist > tr1.fraction )
		{
			tr = tr1;
			flDist = tr.fraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	// Tell the client to start an arm beam
	unsigned char uchAttachment = (nHand==HAND_LEFT) ? m_iLeftHandAttachment : m_iRightHandAttachment;
	EntityMessageBegin( this, true );
		WRITE_BYTE( VORTFX_ARMBEAM );
		WRITE_LONG( entindex() );
		WRITE_BYTE( uchAttachment );
		WRITE_VEC3COORD( tr.endpos );
		WRITE_VEC3NORMAL( tr.plane.normal );
	MessageEnd();
}

//------------------------------------------------------------------------------
// Purpose : Put glowing sprites on hands
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::StartHandGlow( int beamType, int nHand )
{
	// We need this because there's a rare case where a scene can interrupt and turn off our hand glows, but are then
	// turned back on in the same frame due to how animations are applied and anim events are executed after the AI frame.
	if ( m_fGlowChangeTime > gpGlobals->curtime )
		return;

	switch( beamType )
	{
	case VORTIGAUNT_BEAM_DISPEL:
	case VORTIGAUNT_BEAM_HEAL:
	case VORTIGAUNT_BEAM_ZAP:
		{
			// Validate the hand's range
			if ( nHand >= ARRAYSIZE( m_hHandEffect ) )
				return;

			// Start up
			if ( m_hHandEffect[nHand] == NULL )
			{
				// Create the token if it doesn't already exist
				m_hHandEffect[nHand] = CVortigauntEffectDispel::CreateEffectDispel( GetAbsOrigin(), this, NULL );
				if ( m_hHandEffect[nHand] == NULL )
					return;
			}

			// Stomp our settings
			m_hHandEffect[nHand]->SetParent( this, (nHand==HAND_LEFT) ? m_iLeftHandAttachment : m_iRightHandAttachment );
			m_hHandEffect[nHand]->SetMoveType( MOVETYPE_NONE );
			m_hHandEffect[nHand]->SetLocalOrigin( Vector( 8.0f, 4.0f, 0.0f ) );
		}
		break;

	case VORTIGAUNT_BEAM_ALL:
		Assert( 0 );
		break;
	}	
}

//------------------------------------------------------------------------------
// Purpose: Fade glow from hands.
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::EndHandGlow( int beamType /*= VORTIGAUNT_BEAM_ALL*/ )
{
	if ( m_hHandEffect[0] )
	{
		m_hHandEffect[0]->FadeAndDie();
		m_hHandEffect[0] = NULL;
	}

	if ( m_hHandEffect[1] )
	{
		m_hHandEffect[1]->FadeAndDie();
		m_hHandEffect[1] = NULL;
	}

	// Zap
	if ( beamType == VORTIGAUNT_BEAM_ZAP || beamType == VORTIGAUNT_BEAM_ALL )
	{
		m_fGlowAge = 0;

		// Stop our smaller beams as well
		ClearBeams();
	}
}

extern int ACT_ANTLION_ZAP_FLIP;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( IsRoller( pEnemy ) )
	{
		CAI_BaseNPC *pNPC = pEnemy->MyNPCPointer();
		if ( pNPC && pNPC->GetEnemy() != NULL )
			return true;
		return false;
	}

	// Wait until our animation is finished
	if ( GetEnemy() == NULL && m_flAimDelay > gpGlobals->curtime )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a blast where the beam has struck a target
// Input  : &vecOrigin - position to eminate from
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::CreateBeamBlast( const Vector &vecOrigin )
{
	CSprite *pBlastSprite = CSprite::SpriteCreate( "sprites/vortring1.vmt", vecOrigin, true );
	if ( pBlastSprite != NULL )
	{
		pBlastSprite->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 255, kRenderFxNone );
		pBlastSprite->SetBrightness( 255 );
		pBlastSprite->SetScale( random->RandomFloat( 1.0f, 1.5f ) );
		pBlastSprite->AnimateAndDie( 45.0f );
		pBlastSprite->EmitSound( "NPC_Vortigaunt.Explode" );
	}

	CPVSFilter filter( vecOrigin );
	te->GaussExplosion( filter, 0.0f, vecOrigin, Vector( 0, 0, 1 ), 0 );
}

#define COS_30	0.866025404f // sqrt(3) / 2
#define COS_60	0.5 // sqrt(1) / 2

//-----------------------------------------------------------------------------
// Purpose: Heavy damage directly forward
// Input  : nHand - Handedness of the beam
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::ZapBeam( int nHand )
{
	Vector forward;
	GetVectors( &forward, NULL, NULL );

	Vector vecSrc = GetAbsOrigin() + GetViewOffset();
	Vector vecAim = GetShootEnemyDir( vecSrc, false );	// We want a clear shot to their core

	if ( GetEnemy() )
	{
		Vector vecTarget = GetEnemy()->BodyTarget( vecSrc, false );
				
		if ( g_debug_vortigaunt_aim.GetBool() )
		{
			NDebugOverlay::Cross3D( vecTarget, 4.0f, 255, 0, 0, true, 10.0f );
			CBaseAnimating *pAnim = GetEnemy()->GetBaseAnimating();
			if ( pAnim )
			{
				pAnim->DrawServerHitboxes( 10.0f );
			}
		}
	}

	// If we're too far off our center, the shot must miss!
	if ( DotProduct( vecAim, forward ) < COS_60 )
	{
		// Missed, so just shoot forward
		vecAim = forward;
	}

	trace_t tr;

	if ( m_bExtractingBugbait == true )
	{
		CRagdollProp *pTest = dynamic_cast< CRagdollProp *>( GetTarget() );

		if ( pTest )
		{
			ragdoll_t *m_ragdoll = pTest->GetRagdoll();

			if ( m_ragdoll )
			{
				Vector vOrigin;
				m_ragdoll->list[0].pObject->GetPosition( &vOrigin, 0 );

				AI_TraceLine( vecSrc, vOrigin, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			}

			CRagdollBoogie::Create( pTest, 200, gpGlobals->curtime, 1.0f );
		}
	}
	else
	{
		AI_TraceLine( vecSrc, vecSrc + ( vecAim * InnateRange1MaxRange() ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	}

	if ( g_debug_vortigaunt_aim.GetBool() )
	{
		NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 10.0f );
	}

	// Send a message to the client to create a "zap" beam
	unsigned char uchAttachment = (nHand==HAND_LEFT) ? m_iLeftHandAttachment : m_iRightHandAttachment;
	EntityMessageBegin( this, true );
		WRITE_BYTE( VORTFX_ZAPBEAM );
		WRITE_BYTE( uchAttachment );
		WRITE_VEC3COORD( tr.endpos );
	MessageEnd();

	CBaseEntity *pEntity = tr.m_pEnt;
	if ( pEntity != NULL && m_takedamage )
	{
		if ( g_debug_vortigaunt_aim.GetBool() )
		{
			NDebugOverlay::Box( tr.endpos, -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, 8, 10.0f );
		}

		CTakeDamageInfo dmgInfo( this, this, sk_vortigaunt_dmg_zap.GetFloat(), DMG_SHOCK );
		dmgInfo.SetDamagePosition( tr.endpos );
		VectorNormalize( vecAim );// not a unit vec yet
		// hit like a 5kg object flying 100 ft/s
		dmgInfo.SetDamageForce( 5 * 100 * 12 * vecAim );
		
		// Our zaps do special things to antlions
		if ( FClassnameIs( pEntity, "npc_antlion" ) )
		{
			// Make a worker flip instead of explode
			if ( IsAntlionWorker( pEntity ) )
			{
				CNPC_Antlion *pAntlion = static_cast<CNPC_Antlion *>(pEntity);
				pAntlion->Flip();
			}
			else
			{
				// Always gib the antlion hit!
				dmgInfo.ScaleDamage( 4.0f );
			}
			
			// Look in a ring and flip other antlions nearby
			DispelAntlions( tr.endpos, 200.0f, false );
		}

		// Send the damage to the recipient
		pEntity->DispatchTraceAttack( dmgInfo, vecAim, &tr );
	}

	// Create a cover for the end of the beam
	CreateBeamBlast( tr.endpos );
}

//------------------------------------------------------------------------------
// Purpose: Clear glow from hands immediately
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearHandGlow( void )
{
	if ( m_hHandEffect[0] != NULL )
	{
		UTIL_Remove( m_hHandEffect[0] );
		m_hHandEffect[0] = NULL;
	}
	
	if ( m_hHandEffect[1] != NULL )
	{
		UTIL_Remove( m_hHandEffect[1] );
		m_hHandEffect[1] = NULL;
	}
	
	m_fGlowAge = 0;
}

//------------------------------------------------------------------------------
// Purpose: remove all beams
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearBeams( void )
{
	// Stop looping suit charge sound.
	if ( m_bStopLoopingSounds )
	{
		StopSound( "NPC_Vortigaunt.StartHealLoop" );
		StopSound( "NPC_Vortigaunt.StartShootLoop" );
		StopSound( "NPC_Vortigaunt.SuitCharge" );
		StopSound( "NPC_Vortigaunt.ZapPowerup" );
		m_bStopLoopingSounds = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEnableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDisableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputChargeTarget( inputdata_t &data )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, data.value.String(), NULL, data.pActivator, data.pCaller );

	// Must be valid
	if ( pTarget == NULL )
	{
		DevMsg( 1, "Unable to charge from unknown entity: %s!\n", data.value.String() );
		return;
	}

	int playerArmor = (pTarget->IsPlayer()) ? ((CBasePlayer *)pTarget)->ArmorValue() : 0;

	if ( playerArmor >= 100 || ( pTarget->GetFlags() & FL_NOTARGET ) )
	{
		m_OnFinishedChargingTarget.FireOutput( this, this );
		return;
	}

	m_hHealTarget = pTarget;
	m_bForceArmorRecharge = true;

	SetCondition( COND_PROVOKED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputExtractBugbait( inputdata_t &data )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, data.value.String(), NULL, data.pActivator, data.pCaller );

	// Must be valid
	if ( pTarget == NULL )
	{
		DevMsg( 1, "Unable to extract bugbait from unknown entity %s!\n", data.value.String() );
		return;
	}

	// Keep this as our target
	SetTarget( pTarget );

	// Start to extract
	m_bExtractingBugbait = true;
	SetSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT );
}


//-----------------------------------------------------------------------------
// Purpose: Allows the vortigaunt to use health regeneration
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEnableHealthRegeneration( inputdata_t &data )
{
	m_bRegenerateHealth = true;
}

//-----------------------------------------------------------------------------
// Purpose: Stops the vortigaunt from using health regeneration (default)
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDisableHealthRegeneration( inputdata_t &data )
{
	m_bRegenerateHealth = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::IRelationPriority( CBaseEntity *pTarget )
{
	int priority = BaseClass::IRelationPriority( pTarget );
	
	if ( pTarget == NULL )
		return priority;

	CBaseEntity *pEnemy = GetEnemy();

	// Handle antlion cases
	if ( pEnemy != NULL && pEnemy != pTarget )
	{
		// I have an enemy that is not this thing. If that enemy is near, I shouldn't become distracted.
		if ( GetAbsOrigin().DistToSqr( pEnemy->GetAbsOrigin()) < Square(15*12) )
			return priority;
	}

	// Targets near our follow target have a higher priority to us
	if ( m_FollowBehavior.GetFollowTarget() && 
		m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().DistToSqr( pTarget->GetAbsOrigin() ) < Square(25*12) )
	{
		priority++;
	}

	// Flipped antlions are of lower priority
	CAI_BaseNPC *pNPC = pTarget->MyNPCPointer();
	if ( pNPC && pNPC->Classify() == CLASS_ANTLION && pNPC->GetActivity() == ACT_ANTLION_ZAP_FLIP )
		priority--;

	return priority;
}

//-----------------------------------------------------------------------------
// Purpose: back away from overly close zombies
//-----------------------------------------------------------------------------
Disposition_t CNPC_Vortigaunt::IRelationType( CBaseEntity *pTarget )
{
	if ( pTarget == NULL )
		return D_NU;

	Disposition_t disposition = BaseClass::IRelationType( pTarget );

	if ( pTarget->Classify() == CLASS_ZOMBIE && disposition == D_HT )
	{
		if( GetAbsOrigin().DistToSqr(pTarget->GetAbsOrigin()) < VORTIGAUNT_FEAR_ZOMBIE_DIST_SQR )
		{
			// Be afraid of a zombie that's near if I'm not allowed to dodge. This will make Alyx back away.
			return D_FR;
		}
	}

	return disposition;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the heal gesture can successfully reach the player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::HealGestureHasLOS( void )
{
	//For now the player is always our target
	CBaseEntity *pTargetEnt = AI_GetSinglePlayer();
	if ( pTargetEnt == NULL )
		return false;

	// Find our left hand as the starting point
	Vector vecHandPos;
	QAngle vecHandAngle;
	GetAttachment( m_iRightHandAttachment, vecHandPos, vecHandAngle );

	// Trace to our target, skipping ourselves and the target
	trace_t tr;
	CTraceFilterSkipTwoEntities filter( this, pTargetEnt, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecHandPos, pTargetEnt->WorldSpaceCenter(), MASK_SHOT, &filter, &tr );

	// Must be clear
	if ( tr.fraction < 1.0f || tr.startsolid || tr.allsolid )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions for our healing behavior
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::GatherHealConditions( void )
{
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );

	// We stop if there are enemies around
	if ( m_bArmorRechargeEnabled == false ||
		 HasCondition( COND_NEW_ENEMY ) || 
		 HasCondition( COND_HEAR_DANGER ) || 
		 HasCondition( COND_HEAVY_DAMAGE ) )
	{
		ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		return;
	}

	// Start by assuming that we'll succeed
	SetCondition( COND_VORTIGAUNT_HEAL_VALID );

	// Just assume we should
	if ( m_bForceArmorRecharge )
		return;

	// For now we only act on the player
	CBasePlayer *pPlayer = ToBasePlayer( m_hHealTarget );
	if ( pPlayer != NULL )
	{
		Vector vecToPlayer = ( pPlayer->WorldSpaceCenter() - WorldSpaceCenter() );

		// Make sure he's still within heal range
		if ( vecToPlayer.LengthSqr() > (HEAL_RANGE*HEAL_RANGE) )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
			// NOTE: We allow him to send tokens over large distances
			//ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}

		vecToPlayer.z = 0.0f;
		VectorNormalize( vecToPlayer );
		Vector facingDir = BodyDirection2D();

		// Check our direction towards the player
		if ( DotProduct( vecToPlayer, facingDir ) < VIEW_FIELD_NARROW )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );
			ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}

		// Now ensure he's not blocked
		if ( HealGestureHasLOS() == false )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
			ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}
	}
	else
	{
		ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions specific to this NPC
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::GatherConditions( void )
{
	// Call our base
	BaseClass::GatherConditions();

	// See if we're able to heal now
	if ( HealBehaviorAvailable() && ( m_flNextHealTime < gpGlobals->curtime ) )
	{
		// See if we should heal the player
		CBaseEntity *pHealTarget = FindHealTarget();
		if ( pHealTarget != NULL )
		{
			SetHealTarget( pHealTarget, false );
		}

		// Don't try again for a period of time
		m_flNextHealTime = gpGlobals->curtime + 2.0f;
	}

	// Get our state for healing
	GatherHealConditions();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::DispelAntlions( const Vector &vecOrigin, float flRadius, bool bDispel /*= true*/ )
{
	// More effects
	if ( bDispel )
	{
		UTIL_ScreenShake( vecOrigin, 20.0f, 150.0, 1.0, 1250.0f, SHAKE_START );

		CBroadcastRecipientFilter filter2;
		te->BeamRingPoint( filter2, 0, vecOrigin,	//origin
			64,			//start radius
			800,		//end radius
			m_nLightningSprite, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.1f,		//life
			128,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			32,		//a
			0,		//speed
			FBEAM_FADEOUT
			);

		//Shockring
		te->BeamRingPoint( filter2, 0, vecOrigin + Vector( 0, 0, 16 ),	//origin
			64,			//start radius
			800,		//end radius
			m_nLightningSprite, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.2f,		//life
			64,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			200,		//a
			0,		//speed
			FBEAM_FADEOUT
			);

		// Ground effects
		CEffectData	data;
		data.m_vOrigin = vecOrigin;

		DispatchEffect( "VortDispel", data );
	}

	// Make antlions flip all around us!
	trace_t tr;
	CBaseEntity *pEnemySearch[32];
	int nNumEnemies = UTIL_EntitiesInBox( pEnemySearch, ARRAYSIZE(pEnemySearch), vecOrigin-Vector(flRadius,flRadius,flRadius), vecOrigin+Vector(flRadius,flRadius,flRadius), FL_NPC );
	for ( int i = 0; i < nNumEnemies; i++ )
	{
		// We only care about antlions
		if ( IsAntlion( pEnemySearch[i] ) == false )
			continue;

		CNPC_Antlion *pAntlion = static_cast<CNPC_Antlion *>(pEnemySearch[i]);
		if ( pAntlion->IsWorker() == false )
		{
			// Attempt to trace a line to hit the target
			UTIL_TraceLine( vecOrigin, pAntlion->BodyTarget( vecOrigin ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction < 1.0f && tr.m_pEnt != pAntlion )
				continue;

			Vector vecDir = ( pAntlion->GetAbsOrigin() - vecOrigin );
			vecDir[2] = 0.0f;
			float flDist = VectorNormalize( vecDir );

			float flFalloff = RemapValClamped( flDist, 0, flRadius*0.75f, 1.0f, 0.1f );

			vecDir *= ( flRadius * 1.5f * flFalloff );
			vecDir[2] += ( flRadius * 0.5f * flFalloff );

			pAntlion->ApplyAbsVelocityImpulse( vecDir );

			// gib nearby antlions, knock over distant ones. 
			if ( flDist < 128 && bDispel )
			{
				// splat!
				vecDir[2] += 400.0f * flFalloff;
				CTakeDamageInfo dmgInfo( this, this, vecDir, pAntlion->GetAbsOrigin() , 100, DMG_SHOCK );
				pAntlion->TakeDamage( dmgInfo );
			}
			else
			{
				// Turn them over
				pAntlion->Flip( true );

				// Display an effect between us and the flipped creature
				// Tell the client to start an arm beam
				/*
				unsigned char uchAttachment = pAntlion->LookupAttachment( "mouth" );
				EntityMessageBegin( this, true );
					WRITE_BYTE( VORTFX_ARMBEAM );
					WRITE_LONG( pAntlion->entindex() );
					WRITE_BYTE( uchAttachment );
					WRITE_VEC3COORD( vecOrigin );
					WRITE_VEC3NORMAL( Vector( 0, 0, 1 ) );
				MessageEnd();
				*/
			}
		}
	}
	
	// Stop our effects
	if ( bDispel )
	{
		EndHandGlow( VORTIGAUNT_BEAM_ALL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simply tell us to dispel
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDispel( inputdata_t &data )
{
	SetCondition( COND_VORTIGAUNT_DISPEL_ANTLIONS );
}

//-----------------------------------------------------------------------------
// Purpose: Decide when we're allowed to interact with other NPCs
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::CanRunAScriptedNPCInteraction( bool bForced /*= false*/ )
{
	// Never interrupt a range attack!
	if ( InAttackSequence() )
		return false;

	// Can't do them while we're trying to heal the player
	if ( m_eHealState != HEAL_STATE_NONE )
		return false;

	return BaseClass::CanRunAScriptedNPCInteraction( bForced );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : interrupt - 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::SetScriptedScheduleIgnoreConditions( Interruptability_t interrupt )
{
	// First add our base conditions to ignore
	BaseClass::SetScriptedScheduleIgnoreConditions( interrupt );

	static int g_VortConditions[] = 
	{
		COND_VORTIGAUNT_CAN_HEAL,
		COND_VORTIGAUNT_DISPEL_ANTLIONS,
		COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR,
		COND_VORTIGAUNT_HEAL_TARGET_BLOCKED,
		COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US,
		COND_VORTIGAUNT_HEAL_VALID
	};

	ClearIgnoreConditions( g_VortConditions, ARRAYSIZE(g_VortConditions) );

	// Ignore these if we're damage only
 	if ( interrupt > GENERAL_INTERRUPTABILITY )
 		SetIgnoreConditions( g_VortConditions, ARRAYSIZE(g_VortConditions) );
}

//-----------------------------------------------------------------------------
// !!!HACKHACK - EP2 - Stop vortigaunt taking all physics damage to prevent it dying
// in freak accidents resembling spontaneous stress damage death (which are now impossible)
// Also stop it taking damage from flames: Fixes it being burnt to death from entity flames
// attached to random debris chunks while inside scripted sequences.
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( info.GetDamageType() & (DMG_CRUSH | DMG_BURN) )
		return 0;

	// vital vortigaunts (eg the vortigoth in ep2) take less damage from explosions
	// so that zombines don't blow them up disappointingly. They take less damage
	// still from antlion workers.
	if ( Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		// half damage
		CTakeDamageInfo subInfo = info;

		// take less damage from antlion worker acid/poison
		if ( info.GetAttacker()->Classify() == CLASS_ANTLION          &&
			 (info.GetDamageType() & ( DMG_ACID | DMG_POISON ))!=0
			)
		{
			subInfo.ScaleDamage( sk_vortigaunt_vital_antlion_worker_dmg.GetFloat() );
		}

		else if ( info.GetDamageType() & DMG_BLAST )
		{
			subInfo.ScaleDamage( 0.5f );
		}

		return BaseClass::OnTakeDamage_Alive( subInfo );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: Override move and shoot if we're following someone
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::ShouldMoveAndShoot( void )
{
	if ( m_FollowBehavior.IsActive() )
		return true;

	return BaseClass::ShouldMoveAndShoot();
}

//-----------------------------------------------------------------------------
// Purpose: notification from a grub that I squished it. This special case 
// function is necessary because what you would think to be the ordinary 
// channels are in fact missing: Event_KilledOther doesn't actually do anything
// and KilledNPC expects a BaseCombatCharacter, and always uses the same Speak
// line.
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::OnSquishedGrub( const CBaseEntity *pGrub )
{
	Speak(TLK_SQUISHED_GRUB);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::AimGun( void )
{
	// If our aim lock is on, don't bother
	if ( m_flAimDelay >= gpGlobals->curtime )
		return;

	// Aim at our target
	if ( GetEnemy() )
	{
		Vector vecShootOrigin;

		vecShootOrigin = Weapon_ShootPosition();
		Vector vecShootDir;
		
		// Aim where it is
		vecShootDir = GetShootEnemyDir( vecShootOrigin, false );

		if ( g_debug_vortigaunt_aim.GetBool() )
		{
			NDebugOverlay::Line( WorldSpaceCenter(), WorldSpaceCenter() + vecShootDir * 256.0f, 255, 0, 0, true, 0.1f );
		}
		
		SetAim( vecShootDir );
	}
	else
	{
		RelaxAim();
	}
}

//-----------------------------------------------------------------------------
// Purpose: A scripted sequence has interrupted us
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::OnStartScene( void )
{
	// Watch our hand state
	EndHandGlow( VORTIGAUNT_BEAM_ALL );
	m_fGlowChangeTime = gpGlobals->curtime + 0.1f;	// No more glows for this amount of time!

	BaseClass::OnStartScene();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsInterruptable( void )
{
	// Don't interrupt my attack schedule!
	if ( InAttackSequence() )
		return false;

	return BaseClass::IsInterruptable();
}

//-----------------------------------------------------------------------------
// Purpose: Start overriding our animations to "carry" an NPC
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputBeginCarryNPC( inputdata_t &indputdata )
{
	m_bCarryingNPC = true;
}

//-----------------------------------------------------------------------------
// Purpose: Stop overriding our animations for carrying an NPC
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEndCarryNPC( inputdata_t &indputdata )
{
	m_bCarryingNPC = false;
}

//-----------------------------------------------------------------------------
// Purpose: Turn off flinching under certain circumstances
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::CanFlinch( void )
{
	if ( IsActiveDynamicInteraction() )
		return false;

	if ( IsPlayingGesture( ACT_GESTURE_RANGE_ATTACK1 ) )
		return false;

	if ( IsCurSchedule( SCHED_VORTIGAUNT_DISPEL_ANTLIONS ) || IsCurSchedule( SCHED_RANGE_ATTACK1 ) )
		return false;

	return BaseClass::CanFlinch();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::OnUpdateShotRegulator( void )
{
	// Do nothing, we're not really running this code in a normal manner
	GetShotRegulator()->SetBurstInterval( 2.0f, 2.0f );
	GetShotRegulator()->SetBurstShotCountRange( 1, 1 );
	GetShotRegulator()->SetRestInterval( 2.0f, 2.0f );	
}

/*
IMPLEMENT_SERVERCLASS_ST( CVortigauntChargeToken, DT_VortigauntChargeToken )
	SendPropFloat( SENDINFO(m_flFadeOutTime), 0, SPROP_NOSCALE),	
	SendPropBool( SENDINFO(m_bFadeOut) ),
	SendPropFloat( SENDINFO(m_flScale), 0, SPROP_NOSCALE),
END_SEND_TABLE()
*/

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_vortigaunt, CNPC_Vortigaunt )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_TASK(TASK_VORTIGAUNT_HEAL)
	DECLARE_TASK(TASK_VORTIGAUNT_EXTRACT)
	DECLARE_TASK(TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT)
	DECLARE_TASK(TASK_VORTIGAUNT_WAIT_FOR_PLAYER)

	DECLARE_TASK( TASK_VORTIGAUNT_EXTRACT_WARMUP )
	DECLARE_TASK( TASK_VORTIGAUNT_EXTRACT_COOLDOWN )
	DECLARE_TASK( TASK_VORTIGAUNT_GET_HEAL_TARGET )
	DECLARE_TASK( TASK_VORTIGAUNT_DISPEL_ANTLIONS )

	DECLARE_ACTIVITY( ACT_VORTIGAUNT_AIM)
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_START_HEAL )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_HEAL_LOOP )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_END_HEAL )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_TO_ACTION )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_TO_IDLE )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_HEAL )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_DISPEL )
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_ANTLION_THROW )

	DECLARE_CONDITION( COND_VORTIGAUNT_CAN_HEAL )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_VALID )
	DECLARE_CONDITION( COND_VORTIGAUNT_DISPEL_ANTLIONS )

	DECLARE_SQUADSLOT( SQUAD_SLOT_HEAL_PLAYER )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_LEFT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_RIGHT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_POWERUP )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_SHOOT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_DONE )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTGLOW )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTBEAMS )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTSOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SWING_SOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SHOOT_SOUNDSTART )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_PAUSE )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_START_DISPEL )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ACCEL_DISPEL )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_DISPEL )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_START_HURT_GLOW )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_STOP_HURT_GLOW )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_START_HEAL_GLOW )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_STOP_HEAL_GLOW )

	//=========================================================
	// > SCHED_VORTIGAUNT_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_RANGE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_ANNOUNCE_ATTACK			0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_WAIT						0.2" // Wait a sec before killing beams
		""
		"	Interrupts"
		"		COND_NO_CUSTOM_INTERRUPTS"
	);


	//=========================================================
	// > SCHED_VORTIGAUNT_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_HEAL,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_STAND"
		"		TASK_STOP_MOVING				0"
		"		TASK_VORTIGAUNT_GET_HEAL_TARGET	0"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		350"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_PLAYER				0"
		"		TASK_VORTIGAUNT_HEAL			0"
		""
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							2"					// repick IDLESTAND every two seconds."
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_VORTIGAUNT_DISPEL_ANTLIONS"
		"		COND_VORTIGAUNT_CAN_HEAL"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_EXTRACT_BUGBAIT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_EXTRACT_BUGBAIT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_VORTIGAUNT_STAND"
		"		TASK_STOP_MOVING					0"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			128"				// Move within 128 of target ent (client)
		"		TASK_STOP_MOVING					0"
		"		TASK_VORTIGAUNT_WAIT_FOR_PLAYER		0"
		"		TASK_SPEAK_SENTENCE					500"				// Start extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_FACE_TARGET					0"
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_VORTIGAUNT_EXTRACT_WARMUP		0"
		"		TASK_VORTIGAUNT_EXTRACT				0"
		"		TASK_VORTIGAUNT_EXTRACT_COOLDOWN	0"
		"		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT	0"
		"		TASK_SPEAK_SENTENCE					501"				// Finish extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_WAIT							2"
		""
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_VORTIGAUNT_FACE_PLAYER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_FACE_PLAYER,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_TARGET_PLAYER		0"
		"		TASK_FACE_PLAYER		0"
		"		TASK_WAIT				3"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_VORTIGAUNT_DISPEL_ANTLIONS"
		"		COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR"
		"		COND_VORTIGAUNT_HEAL_TARGET_BLOCKED"
		"		COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_RUN_TO_PLAYER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_RUN_TO_PLAYER,

		"	Tasks"
		"		TASK_TARGET_PLAYER					0"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			350"
		""
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
	);	

	//=========================================================
	// > SCHED_VORTIGAUNT_DISPEL_ANTLIONS
	//=========================================================
	DEFINE_SCHEDULE
		(
		SCHED_VORTIGAUNT_DISPEL_ANTLIONS,

		"	Tasks"
		"		TASK_VORTIGAUNT_DISPEL_ANTLIONS	0"
		""
		"	Interrupts"
		"		COND_NO_CUSTOM_INTERRUPTS"
		);	

	//=========================================================
	//
	//=========================================================
	DEFINE_SCHEDULE
		(
		SCHED_VORT_FLEE_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COWER"
		"		 TASK_GET_PATH_AWAY_FROM_BEST_SOUND	600"
		"		 TASK_RUN_PATH_TIMED				1.5"
		"		 TASK_STOP_MOVING					0"
		""
		"	Interrupts"
		)

		//=========================================================
		//  > AlertFace	best sound
		//=========================================================
	DEFINE_SCHEDULE
		(
		SCHED_VORT_ALERT_FACE_BESTSOUND,

		"	Tasks"
		"		TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_SAVEPOSITION		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_DANGER"
		);
AI_END_CUSTOM_NPC()


//=============================================================================
// 
//  Charge Token 
//	
//=============================================================================

LINK_ENTITY_TO_CLASS( vort_charge_token, CVortigauntChargeToken );

BEGIN_DATADESC( CVortigauntChargeToken )
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLifetime, FIELD_TIME ),
	DEFINE_FIELD( m_bFadeOut, FIELD_BOOLEAN ),
	
	DEFINE_ENTITYFUNC( SeekThink ),
	DEFINE_ENTITYFUNC( SeekTouch ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CVortigauntChargeToken, DT_VortigauntChargeToken )
	SendPropBool( SENDINFO(m_bFadeOut) ),
END_SEND_TABLE()

CVortigauntChargeToken::CVortigauntChargeToken( void ) :
m_hTarget( NULL )
{
	m_bFadeOut = false;
}

//-----------------------------------------------------------------------------
// Purpose: Create a charge token for the player to collect
// Input  : &vecOrigin - Where we start
//			*pOwner - Who created us
//			*pTarget - Who we're seeking towards
//-----------------------------------------------------------------------------
CVortigauntChargeToken *CVortigauntChargeToken::CreateChargeToken( const Vector &vecOrigin, CBaseEntity *pOwner, CBaseEntity *pTarget )
{
	CVortigauntChargeToken *pToken = (CVortigauntChargeToken *) CreateEntityByName( "vort_charge_token" );
	if ( pToken == NULL )
		return NULL;

	// Set up our internal data
	UTIL_SetOrigin( pToken, vecOrigin );
	pToken->SetOwnerEntity( pOwner );
	pToken->SetTargetEntity( pTarget );
	pToken->SetThink( &CVortigauntChargeToken::SeekThink );
	pToken->SetTouch( &CVortigauntChargeToken::SeekTouch );
	pToken->Spawn();
	
	// Start out at the same velocity as our owner
	Vector vecInitialVelocity;
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(pOwner);
	if ( pAnimating != NULL )
	{
		vecInitialVelocity = pAnimating->GetGroundSpeedVelocity();
	}
	else
	{
		vecInitialVelocity = pTarget->GetSmoothedVelocity();				
	}

	// Start out at that speed
	pToken->SetAbsVelocity( vecInitialVelocity );

	return pToken;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVortigauntChargeToken::Precache( void )
{
	PrecacheParticleSystem( "vortigaunt_charge_token" );
}

//-----------------------------------------------------------------------------
// Purpose: We want to move through grates!
//-----------------------------------------------------------------------------
unsigned int CVortigauntChargeToken::PhysicsSolidMaskForEntity( void ) const 
{ 
	return MASK_SHOT; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVortigauntChargeToken::Spawn( void )
{
	Precache();

	// Point-sized
	UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );

	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetGravity( 0.0f );

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	SetNextThink( gpGlobals->curtime + 0.05f );

	m_flLifetime = gpGlobals->curtime + VORTIGAUNT_CURE_LIFESPAN;

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Creates an influence vector which causes the token to move away from obstructions
//-----------------------------------------------------------------------------
Vector CVortigauntChargeToken::GetSteerVector( const Vector &vecForward )
{
	Vector vecSteer = vec3_origin;
	Vector vecRight, vecUp;
	VectorVectors( vecForward, vecRight, vecUp );

	// Use two probes fanned out a head of us
	Vector vecProbe;
	float flSpeed = GetAbsVelocity().Length();

	// Try right 
	vecProbe = vecForward + vecRight;	
	vecProbe *= flSpeed;

	// We ignore multiple targets
	CTraceFilterSimpleList filterSkip( COLLISION_GROUP_NONE );
	filterSkip.AddEntityToIgnore( this );
	filterSkip.AddEntityToIgnore( GetOwnerEntity() );
	filterSkip.AddEntityToIgnore( m_hTarget );

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecProbe, MASK_SHOT, &filterSkip, &tr );
	vecSteer -= vecRight * 100.0f * ( 1.0f - tr.fraction );

	// Try left
	vecProbe = vecForward - vecRight;
	vecProbe *= flSpeed;

	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecProbe, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	vecSteer += vecRight * 100.0f * ( 1.0f - tr.fraction );

	return vecSteer;
}

#define VTOKEN_MAX_SPEED	320.0f	// U/sec
#define VTOKEN_ACCEL_SPEED	320.0f	// '

//-----------------------------------------------------------------------------
// Purpose: Move towards our target entity with accel/decel parameters
//-----------------------------------------------------------------------------
void CVortigauntChargeToken::SeekThink( void )
{
	// Move away from the creator and towards the target
	if ( m_hTarget == NULL || m_flLifetime < gpGlobals->curtime )
	{
		// TODO: Play an extinguish sound and fade out
		FadeAndDie();
		return;
	}

	// Find the direction towards our goal and start to go there
	Vector vecDir = ( m_hTarget->WorldSpaceCenter() - GetAbsOrigin() );
	VectorNormalize( vecDir );

	float flSpeed = GetAbsVelocity().Length();
	float flDelta = gpGlobals->curtime - GetLastThink();

	if ( flSpeed < VTOKEN_MAX_SPEED )
	{
		// Accelerate by the desired amount
		flSpeed += ( VTOKEN_ACCEL_SPEED * flDelta );
		if ( flSpeed > VTOKEN_MAX_SPEED )
		{
			flSpeed = VTOKEN_MAX_SPEED;
		}
	}

	// Steer!
	Vector vecRight, vecUp;
	VectorVectors( vecDir, vecRight, vecUp );
	Vector vecOffset = vec3_origin;
	vecOffset += vecUp * cos( gpGlobals->curtime * 20.0f ) * 200.0f * gpGlobals->frametime;
	vecOffset += vecRight * sin( gpGlobals->curtime * 15.0f ) * 200.0f * gpGlobals->frametime;
	
	vecOffset += GetSteerVector( vecDir );

	SetAbsVelocity( ( vecDir * flSpeed ) + vecOffset );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVortigauntChargeToken::SeekTouch( CBaseEntity	*pOther )
{
	// Make sure this is a player
	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( pPlayer == NULL )
		return;

	// FIXME: This probably isn't that interesting for single player missions
	if ( pPlayer != m_hTarget )
		return;

	// TODO: Play a special noise for this event!
	EmitSound( "NPC_Vortigaunt.SuitOn" );

	// Charge the suit's armor
	if ( pPlayer->ArmorValue() < sk_vortigaunt_armor_charge.GetInt() )
	{
		pPlayer->IncrementArmorValue( sk_vortigaunt_armor_charge_per_token.GetInt()+random->RandomInt( -1, 1 ), sk_vortigaunt_armor_charge.GetInt() );
	}

	// Stay attached to the thing we hit as we fade away
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	SetParent( pOther );

	// TODO: Play a "poof!" effect here?
	FadeAndDie();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flTime - 
//-----------------------------------------------------------------------------
void CVortigauntChargeToken::FadeAndDie( void )
{
	SetTouch( NULL );

	SetAbsVelocity( vec3_origin );

	m_bFadeOut = true;
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 2.0f );
}

//=============================================================================
// 
//  Dispel Effect
//	
//=============================================================================

LINK_ENTITY_TO_CLASS( vort_effect_dispel, CVortigauntEffectDispel );

BEGIN_DATADESC( CVortigauntEffectDispel )
	DEFINE_FIELD( m_bFadeOut, FIELD_BOOLEAN ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CVortigauntEffectDispel, DT_VortigauntEffectDispel )
	SendPropBool( SENDINFO(m_bFadeOut) ),
END_SEND_TABLE()

CVortigauntEffectDispel::CVortigauntEffectDispel( void )
{
	m_bFadeOut = false;
}

//-----------------------------------------------------------------------------
// Purpose: Create a charge token for the player to collect
// Input  : &vecOrigin - Where we start
//			*pOwner - Who created us
//			*pTarget - Who we're seeking towards
//-----------------------------------------------------------------------------
CVortigauntEffectDispel *CVortigauntEffectDispel::CreateEffectDispel( const Vector &vecOrigin, CBaseEntity *pOwner, CBaseEntity *pTarget )
{
	CVortigauntEffectDispel *pToken = (CVortigauntEffectDispel *) CreateEntityByName( "vort_effect_dispel" );
	if ( pToken == NULL )
		return NULL;

	// Set up our internal data
	UTIL_SetOrigin( pToken, vecOrigin );
	pToken->SetOwnerEntity( pOwner );
	pToken->Spawn();

	return pToken;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVortigauntEffectDispel::Spawn( void )
{
	Precache();

	UTIL_SetSize( this, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID );

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flTime - 
//-----------------------------------------------------------------------------
void CVortigauntEffectDispel::FadeAndDie( void )
{
	m_bFadeOut = true;
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 2.0f );
}


//=============================================================================
// 
//  Flesh effect target (used for orchestrating the "Invisible Alyx" moment
//	
//=============================================================================

#ifdef HL2_EPISODIC

class CFleshEffectTarget : public CPointEntity
{
	DECLARE_CLASS( CFleshEffectTarget, CPointEntity );

public:
	void InputSetRadius( inputdata_t &inputData );

	virtual void Spawn( void )
	{
		BaseClass::Spawn();

		AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
	}

private:

	CNetworkVar( float, m_flRadius );
	CNetworkVar( float, m_flScaleTime );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();	
};

LINK_ENTITY_TO_CLASS( point_flesh_effect_target, CFleshEffectTarget );

BEGIN_DATADESC( CFleshEffectTarget )

	DEFINE_FIELD( m_flScaleTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),

	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetRadius", InputSetRadius ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFleshEffectTarget, DT_FleshEffectTarget )
	SendPropFloat( SENDINFO(m_flRadius), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flScaleTime), 0, SPROP_NOSCALE),
END_SEND_TABLE()

void CFleshEffectTarget::InputSetRadius( inputdata_t &inputData )
{
	Vector vecRadius;
	inputData.value.Vector3D( vecRadius );

	m_flRadius = vecRadius.x;
	m_flScaleTime = vecRadius.y;
}

#endif // HL2_EPISODIC
