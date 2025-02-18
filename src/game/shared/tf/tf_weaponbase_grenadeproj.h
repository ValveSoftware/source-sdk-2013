//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADEPROJ_H
#define TF_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#include "networkstringtabledefs.h"

#define TF_GRENADE_DESTROYABLE_TIMER	(0.25)

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFWeaponBaseGrenadeProj : public CBaseGrenade
{
public:

	DECLARE_CLASS( CTFWeaponBaseGrenadeProj, CBaseGrenade );
	DECLARE_NETWORKCLASS();

							CTFWeaponBaseGrenadeProj();
	virtual					~CTFWeaponBaseGrenadeProj();
	virtual void			Spawn();
	virtual void			Precache() OVERRIDE;

#ifdef GAME_DLL
	virtual void			InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
	virtual void			InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const int iDamage, const float flRadius );

	virtual int				GetBaseProjectileType() const { return TF_BASE_PROJECTILE_GRENADE; }
#endif

	// Unique identifier.
	virtual int GetWeaponID( void ) const { return TF_WEAPON_NONE; }
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );

	virtual float		GetShakeAmplitude( void ) { return 10.0; }
	virtual float		GetShakeRadius( void ) { return 300.0; }

	void				SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual int			GetDamageType();

	virtual void		SetLauncher( CBaseEntity *pLauncher ) OVERRIDE { m_hLauncher = pLauncher; BaseClass::SetLauncher( pLauncher ); }
	CBaseEntity			*GetLauncher( void ) { return m_hLauncher; }
	virtual void		IncrementDeflected( void ) { m_iDeflected++; }
	void				ResetDeflected( void ) { m_iDeflected = 0; }
	int					GetDeflected( void ) { return m_iDeflected; }
	void				SetDeflectOwner( CBaseEntity *pPlayer ) { m_hDeflectOwner = pPlayer; }
	CBaseEntity			*GetDeflectOwner( void ) { return m_hDeflectOwner; }
	virtual float		GetDamageRadius();
	virtual int			GetDamageCustom();
	virtual int			GetCustomParticleIndex() { return INVALID_STRING_INDEX; }
	void				BounceOff( IPhysicsObject *pPhysics );

protected:
	CNetworkHandleForDerived( CBaseEntity, m_hLauncher );

private:

	CTFWeaponBaseGrenadeProj( const CTFWeaponBaseGrenadeProj & );
	CNetworkVar( int,	m_iDeflected );

	CNetworkHandle( CBaseEntity, m_hDeflectOwner );

	// Client specific.
#ifdef CLIENT_DLL

public:

	virtual void			OnDataChanged( DataUpdateType_t type );

	float					m_flSpawnTime;
	bool					m_bCritical;

	// Server specific.
#else

public:

	DECLARE_DATADESC();

	static CTFWeaponBaseGrenadeProj *Create( const char *szName, const Vector &position, const QAngle &angles, 
				const Vector &velocity, const AngularImpulse &angVelocity, 
				CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags );

	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	void					Detonate( void );

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	virtual void 			Destroy( bool bBlinkOut = true, bool bBreak = false ) OVERRIDE;

	void					SetTimer( float time ){ m_flDetonateTime = time; }
	float					GetDetonateTime( void ){ return m_flDetonateTime; }

	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual bool			IsAllowedToExplode( void ) { return true; }
	void					Explode( trace_t *pTrace, int bitsDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }

	bool					IsCritical() { return m_bCritical; }
	virtual bool			IsDestroyable( bool bOrbAttack = false ) OVERRIDE { return ( !bOrbAttack ? ( gpGlobals->curtime > m_flDestroyableTime ) : true ); }

	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

protected:


	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity ) OVERRIDE;

	float					m_flDetonateTime;
	CHandle<CBaseEntity>	m_hEnemy;
private:

	bool					m_bInSolid;

	CNetworkVar( bool,		m_bCritical );

	float					m_flDestroyableTime;
	bool					m_bIsMerasmusGrenade;

#endif
};

#endif // TF_WEAPONBASE_GRENADEPROJ_H
