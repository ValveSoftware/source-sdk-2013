//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Particle Cannon
//
//=============================================================================
#ifndef TF_WEAPON_PARTICLE_CANNON_H
#define TF_WEAPON_PARTICLE_CANNON_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_rocketlauncher.h"

#ifdef CLIENT_DLL
#include "particle_property.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFParticleCannon C_TFParticleCannon
#endif

#define TF_PARTICLE_MAX_CHARGE_TIME 2.f

class CTFParticleCannon : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFParticleCannon, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFParticleCannon();
	~CTFParticleCannon();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();

	virtual void	Precache();
#endif

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PARTICLE_CANNON; }
	virtual float	GetProjectileSpeed( void );
	virtual float	GetProjectileGravity( void );
	virtual bool	IsViewModelFlipped( void );

	const char*		GetEffectLabelText( void )			{ return "#TF_MANGLER"; }
	float			GetProgress( void );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	WeaponReset( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );

	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Reload( void );
	virtual bool	CanPerformSecondaryAttack( void );
	virtual void	FireChargedShot();
	virtual void	ModifyProjectile( CBaseEntity* pProj );

	virtual void	PlayWeaponShootSound( void );

	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual bool	IsEnergyWeapon( void ) const { return true; }

	virtual bool	ShouldPlayFireAnim( void ) { return !m_bChargedShot; }

#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			StartEffects( void );
	void			ClientEffectsThink( void );
	void			CreateChargeEffect( void );
	void			PlayChargeSound( void );

	virtual void	DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	virtual bool	ShouldPlayClientReloadSound() { return true; }
	virtual void	CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );

#endif

	virtual char const*	GetShootSound( int iIndex ) const;

	// Charged shot support.
	virtual float	GetChargeBeginTime( void ) { return m_flChargeBeginTime; }
	virtual float	GetChargeMaxTime( void ) { return TF_PARTICLE_MAX_CHARGE_TIME; }
	virtual float	GetChargeForceReleaseTime( void ) {
		float fBaseTime = GetChargeMaxTime();
		float fBaseFireSpeed = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;
		return fBaseTime * ApplyFireDelay(fBaseFireSpeed) / fBaseFireSpeed; //reduce charge shot wind-up time according to fire speed reduction
	}
	virtual float	Energy_GetShotCost( void ) const { return 5.f; }
	virtual float	Energy_GetRechargeCost( void ) const { return 5.f; }

	bool			CanChargeFire( void ) { if (Energy_FullyCharged() && (m_flChargeBeginTime<=0)) return true; else return false; }

	virtual bool	OwnerCanTaunt( void );

#ifdef GAME_DLL
	virtual float	GetAfterburnRateOnHit() const OVERRIDE;
	virtual float	GetInitialAfterburnDuration() const OVERRIDE { return 0.f; }
#endif // GAME_DLL

private:
	CNetworkVar( float, m_flChargeBeginTime );
	CNetworkVar( int, m_iChargeEffect );
	bool m_bChargedShot;
	CSoundPatch* m_pSoundCur; //for playing the charged shot sound.
	float f_ChargeSoundStartTime; //checking if the sound has started playing so we don't play it again.


#ifdef CLIENT_DLL
	bool		m_bEffectsThinking;
	int			m_iChargeEffectBase;
#endif
};

#endif // TF_WEAPON_PARTICLE_CANNON_H
