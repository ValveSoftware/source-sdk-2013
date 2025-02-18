//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Energy Ball Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ENERGY_BALL_H
#define TF_PROJECTILE_ENERGY_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_player.h"
#include "tf_weaponbase_rocket.h"
#include "iscorer.h"

class CTFProjectile_EnergyBall : public CTFBaseRocket//, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_EnergyBall, CTFBaseRocket );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFProjectile_EnergyBall();
	~CTFProjectile_EnergyBall();

	// Creation.
	static CTFProjectile_EnergyBall *Create( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void	InitEnergyBall( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	
	virtual void	Spawn();
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PARTICLE_CANNON; }

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void			SetScorer( CBaseEntity *pScorer );
	void			SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	virtual bool	CanHeadshot() { return false; }

	void			ImpactSound( const char *pszSoundName, bool bLoudForAttacker = false );
	virtual void	ImpactTeamPlayer( CTFPlayer *pOther ) {}

	void			FadeOut( int iTime );
	void			RemoveThink();

	virtual float	GetDamage();
	virtual int		GetDamageCustom();
	virtual int		GetDamageType();

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	void			SetChargedShot( bool bCharged ) { m_bChargedShot = bCharged; }

	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );

	void			SetColor( int idx, Vector vColor ) { if ( idx==1 ) m_vColor1=vColor; else m_vColor2=vColor; }
	const char		*GetExplosionParticleName( void );

private:

	CBaseHandle		m_Scorer;

	CNetworkVar( bool, m_bChargedShot );

	CNetworkVector( m_vColor1 );
	CNetworkVector( m_vColor2 );

protected:
	float			m_flInitTime;
};

#endif	//TF_PROJECTILE_ENERGY_BALL_H
