//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc_physicsflyer.h"
#include "weapon_physcannon.h"
#include "hl2_player.h"
#include "npc_scanner.h"
#include "IEffects.h"
#include "explode.h"
#include "ai_route.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	g_debug_basescanner( "g_debug_basescanner", "0", FCVAR_CHEAT );

BEGIN_DATADESC( CNPC_BaseScanner )
	DEFINE_EMBEDDED( m_KilledInfo ),
	DEFINE_SOUNDPATCH( m_pEngineSound ),

	DEFINE_FIELD( m_flFlyNoiseBase,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flEngineStallTime,		FIELD_TIME ),
	DEFINE_FIELD( m_fNextFlySoundTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nFlyMode,				FIELD_INTEGER ),

	DEFINE_FIELD( m_vecDiveBombDirection,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flDiveBombRollForce,	FIELD_FLOAT ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_FIELD( m_flGoalOverrideDistance,	FIELD_FLOAT ),

	DEFINE_FIELD( m_flAttackNearDist,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flAttackFarDist,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flAttackRange,	FIELD_FLOAT ),

	DEFINE_FIELD( m_nPoseTail,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseDynamo,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFlare,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFaceVert,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFaceHoriz,			FIELD_INTEGER ),

	// DEFINE_FIELD( m_bHasSpoken,			FIELD_BOOLEAN ),

	DEFINE_FIELD( m_pSmokeTrail,			FIELD_CLASSPTR ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDistanceOverride", InputSetDistanceOverride ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFlightSpeed", InputSetFlightSpeed ),

	DEFINE_THINKFUNC( DiveBombSoundThink ),
END_DATADESC()

ConVar	sk_scanner_dmg_dive( "sk_scanner_dmg_dive","0");

//-----------------------------------------------------------------------------
// Think contexts
//-----------------------------------------------------------------------------
static const char *s_pDiveBombSoundThinkContext = "DiveBombSoundThinkContext";

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_BaseScanner::CNPC_BaseScanner()
{
#ifdef _DEBUG
	m_vCurrentBanking.Init();
#endif
	m_pEngineSound = NULL;
	m_bHasSpoken = false;

	m_flAttackNearDist = SCANNER_ATTACK_NEAR_DIST;
	m_flAttackFarDist = SCANNER_ATTACK_FAR_DIST;
	m_flAttackRange = SCANNER_ATTACK_RANGE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::Spawn(void)
{
#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
	AddEffects( EF_NOSHADOW );
#endif // _XBOX

	SetHullType( HULL_TINY_CENTERED );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_VPHYSICS );

	m_bloodColor		= DONT_BLEED;
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;

	SetNavType( NAV_FLY );

	AddFlag( FL_FLY );

	// This entity cannot be dissolved by the combine balls,
	// nor does it get killed by the mega physcannon.
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL );

	m_flGoalOverrideDistance = 0.0f;

	m_nFlyMode = SCANNER_FLY_PATROL;
	AngleVectors( GetLocalAngles(), &m_vCurrentBanking );
	m_fHeadYaw = 0;
	m_pSmokeTrail = NULL;

	SetCurrentVelocity( vec3_origin );

	// Noise modifier
	Vector	bobAmount;
	bobAmount.x = random->RandomFloat( -2.0f, 2.0f );
	bobAmount.y = random->RandomFloat( -2.0f, 2.0f );
	bobAmount.z = random->RandomFloat( 2.0f, 4.0f );
	if ( random->RandomInt( 0, 1 ) )
	{
		bobAmount.z *= -1.0f;
	}
	SetNoiseMod( bobAmount );

	// set flight speed
	m_flSpeed = GetMaxSpeed();

	// --------------------------------------------

	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_SKIP_NAV_GROUND_CHECK );

	NPCInit();

	m_flFlyNoiseBase = random->RandomFloat( 0, M_PI );

	m_flNextAttack = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::UpdateEfficiency( bool bInPVS )	
{
	SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); 
	SetMoveEfficiency( AIME_NORMAL ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_BaseScanner::GetAutoAimRadius()
{ 
	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		return 24.0f;
	}

	return 12.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Called just before we are deleted.
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::UpdateOnRemove( void )
{
	// Stop combat loops if I'm alive. If I'm dead, the die sound will already have stopped it.
	if ( IsAlive() && m_bHasSpoken )
	{
		SentenceStop();
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_BaseScanner::SelectSchedule(void)
{
	// ----------------------------------------------------
	//  If I'm dead, go into a dive bomb
	// ----------------------------------------------------
	if ( m_iHealth <= 0 )
	{
		m_flSpeed = SCANNER_MAX_DIVE_BOMB_SPEED;
		return SCHED_SCANNER_ATTACK_DIVEBOMB;
	}

	// -------------------------------
	// If I'm in a script sequence
	// -------------------------------
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return(BaseClass::SelectSchedule());

	// -------------------------------
	// Flinch
	// -------------------------------
	if ( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) )
	{
		if ( IsHeldByPhyscannon( ) ) 
			return SCHED_SMALL_FLINCH;

		if ( m_NPCState == NPC_STATE_IDLE )
			return SCHED_SMALL_FLINCH;

		if ( m_NPCState == NPC_STATE_ALERT )
		{
			if ( m_iHealth < ( 3 * m_iMaxHealth / 4 ))
				return SCHED_TAKE_COVER_FROM_ORIGIN;

			if ( SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
				return SCHED_SMALL_FLINCH;
		}
		else
		{
			if ( random->RandomInt( 0, 10 ) < 4 )
				return SCHED_SMALL_FLINCH;
		}
	}

	// I'm being held by the physcannon... struggle!
	if ( IsHeldByPhyscannon( ) ) 
		return SCHED_SCANNER_HELD_BY_PHYSCANNON;

	// ----------------------------------------------------------
	//  If I have an enemy
	// ----------------------------------------------------------
	if ( GetEnemy() != NULL && GetEnemy()->IsAlive() )
	{
		// Patrol if the enemy has vanished
		if ( HasCondition( COND_LOST_ENEMY ) )
			return SCHED_SCANNER_PATROL;

		// Chase via route if we're directly blocked
		if ( HasCondition( COND_SCANNER_FLY_BLOCKED ) )
			return SCHED_SCANNER_CHASE_ENEMY;

		// Attack if it's time
		if ( gpGlobals->curtime >= m_flNextAttack )
		{
			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
				return SCHED_SCANNER_ATTACK;
		}

		// Otherwise fly in low for attack
		return SCHED_SCANNER_ATTACK_HOVER;
	}

	// Default to patrolling around
	return SCHED_SCANNER_PATROL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::OnScheduleChange( void )
{
	m_flSpeed = GetMaxSpeed();

	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
//-----------------------------------------------------------------------------
int CNPC_BaseScanner::MeleeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return COND_NONE;
	}

	// Check too far to attack with 2D distance
	float vEnemyDist2D = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length2D();

	if (m_flNextAttack > gpGlobals->curtime)
	{
		return COND_NONE;
	}
	else if (vEnemyDist2D > m_flAttackRange)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eOldState - 
//			eNewState - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::OnStateChange( NPC_STATE eOldState, NPC_STATE eNewState )
{
	if (( eNewState == NPC_STATE_ALERT ) || ( eNewState == NPC_STATE_COMBAT ))
	{
		SetPoseParameter(m_nPoseFlare, 1.0f);
	}
	else
	{
		SetPoseParameter(m_nPoseFlare, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_SCANNER_SET_FLY_PATROL:
		{
			// Fly in patrol mode and clear any
			// remaining target entity
			m_nFlyMode = SCANNER_FLY_PATROL;
			TaskComplete();
			break;
		}
	case TASK_SCANNER_SET_FLY_CHASE:
		{
			m_nFlyMode = SCANNER_FLY_CHASE;
			TaskComplete();
			break;
		}
	case TASK_SCANNER_SET_FLY_ATTACK:
		{
			m_nFlyMode = SCANNER_FLY_ATTACK;
			TaskComplete();
			break;
		}

	case TASK_SCANNER_SET_FLY_DIVE:
		{
			// Pick a direction to divebomb.
			if ( GetEnemy() != NULL )
			{
				// Fly towards my enemy
				Vector vEnemyPos = GetEnemyLKP();
				m_vecDiveBombDirection = vEnemyPos - GetLocalOrigin();
			}
			else
			{
				// Pick a random forward and down direction.
				Vector forward;
				GetVectors( &forward, NULL, NULL );
				m_vecDiveBombDirection = forward + Vector( random->RandomFloat( -10, 10 ), random->RandomFloat( -10, 10 ), random->RandomFloat( -20, -10 ) );
			}
			VectorNormalize( m_vecDiveBombDirection );

			// Calculate a roll force.
			m_flDiveBombRollForce = random->RandomFloat( 20.0, 420.0 );
			if ( random->RandomInt( 0, 1 ) )
			{
				m_flDiveBombRollForce *= -1;
			}

			DiveBombSoundThink();

			m_nFlyMode = SCANNER_FLY_DIVE;
			TaskComplete();
			break;
		}

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose: Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_BaseScanner::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Start smoking when we're nearly dead
	if ( m_iHealth < ( m_iMaxHealth - ( m_iMaxHealth / 4 ) ) )
	{
		StartSmokeTrail();
	}

	return (BaseClass::OnTakeDamage_Alive( info ));
}

//------------------------------------------------------------------------------
// Purpose: Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_BaseScanner::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// do the damage
	m_iHealth -= info.GetDamage();

	if ( m_iHealth < -40 )
	{
		Gib();
		return 1;
	}

	return VPhysicsTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Take damage from being thrown by a physcannon 
//-----------------------------------------------------------------------------
#define SCANNER_SMASH_SPEED 250.0	// How fast a scanner must slam into something to take full damage
void CNPC_BaseScanner::TakeDamageFromPhyscannon( CBasePlayer *pPlayer )
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

	float flFactor = flSpeed / SCANNER_SMASH_SPEED;

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
// Take damage from physics impacts
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::TakeDamageFromPhysicsImpact( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pHitEntity = pEvent->pEntities[!index];

	// NOTE: Augment the normal impact energy scale here.
	float flDamageScale = PlayerHasMegaPhysCannon() ? 10.0f : 5.0f;

	// Scale by the mapmaker's energyscale
	flDamageScale *= m_impactEnergyScale;

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
// Is the scanner being held?
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::IsHeldByPhyscannon( )
{
	return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD);
}

//------------------------------------------------------------------------------
// Physics impact
//------------------------------------------------------------------------------
#define SCANNER_SMASH_TIME	0.75		// How long after being thrown from a physcannon that a manhack is eligible to die from impact
void CNPC_BaseScanner::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	// Take no impact damage while being carried.
	if ( IsHeldByPhyscannon( ) )
		return;

	CBasePlayer *pPlayer = HasPhysicsAttacker( SCANNER_SMASH_TIME );
	if( pPlayer )
	{
		TakeDamageFromPhyscannon( pPlayer );
		return;
	}

	// It also can take physics damage from things thrown by the player.
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( pHitEntity )
	{
		if ( pHitEntity->HasPhysicsAttacker( 0.5f ) )
		{
			// It can take physics damage from things thrown by the player.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
		else if ( FClassnameIs( pHitEntity, "prop_combine_ball" ) )
		{
			// It also can take physics damage from a combine ball.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_BaseScanner::Gib( void )
{
	if ( IsMarkedForDeletion() )
		return;

	// Sparks
	for ( int i = 0; i < 4; i++ )
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0, &WorldSpaceCenter(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Cover the gib spawn
	ExplosionCreate( WorldSpaceCenter(), GetAbsAngles(), this, 64, 64, false );

	// Turn off any smoke trail
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_ParticleLifetime = 0;
		UTIL_Remove(m_pSmokeTrail);
		m_pSmokeTrail = NULL;
	}

	// FIXME: This is because we couldn't save/load the CTakeDamageInfo.
	// because it's midnight before the teamwide playtest. Real solution
	// is to add a datadesc to CTakeDamageInfo
	if ( m_KilledInfo.GetInflictor() )
	{
		BaseClass::Event_Killed( m_KilledInfo );
	}

	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//			bPunting - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	if ( reason == PUNTED_BY_CANNON )
	{
		// There's about to be a massive change in velocity. 
		// Think immediately to handle changes in m_vCurrentVelocity;
		SetNextThink( gpGlobals->curtime + 0.01f );

		m_flEngineStallTime = gpGlobals->curtime + 2.0f;
		ScannerEmitSound( "DiveBomb" );
	}
	else
	{
		SetCondition( COND_SCANNER_GRABBED_BY_PHYSCANNON );
		ClearCondition( COND_SCANNER_RELEASED_FROM_PHYSCANNON );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	ClearCondition( COND_SCANNER_GRABBED_BY_PHYSCANNON );
	SetCondition( COND_SCANNER_RELEASED_FROM_PHYSCANNON );

	if ( Reason == LAUNCHED_BY_CANNON )
	{
		m_flEngineStallTime = gpGlobals->curtime + 2.0f;

		// There's about to be a massive change in velocity. 
		// Think immediately to handle changes in m_vCurrentVelocity;
		SetNextThink( gpGlobals->curtime + 0.01f );
		ScannerEmitSound( "DiveBomb" );
	}
}


//------------------------------------------------------------------------------
// Do we have a physics attacker?
//------------------------------------------------------------------------------
CBasePlayer *CNPC_BaseScanner::HasPhysicsAttacker( float dt )
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if ( IsHeldByPhyscannon( ) || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime) )
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_BaseScanner::StopLoopingSounds(void)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pEngineSound );
	m_pEngineSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::Event_Killed( const CTakeDamageInfo &info )
{
	// Copy off the takedamage info that killed me, since we're not going to call
	// up into the base class's Event_Killed() until we gib. (gibbing is ultimate death)
	m_KilledInfo = info;	

	// Interrupt whatever schedule I'm on
	SetCondition(COND_SCHEDULE_DONE);

	// If I have an enemy and I'm up high, do a dive bomb (unless dissolved)
	if ( GetEnemy() != NULL && (info.GetDamageType() & DMG_DISSOLVE) == false )
	{
		Vector vecDelta = GetLocalOrigin() - GetEnemy()->GetLocalOrigin();
		if ( ( vecDelta.z > 120 ) && ( vecDelta.Length() > 360 ) )
		{	
			// If I'm divebombing, don't take any more damage. It will make Event_Killed() be called again.
			// This is especially bad if someone machineguns the divebombing scanner. 
			AttackDivebomb();
			return;
		}
	}

	Gib();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_BaseScanner::AttackDivebomb( void )
{
	ScannerEmitSound( "DiveBomb" );

	m_takedamage = DAMAGE_NO;

	StartSmokeTrail();
}

//------------------------------------------------------------------------------
// Purpose: Checks to see if we hit anything while dive bombing.
//------------------------------------------------------------------------------
void CNPC_BaseScanner::AttackDivebombCollide(float flInterval)
{
	//
	// Trace forward to see if I hit anything
	//
	Vector			checkPos = GetAbsOrigin() + (GetCurrentVelocity() * flInterval);
	trace_t			tr;
	CBaseEntity*	pHitEntity = NULL;
	AI_TraceHull( GetAbsOrigin(), checkPos, GetHullMins(), GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if (tr.m_pEnt)
	{
		pHitEntity = tr.m_pEnt;

		// Did I hit an entity that isn't another scanner?
		if (pHitEntity && pHitEntity->Classify()!=CLASS_SCANNER)
		{
			if ( !pHitEntity->ClassMatches("item_battery") )
			{
				if ( !pHitEntity->IsWorld() )
				{
					CTakeDamageInfo info( this, this, sk_scanner_dmg_dive.GetFloat(), DMG_CLUB );
					CalculateMeleeDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
					pHitEntity->TakeDamage( info );
				}
				Gib();
			}
		}
	}

	if (tr.fraction != 1.0)
	{
		// We've hit something so deflect our velocity based on the surface
		// norm of what we've hit
		if (flInterval > 0)
		{
			float moveLen	= (1.0 - tr.fraction)*(GetAbsOrigin() - checkPos).Length();
			Vector vBounceVel	= moveLen*tr.plane.normal/flInterval;

			// If I'm right over the ground don't push down
			if (vBounceVel.z < 0)
			{
				float floorZ = GetFloorZ(GetAbsOrigin());
				if (abs(GetAbsOrigin().z - floorZ) < 36)
				{
					vBounceVel.z = 0;
				}
			}
			SetCurrentVelocity( GetCurrentVelocity() + vBounceVel );
		}
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHitEntity );

		if (pBCC)
		{
			// Spawn some extra blood where we hit
			SpawnBlood(tr.endpos, g_vecAttackDir, pBCC->BloodColor(), sk_scanner_dmg_dive.GetFloat());
		}
		else
		{
			if (!(m_spawnflags	& SF_NPC_GAG))
			{
				// <<TEMP>> need better sound here...
				ScannerEmitSound( "Shoot" );
			}
			// For sparks we must trace a line in the direction of the surface norm
			// that we hit.
			checkPos = GetAbsOrigin() - (tr.plane.normal * 24);

			AI_TraceLine( GetAbsOrigin(), checkPos,MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if (tr.fraction != 1.0)
			{
				g_pEffects->Sparks( tr.endpos );

				CBroadcastRecipientFilter filter;
				te->DynamicLight( filter, 0.0,
					&GetAbsOrigin(), 255, 180, 100, 0, 50, 0.1, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::PlayFlySound(void)
{
	if ( IsMarkedForDeletion() )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	//Setup the sound if we're not already
	if ( m_pEngineSound == NULL )
	{
		// Create the sound
		CPASAttenuationFilter filter( this );

		m_pEngineSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, GetEngineSound(), ATTN_NORM );

		Assert(m_pEngineSound);

		// Start the engine sound
		controller.Play( m_pEngineSound, 0.0f, 100.0f );
		controller.SoundChangeVolume( m_pEngineSound, 1.0f, 2.0f );
	}

	float	speed	 = GetCurrentVelocity().Length();
	float	flVolume = 0.25f + (0.75f*(speed/GetMaxSpeed()));
	int		iPitch	 = MIN( 255, 80 + (20*(speed/GetMaxSpeed())) );

	//Update our pitch and volume based on our speed
	controller.SoundChangePitch( m_pEngineSound, iPitch, 0.1f );
	controller.SoundChangeVolume( m_pEngineSound, flVolume, 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::ScannerEmitSound( const char *pszSoundName )
{
	CFmtStr snd;
	snd.sprintf("%s.%s", GetScannerSoundPrefix(), pszSoundName );

	m_bHasSpoken = true;

	EmitSound( snd.Access() );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_BaseScanner::SpeakSentence( int sentenceType )
{
	if (sentenceType == SCANNER_SENTENCE_ATTENTION)
	{
		ScannerEmitSound( "Attention" );
	}
	else if (sentenceType == SCANNER_SENTENCE_HANDSUP)
	{
		ScannerEmitSound( "Scan" );
	}
	else if (sentenceType == SCANNER_SENTENCE_PROCEED)
	{
		ScannerEmitSound( "Proceed" );
	}
	else if (sentenceType == SCANNER_SENTENCE_CURIOUS)
	{
		ScannerEmitSound( "Curious" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::InputSetFlightSpeed(inputdata_t &inputdata)
{
	//FIXME: Currently unsupported

	/*
	m_flFlightSpeed = inputdata.value.Int();
	m_bFlightSpeedOverridden = (m_flFlightSpeed > 0);
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::StartSmokeTrail( void )
{
	if ( m_pSmokeTrail != NULL )
		return;

	m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();

	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_SpawnRate = 10;
		m_pSmokeTrail->m_ParticleLifetime = 1;
		m_pSmokeTrail->m_StartSize		= 8;
		m_pSmokeTrail->m_EndSize		= 50;
		m_pSmokeTrail->m_SpawnRadius	= 10;
		m_pSmokeTrail->m_MinSpeed		= 15;
		m_pSmokeTrail->m_MaxSpeed		= 25;

		m_pSmokeTrail->m_StartColor.Init( 0.5f, 0.5f, 0.5f );
		m_pSmokeTrail->m_EndColor.Init( 0, 0, 0 );
		m_pSmokeTrail->SetLifetime( 500.0f );
		m_pSmokeTrail->FollowEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::BlendPhyscannonLaunchSpeed()
{
	// Blend out desired velocity when launched by the physcannon
	if (!VPhysicsGetObject())
		return;

	if ( HasPhysicsAttacker( SCANNER_SMASH_TIME ) && !IsHeldByPhyscannon( ) )
	{
		Vector vecCurrentVelocity;
		VPhysicsGetObject()->GetVelocity( &vecCurrentVelocity, NULL );
		float flLerpFactor = (gpGlobals->curtime - m_flLastPhysicsInfluenceTime) / SCANNER_SMASH_TIME;
		flLerpFactor = clamp( flLerpFactor, 0.0f, 1.0f );
		flLerpFactor = SimpleSplineRemapVal( flLerpFactor, 0.0f, 1.0f, 0.0f, 1.0f );
		flLerpFactor *= flLerpFactor;
		VectorLerp( vecCurrentVelocity, m_vCurrentVelocity, flLerpFactor, m_vCurrentVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::MoveExecute_Alive(float flInterval)
{
	// Amount of noise to add to flying
	float noiseScale = 3.0f;

	// -------------------------------------------
	//  Avoid obstacles, unless I'm dive bombing
	// -------------------------------------------
	if (m_nFlyMode != SCANNER_FLY_DIVE)
	{
		SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );
	}
	// If I am dive bombing add more noise to my flying
	else
	{
		AttackDivebombCollide(flInterval);
		noiseScale *= 4;
	}

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	if ( pPhysics && pPhysics->IsAsleep() )
	{
		pPhysics->Wake();
	}

	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	AddNoiseToVelocity( noiseScale );

	AdjustScannerVelocity();

	float maxSpeed = GetEnemy() ? ( GetMaxSpeed() * 2.0f ) : GetMaxSpeed();
	if ( m_nFlyMode == SCANNER_FLY_DIVE )
	{
		maxSpeed = -1;
	}

	// Limit fall speed
	LimitSpeed( maxSpeed );

	// Blend out desired velocity when launched by the physcannon
	BlendPhyscannonLaunchSpeed();

	// Update what we're looking at
	UpdateHead( flInterval );

	// Control the tail based on our vertical travel
	float tailPerc = clamp( GetCurrentVelocity().z, -150, 250 );
	tailPerc = SimpleSplineRemapVal( tailPerc, -150, 250, -25, 80 );

	SetPoseParameter( m_nPoseTail, tailPerc );

	// Spin the dynamo based upon our speed
	float flCurrentDynamo = GetPoseParameter( m_nPoseDynamo );
	float speed	= GetCurrentVelocity().Length();
	float flDynamoSpeed = (maxSpeed > 0 ? speed / maxSpeed : 1.0) * 60;
	flCurrentDynamo -= flDynamoSpeed;
	if ( flCurrentDynamo < -180.0 )
	{
		flCurrentDynamo += 360.0;
	}
	SetPoseParameter( m_nPoseDynamo, flCurrentDynamo );

	PlayFlySound();
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::OverridePathMove( CBaseEntity *pMoveTarget, float flInterval )
{
	// Save our last patrolling direction
	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetAbsOrigin();

	// Continue on our path
	if ( ProgressFlyPath( flInterval, pMoveTarget, (MASK_NPCSOLID|CONTENTS_WATER), false, 64 ) == AINPP_COMPLETE )
	{
		if ( IsCurSchedule( SCHED_SCANNER_PATROL ) )
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::OverrideMove( float flInterval )
{
	// ----------------------------------------------
	//	If dive bombing
	// ----------------------------------------------
	if (m_nFlyMode == SCANNER_FLY_DIVE)
	{
		MoveToDivebomb( flInterval );
	}
	else
	{
		Vector vMoveTargetPos(0,0,0);
		CBaseEntity *pMoveTarget = NULL;

		// The original line of code was, due to the accidental use of '|' instead of
		// '&', always true. Replacing with 'true' to suppress the warning without changing
		// the (long-standing) behavior.
		if ( true ) //!GetNavigator()->IsGoalActive() || ( GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER ) )
		{
			// Select move target 
			if ( GetTarget() != NULL )
			{
				pMoveTarget = GetTarget();
			}
			else if ( GetEnemy() != NULL )
			{
				pMoveTarget = GetEnemy();
			}

			// Select move target position 
			if ( GetEnemy() != NULL )
			{
				vMoveTargetPos = GetEnemy()->GetAbsOrigin();
			}
		}
		else
		{
			vMoveTargetPos = GetNavigator()->GetCurWaypointPos();
		}

		ClearCondition( COND_SCANNER_FLY_CLEAR );
		ClearCondition( COND_SCANNER_FLY_BLOCKED );

		// See if we can fly there directly
		if ( pMoveTarget )
		{
			trace_t tr;
			AI_TraceHull( GetAbsOrigin(), vMoveTargetPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

			float fTargetDist = (1.0f-tr.fraction)*(GetAbsOrigin() - vMoveTargetPos).Length();

			if ( ( tr.m_pEnt == pMoveTarget ) || ( fTargetDist < 50 ) )
			{
				if ( g_debug_basescanner.GetBool() )
				{
					NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 0,255,0, true, 0);
					NDebugOverlay::Cross3D(tr.endpos,Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
				}

				SetCondition( COND_SCANNER_FLY_CLEAR );
			}
			else		
			{
				//HANDY DEBUG TOOL	
				if ( g_debug_basescanner.GetBool() )
				{
					NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 255,0,0, true, 0);
					NDebugOverlay::Cross3D(tr.endpos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
				}

				SetCondition( COND_SCANNER_FLY_BLOCKED );
			}
		}

		// If I have a route, keep it updated and move toward target
		if ( GetNavigator()->IsGoalActive() )
		{
			if ( OverridePathMove( pMoveTarget, flInterval ) )
			{
				BlendPhyscannonLaunchSpeed();
				return true;
			}
		}	
		// ----------------------------------------------
		//	If attacking
		// ----------------------------------------------
		else if (m_nFlyMode == SCANNER_FLY_ATTACK)
		{
			MoveToAttack( flInterval );
		}
		// -----------------------------------------------------------------
		// If I don't have a route, just decelerate
		// -----------------------------------------------------------------
		else if (!GetNavigator()->IsGoalActive())
		{
			float	myDecay	 = 9.5;
			Decelerate( flInterval, myDecay);
		}
	}

	MoveExecute_Alive( flInterval );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &goalPos - 
//			&startPos - 
//			idealRange - 
//			idealHeight - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_BaseScanner::IdealGoalForMovement( const Vector &goalPos, const Vector &startPos, float idealRange, float idealHeightDiff )
{
	Vector	vMoveDir;

	if ( GetGoalDirection( &vMoveDir ) == false )
	{
		vMoveDir = ( goalPos - startPos );
		vMoveDir.z = 0;
		VectorNormalize( vMoveDir );
	}

	// Move up from the position by the desired amount
	Vector vIdealPos = goalPos + Vector( 0, 0, idealHeightDiff ) + ( vMoveDir * -idealRange );

	// Trace down and make sure we can fit here
	trace_t	tr;
	AI_TraceHull( vIdealPos, vIdealPos - Vector( 0, 0, MinGroundDist() ), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// Move up otherwise
	if ( tr.fraction < 1.0f )
	{
		vIdealPos.z += ( MinGroundDist() * ( 1.0f - tr.fraction ) );
	}

	//FIXME: We also need to make sure that we fit here at all, and if not, chose a new spot

	// Debug tools
	if ( g_debug_basescanner.GetBool() )
	{
		NDebugOverlay::Cross3D( goalPos, -Vector(8,8,8), Vector(8,8,8), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( startPos, -Vector(8,8,8), Vector(8,8,8), 255, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( vIdealPos, -Vector(8,8,8), Vector(8,8,8), 255, 255, 255, true, 0.1f );
		NDebugOverlay::Line( startPos, goalPos, 0, 255, 0, true, 0.1f );

		NDebugOverlay::Cross3D( goalPos + ( vMoveDir * -idealRange ), -Vector(8,8,8), Vector(8,8,8), 255, 255, 255, true, 0.1f );
		NDebugOverlay::Line( goalPos, goalPos + ( vMoveDir * -idealRange ), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Line( goalPos + ( vMoveDir * -idealRange ), vIdealPos, 255, 255, 0, true, 0.1f );
	}

	return vIdealPos;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::MoveToAttack(float flInterval)
{
	if (GetEnemy() == NULL)
		return;

	if ( flInterval <= 0 )
		return;

	Vector vTargetPos = GetEnemyLKP();

	//float flDesiredDist = m_flAttackNearDist + ( ( m_flAttackFarDist - m_flAttackNearDist ) / 2 );

	Vector idealPos = IdealGoalForMovement( vTargetPos, GetAbsOrigin(), GetGoalDistance(), m_flAttackNearDist );

	MoveToTarget( flInterval, idealPos );

	//FIXME: Re-implement?

	/*
	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
	vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Accelerates toward a given position.
// Input  : flInterval - Time interval over which to move.
//			vecMoveTarget - Position to move toward.
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::MoveToTarget( float flInterval, const Vector &vecMoveTarget )
{
	// Don't move if stalling
	if ( m_flEngineStallTime > gpGlobals->curtime )
		return;

	// Look at our inspection target if we have one
	if ( GetEnemy() != NULL )
	{
		// Otherwise at our enemy
		TurnHeadToTarget( flInterval, GetEnemy()->EyePosition() );
	}
	else
	{
		// Otherwise face our motion direction
		TurnHeadToTarget( flInterval, vecMoveTarget );
	}

	// -------------------------------------
	// Move towards our target
	// -------------------------------------
	float myAccel;
	float myZAccel = 400.0f;
	float myDecay  = 0.15f;

	Vector vecCurrentDir;

	// Get the relationship between my current velocity and the way I want to be going.
	vecCurrentDir = GetCurrentVelocity();
	VectorNormalize( vecCurrentDir );

	Vector targetDir = vecMoveTarget - GetAbsOrigin();
	float flDist = VectorNormalize(targetDir);

	float flDot;
	flDot = DotProduct( targetDir, vecCurrentDir );

	if( flDot > 0.25 )
	{
		// If my target is in front of me, my flight model is a bit more accurate.
		myAccel = 250;
	}
	else
	{
		// Have a harder time correcting my course if I'm currently flying away from my target.
		myAccel = 128;
	}

	if ( myAccel > flDist / flInterval )
	{
		myAccel = flDist / flInterval;
	}

	if ( myZAccel > flDist / flInterval )
	{
		myZAccel = flDist / flInterval;
	}

	MoveInDirection( flInterval, targetDir, myAccel, myZAccel, myDecay );

	// calc relative banking targets
	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	m_vCurrentBanking.x	= targetDir.x;
	m_vCurrentBanking.z	= 120.0f * DotProduct( right, targetDir );
	m_vCurrentBanking.y	= 0;

	float speedPerc = SimpleSplineRemapVal( GetCurrentVelocity().Length(), 0.0f, GetMaxSpeed(), 0.0f, 1.0f );

	speedPerc = clamp( speedPerc, 0.0f, 1.0f );

	m_vCurrentBanking *= speedPerc;
}

//-----------------------------------------------------------------------------
// Danger sounds. 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::DiveBombSoundThink()
{
	Vector vecPosition, vecVelocity;
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if ( pPhysicsObject == NULL )
		return;

	pPhysicsObject->GetPosition( &vecPosition, NULL );
	pPhysicsObject->GetVelocity( &vecVelocity, NULL );

	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer )
	{
		Vector vecDelta;
		VectorSubtract( pPlayer->GetAbsOrigin(), vecPosition, vecDelta );
		VectorNormalize( vecDelta );
		if ( DotProduct( vecDelta, vecVelocity ) > 0.5f )
		{
			Vector vecEndPoint;
			VectorMA( vecPosition, 2.0f * TICK_INTERVAL, vecVelocity, vecEndPoint );
			float flDist = CalcDistanceToLineSegment( pPlayer->GetAbsOrigin(), vecPosition, vecEndPoint );
			if ( flDist < 200.0f )
			{
				ScannerEmitSound( "DiveBombFlyby" );
				SetContextThink( &CNPC_BaseScanner::DiveBombSoundThink, gpGlobals->curtime + 0.5f, s_pDiveBombSoundThinkContext );
				return;
			}
		}
	}

	SetContextThink( &CNPC_BaseScanner::DiveBombSoundThink, gpGlobals->curtime + 2.0f * TICK_INTERVAL, s_pDiveBombSoundThinkContext );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::MoveToDivebomb(float flInterval)
{
	float myAccel = 1600;
	float myDecay = 0.05f; // decay current velocity to 10% in 1 second

	// Fly towards my enemy
	Vector vEnemyPos = GetEnemyLKP();
	Vector vFlyDirection  = vEnemyPos - GetLocalOrigin();
	VectorNormalize( vFlyDirection );

	// Set net velocity 
	MoveInDirection( flInterval, m_vecDiveBombDirection, myAccel, myAccel, myDecay);

	// Spin out of control.
	Vector forward;
	VPhysicsGetObject()->LocalToWorldVector( &forward, Vector( 1.0, 0.0, 0.0 ) );
	AngularImpulse torque = forward * m_flDiveBombRollForce;
	VPhysicsGetObject()->ApplyTorqueCenter( torque );

	// BUGBUG: why Y axis and not Z?
	Vector up;
	VPhysicsGetObject()->LocalToWorldVector( &up, Vector( 0.0, 1.0, 0.0 ) );
	VPhysicsGetObject()->ApplyForceCenter( up * 2000 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::IsEnemyPlayerInSuit()
{
	if( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		CHL2_Player *pPlayer = NULL;
		pPlayer = (CHL2_Player *)GetEnemy();

		if( pPlayer && pPlayer->IsSuitEquipped() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_BaseScanner::GetGoalDistance( void )
{
	if ( m_flGoalOverrideDistance != 0.0f )
		return m_flGoalOverrideDistance;

	switch ( m_nFlyMode )
	{
	case SCANNER_FLY_ATTACK:
		{
			float goalDist = ( m_flAttackNearDist + ( ( m_flAttackFarDist - m_flAttackNearDist ) / 2 ) );
			if( IsEnemyPlayerInSuit() )
			{
				goalDist *= 0.5;
			}
			return goalDist;
		}
		break;
	}

	return 128.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vOut - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::GetGoalDirection( Vector *vOut )
{
	CBaseEntity *pTarget = GetTarget();

	if ( pTarget == NULL )
		return false;

	if ( FClassnameIs( pTarget, "info_hint_air" ) || FClassnameIs( pTarget, "info_target" ) )
	{
		AngleVectors( pTarget->GetAbsAngles(), vOut );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CNPC_BaseScanner::VelocityToEvade(CBaseCombatCharacter *pEnemy)
{
	if (pEnemy)
	{
		// -----------------------------------------
		//  Keep out of enemy's shooting position
		// -----------------------------------------
		Vector vEnemyFacing = pEnemy->BodyDirection2D( );
		Vector	vEnemyDir   = pEnemy->EyePosition() - GetLocalOrigin();
		VectorNormalize(vEnemyDir);
		float  fDotPr		= DotProduct(vEnemyFacing,vEnemyDir);

		if (fDotPr < -0.9)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (crossProduct.y < 0)
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
		else if (fDotPr < -0.85)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (random->RandomInt(0,1))
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
	}
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_BaseScanner::DrawDebugTextOverlays(void)
{
	int nOffset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT ) 
	{
		Vector vel;
		GetVelocity( &vel, NULL );

		char tempstr[512];
		Q_snprintf( tempstr, sizeof(tempstr), "speed (max): %.2f (%.2f)", vel.Length(), m_flSpeed );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;
	}

	return nOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_BaseScanner::GetHeadTurnRate( void ) 
{ 
	if ( GetEnemy() )
		return 800.0f;

	return 350.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline CBaseEntity *CNPC_BaseScanner::EntityToWatch( void )
{
	return ( GetTarget() != NULL ) ? GetTarget() : GetEnemy();	// Okay if NULL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::UpdateHead( float flInterval )
{
	float yaw = GetPoseParameter( m_nPoseFaceHoriz );
	float pitch = GetPoseParameter( m_nPoseFaceVert );

	CBaseEntity *pTarget = EntityToWatch();

	Vector	vLookPos;

	if ( !HasCondition( COND_IN_PVS ) || GetAttachment( "eyes", vLookPos ) == false )
	{
		vLookPos = EyePosition();
	}

	if ( pTarget != NULL )
	{
		Vector	lookDir = pTarget->EyePosition() - vLookPos;
		VectorNormalize( lookDir );

		if ( DotProduct( lookDir, BodyDirection3D() ) < 0.0f )
		{
			SetPoseParameter( m_nPoseFaceHoriz,	UTIL_Approach( 0, yaw, 10 ) );
			SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( 0, pitch, 10 ) );

			return;
		}

		float facingYaw = VecToYaw( BodyDirection3D() );
		float yawDiff = VecToYaw( lookDir );
		yawDiff = UTIL_AngleDiff( yawDiff, facingYaw + yaw );

		float facingPitch = UTIL_VecToPitch( BodyDirection3D() );
		float pitchDiff = UTIL_VecToPitch( lookDir );
		pitchDiff = UTIL_AngleDiff( pitchDiff, facingPitch + pitch );

		SetPoseParameter( m_nPoseFaceHoriz, UTIL_Approach( yaw + yawDiff, yaw, 50 ) );
		SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( pitch + pitchDiff, pitch, 50 ) );
	}
	else
	{
		SetPoseParameter( m_nPoseFaceHoriz,	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( 0, pitch, 10 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &linear - 
//			&angular - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::ClampMotorForces( Vector &linear, AngularImpulse &angular )
{ 
	// limit reaction forces
	if ( m_nFlyMode != SCANNER_FLY_DIVE )
	{
		linear.x = clamp( linear.x, -500, 500 );
		linear.y = clamp( linear.y, -500, 500 );
		linear.z = clamp( linear.z, -500, 500 );
	}

	// If we're dive bombing, we need to drop faster than normal
	if ( m_nFlyMode != SCANNER_FLY_DIVE )
	{
		// Add in weightlessness
		linear.z += 800;
	}

	angular.z = clamp( angular.z, -GetHeadTurnRate(), GetHeadTurnRate() );
	if ( m_nFlyMode == SCANNER_FLY_DIVE )
	{
		// Disable pitch and roll motors while crashing.
		angular.x = 0;
		angular.y = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::InputSetDistanceOverride( inputdata_t &inputdata )
{
	m_flGoalOverrideDistance = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: Emit sounds specific to the NPC's state.
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::AlertSound(void)
{
	ScannerEmitSound( "Alert" );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_BaseScanner::DeathSound( const CTakeDamageInfo &info )
{
	ScannerEmitSound( "Die" );
}

//-----------------------------------------------------------------------------
// Purpose: Overridden so that scanners play battle sounds while fighting.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseScanner::ShouldPlayIdleSound( void )
{
	if ( HasSpawnFlags( SF_NPC_GAG ) )
		return false;

	if ( random->RandomInt( 0, 25 ) != 0 )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Plays sounds while idle or in combat.
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::IdleSound(void)
{
	if ( m_NPCState == NPC_STATE_COMBAT )
	{
		// dvs: the combat sounds should be related to what is happening, rather than random
		ScannerEmitSound( "Combat" );
	}
	else
	{
		ScannerEmitSound( "Idle" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Plays a sound when hurt.
//-----------------------------------------------------------------------------
void CNPC_BaseScanner::PainSound( const CTakeDamageInfo &info )
{
	ScannerEmitSound( "Pain" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_BaseScanner::GetMaxSpeed()
{
	return SCANNER_MAX_SPEED;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_basescanner, CNPC_BaseScanner )

	DECLARE_TASK( TASK_SCANNER_SET_FLY_PATROL )
	DECLARE_TASK( TASK_SCANNER_SET_FLY_CHASE )
	DECLARE_TASK( TASK_SCANNER_SET_FLY_ATTACK )
	DECLARE_TASK( TASK_SCANNER_SET_FLY_DIVE )

	DECLARE_CONDITION(COND_SCANNER_FLY_CLEAR)
	DECLARE_CONDITION(COND_SCANNER_FLY_BLOCKED)
	DECLARE_CONDITION(COND_SCANNER_RELEASED_FROM_PHYSCANNON)
	DECLARE_CONDITION(COND_SCANNER_GRABBED_BY_PHYSCANNON)

	//=========================================================
	// > SCHED_SCANNER_PATROL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PATROL,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_PATROL			0"
		"		TASK_SET_TOLERANCE_DISTANCE			32"
		"		TASK_SET_ROUTE_SEARCH_TIME			5"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_GET_PATH_TO_RANDOM_NODE		2000"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_PLAYER"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK
	//
	//	This task does nothing. Translate it in your derived
	//	class to perform your attack.
	//
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_DIVEBOMB
	//
	// Only done when scanner is dead
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_DIVEBOMB,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_DIVE			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							10"
		""
		"	Interrupts"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_ENEMY
	//
	//  Different interrupts than normal chase enemy.  
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCANNER_PATROL"
		"		 TASK_SET_TOLERANCE_DISTANCE		120"
		"		 TASK_GET_PATH_TO_ENEMY				0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_TARGET,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_TOLERANCE_DISTANCE		64"
		"		 TASK_GET_PATH_TO_TARGET			0"	//FIXME: This is wrong!
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_FOLLOW_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_FOLLOW_HOVER,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_HELD_BY_PHYSCANNON
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_HELD_BY_PHYSCANNON,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							5.0"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SCANNER_RELEASED_FROM_PHYSCANNON"
	)
	
AI_END_CUSTOM_NPC()
