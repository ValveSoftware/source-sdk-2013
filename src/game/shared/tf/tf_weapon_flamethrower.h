//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_FLAMETHROWER_H
#define TF_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"
#include "tf_flame.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "particlemgr.h"
	#include "particle_util.h"
	#include "particles_simple.h"
	#include "c_tf_projectile_rocket.h"

	#define CTFFlameThrower C_TFFlameThrower
	#define CTFFlameRocket C_TFFlameRocket
	#define CTFWeaponFlameBall C_TFWeaponFlameBall
#else
	#include "tf_projectile_rocket.h"
	#include "baseentity.h"
	#include "iscorer.h"
#endif

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING,
	FT_STATE_SECONDARY,
};

enum FlameThrowerMode_t
{
	TF_FLAMETHROWER_MODE_NORMAL = 0,
	TF_FLAMETHROWER_MODE_PHLOG = 1,
	TF_FLAMETHROWER_MODE_GIANT = 2,
	TF_FLAMETHROWER_MODE_RAINBOW = 3
};

#define MAX_PARTICLE_EFFECT_NAME_LENGTH 128

//=========================================================
// Flamethrower Weapon
//=========================================================
#ifdef GAME_DLL
class CTFFlameThrower : public CTFWeaponBaseGun, public CGameEventListener
#else
class CTFFlameThrower : public CTFWeaponBaseGun
#endif // GAME_DLL
{
	DECLARE_CLASS( CTFFlameThrower, CTFWeaponBaseGun );
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlameThrower();
	~CTFFlameThrower();

	virtual void	Spawn( void ) OVERRIDE;
	virtual void	UpdateOnRemove( void ) OVERRIDE;

	virtual int		GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_FLAMETHROWER; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;
	virtual void	PrimaryAttack() OVERRIDE;
	virtual void	SecondaryAttack() OVERRIDE;
	virtual bool	Lower( void ) OVERRIDE;
	virtual void	WeaponReset( void ) OVERRIDE;
	virtual void	WeaponIdle( void ) OVERRIDE;

	virtual void	DestroySounds( void );
	virtual void	Precache( void ) OVERRIDE;

	bool			CanAirBlast() const;
	bool			CanAirBlastPushPlayer() const;
	bool			CanAirBlastDeflectProjectile() const;
	bool			CanAirBlastPutOutTeammate() const;

	void			FireAirBlast( int iAmmoPerShot );

	float			GetSpinUpTime( void ) const;
	virtual bool	IsFiring( void ) const OVERRIDE { return m_iWeaponState == FT_STATE_FIRING; }
	void			SetWeaponState( int nWeaponState );

	void			UseRage( void );

	Vector GetVisualMuzzlePos();
	Vector GetFlameOriginPos();

	void			IncrementFlameDamageCount( void );
	void			DecrementFlameDamageCount( void );
	void			IncrementActiveFlameCount( void );
	void			DecrementActiveFlameCount( void );
	void			ResetFlameHitCount( void );

	virtual int		GetBuffType() { int iBuffType = 0; CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type ); return iBuffType; }

	float			GetProgress( void );
	bool			IsRageFull( void ); // same as GetProgress() without the division by 100.0f
	const char*		GetEffectLabelText( void ) { return "#TF_PYRORAGE"; }
	bool			EffectMeterShouldFlash( void );

	virtual bool	Deploy( void ) OVERRIDE;

#if defined( CLIENT_DLL )

	virtual void	OnDataChanged(DataUpdateType_t updateType) OVERRIDE;
	virtual void	SetDormant( bool bDormant ) OVERRIDE;
	virtual int		GetWorldModelIndex( void ) OVERRIDE;

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	virtual void		RestartParticleEffect() OVERRIDE;
	virtual const char* FlameEffectName( bool bIsFirstPersonView );	
	virtual const char* FlameCritEffectName( bool bIsFirstPersonView );
	virtual const char* FullCritChargedEffectName( void );
	const char*			GetParticleEffectName( void );

	void ClientEffectsThink( void );

	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();
	void 			StopFullCritEffect();

	// burning sound when you hit a player/building
	void			StopHitSound();

	float			GetFlameHitRatio( void );

#else
	void			SetHitTarget( void );
	void			HitTargetThink( void );

	virtual float	GetDeflectionRadius() const OVERRIDE;
	virtual bool	DeflectPlayer( CTFPlayer *pTarget, CTFPlayer *pOwner, Vector &vecForward ) OVERRIDE;
	virtual bool	DeflectEntity( CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward ) OVERRIDE;
	virtual void	PlayDeflectionSound( bool bPlayer ) OVERRIDE;

	virtual float	GetInitialAfterburnDuration() const OVERRIDE;
	virtual float	GetAfterburnRateOnHit() const OVERRIDE;

#endif

	virtual void	FireGameEvent( IGameEvent *event ) OVERRIDE;

	void CalculateHalloweenSpell( void );
	int GetFlameThrowerMode( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; }
	bool IsCritFire( void ) { return m_bCritFire; }

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	const char *GetNewFlameEffectInternal( int nTeam, bool bCrit );
#if defined( GAME_DLL )
	void ComputeCrayAirBlastForce( CTFPlayer *pTarget, CTFPlayer *pPlayer, Vector &vecForward, Vector &vecOutForce );
#endif

	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( bool, m_bCritFire );
	CNetworkVar( bool, m_bHitTarget );
	CNetworkVar( int, m_iActiveFlames );		// Number of flames we have in the world
	CNetworkVar( int, m_iDamagingFlames );		// Number of flames that have done damage
	CNetworkVar( float, m_flChargeBeginTime );
	CNetworkVar( float, m_flSpinupBeginTime );
	CNetworkHandle( CTFFlameManager, m_hFlameManager );
	CNetworkVar( bool, m_bHasHalloweenSpell );

	float m_flStartFiringTime;
	float m_flNextPrimaryAttackAnim;
	float m_flSecondaryAnimTime;
	float m_flMinPrimaryAttackBurstTime;

	int			m_iParticleWaterLevel;
	float		m_flAmmoUseRemainder;
	float		m_flResetBurstEffect;
	bool		m_bFiredSecondary;
	bool		m_bFiredBothAttacks;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pFiringStartSound;
	CSoundPatch	*m_pFiringLoop;
	CSoundPatch	*m_pFiringAccuracyLoop;
	CSoundPatch	*m_pFiringHitLoop;
	bool		m_bFiringLoopCritical;
	bool		m_bFiringHitTarget;
	CSoundPatch *m_pPilotLightSound;
	CSoundPatch *m_pSpinUpSound;
	float		m_flFlameHitRatio;
	float		m_flPrevFlameHitRatio;
	const char *m_szAccuracySound;
	bool m_bEffectsThinking;
	bool m_bFullRageEffect;

	class FlameEffect_t
	{
	public:
		FlameEffect_t( CTFFlameThrower* pFlamethrower )
			: m_pFlameEffect( NULL )
			, m_pFlamethrower( pFlamethrower )
			, m_pOwner( NULL )
			, m_hEffectWeapon( NULL )
		{}

		void StartEffects( CTFPlayer *pTFOwner, const char *pszEffectName );
		bool StopEffects();
		CNewParticleEffect *GetEffect() { return m_pFlameEffect; }

	private:
		CNewParticleEffect*	m_pFlameEffect;
		CTFFlameThrower*	m_pFlamethrower;
		CTFPlayer*			m_pOwner;
		EHANDLE				m_hEffectWeapon;
	};

	FlameEffect_t m_FlameEffects;
	FlameEffect_t m_MmmmphEffect;
#else
	float		m_flTimeToStopHitSound;
#endif

	// used for the new style flames (via CTFFlameManager)
	char m_szParticleEffectBlue[MAX_PARTICLE_EFFECT_NAME_LENGTH];
	char m_szParticleEffectRed[MAX_PARTICLE_EFFECT_NAME_LENGTH];
	char m_szParticleEffectBlueCrit[MAX_PARTICLE_EFFECT_NAME_LENGTH];
	char m_szParticleEffectRedCrit[MAX_PARTICLE_EFFECT_NAME_LENGTH];

	CTFFlameThrower( const CTFFlameThrower & );
};

//-----------------------------------------------------------------------------
// Flame rocket.  Not used in TF2, but is a part of data tables in demos and 
// must live on forever.
//-----------------------------------------------------------------------------
class CTFFlameRocket : public CTFBaseRocket
{
	DECLARE_CLASS( CTFFlameRocket, CTFBaseRocket );
public:
	DECLARE_NETWORKCLASS(); 
};

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( ITFFlameEntityAutoList );
class CTFFlameEntity : public CBaseEntity, public ITFFlameEntityAutoList
{
	DECLARE_CLASS( CTFFlameEntity, CBaseEntity );
public:

	CTFFlameEntity();
	virtual void Spawn( void );

public:
	static CTFFlameEntity *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, float flSpeed, int iDmgType, float m_flDmgAmount, bool bAlwaysCritFromBehind, bool bRandomize=true );

	void FlameThink( void );
	void SetCritFromBehind( bool bState ) { m_bCritFromBehind = bState; }
	bool IsEntityAttacker( CBaseEntity *pEnt ) { return m_hAttacker.Get() == pEnt; }

private:
	void RemoveFlame();
	void OnCollide( CBaseEntity *pOther );
	void OnCollideWithTeammate( CTFPlayer *pPlayer );
	void SetHitTarget( void );
	bool IsBehindTarget( CBaseEntity *pTarget );
	float DotProductToTarget( CBaseEntity *pTarget );
	void UpdateFlameThrowerHitRatio( void );
	float GetFlameFloat( void );
	float GetFlameDrag( void );

	Vector					m_vecInitialPos;		// position the flame was fired from
	Vector					m_vecPrevPos;			// position from previous frame
	Vector					m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector					m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	float					m_flTimeRemove;			// time at which the flame should be removed
	int						m_iDmgType;				// damage type
	float					m_flDmgAmount;			// amount of base damage
	CUtlVector<EHANDLE>		m_hEntitiesBurnt;		// list of entities this flame has burnt
	EHANDLE					m_hAttacker;			// attacking player
	bool					m_bCritFromBehind;		// Always crits from behind.
	bool					m_bBurnedEnemy;			// We track hitting to calculate hit/miss ratio in the Flamethrower

	CHandle< CTFFlameThrower > m_hFlameThrower;
};
#endif // GAME_DLL

class CTraceFilterIgnoreObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreObjects, CTraceFilterSimple );

	CTraceFilterIgnoreObjects( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity && pEntity->IsBaseObject() )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

#endif // TF_WEAPON_FLAMETHROWER_H
