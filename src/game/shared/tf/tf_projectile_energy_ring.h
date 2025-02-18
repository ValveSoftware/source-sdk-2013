//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Energy Ring Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ENERGY_RING_H
#define TF_PROJECTILE_ENERGY_RING_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

#include "tf_projectile_base.h"
#include "tf_weapon_flamethrower.h"

#ifdef CLIENT_DLL
#define CTFProjectile_EnergyRing C_TFProjectile_EnergyRing
#endif

class CTFProjectile_EnergyRing : public CTFBaseProjectile
{
public:

	DECLARE_CLASS( CTFProjectile_EnergyRing, CTFBaseProjectile );
	DECLARE_NETWORKCLASS();

	CTFProjectile_EnergyRing();

	virtual const char* GetProjectileModelName( void );
	virtual float GetGravity( void );

	// Creation.
	static CTFProjectile_EnergyRing *Create( CTFWeaponBaseGun *pLauncher, const Vector& vecOrigin, const QAngle& vecAngles, float fSpeed, float fGravity, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, Vector vColor1=vec3_origin, Vector vColor2=vec3_origin, bool bCritical=false );	
	virtual void	Spawn();
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return ShouldPenetrate() ? TF_WEAPON_RAYGUN : TF_WEAPON_DRG_POMSON; }

#ifdef GAME_DLL
	virtual void	ProjectileTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void	ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity ) OVERRIDE;
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
#endif

	virtual bool	CanHeadshot() { return false; }

	virtual void	ImpactTeamPlayer( CTFPlayer *pOther ) {}

	virtual float	GetDamage();
	virtual int		GetDamageCustom() { return TF_DMG_CUSTOM_PLASMA; }

	virtual bool	IsDeflectable() { return false; }

	void			SetColor( int idx, Vector vColor ) { if ( idx==1 ) m_vColor1=vColor; else m_vColor2=vColor; }

	float			GetInitialVelocity( void );

private:

	bool			ShouldPenetrate() const;
	const char*		GetTrailParticleName() const;

	Vector			m_vColor1;
	Vector			m_vColor2;

	Vector			m_vecPrevPos;

#ifdef GAME_DLL
	float m_flLastHitTime;
	void PlayImpactEffects( const Vector& vecPos, bool bHitFlesh );
#endif

protected:
	float			m_flInitTime;
};

#endif	//TF_PROJECTILE_ENERGY_RING_H
