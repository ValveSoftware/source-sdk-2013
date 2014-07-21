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


#ifndef AI_BEHAVIOR_RAPPEL_H
#define AI_BEHAVIOR_RAPPEL_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"

class CBeam;

class CAI_RappelBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_RappelBehavior, CAI_SimpleBehavior );

public:
	CAI_RappelBehavior();
	
	void Precache( void );
	virtual const char *GetName() {	return "Rappel"; }

	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	virtual bool 	CanSelectSchedule();
	void GatherConditions();
	void CleanupOnDeath( CBaseEntity *pCulprit = NULL, bool bFireDeathOutput = true );
	
	//virtual void	BeginScheduleSelection();
	//virtual void	EndScheduleSelection();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	bool IsWaitingToRappel() { return m_bWaitingToRappel; }
	void BeginRappel();
	void SetDescentSpeed();

	void CreateZipline();
	void CutZipline();

	//void BuildScheduleTestBits();
	//int TranslateSchedule( int scheduleType );
	//void OnStartSchedule( int scheduleType );

	//void InitializeBehavior();
	
	enum
	{
		SCHED_RAPPEL_WAIT = BaseClass::NEXT_SCHEDULE,		
		SCHED_RAPPEL,
		SCHED_CLEAR_RAPPEL_POINT, // Get out of the way for the next guy
		NEXT_SCHEDULE,

		TASK_RAPPEL = BaseClass::NEXT_TASK,
		TASK_HIT_GROUND,
		NEXT_TASK,

		COND_BEGIN_RAPPEL = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:

private:
	virtual int		SelectSchedule();

	//---------------------------------
	bool	m_bWaitingToRappel;
	bool	m_bOnGround;
	CHandle<CBeam> m_hLine;
	Vector	m_vecRopeAnchor;
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_RAPPEL_H


