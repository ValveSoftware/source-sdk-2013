//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_THROWABLE_H
#define TF_WEAPON_THROWABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_jar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFThrowable C_TFThrowable
#define CTFThrowablePrimary C_TFThrowablePrimary
#define CTFThrowableSecondary C_TFThrowableSecondary
#define CTFThrowableMelee C_TFThrowableMelee
#define CTFThrowableUtility C_TFThrowableUtility
#define CTFProjectile_Throwable C_TFProjectile_Throwable
#define CTFProjectile_ThrowableRepel C_TFProjectile_ThrowableRepel
#define CTFProjectile_ThrowableBrick C_TFProjectile_ThrowableBrick
#define CTFProjectile_ThrowableBreadMonster C_TFProjectile_ThrowableBreadMonster
#define CTFProjectile_BreadMonster_Jarate C_TFProjectile_BreadMonster_Jarate

#endif

class CTFProjectile_Throwable;

// *************************************************************************************************************************
class CTFThrowable : public CTFJar, public ITFChargeUpWeapon
{
public:
	DECLARE_CLASS( CTFThrowable, CTFJar );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFThrowable();

	virtual void		Precache();
	virtual void		PrimaryAttack( void );
	virtual void		ItemPostFrame( void );
	virtual CBaseEntity *FireJar( CTFPlayer *pPlayer ) OVERRIDE;

	virtual int			GetWeaponID( void ) const				{ return TF_WEAPON_THROWABLE; }
	virtual const char*	GetEffectLabelText( void )				{ return "#TF_Throwable"; }
	virtual bool		ShowHudElement( void )					{ return true; }
	virtual const char*	 ModifyEventParticles( const char* token ) { return NULL; }

	virtual float		InternalGetEffectBarRechargeTime( void );
	virtual float		GetDetonationTime( void );
	
	
	// ITFChargeUpWeapon
	virtual bool CanCharge( void );
	virtual float GetChargeBeginTime( void );
	virtual float GetChargeMaxTime( void );			// Same as Det time

#ifdef GAME_DLL
	//virtual bool		ShouldSpeakWhenFiring( void )				{ return false; }

	//virtual const AngularImpulse GetAngularImpulse( void ){ return AngularImpulse( 300, 0, 0 ); }
	//virtual Vector				 GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual void		TossJarThink( void );
	virtual CTFProjectile_Throwable *FireProjectileInternal( void );

#endif

	CNetworkVar( float, m_flChargeBeginTime );
};

// *************************************************************************************************************************
class CTFThrowablePrimary : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFThrowablePrimary, CTFThrowable );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

class CTFThrowableSecondary : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFThrowableSecondary, CTFThrowable );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

class CTFThrowableMelee : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFThrowableMelee, CTFThrowable );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

class CTFThrowableUtility : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFThrowableUtility, CTFThrowable );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
};

// *************************************************************************************************************************
class CTFProjectile_Throwable : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_Throwable, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_THROWABLE; }
	virtual float		GetModelScale() const				{ return 1.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE{ return TF_DMG_CUSTOM_THROWABLE; }
	virtual bool		IsDeflectable() OVERRIDE			{ return true; }
	virtual bool		ExplodesOnHit()						{ return true; }
	virtual void		SetCustomPipebombModel()			{ return; }
	virtual float		GetShakeAmplitude()					{ return 0.0; }
	virtual float		GetProjectileSpeed()				{ return 1000.0f; }
	virtual float		GetProjectileMaxSpeed()				{ return 2500.0f; }
	virtual const char *GetThrowSoundEffect() const			{ return NULL; }

#ifdef GAME_DLL
	CTFProjectile_Throwable();

	virtual void Spawn( void )
	{
		SetModelScale( GetModelScale() );
		BaseClass::Spawn();
	}
	virtual void InitThrowable( float flChargePercent ) { m_flChargePercent = flChargePercent; }

	virtual int	GetProjectileType( void ) const OVERRIDE { return TF_PROJECTILE_THROWABLE; }

	virtual void OnHit( CBaseEntity *pOther );
	//virtual void Detonate();		// Timer based Explode
	virtual void Misfire()			{ }
	virtual void Explode();			// Explode Helper
	virtual void Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;

	virtual void ApplyBlastDamage( CTFPlayer *pThrower, Vector vecOrigin )												{ }
	virtual bool InitialExplodeEffects( CTFPlayer *pThrower, const trace_t *pTrace )									{ return false; }
	virtual void ExplodeEffectOnTarget( CTFPlayer *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )	{ }

	virtual float		GetDamage()							{ return 0.0f; }
	virtual float		GetDamageRadius() const				{ return 250.0f; }

	virtual const char *GetExplodeEffectParticle() const	{ return GetTeamNumber() == TF_TEAM_RED ? "" : ""; }
	virtual const char *GetExplodeEffectSound()	const		{ return ""; }

	virtual const AngularImpulse	 GetAngularImpulse( void )	{ return AngularImpulse( 300, 0, 0 ); }
	virtual Vector					 GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, float flCharge );

#endif	// GAME_DLL

#ifdef CLIENT_DLL
	virtual const char*	GetTrailParticleName( void )		{ return GetTeamNumber() == TF_TEAM_RED ? "trail_basic_red" : "trail_basic_blue"; }
#endif

protected:

#ifdef GAME_DLL
	float m_flChargePercent;
	bool m_bHit;
#endif // GAME_DLL

private:

};

// *************************************************************************************************************************
class CTFProjectile_ThrowableRepel : public CTFProjectile_Throwable
{
public:
	DECLARE_CLASS( CTFProjectile_ThrowableRepel, CTFProjectile_Throwable );
	DECLARE_NETWORKCLASS();

	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/c_models/c_balloon_default.mdl" ); }
	virtual bool		ExplodesOnHit()						{ return true; }

#ifdef GAME_DLL
	virtual void OnHit( CBaseEntity *pOther );

	virtual float		GetDamage()							{ return RemapVal( m_flChargePercent, 0, 1, 20.0f, 50.0f ); }
	virtual float		GetDamageRadius() const				{ return 0.0f; }

	virtual const char *GetExplodeEffectParticle() const	{ return "Explosion_bubbles"; }
	virtual const char *GetExplodeEffectSound()	const		{ return ""; }

#endif	// GAME_DLL

};

// *************************************************************************************************************************
class CTFProjectile_ThrowableBrick : public CTFProjectile_Throwable
{
public:
	DECLARE_CLASS( CTFProjectile_ThrowableBrick, CTFProjectile_Throwable );
	DECLARE_NETWORKCLASS();

	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/c_models/c_bread/c_bread_plainloaf.mdl" ); }
	virtual bool		ExplodesOnHit()						{ return false; }

#ifdef GAME_DLL
	virtual void OnHit( CBaseEntity *pOther );

	virtual float		GetDamage()							{ return RemapVal( m_flChargePercent, 0, 1, 40.0f, 70.0f ); }
	virtual float		GetDamageRadius() const				{ return 0.0f; }

	virtual const char *GetExplodeEffectParticle() const	{ return ""; }
	virtual const char *GetExplodeEffectSound()	const		{ return ""; }

#endif	// GAME_DLL

};

// *************************************************************************************************************************
class CTFProjectile_ThrowableBreadMonster : public CTFProjectile_Throwable
{
public:
	DECLARE_CLASS( CTFProjectile_ThrowableBreadMonster, CTFProjectile_Throwable );
	DECLARE_NETWORKCLASS();

	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/c_models/c_breadmonster/c_breadmonster.mdl" ); }
	virtual bool		ExplodesOnHit()						{ return true; }

#ifdef GAME_DLL
	virtual int			GetProjectileType( void ) const OVERRIDE { return TF_PROJECTILE_BREAD_MONSTER; }

	virtual void OnHit( CBaseEntity *pOther );
	virtual void Detonate();		// Timer based 'Explode' Just Remove
	virtual void Explode( trace_t *pTrace, int bitsDamageType );

	virtual float		GetDamage()							{ return RemapVal( m_flChargePercent, 0, 1, 40.0f, 85.0f ); }
	virtual float		GetDamageRadius() const				{ return 0.0f; }

	virtual const char *GetExplodeEffectParticle() const	{ return "breadjar_impact"; }
	virtual const char *GetThrowSoundEffect() const			{ return "Weapon_bm_throwable.throw"; }
	virtual const char *GetExplodeEffectSound()	const		{ return "Weapon_bm_throwable.smash"; }
#endif	// GAME_DLL

};


// *************************************************************************************************************************
//class CTFProjectile_BreadMonster_Jarate : public CTFProjectile_Throwable
//{
//public:
//	DECLARE_CLASS( CTFProjectile_BreadMonster_Jarate, CTFProjectile_Throwable );
//	DECLARE_NETWORKCLASS();
//
//	virtual int			GetProjectileType( void )			{ return TF_PROJECTILE_BREAD_MONSTER; }
//	virtual void		SetCustomPipebombModel()			{ SetModel( "models/weapons/c_models/c_breadmonster/c_breadmonster.mdl" ); }
//	virtual bool		ExplodesOnHit()						{ return true; }
////
////#ifdef GAME_DLL
//	virtual void OnHit( CBaseEntity *pOther );
//	virtual void Detonate();		// Timer based 'Explode' Just Remove
//	virtual void Explode( trace_t *pTrace, int bitsDamageType );
////
//	virtual float		GetDamage()							{ return 0.0f; }
//	virtual float		GetDamageRadius() const				{ return 0.0f; }
//
//	virtual const char *GetExplodeEffectParticle() const	{ return "breadjar_impact"; }
//	virtual const char *GetThrowSoundEffect() const			{ return "Weapon_bm_throwable.throw"; }
//	virtual const char *GetExplodeEffectSound()	const		{ return "Weapon_bm_throwable.smash"; }
//#endif	// GAME_DLL

//};



#endif // TF_WEAPON_THROWABLE_H
