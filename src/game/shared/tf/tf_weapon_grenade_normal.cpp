//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Normal Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_normal.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#endif

#define GRENADE_TIMER	3.0f //Seconds

//=============================================================================
//
// TF Normal Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeNormal, DT_TFGrenadeNormal )

BEGIN_NETWORK_TABLE( CTFGrenadeNormal, DT_TFGrenadeNormal )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeNormal )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_normal, CTFGrenadeNormal );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_normal );

//IMPLEMENT_ACTTABLE( CTFGrenadeNormal );

//=============================================================================
//
// TF Normal Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeNormal )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeNormal::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		pTFPlayer->RemoveDisguise();
	}

	return CTFGrenadeNormalProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

//=============================================================================
//
// TF Normal Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/Weapons/w_models/w_grenade_frag.mdl"
//#define GRENADE_MODEL "models/weapons/w_grenade_normal.mdl"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_normal_projectile, CTFGrenadeNormalProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_normal_projectile );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeNormalProjectile* CTFGrenadeNormalProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeNormalProjectile *pGrenade = static_cast<CTFGrenadeNormalProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_normal_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNormalProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNormalProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNormalProjectile::BounceSound( void )
{
	EmitSound( "BaseGrenade.BounceSound" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNormalProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}

#endif
