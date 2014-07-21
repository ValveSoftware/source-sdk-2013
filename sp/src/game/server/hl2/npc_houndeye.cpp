//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Houndeye - a spooky sonic dog.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npc_houndeye.h"
#include "ai_default.h"
#include "ai_node.h"
#include "ai_route.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "ai_squad.h"
#include "AI_TacticalServices.h"
#include "soundent.h"
#include "EntityList.h"
#include "game.h"
#include "activitylist.h"
#include "hl2_shareddefs.h"
#include "grenade_energy.h"
#include "energy_wave.h"
#include "ai_interactions.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "player.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	HOUNDEYE_MAX_ATTACK_RADIUS		500
#define	HOUNDEYE_MIN_ATTACK_RADIUS		100

#define HOUNDEYE_EYE_FRAMES 4 // how many different switchable maps for the eye

ConVar	sk_Houndeye_health( "sk_Houndeye_health","0");
ConVar	sk_Houndeye_dmg_blast( "sk_Houndeye_dmg_blast","0");

//=========================================================
// Interactions
//=========================================================
int	g_interactionHoundeyeGroupAttack					= 0;
int	g_interactionHoundeyeGroupRetreat					= 0;
int	g_interactionHoundeyeGroupRalley					= 0;

//=========================================================
// Specialized Tasks 
//=========================================================
enum 
{
	TASK_HOUND_CLOSE_EYE = LAST_SHARED_TASK,
	TASK_HOUND_OPEN_EYE,
	TASK_HOUND_THREAT_DISPLAY,
	TASK_HOUND_FALL_ASLEEP,
	TASK_HOUND_WAKE_UP,
	TASK_HOUND_HOP_BACK,

	TASK_HOUND_GET_PATH_TO_CIRCLE,
	TASK_HOUND_REVERSE_STRAFE_DIR,
};

//-----------------------------------------------------------------------------
// Custom Conditions
//-----------------------------------------------------------------------------
enum Houndeye_Conds
{
	COND_HOUND_GROUP_ATTACK = LAST_SHARED_CONDITION,
	COND_HOUND_GROUP_RETREAT,
	COND_HOUND_GROUP_RALLEY,
};

//=========================================================
// Specialized Shedules
//=========================================================
enum
{
	SCHED_HOUND_AGITATED = LAST_SHARED_SCHEDULE,
	SCHED_HOUND_HOP_RETREAT,
	SCHED_HOUND_RANGE_ATTACK1,

	SCHED_HOUND_ATTACK_STRAFE,
	SCHED_HOUND_ATTACK_STRAFE_REVERSE,
	SCHED_HOUND_GROUP_ATTACK,
	SCHED_HOUND_GROUP_RETREAT,
	SCHED_HOUND_CHASE_ENEMY,
	SCHED_HOUND_COVER_WAIT,
	SCHED_HOUND_GROUP_RALLEY,
};

//=========================================================
// Specialized activities
//=========================================================
int	ACT_HOUND_GUARD;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HOUND_AE_WARN			1
#define		HOUND_AE_STARTATTACK	2
#define		HOUND_AE_THUMP			3
#define		HOUND_AE_ANGERSOUND1	4
#define		HOUND_AE_ANGERSOUND2	5
#define		HOUND_AE_HOPBACK		6
#define		HOUND_AE_CLOSE_EYE		7
#define		HOUND_AE_LEAP_HIT		8

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Houndeye::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Houndeye);

	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_CLOSE_EYE);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_OPEN_EYE);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_THREAT_DISPLAY);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_FALL_ASLEEP);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_WAKE_UP);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_HOP_BACK);

	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_GET_PATH_TO_CIRCLE);
	ADD_CUSTOM_TASK(CNPC_Houndeye,	TASK_HOUND_REVERSE_STRAFE_DIR);
	
	ADD_CUSTOM_CONDITION(CNPC_Houndeye,	COND_HOUND_GROUP_ATTACK);
	ADD_CUSTOM_CONDITION(CNPC_Houndeye,	COND_HOUND_GROUP_RETREAT);

	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_HOP_RETREAT);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_RANGE_ATTACK1);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_ATTACK_STRAFE);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_ATTACK_STRAFE_REVERSE);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_ATTACK);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_RETREAT);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_CHASE_ENEMY);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_COVER_WAIT);
	ADD_CUSTOM_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_RALLEY);
	
	ADD_CUSTOM_ACTIVITY(CNPC_Houndeye,	ACT_HOUND_GUARD);

	g_interactionHoundeyeGroupAttack				= CBaseCombatCharacter::GetInteractionID();
	g_interactionHoundeyeGroupRetreat				= CBaseCombatCharacter::GetInteractionID();
	g_interactionHoundeyeGroupRalley				= CBaseCombatCharacter::GetInteractionID();

	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_HOP_RETREAT);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_RANGE_ATTACK1);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_ATTACK_STRAFE);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_ATTACK_STRAFE_REVERSE);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_ATTACK);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_RETREAT);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_CHASE_ENEMY);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_COVER_WAIT);
	AI_LOAD_SCHEDULE(CNPC_Houndeye,	SCHED_HOUND_GROUP_RALLEY);
}

LINK_ENTITY_TO_CLASS( npc_houndeye, CNPC_Houndeye );
IMPLEMENT_CUSTOM_AI( npc_houndeye, CNPC_Houndeye );

BEGIN_DATADESC( CNPC_Houndeye )

	DEFINE_FIELD( m_fAsleep,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fDontBlink,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextSecondaryAttack,	FIELD_TIME ),
	DEFINE_FIELD( m_bLoopClockwise,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_pEnergyWave,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flEndEnergyWaveTime,		FIELD_TIME ),	

END_DATADESC()

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Houndeye::Classify ( void )
{
	return	CLASS_HOUNDEYE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Houndeye::RangeAttack1Conditions ( float flDot, float flDist )
{
	// I'm not allowed to attack if standing in another hound eye 
	// (note houndeyes allowed to interpenetrate)
	trace_t tr;
	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,0.1), 
					GetHullMins(), GetHullMaxs(),
					MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	if (tr.startsolid)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity->Classify() == CLASS_HOUNDEYE)
		{
			return( COND_NONE );
		}
	}

	// If I'm really close to my enemy allow me to attack if 
	// I'm facing regardless of next attack time
	if (flDist < 100 && flDot >= 0.3)
	{
		return COND_CAN_RANGE_ATTACK1;
	}
	if ( gpGlobals->curtime < m_flNextAttack )
	{
		return( COND_NONE );
	}
	if (flDist > ( HOUNDEYE_MAX_ATTACK_RADIUS * 0.5 ))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	if (flDot < 0.3)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: Overidden for human grunts because they  hear the DANGER sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Houndeye::GetSoundInterests( void )
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_PLAYER	|
			SOUND_DANGER;
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Houndeye::MaxYawSpeed ( void )
{
	int ys = 90;

	switch ( GetActivity() )
	{
	case ACT_CROUCHIDLE://sleeping!
		ys = 0;
		break;
	case ACT_IDLE:	
		ys = 60;
		break;
	case ACT_WALK:
		ys = 90;
		break;
	case ACT_RUN:	
		ys = 90;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 90;
		break;
	}
	return ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Houndeye::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case HOUND_AE_WARN:
			// do stuff for this event.
			WarnSound();
			break;

		case HOUND_AE_STARTATTACK:
			WarmUpSound();
			break;

		case HOUND_AE_HOPBACK:
			{
				float flGravity = GetCurrentGravity();

				SetGroundEntity( NULL );

				Vector forward;
				AngleVectors( GetLocalAngles(), &forward );
				Vector vecNewVelocity = forward * -200;
				//jump up 36 inches
				vecNewVelocity.z += sqrt( 2 * flGravity * 36 );
				SetAbsVelocity( vecNewVelocity );
				break;
			}

		case HOUND_AE_THUMP:
			// emit the shockwaves
			SonicAttack();
			m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 5.0, 8.0 );
			break;

		case HOUND_AE_ANGERSOUND1:
			{
				EmitSound( "NPC_Houndeye.Anger1" );
			}
			break;

		case HOUND_AE_ANGERSOUND2:
			{
			EmitSound( "NPC_Houndeye.Anger2" );
			}
			break;

		case HOUND_AE_CLOSE_EYE:
			if ( !m_fDontBlink )
			{
			//<<TEMP>>	pev->skin = HOUNDEYE_EYE_FRAMES - 1;
			}
			break;

		case HOUND_AE_LEAP_HIT:
			{
				//<<TEMP>>return;//<<TEMP>>
				SetGroundEntity( NULL );

				//
				// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
				//
				UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));
				Vector vecJumpDir;
				if ( GetEnemy() != NULL )
				{
					Vector vecEnemyEyePos = GetEnemy()->EyePosition();

					float gravity = GetCurrentGravity();
					if ( gravity <= 1 )
					{
						gravity = 1;
					}

					//
					// How fast does the houndeye need to travel to reach my enemy's eyes given gravity?
					//
					float height = ( vecEnemyEyePos.z - GetAbsOrigin().z );
					if ( height < 16 )
					{
						height = 16;
					}
					else if ( height > 120 )
					{
						height = 120;
					}
					float speed = sqrt( 2 * gravity * height );
					float time = speed / gravity;

					//
					// Scale the sideways velocity to get there at the right time
					//
					vecJumpDir = vecEnemyEyePos - GetAbsOrigin();
					vecJumpDir = vecJumpDir / time;

					//
					// Speed to offset gravity at the desired height.
					//
					vecJumpDir.z = speed;

					//
					// Don't jump too far/fast.
					//
					float distance = vecJumpDir.Length();
					if ( distance > 650 )
					{
						vecJumpDir = vecJumpDir * ( 650.0 / distance );
					}
				}
				else
				{
					Vector forward, up;
					AngleVectors( GetLocalAngles(), &forward, NULL, &up );
					//
					// Jump hop, don't care where.
					//
					vecJumpDir = Vector( forward.x, forward.y, up.z ) * 350;
				}

				SetAbsVelocity( vecJumpDir );
				m_flNextAttack = gpGlobals->curtime + 2;
				break;
			}
		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Houndeye::Spawn()
{
	Precache( );

	SetModel("models/houndeye.mdl");
	SetHullType(HULL_WIDE_SHORT);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_iHealth			= sk_Houndeye_health.GetFloat();
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	m_fAsleep			= false; // everyone spawns awake
	m_fDontBlink		= false;
	CapabilitiesAdd( bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_TURN_HEAD );

	m_flNextSecondaryAttack = 0;
	m_bLoopClockwise		= random->RandomInt(0,1) ? true : false;

	m_pEnergyWave			= NULL;
	m_flEndEnergyWaveTime	= 0;

	SetCollisionGroup( HL2COLLISION_GROUP_HOUNDEYE ); 

	NPCInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Houndeye::Precache()
{
	PrecacheModel("models/houndeye.mdl");

	PrecacheScriptSound( "NPC_Houndeye.Anger1" );
	PrecacheScriptSound( "NPC_Houndeye.Anger2" );
	PrecacheScriptSound( "NPC_Houndeye.SpeakSentence" );
	PrecacheScriptSound( "NPC_Houndeye.Idle" );
	PrecacheScriptSound( "NPC_Houndeye.WarmUp" );
	PrecacheScriptSound( "NPC_Houndeye.Warn" );
	PrecacheScriptSound( "NPC_Houndeye.Alert" );
	PrecacheScriptSound( "NPC_Houndeye.Die" );
	PrecacheScriptSound( "NPC_Houndeye.Pain" );
	PrecacheScriptSound( "NPC_Houndeye.Retreat" );
	PrecacheScriptSound( "NPC_Houndeye.SonicAttack" );

	PrecacheScriptSound( "NPC_Houndeye.GroupAttack" );
	PrecacheScriptSound( "NPC_Houndeye.GroupFollow" );


	UTIL_PrecacheOther("grenade_energy");
	BaseClass::Precache();
}	

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Houndeye::SpeakSentence( int sentenceType )
{
	if (gpGlobals->curtime > m_flSoundWaitTime)
	{
		EmitSound( "NPC_Houndeye.SpeakSentence" );
		m_flSoundWaitTime = gpGlobals->curtime + 1.0;
	}
}

//=========================================================
// IdleSound
//=========================================================
void CNPC_Houndeye::IdleSound ( void )
{
	if (!FOkToMakeSound())
	{
		return;
	}
	CPASAttenuationFilter filter( this,"NPC_Houndeye.Idle" );
	EmitSound( filter, entindex(),"NPC_Houndeye.Idle" );
}

//=========================================================
// IdleSound
//=========================================================
void CNPC_Houndeye::WarmUpSound ( void )
{
	EmitSound( "NPC_Houndeye.WarmUp" );
}

//=========================================================
// WarnSound 
//=========================================================
void CNPC_Houndeye::WarnSound ( void )
{
	EmitSound( "NPC_Houndeye.Warn" );
}

//=========================================================
// AlertSound 
//=========================================================
void CNPC_Houndeye::AlertSound ( void )
{
	// only first squad member makes ALERT sound.
	if ( m_pSquad && !m_pSquad->IsLeader( this ) )
	{
		return; 
	}

	EmitSound( "NPC_Houndeye.Alert" );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Houndeye::DeathSound ( void )
{
	EmitSound( "NPC_Houndeye.Die" );
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_Houndeye::PainSound ( void )
{
	EmitSound( "NPC_Houndeye.Pain" );
}

//=========================================================
// WriteBeamColor - writes a color vector to the network 
// based on the size of the group. 
//=========================================================
void CNPC_Houndeye::WriteBeamColor ( void )
{
	BYTE	bRed, bGreen, bBlue;

	if ( m_pSquad )
	{
		switch ( m_pSquad->NumMembers() )
		{
		case 2:
			// no case for 0 or 1, cause those are impossible for monsters in Squads.
			bRed	= 101;
			bGreen	= 133;
			bBlue	= 221;
			break;
		case 3:
			bRed	= 67;
			bGreen	= 85;
			bBlue	= 255;
			break;
		case 4:
			bRed	= 62;
			bGreen	= 33;
			bBlue	= 211;
			break;
		default:
			DevWarning( 2, "Unsupported Houndeye SquadSize!\n" );
			bRed	= 188;
			bGreen	= 220;
			bBlue	= 255;
			break;
		}
	}
	else
	{
		// solo houndeye - weakest beam
		bRed	= 188;
		bGreen	= 220;
		bBlue	= 255;
	}
	
	WRITE_BYTE( bRed   );
	WRITE_BYTE( bGreen );
	WRITE_BYTE( bBlue  );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CNPC_Houndeye::NPCThink(void)
{
	if (m_pEnergyWave)
	{
		if (gpGlobals->curtime > m_flEndEnergyWaveTime)
		{
			UTIL_Remove(m_pEnergyWave);
			m_pEnergyWave = NULL;
		}
	}

	// -----------------------------------------------------
	//  Update collision group
	//		While I'm running I'm allowed to penetrate
	//		other houndeyes
	// -----------------------------------------------------
	Vector vVelocity;
	GetVelocity( &vVelocity, NULL );
	if (vVelocity.Length() > 10)
	{
		SetCollisionGroup( HL2COLLISION_GROUP_HOUNDEYE );
	}
	else 
	{
		// Don't go solid if resting in another houndeye 
		trace_t tr;
		AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,1), 
						GetHullMins(), GetHullMaxs(),
						MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
		if (!tr.startsolid)
		{
			SetCollisionGroup( COLLISION_GROUP_NONE );
		}
		else
		{
			SetCollisionGroup( HL2COLLISION_GROUP_HOUNDEYE );
		}
	}
/*
	if (GetCollisionGroup() == HL2COLLISION_GROUP_HOUNDEYE)
	{
		NDebugOverlay::Box(GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 0);
	}
	else
	{
		NDebugOverlay::Box(GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 255, 0, 0, 0, 0);
	}
*/
	BaseClass::NPCThink();
}

//------------------------------------------------------------------------------
// Purpose : Broadcast retreat occasionally when hurt
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Houndeye::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if (m_pSquad && random->RandomInt(0,10) == 10)
	{
		EmitSound( "NPC_Houndeye.Retreat" );
		m_flSoundWaitTime = gpGlobals->curtime + 1.0;

		m_pSquad->BroadcastInteraction( g_interactionHoundeyeGroupRetreat, NULL, this );
	}
	
	return BaseClass::OnTakeDamage_Alive( info );
}

//------------------------------------------------------------------------------
// Purpose : Broadcast retreat when member of squad killed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Houndeye::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Houndeye.Retreat" );
	m_flSoundWaitTime = gpGlobals->curtime + 1.0;

	if (m_pSquad)
	{
		m_pSquad->BroadcastInteraction( g_interactionHoundeyeGroupRetreat, NULL, this );
	}

	BaseClass::Event_Killed( info );
}

//=========================================================
// SonicAttack
//=========================================================
void CNPC_Houndeye::SonicAttack ( void )
{
	EmitSound( "NPC_Houndeye.SonicAttack" );

	if (m_pEnergyWave)
	{
		UTIL_Remove(m_pEnergyWave);
	}
	Vector vFacingDir = EyeDirection3D( );
	m_pEnergyWave = (CEnergyWave*)Create( "energy_wave", EyePosition(), GetLocalAngles() );
	m_flEndEnergyWaveTime = gpGlobals->curtime + 1; //<<TEMP>> magic
	m_pEnergyWave->SetAbsVelocity( 100*vFacingDir );

	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), HOUNDEYE_MAX_ATTACK_RADIUS ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if (pEntity->Classify()	== CLASS_HOUNDEYE)
		{
			continue;
		}

		if (pEntity->GetFlags() & FL_NOTARGET)
		{
			continue;
		}

		IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();

		if ( pEntity->m_takedamage != DAMAGE_NO || pPhysicsObject)
		{
			// --------------------------
			// Adjust damage by distance
			// --------------------------
			float flDist = (pEntity->WorldSpaceCenter() - GetAbsOrigin()).Length();
			float flDamageAdjuster = 1-( flDist / HOUNDEYE_MAX_ATTACK_RADIUS );

			// --------------------------
			// Adjust damage by direction
			// --------------------------
			Vector forward;
			AngleVectors( GetAbsAngles(), &forward );
			Vector vEntDir		= (pEntity->GetAbsOrigin() - GetAbsOrigin());
			VectorNormalize(vEntDir);
			float flDotPr		= DotProduct(forward,vEntDir);
			flDamageAdjuster   *= flDotPr;

			if (flDamageAdjuster < 0)
			{
				continue;
			}

			// --------------------------
			// Adjust damage by visibility
			// --------------------------
			if ( !FVisible( pEntity ) )
			{
				if ( pEntity->IsPlayer() )
				{
					// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
					// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
					// so that monsters in other parts of the level don't take the damage and get pissed.
					flDamageAdjuster *= 0.5;
				}
				else if ( !FClassnameIs( pEntity, "func_breakable" ) && !FClassnameIs( pEntity, "func_pushable" ) ) 
				{
					// do not hurt nonclients through walls, but allow damage to be done to breakables
					continue;
				}
			}

			// ------------------------------
			//  Apply the damage
			// ------------------------------
			if (pEntity->m_takedamage != DAMAGE_NO)
			{
				CTakeDamageInfo info( this, this, flDamageAdjuster * sk_Houndeye_dmg_blast.GetFloat(), DMG_SONIC | DMG_ALWAYSGIB );
				CalculateExplosiveDamageForce( &info, (pEntity->GetAbsOrigin() - GetAbsOrigin()), pEntity->GetAbsOrigin() );

				pEntity->TakeDamage( info );

				// Throw the player
				if ( pEntity->IsPlayer() )
				{
					Vector forward;
					AngleVectors( GetLocalAngles(), &forward );

					Vector vecVelocity = pEntity->GetAbsVelocity();
					vecVelocity	+= forward * 250 * flDamageAdjuster;
					vecVelocity.z = 300 * flDamageAdjuster;
					pEntity->SetAbsVelocity( vecVelocity );
					pEntity->ViewPunch( QAngle(random->RandomInt(-20,20), 0, random->RandomInt(-20,20)) );
				}
			}
			// ------------------------------
			//  Apply physics foces
			// ------------------------------
			IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
			if (pPhysicsObject)
			{
				float flForce	= flDamageAdjuster * 8000;
				pPhysicsObject->ApplyForceCenter( (vEntDir+Vector(0,0,0.2)) * flForce );
				pPhysicsObject->ApplyTorqueCenter( vEntDir * flForce );
			}
		}
	}
}
		
//=========================================================
// start task
//=========================================================
void CNPC_Houndeye::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HOUND_GET_PATH_TO_CIRCLE:
	{
		if (GetEnemy() == NULL)
		{
			TaskFail(FAIL_NO_ENEMY);
		}
		else 
		{
			Vector vTargetPos = GetEnemyLKP();
			vTargetPos.z	= GetFloorZ(vTargetPos);

			if (GetNavigator()->SetRadialGoal(vTargetPos, random->RandomInt(50,500), 90, 175, m_bLoopClockwise))
			{
				TaskComplete();
				return;
			}
			TaskFail(FAIL_NO_ROUTE);
		}
		break;
	}
	case TASK_HOUND_REVERSE_STRAFE_DIR:
	{
		// Try the other direction
		m_bLoopClockwise = (m_bLoopClockwise) ? false : true;
		TaskComplete();
		break;
	}

	// Override to set appropriate distances
	case TASK_GET_PATH_TO_ENEMY_LOS:
	{
		float			flMaxRange	= HOUNDEYE_MAX_ATTACK_RADIUS * 0.9;
		float			flMinRange	= HOUNDEYE_MIN_ATTACK_RADIUS;
		Vector 			posLos;
		bool			foundLos	= false;
		
		if (GetEnemy() != NULL)
		{
			foundLos = GetTacticalServices()->FindLos(GetEnemyLKP(),GetEnemy()->EyePosition(), flMinRange, flMaxRange, 0.0, &posLos);
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
			return;
		}

		if (foundLos)
		{
			GetNavigator()->SetGoal( AI_NavGoal_t( posLos, ACT_RUN, AIN_HULL_TOLERANCE ) );
		}
		else
		{
			TaskFail(FAIL_NO_SHOOT);
		}
		break;
	}

	case TASK_HOUND_FALL_ASLEEP:
		{
			m_fAsleep = true; // signal that hound is lying down (must stand again before doing anything else!)
			TaskComplete( true );
			break;
		}
	case TASK_HOUND_WAKE_UP:
		{
			m_fAsleep = false; // signal that hound is standing again
			TaskComplete( true );
			break;
		}
	case TASK_HOUND_OPEN_EYE:
		{
			m_fDontBlink = false; // turn blinking back on and that code will automatically open the eye
			TaskComplete( true );
			break;
		}
	case TASK_HOUND_CLOSE_EYE:
		{
//<<TEMP>>			pev->skin = 0;
			m_fDontBlink = true; // tell blink code to leave the eye alone.
			break;
		}
	case TASK_HOUND_THREAT_DISPLAY:
		{
			SetIdealActivity( ACT_IDLE_ANGRY );
			break;
		}
	case TASK_HOUND_HOP_BACK:
		{
			SetIdealActivity( ACT_LEAP );
			break;
		}
	case TASK_RANGE_ATTACK1:
		{
			SetIdealActivity( ACT_RANGE_ATTACK1 );
			break;
		}
	default: 
		{
			BaseClass::StartTask(pTask);
			break;
		}
	}
}

//=========================================================
// RunTask 
//=========================================================
void CNPC_Houndeye::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HOUND_THREAT_DISPLAY:
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( GetEnemyLKP(), AI_KEEP_YAW_SPEED );

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			
			break;
		}
	case TASK_HOUND_CLOSE_EYE:
		{
			/*//<<TEMP>>
			if ( pev->skin < HOUNDEYE_EYE_FRAMES - 1 )
			{
				pev->skin++;
			}
			*/
			break;
		}
	case TASK_HOUND_HOP_BACK:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			BaseClass::RunTask(pTask);
			break;
		}
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Houndeye::PrescheduleThink ( void )
{
	BaseClass::PrescheduleThink();
	
	// if the hound is mad and is running, make hunt noises.
	if ( m_NPCState == NPC_STATE_COMBAT && ( GetActivity() == ACT_RUN ) && random->RandomFloat( 0, 1 ) < 0.2 )
	{
		WarnSound();
	}

	// at random, initiate a blink if not already blinking or sleeping
	if ( !m_fDontBlink )
	{
		/*//<<TEMP>>//<<TEMP>>
		if ( ( pev->skin == 0 ) && random->RandomInt(0,0x7F) == 0 )
		{// start blinking!
			pev->skin = HOUNDEYE_EYE_FRAMES - 1;
		}
		else if ( pev->skin != 0 )
		{// already blinking
			pev->skin--;
		}
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Houndeye::NPC_TranslateActivity( Activity eNewActivity )
{
	if ( eNewActivity == ACT_IDLE && m_NPCState == NPC_STATE_COMBAT )
	{
		return ACT_IDLE_ANGRY;
	}
	return eNewActivity;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Houndeye::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		{
			return BaseClass::TranslateSchedule( scheduleType );
		}
	case SCHED_RANGE_ATTACK1:
		{
			return SCHED_HOUND_RANGE_ATTACK1;
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			return SCHED_COMBAT_FACE;
		}
	default:
		{
			return BaseClass::TranslateSchedule ( scheduleType );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Is anyone in the squad currently attacking
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Houndeye::IsAnyoneInSquadAttacking( void )
{
	if (!m_pSquad)
	{
		return false;
	}

	AISquadIter_t iter;
	for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
	{
		if (pSquadMember->IsCurSchedule(SCHED_HOUND_RANGE_ATTACK1))
		{
			return true;
		}
	}
	return false;
}

//=========================================================
// SelectSchedule
//=========================================================
int CNPC_Houndeye::SelectSchedule( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_IDLE:
	case NPC_STATE_ALERT:
		{
			if ( HasCondition(COND_LIGHT_DAMAGE) ||
				 HasCondition(COND_HEAVY_DAMAGE) )
			{
				return SCHED_TAKE_COVER_FROM_ORIGIN;
			}
			break;
		}
	case NPC_STATE_COMBAT:
		{
			// dead enemy

			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// If a group attack was requested attack even if attack conditions not met
			if ( HasCondition( COND_HOUND_GROUP_ATTACK ))
			{
				// Check that I'm not standing in another hound eye 
				// before attacking
				trace_t tr;
				AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,1), 
								GetHullMins(), GetHullMaxs(),
								MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
				if (!tr.startsolid)
				{
					return SCHED_HOUND_GROUP_ATTACK;
				}

				// Otherwise attack as soon as I can
				else
				{
					m_flNextAttack = gpGlobals->curtime;
					SCHED_HOUND_ATTACK_STRAFE;
				}
			}

			// If a group retread was requested 
			if ( HasCondition( COND_HOUND_GROUP_RETREAT ))
			{
				return SCHED_HOUND_GROUP_RETREAT;
			}

			if ( HasCondition( COND_LIGHT_DAMAGE ) | 
				 HasCondition( COND_HEAVY_DAMAGE ) )
			{
				if ( random->RandomFloat( 0 , 1 ) <= 0.4 )
				{
					trace_t tr;
					Vector forward;
					AngleVectors( GetAbsAngles(), &forward );
					AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + forward * -128, 
						GetHullMins(), GetHullMaxs(),
						MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

					if ( tr.fraction == 1.0 )
					{
						// it's clear behind, so the hound will jump
						return SCHED_HOUND_HOP_RETREAT;
					}
				}

				return SCHED_TAKE_COVER_FROM_ENEMY;
			}

			// If a group rally was requested 
			if ( HasCondition( COND_HOUND_GROUP_RALLEY ))
			{
				return SCHED_HOUND_GROUP_RALLEY;
			}

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				if (m_pSquad && random->RandomInt(0,4) == 0)
				{
					if (!IsAnyoneInSquadAttacking())
					{
						EmitSound( "NPC_Houndeye.GroupAttack" );
						
						m_flSoundWaitTime = gpGlobals->curtime + 1.0;

						m_pSquad->BroadcastInteraction( g_interactionHoundeyeGroupAttack, NULL, this );
						return SCHED_HOUND_GROUP_ATTACK;
					}
				}
				//<<TEMP>>comment
				SetCollisionGroup( COLLISION_GROUP_NONE );
				return SCHED_RANGE_ATTACK1;
			}
			else
			{
				if (m_pSquad && random->RandomInt(0,5) == 0)
				{
					if (!IsAnyoneInSquadAttacking())
					{
						EmitSound( "NPC_Houndeye.GroupFollow" );

						m_flSoundWaitTime = gpGlobals->curtime + 1.0;

						m_pSquad->BroadcastInteraction( g_interactionHoundeyeGroupRalley, NULL, this );
						return SCHED_HOUND_ATTACK_STRAFE;
					}
				}
				return SCHED_HOUND_ATTACK_STRAFE;
			}
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Houndeye::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionHoundeyeGroupAttack)
	{
		SetCondition(COND_HOUND_GROUP_ATTACK);
		return true;
	}
	else if (interactionType == g_interactionHoundeyeGroupRetreat)
	{
		SetCondition(COND_HOUND_GROUP_RETREAT);
		return true;
	}
	else if (interactionType == g_interactionHoundeyeGroupRalley)
	{
		SetCondition(COND_HOUND_GROUP_RALLEY);
		SetTarget(sourceEnt);
		m_bLoopClockwise = false;
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// SCHED_HOUND_ATTACK_STRAFE
//
//  Run a cirle around my enemy
//=========================================================
AI_DEFINE_SCHEDULE 
(
	SCHED_HOUND_ATTACK_STRAFE  ,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_HOUND_ATTACK_STRAFE_REVERSE"
	"		TASK_HOUND_GET_PATH_TO_CIRCLE	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts "
	"		COND_WEAPON_SIGHT_OCCLUDED"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAVY_DAMAGE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_HOUND_GROUP_ATTACK"
	"		COND_HOUND_GROUP_RETREAT"
);

//=========================================================
// SCHED_HOUND_ATTACK_STRAFE_REVERSE
//
//  Run a cirle around my enemy
//=========================================================
AI_DEFINE_SCHEDULE 
(
	SCHED_HOUND_ATTACK_STRAFE_REVERSE  ,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_HOUND_CHASE_ENEMY"
	"		TASK_HOUND_REVERSE_STRAFE_DIR	0"
	"		TASK_HOUND_GET_PATH_TO_CIRCLE	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts "
	"		COND_WEAPON_SIGHT_OCCLUDED"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAVY_DAMAGE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_HOUND_GROUP_ATTACK"
	"		COND_HOUND_GROUP_RETREAT"
);

//========================================================
// SCHED_HOUND_CHASE_ENEMY
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_CHASE_ENEMY,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		30	 "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_HOUND_GROUP_ATTACK"
	"		COND_HOUND_GROUP_RETREAT"
);

//=========================================================
// SCHED_HOUND_GROUP_ATTACK
//
//  Face enemy, pause, then attack
//=========================================================
AI_DEFINE_SCHEDULE 
(
	SCHED_HOUND_GROUP_ATTACK  ,

	"	Tasks "
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_SPEAK_SENTENCE			0"
	"		TASK_WAIT					1"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_HOUND_RANGE_ATTACK1"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAVY_DAMAGE"
);	
	
//=========================================================
// > SCHED_HOUND_GROUP_RETREAT
//
//		Take cover from enemy! 
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_GROUP_RETREAT,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_HOUND_ATTACK_STRAFE"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						0.2"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_FIND_COVER_FROM_ENEMY		0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_REMEMBER					MEMORY:INCOVER"
	"		TASK_FACE_ENEMY					0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Translated to cover
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_HOUND_COVER_WAIT"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
// > SCHED_HOUND_GROUP_RALLEY
//
//		Run to rally hound! 
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_GROUP_RALLEY,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		30"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_HOUND_ATTACK_STRAFE"
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAVY_DAMAGE"
	"		COND_HOUND_GROUP_ATTACK"
	"		COND_HOUND_GROUP_RETREAT"
);

//=========================================================
// > SCHED_HOUND_COVER_WAIT
//
//		Wait in cover till enemy see's me or I take damage
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_COVER_WAIT,

	"	Tasks"
	"		TASK_WAIT						2"
	""
	"	Interrupts"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
);				

//=========================================================
// > SCHED_HOUND_RANGE_ATTACK1
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_RANGE_ATTACK1,

	"	Tasks"
	"		 TASK_STOP_MOVING			0"
	"		 TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_ANGRY"
	"		 TASK_FACE_IDEAL			0"
	"		 TASK_RANGE_ATTACK1			0"
	""
	"	Interrupts"
	//"		COND_LIGHT_DAMAGE"	// don't interupt on small damage
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > SCHED_HOUND_HOP_RETREAT
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HOUND_HOP_RETREAT,

	"	Tasks"
	"		 TASK_STOP_MOVING				0"
	"		 TASK_HOUND_HOP_BACK			0"
	"		 TASK_SET_SCHEDULE				SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
	""
	"	Interrupts"
);
