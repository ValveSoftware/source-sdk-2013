//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "incremental.h"
#include "lightmap.h"



static bool g_bFileError = false;


// -------------------------------------------------------------------------------- //
// Static helpers.
// -------------------------------------------------------------------------------- //

static bool CompareLights( dworldlight_t *a, dworldlight_t *b )
{
	static float flEpsilon = 1e-7;

	bool a1  = VectorsAreEqual( a->origin,    b->origin,    flEpsilon );
	bool a2  = VectorsAreEqual( a->intensity, b->intensity, 1.1f ); // intensities are huge numbers
	bool a3  = VectorsAreEqual( a->normal,    b->normal,    flEpsilon );
	bool a4  = fabs( a->constant_attn - b->constant_attn ) < flEpsilon;
	bool a5  = fabs( a->linear_attn - b->linear_attn ) < flEpsilon;
	bool a6  = fabs( a->quadratic_attn - b->quadratic_attn ) < flEpsilon;
	bool a7  = fabs( float( a->flags - b->flags ) ) < flEpsilon;
	bool a8  = fabs( a->stopdot - b->stopdot ) < flEpsilon;
	bool a9  = fabs( a->stopdot2 - b->stopdot2 ) < flEpsilon;
	bool a10 = fabs( a->exponent - b->exponent ) < flEpsilon;
	bool a11 = fabs( a->radius - b->radius ) < flEpsilon;

	return a1 && a2 && a3 && a4 && a5 && a6 && a7 && a8 && a9 && a10 && a11;
}


long FileOpen( char const *pFilename, bool bRead )
{
	g_bFileError = false;
	return (long)g_pFileSystem->Open( pFilename, bRead ? "rb" : "wb" );
}


void FileClose( long fp )
{
	if( fp )
		g_pFileSystem->Close( (FILE*)fp );
}


// Returns true if there was an error reading from the file.
bool FileError()
{
	return g_bFileError;
}

static inline void FileRead( long fp, void *pOut, int size )
{
	if( g_bFileError || g_pFileSystem->Read( pOut, size, (FileHandle_t)fp ) != size )
	{
		g_bFileError = true;
		memset( pOut, 0, size );
	}
}


template<class T>
static inline void FileRead( long fp, T &out )
{
	FileRead( fp, &out, sizeof(out) );
}


static inline void FileWrite( long fp, void const *pData, int size )
{
	if( g_bFileError || g_pFileSystem->Write( pData, size, (FileHandle_t)fp ) != size )
	{
		g_bFileError = true;
	}
}


template<class T>
static inline void FileWrite( long fp, T out )
{
	FileWrite( fp, &out, sizeof(out) );
}


IIncremental* GetIncremental()
{
	static CIncremental inc;
	return &inc;
}


// -------------------------------------------------------------------------------- //
// CIncremental.
// -------------------------------------------------------------------------------- //

CIncremental::CIncremental()
{
	m_TotalMemory = 0;
	m_pIncrementalFilename = NULL;
	m_pBSPFilename = NULL;
	m_bSuccessfulRun = false;
}


CIncremental::~CIncremental()
{
}


bool CIncremental::Init( char const *pBSPFilename, char const *pIncrementalFilename )
{
	m_pBSPFilename = pBSPFilename;
	m_pIncrementalFilename = pIncrementalFilename;
	return true;
}


bool CIncremental::PrepareForLighting()
{
	if( !m_pBSPFilename )
		return false;

	// Clear the touched faces list.
	m_FacesTouched.SetSize( numfaces );
	memset( m_FacesTouched.Base(), 0, numfaces );

	// If we haven't done a complete successful run yet, then we either haven't
	// loaded the lights, or a run was aborted and our lights are half-done so we
	// should reload them.
	if( !m_bSuccessfulRun )
		LoadIncrementalFile();

	// unmatched = a list of the lights we have
	CUtlLinkedList<int,int> unmatched;
	for( int i=m_Lights.Head(); i != m_Lights.InvalidIndex(); i = m_Lights.Next(i) )
		unmatched.AddToTail( i );

	// Match the light lists and get rid of lights that we already have all the data for.
	directlight_t *pNext;
	directlight_t **pPrev = &activelights;
	for( directlight_t *dl=activelights; dl != NULL; dl = pNext )
	{
		pNext = dl->next;

		//float flClosest = 3000000000;
		//CIncLight *pClosest = 0;

		// Look for this light in our light list.
		int iNextUnmatched, iUnmatched;
		for( iUnmatched=unmatched.Head(); iUnmatched != unmatched.InvalidIndex(); iUnmatched = iNextUnmatched )
		{
			iNextUnmatched = unmatched.Next( iUnmatched );

			CIncLight *pLight = m_Lights[ unmatched[iUnmatched] ];

			//float flTest = (pLight->m_Light.origin - dl->light.origin).Length();
			//if( flTest < flClosest )
			//{
			//	flClosest = flTest;
			//	pClosest = pLight;
			//}

			if( CompareLights( &dl->light, &pLight->m_Light ) )
			{
				unmatched.Remove( iUnmatched );

				// Ok, we have this light's data already, yay!
				// Get rid of it from the active light list.
				*pPrev = dl->next;
				free( dl );
				dl = 0;
				break;
			}
		}

		//bool bTest=false;
		//if(bTest)
		//	CompareLights( &dl->light, &pClosest->m_Light );

		if( iUnmatched == unmatched.InvalidIndex() )
			pPrev = &dl->next;
	}

	// Remove any of our lights that were unmatched.
	for( int iUnmatched=unmatched.Head(); iUnmatched != unmatched.InvalidIndex(); iUnmatched = unmatched.Next( iUnmatched ) )
	{
		CIncLight *pLight = m_Lights[ unmatched[iUnmatched] ];
		
		// First tag faces that it touched so they get recomposited.
		for( unsigned short iFace=pLight->m_LightFaces.Head(); iFace != pLight->m_LightFaces.InvalidIndex(); iFace = pLight->m_LightFaces.Next( iFace ) )
		{
			m_FacesTouched[ pLight->m_LightFaces[iFace]->m_FaceIndex ] = 1;
		}
		
		delete pLight;
		m_Lights.Remove( unmatched[iUnmatched] );
	}

	// Now add a light structure for each new light.
	AddLightsForActiveLights();
	
	return true;
}


bool CIncremental::ReadIncrementalHeader( long fp, CIncrementalHeader *pHeader )
{
	int version;
	FileRead( fp, version );
	if( version != INCREMENTALFILE_VERSION )
		return false;

	int nFaces;
	FileRead( fp, nFaces );

	pHeader->m_FaceLightmapSizes.SetSize( nFaces );
	FileRead( fp, pHeader->m_FaceLightmapSizes.Base(), sizeof(CIncrementalHeader::CLMSize) * nFaces );

	return !FileError();
}


bool CIncremental::WriteIncrementalHeader( long fp )
{
	int version = INCREMENTALFILE_VERSION;
	FileWrite( fp, version );

	int nFaces = numfaces;
	FileWrite( fp, nFaces );

	CIncrementalHeader hdr;
	hdr.m_FaceLightmapSizes.SetSize( nFaces );

	for( int i=0; i < nFaces; i++ )
	{
		hdr.m_FaceLightmapSizes[i].m_Width = g_pFaces[i].m_LightmapTextureSizeInLuxels[0];
		hdr.m_FaceLightmapSizes[i].m_Height = g_pFaces[i].m_LightmapTextureSizeInLuxels[1];
	}

	FileWrite( fp, hdr.m_FaceLightmapSizes.Base(), sizeof(CIncrementalHeader::CLMSize) * nFaces );
	
	return !FileError();
}


bool CIncremental::IsIncrementalFileValid()
{
	long fp = FileOpen( m_pIncrementalFilename, true );
	if( !fp )
		return false;

	bool bValid = false;
	CIncrementalHeader hdr;
	if( ReadIncrementalHeader( fp, &hdr ) )
	{
		// If the number of faces is the same and their lightmap sizes are the same,
		// then this file is considered a legitimate incremental file.
		if( hdr.m_FaceLightmapSizes.Count() == numfaces )
		{
			int i;
			for( i=0; i < numfaces; i++ )
			{
				if( hdr.m_FaceLightmapSizes[i].m_Width  != g_pFaces[i].m_LightmapTextureSizeInLuxels[0] ||							
					hdr.m_FaceLightmapSizes[i].m_Height != g_pFaces[i].m_LightmapTextureSizeInLuxels[1] )
				{
					break;
				}
			}

			// Were all faces valid?
			if( i == numfaces )
				bValid = true;
		}
	}

	FileClose( fp );
	return bValid && !FileError();
}


void CIncremental::AddLightToFace( 
	IncrementalLightID lightID, 
	int iFace, 
	int iSample,
	int lmSize,
	float dot,
	int iThread )
{
	// If we're not being used, don't do anything.
	if( !m_pIncrementalFilename )
		return;

	CIncLight *pLight = m_Lights[lightID];

	// Check for the 99.99% case in which the face already exists.
	CLightFace *pFace;
	if( pLight->m_pCachedFaces[iThread] && 
		pLight->m_pCachedFaces[iThread]->m_FaceIndex == iFace )
	{
		pFace = pLight->m_pCachedFaces[iThread];
	}
	else
	{
		bool bNew;
		
		EnterCriticalSection( &pLight->m_CS );
			pFace = pLight->FindOrCreateLightFace( iFace, lmSize, &bNew );
		LeaveCriticalSection( &pLight->m_CS );

		pLight->m_pCachedFaces[iThread] = pFace;

		if( bNew )
			m_TotalMemory += pFace->m_LightValues.Count() * sizeof( pFace->m_LightValues[0] );
	}

	// Add this into the light's data.
	pFace->m_LightValues[iSample].m_Dot = dot;
}


unsigned short DecodeCharOrShort( CUtlBuffer *pIn )
{
	unsigned short val = pIn->GetUnsignedChar();
	if( val & 0x80 )
	{
		val = ((val & 0x7F) << 8) | pIn->GetUnsignedChar();
	}

	return val;
}


void EncodeCharOrShort( CUtlBuffer *pBuf, unsigned short val )
{
	if( (val & 0xFF80) == 0 )
	{
		pBuf->PutUnsignedChar( (unsigned char)val );
	}
	else
	{
		if( val > 32767 )
			val = 32767;

		pBuf->PutUnsignedChar( (val >> 8) | 0x80 );
		pBuf->PutUnsignedChar( val & 0xFF );
	}
}


void DecompressLightData( CUtlBuffer *pIn, CUtlVector<CLightValue> *pOut )
{
	int iOut = 0;
	while( pIn->TellGet() < pIn->TellPut() )
	{
		unsigned char runLength = pIn->GetUnsignedChar();
		unsigned short usVal = DecodeCharOrShort( pIn );

		while( runLength > 0 )
		{
			--runLength;

			pOut->Element(iOut).m_Dot = usVal;
			++iOut;
		}
	}
}

#ifdef _WIN32
#pragma warning (disable:4701)
#endif

void CompressLightData( 
	CLightValue const *pValues, 
	int nValues, 
	CUtlBuffer *pBuf )
{
	unsigned char runLength=0;
	unsigned short flLastValue;

	for( int i=0; i < nValues; i++ )
	{
		unsigned short flCurValue = (unsigned short)pValues[i].m_Dot;

		if( i == 0 )
		{
			flLastValue = flCurValue;
			runLength = 1;
		}
		else if( flCurValue == flLastValue && runLength < 255 )
		{
			++runLength;
		}
		else
		{
			pBuf->PutUnsignedChar( runLength );
			EncodeCharOrShort( pBuf, flLastValue );

			flLastValue = flCurValue;
			runLength = 1;
		}
	}

	// Write the end..
	if( runLength )
	{
		pBuf->PutUnsignedChar( runLength );
		EncodeCharOrShort( pBuf, flLastValue );
	}
}

#ifdef _WIN32
#pragma warning (default:4701)
#endif

void MultiplyValues( CUtlVector<CLightValue> &values, float scale )
{
	for( int i=0; i < values.Count(); i++ )
		values[i].m_Dot *= scale;
}


void CIncremental::FinishFace(
	IncrementalLightID lightID,
	int iFace,
	int iThread )
{
	CIncLight *pLight = m_Lights[lightID];

	// Check for the 99.99% case in which the face already exists.
	CLightFace *pFace;
	if( pLight->m_pCachedFaces[iThread] && pLight->m_pCachedFaces[iThread]->m_FaceIndex == iFace )
	{
		pFace = pLight->m_pCachedFaces[iThread];

		// Compress the data.
		MultiplyValues( pFace->m_LightValues, pLight->m_flMaxIntensity );
		
		pFace->m_CompressedData.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
		CompressLightData( 
			pFace->m_LightValues.Base(), 
			pFace->m_LightValues.Count(), 
			&pFace->m_CompressedData );

#if 0
	// test decompression
	CUtlVector<CLightValue> test;
	test.SetSize( 2048 );
	pFace->m_CompressedData.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	DecompressLightData( &pFace->m_CompressedData, &test );
#endif

		if( pFace->m_CompressedData.TellPut() == 0 )
		{
			// No contribution.. delete this face from the light.
			EnterCriticalSection( &pLight->m_CS );
				pLight->m_LightFaces.Remove( pFace->m_LightFacesIndex );
				delete pFace;
			LeaveCriticalSection( &pLight->m_CS );
		}
		else
		{
			// Discard the uncompressed data.
			pFace->m_LightValues.Purge();
			m_FacesTouched[ pFace->m_FaceIndex ] = 1;
		}
	}
}


bool CIncremental::Finalize()
{
	// If we're not being used, don't do anything.
	if( !m_pIncrementalFilename || !m_pBSPFilename )
		return false;

	CUtlVector<CFaceLightList> faceLights;
	LinkLightsToFaces( faceLights );
	
	Vector faceLight[(MAX_LIGHTMAP_DIM_WITHOUT_BORDER+2) * (MAX_LIGHTMAP_DIM_WITHOUT_BORDER+2)];
	CUtlVector<CLightValue> faceLightValues;
	faceLightValues.SetSize( (MAX_LIGHTMAP_DIM_WITHOUT_BORDER+2) * (MAX_LIGHTMAP_DIM_WITHOUT_BORDER+2) );

	// Only update the faces we've touched.
    for( int facenum = 0; facenum < numfaces; facenum++ )
    {
        if( !m_FacesTouched[facenum] || !faceLights[facenum].Count() )
			continue;

		int w = g_pFaces[facenum].m_LightmapTextureSizeInLuxels[0]+1;
		int h = g_pFaces[facenum].m_LightmapTextureSizeInLuxels[1]+1;
		int nLuxels = w * h;
		assert( nLuxels <= sizeof(faceLight) / sizeof(faceLight[0]) );

		// Clear the lighting for this face.
		memset( faceLight, 0, nLuxels * sizeof(Vector) );

		// Composite all the light contributions.
		for( int iFace=0; iFace < faceLights[facenum].Count(); iFace++ )
		{
			CLightFace *pFace = faceLights[facenum][iFace];
		
			pFace->m_CompressedData.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
			DecompressLightData( &pFace->m_CompressedData, &faceLightValues );

			for( int iSample=0; iSample < nLuxels; iSample++ )
			{
				float flDot = faceLightValues[iSample].m_Dot;
				if( flDot )
				{
					VectorMA( 
						faceLight[iSample], 
						flDot / pFace->m_pLight->m_flMaxIntensity,
						pFace->m_pLight->m_Light.intensity, 
						faceLight[iSample] );
				}
			}
		}

		// Convert to the floating-point representation in the BSP file.
		Vector *pSrc = faceLight;
		unsigned char *pDest = &(*pdlightdata)[ g_pFaces[facenum].lightofs ];

		for( int iSample=0; iSample < nLuxels; iSample++ )
		{
			VectorToColorRGBExp32( *pSrc, *( ColorRGBExp32 *)pDest );
			pDest += 4;
			pSrc++;
		}
	}
	
	m_bSuccessfulRun = true;
	return true;
}


void CIncremental::GetFacesTouched( CUtlVector<unsigned char> &touched )
{
	touched.CopyArray( m_FacesTouched.Base(), m_FacesTouched.Count() );
}


bool CIncremental::Serialize()
{
	if( !SaveIncrementalFile() )
		return false;

	WriteBSPFile( (char*)m_pBSPFilename );
	return true;
}


void CIncremental::Term()
{
	m_Lights.PurgeAndDeleteElements();
	m_TotalMemory = 0;
}


void CIncremental::AddLightsForActiveLights()
{
	// Create our lights.
	for( directlight_t *dl=activelights; dl != NULL; dl = dl->next )
	{
		CIncLight *pLight = new CIncLight;
		dl->m_IncrementalID = m_Lights.AddToTail( pLight );

		// Copy the light information.
		pLight->m_Light = dl->light;
		pLight->m_flMaxIntensity = max( dl->light.intensity[0], max( dl->light.intensity[1], dl->light.intensity[2] ) );
	}
}


bool CIncremental::LoadIncrementalFile()
{
	Term();

	if( !IsIncrementalFileValid() )
		return false;

	long fp = FileOpen( m_pIncrementalFilename, true );
	if( !fp )
		return false;

	// Read the header.
	CIncrementalHeader hdr;
	if( !ReadIncrementalHeader( fp, &hdr ) )
	{
		FileClose( fp );
		return false;
	}


	// Read the lights.
	int nLights;
	FileRead( fp, nLights );
	for( int iLight=0; iLight < nLights; iLight++ )
	{
		CIncLight *pLight = new CIncLight;
		m_Lights.AddToTail( pLight );

		FileRead( fp, pLight->m_Light );
		pLight->m_flMaxIntensity = 
			max( pLight->m_Light.intensity.x, 
				max( pLight->m_Light.intensity.y, pLight->m_Light.intensity.z ) );

		int nFaces;
		FileRead( fp, nFaces );
		assert( nFaces < 70000 );

		for( int iFace=0; iFace < nFaces; iFace++ )
		{
			CLightFace *pFace = new CLightFace;
			pLight->m_LightFaces.AddToTail( pFace );

			pFace->m_pLight = pLight;
			FileRead( fp, pFace->m_FaceIndex );

			int dataSize;
			FileRead( fp, dataSize );

			pFace->m_CompressedData.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
			while( dataSize )
			{
				--dataSize;
				
				unsigned char ucData;
				FileRead( fp, ucData );

				pFace->m_CompressedData.PutUnsignedChar( ucData );
			}
		}
	}

	
	FileClose( fp );
	return !FileError();
}


bool CIncremental::SaveIncrementalFile()
{
	long fp = FileOpen( m_pIncrementalFilename, false );
	if( !fp )
		return false;

	if( !WriteIncrementalHeader( fp ) )
	{
		FileClose( fp );
		return false;
	}

	// Write the lights.
	int nLights = m_Lights.Count();
	FileWrite( fp, nLights );
	for( int iLight=m_Lights.Head(); iLight != m_Lights.InvalidIndex(); iLight = m_Lights.Next( iLight ) )
	{
		CIncLight *pLight = m_Lights[iLight];
		
		FileWrite( fp, pLight->m_Light );

		int nFaces = pLight->m_LightFaces.Count();
		FileWrite( fp, nFaces );
		for( int iFace=pLight->m_LightFaces.Head(); iFace != pLight->m_LightFaces.InvalidIndex(); iFace = pLight->m_LightFaces.Next( iFace ) )
		{
			CLightFace *pFace = pLight->m_LightFaces[iFace];

			FileWrite( fp, pFace->m_FaceIndex );
			
			int dataSize = pFace->m_CompressedData.TellPut();
			FileWrite( fp, dataSize );

			pFace->m_CompressedData.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
			while( dataSize )
			{
				--dataSize;
				FileWrite( fp, pFace->m_CompressedData.GetUnsignedChar() );
			}
		}
	}


	FileClose( fp );
	return !FileError();
}


void CIncremental::LinkLightsToFaces( CUtlVector<CFaceLightList> &faceLights )
{
	faceLights.SetSize( numfaces );
	
	for( int iLight=m_Lights.Head(); iLight != m_Lights.InvalidIndex(); iLight = m_Lights.Next( iLight ) )
	{
		CIncLight *pLight = m_Lights[iLight];

		for( int iFace=pLight->m_LightFaces.Head(); iFace != pLight->m_LightFaces.InvalidIndex(); iFace = pLight->m_LightFaces.Next( iFace ) )
		{
			CLightFace *pFace = pLight->m_LightFaces[iFace];

			if( m_FacesTouched[pFace->m_FaceIndex] )
				faceLights[ pFace->m_FaceIndex ].AddToTail( pFace );
		}
	}
}


// ------------------------------------------------------------------ //
// CIncLight
// ------------------------------------------------------------------ //

CIncLight::CIncLight()
{
	memset( m_pCachedFaces, 0, sizeof(m_pCachedFaces) );
	InitializeCriticalSection( &m_CS );
}


CIncLight::~CIncLight()
{
	m_LightFaces.PurgeAndDeleteElements();
	DeleteCriticalSection( &m_CS );
}


CLightFace* CIncLight::FindOrCreateLightFace( int iFace, int lmSize, bool *bNew )
{
	if( bNew )
		*bNew = false;


	// Look for it.
	for( int i=m_LightFaces.Head(); i != m_LightFaces.InvalidIndex(); i=m_LightFaces.Next(i) )
	{
		CLightFace *pFace = m_LightFaces[i];

		if( pFace->m_FaceIndex == iFace )
		{
			assert( pFace->m_LightValues.Count() == lmSize );
			return pFace;
		}
	}

	// Ok, create one.
	CLightFace *pFace = new CLightFace;
	pFace->m_LightFacesIndex = m_LightFaces.AddToTail( pFace );
	pFace->m_pLight = this;
	
	pFace->m_FaceIndex = iFace;
	pFace->m_LightValues.SetSize( lmSize );
	memset( pFace->m_LightValues.Base(), 0, sizeof( CLightValue ) * lmSize );

	if( bNew )
		*bNew = true;

	return pFace;
}


