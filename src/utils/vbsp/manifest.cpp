//========= Copyright Valve Corporation, All rights reserved. ============//
#include "vbsp.h"
#include "map_shared.h"
#include "fgdlib/fgdlib.h"
#include "manifest.h"
#include "windows.h"

//-----------------------------------------------------------------------------
// Purpose: default constructor
//-----------------------------------------------------------------------------
CManifestMap::CManifestMap( void )
{
	m_RelativeMapFileName[ 0 ] = 0;
	m_bTopLevelMap = false;
}


//-----------------------------------------------------------------------------
// Purpose: default constructor
//-----------------------------------------------------------------------------
CManifest::CManifest( void ) 
{ 
	m_InstancePath[ 0 ] = 0;
	m_bIsCordoning = false;
	m_CordoningMapEnt = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: this function will parse through the known keys for the manifest map entry
// Input  : szKey - the key name
//			szValue - the value
//			pManifestMap - the manifest map this belongs to
// Output : ChunkFileResult_t - result of the parsing
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadManifestMapKeyCallback( const char *szKey, const char *szValue, CManifestMap *pManifestMap )
{
	if ( !stricmp( szKey, "Name" ) )
	{
		//		pManifestMap->m_FriendlyName = szValue;
	}
	else if ( !stricmp( szKey, "File" ) )
	{
		strcpy( pManifestMap->m_RelativeMapFileName, szValue );
	}
	else if ( !stricmp( szKey, "IsPrimary" ) )
	{
		//		pManifestMap->m_bPrimaryMap = ( atoi( szValue ) == 1 );
	}
	else if ( !stricmp( szKey, "IsProtected" ) )
	{
		//		pManifestMap->m_bCanBeModified = ( atoi( szValue ) != 1 );
	}
	else if ( !stricmp( szKey, "TopLevel" ) )
	{
		pManifestMap->m_bTopLevelMap = ( atoi( szValue ) == 1 );
	}

	return ChunkFile_Ok;
}


//-----------------------------------------------------------------------------
// Purpose: this function is responsible for setting up the manifest map about to be read in
// Input  : pFile - the chunk file being read
//			pDoc - the owning manifest document
// Output : ChunkFileResult_t - result of the parsing
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadManifestVMFCallback( CChunkFile *pFile, CManifest *pManifest )
{
	CManifestMap	*pManifestMap = new CManifestMap();

	pManifest->m_Maps.AddToTail( pManifestMap );

	ChunkFileResult_t eResult = pFile->ReadChunk( ( KeyHandler_t )LoadManifestMapKeyCallback, pManifestMap );

	return( eResult );
}


//-----------------------------------------------------------------------------
// Purpose: this function will load the VMF chunk
// Input  : pFile - the chunk file being read
//			pDoc - the owning manifest document
// Output : ChunkFileResult_t - result of the parsing
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadManifestMapsCallback( CChunkFile *pFile, CManifest *pManifest )
{
	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "VMF", ( ChunkHandler_t )LoadManifestVMFCallback, pManifest );
	pFile->PushHandlers(&Handlers);

	ChunkFileResult_t eResult = ChunkFile_Ok;

	eResult = pFile->ReadChunk();

	pFile->PopHandlers();

	return( eResult );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonBoxCallback( CChunkFile *pFile, Cordon_t *pCordon )
{
	// Add a box to this cordon.
	pCordon->m_Boxes.AddToTail();
	BoundBox &box = pCordon->m_Boxes.Tail();

	// Fill it in with the data from the VMF.
	return pFile->ReadChunk( (KeyHandler_t)LoadCordonBoxKeyCallback, (void *)&box );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonBoxKeyCallback( const char *szKey, const char *szValue, BoundBox *pBox )
{
	if (!stricmp(szKey, "mins"))
	{
		CChunkFile::ReadKeyValuePoint(szValue, pBox->bmins);
	}
	else if (!stricmp(szKey, "maxs"))
	{
		CChunkFile::ReadKeyValuePoint(szValue, pBox->bmaxs);
	}

	return ChunkFile_Ok;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonKeyCallback( const char *szKey, const char *szValue, Cordon_t *pCordon )
{
	if (!stricmp(szKey, "name"))
	{
		pCordon->m_szName.Set( szValue );
	}
	// Whether this particular cordon volume is active.
	else if (!stricmp(szKey, "active"))
	{
		CChunkFile::ReadKeyValueBool(szValue, pCordon->m_bActive);
	}

	return ChunkFile_Ok;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonCallback( CChunkFile *pFile, CManifest *pManifest )
{
	// Add a new cordon which will be filled in by the key callback
	pManifest->m_Cordons.AddToTail();
	Cordon_t &cordon = pManifest->m_Cordons.Tail();

	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "box", (ChunkHandler_t)CManifest::LoadCordonBoxCallback, (void *)&cordon );

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk( (KeyHandler_t)LoadCordonKeyCallback, (void *)&cordon );
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------------------------------------
// Parses keys that are applicable to all cordons in the map.
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonsKeyCallback( const char *szKey, const char *szValue, CManifest *pManifest )
{
	// Whether the cordoning system is enabled or disabled.
	if ( !stricmp( szKey, "active" ) )
	{
		CChunkFile::ReadKeyValueBool( szValue, pManifest->m_bIsCordoning );
	}

	return ChunkFile_Ok;
}


//-----------------------------------------------------------------------------
// Parses the VMF chunk that pertains to all the cordons in the map:
//
//		cordons
//		{
//			"active" "true"
//			cordon
//			{
//				"active" "true"
//				"box"
//				{
//					"mins" "-1024, -1024, -1024"
//					"maxs" "1024, 1024, 1024"
//				}
//				...may be more boxes...
//			}
//			...may be more cordons...
//		}
//
//-----------------------------------------------------------------------------
ChunkFileResult_t CManifest::LoadCordonsCallback( CChunkFile *pFile, CManifest *pManifest )
{
	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "cordon", (ChunkHandler_t)CManifest::LoadCordonCallback, pManifest );

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk( (KeyHandler_t)LoadCordonsKeyCallback, pManifest );
	pFile->PopHandlers();

	return(eResult);
}

extern ChunkFileResult_t LoadSolidCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity);

ChunkFileResult_t CManifest::LoadManifestCordoningPrefsCallback( CChunkFile *pFile, CManifest *pDoc )
{
	pDoc->m_CordoningMapEnt = &g_MainMap->entities[g_MainMap->num_entities];
	g_MainMap->num_entities++;
	memset( pDoc->m_CordoningMapEnt, 0, sizeof( *pDoc->m_CordoningMapEnt ) );
	pDoc->m_CordoningMapEnt->firstbrush = g_MainMap->nummapbrushes;
	pDoc->m_CordoningMapEnt->numbrushes = 0;

	LoadEntity_t LoadEntity;
	LoadEntity.pEntity = pDoc->m_CordoningMapEnt;

	// No default flags/contents
	LoadEntity.nBaseFlags = 0;
	LoadEntity.nBaseContents = 0;

	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "cordons", ( ChunkHandler_t )CManifest::LoadCordonsCallback, pDoc );
	Handlers.AddHandler("solid", (ChunkHandler_t)::LoadSolidCallback, &LoadEntity);
	pFile->PushHandlers(&Handlers);

	ChunkFileResult_t eResult = ChunkFile_Ok;

	eResult = pFile->ReadChunk();

	pFile->PopHandlers();

	return( eResult );
}


//-----------------------------------------------------------------------------
// Purpose: this function will create a new entity pair
// Input  : pKey - the key of the pair
//			pValue - the value of the pair
// Output : returns a newly created epair structure
//-----------------------------------------------------------------------------
epair_t *CManifest::CreateEPair( char *pKey, char *pValue )
{
	epair_t *pEPair = new epair_t;

	pEPair->key = new char[ strlen( pKey ) + 1 ];
	pEPair->value = new char[ strlen( pValue ) + 1 ];

	strcpy( pEPair->key, pKey );
	strcpy( pEPair->value, pValue );

	return pEPair;
}


//-----------------------------------------------------------------------------
// Purpose: this function will load in all of the submaps belonging to this manifest, 
//			except for the top level map, which is loaded separately.
// Input  : pMapFile - the top level map that was previously loaded
//			pszFileName - the absolute file name of the top level map file
// Output : returns true if all submaps were loaded
//-----------------------------------------------------------------------------
bool CManifest::LoadSubMaps( CMapFile *pMapFile, const char *pszFileName )
{
	entity_t	*InstanceEntity;
	epair_t		*pEPair;

	InstanceEntity = &pMapFile->entities[ pMapFile->num_entities ];
	pMapFile->num_entities++;
	memset( InstanceEntity, 0, sizeof( *InstanceEntity ) );

	InstanceEntity->origin.Init( 0.0f, 0.0f, 0.0f );
	pEPair = CreateEPair( "classname", "worldspawn" );
	pEPair->next = InstanceEntity->epairs;
	InstanceEntity->epairs = pEPair;

	for( int i = 0; i < m_Maps.Count(); i++ )
	{
		//		if ( m_Maps[ i ]->m_bTopLevelMap == false )
		{
			char		FileName[ MAX_PATH ];

			sprintf( FileName, "%s%s", m_InstancePath, m_Maps[ i ]->m_RelativeMapFileName );

			InstanceEntity = &pMapFile->entities[ pMapFile->num_entities ];
			pMapFile->num_entities++;

			memset( InstanceEntity, 0, sizeof( *InstanceEntity ) );
			InstanceEntity->origin.Init( 0.0f, 0.0f, 0.0f );

			pEPair = CreateEPair( "angles", "0 0 0" );
			pEPair->next = InstanceEntity->epairs;
			InstanceEntity->epairs = pEPair;

			char temp[ 128 ];
			sprintf( temp, "%d", GameData::NAME_FIXUP_NONE );

			pEPair = CreateEPair( "fixup_style", temp );
			pEPair->next = InstanceEntity->epairs;
			InstanceEntity->epairs = pEPair;

			pEPair = CreateEPair( "classname", "func_instance" );
			pEPair->next = InstanceEntity->epairs;
			InstanceEntity->epairs = pEPair;

			pEPair = CreateEPair( "file", m_Maps[ i ]->m_RelativeMapFileName );
			pEPair->next = InstanceEntity->epairs;
			InstanceEntity->epairs = pEPair;

			if ( m_Maps[ i ]->m_bTopLevelMap == true )
			{
				pEPair = CreateEPair( "toplevel", "1" );
				pEPair->next = InstanceEntity->epairs;
				InstanceEntity->epairs = pEPair;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
bool CManifest::LoadVMFManifestUserPrefs( const char *pszFileName )
{
	char		UserName[ MAX_PATH ], FileName[ MAX_PATH ], UserPrefsFileName[ MAX_PATH ];
	DWORD		UserNameSize;

	UserNameSize = sizeof( UserName );
	if ( GetUserName( UserName, &UserNameSize ) == 0 )
	{
		strcpy( UserPrefsFileName, "default" );
	}

	sprintf( UserPrefsFileName, "\\%s.vmm_prefs", UserName );
	V_StripExtension( pszFileName, FileName, sizeof( FileName ) );
	strcat( FileName, UserPrefsFileName );

	FILE *fp = fopen( FileName, "rb" );
	if ( !fp )
	{
		return false;
	}

	CChunkFile File;
	ChunkFileResult_t eResult = File.Open( FileName, ChunkFile_Read );

	if ( eResult == ChunkFile_Ok )
	{
		//
		// Set up handlers for the subchunks that we are interested in.
		//
		CChunkHandlerMap Handlers;
		Handlers.AddHandler( "cordoning", ( ChunkHandler_t )CManifest::LoadManifestCordoningPrefsCallback, this );

		//		Handlers.SetErrorHandler( ( ChunkErrorHandler_t )CMapDoc::HandleLoadError, this);

		File.PushHandlers(&Handlers);

		while( eResult == ChunkFile_Ok )
		{
			eResult = File.ReadChunk();
		}

		if ( eResult == ChunkFile_EOF )
		{
			eResult = ChunkFile_Ok;
		}

		File.PopHandlers();
	}

	if ( eResult == ChunkFile_Ok )
	{
	}
	else
	{
		// no pref message for now
		//		GetMainWnd()->MessageBox( File.GetErrorText( eResult ), "Error loading manifest!", MB_OK | MB_ICONEXCLAMATION );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Loads a .VMM file.
// Input  : pszFileName - Full path of the map file to load.
//-----------------------------------------------------------------------------
bool CManifest::LoadVMFManifest( const char *pszFileName )
{
	V_StripExtension( pszFileName, m_InstancePath, sizeof( m_InstancePath ) );
	strcat( m_InstancePath, "\\" );

	CChunkFile File;
	ChunkFileResult_t eResult = File.Open( pszFileName, ChunkFile_Read );
	if ( eResult != ChunkFile_Ok )
	{
		g_MapError.ReportError( File.GetErrorText( eResult ) );
		return false;
	}

	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "Maps", ( ChunkHandler_t )LoadManifestMapsCallback, this );

	File.PushHandlers(&Handlers);

	while (eResult == ChunkFile_Ok)
	{
		eResult = File.ReadChunk();
	}

	if (eResult == ChunkFile_EOF)
	{
		eResult = ChunkFile_Ok;
	}

	File.PopHandlers();

	if ( eResult == ChunkFile_Ok )
	{
		int index = g_Maps.AddToTail( new CMapFile() );
		g_LoadingMap = g_Maps[ index ];
		if ( g_MainMap == NULL )
		{
			g_MainMap = g_LoadingMap;
		}

		LoadSubMaps( g_LoadingMap, pszFileName );

		LoadVMFManifestUserPrefs( pszFileName );
	}

	return ( eResult == ChunkFile_Ok );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CManifest::CordonWorld( )
{
	if ( m_bIsCordoning == false )
	{
		return;
	}

	for ( int i = 0; i < g_MainMap->num_entities; i++ )
	{
		if ( i == 0 )
		{	// for world spawn, we look at brushes
			for( int nBrushNum = 0; nBrushNum < g_MainMap->entities[ i ].numbrushes; nBrushNum++ )
			{
				int nIndex = g_MainMap->entities[ i ].firstbrush + nBrushNum;

				bool bRemove = true;

				for( int nCordon = 0; nCordon < m_Cordons.Count(); nCordon++ )
				{
					if ( m_Cordons[ nCordon ].m_bActive == false )
					{
						continue;
					}

					for( int nBox = 0; nBox < m_Cordons[ nCordon ].m_Boxes.Count(); nBox++ )
					{
						if ( m_Cordons[ nCordon ].m_Boxes[ nBox ].IsIntersectingBox( g_MainMap->mapbrushes[ nIndex ].mins, g_MainMap->mapbrushes[ nIndex ].maxs ) == true )
						{
							bRemove = false;
							break;
						}
					}

					if ( bRemove == false )
					{
						break;
					}
				}

				if ( bRemove )
				{
					int nSize = ( g_MainMap->entities[ i ].numbrushes - nBrushNum - 1 ) * sizeof( g_MainMap->mapbrushes[ 0 ] );
					memmove( &g_MainMap->mapbrushes[ nIndex ], &g_MainMap->mapbrushes[ nIndex + 1 ], nSize );
					g_MainMap->entities[ i ].numbrushes--;
					nBrushNum--;
				}
			}
		}
		else if ( &g_MainMap->entities[ i ] != m_CordoningMapEnt )
		{	// for all other entities, even if they include brushes, we look at origin
			if ( g_MainMap->entities[ i ].numbrushes == 0 && g_MainMap->entities[ i ].epairs == NULL )
			{
				continue;
			}

			bool bRemove = true;

			for( int nCordon = 0; nCordon < m_Cordons.Count(); nCordon++ )
			{
				if ( m_Cordons[ nCordon ].m_bActive == false )
				{
					continue;
				}

				for( int nBox = 0; nBox < m_Cordons[ nCordon ].m_Boxes.Count(); nBox++ )
				{
					if ( m_Cordons[ nCordon ].m_Boxes[ nBox ].ContainsPoint( g_MainMap->entities[ i ].origin ) == true )
					{
						bRemove = false;
						break;
					}
				}

				if ( bRemove == false )
				{
					break;
				}
			}

			if ( bRemove )
			{
				g_MainMap->entities[ i ].numbrushes = 0;
				g_MainMap->entities[ i ].epairs = NULL;
			}
		}
	}

	if ( m_CordoningMapEnt )
	{
		g_MainMap->MoveBrushesToWorldGeneral( m_CordoningMapEnt );
		m_CordoningMapEnt->numbrushes = 0;
		m_CordoningMapEnt->epairs = NULL;
	}
}
