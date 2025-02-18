//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#ifndef TF_WEAPONBASE_GUN_H
#define TF_WEAPONBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
	\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
	\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
	\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );

#if defined( CLIENT_DLL )
#define CTFWeaponBaseGun C_TFWeaponBaseGun
#endif

#define ZOOM_CONTEXT		"ZoomContext"
#define ZOOM_REZOOM_TIME	1.4f

//=============================================================================
//
// Weapon Base Melee Gun
//
class CTFWeaponBaseGun : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseGun, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseGun();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	// Derived classes call this to fire a bullet.
	//bool TFBaseGunFire( void );

	virtual void DoFireEffects();
	virtual bool ShouldDoMuzzleFlash( void ) { return true; }

	void ToggleZoom( void );

	virtual int GetWeaponProjectileType( void ) const { return m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iProjectile; }
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void RemoveProjectileAmmo( CTFPlayer *pPlayer );
	virtual void ModifyProjectile( CBaseEntity* pProj ) {};

	virtual void FireBullet( CTFPlayer *pPlayer );
	CBaseEntity *FireRocket( CTFPlayer *pPlayer, int iRocketType=0 );
	CBaseEntity *FireNail( CTFPlayer *pPlayer, int iSpecificNail );
	virtual CBaseEntity *FirePipeBomb( CTFPlayer *pPlayer, int iPipeBombType );
	CBaseEntity *FireFlare( CTFPlayer *pPlayer );
	virtual CBaseEntity *FireArrow( CTFPlayer *pPlayer, ProjectileType_t projectileType );
	virtual CBaseEntity *FireJar( CTFPlayer *pPlayer );
	virtual CBaseEntity *FireFlameRocket( CTFPlayer *pPlayer );
	virtual CBaseEntity *FireEnergyBall( CTFPlayer *pPlayer, bool bRing=false );

	virtual bool HasLastShotCritical( void );

	virtual float GetWeaponSpread( void );
	virtual void  GetCustomProjectileModel( CAttribute_String *attrCustomProjModel );
	virtual float GetProjectileSpeed( void ) { return 0.0f; }
	virtual float GetProjectileGravity( void ) { return 0.f; }
	virtual float GetProjectileSpread( void ) { return 0.0f; }

	virtual int GetAmmoPerShot( void );

	void UpdatePunchAngles( CTFPlayer *pPlayer );
	virtual float GetProjectileDamage( void );

	virtual bool ShouldPlayFireAnim( void ) { return true; }

	virtual void ZoomIn( void );
	virtual void ZoomOut( void );
	void ZoomOutIn( void );

	virtual void PlayWeaponShootSound( void );
	virtual bool HasPrimaryAmmo( void );
	virtual bool CanDeploy( void );
	virtual bool CanBeSelected( void );

	virtual bool ShouldRemoveDisguiseOnPrimaryAttack() const;

private:

	CTFWeaponBaseGun( const CTFWeaponBaseGun & );
};

#endif // TF_WEAPONBASE_GUN_H
