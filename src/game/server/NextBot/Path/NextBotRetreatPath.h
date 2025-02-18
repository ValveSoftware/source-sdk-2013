// NextBotRetreatPath.h
// Maintain and follow a path that leads safely away from the given Actor
// Author: Michael Booth, February 2007
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_RETREAT_PATH_
#define _NEXT_BOT_RETREAT_PATH_

#include "nav.h"
#include "NextBotInterface.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotRetreatPath.h"
#include "NextBotUtil.h"
#include "NextBotPathFollow.h"
#include "tier0/vprof.h"


//----------------------------------------------------------------------------------------------
/**
 * A RetreatPath extends a PathFollower to periodically recompute a path 
 * away from a threat, and to move along the path away from that threat.
 */
class RetreatPath : public PathFollower
{
public:
	RetreatPath( void );
	virtual ~RetreatPath() { }

	void Update( INextBot *bot, CBaseEntity *threat );	// update path away from threat and move bot along path

	virtual float GetMaxPathLength( void ) const;		// return maximum path length

	virtual void Invalidate( void );					// (EXTEND) cause the path to become invalid

private:
	void RefreshPath( INextBot *bot, CBaseEntity *threat );

	CountdownTimer m_throttleTimer;						// require a minimum time between re-paths
	EHANDLE m_pathThreat;								// the threat of our existing path
	Vector m_pathThreatPos;								// where the threat was when the path was built
};

inline RetreatPath::RetreatPath( void )
{
	m_throttleTimer.Invalidate();
	m_pathThreat = NULL;
}

inline float RetreatPath::GetMaxPathLength( void ) const
{
	return 1000.0f;
}

inline void RetreatPath::Invalidate( void )
{
	// path is gone, repath at earliest opportunity
	m_throttleTimer.Invalidate();
	m_pathThreat = NULL;

	// extend
	PathFollower::Invalidate();	
}



//----------------------------------------------------------------------------------------------
/**
 * Maintain a path to our chase threat and move along that path
 */
inline void RetreatPath::Update( INextBot *bot, CBaseEntity *threat )
{
	VPROF_BUDGET( "RetreatPath::Update", "NextBot" );

	if ( threat == NULL )
	{
		return;
	}

	// if our path threat changed, repath immediately
	if ( threat != m_pathThreat )
	{
		if ( bot->IsDebugging( INextBot::PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) Chase path threat changed (from %X to %X).\n", gpGlobals->curtime, bot->GetEntity()->entindex(), m_pathThreat.Get(), threat );
		}

		Invalidate();
	}

	// maintain the path away from the threat
	RefreshPath( bot, threat );

	// move along the path towards the threat
	PathFollower::Update( bot );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Build a path away from retreatFromArea up to retreatRange in length.
 */
class RetreatPathBuilder
{
public:
	RetreatPathBuilder( INextBot *me, CBaseEntity *threat, float retreatRange = 500.0f )
	{
		m_me = me;
		m_mover = me->GetLocomotionInterface();
		
		m_threat = threat;
		m_retreatRange = retreatRange;
	}

	CNavArea *ComputePath( void )
	{
		VPROF_BUDGET( "NavAreaBuildRetreatPath", "NextBot" );
		
		if ( m_mover == NULL )
			return NULL;
		
		CNavArea *startArea = m_me->GetEntity()->GetLastKnownArea();

		if ( startArea == NULL )
			return NULL;

		CNavArea *retreatFromArea = TheNavMesh->GetNearestNavArea( m_threat->GetAbsOrigin() );
		if ( retreatFromArea == NULL )
			return NULL;

		startArea->SetParent( NULL );

		// start search
		CNavArea::ClearSearchLists();

		float initCost = Cost( startArea, NULL, NULL );
		if ( initCost < 0.0f )
			return NULL;

		int teamID = m_me->GetEntity()->GetTeamNumber();

		startArea->SetTotalCost( initCost );

		startArea->AddToOpenList();
		
		// keep track of the area farthest away from the threat
		CNavArea *farthestArea = NULL;
		float farthestRange = 0.0f;

		//
		// Dijkstra's algorithm (since we don't know our goal).
		// Build a path as far away from the retreat area as possible.
		// Minimize total path length and danger.
		// Maximize distance to threat of end of path.
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
				if ( ( newArea->GetCenter() - m_me->GetEntity()->GetAbsOrigin() ).IsLengthGreaterThan( m_retreatRange ) )
					continue;
				
				// determine cost of traversing this area
				float newCost = Cost( newArea, area, m_adjAreaVector[ i ].ladder );
				
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
					// whether this area has been visited or not, we now have a better path
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

					// keep track of area farthest from threat
					float threatRange = ( newArea->GetCenter() - m_threat->GetAbsOrigin() ).Length();
					if ( threatRange > farthestRange )
					{
						farthestArea = newArea;
						farthestRange = threatRange;
					}
				}
			}
		}

		return farthestArea;
	}


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
	
	/**
	 * Cost minimizes path length traveled thus far and "danger" (proximity to threat(s))
	 */
	float Cost( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		// check if we can use this area
		if ( !m_mover->IsAreaTraversable( area ) )
		{
			return -1.0f;
		}

		int teamID = m_me->GetEntity()->GetTeamNumber();
		if ( area->IsBlocked( teamID ) )
		{
			return -1.0f;
		}
		
		const float debugDeltaT = 3.0f;

		float cost;

		const float maxThreatRange = 500.0f;
		const float dangerDensity = 1000.0f;

		if ( fromArea == NULL )
		{
			cost = 0.0f;
			
			if ( area->Contains( m_threat->GetAbsOrigin() ) )
			{
				// maximum danger - threat is in the area with us
				cost += 10.0f * dangerDensity;
				
				if ( m_me->IsDebugging( INextBot::PATH ) )
				{
					area->DrawFilled( 255, 0, 0, 128 );
				}
			}
			else
			{
				// danger proportional to range to us
				float rangeToThreat = ( m_threat->GetAbsOrigin() - m_me->GetEntity()->GetAbsOrigin() ).Length();

				if ( rangeToThreat < maxThreatRange )
				{
					cost += dangerDensity * ( 1.0f - ( rangeToThreat / maxThreatRange ) );

					if ( m_me->IsDebugging( INextBot::PATH ) )
					{
						NDebugOverlay::Line( m_me->GetEntity()->GetAbsOrigin(), m_threat->GetAbsOrigin(), 255, 0, 0, true, debugDeltaT );
					}
				}				
			}
		}
		else
		{
			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				const float ladderCostFactor = 100.0f;
				dist = ladderCostFactor * ladder->m_length;
			}
			else
			{
				Vector to = area->GetCenter() - fromArea->GetCenter();

				dist = to.Length();

				// check for vertical discontinuities
				Vector closeFrom, closeTo;
				area->GetClosestPointOnArea( fromArea->GetCenter(), &closeTo );
				fromArea->GetClosestPointOnArea( area->GetCenter(), &closeFrom );
				
				float deltaZ = closeTo.z - closeFrom.z;

				if ( deltaZ > m_mover->GetMaxJumpHeight() )
				{
					// too high to jump
					return -1.0f;
				}
				else if ( -deltaZ > m_mover->GetDeathDropHeight() )
				{
					// too far down to drop
					return -1.0f;
				}

				// prefer to maintain our level
				const float climbCost = 10.0f;
				dist += climbCost * fabs( deltaZ );
			}

			cost = dist + fromArea->GetTotalCost();

			
			// Add in danger cost due to threat
			// Assume straight line between areas and find closest point
			// to the threat along that line segment. The distance between
			// the threat and closest point on the line is the danger cost.		
			
			// path danger is CUMULATIVE
			float dangerCost = fromArea->GetCostSoFar();
			
			Vector close;
			float t;
			CalcClosestPointOnLineSegment( m_threat->GetAbsOrigin(), area->GetCenter(), fromArea->GetCenter(), close, &t );
			if ( t < 0.0f )
			{
				close = area->GetCenter();
			}
			else if ( t > 1.0f )
			{
				close = fromArea->GetCenter();
			}

			float rangeToThreat = ( m_threat->GetAbsOrigin() - close ).Length();

			if ( rangeToThreat < maxThreatRange )
			{
				float dangerFactor = 1.0f - ( rangeToThreat / maxThreatRange );
				dangerCost = dangerDensity * dangerFactor;

				if ( m_me->IsDebugging( INextBot::PATH ) )
				{
					NDebugOverlay::HorzArrow( fromArea->GetCenter(), area->GetCenter(), 5, 255 * dangerFactor, 0, 0, 255, true, debugDeltaT );

					Vector to = close - m_threat->GetAbsOrigin();
					to.NormalizeInPlace();

					NDebugOverlay::Line( close, close - 50.0f * to, 255, 0, 0, true, debugDeltaT );
				}
			}
			
			cost += dangerCost;
		}

		return cost;
	}
	
private:	
	INextBot *m_me;
	ILocomotion *m_mover;
	
	CBaseEntity *m_threat;
	float m_retreatRange;

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


//----------------------------------------------------------------------------------------------
/**
 * Periodically rebuild the path away from our threat
 */
inline void RetreatPath::RefreshPath( INextBot *bot, CBaseEntity *threat )
{
	VPROF_BUDGET( "RetreatPath::RefreshPath", "NextBot" );

	if ( threat == NULL )
	{
		if ( bot->IsDebugging( INextBot::PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) CasePath::RefreshPath failed. No threat.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
		}
		return;
	}

	// don't change our path if we're on a ladder
	ILocomotion *mover = bot->GetLocomotionInterface();
	if ( IsValid() && mover && mover->IsUsingLadder() )
	{
		if ( bot->IsDebugging( INextBot::PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) RetreatPath::RefreshPath failed. Bot is on a ladder.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
		}
		return;
	}

	// the closer we get, the more accurate our path needs to be
	Vector to = threat->GetAbsOrigin() - bot->GetPosition();
	
	const float minTolerance = 0.0f;
	const float toleranceRate = 0.33f;
	
	float tolerance = minTolerance + toleranceRate * to.Length();

	if ( !IsValid() || ( threat->GetAbsOrigin() - m_pathThreatPos ).IsLengthGreaterThan( tolerance ) )
	{
		if ( !m_throttleTimer.IsElapsed() )
		{
			// require a minimum time between repaths, as long as we have a path to follow
			if ( bot->IsDebugging( INextBot::PATH ) )
			{
				DevMsg( "%3.2f: bot(#%d) RetreatPath::RefreshPath failed. Rate throttled.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
			}
			return;
		}

		// remember our path threat
		m_pathThreat = threat;
		m_pathThreatPos = threat->GetAbsOrigin();

		RetreatPathBuilder retreat( bot, threat, GetMaxPathLength() );

		CNavArea *goalArea = retreat.ComputePath();

		if ( goalArea )
		{
			AssemblePrecomputedPath( bot, goalArea->GetCenter(), goalArea );
		}	
		else
		{
			// all adjacent areas are too far away - just move directly away from threat
			Vector to = threat->GetAbsOrigin() - bot->GetPosition();

			BuildTrivialPath( bot, bot->GetPosition() - to );
		}
			
		const float minRepathInterval = 0.5f;
		m_throttleTimer.Start( minRepathInterval );
	}
}



#endif // _NEXT_BOT_RETREAT_PATH_
