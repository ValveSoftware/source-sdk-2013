//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"
#include "utilmatlib.h"
#include "physdll.h"
#include <assert.h>
#include <malloc.h>
#include "tier1/strtools.h"
#include "materialpatch.h"
#include "KeyValues.h"

void LoadSurfaceProperties( void );

IPhysicsSurfaceProps *physprops = NULL;

int		nummiptex;
textureref_t	textureref[MAX_MAP_TEXTURES];

bool g_bHasWater = false;

extern qboolean onlyents;

dtexdata_t *GetTexData( int index )
{
	if ( index < 0 )
		return NULL;
	Assert( !onlyents );
	return &dtexdata[ index ];
}

static qboolean StringIsTrue( const char *str )
{
	if( Q_strcasecmp( str, "true" ) == 0 )
	{
		return true;
	}
	if( Q_strcasecmp( str, "1" ) == 0 )
	{
		return true;
	}
	return false;
}

int	FindMiptex (const char *name)
{
	int		i;
	MaterialSystemMaterial_t matID;
	const char *propVal, *propVal2;
	int opacity;
	bool found;
		
	for (i=0 ; i<nummiptex ; i++)
	{
		if (!strcmp (name, textureref[i].name))
		{
			return i;
		}
	}
	if (nummiptex == MAX_MAP_TEXTURES)
		Error ("Too many unique textures, max %d", MAX_MAP_TEXTURES);
	strcpy (textureref[i].name, name);

	textureref[i].lightmapWorldUnitsPerLuxel = 0.0f;
	textureref[i].flags = 0;
	textureref[i].contents = 0;

	matID = FindOriginalMaterial( name, &found );
	if( matID == MATERIAL_NOT_FOUND )
	{
		return 0;
	}

	if (!found)
		Warning("Material not found!: %s\n", name );

	// HANDLE ALL OF THE STUFF THAT ISN'T RENDERED WITH THE MATERIAL THAT IS ONE IT.
	
	// handle sky
	if( ( propVal = GetMaterialVar( matID, "%compileSky" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= SURF_SKY | SURF_NOLIGHT;
	}
	else if( ( propVal = GetMaterialVar( matID, "%compile2DSky" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= SURF_SKY | SURF_SKY2D | SURF_NOLIGHT;
	}
	// handle hint brushes
	else if ( ( propVal = GetMaterialVar( matID, "%compileHint" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT | SURF_HINT;
	}
	// handle skip faces
	else if ( ( propVal = GetMaterialVar( matID, "%compileSkip" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT | SURF_SKIP;
	}
	// handle origin brushes
	else if ( ( propVal = GetMaterialVar( matID, "%compileOrigin" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].contents |= CONTENTS_ORIGIN | CONTENTS_DETAIL;
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
	}
	// handle clip brushes
	else if ( ( propVal = GetMaterialVar( matID, "%compileClip" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].contents |= CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP;
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
	}
	else if ( ( propVal = GetMaterialVar( matID, "%playerClip" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].contents |= CONTENTS_PLAYERCLIP;
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
	}
	// handle npc clip brushes
	else if ( ( propVal = GetMaterialVar( matID, "%compileNpcClip" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].contents |= CONTENTS_MONSTERCLIP;
		textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
	}
	// handle surface lights which are meant to 
	else if ( ( propVal = GetMaterialVar( matID, "%compileNoChop" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= SURF_NOCHOP;
	}
	// handle triggers
	else if ( ( propVal = GetMaterialVar( matID, "%compileTrigger" ) ) &&
		StringIsTrue( propVal ) )
	{
		textureref[i].flags |= ( SURF_NOLIGHT | SURF_TRIGGER );
		if ( g_NodrawTriggers )
		{
			textureref[i].flags |= SURF_NODRAW;
		}
	}
	// handle nolight surfs (except water)
	else if ( (( propVal = GetMaterialVar( matID, "%compileNoLight" ) ) && StringIsTrue( propVal )) && 
		!(( propVal2 = GetMaterialVar( matID, "%compileWater" ) ) && StringIsTrue( propVal2 ) ) )
	{
		textureref[i].flags |= SURF_NOLIGHT;
	}
	else
	{
		// HANDLE ALL OF THE STUFF THAT IS RENDERED WITH THE MATERIAL THAT IS ON IT.

		// Handle ladders.
		if ( ( propVal = GetMaterialVar( matID, "%compileLadder" ) ) &&	StringIsTrue( propVal ) )
		{
			textureref[i].contents |= CONTENTS_LADDER;
		}

		// handle wet materials
		if ( ( propVal = GetMaterialVar( matID, "%noPortal" ) ) &&
			StringIsTrue( propVal ) )
		{
			textureref[i].flags |= SURF_NOPORTAL;
		}

		if ( ( propVal = GetMaterialVar( matID, "%compilePassBullets" ) ) && StringIsTrue( propVal ) )
		{
			// change contents to grate, so bullets pass through
			// NOTE: This has effects on visibility too!
			textureref[i].contents &= ~CONTENTS_SOLID;
			textureref[i].contents |= CONTENTS_GRATE;
		}

		if( g_BumpAll || GetMaterialShaderPropertyBool( matID, UTILMATLIB_NEEDS_BUMPED_LIGHTMAPS ) )
		{
			textureref[i].flags |= SURF_BUMPLIGHT;
		}
		
		if( GetMaterialShaderPropertyBool( matID, UTILMATLIB_NEEDS_LIGHTMAP ) )
		{
			textureref[i].flags &= ~SURF_NOLIGHT;
		}
		else if( !g_bLightIfMissing )
		{
			textureref[i].flags |= SURF_NOLIGHT;
		}
		// handle nodraw faces/brushes
		if ( ( propVal = GetMaterialVar( matID, "%compileNoDraw" ) ) && StringIsTrue( propVal ) )
		{								    
			//		textureref[i].contents |= CONTENTS_DETAIL;
			textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
		}

		// Just a combination of nodraw + pass bullets, makes things easier
		if ( ( propVal = GetMaterialVar( matID, "%compileInvisible" ) ) && StringIsTrue( propVal ) )
		{								    
			// change contents to grate, so bullets pass through
			// NOTE: This has effects on visibility too!
			textureref[i].contents &= ~CONTENTS_SOLID;
			textureref[i].contents |= CONTENTS_GRATE;
			textureref[i].flags |= SURF_NODRAW | SURF_NOLIGHT;
		}

		bool checkWindow = true;
		// handle non solid
		if ( ( propVal = GetMaterialVar( matID, "%compileNonsolid" ) ) && StringIsTrue( propVal ) )
		{
			textureref[i].contents = CONTENTS_OPAQUE;
			// Non-Solid can't be a window either!
			checkWindow = false;
		}
		// handle block LOS
		if ( ( propVal = GetMaterialVar( matID, "%compileBlockLOS" ) ) && StringIsTrue( propVal ) )
		{
			textureref[i].contents = CONTENTS_BLOCKLOS;

			// BlockLOS can't be a window either!
			checkWindow = false;
		}

		if ( ( propVal = GetMaterialVar( matID, "%compileDetail" ) ) &&
			StringIsTrue( propVal ) )
		{
			textureref[i].contents |= CONTENTS_DETAIL;
		}

		bool bKeepLighting = ( ( propVal = GetMaterialVar( matID, "%compileKeepLight" ) ) &&
			StringIsTrue( propVal ) );

		// handle materials that want to be treated as water.
		if ( ( propVal = GetMaterialVar( matID, "%compileWater" ) ) &&
			StringIsTrue( propVal ) )
		{
			textureref[i].contents &= ~(CONTENTS_SOLID|CONTENTS_DETAIL);
			textureref[i].contents |= CONTENTS_WATER;
			textureref[i].flags |= SURF_WARP | SURF_NOSHADOWS | SURF_NODECALS;

			if ( g_DisableWaterLighting && !bKeepLighting )
			{
				textureref[i].flags |= SURF_NOLIGHT;
			}

			// Set this so that we can check at the end of the process the presence of a a WaterLODControl entity.
			g_bHasWater = true;
		}
		const char *pShaderName = GetMaterialShaderName(matID);
		if ( !bKeepLighting && !Q_strncasecmp( pShaderName, "water", 5 ) || !Q_strncasecmp( pShaderName, "UnlitGeneric", 12 ) )
		{
			//if ( !(textureref[i].flags & SURF_NOLIGHT) )
			//	Warning("Forcing lit materal %s to nolight\n", name );
			textureref[i].flags |= SURF_NOLIGHT;
		}

		if ( ( propVal = GetMaterialVar( matID, "%compileSlime" ) ) &&
			StringIsTrue( propVal ) )
		{
			textureref[i].contents &= ~(CONTENTS_SOLID|CONTENTS_DETAIL);
			textureref[i].contents |= CONTENTS_SLIME;
			textureref[i].flags |= SURF_NODECALS;
			// Set this so that we can check at the end of the process the presence of a a WaterLODControl entity.
			g_bHasWater = true;
		}
	
		opacity = GetMaterialShaderPropertyInt( matID, UTILMATLIB_OPACITY );
		
		if ( checkWindow && opacity != UTILMATLIB_OPAQUE )
		{
			// transparent *and solid* brushes that aren't grates or water must be windows
			if ( !(textureref[i].contents & (CONTENTS_GRATE|CONTENTS_WATER)) )
			{
				textureref[i].contents |= CONTENTS_WINDOW;
			}

			textureref[i].contents &= ~CONTENTS_SOLID;
			
			// this affects engine primitive sorting, SURF_TRANS means sort as a translucent primitive
			if ( opacity == UTILMATLIB_TRANSLUCENT )
			{
				textureref[i].flags |= SURF_TRANS;
			}
			
		}
		if ( textureref[i].flags & SURF_NOLIGHT )
		{
			textureref[i].flags &= ~SURF_BUMPLIGHT;
		}
	}

	nummiptex++;

	return i;
}

/*
==================
textureAxisFromPlane
==================
*/
Vector	baseaxis[18] =
{
	Vector(0,0,1), Vector(1,0,0), Vector(0,-1,0),			// floor
	Vector(0,0,-1), Vector(1,0,0), Vector(0,-1,0),		// ceiling
	Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1),			// west wall
	Vector(-1,0,0), Vector(0,1,0), Vector(0,0,-1),		// east wall
	Vector(0,1,0), Vector(1,0,0), Vector(0,0,-1),			// south wall
	Vector(0,-1,0), Vector(1,0,0), Vector(0,0,-1)			// north wall
};

void TextureAxisFromPlane(plane_t *pln, Vector& xv, Vector& yv)
{
	int		bestaxis;
	vec_t	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
}



int g_SurfaceProperties[MAX_MAP_TEXDATA];


int GetSurfaceProperties( MaterialSystemMaterial_t matID, const char *pMatName )
{
	const char *pPropString = NULL;
	int surfaceIndex = -1;

	if ( physprops )
	{
		pPropString = GetMaterialVar( matID, "$surfaceprop" );
		if ( pPropString )
		{
			surfaceIndex = physprops->GetSurfaceIndex( pPropString );
			if ( surfaceIndex < 0 )
			{
				Msg("Can't find surfaceprop %s for material %s, using default\n", pPropString, pMatName );
				surfaceIndex = physprops->GetSurfaceIndex( pPropString );
				surfaceIndex = physprops->GetSurfaceIndex( "default" );
			}
		}
	}

	return surfaceIndex;
}

int GetSurfaceProperties2( MaterialSystemMaterial_t matID, const char *pMatName )
{
	const char *pPropString = NULL;
	int surfaceIndex = -1;

	if ( physprops )
	{
		pPropString = GetMaterialVar( matID, "$surfaceprop2" );
		if ( pPropString )
		{
			surfaceIndex = physprops->GetSurfaceIndex( pPropString );
			if ( surfaceIndex < 0 )
			{
				Msg("Can't find surfacepropblend %s for material %s, using default\n", pPropString, pMatName );
				surfaceIndex = physprops->GetSurfaceIndex( "default" );
			}
		}
		else
		{
			// No surface property 2.
			return -1;
		}
	}

	return surfaceIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Finds or adds a texdata for the specified name ( same as below except
//   instead of finding the named texture, copies the settings from the passed
//  in sourceTexture. )
// Used for creation of one off .vmt files for water surface textures
// Input  : *pName - texture name
// Output : int index into dtexdata array
//-----------------------------------------------------------------------------
int FindAliasedTexData( const char *pName_, dtexdata_t *sourceTexture )
{
	char *pName = ( char * )_alloca( strlen( pName_ ) + 1 );
	strcpy( pName, pName_ );
	strlwr( pName );
	int i, output;
	bool found;
	dtexdata_t *pTexData;
	MaterialSystemMaterial_t matID;

	for ( i = 0; i < numtexdata; i++ )
	{
		if ( !strcmp( pName, TexDataStringTable_GetString( GetTexData( i )->nameStringTableID ) ) )
			return i;
	}


	output = numtexdata;
	if ( numtexdata >= MAX_MAP_TEXDATA )
	{
		Error( "Too many unique texture mappings, max = %d\n", MAX_MAP_TEXDATA );
	}
	pTexData = GetTexData( output );
	numtexdata++;

	// Save the name of the material.
	pTexData->nameStringTableID = TexDataStringTable_AddOrFindString( pName );

	// Get the width, height, view_width, view_height, and reflectivity from the material system.
	matID = FindOriginalMaterial( TexDataStringTable_GetString( sourceTexture->nameStringTableID ), &found, false );
	if( matID == MATERIAL_NOT_FOUND || (!found) )
	{
		qprintf( "WARNING: material not found: \"%s\"\n", pName );
		return -1;
	}

	GetMaterialDimensions( matID, &pTexData->width, &pTexData->height );
	pTexData->view_width = pTexData->width;  // undone: what is this?
	pTexData->view_height = pTexData->height;  // undone: what is this?
	
	GetMaterialReflectivity( matID, pTexData->reflectivity.Base() );
	g_SurfaceProperties[output] = GetSurfaceProperties( matID, pName );

	return output;
}


//-----------------------------------------------------------------------------
// Finds a texdata for the specified name, returns -1 if not found
//-----------------------------------------------------------------------------
int FindTexData( const char *pName )
{
	// Make sure the texdata doesn't already exist.
	for( int i = 0; i < numtexdata; i++ )
	{
		char const *pTexDataName = TexDataStringTable_GetString( GetTexData( i )->nameStringTableID );
		if ( !Q_stricmp( pTexDataName, pName ) )
			return i;
	}
	return -1;
}



//-----------------------------------------------------------------------------
// Purpose: Finds or adds a texdata for the specified name
// Input  : *pName - texture name
// Output : int index into dtexdata array
//-----------------------------------------------------------------------------
int FindOrCreateTexData( const char *pName_ )
{
	char *pName = ( char * )_alloca( strlen( pName_ ) + 1 );
	strcpy( pName, pName_ );

	int nOutput = FindTexData( pName );
	if ( nOutput >= 0 )
		return nOutput;

	// Didn't find it, add a new one
	nOutput = numtexdata;
	if ( numtexdata >= MAX_MAP_TEXDATA )
	{
		Error( "Too many unique texture mappings, max = %d\n", MAX_MAP_TEXDATA );
	}
	dtexdata_t *pTexData = GetTexData( nOutput );
	numtexdata++;

	// Save the name of the material.
	pTexData->nameStringTableID = TexDataStringTable_AddOrFindString( pName );

	// Get the width, height, view_width, view_height, and reflectivity from the material system.
	bool bFound;
	MaterialSystemMaterial_t matID = FindOriginalMaterial( pName, &bFound );
	if ( matID == MATERIAL_NOT_FOUND || (!bFound) )
	{
		qprintf( "WARNING: material not found: \"%s\"\n", pName );
		return nOutput;
	}

	GetMaterialDimensions( matID, &pTexData->width, &pTexData->height );
	pTexData->view_width = pTexData->width;  // undone: what is this?
	pTexData->view_height = pTexData->height;  // undone: what is this?
	
	GetMaterialReflectivity( matID, pTexData->reflectivity.Base() );
	g_SurfaceProperties[nOutput] = GetSurfaceProperties( matID, pName );

#if 0
	Msg( "reflectivity: %f %f %f\n", 
		pTexData->reflectivity[0],
		pTexData->reflectivity[1],
		pTexData->reflectivity[2] );
#endif

	return nOutput;
}

int AddCloneTexData( dtexdata_t *pExistingTexData, char const *cloneTexDataName )
{
	int existingIndex = pExistingTexData - GetTexData( 0 );
	dtexdata_t *pNewTexData = GetTexData( numtexdata );
	int newIndex = numtexdata;
	numtexdata++;

	*pNewTexData = *pExistingTexData;
	pNewTexData->nameStringTableID = TexDataStringTable_AddOrFindString( cloneTexDataName );
	g_SurfaceProperties[newIndex] = g_SurfaceProperties[existingIndex];

	return newIndex;
}


//-----------------------------------------------------------------------------
// Finds a texinfo that exactly matches the passed in texinfo
//-----------------------------------------------------------------------------
int FindTexInfo( const texinfo_t &searchTexInfo )
{
	for( int i = 0; i < texinfo.Count(); i++ )
	{
		// Just an early-out for performance
		if ( texinfo[i].texdata != searchTexInfo.texdata )
			continue;

		if ( !memcmp( &texinfo[i], &searchTexInfo, sizeof( texinfo_t ) ) )
			return i;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Finds or creates a texinfo that exactly matches the passed in texinfo
//-----------------------------------------------------------------------------
int FindOrCreateTexInfo( const texinfo_t &searchTexInfo )
{
	int i = FindTexInfo( searchTexInfo );
	if ( i >= 0 )
		return i;

	i = texinfo.AddToTail( searchTexInfo );

	if ( onlyents )
	{
		Error( "FindOrCreateTexInfo:  Tried to create new texinfo during -onlyents compile!\nMust compile without -onlyents" );
	}

	return i;
}

int TexinfoForBrushTexture (plane_t *plane, brush_texture_t *bt, const Vector& origin)
{
	Vector	vecs[2];
	int		sv, tv;
	vec_t	ang, sinv, cosv;
	vec_t	ns, nt;
	texinfo_t	tx;
	int		i, j;

	if (!bt->name[0])
		return 0;

	memset (&tx, 0, sizeof(tx));

	// HLTOOLS - add support for texture vectors stored in the map file
	if (g_nMapFileVersion < 220)
	{
		TextureAxisFromPlane(plane, vecs[0], vecs[1]);
	}

	if (!bt->textureWorldUnitsPerTexel[0])
		bt->textureWorldUnitsPerTexel[0] = 1;
	if (!bt->textureWorldUnitsPerTexel[1])
		bt->textureWorldUnitsPerTexel[1] = 1;


	float shiftScaleU = 1.0f / 16.0f;
	float shiftScaleV = 1.0f / 16.0f;

	if (g_nMapFileVersion < 220)
	{
	// rotate axis
		if (bt->rotate == 0)
			{ sinv = 0 ; cosv = 1; }
		else if (bt->rotate == 90)
			{ sinv = 1 ; cosv = 0; }
		else if (bt->rotate == 180)
			{ sinv = 0 ; cosv = -1; }
		else if (bt->rotate == 270)
			{ sinv = -1 ; cosv = 0; }
		else
		{	
			ang = bt->rotate / 180 * M_PI;
			sinv = sin(ang);
			cosv = cos(ang);
		}

		if (vecs[0][0])
			sv = 0;
		else if (vecs[0][1])
			sv = 1;
		else
			sv = 2;
					
		if (vecs[1][0])
			tv = 0;
		else if (vecs[1][1])
			tv = 1;
		else
			tv = 2;
						
		for (i=0 ; i<2 ; i++)
		{
			ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
			nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
			vecs[i][sv] = ns;
			vecs[i][tv] = nt;
		}

		for (i=0 ; i<2 ; i++)
		{
			for (j=0 ; j<3 ; j++)
			{
				tx.textureVecsTexelsPerWorldUnits[i][j] = vecs[i][j] / bt->textureWorldUnitsPerTexel[i];
				tx.lightmapVecsLuxelsPerWorldUnits[i][j] = tx.textureVecsTexelsPerWorldUnits[i][j] / 16.0f;
			}
		}
	}
	else
	{
		tx.textureVecsTexelsPerWorldUnits[0][0] = bt->UAxis[0] / bt->textureWorldUnitsPerTexel[0];
		tx.textureVecsTexelsPerWorldUnits[0][1] = bt->UAxis[1] / bt->textureWorldUnitsPerTexel[0];
		tx.textureVecsTexelsPerWorldUnits[0][2] = bt->UAxis[2] / bt->textureWorldUnitsPerTexel[0];

		tx.textureVecsTexelsPerWorldUnits[1][0] = bt->VAxis[0] / bt->textureWorldUnitsPerTexel[1];
		tx.textureVecsTexelsPerWorldUnits[1][1] = bt->VAxis[1] / bt->textureWorldUnitsPerTexel[1];
		tx.textureVecsTexelsPerWorldUnits[1][2] = bt->VAxis[2] / bt->textureWorldUnitsPerTexel[1];

		tx.lightmapVecsLuxelsPerWorldUnits[0][0] = bt->UAxis[0] / bt->lightmapWorldUnitsPerLuxel;
		tx.lightmapVecsLuxelsPerWorldUnits[0][1] = bt->UAxis[1] / bt->lightmapWorldUnitsPerLuxel;
		tx.lightmapVecsLuxelsPerWorldUnits[0][2] = bt->UAxis[2] / bt->lightmapWorldUnitsPerLuxel;
		
		tx.lightmapVecsLuxelsPerWorldUnits[1][0] = bt->VAxis[0] / bt->lightmapWorldUnitsPerLuxel;
		tx.lightmapVecsLuxelsPerWorldUnits[1][1] = bt->VAxis[1] / bt->lightmapWorldUnitsPerLuxel;
		tx.lightmapVecsLuxelsPerWorldUnits[1][2] = bt->VAxis[2] / bt->lightmapWorldUnitsPerLuxel;

		shiftScaleU = bt->textureWorldUnitsPerTexel[0] / bt->lightmapWorldUnitsPerLuxel;
		shiftScaleV = bt->textureWorldUnitsPerTexel[1] / bt->lightmapWorldUnitsPerLuxel;
	}

	tx.textureVecsTexelsPerWorldUnits[0][3] = bt->shift[0] + 
		DOT_PRODUCT( origin, tx.textureVecsTexelsPerWorldUnits[0] );
	tx.textureVecsTexelsPerWorldUnits[1][3] = bt->shift[1] + 
		DOT_PRODUCT( origin, tx.textureVecsTexelsPerWorldUnits[1] );
	
	tx.lightmapVecsLuxelsPerWorldUnits[0][3] = shiftScaleU * bt->shift[0] +
		DOT_PRODUCT( origin, tx.lightmapVecsLuxelsPerWorldUnits[0] );
	tx.lightmapVecsLuxelsPerWorldUnits[1][3] = shiftScaleV * bt->shift[1] +
		DOT_PRODUCT( origin, tx.lightmapVecsLuxelsPerWorldUnits[1] );
	
	tx.flags = bt->flags;
	tx.texdata = FindOrCreateTexData( bt->name );

	// find the texinfo
	return FindOrCreateTexInfo( tx );
}


void LoadSurfacePropFile( const char *pMaterialFilename )
{
	FileHandle_t fp = g_pFileSystem->Open( pMaterialFilename, "rb" );

	if ( fp == FILESYSTEM_INVALID_HANDLE )
	{
		return;
	}

	int len = g_pFileSystem->Size( fp );

	char *pText = new char[len];
	g_pFileSystem->Read( pText, len, fp );
	g_pFileSystem->Close( fp );

	physprops->ParseSurfaceData( pMaterialFilename, pText );

	delete[] pText;
}
//-----------------------------------------------------------------------------
// Purpose: Loads the surface properties database into the physics DLL
//-----------------------------------------------------------------------------
void LoadSurfaceProperties( void )
{
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( !physicsFactory )
		return;

	physprops = (IPhysicsSurfaceProps *)physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL );

	const char *SURFACEPROP_MANIFEST_FILE = "scripts/surfaceproperties_manifest.txt";
	KeyValues *manifest = new KeyValues( SURFACEPROP_MANIFEST_FILE );
	if ( manifest->LoadFromFile( g_pFileSystem, SURFACEPROP_MANIFEST_FILE, "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				// Add
				LoadSurfacePropFile( sub->GetString() );
				continue;
			}
		}
	}

	manifest->deleteThis();
}


