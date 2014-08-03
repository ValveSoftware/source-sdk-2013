//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <float.h> // for FLT_MAX

#include "ai_planesolver.h"
#include "ai_moveprobe.h"
#include "ai_motor.h"
#include "ai_basenpc.h"
#include "ai_route.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

const float PLANE_SOLVER_THINK_FREQUENCY[2] = { 0.0f, 0.2f };
const float MAX_PROBE_DIST[2] = { (10.0f*12.0f), (8.0f*12.0f) };

//#define PROFILE_PLANESOLVER 1

#ifdef PROFILE_PLANESOLVER
#define PLANESOLVER_PROFILE_SCOPE( tag ) AI_PROFILE_SCOPE( tag )
#else
#define PLANESOLVER_PROFILE_SCOPE( tag ) ((void)0)
#endif

#define ProbeForNpcs() 0

//#define TESTING_SUGGESTIONS

//-----------------------------------------------------------------------------

inline float sq( float f )
{
	return ( f * f );
}

inline float cube( float f )
{
	return ( f * f * f );
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CAI_PlaneSolver::CAI_PlaneSolver( CAI_BaseNPC *pNpc ) 
 :	m_pNpc( pNpc ),
	m_fSolvedPrev( false ),
	m_PrevTarget( FLT_MAX, FLT_MAX, FLT_MAX ),
	m_PrevSolution( 0 ),
	m_ClosestHaveBeenToCurrent( FLT_MAX ),
	m_TimeLastProgress( FLT_MAX ),
	m_fCannotSolveCurrent( false ),
	m_RefreshSamplesTimer( PLANE_SOLVER_THINK_FREQUENCY[AIStrongOpt()] - 0.05 )
{
}


//-----------------------------------------------------------------------------
// Convenience accessors
//-----------------------------------------------------------------------------
inline CAI_BaseNPC *CAI_PlaneSolver::GetNpc()
{
	return m_pNpc;
}

inline CAI_Motor *CAI_PlaneSolver::GetMotor()
{
	return m_pNpc->GetMotor();
}

inline const Vector &CAI_PlaneSolver::GetLocalOrigin()
{
	return m_pNpc->GetLocalOrigin();
}

//-----------------------------------------------------------------------------
// class CAI_PlaneSolver
//-----------------------------------------------------------------------------

bool CAI_PlaneSolver::MoveLimit( Navigation_t navType, const Vector &target, bool ignoreTransients, bool fCheckStep, int contents, AIMoveTrace_t *pMoveTrace )
{
	AI_PROFILE_SCOPE( CAI_PlaneSolver_MoveLimit );

	int flags = ( navType == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT;

	if ( ignoreTransients )
	{
		Assert( !ProbeForNpcs() );
		flags |= AIMLF_IGNORE_TRANSIENTS;
	}

	CAI_MoveProbe *pProbe = m_pNpc->GetMoveProbe();
	return pProbe->MoveLimit( navType, GetLocalOrigin(), target, contents, 
		m_pNpc->GetNavTargetEntity(), (fCheckStep) ? 100 : 0, 
							  flags, 
							  pMoveTrace );
}

bool CAI_PlaneSolver::MoveLimit( Navigation_t navType, const Vector &target, bool ignoreTransients, bool fCheckStep, AIMoveTrace_t *pMoveTrace )
{
	return MoveLimit( navType, target, ignoreTransients, fCheckStep, MASK_NPCSOLID, pMoveTrace );
}

//-----------------------------------------------------------------------------

bool CAI_PlaneSolver::DetectUnsolvable( const AILocalMoveGoal_t &goal )
{
#ifndef TESTING_SUGGESTIONS
	float curDistance = ( goal.target.AsVector2D() - GetLocalOrigin().AsVector2D() ).Length();
	if ( m_PrevTarget != goal.target )
	{
		m_TimeLastProgress = gpGlobals->curtime;
		m_ClosestHaveBeenToCurrent = curDistance;
		m_fCannotSolveCurrent = false;
	}
	else
	{
		if ( m_fCannotSolveCurrent )
		{
			return true;
		}

		if ( m_ClosestHaveBeenToCurrent - curDistance > 0 )
		{
			m_TimeLastProgress = gpGlobals->curtime;
			m_ClosestHaveBeenToCurrent = curDistance;
		}
		else
		{
			if ( gpGlobals->curtime - m_TimeLastProgress > 0.75 )
			{
				m_fCannotSolveCurrent = true;
				return true;
			}
		}
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------

float CAI_PlaneSolver::AdjustRegulationWeight( CBaseEntity *pEntity, float weight )
{
	if ( pEntity->MyNPCPointer() != NULL )
	{
		// @TODO (toml 10-03-02): How to do this with non-NPC entities. Should be using intended solve velocity...
		Vector2D velOwner = GetNpc()->GetMotor()->GetCurVel().AsVector2D();
		Vector2D velBlocker = ((CAI_BaseNPC *)pEntity)->GetMotor()->GetCurVel().AsVector2D();

		Vector2D velOwnerNorm = velOwner;
		Vector2D velBlockerNorm = velBlocker;

		float speedOwner   = Vector2DNormalize( velOwnerNorm );
		float speedBlocker = Vector2DNormalize( velBlockerNorm );

		float dot = velOwnerNorm.Dot( velBlockerNorm );

		if ( speedBlocker > 0 )
		{
			if ( dot > 0 && speedBlocker >= speedOwner * 0.9 )
			{
				if ( dot > 0.86 )
				{
					// @Note (toml 10-10-02): Even in the case of no obstacle, we generate
					// a suggestion in because we still want to continue sweeping the
					// search
					weight = 0;
				}
				else if ( dot > 0.7 )
				{
					weight *= sq( weight );
				}
				else 
					weight *= weight;
			}
		}
	}

	return weight;
}

//-----------------------------------------------------------------------------

float CAI_PlaneSolver::CalculateRegulationWeight( const AIMoveTrace_t &moveTrace, float pctBlocked )
{
	float weight = 0;
	
	if ( pctBlocked > 0.9)
		weight = 1;
	else if ( pctBlocked < 0.1)
		weight = 0;
	else
	{
		weight = sq( ( pctBlocked - 0.1 ) / 0.8 );
		weight = AdjustRegulationWeight( moveTrace.pObstruction, weight );
	}

	return weight;
}

//-----------------------------------------------------------------------------

void CAI_PlaneSolver::GenerateSuggestionFromTrace( const AILocalMoveGoal_t &goal, 
												   const AIMoveTrace_t &moveTrace, float probeDist, 
												   float arcCenter, float arcSpan, int probeOffset )
{
	AI_MoveSuggestion_t suggestion;
	AI_MoveSuggType_t type;

	switch ( moveTrace.fStatus )
	{
		case AIMR_BLOCKED_ENTITY:	type = AIMST_AVOID_OBJECT;	break;
		case AIMR_BLOCKED_WORLD:	type = AIMST_AVOID_WORLD;	break;
		case AIMR_BLOCKED_NPC:		type = AIMST_AVOID_NPC;		break;
		case AIMR_ILLEGAL:			type = AIMST_AVOID_DANGER;	break;
		default:					type = AIMST_NO_KNOWLEDGE;	AssertMsg( 0, "Unexpected mode status" ); break; 
	}

	if ( goal.pMoveTarget != NULL && goal.pMoveTarget == moveTrace.pObstruction )
	{
		suggestion.Set( type, 0,
						arcCenter, arcSpan, 
						moveTrace.pObstruction );
						
		m_Solver.AddRegulation( suggestion );

		return;
	}
	
	float clearDist = probeDist - moveTrace.flDistObstructed;
	float pctBlocked = 1.0 - ( clearDist / probeDist );
	
	float weight = CalculateRegulationWeight( moveTrace, pctBlocked );

	if ( weight < 0.001 )
		return;
	
	if ( pctBlocked < 0.5 )
	{
		arcSpan *= pctBlocked * 2.0;
	}

	Vector vecToEnd = moveTrace.vEndPosition - GetLocalOrigin();
	Vector crossProduct;
	bool favorLeft = false, favorRight = false;
	
	if ( moveTrace.fStatus == AIMR_BLOCKED_NPC )
	{
		Vector vecToOther = moveTrace.pObstruction->GetLocalOrigin() - GetLocalOrigin();
		
		CrossProduct(vecToEnd, vecToOther, crossProduct);

		favorLeft  = ( crossProduct.z < 0 );
		favorRight = ( crossProduct.z > 0 );
	}
	else if ( moveTrace.vHitNormal != vec3_origin )
	{
		CrossProduct(vecToEnd, moveTrace.vHitNormal, crossProduct);
		favorLeft  = ( crossProduct.z > 0 );
		favorRight = ( crossProduct.z < 0 );
	}
	
	float thirdSpan = arcSpan / 3.0;
	float favoredWeight = weight * pctBlocked;
	
	suggestion.Set( type, weight,
					arcCenter, thirdSpan, 
					moveTrace.pObstruction );
					
	m_Solver.AddRegulation( suggestion );

	suggestion.Set( type, ( favorRight ) ? favoredWeight : weight,
					arcCenter - thirdSpan, thirdSpan, 
					moveTrace.pObstruction );
					
	m_Solver.AddRegulation( suggestion );

	suggestion.Set( type, ( favorLeft ) ? favoredWeight : weight,
					arcCenter + thirdSpan, thirdSpan, 
					moveTrace.pObstruction );
					
	m_Solver.AddRegulation( suggestion );
}

//-----------------------------------------------------------------------------

void CAI_PlaneSolver::CalcYawsFromOffset( float yawScanCenter, float spanPerProbe, int probeOffset, 
										  float *pYawTest, float *pYawCenter )
{
	if ( probeOffset != 0 )
	{
		float sign = ( probeOffset > 0 ) ? 1 : -1;

		*pYawCenter = yawScanCenter + probeOffset * spanPerProbe;
		if ( *pYawCenter < 0 )
			*pYawCenter += 360;
		else if ( *pYawCenter >= 360 )
			*pYawCenter -= 360;

		*pYawTest = *pYawCenter - ( sign * spanPerProbe * 0.5 );
		if ( *pYawTest < 0 )
			*pYawTest += 360;
		else if ( *pYawTest >= 360 )
			*pYawTest -= 360;
	}
	else
	{
		*pYawCenter = *pYawTest = yawScanCenter;
	}
}

//-----------------------------------------------------------------------------

void CAI_PlaneSolver::GenerateObstacleNpcs( const AILocalMoveGoal_t &goal, float probeDist )
{
	if ( !ProbeForNpcs() )
	{
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		Vector minsSelf, maxsSelf;
		m_pNpc->CollisionProp()->WorldSpaceSurroundingBounds( &minsSelf, &maxsSelf );
		float radiusSelf = (minsSelf.AsVector2D() - maxsSelf.AsVector2D()).Length() * 0.5;

		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pAI = ppAIs[i];
			if ( pAI != m_pNpc && pAI->IsAlive() && ( !goal.pPath || pAI != goal.pPath->GetTarget() ) )
			{
				Vector mins, maxs;
				
				pAI->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
				if ( mins.z < maxsSelf.z + 12.0 && maxs.z > minsSelf.z - 12.0 )
				{
					float radius = (mins.AsVector2D() - maxs.AsVector2D()).Length() * 0.5;
					float distance = ( pAI->GetAbsOrigin().AsVector2D() - m_pNpc->GetAbsOrigin().AsVector2D() ).Length();
					if ( distance - radius < radiusSelf + probeDist )
					{
						AddObstacle( pAI->WorldSpaceCenter(), radius, pAI, AIMST_AVOID_NPC );
					}
				}
			}
		}

		CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
		if ( pPlayer )
		{
			Vector mins, maxs;
			
			pPlayer->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
			if ( mins.z < maxsSelf.z + 12.0 && maxs.z > minsSelf.z - 12.0 )
			{
				float radius = (mins.AsVector2D() - maxs.AsVector2D()).Length();
				float distance = ( pPlayer->GetAbsOrigin().AsVector2D() - m_pNpc->GetAbsOrigin().AsVector2D() ).Length();
				if ( distance - radius < radiusSelf + probeDist )
				{
					AddObstacle( pPlayer->WorldSpaceCenter(), radius, pPlayer, AIMST_AVOID_NPC );
				}
			}
		}

	}
}

//-----------------------------------------------------------------------------

AI_SuggestorResult_t CAI_PlaneSolver::GenerateObstacleSuggestion( const AILocalMoveGoal_t &goal, float yawScanCenter, 
																  float probeDist, float spanPerProbe, int probeOffset)
{
	AIMoveTrace_t moveTrace;
	float		  yawTest;
	float		  arcCenter;

	CalcYawsFromOffset( yawScanCenter, spanPerProbe, probeOffset, &yawTest, &arcCenter );

	Vector probeDir = UTIL_YawToVector( yawTest );
	float requiredMovement = goal.speed * GetMotor()->GetMoveInterval();

	// Probe immediate move with footing, then look further out ignoring footing
	bool fTraceClear = true;
	if ( probeDist > requiredMovement )
	{
		if ( !MoveLimit( goal.navType, GetLocalOrigin() + probeDir * requiredMovement, !ProbeForNpcs(), true, &moveTrace ) )
		{
			fTraceClear = false;
			moveTrace.flDistObstructed = (probeDist - requiredMovement) + moveTrace.flDistObstructed;
		}
	}

	if ( fTraceClear )
	{
		fTraceClear = MoveLimit( goal.navType, GetLocalOrigin() + probeDir * probeDist, !ProbeForNpcs(), false, &moveTrace );
	}
	

	if ( !fTraceClear )
	{
		GenerateSuggestionFromTrace( goal, moveTrace, probeDist, arcCenter, spanPerProbe, probeOffset );
		return SR_OK;
	}
	
	return SR_NONE;
}

//-----------------------------------------------------------------------------

AI_SuggestorResult_t CAI_PlaneSolver::GenerateObstacleSuggestions( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace, 
																   float distClear, float probeDist, float degreesToProbe, int nProbes )
{
	Assert( nProbes % 2 == 1 );
	
	PLANESOLVER_PROFILE_SCOPE( CAI_PlaneSolver_GenerateObstacleSuggestions );
	
	AI_SuggestorResult_t seekResult = SR_NONE;
	bool				 fNewTarget = ( !m_fSolvedPrev || m_PrevTarget != goal.target );
	
	if ( fNewTarget )
		m_RefreshSamplesTimer.Force();

	if ( PLANE_SOLVER_THINK_FREQUENCY[AIStrongOpt()] == 0.0 || m_RefreshSamplesTimer.Expired() )
	{
		m_Solver.ClearRegulations();
	
		if ( !ProbeForNpcs() )
			GenerateObstacleNpcs( goal, probeDist );
			
		if ( GenerateCircleObstacleSuggestions( goal, probeDist ) )
			seekResult = SR_OK;
		
		float spanPerProbe = degreesToProbe / nProbes;
		int   nSideProbes  = (nProbes - 1) / 2;
		float yawGoalDir   = UTIL_VecToYaw( goal.dir );
		
		Vector 		  probeTarget;
		AIMoveTrace_t moveTrace;
		int			  i;
		
		// Generate suggestion from direct trace, or probe if direct trace doesn't match
		if ( fabs( probeDist - ( distClear + directTrace.flDistObstructed ) ) < 0.1 && 
			 ( ProbeForNpcs() || directTrace.fStatus != AIMR_BLOCKED_NPC ) )
		{
			if ( directTrace.fStatus != AIMR_OK )
			{
				seekResult = SR_OK;
				GenerateSuggestionFromTrace( goal, directTrace, probeDist, yawGoalDir, spanPerProbe, 0 );
			}
		}
		else if ( GenerateObstacleSuggestion( goal, yawGoalDir, probeDist, spanPerProbe, 0 ) == SR_OK )
		{
			seekResult = SR_OK;
		}
		
		// Scan left. Note that in the left and right scan, the algorithm stops as soon
		// as there is a clear path. This is an optimization in anticipation of the
		// behavior of the underlying solver. This will break more often the higher
		// PLANE_SOLVER_THINK_FREQUENCY becomes
		bool foundClear = false;

		for ( i = 1; i <= nSideProbes; i++ )
		{
			if ( !foundClear )
			{
				AI_SuggestorResult_t curSeekResult = GenerateObstacleSuggestion( goal, yawGoalDir, probeDist, 
																				 spanPerProbe, i );
				if ( curSeekResult == SR_OK )
				{
					seekResult = SR_OK;
				}
				else
					foundClear = true;
			}
			else
			{
				float ignored;
				float arcCenter;
				CalcYawsFromOffset( yawGoalDir, spanPerProbe, i, &ignored, &arcCenter );
				m_Solver.AddRegulation( AI_MoveSuggestion_t( AIMST_NO_KNOWLEDGE, 1, arcCenter, spanPerProbe ) );
			}
		}

		// Scan right
		foundClear = false;

		for ( i = -1; i >= -nSideProbes; i-- )
		{
			if ( !foundClear )
			{
				AI_SuggestorResult_t curSeekResult = GenerateObstacleSuggestion( goal, yawGoalDir, probeDist, 
																				 spanPerProbe, i );
				if ( curSeekResult == SR_OK )
				{
					seekResult = SR_OK;
				}
				else
					foundClear = true;
			}
			else
			{
				float ignored;
				float arcCenter;
				CalcYawsFromOffset( yawGoalDir, spanPerProbe, i, &ignored, &arcCenter );
				m_Solver.AddRegulation( AI_MoveSuggestion_t( AIMST_NO_KNOWLEDGE, 1, arcCenter, spanPerProbe ) );
			}
		}

		if ( seekResult == SR_OK )
		{
			float arcCenter = yawGoalDir - 180;
			if ( arcCenter < 0 )
				arcCenter += 360;
				
			// Since these are not sampled every think, place a negative arc in all directions not sampled
			m_Solver.AddRegulation( AI_MoveSuggestion_t( AIMST_NO_KNOWLEDGE, 1, arcCenter, 360 - degreesToProbe ) );

		}

		m_RefreshSamplesTimer.Reset( PLANE_SOLVER_THINK_FREQUENCY[AIStrongOpt()] );
	}
	else if ( m_Solver.HaveRegulations() )
		seekResult = SR_OK;

	return seekResult;
}

//-----------------------------------------------------------------------------
// Visualizes the regulations for debugging purposes
//-----------------------------------------------------------------------------
void CAI_PlaneSolver::VisualizeRegulations()
{
	// Visualization of regulations
	if ((GetNpc()->m_debugOverlays & OVERLAY_NPC_STEERING_REGULATIONS) != 0)
	{
		m_Solver.VisualizeRegulations( GetNpc()->WorldSpaceCenter() );
	}
}

void CAI_PlaneSolver::VisualizeSolution( const Vector &vecGoal, const Vector& vecActual )
{
	if ((GetNpc()->m_debugOverlays & OVERLAY_NPC_STEERING_REGULATIONS) != 0)
	{
		// Compute centroid...
		Vector centroid = GetNpc()->WorldSpaceCenter();
		Vector goalPt, actualPt;

		VectorMA( centroid, 20, vecGoal, goalPt );
		VectorMA( centroid, 20, vecActual, actualPt );

		NDebugOverlay::Line(centroid, goalPt, 255, 255, 255, true, 0.1f );
		NDebugOverlay::Line(centroid, actualPt, 255, 255, 0, true, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Adjust the solution for fliers
//-----------------------------------------------------------------------------
#define MIN_ZDIR_TO_RADIUS	0.1f

void CAI_PlaneSolver::AdjustSolutionForFliers( const AILocalMoveGoal_t &goal, float flSolutionYaw, Vector *pSolution )
{
	// Fliers should move up if there are local obstructions...
	// A hacky solution, but the bigger the angle of deflection, the more likely
	// we're close to a problem and the higher we should go up.
	Assert( pSolution->z == 0.0f );

	// If we're largely needing to move down, then blow off the upward motion...
	Vector vecDelta, vecDir;
	VectorSubtract( goal.target, GetLocalOrigin(), vecDelta );
	vecDir = vecDelta;
	VectorNormalize( vecDir );
	float flRadius = sqrt( vecDir.x * vecDir.x + vecDir.y * vecDir.y );
	*pSolution *= flRadius;
	pSolution->z = vecDir.z;
	AssertFloatEquals( pSolution->LengthSqr(), 1.0f, 1e-3 );

	// Move up 0 when we have to move forward as much as we have to move down z (45 degree angle)
	// Move up max when we have to move forward 5x as much as we have to move down z,
	// or if we have to move up z.
	float flUpAmount = 0.0f;
	if ( vecDir.z >= -flRadius * MIN_ZDIR_TO_RADIUS)
	{
		flUpAmount = 1.0f;
	}
	else if ((vecDir.z <= -flRadius) || (fabs(vecDir.z) < 1e-3))
	{
		flUpAmount = 0.0f;
	}
	else
	{
		flUpAmount = (-flRadius / vecDir.z) - 1.0f;
		flUpAmount *= MIN_ZDIR_TO_RADIUS;
		Assert( (flUpAmount >= 0.0f) && (flUpAmount <= 1.0f) );
	}

	// Check the deflection amount...
	pSolution->z += flUpAmount * 5.0f;

	// FIXME: Also, if we've got a bunch of regulations, we may 
	// also wish to raise up a little bit..because this indicates
	// that we've got a bunch of stuff to avoid
	VectorNormalize( *pSolution );
}

//-----------------------------------------------------------------------------

unsigned CAI_PlaneSolver::ComputeTurnBiasFlags( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace )
{
	if ( directTrace.fStatus == AIMR_BLOCKED_WORLD )
	{
		// @TODO (toml 11-11-02): stuff plane normal of hit into trace Use here to compute a bias?
		// 
		return 0;
	}

	if ( directTrace.fStatus == AIMR_BLOCKED_NPC )
	{
		return AIMS_FAVOR_LEFT;
	}

	return 0;
}

//-----------------------------------------------------------------------------

bool CAI_PlaneSolver::RunMoveSolver( const AILocalMoveGoal_t &goal, const AIMoveTrace_t &directTrace, float degreesPositiveArc, 
									 bool fDeterOscillation, Vector *pResult )
{
	PLANESOLVER_PROFILE_SCOPE( CAI_PlaneSolver_RunMoveSolver );
		
	AI_MoveSolution_t solution;
	
	if ( m_Solver.HaveRegulations() )
	{
		// @TODO (toml 07-19-02): add a movement threshhold here (the target may be the same,
		// but the ai is nowhere near where the last solution was derived)
		bool fNewTarget = ( !m_fSolvedPrev || m_PrevTarget != goal.target );
		
		// For debugging, visualize our regulations
		VisualizeRegulations();

		AI_MoveSuggestion_t moveSuggestions[2];
		int					nSuggestions = 1;

		moveSuggestions[0].Set( AIMST_MOVE, 1, UTIL_VecToYaw( goal.dir ), degreesPositiveArc );
		moveSuggestions[0].flags |= ComputeTurnBiasFlags( goal, directTrace );

		if ( fDeterOscillation && !fNewTarget )
		{
#ifndef TESTING_SUGGESTIONS
			moveSuggestions[nSuggestions++].Set( AIMST_OSCILLATION_DETERRANCE, 1, m_PrevSolution - 180, 180 );
#endif
		}

		if ( m_Solver.Solve( moveSuggestions, nSuggestions, &solution ) )
		{
			*pResult = UTIL_YawToVector( solution.dir );

			if (goal.navType == NAV_FLY)
			{
				// FIXME: Does the z component have to occur during the goal
				// setting because it's there & only there where MoveLimit
				// will report contact with the world if we move up?
				AdjustSolutionForFliers( goal, solution.dir, pResult );
			}
			// A crude attempt at oscillation detection: if we solved last time, and this time, and the same target is
			// involved, and we resulted in nearly a 180, we are probably oscillating
#ifndef TESTING_SUGGESTIONS
			if ( !fNewTarget )
			{
				float delta = solution.dir - m_PrevSolution;
				if ( delta < 0 )
					delta += 360;
				if ( delta > 165 && delta < 195 )
					return false;
			}
#endif
			m_PrevSolution = solution.dir;
			m_PrevSolutionVector = *pResult;

			Vector curVelocity = m_pNpc->GetSmoothedVelocity();
			if ( curVelocity != vec3_origin )
			{
				VectorNormalize( curVelocity );
				if ( !fNewTarget )
				{
					*pResult = curVelocity * 0.1 + m_PrevSolutionVector * 0.1 + *pResult * 0.8;
				}
				else
				{
					*pResult = curVelocity * 0.2 + *pResult * 0.8;
				}
			}

			return true;
		}
	}
	else
	{
		if (goal.navType != NAV_FLY)
		{
			*pResult = goal.dir;
		}
		else
		{
			VectorSubtract( goal.target, GetLocalOrigin(), *pResult );
			VectorNormalize( *pResult );
		}
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

float CAI_PlaneSolver::CalcProbeDist( float speed )	
{ 
	// one second or one hull
	float result = GetLookaheadTime() * speed;
	if ( result < m_pNpc->GetMoveProbe()->GetHullWidth() )
		return m_pNpc->GetMoveProbe()->GetHullWidth();
	if ( result > MAX_PROBE_DIST[AIStrongOpt()] )
		return MAX_PROBE_DIST[AIStrongOpt()];
	return result;
}

//-----------------------------------------------------------------------------

void CAI_PlaneSolver::AddObstacle( const Vector &center, float radius, CBaseEntity *pEntity, AI_MoveSuggType_t type )
{
	m_Obstacles.AddToTail( CircleObstacles_t( center, radius, pEntity, type ) );
}

//-----------------------------------------------------------------------------
bool CAI_PlaneSolver::GenerateCircleObstacleSuggestions( const AILocalMoveGoal_t &moveGoal, float probeDist )
{
	bool result = false;
	Vector npcLoc = m_pNpc->WorldSpaceCenter();
	Vector mins, maxs;

	m_pNpc->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
	float radiusNpc = (mins.AsVector2D() - maxs.AsVector2D()).Length() * 0.5;
	
	for ( int i = 0; i < m_Obstacles.Count(); i++ )
	{
		CBaseEntity *pObstacleEntity = NULL;

		float zDistTooFar;
		if ( m_Obstacles[i].hEntity && m_Obstacles[i].hEntity->CollisionProp() )
		{
			pObstacleEntity = m_Obstacles[i].hEntity.Get();

			if( pObstacleEntity == moveGoal.pMoveTarget && (pObstacleEntity->IsNPC() || pObstacleEntity->IsPlayer()) )
			{
				// HEY! I'm trying to avoid the very thing I'm trying to get to. This will make we wobble like a drunk as I approach. Don't do it.
				continue;
			}

			pObstacleEntity->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
			zDistTooFar = ( maxs.z - mins.z ) * 0.5 + GetNpc()->GetHullHeight() * 0.5;
		}
		else
			zDistTooFar = GetNpc()->GetHullHeight();
			
		if ( fabs( m_Obstacles[i].center.z - npcLoc.z ) > zDistTooFar )
			continue;

		Vector vecToNpc 		= npcLoc - m_Obstacles[i].center;
		vecToNpc.z = 0;
		float distToObstacleSq 	= sq(vecToNpc.x) + sq(vecToNpc.y);
		float radius = m_Obstacles[i].radius + radiusNpc;

		if ( distToObstacleSq > 0.001 && distToObstacleSq < sq( radius + probeDist ) )
		{
			Vector vecToObstacle = vecToNpc * -1;
			float distToObstacle = VectorNormalize( vecToObstacle );
			float weight;
			float arc;
			float radiusSq = sq(radius);

			float flDot = DotProduct( vecToObstacle, moveGoal.dir );

			// Don't steer around to avoid obstacles we've already passed, unless we're right up against them.
			// That is, do this computation without the probeDist added in.
			if( flDot < 0.0f && distToObstacleSq > radiusSq )
			{
				continue;
			}

			if ( radiusSq < distToObstacleSq )
			{
				Vector vecTangent;
				float distToTangent = FastSqrt( distToObstacleSq - radiusSq );

				float oneOverDistToObstacleSq = 1 / distToObstacleSq;

				vecTangent.x = ( -distToTangent * vecToNpc.x + radius * vecToNpc.y ) * oneOverDistToObstacleSq;
				vecTangent.y = ( -distToTangent * vecToNpc.y - radius * vecToNpc.x ) * oneOverDistToObstacleSq;
				vecTangent.z = 0;

				float cosHalfArc = vecToObstacle.Dot( vecTangent );
				arc = RAD2DEG(acosf( cosHalfArc )) * 2.0;
				weight = 1.0 - (distToObstacle - radius) / probeDist;
				if ( weight > 0.75 )
					arc += (arc * 0.5) * (weight - 0.75) / 0.25;
				
				Assert( weight >= 0.0 && weight <= 1.0 );

#if DEBUG_OBSTACLES
				// -------------------------
				Msg( "Adding arc %f, w %f\n", arc, weight );

				Vector pointTangent = npcLoc + ( vecTangent * distToTangent );
					
				NDebugOverlay::Line( npcLoc - Vector( 0, 0, 64 ), npcLoc + Vector(0,0,64), 0,255,0, false, 0.1 );
				NDebugOverlay::Line( center - Vector( 0, 0, 64 ), center + Vector(0,0,64), 0,255,0, false, 0.1 );
				NDebugOverlay::Line( pointTangent - Vector( 0, 0, 64 ), pointTangent + Vector(0,0,64), 0,255,0, false, 0.1 );
				
				NDebugOverlay::Line( npcLoc + Vector(0,0,64), center + Vector(0,0,64), 0,0,255, false, 0.1 );
				NDebugOverlay::Line( center + Vector(0,0,64), pointTangent + Vector(0,0,64), 0,0,255, false, 0.1 );
				NDebugOverlay::Line( pointTangent + Vector(0,0,64), npcLoc + Vector(0,0,64), 0,0,255, false, 0.1 );
#endif
			}
			else
			{
				arc = 210;
				weight = 1.0;
			}

			if ( m_Obstacles[i].hEntity != NULL )
			{
				weight = AdjustRegulationWeight( m_Obstacles[i].hEntity, weight );
			}
			
			AI_MoveSuggestion_t suggestion( m_Obstacles[i].type, weight, UTIL_VecToYaw(vecToObstacle), arc );
			m_Solver.AddRegulation( suggestion );
			result = true;
		}
	}
	
	m_Obstacles.RemoveAll();
	return result;

}

//-----------------------------------------------------------------------------

bool CAI_PlaneSolver::Solve( const AILocalMoveGoal_t &goal, float distClear, Vector *pSolution )
{
	bool solved = false;
	
	//---------------------------------
	
	if ( goal.speed == 0 )
		return false;

	if ( DetectUnsolvable( goal ) )
		return false;

	//---------------------------------

	bool fVeryClose			 = ( distClear < 1.0 );
	float degreesPositiveArc = ( !fVeryClose ) ? DEGREES_POSITIVE_ARC : DEGREES_POSITIVE_ARC_CLOSE_OBSTRUCTION;
	float probeDist			 = CalcProbeDist( goal.speed );
	
	if ( goal.flags & ( AILMG_TARGET_IS_TRANSITION | AILMG_TARGET_IS_GOAL ) )
	{
		probeDist = MIN( goal.maxDist, probeDist );
	}

	if ( GenerateObstacleSuggestions( goal, goal.directTrace, distClear, probeDist, degreesPositiveArc, NUM_PROBES ) != SR_FAIL )
	{
		if ( RunMoveSolver( goal, goal.directTrace, degreesPositiveArc, !fVeryClose, pSolution ) )
		{
			// Visualize desired + actual directions
			VisualizeSolution( goal.dir, *pSolution );

			AIMoveTrace_t moveTrace;
			float 		  requiredMovement = goal.speed * GetMotor()->GetMoveInterval();

			MoveLimit( goal.navType, GetLocalOrigin() + *pSolution * requiredMovement, false, true, &moveTrace );
			
			if ( !IsMoveBlocked( moveTrace ) )
				solved = true;
			else
				solved = false;
		}
	}

	m_fSolvedPrev 		= ( solved && goal.speed != 0 ); // a solution found when speed is zero is not meaningful
	m_PrevTarget 		= goal.target;

	return solved;
}

//=============================================================================
