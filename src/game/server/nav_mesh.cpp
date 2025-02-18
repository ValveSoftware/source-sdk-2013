//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// NavMesh.cpp
// Implementation of Navigation Mesh interface
// Author: Michael S. Booth, 2003-2004

#include "cbase.h"
#include "filesystem.h"
#include "nav_mesh.h"
#include "nav_node.h"
#include "fmtstr.h"
#include "utlbuffer.h"
#include "tier0/vprof.h"
#ifdef TERROR
#include "func_simpleladder.h"
#endif
#include "functorutils.h"

#ifdef NEXT_BOT
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


#define DrawLine( from, to, duration, red, green, blue )		NDebugOverlay::Line( from, to, red, green, blue, true, NDEBUG_PERSIST_TILL_NEXT_SERVER )


/**
 * The singleton for accessing the navigation mesh
 */
CNavMesh *TheNavMesh = NULL;

ConVar nav_edit( "nav_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to interactively edit the Navigation Mesh. Set to zero to leave edit mode." );
ConVar nav_quicksave( "nav_quicksave", "1", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to skip the time consuming phases of the analysis.  Useful for data collection and testing." );	// TERROR: defaulting to 1, since we don't need the other data
ConVar nav_show_approach_points( "nav_show_approach_points", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show Approach Points in the Navigation Mesh." );
ConVar nav_show_danger( "nav_show_danger", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show current 'danger' levels." );
ConVar nav_show_player_counts( "nav_show_player_counts", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show current player counts in each area." );
ConVar nav_show_func_nav_avoid( "nav_show_func_nav_avoid", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot avoidance due to func_nav_avoid entities" );
ConVar nav_show_func_nav_prefer( "nav_show_func_nav_prefer", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot preference due to func_nav_prefer entities" );
ConVar nav_show_func_nav_prerequisite( "nav_show_func_nav_prerequisite", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Show areas of designer-placed bot preference due to func_nav_prerequisite entities" );
ConVar nav_max_vis_delta_list_length( "nav_max_vis_delta_list_length", "64", FCVAR_CHEAT );

extern ConVar nav_show_potentially_visible;

int g_DebugPathfindCounter = 0;


bool FindGroundForNode( Vector *pos, Vector *normal );


//--------------------------------------------------------------------------------------------------------------
CNavMesh::CNavMesh( void )
{
	m_spawnName = NULL;
	m_gridCellSize = 300.0f;
	m_editMode = NORMAL;
	m_bQuitWhenFinished = false;
	m_hostThreadModeRestoreValue = 0;
	m_placeCount = 0;
	m_placeName = NULL;

	LoadPlaceDatabase();

	ListenForGameEvent( "round_start" );
//	ListenForGameEvent( "round_start_pre_entity" );
	ListenForGameEvent( "break_prop" );
	ListenForGameEvent( "break_breakable" );
	ListenForGameEvent( "teamplay_round_start" );
		
	Reset();
}

//--------------------------------------------------------------------------------------------------------------
CNavMesh::~CNavMesh()
{
	if (m_spawnName)
		delete [] m_spawnName;

 // !!!!bug!!! why does this crash in linux on server exit
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		delete [] m_placeName[i];
	}

}

//--------------------------------------------------------------------------------------------------------------
/**
 * Reset the Navigation Mesh to initial values
 */
void CNavMesh::Reset( void )
{
	DestroyNavigationMesh();

	m_generationMode = GENERATE_NONE;
	m_currentNode = NULL;
	ClearWalkableSeeds();

	m_isAnalyzed = false;
	m_isOutOfDate = false;
	m_isEditing = false;
	m_navPlace = UNDEFINED_PLACE;
	m_markedArea = NULL;
	m_selectedArea = NULL;
	m_bQuitWhenFinished = false;

	m_editMode = NORMAL;

	m_lastSelectedArea = NULL;
	m_isPlacePainting = false;

	m_climbableSurface = false;
	m_markedLadder = NULL;
	m_selectedLadder = NULL;

	m_updateBlockedAreasTimer.Invalidate();

	if (m_spawnName)
	{
		delete [] m_spawnName;
	}

	m_spawnName = NULL;

	m_walkableSeeds.RemoveAll();
}


//--------------------------------------------------------------------------------------------------------------
CNavArea *CNavMesh::GetMarkedArea( void ) const
{
	if ( m_markedArea )
	{
		return m_markedArea;
	}

	if ( m_selectedSet.Count() == 1 )
	{
		return m_selectedSet[0];
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Free all resources of the mesh and reset it to empty state
 */
void CNavMesh::DestroyNavigationMesh( bool incremental )
{
	m_blockedAreas.RemoveAll();
	m_avoidanceObstacleAreas.RemoveAll();
	m_transientAreas.RemoveAll();

	if ( !incremental )
	{
		// destroy all areas
		CNavArea::m_isReset = true;

		// tell players to forget about the areas
		FOR_EACH_VEC( TheNavAreas, it )
		{
			EditDestroyNotification notification( TheNavAreas[it] );
			ForEachActor( notification );
		}

		// remove each element of the list and delete them
		FOR_EACH_VEC( TheNavAreas, it )
		{
			DestroyArea( TheNavAreas[ it ] );
		}

		TheNavAreas.RemoveAll();

		CNavArea::m_isReset = false;


		// destroy ladder representations
		DestroyLadders();
	}
	else
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			TheNavAreas[ it ]->ResetNodes();
		}
	}

	// destroy all hiding spots
	DestroyHidingSpots();

	// destroy navigation nodes created during map generation
	CNavNode::CleanupGeneration();

	if ( !incremental )
	{
		// destroy the grid
		m_grid.RemoveAll();
		m_gridSizeX = 0;
		m_gridSizeY = 0;
	}

	// clear the hash table
	for( int i=0; i<HASH_TABLE_SIZE; ++i )
	{
		m_hashTable[i] = NULL;
	}

	if ( !incremental )
	{
		m_areaCount = 0;
	}

	if ( !incremental )
	{
		// Reset the next area and ladder IDs to 1
		CNavArea::CompressIDs();
		CNavLadder::CompressIDs();
	}

	SetEditMode( NORMAL );

	m_markedArea = NULL;
	m_selectedArea = NULL;
	m_lastSelectedArea = NULL;
	m_climbableSurface = false;
	m_markedLadder = NULL;
	m_selectedLadder = NULL;

	if ( !incremental )
	{
		m_isLoaded = false;
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked on each game frame
 */
void CNavMesh::Update( void )
{
	VPROF( "CNavMesh::Update" );

	if (IsGenerating())
	{
		UpdateGeneration( 0.03 );
		return; // don't bother trying to draw stuff while we're generating
	}

	// Test all of the areas for blocked status
	if ( m_updateBlockedAreasTimer.HasStarted() && m_updateBlockedAreasTimer.IsElapsed() )
	{
		TestAllAreasForBlockedStatus();
		m_updateBlockedAreasTimer.Invalidate();
	}

	UpdateBlockedAreas();
	UpdateAvoidanceObstacleAreas();

	if (nav_edit.GetBool())
	{
		if (m_isEditing == false)
		{
			OnEditModeStart();
			m_isEditing = true;
		}

		DrawEditMode();
	}
	else
	{
		if (m_isEditing)
		{
			OnEditModeEnd();
			m_isEditing = false;
		}
	}

	if (nav_show_danger.GetBool())
	{
		DrawDanger();
	}

	if (nav_show_player_counts.GetBool())
	{
		DrawPlayerCounts();
	}

	if ( nav_show_func_nav_avoid.GetBool() )
	{
		DrawFuncNavAvoid();
	}

	if ( nav_show_func_nav_prefer.GetBool() )
	{
		DrawFuncNavPrefer();
	}

#ifdef NEXT_BOT
	if ( nav_show_func_nav_prerequisite.GetBool() )
	{
		DrawFuncNavPrerequisite();
	}
#endif

	if ( nav_show_potentially_visible.GetBool() )
	{
		CBasePlayer *player = UTIL_GetListenServerHost();
		if ( player && player->GetLastKnownArea() )
		{
			CNavArea *eyepointArea = player->GetLastKnownArea();
			if ( eyepointArea )
			{
				FOR_EACH_VEC( TheNavAreas, it )
				{
					CNavArea *area = TheNavAreas[it];

					if ( eyepointArea->IsCompletelyVisible( area ) )
					{
						area->DrawFilled( 100, 100, 200, 255 );
					}
					else if ( eyepointArea->IsPotentiallyVisible( area ) && nav_show_potentially_visible.GetInt() == 1 )
					{
						area->DrawFilled( 100, 200, 100, 255 );
					}
				}
			}
		}
	}

	// draw any walkable seeds that have been marked
	for ( int it=0; it < m_walkableSeeds.Count(); ++it )
	{
		WalkableSeedSpot spot = m_walkableSeeds[ it ];

		const float height = 50.0f;
		const float width = 25.0f;
		DrawLine( spot.pos, spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( width, 0, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( -width, 0, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( 0, width, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
		DrawLine( spot.pos + Vector( 0, -width, 0 ), spot.pos + height * spot.normal, 3, 255, 0, 255 ); 
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check all nav areas inside the breakable's extent to see if players would now fall through
 */
class CheckAreasOverlappingBreakable
{
public:
	CheckAreasOverlappingBreakable( CBaseEntity *breakable )
	{
		m_breakable = breakable;
		ICollideable *collideable = breakable->GetCollideable();
		collideable->WorldSpaceSurroundingBounds( &m_breakableExtent.lo, &m_breakableExtent.hi );

		const float expand = 10.0f;
		m_breakableExtent.lo += Vector( -expand, -expand, -expand );
		m_breakableExtent.hi += Vector(  expand,  expand,  expand );
	}

	bool operator() ( CNavArea *area )
	{
		if ( area->IsOverlapping( m_breakableExtent ) )
		{
			// area overlaps the breakable
			area->CheckFloor( m_breakable );
		}

		return true;
	}

private:
	Extent m_breakableExtent;
	CBaseEntity *m_breakable;
};


//--------------------------------------------------------------------------------------------------------------
class NavRoundRestart
{
public:
	bool operator()( CNavArea *area )
	{
		area->OnRoundRestart();
		return true;
	}

	bool operator()( CNavLadder *ladder )
	{
		ladder->OnRoundRestart();
		return true;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when the round restarts
 */
void CNavMesh::FireGameEvent( IGameEvent *gameEvent )
{
	VPROF_BUDGET( "CNavMesh::FireGameEvent", VPROF_BUDGETGROUP_NPCS );

	if ( FStrEq( gameEvent->GetName(), "break_prop" ) || FStrEq( gameEvent->GetName(), "break_breakable" ) )
	{
		CheckAreasOverlappingBreakable collector( UTIL_EntityByIndex( gameEvent->GetInt( "entindex" ) ) );
		ForAllAreas( collector );
	}

	if ( FStrEq( gameEvent->GetName(), "round_start" ) || FStrEq( gameEvent->GetName(), "teamplay_round_start" ) )
	{
		OnRoundRestart();
		
		NavRoundRestart restart;
		ForAllAreas( restart );
		ForAllLadders( restart );
	}
	else if ( FStrEq( gameEvent->GetName(), "round_start_pre_entity" ) )
	{
		OnRoundRestartPreEntity();

		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];
			area->OnRoundRestartPreEntity();
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Allocate the grid and define its extents
 */
void CNavMesh::AllocateGrid( float minX, float maxX, float minY, float maxY )
{
	m_grid.RemoveAll();

	m_minX = minX;
	m_minY = minY;

	m_gridSizeX = (int)((maxX - minX) / m_gridCellSize) + 1;
	m_gridSizeY = (int)((maxY - minY) / m_gridCellSize) + 1;

	m_grid.SetCount( m_gridSizeX * m_gridSizeY );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Add an area to the mesh
 */
void CNavMesh::AddNavArea( CNavArea *area )
{
	if ( !m_grid.Count() )
	{
		// If we somehow have no grid (manually creating a nav area without loading or generating a mesh), don't crash
		AllocateGrid( 0, 0, 0, 0 );
	}

	// add to grid
	int loX = WorldToGridX( area->GetCorner( NORTH_WEST ).x );
	int loY = WorldToGridY( area->GetCorner( NORTH_WEST ).y );
	int hiX = WorldToGridX( area->GetCorner( SOUTH_EAST ).x );
	int hiY = WorldToGridY( area->GetCorner( SOUTH_EAST ).y );

	for( int y = loY; y <= hiY; ++y )
	{
		for( int x = loX; x <= hiX; ++x )
		{
			m_grid[ x + y*m_gridSizeX ].AddToTail( const_cast<CNavArea *>( area ) );
		}
	}

	// add to hash table
	int key = ComputeHashKey( area->GetID() );

	if (m_hashTable[key])
	{
		// add to head of list in this slot
		area->m_prevHash = NULL;
		area->m_nextHash = m_hashTable[key];
		m_hashTable[key]->m_prevHash = area;
		m_hashTable[key] = area;
	}
	else
	{
		// first entry in this slot
		m_hashTable[key] = area;
		area->m_nextHash = NULL;
		area->m_prevHash = NULL;
	}

	if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
	{
		m_transientAreas.AddToTail( area );
	}

	++m_areaCount;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Remove an area from the mesh
 */
void CNavMesh::RemoveNavArea( CNavArea *area )
{
	// add to grid
	int loX = WorldToGridX( area->GetCorner( NORTH_WEST ).x );
	int loY = WorldToGridY( area->GetCorner( NORTH_WEST ).y );
	int hiX = WorldToGridX( area->GetCorner( SOUTH_EAST ).x );
	int hiY = WorldToGridY( area->GetCorner( SOUTH_EAST ).y );

	for( int y = loY; y <= hiY; ++y )
	{
		for( int x = loX; x <= hiX; ++x )
		{
			m_grid[ x + y*m_gridSizeX ].FindAndRemove( area );
		}
	}

	// remove from hash table
	int key = ComputeHashKey( area->GetID() );

	if (area->m_prevHash)
	{
		area->m_prevHash->m_nextHash = area->m_nextHash;
	}
	else
	{
		// area was at start of list
		m_hashTable[key] = area->m_nextHash;

		if (m_hashTable[key])
		{
			m_hashTable[key]->m_prevHash = NULL;
		}
	}

	if (area->m_nextHash)
	{
		area->m_nextHash->m_prevHash = area->m_prevHash;
	}

	if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
	{
		BuildTransientAreaList();
	}

	m_avoidanceObstacleAreas.FindAndRemove( area );
	m_blockedAreas.FindAndRemove( area );

	--m_areaCount;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when server loads a new map
 */
void CNavMesh::OnServerActivate( void )
{
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->OnServerActivate();
	}
}

#ifdef NEXT_BOT

//--------------------------------------------------------------------------------------------------------------
class CRegisterPrerequisite
{
public:
	CRegisterPrerequisite( CFuncNavPrerequisite *prereq )
	{
		m_prereq = prereq;
	}

	bool operator() ( CNavArea *area )
	{
		area->AddPrerequisite( m_prereq );
		return true;
	}

	CFuncNavPrerequisite *m_prereq;
};

#endif

//--------------------------------------------------------------------------------------------------------------
/**
* Test all areas for blocked status
*/
void CNavMesh::TestAllAreasForBlockedStatus( void )
{
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->UpdateBlocked( true );
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when a game round restarts
 */
void CNavMesh::OnRoundRestart( void )
{
	m_updateBlockedAreasTimer.Start( 1.0f );

#ifdef NEXT_BOT
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->RemoveAllPrerequisites();
	}

	// attach prerequisites
	for ( int i=0; i<IFuncNavPrerequisiteAutoList::AutoList().Count(); ++i )
	{
		CFuncNavPrerequisite *prereq = static_cast< CFuncNavPrerequisite* >( IFuncNavPrerequisiteAutoList::AutoList()[i] );

		Extent prereqExtent;
		prereqExtent.Init( prereq );

		CRegisterPrerequisite apply( prereq );

		ForAllAreasOverlappingExtent( apply, prereqExtent );
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when a game round restarts, but before entities are deleted and recreated
 */
void CNavMesh::OnRoundRestartPreEntity( void )
{
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::BuildTransientAreaList( void )
{
	m_transientAreas.RemoveAll();

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->GetAttributes() & NAV_MESH_TRANSIENT )
		{
			m_transientAreas.AddToTail( area );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavMesh::GridToWorld( int gridX, int gridY, Vector *pos ) const
{
	gridX = clamp( gridX, 0, m_gridSizeX-1 );
	gridY = clamp( gridY, 0, m_gridSizeY-1 );

	pos->x = m_minX + gridX * m_gridCellSize;
	pos->y = m_minY + gridY * m_gridCellSize;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position, return the nav area that IsOverlapping and is *immediately* beneath it
 */
CNavArea *CNavMesh::GetNavArea( const Vector &pos, float beneathLimit ) const
{
	VPROF_BUDGET( "CNavMesh::GetNavArea", "NextBot"  );

	if ( !m_grid.Count() )
		return NULL;

	// get list in cell that contains position
	int x = WorldToGridX( pos.x );
	int y = WorldToGridY( pos.y );
	NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

	// search cell list to find correct area
	CNavArea *use = NULL;
	float useZ = -99999999.9f;
	Vector testPos = pos + Vector( 0, 0, 5 );

	FOR_EACH_VEC( (*areaVector), it )
	{
		CNavArea *area = (*areaVector)[ it ];

		// check if position is within 2D boundaries of this area
		if (area->IsOverlapping( testPos ))
		{
			// project position onto area to get Z
			float z = area->GetZ( testPos );

			// if area is above us, skip it
			if (z > testPos.z)
				continue;

			// if area is too far below us, skip it
			if (z < pos.z - beneathLimit)
				continue;

			// if area is higher than the one we have, use this instead
			if (z > useZ)
			{
				use = area;
				useZ = z;
			}
		}
	}

	return use;
}


//----------------------------------------------------------------------------
// Given a position, return the nav area that IsOverlapping and is *immediately* beneath it
//----------------------------------------------------------------------------
CNavArea *CNavMesh::GetNavArea( CBaseEntity *pEntity, int nFlags, float flBeneathLimit ) const
{
	VPROF( "CNavMesh::GetNavArea [ent]" );

	if ( !m_grid.Count() )
		return NULL;

	Vector testPos = pEntity->GetAbsOrigin();

	float flStepHeight = 1e-3;
	CBaseCombatCharacter *pBCC = pEntity->MyCombatCharacterPointer();
	if ( pBCC )
	{
		// Check if we're still in the last area
		CNavArea *pLastNavArea = pBCC->GetLastKnownArea();
		if ( pLastNavArea && pLastNavArea->IsOverlapping( testPos ) )
		{
			float flZ = pLastNavArea->GetZ( testPos );
			if ( ( flZ <= testPos.z + StepHeight ) && ( flZ >= testPos.z - StepHeight ) )
				return pLastNavArea;
		}
		flStepHeight = StepHeight;
	}

	// get list in cell that contains position
	int x = WorldToGridX( testPos.x );
	int y = WorldToGridY( testPos.y );
	NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

	// search cell list to find correct area
	CNavArea *use = NULL;
	float useZ = -99999999.9f;

	bool bSkipBlockedAreas = ( ( nFlags & GETNAVAREA_ALLOW_BLOCKED_AREAS ) == 0 );
	FOR_EACH_VEC( (*areaVector), it )
	{
		CNavArea *pArea = (*areaVector)[ it ];

		// check if position is within 2D boundaries of this area
		if ( !pArea->IsOverlapping( testPos ) )
			continue;

		// don't consider blocked areas
		if ( bSkipBlockedAreas && pArea->IsBlocked( pEntity->GetTeamNumber() ) )
			continue;

		// project position onto area to get Z
		float z = pArea->GetZ( testPos );

		// if area is above us, skip it
		if ( z > testPos.z + flStepHeight )
			continue;

		// if area is too far below us, skip it
		if ( z < testPos.z - flBeneathLimit )
			continue;

		// if area is lower than the one we have, skip it
		if ( z <= useZ )
			continue;

		use = pArea;
		useZ = z;
	}

	// Check LOS if necessary
	if ( use && ( nFlags && GETNAVAREA_CHECK_LOS ) && ( useZ < testPos.z - flStepHeight ) )
	{
		// trace directly down to see if it's below us and unobstructed
		trace_t result;
		UTIL_TraceLine( testPos, Vector( testPos.x, testPos.y, useZ ), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );
		if ( ( result.fraction != 1.0f ) && ( fabs( result.endpos.z - useZ ) > flStepHeight ) )
			return NULL;
	}
	return use;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position in the world, return the nav area that is closest
 * and at the same height, or beneath it.
 * Used to find initial area if we start off of the mesh.
 * @todo Make sure area is not on the other side of the wall from goal.
 */
CNavArea *CNavMesh::GetNearestNavArea( const Vector &pos, bool anyZ, float maxDist, bool checkLOS, bool checkGround, int team ) const
{
	VPROF_BUDGET( "CNavMesh::GetNearestNavArea", "NextBot" );

	if ( !m_grid.Count() )
		return NULL;	

	CNavArea *close = NULL;
	float closeDistSq = maxDist * maxDist;

	// quick check
	if ( !checkLOS && !checkGround )
	{
		close = GetNavArea( pos );
		if ( close )
		{
			return close;
		}
	}

	// ensure source position is well behaved
	Vector source;
	source.x = pos.x;
	source.y = pos.y;
	if ( GetGroundHeight( pos, &source.z ) == false )
	{
		if ( !checkGround )
		{
			source.z = pos.z;
		}
		else
		{
			return NULL;
		}
	}

	source.z += HalfHumanHeight;

	// find closest nav area

	// use a unique marker for this method, so it can be used within a SearchSurroundingArea() call
	static unsigned int searchMarker = RandomInt(0, 1024*1024 );

	++searchMarker;

	if ( searchMarker == 0 )
	{
		++searchMarker;
	}


	// get list in cell that contains position
	int originX = WorldToGridX( pos.x );
	int originY = WorldToGridY( pos.y );

	int shiftLimit = ceil(maxDist / m_gridCellSize);

	//
	// Search in increasing rings out from origin, starting with cell
	// that contains the given position.
	// Once we find a close area, we must check one more step out in
	// case our position is just against the edge of the cell boundary
	// and an area in an adjacent cell is actually closer.
	// 
	for( int shift=0; shift <= shiftLimit; ++shift )
	{
		for( int x = originX - shift; x <= originX + shift; ++x )
		{
			if ( x < 0 || x >= m_gridSizeX )
				continue;

			for( int y = originY - shift; y <= originY + shift; ++y )
			{
				if ( y < 0 || y >= m_gridSizeY )
					continue;

				// only check these areas if we're on the outer edge of our spiral
				if ( x > originX - shift &&
					 x < originX + shift &&
					 y > originY - shift &&
					 y < originY + shift )
					continue;

				NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

				// find closest area in this cell
				FOR_EACH_VEC( (*areaVector), it )
				{
					CNavArea *area = (*areaVector)[ it ];

					// skip if we've already visited this area
					if ( area->m_nearNavSearchMarker == searchMarker )
						continue;

					// don't consider blocked areas
					if ( area->IsBlocked( team ) )
						continue;

					// mark as visited
					area->m_nearNavSearchMarker = searchMarker;

					Vector areaPos;
					area->GetClosestPointOnArea( source, &areaPos );

					// TERROR: Using the original pos for distance calculations.  Since it's a pure 3D distance,
					// with no Z restrictions or LOS checks, this should work for passing in bot foot positions.
					// This needs to be ported back to CS:S.
					float distSq = ( areaPos - pos ).LengthSqr();

					// keep the closest area
					if ( distSq >= closeDistSq )
						continue;

					// check LOS to area
					// REMOVED: If we do this for !anyZ, it's likely we wont have LOS and will enumerate every area in the mesh
					// It is still good to do this in some isolated cases, however
					if ( checkLOS )
					{
						trace_t result;

						// make sure 'pos' is not embedded in the world
						Vector safePos;

						UTIL_TraceLine( pos, pos + Vector( 0, 0, StepHeight ), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );
						if ( result.startsolid )
						{
							// it was embedded - move it out
							safePos = result.endpos + Vector( 0, 0, 1.0f );
						}
						else
						{
							safePos = pos;
						}

						// Don't bother tracing from the nav area up to safePos.z if it's within StepHeight of the area, since areas can be embedded in the ground a bit
						float heightDelta = fabs(areaPos.z - safePos.z);
						if ( heightDelta > StepHeight )
						{
							// trace to the height of the original point
							UTIL_TraceLine( areaPos + Vector( 0, 0, StepHeight ), Vector( areaPos.x, areaPos.y, safePos.z ), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );
							
							if ( result.fraction != 1.0f )
							{
								continue;
							}
						}

						// trace to the original point's height above the area
						UTIL_TraceLine( safePos, Vector( areaPos.x, areaPos.y, safePos.z + StepHeight ), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );

						if ( result.fraction != 1.0f )
						{
							continue;
						}
					}

					closeDistSq = distSq;
					close = area;

					// look one more step outwards
					shiftLimit = shift+1;
				}
			}
		}
	}

	return close;
}


//----------------------------------------------------------------------------
// Given a position in the world, return the nav area that is closest
// and at the same height, or beneath it.
// Used to find initial area if we start off of the mesh.
// @todo Make sure area is not on the other side of the wall from goal.
//----------------------------------------------------------------------------
CNavArea *CNavMesh::GetNearestNavArea( CBaseEntity *pEntity, int nFlags, float maxDist ) const
{
	VPROF( "CNavMesh::GetNearestNavArea [ent]" );

	if ( !m_grid.Count() )
		return NULL;

	// quick check
	CNavArea *pClose = GetNavArea( pEntity, nFlags );
	if ( pClose )
		return pClose;

	bool bCheckLOS = ( nFlags & GETNAVAREA_CHECK_LOS ) != 0;
	bool bCheckGround = ( nFlags & GETNAVAREA_CHECK_GROUND ) != 0;
	return GetNearestNavArea( pEntity->GetAbsOrigin(), false, maxDist, bCheckLOS, bCheckGround, pEntity->GetTeamNumber() );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given an ID, return the associated area
 */
CNavArea *CNavMesh::GetNavAreaByID( unsigned int id ) const
{
	if (id == 0)
		return NULL;

	int key = ComputeHashKey( id );

	for( CNavArea *area = m_hashTable[key]; area; area = area->m_nextHash )
	{
		if (area->GetID() == id)
		{
			return area;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given an ID, return the associated ladder
 */
CNavLadder *CNavMesh::GetLadderByID( unsigned int id ) const
{
	if (id == 0)
		return NULL;

	for ( int i=0; i<m_ladders.Count(); ++i )
	{
		CNavLadder *ladder = m_ladders[i];
		if ( ladder->GetID() == id )
		{
			return ladder;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return radio chatter place for given coordinate
 */
unsigned int CNavMesh::GetPlace( const Vector &pos ) const
{
	CNavArea *area = GetNearestNavArea( pos, true );

	if (area)
	{
		return area->GetPlace();
	}

	return UNDEFINED_PLACE;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Load the place names from a file
 */
void CNavMesh::LoadPlaceDatabase( void )
{
	m_placeCount = 0;

#ifdef TERROR
	// TODO: LoadPlaceDatabase happens during the constructor, so we can't override it!
	// Population.txt holds all the info we need for place names in Left4Dead, so let's not
	// make Phil edit yet another text file.
	KeyValues *populationData = new KeyValues( "population" );
	if ( populationData->LoadFromFile( filesystem, "scripts/population.txt" ) )
	{
		CUtlVector< char * > placeNames;

		for ( KeyValues *key = populationData->GetFirstTrueSubKey(); key != NULL; key = key->GetNextTrueSubKey() )
		{
			if ( FStrEq( key->GetName(), "default" ) )	// default population is the undefined place
				continue;

			placeNames.AddToTail( CloneString( key->GetName() ) );
		}

		m_placeCount = placeNames.Count();

		// allocate place name array
		m_placeName = new char * [ m_placeCount ];
		for ( unsigned int i=0; i<m_placeCount; ++i )
		{
			m_placeName[i] = placeNames[i];
		}

		populationData->deleteThis();
		return;
	}

	populationData->deleteThis();
#endif

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	filesystem->ReadFile("NavPlace.db", "GAME", buf);

	if (!buf.Size())
		return;

	const int maxNameLength = 128;
	char buffer[ maxNameLength ];

	CUtlVector<char*> placeNames;

	// count the number of places
	while( true )
	{
		buf.GetLine( buffer, maxNameLength );

		if ( !buf.IsValid() )
			break;

		int len = V_strlen( buffer );
		if ( len >= 2 )
		{
			if ( buffer[len-1] == '\n' || buffer[len-1] == '\r' )
				buffer[len-1] = 0;
			
			if ( buffer[len-2] == '\r' )
				buffer[len-2] = 0;

			char *pName = new char[ len + 1 ];
			V_strncpy( pName, buffer, len+1 );
			placeNames.AddToTail( pName );
		}
	}

	// allocate place name array
	m_placeCount = placeNames.Count();
	m_placeName = new char * [ m_placeCount ];
	
	for ( unsigned int i=0; i < m_placeCount; i++ )
	{
		m_placeName[i] = placeNames[i];
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given a place, return its name.
 * Reserve zero as invalid.
 */
const char *CNavMesh::PlaceToName( Place place ) const
{
	if (place >= 1 && place <= m_placeCount)
		return m_placeName[ (int)place - 1 ];

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a place name, return a place ID or zero if no place is defined
 * Reserve zero as invalid.
 */
Place CNavMesh::NameToPlace( const char *name ) const
{
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if (FStrEq( m_placeName[i], name ))
			return i+1;
	}

	return UNDEFINED_PLACE;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
 */
Place CNavMesh::PartialNameToPlace( const char *name ) const
{
	Place found = UNDEFINED_PLACE;
	bool isAmbiguous = false;
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if (!strnicmp( m_placeName[i], name, strlen( name ) ))
		{
			// check for exact match in case of subsets of other strings
			if (!stricmp( m_placeName[i], name ))
			{
				found = NameToPlace( m_placeName[i] );
				isAmbiguous = false;
				break;
			}

			if (found != UNDEFINED_PLACE)
			{
				isAmbiguous = true;
			}
			else
			{
				found = NameToPlace( m_placeName[i] );
			}
		}
	}

	if (isAmbiguous)
		return UNDEFINED_PLACE;

	return found;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a partial place name, fill in possible place names for ConCommand autocomplete
 */
int CNavMesh::PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int numMatches = 0;
	partial += Q_strlen( "nav_use_place " );
	int partialLength = Q_strlen( partial );

	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if ( !Q_strnicmp( m_placeName[i], partial, partialLength ) )
		{
			// Add the place name to the autocomplete array
			Q_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "nav_use_place %s", m_placeName[i] );

			// Make sure we don't try to return too many place names
			if ( numMatches == COMMAND_COMPLETION_MAXITEMS )
				return numMatches;
		}
	}

	return numMatches;
}


//--------------------------------------------------------------------------------------------------------------
typedef const char * SortStringType;
int StringSort (const SortStringType *s1, const SortStringType *s2)
{
	return strcmp( *s1, *s2 );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Output a list of names to the console
 */
void CNavMesh::PrintAllPlaces( void ) const
{
	if (m_placeCount == 0)
	{
		Msg( "There are no entries in the Place database.\n" );
		return;
	}

	unsigned int i;

	CUtlVector< SortStringType > placeNames;
	for ( i=0; i<m_placeCount; ++i )
	{
		placeNames.AddToTail( m_placeName[i] );
	}
	placeNames.Sort( StringSort );

	for( i=0; i<(unsigned int)placeNames.Count(); ++i )
	{
		if (NameToPlace( placeNames[i] ) == GetNavPlace())
			Msg( "--> %-26s", placeNames[i] );
		else
			Msg( "%-30s", placeNames[i] );

		if ((i+1) % 3 == 0)
			Msg( "\n" );
	}

	Msg( "\n" );
}

class CTraceFilterGroundEntities : public CTraceFilterWalkableEntities
{
	typedef CTraceFilterWalkableEntities BaseClass;

public:
	CTraceFilterGroundEntities( const IHandleEntity *passentity, int collisionGroup, unsigned int flags )
		: BaseClass( passentity, collisionGroup, flags )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( FClassnameIs( pEntity, "prop_door" ) ||
			 FClassnameIs( pEntity, "prop_door_rotating" ) ||
			 FClassnameIs( pEntity, "func_breakable" ) )
		{
			return false;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

bool CNavMesh::GetGroundHeight( const Vector &pos, float *height, Vector *normal ) const
{
	VPROF( "CNavMesh::GetGroundHeight" );

	const float flMaxOffset = 100.0f;

	CTraceFilterGroundEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );

	trace_t result;
	Vector to( pos.x, pos.y, pos.z - 10000.0f );
	Vector from( pos.x, pos.y, pos.z + HalfHumanHeight + 1e-3 );
	while( to.z - pos.z < flMaxOffset ) 
	{
		UTIL_TraceLine( from, to, MASK_NPCSOLID_BRUSHONLY, &filter, &result );
		if ( !result.startsolid && (( result.fraction == 1.0f ) || ( ( from.z - result.endpos.z ) >= HalfHumanHeight ) ) )
		{
			*height = result.endpos.z;
			if ( normal )
			{
				*normal = !result.plane.normal.IsZero() ? result.plane.normal : Vector( 0, 0, 1 );
			}
			return true;
		}	  
		to.z = ( result.startsolid ) ? from.z : result.endpos.z;
		from.z = to.z + HalfHumanHeight + 1e-3;
	}

	*height = 0.0f;
	if ( normal )
	{
		normal->Init( 0.0f, 0.0f, 1.0f );
	}
	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the "simple" ground height below this point in "height".
 * This function is much faster, but less tolerant. Make sure the give position is "well behaved".
 * Return false if position is invalid (outside of map, in a solid area, etc).
 */
bool CNavMesh::GetSimpleGroundHeight( const Vector &pos, float *height, Vector *normal ) const
{
	Vector to;
	to.x = pos.x;
	to.y = pos.y;
	to.z = pos.z - 9999.9f;

	trace_t result;

	UTIL_TraceLine( pos, to, MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &result );

	if (result.startsolid)
		return false;

	*height = result.endpos.z;

	if (normal)
		*normal = result.plane.normal;

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Show danger levels for debugging
 */
void CNavMesh::DrawDanger( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		Vector center = area->GetCenter();
		Vector top;
		center.z = area->GetZ( center );

		float danger = area->GetDanger( 0 );
		if (danger > 0.1f)
		{
			top.x = center.x;
			top.y = center.y;
			top.z = center.z + 10.0f * danger;
			DrawLine( center, top, 3, 255, 0, 0 );
		}

		danger = area->GetDanger( 1 );
		if (danger > 0.1f)
		{
			top.x = center.x;
			top.y = center.y;
			top.z = center.z + 10.0f * danger;
			DrawLine( center, top, 3, 0, 0, 255 );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Show current player counts for debugging.
 * NOTE: Assumes two teams.
 */
void CNavMesh::DrawPlayerCounts( void ) const
{
	CFmtStr msg;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if (area->GetPlayerCount() > 0)
		{
			NDebugOverlay::Text( area->GetCenter(), msg.sprintf( "%d (%d/%d)", area->GetPlayerCount(), area->GetPlayerCount(1), area->GetPlayerCount(2) ), false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot avoidance areas from func_nav_avoid entities
 */
void CNavMesh::DrawFuncNavAvoid( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasFuncNavAvoid() )
		{
			area->DrawFilled( 255, 0, 0, 255 );
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot preference areas from func_nav_prefer entities
 */
void CNavMesh::DrawFuncNavPrefer( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasFuncNavPrefer() )
		{
			area->DrawFilled( 0, 255, 0, 255 );
		}
	}
}


#ifdef NEXT_BOT
//--------------------------------------------------------------------------------------------------------------
/**
 * Draw bot preference areas from func_nav_prerequisite entities
 */
void CNavMesh::DrawFuncNavPrerequisite( void ) const
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		if ( area->HasPrerequisite() )
		{
			area->DrawFilled( 0, 0, 255, 255 );
		}
	}
}
#endif


//--------------------------------------------------------------------------------------------------------------
/**
 * Increase the danger of nav areas containing and near the given position
 */
void CNavMesh::IncreaseDangerNearby( int teamID, float amount, CNavArea *startArea, const Vector &pos, float maxRadius, float dangerLimit )
{
	if (startArea == NULL)
		return;

	CNavArea::MakeNewMarker();
	CNavArea::ClearSearchLists();

	startArea->AddToOpenList();
	startArea->SetTotalCost( 0.0f );
	startArea->Mark();

	float finalDanger = amount;

	if ( dangerLimit > 0.0f && startArea->GetDanger( teamID ) + finalDanger > dangerLimit )
	{
		// clamp danger to given limit
		finalDanger = dangerLimit - startArea->GetDanger( teamID );
	}

	startArea->IncreaseDanger( teamID, finalDanger );

	
	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CNavArea *area = CNavArea::PopOpenList();
		
		// explore adjacent areas
		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );

				if (!adjArea->IsMarked())
				{
					// compute distance from danger source
					float cost = (adjArea->GetCenter() - pos).Length();
					if (cost <= maxRadius)
					{
						adjArea->AddToOpenList();
						adjArea->SetTotalCost( cost );
						adjArea->Mark();

						finalDanger = amount * cost/maxRadius;

						if ( dangerLimit > 0.0f && adjArea->GetDanger( teamID ) + finalDanger > dangerLimit )
						{
							// clamp danger to given limit
							finalDanger = dangerLimit - adjArea->GetDanger( teamID );
						}

						adjArea->IncreaseDanger( teamID, finalDanger );
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavRemoveJumpAreas( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRemoveJumpAreas();
}
static ConCommand nav_remove_jump_areas( "nav_remove_jump_areas", CommandNavRemoveJumpAreas, "Removes legacy jump areas, replacing them with connections.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDelete( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || !nav_edit.GetBool() )
		return;

	TheNavMesh->CommandNavDelete();
}
static ConCommand nav_delete( "nav_delete", CommandNavDelete, "Deletes the currently highlighted Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-------------------------------------------------------------------------------------------------------------- 
void CommandNavDeleteMarked( void ) 
{ 
	if ( !UTIL_IsCommandIssuedByServerAdmin() || !nav_edit.GetBool() ) 
		return; 

	TheNavMesh->CommandNavDeleteMarked(); 
} 
static ConCommand nav_delete_marked( "nav_delete_marked", CommandNavDeleteMarked, "Deletes the currently marked Area (if any).", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_flood_select, "Selects the current Area and all Areas connected to it, recursively. To clear a selection, use this command again.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavFloodSelect( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleSelectedSet();
}
static ConCommand nav_toggle_selected_set( "nav_toggle_selected_set", CommandNavToggleSelectedSet, "Toggles all areas into/out of the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStoreSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavStoreSelectedSet();
}
static ConCommand nav_store_selected_set( "nav_store_selected_set", CommandNavStoreSelectedSet, "Stores the current selected set for later retrieval.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRecallSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRecallSelectedSet();
}
static ConCommand nav_recall_selected_set( "nav_recall_selected_set", CommandNavRecallSelectedSet, "Re-selects the stored selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAddToSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavAddToSelectedSet();
}
static ConCommand nav_add_to_selected_set( "nav_add_to_selected_set", CommandNavAddToSelectedSet, "Add current area to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_add_to_selected_set_by_id, "Add specified area id to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavAddToSelectedSetByID( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavRemoveFromSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRemoveFromSelectedSet();
}
static ConCommand nav_remove_from_selected_set( "nav_remove_from_selected_set", CommandNavRemoveFromSelectedSet, "Remove current area from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleInSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleInSelectedSet();
}
static ConCommand nav_toggle_in_selected_set( "nav_toggle_in_selected_set", CommandNavToggleInSelectedSet, "Remove current area from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavClearSelectedSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavClearSelectedSet();
}
static ConCommand nav_clear_selected_set( "nav_clear_selected_set", CommandNavClearSelectedSet, "Clear the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//----------------------------------------------------------------------------------
CON_COMMAND_F( nav_dump_selected_set_positions, "Write the (x,y,z) coordinates of the centers of all selected nav areas to a file.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	const NavAreaVector &selectedSet = TheNavMesh->GetSelectedSet();

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	for( int i=0; i<selectedSet.Count(); ++i )
	{
		const Vector &center = selectedSet[i]->GetCenter();

		fileBuffer.Printf( "%f %f %f\n", center.x, center.y, center.z );
	}

	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf( filename, sizeof( filename ), "%s\\maps\\%s_xyz.txt", gamePath, STRING( gpGlobals->mapname ) );

	if ( !filesystem->WriteFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
	}
	else
	{
		DevMsg( "Write %d nav area center positions to '%s'.\n", selectedSet.Count(), filename );
	}
};


//----------------------------------------------------------------------------------
CON_COMMAND_F( nav_show_dumped_positions, "Show the (x,y,z) coordinate positions of the given dump file.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf( filename, sizeof( filename ), "%s\\maps\\%s_xyz.txt", gamePath, STRING( gpGlobals->mapname ) );

	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to read %s\n", filename );
	}
	else
	{
		while( true )
		{
			Vector center;
			if ( fileBuffer.Scanf( "%f %f %f", &center.x, &center.y, &center.z ) <= 0 )
			{
				break;
			}

			NDebugOverlay::Cross3D( center, 5.0f, 255, 255, 0, true, 99999.9f );
		}
	}
};


//----------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_larger_than, "Select nav areas where both dimensions are larger than the given size.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( args.ArgC() > 1 )
	{
		float minSize = atof( args[1] );

		int selectedCount = 0;

		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CNavArea *area = TheNavAreas[i];

			if ( area->GetSizeX() > minSize && area->GetSizeY() > minSize )
			{
				TheNavMesh->AddToSelectedSet( area );
				++selectedCount;
			}
		}

		DevMsg( "Selected %d areas with dimensions larger than %3.2f units.\n", selectedCount, minSize );
	}
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginSelecting();
}
static ConCommand nav_begin_selecting( "nav_begin_selecting", CommandNavBeginSelecting, "Start continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndSelecting();
}
static ConCommand nav_end_selecting( "nav_end_selecting", CommandNavEndSelecting, "Stop continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDragSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDragSelecting();
}
static ConCommand nav_begin_drag_selecting( "nav_begin_drag_selecting", CommandNavBeginDragSelecting, "Start dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDragSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDragSelecting();
}
static ConCommand nav_end_drag_selecting( "nav_end_drag_selecting", CommandNavEndDragSelecting, "Stop dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDragDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDragDeselecting();
}
static ConCommand nav_begin_drag_deselecting( "nav_begin_drag_deselecting", CommandNavBeginDragDeselecting, "Start dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDragDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDragDeselecting();
}
static ConCommand nav_end_drag_deselecting( "nav_end_drag_deselecting", CommandNavEndDragDeselecting, "Stop dragging a selection area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRaiseDragVolumeMax( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRaiseDragVolumeMax();
}
static ConCommand nav_raise_drag_volume_max( "nav_raise_drag_volume_max", CommandNavRaiseDragVolumeMax, "Raise the top of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLowerDragVolumeMax( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLowerDragVolumeMax();
}
static ConCommand nav_lower_drag_volume_max( "nav_lower_drag_volume_max", CommandNavLowerDragVolumeMax, "Lower the top of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRaiseDragVolumeMin( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavRaiseDragVolumeMin();
}
static ConCommand nav_raise_drag_volume_min( "nav_raise_drag_volume_min", CommandNavRaiseDragVolumeMin, "Raise the bottom of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLowerDragVolumeMin( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLowerDragVolumeMin();
}
static ConCommand nav_lower_drag_volume_min( "nav_lower_drag_volume_min", CommandNavLowerDragVolumeMin, "Lower the bottom of the drag select volume.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleSelecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleSelecting();
}
static ConCommand nav_toggle_selecting( "nav_toggle_selecting", CommandNavToggleSelecting, "Start or stop continuously adding to the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginDeselecting();
}
static ConCommand nav_begin_deselecting( "nav_begin_deselecting", CommandNavBeginDeselecting, "Start continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndDeselecting();
}
static ConCommand nav_end_deselecting( "nav_end_deselecting", CommandNavEndDeselecting, "Stop continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavToggleDeselecting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleDeselecting();
}
static ConCommand nav_toggle_deselecting( "nav_toggle_deselecting", CommandNavToggleDeselecting, "Start or stop continuously removing from the selected set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_half_space, "Selects any areas that intersect the given half-space.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectHalfSpace( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginShiftXY( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginShiftXY();
}
static ConCommand nav_begin_shift_xy( "nav_begin_shift_xy", CommandNavBeginShiftXY, "Begin shifting the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndShiftXY( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndShiftXY();
}
static ConCommand nav_end_shift_xy( "nav_end_shift_xy", CommandNavEndShiftXY, "Finish shifting the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSelectInvalidAreas( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectInvalidAreas();
}
static ConCommand nav_select_invalid_areas( "nav_select_invalid_areas", CommandNavSelectInvalidAreas, "Adds all invalid areas to the Selected Set.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_blocked_areas, "Adds all blocked areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectBlockedAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_obstructed_areas, "Adds all obstructed areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectObstructedAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_damaging_areas, "Adds all damaging areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectDamagingAreas();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_stairs, "Adds all stairway areas to the selected set", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectStairs();
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_select_orphans, "Adds all orphan areas to the selected set (highlight a valid area first).", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSelectOrphans();
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavSplit( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSplit();
}
static ConCommand nav_split( "nav_split", CommandNavSplit, "To split an Area into two, align the split line using your cursor and invoke the split command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMakeSniperSpots( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMakeSniperSpots();
}
static ConCommand nav_make_sniper_spots( "nav_make_sniper_spots", CommandNavMakeSniperSpots, "Chops the marked area into disconnected sub-areas suitable for sniper spots.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMerge( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMerge();
}
static ConCommand nav_merge( "nav_merge", CommandNavMerge, "To merge two Areas into one, mark the first Area, highlight the second by pointing your cursor at it, and invoke the merge command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMark( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMark( args );
}
static ConCommand nav_mark( "nav_mark", CommandNavMark, "Marks the Area or Ladder under the cursor for manipulation by subsequent editing commands.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavUnmark( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavUnmark();
}
static ConCommand nav_unmark( "nav_unmark", CommandNavUnmark, "Clears the marked Area or Ladder.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavBeginArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBeginArea();
}
static ConCommand nav_begin_area( "nav_begin_area", CommandNavBeginArea, "Defines a corner of a new Area or Ladder. To complete the Area or Ladder, drag the opposite corner to the desired location and issue a 'nav_end_area' command.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavEndArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavEndArea();
}
static ConCommand nav_end_area( "nav_end_area", CommandNavEndArea, "Defines the second corner of a new Area or Ladder and creates it.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavConnect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavConnect();
}
static ConCommand nav_connect( "nav_connect", CommandNavConnect, "To connect two Areas, mark the first Area, highlight the second Area, then invoke the connect command. Note that this creates a ONE-WAY connection from the first to the second Area. To make a two-way connection, also connect the second area to the first.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDisconnect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavDisconnect();
}
static ConCommand nav_disconnect( "nav_disconnect", CommandNavDisconnect, "To disconnect two Areas, mark an Area, highlight a second Area, then invoke the disconnect command. This will remove all connections between the two Areas.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDisconnectOutgoingOneWays( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavDisconnectOutgoingOneWays();
}
static ConCommand nav_disconnect_outgoing_oneways( "nav_disconnect_outgoing_oneways", CommandNavDisconnectOutgoingOneWays, "For each area in the selected set, disconnect all outgoing one-way connections.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSplice( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSplice();
}
static ConCommand nav_splice( "nav_splice", CommandNavSplice, "To splice, mark an area, highlight a second area, then invoke the splice command to create a new, connected area between them.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCrouch( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_CROUCH );
}
static ConCommand nav_crouch( "nav_crouch", CommandNavCrouch, "Toggles the 'must crouch in this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPrecise( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_PRECISE );
}
static ConCommand nav_precise( "nav_precise", CommandNavPrecise, "Toggles the 'dont avoid obstacles' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavJump( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_JUMP );
}
static ConCommand nav_jump( "nav_jump", CommandNavJump, "Toggles the 'traverse this area by jumping' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavNoJump( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_NO_JUMP );
}
static ConCommand nav_no_jump( "nav_no_jump", CommandNavNoJump, "Toggles the 'dont jump in this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStop( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_STOP );
}
static ConCommand nav_stop( "nav_stop", CommandNavStop, "Toggles the 'must stop when entering this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavWalk( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_WALK );
}
static ConCommand nav_walk( "nav_walk", CommandNavWalk, "Toggles the 'traverse this area by walking' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavRun( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_RUN );
}
static ConCommand nav_run( "nav_run", CommandNavRun, "Toggles the 'traverse this area by running' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAvoid( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_AVOID );
}
static ConCommand nav_avoid( "nav_avoid", CommandNavAvoid, "Toggles the 'avoid this area when possible' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTransient( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_TRANSIENT );
}
static ConCommand nav_transient( "nav_transient", CommandNavTransient, "Toggles the 'area is transient and may become blocked' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavDontHide( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_DONT_HIDE );
}
static ConCommand nav_dont_hide( "nav_dont_hide", CommandNavDontHide, "Toggles the 'area is not suitable for hiding spots' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStand( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_STAND );
}
static ConCommand nav_stand( "nav_stand", CommandNavStand, "Toggles the 'stand while hiding' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavNoHostages( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavToggleAttribute( NAV_MESH_NO_HOSTAGES );
}
static ConCommand nav_no_hostages( "nav_no_hostages", CommandNavNoHostages, "Toggles the 'hostages cannot use this area' flag used by the AI system.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavStrip( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->StripNavigationAreas();
}
static ConCommand nav_strip( "nav_strip", CommandNavStrip, "Strips all Hiding Spots, Approach Points, and Encounter Spots from the current Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSave( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (TheNavMesh->Save())
	{
		Msg( "Navigation map '%s' saved.\n", TheNavMesh->GetFilename() );
	}
	else
	{
		const char *filename = TheNavMesh->GetFilename();
		Msg( "ERROR: Cannot save navigation map '%s'.\n", (filename) ? filename : "(null)" );
	}
}
static ConCommand nav_save( "nav_save", CommandNavSave, "Saves the current Navigation Mesh to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLoad( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (TheNavMesh->Load() != NAV_OK)
	{
		Msg( "ERROR: Navigation Mesh load failed.\n" );
	}
}
static ConCommand nav_load( "nav_load", CommandNavLoad, "Loads the Navigation Mesh for the current map.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
static int PlaceNameAutocompleteCallback( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	return TheNavMesh->PlaceNameAutocomplete( partial, commands );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavUsePlace( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (args.ArgC() == 1)
	{
		// no arguments = list all available places
		TheNavMesh->PrintAllPlaces();
	}
	else
	{
		// single argument = set current place
		Place place = TheNavMesh->PartialNameToPlace( args[ 1 ] );

		if (place == UNDEFINED_PLACE)
		{
			Msg( "Ambiguous\n" );
		}
		else
		{
			Msg( "Current place set to '%s'\n", TheNavMesh->PlaceToName( place ) );
			TheNavMesh->SetNavPlace( place );
		}
	}
}
static ConCommand nav_use_place( "nav_use_place", CommandNavUsePlace, "If used without arguments, all available Places will be listed. If a Place argument is given, the current Place is set.", FCVAR_GAMEDLL | FCVAR_CHEAT, PlaceNameAutocompleteCallback );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceReplace( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if (args.ArgC() != 3)
	{
		// no arguments
		Msg( "Usage: nav_place_replace <OldPlace> <NewPlace>\n" );
	}
	else
	{
		// two arguments - replace the first place with the second
		Place oldPlace = TheNavMesh->PartialNameToPlace( args[ 1 ] );
		Place newPlace = TheNavMesh->PartialNameToPlace( args[ 2 ] );

		if ( oldPlace == UNDEFINED_PLACE || newPlace == UNDEFINED_PLACE )
		{
			Msg( "Ambiguous\n" );
		}
		else
		{
			FOR_EACH_VEC( TheNavAreas, it )
			{
				CNavArea *area = TheNavAreas[it];
				if ( area->GetPlace() == oldPlace )
				{
					area->SetPlace( newPlace );
				}
			}
		}
	}
}
static ConCommand nav_place_replace( "nav_place_replace", CommandNavPlaceReplace, "Replaces all instances of the first place with the second place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceList( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CUtlVector< Place > placeDirectory;

	FOR_EACH_VEC( TheNavAreas, nit )
	{
		CNavArea *area = TheNavAreas[ nit ];

		Place place = area->GetPlace();

		if ( place )
		{
			if ( !placeDirectory.HasElement( place ) )
			{
				placeDirectory.AddToTail( place );
			}
		}
	}

	Msg( "Map uses %d place names:\n", placeDirectory.Count() );
	for ( int i=0; i<placeDirectory.Count(); ++i )
	{
		Msg( "    %s\n", TheNavMesh->PlaceToName( placeDirectory[i] ) );
	}
}
static ConCommand nav_place_list( "nav_place_list", CommandNavPlaceList, "Lists all place names used in the map.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTogglePlaceMode( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavTogglePlaceMode();
}
static ConCommand nav_toggle_place_mode( "nav_toggle_place_mode", CommandNavTogglePlaceMode, "Toggle the editor into and out of Place mode. Place mode allows labelling of Area with Place names.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavSetPlaceMode( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	bool on = true;
	if ( args.ArgC() == 2 )
	{
		on = (atoi( args[ 1 ] ) != 0);
	}

	if ( on != TheNavMesh->IsEditMode( CNavMesh::PLACE_PAINTING ) )
	{
		TheNavMesh->CommandNavTogglePlaceMode();
	}
}
static ConCommand nav_set_place_mode( "nav_set_place_mode", CommandNavSetPlaceMode, "Sets the editor into or out of Place mode. Place mode allows labelling of Area with Place names.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceFloodFill( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlaceFloodFill();
}
static ConCommand nav_place_floodfill( "nav_place_floodfill", CommandNavPlaceFloodFill, "Sets the Place of the Area under the cursor to the curent Place, and 'flood-fills' the Place to all adjacent Areas. Flood-filling stops when it hits an Area with the same Place, or a different Place than that of the initial Area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlaceSet( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlaceSet();
}
static ConCommand nav_place_set( "nav_place_set", CommandNavPlaceSet, "Sets the Place of all selected areas to the current Place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavPlacePick( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPlacePick();
}
static ConCommand nav_place_pick( "nav_place_pick", CommandNavPlacePick, "Sets the current Place to the Place of the Area under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavTogglePlacePainting( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavTogglePlacePainting();
}
static ConCommand nav_toggle_place_painting( "nav_toggle_place_painting", CommandNavTogglePlacePainting, "Toggles Place Painting mode. When Place Painting, pointing at an Area will 'paint' it with the current Place.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMarkUnnamed( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMarkUnnamed();
}
static ConCommand nav_mark_unnamed( "nav_mark_unnamed", CommandNavMarkUnnamed, "Mark an Area with no Place name. Useful for finding stray areas missed when Place Painting.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCornerSelect( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerSelect();
}
static ConCommand nav_corner_select( "nav_corner_select", CommandNavCornerSelect, "Select a corner of the currently marked Area. Use multiple times to access all four corners.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_corner_raise, "Raise the selected corner of the currently marked Area.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerRaise( args );
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_corner_lower, "Lower the selected corner of the currently marked Area.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerLower( args );
}


//--------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_corner_place_on_ground, "Places the selected corner of the currently marked Area on the ground.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavCornerPlaceOnGround( args );
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavWarpToMark( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavWarpToMark();
}
static ConCommand nav_warp_to_mark( "nav_warp_to_mark", CommandNavWarpToMark, "Warps the player to the marked area.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavLadderFlip( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavLadderFlip();
}
static ConCommand nav_ladder_flip( "nav_ladder_flip", CommandNavLadderFlip, "Flips the selected ladder's direction.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavGenerate( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->BeginGeneration();
}
static ConCommand nav_generate( "nav_generate", CommandNavGenerate, "Generate a Navigation Mesh for the current map and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavGenerateIncremental( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->BeginGeneration( INCREMENTAL_GENERATION );
}
static ConCommand nav_generate_incremental( "nav_generate_incremental", CommandNavGenerateIncremental, "Generate a Navigation Mesh for the current map and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAnalyze( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( nav_edit.GetBool() )
	{
		TheNavMesh->BeginAnalysis();
	}
}
static ConCommand nav_analyze( "nav_analyze", CommandNavAnalyze, "Re-analyze the current Navigation Mesh and save it to disk.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavAnalyzeScripted( const CCommand &args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	const char *pszCmd = NULL;
	int count = args.ArgC();
	if ( count > 0 )
	{
		pszCmd = args[1];
	}

	bool bForceAnalyze = pszCmd && !Q_stricmp( pszCmd, "force" );

	if ( TheNavMesh->IsAnalyzed() && !bForceAnalyze )
	{
		engine->ServerCommand( "quit\n" );
		return;
	}

	if ( nav_edit.GetBool() )
	{
		TheNavMesh->BeginAnalysis( true );
	}
}
static ConCommand nav_analyze_scripted( "nav_analyze_scripted", CommandNavAnalyzeScripted, "commandline hook to run a nav_analyze and then quit.", FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_HIDDEN );


//--------------------------------------------------------------------------------------------------------------
void CommandNavMarkWalkable( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMarkWalkable();
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMarkWalkable( void )
{
	Vector pos;

	if (nav_edit.GetBool())
	{
		// we are in edit mode, use the edit cursor's location
		pos = GetEditCursorPosition();
	}
	else
	{
		// we are not in edit mode, use the position of the local player
		CBasePlayer *player = UTIL_GetListenServerHost();

		if (player == NULL)
		{
			Msg( "ERROR: No local player!\n" );
			return;
		}

		pos = player->GetAbsOrigin();
	}

	// snap position to the sampling grid
	pos.x = SnapToGrid( pos.x, true );
	pos.y = SnapToGrid( pos.y, true );

	Vector normal;
	if ( !FindGroundForNode( &pos, &normal ) )
	{
		Msg( "ERROR: Invalid ground position.\n" );
		return;
	}

	AddWalkableSeed( pos, normal );

	Msg( "Walkable position marked.\n" );
}
static ConCommand nav_mark_walkable( "nav_mark_walkable", CommandNavMarkWalkable, "Mark the current location as a walkable position. These positions are used as seed locations when sampling the map to generate a Navigation Mesh.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavClearWalkableMarks( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->ClearWalkableSeeds();
}
static ConCommand nav_clear_walkable_marks( "nav_clear_walkable_marks", CommandNavClearWalkableMarks, "Erase any previously placed walkable positions.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavCompressID( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CNavArea::CompressIDs();
	CNavLadder::CompressIDs();
}
static ConCommand nav_compress_id( "nav_compress_id", CommandNavCompressID, "Re-orders area and ladder ID's so they are continuous.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
#ifdef TERROR
void CommandNavShowLadderBounds( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CFuncSimpleLadder *ladder = NULL;
	while( (ladder = dynamic_cast< CFuncSimpleLadder * >(gEntList.FindEntityByClassname( ladder, "func_simpleladder" ))) != NULL )
	{
		Vector mins, maxs;
		ladder->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
		ladder->m_debugOverlays |= OVERLAY_TEXT_BIT | OVERLAY_ABSBOX_BIT;
		NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 0, 600 );
	}
}
static ConCommand nav_show_ladder_bounds( "nav_show_ladder_bounds", CommandNavShowLadderBounds, "Draws the bounding boxes of all func_ladders in the map.", FCVAR_GAMEDLL | FCVAR_CHEAT );
#endif

//--------------------------------------------------------------------------------------------------------------
void CommandNavBuildLadder( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavBuildLadder();
}
static ConCommand nav_build_ladder( "nav_build_ladder", CommandNavBuildLadder, "Attempts to build a nav ladder on the climbable surface under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
void NavEditClearAllAttributes( void )
{
	NavAttributeClearer clear( (NavAttributeType)0xFFFF );
	TheNavMesh->ForAllSelectedAreas( clear );
	TheNavMesh->ClearSelectedSet();
}
static ConCommand ClearAllNavAttributes( "wipe_nav_attributes", NavEditClearAllAttributes, "Clear all nav attributes of selected area.", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
bool NavAttributeToggler::operator() ( CNavArea *area )
{
	// only toggle if dealing with a single selected area
	if ( TheNavMesh->IsSelectedSetEmpty() && (area->GetAttributes() & m_attribute) != 0 )
	{
		area->SetAttributes( area->GetAttributes() & (~m_attribute) );
	}
	else
	{
		area->SetAttributes( area->GetAttributes() | m_attribute );
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------
NavAttributeLookup TheNavAttributeTable[] =
{
	{ "CROUCH", NAV_MESH_CROUCH },
	{ "JUMP", NAV_MESH_JUMP },
	{ "PRECISE", NAV_MESH_PRECISE },
	{ "NO_JUMP", NAV_MESH_NO_JUMP },
	{ "STOP", NAV_MESH_STOP },
	{ "RUN", NAV_MESH_RUN },
	{ "WALK", NAV_MESH_WALK },
	{ "AVOID", NAV_MESH_AVOID },
	{ "TRANSIENT", NAV_MESH_TRANSIENT },
	{ "DONT_HIDE", NAV_MESH_DONT_HIDE },
	{ "STAND", NAV_MESH_STAND },
	{ "NO_HOSTAGES", NAV_MESH_NO_HOSTAGES },
	{ "STAIRS", NAV_MESH_STAIRS },
	{ "NO_MERGE", NAV_MESH_NO_MERGE },
	{ "OBSTACLE_TOP", NAV_MESH_OBSTACLE_TOP },
	{ "CLIFF", NAV_MESH_CLIFF },
#ifdef TERROR
	{ "PLAYERCLIP", (NavAttributeType)CNavArea::NAV_PLAYERCLIP },
	{ "BREAKABLEWALL", (NavAttributeType)CNavArea::NAV_BREAKABLEWALL },
#endif
	{ NULL, NAV_MESH_INVALID }
};


/**
 * Can be used with any command that takes an attribute as its 2nd argument
 */
static int NavAttributeAutocomplete( const char *input, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	if ( Q_strlen( input ) >= COMMAND_COMPLETION_ITEM_LENGTH )
	{
		return 0;
	}
	
	char command[ COMMAND_COMPLETION_ITEM_LENGTH+1 ];
	Q_strncpy( command, input, sizeof( command ) );
	
	// skip to start of argument
	char *partialArg = Q_strrchr( command, ' ' );
	if ( partialArg == NULL )
	{
		return 0;
	}
	
	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;
	
	int partialArgLength = Q_strlen( partialArg );

	int count = 0;
	for( unsigned int i=0; TheNavAttributeTable[i].name && count < COMMAND_COMPLETION_MAXITEMS; ++i )
	{
		if ( !Q_strnicmp( TheNavAttributeTable[i].name, partialArg, partialArgLength ) )
		{
			// Add to the autocomplete array
			Q_snprintf( commands[ count++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", command, TheNavAttributeTable[i].name );
		}
	}

	return count;
}


//--------------------------------------------------------------------------------------------------------
NavAttributeType NameToNavAttribute( const char *name )
{
	for( unsigned int i=0; TheNavAttributeTable[i].name; ++i )
	{
		if ( !Q_stricmp( TheNavAttributeTable[i].name, name ) )
		{
			return TheNavAttributeTable[i].attribute;
		}
	}
	
	return (NavAttributeType)0;
}


//--------------------------------------------------------------------------------------------------------
void NavEditClearAttribute( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: %s <attribute>\n", args[0] );
		return;
	}
	
	NavAttributeType attribute = NameToNavAttribute( args[1] );

	if ( attribute != 0 )
	{
		NavAttributeClearer clear( attribute );
		TheNavMesh->ForAllSelectedAreas( clear );
		TheNavMesh->ClearSelectedSet();
		return;
	}
	
	Msg( "Unknown attribute '%s'", args[1] );		
}
static ConCommand NavClearAttribute( "nav_clear_attribute", NavEditClearAttribute, "Remove given nav attribute from all areas in the selected set.", FCVAR_CHEAT, NavAttributeAutocomplete );


//--------------------------------------------------------------------------------------------------------
void NavEditMarkAttribute( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: %s <attribute>\n", args[0] );
		return;
	}

	NavAttributeType attribute = NameToNavAttribute( args[1] );

	if ( attribute != 0 )
	{
		NavAttributeSetter setter( attribute );	
		TheNavMesh->ForAllSelectedAreas( setter );
		TheNavMesh->ClearSelectedSet();
		return;
	}

	Msg( "Unknown attribute '%s'", args[1] );		
}
static ConCommand NavMarkAttribute( "nav_mark_attribute", NavEditMarkAttribute, "Set nav attribute for all areas in the selected set.", FCVAR_CHEAT, NavAttributeAutocomplete );


/* IN PROGRESS:
//--------------------------------------------------------------------------------------------------------------
void CommandNavPickArea( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavPickArea();
}
static ConCommand nav_pick_area( "nav_pick_area", CommandNavPickArea, "Marks an area (and corner) based on the surface under the cursor.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeHorizontal( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeHorizontal();
}
static ConCommand nav_resize_horizontal( "nav_resize_horizontal", CommandNavResizeHorizontal, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeVertical( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeVertical();
}
static ConCommand nav_resize_vertical( "nav_resize_vertical", CommandNavResizeVertical, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
void CommandNavResizeEnd( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavResizeEnd();
}
static ConCommand nav_resize_end( "nav_resize_end", CommandNavResizeEnd, "TODO", FCVAR_GAMEDLL | FCVAR_CHEAT );
*/


//--------------------------------------------------------------------------------------------------------------
/**
 * Destroy ladder representations
 */
void CNavMesh::DestroyLadders( void )
{
	for ( int i=0; i<m_ladders.Count(); ++i )
	{
		OnEditDestroyNotify( m_ladders[i] );
		delete m_ladders[i];
	}

	m_ladders.RemoveAll();

	m_markedLadder = NULL;
	m_selectedLadder = NULL;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Strip the "analyzed" data out of all navigation areas
 */
void CNavMesh::StripNavigationAreas( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->Strip();
	}

	m_isAnalyzed = false;
}

//--------------------------------------------------------------------------------------------------------------

HidingSpotVector TheHidingSpots;
unsigned int HidingSpot::m_nextID = 1;
unsigned int HidingSpot::m_masterMarker = 0;


//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot factory
 */
HidingSpot *CNavMesh::CreateHidingSpot( void ) const
{
	return new HidingSpot;
}


//--------------------------------------------------------------------------------------------------------------
void CNavMesh::DestroyHidingSpots( void )
{
	// remove all hiding spot references from the nav areas
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];

		area->m_hidingSpots.RemoveAll();
	}

	HidingSpot::m_nextID = 0;

	// free all the HidingSpots
	FOR_EACH_VEC( TheHidingSpots, hit )
	{
		delete TheHidingSpots[ hit ];
	}

	TheHidingSpots.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
/**
 * Construct a Hiding Spot.  Assign a unique ID which may be overwritten if loaded.
 */
HidingSpot::HidingSpot( void )
{
	m_pos = Vector( 0, 0, 0 );
	m_id = m_nextID++;
	m_flags = 0;
	m_area = NULL;

	TheHidingSpots.AddToTail( this );
}


//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	fileBuffer.PutUnsignedInt( m_id );
	fileBuffer.PutFloat( m_pos.x );
	fileBuffer.PutFloat( m_pos.y );
	fileBuffer.PutFloat( m_pos.z );
	fileBuffer.PutUnsignedChar( m_flags );
}


//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Load( CUtlBuffer &fileBuffer, unsigned int version )
{
	m_id = fileBuffer.GetUnsignedInt();
	m_pos.x = fileBuffer.GetFloat();
	m_pos.y = fileBuffer.GetFloat();
	m_pos.z = fileBuffer.GetFloat();
	m_flags = fileBuffer.GetUnsignedChar();

	// update next ID to avoid ID collisions by later spots
	if (m_id >= m_nextID)
		m_nextID = m_id+1;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot post-load processing
 */
NavErrorType HidingSpot::PostLoad( void )
{
	// set our area
	m_area = TheNavMesh->GetNavArea( m_pos + Vector( 0, 0, HalfHumanHeight ) );

	if ( !m_area )
	{
		DevWarning( "A Hiding Spot is off of the Nav Mesh at setpos %.0f %.0f %.0f\n", m_pos.x, m_pos.y, m_pos.z );
	}

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a HidingSpot ID, return the associated HidingSpot
 */
HidingSpot *GetHidingSpotByID( unsigned int id )
{
	FOR_EACH_VEC( TheHidingSpots, it )
	{
		HidingSpot *spot = TheHidingSpots[ it ];

		if (spot->GetID() == id)
			return spot;
	}	

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes blocked
void CNavMesh::OnAreaBlocked( CNavArea *area )
{
	if ( !m_blockedAreas.HasElement( area ) )
	{
		m_blockedAreas.AddToTail( area );
	}
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes un-blocked
void CNavMesh::OnAreaUnblocked( CNavArea *area )
{
	m_blockedAreas.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UpdateBlockedAreas( void )
{
	VPROF( "CNavMesh::UpdateBlockedAreas" );
	for ( int i=0; i<m_blockedAreas.Count(); ++i )
	{
		CNavArea *area = m_blockedAreas[i];
		area->UpdateBlocked();
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::RegisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction )
{
	m_avoidanceObstacles.FindAndFastRemove( obstruction );
	m_avoidanceObstacles.AddToTail( obstruction );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UnregisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction )
{
	m_avoidanceObstacles.FindAndFastRemove( obstruction );
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes blocked
void CNavMesh::OnAvoidanceObstacleEnteredArea( CNavArea *area )
{
	if ( !m_avoidanceObstacleAreas.HasElement( area ) )
	{
		m_avoidanceObstacleAreas.AddToTail( area );
	}
}


//--------------------------------------------------------------------------------------------------------
// invoked when the area becomes un-blocked
void CNavMesh::OnAvoidanceObstacleLeftArea( CNavArea *area )
{
	m_avoidanceObstacleAreas.FindAndRemove( area );
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::UpdateAvoidanceObstacleAreas( void )
{
	VPROF( "CNavMesh::UpdateAvoidanceObstacleAreas" );
	for ( int i=0; i<m_avoidanceObstacleAreas.Count(); ++i )
	{
		CNavArea *area = m_avoidanceObstacleAreas[i];
		area->UpdateAvoidanceObstacles();
	}
}



extern CUtlHash< NavVisPair_t, CVisPairHashFuncs, CVisPairHashFuncs > *g_pNavVisPairHash;

//--------------------------------------------------------------------------------------------------------
void CNavMesh::BeginVisibilityComputations( void )
{
	if ( !g_pNavVisPairHash )
	{
		g_pNavVisPairHash = new CUtlHash< NavVisPair_t, CVisPairHashFuncs, CVisPairHashFuncs >( 16*1024 );
	}
	else
	{
		g_pNavVisPairHash->RemoveAll();
	}

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = TheNavAreas[ it ];
		area->ResetPotentiallyVisibleAreas();
	}
}


//--------------------------------------------------------------------------------------------------------
/**
 * Invoked when custom analysis step is complete
 */
void CNavMesh::EndVisibilityComputations( void )
{
	g_pNavVisPairHash->RemoveAll();

	int avgVisLength = 0;
	int maxVisLength = 0;
	int minVisLength = 999999999;

	// Optimize visibility storage of nav mesh by doing a kind of run-length encoding.
	// Pick an "anchor" area and compare adjacent areas visibility lists to it. If the delta is
	// small, point back to the anchor and just store the delta.
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CNavArea *area = (CNavArea *)TheNavAreas[ it ];

		int visLength = area->m_potentiallyVisibleAreas.Count();
		avgVisLength += visLength;
		if ( visLength < minVisLength )
		{
			minVisLength = visLength;
		}
		if ( visLength > maxVisLength )
		{
			maxVisLength = visLength;
		}

		if ( area->m_isInheritedFrom )
		{
			// another area is inheriting from our vis data, we can't inherit
			continue;
		}

		// find adjacent area with the smallest change from our visibility list
		CNavArea::CAreaBindInfoArray bestDelta;
		CNavArea *anchor = NULL;

		for( int dir = NORTH; dir < NUM_DIRECTIONS; ++dir )
		{
			int count = area->GetAdjacentCount( (NavDirType)dir );
			for( int i=0; i<count; ++i )
			{
				CNavArea *adjArea = (CNavArea *)area->GetAdjacentArea( (NavDirType)dir, i );

				// do not inherit from an area that is inheriting - use its ultimate source
				if ( adjArea->m_inheritVisibilityFrom.area != NULL )
				{
					adjArea = adjArea->m_inheritVisibilityFrom.area;
					if ( adjArea == area )
						continue;	// don't try to inherit visibility from ourselves
				}

				const CNavArea::CAreaBindInfoArray &delta = area->ComputeVisibilityDelta( adjArea );

				// keep the smallest delta
				if ( !anchor || ( anchor && delta.Count() < bestDelta.Count() ) )
				{
					bestDelta = delta;
					anchor = adjArea;
					Assert( anchor != area );
				}
			}
		}

		// if best delta is small enough, inherit our data from this anchor
		if ( anchor && bestDelta.Count() <= nav_max_vis_delta_list_length.GetInt() && anchor != area )
		{
			// inherit from anchor area's visibility list
			area->m_inheritVisibilityFrom.area = anchor;
			area->m_potentiallyVisibleAreas = bestDelta;

			// mark inherited-from area so it doesn't later try to inherit
			anchor->m_isInheritedFrom = true;
		}
		else
		{
			// retain full list of visible areas
			area->m_inheritVisibilityFrom.area = NULL;
		}
	}

	if ( TheNavAreas.Count() )
	{
		avgVisLength /= TheNavAreas.Count();
	}

	Msg( "NavMesh Visibility List Lengths:  min = %d, avg = %d, max = %d\n", minVisLength, avgVisLength, maxVisLength );
}
