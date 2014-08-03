//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "tier1/strtools.h"
#include "macro_texture.h"
#include "bsplib.h"
#include "cmdlib.h"
#include "vtf/vtf.h"
#include "tier1/utldict.h"
#include "tier1/utlbuffer.h"
#include "bitmap/imageformat.h"


class CMacroTextureData
{
public:
	int m_Width, m_Height;
	CUtlMemory<unsigned char> m_ImageData;
};


CMacroTextureData *g_pGlobalMacroTextureData = NULL;

// Which macro texture each map face uses.
static CUtlDict<CMacroTextureData*, int> g_MacroTextureLookup;	// Stores a list of unique macro textures.
static CUtlVector<CMacroTextureData*> g_FaceMacroTextures;		// Which macro texture each face wants to use.
static Vector g_MacroWorldMins, g_MacroWorldMaxs;


CMacroTextureData* FindMacroTexture( const char *pFilename )
{
	int index = g_MacroTextureLookup.Find( pFilename );
	if ( g_MacroTextureLookup.IsValidIndex( index ) )
		return g_MacroTextureLookup[index];
	else
		return NULL;
}


CMacroTextureData* LoadMacroTextureFile( const char *pFilename )
{
	FileHandle_t hFile = g_pFileSystem->Open( pFilename, "rb" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
		return NULL;

	// Read the file in.
	CUtlVector<char> tempData;
	tempData.SetSize( g_pFileSystem->Size( hFile ) );
	g_pFileSystem->Read( tempData.Base(), tempData.Count(), hFile );
	g_pFileSystem->Close( hFile );
	
	
	// Now feed the data into a CUtlBuffer (great...)
	CUtlBuffer buf;
	buf.Put( tempData.Base(), tempData.Count() );

	
	// Now make a texture out of it.
	IVTFTexture *pTex = CreateVTFTexture();
	if ( !pTex->Unserialize( buf ) )
		Error( "IVTFTexture::Unserialize( %s ) failed.", pFilename );

	pTex->ConvertImageFormat( IMAGE_FORMAT_RGBA8888, false );	// Get it in a format we like.

	
	// Now convert to a CMacroTextureData.
	CMacroTextureData *pData = new CMacroTextureData;
	pData->m_Width = pTex->Width();
	pData->m_Height = pTex->Height();
	pData->m_ImageData.EnsureCapacity( pData->m_Width * pData->m_Height * 4 );
	memcpy( pData->m_ImageData.Base(), pTex->ImageData(), pData->m_Width * pData->m_Height * 4 );

	DestroyVTFTexture( pTex );

	Msg( "-- LoadMacroTextureFile: %s\n", pFilename );
	return pData;
}


void InitMacroTexture( const char *pBSPFilename )
{
	// Get the world bounds (same ones used by minimaps and level designers know how to use).
	int i = 0;
	for (i; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if( !strcmp(pEntity, "worldspawn") )
		{
			GetVectorForKey( &entities[i], "world_mins", g_MacroWorldMins );
			GetVectorForKey( &entities[i], "world_maxs", g_MacroWorldMaxs );
			break;
		}
	}

	if ( i == num_entities )
	{
		Warning( "MaskOnMacroTexture: can't find worldspawn" );
		return;
	}


	// Load the macro texture that is mapped onto everything.
	char mapName[512], vtfFilename[512];
	Q_FileBase( pBSPFilename, mapName, sizeof( mapName ) );
	Q_snprintf( vtfFilename, sizeof( vtfFilename ), "materials/macro/%s/base.vtf", mapName );
	g_pGlobalMacroTextureData = LoadMacroTextureFile( vtfFilename );

	
	// Now load the macro texture for each face.
	g_FaceMacroTextures.SetSize( numfaces );
	for ( int iFace=0; iFace < numfaces; iFace++ )
	{
		g_FaceMacroTextures[iFace] = NULL;

		if ( iFace < g_FaceMacroTextureInfos.Count() )
		{
			unsigned short stringID = g_FaceMacroTextureInfos[iFace].m_MacroTextureNameID;
			if ( stringID != 0xFFFF )
			{
				const char *pMacroTextureName = &g_TexDataStringData[ g_TexDataStringTable[stringID] ];
				Q_snprintf( vtfFilename, sizeof( vtfFilename ), "%smaterials/%s.vtf", gamedir, pMacroTextureName );
				
				g_FaceMacroTextures[iFace] = FindMacroTexture( vtfFilename );
				if ( !g_FaceMacroTextures[iFace] )
				{
					g_FaceMacroTextures[iFace] = LoadMacroTextureFile( vtfFilename );
					if ( g_FaceMacroTextures[iFace] )
					{
						g_MacroTextureLookup.Insert( vtfFilename, g_FaceMacroTextures[iFace] );
					}
				}
			}
		}
	}
}


inline Vector SampleMacroTexture( const CMacroTextureData *t, const Vector &vWorldPos )
{
	int ix = (int)RemapVal( vWorldPos.x, g_MacroWorldMins.x, g_MacroWorldMaxs.x, 0, t->m_Width-0.00001 );
	int iy = (int)RemapVal( vWorldPos.y, g_MacroWorldMins.y, g_MacroWorldMaxs.y, 0, t->m_Height-0.00001 );
	ix = clamp( ix, 0, t->m_Width-1 );
	iy = t->m_Height - 1 - clamp( iy, 0, t->m_Height-1 );

	const unsigned char *pInputColor = &t->m_ImageData[(iy*t->m_Width + ix) * 4];
	return Vector( pInputColor[0] / 255.0, pInputColor[1] / 255.0, pInputColor[2] / 255.0 );
}


void ApplyMacroTextures( int iFace, const Vector &vWorldPos, Vector &outLuxel )
{
	// Add the global macro texture.
	Vector vGlobal;
	if ( g_pGlobalMacroTextureData )
		outLuxel *= SampleMacroTexture( g_pGlobalMacroTextureData, vWorldPos );

	// Now add the per-material macro texture.
	if ( g_FaceMacroTextures[iFace] )
		outLuxel *= SampleMacroTexture( g_FaceMacroTextures[iFace], vWorldPos );
}



