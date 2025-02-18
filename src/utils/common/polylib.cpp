//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cmdlib.h"
#include "mathlib/mathlib.h"
#include "polylib.h"
#include "worldsize.h"
#include "threads.h"
#include "tier0/dbg.h"

// doesn't seem to need to be here? -- in threads.h
//extern int numthreads;

// counters are only bumped when running single threaded,
// because they are an awefull coherence problem
int	c_active_windings;
int	c_peak_windings;
int	c_winding_allocs;
int	c_winding_points;

void pw(winding_t *w)
{
	int		i;
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.1f, %5.1f, %5.1f)\n",w->p[i][0], w->p[i][1],w->p[i][2]);
}

winding_t *winding_pool[MAX_POINTS_ON_WINDING+4];

/*
=============
AllocWinding
=============
*/
winding_t *AllocWinding (int points)
{
	winding_t	*w;

	if (numthreads == 1)
	{
		c_winding_allocs++;
		c_winding_points += points;
		c_active_windings++;
		if (c_active_windings > c_peak_windings)
			c_peak_windings = c_active_windings;
	}
	ThreadLock();
	if (winding_pool[points])
	{
		w = winding_pool[points];
		winding_pool[points] = w->next;
	}
	else
	{
		w = (winding_t *)malloc(sizeof(*w));
		w->p = (Vector *)calloc( points, sizeof(Vector) );
	}
	ThreadUnlock();
	w->numpoints = 0; // None are occupied yet even though allocated.
	w->maxpoints = points;
	w->next = NULL;
	return w;
}

void FreeWinding (winding_t *w)
{
	if (w->numpoints == 0xdeaddead)
		Error ("FreeWinding: freed a freed winding");
	
	ThreadLock();
	w->numpoints = 0xdeaddead; // flag as freed
	w->next = winding_pool[w->maxpoints];
	winding_pool[w->maxpoints] = w;
	ThreadUnlock();
}

/*
============
RemoveColinearPoints
============
*/
int	c_removed;

void RemoveColinearPoints (winding_t *w)
{
	int		i, j, k;
	Vector	v1, v2;
	int		nump;
	Vector	p[MAX_POINTS_ON_WINDING];

	nump = 0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		j = (i+1)%w->numpoints;
		k = (i+w->numpoints-1)%w->numpoints;
		VectorSubtract (w->p[j], w->p[i], v1);
		VectorSubtract (w->p[i], w->p[k], v2);
		VectorNormalize(v1);
		VectorNormalize(v2);
		if (DotProduct(v1, v2) < 0.999)
		{
			VectorCopy (w->p[i], p[nump]);
			nump++;
		}
	}

	if (nump == w->numpoints)
		return;

	if (numthreads == 1)
		c_removed += w->numpoints - nump;
	w->numpoints = nump;
	memcpy (w->p, p, nump*sizeof(p[0]));
}

/*
============
WindingPlane
============
*/
void WindingPlane (winding_t *w, Vector &normal, vec_t *dist)
{
	Vector	v1, v2;
	
	VectorSubtract (w->p[1], w->p[0], v1);
	
	// HACKHACK: Avoid potentially collinear verts
	if ( w->numpoints > 3 )
	{
		VectorSubtract (w->p[3], w->p[0], v2);
	}
	else
	{
		VectorSubtract (w->p[2], w->p[0], v2);
	}
	CrossProduct (v2, v1, normal);
	VectorNormalize (normal);
	*dist = DotProduct (w->p[0], normal);

}


/*
=============
WindingArea
=============
*/
vec_t WindingArea(winding_t *w)
{
	int		i;
	Vector	d1, d2, cross;
	vec_t	total;

	total = 0;
	for (i=2 ; i<w->numpoints ; i++)
	{
		VectorSubtract (w->p[i-1], w->p[0], d1);
		VectorSubtract (w->p[i], w->p[0], d2);
		CrossProduct (d1, d2, cross);
		total += VectorLength ( cross );
	}
	return total * 0.5;
}

void WindingBounds (winding_t *w, Vector &mins, Vector &maxs)
{
	vec_t	v;
	int		i,j;

	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;

	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = w->p[i][j];
			if (v < mins[j])
				mins[j] = v;
			if (v > maxs[j])
				maxs[j] = v;
		}
	}
}

/*
=============
WindingCenter
=============
*/
void WindingCenter (winding_t *w, Vector &center)
{
	int		i;
	float	scale;

	VectorCopy (vec3_origin, center);
	for (i=0 ; i<w->numpoints ; i++)
		VectorAdd (w->p[i], center, center);

	scale = 1.0/w->numpoints;
	VectorScale (center, scale, center);
}



/*
=============
WindingCenter
=============
*/
vec_t WindingAreaAndBalancePoint( winding_t *w, Vector &center )
{
	int		i;
	Vector	d1, d2, cross;
	vec_t	total;

	VectorCopy (vec3_origin, center);
	if ( !w )
		return 0.0f;

	total = 0;
	for (i=2 ; i<w->numpoints ; i++)
	{
		VectorSubtract (w->p[i-1], w->p[0], d1);
		VectorSubtract (w->p[i], w->p[0], d2);
		CrossProduct (d1, d2, cross);
		float area = VectorLength ( cross );
		total += area;

		// center of triangle, weighed by area
		VectorMA( center, area / 3.0, w->p[i-1], center );
		VectorMA( center, area / 3.0, w->p[i], center );
		VectorMA( center, area / 3.0, w->p[0], center );
	}
	if (total)
	{
		VectorScale( center, 1.0 / total, center );
	}
	return total * 0.5;
}

/*
=================
BaseWindingForPlane
=================
*/
winding_t *BaseWindingForPlane (const Vector &normal, vec_t dist)
{
	int		i, x;
	vec_t	max, v;
	Vector	org, vright, vup;
	winding_t	*w;
	
// find the major axis

	max = -1;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Error ("BaseWindingForPlane: no axis found");
		
	VectorCopy (vec3_origin, vup);	
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}

	v = DotProduct (vup, normal);
	VectorMA (vup, -v, normal, vup);
	VectorNormalize (vup);
		
	VectorScale (normal, dist, org);
	
	CrossProduct (vup, normal, vright);
	
	VectorScale (vup, (MAX_COORD_INTEGER*4), vup);
	VectorScale (vright, (MAX_COORD_INTEGER*4), vright);

// project a really big	axis aligned box onto the plane
	w = AllocWinding (4);
	
	VectorSubtract (org, vright, w->p[0]);
	VectorAdd (w->p[0], vup, w->p[0]);
	
	VectorAdd (org, vright, w->p[1]);
	VectorAdd (w->p[1], vup, w->p[1]);
	
	VectorAdd (org, vright, w->p[2]);
	VectorSubtract (w->p[2], vup, w->p[2]);
	
	VectorSubtract (org, vright, w->p[3]);
	VectorSubtract (w->p[3], vup, w->p[3]);
	
	w->numpoints = 4;
	
	return w;	
}

/*
==================
CopyWinding
==================
*/
winding_t *CopyWinding (winding_t *w)
{
	int			size;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	c->numpoints = w->numpoints;
	size = w->numpoints*sizeof(w->p[0]);
	memcpy (c->p, w->p, size);
	return c;
}

/*
==================
ReverseWinding
==================
*/
winding_t *ReverseWinding (winding_t *w)
{
	int			i;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorCopy (w->p[w->numpoints-1-i], c->p[i]);
	}
	c->numpoints = w->numpoints;
	return c;
}


// BUGBUG: Hunt this down - it's causing CSG errors
#pragma optimize("g", off)
/*
=============
ClipWindingEpsilon
=============
*/

void ClipWindingEpsilon (winding_t *in, const Vector &normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back)
{
	vec_t	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	Vector	mid = vec3_origin;
	winding_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	*front = *back = NULL;

	if (!counts[0])
	{
		*back = CopyWinding (in);
		return;
	}
	if (!counts[1])
	{
		*front = CopyWinding (in);
		return;
	}

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = AllocWinding (maxpts);
	*back = b = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		Vector& p1 = in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		Vector& p2 = in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, f->p[f->numpoints]);
		f->numpoints++;
		VectorCopy (mid, b->p[b->numpoints]);
		b->numpoints++;
	}
	
	if (f->numpoints > maxpts || b->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints > MAX_POINTS_ON_WINDING)
		Error ("ClipWinding: MAX_POINTS_ON_WINDING");
}
#pragma optimize("", on)


// NOTE: This is identical to ClipWindingEpsilon, but it does a pre/post translation to improve precision
void ClipWindingEpsilon_Offset( winding_t *in, const Vector &normal, vec_t dist, vec_t epsilon, winding_t **front, winding_t **back, const Vector &offset )
{
	TranslateWinding( in, offset );
	ClipWindingEpsilon( in, normal, dist+DotProduct(offset,normal), epsilon, front, back );
	TranslateWinding( in, -offset );
	if ( front && *front )
	{
		TranslateWinding( *front, -offset );
	}
	if ( back && *back )
	{
		TranslateWinding( *back, -offset );
	}
}

void ClassifyWindingEpsilon_Offset( winding_t *in, const Vector &normal, vec_t dist, vec_t epsilon, winding_t **front, winding_t **back, winding_t **on, const Vector &offset)
{
	TranslateWinding( in, offset );
	ClassifyWindingEpsilon( in, normal, dist+DotProduct(offset,normal), epsilon, front, back, on );
	TranslateWinding( in, -offset );
	if ( front && *front )
	{
		TranslateWinding( *front, -offset );
	}
	if ( back && *back )
	{
		TranslateWinding( *back, -offset );
	}
	if ( on && *on )
	{
		TranslateWinding( *on, -offset );
	}
}

/*
=============
ClassifyWindingEpsilon
=============
*/
// This version returns the winding as "on" if all verts lie in the plane
void ClassifyWindingEpsilon( winding_t *in, const Vector &normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back, winding_t **on)
{
	vec_t	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	Vector	mid = vec3_origin;
	winding_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	*front = *back = *on = NULL;

	if ( !counts[0] && !counts[1] )
	{
		*on = CopyWinding(in);
		return;
	}

	if (!counts[0])
	{
		*back = CopyWinding(in);
		return;
	}
	if (!counts[1])
	{
		*front = CopyWinding(in);
		return;
	}

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = AllocWinding (maxpts);
	*back = b = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		Vector& p1 = in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		Vector& p2 = in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, f->p[f->numpoints]);
		f->numpoints++;
		VectorCopy (mid, b->p[b->numpoints]);
		b->numpoints++;
	}

	if (f->numpoints > maxpts || b->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints > MAX_POINTS_ON_WINDING)
		Error ("ClipWinding: MAX_POINTS_ON_WINDING");
}

/*
=============
ChopWindingInPlace
=============
*/
void ChopWindingInPlace (winding_t **inout, const Vector &normal, vec_t dist, vec_t epsilon)
{
	winding_t	*in;
	vec_t	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	Vector	mid = vec3_origin;
	winding_t	*f;
	int		maxpts;

	in = *inout;
	counts[0] = counts[1] = counts[2] = 0;
// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
		{
			sides[i] = SIDE_FRONT;
		}
		else if (dot < -epsilon)
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (!counts[0])
	{
		FreeWinding (in);
		*inout = NULL;
		return;
	}
	if (!counts[1])
		return;		// inout stays the same

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	f = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		Vector& p1 = in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		Vector& p2 = in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, f->p[f->numpoints]);
		f->numpoints++;
	}
	
	if (f->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING)
		Error ("ClipWinding: MAX_POINTS_ON_WINDING");

	FreeWinding (in);
	*inout = f;
}


/*
=================
ChopWinding

Returns the fragment of in that is on the front side
of the cliping plane.  The original is freed.
=================
*/
winding_t *ChopWinding (winding_t *in, const Vector &normal, vec_t dist)
{
	winding_t	*f, *b;

	ClipWindingEpsilon (in, normal, dist, ON_EPSILON, &f, &b);
	FreeWinding (in);
	if (b)
		FreeWinding (b);
	return f;
}


/*
=================
CheckWinding

=================
*/
void CheckWinding (winding_t *w)
{
	int		i, j;
	vec_t	d, edgedist;
	Vector	dir, edgenormal, facenormal;
	vec_t	area;
	vec_t	facedist;

	if (w->numpoints < 3)
		Error ("CheckWinding: %i points",w->numpoints);
	
	area = WindingArea(w);
	if (area < 1)
		Error ("CheckWinding: %f area", area);

	WindingPlane (w, facenormal, &facedist);
	
	for (i=0 ; i<w->numpoints ; i++)
	{
		Vector& p1 = w->p[i];

		for (j=0 ; j<3 ; j++)
		{
			if (p1[j] > MAX_COORD_INTEGER || p1[j] < MIN_COORD_INTEGER)
				Error ("CheckFace: out of range: %f",p1[j]);
		}

		j = i+1 == w->numpoints ? 0 : i+1;
		
	// check the point is on the face plane
		d = DotProduct (p1, facenormal) - facedist;
		if (d < -ON_EPSILON || d > ON_EPSILON)
			Error ("CheckWinding: point off plane");
	
	// check the edge isnt degenerate
		Vector& p2 = w->p[j];
		VectorSubtract (p2, p1, dir);
		
		if (VectorLength (dir) < ON_EPSILON)
			Error ("CheckWinding: degenerate edge");
			
		CrossProduct (facenormal, dir, edgenormal);
		VectorNormalize (edgenormal);
		edgedist = DotProduct (p1, edgenormal);
		edgedist += ON_EPSILON;
		
	// all other points must be on front side
		for (j=0 ; j<w->numpoints ; j++)
		{
			if (j == i)
				continue;
			d = DotProduct (w->p[j], edgenormal);
			if (d > edgedist)
				Error ("CheckWinding: non-convex");
		}
	}
}


/*
============
WindingOnPlaneSide
============
*/
int WindingOnPlaneSide (winding_t *w, const Vector &normal, vec_t dist)
{
	qboolean	front, back;
	int			i;
	vec_t		d;

	front = false;
	back = false;
	for (i=0 ; i<w->numpoints ; i++)
	{
		d = DotProduct (w->p[i], normal) - dist;
		if (d < -ON_EPSILON)
		{
			if (front)
				return SIDE_CROSS;
			back = true;
			continue;
		}
		if (d > ON_EPSILON)
		{
			if (back)
				return SIDE_CROSS;
			front = true;
			continue;
		}
	}

	if (back)
		return SIDE_BACK;
	if (front)
		return SIDE_FRONT;
	return SIDE_ON;
}


//-----------------------------------------------------------------------------
// Purpose: 2d point inside of winding test (assumes the point resides in the
//          winding plane)
//-----------------------------------------------------------------------------
bool PointInWinding( const Vector &pt, winding_t *pWinding )
{
	if( !pWinding )
		return false;

#if 0
	//
	// NOTE: this will be a quicker way to calculate this, however I don't
	//       know the trick off hand (post dot product tests??)  
	// TODO: look in graphics gems!!!! (cab)
	//

	Vector edge1, edge2;
	for( int ndxPt = 0; ndxPt < pWinding->numpoints; ndxPt++ )
	{
		edge1 = pWinding->p[ndxPt] - pt;
		edge2 = pWinding->p[(ndxPt+1)%pWinding->numpoints] - pt;
		
		VectorNormalize( edge1 );
		VectorNormalize( edge2 );

		if( edge2.Dot( edge1 ) < 0.0f )
			return false;
	}

	return true;

#else
	Vector edge, toPt, cross, testCross;

	//
	// get the first normal to test
	//
	toPt = pt - pWinding->p[0];
	edge = pWinding->p[1] - pWinding->p[0];
	testCross = edge.Cross( toPt );
	VectorNormalize( testCross );

	for( int ndxPt = 1; ndxPt < pWinding->numpoints; ndxPt++ )
	{
		toPt = pt - pWinding->p[ndxPt];
		edge = pWinding->p[(ndxPt+1)%pWinding->numpoints] - pWinding->p[ndxPt];
		cross = edge.Cross( toPt );
		VectorNormalize( cross );

		if( cross.Dot( testCross ) < 0.0f )
			return false;
	}

	return true;
#endif
}

void TranslateWinding( winding_t *pWinding, const Vector &offset )
{
	for ( int i = 0; i < pWinding->numpoints; i++ )
	{
		pWinding->p[i] += offset;
	}
}
