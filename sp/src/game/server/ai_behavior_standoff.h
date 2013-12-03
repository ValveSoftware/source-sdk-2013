//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Combat behaviors for AIs in a relatively self-preservationist mode.
//			Lots of cover taking and attempted shots out of cover.
//
//=============================================================================//

#ifndef AI_BEHAVIOR_STANDOFF_H
#define AI_BEHAVIOR_STANDOFF_H

#include "utlvector.h"
#include "utlmap.h"

#include "ai_behavior.h"
#include "ai_utils.h"
#include "ai_hint.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

enum AI_HintChangeReaction_t
{
	AIHCR_DEFAULT_AI,
	AIHCR_MOVE_ON_COVER,
	AIHCR_MOVE_IMMEDIATE,
};

struct AI_StandoffParams_t
{
	AI_HintChangeReaction_t hintChangeReaction;
	bool					fCoverOnReload;
	bool					fPlayerIsBattleline;
	float 					minTimeShots;
	float 					maxTimeShots;
	int 					minShots;
	int 					maxShots;
	int 					oddsCover;
	bool					fStayAtCover;
	float					flAbandonTimeLimit;
	
	DECLARE_SIMPLE_DATADESC();
};

//-------------------------------------

enum AI_Posture_t
{
	AIP_INDIFFERENT,
	AIP_STANDING,
	AIP_CROUCHING,
	AIP_PEEKING,
};

enum
{
	STANDOFF_SENTENCE_BEGIN_STANDOFF = SENTENCE_BASE_BEHAVIOR_INDEX,
	STANDOFF_SENTENCE_END_STANDOFF,
	STANDOFF_SENTENCE_OUT_OF_AMMO,
	STANDOFF_SENTENCE_FORCED_TAKE_COVER,
	STANDOFF_SENTENCE_STAND_CHECK_TARGET,
};

class CAI_MappedActivityBehavior_Temporary : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_MappedActivityBehavior_Temporary, CAI_SimpleBehavior );
public:
	CAI_MappedActivityBehavior_Temporary( CAI_BaseNPC *pOuter = NULL ) 
	 :	CAI_SimpleBehavior(pOuter)
	{
		SetDefLessFunc( m_ActivityMap );
	}

protected:
	Activity GetMappedActivity( AI_Posture_t posture, Activity activity );
	void OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	virtual void UpdateTranslateActivityMap();

private:
	CUtlMap<unsigned, Activity> m_ActivityMap;
};

class CAI_StandoffBehavior : public CAI_MappedActivityBehavior_Temporary
{
	DECLARE_CLASS( CAI_StandoffBehavior, CAI_MappedActivityBehavior_Temporary );
public:
	CAI_StandoffBehavior( CAI_BaseNPC *pOuter = NULL );

	virtual const char *GetName() {	return "Standoff"; }

	void		SetActive( bool fActive );
	void		SetParameters( const AI_StandoffParams_t &params, CAI_GoalEntity *pGoalEntity = NULL );

	Vector		GetStandoffGoalPosition();
	void		SetStandoffGoalPosition( const Vector &vecPos );
	void		ClearStandoffGoalPosition();

	AI_Posture_t GetPosture();

	bool		IsActive( void ) { return m_fActive; }
	void 		OnChangeTacticalConstraints();

	bool 		CanSelectSchedule();
	bool		IsBehindBattleLines( const Vector &point );

protected:
	void 		Spawn();
	void 		BeginScheduleSelection();
	void		EndScheduleSelection();
	void		PrescheduleThink();
	void		GatherConditions();
	int 		SelectSchedule();
	int			TranslateSchedule( int scheduleType );
	void 		StartTask( const Task_t *pTask );
	void		BuildScheduleTestBits();
	virtual void OnUpdateShotRegulator();

	Activity 	NPC_TranslateActivity( Activity eNewActivity );
	
	bool		IsValidCover( const Vector &vecCoverLocation, CAI_Hint const *pHint );
	bool		IsValidShootPosition( const Vector &vecCoverLocation, CAI_Node *pNode, CAI_Hint const *pHint );

	void		SetPosture( AI_Posture_t posture );
	
	void		OnChangeHintGroup( string_t oldGroup, string_t newGroup );

	virtual int SelectScheduleUpdateWeapon();
	virtual int SelectScheduleCheckCover();
	virtual int SelectScheduleEstablishAim();
	virtual int SelectScheduleAttack();
	
	bool 		PlayerIsLeading();
	CBaseEntity *GetPlayerLeader();
	bool		GetDirectionOfStandoff( Vector *pDir );
			
	void UpdateBattleLines();

	Hint_e GetHintType();
	void SetReuseCurrentCover();
	void UnlockHintNode();
	Activity GetCoverActivity();

	void OnRestore();

	void UpdateTranslateActivityMap();

	// Standoff overrides base AI crouch handling
	bool		IsCrouching( void ) { return false; }
	
private:
	
	//----------------------------

	enum
	{
		NEXT_SCHEDULE = BaseClass::NEXT_SCHEDULE,

		NEXT_TASK = BaseClass::NEXT_TASK,

		COND_ABANDON_TIME_EXPIRED = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	
	//---------------------------------
	// @TODO (toml 07-30-03): replace all these booleans with a singe 32 bit unsigned & bit flags

	bool			m_fActive;
	bool			m_fTestNoDamage;

	Vector			m_vecStandoffGoalPosition;
	
	AI_Posture_t	m_posture;
	
	AI_StandoffParams_t m_params;
	EHANDLE			m_hStandoffGoal;
	
	bool 			m_fTakeCover;
	float			m_SavedDistTooFar;
	bool			m_fForceNewEnemy;
	CAI_MoveMonitor m_PlayerMoveMonitor;
	
	CSimTimer		m_TimeForceCoverHint;
	CSimTimer		m_TimePreventForceNewEnemy;
	CRandSimTimer	m_RandomCoverChangeTimer;

	// FIXME: TEMPORARY! REMOVE
	int m_nSavedMinShots, m_nSavedMaxShots;
	float m_flSavedMinRest, m_flSavedMaxRest;

	//---------------------------------
	
	struct BattleLine_t
	{
		Vector point;
		Vector normal;
	};

	CThinkOnceSemaphore		 m_UpdateBattleLinesSemaphore;
	CUtlVector<BattleLine_t> m_BattleLines;
	bool					 m_fIgnoreFronts;

	//---------------------------------
	
	bool						m_bHasLowCoverActivity;

	//---------------------------------
	
	DECLARE_DATADESC();
};

//-------------------------------------

inline void CAI_StandoffBehavior::SetPosture( AI_Posture_t posture )
{
	m_posture = posture;
}

//-------------------------------------

inline AI_Posture_t CAI_StandoffBehavior::GetPosture()
{
	return m_posture;
}

//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_STANDOFF_H
