//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_ROCKET_H
#define TF_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "baseprojectile.h"

// Server specific.
#ifdef GAME_DLL
#include "smoke_trail.h"
#endif

#ifdef CLIENT_DLL
#define CTFBaseRocket C_TFBaseRocket
#endif

#define TF_ROCKET_RADIUS_FOR_RJS	(110.0f * 1.1f)	// radius * TF scale up factor (121) - Used when applying damage to attacker.
#define TF_ROCKET_RADIUS			(146)			// Radius used when applying damage to others
#define TF_FLARE_DET_RADIUS			(110)			// Special version of the flare that can be detonated by the player
#define TF_FLARE_RADIUS_FOR_FJS		(100.0f)
#define TF_ROCKET_DESTROYABLE_TIMER	(0.25)


//=============================================================================
//
// TF Base Rocket.
//
class CTFBaseRocket : public CBaseProjectile
{

//=============================================================================
//
// Shared (client/server).
//
public:

	DECLARE_CLASS( CTFBaseRocket, CBaseProjectile );
	DECLARE_NETWORKCLASS();

			CTFBaseRocket();
			~CTFBaseRocket();

	void	Precache( void );
	void	Spawn( void );

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_ROCKETLAUNCHER; }
	virtual int		GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }
	virtual void	IncrementDeflected( void ) { m_iDeflected++; }
	void			ResetDeflected( void ) { m_iDeflected = 0; }
	int				GetDeflected( void ) { return m_iDeflected; }

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );
	CNetworkVar( int, m_iDeflected );

//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	CreateTrails( void ) { }
	CBaseEntity		*GetLauncher( void ) { return m_hLauncher; }

protected:

	float	 m_flSpawnTime;
	int		m_iCachedDeflect;
	CNetworkHandle( CBaseEntity, m_hLauncher );

//=============================================================================
//
// Server specific.
//
#else

public:

	DECLARE_DATADESC();

	static CTFBaseRocket *Create( CBaseEntity *pLauncher, const char *szClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );	

	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	void			CheckForStunOnImpact( CTFPlayer* pTarget );
	int				GetStunLevel( void );

	virtual bool	ShouldNotDetonate( void );
	virtual void	Destroy( bool bBlinkOut = true, bool bBreakRocket = false ) OVERRIDE;

	virtual float	GetDamage() { return m_flDamage; }
	virtual int		GetDamageType() { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int		GetDamageCustom() { return TF_DMG_CUSTOM_NONE; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	virtual float	GetRadius();

	virtual void	SetDamageForceScale( float flScale ) {  m_flDamageForceScale = flScale; }
	virtual float	GetDamageForceScale() { return m_flDamageForceScale; }

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

	void			SetHomingTarget( CBaseEntity *pHomingTarget );

	virtual void	SetLauncher( CBaseEntity *pLauncher ) OVERRIDE { m_hLauncher = pLauncher; BaseClass::SetLauncher( pLauncher ); }
	CBaseEntity		*GetLauncher( void ) { return m_hLauncher; }

	virtual bool	IsDestroyable( bool bOrbAttack = false ) OVERRIDE { return ( !bOrbAttack ? ( gpGlobals->curtime > m_flDestroyableTime ) : true ); }

	CBaseEntity		*GetOwnerPlayer( void ) const;

protected:

	// Not networked.
	float					m_flDamage;

	CNetworkHandle( CBaseEntity, m_hLauncher );

	float					m_flDestroyableTime;
	bool					m_bCritical;
	bool					m_bStunOnImpact;

	float					m_flDamageForceScale;

	CHandle<CBaseEntity>	m_hEnemy;

#endif
};


#endif // TF_WEAPONBASE_ROCKET_H
