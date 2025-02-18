//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_MEDIGUN_H
#define TF_WEAPON_MEDIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CWeaponMedigun C_WeaponMedigun
#define CTFMedigunShield C_TFMedigunShield
#endif

class CTFMedigunShield;
class CTFReviveMarker;

enum medigun_weapontypes_t
{
	MEDIGUN_STANDARD = 0,
	MEDIGUN_UBER,
	MEDIGUN_QUICKFIX,
	MEDIGUN_RESIST,
};

enum medigun_resist_types_t
{
	MEDIGUN_BULLET_RESIST = 0,
	MEDIGUN_BLAST_RESIST,
	MEDIGUN_FIRE_RESIST,
	MEDIGUN_NUM_RESISTS
};

struct MedigunEffects_t
{
	ETFCond eCondition;
	ETFCond eWearingOffCondition;
	const char *pszChargeOnSound;
	const char *pszChargeOffSound;
};

extern MedigunEffects_t g_MedigunEffects[MEDIGUN_NUM_CHARGE_TYPES];

#define MAX_HEALING_TARGETS			1	//6

#define CLEAR_ALL_TARGETS			-1




//=========================================================
// Beam healing gun
//=========================================================
class CWeaponMedigun : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CWeaponMedigun, CTFWeaponBaseGun );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponMedigun( void );
	~CWeaponMedigun( void );

	virtual void	Precache();

	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	UpdateOnRemove( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	ItemPostFrame( void );
	virtual bool	Lower( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	void			DrainCharge( void );
	virtual void	WeaponReset( void );

	virtual float	GetTargetRange( void );
	virtual float	GetStickRange( void );
	virtual float	GetHealRate( void );
	virtual bool	AppliesModifier( void ) { return true; }

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MEDIGUN; }
	int				GetMedigunType( void ) const;


	bool			IsReleasingCharge( void ) const;
	medigun_charge_types GetChargeType( void ) const;

	void			CycleResistType();
	medigun_resist_types_t GetResistType() const;

	CBaseEntity		*GetHealTarget( void ) { return IsAttachedToBuilding() ? NULL : m_hHealingTarget.Get(); }

	bool			IsAllowedToTargetBuildings( void );
	bool			IsAttachedToBuilding( void );

#ifdef GAME_DLL
	// We may or may not be healing this person still. Resets on WeaponReset
	CBaseEntity		*GetMostRecentHealTarget( void ) { return m_hLastHealingTarget.Get(); }
	void			RecalcEffectOnTarget( CTFPlayer *pPlayer, bool bInstantRemove = false );
#endif

	float			GetReleaseStartedAt( void ) { return m_flReleaseStartedAt; }

#if defined( CLIENT_DLL )
	// Stop all sounds being output.
	void			StopHealSound( bool bStopHealingSound = true, bool bStopNoTargetSound = true, bool bStopDetachSound = true );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink();
	void			UpdateEffects( void );
	void			ForceHealingTargetUpdate( void ) { m_bUpdateHealingTargets = true; }

	void			StopChargeEffect( bool bImmediately );
	void			ManageChargeEffect( void );

	void			UpdateMedicAutoCallers( void );
#else

	void			HealTargetThink( void );
	void			AddCharge( float flPercentage );
	void			SubtractCharge( float flPercentage );
#endif

	void			SetChargeLevel( float flChargeLevel ) { m_flChargeLevel = flChargeLevel; }
	void			SetChargeLevelToPreserve( float flAmount );
	float			GetChargeLevel( void ) const { return m_flChargeLevel; }
	float			GetMinChargeAmount( void ) const;

	virtual void	OnControlStunned( void );

	virtual bool	IsHolstered(){ return m_bHolstered; }

	float			GetProgress( void );
	const char*		GetEffectLabelText( void ) { return "#TF_Rescue"; }
	bool			EffectMeterShouldFlash( void );
	float			GetOverHealBonus( CTFPlayer *pTFTarget );
	float			GetOverHealDecayMult( CTFPlayer *pTFTarget );
	virtual void	HookAttributes( void ) OVERRIDE;

private:
	void					SubtractChargeAndUpdateDeployState( float flSubtractAmount, bool bForceDrain );
	bool					FindAndHealTargets( void );
	void					MaintainTargetInSlot();
	void					FindNewTargetForSlot();
	void					RemoveHealingTarget( bool bStopHealingSelf = false );
	bool					HealingTarget( CBaseEntity *pTarget );
	bool					AllowedToHealTarget( CBaseEntity *pTarget );
	void					CheckAchievementsOnHealTarget( void );
	void					StartHealingTarget( CBaseEntity *pTarget );
	void					StopHealingOwner( void );

#ifdef CLIENT_DLL
	const char				*GetHealSound() const;
	const char				*GetDetachSound() const;
#else
	void					UberchargeChunkDeployed();
#endif

	void					CreateMedigunShield( void );
	void					RemoveMedigunShield( void );

public:


	CNetworkHandle( CBaseEntity, m_hHealingTarget );
	CNetworkHandle( CBaseEntity, m_hLastHealingTarget );

	bool					m_bWasHealingBeforeDeath;

protected:
	// Networked data.
	CNetworkVar( bool,		m_bHealing );
	CNetworkVar( bool,		m_bAttacking );

	double					m_flNextBuzzTime;
	float					m_flHealEffectLifetime;	// Count down until the healing effect goes off.
	float					m_flReleaseStartedAt;

	CNetworkVar( bool,		m_bHolstered );
	CNetworkVar( bool,		m_bChargeRelease );
	CNetworkVar( float,		m_flChargeLevel );
	CNetworkVar( int,		m_nChargeResistType );

	float					m_flNextTargetCheckTime;
	bool					m_bCanChangeTarget; // used to track the PrimaryAttack key being released for AutoHeal mode
	bool					m_bAttack2Down;
	bool					m_bAttack3Down;
	bool					m_bReloadDown;
	float					m_flEndResistCharge;
	
	struct targetdetachtimes_t
	{
		float	flTime;
		EHANDLE	hTarget;
	};
	CUtlVector<targetdetachtimes_t>		m_DetachedTargets; // Tracks times we last applied charge to a target. Used to drain faster for more targets.

#ifdef GAME_DLL
	CDamageModifier			m_DamageModifier;		// This attaches to whoever we're healing.
	bool					m_bHealingSelf;
	int						m_nHealTargetClass;
	int						m_nChargesReleased;
#endif
	float					m_flChargeLevelToPreserve;
	float					m_flOverHealExpert;		// Upgrade

	CHandle< CTFMedigunShield > m_hMedigunShield;
	CHandle< CTFReviveMarker > m_hReviveMarker;

#ifdef CLIENT_DLL
	bool					m_bPlayingSound;
	bool					m_bUpdateHealingTargets;
	struct healingtargeteffects_t
	{
		C_BaseEntity		*pOwner;
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
		CNewParticleEffect	*pCustomEffect;
	};
	healingtargeteffects_t m_hHealingTargetEffect;

	float					m_flDenySecondary;
	bool					m_bOldChargeRelease;
	int						m_nOldChargeResistType;

	C_BaseEntity		*m_pChargeEffectOwner;
	CNewParticleEffect	*m_pChargeEffect;
	CSoundPatch			*m_pChargedSound;
	CSoundPatch			*m_pDisruptSound;
	CSoundPatch			*m_pHealSound;
	CSoundPatch			*m_pDetachSound;

	CUtlVector< int >	m_iAutoCallers;
	float				m_flAutoCallerCheckTime;
#endif

private:														
	CWeaponMedigun( const CWeaponMedigun & );
};

class CTFMedigunShield : public CBaseAnimating
{
	DECLARE_CLASS( CTFMedigunShield, CBaseAnimating );

public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CTFMedigunShield();
	~CTFMedigunShield();

	virtual void Spawn();
	virtual void Precache();
	virtual bool IsCombatItem( void ) const { return true; }

	void UpdateShieldPosition( void );

#ifdef GAME_DLL
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	static CTFMedigunShield *Create( CTFPlayer *pOwner );
	// virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	virtual void StartTouch( CBaseEntity *pOther ) OVERRIDE;
	void ShieldTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther ) OVERRIDE;
	void ShieldThink( void );
	void RemoveShield( void );


#else
	virtual void ClientThink();
#endif

private:
	int m_nBlinkCount;
#ifdef GAME_DLL
	float m_flShieldEnergyLevel;
	CSoundPatch	*m_pTouchLoop;


#endif // GAME_DLL
};

#endif // TF_WEAPON_MEDIGUN_H
