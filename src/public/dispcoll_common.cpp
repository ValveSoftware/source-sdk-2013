//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cmodel.h"
#include "dispcoll_common.h"
#include "collisionutils.h"
#include "tier1/strtools.h"
#include "tier0/vprof.h"
#include "tier1/fmtstr.h"
#include "tier1/utlhash.h"
#include "tier1/generichash.h"
#include "tier0/fasttimer.h"
#include "vphysics/virtualmesh.h"
#include "tier1/datamanager.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//	Cache

#ifdef ENGINE_DLL
CDataManager<CDispCollTree, CDispCollTree *, bool, CThreadFastMutex> g_DispCollTriCache( 2048*1024 );
#endif


struct DispCollPlaneIndex_t
{
	Vector vecPlane;
	int index;
};

class CPlaneIndexHashFuncs
{
public:
	CPlaneIndexHashFuncs( int ) {}

	// Compare
	bool operator()( const DispCollPlaneIndex_t &lhs, const DispCollPlaneIndex_t &rhs ) const
	{
		return ( lhs.vecPlane == rhs.vecPlane || lhs.vecPlane == -rhs.vecPlane );
	}

	// Hash
	unsigned int operator()( const DispCollPlaneIndex_t &item ) const
	{
		return HashItem( item.vecPlane ) ^ HashItem( -item.vecPlane );
	}
};

CUtlHash<DispCollPlaneIndex_t, CPlaneIndexHashFuncs, CPlaneIndexHashFuncs> g_DispCollPlaneIndexHash( 512 );


//=============================================================================
//	Displacement Collision Triangle

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDispCollTri::CDispCollTri()
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::Init( void )
{
	m_vecNormal.Init();
	m_flDist = 0.0f;
	m_TriData[0].m_IndexDummy = m_TriData[1].m_IndexDummy = m_TriData[2].m_IndexDummy = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::CalcPlane( CDispVector<Vector> &m_aVerts )
{
	Vector vecEdges[2];
	vecEdges[0] = m_aVerts[GetVert( 1 )] - m_aVerts[GetVert( 0 )];
	vecEdges[1] = m_aVerts[GetVert( 2 )] - m_aVerts[GetVert( 0 )];
	
	m_vecNormal = vecEdges[1].Cross( vecEdges[0] );
	VectorNormalize( m_vecNormal );
	m_flDist = m_vecNormal.Dot( m_aVerts[GetVert( 0 )] );

	// Calculate the signbits for the plane - fast test.
	m_ucSignBits = 0;
	m_ucPlaneType = PLANE_ANYZ;
	for ( int iAxis = 0; iAxis < 3 ; ++iAxis )
	{
		if ( m_vecNormal[iAxis] < 0.0f )
		{
			m_ucSignBits |= 1 << iAxis;
		}

		if ( m_vecNormal[iAxis] == 1.0f )
		{
			m_ucPlaneType = iAxis;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMin( float v1, float v2, float v3, int &iMin )
{
	float flMin = v1; 
	iMin = 0;
	if( v2 < flMin ) { flMin = v2; iMin = 1; }
	if( v3 < flMin ) { flMin = v3; iMin = 2; }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMax( float v1, float v2, float v3, int &iMax )
{
	float flMax = v1;
	iMax = 0;
	if( v2 > flMax ) { flMax = v2; iMax = 1; }
	if( v3 > flMax ) { flMax = v3; iMax = 2; }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::FindMinMax( CDispVector<Vector> &m_aVerts )
{
	int iMin, iMax;
	FindMin( m_aVerts[GetVert(0)].x, m_aVerts[GetVert(1)].x, m_aVerts[GetVert(2)].x, iMin );
	FindMax( m_aVerts[GetVert(0)].x, m_aVerts[GetVert(1)].x, m_aVerts[GetVert(2)].x, iMax );
	SetMin( 0, iMin );
	SetMax( 0, iMax );

	FindMin( m_aVerts[GetVert(0)].y, m_aVerts[GetVert(1)].y, m_aVerts[GetVert(2)].y, iMin );
	FindMax( m_aVerts[GetVert(0)].y, m_aVerts[GetVert(1)].y, m_aVerts[GetVert(2)].y, iMax );
	SetMin( 1, iMin );
	SetMax( 1, iMax );

	FindMin( m_aVerts[GetVert(0)].z, m_aVerts[GetVert(1)].z, m_aVerts[GetVert(2)].z, iMin );
	FindMax( m_aVerts[GetVert(0)].z, m_aVerts[GetVert(1)].z, m_aVerts[GetVert(2)].z, iMax );
	SetMin( 2, iMin );
	SetMax( 2, iMax );
}


// SIMD Routines for intersecting with the quad tree
FORCEINLINE int IntersectRayWithFourBoxes( const FourVectors &rayStart, const FourVectors &invDelta, const FourVectors &rayExtents, const FourVectors &boxMins, const FourVectors &boxMaxs )
{
	// SIMD Test ray against all four boxes at once
	// each node stores the bboxes of its four children
	FourVectors hitMins = boxMins;
	hitMins -= rayStart;
	FourVectors hitMaxs = boxMaxs;
	hitMaxs -= rayStart;

	// adjust for swept box by enlarging the child bounds to shrink the sweep down to a point
	hitMins -= rayExtents;
	hitMaxs += rayExtents;

	// compute the parametric distance along the ray of intersection in each dimension
	hitMins *= invDelta;
	hitMaxs *= invDelta;

	// Find the exit parametric intersection distance in each dimesion, for each box
	FourVectors exitT = maximum(hitMins,hitMaxs);
	// Find the entry parametric intersection distance in each dimesion, for each box
	FourVectors entryT = minimum(hitMins,hitMaxs);

	// now find the max overall entry distance across all dimensions for each box
	fltx4 minTemp = MaxSIMD(entryT.x, entryT.y);
	fltx4 boxEntryT = MaxSIMD(minTemp, entryT.z);

	// now find the min overall exit distance across all dimensions for each box
	fltx4 maxTemp = MinSIMD(exitT.x, exitT.y);
	fltx4 boxExitT = MinSIMD(maxTemp, exitT.z);

	boxEntryT = MaxSIMD(boxEntryT,Four_Zeros);
	boxExitT = MinSIMD(boxExitT,Four_Ones);

	// if entry<=exit for the box, we've got a hit
	fltx4 active = CmpLeSIMD(boxEntryT,boxExitT);			// mask of which boxes are active

	// hit at least one box?
	return TestSignSIMD(active);
}

// This does 4 simultaneous box intersections
// NOTE: This can be used as a 1 vs 4 test by replicating a single box into the one side
FORCEINLINE int IntersectFourBoxPairs( const FourVectors &mins0, const FourVectors &maxs0, const FourVectors &mins1, const FourVectors &maxs1 )
{
	// find the max mins and min maxs in each dimension
	FourVectors intersectMins = maximum(mins0,mins1);
	FourVectors intersectMaxs = minimum(maxs0,maxs1);

	// if intersectMins <= intersectMaxs then the boxes overlap in this dimension
	fltx4 overlapX = CmpLeSIMD(intersectMins.x,intersectMaxs.x);
	fltx4 overlapY = CmpLeSIMD(intersectMins.y,intersectMaxs.y);
	fltx4 overlapZ = CmpLeSIMD(intersectMins.z,intersectMaxs.z);
	
	// if the boxes overlap in all three dimensions, they intersect
	fltx4 tmp = AndSIMD( overlapX, overlapY );
	fltx4 active = AndSIMD( tmp, overlapZ );

	// hit at least one box?
	return TestSignSIMD(active);
}

// This does 4 simultaneous box vs. sphere intersections
// NOTE: This can be used as a 1 vs 4 test by replicating a single sphere/box into one side
FORCEINLINE int IntersectFourBoxSpherePairs( const FourVectors &center, const fltx4 &radiusSq, const FourVectors &mins, const FourVectors &maxs )
{
	// for each dimension of each box, compute the clamped distance from the mins side to the center (must be >= 0)
	FourVectors minDist = mins;
	minDist -= center;
	FourVectors dist;
	dist.x = MaxSIMD(Four_Zeros, minDist.x);
	dist.y = MaxSIMD(Four_Zeros, minDist.y);
	dist.z = MaxSIMD(Four_Zeros, minDist.z);

	// now compute the distance from the maxs side to the center
	FourVectors maxDist = center;
	maxDist -= maxs;
	// NOTE: Don't need to clamp here because we clamp against the minDist which must be >= 0, so the two clamps
	// get folded together
	FourVectors totalDist;
	totalDist.x = MaxSIMD(dist.x, maxDist.x);
	totalDist.y = MaxSIMD(dist.y, maxDist.y);
	totalDist.z = MaxSIMD(dist.z, maxDist.z);
	// get the total squred distance between each box & sphere center by summing the squares of each
	// component/dimension
	fltx4 distSq = totalDist * totalDist;

	// if squared distance between each sphere center & box is less than the radiusSquared for that sphere
	// we have an intersection
	fltx4 active = CmpLeSIMD(distSq,radiusSq);

	// at least one intersection?
	return TestSignSIMD(active);
}


int FORCEINLINE CDispCollTree::BuildRayLeafList( int iNode, rayleaflist_t &list )
{
	list.nodeList[0] = iNode;
	int listIndex = 0;
	list.maxIndex = 0;
	while ( listIndex <= list.maxIndex )
	{
		iNode = list.nodeList[listIndex];
		// the rest are all leaves
		if ( IsLeafNode(iNode) )
			return listIndex;
		listIndex++;
		const CDispCollNode &node = m_nodes[iNode];
		int mask = IntersectRayWithFourBoxes( list.rayStart, list.invDelta, list.rayExtents, node.m_mins, node.m_maxs );
		if ( mask )
		{
			int child = Nodes_GetChild( iNode, 0 );
			if ( mask & 1 )
			{
				++list.maxIndex;
				list.nodeList[list.maxIndex] = child;
			}
			if ( mask & 2 )
			{
				++list.maxIndex;
				list.nodeList[list.maxIndex] = child+1;
			}
			if ( mask & 4 )
			{
				++list.maxIndex;
				list.nodeList[list.maxIndex] = child+2;
			}
			if ( mask & 8 )
			{
				++list.maxIndex;
				list.nodeList[list.maxIndex] = child+3;
			}
			Assert(list.maxIndex < MAX_AABB_LIST);
		}
	}

	return listIndex;
}


//-----------------------------------------------------------------------------
// Purpose: Create the AABB tree.
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Create( CCoreDispInfo *pDisp )
{
	// Copy the flags.
	m_nFlags = pDisp->GetSurface()->GetFlags();

	// Copy necessary displacement data.
	AABBTree_CopyDispData( pDisp );
	
	// Setup/create the leaf nodes first so the recusion can use this data to stop.
	AABBTree_CreateLeafs();

	// Create the bounding box of the displacement surface + the base face.
	AABBTree_CalcBounds();

	// Successful.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CopyDispData( CCoreDispInfo *pDisp )
{
	// Displacement size.
	m_nPower = pDisp->GetPower();

	// Displacement base surface data.
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_nContents = pSurf->GetContents();
	pSurf->GetNormal( m_vecStabDir );
	for ( int iPoint = 0; iPoint < 4; iPoint++ )
	{
		pSurf->GetPoint( iPoint, m_vecSurfPoints[iPoint] );
	}

	// Allocate collision tree data.
	{
	MEM_ALLOC_CREDIT();
	m_aVerts.SetSize( GetSize() );
	}

	{
	MEM_ALLOC_CREDIT();
	m_aTris.SetSize( GetTriSize() );
	}

	{
	MEM_ALLOC_CREDIT();
	int numLeaves = (GetWidth()-1) * (GetHeight()-1);
	m_leaves.SetCount(numLeaves);
	int numNodes = Nodes_CalcCount( m_nPower );
	numNodes -= numLeaves;
	m_nodes.SetCount(numNodes);
	}


	// Setup size.
	m_nSize = sizeof( this );
	m_nSize += sizeof( Vector ) * GetSize();
	m_nSize += sizeof( CDispCollTri ) * GetTriSize();
#if OLD_DISP_AABB
	m_nSize += sizeof( CDispCollAABBNode ) * Nodes_CalcCount( m_nPower );
#endif
	m_nSize += sizeof(m_nodes[0]) * m_nodes.Count();
	m_nSize += sizeof(m_leaves[0]) * m_leaves.Count();
	m_nSize += sizeof( CDispCollTri* ) * DISPCOLL_TREETRI_SIZE;

	// Copy vertex data.
	for ( int iVert = 0; iVert < m_aVerts.Count(); iVert++ )
	{
		pDisp->GetVert( iVert, m_aVerts[iVert] );
	}

	// Copy and setup triangle data.
	unsigned short iVerts[3];
	for ( int iTri = 0; iTri < m_aTris.Count(); ++iTri )
	{
		pDisp->GetTriIndices( iTri, iVerts[0], iVerts[1], iVerts[2] );
		m_aTris[iTri].SetVert( 0, iVerts[0] );
		m_aTris[iTri].SetVert( 1, iVerts[1] );
		m_aTris[iTri].SetVert( 2, iVerts[2] );
		m_aTris[iTri].m_uiFlags = pDisp->GetTriTagValue( iTri );

		// Calculate the surface props and set flags.
		float flTotalAlpha = 0.0f;
		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			flTotalAlpha += pDisp->GetAlpha( m_aTris[iTri].GetVert( iVert ) );
		}

		if ( flTotalAlpha > DISP_ALPHA_PROP_DELTA )
		{
			m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFPROP2;
		}
		else
		{
			m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFPROP1;
		}

		// Add the displacement surface flag.
		m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFACE;

		// Calculate the plane normal and the min max.
		m_aTris[iTri].CalcPlane( m_aVerts );
		m_aTris[iTri].FindMinMax( m_aVerts );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CreateLeafs( void )
{
	int numLeaves = (GetWidth()-1) * (GetHeight()-1);
	m_leaves.SetCount(numLeaves);
	int numNodes = Nodes_CalcCount( m_nPower );
	numNodes -= numLeaves;
	m_nodes.SetCount(numNodes);

	// Get the width and height of the displacement.
	int nWidth = GetWidth() - 1;
	int nHeight = GetHeight() - 1;

	for ( int iHgt = 0; iHgt < nHeight; ++iHgt )
	{
		for ( int iWid = 0; iWid < nWidth; ++iWid )
		{
			int iLeaf = Nodes_GetIndexFromComponents( iWid, iHgt );
			int iIndex = iHgt * nWidth + iWid;
			int iTri = iIndex * 2;

			m_leaves[iLeaf].m_tris[0] = iTri;
			m_leaves[iLeaf].m_tris[1] = iTri + 1;
		}
	}
}

void CDispCollTree::AABBTree_GenerateBoxes_r( int nodeIndex, Vector *pMins, Vector *pMaxs )
{
	// leaf
	ClearBounds( *pMins, *pMaxs );
	if ( nodeIndex >= m_nodes.Count() )
	{
		int iLeaf = nodeIndex - m_nodes.Count();

		for ( int iTri = 0; iTri < 2; ++iTri )
		{
			int triIndex = m_leaves[iLeaf].m_tris[iTri];
			const CDispCollTri &tri = m_aTris[triIndex];
			AddPointToBounds( m_aVerts[tri.GetVert( 0 )], *pMins, *pMaxs );
			AddPointToBounds( m_aVerts[tri.GetVert( 1 )], *pMins, *pMaxs );
			AddPointToBounds( m_aVerts[tri.GetVert( 2 )], *pMins, *pMaxs );
		}
	}
	else // node
	{
		Vector childMins[4], childMaxs[4];
		for ( int i = 0; i < 4; i++ )
		{
			int child = Nodes_GetChild( nodeIndex, i );
			AABBTree_GenerateBoxes_r( child, &childMins[i], &childMaxs[i] );
			AddPointToBounds( childMins[i], *pMins, *pMaxs );
			AddPointToBounds( childMaxs[i], *pMins, *pMaxs );
		}
		m_nodes[nodeIndex].m_mins.LoadAndSwizzle( childMins[0], childMins[1], childMins[2], childMins[3] );
		m_nodes[nodeIndex].m_maxs.LoadAndSwizzle( childMaxs[0], childMaxs[1], childMaxs[2], childMaxs[3] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CalcBounds( void )
{
	// Check data.
	if ( ( m_aVerts.Count() == 0 ) || ( m_nodes.Count() == 0 ) )
		return;

	AABBTree_GenerateBoxes_r( 0, &m_mins, &m_maxs );

#if INCLUDE_SURFACE_IN_BOUNDS
	// Add surface points to bounds.
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		VectorMin( m_vecSurfPoints[iPoint], m_mins, m_mins );
		VectorMax( m_vecSurfPoints[iPoint], m_maxs, m_maxs );
	}
#endif

	// Bloat a little.
	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		m_mins[iAxis] -= 1.0f;
		m_maxs[iAxis] += 1.0f;
	}
}

static CThreadFastMutex s_CacheMutex;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CDispCollTree::LockCache()
{
#ifdef ENGINE_DLL
	if ( !g_DispCollTriCache.LockResource( m_hCache ) )
	{
		AUTO_LOCK( s_CacheMutex );

		// Cache may have just been created, so check once more
		if ( !g_DispCollTriCache.LockResource( m_hCache ) )
		{
			Cache();
			m_hCache = g_DispCollTriCache.CreateResource( this );
			g_DispCollTriCache.LockResource( m_hCache );
			//Msg( "Adding 0x%x to cache (actual %d) [%d, %d --> %.2f] %d total, %d unique\n", this, GetCacheMemorySize(), GetTriSize(), m_aEdgePlanes.Count(), (float)m_aEdgePlanes.Count()/(float)GetTriSize(), totals, uniques );
		}
	}
#else
	Cache();
#endif
}

inline void CDispCollTree::UnlockCache()
{
#ifdef ENGINE_DLL
	g_DispCollTriCache.UnlockResource( m_hCache );
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Cache( void )
{
	if ( m_aTrisCache.Count() == GetTriSize() )
	{
		return;
	}

	VPROF( "CDispCollTree::Cache" );

	// Alloc.
//	int nSize = sizeof( CDispCollTriCache ) * GetTriSize();
	int nTriCount = GetTriSize();
	{
	MEM_ALLOC_CREDIT();
	m_aTrisCache.SetSize( nTriCount );
	}

	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		Cache_Create( &m_aTris[iTri], iTri );
	}

	g_DispCollPlaneIndexHash.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, RayDispOutput_t &output )
{
	if ( IsBoxIntersectingRay( m_mins, m_maxs, ray.m_Start, ray.m_Delta, DISPCOLL_DIST_EPSILON ) )
	{
		return AABBTree_Ray( ray, ray.InvDelta(), output );
	}
	return false;
}

bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, const Vector &vecInvDelta, RayDispOutput_t &output )
{
	VPROF( "DispRayTest" );

	// Check for ray test.
	if ( CheckFlags( CCoreDispInfo::SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;

	// Pre-calc the inverse delta for perf.
	CDispCollTri *pImpactTri = NULL;

	AABBTree_TreeTrisRayBarycentricTest( ray, vecInvDelta, DISPCOLL_ROOTNODE_INDEX, output, &pImpactTri );

	if ( pImpactTri )
	{
		// Collision.
		output.ndxVerts[0] = pImpactTri->GetVert( 0 );
		output.ndxVerts[1] = pImpactTri->GetVert( 2 );
		output.ndxVerts[2] = pImpactTri->GetVert( 1 );

		Assert( (output.u <= 1.0f ) && ( output.v <= 1.0f ) );
		Assert( (output.u >= 0.0f ) && ( output.v >= 0.0f ) );

		return true;
	}

	// No collision.
	return false;
}

void CDispCollTree::AABBTree_TreeTrisRayBarycentricTest( const Ray_t &ray, const Vector &vecInvDelta, int iNode, RayDispOutput_t &output, CDispCollTri **pImpactTri )
{
	rayleaflist_t list;
	// NOTE: This part is loop invariant - should be hoisted up as far as possible
	list.invDelta.DuplicateVector(vecInvDelta);
	list.rayStart.DuplicateVector(ray.m_Start);
	Vector ext = ray.m_Extents + Vector(DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON);
	list.rayExtents.DuplicateVector(ext);
	int listIndex = BuildRayLeafList( iNode, list );

	float flU, flV, flT;
	for ( ; listIndex <= list.maxIndex; listIndex++ )
	{
		int leafIndex = list.nodeList[listIndex] - m_nodes.Count();
		CDispCollTri *pTri0 = &m_aTris[m_leaves[leafIndex].m_tris[0]];
		CDispCollTri *pTri1 = &m_aTris[m_leaves[leafIndex].m_tris[1]];

		if ( ComputeIntersectionBarycentricCoordinates( ray, m_aVerts[pTri0->GetVert( 0 )], m_aVerts[pTri0->GetVert( 2 )], m_aVerts[pTri0->GetVert( 1 )], flU, flV, &flT ) )
		{
			// Make sure it's inside the range
			if ( ( flU >= 0.0f ) && ( flV >= 0.0f ) && ( ( flU + flV ) <= 1.0f ) )
			{
				if( ( flT > 0.0f ) && ( flT < output.dist ) )
				{
					(*pImpactTri) = pTri0;
					output.u = flU;
					output.v = flV;
					output.dist = flT;
				}
			}
		}
		if ( ComputeIntersectionBarycentricCoordinates( ray, m_aVerts[pTri1->GetVert( 0 )], m_aVerts[pTri1->GetVert( 2 )], m_aVerts[pTri1->GetVert( 1 )], flU, flV, &flT ) )
		{
			// Make sure it's inside the range
			if ( ( flU >= 0.0f ) && ( flV >= 0.0f ) && ( ( flU + flV ) <= 1.0f ) )
			{
				if( ( flT > 0.0f ) && ( flT < output.dist ) )
				{
					(*pImpactTri) = pTri1;
					output.u = flU;
					output.v = flV;
					output.dist = flT;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, const Vector &vecInvDelta, CBaseTrace *pTrace, bool bSide )
{
	VPROF("AABBTree_Ray");

//	VPROF_BUDGET( "DispRayTraces", VPROF_BUDGETGROUP_DISP_RAYTRACES );

	// Check for ray test.
	if ( CheckFlags( CCoreDispInfo::SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;

	// Pre-calc the inverse delta for perf.
	CDispCollTri *pImpactTri = NULL;

	AABBTree_TreeTrisRayTest( ray, vecInvDelta, DISPCOLL_ROOTNODE_INDEX, pTrace, bSide, &pImpactTri );

	if ( pImpactTri )
	{
		// Collision.
		VectorCopy( pImpactTri->m_vecNormal, pTrace->plane.normal );
		pTrace->plane.dist = pImpactTri->m_flDist;
		pTrace->dispFlags = pImpactTri->m_uiFlags;
		return true;
	}

	// No collision.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_TreeTrisRayTest( const Ray_t &ray, const Vector &vecInvDelta, int iNode, CBaseTrace *pTrace, bool bSide, CDispCollTri **pImpactTri )
{
	rayleaflist_t list;
	// NOTE: This part is loop invariant - should be hoisted up as far as possible
	list.invDelta.DuplicateVector(vecInvDelta);
	list.rayStart.DuplicateVector(ray.m_Start);
	Vector ext = ray.m_Extents + Vector(DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON);
	list.rayExtents.DuplicateVector(ext);
	int listIndex = BuildRayLeafList( iNode, list );

	for ( ;listIndex <= list.maxIndex; listIndex++ )
	{
		int leafIndex = list.nodeList[listIndex] - m_nodes.Count();
		CDispCollTri *pTri0 = &m_aTris[m_leaves[leafIndex].m_tris[0]];
		CDispCollTri *pTri1 = &m_aTris[m_leaves[leafIndex].m_tris[1]];
		float flFrac = IntersectRayWithTriangle( ray, m_aVerts[pTri0->GetVert( 0 )], m_aVerts[pTri0->GetVert( 2 )], m_aVerts[pTri0->GetVert( 1 )], bSide );
		if( ( flFrac >= 0.0f ) && ( flFrac < pTrace->fraction ) )
		{
			pTrace->fraction = flFrac;
			(*pImpactTri) = pTri0;
		}
		
		flFrac = IntersectRayWithTriangle( ray, m_aVerts[pTri1->GetVert( 0 )], m_aVerts[pTri1->GetVert( 2 )], m_aVerts[pTri1->GetVert( 1 )], bSide );
		if( ( flFrac >= 0.0f ) && ( flFrac < pTrace->fraction ) )
		{
			pTrace->fraction = flFrac;
			(*pImpactTri) = pTri1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CDispCollTree::AABBTree_GetTrisInSphere( const Vector &center, float radius, unsigned short *pIndexOut, int indexMax )
{
	return AABBTree_BuildTreeTrisInSphere_r( center, radius, DISPCOLL_ROOTNODE_INDEX, pIndexOut, indexMax );
}

int CDispCollTree::AABBTree_BuildTreeTrisInSphere_r( const Vector &center, float radius, int iNode, unsigned short *pIndexOut, unsigned short indexMax )
{
	int nodeList[MAX_AABB_LIST];
	nodeList[0] = iNode;
	int nTriCount = 0;
	int listIndex = 0;
	int maxIndex = 0;
	// NOTE: This part is loop invariant - should be hoisted up as far as possible
	FourVectors sphereCenters;
	sphereCenters.DuplicateVector(center);
	float radiusSq = radius * radius;
	fltx4 sphereRadSq = ReplicateX4(radiusSq);
	while ( listIndex <= maxIndex )
	{
		iNode = nodeList[listIndex];
		listIndex++;
		// the rest are all leaves
		if ( IsLeafNode(iNode) )
		{
			VPROF("Tris");
			for ( --listIndex; listIndex <= maxIndex; listIndex++ )
			{
				if ( (nTriCount+2) <= indexMax )
				{
					int leafIndex = nodeList[listIndex] - m_nodes.Count();
					pIndexOut[nTriCount] = m_leaves[leafIndex].m_tris[0];
					pIndexOut[nTriCount+1] = m_leaves[leafIndex].m_tris[1];
					nTriCount += 2;
				}
			}
			break;
		}
		else
		{
			const CDispCollNode &node = m_nodes[iNode];
			int mask = IntersectFourBoxSpherePairs( sphereCenters, sphereRadSq, node.m_mins, node.m_maxs );
			if ( mask )
			{
				int child = Nodes_GetChild( iNode, 0 );
				if ( mask & 1 )
				{
					++maxIndex;
					nodeList[maxIndex] = child;
				}
				if ( mask & 2 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+1;
				}
				if ( mask & 4 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+2;
				}
				if ( mask & 8 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+3;
				}
				Assert(maxIndex < MAX_AABB_LIST);
			}
		}
	}
	return nTriCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_IntersectAABB( const Vector &absMins, const Vector &absMaxs )
{
	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;
	
	cplane_t plane;
	Vector center = 0.5f *(absMins + absMaxs);
	Vector extents = absMaxs - center;

	int nodeList[MAX_AABB_LIST];
	nodeList[0] = 0;
	int listIndex = 0;
	int maxIndex = 0;

	// NOTE: This part is loop invariant - should be hoisted up as far as possible
	FourVectors mins0;
	mins0.DuplicateVector(absMins);
	FourVectors maxs0;
	maxs0.DuplicateVector(absMaxs);
	while ( listIndex <= maxIndex )
	{
		int iNode = nodeList[listIndex];
		listIndex++;
		// the rest are all leaves
		if ( IsLeafNode(iNode) )
		{
			VPROF("Tris");
			for ( --listIndex; listIndex <= maxIndex; listIndex++ )
			{
				int leafIndex = nodeList[listIndex] - m_nodes.Count();
				CDispCollTri *pTri0 = &m_aTris[m_leaves[leafIndex].m_tris[0]];
				CDispCollTri *pTri1 = &m_aTris[m_leaves[leafIndex].m_tris[1]];

				VectorCopy( pTri0->m_vecNormal, plane.normal );
				plane.dist = pTri0->m_flDist;
				plane.signbits = pTri0->m_ucSignBits;
				plane.type = pTri0->m_ucPlaneType;

				if ( IsBoxIntersectingTriangle( center, extents,
					m_aVerts[pTri0->GetVert( 0 )],
					m_aVerts[pTri0->GetVert( 2 )],
					m_aVerts[pTri0->GetVert( 1 )],
					plane, 0.0f ) )
					return true;
				VectorCopy( pTri1->m_vecNormal, plane.normal );
				plane.dist = pTri1->m_flDist;
				plane.signbits = pTri1->m_ucSignBits;
				plane.type = pTri1->m_ucPlaneType;

				if ( IsBoxIntersectingTriangle( center, extents,
					m_aVerts[pTri1->GetVert( 0 )],
					m_aVerts[pTri1->GetVert( 2 )],
					m_aVerts[pTri1->GetVert( 1 )],
					plane, 0.0f ) )
					return true;
			}
			break;
		}
		else
		{
			const CDispCollNode &node = m_nodes[iNode];
			int mask = IntersectFourBoxPairs( mins0, maxs0, node.m_mins, node.m_maxs );
			if ( mask )
			{
				int child = Nodes_GetChild( iNode, 0 );
				if ( mask & 1 )
				{
					++maxIndex;
					nodeList[maxIndex] = child;
				}
				if ( mask & 2 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+1;
				}
				if ( mask & 4 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+2;
				}
				if ( mask & 8 )
				{
					++maxIndex;
					nodeList[maxIndex] = child+3;
				}
				Assert(maxIndex < MAX_AABB_LIST);
			}
		}
	}

	// no collision
	return false; 
}

static const Vector g_Vec3DispCollEpsilons(DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON,DISPCOLL_DIST_EPSILON);
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_SweepAABB( const Ray_t &ray, const Vector &vecInvDelta, CBaseTrace *pTrace )
{
	VPROF( "DispHullTest" );
	//	VPROF_BUDGET( "DispHullTraces", VPROF_BUDGETGROUP_DISP_HULLTRACES );
	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;

	// Test ray against the triangles in the list.
	Vector rayDir;
	VectorCopy( ray.m_Delta, rayDir );
	VectorNormalize( rayDir );
	// Save fraction.
	float flFrac = pTrace->fraction;
	
	rayleaflist_t list;
	// NOTE: This part is loop invariant - should be hoisted up as far as possible
	list.invDelta.DuplicateVector(vecInvDelta);
	list.rayStart.DuplicateVector(ray.m_Start);
	Vector ext = ray.m_Extents + g_Vec3DispCollEpsilons;
	list.rayExtents.DuplicateVector(ext);
	int listIndex = BuildRayLeafList( 0, list );

	if ( listIndex <= list.maxIndex )
	{
		LockCache();
		for ( ; listIndex <= list.maxIndex; listIndex++ )
		{
			int leafIndex = list.nodeList[listIndex] - m_nodes.Count();
			int iTri0 = m_leaves[leafIndex].m_tris[0];
			int iTri1 = m_leaves[leafIndex].m_tris[1];
			CDispCollTri *pTri0 = &m_aTris[iTri0];
			CDispCollTri *pTri1 = &m_aTris[iTri1];

			SweepAABBTriIntersect( ray, rayDir, iTri0, pTri0, pTrace );
			SweepAABBTriIntersect( ray, rayDir, iTri1, pTri1, pTrace );
		}
		UnlockCache();
	}

	// Collision.
	if ( pTrace->fraction < flFrac )
		return true;
	
	// No collision.
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CDispCollTree::ResolveRayPlaneIntersect( float flStart, float flEnd, const Vector &vecNormal, float flDist, CDispCollHelper *pHelper )
{
	if( ( flStart > 0.0f ) && ( flEnd > 0.0f ) ) 
		return false; 

	if( ( flStart < 0.0f ) && ( flEnd < 0.0f ) ) 
		return true; 

	float flDenom = flStart - flEnd;
	bool bDenomIsZero = ( flDenom == 0.0f );
	if( ( flStart >= 0.0f ) && ( flEnd <= 0.0f ) )
	{
		// Find t - the parametric distance along the trace line.
		float t = ( !bDenomIsZero ) ? ( flStart - DISPCOLL_DIST_EPSILON ) / flDenom : 0.0f;
		if( t > pHelper->m_flStartFrac )
		{
			pHelper->m_flStartFrac = t;
			VectorCopy( vecNormal, pHelper->m_vecImpactNormal );
			pHelper->m_flImpactDist = flDist;
		}
	}
	else
	{
		// Find t - the parametric distance along the trace line.
		float t = ( !bDenomIsZero ) ? ( flStart + DISPCOLL_DIST_EPSILON ) / flDenom : 0.0f;
		if( t < pHelper->m_flEndFrac )
		{
			pHelper->m_flEndFrac = t;
		}	
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CDispCollTree::FacePlane( const Ray_t &ray, const Vector &rayDir, CDispCollTri *pTri, CDispCollHelper *pHelper )
{
	// Calculate the closest point on box to plane (get extents in that direction).
	Vector vecExtent;
	CalcClosestExtents( pTri->m_vecNormal, ray.m_Extents, vecExtent );

	float flExpandDist = pTri->m_flDist - pTri->m_vecNormal.Dot( vecExtent );

	float flStart = pTri->m_vecNormal.Dot( ray.m_Start ) - flExpandDist;
	float flEnd = pTri->m_vecNormal.Dot( ( ray.m_Start + ray.m_Delta ) ) - flExpandDist;

	return ResolveRayPlaneIntersect( flStart, flEnd, pTri->m_vecNormal, pTri->m_flDist, pHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FORCEINLINE CDispCollTree::AxisPlanesXYZ( const Ray_t &ray, CDispCollTri *pTri, CDispCollHelper *pHelper )
{
	static const TableVector g_ImpactNormalVecs[2][3] = 
	{
		{
			{ -1, 0, 0 },
			{ 0, -1, 0 },
			{ 0, 0, -1 },
		},

		{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 },
		}
	};

	float flDist, flExpDist, flStart, flEnd;
	
	int iAxis;
	for ( iAxis = 2; iAxis >= 0; --iAxis )
	{
		const float rayStart = ray.m_Start[iAxis];
		const float rayExtent = ray.m_Extents[iAxis];
		const float rayDelta = ray.m_Delta[iAxis];

		// Min
		flDist = m_aVerts[pTri->GetVert(pTri->GetMin(iAxis))][iAxis];
		flExpDist = flDist - rayExtent;
		flStart = flExpDist - rayStart;
		flEnd = flStart - rayDelta;

		if ( !ResolveRayPlaneIntersect( flStart, flEnd, g_ImpactNormalVecs[0][iAxis], flDist, pHelper ) )
			return false;

		// Max
		flDist = m_aVerts[pTri->GetVert(pTri->GetMax(iAxis))][iAxis];
		flExpDist = flDist + rayExtent;
		flStart = rayStart - flExpDist;
		flEnd = flStart + rayDelta;

		if ( !ResolveRayPlaneIntersect( flStart, flEnd, g_ImpactNormalVecs[1][iAxis], flDist, pHelper ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Testing!
//-----------------------------------------------------------------------------
void CDispCollTree::Cache_Create( CDispCollTri *pTri, int iTri )
{
	MEM_ALLOC_CREDIT();
	Vector *pVerts[3];
	pVerts[0] = &m_aVerts[pTri->GetVert( 0 )];
	pVerts[1] = &m_aVerts[pTri->GetVert( 1 )];
	pVerts[2] = &m_aVerts[pTri->GetVert( 2 )];

	CDispCollTriCache *pCache = &m_aTrisCache[iTri];

	Vector vecEdge;

	// Edge 1
	VectorSubtract( *pVerts[1], *pVerts[0], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossX[0] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossY[0] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossZ[0] );

	// Edge 2
	VectorSubtract( *pVerts[2], *pVerts[1], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossX[1] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossY[1] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossZ[1] );

	// Edge 3
	VectorSubtract( *pVerts[0], *pVerts[2], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossX[2] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossY[2] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossZ[2] );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CDispCollTree::AddPlane( const Vector &vecNormal )
{
	UtlHashHandle_t handle;
	DispCollPlaneIndex_t planeIndex;
	bool bDidInsert;

	planeIndex.vecPlane = vecNormal;
	planeIndex.index = m_aEdgePlanes.Count();

	handle = g_DispCollPlaneIndexHash.Insert( planeIndex, &bDidInsert );

	if ( !bDidInsert )
	{
		DispCollPlaneIndex_t &existingEntry = g_DispCollPlaneIndexHash[handle];
		if ( existingEntry.vecPlane == vecNormal )
		{
			return existingEntry.index;
		}
		else
		{
			return ( existingEntry.index | 0x8000 );
		}
	}

	return m_aEdgePlanes.AddToTail( vecNormal );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: The plane distance get stored in the normal x position since it isn't
//       used.
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisX( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisX = ( 0.0, edgeZ, -edgeY )
	Vector vecNormal( 0.0f, vecEdge.z, -vecEdge.y );
	VectorNormalize( vecNormal );

	// Check for zero length normals.
	if( ( vecNormal.y == 0.0f ) || ( vecNormal.z == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.y * vecOnEdge.y ) + ( vecNormal.z * vecOnEdge.z );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.y * vecOffEdge.y ) + ( vecNormal.z * vecOffEdge.z );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing - triangle should be behind the plane.
		vecNormal.x = -flDist;
		vecNormal.y = -vecNormal.y;
		vecNormal.z = -vecNormal.z;
	}
	else
	{
		vecNormal.x = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: The plane distance get stored in the normal y position since it isn't
//       used.
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisY( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisY = ( -edgeZ, 0.0, edgeX )
	Vector vecNormal( -vecEdge.z, 0.0f, vecEdge.x );
	VectorNormalize( vecNormal );

	// Check for zero length normals
	if( ( vecNormal.x == 0.0f ) || ( vecNormal.z == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.x * vecOnEdge.x ) + ( vecNormal.z * vecOnEdge.z );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.x * vecOffEdge.x ) + ( vecNormal.z * vecOffEdge.z );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing if necessay - triangle should be behind the plane.
		vecNormal.x = -vecNormal.x;
		vecNormal.y = -flDist;
		vecNormal.z = -vecNormal.z;
	}
	else
	{
		vecNormal.y = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisZ( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisY = ( edgeY, -edgeX, 0.0 )
	Vector vecNormal( vecEdge.y, -vecEdge.x, 0.0f );
	VectorNormalize( vecNormal );

	// Check for zero length normals
	if( ( vecNormal.x == 0.0f ) || ( vecNormal.y == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.x * vecOnEdge.x ) + ( vecNormal.y * vecOnEdge.y );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.x * vecOffEdge.x ) + ( vecNormal.y * vecOffEdge.y );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing if necessay - triangle should be behind the plane.
		vecNormal.x = -vecNormal.x;
		vecNormal.y = -vecNormal.y;
		vecNormal.z = -flDist;
	}
	else
	{
		vecNormal.z = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <int AXIS>
bool CDispCollTree::EdgeCrossAxis( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper )
{
	if ( iPlane == DISPCOLL_NORMAL_UNDEF )
		return true;

	// Get the edge plane.
	Vector vecNormal;
	if ( ( iPlane & 0x8000 ) != 0 )
	{
		VectorCopy( m_aEdgePlanes[(iPlane&0x7fff)], vecNormal );
		vecNormal.Negate();
	}
	else
	{
		VectorCopy( m_aEdgePlanes[iPlane], vecNormal );
	}

	const int OTHER_AXIS1 = ( AXIS + 1 ) % 3;
	const int OTHER_AXIS2 = ( AXIS + 2 ) % 3;

	// Get the pland distance are "fix" the normal.
	float flDist = vecNormal[AXIS];
	vecNormal[AXIS] = 0.0f;

	// Calculate the closest point on box to plane (get extents in that direction).
	Vector vecExtent;
	//vecExtent[AXIS] = 0.0f;
	vecExtent[OTHER_AXIS1] = ( vecNormal[OTHER_AXIS1] < 0.0f ) ? ray.m_Extents[OTHER_AXIS1] : -ray.m_Extents[OTHER_AXIS1];
	vecExtent[OTHER_AXIS2] = ( vecNormal[OTHER_AXIS2] < 0.0f ) ? ray.m_Extents[OTHER_AXIS2] : -ray.m_Extents[OTHER_AXIS2];

	// Expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above).
	Vector vecEnd;
	vecEnd[AXIS] = 0;
	vecEnd[OTHER_AXIS1] = ray.m_Start[OTHER_AXIS1] + ray.m_Delta[OTHER_AXIS1];
	vecEnd[OTHER_AXIS2] = ray.m_Start[OTHER_AXIS2] + ray.m_Delta[OTHER_AXIS2];

	float flExpandDist 	= flDist - ( ( vecNormal[OTHER_AXIS1] * vecExtent[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * vecExtent[OTHER_AXIS2] ) );
	float flStart 		= ( vecNormal[OTHER_AXIS1] * ray.m_Start[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * ray.m_Start[OTHER_AXIS2] ) - flExpandDist;
	float flEnd 		= ( vecNormal[OTHER_AXIS1] * vecEnd[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * vecEnd[OTHER_AXIS2] ) - flExpandDist;

	return ResolveRayPlaneIntersect( flStart, flEnd, vecNormal, flDist, pHelper );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisX( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper )
{
	return EdgeCrossAxis<0>( ray, iPlane, pHelper );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisY( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper )
{
	return EdgeCrossAxis<1>( ray, iPlane, pHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisZ( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper )
{
	return EdgeCrossAxis<2>( ray, iPlane, pHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::SweepAABBTriIntersect( const Ray_t &ray, const Vector &rayDir, int iTri, CDispCollTri *pTri, CBaseTrace *pTrace )
{
	// Init test data.
	CDispCollHelper helper;
	helper.m_flEndFrac = 1.0f;
	helper.m_flStartFrac = DISPCOLL_INVALID_FRAC;

	// Make sure objects are traveling toward one another.
	float flDistAlongNormal = pTri->m_vecNormal.Dot( ray.m_Delta );
	if( flDistAlongNormal > DISPCOLL_DIST_EPSILON )
		return;

	// Test against the axis planes.
	if ( !AxisPlanesXYZ( ray, pTri, &helper ) )
	{
		return;
	}

	//
	// There are 9 edge tests - edges 1, 2, 3 cross with the box edges (symmetry) 1, 2, 3.  However, the box
	// is axis-aligned resulting in axially directional edges -- thus each test is edges 1, 2, and 3 vs. 
	// axial planes x, y, and z
	//
	// There are potentially 9 more tests with edges, the edge's edges and the direction of motion!
	// NOTE: I don't think these tests are necessary for a manifold surface.
	//

	CDispCollTriCache *pCache = &m_aTrisCache[iTri];

	// Edges 1-3, interleaved - axis tests are 2d tests
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[0], &helper ) ) { return; }
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[1], &helper ) ) { return; }
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[2], &helper ) ) { return; }

	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[0], &helper ) ) { return; }
	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[1], &helper ) ) { return; }
	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[2], &helper ) ) { return; }

	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[0], &helper ) ) { return; }
	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[1], &helper ) ) { return; }
	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[2], &helper ) ) { return; }

	// Test against the triangle face plane.
	if ( !FacePlane( ray, rayDir, pTri, &helper ) )
		return;

	if ( ( helper.m_flStartFrac < helper.m_flEndFrac ) || ( FloatMakePositive( helper.m_flStartFrac - helper.m_flEndFrac ) < 0.001f ) )
	{
		if ( ( helper.m_flStartFrac != DISPCOLL_INVALID_FRAC ) && ( helper.m_flStartFrac < pTrace->fraction ) )
		{
			// Clamp -- shouldn't really ever be here!???
			if ( helper.m_flStartFrac < 0.0f )
			{
				helper.m_flStartFrac = 0.0f;
			}
			
			pTrace->fraction = helper.m_flStartFrac;
			VectorCopy( helper.m_vecImpactNormal, pTrace->plane.normal );
			pTrace->plane.dist = helper.m_flImpactDist;
			pTrace->dispFlags = pTri->m_uiFlags;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CDispCollTree::CDispCollTree()
{
	m_nPower = 0;
	m_nFlags = 0;

	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		m_vecSurfPoints[iPoint].Init();
	}
	m_nContents = -1;
	m_nSurfaceProps[0] = 0;
	m_nSurfaceProps[1] = 0;

	m_vecStabDir.Init();
	m_mins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	m_iCounter = 0;

	m_aVerts.Purge();
	m_aTris.Purge();
	m_aEdgePlanes.Purge();
#ifdef ENGINE_DLL
	m_hCache = INVALID_MEMHANDLE;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: deconstructor
//-----------------------------------------------------------------------------
CDispCollTree::~CDispCollTree()
{	
#ifdef ENGINE_DLL
	if ( m_hCache != INVALID_MEMHANDLE )
		g_DispCollTriCache.DestroyResource( m_hCache );
#endif
	m_aVerts.Purge();
	m_aTris.Purge();
	m_aEdgePlanes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Create( CCoreDispInfo *pDisp )
{
	// Create the AABB Tree.
	return AABBTree_Create( pDisp );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::PointInBounds( const Vector &vecBoxCenter, const Vector &vecBoxMin, 
								   const Vector &vecBoxMax, bool bPoint )
{
	// Point test inside bounds.
	if( bPoint )
	{
		return IsPointInBox( vecBoxCenter, m_mins, m_maxs );
	}
	
	// Box test inside bounds
	Vector vecExtents;
	VectorSubtract( vecBoxMax, vecBoxMin, vecExtents );
	vecExtents *= 0.5f;

	Vector vecExpandBounds[2];
	vecExpandBounds[0] = m_mins - vecExtents;
	vecExpandBounds[1] = m_maxs + vecExtents;

	return IsPointInBox( vecBoxCenter, vecExpandBounds[0], vecExpandBounds[1] );
}

void CDispCollTree::GetVirtualMeshList( virtualmeshlist_t *pList )
{
	int i;
	int triangleCount = GetTriSize();
	pList->indexCount = triangleCount * 3;
	pList->triangleCount = triangleCount;
	pList->vertexCount = m_aVerts.Count();
	pList->pVerts = m_aVerts.Base();
	pList->pHull = NULL;
	pList->surfacePropsIndex = GetSurfaceProps(0);
	int index = 0;
	for ( i = 0 ; i < triangleCount; i++ )
	{
		pList->indices[index+0] = m_aTris[i].GetVert(0);
		pList->indices[index+1] = m_aTris[i].GetVert(1);
		pList->indices[index+2] = m_aTris[i].GetVert(2);
		index += 3;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef ENGINE_DLL
static int g_nTrees;
#endif
CDispCollTree *DispCollTrees_Alloc( int count )
{
	CDispCollTree *pTrees = NULL;
#ifdef ENGINE_DLL
	pTrees = (CDispCollTree *)Hunk_AllocName( count * sizeof(CDispCollTree), "DispCollTrees_Alloc", false );
	g_nTrees = count;
	for ( int i = 0; i < g_nTrees; i++ )
	{
		Construct( pTrees + i );
	}
#else
	pTrees = new CDispCollTree[count];
#endif
	if( !pTrees )
		return NULL;

	for ( int i = 0; i < count; i++ )
	{
		pTrees[i].m_iCounter = i;
	}
	return pTrees;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispCollTrees_Free( CDispCollTree *pTrees )
{
#ifdef ENGINE_DLL
	for ( int i = 0; i < g_nTrees; i++ )
	{
		Destruct( pTrees + i );
	}
	g_nTrees = 0;
#else
	if( pTrees )
	{
		delete [] pTrees;
		pTrees = NULL;
	}
#endif
}
