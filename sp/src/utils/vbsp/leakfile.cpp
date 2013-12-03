//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"
#include "color.h"

/*
==============================================================================

LEAF FILE GENERATION

Save out name.line for qe3 to read
==============================================================================
*/


/*
=============
LeakFile

Finds the shortest possible chain of portals
that leads from the outside leaf to a specifically
occupied leaf
=============
*/
void LeakFile (tree_t *tree)
{
	Vector	mid;
	FILE	*linefile;
	char	filename[1024];
	node_t	*node;
	int		count;

	if (!tree->outside_node.occupied)
		return;

	tree->leaked = true;
	qprintf ("--- LeakFile ---\n");

	//
	// write the points to the file
	//
	sprintf (filename, "%s.lin", source);
	linefile = fopen (filename, "w");
	if (!linefile)
		Error ("Couldn't open %s\n", filename);

	count = 0;
	node = &tree->outside_node;
	while (node->occupied > 1)
	{
		portal_t	*nextportal = NULL;
		node_t		*nextnode = NULL;
		int			s = 0;

		// find the best portal exit
		int next = node->occupied;
		for (portal_t *p=node->portals ; p ; p = p->next[!s])
		{
			s = (p->nodes[0] == node);
			if (p->nodes[s]->occupied
				&& p->nodes[s]->occupied < next)
			{
				nextportal = p;
				nextnode = p->nodes[s];
				next = nextnode->occupied;
			}
		}
		node = nextnode;
		WindingCenter (nextportal->winding, mid);
		fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);
		count++;
	}

	// Add the occupant's origin to the leakfile.
	Vector origin;
	GetVectorForKey (node->occupant, "origin", origin);

	fprintf (linefile, "%f %f %f\n", origin[0], origin[1], origin[2]);
	qprintf ("%5i point linefile\n", count+1);

	fclose (linefile);

	// Emit a leak warning.
	const char *cl = ValueForKey (node->occupant, "classname");
	Color red(255,0,0,255);
	ColorSpewMessage( SPEW_MESSAGE, &red, "Entity %s (%.2f %.2f %.2f) leaked!\n", cl, origin[0], origin[1], origin[2] );
}

void AreaportalLeakFile( tree_t *tree, portal_t *pStartPortal, portal_t *pEndPortal, node_t *pStart )
{
	Vector	mid;
	FILE	*linefile;
	char	filename[1024];
	node_t	*node;
	int		count;

	// wrote a leak line file already, don't overwrite it with the areaportal leak file
	if ( tree->leaked )
		return;

	tree->leaked = true;
	qprintf ("--- LeakFile ---\n");

	//
	// write the points to the file
	//
	sprintf (filename, "%s.lin", source);
	linefile = fopen (filename, "w");
	if (!linefile)
		Error ("Couldn't open %s\n", filename);

	count = 2;
	WindingCenter (pEndPortal->winding, mid);
	fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);
	mid = 0.5 * (pStart->mins + pStart->maxs);
	fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);

	node = pStart;
	while (node->occupied >= 1)
	{
		portal_t	*nextportal = NULL;
		node_t		*nextnode = NULL;
		int			s = 0;

		// find the best portal exit
		int next = node->occupied;
		for (portal_t *p=node->portals ; p ; p = p->next[!s])
		{
			s = (p->nodes[0] == node);
			if (p->nodes[s]->occupied
				&& p->nodes[s]->occupied < next)
			{
				nextportal = p;
				nextnode = p->nodes[s];
				next = nextnode->occupied;
			}
		}
		if ( !nextnode )
			break;
		node = nextnode;
		WindingCenter (nextportal->winding, mid);
		fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);
		count++;
	}
	// add the occupant center
	if ( node )
	{
		mid = 0.5 * (node->mins + node->maxs);
		fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);
		count++;
	}
	WindingCenter (pStartPortal->winding, mid);
	count++;
	fprintf (linefile, "%f %f %f\n", mid[0], mid[1], mid[2]);

	qprintf ("%5i point linefile\n", count);

	fclose (linefile);
	Warning( "Wrote %s\n", filename );
	Color red(255,0,0,255);
	ColorSpewMessage( SPEW_MESSAGE, &red, "Areaportal leak ! File: %s ", filename );
}