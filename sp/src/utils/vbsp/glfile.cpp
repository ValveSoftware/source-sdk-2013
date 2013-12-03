//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vbsp.h"

int		c_glfaces;

int PortalVisibleSides (portal_t *p)
{
	int		fcon, bcon;

	if (!p->onnode)
		return 0;		// outside

	fcon = p->nodes[0]->contents;
	bcon = p->nodes[1]->contents;

	// same contents never create a face
	if (fcon == bcon)
		return 0;

	// FIXME: is this correct now?
	if (!fcon)
		return 1;
	if (!bcon)
		return 2;
	return 0;
}

void OutputWinding (winding_t *w, FileHandle_t glview)
{
	static	int	level = 128;
	vec_t		light;
	int			i;

	CmdLib_FPrintf( glview, "%i\n", w->numpoints);
	level+=28;
	light = (level&255)/255.0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		CmdLib_FPrintf(glview, "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
			w->p[i][0],
			w->p[i][1],
			w->p[i][2],
			light,
			light,
			light);
	}
	//CmdLib_FPrintf(glview, "\n");
}

void OutputWindingColor (winding_t *w, FileHandle_t glview, int r, int g, int b)
{
	int			i;

	CmdLib_FPrintf( glview, "%i\n", w->numpoints);
	float lr = r * (1.0f/255.0f);
	float lg = g * (1.0f/255.0f);
	float lb = b * (1.0f/255.0f);
	for (i=0 ; i<w->numpoints ; i++)
	{
		CmdLib_FPrintf(glview, "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
			w->p[i][0],
			w->p[i][1],
			w->p[i][2],
			lr,
			lg,
			lb);
	}
	//CmdLib_FPrintf(glview, "\n");
}

/*
=============
OutputPortal
=============
*/
void OutputPortal (portal_t *p, FileHandle_t glview)
{
	winding_t	*w;
	int		sides;

	sides = PortalVisibleSides (p);
	if (!sides)
		return;

	c_glfaces++;

	w = p->winding;

	if (sides == 2)		// back side
		w = ReverseWinding (w);

	OutputWinding (w, glview);

	if (sides == 2)
		FreeWinding(w);
}

/*
=============
WriteGLView_r
=============
*/
void WriteGLView_r (node_t *node, FileHandle_t glview)
{
	portal_t	*p, *nextp;

	if (node->planenum != PLANENUM_LEAF)
	{
		WriteGLView_r (node->children[0], glview);
		WriteGLView_r (node->children[1], glview);
		return;
	}

	// write all the portals
	for (p=node->portals ; p ; p=nextp)
	{
		if (p->nodes[0] == node)
		{
			OutputPortal (p, glview);
			nextp = p->next[0];
		}
		else
			nextp = p->next[1];
	}
}


void WriteGLViewFaces_r( node_t *node, FileHandle_t glview )
{
	portal_t	*p, *nextp;

	if (node->planenum != PLANENUM_LEAF)
	{
		WriteGLViewFaces_r (node->children[0], glview);
		WriteGLViewFaces_r (node->children[1], glview);
		return;
	}

	// write all the portals
	for (p=node->portals ; p ; p=nextp)
	{
		int s = (p->nodes[1] == node);

		if ( p->face[s] )
		{
			OutputWinding( p->face[s]->w, glview );
		}
		nextp = p->next[s];
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView (tree_t *tree, char *source)
{
	char	name[1024];
	FileHandle_t glview;

	c_glfaces = 0;
	sprintf (name, "%s%s.gl",outbase, source);
	Msg("Writing %s\n", name);

	glview = g_pFileSystem->Open( name, "w" );
	if (!glview)
		Error ("Couldn't open %s", name);
	WriteGLView_r (tree->headnode, glview);
	g_pFileSystem->Close( glview );

	Msg("%5i c_glfaces\n", c_glfaces);
}


void WriteGLViewFaces( tree_t *tree, const char *pName )
{
	char	name[1024];
	FileHandle_t glview;

	c_glfaces = 0;
	sprintf (name, "%s%s.gl", outbase, pName);
	Msg("Writing %s\n", name);

	glview = g_pFileSystem->Open( name, "w" );
	if (!glview)
		Error ("Couldn't open %s", name);
	WriteGLViewFaces_r (tree->headnode, glview);
	g_pFileSystem->Close( glview );

	Msg("%5i c_glfaces\n", c_glfaces);
}


void WriteGLViewBrushList( bspbrush_t *pList, const char *pName )
{
	char	name[1024];
	FileHandle_t glview;

	sprintf (name, "%s%s.gl", outbase, pName );
	Msg("Writing %s\n", name);

	glview = g_pFileSystem->Open( name, "w" );
	if (!glview)
	Error ("Couldn't open %s", name);
	for ( bspbrush_t *pBrush = pList; pBrush; pBrush = pBrush->next )
	{
		for (int i =  0; i < pBrush->numsides; i++ )
			OutputWinding( pBrush->sides[i].winding, glview );
	}
	g_pFileSystem->Close( glview );
}
