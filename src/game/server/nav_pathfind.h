//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_pathfind.h
// Path-finding mechanisms using the Navigation Mesh
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_PATHFIND_H_
#define _NAV_PATHFIND_H_

#include "tier0/vprof.h"
#include "mathlib/ssemath.h"
#include "nav_area.h"



//-------------------------------------------------------------------------------------------------------------------
/**
 * Used when building a path to determine the kind of path to build
 */
enum RouteType
{
	DEFAULT_ROUTE,
	FASTEST_ROUTE,
	SAFEST_ROUTE,
	RETREAT_ROUTE,
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class ShortestPathCost
{
public:
	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length )
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			// if this is a "crouch" area, add penalty
			if ( area->GetAttributes() & NAV_MESH_CROUCH )
			{
				const float crouchPenalty = 20.0f;		// 10
				cost += crouchPenalty * dist;
			}

			// if this is a "jump" area, add penalty
			if ( area->GetAttributes() & NAV_MESH_JUMP )
			{
				const float jumpPenalty = 5.0f;
				cost += jumpPenalty * dist;
			}

			return cost;
		}
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * If 'maxPathLength' is nonzero, path building will stop when this length is reached.
 * Returns true if a path exists.
 */
#define IGNORE_NAV_BLOCKERS true
template< typename CostFunctor >
bool NavAreaBuildPath( CNavArea *startArea, CNavArea *goalArea, const Vector *goalPos, CostFunctor &costFunc, CNavArea **closestArea = NULL, float maxPathLength = 0.0f, int teamID = TEAM_ANY, bool ignoreNavBlockers = false )
{
	VPROF_BUDGET( "NavAreaBuildPath", "NextBotSpiky" );

	if ( closestArea )
	{
		*closestArea = startArea;
	}


	if (startArea == NULL)
		return false;

	startArea->SetParent( NULL );

	if (goalArea != NULL && goalArea->IsBlocked( teamID, ignoreNavBlockers ))
		goalArea = NULL;

	if (goalArea == NULL && goalPos == NULL)
		return false;

	// if we are already in the goal area, build trivial path
	if (startArea == goalArea)
	{
		return true;
	}

	// determine actual goal position
	Vector actualGoalPos = (goalPos) ? *goalPos : goalArea->GetCenter();

	// start search
	CNavArea::ClearSearchLists();

	// compute estimate of path length
	/// @todo Cost might work as "manhattan distance"
	startArea->SetTotalCost( (startArea->GetCenter() - actualGoalPos).Length() );

	float initCost = costFunc( startArea, NULL, NULL, NULL, -1.0f );	
	if (initCost < 0.0f)
		return false;
	startArea->SetCostSoFar( initCost );
	startArea->SetPathLengthSoFar( 0.0 );

	startArea->AddToOpenList();

	// keep track of the area we visit that is closest to the goal
	float closestAreaDist = startArea->GetTotalCost();

	// do A* search
	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();


		// don't consider blocked areas
		if ( area->IsBlocked( teamID, ignoreNavBlockers ) )
			continue;

		// check if we have found the goal area or position
		if (area == goalArea || (goalArea == NULL && goalPos && area->Contains( *goalPos )))
		{
			if (closestArea)
			{
				*closestArea = area;
			}

			return true;
		}

		// search adjacent areas
		enum SearchType
		{
			SEARCH_FLOOR, SEARCH_LADDERS, SEARCH_ELEVATORS
		};
		SearchType searchWhere = SEARCH_FLOOR;
		int searchIndex = 0;

		int dir = NORTH;
		const NavConnectVector *floorList = area->GetAdjacentAreas( NORTH );

		bool ladderUp = true;
		const NavLadderConnectVector *ladderList = NULL;
		enum { AHEAD = 0, LEFT, RIGHT, BEHIND, NUM_TOP_DIRECTIONS };
		int ladderTopDir = AHEAD;
		bool bHaveMaxPathLength = ( maxPathLength > 0.0f );
		float length = -1;
		
		while( true )
		{
			CNavArea *newArea = NULL;
			NavTraverseType how;
			const CNavLadder *ladder = NULL;
			const CFuncElevator *elevator = NULL;

			//
			// Get next adjacent area - either on floor or via ladder
			//
			if ( searchWhere == SEARCH_FLOOR )
			{
				// if exhausted adjacent connections in current direction, begin checking next direction
				if ( searchIndex >= floorList->Count() )
				{
					++dir;

					if ( dir == NUM_DIRECTIONS )
					{
						// checked all directions on floor - check ladders next
						searchWhere = SEARCH_LADDERS;

						ladderList = area->GetLadders( CNavLadder::LADDER_UP );
						searchIndex = 0;
						ladderTopDir = AHEAD;
					}
					else
					{
						// start next direction
						floorList = area->GetAdjacentAreas( (NavDirType)dir );
						searchIndex = 0;
					}

					continue;
				}

				const NavConnect &floorConnect = floorList->Element( searchIndex );
				newArea = floorConnect.area;
				length = floorConnect.length;
				how = (NavTraverseType)dir;
				++searchIndex;

				if ( IsX360() && searchIndex < floorList->Count() )
				{
					PREFETCH360( floorList->Element( searchIndex ).area, 0  );
				}
			}
			else if ( searchWhere == SEARCH_LADDERS )
			{
				if ( searchIndex >= ladderList->Count() )
				{
					if ( !ladderUp )
					{
						// checked both ladder directions - check elevators next
						searchWhere = SEARCH_ELEVATORS;
						searchIndex = 0;
						ladder = NULL;
					}
					else
					{
						// check down ladders
						ladderUp = false;
						ladderList = area->GetLadders( CNavLadder::LADDER_DOWN );
						searchIndex = 0;
					}
					continue;
				}

				if ( ladderUp )
				{
					ladder = ladderList->Element( searchIndex ).ladder;

					// do not use BEHIND connection, as its very hard to get to when going up a ladder
					if ( ladderTopDir == AHEAD )
					{
						newArea = ladder->m_topForwardArea;
					}
					else if ( ladderTopDir == LEFT )
					{
						newArea = ladder->m_topLeftArea;
					}
					else if ( ladderTopDir == RIGHT )
					{
						newArea = ladder->m_topRightArea;
					}
					else
					{
						++searchIndex;
						ladderTopDir = AHEAD;
						continue;
					}

					how = GO_LADDER_UP;
					++ladderTopDir;
				}
				else
				{
					newArea = ladderList->Element( searchIndex ).ladder->m_bottomArea;
					how = GO_LADDER_DOWN;
					ladder = ladderList->Element(searchIndex).ladder;
					++searchIndex;
				}

				if ( newArea == NULL )
					continue;

				length = -1.0f;
			}
			else // if ( searchWhere == SEARCH_ELEVATORS )
			{
				const NavConnectVector &elevatorAreas = area->GetElevatorAreas();

				elevator = area->GetElevator();

				if ( elevator == NULL || searchIndex >= elevatorAreas.Count() )
				{
					// done searching connected areas
					elevator = NULL;
					break;
				}

				newArea = elevatorAreas[ searchIndex++ ].area;
				if ( newArea->GetCenter().z > area->GetCenter().z )
				{
					how = GO_ELEVATOR_UP;
				}
				else
				{
					how = GO_ELEVATOR_DOWN;
				}

				length = -1.0f;
			}


			// don't backtrack
			Assert( newArea );
			if ( newArea == area->GetParent() )
				continue;
			if ( newArea == area ) // self neighbor?
				continue;

			// don't consider blocked areas
			if ( newArea->IsBlocked( teamID, ignoreNavBlockers ) )
				continue;

			float newCostSoFar = costFunc( newArea, area, ladder, elevator, length );

			// NaNs really mess this function up causing tough to track down hangs. If
			//  we get inf back, clamp it down to a really high number.
			DebuggerBreakOnNaN_StagingOnly( newCostSoFar );
			if ( IS_NAN( newCostSoFar ) )
				newCostSoFar = 1e30f;

			// check if cost functor says this area is a dead-end
			if ( newCostSoFar < 0.0f )
				continue;

			// Safety check against a bogus functor.  The cost of the path
			// A...B, C should always be at least as big as the path A...B.
			Assert( newCostSoFar >= area->GetCostSoFar() );

			// And now that we've asserted, let's be a bit more defensive.
			// Make sure that any jump to a new area incurs some pathfinsing
			// cost, to avoid us spinning our wheels over insignificant cost
			// benefit, floating point precision bug, or busted cost functor.
			float minNewCostSoFar = area->GetCostSoFar() * 1.00001f + 0.00001f;
			newCostSoFar = Max( newCostSoFar, minNewCostSoFar );
				
			// stop if path length limit reached
			if ( bHaveMaxPathLength )
			{
				// keep track of path length so far
				float deltaLength = ( newArea->GetCenter() - area->GetCenter() ).Length();
				float newLengthSoFar = area->GetPathLengthSoFar() + deltaLength;
				if ( newLengthSoFar > maxPathLength )
					continue;
				
				newArea->SetPathLengthSoFar( newLengthSoFar );
			}

			if ( ( newArea->IsOpen() || newArea->IsClosed() ) && newArea->GetCostSoFar() <= newCostSoFar )
			{
				// this is a worse path - skip it
				continue;
			}
			else
			{
				// compute estimate of distance left to go
				float distSq = ( newArea->GetCenter() - actualGoalPos ).LengthSqr();
				float newCostRemaining = ( distSq > 0.0 ) ? FastSqrt( distSq ) : 0.0 ;

				// track closest area to goal in case path fails
				if ( closestArea && newCostRemaining < closestAreaDist )
				{
					*closestArea = newArea;
					closestAreaDist = newCostRemaining;
				}
				
				newArea->SetCostSoFar( newCostSoFar );
				newArea->SetTotalCost( newCostSoFar + newCostRemaining );

				if ( newArea->IsClosed() )
				{
					newArea->RemoveFromClosedList();
				}

				if ( newArea->IsOpen() )
				{
					// area already on open list, update the list order to keep costs sorted
					newArea->UpdateOnOpenList();
				}
				else
				{
					newArea->AddToOpenList();
				}

				newArea->SetParent( area, how );
			}
		}

		// we have searched this area
		area->AddToClosedList();
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute distance between two areas. Return -1 if can't reach 'endArea' from 'startArea'.
 */
template< typename CostFunctor >
float NavAreaTravelDistance( CNavArea *startArea, CNavArea *endArea, CostFunctor &costFunc, float maxPathLength = 0.0f )
{
	if (startArea == NULL)
		return -1.0f;

	if (endArea == NULL)
		return -1.0f;

	if (startArea == endArea)
		return 0.0f;

	// compute path between areas using given cost heuristic
	if (NavAreaBuildPath( startArea, endArea, NULL, costFunc, NULL, maxPathLength ) == false)
		return -1.0f;

	// compute distance along path
	float distance = 0.0f;
	for( CNavArea *area = endArea; area->GetParent(); area = area->GetParent() )
	{
		distance += (area->GetCenter() - area->GetParent()->GetCenter()).Length();
	}

	return distance;
}



//--------------------------------------------------------------------------------------------------------------
/**
 * Do a breadth-first search, invoking functor on each area.
 * If functor returns 'true', continue searching from this area. 
 * If functor returns 'false', the area's adjacent areas are not explored (dead end).
 * If 'maxRange' is 0 or less, no range check is done (all areas will be examined).
 *
 * NOTE: Returns all areas that overlap range, even partially
 *
 * @todo Use ladder connections
 */

// helper function
inline void AddAreaToOpenList( CNavArea *area, CNavArea *parent, const Vector &startPos, float maxRange )
{
	if (area == NULL)
		return;

	if (!area->IsMarked())
	{
		area->Mark();
		area->SetTotalCost( 0.0f );
		area->SetParent( parent );

		if (maxRange > 0.0f)
		{
			// make sure this area overlaps range
			Vector closePos;
			area->GetClosestPointOnArea( startPos, &closePos );
			if ((closePos - startPos).AsVector2D().IsLengthLessThan( maxRange ))
			{
				// compute approximate distance along path to limit travel range, too
				float distAlong = parent->GetCostSoFar();
				distAlong += (area->GetCenter() - parent->GetCenter()).Length();
				area->SetCostSoFar( distAlong );

				// allow for some fudge due to large size areas
				if (distAlong <= 1.5f * maxRange)
					area->AddToOpenList();
			}
		}
		else
		{
			// infinite range
			area->AddToOpenList();
		}
	}
}


/****************************************************************
 * DEPRECATED: Use filter-based SearchSurroundingAreas below
 ****************************************************************/
#define INCLUDE_INCOMING_CONNECTIONS	0x1
#define INCLUDE_BLOCKED_AREAS			0x2
#define EXCLUDE_OUTGOING_CONNECTIONS	0x4
#define EXCLUDE_ELEVATORS				0x8
template < typename Functor >
void SearchSurroundingAreas( CNavArea *startArea, const Vector &startPos, Functor &func, float maxRange = -1.0f, unsigned int options = 0, int teamID = TEAM_ANY )
{
	if (startArea == NULL)
		return;

	CNavArea::MakeNewMarker();
	CNavArea::ClearSearchLists();

	startArea->AddToOpenList();
	startArea->SetTotalCost( 0.0f );
	startArea->SetCostSoFar( 0.0f );
	startArea->SetParent( NULL );
	startArea->Mark();

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();

		// don't use blocked areas
		if ( area->IsBlocked( teamID ) && !(options & INCLUDE_BLOCKED_AREAS) )
			continue;

		// invoke functor on area
		if (func( area ))
		{
			// explore adjacent floor areas
			for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
			{
				int count = area->GetAdjacentCount( (NavDirType)dir );
				for( int i=0; i<count; ++i )
				{
					CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );
					if ( options & EXCLUDE_OUTGOING_CONNECTIONS )
					{
						if ( !adjArea->IsConnected( area, NUM_DIRECTIONS ) )
						{
							continue;	// skip this outgoing connection
						}
					}
					
					AddAreaToOpenList( adjArea, area, startPos, maxRange );
				}
			}
			
			// potentially include areas that connect TO this area via a one-way link
			if (options & INCLUDE_INCOMING_CONNECTIONS)
			{
				for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
				{
					const NavConnectVector *list = area->GetIncomingConnections( (NavDirType)dir );

					FOR_EACH_VEC( (*list), it )
					{
						NavConnect connect = (*list)[ it ];				
						
						AddAreaToOpenList( connect.area, area, startPos, maxRange );
					}
				}
			}


			// explore adjacent areas connected by ladders

			// check up ladders
			const NavLadderConnectVector *ladderList = area->GetLadders( CNavLadder::LADDER_UP );
			if (ladderList)
			{
				FOR_EACH_VEC( (*ladderList), it )
				{
					const CNavLadder *ladder = (*ladderList)[ it ].ladder;

					// do not use BEHIND connection, as its very hard to get to when going up a ladder
					AddAreaToOpenList( ladder->m_topForwardArea, area, startPos, maxRange );
					AddAreaToOpenList( ladder->m_topLeftArea, area, startPos, maxRange );
					AddAreaToOpenList( ladder->m_topRightArea, area, startPos, maxRange );
				}
			}

			// check down ladders
			ladderList = area->GetLadders( CNavLadder::LADDER_DOWN );
			if (ladderList)
			{
				FOR_EACH_VEC( (*ladderList), it )
				{
					const CNavLadder *ladder = (*ladderList)[ it ].ladder;

					AddAreaToOpenList( ladder->m_bottomArea, area, startPos, maxRange );
				}
			}

			if ( (options & EXCLUDE_ELEVATORS) == 0 )
			{
				const NavConnectVector &elevatorList = area->GetElevatorAreas();
				FOR_EACH_VEC( elevatorList, it )
				{
					CNavArea *elevatorArea = elevatorList[ it ].area;
					AddAreaToOpenList( elevatorArea, area, startPos, maxRange );
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Derive your own custom search functor from this interface method for use with SearchSurroundingAreas below.
 */
class ISearchSurroundingAreasFunctor
{
public:
	virtual ~ISearchSurroundingAreasFunctor() { }

	/**
	 * Perform user-defined action on area.
	 * Return 'false' to end the search (ie: you found what you were looking for)
	 */
	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) = 0;

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		return !adjArea->IsBlocked( TEAM_ANY );
	}

	/**
	 * Collect adjacent areas to continue the search by calling 'IncludeInSearch' on each
	 */
	virtual void IterateAdjacentAreas( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) 
	{
		// search adjacent outgoing connections
		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

				if ( ShouldSearch( adjArea, area, travelDistanceSoFar ) )
				{
					IncludeInSearch( adjArea, area );
				}
			}
		}
	}

	// Invoked after the search has completed
	virtual void PostSearch( void ) { }

	// consider 'area' in upcoming search steps
	void IncludeInSearch( CNavArea *area, CNavArea *priorArea ) 
	{
		if ( area == NULL )
			return;

		if ( !area->IsMarked() )
		{
			area->Mark();
			area->SetTotalCost( 0.0f );
			area->SetParent( priorArea );

			// compute approximate travel distance from start area of search
			if ( priorArea )
			{
				float distAlong = priorArea->GetCostSoFar();
				distAlong += ( area->GetCenter() - priorArea->GetCenter() ).Length();
				area->SetCostSoFar( distAlong );
			}
			else
			{
				area->SetCostSoFar( 0.0f );
			}

			// adding an area to the open list also marks it
			area->AddToOpenList();
		}
	}
};


/**
 * Do a breadth-first search starting from 'startArea' and continuing outward based on
 * adjacent areas that pass the given filter
 */
inline void SearchSurroundingAreas( CNavArea *startArea, ISearchSurroundingAreasFunctor &func, float travelDistanceLimit = -1.0f )
{
	if ( startArea )
	{
		CNavArea::MakeNewMarker();
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetTotalCost( 0.0f );
		startArea->SetCostSoFar( 0.0f );
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVector< CNavArea * > adjVector;

		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			if ( travelDistanceLimit > 0.0f && area->GetCostSoFar() > travelDistanceLimit )
				continue;

			if ( func( area, area->GetParent(), area->GetCostSoFar() ) )
			{
				func.IterateAdjacentAreas( area, area->GetParent(), area->GetCostSoFar() );
			}
			else
			{
				// search aborted
				break;
			}
		}
	}

	func.PostSearch();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Starting from 'startArea', collect adjacent areas via a breadth-first search continuing outward until
 * 'travelDistanceLimit' is reached.
 * Areas in the collection will be "marked", returning true for IsMarked(). 
 * Each area in the collection's GetCostSoFar() will be approximate travel distance from 'startArea'.
 */
inline void CollectSurroundingAreas( CUtlVector< CNavArea * > *nearbyAreaVector, CNavArea *startArea, float travelDistanceLimit = 1500.0f, float maxStepUpLimit = StepHeight, float maxDropDownLimit = 100.0f )
{
	nearbyAreaVector->RemoveAll();

	if ( startArea )
	{
		CNavArea::MakeNewMarker();
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetTotalCost( 0.0f );
		startArea->SetCostSoFar( 0.0f );
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVector< CNavArea * > adjVector;

		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			if ( travelDistanceLimit > 0.0f && area->GetCostSoFar() > travelDistanceLimit )
				continue;

			if ( area->GetParent() )
			{
				float deltaZ = area->GetParent()->ComputeAdjacentConnectionHeightChange( area );

				if ( deltaZ > maxStepUpLimit )
					continue;

				if ( deltaZ < -maxDropDownLimit )
					continue;
			}

			nearbyAreaVector->AddToTail( area );

			// mark here to ensure all marked areas are also valid areas that are in the collection
			area->Mark();

			// search adjacent outgoing connections
			for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
			{
				int count = area->GetAdjacentCount( (NavDirType)dir );
				for( int i=0; i<count; ++i )
				{
					CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

					if ( adjArea->IsBlocked( TEAM_ANY ) )
					{
						continue;
					}

					if ( !adjArea->IsMarked() )
					{
						adjArea->SetTotalCost( 0.0f );
						adjArea->SetParent( area );

						// compute approximate travel distance from start area of search
						float distAlong = area->GetCostSoFar();
						distAlong += ( adjArea->GetCenter() - area->GetCenter() ).Length();
						adjArea->SetCostSoFar( distAlong );
						adjArea->AddToOpenList();
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor that returns lowest cost for farthest away areas
 * For use with FindMinimumCostArea()
 */
class FarAwayFunctor
{
public:
	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		if (area == fromArea)
			return 9999999.9f;

		return 1.0f/(fromArea->GetCenter() - area->GetCenter()).Length();
	}
};

/**
 * Functor that returns lowest cost for areas farthest from given position
 * For use with FindMinimumCostArea()
 */
class FarAwayFromPositionFunctor 
{
public:
	FarAwayFromPositionFunctor( const Vector &pos ) : m_pos( pos )
	{
	}

	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		return 1.0f/(m_pos - area->GetCenter()).Length();
	}

private:
	const Vector &m_pos;
};


/**
 * Pick a low-cost area of "decent" size
 */
template< typename CostFunctor >
CNavArea *FindMinimumCostArea( CNavArea *startArea, CostFunctor &costFunc )
{
	const float minSize = 150.0f;

	// collect N low-cost areas of a decent size
	enum { NUM_CHEAP_AREAS = 32 };
	struct 
	{
		CNavArea *area;
		float cost;
	}
	cheapAreaSet[ NUM_CHEAP_AREAS ] = {};
	int cheapAreaSetCount = 0;

	FOR_EACH_VEC( TheNavAreas, iter )
	{
		CNavArea *area = TheNavAreas[iter];

		// skip the small areas
		if ( area->GetSizeX() < minSize || area->GetSizeY() < minSize)
			continue;

		// compute cost of this area

		// HPE_FIX[pfreese]: changed this to only pass three parameters, in accord with the two functors above
		float cost = costFunc( area, startArea, NULL );

		if (cheapAreaSetCount < NUM_CHEAP_AREAS)
		{
			cheapAreaSet[ cheapAreaSetCount ].area = area;
			cheapAreaSet[ cheapAreaSetCount++ ].cost = cost;
		}
		else
		{
			// replace most expensive cost if this is cheaper
			int expensive = 0;
			for( int i=1; i<NUM_CHEAP_AREAS; ++i )
				if (cheapAreaSet[i].cost > cheapAreaSet[expensive].cost)
					expensive = i;

			if (cheapAreaSet[expensive].cost > cost)
			{
				cheapAreaSet[expensive].area = area;
				cheapAreaSet[expensive].cost = cost;
			}
		}
	}

	if (cheapAreaSetCount)
	{
		// pick one of the areas at random
		return cheapAreaSet[ RandomInt( 0, cheapAreaSetCount-1 ) ].area;
	}
	else
	{
		// degenerate case - no decent sized areas - pick a random area
		int numAreas = TheNavAreas.Count();
		int which = RandomInt( 0, numAreas-1 );

		FOR_EACH_VEC( TheNavAreas, iter )
		{
			if (which-- == 0)
				return TheNavAreas[iter];
		}

	}
	return cheapAreaSet[ RandomInt( 0, cheapAreaSetCount-1 ) ].area;
}


//--------------------------------------------------------------------------------------------------------
//
// Given a vector of CNavAreas (or derived types), 'inVector', populate 'outVector' with a randomly shuffled set
// of 'maxCount' areas that are at least 'minSeparation' travel distance apart from each other.
//
template< typename T >
void SelectSeparatedShuffleSet( int maxCount, float minSeparation, const CUtlVector< T * > &inVector, CUtlVector< T * > *outVector )
{
	if ( !outVector )
		return;

	outVector->RemoveAll();

	CUtlVector< T * > shuffledVector;

	int i, j;

	for( i=0; i<inVector.Count(); ++i )
	{
		shuffledVector.AddToTail( inVector[i] );
	}

	// randomly shuffle the order
	int n = shuffledVector.Count();
	while( n > 1 )
	{
		int k = RandomInt( 0, n-1 );
		n--;

		T *tmp = shuffledVector[n];
		shuffledVector[n] = shuffledVector[k];
		shuffledVector[k] = tmp;
	}

	// enforce minSeparation between shuffled areas
	for( i=0; i<shuffledVector.Count(); ++i )
	{
		T *area = shuffledVector[i];

		CUtlVector< CNavArea * > nearVector;
		CollectSurroundingAreas( &nearVector, area, minSeparation, 2.0f * StepHeight, 2.0f * StepHeight );

		for( j=0; j<i; ++j )
		{
			if ( nearVector.HasElement( (CNavArea *)shuffledVector[j] ) )
			{
				// this area is too near an area earlier in the vector
				break;
			}
		}

		if ( j == i )
		{
			// separated from all prior areas
			outVector->AddToTail( area );

			if ( outVector->Count() >= maxCount )
				return;
		}
	}
}


#endif // _NAV_PATHFIND_H_
