//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_file.cpp
// Reading and writing nav files
// Author: Michael S. Booth (mike@turtlerockstudios.com), January-September 2003

#include "cbase.h"
#include "nav_mesh.h"
#include "gamerules.h"
#include "datacache/imdlcache.h"

#ifdef TERROR
#include "func_elevator.h"
#endif

#include "tier1/lzmaDecoder.h"

#ifdef CSTRIKE_DLL
#include "cs_shareddefs.h"
#include "nav_pathfind.h"
#include "cs_nav_area.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------------
/// The current version of the nav file format

/// IMPORTANT: If this version changes, the swap function in makegamedata 
/// must be updated to match. If not, this will break the Xbox 360.
// TODO: Was changed from 15, update when latest 360 code is integrated (MSB 5/5/09)
const int NavCurrentVersion = 16;

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
PlaceDirectory::PlaceDirectory( void )
{
	Reset();
}

void PlaceDirectory::Reset( void )
{
	m_directory.RemoveAll();
	m_hasUnnamedAreas = false;
}

/// return true if this place is already in the directory
bool PlaceDirectory::IsKnown( Place place ) const
{
	return m_directory.HasElement( place );
}

/// return the directory index corresponding to this Place (0 = no entry)
PlaceDirectory::IndexType PlaceDirectory::GetIndex( Place place ) const
{
	if (place == UNDEFINED_PLACE)
		return 0;

	int i = m_directory.Find( place );

	if (i < 0)
	{
		AssertMsg( false, "PlaceDirectory::GetIndex failure" );
		return 0;
	}

	return (IndexType)(i+1);
}

/// add the place to the directory if not already known
void PlaceDirectory::AddPlace( Place place )
{
	if (place == UNDEFINED_PLACE)
	{
		m_hasUnnamedAreas = true;
		return;
	}

	Assert( place < 1000 );

	if (IsKnown( place ))
		return;

	m_directory.AddToTail( place );
}

/// given an index, return the Place
Place PlaceDirectory::IndexToPlace( IndexType entry ) const
{
	if (entry == 0)
		return UNDEFINED_PLACE;

	int i = entry-1;

	if (i >= m_directory.Count())
	{
		AssertMsg( false, "PlaceDirectory::IndexToPlace: Invalid entry" );
		return UNDEFINED_PLACE;
	}

	return m_directory[ i ];
}

/// store the directory
void PlaceDirectory::Save( CUtlBuffer &fileBuffer )
{
	// store number of entries in directory
	IndexType count = (IndexType)m_directory.Count();
	fileBuffer.PutUnsignedShort( count );

	// store entries		
	for( int i=0; i<m_directory.Count(); ++i )
	{
		const char *placeName = TheNavMesh->PlaceToName( m_directory[i] );

		// store string length followed by string itself
		unsigned short len = (unsigned short)(strlen( placeName ) + 1);
		fileBuffer.PutUnsignedShort( len );
		fileBuffer.Put( placeName, len );
	}

	fileBuffer.PutUnsignedChar( m_hasUnnamedAreas );
}

/// load the directory
void PlaceDirectory::Load( CUtlBuffer &fileBuffer, int version )
{
	// read number of entries
	IndexType count = fileBuffer.GetUnsignedShort();

	m_directory.RemoveAll();

	// read each entry
	char placeName[256];
	unsigned short len;
	for( int i=0; i<count; ++i )
	{
		len = fileBuffer.GetUnsignedShort();
		fileBuffer.Get( placeName, MIN( sizeof( placeName ), len ) );

		Place place = TheNavMesh->NameToPlace( placeName );
		if (place == UNDEFINED_PLACE)
		{
			Warning( "Warning: NavMesh place %s is undefined?\n", placeName );
		}
		AddPlace( place );
	}

	if ( version > 11 )
	{
		m_hasUnnamedAreas = fileBuffer.GetUnsignedChar() != 0;
	}
}



PlaceDirectory placeDirectory;

#if defined( _X360 )
	#define FORMAT_BSPFILE "maps\\%s.360.bsp"
	#define FORMAT_NAVFILE "maps\\%s.360.nav"
#else
	#define FORMAT_BSPFILE "maps\\%s.bsp"
	#define FORMAT_NAVFILE "maps\\%s.nav"
#endif

//--------------------------------------------------------------------------------------------------------------
/**
 * Replace extension with "bsp"
 */
char *GetBspFilename( const char *navFilename )
{
	static char bspFilename[256];

	Q_snprintf( bspFilename, sizeof( bspFilename ), FORMAT_BSPFILE, STRING( gpGlobals->mapname ) );

	int len = strlen( bspFilename );
	if (len < 3)
		return NULL;

	bspFilename[ len-3 ] = 'b';
	bspFilename[ len-2 ] = 's';
	bspFilename[ len-1 ] = 'p';

	return bspFilename;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Save a navigation area to the opened binary stream
 */
void CNavArea::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	// save ID
	fileBuffer.PutUnsignedInt( m_id );

	// save attribute flags
	fileBuffer.PutInt( m_attributeFlags );

	// save extent of area
	fileBuffer.Put( &m_nwCorner, 3*sizeof(float) );
	fileBuffer.Put( &m_seCorner, 3*sizeof(float) );

	// save heights of implicit corners
	fileBuffer.PutFloat( m_neZ );
	fileBuffer.PutFloat( m_swZ );

	// save connections to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		// save number of connections for this direction
		unsigned int count = m_connect[d].Count();
		fileBuffer.PutUnsignedInt( count );

		FOR_EACH_VEC( m_connect[d], it )
		{
			NavConnect connect = m_connect[d][ it ];
			fileBuffer.PutUnsignedInt( connect.area->m_id );
		}
	}

	//
	// Store hiding spots for this area
	//
	unsigned char count;
	if (m_hidingSpots.Count() > 255)
	{
		count = 255;
		Warning( "Warning: NavArea #%d: Truncated hiding spot list to 255\n", m_id );
	}
	else
	{
		count = (unsigned char)m_hidingSpots.Count();
	}
	fileBuffer.PutUnsignedChar( count );

	// store HidingSpot objects
	unsigned int saveCount = 0;
	FOR_EACH_VEC( m_hidingSpots, hit )
	{
		HidingSpot *spot = m_hidingSpots[ hit ];
		
		spot->Save( fileBuffer, version );

		// overflow check
		if (++saveCount == count)
			break;
	}

	//
	// Save encounter spots for this area
	//
	{
		// save number of encounter paths for this area
		unsigned int count = m_spotEncounters.Count();
		fileBuffer.PutUnsignedInt( count );

		SpotEncounter *e;
		FOR_EACH_VEC( m_spotEncounters, it )
		{
			e = m_spotEncounters[ it ];

			if (e->from.area)
				fileBuffer.PutUnsignedInt( e->from.area->m_id );
			else
				fileBuffer.PutUnsignedInt( 0 );

			unsigned char dir = (unsigned char)e->fromDir;
			fileBuffer.PutUnsignedChar( dir );

			if (e->to.area)
				fileBuffer.PutUnsignedInt( e->to.area->m_id );
			else
				fileBuffer.PutUnsignedInt( 0 );

			dir = (unsigned char)e->toDir;
			fileBuffer.PutUnsignedChar( dir );

			// write list of spots along this path
			unsigned char spotCount;
			if (e->spots.Count() > 255)
			{
				spotCount = 255;
				Warning( "Warning: NavArea #%d: Truncated encounter spot list to 255\n", m_id );
			}
			else
			{
				spotCount = (unsigned char)e->spots.Count();
			}
			fileBuffer.PutUnsignedChar( spotCount );
		
			saveCount = 0;
			FOR_EACH_VEC( e->spots, sit )
			{
				SpotOrder *order = &e->spots[ sit ];

				// order->spot may be NULL if we've loaded a nav mesh that has been edited but not re-analyzed
				unsigned int id = (order->spot) ? order->spot->GetID() : 0;
				fileBuffer.PutUnsignedInt( id );

				unsigned char t = (unsigned char)(255 * order->t);
				fileBuffer.PutUnsignedChar( t );

				// overflow check
				if (++saveCount == spotCount)
					break;
			}
		}
	}

	// store place dictionary entry
	PlaceDirectory::IndexType entry = placeDirectory.GetIndex( GetPlace() );
	fileBuffer.Put( &entry, sizeof(entry) );

	// write out ladder info
	int i;
	for ( i=0; i<CNavLadder::NUM_LADDER_DIRECTIONS; ++i )
	{
		// save number of encounter paths for this area
		unsigned int count = m_ladder[i].Count();
		fileBuffer.PutUnsignedInt( count );

		NavLadderConnect ladder;
		FOR_EACH_VEC( m_ladder[i], it )
		{
			ladder = m_ladder[i][it];

			unsigned int id = ladder.ladder->GetID();
			fileBuffer.PutUnsignedInt( id );
		}
	}

	// save earliest occupy times
	for( i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		fileBuffer.Put( &m_earliestOccupyTime[i], sizeof(m_earliestOccupyTime[i]) );
	}

	// save light intensity
	for ( i=0; i<NUM_CORNERS; ++i )
	{
		fileBuffer.PutFloat( m_lightIntensity[i] );
	}

	// save visible area set
	unsigned int visibleAreaCount = m_potentiallyVisibleAreas.Count();
	fileBuffer.PutUnsignedInt( visibleAreaCount );

	for ( int vit=0; vit<m_potentiallyVisibleAreas.Count(); ++vit )
	{
		CNavArea *area = m_potentiallyVisibleAreas[ vit ].area;

		unsigned int id = area ? area->GetID() : 0;

		fileBuffer.PutUnsignedInt( id );
		fileBuffer.PutUnsignedChar( m_potentiallyVisibleAreas[ vit ].attributes );
	}

	// store area we inherit visibility from
	unsigned int id = ( m_inheritVisibilityFrom.area ) ? m_inheritVisibilityFrom.area->GetID() : 0;
	fileBuffer.PutUnsignedInt( id );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load a navigation area from the file
 */
NavErrorType CNavArea::Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion )
{
	// load ID
	m_id = fileBuffer.GetUnsignedInt();

	// update nextID to avoid collisions
	if (m_id >= m_nextID)
		m_nextID = m_id+1;

	// load attribute flags
	if ( version <= 8 )
	{
		m_attributeFlags = fileBuffer.GetUnsignedChar();
	}
	else if ( version < 13 )
	{
		m_attributeFlags = fileBuffer.GetUnsignedShort();
	}
	else
	{
		m_attributeFlags = fileBuffer.GetInt();
	}

	// load extent of area
	fileBuffer.Get( &m_nwCorner, 3*sizeof(float) );
	fileBuffer.Get( &m_seCorner, 3*sizeof(float) );

	m_center.x = (m_nwCorner.x + m_seCorner.x)/2.0f;
	m_center.y = (m_nwCorner.y + m_seCorner.y)/2.0f;
	m_center.z = (m_nwCorner.z + m_seCorner.z)/2.0f;

	if ( ( m_seCorner.x - m_nwCorner.x ) > 0.0f && ( m_seCorner.y - m_nwCorner.y ) > 0.0f )
	{
		m_invDxCorners = 1.0f / ( m_seCorner.x - m_nwCorner.x );
		m_invDyCorners = 1.0f / ( m_seCorner.y - m_nwCorner.y );
	}
	else
	{
		m_invDxCorners = m_invDyCorners = 0;

		DevWarning( "Degenerate Navigation Area #%d at setpos %g %g %g\n", 
			m_id, m_center.x, m_center.y, m_center.z );
	}

	// load heights of implicit corners
	m_neZ = fileBuffer.GetFloat();
	m_swZ = fileBuffer.GetFloat();

	CheckWaterLevel();

	// load connections (IDs) to adjacent areas
	// in the enum order NORTH, EAST, SOUTH, WEST
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		// load number of connections for this direction
		unsigned int count = fileBuffer.GetUnsignedInt();
		Assert( fileBuffer.IsValid() );

		m_connect[d].EnsureCapacity( count );
		for( unsigned int i=0; i<count; ++i )
		{
			NavConnect connect;
			connect.id = fileBuffer.GetUnsignedInt();
			Assert( fileBuffer.IsValid() );

			// don't allow self-referential connections
			if ( connect.id != m_id )
			{
				m_connect[d].AddToTail( connect );
			}
		}
	}

	//
	// Load hiding spots
	//

	// load number of hiding spots
	unsigned char hidingSpotCount = fileBuffer.GetUnsignedChar();

	if (version == 1)
	{
		// load simple vector array
		Vector pos;
		for( int h=0; h<hidingSpotCount; ++h )
		{
			fileBuffer.Get( &pos, 3 * sizeof(float) );

			// create new hiding spot and put on master list
			HidingSpot *spot = TheNavMesh->CreateHidingSpot();
			spot->SetPosition( pos );
			spot->SetFlags( HidingSpot::IN_COVER );
			m_hidingSpots.AddToTail( spot );
		}
	}
	else
	{
		// load HidingSpot objects for this area
		for( int h=0; h<hidingSpotCount; ++h )
		{
			// create new hiding spot and put on master list
			HidingSpot *spot = TheNavMesh->CreateHidingSpot();

			spot->Load( fileBuffer, version );
			
			m_hidingSpots.AddToTail( spot );
		}
	}

	if ( version < 15 )
	{
		//
		// Eat the approach areas
		//
		int nToEat = fileBuffer.GetUnsignedChar();

		// load approach area info (IDs)
		for( int a=0; a<nToEat; ++a )
		{
			fileBuffer.GetUnsignedInt();
			fileBuffer.GetUnsignedInt();
			fileBuffer.GetUnsignedChar();
			fileBuffer.GetUnsignedInt();
			fileBuffer.GetUnsignedChar();
		}
	}


	//
	// Load encounter paths for this area
	//
	unsigned int count = fileBuffer.GetUnsignedInt();

	if (version < 3)
	{
		// old data, read and discard
		for( unsigned int e=0; e<count; ++e )
		{
			SpotEncounter encounter;

			encounter.from.id = fileBuffer.GetUnsignedInt();
			encounter.to.id = fileBuffer.GetUnsignedInt();

			fileBuffer.Get( &encounter.path.from.x, 3 * sizeof(float) );
			fileBuffer.Get( &encounter.path.to.x, 3 * sizeof(float) );

			// read list of spots along this path
			unsigned char spotCount = fileBuffer.GetUnsignedChar();
		
			for( int s=0; s<spotCount; ++s )
			{
				fileBuffer.GetFloat();
				fileBuffer.GetFloat();
				fileBuffer.GetFloat();
				fileBuffer.GetFloat();
			}
		}
		return NAV_OK;
	}

	for( unsigned int e=0; e<count; ++e )
	{
		SpotEncounter *encounter = new SpotEncounter;

		encounter->from.id = fileBuffer.GetUnsignedInt();

		unsigned char dir = fileBuffer.GetUnsignedChar();
		encounter->fromDir = static_cast<NavDirType>( dir );

		encounter->to.id = fileBuffer.GetUnsignedInt();

		dir = fileBuffer.GetUnsignedChar();
		encounter->toDir = static_cast<NavDirType>( dir );

		// read list of spots along this path
		unsigned char spotCount = fileBuffer.GetUnsignedChar();
	
		SpotOrder order;
		for( int s=0; s<spotCount; ++s )
		{
			order.id = fileBuffer.GetUnsignedInt();

			unsigned char t = fileBuffer.GetUnsignedChar();

			order.t = (float)t/255.0f;

			encounter->spots.AddToTail( order );
		}

		m_spotEncounters.AddToTail( encounter );
	}

	if (version < 5)
		return NAV_OK;

	//
	// Load Place data
	//
	PlaceDirectory::IndexType entry = fileBuffer.GetUnsignedShort();

	// convert entry to actual Place
	SetPlace( placeDirectory.IndexToPlace( entry ) );

	if ( version < 7 )
		return NAV_OK;

	// load ladder data
	for ( int dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		count = fileBuffer.GetUnsignedInt();
		for( unsigned int i=0; i<count; ++i )
		{
			NavLadderConnect connect;
			connect.id = fileBuffer.GetUnsignedInt();

			bool alreadyConnected = false;
			FOR_EACH_VEC( m_ladder[dir], j )
			{
				if ( m_ladder[dir][j].id == connect.id )
				{
					alreadyConnected = true;
					break;
				}
			}

			if ( !alreadyConnected )
			{
				m_ladder[dir].AddToTail( connect );
			}
		}
	}

	if ( version < 8 )
		return NAV_OK;

	// load earliest occupy times
	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		m_earliestOccupyTime[i] = fileBuffer.GetFloat();
	}

	if ( version < 11 )
		return NAV_OK;

	// load light intensity
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		m_lightIntensity[i] = fileBuffer.GetFloat();
	}

	if ( version < 16 )
		return NAV_OK;

	// load visibility information
	unsigned int visibleAreaCount = fileBuffer.GetUnsignedInt();
	if ( !IsX360() )
	{
		m_potentiallyVisibleAreas.EnsureCapacity( visibleAreaCount );
	}
	else
	{
/* TODO: Re-enable when latest 360 code gets integrated (MSB 5/5/09)
		size_t nBytes = visibleAreaCount * sizeof( AreaBindInfo ); 
		m_potentiallyVisibleAreas.~CAreaBindInfoArray();
		new ( &m_potentiallyVisibleAreas ) CAreaBindInfoArray( (AreaBindInfo *)engine->AllocLevelStaticData( nBytes ), visibleAreaCount );
*/
	}

	for( unsigned int j=0; j<visibleAreaCount; ++j )
	{
		AreaBindInfo info;
		info.id = fileBuffer.GetUnsignedInt();
		info.attributes = fileBuffer.GetUnsignedChar();

		m_potentiallyVisibleAreas.AddToTail( info );
	}

	// read area from which we inherit visibility
	m_inheritVisibilityFrom.id = fileBuffer.GetUnsignedInt();

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Convert loaded IDs to pointers
 * Make sure all IDs are converted, even if corrupt data is encountered.
 */
NavErrorType CNavArea::PostLoad( void )
{
	NavErrorType error = NAV_OK;

	for ( int dir=0; dir<CNavLadder::NUM_LADDER_DIRECTIONS; ++dir )
	{
		FOR_EACH_VEC( m_ladder[dir], it )
		{
			NavLadderConnect& connect = m_ladder[dir][it];

			unsigned int id = connect.id;

			if ( TheNavMesh->GetLadders().Find( connect.ladder ) == TheNavMesh->GetLadders().InvalidIndex() )
			{
				connect.ladder = TheNavMesh->GetLadderByID( id );
			}

			if (id && connect.ladder == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation ladder data. Cannot connect Navigation Areas.\n" );
				error = NAV_CORRUPT_DATA;
			}
		}
	}

	// connect areas together
	for( int d=0; d<NUM_DIRECTIONS; d++ )
	{
		FOR_EACH_VEC( m_connect[d], it )
		{
			NavConnect *connect = &m_connect[ d ][ it ];

			// convert connect ID into an actual area
			unsigned int id = connect->id;
			connect->area = TheNavMesh->GetNavAreaByID( id );
			if (id && connect->area == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation data. Cannot connect Navigation Areas.\n" );
				error = NAV_CORRUPT_DATA;
			}
			connect->length = ( connect->area->GetCenter() - GetCenter() ).Length();
		}
	}

	// resolve spot encounter IDs
	SpotEncounter *e;
	FOR_EACH_VEC( m_spotEncounters, it )
	{
		e = m_spotEncounters[ it ];

		e->from.area = TheNavMesh->GetNavAreaByID( e->from.id );
		if (e->from.area == NULL)
		{
			Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing \"from\" Navigation Area for Encounter Spot.\n" );
			error = NAV_CORRUPT_DATA;
		}

		e->to.area = TheNavMesh->GetNavAreaByID( e->to.id );
		if (e->to.area == NULL)
		{
			Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing \"to\" Navigation Area for Encounter Spot.\n" );
			error = NAV_CORRUPT_DATA;
		}

		if (e->from.area && e->to.area)
		{
			// compute path
			float halfWidth;
			ComputePortal( e->to.area, e->toDir, &e->path.to, &halfWidth );
			ComputePortal( e->from.area, e->fromDir, &e->path.from, &halfWidth );

			const float eyeHeight = HalfHumanHeight;
			e->path.from.z = e->from.area->GetZ( e->path.from ) + eyeHeight;
			e->path.to.z = e->to.area->GetZ( e->path.to ) + eyeHeight;
		}

		// resolve HidingSpot IDs
		FOR_EACH_VEC( e->spots, sit )
		{
			SpotOrder *order = &e->spots[ sit ];

			order->spot = GetHidingSpotByID( order->id );
			if (order->spot == NULL)
			{
				Msg( "CNavArea::PostLoad: Corrupt navigation data. Missing Hiding Spot\n" );
				error = NAV_CORRUPT_DATA;
			}
		}
	}

	// convert visible ID's to pointers to actual areas
	for ( int it=0; it<m_potentiallyVisibleAreas.Count(); ++it )
	{
		AreaBindInfo &info = m_potentiallyVisibleAreas[ it ];

		info.area = TheNavMesh->GetNavAreaByID( info.id );
		if ( info.area == NULL )
		{
			Warning( "Invalid area in visible set for area #%d\n", GetID() );
		}		
	}

	m_inheritVisibilityFrom.area = TheNavMesh->GetNavAreaByID( m_inheritVisibilityFrom.id );
	Assert( m_inheritVisibilityFrom.area != this );

	// remove any invalid areas from the list
	AreaBindInfo bad;
	bad.area = NULL;
	while( m_potentiallyVisibleAreas.FindAndRemove( bad ) );

	// func avoid/prefer attributes are controlled by func_nav_cost entities
	ClearAllNavCostEntities();

	return error;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute travel distance along shortest path from startPos to goalPos. 
 * Return -1 if can't reach endPos from goalPos.
 */
template< typename CostFunctor >
float NavAreaTravelDistance( const Vector &startPos, const Vector &goalPos, CostFunctor &costFunc )
{
	CNavArea *startArea = TheNavMesh->GetNearestNavArea( startPos );
	if (startArea == NULL)
	{
		return -1.0f;
	}

	// compute path between areas using given cost heuristic
	CNavArea *goalArea = NULL;
	if (NavAreaBuildPath( startArea, NULL, &goalPos, costFunc, &goalArea ) == false)
	{
		return -1.0f;
	}

	// compute distance along path
	if (goalArea->GetParent() == NULL)
	{
		// both points are in the same area - return euclidean distance
		return (goalPos - startPos).Length();
	}
	else
	{
		CNavArea *area;
		float distance;

		// goalPos is assumed to be inside goalArea (or very close to it) - skip to next area
		area = goalArea->GetParent();
		distance = (goalPos - area->GetCenter()).Length();

		for( ; area->GetParent(); area = area->GetParent() )
		{
			distance += (area->GetCenter() - area->GetParent()->GetCenter()).Length();
		}

		// add in distance to startPos
		distance += (startPos - area->GetCenter()).Length();

		return distance;
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine the earliest time this hiding spot can be reached by either team
 */
void CNavArea::ComputeEarliestOccupyTimes( void )
{
#ifdef CSTRIKE_DLL
	/// @todo Derive cstrike-specific navigation classes

	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		// no spot in the map should take longer than this to reach
		m_earliestOccupyTime[i] = 120.0f;
	}

	if (nav_quicksave.GetBool())
		return;

	// maximum player speed in units/second
	const float playerSpeed = 240.0f;

	ShortestPathCost cost;
	CBaseEntity *spot;

	// determine the shortest time it will take a Terrorist to reach this area
	int team = TEAM_TERRORIST % MAX_NAV_TEAMS;
	for( spot = gEntList.FindEntityByClassname( NULL, "info_player_terrorist" );
		 spot;
		 spot = gEntList.FindEntityByClassname( spot, "info_player_terrorist" ) )
	{
		float travelDistance = NavAreaTravelDistance( spot->GetAbsOrigin(), m_center, cost );
		if (travelDistance < 0.0f)
			continue;

		float travelTime = travelDistance / playerSpeed;
		if (travelTime < m_earliestOccupyTime[ team ])
		{
			m_earliestOccupyTime[ team ] = travelTime;
		}
	}


	// determine the shortest time it will take a CT to reach this area
	team = TEAM_CT % MAX_NAV_TEAMS;
	for( spot = gEntList.FindEntityByClassname( NULL, "info_player_counterterrorist" );
		 spot;
		 spot = gEntList.FindEntityByClassname( spot, "info_player_counterterrorist" ) )
	{
		float travelDistance = NavAreaTravelDistance( spot->GetAbsOrigin(), m_center, cost );
		if (travelDistance < 0.0f)
			continue;

		float travelTime = travelDistance / playerSpeed;
		if (travelTime < m_earliestOccupyTime[ team ])
		{
			m_earliestOccupyTime[ team ] = travelTime;
		}
	}

#else
	for( int i=0; i<MAX_NAV_TEAMS; ++i )
	{
		m_earliestOccupyTime[i] = 0.0f;
	}
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine if this area is a "battlefront" area - where two rushing teams first meet.
 */
void CNavMesh::ComputeBattlefrontAreas( void )
{
#if 0
#ifdef CSTRIKE_DLL
	ShortestPathCost cost;
	CBaseEntity *tSpawn, *ctSpawn;

	for( tSpawn = gEntList.FindEntityByClassname( NULL, "info_player_terrorist" );
		 tSpawn;
		 tSpawn = gEntList.FindEntityByClassname( tSpawn, "info_player_terrorist" ) )
	{
		CNavArea *tArea = TheNavMesh->GetNavArea( tSpawn->GetAbsOrigin() );
		if (tArea == NULL)
			continue;

		for( ctSpawn = gEntList.FindEntityByClassname( NULL, "info_player_counterterrorist" );
			 ctSpawn;
			 ctSpawn = gEntList.FindEntityByClassname( ctSpawn, "info_player_counterterrorist" ) )
		{
			CNavArea *ctArea = TheNavMesh->GetNavArea( ctSpawn->GetAbsOrigin() );

			if (ctArea == NULL)
				continue;

			if (tArea == ctArea)
			{
				m_isBattlefront = true;
				return;
			}

			// build path between these two spawn points - assume if path fails, it at least got close
			// (ie: imagine spawn points that you jump down from - can't path to)
			CNavArea *goalArea = NULL;
			NavAreaBuildPath( tArea, ctArea, NULL, cost, &goalArea );

			if (goalArea == NULL)
				continue;


/**
 * @todo Need to enumerate ALL paths between all pairs of spawn points to find all battlefront areas
 */

			// find the area with the earliest overlapping occupy times
			CNavArea *battlefront = NULL;
			float earliestTime = 999999.9f;

			const float epsilon = 1.0f;
			CNavArea *area;
			for( area = goalArea; area; area = area->GetParent() )
			{
				if (fabs(area->GetEarliestOccupyTime( TEAM_TERRORIST ) - area->GetEarliestOccupyTime( TEAM_CT )) < epsilon)
				{
				}
				
			}
		}
	}
#endif
#endif
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return the filename for this map's "nav map" file
 */
const char *CNavMesh::GetFilename( void ) const
{
	// filename is local to game dir for Steam, so we need to prepend game dir for regular file save
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	// persistant return value
	static char filename[256];
	Q_snprintf( filename, sizeof( filename ), "%s\\" FORMAT_NAVFILE, gamePath, STRING( gpGlobals->mapname ) );

	return filename;
}

//--------------------------------------------------------------------------------------------------------------
/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
inline void COM_FixSlashes( char *pname )
{
#ifdef _WIN32
	while ( *pname ) 
	{
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
#else
	while ( *pname ) 
	{
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
#endif
}

static void WarnIfMeshNeedsAnalysis( int version )
{
	// Quick check to warn about needing to analyze: nav_strip, nav_delete, etc set
	// every CNavArea's m_approachCount to 0, and delete their m_spotEncounterList.
	// So, if no area has either, odds are good we need an analyze.

	if ( version >= 14 )
	{
		if ( !TheNavMesh->IsAnalyzed() )
		{
			Warning( "The nav mesh needs a full nav_analyze\n" );
			return;
		}
	}
#ifdef CSTRIKE_DLL
	else
	{
		bool hasApproachAreas = false;
		bool hasSpotEncounters = false;

		FOR_EACH_VEC( TheNavAreas, it )
		{
			CCSNavArea *area = dynamic_cast< CCSNavArea * >( TheNavAreas[ it ] );
			if ( area )
			{
				if ( area->GetApproachInfoCount() )
				{
					hasApproachAreas = true;
				}

				if ( area->GetSpotEncounterCount() )
				{
					hasSpotEncounters = true;
				}
			}
		}

		if ( !hasApproachAreas || !hasSpotEncounters )
		{
			Warning( "The nav mesh needs a full nav_analyze\n" );
		}
	}
#endif
}

/**
 * Store Navigation Mesh to a file
 */
bool CNavMesh::Save( void ) const
{
	WarnIfMeshNeedsAnalysis( NavCurrentVersion );

	const char *filename = GetFilename();
	if (filename == NULL)
		return false;

	//
	// Store the NAV file
	//
	COM_FixSlashes( const_cast<char *>(filename) );

	// get size of source bsp file for later (before we open the nav file for writing, in
	// case of failure)
	char *bspFilename = GetBspFilename( filename );
	if (bspFilename == NULL)
	{
		return false;
	}

	CUtlBuffer fileBuffer( 4096, 1024*1024 );

	// store "magic number" to help identify this kind of file
	unsigned int magic = NAV_MAGIC_NUMBER;
	fileBuffer.PutUnsignedInt( magic );

	// store version number of file
	// 1 = hiding spots as plain vector array
	// 2 = hiding spots as HidingSpot objects
	// 3 = Encounter spots use HidingSpot ID's instead of storing vector again
	// 4 = Includes size of source bsp file to verify nav data correlation
	// ---- Beta Release at V4 -----
	// 5 = Added Place info
	// ---- Conversion to Src ------
	// 6 = Added Ladder info
	// 7 = Areas store ladder ID's so ladders can have one-way connections
	// 8 = Added earliest occupy times (2 floats) to each area
	// 9 = Promoted CNavArea's attribute flags to a short
	// 10 - Added sub-version number to allow derived classes to have custom area data
	// 11 - Added light intensity to each area
	// 12 - Storing presence of unnamed areas in the PlaceDirectory
	// 13 - Widened NavArea attribute bits from unsigned short to int
	// 14 - Added a bool for if the nav needs analysis
	// 15 - removed approach areas
	// 16 - Added visibility data to the base mesh
	fileBuffer.PutUnsignedInt( NavCurrentVersion );

	// The sub-version number is maintained and owned by classes derived from CNavMesh and CNavArea
	// and allows them to track their custom data just as we do at this top level
	fileBuffer.PutUnsignedInt( GetSubVersionNumber() );
	
	// store the size of source bsp file in the nav file
	// so we can test if the bsp changed since the nav file was made
	unsigned int bspSize = filesystem->Size( bspFilename );
	DevMsg( "Size of bsp file '%s' is %u bytes.\n", bspFilename, bspSize );

	fileBuffer.PutUnsignedInt( bspSize );

	// Store the analysis state
	fileBuffer.PutUnsignedChar( m_isAnalyzed );

	//
	// Build a directory of the Places in this map
	//
	placeDirectory.Reset();

	FOR_EACH_VEC( TheNavAreas, nit )
	{
		CNavArea *area = TheNavAreas[ nit ];

		Place place = area->GetPlace();
		placeDirectory.AddPlace( place );
	}

	placeDirectory.Save( fileBuffer );

	SaveCustomDataPreArea( fileBuffer );

	//
	// Store navigation areas
	//
	{
		// store number of areas
		unsigned int count = TheNavAreas.Count();
		fileBuffer.PutUnsignedInt( count );

		// store each area
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CNavArea *area = TheNavAreas[ it ];

			area->Save( fileBuffer, NavCurrentVersion );
		}
	}

	//
	// Store ladders
	//
	{
		// store number of ladders
		unsigned int count = m_ladders.Count();
		fileBuffer.PutUnsignedInt( count );

		// store each ladder
		for ( int i=0; i<m_ladders.Count(); ++i )
		{
			CNavLadder *ladder = m_ladders[i];
			ladder->Save( fileBuffer, NavCurrentVersion );
		}
	}
	
	//
	// Store derived class mesh info
	//
	SaveCustomData( fileBuffer );

	if ( !filesystem->WriteFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
		return false;
	}

	unsigned int navSize = filesystem->Size( filename );
	DevMsg( "Size of nav file '%s' is %u bytes.\n", filename, navSize );

	return true;
}


//--------------------------------------------------------------------------------------------------------------
static NavErrorType CheckNavFile( const char *bspFilename )
{
	if ( !bspFilename )
		return NAV_CANT_ACCESS_FILE;

	char baseName[256];
	Q_StripExtension(bspFilename,baseName,sizeof(baseName));
	char bspPathname[256];
	Q_snprintf(bspPathname,sizeof(bspPathname), FORMAT_BSPFILE, baseName);
	char filename[256];
	Q_snprintf(filename,sizeof(filename), FORMAT_NAVFILE, baseName);

	bool navIsInBsp = false;
	FileHandle_t file = filesystem->Open( filename, "rb", "MOD" );	// this ignores .nav files embedded in the .bsp ...
	if ( !file )
	{
		navIsInBsp = true;
		file = filesystem->Open( filename, "rb", "GAME" );	// ... and this looks for one if it's the only one around.
	}

	if (!file)
	{
		return NAV_CANT_ACCESS_FILE;
	}

	// check magic number
	int result;
	unsigned int magic;
	result = filesystem->Read( &magic, sizeof(unsigned int), file );
	if (!result || magic != NAV_MAGIC_NUMBER)
	{
		filesystem->Close( file );
		return NAV_INVALID_FILE;
	}

	// read file version number
	unsigned int version;
	result = filesystem->Read( &version, sizeof(unsigned int), file );
	if (!result || version > NavCurrentVersion || version < 4)
	{
		filesystem->Close( file );
		return NAV_BAD_FILE_VERSION;
	}

	// get size of source bsp file and verify that the bsp hasn't changed
	unsigned int saveBspSize;
	filesystem->Read( &saveBspSize, sizeof(unsigned int), file );

	// verify size
	unsigned int bspSize = filesystem->Size( bspPathname );

	if (bspSize != saveBspSize && !navIsInBsp)
	{
		return NAV_FILE_OUT_OF_DATE;
	}

	return NAV_OK;
}


//--------------------------------------------------------------------------------------------------------------
void CommandNavCheckFileConsistency( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	FileFindHandle_t findHandle;
	const char *bspFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
	while ( bspFilename )
	{
		switch ( CheckNavFile( bspFilename ) )
		{
		case NAV_CANT_ACCESS_FILE:
			Warning( "Missing nav file for %s\n", bspFilename );
			break;
		case NAV_INVALID_FILE:
			Warning( "Invalid nav file for %s\n", bspFilename );
			break;
		case NAV_BAD_FILE_VERSION:
			Warning( "Old nav file for %s\n", bspFilename );
			break;
		case NAV_FILE_OUT_OF_DATE:
			Warning( "The nav file for %s is built from an old version of the map\n", bspFilename );
			break;
		case NAV_OK:
			Msg( "The nav file for %s is up-to-date\n", bspFilename );
			break;
		}

		bspFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}
static ConCommand nav_check_file_consistency( "nav_check_file_consistency", CommandNavCheckFileConsistency, "Scans the maps directory and reports any missing/out-of-date navigation files.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------------
/**
 * Reads the used place names from the nav file (can be used to selectively precache before the nav is loaded)
 */
const CUtlVector< Place > *CNavMesh::GetPlacesFromNavFile( bool *hasUnnamedPlaces )
{
	placeDirectory.Reset();
	// nav filename is derived from map filename
	char filename[256];
	Q_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "GAME", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		if ( !filesystem->ReadFile( filename, "BSP", fileBuffer ) )	// ... and this looks for one if it's the only one around.
		{
			return NULL;
		}
	}
	
	if ( IsX360() )
	{
		// 360 has compressed NAVs
		CLZMA lzma;
		if ( lzma.IsCompressed( (unsigned char *)fileBuffer.Base() ) )
		{
			int originalSize = lzma.GetActualSize( (unsigned char *)fileBuffer.Base() );
			unsigned char *pOriginalData = new unsigned char[originalSize];
			lzma.Uncompress( (unsigned char *)fileBuffer.Base(), pOriginalData );
			fileBuffer.AssumeMemory( pOriginalData, originalSize, originalSize, CUtlBuffer::READ_ONLY );
		}
	}

	// check magic number
	unsigned int magic = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || magic != NAV_MAGIC_NUMBER )
	{
		return NULL;	// Corrupt nav file?
	}

	// read file version number
	unsigned int version = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || version > NavCurrentVersion )
	{
		return NULL;	// Unknown nav file version
	}

	if ( version < 5 )
	{
		return NULL;	// Too old to have place names
	}

	unsigned int subVersion = 0;
	if ( version >= 10 )
	{
		subVersion = fileBuffer.GetUnsignedInt();
		if ( !fileBuffer.IsValid() )
		{
			return NULL;	// No sub-version
		}
	}

	fileBuffer.GetUnsignedInt();	// skip BSP file size
	if ( version >= 14 )
	{
		fileBuffer.GetUnsignedChar();	// skip m_isAnalyzed
	}

	placeDirectory.Load( fileBuffer, version );

	LoadCustomDataPreArea( fileBuffer, subVersion );

	if ( hasUnnamedPlaces )
	{
		*hasUnnamedPlaces = placeDirectory.HasUnnamedPlaces();
	}

	return placeDirectory.GetPlaces();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load AI navigation data from a file
 */
NavErrorType CNavMesh::Load( void )
{
	MDLCACHE_CRITICAL_SECTION();

	// free previous navigation mesh data
	Reset();
	placeDirectory.Reset();
	CNavVectorNoEditAllocator::Reset();

	GameRules()->OnNavMeshLoad();

	CNavArea::m_nextID = 1;

	// nav filename is derived from map filename
	char filename[256];
	Q_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );

	bool navIsInBsp = false;
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		navIsInBsp = true;
		if ( !filesystem->ReadFile( filename, "BSP", fileBuffer ) )	// ... and this looks for one if it's the only one around.
		{
			return NAV_CANT_ACCESS_FILE;
		}
	}

	if ( IsX360() )
	{
		// 360 has compressed NAVs
		CLZMA lzma;
		if ( lzma.IsCompressed( (unsigned char *)fileBuffer.Base() ) )
		{
			int originalSize = lzma.GetActualSize( (unsigned char *)fileBuffer.Base() );
			unsigned char *pOriginalData = new unsigned char[originalSize];
			lzma.Uncompress( (unsigned char *)fileBuffer.Base(), pOriginalData );
			fileBuffer.AssumeMemory( pOriginalData, originalSize, originalSize, CUtlBuffer::READ_ONLY );
		}
	}

	// check magic number
	unsigned int magic = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || magic != NAV_MAGIC_NUMBER )
	{
		Msg( "Invalid navigation file '%s'.\n", filename );
		return NAV_INVALID_FILE;
	}

	// read file version number
	unsigned int version = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || version > NavCurrentVersion )
	{
		Msg( "Unknown navigation file version.\n" );
		return NAV_BAD_FILE_VERSION;
	}
	
	unsigned int subVersion = 0;
	if ( version >= 10 )
	{
		subVersion = fileBuffer.GetUnsignedInt();
		if ( !fileBuffer.IsValid() )
		{
			Msg( "Error reading sub-version number.\n" );
			return NAV_INVALID_FILE;
		}
	}

	if ( version >= 4 )
	{
		// get size of source bsp file and verify that the bsp hasn't changed
		unsigned int saveBspSize = fileBuffer.GetUnsignedInt();

		// verify size
		char *bspFilename = GetBspFilename( filename );
		if ( bspFilename == NULL )
		{
			return NAV_INVALID_FILE;
		}

		unsigned int bspSize = filesystem->Size( bspFilename );

		if ( bspSize != saveBspSize && !navIsInBsp )
		{
			if ( engine->IsDedicatedServer() )
			{
				// Warning doesn't print to the dedicated server console, so we'll use Msg instead
				DevMsg( "The Navigation Mesh was built using a different version of this map.\n" );
			}
			else
			{
				DevWarning( "The Navigation Mesh was built using a different version of this map.\n" );
			}
			m_isOutOfDate = true;
		}
	}

	if ( version >= 14 )
	{
		m_isAnalyzed = fileBuffer.GetUnsignedChar() != 0;
	}
	else
	{
		m_isAnalyzed = false;
	}

	// load Place directory
	if ( version >= 5 )
	{
		placeDirectory.Load( fileBuffer, version );
	}

	LoadCustomDataPreArea( fileBuffer, subVersion );

	// get number of areas
	unsigned int count = fileBuffer.GetUnsignedInt();
	unsigned int i;

	if ( count == 0 )
	{
		return NAV_INVALID_FILE;
	}

	Extent extent;
	extent.lo.x = 9999999999.9f;
	extent.lo.y = 9999999999.9f;
	extent.hi.x = -9999999999.9f;
	extent.hi.y = -9999999999.9f;

	// load the areas and compute total extent
	TheNavMesh->PreLoadAreas( count );
	Extent areaExtent;
	for( i=0; i<count; ++i )
	{
		CNavArea *area = TheNavMesh->CreateArea();
		area->Load( fileBuffer, version, subVersion );
		TheNavAreas.AddToTail( area );

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

	FOR_EACH_VEC( TheNavAreas, it )
	{
		AddNavArea( TheNavAreas[ it ] );
	}


	//
	// Set up all the ladders
	//
	if (version >= 6)
	{
		count = fileBuffer.GetUnsignedInt();
		m_ladders.EnsureCapacity( count );

		// load the ladders
		for( i=0; i<count; ++i )
		{
			CNavLadder *ladder = new CNavLadder;
			ladder->Load( fileBuffer, version );
			m_ladders.AddToTail( ladder );
		}
	}
	else
	{
		BuildLadders();
	}

	// mark stairways (TODO: this can be removed once all maps are re-saved with this attribute in them)
	MarkStairAreas();

	//
	// Load derived class mesh info
	//
	LoadCustomData( fileBuffer, subVersion );

	//
	// Bind pointers, etc
	//
	NavErrorType loadResult = PostLoad( version );

	WarnIfMeshNeedsAnalysis( version );

	return loadResult;
}


struct OneWayLink_t
{
	CNavArea *destArea;
	CNavArea *area;
	int backD;

	static int Compare(const OneWayLink_t *lhs, const OneWayLink_t *rhs )
	{
		int result = ( lhs->destArea - rhs->destArea );
		if ( result != 0 )
		{
			return result;
		}
		return ( lhs->backD - rhs->backD );
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked after all areas have been loaded - for pointer binding, etc
 */
NavErrorType CNavMesh::PostLoad( unsigned int version )
{
	// allow areas to connect to each other, etc
	FOR_EACH_VEC( TheNavAreas, pit )
	{
		CNavArea *area = TheNavAreas[ pit ];
		area->PostLoad();
	}

	// allow hiding spots to compute information
	FOR_EACH_VEC( TheHidingSpots, hit )
	{
		HidingSpot *spot = TheHidingSpots[ hit ];
		spot->PostLoad();
	}

	if ( version < 8 )
	{
		// Old nav meshes need to compute earliest occupy times
		FOR_EACH_VEC( TheNavAreas, nit )
		{
			CNavArea *area = TheNavAreas[ nit ];
			area->ComputeEarliestOccupyTimes();
		}
	}

	ComputeBattlefrontAreas();
	
	//
	// Allow each nav area to know what other areas have one-way connections to it. Need to gather
	// then sort due to allocation restrictions on the 360
	//


	OneWayLink_t oneWayLink;
	CUtlVectorFixedGrowable<OneWayLink_t, 512> oneWayLinks;

	FOR_EACH_VEC( TheNavAreas, oit )
	{
		oneWayLink.area = TheNavAreas[ oit ];
	
		for( int d=0; d<NUM_DIRECTIONS; d++ )
		{
			const NavConnectVector *connectList = oneWayLink.area->GetAdjacentAreas( (NavDirType)d );

			FOR_EACH_VEC( (*connectList), it )
			{
				NavConnect connect = (*connectList)[ it ];
				oneWayLink.destArea = connect.area;
			
				// if the area we connect to has no connection back to us, allow that area to remember us as an incoming connection
				oneWayLink.backD = OppositeDirection( (NavDirType)d );		
				const NavConnectVector *backConnectList = oneWayLink.destArea->GetAdjacentAreas( (NavDirType)oneWayLink.backD );
				bool isOneWay = true;
				FOR_EACH_VEC( (*backConnectList), bit )
				{
					NavConnect backConnect = (*backConnectList)[ bit ];
					if (backConnect.area->GetID() == oneWayLink.area->GetID())
					{
						isOneWay = false;
						break;
					}
				}
				
				if (isOneWay)
				{
					oneWayLinks.AddToTail( oneWayLink );
				}
			}
		}
	}

	oneWayLinks.Sort( &OneWayLink_t::Compare );

	for ( int i = 0; i < oneWayLinks.Count(); i++ )
	{
		// add this one-way connection
		oneWayLinks[i].destArea->AddIncomingConnection( oneWayLinks[i].area, (NavDirType)oneWayLinks[i].backD );	
	}

	ValidateNavAreaConnections();

	// TERROR: loading into a map directly creates entities before the mesh is loaded.  Tell the preexisting
	// entities now that the mesh is loaded so they can update areas.
	for ( int i=0; i<m_avoidanceObstacles.Count(); ++i )
	{
		m_avoidanceObstacles[i]->OnNavMeshLoaded();
	}

	// the Navigation Mesh has been successfully loaded
	m_isLoaded = true;
	
	return NAV_OK;
}
