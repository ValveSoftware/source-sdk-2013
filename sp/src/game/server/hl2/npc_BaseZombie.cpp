//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the zombie, a horrific once-human headcrab victim.
//
// The zombie has two main states: Full and Torso.
//
// In Full state, the zombie is whole and walks upright as he did in Half-Life.
// He will try to claw the player and swat physics items at him. 
//
// In Torso state, the zombie has been blasted or cut in half, and the Torso will
// drag itself along the ground with its arms. It will try to claw the player.
//
// In either state, a severely injured Zombie will release its headcrab, which
// will immediately go after the player. The Zombie will then die (ragdoll). 
//
//=============================================================================//

#include "cbase.h"
#include "npc_BaseZombie.h"
#include "player.h"
#include "game.h"
#include "ai_network.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_memory.h"
#include "ai_senses.h"
#include "bitstring.h"
#include "EntityFlame.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "activitylist.h"
#include "entitylist.h"
#include "gib.h"
#include "soundenvelope.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "hl2_gamerules.h"
#include "weapon_physcannon.h"
#include "ammodef.h"
#include "vehicle_base.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_npc_head;

#define ZOMBIE_BULLET_DAMAGE_SCALE 0.5f

int g_interactionZombieMeleeWarning;

envelopePoint_t envDefaultZombieMoanVolumeFast[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envDefaultZombieMoanVolume[] =
{
	{	1.0f, 0.1f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};


// if the zombie doesn't find anything closer than this, it doesn't swat.
#define ZOMBIE_FARTHEST_PHYSICS_OBJECT	40.0*12.0
#define ZOMBIE_PHYSICS_SEARCH_DEPTH	100

// Don't swat objects unless player is closer than this.
#define ZOMBIE_PLAYER_MAX_SWAT_DIST		1000

//
// How much health a Zombie torso gets when a whole zombie is broken
// It's whole zombie's MAX Health * this value
#define ZOMBIE_TORSO_HEALTH_FACTOR 0.5

//
// When the zombie has health < m_iMaxHealth * this value, it will
// try to release its headcrab.
#define ZOMBIE_RELEASE_HEALTH_FACTOR	0.5

//
// The heaviest physics object that a zombie should try to swat. (kg)
#define ZOMBIE_MAX_PHYSOBJ_MASS		60

//
// Zombie tries to get this close to a physics object's origin to swat it
#define ZOMBIE_PHYSOBJ_SWATDIST		80

//
// Because movement code sometimes doesn't get us QUITE where we
// want to go, the zombie tries to get this close to a physics object
// Zombie will end up somewhere between PHYSOBJ_MOVE_TO_DIST & PHYSOBJ_SWATDIST
#define ZOMBIE_PHYSOBJ_MOVE_TO_DIST	48

//
// How long between physics swat attacks (in seconds). 
#define ZOMBIE_SWAT_DELAY			5


//
// After taking damage, ignore further damage for n seconds. This keeps the zombie
// from being interrupted while.
//
#define ZOMBIE_FLINCH_DELAY			3


#define ZOMBIE_BURN_TIME		10 // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE	2  // Give or take this many seconds.


//=========================================================
// private activities
//=========================================================
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTLOW;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTLOW;
int CNPC_BaseZombie::ACT_ZOM_RELEASECRAB;
int CNPC_BaseZombie::ACT_ZOM_FALL;

ConVar	sk_zombie_dmg_one_slash( "sk_zombie_dmg_one_slash","0");
ConVar	sk_zombie_dmg_both_slash( "sk_zombie_dmg_both_slash","0");


// When a zombie spawns, he will select a 'base' pitch value
// that's somewhere between basepitchmin & basepitchmax
ConVar zombie_basemin( "zombie_basemin", "100" );
ConVar zombie_basemax( "zombie_basemax", "100" );

ConVar zombie_changemin( "zombie_changemin", "0" );
ConVar zombie_changemax( "zombie_changemax", "0" );

// play a sound once in every zombie_stepfreq steps
ConVar zombie_stepfreq( "zombie_stepfreq", "4" );
ConVar zombie_moanfreq( "zombie_moanfreq", "1" );

ConVar zombie_decaymin( "zombie_decaymin", "0.1" );
ConVar zombie_decaymax( "zombie_decaymax", "0.4" );

ConVar zombie_ambushdist( "zombie_ambushdist", "16000" );

#ifdef MAPBASE
ConVar	zombie_no_flinch_during_unique_anim( "zombie_no_flinch_during_unique_anim", "1", FCVAR_NONE, "Prevents zombies from flinching during actbusies and scripted sequences." );
#endif

//=========================================================
// For a couple of reasons, we keep a running count of how
// many zombies in the world are angry at any given time.
//=========================================================
static int s_iAngryZombies = 0;

//=========================================================
//=========================================================
class CAngryZombieCounter : public CAutoGameSystem
{
public:
	CAngryZombieCounter( char const *name ) : CAutoGameSystem( name )
	{
	}
	// Level init, shutdown
	virtual void LevelInitPreEntity()
	{
		s_iAngryZombies = 0;
	}
};

CAngryZombieCounter	AngryZombieCounter( "CAngryZombieCounter" );


int AE_ZOMBIE_ATTACK_RIGHT;
int AE_ZOMBIE_ATTACK_LEFT;
int AE_ZOMBIE_ATTACK_BOTH;
int AE_ZOMBIE_SWATITEM;
int AE_ZOMBIE_STARTSWAT;
int AE_ZOMBIE_STEP_LEFT;
int AE_ZOMBIE_STEP_RIGHT;
int AE_ZOMBIE_SCUFF_LEFT;
int AE_ZOMBIE_SCUFF_RIGHT;
int AE_ZOMBIE_ATTACK_SCREAM;
int AE_ZOMBIE_GET_UP;
int AE_ZOMBIE_POUND;
int AE_ZOMBIE_ALERTSOUND;
int AE_ZOMBIE_POPHEADCRAB;


//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_BaseZombie )

	DEFINE_SOUNDPATCH( m_pMoanSound ),
	DEFINE_FIELD( m_fIsTorso, FIELD_BOOLEAN ),
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_fIsHeadless, FIELD_BOOLEAN, "Headless" ),
#else
	DEFINE_FIELD( m_fIsHeadless, FIELD_BOOLEAN ),
#endif
	DEFINE_FIELD( m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( m_bHeadShot, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flBurnDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBurnDamageResetTime, FIELD_TIME ),
	DEFINE_FIELD( m_hPhysicsEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextMoanSound, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSwat, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSwatScan, FIELD_TIME ),
	DEFINE_FIELD( m_crabHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMoanPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_iMoanSound, FIELD_INTEGER ),
	DEFINE_FIELD( m_hObstructor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bIsSlumped, FIELD_BOOLEAN ),

#ifdef MAPBASE
	DEFINE_OUTPUT( m_OnSwattedProp, "OnSwattedProp" ),
	DEFINE_OUTPUT( m_OnCrab, "OnCrab" ),
#endif

END_DATADESC()


//LINK_ENTITY_TO_CLASS( base_zombie, CNPC_BaseZombie );

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::g_numZombies = 0;


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::CNPC_BaseZombie()
{
	// Gotta select which sound we're going to play, right here!
	// Because everyone's constructed before they spawn.
	//
	// Assign moan sounds in order, over and over.
	// This means if 3 or so zombies spawn near each
	// other, they will definitely not pick the same
	// moan loop.
	m_iMoanSound = g_numZombies;

	g_numZombies++;
}


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::~CNPC_BaseZombie()
{
	g_numZombies--;
}


//---------------------------------------------------------
// The closest physics object is chosen that is:
// <= MaxMass in Mass
// Between the zombie and the enemy
// not too far from a direct line to the enemy.
//---------------------------------------------------------
bool CNPC_BaseZombie::FindNearestPhysicsObject( int iMaxMass )
{
	CBaseEntity		*pList[ ZOMBIE_PHYSICS_SEARCH_DEPTH ];
	CBaseEntity		*pNearest = NULL;
	float			flDist;
	IPhysicsObject	*pPhysObj;
	int				i;
	Vector			vecDirToEnemy;
	Vector			vecDirToObject;

	if ( !CanSwatPhysicsObjects() || !GetEnemy() )
	{
		// Can't swat, or no enemy, so no swat.
		m_hPhysicsEnt = NULL;
		return false;
	}

	vecDirToEnemy = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	float dist = VectorNormalize(vecDirToEnemy);
	vecDirToEnemy.z = 0;

	if( dist > ZOMBIE_PLAYER_MAX_SWAT_DIST )
	{
		// Player is too far away. Don't bother 
		// trying to swat anything at them until
		// they are closer.
		return false;
	}

	float flNearestDist = MIN( dist, ZOMBIE_FARTHEST_PHYSICS_OBJECT * 0.5 );
	Vector vecDelta( flNearestDist, flNearestDist, GetHullHeight() * 2.0 );

	class CZombieSwatEntitiesEnum : public CFlaggedEntitiesEnum
	{
	public:
		CZombieSwatEntitiesEnum( CBaseEntity **pList, int listMax, int iMaxMass )
		 :	CFlaggedEntitiesEnum( pList, listMax, 0 ),
			m_iMaxMass( iMaxMass )
		{
		}

		virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
		{
			CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
			if ( pEntity && 
				 pEntity->VPhysicsGetObject() && 
				 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
				 pEntity->VPhysicsGetObject()->IsAsleep() && 
				 pEntity->VPhysicsGetObject()->IsMoveable() )
			{
#ifdef MAPBASE
				// Don't swat props that don't want to be swatted
				if (pEntity->HasSpawnFlags(SF_PHYSPROP_NO_ZOMBIE_SWAT) && dynamic_cast<CPhysicsProp*>(pEntity))
					return ITERATION_CONTINUE;
#endif

				return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
			}
			return ITERATION_CONTINUE;
		}

		int m_iMaxMass;
	};

	CZombieSwatEntitiesEnum swatEnum( pList, ZOMBIE_PHYSICS_SEARCH_DEPTH, iMaxMass );

	int count = UTIL_EntitiesInBox( GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, &swatEnum );

	// magically know where they are
	Vector vecZombieKnees;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.25f ), &vecZombieKnees );

	for( i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[ i ]->VPhysicsGetObject();

		Assert( !( !pPhysObj || pPhysObj->GetMass() > iMaxMass || !pPhysObj->IsAsleep() ) );

		Vector center = pList[ i ]->WorldSpaceCenter();
		flDist = UTIL_DistApprox2D( GetAbsOrigin(), center );

		if( flDist >= flNearestDist )
			continue;

		// This object is closer... but is it between the player and the zombie?
		vecDirToObject = pList[ i ]->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		if( DotProduct( vecDirToEnemy, vecDirToObject ) < 0.8 )
			continue;

		if( flDist >= UTIL_DistApprox2D( center, GetEnemy()->GetAbsOrigin() ) )
			continue;

		// don't swat things where the highest point is under my knees
		// NOTE: This is a rough test; a more exact test is going to occur below
		if ( (center.z + pList[i]->BoundingRadius()) < vecZombieKnees.z )
			continue;

		// don't swat things that are over my head.
		if( center.z > EyePosition().z )
			continue;

		vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
		
		Vector objMins, objMaxs;
		physcollision->CollideGetAABB( &objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

		if ( objMaxs.z < vecZombieKnees.z )
			continue;

		if ( !FVisible( pList[i] ) )
			continue;

		if ( hl2_episodic.GetBool() )
		{
			// Skip things that the enemy can't see. Do we want this as a general thing? 
			// The case for this feature is that zombies who are pursuing the player will
			// stop along the way to swat objects at the player who is around the corner or 
			// otherwise not in a place that the object has a hope of hitting. This diversion
			// makes the zombies very late (in a random fashion) getting where they are going. (sjb 1/2/06)
			if( !GetEnemy()->FVisible( pList[i] ) )
				continue;
		}

		// Make this the last check, since it makes a string.
		// Don't swat server ragdolls!
		if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
			continue;
			
		if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
			continue;

		// The object must also be closer to the zombie than it is to the enemy
		pNearest = pList[ i ];
		flNearestDist = flDist;
	}

	m_hPhysicsEnt = pNearest;

	if( m_hPhysicsEnt == NULL )
	{
		return false;
	}
	else
	{
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseZombie::Classify( void )
{
	if ( IsSlumped() )
		return CLASS_NONE;

	return( CLASS_ZOMBIE ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_BaseZombie::IRelationType( CBaseEntity *pTarget )
{
	// Slumping should not affect Zombie's opinion of others
	if ( IsSlumped() )
	{
		m_bIsSlumped = false;
		Disposition_t result = BaseClass::IRelationType( pTarget );
		m_bIsSlumped = true;
		return result;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the maximum yaw speed based on the monster's current activity.
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::MaxYawSpeed( void )
{
	if( m_fIsTorso )
	{
		return( 60 );
	}
	else if (IsMoving() && HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
	{
		return( 15 );
	}
	else
	{
		switch( GetActivity() )
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 100;
			break;
		case ACT_RUN:
			return 15;
			break;
		case ACT_WALK:
		case ACT_IDLE:
			return 25;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 120;
		default:
			return 90;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
	{
		return BaseClass::OverrideMoveFacing( move, flInterval );
	}

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );
	float idealYaw = UTIL_AngleMod( flMoveYaw );

	if (GetEnemy())
	{
		float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

		if (flEDist < 256.0)
		{
			float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

			if (flEDist < 128.0)
			{
				idealYaw = flEYaw;
			}
			else
			{
				idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
			}

			//DevMsg("was %.0f now %.0f\n", flMoveYaw, idealYaw );
		}
	}

	GetMotor()->SetIdealYawAndUpdate( idealYaw );

	// find movement direction to compensate for not being turned far enough
	float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
	SetPoseParameter( m_poseMove_Yaw, GetPoseParameter( m_poseMove_Yaw ) + flDiff );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
	float range = GetClawAttackRange();

	if (flDist > range )
	{
		// Translate a hit vehicle into its passenger if found
		if ( GetEnemy() != NULL )
		{
#if defined(HL2_DLL) && !defined(HL2MP)
			// If the player is holding an object, knock it down.
			if( GetEnemy()->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

				Assert( pPlayer != NULL );

				// Is the player carrying something?
				CBaseEntity *pObject = GetPlayerHeldEntity(pPlayer);

				if( !pObject )
				{
					pObject = PhysCannonGetHeldEntity( pPlayer->GetActiveWeapon() );
				}

				if( pObject )
				{
					float flDist = pObject->WorldSpaceCenter().DistTo( WorldSpaceCenter() );

					if( flDist <= GetClawAttackRange() )
						return COND_CAN_MELEE_ATTACK1;
				}
			}
#endif
		}
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	// Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	trace_t	tr;
	CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * GetClawAttackRange(), vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

	if( tr.fraction == 1.0 || !tr.m_pEnt )
	{

#ifdef HL2_EPISODIC

		// If our trace was unobstructed but we were shooting 
		if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
			return COND_CAN_MELEE_ATTACK1;

#endif // HL2_EPISODIC

		// This attack would miss completely. Trick the zombie into moving around some more.
		return COND_TOO_FAR_TO_ATTACK;
	}

	if( tr.m_pEnt == GetEnemy() || 
		tr.m_pEnt->IsNPC() || 
		( tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt) ) ) )
	{
		// -Let the zombie swipe at his enemy if he's going to hit them.
		// -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
		//  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
		// -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
		return COND_CAN_MELEE_ATTACK1;
	}

	Vector vecTrace = tr.endpos - tr.startpos;
	float lenTraceSq = vecTrace.Length2DSqr();

	if ( GetEnemy() && GetEnemy()->MyCombatCharacterPointer() && tr.m_pEnt == static_cast<CBaseCombatCharacter *>(GetEnemy())->GetVehicleEntity() )
	{
		if ( lenTraceSq < Square( GetClawAttackRange() * 0.75f ) )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}

	if( tr.m_pEnt->IsBSPModel() )
	{
		// The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
		// the enemy is, treat this as an obstruction.
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

		if( lenTraceSq < vecToEnemy.Length2DSqr() )
		{
			return COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION;
		}
	}

#ifdef HL2_EPISODIC

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
	{
		//Try to swat whatever the player is standing on instead of acting like a dill.
		return COND_CAN_MELEE_ATTACK1;
	}

	// Bullseyes are given some grace on if they can be hit
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
		return COND_CAN_MELEE_ATTACK1;

#endif // HL2_EPISODIC

	// Move around some more
	return COND_TOO_FAR_TO_ATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST	96.0f // Triple damage from buckshot at 8 feet (headshot only)
float CNPC_BaseZombie::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	switch( iHitGroup )
	{
	case HITGROUP_HEAD:
		{
			if( info.GetDamageType() & DMG_BUCKSHOT )
			{
				float flDist = FLT_MAX;

				if( info.GetAttacker() )
				{
					flDist = ( GetAbsOrigin() - info.GetAttacker()->GetAbsOrigin() ).Length();
				}

				if( flDist <= ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST )
				{
					return 3.0f;
				}
			}
			else
			{
				return 2.0f;
			}
		}
	}

	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo infoCopy = info;

	// Keep track of headshots so we can determine whether to pop off our headcrab.
	if (ptr->hitgroup == HITGROUP_HEAD)
	{
		m_bHeadShot = true;
	}

	if( infoCopy.GetDamageType() & DMG_BUCKSHOT )
	{
		// Zombie gets across-the-board damage reduction for buckshot. This compensates for the recent changes which
		// make the shotgun much more powerful, and returns the zombies to a level that has been playtested extensively.(sjb)
		// This normalizes the buckshot damage to what it used to be on normal (5 dmg per pellet. Now it's 8 dmg per pellet). 
		infoCopy.ScaleDamage( 0.625 );
	}

	BaseClass::TraceAttack( infoCopy, vecDir, ptr, pAccumulator );
}


//-----------------------------------------------------------------------------
// Purpose: A zombie has taken damage. Determine whether he should split in half
// Input  : 
// Output : bool, true if yes.
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
	if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
		return false;

#ifdef MAPBASE
	if ( HasSpawnFlags(SF_ZOMBIE_NO_TORSO) )
		return false;
#endif

	if ( m_fIsTorso )
	{
		// Already split.
		return false;
	}

	// Not if we're in a dss
	if ( IsRunningDynamicInteraction() )
		return false;

	// Break in half IF:
	// 
	// Take half or more of max health in DMG_BLAST
	if( (info.GetDamageType() & DMG_BLAST) && flDamageThreshold >= 0.5 )
	{
		return true;
	}

	if ( hl2_episodic.GetBool() )
	{
		// Always split after a cannon hit
		if ( info.GetAmmoType() == GetAmmoDef()->Index("CombineHeavyCannon") )
			return true;
	}

#if 0
	if( info.GetDamageType() & DMG_BUCKSHOT )
	{
		if( m_iHealth <= 0 || flDamageThreshold >= 0.5 )
		{
			return true;
		}
	}
#endif 
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: A zombie has taken damage. Determine whether he release his headcrab.
// Output : YES, IMMEDIATE, or SCHEDULED (see HeadcrabRelease_t)
//-----------------------------------------------------------------------------
HeadcrabRelease_t CNPC_BaseZombie::ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold )
{
#ifdef MAPBASE
	if ( m_iHealth <= 0 && !m_fIsHeadless )
#else
	if ( m_iHealth <= 0 )
#endif
	{
		if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
			return RELEASE_NO;

		if ( info.GetDamageType() & DMG_SNIPER )
			return RELEASE_RAGDOLL;

		// If I was killed by a bullet...
		if ( info.GetDamageType() & DMG_BULLET )
		{
			if( m_bHeadShot ) 
			{
				if( flDamageThreshold > 0.25 )
				{
					// Enough force to kill the crab.
					return RELEASE_RAGDOLL;
				}
			}
			else
			{
				// Killed by a shot to body or something. Crab is ok!
				return RELEASE_IMMEDIATE;
			}
		}

		// If I was killed by an explosion, release the crab.
		if ( info.GetDamageType() & DMG_BLAST )
		{
			return RELEASE_RAGDOLL;
		}

		if ( m_fIsTorso && IsChopped( info ) )
		{
			return RELEASE_RAGDOLL_SLICED_OFF;
		}
	}

	return RELEASE_NO;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
#define ZOMBIE_SCORCH_RATE		8
#define ZOMBIE_MIN_RENDERCOLOR	50
int CNPC_BaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	if( inputInfo.GetDamageType() & DMG_BURN )
	{
		// If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
		// This is to stop zombies from burning to death 10x faster when they're standing around
		// 10 fire entities.
		if( IsOnFire() && !(inputInfo.GetDamageType() & DMG_DIRECT) )
		{
			return 0;
		}
		
		Scorch( ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR );
	}

	// Take some percentage of damage from bullets (unless hit in the crab). Always take full buckshot & sniper damage
	if ( !m_bHeadShot && (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & (DMG_BUCKSHOT|DMG_SNIPER)) )
	{
		info.ScaleDamage( ZOMBIE_BULLET_DAMAGE_SCALE );
	}

	if ( ShouldIgnite( info ) )
	{
		Ignite( 100.0f );
	}

	int tookDamage = BaseClass::OnTakeDamage_Alive( info );

	// flDamageThreshold is what percentage of the creature's max health
	// this amount of damage represents. (clips at 1.0)
	float flDamageThreshold = MIN( 1, info.GetDamage() / m_iMaxHealth );
	
	// Being chopped up by a sharp physics object is a pretty special case
	// so we handle it with some special code. Mainly for 
	// Ravenholm's helicopter traps right now (sjb).
	bool bChopped = IsChopped(info);
	bool bSquashed = IsSquashed(info);
	bool bKilledByVehicle = ( ( info.GetDamageType() & DMG_VEHICLE ) != 0 );

#ifdef MAPBASE
	if ( !m_fIsTorso && (bChopped || bSquashed) && !bKilledByVehicle && !(info.GetDamageType() & DMG_REMOVENORAGDOLL) && !HasSpawnFlags(SF_ZOMBIE_NO_TORSO) )
#else
	if( !m_fIsTorso && (bChopped || bSquashed) && !bKilledByVehicle && !(info.GetDamageType() & DMG_REMOVENORAGDOLL) )
#endif
	{
		if( bChopped )
		{
			EmitSound( "E3_Phystown.Slicer" );
		}

		DieChopped( info );
	}
	else
	{
		HeadcrabRelease_t release = ShouldReleaseHeadcrab( info, flDamageThreshold );
		
		switch( release )
		{
		case RELEASE_IMMEDIATE:
			ReleaseHeadcrab( EyePosition(), vec3_origin, true, true );
			break;

		case RELEASE_RAGDOLL:
			// Go a little easy on headcrab ragdoll force. They're light!
			ReleaseHeadcrab( EyePosition(), inputInfo.GetDamageForce() * 0.25, true, false, true );
			break;

		case RELEASE_RAGDOLL_SLICED_OFF:
			{
				EmitSound( "E3_Phystown.Slicer" );
				Vector vecForce = inputInfo.GetDamageForce() * 0.1;
				vecForce += Vector( 0, 0, 2000.0 );
				ReleaseHeadcrab( EyePosition(), vecForce, true, false, true );
			}
			break;

		case RELEASE_VAPORIZE:
			RemoveHead();
			break;

		case RELEASE_SCHEDULED:
			SetCondition( COND_ZOMBIE_RELEASECRAB );
			break;
		}

		if( ShouldBecomeTorso( info, flDamageThreshold ) )
		{
			bool bHitByCombineCannon = (inputInfo.GetAmmoType() == GetAmmoDef()->Index("CombineHeavyCannon"));

			if ( CanBecomeLiveTorso() )
			{
				BecomeTorso( vec3_origin, inputInfo.GetDamageForce() * 0.50 );

				if ( ( info.GetDamageType() & DMG_BLAST) && random->RandomInt( 0, 1 ) == 0 )
				{
					Ignite( 5.0 + random->RandomFloat( 0.0, 5.0 ) );
				}

				// For Combine cannon impacts
				if ( hl2_episodic.GetBool() )
				{
					if ( bHitByCombineCannon )
					{
						// Catch on fire.
						Ignite( 5.0f + random->RandomFloat( 0.0f, 5.0f ) );
					}
				}

				if (flDamageThreshold >= 1.0)
				{
					m_iHealth = 0;
					BecomeRagdollOnClient( info.GetDamageForce() );
				}
			}
			else if ( random->RandomInt(1, 3) == 1 )
				DieChopped( info );
		}
	}

	if( tookDamage > 0 && (info.GetDamageType() & (DMG_BURN|DMG_DIRECT)) && m_ActBusyBehavior.IsActive() ) 
	{
		//!!!HACKHACK- Stuff a light_damage condition if an actbusying zombie takes direct burn damage. This will cause an
		// ignited zombie to 'wake up' and rise out of its actbusy slump. (sjb)
		SetCondition( COND_LIGHT_DAMAGE );
	}

	// IMPORTANT: always clear the headshot flag after applying damage. No early outs!
	m_bHeadShot = false;

	return tookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: make a sound Alyx can hear when in darkness mode
// Input  : volume (radius) of the sound.
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MakeAISpookySound( float volume, float duration )
{
#ifdef HL2_EPISODIC
	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT, EyePosition(), volume, duration, this, SOUNDENT_CHANNEL_SPOOKY_NOISE );
	}
#endif // HL2_EPISODIC
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::CanPlayMoanSound()
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
		return false;

	// Burning zombies play their moan loop at full volume for as long as they're
	// burning. Don't let a moan envelope play cause it will turn the volume down when done.
	if( IsOnFire() )
		return false;

	// Members of a small group of zombies can vocalize whenever they want
	if( s_iAngryZombies <= 4 )
		return true;

	// This serves to limit the number of zombies that can moan at one time when there are a lot. 
	if( random->RandomInt( 1, zombie_moanfreq.GetInt() * (s_iAngryZombies/2) ) == 1 )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Open a window and let a little bit of the looping moan sound
//			come through.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
	{
		// Not yet!
		return;
	}

	if( !m_pMoanSound )
	{
		// Don't set this up until the code calls for it.
		const char *pszSound = GetMoanSound( m_iMoanSound );
		m_flMoanPitch = random->RandomInt( zombie_basemin.GetInt(), zombie_basemax.GetInt() );

		//m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
		CPASAttenuationFilter filter( this );
		m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, ATTN_NORM );

		ENVELOPE_CONTROLLER.Play( m_pMoanSound, 1.0, m_flMoanPitch );
	}

	//HACKHACK get these from chia chin's console vars.
	envDefaultZombieMoanVolumeFast[ 1 ].durationMin = zombie_decaymin.GetFloat();
	envDefaultZombieMoanVolumeFast[ 1 ].durationMax = zombie_decaymax.GetFloat();

	if( random->RandomInt( 1, 2 ) == 1 )
	{
		IdleSound();
	}

	float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

	float flPitch = random->RandomInt( m_flMoanPitch + zombie_changemin.GetInt(), m_flMoanPitch + zombie_changemax.GetInt() );
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, flPitch, 0.3 );

	m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}

//-----------------------------------------------------------------------------
// Purpose: Determine whether the zombie is chopped up by some physics item
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::IsChopped( const CTakeDamageInfo &info )
{
	float flDamageThreshold = MIN( 1, info.GetDamage() / m_iMaxHealth );

	if ( m_iHealth > 0 || flDamageThreshold <= 0.5 )
		return false;

	if ( !( info.GetDamageType() & DMG_SLASH) )
		return false;

	if ( !( info.GetDamageType() & DMG_CRUSH) )
		return false;

	if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
		return false;

	// If you take crush and slash damage, you're hit by a sharp physics item.
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this gibbing zombie should ignite its gibs
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgniteZombieGib( void )
{
#ifdef HL2_EPISODIC
	// If we're in darkness mode, don't ignite giblets, because we don't want to
	// pay the perf cost of multiple dynamic lights per giblet.
	return ( IsOnFire() && !HL2GameRules()->IsAlyxInDarknessMode() );
#else
	return IsOnFire();
#endif 
}

#ifdef MAPBASE
extern CBaseAnimating *CreateServerRagdollSubmodel( CBaseAnimating *pOwner, const char *pModelName, const Vector &position, const QAngle &angles, int collisionGroup );
#endif

//-----------------------------------------------------------------------------
// Purpose: Handle the special case of a zombie killed by a physics chopper.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::DieChopped( const CTakeDamageInfo &info )
{
	bool bSquashed = IsSquashed(info);

	Vector forceVector( vec3_origin );

	forceVector += CalcDamageForceVector( info );

	if( !m_fIsHeadless && !bSquashed )
	{
		if( random->RandomInt( 0, 1 ) == 0 )
		{
			// Drop a live crab half of the time.
			ReleaseHeadcrab( EyePosition(), forceVector * 0.005, true, false, false );
		}
	}

#ifdef MAPBASE
	// Hack for fast zombies using base torso rules.
	if (m_iHealth > 0)
	{
		SetHealth( 0 );
	}
#endif

	float flFadeTime = 0.0;

	if( HasSpawnFlags( SF_NPC_FADE_CORPSE ) )
	{
		flFadeTime = 5.0;
	}

	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	Vector vecLegsForce;
	vecLegsForce.x = random->RandomFloat( -400, 400 );
	vecLegsForce.y = random->RandomFloat( -400, 400 );
	vecLegsForce.z = random->RandomFloat( 0, 250 );

	if( bSquashed && vecLegsForce.z > 0 )
	{
		// Force the broken legs down. (Give some additional force, too)
		vecLegsForce.z *= -10;
	}

#ifdef MAPBASE
	CBaseEntity *pLegGib = NULL;
	if ( m_bForceServerRagdoll )
	{
		pLegGib = CreateServerRagdollSubmodel( this, GetLegsModel(), GetAbsOrigin(), GetAbsAngles(), COLLISION_GROUP_INTERACTIVE_DEBRIS );
		pLegGib->VPhysicsGetObject()->AddVelocity(&vecLegsForce, NULL);
		if (ShouldIgniteZombieGib())
			static_cast<CBaseAnimating*>(pLegGib)->Ignite( random->RandomFloat( 8.0, 12.0 ), false );

		if ( flFadeTime > 0.0 )
		{
			pLegGib->SUB_StartFadeOut( flFadeTime, false );
		}
	}
	else
		pLegGib = CreateRagGib( GetLegsModel(), GetAbsOrigin(), GetAbsAngles(), vecLegsForce, flFadeTime, ShouldIgniteZombieGib() );
#else
	CBaseEntity *pLegGib = CreateRagGib( GetLegsModel(), GetAbsOrigin(), GetAbsAngles(), vecLegsForce, flFadeTime, ShouldIgniteZombieGib() );
#endif
	if ( pLegGib )
	{
#ifdef MAPBASE
		// Inherit some misc. properties
		pLegGib->m_iViewHideFlags = m_iViewHideFlags;
#endif

		CopyRenderColorTo( pLegGib );
	}

	forceVector *= random->RandomFloat( 0.04, 0.06 );
	forceVector.z = ( 100 * 12 * 5 ) * random->RandomFloat( 0.8, 1.2 );

	if( bSquashed && forceVector.z > 0 )
	{
		// Force the broken torso down.
		forceVector.z *= -1.0;
	}

	// Why do I have to fix this up?! (sjb)
	QAngle TorsoAngles;
	TorsoAngles = GetAbsAngles();
	TorsoAngles.x -= 90.0f;
#ifdef MAPBASE
	CBaseEntity *pTorsoGib = NULL;
	if ( m_bForceServerRagdoll )
	{
		pTorsoGib = CreateServerRagdollSubmodel( this, GetTorsoModel(), GetAbsOrigin() + Vector( 0, 0, 64 ), TorsoAngles, COLLISION_GROUP_INTERACTIVE_DEBRIS );
		pTorsoGib->VPhysicsGetObject()->AddVelocity(&forceVector, NULL);
		if (ShouldIgniteZombieGib())
			static_cast<CBaseAnimating*>(pLegGib)->Ignite( random->RandomFloat( 8.0, 12.0 ), false );

		if ( flFadeTime > 0.0 )
		{
			pTorsoGib->SUB_StartFadeOut( flFadeTime, false );
		}
	}
	else
		pTorsoGib = CreateRagGib( GetTorsoModel(), GetAbsOrigin() + Vector( 0, 0, 64 ), TorsoAngles, forceVector, flFadeTime, ShouldIgniteZombieGib() );
#else
	CBaseEntity *pTorsoGib = CreateRagGib( GetTorsoModel(), GetAbsOrigin() + Vector( 0, 0, 64 ), TorsoAngles, forceVector, flFadeTime, ShouldIgniteZombieGib() );
#endif
	if ( pTorsoGib )
	{
		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pTorsoGib);
		if( pAnimating )
		{
			pAnimating->SetBodygroup( ZOMBIE_BODYGROUP_HEADCRAB, !m_fIsHeadless );
#ifdef MAPBASE
			// Inherit some animating properties
			pAnimating->m_nSkin = m_nSkin;
#endif
		}

#ifdef MAPBASE
		// Inherit some misc. properties
		pTorsoGib->m_iViewHideFlags = m_iViewHideFlags;
#endif

		pTorsoGib->SetOwnerEntity( this );
		CopyRenderColorTo( pTorsoGib );

	}

	if ( UTIL_ShouldShowBlood( BLOOD_COLOR_YELLOW ) )
	{
		int i;
		Vector vecSpot;
		Vector vecDir;

		for ( i = 0 ; i < 4; i++ )
		{
			vecSpot = WorldSpaceCenter();

			vecSpot.x += random->RandomFloat( -12, 12 ); 
			vecSpot.y += random->RandomFloat( -12, 12 ); 
			vecSpot.z += random->RandomFloat( -4, 16 ); 

			UTIL_BloodDrips( vecSpot, vec3_origin, BLOOD_COLOR_YELLOW, 50 );
		}

		for ( int i = 0 ; i < 4 ; i++ )
		{
			Vector vecSpot = WorldSpaceCenter();

			vecSpot.x += random->RandomFloat( -12, 12 ); 
			vecSpot.y += random->RandomFloat( -12, 12 ); 
			vecSpot.z += random->RandomFloat( -4, 16 );

			vecDir.x = random->RandomFloat(-1, 1);
			vecDir.y = random->RandomFloat(-1, 1);
			vecDir.z = 0;
			VectorNormalize( vecDir );

			UTIL_BloodImpact( vecSpot, vecDir, BloodColor(), 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: damage has been done. Should the zombie ignite?
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgnite( const CTakeDamageInfo &info )
{
 	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if ( m_flBurnDamage >= m_iMaxHealth * 0.1 )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sufficient fire damage has been done. Zombie ignites!
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

#ifdef HL2_EPISODIC
	if ( HL2GameRules()->IsAlyxInDarknessMode() == true && GetEffectEntity() != NULL )
	{
		GetEffectEntity()->AddEffects( EF_DIMLIGHT );
	}
#endif // HL2_EPISODIC

	// Set the zombie up to burn to death in about ten seconds.
	SetHealth( MIN( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat( -ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE)) ) );

	// FIXME: use overlays when they come online
	//AddOverlay( ACT_ZOM_WALK_ON_FIRE, false );
	if( !m_ActBusyBehavior.IsActive() )
	{
		Activity activity = GetActivity();
		Activity burningActivity = activity;

		if ( activity == ACT_WALK )
		{
			burningActivity = ACT_WALK_ON_FIRE;
		}
		else if ( activity == ACT_RUN )
		{
			burningActivity = ACT_RUN_ON_FIRE;
		}
		else if ( activity == ACT_IDLE )
		{
			burningActivity = ACT_IDLE_ON_FIRE;
		}

		if( HaveSequenceForActivity(burningActivity) )
		{
			// Make sure we have a sequence for this activity (torsos don't have any, for instance) 
			// to prevent the baseNPC & baseAnimating code from throwing red level errors.
			SetActivity( burningActivity );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::CopyRenderColorTo( CBaseEntity *pOther )
{
	color32 color = GetRenderColor();
	pOther->SetRenderColor( color.r, color.g, color.b, color.a );
}

//-----------------------------------------------------------------------------
// Purpose: Look in front and see if the claw hit anything.
//
// Input  :	flDist				distance to trace		
//			iDamage				damage to do if attack hits
//			vecViewPunch		camera punch (if attack hits player)
//			vecVelocityPunch	velocity punch (if attack hits player)
//
// Output : The entity hit by claws. NULL if nothing.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_BaseZombie::ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin  )
{
	// Added test because claw attack anim sometimes used when for cases other than melee
	int iDriverInitialHealth = -1;
	CBaseEntity *pDriver = NULL;
	if ( GetEnemy() )
	{
		trace_t	tr;
		AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f )
			return NULL;

		// CheckTraceHullAttack() can damage player in vehicle as side effect of melee attack damaging physics objects, which the car forwards to the player
		// need to detect this to get correct damage effects
		CBaseCombatCharacter *pCCEnemy = ( GetEnemy() != NULL ) ? GetEnemy()->MyCombatCharacterPointer() : NULL;
		CBaseEntity *pVehicleEntity;
		if ( pCCEnemy != NULL && ( pVehicleEntity = pCCEnemy->GetVehicleEntity() ) != NULL )
		{
			if ( pVehicleEntity->GetServerVehicle() && dynamic_cast<CPropVehicleDriveable *>(pVehicleEntity) )
			{
				pDriver = static_cast<CPropVehicleDriveable *>(pVehicleEntity)->GetDriver();
				if ( pDriver && pDriver->IsPlayer() )
				{
					iDriverInitialHealth = pDriver->GetHealth();
				}
				else
				{
					pDriver = NULL;
				}
			}
		}
	}

	//
	// Trace out a cubic section of our hull and see what we hit.
	//
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	CBaseEntity *pHurt = NULL;
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{ 
		// We always hit bullseyes we're targeting
		pHurt = GetEnemy();
		CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), iDamage, DMG_SLASH );
		pHurt->TakeDamage( info );
	}
	else 
	{
		// Try to hit them with a trace
		pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_SLASH );
	}

	if ( pDriver && iDriverInitialHealth != pDriver->GetHealth() )
	{
		pHurt = pDriver;
	}

	if ( !pHurt && m_hPhysicsEnt != NULL && IsCurSchedule(SCHED_ZOMBIE_ATTACKITEM) )
	{
		pHurt = m_hPhysicsEnt;

		Vector vForce = pHurt->WorldSpaceCenter() - WorldSpaceCenter(); 
		VectorNormalize( vForce );

		vForce *= 5 * 24;

		CTakeDamageInfo info( this, this, vForce, GetAbsOrigin(), iDamage, DMG_SLASH );
		pHurt->TakeDamage( info );

		pHurt = m_hPhysicsEnt;
	}

	if ( pHurt )
	{
		AttackHitSound();

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE ) )
		{
			pPlayer->ViewPunch( qaViewPunch );
			
			pPlayer->VelocityPunch( vecVelocityPunch );
		}
		else if( !pPlayer && UTIL_ShouldShowBlood(pHurt->BloodColor()) )
		{
			// Hit an NPC. Bleed them!
			Vector vecBloodPos;

			switch( BloodOrigin )
			{
			case ZOMBIE_BLOOD_LEFT_HAND:
				if( GetAttachment( "blood_left", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_RIGHT_HAND:
				if( GetAttachment( "blood_right", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_BOTH_HANDS:
				if( GetAttachment( "blood_left", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );

				if( GetAttachment( "blood_right", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_BITE:
				// No blood for these.
				break;
			}
		}
	}
	else 
	{
		AttackMissSound();
	}

	if ( pHurt == m_hPhysicsEnt && IsCurSchedule(SCHED_ZOMBIE_ATTACKITEM) )
	{
		m_hPhysicsEnt = NULL;
		m_flNextSwat = gpGlobals->curtime + random->RandomFloat( 2, 4 );
	}

	return pHurt;
}

//-----------------------------------------------------------------------------
// Purpose: The zombie is frustrated and pounding walls/doors. Make an appropriate noise
// Input  : 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::PoundSound()
{
	trace_t		tr;
	Vector		forward;

	GetVectors( &forward, NULL, NULL );

	AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction == 1.0 )
	{
		// Didn't hit anything!
		return;
	}

	if( tr.fraction < 1.0 && tr.m_pEnt )
	{
		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			EmitSound( physprops->GetString(psurf->sounds.impactHard) );
			return;
		}
	}

	// Otherwise fall through to the default sound.
	CPASAttenuationFilter filter( this,"NPC_BaseZombie.PoundDoor" );
	EmitSound( filter, entindex(),"NPC_BaseZombie.PoundDoor" );
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		if( GetEnemy() && GetEnemy()->IsNPC() )
		{
			if( HasCondition(COND_CAN_MELEE_ATTACK1) )
			{
				// This animation is sometimes played by code that doesn't intend to attack the enemy
				// (For instance, code that makes a zombie take a frustrated swipe at an obstacle). 
				// Try not to trigger a reaction from our enemy unless we're really attacking. 
				GetEnemy()->MyNPCPointer()->DispatchInteraction( g_interactionZombieMeleeWarning, NULL, this );
			}
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_POUND )
	{
		PoundSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
	{
		AlertSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( false );
		return;
	}
	
	if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_GET_UP )
	{
		MakeAIFootstepSound( 180.0f, 3.0f );
		if( !IsOnFire() )
		{
			// If you let this code run while a zombie is burning, it will stop wailing. 
			m_flNextMoanSound = gpGlobals->curtime;
			MoanSound( envDefaultZombieMoanVolumeFast, ARRAYSIZE( envDefaultZombieMoanVolumeFast ) );
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( true );
		return;
	}

	// all swat animations are handled as a single case.
	if ( pEvent->event == AE_ZOMBIE_STARTSWAT )
	{
		MakeAIFootstepSound( 180.0f );
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
	{
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SWATITEM )
	{
		CBaseEntity *pEnemy = GetEnemy();
		if ( pEnemy )
		{
			Vector v;
			CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
			if( !pPhysicsEntity )
			{
				DevMsg( "**Zombie: Missing my physics ent!!" );
				return;
			}
			
			IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

			if( !pPhysObj )
			{
				DevMsg( "**Zombie: No Physics Object for physics Ent!" );
				return;
			}

			EmitSound( "NPC_BaseZombie.Swat" );
			PhysicsImpactSound( pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 800 );

			Vector physicsCenter = pPhysicsEntity->WorldSpaceCenter();
			v = pEnemy->WorldSpaceCenter() - physicsCenter;
			VectorNormalize(v);

			// Send the object at 800 in/sec toward the enemy.  Add 200 in/sec up velocity to keep it
			// in the air for a second or so.
			v = v * 800;
			v.z += 200;

			// add some spin so the object doesn't appear to just fly in a straight line
			// Also this spin will move the object slightly as it will press on whatever the object
			// is resting on.
			AngularImpulse angVelocity( random->RandomFloat(-180, 180), 20, random->RandomFloat(-360, 360) );

			pPhysObj->AddVelocity( &v, &angVelocity );

#ifdef MAPBASE
			m_OnSwattedProp.Set(pPhysicsEntity, pPhysicsEntity, this);
#endif

			// If we don't put the object scan time well into the future, the zombie
			// will re-select the object he just hit as it is flying away from him.
			// It will likely always be the nearest object because the zombie moved
			// close enough to it to hit it.
			m_hPhysicsEnt = NULL;

			m_flNextSwatScan = gpGlobals->curtime + ZOMBIE_SWAT_DELAY;

			return;
		}
	}
	
	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );
		
		right = right * 100;
		forward = forward * 200;

		QAngle qa( -15, -20, -10 );
		Vector vec = right + forward;
		ClawAttack( GetClawAttackRange(), sk_zombie_dmg_one_slash.GetFloat(), qa, vec, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * -100;
		forward = forward * 200;

		QAngle qa( -15, 20, -10 );
		Vector vec = right + forward;
		ClawAttack( GetClawAttackRange(), sk_zombie_dmg_one_slash.GetFloat(), qa, vec, ZOMBIE_BLOOD_LEFT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_BOTH )
	{
		Vector forward;
		QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
		AngleVectors( GetLocalAngles(), &forward );
		forward = forward * 200;
		ClawAttack( GetClawAttackRange(), sk_zombie_dmg_one_slash.GetFloat(), qaPunch, forward, ZOMBIE_BLOOD_BOTH_HANDS );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_POPHEADCRAB )
	{
		if ( GetInteractionPartner() == NULL )
			return;

		const char	*pString = pEvent->options;
		char		token[128];
		pString = nexttoken( token, pString, ' ', sizeof(token) );

		int boneIndex = GetInteractionPartner()->LookupBone( token );

		if ( boneIndex == -1 )
		{
			Warning( "AE_ZOMBIE_POPHEADCRAB event using invalid bone name! Usage: event AE_ZOMBIE_POPHEADCRAB \"<BoneName> <Speed>\" \n" );
			return;
		}

		pString = nexttoken( token, pString, ' ', sizeof( token ) );

		if ( !token )
		{
			Warning( "AE_ZOMBIE_POPHEADCRAB event format missing velocity parameter! Usage: event AE_ZOMBIE_POPHEADCRAB \"<BoneName> <Speed>\" \n" );
			return;
		}

		Vector vecBonePosition;
		QAngle angles;
		Vector vecHeadCrabPosition;

		int iCrabAttachment = LookupAttachment( "headcrab" );
		int iSpeed = atoi( token );

		GetInteractionPartner()->GetBonePosition( boneIndex, vecBonePosition, angles );
		GetAttachment( iCrabAttachment, vecHeadCrabPosition );

		Vector vVelocity = vecHeadCrabPosition - vecBonePosition;
		VectorNormalize( vVelocity );

		CTakeDamageInfo	dmgInfo( this, GetInteractionPartner(), m_iHealth, DMG_DIRECT );

		dmgInfo.SetDamagePosition( vecHeadCrabPosition );

#ifdef MAPBASE
		ReleaseHeadcrab( vecHeadCrabPosition, vVelocity *iSpeed, true, false, true );
#else
		ReleaseHeadcrab( EyePosition(), vVelocity * iSpeed, true, false, true );
#endif

		GuessDamageForce( &dmgInfo, vVelocity, vecHeadCrabPosition, 0.5f );
		TakeDamage( dmgInfo );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the base zombie.
//
// !!!IMPORTANT!!! YOUR DERIVED CLASS'S SPAWN() RESPONSIBILITIES:
//
//		Call Precache();
//		Set status for m_fIsTorso & m_fIsHeadless
//		Set blood color
//		Set health
//		Set field of view
//		Call CapabilitiesClear() & then set relevant capabilities
//		THEN Call BaseClass::Spawn()
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_SQUAD );

	m_flNextSwat = gpGlobals->curtime;
	m_flNextSwatScan = gpGlobals->curtime;
	m_pMoanSound = NULL;

	m_flNextMoanSound = gpGlobals->curtime + 9999;

	SetZombieModel();

	NPCInit();

	m_bIsSlumped = false;

	// Zombies get to cheat for 6 seconds (sjb)
	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

	m_ActBusyBehavior.SetUseRenderBounds(true);
}


//-----------------------------------------------------------------------------
// Purpose: Pecaches all resources this NPC needs.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Precache( void )
{
	UTIL_PrecacheOther( GetHeadcrabClassname() );

	PrecacheScriptSound( "E3_Phystown.Slicer" );
	PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
	PrecacheScriptSound( "NPC_BaseZombie.Swat" );

	PrecacheModel( GetLegsModel() );
	PrecacheModel( GetTorsoModel() );

	PrecacheParticleSystem( "blood_impact_zombie_01" );

	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if( IsSlumped() && hl2_episodic.GetBool() )
	{
		if( FClassnameIs( pOther, "prop_physics" ) )
		{
			// Get up!
			m_ActBusyBehavior.StopBusying();
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::CreateBehaviors()
{
	AddBehavior( &m_ActBusyBehavior );

	return BaseClass::CreateBehaviors();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( HasCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION ) && !HasCondition(COND_TASK_FAILED) && IsCurSchedule( SCHED_ZOMBIE_CHASE_ENEMY, false ) )
		{
			return SCHED_COMBAT_PATROL;
		}
		return SCHED_ZOMBIE_CHASE_ENEMY;
		break;

	case SCHED_ZOMBIE_SWATITEM:
		// If the object is far away, move and swat it. If it's close, just swat it.
		if( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
		{
			return SCHED_ZOMBIE_MOVE_SWATITEM;
		}
		else
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;

	case SCHED_STANDOFF:
		return SCHED_ZOMBIE_WANDER_STANDOFF;

	case SCHED_MELEE_ATTACK1:
		return SCHED_ZOMBIE_MELEE_ATTACK1;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#ifndef HL2_EPISODIC
	else if ( m_flNextFlinch >= gpGlobals->curtime )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#endif // !HL2_EPISODIC

	// Everything should be interrupted if we get killed.
	SetCustomInterruptCondition( COND_ZOMBIE_RELEASECRAB );

	BaseClass::BuildScheduleTestBits();
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change schedules.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnScheduleChange( void )
{
	//
	// If we took damage and changed schedules, ignore further damage for a few seconds.
	//
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
	{
		m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;
	} 

	BaseClass::OnScheduleChange();
}


//---------------------------------------------------------
//---------------------------------------------------------

bool CNPC_BaseZombie::CanFlinch( void )
{
	if (!BaseClass::CanFlinch())
		return false;

#ifdef MAPBASE
	if (zombie_no_flinch_during_unique_anim.GetBool())
	{
		// Don't flinch if currently playing actbusy animation (navigating to or from one is fine)
		if (m_ActBusyBehavior.IsInsideActBusy())
			return false;

		// Don't flinch if currently playing scripted sequence (navigating to or from one is fine)
		if (m_NPCState == NPC_STATE_SCRIPT && (IsCurSchedule( SCHED_SCRIPTED_WAIT, false ) || IsCurSchedule( SCHED_SCRIPTED_FACE, false )))
			return false;
	}
#endif

	return true;
}


//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_BaseZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_ZOMBIE_WANDER_MEDIUM )
	{
		return SCHED_ZOMBIE_WANDER_FAIL;
	}

	// If we can swat physics objects, see if we can swat our obstructor
	if ( CanSwatPhysicsObjects() )
	{
		if ( !m_fIsTorso && IsPathTaskFailure( taskFailCode ) && 
			 m_hObstructor != NULL && m_hObstructor->VPhysicsGetObject() && 
			 m_hObstructor->VPhysicsGetObject()->GetMass() < 100 )
		{
			m_hPhysicsEnt = m_hObstructor;
			m_hObstructor = NULL;
			return SCHED_ZOMBIE_ATTACKITEM;
		}
	}

	m_hObstructor = NULL;

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::SelectSchedule ( void )
{
	if ( HasCondition( COND_ZOMBIE_RELEASECRAB ) )
	{
		// Death waits for no man. Or zombie. Or something.
		return SCHED_ZOMBIE_RELEASECRAB;
	}

	if ( BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
		{
			float flDist;

			flDist = ( GetLocalOrigin() - GetEnemy()->GetLocalOrigin() ).Length();

			// If this is a new enemy that's far away, ambush!!
			if (flDist >= zombie_ambushdist.GetFloat() && MustCloseToAttack() )
			{
				return SCHED_ZOMBIE_MOVE_TO_AMBUSH;
			}
		}

		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}

		if( HasCondition( COND_ZOMBIE_CAN_SWAT_ATTACK ) )
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );

#ifdef DEBUG_ZOMBIES
			DevMsg("Wandering\n");
#endif

			// Just lost track of our enemy. 
			// Wander around a bit so we don't look like a dingus.
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsSlumped( void )
{
	if( hl2_episodic.GetBool() )
	{
		if( m_ActBusyBehavior.IsInsideActBusy() && !m_ActBusyBehavior.IsStopBusying() )
		{
			return true;
		}
	}
	else
	{
		int sequence = GetSequence();
		if ( sequence != -1 )
		{
			return ( strncmp( GetSequenceName( sequence ), "slump", 5 ) == 0 );
		}
	}

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsGettingUp( void )
{
	if( m_ActBusyBehavior.IsActive() && m_ActBusyBehavior.IsStopBusying() )
	{
		return true;
	}
	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::GetSwatActivity( void )
{
	// Hafta figure out whether to swat with left or right arm.
	// Also hafta figure out whether to swat high or low. (later)
	float		flDot;
	Vector		vecRight, vecDirToObj;

	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
	
	vecDirToObj = m_hPhysicsEnt->GetLocalOrigin() - GetLocalOrigin();
	VectorNormalize(vecDirToObj);

	// compare in 2D.
	vecRight.z = 0.0;
	vecDirToObj.z = 0.0;

	flDot = DotProduct( vecRight, vecDirToObj );

	Vector vecMyCenter;
	Vector vecObjCenter;

	vecMyCenter = WorldSpaceCenter();
	vecObjCenter = m_hPhysicsEnt->WorldSpaceCenter();
	float flZDiff;

	flZDiff = vecMyCenter.z - vecObjCenter.z;

	if( flDot >= 0 )
	{
		// Right
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATRIGHTMID;
		}

		return ACT_ZOM_SWATRIGHTLOW;
	}
	else
	{
		// Left
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATLEFTMID;
		}

		return ACT_ZOM_SWATLEFTLOW;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::GatherConditions( void )
{
	ClearCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();

	if( m_NPCState == NPC_STATE_COMBAT && !m_fIsTorso )
	{
		// This check for !m_pPhysicsEnt prevents a crashing bug, but also
		// eliminates the zombie picking a better physics object if one happens to fall
		// between him and the object he's heading for already. 
		if( gpGlobals->curtime >= m_flNextSwatScan && (m_hPhysicsEnt == NULL) )
		{
			FindNearestPhysicsObject( ZOMBIE_MAX_PHYSOBJ_MASS );
			m_flNextSwatScan = gpGlobals->curtime + 2.0;
		}
	}

	if( (m_hPhysicsEnt != NULL) && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_ZOMBIE_RELEASECRAB ) )
	{
		SetCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
	else
	{
		ClearCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
#if 0
	DevMsg(" ** %d Angry Zombies **\n", s_iAngryZombies );
#endif

#if 0
	if( m_NPCState == NPC_STATE_COMBAT )
	{
		// Zombies should make idle sounds in combat
		if( random->RandomInt( 0, 30 ) == 0 )
		{
			IdleSound();
		}
	}	
#endif 

	//
	// Cool off if we aren't burned for five seconds or so. 
	//
	if ( ( m_flBurnDamageResetTime ) && ( gpGlobals->curtime >= m_flBurnDamageResetTime ) )
	{
		m_flBurnDamage = 0;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_DIE:
		// Go to ragdoll
		KillMe();
		TaskComplete();
		break;

	case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ:
		{
			Vector vecGoalPos;
			Vector vecDir;

			vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z = 0;

			AI_NavGoal_t goal( m_hPhysicsEnt->WorldSpaceCenter() );
			goal.pTarget = m_hPhysicsEnt;
			GetNavigator()->SetGoal( goal );

			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_SWAT_ITEM:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
			}
			else if ( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
			{
				// Physics ent is no longer in range! Probably another zombie swatted it or it moved
				// for some other reason.
				TaskFail( "Physics swat item has moved" );
			}
			else
			{
				SetIdealActivity( (Activity)GetSwatActivity() );
			}
			break;
		}
		break;

	case TASK_ZOMBIE_DELAY_SWAT:
		m_flNextSwat = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_ZOMBIE_RELEASE_HEADCRAB:
		{
			// make the crab look like it's pushing off the body
			Vector vecForward;
			Vector vecVelocity;

			AngleVectors( GetAbsAngles(), &vecForward );
			
			vecVelocity = vecForward * 30;
			vecVelocity.z += 100;

			ReleaseHeadcrab( EyePosition(), vecVelocity, true, true );
			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
#ifndef HL2_EPISODIC
			TaskComplete();
			return;
#endif

			// Don't wait when attacking the player
			if ( GetEnemy() && GetEnemy()->IsPlayer() )
			{
				TaskComplete();
				return;
			}

			// Wait a single think
			SetWait( 0.1 );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_SWAT_ITEM:
		if( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
// Make the necessary changes to a zombie to make him a 
// torso!
//---------------------------------------------------------
void CNPC_BaseZombie::BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce )
{
	if( m_fIsTorso )
	{
		DevMsg( "*** Zombie is already a torso!\n" );
		return;
	}

	if( IsOnFire() )
	{
		Extinguish();
		Ignite( 30 );
	}

	if ( !m_fIsHeadless )
	{
		m_iMaxHealth = ZOMBIE_TORSO_HEALTH_FACTOR * m_iMaxHealth;
		m_iHealth = m_iMaxHealth;

		// No more opening doors!
		CapabilitiesRemove( bits_CAP_DOORS_GROUP );
		
		ClearSchedule( "Becoming torso" );
		GetNavigator()->ClearGoal();
		m_hPhysicsEnt = NULL;

		// Put the zombie in a TOSS / fall schedule
		// Otherwise he fails and sits on the ground for a sec.
		SetSchedule( SCHED_FALL_TO_GROUND );

		m_fIsTorso = true;

		// Put the torso up where the torso was when the zombie
		// was whole.
		Vector origin = GetAbsOrigin();
		origin.z += 40;
		SetAbsOrigin( origin );

		SetGroundEntity( NULL );
		// assume zombie mass ~ 100 kg
		ApplyAbsVelocityImpulse( vecTorsoForce * (1.0 / 100.0) );
	}

	float flFadeTime = 0.0;

	if( HasSpawnFlags( SF_NPC_FADE_CORPSE ) )
	{
		flFadeTime = 5.0;
	}

	if ( m_fIsTorso == true )
	{
		// -40 on Z to make up for the +40 on Z that we did above. This stops legs spawning above the head.
#ifdef MAPBASE
		CBaseEntity *pGib = NULL;
		if ( m_bForceServerRagdoll )
		{
			pGib = CreateServerRagdollSubmodel( this, GetLegsModel(), GetAbsOrigin() - Vector(0, 0, 40), GetAbsAngles(), COLLISION_GROUP_INTERACTIVE_DEBRIS );
			if (pGib && pGib->VPhysicsGetObject())
			{
				pGib->VPhysicsGetObject()->AddVelocity( &vecLegsForce, NULL );

				if (flFadeTime > 0.0)
				{
					pGib->SUB_StartFadeOut( flFadeTime, false );
				}
			}
		}
		else
			pGib = CreateRagGib( GetLegsModel(), GetAbsOrigin() - Vector(0, 0, 40), GetAbsAngles(), vecLegsForce, flFadeTime );
#else
		CBaseEntity *pGib = CreateRagGib( GetLegsModel(), GetAbsOrigin() - Vector(0, 0, 40), GetAbsAngles(), vecLegsForce, flFadeTime );
#endif

		// don't collide with this thing ever
		if ( pGib )
		{
			pGib->SetOwnerEntity( this );
		}
	}


	SetZombieModel();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::Event_Killed( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_VEHICLE )
	{
		Vector vecDamageDir = info.GetDamageForce();
		VectorNormalize( vecDamageDir );

		// Big blood splat
		UTIL_BloodSpray( WorldSpaceCenter(), vecDamageDir, BLOOD_COLOR_YELLOW, 8, FX_BLOODSPRAY_CLOUD );
	}

   	BaseClass::Event_Killed( info );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	bool bKilledByVehicle = ( ( info.GetDamageType() & DMG_VEHICLE ) != 0 );
	if( m_fIsTorso || (!IsChopped(info) && !IsSquashed(info)) || bKilledByVehicle )
	{
		return BaseClass::BecomeRagdoll( info, forceVector );
	}

	if( !(GetFlags()&FL_TRANSRAGDOLL) )
	{
		RemoveDeferred();
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StopLoopingSounds()
{
	ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
	m_pMoanSound = NULL;

	BaseClass::StopLoopingSounds();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RemoveHead( void )
{
	m_fIsHeadless = true;
	SetZombieModel();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::SetModel( const char *szModelName )
{
#ifdef MAPBASE
	// Zombies setting the same model again is a problem when they should maintain their current sequence (e.g. during dynamic interactions)
	if ( IsRunningDynamicInteraction() && GetModelIndex() != 0 && FStrEq( szModelName, STRING(GetModelName()) ) )
		return;
#endif

	BaseClass::SetModel( szModelName );
}


bool CNPC_BaseZombie::ShouldPlayFootstepMoan( void )
{
	if( random->RandomInt( 1, zombie_stepfreq.GetInt() * s_iAngryZombies ) == 1 )
	{
		return true;
	}

	return false;
}


#define ZOMBIE_CRAB_INHERITED_SPAWNFLAGS	(SF_NPC_GAG|SF_NPC_LONG_RANGE|SF_NPC_FADE_CORPSE|SF_NPC_ALWAYSTHINK)
#define CRAB_HULL_EXPAND	1.1f
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::HeadcrabFits( CBaseAnimating *pCrab, const Vector *vecOrigin )
{
	Vector vecSpawnLoc;
#ifdef MAPBASE
	if (vecOrigin)
		vecSpawnLoc = *vecOrigin;
	else
#endif
	vecSpawnLoc = pCrab->GetAbsOrigin();

	CTraceFilterSimpleList traceFilter( COLLISION_GROUP_NONE );
	traceFilter.AddEntityToIgnore( pCrab );
	traceFilter.AddEntityToIgnore( this );
	if ( GetInteractionPartner() )
	{
		traceFilter.AddEntityToIgnore( GetInteractionPartner() );
	}

	trace_t tr;
	AI_TraceHull(	vecSpawnLoc,
					vecSpawnLoc - Vector( 0, 0, 1 ), 
					NAI_Hull::Mins(HULL_TINY) * CRAB_HULL_EXPAND,
					NAI_Hull::Maxs(HULL_TINY) * CRAB_HULL_EXPAND,
					MASK_NPCSOLID,
					&traceFilter,
					&tr );

	if( tr.fraction != 1.0 )
	{
		//NDebugOverlay::Box( vecSpawnLoc, NAI_Hull::Mins(HULL_TINY) * CRAB_HULL_EXPAND, NAI_Hull::Maxs(HULL_TINY) * CRAB_HULL_EXPAND, 255, 0, 0, 100, 10.0 );
		return false;
	}

	//NDebugOverlay::Box( vecSpawnLoc, NAI_Hull::Mins(HULL_TINY) * CRAB_HULL_EXPAND, NAI_Hull::Maxs(HULL_TINY) * CRAB_HULL_EXPAND, 0, 255, 0, 100, 10.0 );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			fRemoveHead - 
//			fRagdollBody - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab )
{
	CAI_BaseNPC		*pCrab;
	Vector vecSpot = vecOrigin;

	// Until the headcrab is a bodygroup, we have to approximate the
	// location of the head with magic numbers.
	if( !m_fIsTorso )
	{
		vecSpot.z -= 16;
	}

	if( fRagdollCrab )
	{
		//Vector vecForce = Vector( 0, 0, random->RandomFloat( 700, 1100 ) );
#ifdef MAPBASE
		CBaseEntity *pGib = NULL;
		if ( m_bForceServerRagdoll )
		{
			pGib = CreateServerRagdollSubmodel( this, GetHeadcrabModel(), vecOrigin, GetLocalAngles(), COLLISION_GROUP_INTERACTIVE_DEBRIS );
			if (pGib && pGib->VPhysicsGetObject())
			{
				pGib->VPhysicsGetObject()->AddVelocity(&vecVelocity, NULL);
				if (ShouldIgniteZombieGib())
					static_cast<CBaseAnimating*>(pGib)->Ignite( random->RandomFloat( 8.0, 12.0 ), false );

				pGib->SUB_StartFadeOut( 15, false );
			}
		}
		else
			pGib = CreateRagGib( GetHeadcrabModel(), vecOrigin, GetLocalAngles(), vecVelocity, 15, ShouldIgniteZombieGib() );
#else
		CBaseEntity *pGib = CreateRagGib( GetHeadcrabModel(), vecOrigin, GetLocalAngles(), vecVelocity, 15, ShouldIgniteZombieGib() );
#endif

		if ( pGib )
		{
			CBaseAnimating *pAnimatingGib = dynamic_cast<CBaseAnimating*>(pGib);

			// don't collide with this thing ever
			int iCrabAttachment = LookupAttachment( "headcrab" );
			if (iCrabAttachment > 0 && pAnimatingGib )
			{
				SetHeadcrabSpawnLocation( iCrabAttachment, pAnimatingGib );
			}

#ifdef MAPBASE
			// Server ragdolls don't have a valid origin on spawn, so we have to use the origin originally passed
			if( !HeadcrabFits( pAnimatingGib, m_bForceServerRagdoll ? &vecOrigin : NULL ) )
#else
			if( !HeadcrabFits(pAnimatingGib) )
#endif
			{
				UTIL_Remove(pGib);
				return;
			}

#ifdef MAPBASE
			// Inherit some misc. properties
			pGib->m_iViewHideFlags = m_iViewHideFlags;
#endif

			pGib->SetOwnerEntity( this );
			CopyRenderColorTo( pGib );

			
			if( UTIL_ShouldShowBlood(BLOOD_COLOR_YELLOW) )
			{
				Vector vecGibCenter;
#ifdef MAPBASE
				// Server ragdolls don't have a valid origin on spawn, so we have to use the origin originally passed
				if (m_bForceServerRagdoll)
					vecGibCenter = vecOrigin;
				else
#endif
				vecGibCenter = pGib->WorldSpaceCenter();

				UTIL_BloodImpact( vecGibCenter, Vector(0,0,1), BLOOD_COLOR_YELLOW, 1 );

				for ( int i = 0 ; i < 3 ; i++ )
				{
					Vector vecSpot = vecGibCenter;
					
					vecSpot.x += random->RandomFloat( -8, 8 ); 
					vecSpot.y += random->RandomFloat( -8, 8 ); 
					vecSpot.z += random->RandomFloat( -8, 8 ); 

					UTIL_BloodDrips( vecSpot, vec3_origin, BLOOD_COLOR_YELLOW, 50 );
				}
			}
		}
	}
	else
	{
		pCrab = (CAI_BaseNPC*)CreateEntityByName( GetHeadcrabClassname() );

		if ( !pCrab )
		{
			Warning( "**%s: Can't make %s!\n", GetClassname(), GetHeadcrabClassname() );
			return;
		}

		// Stick the crab in whatever squad the zombie was in.
		pCrab->SetSquadName( m_SquadName );

		// don't pop to floor, fall
		pCrab->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
		
		// add on the parent flags
		pCrab->AddSpawnFlags( m_spawnflags & ZOMBIE_CRAB_INHERITED_SPAWNFLAGS );

#ifdef MAPBASE
		// Inherit some misc. properties
		pCrab->m_bForceServerRagdoll = m_bForceServerRagdoll;
		pCrab->m_iViewHideFlags = m_iViewHideFlags;

		// Add response context for companion response (more reliable than checking for post-death zombie entity)
		pCrab->AddContext( "from_zombie", "1", 2.0f );
#endif
		
		// make me the crab's owner to avoid collision issues
		pCrab->SetOwnerEntity( this );

		pCrab->SetAbsOrigin( vecSpot );
		pCrab->SetAbsAngles( GetAbsAngles() );
		DispatchSpawn( pCrab );

		pCrab->GetMotor()->SetIdealYaw( GetAbsAngles().y );

		// FIXME: npc's with multiple headcrabs will need some way to query different attachments.
		// NOTE: this has till after spawn is called so that the model is set up
		int iCrabAttachment = LookupAttachment( "headcrab" );
		if (iCrabAttachment > 0)
		{
			SetHeadcrabSpawnLocation( iCrabAttachment, pCrab );
			pCrab->GetMotor()->SetIdealYaw( pCrab->GetAbsAngles().y );
			
			// Take out any pitch
			QAngle angles = pCrab->GetAbsAngles();
			angles.x = 0.0;
			pCrab->SetAbsAngles( angles );
		}

		if( !HeadcrabFits(pCrab) )
		{
			UTIL_Remove(pCrab);
			return;
		}

		pCrab->SetActivity( ACT_IDLE );
		pCrab->SetNextThink( gpGlobals->curtime );
		pCrab->PhysicsSimulate();
		pCrab->SetAbsVelocity( vecVelocity );

		// if I have an enemy, stuff that to the headcrab.
		CBaseEntity *pEnemy;
		pEnemy = GetEnemy();

		pCrab->m_flNextAttack = gpGlobals->curtime + 1.0f;

		if( pEnemy )
		{
			pCrab->SetEnemy( pEnemy );
		}
		if( ShouldIgniteZombieGib() )
		{
			pCrab->Ignite( 30 );
		}

		CopyRenderColorTo( pCrab );

		pCrab->Activate();

#ifdef MAPBASE
		m_OnCrab.Set( pCrab, pCrab, this );
#endif
	}

	if( fRemoveHead )
	{
		RemoveHead();
	}

	if( fRagdollBody )
	{
		BecomeRagdollOnClient( vec3_origin );
	}
}



void CNPC_BaseZombie::SetHeadcrabSpawnLocation( int iCrabAttachment, CBaseAnimating *pCrab )
{
	Assert( iCrabAttachment > 0 );

	// get world location of intended headcrab root bone
	matrix3x4_t attachmentToWorld;
	GetAttachment( iCrabAttachment, attachmentToWorld );

	// find offset of root bone from origin 
	pCrab->SetAbsOrigin( Vector( 0, 0, 0 ) );
	pCrab->SetAbsAngles( QAngle( 0, 0, 0 ) );
	pCrab->InvalidateBoneCache();
	matrix3x4_t rootLocal;
	pCrab->GetBoneTransform( 0, rootLocal );

	// invert it
	matrix3x4_t rootInvLocal;
	MatrixInvert( rootLocal, rootInvLocal );

	// find spawn location needed for rootLocal transform to match attachmentToWorld
	matrix3x4_t spawnOrigin;
	ConcatTransforms( attachmentToWorld, rootInvLocal, spawnOrigin );

	// reset location of headcrab
	Vector vecOrigin;
	QAngle vecAngles;
	MatrixAngles( spawnOrigin, vecAngles, vecOrigin );
	pCrab->SetAbsOrigin( vecOrigin );
	
	// FIXME: head crabs don't like pitch or roll!
	vecAngles.z = 0;

	pCrab->SetAbsAngles( vecAngles );
	pCrab->InvalidateBoneCache();
}



//---------------------------------------------------------
// Provides a standard way for the zombie to get the 
// distance to a physics ent. Since the code to find physics 
// objects uses a fast dis approx, we have to use that here
// as well.
//---------------------------------------------------------
float CNPC_BaseZombie::DistToPhysicsEnt( void )
{
	//return ( GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin() ).Length();
	if ( m_hPhysicsEnt != NULL )
		return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );
	return ZOMBIE_PHYSOBJ_SWATDIST + 1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		{
			RemoveSpawnFlags( SF_NPC_GAG );
			s_iAngryZombies++;
		}
		break;

	default:
		if( OldState == NPC_STATE_COMBAT )
		{
			// Only decrement if coming OUT of combat state.
			s_iAngryZombies--;
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Refines a base activity into something more specific to our internal state.
//-----------------------------------------------------------------------------
Activity CNPC_BaseZombie::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_WALK && IsCurSchedule( SCHED_COMBAT_PATROL, false) )
		baseAct = ACT_RUN;

	if ( IsOnFire() )
	{
		switch ( baseAct )
		{
			case ACT_RUN_ON_FIRE:
			{
				return ( Activity )ACT_WALK_ON_FIRE;
			}

			case ACT_WALK:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_WALK_ON_FIRE;
			}

			case ACT_IDLE:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_IDLE_ON_FIRE;
			}
		}
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	
	if( IsCurSchedule(SCHED_BIG_FLINCH) || m_ActBusyBehavior.IsActive() )
	{
		// This zombie is assumed to be standing up. 
		// Return a position that's centered over the absorigin,
		// halfway between the origin and the head. 
		Vector vecTarget = GetAbsOrigin();
		Vector vecHead = HeadTarget( posSrc );
		vecTarget.z = ((vecTarget.z + vecHead.z) * 0.5f);
		return vecTarget;
	}

	return BaseClass::BodyTarget( posSrc, bNoisy );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::HeadTarget( const Vector &posSrc )
{
	int iCrabAttachment = LookupAttachment( "headcrab" );
	Assert( iCrabAttachment > 0 );

	Vector vecPosition;

	GetAttachment( iCrabAttachment, vecPosition );

	return vecPosition;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::GetAutoAimRadius()
{
	if( m_fIsTorso )
	{
		return 12.0f;
	}

	return BaseClass::GetAutoAimRadius();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_ENTITY && gpGlobals->curtime >= m_flNextSwat )
	{
		m_hObstructor = pMoveGoal->directTrace.pObstruction;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	// If our enemy is in a vehicle, we need them to tell us where to navigate to them
	if ( pEnemy == NULL )
		return;

	CBaseCombatCharacter *pBCC = pEnemy->MyCombatCharacterPointer();
	if ( pBCC && pBCC->IsInAVehicle() )
	{
		Vector vecForward, vecRight;
		pBCC->GetVectors( &vecForward, &vecRight, NULL );

		chasePosition = pBCC->WorldSpaceCenter() + ( vecForward * 24.0f ) + ( vecRight * 48.0f );
		return;
	}

	BaseClass::TranslateNavGoal( pEnemy, chasePosition );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( base_zombie, CNPC_BaseZombie )

	DECLARE_TASK( TASK_ZOMBIE_DELAY_SWAT )
	DECLARE_TASK( TASK_ZOMBIE_SWAT_ITEM )
	DECLARE_TASK( TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ )
	DECLARE_TASK( TASK_ZOMBIE_DIE )
	DECLARE_TASK( TASK_ZOMBIE_RELEASE_HEADCRAB )
	DECLARE_TASK( TASK_ZOMBIE_WAIT_POST_MELEE )

	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_RELEASECRAB )
	DECLARE_ACTIVITY( ACT_ZOM_FALL )

	DECLARE_CONDITION( COND_ZOMBIE_CAN_SWAT_ATTACK )
	DECLARE_CONDITION( COND_ZOMBIE_RELEASECRAB )
	DECLARE_CONDITION( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION )

	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SWATITEM )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STARTSWAT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_SCREAM )
	DECLARE_ANIMEVENT( AE_ZOMBIE_GET_UP )
	DECLARE_ANIMEVENT( AE_ZOMBIE_POUND )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND )
	DECLARE_ANIMEVENT( AE_ZOMBIE_POPHEADCRAB )

	DECLARE_INTERACTION( g_interactionZombieMeleeWarning )

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// SwatItem
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_ATTACKITEM,

		"	Tasks"
		"		TASK_FACE_ENEMY					0"
		"		TASK_MELEE_ATTACK1				0"
		"	"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// ChaseEnemy
	//=========================================================
#ifdef HL2_EPISODIC
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
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
		"		COND_ZOMBIE_CAN_SWAT_ATTACK"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_HEAVY_DAMAGE"
	)
#else 
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
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
		"		COND_ZOMBIE_CAN_SWAT_ATTACK"
		"		COND_ZOMBIE_RELEASECRAB"
	)
#endif // HL2_EPISODIC


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_RELEASECRAB,

		"	Tasks"
		"		TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ZOM_RELEASECRAB"
		"		TASK_ZOMBIE_RELEASE_HEADCRAB				0"
		"		TASK_ZOMBIE_DIE								0"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_TO_AMBUSH,

		"	Tasks"
		"		TASK_WAIT						1.0" // don't react as soon as you see the player.
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_TURN_LEFT					180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WAIT_AMBUSH"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_NEW_ENEMY"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WAIT_AMBUSH,

		"	Tasks"
		"		TASK_WAIT_FACE_ENEMY	99999"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
	)

	//=========================================================
	// Wander around for a while so we don't look stupid. 
	// this is done if we ever lose track of our enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_MEDIUM,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384" // 4 feet to 32 feet
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_STANDOFF,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384" // 4 feet to 32 feet
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ZOMBIE_RELEASECRAB"
	)

	//=========================================================
	// If you fail to wander, wait just a bit and try again.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_WAIT				1"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ZOMBIE_RELEASECRAB"
	)

	//=========================================================
	// Like the base class, only don't stop in the middle of 
	// swinging if the enemy is killed, hides, or new enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_MELEE_ATTACK1		0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_POST_MELEE_WAIT"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// Make the zombie wait a frame after a melee attack, to
	// allow itself & it's enemy to test for dynamic scripted sequences.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_POST_MELEE_WAIT,

		"	Tasks"
		"		TASK_ZOMBIE_WAIT_POST_MELEE		0"
	)

AI_END_CUSTOM_NPC()
