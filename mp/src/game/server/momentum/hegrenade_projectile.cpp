//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#ifndef CLIENT_DLL
#include "hegrenade_projectile.h"
#include "soundent.h"
#include "mom_player.h"
#include "KeyValues.h"

#define GRENADE_MODEL "models/Weapons/w_eq_fraggrenade_thrown.mdl"


LINK_ENTITY_TO_CLASS( hegrenade_projectile, CHEGrenadeProjectile );
PRECACHE_WEAPON_REGISTER( hegrenade_projectile );

CHEGrenadeProjectile* CHEGrenadeProjectile::Create( 
    const Vector &position, 
    const QAngle &angles, 
    const Vector &velocity, 
    const AngularImpulse &angVelocity, 
    CBaseCombatCharacter *pOwner, 
    float timer )
{
    CHEGrenadeProjectile *pGrenade = (CHEGrenadeProjectile*)CBaseEntity::Create( "hegrenade_projectile", position, angles, pOwner );

    // Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
    // one second before detonation.

    pGrenade->SetDetonateTimerLength( 1.5 );
    pGrenade->SetAbsVelocity( velocity );
    pGrenade->SetupInitialTransmittedGrenadeVelocity( velocity );
    pGrenade->SetThrower( pOwner ); 

    pGrenade->SetGravity( BaseClass::GetGrenadeGravity() );
    pGrenade->SetFriction( BaseClass::GetGrenadeFriction() );
    pGrenade->SetElasticity( BaseClass::GetGrenadeElasticity() );

    pGrenade->m_flDamage = 100;
    pGrenade->m_DmgRadius = pGrenade->m_flDamage * 3.5f;
    pGrenade->ChangeTeam( pOwner->GetTeamNumber() );
    pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	

    // make NPCs afaid of it while in the air
    pGrenade->SetThink( &CHEGrenadeProjectile::DangerSoundThink );
    pGrenade->SetNextThink( gpGlobals->curtime );

    return pGrenade;
}

void CHEGrenadeProjectile::Spawn()
{
    SetModel( GRENADE_MODEL );
    BaseClass::Spawn();
}

void CHEGrenadeProjectile::Precache()
{
    PrecacheModel( GRENADE_MODEL );

    PrecacheScriptSound( "HEGrenade.Bounce" );

    BaseClass::Precache();
}

void CHEGrenadeProjectile::BounceSound( void )
{
    EmitSound( "HEGrenade.Bounce" );
}

void CHEGrenadeProjectile::Detonate()
{
    BaseClass::Detonate();

    // tell the bots an HE grenade has exploded
    CMomentumPlayer *player = static_cast<CMomentumPlayer*>(GetThrower());
    if ( player )
    {
        IGameEvent * event = gameeventmanager->CreateEvent( "hegrenade_detonate" );
        if ( event )
        {
            event->SetInt( "userid", player->GetUserID() );
            event->SetFloat( "x", GetAbsOrigin().x );
            event->SetFloat( "y", GetAbsOrigin().y );
            event->SetFloat( "z", GetAbsOrigin().z );
            gameeventmanager->FireEvent( event );
        }
    }
}
#endif // CLIENT_DLL
