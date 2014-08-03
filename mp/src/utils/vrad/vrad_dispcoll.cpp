//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vrad.h"
#include "VRAD_DispColl.h"
#include "DispColl_Common.h"
#include "radial.h"
#include "CollisionUtils.h"
#include "tier0\dbg.h"

#define SAMPLE_BBOX_SLOP		5.0f
#define TRIEDGE_EPSILON			0.001f

float g_flMaxDispSampleSize = 512.0f;

static FileHandle_t pDispFile = FILESYSTEM_INVALID_HANDLE;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::CVRADDispColl()
{
	m_iParent = -1;

	m_flSampleRadius2 = 0.0f;
	m_flPatchSampleRadius2 = 0.0f;

	m_flSampleWidth = 0.0f;
	m_flSampleHeight = 0.0f;

	m_aLuxelCoords.Purge();
	m_aVertNormals.Purge();
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::~CVRADDispColl()
{
	m_aLuxelCoords.Purge();
	m_aVertNormals.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::Create( CCoreDispInfo *pDisp )
{
	// Base class create.
	if( !CDispCollTree::Create( pDisp ) )
		return false;

	// Allocate VRad specific memory.
	m_aLuxelCoords.SetSize( GetSize() );
	m_aVertNormals.SetSize( GetSize() );

	// VRad specific base surface data.
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_iParent = pSurf->GetHandle();

	// VRad specific displacement surface data.
	for ( int iVert = 0; iVert < m_aVerts.Count(); ++iVert )
	{
		pDisp->GetNormal( iVert, m_aVertNormals[iVert] );
		pDisp->GetLuxelCoord( 0, iVert, m_aLuxelCoords[iVert] );
	}

	// Re-calculate the lightmap size (in uv) so that the luxels give
	// a better world-space uniform approx. due to the non-linear nature
	// of the displacement surface in uv-space
	dface_t *pFace = &g_pFaces[m_iParent];
	if( pFace )
	{
		CalcSampleRadius2AndBox( pFace );	
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::CalcSampleRadius2AndBox( dface_t *pFace )
{
	// Get the luxel sample size.
	texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
	Assert ( pTexInfo );
	if ( !pTexInfo )
		return;

	// Todo: Width = Height now, should change all the code to look at one value.
	Vector vecTmp( pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][0],
		pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][1],
		pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][2] );
	float flWidth = 1.0f / VectorLength( vecTmp );
	float flHeight = flWidth;
	
	// Save off the sample width and height.
	m_flSampleWidth = flWidth;
	m_flSampleHeight = flHeight;

	// Calculate the sample radius squared.
	float flSampleRadius = sqrt( ( ( flWidth * flWidth ) + ( flHeight * flHeight ) ) ) * 2.2f;//RADIALDIST2; 
	if ( flSampleRadius > g_flMaxDispSampleSize )
	{
		flSampleRadius = g_flMaxDispSampleSize;
	}
	m_flSampleRadius2 = flSampleRadius * flSampleRadius;

	// Calculate the patch radius - the max sample edge length * the number of luxels per edge "chop."
	float flSampleSize = max( m_flSampleWidth, m_flSampleHeight );
	float flPatchSampleRadius = flSampleSize * dispchop * 2.2f;
	if ( flPatchSampleRadius > g_MaxDispPatchRadius )
	{
		flPatchSampleRadius = g_MaxDispPatchRadius;
		Warning( "Patch Sample Radius Clamped!\n" );
	}
	m_flPatchSampleRadius2 = flPatchSampleRadius * flPatchSampleRadius;
}

//-----------------------------------------------------------------------------
// Purpose: Get the min/max of the displacement surface.
//-----------------------------------------------------------------------------
void CVRADDispColl::GetSurfaceMinMax( Vector &boxMin, Vector &boxMax )
{
	// Initialize the minimum and maximum box
	boxMin = m_aVerts[0];
	boxMax = m_aVerts[0];

	for( int i = 1; i < m_aVerts.Count(); i++ )
	{
		if( m_aVerts[i].x < boxMin.x ) { boxMin.x = m_aVerts[i].x; }
		if( m_aVerts[i].y < boxMin.y ) { boxMin.y = m_aVerts[i].y; }
		if( m_aVerts[i].z < boxMin.z ) { boxMin.z = m_aVerts[i].z; }

		if( m_aVerts[i].x > boxMax.x ) { boxMax.x = m_aVerts[i].x; }
		if( m_aVerts[i].y > boxMax.y ) { boxMax.y = m_aVerts[i].y; }
		if( m_aVerts[i].z > boxMax.z ) { boxMax.z = m_aVerts[i].z; }
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the minor projection axes based on the given normal.
//-----------------------------------------------------------------------------
void CVRADDispColl::GetMinorAxes( Vector const &vecNormal, int &nAxis0, int &nAxis1 )
{
	nAxis0 = 0;
	nAxis1 = 1;

	if( FloatMakePositive( vecNormal.x ) > FloatMakePositive( vecNormal.y ) )
	{
		if( FloatMakePositive( vecNormal.x ) > FloatMakePositive( vecNormal.z ) )
		{
			nAxis0 = 1;
			nAxis1 = 2;
		}
	}
	else
	{
		if( FloatMakePositive( vecNormal.y ) > FloatMakePositive( vecNormal.z ) )
		{
			nAxis0 = 0;
			nAxis1 = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::BaseFacePlaneToDispUV( Vector const &vecPlanePt, Vector2D &dispUV )
{
	PointInQuadToBarycentric( m_vecSurfPoints[0], m_vecSurfPoints[3], m_vecSurfPoints[2], m_vecSurfPoints[1], vecPlanePt, dispUV );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfPoint( Vector2D const &dispUV, Vector &vecPoint, float flPushEps )
{
	// Check to see that the point is on the surface.
	if ( dispUV.x < 0.0f || dispUV.x > 1.0f || dispUV.y < 0.0f || dispUV.y > 1.0f )
		return;

	// Get the displacement power.
	int nWidth = ( ( 1 << m_nPower ) + 1 );
	int nHeight = nWidth;

	// Scale the U, V coordinates to the displacement grid size.
	float flU = dispUV.x * static_cast<float>( nWidth - 1.000001f );
	float flV = dispUV.y * static_cast<float>( nHeight - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	// Use this to get the triangle orientation.
	bool bOdd = ( ( ( nSnapV * nWidth ) + nSnapU ) % 2 == 1 );

	// Top Left to Bottom Right
	if( bOdd )
	{
		DispUVToSurf_TriTLToBR( vecPoint, flPushEps, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
	// Bottom Left to Top Right
	else
	{
		DispUVToSurf_TriBLToTR( vecPoint, flPushEps, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurf_TriTLToBR( Vector &vecPoint, float flPushEps, 
											float flU, float flV, int nSnapU, int nSnapV, 
											int nWidth, int nHeight )
{
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( ( flFracU + flFracV ) >= ( 1.0f + TRIEDGE_EPSILON ) )
	{
		int nIndices[3];
		nIndices[0] = nNextV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nNextU;	
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[0]] - m_aVerts[nIndices[1]];
		Vector edgeV = m_aVerts[nIndices[2]] - m_aVerts[nIndices[1]];
		vecPoint = m_aVerts[nIndices[1]] + edgeU * ( 1.0f - flFracU ) + edgeV * ( 1.0f - flFracV );

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
	else
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nSnapU;
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[2]] - m_aVerts[nIndices[0]];
		Vector edgeV = m_aVerts[nIndices[1]] - m_aVerts[nIndices[0]];
		vecPoint = m_aVerts[nIndices[0]] + edgeU * flFracU + edgeV * flFracV;

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurf_TriBLToTR( Vector &vecPoint, float flPushEps, 
											float flU, float flV, int nSnapU, int nSnapV, 
											int nWidth, int nHeight )
{
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( flFracU < flFracV )
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nSnapU;
		nIndices[2] = nNextV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[2]] - m_aVerts[nIndices[1]];
		Vector edgeV = m_aVerts[nIndices[0]] - m_aVerts[nIndices[1]];
		vecPoint =  m_aVerts[nIndices[1]] + edgeU * flFracU + edgeV * ( 1.0f - flFracV );

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
	else
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nNextU;
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[0]] - m_aVerts[nIndices[2]];
		Vector edgeV = m_aVerts[nIndices[1]] - m_aVerts[nIndices[2]];
		vecPoint = m_aVerts[nIndices[2]] + edgeU * ( 1.0f - flFracU ) + edgeV * flFracV;

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfNormal( Vector2D const &dispUV, Vector &vecNormal )
{
	// Check to see that the point is on the surface.
	if ( dispUV.x < 0.0f || dispUV.x > 1.0f || dispUV.y < 0.0f || dispUV.y > 1.0f )
		return;

	// Get the displacement power.
	int nWidth = ( ( 1 << m_nPower ) + 1 );
	int nHeight = nWidth;

	// Scale the U, V coordinates to the displacement grid size.
	float flU = dispUV.x * static_cast<float>( nWidth - 1.000001f );
	float flV = dispUV.y * static_cast<float>( nHeight - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	// Get the four normals "around" the "spot"
	int iQuad[VRAD_QUAD_SIZE];
	iQuad[0] = ( nSnapV * nWidth ) + nSnapU;
	iQuad[1] = ( nNextV * nWidth ) + nSnapU;
	iQuad[2] = ( nNextV * nWidth ) + nNextU;
	iQuad[3] = ( nSnapV * nWidth ) + nNextU;

	// Find the blended normal (bi-linear).
	Vector vecTmpNormals[2], vecBlendedNormals[2], vecDispNormals[4];
	
	for ( int iVert = 0; iVert < VRAD_QUAD_SIZE; ++iVert )
	{
		GetVertNormal( iQuad[iVert], vecDispNormals[iVert] );
	}

	vecTmpNormals[0] = vecDispNormals[0] * ( 1.0f - flFracU );
	vecTmpNormals[1] = vecDispNormals[3] * flFracU;
	vecBlendedNormals[0] = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecBlendedNormals[0] );

	vecTmpNormals[0] = vecDispNormals[1] * ( 1.0f - flFracU );
	vecTmpNormals[1] = vecDispNormals[2] * flFracU;
	vecBlendedNormals[1] = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecBlendedNormals[1] );

	vecTmpNormals[0] = vecBlendedNormals[0] * ( 1.0f - flFracV );
	vecTmpNormals[1] = vecBlendedNormals[1] * flFracV;

	vecNormal = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecNormal );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CVRADDispColl::CreateParentPatches( void )
{
	// Save the total surface area of the displacement.
	float flTotalArea = 0.0f;

	// Get the number of displacement subdivisions.
	int nInterval = GetWidth();

	Vector vecPoints[4];
	vecPoints[0].Init( m_aVerts[0].x, m_aVerts[0].y, m_aVerts[0].z );
	vecPoints[1].Init( m_aVerts[(nInterval*(nInterval-1))].x, m_aVerts[(nInterval*(nInterval-1))].y, m_aVerts[(nInterval*(nInterval-1))].z );
	vecPoints[2].Init( m_aVerts[((nInterval*nInterval)-1)].x, m_aVerts[((nInterval*nInterval)-1)].y, m_aVerts[((nInterval*nInterval)-1)].z );
	vecPoints[3].Init( m_aVerts[(nInterval-1)].x, m_aVerts[(nInterval-1)].y, m_aVerts[(nInterval-1)].z );

	// Create and initialize the patch.
	int iPatch = g_Patches.AddToTail();
	if ( iPatch == g_Patches.InvalidIndex() )
		return flTotalArea;

	// Keep track of the area of the patches.
	float flArea = 0.0f;
	if ( !InitParentPatch( iPatch, vecPoints, flArea ) )
	{
		g_Patches.Remove( iPatch );
		flArea = 0.0f;
	}

	// Return the displacement area.
	return flArea;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iParentPatch - 
//			nLevel - 
//-----------------------------------------------------------------------------
void CVRADDispColl::CreateChildPatchesFromRoot( int iParentPatch, int *pChildPatch )
{
	// Initialize the child patch indices.
	pChildPatch[0] = g_Patches.InvalidIndex();
	pChildPatch[1] = g_Patches.InvalidIndex();

	// Get the number of displacement subdivisions.
	int nInterval = GetWidth();

	// Get the parent patch.
	CPatch *pParentPatch = &g_Patches[iParentPatch];
	if ( !pParentPatch )
		return;

	// Split along the longest edge.
	Vector vecEdges[4];
	vecEdges[0] = pParentPatch->winding->p[1] - pParentPatch->winding->p[0];
	vecEdges[1] = pParentPatch->winding->p[2] - pParentPatch->winding->p[1];
	vecEdges[2] = pParentPatch->winding->p[3] - pParentPatch->winding->p[2];
	vecEdges[3] = pParentPatch->winding->p[3] - pParentPatch->winding->p[0];

	// Should the patch be subdivided - check the area.
	float flMaxLength  = max( m_flSampleWidth, m_flSampleHeight );
	float flMinEdgeLength = flMaxLength * dispchop;

	// Find the longest edge.
	float flEdgeLength = 0.0f;
	int iLongEdge = -1;
	for ( int iEdge = 0; iEdge < 4; ++iEdge )
	{
		float flLength = vecEdges[iEdge].Length();
		if ( flEdgeLength < flLength )
		{
			flEdgeLength = vecEdges[iEdge].Length();
			iLongEdge = iEdge;
		}
	}

	// Small enough already, return.
	if ( flEdgeLength < flMinEdgeLength )
		return;

	// Test area as well so we don't allow slivers.
	float flMinArea = ( dispchop * flMaxLength ) * ( dispchop * flMaxLength );
	Vector vecNormal = vecEdges[3].Cross( vecEdges[0] );
	float flTestArea = VectorNormalize( vecNormal );
	if ( flTestArea < flMinArea )
		return;

	// Get the points for the first triangle.
	int iPoints[3];
	Vector vecPoints[3];
	float flArea;

	iPoints[0] = ( nInterval * nInterval ) - 1;
	iPoints[1] = 0;
	iPoints[2] = nInterval * ( nInterval - 1 );
	for ( int iPoint = 0; iPoint < 3; ++iPoint )
	{
		VectorCopy( m_aVerts[iPoints[iPoint]], vecPoints[iPoint] );
	}

	// Create and initialize the patch.
	pChildPatch[0] = g_Patches.AddToTail();
	if ( pChildPatch[0] == g_Patches.InvalidIndex() )
		return;

	if ( !InitPatch( pChildPatch[0], iParentPatch, 0, vecPoints, iPoints, flArea ) )
	{
		g_Patches.Remove( pChildPatch[0] );
		pChildPatch[0] = g_Patches.InvalidIndex();
		return;
	}

	// Get the points for the second triangle.
	iPoints[0] = 0;
	iPoints[1] = ( nInterval * nInterval ) - 1;
	iPoints[2] = nInterval - 1;
	for ( int iPoint = 0; iPoint < 3; ++iPoint )
	{
		VectorCopy( m_aVerts[iPoints[iPoint]], vecPoints[iPoint] );
	}

	// Create and initialize the patch.
	pChildPatch[1] = g_Patches.AddToTail();
	if ( pChildPatch[1] == g_Patches.InvalidIndex() )
	{
		g_Patches.Remove( pChildPatch[0] );
		pChildPatch[0] = g_Patches.InvalidIndex();
		return;
	}

	if ( !InitPatch( pChildPatch[1], iParentPatch, 1, vecPoints, iPoints, flArea ) )
	{
		g_Patches.Remove( pChildPatch[0] );
		pChildPatch[0] = g_Patches.InvalidIndex();
		g_Patches.Remove( pChildPatch[1] );
		pChildPatch[1] = g_Patches.InvalidIndex();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinArea - 
// Output : float
//-----------------------------------------------------------------------------
void CVRADDispColl::CreateChildPatches( int iParentPatch, int nLevel )
{
	// Get the parent patch.
	CPatch *pParentPatch = &g_Patches[iParentPatch];
	if ( !pParentPatch )
		return;

	// The root face is a quad - special case.
	if ( pParentPatch->winding->numpoints == 4 )
	{
		int iChildPatch[2];
		CreateChildPatchesFromRoot( iParentPatch, iChildPatch );
		if ( iChildPatch[0] != g_Patches.InvalidIndex() && iChildPatch[1] != g_Patches.InvalidIndex() )
		{
			CreateChildPatches( iChildPatch[0], 0 );
			CreateChildPatches( iChildPatch[1], 0 );
		}
		return;
	}

	// Calculate the the area of the patch (triangle!).
	Assert( pParentPatch->winding->numpoints == 3 );
	if ( pParentPatch->winding->numpoints != 3 )
		return;

	// Should the patch be subdivided - check the area.
	float flMaxLength  = max( m_flSampleWidth, m_flSampleHeight );
	float flMinEdgeLength = flMaxLength * dispchop;

	// Split along the longest edge.
	Vector vecEdges[3];
	vecEdges[0] = pParentPatch->winding->p[1] - pParentPatch->winding->p[0];
	vecEdges[1] = pParentPatch->winding->p[2] - pParentPatch->winding->p[0];
	vecEdges[2] = pParentPatch->winding->p[2] - pParentPatch->winding->p[1];

	// Find the longest edge.
	float flEdgeLength = 0.0f;
	int iLongEdge = -1;
	for ( int iEdge = 0; iEdge < 3; ++iEdge )
	{
		if ( flEdgeLength < vecEdges[iEdge].Length() )
		{
			flEdgeLength = vecEdges[iEdge].Length();
			iLongEdge = iEdge;
		}
	}

	// Small enough already, return.
	if ( flEdgeLength < flMinEdgeLength )
		return;

	// Test area as well so we don't allow slivers.
	float flMinArea = ( dispchop * flMaxLength ) * ( dispchop * flMaxLength ) * 0.5f;
	Vector vecNormal = vecEdges[1].Cross( vecEdges[0] );
	float flTestArea = VectorNormalize( vecNormal );
	flTestArea *= 0.5f;
	if ( flTestArea < flMinArea )
		return;

	// Check to see if any more displacement verts exist - go to subdivision if not.
	if ( nLevel >= ( m_nPower * 2 ) )
	{
		CreateChildPatchesSub( iParentPatch );
		return;
	}

	int nChildIndices[2][3];
	int nNewIndex = ( pParentPatch->indices[1] + pParentPatch->indices[0] ) / 2;
	nChildIndices[0][0] = pParentPatch->indices[2];
	nChildIndices[0][1] = pParentPatch->indices[0];
	nChildIndices[0][2] = nNewIndex;

	nChildIndices[1][0] = pParentPatch->indices[1];
	nChildIndices[1][1] = pParentPatch->indices[2];
	nChildIndices[1][2] = nNewIndex;

	Vector vecChildPoints[2][3];
	for ( int iTri = 0; iTri < 2; ++iTri )
	{
		for ( int iPoint = 0; iPoint < 3; ++iPoint )
		{
			VectorCopy( m_aVerts[nChildIndices[iTri][iPoint]], vecChildPoints[iTri][iPoint] );
		}
	}

	// Create and initialize the children patches.
	int iChildPatch[2] = { -1, -1 };
	for ( int iChild = 0; iChild < 2; ++iChild )
	{
		iChildPatch[iChild] = g_Patches.AddToTail();

		float flArea = 0.0f;
		if ( !InitPatch( iChildPatch[iChild], iParentPatch, iChild, vecChildPoints[iChild], nChildIndices[iChild], flArea ) )
		{
			if ( iChild == 0 )
			{
				pParentPatch->child1 = g_Patches.InvalidIndex();
				g_Patches.Remove( iChildPatch[iChild] );
				break;
			}
			else
			{
				pParentPatch->child1 = g_Patches.InvalidIndex();
				pParentPatch->child2 = g_Patches.InvalidIndex();
				g_Patches.Remove( iChildPatch[iChild] );
				g_Patches.Remove( iChildPatch[0] );
			}
		}
	}
	
	// Continue creating children patches.
	int nNewLevel = ++nLevel;
	CreateChildPatches( iChildPatch[0], nNewLevel );
	CreateChildPatches( iChildPatch[1], nNewLevel );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinArea - 
// Output : float
//-----------------------------------------------------------------------------
void CVRADDispColl::CreateChildPatchesSub( int iParentPatch )
{
	// Get the parent patch.
	CPatch *pParentPatch = &g_Patches[iParentPatch];
	if ( !pParentPatch )
		return;

	// Calculate the the area of the patch (triangle!).
	Assert( pParentPatch->winding->numpoints == 3 );
	if ( pParentPatch->winding->numpoints != 3 )
		return;

	// Should the patch be subdivided - check the area.
	float flMaxLength  = max( m_flSampleWidth, m_flSampleHeight );
	float flMinEdgeLength = flMaxLength * dispchop;

	// Split along the longest edge.
	Vector vecEdges[3];
	vecEdges[0] = pParentPatch->winding->p[1] - pParentPatch->winding->p[0];
	vecEdges[1] = pParentPatch->winding->p[2] - pParentPatch->winding->p[1];
	vecEdges[2] = pParentPatch->winding->p[0] - pParentPatch->winding->p[2];

	// Find the longest edge.
	float flEdgeLength = 0.0f;
	int iLongEdge = -1;
	for ( int iEdge = 0; iEdge < 3; ++iEdge )
	{
		if ( flEdgeLength < vecEdges[iEdge].Length() )
		{
			flEdgeLength = vecEdges[iEdge].Length();
			iLongEdge = iEdge;
		}
	}

	// Small enough already, return.
	if ( flEdgeLength < flMinEdgeLength )
		return;

	// Test area as well so we don't allow slivers.
	float flMinArea = ( dispchop * flMaxLength ) * ( dispchop * flMaxLength ) * 0.5f;
	Vector vecNormal = vecEdges[1].Cross( vecEdges[0] );
	float flTestArea = VectorNormalize( vecNormal );
	flTestArea *= 0.5f;
	if ( flTestArea < flMinArea )
		return;

	// Create children patchs - 2 of them.
	Vector vecChildPoints[2][3];
	switch ( iLongEdge )
	{
	case 0:
		{
			vecChildPoints[0][0] = pParentPatch->winding->p[0];
			vecChildPoints[0][1] = ( pParentPatch->winding->p[0] + pParentPatch->winding->p[1] ) * 0.5f;
			vecChildPoints[0][2] = pParentPatch->winding->p[2];

			vecChildPoints[1][0] = ( pParentPatch->winding->p[0] + pParentPatch->winding->p[1] ) * 0.5f;
			vecChildPoints[1][1] = pParentPatch->winding->p[1];
			vecChildPoints[1][2] = pParentPatch->winding->p[2];
			break;
		}
	case 1:
		{
			vecChildPoints[0][0] = pParentPatch->winding->p[0];
			vecChildPoints[0][1] = pParentPatch->winding->p[1];
			vecChildPoints[0][2] = ( pParentPatch->winding->p[1] + pParentPatch->winding->p[2] ) * 0.5f;

			vecChildPoints[1][0] = ( pParentPatch->winding->p[1] + pParentPatch->winding->p[2] ) * 0.5f;
			vecChildPoints[1][1] = pParentPatch->winding->p[2];
			vecChildPoints[1][2] = pParentPatch->winding->p[0];
			break;
		}
	case 2:
		{
			vecChildPoints[0][0] = pParentPatch->winding->p[0];
			vecChildPoints[0][1] = pParentPatch->winding->p[1];
			vecChildPoints[0][2] = ( pParentPatch->winding->p[0] + pParentPatch->winding->p[2] ) * 0.5f;

			vecChildPoints[1][0] = ( pParentPatch->winding->p[0] + pParentPatch->winding->p[2] ) * 0.5f;
			vecChildPoints[1][1] = pParentPatch->winding->p[1];
			vecChildPoints[1][2] = pParentPatch->winding->p[2];
			break;
		}
	}


	// Create and initialize the children patches.
	int iChildPatch[2] = { 0, 0 };
	int nChildIndices[3] = { -1, -1, -1 };
	for ( int iChild = 0; iChild < 2; ++iChild )
	{
		iChildPatch[iChild] = g_Patches.AddToTail();

		float flArea = 0.0f;
		if ( !InitPatch( iChildPatch[iChild], iParentPatch, iChild, vecChildPoints[iChild], nChildIndices, flArea ) )
		{
			if ( iChild == 0 )
			{
				pParentPatch->child1 = g_Patches.InvalidIndex();
				g_Patches.Remove( iChildPatch[iChild] );
				break;
			}
			else
			{
				pParentPatch->child1 = g_Patches.InvalidIndex();
				pParentPatch->child2 = g_Patches.InvalidIndex();
				g_Patches.Remove( iChildPatch[iChild] );
				g_Patches.Remove( iChildPatch[0] );
			}
		}
	}
	
	// Continue creating children patches.
	CreateChildPatchesSub( iChildPatch[0] );
	CreateChildPatchesSub( iChildPatch[1] );
}

int	PlaneTypeForNormal (Vector& normal)
{
	vec_t	ax, ay, az;

	// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] == 1.0 || normal[0] == -1.0)
		return PLANE_X;
	if (normal[1] == 1.0 || normal[1] == -1.0)
		return PLANE_Y;
	if (normal[2] == 1.0 || normal[2] == -1.0)
		return PLANE_Z;

	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);

	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iPatch - 
//			iParentPatch - 
//			iChild - 
//			*pPoints - 
//			*pIndices - 
//			&flArea - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVRADDispColl::InitParentPatch( int iPatch, Vector *pPoints, float &flArea )
{
	// Get the current patch.
	CPatch *pPatch = &g_Patches[iPatch];
	if ( !pPatch )
		return false;

	// Clear the patch data.
	memset( pPatch, 0, sizeof( CPatch ) );

	// This is a parent.
	pPatch->ndxNext = g_FacePatches.Element( GetParentIndex() );
	g_FacePatches[GetParentIndex()] = iPatch;
	pPatch->faceNumber = GetParentIndex();

	// Initialize parent and children indices.
	pPatch->child1 = g_Patches.InvalidIndex();
	pPatch->child2 = g_Patches.InvalidIndex();
	pPatch->parent = g_Patches.InvalidIndex();
	pPatch->ndxNextClusterChild = g_Patches.InvalidIndex();
	pPatch->ndxNextParent = g_Patches.InvalidIndex();

	Vector vecEdges[2];
	vecEdges[0] = pPoints[1] - pPoints[0];
	vecEdges[1] = pPoints[3] - pPoints[0];

	// Calculate the triangle normal and area.
	Vector vecNormal = vecEdges[1].Cross( vecEdges[0] );
	flArea = VectorNormalize( vecNormal );

	// Initialize the patch scale.
	pPatch->scale[0] = pPatch->scale[1] = 1.0f;

	// Set the patch chop - minchop (that is what the minimum area is based on).
	pPatch->chop = dispchop;

	// Displacements are not sky!
	pPatch->sky = false;

	// Copy the winding.
	Vector vecCenter( 0.0f, 0.0f, 0.0f );
	pPatch->winding = AllocWinding( 4 );
	pPatch->winding->numpoints = 4;
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		VectorCopy( pPoints[iPoint], pPatch->winding->p[iPoint] );
		VectorAdd( pPoints[iPoint], vecCenter, vecCenter );
	}

	// Set the origin and normal.
	VectorScale( vecCenter, ( 1.0f / 4.0f ), vecCenter );
	VectorCopy( vecCenter, pPatch->origin );
	VectorCopy( vecNormal, pPatch->normal );

	// Create the plane.
	pPatch->plane = new dplane_t;
	if ( !pPatch->plane )
		return false;

	VectorCopy( vecNormal, pPatch->plane->normal );
	pPatch->plane->dist = vecNormal.Dot( pPoints[0] );
	pPatch->plane->type = PlaneTypeForNormal( pPatch->plane->normal );
	pPatch->planeDist = pPatch->plane->dist;

	// Set the area.
	pPatch->area = flArea;

	// Calculate the mins/maxs.
	Vector vecMin( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector vecMax( FLT_MIN, FLT_MIN, FLT_MIN );
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		for ( int iAxis = 0; iAxis < 3; ++iAxis )
		{
			vecMin[iAxis] = min( vecMin[iAxis], pPoints[iPoint][iAxis] );
			vecMax[iAxis] = max( vecMax[iAxis], pPoints[iPoint][iAxis] );
		}
	}

	VectorCopy( vecMin, pPatch->mins );
	VectorCopy( vecMax, pPatch->maxs );
	VectorCopy( vecMin, pPatch->face_mins );
	VectorCopy( vecMax, pPatch->face_maxs );

	// Check for bumpmap.
	dface_t *pFace = dfaces + pPatch->faceNumber;
	texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
	pPatch->needsBumpmap = pTexInfo->flags & SURF_BUMPLIGHT ? true : false;

	// Misc...
	pPatch->m_IterationKey = 0;

	// Calculate the base light, area, and reflectivity.
	BaseLightForFace( &g_pFaces[pPatch->faceNumber], pPatch->baselight, &pPatch->basearea, pPatch->reflectivity );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPatch - 
//			*pPoints - 
//			&vecNormal - 
//			flArea - 
//-----------------------------------------------------------------------------
bool CVRADDispColl::InitPatch( int iPatch, int iParentPatch, int iChild, Vector *pPoints, int *pIndices, float &flArea )
{
	// Get the current patch.
	CPatch *pPatch = &g_Patches[iPatch];
	if ( !pPatch )
		return false;

	// Clear the patch data.
	memset( pPatch, 0, sizeof( CPatch ) );

	// Setup the parent if we are not the parent.
	CPatch *pParentPatch = NULL;
	if ( iParentPatch != g_Patches.InvalidIndex() )
	{
		// Get the parent patch.
		pParentPatch = &g_Patches[iParentPatch];
		if ( !pParentPatch )
			return false;
	}

	// Attach the face to the correct lists.
	if ( !pParentPatch )
	{
		// This is a parent.
		pPatch->ndxNext = g_FacePatches.Element( GetParentIndex() );
		g_FacePatches[GetParentIndex()] = iPatch;
		pPatch->faceNumber = GetParentIndex();
	}
	else
	{
		pPatch->ndxNext = g_Patches.InvalidIndex();
		pPatch->faceNumber = pParentPatch->faceNumber;

		// Attach to the parent patch.
		if ( iChild == 0 )
		{
			pParentPatch->child1 = iPatch;
		}
		else
		{
			pParentPatch->child2 = iPatch;
		}
	}

	// Initialize parent and children indices.
	pPatch->child1 = g_Patches.InvalidIndex();
	pPatch->child2 = g_Patches.InvalidIndex();
	pPatch->ndxNextClusterChild = g_Patches.InvalidIndex();
	pPatch->ndxNextParent = g_Patches.InvalidIndex();
	pPatch->parent = iParentPatch;

	// Get triangle edges.
	Vector vecEdges[3];
	vecEdges[0] = pPoints[1] - pPoints[0];
	vecEdges[1] = pPoints[2] - pPoints[0];
	vecEdges[2] = pPoints[2] - pPoints[1];

	// Find the longest edge.
//	float flEdgeLength = 0.0f;
//	for ( int iEdge = 0; iEdge < 3; ++iEdge )
//	{
//		if ( flEdgeLength < vecEdges[iEdge].Length() )
//		{
//			flEdgeLength = vecEdges[iEdge].Length();
//		}
//	}

	// Calculate the triangle normal and area.
	Vector vecNormal = vecEdges[1].Cross( vecEdges[0] );
	flArea = VectorNormalize( vecNormal );
	flArea *= 0.5f;

	// Initialize the patch scale.
	pPatch->scale[0] = pPatch->scale[1] = 1.0f;

	// Set the patch chop - minchop (that is what the minimum area is based on).
	pPatch->chop = dispchop;

	// Displacements are not sky!
	pPatch->sky = false;

	// Copy the winding.
	Vector vecCenter( 0.0f, 0.0f, 0.0f );
	pPatch->winding = AllocWinding( 3 );
	pPatch->winding->numpoints = 3;
	for ( int iPoint = 0; iPoint < 3; ++iPoint )
	{
		VectorCopy( pPoints[iPoint], pPatch->winding->p[iPoint] );
		VectorAdd( pPoints[iPoint], vecCenter, vecCenter );

		pPatch->indices[iPoint] = static_cast<short>( pIndices[iPoint] );
	}

	// Set the origin and normal.
	VectorScale( vecCenter, ( 1.0f / 3.0f ), vecCenter );
	VectorCopy( vecCenter, pPatch->origin );
	VectorCopy( vecNormal, pPatch->normal );

	// Create the plane.
	pPatch->plane = new dplane_t;
	if ( !pPatch->plane )
		return false;

	VectorCopy( vecNormal, pPatch->plane->normal );
	pPatch->plane->dist = vecNormal.Dot( pPoints[0] );
	pPatch->plane->type = PlaneTypeForNormal( pPatch->plane->normal );
	pPatch->planeDist = pPatch->plane->dist;

	// Set the area.
	pPatch->area = flArea;

	// Calculate the mins/maxs.
	Vector vecMin( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector vecMax( FLT_MIN, FLT_MIN, FLT_MIN );
	for ( int iPoint = 0; iPoint < 3; ++iPoint )
	{
		for ( int iAxis = 0; iAxis < 3; ++iAxis )
		{
			vecMin[iAxis] = min( vecMin[iAxis], pPoints[iPoint][iAxis] );
			vecMax[iAxis] = max( vecMax[iAxis], pPoints[iPoint][iAxis] );
		}
	}

	VectorCopy( vecMin, pPatch->mins );
	VectorCopy( vecMax, pPatch->maxs );

	if ( !pParentPatch )
	{
		VectorCopy( vecMin, pPatch->face_mins );
		VectorCopy( vecMax, pPatch->face_maxs );
	}
	else
	{
		VectorCopy( pParentPatch->face_mins, pPatch->face_mins );
		VectorCopy( pParentPatch->face_maxs, pPatch->face_maxs );
	}

	// Check for bumpmap.
	dface_t *pFace = dfaces + pPatch->faceNumber;
	texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
	pPatch->needsBumpmap = pTexInfo->flags & SURF_BUMPLIGHT ? true : false;

	// Misc...
	pPatch->m_IterationKey = 0;

	// Get the base light for the face.
	if ( !pParentPatch )
	{
		BaseLightForFace( &g_pFaces[pPatch->faceNumber], pPatch->baselight, &pPatch->basearea, pPatch->reflectivity );
	}
	else
	{
		VectorCopy( pParentPatch->baselight, pPatch->baselight );
		pPatch->basearea = pParentPatch->basearea;
		pPatch->reflectivity = pParentPatch->reflectivity;
	}

	return true;
}

void CVRADDispColl::AddPolysForRayTrace( void )
{
	if ( !( m_nContents & MASK_OPAQUE ) )
		return;

	for ( int ndxTri = 0; ndxTri < m_aTris.Size(); ndxTri++ )
	{
		CDispCollTri *tri = m_aTris.Base() + ndxTri;
		int v[3];
		for ( int ndxv = 0; ndxv < 3; ndxv++ )
			v[ndxv] = tri->GetVert(ndxv);

		Vector fullCoverage;
		fullCoverage.x = 1.0f;
		g_RtEnv.AddTriangle( TRACE_ID_OPAQUE, m_aVerts[v[0]], m_aVerts[v[1]], m_aVerts[v[2]], fullCoverage );
	}
}