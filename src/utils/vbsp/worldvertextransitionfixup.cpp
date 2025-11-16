//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "bsplib.h"
#include "vbsp.h"
#include "tier1/UtlBuffer.h"
#include "tier1/utlvector.h"
#include "KeyValues.h"
#include "materialpatch.h"

struct entitySideList_t
{
	int firstBrushSide;
	int brushSideCount;
};

static bool SideIsNotDispAndHasDispMaterial( int iSide )
{
	side_t *pSide = &g_MainMap->brushsides[iSide];

	// If it's a displacement, then it's fine to have a displacement-only material.
	if ( pSide->pMapDisp )
	{
		return false;
	}

	pSide->texinfo;	

	return true;
}

static void BackSlashToForwardSlash( char *pname )
{
	while ( *pname ) {
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}

//-----------------------------------------------------------------------------
// Generate patched material name
//-----------------------------------------------------------------------------
static void GeneratePatchedMaterialName( const char *pMaterialName, char *pBuffer, int nMaxLen )
{
	int nLen = Q_snprintf( pBuffer, nMaxLen, "maps/%s/%s_wvt_patch", mapbase, pMaterialName ); 

	Assert( nLen < TEXTURE_NAME_LENGTH - 1 );
	if ( nLen >= TEXTURE_NAME_LENGTH - 1 )
	{
		Error( "Generated worldvertextransition patch name : %s too long! (max = %d)\n", pBuffer, TEXTURE_NAME_LENGTH );
	}

	BackSlashToForwardSlash( pBuffer );
	Q_strlower( pBuffer );
}

static void RemoveKey( KeyValues *kv, const char *pSubKeyName )
{
	KeyValues *pSubKey = kv->FindKey( pSubKeyName );
	if( pSubKey )
	{
		kv->RemoveSubKey( pSubKey );
		pSubKey->deleteThis();
	}
}

void CreateWorldVertexTransitionPatchedMaterial( const char *pOriginalMaterialName, const char *pPatchedMaterialName )
{
	KeyValues *kv = LoadMaterialKeyValues( pOriginalMaterialName, 0 );
	if( kv )
	{
		// change shader to Lightmappedgeneric (from worldvertextransition*)
		kv->SetName( "LightmappedGeneric" );
		// don't need no stinking $basetexture2 or any other second texture vars
		RemoveKey( kv, "$basetexture2" );
		RemoveKey( kv, "$bumpmap2" );
		RemoveKey( kv, "$bumpframe2" );
		RemoveKey( kv, "$basetexture2noenvmap" );
		RemoveKey( kv, "$blendmodulatetexture" );
		RemoveKey( kv, "$maskedblending" );
		RemoveKey( kv, "$surfaceprop2" );
		// If we didn't want a basetexture on the first texture in the blend, we don't want an envmap at all.
		KeyValues *basetexturenoenvmap = kv->FindKey( "$BASETEXTURENOENVMAP" );
		if( basetexturenoenvmap->GetInt() )
		{
			RemoveKey( kv, "$envmap" );
		}

		Warning( "Patching WVT material: %s\n", pPatchedMaterialName );
		WriteMaterialKeyValuesToPak( pPatchedMaterialName, kv );
	}
}

int CreateBrushVersionOfWorldVertexTransitionMaterial( int originalTexInfo )
{
	// Don't make cubemap tex infos for nodes
	if ( originalTexInfo == TEXINFO_NODE )
		return originalTexInfo;

	texinfo_t *pTexInfo = &texinfo[originalTexInfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	const char *pOriginalMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );

	// Get out of here if the originalTexInfo is already a patched wvt material
	if ( Q_stristr( pOriginalMaterialName, "_wvt_patch" ) )
		return originalTexInfo;

	char patchedMaterialName[1024];
	GeneratePatchedMaterialName( pOriginalMaterialName, patchedMaterialName, 1024 );
//	Warning( "GeneratePatchedMaterialName: %s %s\n", pMaterialName, patchedMaterialName );
	
	// Make sure the texdata doesn't already exist.
	int nTexDataID = FindTexData( patchedMaterialName );
	bool bHasTexData = (nTexDataID != -1);
	if( !bHasTexData )
	{
		// Create the new vmt material file
		CreateWorldVertexTransitionPatchedMaterial( pOriginalMaterialName, patchedMaterialName );

		// Make a new texdata
		nTexDataID = AddCloneTexData( pTexData, patchedMaterialName );
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

const char *GetShaderNameForTexInfo( int iTexInfo )
{
	texinfo_t *pTexInfo = &texinfo[iTexInfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	const char *pMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );
	MaterialSystemMaterial_t hMaterial = FindMaterial( pMaterialName, NULL, false );
	const char *pShaderName = GetMaterialShaderName( hMaterial );
	return pShaderName;
}

void WorldVertexTransitionFixup( void )
{

	CUtlVector<entitySideList_t> sideList;
	sideList.SetCount( g_MainMap->num_entities );
	int i;
	for ( i = 0; i < g_MainMap->num_entities; i++ )
	{
		sideList[i].firstBrushSide = 0;
		sideList[i].brushSideCount = 0;
	}

	for ( i = 0; i < g_MainMap->nummapbrushes; i++ )
	{
		sideList[g_MainMap->mapbrushes[i].entitynum].brushSideCount += g_MainMap->mapbrushes[i].numsides;
	}
	int curSide = 0;
	for ( i = 0; i < g_MainMap->num_entities; i++ )
	{
		sideList[i].firstBrushSide = curSide;
		curSide += sideList[i].brushSideCount;
	}

	int currentEntity = 0;
	for ( int iSide = 0; iSide < g_MainMap->nummapbrushsides; ++iSide )
	{
		side_t *pSide = &g_MainMap->brushsides[iSide];

		// skip displacments
		if ( pSide->pMapDisp )
			continue;

		if( pSide->texinfo < 0 )
			continue;

		const char *pShaderName = GetShaderNameForTexInfo( pSide->texinfo );
		if ( !pShaderName || !Q_stristr( pShaderName, "worldvertextransition" ) )
		{
			continue;
		}

		while ( currentEntity < g_MainMap->num_entities-1 && 
			iSide > sideList[currentEntity].firstBrushSide + sideList[currentEntity].brushSideCount )
		{
			currentEntity++;
		}

		pSide->texinfo = CreateBrushVersionOfWorldVertexTransitionMaterial( pSide->texinfo );
	}
}