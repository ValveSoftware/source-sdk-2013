//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include "mathlib/vector.h"
#include "bspfile.h"
#include "bsplib.h"
#include "cmdlib.h"
#include "physdll.h"
#include "utlvector.h"
#include "vbsp.h"
#include "phyfile.h"
#include <float.h>
#include "KeyValues.h"
#include "UtlBuffer.h"
#include "utlsymbol.h"
#include "utlrbtree.h"
#include "ivp.h"
#include "disp_ivp.h"
#include "materialpatch.h"
#include "bitvec.h"

// bit per leaf
typedef CBitVec<MAX_MAP_LEAFS> leafbitarray_t;

// parameters for conversion to vphysics
#define	NO_SHRINK			0.0f
// NOTE: vphysics maintains a minimum separation radius between objects
// This radius is set to 0.25, but it's symmetric.  So shrinking potentially moveable
// brushes by 0.5 in every direction ensures that these brushes can be constructed
// touching the world, and constrained in place without collisions or friction
// UNDONE: Add a key to disable this shrinking if necessary
#define VPHYSICS_SHRINK		(0.5f)	// shrink BSP brushes by this much for collision
#define VPHYSICS_MERGE		0.01f	// merge verts closer than this

void EmitPhysCollision();

IPhysicsCollision *physcollision = NULL;
extern IPhysicsSurfaceProps *physprops;

// a list of all of the materials in the world model
static CUtlVector<int> s_WorldPropList;

//-----------------------------------------------------------------------------
// Purpose: Write key/value pairs out to a memory buffer
//-----------------------------------------------------------------------------
CTextBuffer::CTextBuffer( void )
{
}
CTextBuffer::~CTextBuffer( void )
{
}

void CTextBuffer::WriteText( const char *pText )
{
	int len = strlen( pText );
	CopyData( pText, len );
}

void CTextBuffer::WriteIntKey( const char *pKeyName, int outputData )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"%d\"\n", pKeyName, outputData );
	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::WriteStringKey( const char *pKeyName, const char *outputData )
{
	CopyStringQuotes( pKeyName );
	CopyData( " ", 1 );
	CopyStringQuotes( outputData );
	CopyData( "\n", 1 );
}

void CTextBuffer::WriteFloatKey( const char *pKeyName, float outputData )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"%f\"\n", pKeyName, outputData );
	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::WriteFloatArrayKey( const char *pKeyName, const float *outputData, int count )
{
	char tmp[1024];
	
	// FAIL!
	if ( strlen(pKeyName) > 1000 )
	{
		Msg("Error writing collision data %s\n", pKeyName );
		return;
	}
	sprintf( tmp, "\"%s\" \"", pKeyName );
	for ( int i = 0; i < count; i++ )
	{
		char buf[80];

		sprintf( buf, "%f ", outputData[i] );
		strcat( tmp, buf );
	}
	strcat( tmp, "\"\n" );

	CopyData( tmp, strlen(tmp) );
}

void CTextBuffer::CopyStringQuotes( const char *pString )
{
	CopyData( "\"", 1 );
	CopyData( pString, strlen(pString) );
	CopyData( "\"", 1 );
}

void CTextBuffer::Terminate( void )
{
	CopyData( "\0", 1 );
}

void CTextBuffer::CopyData( const char *pData, int len )
{
	int offset = m_buffer.AddMultipleToTail( len );
	memcpy( m_buffer.Base() + offset, pData, len );
}


//-----------------------------------------------------------------------------
// Purpose: Writes a glview text file containing the collision surface in question
// Input  : *pCollide - 
//			*pFilename - 
//-----------------------------------------------------------------------------
void DumpCollideToGlView( CPhysCollide *pCollide, const char *pFilename )
{
	if ( !pCollide )
		return;

	Msg("Writing %s...\n", pFilename );
	Vector *outVerts;
	int vertCount = physcollision->CreateDebugMesh( pCollide, &outVerts );
	FILE *fp = fopen( pFilename, "w" );
	int triCount = vertCount / 3;
	int vert = 0;
	for ( int i = 0; i < triCount; i++ )
	{
		fprintf( fp, "3\n" );
		fprintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		fprintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		fprintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
	}
	fclose( fp );
	physcollision->DestroyDebugMesh( vertCount, outVerts );
}


void DumpCollideToPHY( CPhysCollide *pCollide, CTextBuffer *text,   const char *pFilename )
{
	Msg("Writing %s...\n", pFilename );
	FILE *fp = fopen( pFilename, "wb" );
	phyheader_t header;
	header.size = sizeof(header);
	header.id = 0;
	header.checkSum = 0;
	header.solidCount = 1;
	fwrite( &header, sizeof(header), 1, fp );
	int size = physcollision->CollideSize( pCollide );
	fwrite( &size, sizeof(int), 1, fp );

	char *buf = (char *)malloc( size );
	physcollision->CollideWrite( buf, pCollide );
	fwrite( buf, size, 1, fp );

	fwrite( text->GetData(), text->GetSize(), 1, fp );
	fclose( fp );
	free( buf );
}

CPhysCollisionEntry::CPhysCollisionEntry( CPhysCollide *pCollide )
{
	m_pCollide = pCollide;
}

unsigned int CPhysCollisionEntry::GetCollisionBinarySize() 
{ 
	return physcollision->CollideSize( m_pCollide ); 
}

unsigned int CPhysCollisionEntry::WriteCollisionBinary( char *pDest ) 
{ 
	return physcollision->CollideWrite( pDest, m_pCollide );
}

void CPhysCollisionEntry::DumpCollideFileName( const char *pName, int modelIndex, CTextBuffer *pTextBuffer )
{
	char tmp[128];
	sprintf( tmp, "%s%03d.phy", pName, modelIndex );
	DumpCollideToPHY( m_pCollide, pTextBuffer, tmp );
	sprintf( tmp, "%s%03d.txt", pName, modelIndex );
	DumpCollideToGlView( m_pCollide, tmp );
}


class CPhysCollisionEntrySolid : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntrySolid( CPhysCollide *pCollide, const char *pMaterialName, float mass );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	float		m_volume;
	float		m_mass;
	const char *m_pMaterial;
};


CPhysCollisionEntrySolid::CPhysCollisionEntrySolid( CPhysCollide *pCollide, const char *pMaterialName, float mass )
	: CPhysCollisionEntry( pCollide )
{
	m_volume = physcollision->CollideVolume( m_pCollide );
	m_mass = mass;
	m_pMaterial = pMaterialName;
}

void CPhysCollisionEntrySolid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	DumpCollideFileName( "collide", modelIndex, pTextBuffer );
}

void CPhysCollisionEntrySolid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "solid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteFloatKey( "mass", m_mass );
	if ( m_pMaterial )
	{
		pTextBuffer->WriteStringKey( "surfaceprop", m_pMaterial );
	}
	if ( m_volume != 0.f )
	{
		pTextBuffer->WriteFloatKey( "volume", m_volume );
	}
	pTextBuffer->WriteText( "}\n" );
}


class CPhysCollisionEntryStaticSolid : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntryStaticSolid ( CPhysCollide *pCollide, int contentsMask );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	int		m_contentsMask;
};


CPhysCollisionEntryStaticSolid ::CPhysCollisionEntryStaticSolid ( CPhysCollide *pCollide, int contentsMask )
	: CPhysCollisionEntry( pCollide ), m_contentsMask(contentsMask)
{
}

void CPhysCollisionEntryStaticSolid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "static%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryStaticSolid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "staticsolid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteIntKey( "contents", m_contentsMask );
	pTextBuffer->WriteText( "}\n" );
}

CPhysCollisionEntryStaticMesh::CPhysCollisionEntryStaticMesh( CPhysCollide *pCollide, const char *pMaterialName )
	: CPhysCollisionEntry( pCollide )
{
	m_pMaterial = pMaterialName;
}

void CPhysCollisionEntryStaticMesh::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "mesh%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryStaticMesh::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "staticsolid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteText( "}\n" );
}

class CPhysCollisionEntryFluid : public CPhysCollisionEntry
{
public:
	~CPhysCollisionEntryFluid();
	CPhysCollisionEntryFluid( CPhysCollide *pCollide, const char *pSurfaceProp, float damping, const Vector &normal, float dist, int nContents );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	char	*m_pSurfaceProp;
	float	m_damping;
	Vector 	m_surfaceNormal;
	float	m_surfaceDist;
	int		m_contentsMask;
};


CPhysCollisionEntryFluid::CPhysCollisionEntryFluid( CPhysCollide *pCollide, const char *pSurfaceProp, float damping, const Vector &normal, float dist, int nContents )
	: CPhysCollisionEntry( pCollide )
{
	m_surfaceNormal = normal;
	m_surfaceDist = dist;
	m_pSurfaceProp = new char[strlen(pSurfaceProp)+1];
	strcpy( m_pSurfaceProp, pSurfaceProp );
	m_damping = damping;
	m_contentsMask = nContents;
}

CPhysCollisionEntryFluid::~CPhysCollisionEntryFluid()
{
	delete[] m_pSurfaceProp;
}

void CPhysCollisionEntryFluid::DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	char tmp[128];
	sprintf( tmp, "water%02d", modelIndex );
	DumpCollideFileName( tmp, collideIndex, pTextBuffer );
}

void CPhysCollisionEntryFluid::WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex )
{
	pTextBuffer->WriteText( "fluid {\n" );
	pTextBuffer->WriteIntKey( "index", collideIndex );
	pTextBuffer->WriteStringKey( "surfaceprop", m_pSurfaceProp );		// write out water material
  	pTextBuffer->WriteFloatKey( "damping", m_damping );		// write out water damping
  	pTextBuffer->WriteIntKey( "contents", m_contentsMask );		// write out water contents
	float array[4];
	m_surfaceNormal.CopyToArray( array );
	array[3] = m_surfaceDist;
	pTextBuffer->WriteFloatArrayKey( "surfaceplane", array, 4 );		// write out water surface plane
	pTextBuffer->WriteFloatArrayKey( "currentvelocity", vec3_origin.Base(), 3 );		// write out water velocity
	pTextBuffer->WriteText( "}\n" );
}

// Get an index into the prop list of this prop (add it if necessary)
static int PropIndex( CUtlVector<int> &propList, int propIndex )
{
	for ( int i = 0; i < propList.Count(); i++ )
	{
		if ( propList[i] == propIndex )
			return i+1;
	}

	if ( propList.Count() < 126 )
	{
		return propList.AddToTail( propIndex )+1;
	}

	return 0;
}

int RemapWorldMaterial( int materialIndexIn )
{
	return PropIndex( s_WorldPropList, materialIndexIn );
}

typedef struct
{
	float normal[3];
	float dist;
} listplane_t;

static void AddListPlane( CUtlVector<listplane_t> *list, float x, float y, float z, float d )
{
	listplane_t plane;
	plane.normal[0] = x;
	plane.normal[1] = y;
	plane.normal[2] = z;
	plane.dist = d;

	list->AddToTail( plane );
}

class CPlaneList
{
public:

	CPlaneList( float shrink, float merge );
	~CPlaneList( void );

	void AddConvex( CPhysConvex *pConvex );

	// add the brushes to the model
	int AddBrushes( void );

	// Adds a single brush as a convex object
	void ReferenceBrush( int brushnumber );
	bool IsBrushReferenced( int brushnumber );

	void ReferenceLeaf( int leafIndex );
	bool IsLeafReferenced( int leafIndex );
	int GetFirstBrushSide();

private:

	CPhysConvex *BuildConvexForBrush( int brushnumber, float shrink, CPhysCollide *pCollideTest, float shrinkMinimum );

public:
	CUtlVector<CPhysConvex *>	m_convex;

	CUtlVector<int>				m_leafList;
	int							m_contentsMask;

	float						m_shrink;
	float						m_merge;
	bool						*m_brushAdded;
	float						m_totalVolume;
};

CPlaneList::CPlaneList( float shrink, float merge )
{
	m_shrink = shrink;
	m_merge = merge;
	m_contentsMask = MASK_SOLID;
	m_brushAdded = new bool[numbrushes];
	memset( m_brushAdded, 0, sizeof(bool) * numbrushes );
	m_totalVolume = 0;
	m_leafList.Purge();
}


CPlaneList::~CPlaneList( void )
{
	delete[] m_brushAdded;
}


void CPlaneList::AddConvex( CPhysConvex *pConvex )
{
	if ( pConvex )
	{
		m_totalVolume += physcollision->ConvexVolume( pConvex );
		m_convex.AddToTail( pConvex );
	}
}

// Adds a single brush as a convex object
void CPlaneList::ReferenceBrush( int brushnumber )
{
	if ( !(dbrushes[brushnumber].contents & m_contentsMask) )
		return;

	m_brushAdded[brushnumber] = true;

}


bool CPlaneList::IsBrushReferenced( int brushnumber )
{
	return m_brushAdded[brushnumber];
}

CPhysConvex *CPlaneList::BuildConvexForBrush( int brushnumber, float shrink, CPhysCollide *pCollideTest, float shrinkMinimum )
{
	CUtlVector<listplane_t> temp( 0, 32 );

	for ( int i = 0; i < dbrushes[brushnumber].numsides; i++ )
	{
		dbrushside_t *pside = dbrushsides + i + dbrushes[brushnumber].firstside;
		if ( pside->bevel )
			continue;

		dplane_t *pplane = dplanes + pside->planenum;
		float shrinkThisPlane = shrink;

		if ( i < g_MainMap->mapbrushes[brushnumber].numsides )
		{
			if ( !g_MainMap->mapbrushes[brushnumber].original_sides[i].visible )
			{
				// don't shrink brush sides with no visible components.
				// this produces something closer to the ideal shrink than simply shrinking all planes
				shrinkThisPlane = 0;
			}
		}
		// Make sure shrinking won't swallow geometry along this axis.
		if ( pCollideTest && shrinkThisPlane != 0 )
		{
			Vector start = physcollision->CollideGetExtent( pCollideTest, vec3_origin, vec3_angle, pplane->normal );
			Vector end = physcollision->CollideGetExtent( pCollideTest, vec3_origin, vec3_angle, -pplane->normal );
			float thick = DotProduct( (end-start), pplane->normal );
			// NOTE: The object must be at least "shrinkMinimum" inches wide on each axis
			if ( fabs(thick) < shrinkMinimum )
			{
#if _DEBUG
				Warning("Can't shrink brush %d, plane %d (%.2f, %.2f, %.2f)\n", brushnumber, pside->planenum, pplane->normal[0], pplane->normal[1], pplane->normal[2] );
#endif
				shrinkThisPlane = 0;
			}
		}
		AddListPlane( &temp, pplane->normal[0], pplane->normal[1], pplane->normal[2], pplane->dist - shrinkThisPlane );
	}
	return physcollision->ConvexFromPlanes( (float *)temp.Base(), temp.Count(), m_merge );
}

int CPlaneList::AddBrushes( void )
{
	int count = 0;
	for ( int brushnumber = 0; brushnumber < numbrushes; brushnumber++ )
	{
		if ( IsBrushReferenced(brushnumber) )
		{
			CPhysConvex *pBrushConvex = NULL;
			if ( m_shrink != 0 )
			{
				// Make sure shrinking won't swallow this brush.
				CPhysConvex *pConvex = BuildConvexForBrush( brushnumber, 0, NULL, 0 );
				CPhysCollide *pUnshrunkCollide = physcollision->ConvertConvexToCollide( &pConvex, 1 );
				pBrushConvex = BuildConvexForBrush( brushnumber, m_shrink, pUnshrunkCollide, m_shrink * 3 );
				physcollision->DestroyCollide( pUnshrunkCollide );
			}
			else
			{
				pBrushConvex = BuildConvexForBrush( brushnumber, m_shrink, NULL, 1.0 );
			}

			if ( pBrushConvex )
			{
				count++;
				physcollision->SetConvexGameData( pBrushConvex, brushnumber );
				AddConvex( pBrushConvex );
			}
		}
	}
	return count;
}


int CPlaneList::GetFirstBrushSide()
{
	for ( int brushnumber = 0; brushnumber < numbrushes; brushnumber++ )
	{
		if ( IsBrushReferenced(brushnumber) )
		{
			for ( int i = 0; i < dbrushes[brushnumber].numsides; i++ )
			{
				int sideIndex = i + dbrushes[brushnumber].firstside;
				dbrushside_t *pside = dbrushsides + sideIndex;
				if ( pside->bevel )
					continue;
				return sideIndex;
			}
		}
	}
	return 0;
}

// UNDONE: Try using this kind of algorithm if we run into precision problems.  
// NOTE: ConvexFromPlanes will be doing a bunch of matrix inversions that can suffer
// if plane normals are too close to each other...
#if 0
void CPlaneList::AddBrushes( void )
{
	CUtlVector<listplane_t> temp;
	for ( int brushnumber = 0; brushnumber < numbrushes; brushnumber++ )
	{
		if ( IsBrushReferenced(brushnumber) )
		{
			CUtlVector<winding_t *> windings;

			for ( int i = 0; i < dbrushes[brushnumber].numsides; i++ )
			{
				dbrushside_t *pside = dbrushsides + i + dbrushes[brushnumber].firstside;
				if (pside->bevel)
					continue;
				dplane_t *pplane = dplanes + pside->planenum;
				winding_t *w = BaseWindingForPlane( pplane->normal, pplane->dist - m_shrink );
				for ( int j = 0; j < dbrushes[brushnumber].numsides && w; j++ )
				{
					if (i == j)
						continue;
					dbrushside_t *pClipSide = dbrushsides + j + dbrushes[brushnumber].firstside;
					if (pClipSide->bevel)
						continue;
					dplane_t *pClipPlane = dplanes + pClipSide->planenum;
					ChopWindingInPlace (&w, -pClipPlane->normal, -pClipPlane->dist+m_shrink, 0); //CLIP_EPSILON);
				}
				if ( w )
				{
					windings.AddToTail( w );
				}
			}

			CUtlVector<Vector *> vertList;
			for ( int p = 0; p < windings.Count(); p++ )
			{
				for ( int v = 0; v < windings[p]->numpoints; v++ )
				{
					vertList.AddToTail( windings[p]->p + v );
				}
			}
			CPhysConvex *pConvex = physcollision->ConvexFromVerts( vertList.Base(), vertList.Count() );
			if ( pConvex )
			{
				physcollision->SetConvexGameData( pConvex, brushnumber );
				AddConvex( pConvex );
			}
			temp.RemoveAll();
		}
	}
}
#endif

// If I have a list of leaves, make sure this leaf is in it.
// Otherwise, process all leaves
bool CPlaneList::IsLeafReferenced( int leafIndex )
{
	if ( !m_leafList.Count() )
		return true;

	for ( int i = 0; i < m_leafList.Count(); i++ )
	{
		if ( m_leafList[i] == leafIndex )
			return true;
	}

	return false;
}

// Add a leaf to my list of interesting leaves
void CPlaneList::ReferenceLeaf( int leafIndex )
{
	m_leafList.AddToTail( leafIndex );
}

static void VisitLeaves_r( CPlaneList &planes, int node )
{
	if ( node < 0 )
	{
		int leafIndex = -1 - node;
		if ( planes.IsLeafReferenced(leafIndex) )
		{
			int i;

			// Add the solids in the "empty" leaf
			for ( i = 0; i < dleafs[leafIndex].numleafbrushes; i++ )
			{
				int brushIndex = dleafbrushes[dleafs[leafIndex].firstleafbrush + i];
				planes.ReferenceBrush( brushIndex );
			}
		}
	}
	else
	{
		dnode_t *pnode = dnodes + node;

		VisitLeaves_r( planes, pnode->children[0] );
		VisitLeaves_r( planes, pnode->children[1] );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct waterleaf_t
{
	Vector	surfaceNormal;
	float	surfaceDist;
	float	minZ;
	bool	hasSurface;
	int		waterLeafIndex;// this is the submerged leaf
	int		planenum;	//UNDONE: REMOVE
	int		surfaceTexInfo; // if hasSurface == true, this is the texinfo index for the water material
	int		outsideLeafIndex;// this is the leaf on the other side of the water surface
	node_t	*pNode;
};



// returns true if newleaf should appear before currentleaf in the list
static bool IsLowerLeaf( const waterleaf_t &newleaf, const waterleaf_t &currentleaf )
{
	if ( newleaf.hasSurface && currentleaf.hasSurface )
	{
		// the one with the upmost pointing z goes first
		if ( currentleaf.surfaceNormal.z > newleaf.surfaceNormal.z )
			return false;

		if ( fabs(currentleaf.surfaceNormal.z - newleaf.surfaceNormal.z) < 0.01 )
		{
			if ( newleaf.surfaceDist < currentleaf.surfaceDist )
				return true;
		}
		return true;
	}
	else if ( newleaf.hasSurface )		// the leaf with a surface always goes first
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:   Water surfaces are stored in an RB tree and the tree is used to 
//  create one-off .vmt files embedded in the .bsp for each surface so that the
//  water depth effect occurs on a per-water surface level.
//-----------------------------------------------------------------------------
struct WaterTexInfo
{
	// The mangled new .vmt name ( materials/levelename/oldmaterial_depth_xxx ) where xxx is
	//  the water depth (as an integer )
	CUtlSymbol  m_FullName;

	// The original .vmt name
	CUtlSymbol	m_MaterialName;

	// The depth of the water this texinfo refers to
	int			m_nWaterDepth;

	// The texinfo id
	int			m_nTexInfo;

	// The subdivision size for the water surface
//	float		m_SubdivSize;
};

//-----------------------------------------------------------------------------
// Purpose: Helper for RB tree operations ( we compare full mangled names )
// Input  : src1 - 
//			src2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WaterLessFunc( WaterTexInfo const& src1, WaterTexInfo const& src2 )
{
	return src1.m_FullName < src2.m_FullName;
}

//-----------------------------------------------------------------------------
// Purpose: A growable RB tree of water surfaces
//-----------------------------------------------------------------------------
static CUtlRBTree< WaterTexInfo, int > g_WaterTexInfos( 0, 32, WaterLessFunc );

#if 0
float GetSubdivSizeForFogVolume( int fogVolumeID )
{
	Assert( fogVolumeID >= 0 && fogVolumeID < g_WaterTexInfos.Count() );
	return g_WaterTexInfos[fogVolumeID].m_SubdivSize;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mapname - 
//			*materialname - 
//			waterdepth - 
//			*fullname - 
//-----------------------------------------------------------------------------
void GetWaterTextureName( char const *mapname, char const *materialname, int waterdepth, char *fullname  )
{
	char temp[ 512 ];

	// Construct the full name (prepend mapname to reduce name collisions)
	sprintf( temp, "maps/%s/%s_depth_%i", mapname, materialname, (int)waterdepth );

	// Make sure it's lower case
	strlwr( temp );

	strcpy( fullname, temp );
}

//-----------------------------------------------------------------------------
// Purpose: Called to write procedural materials in the rb tree to the embedded
//  pak file for this .bsp
//-----------------------------------------------------------------------------
void EmitWaterMaterialFile( WaterTexInfo *wti )
{
	char waterTextureName[512];
	if ( !wti )
	{
		return;
	}

	GetWaterTextureName( mapbase, wti->m_MaterialName.String(), ( int )wti->m_nWaterDepth, waterTextureName );
	
	// Convert to string
	char szDepth[ 32 ];
	sprintf( szDepth, "%i", wti->m_nWaterDepth );
	CreateMaterialPatch( wti->m_MaterialName.String(), waterTextureName, "$waterdepth", szDepth, PATCH_INSERT );
}

//-----------------------------------------------------------------------------
// Purpose: Takes the texinfo_t referenced by the .vmt and the computed depth for the
//  surface and looks up or creates a texdata/texinfo for the mangled one-off water .vmt file
// Input  : *pBaseInfo - 
//			depth - 
// Output : int
//-----------------------------------------------------------------------------
int FindOrCreateWaterTexInfo( texinfo_t *pBaseInfo, float depth )
{
	char fullname[ 512 ];
	char materialname[ 512 ];

	// Get the base texture/material name
	char const *name = TexDataStringTable_GetString( GetTexData( pBaseInfo->texdata )->nameStringTableID );

	GetWaterTextureName( mapbase, name, (int)depth, fullname );

	// See if we already have an entry for this depth
	WaterTexInfo lookup;
	lookup.m_FullName = fullname;
	int idx = g_WaterTexInfos.Find( lookup );

	// If so, return the existing entry texinfo index
	if ( idx != g_WaterTexInfos.InvalidIndex() )
	{
		return g_WaterTexInfos[ idx ].m_nTexInfo;
	}

	// Otherwise, fill in the rest of the data
	lookup.m_nWaterDepth = (int)depth;
	// Remember the current material name
	sprintf( materialname, "%s", name );
	strlwr( materialname );
	lookup.m_MaterialName = materialname;

	texinfo_t ti;
	// Make a copy
	ti = *pBaseInfo;
	// Create a texdata that is based on the underlying existing entry
	ti.texdata = FindAliasedTexData( fullname, GetTexData( pBaseInfo->texdata ) );

	// Find or create a new index
	lookup.m_nTexInfo = FindOrCreateTexInfo( ti );

	// Add the new texinfo to the RB tree
	idx = g_WaterTexInfos.Insert( lookup );

	// Msg( "created texinfo for %s\n", lookup.m_FullName.String() );

	// Go ahead and create the new vmt file.
	EmitWaterMaterialFile( &g_WaterTexInfos[idx] );

	// Return the new texinfo
	return g_WaterTexInfos[ idx ].m_nTexInfo;
}

extern node_t *dfacenodes[MAX_MAP_FACES];
static void WriteFogVolumeIDs( dmodel_t *pModel )
{
	int i;
	
	// write fog volume ID to each face in this model
	for( i = pModel->firstface; i < pModel->firstface + pModel->numfaces; i++ )
	{
		dface_t *pFace = &dfaces[i];
		node_t *pFaceNode = dfacenodes[i];
		texinfo_t *pTexInfo = &texinfo[pFace->texinfo];
		pFace->surfaceFogVolumeID = -1;
		if ( pFaceNode )
		{
			if ( (pTexInfo->flags & SURF_WARP ) && pFaceNode->planenum == PLANENUM_LEAF && pFaceNode->diskId >= 0 )
			{
				pFace->surfaceFogVolumeID = dleafs[pFaceNode->diskId].leafWaterDataID;
				dleafwaterdata_t *pLeafWaterData = &dleafwaterdata[pFace->surfaceFogVolumeID];
				
				// HACKHACK: Should probably mark these faces as water bottom or "bottommaterial" faces.
				// HACKHACK: Use a heuristic, if it points up, it's the water top.
				if ( dplanes[pFace->planenum].normal.z > 0 )
				{
					pFace->texinfo = pLeafWaterData->surfaceTexInfoID;
				}
			}
			else
			{
				// missed this face somehow?
				Assert( !(pTexInfo->flags & SURF_WARP ) );
			}

		}
	}
}


static bool PortalCrossesWater( waterleaf_t &baseleaf, portal_t *portal )
{
	if ( baseleaf.hasSurface )
	{
		int side = WindingOnPlaneSide( portal->winding, baseleaf.surfaceNormal, baseleaf.surfaceDist );
		if ( side == SIDE_CROSS || side == SIDE_FRONT )
			return true;
	}

	return false;
}


static int FindOrCreateLeafWaterData( float surfaceZ, float minZ, int surfaceTexInfoID )
{
	int i;
	for( i = 0; i < numleafwaterdata; i++ )
	{
		dleafwaterdata_t *pLeafWaterData = &dleafwaterdata[i];
		if( pLeafWaterData->surfaceZ == surfaceZ &&
			pLeafWaterData->minZ == minZ &&
			pLeafWaterData->surfaceTexInfoID == surfaceTexInfoID )
		{
			return i;
		}
	}
	dleafwaterdata_t *pLeafWaterData = &dleafwaterdata[numleafwaterdata];
	pLeafWaterData->surfaceZ = surfaceZ;
	pLeafWaterData->minZ = minZ;
	pLeafWaterData->surfaceTexInfoID = surfaceTexInfoID;
	numleafwaterdata++;
	return numleafwaterdata - 1;
}


// Enumerate all leaves under node with contents in contentsMask and add them to list
void EnumLeaves_r( CUtlVector<node_t *> &list, node_t *node, int contentsMask )
{
	if ( node->planenum != PLANENUM_LEAF )
	{
		EnumLeaves_r( list, node->children[0], contentsMask );
		EnumLeaves_r( list, node->children[1], contentsMask );
		return;
	}

	if ( !(node->contents & contentsMask) )
		return;


	// has the contents, put it in the list
	list.AddToTail( node );
}


// Builds a waterleaf_t for the given leaf
static void BuildWaterLeaf( node_t *pLeafIn, waterleaf_t &waterLeafOut )
{
	waterLeafOut.pNode = pLeafIn;
	waterLeafOut.waterLeafIndex = pLeafIn->diskId;
	waterLeafOut.outsideLeafIndex = -1;
	waterLeafOut.hasSurface = false;
	waterLeafOut.surfaceDist = MAX_COORD_INTEGER;
	waterLeafOut.surfaceNormal.Init( 0.f, 0.f, 1.f );
	waterLeafOut.planenum = -1;
	waterLeafOut.surfaceTexInfo = -1;
	waterLeafOut.minZ = MAX_COORD_INTEGER;

	// search the list of portals out of this leaf for one that leaves water
	// If you find one, this leaf has a surface, so fill out the surface data
	int oppositeNodeIndex = 0;
	for (portal_t *p = pLeafIn->portals ; p ; p = p->next[!oppositeNodeIndex])
	{
		oppositeNodeIndex = (p->nodes[0] == pLeafIn) ? 1 : 0;

		// not visible, can't be the portals we're looking for...
		if ( !p->side )
			continue;

		// See if this portal crosses into air
		node_t *pOpposite = p->nodes[oppositeNodeIndex];
		if ( !(pOpposite->contents & MASK_WATER) && !(pOpposite->contents & MASK_SOLID) )
		{
			// it does, there must be a surface here
			plane_t *plane = &g_MainMap->mapplanes[p->side->planenum];
			if ( waterLeafOut.hasSurface )
			{
				// Sort to find the most upward facing normal (skips sides)
				if ( waterLeafOut.surfaceNormal.z > plane->normal.z )
					continue;
				if ( (waterLeafOut.surfaceNormal.z == plane->normal.z) && waterLeafOut.surfaceDist >= plane->dist )
					continue;
			}
			// water surface needs to point at least somewhat up, this is 
			// probably a map error
			if ( plane->normal.z <= 0 )
				continue;
			waterLeafOut.surfaceDist = plane->dist;
			waterLeafOut.surfaceNormal = plane->normal;
			waterLeafOut.hasSurface = true;
			waterLeafOut.outsideLeafIndex = p->nodes[oppositeNodeIndex]->diskId;
			waterLeafOut.surfaceTexInfo = p->side->texinfo;
		}
	}
}


static void InsertSortWaterLeaf( CUtlVector<waterleaf_t> &list, const waterleaf_t &leafInsert )
{
	// insertion sort the leaf (lowest leaves go first)
	// leaves that aren't actually on the surface of the water will have leaf.hasSurface == false.
	for ( int i = 0; i < list.Count(); i++ )
	{
		if ( IsLowerLeaf( leafInsert, list[i] ) )
		{
			list.InsertBefore( i, leafInsert );
			return;
		}
	}

	// must the highest one, so stick it at the end.
	list.AddToTail( leafInsert );
}


// Flood fill the tree, finding neighboring water volumes and connecting them to this list
// Cut groups that try to cross the surface.
// Mark leaves that are in a group as "visited" so they won't be chosen by subsequent fills
static void Flood_FindConnectedWaterVolumes_r( CUtlVector<node_t *> &list, node_t *pLeaf, waterleaf_t &baseleaf, leafbitarray_t &visited )
{
	// already visited, or not the same water contents
	if ( pLeaf->diskId < 0 || visited.Get(pLeaf->diskId) || !(pLeaf->contents & (baseleaf.pNode->contents & MASK_WATER) ) )
		return;

	int oppositeNodeIndex = 0;
	for (portal_t *p = pLeaf->portals ; p ; p = p->next[!oppositeNodeIndex])
	{
		oppositeNodeIndex = (p->nodes[0] == pLeaf) ? 1 : 0;

		// If any portal crosses the water surface, don't flow through this leaf
		if ( PortalCrossesWater( baseleaf, p ) )
			return;
	}

	visited.Set( pLeaf->diskId );
	list.AddToTail( pLeaf );

	baseleaf.minZ = min( pLeaf->mins.z, baseleaf.minZ );

	for (portal_t *p = pLeaf->portals ; p ; p = p->next[!oppositeNodeIndex])
	{
		oppositeNodeIndex = (p->nodes[0] == pLeaf) ? 1 : 0;

		Flood_FindConnectedWaterVolumes_r( list, p->nodes[oppositeNodeIndex], baseleaf, visited );
	}
}

// UNDONE: This is a bit of a hack to avoid crashing when we can't find an
// appropriate texinfo for a water model (to get physics properties)
int FirstWaterTexinfo( bspbrush_t *brushlist, int contents )
{
	while (brushlist)
	{
		if ( brushlist->original->contents & contents )
		{
			for ( int i = 0; i < brushlist->original->numsides; i++ )
			{
				if ( brushlist->original->original_sides[i].contents & contents )
				{
					return brushlist->original->original_sides[i].texinfo;
				}
			}
		}
		brushlist = brushlist->next;
	}

	Assert(0);
	return 0;
}

// This is a list of water data that will be turned into physics models 
struct watermodel_t
{
	int	modelIndex;
	int	contents;
	waterleaf_t waterLeafData;
	int depthTexinfo;
	int firstWaterLeafIndex;
	int	waterLeafCount;
	int	fogVolumeIndex;
};

static CUtlVector<watermodel_t>	g_WaterModels;
static CUtlVector<int>			g_WaterLeafList;

// Creates a list of watermodel_t for later processing by EmitPhysCollision
void EmitWaterVolumesForBSP( dmodel_t *pModel, node_t *node )
{
	CUtlVector<node_t *> leafListAnyWater;
	// build the list of all leaves containing water
	EnumLeaves_r( leafListAnyWater, node, MASK_WATER );

	// make a sorted list to flood fill
	CUtlVector<waterleaf_t>	list;
	
	int i;
	for ( i = 0; i < leafListAnyWater.Count(); i++ )
	{
		waterleaf_t waterLeaf;
		BuildWaterLeaf( leafListAnyWater[i], waterLeaf );
		InsertSortWaterLeaf( list, waterLeaf );
	}

	leafbitarray_t visited;
	CUtlVector<node_t *> waterAreaList;
	for ( i = 0; i < list.Count(); i++ )
	{
		Flood_FindConnectedWaterVolumes_r( waterAreaList, list[i].pNode, list[i], visited );

		// did we find a list of leaves connected to this one?
		// remember the list is sorted, so this one may have been attached to a previous
		// leaf.  So it could have nothing hanging off of it.
		if ( waterAreaList.Count() )
		{
			// yes, emit a watermodel
			watermodel_t tmp;
			tmp.modelIndex = nummodels;
			tmp.contents = list[i].pNode->contents;
			tmp.waterLeafData = list[i];
			tmp.firstWaterLeafIndex = g_WaterLeafList.Count();
			tmp.waterLeafCount = waterAreaList.Count();
			
			float waterDepth = tmp.waterLeafData.surfaceDist - tmp.waterLeafData.minZ;
			if ( tmp.waterLeafData.surfaceTexInfo < 0 )
			{
				// the map has probably leaked in this case, but output something anyway.
				Assert(list[i].pNode->planenum == PLANENUM_LEAF);
				tmp.waterLeafData.surfaceTexInfo = FirstWaterTexinfo( list[i].pNode->brushlist, tmp.contents );
			}
			tmp.depthTexinfo = FindOrCreateWaterTexInfo( &texinfo[ tmp.waterLeafData.surfaceTexInfo ], waterDepth );
			tmp.fogVolumeIndex = FindOrCreateLeafWaterData( tmp.waterLeafData.surfaceDist, tmp.waterLeafData.minZ, tmp.waterLeafData.surfaceTexInfo );

			for ( int j = 0; j < waterAreaList.Count(); j++ )
			{
				g_WaterLeafList.AddToTail( waterAreaList[j]->diskId );
			}
			waterAreaList.RemoveAll();
			g_WaterModels.AddToTail( tmp );
		}
	}

	WriteFogVolumeIDs( pModel );
}


static void ConvertWaterModelToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, int modelIndex, 
										    float shrinkSize, float mergeTolerance )
{
	dmodel_t *pModel = dmodels + modelIndex;

	for ( int i = 0; i < g_WaterModels.Count(); i++ )
	{
		watermodel_t &waterModel = g_WaterModels[i];
		if ( waterModel.modelIndex != modelIndex )
			continue;

		CPlaneList planes( shrinkSize, mergeTolerance );
		int firstLeaf = waterModel.firstWaterLeafIndex;
		planes.m_contentsMask = waterModel.contents;
		
		// push all of the leaves into the collision list
		for ( int j = 0; j < waterModel.waterLeafCount; j++ )
		{
			int leafIndex = g_WaterLeafList[firstLeaf + j];

			dleaf_t *pLeaf = dleafs + leafIndex;
			// fixup waterdata
			pLeaf->leafWaterDataID = waterModel.fogVolumeIndex;
			planes.ReferenceLeaf( leafIndex );
		}
	
		// visit the referenced leaves that belong to this model
		VisitLeaves_r( planes, pModel->headnode );

		// Now add the brushes from those leaves as convex

		// BUGBUG: NOTE: If your map has a brush that crosses the surface, it will be added to two water
		// volumes.  This only happens with connected water volumes with multiple surface heights
		// UNDONE: Right now map makers must cut such brushes.  It could be automatically cut by adding the
		// surface plane to the list for each brush before calling ConvexFromPlanes()
		planes.AddBrushes();

		int count = planes.m_convex.Count();
		if ( !count )
			continue;

		// Save off the plane of the surface for this group as well as the collision model
		// for all convex objects in the group.
		CPhysCollide *pCollide = physcollision->ConvertConvexToCollide( planes.m_convex.Base(), count );
		if ( pCollide )
		{
			int waterSurfaceTexInfoID = -1;
			// use defaults
			const char *pSurfaceProp = "water";
			float damping = 0.01;
			if ( waterSurfaceTexInfoID >= 0 )
			{
				// material override
				int texdata = texinfo[waterSurfaceTexInfoID].texdata;
				int prop = g_SurfaceProperties[texdata];
				pSurfaceProp = physprops->GetPropName( prop );
			}

			if ( !waterModel.waterLeafData.hasSurface )
			{
				waterModel.waterLeafData.surfaceNormal.Init( 0,0,1 );
				Vector top = physcollision->CollideGetExtent( pCollide, vec3_origin, vec3_angle, waterModel.waterLeafData.surfaceNormal );
				waterModel.waterLeafData.surfaceDist = top.z;
			}
			CPhysCollisionEntryFluid *pCollisionEntryFuild = new CPhysCollisionEntryFluid( pCollide, 
				pSurfaceProp, damping, waterModel.waterLeafData.surfaceNormal, waterModel.waterLeafData.surfaceDist, waterModel.contents );
			collisionList.AddToTail( pCollisionEntryFuild );
		}
	}
}

// compute a normal for a triangle of the given three points (points are clockwise, normal points out)
static Vector TriangleNormal( const Vector &p0, const Vector &p1, const Vector &p2 )
{
	Vector e0 = p1 - p0;
	Vector e1 = p2 - p0;
	Vector normal = CrossProduct( e1, e0 );
	VectorNormalize( normal );
	
	return normal;
}


// find the side of the brush with the normal closest to the given normal
static dbrushside_t *FindBrushSide( int brushIndex, const Vector &normal )
{
	dbrush_t *pbrush = &dbrushes[brushIndex];
	dbrushside_t *out = NULL;
	float best = -1.f;

	for ( int i = 0; i < pbrush->numsides; i++ )
	{
		dbrushside_t *pside = dbrushsides + i + pbrush->firstside;
		dplane_t *pplane = dplanes + pside->planenum;
		float dot = DotProduct( normal, pplane->normal );
		if ( dot > best )
		{
			best = dot;
			out = pside;
		}
	}

	return out;
}



static void ConvertWorldBrushesToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, float shrinkSize, float mergeTolerance, int contentsMask )
{
	CPlaneList planes( shrinkSize, mergeTolerance );

	planes.m_contentsMask = contentsMask;

	VisitLeaves_r( planes, dmodels[0].headnode );
	planes.AddBrushes();
	
	int count = planes.m_convex.Count();
	if ( count )
	{
		CPhysCollide *pCollide = physcollision->ConvertConvexToCollide( planes.m_convex.Base(), count );

		ICollisionQuery *pQuery = physcollision->CreateQueryModel( pCollide );
		int convex = pQuery->ConvexCount();
		for ( int i = 0; i < convex; i++ )
		{
			int triCount = pQuery->TriangleCount( i );
			int brushIndex = pQuery->GetGameData( i );

			Vector points[3];
			for ( int j = 0; j < triCount; j++ )
			{
				pQuery->GetTriangleVerts( i, j, points );
				Vector normal = TriangleNormal( points[0], points[1], points[2] );
				dbrushside_t *pside = FindBrushSide( brushIndex, normal );
				if ( pside->texinfo != TEXINFO_NODE )
				{
					int prop = g_SurfaceProperties[texinfo[pside->texinfo].texdata];
					pQuery->SetTriangleMaterialIndex( i, j, RemapWorldMaterial( prop ) );
				}
			}
		}
		physcollision->DestroyQueryModel( pQuery );
		pQuery = NULL;

		collisionList.AddToTail( new CPhysCollisionEntryStaticSolid( pCollide, contentsMask ) );
	}
}

// adds any world, terrain, and water collision models to the collision list
static void BuildWorldPhysModel( CUtlVector<CPhysCollisionEntry *> &collisionList, float shrinkSize, float mergeTolerance )
{
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, MASK_SOLID );
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, CONTENTS_PLAYERCLIP );
	ConvertWorldBrushesToPhysCollide( collisionList, shrinkSize, mergeTolerance, CONTENTS_MONSTERCLIP );

	if ( !g_bNoVirtualMesh && Disp_HasPower4Displacements() )
	{
		Warning("WARNING: Map using power 4 displacements, terrain physics cannot be compressed, map will need additional memory and CPU.\n");
		g_bNoVirtualMesh = true;
	}

	// if there's terrain, save it off as a static mesh/polysoup
	if ( g_bNoVirtualMesh || !physcollision->SupportsVirtualMesh() )
	{
		Disp_AddCollisionModels( collisionList, &dmodels[0], MASK_SOLID );
	}
	else
	{
		Disp_BuildVirtualMesh( MASK_SOLID );
	}
	ConvertWaterModelToPhysCollide( collisionList, 0, shrinkSize, mergeTolerance );
}


// adds a collision entry for this brush model
static void ConvertModelToPhysCollide( CUtlVector<CPhysCollisionEntry *> &collisionList, int modelIndex, int contents, float shrinkSize, float mergeTolerance )
{
	int i;
	CPlaneList planes( shrinkSize, mergeTolerance );

	planes.m_contentsMask = contents;

	dmodel_t *pModel = dmodels + modelIndex;
	VisitLeaves_r( planes, pModel->headnode );
	planes.AddBrushes();
	int count = planes.m_convex.Count();
	convertconvexparams_t params;
	params.Defaults();
	params.buildOuterConvexHull = count > 1 ? true : false;
	params.buildDragAxisAreas = true;
	Vector size = pModel->maxs - pModel->mins;

	float minSurfaceArea = -1.0f;
	for ( i = 0; i < 3; i++ )
	{
		int other = (i+1)%3;
		int cross = (i+2)%3;
		float surfaceArea = size[other] * size[cross];
		if ( minSurfaceArea < 0 || surfaceArea < minSurfaceArea )
		{
			minSurfaceArea = surfaceArea;
		}
	}
	// this can be really slow with super-large models and a low error tolerance
	// Basically you get a ray cast through each square of epsilon surface area on each OBB side
	// So compute it for 1% error (on the smallest side, less on larger sides)
	params.dragAreaEpsilon = clamp( minSurfaceArea * 1e-2f, 1.0f, 1024.0f );
	CPhysCollide *pCollide = physcollision->ConvertConvexToCollideParams( planes.m_convex.Base(), count, params );
	
	if ( !pCollide )
		return;

	struct 
	{
		int prop;
		float area;
	} proplist[256];
	int numprops = 1;

	proplist[0].prop = -1;
	proplist[0].area = 1;
	// compute the array of props on the surface of this model

	// NODRAW brushes no longer have any faces
	if ( !dmodels[modelIndex].numfaces )
	{
		int sideIndex = planes.GetFirstBrushSide();
		int texdata = texinfo[dbrushsides[sideIndex].texinfo].texdata;
		int prop = g_SurfaceProperties[texdata];
		proplist[numprops].prop = prop;
		proplist[numprops].area = 2;
		numprops++;
	}

	for ( i = 0; i < dmodels[modelIndex].numfaces; i++ )
	{
		dface_t *face = dfaces + i + dmodels[modelIndex].firstface;
		int texdata = texinfo[face->texinfo].texdata;
		int prop = g_SurfaceProperties[texdata];
		int j;
		for ( j = 0; j < numprops; j++ )
		{
			if ( proplist[j].prop == prop )
			{
				proplist[j].area += face->area;
				break;
			}
		}

		if ( (!numprops || j >= numprops) && numprops < ARRAYSIZE(proplist) )
		{
			proplist[numprops].prop = prop;
			proplist[numprops].area = face->area;
			numprops++;
		}
	}


	// choose the prop with the most surface area
	int maxIndex = -1;
	float maxArea = 0;
	float totalArea = 0;

	for ( i = 0; i < numprops; i++ )
	{
		if ( proplist[i].area > maxArea )
		{
			maxIndex = i;
			maxArea = proplist[i].area;
		}
		// add up the total surface area
		totalArea += proplist[i].area;
	}
	
	float mass = 1.0f;
	const char *pMaterial = "default";
	if ( maxIndex >= 0 )
	{
		int prop = proplist[maxIndex].prop;
		
		// use default if this material has no prop
		if ( prop < 0 )
			prop = 0;

		pMaterial = physprops->GetPropName( prop );
		float density, thickness;
		physprops->GetPhysicsProperties( prop, &density, &thickness, NULL, NULL );

		// if this is a "shell" material (it is hollow and encloses some empty space)
		// compute the mass with a constant surface thickness
		if ( thickness != 0 )
		{
			mass = totalArea * thickness * density * CUBIC_METERS_PER_CUBIC_INCH;
		}
		else
		{
			// material is completely solid, compute total mass as if constant density throughout.
			mass = planes.m_totalVolume * density * CUBIC_METERS_PER_CUBIC_INCH;
		}
	}

	// Clamp mass to 100,000 kg
	if ( mass > VPHYSICS_MAX_MASS )
	{
		mass = VPHYSICS_MAX_MASS;
	}

	collisionList.AddToTail( new CPhysCollisionEntrySolid( pCollide, pMaterial, mass ) );
}

static void ClearLeafWaterData( void )
{
	int i;

	for( i = 0; i < numleafs; i++ )
	{
		dleafs[i].leafWaterDataID = -1;
		dleafs[i].contents &= ~CONTENTS_TESTFOGVOLUME;
	}
}


// This is the only public entry to this file.
// The global data touched in the file is:
// from bsplib.h:
//		g_pPhysCollide		: This is an output from this file.
//		g_PhysCollideSize	: This is set in this file.
//		g_numdispinfo		: This is an input to this file.
//		g_dispinfo			: This is an input to this file.
//		numnodewaterdata	: This is an output from this file.
//		dleafwaterdata		: This is an output from this file.
// from vbsp.h:
//		g_SurfaceProperties : This is an input to this file.
void EmitPhysCollision()
{
	ClearLeafWaterData();
	
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( physicsFactory )
	{
		physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
	}

	if ( !physcollision )
	{
		Warning("!!! WARNING: Can't build collision data!\n" );
		return;
	}

	CUtlVector<CPhysCollisionEntry *> collisionList[MAX_MAP_MODELS];
	CTextBuffer *pTextBuffer[MAX_MAP_MODELS];

	int physModelCount = 0, totalSize = 0;

	int start = Plat_FloatTime();

	Msg("Building Physics collision data...\n" );

	int i, j;
	for ( i = 0; i < nummodels; i++ )
	{
		// Build a list of collision models for this brush model section
		if ( i == 0 )
		{
			// world is the only model that processes water separately.
			// other brushes are assumed to be completely solid or completely liquid
			BuildWorldPhysModel( collisionList[i], NO_SHRINK, VPHYSICS_MERGE);
		}
		else
		{
			ConvertModelToPhysCollide( collisionList[i], i, MASK_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|MASK_WATER, VPHYSICS_SHRINK, VPHYSICS_MERGE );
		}
		
		pTextBuffer[i] = NULL;
		if ( !collisionList[i].Count() )
			continue;

		// if we've got collision models, write their script for processing in the game
		pTextBuffer[i] = new CTextBuffer;
		for ( j = 0; j < collisionList[i].Count(); j++ )
		{
			// dump a text file for visualization
			if ( dumpcollide )
			{
				collisionList[i][j]->DumpCollide( pTextBuffer[i], i, j );
			}
			// each model knows how to write its script
			collisionList[i][j]->WriteToTextBuffer( pTextBuffer[i], i, j );
			// total up the binary section's size
			totalSize += collisionList[i][j]->GetCollisionBinarySize() + sizeof(int);
		}

		// These sections only appear in the world's collision text
		if ( i == 0 )
		{
			if ( !g_bNoVirtualMesh && physcollision->SupportsVirtualMesh() )
			{
				pTextBuffer[i]->WriteText("virtualterrain {}\n");
			}
			if ( s_WorldPropList.Count() )
			{
				pTextBuffer[i]->WriteText( "materialtable {\n" );
				for ( j = 0; j < s_WorldPropList.Count(); j++ )
				{
					int propIndex = s_WorldPropList[j];
					if ( propIndex < 0 )
					{
						pTextBuffer[i]->WriteIntKey( "default", j+1 );
					}
					else
					{
						pTextBuffer[i]->WriteIntKey( physprops->GetPropName( propIndex ), j+1 );
					}
				}
				pTextBuffer[i]->WriteText( "}\n" );
			}
		}

		pTextBuffer[i]->Terminate();

		// total lump size includes the text buffers (scripts)
		totalSize += pTextBuffer[i]->GetSize();

		physModelCount++;
	}

	//  add one for tail of list marker
	physModelCount++;	

	// DWORD align the lump because AddLump assumes that it is DWORD aligned.
	byte *ptr ;
	g_PhysCollideSize = totalSize + (physModelCount * sizeof(dphysmodel_t));
	g_pPhysCollide = (byte *)malloc(( g_PhysCollideSize + 3 ) & ~3 );
	memset( g_pPhysCollide, 0, g_PhysCollideSize );
	ptr = g_pPhysCollide;

	for ( i = 0; i < nummodels; i++ )
	{
		if ( pTextBuffer[i] )
		{
			int j;

			dphysmodel_t model;

			model.modelIndex = i;
			model.solidCount = collisionList[i].Count();
			model.dataSize = sizeof(int) * model.solidCount;

			for ( j = 0; j < model.solidCount; j++ )
			{
				model.dataSize += collisionList[i][j]->GetCollisionBinarySize();
			}
			model.keydataSize = pTextBuffer[i]->GetSize();

			// store the header
			memcpy( ptr, &model, sizeof(model) );
			ptr += sizeof(model);

			for ( j = 0; j < model.solidCount; j++ )
			{
				int collideSize = collisionList[i][j]->GetCollisionBinarySize();

				// write size
				memcpy( ptr, &collideSize, sizeof(int) );
				ptr += sizeof(int);

				// now write the collision model
				collisionList[i][j]->WriteCollisionBinary( reinterpret_cast<char *>(ptr) );
				ptr += collideSize;
			}

			memcpy( ptr, pTextBuffer[i]->GetData(), pTextBuffer[i]->GetSize() );
			ptr += pTextBuffer[i]->GetSize();
		}

		delete pTextBuffer[i];
	}

	dphysmodel_t model;

	// Mark end of list
	model.modelIndex = -1;
	model.dataSize = -1;
	model.keydataSize = 0;
	model.solidCount = 0;
	memcpy( ptr, &model, sizeof(model) );
	ptr += sizeof(model);
	Assert( (ptr-g_pPhysCollide) == g_PhysCollideSize);
	Msg("done (%d) (%d bytes)\n", (int)(Plat_FloatTime() - start), g_PhysCollideSize );

	// UNDONE: Collision models (collisionList) memory leak!
}
