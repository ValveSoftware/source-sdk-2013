//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "disp_common.h"
#include "disp_powerinfo.h"
#include "builddisp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNodeVert
{
public:
						CNodeVert()					{}
						CNodeVert( int ix, int iy ) {x=ix; y=iy;}

	inline int&			operator[]( int i )			{return ((int*)this)[i];}
	inline int const&	operator[]( int i ) const	{return ((int*)this)[i];}

	int x, y;
};

static CNodeVert const g_NodeChildLookup[4][2] =
{
	{CNodeVert(0,0), CNodeVert(1,1)},
	{CNodeVert(1,0), CNodeVert(2,1)},
	{CNodeVert(0,1), CNodeVert(1,2)},
	{CNodeVert(1,1), CNodeVert(2,2)}
};

static CNodeVert const g_NodeTriWinding[9] =
{
	CNodeVert(0, 1), 
	CNodeVert(0, 0), 
	CNodeVert(1, 0), 
	CNodeVert(2, 0), 
	CNodeVert(2, 1), 
	CNodeVert(2, 2), 
	CNodeVert(1, 2), 
	CNodeVert(0, 2), 
	CNodeVert(0, 1) 
};

// Indexed by CORNER_. These store NEIGHBOREDGE_ defines and tell which edges butt up against the corner.
static int g_CornerEdges[4][2] =
{
	{ NEIGHBOREDGE_BOTTOM,	NEIGHBOREDGE_LEFT },	// CORNER_LOWER_LEFT
	{ NEIGHBOREDGE_TOP,		NEIGHBOREDGE_LEFT },	// CORNER_UPPER_LEFT
	{ NEIGHBOREDGE_TOP,		NEIGHBOREDGE_RIGHT },	// CORNER_UPPER_RIGHT
	{ NEIGHBOREDGE_BOTTOM,	NEIGHBOREDGE_RIGHT }	// CORNER_LOWER_RIGHT
};

int g_EdgeDims[4] =
{
	0,		// NEIGHBOREDGE_LEFT   = X
	1,		// NEIGHBOREDGE_TOP    = Y
	0,		// NEIGHBOREDGE_RIGHT  = X
	1		// NEIGHBOREDGE_BOTTOM = Y
};

CShiftInfo g_ShiftInfos[3][3] =
{
	{
		{0,  0, true},		// CORNER_TO_CORNER -> CORNER_TO_CORNER
		{0, -1, true},		// CORNER_TO_CORNER -> CORNER_TO_MIDPOINT
		{2, -1, true}		// CORNER_TO_CORNER -> MIDPOINT_TO_CORNER
	},
	
	{
		{0,  1, true},		// CORNER_TO_MIDPOINT -> CORNER_TO_CORNER
		{0,  0, false},		// CORNER_TO_MIDPOINT -> CORNER_TO_MIDPOINT (invalid)
		{0,  0, false}		// CORNER_TO_MIDPOINT -> MIDPOINT_TO_CORNER (invalid)
	},

	{	
		{-1, 1, true},		// MIDPOINT_TO_CORNER -> CORNER_TO_CORNER
		{0,  0, false},		// MIDPOINT_TO_CORNER -> CORNER_TO_MIDPOINT (invalid)
		{0,  0, false}		// MIDPOINT_TO_CORNER -> MIDPOINT_TO_CORNER (invalid)
	}
};

int g_EdgeSideLenMul[4] =
{
	0,
	1,
	1,
	0
};


// --------------------------------------------------------------------------------- //
// Helper functions.
// --------------------------------------------------------------------------------- //

inline int SignedBitShift( int val, int shift )
{
	if( shift > 0 )
		return val << shift;
	else
		return val >> -shift;
}

static inline void RotateVertIndex( 
	NeighborOrientation neighor, 
	int sideLengthMinus1,
	CVertIndex const &in,
	CVertIndex &out )
{
	if( neighor == ORIENTATION_CCW_0 )
	{
		out = in;
	}
	else if( neighor == ORIENTATION_CCW_90 )
	{
		out.x = in.y;
		out.y = sideLengthMinus1 - in.x;
	}
	else if( neighor == ORIENTATION_CCW_180 )
	{
		out.x = sideLengthMinus1 - in.x;
		out.y = sideLengthMinus1 - in.y;
	}
	else
	{
		out.x = sideLengthMinus1 - in.y;
		out.y = in.x;
	}
}

static inline void RotateVertIncrement( 
	NeighborOrientation neighor, 
	CVertIndex const &in,
	CVertIndex &out )
{
	if( neighor == ORIENTATION_CCW_0 )
	{
		out = in;
	}
	else if( neighor == ORIENTATION_CCW_90 )
	{
		out.x = in.y;
		out.y = -in.x;
	}
	else if( neighor == ORIENTATION_CCW_180 )
	{
		out.x = -in.x;
		out.y = -in.y;
	}
	else
	{
		out.x = -in.y;
		out.y = in.x;
	}
}


// --------------------------------------------------------------------------------- //
// CDispHelper functions.
// --------------------------------------------------------------------------------- //

int GetEdgeIndexFromPoint( CVertIndex const &index, int iMaxPower )
{
	int sideLengthMinus1 = 1 << iMaxPower;

	if( index.x == 0 )
		return NEIGHBOREDGE_LEFT;
	else if( index.y == sideLengthMinus1 )
		return NEIGHBOREDGE_TOP;
	else if( index.x == sideLengthMinus1 )
		return NEIGHBOREDGE_RIGHT;
	else if( index.y == 0 )
		return NEIGHBOREDGE_BOTTOM;
	else
		return -1;
}


int GetCornerIndexFromPoint( CVertIndex const &index, int iPower )
{
	int sideLengthMinus1 = 1 << iPower;

	if( index.x == 0 && index.y == 0 )
		return CORNER_LOWER_LEFT;
	
	else if( index.x == 0 && index.y == sideLengthMinus1 )
		return CORNER_UPPER_LEFT;

	else if( index.x == sideLengthMinus1 && index.y == sideLengthMinus1 )
		return CORNER_UPPER_RIGHT;

	else if( index.x == sideLengthMinus1 && index.y == 0 )
		return CORNER_LOWER_RIGHT;

	else
		return -1;
}


int GetNeighborEdgePower( CDispUtilsHelper *pDisp, int iEdge, int iSub )
{
	CDispNeighbor *pEdge = pDisp->GetEdgeNeighbor( iEdge );
	CDispSubNeighbor *pSub = &pEdge->m_SubNeighbors[iSub];
	if ( !pSub->IsValid() )
		return -1;

	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->GetNeighborIndex() );
	
	CShiftInfo *pInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pInfo->m_bValid );

	return pNeighbor->GetPower() + pInfo->m_PowerShiftAdd;
}


CDispUtilsHelper* SetupEdgeIncrements(
	CDispUtilsHelper *pDisp,
	int iEdge,
	int iSub,
	CVertIndex &myIndex,
	CVertIndex &myInc,
	CVertIndex &nbIndex,
	CVertIndex &nbInc,
	int &myEnd,
	int &iFreeDim )
{
	int iEdgeDim = g_EdgeDims[iEdge];
	iFreeDim = !iEdgeDim;

	CDispNeighbor *pSide = pDisp->GetEdgeNeighbor( iEdge );
	CDispSubNeighbor *pSub = &pSide->m_SubNeighbors[iSub];
	if ( !pSub->IsValid() )
		return NULL;

	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->m_iNeighbor );

	CShiftInfo *pShiftInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pShiftInfo->m_bValid );

	// Setup a start point and edge increment (NOTE: just precalculate these
	// and store them in the CDispSubNeighbors).
	CVertIndex tempInc;
	
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	myIndex[iEdgeDim] = g_EdgeSideLenMul[iEdge] * pPowerInfo->m_SideLengthM1;
	myIndex[iFreeDim] = pPowerInfo->m_MidPoint * iSub;
	TransformIntoSubNeighbor( pDisp, iEdge, iSub, myIndex, nbIndex );
	
	int myPower = pDisp->GetPowerInfo()->m_Power;
	int nbPower = pNeighbor->GetPowerInfo()->m_Power + pShiftInfo->m_PowerShiftAdd;
	
	myInc[iEdgeDim] = tempInc[iEdgeDim] = 0;
	if( nbPower > myPower )
	{
		myInc[iFreeDim] = 1;
		tempInc[iFreeDim] = 1 << (nbPower - myPower);
	}
	else
	{
		myInc[iFreeDim] = 1 << (myPower - nbPower);
		tempInc[iFreeDim] = 1;
	}
	RotateVertIncrement( pSub->GetNeighborOrientation(), tempInc, nbInc );

	// Walk along the edge.
	if( pSub->m_Span == CORNER_TO_MIDPOINT )
		myEnd = pDisp->GetPowerInfo()->m_SideLength >> 1;
	else
		myEnd = pDisp->GetPowerInfo()->m_SideLength - 1;

	return pNeighbor;
}


int GetSubNeighborIndex( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex )
{
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	const CDispNeighbor *pSide = pDisp->GetEdgeNeighbor( iEdge );

	// Figure out if this is a vertical or horizontal edge.
	int iEdgeDim = g_EdgeDims[iEdge];
	int iFreeDim = !iEdgeDim;

	int iFreeIndex = nodeIndex[iFreeDim];

	// Figure out which of the (up to two) neighbors it lies in.
	int iSub = 0;
	if( iFreeIndex == pPowerInfo->m_MidPoint )
	{
		// If it's in the middle, we only are interested if there's one neighbor
		// next to us (so we can enable its middle vert). If there are any neighbors
		// that touch the midpoint, then we have no need to return them because it would
		// touch their corner verts which are always active.
		if( pSide->m_SubNeighbors[0].m_Span != CORNER_TO_CORNER )
			return -1;
	}
	else if ( iFreeIndex > pPowerInfo->m_MidPoint )
	{
		iSub = 1;
	}

	// Make sure we get a valid neighbor.
	if( !pSide->m_SubNeighbors[iSub].IsValid() )
	{
		if( iSub == 1 && 
			pSide->m_SubNeighbors[0].IsValid() && 
			pSide->m_SubNeighbors[0].m_Span == CORNER_TO_CORNER )
		{
			iSub = 0;
		}
		else
		{
			return -1;
		}
	}

	return iSub;
}


void SetupSpan( int iPower, int iEdge, NeighborSpan span, CVertIndex &viStart, CVertIndex &viEnd )
{
	int iFreeDim = !g_EdgeDims[iEdge];
	const CPowerInfo *pPowerInfo = GetPowerInfo( iPower );

	viStart = pPowerInfo->GetCornerPointIndex( iEdge );
	viEnd = pPowerInfo->GetCornerPointIndex( (iEdge+1) & 3 );;

	if ( iEdge == NEIGHBOREDGE_RIGHT || iEdge == NEIGHBOREDGE_BOTTOM )
	{
		// CORNER_TO_MIDPOINT and MIDPOINT_CORNER are defined where the edge moves up or right,
		// but pPowerInfo->GetCornerPointIndex walks around the edges clockwise, so on the
		// bottom and right edges (where GetCornerPointIndex has us moving down and left) we need to
		// reverse the sense here to make sure we return the right span.
		if ( span == CORNER_TO_MIDPOINT )
			viStart[iFreeDim] = pPowerInfo->GetMidPoint();
		else if ( span == MIDPOINT_TO_CORNER )
			viEnd[iFreeDim] = pPowerInfo->GetMidPoint();
	}
	else
	{
		if ( span == CORNER_TO_MIDPOINT )
			viEnd[iFreeDim] = pPowerInfo->GetMidPoint();
		else if ( span == MIDPOINT_TO_CORNER )
			viStart[iFreeDim] = pPowerInfo->GetMidPoint();
	}
}


CDispUtilsHelper* TransformIntoSubNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	int iSub,
	CVertIndex const &nodeIndex, 
	CVertIndex &out
	)
{
	const CDispSubNeighbor *pSub = &pDisp->GetEdgeNeighbor( iEdge )->m_SubNeighbors[iSub];

	// Find the part of pDisp's edge that this neighbor covers.
	CVertIndex viSrcStart, viSrcEnd;
	SetupSpan( pDisp->GetPower(), iEdge, pSub->GetSpan(), viSrcStart, viSrcEnd );

	// Find the corresponding parts on the neighbor.
	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->GetNeighborIndex() );
	int iNBEdge = (iEdge + 2 + pSub->GetNeighborOrientation()) & 3;
	
	CVertIndex viDestStart, viDestEnd;
	SetupSpan( pNeighbor->GetPower(), iNBEdge, pSub->GetNeighborSpan(), viDestEnd, viDestStart );


	// Now map the one into the other.
	int iFreeDim = !g_EdgeDims[iEdge];
	int fixedPercent = ((nodeIndex[iFreeDim] - viSrcStart[iFreeDim]) * (1<<16)) / (viSrcEnd[iFreeDim] - viSrcStart[iFreeDim]);
	Assert( fixedPercent >= 0 && fixedPercent <= (1<<16) );

	int nbDim = g_EdgeDims[iNBEdge];
	out[nbDim] = viDestStart[nbDim];
	out[!nbDim] = viDestStart[!nbDim] + ((viDestEnd[!nbDim] - viDestStart[!nbDim]) * fixedPercent) / (1<<16);

	Assert( out.x >= 0 && out.x < pNeighbor->GetSideLength() );
	Assert( out.y >= 0 && out.y < pNeighbor->GetSideLength() );

	return pNeighbor;
}


CDispUtilsHelper* TransformIntoNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex, 
	CVertIndex &out
	)
{
	if ( iEdge == -1 )
		iEdge = GetEdgeIndexFromPoint( nodeIndex, pDisp->GetPower() );
	
	int iSub = GetSubNeighborIndex( pDisp, iEdge, nodeIndex );
	if ( iSub == -1 )
		return NULL;

	CDispUtilsHelper *pRet = TransformIntoSubNeighbor( pDisp, iEdge, iSub, nodeIndex, out );
	
#if 0
	// Debug check.. make sure it comes back to the same point from the other side.
	#if defined( _DEBUG )
		static bool bTesting = false;
		if ( pRet && !bTesting )
		{
			bTesting = true;

			// We could let TransformIntoNeighbor figure out the index but if this is a corner vert, then
			// it may pick the wrong edge and we'd get a benign assert.
			int nbOrientation = pDisp->GetEdgeNeighbor( iEdge )->m_SubNeighbors[iSub].GetNeighborOrientation();
			int iNeighborEdge = (iEdge + 2 + nbOrientation) & 3;

			CVertIndex testIndex;
			CDispUtilsHelper *pTest = TransformIntoNeighbor( pRet, iNeighborEdge, out, testIndex );
			Assert( pTest == pDisp );
			Assert( testIndex == nodeIndex );
		
			bTesting = false;
		}
	#endif
#endif
		
	return pRet;
}


bool DoesPointHaveAnyNeighbors( 
	CDispUtilsHelper *pDisp,
	const CVertIndex &index )
{
	// See if it connects to a neighbor on the edge.
	CVertIndex dummy;
	if ( TransformIntoNeighbor( pDisp, -1, index, dummy ) )
		return true;

	// See if it connects to a neighbor on a corner.
	int iCorner = GetCornerIndexFromPoint( index, pDisp->GetPower() );
	if ( iCorner == -1 )
		return false;

	// If there are any neighbors on the specified corner, then the point has neighbors.
	if ( pDisp->GetCornerNeighbors( iCorner )->m_nNeighbors > 0 )
		return true;

	// Since points on corners touch two edges, we actually want to test two edges to see
	// if the point has a neighbor on either edge.
	for ( int i=0; i < 2; i++ )
	{
		if ( TransformIntoNeighbor( pDisp, g_CornerEdges[iCorner][i], index, dummy ) )
			return true;
	}

	return false;
}


// ------------------------------------------------------------------------------------ //
// CDispSubEdgeIterator.
// ------------------------------------------------------------------------------------ //

CDispSubEdgeIterator::CDispSubEdgeIterator()
{
	m_pNeighbor = 0;
	m_FreeDim = m_Index.x = m_Inc.x = m_End = 0;	// Setup so Next returns false.
}


void CDispSubEdgeIterator::Start( CDispUtilsHelper *pDisp, int iEdge, int iSub, bool bTouchCorners )
{
	m_pNeighbor = SetupEdgeIncrements( pDisp, iEdge, iSub, m_Index, m_Inc, m_NBIndex, m_NBInc, m_End, m_FreeDim );
	if ( m_pNeighbor )
	{
		if ( bTouchCorners )
		{
			// Back up our current position by 1 so we hit the corner first, and extend the endpoint
			// so we hit the other corner too.
			m_Index -= m_Inc;
			m_NBIndex -= m_NBInc;

			m_End += m_Inc[m_FreeDim];
		}
	}
	else
	{
		m_FreeDim = m_Index.x = m_Inc.x = m_End = 0;	// Setup so Next returns false.
	}
}


bool CDispSubEdgeIterator::Next()
{
	m_Index += m_Inc;
	m_NBIndex += m_NBInc;

	// Were we just at the last point on the edge?
	return m_Index[m_FreeDim] < m_End;
}


bool CDispSubEdgeIterator::IsLastVert() const
{
	return (m_Index[m_FreeDim] + m_Inc[m_FreeDim]) >= m_End;
}


// ------------------------------------------------------------------------------------ //
// CDispEdgeIterator.
// ------------------------------------------------------------------------------------ //

CDispEdgeIterator::CDispEdgeIterator( CDispUtilsHelper *pDisp, int iEdge )
{
	m_pDisp = pDisp;
	m_iEdge = iEdge;
	m_iCurSub = -1;
}


bool CDispEdgeIterator::Next()
{
	while ( !m_It.Next() )
	{
		// Ok, move up to the next sub.
		if ( m_iCurSub == 1 )
			return false;
	
		++m_iCurSub;
		m_It.Start( m_pDisp, m_iEdge, m_iCurSub );
	}
	return true;
}


// ------------------------------------------------------------------------------------ //
// CDispCircumferenceIterator.
// ------------------------------------------------------------------------------------ //

CDispCircumferenceIterator::CDispCircumferenceIterator( int sideLength )
{
	m_iCurEdge = -1;
	m_SideLengthM1 = sideLength - 1;
}


bool CDispCircumferenceIterator::Next()
{
	switch ( m_iCurEdge )
	{
		case -1:
		{
			m_iCurEdge = NEIGHBOREDGE_LEFT;
			m_VertIndex.Init( 0, 0 );
		}
		break;

		case NEIGHBOREDGE_LEFT:
		{
			++m_VertIndex.y;
			if ( m_VertIndex.y == m_SideLengthM1 )
				m_iCurEdge = NEIGHBOREDGE_TOP;
		}
		break;

		case NEIGHBOREDGE_TOP:
		{
			++m_VertIndex.x;
			if ( m_VertIndex.x == m_SideLengthM1 )
				m_iCurEdge = NEIGHBOREDGE_RIGHT;
		}
		break;

		case NEIGHBOREDGE_RIGHT:
		{
			--m_VertIndex.y;
			if ( m_VertIndex.y == 0 )
				m_iCurEdge = NEIGHBOREDGE_BOTTOM;
		}
		break;

		case NEIGHBOREDGE_BOTTOM:
		{
			--m_VertIndex.x;
			if ( m_VertIndex.x == 0 )
				return false; // Done!
		}
		break;
	}

	return true;
}



// Helper function to setup an index either on the edges or the center
// of the box defined by [bottomleft,topRight].
static inline void SetupCoordXY( CNodeVert &out, CNodeVert const &bottomLeft, CNodeVert const &topRight, CNodeVert const &info )
{
	for( int i=0; i < 2; i++ )
	{
		if( info[i] == 0 )
			out[i] = bottomLeft[i];
		else if( info[i] == 1 )
			out[i] = (bottomLeft[i] + topRight[i]) >> 1;
		else
			out[i] = topRight[i];
	}
}


static unsigned short* DispCommon_GenerateTriIndices_R( 
	CNodeVert const &bottomLeft, 
	CNodeVert const &topRight,
	unsigned short *indices, 
	int power,
	int sideLength )
{
	if( power == 1 )
	{
		// Ok, add triangles. All we do here is follow a list of verts (g_NodeTriWinding)
		// around the center vert of this node and make triangles.
		int iCurTri = 0;
		CNodeVert verts[3];

		// verts[0] is always the center vert.
		SetupCoordXY( verts[0], bottomLeft, topRight, CNodeVert(1,1) );
		int iCurVert = 1;

		for( int i=0; i < 9; i++ )
		{
			SetupCoordXY( verts[iCurVert], bottomLeft, topRight, g_NodeTriWinding[i] );
			++iCurVert;

			if( iCurVert == 3 )
			{
				for( int iTriVert=2; iTriVert >= 0; iTriVert-- )
				{
					int index = verts[iTriVert].y * sideLength + verts[iTriVert].x;
					*indices = index;
					++indices;
				}

				// Setup for the next triangle.
				verts[1] = verts[2];
				iCurVert = 2;
				iCurTri++;
			}
		}
	}
	else
	{
		// Recurse into the children.
		for( int i=0; i < 4; i++ )
		{
			CNodeVert childBottomLeft, childTopRight;
			SetupCoordXY( childBottomLeft, bottomLeft, topRight, g_NodeChildLookup[i][0] );
			SetupCoordXY( childTopRight,   bottomLeft, topRight, g_NodeChildLookup[i][1] );

			indices = DispCommon_GenerateTriIndices_R( childBottomLeft, childTopRight, indices, power-1, sideLength );
		}
	}

	return indices;
}


// ------------------------------------------------------------------------------------------- //
// CDispUtilsHelper functions.
// ------------------------------------------------------------------------------------------- //
	
int CDispUtilsHelper::GetPower() const
{
	return GetPowerInfo()->GetPower();
}

int CDispUtilsHelper::GetSideLength() const
{
	return GetPowerInfo()->GetSideLength();
}

const CVertIndex& CDispUtilsHelper::GetCornerPointIndex( int iCorner ) const
{
	return GetPowerInfo()->GetCornerPointIndex( iCorner );
}

int CDispUtilsHelper::VertIndexToInt( const CVertIndex &i ) const
{
	Assert( i.x >= 0 && i.x < GetSideLength() && i.y >= 0 && i.y < GetSideLength() );
	return i.y * GetSideLength() + i.x;
}

CVertIndex CDispUtilsHelper::GetEdgeMidPoint( int iEdge ) const
{
	int end = GetSideLength() - 1;
	int mid = GetPowerInfo()->GetMidPoint();

	if ( iEdge == NEIGHBOREDGE_LEFT )
		return CVertIndex( 0, mid );
	
	else if ( iEdge == NEIGHBOREDGE_TOP )
		return CVertIndex( mid, end );

	else if ( iEdge == NEIGHBOREDGE_RIGHT )
		return CVertIndex( end, mid );
	
	else if ( iEdge == NEIGHBOREDGE_BOTTOM )
		return CVertIndex( mid, 0 );

	Assert( false );
	return CVertIndex( 0, 0 );
}

int DispCommon_GetNumTriIndices( int power )
{
	return (1<<power) * (1<<power) * 2 * 3;
}


void DispCommon_GenerateTriIndices( int power, unsigned short *indices )
{
	int sideLength = 1 << power;
	DispCommon_GenerateTriIndices_R(
		CNodeVert( 0, 0 ), 
		CNodeVert( sideLength, sideLength ),
		indices,
		power,
		sideLength+1 );
}

//=============================================================================
//
// Finding neighbors.
//

// This table swaps MIDPOINT_TO_CORNER and CORNER_TO_MIDPOINT.
static NeighborSpan g_SpanFlip[3] = {CORNER_TO_CORNER, MIDPOINT_TO_CORNER, CORNER_TO_MIDPOINT};
static bool			g_bEdgeNeighborFlip[4] = {false, false, true, true};

// These map CCoreDispSurface neighbor orientations (which are actually edge indices)
// into our 'degrees of rotation' representation.
static int g_CoreDispNeighborOrientationMap[4][4] =
{
	{ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90},
	{ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270, ORIENTATION_CCW_0},
	{ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180, ORIENTATION_CCW_270},
	{ORIENTATION_CCW_270, ORIENTATION_CCW_0,   ORIENTATION_CCW_90,  ORIENTATION_CCW_180}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ClearNeighborData( CCoreDispInfo *pDisp )
{
	for ( int i=0; i < 4; i++ )
	{
		pDisp->GetEdgeNeighbor( i )->SetInvalid();
		pDisp->GetCornerNeighbors( i )->SetInvalid();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetDispBox( CCoreDispInfo *pDisp, CDispBox &box )
{
	// Calculate the bbox for this displacement.
	Vector vMin(  1e24,  1e24,  1e24 );
	Vector vMax( -1e24, -1e24, -1e24 );

	for ( int iVert = 0; iVert < 4; ++iVert )
	{
		const Vector &vTest = pDisp->GetSurface()->GetPoint( iVert );
		VectorMin( vTest, vMin, vMin );
		VectorMax( vTest, vMax, vMax );
	}		

	// Puff the box out a little.
	static float flPuff = 0.1f;
	vMin -= Vector( flPuff, flPuff, flPuff );
	vMax += Vector( flPuff, flPuff, flPuff );

	box.m_Min = vMin;
	box.m_Max = vMax;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SetupDispBoxes( CCoreDispInfo **ppListBase, int nListSize, CUtlVector<CDispBox> &out )
{
	out.SetSize( nListSize );
	for ( int iDisp = 0; iDisp < nListSize; ++iDisp )
	{
		CCoreDispInfo *pDisp = ppListBase[iDisp];
		GetDispBox( pDisp, out[iDisp] );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool DoBBoxesTouch( const CDispBox &a, const CDispBox &b )
{
	for ( int i=0; i < 3; i++ )
	{
		if ( a.m_Max[i] < b.m_Min[i] )
			return false;

		if ( a.m_Min[i] > b.m_Max[i] )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool FindEdge( CCoreDispInfo *pInfo, Vector const &vPoint1, Vector const &vPoint2, int &iEdge )
{
	CCoreDispSurface *pSurface = pInfo->GetSurface();

	for( iEdge=0; iEdge < 4; iEdge++ )
	{
		if( VectorsAreEqual( vPoint1, pSurface->GetPoint( iEdge ), 0.01f ) &&
			VectorsAreEqual( vPoint2, pSurface->GetPoint( (iEdge+1) & 3), 0.01f ) )
		{
			return true;
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
NeighborSpan NeighborSpanFlip( int iEdge, NeighborSpan span )
{
	if ( g_bEdgeNeighborFlip[iEdge] )
		return g_SpanFlip[span];
	else
		return span;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void AddNeighbor( CCoreDispInfo *pMain,
	              int iEdge,				// Which of pMain's sides this is on.
	              int iSub,				// Which sub neighbor this takes up in pSide.
	              NeighborSpan span,		// What span this fills in pMain.
				  CCoreDispInfo *pOther, int iNeighborEdge, NeighborSpan nbSpan )
{
	// The edge iteration before coming in here goes 0-1, 1-2, 2-3, 3-4.
	// This flips the sense of CORNER_TO_MIDPOINT/MIDPOINT_TO_CORNER on the right and 
	// bottom edges and is undone here.
	span = NeighborSpanFlip( iEdge, span );
	nbSpan = NeighborSpanFlip( iNeighborEdge, nbSpan );

	// Get the subspan this fills on our displacement.
	CDispSubNeighbor *pSub = &pMain->GetEdgeNeighbor(iEdge)->m_SubNeighbors[iSub];
	
	// Which subspan does this use in the neighbor?
	CDispSubNeighbor *pNeighborSub;
	if ( nbSpan == MIDPOINT_TO_CORNER )
	{
		pNeighborSub = &pOther->GetEdgeNeighbor(iNeighborEdge)->m_SubNeighbors[1];
	}
	else
	{
		pNeighborSub = &pOther->GetEdgeNeighbor(iNeighborEdge)->m_SubNeighbors[0];
	}

	// Make sure this slot isn't used on either displacement.
	if ( pSub->IsValid() || pNeighborSub->IsValid() )
	{
		ExecuteOnce( Warning( "Found a displacement edge abutting multiple other edges.\n" ) );
		return;
	}

	// Now just copy the data into each displacement.	
	pSub->m_iNeighbor = pOther->GetListIndex();
	pSub->m_NeighborOrientation = g_CoreDispNeighborOrientationMap[iEdge][iNeighborEdge];
	pSub->m_Span = span;
	pSub->m_NeighborSpan = nbSpan;

	pNeighborSub->m_iNeighbor = pMain->GetListIndex();
	pNeighborSub->m_NeighborOrientation = g_CoreDispNeighborOrientationMap[iNeighborEdge][iEdge];
	pNeighborSub->m_Span = nbSpan;
	pNeighborSub->m_NeighborSpan = span;

#if defined( _DEBUG )
	// Walk an iterator over the new connection to make sure it works.
	CDispSubEdgeIterator it;
	it.Start( pMain, iEdge, iSub );
	while ( it.Next() )
	{
		CVertIndex nbIndex;
		TransformIntoNeighbor( pMain, iEdge, it.GetVertIndex(), nbIndex );
	}
#endif
}

//-----------------------------------------------------------------------------
// This function is symmetric wrt pMain and pOther. It sets up valid neighboring data for
// the relationship between both of them.
//-----------------------------------------------------------------------------
void SetupEdgeNeighbors( CCoreDispInfo *pMain, CCoreDispInfo *pOther )
{
	// Initialize..
	for( int iEdge=0; iEdge < 4; iEdge++ )
	{
		// Setup the edge points and the midpoint.
		Vector pt[2], mid;
		pMain->GetSurface()->GetPoint( iEdge, pt[0] );
		pMain->GetSurface()->GetPoint( (iEdge + 1) & 3, pt[1] );
		mid = (pt[0] + pt[1]) * 0.5f;

		// Find neighbors.
		int iNBEdge;
		if( FindEdge( pOther, pt[1], pt[0], iNBEdge ) )
		{
			AddNeighbor( pMain, iEdge, 0, CORNER_TO_CORNER, pOther, iNBEdge, CORNER_TO_CORNER );
		}
		else
		{
			// Look for one that takes up our whole side.
			if( FindEdge( pOther, pt[1], pt[0]*2 - pt[1], iNBEdge ) )
			{
				AddNeighbor( pMain, iEdge, 0, CORNER_TO_CORNER, pOther, iNBEdge, CORNER_TO_MIDPOINT );
			}
			else if( FindEdge( pOther, pt[1]*2 - pt[0], pt[0], iNBEdge ) )
			{
				AddNeighbor( pMain, iEdge, 0, CORNER_TO_CORNER, pOther, iNBEdge, MIDPOINT_TO_CORNER );
			}
			else
			{			
				// Ok, look for 1 or two that abut this side.
				if( FindEdge( pOther, mid, pt[0], iNBEdge ) )
				{
					AddNeighbor( pMain, iEdge, g_bEdgeNeighborFlip[iEdge], CORNER_TO_MIDPOINT, pOther, iNBEdge, CORNER_TO_CORNER );
				}
				
				if( FindEdge( pOther, pt[1], mid, iNBEdge ) )
				{
					AddNeighbor( pMain, iEdge, !g_bEdgeNeighborFlip[iEdge], MIDPOINT_TO_CORNER, pOther, iNBEdge, CORNER_TO_CORNER );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Returns true if the displacement has an edge neighbor with the given index.
//-----------------------------------------------------------------------------
bool HasEdgeNeighbor( const CCoreDispInfo *pMain, int iNeighbor )
{
	for ( int i=0; i < 4; i++ )
	{
		const CDispCornerNeighbors *pCorner = pMain->GetCornerNeighbors( i );
		for ( int iNB=0; iNB < pCorner->m_nNeighbors; iNB++ )
			if ( pCorner->m_Neighbors[iNB] == iNeighbor )
				return true;

		const CDispNeighbor *pEdge = pMain->GetEdgeNeighbor( i );
		if ( pEdge->m_SubNeighbors[0].GetNeighborIndex() == iNeighbor || 
			pEdge->m_SubNeighbors[1].GetNeighborIndex() == iNeighbor )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SetupCornerNeighbors( CCoreDispInfo *pMain, CCoreDispInfo *pOther, int *nOverflows )
{
	if ( HasEdgeNeighbor( pMain, pOther->GetListIndex() ) )
		return;

	// Do these two share a vertex?
	int nShared = 0;
	int iMainSharedCorner = -1;
	int iOtherSharedCorner = -1;

	for ( int iMainCorner=0; iMainCorner < 4; iMainCorner++ )
	{
		Vector const &vMainCorner = pMain->GetCornerPoint( iMainCorner );
		
		for ( int iOtherCorner=0; iOtherCorner < 4; iOtherCorner++ )
		{
			Vector const &vOtherCorner = pOther->GetCornerPoint( iOtherCorner );
		
			if ( VectorsAreEqual( vMainCorner, vOtherCorner, 0.001f ) )
			{
				iMainSharedCorner = iMainCorner;
				iOtherSharedCorner = iOtherCorner;
				++nShared;
			}
		}
	}

	if ( nShared == 1 )
	{
		CDispCornerNeighbors *pMainCorner = pMain->GetCornerNeighbors( iMainSharedCorner );
		CDispCornerNeighbors *pOtherCorner = pOther->GetCornerNeighbors( iOtherSharedCorner );

		if ( pMainCorner->m_nNeighbors < MAX_DISP_CORNER_NEIGHBORS &&
			pOtherCorner->m_nNeighbors < MAX_DISP_CORNER_NEIGHBORS )
		{
			pMainCorner->m_Neighbors[pMainCorner->m_nNeighbors++] = pOther->GetListIndex();
			pOtherCorner->m_Neighbors[pOtherCorner->m_nNeighbors++] = pMain->GetListIndex();
		}
		else
		{
			++(*nOverflows);
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool VerifyNeighborVertConnection( CDispUtilsHelper *pDisp, const CVertIndex &nodeIndex, 
								   const CDispUtilsHelper *pTestNeighbor, const CVertIndex &testNeighborIndex,
								   int mySide )
{
	CVertIndex nbIndex( -1, -1 );
	CDispUtilsHelper *pNeighbor = NULL;
	if( (pNeighbor = TransformIntoNeighbor( pDisp, mySide, nodeIndex, nbIndex ) ) != NULL )
	{
		if ( pTestNeighbor != pNeighbor || nbIndex != testNeighborIndex )
			return false;

		CVertIndex testIndex( -1, -1 );
		int iSide = GetEdgeIndexFromPoint( nbIndex, pNeighbor->GetPowerInfo()->m_Power );
		if ( iSide == -1 )
		{
			return false;
		}

		CDispUtilsHelper *pTest = TransformIntoNeighbor( pNeighbor, iSide, nbIndex, testIndex );
		
		if( pTest != pDisp || nodeIndex != testIndex )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void VerifyNeighborConnections( CCoreDispInfo **ppListBase, int nDisps )
{
	while ( 1 )
	{
		bool bHappy = true;

		int iDisp;
		for ( iDisp = 0; iDisp < nDisps; ++iDisp )
		{
			CCoreDispInfo *pDisp = ppListBase[iDisp];
			CDispUtilsHelper *pHelper = pDisp;

			for ( int iEdge=0; iEdge < 4; iEdge++ )
			{
				CDispEdgeIterator it( pHelper, iEdge );
				while ( it.Next() )
				{
					if ( !VerifyNeighborVertConnection( pHelper, it.GetVertIndex(), it.GetCurrentNeighbor(), it.GetNBVertIndex(), iEdge ) )
					{
						pDisp->GetEdgeNeighbor( iEdge )->SetInvalid();
						Warning( "Warning: invalid neighbor connection on displacement near (%.2f %.2f %.2f)\n", VectorExpand( pDisp->GetCornerPoint(0) ) );
						bHappy = false;
					}
				}			
			}
		}

		if ( bHappy )
			break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void FindNeighboringDispSurfs( CCoreDispInfo **ppListBase, int nListSize )
{
	// First, clear all neighboring data.
	int iDisp;
	for ( iDisp = 0; iDisp < nListSize; ++iDisp )
	{
		ClearNeighborData( ppListBase[iDisp] );
	}

	CUtlVector<CDispBox> boxes;
	SetupDispBoxes( ppListBase, nListSize, boxes );	

	int nCornerOverflows = 0;

	// Now test all pairs of displacements and setup neighboring relations between them.
	for( iDisp = 0; iDisp < nListSize; ++iDisp )
	{
		CCoreDispInfo *pMain = ppListBase[iDisp];

		for ( int iDisp2 = iDisp+1; iDisp2 < nListSize; ++iDisp2 )
		{
			CCoreDispInfo *pOther = ppListBase[iDisp2];

			// Trivial reject.
			if ( !DoBBoxesTouch( boxes[iDisp], boxes[iDisp2] ) )
				continue;

			SetupEdgeNeighbors( pMain, pOther );
			
			// NOTE: this must come after SetupEdgeNeighbors because it makes sure not to add
			// corner neighbors for disps that are already edge neighbors.
			SetupCornerNeighbors( pMain, pOther, &nCornerOverflows );
		}
	}

	if ( nCornerOverflows )
	{
		Warning( "Warning: overflowed %d displacement corner-neighbor lists.", nCornerOverflows );
	}

	// Debug check.. make sure the neighbor connections are intact (make sure that any
	// edge vert that gets mapped into a neighbor gets mapped back the same way).
	VerifyNeighborConnections( ppListBase, nListSize );
}

//=============================================================================
//
// Allowable verts.
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int IsCorner( CVertIndex const &index, int sideLength )
{
	if ( index.x == 0 )
	{
		if ( index.y == 0 )
			return true;
		else if ( index.y == sideLength-1 )
			return true;
	}
	else if ( index.x == sideLength-1 )
	{
		if ( index.y == 0 )
			return true;
		else if ( index.y == sideLength-1 )
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsVertAllowed( CDispUtilsHelper *pDisp, CVertIndex const &sideVert, int iLevel )
{
	if ( IsCorner( sideVert, pDisp->GetPowerInfo()->GetSideLength() ) )
		return true;

	int iSide = GetEdgeIndexFromPoint( sideVert, pDisp->GetPowerInfo()->GetPower() );
	if ( iSide == -1 )
		return true;

	int iSub = GetSubNeighborIndex( pDisp, iSide, sideVert );
	if ( iSub == -1 )
		return true;

	CDispSubNeighbor *pSub = &pDisp->GetEdgeNeighbor( iSide )->m_SubNeighbors[iSub];
	CDispUtilsHelper *pNeighbor = pDisp->GetDispUtilsByIndex( pSub->m_iNeighbor );
	Assert( pNeighbor );

	// Ok, there is a neighbor.. see if this vertex exists in the neighbor.
	CShiftInfo *pShiftInfo = &g_ShiftInfos[pSub->m_Span][pSub->m_NeighborSpan];
	Assert( pShiftInfo->m_bValid );

	if ( ( pNeighbor->GetPowerInfo()->GetPower() + pShiftInfo->m_PowerShiftAdd ) < ( iLevel+1 ) )
	{
		return false;
	}

	// Ok, it exists. Make sure the neighbor hasn't disallowed it.
	CVertIndex nbIndex;
	TransformIntoSubNeighbor( pDisp, iSide, iSub, sideVert, nbIndex );
	
	CBitVec<MAX_DISPVERTS> &allowedVerts = CCoreDispInfo::FromDispUtils( pNeighbor )->GetAllowedVerts();
	return !!allowedVerts.Get( pNeighbor->VertIndexToInt( nbIndex ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void UnallowVerts_R( CDispUtilsHelper *pDisp, CVertIndex const &nodeIndex, int &nUnallowed )
{
	int iNodeIndex = pDisp->VertIndexToInt( nodeIndex );
	
	CCoreDispInfo *pCoreDisp = CCoreDispInfo::FromDispUtils( pDisp );	
	if ( !pCoreDisp->GetAllowedVerts().Get( iNodeIndex ) )
		return;

	nUnallowed++;
	pCoreDisp->GetAllowedVerts().Clear( iNodeIndex );

	for ( int iDep=0; iDep < CVertInfo::NUM_REVERSE_DEPENDENCIES; iDep++ )
	{
		CVertDependency &dep = pDisp->GetPowerInfo()->m_pVertInfo[iNodeIndex].m_ReverseDependencies[iDep];

		if( dep.m_iVert.x != -1 && dep.m_iNeighbor == -1 )
		{
			UnallowVerts_R( pDisp, dep.m_iVert, nUnallowed );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DisableUnallowedVerts_R( CDispUtilsHelper *pDisp, CVertIndex const &nodeIndex, int iLevel, int &nUnallowed )
{
	int iNodeIndex = pDisp->VertIndexToInt( nodeIndex );

	// This vertex is not allowed if it is on an edge with a neighbor
	// that does not have this vertex.

	// Test side verts.
	for( int iSide=0; iSide < 4; iSide++ )
	{
		CVertIndex const &sideVert = pDisp->GetPowerInfo()->m_pSideVerts[iNodeIndex].m_Verts[iSide];

		if( !IsVertAllowed( pDisp, sideVert, iLevel ) )
		{
			// This vert (and its dependencies) can't exist.
			UnallowVerts_R( pDisp, sideVert, nUnallowed );
		}
	}

#if 0
	// Test dependencies.
	for( int iDep=0; iDep < 2; iDep++ )
	{
		CVertDependency const &dep = pDisp->GetPowerInfo()->m_pVertInfo[iNodeIndex].m_Dependencies[iDep];

		if( dep.m_iNeighbor == -1 && !IsVertAllowed( pDisp, dep.m_iVert, iLevel ) )
		{
			UnallowVerts_R( pDisp, nodeIndex, nUnallowed );
		}
	}
#endif

	// Recurse.
	if( iLevel+1 < pDisp->GetPower() )
	{
		for( int iChild=0; iChild < 4; iChild++ )
		{
			DisableUnallowedVerts_R( pDisp, pDisp->GetPowerInfo()->m_pChildVerts[iNodeIndex].m_Verts[iChild], iLevel+1, nUnallowed );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SetupAllowedVerts( CCoreDispInfo **ppListBase, int nListSize )
{
	// Set all verts to allowed to start with.
	int iDisp;
	for ( iDisp = 0; iDisp < nListSize; ++iDisp )
	{
		ppListBase[iDisp]->GetAllowedVerts().SetAll();
	}

	// Disable verts that need to be disabled so higher-powered displacements remove
	// the necessary triangles when bordering lower-powered displacements.	
	// It is necessary to loop around here because disabling verts can accumulate into 
	// neighbors.
	bool bContinue;
	do
	{
		bContinue = false;
		for( iDisp = 0; iDisp < nListSize; ++iDisp )
		{
			CDispUtilsHelper *pDisp = ppListBase[iDisp];
			
			int nUnallowed = 0;
			DisableUnallowedVerts_R( pDisp, pDisp->GetPowerInfo()->m_RootNode, 0, nUnallowed );
			if ( nUnallowed )
				bContinue = true;
		}
	} while( bContinue );
}
