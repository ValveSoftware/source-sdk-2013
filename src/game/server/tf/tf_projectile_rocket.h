//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ROCKET_H
#define TF_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#include "iscorer.h"


//=============================================================================
//
// Generic rocket.
//
class CTFProjectile_Rocket : public CTFBaseRocket, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_Rocket, CTFBaseRocket );
	DECLARE_NETWORKCLASS();

	// Creation.
	static CTFProjectile_Rocket *Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void Spawn();
	virtual void Precache();
	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE;

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );

	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	bool	IsCritical() { return m_bCritical; }
	virtual int		GetDamageType();
	virtual int		GetDamageCustom();
	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	void SetDirectHit( bool bDirectHit ){ m_bDirectHit = bDirectHit; }
	virtual int		GetWeaponID( void ) const { return ( m_bDirectHit ? TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT : TF_WEAPON_ROCKETLAUNCHER ); }

	void SetEyeBallRocket( bool state ){ m_bEyeBallRocket = state; }
	void SetSpell( bool bSpell ) { m_bSpell = bSpell; }

private:
	CBaseHandle m_Scorer;
	CNetworkVar( bool,	m_bCritical );
	bool m_bDirectHit;
	bool m_bEyeBallRocket;
	bool m_bSpell;
};

#endif	//TF_PROJECTILE_ROCKET_H
