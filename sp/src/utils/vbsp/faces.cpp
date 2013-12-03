//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// faces.c

#include "vbsp.h"
#include "utlvector.h"
#include "utilmatlib.h"
#include <float.h>
#include "mstristrip.h"
#include "tier1/strtools.h"
#include "materialpatch.h"
/*

  some faces will be removed before saving, but still form nodes:

  the insides of sky volumes
  meeting planes of different water current volumes

*/

// undefine for dumb linear searches
#define	USE_HASHING

#define	INTEGRAL_EPSILON	0.01
#define	POINT_EPSILON		0.1
#define	OFF_EPSILON			0.25

int	c_merge;
int	c_subdivide;

int	c_totalverts;
int	c_uniqueverts;
int	c_degenerate;
int	c_tjunctions;
int	c_faceoverflows;
int	c_facecollapse;
int	c_badstartverts;

#define	MAX_SUPERVERTS	512
int	superverts[MAX_SUPERVERTS];
int	numsuperverts;

face_t		*edgefaces[MAX_MAP_EDGES][2];
int		firstmodeledge = 1;
int		firstmodelface;

int	c_tryedges;

Vector	edge_dir;
Vector	edge_start;
vec_t	edge_len;

int		num_edge_verts;
int		edge_verts[MAX_MAP_VERTS];


float	g_maxLightmapDimension = 32;


face_t *NewFaceFromFace (face_t *f);

// Used to speed up GetEdge2(). Holds a list of edges connected to each vert.
CUtlVector<int> g_VertEdgeList[MAX_MAP_VERTS];


//===========================================================================

typedef struct hashvert_s
{
	struct hashvert_s	*next;
	int		num;
} hashvert_t;

#define HASH_BITS	7
#define	HASH_SIZE	(COORD_EXTENT>>HASH_BITS)


int	vertexchain[MAX_MAP_VERTS];		// the next vertex in a hash chain
int	hashverts[HASH_SIZE*HASH_SIZE];	// a vertex number, or 0 for no verts

//face_t		*edgefaces[MAX_MAP_EDGES][2];

//============================================================================


unsigned HashVec (Vector& vec)
{
	int			x, y;

	x = (MAX_COORD_INTEGER + (int)(vec[0]+0.5)) >> HASH_BITS;
	y = (MAX_COORD_INTEGER + (int)(vec[1]+0.5)) >> HASH_BITS;

	if ( x < 0 || x >= HASH_SIZE || y < 0 || y >= HASH_SIZE )
		Error ("HashVec: point outside valid range");
	
	return y*HASH_SIZE + x;
}

#ifdef USE_HASHING
/*
=============
GetVertex

Uses hashing
=============
*/
int	GetVertexnum (Vector& in)
{
	int			h;
	int			i;
	Vector		vert;
	int			vnum;

	c_totalverts++;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(in[i] - (int)(in[i]+0.5)) < INTEGRAL_EPSILON)
			vert[i] = (int)(in[i]+0.5);
		else
			vert[i] = in[i];
	}
	
	h = HashVec (vert);
	
	for (vnum=hashverts[h] ; vnum ; vnum=vertexchain[vnum])
	{
		Vector& p = dvertexes[vnum].point;
		if ( fabs(p[0]-vert[0])<POINT_EPSILON
		&& fabs(p[1]-vert[1])<POINT_EPSILON
		&& fabs(p[2]-vert[2])<POINT_EPSILON )
			return vnum;
	}
	
// emit a vertex
	if (numvertexes == MAX_MAP_VERTS)
		Error ("Too many unique verts, max = %d (map has too much brush geometry)\n", MAX_MAP_VERTS);

	dvertexes[numvertexes].point[0] = vert[0];
	dvertexes[numvertexes].point[1] = vert[1];
	dvertexes[numvertexes].point[2] = vert[2];

	vertexchain[numvertexes] = hashverts[h];
	hashverts[h] = numvertexes;

	c_uniqueverts++;

	numvertexes++;
		
	return numvertexes-1;
}
#else
/*
==================
GetVertexnum

Dumb linear search
==================
*/
int	GetVertexnum (Vector& v)
{
	int			i, j;
	dvertex_t	*dv;
	vec_t		d;

	c_totalverts++;

	// make really close values exactly integral
	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(v[i] - (int)(v[i]+0.5)) < INTEGRAL_EPSILON )
			v[i] = (int)(v[i]+0.5);
		if (v[i] < MIN_COORD_INTEGER || v[i] > MAX_COORD_INTEGER)
			Error ("GetVertexnum: outside world, vertex %.1f %.1f %.1f", v.x, v.y, v.z);
	}

	// search for an existing vertex match
	for (i=0, dv=dvertexes ; i<numvertexes ; i++, dv++)
	{
		for (j=0 ; j<3 ; j++)
		{
			d = v[j] - dv->point[j];
			if ( d > POINT_EPSILON || d < -POINT_EPSILON)
				break;
		}
		if (j == 3)
			return i;		// a match
	}

	// new point
	if (numvertexes == MAX_MAP_VERTS)
		Error ("Too many unique verts, max = %d (map has too much brush geometry)\n", MAX_MAP_VERTS);
	VectorCopy (v, dv->point);
	numvertexes++;
	c_uniqueverts++;

	return numvertexes-1;
}
#endif


/*
==================
FaceFromSuperverts

The faces vertexes have beeb added to the superverts[] array,
and there may be more there than can be held in a face (MAXEDGES).

If less, the faces vertexnums[] will be filled in, otherwise
face will reference a tree of split[] faces until all of the
vertexnums can be added.

superverts[base] will become face->vertexnums[0], and the others
will be circularly filled in.
==================
*/
void FaceFromSuperverts (face_t **pListHead, face_t *f, int base)
{
	face_t	*newf;
	int		remaining;
	int		i;

	remaining = numsuperverts;
	while (remaining > MAXEDGES)
	{	// must split into two faces, because of vertex overload
		c_faceoverflows++;

		newf = NewFaceFromFace (f);
		f->split[0] = newf;

		newf->next = *pListHead;
		*pListHead = newf;

		newf->numpoints = MAXEDGES;
		for (i=0 ; i<MAXEDGES ; i++)
			newf->vertexnums[i] = superverts[(i+base)%numsuperverts];

		f->split[1] = NewFaceFromFace (f);
		f = f->split[1];

		f->next = *pListHead;
		*pListHead = f;

		remaining -= (MAXEDGES-2);
		base = (base+MAXEDGES-1)%numsuperverts;
	}

	// copy the vertexes back to the face
	f->numpoints = remaining;
	for (i=0 ; i<remaining ; i++)
		f->vertexnums[i] = superverts[(i+base)%numsuperverts];
}


/*
==================
EmitFaceVertexes
==================
*/
void EmitFaceVertexes (face_t **pListHead, face_t *f)
{
	winding_t	*w;
	int			i;

	if (f->merged || f->split[0] || f->split[1])
		return;

	w = f->w;
	for (i=0 ; i<w->numpoints ; i++)
	{
		if (noweld)
		{	// make every point unique
			if (numvertexes == MAX_MAP_VERTS)
				Error ("Too many unique verts, max = %d (map has too much brush geometry)\n", MAX_MAP_VERTS);
			superverts[i] = numvertexes;
			VectorCopy (w->p[i], dvertexes[numvertexes].point);
			numvertexes++;
			c_uniqueverts++;
			c_totalverts++;
		}
		else
			superverts[i] = GetVertexnum (w->p[i]);
	}
	numsuperverts = w->numpoints;

	// this may fragment the face if > MAXEDGES
	FaceFromSuperverts (pListHead, f, 0);
}

/*
==================
EmitNodeFaceVertexes_r
==================
*/
void EmitNodeFaceVertexes_r (node_t *node)
{
	int		i;
	face_t	*f;

	if (node->planenum == PLANENUM_LEAF)
	{
		// leaf faces are emitted in second pass
		return;
	}

	for (f=node->faces ; f ; f=f->next)
	{
		EmitFaceVertexes (&node->faces, f);
	}

	for (i=0 ; i<2 ; i++)
	{
		EmitNodeFaceVertexes_r (node->children[i]);
	}
}

void EmitLeafFaceVertexes( face_t **ppLeafFaceList )
{
	face_t *f = *ppLeafFaceList;

	while ( f )
	{
		EmitFaceVertexes( ppLeafFaceList, f );
		f = f->next;
	}
}


#ifdef USE_HASHING
/*
==========
FindEdgeVerts

Uses the hash tables to cut down to a small number
==========
*/
void FindEdgeVerts (Vector& v1, Vector& v2)
{
	int		x1, x2, y1, y2, t;
	int		x, y;
	int		vnum;

#if 0
{
	int		i;
	num_edge_verts = numvertexes-1;
	for (i=0 ; i<numvertexes-1 ; i++)
		edge_verts[i] = i+1;
}
#endif

	x1 = (MAX_COORD_INTEGER + (int)(v1[0]+0.5)) >> HASH_BITS;
	y1 = (MAX_COORD_INTEGER + (int)(v1[1]+0.5)) >> HASH_BITS;
	x2 = (MAX_COORD_INTEGER + (int)(v2[0]+0.5)) >> HASH_BITS;
	y2 = (MAX_COORD_INTEGER + (int)(v2[1]+0.5)) >> HASH_BITS;

	if (x1 > x2)
	{
		t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2)
	{
		t = y1;
		y1 = y2;
		y2 = t;
	}
#if 0
	x1--;
	x2++;
	y1--;
	y2++;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= HASH_SIZE)
		x2 = HASH_SIZE;
	if (y1 < 0)
		y1 = 0;
	if (y2 >= HASH_SIZE)
		y2 = HASH_SIZE;
#endif
	num_edge_verts = 0;
	for (x=x1 ; x <= x2 ; x++)
	{
		for (y=y1 ; y <= y2 ; y++)
		{
			for (vnum=hashverts[y*HASH_SIZE+x] ; vnum ; vnum=vertexchain[vnum])
			{
				edge_verts[num_edge_verts++] = vnum;
			}
		}
	}
}

#else
/*
==========
FindEdgeVerts

Forced a dumb check of everything
==========
*/
void FindEdgeVerts (Vector& v1, Vector& v2)
{
	int		i;

	num_edge_verts = numvertexes-1;
	for (i=0 ; i<num_edge_verts ; i++)
		edge_verts[i] = i+1;
}
#endif

/*
==========
TestEdge

Can be recursively reentered
==========
*/
void TestEdge (vec_t start, vec_t end, int p1, int p2, int startvert)
{
	int		j, k;
	vec_t	dist;
	Vector	delta;
	Vector	exact;
	Vector	off;
	vec_t	error;
	Vector	p;

	if (p1 == p2)
	{
		c_degenerate++;
		return;		// degenerate edge
	}

	for (k=startvert ; k<num_edge_verts ; k++)
	{
		j = edge_verts[k];
		if (j==p1 || j == p2)
			continue;

		VectorCopy (dvertexes[j].point, p);

		VectorSubtract (p, edge_start, delta);
		dist = DotProduct (delta, edge_dir);
		if (dist <=start || dist >= end)
			continue;		// off an end
		VectorMA (edge_start, dist, edge_dir, exact);
		VectorSubtract (p, exact, off);
		error = off.Length();

		if (error > OFF_EPSILON)
			continue;		// not on the edge

		// break the edge
		c_tjunctions++;
		TestEdge (start, dist, p1, j, k+1);
		TestEdge (dist, end, j, p2, k+1);
		return;
	}

	// the edge p1 to p2 is now free of tjunctions
	if (numsuperverts >= MAX_SUPERVERTS)
		Error ("Edge with too many vertices due to t-junctions.  Max %d verts along an edge!\n", MAX_SUPERVERTS);
	superverts[numsuperverts] = p1;
	numsuperverts++;
}


// stores the edges that each vert is part of
struct face_vert_table_t
{
	face_vert_table_t()
	{
		edge0 = -1;
		edge1 = -1;
	}

	void AddEdge( int edge )
	{
		if ( edge0 == -1 )
		{
			edge0 = edge;
		}
		else
		{
			// can only have two edges
			Assert(edge1==-1);
			edge1 = edge;
		}
	}

	bool HasEdge( int edge ) const
	{
		if ( edge >= 0 )
		{
			if ( edge0 == edge || edge1 == edge )
				return true;
		}
		return false;
	}
	
	int	edge0;
	int	edge1;
};

// if these two verts share an edge, they must be collinear
bool IsDiagonal( const face_vert_table_t &v0, const face_vert_table_t &v1 )
{
	if ( v1.HasEdge(v0.edge0) || v1.HasEdge(v0.edge1) )
		return false;

	return true;
}


void Triangulate_r( CUtlVector<int> &out, const CUtlVector<int> &inIndices, const CUtlVector<face_vert_table_t> &poly )
{
	Assert( inIndices.Count() > 2 );

	// one triangle left, return
	if ( inIndices.Count() == 3 )
	{
		for ( int i = 0; i < inIndices.Count(); i++ )
		{
			out.AddToTail( inIndices[i] );
		}
		return;
	}

	// check each pair of verts and see if they are diagonal (not on a shared edge)
	// if so, split & recurse
	for ( int i = 0; i < inIndices.Count(); i++ )
	{
		int count = inIndices.Count();

		// i + count is myself, i + count-1 is previous, so we need to stop at i+count-2
		for ( int j = 2; j < count-1; j++ )
		{
			// if these two form a diagonal, split the poly along 
			// the diagonal and triangulate the two sub-polys
			int index = inIndices[i];
			int nextArray = (i+j)%count;
			int nextIndex = inIndices[nextArray];
			if ( IsDiagonal(poly[index], poly[nextIndex]) )
			{
				// add the poly up to the diagonal
				CUtlVector<int> in1;
				for ( int k = i; k != nextArray; k = (k+1)%count )
				{
					in1.AddToTail(inIndices[k]);
				}
				in1.AddToTail(nextIndex);

				// add the rest of the poly starting with the diagonal
				CUtlVector<int> in2;
				in2.AddToTail(index);
				for ( int l = nextArray; l != i; l = (l+1)%count )
				{
					in2.AddToTail(inIndices[l]);
				}

				// triangulate the sub-polys
				Triangulate_r( out, in1, poly );
				Triangulate_r( out, in2, poly );
				return;
			}
		}
	}

	// didn't find a diagonal
	Assert(0);
}

/*
==================
FixFaceEdges

==================
*/
void FixFaceEdges (face_t **pList, face_t *f)
{
	int		p1, p2;
	int		i;
	Vector	e2;
	vec_t	len;
	int		count[MAX_SUPERVERTS], start[MAX_SUPERVERTS];
	int		base;

	if (f->merged || f->split[0] || f->split[1])
		return;

	numsuperverts = 0;

	int originalPoints = f->numpoints;
	for (i=0 ; i<f->numpoints ; i++)
	{
		p1 = f->vertexnums[i];
		p2 = f->vertexnums[(i+1)%f->numpoints];

		VectorCopy (dvertexes[p1].point, edge_start);
		VectorCopy (dvertexes[p2].point, e2);

		FindEdgeVerts (edge_start, e2);

		VectorSubtract (e2, edge_start, edge_dir);
		len = VectorNormalize (edge_dir);

		start[i] = numsuperverts;
		TestEdge (0, len, p1, p2, 0);

		count[i] = numsuperverts - start[i];
	}

	if (numsuperverts < 3)
	{	// entire face collapsed
		f->numpoints = 0;
		c_facecollapse++;
		return;
	}

	// we want to pick a vertex that doesn't have tjunctions
	// on either side, which can cause artifacts on trifans,
	// especially underwater
	for (i=0 ; i<f->numpoints ; i++)
	{
		if (count[i] == 1 && count[(i+f->numpoints-1)%f->numpoints] == 1)
			break;
	}
	if (i == f->numpoints)
	{
		f->badstartvert = true;
		c_badstartverts++;
		base = 0;

	}
	else
	{	// rotate the vertex order
		base = start[i];
	}

	// this may fragment the face if > MAXEDGES
	FaceFromSuperverts (pList, f, base);

	// if this is the world, then re-triangulate to sew cracks
	if ( f->badstartvert && entity_num == 0 )
	{
		CUtlVector<face_vert_table_t> poly;
		CUtlVector<int> inIndices;
		CUtlVector<int> outIndices;
		poly.AddMultipleToTail( numsuperverts );
		for ( i = 0; i < originalPoints; i++ )
		{
			// edge may not have output any points.  Don't mark
			if ( !count[i] )
				continue;
			// mark each edge the point is a member of
			// we'll use this as a fast "is collinear" test
			for ( int j = 0; j <= count[i]; j++ )
			{
				int polyIndex = (start[i] + j) % numsuperverts;
				poly[polyIndex].AddEdge( i );
			}
		}
		for ( i = 0; i < numsuperverts; i++ )
		{
			inIndices.AddToTail( i );
		}
		Triangulate_r( outIndices, inIndices, poly );
		dprimitive_t &newPrim = g_primitives[g_numprimitives];
		f->firstPrimID = g_numprimitives;
		g_numprimitives++;
		f->numPrims = 1;
		newPrim.firstIndex = g_numprimindices;
		newPrim.firstVert = g_numprimverts;
		newPrim.indexCount = outIndices.Count();
		newPrim.vertCount = 0;
		newPrim.type = PRIM_TRILIST;
		g_numprimindices += newPrim.indexCount;
		if ( g_numprimitives > MAX_MAP_PRIMITIVES || g_numprimindices > MAX_MAP_PRIMINDICES )
		{
			Error("Too many t-junctions to fix up! (%d prims, max %d :: %d indices, max %d)\n", g_numprimitives, MAX_MAP_PRIMITIVES, g_numprimindices, MAX_MAP_PRIMINDICES );
		}
		for ( i = 0; i < outIndices.Count(); i++ )
		{
			g_primindices[newPrim.firstIndex + i] = outIndices[i];
		}
	}
}

/*
==================
FixEdges_r
==================
*/
void FixEdges_r (node_t *node)
{
	int		i;
	face_t	*f;

	if (node->planenum == PLANENUM_LEAF)
	{
		return;
	}

	for (f=node->faces ; f ; f=f->next)
		FixFaceEdges (&node->faces, f);

	for (i=0 ; i<2 ; i++)
		FixEdges_r (node->children[i]);
}


//-----------------------------------------------------------------------------
// Purpose: Fix the t-junctions on detail faces
//-----------------------------------------------------------------------------
void FixLeafFaceEdges( face_t **ppLeafFaceList )
{
	face_t *f;

	for ( f = *ppLeafFaceList; f; f = f->next )
	{
		FixFaceEdges( ppLeafFaceList, f );
	}
}

/*
===========
FixTjuncs

===========
*/

face_t *FixTjuncs (node_t *headnode, face_t *pLeafFaceList)
{
	// snap and merge all vertexes
	qprintf ("---- snap verts ----\n");
	memset (hashverts, 0, sizeof(hashverts));
	memset (vertexchain, 0, sizeof(vertexchain));
	c_totalverts = 0;
	c_uniqueverts = 0;
	c_faceoverflows = 0;
	EmitNodeFaceVertexes_r (headnode);

	// UNDONE: This count is wrong with tjuncs off on details - since 

	// break edges on tjunctions
	qprintf ("---- tjunc ----\n");
	c_tryedges = 0;
	c_degenerate = 0;
	c_facecollapse = 0;
	c_tjunctions = 0;
	
	if ( g_bAllowDetailCracks )
	{
		FixEdges_r (headnode);
		EmitLeafFaceVertexes( &pLeafFaceList );
		FixLeafFaceEdges( &pLeafFaceList );
	}
	else
	{
		EmitLeafFaceVertexes( &pLeafFaceList );
		if (!notjunc)
		{
			FixEdges_r (headnode);
			FixLeafFaceEdges( &pLeafFaceList );
		}
	}


	qprintf ("%i unique from %i\n", c_uniqueverts, c_totalverts);
	qprintf ("%5i edges degenerated\n", c_degenerate);
	qprintf ("%5i faces degenerated\n", c_facecollapse);
	qprintf ("%5i edges added by tjunctions\n", c_tjunctions);
	qprintf ("%5i faces added by tjunctions\n", c_faceoverflows);
	qprintf ("%5i bad start verts\n", c_badstartverts);

	return pLeafFaceList;
}


//========================================================

int		c_faces;

face_t	*AllocFace (void)
{
	static int s_FaceId = 0;

	face_t	*f;

	f = (face_t*)malloc(sizeof(*f));
	memset (f, 0, sizeof(*f));
	f->id = s_FaceId;
	++s_FaceId;

	c_faces++;

	return f;
}

face_t *NewFaceFromFace (face_t *f)
{
	face_t	*newf;

	newf = AllocFace ();
	*newf = *f;
	newf->merged = NULL;
	newf->split[0] = newf->split[1] = NULL;
	newf->w = NULL;
	return newf;
}

void FreeFace (face_t *f)
{
	if (f->w)
		FreeWinding (f->w);
	free (f);
	c_faces--;
}


void FreeFaceList( face_t *pFaces )
{
	while ( pFaces )
	{
		face_t *next = pFaces->next;

		FreeFace( pFaces );
		pFaces = next;
	}
}

//========================================================

void GetEdge2_InitOptimizedList()
{
	for( int i=0; i < MAX_MAP_VERTS; i++ )
		g_VertEdgeList[i].RemoveAll();
}


void IntSort( CUtlVector<int> &theList )
{
	for( int i=0; i < theList.Size()-1; i++ )
	{
		if( theList[i] > theList[i+1] )
		{
			int temp = theList[i];
			theList[i] = theList[i+1];
			theList[i+1] = temp;
			if( i > 0 )
				i -= 2;
			else
				i = -1;
		}
	}
}


int AddEdge( int v1, int v2, face_t *f )
{
	if (numedges >= MAX_MAP_EDGES)
		Error ("Too many edges in map, max == %d", MAX_MAP_EDGES);

	g_VertEdgeList[v1].AddToTail( numedges );
	g_VertEdgeList[v2].AddToTail( numedges );
	IntSort( g_VertEdgeList[v1] );
	IntSort( g_VertEdgeList[v2] );
			  
	dedge_t *edge = &dedges[numedges];
	numedges++;
    
    edge->v[0] = v1;
    edge->v[1] = v2;
    edgefaces[numedges-1][0] = f;
	return numedges - 1;
}


/*
==================
GetEdge

Called by writebsp.
Don't allow four way edges
==================
*/
int GetEdge2 (int v1, int v2,  face_t *f)
{
	dedge_t	*edge;

	c_tryedges++;

	if (!noshare)
	{
		// Check all edges connected to v1.
		CUtlVector<int> &theList = g_VertEdgeList[v1];
		for( int i=0; i < theList.Size(); i++ )
		{
			int iEdge = theList[i];
			edge = &dedges[iEdge];
			if (v1 == edge->v[1] && v2 == edge->v[0] && edgefaces[iEdge][0]->contents == f->contents)
			{
				if (edgefaces[iEdge][1])
					continue;

				edgefaces[iEdge][1] = f;
				return -iEdge;
			}
		}
	}

	return AddEdge( v1, v2, f );
}

/*
===========================================================================

FACE MERGING

===========================================================================
*/

#define	CONTINUOUS_EPSILON	0.001

/*
=============
TryMergeWinding

If two polygons share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the faces couldn't be merged, or the new face.
The originals will NOT be freed.
=============
*/
winding_t *TryMergeWinding (winding_t *f1, winding_t *f2, Vector& planenormal)
{
	Vector		*p1, *p2, *p3, *p4, *back;
	winding_t	*newf;
	int			i, j, k, l;
	Vector		normal, delta;
	vec_t		dot;
	qboolean	keep1, keep2;
	

	//
	// find a common edge
	//	
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;			// 
	
	for (i=0 ; i<f1->numpoints ; i++)
	{
		p1 = &f1->p[i];
		p2 = &f1->p[(i+1)%f1->numpoints];
		for (j=0 ; j<f2->numpoints ; j++)
		{
			p3 = &f2->p[j];
			p4 = &f2->p[(j+1)%f2->numpoints];
			for (k=0 ; k<3 ; k++)
			{
				if (fabs((*p1)[k] - (*p4)[k]) > EQUAL_EPSILON)
					break;
				if (fabs((*p2)[k] - (*p3)[k]) > EQUAL_EPSILON)
					break;
			}
			if (k==3)
				break;
		}
		if (j < f2->numpoints)
			break;
	}
	
	if (i == f1->numpoints)
		return NULL;			// no matching edges

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = &f1->p[(i+f1->numpoints-1)%f1->numpoints];
	VectorSubtract (*p1, *back, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);
	
	back = &f2->p[(j+2)%f2->numpoints];
	VectorSubtract (*back, *p1, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep1 = (qboolean)(dot < -CONTINUOUS_EPSILON);
	
	back = &f1->p[(i+2)%f1->numpoints];
	VectorSubtract (*back, *p2, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);

	back = &f2->p[(j+f2->numpoints-1)%f2->numpoints];
	VectorSubtract (*back, *p2, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep2 = (qboolean)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = AllocWinding (f1->numpoints + f2->numpoints);
	
	// copy first polygon
	for (k=(i+1)%f1->numpoints ; k != i ; k=(k+1)%f1->numpoints)
	{
		if (k==(i+1)%f1->numpoints && !keep2)
			continue;
		
		VectorCopy (f1->p[k], newf->p[newf->numpoints]);
		newf->numpoints++;
	}
	
	// copy second polygon
	for (l= (j+1)%f2->numpoints ; l != j ; l=(l+1)%f2->numpoints)
	{
		if (l==(j+1)%f2->numpoints && !keep1)
			continue;
		VectorCopy (f2->p[l], newf->p[newf->numpoints]);
		newf->numpoints++;
	}

	return newf;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool OverlaysAreEqual( face_t *f1, face_t *f2 )
{
	// Check the overlay ids - see if they are the same.
	if ( f1->originalface->aOverlayIds.Count() != f2->originalface->aOverlayIds.Count() )
		return false;

	int nOverlayCount = f1->originalface->aOverlayIds.Count();
	for ( int iOverlay = 0; iOverlay < nOverlayCount; ++iOverlay )
	{
		int nOverlayId = f1->originalface->aOverlayIds[iOverlay];
		if ( f2->originalface->aOverlayIds.Find( nOverlayId ) == -1 )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool FaceOnWaterBrush( face_t *face )
{
	side_t *pSide = face->originalface;
	if ( !pSide )
		return false;

	if ( pSide->contents & ( CONTENTS_WATER | CONTENTS_SLIME ) )
		return true;

	return false;
}

/*
=============
TryMerge

If two polygons share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the faces couldn't be merged, or the new face.
The originals will NOT be freed.
=============
*/
face_t *TryMerge (face_t *f1, face_t *f2, Vector& planenormal)
{
	face_t		*newf;
	winding_t	*nw;

	if (!f1->w || !f2->w)
		return NULL;
	if (f1->texinfo != f2->texinfo)
		return NULL;
	if (f1->planenum != f2->planenum)	// on front and back sides
		return NULL;
	if (f1->contents != f2->contents)
		return NULL;
    if ( f1->originalface->smoothingGroups != f2->originalface->smoothingGroups )
        return NULL;
	if ( !OverlaysAreEqual( f1, f2 ) )
		return NULL;
	if ( nomergewater && ( FaceOnWaterBrush( f1 ) || FaceOnWaterBrush( f2 ) ) )
		return NULL;

	nw = TryMergeWinding (f1->w, f2->w, planenormal);
	if (!nw)
		return NULL;

	c_merge++;
	newf = NewFaceFromFace (f1);
	newf->w = nw;

	f1->merged = newf;
	f2->merged = newf;

	return newf;
}

/*
===============
MergeFaceList
===============
*/
void MergeFaceList(face_t **pList)
{
	face_t	*f1, *f2, *end;
	face_t	*merged;
	plane_t	*plane;

	merged = NULL;
	
	for (f1 = *pList; f1 ; f1 = f1->next)
	{
		if (f1->merged || f1->split[0] || f1->split[1])
			continue;
		for (f2 = *pList; f2 != f1 ; f2=f2->next)
		{
			if (f2->merged || f2->split[0] || f2->split[1])
				continue;

			plane = &g_MainMap->mapplanes[f1->planenum];
			merged = TryMerge (f1, f2, plane->normal);
			if (!merged)
				continue;

			// add merged to the end of the face list 
			// so it will be checked against all the faces again
			for (end = *pList; end->next ; end = end->next)
			;
			merged->next = NULL;
			end->next = merged;
			break;
		}
	}
}

//=====================================================================

/*
===============
SubdivideFace

Chop up faces that are larger than we want in the surface cache
===============
*/
void SubdivideFace (face_t **pFaceList, face_t *f)
{
	float		mins, maxs;
	vec_t		v;
	vec_t		luxelsPerWorldUnit;
	int			axis, i;
	texinfo_t	*tex;
	Vector		temp;
	vec_t		dist;
	winding_t	*w, *frontw, *backw;

	if ( f->merged || f->split[0] || f->split[1] )
		return;

// special (non-surface cached) faces don't need subdivision
	tex = &texinfo[f->texinfo];

	if( tex->flags & SURF_NOLIGHT )
	{
		return;
	}

	for (axis = 0 ; axis < 2 ; axis++)
	{
		while (1)
		{
			mins = 999999;
			maxs = -999999;
			
			VECTOR_COPY (tex->lightmapVecsLuxelsPerWorldUnits[axis], temp);
			w = f->w;
			for (i=0 ; i<w->numpoints ; i++)
			{
				v = DotProduct (w->p[i], temp);
				if (v < mins)
					mins = v;
				if (v > maxs)
					maxs = v;
			}
#if 0
			if (maxs - mins <= 0)
				Error ("zero extents");
#endif
			if (maxs - mins <= g_maxLightmapDimension)
				break;
			
		// split it
			c_subdivide++;
			
			luxelsPerWorldUnit = VectorNormalize (temp);	

			dist = ( mins + g_maxLightmapDimension - 1 ) / luxelsPerWorldUnit;

			ClipWindingEpsilon (w, temp, dist, ON_EPSILON, &frontw, &backw);
			if (!frontw || !backw)
				Error ("SubdivideFace: didn't split the polygon");

			f->split[0] = NewFaceFromFace (f);
			f->split[0]->w = frontw;
			f->split[0]->next = *pFaceList;
			*pFaceList = f->split[0];

			f->split[1] = NewFaceFromFace (f);
			f->split[1]->w = backw;
			f->split[1]->next = *pFaceList;
			*pFaceList = f->split[1];

			SubdivideFace (pFaceList, f->split[0]);
			SubdivideFace (pFaceList, f->split[1]);
			return;
		}
	}
}

void SubdivideFaceList(face_t **pFaceList)
{
	face_t	*f;

	for (f = *pFaceList ; f ; f=f->next)
	{
		SubdivideFace (pFaceList, f);
	}
}


//-----------------------------------------------------------------------------
// Assigns the bottom material to the bottom face
//-----------------------------------------------------------------------------
static bool AssignBottomWaterMaterialToFace( face_t *f )
{
	// NOTE: This happens *after* cubemap fixup occurs, so we need to get the
	// fixed-up bottom material for this
	texinfo_t *pTexInfo = &texinfo[f->texinfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	const char *pMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );

	char pBottomMatName[512];
	if ( !GetValueFromPatchedMaterial( pMaterialName, "$bottommaterial", pBottomMatName, 512 ) )
	{
		if( !Q_stristr( pMaterialName, "nodraw" ) && !Q_stristr( pMaterialName, "toolsskip" ) )
		{
			Warning("error: material %s doesn't have a $bottommaterial\n", pMaterialName );
		}
		return false;
	}

	//Assert( mapplanes[f->planenum].normal.z < 0 );
	texinfo_t newTexInfo;
	newTexInfo.flags = pTexInfo->flags;
	int j, k;
	for (j=0 ; j<2 ; j++)
	{
		for (k=0 ; k<4 ; k++)
		{
			newTexInfo.textureVecsTexelsPerWorldUnits[j][k] = pTexInfo->textureVecsTexelsPerWorldUnits[j][k];
			newTexInfo.lightmapVecsLuxelsPerWorldUnits[j][k] = pTexInfo->lightmapVecsLuxelsPerWorldUnits[j][k];
		}
	}
	newTexInfo.texdata = FindOrCreateTexData( pBottomMatName );
	f->texinfo = FindOrCreateTexInfo( newTexInfo );

	return true;
}


//===========================================================================

int	c_nodefaces;

static void SubdivideFaceBySubdivSize( face_t *f, float subdivsize );
void SubdivideFaceBySubdivSize( face_t *f );

/*
============
FaceFromPortal

============
*/
extern int FindOrCreateTexInfo( const texinfo_t &searchTexInfo );

face_t *FaceFromPortal (portal_t *p, int pside)
{
	face_t	*f;
	side_t	*side;
	int		deltaContents;

    // portal does not bridge different visible contents
	side = p->side;
	if (!side)
		return NULL;	

    // allocate a new face
	f = AllocFace();

    // save the original "side" from the map brush -- portal->side
    // see FindPortalSide(...)
    f->originalface = side;

    //
    // save material info
    //
	f->texinfo = side->texinfo;
	f->dispinfo = -1;					// all faces with displacement info are created elsewhere
	f->smoothingGroups = side->smoothingGroups;

    // save plane info
	f->planenum = (side->planenum & ~1) | pside;
	if ( entity_num != 0 )
	{
		// the brush model renderer doesn't use PLANEBACK, so write the real plane
		// inside water faces can be flipped because they are generated on the inside of the brush
		if ( p->nodes[pside]->contents & (CONTENTS_WATER|CONTENTS_SLIME) )
		{
			f->planenum = (side->planenum & ~1) | pside;
		}
		else
		{
			f->planenum = side->planenum;
		}
	}

    // save portal info
	f->portal = p;
	f->fogVolumeLeaf = NULL;

	deltaContents = VisibleContents(p->nodes[!pside]->contents^p->nodes[pside]->contents);
	
	// don't show insides of windows or grates
	if ( ((p->nodes[pside]->contents & CONTENTS_WINDOW) && deltaContents == CONTENTS_WINDOW) ||
		((p->nodes[pside]->contents & CONTENTS_GRATE) && deltaContents == CONTENTS_GRATE) )
	{
		FreeFace( f );
		return NULL;
	}

	if ( p->nodes[pside]->contents & MASK_WATER )
	{
		f->fogVolumeLeaf = p->nodes[pside];
	}
	else if ( p->nodes[!pside]->contents & MASK_WATER )
	{
		f->fogVolumeLeaf = p->nodes[!pside];
	}

	// If it's the underside of water, we need to figure out what material to use, etc.
	if( ( p->nodes[pside]->contents & CONTENTS_WATER ) && deltaContents == CONTENTS_WATER )
	{
		if ( !AssignBottomWaterMaterialToFace( f ) )
		{
			FreeFace( f );
			return NULL;
		}
	}

    //
    // generate the winding for the face and save face contents
    //
	if( pside )
	{
		f->w = ReverseWinding(p->winding);
		f->contents = p->nodes[1]->contents;
	}
	else
	{
		f->w = CopyWinding(p->winding);
		f->contents = p->nodes[0]->contents;
	}

	f->numPrims = 0;
	f->firstPrimID = 0;
	
	// return the created face
	return f;
}

/*
===============
MakeFaces_r

If a portal will make a visible face,
mark the side that originally created it

  solid / empty : solid
  solid / water : solid
  water / empty : water
  water / water : none
===============
*/
void MakeFaces_r (node_t *node)
{
	portal_t	*p;
	int			s;

	// recurse down to leafs
	if (node->planenum != PLANENUM_LEAF)
	{
		MakeFaces_r (node->children[0]);
		MakeFaces_r (node->children[1]);

		// merge together all visible faces on the node
		if (!nomerge)
			MergeFaceList(&node->faces);
		if (!nosubdiv)
			SubdivideFaceList(&node->faces);

		return;
	}

	// solid leafs never have visible faces
	if (node->contents & CONTENTS_SOLID)
		return;

	// see which portals are valid
	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);

		p->face[s] = FaceFromPortal (p, s);
		if (p->face[s])
		{
			c_nodefaces++;
			p->face[s]->next = p->onnode->faces;
			p->onnode->faces = p->face[s];
		}
	}
}

typedef winding_t *pwinding_t;

static void PrintWinding( winding_t *w )
{
	int i;
	Msg( "\t---\n" );
	for( i = 0; i < w->numpoints; i++ )
	{
		Msg( "\t%f %f %f\n", w->p[i].x, w->p[i].y, w->p[i].z );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a winding to the current list of primverts
// Input  : *w - the winding
//			*pIndices - The output indices
//			vertStart - the starting vert index
//			vertCount - current count
// Output : int - output count including new verts from this winding
//-----------------------------------------------------------------------------
int AddWindingToPrimverts( const winding_t *w, unsigned short *pIndices, int vertStart, int vertCount )
{
	for( int i = 0; i < w->numpoints; i++ )
	{
		int j;
		for( j = vertStart; j < vertStart + vertCount; j++ )
		{
			Vector tmp = g_primverts[j].pos - w->p[i];

			if( tmp.LengthSqr() < POINT_EPSILON*POINT_EPSILON )
			{
				pIndices[i] = j;
				break;
			}
		}
		if ( j >= vertStart + vertCount )
		{
			pIndices[i] = j;
			g_primverts[j].pos = w->p[i];
			vertCount++;
			g_numprimverts++;
			if ( g_numprimverts > MAX_MAP_PRIMVERTS )
			{
				Error( "Exceeded max water verts.\nIncrease surface subdivision size or lower your subdivision size in vmt files! (%d>%d)\n", 
					( int )g_numprimverts, ( int )MAX_MAP_PRIMVERTS );
			}
		}
	}

	return vertCount;
}



#pragma optimize( "g", off )
#define USE_TRISTRIPS

// UNDONE: Should split this function into subdivide and primitive building parts
// UNDONE: We should try building strips of shared verts for all water faces in a leaf
//			since those will be drawn concurrently anyway.  It should be more efficient.
static void SubdivideFaceBySubdivSize( face_t *f, float subdivsize )
{
	// garymcthack - REFACTOR ME!!!
	
	vec_t dummy;
	Vector hackNormal;
	WindingPlane( f->w, hackNormal, &dummy );

	// HACK - only subdivide stuff that is facing up or down (for water)
	if( fabs(hackNormal[2]) < .9f )
	{
		return;
	}

	// Get the extents of the surface.
	// garymcthack - this assumes a surface of constant z for now (for water). . can generalize later.
	subdivsize = ( int )subdivsize;
	winding_t *w;
	w = CopyWinding( f->w );

	Vector min, max;
	WindingBounds( w, min, max );

#if 0
	Msg( "START WINDING: \n" );
	PrintWinding( w );
#endif
	int xStart, yStart, xEnd, yEnd, xSteps, ySteps;
	xStart = ( int )subdivsize * ( int )( ( min[0] - subdivsize ) / subdivsize );
	xEnd = ( int )subdivsize * ( int )( ( max[0] + subdivsize ) / subdivsize );
	yStart = ( int )subdivsize * ( int )( ( min[1] - subdivsize ) / subdivsize );
	yEnd = ( int )subdivsize * ( int )( ( max[1] + subdivsize ) / subdivsize );
	xSteps = ( xEnd - xStart ) / subdivsize;
	ySteps = ( yEnd - yStart ) / subdivsize;
	int x, y;
	int xi, yi;
	winding_t **windings = ( winding_t ** )new pwinding_t[xSteps * ySteps];
	memset( windings, 0, sizeof( winding_t * ) * xSteps * ySteps );

	for( yi = 0, y = yStart; y < yEnd; y += ( int )subdivsize, yi++ )
	{
		for( xi = 0, x = xStart; x < xEnd; x += ( int )subdivsize, xi++ )
		{
			winding_t *tempWinding, *frontWinding, *backWinding;
			float planeDist;
			Vector normal;
			normal.Init( 1.0f, 0.0f, 0.0f );
			planeDist = ( float )x;
			tempWinding = CopyWinding( w );
			ClipWindingEpsilon( tempWinding, normal, planeDist, ON_EPSILON, 
				&frontWinding, &backWinding );
			if( tempWinding )
			{
				FreeWinding( tempWinding );
			}
			if( backWinding )
			{
				FreeWinding( backWinding );
			}
			if( !frontWinding )
			{
				continue;
			}
			tempWinding = frontWinding;

			normal.Init( -1.0f, 0.0f, 0.0f );
			planeDist = -( float )( x + subdivsize );
			ClipWindingEpsilon( tempWinding, normal, planeDist, ON_EPSILON, 
				&frontWinding, &backWinding );
			if( tempWinding )
			{
				FreeWinding( tempWinding );
			}
			if( backWinding )
			{
				FreeWinding( backWinding );
			}
			if( !frontWinding )
			{
				continue;
			}
			tempWinding = frontWinding;

			normal.Init( 0.0f, 1.0f, 0.0f );
			planeDist = ( float )y;
			ClipWindingEpsilon( tempWinding, normal, planeDist, ON_EPSILON, 
				&frontWinding, &backWinding );
			if( tempWinding )
			{
				FreeWinding( tempWinding );
			}
			if( backWinding )
			{
				FreeWinding( backWinding );
			}
			if( !frontWinding )
			{
				continue;
			}
			tempWinding = frontWinding;

			normal.Init( 0.0f, -1.0f, 0.0f );
			planeDist = -( float )( y + subdivsize );
			ClipWindingEpsilon( tempWinding, normal, planeDist, ON_EPSILON, 
				&frontWinding, &backWinding );
			if( tempWinding )
			{
				FreeWinding( tempWinding );
			}
			if( backWinding )
			{
				FreeWinding( backWinding );
			}
			if( !frontWinding )
			{
				continue;
			}

#if 0
			Msg( "output winding:\n" );
			PrintWinding( frontWinding );
#endif
			
			if( frontWinding )
			{
				windings[xi + yi * xSteps] = frontWinding;
			}
		}
	}
	FreeWinding( w );
	dprimitive_t &newPrim = g_primitives[g_numprimitives];
	f->firstPrimID = g_numprimitives;
	f->numPrims = 1;
	newPrim.firstIndex = g_numprimindices;
	newPrim.firstVert = g_numprimverts;
	newPrim.indexCount = 0;
	newPrim.vertCount = 0;
#ifdef USE_TRISTRIPS
	newPrim.type = PRIM_TRISTRIP;
#else
	newPrim.type = PRIM_TRILIST;
#endif

	CUtlVector<WORD> triListIndices;
	int i;
	for( i = 0; i < xSteps * ySteps; i++ )
	{
		if( !windings[i] )
		{
			continue;
		}
		unsigned short *pIndices = 
			( unsigned short * )_alloca( windings[i]->numpoints * sizeof( unsigned short ) );
		// find indices for the verts.
		newPrim.vertCount = AddWindingToPrimverts( windings[i], pIndices, newPrim.firstVert, newPrim.vertCount );

		// Now that we have indices for the verts, fan-tesselate the polygon and spit out tris.
		for( int j = 0; j < windings[i]->numpoints - 2; j++ )
		{
			triListIndices.AddToTail( pIndices[0] );
			triListIndices.AddToTail( pIndices[j+1] );
			triListIndices.AddToTail( pIndices[j+2] );
		}
	}

	delete [] windings;
	// We've already updated the verts and have a trilist. . let's strip it!
	if( !triListIndices.Size() )
	{
		return;
	}
	
#ifdef USE_TRISTRIPS
	int numTristripIndices;
	WORD *pStripIndices = NULL;
	Stripify( triListIndices.Size() / 3, triListIndices.Base(), &numTristripIndices, 
		&pStripIndices );
	Assert( pStripIndices );

	// FIXME: Should also call ComputeVertexPermutation and reorder the verts.

	for( i = 0; i < numTristripIndices; i++ )
	{
		Assert( pStripIndices[i] >= newPrim.firstVert && 
			pStripIndices[i] < newPrim.firstVert + newPrim.vertCount );
		g_primindices[newPrim.firstIndex + newPrim.indexCount] = pStripIndices[i];
		newPrim.indexCount++;
		g_numprimindices++;
		if( g_numprimindices > MAX_MAP_PRIMINDICES )
		{
			Error( "Exceeded max water indicies.\nIncrease surface subdivision size! (%d>%d)\n", g_numprimindices, MAX_MAP_PRIMINDICES );
		}
	}
	delete [] pStripIndices;
#else
	for( i = 0; i < triListIndices.Size(); i++ )
	{
		g_primindices[newPrim.firstIndex + newPrim.indexCount] = triListIndices[i];
		newPrim.indexCount++;
		g_numprimindices++;
		if( g_numprimindices > MAX_MAP_PRIMINDICES )
		{
			Error( "Exceeded max water indicies.\nIncrease surface subdivision size! (%d>%d)\n", g_numprimindices, MAX_MAP_PRIMINDICES );
		}
	}
#endif
	g_numprimitives++; // don't increment until we get here and are sure that we have a primitive.
	if( g_numprimitives > MAX_MAP_PRIMITIVES )
	{
		Error( "Exceeded max water primitives.\nIncrease surface subdivision size! (%d>%d)\n", ( int )g_numprimitives, ( int )MAX_MAP_PRIMITIVES );
	}
}

void SubdivideFaceBySubdivSize( face_t *f )
{
	if( f->numpoints == 0 || f->split[0] || f->split[1] || f->merged || !f->w )
	{
		return;
	}
	// see if the face needs to be subdivided.
	texinfo_t *pTexInfo = &texinfo[f->texinfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	bool bFound;
	const char *pMaterialName = TexDataStringTable_GetString( pTexData->nameStringTableID );
	MaterialSystemMaterial_t matID = 
		FindOriginalMaterial( pMaterialName, &bFound, false );

	if( !bFound )
	{
		return;
	}
	const char *subdivsizeString = GetMaterialVar( matID, "$subdivsize" );	
	if( subdivsizeString )
	{
		float subdivSize = atof( subdivsizeString );
		if( subdivSize > 0.0f )
		{
			// NOTE: Subdivision is unsupported and should be phased out
			Warning("Using subdivision on %s\n", pMaterialName );
			SubdivideFaceBySubdivSize( f, subdivSize );
		}
	}
}

void SplitSubdividedFaces_Node_r( node_t *node )
{
	if (node->planenum == PLANENUM_LEAF)
	{
		return;
	}
	face_t *f;
	for( f = node->faces; f ;f = f->next )
	{
		SubdivideFaceBySubdivSize( f );
	}

	//
	// recursively output the other nodes
	//	
	SplitSubdividedFaces_Node_r( node->children[0] );
	SplitSubdividedFaces_Node_r( node->children[1] );
}

void SplitSubdividedFaces( face_t *pLeafFaceList, node_t *headnode )
{
	// deal with leaf faces.
	face_t *f = pLeafFaceList;
	while ( f )
	{
		SubdivideFaceBySubdivSize( f );
		f = f->next;
	}

	// deal with node faces.
	SplitSubdividedFaces_Node_r( headnode );
}

#pragma optimize( "", on )

/*
============
MakeFaces
============
*/
void MakeFaces (node_t *node)
{
	qprintf ("--- MakeFaces ---\n");
	c_merge = 0;
	c_subdivide = 0;
	c_nodefaces = 0;

	MakeFaces_r (node);

	qprintf ("%5i makefaces\n", c_nodefaces);
	qprintf ("%5i merged\n", c_merge);
	qprintf ("%5i subdivided\n", c_subdivide);
}