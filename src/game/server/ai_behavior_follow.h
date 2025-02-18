//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_BEHAVIOR_FOLLOW_H
#define AI_BEHAVIOR_FOLLOW_H

#include "simtimer.h"
#include "ai_behavior.h"
#include "ai_goalentity.h"
#include "ai_utils.h"
#include "ai_moveshoot.h"

#ifdef HL2_EPISODIC
	#include "hl2_gamerules.h"
#endif

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// NOTE: these must correspond with the AI_FollowFormation_t array in AI_Behavior_Follow.cpp!!
//-----------------------------------------------------------------------------
enum AI_Formations_t
{
	AIF_SIMPLE,
	AIF_WIDE,
	AIF_ANTLION,
	AIF_COMMANDER,
	AIF_TIGHT,
	AIF_MEDIUM,
	AIF_SIDEKICK,
	AIF_HUNTER,
	AIF_VORTIGAUNT,
};

enum AI_FollowFormationFlags_t
{
	AIFF_DEFAULT 					= 0,
	AIFF_USE_FOLLOW_POINTS 			= 0x01,
	AIFF_REQUIRE_LOS_OUTSIDE_COMBAT	= 0x02,
};

//-----------------------------------------------------------------------------
//
// CAI_FollowGoal
//
// Purpose: A level tool to control the follow behavior. Use is not required
//			in order to use behavior.
//
//-----------------------------------------------------------------------------

class CAI_FollowGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_FollowGoal, CAI_GoalEntity );

public:

	virtual void EnableGoal( CAI_BaseNPC *pAI );
	virtual void DisableGoal( CAI_BaseNPC *pAI  );
#ifdef HL2_EPISODIC
	virtual void InputOutsideTransition( inputdata_t &inputdata );
#endif

	int m_iFormation;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------

int AIGetNumFollowers( CBaseEntity *pEntity, string_t iszClassname = NULL_STRING );

//-----------------------------------------------------------------------------

struct AI_FollowNavInfo_t
{
	int		flags;
	Vector  position;
	float 	range;
	float	Zrange;
	float   tolerance;
	float   followPointTolerance;
	float	targetMoveTolerance;
	float	repathOnRouteTolerance;
	float	walkTolerance;
	float	coverTolerance;
	float	enemyLOSTolerance;
	float	chaseEnemyTolerance;

	DECLARE_SIMPLE_DATADESC();
};

struct AI_FollowGroup_t;

struct AI_FollowManagerInfoHandle_t
{
	AI_FollowGroup_t *m_pGroup;
	intp m_hFollower;
};

//-------------------------------------

struct AI_FollowParams_t
{
	AI_FollowParams_t( AI_Formations_t formation = AIF_SIMPLE, bool bNormalMemoryDiscard = false )
	 :	formation(formation),
		bNormalMemoryDiscard( bNormalMemoryDiscard )
	{
	}
	
	AI_Formations_t formation;
	bool			bNormalMemoryDiscard;
	
	DECLARE_SIMPLE_DATADESC();
};

//-------------------------------------

class CAI_FollowBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_FollowBehavior, CAI_SimpleBehavior );
public:
	CAI_FollowBehavior( const AI_FollowParams_t &params = AIF_SIMPLE );
	~CAI_FollowBehavior();

	virtual int		DrawDebugTextOverlays( int text_offset );
	virtual void	DrawDebugGeometryOverlays();
	
	// Returns true if the NPC is actively following a target.
	bool			IsActive( void );

	void			SetParameters( const AI_FollowParams_t &params );

	virtual const char *GetName() {	return "Follow"; }
	AI_Formations_t GetFormation() const { return m_params.formation; }

	virtual bool 	CanSelectSchedule();

	const AI_FollowNavInfo_t &GetFollowGoalInfo();
	CBaseEntity *	GetFollowTarget();
	void			SetFollowTarget( CBaseEntity *pLeader, bool fFinishCurSchedule = false );

	CAI_FollowGoal *GetFollowGoal()	{ return m_hFollowGoalEnt; } // if any
	bool			SetFollowGoal( CAI_FollowGoal *pGoal, bool fFinishCurSchedule = false );
	void			ClearFollowGoal( CAI_FollowGoal *pGoal );
	void			SetFollowGoalDirect( CAI_FollowGoal *pGoal );

	virtual bool	FarFromFollowTarget()	{ return ( m_hFollowTarget && (GetAbsOrigin() - m_hFollowTarget->GetAbsOrigin()).LengthSqr() > (75*12)*(75*12) ); }

	virtual bool	TargetIsUnreachable() { return m_bTargetUnreachable; }
	
	int				GetNumFailedFollowAttempts()	{ return m_nFailedFollowAttempts; }
	float			GetTimeFailFollowStarted()		{ return m_flTimeFailFollowStarted; }
	bool			FollowTargetVisible() { return HasCondition( COND_FOLLOW_TARGET_VISIBLE ); };

	bool			IsMovingToFollowTarget();

	float			GetGoalRange();
	float			GetGoalZRange();

	virtual Activity	NPC_TranslateActivity( Activity activity );
	virtual int			TranslateSchedule( int scheduleType );
	virtual void	StartTask( const Task_t *pTask );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void	TaskComplete( bool fIgnoreSetFailedCondition = false );
	virtual void 	GatherConditions();

protected:

	const Vector	&GetGoalPosition();

	virtual bool	ShouldFollow();

	friend class CAI_FollowManager;

	virtual void	BeginScheduleSelection();
	virtual void	EndScheduleSelection();
	
	virtual void	CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput );

	virtual void	Precache();
	virtual int		SelectSchedule();
	virtual int		FollowCallBaseSelectSchedule() { return BaseClass::SelectSchedule(); }
	virtual void	OnStartSchedule( int scheduleType );
	virtual void	RunTask( const Task_t *pTask );
	void			BuildScheduleTestBits();

	bool			IsCurScheduleFollowSchedule();

	virtual bool	IsCurTaskContinuousMove();
	virtual void	OnMovementFailed();
	virtual void	OnMovementComplete();
	virtual bool	FValidateHintType( CAI_Hint *pHint );
	
	bool			IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool			IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	bool 			FindCoverFromEnemyAtFollowTarget( float coverRadius, Vector *pResult );
	
	bool			ShouldAlwaysThink();

	bool			ShouldMoveToFollowTarget();

	int 			SelectScheduleManagePosition();
	int 			SelectScheduleFollowPoints();
	int 			SelectScheduleMoveToFormation();

	void			GetFollowTargetViewLoc( Vector *pResult);
	bool			ValidateFaceTarget(  Vector *pFaceTarget );

	//----------------------------

	bool			ShouldUseFollowPoints();
	bool			HasFollowPoint();
	void 			SetFollowPoint( CAI_Hint *pHintNode );
	void			ClearFollowPoint();
	const Vector &	GetFollowPoint();
	CAI_Hint *		FindFollowPoint();
	bool			IsFollowPointInRange();
	bool			ShouldIgnoreFollowPointFacing();

	//----------------------------
	
	bool 			UpdateFollowPosition();
	const int		GetGoalFlags();
	float 			GetGoalTolerance();
	bool			PlayerIsPushing();

	bool IsFollowTargetInRange( float rangeMultiplier = 1.0 );

	bool			IsFollowGoalInRange( float tolerance, float zTolerance, int flags );
	virtual bool	IsChaseGoalInRange();

	void			NoteFailedFollow();
	void			NoteSuccessfulFollow();

	//----------------------------

protected:

	enum
	{
		SCHED_FOLLOWER_MOVE_AWAY_FAIL = BaseClass::NEXT_SCHEDULE,								// Turn back toward player
		SCHED_FOLLOWER_MOVE_AWAY_END,
		SCHED_FOLLOW,
		SCHED_FOLLOWER_IDLE_STAND,
		SCHED_MOVE_TO_FACE_FOLLOW_TARGET,
		SCHED_FACE_FOLLOW_TARGET,
		SCHED_FOLLOWER_GO_TO_WAIT_POINT,
		SCHED_FOLLOWER_GO_TO_WAIT_POINT_FAIL,
		SCHED_FOLLOWER_STAND_AT_WAIT_POINT,
		SCHED_FOLLOWER_COMBAT_FACE,
		NEXT_SCHEDULE,

		TASK_CANT_FOLLOW = BaseClass::NEXT_TASK,
		TASK_FACE_FOLLOW_TARGET,
		TASK_MOVE_TO_FOLLOW_POSITION,
		TASK_GET_PATH_TO_FOLLOW_POSITION,
		TASK_SET_FOLLOW_TARGET_MARK,
		TASK_FOLLOWER_FACE_TACTICAL,
		TASK_SET_FOLLOW_DELAY,
		TASK_GET_PATH_TO_FOLLOW_POINT,
		TASK_ARRIVE_AT_FOLLOW_POINT,
		TASK_SET_FOLLOW_POINT_STAND_SCHEDULE,
		TASK_BEGIN_STAND_AT_WAIT_POINT,
		NEXT_TASK,

		COND_TARGET_MOVED_FROM_MARK = BaseClass::NEXT_CONDITION,
		COND_FOUND_WAIT_POINT,
		COND_FOLLOW_DELAY_EXPIRED,
		COND_FOLLOW_TARGET_VISIBLE,
		COND_FOLLOW_TARGET_NOT_VISIBLE,
		COND_FOLLOW_WAIT_POINT_INVALID,
		COND_FOLLOW_PLAYER_IS_LIT,
		COND_FOLLOW_PLAYER_IS_NOT_LIT,
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	
protected:

	//----------------------------
	
	EHANDLE 		   				m_hFollowTarget;
	AI_FollowNavInfo_t 				m_FollowNavGoal;
	float							m_flTimeUpdatedFollowPosition;
	bool							m_bFirstFacing;
	float							m_flTimeFollowTargetVisible;
	
	CAI_MoveMonitor	   				m_TargetMonitor;
	bool							m_bTargetUnreachable;
	bool							m_bFollowNavFailed; // Set when pathfinding fails to limit impact of m_FollowDelay on ShouldFollow

	int								m_nFailedFollowAttempts;
	float							m_flTimeFailFollowStarted;
	Vector							m_vFollowMoveAnchor;

	bool							m_bMovingToCover;
	float							m_flOriginalEnemyDiscardTime;
	float							m_SavedDistTooFar;
	
	CRandStopwatch	   				m_FollowDelay;
	CSimpleSimTimer					m_RepathOnFollowTimer;
	
	//---------------------------------

	Activity						m_CurrentFollowActivity;

	//---------------------------------
	
	CRandSimTimer					m_TimeBlockUseWaitPoint;
	CSimTimer						m_TimeCheckForWaitPoint;
	CAI_Hint *						m_pInterruptWaitPoint;
	
	//---------------------------------

	CRandSimTimer					m_TimeBeforeSpreadFacing;
	CRandSimTimer					m_TimeNextSpreadFacing;

	//---------------------------------
	
	AI_FollowManagerInfoHandle_t 	m_hFollowManagerInfo;
	AI_FollowParams_t				m_params;

	//---------------------------------
	
	CHandle<CAI_FollowGoal>			m_hFollowGoalEnt;
	
	//---------------------------------
	
	DECLARE_DATADESC();
};

//-------------------------------------

inline const AI_FollowNavInfo_t &CAI_FollowBehavior::GetFollowGoalInfo()
{
	return m_FollowNavGoal;
}

//-------------------------------------

inline const int CAI_FollowBehavior::GetGoalFlags()
{
	return m_FollowNavGoal.flags;
}

//-------------------------------------

inline const Vector &CAI_FollowBehavior::GetGoalPosition()
{
	return m_FollowNavGoal.position;
}

//-------------------------------------

inline float CAI_FollowBehavior::GetGoalTolerance()
{
	return m_FollowNavGoal.tolerance;
}

//-------------------------------------

inline float CAI_FollowBehavior::GetGoalRange()
{
	return m_FollowNavGoal.range;
}

//-------------------------------------

inline float CAI_FollowBehavior::GetGoalZRange()
{
	return m_FollowNavGoal.Zrange;
}

//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_FOLLOW_H
