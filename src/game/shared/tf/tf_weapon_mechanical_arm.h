//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_MECHANICAL_ARM_H
#define TF_WEAPON_MECHANICAL_ARM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_shareddefs.h"
#include "tf_viewmodel.h"
#ifdef GAME_DLL
#include "tf_projectile_rocket.h"
#else
#include "c_tf_projectile_rocket.h"
#endif

#ifdef CLIENT_DLL
#define CTFMechanicalArm C_TFMechanicalArm
#define CTFProjectile_MechanicalArmOrb	C_TFProjectile_MechanicalArmOrb
#endif


//=============================================================================
//
// Mechanical Arm class.
//
class CTFMechanicalArm : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFMechanicalArm, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFMechanicalArm();
	~CTFMechanicalArm();

	virtual void	Precache();

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MECHANICAL_ARM; }
	virtual int		GetCustomDamageType( ) const		{ return TF_DMG_CUSTOM_PLASMA; }

	virtual int		GetAmmoPerShot( void );

	virtual bool	UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState );

#ifdef CLIENT_DLL
	virtual void	OnDataChanged( DataUpdateType_t updateType );
#endif // CLIENT_DLL

private:

	bool ShockAttack( void );
#ifdef GAME_DLL
	bool IsValidVictim( CTFPlayer *pOwner, CBaseEntity *pTarget );
	void ShockVictim( CTFPlayer *pOwner, CBaseEntity *pTarget );
#else
	void StopParticleBeam( void );
	void UpdateParticleBeam( void );
	HPARTICLEFFECT m_pParticleBeamEffect;
	HPARTICLEFFECT m_pParticleBeamSpark;
	C_BaseEntity  *m_pEffectOwner;
#endif // CLIENT_DLL
};


//=============================================================================
//
// Mechanical Arm alt-fire.
//
class CTFProjectile_MechanicalArmOrb : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_MechanicalArmOrb, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_MechanicalArmOrb();
	~CTFProjectile_MechanicalArmOrb();

	virtual void Precache() OVERRIDE;
	virtual void Spawn() OVERRIDE;
#ifdef GAME_DLL
	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE;
	bool ShouldProjectileIgnore( CBaseEntity *pOther );
	void ExplodeAndRemove( void );
	void ZapPlayer( const CTakeDamageInfo &info, trace_t *pTrace, CTFPlayer *pTFPlayer );
	void CheckForPlayers( int nNumToZap );
	void CheckForProjectiles( void );
	void OrbThink( void );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void CreateTrails( void ) OVERRIDE;
	virtual const char *GetTrailParticleName( void ) OVERRIDE { return NULL; }
#endif

private:
	float m_flOrbNextAttackTime;
#ifdef CLIENT_DLL
	CNewParticleEffect *m_pTrailParticle;
	int m_iTeamNumPrev;
#endif // CLIENT_DLL

};

#endif // TF_WEAPON_MECHANICAL_ARM_H
