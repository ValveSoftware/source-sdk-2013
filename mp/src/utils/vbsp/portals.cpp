//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"
#include "utlvector.h"
#include "mathlib/vmatrix.h"
#include "iscratchpad3d.h"
#include "csg.h"
#include "fmtstr.h"

int		c_active_portals;
int		c_peak_portals;
int		c_boundary;
int		c_boundary_sides;

/*
===========
AllocPortal
===========
*/
portal_t *AllocPortal (void)
{
	static int s_PortalCount = 0;

	portal_t	*p;
	
	if (numthreads == 1)
		c_active_portals++;
	if (c_active_portals > c_peak_portals)
		c_peak_portals = c_active_portals;
	
	p = (portal_t*)malloc (sizeof(portal_t));
	memset (p, 0, sizeof(portal_t));
	p->id = s_PortalCount;
	++s_PortalCount;

	return p;
}

void FreePortal (portal_t *p)
{
	if (p->winding)
		FreeWinding (p->winding);
	if (numthreads == 1)
		c_active_portals--;
	free (p);
}

//==============================================================

/*
==============
VisibleContents

Returns the single content bit of the
strongest visible content present
==============
*/
int VisibleContents (int contents)
{
	int		i;

	for (i=1 ; i<=LAST_VISIBLE_CONTENTS ; i<<=1)
	{
		if (contents & i )
		{
			return i;
		}
	}

	return 0;
}


/*
===============
ClusterContents
===============
*/
int ClusterContents (node_t *node)
{
	int		c1, c2, c;

	if (node->planenum == PLANENUM_LEAF)
		return node->contents;

	c1 = ClusterContents(node->children[0]);
	c2 = ClusterContents(node->children[1]);
	c = c1|c2;

	// a cluster may include some solid detail areas, but
	// still be seen into
	if ( ! (c1&CONTENTS_SOLID) || ! (c2&CONTENTS_SOLID) )
		c &= ~CONTENTS_SOLID;
	return c;
}

/*
=============
Portal_VisFlood

Returns true if the portal is empty or translucent, allowing
the PVS calculation to see through it.
The nodes on either side of the portal may actually be clusters,
not leafs, so all contents should be ored together
=============
*/
qboolean Portal_VisFlood (portal_t *p)
{
	int		c1, c2;

	if (!p->onnode)
		return false;	// to global outsideleaf

	c1 = ClusterContents(p->nodes[0]);
	c2 = ClusterContents(p->nodes[1]);

	if (!VisibleContents (c1^c2))
		return true;

	if (c1 & (CONTENTS_TRANSLUCENT|CONTENTS_DETAIL))
		c1 = 0;
	if (c2 & (CONTENTS_TRANSLUCENT|CONTENTS_DETAIL))
		c2 = 0;

	if ( (c1|c2) & CONTENTS_SOLID )
		return false;		// can't see through solid

	if (! (c1 ^ c2))
		return true;		// identical on both sides

	if (!VisibleContents (c1^c2))
		return true;
	return false;
}


/*
===============
Portal_EntityFlood

The entity flood determines which areas are
"outside" on the map, which are then filled in.
Flowing from side s to side !s
===============
*/
qboolean Portal_EntityFlood (portal_t *p, int s)
{
	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF)
		Error ("Portal_EntityFlood: not a leaf");

	// can never cross to a solid 
	if ( (p->nodes[0]->contents & CONTENTS_SOLID)
	|| (p->nodes[1]->contents & CONTENTS_SOLID) )
		return false;

	// can flood through everything else
	return true;
}

qboolean Portal_AreaLeakFlood (portal_t *p, int s)
{
	if ( !Portal_EntityFlood( p, s ) )
		return false;

	// can never cross through areaportal
	if ( (p->nodes[0]->contents & CONTENTS_AREAPORTAL)
	|| (p->nodes[1]->contents & CONTENTS_AREAPORTAL) )
		return false;

	// can flood through everything else
	return true;
}


//=============================================================================

int		c_tinyportals;

/*
=============
AddPortalToNodes
=============
*/
void AddPortalToNodes (portal_t *p, node_t *front, node_t *back)
{
	if (p->nodes[0] || p->nodes[1])
		Error ("AddPortalToNode: allready included");

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;
	
	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
=============
RemovePortalFromNode
=============
*/
void RemovePortalFromNode (portal_t *portal, node_t *l)
{
	portal_t	**pp, *t;
	
// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			Error ("RemovePortalFromNode: portal not in leaf");	

		if ( t == portal )
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			Error ("RemovePortalFromNode: portal not bounding leaf");
	}
	
	if (portal->nodes[0] == l)
	{
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	}
	else if (portal->nodes[1] == l)
	{
		*pp = portal->next[1];	
		portal->nodes[1] = NULL;
	}
}

//============================================================================

void PrintPortal (portal_t *p)
{
	int			i;
	winding_t	*w;
	
	w = p->winding;
	for (i=0 ; i<w->numpoints ; i++)
		Msg ("(%5.0f,%5.0f,%5.0f)\n",w->p[i][0]
		, w->p[i][1], w->p[i][2]);
}

// because of water areaportals support, the areaportal may not be the only brush on this node
bspbrush_t *AreaportalBrushForNode( node_t *node )
{
	bspbrush_t *b = node->brushlist;
	while ( b && !(b->original->contents & CONTENTS_AREAPORTAL) )
	{
		b = b->next;
	}
	Assert( b->original->entitynum != 0 );
	return b;
}

/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
// buffer space around sides of nodes
#define	SIDESPACE	8
void MakeHeadnodePortals (tree_t *tree)
{
	Vector		bounds[2];
	int			i, j, n;
	portal_t	*p, *portals[6];
	plane_t		bplanes[6], *pl;
	node_t *node;

	node = tree->headnode;

// pad with some space so there will never be null volume leafs
	for (i=0 ; i<3 ; i++)
	{
		bounds[0][i] = tree->mins[i] - SIDESPACE;
		bounds[1][i] = tree->maxs[i] + SIDESPACE;
	}
	
	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.contents = 0;

	for (i=0 ; i<3 ; i++)
		for (j=0 ; j<2 ; j++)
		{
			n = j*3 + i;

			p = AllocPortal ();
			portals[n] = p;
			
			pl = &bplanes[n];
			memset (pl, 0, sizeof(*pl));
			if (j)
			{
				pl->normal[i] = -1;
				pl->dist = -bounds[j][i];
			}
			else
			{
				pl->normal[i] = 1;
				pl->dist = bounds[j][i];
			}
			p->plane = *pl;
			p->winding = BaseWindingForPlane (pl->normal, pl->dist);
			AddPortalToNodes (p, node, &tree->outside_node);
		}
		
// clip the basewindings by all the other planes
	for (i=0 ; i<6 ; i++)
	{
		for (j=0 ; j<6 ; j++)
		{
			if (j == i)
				continue;
			ChopWindingInPlace (&portals[i]->winding, bplanes[j].normal, bplanes[j].dist, ON_EPSILON);
		}
	}
}

//===================================================


/*
================
BaseWindingForNode
================
*/
#define	BASE_WINDING_EPSILON	0.001
#define	SPLIT_WINDING_EPSILON	0.001

winding_t	*BaseWindingForNode (node_t *node)
{
	winding_t	*w;
	node_t		*n;
	plane_t		*plane;
	Vector		normal;
	vec_t		dist;

	w = BaseWindingForPlane (g_MainMap->mapplanes[node->planenum].normal, g_MainMap->mapplanes[node->planenum].dist);

	// clip by all the parents
	for (n=node->parent ; n && w ; )
	{
		plane = &g_MainMap->mapplanes[n->planenum];

		if (n->children[0] == node)
		{	// take front
			ChopWindingInPlace (&w, plane->normal, plane->dist, BASE_WINDING_EPSILON);
		}
		else
		{	// take back
			VectorSubtract (vec3_origin, plane->normal, normal);
			dist = -plane->dist;
			ChopWindingInPlace (&w, normal, dist, BASE_WINDING_EPSILON);
		}
		node = n;
		n = n->parent;
	}

	return w;
}

//============================================================

/*
==================
MakeNodePortal

create the new portal by taking the full plane winding for the cutting plane
and clipping it by all of parents of this node
==================
*/
void MakeNodePortal (node_t *node)
{
	portal_t	*new_portal, *p;
	winding_t	*w;
	Vector		normal;
	float		dist = 0.0f;
	int			side = 0;

	w = BaseWindingForNode (node);

	// clip the portal by all the other portals in the node
	for (p = node->portals ; p && w; p = p->next[side])	
	{
		if (p->nodes[0] == node)
		{
			side = 0;
			VectorCopy (p->plane.normal, normal);
			dist = p->plane.dist;
		}
		else if (p->nodes[1] == node)
		{
			side = 1;
			VectorSubtract (vec3_origin, p->plane.normal, normal);
			dist = -p->plane.dist;
		}
		else
		{
			Error ("CutNodePortals_r: mislinked portal");
		}

		ChopWindingInPlace (&w, normal, dist, 0.1);
	}

	if (!w)
	{
		return;
	}

	if (WindingIsTiny (w))
	{
		c_tinyportals++;
		FreeWinding (w);
		return;
	}


	new_portal = AllocPortal ();
	new_portal->plane = g_MainMap->mapplanes[node->planenum];
	new_portal->onnode = node;
	new_portal->winding = w;	

	AddPortalToNodes (new_portal, node->children[0], node->children[1]);
}


/*
==============
SplitNodePortals

Move or split the portals that bound node so that the node's
children have portals instead of node.
==============
*/
void SplitNodePortals (node_t *node)
{
	portal_t	*p, *next_portal, *new_portal;
	node_t		*f, *b, *other_node;
	int			side = 0;
	plane_t		*plane;
	winding_t	*frontwinding, *backwinding;

	plane = &g_MainMap->mapplanes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for (p = node->portals ; p ; p = next_portal)	
	{
		if (p->nodes[0] == node)
			side = 0;
		else if (p->nodes[1] == node)
			side = 1;
		else
			Error ("CutNodePortals_r: mislinked portal");
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);

//
// cut the portal into two portals, one on each side of the cut plane
//
		ClipWindingEpsilon (p->winding, plane->normal, plane->dist,
			SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if (frontwinding && WindingIsTiny(frontwinding))
		{
			FreeWinding (frontwinding);
			frontwinding = NULL;
			c_tinyportals++;
		}

		if (backwinding && WindingIsTiny(backwinding))
		{
			FreeWinding (backwinding);
			backwinding = NULL;
			c_tinyportals++;
		}

		if (!frontwinding && !backwinding)
		{	// tiny windings on both sides
			continue;
		}

		if (!frontwinding)
		{
			FreeWinding (backwinding);
			if (side == 0)
				AddPortalToNodes (p, b, other_node);
			else
				AddPortalToNodes (p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			FreeWinding (frontwinding);
			if (side == 0)
				AddPortalToNodes (p, f, other_node);
			else
				AddPortalToNodes (p, other_node, f);
			continue;
		}
		
	// the winding is split
		new_portal = AllocPortal ();
		*new_portal = *p;
		new_portal->winding = backwinding;
		FreeWinding (p->winding);
		p->winding = frontwinding;

		if (side == 0)
		{
			AddPortalToNodes (p, f, other_node);
			AddPortalToNodes (new_portal, b, other_node);
		}
		else
		{
			AddPortalToNodes (p, other_node, f);
			AddPortalToNodes (new_portal, other_node, b);
		}
	}

	node->portals = NULL;
}


/*
================
CalcNodeBounds
================
*/
void CalcNodeBounds (node_t *node)
{
	portal_t	*p;
	int			s;
	int			i;

	// calc mins/maxs for both leafs and nodes
	ClearBounds (node->mins, node->maxs);
	for (p = node->portals ; p ; p = p->next[s])	
	{
		s = (p->nodes[1] == node);
		for (i=0 ; i<p->winding->numpoints ; i++)
			AddPointToBounds (p->winding->p[i], node->mins, node->maxs);
	}
}


/*
==================
MakeTreePortals_r
==================
*/
void MakeTreePortals_r (node_t *node)
{
	int		i;

	CalcNodeBounds (node);
	if (node->mins[0] >= node->maxs[0])
	{
		Warning("WARNING: node without a volume\n");
	}

	for (i=0 ; i<3 ; i++)
	{
		if (node->mins[i] < (MIN_COORD_INTEGER-SIDESPACE) || node->maxs[i] > (MAX_COORD_INTEGER+SIDESPACE))
		{
			const char *pMatName = "<NO BRUSH>";
			// split by brush side
			if ( node->side )
			{
				texinfo_t *pTexInfo = &texinfo[node->side->texinfo];
				dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
				pMatName = TexDataStringTable_GetString( pTexData->nameStringTableID );
			}
			Vector point = node->portals->winding->p[0];
			Warning("WARNING: BSP node with unbounded volume (material: %s, near %s)\n", pMatName, VecToString(point) );
			break;
		}
	}
	if (node->planenum == PLANENUM_LEAF)
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	MakeTreePortals_r (node->children[0]);
	MakeTreePortals_r (node->children[1]);
}

/*
==================
MakeTreePortals
==================
*/
void MakeTreePortals (tree_t *tree)
{
	MakeHeadnodePortals (tree);
	MakeTreePortals_r (tree->headnode);
}

/*
=========================================================

FLOOD ENTITIES

=========================================================
*/

//-----------------------------------------------------------------------------
// Purpose: Floods outward from the given node, marking visited nodes with
//			the number of hops from a node with an entity. If we ever mark
//			the outside_node for this tree, we've leaked.
// Input  : node - 
//			dist - 
//-----------------------------------------------------------------------------
void FloodPortals_r (node_t *node, int dist)
{
	portal_t	*p;
	int			s;

	node->occupied = dist;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);

		// Skip nodes that have already been marked.
		if (p->nodes[!s]->occupied)
			continue;

		// Skip portals that lead to or from nodes with solid contents.
		if (!Portal_EntityFlood (p, s))
			continue;

		FloodPortals_r (p->nodes[!s], dist+1);
	}
}

void FloodAreaLeak_r( node_t *node, int dist )
{
	portal_t	*p;
	int			s;

	node->occupied = dist;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);

		if (p->nodes[!s]->occupied)
			continue;

		if (!Portal_AreaLeakFlood (p, s))
			continue;

		FloodAreaLeak_r( p->nodes[!s], dist+1 );
	}
}

void ClearOccupied_r( node_t *headnode )
{
	if ( !headnode )
		return;

	headnode->occupied = 0;
	ClearOccupied_r( headnode->children[0] );
	ClearOccupied_r( headnode->children[1] );
}

void FloodAreaLeak( node_t *headnode, node_t *pFirstSide )
{
	ClearOccupied_r( headnode );
	FloodAreaLeak_r( pFirstSide, 2 );
}

//-----------------------------------------------------------------------------
// Purpose: For the given entity at the given origin, finds the leaf node in the
//			BSP tree that the entity occupies.
//
//			We then flood outward from that leaf to see if the entity leaks.
// Input  : headnode - 
//			origin - 
//			occupant - 
// Output : Returns false if the entity is in solid, true if it is not.
//-----------------------------------------------------------------------------
qboolean PlaceOccupant (node_t *headnode, Vector& origin, entity_t *occupant)
{
	node_t	*node;
	vec_t	d;
	plane_t	*plane;

	// find the leaf to start in
	node = headnode;
	while (node->planenum != PLANENUM_LEAF)
	{
		plane = &g_MainMap->mapplanes[node->planenum];
		d = DotProduct (origin, plane->normal) - plane->dist;
		if (d >= 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	if (node->contents == CONTENTS_SOLID)
		return false;

	node->occupant = occupant;

	// Flood outward from here to see if this entity leaks.
	FloodPortals_r (node, 1);

	return true;
}

/*
=============
FloodEntities

Marks all nodes that can be reached by entites
=============
*/
qboolean FloodEntities (tree_t *tree)
{
	int		i;
	Vector	origin;
	char	*cl;
	qboolean	inside;
	node_t *headnode;

	headnode = tree->headnode;
	qprintf ("--- FloodEntities ---\n");
	inside = false;
	tree->outside_node.occupied = 0;

	for (i=1 ; i<num_entities ; i++)
	{
		GetVectorForKey (&entities[i], "origin", origin);
		if (VectorCompare(origin, vec3_origin))
			continue;

		cl = ValueForKey (&entities[i], "classname");

		origin[2] += 1;	// so objects on floor are ok

		// nudge playerstart around if needed so clipping hulls allways
		// have a valid point
		if (!strcmp (cl, "info_player_start"))
		{
			int	x, y;

			for (x=-16 ; x<=16 ; x += 16)
			{
				for (y=-16 ; y<=16 ; y += 16)
				{
					origin[0] += x;
					origin[1] += y;
					if (PlaceOccupant (headnode, origin, &entities[i]))
					{
						inside = true;
						goto gotit;
					}
					origin[0] -= x;
					origin[1] -= y;
				}
			}
gotit: ;
		}
		else
		{
			if (PlaceOccupant (headnode, origin, &entities[i]))
				inside = true;
		}
	}

	if (!inside)
	{
		qprintf ("no entities in open -- no filling\n");
	}

	if (tree->outside_node.occupied)
	{
		qprintf ("entity reached from outside -- no filling\n" );
	}

	return (qboolean)(inside && !tree->outside_node.occupied);
}


/*
=========================================================

FLOOD AREAS

=========================================================
*/

int		c_areas;

bool IsAreaportalNode( node_t *node )
{
	return ( node->contents & CONTENTS_AREAPORTAL ) ? true : false;
}
/*
=============
FloodAreas_r
=============
*/

void FloodAreas_r (node_t *node, portal_t *pSeeThrough)
{
	portal_t	*p;
	int			s;
	bspbrush_t	*b;
	entity_t	*e;

	if ( IsAreaportalNode(node) )
	{
		// this node is part of an area portal
		b = AreaportalBrushForNode( node );
		e = &entities[b->original->entitynum];

		// if the current area has allready touched this
		// portal, we are done
		if (e->portalareas[0] == c_areas || e->portalareas[1] == c_areas)
			return;

		// note the current area as bounding the portal
		if (e->portalareas[1])
		{
			Warning("WARNING: areaportal entity %i (brush %i) touches > 2 areas\n", b->original->entitynum, b->original->id );
			return;
		}
		
		if (e->portalareas[0])
		{
			e->portalareas[1] = c_areas;
			e->m_pPortalsLeadingIntoAreas[1] = pSeeThrough;
		}
		else
		{
			e->portalareas[0] = c_areas;
			e->m_pPortalsLeadingIntoAreas[0] = pSeeThrough;
		}

		return;
	}

	if (node->area)
		return;		// allready got it
	node->area = c_areas;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);
#if 0
		if (p->nodes[!s]->occupied)
			continue;
#endif
		if (!Portal_EntityFlood (p, s))
			continue;

		FloodAreas_r (p->nodes[!s], p);
	}
}

/*
=============
FindAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void FindAreas_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FindAreas_r (node->children[0]);
		FindAreas_r (node->children[1]);
		return;
	}

	if (node->area)
		return;		// allready got it

	if (node->contents & CONTENTS_SOLID)
		return;

	if (!node->occupied)
		return;			// not reachable by entities

	// area portals are allways only flooded into, never
	// out of
	if (IsAreaportalNode(node))
		return;

	c_areas++;
	FloodAreas_r (node, NULL);
}


void ReportAreaportalLeak( tree_t *tree, node_t *node )
{
	portal_t *p, *pStart = NULL;
	int s;

	// Find a portal out of this areaportal into empty space
	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);
		if ( !Portal_EntityFlood( p, !s ) )
			continue;
		if ( p->nodes[!s]->contents & CONTENTS_AREAPORTAL )
			continue;

		pStart = p;
		break;
	}

	if ( pStart )
	{
		s = pStart->nodes[0] == node;
		Assert(!(pStart->nodes[s]->contents & CONTENTS_AREAPORTAL) );
		// flood fill the area outside this areaportal brush
		FloodAreaLeak( tree->headnode, pStart->nodes[s] );

		// find the portal into the longest path around the portal
		portal_t *pBest = NULL;
		int bestDist = 0;
		for (p=node->portals ; p ; p = p->next[s])
		{
			if ( p == pStart )
				continue;

			s = (p->nodes[1] == node);
			if ( p->nodes[!s]->occupied > bestDist )
			{
				pBest = p;
				bestDist = p->nodes[!s]->occupied;
			}
		}
		if ( pBest )
		{
			s = (pBest->nodes[0] == node);
			// write the linefile that goes from pBest to pStart
			AreaportalLeakFile( tree, pStart, pBest, pBest->nodes[s] );
		}
	}
}


/*
=============
SetAreaPortalAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void SetAreaPortalAreas_r (tree_t *tree, node_t *node)
{
	bspbrush_t	*b;
	entity_t	*e;

	if (node->planenum != PLANENUM_LEAF)
	{
		SetAreaPortalAreas_r (tree, node->children[0]);
		SetAreaPortalAreas_r (tree, node->children[1]);
		return;
	}

	if (IsAreaportalNode(node))
	{
		if (node->area)
			return;		// already set

		b = AreaportalBrushForNode( node );
		e = &entities[b->original->entitynum];
		node->area = e->portalareas[0];
		if (!e->portalareas[1])
		{
			ReportAreaportalLeak( tree, node );
			Warning("\nBrush %i: areaportal brush doesn't touch two areas\n", b->original->id);
			return;
		}
	}
}


// Return a positive value between 0 and 2*PI telling the angle distance
// from flBaseAngle to flTestAngle.
float AngleOffset( float flBaseAngle, float flTestAngle )
{
	while( flTestAngle > flBaseAngle )
		flTestAngle -= 2 * M_PI;

	return fmod( flBaseAngle - flTestAngle, (float) (2 * M_PI) );
}


int FindUniquePoints( const Vector2D *pPoints, int nPoints, int *indexMap, int nMaxIndexMapPoints, float flTolerance )
{
	float flToleranceSqr = flTolerance * flTolerance;

	// This could be slightly more efficient.
	int nUniquePoints = 0;
	for ( int i=0; i < nPoints; i++ )
	{
		int j;
		for ( j=0; j < nUniquePoints; j++ )
		{
			if ( pPoints[i].DistToSqr( pPoints[indexMap[j]] ) < flToleranceSqr )
				break;
		}
		if ( j == nUniquePoints )
		{
			if ( nUniquePoints >= nMaxIndexMapPoints )
				Error( "FindUniquePoints: overflowed unique point list (size %d).", nMaxIndexMapPoints );

			indexMap[nUniquePoints++] = i;
		}
	}
	
	return nUniquePoints;
}

// Build a 2D convex hull of the set of points.
// This essentially giftwraps the points as it walks around the perimeter.
int Convex2D( Vector2D const *pPoints, int nPoints, int *indices, int nMaxIndices )
{
	int nIndices = 0;
	bool touched[512];
	int indexMap[512];

	if( nPoints == 0 )
		return 0;


	// If we don't collapse the points into a unique set, we can loop around forever
	// and max out nMaxIndices.
	nPoints = FindUniquePoints( pPoints, nPoints, indexMap, ARRAYSIZE( indexMap ), 0.1f );
	memset( touched, 0, nPoints*sizeof(touched[0]) );

	// Find the (lower) left side.
	int i;
	int iBest = 0;
	for( i=1; i < nPoints; i++ )
	{
		if( pPoints[indexMap[i]].x < pPoints[indexMap[iBest]].x ||
			(pPoints[indexMap[i]].x == pPoints[indexMap[iBest]].x && pPoints[indexMap[i]].y < pPoints[indexMap[iBest]].y) )
		{
			iBest = i;
		}
	}

	touched[iBest] = true;
	indices[0] = indexMap[iBest];
	nIndices = 1;

	Vector2D curEdge( 0, 1 );

	// Wind around clockwise.
	while( 1 )
	{
		Vector2D const *pStartPoint = &pPoints[ indices[nIndices-1] ];

		float flEdgeAngle = atan2( curEdge.y, curEdge.x );
		
		int iMinAngle = -1;
		float flMinAngle = 5000;

		for( i=0; i < nPoints; i++ )
		{
			Vector2D vTo = pPoints[indexMap[i]] - *pStartPoint;
			float flDistToSqr = vTo.LengthSqr();
			if ( flDistToSqr <= 0.1f )
				continue;

			// Get the angle from the edge to this point.
			float flAngle = atan2( vTo.y, vTo.x );
			flAngle = AngleOffset( flEdgeAngle, flAngle );

			if( fabs( flAngle - flMinAngle ) < 0.00001f )
			{
				float flDistToTestSqr = pStartPoint->DistToSqr( pPoints[iMinAngle] );

				// If the angle is the same, pick the point farthest away.
				// unless the current one is closing the face loop
				if ( iMinAngle != indices[0] && flDistToSqr > flDistToTestSqr )
				{
					flMinAngle = flAngle;
					iMinAngle = indexMap[i];
				}
			}
			else if( flAngle < flMinAngle )
			{
				flMinAngle = flAngle;
				iMinAngle = indexMap[i];
			}
		}

		if( iMinAngle == -1 )
		{
			// Couldn't find a point?
			Assert( false );
			break;
		}
		else if( iMinAngle == indices[0] )
		{
			// Finished.
			break;
		}
		else
		{
			// Add this point.
			if( nIndices >= nMaxIndices )
				break;

			for ( int jj = 0; jj < nIndices; jj++ )
			{
				// if this assert hits, this routine is broken and is generating a spiral
				// rather than a closed polygon - basically an edge overlap of some kind
				Assert(indices[jj] != iMinAngle );
			}

			indices[nIndices] = iMinAngle;
			++nIndices;
		}

		curEdge = pPoints[indices[nIndices-1]] - pPoints[indices[nIndices-2]];
	}
	
	return nIndices;
}

void FindPortalsLeadingToArea_R( 
	node_t *pHeadNode, 
	int iSrcArea, 
	int iDestArea, 
	plane_t *pPlane, 
	CUtlVector<portal_t*> &portals )
{
	if (pHeadNode->planenum != PLANENUM_LEAF)
	{
		FindPortalsLeadingToArea_R( pHeadNode->children[0], iSrcArea, iDestArea, pPlane, portals );
		FindPortalsLeadingToArea_R( pHeadNode->children[1], iSrcArea, iDestArea, pPlane, portals );
		return;
	}

	// Ok.. this is a leaf, check its portals.
	int s;
	for( portal_t *p = pHeadNode->portals; p ;p = p->next[!s] )
	{
		s = (p->nodes[0] == pHeadNode);

		if( !p->nodes[0]->occupied || !p->nodes[1]->occupied )
			continue;
	
		if( p->nodes[1]->area == iDestArea && p->nodes[0]->area == iSrcArea ||
			p->nodes[0]->area == iDestArea && p->nodes[1]->area == iSrcArea )
		{
			// Make sure the plane normals point the same way.
			plane_t *pMapPlane = &g_MainMap->mapplanes[p->onnode->planenum];
			float flDot = fabs( pMapPlane->normal.Dot( pPlane->normal ) );
			if( fabs( 1 - flDot ) < 0.01f )
			{
				Vector vPlanePt1 = pPlane->normal * pPlane->dist;
				Vector vPlanePt2 = pMapPlane->normal * pMapPlane->dist;

				if( vPlanePt1.DistToSqr( vPlanePt2 ) < 0.01f )
				{
					portals.AddToTail( p );
				}
			}
		}
	}
}


void EmitClipPortalGeometry( node_t *pHeadNode, portal_t *pPortal, int iSrcArea, dareaportal_t *dp )
{
	// Build a list of all the points in portals from the same original face.
	CUtlVector<portal_t*> portals;
	FindPortalsLeadingToArea_R( 
		pHeadNode, 
		iSrcArea, 
		dp->otherarea, 
		&pPortal->plane,
		portals );

	CUtlVector<Vector> points;
	for( int iPortal=0; iPortal < portals.Size(); iPortal++ )
	{
		portal_t *pPointPortal = portals[iPortal];
		winding_t *pWinding = pPointPortal->winding;
		for( int i=0; i < pWinding->numpoints; i++ )
		{
			points.AddToTail( pWinding->p[i] );
		}
	}

	// Get the 2D convex hull.

	//// First transform them into a plane.
	QAngle vAngles;
	Vector vecs[3];

	VectorAngles( pPortal->plane.normal, vAngles );
	AngleVectors( vAngles, &vecs[0], &vecs[1], &vecs[2] );
	VMatrix mTransform;
	mTransform.Identity();
	mTransform.SetBasisVectors( vecs[0], vecs[1], vecs[2] );
	VMatrix mInvTransform = mTransform.Transpose();

	int i;
	CUtlVector<Vector2D> points2D;
	for( i=0; i < points.Size(); i++ )
	{
		Vector vTest = mTransform * points[i];
		points2D.AddToTail( Vector2D( vTest.y, vTest.z ) );
	}

	// Build the hull.
	int indices[512];
	int nIndices = Convex2D( points2D.Base(), points2D.Size(), indices, 512 );

	// Output the hull.
	dp->m_FirstClipPortalVert = g_nClipPortalVerts;
	dp->m_nClipPortalVerts = nIndices;

	if ( nIndices >= 32 )
	{
		Warning( "Warning: area portal has %d verts. Could be a vbsp bug.\n", nIndices );
	}

	if( dp->m_FirstClipPortalVert + dp->m_nClipPortalVerts >= MAX_MAP_PORTALVERTS )
	{
		Vector *p = pPortal->winding->p;
		Error( "MAX_MAP_PORTALVERTS (probably a broken areaportal near %.1f %.1f %.1f ", p->x, p->y, p->z );
	}
	
	for( i=0; i < nIndices; i++ )
	{
		g_ClipPortalVerts[g_nClipPortalVerts] = points[ indices[i] ];
		++g_nClipPortalVerts;
	}
}


// Sets node_t::area for non-leaf nodes (this allows an optimization in the renderer).
void SetNodeAreaIndices_R( node_t *node )
{
	// All leaf area indices should already be set.
	if( node->planenum == PLANENUM_LEAF )
		return;

	// Have the children set their area indices.
	SetNodeAreaIndices_R( node->children[0] );
	SetNodeAreaIndices_R( node->children[1] );

	// If all children (leaves or nodes) are in the same area, then set our area
	// to this area as well. Otherwise, set it to -1.
	if( node->children[0]->area == node->children[1]->area )
		node->area = node->children[0]->area;
	else
		node->area = -1;	
}


/*
=============
EmitAreaPortals

=============
*/
void EmitAreaPortals (node_t *headnode)
{
	entity_t		*e;
	dareaportal_t	*dp;

	if (c_areas > MAX_MAP_AREAS)
		Error ("Map is split into too many unique areas (max = %d)\nProbably too many areaportals", MAX_MAP_AREAS);
	numareas = c_areas+1;
	numareaportals = 1;		// leave 0 as an error

	// Reset the clip portal vert info.
	g_nClipPortalVerts = 0;

	for (int iSrcArea=1 ; iSrcArea<=c_areas ; iSrcArea++)
	{
		dareas[iSrcArea].firstareaportal = numareaportals;
		for (int j=0 ; j<num_entities ; j++)
		{
			e = &entities[j];
			if (!e->areaportalnum)
				continue;
			
			if (e->portalareas[0] == iSrcArea || e->portalareas[1] == iSrcArea)
			{
				int iSide = (e->portalareas[0] == iSrcArea);

				// We're only interested in the portal that divides the two areas.
				// One of the portals that leads into the CONTENTS_AREAPORTAL just bounds
				// the same two areas but the other bounds two different ones.
				portal_t *pLeadingPortal = e->m_pPortalsLeadingIntoAreas[0];
				if( pLeadingPortal->nodes[0]->area == pLeadingPortal->nodes[1]->area )
					pLeadingPortal = e->m_pPortalsLeadingIntoAreas[1];

				if( pLeadingPortal )
				{
					Assert( pLeadingPortal->nodes[0]->area != pLeadingPortal->nodes[1]->area );

					dp = &dareaportals[numareaportals];
					numareaportals++;

					dp->m_PortalKey = e->areaportalnum;
					dp->otherarea = e->portalareas[iSide];
					dp->planenum = pLeadingPortal->onnode->planenum;

					Assert( pLeadingPortal->nodes[0]->planenum == PLANENUM_LEAF );
					Assert( pLeadingPortal->nodes[1]->planenum == PLANENUM_LEAF );

					if( pLeadingPortal->nodes[0]->area == dp->otherarea )
					{
						// Use the flipped version of the plane.
						dp->planenum = (dp->planenum & ~1) | (~dp->planenum & 1);
					}

					EmitClipPortalGeometry( headnode, pLeadingPortal, iSrcArea, dp );
				}
			}
		}

		dareas[iSrcArea].numareaportals = numareaportals - dareas[iSrcArea].firstareaportal;
	}

	SetNodeAreaIndices_R( headnode );

	qprintf ("%5i numareas\n", numareas);
	qprintf ("%5i numareaportals\n", numareaportals);
}

/*
=============
FloodAreas

Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
=============
*/
void FloodAreas (tree_t *tree)
{
	int start = Plat_FloatTime();
	qprintf ("--- FloodAreas ---\n");
	Msg("Processing areas...");
	FindAreas_r (tree->headnode);
	SetAreaPortalAreas_r (tree, tree->headnode);
	qprintf ("%5i areas\n", c_areas);
	Msg("done (%d)\n", (int)(Plat_FloatTime() - start) );
}

//======================================================

int		c_outside;
int		c_inside;
int		c_solid;

void FillOutside_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r (node->children[0]);
		FillOutside_r (node->children[1]);
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if (!node->occupied)
	{
		if (node->contents != CONTENTS_SOLID)
		{
			c_outside++;
			node->contents = CONTENTS_SOLID;
		}
		else
			c_solid++;
	}
	else
		c_inside++;

}

/*
=============
FillOutside

Fill all nodes that can't be reached by entities
=============
*/
void FillOutside (node_t *headnode)
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	qprintf ("--- FillOutside ---\n");
	FillOutside_r (headnode);
	qprintf ("%5i solid leafs\n", c_solid);
	qprintf ("%5i leafs filled\n", c_outside);
	qprintf ("%5i inside leafs\n", c_inside);
}


static float ComputeDistFromPlane( winding_t *pWinding, plane_t *pPlane, float maxdist )
{
	float totaldist = 0.0f;
	for (int i = 0; i < pWinding->numpoints; ++i)
	{
		totaldist += fabs(DotProduct( pPlane->normal, pWinding->p[i] ) - pPlane->dist);
		if (totaldist > maxdist)
			return totaldist;
	}
	return totaldist;
}


//-----------------------------------------------------------------------------
// Display portal error
//-----------------------------------------------------------------------------
static void DisplayPortalError( portal_t *p, int viscontents )
{
	char contents[3][1024];
	PrintBrushContentsToString( p->nodes[0]->contents, contents[0], sizeof( contents[0] ) );
	PrintBrushContentsToString( p->nodes[1]->contents, contents[1], sizeof( contents[1] ) );
	PrintBrushContentsToString( viscontents, contents[2], sizeof( contents[2] ) );

	Vector center;
	WindingCenter( p->winding, center );
	Warning( "\nFindPortalSide: Couldn't find a good match for which brush to assign to a portal near (%.1f %.1f %.1f)\n", center.x, center.y, center.z);
	Warning( "Leaf 0 contents: %s\n", contents[0] );
	Warning( "Leaf 1 contents: %s\n", contents[1] );
	Warning( "viscontents (node 0 contents ^ node 1 contents): %s\n", contents[2] );
	Warning( "This means that none of the brushes in leaf 0 or 1 that touches the portal has %s\n", contents[2] );
	Warning( "Check for a huge brush enclosing the coordinates above that has contents %s\n", contents[2] );
	Warning( "Candidate brush IDs: " );

	CUtlVector<int> listed;
	for (int j=0 ; j<2 ; j++)
	{
		node_t *n = p->nodes[j];
		for (bspbrush_t *bb=n->brushlist ; bb ; bb=bb->next)
		{
			mapbrush_t *brush = bb->original;
			if ( brush->contents & viscontents )
			{
				if ( listed.Find( brush->brushnum ) == -1 )
				{
					listed.AddToTail( brush->brushnum );
					Warning( "Brush %d: ", brush->id );
				}
			}
		}
	}
	Warning( "\n\n" );
}


//==============================================================

/*
============
FindPortalSide

Finds a brush side to use for texturing the given portal
============
*/
void FindPortalSide (portal_t *p)
{
	int			viscontents;
	bspbrush_t	*bb;
	mapbrush_t	*brush;
	node_t		*n;
	int			i,j;
	int			planenum;
	side_t		*side, *bestside;
	float		bestdist;
	plane_t		*p1, *p2;

	// decide which content change is strongest
	// solid > lava > water, etc
	viscontents = VisibleContents (p->nodes[0]->contents ^ p->nodes[1]->contents);
	if (!viscontents)
	{
		return;
	}

	planenum = p->onnode->planenum;
	bestside = NULL;
	bestdist = 1000000;

	for (j=0 ; j<2 ; j++)
	{
		n = p->nodes[j];
		p1 = &g_MainMap->mapplanes[p->onnode->planenum];

		for (bb=n->brushlist ; bb ; bb=bb->next)
		{
			brush = bb->original;
			if ( !(brush->contents & viscontents) )
				continue;

			for (i=0 ; i<brush->numsides ; i++)
			{
				side = &brush->original_sides[i];
				if (side->bevel)
					continue;
				if (side->texinfo == TEXINFO_NODE)
					continue;		// non-visible

				if ((side->planenum&~1) == planenum)
				{	// exact match
					bestside = &brush->original_sides[i];
					bestdist = 0.0f;
					goto gotit;
				}

				p2 = &g_MainMap->mapplanes[side->planenum&~1];

				float dist = ComputeDistFromPlane( p->winding, p2, bestdist );
				if (dist < bestdist)
				{
					bestside = side;
					bestdist = dist;
				}
			}
		}
	}

gotit:
	if (!bestside)
		qprintf ("WARNING: side not found for portal\n");

	// Compute average dist, check for problems...
	if ((bestdist / p->winding->numpoints) > 2)
	{
		static int nWarnCount = 0;
		if ( nWarnCount < 8 )
		{
			DisplayPortalError( p, viscontents );
			if ( ++nWarnCount == 8 )
			{
				Warning("*** Suppressing further FindPortalSide errors.... ***\n" );
			}
		}
	}

	p->sidefound = true;
	p->side = bestside;
}


/*
===============
MarkVisibleSides_r

===============
*/
void MarkVisibleSides_r (node_t *node)
{
	portal_t	*p;
	int			s;

	if (node->planenum != PLANENUM_LEAF)
	{
		MarkVisibleSides_r (node->children[0]);
		MarkVisibleSides_r (node->children[1]);
		return;
	}

	// empty leafs are never boundary leafs
	if (!node->contents)
		return;

	// see if there is a visible face
	for (p=node->portals ; p ; p = p->next[!s])
	{
		s = (p->nodes[0] == node);
		if (!p->onnode)
			continue;		// edge of world
		if (!p->sidefound)
			FindPortalSide (p);
		if (p->side)
			p->side->visible = true;
	}

}

/*
=============
MarkVisibleSides

=============
*/
// UNDONE: Put detail brushes in a separate list (not mapbrushes) ?
void MarkVisibleSides (tree_t *tree, int startbrush, int endbrush, int detailScreen)
{
	int		i, j;
	mapbrush_t	*mb;
	int		numsides;
	qboolean detail;

	qprintf ("--- MarkVisibleSides ---\n");

	// clear all the visible flags
	for (i=startbrush ; i<endbrush ; i++)
	{
		mb = &g_MainMap->mapbrushes[i];

		if ( detailScreen != FULL_DETAIL )
		{
			qboolean onlyDetail = (detailScreen==ONLY_DETAIL)?true:false;
			// true for detail brushes
			detail = (mb->contents & CONTENTS_DETAIL) ? true : false;
			if ( onlyDetail ^ detail )
			{
				// both of these must have the same value or we're not interested in this brush
				continue;
			}
		}

		numsides = mb->numsides;
		for (j=0 ; j<numsides ; j++)
			mb->original_sides[j].visible = false;
	}

	// set visible flags on the sides that are used by portals
	MarkVisibleSides_r (tree->headnode);
}


//-----------------------------------------------------------------------------
// Used to determine which sides are visible
//-----------------------------------------------------------------------------
void MarkVisibleSides (tree_t *tree, mapbrush_t **ppBrushes, int nCount )
{
	qprintf ("--- MarkVisibleSides ---\n");

	// clear all the visible flags
	int	i, j;
	for ( i=0; i < nCount; ++i )
	{
		mapbrush_t *mb = ppBrushes[i];
		int numsides = mb->numsides;
		for (j=0 ; j<numsides ; j++)
		{
			mb->original_sides[j].visible = false;
		}
	}

	// set visible flags on the sides that are used by portals
	MarkVisibleSides_r( tree->headnode );
}

