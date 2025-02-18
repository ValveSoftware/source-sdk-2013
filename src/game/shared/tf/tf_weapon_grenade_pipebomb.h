//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_PIPEBOMB_H
#define TF_WEAPON_GRENADE_PIPEBOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadePipebombProjectile C_TFGrenadePipebombProjectile
#endif

//-----------------------------------------------------------------------------
// Grenade Launcher mode (for pipebombs).
//-----------------------------------------------------------------------------
enum
{
	TF_GL_MODE_REGULAR = 0,
	TF_GL_MODE_REMOTE_DETONATE,
	TF_GL_MODE_REMOTE_DETONATE_PRACTICE,
	TF_GL_MODE_CANNONBALL,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//=============================================================================
//
// TF Pipebomb Grenade
//
class CTFGrenadePipebombProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadePipebombProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();

	CTFGrenadePipebombProjectile();
	~CTFGrenadePipebombProjectile();

	// Unique identifier.
	virtual int			GetWeaponID( void ) const OVERRIDE;

	int GetType( void ) const { return m_iType; } 
	virtual int			GetDamageType();
	bool				HasStickyEffects() const { return m_iType == TF_GL_MODE_REMOTE_DETONATE || m_iType == TF_GL_MODE_REMOTE_DETONATE_PRACTICE; }

	bool				ShouldMiniCritOnReflect() const;

	void				SetChargeTime( float flChargeTime )				{ m_flChargeTime = flChargeTime; }

	CNetworkVar( bool, m_bTouched );
	CNetworkVar( int, m_iType ); // TF_GL_MODE_REGULAR or TF_GL_MODE_REMOTE_DETONATE
	float		m_flCreationTime;
	float		m_flChargeTime;
	bool		m_bPulsed;
	float		m_flFullDamage;

	void SetFullDamage( float flFullDamage ) { m_flFullDamage = flFullDamage; }

	CNetworkVar( bool, m_bDefensiveBomb );

	virtual void	UpdateOnRemove( void );

	virtual void	SetCustomPipebombModel() {}

	virtual float	GetLiveTime( void );
	virtual float	GetDamageRadius() OVERRIDE;

	void			SetDetonateOnPulse( bool bDet ) { m_bDetonateOnPulse = bDet; }

#ifdef CLIENT_DLL

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual const char *GetTrailParticleName( void );
	virtual int DrawModel( int flags );
	virtual void	Simulate( void );
	virtual void	CreateTrailParticles( void );

	void			SetHighlight( bool bHighlight ) { if ( m_bPulsed ) m_bHighlight = bHighlight; }
	bool			IsHighlighted( void ) { return m_bHighlight; }

	int		m_iCachedDeflect;
	CNewParticleEffect	*pEffectTrail;
	CNewParticleEffect	*pEffectCrit;
	bool		m_bHighlight;
	bool		m_bDetonateOnPulse;

	CGlowObject			*m_pGlowEffect;

#else

	DECLARE_DATADESC();

	// Creation.
	static CTFGrenadePipebombProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                         const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iPipeBombType, float flMultDmg );

	static const char* GetPipebombClass( int iPipeBombType );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	Fizzle();
	virtual bool	DetonateStickies( void );
	bool			CanTakeDamage() const { return m_bCanTakeDamage; }
	void			SetCanTakeDamage( bool bCanTakeDamage ) { m_bCanTakeDamage = bCanTakeDamage; }

	virtual void	SetPipebombMode( int iPipebombMode = TF_GL_MODE_REGULAR );
	bool			IsFizzle() { return m_bFizzle; }

	virtual void	PipebombTouch( CBaseEntity *pOther );
	virtual void	StickybombTouch( CBaseEntity *pOther );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info ) OVERRIDE;
	virtual void	IncrementDeflected( void );
	virtual void	DetonateThink( void );
	virtual void	PreArmThink( void );
	virtual void	ArmThink( void );

	void			CreatePipebombGibs( void );

	virtual bool	IsDeflectable( void ) OVERRIDE { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector& vecDir );

	virtual int		GetDamageCustom();
	float			GetTouchedTime()	{ return m_flTouchedTime; }
	bool			IsTouched()			{ return m_bTouched; }

	virtual int		UpdateTransmitState() OVERRIDE;
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
public:

	bool		m_bFizzle;
	bool		m_bWallShatter;
private:
	
	float		m_flMinSleepTime;
	float		m_flDeflectedTime;
	bool		m_bSendPlayerDestroyedEvent;
	bool		m_bDetonateOnPulse;
	bool		m_bCanTakeDamage;
	float		m_flTouchedTime;
	float		GetDamageScaleOnWorldContact();

	CUtlVector < CHandle <CTFPlayer> > m_CritMedics;
	CUtlVector < CHandle <CBaseEntity> > m_penetratedEntities;
#endif
};

#endif // TF_WEAPON_GRENADE_PIPEBOMB_H
