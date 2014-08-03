//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BEHAVIOR_H
#define AI_BEHAVIOR_H

#include "ai_component.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "AI_Criteria.h"
#include "networkvar.h"

#ifdef DEBUG
#pragma warning(push)
#include <typeinfo>
#pragma warning(pop)
#pragma warning(disable:4290)
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_Behavior...
//
// Purpose:	The core component that defines a behavior in an NPC by selecting
//			schedules and running tasks
//
//			Intended to be used as an organizational tool as well as a way
//			for various NPCs to share behaviors without sharing an inheritance
//			relationship, and without cramming those behaviors into the base
//			NPC class.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Base class defines interface to behaviors and provides bridging
//			methods
//-----------------------------------------------------------------------------

class IBehaviorBackBridge;

//-------------------------------------

abstract_class CAI_BehaviorBase : public CAI_Component
{
	DECLARE_CLASS( CAI_BehaviorBase, CAI_Component )
public:
	CAI_BehaviorBase(CAI_BaseNPC *pOuter = NULL)
	 : 	CAI_Component(pOuter),
	 	m_pBackBridge(NULL)
	{
	}

	virtual const char *GetName() = 0;

	virtual bool KeyValue( const char *szKeyName, const char *szValue ) 
	{
		return false;
	}
	
	bool IsRunning()								{ Assert( GetOuter() ); return ( GetOuter()->GetRunningBehavior() == this ); }
	virtual bool CanSelectSchedule()				{ return true; }
	virtual void BeginScheduleSelection() 			{}
	virtual void EndScheduleSelection() 			{}
	
	void SetBackBridge( IBehaviorBackBridge *pBackBridge )
	{
		Assert( m_pBackBridge == NULL || pBackBridge == NULL );
		m_pBackBridge = pBackBridge;
	}

	void BridgePrecache()									{ Precache();		}
	void BridgeSpawn()										{ Spawn();			}
	void BridgeUpdateOnRemove()								{ UpdateOnRemove();	}
	void BridgeEvent_Killed( const CTakeDamageInfo &info )	{ Event_Killed( info );	}
	void BridgeCleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput )		{ CleanupOnDeath( pCulprit, bFireDeathOutput ); }

	void BridgeOnChangeHintGroup( string_t oldGroup, string_t newGroup ) { 	OnChangeHintGroup( oldGroup, newGroup ); }

	void BridgeGatherConditions()					{ GatherConditions(); }
	void BridgePrescheduleThink()					{ PrescheduleThink(); }
	void BridgeOnScheduleChange()					{ OnScheduleChange(); }
	void BridgeOnStartSchedule( int scheduleType );

	int  BridgeSelectSchedule();
	bool BridgeSelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode, int *pResult );
	bool BridgeStartTask( const Task_t *pTask );
	bool BridgeRunTask( const Task_t *pTask);
	bool BridgeAimGun( void );
	int BridgeTranslateSchedule( int scheduleType );
	bool BridgeGetSchedule( int localScheduleID, CAI_Schedule **ppResult );
	bool BridgeTaskName(int taskID, const char **);
	Activity BridgeNPC_TranslateActivity( Activity activity );
	void BridgeBuildScheduleTestBits()		{ BuildScheduleTestBits(); }
	bool BridgeIsCurTaskContinuousMove( bool *pResult );
	void BridgeOnMovementFailed()					{ OnMovementFailed(); }
	void BridgeOnMovementComplete()					{ OnMovementComplete(); }
	float BridgeGetDefaultNavGoalTolerance();
	bool BridgeFValidateHintType( CAI_Hint *pHint, bool *pResult );
	bool BridgeIsValidEnemy( CBaseEntity *pEnemy );
	CBaseEntity *BridgeBestEnemy();
	bool BridgeIsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool BridgeIsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	float BridgeGetMaxTacticalLateralMovement( void );
	bool BridgeShouldIgnoreSound( CSound *pSound );
	void BridgeOnSeeEntity( CBaseEntity *pEntity );
	void BridgeOnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );
	bool BridgeIsInterruptable( void );
	bool BridgeIsNavigationUrgent( void );
	bool BridgeShouldPlayerAvoid( void );
	int	 BridgeOnTakeDamage_Alive( const CTakeDamageInfo &info );
	float BridgeGetReasonableFacingDist( void );
	bool BridgeShouldAlwaysThink( bool *pResult );
	void BridgeOnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	void BridgeOnRestore();
	virtual bool BridgeSpeakMapmakerInterruptConcept( string_t iszConcept );
	bool BridgeCanFlinch( void );
	bool BridgeIsCrouching( void );
	bool BridgeIsCrouchedActivity( Activity activity );
	bool BridgeQueryHearSound( CSound *pSound );
	bool BridgeCanRunAScriptedNPCInteraction( bool bForced );
	Activity BridgeGetFlinchActivity( bool bHeavyDamage, bool bGesture );
	bool BridgeOnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	void BridgeModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );
	void BridgeTeleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	void BridgeHandleAnimEvent( animevent_t *pEvent );

	virtual void GatherConditions();
	virtual void GatherConditionsNotActive() { return; } // Override this and your behavior will call this in place of GatherConditions() when your behavior is NOT the active one.
	virtual void OnUpdateShotRegulator() {}

	virtual CAI_ClassScheduleIdSpace *GetClassScheduleIdSpace();

	virtual int  DrawDebugTextOverlays( int text_offset );

	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );

	static void SaveBehaviors(ISave &save, CAI_BehaviorBase *pCurrentBehavior, CAI_BehaviorBase **ppBehavior, int nBehaviors );
	static int RestoreBehaviors(IRestore &restore, CAI_BehaviorBase **ppBehavior, int nBehaviors ); // returns index of "current" behavior, or -1

protected:

	int GetNpcState() { return GetOuter()->m_NPCState; }

	virtual void Precache()										{}
	virtual void Spawn()										{}
	virtual void UpdateOnRemove()								{}
	virtual void Event_Killed( const CTakeDamageInfo &info )	{}
	virtual void CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput ) {}
	
	virtual void PrescheduleThink();
	virtual void OnScheduleChange();
	virtual void OnStartSchedule( int scheduleType );

	virtual int SelectSchedule();
	virtual int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual void AimGun( void );
	virtual int TranslateSchedule( int scheduleType );
	virtual CAI_Schedule *GetSchedule(int schedule);
	virtual const char *GetSchedulingErrorName();
	virtual void BuildScheduleTestBits() {}
	bool IsCurSchedule( int schedId, bool fIdeal = true );


	CAI_Hint *		GetHintNode()							{ return GetOuter()->GetHintNode(); }
	const CAI_Hint *GetHintNode() const						{ return GetOuter()->GetHintNode(); }
	void			SetHintNode( CAI_Hint *pHintNode )		{ GetOuter()->SetHintNode( pHintNode ); }
	void			ClearHintNode( float reuseDelay = 0.0 )	{ GetOuter()->ClearHintNode( reuseDelay ); }

protected:
	// Used by derived classes to chain a task to a task that might not be the 
	// one they are currently handling:
	void	ChainStartTask( int task, float taskData = 0 );
	void	ChainRunTask( int task, float taskData = 0 );

protected:

	virtual Activity NPC_TranslateActivity( Activity activity );

	virtual bool IsCurTaskContinuousMove();
	virtual void OnMovementFailed() {};
	virtual void OnMovementComplete() {};
	virtual float GetDefaultNavGoalTolerance();
	virtual bool FValidateHintType( CAI_Hint *pHint );

	virtual	bool IsValidEnemy( CBaseEntity *pEnemy );
	virtual CBaseEntity *BestEnemy();
	virtual	bool IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	virtual	bool IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	virtual float GetMaxTacticalLateralMovement( void );
	virtual bool ShouldIgnoreSound( CSound *pSound );
	virtual void OnSeeEntity( CBaseEntity *pEntity );
	virtual void OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );
	virtual bool IsInterruptable( void );
	virtual bool IsNavigationUrgent( void );
	virtual int	 OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual float GetReasonableFacingDist( void );
	virtual bool ShouldPlayerAvoid( void );
	virtual bool CanFlinch( void );
	virtual bool IsCrouching( void );
	virtual bool IsCrouchedActivity( Activity activity );
	virtual bool QueryHearSound( CSound *pSound );
	virtual bool CanRunAScriptedNPCInteraction( bool bForced );
	virtual Activity GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );
	virtual void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual bool ShouldAlwaysThink();

	virtual void OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) {};
	virtual bool SpeakMapmakerInterruptConcept( string_t iszConcept ) { return false; };
	
	virtual void OnRestore() {};
	
	bool NotifyChangeBehaviorStatus( bool fCanFinishSchedule = false );

	bool HaveSequenceForActivity( Activity activity )		{ return GetOuter()->HaveSequenceForActivity( activity ); }
	
	//---------------------------------

	string_t			GetHintGroup()			{ return GetOuter()->GetHintGroup();	}
	void				ClearHintGroup()			{ GetOuter()->ClearHintGroup();			}
	void				SetHintGroup( string_t name )	{ GetOuter()->SetHintGroup( name );		}

	virtual void		OnChangeHintGroup( string_t oldGroup, string_t newGroup ) {}

	//
	// These allow derived classes to implement custom schedules
	//
	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()		{ return CAI_BaseNPC::GetSchedulingSymbols(); }
	static bool				LoadSchedules()							{ return true; }
	virtual bool			IsBehaviorSchedule( int scheduleType )	{ return false; }

	CAI_Navigator *			GetNavigator() 							{ return GetOuter()->GetNavigator(); 		}
	CAI_Motor *				GetMotor() 								{ return GetOuter()->GetMotor(); 			}
	CAI_TacticalServices *	GetTacticalServices()					{ return GetOuter()->GetTacticalServices();	}

	bool 				 m_fOverrode;
	IBehaviorBackBridge *m_pBackBridge;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: Template provides provides back bridge to owning class and 
//			establishes namespace settings
//-----------------------------------------------------------------------------

template <class NPC_CLASS = CAI_BaseNPC, const int ID_SPACE_OFFSET = 100000>
class CAI_Behavior : public CAI_ComponentWithOuter<NPC_CLASS, CAI_BehaviorBase>
{
public:
	DECLARE_CLASS_NOFRIEND( CAI_Behavior, NPC_CLASS );
	
	enum
	{
		NEXT_TASK 			= ID_SPACE_OFFSET,
		NEXT_SCHEDULE 		= ID_SPACE_OFFSET,
		NEXT_CONDITION 		= ID_SPACE_OFFSET
	};

	void SetCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		this->GetOuter()->SetCondition( condition );
	}

	bool HasCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		return this->GetOuter()->HasCondition( condition );
	}

	bool HasInterruptCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		return this->GetOuter()->HasInterruptCondition( condition );
	}

	void ClearCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		this->GetOuter()->ClearCondition( condition );
	}

protected:
	CAI_Behavior(NPC_CLASS *pOuter = NULL)
	 : CAI_ComponentWithOuter<NPC_CLASS, CAI_BehaviorBase>(pOuter)
	{
	}

	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()
	{
		return NPC_CLASS::GetSchedulingSymbols();
	}
	virtual CAI_ClassScheduleIdSpace *GetClassScheduleIdSpace()
	{
		return this->GetOuter()->GetClassScheduleIdSpace();
	}

	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect()
	{
		return NPC_CLASS::AccessClassScheduleIdSpaceDirect();
	}

private:
	virtual bool IsBehaviorSchedule( int scheduleType ) { return ( scheduleType >= ID_SPACE_OFFSET && scheduleType < ID_SPACE_OFFSET + 10000 ); }
};


//-----------------------------------------------------------------------------
// Purpose: Some bridges a little more complicated to allow behavior to see 
//			what base class would do or control order in which it's donw
//-----------------------------------------------------------------------------

abstract_class IBehaviorBackBridge
{
public:
	virtual void 		 BackBridge_GatherConditions() = 0;
	virtual int 		 BackBridge_SelectSchedule() = 0;
	virtual int 		 BackBridge_TranslateSchedule( int scheduleType ) = 0;
	virtual Activity 	 BackBridge_NPC_TranslateActivity( Activity activity ) = 0;
	virtual bool		 BackBridge_IsValidEnemy(CBaseEntity *pEnemy) = 0;
	virtual CBaseEntity* BackBridge_BestEnemy(void) = 0;
	virtual bool		 BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint ) = 0;
	virtual bool		 BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint ) = 0;
	virtual float		 BackBridge_GetMaxTacticalLateralMovement( void ) = 0;
	virtual bool		 BackBridge_ShouldIgnoreSound( CSound *pSound ) = 0;
	virtual void		 BackBridge_OnSeeEntity( CBaseEntity *pEntity ) = 0;
	virtual void		 BackBridge_OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker ) = 0;
	virtual bool		 BackBridge_IsInterruptable( void ) = 0;
	virtual bool		 BackBridge_IsNavigationUrgent( void ) = 0;
	virtual bool		 BackBridge_ShouldPlayerAvoid( void ) = 0;
	virtual int			 BackBridge_OnTakeDamage_Alive( const CTakeDamageInfo &info ) = 0;
	virtual float		 BackBridge_GetDefaultNavGoalTolerance() = 0;
	virtual float		 BackBridge_GetReasonableFacingDist( void ) = 0;
	virtual bool		 BackBridge_CanFlinch( void ) = 0;
	virtual bool		 BackBridge_IsCrouching( void ) = 0;
	virtual bool		 BackBridge_IsCrouchedActivity( Activity activity ) = 0;
	virtual bool		 BackBridge_QueryHearSound( CSound *pSound ) = 0;
	virtual bool		 BackBridge_CanRunAScriptedNPCInteraction( bool bForced ) = 0;
	virtual Activity	 BackBridge_GetFlinchActivity( bool bHeavyDamage, bool bGesture ) = 0;
	virtual bool		 BackBridge_OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) = 0;
	virtual void		 BackBridge_ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet ) = 0;
	virtual void		 BackBridge_Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity ) = 0;

	virtual void		 BackBridge_HandleAnimEvent( animevent_t *pEvent ) = 0;

//-------------------------------------

};

//-----------------------------------------------------------------------------
// Purpose: The common instantiation of the above template
//-----------------------------------------------------------------------------

typedef CAI_Behavior<> CAI_SimpleBehavior;

//-----------------------------------------------------------------------------
// Purpose: Base class for AIs that want to act as a host for CAI_Behaviors
//			NPCs aren't required to use this, but probably want to.
//-----------------------------------------------------------------------------

template <class BASE_NPC>
class CAI_BehaviorHost : public BASE_NPC,
						 private IBehaviorBackBridge
{
public:
	DECLARE_CLASS_NOFRIEND( CAI_BehaviorHost, BASE_NPC );

	CAI_BehaviorHost()
	  : m_pCurBehavior(NULL)
	{
#ifdef DEBUG
  		m_fDebugInCreateBehaviors = false;
#endif
	}

	void CleanupOnDeath( CBaseEntity *pCulprit = NULL, bool bFireDeathOutput = true );

	virtual int		Save( ISave &save );
	virtual int		Restore( IRestore &restore );
	virtual bool 	CreateComponents();

	// Automatically called during entity construction, derived class calls AddBehavior()
	virtual bool 	CreateBehaviors()	{ return true; }
	
	// forces movement and sets a new schedule
	virtual bool	ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity );
	virtual bool	ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity );
	virtual void	ForceSelectedGo(CBaseEntity *pPlayer, const Vector &targetPos, const Vector &traceDir, bool bRun);
	virtual void	ForceSelectedGoRandom(void);

	// Bridges
	void			Precache();
	void			NPCInit();
	void			UpdateOnRemove();
	void			Event_Killed( const CTakeDamageInfo &info );
	void 			GatherConditions();
	void 			PrescheduleThink();
	int 			SelectSchedule();
	void			KeepRunningBehavior();
	int				SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	void 			OnScheduleChange();
	void			OnStartSchedule( int scheduleType );
	int 			TranslateSchedule( int scheduleType );
	void 			StartTask( const Task_t *pTask );
	void 			RunTask( const Task_t *pTask );
	void			AimGun( void );
	CAI_Schedule *	GetSchedule(int localScheduleID);
	const char *	TaskName(int taskID);
	void			BuildScheduleTestBits();

	void			OnChangeHintGroup( string_t oldGroup, string_t newGroup );
	
	Activity 		NPC_TranslateActivity( Activity activity );

	bool			IsCurTaskContinuousMove();
	void			OnMovementFailed();
	void			OnMovementComplete();
	bool			FValidateHintType( CAI_Hint *pHint );
	float			GetDefaultNavGoalTolerance();

	bool			IsValidEnemy(CBaseEntity *pEnemy);
	CBaseEntity*	BestEnemy(void);
	bool			IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool			IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	float			GetMaxTacticalLateralMovement( void );
	bool			ShouldIgnoreSound( CSound *pSound );
	void			OnSeeEntity( CBaseEntity *pEntity );
	void			OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );
	bool			IsInterruptable( void );
	bool			IsNavigationUrgent( void );
	bool			ShouldPlayerAvoid( void );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	float			GetReasonableFacingDist( void );
	bool			CanFlinch( void );
	bool			IsCrouching( void );
	bool			IsCrouchedActivity( Activity activity );
	bool			QueryHearSound( CSound *pSound );
	bool			CanRunAScriptedNPCInteraction( bool bForced );
	Activity		GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	bool			OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	void			HandleAnimEvent( animevent_t *pEvent );
	
	bool			ShouldAlwaysThink();

	void			OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	virtual bool	SpeakMapmakerInterruptConcept( string_t iszConcept );

	void			OnRestore();

	void			ModifyOrAppendCriteria( AI_CriteriaSet& set );
	void			Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );

	//---------------------------------

	virtual bool	OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );
	virtual void	OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

protected:
	void			AddBehavior( CAI_BehaviorBase *pBehavior );
	
	bool			BehaviorSelectSchedule();
	virtual bool	ShouldBehaviorSelectSchedule( CAI_BehaviorBase *pBehavior ) { return true; }

	bool 			IsRunningBehavior() const;
	CAI_BehaviorBase *GetRunningBehavior();
	CAI_BehaviorBase *DeferSchedulingToBehavior( CAI_BehaviorBase *pNewBehavior );
	void			ChangeBehaviorTo( CAI_BehaviorBase *pNewBehavior );

	CAI_Schedule *	GetNewSchedule();
	CAI_Schedule *	GetFailSchedule();
private:
	void 			BackBridge_GatherConditions();
	int				BackBridge_SelectSchedule();
	int				BackBridge_TranslateSchedule( int scheduleType );
	Activity		BackBridge_NPC_TranslateActivity( Activity activity );
	bool			BackBridge_IsValidEnemy(CBaseEntity *pEnemy);
	CBaseEntity*	BackBridge_BestEnemy(void);
	bool			BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool			BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	float			BackBridge_GetMaxTacticalLateralMovement( void );
	bool			BackBridge_ShouldIgnoreSound( CSound *pSound );
	void			BackBridge_OnSeeEntity( CBaseEntity *pEntity );
	void			BackBridge_OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );
	bool			BackBridge_IsInterruptable( void );
	bool			BackBridge_IsNavigationUrgent( void );
	bool			BackBridge_ShouldPlayerAvoid( void );
	int				BackBridge_OnTakeDamage_Alive( const CTakeDamageInfo &info );
	float			BackBridge_GetDefaultNavGoalTolerance();
	float			BackBridge_GetReasonableFacingDist( void );
	bool			BackBridge_CanFlinch( void );
	bool			BackBridge_IsCrouching( void );
	bool			BackBridge_IsCrouchedActivity( Activity activity );
	bool			BackBridge_QueryHearSound( CSound *pSound );
	bool			BackBridge_CanRunAScriptedNPCInteraction( bool bForced );
	Activity		BackBridge_GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	bool			BackBridge_OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	void			BackBridge_ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );
	void			BackBridge_Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );

	void			BackBridge_HandleAnimEvent( animevent_t *pEvent );

	CAI_BehaviorBase **AccessBehaviors();
	int				NumBehaviors();

	CAI_BehaviorBase *			   m_pCurBehavior;
	CUtlVector<CAI_BehaviorBase *> m_Behaviors;

	bool			m_bCalledBehaviorSelectSchedule;
	
#ifdef DEBUG
	bool 			m_fDebugInCreateBehaviors;
#endif
	
};

//-----------------------------------------------------------------------------

// The first frame a behavior begins schedule selection, it won't have had it's GatherConditions()
// called. To fix this, BeginScheduleSelection() manually calls the new behavior's GatherConditions(),
// but sets this global so that the baseclass GatherConditions() isn't called as well.
extern bool g_bBehaviorHost_PreventBaseClassGatherConditions;

//-----------------------------------------------------------------------------

inline void CAI_BehaviorBase::BridgeOnStartSchedule( int scheduleType )
{
	int localId = AI_IdIsGlobal( scheduleType ) ? GetClassScheduleIdSpace()->ScheduleGlobalToLocal( scheduleType ) : scheduleType;
	OnStartSchedule( localId );
}

//-------------------------------------

inline int CAI_BehaviorBase::BridgeSelectSchedule()
{
	int result = SelectSchedule();

	if ( IsBehaviorSchedule( result ) )
		return GetClassScheduleIdSpace()->ScheduleLocalToGlobal( result );

	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeSelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode, int *pResult )
{
	m_fOverrode = true;
	int result = SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
	if ( m_fOverrode )
	{
		if ( result != SCHED_NONE )
		{
			if ( IsBehaviorSchedule( result ) )
				*pResult = GetClassScheduleIdSpace()->ScheduleLocalToGlobal( result );
			else
				*pResult = result;
			return true;
		}
		Warning( "An AI behavior is in control but has no recommended schedule\n" );
	}
	return false;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeStartTask( const Task_t *pTask )
{
	m_fOverrode = true;
	StartTask( pTask );
	return m_fOverrode;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeRunTask( const Task_t *pTask)
{
	m_fOverrode = true;
	RunTask( pTask );
	return m_fOverrode;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeAimGun( void )
{
	m_fOverrode = true;
	AimGun();
	return m_fOverrode;
}

//-------------------------------------

inline void CAI_BehaviorBase::ChainStartTask( int task, float taskData )
{
	Task_t tempTask = { task, taskData }; 

	bool fPrevOverride = m_fOverrode;
	GetOuter()->StartTask( (const Task_t *)&tempTask );
	m_fOverrode = fPrevOverride;;
}

//-------------------------------------

inline void CAI_BehaviorBase::ChainRunTask( int task, float taskData )
{ 
	Task_t tempTask = { task, taskData }; 
	bool fPrevOverride = m_fOverrode;
	GetOuter()->RunTask( (const Task_t *)	&tempTask );
	m_fOverrode = fPrevOverride;;
}

//-------------------------------------

inline int CAI_BehaviorBase::BridgeTranslateSchedule( int scheduleType )
{
	int localId = AI_IdIsGlobal( scheduleType ) ? GetClassScheduleIdSpace()->ScheduleGlobalToLocal( scheduleType ) : scheduleType;
	int result = TranslateSchedule( localId );
	
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeGetSchedule( int localScheduleID, CAI_Schedule **ppResult )
{
	*ppResult = GetSchedule( localScheduleID );
	return (*ppResult != NULL );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeTaskName( int taskID, const char **ppResult )
{
	if ( AI_IdIsLocal( taskID ) )
	{
		*ppResult = GetSchedulingSymbols()->TaskIdToSymbol( GetClassScheduleIdSpace()->TaskLocalToGlobal( taskID ) );
		return (*ppResult != NULL );
	}
	return false;
}

//-------------------------------------

inline Activity CAI_BehaviorBase::BridgeNPC_TranslateActivity( Activity activity )
{
	return NPC_TranslateActivity( activity );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsCurTaskContinuousMove( bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = IsCurTaskContinuousMove();
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeFValidateHintType( CAI_Hint *pHint, bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = FValidateHintType( pHint );
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidEnemy( CBaseEntity *pEnemy )
{
	return IsValidEnemy( pEnemy );
}

//-------------------------------------

inline CBaseEntity *CAI_BehaviorBase::BridgeBestEnemy()
{
	return BestEnemy();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	return IsValidCover( vLocation, pHint );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	return IsValidShootPosition( vLocation, pNode, pHint );
}

//-------------------------------------

inline float CAI_BehaviorBase::BridgeGetMaxTacticalLateralMovement( void )
{
	return GetMaxTacticalLateralMovement();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeShouldIgnoreSound( CSound *pSound )
{
	return ShouldIgnoreSound( pSound );
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnSeeEntity( CBaseEntity *pEntity )
{
	OnSeeEntity( pEntity );
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker )
{
	OnFriendDamaged( pSquadmate, pAttacker );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsInterruptable( void )
{
	return IsInterruptable();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsNavigationUrgent( void )
{
	return IsNavigationUrgent();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeCanFlinch( void )
{
	return CanFlinch();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsCrouching( void )
{
	return IsCrouching();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsCrouchedActivity( Activity activity )
{
	return IsCrouchedActivity( activity );
}

inline bool CAI_BehaviorBase::BridgeQueryHearSound( CSound *pSound )
{
	return QueryHearSound( pSound );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeCanRunAScriptedNPCInteraction( bool bForced )
{
	return CanRunAScriptedNPCInteraction( bForced );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeShouldPlayerAvoid( void )
{
	return ShouldPlayerAvoid();
}

//-------------------------------------

inline int CAI_BehaviorBase::BridgeOnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	return OnTakeDamage_Alive( info );
}

//-------------------------------------

inline float CAI_BehaviorBase::BridgeGetReasonableFacingDist( void )
{
	return GetReasonableFacingDist();
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeShouldAlwaysThink( bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = ShouldAlwaysThink();
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeSpeakMapmakerInterruptConcept( string_t iszConcept )
{
	return SpeakMapmakerInterruptConcept( iszConcept );
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnRestore()
{
	OnRestore();
}

//-------------------------------------

inline float CAI_BehaviorBase::BridgeGetDefaultNavGoalTolerance()
{
	return GetDefaultNavGoalTolerance();
}

//-----------------------------------------------------------------------------

inline Activity CAI_BehaviorBase::BridgeGetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	return GetFlinchActivity( bHeavyDamage, bGesture );
}

//-----------------------------------------------------------------------------

inline bool CAI_BehaviorBase::BridgeOnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	return OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------

inline void CAI_BehaviorBase::BridgeModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	ModifyOrAppendCriteria( criteriaSet );
}

//-----------------------------------------------------------------------------

inline void CAI_BehaviorBase::BridgeTeleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	Teleport( newPosition, newAngles, newVelocity );
}

//-----------------------------------------------------------------------------

inline void CAI_BehaviorBase::BridgeHandleAnimEvent( animevent_t *pEvent )
{
	HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput )
{
	DeferSchedulingToBehavior( NULL );
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeCleanupOnDeath( pCulprit, bFireDeathOutput );
	}
	BaseClass::CleanupOnDeath( pCulprit, bFireDeathOutput );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::GatherConditions()					
{ 
	// Iterate over behaviors and call GatherConditionsNotActive() on each behavior
	// not currently active.
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		if( m_Behaviors[i] != m_pCurBehavior )
		{
			m_Behaviors[i]->GatherConditionsNotActive();
		}
	}

	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeGatherConditions(); 
	else
		BaseClass::GatherConditions();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_GatherConditions()
{
	if ( g_bBehaviorHost_PreventBaseClassGatherConditions )
		return;

	BaseClass::GatherConditions();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnScheduleChange()
{ 
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeOnScheduleChange(); 
	BaseClass::OnScheduleChange();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnStartSchedule( int scheduleType )
{
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeOnStartSchedule( scheduleType ); 
	BaseClass::OnStartSchedule( scheduleType );
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::BackBridge_SelectSchedule() 
{
	return BaseClass::SelectSchedule();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BehaviorSelectSchedule()
{
	for ( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		if ( m_Behaviors[i]->CanSelectSchedule() && ShouldBehaviorSelectSchedule( m_Behaviors[i] ) )
		{
			DeferSchedulingToBehavior( m_Behaviors[i] );
			return true;
		}
	}
	
	DeferSchedulingToBehavior( NULL );
	return false;
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsRunningBehavior() const
{
	return ( m_pCurBehavior != NULL );
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase *CAI_BehaviorHost<BASE_NPC>::GetRunningBehavior()
{
	return m_pCurBehavior;
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_Schedule *CAI_BehaviorHost<BASE_NPC>::GetNewSchedule()
{
	m_bCalledBehaviorSelectSchedule = false;
	CAI_Schedule *pResult = BaseClass::GetNewSchedule();
	if ( !m_bCalledBehaviorSelectSchedule && m_pCurBehavior )
		DeferSchedulingToBehavior( NULL );
	return pResult;
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_Schedule *CAI_BehaviorHost<BASE_NPC>::GetFailSchedule()
{
	m_bCalledBehaviorSelectSchedule = false;
	CAI_Schedule *pResult = BaseClass::GetFailSchedule();
	if ( !m_bCalledBehaviorSelectSchedule && m_pCurBehavior )
		DeferSchedulingToBehavior( NULL );
	return pResult;
}

//------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::ChangeBehaviorTo( CAI_BehaviorBase *pNewBehavior )
{
	bool change = ( m_pCurBehavior != pNewBehavior );
	CAI_BehaviorBase *pOldBehavior = m_pCurBehavior;
	m_pCurBehavior = pNewBehavior;
	
	if ( change ) 
	{
		if ( m_pCurBehavior )
		{
			m_pCurBehavior->BeginScheduleSelection();

			g_bBehaviorHost_PreventBaseClassGatherConditions = true;
			m_pCurBehavior->GatherConditions();
			g_bBehaviorHost_PreventBaseClassGatherConditions = false;
		}

		if ( pOldBehavior )
		{
			pOldBehavior->EndScheduleSelection();
			this->VacateStrategySlot();
		}

		OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
	}
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase *CAI_BehaviorHost<BASE_NPC>::DeferSchedulingToBehavior( CAI_BehaviorBase *pNewBehavior )
{
	CAI_BehaviorBase *pOldBehavior = m_pCurBehavior;
	ChangeBehaviorTo( pNewBehavior );
	return pOldBehavior;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::BackBridge_TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::TranslateSchedule( int scheduleType ) 
{
	if ( m_pCurBehavior )
	{
		return m_pCurBehavior->BridgeTranslateSchedule( scheduleType );
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgePrescheduleThink();
}	

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::SelectSchedule()
{
	m_bCalledBehaviorSelectSchedule = true;
	if ( m_pCurBehavior )
	{
		return m_pCurBehavior->BridgeSelectSchedule();
	}

	return BaseClass::SelectSchedule();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::KeepRunningBehavior()
{
	if ( m_pCurBehavior )
		m_bCalledBehaviorSelectSchedule = true;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	m_bCalledBehaviorSelectSchedule = true;
	int result = 0;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeSelectFailSchedule( failedSchedule, failedTask, taskFailCode, &result ) )
		return result;
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::StartTask( const Task_t *pTask )
{
	if ( m_pCurBehavior && m_pCurBehavior->BridgeStartTask( pTask ) )
		return;
	BaseClass::StartTask( pTask );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::RunTask( const Task_t *pTask )
{
	if ( m_pCurBehavior && m_pCurBehavior->BridgeRunTask( pTask ) )
		return;
	BaseClass::RunTask( pTask );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::AimGun( void )
{
	if ( m_pCurBehavior && m_pCurBehavior->BridgeAimGun() )
		return;
	BaseClass::AimGun();
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_Schedule *CAI_BehaviorHost<BASE_NPC>::GetSchedule(int localScheduleID)
{
	CAI_Schedule *pResult;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeGetSchedule( localScheduleID, &pResult ) )
		return pResult;
	return BaseClass::GetSchedule( localScheduleID );
}

//-------------------------------------

template <class BASE_NPC>
inline const char *CAI_BehaviorHost<BASE_NPC>::TaskName(int taskID)
{
	const char *pszResult = NULL;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeTaskName( taskID, &pszResult ) )
		return pszResult;
	return BaseClass::TaskName( taskID );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BuildScheduleTestBits()
{
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeBuildScheduleTestBits(); 
	BaseClass::BuildScheduleTestBits();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeHintGroup( string_t oldGroup, string_t newGroup )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnChangeHintGroup( oldGroup, newGroup );
	}
	BaseClass::OnChangeHintGroup( oldGroup, newGroup );
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::BackBridge_NPC_TranslateActivity( Activity activity )
{
	return BaseClass::NPC_TranslateActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::NPC_TranslateActivity( Activity activity )
{
	if ( m_pCurBehavior )
	{
		return m_pCurBehavior->BridgeNPC_TranslateActivity( activity );
	}
	return BaseClass::NPC_TranslateActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsCurTaskContinuousMove()
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeIsCurTaskContinuousMove( &result ) )
		return result;
	return BaseClass::IsCurTaskContinuousMove();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnMovementFailed()
{ 
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeOnMovementFailed(); 
	BaseClass::OnMovementFailed();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnMovementComplete()
{ 
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeOnMovementComplete(); 
	BaseClass::OnMovementComplete();
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::GetDefaultNavGoalTolerance()
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeGetDefaultNavGoalTolerance();
	return BaseClass::GetDefaultNavGoalTolerance();
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::BackBridge_GetDefaultNavGoalTolerance() 
{
	return BaseClass::GetDefaultNavGoalTolerance();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::FValidateHintType( CAI_Hint *pHint )
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeFValidateHintType( pHint, &result ) )
		return result;
	return BaseClass::FValidateHintType( pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidEnemy(CBaseEntity *pEnemy)
{
	return BaseClass::IsValidEnemy( pEnemy );
}

//-------------------------------------

template <class BASE_NPC>
inline CBaseEntity *CAI_BehaviorHost<BASE_NPC>::BackBridge_BestEnemy(void)
{
	return BaseClass::BestEnemy();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	return BaseClass::IsValidCover( vLocation, pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	return BaseClass::IsValidShootPosition( vLocation, pNode, pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::BackBridge_GetMaxTacticalLateralMovement( void )
{
	return BaseClass::GetMaxTacticalLateralMovement();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_ShouldIgnoreSound( CSound *pSound )
{
	return BaseClass::ShouldIgnoreSound( pSound );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_OnSeeEntity( CBaseEntity *pEntity )
{
	BaseClass::OnSeeEntity( pEntity );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker )
{
	BaseClass::OnFriendDamaged( pSquadmate, pAttacker );
}


//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsInterruptable( void )
{
	return BaseClass::IsInterruptable();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsNavigationUrgent( void )
{
	return BaseClass::IsNavigationUrgent();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_CanFlinch( void )
{
	return BaseClass::CanFlinch();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsCrouching( void )
{
	return BaseClass::IsCrouching();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsCrouchedActivity( Activity activity )
{
	return BaseClass::IsCrouchedActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_QueryHearSound( CSound *pSound )
{
	return BaseClass::QueryHearSound( pSound );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_CanRunAScriptedNPCInteraction( bool bForced )
{
	return BaseClass::CanRunAScriptedNPCInteraction( bForced );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_ShouldPlayerAvoid( void )
{
	return BaseClass::ShouldPlayerAvoid();
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::BackBridge_OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	return BaseClass::OnTakeDamage_Alive( info );
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::BackBridge_GetReasonableFacingDist( void )
{
	return BaseClass::GetReasonableFacingDist();
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::BackBridge_GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	return BaseClass::OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_ModifyOrAppendCriteria( AI_CriteriaSet &criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	BaseClass::Teleport( newPosition, newAngles, newVelocity );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidEnemy( pEnemy );
	
	return BaseClass::IsValidEnemy( pEnemy );
}

//-------------------------------------

template <class BASE_NPC>
inline CBaseEntity *CAI_BehaviorHost<BASE_NPC>::BestEnemy()
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeBestEnemy();
	
	return BaseClass::BestEnemy();
}
	
//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ShouldAlwaysThink()
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeShouldAlwaysThink( &result ) )
		return result;
	return BaseClass::ShouldAlwaysThink();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnChangeActiveWeapon( pOldWeapon, pNewWeapon );
	}
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::SpeakMapmakerInterruptConcept( string_t iszConcept )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		if ( m_Behaviors[i]->BridgeSpeakMapmakerInterruptConcept( iszConcept ) )
			return true;
	}

	return false;
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnRestore()
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnRestore();
	}
	BaseClass::OnRestore();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidCover( vLocation, pHint );
	
	return BaseClass::IsValidCover( vLocation, pHint );
}
	
//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidShootPosition( vLocation, pNode, pHint );
	
	return BaseClass::IsValidShootPosition( vLocation, pNode, pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::GetMaxTacticalLateralMovement( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeGetMaxTacticalLateralMovement();

	return BaseClass::GetMaxTacticalLateralMovement();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ShouldIgnoreSound( CSound *pSound )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeShouldIgnoreSound( pSound );
	
	return BaseClass::ShouldIgnoreSound( pSound );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnSeeEntity( CBaseEntity *pEntity )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeOnSeeEntity( pEntity );

	BaseClass::OnSeeEntity( pEntity );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeOnFriendDamaged( pSquadmate, pAttacker );

	BaseClass::OnFriendDamaged( pSquadmate, pAttacker );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsInterruptable( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsInterruptable();
	
	return BaseClass::IsInterruptable();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsNavigationUrgent( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsNavigationUrgent();

	return BaseClass::IsNavigationUrgent();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::CanFlinch( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeCanFlinch();

	return BaseClass::CanFlinch();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsCrouching( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsCrouching();

	return BaseClass::IsCrouching();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsCrouchedActivity( Activity activity )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsCrouchedActivity( activity );

	return BaseClass::IsCrouchedActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::QueryHearSound( CSound *pSound )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeQueryHearSound( pSound );

	return BaseClass::QueryHearSound( pSound );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::CanRunAScriptedNPCInteraction( bool bForced )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeCanRunAScriptedNPCInteraction( bForced );

	return BaseClass::CanRunAScriptedNPCInteraction( bForced );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ShouldPlayerAvoid( void )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeShouldPlayerAvoid();
	
	return BaseClass::ShouldPlayerAvoid();
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeOnTakeDamage_Alive( info );
	
	return BaseClass::OnTakeDamage_Alive( info );
}

//-------------------------------------

template <class BASE_NPC>
inline float CAI_BehaviorHost<BASE_NPC>::GetReasonableFacingDist( void )	
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeGetReasonableFacingDist();
	
	return BaseClass::GetReasonableFacingDist();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::Precache()
{
	BaseClass::Precache();
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgePrecache();
	}
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity )
{
	// If a behavior is active, we need to stop running it
	ChangeBehaviorTo( NULL );

	return BaseClass::ScheduledMoveToGoalEntity( scheduleType, pGoalEntity, movementActivity );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity )
{
	// If a behavior is active, we need to stop running it
	ChangeBehaviorTo( NULL );

	return BaseClass::ScheduledFollowPath( scheduleType, pPathStart, movementActivity );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::ForceSelectedGo(CBaseEntity *pPlayer, const Vector &targetPos, const Vector &traceDir, bool bRun)
{
	// If a behavior is active, we need to stop running it
	ChangeBehaviorTo( NULL );

	BaseClass::ForceSelectedGo(pPlayer, targetPos, traceDir, bRun);
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::ForceSelectedGoRandom(void)
{
	// If a behavior is active, we need to stop running it
	ChangeBehaviorTo( NULL );

	BaseClass::ForceSelectedGoRandom();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::NPCInit()
{
	BaseClass::NPCInit();
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeSpawn();
	}
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::UpdateOnRemove()
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeUpdateOnRemove();
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::Event_Killed( const CTakeDamageInfo &info )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeEvent_Killed( info );
	}
	BaseClass::Event_Killed( info );
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeGetFlinchActivity( bHeavyDamage, bGesture );

	return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeOnCalcBaseMove( pMoveGoal, distClear, pResult );

	return BaseClass::OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::ModifyOrAppendCriteria( AI_CriteriaSet &criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	if ( m_pCurBehavior )
	{
		// Append active behavior name
		criteriaSet.AppendCriteria( "active_behavior", GetRunningBehavior()->GetName() );
		
		m_pCurBehavior->BridgeModifyOrAppendCriteria( criteriaSet );
		return;
	}
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	if ( m_pCurBehavior )
	{
		m_pCurBehavior->BridgeTeleport( newPosition, newAngles, newVelocity );
		return;
	}

	BaseClass::Teleport( newPosition, newAngles, newVelocity );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::HandleAnimEvent( animevent_t *pEvent )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeHandleAnimEvent( pEvent );

	return BaseClass::HandleAnimEvent( pEvent );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	if ( pBehavior == GetRunningBehavior() && !pBehavior->CanSelectSchedule() && !fCanFinishSchedule )
	{
		DeferSchedulingToBehavior( NULL );
		return true;
	}
	return false;
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::AddBehavior( CAI_BehaviorBase *pBehavior )
{
#ifdef DEBUG
	Assert( m_Behaviors.Find( pBehavior ) == m_Behaviors.InvalidIndex() );
	Assert( m_fDebugInCreateBehaviors );
	for ( int i = 0; i < m_Behaviors.Count(); i++)
	{
		Assert( typeid(*m_Behaviors[i]) != typeid(*pBehavior) );
	}
#endif
	m_Behaviors.AddToTail( pBehavior );
	pBehavior->SetOuter( this );
	pBehavior->SetBackBridge( this );
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase **CAI_BehaviorHost<BASE_NPC>::AccessBehaviors()
{
	if (m_Behaviors.Count())
		return m_Behaviors.Base();
	return NULL;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::NumBehaviors()
{
	return m_Behaviors.Count();
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::Save( ISave &save )
{
	int result = BaseClass::Save( save );
	if ( result )
		CAI_BehaviorBase::SaveBehaviors( save, m_pCurBehavior, AccessBehaviors(), NumBehaviors() );
	return result;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::Restore( IRestore &restore )
{
	int result = BaseClass::Restore( restore );
	if ( result )
	{
		int iCurrent = CAI_BehaviorBase::RestoreBehaviors( restore, AccessBehaviors(), NumBehaviors() );
		if ( iCurrent != -1 )
			m_pCurBehavior = AccessBehaviors()[iCurrent];
		else
			m_pCurBehavior = NULL;
	}
	return result;
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::CreateComponents()
{
	if ( BaseClass::CreateComponents() )
	{
#ifdef DEBUG
		m_fDebugInCreateBehaviors = true;
#endif
		bool result = CreateBehaviors();
#ifdef DEBUG
		m_fDebugInCreateBehaviors = false;
#endif
		return result;
	}
	return false;
}

//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_H
