//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BUILDDISP_H
#define BUILDDISP_H

#ifdef _WIN32
#pragma once
#endif

#include "commonmacros.h"
#include "tier0/dbg.h"
#include "bspfile.h"
#include "mathlib/mathlib.h"
#include "mathlib/bumpvects.h"
#include "disp_common.h"
#include "bitvec.h"

#define DISP_ALPHA_PROP_DELTA		382.5f

class CCoreDispInfo;

struct CoreDispBBox_t
{
	Vector vMin, vMax;
};

//=========================================================================
//
// Surface Class - interfacing class (fill in with MapFace, dface_t, and 
//                                    msurface_t)
//
class CCoreDispSurface
{
public:

	enum { QUAD_POINT_COUNT = 4 };
	enum { MAX_CORNER_NEIGHBOR_COUNT = 16 };

	CCoreDispSurface();

	//=========================================================================
	//
	// initialization
	//
	void Init( void );

	//=========================================================================
	//
	// parent surface id - index to CMapFace, dface_t, or msurface_t
	//
	inline void SetHandle( int handle );
	inline int GetHandle( void );

	//=========================================================================
	//
	// vertex data - pos, normal, texture, lightmap, alpha, etc...
	//
	inline void SetPointCount( int count );
	inline int GetPointCount( void ) const;

	inline void SetPoint( int index, Vector const &pt );
	inline void GetPoint( int index, Vector& pt ) const;
	inline Vector const& GetPoint( int index ) const;

	inline void SetPointNormal( int index, Vector const &normal );
	inline void GetPointNormal( int index, Vector &normal );
	inline void SetTexCoord( int index, Vector2D const& texCoord );
	inline void GetTexCoord( int index, Vector2D& texCoord ) const;

	inline void SetLuxelCoord( int bumpIndex, int index, Vector2D const& luxelCoord );
	inline void GetLuxelCoord( int bumpIndex, int index, Vector2D& luxelCoord ) const;
	inline void SetLuxelCoords( int bumpIndex, Vector2D const coords[4] );
	inline void GetLuxelCoords( int bumpIndex, Vector2D coords[4] ) const;

	inline void SetLuxelU( int nU )			{ m_nLuxelU = nU; }
	inline int	GetLuxelU( void )			{ return m_nLuxelU; }
	inline void SetLuxelV( int nV )			{ m_nLuxelV = nV; }
	inline int	GetLuxelV( void )			{ return m_nLuxelV; }
	bool		CalcLuxelCoords( int nLuxels, bool bAdjust, const Vector &vecU, const Vector &vecV );

	inline void SetAlpha( int index, float alpha );
	inline float GetAlpha( int const index ) const;

	//=========================================================================
	//
	// utils
	//
	inline void GetNormal( Vector& normal );
	inline void SetFlags( int flag );
	inline int GetFlags( void );
	inline void SetContents( int contents );
	inline int GetContents( void );

	//=========================================================================
	//
	// create utils (texture axis not use anymore but here to support older maps)
	//
	inline void SetSAxis( Vector const &axis );
	inline void GetSAxis( Vector &axis );
	inline void SetTAxis( Vector const &axis );
	inline void GetTAxis( Vector &axis );

	inline void SetPointStartIndex( int index );
	inline int GetPointStartIndex( void );
	inline void SetPointStart( Vector const &pt );
	inline void GetPointStart( Vector &pt );

	// Used by the tools to set the neighbor data from the BSP file.
	void	SetNeighborData( const CDispNeighbor edgeNeighbors[4], const CDispCornerNeighbors cornerNeighbors[4] );

    void	GeneratePointStartIndexFromMappingAxes( Vector const &sAxis, Vector const &tAxis );
	int		GenerateSurfPointStartIndex( void );
	int		FindSurfPointStartIndex( void );
	void	AdjustSurfPointData( void );
	
	// Indexed by CORNER_ defines.
	CDispCornerNeighbors*		GetCornerNeighbors( int iCorner )		{ Assert( iCorner >= 0 && iCorner < ARRAYSIZE( m_CornerNeighbors ) ); return &m_CornerNeighbors[iCorner]; }
	const CDispCornerNeighbors*	GetCornerNeighbors( int iCorner ) const { Assert( iCorner >= 0 && iCorner < ARRAYSIZE( m_CornerNeighbors ) ); return &m_CornerNeighbors[iCorner]; }
	
	// Indexed by CORNER_ defines.
	int							GetCornerNeighborCount( int iCorner ) const				{ return GetCornerNeighbors( iCorner )->m_nNeighbors; }
	int							GetCornerNeighbor( int iCorner, int iNeighbor ) const	{ Assert( iNeighbor >= 0 && iNeighbor < GetCornerNeighbors(iCorner)->m_nNeighbors ); return GetCornerNeighbors( iCorner )->m_Neighbors[iNeighbor]; }
	
	CDispNeighbor*			GetEdgeNeighbor( int iEdge )		{ Assert( iEdge >= 0 && iEdge < ARRAYSIZE( m_EdgeNeighbors ) ); return &m_EdgeNeighbors[iEdge]; }
	const CDispNeighbor*	GetEdgeNeighbor( int iEdge ) const	{ Assert( iEdge >= 0 && iEdge < ARRAYSIZE( m_EdgeNeighbors ) ); return &m_EdgeNeighbors[iEdge]; }


protected:

	// Utility
	bool		LongestInU( const Vector &vecU, const Vector &vecV );


	int			m_Index;																// parent face (CMapFace, dface_t, msurface_t) index "handle"
	
	int			m_PointCount;															// number of points in the face (should be 4!)
	Vector		m_Points[QUAD_POINT_COUNT];												// points
	Vector		m_Normals[QUAD_POINT_COUNT];											// normals at points
	Vector2D	m_TexCoords[QUAD_POINT_COUNT];											// texture coordinates at points
	Vector2D	m_LuxelCoords[NUM_BUMP_VECTS+1][QUAD_POINT_COUNT];						// lightmap coordinates at points
	float		m_Alphas[QUAD_POINT_COUNT];												// alpha at points

	// Luxels sizes
	int					m_nLuxelU;
	int					m_nLuxelV;

	// Straight from the BSP file.	
	CDispNeighbor			m_EdgeNeighbors[4];
	CDispCornerNeighbors	m_CornerNeighbors[4];

    int			m_Flags;																// surface flags - inherited from the "parent" face
	int			m_Contents;																// contents flags - inherited from the "parent" face

	Vector		sAxis;																	// used to generate start disp orientation (old method)
	Vector		tAxis;																	// used to generate start disp orientation (old method)
	int			m_PointStartIndex;														// index to the starting point -- for saving starting point
	Vector		m_PointStart;															// starting point used to determine the orientation of the displacement map on the surface
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetHandle( int handle )
{
	m_Index = handle;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispSurface::GetHandle( void )
{
	return m_Index;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetPointCount( int count )
{
	// quad only -- currently!
	if( count != 4 )
		return;
	m_PointCount = count;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispSurface::GetPointCount( void ) const
{
	return m_PointCount;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetPoint( int index, Vector const &pt )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	VectorCopy( pt, m_Points[index] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetPoint( int index, Vector &pt ) const
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	VectorCopy( m_Points[index], pt );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline Vector const& CCoreDispSurface::GetPoint( int index ) const
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	return m_Points[index];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetPointNormal( int index, Vector const &normal )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	VectorCopy( normal, m_Normals[index] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetPointNormal( int index, Vector& normal )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	VectorCopy( m_Normals[index], normal );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetTexCoord( int index, Vector2D const& texCoord )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	Vector2DCopy( texCoord, m_TexCoords[index] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetTexCoord( int index, Vector2D& texCoord ) const
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	Vector2DCopy( m_TexCoords[index], texCoord );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetLuxelCoord( int bumpIndex, int index, Vector2D const& luxelCoord )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	Vector2DCopy( luxelCoord, m_LuxelCoords[bumpIndex][index] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetLuxelCoord( int bumpIndex, int index, Vector2D& luxelCoord ) const
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	Vector2DCopy( m_LuxelCoords[bumpIndex][index], luxelCoord );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetLuxelCoords( int bumpIndex, Vector2D const luxelCoords[4] )
{
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	for( int i=0; i < 4; i++ )
		Vector2DCopy( luxelCoords[i], m_LuxelCoords[bumpIndex][i] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetLuxelCoords( int bumpIndex, Vector2D luxelCoords[4] ) const
{
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	for( int i=0; i < 4; i++ )
		Vector2DCopy( m_LuxelCoords[bumpIndex][i], luxelCoords[i] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetAlpha( int index, float alpha )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	m_Alphas[index] = alpha;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CCoreDispSurface::GetAlpha( int const index ) const
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	return m_Alphas[index];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetFlags( int flag )
{
	m_Flags = flag;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispSurface::GetFlags( void )
{
	return m_Flags;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetContents( int contents )
{
	m_Contents = contents;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispSurface::GetContents( void )
{
	return m_Contents;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetSAxis( Vector const &axis )
{
	VectorCopy( axis, sAxis );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetSAxis( Vector& axis )
{
	VectorCopy( sAxis, axis );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetTAxis( Vector const &axis )
{
	VectorCopy( axis, tAxis );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetTAxis( Vector& axis )
{
	VectorCopy( tAxis, axis );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetPointStartIndex( int index )
{
	Assert( index >= 0 );
	Assert( index < QUAD_POINT_COUNT );
	m_PointStartIndex = index;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispSurface::GetPointStartIndex( void )
{
	return m_PointStartIndex;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::SetPointStart( Vector const& pt )
{
	VectorCopy( pt, m_PointStart );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetPointStart( Vector& pt )
{
	VectorCopy( m_PointStart, pt );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispSurface::GetNormal( Vector& normal )
{
	//
	// calculate the displacement surface normal
	//
	Vector tmp[2];
	VectorSubtract( m_Points[1], m_Points[0], tmp[0] );
	VectorSubtract( m_Points[3], m_Points[0], tmp[1] );
	CrossProduct( tmp[1], tmp[0], normal );
	VectorNormalize( normal );
}


//=========================================================================
//
// Node Class (for displacement quad-tree)
//
class CCoreDispNode
{
public:

	enum { MAX_NEIGHBOR_NODE_COUNT = 4 };
	enum { MAX_NEIGHBOR_VERT_COUNT = 8 };
	enum { MAX_SURF_AT_NODE_COUNT = 8 };

	//=========================================================================
	//
	// Initialization
	//
	void Init( void );

	//=========================================================================
	//
	//
	//
	inline void SetBoundingBox( Vector const& bMin, Vector const& bMax );
	inline void GetBoundingBox( Vector& bMin, Vector& bMax );

	inline void SetErrorTerm( float errorTerm );
	inline float GetErrorTerm( void );

	inline void SetNeighborNodeIndex( int dir, int index );
	inline int GetNeighborNodeIndex( int dir );

	inline void SetCenterVertIndex( int index );
	inline int GetCenterVertIndex( void );
	inline void SetNeighborVertIndex( int dir, int index );
	inline int GetNeighborVertIndex( int dir );

	inline void SetTriBoundingBox( int index, Vector const& bMin, Vector const& bMax );
	inline void GetTriBoundingBox( int index, Vector& bMin, Vector& bMax );
	inline void SetTriPlane( int index, Vector const& normal, float dist );
	inline void GetTriPlane( int index, cplane_t *plane );

	inline void SetRayBoundingBox( int index, Vector const& bMin, Vector const& bMax );
	inline void GetRayBoundingBox( int index, Vector& bMin, Vector& bMax );

	//=========================================================================
	//
	// Node Functions (friend functions)
	//
	friend int GetNodeLevel( int index );
	friend int GetNodeCount( int power );
	friend int GetNodeParent( int index );
	friend int GetNodeChild( int power, int index, int direction );
	friend int GetNodeNeighborNode( int power, int index, int direction, int level );
	friend int GetNodeNeighborNodeFromNeighborSurf( int power, int index, int direction, int level, int neighborOrient );
	friend int GetNodeMinNodeAtLevel( int level );

	friend void GetDispNodeTriVerts( CCoreDispInfo *pDisp, int nodeIndex, int triIndex, float *v1, float *v2, float *v3 );

	friend void GetComponentsFromNodeIndex( int index, int *x, int *y );
	friend int GetNodeIndexFromComponents( int x, int y );

protected:	
		
	Vector		m_BBox[2];											// displacement node bounding box (take into account size of children)
	float		m_ErrorTerm;										// LOD error term (the "precision" of the representation of the surface at this node's level)
	int			m_VertIndex;										// the node's vertex index (center vertex of node)
	int			m_NeighborVertIndices[MAX_NEIGHBOR_VERT_COUNT];		// all other vertex indices in node (maximally creates 8 trianglar surfaces)
	Vector		m_SurfBBoxes[MAX_SURF_AT_NODE_COUNT][2];			// surface bounding boxes - old method
	cplane_t	m_SurfPlanes[MAX_SURF_AT_NODE_COUNT];				// surface plane info - old method

	Vector		m_RayBBoxes[4][2];									// bounding boxes for ray traces
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetBoundingBox( Vector const& bMin, Vector const& bMax )
{
	VectorCopy( bMin, m_BBox[0] );
	VectorCopy( bMax, m_BBox[1] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::GetBoundingBox( Vector& bMin, Vector& bMax )
{
	VectorCopy( m_BBox[0], bMin );
	VectorCopy( m_BBox[1], bMax );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetErrorTerm( float errorTerm )
{
	m_ErrorTerm = errorTerm;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CCoreDispNode::GetErrorTerm( void )
{
	return m_ErrorTerm;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetCenterVertIndex( int index )
{
	m_VertIndex = index;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispNode::GetCenterVertIndex( void )
{
	return m_VertIndex;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetNeighborVertIndex( int dir, int index )
{
	Assert( dir >= 0 );
	Assert( dir < MAX_NEIGHBOR_VERT_COUNT );
	m_NeighborVertIndices[dir] = index;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispNode::GetNeighborVertIndex( int dir )
{
	Assert( dir >= 0 );
	Assert( dir < MAX_NEIGHBOR_VERT_COUNT );
	return m_NeighborVertIndices[dir];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetTriBoundingBox( int index, Vector const& bMin, Vector const& bMax )
{
	Assert( index >= 0 );
	Assert( index < MAX_SURF_AT_NODE_COUNT );
	VectorCopy( bMin, m_SurfBBoxes[index][0] );
	VectorCopy( bMax, m_SurfBBoxes[index][1] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::GetTriBoundingBox( int index, Vector& bMin, Vector& bMax )
{
	Assert( index >= 0 );
	Assert( index < MAX_SURF_AT_NODE_COUNT );
	VectorCopy( m_SurfBBoxes[index][0], bMin );
	VectorCopy( m_SurfBBoxes[index][1], bMax );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetTriPlane( int index, Vector const &normal, float dist )
{
	Assert( index >= 0 );
	Assert( index < MAX_SURF_AT_NODE_COUNT );
	VectorCopy( normal, m_SurfPlanes[index].normal );
	m_SurfPlanes[index].dist = dist;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::GetTriPlane( int index, cplane_t *plane )
{
	Assert( index >= 0 );
	Assert( index < MAX_SURF_AT_NODE_COUNT );
	VectorCopy( m_SurfPlanes[index].normal, plane->normal );
	plane->dist = m_SurfPlanes[index].dist;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::SetRayBoundingBox( int index, Vector const &bMin, Vector const &bMax )
{
	Assert( index >= 0 );
	Assert( index < 4 );
	VectorCopy( bMin, m_RayBBoxes[index][0] );
	VectorCopy( bMax, m_RayBBoxes[index][1] );
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispNode::GetRayBoundingBox( int index, Vector& bMin, Vector& bMax )
{
	Assert( index >= 0 );
	Assert( index < 4 );
	VectorCopy( m_RayBBoxes[index][0], bMin );
	VectorCopy( m_RayBBoxes[index][1], bMax );
}

	
//=============================================================================
//
// CCoreInfoBuilder - the primary data necessay to derive a displacement surface
//                    used by WorldCraft (CMapFace, CMapDisp), VRAD (dface_t, ddispinto_t),
//                    and the engine (msurface_t, CDispInfo)
//

struct CoreDispVert_t
{
	Vector			m_FieldVector;						// displacement vector field
	float			m_FieldDistance;					// the distances along the displacement vector normal

	Vector			m_SubdivNormal;
	Vector			m_SubdivPos;						// used the create curvature of displacements

	// generated displacement surface data
	Vector			m_Vert;								// displacement surface vertices
	Vector			m_FlatVert;
	Vector			m_Normal;							// displacement surface normals
	Vector			m_TangentS;							// use in calculating the tangent space axes
	Vector			m_TangentT;							// use in calculating the tangent space axes
	Vector2D		m_TexCoord;							// displacement surface texture coordinates
	Vector2D		m_LuxelCoords[NUM_BUMP_VECTS+1];	// displacement surface lightmap coordinates

	// additional per-vertex data
	float			m_Alpha;							// displacement alpha values (per displacement vertex)
};

// New, need to use this at the node level
#define	COREDISPTRI_TAG_WALKABLE				(1<<0)
#define COREDISPTRI_TAG_FORCE_WALKABLE_BIT		(1<<1)
#define COREDISPTRI_TAG_FORCE_WALKABLE_VAL		(1<<2)
#define COREDISPTRI_TAG_BUILDABLE				(1<<3)
#define COREDISPTRI_TAG_FORCE_BUILDABLE_BIT		(1<<4)
#define COREDISPTRI_TAG_FORCE_BUILDABLE_VAL		(1<<5)
#define COREDISPTRI_TAG_FORCE_REMOVE_BIT		(1<<6)

struct CoreDispTri_t
{
	unsigned short  m_iIndex[3];						// the three indices that make up a triangle
	unsigned short	m_uiTags;							// walkable, buildable, etc.
};

class CCoreDispInfo : public CDispUtilsHelper
{
public:

	//
	// tree and displacement surface directions
	//
	enum { WEST      = 0,
		   NORTH     = 1,
		   EAST      = 2,
		   SOUTH     = 3,
		   SOUTHWEST = 4,
		   SOUTHEAST = 5,
		   NORTHWEST = 6,
		   NORTHEAST = 7 };

#if 0
	//
	// building parameters
	//
	enum { BUILD_NORMALS      = 0x1,
		   BUILD_TEXCOORDS    = 0x2,
		   BUILD_LIGHTCOORDS  = 0x4,
		   BUILD_LODTREE      = 0x8,
		   BUILD_COLLISION    = 0x10,
		   BUILD_TANGENTSPACE = 0x20 };
#endif

	//
	// surface info flags
	//
	enum { SURF_BUMPED				= 0x1,  
		   SURF_NOPHYSICS_COLL		= 0x2,
		   SURF_NOHULL_COLL			= 0x4,
		   SURF_NORAY_COLL			= 0x8 };

	enum { MAX_DISP_POWER = MAX_MAP_DISP_POWER };
	enum { MAX_VERT_COUNT = MAX_DISPVERTS };
	enum { MAX_NODE_COUNT = 85 };


// Convert from a CDispUtilsHelper.
public:
	
	static CCoreDispInfo*			FromDispUtils( CDispUtilsHelper *p )	{ return (CCoreDispInfo*)p; }


// CDispUtilsHelper implementation.
public:

	virtual CDispNeighbor*			GetEdgeNeighbor( int index );
	virtual CDispCornerNeighbors*	GetCornerNeighbors( int index );
	virtual const CPowerInfo* 		GetPowerInfo() const;
	virtual CDispUtilsHelper*		GetDispUtilsByIndex( int index );


public:

	//=========================================================================
	//
	// Creation/Destruction
	//
	CCoreDispInfo();
	~CCoreDispInfo();

	void InitSurf( int parentIndex, Vector points[4], Vector normals[4],
		           Vector2D texCoords[4], Vector2D lightCoords[4][4], int contents, int flags,
				   bool bGenerateSurfPointStart, Vector& startPoint, 
				   bool bHasMappingAxes, Vector& uAxis, Vector& vAxis );

	void InitDispInfo( int power, int minTess, float smoothingAngle, 
		               float *alphas, Vector *dispVectorField, float *dispDistances );

	// This just unpacks the contents of the verts into arrays and calls InitDispInfo.
	void InitDispInfo( int power, int minTess, float smoothingAngle, const CDispVert *pVerts, const CDispTri *pTris );
					   
//	bool Create( int creationFlags );
	bool Create( void );
	bool CreateWithoutLOD( void );

	//=========================================================================
	//
	// Parameter "Wrappers"
	//
	CCoreDispSurface*		GetSurface()		{ return &m_Surf; }
	const CCoreDispSurface*	GetSurface() const	{ return &m_Surf; }
	
	inline CCoreDispNode *GetNode( int index );

	inline void SetPower( int power );
	inline int GetPower( void ) const;
	inline int GetPostSpacing( void );
	inline int GetWidth( void );
	inline int GetHeight( void );
	inline int GetSize( void ) const;

	// Use this disp as a CDispUtils.
	void SetDispUtilsHelperInfo( CCoreDispInfo **ppListBase, int listSize );

	void SetNeighborData( const CDispNeighbor edgeNeighbors[4], const CDispCornerNeighbors cornerNeighbors[4] ) { GetSurface()->SetNeighborData( edgeNeighbors, cornerNeighbors ); }

	// Get a corner point. Indexed by the CORNER_ defines.
	const CVertIndex&	GetCornerPointIndex( int index ) const	{ return GetPowerInfo()->GetCornerPointIndex( index ); }
	const Vector&		GetCornerPoint( int index ) const		{ return GetVert( VertIndexToInt( GetCornerPointIndex( index ) ) ); }

	inline void SetVert( int index, Vector const& vert );
	inline void GetVert( int index, Vector& vert ) const;
	
	inline const Vector& GetVert( int index ) const;
	inline const Vector& GetVert( const CVertIndex &index ) const;

	inline void GetFlatVert( int index, Vector& vert ) const;
	inline void SetFlatVert( int index, const Vector &vert );
	
	inline void GetNormal( int index, Vector& normal ) const;
	inline const Vector& GetNormal( int index ) const;
	inline const Vector& GetNormal( const CVertIndex &index ) const;
	inline void SetNormal( int index, Vector const& normal );
	inline void SetNormal( const CVertIndex &index, Vector const& normal );
	
	inline void GetTangentS( int index, Vector& tangentS ) const;
	inline const Vector &GetTangentS( int index ) const;
	inline const Vector &GetTangentS( const CVertIndex &index ) const { return GetTangentS(VertIndexToInt(index)); }
	inline void GetTangentT( int index, Vector& tangentT ) const;
	inline void SetTangentS( int index, Vector const& vTangentS ) { m_pVerts[index].m_TangentS = vTangentS; }
	inline void SetTangentT( int index, Vector const& vTangentT ) { m_pVerts[index].m_TangentT = vTangentT; }

	inline void SetTexCoord( int index, Vector2D const& texCoord );
	inline void GetTexCoord( int index, Vector2D& texCoord ) const;
	
	inline void SetLuxelCoord( int bumpIndex, int index, Vector2D const& luxelCoord );
	inline void GetLuxelCoord( int bumpIndex, int index, Vector2D& luxelCoord ) const;

	inline void SetAlpha( int index, float alpha );
	inline float GetAlpha( int index );

	int GetTriCount( void );
	void GetTriIndices( int iTri, unsigned short &v1, unsigned short &v2, unsigned short &v3 );
	void SetTriIndices( int iTri, unsigned short v1, unsigned short v2, unsigned short v3 );
	void GetTriPos( int iTri, Vector &v1, Vector &v2, Vector &v3 );
	inline void SetTriTag( int iTri, unsigned short nTag )		{ m_pTris[iTri].m_uiTags |= nTag; }
	inline void ResetTriTag( int iTri, unsigned short nTag )	{ m_pTris[iTri].m_uiTags &= ~nTag; }
	inline void ToggleTriTag( int iTri, unsigned short nTag )   { m_pTris[iTri].m_uiTags ^= nTag; }
	inline bool IsTriTag( int iTri, unsigned short nTag )       { return ( ( m_pTris[iTri].m_uiTags & nTag ) != 0 ); }
	inline unsigned short GetTriTagValue( int iTri )			{ return m_pTris[iTri].m_uiTags; }
	inline void SetTriTagValue( int iTri, unsigned short nVal ) { m_pTris[iTri].m_uiTags = nVal; }

	bool IsTriWalkable( int iTri );
	bool IsTriBuildable( int iTri );
	bool IsTriRemove( int iTri );

	inline void SetElevation( float elevation );
	inline float GetElevation( void );

	inline void ResetFieldVectors( void );
	inline void SetFieldVector( int index, Vector const &v );
	inline void GetFieldVector( int index, Vector& v );
	inline void ResetFieldDistances( void );
	inline void SetFieldDistance( int index, float dist );
	inline float GetFieldDistance( int index );

	inline void ResetSubdivPositions( void );
	inline void SetSubdivPosition( int ndx, Vector const &v );
	inline void GetSubdivPosition( int ndx, Vector& v );

	inline void ResetSubdivNormals( void );
	inline void SetSubdivNormal( int ndx, Vector const &v );
	inline void GetSubdivNormal( int ndx, Vector &v );

	inline void SetRenderIndexCount( int count );
	inline int GetRenderIndexCount( void );
	inline void SetRenderIndex( int index, int triIndex );
	inline int GetRenderIndex( int index );

	inline CoreDispVert_t *GetDispVert( int iVert )					{ return &m_pVerts[iVert]; }
	inline CoreDispVert_t *GetDispVertList();
	inline unsigned short *GetRenderIndexList( void );

	inline void SetTouched( bool touched );
	inline bool IsTouched( void );

	void CalcDispSurfCoords( bool bLightMap, int lightmapID );
	void GetPositionOnSurface( float u, float v, Vector &vPos, Vector *pNormal, float *pAlpha );

	void DispUVToSurf( Vector2D const &dispUV, Vector &vecPoint, Vector *pNormal, float *pAlpha );
	void BaseFacePlaneToDispUV( Vector const &planePt, Vector2D &dispUV );
	bool SurfToBaseFacePlane( Vector const &surfPt, Vector &planePt );

	const CDispCornerNeighbors*	GetCornerNeighbors( int iCorner ) const	{ return GetSurface()->GetCornerNeighbors( iCorner ); }
	const CDispNeighbor*	GetEdgeNeighbor( int iEdge ) const	{ return GetSurface()->GetEdgeNeighbor( iEdge ); }

	void SetListIndex( int nIndex )		{ m_nListIndex = nIndex; } 
	int GetListIndex( void )			{ return m_nListIndex; }

	CBitVec<MAX_DISPVERTS>&			GetAllowedVerts()		{ return m_AllowedVerts; }
	const CBitVec<MAX_DISPVERTS>&	GetAllowedVerts() const	{ return m_AllowedVerts; }
	void AllowedVerts_Clear( void )							{ m_AllowedVerts.SetAll(); }
	int	 AllowedVerts_GetNumDWords() const					{ return m_AllowedVerts.GetNumDWords(); }
	unsigned long AllowedVerts_GetDWord(int i) const        { return m_AllowedVerts.GetDWord( i ); }
	void AllowedVerts_SetDWord(int i, unsigned long val)    { m_AllowedVerts.SetDWord( i, val ); }


	void Position_Update( int iVert, Vector vecPos );

	//=========================================================================
	//
	// friend functions
	//
	friend void SmoothNeighboringDispSurfNormals( CCoreDispInfo **ppCoreDispInfoList, int listSize );

private:
																				// be changed to match the paint normal next pass)
	// LOD/collision node data
	CCoreDispNode		*m_Nodes;		// LOD quad-tree nodes

	float				m_Elevation;	// distance along the subdivision normal (should

	// defines the size of the displacement surface
	int					m_Power;		// "size" of the displacement map

	// base surface data
	CCoreDispSurface	m_Surf;			// surface containing displacement data
	                                    // be changed to match the paint normal next pass)
	// Vertex data..
	CoreDispVert_t		*m_pVerts;

	// Triangle data..
	CoreDispTri_t		*m_pTris;

	// render specific data
	int					m_RenderIndexCount;		// number of indices used in rendering
	unsigned short		*m_RenderIndices;		// rendering index list (list of triangles)
	int					m_RenderCounter;		// counter to verify surfaces are renderered/collided with only once per frame

	// utility data
	bool				m_bTouched;				// touched flag
	CCoreDispInfo		*m_pNext;				// used for chaining

	// The list that this disp is in (used for CDispUtils::IHelper implementation).
	CCoreDispInfo		**m_ppListBase;
	int					m_ListSize;

	CBitVec<MAX_DISPVERTS>	m_AllowedVerts;		// Built in VBSP. Defines which verts are allowed to exist based on what the neighbors are.

	int					m_nListIndex;

	//=========================================================================
	//
	// Creation Functions
	//

	void GenerateDispSurf( void );
	void GenerateDispSurfNormals( void );
	void GenerateDispSurfTangentSpaces( void );
	bool DoesEdgeExist( int indexRow, int indexCol, int direction, int postSpacing );
	void CalcNormalFromEdges( int indexRow, int indexCol, bool bIsEdge[4], Vector& normal );
	void CalcDispSurfAlphas( void );
	void GenerateLODTree( void );
	void CalcVertIndicesAtNodes( int nodeIndex );
	int GetNodeVertIndexFromParentIndex( int level, int parentVertIndex, int direction );
	void CalcNodeInfo( int nodeIndex, int terminationLevel );
	void CalcNeighborVertIndicesAtNode( int nodeIndex, int level );
	void CalcNeighborNodeIndicesAtNode( int nodeIndex, int level );
	void CalcErrorTermAtNode( int nodeIndex, int level );
	float GetMaxErrorFromChildren( int nodeIndex, int level );
	void CalcBoundingBoxAtNode( int nodeIndex );
	void CalcMinMaxBoundingBoxAtNode( int nodeIndex, Vector& bMin, Vector& bMax );
	void CalcTriSurfInfoAtNode( int nodeIndex );
	void CalcTriSurfIndices( int nodeIndex, int indices[8][3] );
	void CalcTriSurfBoundingBoxes( int nodeIndex, int indices[8][3] );
	void CalcRayBoundingBoxes( int nodeIndex, int indices[8][3] );
	void CalcTriSurfPlanes( int nodeIndex, int indices[8][3] );
	void GenerateCollisionData( void );
	void GenerateCollisionSurface( void );

	void CreateBoundingBoxes( CoreDispBBox_t *pBBox, int count );

	void DispUVToSurf_TriTLToBR( Vector &vecPoint, Vector *pNormal, float *pAlpha, float flU, float flV, const Vector &vecIntersectPoint );
	void DispUVToSurf_TriBLToTR( Vector &vecPoint, Vector *pNormal, float *pAlpha, float flU, float flV, const Vector &vecIntersectPoint );
	void DispUVToSurf_TriTLToBR_1( const Vector &vecIntersectPoint, int nSnapU, int nNextU, int nSnapV, int nNextV, Vector &vecPoint, Vector *pNormal, float *pAlpha, bool bBackup );
	void DispUVToSurf_TriTLToBR_2( const Vector &vecIntersectPoint, int nSnapU, int nNextU, int nSnapV, int nNextV, Vector &vecPoint, Vector *pNormal, float *pAlpha, bool bBackup );
	void DispUVToSurf_TriBLToTR_1( const Vector &vecIntersectPoint, int nSnapU, int nNextU, int nSnapV, int nNextV, Vector &vecPoint, Vector *pNormal, float *pAlpha, bool bBackup );
	void DispUVToSurf_TriBLToTR_2( const Vector &vecIntersectPoint, int nSnapU, int nNextU, int nSnapV, int nNextV, Vector &vecPoint, Vector *pNormal, float *pAlpha, bool bBackup );

	void GetTriangleIndicesForDispBBox( int nIndex, int nTris[2][3] );

	void BuildTriTLtoBR( int ndx );
	void BuildTriBLtoTR( int ndx );

	void InitTris( void );
	void CreateTris( void );
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetPower( int power )
{
	m_Power = power;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetPower( void ) const
{
	return m_Power;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetPostSpacing( void )
{
	return ( ( 1 << m_Power ) + 1 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetWidth( void )
{
	return ( ( 1 << m_Power ) + 1 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetHeight( void )
{
	return ( ( 1 << m_Power ) + 1 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetSize( void ) const
{
    return ( ( ( 1 << m_Power ) + 1 ) * ( ( 1 << m_Power ) + 1 ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetVert( int index, Vector const &vert )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( vert, m_pVerts[index].m_Vert );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetVert( int index, Vector& vert ) const
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( m_pVerts[index].m_Vert, vert );
}


inline const Vector& CCoreDispInfo::GetVert( int index ) const
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	return m_pVerts[index].m_Vert;
}

inline const Vector& CCoreDispInfo::GetVert( const CVertIndex &index ) const
{
	return GetVert( VertIndexToInt( index ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetFlatVert( int index, Vector& vert ) const
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( m_pVerts[index].m_FlatVert, vert );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetFlatVert( int index, const Vector &vert )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( vert, m_pVerts[index].m_FlatVert );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetNormal( int index, Vector const &normal )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( normal, m_pVerts[index].m_Normal );
}

	
inline void CCoreDispInfo::SetNormal( const CVertIndex &index, Vector const &normal )
{
	SetNormal( VertIndexToInt( index ), normal );
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetNormal( int index, Vector& normal ) const
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( m_pVerts[index].m_Normal, normal );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline const Vector& CCoreDispInfo::GetNormal( int index ) const
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	return m_pVerts[index].m_Normal;
}


inline const Vector& CCoreDispInfo::GetNormal( const CVertIndex &index ) const
{
	return GetNormal( VertIndexToInt( index ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetTangentS( int index, Vector& tangentS ) const
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	VectorCopy( m_pVerts[index].m_TangentS, tangentS );
}

inline const Vector &CCoreDispInfo::GetTangentS( int index ) const
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	return m_pVerts[index].m_TangentS;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetTangentT( int index, Vector& tangentT ) const
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	VectorCopy( m_pVerts[index].m_TangentT, tangentT );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetTexCoord( int index, Vector2D const& texCoord )
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	Vector2DCopy( texCoord, m_pVerts[index].m_TexCoord );
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetTexCoord( int index, Vector2D& texCoord ) const
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	Vector2DCopy( m_pVerts[index].m_TexCoord, texCoord );
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetLuxelCoord( int bumpIndex, int index, Vector2D const& luxelCoord )
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	Vector2DCopy( luxelCoord, m_pVerts[index].m_LuxelCoords[bumpIndex] );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetLuxelCoord( int bumpIndex, int index, Vector2D& luxelCoord ) const 
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	Assert( bumpIndex >= 0 );
	Assert( bumpIndex < NUM_BUMP_VECTS + 1 );
	Vector2DCopy( m_pVerts[index].m_LuxelCoords[bumpIndex], luxelCoord );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetAlpha( int index, float alpha )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	m_pVerts[index].m_Alpha = alpha;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CCoreDispInfo::GetAlpha( int index )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	return m_pVerts[index].m_Alpha;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetElevation( float elevation )
{
	m_Elevation = elevation;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CCoreDispInfo::GetElevation( void )
{
	return m_Elevation;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::ResetFieldVectors( void )
{
//	Vector normal;
//	m_Surf.GetNormal( normal );

	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		m_pVerts[i].m_FieldVector.Init();
//		m_FieldVectors[i] = normal;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetFieldVector( int index, Vector const &v )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( v, m_pVerts[index].m_FieldVector ); 
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetFieldVector( int index, Vector& v )
{
	Assert( index >= 0 );
	Assert( index < MAX_VERT_COUNT );
	VectorCopy( m_pVerts[index].m_FieldVector, v ); 
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::ResetSubdivPositions( void )
{
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		m_pVerts[i].m_SubdivPos.Init();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetSubdivPosition( int ndx, Vector const &v )
{
	Assert( ndx >= 0 );
	Assert( ndx < MAX_VERT_COUNT );
	m_pVerts[ndx].m_SubdivPos = v;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetSubdivPosition( int ndx, Vector& v )
{
	Assert( ndx >= 0 );
	Assert( ndx < MAX_VERT_COUNT );
	v = m_pVerts[ndx].m_SubdivPos;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::ResetSubdivNormals( void )
{
	Vector normal;
	m_Surf.GetNormal( normal );

	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		m_pVerts[i].m_SubdivNormal = normal;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetSubdivNormal( int ndx, Vector const &v )
{
	Assert( ndx >= 0 );
	Assert( ndx < MAX_VERT_COUNT );
	m_pVerts[ndx].m_SubdivNormal = v;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::GetSubdivNormal( int ndx, Vector &v )
{
	Assert( ndx >= 0 );
	Assert( ndx < MAX_VERT_COUNT );
	v = m_pVerts[ndx].m_SubdivNormal;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::ResetFieldDistances( void )
{
	int size = GetSize();
	for( int i = 0; i < size; i++ )
	{
		m_pVerts[i].m_FieldDistance = 0.0f;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetFieldDistance( int index, float dist )
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	m_pVerts[index].m_FieldDistance = dist;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CCoreDispInfo::GetFieldDistance( int index )
{
	Assert( index >= 0 );
	Assert( index < GetSize() );
	return m_pVerts[index].m_FieldDistance;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetRenderIndexCount( int count )
{
	m_RenderIndexCount = count;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetRenderIndexCount( void )
{
	return m_RenderIndexCount;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetRenderIndex( int index, int triIndex )
{
	Assert( index >= 0 );
	Assert( index < ( MAX_VERT_COUNT*2*3) );
	m_RenderIndices[index] = triIndex;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CCoreDispInfo::GetRenderIndex( int index )
{
	Assert( index >= 0 );
	Assert( index < ( MAX_VERT_COUNT*2*3) );
	return m_RenderIndices[index];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CoreDispVert_t *CCoreDispInfo::GetDispVertList()
{
	return m_pVerts;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline unsigned short *CCoreDispInfo::GetRenderIndexList( void )
{
	return &m_RenderIndices[0];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CCoreDispInfo::SetTouched( bool touched )
{
	m_bTouched = touched;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CCoreDispInfo::IsTouched( void )
{
	return m_bTouched;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CCoreDispNode *CCoreDispInfo::GetNode( int index )
{
	Assert( index >= 0 );
	Assert( index < MAX_NODE_COUNT );
	return &m_Nodes[index];
}

bool CalcBarycentricCooefs( Vector const &v0, Vector const &v1, Vector const &v2,
						    Vector const &pt, float &c0, float &c1, float &c2 );

#endif // BUILDDISP_H
