//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions and data pertaining to the NPCs' AI scheduling system.
//			Implements default NPC tasks and schedules.
//
//=============================================================================//


#include "cbase.h"
#include "ai_default.h"
#include "animation.h"
#include "scripted.h"
#include "soundent.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "bitstring.h"
#include "ai_task.h"
#include "ai_network.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_motor.h"
#include "ai_hint.h"
#include "ai_memory.h"
#include "ai_navigator.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_speech.h"
#include "game.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "ndebugoverlay.h"
#include "tier0/vcrmode.h"
#include "env_debughistory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ai_task_pre_script;
extern ConVar ai_use_efficiency;
extern ConVar ai_use_think_optimizations;
#define ShouldUseEfficiency() ( ai_use_think_optimizations.GetBool() && ai_use_efficiency.GetBool() )

ConVar	ai_simulate_task_overtime( "ai_simulate_task_overtime", "0" );

#define MAX_TASKS_RUN 10

struct TaskTimings
{
	const char *pszTask;
	CFastTimer selectSchedule;
	CFastTimer startTimer;
	CFastTimer runTimer;
};

TaskTimings g_AITaskTimings[MAX_TASKS_RUN];
int			g_nAITasksRun;

void CAI_BaseNPC::DumpTaskTimings()
{
	DevMsg(" Tasks timings:\n" );
	for ( int i = 0; i < g_nAITasksRun; ++i )
	{
		DevMsg( "   %32s -- select %5.2f, start %5.2f, run %5.2f\n", g_AITaskTimings[i].pszTask,
			 g_AITaskTimings[i].selectSchedule.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].startTimer.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].runTimer.GetDuration().GetMillisecondsF() );
			
	}
}


//=========================================================
// FHaveSchedule - Returns true if NPC's GetCurSchedule()
// is anything other than NULL.
//=========================================================
bool CAI_BaseNPC::FHaveSchedule( void )
{
	if ( GetCurSchedule() == NULL )
	{
		return false;
	}

	return true;
}

//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CAI_BaseNPC::ClearSchedule( const char *szReason )
{
	if (szReason && m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg( this, AIMF_IGNORE_SELECTED, "  Schedule cleared: %s\n", szReason );
	}

	if ( szReason )
	{
		ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs( "%s(%d):  Schedule cleared: %s\n", GetDebugName(), entindex(), szReason ) );
	}

	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = 0;
	m_ScheduleState.bScheduleWasInterrupted = true;
	SetTaskStatus( TASKSTATUS_NEW );
	m_IdealSchedule = SCHED_NONE;
	m_pSchedule =  NULL;
	ResetScheduleCurTaskIndex();
	m_InverseIgnoreConditions.SetAll();
}

//=========================================================
// FScheduleDone - Returns true if the caller is on the
// last task in the schedule
//=========================================================
bool CAI_BaseNPC::FScheduleDone ( void )
{
	Assert( GetCurSchedule() != NULL );
	
	if ( GetScheduleCurTaskIndex() == GetCurSchedule()->NumTasks() )
	{
		return true;
	}

	return false;
}

//=========================================================

bool CAI_BaseNPC::SetSchedule( int localScheduleID ) 			
{ 
	CAI_Schedule *pNewSchedule = GetScheduleOfType( localScheduleID );
	if ( pNewSchedule )
	{
		// ken: I'm don't know of any remaining cases, but if you find one, hunt it down as to why the schedule is getting slammed while they're in the middle of script
		if (m_hCine != NULL)
		{
			if (!(localScheduleID == SCHED_SLEEP || localScheduleID == SCHED_WAIT_FOR_SCRIPT || localScheduleID == SCHED_SCRIPTED_WALK || localScheduleID == SCHED_SCRIPTED_RUN || localScheduleID == SCHED_SCRIPTED_CUSTOM_MOVE || localScheduleID == SCHED_SCRIPTED_WAIT || localScheduleID == SCHED_SCRIPTED_FACE) )
			{
				Assert( 0 );
				// ExitScriptedSequence();
			}
		}
		

		m_IdealSchedule = GetGlobalScheduleId( localScheduleID );
		SetSchedule( pNewSchedule ); 
		return true;
	}
	return false;
}

//=========================================================
// SetSchedule - replaces the NPC's schedule pointer
// with the passed pointer, and sets the ScheduleIndex back
// to 0
//=========================================================
#define SCHEDULE_HISTORY_SIZE	10
void CAI_BaseNPC::SetSchedule( CAI_Schedule *pNewSchedule )
{
	Assert( pNewSchedule != NULL );
	
	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = gpGlobals->curtime;
	m_ScheduleState.bScheduleWasInterrupted = false;
	
	m_pSchedule = pNewSchedule ;
	ResetScheduleCurTaskIndex();
	SetTaskStatus( TASKSTATUS_NEW );
	m_failSchedule = SCHED_NONE;
	bool bCondInPVS = HasCondition( COND_IN_PVS );
	m_Conditions.ClearAll();
	if ( bCondInPVS )
		SetCondition( COND_IN_PVS );
	m_bConditionsGathered = false;
	GetNavigator()->ClearGoal();
	m_InverseIgnoreConditions.SetAll();
	Forget( bits_MEMORY_TURNING );

/*
#if _DEBUG
	if ( !ScheduleFromName( pNewSchedule->GetName() ) )
	{
		DevMsg( "Schedule %s not in table!!!\n", pNewSchedule->GetName() );
	}
#endif
*/	
// this is very useful code if you can isolate a test case in a level with a single NPC. It will notify
// you of every schedule selection the NPC makes.

	if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg(this, AIMF_IGNORE_SELECTED, "Schedule: %s (time: %.2f)\n", pNewSchedule->GetName(), gpGlobals->curtime );
	}

	ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d): Schedule: %s (time: %.2f)\n", GetDebugName(), entindex(), pNewSchedule->GetName(), gpGlobals->curtime ) );

#ifdef AI_MONITOR_FOR_OSCILLATION
	if( m_bSelected )
	{
		AIScheduleChoice_t choice;
		choice.m_flTimeSelected = gpGlobals->curtime;
		choice.m_pScheduleSelected = pNewSchedule;
		m_ScheduleHistory.AddToHead(choice);

		if( m_ScheduleHistory.Count() > SCHEDULE_HISTORY_SIZE )
		{
			m_ScheduleHistory.Remove( SCHEDULE_HISTORY_SIZE );
		}

		assert( m_ScheduleHistory.Count() <= SCHEDULE_HISTORY_SIZE );

		// No analysis until the vector is full!
		if( m_ScheduleHistory.Count() == SCHEDULE_HISTORY_SIZE )
		{
			int		iNumSelections  = m_ScheduleHistory.Count();
			float	flTimeSpan		= m_ScheduleHistory.Head().m_flTimeSelected - m_ScheduleHistory.Tail().m_flTimeSelected;
			float	flSelectionsPerSecond = ((float)iNumSelections) / flTimeSpan;

			Msg( "%d selections in %f seconds   (avg. %f selections per second)\n", iNumSelections, flTimeSpan, flSelectionsPerSecond );

			if( flSelectionsPerSecond >=  8.0f )
			{
				DevMsg("\n\n %s is thrashing schedule selection:\n", GetDebugName() );

				for( int i = 0 ; i < m_ScheduleHistory.Count() ; i++ )
				{
					AIScheduleChoice_t choice = m_ScheduleHistory[i];
					Msg("--%s  %f\n", choice.m_pScheduleSelected->GetName(), choice.m_flTimeSelected );
				}

				Msg("\n");

				CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
			}
		}
	}
#endif//AI_MONITOR_FOR_OSCILLATION
}

//=========================================================
// NextScheduledTask - increments the ScheduleIndex
//=========================================================
void CAI_BaseNPC::NextScheduledTask ( void )
{
	Assert( GetCurSchedule() != NULL );

	SetTaskStatus( TASKSTATUS_NEW );
	IncScheduleCurTaskIndex();

	if ( FScheduleDone() )
	{
		// Reset memory of failed schedule 
		m_failedSchedule   = NULL;
		m_interuptSchedule = NULL;

		// just completed last task in schedule, so make it invalid by clearing it.
		SetCondition( COND_SCHEDULE_DONE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This function allows NPCs to modify the interrupt mask for the
//			current schedule. This enables them to use base schedules but with
//			different interrupt conditions. Implement this function in your
//			derived class, and Set or Clear condition bits as you please.
//
//			NOTE: Always call the base class in your implementation, but be
//				  aware of the difference between changing the bits before vs.
//				  changing the bits after calling the base implementation.
//
// Input  : pBitString - Receives the updated interrupt mask.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::BuildScheduleTestBits( void )
{
	//NOTENOTE: Always defined in the leaf classes
}


//=========================================================
// IsScheduleValid - returns true as long as the current
// schedule is still the proper schedule to be executing,
// taking into account all conditions
//=========================================================
bool CAI_BaseNPC::IsScheduleValid()
{
	if ( GetCurSchedule() == NULL || GetCurSchedule()->NumTasks() == 0 )
	{
		return false;
	}

	//Start out with the base schedule's set interrupt conditions
	GetCurSchedule()->GetInterruptMask( &m_CustomInterruptConditions );

	// Let the leaf class modify our interrupt test bits, but:
	// - Don't allow any modifications when scripted
	// - Don't modify interrupts for Schedules that set the COND_NO_CUSTOM_INTERRUPTS bit.
	if ( m_NPCState != NPC_STATE_SCRIPT && !IsInLockedScene() && !m_CustomInterruptConditions.IsBitSet( COND_NO_CUSTOM_INTERRUPTS ) )
	{
		BuildScheduleTestBits();
	}

	//Any conditions set here will always be forced on the interrupt conditions
	SetCustomInterruptCondition( COND_NPC_FREEZE );

	// This is like: m_CustomInterruptConditions &= m_Conditions;
	CAI_ScheduleBits testBits;
	m_CustomInterruptConditions.And( m_Conditions, &testBits  );

	if (!testBits.IsAllClear()) 
	{
		// If in developer mode save the interrupt text for debug output
		if (g_pDeveloper->GetInt()) 
		{
			// Reset memory of failed schedule 
			m_failedSchedule   = NULL;
			m_interuptSchedule = GetCurSchedule();

			// Find the first non-zero bit
			for (int i=0;i<MAX_CONDITIONS;i++)
			{
				if (testBits.IsBitSet(i))
				{
					m_interruptText = ConditionName( AI_RemapToGlobal( i ) );
					if (!m_interruptText)
					{
						m_interruptText = "(UNKNOWN CONDITION)";
						/*
						static const char *pError = "ERROR: Unknown condition!";
						DevMsg("%s (%s)\n", pError, GetDebugName());
						m_interruptText = pError;
						*/
					}

					if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
					{
						DevMsg( this, AIMF_IGNORE_SELECTED, "      Break condition -> %s\n", m_interruptText );
					}

					ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      Break condition -> %s\n", GetDebugName(), entindex(), m_interruptText ) );

					break;
				}
			}
			
			if ( HasCondition( COND_NEW_ENEMY ) )
			{
				if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
				{
					DevMsg( this, AIMF_IGNORE_SELECTED, "      New enemy: %s\n", GetEnemy() ? GetEnemy()->GetDebugName() : "<NULL>" );
				}
				
				ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      New enemy: %s\n", GetDebugName(), entindex(), GetEnemy() ? GetEnemy()->GetDebugName() : "<NULL>" ) );
			}
		}

		return false;
	}

	if ( HasCondition(COND_SCHEDULE_DONE) || 
		 HasCondition(COND_TASK_FAILED)   )
	{
#ifdef DEBUG
		if ( HasCondition ( COND_TASK_FAILED ) && m_failSchedule == SCHED_NONE )
		{
			// fail! Send a visual indicator.
			DevWarning( 2, "Schedule: %s Failed\n", GetCurSchedule()->GetName() );

			Vector tmp;
			CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &tmp );
			tmp.z += 16;

			g_pEffects->Sparks( tmp );
		}
#endif // DEBUG

		// some condition has interrupted the schedule, or the schedule is done
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not SelectIdealState() should be called before
//			a NPC selects a new schedule. 
//
//			NOTE: This logic was a source of pure, distilled trouble in Half-Life.
//			If you change this function, please supply good comments.
//
// Output : Returns true if yes, false if no
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ShouldSelectIdealState( void )
{
/*

	HERE's the old Half-Life code that used to control this.

	if ( m_IdealNPCState != NPC_STATE_DEAD && 
		 (m_IdealNPCState != NPC_STATE_SCRIPT || m_IdealNPCState == m_NPCState) )
	{
		if (	(m_afConditions && !HasConditions(bits_COND_SCHEDULE_DONE)) ||
				(GetCurSchedule() && (GetCurSchedule()->iInterruptMask & bits_COND_SCHEDULE_DONE)) ||
				((m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL))	)
		{
			GetIdealState();
		}
	}
*/
	
	// Don't get ideal state if you are supposed to be dead.
	if ( m_IdealNPCState == NPC_STATE_DEAD )
		return false;

	// If I'm supposed to be in scripted state, but i'm not yet, do not allow 
	// SelectIdealState() to be called, because it doesn't know how to determine 
	// that a NPC should be in SCRIPT state and will stomp it with some other 
	// state. (Most likely ALERT)
	if ( (m_IdealNPCState == NPC_STATE_SCRIPT) && (m_NPCState != NPC_STATE_SCRIPT) )
		return false;

	// If the NPC has any current conditions, and one of those conditions indicates
	// that the previous schedule completed successfully, then don't run SelectIdealState(). 
	// Paths between states only exist for interrupted schedules, or when a schedule 
	// contains a task that suggests that the NPC change state.
	if ( !HasCondition(COND_SCHEDULE_DONE) )
		return true;

	// This seems like some sort of hack...
	// Currently no schedule that I can see in the AI uses this feature, but if a schedule
	// interrupt mask contains bits_COND_SCHEDULE_DONE, then force a call to SelectIdealState().
	// If we want to keep this feature, I suggest we create a new condition with a name that
	// indicates exactly what it does. 
	if ( GetCurSchedule() && GetCurSchedule()->HasInterrupt(COND_SCHEDULE_DONE) )
		return true;

	// Don't call SelectIdealState if a NPC in combat state has a valid enemy handle. Otherwise,
	// we need to change state immediately because something unexpected happened to the enemy 
	// entity (it was blown apart by someone else, for example), and we need the NPC to change
	// state. THE REST OF OUR CODE should be robust enough that this can go away!!
	if ( (m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL) )
		return true;

	if ( (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT) && (GetEnemy() != NULL) )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a new schedule based on current condition bits.
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetNewSchedule( void )
{
	int scheduleType;

	//
	// Schedule selection code here overrides all leaf schedule selection.
	//
	if (HasCondition(COND_NPC_FREEZE))
	{
		scheduleType = SCHED_NPC_FREEZE;
	}
	else
	{
		// I dunno how this trend got started, but we need to find the problem.
		// You may not be in combat state with no enemy!!! (sjb) 11/4/03
		if( m_NPCState == NPC_STATE_COMBAT && !GetEnemy() )
		{
			DevMsg("**ERROR: Combat State with no enemy! slamming to ALERT\n");
			SetState( NPC_STATE_ALERT );
		}

		AI_PROFILE_SCOPE_BEGIN( CAI_BaseNPC_SelectSchedule);

		if ( m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_DEAD || m_iInteractionState == NPCINT_MOVING_TO_MARK )
		{
			scheduleType = CAI_BaseNPC::SelectSchedule();
		}
		else
		{
			scheduleType = SelectSchedule();
		}

		m_IdealSchedule = GetGlobalScheduleId( scheduleType );

		AI_PROFILE_SCOPE_END();
	}

	return GetScheduleOfType( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetFailSchedule( void )
{
	int prevSchedule;
	int failedTask;

	if ( GetCurSchedule() )
		prevSchedule = GetLocalScheduleId( GetCurSchedule()->GetId() );
	else
		prevSchedule = SCHED_NONE;
		
	const Task_t *pTask = GetTask();
	if ( pTask )
		failedTask = pTask->iTask;
	else
		failedTask = TASK_INVALID;

	Assert( AI_IdIsLocal( prevSchedule ) );
	Assert( AI_IdIsLocal( failedTask ) );

	int scheduleType = SelectFailSchedule( prevSchedule, failedTask, m_ScheduleState.taskFailureCode );
	return GetScheduleOfType( scheduleType );
}


//=========================================================
// MaintainSchedule - does all the per-think schedule maintenance.
// ensures that the NPC leaves this function with a valid
// schedule!
//=========================================================

static bool ShouldStopProcessingTasks( CAI_BaseNPC *pNPC, int taskTime, int timeLimit )
{
#ifdef DEBUG
	if( ai_simulate_task_overtime.GetBool() )
		return true;
#endif

	// Always stop processing if we've queued up a navigation query on the last task
	if ( pNPC->IsNavigationDeferred() )
		return true;

	if ( AIStrongOpt() )
	{
		bool bInScript = ( pNPC->GetState() == NPC_STATE_SCRIPT || pNPC->IsCurSchedule( SCHED_SCENE_GENERIC, false ) );
		
		// We ran a costly task, don't do it again!
		if ( pNPC->HasMemory( bits_MEMORY_TASK_EXPENSIVE ) && bInScript == false )
			return true;
	}

	if ( taskTime > timeLimit )
	{
		if ( ShouldUseEfficiency() || 
			 pNPC->IsMoving() || 
			 ( pNPC->GetIdealActivity() != ACT_RUN && pNPC->GetIdealActivity() != ACT_WALK ) )
		{
			return true;
		}
	}
	return false;
}

//-------------------------------------

void CAI_BaseNPC::MaintainSchedule ( void )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_RunAI_MaintainSchedule);
	extern CFastTimer g_AIMaintainScheduleTimer;
	CTimeScope timeScope(&g_AIMaintainScheduleTimer);

	//---------------------------------

	CAI_Schedule	*pNewSchedule;
	int			i;
	bool		runTask = true;

#if defined( VPROF_ENABLED )
#if defined(DISABLE_DEBUG_HISTORY)
	bool bDebugTaskNames = ( developer.GetBool() || ( VProfAI() && g_VProfCurrentProfile.IsEnabled() ) );
#else
	bool bDebugTaskNames = true;
#endif
#else
	bool bDebugTaskNames = false;
#endif

	memset( g_AITaskTimings, 0, sizeof(g_AITaskTimings) );
	
	g_nAITasksRun = 0;
	
	const int timeLimit = ( IsDebug() ) ? 16 : 8;
	int taskTime = Plat_MSTime();

	// Reset this at the beginning of the frame
	Forget( bits_MEMORY_TASK_EXPENSIVE );

	// UNDONE: Tune/fix this MAX_TASKS_RUN... This is just here so infinite loops are impossible
	bool bStopProcessing = false;
	for ( i = 0; i < MAX_TASKS_RUN && !bStopProcessing; i++ )
	{
		if ( GetCurSchedule() != NULL && TaskIsComplete() )
		{
			// Schedule is valid, so advance to the next task if the current is complete.
			NextScheduledTask();

			// If we finished the current schedule, clear our ignored conditions so they
			// aren't applied to the next schedule selection.
			if ( HasCondition( COND_SCHEDULE_DONE ) )
			{
				// Put our conditions back the way they were after GatherConditions,
				// but add in COND_SCHEDULE_DONE.
				m_Conditions = m_ConditionsPreIgnore;
				SetCondition( COND_SCHEDULE_DONE );

				m_InverseIgnoreConditions.SetAll();
			}

			// --------------------------------------------------------
			//	If debug stepping advance when I complete a task
			// --------------------------------------------------------
			if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
			{
				m_nDebugCurIndex++;
				return;
			}
		}
		
		int curTiming = g_nAITasksRun;
		g_nAITasksRun++;

		// validate existing schedule 
		if ( !IsScheduleValid() || m_NPCState != m_IdealNPCState )
		{
			// Notify the NPC that his schedule is changing
			m_ScheduleState.bScheduleWasInterrupted = true;
			OnScheduleChange();

			if ( !HasCondition(COND_NPC_FREEZE) && ( !m_bConditionsGathered || m_bSkippedChooseEnemy ) )
			{
				// occurs if a schedule is exhausted within one think
				GatherConditions();
			}

			if ( ShouldSelectIdealState() )
			{
				NPC_STATE eIdealState = SelectIdealState();
				SetIdealState( eIdealState );
			}

			if ( HasCondition( COND_TASK_FAILED ) && m_NPCState == m_IdealNPCState )
			{
				// Get a fail schedule if the previous schedule failed during execution and 
				// the NPC is still in its ideal state. Otherwise, the NPC would immediately
				// select the same schedule again and fail again.
				if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
				{
					DevMsg( this, AIMF_IGNORE_SELECTED, "      (failed)\n" );
				}

				ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      (failed)\n", GetDebugName(), entindex() ) );

				pNewSchedule = GetFailSchedule();
				m_IdealSchedule = pNewSchedule->GetId();
				DevWarning( 2, "(%s) Schedule (%s) Failed at %d!\n", STRING( GetEntityName() ), GetCurSchedule() ? GetCurSchedule()->GetName() : "GetCurSchedule() == NULL", GetScheduleCurTaskIndex() );
				SetSchedule( pNewSchedule );
			}
			else
			{
				// If the NPC is supposed to change state, it doesn't matter if the previous
				// schedule failed or completed. Changing state means selecting an entirely new schedule.
				SetState( m_IdealNPCState );
				
				g_AITaskTimings[curTiming].selectSchedule.Start();

				pNewSchedule = GetNewSchedule();

				g_AITaskTimings[curTiming].selectSchedule.End();

				SetSchedule( pNewSchedule );
			}
		}

		if (!GetCurSchedule())
		{
			g_AITaskTimings[curTiming].selectSchedule.Start();
			
			pNewSchedule = GetNewSchedule();
			
			g_AITaskTimings[curTiming].selectSchedule.End();

			if (pNewSchedule)
			{
				SetSchedule( pNewSchedule );
			}
		}

		if ( !GetCurSchedule() || GetCurSchedule()->NumTasks() == 0 )
		{
			DevMsg("ERROR: Missing or invalid schedule!\n");
			SetActivity ( ACT_IDLE );
			return;
		}
		
		AI_PROFILE_SCOPE_BEGIN_( CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol( GetCurSchedule()->GetId() ) );

		if ( GetTaskStatus() == TASKSTATUS_NEW )
		{	
			if ( GetScheduleCurTaskIndex() == 0 )
			{
				int globalId = GetCurSchedule()->GetId();
				int localId = GetLocalScheduleId( globalId ); // if localId == -1, then it came from a behavior
				OnStartSchedule( (localId != -1)? localId : globalId );
			}

			g_AITaskTimings[curTiming].startTimer.Start();
			const Task_t *pTask = GetTask();
			const char *pszTaskName = ( bDebugTaskNames ) ? TaskName( pTask->iTask ) : "ai_task";
			Assert( pTask != NULL );
			g_AITaskTimings[i].pszTask = pszTaskName;

			if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
			{
				DevMsg(this, AIMF_IGNORE_SELECTED, "  Task: %s\n", pszTaskName );
			}

			ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):  Task: %s\n", GetDebugName(), entindex(), pszTaskName ) );

			OnStartTask();
			
			m_ScheduleState.taskFailureCode    = NO_TASK_FAILURE;
			m_ScheduleState.timeCurTaskStarted = gpGlobals->curtime;
			
			AI_PROFILE_SCOPE_BEGIN_( pszTaskName );
			AI_PROFILE_SCOPE_BEGIN(CAI_BaseNPC_StartTask);

			StartTask( pTask );

			AI_PROFILE_SCOPE_END();
			AI_PROFILE_SCOPE_END();

			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
				StartTaskOverlay();

			g_AITaskTimings[curTiming].startTimer.End();
			// DevMsg( "%.2f StartTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
		}

		AI_PROFILE_SCOPE_END();

		// UNDONE: Twice?!!!
		MaintainActivity();
		
		AI_PROFILE_SCOPE_BEGIN_( CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol( GetCurSchedule()->GetId() ) );

		if ( !TaskIsComplete() && GetTaskStatus() != TASKSTATUS_NEW )
		{
			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) && runTask )
			{
				const Task_t *pTask = GetTask();
				const char *pszTaskName = ( bDebugTaskNames ) ? TaskName( pTask->iTask ) : "ai_task";
				Assert( pTask != NULL );
				g_AITaskTimings[i].pszTask = pszTaskName;
				// DevMsg( "%.2f RunTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
				g_AITaskTimings[curTiming].runTimer.Start();

				AI_PROFILE_SCOPE_BEGIN_( pszTaskName );
				AI_PROFILE_SCOPE_BEGIN(CAI_BaseNPC_RunTask);

				int j;
				for (j = 0; j < 8; j++)
				{
					RunTask( pTask );

					if ( GetTaskInterrupt() == 0 || TaskIsComplete() || HasCondition(COND_TASK_FAILED) )
						break;

					if ( ShouldUseEfficiency() && ShouldStopProcessingTasks( this, Plat_MSTime() - taskTime, timeLimit ) )
					{
						bStopProcessing = true;
						break;
					}
				}
				AssertMsg( j < 8, "Runaway task interrupt\n" );
					
				AI_PROFILE_SCOPE_END();
				AI_PROFILE_SCOPE_END();

				if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
				{
					if ( IsCurTaskContinuousMove() )
						Remember( bits_MEMORY_MOVED_FROM_SPAWN );
					RunTaskOverlay();
				}

				g_AITaskTimings[curTiming].runTimer.End();

				// don't do this again this frame
				// FIXME: RunTask() should eat some of the clock, depending on what it has done
				// runTask = false;

				if ( !TaskIsComplete() )
				{
					bStopProcessing = true;
				}
			}
			else
			{
				bStopProcessing = true;
			}
		}

		AI_PROFILE_SCOPE_END();

		// Decide if we should continue on this frame
		if ( !bStopProcessing && ShouldStopProcessingTasks( this, Plat_MSTime() - taskTime, timeLimit ) )
			bStopProcessing = true;
	}

	// UNDONE: We have to do this so that we have an animation set to blend to if RunTask changes the animation
	// RunTask() will always change animations at the end of a script!
	// Don't do this twice
	MaintainActivity();

	// --------------------------------------------------------
	//	If I'm stopping to debug step, don't animate unless
	//  I'm in motion
	// --------------------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
	{
		if (!GetNavigator()->IsGoalActive() && 
			m_nDebugCurIndex >= CAI_BaseNPC::m_nDebugPauseIndex)
		{
			m_flPlaybackRate = 0;
		}
	}
}


//=========================================================

bool CAI_BaseNPC::FindCoverPos( CBaseEntity *pEntity, Vector *pResult )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_FindCoverPos);

	if ( !GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), 0, pResult ) )
	{
		if ( !GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), pResult ) ) 
		{
			return false;
		}
	}
	return true;
}

//=========================================================

bool CAI_BaseNPC::FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_FindCoverPosInRadius);

	if ( pEntity == NULL )
	{
		// Find cover from self if no enemy available
		pEntity = this;
	}

	Vector					coverPos			= vec3_invalid;
	CAI_TacticalServices *	pTacticalServices	= GetTacticalServices();
	const Vector &			enemyPos			= pEntity->GetAbsOrigin();
	Vector					enemyEyePos			= pEntity->EyePosition();

	if( ( !GetSquad() || GetSquad()->GetFirstMember() == this ) &&
		IsCoverPosition( enemyEyePos, goalPos + GetViewOffset() ) && 
		IsValidCover( goalPos, NULL ) )
	{
		coverPos = goalPos;
	}
	else if ( !pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 0, coverRadius * 0.5, &coverPos ) )
	{
		if ( !pTacticalServices->FindLateralCover( goalPos, enemyEyePos, 0, coverRadius * 0.5, 3, &coverPos ) )
		{
			if ( !pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, coverRadius * 0.5 - 0.1, coverRadius, &coverPos ) )
			{
				pTacticalServices->FindLateralCover( goalPos, enemyEyePos, 0, coverRadius, 5, &coverPos );
			}
		}
	}
	
	if ( coverPos == vec3_invalid )
		return false;
	*pResult = coverPos;
	return true;
}

//=========================================================

bool CAI_BaseNPC::FindCoverPos( CSound *pSound, Vector *pResult )
{
	if ( !GetTacticalServices()->FindCoverPos( pSound->GetSoundReactOrigin(), 
												pSound->GetSoundReactOrigin(), 
												MIN( pSound->Volume(), 120.0 ), 
												CoverRadius(), 
												pResult ) )
	{
		return GetTacticalServices()->FindLateralCover( pSound->GetSoundReactOrigin(), MIN( pSound->Volume(), 60.0 ), pResult );
	}

	return true;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================

//-----------------------------------------------------------------------------
// TASK_TURN_RIGHT / TASK_TURN_LEFT
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartTurn( float flDeltaYaw )
{
	float flCurrentYaw;
	
	flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );
	GetMotor()->SetIdealYaw( UTIL_AngleMod( flCurrentYaw + flDeltaYaw ) );
	SetTurnActivity();
}


//-----------------------------------------------------------------------------
// TASK_CLEAR_HINTNODE
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ClearHintNode( float reuseDelay )
{
	if ( m_pHintNode )
	{
		if ( m_pHintNode->IsLockedBy(this) )
			m_pHintNode->Unlock(reuseDelay);
		m_pHintNode = NULL;
	}
}


void CAI_BaseNPC::SetHintNode( CAI_Hint *pHintNode )
{
	m_pHintNode = pHintNode;
}


//-----------------------------------------------------------------------------

bool CAI_BaseNPC::FindCoverFromEnemy( bool bNodesOnly, float flMinDistance, float flMaxDistance )
{
	CBaseEntity *pEntity = GetEnemy();

	// Find cover from self if no enemy available
	if ( pEntity == NULL )
		pEntity = this;

	Vector coverPos = vec3_invalid;

	ClearHintNode();
	
	if ( bNodesOnly )
	{
		if ( flMaxDistance == FLT_MAX )
			flMaxDistance = CoverRadius();
		
		if ( !GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), flMinDistance, flMaxDistance, &coverPos ) )
			return false;
	}
	else
	{
		if ( !FindCoverPos( pEntity, &coverPos ) )
			return false;
	}

	AI_NavGoal_t goal( GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE );

	if ( !GetNavigator()->SetGoal( goal ) )
		return false;
		
	// FIXME: add to goal
	if (GetHintNode())
	{
		GetNavigator()->SetArrivalActivity( GetCoverActivity( GetHintNode() ) );
		GetNavigator()->SetArrivalDirection( GetHintNode()->GetDirection() );
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// TASK_FIND_COVER_FROM_BEST_SOUND
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::FindCoverFromBestSound( Vector *pCoverPos )
{
	CSound *pBestSound;

	pBestSound = GetBestSound();

	if (pBestSound)
	{
		// UNDONE: Back away if cover fails?  Grunts do this.
		return FindCoverPos( pBestSound, pCoverPos );
	}
	else
	{
		DevMsg( 2, "Attempting to find cover from best sound, but best sound not founc.\n" );
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// TASK_FACE_REASONABLE
//-----------------------------------------------------------------------------
float CAI_BaseNPC::CalcReasonableFacing( bool bIgnoreOriginalFacing )
{
	float flReasonableYaw;

	if( !bIgnoreOriginalFacing && !HasMemory( bits_MEMORY_MOVED_FROM_SPAWN ) && !HasCondition( COND_SEE_ENEMY) )
	{
		flReasonableYaw = m_flOriginalYaw;
	}
	else
	{
		// If I'm facing a wall, change my original yaw and try to find a good direction to face.
		trace_t tr;
		Vector forward;
		QAngle angles( 0, 0, 0 );

		float idealYaw = GetMotor()->GetIdealYaw();
		
		flReasonableYaw = idealYaw;
		
		// Try just using the facing we have
		const float MIN_DIST = GetReasonableFacingDist();
		float longestTrace = 0;
		
		// Early out if we're overriding reasonable facing
		if ( !MIN_DIST )
			return flReasonableYaw;

		// Otherwise, scan out back and forth until something better is found
		const float SLICES = 8.0f;
		const float SIZE_SLICE = 360.0 / SLICES;
		const int SEARCH_MAX = (int)SLICES / 2;

		float zEye = GetAbsOrigin().z + m_vDefaultEyeOffset.z; // always use standing eye so as to not screw with crouch cover

		for( int i = 0 ; i <= SEARCH_MAX; i++ )
		{
			float offset = i * SIZE_SLICE;
			for ( int j = -1; j <= 1; j += 2)
			{
				angles.y = idealYaw + ( offset * j );
				AngleVectors( angles, &forward, NULL, NULL );
				float curTrace;
				if( ( curTrace = LineOfSightDist( forward, zEye ) ) > longestTrace && IsValidReasonableFacing(forward, curTrace) )
				{
					// Take this one.
					flReasonableYaw = angles.y;
					longestTrace = curTrace;
				}
				
				if ( longestTrace > MIN_DIST) // found one
					break;

				if ( i == 0 || i == SEARCH_MAX) // if trying forwards or backwards, skip the check of the other side...
					break;
			}
			
			if ( longestTrace > MIN_DIST ) // found one
				break;
		}
	}
	
	return flReasonableYaw;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CAI_BaseNPC::GetReasonableFacingDist( void )
{
	if ( GetTask() && GetTask()->iTask == TASK_FACE_ENEMY )
	{
		const float dist = 3.5*12;
		if ( GetEnemy() )
		{
			float distEnemy = ( GetEnemy()->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).Length() - 1.0; 
			return MIN( distEnemy, dist );
		}

		return dist;
	}
	return 5*12;
}

//-----------------------------------------------------------------------------
// TASK_SCRIPT_RUN_TO_TARGET / TASK_SCRIPT_WALK_TO_TARGET / TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartScriptMoveToTargetTask( int task )
{
	Activity newActivity;

	if ( m_hTargetEnt == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if ( (m_hTargetEnt->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
	{
		TaskComplete();
	}
	else
	{
		//
		// Select the appropriate activity.
		//
		if ( task == TASK_SCRIPT_WALK_TO_TARGET )
		{
			newActivity = ACT_WALK;
		}
		else if ( task == TASK_SCRIPT_RUN_TO_TARGET )
		{
			newActivity = ACT_RUN;
		}
		else
		{
			newActivity = GetScriptCustomMoveActivity();
		}

		if ( ( newActivity != ACT_SCRIPT_CUSTOM_MOVE ) && TranslateActivity( newActivity ) == ACT_INVALID )
		{
			// This NPC can't do this!
			Assert( 0 );
		}
		else 
		{
			if (m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{

				AI_NavGoal_t goal( GOALTYPE_TARGETENT, newActivity );
				
				if ( GetState() == NPC_STATE_SCRIPT && 
					 ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY || 
					   m_strScriptArrivalSequence != NULL_STRING ) )
				{
					if ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY )
					{
						goal.arrivalActivity = m_ScriptArrivalActivity;
					}
					else
					{
						goal.arrivalSequence = LookupSequence( m_strScriptArrivalSequence.ToCStr() );
					}
				}
					
				if (!GetNavigator()->SetGoal( goal, AIN_DISCARD_IF_FAIL ))
				{
					if ( GetNavigator()->GetNavFailCounter() == 0 )
					{
						// no path was built, but OnNavFailed() did something so that next time it may work
						DevWarning("%s %s failed Urgent Movement, retrying\n", GetDebugName(), TaskName( task ) );
						return;
					}

					// FIXME: scripted sequences don't actually know how to handle failure, but we're failing.  This is serious
					DevWarning("%s %s failed Urgent Movement, abandoning schedule\n", GetDebugName(), TaskName( task ) );
					TaskFail(FAIL_NO_ROUTE);
				}
				else
				{
					GetNavigator()->SetArrivalDirection( m_hTargetEnt->GetAbsAngles() );
				}
			}
		}
	}

	m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
	m_strScriptArrivalSequence = NULL_STRING;

	TaskComplete();
}


//-----------------------------------------------------------------------------
// Start task!
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartTask( const Task_t *pTask )
{
	int task = pTask->iTask;
	switch ( pTask->iTask )
	{
	case TASK_RESET_ACTIVITY:
		m_Activity = ACT_RESET;
		TaskComplete();
		break;

	case TASK_CREATE_PENDING_WEAPON:
		Assert( m_iszPendingWeapon != NULL_STRING );
		GiveWeapon( m_iszPendingWeapon );
		m_iszPendingWeapon = NULL_STRING;
		TaskComplete();
		break;

	case TASK_RANDOMIZE_FRAMERATE:
		{
			float newRate = GetPlaybackRate();
			float percent = pTask->flTaskData / 100.0f;

			newRate += ( newRate * random->RandomFloat(-percent, percent) );

			SetPlaybackRate(newRate);

			TaskComplete();
		}
		break;

	case TASK_DEFER_DODGE:
		m_flNextDodgeTime = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;

	// Default case just completes.  Override in sub-classes
	// to play sound / animation / or pause
	case TASK_ANNOUNCE_ATTACK:
		TaskComplete();
		break;

	case TASK_TURN_RIGHT:
		StartTurn( -pTask->flTaskData );
		break;

	case TASK_TURN_LEFT:
		StartTurn( pTask->flTaskData );
		break;

	case TASK_REMEMBER:
		Remember ( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_FORGET:
		Forget ( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_FIND_HINTNODE:
	case TASK_FIND_LOCK_HINTNODE:
		{
			if (!GetHintNode())
			{
				SetHintNode( CAI_HintManager::FindHint( this, HINT_NONE, pTask->flTaskData, 2000 ) );
			}
			if (GetHintNode())
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}
			if ( task == TASK_FIND_HINTNODE )
				break;
		}
		// Fall through on TASK_FIND_LOCK_HINTNODE...
		
	case TASK_LOCK_HINTNODE:
	{
		if (!GetHintNode())
		{
			TaskFail(FAIL_NO_HINT_NODE);
		}
		else if( GetHintNode()->Lock(this) )
		{
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_ALREADY_LOCKED);
			SetHintNode( NULL );
		}
		break;
	}

	case TASK_STORE_LASTPOSITION:
		m_vecLastPosition = GetLocalOrigin();
		TaskComplete();
		break;

	case TASK_CLEAR_LASTPOSITION:
		m_vecLastPosition = vec3_origin;
		TaskComplete();
		break;

	case TASK_STORE_POSITION_IN_SAVEPOSITION:
		m_vSavePosition = GetLocalOrigin();
		TaskComplete();
		break;

	case TASK_STORE_BESTSOUND_IN_SAVEPOSITION:
		{
			CSound *pBestSound = GetBestSound();
			if ( pBestSound )
			{
				m_vSavePosition = pBestSound->GetSoundOrigin();
				CBaseEntity *pSoundEnt = pBestSound->m_hOwner;
				if ( pSoundEnt )
				{
					Vector vel;
					pSoundEnt->GetVelocity( &vel, NULL );
					// HACKHACK: run away from cars in the right direction
					m_vSavePosition += vel * 2;	// add in 2 seconds of velocity
				}
				TaskComplete();
			}
			else
			{
				TaskFail("No Sound!");
				return;
			}
		}
		break;

	case TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION:
		{
			CSound *pBestSound = GetBestSound();
			if ( pBestSound )
			{
				m_vSavePosition = pBestSound->GetSoundReactOrigin();
				TaskComplete();
			}
			else
			{
				TaskFail("No Sound!");
				return;
			}
		}
		break;

	case TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION:
		if ( GetEnemy() != NULL )
		{
			m_vSavePosition = GetEnemy()->GetAbsOrigin();
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_NO_ENEMY);
		}
		break;

	case TASK_CLEAR_HINTNODE:
		ClearHintNode(pTask->flTaskData);
		TaskComplete();
		break;

	case TASK_STOP_MOVING:
		if ( ( GetNavigator()->IsGoalSet() && GetNavigator()->IsGoalActive() ) || GetNavType() == NAV_JUMP )
		{
			DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );
			if ( pTask->flTaskData == 1 )
			{
				DbgNavMsg( this, "Initiating stopping path\n" );
				GetNavigator()->StopMoving( false );
			}
			else
			{
				GetNavigator()->ClearGoal();
			}

			// E3 Hack
			if  ( HasPoseMoveYaw() ) 
			{
				SetPoseParameter( m_poseMove_Yaw, 0 );
			}
		}
		else
		{
			if ( pTask->flTaskData == 1 && GetNavigator()->SetGoalFromStoppingPath() )
			{
				DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );
				DbgNavMsg( this, "Initiating stopping path\n" );
			}
			else
			{
				GetNavigator()->ClearGoal();
				SetIdealActivity( GetStoppedActivity() );
				TaskComplete();
			}
		}
		break;

	case TASK_PLAY_PRIVATE_SEQUENCE:
	case TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
	case TASK_PLAY_SEQUENCE:
		SetIdealActivity( (Activity)(int)pTask->flTaskData );
		break;

	case TASK_ADD_GESTURE_WAIT:
		{
			int iLayer = AddGesture( (Activity)(int)pTask->flTaskData );
			if (iLayer > 0)
			{
				float flDuration = GetLayerDuration( iLayer );
				SetWait( flDuration );
			}
			else
			{
				TaskFail( "Unable to allocate gesture" );
			}
			break;
		}

	case TASK_ADD_GESTURE:
		{
			AddGesture( (Activity)(int)pTask->flTaskData );
			TaskComplete();
			break;
		}

	case TASK_PLAY_HINT_ACTIVITY:
		if ( GetHintNode()->HintActivityName() != NULL_STRING )
		{
			Activity hintActivity = (Activity)CAI_BaseNPC::GetActivityID( STRING(GetHintNode()->HintActivityName()) );
			if ( hintActivity != ACT_INVALID )
			{
				SetIdealActivity( GetHintActivity(GetHintNode()->HintType(), hintActivity) );
			}
			else
			{
				int iSequence = LookupSequence(STRING(GetHintNode()->HintActivityName()));
				if ( iSequence > ACT_INVALID )
				{
					SetSequenceById( iSequence ); // ???
					SetIdealActivity( ACT_DO_NOT_DISTURB );
				}
				else
					SetIdealActivity( ACT_IDLE );
			}
		}
		else
		{
			SetIdealActivity( ACT_IDLE );
		}
		break;

	case TASK_SET_SCHEDULE:
		if ( !SetSchedule( pTask->flTaskData ) )
			TaskFail(FAIL_SCHEDULE_NOT_FOUND);
		break;

	case TASK_FIND_BACKAWAY_FROM_SAVEPOSITION:
		{
			if ( GetEnemy() != NULL )
			{
				Vector backPos;
				if ( !GetTacticalServices()->FindBackAwayPos( m_vSavePosition, &backPos ) )
				{
					// no place to backaway
					TaskFail(FAIL_NO_BACKAWAY_NODE);
				}
				else 
				{
					if (GetNavigator()->SetGoal( AI_NavGoal_t( backPos, ACT_RUN ) ) )
					{
						TaskComplete();
					}
					else
					{
						// no place to backaway
						TaskFail(FAIL_NO_ROUTE);
					}
				}
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
			}
		}
		break;

	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
	case TASK_FIND_COVER_FROM_ENEMY:
		{	
			bool 	bNodeCover 		= ( task != TASK_FIND_COVER_FROM_ENEMY );
			float 	flMinDistance 	= ( task == TASK_FIND_FAR_NODE_COVER_FROM_ENEMY ) ? pTask->flTaskData : 0.0;
			float 	flMaxDistance 	= ( task == TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY ) ? pTask->flTaskData : FLT_MAX;
			
			if ( FindCoverFromEnemy( bNodeCover, flMinDistance, flMaxDistance ) )
			{
				if ( task == TASK_FIND_COVER_FROM_ENEMY )
					m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
				TaskComplete();
			}
			else
				TaskFail(FAIL_NO_COVER);
		}
		break;


	case TASK_FIND_COVER_FROM_ORIGIN:
		{
			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) ) 
			{
				AI_NavGoal_t goal(coverPos, ACT_RUN, AIN_HULL_TOLERANCE);
				GetNavigator()->SetGoal( goal );

				m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
		}

		break;

	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
		}
		break;

	case TASK_FACE_HINTNODE:
		
		// If the yaw is locked, this function will not act correctly
		Assert( GetMotor()->IsYawLocked() == false );

		GetMotor()->SetIdealYaw( GetHintNode()->Yaw() );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
   		if ( FacingIdeal() )
   			TaskComplete();
		else
			SetTurnActivity();
		break;
	
	case TASK_FACE_LASTPOSITION:
		GetMotor()->SetIdealYawToTarget( m_vecLastPosition );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
		SetTurnActivity(); 
		break;

	case TASK_FACE_SAVEPOSITION:
		GetMotor()->SetIdealYawToTarget( m_vSavePosition );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
		SetTurnActivity(); 
		break;

	case TASK_FACE_AWAY_FROM_SAVEPOSITION:
		GetMotor()->SetIdealYawToTarget( m_vSavePosition, 0, 180.0 );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
		SetTurnActivity(); 
		break;

	case TASK_SET_IDEAL_YAW_TO_CURRENT:
		GetMotor()->SetIdealYaw( UTIL_AngleMod( GetLocalAngles().y ) );
		TaskComplete();
		break;

	case TASK_FACE_TARGET:
		if ( m_hTargetEnt != NULL )
		{
			GetMotor()->SetIdealYawToTarget( m_hTargetEnt->GetAbsOrigin() );
			SetTurnActivity(); 
		}
		else
		{
			TaskFail(FAIL_NO_TARGET);
		}
		break;
		
	case TASK_FACE_PLAYER:
		// track head to the client for a while.
		SetWait( pTask->flTaskData );
		break;

	case TASK_FACE_ENEMY:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTarget( vecEnemyLKP );
				GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
				SetTurnActivity(); 
			}
			else
			{
				float flReasonableFacing = CalcReasonableFacing( true );
				if ( fabsf( flReasonableFacing - GetMotor()->GetIdealYaw() ) < 1 )
					TaskComplete();
				else
				{
					GetMotor()->SetIdealYaw( flReasonableFacing );
					SetTurnActivity();
				}
			}
			break;
		}

	case TASK_FACE_IDEAL:
		SetTurnActivity();
		break;

	case TASK_FACE_REASONABLE:
		GetMotor()->SetIdealYaw( CalcReasonableFacing() );
		SetTurnActivity();
		break;

	case TASK_FACE_PATH:
		{
			if (!GetNavigator()->IsGoalActive())
			{
				DevWarning( 2, "No route to face!\n");
				TaskFail(FAIL_NO_ROUTE);
			}
			else
			{
				const float NPC_TRIVIAL_TURN = 15;	// (Degrees). Turns this small or smaller, don't bother with a transition.

				GetMotor()->SetIdealYawToTarget( GetNavigator()->GetCurWaypointPos());

				if( fabs( GetMotor()->DeltaIdealYaw() ) <= NPC_TRIVIAL_TURN )
				{
					// This character is already facing the path well enough that 
					// moving will look fairly natural. Don't bother with a transitional
					// turn animation.
					TaskComplete();
					break;
				}
				SetTurnActivity();
			}
		}
		break;

	// don't do anything.
	case TASK_WAIT_PVS:
	case TASK_WAIT_INDEFINITE:
		break;

	// set a future time that tells us when the wait is over.
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
		SetWait( pTask->flTaskData );
		break;

	// set a future time that tells us when the wait is over.
	case TASK_WAIT_RANDOM:
	case TASK_WAIT_FACE_ENEMY_RANDOM:
		SetWait( 0, pTask->flTaskData );
		break;

	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_MOVE_TO_GOAL_RANGE:
		{
			// Identical tasks, except that target_range uses m_hTargetEnt, 
			// and Goal range uses the nav goal
			CBaseEntity *pTarget = NULL;
			if ( task == TASK_MOVE_TO_GOAL_RANGE )
			{
				pTarget = GetNavigator()->GetGoalTarget();
			}
			if ( !pTarget )
			{
				pTarget = m_hTargetEnt.Get();
			}

			if ( pTarget == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (pTarget->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
			{
				TaskComplete();
			}

			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Clear residual state
			}
			else
			{
				// set that we're probably going to stop before the goal
				GetNavigator()->SetArrivalDistance( pTask->flTaskData );
			}

			break;
		}

	case TASK_WAIT_UNTIL_NO_DANGER_SOUND:
		if( !HasCondition( COND_HEAR_DANGER ) )
		{
			TaskComplete();
		}
		break;

	case TASK_TARGET_PLAYER:
		{
			CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );
			if ( pPlayer )
			{
				SetTarget( pPlayer );
				TaskComplete();
			}
			else
				TaskFail( FAIL_NO_PLAYER );
			break;
		}

	case TASK_SCRIPT_RUN_TO_TARGET:
	case TASK_SCRIPT_WALK_TO_TARGET:
	case TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET:
		StartScriptMoveToTargetTask( pTask->iTask );
		break;

	case TASK_CLEAR_MOVE_WAIT:
		m_flMoveWaitFinished = gpGlobals->curtime;
		TaskComplete();
		break;

	case TASK_MELEE_ATTACK1:
		SetLastAttackTime( gpGlobals->curtime );
		ResetIdealActivity( ACT_MELEE_ATTACK1 );
		break;

	case TASK_MELEE_ATTACK2:
		SetLastAttackTime( gpGlobals->curtime );
		ResetIdealActivity( ACT_MELEE_ATTACK2 );
		break;

	case TASK_RANGE_ATTACK1:
		SetLastAttackTime( gpGlobals->curtime );
		ResetIdealActivity( ACT_RANGE_ATTACK1 );
		break;

	case TASK_RANGE_ATTACK2:
		SetLastAttackTime( gpGlobals->curtime );
		ResetIdealActivity( ACT_RANGE_ATTACK2 );
		break;

	case TASK_RELOAD:
		ResetIdealActivity( ACT_RELOAD );
		break;

	case TASK_SPECIAL_ATTACK1:
		ResetIdealActivity( ACT_SPECIAL_ATTACK1 );
		break;

	case TASK_SPECIAL_ATTACK2:
		ResetIdealActivity( ACT_SPECIAL_ATTACK2 );
		break;

	case TASK_SET_ACTIVITY:
		{
			Activity goalActivity = (Activity)((int)pTask->flTaskData);
			if (goalActivity != ACT_RESET)
			{
				SetIdealActivity( goalActivity );
			}
			else
			{
				m_Activity = ACT_RESET;
			}
			break;
		}
	case TASK_GET_CHASE_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( !pEnemy )
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			if ( ( pEnemy->GetAbsOrigin() - GetEnemyLKP() ).LengthSqr() < Square(pTask->flTaskData) )
			{
				ChainStartTask( TASK_GET_PATH_TO_ENEMY );
			}
			else
			{
				ChainStartTask( TASK_GET_PATH_TO_ENEMY_LKP );
			}

			if ( !TaskIsComplete() && !HasCondition(COND_TASK_FAILED) )
				TaskFail(FAIL_NO_ROUTE);
			break;
		}

	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (!pEnemy || IsUnreachable(pEnemy))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}
			AI_NavGoal_t goal( GetEnemyLKP() );

			TranslateNavGoal( pEnemy, goal.dest );

			if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				DevWarning( 2, "GetPathToEnemyLKP failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}

	case TASK_GET_PATH_TO_INTERACTION_PARTNER:
		{
			if ( !m_hForcedInteractionPartner || IsUnreachable(m_hForcedInteractionPartner) )
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			// Calculate the position we need to be at to start the interaction.
			CalculateForcedInteractionPosition();

			AI_NavGoal_t goal( m_vecForcedWorldPosition );
			TranslateNavGoal( m_hForcedInteractionPartner, goal.dest );

			if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				DevWarning( 2, "GetPathToInteractionPartner failed!!\n" );
				RememberUnreachable(m_hForcedInteractionPartner);
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}

	case TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS:
		{
			if ( GetEnemy() )
			{
				// Find out which range to use (either innately or a held weapon)
				float flRange = -1.0f;
				if ( CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2) )
				{
					flRange = InnateRange1MaxRange();
				}
				else if ( GetActiveWeapon() )
				{
					flRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
				}
				else
				{
					// You can't call this task without either innate range attacks or a weapon!
					Assert( 0 );
					TaskFail( FAIL_NO_ROUTE );
				}

				// Clamp to the specified range, if supplied
				if ( pTask->flTaskData != 0 && pTask->flTaskData < flRange )
					flRange = pTask->flTaskData;
						
				// For now, just try running straight at enemy
				float dist = EnemyDistance( GetEnemy() );
				if ( dist <= flRange || GetNavigator()->SetVectorGoalFromTarget( GetEnemy()->GetAbsOrigin(), dist - flRange ) )
				{
					TaskComplete();
					break;
				}
			}

			TaskFail( FAIL_NO_ROUTE );
			break;
		}
	
	case TASK_GET_PATH_TO_ENEMY_LOS:
	case TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS:
	case TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS:
	case TASK_GET_PATH_TO_ENEMY_LKP_LOS:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
		
			AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
			float flMaxRange = 2000;
			float flMinRange = 0;
			
			if ( GetActiveWeapon() )
			{
				flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
				flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
			}
			else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
			{
				flMaxRange = InnateRange1MaxRange();
				flMinRange = InnateRange1MinRange();
			}

			//Check against NPC's max range
			if ( flMaxRange > m_flDistTooFar )
			{
				flMaxRange = m_flDistTooFar;
			}

			Vector vecEnemy 	= ( task != TASK_GET_PATH_TO_ENEMY_LKP ) ? GetEnemy()->GetAbsOrigin() : GetEnemyLKP();
			Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

			Vector posLos;
			bool found = false;

			if ( ( task != TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS ) && ( task != TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS ) )
			{
				if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
				{
					float dist = ( posLos - vecEnemyEye ).Length();
					if ( dist < flMaxRange && dist > flMinRange )
						found = true;
				}
			}
			
			if ( !found )
			{
				FlankType_t eFlankType = FLANKTYPE_NONE;
				Vector vecFlankRefPos = vec3_origin;
				float flFlankParam = 0;
			
				if ( task == TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS )
				{
					eFlankType = FLANKTYPE_RADIUS;
					vecFlankRefPos = m_vSavePosition;
					flFlankParam = pTask->flTaskData;
				}
				else if ( task == TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS )
				{
					eFlankType = FLANKTYPE_ARC;
					vecFlankRefPos = m_vSavePosition;
					flFlankParam = pTask->flTaskData;
				}

				if ( GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, eFlankType, vecFlankRefPos, flFlankParam, &posLos ) )
				{
					found = true;
				}
			}

			if ( !found )
			{
				TaskFail( FAIL_NO_SHOOT );
			}
			else
			{
				// else drop into run task to offer an interrupt
				m_vInterruptSavePosition = posLos;
			}
		}
		break;

//==================================================
// TASK_SET_GOAL
//==================================================

	case TASK_SET_GOAL:

		switch ( (int) pTask->flTaskData )
		{
		case GOAL_ENEMY:	//Enemy
			
			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = GetEnemy()->GetAbsOrigin();
			m_nStoredPathType	= GOALTYPE_ENEMY;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= GetEnemy();
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		
		case GOAL_ENEMY_LKP:		//Enemy's last known position

			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = GetEnemyLKP();
			m_nStoredPathType	= GOALTYPE_LOCATION;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= NULL;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		
		case GOAL_TARGET:			//Target entity
			
			if ( m_hTargetEnt == NULL )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = m_hTargetEnt->GetAbsOrigin();
			m_nStoredPathType	= GOALTYPE_TARGETENT;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= m_hTargetEnt;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;

		case GOAL_SAVED_POSITION:	//Saved position
			
			//Setup our stored info
			m_vecStoredPathGoal = m_vSavePosition;
			m_nStoredPathType	= GOALTYPE_LOCATION;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= NULL;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		}

		TaskComplete();

		break;

//==================================================
// TASK_GET_PATH_TO_GOAL
//==================================================

	case TASK_GET_PATH_TO_GOAL:
		{
			AI_NavGoal_t goal( m_nStoredPathType, 
							   AIN_DEF_ACTIVITY, 
							   AIN_HULL_TOLERANCE,
							   AIN_DEF_FLAGS,
							   m_hStoredPathTarget );
			
			bool	foundPath = false;

			//Find our path
			switch ( (int) pTask->flTaskData )
			{
			case PATH_TRAVEL:	//A land path to our goal
				goal.dest = m_vecStoredPathGoal;
				foundPath = GetNavigator()->SetGoal( goal );
				break;

			case PATH_LOS:		//A path to get LOS to our goal
				{
					float flMaxRange = 2000.0f;
					float flMinRange = 0.0f;

					if ( GetActiveWeapon() )
					{
						flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
						flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
					}
					else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
					{
						flMaxRange = InnateRange1MaxRange();
						flMinRange = InnateRange1MinRange();
					}

					// Check against NPC's max range
					if ( flMaxRange > m_flDistTooFar )
					{
						flMaxRange = m_flDistTooFar;
					}

					Vector	eyePosition = ( m_hStoredPathTarget != NULL ) ? m_hStoredPathTarget->EyePosition() : m_vecStoredPathGoal;

					Vector posLos;

					// See if we've found it
					if ( GetTacticalServices()->FindLos( m_vecStoredPathGoal, eyePosition, flMinRange, flMaxRange, 1.0f, &posLos ) )
					{
						goal.dest = posLos;
						foundPath = GetNavigator()->SetGoal( goal );
					}
					else
					{
						// No LOS to goal
						TaskFail( FAIL_NO_SHOOT );
						return;
					}
				}
				
				break;

			case PATH_COVER:	//Get a path to cover FROM our goal
				{
					CBaseEntity *pEntity = ( m_hStoredPathTarget == NULL ) ? this : m_hStoredPathTarget;

					//Find later cover first
					Vector coverPos;

					if ( GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), 0, &coverPos ) )
					{
						AI_NavGoal_t goal( coverPos, ACT_RUN );
						GetNavigator()->SetGoal( goal, AIN_CLEAR_PREVIOUS_STATE );
						
 						//FIXME: What exactly is this doing internally?
						m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
						TaskComplete();
						return;
					}
					else
					{
						//Try any cover
						if ( GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), &coverPos ) ) 
						{
							//If we've found it, find a safe route there
							AI_NavGoal_t coverGoal( GOALTYPE_COVER, 
													coverPos,
													ACT_RUN,
													AIN_HULL_TOLERANCE,
													AIN_DEF_FLAGS,
													m_hStoredPathTarget );
							
							foundPath = GetNavigator()->SetGoal( goal );

							m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
						}
						else
						{
							TaskFail( FAIL_NO_COVER );
						}
					}
				}

				break;
			}

			//Now validate our try
			if ( foundPath )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_GET_PATH_TO_ENEMY:
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			CBaseEntity *pEnemy = GetEnemy();

			if ( pEnemy == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
						
			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =( 
				DevWarning( 2, "GetPathToEnemy failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_CORPSE:
		{
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward );
			Vector vecEnemyLKP = GetEnemyLKP();

			GetNavigator()->SetGoal( vecEnemyLKP - forward * 64, AIN_CLEAR_TARGET);
		}
		break;

	case TASK_GET_PATH_TO_PLAYER:
		{
			CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );

			AI_NavGoal_t goal;

			goal.type = GOALTYPE_LOCATION;
			goal.dest = pPlayer->WorldSpaceCenter();
			goal.pTarget = pPlayer;

			GetNavigator()->SetGoal( goal );
			break;
		}

	case TASK_GET_PATH_TO_SAVEPOSITION_LOS:
	{
		if ( GetEnemy() == NULL )
		{
			TaskFail(FAIL_NO_ENEMY);
			return;
		}
	
		float flMaxRange = 2000;
		float flMinRange = 0;
		if ( GetActiveWeapon() )
		{
			flMaxRange = MAX(GetActiveWeapon()->m_fMaxRange1,GetActiveWeapon()->m_fMaxRange2);
			flMinRange = MIN(GetActiveWeapon()->m_fMinRange1,GetActiveWeapon()->m_fMinRange2);
		}
		else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
		{
			flMaxRange = InnateRange1MaxRange();
			flMinRange = InnateRange1MinRange();
		}

		// Check against NPC's max range
		if (flMaxRange > m_flDistTooFar)
		{
			flMaxRange = m_flDistTooFar;
		}

		Vector posLos;

		if (GetTacticalServices()->FindLos(m_vSavePosition,m_vSavePosition, flMinRange, flMaxRange, 1.0, &posLos))
		{
			GetNavigator()->SetGoal( AI_NavGoal_t( posLos, ACT_RUN, AIN_HULL_TOLERANCE ) );
		}
		else
		{
			// no coverwhatsoever.
			TaskFail(FAIL_NO_SHOOT);
		}
		break;
	}

	case TASK_GET_PATH_TO_TARGET_WEAPON:
		{
			// Finds the nearest node within the leniency distances,
			// whether the node can see the target or not.
			const float XY_LENIENCY = 64.0;
			const float Z_LENIENCY	= 72.0;

			if (m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				// Since this weapon MAY be on a table, we find the nearest node without verifying
				// line-of-sight, since weapons on the table will not be able to see nodes very nearby.
				int node = GetNavigator()->GetNetwork()->NearestNodeToPoint( this, m_hTargetEnt->GetAbsOrigin(), false );
				CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( node );

				if( !pNode )
				{
					TaskFail( FAIL_NO_ROUTE );
					break;
				}

				bool bHasPath = true;
				Vector vecNodePos;

				vecNodePos = pNode->GetPosition( GetHullType() );

				float flDistZ;
				flDistZ = fabs( vecNodePos.z - m_hTargetEnt->GetAbsOrigin().z );
				if( flDistZ > Z_LENIENCY )
				{
					// The gun is too far away from its nearest node on the Z axis.
					TaskFail( "Target not within Z_LENIENCY!\n");
					CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
					if( pWeapon )
					{
						// Lock this weapon for a long time so no one else tries to get it.
						pWeapon->Lock( 30.0f, pWeapon );
						break;
					}
				}

				if( flDistZ >= 16.0 )
				{
					// The gun is higher or lower, but it's within reach. (probably on a table).
					float flDistXY = ( vecNodePos - m_hTargetEnt->GetAbsOrigin() ).Length2D();

					// This means we have to stand on the nearest node and still be able to
					// reach the gun.
					if( flDistXY > XY_LENIENCY )
					{
						TaskFail( "Target not within XY_LENIENCY!\n" );
						CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
						if( pWeapon )
						{
							// Lock this weapon for a long time so no one else tries to get it.
							pWeapon->Lock( 30.0f, pWeapon );
							break;
						}
					}

					AI_NavGoal_t goal( vecNodePos );
					goal.pTarget = m_hTargetEnt;
					bHasPath = GetNavigator()->SetGoal( goal );
				}
				else
				{
					// The gun is likely just lying on the floor. Just pick it up.
					AI_NavGoal_t goal( m_hTargetEnt->GetAbsOrigin() );
					goal.pTarget = m_hTargetEnt;
					bHasPath = GetNavigator()->SetGoal( goal );
				}

				if( !bHasPath )
				{
					CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
					if( pWeapon )
					{
						// Lock this weapon for a long time so no one else tries to get it.
						pWeapon->Lock( 15.0f, pWeapon );
					}
				}
			}
		}
		break;

	case TASK_GET_PATH_TO_TARGET:
		{
			if (m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				AI_NavGoal_t goal( static_cast<const Vector&>(m_hTargetEnt->EyePosition()) );
				goal.pTarget = m_hTargetEnt;
				GetNavigator()->SetGoal( goal );
			}
			break;
		}

	case TASK_GET_PATH_TO_HINTNODE:// for active idles!
		{
			if (!GetHintNode())
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}
			else
			{
				Vector vHintPos;
				GetHintNode()->GetPosition(this, &vHintPos);

				GetNavigator()->SetGoal( AI_NavGoal_t( vHintPos, ACT_RUN ) );
				if ( pTask->flTaskData == 0 )
					GetNavigator()->SetArrivalDirection( GetHintNode()->GetDirection() );
				if ( GetHintNode()->HintActivityName() != NULL_STRING )
				{
					Activity hintActivity = (Activity)CAI_BaseNPC::GetActivityID( STRING(GetHintNode()->HintActivityName()) );
					if ( hintActivity != ACT_INVALID )
					{
						GetNavigator()->SetArrivalActivity( GetHintActivity(GetHintNode()->HintType(), hintActivity) );
					}
					else
					{
						int iSequence = LookupSequence(STRING(GetHintNode()->HintActivityName()));
						if ( iSequence != ACT_INVALID )
							GetNavigator()->SetArrivalSequence( iSequence );
					}

				}
			}
			break;
		}

	case TASK_GET_PATH_TO_COMMAND_GOAL:
		{
			if (!GetNavigator()->SetGoal( m_vecCommandGoal ))
			{
				OnMoveToCommandGoalFailed();
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}

	case TASK_MARK_COMMAND_GOAL_POS:
		// Start watching my position to detect whether another AI process has moved me from my mark.
		m_CommandMoveMonitor.SetMark( this, COMMAND_GOAL_TOLERANCE );
		TaskComplete();
		break;

	case TASK_CLEAR_COMMAND_GOAL:
		m_vecCommandGoal = vec3_invalid;
		TaskComplete();
		break;
		
	case TASK_GET_PATH_TO_LASTPOSITION:
		{
			if (!GetNavigator()->SetGoal( m_vecLastPosition ))
			{
				TaskFail(FAIL_NO_ROUTE);
			}
			else
			{
				GetNavigator()->SetGoalTolerance( 48 );
			}
			break;
		}

	case TASK_GET_PATH_TO_SAVEPOSITION:
		{
			GetNavigator()->SetGoal( m_vSavePosition );
			break;
		}


	case TASK_GET_PATH_TO_RANDOM_NODE:  // Task argument is lenth of path to build
		{
			if ( GetNavigator()->SetRandomGoal( pTask->flTaskData ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
		
			break;
		}

	case TASK_GET_PATH_TO_BESTSOUND:
		{

			CSound *pSound = GetBestSound();
			if (!pSound)
			{
				TaskFail(FAIL_NO_SOUND);
			}
			else
			{
				GetNavigator()->SetGoal( pSound->GetSoundReactOrigin() );
			}
			break;
		}
	case TASK_GET_PATH_TO_BESTSCENT:
		{

			CSound *pScent = GetBestScent();
			if (!pScent) 
			{
				TaskFail(FAIL_NO_SCENT);
			}
			else
			{
				GetNavigator()->SetGoal( pScent->GetSoundOrigin() );
			}
			break;
		}
	
	case TASK_GET_PATH_AWAY_FROM_BEST_SOUND:
	{
		CSound *pBestSound = GetBestSound();
		if ( !pBestSound )
		{
			TaskFail("No Sound!");
			break;
		}

		GetMotor()->SetIdealYawToTarget( pBestSound->GetSoundOrigin() );
		ChainStartTask( TASK_MOVE_AWAY_PATH, pTask->flTaskData );
		LockBestSound();
		break;
	}	
	
	case TASK_MOVE_AWAY_PATH:
		{
			// Drop into run task to support interrupt
			DesireStand();
		}
		break;

	case TASK_WEAPON_RUN_PATH:
	case TASK_ITEM_RUN_PATH:
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;

	case TASK_RUN_PATH:
		{
			// UNDONE: This is in some default AI and some NPCs can't run? -- walk instead?
			if ( TranslateActivity( ACT_RUN ) != ACT_INVALID )
			{
				GetNavigator()->SetMovementActivity( ACT_RUN );
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_WALK);
			}
			// Cover is void once I move
			Forget( bits_MEMORY_INCOVER );
			TaskComplete();
			break;
		}

	case TASK_WALK_PATH_FOR_UNITS:
	{
		GetNavigator()->SetMovementActivity(ACT_WALK);
		break;
	}
		
	case TASK_RUN_PATH_FOR_UNITS:
	{
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;
	}	
	
	case TASK_WALK_PATH:
		{
			bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
			if ( bIsFlying && ( TranslateActivity( ACT_FLY ) != ACT_INVALID) )
			{
				GetNavigator()->SetMovementActivity(ACT_FLY);
			}
			else if ( TranslateActivity( ACT_WALK ) != ACT_INVALID )
			{
				GetNavigator()->SetMovementActivity(ACT_WALK);
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_RUN);
			}
			// Cover is void once I move
			Forget( bits_MEMORY_INCOVER );
			TaskComplete();
			break;
		}
	case TASK_WALK_PATH_WITHIN_DIST:
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
			// set that we're probably going to stop before the goal
			GetNavigator()->SetArrivalDistance( pTask->flTaskData );
			break;
		}
	case TASK_RUN_PATH_WITHIN_DIST:
		{
			GetNavigator()->SetMovementActivity(ACT_RUN);
			// set that we're probably going to stop before the goal
			GetNavigator()->SetArrivalDistance( pTask->flTaskData );
			break;
		}
	case TASK_RUN_PATH_FLEE:
		{
			Vector vecDiff;
			vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

			if( vecDiff.Length() <= pTask->flTaskData )
			{
				GetNavigator()->StopMoving();
				TaskFail("Flee path shorter than task parameter");
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_RUN);
			}

			break;
		}
	case TASK_WALK_PATH_TIMED:
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
			SetWait( pTask->flTaskData );
			break;
		}
	case TASK_RUN_PATH_TIMED:
		{
			GetNavigator()->SetMovementActivity(ACT_RUN);
			SetWait( pTask->flTaskData );
			break;
		}
	case TASK_STRAFE_PATH:
		{
			Vector2D vec2DirToPoint; 
			Vector2D vec2RightSide;

			// to start strafing, we have to first figure out if the target is on the left side or right side
			Vector right;
			AngleVectors( GetLocalAngles(), NULL, &right, NULL );

			vec2DirToPoint = ( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() ).AsVector2D();
			Vector2DNormalize(vec2DirToPoint);
			vec2RightSide = right.AsVector2D();
			Vector2DNormalize(vec2RightSide);

			if ( DotProduct2D ( vec2DirToPoint, vec2RightSide ) > 0 )
			{
				// strafe right
				GetNavigator()->SetMovementActivity(ACT_STRAFE_RIGHT);
			}
			else
			{
				// strafe left
				GetNavigator()->SetMovementActivity(ACT_STRAFE_LEFT);
			}
			TaskComplete();
			break;
		}

	case TASK_WAIT_FOR_MOVEMENT_STEP:
		{
			if(!GetNavigator()->IsGoalActive())
			{
				TaskComplete();
				return;
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
			ValidateNavGoal();
			break;
		}

	case TASK_WAIT_FOR_MOVEMENT:
		{
			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Clear residual state
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			Remember(bits_MEMORY_FLINCHED);
			SetIdealActivity( GetFlinchActivity( false, false ) );
			m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 3, 5 );
			break;
		}
	case TASK_BIG_FLINCH:
		{
			Remember(bits_MEMORY_FLINCHED);
			SetIdealActivity( GetFlinchActivity( true, false ) );
			m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 3, 5 );
			break;
		}
	case TASK_DIE:
		{
			GetNavigator()->StopMoving();	
			SetIdealActivity( GetDeathActivity() );
			m_lifeState = LIFE_DYING;

			break;
		}
	case TASK_SOUND_WAKE:
		{
			AlertSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DIE:
		{
			CTakeDamageInfo info;
			DeathSound( info );
			TaskComplete();
			break;
		}
	case TASK_SOUND_IDLE:
		{
			IdleSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_PAIN:
		{
			CTakeDamageInfo info;
			PainSound( info );
			TaskComplete();
			break;
		}
	case TASK_SOUND_ANGRY:
		{
			// sounds are complete as soon as we get here, cause we've already played them.
			DevMsg( 2, "SOUND\n" );			
			TaskComplete();
			break;
		}
	case TASK_SPEAK_SENTENCE:
		{
			SpeakSentence(pTask->flTaskData);	
			TaskComplete();
			break;
		}
	case TASK_WAIT_FOR_SPEAK_FINISH:
		{
			if ( !GetExpresser() )
				TaskComplete();
			else
			{
				// Are we waiting for our speech to end? Or for the mutex to be free?
				if ( pTask->flTaskData )
				{
					// Waiting for our speech to end
					if ( GetExpresser()->CanSpeakAfterMyself() )
					{
						TaskComplete();
					}
				}
				else
				{
					// Waiting for the speech & the delay afterwards
					if ( !GetExpresser()->IsSpeaking() )
					{
						TaskComplete();
					}
				}
				break;

			}
			break;
		}
	case TASK_WAIT_FOR_SCRIPT:
		{
			if ( !m_hCine )
			{
				DevMsg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			break;
		}
	case TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY:
		{
			if ( !m_hCine )
			{
				DevMsg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			string_t iszArrivalText;

			if ( m_hCine->m_iszEntry != NULL_STRING )
			{
				iszArrivalText = m_hCine->m_iszEntry;
			}
			else if ( m_hCine->m_iszPlay != NULL_STRING )
			{
				iszArrivalText = m_hCine->m_iszPlay;
			}
			else if ( m_hCine->m_iszPostIdle != NULL_STRING )
			{
				iszArrivalText = m_hCine->m_iszPostIdle;
			}
			else
				iszArrivalText = NULL_STRING;

			m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
			m_strScriptArrivalSequence = NULL_STRING;

			if ( iszArrivalText != NULL_STRING )
			{
				m_ScriptArrivalActivity = (Activity)GetActivityID( STRING( iszArrivalText ) );
				if ( m_ScriptArrivalActivity == ACT_INVALID )
					m_strScriptArrivalSequence = iszArrivalText;
			}

			TaskComplete();
			break;
		}

	case TASK_PLAY_SCRIPT:
		{
			// Throw away any stopping paths we have saved, because we 
			// won't be able to resume them after the sequence.
			GetNavigator()->IgnoreStoppingPath();

			if ( HasMovement( GetSequence() ) || m_hCine->m_bIgnoreGravity )
			{
				AddFlag( FL_FLY );
				SetGroundEntity( NULL );
			}

			if (m_hCine)
			{
				m_hCine->SynchronizeSequence( this );
			}
			//
			// Start playing a scripted sequence.
			//
			m_scriptState = SCRIPT_PLAYING;
			break;
		}
	case TASK_PLAY_SCRIPT_POST_IDLE:
		{
			//
			// Start playing a scripted post idle.
			//
			m_scriptState = SCRIPT_POST_IDLE;
			break;
		}

	// This is the first task of every schedule driven by a scripted_sequence.
	// Delay starting the sequence until all actors have hit their marks.
	case TASK_PRE_SCRIPT:
		{
			if ( !ai_task_pre_script.GetBool() )
			{
				TaskComplete();
			}
			else if ( !m_hCine )
			{
				TaskComplete();
				//DevMsg( "Scripted sequence destroyed while in use\n" );
				//TaskFail( FAIL_SCHEDULE_NOT_FOUND );
			}
			else
			{
				m_hCine->DelayStart( true );
				TaskComplete();
			}
			break;
		}

	case TASK_ENABLE_SCRIPT:
		{
			//
			// Start waiting to play a script. Play the script's pre idle animation if one
			// is specified, otherwise just go to our default idle activity.
			//
			if ( m_hCine->m_iszPreIdle != NULL_STRING )
			{
				m_hCine->StartSequence( ( CAI_BaseNPC * )this, m_hCine->m_iszPreIdle, false );
				if ( FStrEq( STRING( m_hCine->m_iszPreIdle ), STRING( m_hCine->m_iszPlay ) ) )
				{
					m_flPlaybackRate = 0;
				}
			}
			else if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
			{
				// FIXME: too many ss assume its safe to leave the npc is whatever sequence they were in before, so only slam their activity
				//		  if they're playing a recognizable movement animation
				//
#ifdef HL2_EPISODIC
				// dvs: Check current activity rather than ideal activity. Since scripted NPCs early out in MaintainActivity,
				//      they'll never reach their ideal activity if it's different from their current activity.
				if ( GetActivity() == ACT_WALK || 
					 GetActivity() == ACT_RUN || 
					 GetActivity() == ACT_WALK_AIM || 
					 GetActivity() == ACT_RUN_AIM )
				{
					SetActivity( ACT_IDLE );
				}
#else
				if ( GetIdealActivity() == ACT_WALK || 
					 GetIdealActivity() == ACT_RUN || 
					 GetIdealActivity() == ACT_WALK_AIM || 
					 GetIdealActivity() == ACT_RUN_AIM )
				{
					SetActivity( ACT_IDLE );
				}
#endif // HL2_EPISODIC
			}
			break;
		}
	case TASK_PLANT_ON_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				SetLocalOrigin( m_hTargetEnt->GetAbsOrigin() );	// Plant on target
			}

			TaskComplete();
			break;
		}
	case TASK_FACE_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				GetMotor()->SetIdealYaw( UTIL_AngleMod( m_hTargetEnt->GetLocalAngles().y ) );
			}

			if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
			{
				SetTurnActivity();
				
				// dvs: HACK: MaintainActivity won't do anything while scripted, so go straight there.
				SetActivity( GetIdealActivity() );
			}

			GetNavigator()->StopMoving();
			break;
		}

	case TASK_PLAY_SCENE:
		{
			// inside a scene with movement and sequence commands
			break;
		}


	case TASK_SUGGEST_STATE:
		{
			SetIdealState( (NPC_STATE)(int)pTask->flTaskData );
			TaskComplete();
			break;
		}

	case TASK_SET_FAIL_SCHEDULE:
		m_failSchedule = (int)pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_SET_TOLERANCE_DISTANCE:
		GetNavigator()->SetGoalTolerance( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_SET_ROUTE_SEARCH_TIME:
		GetNavigator()->SetMaxRouteRebuildTime( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_CLEAR_FAIL_SCHEDULE:
		m_failSchedule = SCHED_NONE;
		TaskComplete();
		break;

	case TASK_WEAPON_FIND:
		{
			m_hTargetEnt = Weapon_FindUsable( Vector(1000,1000,1000) );
			if (m_hTargetEnt)
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_ITEM_NO_FIND);
			}
		}
		break;

	case TASK_ITEM_PICKUP:
		{
			SetIdealActivity( ACT_PICKUP_GROUND );
		}
		break;

	case TASK_WEAPON_PICKUP:
		{
  			if( GetActiveWeapon() )
  			{
  				Weapon_Drop( GetActiveWeapon() );
  			}

			if( GetTarget() )
			{
				CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>(GetTarget());
				if( pWeapon )
				{
					if( Weapon_IsOnGround( pWeapon ) )
					{
						// Squat down
						SetIdealActivity( ACT_PICKUP_GROUND );
					}
					else
					{
						// Reach and take this weapon from rack or shelf.
						SetIdealActivity( ACT_PICKUP_RACK );
					}

					return;
				}
			}

			TaskFail("Weapon went away!\n");
		}
		break;

	case TASK_WEAPON_CREATE:
		{
			if( !GetActiveWeapon() && GetTarget() )
			{
				// Create a copy of the weapon this NPC is trying to pick up.
				CBaseCombatWeapon *pTargetWeapon = dynamic_cast<CBaseCombatWeapon*>(GetTarget());

				if( pTargetWeapon )
				{
					CBaseCombatWeapon *pWeapon = Weapon_Create( pTargetWeapon->GetClassname() );
					if ( pWeapon )
					{
						Weapon_Equip( pWeapon );
					}
				}
			}
			SetTarget( NULL );
			TaskComplete();
		}
		break;

	case TASK_USE_SMALL_HULL:
		{
			SetHullSizeSmall();
			TaskComplete();
		}
		break;
	
	case TASK_FALL_TO_GROUND:
		// Set a wait time to try to force a ground ent.
		SetWait(4);
		break;
		
	case TASK_WANDER:
		{
			// This task really uses 2 parameters, so we have to extract
			// them from a single integer. To send both parameters, the
			// formula is MIN_DIST * 10000 + MAX_DIST
			{
				int iMinDist, iMaxDist, iParameter;

				iParameter = pTask->flTaskData;

				iMinDist = iParameter / 10000;
				iMaxDist = iParameter - (iMinDist * 10000);

				if ( GetNavigator()->SetWanderGoal( iMinDist, iMaxDist ) )
					TaskComplete();
				else
					TaskFail(FAIL_NO_REACHABLE_NODE);
			}
		}
		break;

	case TASK_FREEZE:
		m_flPlaybackRate = 0;
		break;

	case TASK_GATHER_CONDITIONS:
		GatherConditions();
		TaskComplete();
		break;

	case TASK_IGNORE_OLD_ENEMIES:
		m_flAcceptableTimeSeenEnemy = gpGlobals->curtime;
		if ( GetEnemy() && GetEnemyLastTimeSeen() < m_flAcceptableTimeSeenEnemy )
		{
			CBaseEntity *pNewEnemy = BestEnemy();

			Assert( pNewEnemy != GetEnemy() );

			if( pNewEnemy != NULL )
			{
				// New enemy! Clear the timers and set conditions.
				SetEnemy( pNewEnemy );
				SetState( NPC_STATE_COMBAT );
			}
			else
			{
				SetEnemy( NULL );
				ClearAttackConditions();
			}
		}
		TaskComplete();
		break;

	case TASK_ADD_HEALTH:
		TakeHealth( (int)pTask->flTaskData, DMG_GENERIC );
		TaskComplete();
		break;

	default:
		{
			DevMsg( "No StartTask entry for %s\n", TaskName( task ) );
		}
		break;
	}
}

void CAI_BaseNPC::StartTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		if ( ShouldMoveAndShoot() )
		{
			m_MoveAndShootOverlay.StartShootWhileMove();
		}
		else
		{
			m_MoveAndShootOverlay.NoShootWhileMove();
		}
	}
}


//-----------------------------------------------------------------------------
// TASK_DIE.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RunDieTask()
{
	AutoMovement();

	if ( IsActivityFinished() && GetCycle() >= 1.0f )
	{
		m_lifeState = LIFE_DEAD;
		
		SetThink ( NULL );
		StopAnimation();

		if ( !BBoxFlat() )
		{
			// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
			// block the player on a slope or stairs, the corpse is made nonsolid. 
//					SetSolid( SOLID_NOT );
			UTIL_SetSize ( this, Vector ( -4, -4, 0 ), Vector ( 4, 4, 1 ) );
		}
		else // !!!HACKHACK - put NPC in a thin, wide bounding box until we fix the solid type/bounding volume problem
			UTIL_SetSize ( this, WorldAlignMins(), Vector ( WorldAlignMaxs().x, WorldAlignMaxs().y, WorldAlignMins().z + 1 ) );
	}
}


//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1 / TASK_RANGE_ATTACK2 / etc.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RunAttackTask( int task )
{
	AutoMovement( );

	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if( vecEnemyLKP != vec3_origin )
	{
		if ( ( task == TASK_RANGE_ATTACK1 || task == TASK_RELOAD ) && 
			 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
			 FInAimCone( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	if ( IsActivityFinished() )
	{
		if ( task == TASK_RELOAD && GetShotRegulator() )
		{
			GetShotRegulator()->Reset( false );
		}

		TaskComplete();
	}
}


//=========================================================
// RunTask 
//=========================================================
void CAI_BaseNPC::RunTask( const Task_t *pTask )
{
	VPROF_BUDGET( "CAI_BaseNPC::RunTask", VPROF_BUDGETGROUP_NPCS );
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_RANDOM_NODE:
		{
			break;
		}
	case TASK_TURN_RIGHT:
	case TASK_TURN_LEFT:
		{
			// If the yaw is locked, this function will not act correctly
			Assert( GetMotor()->IsYawLocked() == false );

			GetMotor()->UpdateYaw();

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
		{
			CBaseEntity *pTarget;

			if ( pTask->iTask == TASK_PLAY_SEQUENCE_FACE_TARGET )
				pTarget = m_hTargetEnt;
			else
				pTarget = GetEnemy();
			if ( pTarget )
			{
				GetMotor()->SetIdealYawAndUpdate( pTarget->GetAbsOrigin() - GetLocalOrigin() , AI_KEEP_YAW_SPEED );
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}		
		break;

	case TASK_PLAY_HINT_ACTIVITY:
		{
			if (!GetHintNode())
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}

			// Put a debugging check in here
			if (GetHintNode()->User() != this)
			{
				DevMsg("Hint node (%s) being used by non-owner!\n",GetHintNode()->GetDebugName());
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			break;
		}

	case TASK_STOP_MOVING:
		{
			if ( pTask->flTaskData == 1 )
			{
				ChainRunTask( TASK_WAIT_FOR_MOVEMENT );
				if ( GetTaskStatus() == TASKSTATUS_COMPLETE )
				{
					DbgNavMsg( this, "TASK_STOP_MOVING Complete\n" );
				}
			}
			else
			{
				// if they're jumping, wait until they land
				if (GetNavType() == NAV_JUMP)
				{
					if (GetFlags() & FL_ONGROUND)
					{
						DbgNavMsg( this, "Jump landed\n" );
						SetNavType( NAV_GROUND ); // this assumes that NAV_JUMP only happens with npcs that use NAV_GROUND as base movement
					}
					else if (GetSmoothedVelocity().Length() > 0.01) // use an EPSILON
					{
						// wait until you land
						break;
					}
					else
					{
						DbgNavMsg( this, "Jump stuck\n" );
						// stopped and stuck!
						SetNavType( NAV_GROUND );
						TaskFail( FAIL_STUCK_ONTOP );						
					}
				}

				// @TODO (toml 10-30-02): this is unacceptable, but needed until navigation can handle commencing
				// 						  a navigation while in the middle of a climb
				if (GetNavType() == NAV_CLIMB)
				{
					// wait until you reach the end
					break;
				}

				DbgNavMsg( this, "TASK_STOP_MOVING Complete\n" );
				SetIdealActivity( GetStoppedActivity() );

				TaskComplete();
			}
			break;
		}

	case TASK_PLAY_SEQUENCE:
	case TASK_PLAY_PRIVATE_SEQUENCE:
		{
			AutoMovement( );
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_ADD_GESTURE_WAIT:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_SET_ACTIVITY:
		{
			if ( IsActivityStarted() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_FACE_ENEMY:
		{
			// If the yaw is locked, this function will not act correctly
			Assert( GetMotor()->IsYawLocked() == false );

			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTarget( vecEnemyLKP );
				GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
			}
			else
			{
				float flReasonableFacing = CalcReasonableFacing( true );
				if ( fabsf( flReasonableFacing - GetMotor()->GetIdealYaw() ) > 1 )
					GetMotor()->SetIdealYaw( flReasonableFacing );
			}

			GetMotor()->UpdateYaw();
			
			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			CBasePlayer *pPlayer = AI_GetSinglePlayer();
			if ( pPlayer )
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
				SetTurnActivity();
				if ( IsWaitFinished() && GetMotor()->DeltaIdealYaw() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_NO_PLAYER);
			}
		}
		break;

	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
			switch( GetTaskInterrupt() )
			{
			case 0:
				{
					if ( !FindCoverFromBestSound( &m_vInterruptSavePosition ) )
						TaskFail(FAIL_NO_COVER);
					else
					{
						GetNavigator()->IgnoreStoppingPath();
						LockBestSound();
						TaskInterrupt();
					}
				}
				break;

			case 1:
				{
					AI_NavGoal_t goal(m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE);

					CSound *pBestSound = GetBestSound();
					if ( pBestSound )
						goal.maxInitialSimplificationDist = pBestSound->Volume() * 0.5;

					if ( GetNavigator()->SetGoal( goal ) )
					{
						m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
					}
				}
				break;
			}
		}
		break;

	case TASK_FACE_HINTNODE:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_SAVEPOSITION:
	case TASK_FACE_AWAY_FROM_SAVEPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_IDEAL:
	case TASK_FACE_SCRIPT:
	case TASK_FACE_PATH:
		{
			// If the yaw is locked, this function will not act correctly
			Assert( GetMotor()->IsYawLocked() == false );

			GetMotor()->UpdateYaw();
   
   			if ( FacingIdeal() )
   			{
   				TaskComplete();
   			}
   			break;
		}

	case TASK_FACE_REASONABLE:
		{
			// If the yaw is locked, this function will not act correctly
			Assert( GetMotor()->IsYawLocked() == false );

			GetMotor()->UpdateYaw();

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_PVS:
		{
			if ( ShouldAlwaysThink() || 
				 UTIL_FindClientInPVS(edict()) || 
				 ( GetState() == NPC_STATE_COMBAT && GetEnemy() && gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() ) < 15 ) )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_RANDOM:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_FACE_ENEMY_RANDOM:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP , AI_KEEP_YAW_SPEED );
			}

			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_UNTIL_NO_DANGER_SOUND:
		if( !HasCondition( COND_HEAR_DANGER ) )
		{
			TaskComplete();
		}
		break;

	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_MOVE_TO_GOAL_RANGE:
		{
			// Identical tasks, except that target_range uses m_hTargetEnt, 
			// and Goal range uses the nav goal
			CBaseEntity *pTarget = NULL;
			if ( pTask->iTask == TASK_MOVE_TO_GOAL_RANGE )
			{
				pTarget = GetNavigator()->GetGoalTarget();
			}
			if ( !pTarget )
			{
				pTarget = m_hTargetEnt.Get();
			}

			float distance;

			if ( pTarget == NULL )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Clear residual state
			}
			else
			{
				bool bForceRun = false; 

				// Check Z first, and only check 2d if we're within that
				Vector vecGoalPos = GetNavigator()->GetGoalPos();
				distance = fabs(vecGoalPos.z - GetLocalOrigin().z);
				if ( distance < pTask->flTaskData )
				{
					distance = ( vecGoalPos - GetLocalOrigin() ).Length2D();
				}
				else
				{
					// If the target is significantly higher or lower than me, I must run. 
					bForceRun = true;
				}

				// If we're jumping, wait until we're finished to update our goal position.
				if ( GetNavigator()->GetNavType() != NAV_JUMP )
				{
					// Re-evaluate when you think your finished, or the target has moved too far
					if ( (distance < pTask->flTaskData) || (vecGoalPos - pTarget->GetAbsOrigin()).Length() > pTask->flTaskData * 0.5 )
					{
						distance = ( pTarget->GetAbsOrigin() - GetLocalOrigin() ).Length2D();
						if ( !GetNavigator()->UpdateGoalPos( pTarget->GetAbsOrigin() ) )
						{
							TaskFail( FAIL_NO_ROUTE );
							break;
						}
					}
				}
				
				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility
				if ( distance < pTask->flTaskData )
				{
					TaskComplete();
#ifndef HL2_DLL
	// HL2 uses TASK_STOP_MOVING
					GetNavigator()->StopMoving();		// Stop moving
#endif
				}
				else
				{
					// Pick the right movement activity.
					Activity followActivity; 
						
					if( bForceRun )
					{
						followActivity = ACT_RUN;
					}
					else
					{
						followActivity = ( distance < 190 && m_NPCState != NPC_STATE_COMBAT ) ? ACT_WALK : ACT_RUN;
					}

					// Don't confuse move and shoot by resetting the activity every think
					Activity curActivity = GetNavigator()->GetMovementActivity();
					switch( curActivity )
					{
					case ACT_WALK_AIM:	curActivity = ACT_WALK;	break;
					case ACT_RUN_AIM:	curActivity = ACT_RUN;	break;
					}

					if ( curActivity != followActivity )
					{
						GetNavigator()->SetMovementActivity(followActivity);
					}
					GetNavigator()->SetArrivalDirection( pTarget );
				}
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_LOS:
	case TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS:
	case TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS:
	case TASK_GET_PATH_TO_ENEMY_LKP_LOS:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
			if ( GetTaskInterrupt() > 0 )
			{
				ClearTaskInterrupt();

				Vector vecEnemy = ( pTask->iTask == TASK_GET_PATH_TO_ENEMY_LOS ) ? GetEnemy()->GetAbsOrigin() : GetEnemyLKP();
				AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

				GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
				GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
			}
			else
				TaskInterrupt();
		}
		break;

	case TASK_GET_PATH_AWAY_FROM_BEST_SOUND:
	{
		ChainRunTask( TASK_MOVE_AWAY_PATH, pTask->flTaskData );
		if ( GetNavigator()->IsGoalActive() )
		{
			Vector vecDest = GetNavigator()->GetGoalPos();
			float flDist = ( GetAbsOrigin() - vecDest ).Length();

			if( flDist < 10.0 * 12.0 )
			{
				TaskFail("Path away from best sound too short!\n");
			}
		}
		break;
	}	

	case TASK_MOVE_AWAY_PATH:
		{
			QAngle ang = GetLocalAngles();
			ang.y = GetMotor()->GetIdealYaw() + 180;
			Vector move;

			switch ( GetTaskInterrupt() )
			{
			case 0:
				{
					if( IsPlayerAlly() )
					{
						// Look for a move away hint node.
						CAI_Hint *pHint;
						CHintCriteria hintCriteria;

						hintCriteria.AddHintType( HINT_PLAYER_ALLY_MOVE_AWAY_DEST );
						hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
						hintCriteria.AddIncludePosition( GetAbsOrigin(), (20.0f * 12.0f) ); // 20 feet max
						hintCriteria.AddExcludePosition( GetAbsOrigin(), 28.0f ); // don't plant on an hint that you start on

						pHint = CAI_HintManager::FindHint( this, hintCriteria );

						if( pHint )
						{
							CBasePlayer *pPlayer = AI_GetSinglePlayer();
							Vector vecGoal = pHint->GetAbsOrigin();

							if( vecGoal.DistToSqr(GetAbsOrigin()) < vecGoal.DistToSqr(pPlayer->GetAbsOrigin()) )
							{
								if( GetNavigator()->SetGoal(vecGoal) )
								{
									pHint->DisableForSeconds( 0.1f ); // Force others to find their own.
									TaskComplete();
									break;
								}
							}
						}
					}

#ifdef HL2_EPISODIC
					// See if we're moving away from a vehicle
					CSound *pBestSound = GetBestSound( SOUND_MOVE_AWAY );
					if ( pBestSound && pBestSound->m_hOwner && pBestSound->m_hOwner->GetServerVehicle() )
					{
						// Move away from the vehicle's center, regardless of our facing
						move = ( GetAbsOrigin() - pBestSound->m_hOwner->WorldSpaceCenter() );
						VectorNormalize( move );
					}
					else
					{
						// Use the first angles
						AngleVectors( ang, &move );
					}
#else
					AngleVectors( ang, &move );
#endif	//HL2_EPISODIC
					if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(36,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ))
					{
						TaskComplete();
					}
					else
					{
						ang.y = GetMotor()->GetIdealYaw() + 91;
						AngleVectors( ang, &move );

						if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(24,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
						{
							TaskComplete();
						}
						else
						{
							TaskInterrupt();
						}
					}
				}
				break;

			case 1:
				{
					ang.y = GetMotor()->GetIdealYaw() + 271;
					AngleVectors( ang, &move );

					if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(24,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
					{
						TaskComplete();
					}
					else
					{
						ang.y = GetMotor()->GetIdealYaw() + 180;
						while (ang.y < 0)
							ang.y += 360;
						while (ang.y >= 360)
							ang.y -= 360;
						if ( ang.y < 45 || ang.y >= 315 )
							ang.y = 0;
						else if ( ang.y < 135 )
							ang.y = 90;
						else if ( ang.y < 225 )
							ang.y = 180;
						else
							ang.y = 270;

						AngleVectors( ang, &move );

						if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(6,pTask->flTaskData), false ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
						{
							TaskComplete();
						}
						else
						{
							TaskInterrupt();
						}
					}
				}
				break;

			case 2:
				{
					ClearTaskInterrupt();
					Vector coverPos;

					if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) ) 
					{
						GetNavigator()->SetGoal( AI_NavGoal_t( coverPos, ACT_RUN ) );
						m_flMoveWaitFinished = gpGlobals->curtime + 2;
					}
					else
					{
						// no coverwhatsoever.
						TaskFail(FAIL_NO_ROUTE);
					}
				}
				break;

			}
		}
		break;

	case TASK_WEAPON_RUN_PATH:
	case TASK_ITEM_RUN_PATH:
		{
			CBaseEntity *pTarget = m_hTargetEnt;
			if ( pTarget )
			{
				if ( pTarget->GetOwnerEntity() )
				{
					TaskFail(FAIL_WEAPON_OWNED);
				}
				else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_ITEM_NO_FIND);
			}
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT_STEP:
	case TASK_WAIT_FOR_MOVEMENT:
		{
			bool fTimeExpired = ( pTask->flTaskData != 0 && pTask->flTaskData < gpGlobals->curtime - GetTimeTaskStarted() );
			
			if (fTimeExpired || GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->StopMoving();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}
			break;
		}

	case TASK_DIE:
		RunDieTask();
		break;

	case TASK_WAIT_FOR_SPEAK_FINISH:
		Assert( GetExpresser() );
		if ( GetExpresser() )
		{
			// Are we waiting for our speech to end? Or for the mutex to be free?
			if ( pTask->flTaskData )
			{
				// Waiting for our speech to end
				if ( GetExpresser()->CanSpeakAfterMyself() )
				{
					TaskComplete();
				}
			}
			else
			{
				// Waiting for the speech & the delay afterwards
				if ( !GetExpresser()->IsSpeaking() )
				{
					TaskComplete();
				}
			}
		}
		break;

	case TASK_SCRIPT_RUN_TO_TARGET:
	case TASK_SCRIPT_WALK_TO_TARGET:
	case TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET:
		StartScriptMoveToTargetTask( pTask->iTask );
		break;

	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_SPECIAL_ATTACK1:
	case TASK_SPECIAL_ATTACK2:
	case TASK_RELOAD:
		RunAttackTask( pTask->iTask );
		break;

	case TASK_SMALL_FLINCH:
	case TASK_BIG_FLINCH:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_WAIT_FOR_SCRIPT:
		{
			//
			// Waiting to play a script. If the script is ready, start playing the sequence.
			//
			if ( m_hCine && m_hCine->IsTimeToStart() )
			{
				TaskComplete();
				m_hCine->OnBeginSequence();

				// If we have an entry, we have to play it first
				if ( m_hCine->m_iszEntry != NULL_STRING )
				{
					m_hCine->StartSequence( (CAI_BaseNPC *)this, m_hCine->m_iszEntry, true );
				}
				else
				{
					m_hCine->StartSequence( (CAI_BaseNPC *)this, m_hCine->m_iszPlay, true );
				}

				// StartSequence() can call CineCleanup().  If that happened, just exit schedule
				if ( !m_hCine )
				{
					ClearSchedule( "Waiting for script, but lost script!" );
				}

				m_flPlaybackRate = 1.0;
				//DevMsg( 2, "Script %s has begun for %s\n", STRING( m_hCine->m_iszPlay ), GetClassname() );
			}
			else if (!m_hCine)
			{
				DevMsg( "Cine died!\n");
				TaskComplete();
			}
			else if ( IsRunningDynamicInteraction() )
			{
				// If we've lost our partner, abort
				if ( !m_hInteractionPartner )
				{
					CineCleanup();
				}
			}
			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			//
			// Playing a scripted sequence.
			//
			AutoMovement( );

			if ( IsSequenceFinished() )
			{
				// Check to see if we are done with the action sequence.
				if ( m_hCine->FinishedActionSequence( this ) )
				{
					// dvs: This is done in FixScriptNPCSchedule -- doing it here is too early because we still
					//      need to play our post-action idle sequence, which might also require FL_FLY.
					//
					// drop to ground if this guy is only marked "fly" because of the auto movement
					/*if ( !(m_hCine->m_savedFlags & FL_FLY) )
					{
						if ( ( GetFlags() & FL_FLY ) && !m_hCine->m_bIgnoreGravity )
						{
							RemoveFlag( FL_FLY );
						}
					}*/

					if (m_hCine)
					{
						m_hCine->SequenceDone( this );
					}

					TaskComplete();
				}
				else if ( m_hCine && m_hCine->m_bForceSynch )
				{
					m_hCine->SynchronizeSequence( this );
				}
			}
			break;
		}

	case TASK_PLAY_SCRIPT_POST_IDLE:
		{
			if ( !m_hCine )
			{
				DevMsg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			//
			// Playing a scripted post idle sequence. Quit early if another sequence has grabbed the NPC.
			//
			if ( IsSequenceFinished() || ( m_hCine->m_hNextCine != NULL ) )
			{
				m_hCine->PostIdleDone( this );
			}
			break;
		}

	case TASK_ENABLE_SCRIPT:
		{
			if ( !m_hCine )
			{
				DevMsg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			if (!m_hCine->IsWaitingForBegin())
			{
				m_hCine->DelayStart( false );
				TaskComplete();
			}
			break;
		}


	case TASK_PLAY_SCENE:
		{
			if (!IsInLockedScene())
			{
				ClearSchedule( "Playing a scene, but not in a scene!" );
			}
			if (GetNavigator()->GetGoalType() != GOALTYPE_NONE)
			{
				TaskComplete();
			}
			break;
		}
		
	case TASK_RUN_PATH_FOR_UNITS:
	case TASK_WALK_PATH_FOR_UNITS:
	{
		float distance;

		distance = (m_vecLastPosition - GetLocalOrigin()).Length2D();

		// Walk path until far enough away
		if ( distance > pTask->flTaskData || 
			 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
		{
			TaskComplete();
		}
		break;
	}
		
	case TASK_RUN_PATH_FLEE:
		{
			Vector vecDiff;
			vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

			if( vecDiff.Length() <= pTask->flTaskData )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_WALK_PATH_WITHIN_DIST:
	case TASK_RUN_PATH_WITHIN_DIST:
		{
			Vector vecDiff;

			vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

			if( vecDiff.Length() <= pTask->flTaskData )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_WALK_PATH_TIMED:
	case TASK_RUN_PATH_TIMED:
		{
			if ( IsWaitFinished() || 
				 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_WEAPON_PICKUP:
		{
			if ( IsActivityFinished() )
			{
				CBaseCombatWeapon	 *pWeapon = dynamic_cast<CBaseCombatWeapon *>(	(CBaseEntity *)m_hTargetEnt);
				CBaseCombatCharacter *pOwner  = pWeapon->GetOwner();
				if ( !pOwner )
				{
					TaskComplete();
				}
				else
				{
					TaskFail(FAIL_WEAPON_OWNED);
				}
			}
			break;
		}
		break;

	case TASK_ITEM_PICKUP:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
		break;

	case TASK_FALL_TO_GROUND:
		if ( GetFlags() & FL_ONGROUND )
		{
			TaskComplete();
		}
		else if( GetFlags() & FL_FLY )
		{
			// We're never going to fall if we're FL_FLY.
			RemoveFlag( FL_FLY );
		}
		else
		{
			if( IsWaitFinished() )
			{
				// After 4 seconds of trying to fall to ground, Assume that we're in a bad case where the NPC
				// isn't actually falling, and make an attempt to slam the ground entity to whatever's under the NPC.
				Vector maxs = WorldAlignMaxs() - Vector( .1, .1, .2 );
				Vector mins = WorldAlignMins() + Vector( .1, .1, 0 );
				Vector vecStart	= GetAbsOrigin() + Vector( 0, 0, .1 );
				Vector vecDown	= GetAbsOrigin();
				vecDown.z -= 0.2;

				trace_t trace;
				m_pMoveProbe->TraceHull( vecStart, vecDown, mins, maxs, MASK_NPCSOLID, &trace );

				if( trace.m_pEnt )
				{
					// Found something!
					SetGroundEntity( trace.m_pEnt );
					TaskComplete();
				}
				else
				{
					// Try again in a few seconds.
					SetWait(4);
				}
			}
		}
		break;

	case TASK_WANDER:
		break;

	case TASK_FREEZE:
		break;

	default:
		{
			DevMsg( "No RunTask entry for %s\n", TaskName( pTask->iTask ) );
			TaskComplete();
		}
		break;
	}
}

void CAI_BaseNPC::RunTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		m_MoveAndShootOverlay.RunShootWhileMove();
	}
}

void CAI_BaseNPC::EndTaskOverlay()
{
	m_MoveAndShootOverlay.EndShootWhileMove();
}

//=========================================================
// SetTurnActivity - measures the difference between the way
// the NPC is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CAI_BaseNPC::SetTurnActivity ( void )
{
	if ( IsCrouching() )
	{
		SetIdealActivity( ACT_IDLE ); // failure case
		return;
	}

	float flYD;
	flYD = GetMotor()->DeltaIdealYaw();

	// FIXME: unknown case, update yaw should catch these
	/*
	if (GetMotor()->AddTurnGesture( flYD ))
	{
		SetIdealActivity( ACT_IDLE );
		Remember( bits_MEMORY_TURNING );
		return;
	}
	*/

	if( flYD <= -80 && flYD >= -100 && SelectWeightedSequence( ACT_90_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree right.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_RIGHT );
		return;
	}
	if( flYD >= 80 && flYD <= 100 && SelectWeightedSequence( ACT_90_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree left.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_LEFT );
		return;
	}
	if( fabs( flYD ) >= 160 && SelectWeightedSequence ( ACT_180_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_180_LEFT );
		return;
	}

	if ( flYD <= -45 && SelectWeightedSequence ( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		SetIdealActivity( ACT_TURN_RIGHT );
		return;
	}
	if ( flYD >= 45 && SelectWeightedSequence ( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		SetIdealActivity( ACT_TURN_LEFT );
		return;
	}

	SetIdealActivity( ACT_IDLE ); // failure case

}


//-----------------------------------------------------------------------------
// Purpose: For a specific delta, add a turn gesture and set the yaw speed
// Input  : yaw delta
//-----------------------------------------------------------------------------


bool CAI_BaseNPC::UpdateTurnGesture( void )
{
	float flYD = GetMotor()->DeltaIdealYaw();
	return GetMotor()->AddTurnGesture( flYD );
}


//-----------------------------------------------------------------------------
// Purpose: For non-looping animations that may be replayed sequentially (like attacks)
//			Set the activity to ACT_RESET if this is a replay, otherwise just set ideal activity
// Input  : newIdealActivity - desired ideal activity
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ResetIdealActivity( Activity newIdealActivity )
{
	if ( m_Activity == newIdealActivity )
	{
		m_Activity = ACT_RESET;
	}

	SetIdealActivity( newIdealActivity );
}

			
void CAI_BaseNPC::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	if ( GetNavType() == NAV_FLY )
	{
		// UNDONE: Cache these per enemy instead?
		Vector offset = pEnemy->EyePosition() - pEnemy->GetAbsOrigin();
		chasePosition += offset;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the custom movement activity for the script that this NPC
//			is running.
// Output : Returns the activity, or ACT_INVALID is the sequence is unknown.
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetScriptCustomMoveActivity( void )
{
	Activity eActivity = ACT_WALK;

	if ( ( m_hCine != NULL ) && ( m_hCine->m_iszCustomMove != NULL_STRING ) )
	{
		// We have a valid script. Look up the custom movement activity.
		eActivity = ( Activity )LookupActivity( STRING( m_hCine->m_iszCustomMove ) );
		if ( eActivity == ACT_INVALID )
		{
			// Not an activity, at least make sure it's a valid sequence.
			if ( LookupSequence( STRING( m_hCine->m_iszCustomMove ) ) != ACT_INVALID )
			{
				eActivity = ACT_SCRIPT_CUSTOM_MOVE;
			}
			else
			{
				eActivity = ACT_WALK;
			}
		}
	}
	else if ( m_iszSceneCustomMoveSeq != NULL_STRING )
	{
		eActivity = ACT_SCRIPT_CUSTOM_MOVE;
	}

	return eActivity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetScriptCustomMoveSequence( void )
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	// If we have a scripted sequence entity, use it's custom move
	if ( m_hCine != NULL )
	{
		iSequence = LookupSequence( STRING( m_hCine->m_iszCustomMove ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			DevMsg( "SCRIPT_CUSTOM_MOVE: %s has no sequence:%s\n", GetClassname(), STRING(m_hCine->m_iszCustomMove) );
		}
	}
	else if ( m_iszSceneCustomMoveSeq != NULL_STRING )
	{
		// Otherwise, use the .vcd custom move
		iSequence = LookupSequence( STRING( m_iszSceneCustomMoveSeq ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			Warning( "SCRIPT_CUSTOM_MOVE: %s failed scripted custom move. Has no sequence called: %s\n", GetClassname(), STRING(m_iszSceneCustomMoveSeq) );
		}
	}

	// Failed? Use walk.
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
	{
		iSequence = SelectWeightedSequence( ACT_WALK );
	}

	return iSequence;
}

//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
const Task_t *CAI_BaseNPC::GetTask( void ) 
{
	int iScheduleIndex = GetScheduleCurTaskIndex();
	if ( !GetCurSchedule() ||  iScheduleIndex < 0 || iScheduleIndex >= GetCurSchedule()->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &GetCurSchedule()->GetTaskList()[ iScheduleIndex ];
}


//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsInterruptable()
{
	if ( GetState() == NPC_STATE_SCRIPT )
	{
		if ( m_hCine )
		{
			if (!m_hCine->CanInterrupt() )
				return false;

			// are the in an script FL_FLY state?
			if ((GetFlags() & FL_FLY ) && !(m_hCine->m_savedFlags & FL_FLY))
			{
				return false;
			}
		}
	}
	
	return IsAlive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectInteractionSchedule( void )
{
	SetTarget( m_hForcedInteractionPartner );

	// If we have an interaction, we're the initiator. Move to our interaction point.
	if ( m_iInteractionPlaying != NPCINT_NONE )
		return SCHED_INTERACTION_MOVE_TO_PARTNER;

	// Otherwise, turn towards our partner and wait for him to reach us.
	//m_iInteractionState = NPCINT_MOVING_TO_MARK;
	return SCHED_INTERACTION_WAIT_FOR_PARTNER;
}

//-----------------------------------------------------------------------------
// Idle schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectIdleSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition ( COND_HEAR_DANGER ) ||
		 HasCondition ( COND_HEAR_COMBAT ) ||
		 HasCondition ( COND_HEAR_WORLD  ) ||
		 HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
		 HasCondition ( COND_HEAR_PLAYER ) )
	{
		return SCHED_ALERT_FACE_BESTSOUND;
	}
	
	// no valid route!
	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
		return SCHED_IDLE_STAND;

	// valid route. Get moving
	return SCHED_IDLE_WALK;
}


//-----------------------------------------------------------------------------
// Alert schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectAlertSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	// Scan around for new enemies
	if ( HasCondition( COND_ENEMY_DEAD ) && SelectWeightedSequence( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
		return SCHED_ALERT_SCAN;

	if( IsPlayerAlly() && HasCondition(COND_HEAR_COMBAT) )
	{
		return SCHED_ALERT_REACT_TO_COMBAT_SOUND;
	}

	if ( HasCondition ( COND_HEAR_DANGER ) ||
			  HasCondition ( COND_HEAR_PLAYER ) ||
			  HasCondition ( COND_HEAR_WORLD  ) ||
			  HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
			  HasCondition ( COND_HEAR_COMBAT ) )
	{
		return SCHED_ALERT_FACE_BESTSOUND;
	}

	if ( gpGlobals->curtime - GetEnemies()->LastTimeSeen( AI_UNKNOWN_ENEMY ) < TIME_CARE_ABOUT_DAMAGE )
		return SCHED_ALERT_FACE;

	return SCHED_ALERT_STAND;
}


//-----------------------------------------------------------------------------
// Combat schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectCombatSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition(COND_NEW_ENEMY) && gpGlobals->curtime - GetEnemies()->FirstTimeSeen(GetEnemy()) < 2.0 )
	{
		return SCHED_WAKE_ANGRY;
	}
	
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		// clear the current (dead) enemy and try to find another.
		SetEnemy( NULL );
		 
		if ( ChooseEnemy() )
		{
			ClearCondition( COND_ENEMY_DEAD );
			return SelectSchedule();
		}

		SetState( NPC_STATE_ALERT );
		return SelectSchedule();
	}
	
	// If I'm scared of this enemy run away
	if ( IRelationType( GetEnemy() ) == D_FR )
	{
		if (HasCondition( COND_SEE_ENEMY )	|| 
			HasCondition( COND_LIGHT_DAMAGE )|| 
			HasCondition( COND_HEAVY_DAMAGE ))
		{
			FearSound();
			//ClearCommandGoal();
			return SCHED_RUN_FROM_ENEMY;
		}

		// If I've seen the enemy recently, cower. Ignore the time for unforgettable enemies.
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find( GetEnemy() );
		if ( (pMemory && pMemory->bUnforgettable) || (GetEnemyLastTimeSeen() > (gpGlobals->curtime - 5.0)) )
		{
			// If we're facing him, just look ready. Otherwise, face him.
			if ( FInAimCone( GetEnemy()->EyePosition() ) )
				return SCHED_COMBAT_STAND;

			return SCHED_FEAR_FACE;
		}
	}

	// Check if need to reload
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) || HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}

	// Can we see the enemy?
	if ( !HasCondition(COND_SEE_ENEMY) )
	{
		// enemy is unseen, but not occluded!
		// turn to face enemy
		if ( !HasCondition(COND_ENEMY_OCCLUDED) )
			return SCHED_COMBAT_FACE;

		// chase!
		if ( GetActiveWeapon() || (CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2)))
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		else if ( (CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2)))
			return SCHED_CHASE_ENEMY;
		else
			return SCHED_TAKE_COVER_FROM_ENEMY;
	}
	
	if ( HasCondition(COND_TOO_CLOSE_TO_ATTACK) ) 
		return SCHED_BACK_AWAY_FROM_ENEMY;
	
	if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ) || 
			HasCondition( COND_WEAPON_BLOCKED_BY_FRIEND ) || 
			HasCondition( COND_WEAPON_SIGHT_OCCLUDED ) )
	{
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}

	if ( GetShotRegulator()->IsInRestInterval() )
	{
		if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
			return SCHED_COMBAT_FACE;
	}

	// we can see the enemy
	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		if ( !UseAttackSquadSlots() || OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return SCHED_RANGE_ATTACK1;
		return SCHED_COMBAT_FACE;
	}

	if ( HasCondition(COND_CAN_RANGE_ATTACK2) )
		return SCHED_RANGE_ATTACK2;

	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	if ( HasCondition(COND_CAN_MELEE_ATTACK2) )
		return SCHED_MELEE_ATTACK2;

	if ( HasCondition(COND_NOT_FACING_ATTACK) )
		return SCHED_COMBAT_FACE;

	if ( !HasCondition(COND_CAN_RANGE_ATTACK1) && !HasCondition(COND_CAN_MELEE_ATTACK1) )
	{
		// if we can see enemy but can't use either attack type, we must need to get closer to enemy
		if ( GetActiveWeapon() )
			return SCHED_MOVE_TO_WEAPON_RANGE;

		// If we have an innate attack and we're too far (or occluded) then get line of sight
		if ( HasCondition( COND_TOO_FAR_TO_ATTACK ) && ( CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2)) )
			return SCHED_MOVE_TO_WEAPON_RANGE;

		// if we can see enemy but can't use either attack type, we must need to get closer to enemy
		if ( CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2) )
			return SCHED_CHASE_ENEMY;
		else
			return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	DevWarning( 2, "No suitable combat schedule!\n" );
	return SCHED_FAIL;
}


//-----------------------------------------------------------------------------
// Dead schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectDeadSchedule()
{
	if ( BecomeRagdollOnClient( vec3_origin ) )
	{
		CleanupOnDeath();
		return SCHED_DIE_RAGDOLL;
	}

	// Adrian - Alread dead (by animation event maybe?)
	// Is it safe to set it to SCHED_NONE?
	if ( m_lifeState == LIFE_DEAD )
		 return SCHED_NONE;

	CleanupOnDeath();
	return SCHED_DIE;
}


//-----------------------------------------------------------------------------
// Script schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectScriptSchedule()
{
	Assert( m_hCine != NULL );
	if ( m_hCine )
		return SCHED_AISCRIPT;

	DevWarning( 2, "Script failed for %s\n", GetClassname() );
	CineCleanup();
	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: Select a gesture to play in response to damage we've taken
// Output : int
//-----------------------------------------------------------------------------
void CAI_BaseNPC::PlayFlinchGesture()
{
	if ( !CanFlinch() )
		return;

	Activity iFlinchActivity = ACT_INVALID;

	float flNextFlinch = random->RandomFloat( 0.5f, 1.0f );

	// If I haven't flinched for a while, play the big flinch gesture
	if ( !HasMemory(bits_MEMORY_FLINCHED) )
	{
		iFlinchActivity = GetFlinchActivity( true, true );

		if ( HaveSequenceForActivity( iFlinchActivity ) )
		{
			RestartGesture( iFlinchActivity );
		}

		Remember(bits_MEMORY_FLINCHED);

	}
	else
	{
		iFlinchActivity = GetFlinchActivity( false, true );
		if ( HaveSequenceForActivity( iFlinchActivity ) )
		{
			RestartGesture( iFlinchActivity );
		}
	}

	if ( iFlinchActivity != ACT_INVALID )
	{
		//Get the duration of the flinch and delay the next one by that (plus a bit more)
		int iSequence = GetLayerSequence( FindGestureLayer( iFlinchActivity ) );

		if ( iSequence != ACT_INVALID )
		{
			flNextFlinch += SequenceDuration( iSequence );
		}

		m_flNextFlinchTime = gpGlobals->curtime + flNextFlinch;
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if we should flinch in response to damage we've taken
// Output : int
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectFlinchSchedule()
{
	if ( !HasCondition(COND_HEAVY_DAMAGE) )
		return SCHED_NONE;

	// If we've flinched recently, don't do it again. A gesture flinch will be played instead.
 	if ( HasMemory(bits_MEMORY_FLINCHED) )
		return SCHED_NONE;

	if ( !CanFlinch() )
		return SCHED_NONE;

	// Robin: This was in the original HL1 flinch code. Do we still want it?
	//if ( fabs( GetMotor()->DeltaIdealYaw() ) < (1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
	//	return SCHED_TAKE_COVER_FROM_ORIGIN;

	// Heavy damage. Break out of my current schedule and flinch.
	Activity iFlinchActivity = GetFlinchActivity( true, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_BIG_FLINCH;

	/*
	// Not used anymore, because gesture flinches are played instead for heavy damage
	// taken shortly after we've already flinched full.
	//
	iFlinchActivity = GetFlinchActivity( false, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_SMALL_FLINCH;
	*/

	return SCHED_NONE;
}
		
//-----------------------------------------------------------------------------
// Purpose: Decides which type of schedule best suits the NPC's current 
// state and conditions. Then calls NPC's member function to get a pointer 
// to a schedule of the proper type.
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectSchedule( void )
{
	if ( HasCondition( COND_FLOATING_OFF_GROUND ) )
	{
		SetGravity( 1.0 );
		SetGroundEntity( NULL );
		return SCHED_FALL_TO_GROUND;
	}

	switch( m_NPCState )
	{
	case NPC_STATE_NONE:
		DevWarning( 2, "NPC_STATE IS NONE!\n" );
		break;

	case NPC_STATE_PRONE:
		return SCHED_IDLE_STAND;

	case NPC_STATE_IDLE:
		AssertMsgOnce( GetEnemy() == NULL, "NPC has enemy but is not in combat state?" );
		return SelectIdleSchedule();

	case NPC_STATE_ALERT:
		AssertMsgOnce( GetEnemy() == NULL, "NPC has enemy but is not in combat state?" );
		return SelectAlertSchedule();

	case NPC_STATE_COMBAT:
		return SelectCombatSchedule();

	case NPC_STATE_DEAD:
		return SelectDeadSchedule();

	case NPC_STATE_SCRIPT:
		return SelectScriptSchedule();

	default:
		DevWarning( 2, "Invalid State for SelectSchedule!\n" );
		break;
	}

	return SCHED_FAIL;
}


//-----------------------------------------------------------------------------

int CAI_BaseNPC::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return ( m_failSchedule != SCHED_NONE ) ? m_failSchedule : SCHED_FAIL;
}
