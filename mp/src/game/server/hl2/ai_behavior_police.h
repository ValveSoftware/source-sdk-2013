//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_BEHAVIOR_POLICE_H
#define AI_BEHAVIOR_POLICE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"
#include "ai_goal_police.h"
#include "ai_sentence.h"

#define	PATROL_RADIUS_RATIO		2.0f
#define	POLICE_MAX_WARNINGS		4

class CAI_PolicingBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_PolicingBehavior, CAI_SimpleBehavior );
public:
	DECLARE_DATADESC();
	CAI_PolicingBehavior();

	enum
	{
		// Schedules
		SCHED_POLICE_RETURN_FROM_HARASS = BaseClass::NEXT_SCHEDULE,
		SCHED_POLICE_WARN_TARGET,
		SCHED_POLICE_HARASS_TARGET,
		SCHED_POLICE_SUPPRESS_TARGET,
		SCHED_POLICE_FACE_ALONG_GOAL,
		SCHED_POLICE_TRACK_TARGET,
		NEXT_SCHEDULE,
		
		// Tasks
		TASK_POLICE_GET_PATH_TO_HARASS_GOAL = BaseClass::NEXT_TASK,
		TASK_POLICE_GET_PATH_TO_POLICE_GOAL,
		TASK_POLICE_FACE_ALONG_GOAL,
		TASK_POLICE_ANNOUNCE_HARASS,
		NEXT_TASK,
		
		// Conditions
		COND_POLICE_TARGET_TOO_CLOSE_HARASS = BaseClass::NEXT_CONDITION,
		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS,
		NEXT_CONDITION,
	};

	virtual const char *GetName() {	return "Policing"; }

	void	Enable( CAI_PoliceGoal *pGoal );
	void	Disable( void );
	bool	CanSelectSchedule( void );
	void	BuildScheduleTestBits( void );

	bool	IsEnabled( void ) { return m_bEnabled; }
	bool	TargetIsHostile( void );

	bool	ShouldKnockOutTarget( CBaseEntity *pTarget );
	void	KnockOutTarget( CBaseEntity *pTarget );

	int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	
	CBaseEntity *GetGoalTarget( void );

private:
	
	void	HostSpeakSentence( const char *pSentence, SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria );

	int		TranslateSchedule( int scheduleType );
	
	int		SelectSchedule( void );
	int		SelectSuppressSchedule( void );
	int		SelectHarassSchedule( void );
	
	Activity	NPC_TranslateActivity( Activity newActivity );
	void		GatherConditions( void );
	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	
	void	AnnouncePolicing( void );
	void	HostSetBatonState( bool state );
	bool	HostBatonIsOn( void );

	void	SetTargetHostileDuration( float time );
	bool	MaintainGoalPosition( void );

protected:

	bool	m_bEnabled;
	bool	m_bStartPolicing;
	float	m_flNextHarassTime;
	float	m_flAggressiveTime;
	int		m_nNumWarnings;
	bool	m_bTargetIsHostile;
	float	m_flTargetHostileTime;
	
	CHandle<CAI_PoliceGoal> m_hPoliceGoal;

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};

#endif // AI_BEHAVIOR_POLICE_H
