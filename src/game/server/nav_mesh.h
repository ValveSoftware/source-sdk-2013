//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_mesh.h
// The Navigation Mesh interface
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

//
// Author: Michael S. Booth (mike@turtlerockstudios.com), 2003
//
// NOTE: The Navigation code uses Doxygen-style comments. If you run Doxygen over this code, it will 
// auto-generate documentation.  Visit www.doxygen.org to download the system for free.
//

#ifndef _NAV_MESH_H_
#define _NAV_MESH_H_

#include "utlbuffer.h"
#include "filesystem.h"
#include "GameEventListener.h"

#include "nav.h"
#include "nav_area.h"
#include "nav_colors.h"


class CNavArea;
class CBaseEntity; 
class CBreakable;

extern ConVar nav_edit;
extern ConVar nav_quicksave;
extern ConVar nav_show_approach_points;
extern ConVar nav_show_danger;

//--------------------------------------------------------------------------------------------------------
class NavAreaCollector
{
	bool m_checkForDuplicates;
public:
	NavAreaCollector( bool checkForDuplicates = false )
	{
		m_checkForDuplicates = checkForDuplicates;
	}

	bool operator() ( CNavArea *area )
	{
		if ( m_checkForDuplicates && m_area.HasElement( area ) )
			return true;

		m_area.AddToTail( area );
		return true;
	}
	CUtlVector< CNavArea * > m_area;
};


//--------------------------------------------------------------------------------------------------------
class EditDestroyNotification
{
	CNavArea *m_deadArea;

public:
	EditDestroyNotification( CNavArea *deadArea )
	{
		m_deadArea = deadArea;
	}

	bool operator()( CBaseCombatCharacter *actor )
	{
		actor->OnNavAreaRemoved( m_deadArea );
		return true;
	}
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeClearer
{
public:
	NavAttributeClearer( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area )
	{
		area->SetAttributes( area->GetAttributes() & (~m_attribute) );

		return true;
	}

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeSetter
{
public:
	NavAttributeSetter( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area )
	{
		area->SetAttributes( area->GetAttributes() | m_attribute );

		return true;
	}

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
class NavAttributeToggler
{
public:
	NavAttributeToggler( NavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *area );

	NavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
struct NavAttributeLookup
{
	const char *name;
	NavAttributeType attribute;	
};

extern NavAttributeLookup TheNavAttributeTable[];

//--------------------------------------------------------------------------------------------------------
class SelectOverlappingAreas
{
public:
	bool operator()( CNavArea *area );
};

//--------------------------------------------------------------------------------------------------------
abstract_class INavAvoidanceObstacle
{
public:
	virtual bool IsPotentiallyAbleToObstructNavAreas( void ) const = 0;	// could we at some future time obstruct nav?
	virtual float GetNavObstructionHeight( void ) const = 0;			// height at which to obstruct nav areas
	virtual bool CanObstructNavAreas( void ) const = 0;					// can we obstruct nav right this instant?
	virtual CBaseEntity *GetObstructingEntity( void ) = 0;
	virtual void OnNavMeshLoaded( void ) = 0;
};

//--------------------------------------------------------------------------------------------------------
enum GetNavAreaFlags_t
{
	GETNAVAREA_CHECK_LOS			= 0x1,
	GETNAVAREA_ALLOW_BLOCKED_AREAS	= 0x2,
	GETNAVAREA_CHECK_GROUND			= 0x4,
};


//--------------------------------------------------------------------------------------------------------
// for nav mesh visibilty computation
struct NavVisPair_t
{
	void SetPair( CNavArea *pArea1, CNavArea *pArea2 )
	{
		int iArea1 = (int)( pArea1 > pArea2 );
		int iArea2 = ( iArea1 + 1 ) % 2;
		pAreas[iArea1] = pArea1;
		pAreas[iArea2] = pArea2;
	}

	CNavArea *pAreas[2];
};


// for nav mesh visibilty computation
class CVisPairHashFuncs
{
public:
	CVisPairHashFuncs( int ) {}

	bool operator()( const NavVisPair_t &lhs, const NavVisPair_t &rhs ) const
	{
		return ( lhs.pAreas[0] == rhs.pAreas[0] && lhs.pAreas[1] == rhs.pAreas[1] );
	}

	unsigned int operator()( const NavVisPair_t &item ) const
	{
		COMPILE_TIME_ASSERT( sizeof(CNavArea *) == 4 );
		int key[2] = { (int)item.pAreas[0] + item.pAreas[1]->GetID(), (int)item.pAreas[1] + item.pAreas[0]->GetID() };
		return Hash8( key );	
	}
};


//--------------------------------------------------------------------------------------------------------------
//
// The 'place directory' is used to save and load places from
// nav files in a size-efficient manner that also allows for the 
// order of the place ID's to change without invalidating the
// nav files.
//
// The place directory is stored in the nav file as a list of 
// place name strings.  Each nav area then contains an index
// into that directory, or zero if no place has been assigned to 
// that area.
//
class PlaceDirectory
{
public:
	typedef unsigned short IndexType;	// Loaded/Saved as UnsignedShort.  Change this and you'll have to version.

	PlaceDirectory( void );
	void Reset( void );
	bool IsKnown( Place place ) const;						/// return true if this place is already in the directory
	IndexType GetIndex( Place place ) const;				/// return the directory index corresponding to this Place (0 = no entry)
	void AddPlace( Place place );							/// add the place to the directory if not already known
	Place IndexToPlace( IndexType entry ) const;			/// given an index, return the Place
	void Save( CUtlBuffer &fileBuffer );					/// store the directory
	void Load( CUtlBuffer &fileBuffer, int version );		/// load the directory
	const CUtlVector< Place > *GetPlaces( void ) const
	{
		return &m_directory;
	}

	bool HasUnnamedPlaces( void ) const 
	{
		return m_hasUnnamedAreas;
	}


private:
	CUtlVector< Place > m_directory;
	bool m_hasUnnamedAreas;
};

extern PlaceDirectory placeDirectory;



//--------------------------------------------------------------------------------------------------------
/**
 * The CNavMesh is the global interface to the Navigation Mesh.
 * @todo Make this an abstract base class interface, and derive mod-specific implementations.
 */
class CNavMesh : public CGameEventListener
{
public:
	CNavMesh( void );
	virtual ~CNavMesh();
	
	virtual void PreLoadAreas( int nAreas ) {}
	virtual CNavArea *CreateArea( void ) const;							// CNavArea factory
	virtual void DestroyArea( CNavArea * ) const;
	virtual HidingSpot *CreateHidingSpot( void ) const;					// Hiding Spot factory

	virtual void Reset( void );											// destroy Navigation Mesh data and revert to initial state
	virtual void Update( void );										// invoked on each game frame

	virtual void FireGameEvent( IGameEvent *event );					// incoming event processing

	virtual NavErrorType Load( void );									// load navigation data from a file
	virtual NavErrorType PostLoad( unsigned int version );				// (EXTEND) invoked after all areas have been loaded - for pointer binding, etc
	bool IsLoaded( void ) const		{ return m_isLoaded; }				// return true if a Navigation Mesh has been loaded
	bool IsAnalyzed( void ) const	{ return m_isAnalyzed; }			// return true if a Navigation Mesh has been analyzed

	/**
	 * Return true if nav mesh can be trusted for all climbing/jumping decisions because game environment is fairly simple.
	 * Authoritative meshes mean path followers can skip CPU intensive realtime scanning of unpredictable geometry.
	 */
	virtual bool IsAuthoritative( void ) const { return false; }		

	const CUtlVector< Place > *GetPlacesFromNavFile( bool *hasUnnamedPlaces );	// Reads the used place names from the nav file (can be used to selectively precache before the nav is loaded)

	virtual bool Save( void ) const;									// store Navigation Mesh to a file
	bool IsOutOfDate( void ) const	{ return m_isOutOfDate; }			// return true if the Navigation Mesh is older than the current map version

	virtual unsigned int GetSubVersionNumber( void ) const;										// returns sub-version number of data format used by derived classes
	virtual void SaveCustomData( CUtlBuffer &fileBuffer ) const { }								// store custom mesh data for derived classes
	virtual void LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion ) { }			// load custom mesh data for derived classes
	virtual void SaveCustomDataPreArea( CUtlBuffer &fileBuffer ) const { }						// store custom mesh data for derived classes that needs to be loaded before areas are read in
	virtual void LoadCustomDataPreArea( CUtlBuffer &fileBuffer, unsigned int subVersion ) { }	// load custom mesh data for derived classes that needs to be loaded before areas are read in

	// events
	virtual void OnServerActivate( void );								// (EXTEND) invoked when server loads a new map
	virtual void OnRoundRestart( void );								// invoked when a game round restarts
	virtual void OnRoundRestartPreEntity( void );						// invoked when a game round restarts, but before entities are deleted and recreated
	virtual void OnBreakableCreated( CBaseEntity *breakable ) { }		// invoked when a breakable is created
	virtual void OnBreakableBroken( CBaseEntity *broken ) { }			// invoked when a breakable is broken
	virtual void OnAreaBlocked( CNavArea *area );						// invoked when the area becomes blocked
	virtual void OnAreaUnblocked( CNavArea *area );						// invoked when the area becomes un-blocked
	virtual void OnAvoidanceObstacleEnteredArea( CNavArea *area );					// invoked when the area becomes obstructed
	virtual void OnAvoidanceObstacleLeftArea( CNavArea *area );					// invoked when the area becomes un-obstructed

	virtual void OnEditCreateNotify( CNavArea *newArea );				// invoked when given area has just been added to the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavArea *deadArea );				// invoked when given area has just been deleted from the mesh in edit mode
	virtual void OnEditDestroyNotify( CNavLadder *deadLadder );			// invoked when given ladder has just been deleted from the mesh in edit mode
	virtual void OnNodeAdded( CNavNode *node ) {};						

	// Obstructions
	void RegisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction );
	void UnregisterAvoidanceObstacle( INavAvoidanceObstacle *obstruction );
	const CUtlVector< INavAvoidanceObstacle * > &GetObstructions( void ) const { return m_avoidanceObstacles; }

	unsigned int GetNavAreaCount( void ) const	{ return m_areaCount; }	// return total number of nav areas

	// See GetNavAreaFlags_t for flags
	CNavArea *GetNavArea( const Vector &pos, float beneathLimt = 120.0f ) const;	// given a position, return the nav area that IsOverlapping and is *immediately* beneath it
	CNavArea *GetNavArea( CBaseEntity *pEntity, int nGetNavAreaFlags, float flBeneathLimit = 120.0f ) const;
	CNavArea *GetNavAreaByID( unsigned int id ) const;
	CNavArea *GetNearestNavArea( const Vector &pos, bool anyZ = false, float maxDist = 10000.0f, bool checkLOS = false, bool checkGround = true, int team = TEAM_ANY ) const;
	CNavArea *GetNearestNavArea( CBaseEntity *pEntity, int nGetNavAreaFlags = GETNAVAREA_CHECK_GROUND, float maxDist = 10000.0f ) const;

	Place GetPlace( const Vector &pos ) const;							// return Place at given coordinate
	const char *PlaceToName( Place place ) const;						// given a place, return its name
	Place NameToPlace( const char *name ) const;						// given a place name, return a place ID or zero if no place is defined
	Place PartialNameToPlace( const char *name ) const;					// given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
	void PrintAllPlaces( void ) const;									// output a list of names to the console
	int PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );	// Given a partial place name, fill in possible place names for ConCommand autocomplete

	bool GetGroundHeight( const Vector &pos, float *height, Vector *normal = NULL ) const;		// get the Z coordinate of the topmost ground level below the given point
	bool GetSimpleGroundHeight( const Vector &pos, float *height, Vector *normal = NULL ) const;// get the Z coordinate of the ground level directly below the given point


	/// increase "danger" weights in the given nav area and nearby ones
	void IncreaseDangerNearby( int teamID, float amount, CNavArea *area, const Vector &pos, float maxRadius, float dangerLimit = -1.0f );
	void DrawDanger( void ) const;										// draw the current danger levels
	void DrawPlayerCounts( void ) const;								// draw the current player counts for each area
	void DrawFuncNavAvoid( void ) const;								// draw bot avoidance areas from func_nav_avoid entities
	void DrawFuncNavPrefer( void ) const;								// draw bot preference areas from func_nav_prefer entities
#ifdef NEXT_BOT
	void DrawFuncNavPrerequisite( void ) const;							// draw bot prerequisite areas from func_nav_prerequisite entities
#endif
	//-------------------------------------------------------------------------------------
	// Auto-generation
	//
	#define INCREMENTAL_GENERATION true
	void BeginGeneration( bool incremental = false );					// initiate the generation process
	void BeginAnalysis( bool quitWhenFinished = false );						// re-analyze an existing Mesh.  Determine Hiding Spots, Encounter Spots, etc.

	bool IsGenerating( void ) const		{ return m_generationMode != GENERATE_NONE; }	// return true while a Navigation Mesh is being generated
	const char *GetPlayerSpawnName( void ) const;						// return name of player spawn entity
	void SetPlayerSpawnName( const char *name );						// define the name of player spawn entities
	void AddWalkableSeed( const Vector &pos, const Vector &normal );	// add given walkable position to list of seed positions for map sampling
	virtual void AddWalkableSeeds( void );								// adds walkable positions for any/all positions a mod specifies
	void ClearWalkableSeeds( void )		{ m_walkableSeeds.RemoveAll(); }	// erase all walkable seed positions
	void MarkStairAreas( void );

	virtual unsigned int GetGenerationTraceMask( void ) const;			// return the mask used by traces when generating the mesh


	//-------------------------------------------------------------------------------------
	// Edit mode
	//
	unsigned int GetNavPlace( void ) const			{ return m_navPlace; }
	void SetNavPlace( unsigned int place )			{ m_navPlace = place; }

	// Edit callbacks from ConCommands
	void CommandNavDelete( void );										// delete current area
	void CommandNavDeleteMarked( void );								// delete current marked area

	virtual void CommandNavFloodSelect( const CCommand &args );			// select current area and all connected areas, recursively
	void CommandNavToggleSelectedSet( void );							// toggles all areas into/out of the selected set
	void CommandNavStoreSelectedSet( void );							// stores the current selected set for later
	void CommandNavRecallSelectedSet( void );							// restores an older selected set
	void CommandNavAddToSelectedSet( void );							// add current area to selected set
	void CommandNavAddToSelectedSetByID(  const CCommand &args );		// add specified area id to selected set
	void CommandNavRemoveFromSelectedSet( void );						// remove current area from selected set
	void CommandNavToggleInSelectedSet( void );							// add/remove current area from selected set
	void CommandNavClearSelectedSet( void );							// clear the selected set to empty
	void CommandNavBeginSelecting( void );								// start continuously selecting areas into the selected set
	void CommandNavEndSelecting( void );								// stop continuously selecting areas into the selected set
	void CommandNavBeginDragSelecting( void );							// start dragging a selection area
	void CommandNavEndDragSelecting( void );							// stop dragging a selection area
	void CommandNavBeginDragDeselecting( void );						// start dragging a deselection area
	void CommandNavEndDragDeselecting( void );							// stop dragging a deselection area
	void CommandNavRaiseDragVolumeMax( void );							// raise the top of the drag volume
	void CommandNavLowerDragVolumeMax( void );							// lower the top of the drag volume
	void CommandNavRaiseDragVolumeMin( void );							// raise the bottom of the drag volume
	void CommandNavLowerDragVolumeMin( void );							// lower the bottom of the drag volume
	void CommandNavToggleSelecting( bool playSound = true );			// start/stop continuously selecting areas into the selected set
	void CommandNavBeginDeselecting( void );							// start continuously de-selecting areas from the selected set
	void CommandNavEndDeselecting( void );								// stop continuously de-selecting areas from the selected set
	void CommandNavToggleDeselecting( bool playSound = true );			// start/stop continuously de-selecting areas from the selected set
	void CommandNavSelectInvalidAreas( void );							// adds invalid areas to the selected set
	void CommandNavSelectBlockedAreas( void );							// adds blocked areas to the selected set
	void CommandNavSelectObstructedAreas( void );						// adds obstructed areas to the selected set
	void CommandNavSelectDamagingAreas( void );							// adds damaging areas to the selected set
	void CommandNavSelectHalfSpace( const CCommand &args );				// selects all areas that intersect the half-space
	void CommandNavSelectStairs( void );								// adds stairs areas to the selected set
	void CommandNavSelectOrphans( void );								// adds areas not connected to mesh to the selected set

	void CommandNavSplit( void );										// split current area
	void CommandNavMerge( void );										// merge adjacent areas
	void CommandNavMark( const CCommand &args );						// mark an area for further operations
	void CommandNavUnmark( void );										// removes the mark

	void CommandNavBeginArea( void );									// begin creating a new nav area
	void CommandNavEndArea( void );										// end creation of the new nav area

	void CommandNavBeginShiftXY( void );								// begin shifting selected set in the XY plane
	void CommandNavEndShiftXY( void );									// end shifting selected set in the XY plane

	void CommandNavConnect( void );										// connect marked area to selected area
	void CommandNavDisconnect( void );									// disconnect marked area from selected area
	void CommandNavDisconnectOutgoingOneWays( void );					// disconnect all outgoing one-way connects from each area in the selected set
	void CommandNavSplice( void );										// create new area in between marked and selected areas
	void CommandNavCrouch( void );										// toggle crouch attribute on current area
	void CommandNavTogglePlaceMode( void );								// switch between normal and place editing
	void CommandNavSetPlaceMode( void );								// switch between normal and place editing
	void CommandNavPlaceFloodFill( void );								// floodfill areas out from current area
	void CommandNavPlaceSet( void );									// sets the Place for the selected set
	void CommandNavPlacePick( void );									// "pick up" the place at the current area
	void CommandNavTogglePlacePainting( void );							// switch between "painting" places onto areas
	void CommandNavMarkUnnamed( void );									// mark an unnamed area for further operations
	void CommandNavCornerSelect( void );								// select a corner on the current area
	void CommandNavCornerRaise( const CCommand &args );					// raise a corner on the current area
	void CommandNavCornerLower( const CCommand &args );					// lower a corner on the current area
	void CommandNavCornerPlaceOnGround( const CCommand &args );			// position a corner on the current area at ground height
	void CommandNavWarpToMark( void );									// warp a spectating local player to the selected mark
	void CommandNavLadderFlip( void );									// Flips the direction a ladder faces
	void CommandNavToggleAttribute( NavAttributeType attribute );		// toggle an attribute on current area
	void CommandNavMakeSniperSpots( void );								// cuts up the marked area into individual areas suitable for sniper spots
	void CommandNavBuildLadder( void );									// builds a nav ladder on the climbable surface under the cursor
	void CommandNavRemoveJumpAreas( void );								// removes jump areas, replacing them with connections
	void CommandNavSubdivide( const CCommand &args );					// subdivide each nav area in X and Y to create 4 new areas - limit min size
	void CommandNavSaveSelected( const CCommand &args );				// Save selected set to disk
	void CommandNavMergeMesh( const CCommand &args );					// Merge a saved selected set into the current mesh
	void CommandNavMarkWalkable( void );

	void AddToDragSelectionSet( CNavArea *pArea );
	void RemoveFromDragSelectionSet( CNavArea *pArea );
	void ClearDragSelectionSet( void );

	CNavArea *GetMarkedArea( void ) const;										// return area marked by user in edit mode
	CNavLadder *GetMarkedLadder( void ) const	{ return m_markedLadder; }		// return ladder marked by user in edit mode

	CNavArea *GetSelectedArea( void ) const		{ return m_selectedArea; }		// return area user is pointing at in edit mode
	CNavLadder *GetSelectedLadder( void ) const	{ return m_selectedLadder; }	// return ladder user is pointing at in edit mode
	void SetMarkedLadder( CNavLadder *ladder );							// mark ladder for further edit operations
	void SetMarkedArea( CNavArea *area );								// mark area for further edit operations

	bool IsContinuouslySelecting( void ) const
	{
		return m_isContinuouslySelecting;
	}

	bool IsContinuouslyDeselecting( void ) const
	{
		return m_isContinuouslyDeselecting;
	}

	void CreateLadder( const Vector &mins, const Vector &maxs, float maxHeightAboveTopArea );
	void CreateLadder( const Vector &top, const Vector &bottom, float width, const Vector2D &ladderDir, float maxHeightAboveTopArea );

	float SnapToGrid( float x, bool forceGrid = false ) const;					// snap given coordinate to generation grid boundary
	Vector SnapToGrid( const Vector& in, bool snapX = true, bool snapY = true, bool forceGrid = false ) const;	// snap given vector's X & Y coordinates to generation grid boundary

	const Vector &GetEditCursorPosition( void ) const	{ return m_editCursorPos; }	// return position of edit cursor
	void StripNavigationAreas( void );
	const char *GetFilename( void ) const;								// return the filename for this map's "nav" file

	/// @todo Remove old select code and make all commands use this selected set
	void AddToSelectedSet( CNavArea *area );							// add area to the currently selected set
	void RemoveFromSelectedSet( CNavArea *area );						// remove area from the currently selected set
	void ClearSelectedSet( void );										// clear the currently selected set to empty
	bool IsSelectedSetEmpty( void ) const;								// return true if the selected set is empty
	bool IsInSelectedSet( const CNavArea *area ) const;					// return true if the given area is in the selected set
	int GetSelecteSetSize( void ) const;
	const NavAreaVector &GetSelectedSet( void ) const;					// return the selected set

	/**
	 * Apply the functor to all navigation areas in the Selected Set,
	 * or the current selected area.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllSelectedAreas( Functor &func )
	{
		if (IsSelectedSetEmpty())
		{
			CNavArea *area = GetSelectedArea();
			
			if (area)
			{
				if (func( area ) == false)
					return false;
			}
		}
		else
		{
			FOR_EACH_VEC( m_selectedSet, it )
			{
				CNavArea *area = m_selectedSet[ it ];

				if (func( area ) == false)
					return false;
			}
		}
		
		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation areas.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllAreas( Functor &func )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];

			if (func( area ) == false)
				return false;
		}

		return true;
	}

	// const version of the above
	template < typename Functor >
	bool ForAllAreas( Functor &func ) const
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			const CNavArea *area = TheNavAreas[ it ];

			if (func( area ) == false)
				return false;
		}

		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation areas that overlap the given extent.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllAreasOverlappingExtent( Functor &func, const Extent &extent )
	{
		if ( !m_grid.Count() )
		{
#if _DEBUG
			Warning("Query before nav mesh is loaded! %d\n", TheNavAreas.Count() );
#endif
			return true;
		}
		static unsigned int searchMarker = RandomInt(0, 1024*1024 );
		if ( ++searchMarker == 0 )
		{
			++searchMarker;
		}

		Extent areaExtent;

		// get list in cell that contains position
		int startX = WorldToGridX( extent.lo.x );
		int endX = WorldToGridX( extent.hi.x );
		int startY = WorldToGridY( extent.lo.y );
		int endY = WorldToGridY( extent.hi.y );

		for( int x = startX; x <= endX; ++x )
		{
			for( int y = startY; y <= endY; ++y )
			{
				int iGrid = x + y*m_gridSizeX;
				if ( iGrid >= m_grid.Count() )
				{
					ExecuteNTimes( 10, Warning( "** Walked off of the CNavMesh::m_grid in ForAllAreasOverlappingExtent()\n" ) );
					return true;
				}

				NavAreaVector *areaVector = &m_grid[ iGrid ];

				// find closest area in this cell
				FOR_EACH_VEC( (*areaVector), it )
				{
					CNavArea *area = (*areaVector)[ it ];

					// skip if we've already visited this area
					if ( area->m_nearNavSearchMarker == searchMarker )
						continue;

					// mark as visited
					area->m_nearNavSearchMarker = searchMarker;
					area->GetExtent( &areaExtent );

					if ( extent.IsOverlapping( areaExtent ) )
					{
						if ( func( area ) == false )
							return false;
					}
				}
			}
		}
		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Populate the given vector with all navigation areas that overlap the given extent.
	 */
	template< typename NavAreaType >
	void CollectAreasOverlappingExtent( const Extent &extent, CUtlVector< NavAreaType * > *outVector )
	{
		if ( !m_grid.Count() )
		{
			return;
		}

		static unsigned int searchMarker = RandomInt( 0, 1024*1024 );
		if ( ++searchMarker == 0 )
		{
			++searchMarker;
		}

		Extent areaExtent;

		// get list in cell that contains position
		int startX = WorldToGridX( extent.lo.x );
		int endX = WorldToGridX( extent.hi.x );
		int startY = WorldToGridY( extent.lo.y );
		int endY = WorldToGridY( extent.hi.y );

		for( int x = startX; x <= endX; ++x )
		{
			for( int y = startY; y <= endY; ++y )
			{
				int iGrid = x + y*m_gridSizeX;
				if ( iGrid >= m_grid.Count() )
				{
					ExecuteNTimes( 10, Warning( "** Walked off of the CNavMesh::m_grid in CollectAreasOverlappingExtent()\n" ) );
					return;
				}

				NavAreaVector *areaVector = &m_grid[ iGrid ];

				// find closest area in this cell
				for( int v=0; v<areaVector->Count(); ++v )
				{
					CNavArea *area = areaVector->Element( v );

					// skip if we've already visited this area
					if ( area->m_nearNavSearchMarker == searchMarker )
						continue;

					// mark as visited
					area->m_nearNavSearchMarker = searchMarker;
					area->GetExtent( &areaExtent );

					if ( extent.IsOverlapping( areaExtent ) )
					{
						outVector->AddToTail( (NavAreaType *)area );
					}
				}
			}
		}
	}


	template < typename Functor >
	bool ForAllAreasInRadius( Functor &func, const Vector &pos, float radius )
	{
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
		int shiftLimit = ceil( radius / m_gridCellSize );
		float radiusSq = radius * radius;
		if ( radius == 0.0f )
		{
			shiftLimit = MAX( m_gridSizeX, m_gridSizeY );	// range 0 means all areas
		}

		for( int x = originX - shiftLimit; x <= originX + shiftLimit; ++x )
		{
			if ( x < 0 || x >= m_gridSizeX )
				continue;

			for( int y = originY - shiftLimit; y <= originY + shiftLimit; ++y )
			{
				if ( y < 0 || y >= m_gridSizeY )
					continue;

				NavAreaVector *areaVector = &m_grid[ x + y*m_gridSizeX ];

				// find closest area in this cell
				FOR_EACH_VEC( (*areaVector), it )
				{
					CNavArea *area = (*areaVector)[ it ];

					// skip if we've already visited this area
					if ( area->m_nearNavSearchMarker == searchMarker )
						continue;

					// mark as visited
					area->m_nearNavSearchMarker = searchMarker;

					float distSq = ( area->GetCenter() - pos ).LengthSqr();

					if ( ( distSq <= radiusSq ) || ( radiusSq == 0 ) )
					{
						if ( func( area ) == false )
							return false;
					}
				}
			}
		}
		return true;
	}

	//---------------------------------------------------------------------------------------------------------------
	/*
	 * Step through nav mesh along line between startArea and endArea.
	 * Return true if enumeration reached endArea, false if doesn't reach it (no mesh between, bad connection, etc)
	 */
	template < typename Functor >
	bool ForAllAreasAlongLine( Functor &func, CNavArea *startArea, CNavArea *endArea )
	{
		if ( !startArea || !endArea )
			return false;

		if ( startArea == endArea )
		{
			func( startArea );
			return true;
		}

		Vector start = startArea->GetCenter();
		Vector end = endArea->GetCenter();

		Vector to = end - start;
		float range = to.NormalizeInPlace();

		const float epsilon = 0.00001f;

		if ( range < epsilon )
		{
			func( startArea );
			return true;
		}

		if ( abs( to.x ) < epsilon )
		{
			NavDirType dir = ( to.y < 0.0f ) ? NORTH : SOUTH;

			CNavArea *area = startArea;
			while( area )
			{
				func( area );

				if ( area == endArea )
					return true;

				const NavConnectVector *adjVector = area->GetAdjacentAreas( dir );

				area = NULL;

				for( int i=0; i<adjVector->Count(); ++i )
				{
					CNavArea *adjArea = adjVector->Element(i).area;

					const Vector &adjOrigin = adjArea->GetCorner( NORTH_WEST );

					if ( adjOrigin.x <= start.x && adjOrigin.x + adjArea->GetSizeX() >= start.x )
					{
						area = adjArea;
						break;
					}
				}
			}

			return false;
		}
		else if ( abs( to.y ) < epsilon )
		{
			NavDirType dir = ( to.x < 0.0f ) ? WEST : EAST;

			CNavArea *area = startArea;
			while( area )
			{
				func( area );

				if ( area == endArea )
					return true;

				const NavConnectVector *adjVector = area->GetAdjacentAreas( dir );

				area = NULL;

				for( int i=0; i<adjVector->Count(); ++i )
				{
					CNavArea *adjArea = adjVector->Element(i).area;

					const Vector &adjOrigin = adjArea->GetCorner( NORTH_WEST );

					if ( adjOrigin.y <= start.y && adjOrigin.y + adjArea->GetSizeY() >= start.y )
					{
						area = adjArea;
						break;
					}
				}
			}

			return false;
		}


		CNavArea *area = startArea;

		while( area )
		{
			func( area );

			if ( area == endArea )
				return true;

			const Vector &origin = area->GetCorner( NORTH_WEST );
			float xMin = origin.x;
			float xMax = xMin + area->GetSizeX();
			float yMin = origin.y;
			float yMax = yMin + area->GetSizeY();

			// clip ray to area
			Vector exit;
			NavDirType edge = NUM_DIRECTIONS;

			if ( to.x < 0.0f )
			{
				// find Y at west edge intersection
				float t = ( xMin - start.x ) / ( end.x - start.x );
				if ( t > 0.0f && t < 1.0f )
				{
					float y = start.y + t * ( end.y - start.y );
					if ( y >= yMin && y <= yMax )
					{
						// intersects this edge
						exit.x = xMin;
						exit.y = y;
						edge = WEST;
					}
				}
			}
			else
			{
				// find Y at east edge intersection
				float t = ( xMax - start.x ) / ( end.x - start.x );
				if ( t > 0.0f && t < 1.0f )
				{
					float y = start.y + t * ( end.y - start.y );
					if ( y >= yMin && y <= yMax )
					{
						// intersects this edge
						exit.x = xMax;
						exit.y = y;
						edge = EAST;
					}
				}
			}

			if ( edge == NUM_DIRECTIONS )
			{
				if ( to.y < 0.0f )
				{
					// find X at north edge intersection
					float t = ( yMin - start.y ) / ( end.y - start.y );
					if ( t > 0.0f && t < 1.0f )
					{
						float x = start.x + t * ( end.x - start.x );
						if ( x >= xMin && x <= xMax )
						{
							// intersects this edge
							exit.x = x;
							exit.y = yMin;
							edge = NORTH;
						}
					}
				}
				else
				{
					// find X at south edge intersection
					float t = ( yMax - start.y ) / ( end.y - start.y );
					if ( t > 0.0f && t < 1.0f )
					{
						float x = start.x + t * ( end.x - start.x );
						if ( x >= xMin && x <= xMax )
						{
							// intersects this edge
							exit.x = x;
							exit.y = yMax;
							edge = SOUTH;
						}
					}
				}
			}

			if ( edge == NUM_DIRECTIONS )
				break;

			const NavConnectVector *adjVector = area->GetAdjacentAreas( edge );

			area = NULL;

			for( int i=0; i<adjVector->Count(); ++i )
			{
				CNavArea *adjArea = adjVector->Element(i).area;

				const Vector &adjOrigin = adjArea->GetCorner( NORTH_WEST );

				if ( edge == NORTH || edge == SOUTH )
				{
					if ( adjOrigin.x <= exit.x && adjOrigin.x + adjArea->GetSizeX() >= exit.x )
					{
						area = adjArea;
						break;
					}
				}
				else
				{
					if ( adjOrigin.y <= exit.y && adjOrigin.y + adjArea->GetSizeY() >= exit.y )
					{
						area = adjArea;
						break;
					}
				}
			}
		}

		return false;
	}


	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation ladders.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllLadders( Functor &func )
	{
		for ( int i=0; i<m_ladders.Count(); ++i )
		{
			CNavLadder *ladder = m_ladders[i];

			if (func( ladder ) == false)
				return false;
		}

		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation ladders.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllLadders( Functor &func ) const
	{
		for ( int i=0; i<m_ladders.Count(); ++i )
		{
			const CNavLadder *ladder = m_ladders[i];

			if (func( ladder ) == false)
				return false;
		}

		return true;
	}

	//-------------------------------------------------------------------------------------
	/**
	 * tests a new area for connections to adjacent pre-existing areas
	 */
	template < typename Functor > void StitchAreaIntoMesh( CNavArea *area, NavDirType dir, Functor &func );

	//-------------------------------------------------------------------------------------
	/**
	 * Use the functor to test if an area is needing stitching into the existing nav mesh.
	 * The functor is different from how we normally use functors - it does no processing,
	 * and it's return value is true if the area is in the new set to be stiched, and false
	 * if it's a pre-existing area.
	 */
	template < typename Functor >
		bool StitchMesh( Functor &func )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];

			if ( func( area ) )
			{
				StitchAreaIntoMesh( area, NORTH, func );
				StitchAreaIntoMesh( area, SOUTH, func );
				StitchAreaIntoMesh( area, EAST, func );
				StitchAreaIntoMesh( area, WEST, func );
			}
		}

		return true;
	}

	NavLadderVector& GetLadders( void ) { return m_ladders; }	// Returns the list of ladders
	CNavLadder *GetLadderByID( unsigned int id ) const;

	CUtlVector< CNavArea * >& GetTransientAreas( void ) { return m_transientAreas; }

	enum EditModeType
	{
		NORMAL,				// normal mesh editing
		PLACE_PAINTING,		// in place painting mode
		CREATING_AREA,		// creating a new nav area
		CREATING_LADDER,	// creating a nav ladder
		DRAG_SELECTING,		// drag selecting a set of areas
		SHIFTING_XY,		// shifting selected set in XY plane
		SHIFTING_Z,			// shifting selected set in Z plane
	};
	EditModeType GetEditMode( void ) const;						// return the current edit mode
	void SetEditMode( EditModeType mode );						// change the edit mode
	bool IsEditMode( EditModeType mode ) const;					// return true if current mode matches given mode

	bool FindNavAreaOrLadderAlongRay( const Vector &start, const Vector &end, CNavArea **area, CNavLadder **ladder, CNavArea *ignore = NULL );

	void PostProcessCliffAreas();
	void SimplifySelectedAreas( void );	// Simplifies the selected set by reducing to 1x1 areas and re-merging them up with loosened tolerances

protected:
	virtual void PostCustomAnalysis( void ) { }					// invoked when custom analysis step is complete
	bool FindActiveNavArea( void );								// Finds the area or ladder the local player is currently pointing at.  Returns true if a surface was hit by the traceline.
	virtual void RemoveNavArea( CNavArea *area );				// remove an area from the grid
	bool FindGroundForNode( Vector *pos, Vector *normal );
	void GenerateNodes( const Extent &bounds );
	void RemoveNodes( void );

private:
	friend class CNavArea;
	friend class CNavNode;
	friend class CNavUIBasePanel;

	mutable CUtlVector<NavAreaVector> m_grid;
	float m_gridCellSize;										// the width/height of a grid cell for spatially partitioning nav areas for fast access
	int m_gridSizeX;
	int m_gridSizeY;
	float m_minX;
	float m_minY;
	unsigned int m_areaCount;									// total number of nav areas

	bool m_isLoaded;											// true if a Navigation Mesh has been loaded
	bool m_isOutOfDate;											// true if the Navigation Mesh is older than the actual BSP
	bool m_isAnalyzed;											// true if the Navigation Mesh needs analysis

	enum { HASH_TABLE_SIZE = 256 };
	CNavArea *m_hashTable[ HASH_TABLE_SIZE ];					// hash table to optimize lookup by ID
	int ComputeHashKey( unsigned int id ) const;				// returns a hash key for the given nav area ID

	int WorldToGridX( float wx ) const;							// given X component, return grid index
	int WorldToGridY( float wy ) const;							// given Y component, return grid index
	void AllocateGrid( float minX, float maxX, float minY, float maxY );	// clear and reset the grid to the given extents
	void GridToWorld( int gridX, int gridY, Vector *pos ) const;

	void AddNavArea( CNavArea *area );							// add an area to the grid

	void DestroyNavigationMesh( bool incremental = false );		// free all resources of the mesh and reset it to empty state
	void DestroyHidingSpots( void );

	void ComputeBattlefrontAreas( void );						// determine areas where rushing teams will first meet

	//----------------------------------------------------------------------------------
	// Place directory
	//
	char **m_placeName;											// master directory of place names (ie: "places")
	unsigned int m_placeCount;									// number of "places" defined in placeName[]
	void LoadPlaceDatabase( void );								// load the place names from a file

	//----------------------------------------------------------------------------------
	// Edit mode
	//
	EditModeType m_editMode;									// the current edit mode
	bool m_isEditing;											// true if in edit mode

	unsigned int m_navPlace;									// current navigation place for editing
	void OnEditModeStart( void );								// called when edit mode has just been enabled
	void DrawEditMode( void );									// draw navigation areas
	void OnEditModeEnd( void );									// called when edit mode has just been disabled
	void UpdateDragSelectionSet( void );							// update which areas are overlapping the drag selected bounds
	Vector m_editCursorPos;										// current position of the cursor
	CNavArea *m_markedArea;										// currently marked area for edit operations
	CNavArea *m_selectedArea;									// area that is selected this frame
	CNavArea *m_lastSelectedArea;								// area that was selected last frame
	NavCornerType m_markedCorner;								// currently marked corner for edit operations
	Vector m_anchor;											// first corner of an area being created
	bool m_isPlacePainting;										// if true, we set an area's place by pointing at it
	bool m_splitAlongX;											// direction the selected nav area would be split
	float m_splitEdge;											// location of the possible split

	bool m_climbableSurface;									// if true, the cursor is pointing at a climable surface
	Vector m_surfaceNormal;										// Normal of the surface the cursor is pointing at
	Vector m_ladderAnchor;										// first corner of a ladder being created
	Vector m_ladderNormal;										// Normal of the surface of the ladder being created
	CNavLadder *m_selectedLadder;								// ladder that is selected this frame
	CNavLadder *m_lastSelectedLadder;							// ladder that was selected last frame
	CNavLadder *m_markedLadder;									// currently marked ladder for edit operations

	bool FindLadderCorners( Vector *c1, Vector *c2, Vector *c3 );	// computes the other corners of a ladder given m_ladderAnchor, m_editCursorPos, and m_ladderNormal

	void GetEditVectors( Vector *pos, Vector *forward );		// Gets the eye position and view direction of the editing player

	CountdownTimer m_showAreaInfoTimer;							// Timer that controls how long area info is displayed
	
	NavAreaVector m_selectedSet;								// all currently selected areas
	NavAreaVector m_dragSelectionSet;							// all areas in the current drag selection
	bool m_isContinuouslySelecting;								// if true, we are continuously adding to the selected set
	bool m_isContinuouslyDeselecting;							// if true, we are continuously removing from the selected set

	bool m_bIsDragDeselecting;
	int m_nDragSelectionVolumeZMax;
	int m_nDragSelectionVolumeZMin;

	void DoToggleAttribute( CNavArea *area, NavAttributeType attribute );		// toggle an attribute on given area


	//----------------------------------------------------------------------------------
	// Auto-generation
	//
	bool UpdateGeneration( float maxTime = 0.25f );				// process the auto-generation for 'maxTime' seconds. return false if generation is complete.

	virtual void BeginCustomAnalysis( bool bIncremental ) {}
	virtual void EndCustomAnalysis() {}

	CNavNode *m_currentNode;									// the current node we are sampling from
	NavDirType m_generationDir;
	CNavNode *AddNode( const Vector &destPos, const Vector &destNormal, NavDirType dir, CNavNode *source, bool isOnDisplacement, float obstacleHeight, float flObstacleStartDist, float flObstacleEndDist );		// add a nav node and connect it, update current node

	NavLadderVector m_ladders;									// list of ladder navigation representations
	void BuildLadders( void );
	void DestroyLadders( void );

	bool SampleStep( void );									// sample the walkable areas of the map
	void CreateNavAreasFromNodes( void );						// cover all of the sampled nodes with nav areas

	bool TestArea( CNavNode *node, int width, int height );		// check if an area of size (width, height) can fit, starting from node as upper left corner
	int BuildArea( CNavNode *node, int width, int height );		// create a CNavArea of size (width, height) starting fom node at upper left corner
	bool CheckObstacles( CNavNode *node, int width, int height, int x, int y );

	void MarkPlayerClipAreas( void );
	void MarkJumpAreas( void );
	void StichAndRemoveJumpAreas( void );
	void RemoveJumpAreas( void );	
	void SquareUpAreas( void );
	void MergeGeneratedAreas( void );
	void ConnectGeneratedAreas( void );
	void FixUpGeneratedAreas( void );
	void FixCornerOnCornerAreas( void );
	void FixConnections( void );
	void SplitAreasUnderOverhangs( void );
	void ValidateNavAreaConnections( void );
	void StitchGeneratedAreas( void );							// Stitches incrementally-generated areas into the existing mesh
	void StitchAreaSet( CUtlVector< CNavArea * > *areas );		// Stitches an arbitrary set of areas into the existing mesh
	void HandleObstacleTopAreas( void );						// Handles fixing/generating areas on top of slim obstacles such as fences and railings
	void RaiseAreasWithInternalObstacles();
	void CreateObstacleTopAreas();
	bool CreateObstacleTopAreaIfNecessary( CNavArea *area, CNavArea *areaOther, NavDirType dir, bool bMultiNode );
	void RemoveOverlappingObstacleTopAreas();	


	enum GenerationStateType
	{
		SAMPLE_WALKABLE_SPACE,
		CREATE_AREAS_FROM_SAMPLES,
		FIND_HIDING_SPOTS,
		FIND_ENCOUNTER_SPOTS,
		FIND_SNIPER_SPOTS,
		FIND_EARLIEST_OCCUPY_TIMES,
		FIND_LIGHT_INTENSITY,
		COMPUTE_MESH_VISIBILITY,
		CUSTOM,													// mod-specific generation step
		SAVE_NAV_MESH,

		NUM_GENERATION_STATES
	}
	m_generationState;											// the state of the generation process
	enum GenerationModeType
	{
		GENERATE_NONE,
		GENERATE_FULL,
		GENERATE_INCREMENTAL,
		GENERATE_SIMPLIFY,
		GENERATE_ANALYSIS_ONLY,
	}
	m_generationMode;											// true while a Navigation Mesh is being generated
	int m_generationIndex;										// used for iterating nav areas during generation process
	int m_sampleTick;											// counter for displaying pseudo-progress while sampling walkable space
	bool m_bQuitWhenFinished;
	float m_generationStartTime;
	Extent m_simplifyGenerationExtent;

	char *m_spawnName;											// name of player spawn entity, used to initiate sampling

	struct WalkableSeedSpot
	{
		Vector pos;
		Vector normal;
	};
	CUtlVector< WalkableSeedSpot > m_walkableSeeds;				// list of walkable seed spots for sampling

	CNavNode *GetNextWalkableSeedNode( void );					// return the next walkable seed as a node
	int m_seedIdx;
	int m_hostThreadModeRestoreValue;							// stores the value of host_threadmode before we changed it

	void BuildTransientAreaList( void );
	CUtlVector< CNavArea * > m_transientAreas;

	void UpdateAvoidanceObstacleAreas( void );
	CUtlVector< CNavArea * > m_avoidanceObstacleAreas;
	CUtlVector< INavAvoidanceObstacle * > m_avoidanceObstacles;

	void UpdateBlockedAreas( void );
	CUtlVector< CNavArea * > m_blockedAreas;

	CUtlVector< int > m_storedSelectedSet;						// "Stored" selected set, so we can do some editing and then restore the old selected set.  Done by ID, so we don't have to worry about split/delete/etc.

	void BeginVisibilityComputations( void );
	void EndVisibilityComputations( void );

	void TestAllAreasForBlockedStatus( void );					// Used to update blocked areas after a round restart. Need to delay so the map logic has all fired.
	CountdownTimer m_updateBlockedAreasTimer;			
};

// the global singleton interface
extern CNavMesh *TheNavMesh;

// factory for creating the Navigation Mesh
extern CNavMesh *NavMeshFactory( void );

#ifdef STAGING_ONLY
// for debugging the A* algorithm, if nonzero, show debug display and decrement for each pathfind
extern int g_DebugPathfindCounter;
#endif


//--------------------------------------------------------------------------------------------------------------
inline bool CNavMesh::IsEditMode( EditModeType mode ) const
{
	return m_editMode == mode;
}

//--------------------------------------------------------------------------------------------------------------
inline CNavMesh::EditModeType CNavMesh::GetEditMode( void ) const
{
	return m_editMode;
}

//--------------------------------------------------------------------------------------------------------------
inline unsigned int CNavMesh::GetSubVersionNumber( void ) const
{
	return 0;
}


//--------------------------------------------------------------------------------------------------------------
inline CNavArea *CNavMesh::CreateArea( void ) const
{
	return new CNavArea;
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavMesh::DestroyArea( CNavArea *pArea ) const
{
	delete pArea;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::ComputeHashKey( unsigned int id ) const
{
	return id & 0xFF;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridX( float wx ) const
{ 
	int x = (int)( (wx - m_minX) / m_gridCellSize );

	if (x < 0)
		x = 0;
	else if (x >= m_gridSizeX)
		x = m_gridSizeX-1;
	
	return x;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridY( float wy ) const
{ 
	int y = (int)( (wy - m_minY) / m_gridCellSize );

	if (y < 0)
		y = 0;
	else if (y >= m_gridSizeY)
		y = m_gridSizeY-1;
	
	return y;
}


//--------------------------------------------------------------------------------------------------------------
inline unsigned int CNavMesh::GetGenerationTraceMask( void ) const
{
	return MASK_NPCSOLID_BRUSHONLY;
}


//--------------------------------------------------------------------------------------------------------------
//
// Function prototypes
//

extern void ApproachAreaAnalysisPrep( void );
extern void CleanupApproachAreaAnalysisPrep( void );
extern bool IsHeightDifferenceValid( float test, float other1, float other2, float other3 );

#endif // _NAV_MESH_H_
