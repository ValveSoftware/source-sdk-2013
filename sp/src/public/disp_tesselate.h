//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISP_TESSELATE_H
#define DISP_TESSELATE_H
#ifdef _WIN32
#pragma once
#endif


#include "disp_powerinfo.h"


inline int InternalVertIndex( const CPowerInfo *pInfo, const CVertIndex &vert )
{
	return vert.y * pInfo->m_SideLength + vert.x;	
}


template< class TesselateHelper >
inline void InternalEndTriangle( 
	TesselateHelper *pHelper,
	CVertIndex const &nodeIndex, 
	int &iCurTriVert )
{
	// End our current triangle here.
	Assert( iCurTriVert == 2 );
	
	// Finish the triangle.
	pHelper->m_TempIndices[2] = (unsigned short)InternalVertIndex( pHelper->m_pPowerInfo, nodeIndex ); 

	pHelper->EndTriangle();

	// Add on the last vertex to join to the next triangle.
	pHelper->m_TempIndices[0] = pHelper->m_TempIndices[1];
	iCurTriVert = 1;
}


//-----------------------------------------------------------------------------
// Tesselates a single node, doesn't deal with hierarchy
//-----------------------------------------------------------------------------
template< class TesselateHelper >
inline void TesselateDisplacementNode( 
	TesselateHelper *pHelper,
	CVertIndex const &nodeIndex, 
	int iLevel, 
	int *pActiveChildren )
{
	int iPower = pHelper->m_pPowerInfo->m_Power - iLevel;
	int vertInc = 1 << (iPower - 1);

	CTesselateWinding *pWinding = &g_TWinding;

	// Starting at the bottom-left, wind clockwise picking up vertices and
	// generating triangles.
	int iCurTriVert = 0;
	for( int iVert=0; iVert < pWinding->m_nVerts; iVert++ )
	{
		CVertIndex sideVert = BuildOffsetVertIndex( nodeIndex, pWinding->m_Verts[iVert].m_Index, vertInc );
		
		int iVertNode = pWinding->m_Verts[iVert].m_iNode;
		bool bNode = (iVertNode != -1) && pActiveChildren[iVertNode];
		if( bNode )
		{
			if( iCurTriVert == 2 )
				InternalEndTriangle( pHelper, nodeIndex, iCurTriVert );
			
			iCurTriVert = 0;
		}
		else
		{
			int iVertBit = InternalVertIndex( pHelper->m_pPowerInfo, sideVert );
			if( pHelper->m_pActiveVerts[iVertBit>>5] & (1 << (iVertBit & 31)) )
			{
				// Ok, add a vert here.
				pHelper->m_TempIndices[iCurTriVert] = (unsigned short)InternalVertIndex( pHelper->m_pPowerInfo, sideVert );
				iCurTriVert++;
				if( iCurTriVert == 2 )
					InternalEndTriangle( pHelper, nodeIndex, iCurTriVert );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Tesselates in a *breadth first* fashion
//-----------------------------------------------------------------------------
template< class T >
inline void TesselateDisplacement_R( 
	T *pHelper,
	const CVertIndex &nodeIndex,
	int iNodeBitIndex,
	int iLevel
	)
{
	// Here's the node info for our current node
	Assert( iNodeBitIndex < pHelper->m_pPowerInfo->m_NodeCount );
	DispNodeInfo_t& nodeInfo = pHelper->GetNodeInfo( iNodeBitIndex );

	// Store off the current number of indices
	int oldIndexCount = pHelper->m_nIndices;

	// Go through each quadrant. If there is an active child node, recurse down.
	int bActiveChildren[4];
	if( iLevel >= pHelper->m_pPowerInfo->m_Power - 1 )
	{
		// This node has no children.
		bActiveChildren[0] = bActiveChildren[1] = bActiveChildren[2] = bActiveChildren[3] = false;
	}
	else
	{
		int iNodeIndex = InternalVertIndex( pHelper->m_pPowerInfo, nodeIndex );

		int iChildNodeBit = iNodeBitIndex + 1;
		for( int iChild=0; iChild < 4; iChild++ )
		{
			CVertIndex const &childNode = pHelper->m_pPowerInfo->m_pChildVerts[iNodeIndex].m_Verts[iChild];

			// Make sure we really can tesselate here (a smaller neighbor displacement could
			// have inactivated certain edge verts.
			int iVertBit = InternalVertIndex( pHelper->m_pPowerInfo, childNode );
			bActiveChildren[iChild] = ( pHelper->m_pActiveVerts[iVertBit>>5] & (1 << (iVertBit & 31)) );

			if( bActiveChildren[iChild] )
			{
				TesselateDisplacement_R( pHelper, childNode, iChildNodeBit, iLevel+1 );
			}
			else
			{
				// Make sure the triangle counts are cleared on this one because it may visit this
				// node in GenerateDecalFragments_R if nodeInfo's CHILDREN_HAVE_TRIANGLES flag is set.
				DispNodeInfo_t &childInfo = pHelper->GetNodeInfo( iChildNodeBit );
				childInfo.m_Count = 0;
				childInfo.m_Flags = 0;
			}

			iChildNodeBit += pHelper->m_pPowerInfo->m_NodeIndexIncrements[iLevel];
		}
	}

	// Set the child field
	if ( pHelper->m_nIndices != oldIndexCount )
	{
		nodeInfo.m_Flags = DispNodeInfo_t::CHILDREN_HAVE_TRIANGLES;
		oldIndexCount = pHelper->m_nIndices;
	}
	else
	{
		nodeInfo.m_Flags = 0;
	}

	// Now tesselate the node itself...
	TesselateDisplacementNode( pHelper, nodeIndex, iLevel, bActiveChildren );

	// Now that we've tesselated, figure out how many indices we've added at this node
	nodeInfo.m_Count = pHelper->m_nIndices - oldIndexCount;
	nodeInfo.m_FirstTesselationIndex = oldIndexCount;
	Assert( nodeInfo.m_Count % 3 == 0 );
}


class CBaseTesselateHelper
{
public:

	// Functions your derived class must implement:
	// void EndTriangle();								// (the 3 indices are in m_TempIndices).
	// DispNodeInfo_t& GetNodeInfo( int iNodeBit );

	
	// Set these before calling TesselateDisplacement.
	uint32 *m_pActiveVerts;		// These bits control the tesselation.
	const CPowerInfo *m_pPowerInfo;								// Lots of precalculated data about a displacement this size.
	
	
	// Used internally by TesselateDisplacement.
	int m_nIndices;						// After calling TesselateDisplacement, this is set to the # of indices generated.
	unsigned short m_TempIndices[6];
};



// This interface is shared betwixt VBSP and the engine. VBSP uses it to build the 
// physics mesh and the engine uses it to render.
//
// To use this function, derive a class from CBaseTesselateHelper that supports the TesselateHelper functions.
template< class TesselateHelper >
inline void TesselateDisplacement( TesselateHelper *pHelper )
{
	pHelper->m_nIndices = 0;
	
	TesselateDisplacement_R<TesselateHelper>(
		pHelper,
		pHelper->m_pPowerInfo->m_RootNode,
		0,			// node bit indexing CDispDecal::m_NodeIntersects
		0 );
}


#endif // DISP_TESSELATE_H
