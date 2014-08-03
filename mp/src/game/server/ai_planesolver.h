//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_PLANE_SOLVER_H
#define AI_PLANE_SOLVER_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "ai_movesolver.h"
#include "ehandle.h"
#include "mathlib/vector.h"
#include "simtimer.h"
#include "ai_navtype.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Vector2D;
class CBaseEntity;
struct edict_t;
class CAI_BaseNPC;
class CAI_Motor;
class CAI_Navigator;
struct AILocalMoveGoal_t;
struct AIMoveTrace_t;

//-------------------------------------

enum AI_SuggestorResult_t
{
	SR_NONE,
	SR_OK,
	SR_FAIL
};

class CAI_PlaneSolver
{
public:
	// constructor
	CAI_PlaneSolver( CAI_BaseNPC *pNpc );
	
	// Attempt to find a valid move direction for the specifed goal
	bool Solve( const AILocalMoveGoal_t &goal, float distClear, Vector *pSolution );

	float CalcProbeDist( float speed );
	
	// Flush any cached results (e.g., hull changed, results not valid)
	void Reset();
	
	void AddObstacle( const Vector &pos, float radius, CBaseEntity *pEntity = NULL, AI_MoveSuggType_t type = AIMST_AVOID_OBJECT );
	bool HaveObstacles()	{ return ( m_Obstacles.Count() != 0 ); }

private:

	enum 
	{
		DEGREES_POSITIVE_ARC				   = 270,
		DEGREES_POSITIVE_ARC_CLOSE_OBSTRUCTION = 340,
		NUM_PROBES 							   = 5
	};

	// How far ahead does the ground solver look (seconds)
	float GetLookaheadTime() 		{ return 1.0; }

	// --------------------------------

	// For debugging purposes
	void VisualizeRegulations();
	void VisualizeSolution( const Vector &vecGoal, const Vector& vecActual );

	// --------------------------------
	bool MoveLimit( Navigation_t navType, const Vector &target, bool ignoreTransients, bool fCheckStep, AIMoveTrace_t *pMoveTrace );
	bool MoveLimit( Navigation_t navType, const Vector &target, bool ignoreTransients, bool fCheckStep, int contents, AIMoveTrace_t *pMoveTrace );

	//-----------------------------------------------------------------------------
	// Adjust the solution for fliers
	//-----------------------------------------------------------------------------
	void AdjustSolutionForFliers( const AILocalMoveGoal_t &goal, float flSolutionYaw, Vector *pSolution );

	// --------------------------------
	// Convenience accessors
	
	CAI_BaseNPC * 	GetNpc();
	CAI_Motor * 	GetMotor();
	const Vector &	GetLocalOrigin();

	// --------------------------------
	
	void				 GenerateObstacleNpcs( const AILocalMoveGoal_t &goal, float probeDist );
	AI_SuggestorResult_t GenerateObstacleSuggestions( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace, float distClear, float probeDist, float degreesToProbe, int nProbes );
	AI_SuggestorResult_t GenerateObstacleSuggestion( const AILocalMoveGoal_t &goal, float yawScanCenter, float probeDist, float spanPerProbe, int probeOffset );
	void				 GenerateSuggestionFromTrace( const AILocalMoveGoal_t &goal,
													  const AIMoveTrace_t &moveTrace, float probeDist, 
													  float arcCenter, float arcSpan, int probeOffset );
	bool				 GenerateCircleObstacleSuggestions( const AILocalMoveGoal_t &moveGoal, float probeDist );

	void				 CalcYawsFromOffset( float yawScanCenter, float spanPerProbe, int probeOffset,
											 float *pYawTest, float *pYawCenter );
	
	float CalculateRegulationWeight( const AIMoveTrace_t &moveTrace, float pctBlockedt );
	float AdjustRegulationWeight( CBaseEntity *pEntity, float weight );
	unsigned ComputeTurnBiasFlags( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace );


	bool RunMoveSolver( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace, 
						float degreesPositiveArc, bool fDeterOscillation, 
						Vector *pResult );
	bool DetectUnsolvable( const AILocalMoveGoal_t &goal );

	// --------------------------------
	
	CAI_BaseNPC *		m_pNpc;

	Vector				m_PrevTarget;
	bool				m_fSolvedPrev;
	float 				m_PrevSolution;
	Vector				m_PrevSolutionVector;
	
	float				m_ClosestHaveBeenToCurrent;
	float 				m_TimeLastProgress;

	bool				m_fCannotSolveCurrent;
	
	CSimTimer			m_RefreshSamplesTimer;
	
	// --------------------------------
	
	struct CircleObstacles_t
	{
		CircleObstacles_t( const Vector &center, float radius, CBaseEntity *pEntity, AI_MoveSuggType_t type )
		 :	center(center), 
			radius(radius), 
			hEntity(pEntity),
			type(type)
		{
		}
		
		Vector				center;
		float				radius;
		AI_MoveSuggType_t 	type;
		EHANDLE				hEntity;
	};
	
	CUtlVector<CircleObstacles_t> m_Obstacles;
	
	// --------------------------------
	
	CAI_MoveSolver m_Solver;
};
	
//-------------------------------------

inline void CAI_PlaneSolver::Reset()
{
	m_RefreshSamplesTimer.Force();

	m_fSolvedPrev = false;
	m_PrevTarget.Init( FLT_MAX, FLT_MAX, FLT_MAX ),
	m_PrevSolution = 0;
	m_ClosestHaveBeenToCurrent = FLT_MAX;
	m_TimeLastProgress = FLT_MAX;
	m_fCannotSolveCurrent = false;
}
	
//=============================================================================

#endif // AI_PLANE_SOLVER_H
