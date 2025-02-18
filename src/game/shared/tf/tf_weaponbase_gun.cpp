//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_fx_shared.h"
#include "effect_dispatch_data.h"
#include "takedamageinfo.h"
#include "tf_projectile_nail.h"
#include "tf_weapon_jar.h"
#include "tf_weapon_flaregun.h"
#include "tf_projectile_energy_ring.h"

#if !defined( CLIENT_DLL )	// Server specific.

	#include "tf_gamestats.h"
	#include "tf_player.h"
	#include "tf_fx.h"
	#include "te_effect_dispatch.h"

	#include "tf_projectile_flare.h"
	#include "tf_projectile_rocket.h"
	#include "tf_projectile_arrow.h"
	#include "tf_projectile_energy_ball.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "te.h"

#else	// Client specific.

	#include "c_tf_player.h"
	#include "c_te_effect_dispatch.h"
	#include "c_tf_gamestats.h"

#endif

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGun, DT_TFWeaponBaseGun )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGun, DT_TFWeaponBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGun )
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseGun )
DEFINE_THINKFUNC( ZoomOutIn ),
DEFINE_THINKFUNC( ZoomOut ),
DEFINE_THINKFUNC( ZoomIn ),
END_DATADESC()
#endif

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGun::CTFWeaponBaseGun()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iAmmoToAdd = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PrimaryAttack( void )
{
	float flUberChargeAmmoPerShot = UberChargeAmmoPerShot();
	if ( flUberChargeAmmoPerShot > 0.0f )
	{
		if ( !HasPrimaryAmmo() )
			return;
	}

	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flFireDelay, hwn_mult_postfiredelay );

	// Some weapons change fire delay based on player's health
	float flReducedHealthBonus = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flReducedHealthBonus, mult_postfiredelay_with_reduced_health );
	if ( flReducedHealthBonus != 1.0f )
	{
		flReducedHealthBonus = RemapValClamped( pPlayer->HealthFraction(), 0.2f, 0.9f, flReducedHealthBonus, 1.0f );
		flFireDelay *= flReducedHealthBonus;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
	{
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, rocketjump_attackrate_bonus );	
	}
	else
	{
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mul_nonrocketjump_attackrate );
	}

	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( GetOwner() && GetAmmoPerShot() > GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) )
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
			return;
		}
	}

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
	
	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN )
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#else
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	CBaseEntity* pProj = FireProjectile( pPlayer );
	ModifyProjectile( pProj );

	if ( !UsesClipsForAmmo1() )
	{
		// Sniper rifles and such don't actually reload, so we hook reduced reload here
		float flBaseFireDelay = flFireDelay;
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, fast_reload );

		float flPlaybackRate = flFireDelay == 0.f ? 0.f : flBaseFireDelay / flFireDelay;

		if ( pPlayer->GetViewModel( 0 ) )
		{
			pPlayer->GetViewModel( 0 )->SetPlaybackRate( flPlaybackRate );
		}
		if ( pPlayer->GetViewModel( 1 ) )
		{
			pPlayer->GetViewModel( 1 )->SetPlaybackRate( flPlaybackRate );
		}
	}

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	if ( Clip1() > 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}


	m_flLastPrimaryAttackTime = gpGlobals->curtime;

	if ( ShouldRemoveDisguiseOnPrimaryAttack() )
	{
		pPlayer->RemoveDisguise();
	}

	pPlayer->m_Shared.OnAttack();
}	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::ShouldRemoveDisguiseOnPrimaryAttack() const
{
	int iAttr = 0;
	CALL_ATTRIB_HOOK_INT( iAttr, keep_disguise_on_attack );
	if ( iAttr )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;


	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer )
{
	// New behavior: allow weapons to have attributes to specify what sort of
	// projectile they fire.
	int iProjectile = 0;
	CALL_ATTRIB_HOOK_INT( iProjectile, override_projectile_type );

	// Previous default behavior: ask the weapon type for what sort of projectile
	// to launch.
	if ( iProjectile == 0 )
	{
		iProjectile = GetWeaponProjectileType();
	}

	CBaseEntity *pProjectile = NULL;

	// Anyone ever hear of a factory? This is a disgrace.
	switch( iProjectile )
	{
	case TF_PROJECTILE_BULLET:
		FireBullet( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_ROCKET:
		pProjectile = FireRocket( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_SYRINGE:
		pProjectile = FireNail( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_FLARE:
		pProjectile = FireFlare( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB:
	case TF_PROJECTILE_PIPEBOMB_REMOTE:
	case TF_PROJECTILE_PIPEBOMB_PRACTICE:
	case TF_PROJECTILE_CANNONBALL:
		pProjectile = FirePipeBomb( pPlayer, iProjectile );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_JAR:
	case TF_PROJECTILE_JAR_MILK:
	case TF_PROJECTILE_CLEAVER:
	case TF_PROJECTILE_THROWABLE:
	case TF_PROJECTILE_FESTIVE_JAR:
	case TF_PROJECTILE_BREADMONSTER_JARATE:
	case TF_PROJECTILE_BREADMONSTER_MADMILK:
	case TF_PROJECTILE_JAR_GAS:
		pProjectile = FireJar( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;
	case TF_PROJECTILE_ARROW:
	case TF_PROJECTILE_HEALING_BOLT:
	case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
	case TF_PROJECTILE_FESTIVE_ARROW:
	case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
	case TF_PROJECTILE_GRAPPLINGHOOK:
		pProjectile = FireArrow( pPlayer, ProjectileType_t( iProjectile ) );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_FLAME_ROCKET:
		pProjectile = FireFlameRocket( pPlayer );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
		break;

	case TF_PROJECTILE_ENERGY_BALL:
		pProjectile = FireEnergyBall( pPlayer );
		if ( ShouldPlayFireAnim() )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		}
		break;

	case TF_PROJECTILE_ENERGY_RING:
		pProjectile = FireEnergyBall( pPlayer, true );
		if ( ShouldPlayFireAnim() )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		}
		break;

	case TF_PROJECTILE_NONE:
	default:
		// do nothing!
		DevMsg( "Weapon does not have a projectile type set\n" );
		break;
	}

	RemoveProjectileAmmo( pPlayer );

	m_flLastFireTime = gpGlobals->curtime;
	m_iConsecutiveShots++;

	DoFireEffects();

	UpdatePunchAngles( pPlayer );

#ifdef GAME_DLL
	// Some game modes may allow any class to have stealth.  Continuous-fire weapons like the
	// minigun might be firing when stealth is applied, so we try removing it from here, too.
	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
#endif // GAME_DLL

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::RemoveProjectileAmmo( CTFPlayer *pPlayer )
{

	if ( m_iClip1 != -1 )
	{
		m_iClip1 -= GetAmmoPerShot();
	}
	else
	{
		if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
		{
			pPlayer->RemoveAmmo( GetAmmoPerShot(), m_iPrimaryAmmoType );

#ifndef CLIENT_DLL
			// delayed ammo adding for the onhit attribute
			if ( m_iAmmoToAdd > 0 )
			{
				pPlayer->GiveAmmo( m_iAmmoToAdd, m_iPrimaryAmmoType );
				m_iAmmoToAdd = 0;
			}
#endif
		}
		else
		{
			pPlayer->RemoveAmmo( GetAmmoPerShot(), m_iSecondaryAmmoType );

#ifndef CLIENT_DLL
			// delayed ammo adding for the onhit attribute
			if ( m_iAmmoToAdd > 0 )
			{
				pPlayer->GiveAmmo( m_iAmmoToAdd, m_iSecondaryAmmoType );
				m_iAmmoToAdd = 0;
			}
#endif
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::HasPrimaryAmmo( void )
{
	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( GetOwner() && ( GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) < GetAmmoPerShot() ) )
			return false;
	}

	return BaseClass::HasPrimaryAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::CanDeploy( void )
{
	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( GetOwner() && ( GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) < GetAmmoPerShot() ) )
			return false;
	}

	return BaseClass::CanDeploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::CanBeSelected( void )
{
	if ( m_iPrimaryAmmoType == TF_AMMO_METAL )
	{
		if ( GetOwner() && ( GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) < GetAmmoPerShot() ) )
			return false;
	}

	return BaseClass::CanBeSelected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGun::GetAmmoPerShot( void )
{
	if ( IsEnergyWeapon() )
		return 0;
	else
	{
		int iAmmoPerShot = 0;
		CALL_ATTRIB_HOOK_INT( iAmmoPerShot, mod_ammo_per_shot );
		if ( iAmmoPerShot > 0 )
			return iAmmoPerShot;

		return m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;

	if ( flPunchAngle > 0 )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		pPlayer->SetPunchAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bullet!
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	FX_FireBullets(
		this,
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
		GetWeaponID(),
		m_iWeaponMode,
		CBaseEntity::GetPredictionRandomSeed( UseServerRandomSeed() ) & 255,
		GetWeaponSpread(),
		GetProjectileDamage(),
		IsCurrentAttackACrit() );
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer, int iRocketType )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create( this, trace.endpos, angForward, pPlayer, pPlayer );

	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire an energy ball
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireEnergyBall( CTFPlayer *pPlayer, bool bRing )
{
	PlayWeaponShootSound();

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, -8.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	trace_t trace;
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	if ( bRing )
	{
		CTFProjectile_EnergyRing* pProjectile = CTFProjectile_EnergyRing::Create( this, trace.endpos, angForward, 
			GetProjectileSpeed(), GetProjectileGravity(), pPlayer, pPlayer, GetParticleColor(1), GetParticleColor(2), IsCurrentAttackACrit() );
		if ( pProjectile )
		{
			pProjectile->SetWeaponID( GetWeaponID() );
			pProjectile->SetCritical( IsCurrentAttackACrit() );
#ifdef GAME_DLL
			pProjectile->SetDamage( GetProjectileDamage() );
#endif
		}
		return pProjectile;
	}
	else
	{
#ifdef GAME_DLL
		CTFProjectile_EnergyBall* pProjectile = CTFProjectile_EnergyBall::Create( trace.endpos, angForward, GetProjectileSpeed(), GetProjectileGravity(), pPlayer, pPlayer );
		if ( pProjectile )
		{
			pProjectile->SetLauncher( this );
			pProjectile->SetCritical( IsCurrentAttackACrit() );
			pProjectile->SetDamage( GetProjectileDamage() );
		}
		return pProjectile;
#endif
	}


	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a projectile nail
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, int iSpecificNail )
{
	PlayWeaponShootSound();

	Vector vecSrc;
	QAngle angForward;
	
	// Add some spread
	float flSpread = 1.5;
	flSpread += GetProjectileSpread();

	CTFBaseProjectile *pProjectile = NULL;
	switch( iSpecificNail )
	{
	case TF_PROJECTILE_SYRINGE:
		{
			Vector vecOffset( 16, 6, -8 );
			GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward );
			angForward.x += RandomFloat( -flSpread, flSpread );
			angForward.y += RandomFloat( -flSpread, flSpread );
			pProjectile = CTFProjectile_Syringe::Create( vecSrc, angForward, this, pPlayer, pPlayer, IsCurrentAttackACrit() );
		}
		break;
	default:
		Assert(0);
	}

	if ( pProjectile )
	{
		pProjectile->SetWeaponID( GetWeaponID() );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
#ifdef GAME_DLL
		pProjectile->SetLauncher( this );
		pProjectile->SetDamage( GetProjectileDamage() );
#endif
	}
	
	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a pipe bomb
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FirePipeBomb( CTFPlayer *pPlayer, int iPipeBombType )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL
	QAngle angEyes = pPlayer->EyeAngles();

	float flSpreadAngle = 0.0f; 
	CALL_ATTRIB_HOOK_FLOAT( flSpreadAngle, projectile_spread_angle );
	if ( flSpreadAngle > 0.0f )
	{
		QAngle angSpread = RandomAngle( -flSpreadAngle, flSpreadAngle );
		angSpread.z = 0.0f;
		angEyes += angSpread;
		DevMsg( "Fire bomb at %f %f %f\n", XYZ(angEyes) );
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angEyes, &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	float fRight = 8.f;
	if ( IsViewModelFlipped() )
	{
		fRight *= -1;
	}
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc +=  vecForward * 16.0f + vecRight * fRight + vecUp * -6.0f;

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	// If we started in solid, don't let them fire at all
	if ( trace.startsolid )
		return NULL;

	float flLaunchSpeed = GetProjectileSpeed();
	CALL_ATTRIB_HOOK_FLOAT( flLaunchSpeed, mult_projectile_range );
	Vector vecVelocity = ( vecForward * flLaunchSpeed ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );

	float flMultDmg = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flMultDmg, mult_dmg );
	
	// no spin for loch-n-load
	Vector angImpulse = AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 );
	int iNoSpin = 0;
	CALL_ATTRIB_HOOK_INT( iNoSpin, grenade_no_spin );
	if ( iNoSpin )
	{
		angImpulse.Zero();
	}

	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( trace.endpos, angEyes, vecVelocity, angImpulse, pPlayer, GetTFWpnData(), iPipeBombType, flMultDmg );

	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetLauncher( this );

		//float flFizzle = 0;
		//CALL_ATTRIB_HOOK_FLOAT( flFizzle, stickybomb_fizzle_time );
		//if ( flFizzle )
		//{
		//	pProjectile->SetDetonateTimerLength( flFizzle );
		//}
		CAttribute_String attrCustomModelName;
		GetCustomProjectileModel( &attrCustomModelName );
		if ( attrCustomModelName.has_value() )
		{
			pProjectile->SetModel( attrCustomModelName.value().c_str() );
			
			// Set the grenade size again. It was previously set in CTFWeaponBaseGrenadeProj::Spawn() 
			// during CTFGrenadePipebombProjectile::Create() above, but SetModel() resets it to the model's bounds.
			UTIL_SetSize( pProjectile, TF_GRENADE_PROJECTILE_MINS, TF_GRENADE_PROJECTILE_MAXS );
		}
	}

	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a flare
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireFlare( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	// Server only - create the flare.
#ifdef GAME_DLL

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Flare *pProjectile = CTFProjectile_Flare::Create( this, vecSrc, angForward, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetLauncher( this );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
		CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun *>( this );
		if ( pFlareGun && pFlareGun->GetFlareGunType() == FLAREGUN_DETONATE )
		{
			pFlareGun->AddFlare( pProjectile );
		}
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire an arrow
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireArrow( CTFPlayer *pPlayer, ProjectileType_t projectileType )
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, -8.0f, -3.0f );

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	CTFProjectile_Arrow *pProjectile = CTFProjectile_Arrow::Create( vecSrc, angForward, GetProjectileSpeed(), GetProjectileGravity(), projectileType, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetLauncher( this );
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );

		int iPenetrate = 0;
		CALL_ATTRIB_HOOK_INT( iPenetrate, projectile_penetration );
		if ( iPenetrate == 1 )
		{
			pProjectile->SetPenetrate( true );
		}
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Toss a Jar...of something...
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireJar( CTFPlayer *pPlayer )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FireFlameRocket( CTFPlayer *pPlayer )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PlayWeaponShootSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetWeaponSpread( void )
{
	float fSpread = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSpread;
	CALL_ATTRIB_HOOK_FLOAT( fSpread, mult_spread_scale );

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() ); 
	
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			if ( GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				fSpread *= 0.4f;
			}
			else
			{
				fSpread *= 0.1f;
			}
		}

		// Some weapons change fire delay based on player's health
		float flReducedHealthBonus = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flReducedHealthBonus, panic_attack_negative );
		if ( flReducedHealthBonus != 1.0f )
		{
			flReducedHealthBonus = RemapValClamped( pPlayer->HealthFraction(), 0.2f, 0.9f, flReducedHealthBonus, 1.0f );
			fSpread *= flReducedHealthBonus;
		}
		
		float flScaler = 0.f;
		CALL_ATTRIB_HOOK_FLOAT( flScaler, mult_spread_scales_consecutive );
		if ( flScaler != 0.f && m_iConsecutiveShots )
		{
			// We enter this on what is going to be the second shot, due to how/when m_iConsecutiveShots increments
			flScaler = RemapValClamped( (float)m_iConsecutiveShots, 1.f, 5.f, 1.125f, 1.5f );
			fSpread *= flScaler;
			//DevMsg( "Shot: %i  Scalar: %3.2f  Spread: %3.2f\n", m_iConsecutiveShots.Get(), flScaler, fSpread );
		}
	}

	return fSpread;
}

//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetCustomProjectileModel( CAttribute_String *attrCustomProjModel )
{
	// Must still add these to a precache somewhere
	// ie CTFGrenadePipebombProjectile::Precache()
	static CSchemaAttributeDefHandle pAttrDef_ProjectileEntityName( "custom projectile model" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pAttrDef_ProjectileEntityName && pItem )
	{
		pItem->FindAttribute( pAttrDef_ProjectileEntityName, attrCustomProjModel );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileDamage( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	float flDamage = (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	// Some weapons mod dmg when not disguised
	bool bDisguised = pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED );
	if ( bDisguised )
	{
		CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_disguised );
	}

	if ( pPlayer && ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || pPlayer->IsPlayerClass( TF_CLASS_PYRO ) ) )
	{	
		float flRageDamage = 1.f;
		CALL_ATTRIB_HOOK_FLOAT( flRageDamage, rage_damage );

		if ( flRageDamage > 1.f )
		{
			float flRageRatio = pPlayer->m_Shared.GetRageMeter() / 100.f;
			flRageDamage = (flRageDamage - 1.f) * flRageRatio;
			flDamage *= 1.f + flRageDamage;
		}
	}

#ifdef GAME_DLL
	float flMedicHealDamageBonus = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flMedicHealDamageBonus, medic_healed_damage_bonus );

	if ( flMedicHealDamageBonus > 1.f )
	{
		if ( pPlayer )
		{
			int numHealers = pPlayer->m_Shared.GetNumHealers();
			bool bHealedByMedic = false;
			for ( int i=0; i<numHealers; i++ )
			{
				if ( ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) ) != NULL )
				{
					bHealedByMedic = true;
				}
			}

			if ( bHealedByMedic )
			{
				flDamage *= flMedicHealDamageBonus;
			}
		}
	}

	if ( GetWeaponProjectileType() == TF_PROJECTILE_BULLET )
	{
		float flScaleDamage = 1.f;
		CALL_ATTRIB_HOOK_FLOAT( flScaleDamage, accuracy_scales_damage );
		if ( flScaleDamage > 1.f && m_iProjectilesFiredInTime )
		{
			// Bullets fired vs hit ratio over last x.x second(s)
			if ( gpGlobals->curtime < GetLastHitTime() + 0.7f )
			{
				float flRatio = (float)m_iHitsInTime / (float)m_iProjectilesFiredInTime;
				float flDmgMod = RemapValClamped( flRatio, 0.f, 1.f, 1.f, flScaleDamage );

// 				DevMsg( "A: %f - D: %f\n", flRatio, flDmgMod );
// 				DevMsg( "H: %d - F: %d\n", m_iHitsInTime, m_iFiredInTime );

				flDamage *= flDmgMod;
			}
			else
			{
				m_iHitsInTime = 0;
				m_iConsecutiveShots = 0;
			}
		}
	}

#endif

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#if !defined( CLIENT_DLL )

	// Make sure to zoom out before we holster the weapon.
	ZoomOut();
	SetContextThink( NULL, 0, ZOOM_CONTEXT );

#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoFireEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( ShouldDoMuzzleFlash() )
	{
		pPlayer->DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ToggleZoom( void )
{
	// Toggle the zoom.
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer )
	{
		if( pPlayer->GetFOV() >= 75 )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

	// Get the zoom animation time.
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomIn( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Set the weapon zoom.
	// TODO: The weapon fov should be gotten from the script file.
	float fBaseZoom = TF_WEAPON_ZOOM_FOV;

	// Disabled this for now, because we have no attributes using it
	//CALL_ATTRIB_HOOK_FLOAT( fBaseZoom, mult_zoom_fov );

	pPlayer->SetFOV( pPlayer, fBaseZoom, 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );

#if defined( CLIENT_DLL )
	// Doing this allows us to show/hide the player viewmodel/localmodel
	pPlayer->UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOut( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TF_COND_ZOOMED );
	}

#if defined( CLIENT_DLL )
	// Doing this allows us to show/hide the player viewmodel/localmodel
	pPlayer->UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOutIn( void )
{
	//Zoom out, set think to zoom back in.
	ZoomOut();
	SetContextThink( &CTFWeaponBaseGun::ZoomIn, gpGlobals->curtime + ZOOM_REZOOM_TIME, ZOOM_CONTEXT );
}

//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::HasLastShotCritical( void )
{
	if ( m_iClip1 == 1 )
	{
		int iAttr = 0;
		CALL_ATTRIB_HOOK_INT( iAttr, last_shot_crits );
		if ( iAttr )
		{
			return true;
		}
	}
	return false;
}