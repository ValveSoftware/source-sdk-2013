//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the brickbat weapon
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "grenade_pathfollower.h"
#include "soundent.h"
#include "decals.h"
#include "shake.h"
#include "smoke_trail.h"
#include "entitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_PF_TURN_RATE 30
#define GRENADE_PF_TOLERANCE 300
#define GRENADE_PF_MODEL	 "models/Weapons/w_missile.mdl"

extern short	g_sModelIndexFireball;			// (in combatweapon.cpp) holds the index for the smoke cloud

ConVar    sk_dmg_pathfollower_grenade		( "sk_dmg_pathfollower_grenade","0");
ConVar	  sk_pathfollower_grenade_radius	( "sk_pathfollower_grenade_radius","0");

BEGIN_DATADESC( CGrenadePathfollower )

	DEFINE_FIELD( m_pPathTarget,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flFlySpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( m_sFlySound,			FIELD_SOUNDNAME ),
	DEFINE_FIELD( m_flNextFlySoundTime,	FIELD_TIME),
	DEFINE_FIELD( m_hRocketTrail,			FIELD_EHANDLE),

	DEFINE_THINKFUNC( AimThink ),

	// Function pointers
	DEFINE_ENTITYFUNC( GrenadeTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_pathfollower, CGrenadePathfollower );

void CGrenadePathfollower::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "GrenadePathfollower.StopSounds" );
}

void CGrenadePathfollower::Spawn( void )
{
	Precache( );

	// -------------------------
	// Inert when first spawned
	// -------------------------
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	AddFlag( FL_OBJECT );	// So can be shot down
	AddEffects( EF_NODRAW );

	UTIL_SetSize(this, Vector(0, 0, 0), Vector(0, 0, 0));

	m_flDamage		= sk_dmg_pathfollower_grenade.GetFloat();
	m_DmgRadius		= sk_pathfollower_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 200;

	SetGravity( 0.00001 );
	SetFriction( 0.8 );
	SetSequence( 1 );
}

void CGrenadePathfollower::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

void CGrenadePathfollower::GrenadeTouch( CBaseEntity *pOther )
{
	// ----------------------------------
	// If I hit the sky, don't explode
	// ----------------------------------
	trace_t tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity(),  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &tr);

	if (tr.surface.flags & SURF_SKY)
	{
		if(m_hRocketTrail)
		{
			UTIL_Remove(m_hRocketTrail);
			m_hRocketTrail = NULL;
		}
		UTIL_Remove( this );
	}
	Detonate();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadePathfollower::Detonate(void)
{
	StopSound(entindex(), CHAN_BODY, STRING(m_sFlySound));

	m_takedamage	= DAMAGE_NO;	

	if(m_hRocketTrail)
	{
		UTIL_Remove(m_hRocketTrail);
		m_hRocketTrail = NULL;
	}

	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		0.5, 
		15,
		TE_EXPLFLAG_NONE,
		m_DmgRadius,
		m_flDamage );

	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward,  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

	UTIL_DecalTrace( &tr, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 0.2 );

	RadiusDamage ( CTakeDamageInfo( this, GetThrower(), m_flDamage, DMG_BLAST ), GetAbsOrigin(),  m_DmgRadius, CLASS_NONE, NULL );
	CPASAttenuationFilter filter2( this, "GrenadePathfollower.StopSounds" );
	EmitSound( filter2, entindex(), "GrenadePathfollower.StopSounds" );
	UTIL_Remove( this );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadePathfollower::Launch( float flLaunchSpeed, string_t sPathCornerName)
{
	m_pPathTarget = gEntList.FindEntityByName( NULL, sPathCornerName );
	if (m_pPathTarget)
	{
		m_flFlySpeed = flLaunchSpeed;
		Vector vTargetDir = (m_pPathTarget->GetAbsOrigin() - GetAbsOrigin());
		VectorNormalize(vTargetDir);
		SetAbsVelocity( m_flFlySpeed * vTargetDir );
		QAngle angles;
		VectorAngles( GetAbsVelocity(), angles );
		SetLocalAngles( angles );
	}
	else
	{
		Warning( "ERROR: Grenade_Pathfollower (%s) with no pathcorner!\n",GetDebugName());
		return;
	}

	// Make this thing come to life
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	RemoveEffects( EF_NODRAW );

	SetUse( &CGrenadePathfollower::DetonateUse );
	SetTouch( &CGrenadePathfollower::GrenadeTouch );
	SetThink( &CGrenadePathfollower::AimThink );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make the trail
	m_hRocketTrail = RocketTrail::CreateRocketTrail();

	if ( m_hRocketTrail )
	{
		m_hRocketTrail->m_Opacity = 0.2f;
		m_hRocketTrail->m_SpawnRate = 100;
		m_hRocketTrail->m_ParticleLifetime = 0.5f;
		m_hRocketTrail->m_StartColor.Init( 0.65f, 0.65f , 0.65f );
		m_hRocketTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		m_hRocketTrail->m_StartSize = 8;
		m_hRocketTrail->m_EndSize = 16;
		m_hRocketTrail->m_SpawnRadius = 4;
		m_hRocketTrail->m_MinSpeed = 2;
		m_hRocketTrail->m_MaxSpeed = 16;
		
		m_hRocketTrail->SetLifetime( 999 );
		m_hRocketTrail->FollowEntity( this, "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenadePathfollower::PlayFlySound(void)
{
	if (gpGlobals->curtime > m_flNextFlySoundTime)
	{
		CPASAttenuationFilter filter( this, 0.8 );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = STRING(m_sFlySound);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
		m_flNextFlySoundTime	= gpGlobals->curtime + 1.0;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadePathfollower::AimThink( void )
{
	PlayFlySound();

	// ---------------------------------------------------
	// Check if it's time to skip to the next path corner
	// ---------------------------------------------------
	if (m_pPathTarget)
	{
		float flLength = (GetAbsOrigin() - m_pPathTarget->GetAbsOrigin()).Length();
		if (flLength < GRENADE_PF_TOLERANCE)
		{
			m_pPathTarget = gEntList.FindEntityByName( NULL, m_pPathTarget->m_target );
			if (!m_pPathTarget)
			{	
				SetGravity( 1.0 );
			}
		}
	}

	// --------------------------------------------------
	//  If I have a pathcorner, aim towards it
	// --------------------------------------------------
	if (m_pPathTarget)
	{	
		Vector vTargetDir = (m_pPathTarget->GetAbsOrigin() - GetAbsOrigin());
		VectorNormalize(vTargetDir);

		Vector vecNewVelocity = GetAbsVelocity();
		VectorNormalize(vecNewVelocity);

		float flTimeToUse = gpGlobals->frametime;
		while (flTimeToUse > 0)
		{
			vecNewVelocity += vTargetDir;
			flTimeToUse = -0.1;
		}
		vecNewVelocity *= m_flFlySpeed;
		SetAbsVelocity( vecNewVelocity );
	}

	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T	CGrenadePathfollower::Classify( void)
{ 
	return CLASS_MISSILE; 
};

CGrenadePathfollower::CGrenadePathfollower(void)
{
	m_hRocketTrail  = NULL;
}

//------------------------------------------------------------------------------
// Purpose : In case somehow we get removed w/o detonating, make sure
//			 we stop making sounds
// Input   :
// Output  :
//------------------------------------------------------------------------------
CGrenadePathfollower::~CGrenadePathfollower(void)
{
	StopSound(entindex(), CHAN_BODY, STRING(m_sFlySound));
}

///------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CGrenadePathfollower* CGrenadePathfollower::CreateGrenadePathfollower( string_t sModelName, string_t sFlySound, const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner )
{
	CGrenadePathfollower *pGrenade = (CGrenadePathfollower*)CreateEntityByName( "grenade_pathfollower" );
	if ( !pGrenade )
	{
		Warning( "NULL Ent in CGrenadePathfollower!\n" );
		return NULL;
	}

	if ( pGrenade->edict() )
	{
		pGrenade->m_sFlySound	= sFlySound;
		pGrenade->SetOwnerEntity( Instance( pentOwner ) );
		pGrenade->SetLocalOrigin( vecOrigin );
		pGrenade->SetLocalAngles( vecAngles );
		pGrenade->SetModel( STRING(sModelName) );
		pGrenade->Spawn();
	}
	return pGrenade;
}
