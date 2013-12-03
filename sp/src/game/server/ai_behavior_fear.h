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


