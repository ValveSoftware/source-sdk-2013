//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The downtrodden citizens of City 17. Timid when unarmed, they will
//			rise up against their Combine oppressors when given a weapon.
//
//=============================================================================//

#ifndef	NPC_CITIZEN_H
#define	NPC_CITIZEN_H

#include "npc_playercompanion.h"

#include "ai_behavior_functank.h"

struct SquadCandidate_t;

//-----------------------------------------------------------------------------
//
// CLASS: CNPC_Citizen
//
//-----------------------------------------------------------------------------

//-------------------------------------
// Spawnflags
//-------------------------------------

#define SF_CITIZEN_FOLLOW			( 1 << 16 )	//65536 follow the player as soon as I spawn.
#define	SF_CITIZEN_MEDIC			( 1 << 17 )	//131072
#define SF_CITIZEN_RANDOM_HEAD		( 1 << 18 )	//262144
#define SF_CITIZEN_AMMORESUPPLIER	( 1 << 19 )	//524288
#define SF_CITIZEN_NOT_COMMANDABLE	( 1 << 20 ) //1048576
#define SF_CITIZEN_IGNORE_SEMAPHORE ( 1 << 21 ) //2097152		Work outside the speech semaphore system
#define SF_CITIZEN_RANDOM_HEAD_MALE	( 1 << 22 )	//4194304
#define SF_CITIZEN_RANDOM_HEAD_FEMALE ( 1 << 23 )//8388608
#define SF_CITIZEN_USE_RENDER_BOUNDS ( 1 << 24 )//16777216

//-------------------------------------
// Animation events
//-------------------------------------

enum CitizenType_t
{
	CT_DEFAULT,
	CT_DOWNTRODDEN,
	CT_REFUGEE,
	CT_REBEL,
	CT_UNIQUE
};

//-----------------------------------------------------------------------------
// Citizen expression types
//-----------------------------------------------------------------------------
enum CitizenExpressionTypes_t
{
	CIT_EXP_UNASSIGNED,	// Defaults to this, selects other in spawn.

	CIT_EXP_SCARED,
	CIT_EXP_NORMAL,
	CIT_EXP_ANGRY,

	CIT_EXP_LAST_TYPE,
};

//-------------------------------------

class CNPC_Citizen : public CNPC_PlayerCompanion
{
	DECLARE_CLASS( CNPC_Citizen, CNPC_PlayerCompanion );
public:
	CNPC_Citizen()
	 :	m_iHead( -1 )
	{
	}

	//---------------------------------
	bool			CreateBehaviors();
	void			Precache();
	void			PrecacheAllOfType( CitizenType_t );
	void			Spawn();
	void			PostNPCInit();
	virtual void	SelectModel();
	void			SelectExpressionType();
	void			Activate();
	virtual void	OnGivenWeapon( CBaseCombatWeapon *pNewWeapon );
	void			FixupMattWeapon();

#ifdef HL2_EPISODIC
	virtual float	GetJumpGravity() const		{ return 1.8f; }
#endif//HL2_EPISODIC

	void			OnRestore();
	
	//---------------------------------
	string_t 		GetModelName() const;
	
	Class_T 		Classify();

	bool 			ShouldAlwaysThink();

	//---------------------------------
	// Behavior
	//---------------------------------
	bool			ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior );
	void 			GatherConditions();
	void			PredictPlayerPush();
	void 			PrescheduleThink();
	void			BuildScheduleTestBits();

	bool			FInViewCone( CBaseEntity *pEntity );

	int				SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int				SelectSchedule();

	int 			SelectSchedulePriorityAction();
	int 			SelectScheduleHeal();
	int 			SelectScheduleRetrieveItem();
	int 			SelectScheduleNonCombat();
	int 			SelectScheduleManhackCombat();
	int 			SelectScheduleCombat();
	bool			ShouldDeferToFollowBehavior();
	int 			TranslateSchedule( int scheduleType );

	bool			ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal );
	void			OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal );
	
	void 			StartTask( const Task_t *pTask );
	void 			RunTask( const Task_t *pTask );
	
	Activity		NPC_TranslateActivity( Activity eNewActivity );
	void 			HandleAnimEvent( animevent_t *pEvent );
	void			TaskFail( AI_TaskFailureCode_t code );

	void 			PickupItem( CBaseEntity *pItem );

	void 			SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	bool			IgnorePlayerPushing( void );

	int				DrawDebugTextOverlays( void );

	virtual const char *SelectRandomExpressionForState( NPC_STATE state );

	//---------------------------------
	// Combat
	//---------------------------------
	bool 			OnBeginMoveAndShoot();
	void 			OnEndMoveAndShoot();
	
	virtual bool	UseAttackSquadSlots()	{ return false; }
	void 			LocateEnemySound();

	bool			IsManhackMeleeCombatant();
	
	Vector 			GetActualShootPosition( const Vector &shootOrigin );
	void 			OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	bool			ShouldLookForBetterWeapon();


	//---------------------------------
	// Damage handling
	//---------------------------------
	int 			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	
	//---------------------------------
	// Commander mode
	//---------------------------------
	bool 			IsCommandable();
	bool			IsPlayerAlly( CBasePlayer *pPlayer = NULL );
	bool			CanJoinPlayerSquad();
	bool			WasInPlayerSquad();
	bool			HaveCommandGoal() const;
	bool			IsCommandMoving();
	bool			ShouldAutoSummon();
	bool 			IsValidCommandTarget( CBaseEntity *pTarget );
	bool 			NearCommandGoal();
	bool 			VeryFarFromCommandGoal();
	bool 			TargetOrder( CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies );
	void 			MoveOrder( const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies );
	void			OnMoveOrder();
	void 			CommanderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool			ShouldSpeakRadio( CBaseEntity *pListener );
	void			OnMoveToCommandGoalFailed();
	void			AddToPlayerSquad();
	void			RemoveFromPlayerSquad();
	void 			TogglePlayerSquadState();
	void			UpdatePlayerSquad();
	static int __cdecl PlayerSquadCandidateSortFunc( const SquadCandidate_t *, const SquadCandidate_t * );
	void 			FixupPlayerSquad();
	void 			ClearFollowTarget();
	void 			UpdateFollowCommandPoint();
	bool			IsFollowingCommandPoint();
	CAI_BaseNPC *	GetSquadCommandRepresentative();
	void			SetSquad( CAI_Squad *pSquad );
	void			AddInsignia();
	void			RemoveInsignia();
	bool			SpeakCommandResponse( AIConcept_t concept, const char *modifiers = NULL );
	
	//---------------------------------
	// Scanner interaction
	//---------------------------------
	float 			GetNextScannerInspectTime() { return m_fNextInspectTime; }
	void			SetNextScannerInspectTime( float flTime ) { m_fNextInspectTime = flTime; }
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	
	//---------------------------------
	// Hints
	//---------------------------------
	bool			FValidateHintType ( CAI_Hint *pHint );

	//---------------------------------
	// Special abilities
	//---------------------------------
	bool 			IsMedic() 			{ return HasSpawnFlags(SF_CITIZEN_MEDIC); }
	bool 			IsAmmoResupplier() 	{ return HasSpawnFlags(SF_CITIZEN_AMMORESUPPLIER); }
	
	bool 			CanHeal();
	bool 			ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse = false );
#if HL2_EPISODIC
	bool 			ShouldHealTossTarget( CBaseEntity *pTarget, bool bActiveUse = false );
#endif
	void 			Heal();

	bool			ShouldLookForHealthItem();

#if HL2_EPISODIC
	void			TossHealthKit( CBaseCombatCharacter *pThrowAt, const Vector &offset ); // create a healthkit and throw it at someone
	void			InputForceHealthKitToss( inputdata_t &inputdata );
#endif
	
	//---------------------------------
	// Inputs
	//---------------------------------
	void			InputRemoveFromPlayerSquad( inputdata_t &inputdata ) { RemoveFromPlayerSquad(); }
	void 			InputStartPatrolling( inputdata_t &inputdata );
	void 			InputStopPatrolling( inputdata_t &inputdata );
	void			InputSetCommandable( inputdata_t &inputdata );
	void			InputSetMedicOn( inputdata_t &inputdata );
	void			InputSetMedicOff( inputdata_t &inputdata );
	void			InputSetAmmoResupplierOn( inputdata_t &inputdata );
	void			InputSetAmmoResupplierOff( inputdata_t &inputdata );
	void			InputSpeakIdleResponse( inputdata_t &inputdata );

	//---------------------------------
	//	Sounds & speech
	//---------------------------------
	void			FearSound( void );
	void			DeathSound( const CTakeDamageInfo &info );
	bool			UseSemaphore( void );

	virtual void	OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

private:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		COND_CIT_PLAYERHEALREQUEST = BaseClass::NEXT_CONDITION,
		COND_CIT_COMMANDHEAL,
		COND_CIT_HURTBYFIRE,
		COND_CIT_START_INSPECTION,
		
		SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY = BaseClass::NEXT_SCHEDULE,
		SCHED_CITIZEN_HEAL,
		SCHED_CITIZEN_RANGE_ATTACK1_RPG,
		SCHED_CITIZEN_PATROL,
		SCHED_CITIZEN_MOURN_PLAYER,
		SCHED_CITIZEN_SIT_ON_TRAIN,
		SCHED_CITIZEN_STRIDER_RANGE_ATTACK1_RPG,
#ifdef HL2_EPISODIC
		SCHED_CITIZEN_HEAL_TOSS,
#endif
		
		TASK_CIT_HEAL = BaseClass::NEXT_TASK,
		TASK_CIT_RPG_AUGER,
		TASK_CIT_PLAY_INSPECT_SEQUENCE,
		TASK_CIT_SIT_ON_TRAIN,
		TASK_CIT_LEAVE_TRAIN,
		TASK_CIT_SPEAK_MOURNING,
#ifdef HL2_EPISODIC
		TASK_CIT_HEAL_TOSS,
#endif

	};

	//-----------------------------------------------------
	
	int				m_nInspectActivity;
	float			m_flNextFearSoundTime;
	float			m_flStopManhackFlinch;
	float			m_fNextInspectTime;		// Next time I'm allowed to get inspected by a scanner
	float			m_flPlayerHealTime;
	float			m_flNextHealthSearchTime; // Next time I'm allowed to look for a healthkit
	float			m_flAllyHealTime;
	float			m_flPlayerGiveAmmoTime;
	string_t		m_iszAmmoSupply;
	int				m_iAmmoAmount;
	bool			m_bRPGAvoidPlayer;
	bool			m_bShouldPatrol;
	string_t		m_iszOriginalSquad;
	float			m_flTimeJoinedPlayerSquad;
	bool			m_bWasInPlayerSquad;
	float			m_flTimeLastCloseToPlayer;
	string_t		m_iszDenyCommandConcept;

	CSimpleSimTimer	m_AutoSummonTimer;
	Vector			m_vAutoSummonAnchor;

	CitizenType_t	m_Type;
	CitizenExpressionTypes_t	m_ExpressionType;

	int				m_iHead;

	static CSimpleSimTimer gm_PlayerSquadEvaluateTimer;

	float			m_flTimePlayerStare;	// The game time at which the player started staring at me.
	float			m_flTimeNextHealStare;	// Next time I'm allowed to heal a player who is staring at me.

	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent		m_OnJoinedPlayerSquad;
	COutputEvent		m_OnLeftPlayerSquad;
	COutputEvent		m_OnFollowOrder;
	COutputEvent		m_OnStationOrder; 
	COutputEvent		m_OnPlayerUse;
	COutputEvent		m_OnNavFailBlocked;

	//-----------------------------------------------------
	CAI_FuncTankBehavior	m_FuncTankBehavior;

	CHandle<CAI_FollowGoal>	m_hSavedFollowGoalEnt;

	bool					m_bNotifyNavFailBlocked;
	bool					m_bNeverLeavePlayerSquad; // Don't leave the player squad unless killed, or removed via Entity I/O. 
	
	//-----------------------------------------------------
	
	DECLARE_DATADESC();
#ifdef _XBOX
protected:
#endif
	DEFINE_CUSTOM_AI;
};

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_Citizen::NearCommandGoal()
{
	const float flDistSqr = COMMAND_GOAL_TOLERANCE * COMMAND_GOAL_TOLERANCE;
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() <= flDistSqr );
}

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_Citizen::VeryFarFromCommandGoal()
{
	const float flDistSqr = (12*50) * (12*50);
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() > flDistSqr );
}



//==============================================================================
// CITIZEN PLAYER-RESPONSE SYSTEM
//
// NOTE: This system is obsolete, and left here for legacy support.
//		 It has been superseded by the ai_eventresponse system.
//
//==============================================================================
#define CITIZEN_RESPONSE_DISTANCE			768			// Maximum distance for responding citizens
#define CITIZEN_RESPONSE_REFIRE_TIME		15.0		// Time after giving a response before giving any more
#define CITIZEN_RESPONSE_GIVEUP_TIME		4.0			// Time after a response trigger was fired before discarding it without responding

enum citizenresponses_t
{
	CR_PLAYER_SHOT_GUNSHIP,		// Player has shot the gunship with a bullet weapon
	CR_PLAYER_KILLED_GUNSHIP,	// Player has destroyed the gunship
	CR_VITALNPC_DIED,			// Mapmaker specified that an NPC that was vital has died

	// Add new responses here

	MAX_CITIZEN_RESPONSES,
};

//-------------------------------------

class CCitizenResponseSystem : public CBaseEntity
{
	DECLARE_CLASS( CCitizenResponseSystem, CBaseEntity );
public:
	DECLARE_DATADESC();

	void	Spawn();
	void	OnRestore();

	void	AddResponseTrigger( citizenresponses_t	iTrigger );

	void	ResponseThink();

	//---------------------------------
	// Inputs
	//---------------------------------
	void 	InputResponseVitalNPC( inputdata_t &inputdata );

private:
	float	m_flResponseAddedTime[ MAX_CITIZEN_RESPONSES ];		// Time at which the response was added. 0 if we have no response.
	float	m_flNextResponseTime;
};

//-------------------------------------

class CSquadInsignia : public CBaseAnimating
{
	DECLARE_CLASS( CSquadInsignia, CBaseAnimating );
	void Spawn();
};

//-------------------------------------

CCitizenResponseSystem	*GetCitizenResponse();

//-----------------------------------------------------------------------------

#endif	//NPC_CITIZEN_H
