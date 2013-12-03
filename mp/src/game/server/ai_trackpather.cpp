//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "trains.h"
#include "ai_trackpather.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	TRACKPATHER_DEBUG_LEADING	1
#define	TRACKPATHER_DEBUG_PATH		2
#define TRACKPATHER_DEBUG_TRACKS	3
ConVar g_debug_trackpather( "g_debug_trackpather", "0", FCVAR_CHEAT );

//------------------------------------------------------------------------------

BEGIN_DATADESC( CAI_TrackPather )
	DEFINE_FIELD( m_vecDesiredPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecGoalOrientation,		FIELD_VECTOR ),

	DEFINE_FIELD( m_pCurrentPathTarget,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pDestPathTarget,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pLastPathTarget,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pTargetNearestPath,		FIELD_CLASSPTR ),
	
	DEFINE_FIELD( m_strCurrentPathName,		FIELD_STRING ),
	DEFINE_FIELD( m_strDestPathName,		FIELD_STRING ),
	DEFINE_FIELD( m_strLastPathName,		FIELD_STRING ),
	DEFINE_FIELD( m_strTargetNearestPathName,	FIELD_STRING ),
	
	DEFINE_FIELD( m_vecLastGoalCheckPosition,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flEnemyPathUpdateTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bForcedMove,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPatrolling,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPatrolBreakable,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLeading,				FIELD_BOOLEAN ),

	// Derived class pathing data
	DEFINE_FIELD( m_flTargetDistanceThreshold,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flAvoidDistance,		FIELD_FLOAT ),

	DEFINE_FIELD( m_flTargetTolerance,		FIELD_FLOAT ),
	DEFINE_FIELD( m_vecSegmentStartPoint,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecSegmentStartSplinePoint,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bMovingForward,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bChooseFarthestPoint,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flFarthestPathDist,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flPathMaxSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flTargetDistFromPath,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flLeadDistance,			FIELD_FLOAT ),
	DEFINE_FIELD( m_vecTargetPathDir,		FIELD_VECTOR ),
	DEFINE_FIELD( m_vecTargetPathPoint,		FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_nPauseState,			FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTrack", InputSetTrack ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FlyToSpecificTrackViaPath", InputFlyToPathTrack ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "StartPatrol", InputStartPatrol ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "StopPatrol", InputStopPatrol ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "StartBreakableMovement", InputStartBreakableMovement ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "StopBreakableMovement", InputStopBreakableMovement ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "ChooseFarthestPathPoint", InputChooseFarthestPathPoint ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "ChooseNearestPathPoint", InputChooseNearestPathPoint ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,"InputStartLeading", InputStartLeading ),
	DEFINE_INPUTFUNC( FIELD_VOID,	 "InputStopLeading", InputStopLeading ),

	// Obsolete, for backwards compatibility	
	DEFINE_INPUTFUNC( FIELD_VOID,	 "StartPatrolBreakable", InputStartPatrolBreakable ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FlyToPathTrack", InputFlyToPathTrack ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Initialize pathing data
//-----------------------------------------------------------------------------
void CAI_TrackPather::InitPathingData( float flTrackArrivalTolerance, float flTargetDistance, float flAvoidDistance )
{
	m_flTargetTolerance = flTrackArrivalTolerance;
	m_flTargetDistanceThreshold = flTargetDistance;
	m_flAvoidDistance = flAvoidDistance;

	m_pCurrentPathTarget = NULL;
	m_pDestPathTarget = NULL;
	m_pLastPathTarget = NULL;
	m_pTargetNearestPath = NULL;
	m_bLeading = false;

	m_flEnemyPathUpdateTime	= gpGlobals->curtime;
	m_bForcedMove = false;
	m_bPatrolling = false;
	m_bPatrolBreakable = false;
	m_flLeadDistance = 0.0f;
	m_bMovingForward = true;
	m_vecSegmentStartPoint = m_vecSegmentStartSplinePoint = m_vecDesiredPosition = GetAbsOrigin();
	m_bChooseFarthestPoint = true;
	m_flFarthestPathDist = 1e10;
	m_flPathMaxSpeed = 0;
	m_nPauseState = PAUSE_NO_PAUSE; 
}

	   
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::OnRestore( void )
{
	BaseClass::OnRestore();

	// Restore current path
	if ( m_strCurrentPathName != NULL_STRING )
	{
		m_pCurrentPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strCurrentPathName );
	}
	else
	{
		m_pCurrentPathTarget = NULL;
	}

	// Restore destination path
	if ( m_strDestPathName != NULL_STRING )
	{
		m_pDestPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strDestPathName );
	}
	else
	{
		m_pDestPathTarget = NULL;
	}

	// Restore last path
	if ( m_strLastPathName != NULL_STRING )
	{
		m_pLastPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strLastPathName );
	}
	else
	{
		m_pLastPathTarget = NULL;
	}

	// Restore target nearest path
	if ( m_strTargetNearestPathName != NULL_STRING )
	{
		m_pTargetNearestPath = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strTargetNearestPathName );
	}
	else
	{
		m_pTargetNearestPath = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::OnSave( IEntitySaveUtils *pUtils )
{
	BaseClass::OnSave( pUtils );

	// Stash all the paths into strings for restoration later
	m_strCurrentPathName = ( m_pCurrentPathTarget != NULL ) ? m_pCurrentPathTarget->GetEntityName() : NULL_STRING;
	m_strDestPathName = ( m_pDestPathTarget != NULL ) ? m_pDestPathTarget->GetEntityName() : NULL_STRING;
	m_strLastPathName = ( m_pLastPathTarget != NULL ) ? m_pLastPathTarget->GetEntityName() : NULL_STRING;
	m_strTargetNearestPathName = ( m_pTargetNearestPath != NULL ) ? m_pTargetNearestPath->GetEntityName() : NULL_STRING;
}


//-----------------------------------------------------------------------------
// Leading distance
//-----------------------------------------------------------------------------
void CAI_TrackPather::EnableLeading( bool bEnable )
{
	bool bWasLeading = m_bLeading;
	m_bLeading = bEnable;
	if ( m_bLeading )
	{
		m_bPatrolling = false;
	}
	else if ( bWasLeading )
	{
	
		// Going from leading to not leading. Refresh the desired position
		// to prevent us from hovering around our old, no longer valid lead position.
  		if ( m_pCurrentPathTarget )
		{
			SetDesiredPosition( m_pCurrentPathTarget->GetAbsOrigin() );
		}
	}
}

void CAI_TrackPather::SetLeadingDistance( float flLeadDistance )
{
	m_flLeadDistance = flLeadDistance;
}

float CAI_TrackPather::GetLeadingDistance( ) const
{
	return m_flLeadDistance;
}


//-----------------------------------------------------------------------------
// Returns the next path along our current path
//-----------------------------------------------------------------------------
inline CPathTrack *CAI_TrackPather::NextAlongCurrentPath( CPathTrack *pPath ) const
{
	return CPathTrack::ValidPath( m_bMovingForward ? pPath->GetNext() : pPath->GetPrevious() );
}

inline CPathTrack *CAI_TrackPather::PreviousAlongCurrentPath( CPathTrack *pPath ) const
{
	return CPathTrack::ValidPath( m_bMovingForward ? pPath->GetPrevious() : pPath->GetNext() );
}

inline CPathTrack *CAI_TrackPather::AdjustForMovementDirection( CPathTrack *pPath ) const
{
	if ( !m_bMovingForward && CPathTrack::ValidPath( pPath->GetPrevious( ) ) )
	{
		pPath = CPathTrack::ValidPath( pPath->GetPrevious() );
	}
	return pPath;
}

	
//-----------------------------------------------------------------------------
// Enemy visibility check
//-----------------------------------------------------------------------------
CBaseEntity *CAI_TrackPather::FindTrackBlocker( const Vector &vecViewPoint, const Vector &vecTargetPos )
{
	trace_t	tr;
	AI_TraceHull( vecViewPoint, vecTargetPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	return (tr.fraction != 1.0f) ? tr.m_pEnt : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &targetPos - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CPathTrack *CAI_TrackPather::BestPointOnPath( CPathTrack *pPath, const Vector &targetPos, float flAvoidRadius, bool visible, bool bFarthestPoint )
{
	// Find the node nearest to the destination path target if a path is not specified
	if ( pPath == NULL )
	{
		pPath = m_pDestPathTarget;
	}

	// If the path node we're trying to use is not valid, then we're done.
	if ( CPathTrack::ValidPath( pPath ) == NULL )
	{
		//FIXME: Implement
		Assert(0);
		return NULL;
	}

	// Our target may be in a vehicle
	CBaseEntity *pVehicle = NULL;
	CBaseEntity *pTargetEnt = GetTrackPatherTargetEnt();	
	if ( pTargetEnt != NULL )
	{
		CBaseCombatCharacter *pCCTarget = pTargetEnt->MyCombatCharacterPointer();
		if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		{
			pVehicle = pCCTarget->GetVehicleEntity();
		}
	}

	// Faster math...
	flAvoidRadius *= flAvoidRadius;

	// Find the nearest node to the target (going forward)
	CPathTrack *pNearestPath	= NULL;
	float		flNearestDist	= bFarthestPoint ? 0 : 999999999;
	float		flPathDist;

	float flFarthestDistSqr = ( m_flFarthestPathDist - 2.0f * m_flTargetDistanceThreshold );
	flFarthestDistSqr *= flFarthestDistSqr;

	// NOTE: Gotta do it this crazy way because paths can be one-way.
	for ( int i = 0; i < 2; ++i )
	{
		int loopCheck = 0;
		CPathTrack *pTravPath = pPath;
		CPathTrack *pNextPath;

		BEGIN_PATH_TRACK_ITERATION();
		for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = pNextPath, loopCheck++ )
		{
			// Circular loop checking
			if ( pTravPath->HasBeenVisited() )
				break;

			pTravPath->Visit();

			pNextPath = (i == 0) ? pTravPath->GetPrevious() : pTravPath->GetNext();

			// Find the distance between this test point and our goal point
			flPathDist = ( pTravPath->GetAbsOrigin() - targetPos ).LengthSqr();

			// See if it's closer and it's also not within our avoidance radius
			if ( bFarthestPoint )
			{
				if ( ( flPathDist <= flNearestDist ) && ( flNearestDist <= flFarthestDistSqr ) )
					continue;
			}
			else
			{
				if ( flPathDist >= flNearestDist ) 
					continue;
			}

			// Don't choose points that are within the avoid radius
			if ( flAvoidRadius && ( pTravPath->GetAbsOrigin() - targetPos ).Length2DSqr() <= flAvoidRadius )
				continue;

			if ( visible )
			{
				// If it has to be visible, run those checks
				CBaseEntity *pBlocker = FindTrackBlocker( pTravPath->GetAbsOrigin(), targetPos );

				// Check to see if we've hit the target, or the player's vehicle if it's a player in a vehicle
				bool bHitTarget = ( pTargetEnt && ( pTargetEnt == pBlocker ) ) ||
									( pVehicle && ( pVehicle == pBlocker ) );

				// If we hit something, and it wasn't the target or his vehicle, then no dice
				// If we hit the target and forced move was set, *still* no dice
				if ( (pBlocker != NULL) && ( !bHitTarget || m_bForcedMove ) )
					continue;
			}

			pNearestPath	= pTravPath;
			flNearestDist	= flPathDist;
		}
	}

	return pNearestPath;
}


//-----------------------------------------------------------------------------
// Compute a point n units along a path
//-----------------------------------------------------------------------------
CPathTrack *CAI_TrackPather::ComputeLeadingPointAlongPath( const Vector &vecStartPoint, 
				CPathTrack *pFirstTrack, float flDistance, Vector *pTarget )
{
	bool bMovingForward = (flDistance > 0.0f);
	flDistance = fabs(flDistance);

	CPathTrack *pTravPath = pFirstTrack;
	if ( (!bMovingForward) && pFirstTrack->GetPrevious() )
	{
		pTravPath = pFirstTrack->GetPrevious();
	}

	*pTarget = vecStartPoint;
	CPathTrack *pNextPath;

	// No circular loop checking needed; eventually, it'll run out of distance
	for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = pNextPath )
	{
		pNextPath = bMovingForward ? pTravPath->GetNext() : pTravPath->GetPrevious();

		// Find the distance between this test point and our goal point
		float flPathDist = pTarget->DistTo( pTravPath->GetAbsOrigin() );

		// Find the distance between this test point and our goal point
		if ( flPathDist <= flDistance )
		{
			flDistance -= flPathDist;
			*pTarget = pTravPath->GetAbsOrigin();
			if ( !CPathTrack::ValidPath(pNextPath) )
				return bMovingForward ? pTravPath : pTravPath->GetNext();

			continue;
		}
		
		ComputeClosestPoint( *pTarget, flDistance, pTravPath->GetAbsOrigin(), pTarget );
		return bMovingForward ? pTravPath : pTravPath->GetNext();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Compute the distance to a particular point on the path
//-----------------------------------------------------------------------------
float CAI_TrackPather::ComputeDistanceAlongPathToPoint( CPathTrack *pStartTrack, 
	CPathTrack *pDestTrack, const Vector &vecDestPosition, bool bMovingForward )
{													  
	float flTotalDist = 0.0f;

	Vector vecPoint;
	ClosestPointToCurrentPath( &vecPoint );

	CPathTrack *pTravPath = pStartTrack;
	CPathTrack *pNextPath, *pTestPath;
	BEGIN_PATH_TRACK_ITERATION();
	for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = pNextPath )
	{
		// Circular loop checking
		if ( pTravPath->HasBeenVisited() )
			break;

		// Mark it as being visited.
		pTravPath->Visit();

		pNextPath = bMovingForward ? pTravPath->GetNext() : pTravPath->GetPrevious();
		pTestPath = pTravPath;
		Assert( pTestPath );

		if ( pTravPath == pDestTrack )
		{
			Vector vecDelta;
			Vector vecPathDelta;
			VectorSubtract( vecDestPosition, vecPoint, vecDelta );
			ComputePathDirection( pTravPath, &vecPathDelta );
			float flDot = DotProduct( vecDelta, vecPathDelta );
			flTotalDist += (flDot > 0.0f ? 1.0f : -1.0f) * vecDelta.Length2D(); 
			break;
		}

		// NOTE: This would be made more accurate if we did the path direction check here too.
		// The starting vecPoint is sometimes *not* within the bounds of the line segment.

		// Find the distance between this test point and our goal point
		flTotalDist += (bMovingForward ? 1.0f : -1.0f) * vecPoint.AsVector2D().DistTo( pTestPath->GetAbsOrigin().AsVector2D() ); 
		vecPoint = pTestPath->GetAbsOrigin();
	}

	return flTotalDist;
}

	
//------------------------------------------------------------------------------
// Track debugging info
//------------------------------------------------------------------------------
void CAI_TrackPather::VisualizeDebugInfo( const Vector &vecNearestPoint, const Vector &vecTarget )
{
	if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_PATH )
	{
		NDebugOverlay::Line( m_vecSegmentStartPoint, vecTarget, 0, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( vecNearestPoint, -Vector(16,16,16), Vector(16,16,16), 255, 0, 0, true, 0.1f );
		NDebugOverlay::Cross3D( m_pCurrentPathTarget->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( m_vecDesiredPosition, -Vector(16,16,16), Vector(16,16,16), 0, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( m_pDestPathTarget->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 255, 255, 255, true, 0.1f );

		if ( m_pTargetNearestPath )
		{
			NDebugOverlay::Cross3D( m_pTargetNearestPath->GetAbsOrigin(), -Vector(24,24,24), Vector(24,24,24), 255, 0, 255, true, 0.1f );
		}
	}

	if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_TRACKS )
	{
		if ( m_pCurrentPathTarget )
		{
			CPathTrack *pPathTrack = m_pCurrentPathTarget;
			for ( ; CPathTrack::ValidPath( pPathTrack ); pPathTrack = pPathTrack->GetNext() )
			{
				NDebugOverlay::Box( pPathTrack->GetAbsOrigin(), -Vector(2,2,2), Vector(2,2,2), 0,255, 0, 8, 0.1 );
				if ( CPathTrack::ValidPath( pPathTrack->GetNext() ) )
				{
					NDebugOverlay::Line( pPathTrack->GetAbsOrigin(), pPathTrack->GetNext()->GetAbsOrigin(), 0,255,0, true, 0.1 );
				}

				if ( pPathTrack->GetNext() == m_pCurrentPathTarget )
					break;
			}
		}
	}
}


//------------------------------------------------------------------------------
// Does this path track have LOS to the target?
//------------------------------------------------------------------------------
bool CAI_TrackPather::HasLOSToTarget( CPathTrack *pTrack )
{
	CBaseEntity *pTargetEnt = GetTrackPatherTargetEnt();
	if ( !pTargetEnt )
		return true;

	Vector targetPos;
	if ( !GetTrackPatherTarget( &targetPos ) )
		return true;

	// Translate driver into vehicle for testing
	CBaseEntity *pVehicle = NULL;
	CBaseCombatCharacter *pCCTarget = pTargetEnt->MyCombatCharacterPointer();
	if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
	{
		pVehicle = pCCTarget->GetVehicleEntity();
	}

	// If it has to be visible, run those checks
	CBaseEntity *pBlocker = FindTrackBlocker( pTrack->GetAbsOrigin(), targetPos );

	// Check to see if we've hit the target, or the player's vehicle if it's a player in a vehicle
	bool bHitTarget = ( pTargetEnt && ( pTargetEnt == pBlocker ) ) ||
						( pVehicle && ( pVehicle == pBlocker ) );

	return (pBlocker == NULL) || bHitTarget;
}


//------------------------------------------------------------------------------
// Moves to the track
//------------------------------------------------------------------------------
void CAI_TrackPather::UpdateCurrentTarget()
{
	// Find the point along the line that we're closest to.
	const Vector &vecTarget = m_pCurrentPathTarget->GetAbsOrigin();
	Vector vecPoint;
	float t = ClosestPointToCurrentPath( &vecPoint );
	if ( (t < 1.0f) && ( vecPoint.DistToSqr( vecTarget ) > m_flTargetTolerance * m_flTargetTolerance ) )
		goto visualizeDebugInfo;

	// Forced move is gone as soon as we've reached the first point on our path
	if ( m_bLeading )
	{
		m_bForcedMove = false;
	}

	// Trip our "path_track reached" output
	if ( m_pCurrentPathTarget != m_pLastPathTarget )
	{
		// Get the path's specified max speed
		m_flPathMaxSpeed = m_pCurrentPathTarget->m_flSpeed;

		variant_t emptyVariant;
		m_pCurrentPathTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );
		m_pLastPathTarget = m_pCurrentPathTarget;
	}

	if ( m_nPauseState == PAUSED_AT_POSITION )
		return;

	if ( m_nPauseState == PAUSE_AT_NEXT_LOS_POSITION )
	{
		if ( HasLOSToTarget(m_pCurrentPathTarget) )
		{
			m_nPauseState = PAUSED_AT_POSITION;
			return;
		}
	}

	// Update our dest path target, if appropriate...
	if ( m_pCurrentPathTarget == m_pDestPathTarget )
	{
		m_bForcedMove = false;
		SelectNewDestTarget();
	}

	// Did SelectNewDestTarget give us a new point to move to?
	if ( m_pCurrentPathTarget != m_pDestPathTarget )
	{
		// Update to the next path, if there is one...
		m_pCurrentPathTarget = NextAlongCurrentPath( m_pCurrentPathTarget );
		if ( !m_pCurrentPathTarget )
		{
			m_pCurrentPathTarget = m_pLastPathTarget;
		}
	}
	else
	{
		// We're at rest (no patrolling behavior), which means we're moving forward now.
		m_bMovingForward = true;
	}

	SetDesiredPosition( m_pCurrentPathTarget->GetAbsOrigin() );
	m_vecSegmentStartSplinePoint = m_vecSegmentStartPoint;
	m_vecSegmentStartPoint = m_pLastPathTarget->GetAbsOrigin();

visualizeDebugInfo:
	VisualizeDebugInfo( vecPoint, vecTarget );
}


//-----------------------------------------------------------------------------
//
// NOTE: All code below is used exclusively for leading/trailing behavior
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Compute the distance to the leading position
//-----------------------------------------------------------------------------
float CAI_TrackPather::ComputeDistanceToLeadingPosition()
{
	return ComputeDistanceAlongPathToPoint( m_pCurrentPathTarget, m_pDestPathTarget, GetDesiredPosition(), m_bMovingForward );
}

	
//-----------------------------------------------------------------------------
// Compute the distance to the *target* position
//-----------------------------------------------------------------------------
float CAI_TrackPather::ComputeDistanceToTargetPosition()
{
	Assert( m_pTargetNearestPath );

	CPathTrack *pDest = m_bMovingForward ? m_pTargetNearestPath.Get() : m_pTargetNearestPath->GetPrevious();
	if ( !pDest )
	{
		pDest = m_pTargetNearestPath;
	}
	bool bMovingForward = IsForwardAlongPath( m_pCurrentPathTarget, pDest );

	CPathTrack *pStart = m_pCurrentPathTarget;
	if ( bMovingForward != m_bMovingForward )
	{
		if (bMovingForward)
		{
			if ( pStart->GetNext() )
			{
				pStart = pStart->GetNext();
			}
			if ( pDest->GetNext() )
			{
				pDest = pDest->GetNext();
			}
		}
		else
		{
			if ( pStart->GetPrevious() )
			{
				pStart = pStart->GetPrevious();
			}
			if ( pDest->GetPrevious() )
			{
				pDest = pDest->GetPrevious();
			}
		}
	}

	return ComputeDistanceAlongPathToPoint( pStart, pDest, m_vecTargetPathPoint, bMovingForward );
}


//-----------------------------------------------------------------------------
// Compute a path direction
//-----------------------------------------------------------------------------
void CAI_TrackPather::ComputePathDirection( CPathTrack *pPath, Vector *pVecPathDir )
{
	if ( pPath->GetPrevious() )
	{
		VectorSubtract( pPath->GetAbsOrigin(), pPath->GetPrevious()->GetAbsOrigin(), *pVecPathDir );
	}
	else
	{
		if ( pPath->GetNext() )
		{
			VectorSubtract( pPath->GetNext()->GetAbsOrigin(), pPath->GetAbsOrigin(), *pVecPathDir );
		}
		else
		{
			pVecPathDir->Init( 1, 0, 0 );
		}
	}
	VectorNormalize( *pVecPathDir );
}


//-----------------------------------------------------------------------------
// What's the current path direction?
//-----------------------------------------------------------------------------
void CAI_TrackPather::CurrentPathDirection( Vector *pVecPathDir )
{
	if ( m_pCurrentPathTarget )
	{
		ComputePathDirection( m_pCurrentPathTarget, pVecPathDir );
	}
	else
	{
		pVecPathDir->Init( 0, 0, 1 );
	}
}



//-----------------------------------------------------------------------------
// Compute a point n units along the current path from our current position
// (but don't pass the desired target point)
//-----------------------------------------------------------------------------
void CAI_TrackPather::ComputePointAlongCurrentPath( float flDistance, float flPerpDist, Vector *pTarget )
{
	Vector vecPathDir;
	Vector vecStartPoint;
	ClosestPointToCurrentPath( &vecStartPoint );
	*pTarget = vecStartPoint;

	if ( flDistance != 0.0f )
	{
		Vector vecPrevPoint = vecStartPoint;
		CPathTrack *pTravPath = m_pCurrentPathTarget;
		CPathTrack *pAdjustedDest = AdjustForMovementDirection( m_pDestPathTarget );
		for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = NextAlongCurrentPath( pTravPath ) )
		{
			if ( pTravPath == pAdjustedDest )
			{
				ComputePathDirection( pTravPath, &vecPathDir );

				float flPathDist = pTarget->DistTo( GetDesiredPosition() );
				if ( flDistance > flPathDist )
				{
					*pTarget = GetDesiredPosition();
				}
				else
				{
					ComputeClosestPoint( *pTarget, flDistance, GetDesiredPosition(), pTarget );
				}
				break;
			}

			// Find the distance between this test point and our goal point
			float flPathDist = pTarget->DistTo( pTravPath->GetAbsOrigin() );

			// Find the distance between this test point and our goal point
			if ( flPathDist <= flDistance )
			{
				flDistance -= flPathDist;
				*pTarget = pTravPath->GetAbsOrigin();

				// FIXME: Reduce the distance further based on the angle between this segment + the next
				continue;
			}
			
			ComputePathDirection( pTravPath, &vecPathDir );
			ComputeClosestPoint( *pTarget, flDistance, pTravPath->GetAbsOrigin(), pTarget );
			break;
		}
	}
	else
	{
		VectorSubtract( m_pCurrentPathTarget->GetAbsOrigin(), m_vecSegmentStartPoint, vecPathDir );
		VectorNormalize( vecPathDir );
	}

	// Add in the horizontal component
	ComputePointFromPerpDistance( *pTarget, vecPathDir, flPerpDist, pTarget );
}


//-----------------------------------------------------------------------------
// Methods to find a signed perp distance from the track
// and to compute a point off the path based on the signed perp distance
//-----------------------------------------------------------------------------
float CAI_TrackPather::ComputePerpDistanceFromPath( const Vector &vecPointOnPath, const Vector &vecPathDir, const Vector &vecPointOffPath )
{
	// Make it be a signed distance of the target from the path
	// Positive means on the right side, negative means on the left side
	Vector vecAcross, vecDelta;
	CrossProduct( vecPathDir, Vector( 0, 0, 1 ), vecAcross );
	VectorSubtract( vecPointOffPath, vecPointOnPath, vecDelta );
	VectorMA( vecDelta, -DotProduct( vecPathDir, vecDelta ), vecPathDir, vecDelta );

	float flDistanceFromPath = vecDelta.Length2D();
	if ( DotProduct2D( vecAcross.AsVector2D(), vecDelta.AsVector2D() ) < 0.0f )
	{
		flDistanceFromPath *= -1.0f;
	}

	return flDistanceFromPath;
}

void CAI_TrackPather::ComputePointFromPerpDistance( const Vector &vecPointOnPath, const Vector &vecPathDir, float flPerpDist, Vector *pResult )
{
	Vector vecAcross;
	CrossProduct( vecPathDir, Vector( 0, 0, 1 ), vecAcross );
	VectorMA( vecPointOnPath, flPerpDist, vecAcross, *pResult );
}


//-----------------------------------------------------------------------------
// Finds the closest point on the path, returns a signed perpendicular distance
// where negative means on the left side of the path (when travelled from prev to next)
// and positive means on the right side
//-----------------------------------------------------------------------------
CPathTrack *CAI_TrackPather::FindClosestPointOnPath( CPathTrack *pPath, 
	const Vector &targetPos, Vector *pVecClosestPoint, Vector *pVecPathDir, float *pDistanceFromPath )
{
	// Find the node nearest to the destination path target if a path is not specified
	if ( pPath == NULL )
	{
		pPath = m_pDestPathTarget;
	}

	// If the path node we're trying to use is not valid, then we're done.
	if ( CPathTrack::ValidPath( pPath ) == NULL )
	{
		//FIXME: Implement
		Assert(0);
		return NULL;
	}

	// Find the nearest node to the target (going forward)
	CPathTrack *pNearestPath	= NULL;
	float		flNearestDist2D	= 999999999;
	float		flNearestDist	= 999999999;
	float		flPathDist, flPathDist2D;

	// NOTE: Gotta do it this crazy way because paths can be one-way.
	Vector vecNearestPoint;
	Vector vecNearestPathSegment;
	for ( int i = 0; i < 2; ++i )
	{
		int loopCheck = 0;
		CPathTrack *pTravPath = pPath;
		CPathTrack *pNextPath;

		BEGIN_PATH_TRACK_ITERATION();
		for ( ; CPathTrack::ValidPath( pTravPath ); pTravPath = pNextPath, loopCheck++ )
		{
			// Circular loop checking
			if ( pTravPath->HasBeenVisited() )
				break;

			// Mark it as being visited.
			pTravPath->Visit();

			pNextPath = (i == 0) ? pTravPath->GetPrevious() : pTravPath->GetNext();

			// No alt paths allowed in leading mode.
			if ( pTravPath->m_paltpath )
			{
				Warning( "%s: Alternative paths in path_track not allowed when using the leading behavior!\n", GetEntityName().ToCStr() );
			}

			// Need line segments
			if ( !CPathTrack::ValidPath(pNextPath) )
				break;

			// Find the closest point on the line segment on the path
			Vector vecClosest;
			CalcClosestPointOnLineSegment( targetPos, pTravPath->GetAbsOrigin(), pNextPath->GetAbsOrigin(), vecClosest );

			// Find the distance between this test point and our goal point
			flPathDist2D = vecClosest.AsVector2D().DistToSqr( targetPos.AsVector2D() );
			if ( flPathDist2D > flNearestDist2D )
				continue;

			flPathDist = vecClosest.z - targetPos.z;
			flPathDist *= flPathDist;
			flPathDist += flPathDist2D;
			if (( flPathDist2D == flNearestDist2D ) && ( flPathDist >= flNearestDist ))
				continue;

			pNearestPath	= (i == 0) ? pTravPath : pNextPath;
			flNearestDist2D	= flPathDist2D;
			flNearestDist	= flPathDist;
			vecNearestPoint	= vecClosest;
			VectorSubtract( pNextPath->GetAbsOrigin(), pTravPath->GetAbsOrigin(), vecNearestPathSegment );
			if ( i == 0 )
			{
				vecNearestPathSegment *= -1.0f;
			}
		}
	}

	VectorNormalize( vecNearestPathSegment );
	*pDistanceFromPath = ComputePerpDistanceFromPath( vecNearestPoint, vecNearestPathSegment, targetPos );
	*pVecClosestPoint = vecNearestPoint;
	*pVecPathDir = vecNearestPathSegment;
	return pNearestPath;
}


//-----------------------------------------------------------------------------
// Breakable paths?
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStartBreakableMovement( inputdata_t &inputdata )
{
	m_bPatrolBreakable = true;
}

void CAI_TrackPather::InputStopBreakableMovement( inputdata_t &inputdata )
{
	m_bPatrolBreakable = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStartPatrol( inputdata_t &inputdata )
{
	m_bPatrolling = true;	
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStopPatrol( inputdata_t &inputdata )
{
	m_bPatrolling = false;	
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStartPatrolBreakable( inputdata_t &inputdata )
{
	m_bPatrolBreakable = true;
	m_bPatrolling = true;	
}


//------------------------------------------------------------------------------
// Leading behaviors
//------------------------------------------------------------------------------
void CAI_TrackPather::InputStartLeading( inputdata_t &inputdata )
{
	EnableLeading( true );
	SetLeadingDistance( inputdata.value.Int() );
}

void CAI_TrackPather::InputStopLeading( inputdata_t &inputdata )
{
	EnableLeading( false );
}


//------------------------------------------------------------------------------
// Selects a new destination target
//------------------------------------------------------------------------------
void CAI_TrackPather::SelectNewDestTarget()
{
	if ( !m_bPatrolling )
		return;

	// NOTE: This version is bugged, but I didn't want to make the fix
	// here for fear of breaking a lot of maps late in the day.
	// So, only the chopper does the "right" thing.
#ifdef HL2_EPISODIC 
	// Episodic uses the fixed logic for all trackpathers
	if ( 1 )
#else
	if ( ShouldUseFixedPatrolLogic() )
#endif
	{
		CPathTrack *pOldDest = m_pDestPathTarget;

		// Only switch polarity of movement if we're at the *end* of the path
		// This is really useful for initial conditions of patrolling
		// NOTE: We've got to do some extra work for circular paths
		bool bIsCircular = false;
		{
 			BEGIN_PATH_TRACK_ITERATION();
			CPathTrack *pTravPath = m_pDestPathTarget;
 			while( CPathTrack::ValidPath( pTravPath ) )
			{
				// Circular loop checking
				if ( pTravPath->HasBeenVisited() )
				{
					bIsCircular = true;
					break;
				}

				pTravPath->Visit();
				pTravPath = NextAlongCurrentPath( pTravPath );
			}
		}

		if ( bIsCircular || (NextAlongCurrentPath( m_pDestPathTarget ) == NULL) )
		{
			m_bMovingForward = !m_bMovingForward;
		}

		BEGIN_PATH_TRACK_ITERATION();

		while ( true )
		{
			CPathTrack *pNextTrack = NextAlongCurrentPath( m_pDestPathTarget );
			if ( !pNextTrack || (pNextTrack == pOldDest) || pNextTrack->HasBeenVisited() )
				break;

			pNextTrack->Visit();
			m_pDestPathTarget = pNextTrack;
		}
	}
	else
	{
		CPathTrack *pOldDest = m_pDestPathTarget;

		// For patrolling, switch the polarity of movement
		m_bMovingForward = !m_bMovingForward;

		int loopCount = 0;
		while ( true )
		{
			CPathTrack *pNextTrack = NextAlongCurrentPath( m_pDestPathTarget );
			if ( !pNextTrack )
				break;
			if ( ++loopCount > 1024 )
			{
				DevMsg(1,"WARNING: Looping path for %s\n", GetDebugName() );
				break;
			}

			m_pDestPathTarget = pNextTrack;
		}

		if ( m_pDestPathTarget == pOldDest )
		{
			// This can occur if we move to the first point on the path
			SelectNewDestTarget();
		}
	}
}


//------------------------------------------------------------------------------
// Moves to the track
//------------------------------------------------------------------------------
void CAI_TrackPather::UpdateCurrentTargetLeading()
{
	bool bRestingAtDest = false;
	CPathTrack *pAdjustedDest;

	// Find the point along the line that we're closest to.
	const Vector &vecTarget = m_pCurrentPathTarget->GetAbsOrigin();
	Vector vecPoint;
	float t = ClosestPointToCurrentPath( &vecPoint );
	if ( (t < 1.0f) && ( vecPoint.DistToSqr( vecTarget ) > m_flTargetTolerance * m_flTargetTolerance ) )
		goto visualizeDebugInfo;

	// Trip our "path_track reached" output
	if ( m_pCurrentPathTarget != m_pLastPathTarget )
	{
		// Get the path's specified max speed
		m_flPathMaxSpeed = m_pCurrentPathTarget->m_flSpeed;

		variant_t emptyVariant;
		m_pCurrentPathTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );
		m_pLastPathTarget = m_pCurrentPathTarget;
	}

	// NOTE: CurrentPathTarget doesn't mean the same thing as dest path target!
	// It's the "next"most when moving forward + "prev"most when moving backward
	// Must do the tests in the same space
	pAdjustedDest = AdjustForMovementDirection( m_pDestPathTarget );

	// Update our dest path target, if appropriate...
	if ( m_pCurrentPathTarget == pAdjustedDest )
	{
		m_bForcedMove = false;
		SelectNewDestTarget();

		// NOTE: Must do this again since SelectNewDestTarget may change m_pDestPathTarget
		pAdjustedDest = AdjustForMovementDirection( m_pDestPathTarget );
	}

	if ( m_pCurrentPathTarget != pAdjustedDest )
	{
		// Update to the next path, if there is one...
		m_pCurrentPathTarget = NextAlongCurrentPath( m_pCurrentPathTarget );
		if ( !m_pCurrentPathTarget )
		{
			m_pCurrentPathTarget = m_pLastPathTarget;
		}
	}
	else
	{
		// NOTE: Have to do this here because the NextAlongCurrentPath call above
		// could make m_pCurrentPathTarget == m_pDestPathTarget.
		// In this case, we're at rest (no patrolling behavior)
		bRestingAtDest = true;
	}

	if ( bRestingAtDest )
	{
		// NOTE: Must use current path target, instead of dest
		// to get the PreviousAlongCurrentPath working correctly
		CPathTrack *pSegmentStart = PreviousAlongCurrentPath( m_pCurrentPathTarget ); 
		if ( !pSegmentStart )
		{
			pSegmentStart = m_pCurrentPathTarget;
		}
		m_vecSegmentStartPoint = pSegmentStart->GetAbsOrigin();
	}
	else
	{
		m_vecSegmentStartPoint = m_pLastPathTarget->GetAbsOrigin();
	}

visualizeDebugInfo:
	VisualizeDebugInfo( vecPoint, vecTarget );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdateTargetPositionLeading( void )
{
	Vector targetPos;
	if ( !GetTrackPatherTarget( &targetPos ) )
		return;

	// NOTE: FindClosestPointOnPath *always* returns the point on the "far",
	// end of the line segment containing the closest point (namely the 'next'
	// track, as opposed to the 'prev' track)
	Vector vecClosestPoint, vecPathDir;
	float flTargetDistanceFromPath;
	CPathTrack *pNextPath = FindClosestPointOnPath( m_pCurrentPathTarget, 
		targetPos, &vecClosestPoint, &vecPathDir, &flTargetDistanceFromPath );

	// This means that a valid path could not be found to our target!
	if ( CPathTrack::ValidPath( pNextPath ) == NULL )
		return;

//	NDebugOverlay::Cross3D( vecClosestPoint, -Vector(24,24,24), Vector(24,24,24), 0, 255, 255, true, 0.1f );
//	NDebugOverlay::Cross3D( pNextPath->GetAbsOrigin(), -Vector(24,24,24), Vector(24,24,24), 255, 255, 0, true, 0.1f );

	// Here's how far we are from the path
	m_flTargetDistFromPath = flTargetDistanceFromPath;
	m_vecTargetPathDir = vecPathDir;

	// Here's info about where the target is along the path
	m_vecTargetPathPoint = vecClosestPoint;
	m_pTargetNearestPath = pNextPath;

	// Find the best position to be on our path
	// NOTE: This will *also* return a path track on the "far" end of the line segment
	// containing the leading position, namely the "next" end of the segment as opposed
	// to the "prev" end of the segment.
	CPathTrack *pDest = ComputeLeadingPointAlongPath( vecClosestPoint, pNextPath, m_flLeadDistance, &targetPos );
	SetDesiredPosition( targetPos );

	// We only want to switch movement directions when absolutely necessary
	// so convert dest into a more appropriate value based on the current movement direction
	if ( pDest != m_pDestPathTarget )
	{
		// NOTE: This is really tricky + subtle
		// For leading, we don't want to ever change direction when the current target == the
		// adjusted destination target. Namely, if we're going forward, both dest + curr
		// mean the "next"most node so we can compare them directly against eath other.
		// If we're moving backward, dest means "next"most, but curr means "prev"most.
		// We first have to adjust the dest to mean "prev"most, and then do the comparison.
		// If the adjusted dest == curr, then maintain direction. Otherwise, use the forward along path test.
		bool bMovingForward = m_bMovingForward;
		CPathTrack *pAdjustedDest = AdjustForMovementDirection( pDest );
		if ( m_pCurrentPathTarget != pAdjustedDest )
		{
			bMovingForward = IsForwardAlongPath( m_pCurrentPathTarget, pAdjustedDest );
		}

		if ( bMovingForward != m_bMovingForward )
		{
			// As a result of the tricky note above, this should never occur
			Assert( pAdjustedDest != m_pCurrentPathTarget );

			// Oops! Need to reverse direction
			m_bMovingForward = bMovingForward;
			m_vecSegmentStartPoint = m_pCurrentPathTarget->GetAbsOrigin();
			m_pCurrentPathTarget = NextAlongCurrentPath( m_pCurrentPathTarget );
		}
		m_pDestPathTarget = pDest;
	}

//	NDebugOverlay::Cross3D( m_pCurrentPathTarget->GetAbsOrigin(), -Vector(36,36,36), Vector(36,36,36), 255, 0, 0, true, 0.1f );
//	NDebugOverlay::Cross3D( m_pDestPathTarget->GetAbsOrigin(), -Vector(48,48,48), Vector(48,48,48), 0, 255, 0, true, 0.1f );
//	NDebugOverlay::Cross3D( targetPos, -Vector(36,36,36), Vector(36,36,36), 0, 0, 255, true, 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdateTargetPosition( void )
{
	// Don't update our target if we're being told to go somewhere
	if ( m_bForcedMove && !m_bPatrolBreakable )
		return;

	// Don't update our target if we're patrolling
	if ( m_bPatrolling )
	{
		// If we have an enemy, and our patrol is breakable, stop patrolling
		if ( !m_bPatrolBreakable || !GetEnemy() )
			return;

		m_bPatrolling = false;
	}

	Vector targetPos;
	if ( !GetTrackPatherTarget( &targetPos ) )
		return;

	// Not time to update again
	if ( m_flEnemyPathUpdateTime > gpGlobals->curtime )
		return;

	// See if the target has moved enough to make us recheck
	float flDistSqr = ( targetPos - m_vecLastGoalCheckPosition ).LengthSqr();
	if ( flDistSqr < m_flTargetDistanceThreshold * m_flTargetDistanceThreshold )
		return;

	// Find the best position to be on our path
	CPathTrack *pDest = BestPointOnPath( m_pCurrentPathTarget, targetPos, m_flAvoidDistance, true, m_bChooseFarthestPoint );

	if ( CPathTrack::ValidPath( pDest ) == NULL )
	{
		// This means that a valid path could not be found to our target!
//		Assert(0);
		return;
	}

	if ( pDest != m_pDestPathTarget )
	{
		// This is our new destination
		bool bMovingForward = IsForwardAlongPath( m_pCurrentPathTarget, pDest );
		if ( bMovingForward != m_bMovingForward )
		{
			// Oops! Need to reverse direction
			m_bMovingForward = bMovingForward;
			if ( pDest != m_pCurrentPathTarget )
			{
				SetupNewCurrentTarget( NextAlongCurrentPath( m_pCurrentPathTarget ) );
			}
		}
		m_pDestPathTarget = pDest;
	}

	// Keep this goal point for comparisons later
	m_vecLastGoalCheckPosition = targetPos;
	
	// Only do this on set intervals
	m_flEnemyPathUpdateTime	= gpGlobals->curtime + 1.0f;
}


//------------------------------------------------------------------------------
// Returns the direction of the path at the closest point to the target
//------------------------------------------------------------------------------
const Vector &CAI_TrackPather::TargetPathDirection() const
{
	return m_vecTargetPathDir;
}

const Vector &CAI_TrackPather::TargetPathAcrossDirection() const
{
	static Vector s_Result;
	CrossProduct( m_vecTargetPathDir, Vector( 0, 0, 1 ), s_Result );
	return s_Result;
}


//------------------------------------------------------------------------------
// Returns the speed of the target relative to the path
//------------------------------------------------------------------------------
float CAI_TrackPather::TargetSpeedAlongPath() const
{
	if ( !GetEnemy() || !IsLeading() )
		return 0.0f;

	Vector vecSmoothedVelocity = GetEnemy()->GetSmoothedVelocity();
	return DotProduct( vecSmoothedVelocity, TargetPathDirection() );
}


//------------------------------------------------------------------------------
// Returns the speed of the target *across* the path
//------------------------------------------------------------------------------
float CAI_TrackPather::TargetSpeedAcrossPath() const
{
	if ( !GetEnemy() || !IsLeading() )
		return 0.0f;

	Vector vecSmoothedVelocity = GetEnemy()->GetSmoothedVelocity();
	return DotProduct( vecSmoothedVelocity, TargetPathAcrossDirection() );
}


//------------------------------------------------------------------------------
// Returns the max distance we can be from the path
//------------------------------------------------------------------------------
float CAI_TrackPather::MaxDistanceFromCurrentPath() const
{
	if ( !IsLeading() || !m_pCurrentPathTarget )
		return 0.0f;

	CPathTrack *pPrevPath = PreviousAlongCurrentPath( m_pCurrentPathTarget );
	if ( !pPrevPath )
	{
		pPrevPath = m_pCurrentPathTarget;
	}

	// NOTE: Can't use m_vecSegmentStartPoint because we don't have a radius defined for it
	float t;
	Vector vecTemp;
	CalcClosestPointOnLine( GetAbsOrigin(), pPrevPath->GetAbsOrigin(), 
		m_pCurrentPathTarget->GetAbsOrigin(), vecTemp, &t );
	t = clamp( t, 0.0f, 1.0f );
	float flRadius = (1.0f - t) * pPrevPath->GetRadius() + t * m_pCurrentPathTarget->GetRadius(); 
	return flRadius;
}


//------------------------------------------------------------------------------
// Purpose : A different version of the track pather which is more explicit about
// the meaning of dest, current, and prev path points
//------------------------------------------------------------------------------
void CAI_TrackPather::UpdateTrackNavigation( void )
{
	// No target? Use the string specified. We have no spawn method (sucky!!) so this is how that works
	if ( ( CPathTrack::ValidPath( m_pDestPathTarget ) == NULL ) && ( m_target != NULL_STRING ) )
	{
		FlyToPathTrack( m_target );
		m_target = NULL_STRING;
	}

	if ( !IsLeading() )
	{
		if ( !m_pCurrentPathTarget )
			return;

		// Updates our destination node if we're tracking something
		UpdateTargetPosition();

		// Move along our path towards our current destination
		UpdateCurrentTarget();
	}
	else
	{
		// Updates our destination position if we're leading something
		UpdateTargetPositionLeading();

		// Move along our path towards our current destination
		UpdateCurrentTargetLeading();
	}
}


//------------------------------------------------------------------------------
// Sets the farthest path distance
//------------------------------------------------------------------------------
void CAI_TrackPather::SetFarthestPathDist( float flMaxPathDist )
{
	m_flFarthestPathDist = flMaxPathDist;
}


//------------------------------------------------------------------------------
// Sets up a new current path target
//------------------------------------------------------------------------------
void CAI_TrackPather::SetupNewCurrentTarget( CPathTrack *pTrack )
{
	Assert( pTrack );
	m_vecSegmentStartPoint = GetAbsOrigin();
	VectorMA( m_vecSegmentStartPoint, -2.0f, GetAbsVelocity(), m_vecSegmentStartSplinePoint );
	m_pCurrentPathTarget = pTrack;
	SetDesiredPosition( m_pCurrentPathTarget->GetAbsOrigin() );
}


//------------------------------------------------------------------------------
// Moves to an explicit track point
//------------------------------------------------------------------------------
void CAI_TrackPather::MoveToTrackPoint( CPathTrack *pTrack )
{
	if ( IsOnSameTrack( pTrack, m_pDestPathTarget ) )
	{
		// The track must be valid
		if ( CPathTrack::ValidPath( pTrack ) == NULL )
			return;

		m_pDestPathTarget = pTrack;
		m_bMovingForward = IsForwardAlongPath( m_pCurrentPathTarget, pTrack );
		m_bForcedMove = true;
	}
	else
	{
		CPathTrack *pClosestTrack = BestPointOnPath( pTrack, WorldSpaceCenter(), 0.0f, false, false );

		// The track must be valid
		if ( CPathTrack::ValidPath( pClosestTrack ) == NULL )
			return;

		SetupNewCurrentTarget( pClosestTrack );
		m_pDestPathTarget = pTrack;
		m_bMovingForward = IsForwardAlongPath( pClosestTrack, pTrack );
		m_bForcedMove = true;
	}
}


//------------------------------------------------------------------------------
// Moves to the closest track point
//------------------------------------------------------------------------------
void CAI_TrackPather::MoveToClosestTrackPoint( CPathTrack *pTrack )
{
	if ( IsOnSameTrack( pTrack, m_pDestPathTarget ) )
		return;

	CPathTrack *pClosestTrack = BestPointOnPath( pTrack, WorldSpaceCenter(), 0.0f, false, false );

	// The track must be valid
	if ( CPathTrack::ValidPath( pClosestTrack ) == NULL )
		return;

	SetupNewCurrentTarget( pClosestTrack );
	m_pDestPathTarget = pClosestTrack;
	m_bMovingForward = true;

	// Force us to switch tracks if we're leading
	if ( IsLeading() )
	{
		m_bForcedMove = true;
	}
}


//-----------------------------------------------------------------------------
// Are the two path tracks connected?
//-----------------------------------------------------------------------------
bool CAI_TrackPather::IsOnSameTrack( CPathTrack *pPath1, CPathTrack *pPath2 ) const
{
	if ( pPath1 == pPath2 )
		return true;

	{
		BEGIN_PATH_TRACK_ITERATION();
		CPathTrack *pTravPath = pPath1->GetPrevious();
		while( CPathTrack::ValidPath( pTravPath ) && (pTravPath != pPath1) )
		{
			// Circular loop checking
			if ( pTravPath->HasBeenVisited() )
				break;

			pTravPath->Visit();

			if ( pTravPath == pPath2 )
				return true;

			pTravPath = pTravPath->GetPrevious();
		}
	}

	{
		BEGIN_PATH_TRACK_ITERATION();
		CPathTrack *pTravPath = pPath1->GetNext();
		while( CPathTrack::ValidPath( pTravPath ) && (pTravPath != pPath1) )
		{
			// Circular loop checking
			if ( pTravPath->HasBeenVisited() )
				break;

			pTravPath->Visit();

			if ( pTravPath == pPath2 )
				return true;

			pTravPath = pTravPath->GetNext();
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Deal with teleportation
//-----------------------------------------------------------------------------
void CAI_TrackPather::Teleported()
{
	// This updates the paths so they are reasonable
	CPathTrack *pClosestTrack = BestPointOnPath( GetDestPathTarget(), WorldSpaceCenter(), 0.0f, false, false );
	m_pDestPathTarget = NULL;
	MoveToClosestTrackPoint( pClosestTrack );
}

	
//-----------------------------------------------------------------------------
// Returns distance along path to target, returns FLT_MAX if there's no path
//-----------------------------------------------------------------------------
float CAI_TrackPather::ComputePathDistance( CPathTrack *pPath, CPathTrack *pDest, bool bForward ) const
{
	float flDist = 0.0f;
	CPathTrack *pLast = pPath;

	BEGIN_PATH_TRACK_ITERATION();
	while ( CPathTrack::ValidPath( pPath ) )
	{
		// Ciruclar loop checking
		if ( pPath->HasBeenVisited() )
			return FLT_MAX;

		pPath->Visit();

		flDist += pLast->GetAbsOrigin().DistTo( pPath->GetAbsOrigin() );

		if ( pDest == pPath )
			return flDist;

		pLast = pPath;
		pPath = bForward ? pPath->GetNext() : pPath->GetPrevious();
	}

	return FLT_MAX;
}


//-----------------------------------------------------------------------------
// Is pPathTest in "front" of pPath on the same path? (Namely, does GetNext() get us there?)
//-----------------------------------------------------------------------------
bool CAI_TrackPather::IsForwardAlongPath( CPathTrack *pPath, CPathTrack *pPathTest ) const
{
	// Also, in the case of looping paths, we want to return the shortest path
	float flForwardDist = ComputePathDistance( pPath, pPathTest, true );
	float flReverseDist = ComputePathDistance( pPath, pPathTest, false );

	Assert( ( flForwardDist != FLT_MAX ) || ( flReverseDist != FLT_MAX ) );
	return ( flForwardDist <= flReverseDist );
}


//-----------------------------------------------------------------------------
// Computes distance + nearest point from the current path..
//-----------------------------------------------------------------------------
float CAI_TrackPather::ClosestPointToCurrentPath( Vector *pVecPoint ) const
{
	if (!m_pCurrentPathTarget)
	{
		*pVecPoint = GetAbsOrigin();
		return 0;
	}

	float t;
	CalcClosestPointOnLine( GetAbsOrigin(), m_vecSegmentStartPoint, 
		m_pCurrentPathTarget->GetAbsOrigin(), *pVecPoint, &t );
	return t;
}


//-----------------------------------------------------------------------------
// Computes a "path" velocity at a particular point along the current path
//-----------------------------------------------------------------------------
void CAI_TrackPather::ComputePathTangent( float t, Vector *pVecTangent ) const
{
	CPathTrack *pNextTrack = NextAlongCurrentPath(m_pCurrentPathTarget);
	if ( !pNextTrack )
	{
		pNextTrack = m_pCurrentPathTarget;
	}

	t = clamp( t, 0.0f, 1.0f );
	pVecTangent->Init(0,0,0);
	Catmull_Rom_Spline_Tangent( m_vecSegmentStartSplinePoint, m_vecSegmentStartPoint, 
		m_pCurrentPathTarget->GetAbsOrigin(), pNextTrack->GetAbsOrigin(), t, *pVecTangent );
	VectorNormalize( *pVecTangent );
}


//-----------------------------------------------------------------------------
// Computes the *normalized* velocity at which the helicopter should approach the final point
//-----------------------------------------------------------------------------
void CAI_TrackPather::ComputeNormalizedDestVelocity( Vector *pVecVelocity ) const
{
	if ( m_nPauseState != PAUSE_NO_PAUSE )
	{
		pVecVelocity->Init(0,0,0);
		return;
	}

	CPathTrack *pNextTrack = NextAlongCurrentPath(m_pCurrentPathTarget);
	if ( !pNextTrack )
	{
		pNextTrack = m_pCurrentPathTarget;
	}

	if ( ( pNextTrack == m_pCurrentPathTarget ) || ( m_pCurrentPathTarget == m_pDestPathTarget ) )
	{
		pVecVelocity->Init(0,0,0);
		return;
	}

	VectorSubtract( pNextTrack->GetAbsOrigin(), m_pCurrentPathTarget->GetAbsOrigin(), *pVecVelocity );
	VectorNormalize( *pVecVelocity );

	// Slow it down if we're approaching a sharp corner
	Vector vecDelta;
	VectorSubtract( m_pCurrentPathTarget->GetAbsOrigin(), m_vecSegmentStartPoint, vecDelta );
	VectorNormalize( vecDelta );
	float flDot = DotProduct( *pVecVelocity, vecDelta );
	*pVecVelocity *= clamp( flDot, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::SetTrack( CBaseEntity *pGoalEnt )
{
	// Ignore this input if we're *already* on that path.
	CPathTrack *pTrack = dynamic_cast<CPathTrack *>(pGoalEnt);
	if ( !pTrack )
	{
		DevWarning( "%s: Specified entity '%s' must be a path_track!\n", pGoalEnt->GetClassname(), pGoalEnt->GetEntityName().ToCStr() );
		return;
	}

	MoveToClosestTrackPoint( pTrack );
}

void CAI_TrackPather::SetTrack( string_t strTrackName )
{
	// Find our specified target
	CBaseEntity *pGoalEnt = gEntList.FindEntityByName( NULL, strTrackName );
	if ( pGoalEnt == NULL )
	{
		DevWarning( "%s: Could not find path_track '%s'!\n", GetClassname(), STRING( strTrackName ) );
		return;
	}

	SetTrack( pGoalEnt );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputSetTrack( inputdata_t &inputdata )
{
	string_t strTrackName = MAKE_STRING( inputdata.value.String() );
	SetTrack( MAKE_STRING( inputdata.value.String() ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strTrackName - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::FlyToPathTrack( string_t strTrackName )
{
	CBaseEntity *pGoalEnt = gEntList.FindEntityByName( NULL, strTrackName );
	if ( pGoalEnt == NULL )
	{
		DevWarning( "%s: Could not find path_track '%s'!\n", GetClassname(), STRING( strTrackName ) );
		return;
	}

	// Ignore this input if we're *already* on that path.
	CPathTrack *pTrack = dynamic_cast<CPathTrack *>(pGoalEnt);
	if ( !pTrack )
	{
		DevWarning( "%s: Specified entity '%s' must be a path_track!\n", GetClassname(), STRING( strTrackName ) );
		return;
	}

	// Find our specified target
	MoveToTrackPoint( pTrack );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputFlyToPathTrack( inputdata_t &inputdata )
{ 
	// Find our specified target
	string_t strTrackName = MAKE_STRING( inputdata.value.String() );
	m_nPauseState = PAUSE_NO_PAUSE;
	FlyToPathTrack( strTrackName );
}


//-----------------------------------------------------------------------------
// Changes the mode used to determine which path point to move to
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputChooseFarthestPathPoint( inputdata_t &inputdata )
{
	UseFarthestPathPoint( true );
}

void CAI_TrackPather::InputChooseNearestPathPoint( inputdata_t &inputdata )
{
	UseFarthestPathPoint( false );
}

void CAI_TrackPather::UseFarthestPathPoint( bool useFarthest )
{
	m_bChooseFarthestPoint = useFarthest;
}
