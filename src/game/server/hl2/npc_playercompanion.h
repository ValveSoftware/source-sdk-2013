//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for humanoid NPCs intended to fight along side player in close
// environments
//
//=============================================================================//

#ifndef NPC_PLAYERCOMPANION_H
#define NPC_PLAYERCOMPANION_H

#include "ai_playerally.h"

#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_actbusy.h"
#include "ai_behavior_fear.h"

#ifdef HL2_EPISODIC
#include "ai_behavior_operator.h"
#include "ai_behavior_passenger_companion.h"
#endif

#if defined( _WIN32 )
#pragma once
#endif

enum AIReadiness_t
{
	AIRL_PANIC = -2,
	AIRL_STEALTH = -1,
	AIRL_RELAXED = 0,
	AIRL_STIMULATED,
	AIRL_AGITATED,
};

enum AIReadinessUse_t
{
	AIRU_NEVER,
	AIRU_ALWAYS,
	AIRU_ONLY_PLAYER_SQUADMATES,
};


class CCompanionActivityRemap : public CActivityRemap
{
public:
	CCompanionActivityRemap( void ) : 
	  m_fUsageBits( 0 ),
	  m_readiness( AIRL_RELAXED ),
	  m_bAiming( false ),
	  m_bWeaponRequired( false ),
	  m_bInVehicle( false ) {} 

	// This bitfield maps which bits of data are being utilized by this data structure, since not all criteria
	// in the parsed file are essential.  You must add corresponding bits to the definitions below and maintain
	// their state in the parsing of the file, as well as check the bitfield before accessing the data.  This
	// could be encapsulated into this class, but we'll probably move away from this model and closer to something
	// more akin to the response rules -- jdw

	int				m_fUsageBits;

	AIReadiness_t	m_readiness;
	bool			m_bAiming;
	bool			m_bWeaponRequired;
	bool			m_bInVehicle;		// For future expansion, this needs to speak more to the exact seat, role, and vehicle
};

// Usage bits for remap "extra" parsing - if these bits are set, the associated data has changed
#define bits_REMAP_READINESS		(1<<0)
#define bits_REMAP_AIMING			(1<<1)
#define bits_REMAP_WEAPON_REQUIRED	(1<<2)
#define bits_REMAP_IN_VEHICLE		(1<<3)

// Readiness modes that only change due to mapmaker scripts
#define READINESS_MIN_VALUE			-2
#define READINESS_MODE_PANIC		-2
#define READINESS_MODE_STEALTH		-1

// Readiness modes that change normally
#define READINESS_VALUE_RELAXED		0.1f
#define READINESS_VALUE_STIMULATED	0.95f
#define READINESS_VALUE_AGITATED	1.0f

class CPhysicsProp;

//-----------------------------------------------------------------------------
//
// CLASS: CNPC_PlayerCompanion
//
//-----------------------------------------------------------------------------

class CNPC_PlayerCompanion : public CAI_PlayerAlly
{
	DECLARE_CLASS( CNPC_PlayerCompanion, CAI_PlayerAlly );

public:
	//---------------------------------
	bool			CreateBehaviors();
	void			Precache();
	void			Spawn();
	virtual void	SelectModel() {};

	virtual int		Restore( IRestore &restore );
	virtual void	DoCustomSpeechAI( void );

	//---------------------------------
	int 			ObjectCaps();
	bool 			ShouldAlwaysThink();

	Disposition_t	IRelationType( CBaseEntity *pTarget );
	
	bool			IsSilentSquadMember() const;

	//---------------------------------
	// Behavior
	//---------------------------------
	void 			GatherConditions();
	virtual void	PredictPlayerPush();
	void			BuildScheduleTestBits();

	CSound			*GetBestSound( int validTypes = ALL_SOUNDS );
	bool			QueryHearSound( CSound *pSound );
	bool			QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );
	bool			ShouldIgnoreSound( CSound * );
	
	int 			SelectSchedule();

	virtual int 	SelectScheduleDanger();
	virtual int 	SelectSchedulePriorityAction();
	virtual int 	SelectScheduleNonCombat()			{ return SCHED_NONE; }
	virtual int 	SelectScheduleCombat();
	int 			SelectSchedulePlayerPush();

	virtual bool	CanReload( void );

	virtual bool	ShouldDeferToFollowBehavior();
	bool			ShouldDeferToPassengerBehavior( void );

	bool			IsValidReasonableFacing( const Vector &vecSightDir, float sightDist );
	
	int 			TranslateSchedule( int scheduleType );
	
	void 			StartTask( const Task_t *pTask );
	void 			RunTask( const Task_t *pTask );
	
	Activity		TranslateActivityReadiness( Activity activity );
	Activity		NPC_TranslateActivity( Activity eNewActivity );
	void 			HandleAnimEvent( animevent_t *pEvent );
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	int				GetSoundInterests();
	
	void 			Touch( CBaseEntity *pOther );

	virtual bool	IgnorePlayerPushing( void );

	void			ModifyOrAppendCriteria( AI_CriteriaSet& set );
	void			Activate( void );

	void			PrepareReadinessRemap( void );
	
	virtual bool	IsNavigationUrgent( void );

	//---------------------------------
	// Readiness
	//---------------------------------

protected:
	virtual bool	IsReadinessCapable();
	bool			IsReadinessLocked() { return gpGlobals->curtime < m_flReadinessLockedUntil; }
	void			AddReadiness( float flAdd, bool bOverrideLock = false );
	void			SubtractReadiness( float flAdd, bool bOverrideLock = false );
	void			SetReadinessValue( float flSet );
	void			SetReadinessSensitivity( float flSensitivity ) { m_flReadinessSensitivity = flSensitivity; }
	virtual void	UpdateReadiness();
	virtual float	GetReadinessDecay();
	bool			IsInScriptedReadinessState( void ) { return (m_flReadiness < 0 ); }

	CUtlVector< CCompanionActivityRemap > m_activityMappings;

public:
	float			GetReadinessValue()	{ return m_flReadiness; }
	int				GetReadinessLevel();
	void			SetReadinessLevel( int iLevel, bool bOverrideLock, bool bSlam );
	void			LockReadiness( float duration = -1.0f ); // Defaults to indefinitely locked
	void			UnlockReadiness( void );

	virtual			void ReadinessLevelChanged( int iPriorLevel ) { 	}

	void			InputGiveWeapon( inputdata_t &inputdata );

#ifdef HL2_EPISODIC
	//---------------------------------
	// Vehicle passenger
	//---------------------------------
	void			InputEnterVehicle( inputdata_t &inputdata );
	void			InputEnterVehicleImmediately( inputdata_t &inputdata );
	void			InputCancelEnterVehicle( inputdata_t &inputdata );
	void			InputExitVehicle( inputdata_t &inputdata );
	bool			CanEnterVehicle( void );
	bool			CanExitVehicle( void );
	void			EnterVehicle( CBaseEntity *pEntityVehicle, bool bImmediately );
	virtual bool	ExitVehicle( void );

	virtual void	UpdateEfficiency( bool bInPVS );
	virtual bool	IsInAVehicle( void ) const;
	virtual	IServerVehicle *GetVehicle( void );
	virtual CBaseEntity *GetVehicleEntity( void );

	virtual bool CanRunAScriptedNPCInteraction( bool bForced = false );
	virtual bool IsAllowedToDodge( void );

#endif // HL2_EPISODIC

public:

	virtual void	OnPlayerKilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	//---------------------------------
	//---------------------------------
	bool PickTacticalLookTarget( AILookTargetArgs_t *pArgs );

	//---------------------------------
	// Aiming
	//---------------------------------
	CBaseEntity		*GetAimTarget() { return m_hAimTarget; }
	void			SetAimTarget( CBaseEntity *pTarget );
	void			StopAiming( char *pszReason = NULL );
	bool			FindNewAimTarget();
	void			OnNewLookTarget();
	bool			ShouldBeAiming();
	virtual bool	IsAllowedToAim();
	bool			HasAimLOS( CBaseEntity *pAimTarget );
	void			AimGun();
	CBaseEntity		*GetAlternateMoveShootTarget();

	//---------------------------------
	// Combat
	//---------------------------------
	virtual void 	LocateEnemySound() {};

	bool			IsValidEnemy( CBaseEntity *pEnemy );

	bool 			IsSafeFromFloorTurret( const Vector &vecLocation, CBaseEntity *pTurret );

	bool			ShouldMoveAndShoot( void );
	void			OnUpdateShotRegulator();

	void			DecalTrace( trace_t *pTrace, char const *decalName );
	bool 			FCanCheckAttacks();
	Vector 			GetActualShootPosition( const Vector &shootOrigin );
	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );
	bool			ShouldLookForBetterWeapon();
	bool			Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	void			Weapon_Equip( CBaseCombatWeapon *pWeapon );
	void			PickupWeapon( CBaseCombatWeapon *pWeapon );
	
	bool 			FindCoverPos( CBaseEntity *pEntity, Vector *pResult);
	bool			FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult );
	bool			FindCoverPos( CSound *pSound, Vector *pResult );
	bool			FindMortarCoverPos( CSound *pSound, Vector *pResult );
	bool 			IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition );

	bool			IsEnemyTurret() { return ( GetEnemy() && IsTurret(GetEnemy()) ); }
	
	static bool		IsMortar( CBaseEntity *pEntity );
	static bool		IsSniper( CBaseEntity *pEntity );
	static bool		IsTurret(  CBaseEntity *pEntity );
	static bool		IsGunship( CBaseEntity *pEntity );
	
	//---------------------------------
	// Damage handling
	//---------------------------------
	int 			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void 			OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );

	//---------------------------------
	// Hints
	//---------------------------------
	bool			FValidateHintType ( CAI_Hint *pHint );

	//---------------------------------
	// Navigation
	//---------------------------------
	bool			IsValidMoveAwayDest( const Vector &vecDest );
	bool 			ValidateNavGoal();
	bool 			OverrideMove( float flInterval );				// Override to take total control of movement (return true if done so)
	bool			MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );
	float			GetIdealSpeed() const;
	float			GetIdealAccel() const;
	bool			OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	//---------------------------------
	// Inputs
	//---------------------------------
	void 			InputOutsideTransition( inputdata_t &inputdata );
	void			InputSetReadinessPanic( inputdata_t &inputdata );
	void			InputSetReadinessStealth( inputdata_t &inputdata );
	void			InputSetReadinessLow( inputdata_t &inputdata );
	void			InputSetReadinessMedium( inputdata_t &inputdata );
	void			InputSetReadinessHigh( inputdata_t &inputdata );
	void			InputLockReadiness( inputdata_t &inputdata );
#if HL2_EPISODIC
	void			InputClearAllOuputs( inputdata_t &inputdata ); ///< annihilate every output on this npc
#endif

	bool			AllowReadinessValueChange( void );

protected:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		COND_PC_HURTBYFIRE = BaseClass::NEXT_CONDITION,
		COND_PC_SAFE_FROM_MORTAR,
		COND_PC_BECOMING_PASSENGER,
		NEXT_CONDITION,

		SCHED_PC_COWER = BaseClass::NEXT_SCHEDULE,
		SCHED_PC_MOVE_TOWARDS_COVER_FROM_BEST_SOUND,
		SCHED_PC_TAKE_COVER_FROM_BEST_SOUND,
		SCHED_PC_FLEE_FROM_BEST_SOUND,
		SCHED_PC_FAIL_TAKE_COVER_TURRET,
		SCHED_PC_FAKEOUT_MORTAR,
		SCHED_PC_GET_OFF_COMPANION,
		NEXT_SCHEDULE,

		TASK_PC_WAITOUT_MORTAR = BaseClass::NEXT_TASK,
		TASK_PC_GET_PATH_OFF_COMPANION,
		NEXT_TASK,
	};

private:
	void SetupCoverSearch( CBaseEntity *pEntity );
	void CleanupCoverSearch();

	//-----------------------------------------------------
	
	bool			m_bMovingAwayFromPlayer;
	bool			m_bWeightPathsInCover;

	enum eCoverType
	{
		CT_NORMAL,
		CT_TURRET,
		CT_MORTAR
	};

	static eCoverType	gm_fCoverSearchType;
	static bool 		gm_bFindingCoverFromAllEnemies;

	CSimpleSimTimer		m_FakeOutMortarTimer;

	// Derived classes should not use the expresser directly
	virtual CAI_Expresser *GetExpresser()	{ return BaseClass::GetExpresser(); }

protected:
	//-----------------------------------------------------

	virtual CAI_FollowBehavior &GetFollowBehavior( void ) { return m_FollowBehavior; }

	CAI_AssaultBehavior				m_AssaultBehavior;
	CAI_FollowBehavior				m_FollowBehavior;
	CAI_StandoffBehavior			m_StandoffBehavior;
	CAI_LeadBehavior				m_LeadBehavior;
	CAI_ActBusyBehavior				m_ActBusyBehavior;
#ifdef HL2_EPISODIC
	CAI_OperatorBehavior			m_OperatorBehavior;
	CAI_PassengerBehaviorCompanion	m_PassengerBehavior;
	CAI_FearBehavior				m_FearBehavior;
#endif
	//-----------------------------------------------------

	bool	ShouldAlwaysTransition( void );

	// Readiness is a value that's fed by various events in the NPC's AI. It is used
	// to make decisions about what type of posture the NPC should be in (relaxed, agitated).
	// It is not used to make decisions about what to do in the AI. 
	float m_flReadiness;
	float m_flReadinessSensitivity;
	bool m_bReadinessCapable;
	float m_flReadinessLockedUntil;
	float	m_fLastBarrelExploded;
	float	m_fLastPlayerKill;
	int		m_iNumConsecutiveBarrelsExploded;  // Companions keep track of the # of consecutive barrels exploded by the player and speaks a response as it increases
	int		m_iNumConsecutivePlayerKills;  // Alyx keeps track of the # of consecutive kills by the player and speaks a response as it increases

	//-----------------------------------------------------

	float m_flBoostSpeed;

	//-----------------------------------------------------
	
	CSimpleSimTimer m_AnnounceAttackTimer;

	//-----------------------------------------------------

	EHANDLE m_hAimTarget;

#ifdef HL2_EPISODIC
	CHandle<CPhysicsProp>	m_hFlare;
#endif // HL2_EPISODIC

	//-----------------------------------------------------

	static string_t gm_iszMortarClassname;
	static string_t gm_iszFloorTurretClassname;
	static string_t gm_iszGroundTurretClassname;
	static string_t gm_iszShotgunClassname;
	static string_t	gm_iszRollerMineClassname;

	//-----------------------------------------------------

	void	InputEnableAlwaysTransition( inputdata_t &inputdata );
	void	InputDisableAlwaysTransition( inputdata_t &inputdata );
	bool	m_bAlwaysTransition;
	bool	m_bDontPickupWeapons;

	void	InputEnableWeaponPickup( inputdata_t &inputdata );
	void	InputDisableWeaponPickup( inputdata_t &inputdata );

	COutputEvent	m_OnWeaponPickup;

	CStopwatch		m_SpeechWatch_PlayerLooking;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

// Used for quick override move searches against certain types of entities
void OverrideMoveCache_ForceRepopulateList( void );
CBaseEntity *OverrideMoveCache_FindTargetsInRadius( CBaseEntity *pFirstEntity, const Vector &vecOrigin, float flRadius );
void OverrideMoveCache_LevelInitPreEntity( void );
void OverrideMoveCache_LevelShutdownPostEntity( void );

#endif // NPC_PLAYERCOMPANION_H
