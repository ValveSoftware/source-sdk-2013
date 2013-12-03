//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Operate consoles/machinery in the world.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//


#ifndef AI_BEHAVIOR_OPERATOR_H
#define AI_BEHAVIOR_OPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"
#include "ai_goalentity.h"

enum 
{
	OPERATOR_STATE_NOT_READY = 0,
	OPERATOR_STATE_READY,
	OPERATOR_STATE_FINISHED,
};

enum
{
	OPERATOR_MOVETO_RESERVED = 0,
	OPERATOR_MOVETO_WALK,
	OPERATOR_MOVETO_RUN,
};

//=========================================================
//=========================================================
class CAI_OperatorGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_OperatorGoal, CAI_GoalEntity );
public:
	CAI_OperatorGoal()
	{
	}

	void EnableGoal( CAI_BaseNPC *pAI );

	int GetState() { return m_iState; }
	int GetMoveTo() { return m_iMoveTo; }

	// Inputs
	virtual void InputActivate( inputdata_t &inputdata );
	virtual void InputDeactivate( inputdata_t &inputdata );

	void	InputSetStateReady( inputdata_t &inputdata );
	void	InputSetStateFinished( inputdata_t &inputdata );

	COutputEvent	m_OnBeginApproach;
	COutputEvent	m_OnMakeReady;
	COutputEvent	m_OnBeginOperating;
	COutputEvent	m_OnFinished;

	DECLARE_DATADESC();

protected:
	int			m_iState;
	int			m_iMoveTo;
	string_t	m_iszContextTarget;
};

//=========================================================
//=========================================================
class CAI_OperatorBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_OperatorBehavior, CAI_SimpleBehavior );

public:
	CAI_OperatorBehavior();

	virtual const char *GetName() {	return "Operator"; }

	virtual void SetParameters( CAI_OperatorGoal *pGoal, CBaseEntity *pPositionEnt, CBaseEntity *pContextTarget );

	virtual bool 	CanSelectSchedule();
	//virtual void	BeginScheduleSelection();
	//virtual void	EndScheduleSelection();

	bool CanSeePositionEntity();
	bool IsAtPositionEntity();

	void GatherConditionsNotActive();
	void GatherConditions( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	CAI_OperatorGoal *GetGoalEntity();

	bool IsGoalReady();

	//void BuildScheduleTestBits();
	//int TranslateSchedule( int scheduleType );
	//void OnStartSchedule( int scheduleType );

	//void InitializeBehavior();

	enum
	{
		SCHED_OPERATOR_APPROACH_POSITION = BaseClass::NEXT_SCHEDULE,
		SCHED_OPERATOR_MAKE_READY,
		SCHED_OPERATOR_OPERATE,
		SCHED_OPERATOR_WAIT_FOR_HOLSTER,
		NEXT_SCHEDULE,

		TASK_OPERATOR_GET_PATH_TO_POSITION = BaseClass::NEXT_TASK,
		TASK_OPERATOR_START_PATH,
		TASK_OPERATOR_OPERATE,
		NEXT_TASK,

		COND_OPERATOR_LOST_SIGHT_OF_POSITION = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:
	EHANDLE	m_hGoalEntity;
	EHANDLE m_hPositionEnt;
	EHANDLE m_hContextTarget;
	CRandStopwatch  m_WatchSeeEntity;

private:
	virtual int		SelectSchedule();

	//---------------------------------

	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_OPERATOR_H


