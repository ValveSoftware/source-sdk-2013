//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_generate.cpp
// Auto-generate a Navigation Mesh by sampling the current map
// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003

#include "cbase.h"
#include "util_shared.h"
#include "nav_mesh.h"
#include "nav_node.h"
#include "nav_pathfind.h"
#include "viewport_panel_names.h"
//#include "terror/TerrorShared.h"
#include "fmtstr.h"

#ifdef TERROR
#include "func_simpleladder.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


enum { MAX_BLOCKED_AREAS = 256 };
static unsigned int blockedID[ MAX_BLOCKED_AREAS ];
static int blockedIDCount = 0;
static float lastMsgTime = 0.0f;

bool TraceAdjacentNode( int depth, const Vector& start, const Vector& end, trace_t *trace, float zLimit = DeathDrop );
bool StayOnFloor( trace_t *trace, float zLimit = DeathDrop );

ConVar nav_slope_limit( "nav_slope_limit", "0.7", FCVAR_CHEAT, "The ground unit normal's Z component must be greater than this for nav areas to be generated." );
ConVar nav_slope_tolerance( "nav_slope_tolerance", "0.1", FCVAR_CHEAT, "The ground unit normal's Z component must be this close to the nav area's Z component to be generated." );
ConVar nav_displacement_test( "nav_displacement_test", "10000", FCVAR_CHEAT, "Checks for nodes embedded in displacements (useful for in-development maps)" );
ConVar nav_generate_fencetops( "nav_generate_fencetops", "1", FCVAR_CHEAT, "Autogenerate nav areas on fence and obstacle tops" );
ConVar nav_generate_fixup_jump_areas( "nav_generate_fixup_jump_areas", "1", FCVAR_CHEAT, "Convert obsolete jump areas into 2-way connections" );
ConVar nav_generate_jump_connections( "nav_generate_jump_connections", "1", FCVAR_CHEAT, "If disabled, don't generate jump connections from jump areas" );
ConVar nav_generate_incremental_range( "nav_generate_incremental_range", "2000", FCVAR_CHEAT );
ConVar nav_generate_incremental_tolerance( "nav_generate_incremental_tolerance", "0", FCVAR_CHEAT, "Z tolerance for adding new nav areas." );
ConVar nav_area_max_size( "nav_area_max_size", "50", FCVAR_CHEAT, "Max area size created in nav generation" );

// Common bounding box for traces
Vector NavTraceMins( -0.45, -0.45, 0 );
Vector NavTraceMaxs( 0.45, 0.45, HumanCrouchHeight );
bool FindGroundForNode( Vector *pos, Vector *normal );	// find a ground Z for pos that is clear for NavTraceMins -> NavTraceMaxs

const float MaxTraversableHeight = StepHeight;		// max internal obstacle height that can occur between nav nodes and safely disregarded
const float MinObstacleAreaWidth = 10.0f;			// min width of a nav area we will generate on top of an obstacle

//--------------------------------------------------------------------------------------------------------------
/**
 * Shortest path cost, paying attention to "blocked" areas
 */
class ApproachAreaCost
{
public:
	float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator )
	{
		// check if this area is "blocked"
		for( int i=0; i<blockedIDCount; ++i )
		{
			if (area->GetID() == blockedID[i])
			{
				return -1.0f;
			}
		}

		if (fromArea == NULL)
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			// compute distance traveled along path so far
			float dist;

			if (ladder)
			{
				dist = ladder->m_length;
			}
			else
			{
				dist = (area->GetCenter() - fromArea->GetCenter()).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			return cost;
		}
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Start at given position and find first area in given direction
 */
inline CNavArea *findFirstAreaInDirection( const Vector *start, NavDirType dir, float range, float beneathLimit, CBaseEntity *traceIgnore = NULL, Vector *closePos = NULL )
{
	CNavArea *area = NULL;

	Vector pos = *start;

	int end = (int)((range / GenerationStepSize) + 0.5f);

	for( int i=1; i<=end; i++ )
	{
		AddDirectionVector( &pos, dir, GenerationStepSize );

		// make sure we dont look thru the wall
		trace_t result;

		UTIL_TraceHull( *start, pos, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), traceIgnore, COLLISION_GROUP_NONE, &result );

		if (result.fraction < 1.0f)
			break;

		area = TheNavMesh->GetNavArea( pos, beneathLimit );
		if (area)
		{
			if (closePos)
			{
				closePos->x = pos.x;
				closePos->y = pos.y;
				closePos->z = area->GetZ( pos.x, pos.y );
			}

			break;
		}
	}

	return area;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * For each ladder in the map, create a navigation representation of it.
 */
void CNavMesh::BuildLadders( void )
{
	// remove any left-over ladders
	DestroyLadders();

#ifdef TERROR
	CFuncSimpleLadder *ladder = NULL;
	while( (ladder = dynamic_cast< CFuncSimpleLadder * >(gEntList.FindEntityByClassname( ladder, "func_simpleladder" ))) != NULL )
	{
		Vector mins, maxs;
		ladder->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
		CreateLadder( mins, maxs, 0.0f );
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Create a navigation representation of a ladder.
 */
void CNavMesh::CreateLadder( const Vector& absMin, const Vector& absMax, float maxHeightAboveTopArea )
{
	CNavLadder *ladder = new CNavLadder;

	// compute top & bottom of ladder
	ladder->m_top.x = (absMin.x + absMax.x) / 2.0f;
	ladder->m_top.y = (absMin.y + absMax.y) / 2.0f;
	ladder->m_top.z = absMax.z;

	ladder->m_bottom.x = ladder->m_top.x;
	ladder->m_bottom.y = ladder->m_top.y;
	ladder->m_bottom.z = absMin.z;

	// determine facing - assumes "normal" runged ladder
	float xSize = absMax.x - absMin.x;
	float ySize = absMax.y - absMin.y;
	trace_t result;
	if (xSize > ySize)
	{
		// ladder is facing north or south - determine which way
		// "pull in" traceline from bottom and top in case ladder abuts floor and/or ceiling
		Vector from = ladder->m_bottom + Vector( 0.0f, GenerationStepSize, GenerationStepSize/2 );
		Vector to = ladder->m_top + Vector( 0.0f, GenerationStepSize, -GenerationStepSize/2 );

		UTIL_TraceLine( from, to, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction != 1.0f || result.startsolid)
			ladder->SetDir( NORTH );
		else
			ladder->SetDir( SOUTH );

		ladder->m_width = xSize;
	}
	else
	{
		// ladder is facing east or west - determine which way
		Vector from = ladder->m_bottom + Vector( GenerationStepSize, 0.0f, GenerationStepSize/2 );
		Vector to = ladder->m_top + Vector( GenerationStepSize, 0.0f, -GenerationStepSize/2 );

		UTIL_TraceLine( from, to, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction != 1.0f || result.startsolid)
			ladder->SetDir( WEST );
		else
			ladder->SetDir( EAST );

		ladder->m_width = ySize;
	}

	// adjust top and bottom of ladder to make sure they are reachable
	// (cs_office has a crate right in front of the base of a ladder)
	Vector along = ladder->m_top - ladder->m_bottom;
	float length = along.NormalizeInPlace();
	Vector on, out;
	const float minLadderClearance = 32.0f;

	// adjust bottom to bypass blockages
	const float inc = 10.0f;
	float t;		
	for( t = 0.0f; t <= length; t += inc )
	{
		on = ladder->m_bottom + t * along;

		out = on + ladder->GetNormal() * minLadderClearance;

		UTIL_TraceLine( on, out, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction == 1.0f && !result.startsolid)
		{
			// found viable ladder bottom
			ladder->m_bottom = on;
			break;
		}
	}

	// adjust top to bypass blockages
	for( t = 0.0f; t <= length; t += inc )
	{
		on = ladder->m_top - t * along;

		out = on + ladder->GetNormal() * minLadderClearance;

		UTIL_TraceLine( on, out, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction == 1.0f && !result.startsolid)
		{
			// found viable ladder top
			ladder->m_top = on;
			break;
		}
	}

	ladder->m_length = (ladder->m_top - ladder->m_bottom).Length();

	ladder->SetDir( ladder->GetDir() );	// now that we've adjusted the top and bottom, re-check the normal

	ladder->m_bottomArea = NULL;
	ladder->m_topForwardArea = NULL;
	ladder->m_topLeftArea = NULL;
	ladder->m_topRightArea = NULL;
	ladder->m_topBehindArea = NULL;
	ladder->ConnectGeneratedLadder( maxHeightAboveTopArea );

	// add ladder to global list
	m_ladders.AddToTail( ladder );		
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Create a navigation representation of a ladder.
 */
void CNavMesh::CreateLadder( const Vector &top, const Vector &bottom, float width, const Vector2D &ladderDir, float maxHeightAboveTopArea )
{
	CNavLadder *ladder = new CNavLadder;

	ladder->m_top = top;
	ladder->m_bottom = bottom;
	ladder->m_width = width;
	if ( fabs( ladderDir.x ) > fabs( ladderDir.y ) )
	{
		if ( ladderDir.x > 0.0f )
		{
			ladder->SetDir( EAST );
		}
		else
		{
			ladder->SetDir( WEST );
		}
	}
	else
	{
		if ( ladderDir.y > 0.0f )
		{
			ladder->SetDir( SOUTH );
		}
		else
		{
			ladder->SetDir( NORTH );
		}
	}

	// adjust top and bottom of ladder to make sure they are reachable
	// (cs_office has a crate right in front of the base of a ladder)
	Vector along = ladder->m_top - ladder->m_bottom;
	float length = along.NormalizeInPlace();
	Vector on, out;
	const float minLadderClearance = 32.0f;

	// adjust bottom to bypass blockages
	const float inc = 10.0f;
	float t;
	trace_t result;
	for( t = 0.0f; t <= length; t += inc )
	{
		on = ladder->m_bottom + t * along;

		out = on + ladder->GetNormal() * minLadderClearance;

		UTIL_TraceLine( on, out, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction == 1.0f && !result.startsolid)
		{
			// found viable ladder bottom
			ladder->m_bottom = on;
			break;
		}
	}

	// adjust top to bypass blockages
	for( t = 0.0f; t <= length; t += inc )
	{
		on = ladder->m_top - t * along;

		out = on + ladder->GetNormal() * minLadderClearance;

		UTIL_TraceLine( on, out, GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );

		if (result.fraction == 1.0f && !result.startsolid)
		{
			// found viable ladder top
			ladder->m_top = on;
			break;
		}
	}

	ladder->m_length = (ladder->m_top - ladder->m_bottom).Length();

	ladder->SetDir( ladder->GetDir() );	// now that we've adjusted the top and bottom, re-check the normal

	ladder->m_bottomArea = NULL;
	ladder->m_topForwardArea = NULL;
	ladder->m_topLeftArea = NULL;
	ladder->m_topRightArea = NULL;
	ladder->m_topBehindArea = NULL;
	ladder->ConnectGeneratedLadder( maxHeightAboveTopArea );

	// add ladder to global list
	m_ladders.AddToTail( ladder );
}


//--------------------------------------------------------------------------------------------------------------
void CNavLadder::ConnectGeneratedLadder( float maxHeightAboveTopArea )
{
	const float nearLadderRange = 75.0f;		// 50

	//
	// Find naviagtion area at bottom of ladder
	//

	// get approximate postion of player on ladder
	Vector center = m_bottom + Vector( 0, 0, GenerationStepSize );
	AddDirectionVector( &center, m_dir, HalfHumanWidth );

	m_bottomArea = TheNavMesh->GetNearestNavArea( center, true );
	if (!m_bottomArea)
	{
		DevMsg( "ERROR: Unconnected ladder bottom at ( %g, %g, %g )\n", m_bottom.x, m_bottom.y, m_bottom.z );
	}
	else
	{
		// store reference to ladder in the area
		m_bottomArea->AddLadderUp( this );
	}

	//
	// Find adjacent navigation areas at the top of the ladder
	//

	// get approximate postion of player on ladder
	center = m_top + Vector( 0, 0, GenerationStepSize );
	AddDirectionVector( &center, m_dir, HalfHumanWidth );

	float beneathLimit = MIN( 120.0f, m_top.z - m_bottom.z + HalfHumanWidth );

	// find "ahead" area
	m_topForwardArea = findFirstAreaInDirection( &center, OppositeDirection( m_dir ), nearLadderRange, beneathLimit, NULL );
	if (m_topForwardArea == m_bottomArea)
		m_topForwardArea = NULL;

	// find "left" area
	m_topLeftArea = findFirstAreaInDirection( &center, DirectionLeft( m_dir ), nearLadderRange, beneathLimit, NULL );
	if (m_topLeftArea == m_bottomArea)
		m_topLeftArea = NULL;

	// find "right" area
	m_topRightArea = findFirstAreaInDirection( &center, DirectionRight( m_dir ), nearLadderRange, beneathLimit, NULL );
	if (m_topRightArea == m_bottomArea)
		m_topRightArea = NULL;

	// find "behind" area - must look farther, since ladder is against the wall away from this area
	m_topBehindArea = findFirstAreaInDirection( &center, m_dir, 2.0f*nearLadderRange, beneathLimit, NULL );
	if (m_topBehindArea == m_bottomArea)
		m_topBehindArea = NULL;

	// can't include behind area, since it is not used when going up a ladder
	if (!m_topForwardArea && !m_topLeftArea && !m_topRightArea)
		DevMsg( "ERROR: Unconnected ladder top at ( %g, %g, %g )\n", m_top.x, m_top.y, m_top.z );

	// store reference to ladder in the area(s)
	if (m_topForwardArea)
		m_topForwardArea->AddLadderDown( this );

	if (m_topLeftArea)
		m_topLeftArea->AddLadderDown( this );

	if (m_topRightArea)
		m_topRightArea->AddLadderDown( this );

	if (m_topBehindArea)
	{
		m_topBehindArea->AddLadderDown( this );
		Disconnect( m_topBehindArea );
	}

	// adjust top of ladder to highest connected area
	float topZ = m_bottom.z + 5.0f;
	bool topAdjusted = false;
	CNavArea *topAreaList[4];
	topAreaList[0] = m_topForwardArea;
	topAreaList[1] = m_topLeftArea;
	topAreaList[2] = m_topRightArea;
	topAreaList[3] = m_topBehindArea;

	for( int a=0; a<4; ++a )
	{
		CNavArea *topArea = topAreaList[a];
		if (topArea == NULL)
			continue;

		Vector close;
		topArea->GetClosestPointOnArea( m_top, &close );
		if (topZ < close.z)
		{
			topZ = close.z;
			topAdjusted = true;
		}
	}

	if (topAdjusted)
	{
		if ( maxHeightAboveTopArea > 0.0f )
		{
			m_top.z = MIN( topZ + maxHeightAboveTopArea, m_top.z );
		}
		else
		{
			m_top.z = topZ;	// not manually specifying a top, so snap exactly
		}
	}

	//
	// Determine whether this ladder is "dangling" or not
	// "Dangling" ladders are too high to go up
	//
	if (m_bottomArea)
	{
		Vector bottomSpot;
		m_bottomArea->GetClosestPointOnArea( m_bottom, &bottomSpot );
		if (m_bottom.z - bottomSpot.z > HumanHeight)
		{
			m_bottomArea->Disconnect( this );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
class JumpConnector
{
public:
	bool operator()( CNavArea *jumpArea )
	{
		if ( !nav_generate_jump_connections.GetBool() )
		{
			return true;
		}

		if ( !(jumpArea->GetAttributes() & NAV_MESH_JUMP) )
		{
			return true;
		}

		for ( int i=0; i<NUM_DIRECTIONS; ++i )
		{
			NavDirType incomingDir = (NavDirType)i;
			NavDirType outgoingDir = OppositeDirection( incomingDir );

			const NavConnectVector *incoming = jumpArea->GetIncomingConnections( incomingDir );
			const NavConnectVector *from = jumpArea->GetAdjacentAreas( incomingDir );
			const NavConnectVector *dest = jumpArea->GetAdjacentAreas( outgoingDir );

			TryToConnect( jumpArea, incoming, dest, outgoingDir );
			TryToConnect( jumpArea, from, dest, outgoingDir );
		}

		return true;
	}

private:
	struct Connection
	{
		CNavArea *source;
		CNavArea *dest;
		NavDirType direction;
	};

	void TryToConnect( CNavArea *jumpArea, const NavConnectVector *source, const NavConnectVector *dest, NavDirType outgoingDir )
	{
		FOR_EACH_VEC( (*source), sourceIt )
		{
			CNavArea *sourceArea = const_cast< CNavArea * >( (*source)[ sourceIt ].area );
			if ( !sourceArea->IsConnected( jumpArea, outgoingDir ) )
			{
				continue;
			}

			if ( sourceArea->HasAttributes( NAV_MESH_JUMP ) )
			{
				NavDirType incomingDir = OppositeDirection( outgoingDir );
				const NavConnectVector *in1 = sourceArea->GetIncomingConnections( incomingDir );
				const NavConnectVector *in2 = sourceArea->GetAdjacentAreas( incomingDir );

				TryToConnect( jumpArea, in1, dest, outgoingDir );
				TryToConnect( jumpArea, in2, dest, outgoingDir );

				continue;
			}

			TryToConnect( jumpArea, sourceArea, dest, outgoingDir );
		}
	}

	void TryToConnect( CNavArea *jumpArea, CNavArea *sourceArea, const NavConnectVector *dest, NavDirType outgoingDir )
	{
		FOR_EACH_VEC( (*dest), destIt )
		{
			CNavArea *destArea = const_cast< CNavArea * >( (*dest)[ destIt ].area );
			if ( destArea->HasAttributes( NAV_MESH_JUMP ) )
			{
				// Don't connect areas across 2 jump areas.  This means we'll have some missing links due to sampling errors.
				// This is preferable to generating incorrect links across multiple jump areas, which is far more common.
				continue;
			}

			Vector center;
			float halfWidth;
			sourceArea->ComputePortal( destArea, outgoingDir, &center, &halfWidth );

			// Don't create corner-to-corner connections
			if ( halfWidth <= 0.0f )
			{
				continue;
			}

			Vector dir( vec3_origin );
			AddDirectionVector( &dir, outgoingDir, 5.0f );

			if ( halfWidth > 0.0f )
			{
				Vector sourcePos, destPos;
				sourceArea->GetClosestPointOnArea( center, &sourcePos );
				destArea->GetClosestPointOnArea( center, &destPos );

				// No jumping up from stairs.
				if ( sourceArea->HasAttributes( NAV_MESH_STAIRS ) && sourcePos.z + StepHeight < destPos.z )
				{
					continue;
				}

				if ( (sourcePos-destPos).AsVector2D().IsLengthLessThan( GenerationStepSize * 3 ) )
				{
					sourceArea->ConnectTo( destArea, outgoingDir );
//					DevMsg( "Connected %d->%d via %d (len %f)\n",
//						sourceArea->GetID(), destArea->GetID(), jumpArea->GetID(), sourcePos.DistTo( destPos ) );
				}
			}
		}
	}
};

//--------------------------------------------------------------------------------------------------------------
void CNavMesh::MarkPlayerClipAreas( void )
{
#ifdef TERROR
	FOR_EACH_VEC( TheNavAreas, it )
	{
		TerrorNavArea *area = static_cast< TerrorNavArea * >(TheNavAreas[it]);

		// Trace upward a bit from our center point just colliding wtih PLAYERCLIP to see if we're in one, if we are, mark us as accordingly.
		trace_t trace;
		Vector start = area->GetCenter() + Vector(0.0f, 0.0f, 16.0f );
		Vector end = area->GetCenter() + Vector(0.0f, 0.0f, 32.0f );
		UTIL_TraceHull( start, end, Vector(0,0,0), Vector(0,0,0), CONTENTS_PLAYERCLIP, NULL, &trace);

		if ( trace.fraction < 1.0 )
		{
			area->SetAttributes( area->GetAttributes() | TerrorNavArea::NAV_PLAYERCLIP );
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Mark all areas that require a jump to get through them.
 * This currently relies on jump areas having extreme slope.
 */
void CNavMesh::MarkJumpAreas( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		if ( !area->HasNodes() )
			continue;

		Vector normal, otherNormal;
		area->ComputeNormal( &normal );
		area->ComputeNormal( &otherNormal, true );

		float lowestNormalZ = MIN( normal.z, otherNormal.z );
		if (lowestNormalZ < nav_slope_limit.GetFloat())
		{
			// The area is a jump area, and we don't merge jump areas together
			area->SetAttributes( area->GetAttributes() | NAV_MESH_JUMP | NAV_MESH_NO_MERGE );
		}
		else if ( lowestNormalZ < nav_slope_limit.GetFloat() + nav_slope_tolerance.GetFloat() )
		{
			Vector testPos = area->GetCenter();
			testPos.z += HalfHumanHeight;
			Vector groundNormal;
			float dummy;
			if ( GetSimpleGroundHeight( testPos, &dummy, &groundNormal ) )
			{
				// If the ground normal is divergent from the area's normal, mark it as a jump area - it's not
				// really representative of the ground.
				float deltaNormalZ = fabs( groundNormal.z - lowestNormalZ );
				if ( deltaNormalZ > nav_slope_tolerance.GetFloat() )
				{
					// The area is a jump area, and we don't merge jump areas together
					area->SetAttributes( area->GetAttributes() | NAV_MESH_JUMP | NAV_MESH_NO_MERGE );
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
* Remove all areas marked as jump areas and connect the areas connecting to them 
*
*/
void CNavMesh::StichAndRemoveJumpAreas( void )
{
	// Now, go through and remove jump areas, connecting areas to make up for it
	JumpConnector connector;
	ForAllAreas( connector );
	RemoveJumpAreas();
}


//--------------------------------------------------------------------------------------------------------------
/**
* Adjusts obstacle start and end distances such that obstacle width (end-start) is not less than MinObstacleAreaWidth,
* and end distance is not greater than maxAllowedDist
*/
void AdjustObstacleDistances( float *pObstacleStartDist, float *pObstacleEndDist, float maxAllowedDist )
{
	float obstacleWidth = *pObstacleEndDist - *pObstacleStartDist;
	// is the obstacle width too narrow?
	if ( obstacleWidth < MinObstacleAreaWidth )
	{		
		float halfDelta = ( MinObstacleAreaWidth - obstacleWidth ) /2;
		// move start so it's half of min width from center, but no less than zero
		*pObstacleStartDist = MAX( *pObstacleStartDist - halfDelta, 0 );
		// move end so it's min width from start
		*pObstacleEndDist = *pObstacleStartDist + MinObstacleAreaWidth;

		// if this pushes the end past max allowed distance, pull start and end back so that end is within allowed distance
		if ( *pObstacleEndDist > maxAllowedDist )
		{
			float delta = *pObstacleEndDist - maxAllowedDist;
			*pObstacleStartDist -= delta;
			*pObstacleEndDist -= delta;
		}
	}								
}

//--------------------------------------------------------------------------------------------------------------
/**
* Makes sure tall, slim obstacles like fencetops, railings and narrow walls have nav areas placed on top of them
* to allow climbing & traversal
*/
void CNavMesh::HandleObstacleTopAreas( void )
{
	if ( !nav_generate_fencetops.GetBool() )
		return;

	// For any 1x1 area that is internally blocked by an obstacle, raise it on top of the obstacle and size to fit.
	RaiseAreasWithInternalObstacles();

	// Create new areas as required
	CreateObstacleTopAreas();

	// It's possible for obstacle top areas to wind up overlapping one another, fix any such cases
	RemoveOverlappingObstacleTopAreas();
}

//--------------------------------------------------------------------------------------------------------------
/**
* For any nav area that has internal obstacles between its corners of greater than traversable height,
* raise that nav area to sit at the top of the obstacle, and shrink it to fit the obstacle.  Such nav
* areas are already restricted to be 1x1 so this will only be performed on areas that are already small.
*/
void CNavMesh::RaiseAreasWithInternalObstacles()
{
	// obstacle areas next to stairs are bad - delete them
	CUtlVector< CNavArea * > areasToDelete;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		// any nav area with internal obstacles will be 1x1 (width and height = GenerationStepSize), so
		// only need to consider areas of that size
		if ( ( area->GetSizeX() != GenerationStepSize ) || (area->GetSizeY() != GenerationStepSize ) )
			continue;

		float obstacleZ[2] = { -FLT_MAX, -FLT_MAX };
		float obstacleZMax = -FLT_MAX;
		NavDirType obstacleDir = NORTH;
		float obstacleStartDist = GenerationStepSize;
		float obstacleEndDist = 0;

		bool isStairNeighbor = false;

		// Look at all 4 directions and determine if there are obstacles in that direction.  Find the direction with the highest obstacle, if any.
		for ( int i = 0; i < NUM_DIRECTIONS; i++ )
		{
			NavDirType dir = (NavDirType) i;

			// For this direction, look at the left and right edges of the nav area relative to this direction and determined if they are both blocked
			// by obstacles.  We only consider this area obstructed if both edges are blocked (e.g. fence runs all the way through it).

			NavCornerType corner[2];
			int iEdgesBlocked = 0;
			corner[0] = (NavCornerType) ( ( i + 3 ) % NUM_CORNERS );					// lower left-hand corner relative to current direction
			corner[1] = (NavCornerType) ( ( i + 2 ) % NUM_CORNERS );				// lower right-hand corner relative to current direction			
			float obstacleZThisDir[2] = { -FLT_MAX, -FLT_MAX };						// absolute Z pos of obstacle for left and right edge in this direction
			float obstacleStartDistThisDir = GenerationStepSize;					// closest obstacle start distance in this direction
			float obstacleEndDistThisDir = 0;										// farthest obstacle end distance in this direction

			// consider left and right edges of nav area relative to current direction
			for ( int iEdge = 0; iEdge < 2; iEdge++ )
			{
				NavCornerType cornerType = corner[iEdge];
				CNavNode *nodeFrom = area->m_node[cornerType];
				if ( nodeFrom )
				{
					// is there an obstacle going from corner to corner along this edge?
					float obstacleHeight = nodeFrom->m_obstacleHeight[dir];
					if ( obstacleHeight > MaxTraversableHeight )
					{
						// yes, this edge is blocked
						iEdgesBlocked++;
						// keep track of obstacle height and start and end distance for this edge
						float obstacleZ = nodeFrom->GetPosition()->z + obstacleHeight;
						if ( obstacleZ > obstacleZThisDir[iEdge] )
						{							
							obstacleZThisDir[iEdge] = obstacleZ;
						}
						obstacleStartDistThisDir = MIN( nodeFrom->m_obstacleStartDist[dir], obstacleStartDistThisDir );
						obstacleEndDistThisDir = MAX( nodeFrom->m_obstacleEndDist[dir], obstacleEndDistThisDir );
					}
				}
			}

			int BlockedEdgeCutoff = 2;
			const NavConnectVector *connections = area->GetAdjacentAreas( dir );
			if ( connections )
			{
				for ( int conIndex=0; conIndex<connections->Count(); ++conIndex )
				{
					const CNavArea *connectedArea = connections->Element( conIndex ).area;
					if ( connectedArea && connectedArea->HasAttributes( NAV_MESH_STAIRS ) )
					{
						isStairNeighbor = true;
						BlockedEdgeCutoff = 1;	// one blocked edge is already too much when we're next to a stair
						break;
					}
				}
			}

			// are both edged blocked in this direction, and is the obstacle height in this direction the tallest we've seen?
			if ( (iEdgesBlocked >= BlockedEdgeCutoff ) && ( MAX( obstacleZThisDir[0], obstacleZThisDir[1] ) ) > obstacleZMax )
			{
				// this is the tallest obstacle we've encountered so far, remember its details
				obstacleZ[0] = obstacleZThisDir[0];
				obstacleZ[1] = obstacleZThisDir[1];
				obstacleZMax = MAX( obstacleZ[0], obstacleZ[1] );
				obstacleDir = dir;
				obstacleStartDist = obstacleStartDistThisDir;
				obstacleEndDist = obstacleStartDistThisDir;
			}
		}

		if ( isStairNeighbor && obstacleZMax > -FLT_MAX )
		{
			areasToDelete.AddToTail( area );
			continue;
		}

		// if we found an obstacle, raise this nav areas and size it to fit
		if ( obstacleZMax > -FLT_MAX )
		{
			// enforce minimum obstacle width so we don't shrink to become a teensy nav area
			AdjustObstacleDistances( &obstacleStartDist, &obstacleEndDist, GenerationStepSize );
			Assert( obstacleEndDist - obstacleStartDist >= MinObstacleAreaWidth );

			// get current corner coords
			Vector corner[4];
			for ( int i = NORTH_WEST; i < NUM_CORNERS; i++ )
			{
				corner[i] = area->GetCorner( (NavCornerType) i );
			}

			// adjust our size to fit the obstacle
			switch ( obstacleDir )
			{
			case NORTH:
				corner[NORTH_WEST].y = corner[SOUTH_WEST].y - obstacleEndDist;
				corner[NORTH_EAST].y = corner[SOUTH_EAST].y - obstacleEndDist;				
				corner[SOUTH_WEST].y -= obstacleStartDist;
				corner[SOUTH_EAST].y -= obstacleStartDist;				
				break;
			case SOUTH:
				corner[SOUTH_WEST].y = corner[NORTH_WEST].y + obstacleEndDist;
				corner[SOUTH_EAST].y = corner[NORTH_EAST].y + obstacleEndDist;
				corner[NORTH_WEST].y += obstacleStartDist;
				corner[NORTH_EAST].y += obstacleStartDist;
				::V_swap( obstacleZ[0], obstacleZ[1] );			// swap left and right Z heights for obstacle so we can run common code below
				break;
			case EAST:
				corner[NORTH_EAST].x = corner[NORTH_WEST].x + obstacleEndDist;
				corner[SOUTH_EAST].x = corner[SOUTH_WEST].x + obstacleEndDist;				
				corner[NORTH_WEST].x += obstacleStartDist;
				corner[SOUTH_WEST].x += obstacleStartDist;
			case WEST:
				corner[NORTH_WEST].x = corner[NORTH_EAST].x - obstacleEndDist;
				corner[SOUTH_WEST].x = corner[SOUTH_EAST].x - obstacleEndDist;				
				corner[NORTH_EAST].x -= obstacleStartDist;
				corner[SOUTH_EAST].x -= obstacleStartDist;
				::V_swap( obstacleZ[0], obstacleZ[1] );			// swap left and right Z heights for obstacle so we can run common code below
				break;
			}

			// adjust Z positions to be z pos of obstacle top
			corner[NORTH_WEST].z = obstacleZ[0];
			corner[NORTH_EAST].z = obstacleZ[1];
			corner[SOUTH_EAST].z = obstacleZ[1];
			corner[SOUTH_WEST].z = obstacleZ[0];
			
			// move the area
			RemoveNavArea( area );
			area->Build( corner[NORTH_WEST], corner[NORTH_EAST], corner[SOUTH_EAST], corner[SOUTH_WEST] );
			Assert( !area->IsDegenerate() );
			AddNavArea( area );

			// remove side-to-side connections if there are any so AI does try to do things like run along fencetops
			area->RemoveOrthogonalConnections( obstacleDir );
			area->SetAttributes( area->GetAttributes() | NAV_MESH_NO_MERGE | NAV_MESH_OBSTACLE_TOP );
			area->SetAttributes( area->GetAttributes() & ( ~NAV_MESH_JUMP ) );
			// clear out the nodes associated with this area's corners -- corners don't match the node positions any more
			for ( int i = 0; i < NUM_CORNERS; i++ )
			{
				area->m_node[i] = NULL;		
			}
		}
	}

	for ( int i=0; i<areasToDelete.Count(); ++i )
	{
		TheNavAreas.FindAndRemove( areasToDelete[i] );
		DestroyArea( areasToDelete[i] );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
* For any two nav areas that have an obstacle between them such as a fence, railing or small wall, creates a new 
* nav area on top of the obstacle and connects it between the areas
*/
void CNavMesh::CreateObstacleTopAreas()
{
	// enumerate all areas
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		// if this is a jump node (which will ultimately get removed) or is an obstacle top, ignore it
		if ( area->GetAttributes() & ( NAV_MESH_JUMP | NAV_MESH_OBSTACLE_TOP ) )
			return;

		// Look in all directions
		for ( int i = NORTH; i < NUM_DIRECTIONS; i++ )
		{
			NavDirType dir = (NavDirType) i;

			// Look at all adjacent areas in this direction
			int iConnections = area->GetAdjacentCount( dir );
			for ( int j = 0; j < iConnections; j++ )
			{
				CNavArea *areaOther = area->GetAdjacentArea( dir, j );
				// if this is a jump node (which will ultimately get removed) or is an obstacle top, ignore it
				if ( areaOther->GetAttributes() & ( NAV_MESH_JUMP | NAV_MESH_OBSTACLE_TOP ) )
					continue;

				// create an obstacle top if there is a one-node separation between the areas and there is an intra-node obstacle within that separation
				if ( !CreateObstacleTopAreaIfNecessary( area, areaOther, dir, false ) )
				{
					// if not, create an obstacle top if there is a two-node separation between the areas and the intermediate node is significantly
					// higher than the two areas, which means there's some geometry there that causes the middle node to be higher
					CreateObstacleTopAreaIfNecessary( area, areaOther, dir, true );
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
* Creates a new nav area if an obstacle exists between the two nav areas.  If bMultiNode is false, this checks
* if there's a one-node separation between the areas, and if so if there is an obstacle detected between the nodes.
* If bMultiNode is true, checks if there is a two-node separation between the areas, and if so if the middle node is
* higher than the two areas, suggesting an obstacle in the middle.
*/
bool CNavMesh::CreateObstacleTopAreaIfNecessary( CNavArea *area, CNavArea *areaOther, NavDirType dir, bool bMultiNode )
{
	float obstacleHeightMin = FLT_MAX;
	float obstacleHeightMax = 0; 
	float obstacleHeightStart = 0; 
	float obstacleHeightEnd = 0;
	float obstacleDistMin = GenerationStepSize;
	float obstacleDistMax = 0;

	Vector center;
	float halfPortalWidth;

	area->ComputePortal( areaOther, dir, &center, &halfPortalWidth );

	if ( halfPortalWidth > 0 )
	{
		// get the corners to left and right of direction toward other area
		NavCornerType cornerStart = (NavCornerType) dir;
		NavCornerType cornerEnd = (NavCornerType) ( ( dir + 1 ) % NUM_CORNERS );
		CNavNode *node = area->m_node[cornerStart];
		CNavNode *nodeEnd = area->m_node[cornerEnd];
		NavDirType dirEdge = (NavDirType) ( ( dir + 1 ) % NUM_DIRECTIONS );
		obstacleHeightMin = FLT_MAX;
		float zStart = 0, zEnd = 0;
		// along the edge of this area that faces the other area, look at every node that's in the portal between the two
		while ( node )
		{
			Vector vecToPortalCenter = *node->GetPosition() - center;
			vecToPortalCenter.z = 0;
			if ( vecToPortalCenter.IsLengthLessThan( halfPortalWidth + 1.0f ) )
			{
				// this node is in the portal 

				float obstacleHeight = 0;
				float obstacleDistStartCur = node->m_obstacleStartDist[dir];
				float obstacleDistEndCur = node->m_obstacleEndDist[dir];
				
				if ( !bMultiNode )
				{
					// use the inter-node obstacle height from this node toward the next area
					obstacleHeight = node->m_obstacleHeight[dir];
				}
				else
				{
					if ( !areaOther->Contains( *node->GetPosition() ) )
					{
						// step one node toward the other area
						CNavNode *nodeTowardOtherArea = node->GetConnectedNode( dir );
						if ( nodeTowardOtherArea )
						{
							// see if that step took us upward a significant amount
							float deltaZ = nodeTowardOtherArea->GetPosition()->z - node->GetPosition()->z;
							if ( deltaZ > MaxTraversableHeight )
							{
								// see if we've arrived in the other area
								bool bInOtherArea = false;
								if ( areaOther->Contains( *nodeTowardOtherArea->GetPosition() ) )
								{
									float z = areaOther->GetZ( nodeTowardOtherArea->GetPosition()->x, nodeTowardOtherArea->GetPosition()->y );
									float deltaZ = fabs( nodeTowardOtherArea->GetPosition()->z - z );
									if ( deltaZ < 2.0f )
									{
										bInOtherArea = true;
									}
								}

								// if we have not arrived in the other area yet, take one more step in the same direction
								if ( !bInOtherArea )
								{
									CNavNode *nodeTowardOtherArea2 = nodeTowardOtherArea->GetConnectedNode( dir );
									if ( nodeTowardOtherArea2 && areaOther->Contains( *nodeTowardOtherArea2->GetPosition() ) )
									{										
										float areaDeltaZ = node->GetPosition()->z - nodeTowardOtherArea2->GetPosition()->z;
										if ( fabs( areaDeltaZ ) <= MaxTraversableHeight )
										{
											// if we arrived in the other area, the obstacle height to get here was the peak deltaZ of the node above to get here
											obstacleHeight = deltaZ;
											// make a nav area MinObstacleAreaWidth wide centered on the peak node, which is GenerationStepSize away from where we started
											obstacleDistStartCur = GenerationStepSize - (MinObstacleAreaWidth / 2);
											obstacleDistEndCur = GenerationStepSize + (MinObstacleAreaWidth / 2);
										}
									}
								}
							}
						}							
					}
				}

				obstacleHeightMin = MIN( obstacleHeight, obstacleHeightMin );
				obstacleHeightMax = MAX( obstacleHeight, obstacleHeightMax );							
				obstacleDistMin = MIN( obstacleDistStartCur, obstacleDistMin );
				obstacleDistMax = MAX( obstacleDistEndCur, obstacleDistMax );

				if ( obstacleHeightStart == 0 )
				{
					// keep track of the obstacle height and node z pos at the start of the edge
					obstacleHeightStart = obstacleHeight;
					zStart = node->GetPosition()->z;
				}
				// keep track of the obstacle height and node z pos at the end of the edge
				obstacleHeightEnd = obstacleHeight;
				zEnd = node->GetPosition()->z;

			}
			if ( node == nodeEnd )
				break;

			node = node->GetConnectedNode( dirEdge );
		}




		// if we had some obstacle height from EVERY node along the portal, then getting from this area to the other requires scaling an obstacle,
		// need to generate a nav area on top of it
		if ( ( obstacleHeightMax > MaxTraversableHeight ) && ( obstacleHeightMin > MaxTraversableHeight ) )		
		{
			// If the maximum obstacle height was greater than both the height at start and end of the edge, then the obstacle is highest somewhere
			// in the middle.  Use that as the height of both ends.
			if ( ( obstacleHeightMax > obstacleHeightStart ) && ( obstacleHeightMax > obstacleHeightEnd ) )
			{
				obstacleHeightStart = obstacleHeightMax;
				obstacleHeightEnd = obstacleHeightMax;
			}

			// for south and west, swap "start" and "end" values of edges so we can use common code below
			if ( dir == SOUTH || dir == WEST )
			{
				::V_swap( obstacleHeightStart, obstacleHeightEnd );
				::V_swap( zStart, zEnd );
			}					

			// Enforce min area width for new area
			AdjustObstacleDistances( &obstacleDistMin, &obstacleDistMax, bMultiNode ? GenerationStepSize * 2 : GenerationStepSize );
			Assert( obstacleDistMin < obstacleDistMax );
			Assert( obstacleDistMax - obstacleDistMin >= MinObstacleAreaWidth );
			float newAreaWidth = obstacleDistMax - obstacleDistMin;
			Assert( newAreaWidth > 0 );			

			// Calculate new area coordinates
			AddDirectionVector( &center, dir, obstacleDistMin + (newAreaWidth/2) );

			Vector cornerNW, cornerNE, cornerSE, cornerSW;
			switch ( dir )
			{
			case NORTH:
			case SOUTH:
				cornerNW.Init( center.x - halfPortalWidth, center.y - (newAreaWidth/2), zStart + obstacleHeightStart );
				cornerNE.Init( center.x + halfPortalWidth, center.y - (newAreaWidth/2), zEnd + obstacleHeightEnd );
				cornerSE.Init( center.x + halfPortalWidth, center.y + (newAreaWidth/2), zEnd + obstacleHeightEnd );
				cornerSW.Init( center.x - halfPortalWidth, center.y + (newAreaWidth/2), zStart + obstacleHeightStart );
				break;
			case EAST:
			case WEST:
				cornerNW.Init( center.x - (newAreaWidth/2), center.y - halfPortalWidth, zStart + obstacleHeightStart );
				cornerNE.Init( center.x + (newAreaWidth/2), center.y - halfPortalWidth, zEnd + obstacleHeightEnd );
				cornerSE.Init( center.x + (newAreaWidth/2), center.y + halfPortalWidth, zEnd + obstacleHeightEnd );
				cornerSW.Init( center.x - (newAreaWidth/2), center.y + halfPortalWidth, zStart + obstacleHeightStart );
				break;
			}

			CNavArea *areaNew = CreateArea();
			areaNew->Build( cornerNW, cornerNE, cornerSE, cornerSW );

			// add it to the nav area list
			TheNavAreas.AddToTail( areaNew );
			AddNavArea( areaNew );

			Assert( !areaNew->IsDegenerate() );

			Msg( "Created new fencetop area %d(%x) between %d(%x) and %d(%x)\n", areaNew->GetID(), areaNew->GetDebugID(), area->GetID(), area->GetDebugID(), areaOther->GetID(), areaOther->GetDebugID() );

			areaNew->SetAttributes( area->GetAttributes() );
			areaNew->SetAttributes( area->GetAttributes() | NAV_MESH_NO_MERGE | NAV_MESH_OBSTACLE_TOP );

			area->Disconnect( areaOther );							
			area->ConnectTo( areaNew, dir );			

			areaNew->ConnectTo( area, OppositeDirection( dir ) );
			areaNew->ConnectTo( areaOther, dir );							
			if ( areaOther->IsConnected( area, OppositeDirection( dir ) ) )
			{
				areaOther->Disconnect( area );								
				areaOther->ConnectTo( areaNew, OppositeDirection( dir ) );
			}					
//			AddToSelectedSet( areaNew );
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
* Remove any obstacle top areas which overlap.
*/
void CNavMesh::RemoveOverlappingObstacleTopAreas()
{
	// What we really want is the union of all obstacle top areas that get generated.  That would be hard to compute exactly,
	// so instead we'll just remove any that overlap.  The obstacle top areas don't have to be exact, we just need enough of
	// them so there is generally a path to get over any obstacle.

	// make a list of just the obstacle top areas to reduce the N of the N squared operation we're about to do
	CUtlVector<CNavArea *> vecObstacleTopAreas;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		if ( area->GetAttributes() & NAV_MESH_OBSTACLE_TOP ) 
		{
			vecObstacleTopAreas.AddToTail( area );
		}
	}

	// look at every pair of obstacle top areas
	CUtlVector<CNavArea *> vecAreasToRemove;
	FOR_EACH_VEC( vecObstacleTopAreas, it )
	{
		CNavArea *area = vecObstacleTopAreas[it];

		Vector normal, otherNormal;
		area->ComputeNormal( &normal );
		area->ComputeNormal( &otherNormal, true );

		// Remove any obstacle areas that are steep enough to be jump areas
		float lowestNormalZ = MIN( normal.z, otherNormal.z );
		if ( lowestNormalZ < nav_slope_limit.GetFloat() )
		{
			vecAreasToRemove.AddToTail( area );
		}

		for ( int it2 = it+1; it2 < vecObstacleTopAreas.Count(); it2++ )
		{
			CNavArea *areaOther = vecObstacleTopAreas[it2];
			if ( area->IsOverlapping( areaOther ) )
			{		
				if ( area->Contains( areaOther ) )
				{
					// if one entirely contains the other, mark the other for removal
					vecAreasToRemove.AddToTail( areaOther );					
				}
				else if ( areaOther->Contains( area ) )
				{
					// if one entirely contains the other, mark the other for removal
					vecAreasToRemove.AddToTail( area );
				}
				else
				{
					// if they overlap without one being a superset of the other, just remove the smaller area
					CNavArea *areaToRemove = ( area->GetSizeX() * area->GetSizeY() > areaOther->GetSizeX() * areaOther->GetSizeY() ? areaOther : area );
					vecAreasToRemove.AddToTail( areaToRemove );					
				}
			}
		}
	}

	// now go delete all the areas we want to remove
	while ( vecAreasToRemove.Count() > 0 )
	{
		CNavArea *areaToDelete = vecAreasToRemove[0];
		RemoveFromSelectedSet( areaToDelete );
		TheNavMesh->OnEditDestroyNotify( areaToDelete );
		TheNavAreas.FindAndRemove( areaToDelete );
		TheNavMesh->DestroyArea( areaToDelete );

		// remove duplicates so we don't double-delete
		while ( vecAreasToRemove.FindAndRemove( areaToDelete ) );
	}

}

static void CommandNavCheckStairs( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->MarkStairAreas();
}
static ConCommand nav_check_stairs( "nav_check_stairs", CommandNavCheckStairs, "Update the nav mesh STAIRS attribute", FCVAR_CHEAT );

//--------------------------------------------------------------------------------------------------------------
/**
 * Mark all areas that are on stairs.
 */
void CNavMesh::MarkStairAreas( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		area->TestStairs();
	}
}


//--------------------------------------------------------------------------------------------------------------
enum StairTestType
{
	STAIRS_NO,
	STAIRS_YES,
	STAIRS_MAYBE,
};


//--------------------------------------------------------------------------------------------------------
// Test if a line across a nav area could be part of a stairway
StairTestType IsStairs( const Vector &start, const Vector &end, StairTestType ret )
{
	if ( ret == STAIRS_NO )
		return ret;

	const float inc = 5.0f;

	// the minimum height change each step to be a step and not a slope
	const float minStepZ = inc * tan( acos( nav_slope_limit.GetFloat() ) );
	const float MinStairNormal = 0.97f; // we don't care about ramps, just actual flat steps

	float t;
	Vector pos, normal;
	float height, priorHeight;

	// walk the line, checking for step height discontinuities
	float length = start.AsVector2D().DistTo( end.AsVector2D() );

	trace_t trace;
	CTraceFilterNoNPCsOrPlayer filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT );
	Vector hullMins( -inc/2, -inc/2, 0 );
	Vector hullMaxs( inc/2, inc/2, 0 );
	hullMaxs.z = 1; // don't care about vertical clearance

	if ( fabs( start.x - end.x ) > fabs( start.y - end.y ) )
	{
		hullMins.x = -8;
		hullMaxs.x = 8;
	}
	else
	{
		hullMins.y = -8;
		hullMaxs.y = 8;
	}

	Vector traceOffset( 0, 0, VEC_DUCK_HULL_MAX.z );

	// total height change must exceed a single step to be stairs
	if ( abs( start.z - end.z ) > StepHeight )
	{
		// initialize the height delta
		UTIL_TraceHull( start + traceOffset, start - traceOffset, hullMins, hullMaxs, MASK_NPCSOLID, &filter, &trace );
		if ( trace.startsolid || trace.IsDispSurface() )
		{
			return STAIRS_NO;
		}
		priorHeight = trace.endpos.z;

		// Save a copy for debug overlays
		Vector prevGround = start;
		prevGround.z = priorHeight;

		float traceIncrement = inc / length;
		for( t = 0.0f; t <= 1.0f; t += traceIncrement )
		{
			pos = start + t * ( end - start );

			UTIL_TraceHull( pos + traceOffset, pos - traceOffset, hullMins, hullMaxs, MASK_NPCSOLID, &filter, &trace );
			if ( trace.startsolid || trace.IsDispSurface() )
			{
				return STAIRS_NO;
			}
			height = trace.endpos.z;
			normal = trace.plane.normal;

			// Save a copy for debug overlays
			Vector ground( pos );
			ground.z = height;
			//NDebugOverlay::Cross3D( ground, 3, 0, 0, 255, true, 100.0f );
			//NDebugOverlay::Box( ground, hullMins, hullMaxs, 0, 0, 255, 0.0f, 100.0f );

			if ( t == 0.0f && fabs( height - start.z ) > StepHeight )
			{
				// Discontinuity at start
				return STAIRS_NO;
			}

			if ( t == 1.0f && fabs( height - end.z ) > StepHeight )
			{
				// Discontinuity at end
				return STAIRS_NO;
			}

			if ( normal.z < MinStairNormal )
			{
				// too steep here
				return STAIRS_NO;
			}


			float deltaZ = abs( height - priorHeight );

			if ( deltaZ >= minStepZ && deltaZ <= StepHeight )
			{
				// found a step
				ret = STAIRS_YES;
			}
			else if ( deltaZ > StepHeight )
			{
				// too steep here
				//NDebugOverlay::Cross3D( ground, 5, 255, 0, 0, true, 10.0f );
				//NDebugOverlay::Cross3D( prevGround, 5, 0, 255, 0, true, 10.0f );
				return STAIRS_NO;
			}

			// Save a copy for debug overlays
			prevGround = pos;
			prevGround.z = height;

			priorHeight = height;
		}
	}

	return ret;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Test an area for being on stairs
 * NOTE: This assumes a globally constant "step height", 
 * and walkable surface normal, which really should be locomotor-specific.
 */
bool CNavArea::TestStairs( void )
{
	// clear STAIRS attribute
	SetAttributes( GetAttributes() & ~NAV_MESH_STAIRS );

	if ( GetSizeX() <= GenerationStepSize && GetSizeY() <= GenerationStepSize )
	{
		// Don't bother with stairs on small areas
		return false;
	}

	const float MatchingNormalDot = 0.95f;
	Vector firstNormal, secondNormal;
	ComputeNormal( &firstNormal );
	ComputeNormal( &secondNormal, true );
	if ( firstNormal.Dot( secondNormal ) < MatchingNormalDot )
	{
		// area corners aren't coplanar - no stairs
		return false;
	}

	// test center and edges north-to-south, and east-to-west
	StairTestType ret = STAIRS_MAYBE;
	Vector from, to;

	const float inset = 5.0f; // inset to keep the tests completely inside the nav area

	from = GetCorner( NORTH_WEST ) + Vector( inset, inset, 0 );
	to = GetCorner( NORTH_EAST ) + Vector( -inset, inset, 0 );
	ret = IsStairs( from, to, ret );

	from = GetCorner( SOUTH_WEST ) + Vector( inset, -inset, 0 );
	to = GetCorner( SOUTH_EAST ) + Vector( -inset, -inset, 0 );
	ret = IsStairs( from, to, ret );

	from = GetCorner( NORTH_WEST ) + Vector( inset, inset, 0 );
	to = GetCorner( SOUTH_WEST ) + Vector( inset, -inset, 0 );
	ret = IsStairs( from, to, ret );

	from = GetCorner( NORTH_EAST ) + Vector( -inset, inset, 0 );
	to = GetCorner( SOUTH_EAST ) + Vector( -inset, -inset, 0 );
	ret = IsStairs( from, to, ret );

	from = ( GetCorner( NORTH_WEST ) + GetCorner( NORTH_EAST ) ) / 2.0f + Vector( 0, inset, 0 );
	to = ( GetCorner( SOUTH_WEST ) + GetCorner( SOUTH_EAST ) ) / 2.0f + Vector( 0, -inset, 0 );
	ret = IsStairs( from, to, ret );

	from = ( GetCorner( NORTH_EAST ) + GetCorner( SOUTH_EAST ) ) / 2.0f + Vector( -inset, 0, 0 );
	to = ( GetCorner( NORTH_WEST ) + GetCorner( SOUTH_WEST ) ) / 2.0f + Vector( inset, 0, 0 );
	ret = IsStairs( from, to, ret );

	if ( ret == STAIRS_YES )
	{
		SetAttributes( NAV_MESH_STAIRS );
		return true;
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_test_stairs, "Test the selected set for being on stairs", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	int count = 0;

	const NavAreaVector &selectedSet = TheNavMesh->GetSelectedSet();
	for ( int i=0; i<selectedSet.Count(); ++i )
	{
		CNavArea *area = selectedSet[i];
		if ( area->TestStairs() )
		{
			++count;
		}
	}

	Msg( "Marked %d areas as stairs\n", count );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Jump areas aren't used by the NextBot.  Delete them, connecting adjacent areas.
 */
void CNavMesh::RemoveJumpAreas( void )
{
	if ( !nav_generate_fixup_jump_areas.GetBool() )
	{
		return;
	}

	CUtlVector< CNavArea * > unusedAreas;

	int i;
	for ( i=0; i<TheNavAreas.Count(); ++i )
	{
		CNavArea *testArea = TheNavAreas[i];
		if ( !(testArea->GetAttributes() & NAV_MESH_JUMP) )
		{
			continue;
		}

		unusedAreas.AddToTail( testArea );
	}

	for ( i=0; i<unusedAreas.Count(); ++i )
	{
		CNavArea *areaToDelete = unusedAreas[i];
		TheNavMesh->OnEditDestroyNotify( areaToDelete );
		TheNavAreas.FindAndRemove( areaToDelete );
		TheNavMesh->DestroyArea( areaToDelete );
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavRemoveJumpAreas( void )
{
	JumpConnector connector;
	ForAllAreas( connector );

	int before = TheNavAreas.Count();
	RemoveJumpAreas();
	int after = TheNavAreas.Count();

	Msg( "Removed %d jump areas\n", before - after );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Recursively chop area in half along X until child areas are roughly square
 */
static void splitX( CNavArea *area )
{
	if (area->IsRoughlySquare())
		return;

	float split = area->GetSizeX();
	split /= 2.0f;
	split += area->GetCorner( NORTH_WEST ).x;

	split = TheNavMesh->SnapToGrid( split );

	const float epsilon = 0.1f;
	if (fabs(split - area->GetCorner( NORTH_WEST ).x) < epsilon ||
		fabs(split - area->GetCorner( SOUTH_EAST ).x) < epsilon)
	{
		// too small to subdivide
		return;
	}

	CNavArea *alpha, *beta;
	if (area->SplitEdit( false, split, &alpha, &beta ))
	{
		// split each new area until square
		splitX( alpha );
		splitX( beta );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Recursively chop area in half along Y until child areas are roughly square
 */
static void splitY( CNavArea *area )
{
	if (area->IsRoughlySquare())
		return;

	float split = area->GetSizeY();
	split /= 2.0f;
	split += area->GetCorner( NORTH_WEST ).y;

	split = TheNavMesh->SnapToGrid( split );

	const float epsilon = 0.1f;
	if (fabs(split - area->GetCorner( NORTH_WEST ).y) < epsilon ||
		fabs(split - area->GetCorner( SOUTH_EAST ).y) < epsilon)
	{
		// too small to subdivide
		return;
	}

	CNavArea *alpha, *beta;
	if (area->SplitEdit( true, split, &alpha, &beta ))
	{
		// split each new area until square
		splitY( alpha );
		splitY( beta );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Split any long, thin, areas into roughly square chunks.
 */
void CNavMesh::SquareUpAreas( void )
{
	int it = 0;

	while( it < TheNavAreas.Count() )
	{
		CNavArea *area = TheNavAreas[ it ];

		// move the iterator in case the current area is split and deleted
		++it;

		if (area->HasNodes() && !area->IsRoughlySquare())
		{
			// chop this area into square pieces
			if (area->GetSizeX() > area->GetSizeY())
				splitX( area );
			else
				splitY( area );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
static bool testStitchConnection( CNavArea *source, CNavArea *target, const Vector &sourcePos, const Vector &targetPos )
{
	trace_t result;
	Vector from( sourcePos );
	Vector pos( targetPos );
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
	Vector to, toNormal;
	bool success = false;
	if ( TraceAdjacentNode( 0, from, pos, &result ) )
	{
		to = result.endpos;
		toNormal = result.plane.normal;
		success = true;
	}
	else
	{
		// test going up ClimbUpHeight
		bool success = false;
		for ( float height = StepHeight; height <= ClimbUpHeight; height += 1.0f )
		{
			trace_t tr;
			Vector start( from );
			Vector end( pos );
			start.z += height;
			end.z += height;
			UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), &filter, &tr );
			if ( !tr.startsolid && tr.fraction == 1.0f )
			{
				if ( !StayOnFloor( &tr ) )
				{
					break;
				}

				to = tr.endpos;
				toNormal = tr.plane.normal;

				start = end = from;
				end.z += height;
				UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), &filter, &tr );
				if ( tr.fraction < 1.0f )
				{
					break;
				}

				success = true;
				break;
			}
		}
	}

	return success;
}




//--------------------------------------------------------------------------------------------------------
class IncrementallyGeneratedAreas
{
public:
	bool operator()( CNavArea *area )
	{
		return area->HasNodes();
	}
};


//--------------------------------------------------------------------------------------------------------
/**
 * Incremental generation fixup for where edges lap up against the existing nav mesh:
 * we have nodes, but the surrounding areas don't.  So, we trace outward, to see if we
 * can walk/fall to an adjacent area.  This handles dropping down into existing areas etc.
 * TODO: test pre-existing areas for drop-downs into the newly-generated areas.
 */
void CNavMesh::StitchGeneratedAreas( void )
{
	if ( m_generationMode == GENERATE_INCREMENTAL )
	{
		IncrementallyGeneratedAreas incrementalAreas;
		StitchMesh( incrementalAreas );
	}
}


//--------------------------------------------------------------------------------------------------------
class AreaSet
{
public:
	AreaSet( CUtlVector< CNavArea * > *areas )
	{
		m_areas = areas;
	}

	bool operator()( CNavArea *area )
	{
		return ( m_areas->HasElement( area ) );
	}

private:
	CUtlVector< CNavArea * > *m_areas;
};


//--------------------------------------------------------------------------------------------------------
/**
 * Stitches an arbitrary set of areas (newly-merged, for example) into the existing mesh
 */
void CNavMesh::StitchAreaSet( CUtlVector< CNavArea * > *areas )
{
	AreaSet areaSet( areas );
	StitchMesh( areaSet );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine if we can "jump down" from given point
 */
inline bool testJumpDown( const Vector *fromPos, const Vector *toPos )
{
	float dz = fromPos->z - toPos->z;

	// drop can't be too far, or too short (or nonexistant)
	if (dz <= JumpCrouchHeight || dz >= DeathDrop)
		return false;

	//
	// Check LOS out and down
	//
	// +-----+
	// |     |
	// F     |
	//       |
	//       T 
	//

	Vector from, to;
	float up;
	trace_t result;

	// Try to go up and out, up to ClimbUpHeight, to get over obstacles
	for ( up=1.0f; up<=ClimbUpHeight; up += 1.0f )
	{
		from = *fromPos;
		to.Init( fromPos->x, fromPos->y, fromPos->z + up );

		UTIL_TraceHull( from, to, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );
		if (result.fraction <= 0.0f || result.startsolid)
			continue;

		from.Init( fromPos->x, fromPos->y, result.endpos.z - 0.5f );
		to.Init( toPos->x, toPos->y, from.z );

		UTIL_TraceHull( from, to, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );
		if (result.fraction != 1.0f || result.startsolid)
			continue;

		// Success!
		break;
	}

	if ( up > ClimbUpHeight )
		return false;

	// We've made it up and out, so see if we can drop down
	from = to;
	to.z = toPos->z + 2.0f;
	UTIL_TraceHull( from, to, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );
	if (result.fraction <= 0.0f || result.startsolid)
		return false;

	// Allow a little fudge so we can drop down onto stairs
	if ( result.endpos.z > to.z + StepHeight )
		return false;

	return true;
}


//--------------------------------------------------------------------------------------------------------------
inline CNavArea *findJumpDownArea( const Vector *fromPos, NavDirType dir )
{
	if ( !nav_generate_jump_connections.GetBool() )
	{
		return NULL;
	}

	Vector start( fromPos->x, fromPos->y, fromPos->z + HalfHumanHeight );
	AddDirectionVector( &start, dir, GenerationStepSize/2.0f );

	Vector toPos;
	CNavArea *downArea = findFirstAreaInDirection( &start, dir, 4.0f * GenerationStepSize, DeathDrop, NULL, &toPos );

	if (downArea && testJumpDown( fromPos, &toPos ))
		return downArea;

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
template < typename Functor >
void CNavMesh::StitchAreaIntoMesh( CNavArea *area, NavDirType dir, Functor &func )
{
	Vector corner1, corner2;
	switch ( dir )
	{
		default:
			Assert(0);
		case NORTH:
			corner1 = area->GetCorner( NORTH_WEST );
			corner2 = area->GetCorner( NORTH_EAST );
			break;
		case SOUTH:
			corner1 = area->GetCorner( SOUTH_WEST );
			corner2 = area->GetCorner( SOUTH_EAST );
			break;
		case EAST:
			corner1 = area->GetCorner( NORTH_EAST );
			corner2 = area->GetCorner( SOUTH_EAST );
			break;
		case WEST:
			corner1 = area->GetCorner( NORTH_WEST );
			corner2 = area->GetCorner( SOUTH_WEST );
			break;
	}
	
	Vector edgeDir = corner2 - corner1;
	edgeDir.z = 0.0f;
	
	float edgeLength = edgeDir.NormalizeInPlace();
	
	for ( float n=0; n<edgeLength - 1.0f; n += GenerationStepSize )
	{
		Vector sourcePos = corner1 + edgeDir * ( n + 0.5f );
		sourcePos.z += HalfHumanHeight;
		
		Vector targetPos = sourcePos;
		switch ( dir )
		{
			case NORTH:	targetPos.y -= GenerationStepSize * 0.5f; break;
			case SOUTH:	targetPos.y += GenerationStepSize * 0.5f; break;
			case EAST:	targetPos.x += GenerationStepSize * 0.5f; break;
			case WEST:	targetPos.x -= GenerationStepSize * 0.5f; break;
		}
		
		CNavArea *targetArea = TheNavMesh->GetNavArea( targetPos );
		if ( targetArea && !func( targetArea ) )
		{
			targetPos.z = targetArea->GetZ( targetPos.x, targetPos.y ) + HalfHumanHeight;
			
			// outgoing connection
			if ( testStitchConnection( area, targetArea, sourcePos, targetPos ) )
			{
				area->ConnectTo( targetArea, dir );
			}
			
			// incoming connection
			if ( testStitchConnection( targetArea, area, targetPos, sourcePos ) )
			{
				targetArea->ConnectTo( area, OppositeDirection( dir ) );
			}
		}
		else
		{
			sourcePos.z -= HalfHumanHeight;
			sourcePos.z += 1;
			CNavArea *downArea = findJumpDownArea( &sourcePos, dir );
			if ( downArea && downArea != area && !func( downArea ) )
			{
				area->ConnectTo( downArea, dir );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
* Checks to see if there is a cliff - a drop of at least CliffHeight - in specified direction.
*/
inline bool CheckCliff( const Vector *fromPos, NavDirType dir, bool bExhaustive = true )
{
	// cliffs are half-baked, not used by any existing AI, and create poorly behaved nav areas (ie: long, thin, strips) (MSB 8/7/09)
	return false;


	Vector toPos( fromPos->x, fromPos->y, fromPos->z );
	AddDirectionVector( &toPos, dir, GenerationStepSize );

	trace_t trace;
	// trace a step in specified direction and see where we'd find up
	if ( TraceAdjacentNode( 0, *fromPos, toPos, &trace, DeathDrop * 10 ) && !trace.allsolid && !trace.startsolid )
	{
		float deltaZ = fromPos->z - trace.endpos.z;
		// would we fall off a cliff?
		if ( deltaZ > CliffHeight )
			return true;

		// if not, special case for south and east.  South and east edges are not considered part of a nav area, so
		// we look ahead two steps for south and east.  This ensures that the n-1th row and column of nav nodes
		// on the south and east sides of a nav area reflect any cliffs on the nth row and column.

		// if we're looking to south or east, and the first node we found was approximately flat, and this is the top-level
		// call, recurse one level to check one more step in this direction
		if ( ( dir == SOUTH || dir == EAST ) && ( fabs( deltaZ ) < StepHeight ) && bExhaustive )
		{	
			return CheckCliff( &trace.endpos, dir, false ); 
		}
	}
	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Define connections between adjacent generated areas
 */
void CNavMesh::ConnectGeneratedAreas( void )
{
	Msg( "Connecting navigation areas...\n" );

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		// scan along edge nodes, stepping one node over into the next area
		// for now, only use bi-directional connections

		// north edge
		CNavNode *node;
		for( node = area->m_node[ NORTH_WEST ]; node != area->m_node[ NORTH_EAST ]; node = node->GetConnectedNode( EAST ) )
		{
			CNavNode *adj = node->GetConnectedNode( NORTH );

			if (adj && adj->GetArea() && adj->GetConnectedNode( SOUTH ) == node )
			{
				area->ConnectTo( adj->GetArea(), NORTH );
			}
			else
			{
				CNavArea *downArea = findJumpDownArea( node->GetPosition(), NORTH );
				if (downArea && downArea != area)
					area->ConnectTo( downArea, NORTH );
			}
		}

		// west edge
		for( node = area->m_node[ NORTH_WEST ]; node != area->m_node[ SOUTH_WEST ]; node = node->GetConnectedNode( SOUTH ) )
		{
			CNavNode *adj = node->GetConnectedNode( WEST );
			
			if (adj && adj->GetArea() && adj->GetConnectedNode( EAST ) == node )
			{
				area->ConnectTo( adj->GetArea(), WEST );
			}
			else
			{
				CNavArea *downArea = findJumpDownArea( node->GetPosition(), WEST );
				if (downArea && downArea != area)
					area->ConnectTo( downArea, WEST );
			}
		}

		// south edge - this edge's nodes are actually part of adjacent areas
		// move one node north, and scan west to east
		/// @todo This allows one-node-wide areas - do we want this?
		node = area->m_node[ SOUTH_WEST ];
		if ( node ) // pre-existing areas in incremental generates won't have nodes
		{
			node = node->GetConnectedNode( NORTH );
		}
		if (node)
		{
			CNavNode *end = area->m_node[ SOUTH_EAST ]->GetConnectedNode( NORTH );
			/// @todo Figure out why cs_backalley gets a NULL node in here...
			for( ; node && node != end; node = node->GetConnectedNode( EAST ) )
			{
				CNavNode *adj = node->GetConnectedNode( SOUTH );
				
				if (adj && adj->GetArea() && adj->GetConnectedNode( NORTH ) == node )
				{
					area->ConnectTo( adj->GetArea(), SOUTH );
				}
				else
				{
					CNavArea *downArea = findJumpDownArea( node->GetPosition(), SOUTH );
					if (downArea && downArea != area)
						area->ConnectTo( downArea, SOUTH );
				}
			}
		}

		// south edge part 2 - scan the actual south edge.  If the node is not part of an adjacent area, then it
		// really belongs to us.  This will happen if our area runs right up against a ledge.
		for( node = area->m_node[ SOUTH_WEST ]; node != area->m_node[ SOUTH_EAST ]; node = node->GetConnectedNode( EAST ) )
		{
			if ( node->GetArea() )
				continue;	// some other area owns this node, pay no attention to it

			CNavNode *adj = node->GetConnectedNode( SOUTH );

			if ( node->IsBlockedInAnyDirection() || (adj && adj->IsBlockedInAnyDirection()) )
				continue;	// The space around this node is blocked, so don't connect across it

			// Don't directly connect to adj's area, since it's already 1 cell removed from our area.
			// There was no area in between, presumably for good reason.  Only look for jump down links.
			if ( !adj || !adj->GetArea() )
			{
				CNavArea *downArea = findJumpDownArea( node->GetPosition(), SOUTH );
				if (downArea && downArea != area)
					area->ConnectTo( downArea, SOUTH );
			}
		}

		// east edge - this edge's nodes are actually part of adjacent areas
		node = area->m_node[ NORTH_EAST ];
		if ( node ) // pre-existing areas in incremental generates won't have nodes
		{
			node = node->GetConnectedNode( WEST );
		}
		if (node)
		{
			CNavNode *end = area->m_node[ SOUTH_EAST ]->GetConnectedNode( WEST );
			for( ; node && node != end; node = node->GetConnectedNode( SOUTH ) )
			{
				CNavNode *adj = node->GetConnectedNode( EAST );			

				if (adj && adj->GetArea() && adj->GetConnectedNode( WEST ) == node )
				{
					area->ConnectTo( adj->GetArea(), EAST );
				}
				else
				{
					CNavArea *downArea = findJumpDownArea( node->GetPosition(), EAST );
					if (downArea && downArea != area)
						area->ConnectTo( downArea, EAST );
				}
			}
		}

		// east edge part 2 - scan the actual east edge.  If the node is not part of an adjacent area, then it
		// really belongs to us.  This will happen if our area runs right up against a ledge.
		for( node = area->m_node[ NORTH_EAST ]; node != area->m_node[ SOUTH_EAST ]; node = node->GetConnectedNode( SOUTH ) )
		{
			if ( node->GetArea() )
				continue;	// some other area owns this node, pay no attention to it

			CNavNode *adj = node->GetConnectedNode( EAST );

			if ( node->IsBlockedInAnyDirection() || (adj && adj->IsBlockedInAnyDirection()) )
				continue;	// The space around this node is blocked, so don't connect across it

			// Don't directly connect to adj's area, since it's already 1 cell removed from our area.
			// There was no area in between, presumably for good reason.  Only look for jump down links.
			if ( !adj || !adj->GetArea() )
			{
				CNavArea *downArea = findJumpDownArea( node->GetPosition(), EAST );
				if (downArea && downArea != area)
					area->ConnectTo( downArea, EAST );
			}
		}
	}

	StitchGeneratedAreas();
}

//--------------------------------------------------------------------------------------------------------------
bool CNavArea::IsAbleToMergeWith( CNavArea *other ) const
{
	if ( !HasNodes() || ( GetAttributes() & NAV_MESH_NO_MERGE ) ) 
		return false;

	if ( !other->HasNodes() || ( other->GetAttributes() & NAV_MESH_NO_MERGE ) )
		return false;

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Merge areas together to make larger ones (must remain rectangular - convex).
 * Areas can only be merged if their attributes match.
 */
void CNavMesh::MergeGeneratedAreas( void )
{
	Msg( "Merging navigation areas...\n" );

	bool merged;

	do
	{
		merged = false;

		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];
			if ( !area->HasNodes() || ( area->GetAttributes() & NAV_MESH_NO_MERGE ) ) 
				continue;

			// north edge
			FOR_EACH_VEC( area->m_connect[ NORTH ], nit )
			{
				CNavArea *adjArea = area->m_connect[ NORTH ][ nit ].area;
				if ( !area->IsAbleToMergeWith( adjArea ) ) // pre-existing areas in incremental generates won't have nodes
					continue;

				if ( area->GetSizeY() + adjArea->GetSizeY() > GenerationStepSize * nav_area_max_size.GetInt() )
					continue;

				if (area->m_node[ NORTH_WEST ] == adjArea->m_node[ SOUTH_WEST ] &&
					area->m_node[ NORTH_EAST ] == adjArea->m_node[ SOUTH_EAST ] &&
					area->GetAttributes() == adjArea->GetAttributes() &&
					area->IsCoplanar( adjArea ))
				{
					// merge vertical
					area->m_node[ NORTH_WEST ] = adjArea->m_node[ NORTH_WEST ];
					area->m_node[ NORTH_EAST ] = adjArea->m_node[ NORTH_EAST ];

					merged = true;
					//CONSOLE_ECHO( "  Merged (north) areas #%d and #%d\n", area->m_id, adjArea->m_id );

					area->FinishMerge( adjArea );

					// restart scan - iterator is invalidated
					break;
				}
			}

			if (merged)
				break;

			// south edge
			FOR_EACH_VEC( area->m_connect[ SOUTH ], sit )
			{
				CNavArea *adjArea = area->m_connect[ SOUTH ][ sit ].area;
				if ( !area->IsAbleToMergeWith( adjArea ) ) // pre-existing areas in incremental generates won't have nodes
					continue;

				if ( area->GetSizeY() + adjArea->GetSizeY() > GenerationStepSize * nav_area_max_size.GetInt() )
					continue;

				if (adjArea->m_node[ NORTH_WEST ] == area->m_node[ SOUTH_WEST ] &&
					adjArea->m_node[ NORTH_EAST ] == area->m_node[ SOUTH_EAST ] &&
					area->GetAttributes() == adjArea->GetAttributes() &&
					area->IsCoplanar( adjArea ))
				{
					// merge vertical
					area->m_node[ SOUTH_WEST ] = adjArea->m_node[ SOUTH_WEST ];
					area->m_node[ SOUTH_EAST ] = adjArea->m_node[ SOUTH_EAST ];

					merged = true;
					//CONSOLE_ECHO( "  Merged (south) areas #%d and #%d\n", area->m_id, adjArea->m_id );

					area->FinishMerge( adjArea );

					// restart scan - iterator is invalidated
					break;
				}

			}

			if (merged)
				break;


			// west edge
			FOR_EACH_VEC( area->m_connect[ WEST ], wit )
			{
				CNavArea *adjArea = area->m_connect[ WEST ][ wit ].area;
				if ( !area->IsAbleToMergeWith( adjArea ) ) // pre-existing areas in incremental generates won't have nodes
					continue;

				if ( area->GetSizeX() + adjArea->GetSizeX() > GenerationStepSize * nav_area_max_size.GetInt() )
					continue;

				if (area->m_node[ NORTH_WEST ] == adjArea->m_node[ NORTH_EAST ] &&
						area->m_node[ SOUTH_WEST ] == adjArea->m_node[ SOUTH_EAST ] &&
						area->GetAttributes() == adjArea->GetAttributes() &&
						area->IsCoplanar( adjArea ))
				{
					// merge horizontal
					area->m_node[ NORTH_WEST ] = adjArea->m_node[ NORTH_WEST ];
					area->m_node[ SOUTH_WEST ] = adjArea->m_node[ SOUTH_WEST ];

					merged = true;
					//CONSOLE_ECHO( "  Merged (west) areas #%d and #%d\n", area->m_id, adjArea->m_id );

					area->FinishMerge( adjArea );

					// restart scan - iterator is invalidated
					break;
				}

			}

			if (merged)
				break;

			// east edge
			FOR_EACH_VEC( area->m_connect[ EAST ], eit )
			{
				CNavArea *adjArea = area->m_connect[ EAST ][ eit ].area;
				if ( !area->IsAbleToMergeWith( adjArea ) ) // pre-existing areas in incremental generates won't have nodes
					continue;

				if ( area->GetSizeX() + adjArea->GetSizeX() > GenerationStepSize * nav_area_max_size.GetInt() )
					continue;

				if (adjArea->m_node[ NORTH_WEST ] == area->m_node[ NORTH_EAST ] &&
					adjArea->m_node[ SOUTH_WEST ] == area->m_node[ SOUTH_EAST ] &&
					area->GetAttributes() == adjArea->GetAttributes() &&
					area->IsCoplanar( adjArea ))
				{
					// merge horizontal
					area->m_node[ NORTH_EAST ] = adjArea->m_node[ NORTH_EAST ];
					area->m_node[ SOUTH_EAST ] = adjArea->m_node[ SOUTH_EAST ];

					merged = true;
					//CONSOLE_ECHO( "  Merged (east) areas #%d and #%d\n", area->m_id, adjArea->m_id );

					area->FinishMerge( adjArea );

					// restart scan - iterator is invalidated
					break;
				}
			}

			if (merged)
				break;
		}
	}
	while( merged );
}

//--------------------------------------------------------------------------------------------------------------
/**
* Given arbitrary corners of a compass grid-aligned rectangle, classify them by compass direction.
* Input: vec[4]: arbitrary corners
* Output: vecNW, vecNE, vecSE, vecSW: filled in with which corner is in which compass direction
*/
void ClassifyCorners( Vector vec[4], Vector &vecNW, Vector &vecNE, Vector &vecSE, Vector &vecSW )
{
	vecNW = vecNE = vecSE = vecSW = vec[0];

	for ( int i = 0; i < 4; i++ )
	{
		if ( ( vec[i].x <= vecNW.x ) && ( vec[i].y <= vecNW.y ) )
		{
			vecNW = vec[i];
		}
		if ( ( vec[i].x >= vecNE.x ) && ( vec[i].y <= vecNE.y ) )
		{
			vecNE = vec[i];
		}
		if ( ( vec[i].x >= vecSE.x ) && ( vec[i].y >= vecSE.y ) )
		{
			vecSE = vec[i];
		}
		if ( ( vec[i].x <= vecSW.x ) && ( vec[i].y >= vecSW.y ) )
		{
			vecSW = vec[i];
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
* Perform miscellaneous fixups to generated mesh
*/
void CNavMesh::FixUpGeneratedAreas( void )
{
	FixCornerOnCornerAreas();
	FixConnections();
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::FixConnections( void )
{
	// Test the steep sides of stairs for any outgoing links that cross nodes that were partially obstructed.
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		if ( !area->HasAttributes( NAV_MESH_STAIRS ) )
			continue;

		if ( !area->HasNodes() )
			continue;

		for ( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			NavCornerType cornerType[2];
			GetCornerTypesInDirection( (NavDirType)dir, &cornerType[0], &cornerType[1] );

			// Flat edges of stairs need to connect.  It's the slopes we don't want to climb over things for.
			float cornerDeltaZ = fabs( area->GetCorner( cornerType[0] ).z - area->GetCorner( cornerType[1] ).z );
			if ( cornerDeltaZ < StepHeight )
				continue;

			const NavConnectVector *connectedAreas = area->GetAdjacentAreas( (NavDirType)dir );
			CUtlVector< CNavArea * > areasToDisconnect;
			for ( int i=0; i<connectedAreas->Count(); ++i )
			{
				CNavArea *adjArea = connectedAreas->Element(i).area;
				if ( !adjArea->HasNodes() )
					continue;

				Vector pos, adjPos;
				float width;
				area->ComputePortal( adjArea, (NavDirType)dir, &pos, &width );
				adjArea->GetClosestPointOnArea( pos, &adjPos );

				CNavNode *node = area->FindClosestNode( pos, (NavDirType)dir );
				CNavNode *adjNode = adjArea->FindClosestNode( adjPos, OppositeDirection( (NavDirType)dir ) );
				pos = *node->GetPosition();
				adjPos = *adjNode->GetPosition();

				if ( !node || !adjNode )
					continue;

				NavCornerType adjCornerType[2];
				GetCornerTypesInDirection( OppositeDirection((NavDirType)dir), &adjCornerType[0], &adjCornerType[1] );

				// From the stair's perspective, we can't go up more than step height to reach the adjacent area.
				// Also, if the adjacent area has to jump up higher than StepHeight above the stair area to reach the stairs,
				// there's an obstruction close to the adjacent area that could prevent walking from the stairs down.
				if ( node->GetGroundHeightAboveNode( cornerType[0] ) > StepHeight )
				{
					areasToDisconnect.AddToTail( adjArea );
				}
				else if ( node->GetGroundHeightAboveNode( cornerType[1] ) > StepHeight )
				{
					areasToDisconnect.AddToTail( adjArea );
				}
				else if ( adjPos.z + adjNode->GetGroundHeightAboveNode( adjCornerType[0] ) > pos.z + StepHeight )
				{
					areasToDisconnect.AddToTail( adjArea );
				}
				else if ( adjPos.z + adjNode->GetGroundHeightAboveNode( adjCornerType[1] ) > pos.z + StepHeight )
				{
					areasToDisconnect.AddToTail( adjArea );
				}
			}

			for ( int i=0; i<areasToDisconnect.Count(); ++i )
			{
				area->Disconnect( areasToDisconnect[i] );
			}
		}
	}

	// Test to prevent A->C if A->B->C.  This can happen in doorways and dropdowns from rooftops.
	// @TODO: find the root cause of A->C links.
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		CUtlVector< CNavArea * > areasToDisconnect;
		for ( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			const NavConnectVector *connectedAreas = area->GetAdjacentAreas( (NavDirType)dir );
			for ( int i=0; i<connectedAreas->Count(); ++i )
			{
				CNavArea *adjArea = connectedAreas->Element(i).area;
				const NavConnectVector *adjConnectedAreas = adjArea->GetAdjacentAreas( (NavDirType)dir );
				for ( int j=0; j<adjConnectedAreas->Count(); ++j )
				{
					CNavArea *farArea = adjConnectedAreas->Element(j).area;

					if ( area->IsConnected( farArea, (NavDirType)dir ) )
					{
						areasToDisconnect.AddToTail( farArea );
					}
				}
			}
		}

		for ( int i=0; i<areasToDisconnect.Count(); ++i )
		{
			area->Disconnect( areasToDisconnect[i] );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
* Fix any spots where we there are nav nodes touching only corner-on-corner but we intend bots to be able to traverse
*/
void CNavMesh::FixCornerOnCornerAreas( void )
{
	const float MaxDrop = StepHeight;	// don't make corner on corner areas that are too steep

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		// determine if we have any corners where the only nav area we touch is diagonally corner-to-corner.
		// if there are, generate additional small (0.5 x 0.5 grid size) nav areas in the corners between
		// them if map geometry allows and make connections in cardinal compass directions to create a path 
		// between the two areas.

//
//              XXXXXXXXX                       XXXXXXXXX
//              X       X                       X       X 
//				X other X                   ****X other X
//              X       X                   *newX       X
//      XXXXXXXXXXXXXXXXX      =>       XXXXXXXXXXXXXXXXX       
//      X       X                       X       Xnew*
//      X area  X                       X area  X****
//		X       X                       X       X
//      XXXXXXXXX                       XXXXXXXXX
//

		// check each corner
		for ( int iCorner = NORTH_WEST; iCorner < NUM_CORNERS; iCorner++ )
		{
			// get cardinal direction to right and left of this corner
			NavDirType dirToRight = (NavDirType) iCorner;			
			NavDirType dirToLeft = (NavDirType) ( ( iCorner+3 ) % NUM_DIRECTIONS );
					
			// if we have any connections on cardinal compass directions on edge on either side of corner we're OK, skip this nav area
			if ( area->GetAdjacentCount( dirToLeft ) > 0 || area->GetAdjacentCount( dirToRight ) > 0 ||
				area->GetIncomingConnections( dirToLeft )->Count() > 0 || area->GetIncomingConnections( dirToRight )->Count() > 0 )
				continue;

			Vector cornerPos = area->GetCorner( (NavCornerType) iCorner );
			NavDirType dirToRightTwice = DirectionRight( dirToRight );
			NavDirType dirToLeftTwice = DirectionLeft( dirToLeft );
			NavDirType dirsAlongOtherEdge[2] = { dirToLeft, dirToRight };
			NavDirType dirsAlongOurEdge[2] = { dirToLeftTwice, dirToRightTwice };

			// consider 2 potential new nav areas, to left and right of the corner we're considering
			for ( int iDir = 0; iDir < ARRAYSIZE( dirsAlongOtherEdge ); iDir++ )
			{
				NavDirType dirAlongOtherEdge = dirsAlongOtherEdge[iDir];
				NavDirType dirAlongOurEdge = dirsAlongOurEdge[iDir];

				// look at the point 0.5 grid units along edge of other nav area
				Vector vecDeltaOtherEdge;
				DirectionToVector2D( dirAlongOtherEdge, (Vector2D *) &vecDeltaOtherEdge );
				vecDeltaOtherEdge.z = 0;
				vecDeltaOtherEdge *= GenerationStepSize * 0.5;
				Vector vecOtherEdgePos = cornerPos + vecDeltaOtherEdge;

				// see if there is a nav area at that location
				CNavArea *areaOther = GetNavArea( vecOtherEdgePos );
				Assert( areaOther != area );
				if ( !areaOther )
					continue;		// no other area in that location, we're not touching on corner

				// see if we can move from our corner in that direction
				trace_t result;
				if ( !TraceAdjacentNode( 0, cornerPos, vecOtherEdgePos, &result, MaxDrop ) )
					continue;		// something is blocking movement, don't create additional nodes to aid movement
				
				// get the corner of the other nav area that might touch our corner
				int iCornerOther = ( ( iCorner + 2 ) % NUM_CORNERS );						
				Vector cornerPosOther = areaOther->GetCorner( (NavCornerType) iCornerOther );

				if ( cornerPos != cornerPosOther )
					continue;		// that nav area does not touch us on corner
				
				// we are touching corner-to-corner with the other nav area and don't have connections in cardinal directions around
				// the corner that touches, this is a candidate to generate new small helper nav areas.

				// calculate the corners of the 0.5 x 0.5 nav area we would consider building between us and the other nav area whose corner we touch
				Vector vecDeltaOurEdge;
				DirectionToVector2D( dirAlongOurEdge, (Vector2D *) &vecDeltaOurEdge );
				vecDeltaOurEdge.z = 0;
				vecDeltaOurEdge *= GenerationStepSize * 0.5;
				Vector vecOurEdgePos = cornerPos + vecDeltaOurEdge;
				Vector vecCorner[4];					
				vecCorner[0] = cornerPos + vecDeltaOtherEdge + vecDeltaOurEdge;		// far corner of new nav area
				vecCorner[1] = cornerPos + vecDeltaOtherEdge;						// intersection of far edge of new nav area with other nav area we touch
				vecCorner[2] = cornerPos;											// common corner of this nav area, nav area we touch, and new nav area
				vecCorner[3] = cornerPos + vecDeltaOurEdge;							// intersection of far edge of new nav area with this nav area
			
				CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
				if ( !TraceAdjacentNode( 0, vecCorner[1], vecCorner[0], &result, MaxDrop ) ||	// can we move from edge of other area to far corner of new node
					!TraceAdjacentNode( 0, vecCorner[3], vecCorner[0], &result, MaxDrop ) )		// can we move from edge of this area to far corner of new node
					continue;	// new node would not fit

				// as sanity check, make sure there's not already a nav area there, shouldn't be
				CNavArea *areaTest = GetNavArea( vecCorner[0] );
				Assert ( !areaTest );
				if ( areaTest )
					continue;

				vecCorner[0] = result.endpos;

				// create a new nav area
				CNavArea *areaNew = CreateArea();

				// arrange the corners of the new nav area by compass direction
				Vector vecNW, vecNE, vecSE, vecSW;
				ClassifyCorners( vecCorner, vecNW, vecNE, vecSE, vecSW );
				areaNew->Build( vecNW, vecNE, vecSE, vecSW );

				// add it to the nav area list
				TheNavAreas.AddToTail( areaNew );
				AddNavArea( areaNew );
				
				areaNew->SetAttributes( area->GetAttributes() );

				// reciprocally connect between this area and new area
				area->ConnectTo( areaNew, dirAlongOtherEdge );
				areaNew->ConnectTo( area, OppositeDirection( dirAlongOtherEdge ) );

				// reciprocally connect between other area and new area
				areaOther->ConnectTo( areaNew, dirAlongOurEdge );
				areaNew->ConnectTo( areaOther, OppositeDirection( dirAlongOurEdge ) );
			}
		}	
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
* Fix any areas where one nav area overhangs another and the two nav areas are connected.  Subdivide the lower
* nav area such that the upper nav area doesn't overhang any area it's connected to.
*/
void CNavMesh::SplitAreasUnderOverhangs( void )
{
	// restart the whole process whenever this gets set to true
	bool bRestartProcessing = false;

	do 
	{
		bRestartProcessing = false;

		// iterate all nav areas
		for ( int it = 0; it < TheNavAreas.Count() && !bRestartProcessing; it++ )
		{
			CNavArea *area = TheNavAreas[ it ];
			Extent areaExtent;
			area->GetExtent( &areaExtent );

			// iterate all directions
			for ( int dir = NORTH; dir < NUM_DIRECTIONS && !bRestartProcessing; dir++ )
			{
				// iterate all connections in that direction
				const NavConnectVector *pConnections = area->GetAdjacentAreas( (NavDirType) dir );
				for ( int iConnection = 0; iConnection < pConnections->Count() && !bRestartProcessing; iConnection++ )
				{
					CNavArea *otherArea = (*pConnections)[iConnection].area;
					Extent otherAreaExtent;
					otherArea->GetExtent( &otherAreaExtent );

					// see if the area we are connected to overlaps our X/Y extents
					if ( area->IsOverlapping( otherArea ) )
					{
						// if the upper area isn't at least crouch height above the lower area, this is some weird minor
						// overlap, disregard it
						const float flMinSeparation = HumanCrouchHeight;
						if ( !( areaExtent.lo.z > otherAreaExtent.hi.z + flMinSeparation ) &&
							!( otherAreaExtent.lo.z > areaExtent.hi.z + flMinSeparation ) )
							continue;

						// figure out which area is above and which is below
						CNavArea *areaBelow = area, *areaAbove = otherArea;
						NavDirType dirFromAboveToBelow = OppositeDirection( (NavDirType) dir );
						if ( otherAreaExtent.lo.z < areaExtent.lo.z )
						{
							areaBelow = otherArea;
							areaAbove = area;
							dirFromAboveToBelow = OppositeDirection( dirFromAboveToBelow );
						}
						NavDirType dirFromBelowToAbove = OppositeDirection( dirFromAboveToBelow );

						// Msg( "area %d overhangs area %d and is connected\n", areaAbove->GetID(), areaBelow->GetID() );

						Extent extentBelow, extentAbove;
						areaBelow->GetExtent( &extentBelow );
						areaAbove->GetExtent( &extentAbove );
					
						float splitCoord;			// absolute world coordinate along which we will split lower nav area (X or Y, depending on axis we split on)
						float splitLen;				// length of the segment of lower nav area that is in shadow of the upper nav area
						float splitEdgeSize;		// current length of the edge of nav area that is getting split
						bool bSplitAlongX = false;

						// determine along what edge we are splitting and make some key measurements
						if ( ( dirFromAboveToBelow == EAST ) || ( dirFromAboveToBelow == WEST ) )
						{
							splitEdgeSize = extentBelow.hi.x - extentBelow.lo.x;
							if ( extentAbove.hi.x < extentBelow.hi.x )
							{
								splitCoord = extentAbove.hi.x;
								splitLen = splitCoord - extentBelow.lo.x;
							}
							else
							{
								splitCoord = extentAbove.lo.x;
								splitLen = extentBelow.hi.x - splitCoord;
							}							
						}
						else
						{
							splitEdgeSize = extentBelow.hi.y - extentBelow.lo.y;
							bSplitAlongX = true;
							if ( extentAbove.hi.y < extentBelow.hi.y )
							{
								splitCoord = extentAbove.hi.y;
								splitLen = splitCoord - extentBelow.lo.y;
							}
							else
							{
								splitCoord = extentAbove.lo.y;
								splitLen = extentBelow.hi.y - splitCoord;
							}
						}
						Assert( splitLen >= 0 );
						Assert( splitEdgeSize > 0 );

						// if we split the lower nav area right where it's in shadow of the upper nav area, will it create a really tiny strip?
						if ( splitLen < GenerationStepSize )
						{
							// if the "in shadow" part of the lower nav area is really small or the lower nav area is really small to begin with,
							// don't split it, we're better off as is
							if ( ( splitLen < GenerationStepSize*0.3 ) || ( splitEdgeSize <= GenerationStepSize * 2 ) )
								continue;

							// Move our split point so we don't create a really tiny strip on the lower nav area.  Move the split point away from
							// the upper nav area so the "in shadow" area expands to be GenerationStepSize.  The checks above ensure we have room to do this.
							float splitDelta = GenerationStepSize - splitLen;
							splitCoord += splitDelta * ( ( ( dirFromAboveToBelow == NORTH ) || ( dirFromAboveToBelow == WEST ) ) ? -1 : 1 );						
						}

						// remove any connections between the two areas (so they don't get inherited by the new areas when we split the lower area),
						// but remember what the connections were.
						bool bConnectionFromBelow = false, bConnectionFromAbove = false;
						if ( areaBelow->IsConnected( areaAbove, dirFromBelowToAbove ) )
						{
							bConnectionFromBelow = true;
							areaBelow->Disconnect( areaAbove );
						}
						if ( areaAbove->IsConnected( areaBelow, dirFromAboveToBelow ) )
						{
							bConnectionFromAbove = true;
							areaAbove->Disconnect( areaBelow );
						}						

						CNavArea *pNewAlpha = NULL,*pNewBeta = NULL;
//						int idBelow = areaBelow->GetID();
//						AddToSelectedSet( areaBelow );
						// split the lower nav area
						if ( areaBelow->SplitEdit( bSplitAlongX, splitCoord, &pNewAlpha, &pNewBeta ) )
						{
//							Msg( "Split area %d into %d and %d\n", idBelow, pNewAlpha->GetID(), pNewBeta->GetID() );

							// determine which of the two new lower areas is the one *not* in shadow of the upper nav area.  This is the one we want to
							// reconnect to
							CNavArea *pNewNonoverlappedArea = ( ( dirFromAboveToBelow == NORTH ) || ( dirFromAboveToBelow == WEST ) ) ? pNewAlpha : pNewBeta;

							// restore the previous connections from the upper nav area to the new lower nav area that is not in shadow of the upper
							if ( bConnectionFromAbove )
							{
								areaAbove->ConnectTo( pNewNonoverlappedArea, dirFromAboveToBelow );
							}
							if ( bConnectionFromBelow )
							{
								areaBelow->ConnectTo( pNewNonoverlappedArea, OppositeDirection( dirFromAboveToBelow ) );
							}

							// Now we need to just start the whole process over.  We've just perturbed the list we're iterating on (removed a nav area, added two 
							// new ones, when we did the split), and it's possible we may have to subdivide a lower nav area twice if the upper nav area
							// overhangs a corner of the lower area.  We just start all over again each time we do a split until no more overhangs occur.
							bRestartProcessing = true;
						}						
						else
						{
//							Msg( "Failed to split area %d\n", idBelow );
						}
					}
				}			
			}
		}
	} 
	while ( bRestartProcessing );
}


//--------------------------------------------------------------------------------------------------------------
bool TestForValidCrouchArea( CNavNode *node )
{
	// must make sure we don't have a bogus crouch area.  check up to JumpCrouchHeight above
	// the node for a HumanCrouchHeight space.

	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT, WALK_THRU_EVERYTHING );
	trace_t tr;
	Vector start( *node->GetPosition() );
	Vector end( *node->GetPosition() );
	end.z += JumpCrouchHeight;

	Vector mins( 0, 0, 0 );
	Vector maxs( GenerationStepSize, GenerationStepSize, HumanCrouchHeight );

	UTIL_TraceHull(
		start,
		end,
		mins,
		maxs,
		TheNavMesh->GetGenerationTraceMask(),
		&filter,
		&tr );

	return ( !tr.allsolid );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Make sure that if other* are similar, test is also close.  Used in TestForValidJumpArea.
 */
bool IsHeightDifferenceValid( float test, float other1, float other2, float other3 )
{
	// Make sure the other nodes are level.
	const float CloseDelta = StepHeight / 2;
	if ( fabs( other1 - other2 ) > CloseDelta )
		return true;

	if ( fabs( other1 - other3 ) > CloseDelta )
		return true;

	if ( fabs( other2 - other3 ) > CloseDelta )
		return true;

	// Now make sure the test node is near the others.  If it is more than StepHeight away,
	// it'll form a distorted jump area.
	const float MaxDelta = StepHeight;
	if ( fabs( test - other1 ) > MaxDelta )
		return false;

	if ( fabs( test - other2 ) > MaxDelta )
		return false;

	if ( fabs( test - other3 ) > MaxDelta )
		return false;

	return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check that a 1x1 area with 'node' at the northwest corner has a valid shape - if 3 corners
 * are flat, and the 4th is significantly higher or lower, it would form a jump area that bots
 * can't navigate over well.
 */
bool TestForValidJumpArea( CNavNode *node )
{
	return true;

	CNavNode *east = node->GetConnectedNode( EAST );
	CNavNode *south = node->GetConnectedNode( SOUTH );
	if ( !east || !south )
		return false;

	CNavNode *southEast = east->GetConnectedNode( SOUTH );
	if ( !southEast )
		return false;

	if ( !IsHeightDifferenceValid(
		node->GetPosition()->z,
		south->GetPosition()->z,
		southEast->GetPosition()->z,
		east->GetPosition()->z ) )
		return false;

	if ( !IsHeightDifferenceValid(
		south->GetPosition()->z,
		node->GetPosition()->z,
		southEast->GetPosition()->z,
		east->GetPosition()->z ) )
		return false;

	if ( !IsHeightDifferenceValid(
		southEast->GetPosition()->z,
		south->GetPosition()->z,
		node->GetPosition()->z,
		east->GetPosition()->z ) )
		return false;

	if ( !IsHeightDifferenceValid(
		east->GetPosition()->z,
		south->GetPosition()->z,
		southEast->GetPosition()->z,
		node->GetPosition()->z ) )
		return false;

	return true;
}


//--------------------------------------------------------------------------------------------------------------
class TestOverlapping
{
	Vector m_nw;
	Vector m_ne;
	Vector m_sw;
	Vector m_se;
public:
	TestOverlapping( const Vector &nw, const Vector &ne, const Vector &sw, const Vector &se ) :
	  m_nw( nw ), m_ne( ne ), m_sw( sw ), m_se( se )
	{
	}

	// This approximates CNavArea::GetZ, so we can pretend our four corners delineate a nav area
	float GetZ( const Vector &pos ) const
	{
		float dx = m_se.x - m_nw.x;
		float dy = m_se.y - m_nw.y;

		// guard against division by zero due to degenerate areas
		if (dx == 0.0f || dy == 0.0f)
			return m_ne.z;

		float u = (pos.x - m_nw.x) / dx;
		float v = (pos.y - m_nw.y) / dy;

		// clamp Z values to (x,y) volume
		if (u < 0.0f)
			u = 0.0f;
		else if (u > 1.0f)
			u = 1.0f;

		if (v < 0.0f)
			v = 0.0f;
		else if (v > 1.0f)
			v = 1.0f;

		float northZ = m_nw.z + u * (m_ne.z - m_nw.z);
		float southZ = m_sw.z + u * (m_se.z - m_sw.z);

		return northZ + v * (southZ - northZ);
	}

	bool OverlapsExistingArea( void )
	{
		CNavArea *overlappingArea = NULL;
		CNavLadder *overlappingLadder = NULL;

		Vector nw = m_nw;
		Vector se = m_se;
		Vector start = nw;
		start.x += GenerationStepSize/2;
		start.y += GenerationStepSize/2;

		while ( start.x < se.x )
		{
			start.y = nw.y + GenerationStepSize/2;
			while ( start.y < se.y )
			{
				start.z = GetZ( start );
				Vector end = start;
				start.z -= StepHeight;
				end.z += HalfHumanHeight;

				if ( TheNavMesh->FindNavAreaOrLadderAlongRay( start, end, &overlappingArea, &overlappingLadder, NULL ) )
				{
					if ( overlappingArea )
					{
						return true;
					}
				}

				start.y += GenerationStepSize;
			}
			start.x += GenerationStepSize;
		}
		return false;
	}
};


//--------------------------------------------------------------------------------------------------------------
/** 
 * Check if an rectangular area of the given size can be
 * made starting from the given node as the NW corner.
 * Only consider fully connected nodes for this check.
 * All of the nodes within the test area must have the same attributes.
 * All of the nodes must be approximately co-planar w.r.t the NW node's normal, with the
 * exception of 1x1 areas which can be any angle.
 */
bool CNavMesh::TestArea( CNavNode *node, int width, int height )
{
	Vector normal = *node->GetNormal();
	float d = -DotProduct( normal, *node->GetPosition() );

	bool nodeCrouch = node->m_crouch[ SOUTH_EAST ];

	// The area's interior will be the south-east side of this north-west node.
	// If that interior space is blocked, there's no space to build an area.
	if ( node->m_isBlocked[ SOUTH_EAST ] )
	{
		return false;
	}

	int nodeAttributes = node->GetAttributes() & ~NAV_MESH_CROUCH;

	const float offPlaneTolerance = 5.0f;

	CNavNode *vertNode, *horizNode;

	vertNode = node;
	int x,y;
	for( y=0; y<height; y++ )
	{
		horizNode = vertNode;

		for( x=0; x<width; x++ )
		{
			//
			// Compute the crouch attributes for the test node, taking into account only the side(s) of the node
			// that are in the area

			// NOTE: The nodes on the south and east borders of an area aren't contained in the area.  This means that
			// crouch attributes and blocked state need to be checked to the south and east of the southEdge and eastEdge nodes.

			bool horizNodeCrouch = false;
			bool westEdge = (x == 0);
			bool eastEdge = (x == width - 1);
			bool northEdge = (y == 0);
			bool southEdge = (y == height - 1);

			// Check corners first
			if ( northEdge && westEdge )
			{
				// The area's interior will be the south-east side of this north-west node.
				// If that interior space is blocked, there's no space to build an area.
				horizNodeCrouch = horizNode->m_crouch[ SOUTH_EAST ];
				if ( horizNode->m_isBlocked[ SOUTH_EAST ] )
				{
					return false;
				}
			}
			else if ( northEdge && eastEdge )
			{
				// interior space of the area extends one more cell to the east past the easternmost nodes.
				// This means we need to check to the southeast as well as the southwest.
				horizNodeCrouch = horizNode->m_crouch[ SOUTH_EAST ] || horizNode->m_crouch[ SOUTH_WEST ];
				if ( horizNode->m_isBlocked[ SOUTH_EAST ] || horizNode->m_isBlocked[ SOUTH_WEST ] )
				{
					return false;
				}
			}
			else if ( southEdge && westEdge )
			{
				// The interior space of the area extends one more cell to the south past the southernmost nodes.
				// This means we need to check to the southeast as well as the southwest.
				horizNodeCrouch = horizNode->m_crouch[ SOUTH_EAST ] || horizNode->m_crouch[ NORTH_EAST ];
				if ( horizNode->m_isBlocked[ SOUTH_EAST ] || horizNode->m_isBlocked[ NORTH_EAST ] )
				{
					return false;
				}
			}
			else if ( southEdge && eastEdge )
			{
				// This node is completely in the interior of the area, so we need to check in all directions.
				horizNodeCrouch = (horizNode->GetAttributes() & NAV_MESH_CROUCH) != 0;
				if ( horizNode->IsBlockedInAnyDirection() )
				{
					return false;
				}
			}
			// check sides next
			else if ( northEdge )
			{
				horizNodeCrouch = horizNode->m_crouch[ SOUTH_EAST ] || horizNode->m_crouch[ SOUTH_WEST ];
				if ( horizNode->m_isBlocked[ SOUTH_EAST ] || horizNode->m_isBlocked[ SOUTH_WEST ] )
				{
					return false;
				}
			}
			else if ( southEdge )
			{
				// This node is completely in the interior of the area, so we need to check in all directions.
				horizNodeCrouch = (horizNode->GetAttributes() & NAV_MESH_CROUCH) != 0;
				if ( horizNode->IsBlockedInAnyDirection() )
				{
					return false;
				}
			}
			else if ( eastEdge )
			{
				// This node is completely in the interior of the area, so we need to check in all directions.
				horizNodeCrouch = (horizNode->GetAttributes() & NAV_MESH_CROUCH) != 0;
				if ( horizNode->IsBlockedInAnyDirection() )
				{
					return false;
				}
			}
			else if ( westEdge )
			{
				horizNodeCrouch = horizNode->m_crouch[ SOUTH_EAST ] || horizNode->m_crouch[ NORTH_EAST ];
				if ( horizNode->m_isBlocked[ SOUTH_EAST ] || horizNode->m_isBlocked[ NORTH_EAST ] )
				{
					return false;
				}
			}
			// finally, we have a center node
			else
			{
				// This node is completely in the interior of the area, so we need to check in all directions.
				horizNodeCrouch = (horizNode->GetAttributes() & NAV_MESH_CROUCH) != 0;
				if ( horizNode->IsBlockedInAnyDirection() )
				{
					return false;
				}
			}

			// all nodes must be crouch/non-crouch
			if ( nodeCrouch != horizNodeCrouch )
				return false;

			// all nodes must have the same non-crouch attributes
			int horizNodeAttributes = horizNode->GetAttributes() & ~NAV_MESH_CROUCH;
			if (horizNodeAttributes != nodeAttributes)
				return false;

			if (horizNode->IsCovered())
				return false;

			if (!horizNode->IsClosedCell())
				return false;

			if ( !CheckObstacles( horizNode, width, height, x, y ) )
				return false;

			horizNode = horizNode->GetConnectedNode( EAST );
			if (horizNode == NULL)
				return false;

			// nodes must lie on/near the plane
			if (width > 1 || height > 1)
			{
				float dist = (float)fabs( DotProduct( *horizNode->GetPosition(), normal ) + d );
				if (dist > offPlaneTolerance)
					return false;
			}
		}

		// Check the final (x=width) node, the above only checks thru x=width-1
		if ( !CheckObstacles( horizNode, width, height, x, y ) )
			return false;

		vertNode = vertNode->GetConnectedNode( SOUTH );
		if (vertNode == NULL)
			return false;

		// nodes must lie on/near the plane
		if (width > 1 || height > 1)
		{
			float dist = (float)fabs( DotProduct( *vertNode->GetPosition(), normal ) + d );
			if (dist > offPlaneTolerance)
				return false;
		}
	}

	// check planarity of southern edge
	if (width > 1 || height > 1)
	{
		horizNode = vertNode;

		for( x=0; x<width; x++ )
		{
			if ( !CheckObstacles( horizNode, width, height, x, y ) )
				return false;

			horizNode = horizNode->GetConnectedNode( EAST );
			if (horizNode == NULL)
				return false;

			// nodes must lie on/near the plane
			float dist = (float)fabs( DotProduct( *horizNode->GetPosition(), normal ) + d );
			if (dist > offPlaneTolerance)
				return false;
		}

		// Check the final (x=width) node, the above only checks thru x=width-1
		if ( !CheckObstacles( horizNode, width, height, x, y ) )
			return false;
	}

	vertNode = node;
	for( y=0; y<height; ++y )
	{
		horizNode = vertNode;

		for( int x=0; x<width; ++x )
		{
			// look for odd jump areas (3 points on the ground, 1 point floating much higher or lower)
			if ( !TestForValidJumpArea( horizNode ) )
			{
				return false;
			}

			// Now that we've done the quick checks, test for a valid crouch area.
			// This finds pillars etc in the middle of 4 nodes, that weren't found initially.
			if ( nodeCrouch && !TestForValidCrouchArea( horizNode ) )
			{
				return false;
			}

			horizNode = horizNode->GetConnectedNode( EAST );
		}

		vertNode = vertNode->GetConnectedNode( SOUTH );
	}

	if ( m_generationMode == GENERATE_INCREMENTAL )
	{
		// Incremental generation needs to check that it's not overlapping existing areas...
		const Vector *nw = node->GetPosition();

		vertNode = node;
		for( int y=0; y<height; ++y )
		{
			vertNode = vertNode->GetConnectedNode( SOUTH );
		}
		const Vector *sw = vertNode->GetPosition();

		horizNode = node;
		for( int x=0; x<width; ++x )
		{
			horizNode = horizNode->GetConnectedNode( EAST );
		}
		const Vector *ne = horizNode->GetPosition();

		vertNode = horizNode;
		for( int y=0; y<height; ++y )
		{
			vertNode = vertNode->GetConnectedNode( SOUTH );
		}
		const Vector *se = vertNode->GetPosition();

		TestOverlapping test( *nw, *ne, *sw, *se );
		if ( test.OverlapsExistingArea() )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/** 
* Checks if a node has an untraversable obstacle in any direction to a neighbor.  
* width and height are size of nav area this node would be a part of, x and y are node's position
* within that grid
*/
bool CNavMesh::CheckObstacles( CNavNode *node, int width, int height, int x, int y )
{
	// any area bigger than 1x1 can't have obstacles in any connection between nodes
	if ( width > 1 || height > 1 )
	{
		if ( ( x > 0 ) && ( node->m_obstacleHeight[WEST] > MaxTraversableHeight ) )
			return false;

		if ( ( y > 0 ) && ( node->m_obstacleHeight[NORTH] > MaxTraversableHeight ) )
			return false;

		if ( ( x < width-1 ) && ( node->m_obstacleHeight[EAST] > MaxTraversableHeight ) )
			return false;

		if ( ( y < height-1 ) && ( node->m_obstacleHeight[SOUTH] > MaxTraversableHeight ) )
			return false;
	}

	// 1x1 area can have obstacles, that area will get fixed up later
	return true;
}

//--------------------------------------------------------------------------------------------------------------
/** 
 * Create a nav area, and mark all nodes it overlaps as "covered"
 * NOTE: Nodes on the east and south edges are not included.
 * Returns number of nodes covered by this area, or -1 for error;
 */
int CNavMesh::BuildArea( CNavNode *node, int width, int height )
{
	CNavNode *nwNode = node;
	CNavNode *neNode = NULL;
	CNavNode *swNode = NULL;
	CNavNode *seNode = NULL;

	CNavNode *vertNode = node;
	CNavNode *horizNode;

	int coveredNodes = 0;

	for( int y=0; y<height; y++ )
	{
		horizNode = vertNode;

		for( int x=0; x<width; x++ )
		{
			horizNode->Cover();
			++coveredNodes;
			
			horizNode = horizNode->GetConnectedNode( EAST );
		}

		if (y == 0)
			neNode = horizNode;

		vertNode = vertNode->GetConnectedNode( SOUTH );
	}

	swNode = vertNode;

	horizNode = vertNode;		
	for( int x=0; x<width; x++ )
	{
		horizNode = horizNode->GetConnectedNode( EAST );
	}
	seNode = horizNode;

	if (!nwNode || !neNode || !seNode || !swNode)
	{
		Error( "BuildArea - NULL node.\n" );
		return -1;
	}

	CNavArea *area = CreateArea();
	if (area == NULL)
	{
		Error( "BuildArea: Out of memory.\n" );
		return -1;
	}
	
	area->Build( nwNode, neNode, seNode, swNode );
		
	TheNavAreas.AddToTail( area );
	// since all internal nodes have the same attributes, set this area's attributes

	area->SetAttributes( node->GetAttributes() );

	// If any of the corners have an obstacle in the direction of another corner, then there's an internal obstruction of this nav node.
	// Mark it as not mergable so it doesn't become a part of anything else and we will fix it up later.
	if ( nwNode->m_obstacleHeight[SOUTH] > MaxTraversableHeight || nwNode->m_obstacleHeight[EAST] > MaxTraversableHeight ||
		 neNode->m_obstacleHeight[WEST] > MaxTraversableHeight || neNode->m_obstacleHeight[SOUTH] > MaxTraversableHeight ||
		 seNode->m_obstacleHeight[NORTH] > MaxTraversableHeight || seNode->m_obstacleHeight[WEST] > MaxTraversableHeight ||
		 swNode->m_obstacleHeight[EAST] > MaxTraversableHeight || swNode->m_obstacleHeight[NORTH] > MaxTraversableHeight  )
	{		
		Assert( width == 1 );		// We should only ever try to build a 1x1 area out of any two nodes that have an obstruction between them
		Assert( height == 1 );

		area->SetAttributes( area->GetAttributes() | NAV_MESH_NO_MERGE );
	}

	// Check that the node was crouch in the right direction
	bool nodeCrouch = node->m_crouch[ SOUTH_EAST ];
	if ( (area->GetAttributes() & NAV_MESH_CROUCH) && !nodeCrouch )
	{
		area->SetAttributes( area->GetAttributes() & ~NAV_MESH_CROUCH );
	}

	return coveredNodes;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * This function uses the CNavNodes that have been sampled from the map to
 * generate CNavAreas - rectangular areas of "walkable" space. These areas
 * are connected to each other, proving information on know how to move from
 * area to area.
 *
 * This is a "greedy" algorithm that attempts to cover the walkable area 
 * with the fewest, largest, rectangles.
 */
void CNavMesh::CreateNavAreasFromNodes( void )
{
	// haven't yet seen a map use larger than 30...
	int tryWidth = nav_area_max_size.GetInt();
	int tryHeight = tryWidth;
	int uncoveredNodes = CNavNode::GetListLength();

	while( uncoveredNodes > 0 )
	{
		for( CNavNode *node = CNavNode::GetFirst(); node; node = node->GetNext() )
		{
			if (node->IsCovered())
				continue;

			if (TestArea( node, tryWidth, tryHeight ))
			{
				int covered = BuildArea( node, tryWidth, tryHeight );
				if (covered < 0)
				{
					Error( "Generate: Error - Data corrupt.\n" );
					return;
				}

				uncoveredNodes -= covered;
			}
		}

		if (tryWidth >= tryHeight)
			--tryWidth;
		else
			--tryHeight;

		if (tryWidth <= 0 || tryHeight <= 0)
			break;
	}

	if ( !TheNavAreas.Count() )
	{
		// If we somehow have no areas, don't try to create an impossibly-large grid
		AllocateGrid( 0, 0, 0, 0 );
		return;
	}

	Extent extent;
	extent.lo.x = 9999999999.9f;
	extent.lo.y = 9999999999.9f;
	extent.hi.x = -9999999999.9f;
	extent.hi.y = -9999999999.9f;

	// compute total extent
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		Extent areaExtent;
		area->GetExtent( &areaExtent );

		if (areaExtent.lo.x < extent.lo.x)
			extent.lo.x = areaExtent.lo.x;
		if (areaExtent.lo.y < extent.lo.y)
			extent.lo.y = areaExtent.lo.y;
		if (areaExtent.hi.x > extent.hi.x)
			extent.hi.x = areaExtent.hi.x;
		if (areaExtent.hi.y > extent.hi.y)
			extent.hi.y = areaExtent.hi.y;
	}

	// add the areas to the grid
	AllocateGrid( extent.lo.x, extent.hi.x, extent.lo.y, extent.hi.y );

	FOR_EACH_VEC( TheNavAreas, git )
	{
		AddNavArea( TheNavAreas[ git ] );
	}

	
	ConnectGeneratedAreas();
	MarkPlayerClipAreas();
	MarkJumpAreas();	// mark jump areas before we merge generated areas, so we don't merge jump and non-jump areas
	MergeGeneratedAreas();
	SplitAreasUnderOverhangs();
	SquareUpAreas();
	MarkStairAreas();
	StichAndRemoveJumpAreas();
	HandleObstacleTopAreas();
	FixUpGeneratedAreas();

	/// @TODO: incremental generation doesn't create ladders yet
	if ( m_generationMode != GENERATE_INCREMENTAL )
	{
		for ( int i=0; i<m_ladders.Count(); ++i )
		{
			CNavLadder *ladder = m_ladders[i];
			ladder->ConnectGeneratedLadder( 0.0f );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
// adds walkable positions for any/all positions a mod specifies
void CNavMesh::AddWalkableSeeds( void )
{
	CBaseEntity *spawn = gEntList.FindEntityByClassname( NULL, GetPlayerSpawnName() );

	if (spawn )
	{
		// snap it to the sampling grid
		Vector pos = spawn->GetAbsOrigin();
		pos.x = TheNavMesh->SnapToGrid( pos.x );
		pos.y = TheNavMesh->SnapToGrid( pos.y );

		Vector normal;
		if ( FindGroundForNode( &pos, &normal ) )
		{
			AddWalkableSeed( pos, normal );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Initiate the generation process
 */
void CNavMesh::BeginGeneration( bool incremental )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "nav_generate" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

#ifdef TERROR
	engine->ServerCommand( "director_stop\nnb_delete_all\n" );
	if ( !incremental && !engine->IsDedicatedServer() )
	{
		CBasePlayer *host = UTIL_GetListenServerHost();
		if ( host )
		{
			host->ChangeTeam( TEAM_SPECTATOR );
		}
	}
#else
	engine->ServerCommand( "bot_kick\n" );
#endif

	// Right now, incrementally-generated areas won't connect to existing areas automatically.
	// Since this means hand-editing will be necessary, don't do a full analyze.
	if ( incremental )
	{
		nav_quicksave.SetValue( 1 );
	}

	m_generationState = SAMPLE_WALKABLE_SPACE;
	m_sampleTick = 0;
	m_generationMode = (incremental) ? GENERATE_INCREMENTAL : GENERATE_FULL;
	lastMsgTime = 0.0f;

	// clear any previous mesh
	DestroyNavigationMesh( incremental );

	SetNavPlace( UNDEFINED_PLACE );

	// build internal representations of ladders, which are used to find new walkable areas
	if ( !incremental ) ///< @incremental update doesn't build ladders to avoid overlapping existing ones
	{
		BuildLadders();
	}

	// start sampling from a spawn point
	if ( !incremental )
	{
		AddWalkableSeeds();
	}

	// the system will see this NULL and select the next walkable seed
	m_currentNode = NULL;

	// if there are no seed points, we can't generate
	if (m_walkableSeeds.Count() == 0)
	{
		m_generationMode = GENERATE_NONE;
		Msg( "No valid walkable seed positions.  Cannot generate Navigation Mesh.\n" );
		return;
	}

	// initialize seed list index
	m_seedIdx = 0;

	Msg( "Generating Navigation Mesh...\n" );
	m_generationStartTime = Plat_FloatTime();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Re-analyze an existing Mesh.  Determine Hiding Spots, Encounter Spots, etc.
 */
void CNavMesh::BeginAnalysis( bool quitWhenFinished )
{
#ifdef TERROR
	if ( !engine->IsDedicatedServer() )
	{
		CBasePlayer *host = UTIL_GetListenServerHost();
		if ( host )
		{
			host->ChangeTeam( TEAM_SPECTATOR );
			engine->ServerCommand( "director_no_death_check 1\ndirector_stop\nnb_delete_all\n" );

			ConVarRef mat_fullbright( "mat_fullbright" );
			ConVarRef mat_hdr_level( "mat_hdr_level" );

			if( mat_fullbright.GetBool() )
			{
				Warning( "Setting mat_fullbright 0\n" );
				mat_fullbright.SetValue( 0 );
			}

			if ( mat_hdr_level.GetInt() < 2 )
			{
				Warning( "Enabling HDR and reloading materials\n" );
				mat_hdr_level.SetValue( 2 );
				engine->ClientCommand( host->edict(), "mat_reloadallmaterials\n" );
			}

			// Running a threaded server breaks our lighting calculations
			ConVarRef host_thread_mode( "host_thread_mode" );
			m_hostThreadModeRestoreValue = host_thread_mode.GetInt();
			host_thread_mode.SetValue( 0 );
			ConVarRef mat_queue_mode( "mat_queue_mode" );
			mat_queue_mode.SetValue( 0 );
		}
	}
#endif

	// Remove and re-add elements in TheNavAreas, to ensure indices are useful for progress feedback
	NavAreaVector tmpSet;
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			tmpSet.AddToTail( TheNavAreas[it] );
		}
	}
	TheNavAreas.RemoveAll();
	{
		FOR_EACH_VEC( tmpSet, it )
		{
			TheNavAreas.AddToTail( tmpSet[it] );
		}
	}

	DestroyHidingSpots();
	m_generationState = FIND_HIDING_SPOTS;
	m_generationIndex = 0;
	m_generationMode = GENERATE_ANALYSIS_ONLY;
	m_bQuitWhenFinished = quitWhenFinished;
	lastMsgTime = 0.0f;
	m_generationStartTime = Plat_FloatTime();
}


//--------------------------------------------------------------------------------------------------------------
void ShowViewPortPanelToAll( const char * name, bool bShow, KeyValues *data )
{
	CRecipientFilter filter;
	filter.AddAllPlayers();
	filter.MakeReliable();

	int count = 0;
	KeyValues *subkey = NULL;

	if ( data )
	{
		subkey = data->GetFirstSubKey();
		while ( subkey )
		{
			count++; subkey = subkey->GetNextKey();
		}

		subkey = data->GetFirstSubKey(); // reset 
	}

	UserMessageBegin( filter, "VGUIMenu" );
		WRITE_STRING( name ); // menu name
		WRITE_BYTE( bShow?1:0 );
		WRITE_BYTE( count );
		
		// write additional data (be careful not more than 192 bytes!)
		while ( subkey )
		{
			WRITE_STRING( subkey->GetName() );
			WRITE_STRING( subkey->GetString() );
			subkey = subkey->GetNextKey();
		}
	MessageEnd();
}


//--------------------------------------------------------------------------------------------------------------
static void AnalysisProgress( const char *msg, int ticks, int current, bool showPercent = true )
{
	const float MsgInterval = 10.0f;
	float now = Plat_FloatTime();
	if ( now > lastMsgTime + MsgInterval )
	{
		if ( showPercent && ticks )
		{
			Msg( "%s %.0f%%\n", msg, current*100.0f/ticks );
		}
		else
		{
			Msg( "%s\n", msg );
		}

		lastMsgTime = now;
	}

	KeyValues *data = new KeyValues("data");
	data->SetString( "msg",	msg );
	data->SetInt( "total", ticks );
	data->SetInt( "current", current );

	ShowViewPortPanelToAll( PANEL_NAV_PROGRESS, true, data );

	data->deleteThis();
}


//--------------------------------------------------------------------------------------------------------------
static void HideAnalysisProgress( void )
{
	KeyValues *data = new KeyValues("data");
	ShowViewPortPanelToAll( PANEL_NAV_PROGRESS, false, data );
	data->deleteThis();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Process the auto-generation for 'maxTime' seconds. return false if generation is complete.
 */
bool CNavMesh::UpdateGeneration( float maxTime )
{
	double startTime = Plat_FloatTime();
	static unsigned int s_movedPlayerToArea = 0;	// Last area we moved a player to for lighting calcs
	static CountdownTimer s_playerSettleTimer;		// Settle time after moving the player for lighting calcs
	static CUtlVector<CNavArea *> s_unlitAreas;
	static CUtlVector<CNavArea *> s_unlitSeedAreas;

	static ConVarRef host_thread_mode( "host_thread_mode" );

	switch( m_generationState )
	{
		//---------------------------------------------------------------------------
		case SAMPLE_WALKABLE_SPACE:
		{
			AnalysisProgress( "Sampling walkable space...", 100, m_sampleTick / 10, false );
			m_sampleTick = ( m_sampleTick + 1 ) % 1000;

			while ( SampleStep() )
			{
				if ( Plat_FloatTime() - startTime > maxTime )
				{
					return true;
				}
			}

			// sampling is complete, now build nav areas
			m_generationState = CREATE_AREAS_FROM_SAMPLES;

			return true;
		}

		//---------------------------------------------------------------------------
		case CREATE_AREAS_FROM_SAMPLES:
		{
			Msg( "Creating navigation areas from sampled data...\n" );

			// Select all pre-existing areas
			if ( m_generationMode == GENERATE_INCREMENTAL )
			{
				ClearSelectedSet();
				FOR_EACH_VEC( TheNavAreas, nit )
				{
					CNavArea *area = TheNavAreas[nit];
					AddToSelectedSet( area );
				}
			}

			// Create new areas
			CreateNavAreasFromNodes();

			// And toggle the selection, so we end up with the new areas
			if ( m_generationMode == GENERATE_INCREMENTAL )
			{
				CommandNavToggleSelectedSet();
			}

			DestroyHidingSpots();

			// Remove and re-add elements in TheNavAreas, to ensure indices are useful for progress feedback
			NavAreaVector tmpSet;
			{
				FOR_EACH_VEC( TheNavAreas, it )
				{
					tmpSet.AddToTail( TheNavAreas[it] );
				}
			}
			TheNavAreas.RemoveAll();
			{
				FOR_EACH_VEC( tmpSet, it )
				{
					TheNavAreas.AddToTail( tmpSet[it] );
				}
			}

			m_generationState = FIND_HIDING_SPOTS;
			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case FIND_HIDING_SPOTS:
		{
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->ComputeHidingSpots();

				// don't go over our time allotment
				if( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Finding hiding spots...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}

			Msg( "Finding hiding spots...DONE\n" );

			m_generationState = FIND_ENCOUNTER_SPOTS;
			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case FIND_ENCOUNTER_SPOTS:
		{
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->ComputeSpotEncounters();

				// don't go over our time allotment
				if( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Finding encounter spots...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}

			Msg( "Finding encounter spots...DONE\n" );

			m_generationState = FIND_SNIPER_SPOTS;
			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case FIND_SNIPER_SPOTS:
		{
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->ComputeSniperSpots();

				// don't go over our time allotment
				if( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Finding sniper spots...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}

			Msg( "Finding sniper spots...DONE\n" );

			m_generationState = COMPUTE_MESH_VISIBILITY;
			m_generationIndex = 0;
			BeginVisibilityComputations();
			Msg( "Computing mesh visibility...\n" );
		
			return true;
		}

		//---------------------------------------------------------------------------
		case COMPUTE_MESH_VISIBILITY:
		{
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->ComputeVisibilityToMesh();

				// don't go over our time allotment
				if ( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Computing mesh visibility...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}

			Msg( "Optimizing mesh visibility...\n" );

			EndVisibilityComputations();

			Msg( "Computing mesh visibility...DONE\n" );

			m_generationState = FIND_EARLIEST_OCCUPY_TIMES;
			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case FIND_EARLIEST_OCCUPY_TIMES:
		{
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->ComputeEarliestOccupyTimes();

				// don't go over our time allotment
				if( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Finding earliest occupy times...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}

			Msg( "Finding earliest occupy times...DONE\n" );

#ifdef NAV_ANALYZE_LIGHT_INTENSITY
			bool shouldSkipLightComputation = ( m_generationMode == GENERATE_INCREMENTAL || engine->IsDedicatedServer() );
#else
			bool shouldSkipLightComputation = true;
#endif

			if ( shouldSkipLightComputation )
			{
				m_generationState = CUSTOM;	// no light intensity calcs for incremental generation or dedicated servers
			}
			else
			{
				m_generationState = FIND_LIGHT_INTENSITY;
				s_playerSettleTimer.Invalidate();
				CNavArea::MakeNewMarker();
				s_unlitAreas.RemoveAll();
				FOR_EACH_VEC( TheNavAreas, nit )
				{
					s_unlitAreas.AddToTail( TheNavAreas[nit] );
					s_unlitSeedAreas.AddToTail( TheNavAreas[nit] );
				}
			}

			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case FIND_LIGHT_INTENSITY:
		{
			host_thread_mode.SetValue( 0 );	// need non-threaded server for light calcs

			CBasePlayer *host = UTIL_GetListenServerHost();

			if ( !s_unlitAreas.Count() || !host )
			{
				Msg( "Finding light intensity...DONE\n" );

				m_generationState = CUSTOM;
				m_generationIndex = 0;
				return true;
			}

			if ( !s_playerSettleTimer.IsElapsed() )
				return true; // wait for eyePos to settle

			// Now try to compute lighting for remaining areas
			int sit = 0;
			while( sit < s_unlitAreas.Count() )
			{
				CNavArea *area = s_unlitAreas[sit];
				if ( area->ComputeLighting() )
				{
					s_unlitSeedAreas.FindAndRemove( area );
					s_unlitAreas.Remove( sit );

					continue;
				}
				else
				{
					++sit;
				}
			}

			if ( s_unlitAreas.Count() )
			{
				if ( s_unlitSeedAreas.Count() )
				{
					CNavArea *moveArea = s_unlitSeedAreas[0];
					s_unlitSeedAreas.FastRemove( 0 );

					//Msg( "Moving to new area %d to compute lighting for %d/%d areas\n", moveArea->GetID(), s_unlitAreas.Count(), TheNavAreas.Count() );

					Vector eyePos = moveArea->GetCenter();
					float height;
					if ( GetGroundHeight( eyePos, &height ) )
					{
						eyePos.z = height + HalfHumanHeight - StepHeight;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
					}
					else
					{
						eyePos.z += HalfHumanHeight - StepHeight;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
					}
					host->SetAbsOrigin( eyePos );
					AnalysisProgress( "Finding light intensity...", 100, 100 * (TheNavAreas.Count() - s_unlitAreas.Count()) / TheNavAreas.Count() );
					s_movedPlayerToArea = moveArea->GetID();
					s_playerSettleTimer.Start( 0.1f );
					return true;
				}
				else
				{
					Msg( "Finding light intensity...DONE (%d unlit areas)\n", s_unlitAreas.Count() );
					if ( s_unlitAreas.Count() )
					{
						Warning( "To see unlit areas:\n" );
						for ( int sit=0; sit<s_unlitAreas.Count(); ++sit )
						{
							CNavArea *area = s_unlitAreas[ sit ];
							Warning( "nav_unmark; nav_mark %d; nav_warp_to_mark;\n", area->GetID() );
						}
					}

					m_generationState = CUSTOM;
					m_generationIndex = 0;
				}
			}

			Msg( "Finding light intensity...DONE\n" );

			m_generationState = CUSTOM;
			m_generationIndex = 0;
			return true;
		}

		//---------------------------------------------------------------------------
		case CUSTOM:
		{
			if ( m_generationIndex == 0 )
			{
				BeginCustomAnalysis( m_generationMode == GENERATE_INCREMENTAL );
				Msg( "Start custom...\n ");
			}
			while( m_generationIndex < TheNavAreas.Count() )
			{
				CNavArea *area = TheNavAreas[ m_generationIndex ];
				++m_generationIndex;

				area->CustomAnalysis( m_generationMode == GENERATE_INCREMENTAL );

				// don't go over our time allotment
				if( Plat_FloatTime() - startTime > maxTime )
				{
					AnalysisProgress( "Custom game-specific analysis...", 100, 100 * m_generationIndex / TheNavAreas.Count() );
					return true;
				}
			}
			
			Msg( "Post custom...\n ");
			PostCustomAnalysis();

			EndCustomAnalysis();
			Msg( "Custom game-specific analysis...DONE\n" );

			m_generationState = SAVE_NAV_MESH;
			m_generationIndex = 0;
			ConVarRef mat_queue_mode( "mat_queue_mode" );
			mat_queue_mode.SetValue( -1 );
			host_thread_mode.SetValue( m_hostThreadModeRestoreValue );	// restore this
			return true;
		}

		//---------------------------------------------------------------------------
		case SAVE_NAV_MESH:
		{
			if ( m_generationMode == GENERATE_ANALYSIS_ONLY || m_generationMode == GENERATE_FULL )
			{
				m_isAnalyzed = true;
			}

			// generation complete!
			float generationTime = Plat_FloatTime() - m_generationStartTime;
			Msg( "Generation complete!  %0.1f seconds elapsed.\n", generationTime );
			bool restart = m_generationMode != GENERATE_INCREMENTAL;
			m_generationMode = GENERATE_NONE;
			m_isLoaded = true;
			ClearWalkableSeeds();

			HideAnalysisProgress();

			// save the mesh
			if (Save())
			{
				Msg( "Navigation map '%s' saved.\n", GetFilename() );
			}
			else
			{
				const char *filename = GetFilename();
				Msg( "ERROR: Cannot save navigation map '%s'.\n", (filename) ? filename : "(null)" );
			}

			if ( m_bQuitWhenFinished )
			{
				engine->ServerCommand( "quit\n" );
			}
			else if ( restart )
			{
				engine->ChangeLevel( STRING( gpGlobals->mapname ), NULL );
			}
			else
			{
				FOR_EACH_VEC( TheNavAreas, it )
				{
					TheNavAreas[ it ]->ResetNodes();
				}

#if !(DEBUG_NAV_NODES)
				// destroy navigation nodes created during map generation
				CNavNode *node, *next;
				for( node = CNavNode::m_list; node; node = next )
				{
					next = node->m_next;
					delete node;
				}
				CNavNode::m_list = NULL;
				CNavNode::m_listLength = 0;
				CNavNode::m_nextID = 1;
#endif // !(DEBUG_NAV_NODES)
			}

			return false;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Define the name of player spawn entities
 */
void CNavMesh::SetPlayerSpawnName( const char *name )
{
	if (m_spawnName)
	{
		delete [] m_spawnName;
	}

	m_spawnName = new char [ strlen(name) + 1 ];
	strcpy( m_spawnName, name );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return name of player spawn entity
 */
const char *CNavMesh::GetPlayerSpawnName( void ) const
{
	if (m_spawnName)
		return m_spawnName;

	// default value
	return "info_player_start";
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Add a nav node and connect it.
 * Node Z positions are ground level.
 */
CNavNode *CNavMesh::AddNode( const Vector &destPos, const Vector &normal, NavDirType dir, CNavNode *source, bool isOnDisplacement, 
							float obstacleHeight, float obstacleStartDist, float obstacleEndDist )
{
	// check if a node exists at this location
	CNavNode *node = CNavNode::GetNode( destPos );
	
	// if no node exists, create one
	bool useNew = false;
	if (node == NULL)
	{
		node = new CNavNode( destPos, normal, source, isOnDisplacement );
		OnNodeAdded( node );
		useNew = true;
	}

	// connect source node to new node
	source->ConnectTo( node, dir, obstacleHeight, obstacleStartDist, obstacleEndDist );

	// optimization: if deltaZ changes very little, assume connection is commutative
	const float zTolerance = 50.0f;
	float deltaZ = source->GetPosition()->z - destPos.z;
	if (fabs( deltaZ ) < zTolerance)
	{
		if ( obstacleHeight > 0 )
		{
			obstacleHeight = MAX( obstacleHeight + deltaZ, 0 );
			Assert( obstacleHeight > 0 );
		}
		node->ConnectTo( source, OppositeDirection( dir ), obstacleHeight, GenerationStepSize - obstacleEndDist, GenerationStepSize - obstacleStartDist );
		node->MarkAsVisited( OppositeDirection( dir ) );
	}

	if (useNew)
	{
		// new node becomes current node
		m_currentNode = node;
	}

	node->CheckCrouch();

	// determine if there's a cliff nearby and set an attribute on this node
	for ( int i = 0; i < NUM_DIRECTIONS; i++ )
	{
		NavDirType dir = (NavDirType) i;
		if ( CheckCliff( node->GetPosition(), dir ) )
		{
			node->SetAttributes( node->GetAttributes() | NAV_MESH_CLIFF );
			break;
		}
	}

	return node;
}

//--------------------------------------------------------------------------------------------------------------
inline CNavNode *LadderEndSearch( const Vector *pos, NavDirType mountDir )
{
	Vector center = *pos;
	AddDirectionVector( &center, mountDir, HalfHumanWidth );

	//
	// Test the ladder dismount point first, then each cardinal direction one and two steps away
	//
	for( int d=(-1); d<2*NUM_DIRECTIONS; ++d )
	{
		Vector tryPos = center;

		if (d >= NUM_DIRECTIONS)
			AddDirectionVector( &tryPos, (NavDirType)(d - NUM_DIRECTIONS), 2.0f*GenerationStepSize );
		else if (d >= 0)
			AddDirectionVector( &tryPos, (NavDirType)d, GenerationStepSize );

		// step up a rung, to ensure adjacent floors are below us
		tryPos.z += GenerationStepSize;

		tryPos.x = TheNavMesh->SnapToGrid( tryPos.x );
		tryPos.y = TheNavMesh->SnapToGrid( tryPos.y );

		// adjust height to account for sloping areas
		Vector tryNormal;
		if (TheNavMesh->GetGroundHeight( tryPos, &tryPos.z, &tryNormal ) == false)
			continue;

		// make sure this point is not on the other side of a wall
		const float fudge = 4.0f;
		trace_t result;
		UTIL_TraceHull( center + Vector( 0, 0, fudge ), tryPos + Vector( 0, 0, fudge ), NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), NULL, COLLISION_GROUP_NONE, &result );
		if (result.fraction != 1.0f || result.startsolid)
			continue;

		// if no node exists here, create one and continue the search
		if (CNavNode::GetNode( tryPos ) == NULL)
		{
			return new CNavNode( tryPos, tryNormal, NULL, false );
		}
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
bool CNavMesh::FindGroundForNode( Vector *pos, Vector *normal )
{
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT, WALK_THRU_EVERYTHING );
	trace_t tr;
	Vector start( pos->x, pos->y, pos->z + VEC_DUCK_HULL_MAX.z - 0.1f );
	Vector end( *pos );
	end.z -= DeathDrop;

	UTIL_TraceHull(
		start,
		end,
		NavTraceMins,
		NavTraceMaxs,
		GetGenerationTraceMask(),
		&filter,
		&tr );

	*pos = tr.endpos;
	*normal = tr.plane.normal;

	return ( !tr.allsolid );
}


//--------------------------------------------------------------------------------------------------------------
void DrawTrace( const trace_t *trace )
{
	/*
	if ( trace->fraction > 0.0f && !trace->startsolid )
	{
		NDebugOverlay::SweptBox( trace->startpos, trace->endpos, NavTraceMins, NavTraceMaxs, vec3_angle, 0, 255, 0, 45, 100 );
	}
	else
	{
		NDebugOverlay::SweptBox( trace->startpos, trace->endpos, NavTraceMins, NavTraceMaxs, vec3_angle, 255, 0, 0, 45, 100 );
	}
	*/
}


//--------------------------------------------------------------------------------------------------------------
bool StayOnFloor( trace_t *trace, float zLimit /* = DeathDrop */ )
{
	Vector start( trace->endpos );
	Vector end( start );
	end.z -= zLimit;

	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
	UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), &filter, trace );
	DrawTrace( trace );

	if ( trace->startsolid || trace->fraction >= 1.0f )
	{
		return false;
	}

	if ( trace->plane.normal.z < nav_slope_limit.GetFloat() )
	{
		return false;
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------------
bool TraceAdjacentNode( int depth, const Vector& start, const Vector& end, trace_t *trace, float zLimit /* = DeathDrop */ )
{
	const float MinDistance = 1.0f;	// if we can't move at least this far, don't bother stepping up.

	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
	UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), &filter, trace );
	DrawTrace( trace );

	// If we started in the ground for some reason, bail
	if ( trace->startsolid )
		return false;

	// If we made it, so try to find the floor
	if ( end.x == trace->endpos.x && end.y == trace->endpos.y )
	{
		return StayOnFloor( trace, zLimit );
	}

	// If we didn't make enough progress, bail
	if ( depth && start.AsVector2D().DistToSqr( trace->endpos.AsVector2D() ) < MinDistance * MinDistance )
	{
		return false;
	}

	// We made it more than MinDistance.  If the slope is too steep, we can't go on.
	if ( !StayOnFloor( trace, zLimit ) )
	{
		return false;
	}

	// Try to go up as if we stepped up, forward, and down.
	Vector testStart( trace->endpos );
	Vector testEnd( testStart );
	testEnd.z += StepHeight;
	UTIL_TraceHull( testStart, testEnd, NavTraceMins, NavTraceMaxs, TheNavMesh->GetGenerationTraceMask(), &filter, trace );
	DrawTrace( trace );

	Vector forwardTestStart = trace->endpos;
	Vector forwardTestEnd = end;
	forwardTestEnd.z = forwardTestStart.z;
	return TraceAdjacentNode( depth+1, forwardTestStart, forwardTestEnd, trace );
}


//--------------------------------------------------------------------------------------------------------
static bool IsNodeOverlapped( const Vector& pos, const Vector& offset )
{
	bool overlap = TheNavMesh->GetNavArea( pos + offset, HumanHeight ) != NULL;
	if ( !overlap )
	{
		Vector mins( -0.5f, -0.5f, -0.5f );
		Vector maxs( 0.5f, 0.5f, 0.5f );

		Vector start = pos;
		start.z += HalfHumanHeight;
		Vector end = start;
		end.x += offset.x * GenerationStepSize;
		end.y += offset.y * GenerationStepSize;
		trace_t trace;
		CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
		UTIL_TraceHull( start, end, mins, maxs, TheNavMesh->GetGenerationTraceMask(), &filter, &trace );
		if ( trace.startsolid || trace.allsolid )
		{
			return true;
		}

		if ( trace.fraction < 0.1f )
		{
			return true;
		}

		start = trace.endpos;
		end.z -= HalfHumanHeight * 2;
		UTIL_TraceHull( start, end, mins, maxs, TheNavMesh->GetGenerationTraceMask(), &filter, &trace );
		if ( trace.startsolid || trace.allsolid )
		{
			return true;
		}
		
		if ( trace.fraction == 1.0f )
		{
			return true;
		}

		if ( trace.plane.normal.z < 0.7f )
		{
			return true;
		}
	}
	return overlap;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Search the world and build a map of possible movements.
 * The algorithm begins at the bot's current location, and does a recursive search
 * outwards, tracking all valid steps and generating a directed graph of CNavNodes.
 *
 * Sample the map one "step" in a cardinal direction to learn the map.
 *
 * Returns true if sampling needs to continue, or false if done.
 */
bool CNavMesh::SampleStep( void )
{
	// take a step
	while( true )
	{
		if (m_currentNode == NULL)
		{
			// sampling is complete from current seed, try next one
			m_currentNode = GetNextWalkableSeedNode();

			if (m_currentNode == NULL)
			{
				if ( m_generationMode == GENERATE_INCREMENTAL || m_generationMode == GENERATE_SIMPLIFY )
				{
					return false;
				}

				// search is exhausted - continue search from ends of ladders
				for ( int i=0; i<m_ladders.Count(); ++i )
				{
					CNavLadder *ladder = m_ladders[i];

					// check ladder bottom
					if ((m_currentNode = LadderEndSearch( &ladder->m_bottom, ladder->GetDir() )) != 0)
						break;

					// check ladder top
					if ((m_currentNode = LadderEndSearch( &ladder->m_top, ladder->GetDir() )) != 0)
						break;
				}

				if (m_currentNode == NULL)
				{
					// all seeds exhausted, sampling complete
					return false;
				}
			}
		}

		//
		// Take a step from this node
		//
		for( int dir = NORTH; dir < NUM_DIRECTIONS; dir++ )
		{
			if (!m_currentNode->HasVisited( (NavDirType)dir ))
			{
				// have not searched in this direction yet

				// start at current node position
				Vector pos = *m_currentNode->GetPosition();

				// snap to grid
				int cx = SnapToGrid( pos.x );
				int cy = SnapToGrid( pos.y );

				// attempt to move to adjacent node
				switch( dir )
				{
					case NORTH:		cy -= GenerationStepSize; break;
					case SOUTH:		cy += GenerationStepSize; break;
					case EAST:		cx += GenerationStepSize; break;
					case WEST:		cx -= GenerationStepSize; break;
				}

				pos.x = cx;
				pos.y = cy;

				m_generationDir = (NavDirType)dir;

				// mark direction as visited
				m_currentNode->MarkAsVisited( m_generationDir );

				// sanity check to not generate across the world for incremental generation
				const float incrementalRange = nav_generate_incremental_range.GetFloat();
				if ( m_generationMode == GENERATE_INCREMENTAL && incrementalRange > 0 )
				{
					bool inRange = false;
					for ( int i=0; i<m_walkableSeeds.Count(); ++i )
					{
						const Vector &seedPos = m_walkableSeeds[i].pos;
						if ( (seedPos - pos).IsLengthLessThan( incrementalRange ) )
						{
							inRange = true;
							break;
						}
					}

					if ( !inRange )
					{
						return true;
					}
				}

				if ( m_generationMode == GENERATE_SIMPLIFY )
				{
					if ( !m_simplifyGenerationExtent.Contains( pos ) )
					{
						return true;
					}
				}

				// test if we can move to new position
				trace_t result;
				Vector from( *m_currentNode->GetPosition() );
				CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
				Vector to = vec3_origin, toNormal = vec3_origin;
				float obstacleHeight = 0, obstacleStartDist = 0, obstacleEndDist = GenerationStepSize;
				if ( TraceAdjacentNode( 0, from, pos, &result ) )
				{
					to = result.endpos;
					toNormal = result.plane.normal;
				}
				else
				{
					// test going up ClimbUpHeight
					bool success = false;
					for ( float height = StepHeight; height <= ClimbUpHeight; height += 1.0f )
					{						
						trace_t tr;
						Vector start( from );
						Vector end( pos );
						start.z += height;
						end.z += height;
						UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, GetGenerationTraceMask(), &filter, &tr );
						if ( !tr.startsolid && tr.fraction == 1.0f )
						{
							if ( !StayOnFloor( &tr ) )
							{
								break;
							}

							to = tr.endpos;
							toNormal = tr.plane.normal;

							start = end = from;
							end.z += height;
							UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, GetGenerationTraceMask(), &filter, &tr );
							if ( tr.fraction < 1.0f )
							{
								break;
							}

							// keep track of far up we had to go to find a path to the next node
							obstacleHeight = height;
							success = true;
							break;
						}
						else
						{
							// Could not trace from node to node at this height, something is in the way.
							// Trace in the other direction to see if we hit something
							Vector vecToObstacleStart = tr.endpos - start;
							Assert( vecToObstacleStart.LengthSqr() <= Square( GenerationStepSize ) );
							if ( vecToObstacleStart.LengthSqr() <= Square( GenerationStepSize ) )
							{
								UTIL_TraceHull( end, start, NavTraceMins, NavTraceMaxs, GetGenerationTraceMask(), &filter, &tr );
								if ( !tr.startsolid && tr.fraction < 1.0 )
								{
									// We hit something going the other direction.  There is some obstacle between the two nodes.
									Vector vecToObstacleEnd = tr.endpos - start;
									Assert( vecToObstacleEnd.LengthSqr() <= Square( GenerationStepSize ) );
									if ( vecToObstacleEnd.LengthSqr() <= Square( GenerationStepSize )  )
									{
										// Remember the distances to start and end of the obstacle (with respect to the "from" node).
										// Keep track of the last distances to obstacle as we keep increasing the height we do a trace for.
										// If we do eventually clear the obstacle, these values will be the start and end distance to the
										// very tip of the obstacle.
										obstacleStartDist = vecToObstacleStart.Length();
										obstacleEndDist = vecToObstacleEnd.Length();
										if ( obstacleEndDist == 0 )
										{
											obstacleEndDist = GenerationStepSize;
										}
									}								
								}
							}
						}
					}

					if ( !success )
					{
						return true;
					}
				}

				// Don't generate nodes if we spill off the end of the world onto skybox
				if ( result.surface.flags & ( SURF_SKY|SURF_SKY2D ) )
				{
					return true;
				}

				// If we're incrementally generating, don't overlap existing nav areas.
				Vector testPos( to );
				bool overlapSE = IsNodeOverlapped( testPos, Vector(  1,  1, HalfHumanHeight ) );
				bool overlapSW = IsNodeOverlapped( testPos, Vector( -1,  1, HalfHumanHeight ) );
				bool overlapNE = IsNodeOverlapped( testPos, Vector(  1, -1, HalfHumanHeight ) );
				bool overlapNW = IsNodeOverlapped( testPos, Vector( -1, -1, HalfHumanHeight ) );
				if ( overlapSE && overlapSW && overlapNE && overlapNW && m_generationMode != GENERATE_SIMPLIFY )
				{
					return true;
				}

				int nTolerance = nav_generate_incremental_tolerance.GetInt();
				if ( nTolerance > 0 && m_generationMode == GENERATE_INCREMENTAL )
				{
					bool bValid = false;
					int zPos = to.z;
					for ( int i=0; i<m_walkableSeeds.Count(); ++i )
					{
						const Vector &seedPos = m_walkableSeeds[i].pos;
						int zMin = seedPos.z - nTolerance;
						int zMax = seedPos.z + nTolerance;

						if ( zPos >= zMin && zPos <= zMax )
						{
							bValid = true;
							break;
						}
					}

					if ( !bValid )
						return true;
				}


				bool isOnDisplacement = result.IsDispSurface();

				if ( nav_displacement_test.GetInt() > 0 )
				{
					// Test for nodes under displacement surfaces.
					// This happens during development, and is a pain because the space underneath a displacement
					// is not 'solid'.
					Vector start = to + Vector( 0, 0, 0 );
					Vector end = start + Vector( 0, 0, nav_displacement_test.GetInt() );
					UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, GetGenerationTraceMask(), &filter, &result );

					if ( result.fraction > 0 )
					{
						end = start;
						start = result.endpos;
						UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, GetGenerationTraceMask(), &filter, &result );
						if ( result.fraction < 1 )
						{
							// if we made it down to within StepHeight, maybe we're on a static prop
							if ( result.endpos.z > to.z + StepHeight )
							{
								return true;
							}
						}
					}
				}

				float deltaZ = to.z - m_currentNode->GetPosition()->z;
				// If there's an obstacle in the way and it's traversable, or the obstacle is not higher than the destination node itself minus a small epsilon
				// (meaning the obstacle was just the height change to get to the destination node, no extra obstacle between the two), clear obstacle height
				// and distances
				if ( ( obstacleHeight < MaxTraversableHeight ) || ( deltaZ > ( obstacleHeight - 2.0f ) ) )
				{
					obstacleHeight = 0;
					obstacleStartDist = 0;
					obstacleEndDist = GenerationStepSize;
				}

				// we can move here
				// create a new navigation node, and update current node pointer
				AddNode( to, toNormal, m_generationDir, m_currentNode, isOnDisplacement, obstacleHeight, obstacleStartDist, obstacleEndDist );

				return true;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		m_currentNode = m_currentNode->GetParent();
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add given walkable position to list of seed positions for map sampling
 */
void CNavMesh::AddWalkableSeed( const Vector &pos, const Vector &normal )
{
	WalkableSeedSpot seed;

	seed.pos.x = RoundToUnits( pos.x, GenerationStepSize );
	seed.pos.y = RoundToUnits( pos.y, GenerationStepSize );
	seed.pos.z = pos.z;
	seed.normal = normal;

	m_walkableSeeds.AddToTail( seed );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the next walkable seed as a node
 */
CNavNode *CNavMesh::GetNextWalkableSeedNode( void )
{	
	if ( m_seedIdx >= m_walkableSeeds.Count() )
		return NULL;

	WalkableSeedSpot spot = m_walkableSeeds[ m_seedIdx ];
	++m_seedIdx;

	// check if a node exists at this location
	CNavNode *node = CNavNode::GetNode( spot.pos );
	if ( node )
		return NULL;

	return new CNavNode( spot.pos, spot.normal, NULL, false );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check LOS, ignoring any entities that we can walk through
 */
bool IsWalkableTraceLineClear( const Vector &from, const Vector &to, unsigned int flags )
{
	trace_t result;
	CBaseEntity *ignore = NULL;
	Vector useFrom = from;

	CTraceFilterWalkableEntities traceFilter( NULL, COLLISION_GROUP_NONE, flags );

	result.fraction = 0.0f;

	const int maxTries = 50;
	for( int t=0; t<maxTries; ++t )
	{
		UTIL_TraceLine( useFrom, to, MASK_NPCSOLID, &traceFilter, &result );

		// if we hit a walkable entity, try again
		if (result.fraction != 1.0f && IsEntityWalkable( result.m_pEnt, flags ))
		{
			ignore = result.m_pEnt;

			// start from just beyond where we hit to avoid infinite loops
			Vector dir = to - from;
			dir.NormalizeInPlace();
			useFrom = result.endpos + 5.0f * dir;
		}
		else
		{
			break;
		}
	}

	if (result.fraction == 1.0f)
		return true;

	return false;
}


//--------------------------------------------------------------------------------------------------------------
class Subdivider
{
public:
	Subdivider( int depth )
	{
		m_depth = depth;
	}

	bool operator() ( CNavArea *area )
	{
		SubdivideX( area, true, true, m_depth );
		
		return true;
	}

	void SubdivideX( CNavArea *area, bool canDivideX, bool canDivideY, int depth )
	{
		if (!canDivideX || depth <= 0)
			return;
			
		float split = area->GetSizeX() / 2.0f;
		
		if (split < GenerationStepSize)
		{
			if (canDivideY)
			{
				SubdivideY( area, false, canDivideY, depth );
			}
			return;
		}

		split += area->GetCorner( NORTH_WEST ).x;

		split = TheNavMesh->SnapToGrid( split );

		CNavArea *alpha, *beta;
		if (area->SplitEdit( false, split, &alpha, &beta ))
		{
			SubdivideY( alpha, canDivideX, canDivideY, depth );
			SubdivideY( beta, canDivideX, canDivideY, depth );
		}
	}


	void SubdivideY( CNavArea *area, bool canDivideX, bool canDivideY, int depth )
	{
		if (!canDivideY)
			return;
			
		float split = area->GetSizeY() / 2.0f;

		if (split < GenerationStepSize)
		{
			if (canDivideX)
			{
				SubdivideX( area, canDivideX, false, depth-1 );
			}
			return;
		}
		
		split += area->GetCorner( NORTH_WEST ).y;

		split = TheNavMesh->SnapToGrid( split );

		CNavArea *alpha, *beta;
		if (area->SplitEdit( true, split, &alpha, &beta ))
		{
			SubdivideX( alpha, canDivideX, canDivideY, depth-1 );
			SubdivideX( beta, canDivideX, canDivideY, depth-1 );
		}
	}
	
	int m_depth;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Subdivide each nav area in X and Y to create 4 new areas
 */
void CNavMesh::CommandNavSubdivide( const CCommand &args )
{
	int depth = 1;
	
	if (args.ArgC() == 2)
	{
		depth = atoi( args[1] );
	}

	Subdivider chop( depth );
	TheNavMesh->ForAllSelectedAreas( chop );
}

CON_COMMAND_F( nav_subdivide, "Subdivides all selected areas.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSubdivide( args );
}

//--------------------------------------------------------------------------------------------------------------
/**
* Debugging code to verify that all nav area connections are internally consistent
*/
void CNavMesh::ValidateNavAreaConnections( void )
{
	// iterate all nav areas
	NavConnect connect;

	for ( int it = 0; it < TheNavAreas.Count(); it++ )
	{
		CNavArea *area = TheNavAreas[ it ];

		for ( NavDirType dir = NORTH; dir < NUM_DIRECTIONS; dir = (NavDirType) ( ( (int) dir ) +1 ) )
		{
			const NavConnectVector *pOutgoing = area->GetAdjacentAreas(  dir );
			const NavConnectVector *pIncoming = area->GetIncomingConnections( dir );

			for ( int iConnect = 0; iConnect < pOutgoing->Count(); iConnect++ )
			{
				// make sure no area is on both the connection and incoming list
				CNavArea *areaOther = (*pOutgoing)[iConnect].area;
				connect.area = areaOther;
				if ( pIncoming->Find( connect ) != pIncoming->InvalidIndex() )
				{
					Msg( "Area %d has area %d on both 2-way and incoming list, should only be on one\n", area->GetID(), areaOther->GetID() );
					Assert( false );
				}

				// make sure there are no duplicate connections on the list
				for ( int iConnectCheck = iConnect+1; iConnectCheck < pOutgoing->Count(); iConnectCheck++ )
				{
					CNavArea *areaCheck = (*pOutgoing)[iConnectCheck].area;
					if ( areaOther == areaCheck )
					{
						Msg( "Area %d has multiple outgoing connections to area %d in direction %d\n", area->GetID(), areaOther->GetID(), dir );
						Assert( false );
					}
				}

				const NavConnectVector *pOutgoingOther = areaOther->GetAdjacentAreas( OppositeDirection( dir ) );
				const NavConnectVector *pIncomingOther = areaOther->GetIncomingConnections( OppositeDirection( dir ) );

				// if we have a one-way outgoing connection, make sure we are on the other area's incoming list
				connect.area = area;
				bool bIsTwoWay = pOutgoingOther->Find( connect ) != pOutgoingOther->InvalidIndex();				
				if ( !bIsTwoWay )
				{				
					connect.area = area;
					bool bOnOthersIncomingList = pIncomingOther->Find( connect ) != pIncomingOther->InvalidIndex();
					if ( !bOnOthersIncomingList )
					{
						Msg( "Area %d has one-way connect to area %d but does not appear on the latter's incoming list\n", area->GetID(), areaOther->GetID() );
					}					
				}
			}

			for ( int iConnect = 0; iConnect < pIncoming->Count(); iConnect++ )
			{
				CNavArea *areaOther = (*pIncoming)[iConnect].area;

				// make sure there are not duplicate areas on the incoming list
				for ( int iConnectCheck = iConnect+1; iConnectCheck < pIncoming->Count(); iConnectCheck++ )
				{
					CNavArea *areaCheck = (*pIncoming)[iConnectCheck].area;
					if ( areaOther == areaCheck )
					{
						Msg( "Area %d has multiple incoming connections to area %d in direction %d\n", area->GetID(), areaOther->GetID(), dir );
						Assert( false );
					}
				}

				const NavConnectVector *pOutgoingOther = areaOther->GetAdjacentAreas( OppositeDirection( dir ) );
				connect.area = area;
				bool bOnOthersOutgoingList = pOutgoingOther->Find( connect ) != pOutgoingOther->InvalidIndex();
				if ( !bOnOthersOutgoingList )
				{
					Msg( "Area %d has incoming connection from area %d but does not appear on latter's outgoing connection list\n", area->GetID(), areaOther->GetID() );
					Assert( false );
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
* Temp way to mark cliff areas after generation without regen'ing.  Any area that is adjacent to a cliff
* gets marked as a cliff.  This will leave some big areas marked as cliff just because one edge is adjacent to 
* a cliff so it's not great.  The code that does this at generation time is better because it ensures that
* areas next to cliffs don't get merged with no-cliff areas.
*/
void CNavMesh::PostProcessCliffAreas()
{
	for ( int it = 0; it < TheNavAreas.Count(); it++ )
	{
		CNavArea *area = TheNavAreas[ it ];
		if ( area->GetAttributes() & NAV_MESH_CLIFF )
			continue;

		for ( int i = 0; i < NUM_DIRECTIONS; i++ )
		{
			bool bHasCliff = false;
			NavDirType dir = (NavDirType) i;
			NavCornerType corner[2];

			// look at either corner along this edge
			corner[0] = (NavCornerType) i;
			corner[1] = (NavCornerType) ( ( i+ 1 ) % NUM_CORNERS );

			for ( int j = 0; j < 2; j++ )
			{
				Vector cornerPos = area->GetCorner( corner[j] );
				if ( CheckCliff( &cornerPos, dir ) )
				{
					bHasCliff = true;
					break;
				}
			}
			
			if ( bHasCliff )
			{
				area->SetAttributes( area->GetAttributes() | NAV_MESH_CLIFF );
				break;
			}
		}
	}
}

CON_COMMAND_F( nav_gen_cliffs_approx, "Mark cliff areas, post-processing approximation", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->PostProcessCliffAreas();
}
