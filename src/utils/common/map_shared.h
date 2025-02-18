//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MAP_SHARED_H
#define MAP_SHARED_H
#ifdef _WIN32
#pragma once
#endif


#include "ChunkFile.h"
#include "bsplib.h"
#include "cmdlib.h"


struct LoadEntity_t
{
	entity_t *pEntity;
	int nID;
	int nBaseFlags;
	int nBaseContents;
};


class CMapError
{
public:

	void BrushState( int brushID )
	{
		m_brushID = brushID;
	}

	void BrushSide( int side )
	{
		m_sideIndex = side;
	}

	void TextureState( const char *pTextureName )
	{
		Q_strncpy( m_textureName, pTextureName, sizeof( m_textureName ) );
	}

	void ClearState( void )
	{
		BrushState( 0 );
		BrushSide( 0 );
		TextureState( "Not a Parse error!" );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook the map parse errors and report brush/ent/texture state
	// Input  : *pErrorString - 
	//-----------------------------------------------------------------------------
	void ReportError( const char *pErrorString )
	{
		Error( "Brush %i: %s\nSide %i\nTexture: %s\n", m_brushID, pErrorString, m_sideIndex, m_textureName );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook the map parse errors and report brush/ent/texture state without exiting.
	// Input  : pWarningString - 
	//-----------------------------------------------------------------------------
	void ReportWarning( const char *pWarningString )
	{
		printf( "Brush %i, Side %i: %s\n", m_brushID, m_sideIndex, pWarningString );
	}

private:

	int		m_brushID;
	int		m_sideIndex;
	char	m_textureName[80];
};


extern CMapError g_MapError;
extern int g_nMapFileVersion;


// Shared mapload code.
ChunkFileResult_t LoadEntityKeyCallback( const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity );

// Used by VRAD incremental lighting - only load ents from the file and
// fill in the global entities/num_entities array.
bool LoadEntsFromMapFile( char const *pFilename );

#endif // MAP_SHARED_H
