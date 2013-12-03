//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		A schedule
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "bitstring.h"

#ifndef AI_SCHEDULE_H
#define AI_SCHEDULE_H

#pragma once

class	CStringRegistry;
class   CAI_ClassScheduleIdSpace;
class	CAI_BaseNPC;

struct	Task_t;

#ifndef MAX_CONDITIONS
#define	MAX_CONDITIONS 32*8
#endif
typedef CBitVec<MAX_CONDITIONS> CAI_ScheduleBits;

//==================================================
// goalType_t
//==================================================

enum goalType_t
{
	GOAL_NONE = -1,
	GOAL_ENEMY,				//Our current enemy's position
	GOAL_TARGET,			//Our current target's position
	GOAL_ENEMY_LKP,			//Our current enemy's last known position
	GOAL_SAVED_POSITION,	//Our saved position
};

//==================================================
// pathType_t
//==================================================

enum pathType_t
{
	PATH_NONE = -1,
	PATH_TRAVEL,			//Path that will take us to the goal
	PATH_LOS,				//Path that gives us line of sight to our goal
	//PATH_FLANK,				//Path that will take us to a flanking position of our goal
	//PATH_FLANK_LOS,			//Path that will take us to within line of sight to the flanking position of our goal
	PATH_COVER,				//Path that will give us cover from our goal
	//PATH_COVER_LOS,			//Path that will give us line of sight to cover from our goal
};

//=============================================================================
// >> CAI_Schedule
//=============================================================================

class CAI_Schedule;

class CAI_SchedulesManager
{
public:
	CAI_SchedulesManager()
	{
		allSchedules = NULL;
		m_CurLoadSig = 0;		// Note when schedules reset
	}

	int				GetScheduleLoadSignature() { return m_CurLoadSig; }
	CAI_Schedule*	GetScheduleFromID( int schedID );	// Function to return schedule from linked list 
	CAI_Schedule*	GetScheduleByName( const char *name );

	bool LoadAllSchedules(void);

	bool LoadSchedules( const char* prefix, CAI_ClassScheduleIdSpace *pIdSpace  );
	bool LoadSchedulesFromBuffer( const char *prefix, const char *pfile, CAI_ClassScheduleIdSpace *pIdSpace );

private:
	friend class CAI_SystemHook;
	
	int				m_CurLoadSig;					// Note when schedules reset
	CAI_Schedule*	allSchedules;						// A linked list of all schedules

	CAI_Schedule *	CreateSchedule(char *name, int schedule_id);

	void CreateStringRegistries( void );
	void DestroyStringRegistries( void );
	void DeleteAllSchedules(void);

	//static bool	LoadSchedules( char* prefix,	int taskIDOffset,	int taskENOffset,
	//											int schedIDOffset,  int schedENOffset,
	//											int condIDOffset,	int condENOffset);

	// parsing helpers
	int	GetStateID(const char *state_name);
	int	GetMemoryID(const char *memory_name);
	int GetPathID( const char *token );
	int GetGoalID( const char *token );

};

extern CAI_SchedulesManager g_AI_SchedulesManager;

class CAI_Schedule
{
// ---------
//	Static
// ---------
// ---------
public:
	int GetId() const
	{
		return m_iScheduleID;
	}
	
	const Task_t *GetTaskList() const
	{
		return m_pTaskList;
	}
	
	int NumTasks() const
	{
		return m_iNumTasks;
	}
	
	void GetInterruptMask( CAI_ScheduleBits *pBits ) const
	{
		m_InterruptMask.CopyTo( pBits );
	}

	bool HasInterrupt( int condition ) const
	{
		return m_InterruptMask.IsBitSet( condition );
	}
	
	const char *GetName() const
	{
		return m_pName;
	}
	
private:
	friend class CAI_SchedulesManager;

	int			m_iScheduleID;				// The id number of this schedule

	Task_t		*m_pTaskList;
	int			m_iNumTasks;	 

	CAI_ScheduleBits m_InterruptMask;			// a bit mask of conditions that can interrupt this schedule 
	char		*m_pName;

	CAI_Schedule *nextSchedule;				// The next schedule in the list of schedules

	CAI_Schedule(char *name,int schedule_id, CAI_Schedule *pNext);
	~CAI_Schedule( void );
};

//-----------------------------------------------------------------------------
//
// In-memory schedules
//

#define AI_DEFINE_SCHEDULE( name, text ) \
	const char * g_psz##name = \
		"\n	Schedule" \
		"\n		" #name \
		text \
		"\n"


#define AI_LOAD_SCHEDULE( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if ( classname::gm_SchedLoadStatus.fValid ) \
		{ \
			classname::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( #classname,(char *)g_psz##name,&classname::gm_ClassScheduleIdSpace ); \
		} \
	} while (false)


// For loading default schedules in memory  (see ai_default.cpp)
#define AI_LOAD_DEF_SCHEDULE( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if (!g_AI_SchedulesManager.LoadSchedulesFromBuffer( #classname,(char *)g_psz##name,&classname::gm_ClassScheduleIdSpace )) \
			return false; \
	} while (false)


//-----------------------------------------------------------------------------

#endif // AI_SCHEDULE_H
