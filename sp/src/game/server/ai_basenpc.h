//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base NPC character with AI
//
//=============================================================================//

#ifndef AI_BASENPC_H
#define AI_BASENPC_H

#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h" 	 	   
#include "basecombatcharacter.h"
#include "ai_debug.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_condition.h"
#include "ai_component.h"
#include "ai_task.h"
#include "ai_movetypes.h"
#include "ai_navtype.h"
#include "ai_namespaces.h"
#include "ai_npcstate.h"
#include "ai_hull.h"
#include "ai_utils.h"
#include "ai_moveshoot.h"
#include "entityoutput.h"
#include "utlvector.h"
#include "activitylist.h"
#include "bitstring.h"
#include "ai_basenpc.h"
#include "ai_navgoaltype.h" //GoalType_t enum
#include "eventlist.h"
#include "soundent.h"
#include "ai_navigator.h"
#include "tier1/functors.h"


#define PLAYER_SQUADNAME "player_squad"

class CAI_Schedule;
class CAI_Network;
class CAI_Route;
class CAI_Hint;
class CAI_Node;
class CAI_Navigator;
class CAI_Pathfinder;
class CAI_Senses;
class CAI_Enemies;
class CAI_Squad;
class CAI_Expresser;
class CAI_BehaviorBase;
class CAI_GoalEntity;
class CAI_Motor;
class CAI_MoveProbe;
class CAI_LocalNavigator;
class CAI_TacticalServices;
class CVarBitVec;
class CAI_ScriptedSequence;
class CSceneEntity;
class CBaseGrenade;
class CBaseDoor;
class CBasePropDoor;
struct AI_Waypoint_t;
#ifndef NEW_RESPONSE_SYSTEM
class AI_Response;
#endif
class CBaseFilter;

typedef CBitVec<MAX_CONDITIONS> CAI_ScheduleBits;

// Used to control optimizations mostly dealing with pathfinding for NPCs
extern ConVar ai_strong_optimizations;

extern bool AIStrongOpt( void );

// AI_MONITOR_FOR_OSCILLATION defaults to OFF. If you build with this ON, you can flag
// NPC's and monitor them to detect oscillations in their schedule (circular logic and conditions bugs)
// DO NOT SHIP WITH THIS ON!
#undef AI_MONITOR_FOR_OSCILLATION

//=============================================================================
//
// Constants & enumerations
//
//=============================================================================
#define TURRET_CLOSE_RANGE	200
#define TURRET_MEDIUM_RANGE 500

#define COMMAND_GOAL_TOLERANCE	48	// 48 inches.
#define TIME_CARE_ABOUT_DAMAGE	3.0

#define ITEM_PICKUP_TOLERANCE	48.0f

// Max's of the box used to search for a weapon to pick up. 45x45x~8 ft.
#define WEAPON_SEARCH_DELTA	Vector( 540, 540, 100 )

#ifdef MAPBASE
// Defines Mapbase's extended NPC response system usage.
#define EXPANDED_RESPONSE_SYSTEM_USAGE
#endif

#ifdef EXPANDED_RESPONSE_SYSTEM_USAGE

// This macro implements the response system on any NPC, particularly non-actors that can't use CAI_ExpresserHost.
// NOTE: Because of the lack of CAI_ExpresserHost, some Response System settings like odds, delays, etc. cannot be used.
// It's recommended to just use CAI_ExpresserHost if possible.
#define DeclareResponseSystem IResponseSystem *GetResponseSystem() { extern IResponseSystem *g_pResponseSystem; return g_pResponseSystem; }

// Default CAI_ExpresserHost implementation for NPCs using CAI_ExpresserHost.
#define DeclareDefaultExpresser() virtual CAI_Expresser *CreateExpresser( void ) { m_pExpresser = new CAI_Expresser(this); if (!m_pExpresser) return NULL; m_pExpresser->Connect(this); return m_pExpresser; } \\
		virtual CAI_Expresser *GetExpresser() { return m_pExpresser;  } \\
		virtual void		PostConstructor(const char *szClassname) { BaseClass::PostConstructor(szClassname); CreateExpresser(); } \\
	private: \\
		CAI_Expresser *m_pExpresser; \\
	public:

// Variant of DeclareDefaultExpresser() that doesn't implement its own PostConstructor.
// CreateExpresser() should still be called from there.
#define DeclareDefaultExpresser_ExistingPC() virtual CAI_Expresser *CreateExpresser( void ) { m_pExpresser = new CAI_Expresser(this); if (!m_pExpresser) return NULL; m_pExpresser->Connect(this); return m_pExpresser; } \\
		virtual CAI_Expresser *GetExpresser() { return m_pExpresser;  } \\
	private: \\
		CAI_Expresser *m_pExpresser; \\
	public:

#endif

enum Interruptability_t
{
	GENERAL_INTERRUPTABILITY,
	DAMAGEORDEATH_INTERRUPTABILITY,
	DEATH_INTERRUPTABILITY
};

//-------------------------------------
// Memory
//-------------------------------------

#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// npc knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define	bits_MEMORY_TASK_EXPENSIVE		( 1 << 3 )// NPC has completed a task which is considered costly, so don't do another task this frame
//#define	bits_MEMORY_				( 1 << 4 )
#define bits_MEMORY_PATH_FAILED			( 1 << 5 )// Failed to find a path
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
//#define bits_MEMORY_ 					( 1 << 7 )
#define bits_MEMORY_TOURGUIDE			( 1 << 8 )// I have been acting as a tourguide.
//#define bits_MEMORY_					( 1 << 9 )// 
#define bits_MEMORY_LOCKED_HINT			( 1 << 10 )// 
//#define bits_MEMORY_					( 1 << 12 )

#define bits_MEMORY_TURNING				( 1 << 13 )// Turning, don't interrupt me.
#define bits_MEMORY_TURNHACK			( 1 << 14 )

#define bits_MEMORY_HAD_ENEMY			( 1 << 15 )// Had an enemy
#define bits_MEMORY_HAD_PLAYER			( 1 << 16 )// Had player
#define bits_MEMORY_HAD_LOS				( 1 << 17 )// Had LOS to enemy

#define bits_MEMORY_MOVED_FROM_SPAWN	( 1 << 18 )// Has moved since spawning.

#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// NPC-specific memory

//-------------------------------------
// Spawn flags
//-------------------------------------
#define SF_NPC_WAIT_TILL_SEEN			( 1 << 0  )	// spawnflag that makes npcs wait until player can see them before attacking.
#define SF_NPC_GAG						( 1 << 1  )	// no idle noises from this npc
#define SF_NPC_FALL_TO_GROUND			( 1 << 2  )	// used my NPC_Maker
#define SF_NPC_DROP_HEALTHKIT			( 1 << 3  )	// Drop a healthkit upon death
#define SF_NPC_START_EFFICIENT			( 1 << 4  ) // Set into efficiency mode from spawn
//										( 1 << 5  )
//										( 1 << 6  )
#define SF_NPC_WAIT_FOR_SCRIPT			( 1 << 7  )	// spawnflag that makes npcs wait to check for attacking until the script is done or they've been attacked
#define SF_NPC_LONG_RANGE				( 1 << 8  )	// makes npcs look far and relaxes weapon range limit 
#define SF_NPC_FADE_CORPSE				( 1 << 9  )	// Fade out corpse after death
#define SF_NPC_ALWAYSTHINK				( 1 << 10 )	// Simulate even when player isn't in PVS.
#define SF_NPC_TEMPLATE					( 1 << 11 )	// This NPC will be used as a template by an npc_maker -- do not spawn.
#define SF_NPC_ALTCOLLISION				( 1 << 12 )
#define SF_NPC_NO_WEAPON_DROP			( 1 << 13 )	// This NPC will not actually drop a weapon that can be picked up
#define SF_NPC_NO_PLAYER_PUSHAWAY		( 1 << 14 )	
//										( 1 << 15 )	
// !! Flags above ( 1 << 15 )	 are reserved for NPC sub-classes

//-------------------------------------
//
// Return codes from CanPlaySequence.
//
//-------------------------------------

enum CanPlaySequence_t
{
	CANNOT_PLAY = 0,		// Can't play for any number of reasons.
	CAN_PLAY_NOW,			// Can play the script immediately.
	CAN_PLAY_ENQUEUED,		// Can play the script after I finish playing my current script.
};

//-------------------------------------
// Weapon holstering
//-------------------------------------
enum DesiredWeaponState_t
{
	DESIREDWEAPONSTATE_IGNORE = 0,
	DESIREDWEAPONSTATE_HOLSTERED,
	DESIREDWEAPONSTATE_HOLSTERED_DESTROYED, // Put the weapon away, then destroy it.
	DESIREDWEAPONSTATE_UNHOLSTERED,
	DESIREDWEAPONSTATE_CHANGING,
	DESIREDWEAPONSTATE_CHANGING_DESTROY,	// Destroy the weapon when this change is complete.
};

//-------------------------------------
//
// Efficiency modes
//
//-------------------------------------

enum AI_Efficiency_t
{
	// Run at full tilt
	AIE_NORMAL,

	// Run decision process less often
	AIE_EFFICIENT,

	// Run decision process even less often, ignore other NPCs
	AIE_VERY_EFFICIENT,

	// Run decision process even less often, ignore other NPCs
	AIE_SUPER_EFFICIENT,

	// Don't run at all
	AIE_DORMANT,
};

enum AI_MoveEfficiency_t
{
	AIME_NORMAL,
	AIME_EFFICIENT,
};

//-------------------------------------
//
// Sleep state
//
//-------------------------------------

enum AI_SleepState_t
{
	AISS_AWAKE,
	AISS_WAITING_FOR_THREAT,
	AISS_WAITING_FOR_PVS,
	AISS_WAITING_FOR_INPUT,
	AISS_AUTO_PVS,
	AISS_AUTO_PVS_AFTER_PVS, // Same as AUTO_PVS, except doesn't activate until/unless the NPC is IN the player's PVS. 
};

#define AI_SLEEP_FLAGS_NONE					0x00000000
#define AI_SLEEP_FLAG_AUTO_PVS				0x00000001
#define AI_SLEEP_FLAG_AUTO_PVS_AFTER_PVS	0x00000002


//-------------------------------------
//
// Debug bits
//
//-------------------------------------

enum DebugBaseNPCBits_e
{
	bits_debugDisableAI = 0x00000001,		// disable AI
	bits_debugStepAI	= 0x00000002,		// step AI

};

//-------------------------------------
//
// Base Sentence index for behaviors
//
//-------------------------------------
enum SentenceIndex_t
{
	SENTENCE_BASE_BEHAVIOR_INDEX = 1000,
};

#ifdef AI_MONITOR_FOR_OSCILLATION
struct AIScheduleChoice_t 
{
	float			m_flTimeSelected;
	CAI_Schedule	*m_pScheduleSelected;
};
#endif//AI_MONITOR_FOR_OSCILLATION

#define MARK_TASK_EXPENSIVE()	\
	if ( GetOuter() ) \
	{ \
		GetOuter()->Remember( bits_MEMORY_TASK_EXPENSIVE ); \
	}

//=============================================================================
//
// Types used by CAI_BaseNPC
//
//=============================================================================

struct AIScheduleState_t
{
	int					 iCurTask;
	TaskStatus_e		 fTaskStatus;
	float				 timeStarted;
	float				 timeCurTaskStarted;
	AI_TaskFailureCode_t taskFailureCode;
	int					 iTaskInterrupt;
	bool 				 bTaskRanAutomovement;
	bool 				 bTaskUpdatedYaw;
	bool				 bScheduleWasInterrupted;

	DECLARE_SIMPLE_DATADESC();
};

// -----------------------------------------
//	An entity that this NPC can't reach
// -----------------------------------------

struct UnreachableEnt_t
{
	EHANDLE	hUnreachableEnt;	// Entity that's unreachable
	float	fExpireTime;		// Time to forget this information
	Vector	vLocationWhenUnreachable;
	
	DECLARE_SIMPLE_DATADESC();
};

//=============================================================================
// SCRIPTED NPC INTERACTIONS
//=============================================================================
// -----------------------------------------
//	Scripted NPC interaction flags
// -----------------------------------------
#define SCNPC_FLAG_TEST_OTHER_ANGLES			( 1 << 1 )
#define SCNPC_FLAG_TEST_OTHER_VELOCITY			( 1 << 2 )
#define SCNPC_FLAG_LOOP_IN_ACTION				( 1 << 3 )
#define SCNPC_FLAG_NEEDS_WEAPON_ME				( 1 << 4 )
#define SCNPC_FLAG_NEEDS_WEAPON_THEM			( 1 << 5 )
#define SCNPC_FLAG_DONT_TELEPORT_AT_END_ME		( 1 << 6 )
#define SCNPC_FLAG_DONT_TELEPORT_AT_END_THEM	( 1 << 7 )
#ifdef MAPBASE
#define SCNPC_FLAG_MAPBASE_ADDITION				( 1 << 8 )
#define SCNPC_FLAG_TEST_END_POSITION			( 1 << 9 )
#endif

// -----------------------------------------
//	Scripted NPC interaction trigger methods
// -----------------------------------------
enum
{
	SNPCINT_CODE = 0,
	SNPCINT_AUTOMATIC_IN_COMBAT = 1,
};

// -----------------------------------------
//	Scripted NPC interaction loop breaking trigger methods
// -----------------------------------------
#define SNPCINT_LOOPBREAK_ON_DAMAGE				( 1 << 1 )
#define SNPCINT_LOOPBREAK_ON_FLASHLIGHT_ILLUM	( 1 << 2 )

// -----------------------------------------
//	Scripted NPC interaction anim phases
// -----------------------------------------
enum
{
	SNPCINT_ENTRY = 0,
	SNPCINT_SEQUENCE,
	SNPCINT_EXIT,

	SNPCINT_NUM_PHASES
};

struct ScriptedNPCInteraction_Phases_t
{
	string_t	iszSequence;
	int			iActivity;

	DECLARE_SIMPLE_DATADESC();
};

// Allowable delta from the desired dynamic scripted sequence point
#define DSS_MAX_DIST			6
#define DSS_MAX_ANGLE_DIFF		4

// Interaction Logic States
enum
{
	NPCINT_NOT_RUNNING = 0,
	NPCINT_RUNNING_ACTIVE,		// I'm in an interaction that I initiated
	NPCINT_RUNNING_PARTNER,		// I'm in an interaction that was initiated by the other NPC
	NPCINT_MOVING_TO_MARK,		// I'm moving to a position to do an interaction
};

#define NPCINT_NONE				-1

#define MAXTACLAT_IGNORE		-1

// -----------------------------------------
//	A scripted interaction between NPCs
// -----------------------------------------
struct ScriptedNPCInteraction_t
{
	ScriptedNPCInteraction_t()
	{
		iszInteractionName = NULL_STRING;
		iFlags = 0;
		iTriggerMethod = SNPCINT_CODE;
		iLoopBreakTriggerMethod = 0;
		vecRelativeOrigin = vec3_origin;
		bValidOnCurrentEnemy = false;
		flDelay = 5.0;
		flDistSqr = (DSS_MAX_DIST * DSS_MAX_DIST);
		flNextAttemptTime = 0;
		iszMyWeapon = NULL_STRING;
		iszTheirWeapon = NULL_STRING;
#ifdef MAPBASE
		vecRelativeEndPos = vec3_origin;
		bHasSeparateSequenceNames = false;
		flMaxAngleDiff = DSS_MAX_ANGLE_DIFF;
		iszRelatedInteractions = NULL_STRING;
		MiscCriteria = NULL_STRING;
#endif

		for ( int i = 0; i < SNPCINT_NUM_PHASES; i++)
		{
			sPhases[i].iszSequence = NULL_STRING;
			sPhases[i].iActivity = ACT_INVALID;
#ifdef MAPBASE
			sTheirPhases[i].iszSequence = NULL_STRING;
			sTheirPhases[i].iActivity = ACT_INVALID;
#endif
		}
	}

	// Fill out these when passing to AddScriptedNPCInteraction
	string_t	iszInteractionName;
	int			iFlags;
	int			iTriggerMethod;
	int			iLoopBreakTriggerMethod;
	Vector		vecRelativeOrigin;			// (forward, right, up)
	QAngle		angRelativeAngles;				
	Vector		vecRelativeVelocity;		// Desired relative velocity of the other NPC
#ifdef MAPBASE
	Vector		vecRelativeEndPos;			// Relative position that the NPC must fit in
#endif
	float		flDelay;					// Delay before interaction can be used again
	float		flDistSqr;					// Max distance sqr from the relative origin the NPC is allowed to be to trigger
	string_t	iszMyWeapon;				// Classname of the weapon I'm holding, if any
	string_t	iszTheirWeapon;				// Classname of the weapon my interaction partner is holding, if any
	ScriptedNPCInteraction_Phases_t sPhases[SNPCINT_NUM_PHASES];

	// These will be filled out for you in AddScriptedNPCInteraction
	VMatrix		matDesiredLocalToWorld;		// Desired relative position / angles of the other NPC
	bool		bValidOnCurrentEnemy;

	float		flNextAttemptTime;

#ifdef MAPBASE
	ScriptedNPCInteraction_Phases_t sTheirPhases[SNPCINT_NUM_PHASES];	// The animations played by the target NPC, if they are different
	bool		bHasSeparateSequenceNames;

	float		flMaxAngleDiff;
	string_t	iszRelatedInteractions;	// These interactions will be delayed as well when this interaction is used.

	// Unrecognized keyvalues which are tested against response criteria later.
	string_t	MiscCriteria;
#endif

	DECLARE_SIMPLE_DATADESC();
};

//=============================================================================
//
// Utility functions
//
//=============================================================================

Vector VecCheckToss ( CBaseEntity *pEdict, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins = NULL, Vector *vecMaxs = NULL );
Vector VecCheckToss ( CBaseEntity *pEntity, ITraceFilter *pFilter, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins = NULL, Vector *vecMaxs = NULL );
Vector VecCheckThrow( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0f, Vector *vecMins = NULL, Vector *vecMaxs = NULL );

extern Vector g_vecAttackDir;

bool FBoxVisible ( CBaseEntity *pLooker, CBaseEntity *pTarget );
bool FBoxVisible ( CBaseEntity *pLooker, CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize = 0.0 );

// FIXME: move to utils?
float DeltaV( float v0, float v1, float d );
float ChangeDistance( float flInterval, float flGoalDistance, float flGoalVelocity, float flCurVelocity, float flIdealVelocity, float flAccelRate, float &flNewDistance, float &flNewVelocity );

//=============================================================================
//
// class CAI_Manager
//
// Central location for components of the AI to operate across all AIs without
// iterating over the global list of entities.
//
//=============================================================================

class CAI_Manager
{
public:
	CAI_Manager();
	
	CAI_BaseNPC **	AccessAIs();
	int				NumAIs();
	
	void AddAI( CAI_BaseNPC *pAI );
	void RemoveAI( CAI_BaseNPC *pAI );

	bool FindAI( CAI_BaseNPC *pAI )	{ return ( m_AIs.Find( pAI ) != m_AIs.InvalidIndex() ); }
	
private:
	enum
	{
		MAX_AIS = 256
	};
	
	typedef CUtlVector<CAI_BaseNPC *> CAIArray;
	
	CAIArray m_AIs;

};

//-------------------------------------

extern CAI_Manager g_AI_Manager;

//=============================================================================
//
//	class CAI_BaseNPC
//
//=============================================================================

class CAI_BaseNPC : public CBaseCombatCharacter, 
					public CAI_DefMovementSink
{
	DECLARE_CLASS( CAI_BaseNPC, CBaseCombatCharacter );

public:
	//-----------------------------------------------------
	//
	// Initialization, cleanup, serialization, identity
	//
	
	CAI_BaseNPC();
	~CAI_BaseNPC();

	//---------------------------------
	
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
#ifdef MAPBASE_VSCRIPT
	DECLARE_ENT_SCRIPTDESC();
#endif

	virtual int			Save( ISave &save ); 
	virtual int			Restore( IRestore &restore );
	virtual void		OnRestore();
	void				SaveConditions( ISave &save, const CAI_ScheduleBits &conditions );
	void				RestoreConditions( IRestore &restore, CAI_ScheduleBits *pConditions );

	bool				ShouldSavePhysics()	{ return false; }
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
#ifdef MAPBASE
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );
#endif

	//---------------------------------
	
	virtual void		PostConstructor( const char *szClassname );
	virtual void		Activate( void );
	virtual void		Precache( void ); // derived calls at start of Spawn()
	virtual bool 		CreateVPhysics();
	virtual void		NPCInit( void ); // derived calls after Spawn()
	void				NPCInitThink( void );
	virtual void		PostNPCInit() {};// called after NPC_InitThink
	virtual void		StartNPC( void );
	virtual bool		IsTemplate( void );

	virtual void		CleanupOnDeath( CBaseEntity *pCulprit = NULL, bool bFireDeathOutput = true );
	virtual void		UpdateOnRemove( void );

	virtual int			UpdateTransmitState();

	//---------------------------------
	// Component creation factories
	// 
	
	// The master call, override if you introduce new component types. Call base first
	virtual bool 			CreateComponents();
	
	// Components defined by the base AI class
	virtual CAI_Senses *	CreateSenses();
	virtual CAI_MoveProbe *	CreateMoveProbe();
	virtual CAI_Motor *		CreateMotor();
	virtual CAI_LocalNavigator *CreateLocalNavigator();
	virtual CAI_Navigator *	CreateNavigator();
	virtual CAI_Pathfinder *CreatePathfinder();
	virtual CAI_TacticalServices *CreateTacticalServices();

	//---------------------------------

	virtual bool			IsNPC( void ) const { return true; }

	//---------------------------------

	void TestPlayerPushing( CBaseEntity *pPlayer );
	void CascadePlayerPush( const Vector &push, const Vector &pushOrigin );
	void NotifyPushMove();

public:
	//-----------------------------------------------------
	//
	// AI processing - thinking, schedule selection and task running
	//
	//-----------------------------------------------------
	void				CallNPCThink( void );
	
	// Thinking, including core thinking, movement, animation
	virtual void		NPCThink( void );

#ifdef MAPBASE
	void				InputSetThinkNPC( inputdata_t &inputdata );
#endif

	// Core thinking (schedules & tasks)
	virtual void		RunAI( void );// core ai function!	

	// Called to gather up all relevant conditons
	virtual void		GatherConditions( void );

	// Called immediately prior to schedule processing
	virtual void		PrescheduleThink( void );

	// Called immediately after schedule processing
	virtual void		PostscheduleThink( void ) { return; };

	// Notification that the current schedule, if any, is ending and a new one is being selected
	virtual void		OnScheduleChange( void );

	// Notification that a new schedule is about to run its first task
	virtual void		OnStartSchedule( int scheduleType ) {};

	// This function implements a decision tree for the NPC.  It is responsible for choosing the next behavior (schedule)
	// based on the current conditions and state.
	virtual int			SelectSchedule( void );
	virtual int			SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	// After the schedule has been selected, it will be processed by this function so child NPC classes can 
	// remap base schedules into child-specific behaviors
	virtual int			TranslateSchedule( int scheduleType );

	virtual void		StartTask( const Task_t *pTask );
	virtual void		RunTask( const Task_t *pTask );

	void				ClearTransientConditions();

	virtual void		HandleAnimEvent( animevent_t *pEvent );

	virtual bool		IsInterruptable();
	virtual void		OnStartScene( void ) {}	// Called when an NPC begins a cine scene (useful for clean-up)
	virtual bool		ShouldPlayerAvoid( void );
	virtual void		SetPlayerAvoidState( void );
	virtual void		PlayerPenetratingVPhysics( void );

	virtual bool		ShouldAlwaysThink();
	void				ForceGatherConditions()	{ m_bForceConditionsGather = true; SetEfficiency( AIE_NORMAL ); }	// Force an NPC out of PVS to call GatherConditions on next think
	bool				IsForceGatherConditionsSet() { return m_bForceConditionsGather; }

	virtual float		LineOfSightDist( const Vector &vecDir = vec3_invalid, float zEye = FLT_MAX );

	virtual void		MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual const char	*GetTracerType( void );
	virtual void		DoImpactEffect( trace_t &tr, int nDamageType );
		
	enum
	{
		NEXT_SCHEDULE 	= LAST_SHARED_SCHEDULE,
		NEXT_TASK		= LAST_SHARED_TASK,
		NEXT_CONDITION 	= LAST_SHARED_CONDITION,
	};

protected:
	// Used by derived classes to chain a task to a task that might not be the 
	// one they are currently handling:
	void				ChainStartTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; StartTask( (const Task_t *)&tempTask ); }
	void				ChainRunTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; RunTask( (const Task_t *)	&tempTask );	}

	void				StartTaskOverlay();
	void				RunTaskOverlay();
	void				EndTaskOverlay();

	virtual void		PostRunStopMoving();

	bool				CheckPVSCondition();

private:
	bool				CanThinkRebalance();
	void				RebalanceThinks();

	bool				PreNPCThink();
	void				PostNPCThink();

	bool				PreThink( void );
	void				PerformSensing();
	void				CheckOnGround( void );
	void				MaintainSchedule( void );
	void				RunAnimation( void );
	void				PostRun( void );
	void				PerformMovement();
	void				PostMovement();
	
	virtual int			StartTask ( Task_t *pTask ) { DevMsg( "Called wrong StartTask()\n" ); StartTask( (const Task_t *)pTask ); return 0; } // to ensure correct signature in derived classes
	virtual int			RunTask ( Task_t *pTask )	{ DevMsg( "Called wrong RunTask()\n" ); RunTask( (const Task_t *)pTask ); return 0; } // to ensure correct signature in derived classes

public:
	//-----------------------------------------------------
	//
	// Schedules & tasks
	//
	//-----------------------------------------------------
	
	void				SetSchedule( CAI_Schedule *pNewSchedule );
	bool				SetSchedule( int localScheduleID );
	
	void				SetDefaultFailSchedule( int failSchedule )	{ m_failSchedule = failSchedule; }
	
	void				ClearSchedule( const char *szReason );
	
	CAI_Schedule *		GetCurSchedule()							{ return m_pSchedule; }
	bool				IsCurSchedule( int schedId, bool fIdeal = true );
	virtual CAI_Schedule *GetSchedule(int localScheduleID);
	virtual int			GetLocalScheduleId( int globalScheduleID )	{ return AI_IdIsLocal( globalScheduleID ) ? globalScheduleID : GetClassScheduleIdSpace()->ScheduleGlobalToLocal( globalScheduleID ); }
	virtual int			GetGlobalScheduleId( int localScheduleID )	{ return AI_IdIsGlobal( localScheduleID ) ? localScheduleID : GetClassScheduleIdSpace()->ScheduleLocalToGlobal( localScheduleID ); }

	float				GetTimeScheduleStarted() const				{ return m_ScheduleState.timeStarted; }
	
	//---------------------------------
	
	const Task_t*		GetTask( void );
	int					TaskIsRunning( void );
	
	virtual void		TaskFail( AI_TaskFailureCode_t );
	void				TaskFail( const char *pszGeneralFailText )	{ TaskFail( MakeFailCode( pszGeneralFailText ) ); }
	void				TaskComplete( bool fIgnoreSetFailedCondition = false );

	void				TaskInterrupt()								{ m_ScheduleState.iTaskInterrupt++; }
	void				ClearTaskInterrupt()						{ m_ScheduleState.iTaskInterrupt = 0; }
	int					GetTaskInterrupt() const					{ return m_ScheduleState.iTaskInterrupt; }
	
	void				TaskMovementComplete( void );
	inline int			TaskIsComplete( void ) 						{ return (GetTaskStatus() == TASKSTATUS_COMPLETE); }

	virtual const char *TaskName(int taskID);

	float				GetTimeTaskStarted() const					{ return m_ScheduleState.timeCurTaskStarted; }
	virtual int			GetLocalTaskId( int globalTaskId)			{ return GetClassScheduleIdSpace()->TaskGlobalToLocal( globalTaskId ); }

	virtual const char *GetSchedulingErrorName()					{ return "CAI_BaseNPC"; }

protected:
	static bool			LoadSchedules(void);
	virtual bool		LoadedSchedules(void);
	virtual void		BuildScheduleTestBits( void );

	//---------------------------------

	// This is the main call to select/translate a schedule
	virtual CAI_Schedule *GetNewSchedule( void );
	virtual CAI_Schedule *GetFailSchedule( void );

	//---------------------------------

	virtual bool		CanFlinch( void );
	virtual void		CheckFlinches( void );
	virtual void		PlayFlinchGesture( void );
	int					SelectFlinchSchedule( void );

	virtual	bool		IsAllowedToDodge( void );

	bool				IsInChoreo() const;

private:
	// This function maps the type through TranslateSchedule() and then retrieves the pointer
	// to the actual CAI_Schedule from the database of schedules available to this class.
	CAI_Schedule *		GetScheduleOfType( int scheduleType );
	
	bool				FHaveSchedule( void );
	bool				FScheduleDone ( void );
	CAI_Schedule *		ScheduleInList( const char *pName, CAI_Schedule **pList, int listCount );

	int 				GetScheduleCurTaskIndex() const			{ return m_ScheduleState.iCurTask;		}
	inline int			IncScheduleCurTaskIndex();
	inline void			ResetScheduleCurTaskIndex();
	void				NextScheduledTask ( void );
	bool				IsScheduleValid ( void );
	bool				ShouldSelectIdealState( void );
	
	// Selecting the ideal state
	NPC_STATE			SelectIdleIdealState();
	NPC_STATE			SelectAlertIdealState();
	NPC_STATE			SelectScriptIdealState();

	// Various schedule selections based on NPC_STATE
	int					SelectIdleSchedule();
	int					SelectAlertSchedule();
	int					SelectCombatSchedule();
	virtual int			SelectDeadSchedule();
	int					SelectScriptSchedule();
	int					SelectInteractionSchedule();

	void				OnStartTask( void ) 					{ SetTaskStatus( TASKSTATUS_RUN_MOVE_AND_TASK ); }
	void 				SetTaskStatus( TaskStatus_e status )	{ m_ScheduleState.fTaskStatus = status; 	}
	TaskStatus_e 		GetTaskStatus() const					{ return m_ScheduleState.fTaskStatus; 	}

	void				DiscardScheduleState();

	//---------------------------------

	CAI_Schedule *		m_pSchedule;
	int					m_IdealSchedule;
	AIScheduleState_t	m_ScheduleState;
	int					m_failSchedule;				// Schedule type to choose if current schedule fails
	bool				m_bDoPostRestoreRefindPath;

	bool				m_bUsingStandardThinkTime;
	float				m_flLastRealThinkTime;
	int					m_iFrameBlocked;
	bool				m_bInChoreo;

	static int			gm_iNextThinkRebalanceTick;
	static float		gm_flTimeLastSpawn;
	static int			gm_nSpawnedThisFrame;

protected: // pose parameters
	int					m_poseAim_Pitch;
	int					m_poseAim_Yaw;
	int					m_poseMove_Yaw;
#ifdef MAPBASE
	int					m_poseInteractionRelativeYaw;
#endif
	virtual void		PopulatePoseParameters( void );

public:
	inline bool			HasPoseMoveYaw()		{ return ( m_poseMove_Yaw >= 0 );  }

	// Return the stored pose parameter for "move_yaw"
	inline int			LookupPoseMoveYaw()		{ return m_poseMove_Yaw; }

#ifdef MAPBASE
	inline int			LookupPoseInteractionRelativeYaw()	{ return m_poseInteractionRelativeYaw; }
#endif
 

	//-----------------------------------------------------
	//
	// Hooks for CAI_Behaviors, *if* derived class supports them
	//
	//-----------------------------------------------------
	template <class BEHAVIOR_TYPE>
	bool GetBehavior( BEHAVIOR_TYPE **ppBehavior )
	{
		CAI_BehaviorBase **ppBehaviors = AccessBehaviors();
		
		*ppBehavior = NULL;
		for ( int i = 0; i < NumBehaviors(); i++ )
		{
			*ppBehavior = dynamic_cast<BEHAVIOR_TYPE *>(ppBehaviors[i]);
			if ( *ppBehavior )
				return true;
		}
		return false;
	}

	virtual CAI_BehaviorBase *GetRunningBehavior() { return NULL; }

	virtual bool ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )	{ return true; }
	virtual void OnClearGoal( CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal )		{}

	// Notification that the status behavior ability to select schedules has changed.
	// Return "true" to signal a schedule interrupt is desired
	virtual bool OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule ) { return false; }

private:
	virtual CAI_BehaviorBase **	AccessBehaviors() 	{ return NULL; }
	virtual int					NumBehaviors()		{ return 0; }

public:
	//-----------------------------------------------------
	//
	// Conditions
	//
	//-----------------------------------------------------

	virtual const char*	ConditionName(int conditionID);
	
	virtual void		RemoveIgnoredConditions ( void );
	void				SetCondition( int iCondition /*, bool state = true*/ );
	bool				HasCondition( int iCondition );
	bool				HasCondition( int iCondition, bool bUseIgnoreConditions );
	bool				HasInterruptCondition( int iCondition );
	bool				HasConditionsToInterruptSchedule( int nLocalScheduleID );

	void				ClearCondition( int iCondition );
	void				ClearConditions( int *pConditions, int nConditions );
	void				SetIgnoreConditions( int *pConditions, int nConditions );
	void				ClearIgnoreConditions( int *pConditions, int nConditions );
	bool				ConditionInterruptsCurSchedule( int iCondition );
	bool				ConditionInterruptsSchedule( int schedule, int iCondition );

	void				SetCustomInterruptCondition( int nCondition );
	bool				IsCustomInterruptConditionSet( int nCondition );
	void				ClearCustomInterruptCondition( int nCondition );
	void				ClearCustomInterruptConditions( void );

	bool				ConditionsGathered() const		{ return m_bConditionsGathered; }
	const CAI_ScheduleBits &AccessConditionBits() const { return m_Conditions; }
	CAI_ScheduleBits &	AccessConditionBits()			{ return m_Conditions; }

	bool				DidChooseEnemy() const			{ return !m_bSkippedChooseEnemy; }

#ifdef MAPBASE
	void				InputSetCondition( inputdata_t &inputdata );
	void				InputClearCondition( inputdata_t &inputdata );
#endif

private:
	CAI_ScheduleBits	m_Conditions;
	CAI_ScheduleBits	m_CustomInterruptConditions;	//Bit string assembled by the schedule running, then 
														//modified by leaf classes to suit their needs
	CAI_ScheduleBits	m_ConditionsPreIgnore;
	CAI_ScheduleBits	m_InverseIgnoreConditions;

	bool				m_bForceConditionsGather;
	bool				m_bConditionsGathered;
	bool				m_bSkippedChooseEnemy;

public:
	//-----------------------------------------------------
	//
	// NPC State
	//
	//-----------------------------------------------------
	inline void			SetIdealState( NPC_STATE eIdealState );	
	inline NPC_STATE	GetIdealState();
	virtual NPC_STATE	SelectIdealState( void );

	void				SetState( NPC_STATE State );
	virtual bool		ShouldGoToIdleState( void ) 							{ return ( false ); }
	virtual	void 		OnStateChange( NPC_STATE OldState, NPC_STATE NewState ) {/*Base class doesn't care*/};
	
	NPC_STATE			GetState( void )										{ return m_NPCState; }

	AI_Efficiency_t		GetEfficiency() const						{ return m_Efficiency; }
	void				SetEfficiency( AI_Efficiency_t efficiency )	{ m_Efficiency = efficiency; }
	AI_MoveEfficiency_t GetMoveEfficiency() const					{ return m_MoveEfficiency; }
	void				SetMoveEfficiency( AI_MoveEfficiency_t efficiency )	{ m_MoveEfficiency = efficiency; }
	virtual void		UpdateEfficiency( bool bInPVS );
	void				ForceDecisionThink()						{ m_flNextDecisionTime = 0; SetEfficiency( AIE_NORMAL ); }

	bool				IsFlaggedEfficient() const					{ return HasSpawnFlags( SF_NPC_START_EFFICIENT ); }

	AI_SleepState_t		GetSleepState() const						{ return m_SleepState; }
	void				SetSleepState( AI_SleepState_t sleepState )	{ m_SleepState = sleepState; }
	void				AddSleepFlags( int flags ) { m_SleepFlags |= flags; }
	void				RemoveSleepFlags( int flags ) { m_SleepFlags &= ~flags; }
	bool				HasSleepFlags( int flags ) { return (m_SleepFlags & flags) == flags; }

	virtual void		UpdateSleepState( bool bInPVS );
	virtual	void		Wake( bool bFireOutput = true );
#ifdef MAPBASE
	// A version of Wake() that takes an activator
	virtual	void		Wake( CBaseEntity *pActivator );
#endif
	void				Sleep();
	bool				WokeThisTick() const;

	//---------------------------------

	NPC_STATE			m_NPCState;				// npc's current state
	float				m_flLastStateChangeTime;

private:
	NPC_STATE			m_IdealNPCState;		// npc should change to this state
	AI_Efficiency_t		m_Efficiency;
	AI_MoveEfficiency_t m_MoveEfficiency;
	float				m_flNextDecisionTime;

	AI_SleepState_t		m_SleepState;
	int					m_SleepFlags;
	float				m_flWakeRadius;
	bool				m_bWakeSquad;
	int					m_nWakeTick;

public:
	//-----------------------------------------------------
	//
	//	Activities
	// 
	//-----------------------------------------------------
	
	Activity			TranslateActivity( Activity idealActivity, Activity *pIdealWeaponActivity = NULL );
	Activity			NPC_TranslateActivity( Activity eNewActivity );
#ifdef MAPBASE
	Activity			TranslateCrouchActivity( Activity baseAct );
	virtual bool		CanTranslateCrouchActivity( void ) { return true; }
	virtual Activity	NPC_BackupActivity( Activity eNewActivity );
#endif
	Activity			GetActivity( void ) { return m_Activity; }
	virtual void		SetActivity( Activity NewActivity );
	Activity			GetIdealActivity( void ) { return m_IdealActivity; }
	void				SetIdealActivity( Activity NewActivity );
#ifdef MAPBASE
	Activity			GetTranslatedActivity( void ) { return m_translatedActivity; }
	Activity			GetIdealTranslatedActivity( void ) { return m_IdealTranslatedActivity; }
#endif
	void				ResetIdealActivity( Activity newIdealActivity );
	void				SetSequenceByName( const char *szSequence );
	void				SetSequenceById( int iSequence );
	Activity			GetScriptCustomMoveActivity( void );
	int					GetScriptCustomMoveSequence( void );
	Activity			GetStoppedActivity( void );
	inline bool			HaveSequenceForActivity( Activity activity );
	inline bool			IsActivityStarted(void);
	virtual bool		IsActivityFinished( void );
	virtual bool		IsActivityMovementPhased( Activity activity );
	virtual void		OnChangeActivity( Activity eNewActivity );
	void				MaintainActivity(void);
	void				ResetActivity(void) { m_Activity = ACT_RESET; }

	void				SetActivityAndSequence(Activity NewActivity, int iSequence, Activity translatedActivity, Activity weaponActivity);

#ifdef MAPBASE
	//-----------------------------------------------------

	// Returns the gesture variant of an activity (i.e. "ACT_GESTURE_RANGE_ATTACK1")
	static Activity			GetGestureVersionOfActivity( Activity inActivity );

	// Returns the sequence variant of a gesture activity
	static Activity			GetSequenceVersionOfGesture( Activity inActivity );

	//-----------------------------------------------------

	virtual bool				ShouldPlayFakeSequenceGesture( Activity nActivity, Activity nTranslatedActivity );
	virtual Activity			SelectFakeSequenceGesture( Activity nActivity, Activity nTranslatedActivity );
	void				PlayFakeSequenceGesture( Activity nActivity, Activity nSequence, Activity nTranslatedSequence );

	int					GetFakeSequenceGesture();
	void				ResetFakeSequenceGesture();
#endif

private:

	void				AdvanceToIdealActivity(void);
	void				ResolveActivityToSequence(Activity NewActivity, int &iSequence, Activity &translatedActivity, Activity &weaponActivity);

	Activity			m_Activity;				// Current animation state
	Activity			m_translatedActivity;	// Current actual translated animation

	Activity			m_IdealActivity;				// Desired animation state
	int					m_nIdealSequence;				// Desired animation sequence
	Activity			m_IdealTranslatedActivity;		// Desired actual translated animation state
	Activity			m_IdealWeaponActivity;			// Desired weapon animation state

#ifdef MAPBASE
	int					m_FakeSequenceGestureLayer;		// The gesture layer impersonating a sequence (-1 if invalid)
#endif

	CNetworkVar(int, m_iDeathPose );
	CNetworkVar(int, m_iDeathFrame );

public:
	//-----------------------------------------------------
	//
	// Senses
	//
	//-----------------------------------------------------

	CAI_Senses *		GetSenses()			{ return m_pSenses; }
	const CAI_Senses *	GetSenses() const	{ return m_pSenses; }
	
	void				SetDistLook( float flDistLook );
#ifdef MAPBASE
	void				InputSetDistLook( inputdata_t &inputdata );
	void				InputSetDistTooFar( inputdata_t &inputdata );
#endif

	virtual bool		QueryHearSound( CSound *pSound );
	virtual bool		QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );

	virtual void		OnLooked( int iDistance );
	virtual void		OnListened();

	virtual void		OnSeeEntity( CBaseEntity *pEntity ) {}

	// If true, AI will try to see this entity regardless of distance.
	virtual bool		ShouldNotDistanceCull() { return false; }
	
	virtual int			GetSoundInterests( void );
	virtual int			GetSoundPriority( CSound *pSound );

	CSound *			GetLoudestSoundOfType( int iType );
	virtual CSound *	GetBestSound( int validTypes = ALL_SOUNDS );
	virtual CSound *	GetBestScent( void );
	virtual float		HearingSensitivity( void )		{ return 1.0;	}
	virtual bool		ShouldIgnoreSound( CSound * )	{ return false; }
	bool				SoundIsVisible( CSound *pSound );

protected:
	virtual void		ClearSenseConditions( void );
	
private:
	void				LockBestSound();
	void				UnlockBestSound();

	CAI_Senses *		m_pSenses;
	CSound *			m_pLockedBestSound;

public:
	//-----------------------------------------------------
	//
	// Enemy and target
	//
	//-----------------------------------------------------

	Vector GetSmoothedVelocity( void );

	CBaseEntity*		GetEnemy()							{ return m_hEnemy.Get(); }
	CBaseEntity*		GetEnemy() const					{ return m_hEnemy.Get(); }
	float				GetTimeEnemyAcquired()				{ return m_flTimeEnemyAcquired; }
	void				SetEnemy( CBaseEntity *pEnemy, bool bSetCondNewEnemy = true );

	const Vector &		GetEnemyLKP() const;
	float				GetEnemyLastTimeSeen() const;
	void				MarkEnemyAsEluded();
	void				ClearEnemyMemory();
	bool				EnemyHasEludedMe() const;
	
	virtual CBaseEntity *BestEnemy();		// returns best enemy in memory list
	virtual	bool		IsValidEnemy( CBaseEntity *pEnemy );
	virtual	bool		CanBeAnEnemyOf( CBaseEntity *pEnemy );

	void				ForceChooseNewEnemy()	{ m_EnemiesSerialNumber = -1; }

	bool				ChooseEnemy();
	virtual bool		ShouldChooseNewEnemy();
	virtual void		GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual float		EnemyDistTolerance() {  return 0; } // Enemy distances within this tolerance of each other are considered equivalent.
	
	float				EnemyDistance( CBaseEntity *pEnemy );
	CBaseCombatCharacter *GetEnemyCombatCharacterPointer();
	void SetEnemyOccluder(CBaseEntity *pBlocker);
	CBaseEntity *GetEnemyOccluder(void);

	virtual void		StartTargetHandling( CBaseEntity *pTargetEnt );
#ifdef MAPBASE
	void				InputSetTarget( inputdata_t &inputdata );
#endif

	//---------------------------------
	
	CBaseEntity*		GetTarget()								{ return m_hTargetEnt.Get(); }
	void				SetTarget( CBaseEntity *pTarget );
	void				CheckTarget( CBaseEntity *pTarget );
	float				GetAcceptableTimeSeenEnemy( void )		{ return m_flAcceptableTimeSeenEnemy; }
	virtual	CAI_BaseNPC *CreateCustomTarget( const Vector &vecOrigin, float duration = -1 );

	void				SetDeathPose( const int &iDeathPose ) { m_iDeathPose = iDeathPose; }
	void				SetDeathPoseFrame( const int &iDeathPoseFrame ) { m_iDeathFrame = iDeathPoseFrame; }
	
	void				SelectDeathPose( const CTakeDamageInfo &info );
	virtual bool		ShouldPickADeathPose( void ) { return true; }

	virtual	bool		AllowedToIgnite( void ) { return false; }

protected:
	virtual float 		GetGoalRepathTolerance( CBaseEntity *pGoalEnt, GoalType_t type, const Vector &curGoal, const Vector &curTargetPos );

private:
	void *				CheckEnemy( CBaseEntity *pEnemy ) { return NULL; } // OBSOLETE, replaced by GatherEnemyConditions(), left here to make derived code not compile

	// Updates the goal position in case of GOALTYPE_ENEMY
	void				UpdateEnemyPos();

	// Updates the goal position in case of GOALTYPE_TARGETENT
	void				UpdateTargetPos();

	//---------------------------------
	
	EHANDLE				m_hEnemy;		// the entity that the npc is fighting.
	float				m_flTimeEnemyAcquired; // The time at which the entity the NPC is fighting became the NPC's enemy.
	EHANDLE				m_hTargetEnt;	// the entity that the npc is trying to reach

	CRandStopwatch		m_GiveUpOnDeadEnemyTimer;
	CSimpleSimTimer		m_FailChooseEnemyTimer;
	int					m_EnemiesSerialNumber;

	float				m_flAcceptableTimeSeenEnemy;

	CSimpleSimTimer		m_UpdateEnemyPosTimer;
	static CSimpleSimTimer m_AnyUpdateEnemyPosTimer;

public:
	//-----------------------------------------------------
	//
	// Commander mode stuff.
	//
	//-----------------------------------------------------
	virtual bool IsCommandable()										{ return false; }
	virtual bool IsPlayerAlly( CBasePlayer *pPlayer = NULL );
	virtual bool IsMedic()												{ return false; }
	virtual bool IsCommandMoving()										{ return false; }
	virtual bool ShouldAutoSummon()										{ return false; }
	virtual void SetCommandGoal( const Vector &vecGoal );
	virtual void ClearCommandGoal();
	virtual void OnTargetOrder()										{}
	virtual void OnMoveOrder()											{}
	virtual bool IsValidCommandTarget( CBaseEntity *pTarget )			{ return false; }
	const Vector &GetCommandGoal() const								{ return m_vecCommandGoal; }
	virtual void OnMoveToCommandGoalFailed()							{}
	string_t GetPlayerSquadName() const									{ Assert( gm_iszPlayerSquad != NULL_STRING ); return gm_iszPlayerSquad; }
	bool IsInPlayerSquad() const;
	virtual CAI_BaseNPC *GetSquadCommandRepresentative()				{ return NULL; }

	virtual bool TargetOrder( CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies ) { OnTargetOrder(); return true; }
	virtual void MoveOrder( const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies ) { SetCommandGoal( vecDest ); SetCondition( COND_RECEIVED_ORDERS ); OnMoveOrder(); }

	// Return true if you're willing to be idly talked to by other friends.
	virtual bool CanBeUsedAsAFriend( void );


private:
	Vector			m_vecCommandGoal;
	static string_t gm_iszPlayerSquad;

public:
	CAI_MoveMonitor	m_CommandMoveMonitor;

#ifdef MAPBASE
	ThreeState_t m_FriendlyFireOverride = TRS_NONE;
	virtual bool	FriendlyFireEnabled();
	void			InputSetFriendlyFire( inputdata_t &inputdata );

	// Grenade-related functions from Combine soldiers ported to ai_basenpc so they could be shared.
	// 
	// This is necessary because other NPCs can use them now and many instances where they were used relied on dynamic_casts.
	virtual Vector		GetAltFireTarget() { return GetEnemy() ? GetEnemy()->BodyTarget(Weapon_ShootPosition()) : vec3_origin; }
	virtual void		DelayGrenadeCheck(float delay) { ; }
	virtual void		AddGrenades( int inc, CBaseEntity *pLastGrenade = NULL ) { ; }
#endif

#ifdef MAPBASE_VSCRIPT
private:

	// VScript stuff uses "VScript" instead of just "Script" to avoid
	// confusion with NPC_STATE_SCRIPT or StartScripting
	HSCRIPT				VScriptGetEnemy();
	void				VScriptSetEnemy( HSCRIPT pEnemy );
	Vector				VScriptGetEnemyLKP();

	HSCRIPT				VScriptFindEnemyMemory( HSCRIPT pEnemy );

	int					VScriptGetState();

	void				VScriptWake( HSCRIPT hActivator ) { Wake( ToEnt(hActivator) ); }
	void				VScriptSleep() { Sleep(); }

	int					VScriptGetSleepState()	{ return (int)GetSleepState(); }
	void				VScriptSetSleepState( int sleepState ) { SetSleepState( (AI_SleepState_t)sleepState ); }

	const char*			VScriptGetHintGroup() { return STRING( GetHintGroup() ); }
	HSCRIPT				VScriptGetHintNode();

	const char*			ScriptGetActivity() { return GetActivityName( GetActivity() ); }
	int					ScriptGetActivityID() { return GetActivity(); }
	void				ScriptSetActivity( const char *szActivity ) { SetActivity( (Activity)GetActivityID( szActivity ) ); }
	void				ScriptSetActivityID( int iActivity ) { SetActivity((Activity)iActivity); }
	int					ScriptTranslateActivity( const char *szActivity ) { return TranslateActivity( (Activity)GetActivityID( szActivity ) ); }
	int					ScriptTranslateActivityID( int iActivity ) { return TranslateActivity( (Activity)iActivity ); }

	const char*			VScriptGetGestureVersionOfActivity( const char *pszActivity ) { return GetActivityName( GetGestureVersionOfActivity( (Activity)GetActivityID( pszActivity ) ) ); }
	int					VScriptGetGestureVersionOfActivityID( int iActivity ) { return GetGestureVersionOfActivity( (Activity)iActivity ); }
	const char*			VScriptGetSequenceVersionOfGesture( const char *pszActivity ) { return GetActivityName( GetSequenceVersionOfGesture( (Activity)GetActivityID( pszActivity ) ) ); }
	int					VScriptGetSequenceVersionOfGestureID( int iActivity ) { return GetSequenceVersionOfGesture( (Activity)iActivity ); }

	const char*			VScriptGetSchedule();
	int					VScriptGetScheduleID();
	void				VScriptSetSchedule( const char *szSchedule );
	void				VScriptSetScheduleID( int iSched ) { SetSchedule( iSched ); }
	const char*			VScriptGetTask();
	int					VScriptGetTaskID();

	bool				VScriptHasCondition( const char *szCondition ) { return HasCondition( GetConditionID( szCondition ) ); }
	bool				VScriptHasConditionID( int iCondition ) { return HasCondition( iCondition ); }
	void				VScriptSetCondition( const char *szCondition ) { SetCondition( GetConditionID( szCondition ) ); }
	void				VScriptClearCondition( const char *szCondition ) { ClearCondition( GetConditionID( szCondition ) ); }

	HSCRIPT				VScriptGetExpresser();

	HSCRIPT				VScriptGetCine();
	int					GetScriptState() { return m_scriptState; }

	HSCRIPT				VScriptGetSquad();
#endif

	//-----------------------------------------------------
	// Dynamic scripted NPC interactions
	//-----------------------------------------------------
public:
	float GetInteractionYaw( void ) const { return m_flInteractionYaw; }

	bool IsRunningDynamicInteraction( void ) { return (m_iInteractionState != NPCINT_NOT_RUNNING && (m_hCine != NULL)); }
	bool IsActiveDynamicInteraction( void ) { return (m_iInteractionState == NPCINT_RUNNING_ACTIVE && (m_hCine != NULL)); }
	CAI_BaseNPC *GetInteractionPartner( void );

protected:
	void ParseScriptedNPCInteractions( void );
	void AddScriptedNPCInteraction( ScriptedNPCInteraction_t *pInteraction  );
	const char *GetScriptedNPCInteractionSequence( ScriptedNPCInteraction_t *pInteraction, int iPhase, bool bOtherNPC = false );
	void StartRunningInteraction( CAI_BaseNPC *pOtherNPC, bool bActive );
	void StartScriptedNPCInteraction( CAI_BaseNPC *pOtherNPC, ScriptedNPCInteraction_t *pInteraction, Vector vecOtherOrigin, QAngle angOtherAngles );
	void CheckForScriptedNPCInteractions( void );
	void CalculateValidEnemyInteractions( void );
	void CheckForcedNPCInteractions( void );
#ifdef MAPBASE
	// This is checked during automatic dynamic interactions, but not during forced interactions.
	// This is so we can control interaction permissions while still letting forced interactions play when needed.
	virtual bool InteractionIsAllowed( CAI_BaseNPC *pOtherNPC, ScriptedNPCInteraction_t *pInteraction );
#endif
	bool InteractionCouldStart( CAI_BaseNPC *pOtherNPC, ScriptedNPCInteraction_t *pInteraction, Vector &vecOrigin, QAngle &angAngles );
	virtual bool CanRunAScriptedNPCInteraction( bool bForced = false );
	ScriptedNPCInteraction_t *GetRunningDynamicInteraction( void ) { return &(m_ScriptedInteractions[m_iInteractionPlaying]); }
	void SetInteractionCantDie( bool bCantDie ) { m_bCannotDieDuringInteraction = bCantDie; }
	bool HasInteractionCantDie( void );
	bool HasValidInteractionsOnCurrentEnemy( void );
	virtual bool CanStartDynamicInteractionDuringMelee() { return false; }

	void InputForceInteractionWithNPC( inputdata_t &inputdata );
	void StartForcedInteraction( CAI_BaseNPC *pNPC, int iInteraction );
	void CleanupForcedInteraction( void );
	void CalculateForcedInteractionPosition( void );

private:
	// Forced interactions
	CHandle<CAI_BaseNPC>				 m_hForcedInteractionPartner;
	Vector								 m_vecForcedWorldPosition;
	float								m_flForcedInteractionTimeout; // Abort the interaction if it hasn't started by this time.

	CHandle<CAI_BaseNPC>				 m_hInteractionPartner;
	EHANDLE								 m_hLastInteractionTestTarget;
	bool								 m_bCannotDieDuringInteraction;
	int									 m_iInteractionState;
	int									 m_iInteractionPlaying;
#ifdef MAPBASE
public:
#endif
	CUtlVector<ScriptedNPCInteraction_t> m_ScriptedInteractions;

	float								 m_flInteractionYaw;

#ifdef MAPBASE
	// Allows mappers to control dynamic interactions.
	// DI added by Mapbase requires this to be on TRS_TRUE (1). Others, like Alyx's interactions, only require TRS_NONE (2).
	// TRS_FALSE (0) disables all dynamic interactions, including existing ones.
	ThreeState_t						m_iDynamicInteractionsAllowed;
#endif

	
public:
	//-----------------------------------------------------
	//
	//  Sounds
	// 
	//-----------------------------------------------------
	virtual CanPlaySequence_t CanPlaySequence( bool fDisregardState, int interruptLevel );

	virtual bool		CanPlaySentence( bool fDisregardState ) { return IsAlive(); }
	virtual int			PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener = NULL );
	virtual int			PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );

	virtual bool		FOkToMakeSound( int soundPriority = 0 );
	virtual void		JustMadeSound( int soundPriority = 0, float flSoundLength = 0.0f );

	virtual void		DeathSound( const CTakeDamageInfo &info )	{ return; };
	virtual void		AlertSound( void )							{ return; };
	virtual void		IdleSound( void )							{ return; };
	virtual void		PainSound( const CTakeDamageInfo &info )	{ return; };
	virtual void		FearSound( void )				 			{ return; };
	virtual void		LostEnemySound( void ) 						{ return; };
	virtual void		FoundEnemySound( void ) 					{ return; };
	virtual void		BarnacleDeathSound( void )					{ CTakeDamageInfo info;	PainSound( info ); }

	virtual void		SpeakSentence( int sentenceType ) 			{ return; };
	virtual bool		ShouldPlayIdleSound( void );

	virtual void		MakeAIFootstepSound( float volume, float duration = 0.5f );

	//---------------------------------

	virtual CAI_Expresser *GetExpresser() { return NULL; }
	const CAI_Expresser *GetExpresser() const { return const_cast<CAI_BaseNPC *>(this)->GetExpresser(); }

	//---------------------------------
	// NPC Event Response System
	virtual bool		CanRespondToEvent( const char *ResponseConcept ) { return false; }
	virtual bool 		RespondedTo( const char *ResponseConcept, bool bForce, bool bCancelScene ) { return false; }

	virtual void		PlayerHasIlluminatedNPC( CBasePlayer *pPlayer, float flDot );

	virtual void		ModifyOrAppendCriteria( AI_CriteriaSet& set );
#ifdef MAPBASE
	virtual void		ModifyOrAppendEnemyCriteria( AI_CriteriaSet& set, CBaseEntity *pEnemy );
	virtual void		ModifyOrAppendDamageCriteria( AI_CriteriaSet& set, const CTakeDamageInfo &info );
#endif

protected:
	float		SoundWaitTime() const { return m_flSoundWaitTime; }

public:
	//-----------------------------------------------------
	//
	// Capabilities report (from CBaseCombatCharacter)
	//
	//-----------------------------------------------------
	virtual int			CapabilitiesGet( void ) const;

	// local capabilities access
	int					CapabilitiesAdd( int capabilities );
	int					CapabilitiesRemove( int capabilities );
	void				CapabilitiesClear( void );

#ifdef MAPBASE
	void				InputAddCapabilities( inputdata_t &inputdata );
	void				InputRemoveCapabilities( inputdata_t &inputdata );
#endif

private:
	int					m_afCapability;			// tells us what a npc can/can't do.

public:
	//-----------------------------------------------------
	//
	// Pathfinding, navigation & movement
	//
	//-----------------------------------------------------
	
	CAI_Navigator *		GetNavigator() 				{ return m_pNavigator; }
	const CAI_Navigator *GetNavigator() const 		{ return m_pNavigator; }

	CAI_LocalNavigator *GetLocalNavigator()			{ return m_pLocalNavigator; }
	const CAI_LocalNavigator *GetLocalNavigator() const { return m_pLocalNavigator; }

	CAI_Pathfinder *	GetPathfinder() 			{ return m_pPathfinder; }
	const CAI_Pathfinder *GetPathfinder() const 	{ return m_pPathfinder; }

	CAI_MoveProbe *		GetMoveProbe() 				{ return m_pMoveProbe; }
	const CAI_MoveProbe *GetMoveProbe() const		{ return m_pMoveProbe; }

	CAI_Motor *			GetMotor() 					{ return m_pMotor; }
	const CAI_Motor *	GetMotor() const			{ return m_pMotor; }
	
	//---------------------------------

	static bool			FindSpotForNPCInRadius( Vector *pResult, const Vector &vStartPos, CAI_BaseNPC *pNPC, float radius, bool bOutOfPlayerViewcone = false );

	//---------------------------------

	virtual bool		IsNavigationUrgent();
	virtual bool		ShouldFailNav( bool bMovementFailed );
	virtual bool		ShouldBruteForceFailedNav()	{ return false; }
	
	// The current navigation (movement) mode (e.g. fly, swim, locomote, etc) 
	Navigation_t		GetNavType() const;
	void				SetNavType( Navigation_t navType );
	
	CBaseEntity *		GetNavTargetEntity(void);

	bool				IsMoving( void );
	virtual float 		GetTimeToNavGoal();
	
	// NPCs can override this to tweak with how costly particular movements are
	virtual	bool		MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

	// Turns a directional vector into a yaw value that points down that vector.
	float				VecToYaw( const Vector &vecDir );

	//  Turning
	virtual	float		CalcIdealYaw( const Vector &vecTarget );
	virtual float		MaxYawSpeed( void );		// Get max yaw speed
	bool				FacingIdeal( void );
	void				SetUpdatedYaw()	{ m_ScheduleState.bTaskUpdatedYaw = true; }

	//   Add multiple facing goals while moving/standing still.
	virtual void		AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual float		GetFacingDirection( Vector &vecDir );

	// ------------
	// Methods used by motor to query properties/preferences/move-related state
	// ------------
	virtual bool		CanStandOn( CBaseEntity *pSurface ) const;

	virtual bool		IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const; // Override for specific creature types
	bool				IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, float maxUp, float maxDown, float maxDist ) const;
	bool 				ShouldMoveWait();
	virtual float		StepHeight() const			{ return 18.0f; }
	float				GetStepDownMultiplier() const;
	virtual float		GetMaxJumpSpeed() const		{ return 350.0f; }
	virtual float		GetJumpGravity() const		{ return 1.0f; }
	
	//---------------------------------
	
	virtual bool		OverrideMove( float flInterval );				// Override to take total control of movement (return true if done so)
	virtual	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	//---------------------------------
	
	virtual bool		IsUnusableNode(int iNodeID, CAI_Hint *pHint); // Override for special NPC behavior
	virtual bool		ValidateNavGoal();
	virtual bool		IsCurTaskContinuousMove();
	virtual bool		IsValidMoveAwayDest( const Vector &vecDest )	{ return true; }

	//---------------------------------
	//
	// Notifications from navigator
	//
	virtual void		OnMovementFailed() {};
	virtual void		OnMovementComplete() {};

	//---------------------------------

	bool				FindNearestValidGoalPos( const Vector &vTestPoint, Vector *pResult );

	void				RememberUnreachable( CBaseEntity* pEntity, float duration = -1 );	// Remember that entity is unreachable
	virtual bool		IsUnreachable( CBaseEntity* pEntity );			// Is entity is unreachable?

	//---------------------------------
	// Inherited from IAI_MotorMovementServices
	virtual float		CalcYawSpeed( void );

	virtual bool		OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, 
										float distClear, 
										AIMoveResult_t *pResult );

	virtual bool		OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, 
												float distClear, 
												AIMoveResult_t *pResult );

	// Translations of the above into some useful game terms
	virtual bool		OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, 
										 CBaseDoor *pDoor,
										 float distClear, 
										 AIMoveResult_t *pResult );

	virtual bool 		OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal,
 											CBasePropDoor *pDoor,
											float distClear,
											AIMoveResult_t *pResult );

	void	OpenPropDoorBegin( CBasePropDoor *pDoor );
	void	OpenPropDoorNow( CBasePropDoor *pDoor );

	//---------------------------------
	
	void				DelayMoveStart( float delay )	{ m_flMoveWaitFinished = gpGlobals->curtime + delay; }
	
	float				m_flMoveWaitFinished;
	

	//
	// Stuff for opening doors.
	//	
	void OnDoorFullyOpen(CBasePropDoor *pDoor);
	void OnDoorBlocked(CBasePropDoor *pDoor);
	CHandle<CBasePropDoor> m_hOpeningDoor;	// The CBasePropDoor that we are in the midst of opening for navigation.

protected:
	// BRJ 4/11
	// Semi-obsolete-looking Lars code I moved out of the engine and into here
	int FlyMove( const Vector& vecPosition, unsigned int mask );
	int WalkMove( const Vector& vecPosition, unsigned int mask );

	//  Unreachable Entities
	CUtlVector<UnreachableEnt_t> m_UnreachableEnts;								// Array of unreachable entities

private:
	CAI_Navigator *		m_pNavigator;
	CAI_LocalNavigator *m_pLocalNavigator;
	CAI_Pathfinder *	m_pPathfinder;
	CAI_MoveProbe *		m_pMoveProbe;
	CAI_Motor *			m_pMotor;

	EHANDLE				m_hGoalEnt;					// path corner we are heading towards

	float				m_flTimeLastMovement;


	CSimpleSimTimer		m_CheckOnGroundTimer;

public:
	//-----------------------------------------------------
	//
	// Eye position, view offset, head direction, eye direction
	//
	//-----------------------------------------------------
	
	void				SetDefaultEyeOffset ( void );
	const Vector &		GetDefaultEyeOffset( void )			{ return m_vDefaultEyeOffset;	}
	virtual Vector		GetNodeViewOffset()					{ return GetViewOffset();		}

	virtual Vector		EyeOffset( Activity nActivity );
	virtual Vector		EyePosition( void );

	//---------------------------------
	
	virtual Vector		HeadDirection2D( void );
	virtual Vector		HeadDirection3D( void );
	virtual Vector		EyeDirection2D( void );
	virtual Vector		EyeDirection3D( void );

	virtual CBaseEntity *EyeLookTarget( void );		// Overridden by subclass to force look at an entity
	virtual void		AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 ) { };
	virtual void		AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 ) { };
	virtual void		SetHeadDirection( const Vector &vTargetPos, float flInterval );
	virtual void		MaintainLookTargets( float flInterval );
	virtual bool		ValidEyeTarget(const Vector &lookTargetPos);

	virtual	Vector		FacingPosition( void ) { return EyePosition(); }; // position that other npc's use when facing you

	virtual	void		MaintainTurnActivity( void );

	virtual bool		FInAimCone( const Vector &vecSpot );
	virtual void		AimGun();
	virtual void		SetAim( const Vector &aimDir );
	virtual	void		RelaxAim( void );
	virtual CBaseEntity *GetAlternateMoveShootTarget() { return NULL; }

protected:
	Vector				m_vDefaultEyeOffset;
	float				m_flNextEyeLookTime;	// Next time a pick a new place to look
	
	float				m_flEyeIntegRate;		 // How fast does eye move to target

private:
	Vector				m_vEyeLookTarget;		 // Where I want to be looking
	Vector				m_vCurEyeTarget;		 // Direction I'm looking at
	EHANDLE				m_hEyeLookTarget;		 // What I want to be looking at
	float				m_flHeadYaw;			 // Current head yaw
	float				m_flHeadPitch;			 // Current head pitch
protected:
	float				m_flOriginalYaw;		 // This is the direction facing when the level designer placed the NPC in the level.

public:
	//-----------------------------------------------------
	// Mapmaker Scripting
	//
	// Set when the NPC is being scripted by a mapmaker, and
	// shouldn't be responding to external stimuli that would
	// break him out of his "script". NOT a scripted sequence.
	//-----------------------------------------------------
	inline bool			IsInAScript( void ) { return m_bInAScript; }
	inline void			SetInAScript( bool bScript ) { m_bInAScript = bScript; }
	void				InputStartScripting( inputdata_t &inputdata ) { m_bInAScript = true; }
	void				InputStopScripting( inputdata_t &inputdata ) { m_bInAScript = false; }

	void				InputGagEnable( inputdata_t &inputdata ) { AddSpawnFlags(SF_NPC_GAG); }
	void				InputGagDisable( inputdata_t &inputdata ) { RemoveSpawnFlags(SF_NPC_GAG); }

	bool				HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	virtual void		InputOutsideTransition( inputdata_t &inputdata );
	virtual void		InputInsideTransition( inputdata_t &inputdata );

	void				CleanupScriptsOnTeleport( bool bEnrouteAsWell );

	virtual	void		SetScriptedScheduleIgnoreConditions( Interruptability_t interrupt );

private:
	bool				m_bInAScript;

public:
	//-----------------------------------------------------
	//
	// Scripting
	//
	//-----------------------------------------------------

	// Scripted sequence Info
	enum SCRIPTSTATE
	{
		SCRIPT_PLAYING = 0,				// Playing the action animation.
		SCRIPT_WAIT,						// Waiting on everyone in the script to be ready. Plays the pre idle animation if there is one.
		SCRIPT_POST_IDLE,					// Playing the post idle animation after playing the action animation.
		SCRIPT_CLEANUP,					// Cancelling the script / cleaning up.
		SCRIPT_WALK_TO_MARK,				// Walking to the scripted sequence position.
		SCRIPT_RUN_TO_MARK,				// Running to the scripted sequence position.
		SCRIPT_CUSTOM_MOVE_TO_MARK,	// Moving to the scripted sequence position while playing a custom movement animation.
	};

	bool				ExitScriptedSequence();
	bool				CineCleanup();

	virtual void		Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );

	// forces movement and sets a new schedule
	virtual bool		ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity );
	virtual bool		ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity );

	static void			ForceSelectedGo(CBaseEntity *pPlayer, const Vector &targetPos, const Vector &traceDir, bool bRun);
	static void			ForceSelectedGoRandom(void);

	bool				AutoMovement( CBaseEntity *pTarget = NULL, AIMoveTrace_t *pTraceResult = NULL );
	bool				AutoMovement( float flInterval, CBaseEntity *pTarget = NULL, AIMoveTrace_t *pTraceResult = NULL );
	bool				TaskRanAutomovement( void ) { return m_ScheduleState.bTaskRanAutomovement; }

	SCRIPTSTATE			m_scriptState;		// internal cinematic state
	CHandle<CAI_ScriptedSequence>	m_hCine;
	Activity			m_ScriptArrivalActivity;
	string_t			m_strScriptArrivalSequence;

	//-----------------------------------------------------
	//
	// Scenes
	//
	//-----------------------------------------------------

	void				AddSceneLock( float flDuration = 0.2f ) { m_flSceneTime = MAX( gpGlobals->curtime + flDuration, m_flSceneTime ); };
	void				ClearSceneLock( float flDuration = 0.2f ) { m_flSceneTime = gpGlobals->curtime + flDuration; };
	bool				IsInLockedScene( void ) { return m_flSceneTime > gpGlobals->curtime; };
	float				m_flSceneTime;
	string_t			m_iszSceneCustomMoveSeq;

public:
	//-----------------------------------------------------
	//
	// Memory
	//
	//-----------------------------------------------------

	inline void			Remember( int iMemory ) 		{ m_afMemory |= iMemory; }
	inline void			Forget( int iMemory ) 			{ m_afMemory &= ~iMemory; }
	inline bool			HasMemory( int iMemory ) 		{ if ( m_afMemory & iMemory ) return TRUE; return FALSE; }
	inline bool			HasAllMemories( int iMemory ) 	{ if ( (m_afMemory & iMemory) == iMemory ) return TRUE; return FALSE; }

	virtual CAI_Enemies *GetEnemies( void );
	virtual void		RemoveMemory( void );

	virtual bool		UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer = NULL );
	virtual float		GetReactionDelay( CBaseEntity *pEnemy );
	
	void				SetLastAttackTime( float time)	{ m_flLastAttackTime = time; }

	float				GetLastAttackTime() const { return m_flLastAttackTime; }
	float				GetLastDamageTime() const { return m_flLastDamageTime; }
	float				GetLastPlayerDamageTime() const { return m_flLastPlayerDamageTime; }
	float				GetLastEnemyTime() const { return m_flLastEnemyTime; }

	// Set up the shot regulator based on the equipped weapon
	virtual void		OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	// Weapon holstering
	virtual bool		CanHolsterWeapon( void );
	virtual int			HolsterWeapon( void );
	virtual int			UnholsterWeapon( void );
	void				InputHolsterWeapon( inputdata_t &inputdata );
	void				InputHolsterAndDestroyWeapon( inputdata_t &inputdata );
	void				InputUnholsterWeapon( inputdata_t &inputdata );
	bool				IsWeaponHolstered( void );
	bool				IsWeaponStateChanging( void );
	void				SetDesiredWeaponState( DesiredWeaponState_t iState ) { m_iDesiredWeaponState = iState; }

#ifdef MAPBASE
	virtual bool		DoHolster(void);
	virtual bool		DoUnholster(void);

	virtual bool		ShouldUnholsterWeapon();
	virtual bool		CanUnholsterWeapon();

	void				InputGiveWeaponHolstered( inputdata_t &inputdata );
	void				InputChangeWeapon( inputdata_t &inputdata );
	void				InputPickupWeapon( inputdata_t &inputdata );
	void				InputPickupItem( inputdata_t &inputdata );
#endif

	// NOTE: The Shot Regulator is used to manage the RangeAttack1 weapon.
	inline CAI_ShotRegulator* GetShotRegulator()		{ return &m_ShotRegulator; }
#ifdef MAPBASE
	// A special function for ai_weaponmodifier.
	inline void SetShotRegulator(CAI_ShotRegulator NewRegulator) { m_ShotRegulator = NewRegulator; }
#endif
	virtual void		OnRangeAttack1();
	
protected:
	// Shot regulator code
	virtual void		OnUpdateShotRegulator( );

protected:
	CAI_Enemies *		m_pEnemies;	// Holds information about enemies / danger positions / shared between sqaud members
	int					m_afMemory;
	EHANDLE				m_hEnemyOccluder;	// The entity my enemy is hiding behind.

	float				m_flSumDamage;				// How much consecutive damage I've received
	float				m_flLastDamageTime;			// Last time I received damage
	float				m_flLastPlayerDamageTime;	// Last time I received damage from the player
	float				m_flLastSawPlayerTime;		// Last time I saw the player
	float				m_flLastAttackTime;			// Last time that I attacked my current enemy
	float				m_flLastEnemyTime;
	float				m_flNextWeaponSearchTime;	// next time to search for a better weapon
#ifdef MAPBASE
public:
	int					m_iLastHolsteredWeapon;
#endif
	string_t			m_iszPendingWeapon;			// THe NPC should create and equip this weapon.
	bool				m_bIgnoreUnseenEnemies;

private:
	CAI_ShotRegulator	m_ShotRegulator;			// When should I shoot next?

	DesiredWeaponState_t	m_iDesiredWeaponState;

public:
	//-----------------------------------------------------
	//
	// Squads & tactics
	//
	//-----------------------------------------------------

	virtual bool		InitSquad( void );

	virtual const char*	SquadSlotName(int slotID)		{ return gm_SquadSlotNamespace.IdToSymbol(slotID);					}
	bool				OccupyStrategySlot( int squadSlotID );
	bool				OccupyStrategySlotRange( int slotIDStart, int slotIDEnd );
	bool				HasStrategySlot( int squadSlotID );
	bool				HasStrategySlotRange( int slotIDStart, int slotIDEnd );
	int					GetMyStrategySlot() { return m_iMySquadSlot; }
	void				VacateStrategySlot( void );
	bool				IsStrategySlotRangeOccupied( int slotIDStart, int slotIDEnd );	// Returns true if all in the range are occupied
	
	CAI_Squad *			GetSquad()						{ return m_pSquad; 		}
	virtual void		SetSquad( CAI_Squad *pSquad );
	void				AddToSquad( string_t name );
	void				RemoveFromSquad();
	void				CheckSquad();
	void				SetSquadName( string_t name )	{ m_SquadName = name; 	}
	bool				IsInSquad() const				{ return m_pSquad != NULL; }
	virtual bool		IsSilentSquadMember() const 	{ return false; }

	int					NumWeaponsInSquad( const char *pszWeaponClassname );

	string_t			GetHintGroup( void )			{ return m_strHintGroup;		}
	void				ClearHintGroup( void )			{ SetHintGroup( NULL_STRING );	}
	void				SetHintGroup( string_t name, bool bHintGroupNavLimiting = false );
	bool				IsLimitingHintGroups( void )	{ return m_bHintGroupNavLimiting; }

#ifdef MAPBASE
	void				InputSetHintGroup( inputdata_t &inputdata ) { SetHintGroup(inputdata.value.StringID()); }
#endif

	//---------------------------------

	CAI_TacticalServices *GetTacticalServices()			{ return m_pTacticalServices; }
	const CAI_TacticalServices *GetTacticalServices() const { return m_pTacticalServices; }

	//---------------------------------
	//  Cover
	
	virtual bool		FindCoverPos( CBaseEntity *pEntity, Vector *pResult );
	virtual bool		FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult );
	virtual bool		FindCoverPos( CSound *pSound, Vector *pResult );
	virtual bool		IsValidCover ( const Vector &vecCoverLocation, CAI_Hint const *pHint );
	virtual bool		IsValidShootPosition ( const Vector &vecCoverLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	virtual bool		TestShootPosition(const Vector &vecShootPos, const Vector &targetPos )	{ return WeaponLOSCondition( vecShootPos, targetPos, false ); }
	virtual bool		IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition );
	virtual float		CoverRadius( void ) { return 1024; } // Default cover radius
	virtual float		GetMaxTacticalLateralMovement( void ) { return MAXTACLAT_IGNORE; }

protected:
	virtual void		OnChangeHintGroup( string_t oldGroup, string_t newGroup ) {}

	CAI_Squad *			m_pSquad;		// The squad that I'm on
	string_t			m_SquadName;

	int					m_iMySquadSlot;	// this is the behaviour slot that the npc currently holds in the squad. 

private:
	string_t			m_strHintGroup;
	bool				m_bHintGroupNavLimiting;
	CAI_TacticalServices *m_pTacticalServices;

public:
	//-----------------------------------------------------
	//
	// Base schedule & task support; Miscellaneous
	//
	//-----------------------------------------------------

	void				InitRelationshipTable( void );
#ifndef MAPBASE
	void				AddRelationship( const char *pszRelationship, CBaseEntity *pActivator );
#endif

	virtual void		AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority );
	virtual void		AddClassRelationship( Class_T nClass, Disposition_t nDisposition, int nPriority );

	void				NPCUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CBaseGrenade*		IncomingGrenade(void);

	virtual bool		ShouldFadeOnDeath( void );

	void				NPCInitDead( void );	// Call after animation/pose is set up
	void				CorpseFallThink( void );

	float				ThrowLimit( const Vector &vecStart, const Vector &vecEnd, float fGravity, float fArcSize, const Vector &mins, const Vector &maxs, CBaseEntity *pTarget, Vector *jumpVel, CBaseEntity **pBlocker);

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	virtual int			RangeAttack1Conditions( float flDot, float flDist );
	virtual int			RangeAttack2Conditions( float flDot, float flDist );
	virtual int			MeleeAttack1Conditions( float flDot, float flDist );
	virtual int			MeleeAttack2Conditions( float flDot, float flDist );

	virtual float		InnateRange1MinRange( void ) { return 0.0f; }
	virtual float		InnateRange1MaxRange( void ) { return FLT_MAX; }

	virtual bool		OnBeginMoveAndShoot( void )	{ return true; }
	virtual void		OnEndMoveAndShoot( void )	{}

	virtual bool		UseAttackSquadSlots()	{ return false; }

	//---------------------------------
	
#ifndef MAPBASE // Moved to CBaseCombatCharacter
	virtual	CBaseEntity *FindNamedEntity( const char *pszName, IEntityFindFilter *pFilter = NULL );
#endif

	//---------------------------------
	//  States
	//---------------------------------

	virtual void		ClearAttackConditions( void );
	void				GatherAttackConditions( CBaseEntity *pTarget, float flDist );
	virtual bool		ShouldLookForBetterWeapon();
	bool				Weapon_IsBetterAvailable ( void ) ;
	virtual Vector		Weapon_ShootPosition( void );
#ifdef MAPBASE
	virtual	CBaseCombatWeapon*		GiveWeapon( string_t iszWeaponName, bool bDiscardCurrent = true );
	virtual	CBaseCombatWeapon*		GiveWeaponHolstered( string_t iszWeaponName );
#else
	virtual	void		GiveWeapon( string_t iszWeaponName );
#endif
	virtual void		OnGivenWeapon( CBaseCombatWeapon *pNewWeapon ) { }
	bool				IsMovingToPickupWeapon();
	virtual bool		WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	virtual bool		CurrentWeaponLOSCondition(const Vector &targetPos, bool bSetConditions) { return WeaponLOSCondition( GetAbsOrigin(), targetPos, bSetConditions ); }
	virtual bool		IsWaitingToRappel( void ) { return false; }
	virtual void		BeginRappel() {}

	// override to change the chase location of an enemy
	// This is where your origin should go when you are chasing pEnemy when his origin is at chasePosition
	// by default, leave this alone to make your origin coincide with his.
	virtual void		TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition);
	virtual float		GetDefaultNavGoalTolerance() { return (GetHullWidth() * 2.0); }

	virtual bool		FCanCheckAttacks ( void );
	virtual void		CheckAmmo( void ) {}

	virtual bool		FValidateHintType( CAI_Hint *pHint );
	virtual Activity	GetHintActivity( short sHintType, Activity HintsActivity );
	virtual float		GetHintDelay( short sHintType );
	virtual Activity	GetCoverActivity( CAI_Hint* pHint );
	virtual Activity	GetReloadActivity( CAI_Hint* pHint );

	virtual void		SetTurnActivity( void );
	bool				UpdateTurnGesture( void );

	// Returns the time when the door will be open
	float				OpenDoorAndWait( CBaseEntity *pDoor );

	bool				BBoxFlat( void );

	//---------------------------------

	virtual void		Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
#ifdef MAPBASE
	virtual void		EnemyIgnited( CAI_BaseNPC *pVictim ) {}
#endif
	virtual bool		PassesDamageFilter( const CTakeDamageInfo &info );

	//---------------------------------

	void				MakeDamageBloodDecal( int cCount, float flNoise, trace_t *ptr, Vector vecDir );
	virtual float		GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	void				TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	void				DecalTrace( trace_t *pTrace, char const *decalName );
	void				ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	virtual	bool		PlayerInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter, bool ignoreHatedPlayers = true );
	CBaseEntity *		PlayerInRange( const Vector &vecLocation, float flDist );
	bool				PointInSpread( CBaseCombatCharacter *pCheckEntity, const Vector &sourcePos, const Vector &targetPos, const Vector &testPoint, float flSpread, float maxDistOffCenter );
	bool				IsSquadmateInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter );

	//---------------------------------
	// combat functions
	//---------------------------------
	virtual bool		InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	

	virtual Activity	GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	
	virtual bool		ShouldGib( const CTakeDamageInfo &info ) { return false; }	// Always ragdoll, unless specified by the leaf class
	virtual bool		Event_Gibbed( const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual Vector		GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy = true );
#ifdef HL2_DLL
	virtual Vector		GetActualShootPosition( const Vector &shootOrigin );
	virtual Vector		GetActualShootTrajectory( const Vector &shootOrigin );
	virtual	Vector		GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );
	virtual	float		GetSpreadBias( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget );
#endif //HL2_DLL
	virtual void		CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir );
	virtual Vector		BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual Vector		GetAutoAimCenter() { return BodyTarget(vec3_origin, false); }
	virtual void		FireBullets( const FireBulletsInfo_t &info );

	// OLD VERSION! Use the struct version
	void FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, 
		const Vector &vecSpread, float flDistance, int iAmmoType, int iTracerFreq = 4, 
		int firingEntID = -1, int attachmentID = -1, int iDamage = 0, 
		CBaseEntity *pAttacker = NULL, bool bFirstShotAccurate = false );

	virtual	bool		ShouldMoveAndShoot( void );

	//---------------------------------
	//  Damage
	//---------------------------------
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Dead( const CTakeDamageInfo &info );

	virtual void		NotifyFriendsOfDamage( CBaseEntity *pAttackerEntity );
	virtual void		OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );

	virtual bool		IsLightDamage( const CTakeDamageInfo &info );
	virtual bool		IsHeavyDamage( const CTakeDamageInfo &info );

	void				DoRadiusDamage( const CTakeDamageInfo &info, int iClassIgnore, CBaseEntity *pEntityIgnore );
	void				DoRadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, int iClassIgnore, CBaseEntity *pEntityIgnore );

	//---------------------------------

	virtual void	PickupWeapon( CBaseCombatWeapon *pWeapon );
#ifdef MAPBASE
	virtual void	PickupItem( CBaseEntity *pItem );
#else
	virtual void	PickupItem( CBaseEntity *pItem ) { };
#endif
	CBaseEntity*	DropItem( const char *pszItemName, Vector vecPos, QAngle vecAng );// drop an item.


	//---------------------------------
	// Inputs
	//---------------------------------
#ifndef MAPBASE // Moved to CBaseCombatCharacter
	void InputSetRelationship( inputdata_t &inputdata );
#endif
	void InputSetEnemyFilter( inputdata_t &inputdata );
#ifdef MAPBASE
	// This is virtual so npc_helicopter can override it
	virtual void InputSetHealthFraction( inputdata_t &inputdata );
#else
	void InputSetHealth( inputdata_t &inputdata );
#endif
	void InputBeginRappel( inputdata_t &inputdata );
	void InputSetSquad( inputdata_t &inputdata );
	void InputWake( inputdata_t &inputdata );
	void InputForgetEntity( inputdata_t &inputdata );
	void InputIgnoreDangerSounds( inputdata_t &inputdata );
	void InputUpdateEnemyMemory( inputdata_t &inputdata );

	//---------------------------------
	
	virtual void		NotifyDeadFriend( CBaseEntity *pFriend ) { return; }

	//---------------------------------
	// Utility methods
	static Vector CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize);

	//---------------------------------

	float SetWait( float minWait, float maxWait = 0.0 );
	void ClearWait();
	float GetWaitFinishTime()	{ return m_flWaitFinished; }
	bool IsWaitFinished();
	bool IsWaitSet();
	
	CBaseEntity*	GetGoalEnt()							{ return m_hGoalEnt;	}
	void			SetGoalEnt( CBaseEntity *pGoalEnt )		{ m_hGoalEnt.Set( pGoalEnt ); }

	CAI_Hint		*GetHintNode()							{ return m_pHintNode; }
	const CAI_Hint *GetHintNode() const						{ return m_pHintNode; }
	void			SetHintNode( CAI_Hint *pHintNode );
	void			ClearHintNode( float reuseDelay = 0.0 );

	float				m_flWaitFinished;			// if we're told to wait, this is the time that the wait will be over.

	float				m_flNextFlinchTime;			// Time at which we'll flinch fully again (as opposed to just doing gesture flinches)
	float				m_flNextDodgeTime;			// Time at which I can dodge again. Used so that the behavior doesn't happen over and over.

	CAI_MoveAndShootOverlay m_MoveAndShootOverlay;

	Vector				m_vecLastPosition;			// npc sometimes wants to return to where it started after an operation.
	Vector				m_vSavePosition;			// position stored by code that called this schedules
	Vector				m_vInterruptSavePosition;	// position stored by a task that was interrupted

private:
	CHandle<CAI_Hint>	m_pHintNode;				// this is the hint that the npc is moving towards or performing active idle on.

public:
	int					m_cAmmoLoaded;				// how much ammo is in the weapon (used to trigger reload anim sequences)
	float				m_flDistTooFar;				// if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in GatherEnemyConditions
	string_t			m_spawnEquipment;

	bool				m_fNoDamageDecal;

	EHANDLE				m_hStoredPathTarget;		// For TASK_SET_GOAL
	Vector				m_vecStoredPathGoal;		//
	GoalType_t			m_nStoredPathType;			// 
	int					m_fStoredPathFlags;			//

	CHandle<CBaseFilter>	m_hEnemyFilter;
	string_t				m_iszEnemyFilterName;

	bool					m_bDidDeathCleanup;


	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_lifeState );

	//---------------------------------
	//	Outputs
	//---------------------------------
	COutputEvent		m_OnDamaged;
	COutputEvent		m_OnDeath;
	COutputEvent		m_OnHalfHealth;
	COutputEHANDLE		m_OnFoundEnemy; 
	COutputEvent		m_OnLostEnemyLOS; 
	COutputEvent		m_OnLostEnemy; 
	COutputEHANDLE		m_OnFoundPlayer;
	COutputEvent		m_OnLostPlayerLOS;
	COutputEvent		m_OnLostPlayer; 
	COutputEvent		m_OnHearWorld;
	COutputEvent		m_OnHearPlayer;
	COutputEvent		m_OnHearCombat;
	COutputEvent		m_OnDamagedByPlayer;
	COutputEvent		m_OnDamagedByPlayerSquad;
	COutputEvent		m_OnDenyCommanderUse;
	COutputEvent		m_OnRappelTouchdown;
	COutputEvent		m_OnSleep;
	COutputEvent		m_OnWake;
	COutputEvent		m_OnForcedInteractionStarted;
	COutputEvent		m_OnForcedInteractionAborted;
	COutputEvent		m_OnForcedInteractionFinished;

#ifdef MAPBASE
	COutputEHANDLE		m_OnHolsterWeapon;
	COutputEHANDLE		m_OnUnholsterWeapon;

	COutputEHANDLE		m_OnItemPickup;
	COutputEHANDLE		m_OnItemDrop;

	COutputInt			m_OnStateChange;
#endif

public:
	// use this to shrink the bbox temporarily
	void				SetHullSizeNormal( bool force = false );
	bool				SetHullSizeSmall( bool force = false );

	bool				IsUsingSmallHull() const	{ return m_fIsUsingSmallHull; }

	const Vector &		GetHullMins() const		{ return NAI_Hull::Mins(GetHullType()); }
	const Vector &		GetHullMaxs() const		{ return NAI_Hull::Maxs(GetHullType()); }
	float				GetHullWidth()	const	{ return NAI_Hull::Width(GetHullType()); }
	float				GetHullHeight() const	{ return NAI_Hull::Height(GetHullType()); }

	void				SetupVPhysicsHull();
	virtual void		StartTouch( CBaseEntity *pOther );
	void				CheckPhysicsContacts();

private:
	void				TryRestoreHull( void );
	bool				m_fIsUsingSmallHull;
	bool				m_bCheckContacts;

private:
	// Task implementation helpers
	void StartTurn( float flDeltaYaw );
	bool FindCoverFromEnemy( bool bNodesOnly = false, float flMinDistance = 0, float flMaxDistance = FLT_MAX );
	bool FindCoverFromBestSound( Vector *pCoverPos );
	void StartScriptMoveToTargetTask( int task );
	
	void RunDieTask();
	void RunAttackTask( int task );

protected:
	virtual float CalcReasonableFacing( bool bIgnoreOriginalFacing = false );
	virtual bool IsValidReasonableFacing( const Vector &vecSightDir, float sightDist ) { return true; }
	virtual float GetReasonableFacingDist( void );

public:
	inline int UsableNPCObjectCaps( int baseCaps )
	{
		if ( IsAlive() )
			baseCaps |= FCAP_IMPULSE_USE;
		return baseCaps;
	}

	virtual int	ObjectCaps()	{ return (BaseClass::ObjectCaps() | FCAP_NOTIFY_ON_TRANSITION); }

	//-----------------------------------------------------
	//
	// Core mapped data structures 
	//
	// String Registries for default AI Shared by all CBaseNPCs
	//	These are used only during initialization and in debug
	//-----------------------------------------------------

	static void InitSchedulingTables();

	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()		{ return &gm_SchedulingSymbols; }
	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect() { return gm_ClassScheduleIdSpace; }
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()	{ return &gm_ClassScheduleIdSpace; }

	static int			GetScheduleID	(const char* schedName);
	static int			GetActivityID	(const char* actName);
	static int			GetConditionID	(const char* condName);
	static int			GetTaskID		(const char* taskName);
	static int			GetSquadSlotID	(const char* slotName);
	virtual const char* GetSquadSlotDebugName( int iSquadSlot );
	static const char*	GetActivityName	(int actID);	

	static void			AddActivityToSR(const char *actName, int conID);
#ifdef MAPBASE
	static int			GetOrRegisterActivity( const char *actName );
#endif
	
	static void			AddEventToSR(const char *eventName, int conID);
	static const char*	GetEventName	(int actID);
	static int			GetEventID	(const char* actName);

public:
	//-----------------------------------------------------
	// Crouch handling
	//-----------------------------------------------------
	bool	CrouchIsDesired( void ) const;
	virtual bool IsCrouching( void );
	inline void	ForceCrouch( void );
	inline void	ClearForceCrouch( void );

#ifdef MAPBASE
	bool		 CouldShootIfCrouchingAt( const Vector &vecPosition, const Vector &vecForward, const Vector &vecRight, float flDist = 48.0f );
#endif

protected:
	virtual bool Crouch( void );
	virtual bool Stand( void );
	virtual void DesireCrouch( void );
	inline void	 DesireStand( void );
	bool		 CouldShootIfCrouching( CBaseEntity *pTarget );
	virtual bool IsCrouchedActivity( Activity activity );

protected:
	// Override these in your derived NPC class
	virtual Vector GetCrouchEyeOffset( void )		{ return Vector(0,0,40); }
	virtual Vector GetCrouchGunOffset( void )		{ return Vector(0,0,36); }

private:
	bool	m_bCrouchDesired;
	bool	m_bForceCrouch;
	bool	m_bIsCrouching;
	//-----------------------------------------------------

	//-----------------------------------------------------
	// ai_post_frame_navigation
	//-----------------------------------------------------

private:
	bool	m_bDeferredNavigation;	// This NPCs has a navigation query that's being deferred until later in the frame

public:
	void	SetNavigationDeferred( bool bState ) { m_bDeferredNavigation = bState; }
	bool	IsNavigationDeferred( void ) { return m_bDeferredNavigation; }

	//-----------------------------------------------------
protected:
	static CAI_GlobalNamespace gm_SquadSlotNamespace;
	static CAI_LocalIdSpace    gm_SquadSlotIdSpace;

private:
	// Checks to see that the nav hull is valid for the NPC
	bool IsNavHullValid() const;

	friend class CAI_SystemHook;
	friend class CAI_SchedulesManager;
	
	static bool			LoadDefaultSchedules(void);

	static void			InitDefaultScheduleSR(void);
	static void			InitDefaultTaskSR(void);
	static void			InitDefaultConditionSR(void);
	static void			InitDefaultActivitySR(void);
	static void			InitDefaultSquadSlotSR(void);
	
	static CStringRegistry*	m_pActivitySR;
	static int			m_iNumActivities;

	static CStringRegistry*	m_pEventSR;
	static int			m_iNumEvents;
	
	static CAI_GlobalScheduleNamespace	gm_SchedulingSymbols;
	static CAI_ClassScheduleIdSpace		gm_ClassScheduleIdSpace;

#ifdef MAPBASE
	typedef struct
	{
		Activity	sequence;
		Activity	gesture;
	} actlink_t;

	static actlink_t		gm_ActivityGestureLinks[];
#endif

public:
	//----------------------------------------------------
	// Debugging tools
	//
	
	// -----------------------------
	//  Debuging Fields and Methods
	// -----------------------------
	const char*			m_failText;					// Text of why it failed
	const char*			m_interruptText;			// Text of why schedule interrupted
	CAI_Schedule*		m_failedSchedule;			// The schedule that failed last
	CAI_Schedule*		m_interuptSchedule;			// The schedule that was interrupted last
	int					m_nDebugCurIndex;			// Index used for stepping through AI
	virtual void		ReportAIState( void );
	virtual void		ReportOverThinkLimit( float time );
	void 				DumpTaskTimings();
	void				DrawDebugGeometryOverlays(void);
	virtual int			DrawDebugTextOverlays(void);
	void				ToggleFreeze(void);

	static void			ClearAllSchedules(void);

	static int			m_nDebugBits;

	static CAI_BaseNPC*	m_pDebugNPC;
	static int			m_nDebugPauseIndex;		// Current step
	static inline void	SetDebugNPC( CAI_BaseNPC *pNPC )  { m_pDebugNPC = pNPC; }
	static inline bool	IsDebugNPC( CAI_BaseNPC *pNPC ) { return( pNPC == m_pDebugNPC ); }

	float				m_LastShootAccuracy;
	int 				m_TotalShots;
	int					m_TotalHits;
#ifdef _DEBUG
	bool				m_bSelected;
#endif

	float				m_flSoundWaitTime;	// Time when I'm allowed to make another sound
	int					m_nSoundPriority;
	float				m_flIgnoreDangerSoundsUntil;

#ifdef AI_MONITOR_FOR_OSCILLATION
	CUtlVector<AIScheduleChoice_t>	m_ScheduleHistory;
#endif//AI_MONITOR_FOR_OSCILLATION

#ifdef MAPBASE_VSCRIPT
	static ScriptHook_t	g_Hook_QueryHearSound;
	static ScriptHook_t	g_Hook_QuerySeeEntity;
	static ScriptHook_t	g_Hook_TranslateActivity;
	static ScriptHook_t	g_Hook_TranslateSchedule;
	static ScriptHook_t	g_Hook_GetActualShootPosition;
	static ScriptHook_t	g_Hook_OverrideMove;
	static ScriptHook_t	g_Hook_ShouldPlayFakeSequenceGesture;
#endif

private:

	// Break into pieces!
	void				Break( CBaseEntity *pBreaker );
	void				InputBreak( inputdata_t &inputdata );

	friend void 		CC_NPC_Go();
	friend void 		CC_NPC_GoRandom();
	friend void 		CC_NPC_Freeze( const CCommand &args );

public:

	CNetworkVar( bool,  m_bPerformAvoidance );
	CNetworkVar( bool,	m_bIsMoving );
	CNetworkVar( bool,  m_bFadeCorpse );
	CNetworkVar( bool,  m_bImportanRagdoll );

	CNetworkVar( bool,  m_bSpeedModActive );
	CNetworkVar( int,   m_iSpeedModRadius );
	CNetworkVar( int,   m_iSpeedModSpeed );
	CNetworkVar( float, m_flTimePingEffect );			// Display the pinged effect until this time

	void				InputActivateSpeedModifier( inputdata_t &inputdata ) { m_bSpeedModActive = true; }
	void				InputDisableSpeedModifier( inputdata_t &inputdata ) { m_bSpeedModActive = false; }
	void				InputSetSpeedModifierRadius( inputdata_t &inputdata );
	void				InputSetSpeedModifierSpeed( inputdata_t &inputdata );

#ifdef MAPBASE
	// Hammer input to change the speed of the NPC (based on 1upD's npc_shadow_walker code)
	// Not to be confused with the inputs above
	virtual float		GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );
	inline float		GetSequenceGroundSpeed( int iSequence ) { return GetSequenceGroundSpeed( GetModelPtr(), iSequence ); }
	void				InputSetSpeedModifier( inputdata_t &inputdata );
	float				m_flSpeedModifier;
#endif

	virtual bool		ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity );

	bool				m_bPlayerAvoidState;
	void				GetPlayerAvoidBounds( Vector *pMins, Vector *pMaxs );

	void				StartPingEffect( void ) { m_flTimePingEffect = gpGlobals->curtime + 2.0f; DispatchUpdateTransmitState(); }
};


//-----------------------------------------------------------------------------
// Purpose: Returns whether our ideal activity has started. If not, we are in
//			a transition sequence.
//-----------------------------------------------------------------------------
inline bool CAI_BaseNPC::IsActivityStarted(void)
{
	return (GetSequence() == m_nIdealSequence);
}

//-----------------------------------------------------------------------------
// Bullet firing (legacy)...
//-----------------------------------------------------------------------------
inline void CAI_BaseNPC::FireBullets( int cShots, const Vector &vecSrc, 
	const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, 
	int iAmmoType, int iTracerFreq, int firingEntID, int attachmentID,
	int iDamage, CBaseEntity *pAttacker, bool bFirstShotAccurate )
{
	FireBulletsInfo_t info;
	info.m_iShots = cShots;
	info.m_vecSrc = vecSrc;
	info.m_vecDirShooting = vecDirShooting;
	info.m_vecSpread = vecSpread;
	info.m_flDistance = flDistance;
	info.m_iAmmoType = iAmmoType;
	info.m_iTracerFreq = iTracerFreq;
	info.m_flDamage = iDamage;
	info.m_pAttacker = pAttacker;
	info.m_nFlags = bFirstShotAccurate ? FIRE_BULLETS_FIRST_SHOT_ACCURATE : 0;

	FireBullets( info );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the ideal state of this NPC.
//-----------------------------------------------------------------------------
inline void	CAI_BaseNPC::SetIdealState( NPC_STATE eIdealState )
{
	if (eIdealState != m_IdealNPCState)
	{
		/*switch (eIdealState)
		{
			case NPC_STATE_NONE:
				Msg("%s.SetIdealState: NPC_STATE_NONE\n", GetDebugName());
				break;

			case NPC_STATE_IDLE:
				Msg("%s.SetIdealState: NPC_STATE_IDLE\n", GetDebugName());
				break;

			case NPC_STATE_ALERT:
				Msg("%s.SetIdealState: NPC_STATE_ALERT\n", GetDebugName());
				break;

			case NPC_STATE_COMBAT:
				Msg("%s.SetIdealState: NPC_STATE_COMBAT\n", GetDebugName());
				break;

			case NPC_STATE_SCRIPT:
				Msg("%s.SetIdealState: NPC_STATE_SCRIPT\n", GetDebugName());
				break;

			case NPC_STATE_PLAYDEAD:
				Msg("%s.SetIdealState: NPC_STATE_PLAYDEAD\n", GetDebugName());
				break;
				
			case NPC_STATE_PRONE:
				Msg("%s.SetIdealState: NPC_STATE_PRONE\n", GetDebugName());
				break;

			case NPC_STATE_DEAD:
				Msg("%s.SetIdealState: NPC_STATE_DEAD\n", GetDebugName());
				break;

			default:
				Msg("%s.SetIdealState: <Unknown>\n", GetDebugName());
				break;
		}*/

		m_IdealNPCState = eIdealState;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current ideal state the NPC will try to achieve.
//-----------------------------------------------------------------------------
inline NPC_STATE CAI_BaseNPC::GetIdealState()
{
	return m_IdealNPCState;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CAI_BaseNPC::IncScheduleCurTaskIndex()
{
	m_ScheduleState.iTaskInterrupt = 0;
	m_ScheduleState.bTaskRanAutomovement = false;
	m_ScheduleState.bTaskUpdatedYaw = false;
	return ++m_ScheduleState.iCurTask;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CAI_BaseNPC::ResetScheduleCurTaskIndex()
{
	m_ScheduleState.iCurTask = 0;
	m_ScheduleState.iTaskInterrupt = 0;
	m_ScheduleState.bTaskRanAutomovement = false;
	m_ScheduleState.bTaskUpdatedYaw = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CAI_BaseNPC::CrouchIsDesired( void ) const
{
	return ( (CapabilitiesGet() & bits_CAP_DUCK) && (m_bCrouchDesired | m_bForceCrouch) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CAI_BaseNPC::DesireStand( void )			
{ 
	m_bCrouchDesired = false; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void	CAI_BaseNPC::ForceCrouch( void )
{
	m_bForceCrouch = true;
	Crouch();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void	CAI_BaseNPC::ClearForceCrouch( void )
{
	m_bForceCrouch = false;

	if ( IsCrouching() )
	{
		Stand();
	}
}

inline bool	CAI_BaseNPC::HaveSequenceForActivity( Activity activity )				
{
#if STUDIO_SEQUENCE_ACTIVITY_LOOKUPS_ARE_SLOW
	return ( (GetModelPtr()) ? (SelectWeightedSequence( activity ) != ACTIVITY_NOT_AVAILABLE) : false ); 
#else
	return ( (GetModelPtr()) ? GetModelPtr()->HaveSequenceForActivity(activity) : false ); 
#endif
}

typedef CHandle<CAI_BaseNPC> AIHANDLE;


// ============================================================================
//	Macros for introducing new schedules in sub-classes
//
// Strings registries and schedules use unique ID's for each item, but 
// sub-class enumerations are non-unique, so we translate between the 
// enumerations and unique ID's
// ============================================================================

#define AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( derivedClass ) \
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<const char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds;
		

//-----------------

#define AI_BEGIN_CUSTOM_NPC( className, derivedClass ) \
	IMPLEMENT_CUSTOM_AI(className, derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<const char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds; \
		CAI_NamespaceInfos squadSlotIds;
		
//-----------------

#define EXTERN_SCHEDULE( id ) \
	scheduleIds.PushBack( #id, id ); \
	extern const char * g_psz##id; \
	schedulesToLoad.AddToTail( g_psz##id );

//-----------------

#define DEFINE_SCHEDULE( id, text ) \
	scheduleIds.PushBack( #id, id ); \
	const char * g_psz##id = \
		"\n	Schedule" \
		"\n		" #id \
		text \
		"\n"; \
	schedulesToLoad.AddToTail( g_psz##id );
	
//-----------------

#define DECLARE_CONDITION( id ) \
	conditionIds.PushBack( #id, id );

//-----------------

#define DECLARE_TASK( id ) \
	taskIds.PushBack( #id, id );

//-----------------

#define DECLARE_ACTIVITY( id ) \
	ADD_CUSTOM_ACTIVITY( CNpc, id );

//-----------------

#define DECLARE_SQUADSLOT( id ) \
	squadSlotIds.PushBack( #id, id );

//-----------------

#define DECLARE_INTERACTION( interaction ) \
	ADD_CUSTOM_INTERACTION( interaction );

//-----------------

#define DECLARE_ANIMEVENT( id ) \
	ADD_CUSTOM_ANIMEVENT( CNpc, id );

//-----------------

#define DECLARE_USES_SCHEDULE_PROVIDER( classname )	reqiredOthers.AddToTail( ScheduleLoadHelper(classname) );

//-----------------

// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_SCHEDULE_PROVIDER() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( pszClassName, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			if ( ValidateConditionLimits( conditionIds[i].pszName ) ) \
			{ \
				ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
			} \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}

inline bool ValidateConditionLimits( const char *pszNewCondition )
{
	int nGlobalConditions = CAI_BaseNPC::GetSchedulingSymbols()->NumConditions();
	if ( nGlobalConditions >= MAX_CONDITIONS )
	{ 
		AssertMsg2( 0, "Exceeded max number of conditions (%d), ignoring condition %s\n", MAX_CONDITIONS, pszNewCondition ); 
		DevWarning( "Exceeded max number of conditions (%d), ignoring condition %s\n", MAX_CONDITIONS, pszNewCondition ); 
		return false;
	}
	return true;
}


//-------------------------------------

// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_NPC() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( pszClassName, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		CNpc::gm_SquadSlotIdSpace.Init( &BaseClass::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		squadSlotIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			if ( ValidateConditionLimits( conditionIds[i].pszName ) ) \
			{ \
				ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
			} \
		} \
		\
		for ( i = 0; i < squadSlotIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SQUADSLOT_NAMED( CNpc, squadSlotIds[i].pszName, squadSlotIds[i].localId );  \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}

//-------------------------------------

struct AI_NamespaceAddInfo_t
{
	AI_NamespaceAddInfo_t( const char *pszName, int localId )
	 :	pszName( pszName ),
		localId( localId )
	{
	}
	
	const char *pszName;
	int			localId;
};

class CAI_NamespaceInfos : public CUtlVector<AI_NamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId )
	{
		AddToTail( AI_NamespaceAddInfo_t( pszName, localId ) );
	}

	void Sort()
	{
		CUtlVector<AI_NamespaceAddInfo_t>::Sort( Compare );
	}
	
private:
	static int __cdecl Compare(const AI_NamespaceAddInfo_t *pLeft, const AI_NamespaceAddInfo_t *pRight )
	{
		return pLeft->localId - pRight->localId;
	}
	
};

//-------------------------------------

// Declares the static variables that hold the string registry offset for the new subclass
// as well as the initialization in schedule load functions

struct AI_SchedLoadStatus_t
{
	bool fValid;
	int  signature;
};

// Load schedules pulled out to support stepping through with debugger
inline bool AI_DoLoadSchedules( bool (*pfnBaseLoad)(), void (*pfnInitCustomSchedules)(),
								AI_SchedLoadStatus_t *pLoadStatus )
{
	(*pfnBaseLoad)();
	
	if (pLoadStatus->signature != g_AI_SchedulesManager.GetScheduleLoadSignature())
	{
		(*pfnInitCustomSchedules)();
		pLoadStatus->fValid	   = true;
		pLoadStatus->signature = g_AI_SchedulesManager.GetScheduleLoadSignature();
	}
	return pLoadStatus->fValid;
}

//-------------------------------------

typedef bool (*AIScheduleLoadFunc_t)();

// @Note (toml 02-16-03): The following class exists to allow us to establish an anonymous friendship
// in DEFINE_CUSTOM_SCHEDULE_PROVIDER. The particulars of this implementation is almost entirely
// defined by bugs in MSVC 6.0
class ScheduleLoadHelperImpl
{
public:
	template <typename T> 
	static AIScheduleLoadFunc_t AccessScheduleLoadFunc(T *)
	{
		return (&T::LoadSchedules);
	}
};

#define ScheduleLoadHelper( type ) (ScheduleLoadHelperImpl::AccessScheduleLoadFunc((type *)0))

//-------------------------------------

#define DEFINE_CUSTOM_SCHEDULE_PROVIDER\
	static AI_SchedLoadStatus_t 		gm_SchedLoadStatus; \
	static CAI_ClassScheduleIdSpace 	gm_ClassScheduleIdSpace; \
	static const char *					gm_pszErrorClassName;\
	\
	static CAI_ClassScheduleIdSpace &	AccessClassScheduleIdSpaceDirect() 	{ return gm_ClassScheduleIdSpace; } \
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()			{ return &gm_ClassScheduleIdSpace; } \
	virtual const char *				GetSchedulingErrorName()			{ return gm_pszErrorClassName; } \
	\
	static void							InitCustomSchedules(void);\
	\
	static bool							LoadSchedules(void);\
	virtual bool						LoadedSchedules(void); \
	\
	friend class ScheduleLoadHelperImpl;	\
	\
	class CScheduleLoader \
	{ \
	public: \
		CScheduleLoader(); \
	} m_ScheduleLoader; \
	\
	friend class CScheduleLoader;

//-------------------------------------

#define DEFINE_CUSTOM_AI\
	DEFINE_CUSTOM_SCHEDULE_PROVIDER \
	\
	static CAI_LocalIdSpace gm_SquadSlotIdSpace; \
	\
	const char*				SquadSlotName	(int squadSlotID);

//-------------------------------------

#define IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	AI_SchedLoadStatus_t		derivedClass::gm_SchedLoadStatus = { true, -1 }; \
	CAI_ClassScheduleIdSpace 	derivedClass::gm_ClassScheduleIdSpace; \
	const char *				derivedClass::gm_pszErrorClassName = #derivedClass; \
	\
	derivedClass::CScheduleLoader::CScheduleLoader()\
	{ \
		derivedClass::LoadSchedules(); \
	} \
	\
	/* --------------------------------------------- */ \
	/* Load schedules for this type of NPC           */ \
	/* --------------------------------------------- */ \
	bool derivedClass::LoadSchedules(void)\
	{\
		return AI_DoLoadSchedules( derivedClass::BaseClass::LoadSchedules, \
								   derivedClass::InitCustomSchedules, \
								   &derivedClass::gm_SchedLoadStatus ); \
	}\
	\
	bool derivedClass::LoadedSchedules(void) \
	{ \
		return derivedClass::gm_SchedLoadStatus.fValid;\
	} 


//-------------------------------------

// Initialize offsets and implement methods for loading and getting squad info for the subclass
#define IMPLEMENT_CUSTOM_AI(className, derivedClass)\
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	\
	CAI_LocalIdSpace 	derivedClass::gm_SquadSlotIdSpace; \
	\
	/* -------------------------------------------------- */ \
	/* Given squadSlot enumeration return squadSlot name */ \
	/* -------------------------------------------------- */ \
	const char* derivedClass::SquadSlotName(int slotEN)\
	{\
		return gm_SquadSlotNamespace.IdToSymbol( derivedClass::gm_SquadSlotIdSpace.LocalToGlobal(slotEN) );\
	}


//-------------------------------------

#define ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,schedName,schedEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddSchedule( schedName, schedEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SCHEDULE(derivedClass,schedEN) ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,#schedEN,schedEN)

#define ADD_CUSTOM_TASK_NAMED(derivedClass,taskName,taskEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddTask( taskName, taskEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_TASK(derivedClass,taskEN) ADD_CUSTOM_TASK_NAMED(derivedClass,#taskEN,taskEN)

#define ADD_CUSTOM_CONDITION_NAMED(derivedClass,condName,condEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddCondition( condName, condEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_CONDITION(derivedClass,condEN) ADD_CUSTOM_CONDITION_NAMED(derivedClass,#condEN,condEN)

//-------------------------------------

#define INIT_CUSTOM_AI(derivedClass)\
	derivedClass::AccessClassScheduleIdSpaceDirect().Init( #derivedClass, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
	derivedClass::gm_SquadSlotIdSpace.Init( &CAI_BaseNPC::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace);

#ifdef MAPBASE
#define ADD_CUSTOM_INTERACTION( interaction )	{ CBaseCombatCharacter::AddInteractionWithString( interaction, #interaction ); }
#else
#define	ADD_CUSTOM_INTERACTION( interaction )	{ interaction = CBaseCombatCharacter::GetInteractionID(); }
#endif

#define ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,squadSlotName,squadSlotEN)\
	if ( !derivedClass::gm_SquadSlotIdSpace.AddSymbol( squadSlotName, squadSlotEN, "squadslot", derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SQUADSLOT(derivedClass,squadSlotEN) ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,#squadSlotEN,squadSlotEN)

#define ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,activityName,activityEnum)\
	REGISTER_PRIVATE_ACTIVITY(activityEnum);\
	CAI_BaseNPC::AddActivityToSR(activityName,activityEnum);

#define ADD_CUSTOM_ACTIVITY(derivedClass,activityEnum) ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,#activityEnum,activityEnum)


#define ADD_CUSTOM_ANIMEVENT_NAMED(derivedClass,eventName,eventEnum)\
	REGISTER_PRIVATE_ANIMEVENT(eventEnum);\
	CAI_BaseNPC::AddEventToSR(eventName,eventEnum);

#define ADD_CUSTOM_ANIMEVENT(derivedClass,eventEnum) ADD_CUSTOM_ANIMEVENT_NAMED(derivedClass,#eventEnum,eventEnum)


//=============================================================================
// class CAI_Component
//=============================================================================

inline const Vector &CAI_Component::GetLocalOrigin() const
{
	return GetOuter()->GetLocalOrigin();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetLocalOrigin(const Vector &origin)
{
	GetOuter()->SetLocalOrigin(origin);
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetAbsOrigin() const
{
	return GetOuter()->GetAbsOrigin();
}

//-----------------------------------------------------------------------------

inline const QAngle &CAI_Component::GetAbsAngles() const
{
	return GetOuter()->GetAbsAngles();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetSolid( SolidType_t val )
{
	GetOuter()->SetSolid(val);
}

//-----------------------------------------------------------------------------

inline SolidType_t CAI_Component::GetSolid() const
{
	return GetOuter()->GetSolid();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::WorldAlignMins() const
{
	return GetOuter()->WorldAlignMins();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::WorldAlignMaxs() const
{
	return GetOuter()->WorldAlignMaxs();
}
	
//-----------------------------------------------------------------------------

inline Hull_t CAI_Component::GetHullType() const
{
	return GetOuter()->GetHullType();
}

//-----------------------------------------------------------------------------

inline Vector CAI_Component::WorldSpaceCenter() const
{
	return GetOuter()->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetGravity() const
{
	return GetOuter()->GetGravity();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetGravity( float flGravity )
{
	GetOuter()->SetGravity( flGravity );
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetHullWidth() const
{
	return NAI_Hull::Width(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetHullHeight() const
{
	return NAI_Hull::Height(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetHullMins() const
{
	return NAI_Hull::Mins(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetHullMaxs() const
{
	return NAI_Hull::Maxs(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline int CAI_Component::GetCollisionGroup() const
{
	return GetOuter()->GetCollisionGroup();
}

//-----------------------------------------------------------------------------

inline CBaseEntity *CAI_Component::GetEnemy()
{
	return GetOuter()->GetEnemy();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetEnemyLKP() const
{
	return GetOuter()->GetEnemyLKP();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	GetOuter()->TranslateNavGoal( pEnemy, chasePosition );
}

//-----------------------------------------------------------------------------

inline CBaseEntity *CAI_Component::GetTarget()
{
	return GetOuter()->GetTarget();
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetTarget( CBaseEntity *pTarget )
{
	GetOuter()->SetTarget( pTarget );
}

//-----------------------------------------------------------------------------

inline const Task_t *CAI_Component::GetCurTask()
{
	return GetOuter()->GetTask();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::TaskFail( AI_TaskFailureCode_t code )
{
	GetOuter()->TaskFail( code );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::TaskFail( const char *pszGeneralFailText )
{
	GetOuter()->TaskFail( pszGeneralFailText );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::TaskComplete( bool fIgnoreSetFailedCondition )
{
	GetOuter()->TaskComplete( fIgnoreSetFailedCondition );
}
//-----------------------------------------------------------------------------

inline int CAI_Component::TaskIsRunning()
{
	return GetOuter()->TaskIsRunning();
}
//-----------------------------------------------------------------------------

inline int CAI_Component::TaskIsComplete()
{
	return GetOuter()->TaskIsComplete();
}

//-----------------------------------------------------------------------------

inline Activity CAI_Component::GetActivity()
{
	return GetOuter()->GetActivity();
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetActivity( Activity NewActivity )
{
	GetOuter()->SetActivity( NewActivity );
}
//-----------------------------------------------------------------------------

inline float CAI_Component::GetIdealSpeed() const
{
	return GetOuter()->GetIdealSpeed();
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetIdealAccel() const
{
	return GetOuter()->GetIdealAccel();
}

//-----------------------------------------------------------------------------

inline int CAI_Component::GetSequence()
{
	return GetOuter()->GetSequence();
}

//-----------------------------------------------------------------------------

inline int CAI_Component::GetEntFlags() const
{
	return GetOuter()->GetFlags();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::AddEntFlag( int flags )
{
	GetOuter()->AddFlag( flags );
}

//-----------------------------------------------------------------------------

inline void CAI_Component::RemoveEntFlag( int flagsToRemove )
{
	GetOuter()->RemoveFlag( flagsToRemove );
}

//-----------------------------------------------------------------------------
// Purpose: Change the ground entity for the outer
// Input  : *ground - 
// Output : inline void	
//-----------------------------------------------------------------------------
inline void	 CAI_Component::SetGroundEntity( CBaseEntity *ground )
{
	GetOuter()->SetGroundEntity( ground );
}

//-----------------------------------------------------------------------------

inline void CAI_Component::ToggleEntFlag( int flagToToggle )
{
	GetOuter()->ToggleFlag( flagToToggle );
}

//-----------------------------------------------------------------------------

inline CBaseEntity* CAI_Component::GetGoalEnt()
{
	return GetOuter()->GetGoalEnt();
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetGoalEnt( CBaseEntity *pGoalEnt )
{
	GetOuter()->SetGoalEnt( pGoalEnt );
}

//-----------------------------------------------------------------------------

inline void CAI_Component::Remember( int iMemory )
{
	GetOuter()->Remember( iMemory );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::Forget( int iMemory )
{
	GetOuter()->Forget( iMemory );
}
//-----------------------------------------------------------------------------

inline bool CAI_Component::HasMemory( int iMemory )
{
	return GetOuter()->HasMemory( iMemory );
}

//-----------------------------------------------------------------------------

inline CAI_Enemies *CAI_Component::GetEnemies()
{
	return GetOuter()->GetEnemies();
}

//-----------------------------------------------------------------------------

inline const char *CAI_Component::GetEntClassname()
{
	return GetOuter()->GetClassname();
}

//-----------------------------------------------------------------------------

inline int CAI_Component::CapabilitiesGet()
{
	return GetOuter()->CapabilitiesGet();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetLocalAngles( const QAngle& angles )
{
	GetOuter()->SetLocalAngles( angles );
}

//-----------------------------------------------------------------------------

inline const QAngle &CAI_Component::GetLocalAngles( void ) const
{
	return GetOuter()->GetLocalAngles();
}

//-----------------------------------------------------------------------------

inline edict_t *CAI_Component::GetEdict()
{
	return GetOuter()->NetworkProp()->edict();
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetLastThink( const char *szContext )
{
	return GetOuter()->GetLastThink( szContext );
}

// ============================================================================
abstract_class INPCInteractive
{
public:
	virtual bool	CanInteractWith( CAI_BaseNPC *pUser ) = 0;
	virtual	bool	HasBeenInteractedWith()	= 0;
	virtual void	NotifyInteraction( CAI_BaseNPC *pUser ) = 0;

	// Alyx specific interactions
	virtual void	AlyxStartedInteraction( void ) = 0;
	virtual void	AlyxFinishedInteraction( void ) = 0;
};

// Base Class for any NPC that wants to be interactable by other NPCS (i.e. Alyx Hackable)
// NOTE: YOU MUST DEFINE THE OUTPUTS IN YOUR CLASS'S DATADESC!
//		 THE DO SO, INSERT THE FOLLOWING MACRO INTO YOUR CLASS'S DATADESC.
//		
#ifdef MAPBASE
#define	DEFINE_BASENPCINTERACTABLE_DATADESC() \
	DEFINE_OUTPUT( m_OnAlyxStartedInteraction,				"OnAlyxStartedInteraction" ),	\
	DEFINE_OUTPUT( m_OnAlyxFinishedInteraction,				"OnAlyxFinishedInteraction" ),  \
	DEFINE_OUTPUT( m_OnHacked,								"OnHacked" ),  \
	DEFINE_INPUTFUNC( FIELD_VOID, "InteractivePowerDown", InputPowerdown ), \
	DEFINE_INPUTFUNC( FIELD_VOID, "Hack", InputDoInteraction )
#else
#define	DEFINE_BASENPCINTERACTABLE_DATADESC() \
	DEFINE_OUTPUT( m_OnAlyxStartedInteraction,				"OnAlyxStartedInteraction" ),	\
	DEFINE_OUTPUT( m_OnAlyxFinishedInteraction,				"OnAlyxFinishedInteraction" ),  \
	DEFINE_INPUTFUNC( FIELD_VOID, "InteractivePowerDown", InputPowerdown )
#endif

template <class NPC_CLASS>
class CNPCBaseInteractive : public NPC_CLASS, public INPCInteractive
{
	DECLARE_CLASS( CNPCBaseInteractive, NPC_CLASS );
public:
	virtual bool	CanInteractWith( CAI_BaseNPC *pUser ) { return false; };
	virtual	bool	HasBeenInteractedWith()	{ return false; };
	virtual void	NotifyInteraction( CAI_BaseNPC *pUser ) { return; };

	virtual void	InputPowerdown( inputdata_t &inputdata )
	{

	}

#ifdef MAPBASE
	virtual void	InputDoInteraction( inputdata_t &inputdata )
	{
		NotifyInteraction(inputdata.pActivator ? inputdata.pActivator->MyNPCPointer() : NULL);
	}
#endif

	// Alyx specific interactions
	virtual void	AlyxStartedInteraction( void )
	{
		m_OnAlyxStartedInteraction.FireOutput( this, this );
	}
	virtual void	AlyxFinishedInteraction( void )
	{
		m_OnAlyxFinishedInteraction.FireOutput( this, this );
	}

public:
	// Outputs
	// Alyx specific interactions
	COutputEvent	m_OnAlyxStartedInteraction;
	COutputEvent	m_OnAlyxFinishedInteraction;
#ifdef MAPBASE
	COutputEvent	m_OnHacked;
#endif
};

//
//	Deferred Navigation calls go here
//

extern ConVar ai_post_frame_navigation;

class CPostFrameNavigationHook : public CBaseGameSystemPerFrame
{
public:
	virtual const char *Name( void ) { return "CPostFrameNavigationHook"; }

	virtual bool Init( void );
	virtual void FrameUpdatePostEntityThink( void );
	virtual void FrameUpdatePreEntityThink( void );

	bool IsGameFrameRunning( void ) { return m_bGameFrameRunning; }
	void SetGrameFrameRunning( bool bState ) { m_bGameFrameRunning = bState; }
	
	void EnqueueEntityNavigationQuery( CAI_BaseNPC *pNPC, CFunctor *functor );

private:
	CUtlVector<CFunctor *>	m_Functors;
	bool					m_bGameFrameRunning;
};

extern CPostFrameNavigationHook *PostFrameNavigationSystem( void );

#endif // AI_BASENPC_H
