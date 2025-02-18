//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#include "SpriteTrail.h"
#include "tf_condition.h"

// Client specific.
#ifdef CLIENT_DLL
#include "baseobject_shared.h"
#include "c_tf_weapon_builder.h"
class C_TFPlayer;
// Server specific.
#else
#include "entity_currencypack.h"
#include "tf_weapon_builder.h"
class CTFPlayer;
#endif


//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif

enum
{
	MELEE_NOCRIT = 0,
	MELEE_MINICRIT = 1,
	MELEE_CRIT = 2,
};

struct stun_struct_t
{
	CHandle<CTFPlayer> hPlayer;
	float flDuration;
	float flExpireTime;
	float flStartFadeTime;
	float flStunAmount;
	int	iStunFlags;
};

//=============================================================================

#define PERMANENT_CONDITION		-1

#define MOVEMENTSTUN_PARITY_BITS	2

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
#ifdef CLIENT_DLL
	// This redundantly declares friendship which leads to gcc warnings.
	//DECLARE_CLIENTCLASS_NOBASE();
#else
public:
	// This redundantly declares friendship which leads to gcc warnings.
	//DECLARE_SERVERCLASS_NOBASE();
#endif
	DECLARE_EMBEDDED_NETWORKVAR();

public:
	float	flDamage;
	float	flDamageCritScaleMultiplier;		// scale the damage by this amount when taking it into consideration for "should I crit?" calculations
	float	flTime;
	int		nDamageType;
	byte	nKills;
};

#ifdef CLIENT_DLL
struct WheelEffect_t
{
	WheelEffect_t( float flTriggerSpeed, const char *pszRedParticleName, const char *pszBlueParticleName )
		: m_flMinTriggerSpeed( flTriggerSpeed )
	{
		memset( m_pszParticleName, NULL, sizeof( m_pszParticleName ) );
		m_pszParticleName[ TF_TEAM_RED ] = pszRedParticleName;
		m_pszParticleName[ TF_TEAM_BLUE ] = pszBlueParticleName;
	}

	float m_flMinTriggerSpeed;
	const char *m_pszParticleName[ TF_TEAM_COUNT ];
};
#endif

//=============================================================================
// Scoring data for the local player
struct RoundStats_t;
struct localplayerscoring_t
{
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( localplayerscoring_t );

	localplayerscoring_t()
	{
		Reset();
	}

	void Reset( void )
	{
		m_iCaptures = 0;
		m_iDefenses = 0;
		m_iKills = 0;
		m_iDeaths = 0;
		m_iSuicides = 0;
		m_iKillAssists = 0;
		m_iBuildingsBuilt = 0;
		m_iBuildingsDestroyed = 0;
		m_iHeadshots = 0;
		m_iDominations = 0;
		m_iRevenge = 0;
		m_iInvulns = 0;
		m_iTeleports = 0;
		m_iDamageDone = 0;
		m_iCrits = 0;
		m_iBackstabs = 0;
		m_iHealPoints = 0;
		m_iResupplyPoints = 0;
		m_iBonusPoints = 0;
		m_iPoints = 0;
	}

	void UpdateStats( RoundStats_t& roundStats, CTFPlayer *pPlayer, bool bIsRoundData );

	CNetworkVar( int, m_iCaptures );
	CNetworkVar( int, m_iDefenses );
	CNetworkVar( int, m_iKills );
	CNetworkVar( int, m_iDeaths );
	CNetworkVar( int, m_iSuicides );
	CNetworkVar( int, m_iDominations );
	CNetworkVar( int, m_iRevenge );
	CNetworkVar( int, m_iBuildingsBuilt );
	CNetworkVar( int, m_iBuildingsDestroyed );
	CNetworkVar( int, m_iHeadshots );
	CNetworkVar( int, m_iBackstabs );
	CNetworkVar( int, m_iHealPoints );
	CNetworkVar( int, m_iInvulns );
	CNetworkVar( int, m_iTeleports );
	CNetworkVar( int, m_iDamageDone );
	CNetworkVar( int, m_iCrits );
	CNetworkVar( int, m_iResupplyPoints );
	CNetworkVar( int, m_iKillAssists );
	CNetworkVar( int, m_iBonusPoints );
	CNetworkVar( int, m_iPoints );
};


// Condition Provider
struct condition_source_t
{
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( condition_source_t );

	condition_source_t()
	{
		m_nPreventedDamageFromCondition = 0;
		m_flExpireTime = 0.f;
		m_pProvider = NULL;
		m_bPrevActive = false;
	}

	int	m_nPreventedDamageFromCondition;
	float	m_flExpireTime;
	CNetworkHandle( CBaseEntity, m_pProvider );
	bool	m_bPrevActive;
};


//=============================================================================
// For checkpointing upgrades Players have purchased in Mann Vs Machine
class CUpgradeInfo
{
public:
	int m_iPlayerClass;							// the character class this upgrade is being applied too
	item_definition_index_t m_itemDefIndex;		// item that was upgraded (or INVALID_ITEM_DEF_INDEX for the player itself)
	int m_upgrade;								// the upgrade that was applied
	int m_nCost;								// price of the upgrade
};

#define CONTROL_STUN_ANIM_TIME	1.5f

enum TFStunAnimState_t
{
	STUN_ANIM_NONE = 0,
	STUN_ANIM_LOOP,
	STUN_ANIM_END
};

enum TFPassTimeThrowAnimState_t
{
	PASSTIME_THROW_ANIM_NONE = 0,
	PASSTIME_THROW_ANIM_LOOP,
	PASSTIME_THROW_ANIM_END
};

enum TFCYOAPDAAnimState_t
{
	CYOA_PDA_ANIM_NONE = 0,
	CYOA_PDA_ANIM_IDLE,
	CYOA_PDA_ANIM_OUTRO
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared : public CGameEventListener
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

private:

	struct RageBuff
	{
		int m_iBuffTypeActive;
		int m_iBuffPulseCount;
		float m_flNextBuffPulseTime;
	};

	// Condition Provider tracking
	CUtlVector< condition_source_t > m_ConditionData;

public:
	enum ERageBuffSlot
	{
		kBuffSlot_Rage,
		kBuffSlot_Decapitation,
		kBuffSlot_MAX,
	};

	enum ETFStreak
	{
		kTFStreak_Kills				= 0,
		kTFStreak_KillsAll			= 1,	// Counts all kills not just attr based killstreak.  For Data collection purposes
		kTFStreak_Ducks				= 2,
		kTFStreak_Duck_levelup		= 3, 
		kTFStreak_COUNT				= 4,
	};

	enum EKartStateFlags
	{
		kKartState_Driving			= 1 << 0,
		kKartState_Braking			= 1 << 1,
		kKartState_Reversing		= 1 << 2,
		kKartState_Stopped			= 1 << 3,
		kKartState_SteerLeft		= 1 << 4,
		kKartState_SteerRight		= 1 << 5
	};

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );
	void Spawn( void );

	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	void	SharedThink( void );

	// Condition (TF_COND_*).
	void	AddCond( ETFCond eCond, float flDuration = PERMANENT_CONDITION, CBaseEntity *pProvider = NULL );
	void	RemoveCond( ETFCond eCond, bool ignore_duration=false );
	bool	InCond( ETFCond eCond ) const;
	bool	WasInCond( ETFCond eCond ) const;
	void	ForceRecondNextSync( ETFCond eCond );
	void	RemoveAllCond();
	void	OnConditionAdded( ETFCond eCond );
	void	OnConditionRemoved( ETFCond eCond );
	void	ConditionThink( void );
	float	GetConditionDuration( ETFCond eCond ) const;
	void	SetConditionDuration( ETFCond eCond, float flNewDur )
	{
		Assert( eCond < m_ConditionData.Count() );
		m_ConditionData[eCond].m_flExpireTime = flNewDur;
	}

	CBaseEntity *GetConditionProvider( ETFCond eCond ) const;
	CBaseEntity *GetConditionAssistFromVictim( void );
	CBaseEntity *GetConditionAssistFromAttacker( void );

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	void	CheckDisguiseTimer( void );

	int		GetMaxBuffedHealth( bool bIgnoreAttributes = false, bool bIgnoreHealthOverMax = false );
#ifdef GAME_DLL
	float	GetMaxOverhealMultiplier( void );
#endif 

	bool	IsAiming( void );

	bool	ShouldSuppressPrediction( void );

	void SetCarryingRuneType( RuneTypes_t rt );
	RuneTypes_t GetCarryingRuneType( void ) const;
	bool IsCarryingRune( void ) const { return GetCarryingRuneType() != RUNE_NONE; }
	float		m_flRuneAcquireTime;

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	SyncConditions( int nPreviousConditions, int nNewConditions, int nForceConditionBits, int nBaseCondBit );

	enum ECritBoostUpdateType { kCritBoost_Ignore, kCritBoost_ForceRefresh };
	void	UpdateCritBoostEffect( ECritBoostUpdateType eUpdateType = kCritBoost_Ignore );

	bool	ShouldShowRecentlyTeleported( void );
	void	EndRadiusHealEffect( void );
	void	EndKingBuffRadiusEffect( void );
#endif

	bool	IsCritBoosted( void ) const;
	bool	IsInvulnerable( void ) const;
	bool	IsStealthed( void ) const;
	bool	CanBeDebuffed( void ) const;
	bool	IsImmuneToPushback( void ) const;

	void	Disguise( int nTeam, int nClass, CTFPlayer* pDesiredTarget=NULL, bool bOnKill = false );
	void	CompleteDisguise( void );
	void	RemoveDisguise( void );
	void	RemoveDisguiseWeapon( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void ) const;
	int		GetDisguiseClass( void ) const			{ return InCond( TF_COND_DISGUISED_AS_DISPENSER ) ? (int)TF_CLASS_ENGINEER : m_nDisguiseClass; }
	int		GetDisguisedSkinOverride( void ) const	{ return m_nDisguiseSkinOverride; }
	int		GetDisguiseMask( void )	const			{ return InCond( TF_COND_DISGUISED_AS_DISPENSER ) ? (int)TF_CLASS_ENGINEER : m_nMaskClass; }
	int		GetDesiredDisguiseClass( void )	const	{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void ) const	{ return m_nDesiredDisguiseTeam; }
	bool	WasLastDisguiseAsOwnTeam( void ) const	{ return m_bLastDisguisedAsOwnTeam; }
	// Josh: Hack for not including c_tf_player.h in replay code
	// as it causes a bunch of issues with redefines.
#ifndef REPLAY_SOURCE_FILE
	CTFPlayer *GetDisguiseTarget( void ) const			{ return m_hDisguiseTarget; }
#endif
	CTFWeaponBase *GetDisguiseWeapon( void ) const		{ return m_hDisguiseWeapon; }
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );
	int		GetDisguiseMaxHealth( void );
	int		GetDisguiseMaxBuffedHealth( bool bIgnoreAttributes = false, bool bIgnoreHealthOverMax = false );
	void	ProcessDisguiseImpulse( CTFPlayer *pPlayer );
	int		GetDisguiseAmmoCount( void ) { return m_iDisguiseAmmo; }
	void	SetDisguiseAmmoCount( int nValue ) { m_iDisguiseAmmo = nValue; }

	bool	CanRecieveMedigunChargeEffect( medigun_charge_types eType ) const;
#ifdef CLIENT_DLL
	int		GetDisplayedTeam( void ) const;
	void	OnDisguiseChanged( void );

	float	GetTimeTeleEffectAdded( void ) { return m_flGotTeleEffectAt; }
	int		GetTeamTeleporterUsed( void ) { return m_nTeamTeleporterUsed; }

	void	ClientDemoBuffThink( void );
	void	ClientKillStreakBuffThink( void );
#endif

	int		CalculateObjectCost( CTFPlayer* pBuilder, int iObjectType );

	// Pickup effects, including putting out fires, updating HUD, etc.
	void	HealthKitPickupEffects( int iHealthGiven = 0 );

#ifdef GAME_DLL
	void	DetermineDisguiseWeapon( bool bForcePrimary = false );
	void	DetermineDisguiseWearables();
	void	RemoveDisguiseWearables();
	void	Heal( CBaseEntity *pHealer, float flAmount, float flOverhealBonus, float flOverhealDecayMult, bool bDispenserHeal = false, CTFPlayer *pHealScorer = NULL );
	float	StopHealing( CBaseEntity *pHealer );
	void	TestAndExpireChargeEffect( medigun_charge_types iCharge );
	void	RecalculateChargeEffects( bool bInstantRemove = false );
	int		FindHealerIndex( CBaseEntity *pPlayer );
	EHANDLE	GetFirstHealer();
	void	CheckForAchievement( int iAchievement );

	void	IncrementArenaNumChanges( void ) { m_nArenaNumChanges++; }
	void	ResetArenaNumChanges( void ) { m_nArenaNumChanges = 0; }

	bool	AddToSpyCloakMeter( float val, bool bForce=false );

	void	AddTmpDamageBonus( float flBonus, float flExpiration );
	float	GetTmpDamageBonus( void ) { return (InCond(TF_COND_TMPDAMAGEBONUS)) ? m_flTmpDamageBonusAmount : 1.0; }

	void	SetTeamTeleporterUsed( int nTeam ){ m_nTeamTeleporterUsed.Set( nTeam ); }

	void	SetBiteEffectWasApplied() { m_bBiteEffectWasApplied = true; }

#endif	// GAME_DLL

	int		GetArenaNumChanges( void ) { return m_nArenaNumChanges; }

	CBaseEntity *GetHealerByIndex( int index );
	bool HealerIsDispenser( int index );
	int		GetNumHealers( void ) { return m_nNumHealers; }

	void	Burn( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, float flBurningTime = -1.0f );
	void	SelfBurn( float flBurningTime );		// Boss Burn
	void    MakeBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, float flBleedingTime, int nBleedDmg = TF_BLEEDING_DMG, bool bPermanentBleeding = false, int nDmgType = TF_DMG_CUSTOM_BLEEDING );
#ifdef GAME_DLL
	void	StopBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );
#endif // GAME_DLL

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;

	// Utility.
	bool	IsAlly( CBaseEntity *pEntity );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float fAdditionalRateScale );
	float	GetPercentInvisible( void ) const;
	float	GetPercentInvisiblePrevious( void ) { return m_flPrevInvisibility; }
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }
	void	SetNextStealthTime( float flTime ) { m_flStealthNextChangeTime = flTime; }
	bool	IsFullyInvisible( void ) { return ( GetPercentInvisible() == 1.f ); }

	bool	IsEnteringOrExitingFullyInvisible( void );

	bool	CanRuneCharge() const;
	float	GetRuneCharge() const { return m_flRuneCharge; }
	void	SetRuneCharge( float flVal ) { m_flRuneCharge = Clamp( flVal, 0.f, 100.f ); }
	bool	IsRuneCharged() const { return m_flRuneCharge == 100.f; }

	bool	IsRocketPackReady( void ) { return GetItemChargeMeter( LOADOUT_POSITION_SECONDARY ) >= 50.f; }
	float	GetRocketPackCharge( void ) { return GetItemChargeMeter( LOADOUT_POSITION_SECONDARY ); }
	void	SetRocketPackCharge( float flValue ) { SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, flValue ); }

	// generic meter per wpn slot
	// lets assume the value goes from 0.f->100.f for now
	void	UpdateItemChargeMeters();
	float	GetItemChargeMeter( loadout_positions_t slot ) const { return m_flItemChargeMeter[ slot ]; }
	float	GetItemChargeMeterPrev( loadout_positions_t slot ) const { return m_flPrevItemChargeMeter[ slot ]; }
	void	SetItemChargeMeter( loadout_positions_t slot, float flValue );

	bool	CanFallStomp( void );


	int		GetDesiredPlayerClassIndex( void );
	bool	IsInUpgradeZone( void ) { return m_bInUpgradeZone; }
	void	SetInUpgradeZone( bool bInZone ) { m_bInUpgradeZone = bInZone; }

	// Cloak, rage, phase, team juice...this should really all be done with composition?
	void	UpdateCloakMeter( void );
	float	GetSpyCloakMeter() const		{ return m_flCloakMeter; }
	void	SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }

	void	UpdateRageBuffsAndRage();
	bool	IsRageDraining() const			{ return m_bRageDraining; }
	float	GetRageMeter() const		{ return m_flRageMeter; }
	void	SetRageMeter( float val );
	void	ModifyRage( float fDelta );
	void	ResetRageMeter( void );
	void	PulseRageBuff( ERageBuffSlot eBuffSlot );
	void	StartRageDrain( void ) { m_bRageDraining = true; }
	void	ResetRageSystem( void );

	void	ActivateRageBuff( CBaseEntity *pBuffItem, int iBuffType );
	void	SetupRageBuffTimer( int iBuffType, int iPulseCount, ERageBuffSlot eBuffSlot );
	void	ResetRageBuffs( void );

	void	UpdatePhaseEffects( void );
	void	AddPhaseEffects( void );
	void	RemovePhaseEffects( void );

	void	PulseMedicRadiusHeal( void );

	float	GetScoutEnergyDrinkMeter() const{ return m_flEnergyDrinkMeter; }
	void	SetScoutEnergyDrinkMeter( float val ) { m_flEnergyDrinkMeter = val; }
	void	UpdateEnergyDrinkMeter( void );

	float	GetScoutHypeMeter() const		{ return m_flHypeMeter; }
	void	SetScoutHypeMeter( float val );
	void	StopScoutHypeDrain( void )		{ RemoveCond( TF_COND_SODAPOPPER_HYPE ); }
	bool	IsHypeBuffed( void ) const		{ return InCond( TF_COND_SODAPOPPER_HYPE ); }

	void	DemoShieldChargeThink( void );
	void	UpdateChargeMeter( void );
	float	GetDemomanChargeMeter() const		{ return m_flChargeMeter; }
	void	EndCharge( void );
	float	CalculateChargeCap( void ) const;
	void	SetDemomanChargeMeter( float val )  { m_flChargeMeter = Clamp( val, 0.0f, 100.0f); }
	void	CalcChargeCrit( bool bForceCrit=false );
	bool	HasDemoShieldEquipped() const;

	bool	IsJumping( void ) const			{ return m_bJumping; }
	void	SetJumping( bool bJumping );
	bool    IsAirDashing( void ) const		{ return (m_iAirDash > 0); }
	int		GetAirDash( void ) const		{ return m_iAirDash; }
	void    SetAirDash( int iAirDash );
	void	SetAirDucked( int nAirDucked )	{ m_nAirDucked = nAirDucked; }
	int		AirDuckedCount( void )			{ return m_nAirDucked; }
	void	SetDuckTimer( float flTime )	{ m_flDuckTimer = flTime; }
	float	GetDuckTimer( void ) const		{ return m_flDuckTimer; }

	void	DebugPrintConditions( void );
	void	InstantlySniperUnzoom( void );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	float	GetDisguiseCompleteTime( void ) { return m_flDisguiseCompleteTime; }
	bool	IsSpyDisguisedAsMyTeam( CTFPlayer *pPlayer );

	// Stuns
	stun_struct_t *GetActiveStunInfo( void ) const;
#ifdef GAME_DLL
	void	StunPlayer( float flTime, float flReductionAmount, int iStunFlags = TF_STUN_MOVEMENT, CTFPlayer* pAttacker = NULL );
#endif // GAME_DLL
	float	GetAmountStunned( int iStunFlags );
	bool	IsLoserStateStunned( void ) const;
	bool	IsControlStunned( void );
	bool	IsSnared( void );
	CTFPlayer *GetStunner( void );
	void	ControlStunFading( void );
	int		GetStunFlags( void ) const		{ return GetActiveStunInfo() ? GetActiveStunInfo()->iStunFlags : 0; }
	float	GetStunExpireTime( void ) const { return GetActiveStunInfo() ? GetActiveStunInfo()->flExpireTime : 0; }
	void	SetStunExpireTime( float flTime );
	void	UpdateLegacyStunSystem( void );

	bool	IsFirstBloodBoosted( void ) const	{ return m_bArenaFirstBloodBoost; }
	void	SetFirstBloodBoosted( bool bBoost ) { m_bArenaFirstBloodBoost = bBoost; }

	CTFPlayer *GetAssist( void ) const			{ return m_hAssist; }
	void	SetAssist( CTFPlayer* newAssist )	{ m_hAssist = newAssist; }

#ifdef GAME_DLL
	CTFPlayer *GetBurnAttacker( void ) const			{ return m_hBurnAttacker; }
	CTFPlayer *GetOriginalBurnAttacker( void ) const	{ return m_hOriginalBurnAttacker; }
	CTFWeaponBase *GetBurnWeapon( void ) const			{ return m_hBurnWeapon; }
#endif // GAME_DLL

	void SetCloakConsumeRate( float newCloakConsumeRate ) { m_fCloakConsumeRate = newCloakConsumeRate; }
	void SetCloakRegenRate( float newCloakRegenRate ) { m_fCloakRegenRate = newCloakRegenRate; }

	int		GetWeaponKnockbackID( void ) const	{ return m_iWeaponKnockbackID; }
	void	SetWeaponKnockbackID( int iID )		{ m_iWeaponKnockbackID = iID; }

	bool	IsLoadoutUnavailable( void ) { return m_bLoadoutUnavailable; }
	void	SetLoadoutUnavailable( bool bUnavailable ) { m_bLoadoutUnavailable = bUnavailable; }

	float GetInvulOffTime( void ) { return m_flInvulnerabilityRemoveTime; }

	int		GetDisguiseBody( void ) const	{ return m_iDisguiseBody; }
	void	SetDisguiseBody( int iVal )		{ m_iDisguiseBody = iVal; }

	bool	IsCarryingObject( void )		const { return m_bCarryingObject; }
	CBaseObject* GetCarriedObject( void )	const { return m_hCarriedObject.Get(); }
	void	SetCarriedObject( CBaseObject* pObj );
	void	StartBuildingObjectOfType( int iType, int iObjectMode=0 );

	void	InterruptCharge( void );

	void	PulseKingRuneBuff();

public:
	// Scoring
	int	GetCaptures( int iIndex ) const				{ return m_ScoreData.m_iCaptures; }
	int	GetDefenses( int iIndex ) const				{ return m_ScoreData.m_iDefenses; }
	int	GetDominations( int iIndex ) const			{ return m_ScoreData.m_iDominations; }
	int	GetRevenge( int iIndex ) const				{ return m_ScoreData.m_iRevenge; }
	int	GetBuildingsDestroyed( int iIndex ) const	{ return m_ScoreData.m_iBuildingsDestroyed; }
	int	GetHeadshots( int iIndex ) const			{ return m_ScoreData.m_iHeadshots; }
	int	GetBackstabs( int iIndex ) const			{ return m_ScoreData.m_iBackstabs; }
	int	GetHealPoints( int iIndex ) const			{ return m_ScoreData.m_iHealPoints; }
	int	GetInvulns( int iIndex ) const				{ return m_ScoreData.m_iInvulns; }
	int	GetTeleports( int iIndex ) const			{ return m_ScoreData.m_iTeleports; }
	int	GetResupplyPoints( int iIndex ) const		{ return m_ScoreData.m_iResupplyPoints; }
	int	GetKillAssists( int iIndex ) const			{ return m_ScoreData.m_iKillAssists; }
	int GetBonusPoints( int iIndex ) const			{ return m_ScoreData.m_iBonusPoints; }

	void ResetScores( void ) { m_ScoreData.Reset(); }
	localplayerscoring_t *GetScoringData( void ) { return &m_ScoreData; }

	// Per-round scoring data utilized by the steamworks stats system.
	void ResetRoundScores( void ) { m_RoundScoreData.Reset(); }
	localplayerscoring_t *GetRoundScoringData( void ) { return &m_RoundScoreData; }

	void SetFeignDeathReady( bool bVal ) { m_bFeignDeathReady = bVal; }
	bool IsFeignDeathReady( void ) const { return m_bFeignDeathReady; }
	void ReduceFeignDeathDuration( float flAmount ) { m_flFeignDeathEnd -= flAmount; }

	void SetShieldEquipped( bool bVal ) { m_bShieldEquipped = bVal; }
	bool IsShieldEquipped() const		{ return m_bShieldEquipped; }

	void SetParachuteEquipped( bool bVal ) { m_bParachuteEquipped = bVal; }
	bool IsParachuteEquipped() const	{ return m_bParachuteEquipped; }

	void SetNextMeleeCrit( int iVal )	{ m_iNextMeleeCrit = iVal; }
	int	GetNextMeleeCrit( void ) const	{ return m_iNextMeleeCrit; }

	void SetDecapitations( int iVal )	{ m_iDecapitations = iVal; }
	int GetDecapitations( void ) const	{ return m_iDecapitations; }

	void SetStreak( ETFStreak streak_type, int iVal )		{ m_nStreaks.Set( streak_type, iVal ); }
	int GetStreak( ETFStreak streak_type ) const		{ return m_nStreaks[streak_type]; }
	int IncrementStreak( ETFStreak streak_type, int iVal )
	{
		// Track duck streak steps so we can put deltas in the event
		if ( streak_type == kTFStreak_Ducks )
			m_nLastDuckStreakIncrement = iVal;
		m_nStreaks.Set( streak_type, m_nStreaks[streak_type] + iVal );
		return m_nStreaks[streak_type];
	}
	void ResetStreaks( void )	{ for ( int streak_type = 0; streak_type < kTFStreak_COUNT; streak_type++ ) { m_nStreaks.Set( streak_type, 0 ); } }

	int GetLastDuckStreakIncrement( void ) const	{ return m_nLastDuckStreakIncrement; }

	void SetRevengeCrits( int iVal );
	int GetRevengeCrits( void ) const { return m_iRevengeCrits; }
	void IncrementRevengeCrits( void );

	int GetSequenceForDeath( CBaseAnimating* pRagdoll, bool bBurning, int nCustomDeath );

	float GetNextNoiseMakerTime() const			{ return m_flNextNoiseMakerTime; }
	void  SetNextNoiseMakerTime( float time )	{ m_flNextNoiseMakerTime = time; }

	void	Heal_Radius ( bool bActive );

	void IncrementRespawnTouchCount() { ++m_iSpawnRoomTouchCount; }
	void DecrementRespawnTouchCount() { m_iSpawnRoomTouchCount = Max( m_iSpawnRoomTouchCount - 1, 0 ); }
	int GetRespawnTouchCount() const { return m_iSpawnRoomTouchCount; }

#ifdef CLIENT_DLL
	void SetVehicleMoveAngles( const QAngle& angVehicleMoveAngles ) { m_angVehicleMovePitchLast = angVehicleMoveAngles[PITCH]; m_angVehicleMoveAngles = angVehicleMoveAngles; }
#endif

#ifdef GAME_DLL
	void SetBestOverhealDecayMult( float fValue )	{ m_flBestOverhealDecayMult = fValue; }
	float GetBestOverhealDecayMult() const			{ return m_flBestOverhealDecayMult; }

	void	SetPeeAttacker( CTFPlayer *pPlayer )	{ m_hPeeAttacker = pPlayer; }
	CTFPlayer *GetPeeAttacker() const				{ return m_hPeeAttacker; }

	void  SetChargeEffectOffTime( int iCondition, float flTime ) { m_flChargeEffectOffTime[iCondition] = flTime; }

	void GetSpeedWatchersList( CUtlVector<CTFPlayer *> *out_pVecSpeedWatchers ) const;
#endif

	int GetTauntIndex( void ) const { return m_iTauntIndex; }
	int GetTauntConcept( void ) const { return m_iTauntConcept; }
	itemid_t GetTauntSourceItemID( void ) const { return ((uint64)m_unTauntSourceItemID_High << 32) | (uint64)m_unTauntSourceItemID_Low; }
	void CreateVehicleMove( float flInputSampleTime, CUserCmd *pCmd );
	void SetVehicleTurnAmount( float flTurnSpeed ) { m_flCurrentTauntTurnSpeed = flTurnSpeed; }
	float GetVehicleTurnPoseAmount( void ) const { return m_flCurrentTauntTurnSpeed; }
	void VehicleThink( void );

	bool IsLoser( void );

	int	GetItemFindBonus( void ) //{ return m_iItemFindBonus; }
	{
#ifdef GAME_DLL
		if ( !m_iItemFindBonus )
		{
			m_iItemFindBonus = RandomInt( 1, 300 );
		}
#endif
		return m_iItemFindBonus;
	}
	
	void	RecalculatePlayerBodygroups( void );

	void	FireGameEvent( IGameEvent *event );
	
#ifdef GAME_DLL
	float	GetFlameBurnTime( void ) const { return m_flFlameBurnTime; }
#endif // GAME_DLL

	void GetConditionsBits( CBitVec< TF_COND_LAST >& vbConditions ) const;

	void ApplyRocketPackStun( float flStunDuration );


	void OnAttack( void );

#ifdef GAME_DLL
	void SetDefaultItemChargeMeters( void );
#endif // GAME_DLL

private:
	CNetworkVarEmbedded( localplayerscoring_t,	m_ScoreData );
	CNetworkVarEmbedded( localplayerscoring_t,	m_RoundScoreData );

private:

#ifdef CLIENT_DLL
	typedef std::pair<const char *, float> taunt_particle_state_t;
	taunt_particle_state_t GetClientTauntParticleDesiredState() const;
	void FireClientTauntParticleEffects();
#endif // CLIENT_DLL

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddZoomed( void );
	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddDemoCharge( void );
	void OnAddCritBoost( void );
	void OnAddSodaPopperHype( void );
	void OnAddOverhealed( void );
	void OnAddFeignDeath( void );
	void OnAddStunned( void );
	void OnAddPhase( void );
	void OnAddUrine( void );
	void OnAddMarkedForDeath( void );
	void OnAddBleeding( void );
	void OnAddDefenseBuff( void );
	void OnAddOffenseBuff( void );
	void OnAddOffenseHealthRegenBuff( void );
	const char* GetSoldierBuffEffectName( void );
	void OnAddSoldierOffensiveBuff( void );
	void OnAddSoldierDefensiveBuff( void );
	void OnAddSoldierOffensiveHealthRegenBuff( void );
	void OnAddSoldierNoHealingDamageBuff( void );
	void OnAddShieldCharge( void );
	void OnAddDemoBuff( void );
	void OnAddEnergyDrinkBuff( void	);
	void OnAddRadiusHeal( void );
	void OnAddMegaHeal( void );
	void OnAddMadMilk( void );
	void OnAddTaunting( void );
	void OnAddNoHealingDamageBuff( void );
	void OnAddSpeedBoost( bool IsNonCombat );
	void OnAddSapped( void );
	void OnAddReprogrammed( void );
	void OnAddMarkedForDeathSilent( void );
	void OnAddDisguisedAsDispenser( void );
	void OnAddHalloweenBombHead( void );
	void OnAddHalloweenThriller( void );
	void OnAddRadiusHealOnDamage( void );
	void OnAddMedEffectUberBulletResist( void );
	void OnAddMedEffectUberBlastResist( void );
	void OnAddMedEffectUberFireResist( void );
	void OnAddMedEffectSmallBulletResist( void );
	void OnAddMedEffectSmallBlastResist( void );
	void OnAddMedEffectSmallFireResist( void );
	void OnAddStealthedUserBuffFade( void );
	void OnAddBulletImmune( void );
	void OnAddBlastImmune( void );
	void OnAddFireImmune( void );
	void OnAddMVMBotRadiowave( void );
	void OnAddHalloweenSpeedBoost( void );
	void OnAddHalloweenQuickHeal( void );
	void OnAddHalloweenGiant( void );
	void OnAddHalloweenTiny( void );
	void OnAddHalloweenGhostMode( void );
	void OnAddHalloweenKartDash( void );
	void OnAddHalloweenKart( void );
	void OnAddBalloonHead( void );
	void OnAddMeleeOnly( void );
	void OnAddSwimmingCurse( void );
	void OnAddHalloweenKartCage( void );
	void OnAddRuneResist( void );
	void OnAddGrapplingHookLatched( void );
	void OnAddPasstimeInterception( void );
	void OnAddRunePlague( void );
	void OnAddPlague( void );
	void OnAddKingBuff( void );
	void OnAddInPurgatory( void );
	void OnAddCompetitiveWinner( void );
	void OnAddCompetitiveLoser( void );
	void OnAddCondGas( void );
	void OnAddRocketPack( void );


	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveTeleported( void );
	void OnRemoveDemoCharge( void );
	void OnRemoveCritBoost( void );
	void OnRemoveSodaPopperHype( void );
	void OnRemoveTmpDamageBonus( void );
	void OnRemoveOverhealed( void );
	void OnRemoveFeignDeath( void );
	void OnRemoveStunned( void );
	void OnRemovePhase( void );
	void OnRemoveUrine( void );
	void OnRemoveMarkedForDeath( void );
	void OnRemoveBleeding( void );
	void OnRemoveInvulnerableWearingOff( void );
	void OnRemoveDefenseBuff( void );
	void OnRemoveOffenseBuff( void );
	void OnRemoveOffenseHealthRegenBuff( void );
	void OnRemoveSoldierOffensiveBuff( void );
	void OnRemoveSoldierDefensiveBuff( void );
	void OnRemoveSoldierOffensiveHealthRegenBuff( void );
	void OnRemoveSoldierNoHealingDamageBuff( void );
	void OnRemoveShieldCharge( void );
	void OnRemoveDemoBuff( void );
	void OnRemoveEnergyDrinkBuff( void );
	void OnRemoveRadiusHeal( void );
	void OnRemoveMegaHeal( void );
	void OnRemoveMadMilk( void );
	void OnRemoveTaunting( void );
	void OnRemoveNoHealingDamageBuff( void );
	void OnRemoveSpeedBoost( bool IsNonCombat );
	void OnRemoveSapped( void );
	void OnRemoveReprogrammed( void );
	void OnRemoveMarkedForDeathSilent( void );
	void OnRemoveDisguisedAsDispenser( void );
	void OnRemoveHalloweenBombHead( void );
	void OnRemoveHalloweenThriller( void );
	void OnRemoveRadiusHealOnDamage( void );
	void OnRemoveMedEffectUberBulletResist( void );
	void OnRemoveMedEffectUberBlastResist( void );
	void OnRemoveMedEffectUberFireResist( void );
	void OnRemoveMedEffectSmallBulletResist( void );
	void OnRemoveMedEffectSmallBlastResist( void );
	void OnRemoveMedEffectSmallFireResist( void );
	void OnRemoveStealthedUserBuffFade( void );
	void OnRemoveBulletImmune( void );
	void OnRemoveBlastImmune( void );
	void OnRemoveFireImmune( void );
	void OnRemoveMVMBotRadiowave( void );
	void OnRemoveHalloweenSpeedBoost( void );
	void OnRemoveHalloweenQuickHeal( void );
	void OnRemoveHalloweenGiant( void );
	void OnRemoveHalloweenTiny( void );
	void OnRemoveHalloweenGhostMode( void );
	void OnRemoveHalloweenKartDash( void );
	void OnRemoveHalloweenKart( void );
	void OnRemoveBalloonHead( void );
	void OnRemoveMeleeOnly( void );
	void OnRemoveSwimmingCurse( void );
	void OnRemoveHalloweenKartCage( void );
	void OnRemoveRuneResist( void );
	void OnRemoveGrapplingHookLatched( void );
	void OnRemovePasstimeInterception( void );
	void OnRemoveRunePlague( void );
	void OnRemovePlague( void );
	void OnRemoveRuneKing( void );
	void OnRemoveKingBuff( void );
	void OnRemoveRuneSupernova( void );
	void OnRemoveInPurgatory( void );
	void OnRemoveCompetitiveWinner( void );
	void OnRemoveCompetitiveLoser( void );
	void OnRemoveCondGas( void );
	void OnRemoveRocketPack( void );
	void OnRemoveBurningPyro( void );
	

	// Starting a new trend, putting Add and Remove next to each other
	void OnAddCondParachute( void );
	void OnRemoveCondParachute( void );

	void OnAddHalloweenHellHeal( void );
	void OnRemoveHalloweenHellHeal( void );

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill, int nVictimPrevHealth );
	void  AddTempCritBonus( float flAmount );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	void  SendNewInvulnGameEvent( void );
	void  SetChargeEffect( medigun_charge_types iCharge, bool bState, bool bInstant, const struct MedigunEffects_t& effects, float flWearOffTime, CTFPlayer *pProvider = NULL );
	void  SetCritBoosted( bool bState );

	void RadiusCurrencyCollectionCheck( void );
	void RadiusHealthkitCollectionCheck( void );
	void RadiusSpyScan( void );

	// Attr for Conditions
	void ApplyAttributeToPlayer( const char* pszAttribName, float flValue );
	void RemoveAttributeFromPlayer( const char* pszAttribName );

public:
	void SetAfterburnDuration( float flDuration ) { m_flAfterburnDuration = flDuration; }
	float GetAfterburnDuration( void ) { return m_flAfterburnDuration; }
#endif // GAME_DLL

private:
	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	CNetworkVar( int, m_nPlayerCondEx );		// Player condition flags (extended -- we overflowed 32 bits).
	CNetworkVar( int, m_nPlayerCondEx2 );		// Player condition flags (extended -- we overflowed 64 bits).
	CNetworkVar( int, m_nPlayerCondEx3 );		// Player condition flags (extended -- we overflowed 96 bits).
	CNetworkVar( int, m_nPlayerCondEx4 );		// Player condition flags (extended -- we overflowed 128 bits).

	CNetworkVarEmbedded( CTFConditionList, m_ConditionList );

//TFTODO: What if the player we're disguised as leaves the server?
//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	CNetworkVar( int, m_nDisguiseSkinOverride ); // skin override value of the player spy disguised as.
	CNetworkVar( int, m_nMaskClass );
	CNetworkHandle( CTFPlayer, m_hDisguiseTarget ); // Player the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );
	CNetworkHandle( CTFWeaponBase, m_hDisguiseWeapon );
	CNetworkVar( int, m_nTeamTeleporterUsed ); // for disguised spies using enemy teleporters
	CHandle<CTFPlayer>	m_hDesiredDisguiseTarget;
	int m_iDisguiseAmmo;

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player separate from teammates

	float m_flInvisibility;
	float m_flPrevInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;
	float m_fCloakConsumeRate;
	float m_fCloakRegenRate;
	bool  m_bMotionCloak;
#ifdef GAME_DLL
	// Cloak Strange Tracking
	float m_flCloakStartTime;
#endif

	float m_fEnergyDrinkConsumeRate;
	float m_fEnergyDrinkRegenRate;


	EHANDLE m_pPhaseTrail[TF_SCOUT_NUMBEROFPHASEATTACHMENTS];
	bool m_bPhaseFXOn;
	float m_fPhaseAlpha;

	CNetworkVar( int, m_nNumHealers );

	// Halloween silliness.
	CNetworkVar( int, m_nHalloweenBombHeadStage );

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL
	// Healer handling
	struct healers_t
	{
		EHANDLE	pHealer;
		float	flAmount;
		float   flHealAccum;
		float	flOverhealBonus;
		float	flOverhealDecayMult;
		bool	bDispenserHeal;
		EHANDLE pHealScorer;
		int		iKillsWhileBeingHealed; // for engineer achievement ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE
		float	flHealedLastSecond;
	};
	CUtlVector< healers_t >	m_aHealers;	
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing
	float					m_flBestOverhealDecayMult;
	float					m_flHealedPerSecondTimer;

	float m_flChargeEffectOffTime[MEDIGUN_NUM_CHARGE_TYPES];
	bool  m_bChargeSoundEffectsOn[MEDIGUN_NUM_CHARGE_TYPES];

	// Heal_Radius handling
	CUtlVector< int >		m_iRadiusHealTargets;
	float					m_flRadiusHealCheckTime;

#endif

	// King Rune buff 
	float		m_flKingRuneBuffCheckTime;
	CNetworkVar( bool, m_bKingRuneBuffActive );

	bool					m_bPulseRadiusHeal;

	CNetworkVar( bool, m_bLastDisguisedAsOwnTeam );

#ifdef GAME_DLL
	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CHandle<CTFPlayer>		m_hOriginalBurnAttacker;		// Player who originally ignited this target
	CHandle<CTFWeaponBase>	m_hBurnWeapon;
	float					m_flFlameBurnTime;
	float					m_flAfterburnDuration;

	// Bleeding
	struct bleed_struct_t
	{
		CHandle<CTFPlayer>		hBleedingAttacker;
		CHandle<CTFWeaponBase>  hBleedingWeapon;
		float					flBleedingTime;
		float					flBleedingRemoveTime;
		int						nBleedDmg;
		bool					bPermanentBleeding;
		int						nDmgType;
	};
	CUtlVector <bleed_struct_t> m_PlayerBleeds;
#endif // GAME_DLL

	CNetworkVar( int, m_iTauntIndex );
	CNetworkVar( int, m_iTauntConcept );
	CNetworkVar( uint32, m_unTauntSourceItemID_Low );
	CNetworkVar( uint32, m_unTauntSourceItemID_High );
#ifdef CLIENT_DLL
	float					m_flTauntParticleRefireTime;
#endif // CLIENT_DLL

	float					m_flDisguiseCompleteTime;
	float					m_flTmpDamageBonusAmount;

#ifdef CLIENT_DLL
	bool m_bSyncingConditions;
#endif

	// conditions
	int	m_nOldConditions;
	int m_nOldConditionsEx;
	int m_nOldConditionsEx2;
	int m_nOldConditionsEx3;
	int m_nOldConditionsEx4;

	int	m_nForceConditions;
	int m_nForceConditionsEx;
	int m_nForceConditionsEx2;
	int m_nForceConditionsEx3;
	int m_nForceConditionsEx4;

	int	m_nOldDisguiseClass;
	int	m_nOldDisguiseTeam;

	// Feign Death
	float					m_flFeignDeathEnd;
	CNetworkVar( bool, m_bFeignDeathReady );

	CNetworkVar( int, m_iDesiredPlayerClass );

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]

	CNetworkVar( bool, m_bInUpgradeZone );

	// Scout
	CNetworkVar( float, m_flEnergyDrinkMeter );	// [0,100]
	CNetworkVar( float, m_flHypeMeter );

	// Demoman
	CNetworkVar( float, m_flChargeMeter );

	// Rage (Soldier, Pyro, Sniper)
	CNetworkVar( float, m_flRageMeter );
	CNetworkVar( bool, m_bRageDraining );
	CNetworkVar( float, m_flNextRageEarnTime );

	RageBuff m_RageBuffSlots[kBuffSlot_MAX];

	// Movement.
	CNetworkVar( bool, m_bJumping );
	CNetworkVar( int,  m_iAirDash );
	CNetworkVar( int, m_nAirDucked );
	CNetworkVar( float, m_flDuckTimer );

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( float, m_flRuneCharge );

	// generic charge percentage for weapon to use
	
	CNetworkArray( float, m_flItemChargeMeter, LAST_LOADOUT_SLOT_WITH_CHARGE_METER + 1 );
	float m_flPrevItemChargeMeter[ LAST_LOADOUT_SLOT_WITH_CHARGE_METER + 1 ];


	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS_ARRAY_SAFE );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS_ARRAY_SAFE );	// array of state per other player whether other players are dominating this player

	CNetworkVar( float, m_flMovementStunTime );
	CNetworkVar( int, m_iMovementStunAmount );
	CNetworkVar( unsigned char, m_iMovementStunParity );
	CNetworkHandle( CTFPlayer, m_hStunner );
	CNetworkVar( int, m_iStunFlags );
	CNetworkVar( int, m_iStunIndex );

	CNetworkHandle( CBaseObject, m_hCarriedObject );
	CNetworkVar( bool, m_bCarryingObject );

	CHandle<CTFPlayer>	m_hAssist;

	CNetworkVar( int, m_nArenaNumChanges );			// number of times a player has re-rolled their class

	CNetworkVar( int, m_iWeaponKnockbackID );

	CNetworkVar( bool, m_bLoadoutUnavailable );

	CNetworkVar( int,	m_iItemFindBonus );

	CNetworkVar( bool, m_bShieldEquipped );

	CNetworkVar( bool, m_bParachuteEquipped );

	CNetworkVar( int, m_iDecapitations );
	int m_iOldDecapitations;

	CNetworkArray( int, m_nStreaks, kTFStreak_COUNT );

	int m_nLastDuckStreakIncrement;

	int m_iOldKillStreak;
	int m_iOldKillStreakWepSlot;

	CNetworkVar( int, m_iRevengeCrits );

	CNetworkVar( int,  m_iNextMeleeCrit );
	bool m_bPostShieldCharge;
	float m_flChargeEndTime;

	CNetworkVar( int,  m_iDisguiseBody );

	CNetworkVar( int,  m_iSpawnRoomTouchCount );

	CNetworkVar( float, m_flNextNoiseMakerTime );

	float m_flCurrentTauntTurnSpeed;
#ifdef CLIENT_DLL
	const WheelEffect_t *m_pWheelEffect;
	QAngle m_angVehicleMoveAngles;
	float m_angVehicleMovePitchLast;

	CHandle<CBaseAnimating>		m_hKartParachuteEntity;

#endif
public:
	CNetworkVar( int, m_iKillCountSinceLastDeploy );
	CNetworkVar( float, m_flFirstPrimaryAttack );

	CNetworkVar( float, m_flSpyTranqBuffDuration );

private:

#ifdef GAME_DLL
	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;

	CHandle<CTFPlayer>	m_hPeeAttacker;

	float m_flRadiusCurrencyCollectionTime;
	float m_flRadiusSpyScanTime;

	struct pulledcurrencypacks_t
	{
		CHandle<CCurrencyPack> hPack;
		float flTime;
	};
	CUtlVector <pulledcurrencypacks_t> m_CurrencyPacks;

#else
	float	m_flGotTeleEffectAt;
	unsigned char m_iOldMovementStunParity;
	CSoundPatch	*m_pCritBoostSoundLoop;
#endif

public:
	float	m_flStunFade;
	float	m_flStunEnd;
	float	m_flStunMid;
	TFStunAnimState_t	m_iStunAnimState;
	int		m_iPhaseDamage;
	
	// Movement stun state.
	bool		m_bStunNeedsFadeOut;
	float		m_flStunLerpTarget;
	float		m_flLastMovementStunChange;
#ifdef GAME_DLL
	CUtlVector <stun_struct_t> m_PlayerStuns;
	CUtlVector< CHandle<CTFPlayer> >	m_hPlayersVisibleAtChargeStart; // ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE
#else
	stun_struct_t m_ActiveStunInfo;
#endif // CLIENT_DLL

	float	m_flInvulnerabilityRemoveTime;

	/*
	float	m_flShieldChargeStartTime;
	*/

	// Demoman charge weapon glow.
	float	m_flLastChargeTime;
	float	m_flLastNoChargeTime;
	bool	m_bChargeGlowing;

	bool	m_bChargeOffSounded;

	bool	m_bBiteEffectWasApplied;

	float	m_flLastNoMovementTime;

	CNetworkVar( bool, m_bArenaFirstBloodBoost );

// passtime
public:
	void SetHasPasstimeBall( bool has ) { m_bHasPasstimeBall = has; }
	bool HasPasstimeBall() const { return m_bHasPasstimeBall; }
	
	bool IsTargetedForPasstimePass() const { return m_bIsTargetedForPasstimePass; }
	void SetPasstimePassTarget( CTFPlayer *ent );
	CTFPlayer *GetPasstimePassTarget() const;
	
	void SetAskForBallTime( float time ) { m_askForBallTime = time; }
	float AskForBallTime() const { return m_askForBallTime; }

	float m_flPasstimeThrowAnimStateTime;
	TFPassTimeThrowAnimState_t	m_iPasstimeThrowAnimState;

	float m_flCYOAPDAAnimStateTime;
	TFCYOAPDAAnimState_t m_iCYOAPDAAnimState;

private:
	CNetworkVar( bool, m_bHasPasstimeBall );
	CNetworkVar( bool, m_bIsTargetedForPasstimePass );
	CNetworkHandle( CTFPlayer, m_hPasstimePassTarget );
	CNetworkVar( float, m_askForBallTime );

	CNetworkVar( float, m_flHolsterAnimTime );
	CNetworkHandle( CBaseCombatWeapon, m_hSwitchTo );
};

extern const char *g_pszBDayGibs[22];

class CTraceFilterIgnoreTeammatesAndTeamObjects : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesAndTeamObjects, CTraceFilterSimple );

	CTraceFilterIgnoreTeammatesAndTeamObjects( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	int m_iIgnoreTeam;
};

enum { kSoldierBuffCount = 6 };
extern ETFCond g_SoldierBuffAttributeIDToConditionMap[kSoldierBuffCount + 1];

class CTFPlayerSharedUtils
{
public:
	static CEconItemView *GetEconItemViewByLoadoutSlot( CTFPlayer *pTFPlayer, int iSlot, CEconEntity **pEntity = NULL );
	static bool ConceptIsPartnerTaunt( int iConcept );
	static CTFWeaponBuilder *GetBuilderForObjectType( CTFPlayer *pTFPlayer, int iObjectType );
};

class CTargetOnlyFilter : public CTraceFilterSimple
{
public:
	CTargetOnlyFilter( CBaseEntity *pShooter, CBaseEntity *pTarget );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	CBaseEntity	*m_pShooter;
	CBaseEntity	*m_pTarget;
};

#endif // TF_PLAYER_SHARED_H
