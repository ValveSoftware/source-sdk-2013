//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "ai_localnavigator.h"

#include "ai_basenpc.h"
#include "ai_planesolver.h"
#include "ai_moveprobe.h"
#include "ai_motor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_debug_directnavprobe("ai_debug_directnavprobe", "0");

const float TIME_DELAY_FULL_DIRECT_PROBE[2] = { 0.25, 0.35 };

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC(CAI_LocalNavigator)
	//						m_fLastWasClear	(not saved)
	//						m_LastMoveGoal	(not saved)
	//						m_FullDirectTimer	(not saved)
	//						m_pPlaneSolver	(not saved)
	//						m_pMoveProbe	(not saved)
END_DATADESC();

//-------------------------------------

CAI_LocalNavigator::CAI_LocalNavigator(CAI_BaseNPC *pOuter) : CAI_Component( pOuter ) 
{
	m_pMoveProbe = NULL;
	m_pPlaneSolver = new CAI_PlaneSolver( pOuter );

	m_fLastWasClear = false;
	memset( &m_LastMoveGoal, 0, sizeof(m_LastMoveGoal) );
}

//-------------------------------------

CAI_LocalNavigator::~CAI_LocalNavigator() 
{
	delete m_pPlaneSolver;
}

//-------------------------------------

void CAI_LocalNavigator::Init( IAI_MovementSink *pMovementServices )	
{ 
	CAI_ProxyMovementSink::Init( pMovementServices );
	m_pMoveProbe = GetOuter()->GetMoveProbe(); // @TODO (toml 03-30-03): this is a "bad" way to grab this pointer. Components should have an explcit "init" phase.
}

//-------------------------------------

void CAI_LocalNavigator::ResetMoveCalculations()
{
	m_FullDirectTimer.Force();
	m_pPlaneSolver->Reset();
}

//-------------------------------------

void CAI_LocalNavigator::AddObstacle( const Vector &pos, float radius, AI_MoveSuggType_t type )
{
	m_pPlaneSolver->AddObstacle( pos, radius, NULL, type );
}

//-------------------------------------

bool CAI_LocalNavigator::HaveObstacles()
{
	return m_pPlaneSolver->HaveObstacles();
}

//-------------------------------------

bool CAI_LocalNavigator::MoveCalcDirect( AILocalMoveGoal_t *pMoveGoal, bool bOnlyCurThink, float *pDistClear, AIMoveResult_t *pResult )
{
	AI_PROFILE_SCOPE(CAI_LocalNavigator_MoveCalcDirect);

	bool bRetVal = false;
	
	if ( pMoveGoal->speed )
	{
		CAI_Motor *pMotor = GetOuter()->GetMotor();
		float  minCheckDist = pMotor->MinCheckDist();
		float  probeDist	= m_pPlaneSolver->CalcProbeDist( pMoveGoal->speed ); // having this match steering allows one fewer traces
		float  checkDist	= MAX( minCheckDist, probeDist );
		float  checkStepDist = MAX( 16.0, probeDist * 0.5 );

		if ( pMoveGoal->flags & ( AILMG_TARGET_IS_TRANSITION | AILMG_TARGET_IS_GOAL ) )
		{
			// clamp checkDist to be no farther than max distance to goal
			checkDist = MIN( checkDist, pMoveGoal->maxDist );
		}

		if ( checkDist <= 0.0 )
		{
			*pResult = AIMR_OK;
			return true;
		}

		float moveThisInterval = pMotor->CalcIntervalMove();
		bool bExpectingArrival = (moveThisInterval >= checkDist);

		if ( !m_FullDirectTimer.Expired() )
		{
			if ( !m_fLastWasClear || 
				 ( !VectorsAreEqual(pMoveGoal->target, m_LastMoveGoal.target, 0.1) || 
				   !VectorsAreEqual(pMoveGoal->dir, m_LastMoveGoal.dir, 0.1) ) ||
				 bExpectingArrival )
			{
				m_FullDirectTimer.Force();
			}
		}

		if ( bOnlyCurThink ) // Outer code claims to have done a validation (probably a simplify operation)
		{
			m_FullDirectTimer.Set( TIME_DELAY_FULL_DIRECT_PROBE[AIStrongOpt()] );
		}

		// First, check the probable move for this cycle
		bool bTraceClear = true;
		Vector testPos;

		if ( !bExpectingArrival )
		{
			testPos = GetLocalOrigin() + pMoveGoal->dir * moveThisInterval;
			bTraceClear = GetMoveProbe()->MoveLimit( pMoveGoal->navType, GetLocalOrigin(), testPos, 
													 MASK_NPCSOLID, pMoveGoal->pMoveTarget, 
													 100.0, 
													 ( pMoveGoal->navType == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT, 
													 &pMoveGoal->directTrace );

			if ( !bTraceClear )
			{
				// Adjust probe top match expected probe dist (relied on later in process)
				pMoveGoal->directTrace.flDistObstructed = (checkDist - moveThisInterval) + pMoveGoal->directTrace.flDistObstructed;

			}

			if ( !IsRetail() && ai_debug_directnavprobe.GetBool() )
			{
				if ( !bTraceClear )
				{
					DevMsg( GetOuter(), "Close obstruction %f\n", checkDist - pMoveGoal->directTrace.flDistObstructed );
					NDebugOverlay::Line( WorldSpaceCenter(), Vector( testPos.x, testPos.y, WorldSpaceCenter().z ), 255, 0, 0, false, 0.1 );
					if ( pMoveGoal->directTrace.pObstruction )
						NDebugOverlay::Line( WorldSpaceCenter(), pMoveGoal->directTrace.pObstruction->WorldSpaceCenter(), 255, 0, 255, false, 0.1 );

				}
				else
				{
					NDebugOverlay::Line( WorldSpaceCenter(), Vector( testPos.x, testPos.y, WorldSpaceCenter().z ), 0, 255, 0, false, 0.1 );
				}
			}

			pMoveGoal->thinkTrace = pMoveGoal->directTrace;
		}

		// Now project out for future obstructions
		if ( bTraceClear )
		{
			if ( m_FullDirectTimer.Expired() )
			{
				testPos = GetLocalOrigin() + pMoveGoal->dir * checkDist;
				float checkStepPct = (checkStepDist / checkDist) * 100.0;
				if ( checkStepPct > 100.0 )
					checkStepPct = 100.0;
				
				bTraceClear = GetMoveProbe()->MoveLimit( pMoveGoal->navType, GetLocalOrigin(), testPos, 
														 MASK_NPCSOLID, pMoveGoal->pMoveTarget, 
														 checkStepPct, 
														 ( pMoveGoal->navType == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT, 
														 &pMoveGoal->directTrace );
				if ( bExpectingArrival )
					pMoveGoal->thinkTrace = pMoveGoal->directTrace;

				if (ai_debug_directnavprobe.GetBool() )
				{
					if ( !bTraceClear )
					{
						NDebugOverlay::Line( GetOuter()->EyePosition(), Vector( testPos.x, testPos.y, GetOuter()->EyePosition().z ), 255, 0, 0, false, 0.1 );
						DevMsg( GetOuter(), "Obstruction %f\n", checkDist - pMoveGoal->directTrace.flDistObstructed );
					}
					else
					{
						NDebugOverlay::Line( GetOuter()->EyePosition(), Vector( testPos.x, testPos.y, GetOuter()->EyePosition().z ), 0, 255, 0, false, 0.1 );
						DevMsg( GetOuter(), "No obstruction\n" );
					}
				}
			}
			else
			{
				if ( ai_debug_directnavprobe.GetBool() )
					DevMsg( GetOuter(), "No obstruction (Near probe only)\n" );
			}
		}

		pMoveGoal->bHasTraced = true;
	
		float distClear = checkDist - pMoveGoal->directTrace.flDistObstructed;
		if (distClear < 0.001)
			distClear = 0;
		
		if ( bTraceClear )
		{
			*pResult = AIMR_OK;
			bRetVal = true;
			m_fLastWasClear = true;
		}
		else if ( ( pMoveGoal->flags & ( AILMG_TARGET_IS_TRANSITION | AILMG_TARGET_IS_GOAL ) ) && 
			 pMoveGoal->maxDist < distClear )
		{
			*pResult = AIMR_OK;
			bRetVal = true;
			m_fLastWasClear = true;
		}
		else
		{
			*pDistClear = distClear;
			m_fLastWasClear = false;
		}
	}
	else
	{
		// Should never end up in this function with speed of zero. Probably an activity problem.
		*pResult = AIMR_ILLEGAL;
		bRetVal = true;
	}

	m_LastMoveGoal = *pMoveGoal;
	if ( bRetVal && m_FullDirectTimer.Expired() )
		m_FullDirectTimer.Set( TIME_DELAY_FULL_DIRECT_PROBE[AIStrongOpt()] );

	return bRetVal;
}

//-------------------------------------

ConVar ai_no_steer( "ai_no_steer", "0" );

bool CAI_LocalNavigator::MoveCalcSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( (pMoveGoal->flags & AILMG_NO_STEER) )
		return false;

	if ( ai_no_steer.GetBool() )
		return false;

	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	AI_PROFILE_SCOPE(CAI_Motor_MoveCalcSteer);
	Vector moveSolution;
	if ( m_pPlaneSolver->Solve( *pMoveGoal, distClear, &moveSolution ) )
	{
		if ( moveSolution != pMoveGoal->dir )
		{
			float dot = moveSolution.AsVector2D().Dot( pMoveGoal->dir.AsVector2D() );

			const float COS_HALF_30 = 0.966;
			if ( dot > COS_HALF_30 )
			{
				float probeDist = m_pPlaneSolver->CalcProbeDist( pMoveGoal->speed );
				if ( pMoveGoal->maxDist < probeDist * 0.33333 && distClear > probeDist * 0.6666)
				{
					// A waypoint is coming up, but there's probably time to steer
					// away after hitting it
					*pResult = AIMR_OK;
					return true;
				}
			}

			pMoveGoal->facing = pMoveGoal->dir = moveSolution;
		}
		*pResult = AIMR_OK;
		return true;
	}
		
	return false;
}

//-------------------------------------

bool CAI_LocalNavigator::MoveCalcStop( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if (distClear < pMoveGoal->maxDist)
	{
		if ( distClear < 0.1 )
		{
			DebugNoteMovementFailure();
			*pResult = AIMR_ILLEGAL;
		}
		else
		{
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
		}

		return true;
	}
	*pResult = AIMR_OK;
	return true;
}

//-------------------------------------

#ifdef DEBUG
#define SetSolveCookie() 	pMoveGoal->solveCookie = __LINE__;
#else
#define SetSolveCookie()	((void)0)
#endif


AIMoveResult_t CAI_LocalNavigator::MoveCalcRaw( AILocalMoveGoal_t *pMoveGoal, bool bOnlyCurThink )
{
	AI_PROFILE_SCOPE(CAI_Motor_MoveCalc);
	
	AIMoveResult_t result = AIMR_OK; // Assume success
	AIMoveTrace_t  directTrace;
	float	   	   distClear;
	
	// --------------------------------------------------

	bool bDirectClear = MoveCalcDirect( pMoveGoal, bOnlyCurThink, &distClear, &result);
	if ( OnCalcBaseMove( pMoveGoal, distClear, &result ) )
	{
		SetSolveCookie();
		return DbgResult( result );
	}

	bool bShouldSteer = ( !(pMoveGoal->flags & AILMG_NO_STEER) && ( !bDirectClear || HaveObstacles() ) );

	if ( bDirectClear && !bShouldSteer )
	{
		SetSolveCookie();
		return DbgResult( result );
	}
	
	// --------------------------------------------------

	if ( bShouldSteer )
	{
		if ( !bDirectClear )
		{
			if ( OnObstructionPreSteer( pMoveGoal, distClear, &result ) )
			{
				SetSolveCookie();
				return DbgResult( result );
			}
		}

		if ( MoveCalcSteer( pMoveGoal, distClear, &result ) )
		{
			SetSolveCookie();
			return DbgResult( result );			
		}
	}

	if ( OnFailedSteer( pMoveGoal, distClear, &result ) )
	{
		SetSolveCookie();
		return DbgResult( result );
	}

	// --------------------------------------------------
	
	if ( OnFailedLocalNavigation( pMoveGoal, distClear, &result ) )
	{
		SetSolveCookie();
		return DbgResult( result );
	}

	if ( distClear < GetOuter()->GetMotor()->MinStoppingDist() )
	{
		if ( OnInsufficientStopDist( pMoveGoal, distClear, &result ) )
		{
			SetSolveCookie();
			return DbgResult( result );
		}

		if ( MoveCalcStop( pMoveGoal, distClear, &result) )
		{
			SetSolveCookie();
			return DbgResult( result );
		}
	}

	// A hopeful result... may get in trouble at next waypoint and obstruction is still there
	if ( distClear > pMoveGoal->curExpectedDist )
	{
		SetSolveCookie();
		return DbgResult( AIMR_OK );
	}

	// --------------------------------------------------

	DebugNoteMovementFailure();
	SetSolveCookie();
	return DbgResult( IsMoveBlocked( pMoveGoal->directTrace.fStatus ) ? pMoveGoal->directTrace.fStatus : AIMR_ILLEGAL );
}

//-------------------------------------

AIMoveResult_t CAI_LocalNavigator::MoveCalc( AILocalMoveGoal_t *pMoveGoal, bool bPreviouslyValidated )
{
	bool bOnlyCurThink = ( bPreviouslyValidated && !HaveObstacles() );

	AIMoveResult_t result = MoveCalcRaw( pMoveGoal, bOnlyCurThink );

	if ( pMoveGoal->curExpectedDist > pMoveGoal->maxDist )
		pMoveGoal->curExpectedDist = pMoveGoal->maxDist;

	// If success, try to dampen really fast turning movement
	if ( result == AIMR_OK)
	{
		float interval = GetOuter()->GetMotor()->GetMoveInterval();
		float currentYaw = UTIL_AngleMod( GetLocalAngles().y );
		float goalYaw;
		float deltaYaw;
		float speed;
		float clampedYaw;

		// Clamp yaw
		goalYaw = UTIL_VecToYaw( pMoveGoal->facing );
		deltaYaw = fabs( UTIL_AngleDiff( goalYaw, currentYaw ) );
		if ( deltaYaw > 15 )
		{
			speed = deltaYaw * 4.0; // i.e., any maneuver takes a quarter a second
			clampedYaw = AI_ClampYaw( speed, currentYaw, goalYaw, interval );

			if ( clampedYaw != goalYaw )
			{
				pMoveGoal->facing = UTIL_YawToVector( clampedYaw );
			}
		}
	}
	
	return result;
}
//-----------------------------------------------------------------------------
