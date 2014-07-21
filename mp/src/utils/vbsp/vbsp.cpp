//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: BSP Building tool
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "detail.h"
#include "physdll.h"
#include "utilmatlib.h"
#include "disp_vbsp.h"
#include "writebsp.h"
#include "tier0/icommandline.h"
#include "materialsystem/imaterialsystem.h"
#include "map.h"
#include "tools_minidump.h"
#include "materialsub.h"
#include "loadcmdline.h"
#include "byteswap.h"
#include "worldvertextransitionfixup.h"

extern float		g_maxLightmapDimension;

char		source[1024];
char		mapbase[ 64 ];
char		name[1024];
char		materialPath[1024];

vec_t		microvolume = 1.0;
qboolean	noprune;
qboolean	glview;
qboolean	nodetail;
qboolean	fulldetail;
qboolean	onlyents;
bool		onlyprops;
qboolean	nomerge;
qboolean	nomergewater = false;
qboolean	nowater;
qboolean	nocsg;
qboolean	noweld;
qboolean	noshare;
qboolean	nosubdiv;
qboolean	notjunc;
qboolean	noopt;
qboolean	leaktest;
qboolean	verboseentities;
qboolean	dumpcollide = false;
qboolean	g_bLowPriority = false;
qboolean	g_DumpStaticProps = false;
qboolean	g_bSkyVis = false;			// skybox vis is off by default, toggle this to enable it
bool		g_bLightIfMissing = false;
bool		g_snapAxialPlanes = false;
bool		g_bKeepStaleZip = false;
bool		g_NodrawTriggers = false;
bool		g_DisableWaterLighting = false;
bool		g_bAllowDetailCracks = false;
bool		g_bNoVirtualMesh = false;

float		g_defaultLuxelSize = DEFAULT_LUXEL_SIZE;
float		g_luxelScale = 1.0f;
float		g_minLuxelScale = 1.0f;
bool		g_BumpAll = false;

int			g_nDXLevel = 0; // default dxlevel if you don't specify it on the command-line.
CUtlVector<int> g_SkyAreas;
char		outbase[32];

// HLTOOLS: Introduce these calcs to make the block algorithm proportional to the proper 
// world coordinate extents.  Assumes square spatial constraints.
#define BLOCKS_SIZE		1024
#define BLOCKS_SPACE	(COORD_EXTENT/BLOCKS_SIZE)
#define BLOCKX_OFFSET	((BLOCKS_SPACE/2)+1)
#define BLOCKY_OFFSET	((BLOCKS_SPACE/2)+1)
#define BLOCKS_MIN		(-(BLOCKS_SPACE/2))
#define BLOCKS_MAX		((BLOCKS_SPACE/2)-1)

int			block_xl = BLOCKS_MIN, block_xh = BLOCKS_MAX, block_yl = BLOCKS_MIN, block_yh = BLOCKS_MAX;

int			entity_num;


node_t		*block_nodes[BLOCKS_SPACE+2][BLOCKS_SPACE+2];

//-----------------------------------------------------------------------------
// Assign occluder areas (must happen *after* the world model is processed)
//-----------------------------------------------------------------------------
void AssignOccluderAreas( tree_t *pTree );
static void Compute3DSkyboxAreas( node_t *headnode, CUtlVector<int>& areas );


/*
============
BlockTree

============
*/
node_t	*BlockTree (int xl, int yl, int xh, int yh)
{
	node_t	*node;
	Vector	normal;
	float	dist;
	int		mid;

	if (xl == xh && yl == yh)
	{
		node = block_nodes[xl+BLOCKX_OFFSET][yl+BLOCKY_OFFSET];
		if (!node)
		{	// return an empty leaf
			node = AllocNode ();
			node->planenum = PLANENUM_LEAF;
			node->contents = 0; //CONTENTS_SOLID;
			return node;
		}
		return node;
	}

	// create a seperator along the largest axis
	node = AllocNode ();

	if (xh - xl > yh - yl)
	{	// split x axis
		mid = xl + (xh-xl)/2 + 1;
		normal[0] = 1;
		normal[1] = 0;
		normal[2] = 0;
		dist = mid*BLOCKS_SIZE;
		node->planenum = g_MainMap->FindFloatPlane (normal, dist);
		node->children[0] = BlockTree ( mid, yl, xh, yh);
		node->children[1] = BlockTree ( xl, yl, mid-1, yh);
	}
	else
	{
		mid = yl + (yh-yl)/2 + 1;
		normal[0] = 0;
		normal[1] = 1;
		normal[2] = 0;
		dist = mid*BLOCKS_SIZE;
		node->planenum = g_MainMap->FindFloatPlane (normal, dist);
		node->children[0] = BlockTree ( xl, mid, xh, yh);
		node->children[1] = BlockTree ( xl, yl, xh, mid-1);
	}

	return node;
}

/*
============
ProcessBlock_Thread

============
*/
int			brush_start, brush_end;
void ProcessBlock_Thread (int threadnum, int blocknum)
{
	int		xblock, yblock;
	Vector		mins, maxs;
	bspbrush_t	*brushes;
	tree_t		*tree;
	node_t		*node;

	yblock = block_yl + blocknum / (block_xh-block_xl+1);
	xblock = block_xl + blocknum % (block_xh-block_xl+1);

	qprintf ("############### block %2i,%2i ###############\n", xblock, yblock);

	mins[0] = xblock*BLOCKS_SIZE;
	mins[1] = yblock*BLOCKS_SIZE;
	mins[2] = MIN_COORD_INTEGER;
	maxs[0] = (xblock+1)*BLOCKS_SIZE;
	maxs[1] = (yblock+1)*BLOCKS_SIZE;
	maxs[2] = MAX_COORD_INTEGER;

	// the makelist and chopbrushes could be cached between the passes...
	brushes = MakeBspBrushList (brush_start, brush_end, mins, maxs, NO_DETAIL);
	if (!brushes)
	{
		node = AllocNode ();
		node->planenum = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;
		block_nodes[xblock+BLOCKX_OFFSET][yblock+BLOCKY_OFFSET] = node;
		return;
	}    

	FixupAreaportalWaterBrushes( brushes );
	if (!nocsg)
		brushes = ChopBrushes (brushes);

	tree = BrushBSP (brushes, mins, maxs);
	
	block_nodes[xblock+BLOCKX_OFFSET][yblock+BLOCKY_OFFSET] = tree->headnode;
}


/*
============
ProcessWorldModel

============
*/
void SplitSubdividedFaces( node_t *headnode ); // garymcthack
void ProcessWorldModel (void)
{
	entity_t	*e;
	tree_t		*tree = NULL;
	qboolean	leaked;
	int	optimize;
	int			start;

	e = &entities[entity_num];

	brush_start = e->firstbrush;
	brush_end = brush_start + e->numbrushes;
	leaked = false;

	//
	// perform per-block operations
	//
	if (block_xh * BLOCKS_SIZE > g_MainMap->map_maxs[0])
	{
		block_xh = floor(g_MainMap->map_maxs[0]/BLOCKS_SIZE);
	}
	if ( (block_xl+1) * BLOCKS_SIZE < g_MainMap->map_mins[0])
	{
		block_xl = floor(g_MainMap->map_mins[0]/BLOCKS_SIZE);
	}
	if (block_yh * BLOCKS_SIZE > g_MainMap->map_maxs[1])
	{
		block_yh = floor(g_MainMap->map_maxs[1]/BLOCKS_SIZE);
	}
	if ( (block_yl+1) * BLOCKS_SIZE < g_MainMap->map_mins[1])
	{
		block_yl = floor(g_MainMap->map_mins[1]/BLOCKS_SIZE);
	}

	// HLTOOLS: updated to +/- MAX_COORD_INTEGER ( new world size limits / worldsize.h )
	if (block_xl < BLOCKS_MIN)
	{
		block_xl = BLOCKS_MIN;
	}
	if (block_yl < BLOCKS_MIN)
	{
		block_yl = BLOCKS_MIN;
	}
	if (block_xh > BLOCKS_MAX)
	{
		block_xh = BLOCKS_MAX;
	}
	if (block_yh > BLOCKS_MAX)
	{
		block_yh = BLOCKS_MAX;
	}

	for (optimize = 0 ; optimize <= 1 ; optimize++)
	{
		qprintf ("--------------------------------------------\n");

		RunThreadsOnIndividual ((block_xh-block_xl+1)*(block_yh-block_yl+1),
			!verbose, ProcessBlock_Thread);

		//
		// build the division tree
		// oversizing the blocks guarantees that all the boundaries
		// will also get nodes.
		//

		qprintf ("--------------------------------------------\n");

		tree = AllocTree ();
		tree->headnode = BlockTree (block_xl-1, block_yl-1, block_xh+1, block_yh+1);

		tree->mins[0] = (block_xl)*BLOCKS_SIZE;
		tree->mins[1] = (block_yl)*BLOCKS_SIZE;
		tree->mins[2] = g_MainMap->map_mins[2] - 8;

		tree->maxs[0] = (block_xh+1)*BLOCKS_SIZE;
		tree->maxs[1] = (block_yh+1)*BLOCKS_SIZE;
		tree->maxs[2] = g_MainMap->map_maxs[2] + 8;

		//
		// perform the global operations
		//

		// make the portals/faces by traversing down to each empty leaf
		MakeTreePortals (tree);

		if (FloodEntities (tree))
		{
			// turns everthing outside into solid
			FillOutside (tree->headnode);
		}
		else
		{
			Warning( ("**** leaked ****\n") );
			leaked = true;
			LeakFile (tree);
			if (leaktest)
			{
				Warning( ("--- MAP LEAKED ---\n") );
				exit (0);
			}
		}

		// mark the brush sides that actually turned into faces
		MarkVisibleSides (tree, brush_start, brush_end, NO_DETAIL);
		if (noopt || leaked)
			break;
		if (!optimize)
		{
			// If we are optimizing, free the tree.  Next time we will construct it again, but
			// we'll use the information in MarkVisibleSides() so we'll only split with planes that
			// actually contribute renderable geometry
			FreeTree (tree);
		}
	}

	FloodAreas (tree);

	RemoveAreaPortalBrushes_R( tree->headnode );

	start = Plat_FloatTime();
	Msg("Building Faces...");
	// this turns portals with one solid side into faces
	// it also subdivides each face if necessary to fit max lightmap dimensions
	MakeFaces (tree->headnode);
	Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );

	if (glview)
	{
		WriteGLView (tree, source);
	}

	AssignOccluderAreas( tree );
	Compute3DSkyboxAreas( tree->headnode, g_SkyAreas );
	face_t *pLeafFaceList = NULL;
	if ( !nodetail )
	{
		pLeafFaceList = MergeDetailTree( tree, brush_start, brush_end );
	}

	start = Plat_FloatTime();

	Msg("FixTjuncs...\n");
	
	// This unifies the vertex list for all edges (splits collinear edges to remove t-junctions)
	// It also welds the list of vertices out of each winding/portal and rounds nearly integer verts to integer
	pLeafFaceList = FixTjuncs (tree->headnode, pLeafFaceList);

	// this merges all of the solid nodes that have separating planes
	if (!noprune)
	{
		Msg("PruneNodes...\n");
		PruneNodes (tree->headnode);
	}

//	Msg( "SplitSubdividedFaces...\n" );
//	SplitSubdividedFaces( tree->headnode );

	Msg("WriteBSP...\n");
	WriteBSP (tree->headnode, pLeafFaceList);
	Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );

	if (!leaked)
	{
		WritePortalFile (tree);
	}

	FreeTree( tree );
	FreeLeafFaces( pLeafFaceList );
}

/*
============
ProcessSubModel

============
*/
void ProcessSubModel( )
{
	entity_t	*e;
	int			start, end;
	tree_t		*tree;
	bspbrush_t	*list;
	Vector		mins, maxs;

	e = &entities[entity_num];

	start = e->firstbrush;
	end = start + e->numbrushes;

	mins[0] = mins[1] = mins[2] = MIN_COORD_INTEGER;
	maxs[0] = maxs[1] = maxs[2] = MAX_COORD_INTEGER;
	list = MakeBspBrushList (start, end, mins, maxs, FULL_DETAIL);

	if (!nocsg)
		list = ChopBrushes (list);
	tree = BrushBSP (list, mins, maxs);
	
	// This would wind up crashing the engine because we'd have a negative leaf index in dmodel_t::headnode.
	if ( tree->headnode->planenum == PLANENUM_LEAF )
	{
		const char *pClassName = ValueForKey( e, "classname" );
		const char *pTargetName = ValueForKey( e, "targetname" );
		Error( "bmodel %d has no head node (class '%s', targetname '%s')", entity_num, pClassName, pTargetName );
	}

	MakeTreePortals (tree);
	
#if DEBUG_BRUSHMODEL
	if ( entity_num == DEBUG_BRUSHMODEL )
		WriteGLView( tree, "tree_all" );
#endif

	MarkVisibleSides (tree, start, end, FULL_DETAIL);
	MakeFaces (tree->headnode);

	FixTjuncs( tree->headnode, NULL );
	WriteBSP( tree->headnode, NULL );
	
#if DEBUG_BRUSHMODEL
	if ( entity_num == DEBUG_BRUSHMODEL )
	{
		WriteGLView( tree, "tree_vis" );
		WriteGLViewFaces( tree, "tree_faces" );
	}
#endif

	FreeTree (tree);
}


//-----------------------------------------------------------------------------
// Returns true if the entity is a func_occluder
//-----------------------------------------------------------------------------
bool IsFuncOccluder( int entity_num )
{
	entity_t *mapent = &entities[entity_num];
	const char *pClassName = ValueForKey( mapent, "classname" );
	return (strcmp("func_occluder", pClassName) == 0);
}


//-----------------------------------------------------------------------------
// Computes the area of a brush's occluders
//-----------------------------------------------------------------------------
float ComputeOccluderBrushArea( mapbrush_t *pBrush )
{
	float flArea = 0.0f;
	for ( int j = 0; j < pBrush->numsides; ++j )
	{
		side_t *pSide = &(pBrush->original_sides[j]);

		// Skip nodraw surfaces
		if ( texinfo[pSide->texinfo].flags & SURF_NODRAW )
			continue;

		if ( !pSide->winding )
			continue;

		flArea += WindingArea( pSide->winding );
	}

	return flArea;
}


//-----------------------------------------------------------------------------
// Clips all occluder brushes against each other
//-----------------------------------------------------------------------------
static tree_t *ClipOccluderBrushes( )
{
	// Create a list of all occluder brushes in the level
	CUtlVector< mapbrush_t * > mapBrushes( 1024, 1024 );
	for ( entity_num=0; entity_num < g_MainMap->num_entities; ++entity_num )
	{
		if (!IsFuncOccluder(entity_num))
			continue;

		entity_t *e = &entities[entity_num];
		int end = e->firstbrush + e->numbrushes;
		int i;
		for ( i = e->firstbrush; i < end; ++i )
		{
			mapBrushes.AddToTail( &g_MainMap->mapbrushes[i] );
		}
	}

	int nBrushCount = mapBrushes.Count();
	if ( nBrushCount == 0 )
		return NULL;

	Vector mins, maxs;
	mins[0] = mins[1] = mins[2] = MIN_COORD_INTEGER;
	maxs[0] = maxs[1] = maxs[2] = MAX_COORD_INTEGER;

	bspbrush_t *list = MakeBspBrushList( mapBrushes.Base(), nBrushCount, mins, maxs );

	if (!nocsg)
		list = ChopBrushes (list);
	tree_t *tree = BrushBSP (list, mins, maxs);
	MakeTreePortals (tree);
	MarkVisibleSides (tree, mapBrushes.Base(), nBrushCount);
	MakeFaces( tree->headnode );

	// NOTE: This will output the occluder face vertices + planes
	FixTjuncs( tree->headnode, NULL );

	return tree;
}


//-----------------------------------------------------------------------------
// Generate a list of unique sides in the occluder tree
//-----------------------------------------------------------------------------
static void GenerateOccluderSideList( int nEntity, CUtlVector<side_t*> &occluderSides )
{
	entity_t *e = &entities[nEntity];
	int end = e->firstbrush + e->numbrushes;
	int i, j;
	for ( i = e->firstbrush; i < end; ++i )
	{
		mapbrush_t *mb = &g_MainMap->mapbrushes[i];
		for ( j = 0; j < mb->numsides; ++j )
		{
			occluderSides.AddToTail( &(mb->original_sides[j]) );
		}
	}
}


//-----------------------------------------------------------------------------
// Generate a list of unique faces in the occluder tree
//-----------------------------------------------------------------------------
static void GenerateOccluderFaceList( node_t *pOccluderNode, CUtlVector<face_t*> &occluderFaces )
{
	if (pOccluderNode->planenum == PLANENUM_LEAF)
		return;

	for ( face_t *f=pOccluderNode->faces ; f ; f = f->next )
	{
		occluderFaces.AddToTail( f );
	}

	GenerateOccluderFaceList( pOccluderNode->children[0], occluderFaces );
	GenerateOccluderFaceList( pOccluderNode->children[1], occluderFaces );
}


//-----------------------------------------------------------------------------
// For occluder area assignment
//-----------------------------------------------------------------------------
struct OccluderInfo_t
{
	int m_nOccluderEntityIndex;
};

static CUtlVector< OccluderInfo_t > g_OccluderInfo;


//-----------------------------------------------------------------------------
// Emits occluder brushes
//-----------------------------------------------------------------------------
static void EmitOccluderBrushes()
{
	char str[64];

	g_OccluderData.RemoveAll();
	g_OccluderPolyData.RemoveAll();
	g_OccluderVertexIndices.RemoveAll();

	tree_t *pOccluderTree = ClipOccluderBrushes();
	if (!pOccluderTree)
		return;

	CUtlVector<face_t*> faceList( 1024, 1024 );
	CUtlVector<side_t*> sideList( 1024, 1024 );
	GenerateOccluderFaceList( pOccluderTree->headnode, faceList );

#ifdef _DEBUG
	int *pEmitted = (int*)stackalloc( faceList.Count() * sizeof(int) );
	memset( pEmitted, 0, faceList.Count() * sizeof(int) );
#endif

	for ( entity_num=1; entity_num < num_entities; ++entity_num )
	{
		if (!IsFuncOccluder(entity_num))
			continue;

		// Output only those parts of the occluder tree which are a part of the brush
		int nOccluder = g_OccluderData.AddToTail();
		doccluderdata_t &occluderData = g_OccluderData[ nOccluder ];
		occluderData.firstpoly = g_OccluderPolyData.Count();
		occluderData.mins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		occluderData.maxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		occluderData.flags = 0;
		occluderData.area = -1;

		// NOTE: If you change the algorithm by which occluder numbers are allocated,
		// then you must also change FixupOnlyEntsOccluderEntities() below
		sprintf (str, "%i", nOccluder);
		SetKeyValue (&entities[entity_num], "occludernumber", str);

		int nIndex = g_OccluderInfo.AddToTail();
		g_OccluderInfo[nIndex].m_nOccluderEntityIndex = entity_num;
		
		sideList.RemoveAll();
		GenerateOccluderSideList( entity_num, sideList );
		for ( int i = faceList.Count(); --i >= 0; )
		{
			// Skip nodraw surfaces, but not triggers that have been marked as nodraw
			face_t *f = faceList[i];
			if ( ( texinfo[f->texinfo].flags & SURF_NODRAW ) &&
				 (( texinfo[f->texinfo].flags & SURF_TRIGGER ) == 0 ) )
				continue;

			// Only emit faces that appear in the side list of the occluder
			for ( int j = sideList.Count(); --j >= 0; )
			{
				if ( sideList[j] != f->originalface )
					continue;

				if ( f->numpoints < 3 )
					continue;

				// not a final face
				Assert ( !f->merged && !f->split[0] && !f->split[1] );

#ifdef _DEBUG
				Assert( !pEmitted[i] );
				pEmitted[i] = entity_num;
#endif

				int k = g_OccluderPolyData.AddToTail();
				doccluderpolydata_t *pOccluderPoly = &g_OccluderPolyData[k];

				pOccluderPoly->planenum = f->planenum;
				pOccluderPoly->vertexcount = f->numpoints;
				pOccluderPoly->firstvertexindex = g_OccluderVertexIndices.Count();
				for( k = 0; k < f->numpoints; ++k )
				{
					g_OccluderVertexIndices.AddToTail( f->vertexnums[k] );

					const Vector &p = dvertexes[f->vertexnums[k]].point; 
					VectorMin( occluderData.mins, p, occluderData.mins );
					VectorMax( occluderData.maxs, p, occluderData.maxs );
				}

				break;
			}
		}

		occluderData.polycount = g_OccluderPolyData.Count() - occluderData.firstpoly;

		// Mark this brush as not having brush geometry so it won't be re-emitted with a brush model
		entities[entity_num].numbrushes = 0;
	}

	FreeTree( pOccluderTree );
}


//-----------------------------------------------------------------------------
// Set occluder area
//-----------------------------------------------------------------------------
void SetOccluderArea( int nOccluder, int nArea, int nEntityNum )
{
	if ( g_OccluderData[nOccluder].area <= 0 )
	{
		g_OccluderData[nOccluder].area = nArea;
	}
	else if ( (nArea != 0) && (g_OccluderData[nOccluder].area != nArea) )
	{
		const char *pTargetName = ValueForKey( &entities[nEntityNum], "targetname" );
		if (!pTargetName)
		{
			pTargetName = "<no name>";
		}
		Warning("Occluder \"%s\" straddles multiple areas. This is invalid!\n", pTargetName );
	}
}


//-----------------------------------------------------------------------------
// Assign occluder areas (must happen *after* the world model is processed)
//-----------------------------------------------------------------------------
void AssignAreaToOccluder( int nOccluder, tree_t *pTree, bool bCrossAreaPortals )
{
	int nFirstPoly = g_OccluderData[nOccluder].firstpoly;
	int nEntityNum = g_OccluderInfo[nOccluder].m_nOccluderEntityIndex;
	for ( int j = 0; j < g_OccluderData[nOccluder].polycount; ++j )
	{
		doccluderpolydata_t *pOccluderPoly = &g_OccluderPolyData[nFirstPoly + j];
		int nFirstVertex = pOccluderPoly->firstvertexindex;
		for ( int k = 0; k < pOccluderPoly->vertexcount; ++k )
		{
			int nVertexIndex = g_OccluderVertexIndices[nFirstVertex + k];
			node_t *pNode = NodeForPoint( pTree->headnode, dvertexes[ nVertexIndex ].point );

			SetOccluderArea( nOccluder, pNode->area, nEntityNum );

			int nOtherSideIndex;
			portal_t *pPortal;
			for ( pPortal = pNode->portals; pPortal; pPortal = pPortal->next[!nOtherSideIndex] )
			{
				nOtherSideIndex = (pPortal->nodes[0] == pNode) ? 1 : 0;
				if (!pPortal->onnode)
					continue;		// edge of world

				// Don't cross over area portals for the area check
				if ((!bCrossAreaPortals) && pPortal->nodes[nOtherSideIndex]->contents & CONTENTS_AREAPORTAL)
					continue;

				int nAdjacentArea = pPortal->nodes[nOtherSideIndex] ? pPortal->nodes[nOtherSideIndex]->area : 0; 
				SetOccluderArea( nOccluder, nAdjacentArea, nEntityNum );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Assign occluder areas (must happen *after* the world model is processed)
//-----------------------------------------------------------------------------
void AssignOccluderAreas( tree_t *pTree )
{
	for ( int i = 0; i < g_OccluderData.Count(); ++i )
	{
		AssignAreaToOccluder( i, pTree, false );

		// This can only have happened if the only valid portal out leads into an areaportal
		if ( g_OccluderData[i].area <= 0 )
		{
			AssignAreaToOccluder( i, pTree, true );
		}
	}
}



//-----------------------------------------------------------------------------
// Make sure the func_occluders have the appropriate data set
//-----------------------------------------------------------------------------
void FixupOnlyEntsOccluderEntities()
{
	char str[64];
	int nOccluder = 0;
	for ( entity_num=1; entity_num < num_entities; ++entity_num )
	{
		if (!IsFuncOccluder(entity_num))
			continue;

		// NOTE: If you change the algorithm by which occluder numbers are allocated above,
		// then you must also change this
		sprintf (str, "%i", nOccluder);
		SetKeyValue (&entities[entity_num], "occludernumber", str);
		++nOccluder;
	}
}


void MarkNoDynamicShadowSides()
{
	for ( int iSide=0; iSide < g_MainMap->nummapbrushsides; iSide++ )
	{
		g_MainMap->brushsides[iSide].m_bDynamicShadowsEnabled = true;
	}

	for ( int i=0; i < g_NoDynamicShadowSides.Count(); i++ )
	{
		int brushSideID = g_NoDynamicShadowSides[i];
	
		// Find the side with this ID.
		for ( int iSide=0; iSide < g_MainMap->nummapbrushsides; iSide++ )
		{
			if ( g_MainMap->brushsides[iSide].id == brushSideID )
				g_MainMap->brushsides[iSide].m_bDynamicShadowsEnabled = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Compute the 3D skybox areas
//-----------------------------------------------------------------------------
static void Compute3DSkyboxAreas( node_t *headnode, CUtlVector<int>& areas )
{
	for (int i = 0; i < g_MainMap->num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "sky_camera"))
		{
			// Found a 3D skybox camera, get a leaf that lies in it
			node_t *pLeaf = PointInLeaf( headnode, entities[i].origin );
			if (pLeaf->contents & CONTENTS_SOLID)
			{
				Error ("Error! Entity sky_camera in solid volume! at %.1f %.1f %.1f\n", entities[i].origin.x, entities[i].origin.y, entities[i].origin.z);
			}
			areas.AddToTail( pLeaf->area );
		}
	}
}

bool Is3DSkyboxArea( int area )
{
	for ( int i = g_SkyAreas.Count(); --i >=0; )
	{
		if ( g_SkyAreas[i] == area )
			return true;
	}
	return false;
}

		
/*
============
ProcessModels
============
*/
void ProcessModels (void)
{
	BeginBSPFile ();

	// Mark sides that have no dynamic shadows.
	MarkNoDynamicShadowSides();

	// emit the displacement surfaces
	EmitInitialDispInfos();

	// Clip occluder brushes against each other, 
	// Remove them from the list of models to process below
	EmitOccluderBrushes( );

	for ( entity_num=0; entity_num < num_entities; ++entity_num )
	{
		entity_t *pEntity = &entities[entity_num];
		if ( !pEntity->numbrushes )
			continue;

		qprintf ("############### model %i ###############\n", nummodels);

		BeginModel ();

		if (entity_num == 0)
		{
			ProcessWorldModel();
		}
		else
		{
			ProcessSubModel( );
		}

		EndModel ();

		if (!verboseentities)
		{
			verbose = false;	// don't bother printing submodels
		}
	}

	// Turn the skybox into a cubemap in case we don't build env_cubemap textures.
	Cubemap_CreateDefaultCubemaps();
	EndBSPFile ();
}


void LoadPhysicsDLL( void )
{
	PhysicsDLLPath( "vphysics.dll" );
}


void PrintCommandLine( int argc, char **argv )
{
	Warning( "Command line: " );
	for ( int z=0; z < argc; z++ )
	{
		Warning( "\"%s\" ", argv[z] );
	}
	Warning( "\n\n" );
}


int RunVBSP( int argc, char **argv )
{
	int		i;
	double		start, end;
	char		path[1024];

	CommandLine()->CreateCmdLine( argc, argv );
	MathLib_Init( 2.2f, 2.2f, 0.0f, OVERBRIGHT, false, false, false, false );
	InstallSpewFunction();
	SpewActivate( "developer", 1 );
	
	CmdLib_InitFileSystem( argv[ argc-1 ] );

	Q_StripExtension( ExpandArg( argv[ argc-1 ] ), source, sizeof( source ) );
	Q_FileBase( source, mapbase, sizeof( mapbase ) );
	strlwr( mapbase );

	LoadCmdLineFromFile( argc, argv, mapbase, "vbsp" );

	Msg( "Valve Software - vbsp.exe (%s)\n", __DATE__ );

	for (i=1 ; i<argc ; i++)
	{
		if (!stricmp(argv[i],"-threads"))
		{
			numthreads = atoi (argv[i+1]);
			i++;
		}
		else if (!Q_stricmp(argv[i],"-glview"))
		{
			glview = true;
		}
		else if ( !Q_stricmp(argv[i], "-v") || !Q_stricmp(argv[i], "-verbose") )
		{
			Msg("verbose = true\n");
			verbose = true;
		}
		else if (!Q_stricmp(argv[i], "-noweld"))
		{
			Msg ("noweld = true\n");
			noweld = true;
		}
		else if (!Q_stricmp(argv[i], "-nocsg"))
		{
			Msg ("nocsg = true\n");
			nocsg = true;
		}
		else if (!Q_stricmp(argv[i], "-noshare"))
		{
			Msg ("noshare = true\n");
			noshare = true;
		}
		else if (!Q_stricmp(argv[i], "-notjunc"))
		{
			Msg ("notjunc = true\n");
			notjunc = true;
		}
		else if (!Q_stricmp(argv[i], "-nowater"))
		{
			Msg ("nowater = true\n");
			nowater = true;
		}
		else if (!Q_stricmp(argv[i], "-noopt"))
		{
			Msg ("noopt = true\n");
			noopt = true;
		}
		else if (!Q_stricmp(argv[i], "-noprune"))
		{
			Msg ("noprune = true\n");
			noprune = true;
		}
		else if (!Q_stricmp(argv[i], "-nomerge"))
		{
			Msg ("nomerge = true\n");
			nomerge = true;
		}
		else if (!Q_stricmp(argv[i], "-nomergewater"))
		{
			Msg ("nomergewater = true\n");
			nomergewater = true;
		}
		else if (!Q_stricmp(argv[i], "-nosubdiv"))
		{
			Msg ("nosubdiv = true\n");
			nosubdiv = true;
		}
		else if (!Q_stricmp(argv[i], "-nodetail"))
		{
			Msg ("nodetail = true\n");
			nodetail = true;
		}
		else if (!Q_stricmp(argv[i], "-fulldetail"))
		{
			Msg ("fulldetail = true\n");
			fulldetail = true;
		}
		else if (!Q_stricmp(argv[i], "-onlyents"))
		{
			Msg ("onlyents = true\n");
			onlyents = true;
		}
		else if (!Q_stricmp(argv[i], "-onlyprops"))
		{
			Msg ("onlyprops = true\n");
			onlyprops = true;
		}
		else if (!Q_stricmp(argv[i], "-micro"))
		{
			microvolume = atof(argv[i+1]);
			Msg ("microvolume = %f\n", microvolume);
			i++;
		}
		else if (!Q_stricmp(argv[i], "-leaktest"))
		{
			Msg ("leaktest = true\n");
			leaktest = true;
		}
		else if (!Q_stricmp(argv[i], "-verboseentities"))
		{
			Msg ("verboseentities = true\n");
			verboseentities = true;
		}
		else if (!Q_stricmp(argv[i], "-snapaxial"))
		{
			Msg ("snap axial = true\n");
			g_snapAxialPlanes = true;
		}
#if 0
		else if (!Q_stricmp(argv[i], "-maxlightmapdim"))
		{
			g_maxLightmapDimension = atof(argv[i+1]);
			Msg ("g_maxLightmapDimension = %f\n", g_maxLightmapDimension);
			i++;
		}
#endif
		else if (!Q_stricmp(argv[i], "-block"))
		{
			block_xl = block_xh = atoi(argv[i+1]);
			block_yl = block_yh = atoi(argv[i+2]);
			Msg ("block: %i,%i\n", block_xl, block_yl);
			i+=2;
		}
		else if (!Q_stricmp(argv[i], "-blocks"))
		{
			block_xl = atoi(argv[i+1]);
			block_yl = atoi(argv[i+2]);
			block_xh = atoi(argv[i+3]);
			block_yh = atoi(argv[i+4]);
			Msg ("blocks: %i,%i to %i,%i\n", 
				block_xl, block_yl, block_xh, block_yh);
			i+=4;
		}
		else if ( !Q_stricmp( argv[i], "-dumpcollide" ) )
		{
			Msg("Dumping collision models to collideXXX.txt\n" );
			dumpcollide = true;
		}
		else if ( !Q_stricmp( argv[i], "-dumpstaticprop" ) )
		{
			Msg("Dumping static props to staticpropXXX.txt\n" );
			g_DumpStaticProps = true;
		}
		else if ( !Q_stricmp( argv[i], "-forceskyvis" ) )
		{
			Msg("Enabled vis in 3d skybox\n" );
			g_bSkyVis = true;
		}
		else if (!Q_stricmp (argv[i],"-tmpout"))
		{
			strcpy (outbase, "/tmp");
		}
#if 0
		else if( !Q_stricmp( argv[i], "-defaultluxelsize" ) )
		{
			g_defaultLuxelSize = atof( argv[i+1] );
			i++;
		}
#endif
		else if( !Q_stricmp( argv[i], "-luxelscale" ) )
		{
			g_luxelScale = atof( argv[i+1] );
			i++;
		}
		else if( !strcmp( argv[i], "-minluxelscale" ) )
		{
			g_minLuxelScale = atof( argv[i+1] );
			if (g_minLuxelScale < 1)
				g_minLuxelScale = 1;
			i++;
		}
		else if( !Q_stricmp( argv[i], "-dxlevel" ) )
		{
			g_nDXLevel = atoi( argv[i+1] );
			Msg( "DXLevel = %d\n", g_nDXLevel );
			i++;
		}
		else if( !Q_stricmp( argv[i], "-bumpall" ) )
		{
			g_BumpAll = true;
		}
		else if( !Q_stricmp( argv[i], "-low" ) )
		{
			g_bLowPriority = true;
		}
		else if( !Q_stricmp( argv[i], "-lightifmissing" ) )
		{
			g_bLightIfMissing = true;
		}
		else if ( !Q_stricmp( argv[i], CMDLINEOPTION_NOVCONFIG ) )
		{
		}
		else if ( !Q_stricmp( argv[i], "-allowdebug" ) || !Q_stricmp( argv[i], "-steam" ) )
		{
			// nothing to do here, but don't bail on this option
		}
		else if ( !Q_stricmp( argv[i], "-vproject" ) || !Q_stricmp( argv[i], "-game" ) )
		{
			++i;
		}
		else if ( !Q_stricmp( argv[i], "-keepstalezip" ) )
		{
			g_bKeepStaleZip = true;
		}
		else if ( !Q_stricmp( argv[i], "-xbox" ) )
		{
			// enable mandatory xbox extensions
			g_NodrawTriggers = true;
			g_DisableWaterLighting = true;
		}
		else if ( !Q_stricmp( argv[i], "-allowdetailcracks"))
		{
			g_bAllowDetailCracks = true;
		}
		else if ( !Q_stricmp( argv[i], "-novirtualmesh"))
		{
			g_bNoVirtualMesh = true;
		}
		else if ( !Q_stricmp( argv[i], "-replacematerials" ) )
		{
			g_ReplaceMaterials = true;
		}
		else if ( !Q_stricmp(argv[i], "-nodrawtriggers") )
		{
			g_NodrawTriggers = true;
		}
		else if ( !Q_stricmp( argv[i], "-FullMinidumps" ) )
		{
			EnableFullMinidumps( true );
		}
		else if (argv[i][0] == '-')
		{
			Warning("VBSP: Unknown option \"%s\"\n\n", argv[i]);
			i = 100000;	// force it to print the usage
			break;
		}
		else
			break;
	}

	if (i != argc - 1)
	{
		PrintCommandLine( argc, argv );

		Warning(	
			"usage  : vbsp [options...] mapfile\n"
			"example: vbsp -onlyents c:\\hl2\\hl2\\maps\\test\n"
			"\n"
			"Common options (use -v to see all options):\n"
			"\n"
			"  -v (or -verbose): Turn on verbose output (also shows more command\n"
			"                    line options).\n"
			"\n"
			"  -onlyents   : This option causes vbsp only import the entities from the .vmf\n"
			"                file. -onlyents won't reimport brush models.\n"
			"  -onlyprops  : Only update the static props and detail props.\n"
			"  -glview     : Writes .gl files in the current directory that can be viewed\n"
			"                with glview.exe. If you use -tmpout, it will write the files\n"
			"                into the \\tmp folder.\n"
			"  -nodetail   : Get rid of all detail geometry. The geometry left over is\n"
			"                what affects visibility.\n"
			"  -nowater    : Get rid of water brushes.\n"
			"  -low        : Run as an idle-priority process.\n"
			"\n"
			"  -vproject <directory> : Override the VPROJECT environment variable.\n"
			"  -game <directory>     : Same as -vproject.\n"
			"\n" );

		if ( verbose )
		{
			Warning(
				"Other options  :\n"
				"  -novconfig   : Don't bring up graphical UI on vproject errors.\n"
				"  -threads     : Control the number of threads vbsp uses (defaults to the # of\n"
				"                 processors on your machine).\n"
				"  -verboseentities: If -v is on, this disables verbose output for submodels.\n"
				"  -noweld      : Don't join face vertices together.\n"
				"  -nocsg       : Don't chop out intersecting brush areas.\n"
				"  -noshare     : Emit unique face edges instead of sharing them.\n"
				"  -notjunc     : Don't fixup t-junctions.\n"
				"  -noopt       : By default, vbsp removes the 'outer shell' of the map, which\n"
				"                 are all the faces you can't see because you can never get\n"
				"                 outside the map. -noopt disables this behaviour.\n"
				"  -noprune     : Don't prune neighboring solid nodes.\n"
				"  -nomerge     : Don't merge together chopped faces on nodes.\n"
				"  -nomergewater: Don't merge together chopped faces on water.\n"
				"  -nosubdiv    : Don't subdivide faces for lightmapping.\n"
				"  -micro <#>   : vbsp will warn when brushes are output with a volume less\n"
				"                 than this number (default: 1.0).\n"
				"  -fulldetail  : Mark all detail geometry as normal geometry (so all detail\n"
				"                 geometry will affect visibility).\n"
				"  -leaktest    : Stop processing the map if a leak is detected. Whether or not\n"
				"                 this flag is set, a leak file will be written out at\n"
				"                 <vmf filename>.lin, and it can be imported into Hammer.\n"
				"  -bumpall     : Force all surfaces to be bump mapped.\n"
				"  -snapaxial   : Snap axial planes to integer coordinates.\n"
				"  -block # #      : Control the grid size mins that vbsp chops the level on.\n"
				"  -blocks # # # # : Enter the mins and maxs for the grid size vbsp uses.\n"
				"  -dumpstaticprops: Dump static props to staticprop*.txt\n"
				"  -dumpcollide    : Write files with collision info.\n"
				"  -forceskyvis	   : Enable vis calculations in 3d skybox leaves\n"
				"  -luxelscale #   : Scale all lightmaps by this amount (default: 1.0).\n"
				"  -minluxelscale #: No luxel scale will be lower than this amount (default: 1.0).\n"
				"  -lightifmissing : Force lightmaps to be generated for all surfaces even if\n"
				"                    they don't need lightmaps.\n"
				"  -keepstalezip   : Keep the BSP's zip files intact but regenerate everything\n"
				"                    else.\n"
				"  -virtualdispphysics : Use virtual (not precomputed) displacement collision models\n"
				"  -xbox           : Enable mandatory xbox options\n"
				"  -x360		   : Generate Xbox360 version of vsp\n"
				"  -nox360		   : Disable generation Xbox360 version of vsp (default)\n"
				"  -replacematerials : Substitute materials according to materialsub.txt in content\\maps\n"
				"  -FullMinidumps  : Write large minidumps on crash.\n"
				);
			}

		DeleteCmdLine( argc, argv );
		CmdLib_Cleanup();
		CmdLib_Exit( 1 );
	}

	start = Plat_FloatTime();

	// Run in the background?
	if( g_bLowPriority )
	{
		SetLowPriority();
	}

	if( ( g_nDXLevel != 0 ) && ( g_nDXLevel < 80 ) )
	{
		g_BumpAll = false;
	}

	if( g_luxelScale == 1.0f )
	{
		if ( g_nDXLevel == 70 )
		{
			g_luxelScale = 4.0f;
		}
	}

	ThreadSetDefault ();
	numthreads = 1;		// multiple threads aren't helping...

	// Setup the logfile.
	char logFile[512];
	_snprintf( logFile, sizeof(logFile), "%s.log", source );
	SetSpewFunctionLogFile( logFile );

	LoadPhysicsDLL();
	LoadSurfaceProperties();

#if 0
	Msg( "qdir: %s  This is the the path of the initial source file \n", qdir );
	Msg( "gamedir: %s This is the base engine + mod-specific game dir (e.g. d:/tf2/mytfmod/) \n", gamedir );
	Msg( "basegamedir: %s This is the base engine + base game directory (e.g. e:/hl2/hl2/, or d:/tf2/tf2/ )\n", basegamedir );
#endif

	sprintf( materialPath, "%smaterials", gamedir );
	InitMaterialSystem( materialPath, CmdLib_GetFileSystemFactory() );
	Msg( "materialPath: %s\n", materialPath );
	
	// delete portal and line files
	sprintf (path, "%s.prt", source);
	remove (path);
	sprintf (path, "%s.lin", source);
	remove (path);

	strcpy (name, ExpandArg (argv[i]));	

	const char *pszExtension = V_GetFileExtension( name );
	if ( !pszExtension )
	{
		V_SetExtension( name, ".vmm", sizeof( name ) );
		if ( !FileExists( name ) )
		{
			V_SetExtension( name, ".vmf", sizeof( name ) );
		}
	}

	char platformBSPFileName[1024];
	GetPlatformMapPath( source, platformBSPFileName, g_nDXLevel, 1024 );
	
	// if we're combining materials, load the script file
	if ( g_ReplaceMaterials )
	{
		LoadMaterialReplacementKeys( gamedir, mapbase );
	}

	//
	// if onlyents, just grab the entites and resave
	//
	if (onlyents)
	{
		LoadBSPFile (platformBSPFileName);
		num_entities = 0;
		// Clear out the cubemap samples since they will be reparsed even with -onlyents
		g_nCubemapSamples = 0;

		// Mark as stale since the lighting could be screwed with new ents.
		AddBufferToPak( GetPakFile(), "stale.txt", "stale", strlen( "stale" ) + 1, false );

		LoadMapFile (name);
		SetModelNumbers ();
		SetLightStyles ();

		// NOTE: If we ever precompute lighting for static props in
		// vrad, EmitStaticProps should be removed here

		// Emit static props found in the .vmf file
		EmitStaticProps();

		// NOTE: Don't deal with detail props here, it blows away lighting

		// Recompute the skybox
		ComputeBoundsNoSkybox();

		// Make sure that we have a water lod control eneity if we have water in the map.
		EnsurePresenceOfWaterLODControlEntity();

		// Make sure the func_occluders have the appropriate data set
		FixupOnlyEntsOccluderEntities();

		// Doing this here because stuff abov may filter out entities
		UnparseEntities ();

		WriteBSPFile (platformBSPFileName);
	}
	else if (onlyprops)
	{
		// In the only props case, deal with static + detail props only
		LoadBSPFile (platformBSPFileName);

		LoadMapFile(name);
		SetModelNumbers();
		SetLightStyles();

		// Emit static props found in the .vmf file
		EmitStaticProps();

		// Place detail props found in .vmf and based on material properties
		LoadEmitDetailObjectDictionary( gamedir );
		EmitDetailObjects();

		WriteBSPFile (platformBSPFileName);
	}
	else
	{
		//
		// start from scratch
		//

		// Load just the file system from the bsp
		if( g_bKeepStaleZip && FileExists( platformBSPFileName ) )
		{
			LoadBSPFile_FileSystemOnly (platformBSPFileName);
			// Mark as stale since the lighting could be screwed with new ents.
			AddBufferToPak( GetPakFile(), "stale.txt", "stale", strlen( "stale" ) + 1, false );
		}

		LoadMapFile (name);
		WorldVertexTransitionFixup();
		if( ( g_nDXLevel == 0 ) || ( g_nDXLevel >= 70 ) )
		{
			Cubemap_FixupBrushSidesMaterials();
			Cubemap_AttachDefaultCubemapToSpecularSides();
			Cubemap_AddUnreferencedCubemaps();
		}
		SetModelNumbers ();
		SetLightStyles ();
		LoadEmitDetailObjectDictionary( gamedir );
		ProcessModels ();
	}

	end = Plat_FloatTime();
	
	char str[512];
	GetHourMinuteSecondsString( (int)( end - start ), str, sizeof( str ) );
	Msg( "%s elapsed\n", str );

	DeleteCmdLine( argc, argv );
	ReleasePakFileLumps();
	DeleteMaterialReplacementKeys();
	ShutdownMaterialSystem();
	CmdLib_Cleanup();
	return 0;
}


/*
=============
main
============
*/
int main (int argc, char **argv)
{
	// Install an exception handler.
	SetupDefaultToolsMinidumpHandler();
	return RunVBSP( argc, argv );
}


