// nav_merge.cpp
// Save/merge for partial nav meshes
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "fmtstr.h"
#include "tier0/vprof.h"
#include "utldict.h"

#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------
void CNavArea::SaveToSelectedSet( KeyValues *areaKey ) const
{
	const char *placeName = TheNavMesh->PlaceToName( GetPlace() );
	areaKey->SetString( "Place", (placeName)?placeName:"" );

	areaKey->SetInt( "Attributes", GetAttributes() );
}


//--------------------------------------------------------------------------------------------------------
void CNavArea::RestoreFromSelectedSet( KeyValues *areaKey )
{
	SetPlace( TheNavMesh->NameToPlace( areaKey->GetString( "Place" ) ) );

	SetAttributes( areaKey->GetInt( "Attributes" ) );
}


//--------------------------------------------------------------------------------------------------------
class BuildSelectedSet
{
public:
	BuildSelectedSet( KeyValues *kv )
	{
		m_kv = kv;
		m_areaCount = 0;
	}

	bool operator() ( CNavArea *area )
	{
		CFmtStrN<32> name( "%d", area->GetID() );
		KeyValues *areaKey = m_kv->FindKey( name.Access(), true );
		if ( areaKey )
		{
			++m_areaCount;

			WriteCorner( area, areaKey, NORTH_WEST, "NorthWest" );
			WriteCorner( area, areaKey, NORTH_EAST, "NorthEast" );
			WriteCorner( area, areaKey, SOUTH_WEST, "SouthWest" );
			WriteCorner( area, areaKey, SOUTH_EAST, "SouthEast" );

			WriteConnections( area, areaKey, NORTH, "North" );
			WriteConnections( area, areaKey, SOUTH, "South" );
			WriteConnections( area, areaKey, EAST, "East" );
			WriteConnections( area, areaKey, WEST, "West" );

			area->SaveToSelectedSet( areaKey );
		}
		return true;
	}

	int Count( void ) const
	{
		return m_areaCount;
	}

private:
	void WriteCorner( CNavArea *area, KeyValues *areaKey, NavCornerType corner, const char *cornerName )
	{
		KeyValues *cornerKey = areaKey->FindKey( cornerName, true );
		if ( cornerKey )
		{
			Vector pos = area->GetCorner( corner );
			cornerKey->SetFloat( "x", pos.x );
			cornerKey->SetFloat( "y", pos.y );
			cornerKey->SetFloat( "z", pos.z );
		}
	}

	void WriteConnections( CNavArea *area, KeyValues *areaKey, NavDirType dir, const char *dirName )
	{
		KeyValues *dirKey = areaKey->FindKey( dirName, true );
		if ( dirKey )
		{
			for ( int i=0; i<area->GetAdjacentCount( dir ); ++i )
			{
				CNavArea *other = area->GetAdjacentArea( dir, i );
				if ( other && TheNavMesh->IsInSelectedSet( other ) )
				{
					CFmtStrN<32> name( "%d", i );
					dirKey->SetInt( name.Access(), other->GetID() );
				}
			}
		}
	}

	int m_areaCount;
	KeyValues *m_kv;
};


//--------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavSaveSelected( const CCommand &args )
{
	KeyValues *data = new KeyValues( "Selected Nav Areas" );
	data->SetInt( "version", 1 );

	BuildSelectedSet setBuilder( data );
	TheNavMesh->ForAllSelectedAreas( setBuilder );

	if ( !setBuilder.Count() )
	{
		Msg( "Not saving empty selected set to disk.\n" );
		data->deleteThis();
		return;
	}

	char fname[32];
	char path[MAX_PATH];
	if ( args.ArgC() == 2 )
	{
		V_FileBase( args[0], fname, sizeof( fname ) );
	}
	else
	{
		V_strncpy( fname, STRING( gpGlobals->mapname ), sizeof( fname ) );
	}

	int i;
	for ( i=0; i<1000; ++i )
	{
		V_snprintf( path, sizeof( path ), "maps/%s_selected_%4.4d.txt", fname, i );
		if ( !filesystem->FileExists( path ) )
		{
			break;
		}
	}

	if ( i == 1000 )
	{
		Msg( "Unable to find a filename to save the selected set to disk.\n" );
		data->deleteThis();
		return;
	}

	if ( !data->SaveToFile( filesystem, path ) )
	{
		Msg( "Unable to save the selected set to disk.\n" );
	}

	Msg( "Selected set saved to %s.  Use 'nav_merge_mesh %s_selected_%4.4d' to merge it into another mesh.\n", path, fname, i );
	data->deleteThis();
}


//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F( nav_save_selected, "Writes the selected set to disk for merging into another mesh via nav_merge_mesh.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavSaveSelected( args );
}


//--------------------------------------------------------------------------------------------------------
Vector ReadCorner( KeyValues *areaKey, const char *cornerName )
{
	Vector pos( vec3_origin );
	KeyValues *cornerKey = areaKey->FindKey( cornerName, false );
	if ( cornerKey )
	{
		pos.x = cornerKey->GetFloat( "x" );
		pos.y = cornerKey->GetFloat( "y" );
		pos.z = cornerKey->GetFloat( "z" );
	}

	return pos;
}


//--------------------------------------------------------------------------------------------------------
void ReconnectMergedArea( CUtlDict< CNavArea *, int > &newAreas, KeyValues *areaKey, NavDirType dir, const char *dirName )
{
	int index = newAreas.Find( areaKey->GetName() );
	if ( index == newAreas.InvalidIndex() )
	{
		Assert( false );
		return;
	}

	CNavArea *area = newAreas[index];

	KeyValues *dirKey = areaKey->FindKey( dirName, true );
	if ( dirKey )
	{
		KeyValues *connection = dirKey->GetFirstValue();
		while ( connection )
		{
			const char *otherID = connection->GetString();
			int otherIndex = newAreas.Find( otherID );
			Assert( otherIndex != newAreas.InvalidIndex() );
			if ( otherIndex != newAreas.InvalidIndex() )
			{
				CNavArea *other = newAreas[otherIndex];

				area->ConnectTo( other, dir );	// only a 1-way connection.  the other area will connect back to us.
			}

			connection = connection->GetNextValue();
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavMesh::CommandNavMergeMesh( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: nav_merge_mesh filename\n" );
		return;
	}

	char fname[64];
	char path[MAX_PATH];
	V_FileBase( args[1], fname, sizeof( fname ) );
	V_snprintf( path, sizeof( path ), "maps/%s.txt", fname );

	KeyValues *data = new KeyValues( "Nav Selected Set" );
	if ( !data->LoadFromFile( filesystem, path ) )
	{
		Msg( "Unable to load %s.\n", path );
	}
	else
	{
		// Loaded the data - plug it into the existing mesh!

		// First add the areas, and put them in the correct places.  We can save off the new area ID
		// at the same time.
		CUtlDict< CNavArea *, int > newAreas;
		CUtlVector< CNavArea * > areaVector;
		KeyValues *areaKey = data->GetFirstSubKey();
		while ( areaKey )
		{
			Vector northWest = ReadCorner( areaKey, "NorthWest" );
			Vector northEast = ReadCorner( areaKey, "NorthEast" );
			Vector southWest = ReadCorner( areaKey, "SouthWest" );
			Vector southEast = ReadCorner( areaKey, "SouthEast" );

			CNavArea *newArea = TheNavMesh->CreateArea();
			if (newArea == NULL)
			{
				Warning( "nav_merge_mesh: Out of memory\n" );
				return;
			}

			newArea->Build( northWest, northEast, southEast, southWest );
			TheNavAreas.AddToTail( newArea );
			TheNavMesh->AddNavArea( newArea );
			areaVector.AddToTail( newArea );

			// save the new ID for connections
			int index = newAreas.Find( areaKey->GetName() );
			Assert( index == newAreas.InvalidIndex() );
			if ( index == newAreas.InvalidIndex() )
			{
				newAreas.Insert( areaKey->GetName(), newArea );
			}

			// Restore additional data
			newArea->RestoreFromSelectedSet( areaKey );

			areaKey = areaKey->GetNextKey();
		}

		// Go back and reconnect the new areas to each other
		areaKey = data->GetFirstSubKey();
		while ( areaKey )
		{
			ReconnectMergedArea( newAreas, areaKey, NORTH, "North" );
			ReconnectMergedArea( newAreas, areaKey, SOUTH, "South" );
			ReconnectMergedArea( newAreas, areaKey, EAST, "East" );
			ReconnectMergedArea( newAreas, areaKey, WEST, "West" );

			areaKey = areaKey->GetNextKey();
		}

		// Connect selected areas with pre-existing areas
		StitchAreaSet( &areaVector );
	}

	data->deleteThis();
}


//--------------------------------------------------------------------------------------------------------
int NavMeshMergeAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	char *commandName = "nav_merge_mesh";
	int numMatches = 0;
	partial += Q_strlen( commandName ) + 1;
	int partialLength = Q_strlen( partial );

	FileFindHandle_t findHandle;
	char txtFilenameNoExtension[ MAX_PATH ];
	const char *txtFilename = filesystem->FindFirstEx( "maps/*_selected_*.txt", "MOD", &findHandle );
	while ( txtFilename )
	{
		Q_FileBase( txtFilename, txtFilenameNoExtension, sizeof( txtFilenameNoExtension ) );
		if ( !Q_strnicmp( txtFilenameNoExtension, partial, partialLength ) && V_stristr( txtFilenameNoExtension, "_selected_" ) )
		{
			// Add the place name to the autocomplete array
			Q_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", commandName, txtFilenameNoExtension );

			// Make sure we don't try to return too many place names
			if ( numMatches == COMMAND_COMPLETION_MAXITEMS )
				return numMatches;
		}

		txtFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );

	return numMatches;
}


//--------------------------------------------------------------------------------------------------------
CON_COMMAND_F_COMPLETION( nav_merge_mesh, "Merges a saved selected set into the current mesh.", FCVAR_GAMEDLL | FCVAR_CHEAT, NavMeshMergeAutocomplete )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNavMesh->CommandNavMergeMesh( args );
}


//--------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------------
