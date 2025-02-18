//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_FLAREGUN_H
#define TF_WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#ifdef GAME_DLL
#include "tf_projectile_flare.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFFlareGun C_TFFlareGun
#define CTFFlareGun_Revenge C_TFFlareGun_Revenge
#endif

enum FlareGunTypes_t
{
	FLAREGUN_NORMAL = 0,
	FLAREGUN_DETONATE,
	FLAREGUN_GRORDBORT,
	FLAREGUN_SCORCHSHOT
};

//=============================================================================
//
// TF Weapon Flare gun.
//
class CTFFlareGun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFFlareGun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFFlareGun();
	~CTFFlareGun();

	virtual void	Precache();
	virtual void	DestroySounds( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool Deploy( void );
	virtual void ItemPostFrame( void );
	virtual void WeaponReset( void );

	int				GetFlareGunType( void ) const	{ int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; }

#ifdef GAME_DLL
	void			AddFlare( CTFProjectile_Flare *pFlare );
	void			DeathNotice( CBaseEntity *pVictim );

	virtual float	GetAfterburnRateOnHit() const OVERRIDE;
#endif

	virtual void StartCharge( void );
	virtual void StopCharge( void );
	virtual void ChargePostFrame( void ) {}	// Derived weapons can figure out what to do with this
	virtual const char* GetChargeEffect( void ) const { return "drg_cowmangler_muzzleflash_chargeup"; }

	float GetChargeBeginTime( void ) const { return m_flChargeBeginTime; }

protected:

#ifdef GAME_DLL
	typedef CHandle<CTFProjectile_Flare>	FlareHandle;
	CUtlVector <FlareHandle>				m_Flares;
	int										m_iFlareCount;
#endif

protected:
	void StartChargeStartTime() { m_flChargeBeginTime = gpGlobals->curtime; }

#ifdef CLIENT_DLL
	virtual void DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	void ClientEffectsThink( void );
	void StartChargeEffects();
	void StopChargeEffects();
	virtual bool ShouldPlayClientReloadSound() { return true; }
#endif

private:

	CNetworkVar( float, m_flChargeBeginTime );

	bool m_bEffectsThinking;
	float m_flLastDenySoundTime;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pChargeLoop;
	bool m_bReadyToFire;
#endif

	CTFFlareGun( const CTFFlareGun & ) {}
};


//---------------------------------------------------------
class CTFFlareGun_Revenge : public CTFFlareGun
{
public:
	DECLARE_CLASS( CTFFlareGun_Revenge, CTFFlareGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlareGun_Revenge();

	virtual void Precache();

	virtual int GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN_REVENGE; }
	virtual bool IsEnergyWeapon( void ) const { return true; }

	virtual int GetCustomDamageType() const;

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool Deploy( void );

#ifdef GAME_DLL
	virtual void Detach();
#endif

	int GetCount( void );
	float GetProgress( void ) { return 0.0f; }
	const char* GetEffectLabelText( void )			{ return "#TF_CRITS"; }

	const char* GetMuzzleFlashParticleEffect( void ) { return "drg_manmelter_muzzleflash"; }

	virtual void PrimaryAttack();
	virtual void SecondaryAttack( void );
	virtual void ChargePostFrame( void );
	virtual const char* GetChargeEffect( void ) const { return "drg_manmelter_vacuum"; }
	virtual const char* ReadyToFireSound( void ) const { return "Weapon_SniperRailgun.NonScoped"; }

#ifdef CLIENT_DLL
	virtual void OnDataChanged( DataUpdateType_t type );
	void DoAbsorbEffect( void );
#endif

private:
	bool ExtinguishPlayerInternal( CTFPlayer *pTarget, CTFPlayer *pOwner );

	CNetworkVar( float, m_fLastExtinguishTime );

#ifdef CLIENT_DLL
	int m_nOldRevengeCrits;
#endif

};

#endif // TF_WEAPON_FLAREGUN_H
