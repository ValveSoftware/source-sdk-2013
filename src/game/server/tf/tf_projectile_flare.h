//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Flare Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_FLARE_H
#define TF_PROJECTILE_FLARE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#include "iscorer.h"

// Base force scaler
#define TF_FLARE_PELLET_FORCE 20.0f

// The farther the pellet flies, the more force it does up to a max of 4X at 1 second
#define TF_FLARE_PELLET_FORCE_DISTANCE_SCALE 3.75f

// We need this to get the player off the ground
#define TF_FLARE_PELLET_FORCE_UPWARD 275

// We need this to get the heavy off the ground
#define TF_FLARE_PELLET_FORCE_UPWARD_HEAVY 525


class CTFProjectile_Flare : public CTFBaseRocket, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_Flare, CTFBaseRocket );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFProjectile_Flare();
	~CTFProjectile_Flare();

	// Creation.
	static CTFProjectile_Flare *Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void	Spawn();
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_FLAREGUN; }

	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	void			Explode_Air( trace_t *pTrace, int bitsDamageType, bool bSelfOnly = false );
	void			Detonate( bool bSelfOnly = false );
	virtual float	GetRadius( void );
	void			SendDeathNotice( void );

	float			GetTimeAlive( void ) const { return gpGlobals->curtime - m_flCreationTime; }
	bool			IsFromTaunt( void ) const { return m_bIsFromTaunt; }

	void			ImpactThink( void );

	// Implement this if you use MOVETYPE_CUSTOM
	virtual void	PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );

	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual int		GetDamageType();

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual bool	IsDestroyable( bool bOrbAttack = false ) OVERRIDE { return ( !bOrbAttack ? false : true ); }

	float GetProjectileSpeed( void ) const;
	float GetHeatSeekPower( void ) const;

private:

	CBaseHandle m_Scorer;

	bool		m_bIsFromTaunt;
	float		m_flCreationTime;
	float		m_flImpactTime;
	Vector		m_vecImpactNormal;

	CNetworkVar( bool, m_bCritical );

	bool		m_bImpact;

	float		m_flNextSeekUpdate;
};

#endif	//TF_PROJECTILE_FLARE_H
