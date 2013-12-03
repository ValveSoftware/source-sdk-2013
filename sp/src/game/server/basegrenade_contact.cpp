//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basegrenade_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar    sk_plr_dmg_grenade;

// ==========================================================================================

class CBaseGrenadeContact : public CBaseGrenade
{
	DECLARE_CLASS( CBaseGrenadeContact, CBaseGrenade );
public:

	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( npc_contactgrenade, CBaseGrenadeContact );


void CBaseGrenadeContact::Spawn( void )
{
	// point sized, solid, bouncing
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetModel( "models/weapons/w_grenade.mdl" );	// BUG: wrong model

	UTIL_SetSize(this, vec3_origin, vec3_origin);

	// contact grenades arc lower
	SetGravity( UTIL_ScaleForGravity( 400 ) );	// use a lower gravity for grenades to make them easier to see

	QAngle angles;
	VectorAngles(GetAbsVelocity(), angles);
	SetLocalAngles( angles );
	
	// make NPCs afaid of it while in the air
	SetThink( &CBaseGrenadeContact::DangerSoundThink );
	SetNextThink( gpGlobals->curtime );
	
	// Tumble in air
	QAngle vecAngVelocity( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVelocity );

	// Explode on contact
	SetTouch( &CBaseGrenadeContact::ExplodeTouch );

	m_flDamage = sk_plr_dmg_grenade.GetFloat();

	// Allow player to blow this puppy up in the air
	m_takedamage	= DAMAGE_YES;

	m_iszBounceSound = NULL_STRING;
}


void CBaseGrenadeContact::Precache( void )
{
	BaseClass::Precache( );

	PrecacheModel("models/weapons/w_grenade.mdl");
}
