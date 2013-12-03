//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_edit.cpp
// Implementation of Navigation Mesh edit mode
// Author: Michael Booth, 2003-2004

#include "cbase.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_node.h"
#include "nav_colors.h"
#include "Color.h"
#include "tier0/vprof.h"
#include "collisionutils.h"
#include "world.h"
#include "functorutils.h"
#include "team.h"
#ifdef TERROR
#include "TerrorShared.h"
#endif

#ifdef DOTA_SERVER_DLL
#include "dota_npc_base.h"
#include "dota_player.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


ConVar nav_show_area_info( "nav_show_area_info", "0.5", FCVAR_CHEAT, "Duration in seconds to show nav area ID and attributes while editing" );
ConVar nav_snap_to_grid( "nav_snap_to_grid", "0", FCVAR_CHEAT, "Snap to the nav generation grid when creating new nav areas" );
ConVar nav_create_place_on_ground( "nav_create_place_on_ground", "0", FCVAR_CHEAT, "If true, nav areas will be placed flush with the ground when created by hand." );
#ifdef DEBUG
ConVar nav_draw_limit( "nav_draw_limit", "50", FCVAR_CHEAT, "The maximum number of areas to draw in edit mode" );
#else
ConVar nav_draw_limit( "nav_draw_limit", "500", FCVAR_CHEAT, "The maximum number of areas to draw in edit mode" );
#endif
ConVar nav_solid_props( "nav_solid_props", "0", FCVAR_CHEAT, "Make props solid to nav generation/editing" );
ConVar nav_create_area_at_feet( "nav_create_area_at_feet", "0", FCVAR_CHEAT, "Anchor nav_begin_area Z to editing player's feet" );

ConVar nav_drag_selection_volume_zmax_offset( "nav_drag_selection_volume_zmax_offset", "32", FCVAR_REPLICATED, "The offset of the nav drag volume top from center" );
ConVar nav_drag_selection_volume_zmin_offset( "nav_drag_selection_volume_zmin_offset", "32", FCVAR_REPLICATED, "The offset of the nav drag volume bottom from center" );

extern void GetNavUIEditVectors( Vector *pos, Vector *forward );

Color s_dragSelectionSetAddColor( 100, 255, 100, 96 );
Color s_dragSelectionSetDeleteColor( 255, 100, 100, 96 );

#if DEBUG_NAV_NODES
extern ConVar nav_show_nodes;
#endif // DEBUG_NAV_NODES


//--------------------------------------------------------------------------------------------------------------
int GetGridSize( bool forceGrid = false )
{
	if ( TheNavMesh->IsGenerating() )
	{
		return (int)GenerationStepSize;
	}

	int snapVal = nav_snap_to_grid.GetInt();
	if ( forceGrid && !snapVal )
	{
		snapVal = 1;
	}

	if ( snapVal == 0 )
	{
		return 0;
	}

	int scale = (int)GenerationStepSize;
	switch ( snapVal )
	{
	case 3:
		scale = 1;
		break;
	case 2:
		scale = 5;
		break;
	case 1:
	default:
		break;
	}

	return scale;
}


//--------------------------------------------------------------------------------------------------------------
Vector CNavMesh::SnapToGrid( const Vector& in, bool snapX, bool snapY, bool forceGrid ) const
{
	int scale = GetGridSize( forceGrid );
	if ( !scale )
	{
		return in;
	}

	Vector out( in );

	if ( snapX )
	{
		out.x = RoundToUnits( in.x, scale );
	}

	if ( snapY )
	{
		out.y = RoundToUnits( in.y, scale );
	}

	return out;
}


//--------------------------------------------------------------------------------------------------------------
float CNavMesh::SnapToGrid( float x, bool forceGrid ) const
{
	int scale = GetGridSize( forceGrid );
	if ( !scale )
	{
		return x;
	}

	x = RoundToUnits( x, scale );
	return x;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::GetEditVectors( Vector *pos, Vector *forward )
{
	if ( !pos || !forward )
	{
		return;
	}

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( !player )
	{
		return;
	}

	//DOTA places the edit cursor where the 2D cursor is located.
#ifdef DOTA_SERVER_DLL
	CDOTAPlayer *pDOTAPlayer = ToDOTAPlayer( player );

	if ( pDOTAPlayer && pDOTAPlayer->GetMoveType() != MOVETYPE_NOCLIP )
	{
		Vector dir = pDOTAPlayer->GetCrosshairTracePos() - player->EyePosition();
		VectorNormalize( dir );

		*forward = dir;
	}
	else
	{
		AngleVectors( player->EyeAngles() + player->GetPunchAngle(), forward );
	}
#else
	Vector dir;
	AngleVectors( player->EyeAngles() + player->GetPunchAngle(), forward );
#endif

	*pos = player->EyePosition();

#ifdef SERVER_USES_VGUI
//	GetNavUIEditVectors( pos, forward );
#endif // SERVER_USES_VGUI
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Change the edit mode
 */
void CNavMesh::SetEditMode( EditModeType mode )
{
	m_markedLadder = NULL;
	m_markedArea = NULL;
	m_markedCorner = NUM_CORNERS;
	
	m_editMode = mode;

	m_isContinuouslySelecting = false;
	m_isContinuouslyDeselecting = false;
	m_bIsDragDeselecting = false;
}


//--------------------------------------------------------------------------------------------------------------
bool CNavMesh::FindNavAreaOrLadderAlongRay( const Vector &start, const Vector &end, CNavArea **bestArea, CNavLadder **bestLadder, CNavArea *ignore )
{
	if ( !m_grid.Count() )
		return false;

	Ray_t ray;
	ray.Init( start, end, vec3_origin, vec3_origin );

	*bestArea = NULL;
	*bestLadder = NULL;

	float bestDist = 1.0f; // 0..1 fraction

	for ( int i=0; i<m_ladders.Count(); ++i )
	{
		CNavLadder *ladder = m_ladders[i];

		Vector left( 0, 0, 0), right(0, 0, 0), up( 0, 0, 0);
		VectorVectors( ladder->GetNormal(), right, up );
		right *= ladder->m_width * 0.5f;
		left = -right;

		Vector c1 = ladder->m_top + right;
		Vector c2 = ladder->m_top + left;
		Vector c3 = ladder->m_bottom + right;
		Vector c4 = ladder->m_bottom + left;
		float dist = IntersectRayWithTriangle( ray, c1, c2, c4, false );
		if ( dist > 0 && dist < bestDist )
		{
			*bestLadder = ladder;
			bestDist = dist;
		}

		dist = IntersectRayWithTriangle( ray, c1, c4, c3, false );
		if ( dist > 0 && dist < bestDist )
		{
			*bestLadder = ladder;
			bestDist = dist;
		}
	}

	Extent extent;
	extent.lo = extent.hi = start;
	extent.Encompass( end );

	int loX = WorldToGridX( extent.lo.x );
	int loY = WorldToGridY( extent.lo.y );
	int hiX = WorldToGridX( extent.hi.x );
	int hiY = WorldToGridY( extent.hi.y );

	for( int y = loY; y <= hiY; ++y )
	{
		for( int x = loX; x <= hiX; ++x )
		{
			NavAreaVector &areaGrid = m_grid[ x + y*m_gridSizeX ];

			FOR_EACH_VEC( areaGrid, it )
			{
				CNavArea *area = areaGrid[ it ];
				if ( area == ignore )
					continue;

				Vector nw = area->m_nwCorner;
				Vector se = area->m_seCorner;
				Vector ne, sw;
				ne.x = se.x;
				ne.y = nw.y;
				ne.z = area->m_neZ;
				sw.x = nw.x;
				sw.y = se.y;
				sw.z = area->m_swZ;

				float dist = IntersectRayWithTriangle( ray, nw, ne, se, false );
				if ( dist > 0 && dist < bestDist )
				{
					*bestArea = area;
					bestDist = dist;
				}

				dist = IntersectRayWithTriangle( ray, se, sw, nw, false );
				if ( dist > 0 && dist < bestDist )
				{
					*bestArea = area;
					bestDist = dist;
				}
			}
		}
	}

	if ( *bestArea )
	{
		*bestLadder = NULL;
	}

	return bestDist < 1.0f;
}


//--------------------------------------------------------------------------------------------------------------
/**
 *  Convenience function to find the nav area a player is looking at, for editing commands
 */
bool CNavMesh::FindActiveNavArea( void )
{
	VPROF( "CNavMesh::FindActiveNavArea" );

	m_splitAlongX = false;
	m_splitEdge = 0.0f;
	m_selectedArea = NULL;
	m_climbableSurface = false;
	m_selectedLadder = NULL;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return false;

	Vector from, dir;
	GetEditVectors( &from, &dir );

	float maxRange = 2000.0f;		// 500
	bool isClippingRayAtFeet = false;
	if ( nav_create_area_at_feet.GetBool() )
	{
		if ( dir.z < 0 )
		{
			float eyeHeight = player->GetViewOffset().z;
			if ( eyeHeight != 0.0f )
			{
				float rayHeight = -dir.z * maxRange;
				maxRange = maxRange * eyeHeight / rayHeight;
				isClippingRayAtFeet = true;
			}
		}
	}

	Vector to = from + maxRange * dir;

	trace_t result;
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
	UTIL_TraceLine( from, to, (nav_solid_props.GetBool()) ? MASK_NPCSOLID : MASK_NPCSOLID_BRUSHONLY, &filter, &result );

	if (result.fraction != 1.0f)
	{
		if ( !IsEditMode( CREATING_AREA ) )
		{
			m_climbableSurface = physprops->GetSurfaceData( result.surface.surfaceProps )->game.climbable != 0;
			if ( !m_climbableSurface )
			{
				m_climbableSurface = (result.contents & CONTENTS_LADDER) != 0;
			}
			m_surfaceNormal = result.plane.normal;

			if ( m_climbableSurface )
			{
				// check if we're on the same plane as the original point when we're building a ladder
				if ( IsEditMode( CREATING_LADDER ) )
				{
					if ( m_surfaceNormal != m_ladderNormal )
					{
						m_climbableSurface = false;
					}
				}

				if ( m_surfaceNormal.z > 0.9f )
				{
					m_climbableSurface = false; // don't try to build ladders on flat ground
				}
			}
		}

		if ( ( m_climbableSurface && !IsEditMode( CREATING_LADDER ) ) || !IsEditMode( CREATING_AREA ) )
		{
			float closestDistSqr = 200.0f * 200.0f;

			for ( int i=0; i<m_ladders.Count(); ++i )
			{
				CNavLadder *ladder = m_ladders[i];

				Vector absMin = ladder->m_bottom;
				Vector absMax = ladder->m_top;

				Vector left( 0, 0, 0), right(0, 0, 0), up( 0, 0, 0);
				VectorVectors( ladder->GetNormal(), right, up );
				right *= ladder->m_width * 0.5f;
				left = -right;

				absMin.x += MIN( left.x, right.x );
				absMin.y += MIN( left.y, right.y );

				absMax.x += MAX( left.x, right.x );
				absMax.y += MAX( left.y, right.y );

				Extent e;
				e.lo = absMin + Vector( -5, -5, -5 );
				e.hi = absMax + Vector( 5, 5, 5 );

				if ( e.Contains( m_editCursorPos ) )
				{
					m_selectedLadder = ladder;
					break;
				}

				if ( !m_climbableSurface )
					continue;

				Vector p1 = (ladder->m_bottom + ladder->m_top)/2;
				Vector p2 = m_editCursorPos;
				float distSqr = p1.DistToSqr( p2 );

				if ( distSqr < closestDistSqr )
				{
					m_selectedLadder = ladder;
					closestDistSqr = distSqr;
				}
			}
		}

		m_editCursorPos = result.endpos;

		// find the area the player is pointing at
		if ( !m_climbableSurface && !m_selectedLadder )
		{
			// Try to clip our trace to nav areas
			FindNavAreaOrLadderAlongRay( result.startpos, result.endpos + 100.0f * dir, &m_selectedArea, &m_selectedLadder ); // extend a few units into the ground

			// Failing that, get the closest area to the endpoint
			if ( !m_selectedArea && !m_selectedLadder )
			{
				m_selectedArea = TheNavMesh->GetNearestNavArea( result.endpos, false, 500.0f );
			}
		}

		if ( m_selectedArea )
		{
			float yaw = player->EyeAngles().y;
			while( yaw > 360.0f )
				yaw -= 360.0f;

			while( yaw < 0.0f )
				yaw += 360.0f;

			if ((yaw < 45.0f || yaw > 315.0f) || (yaw > 135.0f && yaw < 225.0f))
			{
				m_splitEdge = SnapToGrid( result.endpos.y, true );
				m_splitAlongX = true;
			}
			else
			{
				m_splitEdge = SnapToGrid( result.endpos.x, true );
				m_splitAlongX = false;
			}
		}

		if ( !m_climbableSurface && !IsEditMode( CREATING_LADDER ) )
		{
			m_editCursorPos = SnapToGrid( m_editCursorPos );
		}

		return true;
	}
	else if ( isClippingRayAtFeet )
	{
		m_editCursorPos = SnapToGrid( result.endpos );
	}

	if ( IsEditMode( CREATING_LADDER ) || IsEditMode( CREATING_AREA ) )
		return false;

	// We started solid.  Look for areas in front of us.
	FindNavAreaOrLadderAlongRay( from, to, &m_selectedArea, &m_selectedLadder );

	return (m_selectedArea != NULL || m_selectedLadder != NULL || isClippingRayAtFeet);
}


//--------------------------------------------------------------------------------------------------------------
bool CNavMesh::FindLadderCorners( Vector *corner1, Vector *corner2, Vector *corner3 )
{
	if ( !corner1 || !corner2 || !corner3 )
		return false;

	Vector ladderRight, ladderUp;
	VectorVectors( m_ladderNormal, ladderRight, ladderUp );

	Vector from, dir;
	GetEditVectors( &from, &dir );

	const float maxDist = 100000.0f;

	Ray_t ray;
	ray.Init( from, from + dir * maxDist, vec3_origin, vec3_origin );

	*corner1 = m_ladderAnchor + ladderUp * maxDist + ladderRight * maxDist;
	*corner2 = m_ladderAnchor + ladderUp * maxDist - ladderRight * maxDist;
	*corner3 = m_ladderAnchor - ladderUp * maxDist - ladderRight * maxDist;
	float dist = IntersectRayWithTriangle( ray, *corner1, *corner2, *corner3, false );
	if ( dist < 0 )
	{
		*corner2 = m_ladderAnchor - ladderUp * maxDist + ladderRight * maxDist;
		dist = IntersectRayWithTriangle( ray, *corner1, *corner2, *corner3, false );
	}

	*corner3 = m_editCursorPos;
	if ( dist > 0 && dist < maxDist )
	{
		*corner3 = from + dir * dist * maxDist;

		float vertDistance = corner3->z - m_ladderAnchor.z;
		float val = vertDistance / ladderUp.z;

		*corner1 = m_ladderAnchor + val * ladderUp;
		*corner2 = *corner3 - val * ladderUp;

		return true;
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------------
bool CheckForClimbableSurface( const Vector &start, const Vector &end )
{
	trace_t result;
	UTIL_TraceLine( start, end, MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );

	bool climbableSurface = false;
	if (result.fraction != 1.0f)
	{
		climbableSurface = physprops->GetSurfaceData( result.surface.surfaceProps )->game.climbable != 0;
		if ( !climbableSurface )
		{
			climbableSurface = (result.contents & CONTENTS_LADDER) != 0;
		}
	}

	return climbableSurface;
}


//--------------------------------------------------------------------------------------------------------------
void StepAlongClimbableSurface( Vector &pos, const Vector &increment, const Vector &probe )
{
	while ( CheckForClimbableSurface( pos + increment - probe, pos + increment + probe ) )
	{
		pos += increment;
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavBuildLadder( void )
{
	if ( !IsEditMode( NORMAL ) || !m_climbableSurface )
	{
		return;
	}

	// We've got a ladder at m_editCursorPos, with a normal of m_surfaceNormal
	Vector right, up;
	VectorVectors( -m_surfaceNormal, right, up );

	m_ladderNormal = m_surfaceNormal;

	Vector startPos = m_editCursorPos;

	Vector leftEdge = startPos;
	Vector rightEdge = startPos;

	// trace to the sides to find the width
	Vector probe = m_surfaceNormal * -HalfHumanWidth;
	const float StepSize = 1.0f;
	StepAlongClimbableSurface( leftEdge, right * -StepSize, probe );
	StepAlongClimbableSurface( rightEdge, right * StepSize, probe );

	Vector topEdge = (leftEdge + rightEdge) * 0.5f;
	Vector bottomEdge = topEdge;
	StepAlongClimbableSurface( topEdge, up * StepSize, probe );
	StepAlongClimbableSurface( bottomEdge, up * -StepSize, probe );

	Vector top = (leftEdge + rightEdge) * 0.5f;
	top.z = topEdge.z;

	Vector bottom = top;
	bottom.z = bottomEdge.z;

	CreateLadder( topEdge, bottomEdge, leftEdge.DistTo( rightEdge ), m_ladderNormal.AsVector2D(), 0.0f );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Flood fills all areas with current place
 */
class PlaceFloodFillFunctor
{
public:
	PlaceFloodFillFunctor( CNavArea *area )
	{
		m_initialPlace = area->GetPlace();
	}

	bool operator() ( CNavArea *area )
	{
		if (area->GetPlace() != m_initialPlace)
			return false;

		area->SetPlace( TheNavMesh->GetNavPlace() );

		return true;
	}

private:
	unsigned int m_initialPlace;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Called when edit mode has just been enabled
 */
void CNavMesh::OnEditModeStart( void )
{
	ClearSelectedSet();
	m_isContinuouslySelecting = false;
	m_isContinuouslyDeselecting = false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Called when edit mode has just been disabled
 */
void CNavMesh::OnEditModeEnd( void )
{
}


//--------------------------------------------------------------------------------------------------------------
class DrawSelectedSet
{
public:
	DrawSelectedSet( const Vector &shift )
	{
		m_count = 0;
		m_shift = shift;
	}
	
	bool operator() ( CNavArea *area )
	{
		if (TheNavMesh->IsInSelectedSet( area ))
		{
			area->DrawSelectedSet( m_shift );
			++m_count;
		}
		
		return (m_count < nav_draw_limit.GetInt());
	}
	
	int m_count;
	Vector m_shift;
};


//--------------------------------------------------------------------------------------------------------
class AddToDragSet
{
public:
	AddToDragSet( Extent &area, int zMin, int zMax, bool bDragDeselecting )
	{
		m_nTolerance = 1;
		m_dragArea = area;
		m_zMin = zMin - m_nTolerance;
		m_zMax = zMax + m_nTolerance;
		m_bDragDeselecting = bDragDeselecting;
	}

	bool operator() ( CNavArea *area )
	{
		bool bShouldBeInSelectedSet = m_bDragDeselecting;
		if ( ( TheNavMesh->IsInSelectedSet( area ) == bShouldBeInSelectedSet ) && area->IsOverlapping( m_dragArea ) && area->GetCenter().z >= m_zMin && area->GetCenter().z <= m_zMax )
		{
			TheNavMesh->AddToDragSelectionSet( area );
		}
		return true;
	}

	Extent m_dragArea;
	int m_zMin;
	int m_zMax;
	int m_nTolerance;
	bool m_bDragDeselecting;
};

//--------------------------------------------------------------------------------------------------------------
void CNavMesh::UpdateDragSelectionSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	Extent dragArea;
	int xmin = MIN( m_anchor.x, m_editCursorPos.x );
	int xmax = MAX( m_anchor.x, m_editCursorPos.x );
	int ymin = MIN( m_anchor.y, m_editCursorPos.y );
	int ymax = MAX( m_anchor.y, m_editCursorPos.y );

	dragArea.lo = Vector( xmin, ymin, m_anchor.z );
	dragArea.hi = Vector( xmax, ymax, m_anchor.z );

	ClearDragSelectionSet();

	AddToDragSet add( dragArea, m_anchor.z - m_nDragSelectionVolumeZMin, m_anchor.z + m_nDragSelectionVolumeZMax, m_bIsDragDeselecting );
	ForAllAreas( add );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw navigation areas and edit them
 * @todo Clean the whole edit system up - its structure is legacy from peculiarities in GoldSrc.
 */
ConVar nav_show_compass( "nav_show_compass", "0", FCVAR_CHEAT );
void CNavMesh::DrawEditMode( void )
{
	VPROF( "CNavMesh::DrawEditMode" );

	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( IsGenerating() )
		return;

	// TODO: remove this when host_thread_mode 1 stops breaking NDEBUG_PERSIST_TILL_NEXT_SERVER overlays
	static ConVarRef host_thread_mode( "host_thread_mode" );
	host_thread_mode.SetValue( 0 );

	const float maxRange = 1000.0f;		// 500

#if DEBUG_NAV_NODES
	if ( nav_show_nodes.GetBool() )
	{
		for ( CNavNode *node = CNavNode::GetFirst(); node != NULL; node = node->GetNext() )
		{
			if ( m_editCursorPos.DistToSqr( *node->GetPosition() ) < 150*150 )
			{
				node->Draw();
			}
		}
	}
#endif // DEBUG_NAV_NODES

	Vector from, dir;
	GetEditVectors( &from, &dir );

	Vector to = from + maxRange * dir;

	/* IN_PROGRESS:
	if ( !IsEditMode( PLACE_PAINTING ) && nav_snap_to_grid.GetBool() )
	{
		Vector center = SnapToGrid( m_editCursorPos );

		const int GridCount = 3;
		const int GridArraySize = GridCount * 2 + 1;
		const int GridSize = GetGridSize();

		// fill in an array of heights for the grid
		Vector pos[GridArraySize][GridArraySize];
		int x, y;
		for ( x=0; x<GridArraySize; ++x )
		{
			for ( y=0; y<GridArraySize; ++y )
			{
				pos[x][y] = center;
				pos[x][y].x += (x-GridCount) * GridSize;
				pos[x][y].y += (y-GridCount) * GridSize;
				pos[x][y].z += 36.0f;

				GetGroundHeight( pos[x][y], &pos[x][y].z );
			}
		}

		for ( x=1; x<GridArraySize; ++x )
		{
			for ( y=1; y<GridArraySize; ++y )
			{
				NavDrawLine( pos[x-1][y-1], pos[x-1][y], NavGridColor );
				NavDrawLine( pos[x-1][y-1], pos[x][y-1], NavGridColor );

				if ( x == GridArraySize-1 )
				{
					NavDrawLine( pos[x][y-1], pos[x][y], NavGridColor );
				}

				if ( y == GridArraySize-1 )
				{
					NavDrawLine( pos[x-1][y], pos[x][y], NavGridColor );
				}
			}
		}
	}
	*/

	if ( FindActiveNavArea() || m_markedArea || m_markedLadder || !IsSelectedSetEmpty() || IsEditMode( CREATING_AREA ) || IsEditMode( CREATING_LADDER ) )
	{
		// draw cursor
		float cursorSize = 10.0f;

		if ( m_climbableSurface )
		{
			NDebugOverlay::Cross3D( m_editCursorPos, cursorSize, 0, 255, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
		else
		{
			NavDrawLine( m_editCursorPos + Vector( 0, 0, cursorSize ), m_editCursorPos,								NavCursorColor );
			NavDrawLine( m_editCursorPos + Vector( cursorSize, 0, 0 ), m_editCursorPos + Vector( -cursorSize, 0, 0 ),	NavCursorColor );
			NavDrawLine( m_editCursorPos + Vector( 0, cursorSize, 0 ), m_editCursorPos + Vector( 0, -cursorSize, 0 ),	NavCursorColor );

			if ( nav_show_compass.GetBool() )
			{
				const float offset = cursorSize * 1.5f;
				Vector pos;

				pos = m_editCursorPos;
				AddDirectionVector( &pos, NORTH, offset );
				NDebugOverlay::Text( pos, "N", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				pos = m_editCursorPos;
				AddDirectionVector( &pos, SOUTH, offset );
				NDebugOverlay::Text( pos, "S", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				pos = m_editCursorPos;
				AddDirectionVector( &pos, EAST, offset );
				NDebugOverlay::Text( pos, "E", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				pos = m_editCursorPos;
				AddDirectionVector( &pos, WEST, offset );
				NDebugOverlay::Text( pos, "W", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}

		// show drag rectangle when creating areas and ladders
		if ( IsEditMode( CREATING_AREA ) )
		{
			float z = m_anchor.z + 2.0f;
			NavDrawLine( Vector( m_editCursorPos.x, m_editCursorPos.y, z ), Vector( m_anchor.x, m_editCursorPos.y, z ),	NavCreationColor );
			NavDrawLine( Vector( m_anchor.x, m_anchor.y, z ), Vector( m_anchor.x, m_editCursorPos.y, z ),					NavCreationColor );
			NavDrawLine( Vector( m_anchor.x, m_anchor.y, z ), Vector( m_editCursorPos.x, m_anchor.y, z ),					NavCreationColor );
			NavDrawLine( Vector( m_editCursorPos.x, m_editCursorPos.y, z ), Vector( m_editCursorPos.x, m_anchor.y, z ),	NavCreationColor );
		}
		else if ( IsEditMode( DRAG_SELECTING ) )
		{
			float z1 = m_anchor.z + m_nDragSelectionVolumeZMax;
			float z2 = m_anchor.z - m_nDragSelectionVolumeZMin;

			// Draw the drag select volume
			Vector vMin( m_anchor.x, m_anchor.y, z1 );
			Vector vMax( m_editCursorPos.x, m_editCursorPos.y, z2 );
			NavDrawVolume( vMin, vMax, m_anchor.z, NavDragSelectionColor );

			UpdateDragSelectionSet();

			Color dragSelectionColor = m_bIsDragDeselecting ? s_dragSelectionSetDeleteColor : s_dragSelectionSetAddColor;

			// Draw the drag selection set
			FOR_EACH_VEC( m_dragSelectionSet, it )
			{
				CNavArea *area = m_dragSelectionSet[ it ];
				area->DrawDragSelectionSet( dragSelectionColor );
			}
		}
		else if ( IsEditMode( CREATING_LADDER ) )
		{
			Vector corner1, corner2, corner3;
			if ( FindLadderCorners( &corner1, &corner2, &corner3 ) )
			{
				NavEditColor color = NavCreationColor;
				if ( !m_climbableSurface )
				{
					color = NavInvalidCreationColor;
				}

				NavDrawLine( m_ladderAnchor, corner1, color );
				NavDrawLine( corner1, corner3, color );
				NavDrawLine( corner3, corner2, color );
				NavDrawLine( corner2, m_ladderAnchor, color );
			}
		}

		if ( m_selectedLadder )
		{
			m_lastSelectedArea = NULL;

			// if ladder changed, print its ID
			if (m_selectedLadder != m_lastSelectedLadder || nav_show_area_info.GetBool())
			{
				m_lastSelectedLadder = m_selectedLadder;

				// print ladder info
				char buffer[80];

				CBaseEntity *ladderEntity = m_selectedLadder->GetLadderEntity();
				if ( ladderEntity )
				{
					V_snprintf( buffer, sizeof( buffer ), "Ladder #%d (Team %s)\n", m_selectedLadder->GetID(), GetGlobalTeam( ladderEntity->GetTeamNumber() )->GetName() );
				}
				else
				{
					V_snprintf( buffer, sizeof( buffer ), "Ladder #%d\n", m_selectedLadder->GetID() );
				}
				NDebugOverlay::ScreenText( 0.5, 0.53, buffer, 255, 255, 0, 128, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}

			// draw the ladder we are pointing at and all connected areas
			m_selectedLadder->DrawLadder();
			m_selectedLadder->DrawConnectedAreas();
		}

		if ( m_markedLadder && !IsEditMode( PLACE_PAINTING ) )
		{
			// draw the "marked" ladder
			m_markedLadder->DrawLadder();
		}

		if ( m_markedArea && !IsEditMode( PLACE_PAINTING ) )
		{
			// draw the "marked" area
			m_markedArea->Draw();
		}

		// find the area the player is pointing at
		if (m_selectedArea)
		{
			m_lastSelectedLadder = NULL;
	
			// if area changed, print its ID
			if ( m_selectedArea != m_lastSelectedArea )
			{
				m_showAreaInfoTimer.Start( nav_show_area_info.GetFloat() );
				m_lastSelectedArea = m_selectedArea;
			}

			if (m_showAreaInfoTimer.HasStarted() && !m_showAreaInfoTimer.IsElapsed() )
			{
				char buffer[80];
				char attrib[80];
				char locName[80];

				if (m_selectedArea->GetPlace())
				{
					const char *name = TheNavMesh->PlaceToName( m_selectedArea->GetPlace() );
					if (name)
						V_strcpy_safe( locName, name );
					else
						V_strcpy_safe( locName, "ERROR" );
				}
				else
				{
					locName[0] = '\000';
				}

				if (IsEditMode( PLACE_PAINTING ))
				{
					attrib[0] = '\000';
				}
				else
				{
					attrib[0] = 0;
					int attributes = m_selectedArea->GetAttributes();
					if ( attributes & NAV_MESH_CROUCH )		Q_strncat( attrib, "CROUCH ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_JUMP )		Q_strncat( attrib, "JUMP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_PRECISE )	Q_strncat( attrib, "PRECISE ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_NO_JUMP )	Q_strncat( attrib, "NO_JUMP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_STOP )		Q_strncat( attrib, "STOP ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_RUN )		Q_strncat( attrib, "RUN ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_WALK )		Q_strncat( attrib, "WALK ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_AVOID )		Q_strncat( attrib, "AVOID ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_TRANSIENT )	Q_strncat( attrib, "TRANSIENT ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_DONT_HIDE )	Q_strncat( attrib, "DONT_HIDE ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_STAND )		Q_strncat( attrib, "STAND ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_NO_HOSTAGES )Q_strncat( attrib, "NO HOSTAGES ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_STAIRS )		Q_strncat( attrib, "STAIRS ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_OBSTACLE_TOP ) Q_strncat( attrib, "OBSTACLE ", sizeof( attrib ), -1 );
					if ( attributes & NAV_MESH_CLIFF )		Q_strncat( attrib, "CLIFF ", sizeof( attrib ), -1 );
#ifdef TERROR
					if ( attributes & TerrorNavArea::NAV_PLAYERCLIP )		Q_strncat( attrib, "PLAYERCLIP ", sizeof( attrib ), -1 );
					if ( attributes & TerrorNavArea::NAV_BREAKABLEWALL )	Q_strncat( attrib, "BREAKABLEWALL ", sizeof( attrib ), -1 );
					if ( m_selectedArea->IsBlocked( TEAM_SURVIVOR ) ) Q_strncat( attrib, "BLOCKED_SURVIVOR ", sizeof( attrib ), -1 );
					if ( m_selectedArea->IsBlocked( TEAM_ZOMBIE ) ) Q_strncat( attrib, "BLOCKED_ZOMBIE ", sizeof( attrib ), -1 );
#else
					if ( m_selectedArea->IsBlocked( TEAM_ANY ) ) Q_strncat( attrib, "BLOCKED ", sizeof( attrib ), -1 );
#endif
					if ( m_selectedArea->HasAvoidanceObstacle() )	Q_strncat( attrib, "OBSTRUCTED ", sizeof( attrib ), -1 );
					if ( m_selectedArea->IsDamaging() )		Q_strncat( attrib, "DAMAGING ", sizeof( attrib ), -1 );
					if ( m_selectedArea->IsUnderwater() )	Q_strncat( attrib, "UNDERWATER ", sizeof( attrib ), -1 );

					int connected = 0;
					connected += m_selectedArea->GetAdjacentCount( NORTH );
					connected += m_selectedArea->GetAdjacentCount( SOUTH );
					connected += m_selectedArea->GetAdjacentCount( EAST );
					connected += m_selectedArea->GetAdjacentCount( WEST );
					Q_strncat( attrib, UTIL_VarArgs( "%d Connections ", connected ), sizeof( attrib ), -1 );
				}

				Q_snprintf( buffer, sizeof( buffer ), "Area #%d %s %s\n", m_selectedArea->GetID(), locName, attrib );
				NDebugOverlay::ScreenText( 0.5, 0.53, buffer, 255, 255, 0, 128, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				// do "place painting"
				if ( m_isPlacePainting )
				{
					if (m_selectedArea->GetPlace() != TheNavMesh->GetNavPlace())
					{
						m_selectedArea->SetPlace( TheNavMesh->GetNavPlace() );
						player->EmitSound( "Bot.EditSwitchOn" );
					}
				}
			}
			

			// do continuous selecting into selected set
			if ( m_isContinuouslySelecting )
			{
				AddToSelectedSet( m_selectedArea );
			}
			else if ( m_isContinuouslyDeselecting )
			{
				// do continuous de-selecting into selected set
				RemoveFromSelectedSet( m_selectedArea );
			}


			if (IsEditMode( PLACE_PAINTING ))
			{
				m_selectedArea->DrawConnectedAreas();
			}
			else	// normal editing mode
			{
				// draw split line
				Extent extent;
				m_selectedArea->GetExtent( &extent );

				float yaw = player->EyeAngles().y;
				while( yaw > 360.0f )
					yaw -= 360.0f;

				while( yaw < 0.0f )
					yaw += 360.0f;

				if (m_splitAlongX)
				{
					from.x = extent.lo.x;
					from.y = m_splitEdge;
					from.z = m_selectedArea->GetZ( from );

					to.x = extent.hi.x;
					to.y = m_splitEdge;
					to.z = m_selectedArea->GetZ( to );
				}
				else
				{
					from.x = m_splitEdge;
					from.y = extent.lo.y;
					from.z = m_selectedArea->GetZ( from );

					to.x = m_splitEdge;
					to.y = extent.hi.y;
					to.z = m_selectedArea->GetZ( to );
				}

				NavDrawLine( from, to, NavSplitLineColor );

				// draw the area we are pointing at and all connected areas
				m_selectedArea->DrawConnectedAreas();
			}
		}
		
		// render the selected set
		if (!IsSelectedSetEmpty())
		{
			Vector shift( 0, 0, 0 );
			
			if (IsEditMode( SHIFTING_XY ))
			{
				shift = m_editCursorPos - m_anchor;
				shift.z = 0.0f;
			}

			DrawSelectedSet draw( shift );

			// if the selected set is small, just blast it out
			if (m_selectedSet.Count() < nav_draw_limit.GetInt())
			{
				FOR_EACH_VEC( m_selectedSet, it )
				{
					CNavArea *area = m_selectedSet[ it ];
					
					draw( area );
				}
			}
			else
			{
				// draw the part nearest the player
				CNavArea *nearest = NULL;
				float nearRange = 9999999999.9f;

				FOR_EACH_VEC( m_selectedSet, it )
				{
					CNavArea *area = m_selectedSet[ it ];

					float range = (player->GetAbsOrigin() - area->GetCenter()).LengthSqr();
					if (range < nearRange)
					{
						nearRange = range;
						nearest = area;
					}
				}

				SearchSurroundingAreas( nearest, nearest->GetCenter(), draw, -1, INCLUDE_INCOMING_CONNECTIONS | INCLUDE_BLOCKED_AREAS );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetMarkedLadder( CNavLadder *ladder )
{
	m_markedLadder = ladder;
	m_markedArea = NULL;
	m_markedCorner = NUM_CORNERS;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::SetMarkedArea( CNavArea *area )
{
	m_markedLadder = NULL;
	m_markedArea = area;
	m_markedCorner = NUM_CORNERS;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavDelete( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	if (IsSelectedSetEmpty())
	{
		// the old way
		CNavArea *markedArea = GetMarkedArea(); 
		CNavLadder *markedLadder = GetMarkedLadder(); 
		FindActiveNavArea();

		if( markedArea )
		{
			player->EmitSound( "EDIT_DELETE" );
			TheNavAreas.FindAndRemove( markedArea );
			TheNavMesh->OnEditDestroyNotify( markedArea );
			TheNavMesh->DestroyArea( markedArea );
		}
		else if( markedLadder )
		{
			player->EmitSound( "EDIT_DELETE" );
			m_ladders.FindAndRemove( markedLadder );
			OnEditDestroyNotify( markedLadder );
			delete markedLadder;
		} 
		else if ( m_selectedArea )
		{
			player->EmitSound( "EDIT_DELETE" );
			TheNavAreas.FindAndRemove( m_selectedArea );
			CNavArea *deadArea = m_selectedArea;
			OnEditDestroyNotify( deadArea );
			TheNavMesh->DestroyArea( deadArea );
		}
		else if ( m_selectedLadder )
		{
			player->EmitSound( "EDIT_DELETE" );
			m_ladders.FindAndRemove( m_selectedLadder );
			CNavLadder *deadLadder = m_selectedLadder;
			OnEditDestroyNotify( deadLadder );
			delete deadLadder;
		}
	}
	else
	{
		// delete all areas in the selected set
		player->EmitSound( "EDIT_DELETE" );
		
		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];
			
			TheNavAreas.FindAndRemove( area );
			
			OnEditDestroyNotify( area );
			
			TheNavMesh->DestroyArea( area );
		}
		
		Msg( "Deleted %d areas\n", m_selectedSet.Count() );
		
		ClearSelectedSet();		
	}
	
	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
class SelectCollector
{
public:
	SelectCollector( void )
	{
		m_count =  0;
	}
	
	bool operator() ( CNavArea *area )
	{
		// already selected areas terminate flood select
		if (TheNavMesh->IsInSelectedSet( area ))
			return false;		
		
		TheNavMesh->AddToSelectedSet( area );
		++m_count;
		
		return true;
	}

	int m_count;
};


//-------------------------------------------------------------------------------------------------------------- 
void CNavMesh::CommandNavDeleteMarked( void ) 
{ 
	CBasePlayer *player = UTIL_GetListenServerHost(); 
	if (player == NULL) 
		return; 

	if ( !IsEditMode( NORMAL ) )
		return; 

	CNavArea *markedArea = GetMarkedArea(); 
	if( markedArea ) 
	{ 
		player->EmitSound( "EDIT_DELETE" ); 
		TheNavMesh->OnEditDestroyNotify( markedArea );
		TheNavAreas.FindAndRemove( markedArea ); 
		TheNavMesh->DestroyArea( markedArea ); 
	} 

	CNavLadder *markedLadder = GetMarkedLadder(); 
	if( markedLadder ) 
	{ 
		player->EmitSound( "EDIT_DELETE" );
		m_ladders.FindAndRemove( markedLadder );
		delete markedLadder; 
	} 

	StripNavigationAreas(); 

	ClearSelectedSet();		

	SetMarkedArea( NULL );				// unmark the mark area 
	SetMarkedLadder( NULL );			// unmark the mark ladder
	m_markedCorner = NUM_CORNERS;		// clear the corner selection 
} 


//--------------------------------------------------------------------------------------------------------------
/**
 * Select the current nav area and all recursively connected areas
 */
void CNavMesh::CommandNavFloodSelect( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	CNavArea *start = m_selectedArea;
	if ( !start )
	{
		start = m_markedArea;
	}

	if ( start )
	{
		player->EmitSound( "EDIT_DELETE" );

		int connections = INCLUDE_BLOCKED_AREAS | INCLUDE_INCOMING_CONNECTIONS;
		if ( args.ArgC() == 2 && FStrEq( "out", args[1] ) )
		{
			connections = INCLUDE_BLOCKED_AREAS;
		}
		if ( args.ArgC() == 2 && FStrEq( "in", args[1] ) )
		{
			connections = INCLUDE_BLOCKED_AREAS | INCLUDE_INCOMING_CONNECTIONS | EXCLUDE_OUTGOING_CONNECTIONS;
		}

		// collect all areas connected to this area
		SelectCollector collector;
		SearchSurroundingAreas( start, start->GetCenter(), collector, -1, connections );
		
		Msg( "Selected %d areas.\n", collector.m_count );
	}

	SetMarkedArea( NULL );			// unmark the mark area
}


//--------------------------------------------------------------------------------------------------------------
/**
* Toggles all areas into/out of the selected set
*/
void CNavMesh::CommandNavToggleSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	player->EmitSound( "EDIT_DELETE" );

	NavAreaVector notInSelectedSet;

	// Build a list of all areas not in the selected set
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[it];
		if ( !IsInSelectedSet( area ) )
		{
			notInSelectedSet.AddToTail( area );
		}
	}

	// Clear out the selected set
	ClearSelectedSet();

	// Add areas back into the selected set
	FOR_EACH_VEC( notInSelectedSet, nit )
	{
		CNavArea *area = notInSelectedSet[nit];
		AddToSelectedSet( area );
	}

	Msg( "Selected %d areas.\n", notInSelectedSet.Count() );

	SetMarkedArea( NULL );			// unmark the mark area
}


//--------------------------------------------------------------------------------------------------------------
/**
* Saves the current selected set for later retrieval.
*/
void CNavMesh::CommandNavStoreSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	player->EmitSound( "EDIT_DELETE" );

	m_storedSelectedSet.RemoveAll();
	FOR_EACH_VEC( m_selectedSet, it )
	{
		m_storedSelectedSet.AddToTail( m_selectedSet[it]->GetID() );
	}
}



//--------------------------------------------------------------------------------------------------------------
/**
* Restores an older selected set.
*/
void CNavMesh::CommandNavRecallSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	player->EmitSound( "EDIT_DELETE" );

	ClearSelectedSet();

	for ( int i=0; i<m_storedSelectedSet.Count(); ++i )
	{
		CNavArea *area = GetNavAreaByID( m_storedSelectedSet[i] );
		if ( area )
		{
			AddToSelectedSet( area );
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add current area to selected set
 */
void CNavMesh::CommandNavAddToSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		AddToSelectedSet( m_selectedArea );
		player->EmitSound( "EDIT_MARK.Enable" );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
* Add area ID to selected set
*/
void CNavMesh::CommandNavAddToSelectedSetByID( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) ) || args.ArgC() < 2 )
		return;

	int id = atoi( args[1] );
	CNavArea *area = GetNavAreaByID( id );
	if ( area )
	{
		AddToSelectedSet( area );
		player->EmitSound( "EDIT_MARK.Enable" );
		Msg( "Added area %d.  ( to go there: setpos %f %f %f )\n", id, area->GetCenter().x, area->GetCenter().y, area->GetCenter().z + 5 );
	}
	else
	{
		Msg( "No area with id %d\n", id );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Remove current area from selected set
 */
void CNavMesh::CommandNavRemoveFromSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		RemoveFromSelectedSet( m_selectedArea );
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
* Add/remove current area from selected set
*/
void CNavMesh::CommandNavToggleInSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (IsInSelectedSet( m_selectedArea ))
		{
			RemoveFromSelectedSet( m_selectedArea );
		}
		else
		{
			AddToSelectedSet( m_selectedArea );
		}
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Clear the selected set to empty
 */
void CNavMesh::CommandNavClearSelectedSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	ClearSelectedSet();
	player->EmitSound( "EDIT_MARK.Disable" );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Start continuously selecting areas into the selected set
 */
void CNavMesh::CommandNavBeginSelecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslySelecting = true;
	m_isContinuouslyDeselecting = false;
	
	player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Stop continuously selecting areas into the selected set
 */
void CNavMesh::CommandNavEndSelecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslySelecting = false;
	m_isContinuouslyDeselecting = false;

	player->EmitSound( "EDIT_END_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavBeginDragSelecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) && !IsEditMode( DRAG_SELECTING ) )
		return;

	FindActiveNavArea();

	if ( IsEditMode( DRAG_SELECTING ) )
	{
		ClearDragSelectionSet();
		SetEditMode( NORMAL );
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
	}
	else
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		
		SetEditMode( DRAG_SELECTING );

		// m_anchor starting corner
		m_anchor = m_editCursorPos;
		m_nDragSelectionVolumeZMax = nav_drag_selection_volume_zmax_offset.GetInt();
		m_nDragSelectionVolumeZMin = nav_drag_selection_volume_zmin_offset.GetInt();
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavEndDragSelecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( IsEditMode( DRAG_SELECTING ) )
	{
		// Transfer drag selected areas to the selected set
		FOR_EACH_VEC( m_dragSelectionSet, it )
		{
			AddToSelectedSet( m_dragSelectionSet[it] );
		}
		SetEditMode( NORMAL );
	}
	else
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
	}

	ClearDragSelectionSet();
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavBeginDragDeselecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) && !IsEditMode( DRAG_SELECTING ) )
		return;

	FindActiveNavArea();

	if ( IsEditMode( DRAG_SELECTING ) )
	{
		ClearDragSelectionSet();
		SetEditMode( NORMAL );
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
	}
	else
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		
		SetEditMode( DRAG_SELECTING );
		m_bIsDragDeselecting = true;

		// m_anchor starting corner
		m_anchor = m_editCursorPos;
		m_nDragSelectionVolumeZMax = nav_drag_selection_volume_zmax_offset.GetInt();
		m_nDragSelectionVolumeZMin = nav_drag_selection_volume_zmin_offset.GetInt();
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavEndDragDeselecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( IsEditMode( DRAG_SELECTING ) )
	{
		// Remove drag selected areas from the selected set
		FOR_EACH_VEC( m_dragSelectionSet, it )
		{
			RemoveFromSelectedSet( m_dragSelectionSet[it] );
		}
		SetEditMode( NORMAL );
	}
	else
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
	}

	ClearDragSelectionSet();
	m_bIsDragDeselecting = false;
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavRaiseDragVolumeMax( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	m_nDragSelectionVolumeZMax += 32;
	nav_drag_selection_volume_zmax_offset.SetValue( m_nDragSelectionVolumeZMax );
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavLowerDragVolumeMax( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	m_nDragSelectionVolumeZMax = MAX( 0, m_nDragSelectionVolumeZMax - 32 );
	nav_drag_selection_volume_zmax_offset.SetValue( m_nDragSelectionVolumeZMax );
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavRaiseDragVolumeMin( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	m_nDragSelectionVolumeZMin = MAX( 0, m_nDragSelectionVolumeZMin - 32 );
	nav_drag_selection_volume_zmin_offset.SetValue( m_nDragSelectionVolumeZMin );
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavLowerDragVolumeMin( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	m_nDragSelectionVolumeZMin += 32;
	nav_drag_selection_volume_zmin_offset.SetValue( m_nDragSelectionVolumeZMin );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Start/stop continuously selecting areas into the selected set
 */
void CNavMesh::CommandNavToggleSelecting( bool playSound )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslySelecting = !m_isContinuouslySelecting;
	m_isContinuouslyDeselecting = false;

	if ( playSound )
	{
		player->EmitSound( "EDIT_END_AREA.Creating" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Start continuously de-selecting areas from the selected set
 */
void CNavMesh::CommandNavBeginDeselecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslyDeselecting = true;
	m_isContinuouslySelecting = false;
	
	player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Stop continuously de-selecting areas from the selected set
 */
void CNavMesh::CommandNavEndDeselecting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslyDeselecting = false;
	m_isContinuouslySelecting = false;

	player->EmitSound( "EDIT_END_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Start/stop continuously de-selecting areas from the selected set
 */
void CNavMesh::CommandNavToggleDeselecting( bool playSound )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	m_isContinuouslyDeselecting = !m_isContinuouslyDeselecting;
	m_isContinuouslySelecting = false;

	if ( playSound )
	{
		player->EmitSound( "EDIT_END_AREA.Creating" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Selects all areas that intersect the half-space
 * Arguments: "+z 100", or "-x 30", etc.
 */
void CNavMesh::CommandNavSelectHalfSpace( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	if ( args.ArgC() != 3 )
	{
		Warning( "Error:  <+X|-X|+Y|-Y|+Z|-Z> <value>\n" );
		return;
	}

	enum HalfSpaceType
	{
		PLUS_X, MINUS_X,
		PLUS_Y, MINUS_Y,
		PLUS_Z, MINUS_Z,	
	}
	halfSpace = PLUS_X;
	
	if (FStrEq( "+x", args[1] ))
	{
		halfSpace = PLUS_X;
	}
	else if (FStrEq( "-x", args[1] ))
	{
		halfSpace = MINUS_X;
	}
	else if (FStrEq( "+y", args[1] ))
	{
		halfSpace = PLUS_Y;
	}
	else if (FStrEq( "-y", args[1] ))
	{
		halfSpace = MINUS_Y;
	}
	else if (FStrEq( "+z", args[1] ))
	{
		halfSpace = PLUS_Z;
	}
	else if (FStrEq( "-z", args[1] ))
	{
		halfSpace = MINUS_Z;
	}

	float value = atof( args[2] );
	
	Extent extent;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[it];
		area->GetExtent( &extent );
		
		switch( halfSpace )
		{
			case PLUS_X:
				if (extent.lo.x < value && extent.hi.x < value)
				{
					continue;
				}
				break;

			case PLUS_Y:
				if (extent.lo.y < value && extent.hi.y < value)
				{
					continue;
				}
				break;

			case PLUS_Z:
				if (extent.lo.z < value && extent.hi.z < value)
				{
					continue;
				}
				break;

			case MINUS_X:
				if (extent.lo.x > value && extent.hi.x > value)
				{
					continue;
				}
				break;

			case MINUS_Y:
				if (extent.lo.y > value && extent.hi.y > value)
				{
					continue;
				}
				break;

			case MINUS_Z:
				if (extent.lo.z > value && extent.hi.z > value)
				{
					continue;
				}
				break;
		}

		// toggle membership		
		if ( IsInSelectedSet( area ) )
		{
			RemoveFromSelectedSet( area );
		}
		else
		{
			AddToSelectedSet( area );	
		}
	}

	player->EmitSound( "EDIT_DELETE" );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Begin shifting selected set in the XY plane
 */
void CNavMesh::CommandNavBeginShiftXY( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if (GetEditMode() == SHIFTING_XY)
	{
		SetEditMode( NORMAL );
		player->EmitSound( "EDIT_END_AREA.Creating" );
		return;
	}
	else
	{
		SetEditMode( SHIFTING_XY );
		player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
	}
	
	// m_anchor starting corner
	m_anchor = m_editCursorPos;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Shift a set of areas, and all connected ladders
 */
class ShiftSet
{
public:
	ShiftSet( const Vector &shift )
	{
		m_shift = shift;
	}

	bool operator()( CNavArea *area )
	{
		area->Shift( m_shift );

		const NavLadderConnectVector *ladders = area->GetLadders( CNavLadder::LADDER_UP );
		int it;
		for( it = 0; it < ladders->Count(); ++it )
		{
			CNavLadder *ladder = (*ladders)[ it ].ladder;
			if ( !m_ladders.HasElement( ladder ) )
			{
				ladder->Shift( m_shift );
				m_ladders.AddToTail( ladder );
			}
		}

		ladders = area->GetLadders( CNavLadder::LADDER_DOWN );
		for( it = 0; it < ladders->Count(); ++it )
		{
			CNavLadder *ladder = (*ladders)[ it ].ladder;
			if ( !m_ladders.HasElement( ladder ) )
			{
				ladder->Shift( m_shift );
				m_ladders.AddToTail( ladder );
			}
		}

		return true;
	}

private:
	CUtlVector< CNavLadder * > m_ladders;
	Vector m_shift;
};


//--------------------------------------------------------------------------------------------------------------
/**
 * End shifting selected set in the XY plane
 */
void CNavMesh::CommandNavEndShiftXY( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	SetEditMode( NORMAL );

	Vector shiftAmount = m_editCursorPos - m_anchor;
	shiftAmount.z = 0.0f;

	ShiftSet shift( shiftAmount );

	// update the position of all areas in the selected set
	TheNavMesh->ForAllSelectedAreas( shift );

	player->EmitSound( "EDIT_END_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_shift, "Shifts the selected areas by the specified amount", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	TheNavMesh->SetEditMode( CNavMesh::NORMAL );

	Vector shiftAmount( vec3_origin );
	if ( args.ArgC() > 1 )
	{
		shiftAmount.x = atoi( args[1] );

		if ( args.ArgC() > 2 )
		{
			shiftAmount.y = atoi( args[2] );

			if ( args.ArgC() > 3 )
			{
				shiftAmount.z = atoi( args[3] );
			}
		}
	}

	ShiftSet shift( shiftAmount );

	// update the position of all areas in the selected set
	TheNavMesh->ForAllSelectedAreas( shift );

	player->EmitSound( "EDIT_END_AREA.Creating" );
}


//--------------------------------------------------------------------------------------------------------
void CommandNavCenterInWorld( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	TheNavMesh->SetEditMode( CNavMesh::NORMAL );

	// Build the nav mesh's extent
	Extent navExtent;
	bool first = true;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[it];
		if ( first )
		{
			area->GetExtent( &navExtent );
			first = false;
		}
		else
		{
			navExtent.Encompass( area->GetCorner( NORTH_WEST ) );
			navExtent.Encompass( area->GetCorner( NORTH_EAST ) );
			navExtent.Encompass( area->GetCorner( SOUTH_WEST ) );
			navExtent.Encompass( area->GetCorner( SOUTH_EAST ) );
		}
	}

	// Get the world's extent
	CWorld *world = dynamic_cast< CWorld * >( CBaseEntity::Instance( INDEXENT( 0 ) ) );
	if ( !world )
		return;

	Extent worldExtent;
	world->GetWorldBounds( worldExtent.lo, worldExtent.hi );

	// Compute the difference, and shift in XY
	Vector navCenter = ( navExtent.lo + navExtent.hi ) * 0.5f;
	Vector worldCenter = ( worldExtent.lo + worldExtent.hi ) * 0.5f;
	Vector shift = worldCenter - navCenter;
	shift.z = 0.0f;

	// update the position of all areas
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->Shift( shift );
	}

	player->EmitSound( "EDIT_END_AREA.Creating" );

	Msg( "Shifting mesh by %f,%f\n", shift.x, shift.y );
}
ConCommand nav_world_center( "nav_world_center", CommandNavCenterInWorld, "Centers the nav mesh in the world", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
/**
 * Add invalid areas to selected set
 */
void CNavMesh::CommandNavSelectInvalidAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	ClearSelectedSet();

	Extent areaExtent;
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area )
		{
			area->GetExtent( &areaExtent );
			for ( float x = areaExtent.lo.x; x + GenerationStepSize <= areaExtent.hi.x; x += GenerationStepSize )
			{
				for ( float y = areaExtent.lo.y; y + GenerationStepSize <= areaExtent.hi.y; y += GenerationStepSize )
				{
					float nw = area->GetZ( x, y );
					float ne = area->GetZ( x + GenerationStepSize, y );
					float sw = area->GetZ( x, y + GenerationStepSize );
					float se = area->GetZ( x + GenerationStepSize, y + GenerationStepSize );

					if ( !IsHeightDifferenceValid( nw, ne, sw, se ) ||
						!IsHeightDifferenceValid( ne, nw, sw, se ) ||
						!IsHeightDifferenceValid( sw, ne, nw, se ) ||
						!IsHeightDifferenceValid( se, ne, sw, nw ) )
					{
						AddToSelectedSet( area );
					}
				}
			}
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );

	if ( m_selectedSet.Count() )
	{
		player->EmitSound( "EDIT_MARK.Enable" );
	}
	else
	{
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add blocked areas to selected set
 */
void CNavMesh::CommandNavSelectBlockedAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	ClearSelectedSet();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area && area->IsBlocked( TEAM_ANY ) )
		{
			AddToSelectedSet( area );
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );

	if ( m_selectedSet.Count() )
	{
		player->EmitSound( "EDIT_MARK.Enable" );
	}
	else
	{
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add obstructed areas to selected set
 */
void CNavMesh::CommandNavSelectObstructedAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	ClearSelectedSet();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area && area->HasAvoidanceObstacle() )
		{
			AddToSelectedSet( area );
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );

	if ( m_selectedSet.Count() )
	{
		player->EmitSound( "EDIT_MARK.Enable" );
	}
	else
	{
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add damaging areas to selected set
 */
void CNavMesh::CommandNavSelectDamagingAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	ClearSelectedSet();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area && area->IsDamaging() )
		{
			AddToSelectedSet( area );
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );

	if ( m_selectedSet.Count() )
	{
		player->EmitSound( "EDIT_MARK.Enable" );
	}
	else
	{
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Adds stairs areas to the selected set
 */
void CNavMesh::CommandNavSelectStairs( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	ClearSelectedSet();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area && area->HasAttributes( NAV_MESH_STAIRS ) )
		{
			AddToSelectedSet( area );
		}
	}

	Msg( "Selected %d areas.\n", m_selectedSet.Count() );

	if ( m_selectedSet.Count() )
	{
		player->EmitSound( "EDIT_MARK.Enable" );
	}
	else
	{
		player->EmitSound( "EDIT_MARK.Disable" );
	}
}


//--------------------------------------------------------------------------------------------------------------
// Adds areas not connected to mesh to the selected set
void CNavMesh::CommandNavSelectOrphans( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) && !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	CNavArea *start = m_selectedArea;
	if ( !start )
	{
		start = m_markedArea;
	}

	if ( start )
	{
		player->EmitSound( "EDIT_DELETE" );

		int connections = INCLUDE_BLOCKED_AREAS | INCLUDE_INCOMING_CONNECTIONS;

		// collect all areas connected to this area
		SelectCollector collector;
		SearchSurroundingAreas( start, start->GetCenter(), collector, -1, connections );

		// toggle the selected set to reveal the orphans
		CommandNavToggleSelectedSet();
	}

	SetMarkedArea( NULL );			// unmark the mark area

}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavSplit( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (m_selectedArea->SplitEdit( m_splitAlongX, m_splitEdge ))
			player->EmitSound( "EDIT_SPLIT.MarkedArea" );
		else
			player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
bool MakeSniperSpots( CNavArea *area )
{
	if ( !area )
		return false;

	bool splitAlongX;
	float splitEdge;

	const float minSplitSize = 2.0f; // ensure the first split is larger than this

	float sizeX = area->GetSizeX();
	float sizeY = area->GetSizeY();

	if ( sizeX > GenerationStepSize && sizeX > sizeY )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).x, GenerationStepSize );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).x + minSplitSize )
			splitEdge += GenerationStepSize;
		splitAlongX = false;
	}
	else if ( sizeY > GenerationStepSize && sizeY > sizeX )
	{
		splitEdge = RoundToUnits( area->GetCorner( NORTH_WEST ).y, GenerationStepSize );
		if ( splitEdge < area->GetCorner( NORTH_WEST ).y + minSplitSize )
			splitEdge += GenerationStepSize;
		splitAlongX = true;
	}
	else
	{
		return false;
	}

	CNavArea *first, *second;
	if ( !area->SplitEdit( splitAlongX, splitEdge, &first, &second ) )
	{
		return false;
	}

	first->Disconnect( second );
	second->Disconnect( first );

	MakeSniperSpots( first );
	MakeSniperSpots( second );

	return true;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMakeSniperSpots( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		// recursively split the area
		if ( MakeSniperSpots( m_selectedArea ) )
		{
			player->EmitSound( "EDIT_SPLIT.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
		}
	}
	else
	{
		player->EmitSound( "EDIT_SPLIT.NoMarkedArea" );
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMerge( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		CNavArea *other = m_markedArea;
		if ( !m_markedArea && m_selectedSet.Count() == 1 )
		{
			other = m_selectedSet[0];
		}

		if ( other && other != m_selectedArea )
		{
			if ( m_selectedArea->MergeEdit( other ) )
				player->EmitSound( "EDIT_MERGE.Enable" );
			else
				player->EmitSound( "EDIT_MERGE.Disable" );
		}
		else
		{
			Msg( "To merge, mark an area, highlight a second area, then invoke the merge command" );
			player->EmitSound( "EDIT_MERGE.Disable" );
		}
	}

	StripNavigationAreas();

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
	ClearSelectedSet();
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMark( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;
		
	if (!IsSelectedSetEmpty())
	{
		// add or remove areas from the selected set
		if (IsInSelectedSet( m_selectedArea ))
		{
			// remove from set
			player->EmitSound( "EDIT_MARK.Disable" );
			RemoveFromSelectedSet( m_selectedArea );
		}
		else
		{
			// add to set
			player->EmitSound( "EDIT_MARK.Enable" );
			AddToSelectedSet( m_selectedArea );
		}
		return;
	}

	FindActiveNavArea();

	if ( m_markedArea || m_markedLadder )
	{
		// Unmark area or ladder
		player->EmitSound( "EDIT_MARK.Enable" );
		Msg("Area unmarked.\n");
		SetMarkedArea( NULL );
	}
	else if ( args.ArgC() > 1 )
	{
		if ( FStrEq( args[1], "ladder" ) )
		{
			if ( args.ArgC() > 2 )
			{
				const char *ladderIDNameToMark = args[2];
				if ( ladderIDNameToMark )
				{
					unsigned int ladderIDToMark = atoi( ladderIDNameToMark );
					if ( ladderIDToMark != 0 )
					{
						CNavLadder *ladder = TheNavMesh->GetLadderByID( ladderIDToMark );
						if ( ladder )
						{
							player->EmitSound( "EDIT_MARK.Disable" );
							SetMarkedLadder( ladder );

							int connected = 0;
							connected += m_markedLadder->m_topForwardArea != NULL;
							connected += m_markedLadder->m_topLeftArea != NULL;
							connected += m_markedLadder->m_topRightArea != NULL;
							connected += m_markedLadder->m_topBehindArea != NULL;
							connected += m_markedLadder->m_bottomArea != NULL;

							Msg( "Marked Ladder is connected to %d Areas\n", connected );
						}
					}
				}
			}
		}
		else
		{
			const char *areaIDNameToMark = args[1];
			if( areaIDNameToMark != NULL )
			{
				unsigned int areaIDToMark = atoi(areaIDNameToMark);
				if( areaIDToMark != 0 )
				{
					CNavArea *areaToMark = NULL;
					FOR_EACH_VEC( TheNavAreas, nit )
					{
						if( TheNavAreas[nit]->GetID() == areaIDToMark )
						{
							areaToMark = TheNavAreas[nit];
							break;
						}
					}
					if( areaToMark )
					{
						player->EmitSound( "EDIT_MARK.Disable" );
						SetMarkedArea( areaToMark );

						int connected = 0;
						connected += GetMarkedArea()->GetAdjacentCount( NORTH );
						connected += GetMarkedArea()->GetAdjacentCount( SOUTH );
						connected += GetMarkedArea()->GetAdjacentCount( EAST );
						connected += GetMarkedArea()->GetAdjacentCount( WEST );

						Msg( "Marked Area is connected to %d other Areas\n", connected );
					}
				}
			}
		}
	}
	else if ( m_selectedArea )
	{
		// Mark an area
		player->EmitSound( "EDIT_MARK.Disable" );
		SetMarkedArea( m_selectedArea );

		int connected = 0;
		connected += GetMarkedArea()->GetAdjacentCount( NORTH );
		connected += GetMarkedArea()->GetAdjacentCount( SOUTH );
		connected += GetMarkedArea()->GetAdjacentCount( EAST );
		connected += GetMarkedArea()->GetAdjacentCount( WEST );

		Msg( "Marked Area is connected to %d other Areas\n", connected );
	}
	else if ( m_selectedLadder )
	{
		// Mark a ladder
		player->EmitSound( "EDIT_MARK.Disable" );
		SetMarkedLadder( m_selectedLadder );

		int connected = 0;
		connected += m_markedLadder->m_topForwardArea != NULL;
		connected += m_markedLadder->m_topLeftArea != NULL;
		connected += m_markedLadder->m_topRightArea != NULL;
		connected += m_markedLadder->m_topBehindArea != NULL;
		connected += m_markedLadder->m_bottomArea != NULL;

		Msg( "Marked Ladder is connected to %d Areas\n", connected );
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavUnmark( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	player->EmitSound( "EDIT_MARK.Enable" );
	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavBeginArea( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !(IsEditMode( CREATING_AREA ) || IsEditMode( CREATING_LADDER ) || IsEditMode( NORMAL )) )
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
		return;
	}

	FindActiveNavArea();

	if ( IsEditMode( CREATING_AREA ) )
	{
		SetEditMode( NORMAL );
		player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
	}
	else if ( IsEditMode( CREATING_LADDER ) )
	{
		SetEditMode( NORMAL );
		player->EmitSound( "EDIT_BEGIN_AREA.Creating" );
	}
	else if ( m_climbableSurface )
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		
		SetEditMode( CREATING_LADDER );

		// m_ladderAnchor starting corner
		m_ladderAnchor = m_editCursorPos;
		m_ladderNormal = m_surfaceNormal;
	}
	else
	{
		player->EmitSound( "EDIT_BEGIN_AREA.NotCreating" );
		
		SetEditMode( CREATING_AREA );

		// m_anchor starting corner
		m_anchor = m_editCursorPos;
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavEndArea( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !(IsEditMode( CREATING_AREA ) || IsEditMode( CREATING_LADDER ) || IsEditMode( NORMAL )) )
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
		return;
	}

	if ( IsEditMode( CREATING_AREA ) )
	{
		SetEditMode( NORMAL );
		
		// create the new nav area
		Vector endPos = m_editCursorPos;
		endPos.z = m_anchor.z;
		
		// We're a manually-created area, so let's look around to see what's nearby
		CNavArea *nearby = GetMarkedArea();
		if ( !nearby )
		{
			nearby = TheNavMesh->GetNearestNavArea( m_editCursorPos + Vector( 0, 0, HalfHumanHeight ), false, 10000.0f, true );
		}
		if ( !nearby )
		{
			nearby = TheNavMesh->GetNearestNavArea( endPos + Vector( 0, 0, HalfHumanHeight ), false, 10000.0f, true );
		}
		if ( !nearby )
		{
			nearby = TheNavMesh->GetNearestNavArea( m_editCursorPos );
		}
		if ( !nearby )
		{
			nearby = TheNavMesh->GetNearestNavArea( endPos );
		}

		CNavArea *newArea = CreateArea();
		if (newArea == NULL)
		{
			Warning( "NavEndArea: Out of memory\n" );
			player->EmitSound( "EDIT_END_AREA.NotCreating" );
			return;
		}
		
		newArea->Build( m_anchor, endPos );

		if ( nearby )
		{
			newArea->InheritAttributes( nearby );	// inherit from the nearby area
		}

		TheNavAreas.AddToTail( newArea );
		TheNavMesh->AddNavArea( newArea );
		player->EmitSound( "EDIT_END_AREA.Creating" );

		if ( nav_create_place_on_ground.GetBool() )
		{
			newArea->PlaceOnGround( NUM_CORNERS );
		}

		// if we have a marked area, inter-connect the two
		if (GetMarkedArea())
		{
			Extent extent;
			GetMarkedArea()->GetExtent( &extent );

			if (m_anchor.x > extent.hi.x && m_editCursorPos.x > extent.hi.x)
			{
				GetMarkedArea()->ConnectTo( newArea, EAST );
				newArea->ConnectTo( GetMarkedArea(), WEST );
			}
			else if (m_anchor.x < extent.lo.x && m_editCursorPos.x < extent.lo.x)
			{
				GetMarkedArea()->ConnectTo( newArea, WEST );
				newArea->ConnectTo( GetMarkedArea(), EAST );
			}
			else if (m_anchor.y > extent.hi.y && m_editCursorPos.y > extent.hi.y)
			{
				GetMarkedArea()->ConnectTo( newArea, SOUTH );
				newArea->ConnectTo( GetMarkedArea(), NORTH );
			}
			else if (m_anchor.y < extent.lo.y && m_editCursorPos.y < extent.lo.y)
			{
				GetMarkedArea()->ConnectTo( newArea, NORTH );
				newArea->ConnectTo( GetMarkedArea(), SOUTH );
			}

			// propogate marked area to new area
			SetMarkedArea( newArea );
		}

		TheNavMesh->OnEditCreateNotify( newArea );
	}
	else if ( IsEditMode( CREATING_LADDER ) )
	{
		SetEditMode( NORMAL );
	
		player->EmitSound( "EDIT_END_AREA.Creating" );

		Vector corner1, corner2, corner3;
		if ( m_climbableSurface && FindLadderCorners( &corner1, &corner2, &corner3 ) )
		{
			// m_ladderAnchor and corner2 are at the same Z, and corner1 & corner3 share Z.
			Vector top = (m_ladderAnchor + corner2) * 0.5f;
			Vector bottom = (corner1 + corner3) * 0.5f;
			if ( top.z < bottom.z )
			{
				Vector tmp = top;
				top = bottom;
				bottom = tmp;
			}

			float width = m_ladderAnchor.DistTo( corner2 );
			Vector2D ladderDir = m_surfaceNormal.AsVector2D();

			CreateLadder( top, bottom, width, ladderDir, HumanHeight );
		}
		else
		{
			player->EmitSound( "EDIT_END_AREA.NotCreating" );
		}
	}
	else
	{
		player->EmitSound( "EDIT_END_AREA.NotCreating" );
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavConnect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	Vector center;
	float halfWidth;
	if ( m_selectedSet.Count() > 1 )
	{
		bool bValid = true;
		for ( int i = 1; i < m_selectedSet.Count(); ++i )
		{
			// Make sure all connections are valid
			CNavArea *first = m_selectedSet[0];
			CNavArea *second = m_selectedSet[i];

			NavDirType dir = second->ComputeLargestPortal( first, &center, &halfWidth );
			if (dir == NUM_DIRECTIONS)
			{
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
				bValid = false;
				break;
			}

			dir = first->ComputeLargestPortal( second, &center, &halfWidth );
			if (dir == NUM_DIRECTIONS)
			{
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
				bValid = false;
				break;
			}
		}

		if ( bValid )
		{
			for ( int i = 1; i < m_selectedSet.Count(); ++i )
			{
				CNavArea *first = m_selectedSet[0];
				CNavArea *second = m_selectedSet[i];

				NavDirType dir = second->ComputeLargestPortal( first, &center, &halfWidth );
				second->ConnectTo( first, dir );

				dir = first->ComputeLargestPortal( second, &center, &halfWidth );
				first->ConnectTo( second, dir );
				player->EmitSound( "EDIT_CONNECT.Added" );
			}
		}
	}
	else if ( m_selectedArea )
	{
		if ( m_markedLadder )
		{
			m_markedLadder->ConnectTo( m_selectedArea );
			player->EmitSound( "EDIT_CONNECT.Added" );
		}
		else if ( m_markedArea )
		{
			NavDirType dir = GetMarkedArea()->ComputeLargestPortal( m_selectedArea, &center, &halfWidth );
			if (dir == NUM_DIRECTIONS)
			{
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
			}
			else
			{
				m_markedArea->ConnectTo( m_selectedArea, dir );
				player->EmitSound( "EDIT_CONNECT.Added" );
			}
		}
		else
		{
			if ( m_selectedSet.Count() == 1 )
			{
				CNavArea *area = m_selectedSet[0];
				NavDirType dir = area->ComputeLargestPortal( m_selectedArea, &center, &halfWidth );
				if (dir == NUM_DIRECTIONS)
				{
					player->EmitSound( "EDIT_CONNECT.AllDirections" );
				}
				else
				{
					area->ConnectTo( m_selectedArea, dir );
					player->EmitSound( "EDIT_CONNECT.Added" );
				}
			}
			else
			{
				Msg( "To connect areas, mark an area, highlight a second area, then invoke the connect command. Make sure the cursor is directly north, south, east, or west of the marked area." );
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
			}
		}
	}
	else if ( m_selectedLadder )
	{
		if ( m_markedArea )
		{
			m_markedArea->ConnectTo( m_selectedLadder );
			player->EmitSound( "EDIT_CONNECT.Added" );
		}
		else
		{
			Msg( "To connect areas, mark an area, highlight a second area, then invoke the connect command. Make sure the cursor is directly north, south, east, or west of the marked area." );
			player->EmitSound( "EDIT_CONNECT.AllDirections" );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
	ClearSelectedSet();
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavDisconnect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedSet.Count() > 1 )
	{
		bool bValid = true;
		for ( int i = 1; i < m_selectedSet.Count(); ++i )
		{
			// 2 areas are selected, so connect them bi-directionally
			CNavArea *first = m_selectedSet[0];
			CNavArea *second = m_selectedSet[i];
			if ( !first->IsConnected( second, NUM_DIRECTIONS ) && !second->IsConnected( first, NUM_DIRECTIONS ) )
			{
				player->EmitSound( "EDIT_CONNECT.AllDirections" );
				bValid = false;
				break;
			}
		}

		if ( bValid )
		{
			for ( int i = 1; i < m_selectedSet.Count(); ++i )
			{
				// 2 areas are selected, so connect them bi-directionally
				CNavArea *first = m_selectedSet[0];
				CNavArea *second = m_selectedSet[i];
				first->Disconnect( second );
				second->Disconnect( first );
			}
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
	}
	else if ( m_selectedArea )
	{
		if ( m_markedArea )
		{
			m_markedArea->Disconnect( m_selectedArea );
			m_selectedArea->Disconnect( m_markedArea );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		else if ( m_selectedSet.Count() == 1 )
		{
			m_selectedSet[0]->Disconnect( m_selectedArea );
			m_selectedArea->Disconnect( m_selectedSet[0] );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		else
		{
			if ( m_markedLadder )
			{
				m_markedLadder->Disconnect( m_selectedArea );
				m_selectedArea->Disconnect( m_markedLadder );
				player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
			}
			else
			{
				Msg( "To disconnect areas, mark an area, highlight a second area, then invoke the disconnect command. This will remove all connections between the two areas." );
				player->EmitSound( "EDIT_DISCONNECT.NoMarkedArea" );
			}
		}
	}
	else if ( m_selectedLadder )
	{
		if ( m_markedArea )
		{
			m_markedArea->Disconnect( m_selectedLadder );
			m_selectedLadder->Disconnect( m_markedArea );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		if ( m_selectedSet.Count() == 1 )
		{
			m_selectedSet[0]->Disconnect( m_selectedLadder );
			m_selectedLadder->Disconnect( m_selectedSet[0] );
			player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );
		}
		else
		{
			Msg( "To disconnect areas, mark an area, highlight a second area, then invoke the disconnect command. This will remove all connections between the two areas." );
			player->EmitSound( "EDIT_DISCONNECT.NoMarkedArea" );
		}
	}

	ClearSelectedSet();
	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
// Disconnect all outgoing one-way connects from each area in the selected set
void CNavMesh::CommandNavDisconnectOutgoingOneWays( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( !player )
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	if ( m_selectedSet.Count() == 0 )
	{
		FindActiveNavArea();

		if ( !m_selectedArea )
		{
			return;
		}

		m_selectedSet.AddToTail( m_selectedArea );
	}

	for ( int i = 0; i < m_selectedSet.Count(); ++i )
	{
		CNavArea *area = m_selectedSet[i];

		CUtlVector< CNavArea * > adjVector;
		area->CollectAdjacentAreas( &adjVector );

		for( int j=0; j<adjVector.Count(); ++j )
		{
			CNavArea *adj = adjVector[j];

			if ( !adj->IsConnected( area, NUM_DIRECTIONS ) )
			{
				// no connect back - this is a one-way connection
				area->Disconnect( adj );
			}
		}
	}
	player->EmitSound( "EDIT_DISCONNECT.MarkedArea" );

	ClearSelectedSet();
	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavSplice( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			if (m_selectedArea->SpliceEdit( GetMarkedArea() ))
				player->EmitSound( "EDIT_SPLICE.MarkedArea" );
			else
				player->EmitSound( "EDIT_SPLICE.NoMarkedArea" );
		}
		else
		{
			Msg( "To splice, mark an area, highlight a second area, then invoke the splice command to create an area between them" );
			player->EmitSound( "EDIT_SPLICE.NoMarkedArea" );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	ClearSelectedSet();
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Toggle an attribute on given area
 */
void CNavMesh::DoToggleAttribute( CNavArea *area, NavAttributeType attribute )
{
	area->SetAttributes( area->GetAttributes() ^ attribute );

	// keep a list of all "transient" nav areas
	if ( attribute == NAV_MESH_TRANSIENT )
	{
		if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
		{
			m_transientAreas.AddToTail( area );
		}
		else
		{
			m_transientAreas.FindAndRemove( area );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavToggleAttribute( NavAttributeType attribute )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	if ( IsSelectedSetEmpty() )
	{
		// the old way
		FindActiveNavArea();

		if ( m_selectedArea )
		{
			player->EmitSound( "EDIT.ToggleAttribute" );
			DoToggleAttribute( m_selectedArea, attribute );			
		}
	}
	else
	{
		// toggle the attribute in all areas in the selected set
		player->EmitSound( "EDIT.ToggleAttribute" );

		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];

			DoToggleAttribute( area, attribute );			
		}

		Msg( "Changed attribute in %d areas\n", m_selectedSet.Count() );

		ClearSelectedSet();		
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavTogglePlaceMode( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( IsEditMode( PLACE_PAINTING ) )
	{
		SetEditMode( NORMAL );
	}
	else
	{
		SetEditMode( PLACE_PAINTING );
	}

	player->EmitSound( "EDIT_TOGGLE_PLACE_MODE" );

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavPlaceFloodFill( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		PlaceFloodFillFunctor pff( m_selectedArea );
		SearchSurroundingAreas( m_selectedArea, m_selectedArea->GetCenter(), pff );
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavPlaceSet( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( PLACE_PAINTING ) )
		return;

	if ( !IsSelectedSetEmpty() )
	{
		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];
			area->SetPlace( TheNavMesh->GetNavPlace() );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavPlacePick( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		player->EmitSound( "EDIT_PLACE_PICK" );
		TheNavMesh->SetNavPlace( m_selectedArea->GetPlace() );
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavTogglePlacePainting( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( PLACE_PAINTING ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (m_isPlacePainting)
		{
			m_isPlacePainting = false;
			player->EmitSound( "Bot.EditSwitchOff" );
		}
		else
		{
			m_isPlacePainting = true;

			player->EmitSound( "Bot.EditSwitchOn" );

			// paint the initial area
			m_selectedArea->SetPlace( TheNavMesh->GetNavPlace() );
		}
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMarkUnnamed( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			player->EmitSound( "EDIT_MARK_UNNAMED.Enable" );
			SetMarkedArea( NULL );
		}
		else
		{
			SetMarkedArea( NULL );
			FOR_EACH_VEC( TheNavAreas, it )
			{
				CNavArea *area = TheNavAreas[ it ];

				if ( area->GetPlace() == 0 )
				{
					SetMarkedArea( area );
					break;
				}
			}
			if ( !GetMarkedArea() )
			{
				player->EmitSound( "EDIT_MARK_UNNAMED.NoMarkedArea" );
			}
			else
			{
				player->EmitSound( "EDIT_MARK_UNNAMED.MarkedArea" );

				int connected = 0;
				connected += GetMarkedArea()->GetAdjacentCount( NORTH );
				connected += GetMarkedArea()->GetAdjacentCount( SOUTH );
				connected += GetMarkedArea()->GetAdjacentCount( EAST );
				connected += GetMarkedArea()->GetAdjacentCount( WEST );

				int totalUnnamedAreas = 0;
				FOR_EACH_VEC( TheNavAreas, it )
				{
					CNavArea *area = TheNavAreas[ it ];
					if ( area->GetPlace() == 0 )
					{
						++totalUnnamedAreas;
					}
				}

				Msg( "Marked Area is connected to %d other Areas - there are %d total unnamed areas\n", connected, totalUnnamedAreas );
			}
		}
	}

	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerSelect( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedArea )
	{
		if (GetMarkedArea())
		{
			int corner = (m_markedCorner + 1) % (NUM_CORNERS + 1);
			m_markedCorner = (NavCornerType)corner;
			player->EmitSound( "EDIT_SELECT_CORNER.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_SELECT_CORNER.NoMarkedArea" );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerRaise( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	int amount = 1;
	if ( args.ArgC() > 1 )
	{
		amount = atoi( args[1] );
	}

	if (IsSelectedSetEmpty())
	{
		// the old way
		FindActiveNavArea();

		if ( m_selectedArea )
		{
			if (GetMarkedArea())
			{
				GetMarkedArea()->RaiseCorner( m_markedCorner, amount );
				player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
			}
			else
			{
				player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
			}
		}
	}
	else
	{
		// raise all areas in the selected set
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );

		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];

			area->RaiseCorner( NUM_CORNERS, amount, false );
		}

		Msg( "Raised %d areas\n", m_selectedSet.Count() );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerLower( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	int amount = -1;
	if ( args.ArgC() > 1 )
	{
		amount = -atoi( args[1] );
	}

	if (IsSelectedSetEmpty())
	{
		// the old way
		FindActiveNavArea();

		if ( m_selectedArea )
		{
			if (GetMarkedArea())
			{
				GetMarkedArea()->RaiseCorner( m_markedCorner, amount );
				player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
			}
			else
			{
				player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
			}
		}
	}
	else
	{
		// raise all areas in the selected set
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );

		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];

			area->RaiseCorner( NUM_CORNERS, amount, false );
		}

		Msg( "Lowered %d areas\n", m_selectedSet.Count() );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavCornerPlaceOnGround( const CCommand &args )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	float inset = 0.0f;
	if ( args.ArgC() == 2 )
	{
		inset = atof(args[1]);
	}

	if (IsSelectedSetEmpty())
	{
		// the old way
		FindActiveNavArea();

		if ( m_selectedArea )
		{
			if ( m_markedArea )
			{
				m_markedArea->PlaceOnGround( m_markedCorner, inset );
			}
			else
			{
				m_selectedArea->PlaceOnGround( NUM_CORNERS, inset );
			}
			player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
		}
		else
		{
			player->EmitSound( "EDIT_MOVE_CORNER.NoMarkedArea" );
		}
	}
	else
	{
		// snap all areas in the selected set to the ground
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );

		FOR_EACH_VEC( m_selectedSet, it )
		{
			CNavArea *area = m_selectedSet[ it ];

			area->PlaceOnGround( NUM_CORNERS, inset );
		}

		Msg( "Placed %d areas on the ground\n", m_selectedSet.Count() );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavWarpToMark( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	CNavArea *targetArea = GetMarkedArea();
	if ( !targetArea && !IsSelectedSetEmpty() )
	{
		targetArea = m_selectedSet[0];
	}

	if ( targetArea )
	{
		Vector origin = targetArea->GetCenter() + Vector( 0, 0, 0.75f * HumanHeight );
		QAngle angles = player->GetAbsAngles();

		if ( ( player->IsDead() || player->IsObserver() ) && player->GetObserverMode() == OBS_MODE_ROAMING )
		{
			UTIL_SetOrigin( player, origin );
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
		else
		{
			player->Teleport( &origin, &angles, &vec3_origin );
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
	}
	else if ( GetMarkedLadder() )
	{
		CNavLadder *ladder = GetMarkedLadder();

		QAngle angles = player->GetAbsAngles();
		Vector origin = (ladder->m_top + ladder->m_bottom)/2;
		origin.x += ladder->GetNormal().x * GenerationStepSize;
		origin.y += ladder->GetNormal().y * GenerationStepSize;

		if ( ( player->IsDead() || player->IsObserver() ) && player->GetObserverMode() == OBS_MODE_ROAMING )
		{
			UTIL_SetOrigin( player, origin );
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
		else
		{
			player->Teleport( &origin, &angles, &vec3_origin );
			player->EmitSound( "EDIT_WARP_TO_MARK" );
		}
	}
	else
	{
		player->EmitSound( "EDIT_WARP_TO_MARK" );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavLadderFlip( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if (player == NULL)
		return;

	if ( !IsEditMode( NORMAL ) )
		return;

	FindActiveNavArea();

	if ( m_selectedLadder )
	{
		CNavArea *area;

		// flip direction
		player->EmitSound( "EDIT_MOVE_CORNER.MarkedArea" );
		m_selectedLadder->SetDir( OppositeDirection( m_selectedLadder->GetDir() ) );

		// and reverse ladder's area pointers
		area = m_selectedLadder->m_topBehindArea;
		m_selectedLadder->m_topBehindArea = m_selectedLadder->m_topForwardArea;
		m_selectedLadder->m_topForwardArea = area;

		area = m_selectedLadder->m_topRightArea;
		m_selectedLadder->m_topRightArea = m_selectedLadder->m_topLeftArea;
		m_selectedLadder->m_topLeftArea = area;
	}

	SetMarkedArea( NULL );			// unmark the mark area
	m_markedCorner = NUM_CORNERS;	// clear the corner selection
}


//--------------------------------------------------------------------------------------------------------------
class RadiusSelect
{
	Vector m_origin;
	float m_radiusSquared;
	int m_selected;

public:
	RadiusSelect( const Vector &origin, float radius )
	{
		m_origin = origin;
		m_radiusSquared = radius * radius;
		m_selected = 0;
	}

	bool operator()( CNavArea *area )
	{
		if ( TheNavMesh->IsInSelectedSet( area ) )
			return true;

		Vector close;
		area->GetClosestPointOnArea( m_origin, &close );
		if ( close.DistToSqr( m_origin ) < m_radiusSquared )
		{
			TheNavMesh->AddToSelectedSet( area );
			++m_selected;
		}

		return true;
	}

	int GetNumSelected( void ) const
	{
		return m_selected;
	}
};


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_radius, "Adds all areas in a radius to the selection set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || engine->IsDedicatedServer() )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "Needs a radius\n" );
		return;
	}

	float radius = atof( args[ 1 ] );
	CBasePlayer *host = UTIL_GetListenServerHost();
	if ( !host )
		return;

	RadiusSelect select( host->GetAbsOrigin(), radius );
	TheNavMesh->ForAllAreas( select );

	Msg( "%d areas added to selection\n", select.GetNumSelected() );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add area to the currently selected set
 */
void CNavMesh::AddToSelectedSet( CNavArea *area )
{
	if ( !area )
		return;

	// make sure area is not already in list
	if (m_selectedSet.Find( area ) != m_selectedSet.InvalidIndex())
		return;
		
	m_selectedSet.AddToTail( area );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Remove area from the currently selected set
 */
void CNavMesh::RemoveFromSelectedSet( CNavArea *area )
{
	m_selectedSet.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add area to the drag selection set
 */
void CNavMesh::AddToDragSelectionSet( CNavArea *area )
{
	if ( !area )
		return;

	// make sure area is not already in list
	if (m_dragSelectionSet.Find( area ) != m_dragSelectionSet.InvalidIndex())
		return;
		
	m_dragSelectionSet.AddToTail( area );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Remove area from the drag selection set
 */
void CNavMesh::RemoveFromDragSelectionSet( CNavArea *area )
{
	m_dragSelectionSet.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Clear the currently selected set to empty
 */
void CNavMesh::ClearDragSelectionSet( void )
{
	m_dragSelectionSet.RemoveAll();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Clear the currently selected set to empty
 */
void CNavMesh::ClearSelectedSet( void )
{
	m_selectedSet.RemoveAll();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if the selected set is empty
 */
bool CNavMesh::IsSelectedSetEmpty( void ) const
{
	return (m_selectedSet.Count() == 0);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return size of the selected set
 */
int CNavMesh::GetSelecteSetSize( void ) const
{
	return m_selectedSet.Count();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the selected set
 */
const NavAreaVector &CNavMesh::GetSelectedSet( void ) const
{
	return m_selectedSet;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if the given area is in the selected set
 */
bool CNavMesh::IsInSelectedSet( const CNavArea *area ) const
{
	FOR_EACH_VEC( m_selectedSet, it )
	{
		const CNavArea *setArea = m_selectedSet[ it ];
		
		if (setArea == area)
			return true;
	}
	
	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when given area has just been added to the mesh in edit mode
 */
void CNavMesh::OnEditCreateNotify( CNavArea *newArea )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		TheNavAreas[ it ]->OnEditCreateNotify( newArea );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when given area has just been deleted from the mesh in edit mode
 */
void CNavMesh::OnEditDestroyNotify( CNavArea *deadArea )
{
	// clean up any edit hooks
	m_markedArea = NULL;
	m_selectedArea = NULL;
	m_lastSelectedArea = NULL;
	m_selectedLadder = NULL;
	m_lastSelectedLadder = NULL;
	m_markedLadder = NULL;

	m_avoidanceObstacleAreas.FindAndRemove( deadArea );
	m_blockedAreas.FindAndRemove( deadArea );

	FOR_EACH_VEC( TheNavAreas, it )
	{
		TheNavAreas[ it ]->OnEditDestroyNotify( deadArea );
	}

	EditDestroyNotification notification( deadArea );
	ForEachActor( notification );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when given ladder has just been deleted from the mesh in edit mode
 * @TODO: Implement me
 */
void CNavMesh::OnEditDestroyNotify( CNavLadder *deadLadder )
{
}




