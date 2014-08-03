//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISPCOLL_COMMON_H
#define DISPCOLL_COMMON_H
#pragma once

#include "trace.h"
#include "builddisp.h"
#include "bitvec.h"
#ifdef ENGINE_DLL
#include "../engine/zone.h"
#endif

#ifdef ENGINE_DLL
template<typename T>
class CDispVector : public CUtlVector<T, CHunkMemory<T> >
{
};
#else
template<typename T>
class CDispVector : public CUtlVector<T, CUtlMemoryAligned<T,16> >
{
};
#endif

FORWARD_DECLARE_HANDLE( memhandle_t );

#define DISPCOLL_TREETRI_SIZE		MAX_DISPTRIS
#define DISPCOLL_DIST_EPSILON		0.03125f
#define DISPCOLL_ROOTNODE_INDEX		0
#define DISPCOLL_INVALID_TRI		-1
#define DISPCOLL_INVALID_FRAC		-99999.9f
#define DISPCOLL_NORMAL_UNDEF		0xffff

extern double g_flDispCollSweepTimer;
extern double g_flDispCollIntersectTimer;
extern double g_flDispCollInCallTimer;

struct RayDispOutput_t
{
	short	ndxVerts[4];	// 3 verts and a pad
	float	u, v;			// the u, v paramters (edgeU = v1 - v0, edgeV = v2 - v0)
	float	dist;			// intersection distance
};

// Assumptions:
//	Max patch is 17x17, therefore 9 bits needed to represent a triangle index
// 

//=============================================================================
//	Displacement Collision Triangle
class CDispCollTri
{

	struct index_t
	{
		union
		{
			struct
			{
				unsigned short uiVert:9;
				unsigned short uiMin:2;
				unsigned short uiMax:2;
			} m_Index;
			
			unsigned short m_IndexDummy;
		};
	};

	index_t				m_TriData[3];

public:
	unsigned short		m_ucSignBits:3;			// Plane test.
	unsigned short		m_ucPlaneType:3;		// Axial test?
	unsigned short		m_uiFlags:5;			// Uses 5-bits - maybe look into merging it with something?

	Vector				m_vecNormal;			// Triangle normal (plane normal).
	float				m_flDist;				// Triangle plane dist.

	// Creation.
	     CDispCollTri();
	void Init( void );
	void CalcPlane( CDispVector<Vector> &m_aVerts );
	void FindMinMax( CDispVector<Vector> &m_aVerts );

	// Triangle data.
	inline void SetVert( int iPos, int iVert )			{ Assert( ( iPos >= 0 ) && ( iPos < 3 ) ); Assert( ( iVert >= 0 ) && ( iVert < ( 1 << 9 ) ) ); m_TriData[iPos].m_Index.uiVert = iVert; }
	inline int  GetVert( int iPos ) const				{ Assert( ( iPos >= 0 ) && ( iPos < 3 ) ); return m_TriData[iPos].m_Index.uiVert; }
	inline void SetMin( int iAxis, int iMin )			{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); Assert( ( iMin >= 0 ) && ( iMin < 3 ) ); m_TriData[iAxis].m_Index.uiMin = iMin; }
	inline int  GetMin( int iAxis )	const				{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); return m_TriData[iAxis].m_Index.uiMin; }
	inline void SetMax( int iAxis, int iMax )			{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); Assert( ( iMax >= 0 ) && ( iMax < 3 ) ); m_TriData[iAxis].m_Index.uiMax = iMax; }
	inline int  GetMax( int iAxis ) const				{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); return m_TriData[iAxis].m_Index.uiMax; }
};

//=============================================================================
//	Helper
class CDispCollHelper
{
public:

	float	m_flStartFrac;
	float	m_flEndFrac;
	Vector	m_vecImpactNormal;
	float	m_flImpactDist;
};

//=============================================================================
//	Cache
#pragma pack(1)
class CDispCollTriCache
{
public:
	unsigned short m_iCrossX[3];
	unsigned short m_iCrossY[3];
	unsigned short m_iCrossZ[3];
};
#pragma pack()
#include "mathlib/ssemath.h"

class CDispCollNode
{
public:
	FourVectors m_mins;
	FourVectors m_maxs;
};

class CDispCollLeaf
{
public:
	short	m_tris[2];
};

// a power 4 displacement can have 341 nodes, pad out to 344 for 16-byte alignment
const int MAX_DISP_AABB_NODES = 341;
const int MAX_AABB_LIST = 344;

struct rayleaflist_t
{
	FourVectors rayStart;
	FourVectors rayExtents;
	FourVectors invDelta;
	int nodeList[MAX_AABB_LIST];
	int maxIndex;
};

//=============================================================================
//
// Displacement Collision Tree Data
//
class CDispCollTree
{
public:

	// Creation/Destruction.
	CDispCollTree();
	~CDispCollTree();
	virtual bool Create( CCoreDispInfo *pDisp );

	// Raycasts.
	// NOTE: These assume you've precalculated invDelta as well as culled to the bounds of this disp
	bool AABBTree_Ray( const Ray_t &ray, const Vector &invDelta, CBaseTrace *pTrace, bool bSide = true );
	bool AABBTree_Ray( const Ray_t &ray, const Vector &invDelta, RayDispOutput_t &output );
	// NOTE: Lower perf helper function, should not be used in the game runtime
	bool AABBTree_Ray( const Ray_t &ray, RayDispOutput_t &output );

	// Hull Sweeps.
	// NOTE: These assume you've precalculated invDelta as well as culled to the bounds of this disp
	bool AABBTree_SweepAABB( const Ray_t &ray, const Vector &invDelta, CBaseTrace *pTrace );

	// Hull Intersection.
	bool AABBTree_IntersectAABB( const Vector &absMins, const Vector &absMaxs );

	// Point/Box vs. Bounds.
	bool PointInBounds( Vector const &vecBoxCenter, Vector const &vecBoxMin, Vector const &vecBoxMax, bool bPoint );

	// Utility.
	inline void SetPower( int power )								{ m_nPower = power; }
	inline int GetPower( void )										{ return m_nPower; }

	inline int	GetFlags( void )									{ return m_nFlags; }
	inline void SetFlags( int nFlags )								{ m_nFlags = nFlags; }
	inline bool CheckFlags( int nFlags )							{ return ( ( nFlags & GetFlags() ) != 0 ) ? true : false; }

	inline int GetWidth( void )										{ return ( ( 1 << m_nPower ) + 1 ); }
	inline int GetHeight( void )									{ return ( ( 1 << m_nPower ) + 1 ); }
	inline int GetSize( void )										{ return ( ( 1 << m_nPower ) + 1 ) * ( ( 1 << m_nPower ) + 1 ); }
	inline int GetTriSize( void )									{ return ( ( 1 << m_nPower ) * ( 1 << m_nPower ) * 2 ); }

//	inline void SetTriFlags( short iTri, unsigned short nFlags )	{ m_aTris[iTri].m_uiFlags = nFlags; }

	inline void GetStabDirection( Vector &vecDir )					{ vecDir = m_vecStabDir; }

	inline void GetBounds( Vector &vecBoxMin, Vector &vecBoxMax )	{ vecBoxMin = m_mins; vecBoxMax = m_maxs; }
	inline int GetContents( void )									{ return m_nContents; }
	inline void SetSurfaceProps( int iProp, short nSurfProp )		{ Assert( ( iProp >= 0 ) && ( iProp < 2 ) ); m_nSurfaceProps[iProp] = nSurfProp; }
	inline short GetSurfaceProps( int iProp )						{ return m_nSurfaceProps[iProp]; }

	inline unsigned int GetMemorySize( void )						{ return m_nSize; }
	inline unsigned int GetCacheMemorySize( void )					{ return ( m_aTrisCache.Count() * sizeof(CDispCollTriCache) + m_aEdgePlanes.Count() * sizeof(Vector) ); }

	inline bool IsCached( void )									{ return m_aTrisCache.Count() == m_aTris.Count(); }

	void GetVirtualMeshList( struct virtualmeshlist_t *pList );
	int AABBTree_GetTrisInSphere( const Vector &center, float radius, unsigned short *pIndexOut, int indexMax );

public:

	inline int Nodes_GetChild( int iNode, int nDirection );
	inline int Nodes_CalcCount( int nPower );
	inline int Nodes_GetParent( int iNode );
	inline int Nodes_GetLevel( int iNode );
	inline int Nodes_GetIndexFromComponents( int x, int y );

	void LockCache();
	void UnlockCache();
	void Cache( void );
	void Uncache()	{ m_aTrisCache.Purge(); m_aEdgePlanes.Purge(); }

#ifdef ENGINE_DLL
	// Data manager methods
	static size_t EstimatedSize( CDispCollTree *pTree )
	{
		return pTree->GetCacheMemorySize();
	}

	static CDispCollTree *CreateResource( CDispCollTree *pTree )
	{
		// Created ahead of time
		return pTree;
	}

	bool GetData()
	{
		return IsCached();
	}

	size_t Size()
	{
		return GetCacheMemorySize();
	}

	void DestroyResource()
	{
		Uncache();
		m_hCache = NULL;
	}
#endif

protected:

	bool AABBTree_Create( CCoreDispInfo *pDisp );
	void AABBTree_CopyDispData( CCoreDispInfo *pDisp );
	void AABBTree_CreateLeafs( void );
	void AABBTree_GenerateBoxes_r( int nodeIndex, Vector *pMins, Vector *pMaxs );
	void AABBTree_CalcBounds( void );

	int AABBTree_BuildTreeTrisInSphere_r( const Vector &center, float radius, int iNode, unsigned short *pIndexOut, unsigned short indexMax );

	void AABBTree_TreeTrisRayTest( const Ray_t &ray, const Vector &vecInvDelta, int iNode, CBaseTrace *pTrace, bool bSide, CDispCollTri **pImpactTri );
	void AABBTree_TreeTrisRayBarycentricTest( const Ray_t &ray, const Vector &vecInvDelta, int iNode, RayDispOutput_t &output, CDispCollTri **pImpactTri );

	int FORCEINLINE BuildRayLeafList( int iNode, rayleaflist_t &list );

	struct AABBTree_TreeTrisSweepTest_Args_t
	{
		AABBTree_TreeTrisSweepTest_Args_t( const Ray_t &ray, const Vector &vecInvDelta, const Vector &rayDir, CBaseTrace *pTrace )
			: ray( ray ), vecInvDelta( vecInvDelta ), rayDir( rayDir ), pTrace( pTrace ) {}
		const Ray_t &ray;
		const Vector &vecInvDelta;
		const Vector &rayDir;
		CBaseTrace *pTrace;
	};

protected:

	void SweepAABBTriIntersect( const Ray_t &ray, const Vector &rayDir, int iTri, CDispCollTri *pTri, CBaseTrace *pTrace );

	void Cache_Create( CDispCollTri *pTri, int iTri );		// Testing!
	bool Cache_EdgeCrossAxisX( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );
	bool Cache_EdgeCrossAxisY( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );
	bool Cache_EdgeCrossAxisZ( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );

	inline bool FacePlane( const Ray_t &ray, const Vector &rayDir, CDispCollTri *pTri, CDispCollHelper *pHelper );
	bool FORCEINLINE AxisPlanesXYZ( const Ray_t &ray, CDispCollTri *pTri, CDispCollHelper *pHelper );
	inline bool EdgeCrossAxisX( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper );
	inline bool EdgeCrossAxisY( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper );
	inline bool EdgeCrossAxisZ( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper );

	bool ResolveRayPlaneIntersect( float flStart, float flEnd, const Vector &vecNormal, float flDist, CDispCollHelper *pHelper );
	template <int AXIS> bool FORCEINLINE TestOneAxisPlaneMin( const Ray_t &ray, CDispCollTri *pTri );
	template <int AXIS> bool FORCEINLINE TestOneAxisPlaneMax( const Ray_t &ray, CDispCollTri *pTri );
	template <int AXIS>	bool EdgeCrossAxis( const Ray_t &ray, unsigned short iPlane, CDispCollHelper *pHelper );

	// Utility
	inline void CalcClosestBoxPoint( const Vector &vecPlaneNormal, const Vector &vecBoxStart, const Vector &vecBoxExtents, Vector &vecBoxPoint );
	inline void CalcClosestExtents( const Vector &vecPlaneNormal, const Vector &vecBoxExtents, Vector &vecBoxPoint );
	int AddPlane( const Vector &vecNormal );
	bool FORCEINLINE IsLeafNode(int iNode);
public:
	Vector							m_mins;									// Bounding box of the displacement surface and base face
	int								m_iCounter;
	Vector							m_maxs;									// Bounding box of the displacement surface and base face
protected:
	int								m_nContents;							// The displacement surface "contents" (solid, etc...)

#ifdef ENGINE_DLL
	memhandle_t						m_hCache;
#endif

	int								m_nPower;								// Size of the displacement ( 2^power + 1 )
	int								m_nFlags;

	Vector							m_vecSurfPoints[4];						// Base surface points.
	// Collision data.
	Vector							m_vecStabDir;							// Direction to stab for this displacement surface (is the base face normal)
	short							m_nSurfaceProps[2];						// Surface properties (save off from texdata for impact responses)

protected:
	CDispVector<Vector>				m_aVerts;								// Displacement verts.
	CDispVector<CDispCollTri>		m_aTris;								// Displacement triangles.
	CDispVector<CDispCollNode>		m_nodes;					// Nodes.
	CDispVector<CDispCollLeaf>		m_leaves;								// Leaves.
	// Cache
	CUtlVector<CDispCollTriCache>	m_aTrisCache;
	CUtlVector<Vector> m_aEdgePlanes;

	CDispCollHelper					m_Helper;

	unsigned int					m_nSize;

};

FORCEINLINE bool CDispCollTree::IsLeafNode(int iNode) 
{ 
	return iNode >= m_nodes.Count() ? true : false; 
}

//-----------------------------------------------------------------------------
// Purpose: get the child node index given the current node index and direction
//          of the child (1 of 4)
//   Input: iNode - current node index
//          nDirection - direction of the child ( [0...3] - SW, SE, NW, NE )
//  Output: int - the index of the child node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetChild( int iNode, int nDirection )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_nodes.Count() );

    // ( node index * 4 ) + ( direction + 1 )
    return ( ( iNode << 2 ) + ( nDirection + 1 ) );	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_CalcCount( int nPower )
{ 
	Assert( nPower >= 1 );
	Assert( nPower <= 4 );

	return ( ( 1 << ( ( nPower + 1 ) << 1 ) ) / 3 ); 
}

//-----------------------------------------------------------------------------
// Purpose: get the parent node index given the current node
//   Input: iNode - current node index
//  Output: int - the index of the parent node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetParent( int iNode )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_nodes.Count() );

	// ( node index - 1 ) / 4
	return ( ( iNode - 1 ) >> 2 );
}

//-----------------------------------------------------------------------------
// Purpose:
// TODO: should make this a function - not a hardcoded set of statements!!!
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetLevel( int iNode )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_nodes.Count() );

	// level = 2^n + 1
	if ( iNode == 0 )  { return 1; }
	if ( iNode < 5 )   { return 2; }
	if ( iNode < 21 )  { return 3; }
	if ( iNode < 85 )  { return 4; }
	if ( iNode < 341 ) { return 5; }

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetIndexFromComponents( int x, int y )
{
	int nIndex = 0;

	// Interleave bits from the x and y values to create the index
	int iShift;
	for( iShift = 0; x != 0; iShift += 2, x >>= 1 )
	{
		nIndex |= ( x & 1 ) << iShift;
	}

	for( iShift = 1; y != 0; iShift += 2, y >>= 1 )
	{
		nIndex |= ( y & 1 ) << iShift;
	}

	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTree::CalcClosestBoxPoint( const Vector &vecPlaneNormal, const Vector &vecBoxStart, 
											    const Vector &vecBoxExtents, Vector &vecBoxPoint )
{
	vecBoxPoint = vecBoxStart;
	( vecPlaneNormal[0] < 0.0f ) ? vecBoxPoint[0] += vecBoxExtents[0] : vecBoxPoint[0] -= vecBoxExtents[0];
	( vecPlaneNormal[1] < 0.0f ) ? vecBoxPoint[1] += vecBoxExtents[1] : vecBoxPoint[1] -= vecBoxExtents[1];
	( vecPlaneNormal[2] < 0.0f ) ? vecBoxPoint[2] += vecBoxExtents[2] : vecBoxPoint[2] -= vecBoxExtents[2];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTree::CalcClosestExtents( const Vector &vecPlaneNormal, const Vector &vecBoxExtents, 
											   Vector &vecBoxPoint )
{
	( vecPlaneNormal[0] < 0.0f ) ? vecBoxPoint[0] = vecBoxExtents[0] : vecBoxPoint[0] = -vecBoxExtents[0];
	( vecPlaneNormal[1] < 0.0f ) ? vecBoxPoint[1] = vecBoxExtents[1] : vecBoxPoint[1] = -vecBoxExtents[1];
	( vecPlaneNormal[2] < 0.0f ) ? vecBoxPoint[2] = vecBoxExtents[2] : vecBoxPoint[2] = -vecBoxExtents[2];
}

//=============================================================================
// Global Helper Functions
CDispCollTree *DispCollTrees_Alloc( int count );
void DispCollTrees_Free( CDispCollTree *pTrees );

#endif // DISPCOLL_COMMON_H
