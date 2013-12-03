//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_TRACKPATHER_H
#define AI_TRACKPATHER_H

#if defined( _WIN32 )
#pragma once
#endif


#include "ai_basenpc.h"


class CPathTrack;

//------------------------------------------------------------------------------

class CAI_TrackPather : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_TrackPather, CAI_BaseNPC );
	DECLARE_DATADESC();
public:

	bool			IsOnPathTrack()							{ return (m_pCurrentPathTarget != NULL); }

protected:	
	void			InitPathingData( float flTrackArrivalTolerance, float flTargetDistance, float flAvoidDistance );
	virtual bool	GetTrackPatherTarget( Vector *pPos ) { return false; }
	virtual CBaseEntity *GetTrackPatherTargetEnt()	{ return NULL; }

	const Vector &	GetDesiredPosition() const				{ return m_vecDesiredPosition; 	}
	void 			SetDesiredPosition( const Vector &v )	{ m_vecDesiredPosition = v; 	}
	const Vector &	GetGoalOrientation() const				{ return m_vecGoalOrientation; 	}
	void 			SetGoalOrientation( const Vector &v )	{ m_vecGoalOrientation = v; 	}

	bool			CurPathTargetIsDest()					{ return ( m_pDestPathTarget == m_pCurrentPathTarget ); }

	virtual bool	HasReachedTarget( void ) 				{ return (WorldSpaceCenter() - m_vecDesiredPosition).Length() < 128; }

	CPathTrack *	GetDestPathTarget()						{ return m_pDestPathTarget;		}

	bool			IsInForcedMove() const					{ return m_bForcedMove;			}
	void			ClearForcedMove()						{ m_bForcedMove = false;		}

	float			GetPathMaxSpeed() const					{ return m_flPathMaxSpeed;		}

	void			OnSave( IEntitySaveUtils *pUtils );
	void			OnRestore( void );

protected:
	enum PauseState_t
	{
		PAUSE_NO_PAUSE = 0,
		PAUSED_AT_POSITION,
		PAUSE_AT_NEXT_LOS_POSITION,

		PAUSE_FORCE_DWORD = 0xFFFFFFFF,
	};

	// Sets a track
	void SetTrack( string_t strTrackName );
	void SetTrack( CBaseEntity *pGoalEnt );

	// Fly to a particular track point via the path
	virtual void InputFlyToPathTrack( inputdata_t &inputdata );

	// Updates the nav target if we've reached it
	void UpdateTrackNavigation( void );

	// Computes distance + nearest point from the current path..
	float ClosestPointToCurrentPath( Vector *pVecPoint ) const;

	// Computes a "path" velocity at a particular point along the current path
	void ComputePathTangent( float t, Vector *pVecTangent ) const;

	// Computes the *normalized* velocity at which the helicopter should approach the final point
	void ComputeNormalizedDestVelocity( Vector *pVecVelocity ) const;

	// Sets the farthest path distance
	void SetFarthestPathDist( float flMaxPathDist );

	// Returns the next/previous path along our current path
	CPathTrack *NextAlongCurrentPath( CPathTrack *pPath ) const;
	CPathTrack *PreviousAlongCurrentPath( CPathTrack *pPath ) const;

	// Adjusts a "next"most node based on the current movement direction
	CPathTrack *AdjustForMovementDirection( CPathTrack *pPath ) const;

	// Enemy visibility check
	virtual CBaseEntity *FindTrackBlocker( const Vector &vecViewPoint, const Vector &vecTargetPos );

	// Compute a point n units along a path
	void ComputePointAlongPath( const Vector &vecStartPoint, float flDistance, Vector *pTarget );

	// Are we leading?
	bool IsLeading() const { return m_bLeading && !m_bForcedMove; }

	// Leading + leading distance
	void EnableLeading( bool bEnable );
	void SetLeadingDistance( float flLeadDistance );
	float GetLeadingDistance( ) const;

	// Compute a point n units along the current path from our current position
	// (but don't pass the desired target point)
	void ComputePointAlongCurrentPath( float flDistance, float flPerpDist, Vector *pTarget );

	// Returns the perpendicular distance of the target from the nearest path point
	float TargetDistanceToPath() const { return m_flTargetDistFromPath; }

	// Returns the speed of the target relative to the path
	float TargetSpeedAlongPath() const;

	// Returns the speed of the target *across* the path
	float TargetSpeedAcrossPath() const;

	// Compute a path direction
	void ComputePathDirection( CPathTrack *pPath, Vector *pVecPathDir );

	// What's the current path direction?
	void CurrentPathDirection( Vector *pVecPathDir );

	// Returns the max distance we can be from the path
	float MaxDistanceFromCurrentPath() const;

	// true to use farthest, false for nearest
	void UseFarthestPathPoint( bool useFarthest );

	// Moves to an explicit track point
	void MoveToTrackPoint( CPathTrack *pTrack );

	// Sets up a new current path target
	void SetupNewCurrentTarget( CPathTrack *pTrack );

	// Compute the distance to the leading position
	float ComputeDistanceToLeadingPosition();

	// Compute the distance to the target position
	float ComputeDistanceToTargetPosition();

	// Set the pause state.
	void SetPauseState( PauseState_t pauseState ) { m_nPauseState = pauseState; }

	// Does this path track have LOS to the target?
	bool HasLOSToTarget( CPathTrack *pTrack );

	// FIXME: Work this back into the base class
	virtual bool ShouldUseFixedPatrolLogic() { return false; }

	// Deal with teleportation
	void Teleported();

private:

	CPathTrack		*BestPointOnPath( CPathTrack *pPath, const Vector &targetPos, float avoidRadius, bool visible, bool bFarthestPointOnPath );

	// Input methods
	void InputSetTrack( inputdata_t &inputdata );
	void InputChooseFarthestPathPoint( inputdata_t &inputdata );
	void InputChooseNearestPathPoint( inputdata_t &inputdata );
	void InputStartBreakableMovement( inputdata_t &inputdata );
	void InputStopBreakableMovement( inputdata_t &inputdata );
	void InputStartPatrol( inputdata_t &inputdata );
	void InputStopPatrol( inputdata_t &inputdata );
	void InputStartLeading( inputdata_t &inputdata );
	void InputStopLeading( inputdata_t &inputdata );

	// Obsolete, for backward compatibility
	void InputStartPatrolBreakable( inputdata_t &inputdata );

	// Flies to a point on a track
	void FlyToPathTrack( string_t strTrackName );

	// Selects a new destination target
	void SelectNewDestTarget();

	// Makes sure we've picked the right position along the path if we're chasing an enemy
	void UpdateTargetPosition( );

	// Moves to the track
	void UpdateCurrentTarget();
	void UpdateCurrentTargetLeading();

	// Track debugging info
	void VisualizeDebugInfo( const Vector &vecNearestPoint, const Vector &vecTarget );

	// Moves to the closest track point
	void MoveToClosestTrackPoint( CPathTrack *pTrack );

	// Are the two path tracks connected?
	bool IsOnSameTrack( CPathTrack *pPath1, CPathTrack *pPath2 ) const;

	// Is pPathTest in "front" of pPath on the same path? (Namely, does GetNext() get us there?)
	bool IsForwardAlongPath( CPathTrack *pPath, CPathTrack *pPathTest ) const;

	// Purpose: 
	void UpdateTargetPositionLeading( void );

	// Compute a point n units along a path
	CPathTrack *ComputeLeadingPointAlongPath( const Vector &vecStartPoint, CPathTrack *pFirstTrack, float flDistance, Vector *pTarget );

	// Finds the closest point on the path, returns a signed perpendicular distance
	CPathTrack *FindClosestPointOnPath( CPathTrack *pPath, const Vector &targetPos, Vector *pVecClosestPoint, Vector *pVecPathDir, float *pDistanceFromPath );

	// Methods to find a signed perp distance from the track
	// and to compute a point off the path based on the signed perp distance
	float ComputePerpDistanceFromPath( const Vector &vecPointOnPath, const Vector &vecPathDir, const Vector &vecPointOffPath );
	void ComputePointFromPerpDistance( const Vector &vecPointOnPath, const Vector &vecPathDir, float flPerpDist, Vector *pResult );

	// Returns the direction of the path at the closest point to the target
	const Vector &TargetPathDirection() const;
	const Vector &TargetPathAcrossDirection() const;

	// Returns distance along path to target, returns -1 if there's no path
	float ComputePathDistance( CPathTrack *pStart, CPathTrack *pDest, bool bForward ) const;

	// Compute the distance to a particular point on the path
	float ComputeDistanceAlongPathToPoint( CPathTrack *pStartTrack, CPathTrack *pDestTrack, const Vector &vecDestPosition, bool bMovingForward );

private:
	//---------------------------------
	Vector			m_vecDesiredPosition;
	Vector			m_vecGoalOrientation; // orientation of the goal entity.

	// NOTE: CurrentPathTarget changes meaning based on movement direction
	// For this *after* means the "next" (m_pnext) side of the line segment
	// and "before" means the "prev" (m_pprevious) side of the line segment
	// CurrentPathTarget is *after* the desired point when moving forward, 
	// and *before* the desired point when moving backward.
	// DestPathTarget + TargetNearestPath always represent points
	// *after* the desired point.
	CHandle<CPathTrack> m_pCurrentPathTarget;
	CHandle<CPathTrack> m_pDestPathTarget;
	CHandle<CPathTrack> m_pLastPathTarget;
	CHandle<CPathTrack> m_pTargetNearestPath;	// Used only by leading, it specifies the path point *after* where the target is

	string_t		m_strCurrentPathName;
	string_t		m_strDestPathName;
	string_t		m_strLastPathName;
	string_t		m_strTargetNearestPathName;

	Vector			m_vecLastGoalCheckPosition;	// Last position checked for moving towards
	float			m_flEnemyPathUpdateTime;	// Next time to update our enemies position
	bool			m_bForcedMove;				// Means the destination point must be reached regardless of enemy position
	bool			m_bPatrolling;				// If set, move back and forth along the current track until we see an enemy
	bool			m_bPatrolBreakable;			// If set, I'll stop patrolling if I see an enemy
	bool			m_bLeading;					// If set, we can lead our enemies

	// Derived class pathing data
	float			m_flTargetDistanceThreshold;// Distance threshold used to determine when a target has moved enough to update our navigation to it
	float			m_flAvoidDistance;			//
	
	float			m_flTargetTolerance;		// How far from a path track do we need to be before we 'reached' it?
	Vector			m_vecSegmentStartPoint;		// Starting point for the current segment
	Vector			m_vecSegmentStartSplinePoint;	// Used to define a spline which is used to compute path velocity
	bool			m_bMovingForward;
	bool			m_bChooseFarthestPoint;
	float			m_flFarthestPathDist;		// How far from a path track do we need to be before we 'reached' it?

	float			m_flPathMaxSpeed;
	float			m_flTargetDistFromPath;		// How far is the target from the closest point on the path?
	float			m_flLeadDistance;
	Vector			m_vecTargetPathDir;
	Vector			m_vecTargetPathPoint;		// What point on the path is closest to the target?

	PauseState_t	m_nPauseState;
};

//------------------------------------------------------------------------------

#endif // AI_TRACKPATHER_H
