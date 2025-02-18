//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file loads a KeyValues file containing material name mappings.
//			When the bsp is compiled, all materials listed in the file will
//			be replaced by the second material in the pair.
//
//=============================================================================

#include "vbsp.h"
#include "materialsub.h"
#include "KeyValues.h"
#include "tier1/strtools.h"

bool g_ReplaceMaterials	= false;

static KeyValues *kv			= 0;
static KeyValues *allMapKeys	= 0;
static KeyValues *curMapKeys	= 0;

//-----------------------------------------------------------------------------
// Purpose: Loads the KeyValues file for materials replacements
//-----------------------------------------------------------------------------
void LoadMaterialReplacementKeys( const char *gamedir, const char *mapname )
{
	// Careful with static variables
	if( kv )
	{
		kv->deleteThis();
		kv = 0;
	}
	if( allMapKeys )
		allMapKeys = 0;
	if( curMapKeys )
		curMapKeys = 0;

	Msg( "Loading Replacement Keys\n" );

	// Attach the path to the keyValues file
	char path[1024];
	Q_snprintf( path, sizeof( path ), "%scfg\\materialsub.cfg", gamedir );
	
	// Load the keyvalues file
	kv = new KeyValues( "MaterialReplacements" );

	Msg( "File path: %s", path );
	if( !kv->LoadFromFile( g_pFileSystem, path ) )
	{
		Msg( "Failed to load KeyValues file!\n" );
		g_ReplaceMaterials = false;
		kv->deleteThis();
		kv = 0;
		return;
	}

	// Load global replace keys
	allMapKeys = kv->FindKey( "AllMaps", true );

	// Load keys for the current map
	curMapKeys = kv->FindKey( mapname );

	allMapKeys->ChainKeyValue( curMapKeys );
}

//-----------------------------------------------------------------------------
// Purpose: Deletes all keys
//-----------------------------------------------------------------------------
void DeleteMaterialReplacementKeys( void )
{
	if( kv )
	{
		kv->deleteThis();
		kv = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Replace the passed-in material name with a replacement name, if one exists
//-----------------------------------------------------------------------------
const char* ReplaceMaterialName( const char *name )
{
	// Look for the material name in the global and map KeyValues
	// If it's not there, just return the original name

	// HACK: This stinks - KeyValues won't take a string with '/' in it.
	// If they did, this could be a simple pointer swap.
	char newName[1024];
	Q_strncpy( newName, name, sizeof( newName ) );
	Q_FixSlashes( newName );
	return allMapKeys->GetString( newName, name );
}