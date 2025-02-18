//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "soundent.h"
#include "npcevent.h"
#include "globalstate.h"
#include "ai_squad.h"
#include "ai_tacticalservices.h"
#include "npc_manhack.h"
#include "npc_metropolice.h"
#include "weapon_stunstick.h"
#include "basegrenade_shared.h"
#include "ai_route.h"
#include "hl2_player.h"
#include "iservervehicle.h"
#include "items.h"
#include "hl2_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define SF_METROPOLICE_					0x00010000
#define SF_METROPOLICE_SIMPLE_VERSION		0x00020000
#define SF_METROPOLICE_ALWAYS_STITCH		0x00080000
#define SF_METROPOLICE_NOCHATTER			0x00100000
#define SF_METROPOLICE_ARREST_ENEMY			0x00200000
#define SF_METROPOLICE_NO_FAR_STITCH		0x00400000
#define SF_METROPOLICE_NO_MANHACK_DEPLOY	0x00800000
#define SF_METROPOLICE_ALLOWED_TO_RESPOND	0x01000000
#define SF_METROPOLICE_MID_RANGE_ATTACK		0x02000000

#define METROPOLICE_MID_RANGE_ATTACK_RANGE	3500.0f

#define METROPOLICE_SQUAD_STITCH_MIN_INTERVAL	1.0f
#define METROPOLICE_SQUAD_STITCH_MAX_INTERVAL	1.2f

#define AIM_ALONG_SIDE_LINE_OF_DEATH_DISTANCE		300.0f
#define AIM_ALONG_SIDE_STEER_DISTANCE				200.0f
#define AIM_ALONG_SIDE_DEFAULT_STITCH_LENGTH		750.0f
#define AIM_ALONG_SIDE_LINE_OF_DEATH_LEAD_TIME		0.0f
#define AIM_ALONG_SIDE_LINE_INITIAL_DRAW_FRACTION	0.2f

#define AIM_BEHIND_DEFAULT_STITCH_LENGTH		1000.0f
#define AIM_BEHIND_MINIMUM_DISTANCE				650.0f
#define AIM_BEHIND_STEER_DISTANCE				150.0f

#define RECENT_DAMAGE_INTERVAL		3.0f
#define RECENT_DAMAGE_THRESHOLD		0.2f

#define VEHICLE_PREDICT_ACCELERATION		333.0f
#define VEHICLE_PREDICT_MAX_SPEED			600.0f

#define	METROPOLICE_MAX_WARNINGS	3

#define	METROPOLICE_BODYGROUP_MANHACK	1

enum
{
	// NOTE: Exact #s are important, since they are referred to by number in schedules below

	METROPOLICE_SENTENCE_FREEZE			= 0,
	METROPOLICE_SENTENCE_HES_OVER_HERE	= 1,
	METROPOLICE_SENTENCE_HES_RUNNING	= 2,
	METROPOLICE_SENTENCE_TAKE_HIM_DOWN	= 3,	
	METROPOLICE_SENTENCE_ARREST_IN_POSITION	= 4,
	METROPOLICE_SENTENCE_DEPLOY_MANHACK	= 5,
	METROPOLICE_SENTENCE_MOVE_INTO_POSITION	= 6,
	METROPOLICE_SENTENCE_HEARD_SOMETHING	= 7,
};

enum
{
	METROPOLICE_ANNOUNCE_ATTACK_PRIMARY = 1,
	METROPOLICE_ANNOUNCE_ATTACK_SECONDARY,
	METROPOLICE_ANNOUNCE_ATTACK_HARASS,
};

enum
{
	METROPOLICE_CHATTER_WAIT_FOR_RESPONSE = 0,
	METROPOLICE_CHATTER_ASK_QUESTION = 1,
	METROPOLICE_CHATTER_RESPONSE = 2,

	METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT = 2,
};


enum SpeechMemory_t
{
	bits_MEMORY_PAIN_LIGHT_SOUND	= bits_MEMORY_CUSTOM1,
	bits_MEMORY_PAIN_HEAVY_SOUND	= bits_MEMORY_CUSTOM2,
	bits_MEMORY_PLAYER_HURT			= bits_MEMORY_CUSTOM3,
	bits_MEMORY_PLAYER_HARASSED		= bits_MEMORY_CUSTOM4,
};

//Metrocop
int	g_interactionMetrocopStartedStitch = 0;
int g_interactionMetrocopIdleChatter = 0;
int g_interactionMetrocopClearSentenceQueues = 0;

extern int g_interactionHitByPlayerThrownPhysObj;

ConVar	sk_metropolice_stitch_reaction( "sk_metropolice_stitch_reaction","1.0");
ConVar	sk_metropolice_stitch_tight_hitcount( "sk_metropolice_stitch_tight_hitcount","2");
ConVar	sk_metropolice_stitch_at_hitcount( "sk_metropolice_stitch_at_hitcount","1");
ConVar	sk_metropolice_stitch_behind_hitcount( "sk_metropolice_stitch_behind_hitcount","3");
ConVar	sk_metropolice_stitch_along_hitcount( "sk_metropolice_stitch_along_hitcount","2");


ConVar	sk_metropolice_health( "sk_metropolice_health","0");
ConVar	sk_metropolice_simple_health( "sk_metropolice_simple_health","26");
ConVar	sk_metropolice_stitch_distance( "sk_metropolice_stitch_distance","1000");

ConVar	metropolice_chase_use_follow( "metropolice_chase_use_follow", "0" );
ConVar  metropolice_move_and_melee("metropolice_move_and_melee", "1" );
ConVar  metropolice_charge("metropolice_charge", "1" );

// How many clips of pistol ammo a metropolice carries.
#define METROPOLICE_NUM_CLIPS			5
#define METROPOLICE_BURST_RELOAD_COUNT	20

int AE_METROPOLICE_BATON_ON;
int	AE_METROPOLICE_BATON_OFF;
int AE_METROPOLICE_SHOVE;
int AE_METROPOLICE_START_DEPLOY;
int AE_METROPOLICE_DRAW_PISTOL;		// was	50
int AE_METROPOLICE_DEPLOY_MANHACK;	// was	51

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_POLICE_CHARGE_ENEMY = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_POLICE_HARASS, // Yell at the player with a megaphone, etc.
	SQUAD_SLOT_POLICE_DEPLOY_MANHACK,
	SQUAD_SLOT_POLICE_ADVANCE,
	SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1,
	SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2,
	SQUAD_SLOT_POLICE_COVERING_FIRE1,
	SQUAD_SLOT_POLICE_COVERING_FIRE2,
	SQUAD_SLOT_POLICE_ARREST_ENEMY,
};

//=========================================================
// Metro Police  Activities
//=========================================================
int ACT_METROPOLICE_DRAW_PISTOL;
int ACT_METROPOLICE_DEPLOY_MANHACK;
int ACT_METROPOLICE_FLINCH_BEHIND;

int	ACT_WALK_BATON;
int	ACT_IDLE_ANGRY_BATON;
int	ACT_PUSH_PLAYER;
int ACT_MELEE_ATTACK_THRUST;
int ACT_ACTIVATE_BATON;
int ACT_DEACTIVATE_BATON;
 
LINK_ENTITY_TO_CLASS( npc_metropolice, CNPC_MetroPolice );

BEGIN_DATADESC( CNPC_MetroPolice )

	DEFINE_EMBEDDED( m_BatonSwingTimer ),	
	DEFINE_EMBEDDED( m_NextChargeTimer ),
	DEFINE_FIELD( m_flBatonDebounceTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bShouldActivateBaton, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iPistolClips, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_fWeaponDrawn, FIELD_BOOLEAN, "weapondrawn" ),
	DEFINE_FIELD( m_LastShootSlot, FIELD_INTEGER ),
	DEFINE_EMBEDDED( m_TimeYieldShootSlot ),
	DEFINE_EMBEDDED( m_Sentences ),
	DEFINE_FIELD( m_bPlayerIsNear, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_vecBurstTargetPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecBurstDelta, FIELD_VECTOR ),
	DEFINE_FIELD( m_nBurstHits, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxBurstHits, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBurstPredictTime, FIELD_TIME ),
	DEFINE_FIELD( m_nBurstReloadCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_vecBurstLineOfDeathDelta, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecBurstLineOfDeathOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBurstSteerDistance, FIELD_FLOAT ),
	DEFINE_FIELD( m_nBurstMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_nBurstSteerMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_vecBurstPredictedVelocityDir, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecBurstPredictedSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flValidStitchTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLedgeCheckTime, FIELD_TIME ),
	DEFINE_FIELD( m_flTaskCompletionTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastPhysicsFlinchTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastDamageFlinchTime, FIELD_TIME ),

	DEFINE_FIELD( m_hManhack, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBlockingProp, FIELD_EHANDLE ),

	DEFINE_FIELD( m_nRecentDamage, FIELD_INTEGER ),
	DEFINE_FIELD( m_flRecentDamageTime, FIELD_TIME ),

	DEFINE_FIELD( m_flNextPainSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLostSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nIdleChatterType, FIELD_INTEGER ),

	DEFINE_FIELD( m_bSimpleCops, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLastHitYaw, FIELD_FLOAT ),

	DEFINE_FIELD( m_bPlayerTooClose,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bKeepFacingPlayer,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flChasePlayerTime,	FIELD_TIME ),
	DEFINE_FIELD( m_vecPreChaseOrigin,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flPreChaseYaw,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nNumWarnings,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iNumPlayerHits,		FIELD_INTEGER ),

	//								m_ActBusyBehavior (auto saved by AI)
	//								m_StandoffBehavior (auto saved by AI)
	//								m_AssaultBehavior (auto saved by AI)
	//								m_FuncTankBehavior (auto saved by AI)
	//								m_RappelBehavior (auto saved by AI)
	//								m_PolicingBehavior (auto saved by AI)
	//								m_FollowBehavior (auto saved by AI)

	DEFINE_KEYFIELD( m_iManhacks, FIELD_INTEGER, "manhacks" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableManhackToss", InputEnableManhackToss ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPoliceGoal", InputSetPoliceGoal ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ActivateBaton", InputActivateBaton ),
	
	DEFINE_USEFUNC( PrecriminalUse ),

	DEFINE_OUTPUT( m_OnStunnedPlayer,	"OnStunnedPlayer" ),
	DEFINE_OUTPUT( m_OnCupCopped, "OnCupCopped" ),

END_DATADESC()

//------------------------------------------------------------------------------

float CNPC_MetroPolice::gm_flTimeLastSpokePeek;

//------------------------------------------------------------------------------
// Purpose 
//------------------------------------------------------------------------------
CBaseEntity *CNPC_MetroPolice::CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC )
{
	// If only a length is given assume we want to trace in our facing direction
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vStart = GetAbsOrigin();

	// The ideal place to start the trace is in the center of the attacker's bounding box.
	// however, we need to make sure there's enough clearance. Some of the smaller monsters aren't 
	// as big as the hull we try to trace with. (SJB)
	float flVerticalOffset = WorldAlignSize().z * 0.5;

	if( flVerticalOffset < maxs.z )
	{
		// There isn't enough room to trace this hull, it's going to drag the ground.
		// so make the vertical offset just enough to clear the ground.
		flVerticalOffset = maxs.z + 1.0;
	}

	vStart.z += flVerticalOffset;
	Vector vEnd = vStart + (forward * flDist );
	return CheckTraceHullAttack( vStart, vEnd, mins, maxs, iDamage, iDmgType, forceScale, bDamageAnyNPC );
}

//------------------------------------------------------------------------------
// Melee filter for police
//------------------------------------------------------------------------------
class CTraceFilterMetroPolice : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterMetroPolice );
	
	CTraceFilterMetroPolice( const IHandleEntity *passentity, int collisionGroup, CTakeDamageInfo *dmgInfo, float flForceScale, bool bDamageAnyNPC )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_dmgInfo(dmgInfo), m_pHit(NULL), m_flForceScale(flForceScale), m_bDamageAnyNPC(bDamageAnyNPC)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			if ( pEntity->m_takedamage == DAMAGE_NO )
				return false;

			// Translate the vehicle into its driver for damage
			if ( pEntity->GetServerVehicle() != NULL )
			{
				CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

				if ( pDriver != NULL )
				{
					pEntity = pDriver;
				}
			}
	
			Vector	attackDir = pEntity->WorldSpaceCenter() - m_dmgInfo->GetAttacker()->WorldSpaceCenter();
			VectorNormalize( attackDir );

			CTakeDamageInfo info = (*m_dmgInfo);				
			CalculateMeleeDamageForce( &info, attackDir, info.GetAttacker()->WorldSpaceCenter(), m_flForceScale );

			if( !(pEntity->GetFlags() & FL_ONGROUND) )
			{
				// Don't hit airborne entities so hard. They fly farther since
				// there's no friction with the ground.
				info.ScaleDamageForce( 0.001 );
			}

			CBaseCombatCharacter *pBCC = info.GetAttacker()->MyCombatCharacterPointer();
			CBaseCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

			// Only do these comparisons between NPCs
			if ( pBCC && pVictimBCC )
			{
				// Can only damage other NPCs that we hate
				if ( m_bDamageAnyNPC || pBCC->IRelationType( pEntity ) == D_HT || pEntity->IsPlayer() )
				{
					if ( info.GetDamage() )
					{
						// If gordon's a criminal, do damage now
						if ( !pEntity->IsPlayer() || GlobalEntity_GetState( "gordon_precriminal" ) == GLOBAL_OFF )
						{
							if ( pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->IsSuitEquipped() )
							{
								info.ScaleDamage( .25 );
								info.ScaleDamageForce( .25 );
							}

							pEntity->TakeDamage( info );
						}
					}
					
					m_pHit = pEntity;
					return true;
				}
			}
			else
			{
				// Make sure if the player is holding this, he drops it
				Pickup_ForcePlayerToDropThisObject( pEntity );

				// Otherwise just damage passive objects in our way
				if ( info.GetDamage() )
				{
					pEntity->TakeDamage( info );
				}
			}
		}

		return false;
	}

public:
	const IHandleEntity *m_pPassEnt;
	int					m_collisionGroup;
	CTakeDamageInfo		*m_dmgInfo;
	CBaseEntity			*m_pHit;
	float				m_flForceScale;
	bool				m_bDamageAnyNPC;
};

//------------------------------------------------------------------------------
// Purpose :	start and end trace position, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CNPC_MetroPolice::CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC )
{

	CTakeDamageInfo	dmgInfo( this, this, iDamage, DMG_SLASH );
	
	CTraceFilterMetroPolice traceFilter( this, COLLISION_GROUP_NONE, &dmgInfo, flForceScale, bDamageAnyNPC );

	Ray_t ray;
	ray.Init( vStart, vEnd, mins, maxs );

	trace_t tr;
	enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );

	CBaseEntity *pEntity = traceFilter.m_pHit;
	
	if ( pEntity == NULL )
	{
		// See if perhaps I'm trying to claw/bash someone who is standing on my head.
		Vector vecTopCenter;
		Vector vecEnd;
		Vector vecMins, vecMaxs;

		// Do a tracehull from the top center of my bounding box.
		vecTopCenter = GetAbsOrigin();
		CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		vecTopCenter.z = vecMaxs.z + 1.0f;
		vecEnd = vecTopCenter;
		vecEnd.z += 2.0f;
		
		ray.Init( vecTopCenter, vEnd, mins, maxs );
		enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

		pEntity = traceFilter.m_pHit;
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// My buddies got killed!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::NotifyDeadFriend( CBaseEntity* pFriend )
{
	BaseClass::NotifyDeadFriend(pFriend);

	if ( pFriend == m_hManhack )
	{
		m_Sentences.Speak( "METROPOLICE_MANHACK_KILLED", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
		DevMsg("My manhack died!\n");
		m_hManhack = NULL;
		return;
	}

	// No notifications for squadmates' dead manhacks
	if ( FClassnameIs( pFriend, "npc_manhack" ) )
		return;

	// Reset idle chatter, we may never get a response back
	if ( m_nIdleChatterType == METROPOLICE_CHATTER_WAIT_FOR_RESPONSE )
	{
		m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION;
	}

	if ( GetSquad()->NumMembers() < 2 )
	{
		m_Sentences.Speak( "METROPOLICE_LAST_OF_SQUAD", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		return;
	}

	m_Sentences.Speak( "METROPOLICE_MAN_DOWN", SENTENCE_PRIORITY_MEDIUM );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNPC_MetroPolice::CNPC_MetroPolice()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnScheduleChange()
{
	BaseClass::OnScheduleChange();

	if ( GetEnemy() && HasCondition( COND_ENEMY_DEAD ) )
	{
		AnnounceEnemyKill( GetEnemy() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Speak any queued sentences
	m_Sentences.UpdateSentenceQueue();

	// Look at near players, always
	m_bPlayerIsNear = false;
	if ( PlayerIsCriminal() == false )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
		
		if ( pPlayer && ( pPlayer->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr() < (128*128) )
		{
			m_bPlayerIsNear = true;
			AddLookTarget( pPlayer, 0.75f, 5.0f );
			
			if ( ( m_PolicingBehavior.IsEnabled() == false ) && ( m_nNumWarnings >= METROPOLICE_MAX_WARNINGS ) )
			{
				m_flBatonDebounceTime = gpGlobals->curtime + random->RandomFloat( 2.5f, 4.0f );
				SetTarget( pPlayer );
				SetBatonState( true );
			}
		}
		else 
		{
			if ( m_PolicingBehavior.IsEnabled() == false && gpGlobals->curtime > m_flBatonDebounceTime )
			{
				SetBatonState( false );
			}

			m_bKeepFacingPlayer = false;
		}
	}

	if( IsOnFire() )
	{
		SetCondition( COND_METROPOLICE_ON_FIRE );
	}
	else
	{
		ClearCondition( COND_METROPOLICE_ON_FIRE );
	}

	if (gpGlobals->curtime > m_flRecentDamageTime + RECENT_DAMAGE_INTERVAL)
	{
		m_nRecentDamage = 0;
		m_flRecentDamageTime = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &move - 
//			flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	// Don't do this if we're scripted
	if ( IsInAScript() )
		return BaseClass::OverrideMoveFacing( move, flInterval );
  	
	// ROBIN: Disabled at request of mapmakers for now
	/*
  	// If we're moving during a police sequence, always face our target
	if ( m_PolicingBehavior.IsEnabled() )
  	{	
		CBaseEntity *pTarget = m_PolicingBehavior.GetGoalTarget();

		if ( pTarget )
		{
			AddFacingTarget( pTarget, pTarget->WorldSpaceCenter(), 1.0f, 0.2f );
		}
	}
	*/

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Precache( void )
{
	if ( HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
	{
		SetModelName( AllocPooledString("models/police_cheaple.mdl" ) );
	}
	else
	{
		SetModelName( AllocPooledString("models/police.mdl") );
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "npc_manhack" );

	PrecacheScriptSound( "NPC_Metropolice.Shove" );
	PrecacheScriptSound( "NPC_MetroPolice.WaterSpeech" );
	PrecacheScriptSound( "NPC_MetroPolice.HidingSpeech" );
	enginesound->PrecacheSentenceGroup( "METROPOLICE" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Create components
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_Sentences.Init( this, "NPC_Metropolice.SentenceParameters" );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Spawn( void )
{
	Precache();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	SetModel( STRING( GetModelName() ) );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION; 
	m_bSimpleCops = HasSpawnFlags( SF_METROPOLICE_SIMPLE_VERSION );
	if ( HasSpawnFlags( SF_METROPOLICE_NOCHATTER ) )
	{
		AddSpawnFlags( SF_NPC_GAG );
	}

	if (!m_bSimpleCops)
	{
		m_iHealth = sk_metropolice_health.GetFloat();
	}
	else
	{
		m_iHealth = sk_metropolice_simple_health.GetFloat();
	}

	m_flFieldOfView		= -0.2;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	if ( !HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
	{
		CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
		CapabilitiesAdd( bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT );
	}
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_NO_HIT_SQUADMATES );
	CapabilitiesAdd( bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_DUCK | bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );

	m_nBurstHits = 0;
	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_iPistolClips = METROPOLICE_NUM_CLIPS;

	NPCInit();

	// NOTE: This must occur *after* init, since init sets default dist look
	if ( HasSpawnFlags( SF_METROPOLICE_MID_RANGE_ATTACK ) )
	{
		m_flDistTooFar = METROPOLICE_MID_RANGE_ATTACK_RANGE;
		SetDistLook( METROPOLICE_MID_RANGE_ATTACK_RANGE );
	}

	m_hManhack = NULL;

	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pWeapon;

		pWeapon = GetActiveWeapon();

		if( !FClassnameIs( pWeapon, "weapon_pistol" ) )
		{
			m_fWeaponDrawn = true;
		}
		
		if( !m_fWeaponDrawn ) 
		{
			GetActiveWeapon()->AddEffects( EF_NODRAW );
		}
	}


	m_TimeYieldShootSlot.Set( 2, 6 );

	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

	m_bShouldActivateBaton = false;
	m_flValidStitchTime = -1.0f;
	m_flNextLedgeCheckTime = -1.0f;
	m_nBurstReloadCount = METROPOLICE_BURST_RELOAD_COUNT;
	SetBurstMode( false );

	// Clear out spawnflag if we're missing the smg1
	if( HasSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH ) )
	{
		if ( !Weapon_OwnsThisType( "weapon_smg1" ) )
		{
			Warning( "Warning! Metrocop is trying to use the stitch behavior but he has no smg1!\n" );
			RemoveSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH );
		}
	}

	m_nNumWarnings = 0;
	m_bPlayerTooClose = false;
	m_bKeepFacingPlayer = false;
	m_flChasePlayerTime = 0;
	m_vecPreChaseOrigin = vec3_origin;
	m_flPreChaseYaw = 0;

	SetUse( &CNPC_MetroPolice::PrecriminalUse );

	// Start us with a visible manhack if we have one
	if ( m_iManhacks )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_MANHACK, true );
	}
}


//-----------------------------------------------------------------------------
// Update weapon ranges
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	if ( HasSpawnFlags(SF_METROPOLICE_MID_RANGE_ATTACK) && GetActiveWeapon() )
	{
		GetActiveWeapon()->m_fMaxRange1 = METROPOLICE_MID_RANGE_ATTACK_RANGE;
		GetActiveWeapon()->m_fMaxRange2 = METROPOLICE_MID_RANGE_ATTACK_RANGE;
	}
}


//-----------------------------------------------------------------------------
// FuncTankBehavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakFuncTankSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case FUNCTANK_SENTENCE_MOVE_TO_MOUNT:
		m_Sentences.Speak( "METROPOLICE_FT_APPROACH", SENTENCE_PRIORITY_MEDIUM );
		break;

	case FUNCTANK_SENTENCE_JUST_MOUNTED:
		m_Sentences.Speak( "METROPOLICE_FT_MOUNT", SENTENCE_PRIORITY_HIGH );
		break;

	case FUNCTANK_SENTENCE_SCAN_FOR_ENEMIES:
		m_Sentences.Speak( "METROPOLICE_FT_SCAN", SENTENCE_PRIORITY_NORMAL );
		break;

	case FUNCTANK_SENTENCE_DISMOUNTING:
		m_Sentences.Speak( "METROPOLICE_FT_DISMOUNT", SENTENCE_PRIORITY_HIGH );
		break;
	}
}


//-----------------------------------------------------------------------------
// Standoff Behavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakStandoffSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case STANDOFF_SENTENCE_BEGIN_STANDOFF:
		m_Sentences.Speak( "METROPOLICE_SO_BEGIN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_SQUAD_LEADER );
		break;

	case STANDOFF_SENTENCE_END_STANDOFF:
		m_Sentences.Speak( "METROPOLICE_SO_END", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_SQUAD_LEADER );
		break;

	case STANDOFF_SENTENCE_OUT_OF_AMMO:
		AnnounceOutOfAmmo( );
		break;

	case STANDOFF_SENTENCE_FORCED_TAKE_COVER:
		m_Sentences.Speak( "METROPOLICE_SO_FORCE_COVER" );
		break;

	case STANDOFF_SENTENCE_STAND_CHECK_TARGET:
		if ( gm_flTimeLastSpokePeek != 0 && gpGlobals->curtime - gm_flTimeLastSpokePeek > 20 )
		{
			m_Sentences.Speak( "METROPOLICE_SO_PEEK" );
			gm_flTimeLastSpokePeek = gpGlobals->curtime;
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Assault Behavior-related sentences
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakAssaultSentence( int nSentenceType )
{
	switch ( nSentenceType )
	{
	case ASSAULT_SENTENCE_HIT_RALLY_POINT:
		m_Sentences.SpeakQueued( "METROPOLICE_AS_HIT_RALLY", SENTENCE_PRIORITY_NORMAL );
		break;

	case ASSAULT_SENTENCE_HIT_ASSAULT_POINT:
		m_Sentences.SpeakQueued( "METROPOLICE_AS_HIT_ASSAULT", SENTENCE_PRIORITY_NORMAL );
		break;

	case ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_RALLY:
		if ( m_Sentences.Speak( "METROPOLICE_AS_ADV_RALLY", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_SQUAD_LEADER ) >= 0 )
		{
			GetSquad()->BroadcastInteraction( g_interactionMetrocopClearSentenceQueues, NULL );
		}
		break;

	case ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT:
		if ( m_Sentences.Speak( "METROPOLICE_AS_ADV_ASSAULT", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_SQUAD_LEADER ) >= 0 )
		{
			GetSquad()->BroadcastInteraction( g_interactionMetrocopClearSentenceQueues, NULL );
		}
		break;

	case ASSAULT_SENTENCE_COVER_NO_AMMO:
		AnnounceOutOfAmmo( );
		break;

	case ASSAULT_SENTENCE_UNDER_ATTACK:
		m_Sentences.Speak( "METROPOLICE_GO_ALERT" );
		break;
	}
}


//-----------------------------------------------------------------------------
// Speaking while using TASK_SPEAK_SENTENCE
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SpeakSentence( int nSentenceType )
{
	if ( !PlayerIsCriminal() )
		return;

	if ( nSentenceType >= SENTENCE_BASE_BEHAVIOR_INDEX )
	{
		if ( GetRunningBehavior() == &m_FuncTankBehavior )
		{
			SpeakFuncTankSentence( nSentenceType );
			return;
		}

		if ( GetRunningBehavior() == &m_StandoffBehavior )
		{
			SpeakStandoffSentence( nSentenceType );
			return;
		}

		if ( GetRunningBehavior() == &m_AssaultBehavior )
		{
			SpeakAssaultSentence( nSentenceType );
			return;
		}
	}

	switch ( nSentenceType )
	{
	case METROPOLICE_SENTENCE_FREEZE:
		m_Sentences.Speak( "METROPOLICE_FREEZE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_HES_OVER_HERE:
		m_Sentences.Speak( "METROPOLICE_OVER_HERE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_HES_RUNNING:
		m_Sentences.Speak( "METROPOLICE_HES_RUNNING", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_TAKE_HIM_DOWN:
		m_Sentences.Speak( "METROPOLICE_TAKE_HIM_DOWN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_ARREST_IN_POSITION:
		m_Sentences.Speak( "METROPOLICE_ARREST_IN_POS", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
		break;

	case METROPOLICE_SENTENCE_DEPLOY_MANHACK:
		m_Sentences.Speak( "METROPOLICE_DEPLOY_MANHACK" );
		break;

	case METROPOLICE_SENTENCE_MOVE_INTO_POSITION:
		{
			CBaseEntity *pEntity = GetEnemy();
			
			// NOTE: This is a good time to check to see if the player is hurt.
			// Have the cops notice this and call out
			if ( pEntity && !HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
			{
				if ( pEntity->IsPlayer() && (pEntity->GetHealth() <= 20) )
				{
					if ( !HasMemory(bits_MEMORY_PLAYER_HURT) ) 
					{
						if ( m_Sentences.Speak( "METROPOLICE_PLAYERHIT", SENTENCE_PRIORITY_HIGH ) >= 0 )
						{
							m_pSquad->SquadRemember(bits_MEMORY_PLAYER_HURT);
						}
					}
				}

				if ( GetNavigator()->GetPath()->GetPathLength() > 20 * 12.0f )
				{
					m_Sentences.Speak( "METROPOLICE_FLANK" );
				}
			}
		}
		break;

	case METROPOLICE_SENTENCE_HEARD_SOMETHING:
		if ( ( GetState() == NPC_STATE_ALERT ) || ( GetState() == NPC_STATE_IDLE ) )
		{
			m_Sentences.Speak( "METROPOLICE_HEARD_SOMETHING", SENTENCE_PRIORITY_MEDIUM );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Speaking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceEnemyType( CBaseEntity *pEnemy )
{
	if ( !pEnemy || !m_pSquad )
		return;

	// Don't announce enemies when the player isn't a criminal
	if ( !PlayerIsCriminal() )
		return;

	// Don't announce enemies when I'm in arrest behavior
	if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
		return;

	if ( m_pSquad->IsLeader( this ) || ( m_pSquad->GetLeader() && m_pSquad->GetLeader()->GetEnemy() != GetEnemy() ) )
	{
		// First contact, and I'm the squad leader.
		const char *pSentenceName = "METROPOLICE_MONST";
		switch ( pEnemy->Classify() )
		{
		case CLASS_PLAYER:
			{
				CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( pEnemy );
				if ( pPlayer && pPlayer->IsInAVehicle() )
				{
					pSentenceName = "METROPOLICE_MONST_PLAYER_VEHICLE";
				}
				else
				{
					pSentenceName = "METROPOLICE_MONST_PLAYER";
				}
			}
			break;

		case CLASS_PLAYER_ALLY:
		case CLASS_CITIZEN_REBEL:
		case CLASS_CITIZEN_PASSIVE:
		case CLASS_VORTIGAUNT:
			pSentenceName = "METROPOLICE_MONST_CITIZENS";
			break;

		case CLASS_PLAYER_ALLY_VITAL:
			pSentenceName = "METROPOLICE_MONST_CHARACTER";
			break;

		case CLASS_ANTLION:
			pSentenceName = "METROPOLICE_MONST_BUGS";
			break;

		case CLASS_ZOMBIE:
			pSentenceName = "METROPOLICE_MONST_ZOMBIES";
			break;

		case CLASS_HEADCRAB:
		case CLASS_BARNACLE:
			pSentenceName = "METROPOLICE_MONST_PARASITES";
			break;
		}

		m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
	}
	else
	{
		if ( m_pSquad->GetLeader() && FOkToMakeSound( SENTENCE_PRIORITY_MEDIUM ) )
		{
			// squelch anything that isn't high priority so the leader can speak
			JustMadeSound( SENTENCE_PRIORITY_MEDIUM );	
		}
	}

}


//-----------------------------------------------------------------------------
// Speaking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceEnemyKill( CBaseEntity *pEnemy )
{
	if ( !pEnemy )
		return;

	const char *pSentenceName = "METROPOLICE_KILL_MONST";
	switch ( pEnemy->Classify() )
	{
	case CLASS_PLAYER:
		pSentenceName = "METROPOLICE_KILL_PLAYER";
		break;

	// no sentences for these guys yet
	case CLASS_PLAYER_ALLY:
	case CLASS_CITIZEN_REBEL:
	case CLASS_CITIZEN_PASSIVE:
	case CLASS_VORTIGAUNT:
		pSentenceName = "METROPOLICE_KILL_CITIZENS";
		break;

	case CLASS_PLAYER_ALLY_VITAL:
		pSentenceName = "METROPOLICE_KILL_CHARACTER";
		break;

	case CLASS_ANTLION:
		pSentenceName = "METROPOLICE_KILL_BUGS";
		break;

	case CLASS_ZOMBIE:
		pSentenceName = "METROPOLICE_KILL_ZOMBIES";
		break;

	case CLASS_HEADCRAB:
	case CLASS_BARNACLE:
		pSentenceName = "METROPOLICE_KILL_PARASITES";
		break;
	}

	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH );
}


//-----------------------------------------------------------------------------
// Announce out of ammo
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceOutOfAmmo( )
{
	if ( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		m_Sentences.Speak( "METROPOLICE_COVER_NO_AMMO" );
	}
	else
	{
		m_Sentences.Speak( "METROPOLICE_COVER_LOW_AMMO" );
	}
}

//-----------------------------------------------------------------------------
// We're taking cover from danger
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceTakeCoverFromDanger( CSound *pSound )
{
	CBaseEntity *pSoundOwner = pSound->m_hOwner;
	if ( pSoundOwner )
	{
		CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>(pSoundOwner);
		if ( pGrenade )
		{
			if ( IRelationType( pGrenade->GetThrower() ) != D_LI )
			{
				// special case call out for enemy grenades
				m_Sentences.Speak( "METROPOLICE_DANGER_GREN", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			}
			return;
		}

		if ( pSoundOwner->GetServerVehicle() )
		{
			m_Sentences.Speak( "METROPOLICE_DANGER_VEHICLE", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			return;
		}

		if ( FClassnameIs( pSoundOwner, "npc_manhack" ) )
		{
			if ( pSoundOwner->HasPhysicsAttacker( 1.0f ) )
			{
				m_Sentences.Speak( "METROPOLICE_DANGER_MANHACK", SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
			}
			return;
		}
	}

	// I hear something dangerous, probably need to take cover.
	// dangerous sound nearby!, call it out
	const char *pSentenceName = "METROPOLICE_DANGER";
	m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_HIGH, SENTENCE_CRITERIA_NORMAL );
}


				
//-----------------------------------------------------------------------------
// Are we currently firing a burst?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsCurrentlyFiringBurst() const
{
	return (m_nBurstMode != BURST_NOT_ACTIVE);
}


//-----------------------------------------------------------------------------
// Is my enemy currently in an airboat?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsEnemyInAnAirboat() const
{
	// Should this be a condition??
	if ( !GetEnemy() || !GetEnemy()->IsPlayer() )
		return false;

	CBaseEntity *pVehicle = static_cast<CBasePlayer*>( GetEnemy() )->GetVehicleEntity(); 
	if ( !pVehicle )
		return false;

	// NOTE: Could just return true if in a vehicle maybe
	return FClassnameIs( pVehicle, "prop_vehicle_airboat" );
}


//-----------------------------------------------------------------------------
// Returns the airboat
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_MetroPolice::GetEnemyAirboat() const
{
	// Should this be a condition??
	if ( !GetEnemy() || !GetEnemy()->IsPlayer() )
		return NULL;

	return static_cast<CBasePlayer*>( GetEnemy() )->GetVehicleEntity(); 
}


//-----------------------------------------------------------------------------
// Which entity are we actually trying to shoot at?
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_MetroPolice::GetShootTarget()
{
	// Should this be a condition??
	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy || !pEnemy->IsPlayer() )
		return pEnemy;

	CBaseEntity *pVehicle = static_cast<CBasePlayer*>( pEnemy )->GetVehicleEntity(); 
	return pVehicle ? pVehicle : pEnemy;
}


//-----------------------------------------------------------------------------
// Set up the shot regulator based on the equipped weapon
//-----------------------------------------------------------------------------

// Ranges across which to tune fire rates
const float MIN_PISTOL_MODIFY_DIST = 15 * 12;
const float MAX_PISTOL_MODIFY_DIST = 150 * 12;

// Range for rest period minimums
const float MIN_MIN_PISTOL_REST_INTERVAL = 0.6;
const float MAX_MIN_PISTOL_REST_INTERVAL = 1.2;

// Range for rest period maximums
const float MIN_MAX_PISTOL_REST_INTERVAL = 1.2;
const float MAX_MAX_PISTOL_REST_INTERVAL = 2.0;

// Range for burst minimums
const int 	MIN_MIN_PISTOL_BURST = 2;
const int 	MAX_MIN_PISTOL_BURST = 4;

// Range for burst maximums
const int 	MIN_MAX_PISTOL_BURST = 5;
const int 	MAX_MAX_PISTOL_BURST = 8;

void CNPC_MetroPolice::OnUpdateShotRegulator( )
{
	BaseClass::OnUpdateShotRegulator();

	// FIXME: This code (except the burst interval) could be used for all weapon types 
	if( Weapon_OwnsThisType( "weapon_pistol" ) )
	{
		if ( m_nBurstMode == BURST_NOT_ACTIVE )
		{
			if ( GetEnemy() )
			{
				float dist = WorldSpaceCenter().DistTo( GetEnemy()->WorldSpaceCenter() );
				
				dist = clamp( dist, MIN_PISTOL_MODIFY_DIST, MAX_PISTOL_MODIFY_DIST );
				
				float factor = (dist - MIN_PISTOL_MODIFY_DIST) / (MAX_PISTOL_MODIFY_DIST - MIN_PISTOL_MODIFY_DIST);
				
				int		nMinBurst			= MIN_MIN_PISTOL_BURST + ( MAX_MIN_PISTOL_BURST - MIN_MIN_PISTOL_BURST ) * (1.0 - factor);
				int		nMaxBurst			= MIN_MAX_PISTOL_BURST + ( MAX_MAX_PISTOL_BURST - MIN_MAX_PISTOL_BURST ) * (1.0 - factor);
				float	flMinRestInterval	= MIN_MIN_PISTOL_REST_INTERVAL + ( MAX_MIN_PISTOL_REST_INTERVAL - MIN_MIN_PISTOL_REST_INTERVAL ) * factor;
				float	flMaxRestInterval	= MIN_MAX_PISTOL_REST_INTERVAL + ( MAX_MAX_PISTOL_REST_INTERVAL - MIN_MAX_PISTOL_REST_INTERVAL ) * factor;
				
				GetShotRegulator()->SetRestInterval( flMinRestInterval, flMaxRestInterval );
				GetShotRegulator()->SetBurstShotCountRange( nMinBurst, nMaxBurst );
			}
			else
			{
				GetShotRegulator()->SetBurstShotCountRange(GetActiveWeapon()->GetMinBurst(), GetActiveWeapon()->GetMaxBurst() );
				GetShotRegulator()->SetRestInterval( 0.6, 1.4 );
			}
		}

		// Add some noise into the pistol
		GetShotRegulator()->SetBurstInterval( 0.2f, 0.5f );
	}
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SetBurstMode( bool bEnable )
{
	int nOldBurstMode = m_nBurstMode;
	m_nBurstSteerMode = BURST_STEER_NONE;
	m_flBurstPredictTime = gpGlobals->curtime - 1.0f;
	if ( GetActiveWeapon() )
	{
		m_nBurstMode = bEnable ? BURST_ACTIVE : BURST_NOT_ACTIVE;
		if ( bEnable )
		{
			m_nBurstHits = 0;
		}
	}
	else
	{
		m_nBurstMode = BURST_NOT_ACTIVE;
	}

	if ( m_nBurstMode != nOldBurstMode )
	{
		OnUpdateShotRegulator();
		if ( m_nBurstMode == BURST_NOT_ACTIVE )
		{
			// Check for inconsistency...
			int nMinBurstCount, nMaxBurstCount;
			GetShotRegulator()->GetBurstShotCountRange( &nMinBurstCount, &nMaxBurstCount );
			if ( GetShotRegulator()->GetBurstShotsRemaining() > nMaxBurstCount )
			{
				GetShotRegulator()->SetBurstShotsRemaining( nMaxBurstCount );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Should we attempt to stitch?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldAttemptToStitch()
{
	if ( IsEnemyInAnAirboat() )
		return true;

	if ( !GetShootTarget() )
		return false;

	if ( HasSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH ) )
	{
		// Don't stitch if the player is at the same level or higher
		if ( GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z > -36 )
			return false;

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// position to shoot at
//-----------------------------------------------------------------------------
Vector CNPC_MetroPolice::StitchAimTarget( const Vector &posSrc, bool bNoisy ) 
{
	// This will make us aim a stitch at the feet of the player so we can see it
	if ( !GetEnemy()->IsPlayer() )
		return GetShootTarget()->BodyTarget( posSrc, bNoisy );

	if ( !IsEnemyInAnAirboat() )
	{
		Vector vecBodyTarget;
		if ( ( GetEnemy()->GetWaterLevel() == 0 ) && ( GetEnemy()->GetFlags() & FL_ONGROUND ) )
		{
			GetEnemy()->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.08f ), &vecBodyTarget );
			return vecBodyTarget;
		}

		// Underwater? Just use the normal thing
		if ( GetEnemy()->GetWaterLevel() == 3 )
			return GetShootTarget()->BodyTarget( posSrc, bNoisy );

		// Trace down...
		trace_t	trace;
		GetEnemy()->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &vecBodyTarget );
		float flHeight = GetEnemy()->WorldAlignSize().z;
		UTIL_TraceLine( vecBodyTarget, vecBodyTarget + Vector( 0, 0, -flHeight -80 ), 
			(MASK_SOLID_BRUSHONLY | MASK_WATER), NULL, COLLISION_GROUP_NONE, &trace );
		return trace.endpos;
	}

	// NOTE: HACK! Ths 0.08 is where the water level happens to be.
	// We probably want to find the exact water level and use that as the z position.
	Vector vecBodyTarget;
	if ( !bNoisy )
	{
		GetShootTarget()->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.08f ), &vecBodyTarget );
	}
	else
	{
		GetShootTarget()->CollisionProp()->RandomPointInBounds( Vector( 0.25f, 0.25f, 0.08f ), Vector( 0.75f, 0.75f, 0.08f ), &vecBodyTarget );
	}

	return vecBodyTarget;
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AimBurstRandomly( int nMinCount, int nMaxCount, float flMinDelay, float flMaxDelay )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	GetShotRegulator()->SetParameters( nMinCount, nMaxCount, flMinDelay, flMaxDelay );
	GetShotRegulator()->Reset( true );

	int nShotCount = GetShotRegulator()->GetBurstShotsRemaining();

	Vector vecDelta = StitchAimTarget( GetAbsOrigin(), true ) - Weapon_ShootPosition();
	VectorNormalize( vecDelta );

	// Choose a random direction vector perpendicular to the delta position
	Vector vecRight, vecUp;
	VectorVectors( vecDelta, vecRight, vecUp );
	float flAngle = random->RandomFloat( 0.0f, 2 * M_PI );
	VectorMultiply( vecRight, cos(flAngle), m_vecBurstDelta );
	VectorMA( m_vecBurstDelta, sin(flAngle), vecUp, m_vecBurstDelta );

	// The size of this determines the cone angle
	m_vecBurstDelta *= 0.4f;

	VectorMA( vecDelta, -0.5f, m_vecBurstDelta, m_vecBurstTargetPos );
	m_vecBurstTargetPos += Weapon_ShootPosition();

	m_vecBurstDelta /= (nShotCount - 1);
}


//-----------------------------------------------------------------------------
// Choose a random vector somewhere between the two specified vectors
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::RandomDirectionBetweenVectors( const Vector &vecStart, const Vector &vecEnd, Vector *pResult )
{
	Assert( fabs( vecStart.Length() - 1.0f ) < 1e-3 );
	Assert( fabs( vecEnd.Length() - 1.0f ) < 1e-3 );

	float flCosAngle = DotProduct( vecStart, vecEnd );
	if ( fabs( flCosAngle - 1.0f ) < 1e-3 )
	{
		*pResult = vecStart;
		return;
	}

	Vector vecNormal;
	CrossProduct( vecStart, vecEnd, vecNormal );
	float flLength = VectorNormalize( vecNormal );
	if ( flLength < 1e-3 )
	{
		// This is wrong for anti-parallel vectors. so what?
		*pResult = vecStart;
		return;
	}

	// Rotate the starting angle the specified amount
	float flAngle = acos(flCosAngle) * random->RandomFloat( 0.0f, 1.0f );
	VMatrix rotationMatrix;
	MatrixBuildRotationAboutAxis( rotationMatrix, vecNormal, flAngle );
	Vector3DMultiply( rotationMatrix, vecStart, *pResult );
}


//-----------------------------------------------------------------------------
// Compute a predicted shoot target position n seconds into the future
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PredictShootTargetPosition( float flDeltaTime, float flMinLeadDist, float flAddVelocity, Vector *pVecTarget, Vector *pVecTargetVelocity )
{
	CBaseEntity *pShootTarget = GetShootTarget();
	*pVecTarget = StitchAimTarget( GetAbsOrigin(), true );

	Vector vecSmoothedVel = pShootTarget->GetSmoothedVelocity();

	// When we're in the air, don't predict vertical motion
	if( (pShootTarget->GetFlags() & FL_ONGROUND) == 0 )
	{
		vecSmoothedVel.z = 0.0f;
	}

	Vector vecVelocity;
	AngularImpulse angImpulse;
	GetShootTarget()->GetVelocity( &vecVelocity, &angImpulse );

	Vector vecLeadVector;
	VMatrix rotationMatrix;
	float flAngVel = VectorNormalize( angImpulse );
	flAngVel -= 30.0f;
	if ( flAngVel > 0.0f )
	{
		MatrixBuildRotationAboutAxis( rotationMatrix, angImpulse, flAngVel * flDeltaTime * 0.333f );
		Vector3DMultiply( rotationMatrix, vecSmoothedVel, vecLeadVector );
	}
	else
	{
		vecLeadVector = vecSmoothedVel;
	}

	if ( flAddVelocity != 0.0f )
	{
		Vector vecForward;
		pShootTarget->GetVectors( &vecForward, NULL, NULL );
		VectorMA( vecLeadVector, flAddVelocity, vecForward,	vecLeadVector );
	}

	*pVecTargetVelocity = vecLeadVector;
	
	if ( (vecLeadVector.LengthSqr() * flDeltaTime * flDeltaTime) < flMinLeadDist * flMinLeadDist )
	{
		VectorNormalize( vecLeadVector );
		vecLeadVector *= flMinLeadDist;
	}
	else
	{
		vecLeadVector *= flDeltaTime;
	}

	*pVecTarget += vecLeadVector;
}


//-----------------------------------------------------------------------------
// Compute a predicted velocity n seconds into the future (given a known acceleration rate)
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PredictShootTargetVelocity( float flDeltaTime, Vector *pVecTargetVel )
{
	*pVecTargetVel = GetShootTarget()->GetSmoothedVelocity();

	// Unless there's a big angular velocity, we can assume he accelerates
	// along the forward direction. Predict acceleration for
	Vector vecForward;
	GetShootTarget()->GetVectors( &vecForward, NULL, NULL );

//	float flBlendFactor = 1.0f;
//	VectorMA( *pVecTargetVel, flBlendFactor * VEHICLE_PREDICT_ACCELERATION, vecForward, *pVecTargetVel );
//	if ( pVecTargetVel->LengthSqr() > (VEHICLE_PREDICT_MAX_SPEED * VEHICLE_PREDICT_MAX_SPEED) )
//	{
//		VectorNormalize( *pVecTargetVel );
//		*pVecTargetVel *= VEHICLE_PREDICT_MAX_SPEED;
//	}
}


//-----------------------------------------------------------------------------
// How many shots will I fire in a particular amount of time?
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::CountShotsInTime( float flDeltaTime ) const
{
	return (int)(flDeltaTime / GetActiveWeapon()->GetFireRate() + 0.5f);
}

float CNPC_MetroPolice::GetTimeForShots( int nShotCount ) const
{
	return nShotCount * GetActiveWeapon()->GetFireRate();
}


//-----------------------------------------------------------------------------
// Visualize stitch
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::VisualizeStitch( const Vector &vecStart, const Vector &vecEnd )
{
	NDebugOverlay::Cross3D( vecStart, -Vector(32,32,32), Vector(32,32,32), 255, 0, 0, false, 5.0f );
	NDebugOverlay::Cross3D( vecEnd, -Vector(32,32,32), Vector(32,32,32), 0, 255, 0, false, 5.0f );
	NDebugOverlay::Line( vecStart, vecEnd, 0, 255, 0, true, 5.0f );
}


//-----------------------------------------------------------------------------
// Visualize line of death
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::VisualizeLineOfDeath( )
{
	Vector vecAcross, vecStart;
	CrossProduct( m_vecBurstLineOfDeathDelta, Vector( 0, 0, 1 ), vecAcross );
	VectorNormalize( vecAcross );
	NDebugOverlay::Line( m_vecBurstLineOfDeathOrigin, m_vecBurstLineOfDeathOrigin + m_vecBurstLineOfDeathDelta, 255, 255, 0, false, 5.0f );
	VectorMA( m_vecBurstLineOfDeathOrigin, m_flBurstSteerDistance, vecAcross, vecStart );
	NDebugOverlay::Line( vecStart, vecStart + m_vecBurstLineOfDeathDelta, 255, 0, 0, false, 5.0f );
	VectorMA( m_vecBurstLineOfDeathOrigin, -m_flBurstSteerDistance, vecAcross, vecStart );
	NDebugOverlay::Line( vecStart, vecStart + m_vecBurstLineOfDeathDelta, 255, 0, 0, false, 5.0f );
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
#define AIM_AT_NEAR_DISTANCE_MIN		400.0f
#define AIM_AT_NEAR_DISTANCE_MAX		1000.0f
#define AIM_AT_NEAR_DISTANCE_DELTA		(AIM_AT_NEAR_DISTANCE_MAX - AIM_AT_NEAR_DISTANCE_MIN)
#define AIM_AT_NEAR_DISTANCE_BONUS  	-200.0f

#define AIM_AT_FAR_DISTANCE_MIN			2000.0f
#define AIM_AT_FAR_DISTANCE_BONUS_DISTANCE	500.0f
#define AIM_AT_FAR_DISTANCE_BONUS  		200.0f	// Add this much bonus after each BONUS_DISTANCE


//-----------------------------------------------------------------------------
// Modify the stitch length
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::ComputeDistanceStitchModifier( float flDistanceToTarget ) const
{
	if ( flDistanceToTarget < AIM_AT_NEAR_DISTANCE_MIN )
	{
		return AIM_AT_NEAR_DISTANCE_BONUS;
	}

	if ( flDistanceToTarget < AIM_AT_NEAR_DISTANCE_MAX )
	{
		float flFraction = 1.0f - ((flDistanceToTarget - AIM_AT_NEAR_DISTANCE_MIN) / AIM_AT_NEAR_DISTANCE_DELTA);
		return flFraction * AIM_AT_NEAR_DISTANCE_BONUS;
	}

	if ( flDistanceToTarget > AIM_AT_FAR_DISTANCE_MIN )
	{
		float flFactor = (flDistanceToTarget - AIM_AT_FAR_DISTANCE_MIN) / AIM_AT_FAR_DISTANCE_BONUS_DISTANCE;
		return flFactor * AIM_AT_FAR_DISTANCE_BONUS;
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------
// Set up the shot regulator
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SetupBurstShotRegulator( float flReactionTime )
{
	// We want a certain amount of reaction time before the shots hit the boat
	int nDesiredShotCount = CountShotsInTime( flReactionTime );
	GetShotRegulator()->SetBurstShotCountRange( nDesiredShotCount, nDesiredShotCount );
	GetShotRegulator()->SetRestInterval( 0.7f, 0.9f );
	GetShotRegulator()->Reset( true );
	int nShots = GetShotRegulator()->GetBurstShotsRemaining();
	OnRangeAttack1();
	return nShots;
}


//-----------------------------------------------------------------------------
// Shoots a burst right at the player
//-----------------------------------------------------------------------------
#define TIGHT_GROUP_MIN_DIST 750.0f
#define TIGHT_GROUP_MIN_SPEED 400.0f

void CNPC_MetroPolice::AimBurstTightGrouping( float flShotTime )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	// We want a certain amount of reaction time before the shots hit the boat
	SetupBurstShotRegulator( flShotTime );

	// Max number of times we can hit the enemy.
	// Can hit more if we're slow + close
	float flDistToTargetSqr = GetShootTarget()->WorldSpaceCenter().DistToSqr( Weapon_ShootPosition() );

	int nHitCount = sk_metropolice_stitch_tight_hitcount.GetInt();

	Vector vecTargetVel;
	GetShootTarget()->GetVelocity( &vecTargetVel, NULL );
	if (( flDistToTargetSqr > TIGHT_GROUP_MIN_DIST*TIGHT_GROUP_MIN_DIST ) || 
		( vecTargetVel.LengthSqr() > TIGHT_GROUP_MIN_SPEED * TIGHT_GROUP_MIN_SPEED ))
	{
		m_nMaxBurstHits = random->RandomInt( nHitCount, nHitCount + 1 );
	}
	else
	{
		m_nMaxBurstHits = random->RandomInt( 2 * nHitCount - 1, 2 * nHitCount + 1 );
	}

	m_nBurstMode = BURST_TIGHT_GROUPING;

	// This helps the NPC model aim at the correct point
	m_nBurstSteerMode = BURST_STEER_EXACTLY_TOWARD_TARGET;
	m_vecBurstTargetPos = GetEnemy()->WorldSpaceCenter();
}


//-----------------------------------------------------------------------------
// Reaction time for stitch
//-----------------------------------------------------------------------------
#define AIM_AT_TIME_DELTA_SPEED 100.0f
#define AIM_AT_TIME_DELTA_DIST 500.0f
#define AIM_AT_TIME_SPEED_COUNT 6
#define AIM_AT_TIME_DIST_COUNT 7

static float s_pReactionFraction[AIM_AT_TIME_DIST_COUNT][AIM_AT_TIME_SPEED_COUNT] =
{
	{  0.5f, 0.5f,  0.5f,  0.5f,  0.5f, 1.0f },
	{  0.5f, 0.5f,  0.5f,  0.5f, 0.75f, 1.0f },
	{  0.5f, 0.5f,  0.5f, 0.65f,  0.8f, 1.0f },
	{  0.5f, 0.5f,  0.5f, 0.75f,  1.0f, 1.0f },
	{  0.5f, 0.5f, 0.75f,  1.0f,  1.0f, 1.0f },
	{ 0.75f, 1.0f,  1.0f,  1.0f,  1.0f, 1.0f },
	{  1.0f, 1.0f,  1.0f,  1.0f,  1.0f, 1.0f },
};

float CNPC_MetroPolice::AimBurstAtReactionTime( float flReactionTime, float flDistToTarget, float flCurrentSpeed )
{
	flReactionTime *= sk_metropolice_stitch_reaction.GetFloat();

	if ( IsEnemyInAnAirboat() )
	{
		float u = flCurrentSpeed / AIM_AT_TIME_DELTA_SPEED;
		float v = flDistToTarget / AIM_AT_TIME_DELTA_DIST;
		int nu = (int)u;
		int nv = (int)v;
		if (( nu < AIM_AT_TIME_SPEED_COUNT - 1 ) && ( nv < AIM_AT_TIME_DIST_COUNT - 1 ))
		{
			float fu = u - nu;
			float fv = v - nv;
			float flReactionFactor = s_pReactionFraction[nv][nu] * (1.0f - fu) * (1.0f - fv);
			flReactionFactor += s_pReactionFraction[nv+1][nu] * (1.0f - fu) * fv;
			flReactionFactor += s_pReactionFraction[nv][nu+1] * fu * (1.0f - fv);
			flReactionFactor += s_pReactionFraction[nv+1][nu+1] * fu * fv;

			flReactionTime *= flReactionFactor;
		}
	}
	
	return flReactionTime;
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
#define AIM_AT_SHOT_DELTA_SPEED 100.0f
#define AIM_AT_SHOT_DELTA_DIST 500.0f
#define AIM_AT_SHOT_SPEED_COUNT 6
#define AIM_AT_SHOT_DIST_COUNT 6

static float s_pShotCountFraction[AIM_AT_TIME_DIST_COUNT][AIM_AT_TIME_SPEED_COUNT] =
{
	{  3.0f, 3.0f,  2.5f,  1.5f,  1.0f, 0.0f },
	{  3.0f, 3.0f,  2.5f,  1.25f, 0.5f, 0.0f },
	{  2.5f, 2.5f,  2.0f,  1.0f,  0.0f, 0.0f },
	{  2.0f, 2.0f,  1.5f,  0.5f,  0.0f, 0.0f },
	{  1.0f, 1.0f,  1.0f,  0.5f,  0.0f, 0.0f },
	{  0.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f },
};

int CNPC_MetroPolice::AimBurstAtSetupHitCount( float flDistToTarget, float flCurrentSpeed )
{
	// Max number of times we can hit the enemy
	int nHitCount = sk_metropolice_stitch_at_hitcount.GetInt();
	m_nMaxBurstHits = random->RandomInt( nHitCount, nHitCount + 1 );

	if ( IsEnemyInAnAirboat() )
	{
		float u = flCurrentSpeed / AIM_AT_SHOT_DELTA_SPEED;
		float v = flDistToTarget / AIM_AT_SHOT_DELTA_DIST;
		int nu = (int)u;
		int nv = (int)v;
		if (( nu < AIM_AT_SHOT_SPEED_COUNT - 1 ) && ( nv < AIM_AT_SHOT_DIST_COUNT - 1 ))
		{
			float fu = u - nu;
			float fv = v - nv;
			float flShotFactor = s_pShotCountFraction[nv][nu] * (1.0f - fu) * (1.0f - fv);
			flShotFactor += s_pShotCountFraction[nv+1][nu] * (1.0f - fu) * fv;
			flShotFactor += s_pShotCountFraction[nv][nu+1] * fu * (1.0f - fv);
			flShotFactor += s_pShotCountFraction[nv+1][nu+1] * fu * fv;

			int nExtraShots = nHitCount * flShotFactor;
			m_nMaxBurstHits += random->RandomInt( nExtraShots, nExtraShots + 1 );
			return nExtraShots;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
#define AIM_AT_DEFAULT_STITCH_SHOT_DIST	40.0f
#define AIM_AT_SPEED_BONUS				200.0f
#define AIM_AT_REACTION_TIME_FRACTION	0.8f
#define AIM_AT_NEAR_REACTION_TIME_FRACTION	0.3f
#define AIM_AT_STEER_DISTANCE			125.0f

void CNPC_MetroPolice::AimBurstAtEnemy( float flReactionTime )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	Vector vecVelocity;
	GetShootTarget()->GetVelocity( &vecVelocity, NULL );
	float flCurrentSpeed = vecVelocity.Length();
	float flDistToTargetSqr = GetShootTarget()->WorldSpaceCenter().AsVector2D().DistToSqr( Weapon_ShootPosition().AsVector2D() );
	float flDistToTarget = sqrt(flDistToTargetSqr);

	flReactionTime = AimBurstAtReactionTime( flReactionTime, flDistToTarget, flCurrentSpeed );

	// We want a certain amount of reaction time before the shots hit the boat
	int nShotCount = SetupBurstShotRegulator( flReactionTime );

	bool bIsInVehicle = IsEnemyInAnAirboat();
	if ( bIsInVehicle )
	{
		m_nBurstMode = BURST_LOCK_ON_AFTER_HIT;
		m_flBurstSteerDistance = AIM_AT_STEER_DISTANCE;
	}
	else
	{
		m_nBurstMode = BURST_ACTIVE;
		m_flBurstSteerDistance = 0;
	}
	m_nBurstSteerMode = BURST_STEER_WITHIN_LINE_OF_DEATH;

	// Max number of times we can hit the enemy
	int nExtraShots = AimBurstAtSetupHitCount( flDistToTarget, flCurrentSpeed );
	float flExtraTime = GetTimeForShots( nExtraShots ) + (1.0f - AIM_AT_REACTION_TIME_FRACTION) * flReactionTime;
	float flReactionFraction = 1.0f - flExtraTime / flReactionTime;
	if ( flReactionFraction < 0.5f )
	{
		flReactionFraction = 0.5f;
	}

	float flFirstHitTime = flReactionTime * flReactionFraction;
	Vector vecShootAt, vecShootAtVel;
	PredictShootTargetPosition( flFirstHitTime, 0.0f, 0.0f, &vecShootAt, &vecShootAtVel );

	Vector vecDelta;
	VectorSubtract( vecShootAt, Weapon_ShootPosition(), vecDelta );
	float flDistanceToTarget = vecDelta.Length();

	// Always stitch horizontally...
	vecDelta.z = 0.0f;

	// The max stitch distance here is used to guarantee the cop doesn't try to lead
	// the airboat so much that he ends up shooting behind himself
	float flMaxStitchDistance = VectorNormalize( vecDelta );
	flMaxStitchDistance -= 50.0f;
	if ( flMaxStitchDistance < 0 )
	{
		flMaxStitchDistance = 0.0f;
	}

	float flStitchLength = nShotCount * AIM_AT_DEFAULT_STITCH_SHOT_DIST;

	// Modify the stitch length based on distance from the shooter
	flStitchLength += ComputeDistanceStitchModifier( flDistanceToTarget );

	if ( bIsInVehicle )
	{
		// Make longer stitches if the enemy is going faster
		Vector vecEnemyVelocity = GetShootTarget()->GetSmoothedVelocity();
		if( (GetShootTarget()->GetFlags() & FL_ONGROUND) == 0 )
		{
			vecEnemyVelocity.z = 0.0f;
		}

		float flEnemySpeed = VectorNormalize( vecEnemyVelocity );
		flStitchLength += AIM_AT_SPEED_BONUS * ( flEnemySpeed / 100.0f );

		// Add in a little randomness across the direction of motion...
		// Always put it on the side we're currently looking at
		Vector vecAcross;
		CrossProduct( vecEnemyVelocity, Vector( 0, 0, 1 ), vecAcross );
		VectorNormalize( vecAcross );
		
		Vector eyeForward;
		AngleVectors( GetEnemy()->EyeAngles(), &eyeForward );
		if ( DotProduct( vecAcross, eyeForward ) < 0.0f )
		{
			vecAcross *= -1.0f;
		}

		float flMinAdd = RemapVal( flEnemySpeed, 0.0f, 200.0f, 70.0f, 30.0f );
		VectorMA( vecShootAt, random->RandomFloat( flMinAdd, 100.0f ), vecAcross, vecShootAt );
	}

	// Compute the distance along the stitch direction to the cop. we don't want to cross that line
	Vector vecStitchStart, vecStitchEnd;
	VectorMA( vecShootAt, -MIN( flStitchLength * flReactionFraction, flMaxStitchDistance ), vecDelta, vecStitchStart );
	VectorMA( vecShootAt, flStitchLength * (1.0f - flReactionFraction), vecDelta, vecStitchEnd );
	
	// Trace down a bit to hit the ground if we're above the ground...
	trace_t	trace;
	UTIL_TraceLine( vecStitchStart, vecStitchStart + Vector( 0, 0, -512 ), (MASK_SOLID_BRUSHONLY | MASK_WATER), NULL, COLLISION_GROUP_NONE, &trace );
	m_vecBurstTargetPos = trace.endpos;
	VectorSubtract( vecStitchEnd, m_vecBurstTargetPos, m_vecBurstDelta );

	m_vecBurstLineOfDeathOrigin = m_vecBurstTargetPos;
	m_vecBurstLineOfDeathDelta = m_vecBurstDelta;

	m_vecBurstDelta /= (nShotCount - 1);

//	VisualizeStitch( m_vecBurstTargetPos, vecStitchEnd );
//	VisualizeLineOfDeath();
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
#define AIM_IN_FRONT_OF_DEFAULT_STITCH_LENGTH		1000.0f
#define AIM_IN_FRONT_OF_MINIMUM_DISTANCE			500.0f
#define AIM_IN_FRONT_DRAW_LINE_OF_DEATH_FRACTION	0.5f
#define AIM_IN_FRONT_STEER_DISTANCE					150.0f
#define AIM_IN_FRONT_REACTION_FRACTION				0.8f
#define AIM_IN_FRONT_EXTRA_VEL						200.0f

void CNPC_MetroPolice::AimBurstInFrontOfEnemy( float flReactionTime )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	flReactionTime *= sk_metropolice_stitch_reaction.GetFloat();

	// We want a certain amount of reaction time before the shots hit the boat
	int nShotCount = SetupBurstShotRegulator( flReactionTime );

	// Max number of times we can hit the player in the airboat
	m_nMaxBurstHits = random->RandomInt( 3, 4 );
	m_nBurstMode = BURST_LOCK_ON_AFTER_HIT;
	m_nBurstSteerMode = BURST_STEER_WITHIN_LINE_OF_DEATH;

	// The goal here is to slow him down. Choose a target position such that we predict
	// where he'd be in he accelerated by N over the reaction time. Prevent him from getting there.
	Vector vecShootAt, vecShootAtVel, vecAcross;
	PredictShootTargetPosition( flReactionTime * AIM_IN_FRONT_REACTION_FRACTION, 
		AIM_IN_FRONT_OF_MINIMUM_DISTANCE, 0.0f, &vecShootAt, &vecShootAtVel );

	// Now add in some extra vel in a random direction + try to prevent that....
	Vector vecTargetToGun, vecExtraDistance;
	VectorSubtract( Weapon_ShootPosition(), vecShootAt, vecTargetToGun );
	VectorNormalize( vecTargetToGun );
	VectorNormalize( vecShootAtVel );
	RandomDirectionBetweenVectors( vecShootAtVel, vecTargetToGun, &vecExtraDistance );
	vecExtraDistance *= AIM_IN_FRONT_EXTRA_VEL;
	vecShootAt += vecExtraDistance;

	CrossProduct( vecExtraDistance, Vector( 0, 0, 1 ), vecAcross );
	VectorNormalize( vecAcross );

	float flStitchLength = AIM_IN_FRONT_OF_DEFAULT_STITCH_LENGTH;

	Vector vecEndPoint1, vecEndPoint2;
	VectorSubtract( Weapon_ShootPosition(), StitchAimTarget( GetAbsOrigin(), false ), vecTargetToGun );
	float flSign = ( DotProduct( vecAcross, vecTargetToGun ) >= 0.0f ) ? 1.0f : -1.0f;
	VectorMA( vecShootAt, flSign * flStitchLength * AIM_IN_FRONT_REACTION_FRACTION, vecAcross, vecEndPoint1 );
	VectorMA( vecShootAt, -flSign * flStitchLength * (1.0f - AIM_IN_FRONT_REACTION_FRACTION), vecAcross, vecEndPoint2 );

	m_vecBurstTargetPos = vecEndPoint1;
	VectorSubtract( vecEndPoint2, vecEndPoint1, m_vecBurstDelta );

	// This defines the line of death, which, when crossed, results in damage
	m_vecBurstLineOfDeathOrigin = m_vecBurstTargetPos;
	m_vecBurstLineOfDeathDelta = m_vecBurstDelta;
	m_flBurstSteerDistance = AIM_IN_FRONT_STEER_DISTANCE;

	// Make the visual representation of the line of death lie closest to the boat.
	VectorMA( m_vecBurstTargetPos, -AIM_IN_FRONT_STEER_DISTANCE, vecShootAtVel, m_vecBurstTargetPos );
	m_vecBurstDelta /= (nShotCount - 1);

//	VisualizeStitch( m_vecBurstTargetPos, m_vecBurstTargetPos + m_vecBurstDelta * (nShotCount - 1) );
//	VisualizeLineOfDeath();
}


//-----------------------------------------------------------------------------
// Aim burst behind enemy
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AimBurstBehindEnemy( float flShotTime )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	flShotTime *= sk_metropolice_stitch_reaction.GetFloat();

	// We want a certain amount of reaction time before the shots hit the boat
	int nShotCount = SetupBurstShotRegulator( flShotTime );

	// Max number of times we can hit the player in the airboat
	int nHitCount = sk_metropolice_stitch_behind_hitcount.GetInt();
	m_nMaxBurstHits = random->RandomInt( nHitCount, nHitCount + 1 );
	m_nBurstMode = BURST_LOCK_ON_AFTER_HIT;
	m_nBurstSteerMode = BURST_STEER_WITHIN_LINE_OF_DEATH;

	// Shoot across the enemy in between the enemy and me
	Vector vecShootAt, vecShootAtVel, vecAcross;
	PredictShootTargetPosition( 0.0f, 0.0f, 0.0f, &vecShootAt, &vecShootAtVel );

	// Choose a point in between the shooter + the target
	Vector vecDelta;
	VectorSubtract( Weapon_ShootPosition(), vecShootAt, vecDelta );
	vecDelta.z = 0.0f;
	float flDistTo = VectorNormalize( vecDelta );
	if ( flDistTo > AIM_BEHIND_MINIMUM_DISTANCE )
	{
		flDistTo = AIM_BEHIND_MINIMUM_DISTANCE;
	}
	VectorMA( vecShootAt, flDistTo, vecDelta, vecShootAt );
	CrossProduct( vecDelta, Vector( 0, 0, 1 ), vecAcross );

	float flStitchLength = AIM_BEHIND_DEFAULT_STITCH_LENGTH;

	Vector vecEndPoint1, vecEndPoint2;
	VectorMA( vecShootAt, -flStitchLength * 0.5f, vecAcross, vecEndPoint1 );
	VectorMA( vecShootAt, flStitchLength * 0.5f, vecAcross, vecEndPoint2 );
	
	m_vecBurstTargetPos = vecEndPoint1;
	VectorSubtract( vecEndPoint2, vecEndPoint1, m_vecBurstDelta );

	// This defines the line of death, which, when crossed, results in damage
	m_vecBurstLineOfDeathOrigin = m_vecBurstTargetPos;
	m_vecBurstLineOfDeathDelta = m_vecBurstDelta;
	m_flBurstSteerDistance = AIM_BEHIND_STEER_DISTANCE;

	// Make the visual representation of the line of death lie closest to the boat.
	VectorMA( m_vecBurstTargetPos, -AIM_BEHIND_STEER_DISTANCE, vecDelta, m_vecBurstTargetPos );
	m_vecBurstDelta /= (nShotCount - 1);

//	VisualizeStitch( m_vecBurstTargetPos, m_vecBurstTargetPos + m_vecBurstDelta * (nShotCount - 1) );
//	VisualizeLineOfDeath();
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AimBurstAlongSideOfEnemy( float flFollowTime )
{
	if ( !IsCurrentlyFiringBurst() )
		return;

	flFollowTime *= sk_metropolice_stitch_reaction.GetFloat();

	// We want a certain amount of reaction time before the shots hit the boat
	int nShotCount = SetupBurstShotRegulator( flFollowTime );

	// Max number of times we can hit the player in the airboat
	int nHitCount = sk_metropolice_stitch_along_hitcount.GetInt();
	m_nMaxBurstHits = random->RandomInt( nHitCount, nHitCount + 1 );
	m_nBurstMode = BURST_LOCK_ON_AFTER_HIT;
	m_nBurstSteerMode = BURST_STEER_WITHIN_LINE_OF_DEATH;

	Vector vecShootAt, vecShootAtVel, vecAcross;
	PredictShootTargetPosition( AIM_ALONG_SIDE_LINE_OF_DEATH_LEAD_TIME, 225.0f, 0.0f, &vecShootAt, &vecShootAtVel );
	CrossProduct( vecShootAtVel, Vector( 0, 0, 1 ), vecAcross );
	VectorNormalize( vecAcross );

	// Choose the side of the vehicle which is closer to the shooter
	Vector vecSidePoint;		
	Vector vecTargetToGun;
	VectorSubtract( Weapon_ShootPosition(), vecShootAt, vecTargetToGun );
	float flSign = ( DotProduct( vecTargetToGun, vecAcross ) > 0.0f ) ? 1.0f : -1.0f;
	float flDist = AIM_ALONG_SIDE_LINE_OF_DEATH_DISTANCE + random->RandomFloat( 0.0f, 50.0f );
	VectorMA( vecShootAt, flSign * flDist, vecAcross, vecSidePoint );

	vecShootAtVel.z = 0.0f;
	float flTargetSpeed = VectorNormalize( vecShootAtVel );
	float flStitchLength = MAX( AIM_IN_FRONT_OF_DEFAULT_STITCH_LENGTH, flTargetSpeed * flFollowTime * 0.9 );

	// This defines the line of death, which, when crossed, results in damage
	m_vecBurstLineOfDeathOrigin = vecSidePoint;
	VectorMultiply( vecShootAtVel, flStitchLength, m_vecBurstLineOfDeathDelta );

	// Pull the endpoint a little toward the NPC firing it...
	float flExtraDist = random->RandomFloat( 25.0f, 50.0f );
	VectorNormalize( vecTargetToGun );
	if ( flSign * DotProduct( vecTargetToGun, vecShootAtVel ) < 0.1f )
	{
		flExtraDist += 100.0f;
	}
	VectorMA( m_vecBurstLineOfDeathDelta, flSign * flExtraDist, vecAcross, m_vecBurstLineOfDeathDelta );

	m_flBurstSteerDistance = AIM_ALONG_SIDE_STEER_DISTANCE;
	m_vecBurstDelta = m_vecBurstLineOfDeathDelta;
	m_vecBurstTargetPos = m_vecBurstLineOfDeathOrigin;

	// Make the visual representation of the line of death lie closest to the boat.
	VectorMA( m_vecBurstTargetPos, -flSign * AIM_ALONG_SIDE_STEER_DISTANCE, vecAcross, m_vecBurstTargetPos );
	m_vecBurstDelta /= (nShotCount - 1);

//	VisualizeStitch( m_vecBurstTargetPos, m_vecBurstTargetPos + m_vecBurstDelta * (nShotCount - 1) );
//	VisualizeLineOfDeath();
}


//-----------------------------------------------------------------------------
// Different burst steering modes
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SteerBurstTowardTargetUseSpeedOnly( const Vector &vecShootAt, 
	const Vector &vecShootAtVelocity, float flPredictTime, int nShotsTillPredict )
{
	// Only account for changes in *speed*; ignore all changes in velocity direction, etc.
	// This one only hits the player if there is *no* steering, just acceleration or decceleration
	Vector vecBurstDir = m_vecBurstPredictedVelocityDir;
	float flActualSpeed = DotProduct( vecShootAtVelocity, vecBurstDir );

	vecBurstDir *= (flActualSpeed - m_vecBurstPredictedSpeed) * flPredictTime;
	vecBurstDir /= (nShotsTillPredict - 1);

	m_vecBurstPredictedSpeed = flActualSpeed; 
	m_vecBurstDelta += vecBurstDir;
}

void CNPC_MetroPolice::SteerBurstTowardTargetUseVelocity( const Vector &vecShootAt, const Vector &vecShootAtVelocity, int nShotsTillPredict )
{
	// Only account for all velocity changes
	// This one looks scary in that it always gets near to the player,
	// but it never usually hits actually.
	Vector vecBurstDir = m_vecBurstLineOfDeathDelta;
	m_vecBurstLineOfDeathDelta = vecShootAtVelocity;
	vecBurstDir = vecShootAtVelocity - vecBurstDir;
	vecBurstDir /= (nShotsTillPredict - 1);

	m_vecBurstDelta += vecBurstDir;
}

void CNPC_MetroPolice::SteerBurstTowardTargetUsePosition( const Vector &vecShootAt, const Vector &vecShootAtVelocity, int nShotsTillPredict )
{
	// Account for velocity + position changes
	// This method *always* hits
	VectorSubtract( vecShootAt, m_vecBurstTargetPos, m_vecBurstDelta );
	m_vecBurstDelta /= (nShotsTillPredict - 1);
}

void CNPC_MetroPolice::SteerBurstTowardPredictedPoint( const Vector &vecShootAt, const Vector &vecShootAtVelocity, int nShotsTillPredict )
{
	// Account for velocity + position changes, but only within a constrained cylinder
	Vector vecConstrainedShootPosition;
	CalcClosestPointOnLine( vecShootAt, m_vecBurstLineOfDeathOrigin, m_vecBurstLineOfDeathOrigin + m_vecBurstLineOfDeathDelta, vecConstrainedShootPosition );

	Vector vecDelta;
	VectorSubtract( vecShootAt, vecConstrainedShootPosition, vecDelta );
	if ( vecDelta.LengthSqr( ) <= m_flBurstSteerDistance * m_flBurstSteerDistance )
	{
		vecConstrainedShootPosition = vecShootAt;
	}
	else
	{
		VectorNormalize( vecDelta );
		VectorMA( vecConstrainedShootPosition, m_flBurstSteerDistance, vecDelta, vecConstrainedShootPosition );
	}

	// This method *always* hits if the entity is within the cylinder
	VectorSubtract( vecConstrainedShootPosition, m_vecBurstTargetPos, m_vecBurstDelta );
	if ( nShotsTillPredict >= 2 )
	{
		m_vecBurstDelta /= (nShotsTillPredict - 1);
	}
}

#define STEER_LINE_OF_DEATH_MAX_DISTANCE	250.0f

void CNPC_MetroPolice::SteerBurstWithinLineOfDeath( )
{
	// Account for velocity + position changes, but only within a constrained cylinder
	Vector vecShootAt;
	vecShootAt = StitchAimTarget( GetAbsOrigin(), false );

	// If the target close to the current point the shot is on,
	// move the shot toward the point
	Vector vecPointOnLineOfDeath;
	CalcClosestPointOnLine( m_vecBurstTargetPos, m_vecBurstLineOfDeathOrigin, m_vecBurstLineOfDeathOrigin + m_vecBurstLineOfDeathDelta, vecPointOnLineOfDeath );

	Vector vecDelta;
	VectorSubtract( vecShootAt, vecPointOnLineOfDeath, vecDelta );
	if ( vecDelta.LengthSqr( ) <= m_flBurstSteerDistance * m_flBurstSteerDistance )
	{
		VectorSubtract( vecShootAt, m_vecBurstTargetPos, m_vecBurstDelta );
		if ( m_vecBurstDelta.LengthSqr() > (STEER_LINE_OF_DEATH_MAX_DISTANCE * STEER_LINE_OF_DEATH_MAX_DISTANCE) )
		{
			VectorNormalize( m_vecBurstDelta );
			m_vecBurstDelta *= STEER_LINE_OF_DEATH_MAX_DISTANCE;
		}
	}
	else
	{
		// Just make the burst go back and forth alont the line of death...
		Vector vecNext = m_vecBurstTargetPos + m_vecBurstDelta;

		float t;
		CalcClosestPointOnLine( vecNext, m_vecBurstLineOfDeathOrigin, m_vecBurstLineOfDeathOrigin + m_vecBurstLineOfDeathDelta, vecPointOnLineOfDeath, &t );
		if (( t < -0.1f ) || ( t > 1.1f ))
		{
			m_vecBurstDelta *= -1.0f;

			// This is necessary to make it not look like a machine is firing the gun
			Vector vecBurstDir = m_vecBurstDelta;
			float flLength = VectorNormalize( vecBurstDir );
			vecBurstDir *= random->RandomFloat( -flLength * 0.5f, flLength * 0.5f );

			m_vecBurstTargetPos += vecBurstDir;
		}
	}
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SteerBurstTowardTarget( )
{
	switch ( m_nBurstSteerMode )
	{
	case BURST_STEER_NONE:
		return;

	case BURST_STEER_EXACTLY_TOWARD_TARGET:
		// Necessary to get the cop looking at the target
		m_vecBurstTargetPos = GetEnemy()->WorldSpaceCenter();
		return;
 
	case BURST_STEER_ADJUST_FOR_SPEED_CHANGES:
		{
			// Predict the airboat position at the point where we were expecting to hit them
			if ( m_flBurstPredictTime <= gpGlobals->curtime )
				return;

			float flPredictTime = m_flBurstPredictTime - gpGlobals->curtime;
			int nShotsTillPredict = CountShotsInTime( flPredictTime );
			if ( nShotsTillPredict <= 1 )
				return;

			Vector vecShootAt, vecShootAtVelocity;
			PredictShootTargetPosition( flPredictTime, 0.0f, 0.0f, &vecShootAt, &vecShootAtVelocity );
			SteerBurstTowardTargetUseSpeedOnly( vecShootAt, vecShootAtVelocity, flPredictTime, nShotsTillPredict );
		}
		break;

	case BURST_STEER_TOWARD_PREDICTED_POINT:
		// Don't course-correct until the predicted time
		if ( m_flBurstPredictTime >= gpGlobals->curtime )
			return;

		// fall through!

	case BURST_STEER_WITHIN_LINE_OF_DEATH:
		break;
	}

	SteerBurstWithinLineOfDeath( );
}


//-----------------------------------------------------------------------------
// Various burst trajectory methods
//-----------------------------------------------------------------------------
Vector CNPC_MetroPolice::ComputeBurstLockOnTrajectory( const Vector &shootOrigin )
{
	Vector vecTrajectory;
	VectorSubtract( GetEnemy()->WorldSpaceCenter(), shootOrigin, vecTrajectory );
	VectorNormalize( vecTrajectory );
	return vecTrajectory;
}

Vector CNPC_MetroPolice::ComputeBurstDeliberatelyMissTrajectory( const Vector &shootOrigin )
{
	m_vecBurstTargetPos.z += 8.0f;

	Vector vecTrajectory;
	VectorSubtract( m_vecBurstTargetPos, shootOrigin, vecTrajectory );
	VectorNormalize( vecTrajectory );
	return vecTrajectory;
}

Vector CNPC_MetroPolice::ComputeBurstTrajectory( const Vector &shootOrigin )
{
	// Perform the stitch
	Vector vecPos = m_vecBurstTargetPos;

	// For players, don't let them jump over the burst.
	CBaseEntity *pEnemy = GetEnemy();
	bool bIsPlayerOnFoot = pEnemy && pEnemy->IsPlayer() && !IsEnemyInAnAirboat();
	if ( bIsPlayerOnFoot )
	{
		Vector vecNormalizedPt;
		pEnemy->CollisionProp()->WorldToNormalizedSpace( vecPos, &vecNormalizedPt );
		if ( (vecNormalizedPt.x >= -0.1f) && (vecNormalizedPt.x <= 1.1f) &&
			(vecNormalizedPt.y >= -0.1f) && (vecNormalizedPt.y <= 1.1f) &&
			(vecNormalizedPt.z >= -0.7f) && (vecNormalizedPt.z < 1.1f) )
		{
 			vecPos.z = pEnemy->WorldSpaceCenter().z;
		}
	}

	vecPos -= shootOrigin;

	// Add a little noise. Even though it's non-physical, it looks better
	// to have the same amount of noise regardless of distance from the shooter
	// Always make the noise perpendicular to the burst direction
	float flNoise = bIsPlayerOnFoot ? 16.0f : 32.0f;
	Vector vecNoise;
	CrossProduct( m_vecBurstDelta, Vector( 0, 0, 1 ), vecNoise );
	VectorNormalize( vecNoise );
	vecNoise *= random->RandomFloat( -flNoise, flNoise );
	vecPos += vecNoise;

	VectorNormalize( vecPos );

	// X360BUG: Was causing compiler crash in release, still?
//	if ( IsPC() )
	{
		// Allow for steering towards the target.
		SteerBurstTowardTarget();
	}
	
	// Update the burst target position
	m_vecBurstTargetPos += m_vecBurstDelta;
	
//	NDebugOverlay::Cross3D( m_vecBurstTargetPos, -Vector(32,32,32), Vector(32,32,32), 255, 0, 255, false, 1.0f );

	return vecPos;
}


//-----------------------------------------------------------------------------
// Deliberately aims as close as possible w/o hitting
//-----------------------------------------------------------------------------
Vector CNPC_MetroPolice::AimCloseToTargetButMiss( CBaseEntity *pTarget, const Vector &shootOrigin )
{
	Vector vecNormalizedSpace;
	pTarget->CollisionProp()->WorldToNormalizedSpace( shootOrigin, &vecNormalizedSpace );
	vecNormalizedSpace -= Vector( 0.5f, 0.5f, 0.5f );
	float flDist = VectorNormalize( vecNormalizedSpace );
	float flMinRadius = flDist * sqrt(3.0) / sqrt( flDist * flDist - 3 );

	// Choose random points in a plane perpendicular to the shoot origin.
	Vector vecRandomDir;
	vecRandomDir.Random( -1.0f, 1.0f );
	VectorMA( vecRandomDir, -DotProduct( vecNormalizedSpace, vecRandomDir ), vecNormalizedSpace, vecRandomDir );
	VectorNormalize( vecRandomDir );
	vecRandomDir *= flMinRadius;

	vecRandomDir *= 0.5f;
	vecRandomDir += Vector( 0.5f, 0.5f, 0.5f );

	Vector vecBodyTarget;
	pTarget->CollisionProp()->NormalizedToWorldSpace( vecRandomDir, &vecBodyTarget );
	vecBodyTarget -= shootOrigin;
	return vecBodyTarget;
}


//-----------------------------------------------------------------------------
// A burst that goes right at the enemy
//-----------------------------------------------------------------------------
#define MIN_TIGHT_BURST_DIST 1000.0f
#define MAX_TIGHT_BURST_DIST 2000.0f

Vector CNPC_MetroPolice::ComputeTightBurstTrajectory( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		return BaseClass::GetActualShootTrajectory( shootOrigin );
	}

	// Aim around the player...
	if ( m_nBurstHits >= m_nMaxBurstHits )
	{
		return AimCloseToTargetButMiss( pEnemy, shootOrigin );
	}

	float flDist = shootOrigin.DistTo( pEnemy->WorldSpaceCenter() );
	float flMin = -0.2f;
	float flMax = 1.2f;
	if ( flDist > MIN_TIGHT_BURST_DIST )
	{
		flDist = clamp( flDist, MIN_TIGHT_BURST_DIST, MAX_TIGHT_BURST_DIST );
		flMin = SimpleSplineRemapVal( flDist, MIN_TIGHT_BURST_DIST, MAX_TIGHT_BURST_DIST, -0.2f, -0.7f );
		flMax = SimpleSplineRemapVal( flDist, MIN_TIGHT_BURST_DIST, MAX_TIGHT_BURST_DIST, 1.2f, 1.7f );
	}

	// Aim randomly at the player. Since body target uses the vehicle body target,
	// we instead are going to not use it
	Vector vecBodyTarget;
	pEnemy->CollisionProp()->RandomPointInBounds( Vector( flMin, flMin, flMin ), Vector( flMax, flMax, flMax * 0.75f ), &vecBodyTarget );
	vecBodyTarget -= shootOrigin;
	return vecBodyTarget;
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
Vector CNPC_MetroPolice::GetActualShootTrajectory( const Vector &shootOrigin )
{
	switch ( m_nBurstMode )
	{
	case BURST_NOT_ACTIVE:
		return BaseClass::GetActualShootTrajectory( shootOrigin );

	case BURST_LOCKED_ON:
		if ( m_nBurstHits < m_nMaxBurstHits )
		{
			return ComputeBurstLockOnTrajectory( shootOrigin );
		}

		// Start shooting over the head of the enemy
		GetShootTarget()->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &m_vecBurstTargetPos );
		m_nBurstMode = BURST_DELIBERATELY_MISS;
		// NOTE: Fall through to BURST_DELIBERATELY_MISS!!

	case BURST_DELIBERATELY_MISS:
		return ComputeBurstDeliberatelyMissTrajectory( shootOrigin );

	case BURST_LOCK_ON_AFTER_HIT:
		// See if our target is within the bounds of the enemy
		if ( GetShootTarget()->CollisionProp()->IsPointInBounds( m_vecBurstTargetPos ) )
		{
			// Now raytrace against only the world + (good for cops on bridges)
			trace_t tr;
			CTraceFilterWorldOnly traceFilter;
			UTIL_TraceLine( Weapon_ShootPosition(), m_vecBurstTargetPos, MASK_SOLID, &traceFilter, &tr );
			if ( tr.fraction == 1.0f )
			{
				m_nBurstMode = BURST_LOCKED_ON;
			}
		}
		// NOTE: Fall through to BURST_ACTIVE!

	case BURST_ACTIVE:
		// Stitch toward the target, we haven't hit it yet
		return ComputeBurstTrajectory( shootOrigin );

	case BURST_TIGHT_GROUPING:
		// This one goes right at the enemy
		return ComputeTightBurstTrajectory( shootOrigin );
	}

	Assert(0);
	return vec3_origin;
}


//-----------------------------------------------------------------------------
// Burst mode!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::FireBullets( const FireBulletsInfo_t &info )
{
	CBaseEntity *pEnemy = GetEnemy();
	bool bIsPlayer = pEnemy && pEnemy->IsPlayer();
	if ( bIsPlayer && IsCurrentlyFiringBurst() )
	{
		FireBulletsInfo_t actualInfo = info;
		if ( m_nBurstHits < m_nMaxBurstHits )
		{
			CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pEnemy);

			// This makes it so that if the player gets hit underwater, 
			// he won't take damage if his viewpoint is above water.
			if ( !IsEnemyInAnAirboat() && ( pPlayer->GetWaterLevel() != 3 ) )
			{
				actualInfo.m_nFlags |= FIRE_BULLETS_DONT_HIT_UNDERWATER;
			}

			// This test is here to see if we've damaged the player
			int nPrevHealth = pPlayer->GetHealth();
			int nPrevArmor = pPlayer->ArmorValue();

			BaseClass::FireBullets( actualInfo );

			if (( pPlayer->GetHealth() < nPrevHealth ) || ( pPlayer->ArmorValue() < nPrevArmor ))
			{
				++m_nBurstHits;
			}
		}
		else
		{
			actualInfo.m_pAdditionalIgnoreEnt = pEnemy;
			BaseClass::FireBullets( actualInfo ); 
		}
	}
	else
	{
		BaseClass::FireBullets( info );
	}
}


//-----------------------------------------------------------------------------
// Behaviors! Lovely behaviors
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CreateBehaviors()
{
	AddBehavior( &m_RappelBehavior );
	AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_PolicingBehavior );
	AddBehavior( &m_ActBusyBehavior );
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FuncTankBehavior );
	
	return BaseClass::CreateBehaviors();
}

void CNPC_MetroPolice::InputEnableManhackToss( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY ) )
	{
		RemoveSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputSetPoliceGoal( inputdata_t &inputdata )
{
	CBaseEntity *pGoal = gEntList.FindEntityByName( NULL, inputdata.value.String() );

	if ( pGoal == NULL )
	{
		DevMsg( "SetPoliceGoal: %s (%s) unable to find ai_goal_police: %s\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	CAI_PoliceGoal *pPoliceGoal = dynamic_cast<CAI_PoliceGoal *>(pGoal);

	if ( pPoliceGoal == NULL )
	{
		DevMsg( "SetPoliceGoal: %s (%s)'s target %s is not an ai_goal_police entity!\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	m_PolicingBehavior.Enable( pPoliceGoal );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputActivateBaton( inputdata_t &inputdata )
{
	SetBatonState( inputdata.value.Bool() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AlertSound( void )
{
	m_Sentences.Speak( "METROPOLICE_GO_ALERT" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::DeathSound( const CTakeDamageInfo &info )
{
	if ( IsOnFire() )
		return;

	m_Sentences.Speak( "METROPOLICE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
}


//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::LostEnemySound( void)
{
	// Don't announce enemies when the player isn't a criminal
	if ( !PlayerIsCriminal() )
		return;

	if ( gpGlobals->curtime <= m_flNextLostSoundTime )
		return;

	const char *pSentence;
	if (!(CBaseEntity*)GetEnemy() || gpGlobals->curtime - GetEnemyLastTimeSeen() > 10)
	{
		pSentence = "METROPOLICE_LOST_LONG"; 
	}
	else
	{
		pSentence = "METROPOLICE_LOST_SHORT";
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
void CNPC_MetroPolice::FoundEnemySound( void)
{
	// Don't announce enemies when I'm in arrest behavior
	if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
		return;

	m_Sentences.Speak( "METROPOLICE_REFIND_ENEMY", SENTENCE_PRIORITY_HIGH );
}


//-----------------------------------------------------------------------------
// Purpose: Indicates whether or not this npc should play an idle sound now.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldPlayIdleSound( void )
{
	// If someone is waiting for a response, then respond!
	if ( ( m_NPCState == NPC_STATE_IDLE ) || ( m_NPCState == NPC_STATE_ALERT ) )
	{
		if ( m_nIdleChatterType >= METROPOLICE_CHATTER_RESPONSE )
			return FOkToMakeSound();
	}

	return BaseClass::ShouldPlayIdleSound();
}


//-----------------------------------------------------------------------------
// IdleSound 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::IdleSound( void )
{
	bool bIsCriminal = PlayerIsCriminal();

	// This happens when the NPC is waiting for his buddies to respond to him
	switch( m_nIdleChatterType )
	{
	case METROPOLICE_CHATTER_WAIT_FOR_RESPONSE:
		break;

	case METROPOLICE_CHATTER_ASK_QUESTION:
		{
			if ( m_bPlayerIsNear && !HasMemory(bits_MEMORY_PLAYER_HARASSED) )
			{
				if ( m_Sentences.Speak( "METROPOLICE_IDLE_HARASS_PLAYER", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL ) >= 0 )
				{
					Remember( bits_MEMORY_PLAYER_HARASSED );
					if ( GetSquad() )
					{
						GetSquad()->SquadRemember(bits_MEMORY_PLAYER_HARASSED);
					}
				}
				return;
			}

			if ( !random->RandomInt(0,1) )
				break;

			int nQuestionType = random->RandomInt( 0, METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT );
			if ( !IsInSquad() || ( nQuestionType == METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT ) )
			{
				m_Sentences.Speak( bIsCriminal ? "METROPOLICE_IDLE_CR" : "METROPOLICE_IDLE" );
				break;
			}

			static const char *pQuestion[2][METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT] = 
			{
				{ "METROPOLICE_IDLE_CHECK",		"METROPOLICE_IDLE_QUEST" },
				{ "METROPOLICE_IDLE_CHECK_CR",	"METROPOLICE_IDLE_QUEST_CR" },
			};

			if ( m_Sentences.Speak( pQuestion[bIsCriminal][nQuestionType] ) >= 0 )
			{
				GetSquad()->BroadcastInteraction( g_interactionMetrocopIdleChatter, (void*)(intp)(METROPOLICE_CHATTER_RESPONSE + nQuestionType), this );
				m_nIdleChatterType = METROPOLICE_CHATTER_WAIT_FOR_RESPONSE;
			}
		}
		break;

	default:
		{
			int nResponseType = m_nIdleChatterType - METROPOLICE_CHATTER_RESPONSE;

			static const char *pResponse[2][METROPOLICE_CHATTER_RESPONSE_TYPE_COUNT] = 
			{
				{ "METROPOLICE_IDLE_CLEAR",		"METROPOLICE_IDLE_ANSWER" },
				{ "METROPOLICE_IDLE_CLEAR_CR",	"METROPOLICE_IDLE_ANSWER_CR" },
			};

			if ( m_Sentences.Speak( pResponse[bIsCriminal][nResponseType] ) >= 0 )
			{
				GetSquad()->BroadcastInteraction( g_interactionMetrocopIdleChatter, (void*)(METROPOLICE_CHATTER_ASK_QUESTION), this );
				m_nIdleChatterType = METROPOLICE_CHATTER_ASK_QUESTION;
			}
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime < m_flNextPainSoundTime )
		return;

	// Don't make pain sounds if I'm on fire. The looping sound will take care of that for us.
	if ( IsOnFire() )
		return;

	float healthRatio = (float)GetHealth() / (float)GetMaxHealth();
	if ( healthRatio > 0.0f )
	{
		const char *pSentenceName = "METROPOLICE_PAIN";
		if ( !HasMemory(bits_MEMORY_PAIN_HEAVY_SOUND) && (healthRatio < 0.25f) )
		{
			Remember( bits_MEMORY_PAIN_HEAVY_SOUND | bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "METROPOLICE_PAIN_HEAVY";
		}
		else if ( !HasMemory(bits_MEMORY_PAIN_LIGHT_SOUND) && healthRatio > 0.8f )
		{
			Remember( bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "METROPOLICE_PAIN_LIGHT";
		}
		
		// This causes it to speak it no matter what; doesn't bother with setting sounds.
		m_Sentences.Speak( pSentenceName, SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		m_flNextPainSoundTime = gpGlobals->curtime + 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_DANGER | 
		SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 120;

	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;

	case ACT_WALK:
	case ACT_WALK_CROUCH:
	case ACT_RUN_CROUCH:
		return 25;

	default:
		return 45;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_MetroPolice::Classify ( void )
{
	return CLASS_METROPOLICE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::PlayerIsCriminal( void )
{
	if ( m_PolicingBehavior.IsEnabled() && m_PolicingBehavior.TargetIsHostile() )
		return true;

	if ( GlobalEntity_GetState( "gordon_precriminal" ) == GLOBAL_ON )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
// Input  : pTarget - Entity with which to determine relationship.
// Output : Returns relationship value.
//-----------------------------------------------------------------------------
Disposition_t CNPC_MetroPolice::IRelationType(CBaseEntity *pTarget)
{
	Disposition_t disp = BaseClass::IRelationType(pTarget);

	if ( pTarget == NULL )
		return disp;

	// If the player's not a criminal, then we don't necessary hate him
	if ( pTarget->Classify() == CLASS_PLAYER )
	{
		if ( !PlayerIsCriminal() && (disp == D_HT) )
		{
			// If we're pissed at the player, we're allowed to hate them.
			if ( m_flChasePlayerTime && m_flChasePlayerTime > gpGlobals->curtime )
				return D_HT;
			return D_NU;
		}
	}

	return disp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventStartDeployManhack( void )
{
	Assert( m_iManhacks );
	
	if ( m_iManhacks <= 0 )
	{
		DevMsg( "Error: Throwing manhack but out of manhacks!\n" );
		return;
	}

	m_iManhacks--;

	// Turn off the manhack on our body
	if ( m_iManhacks <= 0 )
	{
		SetBodygroup( METROPOLICE_BODYGROUP_MANHACK, false );
	}

	// Create the manhack to throw
	CNPC_Manhack *pManhack = (CNPC_Manhack *)CreateEntityByName( "npc_manhack" );
	
	Vector	vecOrigin;
	QAngle	vecAngles;

	int handAttachment = LookupAttachment( "LHand" );
	GetAttachment( handAttachment, vecOrigin, vecAngles );

	pManhack->SetLocalOrigin( vecOrigin );
	pManhack->SetLocalAngles( vecAngles );
	pManhack->AddSpawnFlags( (SF_MANHACK_PACKED_UP|SF_MANHACK_CARRIED|SF_NPC_WAIT_FOR_SCRIPT) );
	
	// Also fade if our parent is marked to do it
	if ( HasSpawnFlags( SF_NPC_FADE_CORPSE ) )
	{
		pManhack->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	pManhack->Spawn();

	// Make us move with his hand until we're deployed
	pManhack->SetParent( this, handAttachment );

	m_hManhack = pManhack;
}

//-----------------------------------------------------------------------------
// Anim event handlers
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventDeployManhack( animevent_t *pEvent )
{
	// Let it go
	ReleaseManhack();

	Vector forward, right;
	GetVectors( &forward, &right, NULL );

	IPhysicsObject *pPhysObj = m_hManhack->VPhysicsGetObject();

	if ( pPhysObj )
	{
		Vector	yawOff = right * random->RandomFloat( -1.0f, 1.0f );

		Vector	forceVel = ( forward + yawOff * 16.0f ) + Vector( 0, 0, 250 );
		Vector	forceAng = vec3_origin;

		// Give us velocity
		pPhysObj->AddVelocity( &forceVel, &forceAng );
	}

	// Stop dealing with this manhack
	m_hManhack = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventShove( void )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 16, Vector(-16,-16,-16), Vector(16,16,16), 15, DMG_CLUB, 1.0f, false );

	if ( pHurt )
	{
		Vector vecForceDir = ( pHurt->WorldSpaceCenter() - WorldSpaceCenter() );

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL )
		{
			//Kick the player angles
			pPlayer->ViewPunch( QAngle( 8, 14, 0 ) );

			Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(dir);

			QAngle angles;
			VectorAngles( dir, angles );
			Vector forward, right;
			AngleVectors( angles, &forward, &right, NULL );

			//If not on ground, then don't make them fly!
			if ( !(pHurt->GetFlags() & FL_ONGROUND ) )
				  forward.z = 0.0f;

			//Push the target back
			pHurt->ApplyAbsVelocityImpulse( forward * 250.0f );

			// Force the player to drop anyting they were holding
			pPlayer->ForceDropOfCarriedPhysObjects();
		}

		// Play a random attack hit sound
		EmitSound( "NPC_Metropolice.Shove" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventBatonOn( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());

	if ( pStick )
	{
		pStick->SetStunState( true );
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnAnimEventBatonOff( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());
	
	if ( pStick )
	{
		pStick->SetStunState( false );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pEvent - 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::HandleAnimEvent( animevent_t *pEvent )
{
	// Shove!
	if ( pEvent->event == AE_METROPOLICE_SHOVE )
	{
		OnAnimEventShove();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_BATON_ON )
	{
		OnAnimEventBatonOn();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_BATON_OFF )
	{
		OnAnimEventBatonOff();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_START_DEPLOY )
	{
		OnAnimEventStartDeployManhack();
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_DRAW_PISTOL )
	{
		m_fWeaponDrawn = true;
		if( GetActiveWeapon() )
		{
			GetActiveWeapon()->RemoveEffects( EF_NODRAW );
		}
		return;
	}

	if ( pEvent->event == AE_METROPOLICE_DEPLOY_MANHACK )
	{
		OnAnimEventDeployManhack( pEvent );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if ( interactionType == g_interactionMetrocopStartedStitch )
	{
		// If anybody in our squad started a stitch, we can't for a little while
		m_flValidStitchTime = gpGlobals->curtime + random->RandomFloat( METROPOLICE_SQUAD_STITCH_MIN_INTERVAL, METROPOLICE_SQUAD_STITCH_MAX_INTERVAL );
		return true;
	}

	if ( interactionType == g_interactionMetrocopIdleChatter )
	{
		m_nIdleChatterType = (int)(intp)data;
		return true;
	}

	if ( interactionType == g_interactionMetrocopClearSentenceQueues )
	{
			m_Sentences.ClearQueue();
			return true;
	}

	// React to being hit by physics objects
	if ( interactionType == g_interactionHitByPlayerThrownPhysObj )
	{
		// Ignore if I'm in scripted state
		if ( !IsInAScript() && (m_NPCState != NPC_STATE_SCRIPT) )
		{
			SetCondition( COND_METROPOLICE_PHYSOBJECT_ASSAULT );
		}
		else
		{
			AdministerJustice();
		}

		// See if the object is the cupcop can. If so, fire the output (for x360 achievement)
		CBaseProp *pProp = (CBaseProp*)data;
		if( pProp != NULL )
		{
			if( pProp->NameMatches("cupcop_can") )
				m_OnCupCopped.FireOutput( this, NULL );
		}

		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Activity CNPC_MetroPolice::NPC_TranslateActivity( Activity newActivity )
{
	if( IsOnFire() && newActivity == ACT_RUN )
	{
		return ACT_RUN_ON_FIRE;
	}

	// If we're shoving, see if we should be more forceful in doing so
	if ( newActivity == ACT_PUSH_PLAYER )
	{
		if ( m_nNumWarnings >= METROPOLICE_MAX_WARNINGS )
			return ACT_MELEE_ATTACK1;
	}

	newActivity = BaseClass::NPC_TranslateActivity( newActivity );

	// This will put him into an angry idle, which will then be translated
	// by the weapon to the appropriate type. 
	if ( m_fWeaponDrawn && newActivity == ACT_IDLE && ( GetState() == NPC_STATE_COMBAT || BatonActive() ) )
	{
		newActivity = ACT_IDLE_ANGRY;
	}

	return newActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Makes the held manhack solid
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::ReleaseManhack( void )
{
	Assert( m_hManhack );

	// Make us physical
	m_hManhack->RemoveSpawnFlags( SF_MANHACK_CARRIED );
	m_hManhack->CreateVPhysics();

	// Release us
	m_hManhack->RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_hManhack->SetMoveType( MOVETYPE_VPHYSICS );
	m_hManhack->SetParent( NULL );

	// Make us active
	m_hManhack->RemoveSpawnFlags( SF_NPC_WAIT_FOR_SCRIPT );
	m_hManhack->ClearSchedule( "Manhack released by metropolice" );
	
	// Start him with knowledge of our current enemy
	if ( GetEnemy() )
	{
		m_hManhack->SetEnemy( GetEnemy() );
		m_hManhack->SetState( NPC_STATE_COMBAT );

		m_hManhack->UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin() );
	}

	// Place him into our squad so we can communicate
	if ( m_pSquad )
	{
		m_pSquad->AddToSquad( m_hManhack );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Event_Killed( const CTakeDamageInfo &info )
{
	// Release the manhack if we're in the middle of deploying him
	if ( m_hManhack && m_hManhack->IsAlive() )
	{
		ReleaseManhack();
		m_hManhack = NULL;
	}

	CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

	if ( pPlayer != NULL )
	{
		CHalfLife2 *pHL2GameRules = static_cast<CHalfLife2 *>(g_pGameRules);

		// Attempt to drop health
		if ( pHL2GameRules->NPC_ShouldDropHealth( pPlayer ) )
		{
			DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			pHL2GameRules->NPC_DroppedHealth();
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Try to enter a slot where we shoot a pistol 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::TryToEnterPistolSlot( int nSquadSlot )
{
	// This logic here will not allow us to occupy the a squad slot
	// too soon after we already were in it.
	if ( ( m_LastShootSlot != nSquadSlot || !m_TimeYieldShootSlot.Expired() ) &&
			OccupyStrategySlot( nSquadSlot ) )
	{
		if ( m_LastShootSlot != nSquadSlot )
		{
			m_TimeYieldShootSlot.Reset();
			m_LastShootSlot = nSquadSlot;
		}
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectRangeAttackSchedule()
{
	if ( HasSpawnFlags( SF_METROPOLICE_ALWAYS_STITCH ) )
	{
		int nSched = SelectMoveToLedgeSchedule();
		if ( nSched != SCHED_NONE )
			return nSched;
	}

	// Range attack if we're able
	if( TryToEnterPistolSlot( SQUAD_SLOT_ATTACK1 ) || TryToEnterPistolSlot( SQUAD_SLOT_ATTACK2 ))
		return SCHED_RANGE_ATTACK1;
	
	// We're not in a shoot slot... so we've allowed someone else to grab it
	m_LastShootSlot = SQUAD_SLOT_NONE;

	if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
	{
		return SCHED_METROPOLICE_DEPLOY_MANHACK;
	}

	return SCHED_METROPOLICE_ADVANCE;
}


//-----------------------------------------------------------------------------
// How many squad members are trying to arrest the player?
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SquadArrestCount()
{
	int nCount = 0;

	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		if ( pSquadmate->IsCurSchedule(	SCHED_METROPOLICE_ARREST_ENEMY ) ||
			pSquadmate->IsCurSchedule( SCHED_METROPOLICE_WARN_AND_ARREST_ENEMY ) )
		{
			++nCount;
		}

		pSquadmate = m_pSquad->GetNextMember( &iter );
	}

	return nCount;
}


//-----------------------------------------------------------------------------
// Arrest schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleArrestEnemy()
{
	if ( !HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) || !IsInSquad() )
		return SCHED_NONE;

	if ( !HasCondition( COND_SEE_ENEMY ) )
		return SCHED_NONE;

	if ( !m_fWeaponDrawn )
		return SCHED_METROPOLICE_DRAW_PISTOL;

	// First guy that sees the enemy will tell him to freeze
	if ( OccupyStrategySlot( SQUAD_SLOT_POLICE_ARREST_ENEMY ) )
		return SCHED_METROPOLICE_WARN_AND_ARREST_ENEMY;

	// Squad members 1 -> n will simply gain a line of sight
	return SCHED_METROPOLICE_ARREST_ENEMY;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleNewEnemy()
{
	int nSched = SelectScheduleArrestEnemy();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition( COND_NEW_ENEMY ) )
	{
		m_flNextLedgeCheckTime = gpGlobals->curtime;

		if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
			return SCHED_METROPOLICE_DEPLOY_MANHACK;
	}

	if ( !m_fWeaponDrawn )
		return SCHED_METROPOLICE_DRAW_PISTOL;

	// Switch our baton on, if it's not already
	if ( HasBaton() && BatonActive() == false && IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) == false )
	{
		SetTarget( GetEnemy() );
		SetBatonState( true );
		m_flBatonDebounceTime = gpGlobals->curtime + random->RandomFloat( 2.5f, 4.0f );
		return SCHED_METROPOLICE_ACTIVATE_BATON;
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Sound investigation 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleInvestigateSound()
{
	// SEE_ENEMY is set if LOS is available *and* we're looking the right way
	// Don't investigate if the player's not a criminal.
	if ( PlayerIsCriminal() && !HasCondition( COND_SEE_ENEMY ) )
	{
		if ( HasCondition( COND_HEAR_COMBAT ) || HasCondition( COND_HEAR_PLAYER ) )
		{
			if ( m_pSquad && OccupyStrategySlot( SQUAD_SLOT_INVESTIGATE_SOUND ) )
			{
				return SCHED_METROPOLICE_INVESTIGATE_SOUND;
			}
		}
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.pObstruction )
	{
		// Is it a physics prop? Store it off as the last thing to block me
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>( pMoveGoal->directTrace.pObstruction );
		if ( pProp && pProp->GetHealth() )
		{
			m_hBlockingProp = pProp;
		}
		else
		{
			m_hBlockingProp = NULL;
		}
	}

	return BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectScheduleNoDirectEnemy()
{
	// If you can't attack, but you can deploy a manhack, do it!
	if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
		return SCHED_METROPOLICE_DEPLOY_MANHACK;

	// If you can't attack, but you have a baton & there's a physics object in front of you, swat it
	if ( m_hBlockingProp && HasBaton() )
	{
		SetTarget( m_hBlockingProp );
		m_hBlockingProp = NULL;
		return SCHED_METROPOLICE_SMASH_PROP;
	}

	return SCHED_METROPOLICE_CHASE_ENEMY;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectCombatSchedule()
{
	// Announce a new enemy
	if ( HasCondition( COND_NEW_ENEMY ) )
	{
		AnnounceEnemyType( GetEnemy() );
	}

	int nResult = SelectScheduleNewEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	if( !m_fWeaponDrawn )
	{
		return SCHED_METROPOLICE_DRAW_PISTOL;
	}

	if (!HasBaton() && ((float)m_nRecentDamage / (float)GetMaxHealth()) > RECENT_DAMAGE_THRESHOLD)
	{
		m_nRecentDamage = 0;
		m_flRecentDamageTime = 0;
		m_Sentences.Speak( "METROPOLICE_COVER_HEAVY_DAMAGE", SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );

		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( !GetShotRegulator()->IsInRestInterval() )
			return SelectRangeAttackSchedule();
		else
			return SCHED_METROPOLICE_ADVANCE;
	}

	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		if ( m_BatonSwingTimer.Expired() )
		{
			// Stop chasing the player now that we've taken a swing at them
			m_flChasePlayerTime = 0;
			m_BatonSwingTimer.Set( 1.0, 1.75 );
			return SCHED_MELEE_ATTACK1;
		}
		else
			return SCHED_COMBAT_FACE;
	}

	if ( HasCondition( COND_TOO_CLOSE_TO_ATTACK ) )
	{
		return SCHED_BACK_AWAY_FROM_ENEMY;
	}
	
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) || HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		AnnounceOutOfAmmo( );
		return SCHED_HIDE_AND_RELOAD;
	}

	if ( HasCondition(COND_WEAPON_SIGHT_OCCLUDED) && !HasBaton() )
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlotRange( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1, SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2 ) )
		{
			m_Sentences.Speak( "METROPOLICE_SHOOT_COVER" );
			return SCHED_SHOOT_ENEMY_COVER;
		}
	}

	if (HasCondition(COND_ENEMY_OCCLUDED))
	{
		if ( GetEnemy() && !(GetEnemy()->GetFlags() & FL_NOTARGET) )
		{
			// Charge in and break the enemy's cover!
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}
	}

	nResult = SelectScheduleNoDirectEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: This is a bridge between stunstick, NPC and its behavior
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldKnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_PolicingBehavior.IsEnabled() && m_PolicingBehavior.ShouldKnockOutTarget( pTarget ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: This is a bridge between stunstick, NPC and its behavior
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::KnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_PolicingBehavior.IsEnabled() )
	{
		m_PolicingBehavior.KnockOutTarget( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Can me enemy see me? 
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanEnemySeeMe( )
{
	if ( GetEnemy()->IsPlayer() )
	{
		if ( static_cast<CBasePlayer*>(GetEnemy())->FInViewCone( this ) )
		{
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Choose weights about where we can use particular stitching behaviors 
//-----------------------------------------------------------------------------
#define STITCH_MIN_DISTANCE 1000.0f
#define STITCH_MIN_DISTANCE_SLOW 1250.0f

#define STITCH_AT_CONE	0.866f	// cos(30)
#define STITCH_AT_CONE_WHEN_VISIBLE_MAX	0.3f	// cos(?)
#define STITCH_AT_CONE_WHEN_VISIBLE_MIN	0.707f	// cos(45)
#define STITCH_AT_COS_MIN_SPIN_ANGLE	0.2f

float CNPC_MetroPolice::StitchAtWeight( float flDist, float flSpeed, float flDot, float flReactionTime, const Vector &vecTargetToGun )
{
	// Can't do an 'attacking' stitch if it's too soon
	if ( m_flValidStitchTime > gpGlobals->curtime )
		return 0.0f;

	// No squad slots? no way.
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		return false;

	// Don't do it if the player doesn't have enough time to react
	if ( flDist < STITCH_MIN_DISTANCE )
		return 0.0f;

	// Don't do it if the player is farther but really slow
	if ( ( flDist < STITCH_MIN_DISTANCE_SLOW ) && ( flSpeed < 150.0f ) )
		return 0.0f;

	// Does the predicted stitch position cross the plane from me to the target's initial position?
	// If so, it'll look really dumb. Disallow that.
	Vector vecGunToPredictedTarget, vecShootAtVel;
	PredictShootTargetPosition( flReactionTime, 0.0f, 0.0f, &vecGunToPredictedTarget, &vecShootAtVel );
	vecGunToPredictedTarget -= Weapon_ShootPosition();
	vecGunToPredictedTarget.z = 0.0f;
	VectorNormalize( vecGunToPredictedTarget );

	Vector2D vecGunToTarget;
	Vector2DMultiply( vecTargetToGun.AsVector2D(), -1.0f, vecGunToTarget );
	Vector2DNormalize( vecGunToTarget );
	if ( DotProduct2D( vecGunToTarget, vecGunToPredictedTarget.AsVector2D() ) <= STITCH_AT_COS_MIN_SPIN_ANGLE )
		return 0.0f;

	// If the cop is in the view cone, then up the cone in which the stitch will occur 
	float flConeAngle = STITCH_AT_CONE;
	if ( CanEnemySeeMe() )
	{
		flDist = clamp( flDist, 1500.0f, 2500.0f );
		flConeAngle = RemapVal( flDist, 1500.0f, 2500.0f, STITCH_AT_CONE_WHEN_VISIBLE_MIN, STITCH_AT_CONE_WHEN_VISIBLE_MAX );
	}

	flDot = clamp( flDot, -1.0f, flConeAngle );
	return RemapVal( flDot, -1.0f, flConeAngle, 0.5f, 1.0f );
}


#define STITCH_ACROSS_CONE	0.5f	// cos(60)

float CNPC_MetroPolice::StitchAcrossWeight( float flDist, float flSpeed, float flDot, float flReactionTime )
{
	return 0.0f;

	// Can't do an 'attacking' stitch if it's too soon
	if ( m_flValidStitchTime > gpGlobals->curtime )
		return 0.0f;

	// No squad slots? no way.
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		return 0.0f;

	if ( flDist < STITCH_MIN_DISTANCE )
		return 0.0f;

	// Don't do it if the player doesn't have enough time to react
	if ( flDist < flSpeed * flReactionTime )
		return 0.0f;

	// We want to stitch across if we're within the stitch across cone
	if ( flDot < STITCH_ACROSS_CONE )
		return 0.0f;

	return 1.0f;
}


#define STITCH_ALONG_MIN_CONE	0.866f	// cos(30)
#define STITCH_ALONG_MIN_CONE_WHEN_VISIBLE	0.707f	// cos(45)
#define STITCH_ALONG_MAX_CONE	-0.4f	//
#define STITCH_ALONG_MIN_SPEED	300.0f

float CNPC_MetroPolice::StitchAlongSideWeight( float flDist, float flSpeed, float flDot )
{
	// No squad slots? no way.
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) &&
		IsStrategySlotRangeOccupied( SQUAD_SLOT_POLICE_COVERING_FIRE1, SQUAD_SLOT_POLICE_COVERING_FIRE2 ) )
		return 0.0f;

	if ( flDist < (AIM_ALONG_SIDE_LINE_OF_DEATH_DISTANCE + AIM_ALONG_SIDE_STEER_DISTANCE + 100.0f) )
		return 0.0f;

	if ( flSpeed < STITCH_ALONG_MIN_SPEED )
		return 0.0f;

	// We want to stitch across if we're within the stitch across cone
	float flMinConeAngle = STITCH_ALONG_MIN_CONE;
	bool bCanEnemySeeMe = CanEnemySeeMe( );
	if ( bCanEnemySeeMe )
	{
		flMinConeAngle = STITCH_ALONG_MIN_CONE_WHEN_VISIBLE;
	}

	if (( flDot > flMinConeAngle ) || ( flDot < STITCH_ALONG_MAX_CONE ))
		return 0.0f;

	return bCanEnemySeeMe ? 1.0f : 2.0f;
}

#define STITCH_BEHIND_MIN_CONE	0.0f	// cos(90)

float CNPC_MetroPolice::StitchBehindWeight( float flDist, float flSpeed, float flDot )
{
	return 0.0f;

	// No squad slots? no way.
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) &&
		IsStrategySlotRangeOccupied( SQUAD_SLOT_POLICE_COVERING_FIRE1, SQUAD_SLOT_POLICE_COVERING_FIRE2 ) )
		return 0.0f;

	if ( flDist < AIM_BEHIND_MINIMUM_DISTANCE )
		return 0.0f;

	// We want to stitch across if we're within the stitch across cone
	if ( flDot > STITCH_BEHIND_MIN_CONE )
		return 0.0f;

	// If we're close, reduce the chances of this if we're also slow
	if ( flDist < STITCH_MIN_DISTANCE )
	{
		flSpeed = clamp( flSpeed, 300.0f, 450.0f );
		float flWeight = RemapVal( flSpeed, 300.0f, 450.0f, 0.0f, 1.0f );
		return flWeight;
	}

	return 1.0f;
}

float CNPC_MetroPolice::StitchTightWeight( float flDist, float flSpeed, const Vector &vecTargetToGun, const Vector &vecVelocity )
{
	// No squad slots? no way.
	if( IsStrategySlotRangeOccupied( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) &&
		IsStrategySlotRangeOccupied( SQUAD_SLOT_POLICE_COVERING_FIRE1, SQUAD_SLOT_POLICE_COVERING_FIRE2 ) )
		return 0.0f;

	if ( flDist > STITCH_MIN_DISTANCE )
	{
		if ( flDist > 2000.0f )
			return 0.0f;

		// We can stitch tight if they are close and no other rules apply.
		return 0.0001f;
	}

	// If we're heading right at him, them fire it!
	Vector vecTargetToGunDir = vecTargetToGun;
	Vector vecVelocityDir = vecVelocity;
	VectorNormalize( vecTargetToGunDir );
	VectorNormalize( vecVelocityDir );

	if ( DotProduct( vecTargetToGunDir, vecVelocityDir ) > 0.95f )
		return 8.0f;

	// If we're on the same level, fire at him!
	if ( ( fabs(vecTargetToGun.z) < 50.0f ) && ( flDist < STITCH_MIN_DISTANCE ) )
		return 1.0f;

	flSpeed = clamp( flSpeed, 300.0f, 450.0f );
	float flWeight = RemapVal( flSpeed, 300.0f, 450.0f, 1.0f, 0.0f );
	return flWeight;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
#define STITCH_REACTION_TIME 2.0f
#define STITCH_SCHEDULE_COUNT 5

int CNPC_MetroPolice::SelectStitchSchedule()
{
	// If the boat is very close to us, we're going to stitch at it
	// even if the squad slot is full..
	Vector vecTargetToGun;
	Vector vecTarget = StitchAimTarget( GetAbsOrigin(), false );
	VectorSubtract( Weapon_ShootPosition(), vecTarget, vecTargetToGun );

	Vector2D vecTargetToGun2D = vecTargetToGun.AsVector2D();
	float flDist = Vector2DNormalize( vecTargetToGun2D );

	if ( HasSpawnFlags( SF_METROPOLICE_NO_FAR_STITCH ) )
	{
		if ( flDist > 6000.0f )
			return SCHED_NONE;
	}

	float flReactionTime = STITCH_REACTION_TIME * sk_metropolice_stitch_reaction.GetFloat();
	Vector vecVelocity;
	PredictShootTargetVelocity( flReactionTime, &vecVelocity );

	Vector2D vecVelocity2D = vecVelocity.AsVector2D();
	float flSpeed = Vector2DNormalize( vecVelocity2D );
	float flDot = DotProduct2D( vecTargetToGun2D, vecVelocity2D );

	float flWeight[STITCH_SCHEDULE_COUNT];
	flWeight[0] = StitchAtWeight( flDist, flSpeed, flDot, flReactionTime, vecTargetToGun );
	flWeight[1] = flWeight[0] + StitchAcrossWeight( flDist, flSpeed, flDot, flReactionTime );
	flWeight[2] = flWeight[1] + StitchTightWeight( flDist, flSpeed, vecTargetToGun, vecVelocity );
	flWeight[3] = flWeight[2] + StitchAlongSideWeight( flDist, flSpeed, flDot );
	flWeight[4] = flWeight[3] + StitchBehindWeight( flDist, flSpeed, flDot );

	if ( flWeight[STITCH_SCHEDULE_COUNT - 1] == 0.0f )
		return SCHED_NONE;

	int pSched[STITCH_SCHEDULE_COUNT] =
	{
		SCHED_METROPOLICE_AIM_STITCH_AT_AIRBOAT,
		SCHED_METROPOLICE_AIM_STITCH_IN_FRONT_OF_AIRBOAT,
		SCHED_METROPOLICE_AIM_STITCH_TIGHTLY,
		SCHED_METROPOLICE_AIM_STITCH_ALONG_SIDE_OF_AIRBOAT,
		SCHED_METROPOLICE_AIM_STITCH_BEHIND_AIRBOAT,
	};

	int i;
	float flRand = random->RandomFloat( 0.0f, flWeight[STITCH_SCHEDULE_COUNT - 1] );
	for ( i = 0; i < STITCH_SCHEDULE_COUNT; ++i )
	{
		if ( flRand <= flWeight[i] )
			break;
	}

	// If we're basically a covering activity, take up that slot
	if ( i >= 3 )
	{
		if( OccupyStrategySlotRange( SQUAD_SLOT_POLICE_COVERING_FIRE1, SQUAD_SLOT_POLICE_COVERING_FIRE2 ) )
		{
			return pSched[i];
		}
	}

	if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
	{
		if ( IsInSquad() && (i < 2) )
		{
			GetSquad()->BroadcastInteraction( g_interactionMetrocopStartedStitch, NULL );
		}

		return pSched[i];
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectMoveToLedgeSchedule()
{
	// Prevent a bunch of unnecessary raycasts.
	if ( m_flNextLedgeCheckTime > gpGlobals->curtime )
		return SCHED_NONE;

	// If the NPC is above the airboat (say, on a bridge), make sure he
	// goes to the closest ledge. (may need a spawnflag for this)
	if ( (GetAbsOrigin().z - GetShootTarget()->GetAbsOrigin().z) >= 150.0f )
	{
		m_flNextLedgeCheckTime = gpGlobals->curtime + 3.0f;

		// We need to be able to shoot downward at a 60 degree angle.
		Vector vecDelta;
		VectorSubtract( GetShootTarget()->WorldSpaceCenter(), Weapon_ShootPosition(), vecDelta );
		vecDelta.z = 0.0f;
		VectorNormalize( vecDelta );

		// At this point, vecDelta is 45 degrees below horizontal.
		vecDelta.z = -1;
		vecDelta *= 100.0f;

		trace_t tr;
		CTraceFilterWorldOnly traceFilter;
		UTIL_TraceLine( Weapon_ShootPosition(), Weapon_ShootPosition() + vecDelta, MASK_SOLID, &traceFilter, &tr );

		if (tr.endpos.z >= GetAbsOrigin().z - 25.0f )
			return SCHED_METROPOLICE_ESTABLISH_STITCH_LINE_OF_FIRE;
	}
	
	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Combat schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectAirboatRangeAttackSchedule()
{
	// Move to a ledge, if we need to.
	int nSched = SelectMoveToLedgeSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		nSched = SelectStitchSchedule();
		if ( nSched != SCHED_NONE )
		{
			m_LastShootSlot = SQUAD_SLOT_NONE;
			return nSched;
		}
	}

	if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
	{
		return SCHED_METROPOLICE_DEPLOY_MANHACK;
	}

	return SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE;
}


//-----------------------------------------------------------------------------
// Combat schedule selection for when the enemy is in an airboat
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectAirboatCombatSchedule()
{
	int nResult = SelectScheduleNewEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	// We're assuming here that the cops who attack airboats have SMGs
//	Assert( Weapon_OwnsThisType( "weapon_smg1" ) );

	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		return SelectAirboatRangeAttackSchedule();
	}
	
	if ( HasCondition( COND_WEAPON_SIGHT_OCCLUDED ) )
	{
		// If they are hiding behind something also attack. Don't bother
		// shooting the destroyable thing; it'll happen anyways with the SMG
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlotRange( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1, SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2 ) )
		{
			return SelectAirboatRangeAttackSchedule();
		}
	}

	nResult = SelectScheduleNoDirectEnemy();
	if ( nResult != SCHED_NONE )
		return nResult;

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Metropolice considers bullet fire heavy damage
	if ( info.GetDamageType() & DMG_BULLET )
		return true;

	return BaseClass::IsHeavyDamage( info );
}

//-----------------------------------------------------------------------------
// TraceAttack
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// This is needed so we can keep track of the direction of the shot
	// because we're going to use it to choose the flinch animation
	if ( m_bSimpleCops )
	{
		if ( m_takedamage == DAMAGE_YES )
		{
			Vector vecLastHitDirection;
			VectorIRotate( vecDir, EntityToWorldTransform(), vecLastHitDirection );

			// Point *at* the shooter
			vecLastHitDirection *= -1.0f;

			QAngle lastHitAngles;
			VectorAngles( vecLastHitDirection, lastHitAngles );
			m_flLastHitYaw	= lastHitAngles.y;
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Determines the best type of flinch anim to play.
//-----------------------------------------------------------------------------
Activity CNPC_MetroPolice::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	if ( !bGesture && m_bSimpleCops )
	{
		// Version for getting shot from behind
		if ( ( m_flLastHitYaw > 90 ) && ( m_flLastHitYaw < 270 ) )
		{
			Activity flinchActivity = (Activity)ACT_METROPOLICE_FLINCH_BEHIND;
			if ( SelectWeightedSequence ( flinchActivity ) != ACTIVITY_NOT_AVAILABLE )
				return flinchActivity;
		}

		if ( ( LastHitGroup() == HITGROUP_CHEST ) ||
			( LastHitGroup() == HITGROUP_LEFTLEG ) ||
			( LastHitGroup() == HITGROUP_RIGHTLEG ) )
		{
			Activity flinchActivity = ACT_FLINCH_STOMACH;
			if ( SelectWeightedSequence ( ACT_FLINCH_STOMACH ) != ACTIVITY_NOT_AVAILABLE )
				return flinchActivity;
		}
	}

	return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PlayFlinchGesture( void )
{
	BaseClass::PlayFlinchGesture();

	// To ensure old playtested difficulty stays the same, stop cops shooting for a bit after gesture flinches
	GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 0.5 );
}

//-----------------------------------------------------------------------------
// We're taking cover from danger
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AnnounceHarrassment( void )
{
	static const char *pWarnings[3] = 
	{
		"METROPOLICE_BACK_UP_A",
		"METROPOLICE_BACK_UP_B",
		"METROPOLICE_BACK_UP_C",
	};

	m_Sentences.Speak( pWarnings[ random->RandomInt( 0, ARRAYSIZE(pWarnings)-1 ) ], SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::IncrementPlayerCriminalStatus( void )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );

	if ( pPlayer )
	{
		AddLookTarget( pPlayer, 0.8f, 5.0f );

		if ( m_nNumWarnings < METROPOLICE_MAX_WARNINGS )
		{
			m_nNumWarnings++;
		}

		if ( m_nNumWarnings >= (METROPOLICE_MAX_WARNINGS-1) )
		{
			SetTarget( pPlayer );
			SetBatonState( true );
		}
	}

	m_flBatonDebounceTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 4.0f );

	AnnounceHarrassment();

	m_bKeepFacingPlayer = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectShoveSchedule( void )
{
	IncrementPlayerCriminalStatus();

	// Stop chasing the player now that we've taken a swing at them
	m_flChasePlayerTime = 0;
	return SCHED_METROPOLICE_SHOVE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::GetIdealAccel( void ) const
{
	return GetIdealSpeed() * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Chase after a player who's just pissed us off, and hit him
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AdministerJustice( void )
{
	if ( !AI_IsSinglePlayer() )
		return;

	// If we're allowed to chase the player, do so. Otherwise, just threaten.
	if ( !IsInAScript() && (m_NPCState != NPC_STATE_SCRIPT) && HasSpawnFlags( SF_METROPOLICE_ALLOWED_TO_RESPOND ) )
	{
		if ( m_vecPreChaseOrigin == vec3_origin )
		{
			m_vecPreChaseOrigin = GetAbsOrigin();
			m_flPreChaseYaw = GetAbsAngles().y;
		}
		m_flChasePlayerTime = gpGlobals->curtime + RandomFloat( 3, 7 );

		// Attack the target
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		SetEnemy( pPlayer );
		SetState( NPC_STATE_COMBAT );
		UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
	else
	{
		// Watch the player for a time.
		m_bKeepFacingPlayer = true;

		// Try and find a nearby cop to administer justice
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		int nAIs = g_AI_Manager.NumAIs();
		for ( int i = 0; i < nAIs; i++ )
		{
			if ( ppAIs[i] == this )
				continue;

			if ( ppAIs[i]->Classify() == CLASS_METROPOLICE && FClassnameIs( ppAIs[i], "npc_metropolice" ) )
			{
				CNPC_MetroPolice *pNPC = assert_cast<CNPC_MetroPolice*>(ppAIs[i]);
				if ( pNPC->HasSpawnFlags( SF_METROPOLICE_ALLOWED_TO_RESPOND ) )
				{
					// Is he within site & range?
					if ( FVisible(pNPC) && pNPC->FVisible( UTIL_PlayerByIndex(1) ) && 
						UTIL_DistApprox( WorldSpaceCenter(), pNPC->WorldSpaceCenter() ) < 512 )
					{
						pNPC->AdministerJustice();
						break;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Schedule selection 
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectSchedule( void )
{
	if ( !GetEnemy() && HasCondition( COND_IN_PVS ) && AI_GetSinglePlayer() && !AI_GetSinglePlayer()->IsAlive() )
	{
		return SCHED_PATROL_WALK;
	}

	if ( HasCondition(COND_METROPOLICE_ON_FIRE) )
	{
		m_Sentences.Speak( "METROPOLICE_ON_FIRE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		return SCHED_METROPOLICE_BURNING_STAND;
	}

	// React to being struck by a physics object
	if ( HasCondition( COND_METROPOLICE_PHYSOBJECT_ASSAULT ) )
	{
		ClearCondition( COND_METROPOLICE_PHYSOBJECT_ASSAULT );

		// See which state our player relationship is in
		if ( PlayerIsCriminal() == false )
		{
			m_Sentences.Speak( "METROPOLICE_HIT_BY_PHYSOBJECT", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
			m_nNumWarnings = METROPOLICE_MAX_WARNINGS;
			AdministerJustice();
		}
		else if ( GlobalEntity_GetState( "gordon_precriminal" ) == GLOBAL_ON )
		{
			// We're not allowed to respond, but warn them
			m_Sentences.Speak( "METROPOLICE_IDLE_HARASS_PLAYER", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		}
	}

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasBaton() )
	{
		// See if we're being told to activate our baton
		if ( m_bShouldActivateBaton && BatonActive() == false && IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) == false )
			return SCHED_METROPOLICE_ACTIVATE_BATON;

		if ( m_bShouldActivateBaton == false && BatonActive() && IsCurSchedule( SCHED_METROPOLICE_DEACTIVATE_BATON ) == false )
			return SCHED_METROPOLICE_DEACTIVATE_BATON;

		if( metropolice_chase_use_follow.GetBool() )
		{
			if( GetEnemy() )
			{
				AI_FollowParams_t params;
				params.formation = AIF_TIGHT;
				m_FollowBehavior.SetParameters( params );
				m_FollowBehavior.SetFollowTarget( GetEnemy() );
			}
		}
	}

	// See if the player is in our face (unless we're scripting)
	if ( PlayerIsCriminal() == false )
	{
		if ( !IsInAScript() && (HasCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE ) || m_bPlayerTooClose) )
		{
			// Don't hit the player too many times in a row, unless he's trying to push a cop who hasn't moved
			if ( m_iNumPlayerHits < 3 || m_vecPreChaseOrigin == vec3_origin )
			{
				ClearCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
				m_bPlayerTooClose = false;
				
				return SelectShoveSchedule();
			}
		}
		else if ( m_iNumPlayerHits )
		{
			// If we're not in combat, and we've got a pre-chase origin, move back to it
			if ( ( m_NPCState != NPC_STATE_COMBAT ) && 
				 ( m_vecPreChaseOrigin != vec3_origin ) && 
				 ( m_flChasePlayerTime < gpGlobals->curtime ) )
			{
				return SCHED_METROPOLICE_RETURN_TO_PRECHASE;
			}
		}
	}

	// Cower when physics objects are thrown at me
	if ( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		if ( m_flLastPhysicsFlinchTime + 4.0f <= gpGlobals->curtime )
		{
			m_flLastPhysicsFlinchTime = gpGlobals->curtime;
			return SCHED_FLINCH_PHYSICS;
		}
	}

	// Always run for cover from danger sounds
	if ( HasCondition(COND_HEAR_DANGER) )
	{
		CSound *pSound;
		pSound = GetBestSound();

		Assert( pSound != NULL );
		if ( pSound )
		{
			if (pSound->m_iType & SOUND_DANGER)
			{
				AnnounceTakeCoverFromDanger( pSound );
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}
			if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_COMBAT) ))
			{
				GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
			}
		}
	}

	bool bHighHealth = ((float)GetHealth() / (float)GetMaxHealth() > 0.75f);

	// This will cause the cops to run backwards + shoot at the same time
	if ( !bHighHealth && !HasBaton() )
	{
		if ( GetActiveWeapon() && (GetActiveWeapon()->m_iClip1 <= 5) )
		{
			m_Sentences.Speak( "METROPOLICE_COVER_LOW_AMMO" );
			return SCHED_HIDE_AND_RELOAD;
		}
	}

	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		if ( bHighHealth )
			return SCHED_RELOAD;

		AnnounceOutOfAmmo( );
		return SCHED_HIDE_AND_RELOAD;
	}

	// If we're clubbing someone who threw something at us. chase them
	if ( m_NPCState == NPC_STATE_COMBAT && m_flChasePlayerTime > gpGlobals->curtime )
		return SCHED_CHASE_ENEMY;

	if ( !BehaviorSelectSchedule() )
	{
		// If we've warned the player at all, watch him like a hawk
		if ( m_bKeepFacingPlayer && !PlayerIsCriminal() )
			return SCHED_TARGET_FACE;

		switch( m_NPCState )
		{
		case NPC_STATE_IDLE:
			{
				nSched = SelectScheduleInvestigateSound();
				if ( nSched != SCHED_NONE )
					return nSched;
				break;
			}

		case NPC_STATE_ALERT:
			{
				nSched = SelectScheduleInvestigateSound();
				if ( nSched != SCHED_NONE )
					return nSched;
			}
			break;

		case NPC_STATE_COMBAT:
			if (!IsEnemyInAnAirboat() || !Weapon_OwnsThisType( "weapon_smg1" ) )
			{
				int nResult = SelectCombatSchedule();
				if ( nResult != SCHED_NONE )
					return nResult;
			}
			else
			{
				int nResult = SelectAirboatCombatSchedule();
				if ( nResult != SCHED_NONE )
					return nResult;
			}
			break;
		}
	}

	// If we're not in combat, and we've got a pre-chase origin, move back to it
	if ( ( m_NPCState != NPC_STATE_COMBAT ) && 
		 ( m_vecPreChaseOrigin != vec3_origin ) && 
		 ( m_flChasePlayerTime < gpGlobals->curtime ) )
	{
		return SCHED_METROPOLICE_RETURN_TO_PRECHASE;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : failedSchedule - 
//			failedTask - 
//			taskFailCode - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule == SCHED_METROPOLICE_CHASE_ENEMY )
	{
		return SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ALERT_FACE_BESTSOUND:
		if ( !IsCurSchedule( SCHED_METROPOLICE_ALERT_FACE_BESTSOUND, false ) )
		{
			return SCHED_METROPOLICE_ALERT_FACE_BESTSOUND;
		}
		return SCHED_ALERT_FACE_BESTSOUND;

	case SCHED_CHASE_ENEMY:
		
		if ( !IsRunningBehavior() )
		{
			return SCHED_METROPOLICE_CHASE_ENEMY;
		}
		
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE:
		if ( IsEnemyInAnAirboat() )
		{
			int nSched = SelectMoveToLedgeSchedule();
			if ( nSched != SCHED_NONE )
				return nSched;
		}
		return SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE;		
		
	case SCHED_WAKE_ANGRY:
		return SCHED_METROPOLICE_WAKE_ANGRY;

	case SCHED_FAIL_TAKE_COVER:
		
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Must be able to shoot now
			if( TryToEnterPistolSlot( SQUAD_SLOT_ATTACK1 ) || TryToEnterPistolSlot( SQUAD_SLOT_ATTACK2 ) )
				return SCHED_RANGE_ATTACK1;
		}

		if ( HasCondition( COND_NO_PRIMARY_AMMO ) )
			return SCHED_RELOAD;
		return SCHED_RUN_RANDOM;

	case SCHED_RANGE_ATTACK1:
		Assert( !HasCondition( COND_NO_PRIMARY_AMMO ) );

		if( !m_fWeaponDrawn )
		{
			return SCHED_METROPOLICE_DRAW_PISTOL;
		}

		if( Weapon_OwnsThisType( "weapon_smg1" ) )
		{
			if ( IsEnemyInAnAirboat() )
			{
				int nSched = SelectStitchSchedule();
				if ( nSched != SCHED_NONE )
					return nSched;
			}

			if ( ShouldAttemptToStitch() )
			{
				return SCHED_METROPOLICE_SMG_BURST_ATTACK;
			}
			else
			{
				return SCHED_METROPOLICE_SMG_NORMAL_ATTACK;
			}
		}
		break;
	case SCHED_METROPOLICE_ADVANCE:
		if ( m_NextChargeTimer.Expired() && metropolice_charge.GetBool() )
		{	
			if ( Weapon_OwnsThisType( "weapon_pistol" ) )
			{
				if (  GetEnemy() && GetEnemy()->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) > 300*300 )
				{
					if ( OccupyStrategySlot( SQUAD_SLOT_POLICE_CHARGE_ENEMY ) )
					{
						m_NextChargeTimer.Set( 3, 7 );
						return SCHED_METROPOLICE_CHARGE;
					}
				}
			}
			else
			{
				m_NextChargeTimer.Set( 99999 );
			}
		}
		break;
	}


	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Can't move and shoot when the enemy is an airboat
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::ShouldMoveAndShoot()
{
	if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
		return false;

	if ( ShouldAttemptToStitch() )
		return false;

	return BaseClass::ShouldMoveAndShoot();
}


//-----------------------------------------------------------------------------
// Only move and shoot when attacking
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true; // already have the slot I need

		if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Only move and shoot when attacking
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_METROPOLICE_WAIT_FOR_SENTENCE:
		{
			if ( FOkToMakeSound( pTask->flTaskData ) )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_METROPOLICE_GET_PATH_TO_PRECHASE:
		{
			Assert( m_vecPreChaseOrigin != vec3_origin );
			if ( GetNavigator()->SetGoal( m_vecPreChaseOrigin ) )
			{
				QAngle vecAngles( 0, m_flPreChaseYaw, 0 );
				GetNavigator()->SetArrivalDirection( vecAngles );
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
			break;
		}

	case TASK_METROPOLICE_CLEAR_PRECHASE:
		{
			m_vecPreChaseOrigin = vec3_origin;
			m_flPreChaseYaw = 0;
			TaskComplete();
			break;
		}

	case TASK_METROPOLICE_ACTIVATE_BATON:
		{
			// Simply early out if we're in here without a baton
			if ( HasBaton() == false )
			{
				TaskComplete();
				break;
			}

			bool activate = ( pTask->flTaskData != 0 );

			if ( activate )
			{
				if ( BatonActive() || m_bShouldActivateBaton == false )
				{
					TaskComplete();
					break;
				}

				m_Sentences.Speak( "METROPOLICE_ACTIVATE_BATON", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
				SetIdealActivity( (Activity) ACT_ACTIVATE_BATON );
			}
			else
			{
				if ( BatonActive() == false || m_bShouldActivateBaton )
				{
					TaskComplete();
					break;
				}

				m_Sentences.Speak( "METROPOLICE_DEACTIVATE_BATON", SENTENCE_PRIORITY_NORMAL, SENTENCE_CRITERIA_NORMAL );
				SetIdealActivity( (Activity) ACT_DEACTIVATE_BATON );
			}
		}
		break;

	case TASK_METROPOLICE_DIE_INSTANTLY:
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

	case TASK_METROPOLICE_RESET_LEDGE_CHECK_TIME:
		m_flNextLedgeCheckTime = gpGlobals->curtime;
		TaskComplete();
		break;

	case TASK_METROPOLICE_LEAD_ARREST_ENEMY:
	case TASK_METROPOLICE_ARREST_ENEMY:
		m_flTaskCompletionTime = gpGlobals->curtime + pTask->flTaskData;
		break;

	case TASK_METROPOLICE_SIGNAL_FIRING_TIME:
		EnemyResistingArrest();
		TaskComplete();
		break;

	case TASK_METROPOLICE_GET_PATH_TO_STITCH:
		{
			if ( !ShouldAttemptToStitch() )
			{
				TaskFail( FAIL_NO_ROUTE );
				break;
			}

			Vector vecTarget, vecTargetVel;
			PredictShootTargetPosition( 0.5f, 0.0f, 0.0f, &vecTarget, &vecTargetVel );

			vecTarget -= GetAbsOrigin();
			vecTarget.z = 0.0f;
			float flDist = VectorNormalize( vecTarget );
			if ( GetNavigator()->SetVectorGoal( vecTarget, flDist ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	// Stitching aiming
	case TASK_METROPOLICE_AIM_STITCH_TIGHTLY:
		SetBurstMode( true );
		AimBurstTightGrouping( pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_METROPOLICE_AIM_STITCH_AT_PLAYER:
		SetBurstMode( true );
		AimBurstAtEnemy( pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_METROPOLICE_AIM_STITCH_AT_AIRBOAT:
		if ( IsEnemyInAnAirboat() )
		{
			SetBurstMode( true );
			AimBurstAtEnemy( pTask->flTaskData );
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;

	case TASK_METROPOLICE_AIM_STITCH_IN_FRONT_OF_AIRBOAT:
		if ( IsEnemyInAnAirboat() )
		{
			SetBurstMode( true );
			AimBurstInFrontOfEnemy( pTask->flTaskData );
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;

	case TASK_METROPOLICE_AIM_STITCH_ALONG_SIDE_OF_AIRBOAT:
		if ( IsEnemyInAnAirboat() )
		{
			SetBurstMode( true );
			AimBurstAlongSideOfEnemy( pTask->flTaskData );
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;

	case TASK_METROPOLICE_AIM_STITCH_BEHIND_AIRBOAT:
		if ( IsEnemyInAnAirboat() )
		{
			SetBurstMode( true );
			AimBurstBehindEnemy( pTask->flTaskData );
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;

	case TASK_METROPOLICE_BURST_ATTACK:
		ResetIdealActivity( ACT_RANGE_ATTACK1 );
		break;

	case TASK_METROPOLICE_STOP_FIRE_BURST:
		{
			SetBurstMode( false );
			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_HARASS:
		{
			if( !( m_spawnflags & SF_METROPOLICE_NOCHATTER ) )
			{
				if( GetEnemy() && GetEnemy()->GetWaterLevel() > 0 )
				{
					EmitSound( "NPC_MetroPolice.WaterSpeech" );
				}
				else
				{
					EmitSound( "NPC_MetroPolice.HidingSpeech" );
				}
			}

			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_RELOAD_FOR_BURST:
		{
			if (GetActiveWeapon())
			{
				int nDesiredShotCount = CountShotsInTime( pTask->flTaskData );

				// Do our fake reload to simulate a bigger clip without having to change the SMG1
				int nAddCount = nDesiredShotCount - GetActiveWeapon()->Clip1();
				if ( nAddCount > 0 )
				{
					if ( m_nBurstReloadCount >= nAddCount )
					{
						GetActiveWeapon()->m_iClip1 += nAddCount;
						m_nBurstReloadCount -= nAddCount;
					}
				}

				if ( nDesiredShotCount <= GetActiveWeapon()->Clip1() )
				{
					TaskComplete();
					break;
				}
			}

			// Fake a TASK_RELOAD to make sure we've got a full clip...
			Task_t reloadTask;
			reloadTask.iTask = TASK_RELOAD;
			reloadTask.flTaskData = 0.0f;
			StartTask( &reloadTask );
		}
		break;

	case TASK_RELOAD:
		m_nBurstReloadCount = METROPOLICE_BURST_RELOAD_COUNT;
		BaseClass::StartTask( pTask );
		break;

	case TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS:
		{
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
//
// Run tasks!
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// He's resisting arrest!
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::EnemyResistingArrest()
{
	// Prevent any other arrest from being made in this squad
	// and tell them all that the player is resisting arrest!
	
	if ( m_pSquad != NULL )
	{
		AISquadIter_t iter;
		CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
		while ( pSquadmate )
		{
			pSquadmate->RemoveSpawnFlags( SF_METROPOLICE_ARREST_ENEMY );
			pSquadmate->SetCondition( COND_METROPOLICE_ENEMY_RESISTING_ARREST );
			pSquadmate = m_pSquad->GetNextMember( &iter );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
#define FLEEING_DISTANCE_SQR (100 * 100)

void CNPC_MetroPolice::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		BaseClass::RunTask( pTask );
		break;

	case TASK_METROPOLICE_WAIT_FOR_SENTENCE:
		{
			if ( FOkToMakeSound( pTask->flTaskData ) )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_METROPOLICE_ACTIVATE_BATON:
		AutoMovement();
		
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}	
		break;

	case TASK_METROPOLICE_BURST_ATTACK:
		{
			AutoMovement( );

			Vector vecAimPoint;
			GetMotor()->SetIdealYawToTargetAndUpdate( m_vecBurstTargetPos, AI_KEEP_YAW_SPEED );

			if ( IsActivityFinished() )
			{
				if ( GetShotRegulator()->IsInRestInterval() )
				{
					TaskComplete();
				}
				else
				{
					OnRangeAttack1();
					ResetIdealActivity( ACT_RANGE_ATTACK1 );
				}
			}
		}
		break;

	case TASK_METROPOLICE_RELOAD_FOR_BURST:
		{
			// Fake a TASK_RELOAD
			Task_t reloadTask;
			reloadTask.iTask = TASK_RELOAD;
			reloadTask.flTaskData = 0.0f;
			RunTask( &reloadTask );
		}
		break;

	case TASK_METROPOLICE_LEAD_ARREST_ENEMY:
	case TASK_METROPOLICE_ARREST_ENEMY:
		{
			if ( !GetEnemy() )
			{
				TaskComplete();
				break;
			}

			if ( gpGlobals->curtime >= m_flTaskCompletionTime )
			{
				TaskComplete();
				break;
			}

			// Complete the arrest after the last squad member has a bead on the enemy
			// But only if you're the first guy who saw him
			if ( pTask->iTask == TASK_METROPOLICE_LEAD_ARREST_ENEMY )
			{
				int nArrestCount = SquadArrestCount();
				if ( nArrestCount == m_pSquad->NumMembers() )
				{
					TaskComplete();
					break;
				}

				// Do a distance check of the enemy from his initial position.
				// Shoot if he gets too far.
				if ( m_vSavePosition.DistToSqr( GetEnemy()->GetAbsOrigin() ) > FLEEING_DISTANCE_SQR )
				{
					SpeakSentence( METROPOLICE_SENTENCE_HES_RUNNING );
					EnemyResistingArrest();
					break;
				}
			}

			// Keep aiming at the enemy
			if ( GetEnemy() && FacingIdeal() )
			{
				float flNewIdealYaw = CalcIdealYaw( GetEnemy()->EyePosition() );
				if ( fabs(UTIL_AngleDiff( GetMotor()->GetIdealYaw(), flNewIdealYaw )) >= 45.0f )
				{
					GetMotor()->SetIdealYawToTarget( GetEnemy()->EyePosition() );
					SetTurnActivity(); 
				}
			}
			GetMotor()->UpdateYaw();
		}
		break;

	case TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS:
		{
			switch( GetTaskInterrupt() )
			{
			case 0:
				{
					CSound *pSound = GetBestSound();
					if (!pSound)
					{
						TaskFail(FAIL_NO_SOUND);
					}
					else
					{
						float flMaxRange = 2000;
						float flMinRange = 0;
						if ( GetActiveWeapon() )
						{
							flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
							flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
						}

						// Check against NPC's max range
						if (flMaxRange > m_flDistTooFar)
						{
							flMaxRange = m_flDistTooFar;
						}

						// Why not doing lateral LOS first?

						Vector losTarget = pSound->GetSoundReactOrigin();
						if ( GetTacticalServices()->FindLos( pSound->GetSoundReactOrigin(), losTarget, flMinRange, flMaxRange, 1.0, &m_vInterruptSavePosition ) )
						{
							TaskInterrupt();
						}
						else
						{
							TaskFail(FAIL_NO_SHOOT);
						}
					}
				}
				break;

			case 1:
				{
					AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );
					GetNavigator()->SetGoal( goal );
				}
				break;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


		
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pevInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	if ( HasSpawnFlags( SF_METROPOLICE_ARREST_ENEMY ) )
	{
		EnemyResistingArrest();
	}

#if 0
	// Die instantly from a hit in idle/alert states
	if( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT )
	{
		info.SetDamage( m_iHealth );
	}
#endif //0

	if (info.GetAttacker() == GetEnemy())
	{
		// Keep track of recent damage by my attacker. If it seems like we're
		// being killed, consider running off and hiding.
		m_nRecentDamage += info.GetDamage();
		m_flRecentDamageTime = gpGlobals->curtime;
	}

	return BaseClass::OnTakeDamage_Alive( info ); 
}


//-----------------------------------------------------------------------------
// Purpose: I want to deploy a manhack. Can I?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanDeployManhack( void )
{
	if ( HasSpawnFlags( SF_METROPOLICE_NO_MANHACK_DEPLOY ) )
		return false;

	// Nope, already have one out.
	if( m_hManhack != NULL )
		return false;

	// Nope, don't have any!
	if( m_iManhacks < 1 )
		return false;

	return true;
}
 
//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( PlayerIsCriminal() == false )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_PHYSOBJECT_ASSAULT );
	}

	//FIXME: Always interrupt for now
	if ( !IsInAScript() && 
		 !IsCurSchedule( SCHED_METROPOLICE_SHOVE ) &&
		 !IsCurSchedule( SCHED_MELEE_ATTACK1 ) &&
		 !IsCurSchedule( SCHED_RELOAD ) && 
		 !IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
	}

	if ( !IsCurSchedule( SCHED_METROPOLICE_BURNING_RUN ) && !IsCurSchedule( SCHED_METROPOLICE_BURNING_STAND ) && !IsMoving() )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_ON_FIRE );
	}

	if (IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY))
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	if ( !IsCurSchedule( SCHED_CHASE_ENEMY ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_ACTIVATE_BATON ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_DEACTIVATE_BATON ) &&
		 !IsCurSchedule( SCHED_METROPOLICE_SHOVE ) && 
		 !IsCurSchedule( SCHED_METROPOLICE_RETURN_TO_PRECHASE ) )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_CHANGE_BATON_STATE );
	}

	if ( IsCurSchedule( SCHED_MELEE_ATTACK1 ) )
	{
		if ( gpGlobals->curtime - m_flLastDamageFlinchTime < 10.0 )
		{
			ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
			ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		}
	}
	else if ( HasBaton() && IsCurSchedule( SCHED_COMBAT_FACE ) && !m_BatonSwingTimer.Expired() )
	{
		ClearCustomInterruptCondition( COND_CAN_MELEE_ATTACK1 );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WeaponProficiency_t CNPC_MetroPolice::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_pistol" ) )
	{
		return WEAPON_PROFICIENCY_POOR;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( m_bPlayerTooClose == false )
	{
		ClearCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
	}

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	
	// FIXME: Player can be NULL here during level transitions.
	if ( !pPlayer )
		return;

	float distToPlayerSqr = ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
	
	// See if we're too close
	if ( pPlayer->GetGroundEntity() == this )
	{
		// Always beat a player on our head
		m_iNumPlayerHits = 0;
		SetCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
	}
	else if ( (distToPlayerSqr < (42.0f*42.0f) && FVisible(pPlayer)) )
	{
		// Ignore the player if we've been beating him, but not if we haven't moved
		if ( m_iNumPlayerHits < 3 || m_vecPreChaseOrigin == vec3_origin )
		{
			SetCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
		}
	}
	else
	{
		ClearCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );

		// Don't clear out the player hit count for a few seconds after we last hit him
		// This avoids states where two metropolice have the player pinned between them.
		if ( (gpGlobals->curtime - GetLastAttackTime()) > 3 )
		{
			m_iNumPlayerHits = 0;
		}

		m_bPlayerTooClose = false;
	}

	if( metropolice_move_and_melee.GetBool() )
	{
		if( IsMoving() && HasCondition(COND_CAN_MELEE_ATTACK1) && HasBaton() )
		{
			if ( m_BatonSwingTimer.Expired() )
			{
				m_BatonSwingTimer.Set( 1.0, 1.75 );

				Activity activity = TranslateActivity( ACT_MELEE_ATTACK_SWING_GESTURE );
				Assert( activity != ACT_INVALID );
				AddGesture( activity );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::HasBaton( void )
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon )
		return FClassnameIs( pWeapon, "weapon_stunstick" );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::BatonActive( void )
{
#ifndef HL2MP

	CWeaponStunStick *pStick = dynamic_cast<CWeaponStunStick *>(GetActiveWeapon());

	if ( pStick )
		return pStick->GetStunState();
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SetBatonState( bool state )
{
	if ( !HasBaton() )
		return;

	if ( m_bShouldActivateBaton != state )
	{
		m_bShouldActivateBaton = state;
		SetCondition( COND_METROPOLICE_CHANGE_BATON_STATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSound - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::QueryHearSound( CSound *pSound )
{
	// Only behave differently if the player is pre-criminal
	if ( PlayerIsCriminal() == false )
	{
		// If the person making the sound was a friend, don't respond
		if ( pSound->IsSoundType( SOUND_DANGER ) && pSound->m_hOwner && IRelationType( pSound->m_hOwner ) == D_NU )
			return false;
	}

	return BaseClass::QueryHearSound( pSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>(UTIL_PlayerByIndex( 1 ));

		// See if it's being held by the player
		if ( pPlayer != NULL && pPlayer->IsHoldingEntity( pHitEntity ) )
		{
			//TODO: Play an angry sentence, "Get that outta here!"

			if ( IsCurSchedule( SCHED_METROPOLICE_SHOVE ) == false )
			{
				SetCondition( COND_METROPOLICE_PLAYER_TOO_CLOSE );
				m_bPlayerTooClose = true;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::StunnedTarget( CBaseEntity *pTarget )
{
	SetLastAttackTime( gpGlobals->curtime );

	if ( pTarget && pTarget->IsPlayer() )
	{
		m_OnStunnedPlayer.FireOutput( this, this );
		m_iNumPlayerHits++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use response for when the player is pre-criminal
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PrecriminalUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsInAScript() )
		return;
	// Don't respond if I'm busy hating the player
	if ( IRelationType( pActivator ) == D_HT || ((GetState() != NPC_STATE_ALERT) && (GetState() != NPC_STATE_IDLE)) )
		return;
	if ( PlayerIsCriminal() )
		return;

	// Treat it like the player's bothered the cop
	IncrementPlayerCriminalStatus();

	// If we've hit max warnings, and we're allowed to chase, go for it
	if ( m_nNumWarnings == METROPOLICE_MAX_WARNINGS )
	{
		AdministerJustice();
	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_metropolice, CNPC_MetroPolice )

	gm_flTimeLastSpokePeek = 0;

	DECLARE_ANIMEVENT( AE_METROPOLICE_BATON_ON );
	DECLARE_ANIMEVENT( AE_METROPOLICE_BATON_OFF );
	DECLARE_ANIMEVENT( AE_METROPOLICE_SHOVE );
	DECLARE_ANIMEVENT( AE_METROPOLICE_START_DEPLOY );
	DECLARE_ANIMEVENT( AE_METROPOLICE_DRAW_PISTOL );
	DECLARE_ANIMEVENT( AE_METROPOLICE_DEPLOY_MANHACK );

	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_CHARGE_ENEMY );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_HARASS );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_DEPLOY_MANHACK );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER1 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_OCCLUDER2 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ARREST_ENEMY );

	DECLARE_ACTIVITY( ACT_METROPOLICE_DRAW_PISTOL );
	DECLARE_ACTIVITY( ACT_METROPOLICE_DEPLOY_MANHACK );
	DECLARE_ACTIVITY( ACT_METROPOLICE_FLINCH_BEHIND );
	DECLARE_ACTIVITY( ACT_PUSH_PLAYER );
	DECLARE_ACTIVITY( ACT_MELEE_ATTACK_THRUST );
	DECLARE_ACTIVITY( ACT_ACTIVATE_BATON );
	DECLARE_ACTIVITY( ACT_DEACTIVATE_BATON );
	DECLARE_ACTIVITY( ACT_WALK_BATON );
	DECLARE_ACTIVITY( ACT_IDLE_ANGRY_BATON );

	DECLARE_INTERACTION( g_interactionMetrocopStartedStitch );	
	DECLARE_INTERACTION( g_interactionMetrocopIdleChatter );	
	DECLARE_INTERACTION( g_interactionMetrocopClearSentenceQueues );

	DECLARE_TASK( TASK_METROPOLICE_HARASS );
	DECLARE_TASK( TASK_METROPOLICE_DIE_INSTANTLY );
	DECLARE_TASK( TASK_METROPOLICE_BURST_ATTACK );
	DECLARE_TASK( TASK_METROPOLICE_STOP_FIRE_BURST );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_AT_PLAYER );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_AT_AIRBOAT );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_IN_FRONT_OF_AIRBOAT );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_TIGHTLY );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_ALONG_SIDE_OF_AIRBOAT );
	DECLARE_TASK( TASK_METROPOLICE_AIM_STITCH_BEHIND_AIRBOAT );
	DECLARE_TASK( TASK_METROPOLICE_RELOAD_FOR_BURST );
	DECLARE_TASK( TASK_METROPOLICE_GET_PATH_TO_STITCH );
	DECLARE_TASK( TASK_METROPOLICE_RESET_LEDGE_CHECK_TIME );
	DECLARE_TASK( TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS );
	DECLARE_TASK( TASK_METROPOLICE_ARREST_ENEMY );
	DECLARE_TASK( TASK_METROPOLICE_LEAD_ARREST_ENEMY );
	DECLARE_TASK( TASK_METROPOLICE_SIGNAL_FIRING_TIME );
	DECLARE_TASK( TASK_METROPOLICE_ACTIVATE_BATON );
	DECLARE_TASK( TASK_METROPOLICE_WAIT_FOR_SENTENCE );
	DECLARE_TASK( TASK_METROPOLICE_GET_PATH_TO_PRECHASE );
	DECLARE_TASK( TASK_METROPOLICE_CLEAR_PRECHASE );

	DECLARE_CONDITION( COND_METROPOLICE_ON_FIRE );
	DECLARE_CONDITION( COND_METROPOLICE_ENEMY_RESISTING_ARREST );
//	DECLARE_CONDITION( COND_METROPOLICE_START_POLICING );
	DECLARE_CONDITION( COND_METROPOLICE_PLAYER_TOO_CLOSE );
	DECLARE_CONDITION( COND_METROPOLICE_CHANGE_BATON_STATE );
	DECLARE_CONDITION( COND_METROPOLICE_PHYSOBJECT_ASSAULT );


	//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_WAKE_ANGRY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
);


//=========================================================
// > InvestigateSound
//
//	sends a monster to the location of the
//	sound that was just heard to check things out.
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_INVESTIGATE_SOUND,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_METROPOLICE_GET_PATH_TO_BESTSOUND_LOS		0"
	"		TASK_FACE_IDEAL					0"
//	"		TASK_SET_TOLERANCE_DISTANCE		32"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						5"
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_CLEAR_LASTPOSITION			0"
	"		TASK_FACE_REASONABLE			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
);


//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HARASS,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_FACE_ENEMY			6"
	"		TASK_METROPOLICE_HARASS			0"
	"		TASK_WAIT_PVS					0"
	"	"
	"	Interrupts"
	"	"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_NEW_ENEMY"
);


//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DRAW_PISTOL,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_METROPOLICE_DRAW_PISTOL"
	"		TASK_WAIT_FACE_ENEMY			0.1"
	"	"
	"	Interrupts"
	"	"
);


//=========================================================
// > ChaseEnemy
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_CHASE_ENEMY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
	"		TASK_SPEAK_SENTENCE				6"	// METROPOLICE_SENTENCE_MOVE_INTO_POSITION
	"		TASK_RUN_PATH					0"
	"		TASK_METROPOLICE_RESET_LEDGE_CHECK_TIME 0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_BETTER_WEAPON_AVAILABLE"
	"		COND_HEAR_DANGER"
);


DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ESTABLISH_LINE_OF_FIRE,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
	"		TASK_FACE_ENEMY					0"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
	"		TASK_SPEAK_SENTENCE				6"	// METROPOLICE_SENTENCE_MOVE_INTO_POSITION
	"		TASK_RUN_PATH					0"
	"		TASK_METROPOLICE_RESET_LEDGE_CHECK_TIME 0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"	"
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAVY_DAMAGE"
);


DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ESTABLISH_STITCH_LINE_OF_FIRE,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
	"		TASK_FACE_ENEMY					0"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_METROPOLICE_GET_PATH_TO_STITCH	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"	"
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAR_DANGER"
	"		COND_HEAVY_DAMAGE"
);


//=========================================================
// The uninterruptible portion of this behavior, whereupon 
// the police actually releases the manhack.
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DEPLOY_MANHACK,

	"	Tasks"
	"		TASK_SPEAK_SENTENCE					5"	// METROPOLICE_SENTENCE_DEPLOY_MANHACK
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_METROPOLICE_DEPLOY_MANHACK"
	"	"
	"	Interrupts"
	"	"
);


//===============================================
//===============================================

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ADVANCE,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_FACE_ENEMY						0"
	"		TASK_WAIT_FACE_ENEMY				1" // give the guy some time to come out on his own
	"		TASK_WAIT_FACE_ENEMY_RANDOM			3" 
	"		TASK_GET_PATH_TO_ENEMY_LOS			0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_FACE_ENEMY						0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_ENEMY_DEAD"
	""
);

//===============================================
//===============================================

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_CHARGE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_ADVANCE"
//	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
	"		TASK_RUN_PATH_FOR_UNITS			150"
	"		TASK_STOP_MOVING				1"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LOST_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_METROPOLICE_PLAYER_TOO_CLOSE"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_RUN,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_BURNING_STAND"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH_TIMED				10"	
	"		TASK_METROPOLICE_DIE_INSTANTLY	0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_STAND,

	"	Tasks"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE_ON_FIRE"
	"		TASK_WAIT						1.5"
	"		TASK_METROPOLICE_DIE_INSTANTLY	DMG_BURN"
	"		TASK_WAIT						1.0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_RETURN_TO_PRECHASE,

	"	Tasks"
	"		TASK_WAIT_RANDOM						1"
	"		TASK_METROPOLICE_GET_PATH_TO_PRECHASE	0"
	"		TASK_WALK_PATH							0"
	"		TASK_WAIT_FOR_MOVEMENT					0"
	"		TASK_STOP_MOVING						0"
	"		TASK_METROPOLICE_CLEAR_PRECHASE			0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_HEAR_DANGER"
);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ALERT_FACE_BESTSOUND,

	"	Tasks"
	"		TASK_SPEAK_SENTENCE		7"	// METROPOLICE_SENTENCE_HEARD_SOMETHING
	"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ALERT_FACE_BESTSOUND"
	""
	"	Interrupts"
	""
)


//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ENEMY_RESISTING_ARREST,

	"	Tasks"
	"		TASK_METROPOLICE_SIGNAL_FIRING_TIME		0"
	""
	"	Interrupts"
	""
)


//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_WARN_AND_ARREST_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_METROPOLICE_ENEMY_RESISTING_ARREST"
	"		TASK_STOP_MOVING					0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_SPEAK_SENTENCE					0"	// "Freeze!"
	"		TASK_METROPOLICE_ARREST_ENEMY		0.5"
	"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
	"		TASK_METROPOLICE_ARREST_ENEMY		1"
	"		TASK_METROPOLICE_WAIT_FOR_SENTENCE	1"
	"		TASK_SPEAK_SENTENCE					1"	// "He's over here!"
	"		TASK_METROPOLICE_LEAD_ARREST_ENEMY	5"
	"		TASK_METROPOLICE_ARREST_ENEMY		2"
	"		TASK_METROPOLICE_WAIT_FOR_SENTENCE	1"
	"		TASK_SPEAK_SENTENCE					3"	// "Take him down!"
	"		TASK_METROPOLICE_ARREST_ENEMY		1.5"
	"		TASK_METROPOLICE_WAIT_FOR_SENTENCE	2"
	"		TASK_METROPOLICE_SIGNAL_FIRING_TIME	0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_ENEMY_DEAD"
	"		COND_METROPOLICE_ENEMY_RESISTING_ARREST"
	""
);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ARREST_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_METROPOLICE_ENEMY_RESISTING_ARREST"
	"		TASK_GET_PATH_TO_ENEMY_LOS			0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_STOP_MOVING					0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_METROPOLICE_WAIT_FOR_SENTENCE	0"
	"		TASK_SPEAK_SENTENCE					4"
	"		TASK_METROPOLICE_ARREST_ENEMY		30"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_ENEMY_DEAD"
	"		COND_METROPOLICE_ENEMY_RESISTING_ARREST"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"
	"		COND_WEAPON_SIGHT_OCCLUDED"
	""
);


//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SMG_NORMAL_ATTACK,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_STOP_FIRE_BURST	0"
	"		TASK_RANGE_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"
	"		COND_WEAPON_SIGHT_OCCLUDED"
);


//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SMG_BURST_ATTACK,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST	1.4"
	"		TASK_METROPOLICE_AIM_STITCH_AT_PLAYER	1.4"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_AIM_STITCH_TIGHTLY,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST	1.0"
	"		TASK_METROPOLICE_AIM_STITCH_TIGHTLY	1.0"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);


//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_AIM_STITCH_AT_AIRBOAT,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST		2.5"
	"		TASK_METROPOLICE_AIM_STITCH_AT_AIRBOAT	2.5"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_AIM_STITCH_IN_FRONT_OF_AIRBOAT,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST		2.5"
	"		TASK_METROPOLICE_AIM_STITCH_IN_FRONT_OF_AIRBOAT	2.5"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_AIM_STITCH_ALONG_SIDE_OF_AIRBOAT,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST		2.5"
	"		TASK_METROPOLICE_AIM_STITCH_ALONG_SIDE_OF_AIRBOAT	2.5"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_AIM_STITCH_BEHIND_AIRBOAT,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_METROPOLICE_RELOAD_FOR_BURST		2.5"
	"		TASK_METROPOLICE_AIM_STITCH_BEHIND_AIRBOAT	2.5"
	"		TASK_METROPOLICE_BURST_ATTACK		0"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"

);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SHOVE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_PLAYER				0.1"	//FIXME: This needs to be the target or enemy
	"		TASK_METROPOLICE_ACTIVATE_BATON	1"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_PUSH_PLAYER"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ACTIVATE_BATON,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_TARGET				0"
	"		TASK_METROPOLICE_ACTIVATE_BATON	1"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DEACTIVATE_BATON,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_METROPOLICE_ACTIVATE_BATON	0"
	""
	"	Interrupts"
);

DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_SMASH_PROP,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET		0"
	"		TASK_MOVE_TO_TARGET_RANGE	50"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_TARGET			0"
	"		TASK_ANNOUNCE_ATTACK		1"	// 1 = primary attack
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MELEE_ATTACK1"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
);

AI_END_CUSTOM_NPC()

