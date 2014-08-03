//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: NPC that drives vehicles
//
//=============================================================================//

#ifndef NPC_VEHICLEDRIVER_H
#define NPC_VEHICLEDRIVER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"

class CPropVehicleDriveable;

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_VEHICLEDRIVER_INACTIVE		(1 << 16)

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_VEHICLEDRIVER_INACTIVE = LAST_SHARED_SCHEDULE,
	SCHED_VEHICLEDRIVER_COMBAT_WAIT,
	SCHED_VEHICLEDRIVER_DRIVE_PATH,

	LAST_VEHICLEDRIVER_SCHED,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_VEHICLEDRIVER_GET_PATH = LAST_SHARED_TASK,

	LAST_VEHICLEDRIVER_TASK,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CVehicleWaypoint
{
public:
	CVehicleWaypoint( Vector &pPrevPoint, Vector &pCurPoint, Vector &pNextPoint, Vector &pNextNextPoint )
	{
		splinePoints[0] = pPrevPoint;
		splinePoints[1] = pCurPoint;
		splinePoints[2] = pNextPoint;
		splinePoints[3] = pNextNextPoint;

		RecalculateSpline();
	}

	void RecalculateSpline( void )
	{
		planeWaypoint.normal = (splinePoints[2] - splinePoints[1]);
		VectorNormalize( planeWaypoint.normal );
		planeWaypoint.type = PLANE_ANYZ;
		planeWaypoint.dist = DotProduct( planeWaypoint.normal, splinePoints[2] );
		planeWaypoint.signbits = SignbitsForPlane(&planeWaypoint);
		// TODO: Use the vehicle's absbox
		iInitialPlaneSide = BoxOnPlaneSide( -Vector(32,32,32), Vector(32,32,32), &planeWaypoint );

		// Hackily calculate a length for the spline. Subdivide & measure.
		flSplineLength = 0;
		Vector vecPrev = splinePoints[1];
		const int iDivs = 10;
		for ( int i = 1; i <= iDivs; i++ )
		{
			Vector vecCurr;
			float flT = (float)i / (float)iDivs;
			Catmull_Rom_Spline( splinePoints[0], splinePoints[1], splinePoints[2], splinePoints[3], flT, vecCurr );
			flSplineLength += (vecCurr - vecPrev).Length();
			vecPrev = vecCurr;
		}
	}

	Vector GetPointAt( float flT )
	{
		Vector vecCurr(0,0,0);
		Catmull_Rom_Spline( splinePoints[0], splinePoints[1], splinePoints[2], splinePoints[3], flT, vecCurr );
		return vecCurr;
	}

	Vector GetTangentAt( float flT )
	{
		Vector vecCurr(0,0,0);
		Catmull_Rom_Spline_Tangent( splinePoints[0], splinePoints[1], splinePoints[2], splinePoints[3], flT, vecCurr );
		return vecCurr;
	}

	float GetLength( void )
	{
		return flSplineLength;
	}

public:
	int			iInitialPlaneSide;
	float		flSplineLength;
	Vector		splinePoints[4];
	cplane_t	planeWaypoint;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_VehicleDriver : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_VehicleDriver, CAI_BaseNPC );
public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	CNPC_VehicleDriver( void );
	~CNPC_VehicleDriver( void );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Activate( void );
	virtual void	OnRestore();
	virtual void	UpdateOnRemove( void );
	
	// AI
	void			UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	virtual void	PrescheduleThink( void );
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual int		RangeAttack1Conditions( float flDot, float flDist );
	virtual int		RangeAttack2Conditions( float flDot, float flDist );

	// Driving
	virtual void	DriveVehicle( void );
	virtual bool	OverrideMove( float flInterval );
	bool			OverridePathMove( float flInterval );
	void			CalculatePostPoints( void );
	bool			WaypointReached( void );
	float			GetDefaultNavGoalTolerance();
	void			RecalculateSpeeds( void );
	void			ClearWaypoints( void );
	void			CheckForTeleport( void );

	int				BloodColor( void ) { return DONT_BLEED; }

#ifdef HL2_DLL
	Class_T			Classify( void ) { return CLASS_METROPOLICE; }
#else
	Class_T			Classify( void ) { return CLASS_NONE; }
#endif

	Disposition_t	IRelationType( CBaseEntity *pTarget );

	// Inputs
	void			InputSetDriversMaxSpeed( inputdata_t &inputdata );
	void			InputSetDriversMinSpeed( inputdata_t &inputdata );
	void			InputStartForward( inputdata_t &inputdata );
	void			InputStop( inputdata_t &inputdata );
	void			InputStartFiring( inputdata_t &inputdata );
	void			InputStopFiring( inputdata_t &inputdata );
	void			InputGotoPathCorner( inputdata_t &inputdata );

public:
	string_t		m_iszVehicleName;
	IServerVehicle	*m_pVehicleInterface;
	EHANDLE			m_hVehicleEntity;

	// Path driving
	CVehicleWaypoint	*m_Waypoints[2];
	CVehicleWaypoint	*m_pCurrentWaypoint;
	CVehicleWaypoint	*m_pNextWaypoint;
	Vector				m_vecDesiredVelocity;
	Vector				m_vecDesiredPosition;
	Vector				m_vecPrevPoint;
	Vector				m_vecPrevPrevPoint;
	Vector				m_vecPostPoint;
	Vector				m_vecPostPostPoint;
	float				m_flDistanceAlongSpline;
	float				m_flDriversMaxSpeed;
	float				m_flDriversMinSpeed;

	// Speed
	float				m_flMaxSpeed;		// Maximum speed this driver will go
	float				m_flGoalSpeed;		// Desired speed
	float				m_flInitialSpeed;	
	float				m_flSteering;
};

#endif // NPC_VEHICLEDRIVER_H
