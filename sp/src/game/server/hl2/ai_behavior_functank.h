//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_BEHAVIOR_FUNCTANK_H
#define AI_BEHAVIOR_FUNCTANK_H
#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_behavior.h"
#include "func_tank.h"

#define AI_FUNCTANK_BEHAVIOR_BUSYTIME		10.0f

enum
{
	FUNCTANK_SENTENCE_MOVE_TO_MOUNT = SENTENCE_BASE_BEHAVIOR_INDEX,
	FUNCTANK_SENTENCE_JUST_MOUNTED,
	FUNCTANK_SENTENCE_SCAN_FOR_ENEMIES,
	FUNCTANK_SENTENCE_DISMOUNTING,
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAI_FuncTankBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_FuncTankBehavior, CAI_SimpleBehavior );
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	DECLARE_DATADESC();
	
public:
	// Contructor/Deconstructor
	CAI_FuncTankBehavior();
	~CAI_FuncTankBehavior();

	void UpdateOnRemove();
	
	// Identifier
	const char *GetName() {	return "FuncTank"; }
	
	// Schedule
	bool 		CanSelectSchedule();
	void		BeginScheduleSelection();
	void		EndScheduleSelection();
	void		PrescheduleThink();
#ifdef MAPBASE
	bool		IsInterruptable( void );

	bool		CanManTank( CFuncTank *pTank, bool bForced );
#endif

	void		ModifyOrAppendCriteria( AI_CriteriaSet &set );

	Activity	NPC_TranslateActivity( Activity activity );

	// Conditions:
	virtual void GatherConditions();
	
	enum
	{
		SCHED_MOVE_TO_FUNCTANK = BaseClass::NEXT_SCHEDULE,
		SCHED_FIRE_FUNCTANK,
		SCHED_SCAN_WITH_FUNCTANK,
		SCHED_FAIL_MOVE_TO_FUNCTANK,
#ifdef MAPBASE
		SCHED_FORCE_MOUNT_FUNCTANK,
#endif
	};
	
	// Tasks
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	
	enum
	{
		TASK_GET_PATH_TO_FUNCTANK = BaseClass::NEXT_TASK,
		TASK_FACE_FUNCTANK,
		TASK_HOLSTER_WEAPON,
		TASK_FIRE_FUNCTANK,
		TASK_SCAN_LEFT_FUNCTANK,
		TASK_SCAN_RIGHT_FUNCTANK,
		TASK_FORGET_ABOUT_FUNCTANK,
		TASK_FUNCTANK_ANNOUNCE_SCAN,
	};

	enum
	{
		COND_FUNCTANK_DISMOUNT = BaseClass::NEXT_CONDITION,
#ifdef MAPBASE
		COND_FUNCTANK_FORCED,
#endif
		NEXT_CONDITION,
	};	

	// Combat.
	CBaseEntity *BestEnemy( void );
	void Event_Killed( const CTakeDamageInfo &info );

	bool HasFuncTank( void )							{ return ( m_hFuncTank != NULL ); }
	void SetFuncTank( CHandle<CFuncTank> hFuncTank );
	CFuncTank *GetFuncTank() { return m_hFuncTank; }
	void AimGun( void );

	void Dismount( void );

	int	 OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// Time.
	void SetBusy( float flTime )		{ m_flBusyTime = flTime; }
	bool IsBusy( void )					{ return ( gpGlobals->curtime < m_flBusyTime ); }

	bool IsMounted( void )				{ return m_bMounted; }

#ifdef MAPBASE
	void SetMounted( bool bMounted )	{ m_bMounted = bMounted; }

	bool CanUnholsterWeapon( void ) { return !IsMounted(); }
#endif

private:
	
	// Schedule 
	int			SelectSchedule();
	
private:

	CHandle<CFuncTank>	m_hFuncTank;
	bool				m_bMounted;
	float				m_flBusyTime;
	bool				m_bSpottedPlayerOutOfCover;
};

#endif // AI_BEHAVIOR_FUNCTANK_H