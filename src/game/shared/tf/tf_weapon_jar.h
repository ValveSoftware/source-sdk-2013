//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_JAR_H
#define TF_WEAPON_JAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_pipebomb.h"

#ifdef CLIENT_DLL
#define CTFJar C_TFJar
#define CTFProjectile_Jar C_TFProjectile_Jar
#define CTFJarMilk C_TFJarMilk
#define CTFCleaver C_TFCleaver
#define CTFThrowable C_TFThrowable
#define CTFProjectile_JarMilk C_TFProjectile_JarMilk
#define CTFProjectile_Cleaver C_TFProjectile_Cleaver
#define CTFProjectile_Throwable C_TFProjectile_Throwable
#endif

class CTFProjectile_Jar;

enum EThrowableTypes 
{
	EThrowableBase				= 0,
};

#define JAR_EXPLODE_RADIUS 200		// TF_ROCKET_RADIUS and grenade explosions is 146
#define TF_WEAPON_PEEJAR_EXPLODE_SOUND	"Jar.Explode"

//=============================================================================
//
// Jar class.
//
class CTFJar : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFJar, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFJar();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_JAR; }
	virtual float		GetProjectileSpeed( void );
	virtual void		PrimaryAttack( void );

	float				GetProgress( void ) { return GetEffectBarProgress(); }

	virtual CBaseEntity *FireJar( CTFPlayer *pPlayer );
	virtual void		TossJarThink( void );

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 20.1; }

	virtual const char*			GetEffectLabelText( void ) { return "#TF_JAR"; }

	virtual void		GetProjectileEntityName( CAttribute_String *attrProjectileEntityName );

#ifdef GAME_DLL
	virtual const AngularImpulse GetAngularImpulse( void ){ return AngularImpulse( 300, 0, 0 ); }
	virtual Vector GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual bool		ShouldSpeakWhenFiring( void ){ return true; }

//	virtual bool		SendWeaponAnim( int iActivity );

	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

	virtual bool CanThrowUnderWater( void ){ return false; }

private:

	int m_iProjectileType;

	CTFJar( const CTFJar & ) {}
};

// *************************************************************************************************************************
class CTFJarMilk : public CTFJar
{
public:
	DECLARE_CLASS( CTFJarMilk, CTFJar );
	DECLARE_NETWORKCLASS(); 

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_JAR_MILK; }

#ifdef GAME_DLL
	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

#ifdef CLIENT_DLL
	const char*			ModifyEventParticles( const char* token );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
#endif
};

// *************************************************************************************************************************
class CTFCleaver : public CTFJar
{
public:
	DECLARE_CLASS( CTFCleaver, CTFJar );
	DECLARE_NETWORKCLASS(); 

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CLEAVER; }
	virtual float		GetProjectileSpeed( void );

	virtual void		SecondaryAttack( void );

	virtual const char*			GetEffectLabelText( void ) { return "#TF_CLEAVER"; }

	virtual float		InternalGetEffectBarRechargeTime( void ) { return 5.1; }

#ifdef GAME_DLL
	virtual const AngularImpulse GetAngularImpulse( void ){ return AngularImpulse( 0, 500, 0 ); }
	virtual Vector GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual bool		ShouldSpeakWhenFiring( void ){ return false; }

	virtual CTFProjectile_Jar	*CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
#endif

#ifdef CLIENT_DLL
	const char*			ModifyEventParticles( const char* token );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
#endif
};

//=============================================================================
//
// JarBomb projectile class.
//
class CTFProjectile_Jar : public CTFGrenadePipebombProjectile
{
public:
	DECLARE_CLASS( CTFProjectile_Jar, CTFGrenadePipebombProjectile );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void );
#endif

#ifdef GAME_DLL
	CTFProjectile_Jar();

	static CTFProjectile_Jar *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual int			GetProjectileType() const OVERRIDE			{ return m_iProjectileType; }

	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_GRENADE_JAR; }
	virtual float		GetDamage()									{ return 0.f; }
	virtual bool		ExplodesOnHit()								{ return true; }

	virtual void		Precache() OVERRIDE;
	virtual void		SetCustomPipebombModel() OVERRIDE;

	virtual float		GetDamageRadius()							{ return JAR_EXPLODE_RADIUS ; }
	virtual void		OnHit( CBaseEntity *pOther )				{}
	virtual void		OnHitWorld( void )							{}
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;
	

	virtual void		PipebombTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void		VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) OVERRIDE;
	void				VPhysicsCollisionThink( void );

	void OnBreadMonsterHit( CBaseEntity *pOther, trace_t *pTrace );

	bool PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim );
	void GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex );
	void CreateStickyAttachmentToTarget( CTFPlayer *pOwner, CTFPlayer *pVictim, trace_t *trace );

	virtual const char* GetImpactEffect() { return "peejar_impact"; }
	virtual ETFCond		GetEffectCondition( void ) { return TF_COND_URINE; }

	virtual const char* GetExplodeSound() { return TF_WEAPON_PEEJAR_EXPLODE_SOUND; }

protected:
	Vector		m_vCollisionVelocity;
	int			m_iProjectileType;
#endif

};

// *************************************************************************************************************************
class CTFProjectile_JarMilk : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_JarMilk, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CTFProjectile_JarMilk *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual void		SetCustomPipebombModel() OVERRIDE;

	virtual const char* GetImpactEffect() OVERRIDE { return "peejar_impact_milk"; }
	virtual ETFCond		GetEffectCondition( void ) OVERRIDE { return TF_COND_MAD_MILK; }
#endif

	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_GRENADE_JAR_MILK; }

	virtual void		Precache() OVERRIDE;
	
};

// *************************************************************************************************************************
class CTFProjectile_Cleaver : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_Cleaver, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	virtual const char*			GetTrailParticleName( void );
#endif

#ifdef GAME_DLL
	static CTFProjectile_Cleaver *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int nSkin );
#endif

	CTFProjectile_Cleaver();

	virtual int			GetWeaponID( void ) const OVERRIDE		{ return TF_WEAPON_GRENADE_CLEAVER; }

	virtual void		Precache() OVERRIDE;
	virtual void		SetCustomPipebombModel() OVERRIDE;

#ifdef GAME_DLL
	virtual float		GetDamage() OVERRIDE { return 50.f; }
	virtual void		OnHit( CBaseEntity *pOther ) OVERRIDE;
	virtual void		Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;
	virtual void		Detonate() OVERRIDE;
	virtual const char* GetImpactEffect() OVERRIDE { return ""; }
	virtual ETFCond		GetEffectCondition( void ) OVERRIDE { return TF_COND_BLEEDING; }
	
	void				RemoveCleaver( void );

private:
	bool m_bHitPlayer;
	bool m_bSoundPlayed;
#endif
};

void JarExplode( int iEntIndex, CTFPlayer *pAttacker, CBaseEntity *pOriginalWeapon, CBaseEntity *pWeapon, const Vector& vContactPoint, int iTeam, float flRadius, ETFCond cond, float flDuration, const char *pszImpactEffect, const char *pszExplodeSound );

#endif // TF_WEAPON_JAR_H
