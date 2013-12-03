//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "soundenvelope.h"
#include "npc_manhack.h"
#include "ai_default.h"
#include "ai_node.h"
#include "ai_navigator.h"
#include "ai_pathfinder.h"
#include "ai_moveprobe.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_route.h"
#include "explode.h"
#include "basegrenade_shared.h"
#include "ndebugoverlay.h"
#include "decals.h"
#include "gib.h"
#include "game.h"			
#include "ai_interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "npcevent.h"
#include "props.h"
#include "te_effect_dispatch.h"
#include "ai_squadslot.h"
#include "world.h"
#include "smoke_trail.h"
#include "func_break.h"
#include "physics_impact_damage.h"
#include "weapon_physcannon.h"
#include "physics_prop_ragdoll.h"
#include "soundent.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// When the engine is running and the manhack is operating under power
// we don't let gravity affect him.
#define MANHACK_GRAVITY 0.000

#define MANHACK_GIB_COUNT			5 
#define MANHACK_INGORE_WATER_DIST	384

// Sound stuff
#define MANHACK_PITCH_DIST1		512
#define MANHACK_MIN_PITCH1		(100)
#define MANHACK_MAX_PITCH1		(160)
#define MANHACK_WATER_PITCH1	(85)
#define MANHACK_VOLUME1			0.55

#define MANHACK_PITCH_DIST2		400
#define MANHACK_MIN_PITCH2		(85)
#define MANHACK_MAX_PITCH2		(190)
#define MANHACK_WATER_PITCH2	(90)

#define MANHACK_NOISEMOD_HIDE 5000

#define MANHACK_BODYGROUP_BLADE	1
#define MANHACK_BODYGROUP_BLUR	2
#define MANHACK_BODYGROUP_OFF	0
#define MANHACK_BODYGROUP_ON	1

// ANIMATION EVENTS
#define MANHACK_AE_START_ENGINE			50
#define MANHACK_AE_DONE_UNPACKING		51
#define MANHACK_AE_OPEN_BLADE			52

//#define MANHACK_GLOW_SPRITE	"sprites/laserdot.vmt"
#define MANHACK_GLOW_SPRITE	"sprites/glow1.vmt"

#define	MANHACK_CHARGE_MIN_DIST	200

ConVar	sk_manhack_health( "sk_manhack_health","0");
ConVar	sk_manhack_melee_dmg( "sk_manhack_melee_dmg","0");
ConVar	sk_manhack_v2( "sk_manhack_v2","1");

extern void		SpawnBlood(Vector vecSpot, const Vector &vAttackDir, int bloodColor, float flDamage);
extern float	GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
Activity ACT_MANHACK_UNPACK;

//-----------------------------------------------------------------------------
// Manhack Conditions
//-----------------------------------------------------------------------------
enum ManhackConditions
{
	COND_MANHACK_START_ATTACK = LAST_SHARED_CONDITION,	// We are able to do a bombing run on the current enemy.
};

//-----------------------------------------------------------------------------
// Manhack schedules.
//-----------------------------------------------------------------------------
enum ManhackSchedules
{
	SCHED_MANHACK_ATTACK_HOVER = LAST_SHARED_SCHEDULE,
	SCHED_MANHACK_DEPLOY,
	SCHED_MANHACK_REGROUP,
	SCHED_MANHACK_SWARM_IDLE,
	SCHED_MANHACK_SWARM,
	SCHED_MANHACK_SWARM_FAILURE,
};


//-----------------------------------------------------------------------------
// Manhack tasks.
//-----------------------------------------------------------------------------
enum ManhackTasks
{
	TASK_MANHACK_HOVER = LAST_SHARED_TASK,
	TASK_MANHACK_UNPACK,
	TASK_MANHACK_FIND_SQUAD_CENTER,
	TASK_MANHACK_FIND_SQUAD_MEMBER,
	TASK_MANHACK_MOVEAT_SAVEPOSITION,
};

BEGIN_DATADESC( CNPC_Manhack )

	DEFINE_FIELD( m_vForceVelocity,			FIELD_VECTOR),

	DEFINE_FIELD( m_vTargetBanking,			FIELD_VECTOR),
	DEFINE_FIELD( m_vForceMoveTarget,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_fForceMoveTime,			FIELD_TIME),
	DEFINE_FIELD( m_vSwarmMoveTarget,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_fSwarmMoveTime,			FIELD_TIME),
	DEFINE_FIELD( m_fEnginePowerScale,		FIELD_FLOAT),

	DEFINE_FIELD( m_flNextEngineSoundTime,	FIELD_TIME),
	DEFINE_FIELD( m_flEngineStallTime,		FIELD_TIME),
	DEFINE_FIELD( m_flNextBurstTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flWaterSuspendTime,		FIELD_TIME),
	DEFINE_FIELD( m_nLastSpinSound,			FIELD_INTEGER ),

	// Death
	DEFINE_FIELD( m_fSparkTime,				FIELD_TIME),
	DEFINE_FIELD( m_fSmokeTime,				FIELD_TIME),

	DEFINE_FIELD( m_bDirtyPitch,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bGib,					FIELD_BOOLEAN),
	DEFINE_FIELD( m_bHeld,					FIELD_BOOLEAN),
	
	DEFINE_FIELD( m_bHackedByAlyx,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_vecLoiterPosition,		FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_fTimeNextLoiterPulse,	FIELD_TIME),

	DEFINE_FIELD( m_flBumpSuppressTime,		FIELD_TIME ),

	DEFINE_FIELD( m_bBladesActive,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_flBladeSpeed,				FIELD_FLOAT),
	DEFINE_KEYFIELD( m_bIgnoreClipbrushes,	FIELD_BOOLEAN, "ignoreclipbrushes" ),
	DEFINE_FIELD( m_hSmokeTrail,				FIELD_EHANDLE),

	// DEFINE_FIELD( m_pLightGlow,				FIELD_CLASSPTR ),
	// DEFINE_FIELD( m_pEyeGlow,					FIELD_CLASSPTR ),

	DEFINE_FIELD( m_iPanel1, FIELD_INTEGER ),
	DEFINE_FIELD( m_iPanel2, FIELD_INTEGER ),
	DEFINE_FIELD( m_iPanel3, FIELD_INTEGER ),
	DEFINE_FIELD( m_iPanel4, FIELD_INTEGER ),

	DEFINE_FIELD( m_nLastWaterLevel,			FIELD_INTEGER ),
	DEFINE_FIELD( m_bDoSwarmBehavior,			FIELD_BOOLEAN ),

	DEFINE_FIELD( m_nEnginePitch1,				FIELD_INTEGER ),
	DEFINE_FIELD( m_flEnginePitch1Time,			FIELD_TIME ),
	DEFINE_FIELD( m_nEnginePitch2,				FIELD_INTEGER ),
	DEFINE_FIELD( m_flEnginePitch2Time,			FIELD_TIME ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_FIELD( m_flBurstDuration,	FIELD_FLOAT ),
	DEFINE_FIELD( m_vecBurstDirection,	FIELD_VECTOR ),
	DEFINE_FIELD( m_bShowingHostile,	FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableSwarm", InputDisableSwarm ),
	DEFINE_INPUTFUNC( FIELD_VOID,   "Unpack",		InputUnpack ),

	DEFINE_ENTITYFUNC( CrashTouch ),

	DEFINE_BASENPCINTERACTABLE_DATADESC(),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_manhack, CNPC_Manhack );

IMPLEMENT_SERVERCLASS_ST(CNPC_Manhack, DT_NPC_Manhack)
	SendPropIntWithMinusOneFlag	(SENDINFO(m_nEnginePitch1), 8 ),
	SendPropFloat(SENDINFO(m_flEnginePitch1Time), 0, SPROP_NOSCALE),
	SendPropIntWithMinusOneFlag(SENDINFO(m_nEnginePitch2), 8 )
END_SEND_TABLE()



//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_Manhack::CNPC_Manhack()
{
#ifdef _DEBUG
	m_vForceMoveTarget.Init();
	m_vSwarmMoveTarget.Init();
	m_vTargetBanking.Init();
	m_vForceVelocity.Init();
#endif
	m_bDirtyPitch = true;
	m_nLastWaterLevel = 0;
	m_nEnginePitch1 = -1;
	m_nEnginePitch2 = -1;
	m_flEnginePitch1Time = 0;
	m_flEnginePitch1Time = 0;
	m_bDoSwarmBehavior = true;
	m_flBumpSuppressTime = 0;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CNPC_Manhack::~CNPC_Manhack()
{
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Manhack::Classify(void)
{
	return (m_bHeld||m_bHackedByAlyx) ? CLASS_PLAYER_ALLY : CLASS_MANHACK; 
}



//-----------------------------------------------------------------------------
// Purpose: Turns the manhack into a physics corpse when dying.
//-----------------------------------------------------------------------------
void CNPC_Manhack::Event_Dying(void)
{
	DestroySmokeTrail();
	SetHullSizeNormal();
	BaseClass::Event_Dying();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Manhack::GatherConditions()
{
	BaseClass::GatherConditions();

	if( IsLoitering() && GetEnemy() )
	{
		StopLoitering();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	UpdatePanels();

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Stuck in water!

		// Reduce engine power so that the manhack lifts out of the water slowly.
		m_fEnginePowerScale = 0.75;
	}

	// ----------------------------------------
	//	Am I in water?
	// ----------------------------------------
	if ( GetWaterLevel() > 0 )
	{
		if( m_nLastWaterLevel == 0 )
		{
			Splash( WorldSpaceCenter() );
		}

		if( IsAlive() )
		{
			// If I've been out of water for 2 seconds or more, I'm eligible to be stuck in water again.
			if( gpGlobals->curtime - m_flWaterSuspendTime > 2.0 )
			{
				m_flWaterSuspendTime = gpGlobals->curtime + 1.0;
			}
		}
	}
	else
	{
		if( m_nLastWaterLevel != 0 )
		{
			Splash( WorldSpaceCenter() );
		}
	}

	m_nLastWaterLevel = GetWaterLevel();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	g_vecAttackDir = vecDir;

	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}

	if ( info.GetDamageType() & DMG_CLUB )
	{
		// Clubbed!
//		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);
		g_pEffects->Sparks( ptr->endpos, 1, 1, &ptr->plane.normal );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::DeathSound( const CTakeDamageInfo &info )
{
	StopSound("NPC_Manhack.Stunned");
	CPASAttenuationFilter filter2( this, "NPC_Manhack.Die" );
	EmitSound( filter2, entindex(), "NPC_Manhack.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Manhack::ShouldGib( const CTakeDamageInfo &info )
{
	return ( m_bGib );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Event_Killed( const CTakeDamageInfo &info )
{
	// turn off the blur!
	SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );

	// Sparks
	for (int i = 0; i < 3; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks( sparkPos, 2 );
	}

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	if ( m_nEnginePitch1 < 0 )
	{
		// Probably this manhack was killed immediately after spawning. Turn the sound
		// on right now so that we can pitch it up for the crash!
		SoundInit();
	}

	// Always gib when clubbed or blasted or crushed, or just randomly
	if ( ( info.GetDamageType() & (DMG_CLUB|DMG_CRUSH|DMG_BLAST) ) || ( random->RandomInt( 0, 1 ) ) )
	{
		m_bGib = true;
	}
	else
	{
		m_bGib = false;
		
		//FIXME: These don't stay with the ragdolls currently -- jdw
		// Long fadeout on the sprites!!
		KillSprites( 0.0f );
	}

	BaseClass::Event_Killed( info );
}

void CNPC_Manhack::HitPhysicsObject( CBaseEntity *pOther )
{
	IPhysicsObject *pOtherPhysics = pOther->VPhysicsGetObject();
	Vector pos, posOther;
	// Put the force on the line between the manhack origin and hit object origin
	VPhysicsGetObject()->GetPosition( &pos, NULL );
	pOtherPhysics->GetPosition( &posOther, NULL );
	Vector dir = posOther - pos;
	VectorNormalize(dir);
	// size/2 is approx radius
	pos += dir * WorldAlignSize().x * 0.5;
	Vector cross;

	// UNDONE: Use actual manhack up vector so the fake blade is
	// in the right plane?
	// Get a vector in the x/y plane in the direction of blade spin (clockwise)
	CrossProduct( dir, Vector(0,0,1), cross );
	VectorNormalize( cross );
	// force is a 30kg object going 100 in/s
	pOtherPhysics->ApplyForceOffset( cross * 30 * 100, pos );
}


//-----------------------------------------------------------------------------
// Take damage from being thrown by a physcannon 
//-----------------------------------------------------------------------------
#define MANHACK_SMASH_SPEED 500.0	// How fast a manhack must slam into something to take full damage
void CNPC_Manhack::TakeDamageFromPhyscannon( CBasePlayer *pPlayer )
{
	CTakeDamageInfo info;
	info.SetDamageType( DMG_GENERIC );
	info.SetInflictor( this );
	info.SetAttacker( pPlayer );
	info.SetDamagePosition( GetAbsOrigin() );
	info.SetDamageForce( Vector( 1.0, 1.0, 1.0 ) );

	// Convert velocity into damage.
	Vector vel;
	VPhysicsGetObject()->GetVelocity( &vel, NULL );
	float flSpeed = vel.Length();

	float flFactor = flSpeed / MANHACK_SMASH_SPEED;

	// Clamp. Don't inflict negative damage or massive damage!
	flFactor = clamp( flFactor, 0.0f, 2.0f );
	float flDamage = m_iMaxHealth * flFactor;

#if 0
	Msg("Doing %f damage for %f speed!\n", flDamage, flSpeed );
#endif

	info.SetDamage( flDamage );
	TakeDamage( info );
}


//-----------------------------------------------------------------------------
// Take damage from a vehicle; it's like a really big crowbar 
//-----------------------------------------------------------------------------
void CNPC_Manhack::TakeDamageFromVehicle( int index, gamevcollisionevent_t *pEvent )
{
	// Use the vehicle velocity to determine the damage
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];

	float flSpeed = pEvent->preVelocity[ otherIndex ].Length();
	flSpeed = clamp( flSpeed, 300.0f, 600.0f );
	float flDamage = SimpleSplineRemapVal( flSpeed, 300.0f, 600.0f, 0.0f, 1.0f );
	if ( flDamage == 0.0f )
		return;

	flDamage *= 20.0f;

	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );

	Vector damageForce = 2.0f * pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
	if ( damageForce == vec3_origin )
	{
		// This can happen if this entity is a func_breakable, and can't move.
		// Use the velocity of the entity that hit us instead.
		damageForce = 2.0f * pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
	}
	Assert( damageForce != vec3_origin );
	CTakeDamageInfo dmgInfo( pOther, pOther, damageForce, damagePos, flDamage, DMG_CRUSH );
	TakeDamage( dmgInfo );
}


//-----------------------------------------------------------------------------
// Take damage from combine ball
//-----------------------------------------------------------------------------
void CNPC_Manhack::TakeDamageFromPhysicsImpact( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pHitEntity = pEvent->pEntities[!index];

	// NOTE: Bypass the normal impact energy scale here.
	float flDamageScale = PlayerHasMegaPhysCannon() ? 10.0f : 1.0f;
	int damageType = 0;
	float damage = CalculateDefaultPhysicsDamage( index, pEvent, flDamageScale, true, damageType );
	if ( damage == 0 )
		return;

	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );
	Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
	if ( damageForce == vec3_origin )
	{
		// This can happen if this entity is motion disabled, and can't move.
		// Use the velocity of the entity that hit us instead.
		damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
	}

	// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
	PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MANHACK_SMASH_TIME	0.35		// How long after being thrown from a physcannon that a manhack is eligible to die from impact
void CNPC_Manhack::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	// Take no impact damage while being carried.
	if ( IsHeldByPhyscannon() )
		return;

	// Wake us up
	if ( m_spawnflags & SF_MANHACK_PACKED_UP )
	{
		SetCondition( COND_LIGHT_DAMAGE );
	}

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	CBasePlayer *pPlayer = HasPhysicsAttacker( MANHACK_SMASH_TIME );
	if( pPlayer )
	{
		if (!pHitEntity)
		{
			TakeDamageFromPhyscannon( pPlayer );
			StopBurst( true );
			return;
		}

		// Don't take damage from NPCs or server ragdolls killed by the manhack
		CRagdollProp *pRagdollProp = dynamic_cast<CRagdollProp*>(pHitEntity);
		if (!pHitEntity->IsNPC() && (!pRagdollProp || pRagdollProp->GetKiller() != this))
		{
			TakeDamageFromPhyscannon( pPlayer );
			StopBurst( true );
			return;
		}
	}

	if ( pHitEntity )
	{
		// It can take physics damage if it rams into a vehicle
		if ( pHitEntity->GetServerVehicle() )
		{
			TakeDamageFromVehicle( index, pEvent );
		}
		else if ( pHitEntity->HasPhysicsAttacker( 0.5f ) )
		{
			// It also can take physics damage from things thrown by the player.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
		else if ( FClassnameIs( pHitEntity, "prop_combine_ball" ) )
		{
			// It also can take physics damage from a combine ball.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
		else if ( m_iHealth <= 0 )
		{
			TakeDamageFromPhysicsImpact( index, pEvent );
		}

		StopBurst( true );
	}
}


void CNPC_Manhack::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];

	if ( pOther->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		HitPhysicsObject( pOther );
	}
	BaseClass::VPhysicsShadowCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Manhack is out of control! (dying) Just explode as soon as you touch anything!
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::CrashTouch( CBaseEntity *pOther )
{
	CTakeDamageInfo	info( GetWorldEntity(), GetWorldEntity(), 25, DMG_CRUSH );

	CorpseGib( info );
}


//-----------------------------------------------------------------------------
// Create smoke trail!
//-----------------------------------------------------------------------------
void CNPC_Manhack::CreateSmokeTrail()
{
	if ( HasSpawnFlags( SF_MANHACK_NO_DAMAGE_EFFECTS ) )
		return;

	if ( m_hSmokeTrail != NULL )
		return;

	SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
	if( !pSmokeTrail )
		return;

	pSmokeTrail->m_SpawnRate = 20;
	pSmokeTrail->m_ParticleLifetime = 0.5f;
	pSmokeTrail->m_StartSize	= 8;
	pSmokeTrail->m_EndSize		= 32;
	pSmokeTrail->m_SpawnRadius	= 5;
	pSmokeTrail->m_MinSpeed		= 15;
	pSmokeTrail->m_MaxSpeed		= 25;
	
	pSmokeTrail->m_StartColor.Init( 0.4f, 0.4f, 0.4f );
	pSmokeTrail->m_EndColor.Init( 0, 0, 0 );
	
	pSmokeTrail->SetLifetime(-1);
	pSmokeTrail->FollowEntity(this);

	m_hSmokeTrail = pSmokeTrail;
}

void CNPC_Manhack::DestroySmokeTrail()
{
	if ( m_hSmokeTrail.Get() )
	{
		UTIL_Remove( m_hSmokeTrail );
		m_hSmokeTrail = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Manhack::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Hafta make a copy of info cause we might need to scale damage.(sjb)
	CTakeDamageInfo tdInfo = info;

	if( tdInfo.GetAmmoType() == GetAmmoDef()->Index("SniperRound") )
	{
		// Unfortunately, this is the easiest way to stop the sniper killing manhacks in one shot.
		tdInfo.SetDamage( m_iMaxHealth>>1 );
	}

	if (info.GetDamageType() & DMG_PHYSGUN )
	{
		m_flBladeSpeed = 20.0;

		// respond to physics
		// FIXME: shouldn't this happen in a base class?  Anyway to prevent it from happening twice?
		VPhysicsTakeDamage( info );

		// reduce damage to nothing
		tdInfo.SetDamage( 1.0 );

		StopBurst( true );
	}
	else if ( info.GetDamageType() & DMG_AIRBOAT )
	{
		// Airboat gun kills me instantly.
		tdInfo.SetDamage( GetHealth() );
	}
	else if (info.GetDamageType() & DMG_CLUB)
	{
		// Being hit by a club means a couple of things:
		//
		//		-I'm going to be knocked away from the person that clubbed me.
		//		 if fudging this vector a little bit could help me slam into a physics object,
		//		 then make that adjustment. This is a simple heuristic. The manhack will be
		//		 directed towards the physics object that is closest to g_vecAttackDir
		//

		//		-Take 150% damage from club attacks. This makes crowbar duels take two hits.
		
		tdInfo.ScaleDamage( 1.50 );

#define MANHACK_PHYS_SEARCH_SIZE		64
#define	MANHACK_PHYSICS_SEARCH_RADIUS	128

		CBaseEntity *pList[ MANHACK_PHYS_SEARCH_SIZE ];

		Vector attackDir = info.GetDamageForce();
		VectorNormalize( attackDir );

		Vector testCenter = GetAbsOrigin() + ( attackDir * MANHACK_PHYSICS_SEARCH_RADIUS );
		Vector vecDelta( MANHACK_PHYSICS_SEARCH_RADIUS, MANHACK_PHYSICS_SEARCH_RADIUS, MANHACK_PHYSICS_SEARCH_RADIUS );

		int count = UTIL_EntitiesInBox( pList, MANHACK_PHYS_SEARCH_SIZE, testCenter - vecDelta, testCenter + vecDelta, 0 );

		Vector			vecBestDir = g_vecAttackDir;
		float			flBestDot = 0.90;
		IPhysicsObject	*pPhysObj;

		int i;
		for( i = 0 ; i < count ; i++ )
		{
			pPhysObj = pList[ i ]->VPhysicsGetObject();

			if( !pPhysObj || pPhysObj->GetMass() > 200 )
			{
				// Not physics.
				continue;
			}

			Vector center = pList[ i ]->WorldSpaceCenter();

			Vector vecDirToObject;
			VectorSubtract( center, WorldSpaceCenter(), vecDirToObject );
			VectorNormalize( vecDirToObject );

			float flDot;

			flDot = DotProduct( g_vecAttackDir, vecDirToObject );
			

			if( flDot > flBestDot )
			{
				flBestDot = flDot;
				vecBestDir = vecDirToObject;
			}
		}

		tdInfo.SetDamageForce( vecBestDir * info.GetDamage() * 200 );

		// FIXME: shouldn't this happen in a base class?  Anyway to prevent it from happening twice?
		VPhysicsTakeDamage( tdInfo );

		// Force us away (no more residual speed hits!)
		m_vForceVelocity = vecBestDir * info.GetDamage() * 0.5f;
		m_flBladeSpeed = 10.0;

		EmitSound( "NPC_Manhack.Bat" );	

		// tdInfo.SetDamage( 1.0 );

		m_flEngineStallTime = gpGlobals->curtime + 0.5f;
		StopBurst( true );
	}
	else
	{
		m_flBladeSpeed = 20.0;

		Vector vecDamageDir = tdInfo.GetDamageForce();
		VectorNormalize( vecDamageDir );

		m_flEngineStallTime = gpGlobals->curtime + 0.25f;
		m_vForceVelocity = vecDamageDir * info.GetDamage() * 20.0f;

		tdInfo.SetDamageForce( tdInfo.GetDamageForce() * 20 );

		VPhysicsTakeDamage( info );
	}

	int nRetVal = BaseClass::OnTakeDamage_Alive( tdInfo );
	if ( nRetVal )
	{
		if ( m_iHealth > 0 )
		{
			if ( info.GetDamageType() & DMG_CLUB )
			{
				SetEyeState( MANHACK_EYE_STATE_STUNNED );
			}

			if ( m_iHealth <= ( m_iMaxHealth / 2 ) )
			{
				CreateSmokeTrail();
			}
		}
		else
		{
			DestroySmokeTrail();
		}
	}

	return nRetVal;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_Manhack::CorpseGib( const CTakeDamageInfo &info )
{
	Vector			vecGibVelocity;
	AngularImpulse	vecGibAVelocity;

	if( info.GetDamageType() & DMG_CLUB )
	{
		// If clubbed to death, break apart before the attacker's eyes!
		vecGibVelocity = g_vecAttackDir * -150;

		vecGibAVelocity.x = random->RandomFloat( -2000, 2000 );
		vecGibAVelocity.y = random->RandomFloat( -2000, 2000 );
		vecGibAVelocity.z = random->RandomFloat( -2000, 2000 );
	}
	else
	{
		// Shower the pieces with my velocity.
		vecGibVelocity = GetCurrentVelocity();

		vecGibAVelocity.x = random->RandomFloat( -500, 500 );
		vecGibAVelocity.y = random->RandomFloat( -500, 500 );
		vecGibAVelocity.z = random->RandomFloat( -500, 500 );
	}

	PropBreakableCreateAll( GetModelIndex(), NULL, GetAbsOrigin(), GetAbsAngles(), vecGibVelocity, vecGibAVelocity, 1.0, 60, COLLISION_GROUP_DEBRIS );

	RemoveDeferred();

	KillSprites( 0.0f );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Explode the manhack if it's damaged while crashing
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Manhack::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// Ignore damage for the first 1 second of crashing behavior.
	// If we don't do this, manhacks always just explode under 
	// sustained fire.
	VPhysicsTakeDamage( info );
	
	return 0;
}

//-----------------------------------------------------------------------------
// Turn on the engine sound if we're gagged!
//-----------------------------------------------------------------------------
void CNPC_Manhack::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if( m_vNoiseMod.z == MANHACK_NOISEMOD_HIDE && !(m_spawnflags & SF_NPC_WAIT_FOR_SCRIPT) && !(m_spawnflags & SF_MANHACK_PACKED_UP) )
	{
		// This manhack should get a normal noisemod now.
		float flNoiseMod = random->RandomFloat( 1.7, 2.3 );
		
		// Just bob up and down.
		SetNoiseMod( 0, 0, flNoiseMod );
	}

	if( NewState != NPC_STATE_IDLE && (m_spawnflags & SF_NPC_GAG) && (m_nEnginePitch1 < 0) )
	{
		m_spawnflags &= ~SF_NPC_GAG;
		SoundInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::HandleAnimEvent( animevent_t *pEvent )
{
	Vector vecNewVelocity;
	switch( pEvent->event )
	{
	case MANHACK_AE_START_ENGINE:
		StartEye();
		StartEngine( true );
		m_spawnflags &= ~SF_MANHACK_PACKED_UP;

		// No bursts until fully unpacked!
		m_flNextBurstTime = gpGlobals->curtime + FLT_MAX;
		break;

	case MANHACK_AE_DONE_UNPACKING:
		m_flNextBurstTime = gpGlobals->curtime + 2.0;
		break;

	case MANHACK_AE_OPEN_BLADE:
		m_bBladesActive = true;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not the given activity would translate to flying.
//-----------------------------------------------------------------------------
bool CNPC_Manhack::IsFlyingActivity( Activity baseAct )
{
	return ((baseAct == ACT_FLY || baseAct == ACT_IDLE || baseAct == ACT_RUN || baseAct == ACT_WALK) && m_bBladesActive);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
Activity CNPC_Manhack::NPC_TranslateActivity( Activity baseAct )
{
	if (IsFlyingActivity( baseAct ))
	{
		return (Activity)ACT_FLY;
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_Manhack::TranslateSchedule( int scheduleType ) 
{
	// Fail-safe for deployment if packed up and interrupted
	if ( m_spawnflags & SF_MANHACK_PACKED_UP )
	{
		if ( scheduleType != SCHED_WAIT_FOR_SCRIPT )
			return SCHED_MANHACK_DEPLOY;
	}

	switch ( scheduleType )
	{
	case SCHED_MELEE_ATTACK1:
		{
			return SCHED_MANHACK_ATTACK_HOVER;
			break;
		}
	case SCHED_BACK_AWAY_FROM_ENEMY:
		{
			return SCHED_MANHACK_REGROUP;
			break;
		}
	case SCHED_CHASE_ENEMY:
		{
			// If we're waiting for our next attack opportunity, just swarm
			if ( m_flNextBurstTime > gpGlobals->curtime )
			{
				return SCHED_MANHACK_SWARM;
			}

			if ( !m_bDoSwarmBehavior || OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				return SCHED_CHASE_ENEMY;
			}
			else
			{
				return SCHED_MANHACK_SWARM;
			}
		}
	case SCHED_COMBAT_FACE:
		{
			// Don't care about facing enemy, handled automatically
			return TranslateSchedule( SCHED_CHASE_ENEMY );
			break;
		}
	case SCHED_WAKE_ANGRY:
		{
			if( m_spawnflags & SF_MANHACK_PACKED_UP )
			{
				return SCHED_MANHACK_DEPLOY;
			}
			else
			{
				return TranslateSchedule( SCHED_CHASE_ENEMY );
			}
			break;
		}

	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
	case SCHED_ALERT_FACE:
		{
			if ( m_pSquad && m_bDoSwarmBehavior )
			{
				return SCHED_MANHACK_SWARM_IDLE;
			}
			else
			{
				return BaseClass::TranslateSchedule(scheduleType);
			}
		}

	case SCHED_CHASE_ENEMY_FAILED:
		{
			// Relentless bastard! Doesn't fail (fail not valid anyway)
			return TranslateSchedule( SCHED_CHASE_ENEMY );
			break;
		}

	}
	return BaseClass::TranslateSchedule(scheduleType);
}

#define MAX_LOITER_DIST_SQR 144 // (12 inches sqr)
void CNPC_Manhack::Loiter()
{
	//NDebugOverlay::Line( GetAbsOrigin(), m_vecLoiterPosition, 255, 255, 255, false, 0.1 );

	// Friendly manhack is loitering.
	if( !m_bHeld )
	{
		float distSqr = m_vecLoiterPosition.DistToSqr(GetAbsOrigin());

		if( distSqr > MAX_LOITER_DIST_SQR )
		{
			Vector vecDir = m_vecLoiterPosition - GetAbsOrigin();
			VectorNormalize( vecDir );

			// Move back to our loiter position.
			if( gpGlobals->curtime > m_fTimeNextLoiterPulse )
			{
				// Apply a pulse of force if allowed right now.
				if( distSqr > MAX_LOITER_DIST_SQR * 4.0f )
				{
					//Msg("Big Pulse\n");
					m_vForceVelocity = vecDir * 12.0f;
				}
				else
				{
					//Msg("Small Pulse\n");
					m_vForceVelocity = vecDir * 6.0f;
				}

				m_fTimeNextLoiterPulse = gpGlobals->curtime + 1.0f;
			}
			else
			{
				m_vForceVelocity = vec3_origin;
			}
		}
		else
		{
			// Counteract velocity to slow down.
			Vector velocity = GetCurrentVelocity();
			m_vForceVelocity = velocity * -0.5;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::MaintainGroundHeight( void )
{
	float zSpeed = GetCurrentVelocity().z;

	if ( zSpeed > 32.0f )
		return;

	const float minGroundHeight = 52.0f;

	trace_t	tr;
	AI_TraceHull(	GetAbsOrigin(), 
		GetAbsOrigin() - Vector( 0, 0, minGroundHeight ), 
		GetHullMins(), 
		GetHullMaxs(), 
		(MASK_NPCSOLID_BRUSHONLY), 
		this, 
		COLLISION_GROUP_NONE, 
		&tr );

	if ( tr.fraction != 1.0f )
	{
		float speedAdj = MAX( 16, (-zSpeed*0.5f) );

		m_vForceVelocity += Vector(0,0,1) * ( speedAdj * ( 1.0f - tr.fraction ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_Manhack::OverrideMove( float flInterval )
{
	SpinBlades( flInterval );
		
	// Don't execute any move code if packed up.
	if( HasSpawnFlags(SF_MANHACK_PACKED_UP|SF_MANHACK_CARRIED) )
		return true;

	if( IsLoitering() )
	{
		Loiter();
	}
	else
	{
		MaintainGroundHeight();
	}

	// So cops, etc. will try to avoid them
	if ( !HasSpawnFlags( SF_MANHACK_NO_DANGER_SOUNDS ) && !m_bHeld )
	{
		CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), 75, flInterval, this );
	}

	// -----------------------------------------------------------------
	//  If I'm being forced to move somewhere
	// ------------------------------------------------------------------
	if (m_fForceMoveTime > gpGlobals->curtime)
	{
		MoveToTarget(flInterval, m_vForceMoveTarget);
	}
	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	else if (GetNavigator()->IsGoalActive())
	{
		bool bReducible = GetNavigator()->GetPath()->GetCurWaypoint()->IsReducible();
		const float strictTolerance = 64.0;
		//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, 10 ), 255, 0, 0, true, 0.1);
  		if ( ProgressFlyPath( flInterval, GetEnemy(), MoveCollisionMask(), bReducible, strictTolerance ) == AINPP_COMPLETE )
			return true;
	}
	// -----------------------------------------------------------------
	// If I'm supposed to swarm somewhere, try to go there
	// ------------------------------------------------------------------
	else if (m_fSwarmMoveTime > gpGlobals->curtime)
	{
		MoveToTarget(flInterval, m_vSwarmMoveTarget);
	}
	// -----------------------------------------------------------------
	// If I don't have anything better to do, just decelerate
	// -------------------------------------------------------------- ----
	else
	{
		float	myDecay	 = 9.5;
		Decelerate( flInterval, myDecay );

		m_vTargetBanking = vec3_origin;

		// -------------------------------------
		// If I have an enemy turn to face him
		// -------------------------------------
		if (GetEnemy())
		{
			TurnHeadToTarget(flInterval, GetEnemy()->EyePosition() );
		}
	}

	if ( m_iHealth <= 0 )
	{
		// Crashing!!
		MoveExecute_Dead(flInterval);
	}
	else
	{
		// Alive!
		MoveExecute_Alive(flInterval);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::TurnHeadRandomly(float flInterval )
{
	float desYaw = random->RandomFloat(0,360);

	float	iRate	 = 0.8;
	// Make frame rate independent
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_fHeadYaw	   = (iRate * m_fHeadYaw) + (1-iRate)*desYaw;
		timeToUse = -0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveToTarget(float flInterval, const Vector &vMoveTarget)
{
	if (flInterval <= 0)
	{
		return;
	}

	// -----------------------------------------
	// Don't steer if engine's have stalled
	// -----------------------------------------
	if ( gpGlobals->curtime < m_flEngineStallTime || m_iHealth <= 0 )
		return;

	if ( GetEnemy() != NULL )
	{
		TurnHeadToTarget( flInterval, GetEnemy()->EyePosition() );
	}
	else
	{
		TurnHeadToTarget( flInterval, vMoveTarget );
	}

	// -------------------------------------
	// Move towards our target
	// -------------------------------------
	float	myAccel;
	float	myZAccel = 300.0f;
	float	myDecay	 = 0.3f;

	Vector targetDir;
	float flDist;

	// If we're bursting, just head straight
	if ( m_flBurstDuration > gpGlobals->curtime )
	{
		float zDist = 500;

		// Steer towards our enemy if we're able to
		if ( GetEnemy() != NULL )
		{
			Vector steerDir = ( GetEnemy()->EyePosition() - GetAbsOrigin() );
			zDist = fabs( steerDir.z );
			VectorNormalize( steerDir );

			float useTime = flInterval;
			while ( useTime > 0.0f )
			{
				m_vecBurstDirection += ( steerDir * 4.0f );
				useTime -= 0.1f;
			}

			m_vecBurstDirection.z = steerDir.z;

			VectorNormalize( m_vecBurstDirection );
		}

		// Debug visualizations
		/*
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( targetDir * 64.0f ), 255, 0, 0, true, 2.1f );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( steerDir * 64.0f ), 0, 255, 0, true, 2.1f );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( m_vecBurstDirection * 64.0f ), 0, 0, 255, true, 2.1f );
		NDebugOverlay::Cross3D( GetAbsOrigin() , -Vector(8,8,8), Vector(8,8,8), 255, 0, 0, true, 2.1f );
		*/

		targetDir = m_vecBurstDirection;

		flDist	= FLT_MAX;
		myDecay	 = 0.3f;
#ifdef _XBOX
		myAccel	 = 500;
#else
		myAccel	 = 400;
#endif // _XBOX
		myZAccel = MIN( 500, zDist / flInterval );
	}
	else
	{
		Vector vecCurrentDir = GetCurrentVelocity();
		VectorNormalize( vecCurrentDir );

		targetDir = vMoveTarget - GetAbsOrigin();
		flDist = VectorNormalize( targetDir );
		
		float flDot = DotProduct( targetDir, vecCurrentDir );

		// Otherwise we should steer towards our goal
		if( flDot > 0.25 )
		{
			// If my target is in front of me, my flight model is a bit more accurate.
			myAccel = 300;
		}
		else
		{
			// Have a harder time correcting my course if I'm currently flying away from my target.
			myAccel = 200;
		}
	}

	// Clamp lateral acceleration
	if ( myAccel > ( flDist / flInterval ) )
	{
		myAccel = flDist / flInterval;
	}

	/*
	// Boost vertical movement
	if ( targetDir.z > 0 )
	{
		// Z acceleration is faster when we thrust upwards.
		// This is to help keep manhacks out of water. 
		myZAccel *= 5.0;
	}
	*/

	// Clamp vertical movement
	if ( myZAccel > flDist / flInterval )
	{
		myZAccel = flDist / flInterval;
	}

	// Scale by our engine force
	myAccel *= m_fEnginePowerScale;
	myZAccel *= m_fEnginePowerScale;
	
	MoveInDirection( flInterval, targetDir, myAccel, myZAccel, myDecay );

	// calc relative banking targets
	Vector forward, right;
	GetVectors( &forward, &right, NULL );
	m_vTargetBanking.x	= 40 * DotProduct( forward, targetDir );
	m_vTargetBanking.z	= 40 * DotProduct( right, targetDir );
	m_vTargetBanking.y	= 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: Ignore water if I'm close to my enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Manhack::MoveCollisionMask(void)
{
	return MASK_NPCSOLID;
}


//-----------------------------------------------------------------------------
// Purpose: Make a splash effect
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Splash( const Vector &vecSplashPos )
{
	CEffectData	data;

	data.m_fFlags = 0;
	data.m_vOrigin = vecSplashPos;
	data.m_vNormal = Vector( 0, 0, 1 );

	data.m_flScale = 8.0f;

	int contents = GetWaterType();

	// Verify we have valid contents
	if ( !( contents & (CONTENTS_SLIME|CONTENTS_WATER)))
	{
		// We're leaving the water so we have to reverify what it was
		trace_t	tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 256 ), (CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &tr );

		// Re-validate this
		if ( !(tr.contents&(CONTENTS_WATER|CONTENTS_SLIME)) )
		{
			//NOTENOTE: We called a splash but we don't seem to be near water?
			Assert( 0 );
			return;
		}

		contents = tr.contents;
	}
	
	// Mark us if we're in slime
	if ( contents & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	DispatchEffect( "watersplash", data );
}

//-----------------------------------------------------------------------------
// Computes the slice bounce velocity
//-----------------------------------------------------------------------------
void CNPC_Manhack::ComputeSliceBounceVelocity( CBaseEntity *pHitEntity, trace_t &tr )
{
	if( pHitEntity->IsAlive() && FClassnameIs( pHitEntity, "func_breakable_surf" ) )
	{
		// We want to see if the manhack hits a breakable pane of glass. To keep from checking
		// The classname of the HitEntity on each impact, we only do this check if we hit 
		// something that's alive. Anyway, prevent the manhack bouncing off the pane of glass,
		// since this impact will shatter the glass and let the manhack through.
		return;
	}

	Vector vecDir;
	
	// If the manhack isn't bouncing away from whatever he sliced, force it.
	VectorSubtract( WorldSpaceCenter(), pHitEntity->WorldSpaceCenter(), vecDir );
	VectorNormalize( vecDir );
	vecDir *= 200;
	vecDir[2] = 0.0f;
	
	// Knock it away from us
	if ( VPhysicsGetObject() != NULL )
	{
		VPhysicsGetObject()->ApplyForceOffset( vecDir * 4, GetAbsOrigin() );
	}

	// Also set our velocity
	SetCurrentVelocity( vecDir );
}


//-----------------------------------------------------------------------------
// Is the manhack being held?
//-----------------------------------------------------------------------------
bool CNPC_Manhack::IsHeldByPhyscannon( )
{
	return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD);
}

	
//-----------------------------------------------------------------------------
// Purpose: We've touched something that we can hurt. Slice it!
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Slice( CBaseEntity *pHitEntity, float flInterval, trace_t &tr )
{
	// Don't hurt the player if I'm in water
	if( GetWaterLevel() > 0 && pHitEntity->IsPlayer() )
		return;

	// Can't slice players holding it with the phys cannon
	if ( IsHeldByPhyscannon() )
	{
		if ( pHitEntity && (pHitEntity == HasPhysicsAttacker( FLT_MAX )) )
			return;
	}

	if ( pHitEntity->m_takedamage == DAMAGE_NO )
		return;

	// Damage must be scaled by flInterval so framerate independent
	float flDamage = sk_manhack_melee_dmg.GetFloat() * flInterval;

	if ( pHitEntity->IsPlayer() )
	{
		flDamage *= 2.0f;
	}
	
	// Held manhacks do more damage
	if ( IsHeldByPhyscannon() )
	{
		// Deal 100 damage/sec
		flDamage = 100.0f * flInterval;
	}
	else if ( pHitEntity->IsNPC() && HasPhysicsAttacker( MANHACK_SMASH_TIME ) )
	{
		extern ConVar sk_combine_guard_health;
		// NOTE: The else here is essential.
		// The physics attacker *will* be set even when the manhack is held
		flDamage = sk_combine_guard_health.GetFloat(); // the highest healthed fleshy enemy
	}
	else if ( dynamic_cast<CBaseProp*>(pHitEntity) || dynamic_cast<CBreakable*>(pHitEntity) )
	{
		// If we hit a prop, we want it to break immediately
		flDamage = pHitEntity->GetHealth();
	}
	else if ( pHitEntity->IsNPC() && IRelationType( pHitEntity ) == D_HT  && FClassnameIs( pHitEntity, "npc_combine_s" ) ) 
	{
		flDamage *= 6.0f;
	}

	if (flDamage < 1.0f)
	{
		flDamage = 1.0f;
	}

	CTakeDamageInfo info( this, this, flDamage, DMG_SLASH );

	// check for actual "ownership" of damage
	CBasePlayer *pPlayer = HasPhysicsAttacker( MANHACK_SMASH_TIME );
	if (pPlayer)
	{
		info.SetAttacker( pPlayer );
	}

	Vector dir = (tr.endpos - tr.startpos);
	if ( dir == vec3_origin )
	{
		dir = tr.m_pEnt->GetAbsOrigin() - GetAbsOrigin();
	}
	CalculateMeleeDamageForce( &info, dir, tr.endpos );
	pHitEntity->TakeDamage( info );

	// Spawn some extra blood where we hit
	if ( pHitEntity->BloodColor() == DONT_BLEED )
	{
		CEffectData data;
		Vector velocity = GetCurrentVelocity();

		data.m_vOrigin = tr.endpos;
		data.m_vAngles = GetAbsAngles();

		VectorNormalize( velocity );
		
		data.m_vNormal = ( tr.plane.normal + velocity ) * 0.5;;

		DispatchEffect( "ManhackSparks", data );

		EmitSound( "NPC_Manhack.Grind" );

		//TODO: What we really want to do is get a material reference and emit the proper sprayage! - jdw
	}
	else
	{
		SpawnBlood(tr.endpos, g_vecAttackDir, pHitEntity->BloodColor(), 6 );
		EmitSound( "NPC_Manhack.Slice" );
	}

	// Pop back a little bit after hitting the player
	ComputeSliceBounceVelocity( pHitEntity, tr );

	// Save off when we last hit something
	m_flLastDamageTime = gpGlobals->curtime;

	// Reset our state and give the player time to react
	StopBurst( true );
}

//-----------------------------------------------------------------------------
// Purpose: We've touched something solid. Just bump it.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Bump( CBaseEntity *pHitEntity, float flInterval, trace_t &tr )
{
	if ( !VPhysicsGetObject() )
		return;

	// Surpressing this behavior
	if ( m_flBumpSuppressTime > gpGlobals->curtime )
		return;

	if ( pHitEntity->GetMoveType() == MOVETYPE_VPHYSICS && pHitEntity->Classify()!=CLASS_MANHACK )
	{
		HitPhysicsObject( pHitEntity );
	}

	// We've hit something so deflect our velocity based on the surface
	// norm of what we've hit
	if (flInterval > 0)
	{
		float moveLen = ( (GetCurrentVelocity() * flInterval) * (1.0 - tr.fraction) ).Length();

		Vector moveVec	= moveLen*tr.plane.normal/flInterval;

		// If I'm totally dead, don't bounce me up
		if (m_iHealth <=0 && moveVec.z > 0)
		{
			moveVec.z = 0;
		}

		// If I'm right over the ground don't push down
		if (moveVec.z < 0)
		{
			float floorZ = GetFloorZ(GetAbsOrigin());
			if (abs(GetAbsOrigin().z - floorZ) < 36)
			{
				moveVec.z = 0;
			}
		}

		Vector myUp;
		VPhysicsGetObject()->LocalToWorldVector( &myUp, Vector( 0.0, 0.0, 1.0 ) );

		// plane must be something that could hit the blades
		if (fabs( DotProduct( myUp, tr.plane.normal ) ) < 0.25 )
		{
			CEffectData data;
			Vector velocity = GetCurrentVelocity();

			data.m_vOrigin = tr.endpos;
			data.m_vAngles = GetAbsAngles();

			VectorNormalize( velocity );
			
			data.m_vNormal = ( tr.plane.normal + velocity ) * 0.5;;

			DispatchEffect( "ManhackSparks", data );

			CBroadcastRecipientFilter filter;

			te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 180, 100, 0, 50, 0.3, 150 );
			
			// add some spin, but only if we're not already going fast..
			Vector vecVelocity;
			AngularImpulse vecAngVelocity;
			VPhysicsGetObject()->GetVelocity( &vecVelocity, &vecAngVelocity );
			float flDot = DotProduct( myUp, vecAngVelocity );
			if ( fabs(flDot) < 100 )
			{
				//AngularImpulse torque = myUp * (1000 - flDot * 10);
				AngularImpulse torque = myUp * (1000 - flDot * 2);
				VPhysicsGetObject()->ApplyTorqueCenter( torque );
			}
			
			if (!(m_spawnflags	& SF_NPC_GAG))
			{
				EmitSound( "NPC_Manhack.Grind" );
			}

			// For decals and sparks we must trace a line in the direction of the surface norm
			// that we hit.
			trace_t	decalTrace;
			AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() - (tr.plane.normal * 24),MASK_SOLID, this, COLLISION_GROUP_NONE, &decalTrace );

			if ( decalTrace.fraction != 1.0 )
			{
				// Leave decal only if colliding horizontally
				if ((DotProduct(Vector(0,0,1),decalTrace.plane.normal)<0.5) && (DotProduct(Vector(0,0,-1),decalTrace.plane.normal)<0.5))
				{
					UTIL_DecalTrace( &decalTrace, "ManhackCut" );
				}
			}
		}
		
		// See if we will not have a valid surface
		if ( tr.allsolid || tr.startsolid )
		{
			// Build a fake reflection back along our current velocity because we can't know how to reflect off
			// a non-existant surface! -- jdw

			Vector vecRandomDir = RandomVector( -1.0f, 1.0f );
			SetCurrentVelocity( vecRandomDir * 50.0f );
			m_flBumpSuppressTime = gpGlobals->curtime + 0.5f;
		}
		else
		{
			// This is a valid hit and we can deflect properly
			
			VectorNormalize( moveVec );
			float hitAngle = -DotProduct( tr.plane.normal, -moveVec );

			Vector vReflection = 2.0 * tr.plane.normal * hitAngle + -moveVec;

			float flSpeed = GetCurrentVelocity().Length();
			SetCurrentVelocity( GetCurrentVelocity() + vReflection * flSpeed * 0.5f );
		}
	}

	// -------------------------------------------------------------
	// If I'm on a path check LOS to my next node, and fail on path
	// if I don't have LOS.  Note this is the only place I do this,
	// so the manhack has to collide before failing on a path
	// -------------------------------------------------------------
	if (GetNavigator()->IsGoalActive() && !(GetNavigator()->GetPath()->CurWaypointFlags() & bits_WP_TO_PATHCORNER) )
	{
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_GROUND, GetAbsOrigin(), GetNavigator()->GetCurWaypointPos(), 
			MoveCollisionMask(), GetEnemy(), &moveTrace );

		if (IsMoveBlocked( moveTrace ) && 
			!moveTrace.pObstruction->ClassMatches( GetClassname() ))
		{
			TaskFail(FAIL_NO_ROUTE);
			GetNavigator()->ClearGoal();
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::CheckCollisions(float flInterval)
{
	// Trace forward to see if I hit anything. But trace forward along the
	// owner's view direction if you're being carried.
	Vector vecTraceDir, vecCheckPos;
	VPhysicsGetObject()->GetVelocity( &vecTraceDir, NULL );
	vecTraceDir *= flInterval;
	if ( IsHeldByPhyscannon() )
	{
		CBasePlayer *pCarrier = HasPhysicsAttacker( FLT_MAX );
		if ( pCarrier )
		{
			if ( pCarrier->CollisionProp()->CalcDistanceFromPoint( WorldSpaceCenter() ) < 30 )
			{
				AngleVectors( pCarrier->EyeAngles(), &vecTraceDir, NULL, NULL );
				vecTraceDir *= 40.0f;
			}
		}
	}

	VectorAdd( GetAbsOrigin(), vecTraceDir, vecCheckPos );
	
	trace_t			tr;
	CBaseEntity*	pHitEntity = NULL;
	
	AI_TraceHull(	GetAbsOrigin(), 
					vecCheckPos, 
					GetHullMins(), 
					GetHullMaxs(),
					MoveCollisionMask(),
					this,
					COLLISION_GROUP_NONE,
					&tr );

	if ( (tr.fraction != 1.0 || tr.startsolid) && tr.m_pEnt)
	{
		PhysicsMarkEntitiesAsTouching( tr.m_pEnt, tr );
		pHitEntity = tr.m_pEnt;

		if( m_bHeld && tr.m_pEnt->MyNPCPointer() && tr.m_pEnt->MyNPCPointer()->IsPlayerAlly() )
		{
			// Don't slice Alyx when she approaches to hack. We need a better solution for this!!
			//Msg("Ignoring!\n");
			return;
		}

		if ( pHitEntity != NULL && 
			 pHitEntity->m_takedamage == DAMAGE_YES && 
			 pHitEntity->Classify() != CLASS_MANHACK && 
			 gpGlobals->curtime > m_flWaterSuspendTime )
		{
			// Slice this thing
			Slice( pHitEntity, flInterval, tr );
			m_flBladeSpeed = 20.0;
		}
		else
		{
			// Just bump into this thing.
			Bump( pHitEntity, flInterval, tr );
			m_flBladeSpeed = 20.0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
#define tempTIME_STEP = 0.5;
void CNPC_Manhack::PlayFlySound(void)
{
	float flEnemyDist;

	if( GetEnemy() )
	{
		flEnemyDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length();
	}
	else
	{
		flEnemyDist = FLT_MAX;
	}

	if( m_spawnflags & SF_NPC_GAG )
	{
		// Quiet!
		return;
	}

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Just went in water. Slow the motor!!
		if( m_bDirtyPitch )
		{
			m_nEnginePitch1 = MANHACK_WATER_PITCH1;
			m_flEnginePitch1Time = gpGlobals->curtime + 0.5f;
			m_nEnginePitch2 = MANHACK_WATER_PITCH2;
			m_flEnginePitch2Time = gpGlobals->curtime + 0.5f;
			m_bDirtyPitch = false;
		}
	}
	// Spin sound based on distance from enemy (unless we're crashing)
	else if (GetEnemy() && IsAlive() )
	{
		if( flEnemyDist < MANHACK_PITCH_DIST1 )
		{
			// recalculate pitch.
			int iPitch1, iPitch2;
			float flDistFactor;

			flDistFactor = MIN( 1.0, 1 - flEnemyDist / MANHACK_PITCH_DIST1 ); 
			iPitch1 = MANHACK_MIN_PITCH1 + ( ( MANHACK_MAX_PITCH1 - MANHACK_MIN_PITCH1 ) * flDistFactor); 

			// NOTE: MANHACK_PITCH_DIST2 must be < MANHACK_PITCH_DIST1
			flDistFactor = MIN( 1.0, 1 - flEnemyDist / MANHACK_PITCH_DIST2 ); 
			iPitch2 = MANHACK_MIN_PITCH2 + ( ( MANHACK_MAX_PITCH2 - MANHACK_MIN_PITCH2 ) * flDistFactor); 

			m_nEnginePitch1 = iPitch1;
			m_flEnginePitch1Time = gpGlobals->curtime + 0.1f;
			m_nEnginePitch2 = iPitch2;
			m_flEnginePitch2Time = gpGlobals->curtime + 0.1f;

			m_bDirtyPitch = true;
		}
		else if( m_bDirtyPitch )
		{
			m_nEnginePitch1 = MANHACK_MIN_PITCH1;
			m_flEnginePitch1Time = gpGlobals->curtime + 0.1f;
			m_nEnginePitch2 = MANHACK_MIN_PITCH2;
			m_flEnginePitch2Time = gpGlobals->curtime + 0.2f;
			m_bDirtyPitch = false;
		}
	}
	// If no enemy just play low sound
	else if( IsAlive() && m_bDirtyPitch )
	{
		m_nEnginePitch1 = MANHACK_MIN_PITCH1;
		m_flEnginePitch1Time = gpGlobals->curtime + 0.1f;
		m_nEnginePitch2 = MANHACK_MIN_PITCH2;
		m_flEnginePitch2Time = gpGlobals->curtime + 0.2f;

		m_bDirtyPitch = false;
	}

	// Play special engine every once in a while
	if (gpGlobals->curtime > m_flNextEngineSoundTime && flEnemyDist < 48)
	{
		m_flNextEngineSoundTime	= gpGlobals->curtime + random->RandomFloat( 3.0, 10.0 );

		EmitSound( "NPC_Manhack.EngineNoise" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveExecute_Alive(float flInterval)
{
	PhysicsCheckWaterTransition();

	Vector	vCurrentVelocity = GetCurrentVelocity();

	// FIXME: move this
	VPhysicsGetObject()->Wake();

	if( m_fEnginePowerScale < GetMaxEnginePower() && gpGlobals->curtime > m_flWaterSuspendTime )
	{
		// Power is low, and we're no longer stuck in water, so bring power up.
		m_fEnginePowerScale += 0.05;
	}

	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------
	float noiseScale = 7.0f;

	if ( (CBaseEntity*)GetEnemy() )
	{
		float flDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D();

		if( flDist < MANHACK_CHARGE_MIN_DIST )
		{
			// Less noise up close.
			noiseScale = 2.0;
		}

		if ( IsInEffectiveTargetZone( GetEnemy() ) && flDist < MANHACK_CHARGE_MIN_DIST && gpGlobals->curtime > m_flNextBurstTime )
		{
			Vector vecCurrentDir = GetCurrentVelocity();
			VectorNormalize( vecCurrentDir );

			Vector vecToEnemy = ( GetEnemy()->EyePosition() - WorldSpaceCenter() );
			VectorNormalize( vecToEnemy );

			float flDot = DotProduct( vecCurrentDir, vecToEnemy );

			if ( flDot > 0.75 )
			{				
				Vector offsetDir = ( vecToEnemy - vecCurrentDir );
				VectorNormalize( offsetDir );

				Vector offsetSpeed = GetCurrentVelocity() * flDot;

				//FIXME: This code sucks -- jdw

				offsetDir.z = 0.0f;
				m_vForceVelocity += ( offsetDir * ( offsetSpeed.Length2D() * 0.25f ) );

				// Commit to the attack- no steering for about a second
				StartBurst( vecToEnemy );
				SetEyeState( MANHACK_EYE_STATE_CHARGE );
			}
		}
		
		if ( gpGlobals->curtime > m_flBurstDuration )
		{
			ShowHostile( false );
		}
	}

	// ----------------------------------------------------------------------------------------
	// Add in any forced velocity
	// ----------------------------------------------------------------------------------------
	SetCurrentVelocity( vCurrentVelocity + m_vForceVelocity );
	m_vForceVelocity = vec3_origin;

	if( !m_bHackedByAlyx || GetEnemy() )
	{
		// If hacked and no enemy, don't drift!
		AddNoiseToVelocity( noiseScale );
	}

	LimitSpeed( 200, ManhackMaxSpeed() );

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{ 
		if( UTIL_PointContents( GetAbsOrigin() ) & (CONTENTS_WATER|CONTENTS_SLIME) )
		{
			// Ooops, we're submerged somehow. Move upwards until our origin is out of the water.
			m_vCurrentVelocity.z = 20.0;
		}
		else
		{
			// Skimming the surface. Forbid any movement on Z
			m_vCurrentVelocity.z = 0.0;
		}
	}
	else if( GetWaterLevel() > 0 )
	{
		// Allow the manhack to lift off, but not to go deeper.
		m_vCurrentVelocity.z = MAX( m_vCurrentVelocity.z, 0 );
	}

	CheckCollisions(flInterval);

	// Blend out desired velocity when launched by the physcannon
	if ( HasPhysicsAttacker( MANHACK_SMASH_TIME ) && (!IsHeldByPhyscannon()) && VPhysicsGetObject() )
	{
		Vector vecCurrentVelocity;
		VPhysicsGetObject()->GetVelocity( &vecCurrentVelocity, NULL );
		float flLerpFactor = (gpGlobals->curtime - m_flLastPhysicsInfluenceTime) / MANHACK_SMASH_TIME;
		flLerpFactor = clamp( flLerpFactor, 0.0f, 1.0f );
		flLerpFactor = SimpleSplineRemapVal( flLerpFactor, 0.0f, 1.0f, 0.0f, 1.0f );
		VectorLerp( vecCurrentVelocity, m_vCurrentVelocity, flLerpFactor, m_vCurrentVelocity );
	}

	QAngle angles = GetLocalAngles();

	// ------------------------------------------
	//  Stalling
	// ------------------------------------------
	if (gpGlobals->curtime < m_flEngineStallTime)
	{
		/*
		// If I'm stalled add random noise
		angles.x += -20+(random->RandomInt(-10,10));
		angles.z += -20+(random->RandomInt(0,40));

		TurnHeadRandomly(flInterval);
		*/
	}
	else
	{
		// Make frame rate independent
		float	iRate	 = 0.5;
		float timeToUse = flInterval;
		while (timeToUse > 0)
		{
			m_vCurrentBanking.x = (iRate * m_vCurrentBanking.x) + (1 - iRate)*(m_vTargetBanking.x);
			m_vCurrentBanking.z = (iRate * m_vCurrentBanking.z) + (1 - iRate)*(m_vTargetBanking.z);
			timeToUse = -0.1;
		}
		angles.x = m_vCurrentBanking.x;
		angles.z = m_vCurrentBanking.z;
		angles.y = 0;

#if 0
		// Using our steering if we're not otherwise affecting our panels
		if ( m_flEngineStallTime < gpGlobals->curtime && m_flBurstDuration < gpGlobals->curtime )
		{
			Vector delta( 10 * AngleDiff( m_vTargetBanking.x, m_vCurrentBanking.x ), -10 * AngleDiff( m_vTargetBanking.z, m_vCurrentBanking.z ), 0 );
			//Vector delta( 3 * AngleNormalize( m_vCurrentBanking.x ), -4 * AngleNormalize( m_vCurrentBanking.z ), 0 );
			VectorYawRotate( delta, -m_fHeadYaw, delta );

			// DevMsg("%.0f %.0f\n", delta.x, delta.y );

			SetPoseParameter( m_iPanel1, -delta.x - delta.y * 2);
			SetPoseParameter( m_iPanel2, -delta.x + delta.y * 2);
			SetPoseParameter( m_iPanel3,  delta.x + delta.y * 2);
			SetPoseParameter( m_iPanel4,  delta.x - delta.y * 2);

			//SetPoseParameter( m_iPanel1, -delta.x );
			//SetPoseParameter( m_iPanel2, -delta.x );
			//SetPoseParameter( m_iPanel3, delta.x );
			//SetPoseParameter( m_iPanel4, delta.x );
		}
#endif
	}

	// SetLocalAngles( angles );

	if( m_lifeState != LIFE_DEAD )
	{
		PlayFlySound();
		// SpinBlades( flInterval );
		// WalkMove( GetCurrentVelocity() * flInterval, MASK_NPCSOLID );
	}

//	 NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -10 ), 0, 255, 0, true, 0.1);
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::SpinBlades(float flInterval)
{
	if (!m_bBladesActive)
	{
		SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_OFF );
		SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );
		m_flBladeSpeed = 0.0;
		m_flPlaybackRate = 1.0;
		return;
	}

	if ( IsFlyingActivity( GetActivity() ) )
	{
		// Blades may only ramp up while the engine is running
		if ( m_flEngineStallTime < gpGlobals->curtime )
		{
			if (m_flBladeSpeed < 10)
			{
				m_flBladeSpeed = m_flBladeSpeed * 2 + 1;
			}
			else
			{
				// accelerate engine
				m_flBladeSpeed = m_flBladeSpeed + 80 * flInterval;
			}
		}

		if (m_flBladeSpeed > 100)
		{
			m_flBladeSpeed = 100;
		}

		// blend through blades, blades+blur, blur
		if (m_flBladeSpeed < 20)
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_ON );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );
		}
		else if (m_flBladeSpeed < 40)
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_ON );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_ON );
		}
		else
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_OFF );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_ON );
		}

		m_flPlaybackRate = m_flBladeSpeed / 100.0;
	}
	else
	{
		m_flBladeSpeed = 0.0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Smokes and sparks, exploding periodically. Eventually it goes away.
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveExecute_Dead(float flInterval)
{
	if( GetWaterLevel() > 0 )
	{
		// No movement if sinking in water.
		return;
	}

	// Periodically emit smoke.
	if (gpGlobals->curtime > m_fSmokeTime && GetWaterLevel() == 0)
	{
//		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);
		m_fSmokeTime = gpGlobals->curtime + random->RandomFloat( 0.1, 0.3);
	}

	// Periodically emit sparks.
	if (gpGlobals->curtime > m_fSparkTime)
	{
		g_pEffects->Sparks( GetAbsOrigin() );
		m_fSparkTime = gpGlobals->curtime + random->RandomFloat(0.1, 0.3);
	}

	Vector newVelocity = GetCurrentVelocity();

	// accelerate faster and faster when dying
	newVelocity = newVelocity + (newVelocity * 1.5 * flInterval );

	// Lose lift
	newVelocity.z -= 0.02*flInterval*(GetCurrentGravity());

	// ----------------------------------------------------------------------------------------
	// Add in any forced velocity
	// ----------------------------------------------------------------------------------------
	newVelocity += m_vForceVelocity;
	SetCurrentVelocity( newVelocity );
	m_vForceVelocity = vec3_origin;


	// Lots of noise!! Out of control!
	AddNoiseToVelocity( 5.0 );


	// ----------------------
	// Limit overall speed
	// ----------------------
	LimitSpeed( -1, MANHACK_MAX_SPEED * 2.0 );

	QAngle angles = GetLocalAngles();

	// ------------------------------------------
	// If I'm dying, add random banking noise
	// ------------------------------------------
	angles.x += -20+(random->RandomInt(0,40));
	angles.z += -20+(random->RandomInt(0,40));

	CheckCollisions(flInterval);
	PlayFlySound();

	// SetLocalAngles( angles );

	WalkMove( GetCurrentVelocity() * flInterval,MASK_NPCSOLID );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::Precache(void)
{
	//
	// Model.
	//
	PrecacheModel("models/manhack.mdl");
	PrecacheModel( MANHACK_GLOW_SPRITE );
	PropBreakablePrecacheAll( MAKE_STRING("models/manhack.mdl") );
	
	PrecacheScriptSound( "NPC_Manhack.Die" );
	PrecacheScriptSound( "NPC_Manhack.Bat" );
	PrecacheScriptSound( "NPC_Manhack.Grind" );
	PrecacheScriptSound( "NPC_Manhack.Slice" );
	PrecacheScriptSound( "NPC_Manhack.EngineNoise" );
	PrecacheScriptSound( "NPC_Manhack.Unpack" );
	PrecacheScriptSound( "NPC_Manhack.ChargeAnnounce" );
	PrecacheScriptSound( "NPC_Manhack.ChargeEnd" );
	PrecacheScriptSound( "NPC_Manhack.Stunned" );

	// Sounds used on Client:
	PrecacheScriptSound( "NPC_Manhack.EngineSound1" );
	PrecacheScriptSound( "NPC_Manhack.EngineSound2"  );
	PrecacheScriptSound( "NPC_Manhack.BladeSound" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// The Manhack "regroups" when its in attack range but to
	// far above or below its enemy.  Set the start attack
	// condition if we are far enough away from the enemy
	// or at the correct height

	// Don't bother with Z if the enemy is in a vehicle
	float fl2DDist = 60.0f;
	float flZDist = 12.0f;

	if ( GetEnemy()->IsPlayer() && assert_cast< CBasePlayer * >(GetEnemy())->IsInAVehicle() )
	{
		flZDist = 24.0f;
	}

	if ((GetAbsOrigin() - pEnemy->GetAbsOrigin()).Length2D() > fl2DDist) 
	{
		SetCondition(COND_MANHACK_START_ATTACK);
	}
	else
	{
		float targetZ	= pEnemy->EyePosition().z;
		if (fabs(GetAbsOrigin().z - targetZ) < flZDist)
		{
			SetCondition(COND_MANHACK_START_ATTACK);
		}
	}
	BaseClass::GatherEnemyConditions(pEnemy);
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Manhack::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy() == NULL )
		return COND_NONE;

	//TODO: We could also decide if we want to back up here
	if ( m_flNextBurstTime > gpGlobals->curtime )
		return COND_NONE;

	float flMaxDist = 45;
	float flMinDist = 24;
	bool bEnemyInVehicle = GetEnemy()->IsPlayer() && assert_cast< CBasePlayer * >(GetEnemy())->IsInAVehicle();
	if ( GetEnemy()->IsPlayer() && assert_cast< CBasePlayer * >(GetEnemy())->IsInAVehicle() )
	{
		flMinDist = 0;
		flMaxDist = 200.0f;
	}

	if (flDist > flMaxDist)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDist < flMinDist)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	// Check our current velocity and speed, if it's too far off, we need to settle

	// Don't bother with Z if the enemy is in a vehicle
	if ( bEnemyInVehicle )
	{
		return COND_CAN_MELEE_ATTACK1;
	}

	// Assume the this check is in regards to my current enemy
	// for the Manhacks spetial condition
	float deltaZ = GetAbsOrigin().z - GetEnemy()->EyePosition().z;
	if ( (deltaZ > 12.0f) || (deltaZ < -24.0f) )
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// Override this task so we go for the enemy at eye level
	case TASK_MANHACK_HOVER:
		{
			break;
		}

	// If my enemy has moved significantly, update my path
	case TASK_WAIT_FOR_MOVEMENT:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy &&
				(GetCurSchedule()->GetId() == SCHED_CHASE_ENEMY) && 
				GetNavigator()->IsGoalActive() )
			{
				Vector vecEnemyPosition;
				vecEnemyPosition = pEnemy->EyePosition();
				if ( GetNavigator()->GetGoalPos().DistToSqr(vecEnemyPosition) > 40 * 40 )
				{
					GetNavigator()->UpdateGoalPos( vecEnemyPosition );
				}
			}				
			BaseClass::RunTask(pTask);
			break;
		}

	case TASK_MANHACK_MOVEAT_SAVEPOSITION:
		{
			// do the movement thingy

//			NDebugOverlay::Line( GetAbsOrigin(), m_vSavePosition, 0, 255, 0, true, 0.1);

			Vector dir = (m_vSavePosition - GetAbsOrigin());
			float dist = VectorNormalize( dir );
			float t = m_fSwarmMoveTime - gpGlobals->curtime;

			if (t < 0.1)
			{
				if (dist > 256)
				{
					TaskFail( FAIL_NO_ROUTE );
				}
				else
				{
					TaskComplete();
				}
			}
			else if (dist < 64)
			{
				m_vSwarmMoveTarget = GetAbsOrigin() - Vector( -dir.y, dir.x, 0 ) * 4;
			}
			else
			{
				m_vSwarmMoveTarget = GetAbsOrigin() + dir * 10;
			}
			break;
		}

	default:
		{
			BaseClass::RunTask(pTask);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::Spawn(void)
{
	Precache();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	SetModel( "models/manhack.mdl" );
	SetHullType(HULL_TINY_CENTERED); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	if ( HasSpawnFlags( SF_MANHACK_CARRIED ) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_NONE );
	}
	else
	{
		SetMoveType( MOVETYPE_VPHYSICS );
	}

	m_iHealth			= sk_manhack_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_NPCState			= NPC_STATE_NONE;

	if ( m_spawnflags & SF_MANHACK_USE_AIR_NODES)
	{
		SetNavType(NAV_FLY);
	}
	else
	{
		SetNavType(NAV_GROUND);
	}
		 
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL );
	AddEffects( EF_NOSHADOW );

	SetBloodColor( DONT_BLEED );
	SetCurrentVelocity( vec3_origin );
	m_vForceVelocity.Init();
	m_vCurrentBanking.Init();
	m_vTargetBanking.Init();

	m_flNextBurstTime	= gpGlobals->curtime;

	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_FLY | bits_CAP_SQUAD );

	m_flNextEngineSoundTime		= gpGlobals->curtime;
	m_flWaterSuspendTime		= gpGlobals->curtime;
	m_flEngineStallTime			= gpGlobals->curtime;
	m_fForceMoveTime			= gpGlobals->curtime;
	m_vForceMoveTarget			= vec3_origin;
	m_fSwarmMoveTime			= gpGlobals->curtime;
	m_vSwarmMoveTarget			= vec3_origin;
	m_nLastSpinSound			= -1;

	m_fSmokeTime		= 0;
	m_fSparkTime		= 0;

	// Set the noise mod to huge numbers right now, in case this manhack starts out waiting for a script
	// for instance, we don't want him to bob whilst he's waiting for a script. This allows designers
	// to 'hide' manhacks in small places. (sjb)
	SetNoiseMod( MANHACK_NOISEMOD_HIDE, MANHACK_NOISEMOD_HIDE, MANHACK_NOISEMOD_HIDE );

	// Start out with full power! 
	m_fEnginePowerScale = GetMaxEnginePower();
	
	// find panels
	m_iPanel1 = LookupPoseParameter( "Panel1" );
	m_iPanel2 = LookupPoseParameter( "Panel2" );
	m_iPanel3 = LookupPoseParameter( "Panel3" );
	m_iPanel4 = LookupPoseParameter( "Panel4" );

	m_fHeadYaw			= 0;

	NPCInit();

	// Manhacks are designed to slam into things, so don't take much damage from it!
	SetImpactEnergyScale( 0.001 );

	// Manhacks get 30 seconds worth of free knowledge.
	GetEnemies()->SetFreeKnowledgeDuration( 30.0 );
	
	// don't be an NPC, we want to collide with debris stuff
	SetCollisionGroup( COLLISION_GROUP_NONE );

	m_bHeld = false;
	m_bHackedByAlyx = false;
	StopLoitering();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartEye( void )
{
	//Create our Eye sprite
	if ( m_pEyeGlow == NULL )
	{
		m_pEyeGlow = CSprite::SpriteCreate( MANHACK_GLOW_SPRITE, GetLocalOrigin(), false );
		m_pEyeGlow->SetAttachment( this, LookupAttachment( "Eye" ) );
		
		if( m_bHackedByAlyx )
		{
			m_pEyeGlow->SetTransparency( kRenderTransAdd, 0, 255, 0, 128, kRenderFxNoDissipation );
			m_pEyeGlow->SetColor( 0, 255, 0 );
		}
		else
		{
			m_pEyeGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
			m_pEyeGlow->SetColor( 255, 0, 0 );
		}

		m_pEyeGlow->SetBrightness( 164, 0.1f );
		m_pEyeGlow->SetScale( 0.25f, 0.1f );
		m_pEyeGlow->SetAsTemporary();
	}

	//Create our light sprite
	if ( m_pLightGlow == NULL )
	{
		m_pLightGlow = CSprite::SpriteCreate( MANHACK_GLOW_SPRITE, GetLocalOrigin(), false );
		m_pLightGlow->SetAttachment( this, LookupAttachment( "Light" ) );

		if( m_bHackedByAlyx )
		{
			m_pLightGlow->SetTransparency( kRenderTransAdd, 0, 255, 0, 128, kRenderFxNoDissipation );
			m_pLightGlow->SetColor( 0, 255, 0 );
		}
		else
		{
			m_pLightGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
			m_pLightGlow->SetColor( 255, 0, 0 );
		}

		m_pLightGlow->SetBrightness( 164, 0.1f );
		m_pLightGlow->SetScale( 0.25f, 0.1f );
		m_pLightGlow->SetAsTemporary();
	}
}

//-----------------------------------------------------------------------------

void CNPC_Manhack::Activate()
{
	BaseClass::Activate();

	if ( IsAlive() )
	{
		StartEye();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the engine sound started. Unless we're not supposed to have it on yet!
//-----------------------------------------------------------------------------
void CNPC_Manhack::PostNPCInit( void )
{
	// SetAbsVelocity( vec3_origin ); 
	m_bBladesActive = (m_spawnflags & (SF_MANHACK_PACKED_UP|SF_MANHACK_CARRIED)) ? false : true;
	BladesInit();
}

void CNPC_Manhack::BladesInit()
{
	if( !m_bBladesActive )
	{
		// manhack is packed up, so has no power of its own. 
		// don't start the engine sounds.
		// make us fall a little slower than we should, for visual's sake
		SetGravity( UTIL_ScaleForGravity( 400 ) );

		SetActivity( ACT_IDLE );
	}
	else
	{
		bool engineSound = (m_spawnflags & SF_NPC_GAG) ? false : true;
		StartEngine( engineSound );
		SetActivity( ACT_FLY );
	}
}


//-----------------------------------------------------------------------------
// Crank up the engine!
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartEngine( bool fStartSound )
{
	if( fStartSound )
	{
		SoundInit();
	}

	// Make the blade appear.
	SetBodygroup( 1, 1 );

	// Pop up a little if falling fast!
	Vector vecVelocity;
	GetVelocity( &vecVelocity, NULL );
	if( ( m_spawnflags & SF_MANHACK_PACKED_UP ) && vecVelocity.z < 0 )
	{
		// DevMsg(" POP UP \n" );
		// ApplyAbsVelocityImpulse( Vector(0,0,-vecVelocity.z*0.75) );
	}

	// Under powered flight now.
	// SetMoveType( MOVETYPE_STEP );
	// SetGravity( MANHACK_GRAVITY );
	AddFlag( FL_FLY );
}


//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void CNPC_Manhack::SoundInit( void )
{
	m_nEnginePitch1 = MANHACK_MIN_PITCH1;
	m_flEnginePitch1Time = gpGlobals->curtime;
	m_nEnginePitch2 = MANHACK_MIN_PITCH2;
	m_flEnginePitch2Time = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Manhack::StopLoopingSounds(void)
{
	BaseClass::StopLoopingSounds();
	m_nEnginePitch1 = -1;
	m_flEnginePitch1Time = gpGlobals->curtime;
	m_nEnginePitch2 = -1;
	m_flEnginePitch2Time = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
	case TASK_MANHACK_UNPACK:
		{
			// Just play a sound for now.
			EmitSound( "NPC_Manhack.Unpack" );

			TaskComplete();
		}
		break;

	case TASK_MANHACK_HOVER:
		break;

	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_GET_PATH_TO_GOAL:
	case TASK_GET_PATH_TO_ENEMY_LKP:
	case TASK_GET_PATH_TO_PLAYER:
		{
			BaseClass::StartTask( pTask );
			/*
			// FIXME: why were these tasks considered bad?
			_asm
			{
				int	3;
				int 5;
			}
			*/
		}
		break;

	case TASK_FACE_IDEAL:
		{
			// this shouldn't ever happen, but if it does, don't choke
			TaskComplete();
		}
		break;

	case TASK_GET_PATH_TO_ENEMY:
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			CBaseEntity *pEnemy = GetEnemy();

			if ( pEnemy == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
						
			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =( 
				DevWarning( 2, "GetPathToEnemy failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
		break;

	case TASK_GET_PATH_TO_TARGET:
		// DevMsg("TARGET\n");
		BaseClass::StartTask( pTask );
		break;

	case TASK_MANHACK_FIND_SQUAD_CENTER:
		{
			if (!m_pSquad)
			{
				m_vSavePosition = GetAbsOrigin();
				TaskComplete();
				break;
			}

			// calc center of squad
			int count = 0;
			m_vSavePosition = Vector( 0, 0, 0 );

			// give attacking members more influence
			AISquadIter_t iter;
			for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
			{
				if (pSquadMember->HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
				{
					m_vSavePosition += pSquadMember->GetAbsOrigin() * 10;
					count += 10;
				}
				else
				{
					m_vSavePosition += pSquadMember->GetAbsOrigin();
					count++;
				}
			}

			// pull towards enemy
			if (GetEnemy() != NULL)
			{
				m_vSavePosition += GetEnemyLKP() * 4;
				count += 4;
			}

			Assert( count != 0 );
			m_vSavePosition = m_vSavePosition * (1.0 / count);

			TaskComplete();
		}
		break;

	case TASK_MANHACK_FIND_SQUAD_MEMBER:
		{
			if (m_pSquad)
			{
				CAI_BaseNPC *pSquadMember = m_pSquad->GetAnyMember();
				m_vSavePosition = pSquadMember->GetAbsOrigin();

				// find attacking members
				AISquadIter_t iter;
				for (pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
				{
					// are they attacking?
					if (pSquadMember->HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
					{
						m_vSavePosition = pSquadMember->GetAbsOrigin();
						break;
					}
					// do they have a goal?
					if (pSquadMember->GetNavigator()->IsGoalActive())
					{
						m_vSavePosition = pSquadMember->GetAbsOrigin();
						break;
					}
				}
			}
			else
			{
				m_vSavePosition = GetAbsOrigin();
			}

			TaskComplete();
		}
		break;

	case TASK_MANHACK_MOVEAT_SAVEPOSITION:
		{
			trace_t tr;
			AI_TraceLine( GetAbsOrigin(), m_vSavePosition, MASK_NPCWORLDSTATIC, this, COLLISION_GROUP_NONE, &tr );
			if (tr.DidHitWorld())
			{
				TaskFail( FAIL_NO_ROUTE );
			}
			else
			{
				m_fSwarmMoveTime = gpGlobals->curtime + RandomFloat( pTask->flTaskData * 0.8, pTask->flTaskData * 1.2 );
			}
		}
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::UpdateOnRemove( void )
{
	DestroySmokeTrail();
	KillSprites( 0.0 );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Manhack::HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionVortigauntClaw)
	{
		// Freeze so vortigaunt and hit me easier

		m_vForceMoveTarget.x = ((Vector *)data)->x;
		m_vForceMoveTarget.y = ((Vector *)data)->y;
		m_vForceMoveTarget.z = ((Vector *)data)->z;
		m_fForceMoveTime   = gpGlobals->curtime + 2.0;
		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_Manhack::ManhackMaxSpeed( void )
{
	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Slower in water!
		return MANHACK_MAX_SPEED * 0.1;
	}

	if ( HasPhysicsAttacker( MANHACK_SMASH_TIME ) )
	{
		return MANHACK_NPC_BURST_SPEED;
	}

	return MANHACK_MAX_SPEED;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::ClampMotorForces( Vector &linear, AngularImpulse &angular )
{
	float scale = m_flBladeSpeed / 100.0;

	// Msg("%.0f %.0f %.0f\n", linear.x, linear.y, linear.z );

	float fscale = 3000 * scale;

	if ( m_flEngineStallTime > gpGlobals->curtime )
	{
		linear.x = 0.0f;
		linear.y = 0.0f;
		linear.z = clamp( linear.z, -fscale, fscale < 1200 ? 1200 : fscale );
	}
	else
	{
		// limit reaction forces
		linear.x = clamp( linear.x, -fscale, fscale );
		linear.y = clamp( linear.y, -fscale, fscale );
		linear.z = clamp( linear.z, -fscale, fscale < 1200 ? 1200 : fscale );
	}

	angular.x *= scale;
	angular.y *= scale;
	angular.z *= scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::KillSprites( float flDelay )
{
	if( m_pEyeGlow )
	{
		m_pEyeGlow->FadeAndDie( flDelay );
		m_pEyeGlow = NULL;
	}

	if( m_pLightGlow )
	{
		m_pLightGlow->FadeAndDie( flDelay );
		m_pLightGlow = NULL;
	}

	// Re-enable for light trails
	/*
	if ( m_hLightTrail )
	{
		m_hLightTrail->FadeAndDie( flDelay );
		m_hLightTrail = NULL;
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Tests whether we're above the target's feet but also below their top
// Input  : *pTarget - who we're testing against
//-----------------------------------------------------------------------------
bool CNPC_Manhack::IsInEffectiveTargetZone( CBaseEntity *pTarget )
{
	Vector	vecMaxPos, vecMinPos;
	float	ourHeight = WorldSpaceCenter().z;

	// If the enemy is in a vehicle, we need to get those bounds
	if ( pTarget && pTarget->IsPlayer() && assert_cast< CBasePlayer * >(pTarget)->IsInAVehicle() )
	{
		CBaseEntity *pVehicle = assert_cast< CBasePlayer * >(pTarget)->GetVehicleEntity();
		pVehicle->CollisionProp()->NormalizedToWorldSpace( Vector(0.0f,0.0f,1.0f), &vecMaxPos );
		pVehicle->CollisionProp()->NormalizedToWorldSpace( Vector(0.0f,0.0f,0.0f), &vecMinPos );
	
		if ( ourHeight > vecMinPos.z && ourHeight < vecMaxPos.z )
			return true;

		return false;
	}
	
	// Get the enemies top and bottom point
	pTarget->CollisionProp()->NormalizedToWorldSpace( Vector(0.0f,0.0f,1.0f), &vecMaxPos );
#ifdef _XBOX
	pTarget->CollisionProp()->NormalizedToWorldSpace( Vector(0.0f,0.0f,0.5f), &vecMinPos ); // Only half the body is valid
#else
	pTarget->CollisionProp()->NormalizedToWorldSpace( Vector(0.0f,0.0f,0.0f), &vecMinPos );
#endif // _XBOX
	// See if we're within that range
	if ( ourHeight > vecMinPos.z && ourHeight < vecMaxPos.z )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//			&tolerance - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	if ( pEnemy && pEnemy->IsPlayer() && assert_cast< CBasePlayer * >(pEnemy)->IsInAVehicle() )
	{
		Vector vecNewPos;
		CBaseEntity *pVehicle = assert_cast< CBasePlayer * >(pEnemy)->GetVehicleEntity();
		pVehicle->CollisionProp()->NormalizedToWorldSpace( Vector(0.5,0.5,0.5f), &vecNewPos );
		chasePosition.z = vecNewPos.z;
	}
	else
	{
		Vector vecTarget;
		pEnemy->CollisionProp()->NormalizedToCollisionSpace( Vector(0,0,0.75f), &vecTarget );
		chasePosition.z += vecTarget.z;
	}
}

float CNPC_Manhack::GetDefaultNavGoalTolerance()
{
	return GetHullWidth();
}

//-----------------------------------------------------------------------------
// Purpose: Input that disables the manhack's swarm behavior
//-----------------------------------------------------------------------------
void CNPC_Manhack::InputDisableSwarm( inputdata_t &inputdata )
{
	m_bDoSwarmBehavior = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::InputUnpack( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_MANHACK_PACKED_UP ) == false )
		return;

	SetCondition( COND_LIGHT_DAMAGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//			reason - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	if ( reason == PUNTED_BY_CANNON )
	{
		StopLoitering();

		m_bHeld = false;

		// There's about to be a massive change in velocity. 
		// Think immediately so we can do our slice traces, etc.
		SetNextThink( gpGlobals->curtime + 0.01f );

		// Stall our engine for awhile
		m_flEngineStallTime = gpGlobals->curtime + 2.0f;
		SetEyeState( MANHACK_EYE_STATE_STUNNED );
	}
	else
	{
		// Suppress collisions between the manhack and the player; we're currently bumping
		// almost certainly because it's not purely a physics object.
		SetOwnerEntity( pPhysGunUser );
		m_bHeld = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//			Reason - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	// Stop suppressing collisions between the manhack and the player
	SetOwnerEntity( NULL );

	m_bHeld = false;

	if ( Reason == LAUNCHED_BY_CANNON )
	{
		m_hPhysicsAttacker = pPhysGunUser;
		m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

		// There's about to be a massive change in velocity. 
		// Think immediately so we can do our slice traces, etc.
		SetNextThink( gpGlobals->curtime + 0.01f );

		// Stall our engine for awhile
		m_flEngineStallTime = gpGlobals->curtime + 2.0f;
		SetEyeState( MANHACK_EYE_STATE_STUNNED );
	}
	else
	{
		if( m_bHackedByAlyx && !GetEnemy() )
		{
			// If a hacked manhack is released in peaceable conditions, 
			// just loiter, don't zip off.
			StartLoitering( GetAbsOrigin() );
		}

		m_hPhysicsAttacker = NULL;
		m_flLastPhysicsInfluenceTime = 0;
	}
}

void CNPC_Manhack::StartLoitering( const Vector &vecLoiterPosition )
{
	//Msg("Start Loitering\n");

	m_vTargetBanking = vec3_origin;
	m_vecLoiterPosition = GetAbsOrigin();
	m_vForceVelocity = vec3_origin;
	SetCurrentVelocity( vec3_origin );
}

CBasePlayer *CNPC_Manhack::HasPhysicsAttacker( float dt )
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if ( IsHeldByPhyscannon() || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime) )
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Manhacks that have been hacked by Alyx get more engine power (fly faster)
//-----------------------------------------------------------------------------
float CNPC_Manhack::GetMaxEnginePower()
{
	if( m_bHackedByAlyx )
	{
		return 2.0f;
	}

	return 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::UpdatePanels( void )
{
	if ( m_flEngineStallTime > gpGlobals->curtime )
	{
		SetPoseParameter( m_iPanel1, random->RandomFloat( 0.0f, 90.0f ) );
		SetPoseParameter( m_iPanel2, random->RandomFloat( 0.0f, 90.0f ) );
		SetPoseParameter( m_iPanel3, random->RandomFloat( 0.0f, 90.0f ) );
		SetPoseParameter( m_iPanel4, random->RandomFloat( 0.0f, 90.0f ) );
		return;
	}

	float panelPosition = GetPoseParameter( m_iPanel1 );

	if ( m_bShowingHostile )
	{
		panelPosition = 90.0f;//UTIL_Approach( 90.0f, panelPosition, 90.0f );
	}
	else
	{
		panelPosition = UTIL_Approach( 0.0f, panelPosition, 25.0f );
	}

	//FIXME: If we're going to have all these be equal, there's no need for 4 poses..
	SetPoseParameter( m_iPanel1, panelPosition );
	SetPoseParameter( m_iPanel2, panelPosition );
	SetPoseParameter( m_iPanel3, panelPosition );
	SetPoseParameter( m_iPanel4, panelPosition );

	//TODO: Make these waver randomly?
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hostile - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::ShowHostile( bool hostile /*= true*/)
{
	if ( m_bShowingHostile == hostile )
		return;

	//TODO: Open the manhack panels or close them, depending on the state
	m_bShowingHostile = hostile;

	if ( hostile )
	{
		EmitSound( "NPC_Manhack.ChargeAnnounce" );
	}
	else
	{
		EmitSound( "NPC_Manhack.ChargeEnd" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartBurst( const Vector &vecDirection )
{
	if ( m_flBurstDuration > gpGlobals->curtime )
		return;

	ShowHostile();

	// Don't burst attack again for a couple seconds
	m_flNextBurstTime = gpGlobals->curtime + 2.0;
	m_flBurstDuration = gpGlobals->curtime + 1.0;
	
	// Save off where we were going towards and for how long
	m_vecBurstDirection = vecDirection;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::StopBurst( bool bInterruptSchedule /*= false*/ )
{
	if ( m_flBurstDuration < gpGlobals->curtime )
		return;

	ShowHostile( false );

	// Stop our burst timers
	m_flNextBurstTime = gpGlobals->curtime + 2.0f; //FIXME: Skill level based
	m_flBurstDuration = gpGlobals->curtime - 0.1f;

	if ( bInterruptSchedule )
	{
		// We need to rethink our current schedule
		ClearSchedule( "Stopping burst" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::SetEyeState( int state )
{
	// Make sure we're active
	StartEye();

	switch( state )
	{
	case MANHACK_EYE_STATE_STUNNED:
		{
			if ( m_pEyeGlow )
			{
				//Toggle our state
				m_pEyeGlow->SetColor( 255, 128, 0 );
				m_pEyeGlow->SetScale( 0.15f, 0.1f );
				m_pEyeGlow->SetBrightness( 164, 0.1f );
				m_pEyeGlow->m_nRenderFX = kRenderFxStrobeFast;
			}
			
			if ( m_pLightGlow )
			{
				m_pLightGlow->SetColor( 255, 128, 0 );
				m_pLightGlow->SetScale( 0.15f, 0.1f );
				m_pLightGlow->SetBrightness( 164, 0.1f );
				m_pLightGlow->m_nRenderFX = kRenderFxStrobeFast;
			}

			EmitSound("NPC_Manhack.Stunned");

			break;
		}

	case MANHACK_EYE_STATE_CHARGE:
		{
			if ( m_pEyeGlow )
			{
				//Toggle our state
				if( m_bHackedByAlyx )
				{
					m_pEyeGlow->SetColor( 0, 255, 0 );
				}
				else
				{
					m_pEyeGlow->SetColor( 255, 0, 0 );
				}

				m_pEyeGlow->SetScale( 0.25f, 0.5f );
				m_pEyeGlow->SetBrightness( 164, 0.1f );
				m_pEyeGlow->m_nRenderFX = kRenderFxNone;
			}
			
			if ( m_pLightGlow )
			{
				if( m_bHackedByAlyx )
				{
					m_pLightGlow->SetColor( 0, 255, 0 );
				}
				else
				{
					m_pLightGlow->SetColor( 255, 0, 0 );
				}

				m_pLightGlow->SetScale( 0.25f, 0.5f );
				m_pLightGlow->SetBrightness( 164, 0.1f );
				m_pLightGlow->m_nRenderFX = kRenderFxNone;
			}

			break;
		}
	
	default:
		if ( m_pEyeGlow )
			m_pEyeGlow->m_nRenderFX = kRenderFxNone;
		break;
	}
}


unsigned int CNPC_Manhack::PhysicsSolidMaskForEntity( void ) const 
{ 
	unsigned int mask = BaseClass::PhysicsSolidMaskForEntity();
	if ( m_bIgnoreClipbrushes )
	{
		mask &= ~CONTENTS_MONSTERCLIP;
	}
	return mask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Manhack::CreateVPhysics( void )
{
	if ( HasSpawnFlags( SF_MANHACK_CARRIED ) )
		return false;

	return BaseClass::CreateVPhysics();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_manhack, CNPC_Manhack )

	DECLARE_TASK( TASK_MANHACK_HOVER );
	DECLARE_TASK( TASK_MANHACK_UNPACK );
	DECLARE_TASK( TASK_MANHACK_FIND_SQUAD_CENTER );
	DECLARE_TASK( TASK_MANHACK_FIND_SQUAD_MEMBER );
	DECLARE_TASK( TASK_MANHACK_MOVEAT_SAVEPOSITION );

	DECLARE_CONDITION( COND_MANHACK_START_ATTACK );

	DECLARE_ACTIVITY( ACT_MANHACK_UNPACK );

//=========================================================
// > SCHED_MANHACK_ATTACK_HOVER
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_ATTACK_HOVER,

	"	Tasks"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_FLY"
	"		TASK_MANHACK_HOVER		0"
	"	"
	"	Interrupts"
	"		COND_TOO_FAR_TO_ATTACK"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
);


//=========================================================
// > SCHED_MANHACK_ATTACK_HOVER
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_DEPLOY,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MANHACK_UNPACK"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_FLY"
	"	"
//	"	Interrupts"
);

//=========================================================
// > SCHED_MANHACK_REGROUP
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_REGROUP,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_TOLERANCE_DISTANCE					24"
	"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
	"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts"
	"		COND_MANHACK_START_ATTACK"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
);



//=========================================================
// > SCHED_MANHACK_SWARN
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM_IDLE,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_MANHACK_SWARM_FAILURE"
	"		TASK_MANHACK_FIND_SQUAD_CENTER				0"
	"		TASK_MANHACK_MOVEAT_SAVEPOSITION			5"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_GIVE_WAY"
	"		COND_HEAR_PLAYER"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_BULLET_IMPACT"
);


DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_MANHACK_SWARM_FAILURE"
	"		TASK_MANHACK_FIND_SQUAD_CENTER				0"
	"		TASK_MANHACK_MOVEAT_SAVEPOSITION			1"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM_FAILURE,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_WAIT									2"
	"		TASK_WAIT_RANDOM							2"
	"		TASK_MANHACK_FIND_SQUAD_MEMBER				0"
	"		TASK_GET_PATH_TO_SAVEPOSITION				0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts"
	"		COND_SEE_ENEMY"
	"		COND_NEW_ENEMY"
);

AI_END_CUSTOM_NPC()
