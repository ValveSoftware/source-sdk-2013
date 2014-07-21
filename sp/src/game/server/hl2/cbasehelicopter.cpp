//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for helicopters & helicopter-type vehicles
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "ai_network.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_task.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "entitylist.h"
#include "soundenvelope.h"
#include "gamerules.h"
#include "grenade_homer.h"
#include "ndebugoverlay.h"
#include "cbasehelicopter.h"
#include "soundflags.h"
#include "rope.h"
#include "saverestore_utlvector.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "effects.h"
#include "rotorwash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ExpandBBox(Vector &vecMins, Vector &vecMaxs);

#if 0
virtual void NullThink( void );
#endif //0

#define HELICOPTER_THINK_INTERVAL 0.1
#define HELICOPTER_ROTORWASH_THINK_INTERVAL 0.01
#define	BASECHOPPER_DEBUG_WASH		1

ConVar g_debug_basehelicopter( "g_debug_basehelicopter", "0", FCVAR_CHEAT );

//---------------------------------------------------------
//---------------------------------------------------------
// TODOs
//
// -Member function: CHANGE MOVE GOAL
//
// -Member function: GET GRAVITY (or GetMaxThrust)
//
//---------------------------------------------------------
//---------------------------------------------------------

static const char *s_pRotorWashThinkContext = "RotorWashThink";
static const char *s_pDelayedKillThinkContext = "DelayedKillThink";


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

BEGIN_DATADESC_NO_BASE( washentity_t )
	DEFINE_FIELD( hEntity,			FIELD_EHANDLE ),
	DEFINE_FIELD( flWashStartTime,	FIELD_TIME ),
END_DATADESC()


BEGIN_DATADESC( CBaseHelicopter )

	DEFINE_THINKFUNC( HelicopterThink ),
	DEFINE_THINKFUNC( RotorWashThink ),
	DEFINE_THINKFUNC( CallDyingThink ),
	DEFINE_THINKFUNC( DelayedKillThink ),
	DEFINE_ENTITYFUNC( CrashTouch ),
	DEFINE_ENTITYFUNC( FlyTouch ),

	DEFINE_SOUNDPATCH( m_pRotorSound ),
	DEFINE_SOUNDPATCH( m_pRotorBlast ),
	DEFINE_FIELD( m_flForce,			FIELD_FLOAT ),
	DEFINE_FIELD( m_fHelicopterFlags,	FIELD_INTEGER),
	DEFINE_FIELD( m_vecDesiredFaceDir,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flLastSeen,		FIELD_TIME ),
	DEFINE_FIELD( m_flPrevSeen,		FIELD_TIME ),
//	DEFINE_FIELD( m_iSoundState,		FIELD_INTEGER ),		// Don't save, precached
	DEFINE_FIELD( m_vecTargetPosition,	FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_hRotorWash,		FIELD_EHANDLE ),

	DEFINE_FIELD( m_flMaxSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxSpeedFiring,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flGoalSpeed,		FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flInitialSpeed, FIELD_FLOAT, "InitialSpeed" ),

	DEFINE_FIELD( m_flRandomOffsetTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecRandomOffset, FIELD_VECTOR ),
	DEFINE_FIELD( m_flRotorWashEntitySearchTime, FIELD_TIME ),
	DEFINE_FIELD( m_bSuppressSound,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStartupTime,	FIELD_TIME ),

	DEFINE_FIELD( m_cullBoxMins,	FIELD_VECTOR ),
	DEFINE_FIELD( m_cullBoxMaxs,	FIELD_VECTOR ),

	DEFINE_UTLVECTOR( m_hEntitiesPushedByWash, FIELD_EMBEDDED ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate),
	DEFINE_INPUTFUNC( FIELD_VOID, "GunOn", InputGunOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GunOff", InputGunOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MissileOn", InputMissileOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MissileOff", InputMissileOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableRotorWash", InputEnableRotorWash ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableRotorWash", InputDisableRotorWash ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MoveTopSpeed", InputMoveTopSpeed ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "MoveSpecifiedSpeed", InputMoveSpecifiedSpeed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetAngles", InputSetAngles ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableRotorSound", InputEnableRotorSound ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableRotorSound", InputDisableRotorSound ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CBaseHelicopter, DT_BaseHelicopter )
	SendPropTime( SENDINFO( m_flStartupTime ) ),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseHelicopter::CBaseHelicopter( void )
{
	m_cullBoxMins = vec3_origin;
	m_cullBoxMaxs = vec3_origin;

	m_hRotorWash = NULL;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
// Notes   : Have your derived Helicopter's Spawn() function call this one FIRST
//------------------------------------------------------------------------------
void CBaseHelicopter::Precache( void )
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
// Notes   : Have your derived Helicopter's Spawn() function call this one FIRST
//------------------------------------------------------------------------------
void CBaseHelicopter::Spawn( void )
{
	Precache( );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );
	AddFlag( FL_FLY );
	SetState( NPC_STATE_IDLE );

	m_lifeState			= LIFE_ALIVE;

	// motor
	//******
	// All of this stuff is specific to the individual type of aircraft. Handle it yourself.
	//******
	//	m_iAmmoType = g_pGameRules->GetAmmoDef()->Index("AR2"); 
	//	SetModel( "models/attack_helicopter.mdl" );
	//	UTIL_SetSize( this, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	//	UTIL_SetOrigin( this, GetLocalOrigin() );
	//	m_iHealth = 100;
	//	m_flFieldOfView = -0.707; // 270 degrees
	//	InitBoneControllers();
	//	m_iRockets			= 10;
	//	Get the rotor sound started up.

	// This base class assumes the helicopter has no guns or missiles. 
	// Set the appropriate flags in your derived class' Spawn() function.
	m_fHelicopterFlags &= ~BITS_HELICOPTER_MISSILE_ON;
	m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;

	m_pRotorSound = NULL;
	m_pRotorBlast = NULL;

	SetCycle( 0 );
	ResetSequenceInfo();

	AddFlag( FL_NPC );

	m_flMaxSpeed = BASECHOPPER_MAX_SPEED;
	m_flMaxSpeedFiring = BASECHOPPER_MAX_FIRING_SPEED;
	m_takedamage = DAMAGE_AIM;

	// Don't start up if the level designer has asked the 
	// helicopter to start disabled.
	if ( !(m_spawnflags & SF_AWAITINPUT) )
	{
		Startup();
		SetNextThink( gpGlobals->curtime + 1.0f );
	}
	else
	{
		m_flStartupTime = FLT_MAX;
	}

	InitPathingData( 0, BASECHOPPER_MIN_CHASE_DIST_DIFF, BASECHOPPER_AVOID_DIST );

	// Setup collision hull
	ExpandBBox( m_cullBoxMins, m_cullBoxMaxs );
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &m_cullBoxMins, &m_cullBoxMaxs );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	m_flRandomOffsetTime = -1.0f;
	m_vecRandomOffset.Init( 0, 0, 0 );
}


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateOnRemove()
{
	StopRotorWash();
	BaseClass::UpdateOnRemove();
}


//------------------------------------------------------------------------------
// Gets the max speed of the helicopter
//------------------------------------------------------------------------------
float CBaseHelicopter::GetMaxSpeed()
{
	// If our last path_track has specified a speed, use that instead of ours
	if ( GetPathMaxSpeed() )
		return GetPathMaxSpeed();

	return m_flMaxSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseHelicopter::GetMaxSpeedFiring()
{
	// If our last path_track has specified a speed, use that instead of ours
	if ( GetPathMaxSpeed() )
		return GetPathMaxSpeed();

	return m_flMaxSpeedFiring;
}
  

//------------------------------------------------------------------------------
// Enemy methods
//------------------------------------------------------------------------------
bool CBaseHelicopter::GetTrackPatherTarget( Vector *pPos ) 
{ 
	if ( GetEnemy() ) 
	{ 
		*pPos = GetEnemy()->BodyTarget( GetAbsOrigin(), false ); 
		return true; 
	}
	
	return false; 
}

CBaseEntity *CBaseHelicopter::GetTrackPatherTargetEnt()	
{ 
	return GetEnemy(); 
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CBaseHelicopter::FireGun( void )
{
	return true;
}


//------------------------------------------------------------------------------
// Purpose : The main think function for the helicopters
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::HelicopterThink( void )
{
	CheckPVSCondition();

	SetNextThink( gpGlobals->curtime + HELICOPTER_THINK_INTERVAL );

	// Don't keep this around for more than one frame.
	ClearCondition( COND_ENEMY_DEAD );

	// Animate and dispatch animation events.
	StudioFrameAdvance( );
	DispatchAnimEvents( this );

	PrescheduleThink();

	if ( IsMarkedForDeletion() )
		return;

	ShowDamage( );

	// -----------------------------------------------
	// If AI is disabled, kill any motion and return
	// -----------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		SetAbsVelocity( vec3_origin );
		SetLocalAngularVelocity( vec3_angle );
		SetNextThink( gpGlobals->curtime + HELICOPTER_THINK_INTERVAL );
		return;
	}

	Hunt();

	// Finally, forget dead enemies, or ones we've been told to ignore.
	if( GetEnemy() != NULL && (!GetEnemy()->IsAlive() || GetEnemy()->GetFlags() & FL_NOTARGET || IRelationType( GetEnemy() ) == D_NU ) )
	{
		SetEnemy( NULL );
	}

	HelicopterPostThink();
}

//-----------------------------------------------------------------------------
// Rotor wash think
//-----------------------------------------------------------------------------
void CBaseHelicopter::RotorWashThink( void )
{
	if ( m_lifeState == LIFE_ALIVE || m_lifeState == LIFE_DYING )
	{
		DrawRotorWash( BASECHOPPER_WASH_ALTITUDE, GetAbsOrigin() );
		SetContextThink( &CBaseHelicopter::RotorWashThink, gpGlobals->curtime + HELICOPTER_ROTORWASH_THINK_INTERVAL, s_pRotorWashThinkContext );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pRotorWashThinkContext );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseHelicopter::DrawRotorWash( float flAltitude, const Vector &vecRotorOrigin )
{
	// Shake any ropes nearby
	if ( random->RandomInt( 0, 2 ) == 0 )
	{
		CRopeKeyframe::ShakeRopes( GetAbsOrigin(), flAltitude, 128 );
	}

	if ( m_spawnflags & SF_NOROTORWASH )
		return;

	DoRotorPhysicsPush( vecRotorOrigin, flAltitude );

	if ( m_flRotorWashEntitySearchTime > gpGlobals->curtime )
		return;

	// Only push every half second
	m_flRotorWashEntitySearchTime = gpGlobals->curtime + 0.5f;
}


//-----------------------------------------------------------------------------
// Purpose: Push an airboat in our wash
//-----------------------------------------------------------------------------
#define MAX_AIRBOAT_ROLL_ANGLE			20.0f
#define MAX_AIRBOAT_ROLL_COSANGLE		0.866f
#define MAX_AIRBOAT_ROLL_COSANGLE_X2	0.5f

void CBaseHelicopter::DoWashPushOnAirboat( CBaseEntity *pAirboat, 
	const Vector &vecWashToAirboat, float flWashAmount )
{
	// For the airboat, simply produce a small roll and a push outwards.
	// But don't produce a roll if we're too rolled in that direction already.
	
	// Get the actual up direction vector
	Vector vecUp;
	pAirboat->GetVectors( NULL, NULL, &vecUp );
	if ( vecUp.z < MAX_AIRBOAT_ROLL_COSANGLE )
		return;

	// Compute roll direction so that we get pushed down on the side where the rotor wash is.
	Vector vecRollNormal;
	CrossProduct( vecWashToAirboat, Vector( 0, 0, 1 ), vecRollNormal );

	// Project it into the plane of the roll normal
	VectorMA( vecUp, -DotProduct( vecUp, vecRollNormal ), vecRollNormal, vecUp );
	VectorNormalize( vecUp );

	// Compute a vector which is the max direction we can roll given the roll constraint
	Vector vecExtremeUp;
	VMatrix rot;
	MatrixBuildRotationAboutAxis( rot, vecRollNormal, MAX_AIRBOAT_ROLL_ANGLE );
	MatrixGetColumn( rot, 2, &vecExtremeUp );

	// Find the angle between how vertical we are and how vertical we should be
	float flCosDelta = DotProduct( vecExtremeUp, vecUp );
	float flDelta = acos(flCosDelta) * 180.0f / M_PI;
	flDelta = clamp( flDelta, 0.0f, MAX_AIRBOAT_ROLL_ANGLE );
	flDelta = SimpleSplineRemapVal( flDelta, 0.0f, MAX_AIRBOAT_ROLL_ANGLE, 0.0f, 1.0f );

	float flForce = 12.0f * flWashAmount * flDelta;

	Vector vecWashOrigin;
	Vector vecForce;
	VectorMultiply( Vector( 0, 0, -1 ), flForce, vecForce );
	VectorMA( pAirboat->GetAbsOrigin(), -200.0f, vecWashToAirboat, vecWashOrigin );

	pAirboat->VPhysicsTakeDamage( CTakeDamageInfo( this, this, vecForce, vecWashOrigin, flWashAmount, DMG_BLAST ) );
}


//-----------------------------------------------------------------------------
// Purpose: Push a physics object in our wash. Return false if it's now out of our wash
//-----------------------------------------------------------------------------
bool CBaseHelicopter::DoWashPush( washentity_t *pWash, const Vector &vecWashOrigin )
{
	if ( !pWash || !pWash->hEntity.Get() )
		return false;

	// Make sure the entity is still within our wash's radius
	CBaseEntity *pEntity = pWash->hEntity;

	// This can happen because we can dynamically turn this flag on and off
	if ( pEntity->IsEFlagSet( EFL_NO_ROTORWASH_PUSH ))
		return false;

	Vector vecSpot = pEntity->BodyTarget( vecWashOrigin );
	Vector vecToSpot = ( vecSpot - vecWashOrigin );
	vecToSpot.z = 0;
	float flDist = VectorNormalize( vecToSpot );
	if ( flDist > BASECHOPPER_WASH_RADIUS )
		return false;

	IRotorWashShooter *pShooter = GetRotorWashShooter( pEntity );
	IPhysicsObject *pPhysObject;

	
	float flPushTime = (gpGlobals->curtime - pWash->flWashStartTime);
	flPushTime = clamp( flPushTime, 0, BASECHOPPER_WASH_RAMP_TIME );
	float flWashAmount = RemapVal( flPushTime, 0, BASECHOPPER_WASH_RAMP_TIME, BASECHOPPER_WASH_PUSH_MIN, BASECHOPPER_WASH_PUSH_MAX );

	if ( pShooter )
	{
		Vector vecForce = (0.015f / 0.1f) * flWashAmount * vecToSpot * phys_pushscale.GetFloat();
		pEntity = pShooter->DoWashPush( pWash->flWashStartTime, vecForce );
		if ( !pEntity )
			return true;

		washentity_t Wash;
		Wash.hEntity = pEntity;
		Wash.flWashStartTime = pWash->flWashStartTime;
		int i = m_hEntitiesPushedByWash.AddToTail( Wash );
		pWash = &m_hEntitiesPushedByWash[i];
		
		pPhysObject = pEntity->VPhysicsGetObject();
		if ( !pPhysObject )
			return true;
	}
	else
	{
		// Airboat gets special treatment
		if ( FClassnameIs( pEntity, "prop_vehicle_airboat" ) )
		{
			DoWashPushOnAirboat( pEntity, vecToSpot, flWashAmount );
			return true;
		}

		pPhysObject = pEntity->VPhysicsGetObject();
		if ( !pPhysObject )
			return false;
	}

	// Push it away from the center of the wash
	float flMass = pPhysObject->GetMass();

	// This used to be mass independent, which is a bad idea because it blows 200kg engine blocks
	// as much as it blows cardboard and soda cans. Make this force mass-independent, but clamp at
	// 30kg. 
	flMass = MIN( flMass, 30.0f );

	Vector vecForce = (0.015f / 0.1f) * flWashAmount * flMass * vecToSpot * phys_pushscale.GetFloat();
	pEntity->VPhysicsTakeDamage( CTakeDamageInfo( this, this, vecForce, vecWashOrigin, flWashAmount, DMG_BLAST ) );

	// Debug
	if ( g_debug_basehelicopter.GetInt() == BASECHOPPER_DEBUG_WASH )
	{
		NDebugOverlay::Cross3D( pEntity->GetAbsOrigin(), -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, true, 0.1f );
		NDebugOverlay::Line( pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin() + vecForce, 255, 255, 0, true, 0.1f );

		IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
		Msg("Pushed %s (index %d) (mass %f) with force %f (min %.2f max %.2f) at time %.2f\n", 
			pEntity->GetClassname(), pEntity->entindex(), pPhysObject->GetMass(), flWashAmount, 
			BASECHOPPER_WASH_PUSH_MIN * flMass, BASECHOPPER_WASH_PUSH_MAX * flMass, gpGlobals->curtime );
	}

	// If we've pushed this thing for some time, remove it to give us a chance to find lighter things nearby
	if ( flPushTime > 2.0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::DoRotorPhysicsPush( const Vector &vecRotorOrigin, float flAltitude )
{
	CBaseEntity *pEntity = NULL;
	trace_t tr;

	// First, trace down and find out where the was is hitting the ground
	UTIL_TraceLine( vecRotorOrigin, vecRotorOrigin+Vector(0,0,-flAltitude), (MASK_SOLID_BRUSHONLY|CONTENTS_WATER), NULL, COLLISION_GROUP_NONE, &tr );
	// Always raise the physics origin a bit
	Vector vecPhysicsOrigin = tr.endpos + Vector(0,0,64);

	// Debug
	if ( g_debug_basehelicopter.GetInt() == BASECHOPPER_DEBUG_WASH )
	{
		NDebugOverlay::Cross3D( vecPhysicsOrigin, -Vector(16,16,16), Vector(16,16,16), 0, 255, 255, true, 0.1f );
	}

	// Push entities that we've pushed before, and are still within range
	// Walk backwards because they may be removed if they're now out of range
	int iCount = m_hEntitiesPushedByWash.Count();
	bool bWasPushingObjects = (iCount > 0);
	for ( int i = (iCount-1); i >= 0; i-- )
	{
		if ( !DoWashPush( &(m_hEntitiesPushedByWash[i]), vecPhysicsOrigin ) )
		{
			// Out of range now, so remove
			m_hEntitiesPushedByWash.Remove(i);
		}
	}

	if ( m_flRotorWashEntitySearchTime > gpGlobals->curtime )
		return;

	// Any spare slots?
	iCount = m_hEntitiesPushedByWash.Count();
	if ( iCount >= BASECHOPPER_WASH_MAX_OBJECTS )
		return;

	// Find the lightest physics entity below us and add it to our list to push around
	CBaseEntity *pLightestEntity = NULL;
	float flLightestMass = 9999;
	while ((pEntity = gEntList.FindEntityInSphere(pEntity, vecPhysicsOrigin, BASECHOPPER_WASH_RADIUS )) != NULL)
	{
		IRotorWashShooter *pShooter = GetRotorWashShooter( pEntity );

		if ( pEntity->IsEFlagSet( EFL_NO_ROTORWASH_PUSH ))
			continue;

		if ( pShooter || pEntity->GetMoveType() == MOVETYPE_VPHYSICS || (pEntity->VPhysicsGetObject() && !pEntity->IsPlayer()) ) 
		{
			// Make sure it's not already in our wash
			bool bAlreadyPushing = false;
			for ( int i = 0; i < iCount; i++ )
			{
				if ( m_hEntitiesPushedByWash[i].hEntity == pEntity )
				{
					bAlreadyPushing = true;
					break;
				}
			}
			if ( bAlreadyPushing )
				continue;

			float flMass = FLT_MAX;
			if ( pShooter )
			{
				flMass = 1.0f;
			}
			else
			{
				// Don't try to push anything too big
				IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
				if ( pPhysObject )
				{
					flMass = pPhysObject->GetMass();
					if ( flMass > BASECHOPPER_WASH_MAX_MASS )
						continue;
				}
			}

			// Ignore anything bigger than the one we've already found
			if ( flMass > flLightestMass )
				continue;

			Vector vecSpot = pEntity->BodyTarget( vecPhysicsOrigin );

			// Don't push things too far below our starting point (helps reduce through-roof cases w/o doing a trace)
			if ( fabs( vecSpot.z - vecPhysicsOrigin.z ) > 96 )
				continue;

			Vector vecToSpot = ( vecSpot - vecPhysicsOrigin );
			vecToSpot.z = 0;
			float flDist = VectorNormalize( vecToSpot );
			if ( flDist > BASECHOPPER_WASH_RADIUS )
				continue;

			
			// Try to cast to the helicopter; if we can't, then we can't be hit.
			if ( pEntity->GetServerVehicle() )
			{
				UTIL_TraceLine( vecSpot, vecPhysicsOrigin, MASK_SOLID_BRUSHONLY, pEntity, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction != 1.0f )
					continue;
			}

			flLightestMass = flMass;
			pLightestEntity = pEntity;

			washentity_t Wash;
			Wash.hEntity = pLightestEntity;
			Wash.flWashStartTime = gpGlobals->curtime;
			m_hEntitiesPushedByWash.AddToTail( Wash );

			// Can we fit more after adding this one? No? Then we are done.
			iCount = m_hEntitiesPushedByWash.Count();
			if ( iCount >= BASECHOPPER_WASH_MAX_OBJECTS )
				break;
		}
	}

	// Handle sound.
	// If we just started pushing objects, ramp the blast sound up.
	if ( !bWasPushingObjects && m_hEntitiesPushedByWash.Count() )
	{
		if ( m_pRotorBlast )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pRotorBlast, 1.0, 1.0 );
		}
	}
	else if ( bWasPushingObjects && m_hEntitiesPushedByWash.Count() == 0 )
	{
		if ( m_pRotorBlast )
		{
			// We just stopped pushing objects, so fade the blast sound out.
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pRotorBlast, 0, 1.0 );
		}
	}
}


//------------------------------------------------------------------------------
// Updates the enemy
//------------------------------------------------------------------------------
float CBaseHelicopter::EnemySearchDistance( ) 
{
	return 4092;
}


//------------------------------------------------------------------------------
// Updates the enemy
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateEnemy()
{
	if( HasCondition( COND_ENEMY_DEAD ) )
	{
		SetEnemy( NULL );
	}

	// Look for my best enemy. If I change enemies, 
	// be sure and change my prevseen/lastseen timers.
	if( m_lifeState == LIFE_ALIVE )
	{
		GetSenses()->Look( EnemySearchDistance() );

		GetEnemies()->RefreshMemories();
		ChooseEnemy();

		if( HasEnemy() )
		{
			CBaseEntity *pEnemy = GetEnemy();
			GatherEnemyConditions( pEnemy );
			if ( FVisible( pEnemy ) )
			{
				if (m_flLastSeen < gpGlobals->curtime - 2)
				{
					m_flPrevSeen = gpGlobals->curtime;
				}

				m_flLastSeen = gpGlobals->curtime;
				m_vecTargetPosition = pEnemy->WorldSpaceCenter();
			}
		}
		else
		{
			// look at where we're going instead
			m_vecTargetPosition = GetDesiredPosition();
		}
	}
	else
	{
		// If we're dead or dying, forget our enemy and don't look for new ones(sjb)
		SetEnemy( NULL );
	}

}

//------------------------------------------------------------------------------
// Purpose : Override the desired position if your derived helicopter is doing something special
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateDesiredPosition( void )
{
}

//------------------------------------------------------------------------------
// Updates the facing direction
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateFacingDirection()
{
	if ( 1 )
	{
		Vector targetDir = m_vecTargetPosition - GetAbsOrigin();
		Vector desiredDir = GetDesiredPosition() - GetAbsOrigin();

		VectorNormalize( targetDir ); 
		VectorNormalize( desiredDir ); 

		if ( !IsCrashing() && m_flLastSeen + 5 > gpGlobals->curtime ) //&& DotProduct( targetDir, desiredDir) > 0.25)
		{
			// If we've seen the target recently, face the target.
			//Msg( "Facing Target \n" );
			m_vecDesiredFaceDir = targetDir;
		}
		else
		{
			// Face our desired position.
			// Msg( "Facing Position\n" );
			m_vecDesiredFaceDir = desiredDir;
		}
	}
	else
	{
		// Face the way the path corner tells us to.
		//Msg( "Facing my path corner\n" );
		m_vecDesiredFaceDir = GetGoalOrientation();
	}

}


//------------------------------------------------------------------------------
// Fire weapons
//------------------------------------------------------------------------------
void CBaseHelicopter::FireWeapons()
{
	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->curtime, m_flLastSeen, m_flPrevSeen );
	if (m_fHelicopterFlags & BITS_HELICOPTER_GUN_ON)
	{
		//if ( (m_flLastSeen + 1 > gpGlobals->curtime) && (m_flPrevSeen + 2 < gpGlobals->curtime) )
		{
			if (FireGun( ))
			{
				// slow down if we're firing
				if (m_flGoalSpeed > GetMaxSpeedFiring() )
				{
					m_flGoalSpeed = GetMaxSpeedFiring();
				}
			}
		}
	}

	if (m_fHelicopterFlags & BITS_HELICOPTER_MISSILE_ON)
	{
		AimRocketGun();
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::Hunt( void )
{
	UpdateEnemy();

	UpdateTrackNavigation( );

	UpdateDesiredPosition();

	UpdateFacingDirection();

	Flight();

	UpdatePlayerDopplerShift( );

	FireWeapons();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdatePlayerDopplerShift( )
{
	// -----------------------------
	// make rotor, engine sounds
	// -----------------------------
	if (m_iSoundState == 0)
	{
		// Sound startup.
		InitializeRotorSound();
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		// FIXME: this isn't the correct way to find a player!!!
		pPlayer = gEntList.FindEntityByName( NULL, "!player" );
		if (pPlayer)
		{
			Vector dir;
			VectorSubtract( pPlayer->GetAbsOrigin(), GetAbsOrigin(), dir );
			VectorNormalize(dir);

#if 1
			float velReceiver = DotProduct( pPlayer->GetAbsVelocity(), dir );
			float velTransmitter = -DotProduct( GetAbsVelocity(), dir );
			// speed of sound == 13049in/s
			int iPitch = 100 * ((1 - velReceiver / 13049) / (1 + velTransmitter / 13049));
#else
			// This is a bogus doppler shift, but I like it better
			float relV = DotProduct( GetAbsVelocity() - pPlayer->GetAbsVelocity(), dir );
			int iPitch = (int)(100 + relV / 50.0);
#endif

			// clamp pitch shifts
			if (iPitch > 250)
			{
				iPitch = 250;
			}
			if (iPitch < 50)
			{
				iPitch = 50;
			}

			UpdateRotorSoundPitch( iPitch );
			// Msg( "Pitch:%d\n", iPitch );
		}
		else
		{
			Msg( "Chopper didn't find a player!\n" );
		}
	}
}


//-----------------------------------------------------------------------------
// Computes the actual position to fly to
//-----------------------------------------------------------------------------
void CBaseHelicopter::ComputeActualTargetPosition( float flSpeed, float flTime, float flPerpDist, Vector *pDest, bool bApplyNoise )
{
	// This is used to make the helicopter drift around a bit.
	if ( bApplyNoise && m_flRandomOffsetTime <= gpGlobals->curtime )
	{
		m_vecRandomOffset.Random( -25.0f, 25.0f );
		m_flRandomOffsetTime = gpGlobals->curtime + 1.0f;
	}

	if ( IsLeading() && GetEnemy() && IsOnPathTrack() )
	{
		ComputePointAlongCurrentPath( flSpeed * flTime, flPerpDist, pDest );
		*pDest += m_vecRandomOffset;
		return;
	}

	*pDest = GetDesiredPosition() - GetAbsOrigin();
	float flDistToDesired = pDest->Length();
	if (flDistToDesired > flSpeed * flTime)
	{
		float scale = flSpeed * flTime / flDistToDesired;
		*pDest *= scale;
	}
	else if ( IsOnPathTrack() )
	{
		// Blend in a fake destination point based on the dest velocity 
		Vector vecDestVelocity;
		ComputeNormalizedDestVelocity( &vecDestVelocity );
		vecDestVelocity *= flSpeed;

		float flBlendFactor = 1.0f - flDistToDesired / (flSpeed * flTime);
		VectorMA( *pDest, flTime * flBlendFactor, vecDestVelocity, *pDest );
	}

	*pDest += GetAbsOrigin();

	if ( bApplyNoise )
	{
		//	ComputePointAlongCurrentPath( flSpeed * flTime, flPerpDist, pDest );
		*pDest += m_vecRandomOffset;
	}
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CBaseHelicopter::Flight( void )
{
	if( GetFlags() & FL_ONGROUND )
	{
		//This would be really bad.
		SetGroundEntity( NULL );
	}

	// Generic speed up
	if (m_flGoalSpeed < GetMaxSpeed())
	{
		m_flGoalSpeed += GetAcceleration();
	}
	
	//NDebugOverlay::Line(GetAbsOrigin(), m_vecDesiredPosition, 0,0,255, true, 0.1);

	// tilt model 5 degrees (why?! sjb)
	QAngle vecAdj = QAngle( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	Vector forward, right, up;
	AngleVectors( GetLocalAngles() + GetLocalAngularVelocity() * 2 + vecAdj, &forward, &right, &up );

	// Vector vecEst1 = GetLocalOrigin() + GetAbsVelocity() + up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_vecDesiredPosition - vecEst1, right );
	QAngle angVel = GetLocalAngularVelocity();
	float flSide = DotProduct( m_vecDesiredFaceDir, right );
	if (flSide < 0)
	{
		if (angVel.y < 60)
		{
			angVel.y += 8;
		}
	}
	else
	{
		if (angVel.y > -60)
		{
			angVel.y -= 8;
		}
	}

	angVel.y *= ( 0.98 ); // why?! (sjb)

	// estimate where I'll be in two seconds
	AngleVectors( GetLocalAngles() + angVel * 1 + vecAdj, NULL, NULL, &up );
	Vector vecEst = GetAbsOrigin() + GetAbsVelocity() * 2.0 + up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	AngleVectors( GetLocalAngles() + vecAdj, &forward, &right, &up );
	
	Vector vecImpulse( 0, 0, 0 );
	vecImpulse.x += up.x * m_flForce;
	vecImpulse.y += up.y * m_flForce;
	vecImpulse.z += up.z * m_flForce;

	// add gravity
	vecImpulse.z -= 38.4; // 32ft/sec
	ApplyAbsVelocityImpulse( vecImpulse );

	float flSpeed = GetAbsVelocity().Length();
	float flDir = DotProduct( Vector( forward.x, forward.y, 0 ), Vector( GetAbsVelocity().x, GetAbsVelocity().y, 0 ) );
	if (flDir < 0)
	{
		flSpeed = -flSpeed;
	}

	float flDist = DotProduct( GetDesiredPosition() - vecEst, forward );

	// float flSlip = DotProduct( GetAbsVelocity(), right );
	float flSlip = -DotProduct( GetDesiredPosition() - vecEst, right );

	// fly sideways
	if (flSlip > 0)
	{
		if (GetLocalAngles().z > -30 && angVel.z > -15)
			angVel.z -= 4;
		else
			angVel.z += 2;
	}
	else
	{
		if (GetLocalAngles().z < 30 && angVel.z < 15)
			angVel.z += 4;
		else
			angVel.z -= 2;
	}

	// These functions contain code Ken wrote that used to be right here as part of the flight model,
	// but we want different helicopter vehicles to have different drag characteristics, so I made
	// them virtual functions (sjb)
	ApplySidewaysDrag( right );
	ApplyGeneralDrag();
	
	// apply power to stay correct height
	// FIXME: these need to be per class variables
#define MAX_FORCE		80
#define FORCE_POSDELTA	12	
#define FORCE_NEGDELTA	8

	if (m_flForce < MAX_FORCE && vecEst.z < GetDesiredPosition().z) 
	{
		m_flForce += FORCE_POSDELTA;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > GetDesiredPosition().z) 
			m_flForce -= FORCE_NEGDELTA;
	}
	
	// pitch forward or back to get to target
	//-----------------------------------------
	// Pitch is reversed since Half-Life! (sjb)
	//-----------------------------------------
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && GetLocalAngles().x + angVel.x < 40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		angVel.x += 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && GetLocalAngles().x + angVel.x  > -20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		angVel.x -= 12.0;
	}
	else if (GetLocalAngles().x + angVel.x < 0)
	{
		// ALERT( at_console, "f " );
		angVel.x += 4.0;
	}
	else if (GetLocalAngles().x + angVel.x > 0)
	{
		// ALERT( at_console, "b " );
		angVel.x -= 4.0;
	}

	SetLocalAngularVelocity( angVel );
	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", GetAbsOrigin().x, GetAbsVelocity().x, flDist, flSpeed, GetLocalAngles().x, m_vecAngVelocity.x, m_flForce ); 
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", GetAbsOrigin().z, GetAbsVelocity().z, vecEst.z, m_vecDesiredPosition.z, m_flForce ); 
}


//------------------------------------------------------------------------------
// Updates the rotor wash volume
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateRotorWashVolume()
{
	if ( !m_pRotorSound )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	float flVolDelta = GetRotorVolume()	- controller.SoundGetVolume( m_pRotorSound );
	if ( flVolDelta )
	{
		// We can change from 0 to 1 in 3 seconds. 
		// Figure out how many seconds flVolDelta will take.
		float flRampTime = fabs( flVolDelta ) * 3.0f; 
		controller.SoundChangeVolume( m_pRotorSound, GetRotorVolume(), flRampTime );
	}
}


//------------------------------------------------------------------------------
// For scripted times where it *has* to shoot
//------------------------------------------------------------------------------
float CBaseHelicopter::GetRotorVolume( void )
{
	return m_bSuppressSound ? 0.0f : 1.0f;
}


//-----------------------------------------------------------------------------
// Rotor sound
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputEnableRotorSound( inputdata_t &inputdata )
{
	m_bSuppressSound = false;
}

void CBaseHelicopter::InputDisableRotorSound( inputdata_t &inputdata )
{
	m_bSuppressSound = true;
}


//-----------------------------------------------------------------------------
// Purpose: Marks the entity for deletion
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputKill( inputdata_t &inputdata )
{
	StopRotorWash();

	m_bSuppressSound = true;
	SetContextThink( &CBaseHelicopter::DelayedKillThink, gpGlobals->curtime + 3.0f, s_pDelayedKillThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::StopRotorWash( void )
{
	if ( m_hRotorWash )
	{
		UTIL_Remove( m_hRotorWash );
		m_hRotorWash = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Marks the entity for deletion
//-----------------------------------------------------------------------------
void CBaseHelicopter::DelayedKillThink( )
{
	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	UTIL_Remove( this );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pRotorSound )
	{
		// Get the rotor sound started up.
		controller.Play( m_pRotorSound, 0.0, 100 );
		UpdateRotorWashVolume();
	}

	if ( m_pRotorBlast )
	{
		// Start the blast sound and then immediately drop it to 0 (starting it at 0 wouldn't start it)
		controller.Play( m_pRotorBlast, 1.0, 100 );
		controller.SoundChangeVolume(m_pRotorBlast, 0, 0.0);
	}

	m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateRotorSoundPitch( int iPitch )
{
	if (m_pRotorSound)
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pRotorSound, iPitch, 0.1 );
		UpdateRotorWashVolume();
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->GetSolid() == SOLID_BSP) 
	{
//		trace_t tr;
//		tr = CBaseEntity::GetTouchTrace();

		// UNDONE, do a real bounce
		// FIXME: This causes bad problems, so we just ignore it right now
		//ApplyAbsVelocityImpulse( tr.plane.normal * (GetAbsVelocity().Length() + 200) );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->GetSolid() == SOLID_BSP) 
	{
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::DyingThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	SetLocalAngularVelocity( GetLocalAngularVelocity() * 1.02 );
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::DrawDebugGeometryOverlays(void) 
{
	if (m_pfnThink!= NULL)
	{
		// ------------------------------
		// Draw route if requested
		// ------------------------------
		if (m_debugOverlays & OVERLAY_NPC_ROUTE_BIT)
		{
			NDebugOverlay::Line(GetAbsOrigin(), GetDesiredPosition(), 0,0,255, true, 0);
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Take no damage from trace attacks unless it's blast damage. RadiusDamage() sometimes calls
	// TraceAttack() as a means for delivering blast damage. Usually when the explosive penetrates
	// the target. (RPG missiles do this sometimes).
	if( info.GetDamageType() & (DMG_BLAST|DMG_AIRBOAT) )
	{
		BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::NullThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.5f );
}


void CBaseHelicopter::Startup( void )
{
	StopRotorWash();

	if ( !( m_spawnflags & SF_NOROTORWASH ) )
	{
		 m_hRotorWash = CreateRotorWashEmitter( GetAbsOrigin(), GetAbsAngles(), this, BASECHOPPER_WASH_ALTITUDE );
	}

	// Fade in the blades
	m_flStartupTime = gpGlobals->curtime;

	m_flGoalSpeed = m_flInitialSpeed;
	SetThink( &CBaseHelicopter::HelicopterThink );
	SetTouch( &CBaseHelicopter::FlyTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flRotorWashEntitySearchTime = gpGlobals->curtime;
	SetContextThink( &CBaseHelicopter::RotorWashThink, gpGlobals->curtime, s_pRotorWashThinkContext );
}

void CBaseHelicopter::StopLoopingSounds()
{
	// Kill the rotor sounds
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );
	controller.SoundDestroy( m_pRotorBlast );
	m_pRotorSound = NULL;
	m_pRotorBlast = NULL;

	BaseClass::StopLoopingSounds();
}

void CBaseHelicopter::Event_Killed( const CTakeDamageInfo &info )
{
	m_lifeState			= LIFE_DYING;

	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetGravity( UTIL_ScaleForGravity( 240 ) );	// use a lower gravity

	StopLoopingSounds();

	UTIL_SetSize( this, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( &CBaseHelicopter::CallDyingThink );
	SetTouch( &CBaseHelicopter::CrashTouch );

	SetNextThink( gpGlobals->curtime + 0.1f );
	m_iHealth = 0;
	m_takedamage = DAMAGE_NO;

/*
	if (m_spawnflags & SF_NOWRECKAGE)
	{
		m_flNextRocket = gpGlobals->curtime + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->curtime + 15.0;
	}
*/	
	StopRotorWash();

	m_OnDeath.FireOutput( info.GetAttacker(), this );
}


void CBaseHelicopter::GibMonster( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Call Startup for a helicopter that's been flagged to start disabled
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputActivate( inputdata_t &inputdata )
{
	if( m_spawnflags & SF_AWAITINPUT )
	{
		Startup();

		// Now clear the spawnflag to protect from
		// subsequent calls.
		m_spawnflags &= ~SF_AWAITINPUT;
	}
}

//------------------------------------------------------------------------------
// Purpose : Turn the gun on
//------------------------------------------------------------------------------
void CBaseHelicopter::InputGunOn( inputdata_t &inputdata )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_GUN_ON;
}

//-----------------------------------------------------------------------------
// Purpose: Turn the gun off
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputGunOff( inputdata_t &inputdata )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;
}

//------------------------------------------------------------------------------
// Purpose : Turn the missile on
//------------------------------------------------------------------------------
void CBaseHelicopter::InputMissileOn( inputdata_t &inputdata )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_MISSILE_ON;
}

//-----------------------------------------------------------------------------
// Purpose: Turn the missile off
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputMissileOff( inputdata_t &inputdata )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_MISSILE_ON;
}


//-----------------------------------------------------------------------------
// Enable, disable rotor wash
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputEnableRotorWash( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_NOROTORWASH;
}

void CBaseHelicopter::InputDisableRotorWash( inputdata_t &inputdata )
{
	m_spawnflags |= SF_NOROTORWASH;
}


//-----------------------------------------------------------------------------
// Causes the helicopter to immediately accelerate to its desired velocity
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputMoveTopSpeed( inputdata_t &inputdata )
{
	Vector vecVelocity;
	ComputeActualTargetPosition( GetMaxSpeed(), 1.0f, 0.0f, &vecVelocity, false );
	vecVelocity -= GetAbsOrigin();

	float flLength = VectorNormalize( vecVelocity );
	if (flLength < 1e-3)
	{
		GetVectors( &vecVelocity, NULL, NULL );
	}

	vecVelocity *= GetMaxSpeed();
	SetAbsVelocity( vecVelocity );
}

//-----------------------------------------------------------------------------
// Cause helicopter to immediately accelerate to specified velocity
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputMoveSpecifiedSpeed( inputdata_t &inputdata )
{
	Vector vecVelocity;
	ComputeActualTargetPosition( GetMaxSpeed(), 1.0f, 0.0f, &vecVelocity, false );
	vecVelocity -= GetAbsOrigin();

	float flLength = VectorNormalize( vecVelocity );
	if (flLength < 1e-3)
	{
		GetVectors( &vecVelocity, NULL, NULL );
	}

	float flSpeed = inputdata.value.Float();

	vecVelocity *= flSpeed;
	SetAbsVelocity( vecVelocity );
}

//------------------------------------------------------------------------------
// Input values
//------------------------------------------------------------------------------
void CBaseHelicopter::InputSetAngles( inputdata_t &inputdata )
{
	const char *pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector( angles.Base(), pAngles );
	SetAbsAngles( angles );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::ApplySidewaysDrag( const Vector &vecRight )
{
	Vector vecNewVelocity = GetAbsVelocity();
	vecNewVelocity.x *= 1.0 - fabs( vecRight.x ) * 0.05;
	vecNewVelocity.y *= 1.0 - fabs( vecRight.y ) * 0.05;
	vecNewVelocity.z *= 1.0 - fabs( vecRight.z ) * 0.05;
	SetAbsVelocity( vecNewVelocity );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::ApplyGeneralDrag( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	vecNewVelocity *= 0.995;
	SetAbsVelocity( vecNewVelocity );
}
	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
bool CBaseHelicopter::ChooseEnemy( void )
{
	// See if there's a new enemy.
	CBaseEntity *pNewEnemy;

	pNewEnemy = BestEnemy();

	if ( pNewEnemy != GetEnemy() )
	{
		if ( pNewEnemy != NULL )
		{
			// New enemy! Clear the timers and set conditions.
			SetEnemy( pNewEnemy );
			m_flLastSeen = m_flPrevSeen = gpGlobals->curtime;
		}
		else
		{
			SetEnemy( NULL );
			SetState( NPC_STATE_ALERT );
		}
		return true;
	}
	else
	{
		ClearCondition( COND_NEW_ENEMY );
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// -------------------
	// If enemy is dead
	// -------------------
	if ( !pEnemy->IsAlive() )
	{
		SetCondition( COND_ENEMY_DEAD );
		ClearCondition( COND_SEE_ENEMY );
		ClearCondition( COND_ENEMY_OCCLUDED );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInfo - 
//			bAlways - 
//-----------------------------------------------------------------------------
void CBaseHelicopter::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Make our smoke trail always come with us
	if ( m_hRotorWash )
	{
		m_hRotorWash->SetTransmit( pInfo, bAlways );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ExpandBBox(Vector &vecMins, Vector &vecMaxs)
{
	// expand for *any* rotation
	float maxval = 0;
	for (int i = 0; i < 3; i++)
	{
		float v = fabs( vecMins[i]);
		if (v > maxval)
			maxval = v;

		v = fabs( vecMaxs[i]);
		if (v > maxval)
			maxval = v;
	}

	vecMins.Init(-maxval, -maxval, -maxval);
	vecMaxs.Init(maxval, maxval, maxval);
}
