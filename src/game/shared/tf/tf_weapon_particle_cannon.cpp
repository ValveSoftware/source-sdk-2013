//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_particle_cannon.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "soundenvelope.h"
#include "particle_property.h"
#include "c_tf_gamestats.h"
// Server specific.
#else
#include "tf_gamestats.h"
#include "tf_player.h"
#include "tf_projectile_energy_ball.h"
#endif

//=============================================================================
//
// Particle cannon tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFParticleCannon, DT_ParticleCannon )

BEGIN_NETWORK_TABLE( CTFParticleCannon, DT_ParticleCannon )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
	RecvPropInt( RECVINFO( m_iChargeEffect ) )
#else
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
	SendPropInt( SENDINFO( m_iChargeEffect ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFParticleCannon )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_particle_cannon, CTFParticleCannon );
PRECACHE_WEAPON_REGISTER( tf_weapon_particle_cannon );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFParticleCannon )
END_DATADESC()
#endif

#ifdef GAME_DLL
const float tf_particle_cannon_afterburn_rate = 6.f;
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFParticleCannon::CTFParticleCannon() : CTFRocketLauncher()
{
#ifdef CLIENT_DLL
	m_bEffectsThinking = false;
	m_iChargeEffectBase = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFParticleCannon::GetProjectileSpeed( void )
{
	return 1100.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFParticleCannon::GetProjectileGravity( void )
{
	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFParticleCannon::IsViewModelFlipped( void )
{
	return !BaseClass::IsViewModelFlipped(); // Invert because arrows are backwards by default.
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFParticleCannon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !pPlayer->IsRegenerating() )
		return false;

	m_flChargeBeginTime = 0;

	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

#ifdef CLIENT_DLL
	ParticleProp()->Init( this );
	ParticleProp()->StopParticlesNamed( "drg_cowmangler_idle", true );
	m_bEffectsThinking = false;
#endif

	StopSound( "Weapon_CowMangler.Charging" );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFParticleCannon::Deploy( void )
{
	m_flChargeBeginTime = 0;

#ifdef CLIENT_DLL
	m_bEffectsThinking = true;
	SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "PC_EFFECTS_THINK" );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_flChargeBeginTime > 0 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		// If we're not holding down the attack button, launch our grenade
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
		if ( flTotalChargeTime >= GetChargeForceReleaseTime() )
		{
			FireChargedShot();
		}
	}

#ifdef CLIENT_DLL

	if ( !m_bEffectsThinking )
	{
		m_bEffectsThinking = true;
		SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "PC_EFFECTS_THINK" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::PrimaryAttack( void )
{
	if ( m_flChargeBeginTime > 0 )
		return;

	if ( !Energy_HasEnergy() )
		return;

	m_bChargedShot = false;
	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::SecondaryAttack( void )
{
	// Check for ammunition.
	if ( !Energy_FullyCharged() )
	{
		Reload();
		return;
	}

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( m_flChargeBeginTime > 0 )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}

	m_bChargedShot = true;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// save that we had the attack button down
	m_flChargeBeginTime = gpGlobals->curtime;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		SendWeaponAnim( ACT_PRIMARY_VM_PRIMARYATTACK_3 );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER );
	}

	WeaponSound( SPECIAL1 );

	pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

	m_iChargeEffect++;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::CreateChargeEffect()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		DispatchParticleEffect( "drg_cowmangler_muzzleflash_chargeup", PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::FireChargedShot()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifndef CLIENT_DLL
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, false );
#else
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, false );
#endif

//	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
//	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	CBaseEntity* pProj = FireProjectile( pPlayer );
	ModifyProjectile( pProj );

	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	m_iReloadMode.Set( TF_RELOAD_START );

	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ModifyProjectile( CBaseEntity* pProj )
{
#ifdef GAME_DLL
	CTFProjectile_EnergyBall* pEnergyBall = dynamic_cast<CTFProjectile_EnergyBall*>( pProj );
	if ( pEnergyBall == NULL )
		return;

	pEnergyBall->SetChargedShot( m_bChargedShot );
	pEnergyBall->SetColor( 1, GetParticleColor( 1 ) );
	pEnergyBall->SetColor( 2, GetParticleColor( 2 ) );
#endif

	if ( m_bChargedShot )
	{
		Energy_DrainEnergy( Energy_GetMaxEnergy() );
	}
	else
	{
		Energy_DrainEnergy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFParticleCannon::GetProgress( void )
{
	return Energy_GetEnergy() / Energy_GetMaxEnergy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFParticleCannon::GetMuzzleFlashParticleEffect( void )
{
	if ( m_bChargedShot )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_muzzleflash_charged" : "drg_cow_muzzleflash_charged_blue";
	}
	else
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_muzzleflash_normal" : "drg_cow_muzzleflash_normal_blue";
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_cow_explosioncore_charged" );
	PrecacheParticleSystem( "drg_cow_explosioncore_charged_blue" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal" );
	PrecacheParticleSystem( "drg_cow_explosioncore_normal_blue" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_charged" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_charged_blue" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_normal" );
	PrecacheParticleSystem( "drg_cow_muzzleflash_normal_blue" );
	PrecacheParticleSystem( "drg_cow_idle" );

	PrecacheScriptSound( "Weapon_CowMangler.ReloadFinal" );
}
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) )
	{
		if ( m_iChargeEffect != m_iChargeEffectBase )
		{
			CreateChargeEffect();
			m_iChargeEffectBase = m_iChargeEffect;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( !m_bEffectsThinking )
		return;

	SetContextThink( &CTFParticleCannon::ClientEffectsThink, gpGlobals->curtime + 2 + rand() % 5, "PC_EFFECTS_THINK" );

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	const char* mounts[4] =
	{
		"crit_frontspark1",
		"crit_frontspark2",
		"crit_frontspark3",
		"crit_frontspark4"
	};

	int iPoint = rand() % 4;

	ParticleProp()->Init( this );
	const char *pszIdleParticle = ( GetTeamNumber() == TF_TEAM_RED ) ? "drg_cow_idle" : "drg_cow_idle_blue";
	CNewParticleEffect* pEffect = ParticleProp()->Create( pszIdleParticle, PATTACH_POINT_FOLLOW, mounts[iPoint] );
	if ( pEffect )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetParticleColor( 1 ) );
		pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetParticleColor( 2 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetParticleColor( 1 ), GetParticleColor( 2 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	// Don't call direct parent. We don't want back blast effects.
	CTFWeaponBaseGun::CreateMuzzleFlashEffects( pAttachEnt, nIndex );
}

#endif


//-----------------------------------------------------------------------------
// Purpose: Utility function for default colors.
//-----------------------------------------------------------------------------
Vector GetParticleColorForTeam( int iTeam, int iColor )
{
	if ( iColor == 1 )
	{
		if ( iTeam == TF_TEAM_RED )
			return TF_PARTICLE_WEAPON_RED_1;
		else
			return TF_PARTICLE_WEAPON_BLUE_1;
	}
	else
	{
		if ( iTeam == TF_TEAM_RED )
			return TF_PARTICLE_WEAPON_RED_2;
		else
			return TF_PARTICLE_WEAPON_BLUE_2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::PlayWeaponShootSound( void )
{
	if ( m_bChargedShot )
	{
//		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFParticleCannon::GetShootSound( int iIndex ) const
{
	if ( iIndex == RELOAD )
	{
		bool bLastReload = (Energy_GetEnergy()+Energy_GetRechargeCost()) == Energy_GetMaxEnergy();
		if ( bLastReload )
		{
			return "Weapon_CowMangler.ReloadFinal";
		}
	}

	return BaseClass::GetShootSound(iIndex);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFParticleCannon::OwnerCanTaunt( void )
{
	if ( m_flChargeBeginTime > 0 )
	{
		return false;
	}
	else
	{
		return true;
	}
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFParticleCannon::GetAfterburnRateOnHit() const
{
	return tf_particle_cannon_afterburn_rate;
}
#endif // GAME_DLL
