//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"
#include "disp_vbsp.h"
#include "utlvector.h"
#include "faces.h"
#include "builddisp.h"
#include "tier1/strtools.h"
#include "utilmatlib.h"
#include "utldict.h"
#include "map.h"

int		c_nofaces;
int		c_facenodes;

// NOTE: This is a global used to link faces back to the tree node/portals they came from
// it's used when filling water volumes
node_t *dfacenodes[MAX_MAP_FACES];


/*
=========================================================

ONLY SAVE OUT PLANES THAT ARE ACTUALLY USED AS NODES

=========================================================
*/
  
void EmitFaceVertexes (face_t **list, face_t *f);
void AssignOccluderAreas();

/*
============
EmitPlanes

There is no oportunity to discard planes, because all of the original
brushes will be saved in the map.
============
*/
void EmitPlanes (void)
{
	int			i;
	dplane_t	*dp;
	plane_t		*mp;
	int		planetranslate[MAX_MAP_PLANES];

	mp = g_MainMap->mapplanes;
	for (i=0 ; i<g_MainMap->nummapplanes ; i++, mp++)
	{
		dp = &dplanes[numplanes];
		planetranslate[i] = numplanes;
		VectorCopy ( mp->normal, dp->normal);
		dp->dist = mp->dist;
		dp->type = mp->type;
		numplanes++;
	}
}


//========================================================

void EmitMarkFace (dleaf_t *leaf_p, face_t *f)
{
	int			i;
	int			facenum;

	while (f->merged)
		f = f->merged;

	if (f->split[0])
	{
		EmitMarkFace (leaf_p, f->split[0]);
		EmitMarkFace (leaf_p, f->split[1]);
		return;
	}

	facenum = f->outputnumber;
	if (facenum == -1)
		return;	// degenerate face

	if (facenum < 0 || facenum >= numfaces)
		Error ("Bad leafface");
	for (i=leaf_p->firstleafface ; i<numleaffaces ; i++)
		if (dleaffaces[i] == facenum)
			break;		// merged out face
	if (i == numleaffaces)
	{
		if (numleaffaces >= MAX_MAP_LEAFFACES)
			Error ("Too many detail brush faces, max = %d\n", MAX_MAP_LEAFFACES);

		dleaffaces[numleaffaces] =  facenum;
		numleaffaces++;
	}

}


/*
==================
EmitLeaf
==================
*/
void EmitLeaf (node_t *node)
{
	dleaf_t		*leaf_p;
	portal_t	*p;
	int			s;
	face_t		*f;
	bspbrush_t	*b;
	int			i;
	int			brushnum;
	leafface_t	*pList;

	// emit a leaf
	if (numleafs >= MAX_MAP_LEAFS)
		Error ("Too many BSP leaves, max = %d", MAX_MAP_LEAFS);

	node->diskId = numleafs;
	leaf_p = &dleafs[numleafs];
	numleafs++;

	if( nummodels == 0 )
	{
		leaf_p->cluster = node->cluster;
	}
	else
	{
		// Submodels don't have clusters. If this isn't set to -1 here, then there
		// will be multiple leaves (albeit from different models) that reference
		// the same cluster and parts of the code like ivp.cpp's ConvertWaterModelToPhysCollide
		// won't work.
		leaf_p->cluster = -1; 
	}

	leaf_p->contents = node->contents;
	leaf_p->area = node->area;

	// By default, assume the leaf can see the skybox.
	// VRAD will do the actual computation to see if it really can see the skybox
	leaf_p->flags = LEAF_FLAGS_SKY;

	//
	// write bounding box info
	//	
	VECTOR_COPY (node->mins, leaf_p->mins);
	VECTOR_COPY (node->maxs, leaf_p->maxs);
	
	//
	// write the leafbrushes
	//
	leaf_p->firstleafbrush = numleafbrushes;
	for (b=node->brushlist ; b ; b=b->next)
	{
		if (numleafbrushes >= MAX_MAP_LEAFBRUSHES)
			Error ("Too many brushes in one leaf, max = %d", MAX_MAP_LEAFBRUSHES);

		brushnum = b->original - g_MainMap->mapbrushes;
		for (i=leaf_p->firstleafbrush ; i<numleafbrushes ; i++)
		{
			if (dleafbrushes[i] == brushnum)
				break;
		}

		if (i == numleafbrushes)
		{
			dleafbrushes[numleafbrushes] = brushnum;
			numleafbrushes++;
		}
	}
	leaf_p->numleafbrushes = numleafbrushes - leaf_p->firstleafbrush;

	//
	// write the leaffaces
	//
	if (leaf_p->contents & CONTENTS_SOLID)
		return;		// no leaffaces in solids

	leaf_p->firstleafface = numleaffaces;

	for (p = node->portals ; p ; p = p->next[s])	
	{
		s = (p->nodes[1] == node);
		f = p->face[s];
		if (!f)
			continue;	// not a visible portal

		EmitMarkFace (leaf_p, f);
	}
	
	// emit the detail faces
	for ( pList = node->leaffacelist; pList; pList = pList->pNext )
	{
		EmitMarkFace( leaf_p, pList->pFace );
	}


	leaf_p->numleaffaces = numleaffaces - leaf_p->firstleafface;
}

// per face plane - original face "side" list
side_t *pOrigFaceSideList[MAX_MAP_PLANES];

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CreateOrigFace( face_t *f )
{
    int         i, j;
    dface_t     *of;
    side_t      *side;
    int         vIndices[128];
    int         eIndex[2];
    winding_t   *pWinding;

    // not a real face!
    if( !f->w )
        return -1;

    // get the original face -- the "side"
    side = f->originalface;

    // get the original face winding
	if( !side->winding )
	{
		return -1;
	}

    //
    // get the next original face
    //
    if( numorigfaces >= MAX_MAP_FACES )
		Error( "Too many faces in map, max = %d", MAX_MAP_FACES );
	of = &dorigfaces[numorigfaces];
    numorigfaces++;

    // set original face to -1 -- it is an origianl face!
    of->origFace = -1;

    //
    // add side to plane list
    //
    side->next = pOrigFaceSideList[f->planenum];
    pOrigFaceSideList[f->planenum] = side;
    side->origIndex = numorigfaces - 1;

    pWinding = CopyWinding( side->winding );

    //
    // plane info
    //
    of->planenum = side->planenum;
	if ( side->contents & CONTENTS_DETAIL )
		of->onNode = 0;
	else
		of->onNode = 1;
	of->side = side->planenum & 1;

    //
    // edge info
    //
    of->firstedge = numsurfedges;
    of->numedges = side->winding->numpoints;

    //
    // material info
    //
    of->texinfo = side->texinfo;
	of->dispinfo = f->dispinfo;

    //
    // save the vertices
    //
    for( i = 0; i < pWinding->numpoints; i++ )
    {
        //
        // compare vertices
        //
		vIndices[i] = GetVertexnum( pWinding->p[i] );
    }

    //
    // save off points -- as edges
    //
    for( i = 0; i < pWinding->numpoints; i++ )
	{
        //
        // look for matching edges first
        //
        eIndex[0] = vIndices[i];
        eIndex[1] = vIndices[(i+1)%pWinding->numpoints];

        for( j = firstmodeledge; j < numedges; j++ )
        {
            if( ( eIndex[0] == dedges[j].v[1] ) &&
                ( eIndex[1] == dedges[j].v[0] ) &&
         	    ( edgefaces[j][0]->contents == f->contents ) )
            {
                // check for multiple backward edges!! -- shouldn't have
				if( edgefaces[j][1] )
					continue;

                // set back edge
				edgefaces[j][1] = f;

                //
                // get next surface edge
                //
                if( numsurfedges >= MAX_MAP_SURFEDGES )
                    Error( "Too much brush geometry in bsp, numsurfedges == MAX_MAP_SURFEDGES" );                
                dsurfedges[numsurfedges] = -j;
                numsurfedges++;
                break;
            }
        }
        
        if( j == numedges )
        {
            //
            // get next edge
            //
			AddEdge( eIndex[0], eIndex[1], f );
        
            //
            // get next surface edge
            //
            if( numsurfedges >= MAX_MAP_SURFEDGES )
				Error( "Too much brush geometry in bsp, numsurfedges == MAX_MAP_SURFEDGES" );                
            dsurfedges[numsurfedges] = ( numedges - 1 );
            numsurfedges++;
        }
	}

    // return the index
    return ( numorigfaces - 1 );
}


//-----------------------------------------------------------------------------
// Purpose: search for a face within the origface list and return the index if
//          found
//   Input: f - the face to compare
//  Output: the index of the face it found, -1 if not found
//-----------------------------------------------------------------------------
int FindOrigFace( face_t *f )
{
    int         i;
    static int  bClear = 0;
    side_t      *pSide;

    //
    // initially clear the face side lists (per face plane)
    //
    if( !bClear )
    {
        for( i = 0; i < MAX_MAP_PLANES; i++ )
        {
            pOrigFaceSideList[i] = NULL;
        }
        bClear = 1;
    }

    //
    // compare the sides
    //
    for( pSide = pOrigFaceSideList[f->planenum]; pSide; pSide = pSide->next )
    {
        if( pSide == f->originalface )
            return pSide->origIndex;
    }

    // original face not found in list
    return -1;
}


//-----------------------------------------------------------------------------
// Purpose: to find an the original face within the list of original faces, if
//          a match is not found then create a new origFace -- either way pass
//          back the index of the origface in the list
//   Input: f - face containing the original face information
//  Output: the index of the origface in the origface list
//-----------------------------------------------------------------------------
int FindOrCreateOrigFace( face_t *f )
{
    int index;

    // check for an original face
    if( !f->originalface )
        return -1;

    //
    // find or create a orig face and return the index
    //
    index = FindOrigFace( f );

    if( index == -1 )
        return CreateOrigFace( f );
    else if( index == -2 )
        return -1;

    return index;
}

/*
==================
EmitFace
==================
*/
void EmitFace( face_t *f, qboolean onNode )
{
	dface_t	*df;
	int		i;
	int		e;

//	void SubdivideFaceBySubdivSize( face_t *f ); // garymcthack
//	SubdivideFaceBySubdivSize( f );
    
	// set initial output number
	f->outputnumber = -1;

    // degenerated
	if( f->numpoints < 3 )
		return;
    
    // not a final face
	if( f->merged || f->split[0] || f->split[1] )
		return;

	// don't emit NODRAW faces for runtime
	if ( texinfo[f->texinfo].flags & SURF_NODRAW )
	{
		// keep NODRAW terrain surfaces though
		if ( f->dispinfo == -1 )
			return;
		Warning("NODRAW on terrain surface!\n");
	}

	// save output number so leaffaces can use
	f->outputnumber = numfaces;

    //
    // get the next available .bsp face slot
    //
	if (numfaces >= MAX_MAP_FACES)
		Error( "Too many faces in map, max = %d", MAX_MAP_FACES );
	df = &dfaces[numfaces];

	// Save the correlation between dfaces and faces -- since dfaces doesnt have worldcraft face id
	dfaceids.AddToTail();
	dfaceids[numfaces].hammerfaceid = f->originalface->id;

	numfaces++;

    //
	// plane info - planenum is used by qlight, but not quake
    //
	df->planenum = f->planenum;
	df->onNode = onNode;
	df->side = f->planenum & 1;
    
    //
    // material info
    //
	df->texinfo = f->texinfo;
	df->dispinfo = f->dispinfo;
	df->smoothingGroups = f->smoothingGroups;

    // save the original "side"/face data
    df->origFace = FindOrCreateOrigFace( f );
	df->surfaceFogVolumeID = -1;
	dfacenodes[numfaces-1] = f->fogVolumeLeaf;
	if ( f->fogVolumeLeaf )
	{
		Assert( f->fogVolumeLeaf->planenum == PLANENUM_LEAF );
	}

    //
    // edge info
    //
	df->firstedge = numsurfedges;
	df->numedges = f->numpoints;

	// UNDONE: Nodraw faces have no winding - revisit to see if this is necessary
	if ( f->w )
	{
		df->area = WindingArea( f->w );
	}
	else
	{
		df->area = 0;
	}

	df->firstPrimID = f->firstPrimID;
	df->SetNumPrims( f->numPrims );
	df->SetDynamicShadowsEnabled( f->originalface->m_bDynamicShadowsEnabled );

    //
    // save off points -- as edges
    //
	for( i = 0; i < f->numpoints; i++ )
	{
		//e = GetEdge (f->pts[i], f->pts[(i+1)%f->numpoints], f);
		e = GetEdge2 (f->vertexnums[i], f->vertexnums[(i+1)%f->numpoints], f);

		if (numsurfedges >= MAX_MAP_SURFEDGES)
			Error( "Too much brush geometry in bsp, numsurfedges == MAX_MAP_SURFEDGES" );                
		dsurfedges[numsurfedges] = e;
		numsurfedges++;
	}

	// Create overlay face lists.
	side_t *pSide = f->originalface;
	if ( pSide )
	{
		int nOverlayCount = pSide->aOverlayIds.Count();
		if ( nOverlayCount > 0 )
		{
			Overlay_AddFaceToLists( ( numfaces - 1 ), pSide );
		}

		nOverlayCount = pSide->aWaterOverlayIds.Count();
		if ( nOverlayCount > 0 )
		{
			OverlayTransition_AddFaceToLists( ( numfaces - 1 ), pSide );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Emit all of the faces stored at the leaves (faces from detail brushes)
//-----------------------------------------------------------------------------
void EmitLeafFaces( face_t *pLeafFaceList )
{
	face_t *f = pLeafFaceList;
	while ( f )
	{
		EmitFace( f, false );
		f = f->next;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Free the list of faces stored at the leaves
//-----------------------------------------------------------------------------
void FreeLeafFaces( face_t *pLeafFaceList )
{
	int count = 0;
	face_t *f, *next;
	
	f = pLeafFaceList;

	while ( f )
	{
		next = f->next;
		FreeFace( f );
		f = next;
		count++;
	}
}

/*
============
EmitDrawingNode_r
============
*/
int EmitDrawNode_r (node_t *node)
{
	dnode_t	*n;
	face_t	*f;
	int		i;

	if (node->planenum == PLANENUM_LEAF)
	{
		EmitLeaf (node);
		return -numleafs;
	}

	// emit a node	
	if (numnodes == MAX_MAP_NODES)
		Error ("MAX_MAP_NODES");
	node->diskId = numnodes;

	n = &dnodes[numnodes];
	numnodes++;

	VECTOR_COPY (node->mins, n->mins);
	VECTOR_COPY (node->maxs, n->maxs);

	if (node->planenum & 1)
		Error ("WriteDrawNodes_r: odd planenum");
	n->planenum = node->planenum;
	n->firstface = numfaces;
	n->area = node->area;

	if (!node->faces)
		c_nofaces++;
	else
		c_facenodes++;

	for (f=node->faces ; f ; f=f->next)
		EmitFace (f, true);

	n->numfaces = numfaces - n->firstface;


	//
	// recursively output the other nodes
	//	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i]->planenum == PLANENUM_LEAF)
		{
			n->children[i] = -(numleafs + 1);
			EmitLeaf (node->children[i]);
		}
		else
		{
			n->children[i] = numnodes;	
			EmitDrawNode_r (node->children[i]);
		}
	}

	return n - dnodes;
}


//=========================================================

// This will generate a scratchpad file with the level's geometry in it and the noshadow faces drawn red.
// #define SCRATCHPAD_NO_SHADOW_FACES
#if defined( SCRATCHPAD_NO_SHADOW_FACES )
	#include "scratchpad_helpers.h"
	IScratchPad3D *g_pPad;
#endif


void MarkNoShadowFaces()
{
#if defined( SCRATCHPAD_NO_SHADOW_FACES )
	g_pPad = ScratchPad3D_Create();
	ScratchPad_DrawWorld( g_pPad, false, CSPColor(1,1,1,0.3) );

	for ( int iFace=0; iFace < numfaces; iFace++ )
	{
		dface_t *pFace = &dfaces[iFace];

		if ( !pFace->AreDynamicShadowsEnabled() )
		{
			ScratchPad_DrawFace( g_pPad, pFace, iFace, CSPColor(1,0,0,1), Vector(1,0,0) );
			ScratchPad_DrawFace( g_pPad, pFace, iFace, CSPColor(1,0,0,1), Vector(-1,0,0) );
			ScratchPad_DrawFace( g_pPad, pFace, iFace, CSPColor(1,0,0,1), Vector(0,1,0) );
		}
	}
	g_pPad->Release();
#endif
}

struct texinfomap_t
{
	int refCount;
	int outputIndex;
};
struct texdatamap_t
{
	int refCount;
	int outputIndex;
};

// Find the best used texinfo to remap this brush side
int FindMatchingBrushSideTexinfo( int sideIndex, const texinfomap_t *pMap )
{
	dbrushside_t &side = dbrushsides[sideIndex];
	// find one with the same flags & surfaceprops (even if the texture name is different)
	int sideTexFlags = texinfo[side.texinfo].flags;
	int sideTexData = texinfo[side.texinfo].texdata;
	int sideSurfaceProp = g_SurfaceProperties[sideTexData];
	for ( int j = 0; j < texinfo.Count(); j++ )
	{
		if ( pMap[j].refCount > 0 && 
			texinfo[j].flags == sideTexFlags && 
			g_SurfaceProperties[texinfo[j].texdata] == sideSurfaceProp )
		{
			// found one
			return j;
		}
	}

	// can't find a better match
	return side.texinfo;
}

// Remove all unused texinfos and rebuild array
void ComapctTexinfoArray( texinfomap_t *pMap )
{
	CUtlVector<texinfo_t> old;
	old.CopyArray( texinfo.Base(), texinfo.Count() );
	texinfo.RemoveAll();
	int firstSky = -1;
	int first2DSky = -1;
	for ( int i = 0; i < old.Count(); i++ )
	{
		if ( !pMap[i].refCount )
		{
			pMap[i].outputIndex = -1;
			continue;
		}
		// only add one sky texinfo + one 2D sky texinfo
		if ( old[i].flags & SURF_SKY2D )
		{
			if ( first2DSky < 0 )
			{
				first2DSky = texinfo.AddToTail( old[i] );
			}
			pMap[i].outputIndex = first2DSky;
			continue;
		}
		if ( old[i].flags & SURF_SKY )
		{
			if ( firstSky < 0 )
			{
				firstSky = texinfo.AddToTail( old[i] );
			}
			pMap[i].outputIndex = firstSky;
			continue;
		}
		pMap[i].outputIndex = texinfo.AddToTail( old[i] );
	}
}

void CompactTexdataArray( texdatamap_t *pMap )
{
	CUtlVector<char>	oldStringData;
	oldStringData.CopyArray( g_TexDataStringData.Base(), g_TexDataStringData.Count() );
	g_TexDataStringData.RemoveAll();
	CUtlVector<int>		oldStringTable;
	oldStringTable.CopyArray( g_TexDataStringTable.Base(), g_TexDataStringTable.Count() );
	g_TexDataStringTable.RemoveAll();
	CUtlVector<dtexdata_t> oldTexData;
	oldTexData.CopyArray( dtexdata, numtexdata );
	// clear current table and rebuild
	numtexdata = 0;
	for ( int i = 0; i < oldTexData.Count(); i++ )
	{
		// unreferenced, note in map and skip
		if ( !pMap[i].refCount )
		{
			pMap[i].outputIndex = -1;
			continue;
		}
		pMap[i].outputIndex = numtexdata;

		// get old string and re-add to table
		const char *pString = &oldStringData[oldStringTable[oldTexData[i].nameStringTableID]];
		int nameIndex = TexDataStringTable_AddOrFindString( pString );
		// copy old texdata and fixup with new name in compacted table
		dtexdata[numtexdata] = oldTexData[i];
		dtexdata[numtexdata].nameStringTableID = nameIndex;
		numtexdata++;
	}
}

void CompactTexinfos()
{
	Msg("Compacting texture/material tables...\n");
	texinfomap_t *texinfoMap = new texinfomap_t[texinfo.Count()];
	texdatamap_t *texdataMap = new texdatamap_t[numtexdata];
	memset( texinfoMap, 0, sizeof(texinfoMap[0])*texinfo.Count() );
	memset( texdataMap, 0, sizeof(texdataMap[0])*numtexdata );
	int i;
	// get texinfos referenced by faces
	for ( i = 0; i < numfaces; i++ )
	{
		texinfoMap[dfaces[i].texinfo].refCount++;
	}
	// get texinfos referenced by brush sides
	for ( i = 0; i < numbrushsides; i++ )
	{
		// not referenced by any visible geometry
		Assert( dbrushsides[i].texinfo >=  0 );
		if ( !texinfoMap[dbrushsides[i].texinfo].refCount )
		{
			dbrushsides[i].texinfo = FindMatchingBrushSideTexinfo( i, texinfoMap );
			// didn't find anything suitable, go ahead and reference it
			if ( !texinfoMap[dbrushsides[i].texinfo].refCount )
			{
				texinfoMap[dbrushsides[i].texinfo].refCount++;
			}
		}
	}
	// get texinfos referenced by overlays
	for ( i = 0; i < g_nOverlayCount; i++ )
	{
		texinfoMap[g_Overlays[i].nTexInfo].refCount++;
	}
	for ( i = 0; i < numleafwaterdata; i++ )
	{
		if ( dleafwaterdata[i].surfaceTexInfoID >= 0 )
		{
			texinfoMap[dleafwaterdata[i].surfaceTexInfoID].refCount++;
		}
	}
	for ( i = 0; i < *pNumworldlights; i++ )
	{
		if ( dworldlights[i].texinfo >= 0 )
		{
			texinfoMap[dworldlights[i].texinfo].refCount++;
		}
	}
	for ( i = 0; i < g_nWaterOverlayCount; i++ )
	{
		if ( g_WaterOverlays[i].nTexInfo >= 0 )
		{
			texinfoMap[g_WaterOverlays[i].nTexInfo].refCount++;
		}
	}
	// reference all used texdatas
	for ( i = 0; i < texinfo.Count(); i++ )
	{
		if ( texinfoMap[i].refCount > 0 )
		{
			texdataMap[texinfo[i].texdata].refCount++;
		}
	}

	int oldCount = texinfo.Count();
	int oldTexdataCount = numtexdata;
	int oldTexdataString = g_TexDataStringData.Count();
	ComapctTexinfoArray( texinfoMap );
	CompactTexdataArray( texdataMap );
	for ( i = 0; i < texinfo.Count(); i++ )
	{
		int mapIndex = texdataMap[texinfo[i].texdata].outputIndex;
		Assert( mapIndex >= 0 );
		texinfo[i].texdata = mapIndex;
		//const char *pName = TexDataStringTable_GetString( dtexdata[texinfo[i].texdata].nameStringTableID );
	}
	// remap texinfos on faces
	for ( i = 0; i < numfaces; i++ )
	{
		Assert( texinfoMap[dfaces[i].texinfo].outputIndex >= 0 );
		dfaces[i].texinfo = texinfoMap[dfaces[i].texinfo].outputIndex;
	}
	// remap texinfos on brushsides
	for ( i = 0; i < numbrushsides; i++ )
	{
		Assert( texinfoMap[dbrushsides[i].texinfo].outputIndex >= 0 );
		dbrushsides[i].texinfo = texinfoMap[dbrushsides[i].texinfo].outputIndex;
	}
	// remap texinfos on overlays
	for ( i = 0; i < g_nOverlayCount; i++ )
	{
		g_Overlays[i].nTexInfo = texinfoMap[g_Overlays[i].nTexInfo].outputIndex;
	}
	// remap leaf water data
	for ( i = 0; i < numleafwaterdata; i++ )
	{
		if ( dleafwaterdata[i].surfaceTexInfoID >= 0 )
		{
			dleafwaterdata[i].surfaceTexInfoID = texinfoMap[dleafwaterdata[i].surfaceTexInfoID].outputIndex;
		}
	}
	// remap world lights
	for ( i = 0; i < *pNumworldlights; i++ )
	{
		if ( dworldlights[i].texinfo >= 0 )
		{
			dworldlights[i].texinfo = texinfoMap[dworldlights[i].texinfo].outputIndex;
		}
	}
	// remap water overlays
	for ( i = 0; i < g_nWaterOverlayCount; i++ )
	{
		if ( g_WaterOverlays[i].nTexInfo >= 0 )
		{
			g_WaterOverlays[i].nTexInfo = texinfoMap[g_WaterOverlays[i].nTexInfo].outputIndex;
		}
	}

	Msg("Reduced %d texinfos to %d\n", oldCount, texinfo.Count() );
	Msg("Reduced %d texdatas to %d (%d bytes to %d)\n", oldTexdataCount, numtexdata, oldTexdataString, g_TexDataStringData.Count() );

	delete[] texinfoMap;
	delete[] texdataMap;
}

/*
============
WriteBSP
============
*/
void WriteBSP (node_t *headnode, face_t *pLeafFaceList )
{
	int		i;
	int		oldfaces;
    int     oldorigfaces;

	c_nofaces = 0;
	c_facenodes = 0;

	qprintf ("--- WriteBSP ---\n");

	oldfaces = numfaces;
    oldorigfaces = numorigfaces;

	GetEdge2_InitOptimizedList();
	EmitLeafFaces( pLeafFaceList );
	dmodels[nummodels].headnode = EmitDrawNode_r (headnode);
	
	// Only emit area portals for the main world.
	if( nummodels == 0 )
	{
		EmitAreaPortals (headnode);
	}
		
	//
	// add all displacement faces for the particular model
	//
	for( i = 0; i < nummapdispinfo; i++ )
	{
		int entityIndex = GetDispInfoEntityNum( &mapdispinfo[i] );
		if( entityIndex == entity_num )
		{
			EmitFaceVertexes( NULL, &mapdispinfo[i].face );
			EmitFace( &mapdispinfo[i].face, FALSE );
		}
	}

	EmitWaterVolumesForBSP( &dmodels[nummodels], headnode );
	qprintf ("%5i nodes with faces\n", c_facenodes);
	qprintf ("%5i nodes without faces\n", c_nofaces);
	qprintf ("%5i faces\n", numfaces-oldfaces);
    qprintf( "%5i original faces\n", numorigfaces-oldorigfaces );
}



//===========================================================

/*
============
SetModelNumbers
============
*/
void SetModelNumbers (void)
{
	int		i;
	int		models;
	char	value[10];

	models = 1;
	for (i=1 ; i<num_entities ; i++)
	{
		if (!entities[i].numbrushes)
			continue;

		if ( !IsFuncOccluder(i) )
		{
			sprintf (value, "*%i", models);
			models++;
		}
		else
		{
			sprintf (value, "");
		}
		SetKeyValue (&entities[i], "model", value);
	}
}


/*
============
SetLightStyles
============
*/
#define	MAX_SWITCHED_LIGHTS	32
void SetLightStyles (void)
{
	int		stylenum;
	char	*t;
	entity_t	*e;
	int		i, j;
	char	value[10];
	char	lighttargets[MAX_SWITCHED_LIGHTS][64];


	// any light that is controlled (has a targetname)
	// must have a unique style number generated for it

	stylenum = 0;
	for (i=1 ; i<num_entities ; i++)
	{
		e = &entities[i];

		t = ValueForKey (e, "classname");
		if (Q_strncasecmp (t, "light", 5))
			continue;

		// This is not true for dynamic lights
		if (!Q_strcasecmp (t, "light_dynamic"))
			continue;

		t = ValueForKey (e, "targetname");
		if (!t[0])
			continue;
		
		// find this targetname
		for (j=0 ; j<stylenum ; j++)
			if (!strcmp (lighttargets[j], t))
				break;
		if (j == stylenum)
		{
			if (stylenum == MAX_SWITCHED_LIGHTS)
				Error ("Too many switched lights (error at light %s), max = %d", t, MAX_SWITCHED_LIGHTS);
			strcpy (lighttargets[j], t);
			stylenum++;
		}
		sprintf (value, "%i", 32 + j);
		char *pCurrentStyle = ValueForKey( e, "style" );
		// the designer has set a default lightstyle as well as making the light switchable
		if ( pCurrentStyle )
		{
			int oldStyle = atoi(pCurrentStyle);
			if ( oldStyle != 0 )
			{
				// save off the default style so the game code can make a switchable copy of it
				SetKeyValue( e, "defaultstyle", pCurrentStyle );
			}
		}
		SetKeyValue (e, "style", value);
	}

}

/*
============
EmitBrushes
============
*/
void EmitBrushes (void)
{
	int			    i, j, bnum, s, x;
	dbrush_t	    *db;
	mapbrush_t		*b;
	dbrushside_t	*cp;
	Vector		    normal;
	vec_t		    dist;
	int			    planenum;

	numbrushsides = 0;
	numbrushes = g_MainMap->nummapbrushes;

	for (bnum=0 ; bnum<g_MainMap->nummapbrushes ; bnum++)
	{
		b = &g_MainMap->mapbrushes[bnum];
		db = &dbrushes[bnum];

		db->contents = b->contents;
		db->firstside = numbrushsides;
		db->numsides = b->numsides;
		for (j=0 ; j<b->numsides ; j++)
		{
			if (numbrushsides == MAX_MAP_BRUSHSIDES)
				Error ("MAX_MAP_BRUSHSIDES");
			cp = &dbrushsides[numbrushsides];
			numbrushsides++;
			cp->planenum = b->original_sides[j].planenum;
			cp->texinfo = b->original_sides[j].texinfo;
			if ( cp->texinfo == -1 )
			{
				cp->texinfo = g_MainMap->g_ClipTexinfo;
			}
			cp->bevel = b->original_sides[j].bevel;
		}

		// add any axis planes not contained in the brush to bevel off corners
		for (x=0 ; x<3 ; x++)
			for (s=-1 ; s<=1 ; s+=2)
			{
			// add the plane
				VectorCopy (vec3_origin, normal);
				normal[x] = s;
				if (s == -1)
					dist = -b->mins[x];
				else
					dist = b->maxs[x];
				planenum = g_MainMap->FindFloatPlane (normal, dist);
				for (i=0 ; i<b->numsides ; i++)
					if (b->original_sides[i].planenum == planenum)
						break;
				if (i == b->numsides)
				{
					if (numbrushsides >= MAX_MAP_BRUSHSIDES)
						Error ("MAX_MAP_BRUSHSIDES");

					dbrushsides[numbrushsides].planenum = planenum;
					dbrushsides[numbrushsides].texinfo =
						dbrushsides[numbrushsides-1].texinfo;
					numbrushsides++;
					db->numsides++;
				}
			}
	}
}



/*
==================
BeginBSPFile
==================
*/
void BeginBSPFile (void)
{
	// these values may actually be initialized
	// if the file existed when loaded, so clear them explicitly
	nummodels = 0;
	numfaces = 0;
	numnodes = 0;
	numbrushsides = 0;
	numvertexes = 0;
	numleaffaces = 0;
	numleafbrushes = 0;
	numsurfedges = 0;

	// edge 0 is not used, because 0 can't be negated
	numedges = 1;

	// leave vertex 0 as an error
	numvertexes = 1;

	// leave leaf 0 as an error
	numleafs = 1;
	dleafs[0].contents = CONTENTS_SOLID;

	// BUGBUG: This doesn't work!
#if 0
	// make a default empty leaf for the tracing code
	memset( &dleafs[1], 0, sizeof(dleafs[1]) );
	dleafs[1].contents = CONTENTS_EMPTY;
#endif
}

// We can't calculate this properly until vvis (since we need vis to do this), so we set
// to zero everywhere by default.
static void ClearDistToClosestWater( void )
{
	int i;
	for( i = 0; i < numleafs; i++ )
	{
		g_LeafMinDistToWater[i] = 0;
	}
}


void DiscoverMacroTextures()
{
	CUtlDict<int,int> tempDict;
	
	g_FaceMacroTextureInfos.SetSize( numfaces );
	for ( int iFace=0; iFace < numfaces; iFace++ )
	{
		texinfo_t *pTexInfo = &texinfo[dfaces[iFace].texinfo];
		if ( pTexInfo->texdata < 0 )
			continue;

		dtexdata_t *pTexData = &dtexdata[pTexInfo->texdata];
		const char *pMaterialName = &g_TexDataStringData[ g_TexDataStringTable[pTexData->nameStringTableID] ];
		
		MaterialSystemMaterial_t hMaterial = FindMaterial( pMaterialName, NULL, false );

		const char *pMacroTextureName = GetMaterialVar( hMaterial, "$macro_texture" );
		if ( pMacroTextureName )
		{
			if ( tempDict.Find( pMacroTextureName ) == tempDict.InvalidIndex() )
			{
				Msg( "-- DiscoverMacroTextures: %s\n", pMacroTextureName );
				tempDict.Insert( pMacroTextureName, 0 );
			}

			int stringID = TexDataStringTable_AddOrFindString( pMacroTextureName );
			g_FaceMacroTextureInfos[iFace].m_MacroTextureNameID = (unsigned short)stringID;
		}
		else
		{
			g_FaceMacroTextureInfos[iFace].m_MacroTextureNameID = 0xFFFF;		
		}
	}
}


// Make sure that we have a water lod control entity if we have water in the map.
void EnsurePresenceOfWaterLODControlEntity( void )
{
	extern bool g_bHasWater;
	if( !g_bHasWater )
	{
		// Don't bother if there isn't any water in the map.
		return;
	}
	for( int i=0; i < num_entities; i++ )
	{
		entity_t *e = &entities[i];

		const char *pClassName = ValueForKey( e, "classname" );
		if( !Q_stricmp( pClassName, "water_lod_control" ) )
		{
			// Found one!!!!
			return;
		}
	}

	// None found, add one.
	Warning( "Water found with no water_lod_control entity, creating a default one.\n" );

	entity_t *mapent = &entities[num_entities];
	num_entities++;
	memset(mapent, 0, sizeof(*mapent));
	mapent->firstbrush = g_MainMap->nummapbrushes;
	mapent->numbrushes = 0;

	SetKeyValue( mapent, "classname", "water_lod_control" );
	SetKeyValue( mapent, "cheapwaterstartdistance", "1000" );
	SetKeyValue( mapent, "cheapwaterenddistance", "2000" );
}


/*
============
EndBSPFile
============
*/
void EndBSPFile (void)
{
	// Mark noshadow faces.
	MarkNoShadowFaces();

	EmitBrushes ();
	EmitPlanes ();

	// stick flat normals at the verts
	SaveVertexNormals();

	// Figure out lightmap extents for all faces.
	UpdateAllFaceLightmapExtents();

	// Generate geometry and lightmap alpha for displacements.
	EmitDispLMAlphaAndNeighbors();

	// Emit overlay data.
	Overlay_EmitOverlayFaces();
	OverlayTransition_EmitOverlayFaces();

	// phys collision needs dispinfo to operate (needs to generate phys collision for displacement surfs)
	EmitPhysCollision();

	// We can't calculate this properly until vvis (since we need vis to do this), so we set
	// to zero everywhere by default.
	ClearDistToClosestWater();

	// Emit static props found in the .vmf file
	EmitStaticProps();

	// Place detail props found in .vmf and based on material properties
	EmitDetailObjects();

	// Compute bounds after creating disp info because we need to reference it
	ComputeBoundsNoSkybox();
	
	// Make sure that we have a water lod control eneity if we have water in the map.
	EnsurePresenceOfWaterLODControlEntity();

	// Doing this here because stuff about may filter out entities
	UnparseEntities ();
	
	// remove unused texinfos
	CompactTexinfos();

	// Figure out which faces want macro textures.
	DiscoverMacroTextures();
	
	char	targetPath[1024];
	GetPlatformMapPath( source, targetPath, g_nDXLevel, 1024 );
	Msg ("Writing %s\n", targetPath);
	WriteBSPFile (targetPath);
}


/*
==================
BeginModel
==================
*/
int	firstmodleaf;
void BeginModel (void)
{
	dmodel_t	*mod;
	int			start, end;
	mapbrush_t	*b;
	int			j;
	entity_t	*e;
	Vector		mins, maxs;

	if (nummodels == MAX_MAP_MODELS)
		Error ("Too many brush models in map, max = %d", MAX_MAP_MODELS);
	mod = &dmodels[nummodels];

	mod->firstface = numfaces;

	firstmodleaf = numleafs;
	firstmodeledge = numedges;
	firstmodelface = numfaces;

	//
	// bound the brushes
	//
	e = &entities[entity_num];

	start = e->firstbrush;
	end = start + e->numbrushes;
	ClearBounds (mins, maxs);

	for (j=start ; j<end ; j++)
	{
		b = &g_MainMap->mapbrushes[j];
		if (!b->numsides)
			continue;	// not a real brush (origin brush)
		AddPointToBounds (b->mins, mins, maxs);
		AddPointToBounds (b->maxs, mins, maxs);
	}

	VectorCopy (mins, mod->mins);
	VectorCopy (maxs, mod->maxs);
}


/*
==================
EndModel
==================
*/
void EndModel (void)
{
	dmodel_t	*mod;

	mod = &dmodels[nummodels];

	mod->numfaces = numfaces - mod->firstface;

	nummodels++;
}



//-----------------------------------------------------------------------------
// figure out which leaf a point is in
//-----------------------------------------------------------------------------
static int PointLeafnum_r (const Vector& p, int num)
{
	float		d;
	while (num >= 0)
	{
		dnode_t* node = dnodes + num;
		dplane_t* plane = dplanes + node->planenum;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return -1 - num;
}

int PointLeafnum ( dmodel_t* pModel, const Vector& p )
{
	return PointLeafnum_r (p, pModel->headnode);
}


//-----------------------------------------------------------------------------
// Adds a noew to the bounding box
//-----------------------------------------------------------------------------
static void AddNodeToBounds(int node, CUtlVector<int>& skipAreas, Vector& mins, Vector& maxs)
{
	// not a leaf
	if (node >= 0)
	{
		AddNodeToBounds( dnodes[node].children[0], skipAreas, mins, maxs );
		AddNodeToBounds( dnodes[node].children[1], skipAreas, mins, maxs );
	}
	else
	{
		int leaf = - 1 - node;

		// Don't bother with solid leaves
		if (dleafs[leaf].contents & CONTENTS_SOLID)
			return;

		// Skip 3D skybox
		int i;
		for ( i = skipAreas.Count(); --i >= 0; )
		{
			if (dleafs[leaf].area == skipAreas[i])
				return;
		}

		unsigned int firstface = dleafs[leaf].firstleafface;
		for ( i = 0; i < dleafs[leaf].numleaffaces; ++i )
		{
			unsigned int face = dleaffaces[ firstface + i ];

			// Skip skyboxes + nodraw
			texinfo_t& tex = texinfo[dfaces[face].texinfo];
			if (tex.flags & (SURF_SKY | SURF_NODRAW))
				continue;

			unsigned int firstedge = dfaces[face].firstedge;
			Assert( firstedge >= 0 );

			for (int j = 0; j < dfaces[face].numedges; ++j)
			{
				Assert( firstedge+j < numsurfedges );
				int edge = abs(dsurfedges[firstedge+j]); 
				dedge_t* pEdge = &dedges[edge];
				Assert( pEdge->v[0] >= 0 );
				Assert( pEdge->v[1] >= 0 );
				AddPointToBounds (dvertexes[pEdge->v[0]].point, mins, maxs);
				AddPointToBounds (dvertexes[pEdge->v[1]].point, mins, maxs);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Check to see if a displacement lives in any leaves that are not
// in the 3d skybox
//-----------------------------------------------------------------------------
bool IsBoxInsideWorld( int node, CUtlVector<int> &skipAreas, const Vector &vecMins, const Vector &vecMaxs )
{
	while( 1 )
	{			
		// leaf
		if (node < 0)
		{
			// get the leaf
			int leaf = - 1 - node;

			// Don't bother with solid leaves
			if (dleafs[leaf].contents & CONTENTS_SOLID)
				return false;

			// Skip 3D skybox
			int i;
			for ( i = skipAreas.Count(); --i >= 0; )
			{
				if ( dleafs[leaf].area == skipAreas[i] )
					return false;
			}
			
			return true;
		}

		//
		// get displacement bounding box position relative to the node plane
		//
		dnode_t *pNode = &dnodes[ node ];
		dplane_t *pPlane = &dplanes[ pNode->planenum ];

        int sideResult = BrushBspBoxOnPlaneSide( vecMins, vecMaxs, pPlane );

        // front side
        if( sideResult == 1 )
        {
			node = pNode->children[0];
        }
        // back side
        else if( sideResult == 2 )
        {
			node = pNode->children[1];
        }
        //split
        else
        {
			if ( IsBoxInsideWorld( pNode->children[0], skipAreas, vecMins, vecMaxs ) )
				return true;

			node = pNode->children[1];
        }
	}
}


//-----------------------------------------------------------------------------
// Adds the displacement surfaces in the world to the bounds
//-----------------------------------------------------------------------------
void AddDispsToBounds( int nHeadNode, CUtlVector<int>& skipAreas, Vector &vecMins, Vector &vecMaxs )
{
	Vector vecDispMins, vecDispMaxs;

	// first determine how many displacement surfaces there will be per leaf
	int i;
	for ( i = 0; i < g_dispinfo.Count(); ++i )
	{
		ComputeDispInfoBounds( i, vecDispMins, vecDispMaxs );
		if ( IsBoxInsideWorld( nHeadNode, skipAreas, vecDispMins, vecDispMaxs ) )
		{
			AddPointToBounds( vecDispMins, vecMins, vecMaxs );
			AddPointToBounds( vecDispMaxs, vecMins, vecMaxs );
		}
	}
}


//-----------------------------------------------------------------------------
// Compute the bounding box, excluding 3D skybox + skybox, add it to keyvalues
//-----------------------------------------------------------------------------
void ComputeBoundsNoSkybox( )
{
	// Iterate over all world leaves, skip those which are part of skybox
	Vector mins, maxs;
	ClearBounds (mins, maxs);
	AddNodeToBounds( dmodels[0].headnode, g_SkyAreas, mins, maxs );
	AddDispsToBounds( dmodels[0].headnode, g_SkyAreas, mins, maxs );

	// Add the bounds to the worldspawn data
	for (int i = 0; i < num_entities; ++i)
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "worldspawn"))
		{
			char	string[32];
			sprintf (string, "%i %i %i", (int)mins[0], (int)mins[1], (int)mins[2]);
			SetKeyValue (&entities[i], "world_mins", string);
			sprintf (string, "%i %i %i", (int)maxs[0], (int)maxs[1], (int)maxs[2]);
			SetKeyValue (&entities[i], "world_maxs", string);
			break;
		}
	}
}


