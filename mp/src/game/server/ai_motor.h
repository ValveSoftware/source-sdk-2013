//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_MOTOR_H
#define AI_MOTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_component.h"
#include "ai_navtype.h"
#include "ai_movetypes.h"
#include "AI_Interest_Target.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
enum Navigation_t;
class CAI_PlaneSolver;
class CAI_MoveProbe;
class CAI_Navigator;

#define AI_CALC_YAW_SPEED -1
#define AI_KEEP_YAW_SPEED -2

//-----------------------------------------------------------------------------

float AI_ClampYaw( float yawSpeedPerSec, float current, float target, float time );

//-----------------------------------------------------------------------------
// CAI_Motor
//
// Purpose: Implements the primitive locomotion of AIs. 
//-----------------------------------------------------------------------------

class CAI_Motor : public CAI_Component,
				  public CAI_ProxyMovementSink
{
public:
	CAI_Motor(CAI_BaseNPC *pOuter);
	virtual ~CAI_Motor();

	void Init( IAI_MovementSink *pMovementServices );

	// --------------------------------
	// The current timestep the motor is working on
	// --------------------------------
	float 				GetMoveInterval() 					{ return m_flMoveInterval; }
	float				SetMoveInterval( float flInterval ) { return (m_flMoveInterval = flInterval); }

	// ----------------------------------------------------
	// Translational movement
	// ----------------------------------------------------
	AIMoveResult_t 		MoveNormalExecute( const AILocalMoveGoal_t &move );

	virtual void 		MoveClimbStart( const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw );
	virtual AIMoveResult_t MoveClimbExecute( const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw, int climbNodesLeft );
	virtual void 		MoveClimbStop();

	//---------------------------------


	virtual void 		MoveJumpStart( const Vector &velocity );
	virtual int 		MoveJumpExecute();
	virtual AIMoveResult_t MoveJumpStop();

	virtual void		ResetMoveCalculations();
	virtual	void		MoveStart();
	virtual	void		MoveStop();
	virtual void		MovePaused();
	
	//---------------------------------
	
	float				GetIdealSpeed() const;
	float				GetIdealAccel() const;
	float				GetCurSpeed() const			{ return m_vecVelocity.Length(); }
	const Vector &		GetCurVel() const			{ return m_vecVelocity;			 }

	virtual float		OverrideMaxYawSpeed( Activity activity )	{ return -1; }
	bool				IsDeceleratingToGoal() const				{ return false; }

	//---------------------------------
	// Raw ground step forward to the specifed position
	//

	AIMotorMoveResult_t MoveGroundStep( const Vector &newPos, CBaseEntity *pMoveTarget = NULL, float yaw = -1, bool bAsFarAsCan = true, bool bTestZ = true, AIMoveTrace_t *pTraceResult = NULL );
	
	// ----------------------------------------------------
	// Rotational movement (yaw); goal and speed
	// ----------------------------------------------------
	
	void 				SetYawSpeed( float yawSpeed )		{ m_YawSpeed = yawSpeed; 	}
	float 				GetYawSpeed() const					{ return m_YawSpeed; 		}
	float 				GetIdealYaw() const					{ return m_IdealYaw; 		}
	void 				SetIdealYaw( float idealYaw)		{ m_IdealYaw = idealYaw; 	}
	
	// Set ideal yaw specified as a vector
	void 				SetIdealYaw( const Vector &vecFacing)	{ SetIdealYaw( UTIL_VecToYaw( vecFacing )); }

	// Set ideal yaw based on a specified target
	void 				SetIdealYawToTarget( const Vector &target, float noise = 0.0, float offset = 0.0 );

	// Set the ideal yaw and run the current or specified timestep worth of rotation. Note 
	// it is not correct to call any "update" variant of these methods more
	// than once per think cycle
	void 				SetIdealYawAndUpdate( float idealYaw, float yawSpeed = AI_CALC_YAW_SPEED );
	void 				SetIdealYawAndUpdate( const Vector &vecFacing, float yawSpeed = AI_CALC_YAW_SPEED ) 		{ SetIdealYawAndUpdate( UTIL_VecToYaw( vecFacing ), yawSpeed );	}
	void 				SetIdealYawToTargetAndUpdate( const Vector &target, float yawSpeed = AI_CALC_YAW_SPEED );

	//   Add multiple facing goals while moving/standing still.
	virtual void		AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual float		GetFacingDirection( Vector &vecDir );

	// Force the heading to the ideal yaw
	void 				SnapYaw()	{ UpdateYaw(360); }

	// Run the current or specified timestep worth of rotation
	virtual	void		UpdateYaw( int speed = -1 );

	// 
	virtual	void 		RecalculateYawSpeed();

	// Returns the difference ( in degrees ) between npc's current yaw and ideal_yaw
	float				DeltaIdealYaw(); 

	// Issues turn gestures when needed due to turning
	virtual	void		MaintainTurnActivity( void ) { };
	virtual bool		AddTurnGesture( float flYD ) { return false; };

	// --------------------------------
	// Move primitives
	// --------------------------------
	virtual float		MinStoppingDist( float flMinResult = 10.0 );	// how far before I can come to a complete stop?
	virtual float		MinCheckDist();		// how far should I look ahead in my route?

	//---------------------------------

	CAI_Navigator		*GetNavigator( void );
	int					SelectWeightedSequence( Activity activity );
	float				GetSequenceGroundSpeed( int iSequence );

	float				CalcIntervalMove();

	// Yaw locking
	bool	IsYawLocked( void ) const { return m_bYawLocked; }
	void	SetYawLocked( bool state ) { m_bYawLocked = state; }

protected:

	//
	// Common services provided by CAI_BaseNPC, Convenience methods to simplify derived code
	//
	CAI_MoveProbe *		GetMoveProbe()										{ return m_pMoveProbe; }
	void 				SetSmoothedVelocity(const Vector &vecVelocity);
	Vector 				GetSmoothedVelocity();
	float				CalcIdealYaw( const Vector &vecTarget );
	float				SetBoneController ( int iController, float flValue );
	float 				GetSequenceMoveYaw( int iSequence );
	void				SetPlaybackRate( float flRate );
	float				GetPlaybackRate(); //get
	float				SetPoseParameter( const char *szName, float flValue );
	float				SetPoseParameter( int iParameter, float flValue );
	float				GetPoseParameter( const char *szName );
	bool				HasPoseParameter( int iSequence, const char *szName );
	bool				HasPoseParameter( int iSequence, int iParameter );
	void				SetMoveType( MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );
	float				StepHeight() const;
	bool				CanStandOn( CBaseEntity *pSurface ) const;
	
	// ----------------------------------------------------
	// Primitives
	// ----------------------------------------------------
	
	virtual void		MoveFacing( const AILocalMoveGoal_t &move );

	virtual AIMotorMoveResult_t MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );
	AIMotorMoveResult_t			MoveGroundExecuteWalk( const AILocalMoveGoal_t &move, float speed, float dist, AIMoveTrace_t *pTraceResult );
	virtual AIMotorMoveResult_t MoveFlyExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );
	
protected: // made protected while animation transition details worked out, private:

	// --------------------------------
	void				SetMoveVel(const Vector &velocity)		{ m_vecVelocity = velocity; }
	float				IdealVelocity();		// how fast should I be moving in an ideal state?

	// --------------------------------
	float 				m_flMoveInterval;

	float				m_IdealYaw;
	float				m_YawSpeed;

	Vector				m_vecVelocity;
	Vector				m_vecAngularVelocity;

	// --------------------------------

	int					m_nDismountSequence;
	Vector				m_vecDismount;

	// --------------------------------

	CAI_InterestTarget	m_facingQueue;

	// --------------------------------
	
	CAI_MoveProbe *		m_pMoveProbe;

	bool				m_bYawLocked;

	//---------------------------------
public:
	DECLARE_SIMPLE_DATADESC();
};

//=============================================================================

#endif // AI_MOTOR_H
