//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "bsplib.h"
#include "tier1/UtlBuffer.h"
#include "tier1/utlvector.h"
#include "bitmap/imageformat.h"
#include <KeyValues.h>
#include "tier1/strtools.h"
#include "tier1/utlsymbol.h"
#include "vtf/vtf.h"
#include "materialpatch.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"


/*
	Meager documentation for how the cubemaps are assigned.


	While loading the map, it calls:
		*** Cubemap_SaveBrushSides
			Builds a list of what cubemaps manually were assigned to what faces
			in s_EnvCubemapToBrushSides.

	Immediately after loading the map, it calls:
		*** Cubemap_FixupBrushSidesMaterials
			Goes through s_EnvCubemapToBrushSides and does Cubemap_CreateTexInfo for each
			side referenced by an env_cubemap manually.

	Then it calls Cubemap_AttachDefaultCubemapToSpecularSides:
		*** Cubemap_InitCubemapSideData:
			Setup s_aCubemapSideData.bHasEnvMapInMaterial and bManuallyPickedByAnEnvCubemap for each side.
			bHasEnvMapInMaterial is set if the side's material has $envmap.
			bManuallyPickedByAnEnvCubemap is true if the side was in s_EnvCubemapToBrushSides.

		Then, for each bHasEnvMapInMaterial and !bManuallyPickedByAnEnvCubemap (ie: every specular surface that wasn't 
		referenced by some env_cubemap), it does Cubemap_CreateTexInfo.
*/

struct PatchInfo_t
{
	char *m_pMapName;
	int m_pOrigin[3];
};

struct CubemapInfo_t
{
	int m_nTableId;
	bool m_bSpecular;
};

static bool CubemapLessFunc( const CubemapInfo_t &lhs, const CubemapInfo_t &rhs )
{ 
	return ( lhs.m_nTableId < rhs.m_nTableId );	
}


typedef CUtlVector<int> IntVector_t;
static CUtlVector<IntVector_t> s_EnvCubemapToBrushSides;

static CUtlVector<char *> s_DefaultCubemapNames;
static char g_IsCubemapTexData[MAX_MAP_TEXDATA];


struct CubemapSideData_t
{
	bool bHasEnvMapInMaterial;
	bool bManuallyPickedByAnEnvCubemap;
};

static CubemapSideData_t s_aCubemapSideData[MAX_MAP_BRUSHSIDES];



inline bool SideHasCubemapAndWasntManuallyReferenced( int iSide )
{
	return s_aCubemapSideData[iSide].bHasEnvMapInMaterial && !s_aCubemapSideData[iSide].bManuallyPickedByAnEnvCubemap;
}


void Cubemap_InsertSample( const Vector& origin, int size )
{
	dcubemapsample_t *pSample = &g_CubemapSamples[g_nCubemapSamples];
	pSample->origin[0] = ( int )origin[0];	
	pSample->origin[1] = ( int )origin[1];	
	pSample->origin[2] = ( int )origin[2];	
	pSample->size = size;
	g_nCubemapSamples++;
}

static const char *FindSkyboxMaterialName( void )
{
	for( int i = 0; i < g_MainMap->num_entities; i++ )
	{
		char* pEntity = ValueForKey(&g_MainMap->entities[i], "classname");
		if (!strcmp(pEntity, "worldspawn"))
		{
			return ValueForKey( &g_MainMap->entities[i], "skyname" );
		}
	}
	return NULL;
}

static void BackSlashToForwardSlash( char *pname )
{
	while ( *pname ) {
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}

static void ForwardSlashToBackSlash( char *pname )
{
	while ( *pname ) {
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
}


//-----------------------------------------------------------------------------
// Finds materials that are used by a particular material
//-----------------------------------------------------------------------------
#define MAX_MATERIAL_NAME 512

// This is the list of materialvars which are used in our codebase to look up dependent materials
static const char *s_pDependentMaterialVar[] = 
{
	"$bottommaterial",	// Used by water materials
	"$crackmaterial",	// Used by shattered glass materials
	"$fallbackmaterial",	// Used by all materials

	"",					// Always must be last
};

static const char *FindDependentMaterial( const char *pMaterialName, const char **ppMaterialVar = NULL )
{
	// FIXME: This is a terrible way of doing this! It creates a dependency
	// between vbsp and *all* code which reads dependent materials from materialvars
	// At the time of writing this function, that means the engine + studiorender.
	// We need a better way of figuring out how to do this, but for now I'm trying to do
	// the fastest solution possible since it's close to ship

	static char pDependentMaterialName[MAX_MATERIAL_NAME];
	for( int i = 0; s_pDependentMaterialVar[i][0]; ++i )
	{
		if ( !GetValueFromMaterial( pMaterialName, s_pDependentMaterialVar[i], pDependentMaterialName, MAX_MATERIAL_NAME - 1 ) )
			continue;

		if ( !Q_stricmp( pDependentMaterialName, pMaterialName ) )
		{
			Warning( "Material %s is depending on itself through materialvar %s! Ignoring...\n", pMaterialName, s_pDependentMaterialVar[i] );
				continue;
		}

		// Return the material var that caused the dependency
		if ( ppMaterialVar )
		{
			*ppMaterialVar = s_pDependentMaterialVar[i];
		}

#ifdef _DEBUG
		// FIXME: Note that this code breaks if a material has more than 1 dependent material
		++i;
		static char pDependentMaterialName2[MAX_MATERIAL_NAME];
		while( s_pDependentMaterialVar[i][0] )
		{
			Assert( !GetValueFromMaterial( pMaterialName, s_pDependentMaterialVar[i], pDependentMaterialName2, MAX_MATERIAL_NAME - 1 ) );
			++i;
		}
#endif

		return pDependentMaterialName;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Loads VTF files
//-----------------------------------------------------------------------------
static bool LoadSrcVTFFiles( IVTFTexture *pSrcVTFTextures[6], const char *pSkyboxMaterialBaseName,
							int *pUnionTextureFlags, bool bHDR )
{
	const char *facingName[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	int i;
	for( i = 0; i < 6; i++ )
	{
		char srcMaterialName[1024];
		sprintf( srcMaterialName, "%s%s", pSkyboxMaterialBaseName, facingName[i] );

		IMaterial *pSkyboxMaterial = g_pMaterialSystem->FindMaterial( srcMaterialName, "skybox" );
		//IMaterialVar *pSkyTextureVar = pSkyboxMaterial->FindVar( bHDR ? "$hdrbasetexture" : "$basetexture", NULL ); //, bHDR ? false : true );
		IMaterialVar *pSkyTextureVar = pSkyboxMaterial->FindVar( "$basetexture", NULL ); // Since we're setting it to black anyway, just use $basetexture for HDR
		const char *vtfName = pSkyTextureVar->GetStringValue();
		char srcVTFFileName[MAX_PATH];
		Q_snprintf( srcVTFFileName, MAX_PATH, "materials/%s.vtf", vtfName );

		CUtlBuffer buf;
		if ( !g_pFullFileSystem->ReadFile( srcVTFFileName, NULL, buf ) )
		{
			// Try looking for a compressed HDR texture
			if ( bHDR )
			{
				/* // FIXME: We need a way to uncompress this format!
				bool bHDRCompressed = true;

				pSkyTextureVar = pSkyboxMaterial->FindVar( "$hdrcompressedTexture", NULL );
				vtfName = pSkyTextureVar->GetStringValue();
				Q_snprintf( srcVTFFileName, MAX_PATH, "materials/%s.vtf", vtfName );

				if ( !g_pFullFileSystem->ReadFile( srcVTFFileName, NULL, buf ) )
				*/
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		pSrcVTFTextures[i] = CreateVTFTexture();
		if (!pSrcVTFTextures[i]->Unserialize(buf))
		{
			Warning("*** Error unserializing skybox texture: %s\n", pSkyboxMaterialBaseName );
			return false;
		}

		*pUnionTextureFlags |= pSrcVTFTextures[i]->Flags();
		int flagsNoAlpha = pSrcVTFTextures[i]->Flags() & ~( TEXTUREFLAGS_EIGHTBITALPHA | TEXTUREFLAGS_ONEBITALPHA );
		int flagsFirstNoAlpha = pSrcVTFTextures[0]->Flags() & ~( TEXTUREFLAGS_EIGHTBITALPHA | TEXTUREFLAGS_ONEBITALPHA );
		
		// NOTE: texture[0] is a side texture that could be 1/2 height, so allow this and also allow 4x4 faces
		if ( ( ( pSrcVTFTextures[i]->Width() != pSrcVTFTextures[0]->Width() ) && ( pSrcVTFTextures[i]->Width() != 4 ) ) ||
			 ( ( pSrcVTFTextures[i]->Height() != pSrcVTFTextures[0]->Height() ) && ( pSrcVTFTextures[i]->Height() != pSrcVTFTextures[0]->Height()*2 )  && ( pSrcVTFTextures[i]->Height() != 4 ) ) ||
			 ( flagsNoAlpha != flagsFirstNoAlpha ) )
		{
			Warning("*** Error: Skybox vtf files for %s weren't compiled with the same size texture and/or same flags!\n", pSkyboxMaterialBaseName );
			return false;
		}

		if ( bHDR )
		{
			pSrcVTFTextures[i]->ConvertImageFormat( IMAGE_FORMAT_RGB323232F, false );
			pSrcVTFTextures[i]->GenerateMipmaps();
			pSrcVTFTextures[i]->ConvertImageFormat( IMAGE_FORMAT_RGBA16161616F, false );
		}
	}

	return true;
}

void VTFNameToHDRVTFName( const char *pSrcName, char *pDest, int maxLen, bool bHDR )
{
	Q_strncpy( pDest, pSrcName, maxLen );
	if( !bHDR )
	{
		return;
	}
	char *pDot = Q_stristr( pDest, ".vtf" );
	if( !pDot )
	{
		return;
	}
	Q_strncpy( pDot, ".hdr.vtf", maxLen - ( pDot - pDest ) );
}

#define DEFAULT_CUBEMAP_SIZE 32

void CreateDefaultCubemaps( bool bHDR )
{
	memset( g_IsCubemapTexData, 0, sizeof(g_IsCubemapTexData) );

	// NOTE: This implementation depends on the fact that all VTF files contain
	// all mipmap levels
	const char *pSkyboxBaseName = FindSkyboxMaterialName();

	if( !pSkyboxBaseName )
	{
		if( s_DefaultCubemapNames.Count() )
		{
			Warning( "This map uses env_cubemap, and you don't have a skybox, so no default env_cubemaps will be generated.\n" );
		}
		return;
	}

	char skyboxMaterialName[MAX_PATH];
	Q_snprintf( skyboxMaterialName, MAX_PATH, "skybox/%s", pSkyboxBaseName );

	IVTFTexture *pSrcVTFTextures[6];

	int unionTextureFlags = 0;
	if( !LoadSrcVTFFiles( pSrcVTFTextures, skyboxMaterialName, &unionTextureFlags, bHDR ) )
	{
		Warning( "Can't load skybox file %s to build the default cubemap!\n", skyboxMaterialName );
		return;
	}
	Msg( "Creating default %scubemaps for env_cubemap using skybox materials:\n   %s*.vmt\n"
		" ! Run buildcubemaps in the engine to get the correct cube maps.\n", bHDR ? "HDR " : "LDR ", skyboxMaterialName );
			
	// Figure out the mip differences between the two textures
	int iMipLevelOffset = 0;
	int tmp = pSrcVTFTextures[0]->Width();
	while( tmp > DEFAULT_CUBEMAP_SIZE )
	{
		iMipLevelOffset++;
		tmp >>= 1;
	}

	// Create the destination cubemap
	IVTFTexture *pDstCubemap = CreateVTFTexture();
	pDstCubemap->Init( DEFAULT_CUBEMAP_SIZE, DEFAULT_CUBEMAP_SIZE, 1,
		pSrcVTFTextures[0]->Format(), unionTextureFlags | TEXTUREFLAGS_ENVMAP, 
		pSrcVTFTextures[0]->FrameCount() );

	// First iterate over all frames
	for (int iFrame = 0; iFrame < pDstCubemap->FrameCount(); ++iFrame)
	{
		// Next iterate over all normal cube faces (we know there's 6 cause it's an envmap)
		for (int iFace = 0; iFace < 6; ++iFace )
		{
			// Finally, iterate over all mip levels in the *destination*
			for (int iMip = 0; iMip < pDstCubemap->MipCount(); ++iMip )
			{
				// Copy the bits from the source images into the cube faces
				unsigned char *pSrcBits = pSrcVTFTextures[iFace]->ImageData( iFrame, 0, iMip + iMipLevelOffset );
				unsigned char *pDstBits = pDstCubemap->ImageData( iFrame, iFace, iMip );
				int iSize = pDstCubemap->ComputeMipSize( iMip );
				int iSrcMipSize = pSrcVTFTextures[iFace]->ComputeMipSize( iMip + iMipLevelOffset );

				// !!! FIXME: Set this to black until HDR cubemaps are built properly!
				memset( pDstBits, 0, iSize );
				continue;

				if ( ( pSrcVTFTextures[iFace]->Width() == 4 ) && ( pSrcVTFTextures[iFace]->Height() == 4 ) ) // If texture is 4x4 square
				{
					// Force mip level 2 to get the 1x1 face
					unsigned char *pSrcBits = pSrcVTFTextures[iFace]->ImageData( iFrame, 0, 2 );
					int iSrcMipSize = pSrcVTFTextures[iFace]->ComputeMipSize( 2 );

					// Replicate 1x1 mip level across entire face
					//memset( pDstBits, 0, iSize ); 
					for ( int i = 0; i < ( iSize / iSrcMipSize ); i++ )
					{
						memcpy( pDstBits + ( i * iSrcMipSize ), pSrcBits, iSrcMipSize ); 
					}
				}
				else if ( pSrcVTFTextures[iFace]->Width() == pSrcVTFTextures[iFace]->Height() ) // If texture is square
				{
					if ( iSrcMipSize != iSize )
					{
						Warning( "%s - ERROR! Cannot copy square face for default cubemap! iSrcMipSize(%d) != iSize(%d)\n", skyboxMaterialName, iSrcMipSize, iSize );
						memset( pDstBits, 0, iSize );
					}
					else
					{
						// Just copy the mip level
						memcpy( pDstBits, pSrcBits, iSize ); 
					}
				}
				else if ( pSrcVTFTextures[iFace]->Width() == pSrcVTFTextures[iFace]->Height()*2 ) // If texture is rectangle 2x wide
				{
					int iMipWidth, iMipHeight, iMipDepth;
					pDstCubemap->ComputeMipLevelDimensions( iMip, &iMipWidth, &iMipHeight, &iMipDepth );
					if ( ( iMipHeight > 1 ) && ( iSrcMipSize*2 != iSize ) )
					{
						Warning( "%s - ERROR building default cube map! %d*2 != %d\n", skyboxMaterialName, iSrcMipSize, iSize );
						memset( pDstBits, 0, iSize );
					}
					else
					{
						// Copy row at a time and repeat last row
						memcpy( pDstBits, pSrcBits, iSize/2 ); 
						//memcpy( pDstBits + iSize/2, pSrcBits, iSize/2 );
						int nSrcRowSize = pSrcVTFTextures[iFace]->RowSizeInBytes( iMip + iMipLevelOffset );
						int nDstRowSize = pDstCubemap->RowSizeInBytes( iMip );
						if ( nSrcRowSize != nDstRowSize )
						{
							Warning( "%s - ERROR building default cube map! nSrcRowSize(%d) != nDstRowSize(%d)!\n", skyboxMaterialName, nSrcRowSize, nDstRowSize );
							memset( pDstBits, 0, iSize );
						}
						else
						{
							for ( int i = 0; i < ( iSize/2 / nSrcRowSize ); i++ )
							{
								memcpy( pDstBits + iSize/2 + i*nSrcRowSize, pSrcBits + iSrcMipSize - nSrcRowSize, nSrcRowSize );
							}
						}
					}
				}
				else
				{
					// ERROR! This code only supports square and rectangluar 2x wide
					Warning( "%s - Couldn't create default cubemap because texture res is %dx%d\n", skyboxMaterialName, pSrcVTFTextures[iFace]->Width(), pSrcVTFTextures[iFace]->Height() );
					memset( pDstBits, 0, iSize );
					return;
				}
			}
		}
	}

	ImageFormat originalFormat = pDstCubemap->Format();
	if( !bHDR )
	{
		// Convert the cube to format that we can apply tools to it...
		pDstCubemap->ConvertImageFormat( IMAGE_FORMAT_DEFAULT, false );
	}

	// Fixup the cubemap facing
	pDstCubemap->FixCubemapFaceOrientation();

	// Now that the bits are in place, compute the spheremaps...
	pDstCubemap->GenerateSpheremap();

	if( !bHDR )
	{
		// Convert the cubemap to the final format
		pDstCubemap->ConvertImageFormat( originalFormat, false );
	}

	// Write the puppy out!
	char dstVTFFileName[1024];
	if( bHDR )
	{
		sprintf( dstVTFFileName, "materials/maps/%s/cubemapdefault.hdr.vtf", mapbase );
	}
	else
	{
		sprintf( dstVTFFileName, "materials/maps/%s/cubemapdefault.vtf", mapbase );
	}

	CUtlBuffer outputBuf;
	if (!pDstCubemap->Serialize( outputBuf ))
	{
		Warning( "Error serializing default cubemap %s\n", dstVTFFileName );
		return;
	}

	IZip *pak = GetPakFile();

	// spit out the default one.
	AddBufferToPak( pak, dstVTFFileName, outputBuf.Base(), outputBuf.TellPut(), false );

	// spit out all of the ones that are attached to world geometry.
	int i;
	for( i = 0; i < s_DefaultCubemapNames.Count(); i++ )
	{
		char vtfName[MAX_PATH];
		VTFNameToHDRVTFName( s_DefaultCubemapNames[i], vtfName, MAX_PATH, bHDR );
		if( FileExistsInPak( pak, vtfName ) )
		{
			continue;
		}
		AddBufferToPak( pak, vtfName, outputBuf.Base(),outputBuf.TellPut(), false );
	}

	// Clean up the textures
	for( i = 0; i < 6; i++ )
	{
		DestroyVTFTexture( pSrcVTFTextures[i] );
	}
	DestroyVTFTexture( pDstCubemap );
}	

void Cubemap_CreateDefaultCubemaps( void )
{
	CreateDefaultCubemaps( false );
	CreateDefaultCubemaps( true );
}

// Builds a list of what cubemaps manually were assigned to what faces
// in s_EnvCubemapToBrushSides.
void Cubemap_SaveBrushSides( const char *pSideListStr )
{
	IntVector_t &brushSidesVector = s_EnvCubemapToBrushSides[s_EnvCubemapToBrushSides.AddToTail()];
	char *pTmp = ( char * )_alloca( strlen( pSideListStr ) + 1 );
	strcpy( pTmp, pSideListStr );
	const char *pScan = strtok( pTmp, " " );
	if( !pScan )
	{
		return;
	}
	do
	{
		int brushSideID;
		if( sscanf( pScan, "%d", &brushSideID ) == 1 )
		{
			brushSidesVector.AddToTail( brushSideID );
		}
	} while( ( pScan = strtok( NULL, " " ) ) );
}


//-----------------------------------------------------------------------------
// Generate patched material name
//-----------------------------------------------------------------------------
static void GeneratePatchedName( const char *pMaterialName, const PatchInfo_t &info, bool bMaterialName, char *pBuffer, int nMaxLen )
{
	const char *pSeparator = bMaterialName ? "_" : "";
	int nLen = Q_snprintf( pBuffer, nMaxLen, "maps/%s/%s%s%d_%d_%d", info.m_pMapName, 
		pMaterialName, pSeparator, info.m_pOrigin[0], info.m_pOrigin[1], info.m_pOrigin[2] ); 

	if ( bMaterialName )
	{
		Assert( nLen < TEXTURE_NAME_LENGTH - 1 );
		if ( nLen >= TEXTURE_NAME_LENGTH - 1 )
		{
			Error( "Generated env_cubemap patch name : %s too long! (max = %d)\n", pBuffer, TEXTURE_NAME_LENGTH );
		}
	}

	BackSlashToForwardSlash( pBuffer );
	Q_strlower( pBuffer );
}


//-----------------------------------------------------------------------------
// Patches the $envmap for a material and all its dependents, returns true if any patching happened
//-----------------------------------------------------------------------------
static bool PatchEnvmapForMaterialAndDependents( const char *pMaterialName, const PatchInfo_t &info, const char *pCubemapTexture )
{
	// Do *NOT* patch the material if there is an $envmap specified and it's not 'env_cubemap'

	// FIXME: It's theoretically ok to patch the material if $envmap is not specified,
	// because we're using the 'replace' block, which will only add the env_cubemap if 
	// $envmap is specified in the source material. But it will fail if someone adds
	// a specific non-env_cubemap $envmap to the source material at a later point. Bleah

	// See if we have an $envmap to patch
	bool bShouldPatchEnvCubemap = DoesMaterialHaveKeyValuePair( pMaterialName, "$envmap", "env_cubemap" );

	// See if we have a dependent material to patch
	bool bDependentMaterialPatched = false;
	const char *pDependentMaterialVar = NULL;
	const char *pDependentMaterial = FindDependentMaterial( pMaterialName, &pDependentMaterialVar );
	if ( pDependentMaterial )
	{
		bDependentMaterialPatched = PatchEnvmapForMaterialAndDependents( pDependentMaterial, info, pCubemapTexture );
	}

	// If we have neither to patch, we're done
	if ( !bShouldPatchEnvCubemap && !bDependentMaterialPatched )
		return false;

	// Otherwise we have to make a patched version of ourselves
	char pPatchedMaterialName[1024];
	GeneratePatchedName( pMaterialName, info, true, pPatchedMaterialName, 1024 );

	MaterialPatchInfo_t pPatchInfo[2];
	int nPatchCount = 0;
	if ( bShouldPatchEnvCubemap )
	{
		pPatchInfo[nPatchCount].m_pKey = "$envmap";
		pPatchInfo[nPatchCount].m_pRequiredOriginalValue = "env_cubemap";
		pPatchInfo[nPatchCount].m_pValue = pCubemapTexture;
		++nPatchCount;
	}

	char pDependentPatchedMaterialName[1024];
	if ( bDependentMaterialPatched )
	{
		// FIXME: Annoying! I either have to pass back the patched dependent material name
		// or reconstruct it. Both are sucky.
		GeneratePatchedName( pDependentMaterial, info, true, pDependentPatchedMaterialName, 1024 );
		pPatchInfo[nPatchCount].m_pKey = pDependentMaterialVar;
		pPatchInfo[nPatchCount].m_pValue = pDependentPatchedMaterialName;
		++nPatchCount;
	}

	CreateMaterialPatch( pMaterialName, pPatchedMaterialName, nPatchCount, pPatchInfo, PATCH_REPLACE );

	return true;
}


//-----------------------------------------------------------------------------
// Finds a texinfo that has a particular 
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Create a VMT to override the specified texinfo which references the cubemap entity at the specified origin.
// Returns the index of the new (or preexisting) texinfo referencing that VMT.
//
// Also adds the new cubemap VTF filename to s_DefaultCubemapNames so it can copy the
// default (skybox) cubemap into this file so the cubemap doesn't have the pink checkerboard at
// runtime before they run buildcubemaps.
//-----------------------------------------------------------------------------
static int Cubemap_CreateTexInfo( int originalTexInfo, int origin[3] )
{
	// Don't make cubemap tex infos for nodes
	if ( originalTexInfo == TEXINFO_NODE )
		return originalTexInfo;

	texinfo_t *pTexInfo = &texinfo[originalTexInfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	const char *pMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );
	if ( g_IsCubemapTexData[pTexInfo->texdata] )
	{
		Warning("Multiple references for cubemap on texture %s!!!\n", pMaterialName );
		return originalTexInfo;
	}

	// Get out of here if the originalTexInfo is already a generated material for this position.
	char pStringToSearchFor[512];
	Q_snprintf( pStringToSearchFor, 512, "_%d_%d_%d", origin[0], origin[1], origin[2] );
	if ( Q_stristr( pMaterialName, pStringToSearchFor ) )
		return originalTexInfo;

	// Package up information needed to generate patch names
	PatchInfo_t info;
	info.m_pMapName = mapbase;
	info.m_pOrigin[0] = origin[0];
	info.m_pOrigin[1] = origin[1];
	info.m_pOrigin[2] = origin[2];

	// Generate the name of the patched material
	char pGeneratedTexDataName[1024];
	GeneratePatchedName( pMaterialName, info, true, pGeneratedTexDataName, 1024 );

	// Make sure the texdata doesn't already exist.
	int nTexDataID = FindTexData( pGeneratedTexDataName );
	bool bHasTexData = (nTexDataID != -1);
	if( !bHasTexData )
	{
		// Generate the new "$envmap" texture name.
		char pTextureName[1024];
		GeneratePatchedName( "c", info, false, pTextureName, 1024 );

		// Hook the texture into the material and all dependent materials
		// but if no hooking was necessary, exit out
		if ( !PatchEnvmapForMaterialAndDependents( pMaterialName, info, pTextureName ) )
			return originalTexInfo;
		
		// Store off the name of the cubemap that we need to create since we successfully patched
		char pFileName[1024];
		int nLen = Q_snprintf( pFileName, 1024, "materials/%s.vtf", pTextureName );
		int id = s_DefaultCubemapNames.AddToTail();
		s_DefaultCubemapNames[id] = new char[ nLen + 1 ];
		strcpy( s_DefaultCubemapNames[id], pFileName );

		// Make a new texdata
		nTexDataID = AddCloneTexData( pTexData, pGeneratedTexDataName );
		g_IsCubemapTexData[nTexDataID] = true;
	}

	Assert( nTexDataID != -1 );
	
	texinfo_t newTexInfo;
	newTexInfo = *pTexInfo;
	newTexInfo.texdata = nTexDataID;
	
	int nTexInfoID = -1;

	// See if we need to make a new texinfo
	bool bHasTexInfo = false;
	if( bHasTexData )
	{
		nTexInfoID = FindTexInfo( newTexInfo );
		bHasTexInfo = (nTexInfoID != -1);
	}
	
	// Make a new texinfo if we need to.
	if( !bHasTexInfo )
	{
		nTexInfoID = texinfo.AddToTail( newTexInfo );
	}

	Assert( nTexInfoID != -1 );
	return nTexInfoID;
}

static int SideIDToIndex( int brushSideID )
{
	int i;
	for( i = 0; i < g_MainMap->nummapbrushsides; i++ )
	{
		if( g_MainMap->brushsides[i].id == brushSideID )
		{
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Goes through s_EnvCubemapToBrushSides and does Cubemap_CreateTexInfo for each
// side referenced by an env_cubemap manually.
//-----------------------------------------------------------------------------
void Cubemap_FixupBrushSidesMaterials( void )
{
	Msg( "fixing up env_cubemap materials on brush sides...\n" );
	Assert( s_EnvCubemapToBrushSides.Count() == g_nCubemapSamples );

	int cubemapID;
	for( cubemapID = 0; cubemapID < g_nCubemapSamples; cubemapID++ )
	{
		IntVector_t &brushSidesVector = s_EnvCubemapToBrushSides[cubemapID];
		int i;
		for( i = 0; i < brushSidesVector.Count(); i++ )
		{
			int brushSideID = brushSidesVector[i];
			int sideIndex = SideIDToIndex( brushSideID );
			if( sideIndex < 0 )
			{
				Warning("env_cubemap pointing at deleted brushside near (%d, %d, %d)\n", 
					g_CubemapSamples[cubemapID].origin[0], g_CubemapSamples[cubemapID].origin[1], g_CubemapSamples[cubemapID].origin[2] );

				continue;
			}
			
			side_t *pSide = &g_MainMap->brushsides[sideIndex];

#ifdef DEBUG
			if ( pSide->pMapDisp )
			{
				Assert( pSide->texinfo == pSide->pMapDisp->face.texinfo );
			}
#endif
			
			pSide->texinfo = Cubemap_CreateTexInfo( pSide->texinfo, g_CubemapSamples[cubemapID].origin );
			if ( pSide->pMapDisp )
			{
				pSide->pMapDisp->face.texinfo = pSide->texinfo;
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Cubemap_ResetCubemapSideData( void )
{
	for ( int iSide = 0; iSide < MAX_MAP_BRUSHSIDES; ++iSide )
	{
		s_aCubemapSideData[iSide].bHasEnvMapInMaterial = false;
		s_aCubemapSideData[iSide].bManuallyPickedByAnEnvCubemap = false;
	}
}


//-----------------------------------------------------------------------------
// Returns true if the material or any of its dependents use an $envmap
//-----------------------------------------------------------------------------
bool DoesMaterialOrDependentsUseEnvmap( const char *pPatchedMaterialName )
{
	const char *pOriginalMaterialName = GetOriginalMaterialNameForPatchedMaterial( pPatchedMaterialName );
	if( DoesMaterialHaveKey( pOriginalMaterialName, "$envmap" ) )
		return true;

	const char *pDependentMaterial = FindDependentMaterial( pOriginalMaterialName );
	if ( !pDependentMaterial )
		return false;

	return DoesMaterialOrDependentsUseEnvmap( pDependentMaterial );
}


//-----------------------------------------------------------------------------
// Builds a list of all texdatas which need fixing up
//-----------------------------------------------------------------------------
void Cubemap_InitCubemapSideData( void )
{
	// This tree is used to prevent re-parsing material vars multiple times
	CUtlRBTree<CubemapInfo_t> lookup( 0, g_MainMap->nummapbrushsides, CubemapLessFunc );

	// Fill in specular data.
	for ( int iSide = 0; iSide < g_MainMap->nummapbrushsides; ++iSide )
	{
		side_t *pSide = &g_MainMap->brushsides[iSide];
		if ( !pSide )
			continue;

		if ( pSide->texinfo == TEXINFO_NODE )
			continue;

		texinfo_t *pTex = &texinfo[pSide->texinfo];
		if ( !pTex )
			continue;

		dtexdata_t *pTexData = GetTexData( pTex->texdata );
		if ( !pTexData )
			continue;

		CubemapInfo_t info;
		info.m_nTableId = pTexData->nameStringTableID;

		// Have we encountered this materal? If so, then copy the data we cached off before
		int i = lookup.Find( info );
		if ( i != lookup.InvalidIndex() )
		{
			s_aCubemapSideData[iSide].bHasEnvMapInMaterial = lookup[i].m_bSpecular;
			continue;
		}

		// First time we've seen this material. Figure out if it uses env_cubemap
		const char *pPatchedMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );
		info.m_bSpecular = DoesMaterialOrDependentsUseEnvmap( pPatchedMaterialName );
		s_aCubemapSideData[ iSide ].bHasEnvMapInMaterial = info.m_bSpecular;
		lookup.Insert( info );
	}

	// Fill in cube map data.
	for ( int iCubemap = 0; iCubemap < g_nCubemapSamples; ++iCubemap )
	{
		IntVector_t &sideList = s_EnvCubemapToBrushSides[iCubemap];
		int nSideCount = sideList.Count();
		for ( int iSide = 0; iSide < nSideCount; ++iSide )
		{
			int nSideID = sideList[iSide];
			int nIndex = SideIDToIndex( nSideID );
			if ( nIndex < 0 )
				continue;

			s_aCubemapSideData[nIndex].bManuallyPickedByAnEnvCubemap = true;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int Cubemap_FindClosestCubemap( const Vector &entityOrigin, side_t *pSide )
{
	if ( !pSide )
		return -1;

	// Return a valid (if random) cubemap if there's no winding
	if ( !pSide->winding )
		return 0;

	// Calculate the center point.
	Vector vecCenter;
	vecCenter.Init();

	for ( int iPoint = 0; iPoint < pSide->winding->numpoints; ++iPoint )
	{
		VectorAdd( vecCenter, pSide->winding->p[iPoint], vecCenter );
	}
	VectorScale( vecCenter, 1.0f / pSide->winding->numpoints, vecCenter );
	vecCenter += entityOrigin;
	plane_t *pPlane = &g_MainMap->mapplanes[pSide->planenum];

	// Find the closest cubemap.
	int iMinCubemap = -1;
	float flMinDist = FLT_MAX;

	// Look for cubemaps in front of the surface first.
	for ( int iCubemap = 0; iCubemap < g_nCubemapSamples; ++iCubemap )
	{	
		dcubemapsample_t *pSample = &g_CubemapSamples[iCubemap];
		Vector vecSampleOrigin( static_cast<float>( pSample->origin[0] ),
								static_cast<float>( pSample->origin[1] ),
								static_cast<float>( pSample->origin[2] ) );
		Vector vecDelta;
		VectorSubtract( vecSampleOrigin, vecCenter, vecDelta );
		float flDist = vecDelta.NormalizeInPlace();
		float flDot = DotProduct( vecDelta, pPlane->normal );
		if ( ( flDot >= 0.0f ) && ( flDist < flMinDist ) )
		{
			flMinDist = flDist;
			iMinCubemap = iCubemap;
		}
	}

	// Didn't find anything in front search for closest.
	if( iMinCubemap == -1 )
	{
		for ( int iCubemap = 0; iCubemap < g_nCubemapSamples; ++iCubemap )
		{	
			dcubemapsample_t *pSample = &g_CubemapSamples[iCubemap];
			Vector vecSampleOrigin( static_cast<float>( pSample->origin[0] ),
				static_cast<float>( pSample->origin[1] ),
				static_cast<float>( pSample->origin[2] ) );
			Vector vecDelta;
			VectorSubtract( vecSampleOrigin, vecCenter, vecDelta );
			float flDist = vecDelta.Length();
			if ( flDist < flMinDist )
			{
				flMinDist = flDist;
				iMinCubemap = iCubemap;
			}
		}
	}

	return iMinCubemap;
}


//-----------------------------------------------------------------------------
// For every specular surface that wasn't referenced by some env_cubemap, call Cubemap_CreateTexInfo.
//-----------------------------------------------------------------------------
void Cubemap_AttachDefaultCubemapToSpecularSides( void )
{
	Cubemap_ResetCubemapSideData();
	Cubemap_InitCubemapSideData();

	// build a mapping from side to entity id so that we can get the entity origin
	CUtlVector<int> sideToEntityIndex;
	sideToEntityIndex.SetCount(g_MainMap->nummapbrushsides);
	int i;
	for ( i = 0; i < g_MainMap->nummapbrushsides; i++ )
	{
		sideToEntityIndex[i] = -1;
	}

	for ( i = 0; i < g_MainMap->nummapbrushes; i++ )
	{
		int entityIndex = g_MainMap->mapbrushes[i].entitynum;
		for ( int j = 0; j < g_MainMap->mapbrushes[i].numsides; j++ )
		{
			side_t *side = &g_MainMap->mapbrushes[i].original_sides[j];
			int sideIndex = side - g_MainMap->brushsides;
			sideToEntityIndex[sideIndex] = entityIndex;
		}
	}

	for ( int iSide = 0; iSide < g_MainMap->nummapbrushsides; ++iSide )
	{
		side_t *pSide = &g_MainMap->brushsides[iSide];
		if ( !SideHasCubemapAndWasntManuallyReferenced( iSide ) )
			continue;


		int currentEntity = sideToEntityIndex[iSide];

		int iCubemap = Cubemap_FindClosestCubemap( g_MainMap->entities[currentEntity].origin, pSide );
		if ( iCubemap == -1 )
			continue;

#ifdef DEBUG
		if ( pSide->pMapDisp )
		{
			Assert( pSide->texinfo == pSide->pMapDisp->face.texinfo );
		}
#endif				
		pSide->texinfo = Cubemap_CreateTexInfo( pSide->texinfo, g_CubemapSamples[iCubemap].origin );
		if ( pSide->pMapDisp )
		{
			pSide->pMapDisp->face.texinfo = pSide->texinfo;
		}
	}
}

// Populate with cubemaps that were skipped
void Cubemap_AddUnreferencedCubemaps()
{
	char				pTextureName[1024];
	char				pFileName[1024];
	PatchInfo_t			info;
	dcubemapsample_t	*pSample;
	int					i,j;

	for ( i=0; i<g_nCubemapSamples; ++i )
	{
		pSample = &g_CubemapSamples[i];	

		// generate the formatted texture name based on cubemap origin
		info.m_pMapName   = mapbase;
		info.m_pOrigin[0] = pSample->origin[0];
		info.m_pOrigin[1] = pSample->origin[1];
		info.m_pOrigin[2] = pSample->origin[2];
		GeneratePatchedName( "c", info, false, pTextureName, 1024 );
		
		// find or add
		for ( j=0; j<s_DefaultCubemapNames.Count(); ++j )
		{
			if ( !stricmp( s_DefaultCubemapNames[j], pTextureName ) )
			{
				// already added
				break;
			}
		}
		if ( j == s_DefaultCubemapNames.Count() )
		{
			int nLen = Q_snprintf( pFileName, 1024, "materials/%s.vtf", pTextureName );

			int id = s_DefaultCubemapNames.AddToTail();
			s_DefaultCubemapNames[id] = new char[nLen + 1];
			strcpy( s_DefaultCubemapNames[id], pFileName );
		}
	}
}
