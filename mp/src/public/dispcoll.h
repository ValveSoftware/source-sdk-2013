//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISPCOLL_H
#define DISPCOLL_H
#pragma once

#include "mathlib/vector.h"

class CCoreDispInfo;

//=============================================================================
//
// Displacement Collision Triangle Data
//
class CDispCollTri
{
public:
	
	void Init( void );
	inline void SetPoint( int index, Vector const& vert );
	inline void SetPointNormal( int index, Vector const& normal );
	void CalcPlane( void );
	
	inline void SetIntersect( bool bIntersect );
	inline bool IsIntersect( void );

	Vector	m_Points[3];		// polygon points
	Vector  m_PointNormals[3];	// polygon point normals
	Vector	m_Normal;			// plane normal
	float	m_Distance;			// plane distance
	short	m_ProjAxes[2];		// projection axes (2 minor axes)
	bool	m_bIntersect;		// intersected triangle???
};	

//=============================================================================
//
// Displacement Collision Node Data
//
class CDispCollNode
{
public:
	
	CDispCollNode();
	inline bool IsLeaf( void );
	inline void SetBounds( Vector const &bMin, Vector const &bMax );
	inline void GetBounds( Vector &bMin, Vector &bMax );
	
	Vector			m_Bounds[2];		// node minimum and maximum

	bool			m_bIsLeaf;			// is the node a leaf? ( may have to make this an int for alignment!)
	CDispCollTri	m_Tris[2];			// two triangles contained in leaf node
};


//=============================================================================
//
// Displacement Collision Data
//
class CDispCollData
{
public:

	Vector	m_StartPos;
	Vector	m_EndPos;
	Vector  m_Extents;
	float	m_Fraction;
	int		m_Contents;
	Vector	m_Normal;
	float	m_Distance;
	bool	m_bOcclude;
};



// HACKHACK: JAY: Moved this out of CDispCollTree to be thread safe in vrad
enum { TRILIST_CACHE_SIZE = 128 };

class CDispCollTreeTempData
{
public:
	//
	// temps
	//
	int					m_TriListCount;
	CDispCollTri		*m_ppTriList[TRILIST_CACHE_SIZE];

	// collision tree node cache
	float				m_AABBDistances[6];
};


//=============================================================================
//
// Displacement Collision Tree
//
class CDispCollTree
{
public:

	static const float COLLISION_EPSILON;
	static const float ONE_MINUS_COLLISION_EPSILON;

	//=========================================================================
	//
	// Creation/Destruction
	//
	CDispCollTree();
	~CDispCollTree();

	virtual bool Create( CCoreDispInfo *pDisp );

	//=========================================================================
	//
	// Collision Functions
	//
	bool RayTest( CDispCollData *pData );
	bool RayTestAllTris( CDispCollData *pData, int power );

	bool AABBIntersect( CDispCollData *pData );
	bool AABBSweep( CDispCollData *pData );

	//=========================================================================
	//
	// Attrib Functions
	//
	inline void SetPower( int power );
	inline int GetPower( void );

	inline void SetCheckCount( int count );
	inline int GetCheckCount( void );

	inline void GetBounds( Vector& boundMin, Vector& boundMax );

protected:

	int					m_Power;

	int					m_NodeCount;
	CDispCollNode		*m_pNodes;

	int					m_CheckCount;

	// collision tree node cache
	Vector				m_AABBNormals[6];
	//=========================================================================
	//
	// Creation/Destruction
	//
	void InitAABBData( void );
	void InitLeaves( CCoreDispInfo *pDisp );
	void CreateNodes( CCoreDispInfo *pDisp );
	void CreateNodes_r( CCoreDispInfo *pDisp, int nodeIndex, int termLevel );
	void CalcBounds( CDispCollNode *pNode, int nodeIndex );

	//=========================================================================
	//
	// Collision Functions
	//
	void CreatePlanesFromBounds( CDispCollTreeTempData *pTemp, Vector const &bbMin, Vector const &bbMax );

//	void RayNodeTest_r( int nodeIndex, Vector &rayStart, Vector &rayEnd );
	void RayNodeTest_r( CDispCollTreeTempData *pTemp, int nodeIndex, Vector rayStart, Vector rayEnd );
	bool RayAABBTest( CDispCollTreeTempData *pTemp, Vector &rayStart, Vector &rayEnd );
	bool RayTriListTest( CDispCollTreeTempData *pTemp, CDispCollData *pData );
	bool RayTriTest( Vector const &rayStart, Vector const &rayDir, float const rayLength, CDispCollTri const *pTri, float *fraction );

	void BuildTriList_r( CDispCollTreeTempData *pTemp, int nodeIndex, Vector &rayStart, Vector &rayEnd, Vector &extents, bool bIntersect );
	bool IntersectAABBAABBTest( CDispCollTreeTempData *pTemp, const Vector &pos, const Vector &extents );
	bool SweptAABBAABBTest( CDispCollTreeTempData *pTemp, const Vector &rayStart, const Vector &rayEnd, const Vector &extents );

	bool CullTriList( CDispCollTreeTempData *pTemp, Vector &rayStart, Vector &rayEnd, Vector &extents, bool bIntersect );
	bool SweptAABBTriTest( Vector &rayStart, Vector &rayEnd, Vector &extents, CDispCollTri const *pTri );
	bool AABBTriIntersect( CDispCollTreeTempData *pTemp, CDispCollData *pData );
	bool IntersectAABBTriTest( Vector &rayStart, Vector &extents, CDispCollTri const *pTri );
	bool SweptAABBTriIntersect( Vector &rayStart, Vector &rayEnd, Vector &extents,
						        CDispCollTri const *pTri, Vector &plNormal, float *plDist,
								float *fraction );

	//=========================================================================
	//
	// Memory Functions
	//
	bool AllocNodes( int nodeCount );
	void FreeNodes( void );

	//=========================================================================
	//
	// Utility Functions
	//
	inline int CalcNodeCount( int power );
	inline int GetParentNode( int nodeIndex );
	inline int GetChildNode( int nodeIndex, int direction );
	inline int GetNodeLevel( int nodeIndex );
	int GetNodeIndexFromComponents( int x, int y );
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::SetPower( int power )
{
	m_Power = power;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetPower( void )
{
	return m_Power;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::SetCheckCount( int count )
{
	m_CheckCount = count;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetCheckCount( void )
{
	return m_CheckCount;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::GetBounds( Vector& boundMin, Vector& boundMax )
{
	boundMin[0] = m_pNodes[0].m_Bounds[0].x;
	boundMin[1] = m_pNodes[0].m_Bounds[0].y;
	boundMin[2] = m_pNodes[0].m_Bounds[0].z;

	boundMax[0] = m_pNodes[0].m_Bounds[1].x;
	boundMax[1] = m_pNodes[0].m_Bounds[1].y;
	boundMax[2] = m_pNodes[0].m_Bounds[1].z;
}


#endif // DISPCOLL_H