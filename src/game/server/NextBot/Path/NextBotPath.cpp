// NextBotPath.cpp
// Encapsulate and manipulate a path through the world
// Author: Michael Booth, February 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "nav_mesh.h"
#include "fmtstr.h"

#include "NextBotPath.h"
#include "NextBotInterface.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotUtil.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar NextBotPathDrawIncrement( "nb_path_draw_inc", "100", FCVAR_CHEAT );
ConVar NextBotPathDrawSegmentCount( "nb_path_draw_segment_count", "100", FCVAR_CHEAT );
ConVar NextBotPathSegmentInfluenceRadius( "nb_path_segment_influence_radius", "100", FCVAR_CHEAT );

//--------------------------------------------------------------------------------------------------------------
Path::Path( void )
{
	m_segmentCount = 0;
	
	m_cursorPos = 0.0f;
	m_isCursorDataDirty = true;
	m_cursorData.segmentPrior = NULL;
	m_ageTimer.Invalidate();
	m_subject = NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine actual path positions
 */
bool Path::ComputePathDetails( INextBot *bot, const Vector &start )
{
	VPROF_BUDGET( "Path::ComputePathDetails", "NextBot" );

	if (m_segmentCount == 0)
		return false;
		
	IBody *body = bot->GetBodyInterface();
	ILocomotion *mover = bot->GetLocomotionInterface();
	
	const float stepHeight = ( mover ) ? mover->GetStepHeight() : 18.0f;

	// inflate hull width slightly as a safety margin
	const float hullWidth = ( body ) ? body->GetHullWidth() + 5.0f : 1.0f;

	// set first path position
	if ( m_path[0].area->Contains( start ) )
	{
		m_path[0].pos = start;
	}
	else
	{
		// start in first area's center
		m_path[0].pos = m_path[0].area->GetCenter();
	}	
	m_path[0].ladder = NULL;
	m_path[0].how = NUM_TRAVERSE_TYPES;
	m_path[0].type = ON_GROUND;

	// set positions along the path
	for( int i=1; i<m_segmentCount; ++i )
	{
		Segment *from = &m_path[ i-1 ];
		Segment *to = &m_path[ i ];
		
		if ( to->how <= GO_WEST )		// walk along the floor to the next area
		{
			to->ladder = NULL;

			from->area->ComputePortal( to->area, (NavDirType)to->how, &to->m_portalCenter, &to->m_portalHalfWidth );

			// compute next point
			ComputeAreaCrossing( bot, from->area, from->pos, to->area, (NavDirType)to->how, &to->pos );

			// we need to walk out of "from" area, so keep Z where we can reach it
			to->pos.z = from->area->GetZ( to->pos );

			// if this is a "jump down" connection, we must insert an additional point on the path
			//float expectedHeightDrop = from->area->GetZ( from->pos ) - to->area->GetZ( to->pos );

			// measure the drop distance relative to the actual slope of the ground
			Vector fromPos = from->pos;
			fromPos.z = from->area->GetZ( fromPos );

			Vector toPos = to->pos;
			toPos.z = to->area->GetZ( toPos );

			Vector groundNormal;
			from->area->ComputeNormal( &groundNormal );

			Vector alongPath = toPos - fromPos;

			float expectedHeightDrop = -DotProduct( alongPath, groundNormal );

			if ( expectedHeightDrop > mover->GetStepHeight() )
			{
				// NOTE: We can't know this is a drop-down yet, because of subtle interactions
				// between nav area links and "portals" and "area crossings"

				// compute direction of path just prior to "jump down"
				Vector2D dir;
				DirectionToVector2D( (NavDirType)to->how, &dir );

				// shift top of "jump down" out a bit to "get over the ledge"
				const float inc = 10.0f; // 0.25f * hullWidth;
				const float maxPushDist = 2.0f * hullWidth; // 75.0f;
				float halfWidth = hullWidth/2.0f;
				float hullHeight = ( body ) ? body->GetCrouchHullHeight() : 1.0f;
				
				float pushDist;
				for( pushDist = 0.0f; pushDist <= maxPushDist; pushDist += inc )
				{
					Vector pos = to->pos + Vector( pushDist * dir.x, pushDist * dir.y, 0.0f );
					Vector lowerPos = Vector( pos.x, pos.y, toPos.z );
					
					trace_t result;
					NextBotTraceFilterIgnoreActors filter( bot->GetEntity(), COLLISION_GROUP_NONE );
					UTIL_TraceHull( pos, lowerPos,
									Vector( -halfWidth, -halfWidth, stepHeight ), Vector( halfWidth, halfWidth, hullHeight ), 
									bot->GetBodyInterface()->GetSolidMask(), &filter, &result );
					
					if ( result.fraction >= 1.0f )
					{
						// found clearance to drop
						break;
					}
				}
				
				Vector startDrop( to->pos.x + pushDist * dir.x, to->pos.y + pushDist * dir.y, to->pos.z );
				Vector endDrop( startDrop.x, startDrop.y, to->area->GetZ( to->pos ) );

				if ( bot->IsDebugging( NEXTBOT_PATH ) )
				{
					NDebugOverlay::Cross3D( startDrop, 5.0f, 255, 0, 255, true, 5.0f );
					NDebugOverlay::Cross3D( endDrop, 5.0f, 255, 255, 0, true, 5.0f );
					NDebugOverlay::VertArrow( startDrop, endDrop, 5.0f, 255, 100, 0, 255, true, 5.0f );
				}
				
				// verify that there is actually ground down there in case this is a far jump dropdown
				float ground;
				if ( TheNavMesh->GetGroundHeight( endDrop, &ground ) )
				{
					if ( startDrop.z > ground + stepHeight )
					{
						// if "ground" is lower than the next segment along the path
						// there is a chasm between - this is not a drop down
						// NOTE next->pos is not yet valid - this loop is computing it!
						// const Segment *next = NextSegment( to );
						// if ( !next || next->area->GetCenter().z < ground + stepHeight )
						{
							// this is a "jump down" link
							to->pos = startDrop;
							to->type = DROP_DOWN;

							// insert a duplicate node to represent the bottom of the fall
							if ( m_segmentCount < MAX_PATH_SEGMENTS-1 )
							{
								// copy nodes down
								for( int j=m_segmentCount; j>i; --j )
									m_path[j] = m_path[j-1];

								// path is one node longer
								++m_segmentCount;

								// move index ahead into the new node we just duplicated
								++i;

								m_path[i].pos.x = endDrop.x;
								m_path[i].pos.y = endDrop.y;
								m_path[i].pos.z = ground;
								
								m_path[i].type = ON_GROUND;
							}						
						}
					}
				}
			}
		}
		else if ( to->how == GO_LADDER_UP )		// to get to next area, must go up a ladder
		{
			// find our ladder
			const NavLadderConnectVector *ladders = from->area->GetLadders( CNavLadder::LADDER_UP );
			int it;
			for( it=0; it<ladders->Count(); ++it )
			{
				CNavLadder *ladder = (*ladders)[ it ].ladder;
				
				// can't use "behind" area when ascending...
				if (ladder->m_topForwardArea == to->area ||
					ladder->m_topLeftArea == to->area ||
					ladder->m_topRightArea == to->area)
				{
					to->ladder = ladder;
					to->pos = ladder->m_bottom + ladder->GetNormal() * 2.0f * HalfHumanWidth;
					to->type = LADDER_UP;
					break;
				}
			}
			
			if (it == ladders->Count())
			{
				//PrintIfWatched( "ERROR: Can't find ladder in path\n" );
				return false;
			}
		}
		else if ( to->how == GO_LADDER_DOWN )		// to get to next area, must go down a ladder
		{
			// find our ladder
			const NavLadderConnectVector *ladders = from->area->GetLadders( CNavLadder::LADDER_DOWN );
			int it;
			for( it=0; it<ladders->Count(); ++it )
			{
				CNavLadder *ladder = (*ladders)[ it ].ladder;

				if (ladder->m_bottomArea == to->area)
				{
					to->ladder = ladder;
					to->pos = ladder->m_top;
					to->pos = ladder->m_top - ladder->GetNormal() * 2.0f * HalfHumanWidth;
					to->type = LADDER_DOWN;
					break;
				}
			}

			if (it == ladders->Count())
			{
				//PrintIfWatched( "ERROR: Can't find ladder in path\n" );
				return false;
			}
		}
		else if ( to->how == GO_ELEVATOR_UP || to->how == GO_ELEVATOR_DOWN )
		{
			to->pos = to->area->GetCenter();
			to->ladder = NULL;
		}
	}


	//
	// Scan for non-adjacent nav areas and add gap-jump-target nodes
	// and jump-up target nodes for adjacent ledge mantling
	// @todo Adjacency should be baked into the mesh data
	//
	for( int i=0; i<m_segmentCount-1; ++i )
	{
		Segment *from = &m_path[ i ];
		Segment *to = &m_path[ i+1 ];
		
		// first segment doesnt have a direction
		if ( from->how != NUM_TRAVERSE_TYPES && from->how > GO_WEST )
			continue;
		
		if ( to->how > GO_WEST || !to->type == ON_GROUND )
			continue;

		// if areas are separated, we may need to 'gap jump' between them
		// add a node to minimize the jump distance
		Vector closeFrom, closeTo;
		to->area->GetClosestPointOnArea( from->pos, &closeTo );
		from->area->GetClosestPointOnArea( closeTo, &closeFrom );
		
		if ( bot->IsDebugging( NEXTBOT_PATH ) )
		{
			NDebugOverlay::Line( closeFrom, closeTo, 255, 0, 255, true, 5.0f );
		}


		const float separationTolerance = 1.9f * GenerationStepSize;
		if ( (closeFrom - closeTo).AsVector2D().IsLengthGreaterThan( separationTolerance ) && ( closeTo - closeFrom ).AsVector2D().IsLengthGreaterThan( 0.5f * fabs( closeTo.z - closeFrom.z ) ) )
		{
			// areas are disjoint and mostly level - add gap jump target

			// compute landing spot in 'to' area			
			Vector landingPos;
			to->area->GetClosestPointOnArea( to->pos, &landingPos );

			// compute launch spot in 'from' area			
			Vector launchPos;
			from->area->GetClosestPointOnArea( landingPos, &launchPos );

			Vector forward = landingPos - launchPos;
			forward.NormalizeInPlace();
			
			const float halfWidth = hullWidth/2.0f;

			// adjust path position to landing spot
			to->pos = landingPos + forward * halfWidth;
			
			// insert launch position just before that segment to ensure bot is 
			// positioned for minimal jump distance
			Segment newSegment = *from;

			newSegment.pos = launchPos - forward * halfWidth;
			newSegment.type = JUMP_OVER_GAP;

			InsertSegment( newSegment, i+1 );
			
			++i;
		}
		else if ( (closeTo.z - closeFrom.z) > stepHeight )
		{
			// areas are adjacent, but need a jump-up - add a jump-to target
			
			// adjust goal to be at top of ledge
			//to->pos.z = to->area->GetZ( to->pos.x, to->pos.y );
			// use center of climb-up destination area to make sure bot moves onto actual ground once they finish their climb
			to->pos = to->area->GetCenter();
			
			// add launch position at base of jump	
			Segment newSegment = *from;
			
			Vector launchPos;
			from->area->GetClosestPointOnArea( to->pos, &launchPos );
			
			newSegment.pos = launchPos;
			newSegment.type = CLIMB_UP;

			if ( bot->IsDebugging( NEXTBOT_PATH ) )
			{
				NDebugOverlay::Cross3D( newSegment.pos, 15.0f, 255, 100, 255, true, 3.0f );
			}

			InsertSegment( newSegment, i+1 );

			++i;			
		}

		/** RETHINK THIS.  It doesn't work in general cases, and messes up on doorways
		else if ( from->type == ON_GROUND && from->how <= GO_WEST )
		{
			// if any segment is not directly walkable, add a segment
			// fixup corners that are being cut too tightly
			if ( mover && !mover->IsPotentiallyTraversable( from->pos, to->pos ) )
			{
				Segment newSegment = *from;
				
				if ( bot->IsDebugging( INextBot::PATH ) )
				{
					NDebugOverlay::HorzArrow( from->pos, to->pos, 3.0f, 255, 0, 0, 255, true, 3.0f );
				}

				//newSegment.pos = from->area->GetCenter();
				
				Vector2D shift;
				DirectionToVector2D( OppositeDirection( (NavDirType)to->how ), &shift );
												
				newSegment.pos = to->pos;
				newSegment.pos.x += hullWidth * shift.x;
				newSegment.pos.y += hullWidth * shift.y;
				
				newSegment.type = ON_GROUND;

				if ( bot->IsDebugging( INextBot::PATH ) )
				{
					NDebugOverlay::Cross3D( newSegment.pos, 15.0f, 255, 0, 255, true, 3.0f );
				}

				InsertSegment( newSegment, i+1 );
				
				i += 2;
			}
		}
		*/
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Insert new segment at index i
 */
void Path::InsertSegment( Segment newSegment, int i )
{
	if (m_segmentCount < MAX_PATH_SEGMENTS-1)
	{
		// shift segments to make room for new one
		for( int j=m_segmentCount; j>i; --j )
			m_path[j] = m_path[j-1];

		// path is one node longer
		++m_segmentCount;

		m_path[i] = newSegment;
	}										
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Build trivial path when start and goal are in the same nav area
 */
bool Path::BuildTrivialPath( INextBot *bot, const Vector &goal )
{
	const Vector &start = bot->GetPosition();
	
	m_segmentCount = 0;

	/// @todo Dangerous to use "nearset" nav area - could be far away
	CNavArea *startArea = TheNavMesh->GetNearestNavArea( start );
	if (startArea == NULL)
		return false;

	CNavArea *goalArea = TheNavMesh->GetNearestNavArea( goal );
	if (goalArea == NULL)
		return false;

	m_segmentCount = 2;

	m_path[0].area = startArea;
	m_path[0].pos.x = start.x;
	m_path[0].pos.y = start.y;
	m_path[0].pos.z = startArea->GetZ( start );
	m_path[0].ladder = NULL;
	m_path[0].how = NUM_TRAVERSE_TYPES;
	m_path[0].type = ON_GROUND;

	m_path[1].area = goalArea;
	m_path[1].pos.x = goal.x;
	m_path[1].pos.y = goal.y;
	m_path[1].pos.z = goalArea->GetZ( goal );
	m_path[1].ladder = NULL;
	m_path[1].how = NUM_TRAVERSE_TYPES;
	m_path[1].type = ON_GROUND;

	m_path[0].forward = m_path[1].pos - m_path[0].pos;
	m_path[0].length = m_path[0].forward.NormalizeInPlace();
	m_path[0].distanceFromStart = 0.0f;
	m_path[0].curvature = 0.0f;
	
	m_path[1].forward = m_path[0].forward;
	m_path[1].length = 0.0f;
	m_path[1].distanceFromStart = m_path[0].length;
	m_path[1].curvature = 0.0f;

	OnPathChanged( bot, COMPLETE_PATH );

	return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw the path for debugging.
 */
void Path::Draw( const Path::Segment *start ) const
{
	if ( !IsValid() )
		return;

	CFmtStr msg;

	// limit length of path we draw
	int count = NextBotPathDrawSegmentCount.GetInt();

	const Segment *s = start ? start : FirstSegment();
	int i=0;
	while( s && count-- )
	{
		const Segment *next = NextSegment( s );
		if ( next == NULL )
		{
			// end of the path
			break;
		}

		Vector to = next->pos - s->pos;
		float horiz = MAX( abs(to.x), abs(to.y) );
		float vert = abs( to.z );

		int r,g,b;
		switch( s->type )
		{
			case DROP_DOWN:			r = 255; g =   0; b = 255; break;
			case CLIMB_UP:			r =   0; g =   0; b = 255; break;
			case JUMP_OVER_GAP:		r =   0; g = 255; b = 255; break;
			case LADDER_UP:			r =   0; g = 255; b =   0; break;
			case LADDER_DOWN:		r =   0; g = 100; b =   0; break;
			default:				r = 255; g =  77; b =   0; break;		// ON_GROUND
		}

		if ( s->ladder )
		{
			NDebugOverlay::VertArrow( s->ladder->m_bottom, s->ladder->m_top, 5.0f, r, g, b, 255, true, 0.1f );
		}
		else
		{
			NDebugOverlay::Line( s->pos, next->pos, r, g, b, true, 0.1f );		
		}

		const float nodeLength = 25.0f;
		if ( horiz > vert )
		{
			NDebugOverlay::HorzArrow( s->pos, s->pos + nodeLength * s->forward, 5.0f, r, g, b, 255, true, 0.1f );
		}
		else
		{
			NDebugOverlay::VertArrow( s->pos, s->pos + nodeLength * s->forward, 5.0f, r, g, b, 255, true, 0.1f );
		}

		NDebugOverlay::Text( s->pos, msg.sprintf( "%d", i ), true, 0.1f );

		//NDebugOverlay::Text( s->pos, msg.sprintf( "%d  (%3.2f)", i, s->curvature ), false, 0.1f );

		s = next;
		++i;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw the path for debugging - MODIFIES cursor position
 */
void Path::DrawInterpolated( float from, float to )
{
	if ( !IsValid() )
	{
		return;
	}
	
	float t = from;

	MoveCursor( t );
	const Data &data = GetCursorData();
	Vector lastPos = data.pos;	

	do
	{
		t += NextBotPathDrawIncrement.GetFloat();

		MoveCursor( t );
		const Data &data = GetCursorData();
		
		float curvePower = 3.0f * data.curvature;

		int r = 255 * ( 1.0f - curvePower );
		r = clamp( r, 0, 255 );
		
		int g = 255 * ( 1.0f + curvePower );
		g = clamp( g, 0, 255 );
				
		NDebugOverlay::Line( lastPos, data.pos, r, g, 0, true, 0.1f );

		/*
		int i = 0xFF & (int)( data.pos.x + data.pos.y + data.pos.z );
		i >>= 1;
		i += 128;
		
		NDebugOverlay::Line( data.pos, data.pos + 10.0f * data.forward, 0, i, i, true, 0.1f );
		*/

		lastPos = data.pos;
	}
	while( t < to );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check line of sight from 'anchor' node on path to subsequent nodes until
 * we find a node that can't been seen from 'anchor'.
 */
int Path::FindNextOccludedNode( INextBot *bot, int anchorIndex )
{
	ILocomotion *mover = bot->GetLocomotionInterface();
	if ( mover == NULL)
	{
		return m_segmentCount;
	}
	
	Segment *anchor = &m_path[ anchorIndex ];
	
	for( int i=anchorIndex+1; i<m_segmentCount; ++i )
	{
		Segment *to = &m_path[i];
		
		// if this segment is not on the ground, or is precise, don't skip past it
		if ( !to->type == ON_GROUND || (to->area->GetAttributes() & NAV_MESH_PRECISE) )
		{
			return i;
		}

		if ( !mover->IsPotentiallyTraversable( anchor->pos, to->pos, ILocomotion::IMMEDIATELY ) )
		{
			// cant reach this node directly from anchor node
			return i;
		}

		if ( mover->HasPotentialGap( anchor->pos, to->pos ) )
		{
			// we would fall into a gap if we took this cutoff
			return i;
		}
	}

	return m_segmentCount;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Smooth out path, removing redundant nodes
 */
void Path::Optimize( INextBot *bot )
{
	// this is SUPER expensive - especially the IsGap() check
	return;

	VPROF_BUDGET( "Path::Optimize", "NextBot" );

	if (m_segmentCount < 3)
		return;

	int anchor = 0;

	while( anchor < m_segmentCount )
	{
		int occluded = FindNextOccludedNode( bot, anchor );
		int nextAnchor = occluded-1;

		if (nextAnchor > anchor)
		{
			// remove redundant nodes between anchor and nextAnchor
			int removeCount = nextAnchor - anchor - 1;
			if (removeCount > 0)
			{
				for( int i=nextAnchor; i<m_segmentCount; ++i )
				{
					m_path[i-removeCount] = m_path[i];
				}
				m_segmentCount -= removeCount;
			}
		}

		++anchor;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute final data for completed path
 */
void Path::PostProcess( void )
{
	VPROF_BUDGET( "Path::PostProcess", "NextBot" );

	m_ageTimer.Start();

	if (m_segmentCount == 0)
		return;
		
	if (m_segmentCount == 1)
	{
		m_path[0].forward = vec3_origin;
		m_path[0].length = 0.0f;
		m_path[0].distanceFromStart = 0.0f;
		m_path[0].curvature = 0.0f;
		return;
	}
	
	float distanceSoFar = 0.0f;
	int i;
	for( i=0; i < m_segmentCount-1; ++i )
	{
		Segment *from = &m_path[ i ];
		Segment *to = &m_path[ i+1 ];
		
		from->forward = to->pos - from->pos;
		from->length = from->forward.NormalizeInPlace();
		
		from->distanceFromStart = distanceSoFar;

		distanceSoFar += from->length;
	}
	
		
	// compute curvature in XY plane
	Vector2D from, to;
	for( i=1; i < m_segmentCount-1; ++i )
	{
		if (m_path[ i ].type != ON_GROUND)
		{
			m_path[ i ].curvature = 0.0f;
		}
		else
		{
			from = m_path[ i-1 ].forward.AsVector2D();
			from.NormalizeInPlace();
			
			to = m_path[ i ].forward.AsVector2D();
			to.NormalizeInPlace();	
		
			m_path[ i ].curvature = 0.5f * ( 1.0f - from.Dot( to ) );
			
			Vector2D right( -from.y, from.x );
			if ( to.Dot( right ) < 0.0f )
			{
				m_path[ i ].curvature = -m_path[ i ].curvature;
			}
		}
	}

	// first segment has no curvature
	m_path[ 0 ].curvature = 0.0f;
	
	// last segment maintains direction
	m_path[ i ].forward = m_path[ i-1 ].forward;
	m_path[ i ].length = 0.0f;
	m_path[ i ].distanceFromStart = distanceSoFar;
	m_path[ i ].curvature = 0.0f;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return a position on the path at the given distance from the path start
 */
const Vector &Path::GetPosition( float distanceFromStart, const Segment *start ) const
{
	if (!IsValid())
	{
		return vec3_origin;
	}

	float lengthSoFar;
	const Segment *segment;
	
	if (start)
	{
		segment = start;
		lengthSoFar = start->distanceFromStart;
	}
	else
	{
		segment = &m_path[0];
		lengthSoFar = 0.0f;
	}

	if (segment->distanceFromStart > distanceFromStart)
	{
		// clamp to path start
		return segment->pos;
	}

	
	const Segment *next = NextSegment( segment );

	Vector delta;
	float length;

	while( next )
	{
		delta = next->pos - segment->pos;
		length = segment->length;

		if (lengthSoFar + length >= distanceFromStart)
		{
			// desired point is on this segment of the path
			float overlap = distanceFromStart - lengthSoFar;
			float t = overlap / length;

			m_pathPos = segment->pos + t * delta;
			
			return m_pathPos;
		}

		lengthSoFar += length;
		
		segment = next;
		next = NextSegment( next );
	}

	// clamp to path end
	return segment->pos;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the closest point on the path to the given position
 */
const Vector &Path::GetClosestPosition( const Vector &pos, const Segment *start, float alongLimit ) const
{
	const Segment *segment = (start) ? start : &m_path[0];
	
	if (segment == NULL)
	{
		return pos;
	}
	
	m_closePos = pos;
	float closeRangeSq = 99999999999.9f;

	float distanceSoFar = 0.0f;	
	while( alongLimit == 0.0f || distanceSoFar <= alongLimit )
	{
		const Segment *nextSegment = NextSegment( segment );
		
		if (nextSegment)
		{
			Vector close;
			CalcClosestPointOnLineSegment( pos, segment->pos, nextSegment->pos, close );
			float rangeSq = (close - pos).LengthSqr();
			if (rangeSq < closeRangeSq)
			{	
				m_closePos = close;
				closeRangeSq = rangeSq;
			}
		}
		else
		{
			// end of the path
			break;
		}
		
		distanceSoFar += segment->length;
		segment = nextSegment;
	}
	
	return m_closePos;
}



//--------------------------------------------------------------------------------------------------------------
/**
 * Replace this path with the given path's data
 */
void Path::Copy( INextBot *bot, const Path &path )
{
	VPROF_BUDGET( "Path::Copy", "NextBot" );

	Invalidate();
	
	for( int i = 0; i < path.m_segmentCount; ++i )
	{
		m_path[i] = path.m_path[i];
	}
	m_segmentCount = path.m_segmentCount;

	OnPathChanged( bot, COMPLETE_PATH );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Set cursor position to closest point on path to given position
 */
void Path::MoveCursorToClosestPosition( const Vector &pos, SeekType type, float alongLimit ) const
{
	if ( !IsValid() )
	{
		return;
	}
	
	if ( type == SEEK_ENTIRE_PATH || type == SEEK_AHEAD )
	{
		const Segment *segment;
		
		if ( type == SEEK_AHEAD )
		{
			// continue search from cursor position onward
			if ( m_cursorData.segmentPrior )
			{
				segment = m_cursorData.segmentPrior;
			}
			else
			{
				// no prior segment, start from the start
				segment = &m_path[ 0 ];
			}
		}
		else
		{
			// search entire path from the start
			segment = &m_path[ 0 ];
		}

		m_cursorData.pos = pos;
		m_cursorData.segmentPrior = segment;
		float closeRangeSq = 99999999999.9f;

		float distanceSoFar = 0.0f;	
		while( alongLimit == 0.0f || distanceSoFar <= alongLimit )
		{
			const Segment *nextSegment = NextSegment( segment );

			if ( nextSegment )
			{
				Vector close;
				CalcClosestPointOnLineSegment( pos, segment->pos, nextSegment->pos, close );
				
				float rangeSq = ( close - pos ).LengthSqr();
				if ( rangeSq < closeRangeSq )
				{	
					m_cursorData.pos = close;
					m_cursorData.segmentPrior = segment;
					
					closeRangeSq = rangeSq;
				}
			}
			else
			{
				// end of the path
				break;
			}

			distanceSoFar += segment->length;
			segment = nextSegment;
		}

		//
		// Move cursor to closest point on path
		//
		segment = m_cursorData.segmentPrior;
				
		float t = ( m_cursorData.pos - segment->pos ).Length() / segment->length;

		m_cursorPos = segment->distanceFromStart + t * segment->length;	
		m_isCursorDataDirty = true;
	}
	else
	{
		AssertMsg( false, "SEEK_BEHIND not implemented" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return path state at the current cursor position
 */
const Path::Data &Path::GetCursorData( void ) const
{
	if ( IsValid() )
	{
		if ( m_isCursorDataDirty )
		{
			const float epsilon = 0.0001f;
			if ( m_cursorPos < epsilon || m_segmentCount < 2 )
			{
				// start of path
				m_cursorData.pos = m_path[0].pos;
				m_cursorData.forward = m_path[0].forward;
				m_cursorData.curvature = m_path[0].curvature;
				m_cursorData.segmentPrior = &m_path[0];
			}
			else if ( m_cursorPos > GetLength() - epsilon )
			{
				// end of path
				m_cursorData.pos = m_path[ m_segmentCount-1 ].pos;
				m_cursorData.forward = m_path[ m_segmentCount-1 ].forward;
				m_cursorData.curvature = m_path[ m_segmentCount-1 ].curvature;
				m_cursorData.segmentPrior = &m_path[ m_segmentCount-1 ];
			}
			else
			{
				// along path
				float lengthSoFar = 0.0f;
				const Segment *segment = &m_path[0];

				const Segment *next = NextSegment( segment );

				while( next )
				{
					float length = segment->length;

					if ( lengthSoFar + length >= m_cursorPos )
					{
						// desired point is on this segment of the path
						float overlap = m_cursorPos - lengthSoFar;
						float t = 1.0f;	// 0-length segments are assumed to be complete, to avoid NaNs
						if ( length > 0.0f )
						{
							t = overlap / length;
						}
						
						// interpolate data at this point along the path
						m_cursorData.pos = segment->pos + t * ( next->pos - segment->pos );
						m_cursorData.forward = segment->forward + t * ( next->forward - segment->forward );
						m_cursorData.segmentPrior = segment;

						// curvature fades to zero along midpoint of long straight segments
						// and is influenced as it nears ends of segment
						if ( overlap < NextBotPathSegmentInfluenceRadius.GetFloat() )
						{
							if ( length - overlap < NextBotPathSegmentInfluenceRadius.GetFloat() )
							{
								// near start and end - interpolate
								float startCurvature = segment->curvature * ( 1.0f - ( overlap / NextBotPathSegmentInfluenceRadius.GetFloat() ) );
								float endCurvature = next->curvature * ( 1.0f - ( ( length - overlap ) / NextBotPathSegmentInfluenceRadius.GetFloat() ) );

								m_cursorData.curvature = ( startCurvature + endCurvature ) / 2.0f;
							}
							else
							{
								// near start only
								m_cursorData.curvature = segment->curvature * ( 1.0f - ( overlap / NextBotPathSegmentInfluenceRadius.GetFloat() ) );
							}
						}
						else if ( length - overlap < NextBotPathSegmentInfluenceRadius.GetFloat() )
						{
							// near end only
							m_cursorData.curvature = next->curvature * ( 1.0f - ( ( length - overlap ) / NextBotPathSegmentInfluenceRadius.GetFloat() ) );
						}

						
						break;
					}

					lengthSoFar += length;

					segment = next;
					next = NextSegment( next );
				}
			}
			
			// data is up to date
			m_isCursorDataDirty = false;
		}
	}
	else
	{
		// path is not valid
		m_cursorData.pos = vec3_origin;
		m_cursorData.forward = Vector( 1.0f, 0, 0 );
		m_cursorData.curvature = 0.0f;
		m_cursorData.segmentPrior = NULL;
	}
	
	return m_cursorData;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine exactly where the path goes between the given two areas
 * on the path. Return this point in 'crossPos'.
 */
void Path::ComputeAreaCrossing( INextBot *bot, const CNavArea *from, const Vector &fromPos, const CNavArea *to, NavDirType dir, Vector *crossPos ) const
{
	from->ComputeClosestPointInPortal( to, dir, fromPos, crossPos );

	// move goal position into the goal area a bit to avoid running directly along the edge of an area against a wall, etc
	// don't do this unless area is against a wall - and what if our hull is wider than the area?
	// AddDirectionVector( crossPos, dir, bot->GetBodyInterface()->GetHullWidth()/2.0f );
}
































