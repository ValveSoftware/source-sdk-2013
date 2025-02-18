//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

//#include <stdafx.h>
#include <stdlib.h>
#include <malloc.h>
#include "builddisp.h"
#include "collisionutils.h"
#include "tier1/strtools.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// Node Functions (friend functions)
//

//-----------------------------------------------------------------------------
// should make this more programatic and extensible!
//-----------------------------------------------------------------------------
int GetNodeLevel( int index )
{
    // root
    if( index == 0 )
        return 1;

    // [1...4]
    if( index < 5 )
        return 2;

    // [5....20]
    if( index < 21 )
        return 3;

    // [21....84]
    if( index < 85 )
        return 4;

    // error!!!
    return -1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeCount( int power )
{
    return ( ( 1 << ( power << 1 ) ) / 3 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeParent( int index )
{
	// ( index - 1 ) / 4
	return ( ( index - 1 ) >> 2 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeChild( int power, int index, int direction )
{
    // ( index * 4 ) + direction
    return ( ( index << 2 ) + ( direction - 3 ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeMinNodeAtLevel( int level )
{
    switch( level )
    {
    case 1: return 0;
    case 2: return 1;
    case 3: return 5;
    case 4: return 21;
    default: return -99999;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetComponentsFromNodeIndex( int index, int *x, int *y )
{
	*x = 0;
	*y = 0;

	for( int shift = 0; index != 0; shift++ )
	{
		*x |= ( index & 1 ) << shift;
		index >>= 1;

		*y |= ( index & 1 ) << shift;
		index >>= 1;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeIndexFromComponents( int x, int y )
{
	int index = 0;

	// Interleave bits from the x and y values to create the index:
	
	int shift;
	for( shift = 0; x != 0; shift += 2, x >>= 1 )
	{
		index |= ( x & 1 ) << shift;
	}

	for( shift = 1; y != 0; shift += 2, y >>= 1 )
	{
		index |= ( y & 1 ) << shift;
	}

	return index;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CalcBarycentricCooefs( Vector const &v0, Vector const &v1, Vector const &v2,
						    Vector const &pt, float &c0, float &c1, float &c2 )
{
	Vector vSeg0, vSeg1, vCross;
	vSeg0 = v1 - v0;
	vSeg1 = v2 - v0;

	// get the area of the triangle
	vCross = vSeg0.Cross( vSeg1 );
	float totalArea = vCross.Length() * 0.5f;
	float ooTotalArea = totalArea ? 1.0f / totalArea : 0.0f;

	// get the area for cooeficient 0 (pt, v1, v2)
	vSeg0 = v1 - pt;
	vSeg1 = v2 - pt;
	vCross = vSeg0.Cross( vSeg1 );
	float subArea = vCross.Length() * 0.5f;
	c0 = subArea * ooTotalArea;

	// get the area for cooeficient 1 (v0, pt, v2)
	vSeg0 = v2 - pt;
	vSeg1 = v0 - pt;
	vCross = vSeg0.Cross( vSeg1 );
	subArea = vCross.Length() * 0.5f;
	c1 = subArea * ooTotalArea;

	// get the area for cooeficient 2 (v0, v1, pt)
	vSeg0 = v0 - pt;
	vSeg1 = v1 - pt;
	vCross = vSeg0.Cross( vSeg1 );
	subArea = vCross.Length() * 0.5f;
	c2 = subArea * ooTotalArea;

	float cTotal = c0 + c1 + c2;
	if ( FloatMakePositive( 1.0f - cTotal ) < 1e-3 )
		return true;

	return false;
}

// For some reason, the global optimizer screws up the recursion here.  disable the global optimizations to fix this.
// IN VC++ 6.0
#pragma optimize( "g", off )

CCoreDispSurface::CCoreDispSurface()
{
	Init();
}


//=============================================================================
//
// CDispSurface Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispSurface::Init( void )
{
	m_Index = -1;

	m_PointCount = 0;
	int i;
	for( i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		VectorClear( m_Points[i] );
		VectorClear( m_Normals[i] );
		Vector2DClear( m_TexCoords[i] );

		for( int j = 0; j < NUM_BUMP_VECTS+1; j++ )
		{
			Vector2DClear( m_LuxelCoords[i][j] );
		}

		m_Alphas[i] = 1.0f;
	}

	m_PointStartIndex = -1;
	VectorClear( m_PointStart );
	VectorClear( sAxis );
	VectorClear( tAxis );

	for( i = 0; i < 4; i++ )
	{
		m_EdgeNeighbors[i].SetInvalid();
		m_CornerNeighbors[i].SetInvalid();
	}

	m_Flags = 0;
	m_Contents = 0;
}


void CCoreDispSurface::SetNeighborData( const CDispNeighbor edgeNeighbors[4], const CDispCornerNeighbors cornerNeighbors[4] )
{
	for ( int i=0; i < 4; i++ )
	{
		m_EdgeNeighbors[i] = edgeNeighbors[i];
		m_CornerNeighbors[i] = cornerNeighbors[i];
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispSurface::GeneratePointStartIndexFromMappingAxes( Vector const &sAxis, Vector const &tAxis )
{
	if( m_PointStartIndex != -1 )
		return;
	
	int	numIndices = 0;
    int indices[4];
    int offsetIndex;

    //
    // project all points on to the v-axis first and find the minimum
    //
	float minValue = DotProduct( tAxis, m_Points[0] );
    indices[numIndices] = 0;
    numIndices++;

	int i;
    for( i = 1; i < m_PointCount; i++ )
    {
		float value = DotProduct( tAxis, m_Points[i] );
		float delta = ( value - minValue );
		delta = FloatMakePositive( delta );
        if( delta < 0.1 )
        {
            indices[numIndices] = i;
            numIndices++;
        }
        else if( value < minValue )
        {
            minValue = value;
            indices[0] = i;
            numIndices = 1;
        }
    }

    //
    // break ties with the u-axis projection
    //
	minValue = DotProduct( sAxis, m_Points[indices[0]] );
    offsetIndex = indices[0];
    
    for( i = 1; i < numIndices; i++ )
    {
		float value = DotProduct( sAxis, m_Points[indices[i]] );
        if( ( value < minValue ) )
        {
            minValue = value;
            offsetIndex = indices[i];
        }
    }

	m_PointStartIndex = offsetIndex;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CCoreDispSurface::GenerateSurfPointStartIndex( void )
{
	//
	// get the minimum surface component values
	//
	Vector bMin;
	VectorFill( bMin, 99999.0f );

	int i;
	for( i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( m_Points[i][j] < bMin[j] )
			{
				bMin[j] = m_Points[i][j];
			}
		}
	}

	//
	// find the point closest to the minimum, that is the start point
	//
	int minIndex = -1;
	float minDistance = 999999999.0f;
	for( i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		Vector segment;
		segment = m_Points[i] - bMin;
		float distanceSq = segment.LengthSqr();
		if( distanceSq < minDistance )
		{
			minDistance = distanceSq;
			minIndex = i;
		}
	}

	m_PointStartIndex = minIndex;

	return minIndex;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CCoreDispSurface::FindSurfPointStartIndex( void )
{
	if( m_PointStartIndex != -1 )
		return m_PointStartIndex;

	int minIndex = -1;
	float minDistance = 999999999.0f;

	for( int i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		Vector segment;
		VectorSubtract( m_PointStart, m_Points[i], segment );
		float distanceSq = segment.LengthSqr();
		if( distanceSq < minDistance )
		{
			minDistance = distanceSq;
			minIndex = i;
		}
	}

	m_PointStartIndex = minIndex;

	return minIndex;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispSurface::AdjustSurfPointData( void )
{
	Vector tmpPoints[4];
	Vector tmpNormals[4];
	Vector2D tmpTexCoords[4];
	float  tmpAlphas[4];

	int i;
	for( i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		VectorCopy( m_Points[i], tmpPoints[i] );
		VectorCopy( m_Normals[i], tmpNormals[i] );
		Vector2DCopy( m_TexCoords[i], tmpTexCoords[i] );

		tmpAlphas[i] = m_Alphas[i];
	}

	for( i = 0; i < QUAD_POINT_COUNT; i++ )
	{
		VectorCopy( tmpPoints[(i+m_PointStartIndex)%4], m_Points[i] );
		VectorCopy( tmpNormals[(i+m_PointStartIndex)%4], m_Normals[i] );
		Vector2DCopy( tmpTexCoords[(i+m_PointStartIndex)%4], m_TexCoords[i] );

		m_Alphas[i] = tmpAlphas[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCoreDispSurface::LongestInU( const Vector &vecU, const Vector &vecV )
{
	Vector vecNormU = vecU;
	Vector vecNormV = vecV;
	VectorNormalize( vecNormU );
	VectorNormalize( vecNormV );

	float flDistU[4];
	float flDistV[4];
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		flDistU[iPoint] = vecNormU.Dot( m_Points[iPoint] );
		flDistV[iPoint] = vecNormV.Dot( m_Points[iPoint] );
	}

	float flULength = 0.0f;
	float flVLength = 0.0f;
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		float flTestDist = fabs( flDistU[(iPoint+1)%4] - flDistU[iPoint] );
		if ( flTestDist > flULength )
		{
			flULength = flTestDist;
		}

		flTestDist = fabs( flDistV[(iPoint+1)%4] - flDistV[iPoint] );
		if ( flTestDist > flVLength )
		{
			flVLength = flTestDist;
		}
	}

	if ( flULength < flVLength )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
bool CCoreDispSurface::CalcLuxelCoords( int nLuxels, bool bAdjust, const Vector &vecU, const Vector &vecV )
{
	// Valid value?
	if ( nLuxels <= 0.0f )
		return false;

	// Get the start point offset.
	int iOffset = 0;
	if ( bAdjust )
	{
		iOffset = GetPointStartIndex();
	}

	// Does projecting along U or V create the longest edge?
	bool bLongU = LongestInU( vecU, vecV );

	float flLengthTemp = 0.0f;
	float flULength = ( m_Points[(3+iOffset)%4] - m_Points[(0+iOffset)%4] ).Length();
	flLengthTemp = ( m_Points[(2+iOffset)%4] - m_Points[(1+iOffset)%4] ).Length();
	if ( flLengthTemp > flULength )
	{
		flULength = flLengthTemp;
	}

	// Find the largest edge in V.
	float flVLength = ( m_Points[(1+iOffset)%4] - m_Points[(0+iOffset)%4] ).Length();
	flLengthTemp = ( m_Points[(2+iOffset)%4] - m_Points[(3+iOffset)%4] ).Length();
	if ( flLengthTemp > flVLength )
	{
		flVLength = flLengthTemp;
	}

	float flOOLuxelScale = 1.0f / static_cast<float>( nLuxels );
	float flUValue = static_cast<float>( static_cast<int>( flULength * flOOLuxelScale ) + 1 );
	if ( flUValue > MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER )
	{
		flUValue = MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER;
	}

	float flVValue = static_cast<float>( static_cast<int>( flVLength * flOOLuxelScale ) + 1 );
	if ( flVValue > MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER )
	{
		flVValue = MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER;
	}

	// Swap if necessary.
	bool bSwapped = false;
	if ( bLongU )
	{
		if ( flVValue > flUValue )
		{
			bSwapped = true;
		}
	}
	else
	{
		if ( flUValue > flVValue )
		{
			bSwapped = true;
		}
	}

	m_nLuxelU = static_cast<int>( flUValue );
	m_nLuxelV = static_cast<int>( flVValue );

	// Generate luxel coordinates.
	for( int iBump = 0; iBump < NUM_BUMP_VECTS+1; ++iBump )
	{
		m_LuxelCoords[iBump][(0+iOffset)%4].Init( 0.5f, 0.5f );
		m_LuxelCoords[iBump][(1+iOffset)%4].Init( 0.5f, flVValue + 0.5 );
		m_LuxelCoords[iBump][(2+iOffset)%4].Init( flUValue + 0.5, flVValue + 0.5 );
		m_LuxelCoords[iBump][(3+iOffset)%4].Init( flUValue + 0.5, 0.5f );
	}

	return bSwapped;
}

//=============================================================================
//
// CDispNode Functions
//


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispNode::Init( void )
{
	VectorClear( m_BBox[0] );
	VectorClear( m_BBox[1] );
	
	m_ErrorTerm = 0.0f;
	
	m_VertIndex = -1;
	
	int j;
	for( j = 0; j < MAX_NEIGHBOR_NODE_COUNT; j++ )
	{
		m_NeighborVertIndices[j] = -1;
	}
	
	for( j = 0; j < MAX_SURF_AT_NODE_COUNT; j++ )
	{
		VectorClear( m_SurfBBoxes[j][0] );
		VectorClear( m_SurfBBoxes[j][1] );
		VectorClear( m_SurfPlanes[j].normal );
		m_SurfPlanes[j].dist = 0.0f;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetDispNodeTriVerts( CCoreDispInfo *pDisp, int nodeIndex, int triIndex, Vector& v1, Vector& v2, Vector& v3 )
{
	// get the node
	CCoreDispNode *pNode = pDisp->GetNode( nodeIndex );

	switch( triIndex )
	{
	case 0:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 4 ), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 0 ), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 3 ), v3 );
			return;
		}
	case 1:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 3 ), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 0 ), v2 );
			pDisp->GetVert( pNode->GetCenterVertIndex(), v3 );
			return;
		}
	case 2:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 3 ), v1 );
			pDisp->GetVert( pNode->GetCenterVertIndex(), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 5 ), v3 );
			return;
		}
	case 3:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 5 ), v1 );
			pDisp->GetVert( pNode->GetCenterVertIndex(), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 2 ), v3 );
			return;
		}
	case 4:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 0 ), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 6 ), v2 );
			pDisp->GetVert( pNode->GetCenterVertIndex(), v3 );
			return;
		}
	case 5:
		{
			pDisp->GetVert( pNode->GetCenterVertIndex(), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 6 ), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 1 ), v3 );
			return;
		}
	case 6:
		{
			pDisp->GetVert( pNode->GetCenterVertIndex(), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 1 ), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 2 ), v3 );
			return;
		}
	case 7:
		{
			pDisp->GetVert( pNode->GetNeighborVertIndex( 2 ), v1 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 1 ), v2 );
			pDisp->GetVert( pNode->GetNeighborVertIndex( 7 ), v3 );
			return;
		}
	default: { return; }
	}
}


//=============================================================================
//
// CCoreDispInfo Functions
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CCoreDispInfo::CCoreDispInfo()
{
	m_pVerts = NULL;
	m_RenderIndices = NULL;
	m_Nodes = NULL;
	m_pTris = NULL;

	// initialize the base surface data
	m_Surf.Init();
	
	//
	// initialize the disp info
	//
	m_Power = 0;
	m_Elevation = 0.0f;
	m_RenderIndexCount = 0;
	m_RenderCounter = 0;	
	m_bTouched = false;

	m_pNext = NULL;

	m_ppListBase = NULL;
	m_ListSize = 0;
	m_nListIndex = -1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CCoreDispInfo::~CCoreDispInfo()
{
	if (m_pVerts)
		delete [] m_pVerts;
	if (m_RenderIndices)
		delete [] m_RenderIndices;
	if (m_Nodes)
		delete [] m_Nodes;
	if (m_pTris)
		delete [] m_pTris;
}


#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::InitSurf( int parentIndex, Vector points[4], Vector normals[4],
		                      Vector2D texCoords[4], Vector2D lightCoords[4][4], int contents, int flags,
							  bool bGenerateSurfPointStart, Vector& startPoint, 
				              bool bHasMappingAxes, Vector& uAxis, Vector& vAxis )
{	
	// save the "parent" index
	m_Surf.m_Index = parentIndex;
	
	//
	// save the surface points and point normals, texture coordinates, and
	// lightmap coordinates
	//
	m_Surf.m_PointCount = CSurface::QUAD_POINT_COUNT;
	for( int i = 0; i < CSurface::QUAD_POINT_COUNT; i++ )
	{
		VectorCopy( points[i], m_Surf.m_Points[i] );

		if( normals )
		{
			VectorCopy( normals[i], m_Surf.m_pVerts[i].m_Normal );
		}

		if( texCoords )
		{
			Vector2DCopy( texCoords[i], m_Surf.m_TexCoords[i] );
		}

		if( lightCoords )
		{
			Assert( NUM_BUMP_VECTS == 3 );
			Vector2DCopy( lightCoords[0][i], m_Surf.m_LightCoords[i][0] );
			Vector2DCopy( lightCoords[1][i], m_Surf.m_LightCoords[i][1] );
			Vector2DCopy( lightCoords[2][i], m_Surf.m_LightCoords[i][2] );
			Vector2DCopy( lightCoords[3][i], m_Surf.m_LightCoords[i][3] );
		}
	}
	
	// save the starting point
	if( startPoint )
	{
		VectorCopy( startPoint, m_Surf.m_PointStart );
	}

	//
	// save the surface contents and flags
	//
	m_Contents = contents;
	m_Flags = flags;	

	//
	// adjust surface points, texture coordinates, etc....
	//
	if( bHasMappingAxes && ( m_Surf.m_PointStartIndex == -1 ) )
	{
		GeneratePointStartIndexFromMappingAxes( uAxis, vAxis );
	}
	else
	{
		//
		// adjust the surf data
		//
		if( bGenerateSurfPointStart )
		{
			GenerateSurfPointStartIndex();
		}
		else
		{
			FindSurfPointStartIndex();
		}
	}

	AdjustSurfPointData();
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::InitDispInfo( int power, int minTess, float smoothingAngle,
		                          float *alphas, Vector *dispVectorField,  float *dispDistances )
{
	Assert( power >= MIN_MAP_DISP_POWER && power <= MAX_MAP_DISP_POWER );

	//
	// general displacement data
	//
	m_Power = power;

	if ( ( minTess & 0x80000000 ) != 0 )
	{
		// If the high bit is set, this represents FLAGS (SURF_NOPHYSICS_COLL, etc.) flags.
		int nFlags = minTess;
		nFlags &= ~0x80000000;
		GetSurface()->SetFlags( nFlags );
	}

	// Allocate + initialize verts
	int size = GetSize();
	m_pVerts = new CoreDispVert_t[size];

	int nIndexCount = size * 2 * 3;
	m_RenderIndices = new unsigned short[nIndexCount];

	int nNodeCount = GetNodeCount(power);
	m_Nodes = new CCoreDispNode[nNodeCount];

	int i;
	for( i = 0; i < size; i++ )
	{
		m_pVerts[i].m_FieldVector.Init();
		m_pVerts[i].m_SubdivPos.Init();
		m_pVerts[i].m_SubdivNormal.Init();
		
		m_pVerts[i].m_FieldDistance = 0.0f;
	
		m_pVerts[i].m_Vert.Init();
		m_pVerts[i].m_FlatVert.Init();
		m_pVerts[i].m_Normal.Init();
		m_pVerts[i].m_TangentS.Init();
		m_pVerts[i].m_TangentT.Init();
		m_pVerts[i].m_TexCoord.Init();

		for( int j = 0; j < ( NUM_BUMP_VECTS + 1 ); j++ )
		{
			m_pVerts[i].m_LuxelCoords[j].Init();
		}

		m_pVerts[i].m_Alpha = 0.0f;
	}

	for( i = 0; i < nIndexCount; i++ )
	{
		m_RenderIndices[i] = 0;
	}

	for( i = 0; i < nNodeCount; i++ )
	{
		m_Nodes[i].Init();
	}

	//
	// save the displacement vector field and distances within the field
	// offset have been combined with fieldvectors at this point!!!
	//
	if (alphas && dispVectorField && dispDistances)
	{
		for( i = 0; i < size; i++ )
		{
			VectorCopy( dispVectorField[i], m_pVerts[i].m_FieldVector );
			m_pVerts[i].m_FieldDistance = dispDistances[i];
			m_pVerts[i].m_Alpha = alphas[i];
		}
	}

	// Init triangle information.
	int nTriCount = GetTriCount();
	if ( nTriCount != 0 )
	{
		m_pTris = new CoreDispTri_t[nTriCount];
		if ( m_pTris )
		{
			InitTris();
		}
	}
}


void CCoreDispInfo::InitDispInfo( int power, int minTess, float smoothingAngle, const CDispVert *pVerts,
								  const CDispTri *pTris )
{
	Vector vectors[MAX_DISPVERTS];
	float dists[MAX_DISPVERTS];
	float alphas[MAX_DISPVERTS];

	int nVerts = NUM_DISP_POWER_VERTS( power );
	for ( int i=0; i < nVerts; i++ )
	{
		vectors[i] = pVerts[i].m_vVector;
		dists[i] = pVerts[i].m_flDist;
		alphas[i] = pVerts[i].m_flAlpha;
	}

	InitDispInfo( power, minTess, smoothingAngle, alphas, vectors, dists );

	int nTris = NUM_DISP_POWER_TRIS( power );
	for ( int iTri = 0; iTri < nTris; ++iTri )
	{
		m_pTris[iTri].m_uiTags = pTris[iTri].m_uiTags;
	}
}


void CCoreDispInfo::SetDispUtilsHelperInfo( CCoreDispInfo **ppListBase, int listSize )
{
	m_ppListBase = ppListBase;
	m_ListSize = listSize;
}

const CPowerInfo* CCoreDispInfo::GetPowerInfo() const
{
	return ::GetPowerInfo( GetPower() );
}

CDispNeighbor* CCoreDispInfo::GetEdgeNeighbor( int index )
{
	return GetSurface()->GetEdgeNeighbor( index );
}

CDispCornerNeighbors* CCoreDispInfo::GetCornerNeighbors( int index )
{
	return GetSurface()->GetCornerNeighbors( index );
}

CDispUtilsHelper* CCoreDispInfo::GetDispUtilsByIndex( int index )
{
	Assert( m_ppListBase );
	return index == 0xFFFF ? 0 : m_ppListBase[index];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::BuildTriTLtoBR( int ndx )
{
    // get width and height of displacement maps
	int nWidth = ( ( 1 << m_Power ) + 1 );
	
	m_RenderIndices[m_RenderIndexCount] = ndx;
	m_RenderIndices[m_RenderIndexCount+1] = ndx + nWidth;
	m_RenderIndices[m_RenderIndexCount+2] = ndx + 1;
	m_RenderIndexCount += 3;

	m_RenderIndices[m_RenderIndexCount] = ndx + 1;
	m_RenderIndices[m_RenderIndexCount+1] = ndx + nWidth;
	m_RenderIndices[m_RenderIndexCount+2] = ndx + nWidth + 1;
	m_RenderIndexCount += 3;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::BuildTriBLtoTR( int ndx )
{
    // get width and height of displacement maps
	int nWidth = ( ( 1 << m_Power ) + 1 );
	
	m_RenderIndices[m_RenderIndexCount] = ndx;
	m_RenderIndices[m_RenderIndexCount+1] = ndx + nWidth;
	m_RenderIndices[m_RenderIndexCount+2] = ndx + nWidth + 1;
	m_RenderIndexCount += 3;

	m_RenderIndices[m_RenderIndexCount] = ndx;
	m_RenderIndices[m_RenderIndexCount+1] = ndx + nWidth + 1;
	m_RenderIndices[m_RenderIndexCount+2] = ndx + 1;
	m_RenderIndexCount += 3;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateCollisionSurface( void )
{
    // get width and height of displacement maps
	int nWidth = ( ( 1 << m_Power ) + 1 );
	int nHeight = ( ( 1 << m_Power ) + 1 );

	//
	// generate a fan tesselated (at quadtree node) rendering index list
	//
	m_RenderIndexCount = 0;
	for ( int iV = 0; iV < ( nHeight - 1 ); iV++ )
	{
		for ( int iU = 0; iU < ( nWidth - 1 ); iU++ )
		{
			int ndx = ( iV * nWidth ) + iU;

			// test whether or not the index is odd
			bool bOdd = ( ( ndx %2 ) == 1 );

			// Top Left to Bottom Right
			if( bOdd )
			{
				BuildTriTLtoBR( ndx );
			}
			// Bottom Left to Top Right
			else
			{
				BuildTriBLtoTR( ndx );
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateCollisionData( void )
{
    GenerateCollisionSurface();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcTriSurfPlanes( int nodeIndex, int indices[8][3] )
{
    //
    // calculate plane info for each face
    //
    for( int i = 0; i < 8; i++ )
    {
		Vector v[3];
        VectorCopy( m_pVerts[indices[i][0]].m_Vert, v[0] );
        VectorCopy( m_pVerts[indices[i][1]].m_Vert, v[1] );
        VectorCopy( m_pVerts[indices[i][2]].m_Vert, v[2] );

		Vector seg[2];
        VectorSubtract( v[1], v[0], seg[0] );
        VectorSubtract( v[2], v[0], seg[1] );

		Vector normal;
        CrossProduct( seg[1], seg[0], normal );
        VectorNormalize( normal );
        float dist = DotProduct( v[0], normal );

		m_Nodes[nodeIndex].SetTriPlane( i, normal, dist );
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcRayBoundingBoxes( int nodeIndex, int indices[8][3] )
{
	Vector triMin, triMax;

	for( int i = 0; i < 4; i++ )
	{
		triMin[0] = triMax[0] = m_pVerts[indices[(i*2)][0]].m_Vert[0];
		triMin[1] = triMax[1] = m_pVerts[indices[(i*2)][0]].m_Vert[1];
		triMin[2] = triMax[2] = m_pVerts[indices[(i*2)][0]].m_Vert[2];

		for( int j = 0; j < 3; j++ )
		{
			//
			// minimum
			//
			if( triMin[0] > m_pVerts[indices[(i*2)][j]].m_Vert[0] )
				triMin[0] = m_pVerts[indices[(i*2)][j]].m_Vert[0];
			if( triMin[0] > m_pVerts[indices[(i*2+1)][j]].m_Vert[0] )
				triMin[0] = m_pVerts[indices[(i*2+1)][j]].m_Vert[0];
			
			if( triMin[1] > m_pVerts[indices[(i*2)][j]].m_Vert[1] )
				triMin[1] = m_pVerts[indices[(i*2)][j]].m_Vert[1];
			if( triMin[1] > m_pVerts[indices[(i*2+1)][j]].m_Vert[1] )
				triMin[1] = m_pVerts[indices[(i*2+1)][j]].m_Vert[1];

			if( triMin[2] > m_pVerts[indices[(i*2)][j]].m_Vert[2] )
				triMin[2] = m_pVerts[indices[(i*2)][j]].m_Vert[2];
			if( triMin[2] > m_pVerts[indices[(i*2+1)][j]].m_Vert[2] )
				triMin[2] = m_pVerts[indices[(i*2+1)][j]].m_Vert[2];

			//
			// maximum
			//
			if( triMax[0] < m_pVerts[indices[(i*2)][j]].m_Vert[0] )
				triMax[0] = m_pVerts[indices[(i*2)][j]].m_Vert[0];
			if( triMax[0] < m_pVerts[indices[(i*2+1)][j]].m_Vert[0] )
				triMax[0] = m_pVerts[indices[(i*2+1)][j]].m_Vert[0];
			
			if( triMax[1] < m_pVerts[indices[(i*2)][j]].m_Vert[1] )
				triMax[1] = m_pVerts[indices[(i*2)][j]].m_Vert[1];
			if( triMax[1] < m_pVerts[indices[(i*2+1)][j]].m_Vert[1] )
				triMax[1] = m_pVerts[indices[(i*2+1)][j]].m_Vert[1];

			if( triMax[2] < m_pVerts[indices[(i*2)][j]].m_Vert[2] )
				triMax[2] = m_pVerts[indices[(i*2)][j]].m_Vert[2];
			if( triMax[2] < m_pVerts[indices[(i*2+1)][j]].m_Vert[2] )
				triMax[2] = m_pVerts[indices[(i*2+1)][j]].m_Vert[2];
		}

		m_Nodes[nodeIndex].SetRayBoundingBox( i, triMin, triMax );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcTriSurfBoundingBoxes( int nodeIndex, int indices[8][3] )
{
	Vector triMin, triMax;

    for( int i = 0; i < 8; i++ )
    {
		m_Nodes[nodeIndex].GetTriBoundingBox( i, triMin, triMax );

        for( int j = 0; j < 3; j++ )
        {
            //
            // minimum
            //
            if( triMin[0] > m_pVerts[indices[i][j]].m_Vert[0] )
                triMin[0] = m_pVerts[indices[i][j]].m_Vert[0];

            if( triMin[1] > m_pVerts[indices[i][j]].m_Vert[1] )
                triMin[1] = m_pVerts[indices[i][j]].m_Vert[1];

            if( triMin[2] > m_pVerts[indices[i][j]].m_Vert[2] )
                triMin[2] = m_pVerts[indices[i][j]].m_Vert[2];

            //
            // maximum
            //
            if( triMax[0] < m_pVerts[indices[i][j]].m_Vert[0] )
                triMax[0] = m_pVerts[indices[i][j]].m_Vert[0];

            if( triMax[1] < m_pVerts[indices[i][j]].m_Vert[1] )
                triMax[1] = m_pVerts[indices[i][j]].m_Vert[1];

            if( triMax[2] < m_pVerts[indices[i][j]].m_Vert[2] )
                triMax[2] = m_pVerts[indices[i][j]].m_Vert[2];
        }

		m_Nodes[nodeIndex].SetTriBoundingBox( i, triMin, triMax );
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcTriSurfIndices( int nodeIndex, int indices[8][3] )
{
    indices[0][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 4 );
    indices[0][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 0 );
    indices[0][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 3 );

    indices[1][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 3 );
    indices[1][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 0 );
    indices[1][2] = m_Nodes[nodeIndex].GetCenterVertIndex();

    indices[2][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 3 );
    indices[2][1] = m_Nodes[nodeIndex].GetCenterVertIndex();
    indices[2][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 5 );

    indices[3][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 5 );
    indices[3][1] = m_Nodes[nodeIndex].GetCenterVertIndex();
    indices[3][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 2 );

    indices[4][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 0 );
    indices[4][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 6 );
    indices[4][2] = m_Nodes[nodeIndex].GetCenterVertIndex();

    indices[5][0] = m_Nodes[nodeIndex].GetCenterVertIndex();
    indices[5][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 6 );
    indices[5][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 1 );

    indices[6][0] = m_Nodes[nodeIndex].GetCenterVertIndex();
    indices[6][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 1 );
    indices[6][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 2 );

    indices[7][0] = m_Nodes[nodeIndex].GetNeighborVertIndex( 2 );
    indices[7][1] = m_Nodes[nodeIndex].GetNeighborVertIndex( 1 );
    indices[7][2] = m_Nodes[nodeIndex].GetNeighborVertIndex( 7 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcTriSurfInfoAtNode( int nodeIndex )
{
    int indices[8][3];

	CalcTriSurfIndices( nodeIndex, indices );
	CalcTriSurfBoundingBoxes( nodeIndex, indices );
	CalcRayBoundingBoxes( nodeIndex, indices );
	CalcTriSurfPlanes( nodeIndex, indices );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcMinMaxBoundingBoxAtNode( int nodeIndex, Vector& bMin, Vector& bMax )
{
    // get the child node index
    int childNodeIndex = GetNodeChild( m_Power, nodeIndex, 4 );

    // get initial bounding box values
	m_Nodes[childNodeIndex].GetBoundingBox( bMin, bMax );

	Vector nodeMin, nodeMax;
    for( int i = 1, j = 5; i < 4; i++, j++ )
    {
		//
        // get the child node bounding box
		//
        childNodeIndex = GetNodeChild( m_Power, nodeIndex, j );
		m_Nodes[childNodeIndex].GetBoundingBox( nodeMin, nodeMax );

        // minimum
        if( bMin[0] > nodeMin[0] )
            bMin[0] = nodeMin[0];

        if( bMin[1] > nodeMin[1] )
            bMin[1] = nodeMin[1];

        if( bMin[2] > nodeMin[2] )
            bMin[2] = nodeMin[2];

        // maximum
        if( bMax[0] < nodeMax[0] )
            bMax[0] = nodeMax[0];

        if( bMax[1] < nodeMax[1] )
            bMax[1] = nodeMax[1];

        if( bMax[2] < nodeMax[2] )
            bMax[2] = nodeMax[2];
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcBoundingBoxAtNode( int nodeIndex )
{
    Vector bMin, bMax;

    //
    // initialize the minimum and maximum values for the bounding box
    //
    int level = GetNodeLevel( nodeIndex );

	int vertIndex = m_Nodes[nodeIndex].GetCenterVertIndex();
    if( level == m_Power )
    {
        VectorCopy( m_pVerts[vertIndex].m_Vert, bMin );
        VectorCopy( m_pVerts[vertIndex].m_Vert, bMax );
    }
    else
    {
		CalcMinMaxBoundingBoxAtNode( nodeIndex, bMin, bMax );

        if( bMin[0] > m_pVerts[vertIndex].m_Vert[0] )
            bMin[0] = m_pVerts[vertIndex].m_Vert[0];

        if( bMin[1] > m_pVerts[vertIndex].m_Vert[1] )
            bMin[1] = m_pVerts[vertIndex].m_Vert[1];

        if( bMin[2] > m_pVerts[vertIndex].m_Vert[2] )
            bMin[2] = m_pVerts[vertIndex].m_Vert[2];


        if( bMax[0] < m_pVerts[vertIndex].m_Vert[0] )
            bMax[0] = m_pVerts[vertIndex].m_Vert[0];

        if( bMax[1] < m_pVerts[vertIndex].m_Vert[1] )
            bMax[1] = m_pVerts[vertIndex].m_Vert[1];

        if( bMax[2] < m_pVerts[vertIndex].m_Vert[2] )
            bMax[2] = m_pVerts[vertIndex].m_Vert[2];
    }

    for( int i = 0; i < 8; i++ )
    {
		int neighborVertIndex = m_Nodes[nodeIndex].GetNeighborVertIndex( i );

        //
        // minimum
        //
        if( bMin[0] > m_pVerts[neighborVertIndex].m_Vert[0] )
            bMin[0] = m_pVerts[neighborVertIndex].m_Vert[0];

        if( bMin[1] > m_pVerts[neighborVertIndex].m_Vert[1] )
            bMin[1] = m_pVerts[neighborVertIndex].m_Vert[1];

        if( bMin[2] > m_pVerts[neighborVertIndex].m_Vert[2] )
            bMin[2] = m_pVerts[neighborVertIndex].m_Vert[2];

        //
        // maximum
        //
        if( bMax[0] < m_pVerts[neighborVertIndex].m_Vert[0] )
            bMax[0] = m_pVerts[neighborVertIndex].m_Vert[0];

        if( bMax[1] < m_pVerts[neighborVertIndex].m_Vert[1] )
            bMax[1] = m_pVerts[neighborVertIndex].m_Vert[1];

        if( bMax[2] < m_pVerts[neighborVertIndex].m_Vert[2] )
            bMax[2] = m_pVerts[neighborVertIndex].m_Vert[2];
    }

	m_Nodes[nodeIndex].SetBoundingBox( bMin, bMax );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CCoreDispInfo::GetMaxErrorFromChildren( int nodeIndex, int level )
{
	//
	// check for children nodes
	//
	if( level == m_Power )
		return 0.0f;

	//
	// get the child's error term and save the greatest error -- SW, SE, NW, NE
	//
	float errorTerm = 0.0f;
	for( int i = 4; i < 8; i++ )
	{
		int childIndex = GetNodeChild( m_Power, nodeIndex, i );

		float nodeErrorTerm = m_Nodes[childIndex].GetErrorTerm();
		if( errorTerm < nodeErrorTerm )
		{
			errorTerm = nodeErrorTerm;
		}
	}

	return errorTerm;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcErrorTermAtNode( int nodeIndex, int level )
{
    if( level == m_Power )
        return;

	//
	// get the vertex indices
	//
	int neighborVertIndices[9];
	for( int i = 0; i < 8; i++ )
	{
		neighborVertIndices[i] = m_Nodes[nodeIndex].GetNeighborVertIndex( i );
	}
	neighborVertIndices[8] = m_Nodes[nodeIndex].GetCenterVertIndex();


	//
	// calculate the error terms
	//
    Vector          segment;
    Vector          v;

    VectorAdd( m_pVerts[neighborVertIndices[5]].m_Vert, m_pVerts[neighborVertIndices[4]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[0]].m_Vert, v, segment );
    float errorTerm = ( float )VectorLength( segment );

    VectorAdd( m_pVerts[neighborVertIndices[5]].m_Vert, m_pVerts[neighborVertIndices[6]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[1]].m_Vert, v, segment );
    if( errorTerm < ( float )VectorLength( segment ) )
        errorTerm = ( float )VectorLength( segment );

    VectorAdd( m_pVerts[neighborVertIndices[6]].m_Vert, m_pVerts[neighborVertIndices[7]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[2]].m_Vert, v, segment );
    if( errorTerm < ( float )VectorLength( segment ) )
        errorTerm = ( float )VectorLength( segment );

    VectorAdd( m_pVerts[neighborVertIndices[7]].m_Vert, m_pVerts[neighborVertIndices[4]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[3]].m_Vert, v, segment );
    if( errorTerm < ( float )VectorLength( segment ) )
        errorTerm = ( float )VectorLength( segment );

    VectorAdd( m_pVerts[neighborVertIndices[4]].m_Vert, m_pVerts[neighborVertIndices[6]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[8]].m_Vert, v, segment );
    if( errorTerm < ( float )VectorLength( segment ) )
        errorTerm = ( float )VectorLength( segment );

    VectorAdd( m_pVerts[neighborVertIndices[5]].m_Vert, m_pVerts[neighborVertIndices[7]].m_Vert, v );
    VectorScale( v, 0.5f, v );
    VectorSubtract( m_pVerts[neighborVertIndices[8]].m_Vert, v, segment );
    if( errorTerm < ( float )VectorLength( segment ) )
        errorTerm = ( float )VectorLength( segment );

	//
	// add the max child's error term
	//
	errorTerm += GetMaxErrorFromChildren( nodeIndex, level );

	// set the error term
	m_Nodes[nodeIndex].SetErrorTerm( errorTerm );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcNeighborVertIndicesAtNode( int nodeIndex, int level )
{
	// calculate the shift in direction in the matrix
	int shift = ( 1 << ( m_Power - level ) );
	
	// calculate the width, height of the displacement surface (are uniform)
	int extent = ( ( 1 << m_Power ) + 1 );

	//
	// get the neighbor vertex indices (defining the surface at the node level)
	//
	for( int direction = 0; direction < 8; direction++ )
	{
		//
		// get the parent vertex index in component form
		//
		int posX = m_Nodes[nodeIndex].GetCenterVertIndex() % extent;
		int posY = m_Nodes[nodeIndex].GetCenterVertIndex() / extent;

		//
		// calculate the neighboring vertex indices for surface rendering
		//
		bool bError = false;
		switch( direction )
		{
		case WEST: { posX -= shift; break; }
		case NORTH: { posY += shift; break; }
		case EAST: { posX += shift; break; }
		case SOUTH: { posY -= shift; break; }
		case SOUTHWEST: { posX -= shift; posY -= shift; break; }
		case SOUTHEAST: { posX += shift; posY -= shift; break; }
		case NORTHWEST: { posX -= shift; posY += shift; break; }
		case NORTHEAST: { posX += shift; posY += shift; break; }
		default: { bError = true; break; }
		}
	
		if( bError )
		{
			m_Nodes[nodeIndex].SetNeighborVertIndex( direction, -99999 );
		}
		else
		{
			m_Nodes[nodeIndex].SetNeighborVertIndex( direction, ( ( posY * extent ) + posX ) );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcNodeInfo( int nodeIndex, int terminationLevel )
{
	// get the level of the current node
	int level = GetNodeLevel( nodeIndex );

	//
	// get the node data at the termination level
	//
    if( level == terminationLevel )
	{	
		// get the neighbor vertex indices (used to create surface at node level)
		CalcNeighborVertIndicesAtNode( nodeIndex, level );

		// get the neighbor node indices
		//CalcNeighborNodeIndicesAtNode( nodeIndex, level );

		// calculate the error term at the node
		CalcErrorTermAtNode( nodeIndex, level );

		// calcluate the axial-aligned bounding box at the node
		CalcBoundingBoxAtNode( nodeIndex );

		// calculate the triangular surface info at the node
		CalcTriSurfInfoAtNode( nodeIndex );

		return;
	}

	//
	// continue recursion (down to nodes "children")
	//
    for( int i = 4; i < 8; i++ )
    {
		int childIndex = GetNodeChild( m_Power, nodeIndex, i );
		CalcNodeInfo( childIndex, terminationLevel );
    }	
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CCoreDispInfo::GetNodeVertIndexFromParentIndex( int level, int parentVertIndex, int direction )
{
	// calculate the "shift"
	int shift = ( 1 << ( m_Power - ( level + 1 ) ) );

	// calculate the width and height of displacement (is uniform)
	int extent = ( ( 1 << m_Power ) + 1 );

	// get the parent vertex index in component form
	int posX = parentVertIndex % extent;
	int posY = parentVertIndex / extent;

    //
    // calculate the child index based on the parent index and child
    // direction
    //
    switch( direction )
    {
    case SOUTHWEST: { posX -= shift; posY -= shift; break; }
    case SOUTHEAST: { posX += shift; posY -= shift; break; }
    case NORTHWEST: { posX -= shift; posY += shift; break; }
    case NORTHEAST: { posX += shift; posY += shift; break; }
    default: return -99999;
    }

    // return the child vertex index
    return ( ( posY * extent ) + posX );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcVertIndicesAtNodes( int nodeIndex )
{
	//
	// check for recursion termination ( node level = power )
	//
	int level = GetNodeLevel( nodeIndex );
	if( level == m_Power )
		return;

	//
	// get the children indices - SW, SE, NW, NE
	//
	int childIndices[4];
	int i, j;
	for( i = 0, j = 4; i < 4; i++, j++ )
	{
		childIndices[i] = GetNodeChild( m_Power, nodeIndex, j );
		int centerIndex = GetNodeVertIndexFromParentIndex( level, m_Nodes[nodeIndex].GetCenterVertIndex(), j );
		m_Nodes[childIndices[i]].SetCenterVertIndex( centerIndex );
	}

	//
	// calculate the children's node vertex indices
	//
	for( i = 0; i < 4; i++ )
	{
		CalcVertIndicesAtNodes( childIndices[i] );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateLODTree( void )
{
	//
	// calculate the displacement surface's vertex index at each quad-tree node
	// centroid
	//
	int size = GetSize();
	int initialIndex = ( ( size - 1 ) >> 1 );
	m_Nodes[0].SetCenterVertIndex( initialIndex );
	CalcVertIndicesAtNodes( 0 );

    //
    // calculate the error terms, bounding boxes, and neighboring vertex indices
    // at each node
    //
    for( int i = m_Power; i > 0; i-- )
    {
		CalcNodeInfo( 0, i );
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcDispSurfCoords( bool bLightMap, int lightmapID )
{
	//
	// get base surface texture coords
	//
	Vector2D texCoords[4];
	Vector2D luxelCoords[4];
	CCoreDispSurface *pSurf = GetSurface();

	int i;
	for( i = 0; i < 4; i++ )
	{
		pSurf->GetTexCoord( i, texCoords[i] );
		pSurf->GetLuxelCoord( lightmapID, i, luxelCoords[i] );
	}

	//
	// get images width and intervals along the edge
	//
    int postSpacing = GetPostSpacing();
    float ooInt = ( 1.0f / ( float )( postSpacing - 1 ) );

    //
    // calculate the parallel edge intervals
    //
	Vector2D edgeInt[2];
    if( !bLightMap )
    {
        Vector2DSubtract( texCoords[1], texCoords[0], edgeInt[0] );
        Vector2DSubtract( texCoords[2], texCoords[3], edgeInt[1] );
    }
    else
    {
        Vector2DSubtract( luxelCoords[1], luxelCoords[0], edgeInt[0] );
        Vector2DSubtract( luxelCoords[2], luxelCoords[3], edgeInt[1] );
    }
    Vector2DMultiply( edgeInt[0], ooInt, edgeInt[0] );
    Vector2DMultiply( edgeInt[1], ooInt, edgeInt[1] );

    //
    // calculate the displacement points
    //    
	for( i = 0; i < postSpacing; i++ )
	{
        //
        // position along parallel edges (start and end for a perpendicular segment)
        //
		Vector2D endPts[2];
        Vector2DMultiply( edgeInt[0], ( float )i, endPts[0] );
        Vector2DMultiply( edgeInt[1], ( float )i, endPts[1] );
        if( !bLightMap )
        {
            Vector2DAdd( endPts[0], texCoords[0], endPts[0] );
            Vector2DAdd( endPts[1], texCoords[3], endPts[1] );
        }
        else
        {
            Vector2DAdd( endPts[0], luxelCoords[0], endPts[0] );
            Vector2DAdd( endPts[1], luxelCoords[3], endPts[1] );
        }
        
        //
        // interval length for perpendicular edge
        //
		Vector2D seg, segInt;
        Vector2DSubtract( endPts[1], endPts[0], seg );
        Vector2DMultiply( seg, ooInt, segInt );

        //
		// calculate the material (texture or light) coordinate at each point
        //
        for( int j = 0; j < postSpacing; j++ )
        {
            Vector2DMultiply( segInt, ( float )j, seg );

            if( !bLightMap )
            {
                Vector2DAdd( endPts[0], seg, m_pVerts[i*postSpacing+j].m_TexCoord );
            }
            else
            {
                Vector2DAdd( endPts[0], seg, m_pVerts[i*postSpacing+j].m_LuxelCoords[lightmapID] );
            }
        }
	}
}

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcDispSurfAlphas( void )
{
	//
	// get images width and intervals along the edge
	//
    int postSpacing = GetPostSpacing();
    float ooInt = ( 1.0f / ( float )( postSpacing - 1 ) );

    //
    // calculate the parallel edge intervals
    //
	float edgeInt[2];
	edgeInt[0] = m_Surf.m_Alpha[1] - m_Surf.m_Alpha[0];
	edgeInt[1] = m_Surf.m_Alpha[2] - m_Surf.m_Alpha[3];
	edgeInt[0] *= ooInt;
	edgeInt[1] *= ooInt;

    //
    // calculate the displacement points
    //    
	for( int i = 0; i < postSpacing; i++ )
	{
        //
        // position along parallel edges (start and end for a perpendicular segment)
        //
		float endValues[2];

		endValues[0] = edgeInt[0] * ( float )i;
		endValues[1] = edgeInt[1] * ( float )i;
		endValues[0] += m_Surf.m_Alpha[0];
		endValues[1] += m_Surf.m_Alpha[3];
		
        //
        // interval length for perpendicular edge
        //
		float seg, segInt;
		seg = endValues[1] - endValues[0];
		segInt = seg * ooInt;

        //
		// calculate the alpha value at each point
        //
		for( int j = 0; j < postSpacing; j++ )
		{
			seg = segInt * ( float )j;
			m_Alphas[i*postSpacing+j] = endValues[0] + seg;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateDispSurfTangentSpaces( void )
{
	//
	// get texture axes from base surface
	//
	CCoreDispSurface *pSurf = GetSurface();
	Vector sAxis, tAxis;
	pSurf->GetSAxis( sAxis );
	pSurf->GetTAxis( tAxis );

	//
	// calculate the tangent spaces
	//
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		//
		// create the axes - normals, tangents, and binormals
		//
		VectorCopy( tAxis, m_pVerts[i].m_TangentT );
		VectorNormalize( m_pVerts[i].m_TangentT );
		CrossProduct( m_pVerts[i].m_Normal, m_pVerts[i].m_TangentT, m_pVerts[i].m_TangentS );
		VectorNormalize( m_pVerts[i].m_TangentS );
		CrossProduct( m_pVerts[i].m_TangentS, m_pVerts[i].m_Normal, m_pVerts[i].m_TangentT );
		VectorNormalize( m_pVerts[i].m_TangentT );

		Vector tmpVect;
		Vector planeNormal;
		pSurf->GetNormal( planeNormal );
		CrossProduct( sAxis, tAxis, tmpVect );
		if( DotProduct( planeNormal, tmpVect ) > 0.0f )
		{
			VectorScale( m_pVerts[i].m_TangentS, -1.0f, m_pVerts[i].m_TangentS );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::CalcNormalFromEdges( int indexRow, int indexCol, bool bIsEdge[4],
										 Vector& normal )
{
	// get the post spacing (size/interval of displacement surface)
	int postSpacing = ( ( 1 << m_Power ) + 1 );

	// initialize the normal accumulator - counter
	Vector accumNormal;
	int	   normalCount = 0;

	VectorClear( accumNormal );

	Vector tmpVect[2];
	Vector tmpNormal;

	//
	// check quadrant I (posX, posY)
	//
	if( bIsEdge[1] && bIsEdge[2] )
	{
		// tri i
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+indexRow].m_Vert, m_pVerts[indexCol*postSpacing+indexRow].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[indexCol*postSpacing+(indexRow+1)].m_Vert, m_pVerts[indexCol*postSpacing+indexRow].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;

		// tri 2
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+indexRow].m_Vert, m_pVerts[indexCol*postSpacing+(indexRow+1)].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+(indexRow+1)].m_Vert, m_pVerts[indexCol*postSpacing+(indexRow+1)].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;
	}

	//
	// check quadrant II (negX, posY)
	//
	if( bIsEdge[0] && bIsEdge[1] )
	{
		// tri i
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+(indexRow-1)].m_Vert, m_pVerts[indexCol*postSpacing+(indexRow-1)].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[indexCol*postSpacing+indexRow].m_Vert, m_pVerts[indexCol*postSpacing+(indexRow-1)].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;

		// tri 2
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+(indexRow-1)].m_Vert, m_pVerts[indexCol*postSpacing+indexRow].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[(indexCol+1)*postSpacing+indexRow].m_Vert, m_pVerts[indexCol*postSpacing+indexRow].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;
	}

	//
	// check quadrant III (negX, negY)
	//
	if( bIsEdge[0] && bIsEdge[3] )
	{
		// tri i
		VectorSubtract( m_pVerts[indexCol*postSpacing+(indexRow-1)].m_Vert, m_pVerts[(indexCol-1)*postSpacing+(indexRow-1)].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[(indexCol-1)*postSpacing+indexRow].m_Vert, m_pVerts[(indexCol-1)*postSpacing+(indexRow-1)].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;

		// tri 2
		VectorSubtract( m_pVerts[indexCol*postSpacing+(indexRow-1)].m_Vert, m_pVerts[(indexCol-1)*postSpacing+indexRow].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[indexCol*postSpacing+indexRow].m_Vert, m_pVerts[(indexCol-1)*postSpacing+indexRow].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;
	}

	//
	// check quadrant IV (posX, negY)
	//
	if( bIsEdge[2] && bIsEdge[3] )
	{
		// tri i
		VectorSubtract( m_pVerts[indexCol*postSpacing+indexRow].m_Vert, m_pVerts[(indexCol-1)*postSpacing+indexRow].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[(indexCol-1)*postSpacing+(indexRow+1)].m_Vert, m_pVerts[(indexCol-1)*postSpacing+indexRow].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;

		// tri 2
		VectorSubtract( m_pVerts[indexCol*postSpacing+indexRow].m_Vert, m_pVerts[(indexCol-1)*postSpacing+(indexRow+1)].m_Vert, tmpVect[0] );
		VectorSubtract( m_pVerts[indexCol*postSpacing+(indexRow+1)].m_Vert, m_pVerts[(indexCol-1)*postSpacing+(indexRow+1)].m_Vert, tmpVect[1] );
		CrossProduct( tmpVect[1], tmpVect[0], tmpNormal );
		VectorNormalize( tmpNormal );
		VectorAdd( accumNormal, tmpNormal, accumNormal );
		normalCount++;
	}

	VectorScale( accumNormal, ( 1.0f / ( float )normalCount ), normal );
}


//-----------------------------------------------------------------------------
// Purpose: This function determines if edges exist in each of the directions
//          off of the given point (given in component form).  We know ahead of
//          time that there are only 4 possibilities.
//
//            1     "directions"
//          0 + 2
//            3
//
//   Input: indexRow - row position
//          indexCol - col position
//          direction - the direction (edge) currently being evaluated
//          postSpacing - the number of intervals in the row and col directions
//  Output: the edge existed? (true/false)
//-----------------------------------------------------------------------------
bool CCoreDispInfo::DoesEdgeExist( int indexRow, int indexCol, int direction, int postSpacing )
{
    switch( direction )
    {
    case 0:
        // left edge
        if( ( indexRow - 1 ) < 0 )
            return false;
        return true;
    case 1:
        // top edge
        if( ( indexCol + 1 ) > ( postSpacing - 1 ) )
            return false;
        return true;
    case 2:
        // right edge
        if( ( indexRow + 1 ) > ( postSpacing - 1 ) )
            return false;
        return true;
    case 3:
        // bottom edge
        if( ( indexCol - 1 ) < 0 )
            return false;
        return true;
    default:
        return false;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateDispSurfNormals( void )
{
	// get the post spacing (size/interval of displacement surface)
	int postSpacing = GetPostSpacing();

	//
	// generate the normals at each displacement surface vertex
	//
	for( int i = 0; i < postSpacing; i++ )
	{
		for( int j = 0; j < postSpacing; j++ )
		{
			bool bIsEdge[4];

			// edges
			for( int k = 0; k < 4; k++ )
			{
				bIsEdge[k] = DoesEdgeExist( j, i, k, postSpacing );
			}

			Vector normal;
			CalcNormalFromEdges( j, i, bIsEdge, normal );

			// save generated normal
			VectorCopy( normal, m_pVerts[i*postSpacing+j].m_Normal );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GenerateDispSurf( void )
{
	int i;
	CCoreDispSurface *pSurf = GetSurface();
	Vector points[4];
	for( i = 0; i < 4; i++ )
	{
		pSurf->GetPoint( i, points[i] );
	}

    //
    // get the spacing (interval = width/height, are equal because it is uniform) along the edge
    //
	int postSpacing = GetPostSpacing();
	float ooInt = 1.0f / ( float )( postSpacing - 1 );

	//
	// calculate the opposite edge intervals
	//
	Vector edgeInt[2];
    VectorSubtract( points[1], points[0], edgeInt[0] );
    VectorScale( edgeInt[0], ooInt, edgeInt[0] );
    VectorSubtract( points[2], points[3], edgeInt[1] );
    VectorScale( edgeInt[1], ooInt, edgeInt[1] );

	Vector elevNormal;
	elevNormal.Init();
	if( m_Elevation != 0.0f )
	{
		pSurf->GetNormal( elevNormal );
		VectorScale( elevNormal, m_Elevation, elevNormal );
	}

	//
	// calculate the displaced vertices
	//
	for( i = 0; i < postSpacing; i++ )
	{
		//
		// calculate segment interval between opposite edges
		//
		Vector endPts[2];
        VectorScale( edgeInt[0], ( float )i, endPts[0] );
        VectorAdd( endPts[0], points[0], endPts[0] );
        VectorScale( edgeInt[1], ( float )i, endPts[1] );
        VectorAdd( endPts[1], points[3], endPts[1] );

		Vector seg, segInt;
        VectorSubtract( endPts[1], endPts[0], seg );
        VectorScale( seg, ooInt, segInt );

		//
		// calculate the surface vertices
		//
		for( int j = 0; j < postSpacing; j++ )
		{	
			int ndx = i * postSpacing + j;

			CoreDispVert_t *pVert = &m_pVerts[ndx];

			// calculate the flat surface position -- saved separately
			pVert->m_FlatVert = endPts[0] + ( segInt * ( float )j );

			// start with the base surface position
			pVert->m_Vert = pVert->m_FlatVert;

			// add the elevation vector -- if it exists
			if( m_Elevation != 0.0f )
			{
				pVert->m_Vert += elevNormal;
			}

			// add the subdivision surface position
			pVert->m_Vert += pVert->m_SubdivPos;

			// add the displacement field direction(normalized) and distance
			pVert->m_Vert += pVert->m_FieldVector * pVert->m_FieldDistance;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//bool CCoreDispInfo::Create( int creationFlags )
bool CCoreDispInfo::Create( void )
{
	// sanity check
	CCoreDispSurface *pSurf = GetSurface();
	if( pSurf->GetPointCount() != 4 )
		return false;
	
	// generate the displacement surface
	GenerateDispSurf();
	
	GenerateDispSurfNormals();
	
	GenerateDispSurfTangentSpaces();
	
	CalcDispSurfCoords( false, 0 );
	
	for( int bumpID = 0; bumpID < ( NUM_BUMP_VECTS + 1 ); bumpID++ )
	{
		CalcDispSurfCoords( true, bumpID );
	}
	
	GenerateLODTree();
	
	GenerateCollisionData();
	
	CreateTris();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Create a displacement surface without generating the LOD for it.
//-----------------------------------------------------------------------------
bool CCoreDispInfo::CreateWithoutLOD( void )
{
	// sanity check
	CCoreDispSurface *pSurf = GetSurface();
	if( pSurf->GetPointCount() != 4 )
		return false;
	
	GenerateDispSurf();
	
	GenerateDispSurfNormals();
	
	GenerateDispSurfTangentSpaces();
	
	CalcDispSurfCoords( false, 0 );
	
	for( int bumpID = 0; bumpID < ( NUM_BUMP_VECTS + 1 ); bumpID++ )
	{
		CalcDispSurfCoords( true, bumpID );
	}
	GenerateCollisionData();

	CreateTris();
	
    return true;
}



//-----------------------------------------------------------------------------
// Purpose: This function calculates the neighbor node index given the base
//          node and direction of the neighbor node in the tree.
//   Input: power - the size in one dimension of the displacement map (2^power + 1 )
//          index - the "base" node index
//          direction - the direction of the neighbor { W = 1, N = 2, E = 3, S = 4 }
//  Output: returns the index of the neighbor node
//-----------------------------------------------------------------------------
int GetNodeNeighborNode( int power, int index, int direction, int level )
{
    // adjust the index to range [0...?]
	int minNodeIndex = GetNodeMinNodeAtLevel( level );

    // get node extent (uniform: height = width)
    int nodeExtent = ( 1 << ( level - 1 ) );

	//
	// get node's component positions in quad-tree
	//
	int posX, posY;
	GetComponentsFromNodeIndex( ( index - minNodeIndex ), &posX, &posY );

	//
	// find the neighbor in the "direction"
	//
    switch( direction )
    {
    case CCoreDispInfo::WEST:
        {
            if( ( posX - 1 ) < 0 )
            {
                return -( CCoreDispInfo::WEST + 1 );
            }
            else
            {
                return ( GetNodeIndexFromComponents( ( posX - 1 ), posY ) + minNodeIndex );
            }
        }
    case CCoreDispInfo::NORTH:
        {
            if( ( posY + 1 ) == nodeExtent )
            {
                return -( CCoreDispInfo::NORTH + 1 );
            }
            else
            {
                return ( GetNodeIndexFromComponents( posX, ( posY + 1 ) ) + minNodeIndex );
            }
        }
    case CCoreDispInfo::EAST:
        {
            if( ( posX + 1 ) == nodeExtent )
            {
                return -( CCoreDispInfo::EAST + 1 );
            }
            else
            {
                return ( GetNodeIndexFromComponents( ( posX + 1 ), posY ) + minNodeIndex );
            }
        }
    case CCoreDispInfo::SOUTH: 
        {
            if( ( posY - 1 ) < 0 )
            {
                return -( CCoreDispInfo::SOUTH + 1 );
            }
            else
            {
                return ( GetNodeIndexFromComponents( posX, ( posY - 1 ) ) + minNodeIndex );
            }
        }
    default:
        {
            return -99999;
        }
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int GetNodeNeighborNodeFromNeighborSurf( int power, int index, int direction, int level, int neighborOrient )
{
    // adjust the index to range [0...?]
	int minNodeIndex = GetNodeMinNodeAtLevel( level );

    // get node extent (uniform: height = width)
    int nodeExtent = ( 1 << ( level - 1 ) );

	//
	// get node's component positions in quad-tree
	//
	int posX, posY;
	GetComponentsFromNodeIndex( ( index - minNodeIndex ), &posX, &posY );

    switch( direction )
    {
    case CCoreDispInfo::WEST:
        {
            switch( neighborOrient )
            {
            case CCoreDispInfo::WEST: return -( ( GetNodeIndexFromComponents( posX, ( ( nodeExtent - 1 ) - posY ) ) ) + minNodeIndex );
            case CCoreDispInfo::NORTH: return -( ( GetNodeIndexFromComponents( ( nodeExtent - 1 ) - posY, ( nodeExtent - 1 ) ) ) + minNodeIndex );
            case CCoreDispInfo::EAST: return -( ( GetNodeIndexFromComponents( ( nodeExtent - 1 ), posY ) ) + minNodeIndex );
            case CCoreDispInfo::SOUTH: return -( ( GetNodeIndexFromComponents( posY, posX ) ) + minNodeIndex );
            default: return -99999;
            }
        }
    case CCoreDispInfo::NORTH:
        {
            switch( neighborOrient )
            {
            case CCoreDispInfo::WEST: return -( ( GetNodeIndexFromComponents( ( ( nodeExtent - 1 ) - posY ), ( ( nodeExtent - 1 ) - posX ) ) ) + minNodeIndex );
            case CCoreDispInfo::NORTH: return -( ( GetNodeIndexFromComponents( ( ( nodeExtent - 1 ) - posX ), posY ) ) + minNodeIndex );
            case CCoreDispInfo::EAST: return -( ( GetNodeIndexFromComponents( posY, posX ) ) + minNodeIndex );
            case CCoreDispInfo::SOUTH: return -( ( GetNodeIndexFromComponents( posX, ( ( nodeExtent - 1 ) - posY ) ) ) + minNodeIndex );
            default: return -99999;
            }
        }
    case CCoreDispInfo::EAST:
        {
            switch( neighborOrient )
            {
            case CCoreDispInfo::WEST: return -( ( GetNodeIndexFromComponents( ( ( nodeExtent - 1 ) - posX ), posY ) ) + minNodeIndex );
            case CCoreDispInfo::NORTH: return -( ( GetNodeIndexFromComponents( posY, posX ) ) + minNodeIndex );
            case CCoreDispInfo::EAST: return -( ( GetNodeIndexFromComponents( posX, ( ( nodeExtent - 1 ) - posY ) ) ) + minNodeIndex );
            case CCoreDispInfo::SOUTH: return -( ( GetNodeIndexFromComponents( ( ( nodeExtent - 1 ) - posY ), ( ( nodeExtent - 1 ) - posX ) ) ) + minNodeIndex );
            default: return -99999;
            }
        }
    case CCoreDispInfo::SOUTH:
        {
            switch( neighborOrient )
            {
            case CCoreDispInfo::WEST: return -( ( GetNodeIndexFromComponents( posY, posX ) ) + minNodeIndex );
            case CCoreDispInfo::NORTH: return -( ( GetNodeIndexFromComponents( posX, ( nodeExtent - 1 ) ) ) + minNodeIndex );
            case CCoreDispInfo::EAST: return -( ( GetNodeIndexFromComponents( ( nodeExtent - 1 ), ( ( nodeExtent - 1 ) - posX ) ) ) + minNodeIndex );
            case CCoreDispInfo::SOUTH: return -( ( GetNodeIndexFromComponents( ( ( nodeExtent - 1 ) - posX ), posY ) ) + minNodeIndex );
            default: return -99999;
            }
        }
    default:
        {
            return -99999;
        }
    }
}



// Turn the optimizer back on
#pragma optimize( "", on )

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GetPositionOnSurface( float u, float v, Vector &vPos,
										  Vector *pNormal, float *pAlpha )
{
	Vector2D dispUV( u, v );
	DispUVToSurf( dispUV, vPos, pNormal, pAlpha );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::BaseFacePlaneToDispUV( Vector const &planePt, Vector2D &dispUV )
{
	// Get the base surface points.
	CCoreDispSurface *pSurf = GetSurface();
	Vector vecPoints[4];
	for( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		pSurf->GetPoint( iPoint, vecPoints[iPoint] );
	}

	PointInQuadToBarycentric( vecPoints[0], vecPoints[3], vecPoints[2], vecPoints[1], planePt, dispUV );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriTLToBR_1( const Vector &vecIntersectPoint,   
											  int nSnapU, int nNextU, int nSnapV, int nNextV, 
											  Vector &vecPoint, Vector *pNormal, float *pAlpha,
											  bool bBackup )
{
	int nWidth = GetWidth();

	int nIndices[3];
	nIndices[0] = nNextV * nWidth + nSnapU;
	nIndices[1] = nNextV * nWidth + nNextU;	
	nIndices[2] = nSnapV * nWidth + nNextU;

	Vector vecFlatVerts[3], vecVerts[3];
	float flAlphas[3];
	for ( int iVert = 0; iVert < 3; ++iVert )
	{
		vecFlatVerts[iVert] = m_pVerts[nIndices[iVert]].m_FlatVert;
		vecVerts[iVert] = m_pVerts[nIndices[iVert]].m_Vert;
		flAlphas[iVert] = m_pVerts[nIndices[iVert]].m_Alpha;
	}

	if ( nSnapU == nNextU )
	{
		if ( nSnapV == nNextV )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[0] - vecVerts[1];
			Vector edgeV = vecVerts[2] - vecVerts[1];
			*pNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( *pNormal );
		}			
	}
	else if ( nSnapV == nNextV )
	{
		if ( nSnapU == nNextU )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[0] - vecVerts[1];
			Vector edgeV = vecVerts[2] - vecVerts[1];
			*pNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( *pNormal );
		}			
	}
	else
	{
		float flCfs[3];
		if ( CalcBarycentricCooefs( vecFlatVerts[0], vecFlatVerts[1], vecFlatVerts[2], vecIntersectPoint, flCfs[0], flCfs[1], flCfs[2] ) )
		{
			vecPoint = ( vecVerts[0] * flCfs[0] ) + ( vecVerts[1] * flCfs[1] ) + ( vecVerts[2] * flCfs[2] );

			if( pAlpha )
			{
				*pAlpha = ( flAlphas[0] * flCfs[0] ) + ( flAlphas[1] * flCfs[1] ) + ( flAlphas[2] * flCfs[2] );
			}
			
			if( pNormal )
			{
				Vector edgeU = vecVerts[0] - vecVerts[1];
				Vector edgeV = vecVerts[2] - vecVerts[1];
				*pNormal = CrossProduct( edgeU, edgeV );
				VectorNormalize( *pNormal );
			}			
		}
		else
		{
			if ( !bBackup )
			{
				DispUVToSurf_TriTLToBR_2( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriTLToBR_2( const Vector &vecIntersectPoint,   
											  int nSnapU, int nNextU, int nSnapV, int nNextV, 
											  Vector &vecPoint, Vector *pNormal, float *pAlpha,
											  bool bBackup )
{
	int nWidth = GetWidth();

	int nIndices[3];
	nIndices[0] = nSnapV * nWidth + nSnapU;
	nIndices[1] = nNextV * nWidth + nSnapU;
	nIndices[2] = nSnapV * nWidth + nNextU;

	Vector vecFlatVerts[3], vecVerts[3];
	float flAlphas[3];
	for ( int iVert = 0; iVert < 3; ++iVert )
	{
		vecFlatVerts[iVert] = m_pVerts[nIndices[iVert]].m_FlatVert;
		vecVerts[iVert] = m_pVerts[nIndices[iVert]].m_Vert;
		flAlphas[iVert] = m_pVerts[nIndices[iVert]].m_Alpha;
	}

	if ( nSnapU == nNextU )
	{
		if ( nSnapV == nNextV )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[1] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[1] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[1] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[2] - vecVerts[0];
			Vector edgeV = vecVerts[1] - vecVerts[0];
			*pNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( *pNormal );
		}			
	}
	else if ( nSnapV == nNextV )
	{
		if ( nSnapU == nNextU )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[2] - vecVerts[0];
			Vector edgeV = vecVerts[1] - vecVerts[0];
			*pNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( *pNormal );
		}			
	}
	else
	{
		float flCfs[3];
		if ( CalcBarycentricCooefs( vecFlatVerts[0], vecFlatVerts[1], vecFlatVerts[2], vecIntersectPoint, flCfs[0], flCfs[1], flCfs[2] ) )
		{
			vecPoint = ( vecVerts[0] * flCfs[0] ) + ( vecVerts[1] * flCfs[1] ) + ( vecVerts[2] * flCfs[2] );

			if( pAlpha )
			{
				*pAlpha = ( flAlphas[0] * flCfs[0] ) + ( flAlphas[1] * flCfs[1] ) + ( flAlphas[2] * flCfs[2] );
			}
	
			if( pNormal )
			{
				Vector edgeU = vecVerts[2] - vecVerts[0];
				Vector edgeV = vecVerts[1] - vecVerts[0];
				*pNormal = CrossProduct( edgeU, edgeV );
				VectorNormalize( *pNormal );
			}			
		}
		else
		{
			if ( !bBackup )
			{
				DispUVToSurf_TriTLToBR_1( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriTLToBR( Vector &vecPoint, Vector *pNormal, float *pAlpha, 
											float flU, float flV, const Vector &vecIntersectPoint )
{
	const float TRIEDGE_EPSILON = 0.00001f;

	int nWidth = GetWidth();
	int nHeight = GetHeight();

	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if ( ( flFracU + flFracV ) >= ( 1.0f + TRIEDGE_EPSILON ) )
	{
		DispUVToSurf_TriTLToBR_1( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, false );
	}
	else
	{
		DispUVToSurf_TriTLToBR_2( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriBLToTR_1( const Vector &vecIntersectPoint,   
											  int nSnapU, int nNextU, int nSnapV, int nNextV, 
											  Vector &vecPoint, Vector *pNormal, float *pAlpha,
											  bool bBackup )
{
	int nWidth = GetWidth();

	int nIndices[3];
	nIndices[0] = nSnapV * nWidth + nSnapU;
	nIndices[1] = nNextV * nWidth + nSnapU;
	nIndices[2] = nNextV * nWidth + nNextU;

	Vector vecFlatVerts[3], vecVerts[3];
	float flAlphas[3];
	for ( int iVert = 0; iVert < 3; ++iVert )
	{
		vecFlatVerts[iVert] = m_pVerts[nIndices[iVert]].m_FlatVert;
		vecVerts[iVert] = m_pVerts[nIndices[iVert]].m_Vert;
		flAlphas[iVert] = m_pVerts[nIndices[iVert]].m_Alpha;
	}

	if ( nSnapU == nNextU )
	{
		if ( nSnapV == nNextV )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[2] - vecVerts[1];
			Vector edgeV = vecVerts[0] - vecVerts[1];
			*pNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( *pNormal );
		}			
	}
	else if ( nSnapV == nNextV )
	{
		if ( nSnapU == nNextU )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[2] - vecVerts[1];
			Vector edgeV = vecVerts[0] - vecVerts[1];
			*pNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( *pNormal );
		}			
	}
	else
	{
		float flCfs[3];
		if ( CalcBarycentricCooefs( vecFlatVerts[0], vecFlatVerts[1], vecFlatVerts[2], vecIntersectPoint, flCfs[0], flCfs[1], flCfs[2] ) )
		{
			vecPoint = ( vecVerts[0] * flCfs[0] ) + ( vecVerts[1] * flCfs[1] ) + ( vecVerts[2] * flCfs[2] );
			
			if( pAlpha )
			{
				*pAlpha = ( flAlphas[0] * flCfs[0] ) + ( flAlphas[1] * flCfs[1] ) + ( flAlphas[2] * flCfs[2] );
			}

			if( pNormal )
			{
				Vector edgeU = vecVerts[2] - vecVerts[1];
				Vector edgeV = vecVerts[0] - vecVerts[1];
				*pNormal = CrossProduct( edgeV, edgeU );
				VectorNormalize( *pNormal );
			}
		}
		else
		{
			if ( !bBackup )
			{
				DispUVToSurf_TriBLToTR_2( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriBLToTR_2( const Vector &vecIntersectPoint,   
											  int nSnapU, int nNextU, int nSnapV, int nNextV, 
											  Vector &vecPoint, Vector *pNormal, float *pAlpha,
											  bool bBackup )
{
	int nWidth = GetWidth();

	int nIndices[3];
	nIndices[0] = nSnapV * nWidth + nSnapU;
	nIndices[1] = nNextV * nWidth + nNextU;
	nIndices[2] = nSnapV * nWidth + nNextU;

	Vector vecFlatVerts[3], vecVerts[3];
	float flAlphas[3];
	for ( int iVert = 0; iVert < 3; ++iVert )
	{
		vecFlatVerts[iVert] = m_pVerts[nIndices[iVert]].m_FlatVert;
		vecVerts[iVert] = m_pVerts[nIndices[iVert]].m_Vert;
		flAlphas[iVert] = m_pVerts[nIndices[iVert]].m_Alpha;
	}

	if ( nSnapU == nNextU )
	{
		if ( nSnapV == nNextV )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[1] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[1] - vecVerts[0] ) );
			
			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[1] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[0] - vecVerts[2];
			Vector edgeV = vecVerts[1] - vecVerts[2];
			*pNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( *pNormal );
		}			
	}
	else if ( nSnapV == nNextV )
	{
		if ( nSnapU == nNextU )
		{
			vecPoint = vecVerts[0];
			*pAlpha = flAlphas[0];
		}
		else
		{
			float flFrac = ( vecIntersectPoint - vecFlatVerts[0] ).Length() / ( vecFlatVerts[2] - vecFlatVerts[0] ).Length();
			vecPoint = vecVerts[0] + ( flFrac * ( vecVerts[2] - vecVerts[0] ) );

			if ( pAlpha )
			{
				*pAlpha = flAlphas[0] + ( flFrac * ( flAlphas[2] - flAlphas[0] ) );
			}
		}

		if( pNormal )
		{
			Vector edgeU = vecVerts[0] - vecVerts[2];
			Vector edgeV = vecVerts[1] - vecVerts[2];
			*pNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( *pNormal );
		}			
	}
	else
	{
		float flCfs[3];
		if ( CalcBarycentricCooefs( vecFlatVerts[0], vecFlatVerts[1], vecFlatVerts[2], vecIntersectPoint, flCfs[0], flCfs[1], flCfs[2] ) )
		{
			vecPoint = ( vecVerts[0] * flCfs[0] ) + ( vecVerts[1] * flCfs[1] ) + ( vecVerts[2] * flCfs[2] );
			
			if( pAlpha )
			{
				*pAlpha = ( flAlphas[0] * flCfs[0] ) + ( flAlphas[1] * flCfs[1] ) + ( flAlphas[2] * flCfs[2] );
			}

			if( pNormal )
			{
				Vector edgeU = vecVerts[0] - vecVerts[2];
				Vector edgeV = vecVerts[1] - vecVerts[2];
				*pNormal = CrossProduct( edgeV, edgeU );
				VectorNormalize( *pNormal );
			}			
		}
		else
		{
			if ( !bBackup )
			{
				DispUVToSurf_TriBLToTR_1( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf_TriBLToTR( Vector &vecPoint, Vector *pNormal, float *pAlpha, 
											float flU, float flV, const Vector &vecIntersectPoint )
{
	int nWidth = GetWidth();
	int nHeight = GetHeight();

	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( flFracU < flFracV )
	{
		DispUVToSurf_TriBLToTR_1( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, false );
	}
	else
	{
		DispUVToSurf_TriBLToTR_2( vecIntersectPoint, nSnapU, nNextU, nSnapV, nNextV, vecPoint, pNormal, pAlpha, false );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::DispUVToSurf( Vector2D const &dispUV, Vector &vecPoint,
								  Vector *pNormal, float *pAlpha )
{
	// Check to see that the point is on the surface.
	if ( dispUV.x < 0.0f || dispUV.x > 1.0f || dispUV.y < 0.0f || dispUV.y > 1.0f )
		return;

	// Get the base surface points.
	Vector vecIntersectPoint;
	CCoreDispSurface *pSurf = GetSurface();
	PointInQuadFromBarycentric( pSurf->GetPoint( 0 ), pSurf->GetPoint( 3 ), pSurf->GetPoint( 2 ), pSurf->GetPoint( 1 ), dispUV, vecIntersectPoint );

	// Get the displacement power.
	int nWidth = GetWidth();
	int nHeight = GetHeight();
 
	// Scale the U, V coordinates to the displacement grid size.
	float flU = dispUV.x * ( static_cast<float>( nWidth ) - 1.000001f );
	float flV = dispUV.y * ( static_cast<float>( nHeight ) - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	// Use this to get the triangle orientation.
	bool bOdd = ( ( ( nSnapV * nWidth ) + nSnapU ) % 2 == 1 );

	// Top Left to Bottom Right
	if( bOdd )
	{
		DispUVToSurf_TriTLToBR( vecPoint, pNormal, pAlpha, flU, flV, vecIntersectPoint );
	}
	// Bottom Left to Top Right
	else
	{
		DispUVToSurf_TriBLToTR( vecPoint, pNormal, pAlpha, flU, flV, vecIntersectPoint );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create bounding boxes around pairs of triangles (in a grid-like) 
//          fashion; used for culling
//-----------------------------------------------------------------------------
void CCoreDispInfo::CreateBoundingBoxes( CoreDispBBox_t *pBBox, int count )
{
	//
	// Initialize the bounding boxes.
	//
	int iBox;
	for( iBox = 0; iBox < count; ++iBox )
	{
		pBBox[iBox].vMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		pBBox[iBox].vMax.Init( FLT_MIN, FLT_MIN, FLT_MIN );
	}

	// Get the width and height of the displacement surface.
	int nHeight = GetHeight();
	int nWidth = GetWidth();

	// Find bounding box of every two consecutive triangles
	iBox = 0;
	int nIndex = 0;
	for( int iHgt = 0; iHgt < ( nHeight - 1 ); ++iHgt )
	{
		for( int iWid = 0; iWid < ( nWidth - 1 ); ++iWid )
		{
			for( int iPoint = 0; iPoint < 4; ++iPoint )
			{
				switch( iPoint )
				{
				case 0: { nIndex = ( nHeight * iHgt ) + iWid; break; }
				case 1: { nIndex = ( nHeight * ( iHgt + 1 ) ) + iWid; break; }
				case 2: { nIndex = ( nHeight * ( iHgt + 1 ) ) + ( iWid + 1 ); break; }
				case 3: { nIndex = ( nHeight * iHgt ) + ( iWid + 1 ); break; }
				default: { break; }
				}

				Vector vecPoint;
				GetVert( nIndex, vecPoint );
				if( vecPoint[0] < pBBox[iBox].vMin[0] ) { pBBox[iBox].vMin[0] = vecPoint[0]; }
				if( vecPoint[1] < pBBox[iBox].vMin[1] ) { pBBox[iBox].vMin[1] = vecPoint[1]; }
				if( vecPoint[2] < pBBox[iBox].vMin[2] ) { pBBox[iBox].vMin[2] = vecPoint[2]; }
				
				if( vecPoint[0] > pBBox[iBox].vMax[0] ) { pBBox[iBox].vMax[0] = vecPoint[0]; }
				if( vecPoint[1] > pBBox[iBox].vMax[1] ) { pBBox[iBox].vMax[1] = vecPoint[1]; }
				if( vecPoint[2] > pBBox[iBox].vMax[2] ) { pBBox[iBox].vMax[2] = vecPoint[2]; }
			}

			iBox++;
		}
	}

	// Verify.
	Assert( iBox == count );

	// Bloat.
	for ( iBox = 0; iBox < count; ++iBox )
	{
		for( int iAxis = 0; iAxis < 3; ++iAxis )
		{
			pBBox[iBox].vMin[iAxis] -= 1.0f;
			pBBox[iBox].vMax[iAxis] += 1.0f;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool PointInDispBBox( CoreDispBBox_t *pBox, const Vector &vecPoint )
{
	// Check to see if point lies in box
	if( ( vecPoint.x < pBox->vMin.x ) || ( vecPoint.x > pBox->vMax.x ) )
		return false;
	
	if( ( vecPoint.y < pBox->vMin.y ) || ( vecPoint.y > pBox->vMax.y ) )
		return false;
	
	if( ( vecPoint.z < pBox->vMin.z ) || ( vecPoint.z > pBox->vMax.z ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCoreDispInfo::GetTriangleIndicesForDispBBox( int nIndex, int nTris[2][3] )
{
	// Test to see whether or not the index is odd.
	bool bOdd = ( ( nIndex % 2 ) == 1 );

	int nWidth = GetWidth();

	// Tris for TLtoBR
	if ( bOdd )
	{
		nTris[0][0] = nIndex;
		nTris[0][1] = nIndex + nWidth;
		nTris[0][2] = nIndex + 1;
	
		nTris[1][0] = nIndex + 1;
		nTris[1][1] = nIndex + nWidth;
		nTris[1][2] = nIndex + nWidth + 1;
	}
	// Tris for BLtoTR
	else
	{
		nTris[0][0] = nIndex;
		nTris[0][1] = nIndex + nWidth;
		nTris[0][2] = nIndex + nWidth + 1;
	
		nTris[1][0] = nIndex;
		nTris[1][1] = nIndex + nWidth + 1;
		nTris[1][2] = nIndex + 1;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CCoreDispInfo::SurfToBaseFacePlane( Vector const &surfPt, Vector &planePt )
{
	// Create bounding boxes
	int nBoxCount = ( GetHeight() - 1 ) * ( GetWidth() - 1 );
	CoreDispBBox_t *pBBox = new CoreDispBBox_t[nBoxCount];
	CreateBoundingBoxes( pBBox, nBoxCount );

	// Use the boxes as a first-pass culling mechanism.
	for( int iBox = 0; iBox < nBoxCount; ++iBox )
	{
		// Get the current displacement triangle-pair bounding-box.
		CoreDispBBox_t *pBox = &pBBox[iBox];
		if( !pBox )
			continue;

		// Check the point against the current displacement bounding-box.
		if ( !PointInDispBBox( pBox, surfPt ) )
			continue;

		// Point lies within the bounding box.
		int nIndex = iBox + ( iBox / ( GetWidth() - 1 ) );

		// Get the triangle coordinates for this box.
		int aTris[2][3];
		GetTriangleIndicesForDispBBox( nIndex, aTris );
		
		// Barycentrically test the triangles on the displacement surface.
		Vector vecPoints[3];
		for ( int iTri = 0; iTri < 2; ++iTri )
		{
			for ( int iVert = 0; iVert < 3; ++iVert )
			{
				GetVert( aTris[iTri][iVert], vecPoints[iVert] );
			}

			float c[3];
			if ( CalcBarycentricCooefs( vecPoints[0], vecPoints[1], vecPoints[2], surfPt, c[0], c[1], c[2] ) )
			{
				Vector vecFlatPoints[3];
				for ( int iVert = 0; iVert < 3; ++iVert )
				{
					GetFlatVert( aTris[iTri][iVert], vecFlatPoints[iVert] );
				}

				planePt = ( vecFlatPoints[0] * c[0] ) + ( vecFlatPoints[1] * c[1] ) + ( vecFlatPoints[2] * c[2] );

				// Delete temporary memory.
				delete [] pBBox;
				return true;
			}
		}
	}

	// Delete temporary memory
	delete [] pBBox;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCoreDispInfo::GetTriCount( void )
{
	return ( ( GetHeight() - 1 ) * ( GetWidth() -1 ) * 2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::GetTriIndices( int iTri, unsigned short &v1, unsigned short &v2, unsigned short &v3 )
{
	// Verify we have the correct data (only build when collision data is built).
	if ( !m_pTris || ( iTri < 0 ) || ( iTri >= GetTriCount() ) )
	{
		Assert( iTri >= 0 );
		Assert( iTri < GetTriCount() );
		Assert( m_pTris );
		return;
	}

	CoreDispTri_t *pTri = &m_pTris[iTri];
	v1 = pTri->m_iIndex[0];
	v2 = pTri->m_iIndex[1];
	v3 = pTri->m_iIndex[2];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::SetTriIndices( int iTri, unsigned short v1, unsigned short v2, unsigned short v3 )
{
	// Verify we have the correct data (only build when collision data is built).
	if ( !m_pTris || ( iTri < 0 ) || ( iTri >= GetTriCount() ) )
	{
		Assert( iTri >= 0 );
		Assert( iTri < GetTriCount() );
		Assert( m_pTris );
		return;
	}

	CoreDispTri_t *pTri = &m_pTris[iTri];
	pTri->m_iIndex[0] = v1;
	pTri->m_iIndex[1] = v2;
	pTri->m_iIndex[2] = v3;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::GetTriPos( int iTri, Vector &v1, Vector &v2, Vector &v3 )
{
	// Verify we have the correct data (only build when collision data is built).
	if ( !m_pTris || ( iTri < 0 ) || ( iTri >= GetTriCount() ) )
	{
		Assert( iTri >= 0 );
		Assert( iTri < GetTriCount() );
		Assert( m_pTris );
		return;
	}

	CoreDispTri_t *pTri = &m_pTris[iTri];
	v1 = m_pVerts[pTri->m_iIndex[0]].m_Vert;
	v2 = m_pVerts[pTri->m_iIndex[1]].m_Vert;
	v3 = m_pVerts[pTri->m_iIndex[2]].m_Vert;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::InitTris( void )
{
	// Verify we have the correct data (only build when collision data is built).
	if ( !m_pTris )
	{
		Assert( m_pTris );
		return;
	}

	int nTriCount = GetTriCount();
	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		m_pTris[iTri].m_uiTags = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::CreateTris( void )
{
	// Verify we have the correct data (only build when collision data is built).
	if ( !m_pTris )
	{
		Assert( m_pTris );
		return;
	}

	// Extra sanity check if wanted!
	Assert( GetTriCount() == ( m_RenderIndexCount / 3 ) );

	int nTriCount = GetTriCount();
	for ( int iTri = 0, iRender = 0; iTri < nTriCount; ++iTri, iRender += 3 )
	{
		m_pTris[iTri].m_iIndex[0] = m_RenderIndices[iRender];
		m_pTris[iTri].m_iIndex[1] = m_RenderIndices[iRender+1];
		m_pTris[iTri].m_iIndex[2] = m_RenderIndices[iRender+2];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCoreDispInfo::IsTriWalkable( int iTri )
{ 
	if ( IsTriTag( iTri, COREDISPTRI_TAG_FORCE_WALKABLE_BIT ) )
	{
		return IsTriTag( iTri, COREDISPTRI_TAG_FORCE_WALKABLE_VAL );
	}

	return IsTriTag( iTri, COREDISPTRI_TAG_WALKABLE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCoreDispInfo::IsTriBuildable( int iTri )						
{ 
	if ( IsTriTag( iTri, COREDISPTRI_TAG_FORCE_BUILDABLE_BIT ) )
	{
		return IsTriTag( iTri, COREDISPTRI_TAG_FORCE_BUILDABLE_VAL );
	}

	return IsTriTag( iTri, COREDISPTRI_TAG_BUILDABLE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCoreDispInfo::IsTriRemove( int iTri )						
{ 
	return IsTriTag( iTri, COREDISPTRI_TAG_FORCE_REMOVE_BIT );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCoreDispInfo::Position_Update( int iVert, Vector vecPos )
{
	Vector vSPos, vFlat;
	GetFlatVert( iVert, vFlat );
	GetSubdivPosition( iVert, vSPos );
				
	Vector vSeg;
	vSeg = vecPos - vFlat;
	vSeg -= vSPos;
				
	// Subtract out the elevation.
	float elev = GetElevation();
	if( elev != 0.0 )
	{
		Vector vNormal;
		GetSurface()->GetNormal( vNormal );
		vNormal *= elev;
		
		vSeg -= vNormal;
	}
				
	float flDistance = VectorNormalize( vSeg );
	
	SetFieldVector( iVert, vSeg );
	SetFieldDistance( iVert, flDistance );
}
