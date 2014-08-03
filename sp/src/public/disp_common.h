//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISP_COMMON_H
#define DISP_COMMON_H
#ifdef _WIN32
#pragma once
#endif

#include "disp_vertindex.h"
#include "bspfile.h"
#include "utlvector.h"

class CPowerInfo;
class CCoreDispInfo;

// ----------------------------------------------------------------------------- //
// Classes.
// ----------------------------------------------------------------------------- //

// This class provides a set of utility functions for displacements that work in the tools and the engine.
abstract_class CDispUtilsHelper
{
// Derived classes must implement these.
public:
	virtual const CPowerInfo*		GetPowerInfo() const = 0;
	virtual CDispNeighbor*			GetEdgeNeighbor( int index ) = 0;
	virtual CDispCornerNeighbors*	GetCornerNeighbors( int index ) = 0;
	virtual CDispUtilsHelper*		GetDispUtilsByIndex( int index ) = 0;

// Helper functions.
public:

	int					GetPower() const;
	int					GetSideLength() const;
	const CVertIndex&	GetCornerPointIndex( int iCorner ) const;
	int					VertIndexToInt( const CVertIndex &i ) const;
	CVertIndex			GetEdgeMidPoint( int iEdge ) const;
};


// Use this to walk along two neighboring displacements and touch all the
// common vertices.
class CDispSubEdgeIterator
{
public:
						
						CDispSubEdgeIterator();

	// Normally, this will iterate all shared verts along the edge except the corners.
	// If you want the corners to be touched too, then pass in bTouchCorners=true.
	void				Start( CDispUtilsHelper *pDisp, int iEdge, int iSub, bool bTouchCorners = false );
	bool				Next();

	const CVertIndex&	GetVertIndex() const	{ return m_Index; }		// Get the vert index for the displacement in pUtils.
	const CVertIndex&	GetNBVertIndex() const	{ return m_NBIndex; }	// Get the neighbor's vert index.
	CDispUtilsHelper*	GetNeighbor() const		{ return m_pNeighbor; }

	// Returns true if you're on the last vert (ie: the next Next() call will return false).ssssss
	bool				IsLastVert() const;


private:
	CDispUtilsHelper	*m_pNeighbor;	// The neighbor to the edge we were setup on.

	CVertIndex			m_Index;
	CVertIndex			m_Inc;
	
	CVertIndex			m_NBIndex;
	CVertIndex			m_NBInc;

	int					m_End;
	int					m_FreeDim;
};


// Use this to walk along the edge of a displacement, touching the points in common
// between the two neighbors. Note: this won't hit the corner points of any of the displacements.
// (As a result, it won't hit the midpoint of pDisps's edge if there are 2 neighbors).
class CDispEdgeIterator
{
public:
						CDispEdgeIterator( CDispUtilsHelper *pDisp, int iEdge );

	// Seek to the next point on the edge.
	bool				Next();

	const CVertIndex&	GetVertIndex() const	{ return m_It.GetVertIndex(); }		// Get the vert index for the displacement in pUtils.
	const CVertIndex&	GetNBVertIndex() const	{ return m_It.GetNBVertIndex(); }	// Get the neighbor's vert index.

	// What is the current neighbor?
	CDispUtilsHelper*	GetCurrentNeighbor() const	{ return m_It.GetNeighbor(); }


private:
	CDispUtilsHelper		*m_pDisp;
	int						m_iEdge;
	int						m_iCurSub;

	CDispSubEdgeIterator	m_It;
};


// Use this to walk all the corners and edge verts in the displacement.
// It walks the edges in the order of the NEIGHBOREDGE_ defines.
// Iterate like this:
// CDispCircumferenceIterator iterator( pDisp->GetSideLength() );
// while ( iterator.Next() )
//     ...
class CDispCircumferenceIterator
{
public:
						CDispCircumferenceIterator( int sideLength );

	// Seek to the next point. Returns false when there are no more points.
	bool				Next();

	const CVertIndex&	GetVertIndex() const	{ return m_VertIndex; }


private:
	int					m_SideLengthM1;
	int					m_iCurEdge;
	CVertIndex			m_VertIndex;
};


// These store info about how to scale and shift coordinates between neighbors
// of different relations (in g_ShiftInfos).	
class CShiftInfo
{
public:
	int		m_MidPointScale;
	int		m_PowerShiftAdd;
	bool	m_bValid;
};

class CDispBox
{
public:
	Vector	m_Min, m_Max;
};

// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //


extern int			g_EdgeDims[4];		// This tells which dimension (0 or 1) is locked on an edge for each NEIGHBOREDGE_ enum.
extern CShiftInfo	g_ShiftInfos[3][3];	// See CShiftInfo.
extern int			g_EdgeSideLenMul[4];// Multiply these by the side length to get the index of the edge.


// ----------------------------------------------------------------------------- //
// Helper functions.
// ----------------------------------------------------------------------------- //

// Reference implementation to generate triangle indices for a displacement.
int		DispCommon_GetNumTriIndices( int power );
void	DispCommon_GenerateTriIndices( int power, unsigned short *indices );

// Returns a NEIGHBOREDGE_ value for the edge that the index is on.
// Returns -1 if the index is not on a side.
// If the point is on a corner, the edges are tested in the order of the NEIGHBOREDGE_ defines.
int		GetEdgeIndexFromPoint( CVertIndex const &index, int iPower );

// Returns a CORNER_ value for the corner the point is on, or -1 if it's not on a corner.
int		GetEdgeIndexFromPoint( CVertIndex const &index, int iPower );

// This returns the neighbor's power, possibly +1 or -1.
//
// It will add one if the neighbor takes up half of your edge (ie: if it took up your 
// whole edge, its resolution would be twice what it really is).
//
// It will subtract one if you take up half of its edge (ie: you only touch half of its verts).
//
// Returns -1 if the edge connection is invalid.
int		GetNeighborEdgePower( CDispUtilsHelper *pDisp, int iEdge, int iSub );

// This function sets you up so you can walk along an edge that joins two neighbors.
// Add myInc to myIndex and nbInc to nbIndex until myIndex[iFreeDim] >= myEnd.
//
// Returns the neighbor displacement, or NULL if the specified sub neighbor isn't valid.
CDispUtilsHelper*	SetupEdgeIncrements(
	CDispUtilsHelper *pDisp,
	int iEdge,
	int iSub,
	CVertIndex &myIndex,
	CVertIndex &myInc,
	CVertIndex &nbIndex,
	CVertIndex &nbInc,
	int &myEnd,
	int &iFreeDim );

// Figure out which sub neighbor nodeIndex touches.
// Returns -1 if there is no valid sub neighbor at the specified index.
int GetSubNeighborIndex( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex
	);

// Given a vert index and the CSubNeighbor the vert lies on, this
// transforms the specified vert into the neighbor's space.
//
// Note: for corner verts, there may be multiple neighbors touching the same vert, so the
//       result you get depends on the edge you specify in iEdge (ie: if you specify the same
//       node index but a different edge, you may get a different neighbor).
//
// Note: This only returns a point if the point at nodeIndex actually touches a neighbor point.
//       An example where this might be unexpected is if pDisp is power 4 and its neighbor on iEdge
//       is power 3, and nodeIndex points at a vert in between two of its neighbor's verts.
//       In that case, even though there is a neighbor displacement, nodeIndex doesn't touch 
//       any points on it, so NULL is returned.
CDispUtilsHelper*	TransformIntoSubNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge, 
	int iSub,
	CVertIndex const &nodeIndex, 
	CVertIndex &out 
	);

// Transform pDisp's node at nodeIndex into its neighboring connection.
// Returns the neighbor displacement and sets out to the index in the neighbor.
//
// Note: for corner verts, there may be multiple neighbors touching the same vert, so the
//       result you get depends on the edge you specify in iEdge (ie: if you specify the same
//       node index but a different edge, you may get a different neighbor).
//
// Note: This only returns a point if the point at nodeIndex actually touches a neighbor point.
//       An example where this might surprise you is if pDisp is power 4 and its neighbor on iEdge
//       is power 3, and nodeIndex points at a vert in between two of its neighbor's verts.
//       In that case, even though there is a neighbor displacement, nodeIndex doesn't touch 
//       any points on it, so NULL is returned.
CDispUtilsHelper* TransformIntoNeighbor( 
	CDispUtilsHelper *pDisp,
	int iEdge,
	CVertIndex const &nodeIndex, 
	CVertIndex &out );

// Returns true if the specified point has one or more neighbors.
bool DoesPointHaveAnyNeighbors( 
	CDispUtilsHelper *pDisp,
	const CVertIndex &index );


void FindNeighboringDispSurfs( CCoreDispInfo **ppListBase, int nListSize );
void SetupAllowedVerts( CCoreDispInfo **ppListBase, int nListSize );
void GetDispBox( CCoreDispInfo *pDisp, CDispBox &box );

// ----------------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------------- //

#include "disp_powerinfo.h"


#endif // DISP_COMMON_H
