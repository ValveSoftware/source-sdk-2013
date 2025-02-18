//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Pumpkin Bomb
//
//=============================================================================
#include "cbase.h"

#include "particle_parse.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_target_dummy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "tf_ammo_pack.h"

#define DUMMY_MODEL "models/props_training/target_engineer.mdl"
#define DUMMY_DEMO_MODEL "models/props_training/target_demoman.mdl"
//#define TEAM_PUMPKIN "models/props_halloween/pumpkin_explode_teamcolor.mdl"

LINK_ENTITY_TO_CLASS( tf_target_dummy, CTFTargetDummy);

IMPLEMENT_AUTO_LIST( ITFTargetDummy );

ConVar tf_target_dummy_health( "tf_target_dummy_health", "200", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY ); // DEV ONLY
ConVar tf_target_dummy_melee_mult( "tf_target_dummy_melee_mult", "3.0f", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY ); // DEV ONLY
ConVar tf_target_dummy_bullet_mult( "tf_target_dummy_bullet_mult", "0.1f", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY ); // DEV ONLY
ConVar tf_target_dummy_other_mult( "tf_target_dummy_other_mult", "0.2f", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY ); // DEV ONLY
ConVar tf_target_dummy_lifetime( "tf_target_dummy_lifetime", "30.0f", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY ); // DEV ONLY

#ifdef GAME_DLL
extern ConVar tf_obj_gib_velocity_min;
extern ConVar tf_obj_gib_velocity_max;
extern ConVar tf_obj_gib_maxspeed;
#endif


//-----------------------------------------------------------------------------
// Static
CTFTargetDummy* CTFTargetDummy::Create( const Vector& vPosition, const QAngle& qAngles, CTFPlayer *pOwner )
{
	QAngle qProper = qAngles;
	qProper.z = 0;
	qProper.x = 0;
	CTFTargetDummy *pDummy = assert_cast<CTFTargetDummy*>( CBaseEntity::Create( "tf_target_dummy", vPosition, qProper, pOwner ) );

	if ( pDummy && pOwner )
	{
		pDummy->ChangeTeam( pOwner->GetTeamNumber() );
		pDummy->m_nSkin = pOwner->GetTeamNumber() == TF_TEAM_BLUE ? 1 : 2;
	}

	return pDummy;
}

//-----------------------------------------------------------------------------
CTFTargetDummy::CTFTargetDummy()
{
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTargetDummy::Precache( void )
{
	PrecacheModel( DUMMY_MODEL );
	int iModel = PrecacheModel( DUMMY_DEMO_MODEL );
	PrecacheGibsForModel( iModel );

	PrecacheParticleSystem( "target_break" );
	PrecacheParticleSystem( "lowV_debrischunks" );
	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
void CTFTargetDummy::Spawn()
{
	Precache();

	m_aGibs.Purge();
	// TODO: Model Selection
	if ( RandomInt( 0, 1 ) == 1 )
	{
		SetModel( DUMMY_DEMO_MODEL );
		BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );
	}
	else
	{
		SetModel( DUMMY_MODEL );
	}
	// Set Team
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	SetCollisionGroup( TFCOLLISION_GROUP_OBJECT );

	//SetSolidFlags( FSOLID_TRIGGER );

	m_takedamage = DAMAGE_YES;
	SetHealth( tf_target_dummy_health.GetInt() );

	//DispatchParticleEffect( "merasmus_object_spawn", WorldSpaceCenter(), GetAbsAngles() );

	SetContextThink( &CTFTargetDummy::DestroyThink, gpGlobals->curtime + tf_target_dummy_lifetime.GetFloat(), "DestroyThink" );
}

//-----------------------------------------------------------------------------
void CTFTargetDummy::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "Halloween.Merasmus_Hiding_Explode" );

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
int CTFTargetDummy::OnTakeDamage( const CTakeDamageInfo &info )
{
	DispatchParticleEffect( "lowV_debrischunks", info.GetDamagePosition(), GetAbsAngles() );

	CTakeDamageInfo newinfo = info;

	if ( InSameTeam( info.GetAttacker() ))
	{
		return 0;
	}

	// melee and fire is normal
	// bullet is heavily reduced
	// everything else (blast/ energy etc..) reduced

	//CTFPlayer *pTFPlayer = ToTFPlayer( newinfo.GetAttacker() );
	if ( ( newinfo.GetDamageType() & DMG_BUCKSHOT ) || ( newinfo.GetDamageType() & DMG_BULLET ) )
	{
		newinfo.SetDamage( newinfo.GetDamage() * tf_target_dummy_bullet_mult.GetFloat() );
	}
	else if ( ( newinfo.GetDamageType() & DMG_CLUB ) || ( newinfo.GetDamageType() & DMG_BURN ) )
	{
		newinfo.SetDamage( newinfo.GetDamage() * tf_target_dummy_melee_mult.GetFloat() );
	}
	else
	{
		newinfo.SetDamage( newinfo.GetDamage() * tf_target_dummy_other_mult.GetFloat() );
	}

	int iDamage = BaseClass::OnTakeDamage( newinfo );

	if ( m_iHealth <= 0 )
	{
		Destroy();
	}
	return iDamage;
}

//-----------------------------------------------------------------------------
void CTFTargetDummy::DestroyThink()
{
	// Blow us up.
	//CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), 0, DMG_GENERIC );
	//Killed( info );
	Destroy();
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
void CTFTargetDummy::Destroy()
{
	SpewGibs();
	DispatchParticleEffect( "target_break", WorldSpaceCenter(), GetAbsAngles() );
}

//-----------------------------------------------------------------------------
void CTFTargetDummy::SpewGibs()
{
	for ( int i = 0; i < m_aGibs.Count(); i++ )
	{
		CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin() + m_aGibs[i].offset, GetAbsAngles(), this, m_aGibs[i].modelName );
		Assert( pAmmoPack );
		if ( pAmmoPack )
		{
			pAmmoPack->ActivateWhenAtRest();

			// Calculate the initial impulse on the weapon.
			Vector vecImpulse( random->RandomFloat( -0.5f, 0.5f ), random->RandomFloat( -0.5f, 0.5f ), random->RandomFloat( 0.75f, 1.25f ) );
			VectorNormalize( vecImpulse );
			vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_min.GetFloat(), tf_obj_gib_velocity_max.GetFloat() );

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			if ( pAmmoPack->VPhysicsGetObject() )
			{
				AngularImpulse angImpulse( 0.f, random->RandomFloat( 0.f, 100.f ), 0.f );
				pAmmoPack->VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
			}

			pAmmoPack->SetInitialVelocity( vecImpulse );

			pAmmoPack->m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;

			// Give the ammo pack some health, so that trains can destroy it.
			pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pAmmoPack->m_takedamage = DAMAGE_YES;
			pAmmoPack->SetHealth( 900 );
			pAmmoPack->m_bObjGib = true;

			pAmmoPack->GiveAmmo( 0, TF_AMMO_METAL );
		}
	}
}

//-----------------------------------------------------------------------------
bool CTFTargetDummy::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		if ( GetCollisionGroup() == TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT )
		{
			return true;
		}

		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}