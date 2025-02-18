//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef AI_BEHAVIOR_PASSENGER_COMPANION_H
#define AI_BEHAVIOR_PASSENGER_COMPANION_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior_passenger.h"

class CNPC_PlayerCompanion;

struct VehicleAvoidParams_t
{
	Vector vecStartPos;
	Vector vecGoalPos;
	Vector *pNodePositions;
	int nNumNodes;
	int nDirection;
	int nStartNode;
	int nEndNode;
};

struct FailPosition_t
{
	Vector	vecPosition;
	float	flTime;

	DECLARE_SIMPLE_DATADESC();
};

class CAI_PassengerBehaviorCompanion : public CAI_PassengerBehavior
{
	DECLARE_CLASS( CAI_PassengerBehaviorCompanion, CAI_PassengerBehavior );
	DECLARE_DATADESC()

public:

	CAI_PassengerBehaviorCompanion( void );

	enum
	{
		// Schedules
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE = BaseClass::NEXT_SCHEDULE,
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED,
		SCHED_PASSENGER_ENTER_VEHICLE_PAUSE,
		SCHED_PASSENGER_RANGE_ATTACK1,
		SCHED_PASSENGER_RELOAD,
		SCHED_PASSENGER_EXIT_STUCK_VEHICLE,
		SCHED_PASSENGER_OVERTURNED,
		SCHED_PASSENGER_IMPACT,
		SCHED_PASSENGER_ENTER_VEHICLE_IMMEDIATELY,
		SCHED_PASSENGER_COWER,
		SCHED_PASSENGER_FIDGET,
		NEXT_SCHEDULE,

		// Tasks
		TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT = BaseClass::NEXT_TASK,
		TASK_GET_PATH_TO_NEAR_VEHICLE,
		TASK_PASSENGER_RELOAD,
		TASK_PASSENGER_EXIT_STUCK_VEHICLE,
		TASK_PASSENGER_OVERTURNED,
		TASK_PASSENGER_IMPACT,
		TASK_RUN_TO_VEHICLE_ENTRANCE,
		NEXT_TASK,

		// Conditions
		 
		COND_PASSENGER_CAN_LEAVE_STUCK_VEHICLE = BaseClass::NEXT_CONDITION,
		COND_PASSENGER_WARN_OVERTURNED,
		COND_PASSENGER_WARN_COLLISION,
		COND_PASSENGER_VEHICLE_MOVED_FROM_MARK,
		COND_PASSENGER_CAN_FIDGET,
		COND_PASSENGER_CAN_ENTER_IMMEDIATELY,
		NEXT_CONDITION,
	};

	virtual bool	CanSelectSchedule( void );
	virtual void	Enable( CPropJeepEpisodic *pVehicle, bool bImmediateEnter = false);
	virtual void	GatherConditions( void );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	AimGun( void );
	virtual void	EnterVehicle( void );
	virtual void	ExitVehicle( void );
	virtual void	FinishEnterVehicle( void );
	virtual void	FinishExitVehicle( void );
	virtual void	BuildScheduleTestBits( void );
	virtual Activity NPC_TranslateActivity( Activity activity );
	virtual bool	CanExitVehicle( void );
	virtual bool	IsValidEnemy( CBaseEntity *pEntity );
	virtual void	OnUpdateShotRegulator( void );
	virtual bool	IsNavigationUrgent( void );
	virtual	bool	IsCurTaskContinuousMove( void );
	virtual bool	IsCrouching( void );

private:

	void			SpeakVehicleConditions( void );
	virtual void	OnExitVehicleFailed( void );

	bool	CanFidget( void );
	bool	UseRadialRouteToEntryPoint( const Vector &vecEntryPoint );
	float	GetArcToEntryPoint( const Vector &vecCenterPoint, const Vector &vecEntryPoint, bool &bClockwise );
	int		SelectScheduleInsideVehicle( void );
	int		SelectScheduleOutsideVehicle( void );
	bool	FindPathToVehicleEntryPoint( void );
	bool	CanEnterVehicleImmediately( int *pResultSequence, Vector *pResultPos, QAngle *pResultAngles );
	void	EnterVehicleImmediately( void );

	// ------------------------------------------
	//  Passenger sensing
	// ------------------------------------------

	virtual void	GatherVehicleStateConditions( void );

	float	GetVehicleSpeed( void );
	void	GatherVehicleCollisionConditions( const Vector &localVelocity );

	// ------------------------------------------
	//  Overturned tracking
	// ------------------------------------------
	void	UpdateStuckStatus( void );
	bool	CanExitAtPosition( const Vector &vecTestPos );
	bool	GetStuckExitPos( Vector *vecResult );
	bool	ExitStuckVehicle( void );

	bool			UpdateVehicleEntrancePath( void );
	bool			PointIsWithinEntryFailureRadius( const Vector &vecPosition );
	void			ResetVehicleEntryFailedState( void );
	void			MarkVehicleEntryFailed( const Vector &vecPosition );
	virtual int		FindEntrySequence( bool bNearest = false );
	void			CalculateBodyLean( void );

	float	m_flNextJostleTime;
	float	m_flNextOverturnWarning;	// The next time the NPC may complained about being upside-down
	float	m_flOverturnedDuration;		// Amount of time we've been stuck in the vehicle (unable to exit)
	float	m_flUnseenDuration;			// Amount of time we've been hidden from the player's view

	float	m_flEnterBeginTime;			// Time the NPC started to try and enter the vehicle
	int		m_nExitAttempts;			// Number of times we've attempted to exit the vehicle but failed
	int		m_nVisibleEnemies;			// Keeps a record of how many enemies I know about
	float	m_flLastLateralLean;		// Our last lean value

	CAI_MoveMonitor				m_VehicleMonitor;		// Used to keep track of the vehicle's movement relative to a mark
	CUtlVector<FailPosition_t>	m_FailedEntryPositions;	// Used to keep track of the vehicle's movement relative to a mark

protected:
	virtual int	SelectTransitionSchedule( void );
	
	void	ExtendFidgetDelay( float flDuration );
	bool	CanPlayJostle( bool bLargeJostle );
	
	float	m_flEntraceUpdateTime;
	float	m_flNextEnterAttempt;
	float	m_flNextFidgetTime;
	CHandle< CNPC_PlayerCompanion > m_hCompanion;

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};

#endif // AI_BEHAVIOR_PASSENGER_COMPANION_H
