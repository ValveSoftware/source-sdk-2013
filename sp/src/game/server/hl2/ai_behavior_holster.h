//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Deal with weapon being out
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//


#ifndef AI_BEHAVIOR_HOLSTER_H
#define AI_BEHAVIOR_HOLSTER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"

class CAI_HolsterBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_HolsterBehavior, CAI_SimpleBehavior );

public:
	CAI_HolsterBehavior();
	
	virtual const char *GetName() {	return "Holster"; }

	virtual bool 	CanSelectSchedule();
	//virtual void	BeginScheduleSelection();
	//virtual void	EndScheduleSelection();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	//void BuildScheduleTestBits();
	//int TranslateSchedule( int scheduleType );
	//void OnStartSchedule( int scheduleType );

	//void InitializeBehavior();
	
	enum
	{
		SCHED_HOLSTER_WEAPON = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_DRAW_WEAPON,
		NEXT_SCHEDULE,

		TASK_HOLSTER_WEAPON = BaseClass::NEXT_TASK,
		TASK_DRAW_WEAPON,
		NEXT_TASK,

/*
		COND_PUT_CONDITIONS_HERE = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
*/
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:

private:
	virtual int		SelectSchedule();

	bool			m_bWeaponOut;

	//---------------------------------
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_HOLSTER_H


