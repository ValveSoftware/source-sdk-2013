//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_SENTRYGUN_H
#define TF_OBJ_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"
#include "tf_projectile_rocket.h"

class CTFPlayer;

enum
{
	SENTRY_LEVEL_1 = 0,
	// SENTRY_LEVEL_1_UPGRADING,
	SENTRY_LEVEL_2,
	// SENTRY_LEVEL_2_UPGRADING,
	SENTRY_LEVEL_3,
};

#define SF_SENTRY_UPGRADEABLE	(LAST_SF_BASEOBJ<<1)
#define SF_SENTRY_INFINITE_AMMO (LAST_SF_BASEOBJ<<2)

#define SENTRYGUN_MAX_HEALTH	150

#define SENTRYGUN_MINI_MAX_HEALTH	100

#define SENTRY_MAX_RANGE 1100.0f		// magic numbers are evil, people. adding this #define to demystify the value. (MSB 5/14/09)
#define SENTRY_MAX_RANGE_SQRD 1210000.0f


// ------------------------------------------------------------------------ //
// Sentrygun object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectSentrygun : public CBaseObject
{
	DECLARE_CLASS( CObjectSentrygun, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectSentrygun();

	static CObjectSentrygun* Create(const Vector &vOrigin, const QAngle &vAngles);

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	Precache();
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void);
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );
	virtual void	SetModel( const char *pModel );

	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	SetStartBuildingModel( void );
	virtual void	StartPlacement( CTFPlayer *pPlayer );

	// Engineer hit me with a wrench
	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );
	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	virtual int		GetTracerAttachment( void );

	virtual bool	IsUpgrading( void ) const { return m_iState == SENTRY_STATE_UPGRADING; }

	virtual float	GetTimeSinceLastFired( void ) const		{ return m_timeSinceLastFired.GetElapsedTime();	}

	virtual const QAngle &GetTurretAngles( void ) const		{ return m_vecCurAngles; }

	virtual void	SetBuildingSize( void );

	void			ClearTarget( void ) { m_hEnemy = NULL; }
	CBaseEntity		*GetTarget( void ) { return m_hEnemy.Get(); }
	void			FireNextFrame( void ) { m_bFireNextFrame = true; }
	void			FireRocketNextFrame( void ) {m_bFireRocketNextFrame = true; }

	void			SetAutoAimTarget( CTFPlayer* pPlayer );

	int				GetFireAttachment( void );

	void            OnKilledEnemy(CBasePlayer* pVictim);
	
	virtual void	MakeMiniBuilding( CTFPlayer* pPlayer );
	virtual void	MakeCarriedObject( CTFPlayer *pCarrier );
	void			MakeScaledBuilding( CTFPlayer* pPlayer );

	float			GetPushMultiplier();

	void			EmitSentrySound( const char* soundname );
	void			EmitSentrySound( IRecipientFilter& filter, int iEntIndex, const char *soundname );

	CTFPlayer *		GetAssistingTeammate( float maxAssistDuration = -1.0f ) const;

	virtual int		GetBaseHealth( void ) { return SENTRYGUN_MAX_HEALTH; }

	void			SetShieldLevel( int nLevel, float flDuration ) { m_nShieldLevel = nLevel; m_flShieldFadeTime = gpGlobals->curtime + flDuration; }

	virtual int		GetMaxUpgradeLevel( void ) OVERRIDE;

	virtual int		GetMiniBuildingStartingHealth( void ) OVERRIDE { return SENTRYGUN_MINI_MAX_HEALTH; }

	void RemoveAllAmmo();

	virtual int		GetMaxHealthForCurrentLevel( void );
	virtual int		GetUpgradeMetalRequired();
	bool			IsScaledSentry() { return m_flScaledSentry != 1.0f; }

	virtual int	GetShieldLevel() { return m_nShieldLevel; }

	virtual bool	IsTruceValidForEnt( void ) const OVERRIDE { return true; }

private:
	Vector GetEnemyAimPosition( CBaseEntity* pEnemy ) const;

	// Main think
	void SentryThink( void );

	// If the players hit us with a wrench, should we upgrade
	bool CanBeUpgraded( CTFPlayer *pPlayer );
	void StartUpgrading( void );
	void FinishUpgrading( void );

	// Target acquisition
	bool FindTarget( void );
	bool ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd );
	bool ValidTargetBot( CBaseCombatCharacter *pBot, const Vector &vecStart, const Vector &vecEnd );

	void FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter, bool bNoSound=false );
	bool FInViewCone ( CBaseEntity *pEntity );
	int Range( CBaseEntity *pTarget );

	// Rotations
	void SentryRotate( void );
	bool MoveTurret( void );

	// Attack
	void Attack( void );
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	bool Fire( void );
	bool FireRocket( void );
	virtual void ModifyFireBulletsDamage( CTakeDamageInfo* dmgInfo );

	int GetBaseTurnRate( void );
	
	virtual void	MakeDisposableBuilding( CTFPlayer *pPlayer );

	CNetworkVar( int, m_iState );

	float m_flNextAttack;
	float m_flFireRate;
	IntervalTimer m_timeSinceLastFired;

	// Rotation
	int m_iRightBound;
	int m_iLeftBound;
	int	m_iBaseTurnRate;
	bool m_bTurningRight;

	QAngle m_vecCurAngles;
	QAngle m_vecGoalAngles;

	float m_flTurnRate;

	// Ammo
	CNetworkVar( int, m_iAmmoShells );
	CNetworkVar( int, m_iMaxAmmoShells );
	CNetworkVar( int, m_iAmmoRockets );
	CNetworkVar( int, m_iMaxAmmoRockets );
	int m_iOldAmmoShells;
	int m_iOldAmmoRockets;

	int	m_iAmmoType;

	float m_flNextRocketAttack;

	// Target player / object
	CNetworkHandle( CBaseEntity, m_hEnemy );
	bool m_bFireNextFrame;
	bool m_bFireRocketNextFrame;
	float m_flSentryRange;

	// Player control shield.
	CNetworkVar( bool, m_bPlayerControlled );
	CNetworkVar( uint32, m_nShieldLevel );
	float m_flShieldFadeTime;

	// PDQ Sentry
	CNetworkVar( bool, m_bPDQSentry );

	//cached attachment indeces
	int m_iAttachments[4];

	int m_iPitchPoseParameter;
	int m_iYawPoseParameter;

	float m_flLastAttackedTime;

	float m_flHeavyBulletResist;

	CNetworkHandle( CTFPlayer, m_hAutoAimTarget );
	float m_flAutoAimStartTime;

	// for achievement: ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_ABSORB_DMG
	int m_iLifetimeShieldedDamage;

	DECLARE_DATADESC();

	CountdownTimer m_inCombatThrottleTimer;
	void UpdateNavMeshCombatStatus( void );

	CHandle< CTFPlayer > m_lastTeammateWrenchHit;	// which teammate last hit us with a wrench
	IntervalTimer m_lastTeammateWrenchHitTimer;		// time since last wrench hit

	int m_iLastMuzzleAttachmentFired;

	float m_flScaledSentry;
};

// sentry rocket class just to give it a unique class name, so we can distinguish it from other rockets
class CTFProjectile_SentryRocket : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_SentryRocket, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFProjectile_SentryRocket();

	virtual int GetProjectileType() const OVERRIDE { return TF_PROJECTILE_SENTRY_ROCKET; }

	// Creation.
	static CTFProjectile_SentryRocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );	

	virtual void Spawn();
};

#endif // TF_OBJ_SENTRYGUN_H
