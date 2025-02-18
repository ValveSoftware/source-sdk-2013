//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_MOVETYPES_H
#define AI_MOVETYPES_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_navtype.h"

class CAI_Path;

//-----------------------------------------------------------------------------
// Debugging code
//
// Use this function to set breakpoints to find out where movement is failing
//
#ifdef DEBUG
extern void DebugNoteMovementFailure();
#define DebugNoteMovementFailureIfBlocked( moveResult ) if ( !IsMoveBlocked( moveResult ) ) ((void)0); else DebugNoteMovementFailure()
#else
#define DebugNoteMovementFailure() ((void)0)
#define DebugNoteMovementFailureIfBlocked( moveResult ) ((void)0)
#endif

enum AIMoveResult_t
{
	AIMR_BLOCKED_ENTITY			= -1,	         // Move was blocked by an entity
	AIMR_BLOCKED_WORLD			= -2,	         // Move was blocked by the world
	AIMR_BLOCKED_NPC			= -3,	         // Move was blocked by an NPC
	AIMR_ILLEGAL				= -4,	         // Move is illegal for some reason

	AIMR_OK						= 0,
	
	AIMR_CHANGE_TYPE,	                         // Locomotion method has changed
};


#ifdef DEBUG
extern AIMoveResult_t DbgResult( AIMoveResult_t result );
#else
inline AIMoveResult_t DbgResult( AIMoveResult_t result ) { return result; } // inline not macro for compiler typing
#endif


//-----------------------------------------------------------------------------
// Movement related constants and base types
//-----------------------------------------------------------------------------
#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
const float AI_EPS_CASTS = 0.3;  // The amount physics and hull cast can disagree
#endif


inline bool IsMoveBlocked( AIMoveResult_t moveResult )
{
	return (moveResult < AIMR_OK );
}

//-------------------------------------

enum StepGroundTest_t
{
	STEP_DONT_CHECK_GROUND = 0,
	STEP_ON_VALID_GROUND,
	STEP_ON_INVALID_GROUND,
};


//-------------------------------------

struct AIMoveTrace_t
{
	AIMoveTrace_t()
	{
		memset( this, 0, sizeof(*this) );
	}
	
	AIMoveResult_t 	fStatus;				// See AIMoveResult_t
	Vector 			vEndPosition;			// The last point that could be moved to
	Vector			vHitNormal;				// The normal of a hit, if any. vec3_origin if none. Can be none even if "hit"
	CBaseEntity* 	pObstruction;			// The obstruction I bumped into (if any)
	float			flTotalDist;
	float 			flDistObstructed; 	// FIXME: This is a strange number. In the case
											// of calling MoveLimit with navtype NAV_GROUND,
											// it represents a 2D distance to the obstruction.
											// In the case of other navtypes, it represents a
											// 3D distance to the obstruction
	Vector 			vJumpVelocity;			// FIXME: Remove this; it's bogus
											// It's only returned by JumpMoveLimit
											// which seems to be a bogus concept to begin with
	float			flStepUpDistance;
};

inline bool IsMoveBlocked( const AIMoveTrace_t &moveTrace )
{
	return (moveTrace.fStatus < AIMR_OK );
}


// Categorizes the blocker and sets the appropriate bits
AIMoveResult_t		AIComputeBlockerMoveResult( CBaseEntity *pBlocker );


//-------------------------------------
// Purpose: Specifies an immediate, localized, straight line movement goal
//-------------------------------------

enum AILocalMoveGoalFlags_t
{
	AILMG_NONE,
	AILMG_TARGET_IS_GOAL		= 0x01,
	AILMG_CONSUME_INTERVAL		= 0x02,
	AILMG_TARGET_IS_TRANSITION 	= 0x04,
	AILMG_NO_STEER 				= 0x08,
	AILMG_NO_AVOIDANCE_PATHS 	= 0x10,
};

struct AILocalMoveGoal_t
{
	AILocalMoveGoal_t()
	{
		memset( this, 0, sizeof(*this) );
	}
	
	// Object of the goal
	Vector			target;

	// The actual move. Note these need not always agree with "target"
	Vector			dir; 
	Vector			facing;
	float			speed;
	
	// The distance maximum distance intended to travel in path length
	float			maxDist;

	// The distance expected to move this think
	float			curExpectedDist;

	Navigation_t	navType;
	CBaseEntity *	pMoveTarget;

	unsigned		flags;

	// The path from which this goal was derived
	CAI_Path *		pPath;

	// The result if a forward probing trace has been done
	bool			bHasTraced;
	AIMoveTrace_t	directTrace;
	AIMoveTrace_t	thinkTrace;

#ifdef DEBUG
	int				solveCookie;
#endif
};

//-------------------------------------

enum AIMotorMoveResult_t
{
	AIM_FAILED,
	AIM_SUCCESS,
	
	// Partial successes
	AIM_PARTIAL_HIT_NPC,
	AIM_PARTIAL_HIT_WORLD,
	AIM_PARTIAL_HIT_TARGET,
	
	AIM_NUM_RESULTS
};

//-----------------------------------------------------------------------------
// Purpose: The set of callbacks used by lower-level movement classes to
//			notify and receive guidance from higher level-classes
//-----------------------------------------------------------------------------

abstract_class IAI_MovementSink
{
public:
	//---------------------------------
	//
	// Queries
	//
	virtual float CalcYawSpeed( void ) = 0;

	//---------------------------------
	//
	// Local navigation notifications, each allows services provider to overridde default result
	//
	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, 
								float distClear, 
								AIMoveResult_t *pResult ) = 0;

	virtual bool OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, 
											float distClear, 
											AIMoveResult_t *pResult ) = 0;

	virtual bool OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, 
									float distClear, 
									AIMoveResult_t *pResult ) = 0;

	virtual bool OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal,
									float distClear, 
									AIMoveResult_t *pResult ) = 0;
	
	virtual bool OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, 
											 float distClear, 
											 AIMoveResult_t *pResult ) = 0;


	virtual bool OnMoveBlocked( AIMoveResult_t *pResult ) = 0;
	
	//---------------------------------
	//
	// Motor notifications, each allows services provider to overridde default result
	//
	virtual bool OnMoveStalled( const AILocalMoveGoal_t &move ) = 0;
	virtual bool OnMoveExecuteFailed( const AILocalMoveGoal_t &move, 
									const AIMoveTrace_t &trace, 
									AIMotorMoveResult_t fMotorResult, 
									AIMoveResult_t *pResult ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Default implementations of IAI_MovementSink
//-----------------------------------------------------------------------------

class CAI_DefMovementSink : public IAI_MovementSink
{
public:
	//---------------------------------
	//
	// Queries
	//
	virtual float CalcYawSpeed( void ) { return -1.0; } 


	//---------------------------------
	//
	// Local navigation notifications, each allows services provider to overridde default result
	//
	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) { return false; }
	virtual bool OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) { return false;	}
	virtual bool OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) { return false;	}
	virtual bool OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) { return false;	}
	virtual bool OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) { return false;	}
	virtual bool OnMoveBlocked( AIMoveResult_t *pResult ) { return false;	}
	
	//---------------------------------
	//
	// Motor notifications, each allows services provider to overridde default result
	//
	virtual bool OnMoveStalled( const AILocalMoveGoal_t &move ) { return false;	}
	virtual bool OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult ) { return false;	}
	
};

//-------------------------------------

class CAI_ProxyMovementSink : public CAI_DefMovementSink
{
public:
	CAI_ProxyMovementSink()
	 :	m_pProxied( NULL )
	{
	}
	
	//---------------------------------
	
	void Init( IAI_MovementSink *pMovementServices ) { m_pProxied = pMovementServices; }
	
	//---------------------------------
	//
	// Queries
	//
	virtual float CalcYawSpeed( void );

	//---------------------------------
	//
	// Local navigation notifications, each allows services provider to overridde default result
	//
	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, 
								float distClear, 
								AIMoveResult_t *pResult );

	virtual bool OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, 
											float distClear, 
											AIMoveResult_t *pResult );
	virtual bool OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, 
									float distClear, 
									AIMoveResult_t *pResult );
	virtual bool OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, 
									float distClear, 
									AIMoveResult_t *pResult );
	virtual bool OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, 
											 float distClear, 
											 AIMoveResult_t *pResult );
	virtual bool OnMoveBlocked( AIMoveResult_t *pResult );
	
	//---------------------------------
	//
	// Motor notifications, each allows services provider to overridde default result
	//
	virtual bool OnMoveStalled( const AILocalMoveGoal_t &move );
	virtual bool OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult );
	
	IAI_MovementSink *m_pProxied;
};

// ----------------------------------------------------------------------------

inline float CAI_ProxyMovementSink::CalcYawSpeed( void )
{
	float result;
	if ( m_pProxied && ( result = m_pProxied->CalcYawSpeed() ) != -1.0 )
		return result;
	return CAI_DefMovementSink::CalcYawSpeed();
}

inline bool CAI_ProxyMovementSink::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnCalcBaseMove( pMoveGoal, distClear, pResult ) )
		return true;
	return CAI_DefMovementSink::OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

inline bool CAI_ProxyMovementSink::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnObstructionPreSteer( pMoveGoal, distClear, pResult ) )
		return true;
	return CAI_DefMovementSink::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

inline bool CAI_ProxyMovementSink::OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnFailedSteer( pMoveGoal, distClear, pResult ) )
		return true;
	return CAI_DefMovementSink::OnFailedSteer( pMoveGoal, distClear, pResult );
}

inline bool CAI_ProxyMovementSink::OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnFailedLocalNavigation( pMoveGoal, distClear, pResult ) )
		return true;
	return CAI_DefMovementSink::OnFailedLocalNavigation( pMoveGoal, distClear, pResult );
}

inline bool CAI_ProxyMovementSink::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnInsufficientStopDist( pMoveGoal, distClear, pResult ) )
		return true;
	return CAI_DefMovementSink::OnInsufficientStopDist( pMoveGoal, distClear, pResult );
}

inline bool CAI_ProxyMovementSink::OnMoveStalled( const AILocalMoveGoal_t &move )
{
	if ( m_pProxied && m_pProxied->OnMoveStalled( move ) )
		return true;
	return CAI_DefMovementSink::OnMoveStalled( move );
}

inline bool CAI_ProxyMovementSink::OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnMoveExecuteFailed( move, trace, fMotorResult, pResult ) )
		return true;
	return CAI_DefMovementSink::OnMoveExecuteFailed( move, trace, fMotorResult, pResult );
}

inline bool CAI_ProxyMovementSink::OnMoveBlocked( AIMoveResult_t *pResult )
{
	if ( m_pProxied && m_pProxied->OnMoveBlocked( pResult ) )
		return true;
	return CAI_DefMovementSink::OnMoveBlocked( pResult );
}

//=============================================================================

#endif // AI_MOVETYPES_H
