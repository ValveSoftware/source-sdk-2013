//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module defines the CPowerInfo class, which contains a 
//          whole bunch of precalculated data for each displacement power.
//          It holds data that indicates how to tesselate, how to access
//          neighbor displacements, etc.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISP_POWERINFO_H
#define DISP_POWERINFO_H
#ifdef _WIN32
#pragma once
#endif


#include "disp_vertindex.h"
#include "bspfile.h"


#define NUM_POWERINFOS				(MAX_MAP_DISP_POWER+1)


struct DispNodeInfo_t
{
	enum
	{
		// Indicates if any children at all have triangles
		CHILDREN_HAVE_TRIANGLES = 0x1
	};


	// Indicates which tesselation indices are associated with a node
	unsigned short	m_FirstTesselationIndex;
	unsigned char	m_Count;
	unsigned char	m_Flags;
};


// ------------------------------------------------------------------------ //
// CTesselateWindings are used to tell what order a node needs to visit
// vertices while tesselating.
// ------------------------------------------------------------------------ //
class CTesselateVert
{
public:
				CTesselateVert( CVertIndex const &index, int iNode );

	CVertIndex	m_Index;
	short			m_iNode;	// Which node this vert is a part of (-1 on left, right, up, and down).
};


class CTesselateWinding
{
public:
	CTesselateVert	*m_Verts;
	short			m_nVerts;	// (includes the last vert)
};


class CVertDependency
{
public:
	
	// Returns false if there is no dependency stored here.
	bool		IsValid()	{ return m_iVert.x != -1; }


public:
			
	// The vert index is in the same power as the source displacement.
	// It is also wrapped, so for example, on the middle of the right edge
	// of a 3x3, it will have a dependency on the 3x3's root node (1,1), and it
	// will have another (1,1) entry that references a neighbor.
	CVertIndex	m_iVert;

	// This is -1 if the vert exists inside the source displacement.
	// It is one of the NEIGHBOREDGE_ codes above if it reaches into a neighbor.
	short		m_iNeighbor;
};


// Precalculated data about displacement vertices.
class CVertInfo
{
public:
					CVertInfo();

	// These are the vertices that this vertex depends on (vertices that must be 
	// active for this vert to exist).
	CVertDependency	m_Dependencies[2];

	// These are the vertices that have this vert in their m_Dependencies.
	enum			{ NUM_REVERSE_DEPENDENCIES=4 };
	CVertDependency	m_ReverseDependencies[NUM_REVERSE_DEPENDENCIES];

	short			m_iNodeLevel;		// -1 if this is not a node. Otherwise, the recursion level
										// of this node (root node = 1).
	CVertIndex		m_iParent;			// x=-1 if this is a not a node or if it's the root node.
};


class CTwoUShorts
{
public:
	unsigned short	m_Values[2];
};


class CFourVerts
{
public:
	CVertIndex		m_Verts[4];
};


// Used for referencing triangles in the fully-tesselated displacement by index.
class CTriInfo
{
public:
	unsigned short	m_Indices[3];
};


// Precalculated data for displacements of a certain power.
class CPowerInfo
{
public:
						CPowerInfo( 
							CVertInfo *pVertInfo, 
							CFourVerts *pSideVerts,
							CFourVerts *pChildVerts,
							CFourVerts *pSideVertCorners,
							CTwoUShorts *pErrorEdges,
							CTriInfo *pTriInfos );

	int					GetPower() const		{ return m_Power; }
	int					GetSideLength() const	{ return m_SideLength; }
	const CVertIndex&	GetRootNode() const		{ return m_RootNode; }
	int					GetMidPoint() const		{ return m_MidPoint; } // Half the edge length.

	// Get at the tri list.
	int					GetNumTriInfos() const		{ return m_nTriInfos; }
	const CTriInfo*		GetTriInfo( int i ) const	{ return &m_pTriInfos[i]; }

	// Get the number of vertices in a displacement of this power.
	int					GetNumVerts() const			{ return m_MaxVerts; }
	
	// Return a corner point index. Indexed by the CORNER_ defines.
	const CVertIndex&	GetCornerPointIndex( int iCorner ) const;


public:

	CVertInfo			*m_pVertInfo;
	CFourVerts			*m_pSideVerts;	// The 4 side verts for each node.
	CFourVerts			*m_pChildVerts;	// The 4 children for each node.
	CFourVerts			*m_pSideVertCorners;
	CTwoUShorts			*m_pErrorEdges;			// These are the edges
												// that are used to measure the screenspace
												// error with respect to each vert.

	CTriInfo			*m_pTriInfos;
	int					m_nTriInfos;
	
	int					m_Power;

	CVertIndex			m_RootNode;
	int					m_SideLength;
	int					m_SideLengthM1;			// Side length minus 1.
	int					m_MidPoint;				// Side length / 2.
	int					m_MaxVerts;				// m_SideLength * m_SideLength
	int					m_NodeCount;			// total # of nodes, including children

	// Precalculated increments if you're using a bit vector to represent nodes.
	// Starting at level 0 of the tree, this stores the increment between the nodes at this
	// level. Vectors holding node data are stored in preorder traversal, and these
	// increments tell the number of elements between nodes at each level.
	int					m_NodeIndexIncrements[MAX_MAP_DISP_POWER];

	CVertIndex			m_EdgeStartVerts[4];
	CVertIndex			m_EdgeIncrements[4];

	CVertIndex			m_NeighborStartVerts[4][4];	// [side][orientation]
	CVertIndex			m_NeighborIncrements[4][4];	// [side][orientation]


private:
	friend void InitPowerInfo( CPowerInfo *pInfo, int iMaxPower );

	CVertIndex			m_CornerPointIndices[4];
};


// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

// Indexed by the TWINDING_ enums.
extern CTesselateWinding	g_TWinding;


// ----------------------------------------------------------------------------- //
// Functions.
// ----------------------------------------------------------------------------- //

// Valid indices are MIN_MAP_DISP_POWER through (and including) MAX_MAP_DISP_POWER.
const CPowerInfo* GetPowerInfo( int iPower );


#endif // DISP_POWERINFO_H
