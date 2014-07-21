//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "disp_powerinfo.h"
#include "disp_common.h"
#include "commonmacros.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------ //
// Internal classes.
// ------------------------------------------------------------------------ //

// These point at the vertices connecting to each of the [north,south,east,west] vertices.
class CVertCorners
{
public:
	short	m_Corner1[2];
	short	m_Corner2[2];
};


// ------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------ //

// This points at vertices to the side of a node (north, south, east, west).
static short g_SideVertMul[4][2] = { {1,0}, {0,1}, {-1,0}, {0,-1} };

static CVertCorners g_SideVertCorners[4] =
{
	{ {1,-1},  {1,1} },
	{ {1,1},   {-1,1} },
	{ {-1,1},  {-1,-1} },
	{ {-1,-1}, {1,-1} }
};

// This is used in loops on child nodes. The indices point at the nodes:
// 0 = upper-right
// 1 = upper-left
// 2 = lower-left
// 3 = lower-right
static CVertIndex g_ChildNodeIndexMul[4] = 
{ 
	CVertIndex(1,1), 
	CVertIndex(-1,1), 
	CVertIndex(-1,-1), 
	CVertIndex(1,-1)
};

// These are multipliers on vertMul (not nodeMul).
static CVertIndex g_ChildNodeDependencies[4][2] =
{
	{ CVertIndex(1,0),  CVertIndex(0,1) },
	{ CVertIndex(0,1),  CVertIndex(-1,0) },
	{ CVertIndex(-1,0), CVertIndex(0,-1) },
	{ CVertIndex(0,-1), CVertIndex(1,0) }
};

// 2x2 rotation matrices for each orientation.
static int g_OrientationRotations[4][2][2] =
{
	{{1, 0},		// CCW_0
	{0, 1}},

	{{0, 1},		// CCW_90
	{-1,0}},

	{{-1,0},		// CCW_180
	{0,-1}},

	{{0, -1},		// CCW_270
	{1, 0}}
};	   


// ------------------------------------------------------------------------ //
// Helper functions.
// ------------------------------------------------------------------------ //

// Apply a 2D rotation to the specified CVertIndex around the specified centerpoint.
static CVertIndex Transform2D(
	int const mat[2][2],
	CVertIndex const &vert,
	CVertIndex const &centerPoint )
{
	CVertIndex translated = vert - centerPoint;
	
	CVertIndex transformed(
		translated.x*mat[0][0] + translated.y*mat[0][1],
		translated.x*mat[1][0] + translated.y*mat[1][1] );
	
	return transformed + centerPoint;
}


// Rotate a given CVertIndex with a specified orientation.
// Do this with a lookup table eventually!
static void GetEdgeVertIndex( int sideLength, int iEdge, int iVert, CVertIndex &out )
{
	if( iEdge == NEIGHBOREDGE_RIGHT )
	{
		out.x = sideLength - 1;
		out.y = iVert;
	}
	else if( iEdge == NEIGHBOREDGE_TOP )
	{
		out.x = iVert;
		out.y = sideLength - 1;
	}
	else if( iEdge == NEIGHBOREDGE_LEFT )
	{
		out.x = 0;
		out.y = iVert;
	}
	else
	{
		out.x = iVert;
		out.y = 0;
	}
}


// Generate an index given a CVertIndex and the size of the displacement it resides in.
static int VertIndex( CVertIndex const &vert, int iMaxPower )
{
	return vert.y * ((1 << iMaxPower) + 1) + vert.x;
}
 

static CVertIndex WrapVertIndex( CVertIndex const &in, int sideLength )
{
	int out[2];

	for( int i=0; i < 2; i++ )
	{
		if( in[i] < 0 )
			out[i] = sideLength - 1 - (-in[i] % sideLength);
		else if( in[i] >= sideLength )
			out[i] = in[i] % sideLength;
		else
			out[i] = in[i];
	}

	return CVertIndex( out[0], out[1] );
}


static int GetFreeDependency( CVertDependency *pDep, int nElements )
{
	for( int i=0; i < nElements; i++ )
	{
		if( !pDep[i].IsValid() ) 
			return i;
	}

	Assert( false );
	return 0;
}


static void AddDependency( 
	CVertInfo *dependencies, 
	int sideLength,
	CVertIndex const &nodeIndex,
	CVertIndex const &dependency,
	int iMaxPower,
	bool bCheckNeighborDependency,
	bool bAddReverseDependency )
{
	int iNodeIndex = VertIndex( nodeIndex, iMaxPower );
	CVertInfo *pNode = &dependencies[iNodeIndex];

	int iDep = GetFreeDependency( pNode->m_Dependencies, sizeof(pNode->m_Dependencies)/sizeof(pNode->m_Dependencies[0]) );
	pNode->m_Dependencies[iDep].m_iVert = dependency;
	pNode->m_Dependencies[iDep].m_iNeighbor = -1;

	if( bAddReverseDependency )
	{
		CVertInfo *pDep = &dependencies[VertIndex( dependency, iMaxPower )];
		iDep = GetFreeDependency( pDep->m_ReverseDependencies, CVertInfo::NUM_REVERSE_DEPENDENCIES );
		pDep->m_ReverseDependencies[iDep].m_iVert = nodeIndex;
		pDep->m_ReverseDependencies[iDep].m_iNeighbor = -1;
	}

	// Edge verts automatically add a dependency for the neighbor.
	// Internal verts wind up in here twice anyway so it doesn't need to 
	if( bCheckNeighborDependency )
	{
		int iConnection = GetEdgeIndexFromPoint( nodeIndex, iMaxPower );
		if( iConnection != -1 )
		{
			Assert( !pNode->m_Dependencies[1].IsValid() );

			CVertIndex delta( nodeIndex.x - dependency.x, nodeIndex.y - dependency.y );
			CVertIndex newIndex( nodeIndex.x + delta.x, nodeIndex.y + delta.y );
			
			int fullSideLength = (1 << iMaxPower) + 1;
			pNode->m_Dependencies[1].m_iVert = WrapVertIndex( CVertIndex( newIndex.x, newIndex.y ), fullSideLength );
			pNode->m_Dependencies[1].m_iNeighbor = iConnection;
		}
	}
}


// --------------------------------------------------------------------------------- //
// CTesselateWinding stuff.
// --------------------------------------------------------------------------------- //

CTesselateVert::CTesselateVert( CVertIndex const &index, int iNode )
	: m_Index( index )
{
	m_iNode = iNode;
}


CVertInfo::CVertInfo()
{
	int i;
	for( i=0; i < sizeof(m_Dependencies)/sizeof(m_Dependencies[0]); i++ )
	{
		m_Dependencies[i].m_iVert = CVertIndex( -1, -1 );
		m_Dependencies[i].m_iNeighbor = -1;
	}

	for( i=0; i < sizeof(m_ReverseDependencies)/sizeof(m_ReverseDependencies[0]); i++ )
	{
		m_ReverseDependencies[i].m_iVert = CVertIndex( -1, -1 );
		m_ReverseDependencies[i].m_iNeighbor = -1;
	}
	
	m_iParent.x = m_iParent.y = -1;
	m_iNodeLevel = -1;
}


CTesselateVert g_TesselateVerts[] =
{
	CTesselateVert( CVertIndex(1,-1),  CHILDNODE_LOWER_RIGHT),
	CTesselateVert( CVertIndex(0,-1),  -1),
	CTesselateVert( CVertIndex(-1,-1), CHILDNODE_LOWER_LEFT),
	CTesselateVert( CVertIndex(-1, 0), -1),
	CTesselateVert( CVertIndex(-1, 1), CHILDNODE_UPPER_LEFT),
	CTesselateVert( CVertIndex(0, 1),  -1),
	CTesselateVert( CVertIndex(1, 1),  CHILDNODE_UPPER_RIGHT),
	CTesselateVert( CVertIndex(1, 0),  -1),
	CTesselateVert( CVertIndex(1,-1),  CHILDNODE_LOWER_RIGHT)
};

CTesselateWinding g_TWinding =
{
	g_TesselateVerts,
	sizeof( g_TesselateVerts ) / sizeof( g_TesselateVerts[0] )
};



// --------------------------------------------------------------------------------- //
// CPowerInfo stuff.
// --------------------------------------------------------------------------------- //

// Precalculated info about each particular displacement size.
#define DECLARE_TABLES( size ) \
	static CVertInfo	g_VertInfo_##size##x##size[ size*size ];				\
	static CFourVerts	g_SideVerts_##size##x##size[ size*size ];				\
	static CFourVerts	g_ChildVerts_##size##x##size[ size*size ];			\
	static CFourVerts	g_SideVertCorners_##size##x##size[ size*size ];	\
	static CTwoUShorts	g_ErrorEdges_##size##x##size[ size*size ];			\
	static CTriInfo		g_TriInfos_##size##x##size[ (size-1)*(size-1)*2 ];	\
	static CPowerInfo	g_PowerInfo_##size##x##size(							\
		g_VertInfo_##size##x##size,		\
		g_SideVerts_##size##x##size,		\
		g_ChildVerts_##size##x##size,		\
		g_SideVertCorners_##size##x##size,\
		g_ErrorEdges_##size##x##size,		\
		g_TriInfos_##size##x##size		\
	)

#define POWERINFO_ENTRY( size )	\
	(&g_PowerInfo_##size##x##size)

DECLARE_TABLES( 5 );	
DECLARE_TABLES( 9 );	
DECLARE_TABLES( 17 );	


// Index by m_Power.
CPowerInfo *g_PowerInfos[NUM_POWERINFOS] =
{
	NULL,
	NULL,
	POWERINFO_ENTRY(5),
	POWERINFO_ENTRY(9),
	POWERINFO_ENTRY(17)
};


CPowerInfo::CPowerInfo( 
	CVertInfo *pVertInfo, 
	CFourVerts *pSideVerts,
	CFourVerts *pChildVerts,
	CFourVerts *pSideVertCorners,
	CTwoUShorts *pErrorEdges,
	CTriInfo *pTriInfos )
{
	m_pVertInfo = pVertInfo;
	m_pSideVerts = pSideVerts;
	m_pChildVerts = pChildVerts;
	m_pSideVertCorners = pSideVertCorners;
	m_pErrorEdges = pErrorEdges;
	m_pTriInfos = pTriInfos;
}

static void InitPowerInfoTriInfos_R( 
	CPowerInfo *pInfo,
	CVertIndex const &nodeIndex,
	CTriInfo* &pTriInfo,
	int iMaxPower,
	int iLevel )
{
	int iNodeIndex = VertIndex( nodeIndex, iMaxPower );

	if( iLevel+1 < iMaxPower )
	{
		// Recurse into children.
		for( int iChild=0; iChild < 4; iChild++ )
		{
			InitPowerInfoTriInfos_R(
				pInfo,
				pInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild],
				pTriInfo,
				iMaxPower,
				iLevel+1 );
		}
	}
	else
	{
		unsigned short indices[3];
		
		int vertInc = 1 << ((iMaxPower - iLevel) - 1);

		// We're at a leaf, generate the tris.
		CTesselateWinding *pWinding = &g_TWinding;

		// Starting at the bottom-left, wind clockwise picking up vertices and
		// generating triangles.
		int iCurTriVert = 0;
		for( int iVert=0; iVert < pWinding->m_nVerts; iVert++ )
		{
			CVertIndex sideVert  = BuildOffsetVertIndex( nodeIndex, pWinding->m_Verts[iVert].m_Index, vertInc );
			
			if( iCurTriVert == 1 )
			{
				// Add this vert and finish the tri.
				pTriInfo->m_Indices[0] = indices[0];
				pTriInfo->m_Indices[1] = VertIndex( sideVert, iMaxPower );
				pTriInfo->m_Indices[2] = iNodeIndex;
				++pTriInfo;
			}

			indices[0] = VertIndex( sideVert, iMaxPower );
			iCurTriVert = 1;
		}
	}
}	


static void InitPowerInfo_R( 
	CPowerInfo *pPowerInfo, 
	int iMaxPower, 
	CVertIndex const &nodeIndex,
	CVertIndex const &dependency1,
	CVertIndex const &dependency2,
	CVertIndex const &nodeEdge1, 
	CVertIndex const &nodeEdge2, 
	CVertIndex const &iParent,
	int iLevel )
{
	int sideLength = ((1 << iMaxPower) + 1);
	int iNodeIndex = VertIndex( nodeIndex, iMaxPower );
	
	pPowerInfo->m_pVertInfo[iNodeIndex].m_iParent = iParent;
	pPowerInfo->m_pVertInfo[iNodeIndex].m_iNodeLevel = iLevel + 1;

	pPowerInfo->m_pErrorEdges[iNodeIndex].m_Values[0] = (unsigned short)(VertIndex( nodeEdge1, iMaxPower ));
	pPowerInfo->m_pErrorEdges[iNodeIndex].m_Values[1] = (unsigned short)(VertIndex( nodeEdge2, iMaxPower ));
	
	// Add this node's dependencies.
	AddDependency( pPowerInfo->m_pVertInfo, sideLength, nodeIndex, dependency1, iMaxPower, false, true );
	AddDependency( pPowerInfo->m_pVertInfo, sideLength, nodeIndex, dependency2, iMaxPower, false, true );

	// The 4 side vertices depend on this node.
	int iPower = iMaxPower - iLevel;
	int vertInc = 1 << (iPower - 1);

	for( int iSide=0; iSide < 4; iSide++ )
	{
		// Store the side vert index.
		CVertIndex sideVert( nodeIndex.x + g_SideVertMul[iSide][0]*vertInc, nodeIndex.y + g_SideVertMul[iSide][1]*vertInc );
		int iSideVert = VertIndex( sideVert, iMaxPower );

		pPowerInfo->m_pSideVerts[iNodeIndex].m_Verts[iSide] = sideVert;

		// Store the side vert corners.		
		CVertIndex sideVertCorner0 = CVertIndex( nodeIndex.x + g_SideVertCorners[iSide].m_Corner1[0]*vertInc, nodeIndex.y + g_SideVertCorners[iSide].m_Corner1[1]*vertInc );
		CVertIndex sideVertCorner1 = CVertIndex( nodeIndex.x + g_SideVertCorners[iSide].m_Corner2[0]*vertInc, nodeIndex.y + g_SideVertCorners[iSide].m_Corner2[1]*vertInc );

		pPowerInfo->m_pSideVertCorners[iNodeIndex].m_Verts[iSide] = sideVertCorner0;

		// Write the side vert corners into the error-edges list.
		pPowerInfo->m_pErrorEdges[iSideVert].m_Values[0] = (unsigned short)VertIndex( sideVertCorner0, iMaxPower );
		pPowerInfo->m_pErrorEdges[iSideVert].m_Values[1] = (unsigned short)VertIndex( sideVertCorner1, iMaxPower );

		AddDependency( 
			pPowerInfo->m_pVertInfo, 
			sideLength, 
			sideVert, 
			nodeIndex, 
			iMaxPower, 
			true, 
			true );
	}

	// Recurse into the children.
	int nodeInc = vertInc >> 1;
	if( nodeInc )
	{
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex childVert( nodeIndex.x + g_ChildNodeIndexMul[iChild].x * nodeInc, nodeIndex.y + g_ChildNodeIndexMul[iChild].y * nodeInc );

			pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild] = childVert;

			InitPowerInfo_R( pPowerInfo, 
				iMaxPower,
				childVert,
				
				CVertIndex(nodeIndex.x + g_ChildNodeDependencies[iChild][0].x*vertInc, nodeIndex.y + g_ChildNodeDependencies[iChild][0].y*vertInc),
				CVertIndex(nodeIndex.x + g_ChildNodeDependencies[iChild][1].x*vertInc, nodeIndex.y + g_ChildNodeDependencies[iChild][1].y*vertInc),
				
				nodeIndex,
				CVertIndex( nodeIndex.x + g_ChildNodeIndexMul[iChild].x * vertInc, nodeIndex.y + g_ChildNodeIndexMul[iChild].y * vertInc ),
				
				nodeIndex,
				iLevel + 1 );
		}
	}
}


void InitPowerInfo( CPowerInfo *pInfo, int iMaxPower )
{
	int sideLength = (1 << iMaxPower) + 1;

	// Precalculate the dependency graph.
	CVertIndex nodeDependency1( sideLength-1, sideLength-1 );
	CVertIndex nodeDependency2( 0, 0 );

	pInfo->m_RootNode = CVertIndex( sideLength/2, sideLength/2 );
	pInfo->m_SideLength = sideLength;
	pInfo->m_SideLengthM1 = sideLength - 1;
	pInfo->m_MidPoint = sideLength / 2;
	pInfo->m_MaxVerts = sideLength * sideLength;

	// Setup the corner indices.
	pInfo->m_CornerPointIndices[CORNER_LOWER_LEFT].Init( 0, 0 );
	pInfo->m_CornerPointIndices[CORNER_UPPER_LEFT].Init( 0, sideLength-1 );
	pInfo->m_CornerPointIndices[CORNER_UPPER_RIGHT].Init( sideLength-1, sideLength-1 );
	pInfo->m_CornerPointIndices[CORNER_LOWER_RIGHT].Init( sideLength-1, 0 );
	
	InitPowerInfo_R( 
		pInfo, 
		iMaxPower, 
		pInfo->m_RootNode, 
		
		nodeDependency1,							// dependencies
		nodeDependency2, 
		
		CVertIndex(0,0),							// error edge
		CVertIndex(sideLength-1, sideLength-1),		
		
		CVertIndex(-1,-1),							// parent
		0 );

	pInfo->m_Power = iMaxPower;
	
	CTriInfo *pTriInfo = pInfo->m_pTriInfos;
	InitPowerInfoTriInfos_R( pInfo, pInfo->m_RootNode, pTriInfo, iMaxPower, 0 );

	for( int iEdge=0; iEdge < 4; iEdge++ )
	{
		// Figure out the start vert and increment.
		CVertIndex nextVert;
		GetEdgeVertIndex( sideLength, iEdge, 0, pInfo->m_EdgeStartVerts[iEdge] );
		GetEdgeVertIndex( sideLength, iEdge, 1, nextVert );
		pInfo->m_EdgeIncrements[iEdge] = nextVert - pInfo->m_EdgeStartVerts[iEdge];

		// Now get the neighbor's start vert and increment.
		CVertIndex nbStartVert, nbNextVert, nbDelta;
		GetEdgeVertIndex( sideLength, (iEdge+2)&3, 0, nbStartVert );
		GetEdgeVertIndex( sideLength, (iEdge+2)&3, 1, nbNextVert );
		nbDelta = nbNextVert - nbStartVert;

		// Rotate it for each orientation.
		for( int orient=0; orient < 4; orient++ )
		{
			pInfo->m_NeighborStartVerts[iEdge][orient] = Transform2D( 
				g_OrientationRotations[orient], 
				nbStartVert, 
				CVertIndex( sideLength/2, sideLength/2 ) );

			pInfo->m_NeighborIncrements[iEdge][orient] = Transform2D( 
				g_OrientationRotations[orient],
				nbDelta,
				CVertIndex(0,0) );				 
		}
	}


	// Init the node index increments.
	int curPowerOf4 = 1;
	int curTotal = 0;
	for( int i=0; i < iMaxPower-1; i++ )
	{
		curTotal += curPowerOf4;

		pInfo->m_NodeIndexIncrements[iMaxPower-i-2] = curTotal;

		curPowerOf4 *= 4;		
	}

	// Store off the total node count
	pInfo->m_NodeCount = curTotal + curPowerOf4;

	pInfo->m_nTriInfos = Square( 1 << iMaxPower ) * 2;
}

class CPowerInfoInitializer
{
public:
	CPowerInfoInitializer()
	{
		Assert( MAX_MAP_DISP_POWER+1 == NUM_POWERINFOS );

		for( int i=0; i <= MAX_MAP_DISP_POWER; i++ )
		{
			if( g_PowerInfos[i] )
			{
				InitPowerInfo( g_PowerInfos[i], i );
			}
		}
	}
};

static CPowerInfoInitializer g_PowerInfoInitializer;


const CPowerInfo* GetPowerInfo( int iPower )
{
	Assert( iPower >= 0 && iPower < ARRAYSIZE( g_PowerInfos ) );
	Assert( g_PowerInfos[iPower] );
	return g_PowerInfos[iPower];
}


// ------------------------------------------------------------------------------------------------ //
// CPowerInfo member function initialization.
// ------------------------------------------------------------------------------------------------ //

const CVertIndex& CPowerInfo::GetCornerPointIndex( int iCorner ) const
{
	Assert( iCorner >= 0 && iCorner < 4 );
	return m_CornerPointIndices[iCorner];
}

