//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Base Projectile
//
//=============================================================================
#ifndef TF_BASE_PROJECTILE_H
#define TF_BASE_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "baseprojectile.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "tempent.h"
// Server specific.
#else
	#include "iscorer.h"
#endif

#ifdef CLIENT_DLL
#define CTFBaseProjectile C_TFBaseProjectile
C_LocalTempEntity *ClientsideProjectileCallback( const CEffectData &data, float flGravityBase, const char *pszParticleName = NULL );
#endif

/*
CBaseProjectile
	|-  CTFBaseProjectile
			|- CTFProjectile_Nail
			|- CTFProjectile_Dart
			|- CTFProjectile_Syringe
			|- CTFProjectile_EnergyRing
	|-  CTFBaseRocket
			|- Soldier rocket
			|- Pyro rocket
			|- CTFProjectile_Flare
			|- CTFProjectile_Arrow
	|-  CBaseGrenade
			|- CTFWeaponBaseGrenadeProj
					|- CTFGrenadePipebombProjectile
*/

//=============================================================================
//
// Generic projectile
//
class CTFBaseProjectile : public CBaseProjectile
#if !defined( CLIENT_DLL )
	, public IScorer
#endif
{
public:

	DECLARE_CLASS( CTFBaseProjectile, CBaseProjectile );
	DECLARE_NETWORKCLASS();

	CTFBaseProjectile();
	~CTFBaseProjectile();

	void	Precache( void );
	void	Spawn( void );

	virtual int   GetWeaponID( void ) const { return m_iWeaponID; }
	void		  SetWeaponID( int iID ) { m_iWeaponID = iID; }

	bool		  IsCritical( void ) const			{ return m_bCritical; }
	virtual void  SetCritical( bool bCritical )		{ m_bCritical = bCritical; }
	
	CBaseEntity		*GetLauncher( void ) { return m_hLauncher; }

private:

	int				m_iWeaponID;
	bool			m_bCritical;

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );

	static CTFBaseProjectile *Create( const char *pszClassname, const Vector &vecOrigin, 
		const QAngle &vecAngles, CBaseEntity *pOwner, float flVelocity, short iProjModelIndex, const char *pszDispatchEffect = NULL, CBaseEntity *pScorer = NULL, bool bCritical = false, Vector vColor1=vec3_origin, Vector vColor2=vec3_origin );

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void ) { return 0.001f; }

#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );

private:

	float	 m_flSpawnTime;
#else

public:

	DECLARE_DATADESC();

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );

	virtual void	ProjectileTouch( CBaseEntity *pOther );

	virtual int		GetProjectileType ( void )					{ return TF_PROJECTILE_NONE; }	// Default unset

	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	virtual Vector	GetDamageForce( void );
	virtual int		GetDamageType( void );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const OVERRIDE;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual void	SetLauncher( CBaseEntity *pLauncher ) OVERRIDE { m_hLauncher = pLauncher; BaseClass::SetLauncher( pLauncher ); }

protected:

	void			FlyThink( void );

protected:
	float			m_flDamage;
	CBaseHandle		m_Scorer;

#endif // ndef CLIENT_DLL

protected:
	CNetworkHandle( CBaseEntity, m_hLauncher );
};

#endif	//TF_BASE_PROJECTILE_H
