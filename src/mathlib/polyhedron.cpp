//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "mathlib/polyhedron.h"
#include "mathlib/vmatrix.h"
#include <stdlib.h>
#include <stdio.h>
#include "tier1/utlvector.h"



struct GeneratePolyhedronFromPlanes_Point;
struct GeneratePolyhedronFromPlanes_PointLL;
struct GeneratePolyhedronFromPlanes_Line;
struct GeneratePolyhedronFromPlanes_LineLL;
struct GeneratePolyhedronFromPlanes_Polygon;
struct GeneratePolyhedronFromPlanes_PolygonLL;

struct GeneratePolyhedronFromPlanes_UnorderedPointLL;
struct GeneratePolyhedronFromPlanes_UnorderedLineLL;
struct GeneratePolyhedronFromPlanes_UnorderedPolygonLL;

Vector FindPointInPlanes( const float *pPlanes, int planeCount );
bool FindConvexShapeLooseAABB( const float *pInwardFacingPlanes, int iPlaneCount, Vector *pAABBMins, Vector *pAABBMaxs );
CPolyhedron *ClipLinkedGeometry( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory );
CPolyhedron *ConvertLinkedGeometryToPolyhedron( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, bool bUseTemporaryMemory );

//#define ENABLE_DEBUG_POLYHEDRON_DUMPS //Dumps debug information to disk for use with glview. Requires that tier2 also be in all projects using debug mathlib
//#define DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS //dumps successfully generated polyhedrons

#ifdef _DEBUG
void DumpPolyhedronToGLView( const CPolyhedron *pPolyhedron, const char *pFilename, const VMatrix *pTransform );
void DumpPlaneToGlView( const float *pPlane, float fGrayScale, const char *pszFileName, const VMatrix *pTransform );
void DumpLineToGLView( const Vector &vPoint1, const Vector &vColor1, const Vector &vPoint2, const Vector &vColor2, float fThickness, FILE *pFile );
void DumpAABBToGLView( const Vector &vCenter, const Vector &vExtents, const Vector &vColor, FILE *pFile );

#if defined( ENABLE_DEBUG_POLYHEDRON_DUMPS ) && defined( WIN32 )
#include "winlite.h"
#endif

static VMatrix s_matIdentity( 1.0f, 0.0f, 0.0f, 0.0f, 
							 0.0f, 1.0f, 0.0f, 0.0f, 
							 0.0f, 0.0f, 1.0f, 0.0f, 
							 0.0f, 0.0f, 0.0f, 1.0f );
#endif

#if defined( DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS )
static int g_iPolyhedronDumpCounter = 0;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( _DEBUG ) && defined( ENABLE_DEBUG_POLYHEDRON_DUMPS )
void CreateDumpDirectory( const char *szDirectoryName )
{
#if defined( WIN32 )
	CreateDirectory( szDirectoryName, NULL );
#else
	Assert( false ); //TODO: create directories in linux
#endif
}
#endif



void CPolyhedron_AllocByNew::Release( void )
{
	delete this;
}

CPolyhedron_AllocByNew *CPolyhedron_AllocByNew::Allocate( unsigned short iVertices, unsigned short iLines, unsigned short iIndices, unsigned short iPolygons ) //creates the polyhedron along with enough memory to hold all it's data in a single allocation
{
	void *pMemory = new unsigned char [ sizeof( CPolyhedron_AllocByNew ) +
										(iVertices * sizeof(Vector)) + 
										(iLines * sizeof(Polyhedron_IndexedLine_t)) + 
										(iIndices * sizeof( Polyhedron_IndexedLineReference_t )) + 
										(iPolygons * sizeof( Polyhedron_IndexedPolygon_t ))];

#include "tier0/memdbgoff.h" //the following placement new doesn't compile with memory debugging
	CPolyhedron_AllocByNew *pAllocated = new ( pMemory ) CPolyhedron_AllocByNew;
#include "tier0/memdbgon.h"

	pAllocated->iVertexCount = iVertices;
	pAllocated->iLineCount = iLines;
	pAllocated->iIndexCount = iIndices;
	pAllocated->iPolygonCount = iPolygons;
	pAllocated->pVertices = (Vector *)(pAllocated + 1); //start vertex memory at the end of the class
	pAllocated->pLines = (Polyhedron_IndexedLine_t *)(pAllocated->pVertices + iVertices);
	pAllocated->pIndices = (Polyhedron_IndexedLineReference_t *)(pAllocated->pLines + iLines);
	pAllocated->pPolygons = (Polyhedron_IndexedPolygon_t *)(pAllocated->pIndices + iIndices);

	return pAllocated;
}


class CPolyhedron_TempMemory : public CPolyhedron
{
public:
#ifdef DBGFLAG_ASSERT
	int iReferenceCount;
#endif

	virtual void Release( void )
	{
#ifdef DBGFLAG_ASSERT
		--iReferenceCount;
#endif
	}

	CPolyhedron_TempMemory( void )
#ifdef DBGFLAG_ASSERT
		: iReferenceCount( 0 )
#endif
	{ };
};


static CUtlVector<unsigned char> s_TempMemoryPolyhedron_Buffer;
static CPolyhedron_TempMemory s_TempMemoryPolyhedron;

CPolyhedron *GetTempPolyhedron( unsigned short iVertices, unsigned short iLines, unsigned short iIndices, unsigned short iPolygons ) //grab the temporary polyhedron. Avoids new/delete for quick work. Can only be in use by one chunk of code at a time
{
	AssertMsg( s_TempMemoryPolyhedron.iReferenceCount == 0, "Temporary polyhedron memory being rewritten before released" );
#ifdef DBGFLAG_ASSERT
	++s_TempMemoryPolyhedron.iReferenceCount;
#endif
	s_TempMemoryPolyhedron_Buffer.SetCount( (sizeof( Vector ) * iVertices) +
											(sizeof( Polyhedron_IndexedLine_t ) * iLines) +
											(sizeof( Polyhedron_IndexedLineReference_t ) * iIndices) +
											(sizeof( Polyhedron_IndexedPolygon_t ) * iPolygons) );

	s_TempMemoryPolyhedron.iVertexCount = iVertices;
	s_TempMemoryPolyhedron.iLineCount = iLines;
	s_TempMemoryPolyhedron.iIndexCount = iIndices;
	s_TempMemoryPolyhedron.iPolygonCount = iPolygons;

	s_TempMemoryPolyhedron.pVertices = (Vector *)s_TempMemoryPolyhedron_Buffer.Base();
	s_TempMemoryPolyhedron.pLines = (Polyhedron_IndexedLine_t *)(&s_TempMemoryPolyhedron.pVertices[s_TempMemoryPolyhedron.iVertexCount]);
	s_TempMemoryPolyhedron.pIndices = (Polyhedron_IndexedLineReference_t *)(&s_TempMemoryPolyhedron.pLines[s_TempMemoryPolyhedron.iLineCount]);
	s_TempMemoryPolyhedron.pPolygons = (Polyhedron_IndexedPolygon_t *)(&s_TempMemoryPolyhedron.pIndices[s_TempMemoryPolyhedron.iIndexCount]);

	return &s_TempMemoryPolyhedron;
}


Vector CPolyhedron::Center( void )
{
	if( iVertexCount == 0 )
		return vec3_origin;

	Vector vAABBMin, vAABBMax;
	vAABBMin = vAABBMax = pVertices[0];
	for( int i = 1; i != iVertexCount; ++i )
	{
		Vector &vPoint = pVertices[i];
		if( vPoint.x < vAABBMin.x )
			vAABBMin.x = vPoint.x;
		if( vPoint.y < vAABBMin.y )
			vAABBMin.y = vPoint.y;
		if( vPoint.z < vAABBMin.z )
			vAABBMin.z = vPoint.z;

		if( vPoint.x > vAABBMax.x )
			vAABBMax.x = vPoint.x;
		if( vPoint.y > vAABBMax.y )
			vAABBMax.y = vPoint.y;
		if( vPoint.z > vAABBMax.z )
			vAABBMax.z = vPoint.z;
	}
	return ((vAABBMin + vAABBMax) * 0.5f);
}

enum PolyhedronPointPlanarity
{
	POINT_DEAD,
	POINT_ONPLANE,
	POINT_ALIVE	
};

struct GeneratePolyhedronFromPlanes_Point
{
	Vector ptPosition;
	GeneratePolyhedronFromPlanes_LineLL *pConnectedLines; //keep these in a clockwise order, circular linking
	float fPlaneDist; //used in plane cutting
	PolyhedronPointPlanarity planarity;
	int iSaveIndices;
};

struct GeneratePolyhedronFromPlanes_Line
{
	GeneratePolyhedronFromPlanes_Point *pPoints[2]; //the 2 connecting points in no particular order
	GeneratePolyhedronFromPlanes_Polygon *pPolygons[2]; //viewing from the outside with the point connections going up, 0 is the left polygon, 1 is the right
	int iSaveIndices;
	bool bAlive; //connected to at least one living point
	bool bCut; //connected to at least one dead point

	GeneratePolyhedronFromPlanes_LineLL *pPointLineLinks[2]; //rather than going into a point and searching for its link to this line, lets just cache it to eliminate searching
	GeneratePolyhedronFromPlanes_LineLL *pPolygonLineLinks[2]; //rather than going into a polygon and searching for its link to this line, lets just cache it to eliminate searching
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
	int iDebugFlags;
#endif
};

struct GeneratePolyhedronFromPlanes_LineLL
{
	GeneratePolyhedronFromPlanes_Line *pLine;
	int iReferenceIndex; //whatever is referencing the line should know which side of the line it's on (points and polygons), for polygons, it's which point to follow to continue going clockwise, which makes polygon 0 the one on the left side of an upward facing line vector, for points, it's the OTHER point's index
	GeneratePolyhedronFromPlanes_LineLL *pPrev;
	GeneratePolyhedronFromPlanes_LineLL *pNext;
};

struct GeneratePolyhedronFromPlanes_Polygon
{
	Vector vSurfaceNormal; 
	GeneratePolyhedronFromPlanes_LineLL *pLines; //keep these in a clockwise order, circular linking
	
	bool bMissingASide;
};

struct GeneratePolyhedronFromPlanes_UnorderedPolygonLL //an unordered collection of polygons
{
	GeneratePolyhedronFromPlanes_Polygon *pPolygon;
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPrev;
};

struct GeneratePolyhedronFromPlanes_UnorderedLineLL //an unordered collection of lines
{
	GeneratePolyhedronFromPlanes_Line *pLine;
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pPrev;
};

struct GeneratePolyhedronFromPlanes_UnorderedPointLL //an unordered collection of points
{
	GeneratePolyhedronFromPlanes_Point *pPoint;
	GeneratePolyhedronFromPlanes_UnorderedPointLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedPointLL *pPrev;
};




CPolyhedron *ClipPolyhedron( const CPolyhedron *pExistingPolyhedron, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	if( pExistingPolyhedron == NULL )
		return NULL;

	AssertMsg( (pExistingPolyhedron->iVertexCount >= 3) && (pExistingPolyhedron->iPolygonCount >= 2), "Polyhedron doesn't meet absolute minimum spec" );

	float *pUsefulPlanes = (float *)stackalloc( sizeof( float ) * 4 * iPlaneCount );
	int iUsefulPlaneCount = 0;
	Vector *pExistingVertices = pExistingPolyhedron->pVertices;

	//A large part of clipping will either eliminate the polyhedron entirely, or clip nothing at all, so lets just check for those first and throw away useless planes
	{
		int iLiveCount = 0;
		int iDeadCount = 0;
		const float fNegativeOnPlaneEpsilon = -fOnPlaneEpsilon;

		for( int i = 0; i != iPlaneCount; ++i )
		{
			Vector vNormal = *((Vector *)&pOutwardFacingPlanes[(i * 4) + 0]);
			float fPlaneDist = pOutwardFacingPlanes[(i * 4) + 3];

			for( int j = 0; j != pExistingPolyhedron->iVertexCount; ++j )
			{
				float fPointDist = vNormal.Dot( pExistingVertices[j] ) - fPlaneDist;
				
				if( fPointDist <= fNegativeOnPlaneEpsilon )
					++iLiveCount;
				else if( fPointDist > fOnPlaneEpsilon )
					++iDeadCount;
			}

			if( iLiveCount == 0 )
			{
				//all points are dead or on the plane, so the polyhedron is dead
				return NULL;
			}

			if( iDeadCount != 0 )
			{
				//at least one point died, this plane yields useful results
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 0] = vNormal.x;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 1] = vNormal.y;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 2] = vNormal.z;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 3] = fPlaneDist;
				++iUsefulPlaneCount;
			}
		}
	}

	if( iUsefulPlaneCount == 0 )
	{
		//testing shows that the polyhedron won't even be cut, clone the existing polyhedron and return that

		CPolyhedron *pReturn;
		if( bUseTemporaryMemory )
		{
			pReturn = GetTempPolyhedron( pExistingPolyhedron->iVertexCount, 
											pExistingPolyhedron->iLineCount, 
											pExistingPolyhedron->iIndexCount, 
											pExistingPolyhedron->iPolygonCount );
		}
		else
		{
			pReturn = CPolyhedron_AllocByNew::Allocate( pExistingPolyhedron->iVertexCount, 
														pExistingPolyhedron->iLineCount, 
														pExistingPolyhedron->iIndexCount, 
														pExistingPolyhedron->iPolygonCount );
		}

		memcpy( pReturn->pVertices, pExistingPolyhedron->pVertices, sizeof( Vector ) * pReturn->iVertexCount );
		memcpy( pReturn->pLines, pExistingPolyhedron->pLines, sizeof( Polyhedron_IndexedLine_t ) * pReturn->iLineCount );
		memcpy( pReturn->pIndices, pExistingPolyhedron->pIndices, sizeof( Polyhedron_IndexedLineReference_t ) * pReturn->iIndexCount );
		memcpy( pReturn->pPolygons, pExistingPolyhedron->pPolygons, sizeof( Polyhedron_IndexedPolygon_t ) * pReturn->iPolygonCount );

		return pReturn;
	}



	//convert the polyhedron to linked geometry
	GeneratePolyhedronFromPlanes_Point *pStartPoints = (GeneratePolyhedronFromPlanes_Point *)stackalloc( pExistingPolyhedron->iVertexCount * sizeof( GeneratePolyhedronFromPlanes_Point ) );
	GeneratePolyhedronFromPlanes_Line *pStartLines = (GeneratePolyhedronFromPlanes_Line *)stackalloc( pExistingPolyhedron->iLineCount * sizeof( GeneratePolyhedronFromPlanes_Line ) );
	GeneratePolyhedronFromPlanes_Polygon *pStartPolygons = (GeneratePolyhedronFromPlanes_Polygon *)stackalloc( pExistingPolyhedron->iPolygonCount * sizeof( GeneratePolyhedronFromPlanes_Polygon ) );

	GeneratePolyhedronFromPlanes_LineLL *pStartLineLinks = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( pExistingPolyhedron->iLineCount * 4 * sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
	
	int iCurrentLineLinkIndex = 0;

	//setup points
	for( int i = 0; i != pExistingPolyhedron->iVertexCount; ++i )
	{
		pStartPoints[i].ptPosition = pExistingPolyhedron->pVertices[i];
		pStartPoints[i].pConnectedLines = NULL; //we won't be circular linking until later
	}

	//setup lines and interlink to points (line links are not yet circularly linked, and are unordered)
	for( int i = 0; i != pExistingPolyhedron->iLineCount; ++i )
	{
		for( int j = 0; j != 2; ++j )
		{
			pStartLines[i].pPoints[j] = &pStartPoints[pExistingPolyhedron->pLines[i].iPointIndices[j]];

			GeneratePolyhedronFromPlanes_LineLL *pLineLink = &pStartLineLinks[iCurrentLineLinkIndex++];
			pStartLines[i].pPointLineLinks[j] = pLineLink;
			pLineLink->pLine = &pStartLines[i];
			pLineLink->iReferenceIndex = 1 - j;
			//pLineLink->pPrev = NULL;
			pLineLink->pNext = pStartLines[i].pPoints[j]->pConnectedLines;
			pStartLines[i].pPoints[j]->pConnectedLines = pLineLink;
		}
	}



	//setup polygons
	for( int i = 0; i != pExistingPolyhedron->iPolygonCount; ++i )
	{
		pStartPolygons[i].vSurfaceNormal = pExistingPolyhedron->pPolygons[i].polyNormal;
		Polyhedron_IndexedLineReference_t *pOffsetPolyhedronLines = &pExistingPolyhedron->pIndices[pExistingPolyhedron->pPolygons[i].iFirstIndex];

		
		GeneratePolyhedronFromPlanes_LineLL *pFirstLink = &pStartLineLinks[iCurrentLineLinkIndex];
		pStartPolygons[i].pLines = pFirstLink; //technically going to link to itself on first pass, then get linked properly immediately afterward
		for( int j = 0; j != pExistingPolyhedron->pPolygons[i].iIndexCount; ++j )
		{
			GeneratePolyhedronFromPlanes_LineLL *pLineLink = &pStartLineLinks[iCurrentLineLinkIndex++];
			pLineLink->pLine = &pStartLines[pOffsetPolyhedronLines[j].iLineIndex];
			pLineLink->iReferenceIndex = pOffsetPolyhedronLines[j].iEndPointIndex;
			
			pLineLink->pLine->pPolygons[pLineLink->iReferenceIndex] = &pStartPolygons[i];
			pLineLink->pLine->pPolygonLineLinks[pLineLink->iReferenceIndex] = pLineLink;			

			pLineLink->pPrev = pStartPolygons[i].pLines;
			pStartPolygons[i].pLines->pNext = pLineLink;
			pStartPolygons[i].pLines = pLineLink;
		}
		
		pFirstLink->pPrev = pStartPolygons[i].pLines;
		pStartPolygons[i].pLines->pNext = pFirstLink;
	}

	Assert( iCurrentLineLinkIndex == (pExistingPolyhedron->iLineCount * 4) );

	//go back to point line links so we can circularly link them as well as order them now that every point has all its line links
	for( int i = 0; i != pExistingPolyhedron->iVertexCount; ++i )
	{
		//interlink the points
		{
			GeneratePolyhedronFromPlanes_LineLL *pLastVisitedLink = pStartPoints[i].pConnectedLines;
			GeneratePolyhedronFromPlanes_LineLL *pCurrentLink = pLastVisitedLink;
			
			do
			{
				pCurrentLink->pPrev = pLastVisitedLink;
				pLastVisitedLink = pCurrentLink;
				pCurrentLink = pCurrentLink->pNext;
			} while( pCurrentLink );

			//circular link
			pLastVisitedLink->pNext = pStartPoints[i].pConnectedLines;
			pStartPoints[i].pConnectedLines->pPrev = pLastVisitedLink;
		}


		//fix ordering
		GeneratePolyhedronFromPlanes_LineLL *pFirstLink = pStartPoints[i].pConnectedLines;
		GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pFirstLink;
		GeneratePolyhedronFromPlanes_LineLL *pSearchLink;
		GeneratePolyhedronFromPlanes_Polygon *pLookingForPolygon;
		Assert( pFirstLink->pNext != pFirstLink );
		do
		{
			pLookingForPolygon = pWorkLink->pLine->pPolygons[1 - pWorkLink->iReferenceIndex]; //grab pointer to left polygon
			pSearchLink = pWorkLink->pPrev;

			while( pSearchLink->pLine->pPolygons[pSearchLink->iReferenceIndex] != pLookingForPolygon )
				pSearchLink = pSearchLink->pPrev;

			Assert( pSearchLink->pLine->pPolygons[pSearchLink->iReferenceIndex] == pWorkLink->pLine->pPolygons[1 - pWorkLink->iReferenceIndex] );

			//pluck the search link from wherever it is
			pSearchLink->pPrev->pNext = pSearchLink->pNext;
			pSearchLink->pNext->pPrev = pSearchLink->pPrev;

			//insert the search link just before the work link			
			pSearchLink->pPrev = pWorkLink->pPrev;
			pSearchLink->pNext = pWorkLink;
			
			pSearchLink->pPrev->pNext = pSearchLink;
			pWorkLink->pPrev = pSearchLink;

			pWorkLink = pSearchLink;
		} while( pWorkLink != pFirstLink );
	}

	GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints = (GeneratePolyhedronFromPlanes_UnorderedPointLL *)stackalloc( pExistingPolyhedron->iVertexCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedPointLL ) );
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines = (GeneratePolyhedronFromPlanes_UnorderedLineLL *)stackalloc( pExistingPolyhedron->iLineCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedLineLL ) );
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons = (GeneratePolyhedronFromPlanes_UnorderedPolygonLL *)stackalloc( pExistingPolyhedron->iPolygonCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedPolygonLL ) );

	//setup point collection
	{
		pPoints[0].pPrev = NULL;
		pPoints[0].pPoint = &pStartPoints[0];
		pPoints[0].pNext = &pPoints[1];
		int iLastPoint = pExistingPolyhedron->iVertexCount - 1;
		for( int i = 1; i != iLastPoint; ++i )
		{
			pPoints[i].pPrev = &pPoints[i - 1];
			pPoints[i].pPoint = &pStartPoints[i];
			pPoints[i].pNext = &pPoints[i + 1];
		}
		pPoints[iLastPoint].pPrev = &pPoints[iLastPoint - 1];
		pPoints[iLastPoint].pPoint = &pStartPoints[iLastPoint];
		pPoints[iLastPoint].pNext = NULL;
	}

	//setup line collection
	{
		pLines[0].pPrev = NULL;
		pLines[0].pLine = &pStartLines[0];
		pLines[0].pNext = &pLines[1];
		int iLastLine = pExistingPolyhedron->iLineCount - 1;
		for( int i = 1; i != iLastLine; ++i )
		{
			pLines[i].pPrev = &pLines[i - 1];
			pLines[i].pLine = &pStartLines[i];
			pLines[i].pNext = &pLines[i + 1];
		}
		pLines[iLastLine].pPrev = &pLines[iLastLine - 1];
		pLines[iLastLine].pLine = &pStartLines[iLastLine];
		pLines[iLastLine].pNext = NULL;
	}

	//setup polygon collection
	{
		pPolygons[0].pPrev = NULL;
		pPolygons[0].pPolygon = &pStartPolygons[0];
		pPolygons[0].pNext = &pPolygons[1];
		int iLastPolygon = pExistingPolyhedron->iPolygonCount - 1;
		for( int i = 1; i != iLastPolygon; ++i )
		{
			pPolygons[i].pPrev = &pPolygons[i - 1];
			pPolygons[i].pPolygon = &pStartPolygons[i];
			pPolygons[i].pNext = &pPolygons[i + 1];
		}
		pPolygons[iLastPolygon].pPrev = &pPolygons[iLastPolygon - 1];
		pPolygons[iLastPolygon].pPolygon = &pStartPolygons[iLastPolygon];
		pPolygons[iLastPolygon].pNext = NULL;
	}

	return ClipLinkedGeometry( pPolygons, pLines, pPoints, pUsefulPlanes, iUsefulPlaneCount, fOnPlaneEpsilon, bUseTemporaryMemory );
}



Vector FindPointInPlanes( const float *pPlanes, int planeCount )
{
	Vector point = vec3_origin;

	for ( int i = 0; i < planeCount; i++ )
	{
		float fD = DotProduct( *(Vector *)&pPlanes[i*4], point ) - pPlanes[i*4 + 3];
		if ( fD < 0 )
		{
			point -= fD * (*(Vector *)&pPlanes[i*4]);
		}
	}
	return point;
}



bool FindConvexShapeLooseAABB( const float *pInwardFacingPlanes, int iPlaneCount, Vector *pAABBMins, Vector *pAABBMaxs ) //bounding box of the convex shape (subject to floating point error)
{
	//returns false if the AABB hasn't been set
	if( pAABBMins == NULL && pAABBMaxs == NULL ) //no use in actually finding out what it is
		return false;

	struct FindConvexShapeAABB_Polygon_t
	{
		float *verts;
		int iVertCount;
	};

	float *pMovedPlanes = (float *)stackalloc( iPlaneCount * 4 * sizeof( float ) );
	//Vector vPointInPlanes = FindPointInPlanes( pInwardFacingPlanes, iPlaneCount );

	for( int i = 0; i != iPlaneCount; ++i )
	{
		pMovedPlanes[(i * 4) + 0] = pInwardFacingPlanes[(i * 4) + 0];
		pMovedPlanes[(i * 4) + 1] = pInwardFacingPlanes[(i * 4) + 1];
		pMovedPlanes[(i * 4) + 2] = pInwardFacingPlanes[(i * 4) + 2];
		pMovedPlanes[(i * 4) + 3] = pInwardFacingPlanes[(i * 4) + 3] - 100.0f; //move planes out a lot to kill some imprecision problems
	}
	
	

	//vAABBMins = vAABBMaxs = FindPointInPlanes( pPlanes, iPlaneCount );
	float *vertsIn = NULL; //we'll be allocating a new buffer for this with each new polygon, and moving it off to the polygon array
	float *vertsOut = (float *)stackalloc( (iPlaneCount + 4) * (sizeof( float ) * 3) ); //each plane will initially have 4 points in its polygon representation, and each plane clip has the possibility to add 1 point to the polygon
	float *vertsSwap;

	FindConvexShapeAABB_Polygon_t *pPolygons = (FindConvexShapeAABB_Polygon_t *)stackalloc( iPlaneCount * sizeof( FindConvexShapeAABB_Polygon_t ) );
	int iPolyCount = 0;

	for ( int i = 0; i < iPlaneCount; i++ )
	{
		Vector *pPlaneNormal = (Vector *)&pInwardFacingPlanes[i*4];
		float fPlaneDist = pInwardFacingPlanes[(i*4) + 3];

		if( vertsIn == NULL )
			vertsIn = (float *)stackalloc( (iPlaneCount + 4) * (sizeof( float ) * 3) );

		// Build a big-ass poly in this plane
		int vertCount = PolyFromPlane( (Vector *)vertsIn, *pPlaneNormal, fPlaneDist, 100000.0f );

		//chop it by every other plane
		for( int j = 0; j < iPlaneCount; j++ )
		{
			// don't clip planes with themselves
			if ( i == j )
				continue;

			// Chop the polygon against this plane
			vertCount = ClipPolyToPlane( (Vector *)vertsIn, vertCount, (Vector *)vertsOut, *(Vector *)&pMovedPlanes[j*4], pMovedPlanes[(j*4) + 3], 0.0f );

			//swap the input and output arrays
			vertsSwap = vertsIn; vertsIn = vertsOut; vertsOut = vertsSwap;

			// Less than a poly left, something's wrong, don't bother with this polygon
			if ( vertCount < 3 )
				break;
		}

		if ( vertCount < 3 )
			continue; //not enough to work with

		pPolygons[iPolyCount].iVertCount = vertCount;
		pPolygons[iPolyCount].verts = vertsIn;
		vertsIn = NULL;
		++iPolyCount;
	}

	if( iPolyCount == 0 )
		return false;

	//initialize the AABB to the first point available
	Vector vAABBMins, vAABBMaxs;
	vAABBMins = vAABBMaxs = ((Vector *)pPolygons[0].verts)[0];

	if( pAABBMins && pAABBMaxs ) //they want the full box
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x < vAABBMins.x ) 
					vAABBMins.x = PolyVerts[j].x;
				if( PolyVerts[j].y < vAABBMins.y ) 
					vAABBMins.y = PolyVerts[j].y;
				if( PolyVerts[j].z < vAABBMins.z ) 
					vAABBMins.z = PolyVerts[j].z;

				if( PolyVerts[j].x > vAABBMaxs.x ) 
					vAABBMaxs.x = PolyVerts[j].x;
				if( PolyVerts[j].y > vAABBMaxs.y ) 
					vAABBMaxs.y = PolyVerts[j].y;
				if( PolyVerts[j].z > vAABBMaxs.z ) 
					vAABBMaxs.z = PolyVerts[j].z;
			}
		}
		*pAABBMins = vAABBMins;
		*pAABBMaxs = vAABBMaxs;
	}
	else if( pAABBMins ) //they only want the min
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x < vAABBMins.x ) 
					vAABBMins.x = PolyVerts[j].x;
				if( PolyVerts[j].y < vAABBMins.y ) 
					vAABBMins.y = PolyVerts[j].y;
				if( PolyVerts[j].z < vAABBMins.z ) 
					vAABBMins.z = PolyVerts[j].z;
			}
		}
		*pAABBMins = vAABBMins;
	}
	else //they only want the max
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x > vAABBMaxs.x ) 
					vAABBMaxs.x = PolyVerts[j].x;
				if( PolyVerts[j].y > vAABBMaxs.y ) 
					vAABBMaxs.y = PolyVerts[j].y;
				if( PolyVerts[j].z > vAABBMaxs.z ) 
					vAABBMaxs.z = PolyVerts[j].z;
			}
		}
		*pAABBMaxs = vAABBMaxs;
	}

	return true;
}







CPolyhedron *ConvertLinkedGeometryToPolyhedron( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, bool bUseTemporaryMemory )
{
	Assert( (pPolygons != NULL) && (pLines != NULL) && (pPoints != NULL) );
	unsigned int iPolyCount = 0, iLineCount = 0, iPointCount = 0, iIndexCount = 0;

	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pPolygons;	
	do
	{
		++iPolyCount;
		GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pActivePolygonWalk->pPolygon->pLines;
		GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
		Assert( pLineWalk != NULL );
		
		do
		{
			++iIndexCount;
			pLineWalk = pLineWalk->pNext;
		} while( pLineWalk != pFirstLine );

		pActivePolygonWalk = pActivePolygonWalk->pNext;
	} while( pActivePolygonWalk );

	GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pLines;
	do
	{
		++iLineCount;
		pActiveLineWalk = pActiveLineWalk->pNext;
	} while( pActiveLineWalk );

	GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pPoints;
	do
	{
		++iPointCount;
		pActivePointWalk = pActivePointWalk->pNext;
	} while( pActivePointWalk );	
	
	CPolyhedron *pReturn;
	if( bUseTemporaryMemory )
	{
		pReturn = GetTempPolyhedron( iPointCount, iLineCount, iIndexCount, iPolyCount );
	}
	else
	{
		pReturn = CPolyhedron_AllocByNew::Allocate( iPointCount, iLineCount, iIndexCount, iPolyCount );
	}

	Vector *pVertexArray = pReturn->pVertices;
	Polyhedron_IndexedLine_t *pLineArray = pReturn->pLines;
	Polyhedron_IndexedLineReference_t *pIndexArray = pReturn->pIndices;
	Polyhedron_IndexedPolygon_t *pPolyArray = pReturn->pPolygons;

	//copy points
	pActivePointWalk = pPoints;
	for( unsigned int i = 0; i != iPointCount; ++i )
	{
		pVertexArray[i] = pActivePointWalk->pPoint->ptPosition;
		pActivePointWalk->pPoint->iSaveIndices = i; //storing array indices
		pActivePointWalk = pActivePointWalk->pNext;
	}

	//copy lines
	pActiveLineWalk = pLines;
	for( unsigned int i = 0; i != iLineCount; ++i )
	{
		pLineArray[i].iPointIndices[0] = (unsigned short)pActiveLineWalk->pLine->pPoints[0]->iSaveIndices;
		pLineArray[i].iPointIndices[1] = (unsigned short)pActiveLineWalk->pLine->pPoints[1]->iSaveIndices;

		pActiveLineWalk->pLine->iSaveIndices = i; //storing array indices

		pActiveLineWalk = pActiveLineWalk->pNext;
	}

	//copy polygons and indices at the same time
	pActivePolygonWalk = pPolygons;
	iIndexCount = 0;
	for( unsigned int i = 0; i != iPolyCount; ++i )
	{
		pPolyArray[i].polyNormal = pActivePolygonWalk->pPolygon->vSurfaceNormal;
		pPolyArray[i].iFirstIndex = iIndexCount;		
		
		GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pActivePolygonWalk->pPolygon->pLines;
		GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
		do
		{
			//pIndexArray[iIndexCount] = pLineWalk->pLine->pPoints[pLineWalk->iReferenceIndex]->iWorkData; //startpoint of each line, iWorkData is the index of the vertex
			pIndexArray[iIndexCount].iLineIndex = pLineWalk->pLine->iSaveIndices;
			pIndexArray[iIndexCount].iEndPointIndex = pLineWalk->iReferenceIndex;
			
			++iIndexCount;
			pLineWalk = pLineWalk->pNext;
		} while( pLineWalk != pFirstLine );

		pPolyArray[i].iIndexCount = iIndexCount - pPolyArray[i].iFirstIndex;

		pActivePolygonWalk = pActivePolygonWalk->pNext;	
	}

#if defined( _DEBUG ) && defined( ENABLE_DEBUG_POLYHEDRON_DUMPS ) && defined( DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS )
	char szCollisionFile[128];
	CreateDumpDirectory( "PolyhedronDumps" );
	Q_snprintf( szCollisionFile, 128, "PolyhedronDumps/NewStyle_PolyhedronDump%i.txt", g_iPolyhedronDumpCounter );
	++g_iPolyhedronDumpCounter;

	remove( szCollisionFile );
	DumpPolyhedronToGLView( pReturn, szCollisionFile, &s_matIdentity );
	DumpPolyhedronToGLView( pReturn, "PolyhedronDumps/NewStyle_PolyhedronDump_All-Appended.txt", &s_matIdentity );
#endif

	return pReturn;
}



#ifdef _DEBUG

void DumpPointListToGLView( GeneratePolyhedronFromPlanes_UnorderedPointLL *pHead, PolyhedronPointPlanarity planarity, const Vector &vColor, const char *szDumpFile, const VMatrix *pTransform )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	if( pTransform == NULL )
		pTransform = &s_matIdentity;
	
	FILE *pFile = fopen( szDumpFile, "ab" );
	
	while( pHead )
	{
		if( pHead->pPoint->planarity == planarity )
		{
			const Vector vPointExtents( 0.5f, 0.5f, 0.01f );
			DumpAABBToGLView( (*pTransform) * pHead->pPoint->ptPosition, vPointExtents, vColor, pFile );
		}
		pHead = pHead->pNext;
	}

	fclose( pFile );
#endif
}

const char * DumpPolyhedronCutHistory( const CUtlVector<CPolyhedron *> &DumpedHistory, const CUtlVector<const float *> &CutHistory, const VMatrix *pTransform )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	if( pTransform == NULL )
		pTransform = &s_matIdentity;

	static char szDumpFile[100] = "FailedPolyhedronCut_Error.txt"; //most recent filename returned for further dumping

	for( int i = 0; i != DumpedHistory.Count(); ++i )
	{
		if( DumpedHistory[i] != NULL )
		{
			Q_snprintf( szDumpFile, 100, "FailedPolyhedronCut_%d.txt", i );
			DumpPolyhedronToGLView( DumpedHistory[i], szDumpFile, pTransform );
			DumpPlaneToGlView( CutHistory[i], 1.0f, szDumpFile, pTransform );
		}
	}

	return szDumpFile;
#else
	return NULL;
#endif
}

#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
#define AssertMsg_DumpPolyhedron(condition, message)\
	if( (condition) == false )\
	{\
		VMatrix matTransform;\
		matTransform.Identity();\
		matTransform[0][0] = matTransform[1][1] = matTransform[2][2] = 25.0f;\
		matTransform.SetTranslation( -DebugCutHistory.Tail()->Center() * 25.0f );\
		const char *szLastDumpFile = DumpPolyhedronCutHistory( DebugCutHistory, PlaneCutHistory, &matTransform );\
		DumpPointListToGLView( pAllPoints, POINT_ALIVE, Vector( 0.9f, 0.9f, 0.9f ), szLastDumpFile, &matTransform );\
		DumpPointListToGLView( pAllPoints, POINT_ONPLANE, Vector( 0.5f, 0.5f, 0.5f ), szLastDumpFile, &matTransform );\
		DumpPointListToGLView( pDeadPointCollection, POINT_DEAD, Vector( 0.1f, 0.1f, 0.1f ), szLastDumpFile, &matTransform );\
		if( pStartPoint )\
		{\
			FILE *pFileDumpRepairProgress = fopen( szLastDumpFile, "ab" );\
			DumpAABBToGLView( matTransform * pStartPoint->ptPosition, Vector( 2.0f, 0.05f, 0.05f ), Vector( 0.0f, 1.0f, 0.0f ), pFileDumpRepairProgress );\
			DumpAABBToGLView( matTransform * pWorkPoint->ptPosition, Vector( 2.0f, 0.05f, 0.05f ), Vector( 1.0f, 0.0f, 0.0f ), pFileDumpRepairProgress );\
			fclose( pFileDumpRepairProgress );\
		}\
		AssertMsg( condition, message );\
	}
#else
#define AssertMsg_DumpPolyhedron(condition, message) AssertMsg( condition, message )
#endif
#define Assert_DumpPolyhedron(condition) AssertMsg_DumpPolyhedron( condition, #condition )

#else

#define AssertMsg_DumpPolyhedron(condition, message) NULL;
#define Assert_DumpPolyhedron(condition) NULL;

#endif

CPolyhedron *ClipLinkedGeometry( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pAllPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pAllLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pAllPoints, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	const float fNegativeOnPlaneEpsilon = -fOnPlaneEpsilon;

#ifdef _DEBUG
	CUtlVector<CPolyhedron *> DebugCutHistory;
	CUtlVector<const float *> PlaneCutHistory;
	GeneratePolyhedronFromPlanes_Point *pStartPoint = NULL;
	GeneratePolyhedronFromPlanes_Point *pWorkPoint = NULL;

	static int iPolyhedronClipCount = 0;
	++iPolyhedronClipCount;
	
	DebugCutHistory.AddToTail( ConvertLinkedGeometryToPolyhedron( pAllPolygons, pAllLines, pAllPoints, false ) );
#endif

	//clear out polygon work variables
	{
		GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pAllPolygons;
		do
		{
			pActivePolygonWalk->pPolygon->bMissingASide = false;
			pActivePolygonWalk = pActivePolygonWalk->pNext;
		} while( pActivePolygonWalk );
	}


	//Collections of dead pointers for reallocation, shouldn't be touched until the current loop iteration is done.
	GeneratePolyhedronFromPlanes_UnorderedPointLL	*pDeadPointCollection = NULL;
	GeneratePolyhedronFromPlanes_UnorderedLineLL	*pDeadLineCollection = NULL;
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL	*pDeadPolygonCollection = NULL;
	GeneratePolyhedronFromPlanes_LineLL				*pDeadLineLinkCollection = NULL;


	for( int iCurrentPlane = 0; iCurrentPlane != iPlaneCount; ++iCurrentPlane )
	{
		//clear out line work variables
		{
			GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pAllLines;
			do
			{
				pActiveLineWalk->pLine->bAlive = false;
				pActiveLineWalk->pLine->bCut = false;

				pActiveLineWalk = pActiveLineWalk->pNext;
			} while( pActiveLineWalk );
		}
		
		//TODO: Move these pointers into a reallocation pool
		pDeadPointCollection = NULL; 
		pDeadLineCollection = NULL;
		pDeadLineLinkCollection = NULL;
		pDeadPolygonCollection = NULL;

		Vector vNormal = *((Vector *)&pOutwardFacingPlanes[(iCurrentPlane * 4) + 0]);
		/*double vNormalAsDouble[3];
		vNormalAsDouble[0] = vNormal.x;
		vNormalAsDouble[1] = vNormal.y;
		vNormalAsDouble[2] = vNormal.z;*/
		float fPlaneDist = pOutwardFacingPlanes[(iCurrentPlane * 4) + 3];

		//===================================================================================================
		// Step 1: Categorize each point as being either cut, split, or alive
		//===================================================================================================
		{
			bool bAllPointsDead = true;
			bool bAllPointsAlive = true;

			//find point distances from the plane
			GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pAllPoints;
			do
			{
				GeneratePolyhedronFromPlanes_Point *pPoint = pActivePointWalk->pPoint;
				float fPointDist = vNormal.Dot( pPoint->ptPosition ) - fPlaneDist;
				if( fPointDist > fOnPlaneEpsilon )
				{
					pPoint->planarity = POINT_DEAD; //point is dead, bang bang

					//mark connected lines as cut
					GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pPoint->pConnectedLines;
					GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
					do
					{
						pLineWalk->pLine->bCut = true;
						pLineWalk = pLineWalk->pNext;
					} while( pLineWalk != pFirstLine );

					bAllPointsAlive = false;
				}
				else if( fPointDist <= fNegativeOnPlaneEpsilon )
				{
					pPoint->planarity = POINT_ALIVE; //point is in behind plane, not voted off the island....yet
					bAllPointsDead = false;

					//mark connected lines as alive
					GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pPoint->pConnectedLines;
					GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
					do
					{
						pLineWalk->pLine->bAlive = true; //mark the line as alive
						pLineWalk = pLineWalk->pNext;
					} while( pLineWalk != pFirstLine );
				}
				else
				{
					pPoint->planarity = POINT_ONPLANE; //point is on the plane, he's everyone's buddy

					//Project on-plane points leaning towards death closer to the plane. This battles floating point precision decay.
					// Consider the case of a large on-plane epsilon leaving protrusions over time
					/*if( fPointDist < 0.0f )
					{
						double distAsDouble = fPointDist;
						double vPositionAsDouble[3];
						vPositionAsDouble[0] = pPoint->ptPosition.x;
						vPositionAsDouble[1] = pPoint->ptPosition.y;
						vPositionAsDouble[2] = pPoint->ptPosition.z;

						pPoint->ptPosition.x = vPositionAsDouble[0] - (distAsDouble * vNormalAsDouble[0]);
						pPoint->ptPosition.y = vPositionAsDouble[1] - (distAsDouble * vNormalAsDouble[1]);
						pPoint->ptPosition.z = vPositionAsDouble[2] - (distAsDouble * vNormalAsDouble[2]);

#if ( 0 && defined( _DEBUG ) )
						float fDebugDist = vNormal.Dot( pPoint->ptPosition ) - fPlaneDist; //just for looking at in watch windows
						AssertMsg( fabs( fDebugDist ) < fabs(fPointDist), "Projected point is further from plane than unprojected." );
#endif
						fPointDist = vNormal.Dot( pPoint->ptPosition ) - fPlaneDist; //recompute dist (not guaranteed to be 0.0 like we want)
					}*/				
				}

				pPoint->fPlaneDist = fPointDist;

				pActivePointWalk = pActivePointWalk->pNext;
			} while( pActivePointWalk );

			if( bAllPointsDead ) //all the points either died or are on the plane, no polyhedron left at all
			{
#ifdef _DEBUG
				for( int i = DebugCutHistory.Count(); --i >= 0; )
				{
					if( DebugCutHistory[i] )
						DebugCutHistory[i]->Release();
				}
				DebugCutHistory.RemoveAll();
#endif

				return NULL; 
			}

			if( bAllPointsAlive )
				continue; //no cuts made


			//Scan for onplane points connected to only other onplane/dead points, these points get downgraded to dead status.
			{
				pActivePointWalk = pAllPoints;
				do
				{
					if( pActivePointWalk->pPoint->planarity == POINT_ONPLANE )
					{
						GeneratePolyhedronFromPlanes_LineLL *pOnPlaneLineWalk = pActivePointWalk->pPoint->pConnectedLines;
						GeneratePolyhedronFromPlanes_LineLL *pStartLineWalk = pOnPlaneLineWalk;
						bool bDead = true; //assume it's dead and disprove
						do
						{
							if ( pOnPlaneLineWalk->pLine->bAlive )
							{
								bDead = false;
							}
							else if ( pOnPlaneLineWalk->pLine->bCut )
							{
								//connected to a dead point.
								if( pOnPlaneLineWalk->pNext->pLine->bCut || pOnPlaneLineWalk->pPrev->pLine->bCut )
								{
									//This on-plane point is surrounded by dead points on one polygon of the polyhedron.
									//	We have to downgrade this point to dead to avoid situations where float imprecision 
									//	turns the polyhedron into a *slightly* concave shape. Concave shapes might break this algorithm, even falsely concave shapes.
									bDead = true;
									break;
								}
							}

							pOnPlaneLineWalk = pOnPlaneLineWalk->pNext;
						} while( pOnPlaneLineWalk != pStartLineWalk );

						if( bDead )
						{
							pActivePointWalk->pPoint->planarity = POINT_DEAD;

							pOnPlaneLineWalk = pStartLineWalk;

							//mark connected lines as cut
							do
							{
								pOnPlaneLineWalk->pLine->bCut = true;
								pOnPlaneLineWalk = pOnPlaneLineWalk->pNext;
							} while( pOnPlaneLineWalk != pStartLineWalk );
						}
					}
					pActivePointWalk = pActivePointWalk->pNext;
				} while( pActivePointWalk );
			}
#ifdef _DEBUG
			PlaneCutHistory.AddToTail( &pOutwardFacingPlanes[iCurrentPlane * 4] );
#endif
		}

		


#ifdef _DEBUG
		//Run around the edges of all the polygons and ensure they don't have more than one point of lowered "alive" status (alive > onplane > dead) surrounded by higher status
		//	It indicates a concave shape. It's impossible to have it occur in theoretical space. But floating point numbers introduce error.
		{
			GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pDebugPolygonWalk = pAllPolygons;
			do
			{
				int iSurroundedCount = 0;
				GeneratePolyhedronFromPlanes_LineLL *pDebugLineWalk = pDebugPolygonWalk->pPolygon->pLines;
				GeneratePolyhedronFromPlanes_LineLL *pFirstDebugLine = pDebugLineWalk;

				do
				{
					PolyhedronPointPlanarity currentPlanarity = pDebugLineWalk->pLine->pPoints[pDebugLineWalk->iReferenceIndex]->planarity;
					
					GeneratePolyhedronFromPlanes_LineLL *pNext = pDebugLineWalk->pNext;
					PolyhedronPointPlanarity nextPlanarity = pNext->pLine->pPoints[pNext->iReferenceIndex]->planarity;

					if( currentPlanarity < nextPlanarity )
					{
						GeneratePolyhedronFromPlanes_LineLL *pPrev = pDebugLineWalk->pPrev;
						PolyhedronPointPlanarity prevPlanarity = pPrev->pLine->pPoints[pPrev->iReferenceIndex]->planarity;

						if( currentPlanarity < prevPlanarity )
						{
							++iSurroundedCount;
						}
					}

					pDebugLineWalk = pDebugLineWalk->pNext;
				} while( pDebugLineWalk != pFirstDebugLine );

				AssertMsg_DumpPolyhedron( iSurroundedCount <= 1, "Concave polygon, cutting process might break. Consider adjusting the on-plane epsilon to better compensate for floating point precision." );
				pDebugPolygonWalk = pDebugPolygonWalk->pNext;
			} while( pDebugPolygonWalk );
		}
#endif

		//===================================================================================================
		// Step 2: Remove dead lines. A dead line is one with a dead point that isn't connected to a living point
		//===================================================================================================
		{
			GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pAllLines;
			do
			{
				GeneratePolyhedronFromPlanes_Line *pLine = pActiveLineWalk->pLine;
				if( (pLine->bAlive == false) && (pLine->bCut == true) ) //not connected to a live point, but connected to a dead one. Dead line
				{
					//remove line from connected polygons
					for( int i = 0; i != 2; ++i )
					{
						GeneratePolyhedronFromPlanes_Polygon *pPolygon = pLine->pPolygons[i];
						GeneratePolyhedronFromPlanes_LineLL *pLineLink = pLine->pPolygonLineLinks[i];
                        
						pPolygon->bMissingASide = true;

						if( pLineLink->pNext == pLineLink )
						{
							//this was the last line of the polygon, it's dead
							pPolygon->pLines = NULL;
						}
						else
						{
							//link around this line
							pPolygon->pLines = pLineLink->pPrev; //Always have the polygon's head line be just before the gap in the polygon
							pLineLink->pNext->pPrev = pLineLink->pPrev;
							pLineLink->pPrev->pNext = pLineLink->pNext;
						}

						//move the line link to the dead list
						pLineLink->pNext = pDeadLineLinkCollection;
						pDeadLineLinkCollection = pLineLink;
					}

					//remove the line from connected points
					for( int i = 0; i != 2; ++i )
					{
						GeneratePolyhedronFromPlanes_Point *pPoint = pLine->pPoints[i];
						GeneratePolyhedronFromPlanes_LineLL *pLineLink = pLine->pPointLineLinks[i];
						
						if( pLineLink->pNext == pLineLink )
						{					
							//this is the last line
							pPoint->pConnectedLines = NULL;
							Assert( pPoint->planarity != POINT_ALIVE );
							pPoint->planarity = POINT_DEAD; //in case it was merely POINT_ONPLANE before
						}
						else
						{
							//link around this line
							pPoint->pConnectedLines = pLineLink->pNext; //in case pLineLink was the head line
							pLineLink->pNext->pPrev = pLineLink->pPrev;
							pLineLink->pPrev->pNext = pLineLink->pNext;
						}

						//move the line link to the dead list
						pLineLink->pNext = pDeadLineLinkCollection;
						pDeadLineLinkCollection = pLineLink;
					}

					//move the line to the dead list
					{
						//link past this node
						if( pActiveLineWalk->pPrev )
							pActiveLineWalk->pPrev->pNext = pActiveLineWalk->pNext;
						else
							pAllLines = pActiveLineWalk->pNext;

						if( pActiveLineWalk->pNext )
							pActiveLineWalk->pNext->pPrev = pActiveLineWalk->pPrev;

						GeneratePolyhedronFromPlanes_UnorderedLineLL *pNextLineWalk = pActiveLineWalk->pNext;
						
						//add to the dead list
						pActiveLineWalk->pNext = pDeadLineCollection;
						pDeadLineCollection = pActiveLineWalk;
						
						//next
						pActiveLineWalk = pNextLineWalk;
					}
				}
				else
				{
					pActiveLineWalk = pActiveLineWalk->pNext;
				}
			} while( pActiveLineWalk );
		}


		//===================================================================================================
		// Step 3: Remove dead polygons. A dead polygon has less than 2 lines.
		//===================================================================================================
		{
			GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pAllPolygons;
			do
			{
				GeneratePolyhedronFromPlanes_Polygon *pPolygon = pActivePolygonWalk->pPolygon;
				GeneratePolyhedronFromPlanes_LineLL *pHeadLine = pPolygon->pLines;

				bool bDead = (pHeadLine == NULL) || (pHeadLine->pNext == pHeadLine);
				if( !bDead )
				{
					//there's a rare case where a polygon can be almost entirely coplanar with the cut, it comes purely out of the land of imprecision
					bDead = true; //assume it's dead, and disprove

					GeneratePolyhedronFromPlanes_LineLL *pTestLineWalk = pHeadLine;
					do
					{
						if( pTestLineWalk->pLine->bAlive )
						{
							bDead = false;
							break;
						}
							
						pTestLineWalk = pTestLineWalk->pNext;
					} while( pTestLineWalk != pHeadLine );
				}

				if( bDead )
				{
					//dead polygon, move it to the dead list

					//link around this node
					if( pActivePolygonWalk->pPrev )
						pActivePolygonWalk->pPrev->pNext = pActivePolygonWalk->pNext;
					else
						pAllPolygons = pAllPolygons->pNext; //pActivePolygonWalk was the head node

					if( pActivePolygonWalk->pNext )
						pActivePolygonWalk->pNext->pPrev = pActivePolygonWalk->pPrev;

					GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pNextPolygonWalk = pActivePolygonWalk->pNext;

					//add to the dead list
					pActivePolygonWalk->pNext = pDeadPolygonCollection;
					pDeadPolygonCollection = pActivePolygonWalk;

					//next
					pActivePolygonWalk = pNextPolygonWalk;
				}
				else
				{
					AssertMsg_DumpPolyhedron( (pActivePolygonWalk->pPolygon->pLines != NULL) && 
						(pActivePolygonWalk->pPolygon->pLines != pActivePolygonWalk->pPolygon->pLines->pNext), "Living polygon with less than 2 lines" );
					
					pActivePolygonWalk = pActivePolygonWalk->pNext;
				}
			} while( pActivePolygonWalk );
		}

		//===================================================================================================
		// Step 4: Remove dead points.
		//===================================================================================================
		{
			GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pAllPoints;
			do
			{
				if( pActivePointWalk->pPoint->planarity == POINT_DEAD )
				{
					GeneratePolyhedronFromPlanes_UnorderedPointLL *pNext = pActivePointWalk->pNext;

					if( pActivePointWalk->pPrev )
						pActivePointWalk->pPrev->pNext = pActivePointWalk->pNext;
					else
						pAllPoints = pAllPoints->pNext;

					if( pActivePointWalk->pNext )
						pActivePointWalk->pNext->pPrev = pActivePointWalk->pPrev;

					pActivePointWalk->pNext = pDeadPointCollection;
					pDeadPointCollection = pActivePointWalk;

					pActivePointWalk = pNext;
				}
				else
				{
					pActivePointWalk = pActivePointWalk->pNext;
				}				
			} while( pActivePointWalk );
		}


		//===================================================================================================
		// Step 5: Handle cut lines
		//===================================================================================================
		{
			GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pAllLines;
			do
			{
				GeneratePolyhedronFromPlanes_Line *pWorkLine = pActiveLineWalk->pLine;
				Assert_DumpPolyhedron( (pWorkLine->bAlive == true) || (pWorkLine->bCut == false) ); //all dead lines should have already been removed
				
				if( pWorkLine->bCut )
				{
					GeneratePolyhedronFromPlanes_Point **pLinePoints = pWorkLine->pPoints;

					Assert_DumpPolyhedron( (pLinePoints[0]->planarity == POINT_DEAD) || (pLinePoints[1]->planarity == POINT_DEAD) ); //one of the two has to be a dead point

					int iDeadIndex = (pLinePoints[0]->planarity == POINT_DEAD)?(0):(1);
					int iLivingIndex = 1 - iDeadIndex;
					GeneratePolyhedronFromPlanes_Point *pDeadPoint = pLinePoints[iDeadIndex];
					GeneratePolyhedronFromPlanes_Point *pLivingPoint = pLinePoints[iLivingIndex];

					Assert_DumpPolyhedron( pLivingPoint->planarity == POINT_ALIVE ); //if this point were on-plane or dead, the line should be dead

					//We'll be de-linking from the old point and generating a new one. We do this so other lines can still access the dead point's untouched data.
					
					//Generate a new point
					GeneratePolyhedronFromPlanes_Point *pNewPoint = (GeneratePolyhedronFromPlanes_Point *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Point ) );
					{
						//add this point to the active list
						pAllPoints->pPrev = (GeneratePolyhedronFromPlanes_UnorderedPointLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedPointLL ) );
						pAllPoints->pPrev->pNext = pAllPoints;
						pAllPoints = pAllPoints->pPrev;
						pAllPoints->pPrev = NULL;
						pAllPoints->pPoint = pNewPoint;


						float fInvTotalDist = 1.0f/(pDeadPoint->fPlaneDist - pLivingPoint->fPlaneDist); //subtraction because the living index is known to be negative
						pNewPoint->ptPosition = (pLivingPoint->ptPosition * (pDeadPoint->fPlaneDist * fInvTotalDist)) - (pDeadPoint->ptPosition * (pLivingPoint->fPlaneDist * fInvTotalDist));


						pNewPoint->planarity = POINT_ONPLANE;
						pNewPoint->fPlaneDist = 0.0f;
					}
					
					GeneratePolyhedronFromPlanes_LineLL *pNewLineLink = pNewPoint->pConnectedLines = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
					pNewLineLink->pLine = pWorkLine;
					pNewLineLink->pNext = pNewLineLink;
					pNewLineLink->pPrev = pNewLineLink;
					pNewLineLink->iReferenceIndex = iLivingIndex;

					pWorkLine->pPoints[iDeadIndex] = pNewPoint;
					pWorkLine->pPointLineLinks[iDeadIndex] = pNewLineLink;
					pNewPoint->pConnectedLines = pNewLineLink;

					//A new line is needed on each polygon touching the dead point to connect the two new endpoints for split lines. 
					// So mark connected polygons as missing a side.
					for( int i = 0; i != 2; ++i )
						pWorkLine->pPolygons[i]->bMissingASide = true;
					

					//Always have a cut polygon's head line be just before the gap in the polygon. 
					// In this case, we know that one of the two polygons goes clockwise into the dead point, so have that polygon point at this line.
					// We don't know enough about the other polygon to do anything here, but another cut line will handle that polygon. So it all works out in the end.
					pWorkLine->pPolygons[iDeadIndex]->pLines = pWorkLine->pPolygonLineLinks[iDeadIndex];
				}

				pActiveLineWalk = pActiveLineWalk->pNext;
			} while( pActiveLineWalk );
		}


		//===================================================================================================
		// Step 6: Repair polygons that are missing a side. And generate the new coplanar polygon.
		//===================================================================================================
		{
			//Find the first polygon missing a side.
			// We'll then walk from polygon to polygon using line connections so that we can generate the new polygon in a clockwise manner.
			GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pAllPolygons;

			while( (pActivePolygonWalk != NULL) && (pActivePolygonWalk->pPolygon->bMissingASide == false) )
			{
				pActivePolygonWalk = pActivePolygonWalk->pNext;
			}

			//acquire iteration data
#ifndef _DEBUG
			GeneratePolyhedronFromPlanes_Point *pStartPoint;
			GeneratePolyhedronFromPlanes_Point *pWorkPoint;
#endif

			GeneratePolyhedronFromPlanes_LineLL *pLastLineLink;
			GeneratePolyhedronFromPlanes_Polygon *pWorkPolygon;			
			GeneratePolyhedronFromPlanes_LineLL *pTestLine;

#ifdef _DEBUG
			GeneratePolyhedronFromPlanes_Polygon *pLastWorkPolygon = NULL;
			GeneratePolyhedronFromPlanes_Point *pLastWorkPoint = NULL;
#endif

			if( pActivePolygonWalk )
			{
				//grab the polygon we'll be starting with
				GeneratePolyhedronFromPlanes_Polygon *pBrokenPolygon = pActivePolygonWalk->pPolygon;
				
				{
					GeneratePolyhedronFromPlanes_LineLL *pTemp = pBrokenPolygon->pLines->pNext;
					pStartPoint = pTemp->pLine->pPoints[1 - pTemp->iReferenceIndex];
					Assert_DumpPolyhedron( pStartPoint->planarity == POINT_ONPLANE ); //every working point should be coplanar
					pLastLineLink = pTemp->pLine->pPointLineLinks[1 - pTemp->iReferenceIndex]->pNext;
					pWorkPolygon = pBrokenPolygon;
				}

				pWorkPoint = pStartPoint;
				pTestLine = pLastLineLink->pPrev; //rotate counterclockwise around the point
			}
			else
			{
				//apparently the plane was entirely through existing polygonal borders, extremely rare but it can happen with inefficient cutting planes
                GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pAllPoints;
				while( (pActivePointWalk != NULL) && (pActivePointWalk->pPoint->planarity != POINT_ONPLANE) )
				{
					pActivePointWalk = pActivePointWalk->pNext;
				}

				Assert( pActivePointWalk != NULL );

				pStartPoint = pWorkPoint = pActivePointWalk->pPoint;
				GeneratePolyhedronFromPlanes_LineLL *pLines = pWorkPoint->pConnectedLines;
				
				while( !pLines->pLine->bAlive ) //seek clockwise until we find a line not on the plane
					pLines = pLines->pNext;

				while( pLines->pLine->bAlive ) //now seek counterclockwise until we find a line on the plane (in case we started on an alive line last seek)
					pLines = pLines->pPrev;

				//now pLines points at one side of the polygon, with pActivePointWalk
				pLastLineLink = pLines;
				pTestLine = pLines->pPrev;
				pWorkPolygon = pTestLine->pLine->pPolygons[1 - pTestLine->iReferenceIndex];

			}

			//create the new polygon
			GeneratePolyhedronFromPlanes_Polygon *pNewPolygon = (GeneratePolyhedronFromPlanes_Polygon *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Polygon ) );
			{
				//before we forget, add this polygon to the active list
				pAllPolygons->pPrev = (GeneratePolyhedronFromPlanes_UnorderedPolygonLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedPolygonLL ) );
				pAllPolygons->pPrev->pNext = pAllPolygons;
				pAllPolygons = pAllPolygons->pPrev;
				pAllPolygons->pPrev = NULL;
				pAllPolygons->pPolygon = pNewPolygon;

				pNewPolygon->bMissingASide = false; //technically missing all it's sides, but we're fixing it now
				pNewPolygon->vSurfaceNormal = vNormal;
				pNewPolygon->pLines = NULL;
			}



			//===================================================================================================================
			// The general idea of the upcoming algorithm to put together a new polygon and patch broken polygons...
			//	You have a point and a line the algorithm just jumped across.
			//		1. Rotate through the point's line links one time counterclockwise (pPrev)
			//		2. If the line is cut, then we make a new bridging line in the polygon between that line and the one counterclockwise to it. (pPrev)
			//			If the line is on-plane. Skip the bridge line making, but set links to the new polygon as if we'd just created the bridge
			//		3. Once we follow a line back to the point where we started, we should be all done.

			do
			{
				if( pWorkPolygon->bMissingASide )
				{
					//during the cutting process we made sure that the head line link was going clockwise into the missing area
					GeneratePolyhedronFromPlanes_LineLL *pGapLines[2];
					pGapLines[1] = pTestLine->pLine->pPolygonLineLinks[pTestLine->iReferenceIndex]; //get the same line, but in the polygons linked list.
					Assert_DumpPolyhedron( pGapLines[1]->pLine == pTestLine->pLine );
					pGapLines[0] = pGapLines[1]->pPrev;

					Assert_DumpPolyhedron( pWorkPolygon->bMissingASide );

#ifdef _DEBUG
					{
						//ensure that the space between the gap lines is the only space where fixing is required
						GeneratePolyhedronFromPlanes_LineLL *pDebugLineWalk = pGapLines[1]->pNext;
						
						while( pDebugLineWalk != pGapLines[0] )
						{
							Assert_DumpPolyhedron( pDebugLineWalk->pLine->bCut == false );
							pDebugLineWalk = pDebugLineWalk->pNext;
						}
					}
#endif

					GeneratePolyhedronFromPlanes_Line *pJoinLine = (GeneratePolyhedronFromPlanes_Line *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Line ) );
					{
						//before we forget, add this line to the active list
						pAllLines->pPrev = (GeneratePolyhedronFromPlanes_UnorderedLineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedLineLL ) );
						pAllLines->pPrev->pNext = pAllLines;
						pAllLines = pAllLines->pPrev;
						pAllLines->pPrev = NULL;
						pAllLines->pLine = pJoinLine;

						pJoinLine->bAlive = false;
						pJoinLine->bCut = false;
					}


					pJoinLine->pPoints[0] = pGapLines[0]->pLine->pPoints[pGapLines[0]->iReferenceIndex];
					pJoinLine->pPoints[1] = pGapLines[1]->pLine->pPoints[1 - pGapLines[1]->iReferenceIndex];

					pJoinLine->pPolygons[0] = pNewPolygon;
					pJoinLine->pPolygons[1] = pWorkPolygon;

					//now create all 4 links into the line
					GeneratePolyhedronFromPlanes_LineLL *pPointLinks[2];
					pPointLinks[0] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
					pPointLinks[1] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );

					GeneratePolyhedronFromPlanes_LineLL *pPolygonLinks[2];
					pPolygonLinks[0] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
					pPolygonLinks[1] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );

					pPointLinks[0]->pLine = pPointLinks[1]->pLine = pPolygonLinks[0]->pLine = pPolygonLinks[1]->pLine = pJoinLine;

					pJoinLine->pPointLineLinks[0] = pPointLinks[0];
					pJoinLine->pPointLineLinks[1] = pPointLinks[1];
					pJoinLine->pPolygonLineLinks[0] = pPolygonLinks[0];
					pJoinLine->pPolygonLineLinks[1] = pPolygonLinks[1];



					pPointLinks[0]->iReferenceIndex = 1;
					pPointLinks[1]->iReferenceIndex = 0;

					//Insert before the link from point 0 to gap line 0 (counterclockwise rotation)
					{
						GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pGapLines[0]->pLine->pPointLineLinks[pGapLines[0]->iReferenceIndex];
						Assert_DumpPolyhedron( pWorkLink->pLine == pGapLines[0]->pLine );

						pPointLinks[0]->pPrev = pWorkLink->pPrev;
						pPointLinks[0]->pNext = pWorkLink;

						pWorkLink->pPrev->pNext = pPointLinks[0];
						pWorkLink->pPrev = pPointLinks[0];						
					}

					//Insert after the link from point 1 to gap line 1 (clockwise rotation)
					{
						GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pGapLines[1]->pLine->pPointLineLinks[1 - pGapLines[1]->iReferenceIndex];
						Assert_DumpPolyhedron( pWorkLink->pLine == pGapLines[1]->pLine );

						pPointLinks[1]->pNext = pWorkLink->pNext;
						pPointLinks[1]->pPrev = pWorkLink;
						
						pWorkLink->pNext->pPrev = pPointLinks[1];
						pWorkLink->pNext = pPointLinks[1];						
					}




					pPolygonLinks[0]->iReferenceIndex = 0;
					pPolygonLinks[1]->iReferenceIndex = 1;

					//Insert before the head line in the new polygon (at the end of the clockwise order)
					{
						if( pNewPolygon->pLines == NULL )
						{
							//this is the first line being added to the polygon
							pNewPolygon->pLines = pPolygonLinks[0];
							pPolygonLinks[0]->pNext = pPolygonLinks[0];
							pPolygonLinks[0]->pPrev = pPolygonLinks[0];
						}
						else
						{
							GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pNewPolygon->pLines;

							pPolygonLinks[0]->pNext = pWorkLink;
							pPolygonLinks[0]->pPrev = pWorkLink->pPrev;

							pWorkLink->pPrev->pNext = pPolygonLinks[0];
							pWorkLink->pPrev = pPolygonLinks[0];
						}
					}

					//Insert after the head line in the work polygon
					{
						GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pWorkPolygon->pLines;

						pPolygonLinks[1]->pNext = pWorkLink->pNext;
						pPolygonLinks[1]->pPrev = pWorkLink;

						pWorkLink->pNext->pPrev = pPolygonLinks[1];
						pWorkLink->pNext = pPolygonLinks[1];
					}

					pWorkPolygon->bMissingASide = false; //repairs are finished

#ifdef _DEBUG
					pLastWorkPolygon = pWorkPolygon;
					pLastWorkPoint = pWorkPoint;
#endif
					//move to the next point
					pWorkPoint = pJoinLine->pPoints[0];
					pLastLineLink = pJoinLine->pPointLineLinks[0];
					Assert_DumpPolyhedron( pWorkPoint->planarity == POINT_ONPLANE ); //every working point should be coplanar
					
					pTestLine = pLastLineLink->pPrev;
					if( pTestLine->pLine->pPoints[pTestLine->iReferenceIndex]->planarity == POINT_ALIVE )
						pWorkPolygon = pTestLine->pLine->pPolygons[pTestLine->iReferenceIndex];
					else
						pWorkPolygon = pTestLine->pLine->pPolygons[1 - pTestLine->iReferenceIndex];
					
					Assert_DumpPolyhedron( pWorkPolygon != pLastWorkPolygon );
					Assert_DumpPolyhedron( (pWorkPoint == pStartPoint) ||
											(pGapLines[0]->pLine->bCut == false) || 
											(pWorkPolygon->bMissingASide == true) ); //if we're not done fixing, and if the shared line was cut, the next polygon must be missing a side
				}
				else
				{
					//line is on the plane, meaning the polygon isn't broken and doesn't need patching
					Assert_DumpPolyhedron( pTestLine->pLine->bCut == false );
					Assert_DumpPolyhedron( (pTestLine->pLine->pPoints[0]->planarity == POINT_ONPLANE) && (pTestLine->pLine->pPoints[1]->planarity == POINT_ONPLANE) );

					
					//link to this line from the new polygon
					GeneratePolyhedronFromPlanes_LineLL *pNewLineLink;
					pNewLineLink = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
					
					pNewLineLink->pLine = pTestLine->pLine;
					pNewLineLink->iReferenceIndex = pTestLine->iReferenceIndex;

					//Insert before the head line in the new polygon (at the end of the clockwise order)
					{
						if( pNewPolygon->pLines == NULL )
						{
							//this is the first line being added to the polygon
							pNewPolygon->pLines = pNewLineLink;
							pNewLineLink->pNext = pNewLineLink;
							pNewLineLink->pPrev = pNewLineLink;
						}
						else
						{
							GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pNewPolygon->pLines;

							pNewLineLink->pNext = pWorkLink;
							pNewLineLink->pPrev = pWorkLink->pPrev;

							pWorkLink->pPrev->pNext = pNewLineLink;
							pWorkLink->pPrev = pNewLineLink;
						}
					}

					//Since the entire line is on the plane, that means it used to point to something that used to reside where the new polygon is going
					// Update the link to the new the polygon pointer and be on our way
					pTestLine->pLine->pPolygons[pTestLine->iReferenceIndex] = pNewPolygon;
					pTestLine->pLine->pPolygonLineLinks[pTestLine->iReferenceIndex] = pNewLineLink;

#ifdef _DEBUG
					pLastWorkPolygon = pWorkPolygon;
					pLastWorkPoint = pWorkPoint;
#endif

					pWorkPoint = pTestLine->pLine->pPoints[pTestLine->iReferenceIndex];
					pLastLineLink = pTestLine->pLine->pPointLineLinks[pTestLine->iReferenceIndex];
					Assert_DumpPolyhedron( pWorkPoint->planarity == POINT_ONPLANE ); //every working point should be coplanar

					pTestLine = pLastLineLink->pPrev;
					if( pTestLine->pLine->pPoints[pTestLine->iReferenceIndex]->planarity == POINT_ALIVE )
						pWorkPolygon = pTestLine->pLine->pPolygons[pTestLine->iReferenceIndex];
					else
						pWorkPolygon = pTestLine->pLine->pPolygons[1 - pTestLine->iReferenceIndex];

					Assert_DumpPolyhedron( pWorkPolygon != pLastWorkPolygon );
				}
			} while( pWorkPoint != pStartPoint );
		}

#ifdef _DEBUG
		//verify that repairs are complete
		{
			GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pDebugPolygonWalk = pAllPolygons;
			do
			{
				AssertMsg_DumpPolyhedron( pDebugPolygonWalk->pPolygon->bMissingASide == false, "Some polygons not repaired after cut" );
				pDebugPolygonWalk = pDebugPolygonWalk->pNext;
			} while( pDebugPolygonWalk );


			GeneratePolyhedronFromPlanes_UnorderedPointLL *pDebugPointWalk = pAllPoints;
			do
			{
				AssertMsg_DumpPolyhedron( pDebugPointWalk->pPoint->pConnectedLines, "Point connected to no lines after cut" );
				pDebugPointWalk = pDebugPointWalk->pNext;
			} while( pDebugPointWalk );

			pStartPoint = NULL;
		}

		//maintain the cut history
		DebugCutHistory.AddToTail( ConvertLinkedGeometryToPolyhedron( pAllPolygons, pAllLines, pAllPoints, false ) );
#endif
	}

#ifdef _DEBUG
	for( int i = DebugCutHistory.Count(); --i >= 0; )
	{
		if( DebugCutHistory[i] )
			DebugCutHistory[i]->Release();
	}
	DebugCutHistory.RemoveAll();
#endif

	return ConvertLinkedGeometryToPolyhedron( pAllPolygons, pAllLines, pAllPoints, bUseTemporaryMemory );
}



#define STARTPOINTTOLINELINKS(iPointNum, lineindex1, iOtherPointIndex1, lineindex2, iOtherPointIndex2, lineindex3, iOtherPointIndex3 )\
	StartingBoxPoints[iPointNum].pConnectedLines = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pLine = &StartingBoxLines[lineindex1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].iReferenceIndex = iOtherPointIndex1;\
	StartingBoxLines[lineindex1].pPointLineLinks[1 - iOtherPointIndex1] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pLine = &StartingBoxLines[lineindex2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].iReferenceIndex = iOtherPointIndex2;\
	StartingBoxLines[lineindex2].pPointLineLinks[1 - iOtherPointIndex2] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pLine = &StartingBoxLines[lineindex3];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].iReferenceIndex = iOtherPointIndex3;\
	StartingBoxLines[lineindex3].pPointLineLinks[1 - iOtherPointIndex3] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];

#define STARTBOXCONNECTION( linenum, point1, point2, poly1, poly2 )\
	StartingBoxLines[linenum].pPoints[0] = &StartingBoxPoints[point1];\
	StartingBoxLines[linenum].pPoints[1] = &StartingBoxPoints[point2];\
	StartingBoxLines[linenum].pPolygons[0] = &StartingBoxPolygons[poly1];\
	StartingBoxLines[linenum].pPolygons[1] = &StartingBoxPolygons[poly2];

#define STARTPOLYGONTOLINELINKS( polynum, lineindex1, iThisPolyIndex1, lineindex2, iThisPolyIndex2, lineindex3, iThisPolyIndex3, lineindex4, iThisPolyIndex4 )\
	StartingBoxPolygons[polynum].pLines = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pLine = &StartingBoxLines[lineindex1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].iReferenceIndex = iThisPolyIndex1;\
	StartingBoxLines[lineindex1].pPolygonLineLinks[iThisPolyIndex1] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pLine = &StartingBoxLines[lineindex2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].iReferenceIndex = iThisPolyIndex2;\
	StartingBoxLines[lineindex2].pPolygonLineLinks[iThisPolyIndex2] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pLine = &StartingBoxLines[lineindex3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].iReferenceIndex = iThisPolyIndex3;\
	StartingBoxLines[lineindex3].pPolygonLineLinks[iThisPolyIndex3] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pLine = &StartingBoxLines[lineindex4];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].iReferenceIndex = iThisPolyIndex4;\
	StartingBoxLines[lineindex4].pPolygonLineLinks[iThisPolyIndex4] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];


CPolyhedron *GeneratePolyhedronFromPlanes( const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	//this is version 2 of the polyhedron generator, version 1 made individual polygons and joined points together, some guesswork is involved and it therefore isn't a solid method
	//this version will start with a cube and hack away at it (retaining point connection information) to produce a polyhedron with no guesswork involved, this method should be rock solid
	
	//the polygon clipping functions we're going to use want inward facing planes
	float *pFlippedPlanes = (float *)stackalloc( (iPlaneCount * 4) * sizeof( float ) );
	for( int i = 0; i != iPlaneCount * 4; ++i )
	{
		pFlippedPlanes[i] = -pOutwardFacingPlanes[i];
	}

	//our first goal is to find the size of a cube big enough to encapsulate all points that will be in the final polyhedron
	Vector vAABBMins, vAABBMaxs;
	if( FindConvexShapeLooseAABB( pFlippedPlanes, iPlaneCount, &vAABBMins, &vAABBMaxs ) == false )
		return NULL; //no shape to work with apparently

	
	//grow the bounding box to a larger size since it's probably inaccurate a bit
	{
		Vector vGrow = (vAABBMaxs - vAABBMins) * 0.5f;
		vGrow.x += 100.0f;
		vGrow.y += 100.0f;
		vGrow.z += 100.0f;

		vAABBMaxs += vGrow;
		vAABBMins -= vGrow;
	}

	//generate our starting cube using the 2x AABB so we can start hacking away at it
	
	

	//create our starting box on the stack
	GeneratePolyhedronFromPlanes_Point StartingBoxPoints[8];
	GeneratePolyhedronFromPlanes_Line StartingBoxLines[12];
	GeneratePolyhedronFromPlanes_Polygon StartingBoxPolygons[6];
	GeneratePolyhedronFromPlanes_LineLL StartingPoints_To_Lines_Links[24]; //8 points, 3 lines per point
	GeneratePolyhedronFromPlanes_LineLL StartingPolygon_To_Lines_Links[24]; //6 polygons, 4 lines per poly
	
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL StartingPolygonList[6]; //6 polygons
	GeneratePolyhedronFromPlanes_UnorderedLineLL StartingLineList[12]; //12 lines
	GeneratePolyhedronFromPlanes_UnorderedPointLL StartingPointList[8]; //8 points


	//I had to work all this out on a whiteboard if it seems completely unintuitive.
	{
		StartingBoxPoints[0].ptPosition.Init( vAABBMins.x, vAABBMins.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 0, 0, 1, 4, 1, 3, 0 );

		StartingBoxPoints[1].ptPosition.Init( vAABBMins.x, vAABBMaxs.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 1, 0, 0, 1, 1, 5, 1 );

		StartingBoxPoints[2].ptPosition.Init( vAABBMins.x, vAABBMins.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 2, 4, 0, 8, 1, 11, 0 );

		StartingBoxPoints[3].ptPosition.Init( vAABBMins.x, vAABBMaxs.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 3, 5, 0, 9, 1, 8, 0 );

		StartingBoxPoints[4].ptPosition.Init( vAABBMaxs.x, vAABBMins.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 4, 2, 0, 3, 1, 7, 1 );

		StartingBoxPoints[5].ptPosition.Init( vAABBMaxs.x, vAABBMaxs.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 5, 1, 0, 2, 1, 6, 1 );

		StartingBoxPoints[6].ptPosition.Init( vAABBMaxs.x, vAABBMins.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 6, 7, 0, 11, 1, 10, 0 );

		StartingBoxPoints[7].ptPosition.Init( vAABBMaxs.x, vAABBMaxs.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 7, 6, 0, 10, 1, 9, 0 );

		STARTBOXCONNECTION( 0, 0, 1, 0, 5 );
		STARTBOXCONNECTION( 1, 1, 5, 1, 5 );
		STARTBOXCONNECTION( 2, 5, 4, 2, 5 );
		STARTBOXCONNECTION( 3, 4, 0, 3, 5 );
		STARTBOXCONNECTION( 4, 0, 2, 3, 0 );
		STARTBOXCONNECTION( 5, 1, 3, 0, 1 );
		STARTBOXCONNECTION( 6, 5, 7, 1, 2 );
		STARTBOXCONNECTION( 7, 4, 6, 2, 3 );
		STARTBOXCONNECTION( 8, 2, 3, 4, 0 );
		STARTBOXCONNECTION( 9, 3, 7, 4, 1 );
		STARTBOXCONNECTION( 10, 7, 6, 4, 2 );
		STARTBOXCONNECTION( 11, 6, 2, 4, 3 );


		STARTBOXCONNECTION( 0, 0, 1, 5, 0 );
		STARTBOXCONNECTION( 1, 1, 5, 5, 1 );
		STARTBOXCONNECTION( 2, 5, 4, 5, 2 );
		STARTBOXCONNECTION( 3, 4, 0, 5, 3 );
		STARTBOXCONNECTION( 4, 0, 2, 0, 3 );
		STARTBOXCONNECTION( 5, 1, 3, 1, 0 );
		STARTBOXCONNECTION( 6, 5, 7, 2, 1 );
		STARTBOXCONNECTION( 7, 4, 6, 3, 2 );
		STARTBOXCONNECTION( 8, 2, 3, 0, 4 );
		STARTBOXCONNECTION( 9, 3, 7, 1, 4 );
		STARTBOXCONNECTION( 10, 7, 6, 2, 4 );
		STARTBOXCONNECTION( 11, 6, 2, 3, 4 );

		StartingBoxPolygons[0].vSurfaceNormal.Init( -1.0f, 0.0f, 0.0f );
		StartingBoxPolygons[1].vSurfaceNormal.Init( 0.0f, 1.0f, 0.0f );
		StartingBoxPolygons[2].vSurfaceNormal.Init( 1.0f, 0.0f, 0.0f );
		StartingBoxPolygons[3].vSurfaceNormal.Init( 0.0f, -1.0f, 0.0f );
		StartingBoxPolygons[4].vSurfaceNormal.Init( 0.0f, 0.0f, 1.0f );
		StartingBoxPolygons[5].vSurfaceNormal.Init( 0.0f, 0.0f, -1.0f );


		STARTPOLYGONTOLINELINKS( 0, 0, 1, 5, 1, 8, 0, 4, 0 );
		STARTPOLYGONTOLINELINKS( 1, 1, 1, 6, 1, 9, 0, 5, 0 );
		STARTPOLYGONTOLINELINKS( 2, 2, 1, 7, 1, 10, 0, 6, 0 );
		STARTPOLYGONTOLINELINKS( 3, 3, 1, 4, 1, 11, 0, 7, 0 );
		STARTPOLYGONTOLINELINKS( 4, 8, 1, 9, 1, 10, 1, 11, 1 );
		STARTPOLYGONTOLINELINKS( 5, 0, 0, 3, 0, 2, 0, 1, 0 );


		{
			StartingPolygonList[0].pPolygon = &StartingBoxPolygons[0];
			StartingPolygonList[0].pNext = &StartingPolygonList[1];
			StartingPolygonList[0].pPrev = NULL;

			StartingPolygonList[1].pPolygon = &StartingBoxPolygons[1];
			StartingPolygonList[1].pNext = &StartingPolygonList[2];
			StartingPolygonList[1].pPrev = &StartingPolygonList[0];

			StartingPolygonList[2].pPolygon = &StartingBoxPolygons[2];
			StartingPolygonList[2].pNext = &StartingPolygonList[3];
			StartingPolygonList[2].pPrev = &StartingPolygonList[1];

			StartingPolygonList[3].pPolygon = &StartingBoxPolygons[3];
			StartingPolygonList[3].pNext = &StartingPolygonList[4];
			StartingPolygonList[3].pPrev = &StartingPolygonList[2];

			StartingPolygonList[4].pPolygon = &StartingBoxPolygons[4];
			StartingPolygonList[4].pNext = &StartingPolygonList[5];
			StartingPolygonList[4].pPrev = &StartingPolygonList[3];

			StartingPolygonList[5].pPolygon = &StartingBoxPolygons[5];
			StartingPolygonList[5].pNext = NULL;
			StartingPolygonList[5].pPrev = &StartingPolygonList[4];
		}



		{
			StartingLineList[0].pLine = &StartingBoxLines[0];
			StartingLineList[0].pNext = &StartingLineList[1];
			StartingLineList[0].pPrev = NULL;

			StartingLineList[1].pLine = &StartingBoxLines[1];
			StartingLineList[1].pNext = &StartingLineList[2];
			StartingLineList[1].pPrev = &StartingLineList[0];

			StartingLineList[2].pLine = &StartingBoxLines[2];
			StartingLineList[2].pNext = &StartingLineList[3];
			StartingLineList[2].pPrev = &StartingLineList[1];

			StartingLineList[3].pLine = &StartingBoxLines[3];
			StartingLineList[3].pNext = &StartingLineList[4];
			StartingLineList[3].pPrev = &StartingLineList[2];

			StartingLineList[4].pLine = &StartingBoxLines[4];
			StartingLineList[4].pNext = &StartingLineList[5];
			StartingLineList[4].pPrev = &StartingLineList[3];

			StartingLineList[5].pLine = &StartingBoxLines[5];
			StartingLineList[5].pNext = &StartingLineList[6];
			StartingLineList[5].pPrev = &StartingLineList[4];

			StartingLineList[6].pLine = &StartingBoxLines[6];
			StartingLineList[6].pNext = &StartingLineList[7];
			StartingLineList[6].pPrev = &StartingLineList[5];

			StartingLineList[7].pLine = &StartingBoxLines[7];
			StartingLineList[7].pNext = &StartingLineList[8];
			StartingLineList[7].pPrev = &StartingLineList[6];

			StartingLineList[8].pLine = &StartingBoxLines[8];
			StartingLineList[8].pNext = &StartingLineList[9];
			StartingLineList[8].pPrev = &StartingLineList[7];

			StartingLineList[9].pLine = &StartingBoxLines[9];
			StartingLineList[9].pNext = &StartingLineList[10];
			StartingLineList[9].pPrev = &StartingLineList[8];

			StartingLineList[10].pLine = &StartingBoxLines[10];
			StartingLineList[10].pNext = &StartingLineList[11];
			StartingLineList[10].pPrev = &StartingLineList[9];

			StartingLineList[11].pLine = &StartingBoxLines[11];
			StartingLineList[11].pNext = NULL;
			StartingLineList[11].pPrev = &StartingLineList[10];
		}

		{
			StartingPointList[0].pPoint = &StartingBoxPoints[0];
			StartingPointList[0].pNext = &StartingPointList[1];
			StartingPointList[0].pPrev = NULL;

			StartingPointList[1].pPoint = &StartingBoxPoints[1];
			StartingPointList[1].pNext = &StartingPointList[2];
			StartingPointList[1].pPrev = &StartingPointList[0];

			StartingPointList[2].pPoint = &StartingBoxPoints[2];
			StartingPointList[2].pNext = &StartingPointList[3];
			StartingPointList[2].pPrev = &StartingPointList[1];

			StartingPointList[3].pPoint = &StartingBoxPoints[3];
			StartingPointList[3].pNext = &StartingPointList[4];
			StartingPointList[3].pPrev = &StartingPointList[2];

			StartingPointList[4].pPoint = &StartingBoxPoints[4];
			StartingPointList[4].pNext = &StartingPointList[5];
			StartingPointList[4].pPrev = &StartingPointList[3];

			StartingPointList[5].pPoint = &StartingBoxPoints[5];
			StartingPointList[5].pNext = &StartingPointList[6];
			StartingPointList[5].pPrev = &StartingPointList[4];

			StartingPointList[6].pPoint = &StartingBoxPoints[6];
			StartingPointList[6].pNext = &StartingPointList[7];
			StartingPointList[6].pPrev = &StartingPointList[5];

			StartingPointList[7].pPoint = &StartingBoxPoints[7];
			StartingPointList[7].pNext = NULL;
			StartingPointList[7].pPrev = &StartingPointList[6];
		}
	}

	return ClipLinkedGeometry( StartingPolygonList, StartingLineList, StartingPointList, pOutwardFacingPlanes, iPlaneCount, fOnPlaneEpsilon, bUseTemporaryMemory );
}










































#ifdef _DEBUG
void DumpAABBToGLView( const Vector &vCenter, const Vector &vExtents, const Vector &vColor, FILE *pFile )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	Vector vMins = vCenter - vExtents;
	Vector vMaxs = vCenter + vExtents;

	//x min side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );	
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );

	//x max side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );	
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );


	//y min side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );	
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );



	//y max side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );	
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );



	//z min side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );	
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, vColor.x, vColor.y, vColor.z );


	//z max side
	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );

	fprintf( pFile, "4\n" );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, vColor.x, vColor.y, vColor.z );
#endif
}

void DumpLineToGLView( const Vector &vPoint1, const Vector &vColor1, const Vector &vPoint2, const Vector &vColor2, float fThickness, FILE *pFile )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	Vector vDirection = vPoint2 - vPoint1;
	vDirection.NormalizeInPlace();

	Vector vPseudoPerpandicular = vec3_origin;

	if( vDirection.x != 0.0f )
		vPseudoPerpandicular.z = 1.0f;
	else
		vPseudoPerpandicular.x = 1.0f;

	Vector vWidth = vDirection.Cross( vPseudoPerpandicular );
	vWidth.NormalizeInPlace();

	Vector vHeight = vDirection.Cross( vWidth );
	vHeight.NormalizeInPlace();

	fThickness *= 0.5f; //we use half thickness in both directions
	vDirection *= fThickness;
	vWidth *= fThickness;
	vHeight *= fThickness;

	Vector vLinePoints[8];
	vLinePoints[0] = vPoint1 - vDirection - vWidth - vHeight;
	vLinePoints[1] = vPoint1 - vDirection - vWidth + vHeight;
	vLinePoints[2] = vPoint1 - vDirection + vWidth - vHeight;
	vLinePoints[3] = vPoint1 - vDirection + vWidth + vHeight;

	vLinePoints[4] = vPoint2 + vDirection - vWidth - vHeight;
	vLinePoints[5] = vPoint2 + vDirection - vWidth + vHeight;
	vLinePoints[6] = vPoint2 + vDirection + vWidth - vHeight;
	vLinePoints[7] = vPoint2 + vDirection + vWidth + vHeight;

	const Vector *pLineColors[8] = { &vColor1, &vColor1, &vColor1, &vColor1, &vColor2, &vColor2, &vColor2, &vColor2 };


#define DPTGLV_LINE_WRITEPOINT(index) fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vLinePoints[index].x, vLinePoints[index].y, vLinePoints[index].z, pLineColors[index]->x, pLineColors[index]->y, pLineColors[index]->z );
#define DPTGLV_LINE_DOUBLESIDEDQUAD(index1,index2,index3,index4)\
	fprintf( pFile, "4\n" );\
	DPTGLV_LINE_WRITEPOINT(index1);\
	DPTGLV_LINE_WRITEPOINT(index2);\
	DPTGLV_LINE_WRITEPOINT(index3);\
	DPTGLV_LINE_WRITEPOINT(index4);\
	fprintf( pFile, "4\n" );\
	DPTGLV_LINE_WRITEPOINT(index4);\
	DPTGLV_LINE_WRITEPOINT(index3);\
	DPTGLV_LINE_WRITEPOINT(index2);\
	DPTGLV_LINE_WRITEPOINT(index1);


	DPTGLV_LINE_DOUBLESIDEDQUAD(0,4,6,2);
	DPTGLV_LINE_DOUBLESIDEDQUAD(3,7,5,1);
	DPTGLV_LINE_DOUBLESIDEDQUAD(1,5,4,0);
	DPTGLV_LINE_DOUBLESIDEDQUAD(2,6,7,3);
	DPTGLV_LINE_DOUBLESIDEDQUAD(0,2,3,1);
	DPTGLV_LINE_DOUBLESIDEDQUAD(5,7,6,4);
#endif
}

void DumpPolyhedronToGLView( const CPolyhedron *pPolyhedron, const char *pFilename, const VMatrix *pTransform )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	if ( (pPolyhedron == NULL) || (pPolyhedron->iVertexCount == 0) )
		return;

	if( pTransform == NULL )
		pTransform = &s_matIdentity;

	printf("Writing %s...\n", pFilename );

	FILE *pFile = fopen( pFilename, "ab" );

	//randomizing an array of colors to help spot shared/unshared vertices
	Vector *pColors = (Vector *)stackalloc( sizeof( Vector ) * pPolyhedron->iVertexCount );	
	int counter;
	for( counter = 0; counter != pPolyhedron->iVertexCount; ++counter )
	{
		pColors[counter].Init( rand()/32768.0f, rand()/32768.0f, rand()/32768.0f );
	}

	Vector *pTransformedPoints = (Vector *)stackalloc( pPolyhedron->iVertexCount * sizeof( Vector ) );
	for ( counter = 0; counter != pPolyhedron->iVertexCount; ++counter )
	{
		pTransformedPoints[counter] = (*pTransform) * pPolyhedron->pVertices[counter];
	}

	for ( counter = 0; counter != pPolyhedron->iPolygonCount; ++counter )
	{
		fprintf( pFile, "%i\n", pPolyhedron->pPolygons[counter].iIndexCount );
		int counter2;
		for( counter2 = 0; counter2 != pPolyhedron->pPolygons[counter].iIndexCount; ++counter2 )
		{
			Polyhedron_IndexedLineReference_t *pLineReference = &pPolyhedron->pIndices[pPolyhedron->pPolygons[counter].iFirstIndex + counter2];

			Vector *pVertex = &pTransformedPoints[pPolyhedron->pLines[pLineReference->iLineIndex].iPointIndices[pLineReference->iEndPointIndex]];
			Vector *pColor = &pColors[pPolyhedron->pLines[pLineReference->iLineIndex].iPointIndices[pLineReference->iEndPointIndex]];
			fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n",pVertex->x, pVertex->y, pVertex->z, pColor->x, pColor->y, pColor->z );
		}
	}

	for( counter = 0; counter != pPolyhedron->iLineCount; ++counter )
	{
		const Vector vOne( 1.0f, 1.0f, 1.0f );
		DumpLineToGLView( pTransformedPoints[pPolyhedron->pLines[counter].iPointIndices[0]], vOne - pColors[pPolyhedron->pLines[counter].iPointIndices[0]],
							pTransformedPoints[pPolyhedron->pLines[counter].iPointIndices[1]], vOne - pColors[pPolyhedron->pLines[counter].iPointIndices[1]], 
							0.1f, pFile );
	}

	for( counter = 0; counter != pPolyhedron->iVertexCount; ++counter )
	{
		const Vector vPointHalfSize(0.15f, 0.15f, 0.15f );
		DumpAABBToGLView( pTransformedPoints[counter], vPointHalfSize, pColors[counter], pFile );
	}

	fclose( pFile );
#endif
}


void DumpPlaneToGlView( const float *pPlane, float fGrayScale, const char *pszFileName, const VMatrix *pTransform )
{
#ifdef ENABLE_DEBUG_POLYHEDRON_DUMPS
	if( pTransform == NULL )
		pTransform = &s_matIdentity;

	FILE *pFile = fopen( pszFileName, "ab" );

	//transform the plane
	Vector vNormal = pTransform->ApplyRotation( *(Vector *)pPlane );
	float fDist = pPlane[3] * vNormal.NormalizeInPlace(); //possible scaling going on
	fDist += vNormal.Dot( pTransform->GetTranslation() );
	
	Vector vPlaneVerts[4];

	PolyFromPlane( vPlaneVerts, vNormal, fDist, 100000.0f );

	fprintf( pFile, "4\n" );

	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[0].x, vPlaneVerts[0].y, vPlaneVerts[0].z, fGrayScale, fGrayScale, fGrayScale );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[1].x, vPlaneVerts[1].y, vPlaneVerts[1].z, fGrayScale, fGrayScale, fGrayScale );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[2].x, vPlaneVerts[2].y, vPlaneVerts[2].z, fGrayScale, fGrayScale, fGrayScale );
	fprintf( pFile, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[3].x, vPlaneVerts[3].y, vPlaneVerts[3].z, fGrayScale, fGrayScale, fGrayScale );

	fclose( pFile );
#endif
}
#endif


