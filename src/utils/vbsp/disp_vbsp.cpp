//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "disp_vbsp.h"
#include "tier0/dbg.h"
#include "vbsp.h"
#include "mstristrip.h"
#include "writebsp.h"
#include "pacifier.h"
#include "disp_ivp.h"
#include "builddisp.h"
#include "mathlib/vector.h"

// map displacement info -- runs parallel to the dispinfos struct
int              nummapdispinfo = 0;
mapdispinfo_t    mapdispinfo[MAX_MAP_DISPINFO];

CUtlVector<CCoreDispInfo*> g_CoreDispInfos;

//-----------------------------------------------------------------------------
// Computes the bounds for a disp info
//-----------------------------------------------------------------------------
void ComputeDispInfoBounds( int dispinfo, Vector& mins, Vector& maxs )
{
	CDispBox box;

	// Get a CCoreDispInfo. All we need is the triangles and lightmap texture coordinates.
	mapdispinfo_t *pMapDisp = &mapdispinfo[dispinfo];

	CCoreDispInfo coreDispInfo;
	DispMapToCoreDispInfo( pMapDisp, &coreDispInfo, NULL, NULL );

	GetDispBox( &coreDispInfo, box );	
	mins = box.m_Min;
	maxs = box.m_Max;
}

// Gets the barycentric coordinates of the position on the triangle where the lightmap
// coordinates are equal to lmCoords. This always generates the coordinates but it
// returns false if the point containing them does not lie inside the triangle.
bool GetBarycentricCoordsFromLightmapCoords( Vector2D tri[3], Vector2D const &lmCoords, float bcCoords[3] )
{
	GetBarycentricCoords2D( tri[0], tri[1], tri[2], lmCoords, bcCoords );

	return 
		(bcCoords[0] >= 0.0f && bcCoords[0] <= 1.0f) && 
		(bcCoords[1] >= 0.0f && bcCoords[1] <= 1.0f) && 
		(bcCoords[2] >= 0.0f && bcCoords[2] <= 1.0f);
}


bool FindTriIndexMapByUV( CCoreDispInfo *pCoreDisp, Vector2D const &lmCoords,
						  int &iTriangle, float flBarycentric[3] )
{
	const CPowerInfo *pPowerInfo = GetPowerInfo( pCoreDisp->GetPower() );

	// Search all the triangles..
	int nTriCount= pCoreDisp->GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		unsigned short iVerts[3];
//		pCoreDisp->GetTriIndices( iTri, iVerts[0], iVerts[1], iVerts[2] );
		CTriInfo *pTri = &pPowerInfo->m_pTriInfos[iTri];
		iVerts[0] = pTri->m_Indices[0];
		iVerts[1] = pTri->m_Indices[1];
		iVerts[2] = pTri->m_Indices[2];

		// Get this triangle's UVs.
		Vector2D vecUV[3];
		for ( int iCoord = 0; iCoord < 3; ++iCoord )
		{
			pCoreDisp->GetLuxelCoord( 0, iVerts[iCoord], vecUV[iCoord] );
		}

		// See if the passed-in UVs are in this triangle's UVs.
		if( GetBarycentricCoordsFromLightmapCoords( vecUV, lmCoords, flBarycentric ) )
		{
			iTriangle = iTri;
			return true;
		}
	}		

	return false;
}


void CalculateLightmapSamplePositions( CCoreDispInfo *pCoreDispInfo, const dface_t *pFace, CUtlVector<unsigned char> &out )
{
	int width  = pFace->m_LightmapTextureSizeInLuxels[0] + 1;
	int height = pFace->m_LightmapTextureSizeInLuxels[1] + 1;

	// For each lightmap sample, find the triangle it sits in.
	Vector2D lmCoords;
	for( int y=0; y < height; y++ )
	{
		lmCoords.y = y + 0.5f;

		for( int x=0; x < width; x++ )
		{
			lmCoords.x = x + 0.5f;

			float flBarycentric[3];
			int iTri;

			if( FindTriIndexMapByUV( pCoreDispInfo, lmCoords, iTri, flBarycentric ) )
			{
				if( iTri < 255 )
				{
					out.AddToTail( iTri );
				}
				else
				{
					out.AddToTail( 255 );
					out.AddToTail( iTri - 255 );
				}

				out.AddToTail( (unsigned char)( flBarycentric[0] * 255.9f ) );
				out.AddToTail( (unsigned char)( flBarycentric[1] * 255.9f ) );
				out.AddToTail( (unsigned char)( flBarycentric[2] * 255.9f ) );
			}
			else
			{
				out.AddToTail( 0 );
				out.AddToTail( 0 );
				out.AddToTail( 0 );
				out.AddToTail( 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetDispInfoEntityNum( mapdispinfo_t *pDisp )
{
	return pDisp->entitynum;
}

// Setup a CCoreDispInfo given a mapdispinfo_t.
// If pFace is non-NULL, then lightmap texture coordinates will be generated.
void DispMapToCoreDispInfo( mapdispinfo_t *pMapDisp, CCoreDispInfo *pCoreDispInfo, dface_t *pFace, int *pSwappedTexInfos )
{
	winding_t *pWinding = pMapDisp->face.originalface->winding;

	Assert( pWinding->numpoints == 4 );

	//
	// set initial surface data
	//
	CCoreDispSurface *pSurf = pCoreDispInfo->GetSurface();

	texinfo_t *pTexInfo = &texinfo[ pMapDisp->face.texinfo ];
	Assert( pTexInfo != NULL );

	// init material contents
	pMapDisp->contents = pMapDisp->face.contents;
	if (!(pMapDisp->contents & (ALL_VISIBLE_CONTENTS | CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) ) )
	{
		pMapDisp->contents |= CONTENTS_SOLID;
	}

	pSurf->SetContents( pMapDisp->contents );

	// Calculate the lightmap coordinates.
	Vector2D tCoords[4] = {Vector2D(0,0),Vector2D(0,1),Vector2D(1,0),Vector2D(1,1)};
	if( pFace )
	{
		Assert( pFace->numedges == 4 );

		Vector pt[4];
		for( int i=0; i < 4; i++ )
			pt[i] = pWinding->p[i];

		int zeroOffset[2] = {0,0};
		CalcTextureCoordsAtPoints( 
			pTexInfo->textureVecsTexelsPerWorldUnits,
			zeroOffset,
			pt,
			4,
			tCoords );
	}
	
	//
	// set face point data ...
	//
	pSurf->SetPointCount( 4 );
	for( int i = 0; i < 4; i++ )
	{
		// position
		pSurf->SetPoint( i, pWinding->p[i] );
		pSurf->SetTexCoord( i, tCoords[i] );
	}
	
	// reset surface given start info
	pSurf->SetPointStart( pMapDisp->startPosition );
	pSurf->FindSurfPointStartIndex();
	pSurf->AdjustSurfPointData();

	// Set the luxel coordinates on the base displacement surface.
	Vector vecTmp( pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][0],
				   pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][1],
				   pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][2] );
	int nLuxelsPerWorldUnit = static_cast<int>( 1.0f / VectorLength( vecTmp ) );
	Vector vecU( pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][0],
				 pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][1],
				 pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][2] );
	Vector vecV( pTexInfo->lightmapVecsLuxelsPerWorldUnits[1][0],
				 pTexInfo->lightmapVecsLuxelsPerWorldUnits[1][1],
				 pTexInfo->lightmapVecsLuxelsPerWorldUnits[1][2] );
	bool bSwap = pSurf->CalcLuxelCoords( nLuxelsPerWorldUnit, false, vecU, vecV );

	// Set the face m_LightmapExtents
	if ( pFace )
	{
		pFace->m_LightmapTextureSizeInLuxels[0] = pSurf->GetLuxelU();
		pFace->m_LightmapTextureSizeInLuxels[1] = pSurf->GetLuxelV();
		if ( bSwap )
		{
			if ( pSwappedTexInfos[ pMapDisp->face.texinfo ] < 0 )
			{
				// Create a new texinfo to hold the swapped data.
				// We must do this because other surfaces may want the non-swapped data
				// This fixes a lighting bug in d2_prison_08 where many non-displacement surfaces
				// were pitch black, in addition to bugs in other maps I bet.

				// NOTE: Copy here because adding a texinfo could realloc.
				texinfo_t temp = *pTexInfo;
				memcpy( temp.lightmapVecsLuxelsPerWorldUnits[0], pTexInfo->lightmapVecsLuxelsPerWorldUnits[1], 4 * sizeof(float) );
				memcpy( temp.lightmapVecsLuxelsPerWorldUnits[1], pTexInfo->lightmapVecsLuxelsPerWorldUnits[0], 4 * sizeof(float) );
				temp.lightmapVecsLuxelsPerWorldUnits[1][0] *= -1.0f;
				temp.lightmapVecsLuxelsPerWorldUnits[1][1] *= -1.0f;
				temp.lightmapVecsLuxelsPerWorldUnits[1][2] *= -1.0f;
				temp.lightmapVecsLuxelsPerWorldUnits[1][3] *= -1.0f;
				pSwappedTexInfos[ pMapDisp->face.texinfo ] = texinfo.AddToTail( temp );
			}
			pMapDisp->face.texinfo = pSwappedTexInfos[ pMapDisp->face.texinfo ];
		}

		// NOTE: This is here to help future-proof code, since there are codepaths where
		// pTexInfo can be made invalid (texinfo.AddToTail above).
		pTexInfo = NULL;
	}

	// Setup the displacement vectors and offsets.
	int size = ( ( ( 1 << pMapDisp->power ) + 1 ) * ( ( 1 << pMapDisp->power ) + 1 ) );

	Vector vectorDisps[2048];
	float dispDists[2048];
	Assert( size < sizeof(vectorDisps)/sizeof(vectorDisps[0]) );

	for( int j = 0; j < size; j++ )
	{
		Vector v;
		float dist;

		VectorScale( pMapDisp->vectorDisps[j], pMapDisp->dispDists[j], v );
		VectorAdd( v, pMapDisp->vectorOffsets[j], v );
		
		dist = VectorLength( v );
		VectorNormalize( v );
		
		vectorDisps[j] = v;
		dispDists[j] = dist;
	}


	// Use CCoreDispInfo to setup the actual vertex positions.
	pCoreDispInfo->InitDispInfo( pMapDisp->power, pMapDisp->minTess, pMapDisp->smoothingAngle,
						 pMapDisp->alphaValues, vectorDisps, dispDists );
	pCoreDispInfo->Create();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void EmitInitialDispInfos( void )
{
	int					i;
	mapdispinfo_t		*pMapDisp;
	ddispinfo_t			*pDisp;
	Vector				v;
	
	// Calculate the total number of verts.
	int nTotalVerts = 0;
	int nTotalTris = 0;
	for ( i=0; i < nummapdispinfo; i++ )
	{
		nTotalVerts += NUM_DISP_POWER_VERTS( mapdispinfo[i].power );
		nTotalTris += NUM_DISP_POWER_TRIS( mapdispinfo[i].power );
	}

	// Clear the output arrays..
	g_dispinfo.Purge();
	g_dispinfo.SetSize( nummapdispinfo );
	g_DispVerts.SetSize( nTotalVerts );
	g_DispTris.SetSize( nTotalTris );

	int iCurVert = 0;
	int iCurTri = 0;
	for( i = 0; i < nummapdispinfo; i++ )
	{
		pDisp = &g_dispinfo[i];
		pMapDisp = &mapdispinfo[i];
		
		CDispVert *pOutVerts = &g_DispVerts[iCurVert];
		CDispTri *pOutTris = &g_DispTris[iCurTri];

		// Setup the vert pointers.
		pDisp->m_iDispVertStart = iCurVert;
		pDisp->m_iDispTriStart = iCurTri;
		iCurVert += NUM_DISP_POWER_VERTS( pMapDisp->power );
		iCurTri += NUM_DISP_POWER_TRIS( pMapDisp->power );

		//
		// save power, minimum tesselation, and smoothing angle
		//
		pDisp->power = pMapDisp->power;
		
		// If the high bit is set - this is FLAGS!
		pDisp->minTess = pMapDisp->flags;
		pDisp->minTess |= 0x80000000;
//		pDisp->minTess = pMapDisp->minTess;
		pDisp->smoothingAngle = pMapDisp->smoothingAngle;
		pDisp->m_iMapFace = (unsigned short)-2;

		// get surface contents
		pDisp->contents = pMapDisp->face.contents;
		
		pDisp->startPosition = pMapDisp->startPosition;
		//
		// add up the vectorOffsets and displacements, save alphas (per vertex)
		//
		int size = ( ( ( 1 << pDisp->power ) + 1 ) * ( ( 1 << pDisp->power ) + 1 ) );
		for( int j = 0; j < size; j++ )
		{
			VectorScale( pMapDisp->vectorDisps[j], pMapDisp->dispDists[j], v );
			VectorAdd( v, pMapDisp->vectorOffsets[j], v );
			
			float dist = VectorLength( v );
			VectorNormalize( v );
			
			VectorCopy( v, pOutVerts[j].m_vVector );
			pOutVerts[j].m_flDist = dist;

			pOutVerts[j].m_flAlpha = pMapDisp->alphaValues[j];
		}

		int nTriCount = ( (1 << (pDisp->power)) * (1 << (pDisp->power)) * 2 );
		for ( int iTri = 0; iTri< nTriCount; ++iTri )
		{
			pOutTris[iTri].m_uiTags = pMapDisp->triTags[iTri];
		}
		//===================================================================
		//===================================================================

		// save the index for face data reference
		pMapDisp->face.dispinfo = i;
	}
}


void ExportCoreDispNeighborData( const CCoreDispInfo *pIn, ddispinfo_t *pOut )
{
	for ( int i=0; i < 4; i++ )
	{
		pOut->m_EdgeNeighbors[i] = *pIn->GetEdgeNeighbor( i );
		pOut->m_CornerNeighbors[i] = *pIn->GetCornerNeighbors( i );
	}
}

void ExportNeighborData( CCoreDispInfo **ppListBase, ddispinfo_t *pBSPDispInfos, int listSize )
{
	FindNeighboringDispSurfs( ppListBase, listSize );

	// Export the neighbor data.
	for ( int i=0; i < nummapdispinfo; i++ )
	{
		ExportCoreDispNeighborData( g_CoreDispInfos[i], &pBSPDispInfos[i] );
	}
}


void ExportCoreDispAllowedVertList( const CCoreDispInfo *pIn, ddispinfo_t *pOut )
{
	ErrorIfNot( 
		pIn->GetAllowedVerts().GetNumDWords() == sizeof( pOut->m_AllowedVerts ) / 4,
		("ExportCoreDispAllowedVertList: size mismatch")
		);
	for ( int i=0; i < pIn->GetAllowedVerts().GetNumDWords(); i++ )
		pOut->m_AllowedVerts[i] = pIn->GetAllowedVerts().GetDWord( i );
}


void ExportAllowedVertLists( CCoreDispInfo **ppListBase, ddispinfo_t *pBSPDispInfos, int listSize )
{
	SetupAllowedVerts( ppListBase, listSize );

	for ( int i=0; i < listSize; i++ )
	{
		ExportCoreDispAllowedVertList( ppListBase[i], &pBSPDispInfos[i] );
	}
}

bool FindEnclosingTri( 
	const Vector2D &vert,
	CUtlVector<Vector2D> &vertCoords,
	CUtlVector<unsigned short> &indices,
	int *pStartVert,
	float bcCoords[3] )
{
	for ( int i=0; i < indices.Count(); i += 3 )
	{
		GetBarycentricCoords2D( 
			vertCoords[indices[i+0]],
			vertCoords[indices[i+1]],
			vertCoords[indices[i+2]],
			vert,
			bcCoords );

		if ( bcCoords[0] >= 0 && bcCoords[0] <= 1 && 
			bcCoords[1] >= 0 && bcCoords[1] <= 1 && 
			bcCoords[2] >= 0 && bcCoords[2] <= 1 )
		{
			*pStartVert = i;
			return true;
		}
	}

	return false;
}

void SnapRemainingVertsToSurface( CCoreDispInfo *pCoreDisp, ddispinfo_t *pDispInfo )
{
	// First, tesselate the displacement.
	CUtlVector<unsigned short> indices;
	CVBSPTesselateHelper helper;
	helper.m_pIndices = &indices;
	helper.m_pActiveVerts = pCoreDisp->GetAllowedVerts().Base();
	helper.m_pPowerInfo = pCoreDisp->GetPowerInfo();
	::TesselateDisplacement( &helper );

	// Figure out which verts are actually referenced in the tesselation.
	CUtlVector<bool> vertsTouched;
	vertsTouched.SetSize( pCoreDisp->GetSize() );
	memset( vertsTouched.Base(), 0, sizeof( bool ) * vertsTouched.Count() );

	for ( int i=0; i < indices.Count(); i++ )
		vertsTouched[ indices[i] ] = true;

	// Generate 2D floating point coordinates for each vertex. We use these to generate
	// barycentric coordinates, and the scale doesn't matter.
	CUtlVector<Vector2D> vertCoords;
	vertCoords.SetSize( pCoreDisp->GetSize() );
	for ( int y=0; y < pCoreDisp->GetHeight(); y++ )
	{
		for ( int x=0; x < pCoreDisp->GetWidth(); x++ )
			vertCoords[y*pCoreDisp->GetWidth()+x].Init( x, y );
	}
	
	// Now, for each vert not touched, snap its position to the main surface.
	for ( int y=0; y < pCoreDisp->GetHeight(); y++ )
	{
		for ( int x=0; x < pCoreDisp->GetWidth(); x++ )
		{
			int index = y * pCoreDisp->GetWidth() + x;
			if ( !( vertsTouched[index] ) )
			{
				float bcCoords[3];
				int iStartVert = -1;
				if ( FindEnclosingTri( vertCoords[index], vertCoords, indices, &iStartVert, bcCoords ) )
				{
					const Vector &A = pCoreDisp->GetVert( indices[iStartVert+0] );
					const Vector &B = pCoreDisp->GetVert( indices[iStartVert+1] );
					const Vector &C = pCoreDisp->GetVert( indices[iStartVert+2] );
					Vector vNewPos = A*bcCoords[0] + B*bcCoords[1] + C*bcCoords[2];

					// This is kind of cheesy, but it gets the job done. Since the CDispVerts store the
					// verts relative to some other offset, we'll just offset their position instead
					// of setting it directly.
					Vector vOffset = vNewPos - pCoreDisp->GetVert( index );

					// Modify the mapfile vert.
					CDispVert *pVert = &g_DispVerts[pDispInfo->m_iDispVertStart + index];
					pVert->m_vVector = (pVert->m_vVector * pVert->m_flDist) + vOffset;
					pVert->m_flDist = 1;

					// Modify the CCoreDispInfo vert (although it probably won't be used later).
					pCoreDisp->SetVert( index, vNewPos );
				}
				else
				{
					// This shouldn't happen because it would mean that the triangulation that 
					// disp_tesselation.h produced was missing a chunk of the space that the
					// displacement covers. 
					// It also could indicate a floating-point epsilon error.. check to see if
					// FindEnclosingTri finds a triangle that -almost- encloses the vert.
					Assert( false );
				}
			}
		}
	} 
}

void SnapRemainingVertsToSurface( CCoreDispInfo **ppListBase, ddispinfo_t *pBSPDispInfos, int listSize )
{
//g_pPad = ScratchPad3D_Create();
	for ( int i=0; i < listSize; i++ )
	{
		SnapRemainingVertsToSurface( ppListBase[i], &pBSPDispInfos[i] );
	}
}

void EmitDispLMAlphaAndNeighbors()
{

	int i;

	Msg( "Finding displacement neighbors...\n" );

	// Build the CCoreDispInfos.
	CUtlVector<dface_t*> faces;

	// Create the core dispinfos and init them for use as CDispUtilsHelpers.
	for ( int iDisp = 0; iDisp < nummapdispinfo; ++iDisp )
	{
		CCoreDispInfo *pDisp = new CCoreDispInfo;
		if ( !pDisp )
		{
			g_CoreDispInfos.Purge();
			return;
		}

		int nIndex = g_CoreDispInfos.AddToTail();
		pDisp->SetListIndex( nIndex );
		g_CoreDispInfos[nIndex] = pDisp;
	}

	for ( i=0; i < nummapdispinfo; i++ )
	{
		g_CoreDispInfos[i]->SetDispUtilsHelperInfo( g_CoreDispInfos.Base(), nummapdispinfo );
	}

	faces.SetSize( nummapdispinfo );

	int nMemSize = texinfo.Count() * sizeof(int);
	int *pSwappedTexInfos = (int*)stackalloc( nMemSize );
	memset( pSwappedTexInfos, 0xFF, nMemSize );
	for( i = 0; i < numfaces; i++ )
	{
        dface_t *pFace = &dfaces[i];

		if( pFace->dispinfo == -1 )
			continue;

		mapdispinfo_t *pMapDisp = &mapdispinfo[pFace->dispinfo];
		
		// Set the displacement's face index.
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];
		pDisp->m_iMapFace = i;

		// Get a CCoreDispInfo. All we need is the triangles and lightmap texture coordinates.
		CCoreDispInfo *pCoreDispInfo = g_CoreDispInfos[pFace->dispinfo];
		DispMapToCoreDispInfo( pMapDisp, pCoreDispInfo, pFace, pSwappedTexInfos );
		
		faces[pFace->dispinfo] = pFace;
	}
	stackfree( pSwappedTexInfos );
	
	// Generate and export neighbor data.
	ExportNeighborData( g_CoreDispInfos.Base(), g_dispinfo.Base(), nummapdispinfo );

	// Generate and export the active vert lists.
	ExportAllowedVertLists( g_CoreDispInfos.Base(), g_dispinfo.Base(), nummapdispinfo );

	
	// Now that we know which vertices are actually going to be around, snap the ones that won't
	// be around onto the slightly-reduced mesh. This is so the engine's ray test code and 
	// overlay code works right.
	SnapRemainingVertsToSurface( g_CoreDispInfos.Base(), g_dispinfo.Base(), nummapdispinfo );

	Msg( "Finding lightmap sample positions...\n" );
	for ( i=0; i < nummapdispinfo; i++ )
	{
		dface_t *pFace = faces[i];
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];
		CCoreDispInfo *pCoreDispInfo = g_CoreDispInfos[i];

		pDisp->m_iLightmapSamplePositionStart = g_DispLightmapSamplePositions.Count();

		CalculateLightmapSamplePositions( pCoreDispInfo, pFace, g_DispLightmapSamplePositions );
	}

	StartPacifier( "Displacement Alpha : ");

	// Build lightmap alphas.
	int dispCount = 0;	// How many we've processed.
	for( i = 0; i < nummapdispinfo; i++ )
	{
        dface_t *pFace = faces[i];

		Assert( pFace->dispinfo == i );
		ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];

		// Allocate space for the alpha values.
		pDisp->m_iLightmapAlphaStart = 0; // not used anymore
		
		++dispCount;
	}

	EndPacifier();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispGetFaceInfo( mapbrush_t *pBrush )
{
	int		i;
	side_t	*pSide;

	// we don't support displacement on entities at the moment!!
	if( pBrush->entitynum != 0 )
	{
		char* pszEntityName = ValueForKey( &g_LoadingMap->entities[pBrush->entitynum], "classname" );
		Error( "Error: displacement found on a(n) %s entity - not supported (entity %d, brush %d)\n", pszEntityName, pBrush->entitynum, pBrush->brushnum );
	}

	for( i = 0; i < pBrush->numsides; i++ )
	{
		pSide = &pBrush->original_sides[i];
		if( pSide->pMapDisp )
		{
			// error checking!!
			if( pSide->winding->numpoints != 4 )
				Error( "Trying to create a non-quad displacement! (entity %d, brush %d)\n", pBrush->entitynum, pBrush->brushnum );
			pSide->pMapDisp->face.originalface = pSide;
			pSide->pMapDisp->face.texinfo = pSide->texinfo;
			pSide->pMapDisp->face.dispinfo = -1;
			pSide->pMapDisp->face.planenum = pSide->planenum;
			pSide->pMapDisp->face.numpoints = pSide->winding->numpoints;
			pSide->pMapDisp->face.w = CopyWinding( pSide->winding );
			pSide->pMapDisp->face.contents = pBrush->contents;

			pSide->pMapDisp->face.merged = FALSE;
			pSide->pMapDisp->face.split[0] = FALSE;
			pSide->pMapDisp->face.split[1] = FALSE;

			pSide->pMapDisp->entitynum = pBrush->entitynum;
			pSide->pMapDisp->brushSideID = pSide->id;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool HasDispInfo( mapbrush_t *pBrush )
{
	int		i;
	side_t	*pSide;

	for( i = 0; i < pBrush->numsides; i++ )
	{
		pSide = &pBrush->original_sides[i];
		if( pSide->pMapDisp )
			return true;
	}

	return false;
}
