//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef TF_PLAYER_H
#define TF_PLAYER_H
#pragma once

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "tf_achievementdata.h"
#include "tf_playeranimstate.h"
#include "tf_shareddefs.h"
#include "tf_obj.h"
#include "tf_player_shared.h"
#include "tf_playerclass.h"
#include "entity_tfstart.h"
#include "steam/steam_gameserver.h"
#include "ihasattributes.h"
#include "tf_item_inventory.h"

class CTFPlayer;
class CTFTeam;
class CTFGoal;
class CTFGoalItem;
class CTFItem;
class CTFWeaponBuilder;
//class CBaseObject;
class CTFWeaponBase;
class CIntroViewpoint;
class CTriggerAreaCapture;
class CTFWeaponBaseGun;
class CCaptureZone;
class CTFReviveMarker;
class CWaveSpawnPopulator;
class CTFTauntProp;
class CTFDroppedWeapon;

extern const float tf_afterburn_max_duration;

#define MAX_FIRE_WEAPON_SCENES 4

//=============================================================================
//
// Player State Information
//
class CPlayerStateInfo
{
public:

	int				m_nPlayerState;
	const char		*m_pStateName;

	// Enter/Leave state.
	void ( CTFPlayer::*pfnEnterState )();	
	void ( CTFPlayer::*pfnLeaveState )();

	// Think (called every frame).
	void ( CTFPlayer::*pfnThink )();
};

enum EAmmoSource
{
	kAmmoSource_Pickup,					// this came from either a box of ammo or a player's dropped weapon
	kAmmoSource_Resupply,				// resupply cabinet and/or full respawn
	kAmmoSource_DispenserOrCart,		// the player is standing next to an engineer's dispenser or pushing the cart in a payload game
	kAmmoSource_ResourceMeter,			// it regenerated after a cooldown
};

extern ConVar tf_voice_command_suspension_rate_limit_bucket_count;
extern ConVar tf_voice_command_suspension_rate_limit_bucket_refill_rate;

class CVoiceCommandBucketSizer
{
public:
	int GetBucketSize() const { return tf_voice_command_suspension_rate_limit_bucket_count.GetInt(); }
	float GetBucketRefillRate() const { return tf_voice_command_suspension_rate_limit_bucket_refill_rate.GetFloat(); }
};

template <typename TBucketSizer>
class CRateLimitingTokenBucket : public TBucketSizer
{
public:
	CRateLimitingTokenBucket()
		: m_nBucket( this->GetBucketSize() )
	{
	}

	bool BTakeToken( float flNow )
	{
		// misyl: This token bucket doesn't go negative, so you don't ever dig yourself into a hole by spamming.
		// You might want that if you use this class, feel free to add something to the BucketSizer.

		int nNewBucket = MIN( m_nBucket + ( flNow - m_flLastTokenTaken ) / this->GetBucketRefillRate(), this->GetBucketSize() ) - 1;
		if ( nNewBucket <= 0 )
		{
			return false;
		}

		m_nBucket = nNewBucket;
		m_flLastTokenTaken = flNow;

		return true;
	}
private:
	float m_flLastTokenTaken = 0.0f;
	int m_nBucket = 0;
};

//=============================================================================
//
// TF Player
//
class CTFPlayer : public CBaseMultiplayerPlayer, public IHasAttributes, public IInventoryUpdateListener
{
public:
	DECLARE_CLASS( CTFPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	CTFPlayer();
	~CTFPlayer();

	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	Added a player type so we can distinguish between bots and humans.
	//=============================================================================
	enum TFPlayerType{
		HUMAN_PLAYER,
		TEMP_BOT,
		TRAINING_BOT
	};
	//=============================================================================
	// HPE_END
	//=============================================================================

	// Creation/Destruction.
	static CTFPlayer	*CreatePlayer( const char *className, edict_t *ed );
	static CTFPlayer	*Instance( int iEnt );

	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo );

	virtual void		SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize );
	virtual void		Spawn();
	virtual void		ForceRespawn();
	void				ForceRegenerateAndRespawn( void );
	virtual CBaseEntity	*EntSelectSpawnPoint( void );
	virtual void		InitialSpawn();
	static void			PrecacheMvM();
	static void			PrecacheKart();
private:
	static void			PrecachePlayerModels();
	static void			PrecacheTFPlayer();
public:
	virtual void		Precache();
	virtual bool		IsReadyToPlay( void );
	virtual bool		IsReadyToSpawn( void );
	virtual bool		ShouldGainInstantSpawn( void );
	virtual void		ResetScores( void );
	virtual void		UpdateOnRemove( void );
	void				CheckInstantLoadoutRespawn( void );

	virtual void		ResetPerRoundStats( void );

	void				HandleCommand_JoinTeam( const char *pTeamName );
	void				HandleCommand_JoinClass( const char *pClassName, bool bAllowSpawn = true );
	void				HandleCommand_JoinTeam_NoMenus( const char *pTeamName );

	void				CreateViewModel( int iViewModel = 0 );
	CBaseViewModel		*GetOffHandViewModel();
	void				SendOffHandViewModelActivity( Activity activity );

	virtual void		CheatImpulseCommands( int iImpulse );
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );

	// Combats
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int			TakeHealth( float flHealth, int bitsDamageType );
	virtual	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual void		PlayerDeathThink( void );
	virtual void		DetermineAssistForKill( const CTakeDamageInfo &info );
	virtual void		SetNumberofDominations( int iDominations )
	{
		// Check for some bogus values, which are sneaking in somehow
		if ( iDominations < 0 )
		{
			Assert( iDominations >= 0 );
			iDominations = 0;
		}
		else if ( iDominations >= MAX_PLAYERS )
		{
			Assert( iDominations < MAX_PLAYERS );
			iDominations = MAX_PLAYERS-1;
		}
		m_iNumberofDominations = iDominations;
	}
	virtual int			GetNumberofDominations( void ) { return m_iNumberofDominations; }
	void				OnKilledOther_Effects( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	void				CheckSpellHalloweenDeathGhosts( const CTakeDamageInfo &info, CTFPlayer *pTFVictim );

	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void				AddConnectedPlayers( CUtlVector<CTFPlayer*> &vecPlayers, CTFPlayer *pPlayerToConsider );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void		DamageEffect(float flDamage, int fDamageType);

	void				OnDealtDamage( CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info );		// invoked when we deal damage to another victim
	int					GetDamagePerSecond( void ) const;
	void				ResetDamagePerSecond( void );

	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;
	void				ApplyPushFromDamage( const CTakeDamageInfo &info, Vector vecDir );
	void				PlayDamageResistSound( float flStartDamage, float flModifiedDamage );
	bool				CheckBlockBackstab( CTFPlayer *pTFAttacker );

	virtual bool		Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );

	void				SetHealthBuffTime( float flTime )		{ m_flHealthBuffTime = flTime; }

	CTFWeaponBase		*GetActiveTFWeapon( void ) const;
	int					GetPassiveWeapons( CUtlVector<CTFWeaponBase*>& vecOut );
	bool				IsActiveTFWeapon( const CSchemaItemDefHandle &weaponHandle ) const;
	bool				IsActiveTFWeapon( CEconItemDefinition *weaponHandle ) const;
	virtual void		RemoveAllWeapons();
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon ) OVERRIDE;			// Adds weapon to player

	void				SaveMe( void );


	void				FireBullet( CTFWeaponBase *pWpn, const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType = TF_DMG_CUSTOM_NONE );
	void				ImpactWaterTrace( trace_t &trace, const Vector &vecStart );
	void				NoteWeaponFired();

	bool				HasItem( void ) const;					// Currently can have only one item at a time.
	void				SetItem( CTFItem *pItem );
	CTFItem				*GetItem( void ) const;
	
	void				SaveLastWeaponSlot( void );
	void				SetRememberLastWeapon( bool bRememberLastWeapon ) { m_bRememberLastWeapon = bRememberLastWeapon; }
	void				SetRememberActiveWeapon( bool bRememberActiveWeapon ) { m_bRememberActiveWeapon = bRememberActiveWeapon; }

	void				Regenerate( bool bRefillHealthAndAmmo = true );
	float				GetNextRegenTime( void ){ return m_flNextRegenerateTime; }
	void				SetNextRegenTime( float flTime ){ m_flNextRegenerateTime = flTime; }

	float				GetNextChangeClassTime( void ){ return m_flNextChangeClassTime; }
	void				SetNextChangeClassTime( float flTime ){ m_flNextChangeClassTime = flTime; }

	float				GetNextChangeTeamTime( void ){ return m_flNextChangeTeamTime; }
	void				SetNextChangeTeamTime( float flTime ){ m_flNextChangeTeamTime = flTime; }

	virtual	void		RemoveAllItems( bool removeSuit );

	virtual bool		BumpWeapon( CBaseCombatWeapon *pWeapon );
	bool				DropCurrentWeapon( void );
	void				DropFlag( bool bSilent = false );
	void				DropRune( bool bApplyForce = true, int nTeam = TEAM_ANY );
	void				TFWeaponRemove( int iWeaponID );
	bool				TFWeaponDrop( CTFWeaponBase *pWeapon, bool bThrowForward );

	// Class.
	CTFPlayerClass		 *GetPlayerClass( void ) 					{ return &m_PlayerClass; }
	const CTFPlayerClass *GetPlayerClass( void ) const				{ return &m_PlayerClass; }
	int					GetDesiredPlayerClassIndex( void )			{ return m_Shared.m_iDesiredPlayerClass; }
	void				SetDesiredPlayerClassIndex( int iClass )	{ m_Shared.m_iDesiredPlayerClass = iClass; }

	// Team.
	void				ForceChangeTeam( int iTeamNum, bool bFullTeamSwitch = false );
	virtual void		ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent, bool bAutoBalance = false ) OVERRIDE;
	virtual void		ChangeTeam( int iTeamNum ) OVERRIDE { BaseClass::ChangeTeam( iTeamNum ); }

	// mp_fadetoblack
	void				HandleFadeToBlack( void );

	// Flashlight controls for SFM - JasonM
	virtual int FlashlightIsOn( void );
	virtual void FlashlightTurnOn( void );
	virtual void FlashlightTurnOff( void );

	// Think.
	virtual void		PreThink();
	virtual void		PostThink();

	virtual void		ItemPostFrame();
	virtual void		Weapon_FrameUpdate( void );
	virtual void		Weapon_HandleAnimEvent( animevent_t *pEvent );
	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	void				Weapon_PoseParamOverride( CTFWeaponBase *pOldWeapon, CTFWeaponBase *pNewWeapon );

	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char *GetOverrideStepSound( const char *pszBaseStepSoundName );

	virtual void		OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume );

	virtual void		ModifyEmitSoundParams( EmitSound_t &params );

	// Utility.
	void				UpdateModel( void );
	void				UpdateSkin( int iTeam );

	int					GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource eAmmoSource );
	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	virtual void		RemoveAmmo( int iCount, int iAmmoIndex );
	virtual void		RemoveAmmo( int iCount, const char *szName );
	virtual int			GetAmmoCount( int iAmmoIndex ) const;
	int					GetMaxAmmo( int iAmmoIndex, int iClassIndex = -1 );
	virtual int			GetMaxHealth()  const OVERRIDE;
	int					GetMaxHealthForBuffing();
	int					GetRuneHealthBonus() const;

	//-----------------------------------------------------------------------------------------------------
	// Return true if we are a "mini boss" in Mann Vs Machine mode
	bool				IsMiniBoss( void ) const;
	void				SetIsMiniBoss( bool isMiniBoss ) { m_bIsMiniBoss = isMiniBoss; }

	bool				CanAttack( int iCanAttackFlags = 0 );
	bool				CanJump() const;
	bool				CanDuck() const;

	void				RemoveMeleeCrit( void );

	// This passes the event to the client's and server's CPlayerAnimState.
	void				DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0 );

	virtual void		HandleAnimEvent( animevent_t *pEvent ) OVERRIDE;

	virtual bool		ClientCommand( const CCommand &args );
	void				ClientHearVox( const char *pSentence );
	void				DisplayLocalItemStatus( CTFGoal *pGoal );

	int					BuildObservableEntityList( void );
	virtual int			GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget(bool bReverse);
	virtual bool		IsValidObserverTarget(CBaseEntity * target); // true, if player is allowed to see this target
	virtual bool		SetObserverTarget(CBaseEntity * target);
	virtual bool		ModeWantsSpectatorGUI( int iMode ) { return (iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM); }
	void				FindInitialObserverTarget( void );
	CBaseEntity		    *FindNearestObservableTarget( Vector vecOrigin, float flMaxDist );
	virtual void		ValidateCurrentObserverTarget( void );

	void CheckUncoveringSpies( CTFPlayer *pTouchedPlayer );
	void Touch( CBaseEntity *pOther );

	virtual	void RefreshCollisionBounds( void );

	float GetMovementForwardPull( void ) const;
	bool CanPlayerMove() const;
	float TeamFortress_CalculateMaxSpeed( bool bIgnoreSpecialAbility = false ) const;
	void TeamFortress_SetSpeed();
	CTFPlayer *TeamFortress_GetDisguiseTarget( int nTeam, int nClass );

	void TeamFortress_ClientDisconnected();
	void RemoveAllOwnedEntitiesFromWorld( bool bExplodeBuildings = false );
	void RemoveOwnedProjectiles();
	int GetNumActivePipebombs( void );

	Vector EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon );				// estimate where a projectile fired from the given weapon will initially hit (it may bounce on from there)
	Vector EstimateProjectileImpactPosition( float pitch, float yaw, float initVel );	// estimate where a projectile fired will initially hit (it may bounce on from there)
	Vector EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge );	// Estimate where a stickybomb projectile will hit, using given pitch, yaw, and weapon charge (0-1)

	CTFTeamSpawn *GetSpawnPoint( void ){ return m_pSpawnPoint; }
		
	void SetAnimation( PLAYER_ANIM playerAnim );

	bool IsPlayerClass( int iClass ) const;

	void PlayFlinch( const CTakeDamageInfo &info );

	float PlayCritReceivedSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	virtual const char* GetSceneSoundToken( void );
	void StunSound( CTFPlayer* pAttacker, int iStunFlags, int iOldStunFlags=0 );

	void SetSeeCrit( bool bAllSeeCrit, bool bMiniCrit, bool bShowDisguisedCrit ) { m_bAllSeeCrit = bAllSeeCrit; m_bMiniCrit = bMiniCrit; m_bShowDisguisedCrit = bShowDisguisedCrit;  }
	void SetAttackBonusEffect( EAttackBonusEffects_t effect ) { m_eBonusAttackEffect = effect; }
	EAttackBonusEffects_t GetAttackBonusEffect( void ) { return m_eBonusAttackEffect; }

	// TF doesn't want the explosion ringing sound
	virtual void OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	void OnBurnOther( CTFPlayer *pTFPlayerVictim, CTFWeaponBase *pWeapon );

	// Buildables
	void SetWeaponBuilder( CTFWeaponBuilder *pBuilder );
	CTFWeaponBuilder *GetWeaponBuilder( void );

	int GetBuildResources( void );
	void RemoveBuildResources( int iAmount );
	void AddBuildResources( int iAmount );

	bool IsBuilding( void );
	int CanBuild( int iObjectType, int iObjectMode = 0 );

	CBaseObject	*GetObject( int index ) const;
	CBaseObject	*GetObjectOfType( int iObjectType, int iObjectMode = 0 ) const;
	int	GetObjectCount( void ) const;
	int GetNumObjects( int iObjectType, int iObjectMode = 0 );
	void RemoveAllObjects( bool bExplodeBuildings = false );
	void StopPlacement( void );
	int	StartedBuildingObject( int iObjectType );
	void StoppedBuilding( int iObjectType );
	void FinishedObject( CBaseObject *pObject );
	void AddObject( CBaseObject *pObject );
	void OwnedObjectDestroyed( CBaseObject *pObject );
	void RemoveObject( CBaseObject *pObject );
	bool PlayerOwnsObject( CBaseObject *pObject );
	void DetonateObjectOfType( int iObjectType, int iObjectMode = 0, bool bIgnoreSapperState = false );
	void StartBuildingObjectOfType( int iType, int iObjectMode = 0 );
	float GetObjectBuildSpeedMultiplier( int iObjectType, bool bIsRedeploy ) const;

	void OnSapperPlaced( CBaseEntity *sappedObject );			// invoked when we place a sapper on an enemy building
	bool IsPlacingSapper( void ) const;							// return true if we are a spy who placed a sapper on a building in the last few moments
	void OnSapperStarted( float flStartTime );
	void OnSapperFinished( float flStartTime );
	bool IsSapping( void ) const;
	int GetSappingEvent( void) const;
	void ClearSappingEvent( void );
	void ClearSappingTracking( void );

	CTFTeam *GetTFTeam( void );
	CTFTeam *GetOpposingTFTeam( void );

	void TeleportEffect( void );
	void RemoveTeleportEffect( void );
	bool HasTheFlag( ETFFlagType exceptionTypes[] = NULL, int nNumExceptions = 0 ) const;
	virtual bool IsAllowedToPickUpFlag( void ) const;

	// Death & Ragdolls.
	virtual void CreateRagdollEntity( void );
	void CreateRagdollEntity( bool bGib, bool bBurning, bool bElectrocuted, bool bOnGround, bool bCloakedCorpse, bool bGoldRagdoll, bool bIceRagdoll, bool bBecomeAsh, int iDamageCustom = 0, bool bCritOnHardHit = false );
	void DestroyRagdoll( void );
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	virtual bool ShouldGib( const CTakeDamageInfo &info ) OVERRIDE;
	bool HasBombinomiconEffectOnDeath( void );
	void StopRagdollDeathAnim( void );
	void SetGibbedOnLastDeath( bool bGibbed ) { m_bGibbedOnLastDeath = bGibbed; }
	bool WasGibbedOnLastDeath( void ) { return m_bGibbedOnLastDeath; }

	// Feign Death
	void SpyDeadRingerDeath( const CTakeDamageInfo& info );
	void FeignDeath( const CTakeDamageInfo& info, bool bDeathnotice );
	void CreateFeignDeathRagdoll( const CTakeDamageInfo& info, bool bGib, bool bBurning, bool bDisguised );

	// Dropping Ammo
	bool ShouldDropAmmoPack( void );
	void DropAmmoPack( const CTakeDamageInfo &info, bool bEmpty, bool bDisguisedWeapon );
	void DropAmmoPackFromProjectile( CBaseEntity *pProjectile );
	void DropExtraAmmo( const CTakeDamageInfo& info, bool bFromDeath = false );
	void DropHealthPack( const CTakeDamageInfo &info, bool bEmpty );
	void DropCurrencyPack( CurrencyRewards_t nSize = TF_CURRENCY_PACK_SMALL, int nAmount = 0, bool bForceDistribute = false, CBasePlayer* pMoneyMaker = NULL );	// Only pass in an amount when nSize = TF_CURRENCY_PACK_CUSTOM
	
	bool CanDisguise( void );
	bool CanDisguise_OnKill( void );
	bool CanGoInvisible( bool bAllowWhileCarryingFlag = false );
	void RemoveInvisibility( void );

	bool CanStartPhase( void );

	void RemoveDisguise( void );

	bool DoClassSpecialSkill( void );
	bool EndClassSpecialSkill( void );

	bool	CanPickupBuilding( CBaseObject *pPickupObject );
	bool TryToPickupBuilding( void );

	float GetLastDamageReceivedTime( void ) { return m_flLastDamageTime; }
	float GetLastEntityDamagedTime( void ) { return m_flLastDamageDoneTime; }
	void SetLastEntityDamagedTime( float flTime ) { m_flLastDamageDoneTime = flTime; }
	CBaseEntity *GetLastEntityDamaged( void ) { return m_hLastDamageDoneEntity; }
	void SetLastEntityDamaged( CBaseEntity *pEnt ) { m_hLastDamageDoneEntity = pEnt; }

	void SetClassMenuOpen( bool bIsOpen );
	bool IsClassMenuOpen( void );

	float GetCritMult( void ) { return m_Shared.GetCritMult(); }
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill, int nVictimPrevHealth ) { m_Shared.RecordDamageEvent(info,bKill,nVictimPrevHealth); }

	bool GetHudClassAutoKill( void ){ return m_bHudClassAutoKill; }
	void SetHudClassAutoKill( bool bAutoKill ){ m_bHudClassAutoKill = bAutoKill; }

	bool GetMedigunAutoHeal( void ){ return m_bMedigunAutoHeal; }
	void SetMedigunAutoHeal( bool bMedigunAutoHeal ){ m_bMedigunAutoHeal = bMedigunAutoHeal; }
	CBaseEntity		*MedicGetHealTarget( void );
	HSCRIPT ScriptGetHealTarget() { return ToHScript( MedicGetHealTarget() ); }
	float			MedicGetChargeLevel( CTFWeaponBase **pRetMedigun = NULL );
	bool IsCallingForMedic( void ) const;			// return true if this player has called for a Medic in the last few seconds
	float GetTimeSinceCalledForMedic( void ) const;
	void NoteMedicCall( void );

	bool ShouldAutoRezoom( void ) { return m_bAutoRezoom; }
	void SetAutoRezoom( bool bAutoRezoom ) { m_bAutoRezoom = bAutoRezoom; }
	bool ShouldAutoReload( void ){ return m_bAutoReload; }
	void SetAutoReload( bool bAutoReload ) { m_bAutoReload = bAutoReload; }

	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );

	virtual bool CanHearAndReadChatFrom( CBasePlayer *pPlayer );
	virtual bool CanBeAutobalanced();

	Vector 	GetClassEyeHeight( void );

	void	UpdateExpression( void );
	void	ClearExpression( void );

	virtual IResponseSystem *GetResponseSystem();
	virtual bool			SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool CanSpeakVoiceCommand( void );
	virtual bool ShouldShowVoiceSubtitleToEnemy( void );
	virtual void NoteSpokeVoiceCommand( const char *pszScenePlayed );
	void	SpeakWeaponFire( int iCustomConcept = MP_CONCEPT_NONE );
	void	ClearWeaponFireScene( void );
	void	FiringTalk() { SpeakWeaponFire(); }

	virtual int DrawDebugTextOverlays( void );

	float m_flNextVoiceCommandTime;
	int m_iVoiceSpamCounter;

	CRateLimitingTokenBucket<CVoiceCommandBucketSizer> m_RateLimitedVoiceCommandTokenBucket;

	float m_flNextSpeakWeaponFire;

	virtual int	CalculateTeamBalanceScore( void );

	bool ShouldAnnounceAchievement( void ) OVERRIDE;
	virtual void OnAchievementEarned( int iAchievement );

	void CheckObserverSettings(); // checks, if target still valid (didn't die etc)

	CTriggerAreaCapture *GetControlPointStandingOn( void );
	CCaptureZone *GetCaptureZoneStandingOn( void );
	CCaptureZone *GetClosestCaptureZone( void );

	// given a vector of points, return the point we can actually travel to the quickest (requires a nav mesh)
	CTeamControlPoint *SelectClosestControlPointByTravelDistance( CUtlVector< CTeamControlPoint * > *pointVector ) const;

	bool CanAirDash( void ) const;

	virtual bool CanBreatheUnderwater() const OVERRIDE;
	bool CanGetWet() const;

	virtual bool IsDeflectable() { return true; }
	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	Added a player type so we can distinguish between bots and humans.
	//=============================================================================
	inline TFPlayerType GetPlayerType(){ return m_playerType; }
	inline void SetPlayerType( TFPlayerType playerType ){ m_playerType = playerType; }
	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );	// invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)

	bool IsThreatAimingTowardMe( CBaseEntity *threat, float cosTolerance = 0.8f ) const;	// return true if the given threat is aiming in our direction
	bool IsThreatFiringAtMe( CBaseEntity *threat ) const;		// return true if the given threat is aiming in our direction and firing its weapon
	bool IsInCombat( void ) const;								// return true if we are engaged in active combat

	void PlayerUse( void );

	void IgnitePlayer();
	void SetCustomModel( const char *pszModel );
	void SetCustomModelWithClassAnimations( const char *pszModel );
	void SetCustomModelOffset( const Vector &offset );
	void SetCustomModelRotation( const QAngle &angle );
	void ClearCustomModelRotation();
	void SetCustomModelRotates( bool bRotates );
	void SetCustomModelVisibleToSelf( bool bVisibleToSelf );
	void SetForcedTauntCam( int nForceTauntCam );
	void ExtinguishPlayerBurning();
	void BleedPlayer( float flBleedingTime );
	void BleedPlayerEx( float flBleedingTime, int nBleedDmg, bool bPermenantBleeding, int nDmgType );
	void RollRareSpell();
	void ClearSpells();

	void InputIgnitePlayer( inputdata_t &inputdata );
	void InputSetCustomModel( inputdata_t &inputdata );
	void InputSetCustomModelWithClassAnimations( inputdata_t &inputdata );
	void InputSetCustomModelOffset( inputdata_t &inputdata );
	void InputSetCustomModelRotation( inputdata_t &inputdata );
	void InputClearCustomModelRotation( inputdata_t &inputdata );
	void InputSetCustomModelRotates( inputdata_t &inputdata );
	void InputSetCustomModelVisibleToSelf( inputdata_t &inputdata );
	void InputSetForcedTauntCam( inputdata_t &inputdata );
	void InputExtinguishPlayer( inputdata_t &inputdata );
	void InputBleedPlayer( inputdata_t &inputdata );
	void InputTriggerLootIslandAchievement( inputdata_t &inputdata );
	void InputTriggerLootIslandAchievement2( inputdata_t &inputdata );
	void InputRollRareSpell( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );

	bool InAirDueToExplosion( void ) { return (!(GetFlags() & FL_ONGROUND) && (GetWaterLevel() == WL_NotInWater) && ( (m_iBlastJumpState != 0) ) || m_Shared.InCond( TF_COND_ROCKETPACK ) ); }
	bool InAirDueToKnockback( void ) { return (!(GetFlags() & FL_ONGROUND) && (GetWaterLevel() == WL_NotInWater) && ( (m_iBlastJumpState != 0) || m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) || m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) || m_Shared.InCond( TF_COND_GRAPPLINGHOOK_SAFEFALL ) ) ); }

	bool IsCoaching() const { return m_bIsCoaching; }
	void SetIsCoaching( bool bIsCoaching );

	void SetCoach( CTFPlayer *pCoach ) { m_hCoach = pCoach; }
	CTFPlayer* GetCoach() const { return m_hCoach; }

	void SetStudent( CTFPlayer *pStudent ) { m_hStudent = pStudent; }
	CTFPlayer* GetStudent() const { return m_hStudent; }

	void DoNoiseMaker(); // Halloween event item support.

	bool IsWormsGearEquipped( void ) const;
	bool IsRobotCostumeEquipped( void ) const;
	bool IsDemowolf( void ) const;
	bool IsFrankenHeavy( void ) const;
	bool IsFairyHeavy( void ) const;
	bool IsZombieCostumeEquipped( void ) const;
	bool HasWearablesEquipped( const CSchemaItemDefHandle *ppItemDefs, int nWearables ) const;

	CEconItemView *GetEquippedItemForLoadoutSlot( int iLoadoutSlot ){ return m_Inventory.GetInventoryItemByItemID( m_EquippedLoadoutItemIndices[iLoadoutSlot] ); }
	CBaseEntity *GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable = false );			//Gets whatever entity is associated with the loadout slot (wearable or weapon)
	CTFWearable *GetEquippedWearableForLoadoutSlot( int iLoadoutSlot );

	//Base entity overrides
	// Functions that intercept Base Calls for Attribute Checking
	void ApplyAbsVelocityImpulse ( const Vector &vecImpulse );
	bool ApplyPunchImpulseX ( float flImpulse );
	void ApplyGenericPushbackImpulse( const Vector &vecImpulse, CTFPlayer *pAttacker );

	void SetUseBossHealthBar( bool bUseBossHealthBar ) { m_bUseBossHealthBar = bUseBossHealthBar; }

	void SetUsingVRHeadset( bool bState ){ m_bUsingVRHeadset = bState; }

	static bool m_bTFPlayerNeedsPrecache;

	// IHasAttributes
	CAttributeManager		*GetAttributeManager( void ) { return &m_AttributeManager; }
	CAttributeContainer		*GetAttributeContainer( void ) { return NULL; }
	CBaseEntity				*GetAttributeOwner( void ) { return NULL; }
	CAttributeList			*GetAttributeList( void ) { return &m_AttributeList; }
	virtual void			ReapplyProvision( void ) { return; }

	CVoteController 		*GetTeamVoteController() OVERRIDE;

protected:
	CNetworkVarEmbedded( CAttributeContainerPlayer, m_AttributeManager );

	//----------------------------
	// INVENTORY MANAGEMENT
public:
	// IInventoryUpdateListener
	virtual void InventoryUpdated( CPlayerInventory *pInventory );

	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner ) { m_Shared.SetLoadoutUnavailable( true ); }

	// Inventory access
	CTFPlayerInventory	*Inventory( void ) { return &m_Inventory; }

private:
	CTFPlayerInventory			m_Inventory;
	// Items that have been equipped on this player instance (the inventory loadout may have changed)
	itemid_t				m_EquippedLoadoutItemIndices[CLASS_LOADOUT_POSITION_COUNT];

public:
	void		UpdateInventory( bool bInit );
	void		VerifySOCache();

	CNetworkVarEmbedded( CTFPlayerShared, m_Shared );
	friend class CTFPlayerShared;

	int m_flNextTimeCheck;		// Next time the player can execute a "timeleft" command

	CNetworkVar( bool, m_bSaveMeParity );
	
	CNetworkVar( bool, m_bIsCoaching);
	CNetworkHandle( CTFPlayer, m_hCoach );
	CNetworkHandle( CTFPlayer, m_hStudent );
	float	m_flLastCoachCommand;

	CNetworkVar( bool, m_bIsABot );
	CNetworkVar( int, m_nBotSkill );

	int					StateGet( void ) const;

	void				SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void				HolsterOffHandWeapon( void );
	CTFWeaponBase*		GetOffHandWeapon( void ) { return m_hOffHandWeapon; }

	float				GetSpawnTime() { return m_flSpawnTime; }

	
	virtual void 		SelectItem( const char *pstr, int iSubType = 0 ) OVERRIDE;
	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 ) OVERRIDE;
	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) OVERRIDE;

	virtual void		OnMyWeaponFired( CBaseCombatWeapon *weapon );	// call this when this player fires a weapon to allow other systems to react

	bool				ItemsMatch( TFPlayerClassData_t *pData, CEconItemView *pCurWeaponItem, CEconItemView *pNewWeaponItem, CTFWeaponBase *pWpnEntity = NULL );
	void				ManageRegularWeapons( TFPlayerClassData_t *pData );
	void				ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData );	// Older, pre-inventory method of managing regular weapons
	void				ManageBuilderWeapons( TFPlayerClassData_t *pData );
	virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0, const CEconItemView *pScriptItem = NULL, bool bForce = false );
	void				PostInventoryApplication( void );
	bool				ItemIsAllowed( CEconItemView *pItem );
	void				RemovePlayerAttributes( bool bSetBonuses );
	void				ApplySetBonuses( void );
	void				GetActiveSets( CUtlVector<const CEconItemSetDefinition *> *pItemSets );
	void				ValidateWeapons(  TFPlayerClassData_t *pData, bool bResetWeapons );
	void				ValidateWearables( TFPlayerClassData_t *pData );
	CEconItemView* GetLoadoutItem( int iClass, int iSlot, bool bReportWhitelistFails = false );
	void				UseActionSlotItemPressed( void );
	void				UseActionSlotItemReleased( void );

	bool				IsFireproof( void ) const;
	bool				AddToSpyKnife( float value, bool force );

private:
	void				GetHorriblyHackedRailgunPosition( const Vector& vStart, Vector *out_pvStartPos );
	void				MaybeDrawRailgunBeam( IRecipientFilter *pFilter, CTFWeaponBase *pWeapon, const Vector& vStartPos, const Vector& vEndPos );

// Taunts
public:
	bool				IsReadyToTauntWithPartner( void ) const { return m_bIsReadyToHighFive; }
	CTFPlayer *			GetTauntPartner( void )		{ return m_hHighFivePartner; }
	float				GetTauntYaw( void )				{ return m_flTauntYaw; }
	float				GetPrevTauntYaw( void )		{ return m_flPrevTauntYaw; }
	void				SetTauntYaw( float flTauntYaw );
	CTFPlayer *			FindPartnerTauntInitiator( void );
	void				AcceptTauntWithPartner( CTFPlayer *initiator );
	void				MimicTauntFromPartner( CTFPlayer *initiator );
	bool				CanMoveDuringTaunt();
	bool				ShouldStopTaunting();
	bool				IsTauntInitiator() const { return m_bIsTauntInitiator; }
	bool				IsTauntForceMovingForward() const { return m_bTauntForceMoveForward; }
	float				GetTauntMoveAcceleration() const { return m_flTauntMoveAccelerationTime; }
	float				GetTauntMoveSpeed() const { return m_flTauntForceMoveForwardSpeed; }
	float				GetTauntTurnAccelerationTime() const { return m_flTauntTurnAccelerationTime; }
	virtual int			GetAllowedTauntPartnerTeam() const;
	CEconItemView		*GetTauntEconItemView() { return m_TauntEconItemView.IsValid() ? &m_TauntEconItemView : NULL; }

	int					GetTauntConcept( CEconItemDefinition *pItemDef );
	bool				PlayTauntSceneFromItem( const CEconItemView *pEconItemView );
	
	void				OnTauntSucceeded( const char* pszSceneName, int iTauntIndex = 0, int iTauntConcept = 0 );
	void				Taunt( taunts_t iTauntIndex = TAUNT_BASE_WEAPON, int iTauntConcept = 0 );
	void				ScriptTaunt( int iTauntIndex, int iTauntConcept ) { Taunt((taunts_t)iTauntIndex, iTauntConcept); }
	bool				IsTaunting( void ) const { return m_Shared.InCond( TF_COND_TAUNTING ); }
	void				DoTauntAttack( void );
	bool				IsAllowedToTaunt( void );
	bool				FindOpenTauntPartnerPosition( const CEconItemView *pEconItemView, Vector &position, float *flTolerance );
	bool				IsAllowedToInitiateTauntWithPartner( const CEconItemView *pEconItemView, char *pszErrorMessage = NULL, int cubErrorMessage = 0 );
	void				CancelTaunt( void );
	void				StopTaunt( bool bForceRemoveProp = true );
	void				EndLongTaunt();
	float				GetTauntRemoveTime( void ) const { return m_flTauntRemoveTime; }
	bool				IsAllowedToRemoveTaunt() const { return m_bAllowedToRemoveTaunt; }
	void				HandleTauntCommand( int iTauntSlot = 0 );
	QAngle				m_angTauntCamera;

	CHandle< CBaseEntity > m_hTauntItem;

	void				ClearTauntAttack();
	float				GetTauntAttackTime() const { return m_flTauntAttackTime; }

	void				SetRPSResult( int iRPSResult ) { m_iTauntRPSResult = iRPSResult; }

	void				HandleWeaponSlotAfterTaunt();

	float				GetCurrentTauntMoveSpeed() const { return m_flCurrentTauntMoveSpeed; }
	void				SetCurrentTauntMoveSpeed( float flSpeed ) { m_flCurrentTauntMoveSpeed = flSpeed; }

	float				GetVehicleReverseTime() const { return m_flVehicleReverseTime; }
	void				SetVehicleReverseTime( float flTime ) { m_flVehicleReverseTime = flTime; }

	bool				IsViewingCYOAPDA( void ) const { return m_bViewingCYOAPDA; }
	bool				IsRegenerating( void ) const { return m_bRegenerating; }

	HSCRIPT				ScriptGetActiveWeapon( void ) { return ToHScript( GetActiveTFWeapon() ); }

	void				ScriptAddCond( int nCond );
	void				ScriptAddCondEx( int nCond, float flDuration, HSCRIPT hProvider );
	void				ScriptRemoveCond( int nCond );
	void				ScriptRemoveCondEx( int nCond, bool bIgnoreDuration );
	bool				ScriptInCond( int nCond );
	bool				ScriptWasInCond( int nCond );
	void				ScriptRemoveAllCond();
	float				ScriptGetCondDuration( int nCond );
	void				ScriptSetCondDuration( int nCond, float flNewDuration );
	HSCRIPT				ScriptGetDisguiseTarget();

	bool				ScriptIsCarryingRune()						{ return m_Shared.IsCarryingRune(); }
	bool				ScriptIsCritBoosted( void ) const			{ return m_Shared.IsCritBoosted(); }
	bool				ScriptIsInvulnerable( void ) const			{ return m_Shared.IsInvulnerable(); }
	bool				ScriptIsStealthed( void ) const				{ return m_Shared.IsStealthed(); }
	bool				ScriptCanBeDebuffed( void ) const			{ return m_Shared.CanBeDebuffed(); }
	bool				ScriptIsImmuneToPushback( void ) const		{ return m_Shared.IsImmuneToPushback(); }
	int					ScriptGetDisguiseAmmoCount()				{ return m_Shared.GetDisguiseAmmoCount(); }
	void				ScriptSetDisguiseAmmoCount( int nValue )	{ return m_Shared.SetDisguiseAmmoCount(nValue); }
	int					ScriptGetDisguiseTeam()						{ return m_Shared.GetDisguiseTeam(); }
	bool				ScriptIsFullyInvisible()					{ return m_Shared.IsFullyInvisible(); }
	float				ScriptGetSpyCloakMeter()					{ return m_Shared.GetSpyCloakMeter(); }
	void				ScriptSetSpyCloakMeter( float flValue )		{ return m_Shared.SetSpyCloakMeter( flValue ); }
	bool				ScriptIsRageDraining()						{ return m_Shared.IsRageDraining(); }
	float				ScriptGetRageMeter()						{ return m_Shared.GetRageMeter(); }
	void				ScriptSetRageMeter( float flValue )			{ return m_Shared.SetRageMeter( flValue ); }
	float				ScriptGetScoutHypeMeter()					{ return m_Shared.GetScoutHypeMeter(); }
	void				ScriptSetScoutHypeMeter( float flValue )	{ return m_Shared.SetScoutHypeMeter( flValue ); }
	bool				ScriptIsHypeBuffed()						{ return m_Shared.IsHypeBuffed(); }
	bool				ScriptIsJumping()							{ return m_Shared.IsJumping(); }
	bool				ScriptIsAirDashing()						{ return m_Shared.IsAirDashing(); }
	bool				ScriptIsControlStunned()					{ return m_Shared.IsControlStunned(); }
	bool				ScriptIsSnared()							{ return m_Shared.IsSnared(); }
	int					ScriptGetCaptures() const					{ return m_Shared.GetCaptures( 0 ); }
	int					ScriptGetDefenses() const					{ return m_Shared.GetDefenses( 0 ); }
	int					ScriptGetDominations() const				{ return m_Shared.GetDominations( 0 ); }
	int					ScriptGetRevenge() const					{ return m_Shared.GetRevenge( 0 ); }
	int					ScriptGetBuildingsDestroyed() const			{ return m_Shared.GetBuildingsDestroyed( 0 ); }
	int					ScriptGetHeadshots() const					{ return m_Shared.GetHeadshots( 0 ); }
	int					ScriptGetBackstabs() const					{ return m_Shared.GetBackstabs( 0 ); }
	int					ScriptGetHealPoints() const					{ return m_Shared.GetHealPoints( 0 ); }
	int					ScriptGetInvulns() const					{ return m_Shared.GetInvulns( 0 ); }
	int					ScriptGetTeleports() const					{ return m_Shared.GetTeleports( 0 ); }
	int					ScriptGetResupplyPoints() const				{ return m_Shared.GetResupplyPoints( 0 ); }
	int					ScriptGetKillAssists() const				{ return m_Shared.GetKillAssists( 0 ); }
	int					ScriptGetBonusPoints() const				{ return m_Shared.GetBonusPoints( 0 ); }
	void				ScriptResetScores()							{ m_Shared.ResetScores(); }
	bool				ScriptIsParachuteEquipped()					{ return m_Shared.IsParachuteEquipped(); }

	int					ScriptGetPlayerClass()
	{
		return GetPlayerClass()->GetClassIndex();
	}

	void				ScriptSetPlayerClass( int iClass )
	{
		GetPlayerClass()->Init( iClass );

		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( i );
			if ( pWeapon )
			{
				pWeapon->OnOwnerClassChange();
			}
		}
	}

	void				ScriptRemoveAllItems( bool bRemoveSuit )
	{
		RemoveAllItems( bRemoveSuit );
	}

	Vector	ScriptWeapon_ShootPosition();
	bool	ScriptWeapon_CanUse( HSCRIPT hWeapon );
	void	ScriptWeapon_Equip( HSCRIPT hWeapon );
	void	ScriptWeapon_Drop( HSCRIPT hWeapon );
	void	ScriptWeapon_DropEx( HSCRIPT hWeapon, Vector vecTarget, Vector vecVelocity );
	void	ScriptWeapon_Switch( HSCRIPT hWeapon );
	void	ScriptWeapon_SetLast( HSCRIPT hWeapon );
	HSCRIPT	ScriptGetLastWeapon();
	void ScriptEquipWearableViewModel( HSCRIPT hWearableViewModel );
	bool ScriptIsFakeClient() const { return this->IsFakeClient(); }
	int ScriptGetBotType() const { return this->GetBotType(); }
	bool ScriptIsBotOfType(int nType) const { return this->IsBotOfType(nType); }

	void ScriptStunPlayer( float flTime, float flReductionAmount, int iStunFlags = TF_STUN_MOVEMENT, HSCRIPT hAttacker = NULL );

private:
	void				GetReadyToTauntWithPartner( void );
	void				CancelTauntWithPartner( void );
	void				StopTauntSoundLoop();
	float				PlayTauntOutroScene();
	float				PlayTauntRemapInputScene();
	void				ParseSharedTauntDataFromEconItemView( const CEconItemView *pEconItemView );

	CNetworkVar( bool, m_bAllowMoveDuringTaunt );
	CNetworkVar( bool, m_bIsReadyToHighFive );
	CNetworkHandle( CTFPlayer, m_hHighFivePartner );
	CNetworkVar( int, m_nForceTauntCam );
	CNetworkVar( float, m_flTauntYaw );
	CNetworkVar( int, m_nActiveTauntSlot );
	CNetworkVar( item_definition_index_t, m_iTauntItemDefIndex );
	CNetworkVar( float, m_flCurrentTauntMoveSpeed );
	CNetworkVar( float, m_flVehicleReverseTime );

	bool				m_bTauntForceMoveForward;
	float				m_flTauntForceMoveForwardSpeed;
	float				m_flTauntMoveAccelerationTime;
	float				m_flTauntTurnSpeed;
	float				m_flTauntTurnAccelerationTime;

	float				m_flPrevTauntYaw;
	EHANDLE				m_hTauntScene;
	CHandle< CTFTauntProp >	m_hTauntProp;
	bool				m_bInitTaunt;
	bool				m_bTauntMimic;
	bool				m_bIsTauntInitiator;
	float				m_flTauntSoundTime;
	CUtlString			m_strTauntSoundName;
	float				m_flTauntSoundLoopTime;
	CUtlString			m_strTauntSoundLoopName;
	CEconItemView		m_TauntEconItemView;

	enum TauntStage_t
	{
		TAUNT_NONE = 0,
		TAUNT_INTRO,
		TAUNT_OUTRO
	} m_TauntStage;

	bool				m_bAllowedToRemoveTaunt;
	float				m_flTauntStartTime;
	float				m_flTauntNextStartTime;
	float				m_flTauntRemoveTime;
	float				m_flTauntOutroTime;
	Vector				m_vecTauntStartPosition;

	float				m_flNextAllowTauntRemapInputTime;
	float				m_flTauntAttackTime;
	float				m_flTauntInhaleTime;
	taunt_attack_t		m_iTauntAttack;
	int					m_iTauntAttackCount;
	int					m_iTauntRPSResult;
	int					m_iPreTauntWeaponSlot;
	int					m_iPreTauntFOV;

	float				m_flNextReflectZap;

public:
	virtual int			GetSpecialDSP( void );

	virtual float		PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	void				SetDeathFlags( int iDeathFlags ) { m_iDeathFlags = iDeathFlags; }
	int					GetDeathFlags() { return m_iDeathFlags; }
	void				SetMaxSentryKills( int iMaxSentryKills ) { m_iMaxSentryKills = iMaxSentryKills; }
	int					GetMaxSentryKills() { return m_iMaxSentryKills; }

	CNetworkVar( bool, m_iSpawnCounter );
	
	void				CheckForIdle( void );
	inline bool			IsAwayFromKeyboard( void ) { return m_bIsAFK; }
	
	void				PickWelcomeObserverPoint();

	virtual	bool		ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );

	void				StopRandomExpressions( void ) { m_flNextRandomExpressionTime = -1; }
	void				StartRandomExpressions( void ) { m_flNextRandomExpressionTime = gpGlobals->curtime; }

	virtual bool			WantsLagCompensationOnEntity( const CBasePlayer	*pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	CTFWeaponBase		*Weapon_OwnsThisID( int iWeaponID ) const;
	CTFWeaponBase		*Weapon_GetWeaponByType( int iType );

	virtual void		PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );

	medigun_charge_types	GetChargeEffectBeingProvided( void );

	// Achievements
	void				AwardAchievement( int iAchievement, int iCount = 1 );
	void				HandleAchievement_Medic_AssistHeavy( CTFPlayer *pPunchVictim );
	void				HandleAchievement_Pyro_BurnFromBehind( CTFPlayer *pBurner );

	void				ClearPunchVictims( void ) { m_aPunchVictims.RemoveAll(); }
	void				ClearBurnFromBehindAttackers( void ) { m_aBurnFromBackAttackers.RemoveAll(); }

	int					RocketJumped( void ) { return m_iBlastJumpState & TF_PLAYER_ROCKET_JUMPED; }
	int					StickyJumped( void ) { return m_iBlastJumpState & TF_PLAYER_STICKY_JUMPED; }
	void				SetBlastJumpState( int iState, bool bPlaySound = false );
	void				ClearBlastJumpState( void );

	int					GetPreviousTeam( void ) { return m_iPreviousteam; }
	bool				IsArenaSpectator( void ) { return m_bArenaSpectator; }

	float				GetTeamJoinTime( void ) { return m_flTeamJoinTime; }
	void				MarkTeamJoinTime( void ) { m_flTeamJoinTime = gpGlobals->curtime; }
	void				PlayerJustPlayed( bool bPlayed ) { m_bJustPlayed = bPlayed; }
	bool				DidPlayerJustPlay( void ) { return m_bJustPlayed; }

	bool				IsCapturingPoint( void );

	bool				m_bSuicideExplode;

	int					m_iOldStunFlags;

	CNetworkVar( bool, m_bFlipViewModels );
	int					m_iBlastJumpState;
	float				m_flBlastJumpLandTime;
	bool				m_bTakenBlastDamageSinceLastMovement;

	void				SetTargetDummy( void ){ m_bIsTargetDummy = true; }

	bool				ShouldCollideWithSentry( void ){ return m_bCollideWithSentry; }
	bool				IsAnyEnemySentryAbleToAttackMe( void ) const;		// return true if any enemy sentry has LOS and is facing me and is in range to attack

	int					GetHealthBefore( void ) { return m_iHealthBefore; }

	int					GetAutoTeam( int nPreferedTeam = TF_TEAM_AUTOASSIGN );
	bool				ShouldForceAutoTeam( void );

	float				m_flCommentOnCarrying;

	int					GetTeamChangeCount( void ) { return m_iTeamChanges; }
	int					GetClassChangeCount( void ) { return m_iClassChanges; }

	void				IncrementKillCountSinceLastDeploy( const CTakeDamageInfo &info );

	void				ForceItemRemovalOnRespawn( void ) { m_bForceItemRemovalOnRespawn = true; }

	// Item Testing
public:
	void				ItemTesting_Start( KeyValues *pKV );
	void				ItemTesting_UpdateBots( KeyValues *pKV );
	CEconItemView	*ItemTesting_GetTestItem( int iClass, int iSlot );
	void				ItemTesting_DeleteItems();

public:
	struct itemtest_t
	{
		KeyValues			*pKV;
		CEconItemView	scriptItem;
	};
	CUtlVector<itemtest_t>	m_ItemsToTest;
	bool					m_bItemTestingRespawn;

	bool				IsMissionEnemy( void ){ return m_bIsMissionEnemy; }
	void 				MarkAsMissionEnemy( void ){ m_bIsMissionEnemy = true; }
	bool				IsSupportEnemy( void ){ return m_bIsSupportEnemy; }
	void 				MarkAsSupportEnemy( void ){ m_bIsSupportEnemy = true; }
	void 				MarkAsLimitedSupportEnemy( void ){ m_bIsLimitedSupportEnemy = true; }

	// In-game currency
	int					GetCurrency( void ) const { return m_nCurrency; }
	void				SetCurrency( int nAmount ){ m_nCurrency = nAmount; }
	void				AddCurrency( int nAmount );
	void				RemoveCurrency( int nAmount );

	// Set the amount of money this bot is worth when killed. We re-use m_nCurrency, because bots don't collect currency.
	void				SetCustomCurrencyWorth( int nAmount )	{ m_nCurrency = nAmount; }

	// Bounty Mode
	int					GetExperienceLevel( void ) { return m_nExperienceLevel; }
	void				SetExperienceLevel( int nValue ) { m_nExperienceLevel.Set( MAX( nValue, 1 ) ); }
	int					GetExperiencePoints( void ) { return m_nExperiencePoints; }
	void				SetExperiencePoints( int nValue ) { m_nExperiencePoints = MAX( nValue, 0 ); }
	void				AddExperiencePoints( int nValue, bool bGiveCurrency = false, CTFPlayer *pSource = NULL );
	void				CalculateExperienceLevel( bool bAnnounce = true );
	void				RefundExperiencePoints( void );

	void				RememberUpgrade( int iPlayerClass, CEconItemView *pItem, int iUpgrade, int nCost, bool bDowngrade = false );	// store this upgrade for restoring at a checkpoint
	void				ForgetFirstUpgradeForItem( CEconItemView *pItem );						// erase the first upgrade stored for this item (for powerup bottles)
	void				ClearUpgradeHistory( void );
	void				ReapplyItemUpgrades ( CEconItemView *pItem );
	void				ReapplyPlayerUpgrades ( void );
	void				SetWaveSpawnPopulator( CWaveSpawnPopulator *pWave ){ m_pWaveSpawnPopulator = pWave; }
	CUtlVector< CUpgradeInfo >* GetRefundableUpgrades( void ) { return &m_RefundableUpgrades; }
	void				ResetRefundableUpgrades( void ) { m_RefundableUpgrades.RemoveAll(); }
	void				BeginPurchasableUpgrades( void );
	void				EndPurchasableUpgrades( void );
	bool				CanPurchaseUpgrades( void ) const { Assert( m_nCanPurchaseUpgradesCount >= 0 ); return m_nCanPurchaseUpgradesCount > 0; }

	void				PlayReadySound( void );

	void				AccumulateSentryGunDamageDealt( float damage );
	void				ResetAccumulatedSentryGunDamageDealt();
	float				GetAccumulatedSentryGunDamageDealt();

	void				IncrementSentryGunKillCount( void );
	void				ResetAccumulatedSentryGunKillCount();
	int					GetAccumulatedSentryGunKillCount();

	bool				PlaySpecificSequence( const char *pSequenceName );

	void				SetWaterExitTime( float flTime ){ m_flWaterExitTime = flTime; }
	float				GetWaterExitTime( void ){ return m_flWaterExitTime; }

	void				MerasmusPlayerBombExplode( bool bExcludeMe = true );

	void				DropDeathCallingCard( CTFPlayer* pTFAttacker, CTFPlayer* pTFVictim );


	//---------------------------------
	// support entity IO for forcing speech concepts
	void InputSpeakResponseConcept( inputdata_t &inputdata );

	//---------------------------------

	float				GetTimeSinceLastThink( void ) const { return ( m_flLastThinkTime >= 0.f ) ? gpGlobals->curtime - m_flLastThinkTime : -1.f; }
	float				GetRespawnTimeOverride( void ) const { return m_flRespawnTimeOverride; }
	const char			*GetRespawnLocationOverride( void ) const { return ( m_strRespawnLocationOverride == NULL_STRING ) ? NULL : m_strRespawnLocationOverride.ToCStr(); }
	void				SetRespawnOverride( float flRespawnTime, string_t respawnLocation ) { m_flRespawnTimeOverride = flRespawnTime; m_strRespawnLocationOverride = respawnLocation; }
	void				ResetIdleCheck( void ) { m_flLastAction = gpGlobals->curtime; }

	// Matchmaking
	void				SetMatchSafeToLeave( bool bMatchSafeToLeave ) { m_bMatchSafeToLeave = bMatchSafeToLeave; }

	void				SetPrevRoundTeamNum( int nTeamNum ){ m_nPrevRoundTeamNum = nTeamNum; }
	int					GetPrevRoundTeamNum( void ){ return m_nPrevRoundTeamNum; }

	// Talk control
	virtual bool		CanPlayerTalk() OVERRIDE;

protected:

	// Creation/Destruction.
	virtual void		InitClass( void );
	void				GiveDefaultItems();
	bool				SelectSpawnSpotByType( const char *pEntClassName, CBaseEntity* &pSpot );	// "info_player_teamspawn"
	bool				SelectSpawnSpotByName( const char *pEntName, CBaseEntity* &pSpot );			// named info_player_teamspawn, i.e. "my_blue_offense_respawns"
	void				RemoveNemesisRelationships();
	void				RemoveAllItems();

	// Think.
	void				TFPlayerThink();
	void				UpdateTimers( void );
	void				PostSpawnThink( void );

	// Regeneration due to being a Medic, or derived from items
	void				RegenThink();
	void				RuneRegenThink();
	void				RegenAmmoInternal( int iAmmo, float flRegen );
	void				ResetPlayerClass( void );

	virtual void		Internal_HandleMapEvent( inputdata_t &inputdata ) OVERRIDE;

private:
	float				m_flAccumulatedHealthRegen;	// Regeneration can be in small amounts, so we accumulate it and apply when it's > 1
	float				m_flNextAmmoRegenAt;
	float				m_flLastHealthRegenAt;
	float				m_flAccumulatedRuneHealthRegen;
	float				m_flNextRuneAmmoRegenAt;
	float				m_flLastRuneHealthRegenAt;
	float				m_flAccumulatedAmmoRegens[TF_AMMO_SECONDARY+1];	// Only support regenerating primary & secondary right now

	// Bots.
	friend void			Bot_Think( CTFPlayer *pBot );

	// Physics.
	void				PhysObjectSleep();
	void				PhysObjectWake();

	// Ammo pack.
	bool CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles );
	void AmmoPackCleanUp( void );

	// State.
	CPlayerStateInfo	*StateLookupInfo( int nState );
	void				StateEnter( int nState );
	void				StateLeave( void );
	void				StateTransition( int nState );
	void				StateEnterWELCOME( void );
	void				StateThinkWELCOME( void );
	void				StateEnterPICKINGTEAM( void );
	void				StateEnterACTIVE( void );
	void				StateEnterOBSERVER( void );
	void				StateThinkOBSERVER( void );
	void				StateEnterDYING( void );
	void				StateThinkDYING( void );

	virtual bool		SetObserverMode(int mode);
	virtual void		AttemptToExitFreezeCam( void );

	bool				PlayGesture( const char *pGestureName );

	bool				GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes );

public:
	const QAngle& GetNetworkEyeAngles() const { return m_angEyeAngles; }

	// Achievement data storage
	CAchievementData	m_AchievementData;
	CTFPlayerAnimState	*m_PlayerAnimState;

private:
	// Map introductions
	int					m_iIntroStep;
	CHandle<CIntroViewpoint> m_hIntroView;
	float				m_flIntroShowHintAt;
	float				m_flIntroShowEventAt;
	bool				m_bHintShown;
	bool				m_bAbortFreezeCam;
	bool				m_bSeenRoundInfo;
	CNetworkVar( bool, m_bRegenerating );
	bool				m_bRespawning;

	// Items.
	CNetworkHandle( CTFItem, m_hItem );

	// Combat.
	CNetworkHandle( CTFWeaponBase, m_hOffHandWeapon );

	float					m_flHealthBuffTime;
	int						m_iHealthBefore;

	float					m_flNextRegenerateTime;
	float					m_flNextChangeClassTime;
	float					m_flNextChangeTeamTime;
	bool					m_bAllSeeCrit;
	bool					m_bMiniCrit;
	bool					m_bShowDisguisedCrit;
	EAttackBonusEffects_t	m_eBonusAttackEffect;

	int						m_iTeamChanges;
	int						m_iClassChanges;

	// Ragdolls.
	Vector					m_vecTotalBulletForce;

	// State.
	CPlayerStateInfo		*m_pStateInfo;

	// Spawn Point
	CTFTeamSpawn			*m_pSpawnPoint;

	// Networked.
	CNetworkQAngle( m_angEyeAngles );					// Copied from EyeAngles() so we can send it to the client.

	CTFPlayerClass		m_PlayerClass;
	int					m_iLastWeaponFireUsercmd;				// Firing a weapon.  Last usercmd we shot a bullet on.
	int					m_iLastWeaponSlot;				            // To save last switch between lives
	int					m_iLastSkin;
	float				m_flLastDamageTime;
	CNetworkVar( float, m_flMvMLastDamageTime );
	float				m_flLastDamageDoneTime;
	CHandle< CBaseEntity > m_hLastDamageDoneEntity;
	float				m_flLastHealedTime;
	float				m_flNextPainSoundTime;
	int					m_LastDamageType;
	int					m_iDeathFlags;				// TF_DEATH_* flags with additional death info
	int					m_iMaxSentryKills;			// most kills by a single sentry
	int					m_iNumberofDominations;		// number of active dominations for this player

	bool				m_bPlayedFreezeCamSound;
	bool				m_bSwitchedClass;
	bool				m_bRememberLastWeapon;
	bool				m_bRememberActiveWeapon;
	int					m_iActiveWeaponTypePriorToDeath;

	CHandle< CTFWeaponBuilder > m_hWeaponBuilder;

	CUtlVector< CHandle< CBaseObject > >	m_aObjects;			// List of player objects

	bool m_bIsClassMenuOpen;

	Vector m_vecLastDeathPosition;

	float				m_flSpawnTime;

	float				m_flLastAction;
	float				m_flTimeInSpawn;

	CUtlVector<EHANDLE>	m_hObservableEntities;
	CUtlVector<float>	m_aBurnOtherTimes;					// vector of times this player has burned others

	bool m_bHudClassAutoKill;

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	EHANDLE				m_hFireWeaponScenes[ MAX_FIRE_WEAPON_SCENES ];
	int					m_nNextFireWeaponScene = 0;
	float				m_flNextRandomExpressionTime;

	bool				m_bSpeakingConceptAsDisguisedSpy;

	bool 				m_bMedigunAutoHeal;
	bool				m_bAutoRezoom;	// does the player want to re-zoom after each shot for sniper rifles
	bool				m_bAutoReload;

	bool				m_bForceItemRemovalOnRespawn;

	int					m_nPrevRoundTeamNum;

public:
	bool				IsGoingFeignDeath( void ) { return m_bGoingFeignDeath; }

	void					SetDeployingBombState( BombDeployingState_t nDeployingBombState ) { m_nDeployingBombState = nDeployingBombState; }
	BombDeployingState_t	GetDeployingBombState( void ) const { return m_nDeployingBombState; }

	void				SetPendingMerasmusPlayerBombExplode( void ){ m_bPendingMerasmusPlayerBombExplode = true; }

private:
	// Achievement data
	CUtlVector<EHANDLE> m_aPunchVictims;
	CUtlVector<EHANDLE> m_aBurnFromBackAttackers;
	int					m_iLeftGroundHealth;	// health we were at the last time we left the ground

	float				m_flTeamJoinTime;
	bool				m_bCreatedRocketJumpParticles;
	bool				m_bJustPlayed;
	int					m_iPreviousteam;
	bool				m_bGibbedOnLastDeath;
	CUtlMap<int, float> m_Cappers;		
	float				m_fMaxHealthTime;

	// Feign death.
	bool				m_bGoingFeignDeath;
	CHandle<CBaseEntity> m_hFeignRagdoll;	// Don't use the normal ragdoll.
	Vector				m_vecFeignDeathVelocity;

	CNetworkVar( bool, m_bArenaSpectator );

	bool				m_bArenaIsAFK; // used to detect when players are AFK during an Arena-mode round
	bool				m_bIsAFK;

	BombDeployingState_t	m_nDeployingBombState;

	bool				m_bIsMissionEnemy;
	bool				m_bIsSupportEnemy;
	bool				m_bIsLimitedSupportEnemy;

	// In-game currency
	CNetworkVar( int, m_nCurrency );
	CNetworkVar( bool, m_bIsMiniBoss );

	// Bounty Mode
	CNetworkVar( uint32, m_nExperienceLevel );
	CNetworkVar( uint32, m_nExperienceLevelProgress );	// Networked progress bar
	uint32				m_nExperiencePoints;			// Raw player-only value

	// Matchmaking
	// is this player bound to the match on penalty of abandon. Sync'd via local-player-only DT
	CNetworkVar( bool, m_bMatchSafeToLeave );

	CWaveSpawnPopulator *m_pWaveSpawnPopulator;
	float				m_flLastReadySoundTime;

	int						m_nCanPurchaseUpgradesCount;
	CUtlVector< CUpgradeInfo >	m_RefundableUpgrades;

	CUtlVector< CUpgradeInfo > * GetPlayerUpgradeHistory( void );
	CUtlVector< CUpgradeInfo >  m_LocalUpgradeHistory;

public:
	void GrantOrRemoveAllUpgrades( bool bRemove, bool bRefund );

	// Marking for death.
	CHandle<CTFPlayer>	m_pMarkedForDeathTarget;

	CountdownTimer m_playerMovementStuckTimer;			// for destroying stuck bots in MvM

	QAngle				m_qPreviousChargeEyeAngle;		// Previous EyeAngles to compute deltas for legal mouse movement
private:

	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	Added a player type so we can distinguish between bots and humans.
	//=============================================================================
	TFPlayerType m_playerType;
	//=============================================================================
	// HPE_END
	//=============================================================================
	
	bool				m_bIsTargetDummy;

	bool				m_bCollideWithSentry;
	IntervalTimer		m_calledForMedicTimer;
	CountdownTimer		m_placedSapperTimer;

	CountdownTimer		m_inCombatThrottleTimer;

	mutable char		m_bIsCalculatingMaximumSpeed;

public:

	float				GetDesiredHeadScale() const;
	float				GetHeadScaleSpeed() const;
	float				GetDesiredTorsoScale() const;
	float				GetTorsoScaleSpeed() const;
	float				GetDesiredHandScale() const;
	float				GetHandScaleSpeed() const;

	bool				IsInPurgatory( void ) const;
	bool				HasPurgatoryBuff( void ) const;

	void				SetBombHeadTimestamp();
	float				GetTimeSinceWasBombHead() const;
	
	float				GetKartSpeedBoost( void );
	float				GetKartHealth( void )				{ return m_iKartHealth; }
	void				AddKartDamage( int iDamage )		{ m_iKartHealth = Max(0, m_iKartHealth + iDamage); }
	float				GetKartKnockbackMultiplier( float flExtraMultiplier ) const;
	
	void				ResetKartDamage();
	CBaseEntity			*GetKartBombHeadTarget() const { return m_hKartBombHeadTarget; }
	void				SetKartBombHeadTarget( CBaseEntity* pEnt ) { m_hKartBombHeadTarget = pEnt; }

	void				AddHalloweenKartPushEvent( CTFPlayer *pOther, CBaseEntity *pInflictor, CBaseEntity *pWeapon, Vector vForce, int iDamage, int iDamageType = 0 );
	QAngle				GetAnimRenderAngles( void ) { return m_PlayerAnimState->GetRenderAngles(); }

	void				CancelEurekaTeleport();

	
	CNetworkVar( int,	m_iKartState );
	CNetworkVar( float, m_flKartNextAvailableBoost );
	float				m_flHHHKartAttackTime;

	// Wrenchmotron teleport
	bool				m_bIsTeleportingUsingEurekaEffect;

private:
	void				UpdateHalloween( void );

	Vector				m_vHalloweenKartPush;
	float				m_flHalloweenKartPushEventTime;
	bool				m_bCheckKartCollision;
	EHANDLE				m_hKartBombHeadTarget;
	float				m_flNextBonusDucksVOAllowedTime;

	CNetworkVar( int,	m_iKartHealth );

	float				m_flGhostLastHitByKartTime;

	bool				m_bIsInPurgatory;		// for 2011 Halloween event
	CountdownTimer		m_purgatoryBuffTimer;
	CountdownTimer		m_purgatoryPainMultiplierTimer;
	int					m_purgatoryPainMultiplier;
	CNetworkVar( float, m_flHeadScale );
	CNetworkVar( float, m_flTorsoScale );
	CNetworkVar( float, m_flHandScale );

	//CountdownTimer		m_fireproofTimer;		// if active, we're fireproof

	// Wrenchmotron teleport
	CountdownTimer		m_teleportHomeFlashTimer;
	eEurekaTeleportTargets	m_eEurekaTeleportTarget;

	float				m_accumulatedSentryGunDamageDealt;	// for Sentry Buster missions in MvM
	int					m_accumulatedSentryGunKillCount;	// for Sentry Buster missions in MvM

	static const int	DPS_Period = 90;					// The duration of the sliding window for calculating DPS, in seconds
	int					*m_damageRateArray;					// One array element per second, for accumulating damage done during that time
	int					m_lastDamageRateIndex;
	int					m_peakDamagePerSecond;

	CNetworkVar( uint16, m_nActiveWpnClip );
	uint16				m_nActiveWpnClipPrev;
	float				m_flNextClipSendTime;

	float				m_flWaterExitTime;
	bool				m_bPendingMerasmusPlayerBombExplode;
	float				m_fLastBombHeadTimestamp;

	bool				m_bIsSapping;
	int					m_iSappingEvent;
	float				m_flSapStartTime;
	float				m_flLastThinkTime;
	float				m_flRespawnTimeOverride;
	string_t			m_strRespawnLocationOverride;

	CountdownTimer		m_booTimer;

	CNetworkVar( bool, m_bUseBossHealthBar );

	CNetworkVar( bool, m_bUsingVRHeadset );

	CNetworkVar( bool, m_bForcedSkin );
	CNetworkVar( int, m_nForcedSkin );

private:
	CHandle< CTFReviveMarker >		m_hReviveMarker;
public:
	CTFReviveMarker	*GetReviveMarker( void ) { return m_hReviveMarker; }

	// Send ForcePlayerViewAngles user message. Handled in __MsgFunc_ForcePlayerViewAngles in
	// clientmode_tf.cpp. Sets Local and Abs angles, along with TauntYaw and VehicleMovingAngles.
	void ForcePlayerViewAngles( const QAngle& qTeleportAngles );

	CBaseEntity *GetGrapplingHookTarget() const { return m_hGrapplingHookTarget; }
	void SetGrapplingHookTarget( CBaseEntity *pTarget, bool bShouldBleed = false );
	HSCRIPT ScriptGetGrapplingHookTarget() const { return ToHScript( m_hGrapplingHookTarget.Get() ); }
	void ScriptSetGrapplingHookTarget( HSCRIPT pTarget, bool bShouldBleed ) { return SetGrapplingHookTarget( ToEnt( pTarget ), bShouldBleed ); }

	void AddHudHideFlags(int flags) { m_Local.m_iHideHUD |= flags; }
	void RemoveHudHideFlags(int flags) { m_Local.m_iHideHUD &= ~flags; }
	void SetHudHideFlags(int flags) { m_Local.m_iHideHUD = flags; }
	int GetHudHideFlags() { return m_Local.m_iHideHUD; }

	bool IsUsingActionSlot() const { return m_bUsingActionSlot; }

	void SetSecondaryLastWeapon( CBaseCombatWeapon *pSecondaryLastWeapon ) { m_hSecondaryLastWeapon = pSecondaryLastWeapon; }
	CBaseCombatWeapon* GetSecondaryLastWeapon() const { return m_hSecondaryLastWeapon; }

	bool CanBeForcedToLaugh( void );

	void CreateDisguiseWeaponList( CTFPlayer *pDisguiseTarget );
	void ClearDisguiseWeaponList();

	bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
	CTFDroppedWeapon* GetDroppedWeaponInRange();

	bool HasCampaignMedal( int iMedal );
	void SetCampaignMedalActive( int iMedal ){ m_iCampaignMedals |= iMedal; }

	void InspectButtonPressed();
	void InspectButtonReleased();
	bool IsInspecting() const;

	bool HandleHelpmeTrace();
	void HelpmeButtonPressed();
	void HelpmeButtonReleased();
	bool IsHelpmeButtonPressed() const;

	void SetNextScorePointForPD( float flTime ){ m_flNextScorePointForPD = flTime; }
	bool CanScorePointForPD( void ) const;

	void AddCustomAttribute( const char *pszAttributeName, float flVal, float flDuration = -1.f );
	void RemoveCustomAttribute( const char *pszAttributeName );
	float ScriptGetCustomAttribute( const char *pName, float flFallbackValue );

	int GetSkinOverride() const { return m_iPlayerSkinOverride; }

	bool ShouldGetBonusPointsForExtinguishEvent( int userID );

	void SetLastAutobalanceTime( float flTime ) { m_flLastAutobalanceTime = flTime; }
	float GetLastAutobalanceTime() { return m_flLastAutobalanceTime; }
	bool IsMaxHealthDraining( void ) { return m_nMaxHealthDrainBucket != 0.0; }

private:
	bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
	bool TryToPickupDroppedWeapon();
	float m_flSendPickupWeaponMessageTime;

	void ModifyDamageInfo( CTakeDamageInfo *pInfo, const CBaseEntity *pTarget );

	CNetworkHandle( CBaseEntity, m_hGrapplingHookTarget );
	float m_flLastSeenHookTarget;
	int m_nHookAttachedPlayers;

	CNetworkHandle( CBaseCombatWeapon, m_hSecondaryLastWeapon );
	CNetworkVar( bool, m_bUsingActionSlot );

	CNetworkVar( float, m_flInspectTime );

	CNetworkVar( float, m_flHelpmeButtonPressTime );

	CNetworkVar( bool, m_bViewingCYOAPDA );

	CUtlVector< CHandle< CTFWeaponBase > > m_hDisguiseWeaponList; // copy disguise target weapons to this list

	CNetworkVar( int, m_iCampaignMedals );

	float m_flNextScorePointForPD;

	float m_flLastRuneChargeUpdate;
	float m_flLastDamageResistSoundTime;

	void UpdateCustomAttributes();
	void RemoveAllCustomAttributes();
	CUtlMap< CUtlString, float > m_mapCustomAttributes;

	CNetworkVar( int, m_iPlayerSkinOverride );

	CUtlMap<int, float> m_PlayersExtinguished;	// userID and most recent time they were extinguished for bonus points

	float m_flLastAutobalanceTime;

	void ResetMaxHealthDrain( void );
	int m_nMaxHealthDrainBucket;
	double m_dMaxHealthDrainLastUpdate;
	bool m_bMaxHealthRefilling;
	double m_dMaxHealthDrainAccumulator;
	double m_dMaxHealthDrainHealthAccumulator;

	bool m_bAlreadyUsedExtendFreezeThisDeath = false;
	
	// begin passtime
public:
	bool SayAskForBall();
	bool m_bPasstimeBallSlippery;
	// end passtime

	virtual bool ShouldForceTransmitsForTeam( int iTeam ) OVERRIDE;

	virtual bool IsTruceValidForEnt( void ) const OVERRIDE;

	virtual bool BHaveChatSuspensionInCurrentMatch() OVERRIDE;

	void StartPowerupModeDominant( bool bIsAlreadyDominant );
	void EndPowerupModeDominant( void );

	float m_flRemoveDominantConditionTime = -1.f;
	bool m_bIsInMannpowerDominantCondition = false;
	int m_nMannpowerKills = 0;
	int m_nMannpowerDeaths = 0;
	bool m_bMannpowerHereForFullInterval = false;

	virtual bool BCanCallVote() OVERRIDE;
	bool m_bFirstSpawnAndCanCallVote = false;
};

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert an entity into a tf player.
//   Input: pEntity - the entity to convert into a player
//-----------------------------------------------------------------------------
inline CTFPlayer *ToTFPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<CTFPlayer*>( pEntity ) != 0 );
	return static_cast< CTFPlayer* >( pEntity );
}

inline bool CTFPlayer::IsFireproof( void ) const
{
	return m_Shared.InCond( TF_COND_FIRE_IMMUNE );
}

inline bool CTFPlayer::HasPurgatoryBuff( void ) const
{
	return m_purgatoryBuffTimer.HasStarted() && !m_purgatoryBuffTimer.IsElapsed();
}

inline void CTFPlayer::OnSapperPlaced( CBaseEntity *sappedObject )
{
	m_placedSapperTimer.Start( 3.0f );
}
inline void CTFPlayer::OnSapperStarted( float flStartTime )
{
	if (m_iSappingEvent == TF_SAPEVENT_NONE && m_flSapStartTime == 0.00 )
	{
		m_flSapStartTime = flStartTime;
		m_bIsSapping = true;
		m_iSappingEvent = TF_SAPEVENT_PLACED;
	}
}
inline void CTFPlayer::OnSapperFinished( float flStartTime )
{
	if (m_iSappingEvent == TF_SAPEVENT_NONE && flStartTime == m_flSapStartTime )
	{
		if ( m_bIsSapping )
			m_iSappingEvent = TF_SAPEVENT_DONE;

		m_bIsSapping = false;
		m_flSapStartTime = 0.00;
	}
}
inline bool CTFPlayer::IsSapping( void ) const
{
	return m_bIsSapping;
}

inline int CTFPlayer::GetSappingEvent( void ) const
{
	return m_iSappingEvent;
}

inline void CTFPlayer::ClearSappingEvent( void )
{
	m_iSappingEvent = TF_SAPEVENT_NONE;
}

inline void CTFPlayer::ClearSappingTracking( void )
{
	ClearSappingEvent();
	m_bIsSapping = false;
	m_flSapStartTime = 0.00;
}

inline bool CTFPlayer::IsPlacingSapper( void ) const
{
	return !m_placedSapperTimer.IsElapsed();
}

inline int CTFPlayer::StateGet( void ) const
{
	return m_Shared.m_nPlayerState;
}

inline bool CTFPlayer::IsInCombat( void ) const
{
	// the simplest condition is whether we've been firing our weapon very recently
	return GetTimeSinceWeaponFired() < 2.0f;
}

inline bool CTFPlayer::IsCallingForMedic( void ) const
{
	return m_calledForMedicTimer.HasStarted() && m_calledForMedicTimer.IsLessThen( 5.0f );
}

inline float CTFPlayer::GetTimeSinceCalledForMedic() const
{
	return m_calledForMedicTimer.GetElapsedTime();
}

inline void CTFPlayer::NoteMedicCall( void )
{
	m_calledForMedicTimer.Start();
}

inline bool CTFPlayer::IsInPurgatory( void ) const
{
	return m_Shared.InCond( TF_COND_PURGATORY );
}

inline void CTFPlayer::AccumulateSentryGunDamageDealt( float damage )
{
	m_accumulatedSentryGunDamageDealt += damage;
}

inline void	CTFPlayer::ResetAccumulatedSentryGunDamageDealt()
{
	m_accumulatedSentryGunDamageDealt = 0.0f;
}

inline float CTFPlayer::GetAccumulatedSentryGunDamageDealt()
{
	return m_accumulatedSentryGunDamageDealt;
}

inline void CTFPlayer::IncrementSentryGunKillCount( void )
{
	++m_accumulatedSentryGunKillCount;
}

inline void	CTFPlayer::ResetAccumulatedSentryGunKillCount()
{
	m_accumulatedSentryGunKillCount = 0;
}

inline int CTFPlayer::GetAccumulatedSentryGunKillCount()
{
	return m_accumulatedSentryGunKillCount;
}

inline int CTFPlayer::GetDamagePerSecond( void ) const
{
	return m_peakDamagePerSecond;
}

inline void CTFPlayer::ResetDamagePerSecond( void )
{
	for( int i=0; i<DPS_Period; ++i )
	{
		m_damageRateArray[i] = 0;
	}

	m_lastDamageRateIndex = -1;
	m_peakDamagePerSecond = 0;
}


//=============================================================================
//
// CObserverPoint
//

class CObserverPoint : public CPointEntity
{
	DECLARE_CLASS( CObserverPoint, CPointEntity );
public:
	DECLARE_DATADESC();

	CObserverPoint();

	virtual void Activate( void );
	bool CanUseObserverPoint( CTFPlayer *pPlayer );
	virtual int UpdateTransmitState();
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	bool IsDefaultWelcome( void ) { return m_bDefaultWelcome; }
	bool IsMatchSummary( void ) { return m_bMatchSummary; }

	void SetDisabled( bool bDisabled ){ m_bDisabled = bDisabled; }

public:
	bool		m_bDisabled;
	bool		m_bDefaultWelcome;
	EHANDLE		m_hAssociatedTeamEntity;
	string_t	m_iszAssociateTeamEntityName;
	float		m_flFOV;
	bool		m_bMatchSummary;
};

#endif	// TF_PLAYER_H
