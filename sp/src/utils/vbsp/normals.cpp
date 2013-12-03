//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "bsplib.h"
#include "vbsp.h"


void SaveVertexNormals( void )
{
	int i, j;
	dface_t *f;
	texinfo_t	*tex;


	g_numvertnormalindices = 0;
	g_numvertnormals = 0;

	for( i = 0 ;i<numfaces ; i++ )
	{
		f = &dfaces[i];
		tex = &texinfo[f->texinfo];

		for( j = 0; j < f->numedges; j++ )
		{
			if( g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES )
			{
				Error( "g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES (%d)", MAX_MAP_VERTNORMALINDICES );
			}
			
			g_vertnormalindices[g_numvertnormalindices] = g_numvertnormals;
			g_numvertnormalindices++;
		}

		// Add this face plane's normal.
		// Note: this doesn't do an exhaustive vertex normal match because the vrad does it.
		// The result is that a little extra memory is wasted coming out of vbsp, but it
		// goes away after vrad.
		if( g_numvertnormals == MAX_MAP_VERTNORMALS )
		{
			Error( "g_numvertnormals == MAX_MAP_VERTNORMALS (%d)", MAX_MAP_VERTNORMALS );
		}

		g_vertnormals[g_numvertnormals] = dplanes[f->planenum].normal;
		g_numvertnormals++;
	}
}
