//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Builds/merges the BSP tree of detail brushes
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "detail.h"
#include "utlvector.h"
#include <assert.h>

face_t *NewFaceFromFace (face_t *f);
face_t *ComputeVisibleBrushSides( bspbrush_t *list );

//-----------------------------------------------------------------------------
// Purpose: Copies a face and its winding
// Input  : *pFace - 
// Output : face_t
//-----------------------------------------------------------------------------
face_t *CopyFace( face_t *pFace )
{
	face_t *f = NewFaceFromFace( pFace );
	f->w = CopyWinding( pFace->w );

	return f;
}

//-----------------------------------------------------------------------------
// Purpose: Link this brush into the list for this leaf
// Input  : *node - 
//			*brush - 
//-----------------------------------------------------------------------------
void AddBrushToLeaf( node_t *node, bspbrush_t *brush )
{
	brush->next = node->brushlist;
	node->brushlist = brush;
}

//-----------------------------------------------------------------------------
// Purpose: Recursively filter a brush through the tree
// Input  : *node - 
//			*brush - 
//-----------------------------------------------------------------------------
void MergeBrush_r( node_t *node, bspbrush_t *brush )
{
	if ( node->planenum == PLANENUM_LEAF )
	{
		if ( node->contents & CONTENTS_SOLID )
		{
			FreeBrush( brush );
		}
		else
		{
			AddBrushToLeaf( node, brush );
		}
		return;
	}

	bspbrush_t *front, *back;
	SplitBrush( brush, node->planenum, &front, &back );
	FreeBrush( brush );

	if ( front )
	{
		MergeBrush_r( node->children[0], front );
	}
	if ( back )
	{
		MergeBrush_r( node->children[1], back );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Recursively filter a face into the tree leaving references to the
//			original face in any visible leaves that a clipped fragment falls
//			into.
// Input  : *node - current head of tree
//			*face - clipped face fragment
//			*original - unclipped original face
// Output : Returns true if any references were left
//-----------------------------------------------------------------------------
bool MergeFace_r( node_t *node, face_t *face, face_t *original )
{
	bool referenced = false;

	if ( node->planenum == PLANENUM_LEAF )
	{
		if ( node->contents & CONTENTS_SOLID )
		{
			FreeFace( face );
			return false;
		}

		leafface_t *plist = new leafface_t;
		plist->pFace = original;
		plist->pNext = node->leaffacelist;
		node->leaffacelist = plist;

		referenced = true;
	}
	else
	{
		// UNDONE: Don't copy the faces each time unless it's necessary!?!?!
		plane_t *plane = &g_MainMap->mapplanes[node->planenum];
		winding_t *frontwinding, *backwinding, *onwinding;

		Vector offset;
		WindingCenter( face->w, offset );

		// UNDONE: Export epsilon from original face clipping code
		ClassifyWindingEpsilon_Offset(face->w, plane->normal, plane->dist, 0.001, &frontwinding, &backwinding, &onwinding, -offset);

		if ( onwinding )
		{
			// face is in the split plane, go down the appropriate side according to the facing direction
			assert( frontwinding == NULL );
			assert( backwinding == NULL );

			if ( DotProduct( g_MainMap->mapplanes[face->planenum].normal, g_MainMap->mapplanes[node->planenum].normal ) > 0 )
			{
				frontwinding = onwinding;
			}
			else
			{
				backwinding = onwinding;
			}
		}

		if ( frontwinding )
		{
			face_t *tmp = NewFaceFromFace( face );
			tmp->w = frontwinding;
			referenced = MergeFace_r( node->children[0], tmp, original );
		}
		if ( backwinding )
		{
			face_t *tmp = NewFaceFromFace( face );
			tmp->w = backwinding;
			bool test = MergeFace_r( node->children[1], tmp, original );
			referenced = referenced || test;
		}
	}
	FreeFace( face );

	return referenced;
}

//-----------------------------------------------------------------------------
// Purpose: Loop through each face and filter it into the tree
// Input  : *out - 
//			*pFaces - 
//-----------------------------------------------------------------------------
face_t *FilterFacesIntoTree( tree_t *out, face_t *pFaces )
{
	face_t *pLeafFaceList = NULL;
	for ( face_t *f = pFaces; f; f = f->next )
	{
		if( f->merged || f->split[0] || f->split[1] )
			continue;

		face_t *tmp = CopyFace( f );
		face_t *original = CopyFace( f );

		if ( MergeFace_r( out->headnode, tmp, original ) )
		{
			// clear out portal (comes from a different tree)
			original->portal = NULL;
			original->next = pLeafFaceList;
			pLeafFaceList = original;
		}
		else
		{
			FreeFace( original );
		}
	}

	return pLeafFaceList;
}


//-----------------------------------------------------------------------------
// Purpose: Splits the face list into faces from the same plane and tries to merge
//			them if possible
// Input  : **pFaceList - 
//-----------------------------------------------------------------------------
void TryMergeFaceList( face_t **pFaceList )
{
	face_t **pPlaneList = NULL;

	// divide the list into buckets by plane number
	pPlaneList = new face_t *[g_MainMap->nummapplanes];
	memset( pPlaneList, 0, sizeof(face_t *) * g_MainMap->nummapplanes );

	face_t *pFaces = *pFaceList;
	face_t *pOutput = NULL;

	while ( pFaces )
	{
		face_t *next = pFaces->next;

		// go ahead and delete the old split/merged faces
		if ( pFaces->merged || pFaces->split[0] || pFaces->split[1] )
		{
			Error("Split face in merge list!");
		}
		else
		{
			// add to the list for this plane
			pFaces->next = pPlaneList[pFaces->planenum];
			pPlaneList[pFaces->planenum] = pFaces;
		}

		pFaces = next;
	}

	// now merge each plane's list of faces
	int merged = 0;
	for ( int i = 0; i < g_MainMap->nummapplanes; i++ )
	{
		if ( pPlaneList[i] )
		{
			MergeFaceList( &pPlaneList[i] );
		}
		
		// move these over to the output face list
		face_t *list = pPlaneList[i];
		while ( list )
		{
			face_t *next = list->next;

			if ( list->merged )
				merged++;

			list->next = pOutput;
			pOutput = list;
			list = next;
		}
	}

	if ( merged )
	{
		Msg("\nMerged %d detail faces...", merged );
	}
	delete[] pPlaneList;

	*pFaceList = pOutput;
}


//-----------------------------------------------------------------------------
// Purpose: filter each brush in the list into the tree
// Input  : *out - 
//			*brushes - 
//-----------------------------------------------------------------------------
void FilterBrushesIntoTree( tree_t *out, bspbrush_t *brushes )
{
	// Merge all of the brushes into the world tree
	for ( bspbrush_t *plist = brushes; plist; plist = plist->next )
	{
		MergeBrush_r( out->headnode, CopyBrush(plist) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Build faces for the detail brushes and merge them into the BSP
// Input  : *worldtree - 
//			brush_start - 
//			brush_end - 
//-----------------------------------------------------------------------------
face_t *MergeDetailTree( tree_t *worldtree, int brush_start, int brush_end )
{
	int			start;
	bspbrush_t	*detailbrushes = NULL;
	face_t		*pFaces = NULL;
	face_t		*pLeafFaceList = NULL;

	// Grab the list of detail brushes
	detailbrushes = MakeBspBrushList (brush_start, brush_end, g_MainMap->map_mins, g_MainMap->map_maxs, ONLY_DETAIL );
	if (detailbrushes)
	{
		start = Plat_FloatTime();
		Msg("Chop Details...");
		// if there are detail brushes, chop them against each other
		if (!nocsg)
			detailbrushes = ChopBrushes (detailbrushes);

		Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );
		// Now mark the visible sides so we can eliminate all detail brush sides
		// that are covered by other detail brush sides
		// NOTE: This still leaves detail brush sides that are covered by the world. (these are removed in the merge operation)
		Msg("Find Visible Detail Sides...");
		pFaces = ComputeVisibleBrushSides( detailbrushes );
		TryMergeFaceList( &pFaces );
		SubdivideFaceList( &pFaces );
		Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );

		start = Plat_FloatTime();
		Msg("Merging details...");
		// Merge the detail solids and faces into the world tree
		// Merge all of the faces into the world tree
		pLeafFaceList = FilterFacesIntoTree( worldtree, pFaces );
		FilterBrushesIntoTree( worldtree, detailbrushes );

		FreeFaceList( pFaces );
		FreeBrushList(detailbrushes);

		Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );
	}

	return pLeafFaceList;
}


//-----------------------------------------------------------------------------
// Purpose: Quick overlap test for brushes
// Input  : *p1 - 
//			*p2 - 
// Output : Returns false if the brushes cannot intersect
//-----------------------------------------------------------------------------
bool BrushBoxOverlap( bspbrush_t *p1, bspbrush_t *p2 )
{
	if ( p1 == p2 )
		return false;

	for ( int i = 0; i < 3; i++ )
	{
		if ( p1->mins[i] > p2->maxs[i] || p1->maxs[i] < p2->mins[i] )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFace - input face to test
//			*pbrush - brush to clip face against
//			**pOutputList - list of faces clipped from pFace
// Output : Returns true if the brush completely clips the face
//-----------------------------------------------------------------------------
// NOTE: This assumes the brushes have already been chopped so that no solid space
// is enclosed by more than one brush!!
bool ClipFaceToBrush( face_t *pFace, bspbrush_t *pbrush, face_t **pOutputList )
{
	int planenum = pFace->planenum & (~1);
	int foundSide = -1;

	CUtlVector<int> sortedSides;

	int i;
	for ( i = 0; i < pbrush->numsides && foundSide < 0; i++ )
	{
		int bplane = pbrush->sides[i].planenum & (~1);
		if ( bplane == planenum )
			foundSide = i;
	}

	Vector offset = -0.5f * (pbrush->maxs + pbrush->mins);
	face_t *currentface = CopyFace( pFace );

	if ( foundSide >= 0 )
	{
		sortedSides.RemoveAll();
		for ( i = 0; i < pbrush->numsides; i++ )
		{
			// don't clip to bevels
			if ( pbrush->sides[i].bevel )
				continue;

			if ( g_MainMap->mapplanes[pbrush->sides[i].planenum].type <= PLANE_Z )
			{
				sortedSides.AddToHead( i );
			}
			else
			{
				sortedSides.AddToTail( i );
			}
		}

		for ( i = 0; i < sortedSides.Size(); i++ )
		{
			int index = sortedSides[i];
			if ( index == foundSide )
				continue;
			
			plane_t *plane = &g_MainMap->mapplanes[pbrush->sides[index].planenum];
			winding_t *frontwinding, *backwinding;
			ClipWindingEpsilon_Offset(currentface->w, plane->normal, plane->dist, 0.001, &frontwinding, &backwinding, offset);
			
			// only clip if some part of this face is on the back side of all brush sides
			if ( !backwinding || WindingIsTiny(backwinding))
			{
				FreeFaceList( *pOutputList );
				*pOutputList = NULL;
				break;
			}
			if ( frontwinding && !WindingIsTiny(frontwinding) )
			{
				// add this fragment to the return list
				// make a face for the fragment
				face_t *f = NewFaceFromFace( pFace );
				f->w = frontwinding;
				
				// link the fragment in
				f->next = *pOutputList;
				*pOutputList = f;
			}

			// update the current winding to be the part behind each plane
			FreeWinding( currentface->w );
			currentface->w = backwinding;
		}

		// free the bit that is left in solid or not clipped (if we broke out early)
		FreeFace( currentface );

		// if we made it all the way through and didn't produce any fragments then the whole face was clipped away
		if ( !*pOutputList && i == sortedSides.Size() )
		{
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Given an original side and chopped winding, make a face_t 
// Input  : *side - side of the original brush
//			*winding - winding for this face (portion of the side)
// Output : face_t
//-----------------------------------------------------------------------------
face_t *MakeBrushFace( side_t *originalSide, winding_t *winding )
{
	face_t *f = AllocFace();
	f->merged = NULL;
	f->split[0] = f->split[1] = NULL;
	f->w = CopyWinding( winding );
	f->originalface = originalSide;
	//
	// save material info
	//
	f->texinfo = originalSide->texinfo;
	f->dispinfo = -1;

	// save plane info
	f->planenum = originalSide->planenum;
	f->contents = originalSide->contents;

	return f;
}


//-----------------------------------------------------------------------------
// Purpose: Chop away sides that are inside other brushes.
//			Brushes have already been chopped up so that they do not overlap, 
//			they merely touch.
// Input  : *list - list of brushes
// Output : face_t * - list of visible faces (some marked bad/split)
//-----------------------------------------------------------------------------
// assumes brushes were chopped!


side_t *FindOriginalSide( mapbrush_t *mb, side_t *pBspSide )
{
	side_t *bestside = NULL;
	float bestdot = 0;

	plane_t *p1 = g_MainMap->mapplanes + pBspSide->planenum;

	for (int i=0 ; i<mb->numsides ; i++)
	{
		side_t *side = &mb->original_sides[i];
		if (side->bevel)
			continue;
		if (side->texinfo == TEXINFO_NODE)
			continue;		// non-visible
		if ((side->planenum&~1) == (pBspSide->planenum&~1))
		{	// exact match
			return mb->original_sides + i;
		}
		// see how close the match is
		plane_t *p2 = &g_MainMap->mapplanes[side->planenum&~1];
		float dot = DotProduct (p1->normal, p2->normal);
		if (dot > bestdot)
		{
			bestdot = dot;
			bestside = side;
		}
	}

	if ( !bestside )
	{
		Error( "Bad detail brush side\n" );
	}
	return bestside;
}

// Get a list of brushes from pBrushList that could cut faces on the source brush
int GetListOfCutBrushes( CUtlVector<bspbrush_t *> &out, bspbrush_t *pSourceBrush, bspbrush_t *pBrushList )
{
	mapbrush_t *mb = pSourceBrush->original;
	for ( bspbrush_t *walk = pBrushList; walk; walk = walk->next )
	{
		if ( walk == pSourceBrush )
			continue;

		// only clip to transparent brushes if the original brush is transparent
		if ( walk->original->contents & TRANSPARENT_CONTENTS )
		{
			if ( !(mb->contents & TRANSPARENT_CONTENTS) )
				continue;
		}

		// don't clip to clip brushes, etc.
		if ( !(walk->original->contents & ALL_VISIBLE_CONTENTS) )
			continue;

		// brushes overlap, test faces
		if ( !BrushBoxOverlap( pSourceBrush, walk ) )
			continue;

		out.AddToTail( walk );
	}
	return out.Count();
}

// Count the number of real (unsplit) faces in the list
static int CountFaceList( face_t *f )
{
	int count = 0;
	for ( ; f; f = f->next )
	{
		if ( f->split[0] )
			continue;
		count++;
	}

	return count;
}

// Clips f to a list of potential cutting brushes
// If f clips into new faces, returns the list of new faces in pOutputList
static void ClipFaceToBrushList( face_t *f, const CUtlVector<bspbrush_t *> &cutBrushes, face_t **pOutputList )
{
	*pOutputList = NULL;

	if ( f->split[0] )
		return;

	face_t *pClipList = CopyFace( f );
	pClipList->next = NULL;
	bool clipped = false;
	for ( int i = 0; i < cutBrushes.Count(); i++ )
	{
		bspbrush_t *cut = cutBrushes[i];
		for ( face_t *pCutFace = pClipList; pCutFace; pCutFace = pCutFace->next )
		{
			face_t *pClip = NULL;
			// already split, no need to clip
			if ( pCutFace->split[0] )
				continue;

			if ( ClipFaceToBrush( pCutFace, cut, &pClip ) )
			{
				clipped = true;
				// mark face bad, the brush clipped it away
				pCutFace->split[0] = pCutFace;
			}
			else if ( pClip )
			{
				clipped = true;
				// mark this face as split
				pCutFace->split[0] = pCutFace;

				// insert face fragments at head of list (UNDONE: reverses order, do we care?)
				while ( pClip )
				{
					face_t *next = pClip->next;
					pClip->next = pClipList;
					pClipList = pClip;
					pClip = next;
				}
			}
		}
	}
	if ( clipped )
	{
		*pOutputList = pClipList;
	}
	else
	{
		// didn't do any clipping, go ahead and free the copy of the face here.
		FreeFaceList( pClipList );
	}
}

// Compute a list of faces that are visible on the detail brush sides
face_t *ComputeVisibleBrushSides( bspbrush_t *list )
{
	face_t *pTotalFaces = NULL;
	CUtlVector<bspbrush_t *> cutBrushes;

	// Go through the whole brush list
	for ( bspbrush_t *pbrush = list; pbrush; pbrush = pbrush->next )
	{
		face_t *pFaces = NULL;
		mapbrush_t *mb = pbrush->original;

		if ( !(mb->contents & ALL_VISIBLE_CONTENTS) )
			continue;

		// Make a face for each brush side, then clip it by the other
		// details to see if any fragments are visible
		for ( int i = 0; i < pbrush->numsides; i++ )
		{
			winding_t *winding = pbrush->sides[i].winding;
			if ( !winding )
				continue;
			
			if (! (pbrush->sides[i].contents & ALL_VISIBLE_CONTENTS) )
				continue;

			side_t *side = FindOriginalSide( mb, pbrush->sides + i );
			face_t *f = MakeBrushFace( side, winding );

			// link to head of face list
			f->next = pFaces;
			pFaces = f;
		}

		// Make a list of brushes that can cut the face list for this brush
		cutBrushes.RemoveAll();
		if ( GetListOfCutBrushes( cutBrushes, pbrush, list ) )
		{
			// now cut each face to find visible fragments
			for ( face_t *f = pFaces; f; f = f->next )
			{
				// this will be a new list of faces that this face cuts into
				face_t *pClip = NULL;
				ClipFaceToBrushList( f, cutBrushes, &pClip );
				if ( pClip )
				{
					int outCount = CountFaceList(pClip);
					// it cut into more faces (or it was completely cut away)
					if ( outCount <= 1 )
					{
						// was removed or cut down, mark as split
						f->split[0] = f;
						// insert face fragments at head of list (UNDONE: reverses order, do we care?)
						while ( pClip )
						{
							face_t *next = pClip->next;
							pClip->next = pFaces;
							pFaces = pClip;
							pClip = next;
						}
					}
					else
					{
						// it cut into more than one visible fragment
						// Don't fragment details
						// UNDONE: Build 2d convex hull of this list and swap face winding 
						// with that polygon?  That would fix the remaining issues.
						FreeFaceList( pClip );
						pClip = NULL;
					}
				}
			}
		}
	
		// move visible fragments to global face list
		while ( pFaces )
		{
			face_t *next = pFaces->next;
			if ( pFaces->split[0] )
			{
				FreeFace( pFaces );
			}
			else
			{
				pFaces->next = pTotalFaces;
				pTotalFaces = pFaces;
			}
			pFaces = next;
		}
	}

	return pTotalFaces;
}
