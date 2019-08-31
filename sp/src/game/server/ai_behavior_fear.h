//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Deal intelligently with an enemy that we're afraid of
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//


#ifndef AI_BEHAVIOR_FEAR_H
#define AI_BEHAVIOR_FEAR_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"

#ifdef MAPBASE
#include "ai_goalentity.h"

//=========================================================
//=========================================================
class CAI_FearGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_FearGoal, CAI_GoalEntity );
public:
	CAI_FearGoal()
	{
	}

	void EnableGoal( CAI_BaseNPC *pAI );
	void DisableGoal( CAI_BaseNPC *pAI );

	// Inputs
	virtual void InputActivate( inputdata_t &inputdata );
	virtual void InputDeactivate( inputdata_t &inputdata );

	// Note that the outer is the caller in these outputs
	//COutputEvent	m_OnSeeFearEntity;
	COutputEvent	m_OnArriveAtFearNode;

	DECLARE_DATADESC();

protected:
	// Put something here
};
#endif

class CAI_FearBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_FearBehavior, CAI_SimpleBehavior );

public:
	CAI_FearBehavior();
	
	void Precache( void );
	virtual const char *GetName() {	return "Fear"; }

	virtual bool 	CanSelectSchedule();
	void GatherConditions();
	
	virtual void BeginScheduleSelection();
	virtual void EndScheduleSelection();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	//void BuildScheduleTestBits();
	//int TranslateSchedule( int scheduleType );
	//void OnStartSchedule( int scheduleType );

	//void InitializeBehavior();

	bool EnemyDislikesMe();
	
	void MarkAsUnsafe();
	bool IsInASafePlace();
	void SpoilSafePlace();
	void ReleaseAllHints();

	CAI_Hint *FindFearWithdrawalDest();
	void BuildScheduleTestBits();
	int TranslateSchedule( int scheduleType );

#ifdef MAPBASE
	virtual Activity	NPC_TranslateActivity( Activity activity );

	virtual void OnRestore();
	virtual void SetParameters( CAI_FearGoal *pGoal, string_t target );
	CHandle<CAI_FearGoal> m_hFearGoal;

	// Points to goal's fear target
	string_t m_iszFearTarget;
#endif

	
	enum
	{
		SCHED_FEAR_MOVE_TO_SAFE_PLACE = BaseClass::NEXT_SCHEDULE,		
		SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY,
		SCHED_FEAR_STAY_IN_SAFE_PLACE,
		NEXT_SCHEDULE,

		TASK_FEAR_GET_PATH_TO_SAFETY_HINT = BaseClass::NEXT_TASK,
		TASK_FEAR_WAIT_FOR_SAFETY,
		TASK_FEAR_IN_SAFE_PLACE,
		NEXT_TASK,

		COND_FEAR_ENEMY_CLOSE = BaseClass::NEXT_CONDITION,	// within 30 feet
		COND_FEAR_ENEMY_TOO_CLOSE,							// within 5 feet
		COND_FEAR_SEPARATED_FROM_PLAYER,
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:

private:
	virtual int		SelectSchedule();

	float			m_flTimeToSafety;
	float			m_flTimePlayerLastVisible;
	float			m_flDeferUntil;

	CAI_MoveMonitor		m_SafePlaceMoveMonitor;
	CHandle<CAI_Hint>	m_hSafePlaceHint;
	CHandle<CAI_Hint>	m_hMovingToHint;

	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_FEAR_H


