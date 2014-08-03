//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_LOCALNAVIGATOR_H
#define AI_LOCALNAVIGATOR_H

#include "simtimer.h"
#include "ai_component.h"
#include "ai_movetypes.h"
#include "ai_obstacle_type.h"

#if defined( _WIN32 )
#pragma once
#endif

class CAI_PlaneSolver;
class CAI_MoveProbe;

//-----------------------------------------------------------------------------
// CAI_LocalNavigator
//
// Purpose: Handles all the immediate tasks of navigation, independent of
//			path. Implements steering.
//-----------------------------------------------------------------------------

class CAI_LocalNavigator : public CAI_Component,
						   public CAI_ProxyMovementSink
{
public:
	CAI_LocalNavigator(CAI_BaseNPC *pOuter);
	virtual ~CAI_LocalNavigator();

	void Init( IAI_MovementSink *pMovementServices );

	//---------------------------------
	
	AIMoveResult_t		MoveCalc( AILocalMoveGoal_t *pResult, bool bPreviouslyValidated = false );
	void				ResetMoveCalculations();

	//---------------------------------
	
	void 				AddObstacle( const Vector &pos, float radius, AI_MoveSuggType_t type = AIMST_AVOID_OBJECT );
	bool				HaveObstacles();

protected:

	AIMoveResult_t		MoveCalcRaw( AILocalMoveGoal_t *pResult, bool bOnlyCurThink );
	bool 				MoveCalcDirect( AILocalMoveGoal_t *pMoveGoal, bool bOnlyCurThink, float *pDistClear, AIMoveResult_t *pResult );
	bool 				MoveCalcSteer(  AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	bool		 		MoveCalcStop( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	CAI_MoveProbe *		GetMoveProbe()		  { return m_pMoveProbe; }
	const CAI_MoveProbe *GetMoveProbe() const { return m_pMoveProbe; }

private:

	// --------------------------------

	bool				m_fLastWasClear;
	AILocalMoveGoal_t	m_LastMoveGoal;
	CSimpleSimTimer		m_FullDirectTimer;
	
	CAI_PlaneSolver *	m_pPlaneSolver;
	CAI_MoveProbe *		m_pMoveProbe;

	DECLARE_SIMPLE_DATADESC();
};

#endif // AI_LOCALNAVIGATOR_H
