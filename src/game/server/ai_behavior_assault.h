//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_BEHAVIOR_ASSAULT_H
#define AI_BEHAVIOR_ASSAULT_H
#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_behavior.h"
#include "ai_goalentity.h"
#include "ai_moveshoot.h"
#include "ai_utils.h"

#define CUE_POINT_TOLERANCE (3.0*12.0)

enum RallySelectMethod_t
{
	RALLY_POINT_SELECT_DEFAULT = 0,
	RALLY_POINT_SELECT_RANDOM,
};

enum AssaultCue_t
{
	CUE_NO_ASSAULT = 0,	// used to indicate that no assault is being conducted presently

	CUE_ENTITY_INPUT = 1,
	CUE_PLAYER_GUNFIRE,
	CUE_DONT_WAIT,
	CUE_COMMANDER,
	CUE_NONE,
};

enum
{
	ASSAULT_SENTENCE_HIT_RALLY_POINT = SENTENCE_BASE_BEHAVIOR_INDEX,
	ASSAULT_SENTENCE_HIT_ASSAULT_POINT,
	ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_RALLY,
	ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT,
	ASSAULT_SENTENCE_COVER_NO_AMMO,
	ASSAULT_SENTENCE_UNDER_ATTACK,
};

// Allow diversion from the assault up to this amount of time after last having an enemy
#define ASSAULT_DIVERSION_TIME		4

#define SF_ASSAULTPOINT_CLEARONARRIVAL	0x00000001

//=============================================================================
//=============================================================================
class CRallyPoint : public CPointEntity 
{
	DECLARE_CLASS( CRallyPoint, CPointEntity );

public:
	CRallyPoint()
	{
		m_hLockedBy.Set(NULL);
		m_sExclusivity = RALLY_EXCLUSIVE_NOT_EVALUATED;
	}

	bool Lock( CBaseEntity *pLocker )
	{
		if( IsLocked() )
		{
			// Already locked.
			return false;
		}

		m_hLockedBy.Set( pLocker );
		return true;
	}

	bool Unlock( CBaseEntity *pUnlocker )
	{
		if( IsLocked() )
		{
			if( m_hLockedBy.Get() != pUnlocker )
			{
				// Refuse! Only the locker may unlock.
				return false;
			}
		}

		m_hLockedBy.Set( NULL );
		return true;
	}

	bool IsLocked( void ) { return (m_hLockedBy.Get() != NULL); }

	int DrawDebugTextOverlays();
	bool IsExclusive();

	enum
	{
		RALLY_EXCLUSIVE_NOT_EVALUATED = -1,
		RALLY_EXCLUSIVE_NO,
		RALLY_EXCLUSIVE_YES,
	};

	string_t	m_AssaultPointName;
	string_t	m_RallySequenceName;
	float		m_flAssaultDelay;
	int			m_iPriority;
	int			m_iStrictness;
	bool		m_bForceCrouch;
	bool		m_bIsUrgent;
	short		m_sExclusivity;

	COutputEvent	m_OnArrival;

	DECLARE_DATADESC();

private:
	EHANDLE		m_hLockedBy;
};

//=============================================================================
//=============================================================================
class CAssaultPoint : public CPointEntity 
{
	DECLARE_CLASS( CAssaultPoint, CPointEntity );

public:
	CAssaultPoint()
	{
		// This used to be a constant in code. Now it's a keyfield in hammer. 
		// So in the constructor, we set this value to the former constant
		// default value, for legacy maps. (sjb)
		m_flAssaultPointTolerance = CUE_POINT_TOLERANCE;
	}

	void 			InputSetClearOnContact( inputdata_t &inputdata )
	{
		m_bClearOnContact = inputdata.value.Bool();
	}

	void 			InputSetAllowDiversion( inputdata_t &inputdata )
	{
		m_bAllowDiversion = inputdata.value.Bool();
	}

	void 			InputSetForceClear( inputdata_t &inputdata )
	{
		m_bInputForcedClear = inputdata.value.Bool();
	}

public:
	string_t		m_AssaultHintGroup;
	string_t		m_NextAssaultPointName;
	COutputEvent	m_OnAssaultClear;
	float			m_flAssaultTimeout;
	bool			m_bClearOnContact;
	bool			m_bAllowDiversion;
	float			m_flAllowDiversionRadius;
	bool			m_bNeverTimeout;
	int				m_iStrictness;
	bool			m_bForceCrouch;
	bool			m_bIsUrgent;
	bool			m_bInputForcedClear;
	float			m_flAssaultPointTolerance;
	float			m_flTimeLastUsed;

	COutputEvent	m_OnArrival;

	DECLARE_DATADESC();
};

//=============================================================================
//=============================================================================
class CAI_AssaultBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_AssaultBehavior, CAI_SimpleBehavior );

public:
	CAI_AssaultBehavior();
	
	virtual const char *GetName() {	return "Assault"; }
	virtual int	DrawDebugTextOverlays( int text_offset );

	virtual void OnRestore();

	bool CanRunAScriptedNPCInteraction( bool bForced );

	virtual bool 	CanSelectSchedule();
	virtual void	BeginScheduleSelection();
	virtual void	EndScheduleSelection();
	
	bool HasHitRallyPoint() { return m_bHitRallyPoint; }
	bool HasHitAssaultPoint() { return m_bHitAssaultPoint; }

	void ClearAssaultPoint( void );
	void OnHitAssaultPoint( void );
	bool PollAssaultCue( void );
	void ReceiveAssaultCue( AssaultCue_t cue );
	bool HasAssaultCue( void ) { return m_AssaultCue != CUE_NO_ASSAULT; }
	bool AssaultHasBegun();

	CAssaultPoint *FindAssaultPoint( string_t iszAssaultPointName );
	void SetAssaultPoint( CAssaultPoint *pAssaultPoint );

	void GatherConditions( void );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	void BuildScheduleTestBits();
	int TranslateSchedule( int scheduleType );
	void OnStartSchedule( int scheduleType );
	void ClearSchedule( const char *szReason );

	void InitializeBehavior();
	void SetParameters( string_t rallypointname, AssaultCue_t assaultcue, int rallySelectMethod );
	void SetParameters( CBaseEntity *pRallyEnt, AssaultCue_t assaultcue );

	bool IsAllowedToDivert( void );
	bool IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	float GetMaxTacticalLateralMovement( void );

	void UpdateOnRemove();

	bool OnStrictAssault( void	);
	bool UpdateForceCrouch( void );
	bool IsForcingCrouch( void );
	bool IsUrgent( void );

	CRallyPoint *FindBestRallyPointInRadius( const Vector &vecCenter, float flRadius );

	void Disable( void ) { m_AssaultCue = CUE_NO_ASSAULT; m_bHitRallyPoint = false; m_bHitAssaultPoint = false; }

	enum
	{
		SCHED_MOVE_TO_RALLY_POINT = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_ASSAULT_FAILED_TO_MOVE,
		SCHED_FAIL_MOVE_TO_RALLY_POINT,
		SCHED_MOVE_TO_ASSAULT_POINT,
		SCHED_AT_ASSAULT_POINT,
		SCHED_HOLD_RALLY_POINT,
		SCHED_HOLD_ASSAULT_POINT,
		SCHED_WAIT_AND_CLEAR,
		SCHED_ASSAULT_MOVE_AWAY,
		SCHED_CLEAR_ASSAULT_POINT,
		NEXT_SCHEDULE,

		TASK_GET_PATH_TO_RALLY_POINT = BaseClass::NEXT_TASK,
		TASK_FACE_RALLY_POINT,
		TASK_GET_PATH_TO_ASSAULT_POINT,
		TASK_FACE_ASSAULT_POINT,
		TASK_HIT_ASSAULT_POINT,
		TASK_HIT_RALLY_POINT,
		TASK_AWAIT_CUE,
		TASK_AWAIT_ASSAULT_TIMEOUT,
		TASK_ANNOUNCE_CLEAR,
		TASK_WAIT_ASSAULT_DELAY,
		TASK_ASSAULT_MOVE_AWAY_PATH,
		TASK_ASSAULT_DEFER_SCHEDULE_SELECTION,
		NEXT_TASK,

/*
		COND_PUT_CONDITIONS_HERE = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
*/
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:
	CHandle<CAssaultPoint> m_hAssaultPoint;
	CHandle<CRallyPoint> m_hRallyPoint;

public:
	void			UnlockRallyPoint( void );

private:
	void			OnScheduleChange();
	virtual int		SelectSchedule();

	AssaultCue_t	m_AssaultCue;			// the cue we're waiting for to begin the assault
	AssaultCue_t	m_ReceivedAssaultCue;	// the last assault cue we received from someone/thing external.

	bool			m_bHitRallyPoint;
	bool			m_bHitAssaultPoint;

	// Diversion
	bool			m_bDiverting;
	float			m_flLastSawAnEnemyAt;

	float			m_flTimeDeferScheduleSelection;

	string_t		m_AssaultPointName;

	//---------------------------------
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_ASSAULT_H
