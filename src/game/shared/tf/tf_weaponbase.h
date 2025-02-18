//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	Weapons.
//
//	CTFWeaponBase
//	|
//	|--> CTFWeaponBaseMelee
//	|		|
//	|		|--> CTFWeaponCrowbar
//	|		|--> CTFWeaponKnife
//	|		|--> CTFWeaponMedikit
//	|		|--> CTFWeaponWrench
//	|
//	|--> CTFWeaponBaseGrenade
//	|		|
//	|		|--> CTFWeapon
//	|		|--> CTFWeapon
//	|
//	|--> CTFWeaponBaseGun
//
//=============================================================================
#ifndef TF_WEAPONBASE_H
#define TF_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "tf_weapon_parse.h"
#include "npcevent.h"
#include "ihasowner.h"
#include "tf_item_wearable.h"

// Client specific.
#if defined( CLIENT_DLL )
#define CTFWeaponBase C_TFWeaponBase
#define CTFWeaponAttachmentModel C_TFWeaponAttachmentModel
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#include "tf_fx_muzzleflash.h"
#include "GameEventListener.h"
#endif // CLIENT_DLL

#ifdef GAME_DLL
#include "ihasgenericmeter.h"
#endif // GAME_DLL

#define MAX_TRACER_NAME		128

class CTFPlayer;
class CBaseObject;
class CTFWeaponBaseGrenadeProj;
class CTFWeaponAttachmentModel;

// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// TFTODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity );

// Reloading singly.
enum
{
	TF_RELOAD_START = 0,
	TF_RELOADING,
	TF_RELOADING_CONTINUE,
	TF_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

enum EWeaponStrangeType_t
{
	STRANGE_UNKNOWN = -1,
	STRANGE_NOT_STRANGE = 0,
	STRANGE_IS_STRANGE = 1,
};

enum EWeaponStatTrakModuleType_t
{
	MODULE_UNKNOWN = -1,
	MODULE_NONE = 0,
	MODULE_FOUND = 1,
};

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

// Interface for weapons that have a charge time
class ITFChargeUpWeapon 
{
public:
	virtual bool CanCharge( void ) = 0;

	virtual float GetChargeBeginTime( void ) = 0;

	virtual float GetChargeMaxTime( void ) = 0;

	virtual float GetCurrentCharge( void )
	{ 
		return ( gpGlobals->curtime - GetChargeBeginTime() ) / GetChargeMaxTime();
	}
};

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( ( pEntity->IsPlayer() || pEntity->IsCombatItem() ) && ( pEntity->GetTeamNumber() == m_iIgnoreTeam || m_iIgnoreTeam == TEAM_ANY ) )
		{
			return false;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

	int m_iIgnoreTeam;
};

class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity && pEntity->IsPlayer() )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

class CTraceFilterIgnoreFriendlyCombatItems : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreFriendlyCombatItems, CTraceFilterSimple );

	CTraceFilterIgnoreFriendlyCombatItems( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, bool bIsProjectile = false )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
		m_bCallerIsProjectile = bIsProjectile;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

// 		if ( ( pEntity->MyCombatCharacterPointer() || pEntity->MyCombatWeaponPointer() ) && pEntity->GetTeamNumber() == m_iIgnoreTeam )
// 			return false;
// 
// 		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
// 			return false;

		if ( pEntity->IsCombatItem() )
		{
			if ( pEntity->GetTeamNumber() == m_iIgnoreTeam )
				return false;

			// If source is a enemy projectile, be explicit, otherwise we fail a "IsTransparent" test downstream
			if ( m_bCallerIsProjectile )
				return true;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

	int m_iIgnoreTeam;
	bool m_bCallerIsProjectile;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTraceFilterCollisionArrows : public CTraceFilterEntitiesOnly
{
public:
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionArrows );

	CTraceFilterCollisionArrows( const IHandleEntity *passentity, const IHandleEntity *passentity2 )
		: m_pPassEnt( passentity ), m_pPassEnt2( passentity2 )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( pEntity == m_pPassEnt2 )
				return false;
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;

			return true;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	const IHandleEntity *m_pPassEnt2;
};

#define ENERGY_WEAPON_MAX_CHARGE		20

#define TF_PARTICLE_WEAPON_BLUE_1	Vector( 0.345, 0.52, 0.635 )
#define TF_PARTICLE_WEAPON_BLUE_2	Vector( 0.145, 0.427, 0.55 )
#define TF_PARTICLE_WEAPON_RED_1	Vector( 0.72, 0.22, 0.23 )
#define TF_PARTICLE_WEAPON_RED_2	Vector( 0.5, 0.18, 0.125 )

//=============================================================================
//
// Base TF Weapon Class
//
#if defined( CLIENT_DLL )
class CTFWeaponBase : public CBaseCombatWeapon, public IHasOwner, public IHasGenericMeter, public CGameEventListener
#else
class CTFWeaponBase : public CBaseCombatWeapon, public IHasOwner, public IHasGenericMeter
#endif
{
	DECLARE_CLASS( CTFWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined ( CLIENT_DLL )
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();
#endif

	// Setup.
	CTFWeaponBase();
	~CTFWeaponBase();

	virtual void Spawn();
	virtual void Activate( void );
	virtual void Precache();
	virtual bool IsPredicted() const			{ return true; }
	virtual void FallInit( void );

	// Weapon Data.
	CTFWeaponInfo const	&GetTFWpnData() const;
	virtual int GetWeaponID( void ) const;
	bool IsWeapon( int iWeapon ) const;
	virtual int	GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }
	virtual int	GetMaxClip1( void ) const;
	virtual int GetDefaultClip1( void ) const;
	virtual bool UsesPrimaryAmmo();
	virtual float UberChargeAmmoPerShot( void ) { float fAmmo = 0; CALL_ATTRIB_HOOK_FLOAT( fAmmo, ubercharge_ammo ); return fAmmo * 0.01f; }

	virtual int	Clip1() { return IsEnergyWeapon() ? Energy_GetEnergy() : m_iClip1; }
	virtual int	Clip2() { return m_iClip2; }

	virtual bool HasAmmo( void );

	// View model.
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual Activity ActivityOverride( Activity baseAct, bool *pRequired ) OVERRIDE;
	
	virtual poseparamtable_t* GetPlayerPoseParamList( int &iPoseParamCount );
	virtual poseparamtable_t* GetItemPoseParamList( int &iPoseParamCount );

	virtual bool SendWeaponAnim( int iActivity ) OVERRIDE;

	virtual CBaseEntity	*GetOwnerViaInterface( void ) { return GetOwner(); }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Drop( const Vector &vecVelocity );
	virtual void UpdateOnRemove( void );
	virtual bool CanHolster( void ) const;
	virtual void StartHolsterAnim( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool Deploy( void );
	virtual bool ForceWeaponSwitch() const OVERRIDE;
	virtual void Detach( void );
	virtual void OnActiveStateChanged( int iOldState );
	virtual bool VisibleInWeaponSelection( void );
	virtual void UpdateHands( void );

	void EnableAttack();
	void DisableAttack();
	void EnableJump();
	void DisableJump();
	void EnableDuck();
	void DisableDuck();

	virtual bool OwnerCanTaunt( void ) { return true; }
	virtual bool CanBeCritBoosted( void );
	bool CanHaveRevengeCrits( void );

	virtual const CEconItemView *GetTauntItem() const;

	// Extra wearables.
#ifdef GAME_DLL
	virtual void ChangeTeam( int iTeamNum ) OVERRIDE;	
	virtual void UpdateExtraWearables();
	virtual void ExtraWearableEquipped( CTFWearable *pExtraWearableItem );
	virtual void ExtraWearableViewModelEquipped( CTFWearable *pExtraWearableItem );
	virtual bool HideAttachmentsAndShowBodygroupsWhenPerformingWeaponIndependentTaunt() const { return true; }
#endif // GAME_DLL
	virtual void RemoveExtraWearables( void );

	// Attacks.
	virtual void Misfire( void );
	virtual void FireFullClipAtOnce( void );
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void CalcIsAttackCritical( void );
	virtual bool CalcIsAttackCriticalHelper();
	virtual bool CalcIsAttackCriticalHelperNoCrits();
	bool IsCurrentAttackACrit() const { return m_bCurrentAttackIsCrit; }
	bool IsCurrentAttackARandomCrit() const { return m_bCurrentAttackIsCrit && m_bCurrentCritIsRandom; }
	bool IsCurrentAttackDuringDemoCharge() const { return m_bCurrentAttackIsDuringDemoCharge; }
	virtual ETFDmgCustom GetPenetrateType() const;
	virtual void GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f );
	virtual QAngle GetSpreadAngles( void );
	float GetLastPrimaryAttackTime( void ) const { return m_flLastPrimaryAttackTime; }
	virtual bool CanPerformSecondaryAttack() const OVERRIDE;
	virtual bool IsFiring( void ) const { return false; }
	virtual bool AreRandomCritsEnabled( void );

	// Reloads.
	virtual bool Reload( void );
	virtual void AbortReload( void );
	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	void SendReloadEvents();
	virtual bool IsReloading() const;			// is the weapon reloading right now?
	virtual float GetReloadSpeedScale() const { return 1.f; }

	virtual bool AutoFiresFullClip( void ) const OVERRIDE;
	bool AutoFiresFullClipAllAtOnce( void ) const;
	bool CanOverload( void ) const;
	virtual bool CheckReloadMisfire( void ) { return false; }

	virtual bool CanDrop( void ) { return false; }
	virtual bool AllowTaunts( void ) { return true; }

	// Fire Rate
	virtual float ApplyFireDelay( float flDelay ) const;

	// Sound.
	bool PlayEmptySound();
	bool IsSilentKiller();

	// Activities.
	virtual void ItemBusyFrame( void );
	virtual void ItemPostFrame( void );
	virtual void ItemHolsterFrame( void );

	virtual void SetWeaponVisible( bool visible );

	virtual int GetActivityWeaponRole() const;
	virtual acttable_t *ActivityList( int &iActivityCount ) OVERRIDE;

	virtual Activity	TranslateViewmodelHandActivityInternal( Activity actBase );
	virtual int			GetViewModelWeaponRole() { return GetTFWpnData().m_iWeaponType; }

#ifdef GAME_DLL
	virtual void	AddAssociatedObject( CBaseObject *pObject ) { }
	virtual void	RemoveAssociatedObject( CBaseObject *pObject ) { }

	virtual void	ApplyOnHitAttributes( CBaseEntity *pVictimBaseEntity, CTFPlayer *pAttacker, const CTakeDamageInfo &info );
	virtual void	ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer );
	virtual void	ApplyOnInjuredAttributes( CTFPlayer *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info );		// when owner of this weapon is hit

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual bool	DeflectProjectiles();
	virtual bool	DeflectPlayer( CTFPlayer *pTarget, CTFPlayer *pOwner, Vector &vecForward );
	virtual bool	DeflectEntity( CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward );
	static void		SendObjectDeflectedEvent( CTFPlayer *pNewOwner, CTFPlayer *pPrevOwner, int iWeaponID, CBaseAnimating *pObject );
	static float	DeflectionForce( const Vector &size, float damage, float scale );
	virtual void	PlayDeflectionSound( bool bPlayer ) {}
	virtual float	GetDeflectionRadius() const { return 128.f; }

	virtual float	GetJarateTime() { return 0.f; }

	void			ApplyItemRegen( void );

	kill_eater_event_t GetKillEaterKillEventType() const;
#endif

	// Utility.
	CBasePlayer *GetPlayerOwner() const;
	CTFPlayer *GetTFPlayerOwner() const;

#ifdef CLIENT_DLL
	virtual bool	ShouldPlayClientReloadSound() { return false; }

	C_BaseEntity *GetWeaponForEffect();

	virtual const char* ModifyEventParticles( const char* token ) { return token; }

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) OVERRIDE;
#endif

	virtual bool	CanAttack();
	virtual int		GetCanAttackFlags() const { return TF_CAN_ATTACK_FLAG_NONE; }

	// Raising & Lowering for grenade throws
	bool			WeaponShouldBeLowered( void );
	virtual bool	Ready( void );
	virtual bool	Lower( void );

	virtual void	WeaponIdle( void );

	virtual void	WeaponReset( void );
	virtual void	WeaponRegenerate( void );

	// Muzzleflashes
	virtual const char *GetMuzzleFlashEffectName_3rd( void ) { return NULL; }
	virtual const char *GetMuzzleFlashEffectName_1st( void ) { return NULL; }
	virtual const char *GetMuzzleFlashModel( void );
	virtual float	GetMuzzleFlashModelLifetime( void );
	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual const char	*GetTracerType( void );

	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	// CEconEntity
	virtual const char *GetInventoryModel( void );
	virtual void		ReapplyProvision( void );
	virtual float		GetSpeedMod( void ) { return 1.f; };

	virtual bool		CanFireCriticalShot( bool bIsHeadshot = false, CBaseEntity *pTarget = NULL );
	virtual bool		CanFireRandomCriticalShot( float flCritChance );

	virtual char const	*GetShootSound( int iIndex ) const;
	void			UpdateHiddenParentBodygroup( bool bHide );

	virtual void		OnControlStunned( void );

	virtual bool		HideWhileStunned( void ) { return true; }

	virtual bool IsViewModelFlipped( void );

	virtual int			GetMaxHealthMod() { return 0; }

	virtual float		GetLastDeployTime( void ) { return m_flLastDeployTime; }

	bool				IsPassiveWeapon( void ) const;

	// Energy Weapons
	virtual bool		IsEnergyWeapon( void ) const { return false; }
	virtual bool		IsBlastImpactWeapon( void ) const { return false; }
	float				Energy_GetMaxEnergy( void ) const;
	float				Energy_GetEnergy( void ) const { return m_flEnergy; }
	void				Energy_SetEnergy( float flEnergy ) { m_flEnergy = flEnergy; }
	bool				Energy_FullyCharged( void ) const;
	bool				Energy_HasEnergy( void );
	void				Energy_DrainEnergy( void );
	void				Energy_DrainEnergy( float flDrain );
	bool				Energy_Recharge( void );
	virtual float		Energy_GetShotCost( void ) const { return 4.f; }
	virtual float		Energy_GetRechargeCost( void ) const { return 4.f; }

	virtual Vector		GetParticleColor( int iColor );

	virtual void		CheckReload( void );
	virtual void		FinishReload( void );

	virtual bool		HasLastShotCritical( void ) { return false; }

	virtual bool		UseServerRandomSeed( void ) const { return true; }

	virtual bool		IsBroken( void ) const { return false; }
	virtual void		SetBroken( bool bBroken ) {}

// Server specific.
#if !defined( CLIENT_DLL )

	// Spawning.
	virtual void CheckRespawn();
	virtual CBaseEntity* Respawn();
	void Materialize();
	void AttemptToMaterialize();

	// Death.
	void Die( void );
	void SetDieThink( bool bDie );

	// Disguise weapon.
	void DisguiseWeaponThink( void );

	// Ammo.
	virtual const Vector& GetBulletSpread();

	// Hit tracking for achievements
	// Track the number of kills we've had since we missed. Only works for bullet firing weapons right now.
	virtual void	OnBulletFire( int iEnemyPlayersHit );
	virtual void	OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info );
	virtual float	GetLastHitTime( void ) { return m_flLastHitTime; }

	virtual int		GetDropSkinOverride( void ) { return -1; }

	int				GetKillStreak () const { return m_iKillStreak; }
	void			SetKillStreak ( int value ) { m_iKillStreak = value; };

	float			GetClipScale () const { return m_flClipScale; }
	void			SetClipScale ( float flScale ) { m_flClipScale = flScale; }

	virtual float	GetInitialAfterburnDuration() const { return 0.f; }
	virtual float	GetAfterburnRateOnHit() const { return 0.f; }

// Client specific.
#else

	bool			IsFirstPersonView();
	bool			UsingViewModel();
	C_BaseAnimating *GetAppropriateWorldOrViewModel();

	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	UpdateVisibility( void ) OVERRIDE;

	virtual void	ProcessMuzzleFlashEvent( void );
	virtual void	DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	virtual int		InternalDrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo ) OVERRIDE;

	virtual bool	ShouldPredict();
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	void			UpdateModelIndex();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	Redraw( void );
	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	BobState_t		*GetBobState();
	virtual bool	AttachmentModelsShouldBeVisible( void ) OVERRIDE { return (m_iState == WEAPON_IS_ACTIVE) && !IsBeingRepurposedForTaunt(); }

	virtual bool ShouldEjectBrass() { return true; }

	bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	// ItemEffect Hud defaults
	virtual const char * GetEffectLabelText()	{ return ""; }
	virtual float GetProgress()					{ return 0; }

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel[2];

	bool			IsUsingOverrideModel() const { return m_iWorldModelIndex != m_iCachedModelIndex; }

#endif

	virtual int		GetSkin();

	static void UpdateWeaponBodyGroups( CTFPlayer* pPlayer, bool bHandleDeployedBodygroups );
	void SetIsBeingRepurposedForTaunt( bool bCanOverride )	{ m_bBeingRepurposedForTaunt = bCanOverride; }
	bool IsBeingRepurposedForTaunt() const { return m_bBeingRepurposedForTaunt; }

	int GetKillComboClass( void ) const { return m_nKillComboClass; }
	int GetKillComboCount( void ) const { return m_nKillComboCount; }
	void ClearKillComboCount( void ) { m_nKillComboCount = 0; }
	void AddKillCombo( int nClassKilled )
	{
		if ( m_nKillComboClass != nClassKilled )
		{
			m_nKillComboClass = nClassKilled;
			ClearKillComboCount();
		}

		m_nKillComboCount = Min( 3, m_nKillComboCount + 1 );
	}

	// Effect / Regeneration bar handling
	virtual float	GetEffectBarProgress( void );			// Get the current bar state (will return a value from 0.0 to 1.0)
	bool			HasEffectBarRegeneration( void ) { return InternalGetEffectBarRechargeTime() > 0; }	// Check the base, not modified by attribute, because attrib may have reduced it to 0.
	float			GetEffectBarRechargeTime( void ) { float flTime = InternalGetEffectBarRechargeTime(); CALL_ATTRIB_HOOK_FLOAT( flTime, effectbar_recharge_rate ); return flTime; }
	void			DecrementBarRegenTime( float flTime ) { m_flEffectBarRegenTime -= flTime; }

	bool			IsHonorBound( void ) const;

	virtual bool	CanPickupOtherWeapon() const { return true; }

	EWeaponStrangeType_t	GetStrangeType();
	bool					BHasStatTrakModule();
#ifdef CLIENT_DLL
	// StatTrak View Model Test
	void					UpdateAllViewmodelAddons( void );

	void					AddStatTrakModel( CEconItemView *pItem, int nStatTrakType, AccountID_t holderAcctId );
	void					RemoveViewmodelStatTrak( void );
	void					RemoveWorldmodelStatTrak( void );

	CHandle< CTFWeaponAttachmentModel > m_viewmodelStatTrakAddon;
	CHandle< CTFWeaponAttachmentModel > m_worldmodelStatTrakAddon;

	virtual const Vector&	GetViewmodelOffset() OVERRIDE;
#endif

	virtual bool ShouldRemoveInvisibilityOnPrimaryAttack() const { return true; }

protected:
	virtual int		GetEffectBarAmmo( void ) { return m_iPrimaryAmmoType; }
	virtual float	InternalGetEffectBarRechargeTime( void ) { return 0; }	// Time it takes for this regeneration bar to fully recharge from 0 to full.

	void			StartEffectBarRegen( void );						// Call this when you want your bar to start recharging (usually when you've deployed your action)
	void			EffectBarRegenFinished( void );
	void			CheckEffectBarRegen( void );

private:
	CNetworkVar(	float, m_flEffectBarRegenTime );	// The time Regen is scheduled to complete

protected:
#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );

	virtual void UpdateExtraWearablesVisibility();
#endif // CLIENT_DLL

	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime );
	bool ReloadSingly( void );
	void ReloadSinglyPostFrame( void );
	void IncrementAmmo( void );

	bool NeedsReloadForAmmo1( int iClipSize1 ) const;
	bool NeedsReloadForAmmo2( int iClipSize2 ) const;

protected:

	void			PlayUpgradedShootSound( const char *pszSound );

	int				m_iWeaponMode;
	CNetworkVar(	int,	m_iReloadMode );
	CNetworkVar( float, m_flReloadPriorNextFire );
	CTFWeaponInfo	*m_pWeaponInfo;
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;
	bool			m_bCurrentCritIsRandom;
	bool			m_bCurrentAttackIsDuringDemoCharge;

	EWeaponStrangeType_t			m_eStrangeType;
	EWeaponStatTrakModuleType_t		m_eStatTrakModuleType;

	CNetworkVar(	bool,	m_bLowered );

	int				m_iAltFireHint;

	int				m_iReloadStartClipAmount;

	float			m_flCritTime;
	CNetworkVar( float, m_flLastCritCheckTime );	// Deprecated
	int				m_iLastCritCheckFrame;
	int				m_iCurrentSeed;
	float			m_flLastRapidFireCritCheckTime;

	float			m_flLastDeployTime;

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar(	bool, m_bResetParity );

	int				m_iAmmoToAdd;
	float			m_flLastPrimaryAttackTime;

#ifdef CLIENT_DLL
	bool m_bOldResetParity;
	int m_iCachedModelIndex;
	int m_iEjectBrassAttachpoint;

#endif

	CNetworkVar(	bool,	m_bReloadedThroughAnimEvent );

	CNetworkVar( float, m_flEnergy );

public:
#ifdef GAME_DLL
	// Stores the number of kills we've made since we last shot & didn't hit a player.
	// Only hooked up to bullet firing right now, so you'll need to do plumbing if you want it for other weaponry.
	int				m_iConsecutiveKills;

	// Accuracy tracking
	float			m_flLastHitTime;
	int				m_iHitsInTime;
	int				m_iProjectilesFiredInTime;

	// Used to generate active-weapon-only regen
	float			m_flRegenTime;

	// for penetrating weapons with drain - only drain each victim once
	CHandle< CTFPlayer > m_hLastDrainVictim;
	CountdownTimer m_lastDrainVictimTimer;

	int				m_iKillStreak;
	float			m_flClipScale;
#endif
	CNetworkVar( int, m_iConsecutiveShots );

	CNetworkVar(	bool, m_bDisguiseWeapon );

	CNetworkVar(	float, m_flLastFireTime );

	CNetworkHandle( CTFWearable, m_hExtraWearable );
	CNetworkHandle( CTFWearable, m_hExtraWearableViewModel );

	CNetworkVar( float, m_flObservedCritChance );

	virtual bool CanInspect() const { return true; }
	void HandleInspect();
	
	virtual void HookAttributes( void ) {};
	virtual void OnUpgraded( void ) { HookAttributes(); }
	virtual float GetNextSecondaryAttackDelay( void ) OVERRIDE;

	enum TFWeaponInspectStage
	{
		INSPECT_INVALID = -1,
		INSPECT_START,
		INSPECT_IDLE,
		INSPECT_END,

		INSPECT_STAGE_COUNT
	};
	TFWeaponInspectStage GetInspectStage() const { return (TFWeaponInspectStage)m_nInspectStage.Get(); }
	float GetInspectAnimEndTime() const { return m_flInspectAnimEndTime; }

	virtual bool UsesCenterFireProjectile( void ) const OVERRIDE;

private:
	CTFWeaponBase( const CTFWeaponBase & );

	CNetworkVar( bool, m_bBeingRepurposedForTaunt );

	CNetworkVar( int,	m_nKillComboClass );
	CNetworkVar( int,	m_nKillComboCount );
	
	Activity GetInspectActivity( TFWeaponInspectStage inspectStage );
	bool IsInspectActivity( int iActivity );
	CNetworkVar( float, m_flInspectAnimEndTime );
	CNetworkVar( int, m_nInspectStage );
	bool m_bInspecting;

	friend class CTFDroppedWeapon;

#ifdef CLIENT_DLL
	bool m_bInitViewmodelOffset;
	Vector m_vecViewmodelOffset;
#endif // CLIENT_DLL
};

bool WeaponID_IsSniperRifle( int iWeaponID );
bool WeaponID_IsSniperRifleOrBow( int iWeaponID );

#define WEAPON_RANDOM_RANGE 10000

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// CTFWeaponAttachmentModel 
//-----------------------------------------------------------------------------
class CTFWeaponAttachmentModel : public CBaseAnimating, public IHasOwner
{
	DECLARE_CLASS( CTFWeaponAttachmentModel, CBaseAnimating );
public:
	CTFWeaponAttachmentModel() { m_bIsViewModelAttachment = false; m_hWeaponAssociatedWith = NULL; }

	virtual bool ShouldDraw( void );
	
	void				Init( CBaseEntity *pParent, CTFWeaponBase *pAssociatedWeapon, bool bIsViewModel );
	void				SetWeaponAssociatedWith( CTFWeaponBase *pWeapon ) { m_hWeaponAssociatedWith = pWeapon; }
	CBaseEntity*		GetWeaponAssociatedWith( void ) const { return m_hWeaponAssociatedWith.Get(); }

	bool BIsViewModelAttachment() { return m_bIsViewModelAttachment; }
	
	virtual CBaseEntity	*GetOwnerViaInterface( void ) OVERRIDE { return m_hWeaponAssociatedWith.Get() ? m_hWeaponAssociatedWith.Get()->GetOwner() : NULL; }
private:

	bool m_bIsViewModelAttachment;
	CHandle< CTFWeaponBase > m_hWeaponAssociatedWith;
};
#endif // CLIENT_DLL

#endif // TF_WEAPONBASE_H
