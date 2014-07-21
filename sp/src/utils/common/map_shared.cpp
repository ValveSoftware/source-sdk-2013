//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "map_shared.h"
#include "bsplib.h"
#include "cmdlib.h"


CMapError g_MapError;
int g_nMapFileVersion;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pLoadEntity - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadEntityKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity)
{
	if (!stricmp(szKey, "classname"))
	{
		if (!stricmp(szValue, "func_detail"))
		{
			pLoadEntity->nBaseContents = CONTENTS_DETAIL;
		}
		else if (!stricmp(szValue, "func_ladder"))
		{
			pLoadEntity->nBaseContents = CONTENTS_LADDER;
		}
		else if (!stricmp(szValue, "func_water"))
		{
			pLoadEntity->nBaseContents = CONTENTS_WATER;
		}
	}
	else if (!stricmp(szKey, "id"))
	{
		// UNDONE: flag entity errors by ID instead of index
		//g_MapError.EntityState( atoi( szValue ) );
		// rename this field since DME code uses this name
		SetKeyValue( pLoadEntity->pEntity, "hammerid", szValue );
		return(ChunkFile_Ok);
	}
	else if( !stricmp( szKey, "mapversion" ) )
	{
		// .vmf map revision number
		g_MapRevision = atoi( szValue );
		SetKeyValue( pLoadEntity->pEntity, szKey, szValue );
		return ( ChunkFile_Ok );
	}

	SetKeyValue( pLoadEntity->pEntity, szKey, szValue );

	return(ChunkFile_Ok);
}


static ChunkFileResult_t LoadEntityCallback( CChunkFile *pFile, int nParam )
{
	if (num_entities == MAX_MAP_ENTITIES)
	{
		// Exits.
		g_MapError.ReportError ("num_entities == MAX_MAP_ENTITIES");
	}

	entity_t *mapent = &entities[num_entities];
	num_entities++;
	memset(mapent, 0, sizeof(*mapent));
	mapent->numbrushes = 0;

	LoadEntity_t LoadEntity;
	LoadEntity.pEntity = mapent;

	// No default flags/contents
	LoadEntity.nBaseFlags = 0;
	LoadEntity.nBaseContents = 0;

	//
	// Read the entity chunk.
	//
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadEntityKeyCallback, &LoadEntity);

	return eResult;
}


bool LoadEntsFromMapFile( char const *pFilename )
{
	//
	// Dummy this up for the texture handling. This can be removed when old .MAP file
	// support is removed.
	//
	g_nMapFileVersion = 400;

	//
	// Open the file.
	//
	CChunkFile File;
	ChunkFileResult_t eResult = File.Open( pFilename, ChunkFile_Read );

	if(eResult == ChunkFile_Ok)
	{
		num_entities = 0;

		//
		// Set up handlers for the subchunks that we are interested in.
		//
		CChunkHandlerMap Handlers;
		Handlers.AddHandler("entity", (ChunkHandler_t)LoadEntityCallback, 0);

		File.PushHandlers(&Handlers);

		//
		// Read the sub-chunks. We ignore keys in the root of the file.
		//
		while (eResult == ChunkFile_Ok)
		{
			eResult = File.ReadChunk();
		}

		File.PopHandlers();
		return true;
	}
	else
	{
		Error("Error in LoadEntsFromMapFile (in-memory file): %s.\n", File.GetErrorText(eResult));
		return false;
	}
}


