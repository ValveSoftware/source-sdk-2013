//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Flaming bottle thrown from the hand
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "ammodef.h"
#include "gamerules.h"
#include "grenade_molotov.h"
#include "weapon_brickbat.h"
#include "soundent.h"
#include "decals.h"
#include "fire.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;

extern ConVar    sk_plr_dmg_molotov;
extern ConVar    sk_npc_dmg_molotov;
ConVar    sk_molotov_radius			( "sk_molotov_radius","0");

#define MOLOTOV_EXPLOSION_VOLUME	1024

BEGIN_DATADESC( CGrenade_Molotov )

	DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),

	// Function Pointers
	DEFINE_FUNCTION( MolotovTouch ),
	DEFINE_FUNCTION( MolotovThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_molotov, CGrenade_Molotov );

void CGrenade_Molotov::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX ); 
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

	SetModel( "models/weapons/w_molotov.mdl");

	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetTouch( MolotovTouch );
	SetThink( MolotovThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_molotov.GetFloat();
	m_DmgRadius		= sk_molotov_radius.GetFloat();

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( 1.0 );
	SetFriction( 0.8 );  // Give a little bounce so can flatten
	SetSequence( 1 );

	m_pFireTrail = SmokeTrail::CreateSmokeTrail();

	if( m_pFireTrail )
	{
		m_pFireTrail->m_SpawnRate			= 48;
		m_pFireTrail->m_ParticleLifetime	= 1.0f;
		
		m_pFireTrail->m_StartColor.Init( 0.2f, 0.2f, 0.2f );
		m_pFireTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pFireTrail->m_StartSize	= 8;
		m_pFireTrail->m_EndSize		= 32;
		m_pFireTrail->m_SpawnRadius	= 4;
		m_pFireTrail->m_MinSpeed	= 8;
		m_pFireTrail->m_MaxSpeed	= 16;
		m_pFireTrail->m_Opacity		= 0.25f;

		m_pFireTrail->SetLifetime( 20.0f );
		m_pFireTrail->FollowEntity( this, "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenade_Molotov::MolotovTouch( CBaseEntity *pOther )
{
	Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CGrenade_Molotov::Detonate( void ) 
{
	SetModelName( NULL_STRING );		//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );	// intangible

	m_takedamage = DAMAGE_NO;

	trace_t trace;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + Vector ( 0, 0, -128 ),  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &trace);

	// Pull out of the wall a bit
	if ( trace.fraction != 1.0 )
	{
		SetLocalOrigin( trace.endpos + (trace.plane.normal * (m_flDamage - 24) * 0.6) );
	}

	int contents = UTIL_PointContents ( GetAbsOrigin() );
	
	if ( (contents & MASK_WATER) )
	{
		UTIL_Remove( this );
		return;
	}

	EmitSound( "Grenade_Molotov.Detonate");

// Start some fires
	int i;
	QAngle vecTraceAngles;
	Vector vecTraceDir;
	trace_t firetrace;

	for( i = 0 ; i < 16 ; i++ )
	{
		// build a little ray
		vecTraceAngles[PITCH]	= random->RandomFloat(45, 135);
		vecTraceAngles[YAW]		= random->RandomFloat(0, 360);
		vecTraceAngles[ROLL]	= 0.0f;

		AngleVectors( vecTraceAngles, &vecTraceDir );

		Vector vecStart, vecEnd;

		vecStart = GetAbsOrigin() + ( trace.plane.normal * 128 );
		vecEnd = vecStart + vecTraceDir * 512;

		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &firetrace );

		Vector	ofsDir = ( firetrace.endpos - GetAbsOrigin() );
		float	offset = VectorNormalize( ofsDir );

		if ( offset > 128 )
			offset = 128;

		//Get our scale based on distance
		float scale	 = 0.1f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) );
		float growth = 0.1f + ( 0.75f * ( offset / 128.0f ) );

		if( firetrace.fraction != 1.0 )
		{
			FireSystem_StartFire( firetrace.endpos, scale, growth, 30.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS|SF_FIRE_NO_GLOW), (CBaseEntity*) this, FIRE_NATURAL );
		}
	}
// End Start some fires
	
	CPASFilter filter2( trace.endpos );

	te->Explosion( filter2, 0.0,
		&trace.endpos, 
		g_sModelIndexFireball,
		2.0, 
		15,
		TE_EXPLFLAG_NOPARTICLES,
		m_DmgRadius,
		m_flDamage );

	CBaseEntity *pOwner;
	pOwner = GetOwnerEntity();
	SetOwnerEntity( NULL ); // can't traceline attack owner if this is set

	UTIL_DecalTrace( &trace, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	RadiusDamage( CTakeDamageInfo( this, pOwner, m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime + 0.2 );

	if ( m_pFireTrail )
	{
		UTIL_Remove( m_pFireTrail );
	}

	UTIL_Remove(this);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Molotov::MolotovThink( void )
{
	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can throw the brickbat
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenade_Molotov::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel("models/weapons/w_bb_bottle.mdl");

	UTIL_PrecacheOther("_firesmoke");

	PrecacheScriptSound( "Grenade_Molotov.Detonate" );
}

