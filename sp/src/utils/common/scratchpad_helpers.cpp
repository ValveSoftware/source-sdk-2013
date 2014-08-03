//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "scratchpad_helpers.h"
#include "bspfile.h"
#include "bsplib.h"


void ScratchPad_DrawWinding( 
	IScratchPad3D *pPad, 
	int nPoints, 
	Vector *pPoints, 
	Vector vColor, 
	Vector vOffset )
{
	for ( int i=0; i < nPoints; i++ )
	{
		pPad->DrawLine( CSPVert( pPoints[i]+vOffset, vColor ), CSPVert( pPoints[(i+1)%nPoints]+vOffset, vColor ) );
	}
}


void ScratchPad_DrawFace( IScratchPad3D *pPad, dface_t *f, int iFaceNumber, const CSPColor &faceColor, const Vector &vOffset )
{
	// Draw the face's outline, then put text for its face index on it too.
	CUtlVector<Vector> points;
	for ( int iEdge = 0; iEdge < f->numedges; iEdge++ )
	{
		int v;
		int se = dsurfedges[f->firstedge + iEdge];
		if ( se < 0 )
			v = dedges[-se].v[1];
		else
			v = dedges[se].v[0];
	
		dvertex_t *dv = &dvertexes[v];
		points.AddToTail( dv->point );
	}

	// Draw the outline.
	Vector vCenter( 0, 0, 0 );
	for ( int iEdge=0; iEdge < points.Count(); iEdge++ )
	{
		pPad->DrawLine( CSPVert( points[iEdge]+vOffset, faceColor ), CSPVert( points[(iEdge+1)%points.Count()]+vOffset, faceColor ) );
		vCenter += points[iEdge];
	}
	vCenter /= points.Count();
	vCenter += vOffset;

	// Draw the text.
	if ( iFaceNumber != -1 )
	{
		char str[64];
		Q_snprintf( str, sizeof( str ), "%d", iFaceNumber );

		CTextParams params;

		params.m_bCentered = true;
		params.m_bOutline = true;
		params.m_flLetterWidth = 2;
		params.m_vColor.Init( 1, 0, 0 );
		
		VectorAngles( dplanes[f->planenum].normal, params.m_vAngles );
		params.m_bTwoSided = true;

		params.m_vPos = vCenter;
		
		pPad->DrawText( str, params );
	}
}


void ScratchPad_DrawWorld( IScratchPad3D *pPad, bool bDrawFaceNumbers, const CSPColor &faceColor )
{
	bool bAutoFlush = pPad->GetAutoFlush();
	pPad->SetAutoFlush( false );

	for ( int i=0; i < numleafs; i++ )
	{
		dleaf_t *l = &dleafs[i];
		if ( l->contents & CONTENTS_DETAIL )
			continue;
			
		for ( int z=0; z < l->numleaffaces; z++ )
		{
			int iFace = dleaffaces[l->firstleafface+z];
			dface_t *f = &dfaces[iFace];
			ScratchPad_DrawFace( pPad, f, bDrawFaceNumbers ? i : -1 );
		}
	}

	pPad->SetAutoFlush( bAutoFlush );
}


void ScratchPad_DrawWorld( bool bDrawFaceNumbers, const CSPColor &faceColor )
{
	IScratchPad3D *pPad = ScratchPad3D_Create();
	ScratchPad_DrawWorld( pPad, bDrawFaceNumbers );
}
