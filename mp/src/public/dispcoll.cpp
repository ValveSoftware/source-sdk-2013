//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BuildDisp.h"
#include "DispColl.h"
#include "tier0/dbg.h"

//=============================================================================

const float CDispCollTree::COLLISION_EPSILON = 0.01f;
const float CDispCollTree::ONE_MINUS_COLLISION_EPSILON = 1.0f - COLLISION_EPSILON;

//=============================================================================
//
// Displacement Collision Triangle Functions
//


//-----------------------------------------------------------------------------
// Purpose: initialize the displacement triangles
//-----------------------------------------------------------------------------
void CDispCollTri::Init( void )
{
	for( int i = 0; i < 3; i++ )
	{
		m_Points[i].x = 0.0f;  m_Points[i].y = 0.0f;  m_Points[i].z = 0.0f;
		m_PointNormals[i].x = 0.0f;  m_PointNormals[i].y = 0.0f;  m_PointNormals[i].z = 0.0f;
	}

	m_Normal.x = 0.0f;  m_Normal.y = 0.0f;  m_Normal.z = 0.0f;
	m_Distance = 0.0f;

	m_ProjAxes[0] = -1;
	m_ProjAxes[1] = -1;

	m_bIntersect = false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTri::SetPoint( int index, Vector const &vert )
{
	Assert( index >= 0 );
	Assert( index < 3 );

	m_Points[index].x = vert[0];
	m_Points[index].y = vert[1];
	m_Points[index].z = vert[2];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTri::SetPointNormal( int index, Vector const &normal )
{
	Assert( index >= 0 );
	Assert( index < 3 );

	m_PointNormals[index].x = normal[0];
	m_PointNormals[index].y = normal[1];
	m_PointNormals[index].z = normal[2];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::CalcPlane( void )
{
	//
	// calculate the plane normal and distance
	//
	Vector segment1, segment2, cross;

	segment1 = m_Points[1] - m_Points[0];
	segment2 = m_Points[2] - m_Points[0];
	cross = segment1.Cross( segment2 );
	m_Normal = cross;
	VectorNormalize(m_Normal);

	m_Distance = m_Normal.Dot( m_Points[0] );

	//
	// calculate the projection axes
	//
    if( FloatMakePositive( m_Normal[0] ) > FloatMakePositive( m_Normal[1] ) )
	{
		if( FloatMakePositive( m_Normal[0] ) > FloatMakePositive( m_Normal[2] ) )
		{
			m_ProjAxes[0] = 1;
			m_ProjAxes[1] = 2;
		}
		else
		{
			m_ProjAxes[0] = 0;
			m_ProjAxes[1] = 1;
		}
	}
	else
	{
		if( FloatMakePositive( m_Normal[1] ) > FloatMakePositive( m_Normal[2] ) )
		{
			m_ProjAxes[0] = 0;
			m_ProjAxes[1] = 2;
		}
		else
		{
			m_ProjAxes[0] = 0;
			m_ProjAxes[1] = 1;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTri::SetIntersect( bool bIntersect )
{
	m_bIntersect = bIntersect;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTri::IsIntersect( void )
{
	return m_bIntersect;
}


//=============================================================================
//
// Displacement Collision Node Functions
//


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CDispCollNode::CDispCollNode()
{
	m_Bounds[0].x = m_Bounds[0].y = m_Bounds[0].z = 99999.9f;
	m_Bounds[1].x = m_Bounds[1].y = m_Bounds[1].z = -99999.9f;

	m_Tris[0].Init();
	m_Tris[1].Init();

	m_bIsLeaf = false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollNode::IsLeaf( void )
{
	return m_bIsLeaf;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollNode::SetBounds( Vector const &bMin, Vector const &bMax )
{
	m_Bounds[0] = bMin;
	m_Bounds[1] = bMax;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollNode::GetBounds( Vector &bMin, Vector &bMax )
{
	bMin = m_Bounds[0];
	bMax = m_Bounds[1];
}


//=============================================================================
//
// Displacement Collision Tree Functions
//

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CDispCollTree::CDispCollTree()
{
	m_Power = 0;

	m_NodeCount = 0;
	m_pNodes = NULL;

	InitAABBData();
}


//-----------------------------------------------------------------------------
// Purpose: deconstructor
//-----------------------------------------------------------------------------
CDispCollTree::~CDispCollTree()
{
	FreeNodes();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::InitAABBData( void )
{
	m_AABBNormals[0].x = -1.0f;  m_AABBNormals[0].y = 0.0f;   m_AABBNormals[0].z = 0.0f;
	m_AABBNormals[1].x = 1.0f;   m_AABBNormals[1].y = 0.0f;   m_AABBNormals[1].z = 0.0f;

	m_AABBNormals[2].x = 0.0f;   m_AABBNormals[2].y = -1.0f;  m_AABBNormals[2].z = 0.0f;
	m_AABBNormals[3].x = 0.0f;   m_AABBNormals[3].y = 1.0f;   m_AABBNormals[3].z = 0.0f;

	m_AABBNormals[4].x = 0.0f;   m_AABBNormals[4].y = 0.0f;  m_AABBNormals[4].z = -1.0f;
	m_AABBNormals[5].x = 0.0f;   m_AABBNormals[5].y = 0.0f;   m_AABBNormals[5].z = 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::CalcBounds( CDispCollNode *pNode, int nodeIndex )
{
	Vector bounds[2];
	bounds[0].Init( 99999.9f, 99999.9f, 99999.9f );
	bounds[1].Init( -99999.9f, -99999.9f, -99999.9f );

	//
	// handle leaves differently -- bounding volume defined by triangles
	//
	if( pNode->IsLeaf() )
	{
		for( int i = 0; i < 2; i++ )
		{
			for( int j = 0; j < 3; j++ )
			{
				//
				// minimum
				//
				if( bounds[0].x > pNode->m_Tris[i].m_Points[j].x ) { bounds[0].x = pNode->m_Tris[i].m_Points[j].x; } 
				if( bounds[0].y > pNode->m_Tris[i].m_Points[j].y ) { bounds[0].y = pNode->m_Tris[i].m_Points[j].y; } 
				if( bounds[0].z > pNode->m_Tris[i].m_Points[j].z ) { bounds[0].z = pNode->m_Tris[i].m_Points[j].z; } 

				//
				// maximum
				//
				if( bounds[1].x < pNode->m_Tris[i].m_Points[j].x ) { bounds[1].x = pNode->m_Tris[i].m_Points[j].x; } 
				if( bounds[1].y < pNode->m_Tris[i].m_Points[j].y ) { bounds[1].y = pNode->m_Tris[i].m_Points[j].y; } 
				if( bounds[1].z < pNode->m_Tris[i].m_Points[j].z ) { bounds[1].z = pNode->m_Tris[i].m_Points[j].z; } 
			}
		}
	}
	//
	// bounding volume defined by maxima and minima of children volumes
	//
	else
	{
		for( int i = 0; i < 4; i++ )
		{
			int childIndex = GetChildNode( nodeIndex, i );
			CDispCollNode *pChildNode = &m_pNodes[childIndex];

			Vector childBounds[2];
			pChildNode->GetBounds( childBounds[0], childBounds[1] );

			//
			// minimum
			//
			if( bounds[0].x > childBounds[0].x ) { bounds[0].x = childBounds[0].x; } 
			if( bounds[0].y > childBounds[0].y ) { bounds[0].y = childBounds[0].y; } 
			if( bounds[0].z > childBounds[0].z ) { bounds[0].z = childBounds[0].z; } 
			
			//
			// maximum
			//
			if( bounds[1].x < childBounds[1].x ) { bounds[1].x = childBounds[1].x; } 
			if( bounds[1].y < childBounds[1].y ) { bounds[1].y = childBounds[1].y; } 
			if( bounds[1].z < childBounds[1].z ) { bounds[1].z = childBounds[1].z; } 
		}
	}

	pNode->SetBounds( bounds[0], bounds[1] );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::CreateNodes_r( CCoreDispInfo *pDisp, int nodeIndex, int termLevel )
{
	int nodeLevel = GetNodeLevel( nodeIndex );
	
	//
	// terminating condition -- set node info (leaf or otherwise)
	//
	if( nodeLevel == termLevel )
	{
		CDispCollNode *pNode = &m_pNodes[nodeIndex];
		CalcBounds( pNode, nodeIndex );

		return;
	}

	//
	// recurse into children
	//
	for( int i = 0; i < 4; i++ )
	{
		CreateNodes_r( pDisp, GetChildNode( nodeIndex, i ), termLevel );
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::CreateNodes( CCoreDispInfo *pDisp )
{
	//
	// create all nodes in tree
	//
	int power = pDisp->GetPower() + 1;
    for( int level = power; level > 0; level-- )
    {
		CreateNodes_r( pDisp, 0 /* rootIndex */, level );
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CDispCollTree::GetNodeIndexFromComponents( int x, int y )
{
	int index = 0;

	// Interleave bits from the x and y values to create the index:
	
	for( int shift = 0; x != 0; shift += 2, x >>= 1 )
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
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::InitLeaves( CCoreDispInfo *pDisp )
{
	//
	// get power and width and displacement surface
	//
	int power = pDisp->GetPower();
	int width = pDisp->GetWidth();

	//
	// get leaf indices
	//
	int startIndex = CalcNodeCount( power - 1 );
	int endIndex = CalcNodeCount( power );

	for( int index = startIndex; index < endIndex; index++ )
	{
		//
		// create triangles at leaves
		//
		int x = ( index - startIndex ) % ( width - 1 );
		int y = ( index - startIndex ) / ( width - 1 );

		int nodeIndex = GetNodeIndexFromComponents( x, y );
		nodeIndex += startIndex;

		Vector vert;
		Vector normal;

		//
		// tri 1
		//
		pDisp->GetVert( x + ( y * width ), vert );
		pDisp->GetNormal( x + ( y * width ), normal );
		m_pNodes[nodeIndex].m_Tris[0].SetPoint( 0, vert );
		m_pNodes[nodeIndex].m_Tris[0].SetPointNormal( 0, normal );

		pDisp->GetVert( x + ( ( y + 1 ) * width ), vert );
		pDisp->GetNormal( x + ( ( y + 1 ) * width ), normal );
		m_pNodes[nodeIndex].m_Tris[0].SetPoint( 1, vert );
		m_pNodes[nodeIndex].m_Tris[0].SetPointNormal( 1, normal );

		pDisp->GetVert( ( x + 1 ) + ( y * width ), vert );
		pDisp->GetNormal( ( x + 1 ) + ( y * width ), normal );
		m_pNodes[nodeIndex].m_Tris[0].SetPoint( 2, vert );
		m_pNodes[nodeIndex].m_Tris[0].SetPointNormal( 2, normal );

		m_pNodes[nodeIndex].m_Tris[0].CalcPlane();

		//
		// tri 2
		//
		pDisp->GetVert( ( x + 1 ) + ( y * width ), vert );
		pDisp->GetNormal( ( x + 1 ) + ( y * width ), normal );
		m_pNodes[nodeIndex].m_Tris[1].SetPoint( 0, vert );
		m_pNodes[nodeIndex].m_Tris[1].SetPointNormal( 0, normal );

		pDisp->GetVert( x + ( ( y + 1 ) * width ), vert );
		pDisp->GetNormal( x + ( ( y + 1 ) * width ), normal );
		m_pNodes[nodeIndex].m_Tris[1].SetPoint( 1, vert );
		m_pNodes[nodeIndex].m_Tris[1].SetPointNormal( 1, normal );

		pDisp->GetVert( ( x + 1 ) + ( ( y + 1 ) * width ), vert );
		pDisp->GetNormal( ( x + 1 ) + ( ( y + 1 ) * width ), normal );
		m_pNodes[nodeIndex].m_Tris[1].SetPoint( 2, vert );
		m_pNodes[nodeIndex].m_Tris[1].SetPointNormal( 2, normal );

		m_pNodes[nodeIndex].m_Tris[1].CalcPlane();

		// set node as leaf
		m_pNodes[nodeIndex].m_bIsLeaf = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: allocate and initialize the displacement collision tree
//   Input: power - size of the displacement surface
//  Output: bool - success? (true/false)
//-----------------------------------------------------------------------------
bool CDispCollTree::Create( CCoreDispInfo *pDisp )
{
	//
	// calculate the number of nodes needed given the size of the displacement
	//
	m_Power = pDisp->GetPower();
	m_NodeCount = CalcNodeCount( m_Power );

	//
	// allocate tree space
	//
	if( !AllocNodes( m_NodeCount ) )
		return false;	
	
	// initialize leaves
	InitLeaves( pDisp );

	// create tree nodes
	CreateNodes( pDisp );

	// tree successfully created!
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: allocate memory for the displacement collision tree
//   Input: nodeCount - number of nodes to allocate
//  Output: bool - success? (true/false)
//-----------------------------------------------------------------------------
bool CDispCollTree::AllocNodes( int nodeCount )
{
	// sanity check
	Assert( nodeCount != 0 );

	m_pNodes = new CDispCollNode[nodeCount];
	if( !m_pNodes )
		return false;

	// tree successfully allocated!
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: release allocated memory for displacement collision tree
//-----------------------------------------------------------------------------
void CDispCollTree::FreeNodes( void )
{
	if( m_pNodes )
	{
		delete [] m_pNodes;
		m_pNodes = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: calculate the number of tree nodes given the size of the 
//          displacement surface
//   Input: power - size of the displacement surface
//  Output: int - the number of tree nodes
//-----------------------------------------------------------------------------
inline int CDispCollTree::CalcNodeCount( int power )
{
	// power range [2...4]
	Assert( power > 0 );
	Assert( power < 5 );

	return ( ( 1 << ( ( power + 1 ) << 1 ) ) / 3 );
}


//-----------------------------------------------------------------------------
// Purpose: get the parent node index given the current node
//   Input: nodeIndex - current node index
//  Output: int - the index of the parent node
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetParentNode( int nodeIndex )
{
	// node range [0...m_NodeCount)
	Assert( nodeIndex >= 0 );
	Assert( nodeIndex < m_NodeCount );

	// ( nodeIndex - 1 ) / 4
	return ( ( nodeIndex - 1 ) >> 2 );
}


//-----------------------------------------------------------------------------
// Purpose: get the child node index given the current node index and direction
//          of the child (1 of 4)
//   Input: nodeIndex - current node index
//          direction - direction of the child ( [0...3] - SW, SE, NW, NE )
//  Output: int - the index of the child node
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetChildNode( int nodeIndex, int direction )
{
	// node range [0...m_NodeCount)
	Assert( nodeIndex >= 0 );
	Assert( nodeIndex < m_NodeCount );

    // ( nodeIndex * 4 ) + ( direction + 1 )
    return ( ( nodeIndex << 2 ) + ( direction + 1 ) );	
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetNodeLevel( int nodeIndex )
{
	// node range [0...m_NodeCount)
	Assert( nodeIndex >= 0 );
	Assert( nodeIndex < m_NodeCount );

	// level = 2^n + 1
	if( nodeIndex == 0 )  { return 1; }
	if( nodeIndex < 5 )   { return 2; }
	if( nodeIndex < 21 )  { return 3; }
	if( nodeIndex < 85 )  { return 4; }
	if( nodeIndex < 341 ) { return 5; }

	return -1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTriTest( Vector const &rayStart, Vector const &rayDir, float const rayLength,
							    CDispCollTri const *pTri, float *fraction )
{
	const float DET_EPSILON = 0.001f;
	const float DIST_EPSILON = 0.001f;

	//
	// calculate the edges
	//
	Vector edge1 = pTri->m_Points[1] - pTri->m_Points[0];
	Vector edge2 = pTri->m_Points[2] - pTri->m_Points[0];

//	Vector faceNormal = edge1.Cross( edge2 );
//	Vector normNormal = faceNormal.Normalize();

	//
	// calculate the triangle's determinant
	//
	Vector pVec = rayDir.Cross( edge2 );
	float det = pVec.Dot( edge1 );

	// if determinant is zero -- ray lies in plane
	if( ( det > -DET_EPSILON ) && ( det < DET_EPSILON ) ) 
		return false;

	//
	// utility calculations - inverse determinant and distance from v0 to ray start
	//
	double invDet = 1.0f / det;
	Vector tVec = rayStart - pTri->m_Points[0];

	//
	// calculate the U parameter and test bounds
	//
	double u = pVec.Dot( tVec ) * invDet;
	if( ( u < 0.0f ) || ( u > 1.0f ) )
		return false;

	Vector qVec = tVec.Cross( edge1 );

	//
	// calculate the V parameter and test bounds
	//
	double v = qVec.Dot( rayDir ) * invDet;
	if( ( v < 0.0f ) || ( ( u + v ) > 1.0f ) )
		return false;

	// calculate where ray intersects triangle
	*fraction = qVec.Dot( edge2 ) * invDet;
	*fraction /= rayLength;

	if( ( *fraction < DIST_EPSILON ) || ( *fraction > ( 1.0f - DIST_EPSILON ) ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTriListTest( CDispCollTreeTempData *pTemp, CDispCollData *pData )
{
	// save starting fraction -- to test for collision
	float startFraction = pData->m_Fraction;

	//
	// calculate the ray
	//
	Vector seg = pData->m_EndPos - pData->m_StartPos;
	Vector rayDir = seg;
	float rayLength = VectorNormalize( rayDir );

	//
	// test ray against all triangles in list
	//
	for( int i = 0; i < pTemp->m_TriListCount; i++ )
	{
		float fraction = 1.0f;
		bool bResult = RayTriTest( pData->m_StartPos, rayDir, rayLength, pTemp->m_ppTriList[i], &fraction );
		if( !bResult )
			continue;

		if( pData->m_bOcclude )
		{
			return true;
		}

		if( fraction < pData->m_Fraction )
		{
			pData->m_Fraction = fraction;
			pData->m_Normal = pTemp->m_ppTriList[i]->m_Normal;
			pData->m_Distance = pTemp->m_ppTriList[i]->m_Distance;
		}
	}

	// collision!
	if( pData->m_Fraction < startFraction )
		return true;

	// no collision!
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayAABBTest( CDispCollTreeTempData *pTemp, Vector &rayStart, Vector &rayEnd )
{
	const float MY_DIST_EPSILON = 0.01f;

	for( int i = 0; i < 6; i++ )
	{
		float dist1 = m_AABBNormals[i].Dot( rayStart ) - pTemp->m_AABBDistances[i];
		float dist2 = m_AABBNormals[i].Dot( rayEnd ) - pTemp->m_AABBDistances[i];

		//
		// entry intersection point - move ray start up to intersection
		//
		if( ( dist1 > MY_DIST_EPSILON ) && ( dist2 < -MY_DIST_EPSILON ) )
		{
			float fraction = ( dist1 / ( dist1 - dist2 ) );

			Vector segment, increment;
			segment = ( rayEnd - rayStart ) * fraction;
			increment = segment;
			VectorNormalize(increment);
			segment += increment;
			rayStart += segment;
		}
		else if( ( dist1 > MY_DIST_EPSILON ) && ( dist2 > MY_DIST_EPSILON ) )
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::CreatePlanesFromBounds( CDispCollTreeTempData *pTemp, Vector const &bbMin, Vector const &bbMax )
{
	//
	// note -- these never change!
	//
//	m_AABBNormals[0].x = -1;
//	m_AABBNormals[1].x = 1;

//	m_AABBNormals[2].y = -1;
//	m_AABBNormals[3].y = 1;

//	m_AABBNormals[4].z = -1;
//	m_AABBNormals[5].z = 1;

	pTemp->m_AABBDistances[0] = -bbMin.x;	
	pTemp->m_AABBDistances[1] = bbMax.x;	
	
	pTemp->m_AABBDistances[2] = -bbMin.y;	
	pTemp->m_AABBDistances[3] = bbMax.y;	

	pTemp->m_AABBDistances[4] = -bbMin.z;	
	pTemp->m_AABBDistances[5] = bbMax.z;	
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::RayNodeTest_r( CDispCollTreeTempData *pTemp, int nodeIndex, Vector rayStart, Vector rayEnd )
{
	// get the current node
	CDispCollNode *pNode = &m_pNodes[nodeIndex];

	//
	// get node bounding box and create collision planes
	//
	Vector bounds[2];
	pNode->GetBounds( bounds[0], bounds[1] );
	CreatePlanesFromBounds( pTemp, bounds[0], bounds[1] );

	bool bIntersect = RayAABBTest( pTemp, rayStart, rayEnd );
	if( bIntersect )
	{
		// done -- add triangles to triangle list
		if( pNode->IsLeaf() )
		{
			// Assert for now -- flush cache later!!!!!
			Assert( pTemp->m_TriListCount >= 0 );
			Assert( pTemp->m_TriListCount < TRILIST_CACHE_SIZE );

			pTemp->m_ppTriList[pTemp->m_TriListCount] = &pNode->m_Tris[0];
			pTemp->m_ppTriList[pTemp->m_TriListCount+1] = &pNode->m_Tris[1];
			pTemp->m_TriListCount += 2;
		}
		// continue recursion
		else
		{
			for( int i = 0; i < 4; i++ )
			{
				RayNodeTest_r( pTemp, GetChildNode( nodeIndex, i ), rayStart, rayEnd );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTestAllTris( CDispCollData *pData, int power )
{
	//
	// get leaf indices
	//
	int startIndex = CalcNodeCount( power - 1 );
	int endIndex = CalcNodeCount( power );

	// save incoming fraction
	float startFraction = pData->m_Fraction;
	float fraction = pData->m_Fraction;

	Vector ray = pData->m_EndPos - pData->m_StartPos;
	Vector rayDir = ray;
	float rayLength = VectorNormalize(rayDir);

	//
	// test ray against all triangles in list
	//
	for( int index = startIndex; index < endIndex; index++ )
	{
		for( int j = 0; j < 2; j++ )
		{
			bool bResult = RayTriTest( pData->m_StartPos, rayDir, rayLength, &m_pNodes[index].m_Tris[j], &fraction );
			if( !bResult )
				continue;
			
			if( pData->m_bOcclude )
			{
				return true;
			}
			
			if( fraction < pData->m_Fraction )
			{
				pData->m_Fraction = fraction;
				pData->m_Normal = m_pNodes[index].m_Tris[j].m_Normal;
				pData->m_Distance = m_pNodes[index].m_Tris[j].m_Distance;
			}
		}
	}

	// collision!
	if( pData->m_Fraction < startFraction )
		return true;

	// no collision!
	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::RayTest( CDispCollData *pData )
{
	// reset the triangle list count
	CDispCollTreeTempData tmp;
	tmp.m_TriListCount = 0;

	// trace against nodes (copy start, end because they change)
	RayNodeTest_r( &tmp, 0, pData->m_StartPos, pData->m_EndPos );

	//
	// trace against tris (if need be)
	//
	if( tmp.m_TriListCount != 0 )
	{
		bool result = RayTriListTest( &tmp, pData );
		return result;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::SweptAABBTriIntersect( Vector &rayStart, Vector &rayEnd, Vector &extents,
										   CDispCollTri const *pTri, Vector &plNormal, float *plDist,
										   float *fraction )
{

	//
	// PUT A COPY HERE OF START AND END -- SINCE I CHANGE THEM!!!!!!
	//





	int		dir, ptIndex;
	float	closeValue;
	float	distStart, distEnd;
	float   t;
	Vector  rayPt;

	// get ray direction
	Vector rayDir = rayEnd - rayStart;

	// initialize fraction
	*fraction = 1.0f;

	//
	// test for collision with axial planes (x, y, z)
	//
	for( dir = 0; dir < 3; dir++ )
	{
		if( rayDir[dir] < 0.0f )
		{
			closeValue = -99999.9f;
			for( ptIndex = 0; ptIndex < 3; ptIndex++ )
			{
				if( pTri->m_Points[ptIndex][dir] > closeValue )
				{
					closeValue = pTri->m_Points[ptIndex][dir];
				}
			}

			closeValue += extents[dir];

			distStart = rayStart[dir] - closeValue;
			distEnd = rayEnd[dir] - closeValue;
		}
		else
		{
			closeValue = 99999.9f;
			for( ptIndex = 0; ptIndex < 3; ptIndex++ )
			{
				if( pTri->m_Points[ptIndex][dir] < closeValue )
				{
					closeValue = pTri->m_Points[ptIndex][dir];
				}
			}

			closeValue -= extents[dir];

			distStart = -( rayStart[dir] - closeValue );
			distEnd = -( rayEnd[dir] - closeValue );
		}

		if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
		{
			t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
			if( t > *fraction )
			{
				VectorScale( rayDir, t, rayPt );
				VectorAdd( rayStart, rayPt, rayStart );
				*fraction = t;
				plNormal.Init();
				plNormal[dir] = 1.0f;
				*plDist = closeValue;
			}
		}
		else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayEnd );
		}
		else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			return false;
		}
	}

	//
	// check for an early out
	//
	if( ( pTri->m_Normal[0] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[1] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[2] > ONE_MINUS_COLLISION_EPSILON ) )
	{
		if( *fraction == 1.0f )
			return false;

		return true;
	}
	
	//
	// handle 9 edge tests
	//
	Vector  normal;
	Vector  edge;
	float	dist;

	// find the closest box point	
	Vector boxPt( 0.0f, 0.0f, 0.0f );
	for( dir = 0; dir < 3; dir++ )
	{
		if( rayDir[dir] < 0.0f )
		{
			boxPt[dir] = extents[dir];
		}
		else
		{
			boxPt[dir] = -extents[dir];
		}
	}

	//
	// edge 0
	//
	edge = pTri->m_Points[1] - pTri->m_Points[0];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.y * rayStart.y ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.y * rayEnd.y ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.y * rayStart.y ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.y * rayEnd.y ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	//
	// edge 1
	//
	edge = pTri->m_Points[2] - pTri->m_Points[1];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.y * rayStart.y ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.y * rayEnd.y ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.y * rayStart.y ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.y * rayEnd.y ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	//
	// edge 2
	//
	edge = pTri->m_Points[0] - pTri->m_Points[2];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.y * rayStart.y ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.y * rayEnd.y ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.z * rayStart.z ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.z * rayEnd.z ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;

	// extents adjusted dist
	dist = ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ); 

	// find distances from plane (start, end)
	distStart = ( normal.x * rayStart.x ) + ( normal.y * rayStart.y ) - dist;
	distEnd = ( normal.x * rayEnd.x ) + ( normal.y * rayEnd.y ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	//
	// test face plane
	//
	dist = ( pTri->m_Normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) +
		   ( pTri->m_Normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) +
		   ( pTri->m_Normal.z * ( boxPt.z - pTri->m_Points[0].z ) );

	distStart = pTri->m_Normal.Dot( rayStart ) - dist;
	distEnd = pTri->m_Normal.Dot( rayEnd ) - dist;

	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
	{
		t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > *fraction )
		{
			VectorScale( rayDir, t, rayPt );
			VectorAdd( rayStart, rayPt, rayStart );
			*fraction = t;
			plNormal = normal;
			*plDist = dist;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		VectorScale( rayDir, t, rayPt );
		VectorAdd( rayStart, rayPt, rayEnd );
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	if( *fraction == 1.0f )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTriIntersect( CDispCollTreeTempData *pTemp, CDispCollData *pData )
{
	bool bResult = false;

	Vector  normal;
	float   fraction, dist;

	//
	// sweep ABB against all triangles in list
	//
	for( int i = 0; i < pTemp->m_TriListCount; i++ )
	{
		if( pTemp->m_ppTriList[i]->IsIntersect() )
		{
			bResult = SweptAABBTriIntersect( pData->m_StartPos, pData->m_EndPos, pData->m_Extents,
				                             pTemp->m_ppTriList[i], normal, &dist, &fraction );
			if( bResult )
			{
				if( fraction < pData->m_Fraction )
				{
					pData->m_Fraction = fraction;
					pData->m_Normal = normal;
					pData->m_Distance = dist;
				}
			}
		}
	}

	return bResult;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::IntersectAABBTriTest( Vector &rayStart, Vector &extents, 
										  CDispCollTri const *pTri )
{
	int   dir, ptIndex;
	float dist;

	//
	// test axail planes (x, y, z)
	//

	for( dir = 0; dir < 3; dir++ )
	{
		//
		// negative axial plane, component = dir
		//
		dist = rayStart[dir] - extents[dir];
		for( ptIndex = 0; ptIndex < 3; ptIndex++ )
		{
			if( pTri->m_Points[ptIndex][dir] > dist )
				break;
		}

		if( ptIndex == 3 )
			return false;

		//
		// positive axial plane, component = dir
		//
		dist = rayStart[dir] + extents[dir];
		for( ptIndex = 0; ptIndex < 3; ptIndex++ )
		{
			if( pTri->m_Points[ptIndex][dir] < dist )
				break;
		}

		if( ptIndex == 3 )
			return false;
	}

	//
	// add a test here to see if triangle face normal is close to axial -- done if so!!!
	//
	if( ( pTri->m_Normal[0] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[1] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[2] > ONE_MINUS_COLLISION_EPSILON ) )
		return true;

	// find the closest point on the box (use negated tri face noraml)
	Vector boxPt( 0.0f, 0.0f, 0.0f );
	for( dir = 0; dir < 3; dir++ )
	{
		if( pTri->m_Normal[dir] < 0.0f )
		{
			boxPt[dir] = extents[dir];
		}
		else
		{
			boxPt[dir] = -extents[dir];
		}
	}

	//
	// triangle plane test
	//
	// do the opposite because the ray has been negated
	if( ( ( pTri->m_Normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) +
		  ( pTri->m_Normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) +
		  ( pTri->m_Normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		  return false;

	//
	// test edge planes - 9 of them
	//
	Vector normal;
	Vector edge;

	//
	// edge 0
	//
	edge = pTri->m_Points[1] - pTri->m_Points[0];

	// cross x
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross y
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross z
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) ) > 0.0f )
		return false;

	//
	// edge 1
	//
	edge = pTri->m_Points[2] - pTri->m_Points[1];

	// cross x
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross y
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross z
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) ) > 0.0f )
		return false;

	//
	// edge 2
	//
	edge = pTri->m_Points[0] - pTri->m_Points[2];

	// cross x
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross y
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.z * ( boxPt.z - pTri->m_Points[0].z ) ) ) > 0.0f )
		return false;

	// cross z
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - pTri->m_Points[0].x ) ) + ( normal.y * ( boxPt.y - pTri->m_Points[0].y ) ) ) > 0.0f )
		return false;

	return true;
}




//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::SweptAABBTriTest( Vector &rayStart, Vector &rayEnd, Vector &extents, 
									  CDispCollTri const *pTri )
{
	// get ray direction
	Vector rayDir = rayEnd - rayStart;

	//
	// quick and dirty test -- test to see if the object is traveling away from triangle surface???
	//
	if( pTri->m_Normal.Dot( rayDir ) > 0.0f )
		return false;

	//
	// calc the swept triangle face (negate the ray -- opposite direction of box travel)
	//	
	rayDir.Negate();

	Vector points[3];
	points[0] = pTri->m_Points[0] + rayDir;
	points[1] = pTri->m_Points[1] + rayDir;
	points[2] = pTri->m_Points[2] + rayDir;

	//
	// handle 4 faces tests (3 axial planes and triangle face)
	//
	int	  dir;
	float dist;

	//
	// axial planes tests (x, y, z)
	//
	for( dir = 0; dir < 3; dir++ )
	{
		bool bOutside = true;

		if( rayDir[dir] < 0.0f )
		{
			dist = rayStart[dir] - extents[dir];
			for( int ptIndex = 0; ptIndex < 3; ptIndex )
			{
				if( points[ptIndex][dir] > dist )
				{
					bOutside = false;
					break;
				}
			}
		}
		else
		{
			dist = rayStart[dir] + extents[dir];
			for( int ptIndex = 0; ptIndex < 3; ptIndex )
			{
				if( pTri->m_Points[ptIndex][dir] < dist )
				{
					bOutside = false;
					break;
				}
			}
		}

		if( bOutside )
			return false;
	}
	
	//
	// add a test here to see if triangle face normal is close to axial -- done if so!!!
	//
	if( ( pTri->m_Normal[0] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[1] > ONE_MINUS_COLLISION_EPSILON ) || 
		( pTri->m_Normal[2] > ONE_MINUS_COLLISION_EPSILON ) )
		return true;

	//
	// handle 9 edge tests - always use the newly swept face for this
	//
	Vector normal;	
	Vector edge;

	// find the closest box point - (is written opposite to normal due to negating ray)
	Vector boxPt( 0.0f, 0.0f, 0.0f );
	for( dir = 0; dir < 3; dir++ )
	{
		if( rayDir[dir] < 0.0f )
		{
			boxPt[dir] = rayStart[dir] - extents[dir];
		}
		else
		{
			boxPt[dir] = rayStart[dir] + extents[dir];
		}
	}

	//
	// edge 0
	//
	edge = points[1] - points[0];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - points[0].y ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross, y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.y * ( boxPt.y - points[0].y ) ) ) > 0.0f )
		return false;

	//
	// edge 1
	//
	edge = points[2] - points[1];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - points[0].y ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross, y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.y * ( boxPt.y - points[0].y ) ) ) > 0.0f )
		return false;

	//
	// edge 2
	//
	edge = points[0] - points[2];

	// cross x-edge
	normal.x = 0.0f;
	normal.y = -edge.z;
	normal.z = edge.y;
    if(	( ( normal.y * ( boxPt.y - points[0].y ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross, y-edge
	normal.x = edge.z;
	normal.y = 0.0f;
	normal.z = edge.y;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		return false;

	// cross z-edge
	normal.x = -edge.y;
	normal.y = edge.x;
	normal.z = 0.0f;
	if( ( ( normal.x * ( boxPt.x - points[0].x ) ) + ( normal.y * ( boxPt.y - points[0].y ) ) ) > 0.0f )
		return false;

	//
	// triangle plane test
	//
	// do the opposite because the ray has been negated
	if( ( ( pTri->m_Normal.x * ( boxPt.x - points[0].x ) ) +
		  ( pTri->m_Normal.y * ( boxPt.y - points[0].y ) ) +
		  ( pTri->m_Normal.z * ( boxPt.z - points[0].z ) ) ) > 0.0f )
		  return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::CullTriList( CDispCollTreeTempData *pTemp, Vector &rayStart, Vector &rayEnd, Vector &extents, bool bIntersect )
{
	//
	// intersect AABB with all triangles in list
	//
	if( bIntersect )
	{
		for( int i = 0; i < pTemp->m_TriListCount; i++ )
		{
			if( IntersectAABBTriTest( rayStart, extents, pTemp->m_ppTriList[i] ) )
				return true;
		}

		return false;
	}
	//
	// sweep AABB against all triangles in list
	//
	else
	{
		bool bResult = false;

		for( int i = 0; i < pTemp->m_TriListCount; i++ )
		{
			if( SweptAABBTriTest( rayStart, rayEnd, extents, pTemp->m_ppTriList[i] ) )
			{
				pTemp->m_ppTriList[i]->SetIntersect( true );
				bResult = true;
			}
		}

		return bResult;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::IntersectAABBAABBTest( CDispCollTreeTempData *pTemp, const Vector &pos, const Vector &extents )
{
	float dist;

	for( int dir = 0; dir < 3; dir++ )
	{
		// negative direction
		dist = -( pos[dir] - ( pTemp->m_AABBDistances[(dir>>1)] - extents[dir] ) );
		if( dist > COLLISION_EPSILON )
			return false;

		// positive direction
		dist = pos[dir] - ( pTemp->m_AABBDistances[(dir>>1)+1] + extents[dir] );
		if( dist > COLLISION_EPSILON )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::SweptAABBAABBTest( CDispCollTreeTempData *pTemp, const Vector &rayStart, const Vector &rayEnd, const Vector &extents )
{
	int   dir;
	float distStart, distEnd;
	float fraction;
	float deltas[3];
	float scalers[3];

	//
	// enter and exit fractions
	//
	float enterFraction = 0.0f;
	float exitFraction = 0.0f;

	//
	// de-normalize the paramter space so that we don't have to divide
	// to find the fractional amount later (clamped for precision)
	//
	deltas[0] = rayEnd.x - rayStart.x;
	deltas[1] = rayEnd.y - rayStart.y;
	deltas[2] = rayEnd.z - rayStart.z;
	if( ( deltas[0] < COLLISION_EPSILON ) && ( deltas[0] > -COLLISION_EPSILON ) ) { deltas[0] = 1.0f; }
	if( ( deltas[1] < COLLISION_EPSILON ) && ( deltas[1] > -COLLISION_EPSILON ) ) { deltas[0] = 1.0f; }
	if( ( deltas[2] < COLLISION_EPSILON ) && ( deltas[2] > -COLLISION_EPSILON ) ) { deltas[0] = 1.0f; }
	scalers[0] = deltas[1] * deltas[2];
	scalers[1] = deltas[0] * deltas[2];
	scalers[2] = deltas[0] * deltas[1];

	for( dir = 0; dir < 3; dir++ )
	{
		//
		// negative direction
		//
		distStart = -( rayStart[dir] - ( pTemp->m_AABBDistances[(dir>>1)] - extents[dir] ) );
		distEnd = -( rayEnd[dir] - ( pTemp->m_AABBDistances[(dir>>1)] - extents[dir] ) );

		if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
		{
			fraction = distStart * scalers[dir];
			if( fraction > enterFraction )
			{
				enterFraction = fraction;
			}
		}
		else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			fraction = distStart * scalers[dir];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
		else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			return false;
		}

		//
		// positive direction
		//
		distStart = rayStart[dir] - ( pTemp->m_AABBDistances[(dir>>1)+1] + extents[dir] );
		distEnd = rayEnd[dir] - ( pTemp->m_AABBDistances[(dir>>1)+1] + extents[dir] );

		if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) )
		{
			fraction = distStart * scalers[dir];
			if( fraction > enterFraction )
			{
				enterFraction = fraction;
			}
		}
		else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			fraction = distStart * scalers[dir];
			if( fraction < exitFraction )
			{
				exitFraction = fraction;
			}
		}
		else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
		{
			return false;
		}
	}

	if( exitFraction < enterFraction )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::BuildTriList_r( CDispCollTreeTempData *pTemp, int nodeIndex, Vector &rayStart, Vector &rayEnd, Vector &extents,
								    bool bIntersect )
{
	//
	// get the current nodes bounds and create collision test planes
	// (saved in the in class cache m_AABBNormals, m_AABBDistances) 
	//
	Vector bounds[2];
	CDispCollNode *pNode = &m_pNodes[nodeIndex];
	pNode->GetBounds( bounds[0], bounds[1] );
	CreatePlanesFromBounds( pTemp, bounds[0], bounds[1] );

	//
	// interesect/sweep test
	//
	bool bResult;
	if( bIntersect )
	{
		bResult = IntersectAABBAABBTest( pTemp, rayStart, extents );
	}
	else
	{
		bResult = SweptAABBAABBTest( pTemp, rayStart, rayEnd, extents );
	}

	if( bResult )
	{
		// if leaf node -- add triangles to interstection test list
		if( pNode->IsLeaf() )
		{
			// Assert for now -- flush cache later!!!!!
			Assert( pTemp->m_TriListCount >= 0 );
			Assert( pTemp->m_TriListCount < TRILIST_CACHE_SIZE );

			pTemp->m_ppTriList[pTemp->m_TriListCount] = &pNode->m_Tris[0];
			pTemp->m_ppTriList[pTemp->m_TriListCount+1] = &pNode->m_Tris[1];
			pTemp->m_TriListCount += 2;
		}
		// continue recursion
		else
		{
			BuildTriList_r( pTemp, GetChildNode( nodeIndex, 0 ), rayStart, rayEnd, extents, bIntersect );
			BuildTriList_r( pTemp, GetChildNode( nodeIndex, 1 ), rayStart, rayEnd, extents, bIntersect );
			BuildTriList_r( pTemp, GetChildNode( nodeIndex, 2 ), rayStart, rayEnd, extents, bIntersect );
			BuildTriList_r( pTemp, GetChildNode( nodeIndex, 3 ), rayStart, rayEnd, extents, bIntersect );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBSweep( CDispCollData *pData )
{
	// reset the triangle lists counts
	CDispCollTreeTempData tmp;
	tmp.m_TriListCount = 0;

	// sweep the AABB against the tree
	BuildTriList_r( &tmp, 0, pData->m_StartPos, pData->m_EndPos, pData->m_Extents, false );

	// find collision triangles
	if( CullTriList( &tmp, pData->m_StartPos, pData->m_EndPos, pData->m_Extents, false ) )
	{
		// find closest intersection
		return AABBTriIntersect( &tmp, pData );
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBIntersect( CDispCollData *pData )
{
	// reset the triangle lists counts
	CDispCollTreeTempData tmp;
	tmp.m_TriListCount = 0;

	// sweep the AABB against the tree
	BuildTriList_r( &tmp, 0, pData->m_StartPos, pData->m_StartPos, pData->m_Extents, true );

	// find collision triangles
	return CullTriList( &tmp, pData->m_StartPos, pData->m_StartPos, pData->m_Extents, true );
}
