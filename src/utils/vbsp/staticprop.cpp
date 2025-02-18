//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Places "detail" objects which are client-only renderable things
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "bsplib.h"
#include "utlvector.h"
#include "bspfile.h"
#include "gamebspfile.h"
#include "VPhysics_Interface.h"
#include "Studio.h"
#include "byteswap.h"
#include "UtlBuffer.h"
#include "CollisionUtils.h"
#include <float.h>
#include "CModel.h"
#include "PhysDll.h"
#include "utlsymbol.h"
#include "tier1/strtools.h"
#include "KeyValues.h"

static void SetCurrentModel( studiohdr_t *pStudioHdr );
static void FreeCurrentModelVertexes();

IPhysicsCollision *s_pPhysCollision = NULL;

//-----------------------------------------------------------------------------
// These puppies are used to construct the game lumps
//-----------------------------------------------------------------------------
static CUtlVector<StaticPropDictLump_t>	s_StaticPropDictLump;
static CUtlVector<StaticPropLump_t>		s_StaticPropLump;
static CUtlVector<StaticPropLeafLump_t>	s_StaticPropLeafLump;


//-----------------------------------------------------------------------------
// Used to build the static prop
//-----------------------------------------------------------------------------
struct StaticPropBuild_t
{
	char const* m_pModelName;
	char const* m_pLightingOrigin;
	Vector	m_Origin;
	QAngle	m_Angles;
	int		m_Solid;
	int		m_Skin;
	int		m_Flags;
	float	m_FadeMinDist;
	float	m_FadeMaxDist;
	bool	m_FadesOut;
	float	m_flForcedFadeScale;
	unsigned short	m_nMinDXLevel;
	unsigned short	m_nMaxDXLevel;
	int		m_LightmapResolutionX;
	int		m_LightmapResolutionY;
};
 

//-----------------------------------------------------------------------------
// Used to cache collision model generation
//-----------------------------------------------------------------------------
struct ModelCollisionLookup_t
{
	CUtlSymbol m_Name;
	CPhysCollide* m_pCollide;
};

static bool ModelLess( ModelCollisionLookup_t const& src1, ModelCollisionLookup_t const& src2 )
{
	return src1.m_Name < src2.m_Name;
}

static CUtlRBTree<ModelCollisionLookup_t, unsigned short>	s_ModelCollisionCache( 0, 32, ModelLess );
static CUtlVector<int>	s_LightingInfo;


//-----------------------------------------------------------------------------
// Gets the keyvalues from a studiohdr
//-----------------------------------------------------------------------------
bool StudioKeyValues( studiohdr_t* pStudioHdr, KeyValues *pValue )
{
	if ( !pStudioHdr )
		return false;

	return pValue->LoadFromBuffer( pStudioHdr->pszName(), pStudioHdr->KeyValueText() );
}


//-----------------------------------------------------------------------------
// Makes sure the studio model is a static prop
//-----------------------------------------------------------------------------
enum isstaticprop_ret
{
	RET_VALID,
	RET_FAIL_NOT_MARKED_STATIC_PROP,
	RET_FAIL_DYNAMIC,
};

isstaticprop_ret IsStaticProp( studiohdr_t* pHdr )
{
	if (!(pHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP))
		return RET_FAIL_NOT_MARKED_STATIC_PROP;

	// If it's got a propdata section in the model's keyvalues, it's not allowed to be a prop_static
	KeyValues *modelKeyValues = new KeyValues(pHdr->pszName());
	if ( StudioKeyValues( pHdr, modelKeyValues ) )
	{
		KeyValues *sub = modelKeyValues->FindKey("prop_data");
		if ( sub )
		{
			if ( !(sub->GetInt( "allowstatic", 0 )) )
			{
				modelKeyValues->deleteThis();
				return RET_FAIL_DYNAMIC;
			}
		}
	}
	modelKeyValues->deleteThis();

	return RET_VALID;
}


//-----------------------------------------------------------------------------
// Add static prop model to the list of models
//-----------------------------------------------------------------------------

static int AddStaticPropDictLump( char const* pModelName )
{
	StaticPropDictLump_t dictLump;
	strncpy( dictLump.m_Name, pModelName, DETAIL_NAME_LENGTH );

	for (int i = s_StaticPropDictLump.Size(); --i >= 0; )
	{
		if (!memcmp(&s_StaticPropDictLump[i], &dictLump, sizeof(dictLump) ))
			return i;
	}

	return s_StaticPropDictLump.AddToTail( dictLump );
}


//-----------------------------------------------------------------------------
// Load studio model vertex data from a file...
//-----------------------------------------------------------------------------
bool LoadStudioModel( char const* pModelName, char const* pEntityType, CUtlBuffer& buf )
{
	if ( !g_pFullFileSystem->ReadFile( pModelName, NULL, buf ) )
		return false;

	// Check that it's valid
	if (strncmp ((const char *) buf.PeekGet(), "IDST", 4) &&
		strncmp ((const char *) buf.PeekGet(), "IDAG", 4))
	{
		return false;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.PeekGet();

	Studio_ConvertStudioHdrToNewVersion( pHdr );

	if (pHdr->version != STUDIO_VERSION)
	{
		return false;
	}

	isstaticprop_ret isStaticProp = IsStaticProp(pHdr);
	if ( isStaticProp != RET_VALID )
	{
		if ( isStaticProp == RET_FAIL_NOT_MARKED_STATIC_PROP )
		{
			Warning("Error! To use model \"%s\"\n"
				"      with %s, it must be compiled with $staticprop!\n", pModelName, pEntityType );
		}
		else if ( isStaticProp == RET_FAIL_DYNAMIC )
		{
			Warning("Error! %s using model \"%s\", which must be used on a dynamic entity (i.e. prop_physics). Deleted.\n", pEntityType, pModelName );
		}
		return false;
	}

	// ensure reset
	pHdr->pVertexBase = NULL;
	pHdr->pIndexBase  = NULL;

	return true;
}


//-----------------------------------------------------------------------------
// Computes a convex hull from a studio mesh
//-----------------------------------------------------------------------------
static CPhysConvex* ComputeConvexHull( mstudiomesh_t* pMesh )
{
	// Generate a list of all verts in the mesh
	Vector** ppVerts = (Vector**)stackalloc(pMesh->numvertices * sizeof(Vector*) );
	const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData();
	Assert( vertData ); // This can only return NULL on X360 for now
	for (int i = 0; i < pMesh->numvertices; ++i)
	{
		ppVerts[i] = vertData->Position(i);
	}

	// Generate a convex hull from the verts
	return s_pPhysCollision->ConvexFromVerts( ppVerts, pMesh->numvertices );
}


//-----------------------------------------------------------------------------
// Computes a convex hull from the studio model
//-----------------------------------------------------------------------------
CPhysCollide* ComputeConvexHull( studiohdr_t* pStudioHdr )
{
	CUtlVector<CPhysConvex*>	convexHulls;

	for (int body = 0; body < pStudioHdr->numbodyparts; ++body )
	{
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( body );
		for( int model = 0; model < pBodyPart->nummodels; ++model )
		{
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( model );
			for( int mesh = 0; mesh < pStudioModel->nummeshes; ++mesh )
			{
				// Make a convex hull for each mesh
				// NOTE: This won't work unless the model has been compiled
				// with $staticprop
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( mesh );
				convexHulls.AddToTail( ComputeConvexHull( pStudioMesh ) );
			}
		}
	}

	// Convert an array of convex elements to a compiled collision model
	// (this deletes the convex elements)
	return s_pPhysCollision->ConvertConvexToCollide( convexHulls.Base(), convexHulls.Size() );
}


//-----------------------------------------------------------------------------
// Add, find collision model in cache
//-----------------------------------------------------------------------------
static CPhysCollide* GetCollisionModel( char const* pModelName )
{
	// Convert to a common string
	char* pTemp = (char*)_alloca(strlen(pModelName) + 1);
	strcpy( pTemp, pModelName );
	_strlwr( pTemp );

	char* pSlash = strchr( pTemp, '\\' );
	while( pSlash )
	{
		*pSlash = '/';
		pSlash = strchr( pTemp, '\\' );
	}

	// Find it in the cache
	ModelCollisionLookup_t lookup;
	lookup.m_Name = pTemp;
	int i = s_ModelCollisionCache.Find( lookup );
	if (i != s_ModelCollisionCache.InvalidIndex())
		return s_ModelCollisionCache[i].m_pCollide;

	// Load the studio model file
	CUtlBuffer buf;
	if (!LoadStudioModel(pModelName, "prop_static", buf))
	{
		Warning("Error loading studio model \"%s\"!\n", pModelName );

		// This way we don't try to load it multiple times
		lookup.m_pCollide = 0;
		s_ModelCollisionCache.Insert( lookup );

		return 0;
	}

	// Compute the convex hull of the model...
	studiohdr_t* pStudioHdr = (studiohdr_t*)buf.PeekGet();

	// necessary for vertex access
	SetCurrentModel( pStudioHdr );

	lookup.m_pCollide = ComputeConvexHull( pStudioHdr );
	s_ModelCollisionCache.Insert( lookup );

	if ( !lookup.m_pCollide )
	{
		Warning("Bad geometry on \"%s\"!\n", pModelName );
	}

	// Debugging
	if (g_DumpStaticProps)
	{
		static int propNum = 0;
		char tmp[128];
		sprintf( tmp, "staticprop%03d.txt", propNum );
		DumpCollideToGlView( lookup.m_pCollide, tmp );
		++propNum;
	}

	FreeCurrentModelVertexes();

	// Insert into cache...
	return lookup.m_pCollide;
}


//-----------------------------------------------------------------------------
// Tests a single leaf against the static prop
//-----------------------------------------------------------------------------

static bool TestLeafAgainstCollide( int depth, int* pNodeList, 
	Vector const& origin, QAngle const& angles, CPhysCollide* pCollide )
{
	// Copy the planes in the node list into a list of planes
	float* pPlanes = (float*)_alloca(depth * 4 * sizeof(float) );
	int idx = 0;
	for (int i = depth; --i >= 0; ++idx )
	{
		int sign = (pNodeList[i] < 0) ? -1 : 1;
		int node = (sign < 0) ? - pNodeList[i] - 1 : pNodeList[i];
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		pPlanes[idx*4] = sign * pPlane->normal[0];
		pPlanes[idx*4+1] = sign * pPlane->normal[1];
		pPlanes[idx*4+2] = sign * pPlane->normal[2];
		pPlanes[idx*4+3] = sign * pPlane->dist;
	}

	// Make a convex solid out of the planes
	CPhysConvex* pPhysConvex = s_pPhysCollision->ConvexFromPlanes( pPlanes, depth, 0.0f );

	// This should never happen, but if it does, return no collision
	Assert( pPhysConvex );
	if (!pPhysConvex)
		return false;

	CPhysCollide* pLeafCollide = s_pPhysCollision->ConvertConvexToCollide( &pPhysConvex, 1 );

	// Collide the leaf solid with the static prop solid
	trace_t	tr;
	s_pPhysCollision->TraceCollide( vec3_origin, vec3_origin, pLeafCollide, vec3_angle,
		pCollide, origin, angles, &tr );

	s_pPhysCollision->DestroyCollide( pLeafCollide );

	return (tr.startsolid != 0);
}

//-----------------------------------------------------------------------------
// Find all leaves that intersect with this bbox + test against the static prop..
//-----------------------------------------------------------------------------

static void ComputeConvexHullLeaves_R( int node, int depth, int* pNodeList,
	Vector const& mins, Vector const& maxs,
	Vector const& origin, QAngle const& angles,	CPhysCollide* pCollide,
	CUtlVector<unsigned short>& leafList )
{
	Assert( pNodeList && pCollide );
	Vector cornermin, cornermax;

	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		// Arbitrary split plane here
		for (int i = 0; i < 3; ++i)
		{
			if (pPlane->normal[i] >= 0)
			{
				cornermin[i] = mins[i];
				cornermax[i] = maxs[i];
			}
			else
			{
				cornermin[i] = maxs[i];
				cornermax[i] = mins[i];
			}
		}

		if (DotProduct( pPlane->normal, cornermax ) <= pPlane->dist)
		{
			// Add the node to the list of nodes
			pNodeList[depth] = node;
			++depth;

			node = pNode->children[1];
		}
		else if (DotProduct( pPlane->normal, cornermin ) >= pPlane->dist)
		{
			// In this case, we are going in front of the plane. That means that
			// this plane must have an outward normal facing in the oppisite direction
			// We indicate this be storing a negative node index in the node list
			pNodeList[depth] = - node - 1;
			++depth;

			node = pNode->children[0];
		}
		else
		{
			// Here the box is split by the node. First, we'll add the plane as if its
			// outward facing normal is in the direction of the node plane, then
			// we'll have to reverse it for the other child...
			pNodeList[depth] = node;
			++depth;

			ComputeConvexHullLeaves_R( pNode->children[1], 
				depth, pNodeList, mins, maxs, origin, angles, pCollide, leafList );
			
			pNodeList[depth - 1] = - node - 1;
			ComputeConvexHullLeaves_R( pNode->children[0],
				depth, pNodeList, mins, maxs, origin, angles, pCollide, leafList );
			return;
		}
	}

	Assert( pNodeList && pCollide );

	// Never add static props to solid leaves
	if ( (dleafs[-node-1].contents & CONTENTS_SOLID) == 0 )
	{
		if (TestLeafAgainstCollide( depth, pNodeList, origin, angles, pCollide ))
		{
			leafList.AddToTail( -node - 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------

static void ComputeStaticPropLeaves( CPhysCollide* pCollide, Vector const& origin, 
				QAngle const& angles, CUtlVector<unsigned short>& leafList )
{
	// Compute an axis-aligned bounding box for the collide
	Vector mins, maxs;
	s_pPhysCollision->CollideGetAABB( &mins, &maxs, pCollide, origin, angles );

	// Find all leaves that intersect with the bounds
	int tempNodeList[1024];
	ComputeConvexHullLeaves_R( 0, 0, tempNodeList, mins, maxs,
		origin, angles, pCollide, leafList );
}


//-----------------------------------------------------------------------------
// Computes the lighting origin
//-----------------------------------------------------------------------------
static bool ComputeLightingOrigin( StaticPropBuild_t const& build, Vector& lightingOrigin )
{
	for (int i = s_LightingInfo.Count(); --i >= 0; )
	{
		int entIndex = s_LightingInfo[i];

		// Check against all lighting info entities
		char const* pTargetName = ValueForKey( &entities[entIndex], "targetname" );
		if (!Q_strcmp(pTargetName, build.m_pLightingOrigin))
		{
			GetVectorForKey( &entities[entIndex], "origin", lightingOrigin );
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------
static void AddStaticPropToLump( StaticPropBuild_t const& build )
{
	// Get the collision model
	CPhysCollide* pConvexHull = GetCollisionModel( build.m_pModelName );
	if (!pConvexHull)
		return;

	// Compute the leaves the static prop's convex hull hits
	CUtlVector< unsigned short > leafList;
	ComputeStaticPropLeaves( pConvexHull, build.m_Origin, build.m_Angles, leafList );

	if ( !leafList.Count() )
	{
		Warning( "Static prop %s outside the map (%.2f, %.2f, %.2f)\n", build.m_pModelName, build.m_Origin.x, build.m_Origin.y, build.m_Origin.z );
		return;
	}
	// Insert an element into the lump data...
	int i = s_StaticPropLump.AddToTail( );
	StaticPropLump_t& propLump = s_StaticPropLump[i];
	propLump.m_PropType = AddStaticPropDictLump( build.m_pModelName ); 
	VectorCopy( build.m_Origin, propLump.m_Origin );
	VectorCopy( build.m_Angles, propLump.m_Angles );
	propLump.m_FirstLeaf = s_StaticPropLeafLump.Count();
	propLump.m_LeafCount = leafList.Count();
	propLump.m_Solid = build.m_Solid;
	propLump.m_Skin = build.m_Skin;
	propLump.m_Flags = build.m_Flags;
	if (build.m_FadesOut)
	{
		propLump.m_Flags |= STATIC_PROP_FLAG_FADES;
	}
	propLump.m_FadeMinDist = build.m_FadeMinDist;
	propLump.m_FadeMaxDist = build.m_FadeMaxDist;
	propLump.m_flForcedFadeScale = build.m_flForcedFadeScale;
	propLump.m_nMinDXLevel = build.m_nMinDXLevel;
	propLump.m_nMaxDXLevel = build.m_nMaxDXLevel;
	
	if (build.m_pLightingOrigin && *build.m_pLightingOrigin)
	{
		if (ComputeLightingOrigin( build, propLump.m_LightingOrigin ))
		{
			propLump.m_Flags |= STATIC_PROP_USE_LIGHTING_ORIGIN;
		}
	}

	propLump.m_nLightmapResolutionX = build.m_LightmapResolutionX;
	propLump.m_nLightmapResolutionY = build.m_LightmapResolutionY;

	// Add the leaves to the leaf lump
	for (int j = 0; j < leafList.Size(); ++j)
	{
		StaticPropLeafLump_t insert;
		insert.m_Leaf = leafList[j];
		s_StaticPropLeafLump.AddToTail( insert );
	}

}


//-----------------------------------------------------------------------------
// Places static props in the lump
//-----------------------------------------------------------------------------

static void SetLumpData( )
{
	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle(GAMELUMP_STATIC_PROPS);
	if (handle != g_GameLumps.InvalidGameLump())
		g_GameLumps.DestroyGameLump(handle);

	int dictsize = s_StaticPropDictLump.Size() * sizeof(StaticPropDictLump_t);
	int objsize = s_StaticPropLump.Size() * sizeof(StaticPropLump_t);
	int leafsize = s_StaticPropLeafLump.Size() * sizeof(StaticPropLeafLump_t);
	int size = dictsize + objsize + leafsize + 3 * sizeof(int);

	handle = g_GameLumps.CreateGameLump( GAMELUMP_STATIC_PROPS, size, 0, GAMELUMP_STATIC_PROPS_VERSION );

	// Serialize the data
	CUtlBuffer buf( g_GameLumps.GetGameLump(handle), size );
	buf.PutInt( s_StaticPropDictLump.Size() );
	if (dictsize)
		buf.Put( s_StaticPropDictLump.Base(), dictsize );
	buf.PutInt( s_StaticPropLeafLump.Size() );
	if (leafsize)
		buf.Put( s_StaticPropLeafLump.Base(), leafsize );
	buf.PutInt( s_StaticPropLump.Size() );
	if (objsize)
		buf.Put( s_StaticPropLump.Base(), objsize );
}


//-----------------------------------------------------------------------------
// Places Static Props in the level
//-----------------------------------------------------------------------------

void EmitStaticProps()
{
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( physicsFactory )
	{
		s_pPhysCollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
		if( !s_pPhysCollision )
			return;
	}

	// Generate a list of lighting origins, and strip them out
	int i;
	for ( i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!Q_strcmp(pEntity, "info_lighting"))
		{
			s_LightingInfo.AddToTail(i);
		}
	}

	// Emit specifically specified static props
	for ( i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "static_prop") || !strcmp(pEntity, "prop_static"))
		{
			StaticPropBuild_t build;

			GetVectorForKey( &entities[i], "origin", build.m_Origin );
			GetAnglesForKey( &entities[i], "angles", build.m_Angles );
			build.m_pModelName = ValueForKey( &entities[i], "model" );
			build.m_Solid = IntForKey( &entities[i], "solid" );
			build.m_Skin = IntForKey( &entities[i], "skin" );
			build.m_FadeMaxDist = FloatForKey( &entities[i], "fademaxdist" );
			build.m_Flags = 0;//IntForKey( &entities[i], "spawnflags" ) & STATIC_PROP_WC_MASK;
			if (IntForKey( &entities[i], "ignorenormals" ) == 1)
			{
				build.m_Flags |= STATIC_PROP_IGNORE_NORMALS;
			}
			if (IntForKey( &entities[i], "disableshadows" ) == 1)
			{
				build.m_Flags |= STATIC_PROP_NO_SHADOW;
			}
			if (IntForKey( &entities[i], "disablevertexlighting" ) == 1)
			{
				build.m_Flags |= STATIC_PROP_NO_PER_VERTEX_LIGHTING;
			}
			if (IntForKey( &entities[i], "disableselfshadowing" ) == 1)
			{
				build.m_Flags |= STATIC_PROP_NO_SELF_SHADOWING;
			}

			if (IntForKey( &entities[i], "screenspacefade" ) == 1)
			{
				build.m_Flags |= STATIC_PROP_SCREEN_SPACE_FADE;
			}

			if (IntForKey( &entities[i], "generatelightmaps") == 0)
			{
				build.m_Flags |= STATIC_PROP_NO_PER_TEXEL_LIGHTING;			
				build.m_LightmapResolutionX = 0;
				build.m_LightmapResolutionY = 0;
			}
			else
			{
				build.m_LightmapResolutionX = IntForKey( &entities[i], "lightmapresolutionx" );
				build.m_LightmapResolutionY = IntForKey( &entities[i], "lightmapresolutiony" );
			}

			const char *pKey = ValueForKey( &entities[i], "fadescale" );
			if ( pKey && pKey[0] )
			{
				build.m_flForcedFadeScale = FloatForKey( &entities[i], "fadescale" );
			}
			else
			{
				build.m_flForcedFadeScale = 1;
			}
			build.m_FadesOut = (build.m_FadeMaxDist > 0);
			build.m_pLightingOrigin = ValueForKey( &entities[i], "lightingorigin" );
			if (build.m_FadesOut)
			{			  
				build.m_FadeMinDist = FloatForKey( &entities[i], "fademindist" );
				if (build.m_FadeMinDist < 0)
				{
					build.m_FadeMinDist = build.m_FadeMaxDist; 
				}
			}
			else
			{
				build.m_FadeMinDist = 0;
			}
			build.m_nMinDXLevel = (unsigned short)IntForKey( &entities[i], "mindxlevel" );
			build.m_nMaxDXLevel = (unsigned short)IntForKey( &entities[i], "maxdxlevel" );
			AddStaticPropToLump( build );

			// strip this ent from the .bsp file
			entities[i].epairs = 0;
		}
	}

	// Strip out lighting origins; has to be done here because they are used when
	// static props are made
	for ( i = s_LightingInfo.Count(); --i >= 0; )
	{
		// strip this ent from the .bsp file
		entities[s_LightingInfo[i]].epairs = 0;
	}


	SetLumpData( );
}

static studiohdr_t *g_pActiveStudioHdr;
static void SetCurrentModel( studiohdr_t *pStudioHdr )
{
	// track the correct model
	g_pActiveStudioHdr = pStudioHdr;
}

static void FreeCurrentModelVertexes()
{
	Assert( g_pActiveStudioHdr );

	if ( g_pActiveStudioHdr->pVertexBase )
	{
		free( g_pActiveStudioHdr->pVertexBase );
		g_pActiveStudioHdr->pVertexBase = NULL;
	}
}

const vertexFileHeader_t * mstudiomodel_t::CacheVertexData( void * pModelData )
{
	char				fileName[260];
	FileHandle_t		fileHandle;
	vertexFileHeader_t	*pVvdHdr;

	Assert( pModelData == NULL );
	Assert( g_pActiveStudioHdr );

	if ( g_pActiveStudioHdr->pVertexBase )
	{
		return (vertexFileHeader_t *)g_pActiveStudioHdr->pVertexBase;
	}

	// mandatory callback to make requested data resident
	// load and persist the vertex file
	strcpy( fileName, "models/" );	
	strcat( fileName, g_pActiveStudioHdr->pszName() );
	Q_StripExtension( fileName, fileName, sizeof( fileName ) );
	strcat( fileName, ".vvd" );

	// load the model
	fileHandle = g_pFileSystem->Open( fileName, "rb" );
	if ( !fileHandle )
	{
		Error( "Unable to load vertex data \"%s\"\n", fileName );
	}

	// Get the file size
	int size = g_pFileSystem->Size( fileHandle );
	if (size == 0)
	{
		g_pFileSystem->Close( fileHandle );
		Error( "Bad size for vertex data \"%s\"\n", fileName );
	}

	pVvdHdr = (vertexFileHeader_t *)malloc(size);
	g_pFileSystem->Read( pVvdHdr, size, fileHandle );
	g_pFileSystem->Close( fileHandle );

	// check header
	if (pVvdHdr->id != MODEL_VERTEX_FILE_ID)
	{
		Error("Error Vertex File %s id %d should be %d\n", fileName, pVvdHdr->id, MODEL_VERTEX_FILE_ID);
	}
	if (pVvdHdr->version != MODEL_VERTEX_FILE_VERSION)
	{
		Error("Error Vertex File %s version %d should be %d\n", fileName, pVvdHdr->version, MODEL_VERTEX_FILE_VERSION);
	}
	if (pVvdHdr->checksum != g_pActiveStudioHdr->checksum)
	{
		Error("Error Vertex File %s checksum %d should be %d\n", fileName, pVvdHdr->checksum, g_pActiveStudioHdr->checksum);
	}

	g_pActiveStudioHdr->pVertexBase = (void*)pVvdHdr;
	return pVvdHdr;
}

