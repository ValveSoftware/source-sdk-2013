//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Caltrop Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_caltrop.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#endif

#define GRENADE_CALTROP_TIMER			3.0f //Seconds
#define GRENADE_CALTROP_RELEASE_COUNT	6
#define GRENADE_CALTROP_DAMAGE			10

//=============================================================================
//
// TF Caltrop Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeCaltrop, DT_TFGrenadeCaltrop )

BEGIN_NETWORK_TABLE( CTFGrenadeCaltrop, DT_TFGrenadeCaltrop )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeCaltrop )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_caltrop, CTFGrenadeCaltrop );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_caltrop );

//=============================================================================
//
// TF Caltrop Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeCaltrop )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeCaltrop::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	// Release several at a time (different directions, angles, speeds, etc.)
	for ( int i = 0 ; i < GRENADE_CALTROP_RELEASE_COUNT ; i++ )
	{
		Vector velocity( random->RandomFloat(-100,100), random->RandomFloat(-100,100), random->RandomFloat(150,200) );
		CTFGrenadeCaltropProjectile::Create( vecSrc, vecAngles, velocity, angImpulse, 
			                                 pPlayer, GetTFWpnData(), random->RandomFloat( 10.0f, 15.0f ) );
	}

	return NULL;
}

#endif

//=============================================================================
//
// TF Caltrop Grenade Projectile functions (Server specific).
//
LINK_ENTITY_TO_CLASS( tf_weapon_grenade_caltrop_projectile, CTFGrenadeCaltropProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_caltrop_projectile );

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeCaltropProjectile, DT_TFGrenadeCaltropProjectile )

BEGIN_NETWORK_TABLE( CTFGrenadeCaltropProjectile, DT_TFGrenadeCaltropProjectile )
END_NETWORK_TABLE()

#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_beartrap.mdl"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeCaltropProjectile* CTFGrenadeCaltropProjectile::Create( const Vector &position, const QAngle &angles, 
																  const Vector &velocity, const AngularImpulse &angVelocity, 
																  CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo,
																  float timer, int iFlags )
{
	CTFGrenadeCaltropProjectile *pGrenade = static_cast<CTFGrenadeCaltropProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_caltrop_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );

		pGrenade->SetTouch( &CTFGrenadeCaltropProjectile::Touch );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Caltrop.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// have the caltrop disappear
	UTIL_Remove( this );

#if 0
	// Tell the bots an HE grenade has exploded
	CTFPlayer *pPlayer = ToTFPlayer( GetThrower() );
	if ( pPlayer )
	{
		KeyValues *pEvent = new KeyValues( "tf_weapon_grenade_detonate" );
		pEvent->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEventServerOnly( pEvent );
	}
#endif
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() || !( pOther->GetFlags() & FL_ONGROUND ) || !pOther->IsAlive() )
		return;

	// Don't hurt friendlies
	if ( GetTeamNumber() == pOther->GetTeamNumber() )
		return;

	// Caltrops need to be on the ground. Check to see if we're still moving.
	Vector vecVelocity;
	VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );
	if ( vecVelocity.LengthSqr() > (1*1) )
		return;

#ifdef GAME_DLL
	// Do the leg damage to the player
	CTakeDamageInfo info( this, GetThrower(), GRENADE_CALTROP_DAMAGE, DMG_LEG_DAMAGE | DMG_PREVENT_PHYSICS_FORCE );
	pOther->TakeDamage( info );

	// have the caltrop disappear
	UTIL_Remove( this );
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		/*
		SetSolidFlags( FSOLID_NOT_STANDABLE );
		SetSolid( SOLID_BBOX );	

		SetCollisionBounds( Vector( -2.0f, -2.0f, -2.0f ), Vector( 2.0f, 2.0f, 2.0f ) );

		// We want touch calls on the client.
		// So override the collision group, but make it a trigger
		SetCollisionGroup( COLLISION_GROUP_NONE );
		AddSolidFlags( FSOLID_TRIGGER );

		UpdatePartitionListEntry();

		CollisionProp()->UpdatePartition();
		*/
	}
}
#endif
