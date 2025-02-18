// NextBotPath.h
// Encapsulate and manipulate a path through the world
// Author: Michael Booth, February 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_PATH_H_
#define _NEXT_BOT_PATH_H_

#include "NextBotInterface.h"

#include "tier0/vprof.h"

#define PATH_NO_LENGTH_LIMIT 0.0f				// non-default argument value for Path::Compute()
#define PATH_TRUNCATE_INCOMPLETE_PATH false		// non-default argument value for Path::Compute()

class INextBot;
class CNavArea;
class CNavLadder;


//---------------------------------------------------------------------------------------------------------------
/**
 * The interface for pathfinding costs.
 * TODO: Replace all template cost functors with this interface, so we can virtualize and derive from them.
 */
class IPathCost
{
public:
	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const = 0;
};


//---------------------------------------------------------------------------------------------------------------
/**
 * The interface for selecting a goal area during "open goal" pathfinding
 */
class IPathOpenGoalSelector
{
public:
	// compare "newArea" to "currentGoal" and return the area that is the better goal area
	virtual CNavArea *operator() ( CNavArea *currentGoal, CNavArea *newArea ) const = 0;
};


//---------------------------------------------------------------------------------------------------------------
/**
 * A Path through the world.
 * Not only does this encapsulate a path to get from point A to point B,
 * but also the selecting the decision algorithm for how to build that path.
 */
class Path
{
public:
	Path( void );
	virtual ~Path() { }
	
	enum SegmentType
	{
		ON_GROUND,
		DROP_DOWN,
		CLIMB_UP,
		JUMP_OVER_GAP,
		LADDER_UP,
		LADDER_DOWN,
		
		NUM_SEGMENT_TYPES
	};

	// @todo Allow custom Segment classes for different kinds of paths	
	struct Segment
	{
		CNavArea *area;									// the area along the path
		NavTraverseType how;							// how to enter this area from the previous one
		Vector pos;										// our movement goal position at this point in the path
		const CNavLadder *ladder;						// if "how" refers to a ladder, this is it
		
		SegmentType type;								// how to traverse this segment of the path
		Vector forward;									// unit vector along segment
		float length;									// length of this segment
		float distanceFromStart;						// distance of this node from the start of the path
		float curvature;								// how much the path 'curves' at this point in the XY plane (0 = none, 1 = 180 degree doubleback)

		Vector m_portalCenter;							// position of center of 'portal' between previous area and this area
		float m_portalHalfWidth;						// half width of 'portal'
	};

	virtual float GetLength( void ) const;						// return length of path from start to finish
	virtual const Vector &GetPosition( float distanceFromStart, const Segment *start = NULL ) const;	// return a position on the path at the given distance from the path start
	virtual const Vector &GetClosestPosition( const Vector &pos, const Segment *start = NULL, float alongLimit = 0.0f ) const;		// return the closest point on the path to the given position

	virtual const Vector &GetStartPosition( void ) const;	// return the position where this path starts
	virtual const Vector &GetEndPosition( void ) const;		// return the position where this path ends
	virtual CBaseCombatCharacter *GetSubject( void ) const;	// return the actor this path leads to, or NULL if there is no subject

	virtual const Path::Segment *GetCurrentGoal( void ) const;	// return current goal along the path we are trying to reach

	virtual float GetAge( void ) const;					// return "age" of this path (time since it was built)

	enum SeekType
	{
		SEEK_ENTIRE_PATH,			// search the entire path length
		SEEK_AHEAD,					// search from current cursor position forward toward end of path
		SEEK_BEHIND					// search from current cursor position backward toward path start
	};
	virtual void MoveCursorToClosestPosition( const Vector &pos, SeekType type = SEEK_ENTIRE_PATH, float alongLimit = 0.0f ) const;		// Set cursor position to closest point on path to given position
	
	enum MoveCursorType
	{
		PATH_ABSOLUTE_DISTANCE,
		PATH_RELATIVE_DISTANCE
	};
	virtual void MoveCursorToStart( void );				// set seek cursor to start of path
	virtual void MoveCursorToEnd( void );				// set seek cursor to end of path
	virtual void MoveCursor( float value, MoveCursorType type = PATH_ABSOLUTE_DISTANCE );	// change seek cursor position
	virtual float GetCursorPosition( void ) const;		// return position of seek cursor (distance along path)

	struct Data
	{
		Vector pos;										// the position along the path
		Vector forward;									// unit vector along path direction
		float curvature;								// how much the path 'curves' at this point in the XY plane (0 = none, 1 = 180 degree doubleback)
		const Segment *segmentPrior;					// the segment just before this position
	};
	virtual const Data &GetCursorData( void ) const;	// return path state at the current cursor position

	virtual bool IsValid( void ) const;
	virtual void Invalidate( void );					// make path invalid (clear it)

	virtual void Draw( const Path::Segment *start = NULL ) const;	// draw the path for debugging
	virtual void DrawInterpolated( float from, float to );	// draw the path for debugging - MODIFIES cursor position

	virtual const Segment *FirstSegment( void ) const;	// return first segment of path
	virtual const Segment *NextSegment( const Segment *currentSegment ) const;	// return next segment of path, given current one
	virtual const Segment *PriorSegment( const Segment *currentSegment ) const;	// return previous segment of path, given current one
	virtual const Segment *LastSegment( void ) const;	// return last segment of path

	enum ResultType
	{
		COMPLETE_PATH,
		PARTIAL_PATH,
		NO_PATH
	};
	virtual void OnPathChanged( INextBot *bot, ResultType result ) { }		// invoked when the path is (re)computed (path is valid at the time of this call)

	virtual void Copy( INextBot *bot, const Path &path );	// Replace this path with the given path's data


	//-----------------------------------------------------------------------------------------------------------------
	/**
	 * Compute shortest path from bot to given actor via A* algorithm.
	 * If returns true, path was found to the subject.
	 * If returns false, path may either be invalid (use IsValid() to check), or valid but 
	 * doesn't reach all the way to the subject.
	 */
	template< typename CostFunctor >
	bool Compute( INextBot *bot, CBaseCombatCharacter *subject, CostFunctor &costFunc, float maxPathLength = 0.0f, bool includeGoalIfPathFails = true )
	{
		VPROF_BUDGET( "Path::Compute(subject)", "NextBot" );

		Invalidate();

		m_subject = subject;
		
		const Vector &start = bot->GetPosition();
		
		CNavArea *startArea = bot->GetEntity()->GetLastKnownArea();
		if ( !startArea )
		{
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		CNavArea *subjectArea = subject->GetLastKnownArea();
		if ( !subjectArea )
		{
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		Vector subjectPos = subject->GetAbsOrigin();
		
		// if we are already in the subject area, build trivial path
		if ( startArea == subjectArea )
		{
			BuildTrivialPath( bot, subjectPos );
			return true;
		}

		//
		// Compute shortest path to subject
		//
		CNavArea *closestArea = NULL;
		bool pathResult = NavAreaBuildPath( startArea, subjectArea, &subjectPos, costFunc, &closestArea, maxPathLength, bot->GetEntity()->GetTeamNumber() );

		// Failed?
		if ( closestArea == NULL )
			return false;

		//
		// Build actual path by following parent links back from goal area
		//

		// get count
		int count = 0;
		CNavArea *area;
		for( area = closestArea; area; area = area->GetParent() )
		{
			++count;

			if ( area == startArea )
			{
				// startArea can be re-evaluated during the pathfind and given a parent...
				break;
			}
			if ( count >= MAX_PATH_SEGMENTS-1 ) // save room for endpoint
				break;
		}
		
		if ( count == 1 )
		{
			BuildTrivialPath( bot, subjectPos );
			return pathResult;
		}

		// assemble path
		m_segmentCount = count;
		for( area = closestArea; count && area; area = area->GetParent() )
		{
			--count;
			m_path[ count ].area = area;
			m_path[ count ].how = area->GetParentHow();
			m_path[ count ].type = ON_GROUND;
		}

		if ( pathResult || includeGoalIfPathFails )
		{
			// append actual subject position
			m_path[ m_segmentCount ].area = closestArea;
			m_path[ m_segmentCount ].pos = subjectPos;
			m_path[ m_segmentCount ].ladder = NULL;
			m_path[ m_segmentCount ].how = NUM_TRAVERSE_TYPES;
			m_path[ m_segmentCount ].type = ON_GROUND;
			++m_segmentCount;
		}
				
		// compute path positions
		if ( ComputePathDetails( bot, start ) == false )
		{
			Invalidate();
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		// remove redundant nodes and clean up path
		Optimize( bot );
		
		PostProcess();

		OnPathChanged( bot, pathResult ? COMPLETE_PATH : PARTIAL_PATH );

		return pathResult;
	}


	//-----------------------------------------------------------------------------------------------------------------
	/**
	 * Compute shortest path from bot to 'goal' via A* algorithm.
	 * If returns true, path was found to the goal position.
	 * If returns false, path may either be invalid (use IsValid() to check), or valid but 
	 * doesn't reach all the way to the goal.
	 */
	template< typename CostFunctor >
	bool Compute( INextBot *bot, const Vector &goal, CostFunctor &costFunc, float maxPathLength = 0.0f, bool includeGoalIfPathFails = true, bool requireGoalArea = false )
	{
		VPROF_BUDGET( "Path::Compute(goal)", "NextBotSpiky" );

		Invalidate();
		
		const Vector &start = bot->GetPosition();
		
		CNavArea *startArea = bot->GetEntity()->GetLastKnownArea();
		if ( !startArea )
		{
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		// check line-of-sight to the goal position when finding it's nav area
		const float maxDistanceToArea = 200.0f;
		CNavArea *goalArea = TheNavMesh->GetNearestNavArea( goal, true, maxDistanceToArea, true );

		if ( requireGoalArea && !goalArea )
		{
			Invalidate();
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		// if we are already in the goal area, build trivial path
		if ( startArea == goalArea )
		{
			BuildTrivialPath( bot, goal );
			return true;
		}

		// make sure path end position is on the ground
		Vector pathEndPosition = goal;
		if ( goalArea )
		{
			pathEndPosition.z = goalArea->GetZ( pathEndPosition );
		}
		else
		{
			TheNavMesh->GetGroundHeight( pathEndPosition, &pathEndPosition.z );
		}

		//
		// Compute shortest path to goal
		//
		CNavArea *closestArea = NULL;
		bool pathResult = NavAreaBuildPath( startArea, goalArea, &goal, costFunc, &closestArea, maxPathLength, bot->GetEntity()->GetTeamNumber() );

		// Failed?
		if ( closestArea == NULL )
			return false;

		//
		// Build actual path by following parent links back from goal area
		//

		// get count
		int count = 0;
		CNavArea *area;
		for( area = closestArea; area; area = area->GetParent() )
		{
			++count;

			if ( area == startArea )
			{
				// startArea can be re-evaluated during the pathfind and given a parent...
				break;
			}
			if ( count >= MAX_PATH_SEGMENTS-1 ) // save room for endpoint
				break;
		}
		
		if ( count == 1 )
		{
			BuildTrivialPath( bot, goal );
			return pathResult;
		}

		// assemble path
		m_segmentCount = count;
		for( area = closestArea; count && area; area = area->GetParent() )
		{
			--count;
			m_path[ count ].area = area;
			m_path[ count ].how = area->GetParentHow();
			m_path[ count ].type = ON_GROUND;
		}

		if ( pathResult || includeGoalIfPathFails )
		{
			// append actual goal position
			m_path[ m_segmentCount ].area = closestArea;
			m_path[ m_segmentCount ].pos = pathEndPosition;
			m_path[ m_segmentCount ].ladder = NULL;
			m_path[ m_segmentCount ].how = NUM_TRAVERSE_TYPES;
			m_path[ m_segmentCount ].type = ON_GROUND;
			++m_segmentCount;
		}
				
		// compute path positions
		if ( ComputePathDetails( bot, start ) == false )
		{
			Invalidate();
			OnPathChanged( bot, NO_PATH );
			return false;
		}

		// remove redundant nodes and clean up path
		Optimize( bot );
		
		PostProcess();

		OnPathChanged( bot, pathResult ? COMPLETE_PATH : PARTIAL_PATH );

		return pathResult;
	}


	//-----------------------------------------------------------------------------------------------------------------
	/**
	 * Build a path from bot's current location to an undetermined goal area
	 * that minimizes the given cost along the final path and meets the
	 * goal criteria.
	 */
	virtual bool ComputeWithOpenGoal( INextBot *bot, const IPathCost &costFunc, const IPathOpenGoalSelector &goalSelector, float maxSearchRadius = 0.0f )
	{
		VPROF_BUDGET( "ComputeWithOpenGoal", "NextBot" );

		int teamID = bot->GetEntity()->GetTeamNumber();

		CNavArea *startArea = bot->GetEntity()->GetLastKnownArea();

		if ( startArea == NULL )
			return NULL;

		startArea->SetParent( NULL );

		// start search
		CNavArea::ClearSearchLists();

		float initCost = costFunc( startArea, NULL, NULL, NULL, -1.0f );
		if ( initCost < 0.0f )
			return NULL;

		startArea->SetTotalCost( initCost );
		startArea->AddToOpenList();

		// find our goal as we search
		CNavArea *goalArea = NULL;

		//
		// Dijkstra's algorithm (since we don't know our goal).
		//
		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			area->AddToClosedList();

			// don't consider blocked areas
			if ( area->IsBlocked( teamID ) )
				continue;

			// build adjacent area array
			CollectAdjacentAreas( area );

			// search adjacent areas
			for( int i=0; i<m_adjAreaIndex; ++i )
			{
				CNavArea *newArea = m_adjAreaVector[ i ].area;

				// only visit each area once
				if ( newArea->IsClosed() )
					continue;

				// don't consider blocked areas
				if ( newArea->IsBlocked( teamID ) )
					continue;

				// don't use this area if it is out of range
				if ( maxSearchRadius > 0.0f && ( newArea->GetCenter() - bot->GetEntity()->GetAbsOrigin() ).IsLengthGreaterThan( maxSearchRadius ) )
					continue;

				// determine cost of traversing this area
				float newCost = costFunc( newArea, area, m_adjAreaVector[ i ].ladder, NULL, -1.0f );

				// don't use adjacent area if cost functor says it is a dead-end
				if ( newCost < 0.0f )
					continue;

				if ( newArea->IsOpen() && newArea->GetTotalCost() <= newCost )
				{
					// we have already visited this area, and it has a better path
					continue;
				}
				else
				{
					// whether this area has been visited or not, we now have a better path to it
					newArea->SetParent( area, m_adjAreaVector[ i ].how );
					newArea->SetTotalCost( newCost );

					// use 'cost so far' to hold cumulative cost
					newArea->SetCostSoFar( newCost );

					// tricky bit here - relying on OpenList being sorted by cost
					if ( newArea->IsOpen() )
					{
						// area already on open list, update the list order to keep costs sorted
						newArea->UpdateOnOpenList();
					}
					else
					{
						newArea->AddToOpenList();
					}

					// keep track of best goal so far
					goalArea = goalSelector( goalArea, newArea );
				}
			}
		}

		if ( goalArea )
		{
			// compile the path details into a usable path
			AssemblePrecomputedPath( bot, goalArea->GetCenter(), goalArea );
			return true;
		}	

		// all adjacent areas are likely too far away
		return false;
	}


	//-----------------------------------------------------------------------------------------------------------------
	/**
	 * Given the last area in a path with valid parent pointers, 
	 * construct the actual path.
	 */
	void AssemblePrecomputedPath( INextBot *bot, const Vector &goal, CNavArea *endArea )
	{
		VPROF_BUDGET( "AssemblePrecomputedPath", "NextBot" );

		const Vector &start = bot->GetPosition();

		// get count
		int count = 0;
		CNavArea *area;
		for( area = endArea; area; area = area->GetParent() )
		{
			++count;
		}

		// save room for endpoint
		if ( count > MAX_PATH_SEGMENTS-1 )
		{
			count = MAX_PATH_SEGMENTS-1;
		}
		else if ( count == 0 )
		{
			return;
		}

		if ( count == 1 )
		{
			BuildTrivialPath( bot, goal );
			return;
		}

		// assemble path
		m_segmentCount = count;
		for( area = endArea; count && area; area = area->GetParent() )
		{
			--count;
			m_path[ count ].area = area;
			m_path[ count ].how = area->GetParentHow();
			m_path[ count ].type = ON_GROUND;
		}

		// append actual goal position
		m_path[ m_segmentCount ].area = endArea;
		m_path[ m_segmentCount ].pos = goal;
		m_path[ m_segmentCount ].ladder = NULL;
		m_path[ m_segmentCount ].how = NUM_TRAVERSE_TYPES;
		m_path[ m_segmentCount ].type = ON_GROUND;
		++m_segmentCount;

		// compute path positions
		if ( ComputePathDetails( bot, start ) == false )
		{
			Invalidate();
			OnPathChanged( bot, NO_PATH );
			return;
		}

		// remove redundant nodes and clean up path
		Optimize( bot );

		PostProcess();

		OnPathChanged( bot, COMPLETE_PATH );
	}

	/**
	 * Utility function for when start and goal are in the same area
	 */
	bool BuildTrivialPath( INextBot *bot, const Vector &goal );	

	/**
	 * Determine exactly where the path goes between the given two areas
	 * on the path. Return this point in 'crossPos'.
	 */
	virtual void ComputeAreaCrossing( INextBot *bot, const CNavArea *from, const Vector &fromPos, const CNavArea *to, NavDirType dir, Vector *crossPos ) const;


private:
	enum { MAX_PATH_SEGMENTS = 256 };
	Segment m_path[ MAX_PATH_SEGMENTS ];
	int m_segmentCount;

	bool ComputePathDetails( INextBot *bot, const Vector &start );		// determine actual path positions 

	void Optimize( INextBot *bot );
	void PostProcess( void );
	int FindNextOccludedNode( INextBot *bot, int anchor );	// used by Optimize()

	void InsertSegment( Segment newSegment, int i );		// insert new segment at index i
	
	mutable Vector m_pathPos;								// used by GetPosition()
	mutable Vector m_closePos;								// used by GetClosestPosition()

	mutable float m_cursorPos;					// current cursor position (distance along path)
	mutable Data m_cursorData;					// used by GetCursorData()
	mutable bool m_isCursorDataDirty;

	IntervalTimer m_ageTimer;					// how old is this path?
	CHandle< CBaseCombatCharacter > m_subject;	// the subject this path leads to

	/**
	 * Build a vector of adjacent areas reachable from the given area
	 */
	void CollectAdjacentAreas( CNavArea *area )
	{
		m_adjAreaIndex = 0;			

		const NavConnectVector &adjNorth = *area->GetAdjacentAreas( NORTH );		
		FOR_EACH_VEC( adjNorth, it )
		{
			if ( m_adjAreaIndex >= MAX_ADJ_AREAS )
				break;

			m_adjAreaVector[ m_adjAreaIndex ].area = adjNorth[ it ].area;
			m_adjAreaVector[ m_adjAreaIndex ].how = GO_NORTH;
			m_adjAreaVector[ m_adjAreaIndex ].ladder = NULL;
			++m_adjAreaIndex;
		}

		const NavConnectVector &adjSouth = *area->GetAdjacentAreas( SOUTH );		
		FOR_EACH_VEC( adjSouth, it )
		{
			if ( m_adjAreaIndex >= MAX_ADJ_AREAS )
				break;

			m_adjAreaVector[ m_adjAreaIndex ].area = adjSouth[ it ].area;
			m_adjAreaVector[ m_adjAreaIndex ].how = GO_SOUTH;
			m_adjAreaVector[ m_adjAreaIndex ].ladder = NULL;
			++m_adjAreaIndex;
		}

		const NavConnectVector &adjWest = *area->GetAdjacentAreas( WEST );		
		FOR_EACH_VEC( adjWest, it )
		{
			if ( m_adjAreaIndex >= MAX_ADJ_AREAS )
				break;

			m_adjAreaVector[ m_adjAreaIndex ].area = adjWest[ it ].area;
			m_adjAreaVector[ m_adjAreaIndex ].how = GO_WEST;
			m_adjAreaVector[ m_adjAreaIndex ].ladder = NULL;
			++m_adjAreaIndex;
		}

		const NavConnectVector &adjEast = *area->GetAdjacentAreas( EAST );	
		FOR_EACH_VEC( adjEast, it )
		{
			if ( m_adjAreaIndex >= MAX_ADJ_AREAS )
				break;

			m_adjAreaVector[ m_adjAreaIndex ].area = adjEast[ it ].area;
			m_adjAreaVector[ m_adjAreaIndex ].how = GO_EAST;
			m_adjAreaVector[ m_adjAreaIndex ].ladder = NULL;
			++m_adjAreaIndex;
		}

		const NavLadderConnectVector &adjUpLadder = *area->GetLadders( CNavLadder::LADDER_UP );
		FOR_EACH_VEC( adjUpLadder, it )
		{
			CNavLadder *ladder = adjUpLadder[ it ].ladder;

			if ( ladder->m_topForwardArea && m_adjAreaIndex < MAX_ADJ_AREAS )
			{
				m_adjAreaVector[ m_adjAreaIndex ].area = ladder->m_topForwardArea;
				m_adjAreaVector[ m_adjAreaIndex ].how = GO_LADDER_UP;
				m_adjAreaVector[ m_adjAreaIndex ].ladder = ladder;
				++m_adjAreaIndex;
			}

			if ( ladder->m_topLeftArea && m_adjAreaIndex < MAX_ADJ_AREAS )
			{
				m_adjAreaVector[ m_adjAreaIndex ].area = ladder->m_topLeftArea;
				m_adjAreaVector[ m_adjAreaIndex ].how = GO_LADDER_UP;
				m_adjAreaVector[ m_adjAreaIndex ].ladder = ladder;
				++m_adjAreaIndex;
			}

			if ( ladder->m_topRightArea && m_adjAreaIndex < MAX_ADJ_AREAS )
			{
				m_adjAreaVector[ m_adjAreaIndex ].area = ladder->m_topRightArea;
				m_adjAreaVector[ m_adjAreaIndex ].how = GO_LADDER_UP;
				m_adjAreaVector[ m_adjAreaIndex ].ladder = ladder;
				++m_adjAreaIndex;
			}
		}

		const NavLadderConnectVector &adjDownLadder = *area->GetLadders( CNavLadder::LADDER_DOWN );
		FOR_EACH_VEC( adjDownLadder, it )
		{
			CNavLadder *ladder = adjDownLadder[ it ].ladder;

			if ( m_adjAreaIndex >= MAX_ADJ_AREAS )
				break;

			if ( ladder->m_bottomArea )
			{
				m_adjAreaVector[ m_adjAreaIndex ].area = ladder->m_bottomArea;
				m_adjAreaVector[ m_adjAreaIndex ].how = GO_LADDER_DOWN;
				m_adjAreaVector[ m_adjAreaIndex ].ladder = ladder;
				++m_adjAreaIndex;
			}
		}
	}

	enum { MAX_ADJ_AREAS = 64 };

	struct AdjInfo
	{
		CNavArea *area;
		CNavLadder *ladder;
		NavTraverseType how;		
	};

	AdjInfo m_adjAreaVector[ MAX_ADJ_AREAS ];
	int m_adjAreaIndex;

};


inline float Path::GetLength( void ) const
{
	if (m_segmentCount <= 0)
	{
		return 0.0f;
	}
	
	return m_path[ m_segmentCount-1 ].distanceFromStart;
}

inline bool Path::IsValid( void ) const
{
	return (m_segmentCount > 0);
}

inline void Path::Invalidate( void )
{
	m_segmentCount = 0;

	m_cursorPos = 0.0f;
	
	m_cursorData.pos = vec3_origin;
	m_cursorData.forward = Vector( 1.0f, 0, 0 );
	m_cursorData.curvature = 0.0f;
	m_cursorData.segmentPrior = NULL;
	
	m_isCursorDataDirty = true;

	m_subject = NULL;
}

inline const Path::Segment *Path::FirstSegment( void ) const
{
	return (IsValid()) ? &m_path[0] : NULL;
}

inline const Path::Segment *Path::NextSegment( const Segment *currentSegment ) const
{
	if (currentSegment == NULL || !IsValid())
		return NULL;
		
	int i = currentSegment - m_path;

	if (i < 0 || i >= m_segmentCount-1)
	{
		return NULL;
	}
	
	return &m_path[ i+1 ];
}

inline const Path::Segment *Path::PriorSegment( const Segment *currentSegment ) const
{
	if (currentSegment == NULL || !IsValid())
		return NULL;
		
	int i = currentSegment - m_path;

	if (i < 1 || i >= m_segmentCount)
	{
		return NULL;
	}
	
	return &m_path[ i-1 ];
}

inline const Path::Segment *Path::LastSegment( void ) const
{
	return ( IsValid() ) ? &m_path[ m_segmentCount-1 ] : NULL;
}

inline const Vector &Path::GetStartPosition( void ) const
{
	return ( IsValid() ) ? m_path[ 0 ].pos : vec3_origin;
}

inline const Vector &Path::GetEndPosition( void ) const
{
	return ( IsValid() ) ? m_path[ m_segmentCount-1 ].pos : vec3_origin;
}

inline CBaseCombatCharacter *Path::GetSubject( void ) const
{
	return m_subject;
}

inline void Path::MoveCursorToStart( void )
{
	m_cursorPos = 0.0f;
	m_isCursorDataDirty = true;
}

inline void Path::MoveCursorToEnd( void )
{
	m_cursorPos = GetLength();
	m_isCursorDataDirty = true;
}

inline void Path::MoveCursor( float value, MoveCursorType type )
{
	if ( type == PATH_ABSOLUTE_DISTANCE )
	{
		m_cursorPos = value;
	}
	else	// relative distance
	{
		m_cursorPos += value;
	}
	
	if ( m_cursorPos < 0.0f )
	{
		m_cursorPos = 0.0f;
	}
	else if ( m_cursorPos > GetLength() )
	{
		m_cursorPos = GetLength();
	}
	
	m_isCursorDataDirty = true;
}

inline float Path::GetCursorPosition( void ) const
{
	return m_cursorPos;
}

inline const Path::Segment *Path::GetCurrentGoal( void ) const
{
	return NULL;
}

inline float Path::GetAge( void ) const
{
	return m_ageTimer.GetElapsedTime();
}


#endif	// _NEXT_BOT_PATH_H_

