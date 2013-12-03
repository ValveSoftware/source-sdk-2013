//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "grenade_spit.h"
#include "soundent.h"
#include "decals.h"
#include "smoke_trail.h"
#include "hl2_shareddefs.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "soundenvelope.h"
#include "ai_utils.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_antlion_worker_spit_grenade_dmg		  ( "sk_antlion_worker_spit_grenade_dmg", "20", FCVAR_NONE, "Total damage done by an individual antlion worker loogie.");
ConVar	  sk_antlion_worker_spit_grenade_radius		  ( "sk_antlion_worker_spit_grenade_radius","40", FCVAR_NONE, "Radius of effect for an antlion worker spit grenade.");
ConVar	  sk_antlion_worker_spit_grenade_poison_ratio ( "sk_antlion_worker_spit_grenade_poison_ratio","0.3", FCVAR_NONE, "Percentage of an antlion worker's spit damage done as poison (which regenerates)"); 

LINK_ENTITY_TO_CLASS( grenade_spit, CGrenadeSpit );

BEGIN_DATADESC( CGrenadeSpit )

	DEFINE_FIELD( m_bPlaySound, FIELD_BOOLEAN ),

	// Function pointers
	DEFINE_ENTITYFUNC( GrenadeSpitTouch ),

END_DATADESC()

CGrenadeSpit::CGrenadeSpit( void ) : m_bPlaySound( true ), m_pHissSound( NULL )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeSpit::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolidFlags( FSOLID_NOT_STANDABLE );

	SetModel( "models/spitball_large.mdl" );
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	SetUse( &CBaseGrenade::DetonateUse );
	SetTouch( &CGrenadeSpit::GrenadeSpitTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_antlion_worker_spit_grenade_dmg.GetFloat();
	m_DmgRadius		= sk_antlion_worker_spit_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_NO;
	m_iHealth		= 1;

	SetGravity( UTIL_ScaleForGravity( SPIT_GRAVITY ) );
	SetFriction( 0.8f );

	SetCollisionGroup( HL2COLLISION_GROUP_SPIT );

	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// We're self-illuminating, so we don't take or give shadows
	AddEffects( EF_NOSHADOW|EF_NORECEIVESHADOW );

	// Create the dust effect in place
	m_hSpitEffect = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
	if ( m_hSpitEffect != NULL )
	{
		// Setup our basic parameters
		m_hSpitEffect->KeyValue( "start_active", "1" );
		m_hSpitEffect->KeyValue( "effect_name", "antlion_spit_trail" );
		m_hSpitEffect->SetParent( this );
		m_hSpitEffect->SetLocalOrigin( vec3_origin );
		DispatchSpawn( m_hSpitEffect );
		if ( gpGlobals->curtime > 0.5f )
			m_hSpitEffect->Activate();
	}
}


void CGrenadeSpit::SetSpitSize( int nSize )
{
	switch (nSize)
	{
		case SPIT_LARGE:
		{
			m_bPlaySound = true;
			SetModel( "models/spitball_large.mdl" );
			break;
		}
		case SPIT_MEDIUM:
		{
			m_bPlaySound = true;
			m_flDamage *= 0.5f;
			SetModel( "models/spitball_medium.mdl" );
			break;
		}
		case SPIT_SMALL:
		{
			m_bPlaySound = false;
			m_flDamage *= 0.25f;
			SetModel( "models/spitball_small.mdl" );
			break;
		}
	}
}

void CGrenadeSpit::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

//-----------------------------------------------------------------------------
// Purpose: Handle spitting
//-----------------------------------------------------------------------------
void CGrenadeSpit::GrenadeSpitTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
			return;
	}

	// Don't hit other spit
	if ( pOther->GetCollisionGroup() == HL2COLLISION_GROUP_SPIT )
		return;

	// We want to collide with water
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

	// copy out some important things about this trace, because the first TakeDamage
	// call below may cause another trace that overwrites the one global pTrace points
	// at.
	bool bHitWater = ( ( pTrace->contents & CONTENTS_WATER ) != 0 );
	CBaseEntity *const pTraceEnt = pTrace->m_pEnt;
	const Vector tracePlaneNormal = pTrace->plane.normal;

	if ( bHitWater )
	{
		// Splash!
		CEffectData data;
		data.m_fFlags = 0;
		data.m_vOrigin = pTrace->endpos;
		data.m_vNormal = Vector( 0, 0, 1 );
		data.m_flScale = 8.0f;

		DispatchEffect( "watersplash", data );
	}
	else
	{
		// Make a splat decal
		trace_t *pNewTrace = const_cast<trace_t*>( pTrace );
		UTIL_DecalTrace( pNewTrace, "BeerSplash" );
	}

	// Part normal damage, part poison damage
	float poisonratio = sk_antlion_worker_spit_grenade_poison_ratio.GetFloat();

	// Take direct damage if hit
	// NOTE: assume that pTrace is invalidated from this line forward!
	if ( pTraceEnt )
	{
		pTraceEnt->TakeDamage( CTakeDamageInfo( this, GetThrower(), m_flDamage * (1.0f-poisonratio), DMG_ACID ) );
		pTraceEnt->TakeDamage( CTakeDamageInfo( this, GetThrower(), m_flDamage * poisonratio, DMG_POISON ) );
	}

	CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), m_DmgRadius * 2.0f, 0.5f, GetThrower() );

	QAngle vecAngles;
	VectorAngles( tracePlaneNormal, vecAngles );
	
	if ( pOther->IsPlayer() || bHitWater )
	{
		// Do a lighter-weight effect if we just hit a player
		DispatchParticleEffect( "antlion_spit_player", GetAbsOrigin(), vecAngles );
	}
	else
	{
		DispatchParticleEffect( "antlion_spit", GetAbsOrigin(), vecAngles );
	}

	Detonate();
}

void CGrenadeSpit::Detonate(void)
{
	m_takedamage = DAMAGE_NO;

	EmitSound( "GrenadeSpit.Hit" );	

	// Stop our hissing sound
	if ( m_pHissSound != NULL )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHissSound );
		m_pHissSound = NULL;
	}

	if ( m_hSpitEffect )
	{
		UTIL_Remove( m_hSpitEffect );
	}

	UTIL_Remove( this );
}

void CGrenadeSpit::InitHissSound( void )
{
	if ( m_bPlaySound == false )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pHissSound == NULL )
	{
		CPASAttenuationFilter filter( this );
		m_pHissSound = controller.SoundCreate( filter, entindex(), "NPC_Antlion.PoisonBall" );
		controller.Play( m_pHissSound, 1.0f, 100 );
	}
}

void CGrenadeSpit::Think( void )
{
	InitHissSound();
	if ( m_pHissSound == NULL )
		return;
	
	// Add a doppler effect to the balls as they travel
	CBaseEntity *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer != NULL )
	{
		Vector dir;
		VectorSubtract( pPlayer->GetAbsOrigin(), GetAbsOrigin(), dir );
		VectorNormalize(dir);

		float velReceiver = DotProduct( pPlayer->GetAbsVelocity(), dir );
		float velTransmitter = -DotProduct( GetAbsVelocity(), dir );
		
		// speed of sound == 13049in/s
		int iPitch = 100 * ((1 - velReceiver / 13049) / (1 + velTransmitter / 13049));

		// clamp pitch shifts
		if ( iPitch > 250 )
		{
			iPitch = 250;
		}
		if ( iPitch < 50 )
		{
			iPitch = 50;
		}

		// Set the pitch we've calculated
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pHissSound, iPitch, 0.1f );
	}

	// Set us up to think again shortly
	SetNextThink( gpGlobals->curtime + 0.05f );
}

void CGrenadeSpit::Precache( void )
{
	// m_nSquidSpitSprite = PrecacheModel("sprites/greenglow1.vmt");// client side spittle.

	PrecacheModel( "models/spitball_large.mdl" ); 
	PrecacheModel("models/spitball_medium.mdl"); 
	PrecacheModel("models/spitball_small.mdl"); 

	PrecacheScriptSound( "GrenadeSpit.Hit" );

	PrecacheParticleSystem( "antlion_spit_player" );
	PrecacheParticleSystem( "antlion_spit" );
}
