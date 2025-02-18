//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "convar.h"
#include "ai_basenpc.h"
#include "tier1/strtools.h"
#include "ai_activity.h"
#include "ai_schedule.h"
#include "ai_default.h"
#include "ai_hint.h"
#include "bitstring.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_SchedulesManager g_AI_SchedulesManager;

//-----------------------------------------------------------------------------
// Purpose:	Delete all the string registries
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_SchedulesManager::DestroyStringRegistries(void)
{
	CAI_BaseNPC::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::gm_SquadSlotNamespace.Clear();

	delete CAI_BaseNPC::m_pActivitySR;
	CAI_BaseNPC::m_pActivitySR = NULL;
	CAI_BaseNPC::m_iNumActivities = 0;
}

void CAI_SchedulesManager::CreateStringRegistries( void )
{
	CAI_BaseNPC::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::gm_SquadSlotNamespace.Clear();

	CAI_BaseNPC::m_pActivitySR = new CStringRegistry();
	CAI_BaseNPC::m_pEventSR = new CStringRegistry();
}

//-----------------------------------------------------------------------------
// Purpose: Load all the schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitSchedulingTables()
{
	CAI_BaseNPC::gm_ClassScheduleIdSpace.Init( "CAI_BaseNPC", CAI_BaseNPC::GetSchedulingSymbols() );
	CAI_BaseNPC::InitDefaultScheduleSR();
	CAI_BaseNPC::InitDefaultConditionSR();
	CAI_BaseNPC::InitDefaultTaskSR();
	CAI_BaseNPC::InitDefaultActivitySR();
	CAI_BaseNPC::InitDefaultSquadSlotSR();
}

bool CAI_SchedulesManager::LoadAllSchedules(void)
{
	// If I haven't loaded schedules yet
	if (!CAI_SchedulesManager::allSchedules)
	{
		// Init defaults
		CAI_BaseNPC::InitSchedulingTables();
		if (!CAI_BaseNPC::LoadDefaultSchedules())
		{
			CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
			DevMsg("ERROR:  Mistake in default schedule definitions, AI Disabled.\n");
		}

// UNDONE: enable this after the schedules are all loaded (right now some load in monster spawns)
#if 0
		// If not in developer mode, free the string memory.  Otherwise
		// keep it around for debugging information
		if (!g_pDeveloper->GetInt())
		{
			ClearStringRegistries();
		}
#endif

	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Creates and returns schedule of the given name
//			This should eventually be replaced when we convert to
//			non-hard coded schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::CreateSchedule(char *name, int schedule_id)
{
	// Allocate schedule
	CAI_Schedule *pSched = new CAI_Schedule(name,schedule_id,CAI_SchedulesManager::allSchedules);
	CAI_SchedulesManager::allSchedules = pSched;

	// Return schedule
	return pSched;
}

//-----------------------------------------------------------------------------
// Purpose: Given text name of a NPC state returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetStateID(const char *state_name)
{
	if		(!stricmp(state_name,"NONE"))		{	return NPC_STATE_NONE;		}
	else if (!stricmp(state_name,"IDLE"))		{	return NPC_STATE_IDLE;		}
	else if (!stricmp(state_name,"COMBAT"))		{	return NPC_STATE_COMBAT;	}
	else if (!stricmp(state_name,"PRONE"))		{	return NPC_STATE_PRONE;		}
	else if (!stricmp(state_name,"ALERT"))		{	return NPC_STATE_ALERT;		}
	else if (!stricmp(state_name,"SCRIPT"))		{	return NPC_STATE_SCRIPT;	}
	else if (!stricmp(state_name,"PLAYDEAD"))	{	return NPC_STATE_PLAYDEAD;	}
	else if (!stricmp(state_name,"DEAD"))		{	return NPC_STATE_DEAD;		}
	else											return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Given text name of a memory bit returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetMemoryID(const char *state_name)
{
	if		(!stricmp(state_name,"PROVOKED"))		{	return bits_MEMORY_PROVOKED;		}
	else if (!stricmp(state_name,"INCOVER"))		{	return bits_MEMORY_INCOVER;			}
	else if (!stricmp(state_name,"SUSPICIOUS"))		{	return bits_MEMORY_SUSPICIOUS;		}
	else if (!stricmp(state_name,"PATH_FAILED"))	{	return bits_MEMORY_PATH_FAILED;		}
	else if (!stricmp(state_name,"FLINCHED"))		{	return bits_MEMORY_FLINCHED;		}
	else if (!stricmp(state_name,"TOURGUIDE"))		{	return bits_MEMORY_TOURGUIDE;		}
	else if (!stricmp(state_name,"LOCKED_HINT"))	{	return bits_MEMORY_LOCKED_HINT;		}
	else if (!stricmp(state_name,"TURNING"))		{	return bits_MEMORY_TURNING;			}
	else if (!stricmp(state_name,"TURNHACK"))		{	return bits_MEMORY_TURNHACK;		}
	else if (!stricmp(state_name,"CUSTOM4"))		{	return bits_MEMORY_CUSTOM4;			}
	else if (!stricmp(state_name,"CUSTOM3"))		{	return bits_MEMORY_CUSTOM3;			}
	else if (!stricmp(state_name,"CUSTOM2"))		{	return bits_MEMORY_CUSTOM2;			}
	else if (!stricmp(state_name,"CUSTOM1"))		{	return bits_MEMORY_CUSTOM1;			}
	else												return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetPathID( const char *token )
{
	if		( !stricmp( token, "TRAVEL" ) )	{	return PATH_TRAVEL;		}
	else if ( !stricmp( token, "LOS" ) )		{	return PATH_LOS;		}
//	else if ( !stricmp( token, "FLANK" ) )		{	return PATH_FLANK;		}
//	else if ( !stricmp( token, "FLANK_LOS" ) )	{	return PATH_FLANK_LOS;	}
	else if ( !stricmp( token, "COVER" ) )		{	return PATH_COVER;		}
//	else if ( !stricmp( token, "COVER_LOS" ) )	{	return PATH_COVER_LOS;	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetGoalID( const char *token )
{
	if		( !stricmp( token, "ENEMY" ) )			{	return GOAL_ENEMY;			}
	else if ( !stricmp( token, "ENEMY_LKP" ) )		{	return GOAL_ENEMY_LKP;		}
	else if ( !stricmp( token, "TARGET" ) )			{	return GOAL_TARGET;			}
	else if ( !stricmp( token, "SAVED_POSITION" ) )	{	return GOAL_SAVED_POSITION;	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Read data on schedules
//			As I'm parsing a human generated file, give a lot of error output
// Output:  true  - if data successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------

bool CAI_SchedulesManager::LoadSchedulesFromBuffer( const char *prefix, const char *pStartFile, CAI_ClassScheduleIdSpace *pIdSpace )
{
	char token[1024];
	char save_token[1024];
	const char *pfile = engine->ParseFile(pStartFile, token, sizeof( token ) );

	while (!stricmp("Schedule",token))
	{
		pfile = engine->ParseFile(pfile, token, sizeof( token ) );

		// -----------------------------
		// Check for duplicate schedule
		// -----------------------------
		if (GetScheduleByName(token))
		{
			DevMsg("ERROR: file contains a schedule (%s) that has already been defined!\n",token);
			DevMsg("       Aborting schedule load.\n");
			Assert(0);
			return false;
		}

		int scheduleID = CAI_BaseNPC::GetScheduleID(token);
		if (scheduleID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s): Unknown schedule type (%s)\n", prefix, token);
			// FIXME: .sch's not being in code/perforce makes it hard to share branches between developers
			// for now, just stop processing this entities schedules if one is found that isn't in the schedule registry
			break;
			// return false;
		}

		CAI_Schedule *new_schedule = CreateSchedule(token,scheduleID);

		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		if (stricmp(token,"Tasks"))
		{
			DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting 'Tasks' keyword.\n",prefix,new_schedule->GetName());
			Assert(0);
			return false;
		}

		// ==========================
		// Now read in the tasks
		// ==========================
		// Store in temp array until number of tasks is known
		Task_t tempTask[50];
		int	   taskNum = 0;

		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		while ((token[0]!='\0') && (stricmp("Interrupts",token)))
		{
			// Convert generic ID to sub-class specific enum
			int taskID = CAI_BaseNPC::GetTaskID(token);
			tempTask[taskNum].iTask = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskID) : AI_RemapFromGlobal( taskID );
			
			// If not a valid condition, send a warning message
			if (tempTask[taskNum].iTask == -1)
			{
				DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown task %s!\n", prefix,new_schedule->GetName(), token);
				Assert(0);
				return false;
			}

			Assert( AI_IdIsLocal( tempTask[taskNum].iTask ) );

			// Read in the task argument
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );

			if (!stricmp("Activity",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_BaseNPC::GetActivityID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown activity %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Task",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );

				// Convert generic ID to sub-class specific enum
				int taskID = CAI_BaseNPC::GetTaskID(token);
				tempTask[taskNum].flTaskData = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskID) : AI_RemapFromGlobal( taskID );

				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown task %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Schedule",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the schedule and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );

				// Convert generic ID to sub-class specific enum
				int schedID = CAI_BaseNPC::GetScheduleID(token);
				tempTask[taskNum].flTaskData = (pIdSpace) ? pIdSpace->ScheduleGlobalToLocal(schedID) : AI_RemapFromGlobal( schedID );

				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("State",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'STATE.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetStateID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Memory",token))
			{

				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'STATE.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetMemoryID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Path",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'PATH.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetPathID( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown path type %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Goal",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'GOAL.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetGoalID( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown goal type  %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if ( !stricmp( "HintFlags",token ) )
			{
				// Skip the ":", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'HINTFLAG'\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the flags and make sure they are valid
				pfile = engine->ParseFile( pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_HintManager::GetFlags( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown hint flag type  %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Interrupts",token) || !strnicmp("TASK_",token,5) )
			{
				// a parse error.  Interrupts is the next section, TASK_ is probably the next task, missing task argument?
				Warning( "ERROR: LoadSchd (%s): (%s) Bad syntax at task #%d (wasn't expecting %s)\n", prefix, new_schedule->GetName(), taskNum, token);
				Assert(0);
				return false;
			}
			else
			{
				tempTask[taskNum].flTaskData = atof(token);
			}
			taskNum++;

			// Read the next token
			Q_strncpy(save_token,token,sizeof(save_token));
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );

			// Check for malformed task argument type
			if (!stricmp(token,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s): Schedule (%s),\n        Task (%d), has a malformed AI Task Argument = (%s)\n",
						prefix,new_schedule->GetName(),taskID,save_token);
				Assert(0);
				return false;
			}
		}

		// Now copy the tasks into the new schedule
		new_schedule->m_iNumTasks = taskNum;
		new_schedule->m_pTaskList = new Task_t[taskNum];
		for (int i=0;i<taskNum;i++)
		{
			new_schedule->m_pTaskList[i].iTask		= tempTask[i].iTask;
			new_schedule->m_pTaskList[i].flTaskData = tempTask[i].flTaskData;

			Assert( AI_IdIsLocal( new_schedule->m_pTaskList[i].iTask ) );
		}

		// ==========================
		// Now read in the interrupts
		// ==========================
		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		while ((token[0]!='\0') && (stricmp("Schedule",token)))
		{
			// Convert generic ID to sub-class specific enum
			int condID = CAI_BaseNPC::GetConditionID(token);

			// If not a valid condition, send a warning message
			if (condID == -1)
			{
				DevMsg( "ERROR: LoadSchd (%s): Schedule (%s), Unknown condition %s!\n", prefix,new_schedule->GetName(),token);
				Assert(0);
			}

			// Otherwise, add to this schedules list of conditions
			else
			{
				int interrupt = AI_RemapFromGlobal(condID);
				Assert( AI_IdIsGlobal( condID ) && interrupt >= 0 && interrupt < MAX_CONDITIONS );
				new_schedule->m_InterruptMask.Set(interrupt);
			}

			// Read the next token
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		}
	}
	return true;
}

bool CAI_SchedulesManager::LoadSchedules( const char *prefix, CAI_ClassScheduleIdSpace *pIdSpace  )
{
	char sz[128];

	// Open the weapon's data file and read the weaponry details
	Q_snprintf(sz,sizeof(sz), "scripts/%s.sch",prefix);
	char *pfile = (char*)UTIL_LoadFileForMe(sz, NULL);

	if (!pfile)
	{
		DevMsg( "Unable to open AI Schedule data file for: %s\n", sz);
		return false;
	}
	if (!LoadSchedulesFromBuffer( prefix, pfile, pIdSpace))
	{
		DevMsg( "       Schedule file: %s\n", sz );
		UTIL_FreeFile( (byte*)pfile );
		return false;
	}
	UTIL_FreeFile( (byte*)pfile );
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Given a schedule ID, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleFromID( int schedID )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (schedule->m_iScheduleID == schedID)
			return schedule;
	}

	DevMsg( "Couldn't find schedule (%s)\n", CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol(schedID) );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given a schedule name, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleByName( const char *name )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (FStrEq(schedule->GetName(),name))
			return schedule;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Delete all the schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_SchedulesManager::DeleteAllSchedules(void)
{
	m_CurLoadSig++;

	if ( m_CurLoadSig < 0 )
		m_CurLoadSig = 0;

	CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules;
	CAI_Schedule *next;

	while (schedule)
	{
		next = schedule->nextSchedule;
		delete schedule;
		schedule = next;
	}
	CAI_SchedulesManager::allSchedules = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------

CAI_Schedule::CAI_Schedule(char *name, int schedule_id, CAI_Schedule *pNext)
{
	m_iScheduleID = schedule_id;

	int len = strlen(name);
	m_pName = new char[len+1];
	Q_strncpy(m_pName,name,len+1);

	m_pTaskList = NULL;
	m_iNumTasks = 0;

	// ---------------------------------
	//  Add to linked list of schedules
	// ---------------------------------
	nextSchedule = pNext;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAI_Schedule::~CAI_Schedule( void )
{
	delete[] m_pName;
	delete[] m_pTaskList;
}
