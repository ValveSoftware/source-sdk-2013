//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PLAYER_H
#define C_TF_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "c_baseplayer.h"
#include "tf_shareddefs.h"
#include "baseparticleentity.h"
#include "tf_player_shared.h"
#include "c_tf_playerclass.h"
#include "tf_item.h"
#include "props_shared.h"
#include "hintsystem.h"
#include "c_playerattachedmodel.h"
#include "c_playerrelativemodel.h"
#include "iinput.h"
#include "ihasattributes.h"
#include "GameEventListener.h"
#include "tf_item_inventory.h"
#include "c_tf_mvm_boss_progress_user.h"
#include "c_te_legacytempents.h"


class C_MuzzleFlashModel;
class C_BaseObject;
class C_TFRagdoll;
class C_TFWearable;
class C_CaptureZone;
class C_MerasmusBombEffect;
class CTFReviveDialog;
class C_TFDroppedWeapon;
class C_PasstimePlayerReticle;
class C_PasstimeAskForBallReticle;

extern ConVar tf_medigun_autoheal;
extern ConVar cl_autorezoom;
extern ConVar cl_autoreload;

enum EBonusEffectFilter_t
{
	kEffectFilter_AttackerOnly,
	kEffectFilter_AttackerTeam,
	kEffectFilter_VictimOnly,
	kEffectFilter_VictimTeam,
	kEffectFilter_AttackerAndVictimOnly,
	kEffectFilter_BothTeams,
};

struct BonusEffect_t
{
	const char* m_pszSoundName;
	const char* m_pszParticle;
	ParticleAttachment_t m_eAttachment;
	const char* m_pszAttachmentName;
	EBonusEffectFilter_t m_eParticleFilter;
	EBonusEffectFilter_t m_eSoundFilter;
	bool m_bPlaySoundInAttackersEars;
	bool m_bLargeCombatText;
};

extern BonusEffect_t g_BonusEffects[ kBonusEffect_Count ];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TFPlayer : public C_BasePlayer, public IHasAttributes, public IInventoryUpdateListener, public C_TFMvMBossProgressUser
{
public:

	DECLARE_CLASS( C_TFPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_TFPlayer();
	~C_TFPlayer();

	virtual void Spawn();

	static C_TFPlayer* GetLocalTFPlayer();

	virtual void UpdateOnRemove( void );

	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void SetDormant( bool bDormant );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ProcessMuzzleFlashEvent();
	virtual void ValidateModelIndex( void );
	void Touch( CBaseEntity *pOther );

	virtual Vector GetObserverCamOrigin( void );
	virtual int DrawModel( int flags );

	virtual void ApplyBoneMatrixTransform( matrix3x4_t& transform );
	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd ) OVERRIDE;
	void CreateVehicleMove( float flInputSampleTime, CUserCmd *pCmd );

	virtual bool		IsAllowedToSwitchWeapons( void );

	void    StopViewModelParticles( C_BaseEntity *pParticleEnt );

	virtual void ClientThink();

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

	CTFWeaponBase *GetActiveTFWeapon( void ) const;
	int GetPassiveWeapons( CUtlVector<CTFWeaponBase*>& vecOut );
	bool IsActiveTFWeapon( CEconItemDefinition *weaponHandle ) const;
	bool IsActiveTFWeapon( const CSchemaItemDefHandle &weaponHandle ) const;

	virtual void Simulate( void );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options ) OVERRIDE;
	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity ) OVERRIDE;

	CNewParticleEffect *SpawnHalloweenSpellFootsteps( ParticleAttachment_t eParticleAttachment, int iHalloweenFootstepType );

	void FireBullet( CTFWeaponBase *pWpn, const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType = TF_DMG_CUSTOM_NONE );

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	bool CanAttack( int iCanAttackFlags = 0 );
	bool CanJump() const;
	bool CanDuck() const;

	const C_TFPlayerClass *GetPlayerClass( void ) const	{ return &m_PlayerClass; }
	C_TFPlayerClass *GetPlayerClass( void )				{ return &m_PlayerClass; }
	bool IsPlayerClass( int iClass ) const;
	virtual int GetMaxHealth( void ) const;
	int			GetMaxHealthForBuffing()  const;

	virtual int GetRenderTeamNumber( void );

	bool IsWeaponLowered( void );

	void	AvoidPlayers( CUserCmd *pCmd );

	bool	IsABot( void );

	// Get the ID target entity index. The ID target is the player that is behind our crosshairs, used to
	// display the player's name.
	void UpdateIDTarget();
	int GetIDTarget() const;
	void SetForcedIDTarget( int iTarget );

	void SetAnimation( PLAYER_ANIM playerAnim );

	virtual float GetMinFOV() const;

	virtual const QAngle& EyeAngles();

	bool	ShouldDrawSpyAsDisguised();
	virtual int GetBody( void );

	int GetBuildResources( void );

	// MATTTODO: object selection if necessary
	void SetSelectedObject( C_BaseObject *pObject ) {}

	void GetTeamColor( Color &color );
	bool InSameDisguisedTeam( CBaseEntity *pEnt );

	void ForceTempForceDraw( bool bThirdPerson );

	void FlushAllPlayerVisibilityState();

	virtual void ComputeFxBlend( void );

	// Taunts/VCDs
	virtual bool	StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );
	virtual	bool	ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );
	virtual void	CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	bool			StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	bool			StopGestureSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );
	void			TurnOnTauntCam( void );
	void			TurnOnTauntCam_Finish( void );
	void			TurnOffTauntCam( void );
	void			TurnOffTauntCam_Finish( void );
	bool			IsTaunting( void ) const { return m_Shared.InCond( TF_COND_TAUNTING ); }

	bool			IsViewingCYOAPDA( void ) const { return m_bViewingCYOAPDA; }
	bool			IsRegenerating( void ) const { return m_bRegenerating; }

	virtual void	InitPhonemeMappings();

	// Gibs.
	void InitPlayerGibs( void );
	void CheckAndUpdateGibType( void );
	void CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning, bool bWearableGibs=false, bool bOnlyHead=false, bool bDisguiseGibs=false );
	void DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity );
	void DropWearable( C_TFWearable *pItem, const breakablepropparams_t &params );

	int	GetObjectCount( void );
	C_BaseObject *GetObject( int index );
	C_BaseObject *GetObjectOfType( int iObjectType, int iObjectMode=0 ) const;
	int GetNumObjects( int iObjectType, int iObjectMode=0 );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	float GetPercentInvisible( void );
	float GetEffectiveInvisibilityLevel( void );	// takes viewer into account
	virtual bool IsTransparent( void ) OVERRIDE { return GetPercentInvisible() > 0.f; }

	virtual void AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	virtual void CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual Vector GetChaseCamViewOffset( CBaseEntity *target );
	virtual Vector GetDeathViewPosition();

	void ClientPlayerRespawn( void );

	virtual bool	ShouldDraw();

	virtual int		GetVisionFilterFlags( bool bWeaponsCheck = false );
	virtual void	CalculateVisionUsingCurrentFlags( void );

	void CreateSaveMeEffect( MedicCallerType nType = CALLER_TYPE_NORMAL );
	void StopSaveMeEffect( bool bForceRemoveInstantly = false );

	void CreateTauntWithMeEffect();
	void StopTauntWithMeEffect();

	void CreateKart();
	void RemoveKart();
	C_BaseAnimating *GetKart() const { return m_pKart; }
	void CreateKartEffect( const char *pszEffectName );
	void StopKartEffect();
	void UpdateKartSounds();
	void StartKartBrakeEffect();
	void StopKartBrakeEffect();
	CNetworkVar( int, m_iKartState );

	bool IsAllowedToTaunt( void );
	
	virtual bool	IsOverridingViewmodel( void );
	virtual int		DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );

	void			SetHealer( C_TFPlayer *pHealer, float flChargeLevel );
	void			SetWasHealedByLocalPlayer( bool bState )	{ m_bWasHealedByLocalPlayer = bState; }
	void			GetHealer( C_TFPlayer **pHealer, float *flChargeLevel ) { *pHealer = m_hHealer; *flChargeLevel = m_flHealerChargeLevel; }
	bool			GetWasHealedByLocalPlayer() { return m_bWasHealedByLocalPlayer; }
	float			MedicGetChargeLevel( CTFWeaponBase **pRetMedigun = NULL );
	bool			MedicIsReleasingCharge( void );
	CBaseEntity		*MedicGetHealTarget( void );

	void			StartBurningSound( void );
	void			StopBurningSound( void );

	void			StopBlastJumpLoopSound( int iUserID );
	
	void			UpdateSpyStateChange( void );

	void			UpdateRecentlyTeleportedEffect( void );
	void			UpdateOverhealEffect( void );
	void			UpdatedMarkedForDeathEffect( bool bFroceStop = false );
	void			CreateOverhealEffect( int iTeam );
	void			UpdateRuneIcon( bool bForceStop = false );

	bool			CanShowClassMenu( void );
	bool			CanShowTeamMenu( void );

	void			InitializePoseParams( void );
	void			UpdateLookAt( void );

	bool			IsEnemyPlayer( void );
	void			ShowNemesisIcon( bool bShow );
	void			ShowDuelingIcon( bool bShow );
	void			ShowIconForIT( bool bShow );

	void			ShowBirthdayEffect( bool bShow );

	CUtlVector<EHANDLE>		*GetSpawnedGibs( void ) { return &m_hSpawnedGibs; }

	bool			HasBombinomiconEffectOnDeath( void );

	Vector			GetClassEyeHeight( void );

	void			ForceUpdateObjectHudState( void );

	bool			GetMedigunAutoHeal( void ){ return tf_medigun_autoheal.GetBool(); }
	bool			ShouldAutoRezoom( void ){ return cl_autorezoom.GetBool(); }
	bool			ShouldAutoReload( void ){ return cl_autoreload.GetBool(); }

	void			GetTargetIDDataString( bool bIsDisguised, OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes, bool &bIsAmmoData, bool &bIsKillStreakData );

	void			RemoveDisguise( void );
	bool			CanDisguise( void );
	bool			CanDisguise_OnKill( void );

	virtual void OnAchievementAchieved( int iAchievement );

	virtual void OverrideView( CViewSetup *pSetup );

	bool			CanAirDash( void ) const;
	bool			CanGetWet() const;

	void			CreateBoneAttachmentsFromWearables( C_TFRagdoll *pRagdoll, bool bDisguised );

	bool			CanUseFirstPersonCommand( void );

	bool			IsEffectRateLimited( EBonusEffectFilter_t effect, const C_TFPlayer* pAttacker ) const;
	bool			ShouldPlayEffect( EBonusEffectFilter_t filter, const C_TFPlayer* pAttacker, const C_TFPlayer* pVictim ) const;
	virtual void	FireGameEvent( IGameEvent *event );

	virtual const char* ModifyEventParticles( const char* token );

	// Set the distances the camera should use. 
	void			SetTauntCameraTargets( float back, float up );

	// TF-specific color values for GlowEffect
	virtual void	GetGlowEffectColor( float *r, float *g, float *b );
	void UpdateGlowColor( void );

	virtual const Vector&	GetRenderOrigin( void );

//	RTime32			GetSpottedInPVSTime() const { return m_rtSpottedInPVSTime; }
//	RTime32			GetJoinedSpectatorTeamTime() const { return m_rtJoinedSpectatorTeam; }
//	RTime32			GetJoinedNormalTeamTime() const { return m_rtJoinedNormalTeam; }

	// IHasAttributes
	CAttributeManager		*GetAttributeManager( void ) { return &m_AttributeManager; }
	CAttributeContainer		*GetAttributeContainer( void ) { return NULL; }
	CBaseEntity				*GetAttributeOwner( void ) { return NULL; }
	CAttributeList			*GetAttributeList( void ) { return &m_AttributeList; }
	virtual void			ReapplyProvision( void ) { return; }

	// ITFMvMBossProgressUser
	virtual const char* GetBossProgressImageName() const OVERRIDE;
	virtual float GetBossStatusProgress() const OVERRIDE;

protected:
	CNetworkVarEmbedded(	CAttributeContainerPlayer, m_AttributeManager );

	// IClientNetworkable implementation.
public:
	virtual void	NotifyShouldTransmit( ShouldTransmitState_t state );

public:
	// Shared functions
	float			GetMovementForwardPull( void ) const;
	bool			CanPlayerMove() const;
	float			TeamFortress_CalculateMaxSpeed( bool bIgnoreSpecialAbility = false ) const;
	void			TeamFortress_SetSpeed();
	bool			HasItem( void ) const;				// Currently can have only one item at a time.
	void			SetItem( C_TFItem *pItem );
	C_TFItem		*GetItem( void ) const;
	bool			HasTheFlag( ETFFlagType exceptionTypes[] = NULL, int nNumExceptions = 0 ) const;
	virtual bool	IsAllowedToPickUpFlag( void ) const;
	float			GetCritMult( void ) { return m_Shared.GetCritMult(); }

	virtual void	ItemPostFrame( void );

	void			SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void			HolsterOffHandWeapon( void );
	CTFWeaponBase*	GetOffHandWeapon( void ) { return m_hOffHandWeapon; }

	void			GetActiveSets( CUtlVector<const CEconItemSetDefinition *> *pItemSets );

	virtual int GetSkin();

	float GetLastDamageTimeMvMOnly( void ) const { return m_flMvMLastDamageTime; }

	virtual bool		Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );

	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) OVERRIDE;
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 ) OVERRIDE;
	virtual void 		SelectItem( const char *pstr, int iSubType = 0 ) OVERRIDE;

	void				Weapon_PoseParamOverride( CTFWeaponBase *pOldWeapon, CTFWeaponBase *pNewWeapon );

	virtual void		UpdateWearables() OVERRIDE;
	CTFWearable			*GetEquippedWearableForLoadoutSlot( int iLoadoutSlot );
	CBaseEntity			*GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable = false );			//Gets whatever entity is associated with the loadout slot (wearable or weapon)

	CTFWeaponBase		*Weapon_OwnsThisID( int iWeaponID ) const;
	CTFWeaponBase		*Weapon_GetWeaponByType( int iType );

	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char *GetOverrideStepSound( const char *pszBaseStepSoundName );

	virtual void		OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume );

	virtual void		ModifyEmitSoundParams( EmitSound_t &params );

	virtual void		ThirdPersonSwitch( bool bThirdperson );

	bool	DoClassSpecialSkill( void );
	bool	EndClassSpecialSkill( void );
	bool	CanGoInvisible( bool bAllowWhileCarryingFlag = false );
	int		GetMaxAmmo( int iAmmoIndex, int iClassIndex = -1 );

	//-----------------------------------------------------------------------------------------------------
	// Return true if we are a "mini boss" in Mann Vs Machine mode
	bool IsMiniBoss( void ) const;
	bool ShouldTauntHintIconBeVisible() const;
	virtual bool IsHealthBarVisible( void ) const OVERRIDE;

	bool	CanStartPhase( void );

	bool	CanPickupBuilding( CBaseObject *pPickupObject );
	bool	TryToPickupBuilding( void );
	void	StartBuildingObjectOfType( int iType, int iObjectMode=0 );

	void			FeignDeath( CTakeDamageInfo& info );

	C_CaptureZone *GetCaptureZoneStandingOn( void );
	C_CaptureZone *GetClosestCaptureZone( void );

	float			GetMetersRan( void )	{ return m_fMetersRan; }
	void			SetMetersRan( float fMeters, int iFrame );

	CEconItemView *GetInspectItem( int *iLastItem );

	void			SetBodygroupsDirty( void );
	void			RecalcBodygroupsIfDirty( void );

	bool			CanMoveDuringTaunt();
	bool			ShouldStopTaunting();
	bool			IsTauntForceMovingForward() const { return m_bTauntForceMoveForward; }
	float			GetTauntMoveAcceleration() const { return m_flTauntMoveAccelerationTime; }
	float			GetTauntMoveSpeed() const { return m_flTauntForceMoveForwardSpeed; }
	float			GetTauntTurnAccelerationTime() const { return m_flTauntTurnAccelerationTime; }
	bool			IsReadyToTauntWithPartner( void ) const { return m_bIsReadyToHighFive; }
	CTFPlayer *		GetTauntPartner( void )		{ return m_hHighFivePartner; }
	float			GetTauntYaw( void )				{ return m_flTauntYaw; }
	float			GetPrevTauntYaw( void )		{ return m_flPrevTauntYaw; }
	void			SetTauntYaw( float flTauntYaw );
	int				GetActiveTauntSlot() const { return m_nActiveTauntSlot; }
	void			PlayTauntSoundLoop( const char *pszSoundLoopName );
	void			StopTauntSoundLoop();
	float			GetCurrentTauntMoveSpeed() const { return m_flCurrentTauntMoveSpeed; }
	void			SetCurrentTauntMoveSpeed( float flSpeed ) { m_flCurrentTauntMoveSpeed = flSpeed; }
	float			GetVehicleReverseTime() const { return m_flVehicleReverseTime; }
	void			SetVehicleReverseTime( float flTime ) { m_flVehicleReverseTime = flTime; }

	CEconItemView	*GetTauntEconItemView() { return m_TauntEconItemView.IsValid() ? &m_TauntEconItemView : NULL; }

	float			GetHeadScale() const { return m_flHeadScale; }
	float			GetTorsoScale() const { return m_flTorsoScale; }
	float			GetHandScale() const { return m_flHandScale; }
	float			GetLastResistTime()	const { return m_flLastResistTime; }
	bool			BRenderAsZombie( bool bWeaponsCheck = false );
	static void AdjustSkinIndexForZombie( int iClassIndex, int &iSkinIndex );

	// Ragdolls.
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual IRagdoll		*GetRepresentativeRagdoll() const;
	EHANDLE	m_hRagdoll;
	Vector m_vecRagdollVelocity;

	// Objects
	int CanBuild( int iObjectType, int iObjectMode=0 );
	CUtlVector< CHandle<C_BaseObject> > m_aObjects;

	virtual CStudioHdr *OnNewModel( void );

	void				DisplaysHintsForTarget( C_BaseEntity *pTarget );

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) ;
	virtual void GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual bool GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;

	CMaterialReference *GetInvulnMaterialRef( void ) { return &m_InvulnerableMaterial; }
	bool IsNemesisOfLocalPlayer();
	bool ShouldShowDuelingIcon();
	bool ShouldShowNemesisIcon();

	virtual	IMaterial *GetHeadLabelMaterial( void );

	// Spy Cigarette
	bool CanLightCigarette( void );

	void		UpdateDemomanEyeEffect( int iDecapitations );
	const char* GetDemomanEyeEffectName( int iDecapitations );

	int		GetCurrency( void ){ return m_nCurrency; }

	virtual void UpdateMVMEyeGlowEffect( bool bVisible );

	void	UpdateKillStreakEffects( int iCount, bool bKillScored = false );
	const char *GetEyeGlowEffect() { return m_pszEyeGlowEffectName; }
	Vector GetEyeGlowColor( bool bAlternate ) { return bAlternate? m_vEyeGlowColor1 : m_vEyeGlowColor2 ; }

	// Bounty Mode
	int	 GetExperienceLevel( void ) { return m_nExperienceLevel; }

	// Matchmaking
	bool	GetMatchSafeToLeave() { return m_bMatchSafeToLeave; }

	// Halloween silliness.
	void	HalloweenBombHeadUpdate( void );


	bool	IsUsingVRHeadset( void ){ return m_bUsingVRHeadset; }

	bool	ShouldPlayerDrawParticles( void );

	bool	IsPlayerOnSteamFriendsList( C_BasePlayer *pPlayer );

protected:

	void ResetFlexWeights( CStudioHdr *pStudioHdr );

	virtual void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual void UpdateGlowEffect( void );
	virtual void DestroyGlowEffect( void );

private:

	bool ShouldShowPowerupGlowEffect();
	void GetPowerupGlowEffectColor( float *r, float *g, float *b );

	void HandleTaunting( void );
	void TauntCamInterpolation( void );

	void OnPlayerClassChange( void );
	void UpdatePartyHat( void );

	void InitInvulnerableMaterial( void );

	void GetHorriblyHackedRailgunPosition( const Vector& vStart, Vector *out_pvStartPos );
	void MaybeDrawRailgunBeam( IRecipientFilter *pFilter, CTFWeaponBase *pWeapon, const Vector& vStartPos, const Vector& vEndPos );

	bool				m_bWasTaunting;
	bool				m_bTauntInterpolating;
	CameraThirdData_t	m_TauntCameraData;
	float				m_flTauntCamCurrentDist;
	float				m_flTauntCamTargetDist;
	float				m_flTauntCamCurrentDistUp;
	float				m_flTauntCamTargetDistUp;

	QAngle				m_angTauntPredViewAngles;
	QAngle				m_angTauntEngViewAngles;

	CSoundPatch			*m_pTauntSoundLoop;

	C_TFPlayerClass		m_PlayerClass;

	// ID Target
	int					m_iIDEntIndex;
	int					m_iForcedIDTarget;

	CNewParticleEffect	*m_pTeleporterEffect;
	bool				m_bToolRecordingVisibility;

	int					m_iOldSpawnCounter;

	// Healer
	CHandle<C_TFPlayer>	m_hHealer;
	bool				m_bWasHealedByLocalPlayer;
	float				m_flHealerChargeLevel;
	int					m_iOldHealth;
	int					m_nOldMaxHealth;

	float				m_fMetersRan;
	int					m_iLastRanFrame;

	HPARTICLEFFECT		m_pEyeEffect;

	bool				m_bOldCustomModelVisible;

	CHandle< C_BaseCombatWeapon > m_hOldActiveWeapon;

	// Look At
	/*
	int m_headYawPoseParam;
	int m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;
	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;
	*/

	// Spy cigarette smoke
	bool m_bCigaretteSmokeActive;

	// Medic callout particle effect
	CNewParticleEffect	*m_pSaveMeEffect;
	CNewParticleEffect	*m_pTauntWithMeEffect;

	bool m_bUpdateObjectHudState;
	bool	m_bBodygroupsDirty;

	HPARTICLEFFECT	m_hKartDamageEffect;
	CNetworkVar( float, m_flKartNextAvailableBoost );
	CNetworkVar( int,	m_iKartHealth );
	int			m_iOldKartHealth;
	void		UpdateKartEffects();

	void		UpdateKartState();
	int			m_iOldKartState;

	C_BaseAnimating *m_pKart;

public:
	float			GetKartSpeedBoost( void );
	float			GetKartHealth( void )				{ return m_iKartHealth; }

	CTFPlayerShared m_Shared;
	friend class CTFPlayerShared;

// Called by shared code.
public:
	float GetClassChangeTime() const { return m_flChangeClassTime; }
	void SetFootStamps( int nFootStamps ) { m_nFootStamps = nFootStamps; }

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	bool PlayAnimEventInPrediction( PlayerAnimEvent_t event );

	bool							GetPredictable( void ) const;

	const QAngle& GetNetworkEyeAngles() const { return m_angEyeAngles; }

	// Halloween
	void CreateBombonomiconHint();
	void DestroyBombonomiconHint();

	void CleanUpAnimationOnSpawn();
	CTFPlayerAnimState *m_PlayerAnimState;

	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkHandle( C_TFItem, m_hItem );

	CNetworkHandle( C_TFWeaponBase, m_hOffHandWeapon );
	CNetworkHandle( C_TFPlayer, m_hCoach );
	CNetworkHandle( C_TFPlayer, m_hStudent );

	CGlowObject		*m_pStudentGlowEffect;
	CGlowObject		*m_pPowerupGlowEffect;

	int				m_iOldPlayerClass;	// Used to detect player class changes
	bool			m_bIsDisplayingNemesisIcon;
	bool			m_bIsDisplayingDuelingIcon;
	bool			m_bIsDisplayingIconForIT;
	bool			m_bIsDisplayingTranqMark;
	bool			m_bShouldShowBirthdayEffect;

	RuneTypes_t		m_eDisplayingRuneIcon;

	float			m_flMvMLastDamageTime;
	int				m_iSpawnCounter;
	bool			m_bArenaSpectator;

	bool			m_bIsMiniBoss;
	bool			m_bIsABot;
	int				m_nBotSkill;
	int				m_nOldBotSkill;
	bool			m_bSaveMeParity;
	bool			m_bOldSaveMeParity;
	bool			m_bIsCoaching;

private:
	void			UpdateTauntItem();
	void			ParseSharedTauntDataFromEconItemView( const CEconItemView *pEconItemView );

	QAngle			m_angEyeAngles;

	bool			m_bAllowMoveDuringTaunt;
	bool			m_bTauntForceMoveForward;
	float			m_flTauntForceMoveForwardSpeed;
	float			m_flTauntMoveAccelerationTime;
	float			m_flTauntTurnSpeed;
	float			m_flTauntTurnAccelerationTime;
	bool			m_bIsReadyToHighFive;
	CNetworkHandle( C_TFPlayer, m_hHighFivePartner );
	int				m_nForceTauntCam;
	float			m_flTauntYaw;
	float			m_flPrevTauntYaw;
	int				m_nActiveTauntSlot;
	int				m_nPrevTauntSlot;
	item_definition_index_t	m_iTauntItemDefIndex;
	item_definition_index_t m_iPrevTauntItemDefIndex;
	float			m_flCurrentTauntMoveSpeed;
	float			m_flVehicleReverseTime;

	int				m_nTauntSequence;
	float			m_flTauntStartTime;
	float			m_flTauntDuration;

	CEconItemView	m_TauntEconItemView;

public:

	int				m_nOldWaterLevel;
	float			m_flWaterEntryTime;
	bool			m_bWaterExitEffectActive;

	bool			m_bDuckJumpInterp;
	float			m_flFirstDuckJumpInterp;
	float			m_flLastDuckJumpInterp;
	float			m_flDuckJumpInterp;

	CMaterialReference	m_InvulnerableMaterial;


	// Burning
	CSoundPatch			*m_pBurningSound;
	HPARTICLEFFECT      m_pBurningEffect;
	float				m_flBurnEffectStartTime;

	// Urine
	HPARTICLEFFECT		m_pUrineEffect;

	// Milk
	HPARTICLEFFECT		m_pMilkEffect;

	// Gas
	HPARTICLEFFECT		m_pGasEffect;

	// Soldier Buff
	HPARTICLEFFECT		m_pSoldierOffensiveBuffEffect;
	HPARTICLEFFECT		m_pSoldierDefensiveBuffEffect;
	HPARTICLEFFECT		m_pSoldierOffensiveHealthRegenBuffEffect;
	HPARTICLEFFECT		m_pSoldierNoHealingDamageBuffEffect;

	// Speed boost
	HPARTICLEFFECT		m_pSpeedBoostEffect;

	// Taunt effects
	HPARTICLEFFECT		m_pTauntEffect;

	// Temp HACK for crit boost
	HPARTICLEFFECT m_pCritBoostEffect;

	HPARTICLEFFECT m_pOverHealedEffect;
	HPARTICLEFFECT m_pPhaseStandingEffect;

	HPARTICLEFFECT m_pStunnedEffect;

	HPARTICLEFFECT m_pMegaHealEffect;
	HPARTICLEFFECT m_pRadiusHealEffect;
	HPARTICLEFFECT m_pKingRuneRadiusEffect;
	HPARTICLEFFECT m_pKingBuffRadiusEffect;
	HPARTICLEFFECT m_pRunePlagueEffect;
	C_LocalTempEntity*	m_pTempShield;
	float				m_flLastResistTime;

	HPARTICLEFFECT m_pSappedPlayerEffect;
	HPARTICLEFFECT m_pMVMEyeGlowEffect[ 2 ];

	// KillStreak Weapons
	char m_pszEyeGlowEffectName[MAX_PATH];
	Vector m_vEyeGlowColor1;
	Vector m_vEyeGlowColor2;
	HPARTICLEFFECT m_pEyeGlowEffect[ 2 ];
	float m_flNextSheenStartTime;
	
	HPARTICLEFFECT m_pMVMBotRadiowave;

	HPARTICLEFFECT m_pRuneChargeReadyEffect;

	enum EKartParticles
	{
		KART_PARTICLE_LEFT_LIGHT = 0,
		KART_PARTICLE_RIGHT_LIGHT,

		KART_PARTICLE_LEFT_WHEEL,
		KART_PARTICLE_RIGHT_WHEEL,
		NUM_KART_PARTICLES
	};
	HPARTICLEFFECT m_pKartParticles[ NUM_KART_PARTICLES ];

	enum EKartSounds
	{
		KART_SOUND_ENGINE_LOOP = 0,
		KART_SOUND_BURNOUT_LOOP,

		NUM_KART_SOUNDS,
	};
	CSoundPatch			*m_pKartSounds[ NUM_KART_SOUNDS ];

	CNewParticleEffect	*m_pDisguisingEffect;
	float m_flDisguiseEffectStartTime;
	float m_flDisguiseEndEffectStartTime;

	EHANDLE					m_hFirstGib;
	EHANDLE					m_hHeadGib;
	CUtlVector<EHANDLE>		m_hSpawnedGibs;

	int				m_iOldTeam;
	int				m_iOldClass;
	int				m_iOldDisguiseTeam;
	int				m_iOldDisguiseClass;
	int				m_iOldObserverMode;
	EHANDLE			m_hOldObserverTarget;

	bool			m_bDisguised;
	int				m_iPreviousMetal;

	int GetNumActivePipebombs( void );

	int				m_iSpyMaskBodygroup;
	Vector			m_vecCustomModelOrigin;

	// Halloween
	CHandle<C_PlayerAttachedModel>	m_hHalloweenBombHat;
	CHandle<C_MerasmusBombEffect>	m_hBombonomiconHint;
	CHandle<C_PlayerAttachedModel>	m_hHalloweenKartCage;
	float			m_flBombDelay;

	// Achievements
	float m_flSaveMeExpireTime;

	//CountdownTimer m_LeaveServerTimer;

	//----------------------------
	// INVENTORY MANAGEMENT
public:
	// IInventoryUpdateListener
	virtual void InventoryUpdated( CPlayerInventory *pInventory );
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner ) { m_Shared.SetLoadoutUnavailable( true ); }
	void		 UpdateInventory( bool bInit );

	// Inventory access
	CTFPlayerInventory	*Inventory( void ) { return &m_Inventory; }

	bool		CanDisplayAllSeeEffect( EAttackBonusEffects_t effect ) const;
	void		SetNextAllSeeEffectTime( EAttackBonusEffects_t effect, float flTime );

private:
	CTFPlayerInventory	m_Inventory;
	bool				m_bInventoryReceived;

private:
	float			m_flChangeClassTime;

	float m_flWaterImpactTime;
//	RTime32 m_rtSpottedInPVSTime;
//	RTime32 m_rtJoinedSpectatorTeam;
//	RTime32 m_rtJoinedNormalTeam;

	// Gibs.
	CUtlVector< int > m_aSillyGibs;
	CUtlVector< char* > m_aNormalGibs;
	CUtlVector<breakmodel_t>	m_aGibs;

	C_TFPlayer( const C_TFPlayer & );

	mutable char m_bIsCalculatingMaximumSpeed;

	// In-game currency
	int m_nCurrency;
	int m_nOldCurrency;

	// Bounty Mode
	int m_nExperienceLevel;
	int m_nExperienceLevelProgress;
	int m_nPrevExperienceLevel;

	// Matchmaking
	// is this player bound to the match on penalty of abandon. Sync'd via local-player-only DT
	bool m_bMatchSafeToLeave;

	// Medic healtarget active weapon ammo/clip count
	uint16	m_nActiveWpnClip;
	
	// Blast jump whistle
	CSoundPatch		*m_pBlastJumpLoop;
	float			m_flBlastJumpLaunchTime;

	// falling sound that plays when player reaches fall speed that will apply fall damage
	CSoundPatch		*m_pFallingSoundLoop;

	CNetworkVar( float, m_flHeadScale );
	CNetworkVar( float, m_flTorsoScale );
	CNetworkVar( float, m_flHandScale );

	// Allseecrit throttle - other clients ask us if we can be the source of another particle+sound
	float	m_flNextMiniCritEffectTime[ kBonusEffect_Count ];

	CNetworkVar( bool, m_bUseBossHealthBar );

	CNetworkVar( bool, m_bUsingVRHeadset );

	CNetworkVar( bool, m_bForcedSkin );
	CNetworkVar( int, m_nForcedSkin );

	int m_nFootStamps;

	vgui::DHANDLE< CTFReviveDialog > m_hRevivePrompt;

public:
	void SetShowHudMenuTauntSelection( bool bShow ) { m_bShowHudMenuTauntSelection = bShow; }
	bool ShouldShowHudMenuTauntSelection() const { return m_bShowHudMenuTauntSelection; }

private:
	bool m_bShowHudMenuTauntSelection;

public:
	CBaseEntity *GetGrapplingHookTarget() const { return m_hGrapplingHookTarget; }

	bool IsUsingActionSlot() const { return m_bUsingActionSlot; }
	void SetUsingActionSlot( bool bUsingActionSlot ) { m_bUsingActionSlot = bUsingActionSlot; }
	
	void SetSecondaryLastWeapon( CBaseCombatWeapon *pSecondaryLastWeapon ) { m_hSecondaryLastWeapon = pSecondaryLastWeapon; }
	CBaseCombatWeapon* GetSecondaryLastWeapon() const { return m_hSecondaryLastWeapon; }

	bool CanPickupDroppedWeapon( const C_TFDroppedWeapon *pWeapon );
	C_TFDroppedWeapon* GetDroppedWeaponInRange();

	bool HasCampaignMedal( int iMedal );
	CampaignMedalDisplayType_t GetCampaignMedalType( void );
	const char *GetCampaignMedalImage( void );

	void SetInspectTime( float flInspectTime ) { m_flInspectTime = flInspectTime; }
	bool IsInspecting() const;
	void HandleInspectHint();

	void SetHelpmeButtonPressedTime( float flPressTime ) { m_flHelpmeButtonPressTime = flPressTime; }
	bool IsHelpmeButtonPressed() const;

	bool AddOverheadEffect( const char *pszEffectName );
	void RemoveOverheadEffect( const char *pszEffectName, bool bRemoveInstantly );
	void UpdateOverheadEffects();
	Vector GetOverheadEffectPosition();

	int GetSkinOverride() const { return m_iPlayerSkinOverride; }

	virtual void ClientAdjustStartSoundParams( EmitSound_t &params ) override;
	virtual void ClientAdjustStartSoundParams( StartSoundParams_t& params ) override;

private:
	void ClientAdjustVOPitch( int& pitch );

private:
	CNetworkHandle( CBaseEntity, m_hGrapplingHookTarget );
	CNetworkHandle( CBaseCombatWeapon, m_hSecondaryLastWeapon );
	CNetworkVar( bool, m_bUsingActionSlot );
	CNetworkVar( int, m_iCampaignMedals );
	CNetworkVar( float, m_flInspectTime );
	CNetworkVar( float, m_flHelpmeButtonPressTime );
	CNetworkVar( bool, m_bViewingCYOAPDA );
	CNetworkVar( bool, m_bRegenerating );

	bool m_bNotifiedWeaponInspectThisLife;

	C_PasstimePlayerReticle *m_pPasstimePlayerReticle;
	C_PasstimeAskForBallReticle *m_pPasstimeAskForBallReticle;

	CUtlMap< const char *, HPARTICLEFFECT > m_mapOverheadEffects;
	float m_flOverheadEffectStartTime;

	int m_nTempForceDrawViewModelSequence = -1;
	int m_nTempForceDrawViewModelSkin = 0;
	float m_flTempForceDrawViewModelCycle  = 0.0f;

	CNetworkVar( int, m_iPlayerSkinOverride );
};

inline C_TFPlayer* ToTFPlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<C_TFPlayer*>( pEntity ) != 0 );
	return static_cast< C_TFPlayer* >( pEntity );
}

void SetAppropriateCamera( C_TFPlayer *pPlayer );

class C_TFPlayerPreviewEffect
{
public:
	// If you re-order this list, please update TF_ImportPreview_Effect* in tf_english.txt
	enum PREVIEW_EFFECT
	{
		PREVIEW_EFFECT_NONE,
		PREVIEW_EFFECT_UBER,
		//PREVIEW_EFFECT_CRIT,	// Punting on particle effects for now
		PREVIEW_EFFECT_URINE,
		//PREVIEW_EFFECT_MILK,	// Punting on particle effects for now
		//PREVIEW_EFFECT_INVIS,	// The CMDLPanel draw path doesn't handle transparency at the moment
		PREVIEW_EFFECT_BURN,
		NUM_PREVIEW_EFFECTS
	};

public:
	C_TFPlayerPreviewEffect();

	void SetEffect(PREVIEW_EFFECT nEffect) { m_nPreviewEffect = nEffect; }
	PREVIEW_EFFECT GetEffect() const { return m_nPreviewEffect; }

	void SetTeam(int nTeam);
	int GetTeam() const { return m_nTeam; }

	CMaterialReference *GetInvulnMaterialRef( void ) { return &m_InvulnerableMaterial; }

	void Reset();

protected:
	PREVIEW_EFFECT m_nPreviewEffect;
	int m_nTeam;
	CMaterialReference	m_InvulnerableMaterial;
};
extern C_TFPlayerPreviewEffect g_PlayerPreviewEffect;

class C_TFRagdoll : public C_BaseFlex
{
public:

	DECLARE_CLASS( C_TFRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();

	C_TFRagdoll();
	~C_TFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	void ClientThink( void );

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

	void StartFadeOut( float fDelay );
	void EndFadeOut();
	void DissolveEntity( CBaseEntity* pEnt );

	C_TFPlayer *GetPlayer( void ) const { return m_hPlayer; }

	bool IsRagdollVisible();
	float GetBurnStartTime() { return m_flBurnEffectStartTime; }

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );

	bool IsDeathAnim() { return m_bDeathAnim; }

	int GetDamageCustom() { return m_iDamageCustom; }

	virtual bool GetAttachment( int iAttachment, matrix3x4_t &attachmentToWorld );

	int GetClass() { return m_iClass; }

	float GetPercentInvisible( void ) { return m_flPercentInvisible; }
	bool IsCloaked( void ) { return m_bCloaked; }

	int GetRagdollTeam( void ) { return m_iTeam; }

	float GetHeadScale() const { return m_flHeadScale; }
	float GetTorsoScale() const { return m_flTorsoScale; }
	float GetHandScale() const { return m_flHandScale; }

private:

	C_TFRagdoll( const C_TFRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateTFRagdoll();
	void CreateTFGibs( bool bDestroyRagdoll = true, bool bCurrentPosition = false );
	void CreateWearableGibs( bool bDisguiseWearables );
	void CreateTFHeadGib();

	virtual float FrameAdvance( float flInterval );

	bool IsDecapitation();
	bool IsHeadSmash();

	virtual int	InternalDrawModel( int flags );

private:

	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkHandle( CTFPlayer, m_hPlayer );
	float m_fDeathTime;
	bool  m_bFadingOut;
	bool  m_bGib;
	bool  m_bBurning;
	bool  m_bElectrocuted;
	bool  m_bBatted;
	bool  m_bDissolving;
	bool  m_bFeignDeath;
	bool  m_bWasDisguised;
	bool  m_bCloaked;
	bool  m_bBecomeAsh;
	int	  m_iDamageCustom;
	bool  m_bGoldRagdoll;
	bool  m_bIceRagdoll;
	CountdownTimer m_freezeTimer;
	CountdownTimer m_frozenTimer;
	int	  m_iTeam;
	int	  m_iClass;
	float m_flBurnEffectStartTime;	// start time of burning, or 0 if not burning
	bool  m_bRagdollOn;
	bool  m_bDeathAnim;
	bool  m_bOnGround;
	bool  m_bFixedConstraints;
	matrix3x4_t m_mHeadAttachment;
	bool  m_bBaseTransform;
	float m_flPercentInvisible;
	float m_flTimeToDissolve;
	bool  m_bCritOnHardHit;	// plays the red mist particle effect
	float m_flHeadScale;
	float m_flTorsoScale;
	float m_flHandScale;

	CMaterialReference		m_MaterialOverride;

	CUtlVector<CHandle<CEconWearable > > m_hRagWearables;		// These look like they are no longer used?

	CUtlVector< CHandle< CEconWearable > > m_hClientWearables;	// wearables on the ragdoll that are "following" it

	bool  m_bCreatedWhilePlaybackSkipping;
};

#endif // C_TF_PLAYER_H
