//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: combine ball -	can be held by the super physcannon and launched
//							by the AR2's alt-fire
//
//=============================================================================//

#include "cbase.h"
#include "prop_combine_ball.h"
#include "props.h"
#include "explode.h"
#include "saverestore_utlvector.h"
#include "hl2_shareddefs.h"
#include "materialsystem/imaterial.h"
#include "beam_flags.h"
#include "physics_prop_ragdoll.h"
#include "soundent.h"
#include "soundenvelope.h"
#include "te_effect_dispatch.h"
#include "ai_basenpc.h"
#include "npc_bullseye.h"
#include "filters.h"
#include "SpriteTrail.h"
#include "decals.h"
#include "hl2_player.h"
#include "eventqueue.h"
#include "physics_collisionevent.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PROP_COMBINE_BALL_MODEL	"models/effects/combineball.mdl"
#define PROP_COMBINE_BALL_SPRITE_TRAIL "sprites/combineball_trail_black_1.vmt" 

#define PROP_COMBINE_BALL_LIFETIME	4.0f	// Seconds

#define PROP_COMBINE_BALL_HOLD_DISSOLVE_TIME	8.0f

#define SF_COMBINE_BALL_BOUNCING_IN_SPAWNER		0x10000

#define	MAX_COMBINEBALL_RADIUS	12

ConVar	sk_npc_dmg_combineball( "sk_npc_dmg_combineball","15", FCVAR_REPLICATED);
ConVar	sk_combineball_guidefactor( "sk_combineball_guidefactor","0.5", FCVAR_REPLICATED);
ConVar	sk_combine_ball_search_radius( "sk_combine_ball_search_radius", "512", FCVAR_REPLICATED);
ConVar	sk_combineball_seek_angle( "sk_combineball_seek_angle","15.0", FCVAR_REPLICATED);
ConVar	sk_combineball_seek_kill( "sk_combineball_seek_kill","0", FCVAR_REPLICATED);

// For our ring explosion
int s_nExplosionTexture = -1;

//-----------------------------------------------------------------------------
// Context think
//-----------------------------------------------------------------------------
static const char *s_pWhizThinkContext = "WhizThinkContext";
static const char *s_pHoldDissolveContext = "HoldDissolveContext";
static const char *s_pExplodeTimerContext = "ExplodeTimerContext";
static const char *s_pAnimThinkContext = "AnimThinkContext";
static const char *s_pCaptureContext = "CaptureContext";
static const char *s_pRemoveContext = "RemoveContext";

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : radius - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CreateCombineBall( const Vector &origin, const Vector &velocity, float radius, float mass, float lifetime, CBaseEntity *pOwner )
{
	CPropCombineBall *pBall = static_cast<CPropCombineBall*>( CreateEntityByName( "prop_combine_ball" ) );
	pBall->SetRadius( radius );

	pBall->SetAbsOrigin( origin );
	pBall->SetOwnerEntity( pOwner );
	pBall->SetOriginalOwner( pOwner );

	pBall->SetAbsVelocity( velocity );
	pBall->Spawn();

	pBall->SetState( CPropCombineBall::STATE_THROWN );
	pBall->SetSpeed( velocity.Length() );

	pBall->EmitSound( "NPC_CombineBall.Launch" );

	PhysSetGameFlags( pBall->VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );

	pBall->StartWhizSoundThink();

	pBall->SetMass( mass );
	pBall->StartLifetime( lifetime );
	pBall->SetWeaponLaunched( true );

	return pBall;
}

//-----------------------------------------------------------------------------
// Purpose: Allows game to know if the physics object should kill allies or not
//-----------------------------------------------------------------------------
CBasePlayer *CPropCombineBall::HasPhysicsAttacker( float dt )
{
	// Must have an owner
	if ( GetOwnerEntity() == NULL )
		return NULL;

	// Must be a player
	if ( GetOwnerEntity()->IsPlayer() == false )
		return NULL;

	// We don't care about the time passed in
	return static_cast<CBasePlayer *>(GetOwnerEntity());
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether a physics object is a combine ball or not
// Input  : *pObj - Object to test
// Output : Returns true on success, false on failure.
// Notes  : This function cannot identify a combine ball that is held by
//			the physcannon because any object held by the physcannon is
//			COLLISIONGROUP_DEBRIS.
//-----------------------------------------------------------------------------
bool UTIL_IsCombineBall( CBaseEntity *pEntity )
{
	// Must be the correct collision group
	if ( pEntity->GetCollisionGroup() != HL2COLLISION_GROUP_COMBINE_BALL )
		return false;

	//NOTENOTE: This allows ANY combine ball to pass the test

	/*
	CPropCombineBall *pBall = dynamic_cast<CPropCombineBall *>(pEntity);

	if ( pBall && pBall->WasWeaponLaunched() )
		return false;
	*/

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether a physics object is an AR2 combine ball or not
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsAR2CombineBall( CBaseEntity *pEntity )
{
	// Must be the correct collision group
	if ( pEntity->GetCollisionGroup() != HL2COLLISION_GROUP_COMBINE_BALL )
		return false;

	CPropCombineBall *pBall = dynamic_cast<CPropCombineBall *>(pEntity);

	if ( pBall && pBall->WasWeaponLaunched() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Uses a deeper casting check to determine if pEntity is a combine
//			ball. This function exists because the normal (much faster) check
//			in UTIL_IsCombineBall() can never identify a combine ball held by
//			the physcannon because the physcannon changes the held entity's
//			collision group.
// Input  : *pEntity - Entity to check 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsCombineBallDefinite( CBaseEntity *pEntity )
{
	CPropCombineBall *pBall = dynamic_cast<CPropCombineBall *>(pEntity);

	return pBall != NULL;
}

//-----------------------------------------------------------------------------
//
// Spawns combine balls
//
//-----------------------------------------------------------------------------
#define SF_SPAWNER_START_DISABLED 0x1000
#define SF_SPAWNER_POWER_SUPPLY 0x2000



//-----------------------------------------------------------------------------
// Implementation of CPropCombineBall
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( prop_combine_ball, CPropCombineBall );

//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPropCombineBall )

	DEFINE_FIELD( m_flLastBounceTime, FIELD_TIME ),
	DEFINE_FIELD( m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_nState, FIELD_CHARACTER ),
	DEFINE_FIELD( m_pGlowTrail, FIELD_CLASSPTR ),
	DEFINE_SOUNDPATCH( m_pHoldingSound ),
	DEFINE_FIELD( m_bFiredGrabbedOutput, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHeld, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLaunched, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bStruckEntity, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWeaponLaunched, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bForward, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flSpeed, FIELD_FLOAT ),

	DEFINE_FIELD( m_flNextDamageTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastCaptureTime, FIELD_TIME ),
	DEFINE_FIELD( m_bCaptureInProgress, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nBounceCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxBounces,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bBounceDie,	FIELD_BOOLEAN ),
	
	
	DEFINE_FIELD( m_hSpawner, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( ExplodeThink ),
	DEFINE_THINKFUNC( WhizSoundThink ),
	DEFINE_THINKFUNC( DieThink ),
	DEFINE_THINKFUNC( DissolveThink ),
	DEFINE_THINKFUNC( DissolveRampSoundThink ),
	DEFINE_THINKFUNC( AnimThink ),
	DEFINE_THINKFUNC( CaptureBySpawner ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FadeAndRespawn", InputFadeAndRespawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Socketed", InputSocketed ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPropCombineBall, DT_PropCombineBall )
	SendPropBool( SENDINFO( m_bEmit ) ),
	SendPropFloat( SENDINFO( m_flRadius ), 0, SPROP_NOSCALE ),
	SendPropBool( SENDINFO( m_bHeld ) ),
	SendPropBool( SENDINFO( m_bLaunched ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Gets at the spawner
//-----------------------------------------------------------------------------
CFuncCombineBallSpawner *CPropCombineBall::GetSpawner()
{
	return m_hSpawner;
}

//-----------------------------------------------------------------------------
// Precache 
//-----------------------------------------------------------------------------
void CPropCombineBall::Precache( void )
{
	//NOTENOTE: We don't call into the base class because it chains multiple 
	//			precaches we don't need to incur

	PrecacheModel( PROP_COMBINE_BALL_MODEL );
	PrecacheModel( PROP_COMBINE_BALL_SPRITE_TRAIL );

	s_nExplosionTexture = PrecacheModel( "sprites/lgtning.vmt" );

	PrecacheScriptSound( "NPC_CombineBall.Launch" );
	PrecacheScriptSound( "NPC_CombineBall.KillImpact" );

	if ( hl2_episodic.GetBool() )
	{
		PrecacheScriptSound( "NPC_CombineBall_Episodic.Explosion" );
		PrecacheScriptSound( "NPC_CombineBall_Episodic.WhizFlyby" );
		PrecacheScriptSound( "NPC_CombineBall_Episodic.Impact" );
	}
	else
	{
		PrecacheScriptSound( "NPC_CombineBall.Explosion" );
		PrecacheScriptSound( "NPC_CombineBall.WhizFlyby" );
		PrecacheScriptSound( "NPC_CombineBall.Impact" );
	}

	PrecacheScriptSound( "NPC_CombineBall.HoldingInPhysCannon" );
}


//-----------------------------------------------------------------------------
// Spherical vphysics
//-----------------------------------------------------------------------------
bool CPropCombineBall::OverridePropdata() 
{ 
	return true; 
}


//-----------------------------------------------------------------------------
// Spherical vphysics
//-----------------------------------------------------------------------------
void CPropCombineBall::SetState( int state ) 
{ 
	if ( m_nState != state )
	{
		if ( m_nState == STATE_NOT_THROWN )
		{
			m_flLastCaptureTime = gpGlobals->curtime;
		}

		m_nState = state;
	}
}

bool CPropCombineBall::IsInField() const 
{ 
	return (m_nState == STATE_NOT_THROWN); 
}

	
//-----------------------------------------------------------------------------
// Sets the radius
//-----------------------------------------------------------------------------
void CPropCombineBall::SetRadius( float flRadius )
{
	m_flRadius = clamp( flRadius, 1, MAX_COMBINEBALL_RADIUS );
}

//-----------------------------------------------------------------------------
// Create vphysics
//-----------------------------------------------------------------------------
bool CPropCombineBall::CreateVPhysics()
{
	SetSolid( SOLID_BBOX );

	float flSize = m_flRadius;

	SetCollisionBounds( Vector(-flSize, -flSize, -flSize), Vector(flSize, flSize, flSize) );
	objectparams_t params = g_PhysDefaultObjectParams;
	params.pGameData = static_cast<void *>(this);
	int nMaterialIndex = physprops->GetSurfaceIndex("metal_bouncy");
	IPhysicsObject *pPhysicsObject = physenv->CreateSphereObject( flSize, nMaterialIndex, GetAbsOrigin(), GetAbsAngles(), &params, false );
	if ( !pPhysicsObject )
		return false;

	VPhysicsSetObject( pPhysicsObject );
	SetMoveType( MOVETYPE_VPHYSICS );
	pPhysicsObject->Wake();

	pPhysicsObject->SetMass( 750.0f );
	pPhysicsObject->EnableGravity( false );
	pPhysicsObject->EnableDrag( false );

	float flDamping = 0.0f;
	float flAngDamping = 0.5f;
	pPhysicsObject->SetDamping( &flDamping, &flAngDamping );
	pPhysicsObject->SetInertia( Vector( 1e30, 1e30, 1e30 ) );

	if( WasFiredByNPC() )
	{
		// Don't do impact damage. Just touch them and do your dissolve damage and move on.
		PhysSetGameFlags( pPhysicsObject, FVPHYSICS_NO_NPC_IMPACT_DMG );
	}
	else
	{
		PhysSetGameFlags( pPhysicsObject, FVPHYSICS_DMG_DISSOLVE | FVPHYSICS_HEAVY_OBJECT );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Spawn: 
//-----------------------------------------------------------------------------
void CPropCombineBall::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( PROP_COMBINE_BALL_MODEL );

	if( ShouldHitPlayer() )
	{
		// This allows the combine ball to hit the player.
		SetCollisionGroup( HL2COLLISION_GROUP_COMBINE_BALL_NPC );
	}
	else
	{
		SetCollisionGroup( HL2COLLISION_GROUP_COMBINE_BALL );
	}

	CreateVPhysics();

	Vector vecAbsVelocity = GetAbsVelocity();
	VPhysicsGetObject()->SetVelocity( &vecAbsVelocity, NULL );

	m_nState = STATE_NOT_THROWN;
	m_flLastBounceTime = -1.0f;
	m_bFiredGrabbedOutput = false;
	m_bForward = true;
	m_bCaptureInProgress = false;

	// No shadow!
	AddEffects( EF_NOSHADOW );

	// Start up the eye trail
	m_pGlowTrail = CSpriteTrail::SpriteTrailCreate( PROP_COMBINE_BALL_SPRITE_TRAIL, GetAbsOrigin(), false );
	
	if ( m_pGlowTrail != NULL )
	{
		m_pGlowTrail->FollowEntity( this );
		m_pGlowTrail->SetTransparency( kRenderTransAdd, 0, 0, 0, 255, kRenderFxNone );
		m_pGlowTrail->SetStartWidth( m_flRadius );
		m_pGlowTrail->SetEndWidth( 0 );
		m_pGlowTrail->SetLifeTime( 0.1f );
		m_pGlowTrail->TurnOff();
	}

	m_bEmit = true;
	m_bHeld = false;
	m_bLaunched = false;
	m_bStruckEntity = false;
	m_bWeaponLaunched = false;

	m_flNextDamageTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::StartAnimating( void )
{
	// Start our animation cycle. Use the random to avoid everything thinking the same frame
	SetContextThink( &CPropCombineBall::AnimThink, gpGlobals->curtime + random->RandomFloat( 0.0f, 0.1f), s_pAnimThinkContext );

	int nSequence = LookupSequence( "idle" );

	SetCycle( 0 );
	m_flAnimTime = gpGlobals->curtime;
	ResetSequence( nSequence );
	ResetClientsideFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::StopAnimating( void )
{
	SetContextThink( NULL, gpGlobals->curtime, s_pAnimThinkContext );
}

//-----------------------------------------------------------------------------
// Put it into the spawner
//-----------------------------------------------------------------------------
void CPropCombineBall::CaptureBySpawner( )
{
	m_bCaptureInProgress = true;
	m_bFiredGrabbedOutput = false;

	// Slow down the ball
	Vector vecVelocity;
	VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );
	float flSpeed = VectorNormalize( vecVelocity );
	if ( flSpeed > 25.0f )
	{
		vecVelocity *= flSpeed * 0.4f;
		VPhysicsGetObject()->SetVelocity( &vecVelocity, NULL );

		// Slow it down until we can set its velocity ok
		SetContextThink( &CPropCombineBall::CaptureBySpawner, gpGlobals->curtime + 0.01f,	s_pCaptureContext );
		return;
	}

	// Ok, we're captured
	SetContextThink( NULL, gpGlobals->curtime,	s_pCaptureContext );
	ReplaceInSpawner( GetSpawner()->GetBallSpeed() );
	m_bCaptureInProgress = false;
}

//-----------------------------------------------------------------------------
// Put it into the spawner
//-----------------------------------------------------------------------------
void CPropCombineBall::ReplaceInSpawner( float flSpeed )
{
	m_bForward = true;
	m_nState = STATE_NOT_THROWN;

	// Prevent it from exploding
	ClearLifetime( );

	// Stop whiz noises
	SetContextThink( NULL, gpGlobals->curtime, s_pWhizThinkContext );

	// Slam velocity to what the field wants
	Vector vecTarget, vecVelocity;
	GetSpawner()->GetTargetEndpoint( m_bForward, &vecTarget );
	VectorSubtract( vecTarget, GetAbsOrigin(), vecVelocity );
	VectorNormalize( vecVelocity );
	vecVelocity *= flSpeed; 
	VPhysicsGetObject()->SetVelocity( &vecVelocity, NULL );
	
	// Set our desired speed to the spawner's speed. This will be
	// our speed on our first bounce in the field.
	SetSpeed( flSpeed );
}


float CPropCombineBall::LastCaptureTime() const
{
	if ( IsInField() || IsBeingCaptured() )
		return gpGlobals->curtime;

	return m_flLastCaptureTime;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the lifetime countdown on the ball
// Input  : flDuration - number of seconds to live before exploding
//-----------------------------------------------------------------------------
void CPropCombineBall::StartLifetime( float flDuration )
{
	SetContextThink( &CPropCombineBall::ExplodeThink, gpGlobals->curtime + flDuration, s_pExplodeTimerContext );
}

//-----------------------------------------------------------------------------
// Purpose: Stops the lifetime on the ball from expiring
//-----------------------------------------------------------------------------
void CPropCombineBall::ClearLifetime( void )
{
	// Prevent it from exploding
	SetContextThink( NULL, gpGlobals->curtime, s_pExplodeTimerContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mass - 
//-----------------------------------------------------------------------------
void CPropCombineBall::SetMass( float mass )
{
	IPhysicsObject *pObj = VPhysicsGetObject();

	if ( pObj != NULL )
	{
		pObj->SetMass( mass );
		pObj->SetInertia( Vector( 500, 500, 500 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropCombineBall::ShouldHitPlayer() const 
{ 
	if ( GetOwnerEntity() ) 
	{
		CAI_BaseNPC *pNPC = GetOwnerEntity()->MyNPCPointer();
		if ( pNPC && !pNPC->IsPlayerAlly() )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::InputKill( inputdata_t &inputdata )
{
	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	UTIL_Remove( this );

	NotifySpawnerOfRemoval();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::InputSocketed( inputdata_t &inputdata )
{
	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	// if our owner is a player, tell them we were socketed
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( pOwner );
	if ( pPlayer )
	{
		pPlayer->CombineBallSocketed( this );
	}

	UTIL_Remove( this );

	NotifySpawnerOfRemoval();
}

//-----------------------------------------------------------------------------
// Cleanup. 
//-----------------------------------------------------------------------------
void CPropCombineBall::UpdateOnRemove()
{
	if ( m_pGlowTrail != NULL )
	{
		UTIL_Remove( m_pGlowTrail );
		m_pGlowTrail = NULL;
	}

	//Sigh... this is the only place where I can get a message after the ball is done dissolving.
	if ( hl2_episodic.GetBool()  )
	{
		if ( IsDissolving() )
		{
			if ( GetSpawner() )
			{
				GetSpawner()->BallGrabbed( this );
				NotifySpawnerOfRemoval();
			}
		}
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::ExplodeThink( void )
{
	DoExplosion();	
}

//-----------------------------------------------------------------------------
// Purpose: Tell the respawner to make a new one
//-----------------------------------------------------------------------------
void CPropCombineBall::NotifySpawnerOfRemoval( void )
{
	if ( GetSpawner() )
	{
		GetSpawner()->RespawnBallPostExplosion();
	}
}

//-----------------------------------------------------------------------------
// Fade out. 
//-----------------------------------------------------------------------------
void CPropCombineBall::DieThink()
{
	if ( GetSpawner() )
	{
		//Let the spawner know we died so it does it's thing
		if( hl2_episodic.GetBool() && IsInField() )
		{
			GetSpawner()->BallGrabbed( this );
		}

		GetSpawner()->RespawnBall( 0.1 );
	}

	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Fade out. 
//-----------------------------------------------------------------------------
void CPropCombineBall::FadeOut( float flDuration )
{
	AddSolidFlags( FSOLID_NOT_SOLID );

	// Start up the eye trail
	if ( m_pGlowTrail != NULL )
	{
		m_pGlowTrail->SetBrightness( 0, flDuration );
	}

	SetThink( &CPropCombineBall::DieThink );
	SetNextThink( gpGlobals->curtime + flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::StartWhizSoundThink( void )
{
	SetContextThink( &CPropCombineBall::WhizSoundThink, gpGlobals->curtime + 2.0f * TICK_INTERVAL, s_pWhizThinkContext );
}

//-----------------------------------------------------------------------------
// Danger sounds. 
//-----------------------------------------------------------------------------
void CPropCombineBall::WhizSoundThink()
{
	Vector vecPosition, vecVelocity;
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	
	if ( pPhysicsObject == NULL )
	{
		//NOTENOTE: We should always have been created at this point
		Assert( 0 );
		SetContextThink( &CPropCombineBall::WhizSoundThink, gpGlobals->curtime + 2.0f * TICK_INTERVAL, s_pWhizThinkContext );
		return;
	}

	pPhysicsObject->GetPosition( &vecPosition, NULL );
	pPhysicsObject->GetVelocity( &vecVelocity, NULL );
	
	if ( gpGlobals->maxClients == 1 )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
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
					CPASAttenuationFilter filter( vecPosition, ATTN_NORM );

					EmitSound_t ep;
					ep.m_nChannel = CHAN_STATIC;
					if ( hl2_episodic.GetBool() )
					{
						ep.m_pSoundName = "NPC_CombineBall_Episodic.WhizFlyby";
					}
					else
					{
						ep.m_pSoundName = "NPC_CombineBall.WhizFlyby";
					}
					ep.m_flVolume = 1.0f;
					ep.m_SoundLevel = SNDLVL_NORM;

					EmitSound( filter, entindex(), ep );

					SetContextThink( &CPropCombineBall::WhizSoundThink, gpGlobals->curtime + 0.5f, s_pWhizThinkContext );
					return;
				}
			}
		}
	}

	SetContextThink( &CPropCombineBall::WhizSoundThink, gpGlobals->curtime + 2.0f * TICK_INTERVAL, s_pWhizThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::SetBallAsLaunched( void )
{
	// Give the ball a duration
	StartLifetime( PROP_COMBINE_BALL_LIFETIME );

	m_bHeld = false;
	m_bLaunched = true;
	SetState( STATE_THROWN );

	VPhysicsGetObject()->SetMass( 750.0f );
	VPhysicsGetObject()->SetInertia( Vector( 1e30, 1e30, 1e30 ) );

	StopLoopingSounds();
	EmitSound( "NPC_CombineBall.Launch" );
	
	WhizSoundThink();
}

//-----------------------------------------------------------------------------
// Lighten the mass so it's zippy toget to the gun
//-----------------------------------------------------------------------------
void CPropCombineBall::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	CDefaultPlayerPickupVPhysics::OnPhysGunPickup( pPhysGunUser, reason );

	if ( m_nMaxBounces == -1 )
	{
		m_nMaxBounces = 0;
	}

	if ( !m_bFiredGrabbedOutput )
	{
		if ( GetSpawner() )
		{
			GetSpawner()->BallGrabbed( this );
		}

		m_bFiredGrabbedOutput = true;
	}

	if ( m_pGlowTrail )
	{
		m_pGlowTrail->TurnOff();
		m_pGlowTrail->SetRenderColor( 0, 0, 0, 0 );
	}

	if ( reason != PUNTED_BY_CANNON )
	{
		SetState( STATE_HOLDING );
		CPASAttenuationFilter filter( GetAbsOrigin(), ATTN_NORM );
		filter.MakeReliable();
		
		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;

		if( hl2_episodic.GetBool() )
		{
			ep.m_pSoundName = "NPC_CombineBall_Episodic.HoldingInPhysCannon";
		}
		else
		{
			ep.m_pSoundName = "NPC_CombineBall.HoldingInPhysCannon";
		}

		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		// Now we own this ball
		SetPlayerLaunched( pPhysGunUser );

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pHoldingSound = controller.SoundCreate( filter, entindex(), ep );
		controller.Play( m_pHoldingSound, 1.0f, 100 ); 

		// Don't collide with anything we may have to pull the ball through
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );

		VPhysicsGetObject()->SetMass( 20.0f );
		VPhysicsGetObject()->SetInertia( Vector( 100, 100, 100 ) );

		// Make it not explode
		ClearLifetime( );

		m_bHeld = true;
		m_bLaunched = false;

		//Let the ball know is not being captured by one of those ball fields anymore.
		//
		m_bCaptureInProgress = false;


		SetContextThink( &CPropCombineBall::DissolveRampSoundThink, gpGlobals->curtime + GetBallHoldSoundRampTime(), s_pHoldDissolveContext );

		StartAnimating();
	}
	else
	{
		Vector vecVelocity;
		VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

		SetSpeed( vecVelocity.Length() );

		// Set us as being launched by the player
		SetPlayerLaunched( pPhysGunUser );

		SetBallAsLaunched();

		StopAnimating();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset the ball to be deadly to NPCs after we've picked it up
//-----------------------------------------------------------------------------
void CPropCombineBall::SetPlayerLaunched( CBasePlayer *pOwner )
{
	// Now we own this ball
	SetOwnerEntity( pOwner );
	SetWeaponLaunched( false );
	
	if( VPhysicsGetObject() )
	{
		PhysClearGameFlags( VPhysicsGetObject(), FVPHYSICS_NO_NPC_IMPACT_DMG );
		PhysSetGameFlags( VPhysicsGetObject(), FVPHYSICS_DMG_DISSOLVE | FVPHYSICS_HEAVY_OBJECT );
	}
}

//-----------------------------------------------------------------------------
// Activate death-spin!
//-----------------------------------------------------------------------------
void CPropCombineBall::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	CDefaultPlayerPickupVPhysics::OnPhysGunDrop( pPhysGunUser, Reason );

	SetState( STATE_THROWN );
	WhizSoundThink();

	m_bHeld = false;
	m_bLaunched = true;

	// Stop with the dissolving
	SetContextThink( NULL, gpGlobals->curtime, s_pHoldDissolveContext );

	// We're ready to start colliding again.
	SetCollisionGroup( HL2COLLISION_GROUP_COMBINE_BALL );

	if ( m_pGlowTrail )
	{
		m_pGlowTrail->TurnOn();
		m_pGlowTrail->SetRenderColor( 255, 255, 255, 255 );
	}

	// Set our desired speed to be launched at
	SetSpeed( 1500.0f );
	SetPlayerLaunched( pPhysGunUser );

	if ( Reason != LAUNCHED_BY_CANNON )
	{
		// Choose a random direction (forward facing)
		Vector vecForward;
		pPhysGunUser->GetVectors( &vecForward, NULL, NULL );

		QAngle shotAng;
		VectorAngles( vecForward, shotAng );

		// Offset by some small cone
		shotAng[PITCH] += random->RandomInt( -55, 55 );
		shotAng[YAW] += random->RandomInt( -55, 55 );

		AngleVectors( shotAng, &vecForward, NULL, NULL );

		vecForward *= GetSpeed();

		VPhysicsGetObject()->SetVelocity( &vecForward, &vec3_origin );
	}
	else
	{
		// This will have the consequence of making it so that the
		// ball is launched directly down the crosshair even if the player is moving.
		VPhysicsGetObject()->SetVelocity( &vec3_origin, &vec3_origin );
	}

	SetBallAsLaunched();
	StopAnimating();
}

//------------------------------------------------------------------------------
// Stop looping sounds
//------------------------------------------------------------------------------
void CPropCombineBall::StopLoopingSounds()
{
	if ( m_pHoldingSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.Shutdown( m_pHoldingSound );
		controller.SoundDestroy( m_pHoldingSound );
		m_pHoldingSound = NULL;
	}
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CPropCombineBall::DissolveRampSoundThink( )
{
	float dt = GetBallHoldDissolveTime() - GetBallHoldSoundRampTime();
	if ( m_pHoldingSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pHoldingSound, 150, dt );
	}
	SetContextThink( &CPropCombineBall::DissolveThink, gpGlobals->curtime + dt, s_pHoldDissolveContext );
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CPropCombineBall::DissolveThink( )
{
	DoExplosion();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CPropCombineBall::GetBallHoldDissolveTime()
{
	float flDissolveTime = PROP_COMBINE_BALL_HOLD_DISSOLVE_TIME;

	if( g_pGameRules->IsSkillLevel( 1 ) && hl2_episodic.GetBool() )
	{
		// Give players more time to handle/aim combine balls on Easy.
		flDissolveTime *= 1.5f;
	}

	return flDissolveTime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CPropCombineBall::GetBallHoldSoundRampTime()
{
	return GetBallHoldDissolveTime() - 1.0f;
}

//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CPropCombineBall::DoExplosion( )
{
	// don't do this twice
	if ( GetMoveType() == MOVETYPE_NONE )
		return;

	if ( PhysIsInCallback() )
	{
		g_PostSimulationQueue.QueueCall( this, &CPropCombineBall::DoExplosion );
		return;
	}
	// Tell the respawner to make a new one
	if ( GetSpawner() )
	{
		GetSpawner()->RespawnBallPostExplosion();
	}

	//Shockring
	CBroadcastRecipientFilter filter2;

	if ( OutOfBounces() == false )
	{
		if ( hl2_episodic.GetBool() )
		{
			EmitSound( "NPC_CombineBall_Episodic.Explosion" );
		}
		else
		{
			EmitSound( "NPC_CombineBall.Explosion" );
		}

		UTIL_ScreenShake( GetAbsOrigin(), 20.0f, 150.0, 1.0, 1250.0f, SHAKE_START );

		CEffectData data;

		data.m_vOrigin = GetAbsOrigin();

		DispatchEffect( "cball_explode", data );

		te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
			m_flRadius,	//start radius
			1024,		//end radius
			s_nExplosionTexture, //texture
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
			32,		//a
			0,		//speed
			FBEAM_FADEOUT
			);

		//Shockring
		te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
			m_flRadius,	//start radius
			1024,		//end radius
			s_nExplosionTexture, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.5f,		//life
			64,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			64,		//a
			0,		//speed
			FBEAM_FADEOUT
			);
	}
	else
	{
		//Shockring
		te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
			128,	//start radius
			384,		//end radius
			s_nExplosionTexture, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.25f,		//life
			48,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			64,		//a
			0,		//speed
			FBEAM_FADEOUT
			);
	}

	if( hl2_episodic.GetBool() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION, WorldSpaceCenter(), 180.0f, 0.25, this );
	}

	// Turn us off and wait because we need our trails to finish up properly
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bEmit = false;

	
	if( !m_bStruckEntity && hl2_episodic.GetBool() && GetOwnerEntity() != NULL )
	{
		// Notify the player proxy that this combine ball missed so that it can fire an output.
		CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( GetOwnerEntity() );
		if ( pPlayer )
		{
			pPlayer->MissedAR2AltFire();
		}
	}

	SetContextThink( &CPropCombineBall::SUB_Remove, gpGlobals->curtime + 0.5f, s_pRemoveContext );
	StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Enable/disable
//-----------------------------------------------------------------------------
void CPropCombineBall::InputExplode( inputdata_t &inputdata )
{
	DoExplosion();
}

//-----------------------------------------------------------------------------
// Enable/disable
//-----------------------------------------------------------------------------
void CPropCombineBall::InputFadeAndRespawn( inputdata_t &inputdata )
{
	FadeOut( 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::CollisionEventToTrace( int index, gamevcollisionevent_t *pEvent, trace_t &tr )
{
	UTIL_ClearTrace( tr );
	pEvent->pInternalData->GetSurfaceNormal( tr.plane.normal );
	pEvent->pInternalData->GetContactPoint( tr.endpos );
	tr.plane.dist = DotProduct( tr.plane.normal, tr.endpos );
	VectorMA( tr.endpos, -1.0f, pEvent->preVelocity[index], tr.startpos );
	tr.m_pEnt = pEvent->pEntities[!index];
	tr.fraction = 0.01f;	// spoof!
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CPropCombineBall::DissolveEntity( CBaseEntity *pEntity )
{
	if( pEntity->IsEFlagSet( EFL_NO_DISSOLVE ) )
		return false;

#ifdef HL2MP
	if ( pEntity->IsPlayer() )
	{
		m_bStruckEntity = true;
		return false;
	}
#endif

	if( !pEntity->IsNPC() && !(dynamic_cast<CRagdollProp*>(pEntity)) )
		return false;

	pEntity->GetBaseAnimating()->Dissolve( "", gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	
	// Note that we've struck an entity
	m_bStruckEntity = true;
	
	// Force an NPC to not drop their weapon if dissolved
//	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pEntity );
//	if ( pBCC != NULL )
//	{
//		pEntity->AddSpawnFlags( SF_NPC_NO_WEAPON_DROP );
//	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::OnHitEntity( CBaseEntity *pHitEntity, float flSpeed, int index, gamevcollisionevent_t *pEvent )
{
	// Detonate on the strider + the bone followers in the strider
	if ( FClassnameIs( pHitEntity, "npc_strider" ) || 
		(pHitEntity->GetOwnerEntity() && FClassnameIs( pHitEntity->GetOwnerEntity(), "npc_strider" )) )
	{
		DoExplosion();
		return;
	}

	CTakeDamageInfo info( this, GetOwnerEntity(), GetAbsVelocity(), GetAbsOrigin(), sk_npc_dmg_combineball.GetFloat(), DMG_DISSOLVE );

	bool bIsDissolving = (pHitEntity->GetFlags() & FL_DISSOLVING) != 0;
	bool bShouldHit = pHitEntity->PassesDamageFilter( info );

	//One more check
	//Combine soldiers are not allowed to hurt their friends with combine balls (they can still shoot and hurt each other with grenades).
	CBaseCombatCharacter *pBCC = pHitEntity->MyCombatCharacterPointer();

	if ( pBCC )
	{
		bShouldHit = pBCC->IRelationType( GetOwnerEntity() ) != D_LI;
	}

	if ( !bIsDissolving && bShouldHit == true )
	{
		if ( pHitEntity->PassesDamageFilter( info ) )
		{
			if( WasFiredByNPC() || m_nMaxBounces == -1 )
			{
				// Since Combine balls fired by NPCs do a metered dose of damage per impact, we have to ignore touches
				// for a little while after we hit someone, or the ball will immediately touch them again and do more
				// damage. 
				if( gpGlobals->curtime >= m_flNextDamageTime )
				{
					EmitSound( "NPC_CombineBall.KillImpact" );

					if ( pHitEntity->IsNPC() && pHitEntity->Classify() != CLASS_PLAYER_ALLY_VITAL && hl2_episodic.GetBool() == true )
					{
						if ( pHitEntity->Classify() != CLASS_PLAYER_ALLY || ( pHitEntity->Classify() == CLASS_PLAYER_ALLY && m_bStruckEntity == false ) )
						{
							info.SetDamage( pHitEntity->GetMaxHealth() );
							m_bStruckEntity = true;
						}
					}
					else
					{
						// Ignore touches briefly.
						m_flNextDamageTime = gpGlobals->curtime + 0.1f;
					}

					pHitEntity->TakeDamage( info );
				}
			}
			else
			{
				if ( (m_nState == STATE_THROWN) && (pHitEntity->IsNPC() || dynamic_cast<CRagdollProp*>(pHitEntity) ))
				{
					EmitSound( "NPC_CombineBall.KillImpact" );
				}
				if ( (m_nState != STATE_HOLDING) )
				{

					CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
					if ( pPlayer && UTIL_IsAR2CombineBall( this ) && ToBaseCombatCharacter( pHitEntity ) )
					{
						gamestats->Event_WeaponHit( pPlayer, false, "weapon_ar2", info );
					}

					DissolveEntity( pHitEntity );
					if ( pHitEntity->ClassMatches( "npc_hunter" ) )
					{
						DoExplosion();
						return;
					}
				}
			}
		}
	}

	Vector vecFinalVelocity;
	if ( IsInField() )
	{
		// Don't deflect when in a spawner field
		vecFinalVelocity = pEvent->preVelocity[index];
	}
	else
	{
		// Don't slow down when hitting other entities.
		vecFinalVelocity = pEvent->postVelocity[index];
		VectorNormalize( vecFinalVelocity );
		vecFinalVelocity *= GetSpeed();
	}
	PhysCallbackSetVelocity( pEvent->pObjects[index], vecFinalVelocity ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::DoImpactEffect( const Vector &preVelocity, int index, gamevcollisionevent_t *pEvent )
{
	// Do that crazy impact effect!
	trace_t tr;
	CollisionEventToTrace( !index, pEvent, tr );
	
	CBaseEntity *pTraceEntity = pEvent->pEntities[index];
	UTIL_TraceLine( tr.startpos - preVelocity * 2.0f, tr.startpos + preVelocity * 2.0f, MASK_SOLID, pTraceEntity, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		// See if we hit the sky
		if ( tr.surface.flags & SURF_SKY )
		{
			DoExplosion();
			return;
		}

		// Send the effect over
		CEffectData	data;

		data.m_flRadius = 16;
		data.m_vNormal	= tr.plane.normal;
		data.m_vOrigin	= tr.endpos + tr.plane.normal * 1.0f;

		DispatchEffect( "cball_bounce", data );
	}

	if ( hl2_episodic.GetBool() )
	{
		EmitSound( "NPC_CombineBall_Episodic.Impact" );
	}
	else
	{
		EmitSound( "NPC_CombineBall.Impact" );
	}
}

//-----------------------------------------------------------------------------
// Tells whether this combine ball should consider deflecting towards this entity.
//-----------------------------------------------------------------------------
bool CPropCombineBall::IsAttractiveTarget( CBaseEntity *pEntity )
{
	if ( !pEntity->IsAlive() )
		return false;

	if ( pEntity->GetFlags() & EF_NODRAW )
		return false;

	// Don't guide toward striders
	if ( FClassnameIs( pEntity, "npc_strider" ) )
		return false;

	if( WasFiredByNPC() )
	{
		// Fired by an NPC
		if( !pEntity->IsNPC() && !pEntity->IsPlayer() )
			return false;

		// Don't seek entities of the same class.
		if ( pEntity->m_iClassname == GetOwnerEntity()->m_iClassname )
			return false;
	}
	else
	{
		
#ifndef HL2MP
		if ( GetOwnerEntity() ) 
		{
			// Things we check if this ball has an owner that's not an NPC.
			if( GetOwnerEntity()->IsPlayer() ) 
			{
				if( pEntity->Classify() == CLASS_PLAYER				||
					pEntity->Classify() == CLASS_PLAYER_ALLY		||
					pEntity->Classify() == CLASS_PLAYER_ALLY_VITAL )
				{
					// Not attracted to other players or allies.
					return false;
				}
			}
		}

		// The default case.
		if ( !pEntity->IsNPC() )
			return false;

		if( pEntity->Classify() == CLASS_BULLSEYE )
			return false;

#else
		if ( pEntity->IsPlayer() == false )
			 return false;

		if ( pEntity == GetOwnerEntity() )
			 return false;
		
		//No tracking teammates in teammode!
		if ( g_pGameRules->IsTeamplay() )
		{
			if ( g_pGameRules->PlayerRelationship( GetOwnerEntity(), pEntity ) == GR_TEAMMATE )
				 return false;
		}
#endif

		// We must be able to hit them
		trace_t	tr;
		UTIL_TraceLine( WorldSpaceCenter(), pEntity->BodyTarget( WorldSpaceCenter() ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f && tr.m_pEnt != pEntity )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Deflects the ball toward enemies in case of a collision 
//-----------------------------------------------------------------------------
void CPropCombineBall::DeflectTowardEnemy( float flSpeed, int index, gamevcollisionevent_t *pEvent )
{
	// Bounce toward a particular enemy; choose one that's closest to my new velocity.
	Vector vecVelDir = pEvent->postVelocity[index];
	VectorNormalize( vecVelDir );

	CBaseEntity *pBestTarget = NULL;

	Vector vecStartPoint;
	pEvent->pInternalData->GetContactPoint( vecStartPoint );

	float flBestDist = MAX_COORD_FLOAT;

	CBaseEntity *list[1024];

	Vector	vecDelta;
	float	distance, flDot;

	// If we've already hit something, get accurate
	bool bSeekKill = m_bStruckEntity && (WasWeaponLaunched() || sk_combineball_seek_kill.GetInt() );

	if ( bSeekKill )
	{
		int nCount = UTIL_EntitiesInSphere( list, 1024, GetAbsOrigin(), sk_combine_ball_search_radius.GetFloat(), FL_NPC | FL_CLIENT );
		
		for ( int i = 0; i < nCount; i++ )
		{
			if ( !IsAttractiveTarget( list[i] ) )
				continue;

			VectorSubtract( list[i]->WorldSpaceCenter(), vecStartPoint, vecDelta );
			distance = VectorNormalize( vecDelta );

			if ( distance < flBestDist )
			{
				// Check our direction
				if ( DotProduct( vecDelta, vecVelDir ) > 0.0f )
				{
					pBestTarget = list[i];
					flBestDist = distance;
				}
			}
		}
	}
	else
	{
		float flMaxDot = 0.966f;
		if ( !WasWeaponLaunched() )
		{
			float flMaxDot = sk_combineball_seek_angle.GetFloat();
			float flGuideFactor = sk_combineball_guidefactor.GetFloat();
			for ( int i = m_nBounceCount; --i >= 0; )
			{
				flMaxDot *= flGuideFactor;
			}
			flMaxDot = cos( flMaxDot * M_PI / 180.0f );

			if ( flMaxDot > 1.0f )
			{
				flMaxDot = 1.0f;
			}
		}

		// Otherwise only help out a little
		Vector extents = Vector(256, 256, 256);
		Ray_t ray;
		ray.Init( vecStartPoint, vecStartPoint + 2048 * vecVelDir, -extents, extents );
		int nCount = UTIL_EntitiesAlongRay( list, 1024, ray, FL_NPC | FL_CLIENT );
		for ( int i = 0; i < nCount; i++ )
		{
			if ( !IsAttractiveTarget( list[i] ) )
				continue;

			VectorSubtract( list[i]->WorldSpaceCenter(), vecStartPoint, vecDelta );
			distance = VectorNormalize( vecDelta );
			flDot = DotProduct( vecDelta, vecVelDir );
			
			if ( flDot > flMaxDot )
			{
				if ( distance < flBestDist )
				{
					pBestTarget = list[i];
					flBestDist = distance;
				}
			}
		}
	}

	if ( pBestTarget )
	{
		Vector vecDelta;
		VectorSubtract( pBestTarget->WorldSpaceCenter(), vecStartPoint, vecDelta );
		VectorNormalize( vecDelta );
		vecDelta *= GetSpeed();
		PhysCallbackSetVelocity( pEvent->pObjects[index], vecDelta ); 
	}
}


//-----------------------------------------------------------------------------
// Bounce inside the spawner: 
//-----------------------------------------------------------------------------
void CPropCombineBall::BounceInSpawner( float flSpeed, int index, gamevcollisionevent_t *pEvent )
{
	GetSpawner()->RegisterReflection( this, m_bForward );

	m_bForward = !m_bForward;

	Vector vecTarget;
	GetSpawner()->GetTargetEndpoint( m_bForward, &vecTarget );

	Vector vecVelocity;
	VectorSubtract( vecTarget, GetAbsOrigin(), vecVelocity );
	VectorNormalize( vecVelocity );
	vecVelocity *= flSpeed;

	PhysCallbackSetVelocity( pEvent->pObjects[index], vecVelocity ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropCombineBall::IsHittableEntity( CBaseEntity *pHitEntity )
{
	if ( pHitEntity->IsWorld() )
		return false;

	if ( pHitEntity->GetMoveType() == MOVETYPE_PUSH )
	{
		if( pHitEntity->GetOwnerEntity() && FClassnameIs(pHitEntity->GetOwnerEntity(), "npc_strider") )
		{
			// The Strider's Bone Followers are MOVETYPE_PUSH, and we want the combine ball to hit these.
			return true;
		}

		// If the entity we hit can take damage, we're good
		if ( pHitEntity->m_takedamage == DAMAGE_YES )
			return true;

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	Vector preVelocity = pEvent->preVelocity[index];
	float flSpeed = VectorNormalize( preVelocity );

	if ( m_nMaxBounces == -1 )
	{
		const surfacedata_t *pHit = physprops->GetSurfaceData( pEvent->surfaceProps[!index] );

		if( pHit->game.material != CHAR_TEX_FLESH || !hl2_episodic.GetBool() )
		{
			CBaseEntity *pHitEntity = pEvent->pEntities[!index];
			if ( pHitEntity && IsHittableEntity( pHitEntity ) )
			{
				OnHitEntity( pHitEntity, flSpeed, index, pEvent );
			}

			// Remove self without affecting the object that was hit. (Unless it was flesh)
			NotifySpawnerOfRemoval();
			PhysCallbackRemove( this->NetworkProp() );

			// disable dissolve damage so we don't kill off the player when he's the one we hit
			PhysClearGameFlags( VPhysicsGetObject(), FVPHYSICS_DMG_DISSOLVE );
			return;
		}
	}

	// Prevents impact sounds, effects, etc. when it's in the field
	if ( !IsInField() )
	{
		BaseClass::VPhysicsCollision( index, pEvent );
	}

	if ( m_nState == STATE_HOLDING )
		return;

	// If we've collided going faster than our desired, then up our desired
	if ( flSpeed > GetSpeed() )
	{
		SetSpeed( flSpeed );
	}

	// Make sure we don't slow down
	Vector vecFinalVelocity = pEvent->postVelocity[index];
	VectorNormalize( vecFinalVelocity );
	vecFinalVelocity *= GetSpeed();
	PhysCallbackSetVelocity( pEvent->pObjects[index], vecFinalVelocity ); 

	CBaseEntity *pHitEntity = pEvent->pEntities[!index];
	if ( pHitEntity && IsHittableEntity( pHitEntity ) )
	{
		OnHitEntity( pHitEntity, flSpeed, index, pEvent );
		return;
	}

	if ( IsInField() )
	{
		if ( HasSpawnFlags( SF_COMBINE_BALL_BOUNCING_IN_SPAWNER ) && GetSpawner() )
		{
			BounceInSpawner( GetSpeed(), index, pEvent );
			return;
		}

		PhysCallbackSetVelocity( pEvent->pObjects[index], vec3_origin ); 

		// Delay the fade out so that we don't change our 
		// collision rules inside a vphysics callback.
		variant_t emptyVariant;
		g_EventQueue.AddEvent( this, "FadeAndRespawn", 0.01, NULL, NULL );
		return;
	}

	if ( IsBeingCaptured() )
		return;

	// Do that crazy impact effect!
	DoImpactEffect( preVelocity, index, pEvent );

	// Only do the bounce so often
	if ( gpGlobals->curtime - m_flLastBounceTime < 0.25f )
		return;

	// Save off our last bounce time
	m_flLastBounceTime = gpGlobals->curtime;

	// Reset the sound timer
	SetContextThink( &CPropCombineBall::WhizSoundThink, gpGlobals->curtime + 0.01, s_pWhizThinkContext );

	// Deflect towards nearby enemies
	DeflectTowardEnemy( flSpeed, index, pEvent );

	// Once more bounce
	++m_nBounceCount;

	if ( OutOfBounces() && m_bBounceDie == false )
	{
		StartLifetime( 0.5 );
		//Hack: Stop this from being called by doing this.
		m_bBounceDie = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCombineBall::AnimThink( void )
{
	StudioFrameAdvance();
	SetContextThink( &CPropCombineBall::AnimThink, gpGlobals->curtime + 0.1f, s_pAnimThinkContext );
}

//-----------------------------------------------------------------------------
//
// Implementation of CPropCombineBall
//
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( func_combine_ball_spawner, CFuncCombineBallSpawner );


//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncCombineBallSpawner )

	DEFINE_KEYFIELD( m_nBallCount, FIELD_INTEGER, "ballcount" ),
	DEFINE_KEYFIELD( m_flMinSpeed, FIELD_FLOAT, "minspeed" ),
	DEFINE_KEYFIELD( m_flMaxSpeed, FIELD_FLOAT, "maxspeed" ),
	DEFINE_KEYFIELD( m_flBallRadius, FIELD_FLOAT, "ballradius" ),
	DEFINE_KEYFIELD( m_flBallRespawnTime,	FIELD_FLOAT, "ballrespawntime" ),
	DEFINE_FIELD( m_flRadius,		FIELD_FLOAT ),
	DEFINE_FIELD( m_nBallsRemainingInField, FIELD_INTEGER ),
	DEFINE_FIELD( m_bEnabled,		FIELD_BOOLEAN ),
	DEFINE_UTLVECTOR( m_BallRespawnTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDisableTime,	FIELD_TIME ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_OnBallGrabbed, "OnBallGrabbed" ),
	DEFINE_OUTPUT( m_OnBallReinserted, "OnBallReinserted" ),
	DEFINE_OUTPUT( m_OnBallHitTopSide, "OnBallHitTopSide" ),
	DEFINE_OUTPUT( m_OnBallHitBottomSide, "OnBallHitBottomSide" ),
	DEFINE_OUTPUT( m_OnLastBallGrabbed, "OnLastBallGrabbed" ),
	DEFINE_OUTPUT( m_OnFirstBallReinserted, "OnFirstBallReinserted" ),

	DEFINE_THINKFUNC( BallThink ),
	DEFINE_ENTITYFUNC( GrabBallTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFuncCombineBallSpawner::CFuncCombineBallSpawner()
{
	m_flBallRespawnTime = 0.0f;
	m_flBallRadius = 20.0f;
	m_flDisableTime = 0.0f;
	m_bShooter = false;
}


//-----------------------------------------------------------------------------
// Spawn a ball
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::SpawnBall()
{
	CPropCombineBall *pBall = static_cast<CPropCombineBall*>( CreateEntityByName( "prop_combine_ball" ) );

	float flRadius = m_flBallRadius;
	pBall->SetRadius( flRadius );

	Vector vecAbsOrigin;
	ChoosePointInBox( &vecAbsOrigin );
	Vector zaxis;
	MatrixGetColumn( EntityToWorldTransform(), 2, zaxis );
	VectorMA( vecAbsOrigin, flRadius, zaxis, vecAbsOrigin );

	pBall->SetAbsOrigin( vecAbsOrigin );
	pBall->SetSpawner( this );

	float flSpeed = random->RandomFloat( m_flMinSpeed, m_flMaxSpeed );

	zaxis *= flSpeed;
	pBall->SetAbsVelocity( zaxis );
	if ( HasSpawnFlags( SF_SPAWNER_POWER_SUPPLY ) )
	{
		pBall->AddSpawnFlags( SF_COMBINE_BALL_BOUNCING_IN_SPAWNER );
	}

	pBall->Spawn();
}

void CFuncCombineBallSpawner::Precache()
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "prop_combine_ball" );
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::Spawn()
{
	BaseClass::Spawn();

	Precache();

	AddEffects( EF_NODRAW );
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_nBallsRemainingInField = m_nBallCount;

	float flWidth = CollisionProp()->OBBSize().x;
	float flHeight = CollisionProp()->OBBSize().y;
	m_flRadius = MIN( flWidth, flHeight ) * 0.5f;
	if ( m_flRadius <= 0.0f && m_bShooter == false )
	{
		Warning("Zero dimension func_combine_ball_spawner! Removing...\n");
		UTIL_Remove( this );
		return;
	}

	// Compute a respawn time
	float flDeltaT = 1.0f;
	if ( !( m_flMinSpeed == 0 && m_flMaxSpeed == 0 ) )
	{
		flDeltaT = (CollisionProp()->OBBSize().z - 2 * m_flBallRadius) / ((m_flMinSpeed + m_flMaxSpeed) * 0.5f);
		flDeltaT /= m_nBallCount;
	}

	m_BallRespawnTime.EnsureCapacity( m_nBallCount );
	for ( int i = 0; i < m_nBallCount; ++i )
	{
		RespawnBall( (float)i * flDeltaT );
	}

	m_bEnabled = true;
	if ( HasSpawnFlags( SF_SPAWNER_START_DISABLED ) )
	{
		inputdata_t inputData;
		InputDisable( inputData );
	}
	else
	{
		SetThink( &CFuncCombineBallSpawner::BallThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//-----------------------------------------------------------------------------
// Enable/disable
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::InputEnable( inputdata_t &inputdata )
{
	if ( m_bEnabled )
		return;

	m_bEnabled = true;
	m_flDisableTime = 0.0f;

	for ( int i = m_BallRespawnTime.Count(); --i >= 0; )
	{
		m_BallRespawnTime[i] += gpGlobals->curtime;
	}

	SetThink( &CFuncCombineBallSpawner::BallThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CFuncCombineBallSpawner::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bEnabled )
		return;

	m_flDisableTime = gpGlobals->curtime;
	m_bEnabled = false;

	for ( int i = m_BallRespawnTime.Count(); --i >= 0; )
	{
		m_BallRespawnTime[i] -= gpGlobals->curtime;
	}

	SetThink( NULL );
}

	
//-----------------------------------------------------------------------------
// Choose a random point inside the cylinder
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::ChoosePointInBox( Vector *pVecPoint )
{
	float flXBoundary = ( CollisionProp()->OBBSize().x != 0 ) ? m_flBallRadius / CollisionProp()->OBBSize().x : 0.0f;
	float flYBoundary = ( CollisionProp()->OBBSize().y != 0 ) ? m_flBallRadius / CollisionProp()->OBBSize().y : 0.0f;
	if ( flXBoundary > 0.5f )
	{
		flXBoundary = 0.5f;
	}
	if ( flYBoundary > 0.5f )
	{
		flYBoundary = 0.5f;
	}

	CollisionProp()->RandomPointInBounds( 
		Vector( flXBoundary, flYBoundary, 0.0f ), Vector( 1.0f - flXBoundary, 1.0f - flYBoundary, 0.0f ), pVecPoint );
}


//-----------------------------------------------------------------------------
// Choose a random point inside the cylinder
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::ChoosePointInCylinder( Vector *pVecPoint )
{
	float flXRange = m_flRadius / CollisionProp()->OBBSize().x;
	float flYRange = m_flRadius / CollisionProp()->OBBSize().y;

	Vector vecEndPoint1, vecEndPoint2;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecEndPoint1 ); 
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &vecEndPoint2 ); 

	// Choose a point inside the cylinder
	float flDistSq;
	do
	{
		CollisionProp()->RandomPointInBounds( 
			Vector( 0.5f - flXRange, 0.5f - flYRange, 0.0f ),
			Vector( 0.5f + flXRange, 0.5f + flYRange, 0.0f ),
			pVecPoint );

		flDistSq = CalcDistanceSqrToLine( *pVecPoint, vecEndPoint1, vecEndPoint2 );

	} while ( flDistSq > m_flRadius * m_flRadius );
}


//-----------------------------------------------------------------------------
// Register that a reflection occurred
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::RegisterReflection( CPropCombineBall *pBall, bool bForward )
{
	if ( bForward )
	{
		m_OnBallHitTopSide.FireOutput( pBall, this );
	}
	else
	{
		m_OnBallHitBottomSide.FireOutput( pBall, this );
	}
}


//-----------------------------------------------------------------------------
// Choose a random point on the 
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::GetTargetEndpoint( bool bForward, Vector *pVecEndPoint )
{
	float flZValue = bForward ? 1.0f : 0.0f;

	CollisionProp()->RandomPointInBounds( 
		Vector( 0.0f, 0.0f, flZValue ), Vector( 1.0f, 1.0f, flZValue ), pVecEndPoint );
}


//-----------------------------------------------------------------------------
// Fire ball grabbed output
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::BallGrabbed( CBaseEntity *pCombineBall )
{
	m_OnBallGrabbed.FireOutput( pCombineBall, this );
	--m_nBallsRemainingInField;
	if ( m_nBallsRemainingInField == 0 )
	{
		m_OnLastBallGrabbed.FireOutput( pCombineBall, this );
	}

	// Wait for another ball to touch this to re-power it up.
	if ( HasSpawnFlags( SF_SPAWNER_POWER_SUPPLY ) )
	{
		AddSolidFlags( FSOLID_TRIGGER );
		SetTouch( &CFuncCombineBallSpawner::GrabBallTouch );
	}

	// Stop the ball thinking in case it was in the middle of being captured (which could re-add incorrectly)
	if ( pCombineBall != NULL )
	{
		pCombineBall->SetContextThink( NULL, gpGlobals->curtime, s_pCaptureContext );
	}
}


//-----------------------------------------------------------------------------
// Fire ball grabbed output
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::GrabBallTouch( CBaseEntity *pOther )
{
	// Safety net for two balls hitting this at once
	if ( m_nBallsRemainingInField >= m_nBallCount )
		return;

	if ( pOther->GetCollisionGroup() != HL2COLLISION_GROUP_COMBINE_BALL )
		return;

	CPropCombineBall *pBall = dynamic_cast<CPropCombineBall*>( pOther );
	Assert( pBall );

	// Don't grab AR2 alt-fire
	if ( pBall->WasWeaponLaunched() || !pBall->VPhysicsGetObject() )
		return;

	// Don't grab balls that are already in the field..
	if ( pBall->IsInField() )
		return;

	// Don't grab fading out balls...
	if ( !pBall->IsSolid() )
		return;

	// Don't capture balls that were very recently in the field (breaks punting)
	if ( gpGlobals->curtime - pBall->LastCaptureTime() < 0.5f )
		return;

	// Now we're bouncing in this spawner
	pBall->AddSpawnFlags( SF_COMBINE_BALL_BOUNCING_IN_SPAWNER );

	// Tell the respawner we're no longer its ball
	pBall->NotifySpawnerOfRemoval();

	pBall->SetOwnerEntity( NULL );
	pBall->SetSpawner( this );
	pBall->CaptureBySpawner();

	++m_nBallsRemainingInField;

	if ( m_nBallsRemainingInField >= m_nBallCount )
	{
		RemoveSolidFlags( FSOLID_TRIGGER );
		SetTouch( NULL );
	}

	m_OnBallReinserted.FireOutput( pBall, this );
	if ( m_nBallsRemainingInField == 1 )
	{
		m_OnFirstBallReinserted.FireOutput( pBall, this );
	}
}


//-----------------------------------------------------------------------------
// Get a speed for the ball to insert
//-----------------------------------------------------------------------------
float CFuncCombineBallSpawner::GetBallSpeed( ) const
{
	return random->RandomFloat( m_flMinSpeed, m_flMaxSpeed );
}


//-----------------------------------------------------------------------------
// Balls call this when they've been removed from the spawner
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::RespawnBall( float flRespawnTime )
{
	// Insert the time in sorted order, 
	// which by definition means to always insert at the start
	m_BallRespawnTime.AddToTail( gpGlobals->curtime + flRespawnTime - m_flDisableTime );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::RespawnBallPostExplosion( void )
{
	if ( m_flBallRespawnTime < 0 )
		return;

	if ( m_flBallRespawnTime == 0.0f )
	{
		m_BallRespawnTime.AddToTail( gpGlobals->curtime + 4.0f - m_flDisableTime );
	}
	else
	{
		m_BallRespawnTime.AddToTail( gpGlobals->curtime + m_flBallRespawnTime - m_flDisableTime );
	}
}
	
//-----------------------------------------------------------------------------
// Ball think
//-----------------------------------------------------------------------------
void CFuncCombineBallSpawner::BallThink()
{
	for ( int i = m_BallRespawnTime.Count(); --i >= 0; )
	{
		if ( m_BallRespawnTime[i] < gpGlobals->curtime )
		{
			SpawnBall();
			m_BallRespawnTime.FastRemove( i );
		}
	}

	// There are no more to respawn
	SetNextThink( gpGlobals->curtime + 0.1f );
}

BEGIN_DATADESC( CPointCombineBallLauncher )
	DEFINE_KEYFIELD( m_flConeDegrees, FIELD_FLOAT, "launchconenoise" ),
	DEFINE_KEYFIELD( m_iszBullseyeName, FIELD_STRING, "bullseyename" ),
	DEFINE_KEYFIELD( m_iBounces, FIELD_INTEGER, "maxballbounces" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LaunchBall", InputLaunchBall ),
END_DATADESC()

#define SF_COMBINE_BALL_LAUNCHER_ATTACH_BULLSEYE	0x00000001
#define SF_COMBINE_BALL_LAUNCHER_COLLIDE_PLAYER		0x00000002

LINK_ENTITY_TO_CLASS( point_combine_ball_launcher, CPointCombineBallLauncher );

CPointCombineBallLauncher::CPointCombineBallLauncher()
{
	m_bShooter = true;
	m_flConeDegrees = 0.0f;
	m_iBounces = 0;
}

void CPointCombineBallLauncher::Spawn( void )
{
	m_bShooter = true;

	BaseClass::Spawn();
}

void CPointCombineBallLauncher::InputLaunchBall ( inputdata_t &inputdata )
{
	SpawnBall();
}

//-----------------------------------------------------------------------------
// Spawn a ball
//-----------------------------------------------------------------------------
void CPointCombineBallLauncher::SpawnBall()
{
	CPropCombineBall *pBall = static_cast<CPropCombineBall*>( CreateEntityByName( "prop_combine_ball" ) );

	if ( pBall == NULL )
		 return;

	float flRadius = m_flBallRadius;
	pBall->SetRadius( flRadius );

	Vector vecAbsOrigin = GetAbsOrigin();
	Vector zaxis;
	
	pBall->SetAbsOrigin( vecAbsOrigin );
	pBall->SetSpawner( this );

	float flSpeed = random->RandomFloat( m_flMinSpeed, m_flMaxSpeed );

	Vector vDirection;
	QAngle qAngle = GetAbsAngles();

	qAngle = qAngle + QAngle ( random->RandomFloat( -m_flConeDegrees, m_flConeDegrees ), random->RandomFloat( -m_flConeDegrees, m_flConeDegrees ), 0 );

	AngleVectors( qAngle, &vDirection, NULL, NULL );

	vDirection *= flSpeed;
	pBall->SetAbsVelocity( vDirection );

	DispatchSpawn(pBall);
	pBall->Activate();
	pBall->SetState( CPropCombineBall::STATE_LAUNCHED );
	pBall->SetMaxBounces( m_iBounces );

	if ( HasSpawnFlags( SF_COMBINE_BALL_LAUNCHER_COLLIDE_PLAYER ) )
	{
		pBall->SetCollisionGroup( HL2COLLISION_GROUP_COMBINE_BALL_NPC );
	}

	if( GetSpawnFlags() & SF_COMBINE_BALL_LAUNCHER_ATTACH_BULLSEYE )
	{
		CNPC_Bullseye *pBullseye = static_cast<CNPC_Bullseye*>( CreateEntityByName( "npc_bullseye" ) );

		if( pBullseye )
		{
			pBullseye->SetAbsOrigin( pBall->GetAbsOrigin() );
			pBullseye->SetAbsAngles( QAngle( 0, 0, 0 ) );
			pBullseye->KeyValue( "solid", "6" );
			pBullseye->KeyValue( "targetname", STRING(m_iszBullseyeName) );
			pBullseye->Spawn();

			DispatchSpawn(pBullseye);
			pBullseye->Activate();

			pBullseye->SetParent(pBall);
			pBullseye->SetHealth(10);
		}
	}
}

// ###################################################################
//	> FilterClass
// ###################################################################
class CFilterCombineBall : public CBaseFilter
{
	DECLARE_CLASS( CFilterCombineBall, CBaseFilter );
	DECLARE_DATADESC();

public:
	int m_iBallType;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CPropCombineBall *pBall = dynamic_cast<CPropCombineBall*>(pEntity );

		if ( pBall )
		{
			//Playtest HACK: If we have an NPC owner then we were shot from an AR2.
			if ( pBall->GetOwnerEntity() && pBall->GetOwnerEntity()->IsNPC() )
				return false;

			return pBall->GetState() == m_iBallType;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS( filter_combineball_type, CFilterCombineBall );

BEGIN_DATADESC( CFilterCombineBall )
	// Keyfields
	DEFINE_KEYFIELD( m_iBallType,	FIELD_INTEGER,	"balltype" ),
END_DATADESC()
